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

// sendQueryResponse()'s and sendExecuteResponse()'s bodies are both fixed
// size - no message, no rows, nothing variable-length - and neither is the
// size a legacy error body comes out at, which is the summary object plus
// the backend's message.  so checking the exact size (rather than the ttc
// code, which sendQueryResponse()/sendExecuteResponse() and the error path
// all set to the same 0x04) is what actually tells a genuine parse or
// execute success apart from an error answering in its place
static const size_t	ORA_QUERY_RESPONSE_SIZE=33;
static const size_t	ORA_EXECUTE_RESPONSE_SIZE=34;

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

// a legacy fetch's NUMBER column, back to an integer.  putField() in
// src/protocols/oracle.cpp writes a NUMBER as the ascii digits the backend
// handed back, not as putNumberField()'s base-100 form - "a legacy client
// asks the server to convert" - so this is a plain decimal parse, not
// putNumberField()'s inverse.  #9637's legacy-fetch captures against a real
// 10.2 server confirm it: "select 1, 2, 3 from dual" comes back as the clrs
// "01 31", "01 32" and "01 33" - one-byte clrs holding the ascii digits -
// where putNumberField() would have written "02 c1 02" for the first of them
static bool oracleNumberToInteger(const unsigned char *bytes,
					size_t size,
					int64_t *value) {

	if (!size) {
		return false;
	}

	bool	negative=(bytes[0]=='-');
	size_t	i=(negative)?1:0;

	if (i>=size) {
		return false;
	}

	int64_t	result=0;
	for (; i<size; i++) {
		if (bytes[i]<'0' || bytes[i]>'9') {
			return false;
		}
		result=result*10+(int64_t)(bytes[i]-'0');
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
// carries.  see sendFetchResponse()/putRowHeader() in src/protocols/oracle.cpp:
// the data flags, then TTC_ROW_HEADER and its flags byte, then six
// length-prefixed counts (column count, iteration number, row count, uac
// buffer length, bit vector size and one more of unknown meaning) - it has
// no fixed size, so it has to be walked field by field the way
// readLegacyError() below walks the error object, not skipped as a fixed
// number of bytes.  a marker and a row follow for each row, then a trailer
// that starts with something other than the marker.
//
// putRow() writes two more length-prefixed fields behind every column's
// value - the indicator and the return code odefin() gave the client a
// pointer for - both always 0 here, since this test never odefin's
// anything.  skipping only the value and landing on these as though they
// were the next row's marker is what made the three-row query decode a
// single, wrong row: 0x00 isn't ORA_ROW_MARKER, so the walk below stopped
// after row one every time.
//
// the row loop below reads exactly one value/indicator/returncode triple
// per marker, so it only walks a genuine one-column result - the shape
// *colcount is checked against in main() below.  a legacy fetch with more
// than one defined column would need an inner loop over *colcount here;
// nothing in this file exercises that
static bool readLegacyFetchRows(oracleprotocolclient *client,
					int64_t *values,
					size_t maxvalues,
					size_t *valuecount,
					uint32_t *colcount) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	unsigned char	flags=0;
	uint32_t	skipint=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ROW_HEADER ||
		!client->readByte(&flags) ||
		!client->readLenPreInt(colcount) ||	// column count
		!client->readLenPreInt(&skipint) ||	// iteration number
		!client->readLenPreInt(&skipint) ||	// row count
		!client->readLenPreInt(&skipint) ||	// uac buffer length
		!client->readLenPreInt(&skipint) ||	// bit vector size
		!client->readLenPreInt(&skipint)) {
		return false;
	}

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
		uint32_t	indicator=0;
		uint32_t	returncode=0;
		if (!client->readByte(&numbersize) ||
			numbersize>sizeof(number) ||
			!client->readBytes(number,numbersize) ||
			!client->readLenPreInt(&indicator) ||
			!client->readLenPreInt(&returncode)) {
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

// walk sendQueryError()'s legacy answer - the data flags, then the ttc code
// and the summary object putOci7Error() writes, field for field, then the
// message.  the object's fields are length-prefixed ints in this encoding,
// so it has no fixed size and has to be walked rather than skipped: see
// putOci7Summary() in src/protocols/oracle.cpp, which this mirrors.
//
// "leftover" says whether anything at all follows the message.  nothing
// should - #9976 found that what used to answer here ran 21 bytes past the
// end of the object the client parses, which cost the client the call and
// turned up on the next one as ORA-03120
static bool readLegacyError(oracleprotocolclient *client,
				uint32_t *oranum,
				char *message,
				size_t messagemax,
				size_t *messagesize,
				bool *leftover) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	uint32_t	skipint=0;
	unsigned char	skipbyte=0;
	unsigned char	skipbytes[5];
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ERROR ||
		!client->readLenPreInt(&skipint) ||	// end of call status
		!client->readLenPreInt(&skipint) ||	// rows processed
		!client->readLenPreInt(oranum) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||	// cursor id
		!client->readLenPreInt(&skipint) ||	// parse error offset
		!client->readByte(&skipbyte) ||		// command type
		!client->readBytes(skipbytes,5) ||
		// the rowid - a ub4, a ub2, a raw byte, a ub4 and a ub2
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readByte(&skipbyte) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readByte(&skipbyte) ||
		!client->readByte(&skipbyte) ||		// call number
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||	// success iterations
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint)) {
		return false;
	}

	unsigned char	size=0;
	if (!client->readByte(&size) || (size_t)size>=messagemax ||
		!client->readBytes((unsigned char *)message,size)) {
		return false;
	}
	message[size]='\0';
	*messagesize=size;

	unsigned char	extra=0;
	*leftover=client->readByte(&extra);
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
	if (!client.legacyQuery(cursorid,goodquery)) {
		report("parse",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("parse",client.getResponseSize()==ORA_QUERY_RESPONSE_SIZE);

	if (!client.legacyExecute(cursorid,1,0)) {
		report("execute",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute",client.getResponseSize()==ORA_EXECUTE_RESPONSE_SIZE);

	// no options at all: no column definitions, no iov, and the
	// non-exact-fetch trailer.  #9609 left the exact-fetch trailer and
	// putLobField() unverified, so this stays clear of both
	if (!client.legacyFetch(cursorid,0)) {
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

	if (!client.legacyQuery(badcursorid,badquery)) {
		report("parse failing query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("parse failing query",
			client.getResponseSize()==ORA_QUERY_RESPONSE_SIZE);

	// the execute has to succeed - legacy execute() never fetches a row,
	// so nothing has divided by zero yet.  an error here would mean the
	// failure moved off the path this test covers
	if (!client.legacyExecute(badcursorid,1,0)) {
		report("execute failing query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute failing query",
			client.getResponseSize()==ORA_EXECUTE_RESPONSE_SIZE);

	if (!client.legacyFetch(badcursorid,0)) {
		report("fetch failing query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}

	uint32_t	oranum=0;
	char		message[512];
	size_t		messagesize=0;
	bool		leftover=false;
	bool		iserror=readLegacyError(&client,&oranum,
						message,sizeof(message),
						&messagesize,&leftover);
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

	// row one is -1, which putField() writes as the ascii text "-1" -
	// see oracleNumberToInteger() above - behind a length byte, behind
	// the row marker.  putNumberField()'s binary "3e 64 66" never goes
	// out on this path at all, so checking for it here would pass
	// whether or not the real bytes leaked
	static const unsigned char	rowone[]={
		ORA_ROW_MARKER, 0x02, 0x2d, 0x31
	};
	report("no row one residue reached the wire",
			!responseContainsBytes(&client,rowone,sizeof(rowone)));

	// and nothing else did either - the walk above consumed the whole
	// response, ending on the last byte of the message.  #9817's rule:
	// a client's parse has to account for every byte, and anything left
	// over costs it the call
	report("nothing follows the error object",!leftover);

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
