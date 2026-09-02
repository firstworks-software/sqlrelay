// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for the third call site #9585 fixed: the fetchRow()
// error branch inside sendFetchResponse() in src/protocols/oracle.cpp.  It
// used to treat a backend error as end of data and answer ORA-01403; it now
// answers sendQueryError(cursor).
//
// #9601 covered the other two call sites (sendQuery3Response() and
// sendFetch3Response()) with the OCI test in this directory.  It could not
// reach this one.  fetch() routes to fetch3() as soon as query3session is
// true, and that flag is set by the first TTI_QUERY3 a session sends - which
// every OCI client from 10g onward sends, and no other kind of client exists
// in this environment.  So the legacy path is only reachable from a client
// that speaks the pre-query3 wire shape on purpose, which is what this is:
// oracleprotocolclient's hand-built login, then TTI_QUERY, TTI_EXECUTE and
// the legacy TTI_FETCH straight onto the socket, and never a TTI_QUERY3.
//
// The instance is oracleprotocolfetchatonce (ORACLEPROTOCOLPORT5), whose
// fetchatonce=1 puts src/connections/oracle.cpp's fetchRow() into one
// physical fetch per logical row.  That is what makes the failing row fail
// where this test needs it to: legacy execute() never calls fetchRow() at
// all, so with one row per fetch the divide-by-zero necessarily lands on the
// second pass of sendFetchResponse()'s do-while loop.

// the fixed parts of the legacy responses, from the builders in
// src/protocols/oracle.cpp - putError()'s unknown1 block, putGenericFooter()
// and sendFetchResponse()'s non-exact-fetch trailer
static const size_t	ORA_ERROR_BLOCK_SIZE=48;
static const size_t	ORA_GENERIC_FOOTER_SIZE=41;

// putError() writes the ora number little-endian into the block above, at
// its offsets 4 and 5
static const size_t	ORA_ERROR_BLOCK_NUMBER_OFFSET=4;

// the byte putError() writes behind the message
static const unsigned char	ORA_ERROR_MESSAGE_TERMINATOR=0x0a;

// sendQueryResponse()'s and sendExecuteResponse()'s bodies are both fixed
// size - no message, no rows, nothing variable-length.  a legacy error body
// is always ORA_ERROR_BLOCK_SIZE + ORA_GENERIC_FOOTER_SIZE + a message
// bigger than either of these, so checking the exact size (rather than the
// ttc code, which sendQueryResponse()/sendExecuteResponse() and the error
// path all set to the same 0x04) is what actually tells a genuine parse or
// execute success apart from an error answering in its place
static const size_t	ORA_QUERY_RESPONSE_SIZE=92;
static const size_t	ORA_EXECUTE_RESPONSE_SIZE=56;

// the marker sendFetchResponse() writes in front of every row
static const unsigned char	ORA_ROW_MARKER=0x07;

// the errors this test tells apart: the divide by zero the query really
// raises, and the end-of-data the unfixed code answered with instead
static const uint32_t	ORA_DIVISOR_IS_EQUAL_TO_ZERO=1476;
static const uint32_t	ORA_NO_DATA_FOUND=1403;

// three rows of one plain NUMBER column, all of which evaluate
static const char	*goodquery=
	"select level from dual connect by level<=3";

// the same shape, but row two divides by zero.  oracle only raises it when
// it actually produces that row, so the parse and the execute both succeed
// and the failure surfaces inside the fetch
static const char	*badquery=
	"select 1/(level-2) from dual connect by level<=3";

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

// oracle's number format back to an integer - the inverse of
// putNumberField() in src/protocols/oracle.cpp, for the integer cases this
// test's two queries produce.  a positive number's exponent byte is 193+e
// and its base 100 digits are each digit+1; a negative number's is 62-e,
// its digits are each 101-digit, and a 0x66 terminator follows them
static bool oracleNumberToInteger(const unsigned char *bytes,
					size_t size,
					int64_t *value) {

	if (!size) {
		return false;
	}

	if (size==1 && bytes[0]==0x80) {
		*value=0;
		return true;
	}

	bool	negative=(bytes[0]<0x80);
	int32_t	exponent=(negative)?(62-(int32_t)bytes[0]):
					((int32_t)bytes[0]-193);

	size_t	digitcount=size-1;
	if (negative && digitcount && bytes[size-1]==0x66) {
		digitcount--;
	}

	// a fraction would need the exponent to run past the digits sent;
	// this test's queries produce whole numbers only.  the exponent has
	// to be checked before the cast - a negative one would wrap to a
	// huge size_t, sail past the comparison and spin the padding loop
	// below
	if (exponent<0 || (size_t)(exponent+1)<digitcount) {
		return false;
	}

	int64_t	result=0;
	for (size_t i=0; i<digitcount; i++) {
		unsigned char	d=(negative)?(unsigned char)(101-bytes[i+1]):
					(unsigned char)(bytes[i+1]-1);
		result=result*100+(int64_t)d;
	}
	for (size_t i=digitcount; i<(size_t)(exponent+1); i++) {
		result=result*100;
	}

	*value=(negative)?-result:result;
	return true;
}

// a substring search over raw bytes, which responseContains() can't do -
// the row bytes this test looks for the absence of contain zeros
static bool responseContainsBytes(oracleprotocolclient *client,
					const unsigned char *bytes,
					size_t size) {
	const unsigned char	*response=client->getResponse();
	size_t			responsesize=client->getResponseSize();
	if (!size || responsesize<size) {
		return false;
	}
	for (size_t i=0; i<=responsesize-size; i++) {
		if (!bytestring::compare(response+i,bytes,size)) {
			return true;
		}
	}
	return false;
}

// walk the plainest legacy fetch response - the one a fetch with no options
// asks for - and collect the value of the single column of each row it
// carries.  see sendFetchResponse() in src/protocols/oracle.cpp: the data
// flags, two filler bytes standing in for the iov, three more fixed bytes,
// the column count, seven more fixed bytes, then a marker and a row for
// each row, then a trailer that starts with something other than the marker
static bool readLegacyFetchRows(oracleprotocolclient *client,
					int64_t *values,
					size_t maxvalues,
					size_t *valuecount,
					uint32_t *colcount) {

	client->rewindResponse();

	unsigned char	skip[7];
	unsigned char	columns=0;
	if (!client->readBytes(skip,2) ||	// data flags
		!client->readBytes(skip,2) ||	// no iov
		!client->readBytes(skip,3) ||	// unknown2, unknown3
		!client->readByte(&columns) ||
		!client->readBytes(skip,7)) {	// unknown4
		return false;
	}
	*colcount=columns;

	*valuecount=0;
	for (;;) {

		unsigned char	marker=0;
		if (!client->readByte(&marker)) {
			return false;
		}
		if (marker!=ORA_ROW_MARKER) {
			// the trailer, so the rows are done
			return true;
		}

		unsigned char	numbersize=0;
		unsigned char	number[32];
		if (!client->readByte(&numbersize) ||
			numbersize>sizeof(number) ||
			!client->readBytes(number,numbersize)) {
			return false;
		}

		if (*valuecount>=maxvalues) {
			return false;
		}
		if (!oracleNumberToInteger(number,numbersize,
						&values[*valuecount])) {
			return false;
		}
		(*valuecount)++;
	}
}

// walk sendQueryError()'s legacy answer - the data flags, then putError()'s
// ttc code, fixed block, message and terminator, then putGenericFooter()
static bool readLegacyError(oracleprotocolclient *client,
				uint32_t *oranum,
				char *message,
				size_t messagemax,
				size_t *messagesize,
				size_t *totalsize) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	unsigned char	block[ORA_ERROR_BLOCK_SIZE];
	unsigned char	size=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ERROR ||
		!client->readBytes(block,sizeof(block)) ||
		!client->readByte(&size) ||
		(size_t)size>=messagemax) {
		return false;
	}

	*oranum=(uint32_t)block[ORA_ERROR_BLOCK_NUMBER_OFFSET]|
		((uint32_t)block[ORA_ERROR_BLOCK_NUMBER_OFFSET+1]<<8);

	if (!client->readBytes((unsigned char *)message,size)) {
		return false;
	}
	message[size]='\0';
	*messagesize=size;

	unsigned char	terminator=0;
	if (!client->readByte(&terminator) ||
		terminator!=ORA_ERROR_MESSAGE_TERMINATOR) {
		return false;
	}

	*totalsize=sizeof(dataflags)+1+sizeof(block)+1+
			(size_t)size+1+ORA_GENERIC_FOOTER_SIZE;
	return true;
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #9585 legacy fetch error path ======\n\n");

	// the oracleprotocolfetchatonce instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  as with
	// oracledescribeonly, it isn't a real oracle server, just a listener
	// speaking oracle's wire protocol, and ORACLEPROTOCOLPORT5 names the
	// port it ended up on
	const char	*host="127.0.0.1";
	uint16_t	port=1525;
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	const char	*portoverride=
			environment::getValue("ORACLEPROTOCOLPORT5");
	if (!charstring::isNullOrEmpty(portoverride)) {
		port=(uint16_t)charstring::convertToInteger(portoverride);
	}

	oracleprotocolclient	client;

	if (!client.connect(host,port,sid)) {
		report("connect",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("connect",true);

	if (!client.login(user,password)) {
		report("login",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("login",true);

	uint32_t	cursorid=0;
	if (!client.open(&cursorid)) {
		report("open cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open cursor",true);


	// first, a query that can't fail, to prove the hand-built legacy
	// request layouts are right and the response decoding below really
	// is reading rows.  without this, an error answer to the failing
	// query proves nothing - a request the listener couldn't parse
	// would produce one too
	if (!client.legacyQuery(ORA_OPTION_PARSE,0,
					(uint16_t)cursorid,goodquery)) {
		report("parse",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("parse",client.getResponseSize()==ORA_QUERY_RESPONSE_SIZE);

	if (!client.legacyExecute(ORA_OPTION_EXECUTE,0,(uint16_t)cursorid)) {
		report("execute",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute",client.getResponseSize()==ORA_EXECUTE_RESPONSE_SIZE);

	// no options at all: no column definitions, no iov, and the
	// non-exact-fetch trailer.  #9609 left the exact-fetch trailer and
	// putLobField() unverified, so this stays clear of both
	if (!client.legacyFetch(0,0)) {
		report("fetch",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}

	int64_t	values[8];
	size_t	valuecount=0;
	uint32_t	colcount=0;
	bool	decoded=readLegacyFetchRows(&client,values,
					sizeof(values)/sizeof(values[0]),
					&valuecount,&colcount);
	report("fetch response decodes",decoded);
	if (!decoded) {
		stdoutput.printf("response (%d bytes):\n",
					(int)client.getResponseSize());
		stdoutput.safePrint(client.getResponse(),
					client.getResponseSize());
		stdoutput.printf("\n");
		client.disconnect();
		return status;
	}
	report("fetch response has one column",colcount==1);
	report("fetch response carries three rows",valuecount==3);
	for (size_t i=0; i<valuecount; i++) {
		stdoutput.printf("  row %d: %lld\n",
					(int)i+1,(long long)values[i]);
	}
	report("fetch response carries 1, 2, 3",
			valuecount==3 &&
			values[0]==1 && values[1]==2 && values[2]==3);


	// and now the query this test exists for.  a fresh cursor, so
	// nothing the first one holds can explain the answer
	uint32_t	badcursorid=0;
	if (!client.open(&badcursorid)) {
		report("open second cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open second cursor",true);

	if (!client.legacyQuery(ORA_OPTION_PARSE,0,
					(uint16_t)badcursorid,badquery)) {
		report("parse failing query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("parse failing query",
			client.getResponseSize()==ORA_QUERY_RESPONSE_SIZE);

	// the execute has to succeed - legacy execute() never fetches a row,
	// so nothing has divided by zero yet.  an error here would mean the
	// failure moved off the path this test covers
	if (!client.legacyExecute(ORA_OPTION_EXECUTE,0,
						(uint16_t)badcursorid)) {
		report("execute failing query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute failing query",
			client.getResponseSize()==ORA_EXECUTE_RESPONSE_SIZE);

	if (!client.legacyFetch(0,0)) {
		report("fetch failing query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}

	uint32_t	oranum=0;
	char		message[512];
	size_t		messagesize=0;
	size_t		expectedsize=0;
	bool		iserror=readLegacyError(&client,&oranum,
						message,sizeof(message),
						&messagesize,&expectedsize);
	report("fetch answers with an error",iserror);
	if (!iserror) {
		stdoutput.printf("response (%d bytes):\n",
					(int)client.getResponseSize());
		stdoutput.safePrint(client.getResponse(),
					client.getResponseSize());
		stdoutput.printf("\n");
		client.disconnect();
		return status;
	}
	stdoutput.printf("  ora number: %d\n",(int)oranum);
	stdoutput.printf("  message: %s\n",message);

	report("the error is the divide by zero",
			oranum==ORA_DIVISOR_IS_EQUAL_TO_ZERO);
	report("the error is not end of data",
			oranum!=ORA_NO_DATA_FOUND);
	report("the message is the backend's",
			charstring::contains(message,
					"divisor is equal to zero"));

	// the substantive assertion.  sendFetchResponse() appends the fetch
	// header and row one into its buffer before row two fails;
	// sendQueryError() calls resetSendPacketBuffer() first, so none of
	// that can reach the wire.  a response that still carries it would
	// be a partial fetch with an error stuck on the end, which is a
	// packet no client can parse
	static const unsigned char	fetchheader[]={
		0x00, 0x00, 0x06, 0x02, 0x8c
	};
	report("no fetch response header reached the wire",
			!responseContainsBytes(&client,fetchheader,
						sizeof(fetchheader)));

	// row one is -1, which putNumberField() writes as 3e 64 66 behind a
	// length byte, behind the row marker
	static const unsigned char	rowone[]={
		ORA_ROW_MARKER, 0x03, 0x3e, 0x64, 0x66
	};
	report("no row one residue reached the wire",
			!responseContainsBytes(&client,rowone,sizeof(rowone)));

	// and nothing else did either - the response is exactly the error
	// and its footer, to the byte
	report("the response is exactly the error and its footer",
			client.getResponseSize()==expectedsize);
	if (client.getResponseSize()!=expectedsize) {
		stdoutput.printf("  %d bytes, expected %d\n",
					(int)client.getResponseSize(),
					(int)expectedsize);
	}

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
