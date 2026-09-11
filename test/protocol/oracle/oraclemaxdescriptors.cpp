// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for #10059: a TTI_QUERY2 whose descriptor block names
// more define positions than the connection's maxcolumncount allows, or more
// binds than its maxbindcount does.
//
// getQuery2Descriptors() in src/protocols/oracle.cpp walks that block and
// bails on either limit.  Both bails used to return quietly, and the define
// one was the damaging half: the clearDefines() at the top of the walk had
// already run, definecounts[] was never reached, and query2rowcount was left
// at 0.  So query2()'s OPTION_FETCH path went on to call
// sendFetchResponse(cursor,true,0), which sends every column - unconverted,
// the define list being gone - and, with no row count to run to, stops at the
// first full packet and reports success.  A client that asked for a batch of
// rows got a short one back with no error anywhere and no way to ask for the
// rest.  Both bails now set a request-scoped flag that query2() answers
// before the statement is prepared or executed: ORA-20002 for the defines and
// ORA-20003 for the binds, sent through sendOci7StatementError() as the
// summary object every other oci7 call is answered with.
//
// The two limits are configured differently but default to 256 apiece.
// maxcolumncount is a connect-string value, MAX_COLUMN_COUNT in
// src/server/sqlrserverconnection.cpp; maxbindcount is an instance
// attribute, DEFAULT_MAXBINDCOUNT in src/common/defaults.h.  The
// oracleprotocol instance sets neither (see
// test/sqlrelay.conf.d/oracleprotocol.conf.in), so 257 of either is one past
// the limit and no instance of its own is needed.
//
// Three cases, each on its own session:
//
//	- the defined batch, which is the control.  a define list well
//	  within the limit, and an exact fetch of every row the statement
//	  has, on a 512 byte sdu that no single packet of that batch can
//	  fit.  it is the normal path the two refusals below sit beside,
//	  and it is what says this harness would see a batch come back
//	  short - the pre-fix failure is a short batch, so a case that
//	  can't tell a whole batch from one packet's worth of it proves
//	  nothing about the two below
//	- 257 defines, which the module must refuse with ORA-20002 and no
//	  rows at all.  pre-fix the same request came back carrying 62 of
//	  the statement's 200 rows and no error anywhere
//	- 257 binds, which it must refuse with ORA-20003.  pre-fix that
//	  request was refused too, but by query2()'s own bind block, with
//	  the ORA-01007 it sends for any request whose bind count came back
//	  0 - an answer that says nothing about a limit
//
// Both refusal cases run on the sdu the session negotiates by default rather
// than on the control case's 512, and that is load bearing rather than
// incidental.  A descriptor block naming 257 positions is around 3600 bytes,
// and a bail abandons the walk without reading the rest of the request - so a
// request split across packets would leave its later fragments on the socket
// for the module to read as the front of the next request.  Each of the two
// asserts that its request really did fit one packet before it sends it.
//
// Every case ends by opening a second cursor and running a statement on it,
// since a refusal that cost the session its place in the byte stream would
// show up there rather than in the answer itself.

// what the control case's session asks for, and so - since it is the floor
// recvConnectRequest() clamps to - what it gets
static const uint16_t	ORA_SPLIT_SDU=512;

// TTI_QUERY2 - an oci7 client's oexec() and oexfet().  the call a define or
// bind descriptor block rides in
static const unsigned char	ORA_TTI_QUERY2=0x47;

// how wide a pointer field is on the wire - see the same constant in
// oraclemidfetchdescribe.cpp.  getPointer() in src/protocols/oracle.cpp sizes
// it off the representation the client offered first for DATATYPE_POINTER,
// and oracleprotocolclient offers the universal one
static const size_t	POINTER_SIZE=1;

// one past each limit, which is what the two refusal cases name
static const uint32_t	ORA_TOO_MANY_DEFINES=257;
static const uint32_t	ORA_TOO_MANY_BINDS=257;

// what the two refusals come back as - ORA_MAX_COLUMN_COUNT_EXCEEDED and
// ORA_MAX_BIND_COUNT_EXCEEDED in src/protocols/oracle.cpp, whose wording is
// the server's own for these two limits
static const uint32_t	ORA_MAX_COLUMN_COUNT_EXCEEDED=20002;
static const uint32_t	ORA_MAX_BIND_COUNT_EXCEEDED=20003;
static const char	*ORA_MAX_COLUMN_COUNT_EXCEEDED_TEXT=
			"ORA-20002: Maximum column count exceeded.";
static const char	*ORA_MAX_BIND_COUNT_EXCEEDED_TEXT=
			"ORA-20003: Maximum bind variable count exceeded.";

// ORA-01007, "variable not in select list" - what query2()'s bind block
// answers a request whose bind count came back 0, and so what the bind case
// used to get instead of a bind count limit
static const char	*ORA_NOT_IN_SELECT_LIST_TEXT="ORA-01007";

// a descriptor's flag byte and the three fields the module keeps out of the
// counts behind it, all read off packet [0027] of samples/
// 9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy - the same
// values oraclemidfetchdescribe.cpp and oraclerefcursorreuse.cpp build
// theirs from.  0x07 is what a 9.2 client marks a position it really defined
// with
static const unsigned char	ORA_DEFINE_FLAG_DEFINED=0x07;
static const unsigned char	ORA_DEFINE_DATATYPE=1;
static const uint32_t		ORA_DEFINE_BUFFER_SIZE=63;
static const uint32_t		ORA_DEFINE_CHARSET=31;

// how many counts ride behind a descriptor's four raw bytes
static const size_t	ORA_DESCRIPTOR_COUNTS=8;

// how many fields sit behind the define count, ahead of the descriptors,
// once the bind count and the pointer in front of it are written out
static const size_t	ORA_DESCRIPTOR_HEADER_TAIL=7;

// how many rows the control case's statement has, and how many its exact
// fetch asks for.  the two are deliberately equal: the fetch behind the batch
// has to come back empty, and it can only say that if the batch really was
// the whole result set
static const uint32_t	ORA_BATCH_ROWS=40;

// and how many the two refusal cases' statement has.  more than one packet of
// answer can hold at the default sdu, so a pre-fix run's short batch is a
// count this test prints rather than something to infer
static const uint32_t	ORA_WIDE_ROWS=200;

// one row: four digits of row number and 56 bytes of padding behind them.  60
// bytes goes out as 64 on the wire - the row marker, a short form clr's
// length byte and the value, and the indicator and return code pair behind it
// - so the control case's 40 rows are ~2600 bytes, six packets' worth at its
// sdu
static const size_t	ORA_ROW_SIZE=60;
static const size_t	ORA_ROW_NUMBER_SIZE=4;
static const size_t	ORA_ROW_PAD_SIZE=56;

// how many rows a case will decode, which is more than any statement here
// has - an answer carrying rows nobody asked for is then counted rather than
// truncated into looking right
static const size_t	ORA_MAX_DECODED_ROWS=256;

// how wide a decoded value may be.  the short form clr's own ceiling, so a
// row that came back some other shape fails the decode rather than
// overrunning anything
static const size_t	ORA_MAX_DECODED_ROW_SIZE=ORA_CLR_MAX_SHORT_LENGTH;

// the alphabet the padding is cut from.  36 bytes long, and 36 divides
// neither the 56 byte pad nor the 502 bytes a fragment carries, so a row read
// at the wrong place shows up as shifted or swapped text rather than as
// plausible bytes
static const char	*ORA_PATTERN=
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// 40 rows of exactly 60 characters: the row number, four digits wide and zero
// padded, then 56 bytes of the alphabet.  the row number is what makes every
// row different from every other one, so a batch that came back short,
// doubled or out of order is a wrong answer rather than the same answer forty
// times.
//
// "connect by level" rather than a table, so the case needs nothing set up
// ahead of it, and an alias rather than the expression's own text, so the
// column name in the describe block is one byte wide whatever oracle would
// otherwise have called it
static const char	*ORA_BATCH_QUERY=
	"select lpad(to_char(level),4,'0')||"
	"rpad('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789',56,"
	"'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789') as v "
	"from dual connect by level<=40";

// the same rows, 200 of them
static const char	*ORA_WIDE_QUERY=
	"select lpad(to_char(level),4,'0')||"
	"rpad('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789',56,"
	"'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789') as v "
	"from dual connect by level<=200";

// the statement every case runs on a second cursor at the end, whose value is
// unlike anything else on the wire here
static const char	*ORA_ALIVE_QUERY="select 'STILLALIVE' from dual";
static const char	*ORA_ALIVE_VALUE="STILLALIVE";

// putRowHeader()'s flags byte in the answer to a fetch
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;

// one row of a decoded batch
struct oraclefetchrow {
	unsigned char	value[ORA_MAX_DECODED_ROW_SIZE];
	size_t		size;
	bool		isnull;
};

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static void reportResponse(oracleprotocolclient *client) {
	stdoutput.printf("response (%d bytes):\n",
				(int)client->getResponseSize());
	stdoutput.safePrint(client->getResponse(),
				(int32_t)client->getResponseSize());
	stdoutput.printf("\n");
}

// what row "rownumber" of a batch holds - the same bytes the statement
// builds, built again here rather than read back off the wire
static void buildExpectedRow(char *row, uint32_t rownumber) {

	charstring::printf(row,ORA_ROW_SIZE+1,"%04d",(int)rownumber);

	size_t	patternsize=charstring::getLength(ORA_PATTERN);
	for (size_t i=0; i<ORA_ROW_PAD_SIZE; i++) {
		row[ORA_ROW_NUMBER_SIZE+i]=ORA_PATTERN[i%patternsize];
	}
	row[ORA_ROW_SIZE]='\0';
}

// where two byte strings first differ, or -1 if they don't
static int64_t firstDifference(const unsigned char *actual,
				size_t actualsize,
				const char *expected,
				size_t expectedsize) {
	size_t	size=(actualsize<expectedsize)?actualsize:expectedsize;
	for (size_t i=0; i<size; i++) {
		if (actual[i]!=(unsigned char)expected[i]) {
			return (int64_t)i;
		}
	}
	if (actualsize!=expectedsize) {
		return (int64_t)size;
	}
	return -1;
}

static void reportDifference(const unsigned char *actual, size_t actualsize,
				const char *expected, size_t expectedsize,
				int64_t difference) {

	stdoutput.printf("  %d bytes back, %d expected\n",
				(int)actualsize,(int)expectedsize);
	stdoutput.printf("  first difference at byte %d\n",(int)difference);

	size_t	from=(difference>16)?(size_t)(difference-16):0;
	size_t	count=48;
	if (from+count>actualsize) {
		count=actualsize-from;
	}
	stdoutput.printf("  back: ");
	stdoutput.safePrint(actual+from,(int32_t)count);
	stdoutput.printf("\n");

	count=48;
	if (from+count>expectedsize) {
		count=expectedsize-from;
	}
	stdoutput.printf("  want: ");
	stdoutput.safePrint(expected+from,(int32_t)count);
	stdoutput.printf("\n");
}

// every row of a batch against the value the statement built for it, in
// order, reporting the first row that doesn't match and where inside it the
// bytes part
static bool checkBatchRows(const oraclefetchrow *rows, size_t rowcount) {

	for (size_t i=0; i<rowcount; i++) {

		char	expected[ORA_ROW_SIZE+1];
		buildExpectedRow(expected,(uint32_t)(i+1));

		int64_t	difference=firstDifference(rows[i].value,rows[i].size,
							expected,ORA_ROW_SIZE);
		if (difference<0 && !rows[i].isnull) {
			continue;
		}

		stdoutput.printf("  row %d is wrong\n",(int)(i+1));
		if (rows[i].isnull) {
			stdoutput.printf("  it came back null\n");
			return false;
		}
		reportDifference(rows[i].value,rows[i].size,
					expected,ORA_ROW_SIZE,difference);
		return false;
	}
	return true;
}


// ---- the calls oracleprotocolclient doesn't have ----

// a pointer field, whose contents the module reads past without looking at
static void appendPointer(oracleprotocolclient *client) {
	for (size_t i=0; i<POINTER_SIZE; i++) {
		client->appendByte(0);
	}
}

// one define or bind descriptor - four raw bytes, the wire datatype, a flag,
// a precision and a scale, then eight counts of which the first is the
// client's buffer size and the sixth its character set.  the two are
// byte-identical in shape: getQuery2Descriptors() in src/protocols/oracle.cpp
// reads both with getQuery2Descriptor()
static void appendDescriptor(oracleprotocolclient *client) {

	client->appendByte(ORA_DEFINE_DATATYPE);
	client->appendByte(ORA_DEFINE_FLAG_DEFINED);
	client->appendByte(0);
	client->appendByte(0);

	for (size_t i=0; i<ORA_DESCRIPTOR_COUNTS; i++) {
		uint32_t	value=0;
		if (!i) {
			value=ORA_DEFINE_BUFFER_SIZE;
		} else if (i==5) {
			value=ORA_DEFINE_CHARSET;
		}
		client->appendAuthCount(value,4);
	}
}

// TTI_QUERY2 with nothing behind its header, which is all query2() in
// src/protocols/oracle.cpp reads for a call that neither defines nor binds.
// with OPTION_FETCH set it is an oci7 client's oexfet() with no count on it,
// which sendFetchResponse() answers with every row the cursor has left
static bool query2(oracleprotocolclient *client, unsigned char sequence,
					uint32_t options, uint32_t cursorid) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

	return client->sendPacket() && client->recvPacket();
}

// TTI_QUERY2 carrying the descriptor block an oci7 client's odefin's and
// obndrv's ride in, built into the request buffer and left there for the
// caller to measure before it goes out.
//
// behind the header come ten fields, the number of positions the define list
// names, a pointer and the bind count, seven more counts, and then one
// descriptor per define and one per bind.  the whole shape is packet [0027]
// of samples/9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy,
// which oraclemidfetchdescribe.cpp reads its own copy off; the bind half is
// the #9700 captures'.
//
// "rowcount" is the second of those seven counts - oexfet()'s own nrows, the
// number of rows an exact fetch asks for.  every other field in the preamble
// is a zero the module reads past.
//
// the bind values go out behind the descriptors as one row data block, a
// single marker byte and then one clr per bind.  they are all nulls: the two
// limits are checked ahead of the values, so what a value carries reaches
// nothing here, and a null is the one byte apiece that keeps a 257 bind
// request inside one packet
static void buildQuery2Descriptors(oracleprotocolclient *client,
					unsigned char sequence,
					uint32_t options,
					uint32_t cursorid,
					uint32_t definecount,
					uint32_t bindcount,
					uint32_t rowcount) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

	// the ten fields ahead of the define count.  the pointer right in
	// front of it is the client's address of its own define array
	appendPointer(client);
	appendPointer(client);
	client->appendAuthCount(0,4);
	client->appendAuthCount(0,4);
	appendPointer(client);
	client->appendAuthCount(0,4);
	appendPointer(client);
	client->appendAuthCount(0,4);
	appendPointer(client);
	appendPointer(client);

	client->appendAuthCount(definecount,4);

	// the client's address of its own bind array, the bind count, and the
	// seven counts the row count rides in
	appendPointer(client);
	client->appendAuthCount(bindcount,4);
	for (size_t i=0; i<ORA_DESCRIPTOR_HEADER_TAIL; i++) {
		client->appendAuthCount((i==1)?rowcount:0,4);
	}

	// the defines, then the binds - one run of identically shaped
	// descriptors, told apart only by the two counts above
	for (uint32_t i=0; i<definecount; i++) {
		appendDescriptor(client);
	}
	for (uint32_t i=0; i<bindcount; i++) {
		appendDescriptor(client);
	}

	if (bindcount) {
		client->appendByte(ORA_TTC_ROW_DATA);
		for (uint32_t i=0; i<bindcount; i++) {
			client->appendLenBytes(NULL,0);
		}
	}
}


// ---- reading the answers ----

// walk a batch out of one logical response - the data flags, a row header,
// one row data message per row and then the trailer, which this stops short
// of.  see
// sendFetchResponse(), putRowHeader() and putRow() in
// src/protocols/oracle.cpp.
//
// a legacy row is not shaped like a modern one: behind each column's value
// come the indicator and the return code odefin() gave the client a pointer
// for, a pair of counts that read as two zero bytes for a value that isn't
// null.  every statement here selects one column, so that is what this
// decodes - a response claiming any other column count fails the decode
// rather than being read at some other shape's offsets.
//
// a response carrying no rows at all leads with the summary object instead of
// a row header, which is not a decode failure but an answer of zero rows -
// what both refusal cases have to get
static bool readFetchRows(oracleprotocolclient *client,
				unsigned char *flags,
				uint32_t *colcount,
				uint32_t *headerrowcount,
				oraclefetchrow *rows,
				size_t maxrows,
				size_t *rowcount) {

	client->rewindResponse();

	*flags=0;
	*colcount=0;
	*headerrowcount=0;
	*rowcount=0;

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode)) {
		return false;
	}

	if (ttccode!=ORA_TTC_ROW_HEADER) {
		return true;
	}

	uint32_t	skip=0;
	if (!client->readByte(flags) ||
		*flags!=ORA_ROW_HEADER_FLAGS_FETCH ||
		!client->readLenPreInt(colcount) ||	// column count
		!client->readLenPreInt(&skip) ||	// iteration number
		!client->readLenPreInt(headerrowcount) ||
		!client->readLenPreInt(&skip) ||	// uac buffer length
		!client->readLenPreInt(&skip) ||	// bit vector size
		!client->readLenPreInt(&skip)) {	// meaning unknown
		return false;
	}

	if (*colcount!=1) {
		return false;
	}

	for (;;) {

		unsigned char	marker=0;
		if (!client->readByte(&marker)) {
			return false;
		}
		if (marker!=ORA_TTC_ROW_DATA) {
			// the trailer, so the rows are done
			return true;
		}

		if (*rowcount>=maxrows) {
			return false;
		}

		oraclefetchrow	*row=&(rows[*rowcount]);
		uint32_t	indicator=0;
		uint32_t	returncode=0;
		if (!client->readLenBytes(row->value,sizeof(row->value),
						&(row->size),&(row->isnull)) ||
			!client->readLenPreInt(&indicator) ||
			!client->readLenPreInt(&returncode)) {
			return false;
		}

		(*rowcount)++;
	}
}

// the oracle error number out of a summary object at the front of a response.
// the fields are putOci7Summary()'s in src/protocols/oracle.cpp: the end of
// call status, the rows processed, and then the number.
//
// there is an end to end sequence number between the first two, but only for
// an o5logonclient, and no session here is one: that flag takes a listener on
// verifiertype="9i" (see recvDataTypeRequest()), where this test's listener
// runs on the 11g verifier serverversion="11.2" defaults to
static bool readSummaryOraNumber(oracleprotocolclient *client,
						uint32_t *oranum) {

	client->rewindResponse();

	*oranum=0;

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) || ttccode!=ORA_TTC_ERROR) {
		return false;
	}

	uint32_t	skip=0;
	return (client->readLenPreInt(&skip) &&		// end of call status
		client->readLenPreInt(&skip) &&		// rows processed
		client->readLenPreInt(oranum));
}

// how many packets a message that size had to be split into - the mirror of
// sendSplitPacket() in src/protocols/oracle.cpp: every fragment repeats the
// eight byte header and the two data flag bytes, so a fragment carries
// sdu-10 bytes of body and the boundaries fall a fixed distance apart.
//
// it is arithmetic on the size and not a count of what came off the socket.
// an observed count would take a fragment counter inside
// oracleprotocolclient::recvPacket(), which keeps none, so a body that
// arrived whole in one oversized packet counts the same as one that arrived
// in pieces
static size_t fragmentCount(size_t packetsize, uint16_t sdu) {

	// too small to carry a body at all, so one packet
	if (packetsize<=ORA_FRAGMENT_OVERHEAD ||
			(size_t)sdu<=ORA_FRAGMENT_OVERHEAD) {
		return 1;
	}

	size_t	body=packetsize-ORA_FRAGMENT_OVERHEAD;
	size_t	maxbodysize=(size_t)sdu-ORA_FRAGMENT_OVERHEAD;
	return (body+maxbodysize-1)/maxbodysize;
}

// the answer measured the way the module measured it before splitting it.
// getResponseSize() counts the data flags and the body recvPacket()
// reassembled, but not the eight byte header, which is what the module's own
// "bigger than the sdu" test is against
static size_t responseFragmentCount(oracleprotocolclient *client) {
	return fragmentCount(client->getResponseSize()+
				ORA_FRAGMENT_OVERHEAD-ORA_DATA_FLAGS_SIZE,
				client->getSdu());
}


// ---- the pieces every case is built from ----

// the login and the cursor a case starts with.  an "sdu" of 0 leaves the
// session on the one it negotiates by default, which is what the two refusal
// cases need: their requests are too big to fit a small one
static bool startSession(oracleprotocolclient *client,
				const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password,
				uint16_t sdu,
				uint32_t *cursorid) {

	char	label[192];

	if (sdu) {
		client->setSdu(sdu);
	}

	charstring::printf(label,sizeof(label),"%s: connect",mode);
	if (!client->connect(host,port,sid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	if (sdu) {
		charstring::printf(label,sizeof(label),
				"%s: the session runs on the small sdu",mode);
		report(label,client->getSdu()==sdu);
	}

	charstring::printf(label,sizeof(label),"%s: login",mode);
	if (!client->login(user,password)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),"%s: open cursor",mode);
	if (!client->open(cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	return true;
}

// parse and execute one statement, leaving its whole result set for a fetch
// of its own.  a prefetch count of 0 and no OPTION_FETCH, so the answer is
// the describe alone
static bool parseAndExecute(oracleprotocolclient *client,
				const char *mode, const char *what,
				uint32_t cursorid, const char *query) {

	char	label[192];

	charstring::printf(label,sizeof(label),
				"%s: parse and execute %s",mode,what);
	if (!client->query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,query)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	bool	executed=(client->getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		reportResponse(client);
	}
	return executed;
}

// the session is still where the byte stream says it is: a fresh cursor, a
// statement on it and the row it selects.  a session left out of step by a
// half-read request or a half-written answer fails here rather than in the
// answer the case was really about
static void checkSessionSurvived(oracleprotocolclient *client,
							const char *mode) {

	char	label[192];

	uint32_t	cursorid=0;
	charstring::printf(label,sizeof(label),
				"%s: the session answers the next request",mode);
	if (!client->open(&cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}

	if (!parseAndExecute(client,mode,"on a second cursor",
						cursorid,ORA_ALIVE_QUERY)) {
		report(label,false);
		return;
	}

	if (!client->fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}
	report(label,client->responseContains(ORA_ALIVE_VALUE));
}

// everything a refused request's answer has to be: the summary object, the
// ora number and message that name the limit, and not one row.  the row count
// is printed rather than only asserted, since what a pre-fix run sends here is
// a count worth reading
static void checkRefusal(oracleprotocolclient *client,
				const char *mode,
				uint32_t expectedoranum,
				const char *expectedtext) {

	char	label[192];

	oraclefetchrow	rows[ORA_MAX_DECODED_ROWS];
	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	size_t		rowsback=0;
	bool		decoded=readFetchRows(client,&flags,&colcount,
						&headerrowcount,rows,
						ORA_MAX_DECODED_ROWS,&rowsback);

	stdoutput.printf("  answer: %d bytes, ttc code 0x%02x, %d rows\n",
				(int)client->getResponseSize(),
				(int)client->getResponseTtcCode(),
				(int)rowsback);

	// the substantive assertion.  pre-fix this answer carried rows: the
	// walk's quiet return left the request's row count at 0, so the fetch
	// behind it sent every row it could fit in one packet and called that
	// the answer
	charstring::printf(label,sizeof(label),"%s: no rows came back",mode);
	report(label,decoded && !rowsback);
	if (!decoded) {
		stdoutput.printf("  the answer didn't decode\n");
		reportResponse(client);
	}

	charstring::printf(label,sizeof(label),
				"%s: the answer is a summary object",mode);
	report(label,client->getResponseTtcCode()==ORA_TTC_ERROR);

	uint32_t	oranum=0;
	bool		readnumber=readSummaryOraNumber(client,&oranum);
	charstring::printf(label,sizeof(label),
				"%s: the answer says ORA-%05d",
				mode,(int)expectedoranum);
	report(label,readnumber && oranum==expectedoranum);
	if (!readnumber) {
		stdoutput.printf("  the summary object didn't decode\n");
		reportResponse(client);
	} else if (oranum!=expectedoranum) {
		stdoutput.printf("  it says ORA-%05d\n",(int)oranum);
		reportResponse(client);
	}

	charstring::printf(label,sizeof(label),
			"%s: the message names the limit",mode);
	report(label,client->responseContains(expectedtext));
}


// ---- the cases ----

// the control: a define list of one position on a one column select, and an
// exact fetch of every row the statement has, on an sdu no single packet of
// that batch fits under.  it is the shape both refusal cases are a refusal
// of, and it is what says a short batch would be seen here
static void runDefinedBatchCase(const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
						ORA_SPLIT_SDU,&cursorid)) {
		return;
	}

	if (!parseAndExecute(&client,mode,"the batch statement",
						cursorid,ORA_BATCH_QUERY)) {
		client.disconnect();
		return;
	}

	// the exact fetch: one define, and the whole batch as the row count
	// that rides in the descriptor block behind it.  OPTION_DEFINE is what
	// gets the block walked at all and OPTION_FETCH is what makes the
	// answer sendFetchResponse()'s rather than a plain execute's summary
	buildQuery2Descriptors(&client,1,ORA_OPTION_DEFINE|ORA_OPTION_FETCH,
						cursorid,1,0,ORA_BATCH_ROWS);

	charstring::printf(label,sizeof(label),
				"%s: exact fetch the batch",mode);
	if (!client.sendPacket() || !client.recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	oraclefetchrow	rows[ORA_MAX_DECODED_ROWS];
	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	size_t		rowcount=0;
	bool		decoded=readFetchRows(&client,&flags,&colcount,
						&headerrowcount,rows,
						ORA_MAX_DECODED_ROWS,&rowcount);

	stdoutput.printf("  answer: %d bytes, %d packets of at most %d, "
				"%d rows\n",
				(int)client.getResponseSize(),
				(int)responseFragmentCount(&client),
				(int)client.getSdu(),
				(int)rowcount);

	charstring::printf(label,sizeof(label),
				"%s: the batch response decodes",mode);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	// the row header's own count of the rows behind it, which is a check
	// on the decode rather than on the batch: it carries the count the
	// client asked for whatever follows it, so it reads 40 either way.  it
	// says the fields in front of it were read at the right offsets, which
	// is what the rows are read at too
	charstring::printf(label,sizeof(label),
			"%s: the row header counts the whole batch",mode);
	report(label,headerrowcount==ORA_BATCH_ROWS);
	if (headerrowcount!=ORA_BATCH_ROWS) {
		stdoutput.printf("  the header counts %d rows, %d asked for\n",
					(int)headerrowcount,(int)ORA_BATCH_ROWS);
	}

	charstring::printf(label,sizeof(label),
			"%s: every row asked for came back in one response",
			mode);
	report(label,rowcount==(size_t)ORA_BATCH_ROWS);
	if (rowcount!=(size_t)ORA_BATCH_ROWS) {
		stdoutput.printf("  %d rows back, %d asked for\n",
					(int)rowcount,(int)ORA_BATCH_ROWS);
	}

	charstring::printf(label,sizeof(label),
			"%s: every row arrived whole and in order",mode);
	report(label,rowcount && checkBatchRows(rows,rowcount));

	// and that the answer could only have got here as more than one
	// packet - without this the case would pass just as well against a
	// batch that fitted one, which is the size a pre-fix refusal sent
	charstring::printf(label,sizeof(label),
			"%s: the answer needed more than one packet at this sdu",
			mode);
	report(label,responseFragmentCount(&client)>1);

	// nothing may be left on the cursor: the batch was the whole result
	// set, so a fetch with no count on it has to come back with the
	// summary object alone.  a legacy client has no way to ask for the
	// rest of a batch, so a row still sitting here is a row lost
	charstring::printf(label,sizeof(label),
			"%s: nothing was held back on the cursor",mode);
	if (!query2(&client,2,ORA_OPTION_FETCH,cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	size_t	morerowcount=0;
	bool	moredecoded=readFetchRows(&client,&flags,&colcount,
						&headerrowcount,rows,
						ORA_MAX_DECODED_ROWS,
						&morerowcount);
	bool	leadssummary=(client.getResponseTtcCode()==ORA_TTC_ERROR);
	report(label,moredecoded && leadssummary && !morerowcount);
	if (!moredecoded) {
		stdoutput.printf("  the answer didn't decode\n");
		reportResponse(&client);
	} else if (morerowcount) {
		stdoutput.printf("  %d more rows came back\n",
							(int)morerowcount);
	} else if (!leadssummary) {
		stdoutput.printf("  the answer leads with ttc code 0x%02x, "
					"not a summary object\n",
					(int)client.getResponseTtcCode());
		reportResponse(&client);
	}

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the regression: a define list one position past maxcolumncount, on the same
// exact fetch shape the control case runs.  pre-fix the walk returned quietly
// and the fetch behind it answered with whatever rows fitted one packet and
// no error at all
static void runTooManyDefinesCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
							0,&cursorid)) {
		return;
	}

	if (!parseAndExecute(&client,mode,"the wide statement",
						cursorid,ORA_WIDE_QUERY)) {
		client.disconnect();
		return;
	}

	buildQuery2Descriptors(&client,1,ORA_OPTION_DEFINE|ORA_OPTION_FETCH,
						cursorid,ORA_TOO_MANY_DEFINES,
						0,ORA_WIDE_ROWS);

	stdoutput.printf("  request: %d bytes, %d defines, sdu %d\n",
				(int)client.getRequestSize(),
				(int)ORA_TOO_MANY_DEFINES,
				(int)client.getSdu());

	charstring::printf(label,sizeof(label),
				"%s: the request fits one packet",mode);
	report(label,client.getRequestSize()<=(size_t)client.getSdu());

	charstring::printf(label,sizeof(label),
				"%s: send the define list",mode);
	if (!client.sendPacket() || !client.recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkRefusal(&client,mode,ORA_MAX_COLUMN_COUNT_EXCEEDED,
				ORA_MAX_COLUMN_COUNT_EXCEEDED_TEXT);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the other limit: a bind list one position past maxbindcount.  pre-fix this
// request was refused as well, but by query2()'s bind block rather than by
// the walk - it answered the ORA-01007 it sends for any request whose bind
// count came back 0, which says nothing about a bind count limit
static void runTooManyBindsCase(const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
							0,&cursorid)) {
		return;
	}

	if (!parseAndExecute(&client,mode,"the wide statement",
						cursorid,ORA_WIDE_QUERY)) {
		client.disconnect();
		return;
	}

	buildQuery2Descriptors(&client,1,ORA_OPTION_BIND|ORA_OPTION_EXECUTE,
						cursorid,0,
						ORA_TOO_MANY_BINDS,0);

	stdoutput.printf("  request: %d bytes, %d binds, sdu %d\n",
				(int)client.getRequestSize(),
				(int)ORA_TOO_MANY_BINDS,
				(int)client.getSdu());

	charstring::printf(label,sizeof(label),
				"%s: the request fits one packet",mode);
	report(label,client.getRequestSize()<=(size_t)client.getSdu());

	charstring::printf(label,sizeof(label),
				"%s: send the bind list",mode);
	if (!client.sendPacket() || !client.recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkRefusal(&client,mode,ORA_MAX_BIND_COUNT_EXCEEDED,
				ORA_MAX_BIND_COUNT_EXCEEDED_TEXT);

	charstring::printf(label,sizeof(label),
			"%s: the answer isn't the old ORA-01007",mode);
	report(label,!client.responseContains(ORA_NOT_IN_SELECT_LIST_TEXT));

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the port a listener ended up on.  the default is what
// test/sqlrelay.conf.d/oracleprotocol.conf.in's @ORACLEPROTOCOLPORT1@ token
// defaults to; test/test.sh exports the real one, the same way
// oraclesplitresponse reads it
static uint16_t portFromEnvironment(const char *name, uint16_t fallback) {
	const char	*value=environment::getValue(name);
	if (charstring::isNullOrEmpty(value)) {
		return fallback;
	}
	return (uint16_t)charstring::convertToInteger(value);
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10059 query2 descriptor limits ======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521 is
	// just the port it was configured with
	const char	*host="127.0.0.1";
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	uint16_t	port=portFromEnvironment("ORACLEPROTOCOLPORT1",1521);

	// the define list the module has to honor
	runDefinedBatchCase("defined batch",host,port,sid,user,password);

	// and the two it has to refuse
	runTooManyDefinesCase("too many defines",host,port,sid,user,password);
	runTooManyBindsCase("too many binds",host,port,sid,user,password);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
