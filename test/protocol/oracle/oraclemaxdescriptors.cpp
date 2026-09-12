// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/signalclasses.h>
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
// Six cases, each on its own session:
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
//	- the same 257 defines again, refused the same way, but on the
//	  control case's 512 byte sdu rather than the default.  #10069: the
//	  walk used to bail on the limit without reading the rest of the
//	  request, so a request split across packets left its later
//	  fragments on the socket for the module to read as the front of the
//	  next one - which it answered "bad ttccode 0xff" and dropped the
//	  session over, so the client's next write hit a closed socket and
//	  got SIGPIPE
//	- the same 257 binds again, refused the same way, at the same 512
//	  byte sdu and for the same reason
//	- a definitions count nothing on the wire could really carry, rather
//	  than one merely over a configured limit.  #10069: this is the bail
//	  the walk can never read its way past, so it can't be answered with
//	  a refusal the session survives the way the four above are - the
//	  module answers ORA-03137 and ends the session instead, and the case
//	  asserts both: the error came back, and a further request on the
//	  same connection has nowhere to land
//
// The two default-sdu refusal cases still earn their place: each asserts its
// request really did fit one packet before sending it, which is what
// isolates the refusal from #10069's fragment handling, and is why they send
// with sendPacket() rather than sendSplitPacket().  It's the two 512-sdu
// cases below them that prove the fix - each asserts the opposite, that its
// request needed more than one packet, then sends it with sendSplitPacket()
// the way sendBindQuery3() does further down for the query3 cases.
//
// Every refusal case ends by opening a second cursor and running a statement
// on it, since a refusal that cost the session its place in the byte stream
// would show up there rather than in the answer itself.  the malformed count
// case ends the opposite way, asserting that a further request fails, since
// the session it lands on is already gone.
//
// #10067 is the same silent truncation on the modern path.  installQuery3Binds()
// and saveQuery3Binds() write into arrays sized to maxbindcount and used to
// stop at the end of them, so a TTI_QUERY3 naming more binds than that ran its
// statement on the first maxbindcount values and answered with an ordinary
// describe.  getQuery3Binds() now flags the over-wide list and query3() refuses
// the whole request with the same ORA-20003, ahead of the save, the parse and
// the execute.
//
// Three more cases, each on its own session:
//
//	- 257 binds on a query3 parse and execute, which must come back
//	  ORA-20003 and no describe at all.  pre-fix the statement ran
//	- 256 binds, one descriptor short of the limit, which must still parse,
//	  execute and answer with the values it bound.  a harness that can't
//	  tell that from a refusal says nothing about the case above
//	- a refusal landing on a cursor that already carries a statement.  it
//	  happens ahead of saveQuery3Binds(), and getQuery3Binds() touches no
//	  cursor state at all, so the statement and the binds saved with it have
//	  to come through untouched - a bare re-execute of them still runs, with
//	  fresh values of its own
//
// Their requests are nothing like the query2 ones': 257 bind descriptors
// behind a statement with 257 placeholders is around 3000 bytes past the sdu
// the session negotiates, so each one goes out through sendSplitPacket().  That
// is safe here and isn't above because the bind walk consumes the whole request
// whatever it finds, so a refusal leaves no fragment of it on the socket.
//
// #10084 is a different bail in getQuery2Descriptors(), on the same walk:
// where the wire's bind count and OPTION_BIND disagree, the count is dropped
// to 0 so nothing is installed, but the walk used to return without ever
// reading the (really nonzero) bind descriptors and their values off the
// wire.  those bytes are on the socket regardless of the disagreement, so
// pre-fix they were left for the next request's parse to read as the front
// of one of its own - #10069's desync, from a different bail in the same
// function.  the fix walks and discards the bind block here too.
//
// Two more cases, each on its own session:
//
//	- a define list well within the limit, and a wire bind count naming
//	  real descriptors with OPTION_BIND unset, split across packets at
//	  ORA_SPLIT_SDU so the mismatched bind block lands partly behind the
//	  first one.  the mismatch costs the call nothing of its own - the
//	  exact fetch behind it answers the same as the control case's - so
//	  the case's assertion is the one #10084 is really about: the next
//	  request still parses, rather than landing on a session already lost
//	  to the same "bad ttccode" desync #10069 fixed for the two limits
//	  above
//	- the same disagreement with a bind count past maxbindcount rather
//	  than inside it.  the count is dropped either way, so the request
//	  is answered rather than refused, but the block the walk steps over
//	  is now wider than the arrays it would write into.  those writes are
//	  all behind the walk's discard guard, and this is the case that runs
//	  off the end of them if the guard is lost - the case above stays
//	  inside them with or without it

// what the control case's session asks for, and now the two split-request
// refusal cases too - since it is the floor recvConnectRequest() clamps to,
// it's what each of them gets
static const uint16_t	ORA_SPLIT_SDU=512;

// TTI_QUERY2 - an oci7 client's oexec() and oexfet().  the call a define or
// bind descriptor block rides in
static const unsigned char	ORA_TTI_QUERY2=0x47;

// how wide a pointer field is on the wire - see the same constant in
// oraclemidfetchdescribe.cpp.  getPointer() in src/protocols/oracle.cpp sizes
// it off the representation the client offered first for DATATYPE_POINTER,
// and oracleprotocolclient offers the universal one
static const size_t	POINTER_SIZE=1;

// one past each limit, which is what the refusal cases name
static const uint32_t	ORA_TOO_MANY_DEFINES=257;
static const uint32_t	ORA_TOO_MANY_BINDS=257;

// and the limit itself, which is the widest bind list the query3 path still
// has to honor
static const uint32_t	ORA_IN_LIMIT_BINDS=256;

// a bind count named on the wire while OPTION_BIND is left unset - the
// #10084 disagreement getQuery2Descriptors() drops to 0.  wide enough that
// its descriptors and values don't fit behind one define in the first packet
// at ORA_SPLIT_SDU, so some of them land in the next one
static const uint32_t	ORA_MISMATCHED_BINDS=60;

// and the same disagreement one past the bind limit, which the count above
// is well inside of.  the walk steps over a block this wide with the wire's
// count while query2bindtypes[] and the arrays beside it hold maxbindcount
// entries, so it is this case rather than the one above that runs off the
// end of them if the walk's discard guard is ever lost
static const uint32_t	ORA_MISMATCHED_BINDS_OVER_LIMIT=ORA_TOO_MANY_BINDS;

// what the two refusals come back as - ORA_MAX_COLUMN_COUNT_EXCEEDED and
// ORA_MAX_BIND_COUNT_EXCEEDED in src/protocols/oracle.cpp, whose wording is
// the server's own for these two limits
static const uint32_t	ORA_MAX_COLUMN_COUNT_EXCEEDED=20002;
static const uint32_t	ORA_MAX_BIND_COUNT_EXCEEDED=20003;
static const char	*ORA_MAX_COLUMN_COUNT_EXCEEDED_TEXT=
			"ORA-20002: Maximum column count exceeded.";
static const char	*ORA_MAX_BIND_COUNT_EXCEEDED_TEXT=
			"ORA-20003: Maximum bind variable count exceeded.";

// a define count nothing on the wire could really carry - past descriptorspace
// however wide the request behind it is, so the walk bails on the count
// itself rather than on one merely over a configured limit
static const uint32_t	ORA_HUGE_DEFINE_COUNT=0xfffffff0;

// what a count that far out comes back as - ORA_MALFORMED_TTC_PACKET in
// src/protocols/oracle.cpp, #10069's answer to a request the walk can't read
// its way past
static const uint32_t	ORA_MALFORMED_TTC_PACKET=3137;
static const char	*ORA_MALFORMED_TTC_PACKET_TEXT=
			"ORA-03137: malformed TTC packet from client rejected";

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

// how wide every query3 bind is declared.  installQuery3Binds() in
// src/protocols/oracle.cpp sizes the bind buffer from this, so it has to be
// past the longest value any of them carries
static const uint32_t	ORA_QUERY3_BIND_BUFFER_SIZE=32;

// room for the longest statement a query3 case builds - 257 placeholders and
// the concatenation between them, which is around 2300 bytes
static const size_t	ORA_MAX_QUERY3_QUERY_SIZE=4096;

// what a wide bind list carries: a value in the first descriptor, a value in
// the last, and a null in every one between.  oracle concatenates a null as
// nothing, so the answer is those two whatever the list's width - and the half
// of it the last descriptor put there is the half a clamped list drops
static const char	*ORA_FIRST_BIND_VALUE="FIRSTBIND";
static const char	*ORA_LAST_BIND_VALUE="LASTBIND";
static const char	*ORA_IN_LIMIT_VALUE="FIRSTBINDLASTBIND";

// the statement the last case's cursor is already carrying when the refused
// request lands on it, and the values of its two executions - the first
// through query3(), the second through a bare re-execute of the binds saved
// with it
static const uint32_t	ORA_PAIR_BINDS=2;
static const char	*ORA_PAIR_QUERY="select :b1 || :b2 from dual";
static const char	*ORA_PRIOR_BIND_VALUE1="PRIORONE";
static const char	*ORA_PRIOR_BIND_VALUE2="PRIORTWO";
static const char	*ORA_PRIOR_VALUE="PRIORONEPRIORTWO";
static const char	*ORA_FRESH_BIND_VALUE1="FRESHONE";
static const char	*ORA_FRESH_BIND_VALUE2="FRESHTWO";
static const char	*ORA_FRESH_VALUE="FRESHONEFRESHTWO";

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

// the same header buildQuery2Descriptors() writes, up through the seven field
// tail behind the bind count, but with a definitions value the module can't
// possibly walk and no descriptors behind it - getQuery2Descriptors() bails
// on the count before it ever tries to read one, so there is nothing here for
// a descriptor loop to build
static void buildQuery2MalformedDescriptors(oracleprotocolclient *client,
					unsigned char sequence,
					uint32_t options,
					uint32_t cursorid,
					uint32_t hugedefinitions) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

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

	client->appendAuthCount(hugedefinitions,4);

	// no binds, and the seven counts behind them, all zero
	appendPointer(client);
	client->appendAuthCount(0,4);
	for (size_t i=0; i<ORA_DESCRIPTOR_HEADER_TAIL; i++) {
		client->appendAuthCount(0,4);
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

// the same number out of the summary object a query3 request is answered with,
// which is a different object: putSummary() in src/protocols/oracle.cpp writes
// the end to end sequence number putOci7Summary() only writes for an
// o5logonclient, so the number sits one field further in
static bool readQuery3SummaryOraNumber(oracleprotocolclient *client,
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
		client->readLenPreInt(&skip) &&		// sequence number
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
// session on the one it negotiates by default, which is what the two
// default-sdu refusal cases need to fit their request in one packet; the
// split-request variants pass ORA_SPLIT_SDU instead, deliberately too small
// for theirs to fit
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

// the opposite of checkSessionSurvived(): a bail the module couldn't walk
// past closes the connection rather than leaving the session to go on, so the
// next request on it has nowhere to land.  a fresh cursor is the lightest
// thing to ask for it - the send finds the socket already gone, or the recv
// comes back empty, and either one is success here
static void checkSessionEnded(oracleprotocolclient *client, const char *mode) {

	char	label[192];

	uint32_t	cursorid=0;
	charstring::printf(label,sizeof(label),
				"%s: the session is over, not just the call",mode);
	report(label,!client->open(&cursorid));
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


// ---- the pieces the query3 cases are built from ----

// "select :b1 || :b2 || ... || :bN from dual", the statement a case with N
// binds parses.  the spaces around the concatenation are load bearing:
// afterBindVariable() in src/common/bindvariables.h ends a placeholder's name
// on whitespace and a short set of punctuation that doesn't include the pipe,
// so ":b1||:b2" reads as one placeholder named "b1||:b2" and the bind never
// matches
static bool buildBindQuery(char *query, size_t querysize, uint32_t bindcount) {

	// "select ", at most ":b256 || " per bind, and " from dual"
	if (querysize<(size_t)(7+bindcount*9+11)) {
		return false;
	}

	query[0]='\0';
	charstring::append(query,"select ");
	for (uint32_t i=0; i<bindcount; i++) {
		if (i) {
			charstring::append(query," || ");
		}
		charstring::append(query,":b");
		charstring::append(query,(uint64_t)(i+1));
	}
	charstring::append(query," from dual");
	return true;
}

// the descriptors and values that go with it - a varchar apiece, and a value
// in the first and last of them with nulls in between
static void buildBindValues(oracleprotocolbind *binds,
				oracleprotocolbindvalue *values,
				uint32_t bindcount) {

	for (uint32_t i=0; i<bindcount; i++) {
		binds[i].varchar(ORA_QUERY3_BIND_BUFFER_SIZE);
		values[i].setNull();
	}
	values[0].set(ORA_FIRST_BIND_VALUE);
	values[bindcount-1].set(ORA_LAST_BIND_VALUE);
}

// one query3 parse and execute carrying a whole bind list, written out as
// several packets - a list this wide is thousands of bytes past any sdu the
// session can negotiate, so sendPacket() is not an option and sendSplitPacket()
// is what a real client's tns layer would do with it anyway
static bool sendBindQuery3(oracleprotocolclient *client,
				const char *mode, const char *what,
				uint32_t cursorid,
				const char *query,
				const oracleprotocolbind *binds,
				const oracleprotocolbindvalue *values,
				uint32_t bindcount) {

	char	label[192];

	charstring::printf(label,sizeof(label),"%s: send %s",mode,what);
	if (!client->buildQuery3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,query,binds,bindcount,1,values,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	stdoutput.printf("  request: %d bytes, %d binds, "
				"%d packets of at most %d\n",
				(int)client->getRequestSize(),(int)bindcount,
				(int)fragmentCount(client->getRequestSize(),
							client->getSdu()),
				(int)client->getSdu());

	if (!client->sendSplitPacket() || !client->recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);
	return true;
}

// everything a refused query3 request's answer has to be: the summary object
// rather than the describe a parse and execute is answered with, and the ora
// number and message that name the limit.  checkRefusal() above can't do duty
// here - the two paths answer with different summary objects, and this one
// carries no row header to walk either way
static void checkQuery3Refusal(oracleprotocolclient *client,
					const char *mode,
					uint32_t expectedoranum,
					const char *expectedtext) {

	char	label[192];

	stdoutput.printf("  answer: %d bytes, ttc code 0x%02x\n",
				(int)client->getResponseSize(),
				(int)client->getResponseTtcCode());

	// the substantive assertion.  pre-fix the answer was a describe - the
	// bind list was clamped to maxbindcount and the statement ran on what
	// was left of it
	charstring::printf(label,sizeof(label),
			"%s: the answer is a summary object, not a describe",
			mode);
	report(label,client->getResponseTtcCode()==ORA_TTC_ERROR);

	uint32_t	oranum=0;
	bool		readnumber=readQuery3SummaryOraNumber(client,&oranum);
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

// #10069: the same refusal as above, but with the request split across
// packets rather than fitting one.  pre-fix the walk bailed on the limit
// without reading the rest of the request, so the later fragments sat
// unread on the socket, the module read them as the front of the next
// request and answered "bad ttccode 0xff", and the session was gone by the
// time the client wrote to it again
static void runTooManyDefinesSplitCase(const char *mode,
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

	if (!parseAndExecute(&client,mode,"the wide statement",
						cursorid,ORA_WIDE_QUERY)) {
		client.disconnect();
		return;
	}

	buildQuery2Descriptors(&client,1,ORA_OPTION_DEFINE|ORA_OPTION_FETCH,
						cursorid,ORA_TOO_MANY_DEFINES,
						0,ORA_WIDE_ROWS);

	stdoutput.printf("  request: %d bytes, %d defines, "
				"%d packets of at most %d\n",
				(int)client.getRequestSize(),
				(int)ORA_TOO_MANY_DEFINES,
				(int)fragmentCount(client.getRequestSize(),
							client.getSdu()),
				(int)client.getSdu());

	charstring::printf(label,sizeof(label),
			"%s: the request needs more than one packet",mode);
	report(label,fragmentCount(client.getRequestSize(),
						client.getSdu())>1);

	charstring::printf(label,sizeof(label),
				"%s: send the define list",mode);
	if (!client.sendSplitPacket() || !client.recvPacket()) {
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

// the bind limit's own split-request case, for the same reason as above
static void runTooManyBindsSplitCase(const char *mode,
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

	if (!parseAndExecute(&client,mode,"the wide statement",
						cursorid,ORA_WIDE_QUERY)) {
		client.disconnect();
		return;
	}

	buildQuery2Descriptors(&client,1,ORA_OPTION_BIND|ORA_OPTION_EXECUTE,
						cursorid,0,
						ORA_TOO_MANY_BINDS,0);

	stdoutput.printf("  request: %d bytes, %d binds, "
				"%d packets of at most %d\n",
				(int)client.getRequestSize(),
				(int)ORA_TOO_MANY_BINDS,
				(int)fragmentCount(client.getRequestSize(),
							client.getSdu()),
				(int)client.getSdu());

	charstring::printf(label,sizeof(label),
			"%s: the request needs more than one packet",mode);
	report(label,fragmentCount(client.getRequestSize(),
						client.getSdu())>1);

	charstring::printf(label,sizeof(label),
				"%s: send the bind list",mode);
	if (!client.sendSplitPacket() || !client.recvPacket()) {
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

// #10084: the disagreement check just above the two limits, in the same
// walk - a wire bind count naming real descriptors while OPTION_BIND is
// unset, split across packets the way the two cases above are.
// getQuery2Descriptors() drops the count to 0 either way, so the mismatch
// costs the call nothing of its own: the exact fetch behind it answers the
// same as the control case's.  pre-fix, though, the walk that drops the
// count also skipped reading the (really nonzero) bind descriptors and
// values off the wire, so the block's own bytes - the ones this sdu pushes
// behind the first packet - were left on the socket for the next request's
// parse to read as the front of one of its own, the same desync #10069
// fixed for the two refusals above
static void runBindCountMismatchSplitCase(const char *mode,
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

	// one define - the column the batch statement selects - and a wire
	// bind count naming real descriptors, with OPTION_BIND left unset.
	// the exact fetch is the control case's own shape, so a correct
	// answer here is the same batch it gets
	buildQuery2Descriptors(&client,1,ORA_OPTION_DEFINE|ORA_OPTION_FETCH,
						cursorid,1,ORA_MISMATCHED_BINDS,
						ORA_BATCH_ROWS);

	stdoutput.printf("  request: %d bytes, 1 define, %d mismatched "
				"binds, %d packets of at most %d\n",
				(int)client.getRequestSize(),
				(int)ORA_MISMATCHED_BINDS,
				(int)fragmentCount(client.getRequestSize(),
							client.getSdu()),
				(int)client.getSdu());

	charstring::printf(label,sizeof(label),
			"%s: the request needs more than one packet",mode);
	report(label,fragmentCount(client.getRequestSize(),
						client.getSdu())>1);

	charstring::printf(label,sizeof(label),
			"%s: send the mismatched descriptor block",mode);
	if (!client.sendSplitPacket() || !client.recvPacket()) {
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

	stdoutput.printf("  answer: %d bytes, ttc code 0x%02x, %d rows\n",
				(int)client.getResponseSize(),
				(int)client.getResponseTtcCode(),
				(int)rowcount);

	charstring::printf(label,sizeof(label),
				"%s: the batch response decodes",mode);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	// the substantive assertion about this request's own answer: the
	// mismatch is not a refusal, so it has to come back as the batch, not
	// as the summary object the two limit cases above get
	charstring::printf(label,sizeof(label),
			"%s: the answer isn't an error",mode);
	report(label,client.getResponseTtcCode()!=ORA_TTC_ERROR);

	charstring::printf(label,sizeof(label),
			"%s: every row of the batch came back whole",mode);
	report(label,rowcount==(size_t)ORA_BATCH_ROWS &&
				checkBatchRows(rows,rowcount));

	// the substantive assertion about the one after it: the mismatched
	// bind block's own packet was consumed rather than left on the
	// socket.  pre-fix it wasn't, and the next request's parse read it as
	// the front of one of its own
	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// #10084 again, with the same disagreement carrying a bind count past the
// instance's maxbindcount rather than one inside it.  the count is still
// dropped to 0, so this is no more a refusal than the case above is - the
// batch still comes back whole - but the walk steps over a block wider than
// query2bindtypes[] and the arrays beside it, which are sized to
// maxbindcount.  every write into them is behind the walk's discard guard,
// and this is the case that runs off the end of them if that guard is ever
// lost: the 60 bind case above stays inside them whether the guard is there
// or not
static void runBindCountMismatchOverLimitSplitCase(const char *mode,
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

	// the same request as the case above, with a wire bind count past the
	// limit instead of inside it
	buildQuery2Descriptors(&client,1,ORA_OPTION_DEFINE|ORA_OPTION_FETCH,
					cursorid,1,
					ORA_MISMATCHED_BINDS_OVER_LIMIT,
					ORA_BATCH_ROWS);

	stdoutput.printf("  request: %d bytes, 1 define, %d mismatched "
				"binds, %d packets of at most %d\n",
				(int)client.getRequestSize(),
				(int)ORA_MISMATCHED_BINDS_OVER_LIMIT,
				(int)fragmentCount(client.getRequestSize(),
							client.getSdu()),
				(int)client.getSdu());

	charstring::printf(label,sizeof(label),
			"%s: the request needs more than one packet",mode);
	report(label,fragmentCount(client.getRequestSize(),
						client.getSdu())>1);

	charstring::printf(label,sizeof(label),
			"%s: send the mismatched descriptor block",mode);
	if (!client.sendSplitPacket() || !client.recvPacket()) {
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

	stdoutput.printf("  answer: %d bytes, ttc code 0x%02x, %d rows\n",
				(int)client.getResponseSize(),
				(int)client.getResponseTtcCode(),
				(int)rowcount);

	charstring::printf(label,sizeof(label),
				"%s: the batch response decodes",mode);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	// a count past the limit is still not a refusal here - the
	// disagreement drops it before the limit is ever checked against it,
	// so this comes back as the batch rather than as the summary object
	// the bind limit case gets
	charstring::printf(label,sizeof(label),
			"%s: the answer isn't an error",mode);
	report(label,client.getResponseTtcCode()!=ORA_TTC_ERROR);

	charstring::printf(label,sizeof(label),
			"%s: the answer isn't the bind count refusal",mode);
	report(label,!client.responseContains(
				ORA_MAX_BIND_COUNT_EXCEEDED_TEXT));

	charstring::printf(label,sizeof(label),
			"%s: every row of the batch came back whole",mode);
	report(label,rowcount==(size_t)ORA_BATCH_ROWS &&
				checkBatchRows(rows,rowcount));

	// and the same assertion about the request after it: the block's
	// packets came off the socket rather than being left for that
	// request's parse to read as the front of one of its own
	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// #10069: a define count the walk can't get past at all, rather than one
// merely over a configured limit.  pre-fix this bailed quietly too, leaving
// whatever was left of the request unread on the socket for the next request
// to be misread against; there being no describing this bail as a limit, the
// module now answers ORA-03137 and ends the session instead of going on with
// the socket out of sync
static void runMalformedDescriptorCountCase(const char *mode,
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

	buildQuery2MalformedDescriptors(&client,1,
					ORA_OPTION_DEFINE|ORA_OPTION_FETCH,
					cursorid,ORA_HUGE_DEFINE_COUNT);

	stdoutput.printf("  request: %d bytes, %d claimed defines\n",
				(int)client.getRequestSize(),
				(int)ORA_HUGE_DEFINE_COUNT);

	charstring::printf(label,sizeof(label),
				"%s: send the malformed descriptor block",mode);
	if (!client.sendPacket() || !client.recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkRefusal(&client,mode,ORA_MALFORMED_TTC_PACKET,
				ORA_MALFORMED_TTC_PACKET_TEXT);

	checkSessionEnded(&client,mode);

	client.disconnect();
}

// the same limit on the modern path: a query3 parse and execute naming one
// bind past maxbindcount.  pre-fix installQuery3Binds() stopped at the limit
// and the statement ran on the first 256 of the 257 values, so this request
// came back a describe and the client had no way to know
static void runQuery3TooManyBindsCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	char	query[ORA_MAX_QUERY3_QUERY_SIZE];
	if (!buildBindQuery(query,sizeof(query),ORA_TOO_MANY_BINDS)) {
		charstring::printf(label,sizeof(label),
					"%s: build the statement",mode);
		report(label,false);
		return;
	}

	oracleprotocolbind	binds[ORA_TOO_MANY_BINDS];
	oracleprotocolbindvalue	values[ORA_TOO_MANY_BINDS];
	buildBindValues(binds,values,ORA_TOO_MANY_BINDS);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
							0,&cursorid)) {
		return;
	}

	if (!sendBindQuery3(&client,mode,"the wide bind list",cursorid,
					query,binds,values,ORA_TOO_MANY_BINDS)) {
		client.disconnect();
		return;
	}

	checkQuery3Refusal(&client,mode,ORA_MAX_BIND_COUNT_EXCEEDED,
					ORA_MAX_BIND_COUNT_EXCEEDED_TEXT);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the control for it: the same shape one descriptor short of the limit, which
// has to parse, execute and answer with the values it bound.  a harness that
// can't tell that from a refusal says nothing about the case above, and the
// limit itself is where an off by one in either direction shows up
static void runQuery3InLimitBindsCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	char	query[ORA_MAX_QUERY3_QUERY_SIZE];
	if (!buildBindQuery(query,sizeof(query),ORA_IN_LIMIT_BINDS)) {
		charstring::printf(label,sizeof(label),
					"%s: build the statement",mode);
		report(label,false);
		return;
	}

	oracleprotocolbind	binds[ORA_IN_LIMIT_BINDS];
	oracleprotocolbindvalue	values[ORA_IN_LIMIT_BINDS];
	buildBindValues(binds,values,ORA_IN_LIMIT_BINDS);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
							0,&cursorid)) {
		return;
	}

	if (!sendBindQuery3(&client,mode,"the bind list",cursorid,
					query,binds,values,ORA_IN_LIMIT_BINDS)) {
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: the statement was executed",mode);
	bool	executed=(client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
			"%s: the answer isn't the bind count limit",mode);
	report(label,!client.responseContains(
				ORA_MAX_BIND_COUNT_EXCEEDED_TEXT));

	charstring::printf(label,sizeof(label),"%s: fetch",mode);
	if (!client.fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	// the first and last bind's values, concatenated by the statement.  the
	// last is the one a list clamped at the limit would have dropped, so a
	// row carrying both says every descriptor was installed
	charstring::printf(label,sizeof(label),
			"%s: the whole bind list was installed",mode);
	bool	installed=client.responseContains(ORA_IN_LIMIT_VALUE);
	report(label,installed);
	if (!installed) {
		reportResponse(&client);
	}

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// what a refusal costs the cursor it lands on, which is nothing.  query3()
// refuses ahead of saveQuery3Binds(), and getQuery3Binds() touches no cursor
// state at all, so a cursor that already carries a statement keeps it and
// keeps the binds saved with it - a bare re-execute still re-runs it, with
// fresh values of its own.  pre-fix there was no refusal to survive: the wide
// request parsed its own statement over the top of this one
static void runQuery3RefusalKeepsStatementCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	char	widequery[ORA_MAX_QUERY3_QUERY_SIZE];
	if (!buildBindQuery(widequery,sizeof(widequery),ORA_TOO_MANY_BINDS)) {
		charstring::printf(label,sizeof(label),
					"%s: build the statement",mode);
		report(label,false);
		return;
	}

	oracleprotocolbind	widebinds[ORA_TOO_MANY_BINDS];
	oracleprotocolbindvalue	widevalues[ORA_TOO_MANY_BINDS];
	buildBindValues(widebinds,widevalues,ORA_TOO_MANY_BINDS);

	oracleprotocolbind	pairbinds[ORA_PAIR_BINDS];
	oracleprotocolbindvalue	pairvalues[ORA_PAIR_BINDS];
	pairbinds[0].varchar(ORA_QUERY3_BIND_BUFFER_SIZE);
	pairbinds[1].varchar(ORA_QUERY3_BIND_BUFFER_SIZE);
	pairvalues[0].set(ORA_PRIOR_BIND_VALUE1);
	pairvalues[1].set(ORA_PRIOR_BIND_VALUE2);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
							0,&cursorid)) {
		return;
	}

	// the statement the cursor is carrying when the refusal arrives, and
	// the binds saved with it
	charstring::printf(label,sizeof(label),
				"%s: parse and execute the pair statement",mode);
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,ORA_PAIR_QUERY,
				pairbinds,ORA_PAIR_BINDS,1,
				pairvalues,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	bool	executed=(client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: the pair statement's own values",mode);
	if (!client.fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,client.responseContains(ORA_PRIOR_VALUE));

	// the refusal, on that same cursor
	if (!sendBindQuery3(&client,mode,"the wide bind list",cursorid,
				widequery,widebinds,widevalues,
				ORA_TOO_MANY_BINDS)) {
		client.disconnect();
		return;
	}

	checkQuery3Refusal(&client,mode,ORA_MAX_BIND_COUNT_EXCEEDED,
					ORA_MAX_BIND_COUNT_EXCEEDED_TEXT);

	// and the re-execute, which carries values and nothing else - the
	// statement and the descriptors behind them are whatever the cursor
	// still has
	pairvalues[0].set(ORA_FRESH_BIND_VALUE1);
	pairvalues[1].set(ORA_FRESH_BIND_VALUE2);

	charstring::printf(label,sizeof(label),
				"%s: re-execute the pair statement",mode);
	if (!client.reexecute(cursorid,1,ORA_OPTION_EXECUTE,0,
					ORA_PAIR_BINDS,pairvalues,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	uint32_t	oranum=0;
	bool		readnumber=readQuery3SummaryOraNumber(&client,&oranum);
	report(label,readnumber && !oranum);
	if (!readnumber) {
		stdoutput.printf("  the summary object didn't decode\n");
		reportResponse(&client);
	} else if (oranum) {
		stdoutput.printf("  it says ORA-%05d\n",(int)oranum);
		reportResponse(&client);
	}

	// the substantive assertion: the statement the refusal was supposed to
	// leave alone, run again on values of its own
	charstring::printf(label,sizeof(label),
			"%s: the re-executed statement answers with its fresh "
			"values",mode);
	if (!client.fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	bool	refreshed=client.responseContains(ORA_FRESH_VALUE);
	report(label,refreshed);
	if (!refreshed) {
		reportResponse(&client);
	}

	charstring::printf(label,sizeof(label),
			"%s: and not the previous execution's",mode);
	report(label,!client.responseContains(ORA_PRIOR_VALUE));

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

	// the malformed descriptor count case (#10069) sends the module a
	// request it ends the session over, so the write behind that
	// session's next call - checkSessionEnded()'s open() - lands on a
	// socket the peer already closed.  a real client hits the same
	// SIGPIPE the ticket describes; this one ignores it instead, the way
	// sqlrsh.cpp does, so that write comes back as an ordinary failed
	// call rather than taking the whole harness down with it
	#ifdef SIGPIPE
	signalset	set;
	set.removeAllSignals();
	set.addSignal(SIGPIPE);
	signalmanager::ignoreSignals(&set);
	#endif

	stdoutput.printf("\n====== #10059/#10067/#10069/#10084 descriptor and "
							"bind limits ======\n\n");

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

	// the same two refusals, forced across more than one packet - #10069
	runTooManyDefinesSplitCase("too many defines, split request",
					host,port,sid,user,password);
	runTooManyBindsSplitCase("too many binds, split request",
					host,port,sid,user,password);

	// #10084: a wire bind count and OPTION_BIND that disagree, split the
	// same way, whose own bind block has to come off the wire even though
	// nothing from it is kept
	runBindCountMismatchSplitCase("bind count and OPTION_BIND disagree, "
					"split request",
					host,port,sid,user,password);

	// and the same disagreement carrying more binds than the arrays the
	// walk would write into hold, which is the shape a lost discard guard
	// shows up in
	runBindCountMismatchOverLimitSplitCase(
				"bind count and OPTION_BIND disagree past "
				"the bind limit, split request",
				host,port,sid,user,password);

	// #10069's other new path: a count the walk can't read past at all,
	// which ends the session with ORA-03137 rather than refusing the
	// request and going on the way the four cases above do
	runMalformedDescriptorCountCase("malformed descriptor count",
					host,port,sid,user,password);

	// the same bind limit on the query3 path, which has to be refused
	// rather than clamped, and the bind list at the limit that still runs
	runQuery3TooManyBindsCase("too many query3 binds",
					host,port,sid,user,password);
	runQuery3InLimitBindsCase("query3 binds at the limit",
					host,port,sid,user,password);

	// and what the refusal leaves the cursor carrying, which is everything
	// it was carrying before
	runQuery3RefusalKeepsStatementCase("a refusal keeps the statement",
					host,port,sid,user,password);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
