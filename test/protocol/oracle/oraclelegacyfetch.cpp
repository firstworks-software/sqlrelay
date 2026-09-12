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

// sendQueryResponse() and sendExecuteResponse() (src/protocols/oracle.cpp)
// each answer with nothing but a two-byte data flags header and a
// putOci7Summary() object - no message, no rows, nothing else - and a
// genuine error answers with that same object, behind the same leading
// TTC_ERROR (0x04) byte, plus a message.  so the ttc code can't tell a
// success from an error here, and neither can a fixed total size: the
// summary object's own fields (cursor id, rows processed, parse error
// offset) are themselves length-prefixed and vary in width from one
// answer to the next.  what does tell them apart is walking the object
// field by field, the way readLegacyError() below walks the matching
// fields in the error object, and checking nothing is left over once the
// walk ends - a genuine success is a summary object and nothing else,
// where a genuine error is a summary object plus a message.
// readLegacySummary() below does that walk for both call sites

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

// a sequence of this test's own, for the describe-before-fetch arm below.
// a describe that re-executes the statement rewinds the result set, which
// is invisible on a plain table - the fetch behind it answers the same row
// whether the statement ran once or twice.  a nextval answers a different
// value every execute, so the arm's own fetch says how many executes it
// took to get there
static const char	*sequencequery=
	"select protocolseq10003.nextval from dual";
static const char	*dropsequence=
	"drop sequence protocolseq10003";
static const char	*createsequence=
	"create sequence protocolseq10003 start with 7654321 increment by 1";
static const int64_t	firstnextval=7654321;
static const int64_t	secondnextval=7654322;

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
//
// the header's row count comes back in "headerrowcount" rather than being
// skipped with the rest: a fetch that named a row count gets that count back
// there, and a fetch that asked for 0 - every row there is - gets the number
// of rows that really follow, which is the only thing in the response that
// says how many the client was sent
static bool readLegacyFetchRows(oracleprotocolclient *client,
					int64_t *values,
					size_t maxvalues,
					size_t *valuecount,
					uint32_t *colcount,
					uint32_t *headerrowcount) {

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
		!client->readLenPreInt(headerrowcount) ||	// row count
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

// walk sendQueryResponse()'s and sendExecuteResponse()'s answer - the data
// flags, then the ttc code and the same putOci7Summary() fields
// readLegacyError() above walks for the error object, but nothing after
// them.  see the note on this function's call sites in main() below: a
// genuine parse or execute success is this object and nothing else; a
// genuine error is this object plus a message, which is what "leftover"
// catches
static bool readLegacySummary(oracleprotocolclient *client,
				uint32_t *cursorid,
				unsigned char *commandtype,
				uint32_t *rowsprocessed,
				uint32_t *successiterations,
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
		!client->readLenPreInt(rowsprocessed) ||
		!client->readLenPreInt(&skipint) ||	// error number
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(cursorid) ||
		!client->readLenPreInt(&skipint) ||	// parse error offset
		!client->readByte(commandtype) ||
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
		!client->readLenPreInt(successiterations) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint)) {
		return false;
	}

	unsigned char	extra=0;
	*leftover=client->readByte(&extra);
	return true;
}

// a parse or execute response decodes as a summary answering the given
// cursor, with the given rows-processed and success-iteration counts
// (both known ahead of time here - every query in this file is a select,
// so a parse always sends 0/0 and an execute always sends 0/1, per
// sendQueryResponse()/sendExecuteResponse() in src/protocols/oracle.cpp),
// a command type of 3 (parse or execute of a select), and nothing left
// over
static bool checkLegacySummaryResponse(oracleprotocolclient *client,
					uint32_t expectedcursorid,
					uint32_t expectedrowsprocessed,
					uint32_t expectedsuccessiterations) {

	uint32_t	curid=0;
	unsigned char	commandtype=0;
	uint32_t	rowsprocessed=0;
	uint32_t	successiterations=0;
	bool		leftover=false;
	return readLegacySummary(client,&curid,&commandtype,
					&rowsprocessed,&successiterations,
					&leftover) &&
			curid==expectedcursorid &&
			commandtype==3 &&
			rowsprocessed==expectedrowsprocessed &&
			successiterations==expectedsuccessiterations &&
			!leftover;
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
	report("parse",checkLegacySummaryResponse(&client,cursorid,0,0));

	if (!client.legacyExecute(cursorid,1,0)) {
		report("execute",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute",checkLegacySummaryResponse(&client,cursorid,0,1));

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
	uint32_t	headerrows=0;
	bool	decoded=readLegacyFetchRows(&client,values,
					sizeof(values)/sizeof(values[0]),
					&valuecount,&colcount,&headerrows);
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

	// a fetch that asked for 0 rows asked for every row there is, so the
	// header's row count is the count that came out of the fetch loop and
	// nothing the client named.  an ofen() caller reads that many rows out
	// of its define buffers, so a header saying 1 in front of three rows
	// loses the other two - see sendFetchResponse() in
	// src/protocols/oracle.cpp
	stdoutput.printf("  header row count: %d\n",(int)headerrows);
	report("the row header counts the rows that followed",
			headerrows==valuecount);


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
			checkLegacySummaryResponse(&client,badcursorid,0,0));

	// the execute has to succeed - legacy execute() never fetches a row,
	// so nothing has divided by zero yet.  an error here would mean the
	// failure moved off the path this test covers
	if (!client.legacyExecute(badcursorid,1,0)) {
		report("execute failing query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute failing query",
			checkLegacySummaryResponse(&client,badcursorid,0,1));

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


	// Regression coverage for the same describe-before-fetch hazard #9973
	// fixed in query2(), but on the bare legacy TTI_EXECUTE path instead:
	// execute() in src/protocols/oracle.cpp never called
	// cacheColumnDefinitions() after a successful execute, so a
	// TTI_DESCRIBE landing between that TTI_EXECUTE and the client's
	// first TTI_FETCH found columntypescached[curid] still false and
	// re-ran the statement - a rewind invisible on most result sets, but
	// not on one drawn from a sequence.  #10001 added the missing call;
	// this drives the scenario it had no test for.
	//
	// the setup and the arm both stay on the legacy TTI_QUERY/TTI_EXECUTE
	// pair on purpose.  a query3()/reexecute() call anywhere in the
	// session would flip query3session in src/protocols/oracle.cpp and
	// hand every TTI_EXECUTE after it - including the one this arm means
	// to exercise - to reexecute() instead, which already caches its
	// column definitions and so can't reproduce the gap
	uint32_t	seqcursorid=0;
	if (!client.open(&seqcursorid)) {
		report("open third cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open third cursor",true);

	// a drop of a sequence that isn't there answers with an error and
	// leaves the session running, so its result is deliberately not
	// checked
	client.legacyQuery(seqcursorid,dropsequence);
	client.legacyExecute(seqcursorid,1,0);

	// dropped and recreated rather than reset, so the first nextval the
	// arm below asks for is the start value whatever a previous run left
	// behind.  a legacy execute's response carries no ttc code of its
	// own to check - see readLegacySummary() above - so only the wire
	// call itself is checked here; a create that silently failed still
	// shows up below, once the sequence it was supposed to create can't
	// be parsed
	if (!client.legacyQuery(seqcursorid,createsequence)) {
		report("create sequence",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	if (!client.legacyExecute(seqcursorid,1,0)) {
		report("create sequence",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("create sequence",true);

	// parse, execute, describe, then fetch - the describe lands exactly
	// where the gap used to be
	if (!client.legacyQuery(seqcursorid,sequencequery)) {
		report("parse sequence query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	if (!client.legacyExecute(seqcursorid,1,0)) {
		report("execute sequence query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute sequence query",
			checkLegacySummaryResponse(&client,seqcursorid,0,1));

	if (!client.describe(seqcursorid,1)) {
		report("describe sequence query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("describe sequence query answers ok",
			client.getResponseTtcCode()==ORA_TTC_OK);

	if (!client.legacyFetch(seqcursorid,0)) {
		report("fetch sequence query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}

	int64_t	seqvalues[8];
	size_t	seqvaluecount=0;
	uint32_t	seqcolcount=0;
	uint32_t	seqheaderrows=0;
	bool	seqdecoded=readLegacyFetchRows(&client,seqvalues,
					sizeof(seqvalues)/sizeof(seqvalues[0]),
					&seqvaluecount,&seqcolcount,
					&seqheaderrows);
	report("sequence query fetch response decodes",seqdecoded);
	if (!seqdecoded) {
		stdoutput.printf("response (%d bytes):\n",
					(int)client.getResponseSize());
		stdoutput.safePrint(client.getResponse(),
					client.getResponseSize());
		stdoutput.printf("\n");
		client.disconnect();
		return status;
	}
	report("sequence query fetch response has one column",
			seqcolcount==1);
	report("sequence query fetch response has one row",
			seqvaluecount==1);
	stdoutput.printf("  value: %lld\n",
			(seqvaluecount)?(long long)seqvalues[0]:0LL);
	report("the describe didn't re-execute the sequence query",
			seqvaluecount==1 && seqvalues[0]==firstnextval);


	// the same parse, execute and fetch with no describe in between.
	// without this, the arm above's first-sequence-value answer would
	// only show the sequence started where it was created - this shows
	// the sequence really advances by one value per execute, which is
	// what makes "still the first value" after a describe mean the
	// describe cost the statement nothing
	if (!client.legacyQuery(seqcursorid,sequencequery)) {
		report("parse sequence query (control)",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	if (!client.legacyExecute(seqcursorid,1,0)) {
		report("execute sequence query (control)",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute sequence query (control)",
			checkLegacySummaryResponse(&client,seqcursorid,0,1));

	if (!client.legacyFetch(seqcursorid,0)) {
		report("fetch sequence query (control)",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}

	bool	controldecoded=readLegacyFetchRows(&client,seqvalues,
					sizeof(seqvalues)/sizeof(seqvalues[0]),
					&seqvaluecount,&seqcolcount,
					&seqheaderrows);
	report("sequence query (control) fetch response decodes",
			controldecoded);
	if (!controldecoded) {
		stdoutput.printf("response (%d bytes):\n",
					(int)client.getResponseSize());
		stdoutput.safePrint(client.getResponse(),
					client.getResponseSize());
		stdoutput.printf("\n");
		client.disconnect();
		return status;
	}
	stdoutput.printf("  value: %lld\n",
			(seqvaluecount)?(long long)seqvalues[0]:0LL);
	report("sequence query (control) carries the value behind "
			"the arm above's",
			seqvaluecount==1 && seqvalues[0]==secondnextval);


	// the other half of the row header's row-count rule, on a fetch that
	// did name a count.  there the header carries the count the client
	// asked for and not the count that follows, which is what a real
	// server sends: [0065] of test/protocol/oracle/samples/
	// 10030-redhat9x86-native-multirowfetch-realserver.oraproxy asks a
	// three row result set for five, and [0066] answers with a header
	// saying five, three rows and ORA-01403.  this arm asks the same
	// three row result set for five too, so the header (5) and the rows
	// that actually come back (3) have to differ - asking for exactly
	// what came back, the way this arm used to, can't tell the asked-for
	// count from the delivered one
	uint32_t	boundedcursorid=0;
	if (!client.open(&boundedcursorid)) {
		report("open fourth cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open fourth cursor",true);

	if (!client.legacyQuery(boundedcursorid,goodquery)) {
		report("parse bounded fetch query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("parse bounded fetch query",
			checkLegacySummaryResponse(&client,boundedcursorid,0,0));

	if (!client.legacyExecute(boundedcursorid,1,0)) {
		report("execute bounded fetch query",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute bounded fetch query",
			checkLegacySummaryResponse(&client,boundedcursorid,0,1));

	// five, though the query only has three rows
	if (!client.legacyFetch(boundedcursorid,5)) {
		report("bounded fetch",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}

	int64_t	boundedvalues[8];
	size_t	boundedvaluecount=0;
	uint32_t	boundedcolcount=0;
	uint32_t	boundedheaderrows=0;
	bool	boundeddecoded=readLegacyFetchRows(&client,boundedvalues,
				sizeof(boundedvalues)/sizeof(boundedvalues[0]),
				&boundedvaluecount,&boundedcolcount,
				&boundedheaderrows);
	report("bounded fetch response decodes",boundeddecoded);
	if (!boundeddecoded) {
		stdoutput.printf("response (%d bytes):\n",
					(int)client.getResponseSize());
		stdoutput.safePrint(client.getResponse(),
					client.getResponseSize());
		stdoutput.printf("\n");
		client.disconnect();
		return status;
	}
	stdoutput.printf("  header row count: %d\n",(int)boundedheaderrows);
	for (size_t i=0; i<boundedvaluecount; i++) {
		stdoutput.printf("  row %d: %lld\n",
					(int)i+1,(long long)boundedvalues[i]);
	}
	report("bounded fetch response carries the three rows there are",
			boundedvaluecount==3);
	report("bounded fetch response carries 1, 2, 3",
			boundedvaluecount==3 &&
			boundedvalues[0]==1 && boundedvalues[1]==2 &&
			boundedvalues[2]==3);
	report("the row header counts the rows the client asked for",
			boundedheaderrows==5);


	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
