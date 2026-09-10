// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for tickets #10000 and #10028: the per-cursor state a
// ref cursor child carries back into the pool, and the release that has to
// walk down through a child's own children, and sideways across a statement's
// several children, to hand all of it back.
//
// A ref cursor bind takes a cursor out of the pool for the statement to open
// its result set on, and releaseRefCursors() in src/protocols/oracle.cpp hands
// it back when the statement re-executes or closes.  The reset applied on the
// way back, and again when installQuery3Binds() takes one out, used to leave
// two per-cursor arrays alone: definecounts[] and query2cursorbindcounts[].
// So the next statement to get that cursor id as a ref cursor child got it
// with the previous one's oci7 define list still on it, and a fetch from the
// fresh child dropped every column the stale list didn't name.  Both resets
// now go through resetCursorState(), which clears all of it.
//
// The define list is what this drives, because it is the half a client can
// see.  The stale query2 bind count is only ever read on the legacy
// TTI_EXECUTE path, and execute() there hands the call straight to
// reexecute() whenever query3session is set - which any session that has
// opened a ref cursor necessarily has, since a ref cursor bind only exists on
// the TTI_QUERY3 path.  Nothing a client can send reaches that read after a
// ref cursor, so the bind-count half is not observable over the wire.
//
// Which fetch shape the assertions use matters, and it is not the obvious
// one.  A TTI_FETCH in a query3 session goes to fetch3(), whose
// sendFetch3Response() sends every column and never asks columnIsDefined().
// The define list is only consulted by sendFetchResponse(), and the one way
// to reach that after a query3 is a TTI_QUERY2 with OPTION_FETCH - an oci7
// client's oexfet() - which query2() answers directly whatever path the
// session is on.  So the plain TTI_FETCH below is the baseline that says
// which cursor id the child landed on, and the query2 exact fetches are the
// ones that see the defines.
//
// The second scenario, ticket #10028, is the other half of the same reset:
// the release walking down more than one level.  A ref cursor opened from
// within another ref cursor's own result set leaves the middle cursor a child
// of one statement and the parent of another, and re-executing the top
// statement has to hand back both ids.  What a client sees there is which id
// the next open() lands on, so that is what the assertions read.
//
// The third scenario is that same ticket's other half: two ref cursor binds in
// one statement, so the release runs its loop sideways over two children
// rather than down through one.  What a client sees there is which ids the
// next opens land on, and that neither of the two came back out of the pool
// carrying the other's leftovers.
//
// Cursor id reuse is deterministic rather than hoped for.  getCursor() in
// src/server/sqlrservercontroller.cpp walks the pool from the front and takes
// the first cursor in SQLRCURSORSTATE_AVAILABLE, so with only the parent busy
// the child is always the next id up, and the child released by one execute
// is always the one the next execute takes back.

static const unsigned char	ORA_TTI_QUERY2=0x47;

// TTI_CLOSE, an explicit close of one cursor - see close() in
// src/protocols/oracle.cpp
static const unsigned char	ORA_TTI_CLOSE=0x08;

// ORACLE_TYPE_RESULT_SET in src/protocols/oracle.cpp - the wire type a ref
// cursor bind carries, and the only thing that marks it as one.  it has no
// value behind it: the statement opens the cursor and the response hands back
// its id
static const unsigned char	ORA_TYPE_RESULT_SET=102;

// how wide a pointer field is on the wire - see the same constant in
// oraclemidfetchdescribe.cpp.  getPointer() in src/protocols/oracle.cpp sizes
// it off the representation the client offered first for DATATYPE_POINTER,
// and oracleprotocolclient offers the universal one
static const size_t	POINTER_SIZE=1;

// what a define descriptor's flag byte says about its position.  0x80 marks
// one the client never defined, a placeholder the module has to skip over; a
// real define carries 0x07 from a 9.2 client
static const unsigned char	ORA_DEFINE_FLAG_DEFINED=0x07;
static const unsigned char	ORA_DEFINE_FLAG_SKIPPED=0x80;

// the datatype, buffer size and character set of the real define in packet
// [0027] of samples/9810-redhat9x86-portable-midfetch-defines3-sqlrelay.
// oraproxy.  the module keeps all three and this test reads none of them back
static const unsigned char	ORA_DEFINE_DATATYPE=1;
static const uint32_t		ORA_DEFINE_BUFFER_SIZE=63;
static const uint32_t		ORA_DEFINE_CHARSET=31;

// how many counts ride behind a descriptor's four raw bytes
static const size_t	ORA_DEFINE_COUNTS=8;

// the ref cursor the stale define list gets planted on: three columns, two
// rows, every value distinct so a response says exactly which of them it
// carries
static const char	*refcursorthree=
	"begin open :rc for "
	"select decode(level,1,'AAAONE','AAATWO') c1,"
	"decode(level,1,'BBBONE','BBBTWO') c2,"
	"decode(level,1,'CCCONE','CCCTWO') c3 "
	"from dual connect by level<=2 order by 1; end;";

// and the fresh, unrelated one that reuses its cursor id - a different
// statement with a different column count, so a child that came out of the
// pool clean can't be mistaken for one that came out carrying the three
// column list above
static const char	*refcursortwo=
	"begin open :rc for "
	"select 'XCOLVALUE' c1,'YCOLVALUE' c2 from dual; end;";

// the block the grandchild scenario runs on the child's own cursor id: one
// column where the parent's block has three, so a fetch from it says outright
// whether it landed on an id of its own or back on the child's
static const char	*refcursorone=
	"begin open :rc for select 'GGGVALUE' g1 from dual; end;";

// the unrelated cursor's rows.  three of them, so each of the three points the
// grandchild scenario fetches it at has one left to bring back, and every
// value distinct so a response says which
static const char	*unrelatedrows=
	"select decode(level,1,'UUUROW1',2,'UUUROW2','UUUROW3') u1 "
	"from dual connect by level<=3 order by 1";

// and the plain two column select the released grandchild's id gets reopened
// with, a shape nothing in the chain above it ever ran
static const char	*twocolumns=
	"select 'PCOLVALUE' p1,'QCOLVALUE' p2 from dual";

// the block that opens two ref cursors at once, one per placeholder - the
// shape the release loop has to run more than one iteration for.  three
// columns and two rows each, the same shape the define list further down is
// built for, and every value distinct so a response says which of the two
// children it came from
static const char	*refcursorpair=
	"begin open :rc1 for "
	"select decode(level,1,'FFFONE','FFFTWO') f1,"
	"decode(level,1,'HHHONE','HHHTWO') f2,"
	"decode(level,1,'IIIONE','IIITWO') f3 "
	"from dual connect by level<=2 order by 1; "
	"open :rc2 for "
	"select decode(level,1,'JJJONE','JJJTWO') j1,"
	"decode(level,1,'KKKONE','KKKTWO') j2,"
	"decode(level,1,'LLLONE','LLLTWO') j3 "
	"from dual connect by level<=2 order by 1; end;";

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

// a pointer field, whose contents the module reads past without looking at
static void appendPointer(oracleprotocolclient *client) {
	for (size_t i=0; i<POINTER_SIZE; i++) {
		client->appendByte(0);
	}
}

// TTI_QUERY2 with nothing behind its header, which is all query2() in
// src/protocols/oracle.cpp reads for a call that neither defines nor binds.
// with OPTION_FETCH set it is an oci7 client's oexfet(), and its answer comes
// from sendFetchResponse() - the one fetch response that honors the cursor's
// define list
static bool query2(oracleprotocolclient *client, unsigned char sequence,
					uint32_t options, uint32_t cursorid) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

	return client->sendPacket() && client->recvPacket();
}

// TTI_QUERY2 carrying the define block an oci7 client's odefin's ride in -
// the same shape oraclemidfetchdescribe.cpp builds, read off packet [0027] of
// samples/9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy.
// behind the header come ten fields, the number of positions the define list
// names, nine more fields, and then one descriptor per position from 1 up: a
// descriptor is four raw bytes - the wire datatype, a flag, a precision and a
// scale - then eight counts, of which the first is the client's buffer size
// and the sixth its character set
static bool query2WithDefines(oracleprotocolclient *client,
					unsigned char sequence,
					uint32_t options,
					uint32_t cursorid,
					const bool *defined,
					size_t positions) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

	// the ten fields ahead of the count.  the pointer right in front of it
	// is the client's address of its own define array
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

	client->appendAuthCount((uint32_t)positions,4);

	// and the nine behind it
	appendPointer(client);
	for (size_t i=0; i<8; i++) {
		client->appendAuthCount(0,4);
	}

	for (size_t i=0; i<positions; i++) {

		client->appendByte(ORA_DEFINE_DATATYPE);
		client->appendByte((defined[i])?ORA_DEFINE_FLAG_DEFINED:
						ORA_DEFINE_FLAG_SKIPPED);
		client->appendByte(0);
		client->appendByte(0);

		// a placeholder's counts are all zero; a real define's carry
		// the buffer size in the first and the character set in the
		// sixth
		for (size_t j=0; j<ORA_DEFINE_COUNTS; j++) {
			uint32_t	value=0;
			if (defined[i] && !j) {
				value=ORA_DEFINE_BUFFER_SIZE;
			} else if (defined[i] && j==5) {
				value=ORA_DEFINE_CHARSET;
			}
			client->appendAuthCount(value,4);
		}
	}

	return client->sendPacket() && client->recvPacket();
}

// TTI_CLOSE, which carries the sequence number and the cursor id and nothing
// else.  close() in src/protocols/oracle.cpp answers it by releasing the
// cursor and everything the ref cursor arrays still name as its child, so it
// is where an over-release shows
static bool closeCursor(oracleprotocolclient *client, unsigned char sequence,
							uint32_t cursorid) {

	client->beginTtiCall(ORA_TTI_CLOSE);
	client->appendByte(sequence);
	client->appendAuthCount(cursorid,4);

	return client->sendPacket() && client->recvPacket();
}

// parse and execute one pl/sql block whose only placeholder is a ref cursor.
// OPTION_SNDIOV with OPTION_NOPLSQL clear is what puts classifyQuery3Binds()
// on the branch that reads the bind directions out of the block, and a ref
// cursor bind is the one type it recognizes from the descriptor alone - it
// carries no indicator flag, because the client binds a statement handle
// rather than a buffer.  the request still needs a row data block for the
// bind, valueless though it is: query3() only installs binds for a block it
// actually received
static bool openRefCursor(oracleprotocolclient *client,
					uint32_t cursorid, const char *block) {

	oracleprotocolbind	bind;
	bind.clear();
	bind.type=ORA_TYPE_RESULT_SET;

	oracleprotocolbindvalue	value;
	value.setNull();

	return client->query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_SNDIOV,
				cursorid,0,block,&bind,1,1,&value,1);
}

// the same request with a ref cursor bind per placeholder rather than one.
// nothing about a ref cursor bind is special-cased to a single one:
// installQuery3Binds() in src/protocols/oracle.cpp walks the descriptors in
// order, matches each to the placeholder at its own position, and takes a
// cursor out of the pool for every descriptor of this type - so a block with
// two placeholders comes back holding two children of its own, named in
// descriptor order.  the arrays below are sized for the widest block this
// test sends rather than genuinely parameterized - count is only ever 2
static bool openRefCursors(oracleprotocolclient *client, uint32_t cursorid,
					const char *block, uint32_t count) {

	oracleprotocolbind	binds[2];
	oracleprotocolbindvalue	values[2];
	for (uint32_t i=0; i<count; i++) {
		binds[i].clear();
		binds[i].type=ORA_TYPE_RESULT_SET;
		values[i].setNull();
	}

	return client->query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_SNDIOV,
				cursorid,0,block,binds,count,1,values,1);
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10000/#10028 "
				"ref cursor id reuse ======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf
	const char	*host="127.0.0.1";
	uint16_t	port=1521;
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	// which port that instance's first listener actually ended up on -
	// the same variable oracleprotocol.conf.in's @ORACLEPROTOCOLPORT1@ is
	// generated from, so one value drives both ends
	const char	*portoverride=
			environment::getValue("ORACLEPROTOCOLPORT1");
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

	// the cursor the pl/sql block itself runs on.  it is the first cursor
	// this session takes, so it is the front of the pool, and the child
	// the block opens is the id behind it
	uint32_t	parentid=0;
	if (!client.open(&parentid)) {
		report("open cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open cursor",true);

	uint32_t	childid=parentid+1;


	// ---- the child that leaves the stale define list behind ----

	if (!openRefCursor(&client,parentid,refcursorthree)) {
		report("open the three column ref cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open the three column ref cursor",
			client.getResponseTtcCode()!=ORA_TTC_ERROR);

	// a plain TTI_FETCH, which fetch3() answers with every column whatever
	// the cursor's define list says.  this is the baseline: it says the
	// child really did land on the id behind the parent's, and it says
	// what an undefined fetch of this statement looks like
	if (!client.fetch(childid,1)) {
		report("fetch row one from the child",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("the ref cursor child is the next cursor id",
			client.responseContains("AAAONE"));
	report("row one carries every column",
			client.responseContains("AAAONE") &&
			client.responseContains("BBBONE") &&
			client.responseContains("CCCONE"));

	// the define list this test exists to see cleared: positions 1 and 3
	// of three, with position 2 a placeholder the client never defined
	static const bool	defined[]={ true, false, true };
	static const size_t	definedcount=
				sizeof(defined)/sizeof(defined[0]);
	if (!query2WithDefines(&client,1,ORA_OPTION_DEFINE,
					childid,defined,definedcount)) {
		report("define the child's columns",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("define the child's columns",
			client.getResponseTtcCode()!=ORA_TTC_ERROR);

	// and the fetch that proves the define list took.  an exact fetch is
	// the only fetch that reads it, so this is also the shape the
	// assertion below has to use
	if (!query2(&client,2,ORA_OPTION_FETCH,childid)) {
		report("exact fetch from the defined child",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("the defined child sends only the columns it was asked for",
			client.responseContains("AAATWO") &&
			!client.responseContains("BBBTWO") &&
			client.responseContains("CCCTWO"));


	// ---- the fresh child that must not inherit it ----

	// re-executing the parent is what hands the child back: installQuery3
	// Binds() releases whatever the previous execute's ref cursor binds
	// took before it takes any of its own, so the id above goes back to
	// the front of the pool and comes straight back out as this
	// statement's child
	if (!openRefCursor(&client,parentid,refcursortwo)) {
		report("open the two column ref cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open the two column ref cursor",
			client.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!client.fetch(childid,1)) {
		report("fetch from the fresh child",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("the fresh ref cursor child reused the same cursor id",
			client.responseContains("XCOLVALUE"));

	// the assertion the whole test is built around.  pre-fix, the three
	// column define list above is still on this cursor, and it names
	// position 2 as one the client never defined - so the second column
	// of a statement that has nothing to do with it goes missing
	if (!openRefCursor(&client,parentid,refcursortwo)) {
		report("re-open the two column ref cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("re-open the two column ref cursor",
			client.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!query2(&client,3,ORA_OPTION_FETCH,childid)) {
		report("exact fetch from the fresh child",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("the fresh child sends its first column",
			client.responseContains("XCOLVALUE"));
	report("the fresh child sends its second column",
			client.responseContains("YCOLVALUE"));


	// ---- the grandchild two levels down ----

	// ticket #10028: a ref cursor opened from within another ref cursor's
	// own result set, so the middle cursor is a child of one statement and
	// the parent of another at the same time.  releasing the top statement
	// has to walk down both levels - releaseRefCursors() releases a child's
	// own children before it resets the child.  before it recursed, the
	// grandchild's id was held by nothing and out of the pool for the rest
	// of the session.
	//
	// this runs in a session of its own, because the assertions name ids
	// counted up from the first cursor the session opens and the scenario
	// above has already moved the pool along
	client.disconnect();

	oracleprotocolclient	fresh;

	if (!fresh.connect(host,port,sid)) {
		report("connect a fresh session",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("connect a fresh session",true);

	if (!fresh.login(user,password)) {
		report("log the fresh session in",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("log the fresh session in",true);

	// the unrelated cursor the release must leave alone.  it is the first
	// cursor the session opens, so it sits at the front of the pool with a
	// live result set on it while the whole chain below runs
	uint32_t	unrelatedid=0;
	if (!fresh.open(&unrelatedid)) {
		report("open the unrelated cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("open the unrelated cursor",true);

	if (!fresh.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				unrelatedid,0,unrelatedrows)) {
		report("run the unrelated select",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("run the unrelated select",
			fresh.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!fresh.fetch(unrelatedid,1)) {
		report("fetch row one from the unrelated cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("the unrelated cursor has a live result set",
			fresh.responseContains("UUUROW1"));

	// the top of the chain, and the two ids behind it the chain takes: the
	// child the parent's block opens, and the grandchild the child's own
	// block opens
	uint32_t	topid=0;
	if (!fresh.open(&topid)) {
		report("open the parent cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("open the parent cursor",true);

	uint32_t	midid=topid+1;
	uint32_t	grandid=midid+1;

	if (!openRefCursor(&fresh,topid,refcursorthree)) {
		report("open the parent's ref cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("open the parent's ref cursor",
			fresh.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!fresh.fetch(midid,1)) {
		report("fetch row one from the parent's ref cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("the parent's ref cursor child is the next cursor id",
			fresh.responseContains("AAAONE"));

	// the grandchild: the same ref cursor bind, aimed at the child's own
	// cursor id rather than the parent's.  query3()'s cursorFromWireId()
	// resolves it, installQuery3Binds() runs against the child, and the
	// child ends up with a ref cursor count of its own - a cursor that is
	// somebody's child and somebody's parent at once, which is the state
	// the recursion below exists for.
	//
	// the backend is allowed to refuse this.  the child is holding the
	// parent's result set, and live oci does not have to accept a new
	// pl/sql block on it.  a refusal at execute time still leaves the
	// bookkeeping the release walks written, since installQuery3Binds()
	// takes the grandchild's cursor before the execute runs, but a refusal
	// at parse time returns out of query3() ahead of installQuery3Binds()
	// and takes nothing at all.  only a wire level failure is fatal here,
	// the ttc code is deliberately not checked, and the open() below is
	// what says which of the two happened
	if (!openRefCursor(&fresh,midid,refcursorone)) {
		report("open a ref cursor from the child's own cursor id",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("open a ref cursor from the child's own cursor id",true);

	// best effort for the same reason, so it prints rather than reports:
	// if the block did run, the grandchild's one column says it landed on
	// an id of its own rather than back on the child's.  it is a
	// diagnostic detail, not a check - whether the id was taken at all is
	// what the open() below asserts
	stdoutput.printf("the grandchild's own rows: %s\n",
			(fresh.fetch(grandid,1) &&
				fresh.responseContains("GGGVALUE"))?
						"fetched":"not available");

	// and the check the release assertion rests on.  the bind takes the
	// grandchild's id out of the pool, so an open() now has to land past
	// it.  a block refused at parse time takes nothing, and this open()
	// comes back with the grandchild's own id - which would leave the
	// assertion further down with nothing released to see and passing on
	// an empty chain
	uint32_t	besideid=0;
	if (!fresh.open(&besideid)) {
		report("open a cursor beside the grandchild",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("open a cursor beside the grandchild",true);
	report("the grandchild's bind took its id out of the pool",
			besideid!=grandid);

	// a define list on the grandchild itself, the way the child one level
	// up carries one, so the release two levels down has something to
	// clear and the exact fetch at the end has something to catch
	if (!query2WithDefines(&fresh,4,ORA_OPTION_DEFINE,
					grandid,defined,definedcount)) {
		report("define the grandchild's columns",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("define the grandchild's columns",
			fresh.getResponseTtcCode()!=ORA_TTC_ERROR);

	// and hand the id beside it back, so the pool below is exactly what it
	// would have been without the check above
	if (!closeCursor(&fresh,5,besideid)) {
		report("close the cursor beside the grandchild",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("close the cursor beside the grandchild",
			fresh.getResponseTtcCode()!=ORA_TTC_ERROR);

	// re-executing the parent is what makes the release walk down both
	// levels: installQuery3Binds() calls releaseRefCursors() on the parent
	// before it takes any cursor of its own, and that goes parent to child
	// to grandchild
	if (!openRefCursor(&fresh,topid,refcursorthree)) {
		report("re-open the parent's ref cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("re-open the parent's ref cursor",
			fresh.getResponseTtcCode()!=ORA_TTC_ERROR);

	// the assertion this scenario is built around.  the release hands back
	// the child and the grandchild both, and the child goes straight back
	// out as the re-executed parent's own child - so the grandchild's id is
	// the lowest one free and the next open() has to land on it.  before
	// the release recursed it stopped at the child, the grandchild's id was
	// never handed back, and this open() landed one past it
	uint32_t	reopenedid=0;
	if (!fresh.open(&reopenedid)) {
		report("open a cursor after the parent re-executed",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("open a cursor after the parent re-executed",true);
	report("the released grandchild's id is the next one out of the pool",
			reopenedid==grandid);

	// and it comes out of the pool clean, the way the child one level up
	// does.  an exact fetch is the one fetch that reads the define list,
	// and the list planted on the grandchild above names position 2 as one
	// the client never defined - so a release that handed the id back
	// without clearing it drops the second column here
	if (!fresh.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				reopenedid,0,twocolumns)) {
		report("run a two column select on the reopened id",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("run a two column select on the reopened id",
			fresh.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!query2(&fresh,6,ORA_OPTION_FETCH,reopenedid)) {
		report("exact fetch from the reopened id",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("the reopened id sends its first column",
			fresh.responseContains("PCOLVALUE"));
	report("the reopened id sends its second column",
			fresh.responseContains("QCOLVALUE"));

	// and nothing walked off the chain onto the cursor beside it: the
	// unrelated cursor still has its result set after the release, and
	// still has it after the child is closed outright - close() releases
	// everything the ref cursor arrays still name as the closing cursor's
	// child, so a row left naming an id the chain no longer owns would
	// show up here.  neither of these fails against a release that stops
	// one level short; they guard against a later one that goes too far
	if (!fresh.fetch(unrelatedid,1)) {
		report("fetch row two from the unrelated cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("the unrelated cursor survives the release",
			fresh.responseContains("UUUROW2"));

	if (!closeCursor(&fresh,7,midid)) {
		report("close the child cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("close the child cursor",
			fresh.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!fresh.fetch(unrelatedid,1)) {
		report("fetch row three from the unrelated cursor",false);
		stdoutput.printf("%s\n",fresh.getError());
		return status;
	}
	report("the unrelated cursor survives the child's close",
			fresh.responseContains("UUUROW3"));

	fresh.disconnect();


	// ---- two direct children of one parent ----

	// the other half of ticket #10028: one statement whose block opens a ref
	// cursor into each of two bind slots, so releaseRefCursors() runs its
	// loop over two children in a single call rather than one.  what that
	// shows is the loop itself: that it walks every child and not just the
	// one it reaches first, with the count saved into a local and the
	// parent's row zeroed under it before it starts.  the re-entry that
	// saved count guards against is not something a client can drive from
	// the wire, so it stays verified by inspection - this only shows the
	// loop staying correct with more than one child in play.
	//
	// its own session again, since the ids below are counted up from the
	// first cursor the session opens
	oracleprotocolclient	pair;

	if (!pair.connect(host,port,sid)) {
		report("connect a second fresh session",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("connect a second fresh session",true);

	if (!pair.login(user,password)) {
		report("log the second fresh session in",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("log the second fresh session in",true);

	// the cursor beside the pair that the release must leave alone, holding a
	// live result set at the front of the pool for the whole scenario
	uint32_t	sideid=0;
	if (!pair.open(&sideid)) {
		report("open the cursor beside the pair",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("open the cursor beside the pair",true);

	if (!pair.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				sideid,0,unrelatedrows)) {
		report("run the select beside the pair",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("run the select beside the pair",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!pair.fetch(sideid,1)) {
		report("fetch row one from the cursor beside the pair",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the cursor beside the pair has a live result set",
			pair.responseContains("UUUROW1"));

	// the statement that holds both, and the two ids behind it its two binds
	// take - one per placeholder, in the order the descriptors go out
	uint32_t	pairid=0;
	if (!pair.open(&pairid)) {
		report("open the cursor the pair hangs off",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("open the cursor the pair hangs off",true);

	uint32_t	firstid=pairid+1;
	uint32_t	secondid=pairid+2;

	if (!openRefCursors(&pair,pairid,refcursorpair,2)) {
		report("open two ref cursors from one block",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("open two ref cursors from one block",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	// both are genuinely their own cursor with their own result set, not one
	// cursor answering to two ids: each carries only its own block's values
	if (!pair.fetch(firstid,1)) {
		report("fetch row one from the first child",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the first child is the next cursor id",
			pair.responseContains("FFFONE") &&
			!pair.responseContains("JJJONE"));

	if (!pair.fetch(secondid,1)) {
		report("fetch row one from the second child",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the second child is the id behind it",
			pair.responseContains("JJJONE") &&
			!pair.responseContains("FFFONE"));

	// a define list on each, so the release has something to clear on both
	// ends of the loop rather than just the one it reaches first
	if (!query2WithDefines(&pair,1,ORA_OPTION_DEFINE,
					firstid,defined,definedcount)) {
		report("define the first child's columns",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("define the first child's columns",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!query2WithDefines(&pair,2,ORA_OPTION_DEFINE,
					secondid,defined,definedcount)) {
		report("define the second child's columns",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("define the second child's columns",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!query2(&pair,3,ORA_OPTION_FETCH,firstid)) {
		report("exact fetch from the defined first child",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the defined first child sends only what it was asked for",
			pair.responseContains("FFFTWO") &&
			!pair.responseContains("HHHTWO") &&
			pair.responseContains("IIITWO"));

	if (!query2(&pair,4,ORA_OPTION_FETCH,secondid)) {
		report("exact fetch from the defined second child",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the defined second child sends only what it was asked for",
			pair.responseContains("JJJTWO") &&
			!pair.responseContains("KKKTWO") &&
			pair.responseContains("LLLTWO"));

	// re-executing the statement is what runs the loop.  it goes back with a
	// block that opens one ref cursor rather than two, so only the first of
	// the two ids it just handed back comes straight out again and the second
	// is left sitting at the front of the pool for the open() below to find
	if (!openRefCursor(&pair,pairid,refcursortwo)) {
		report("re-open the pair's statement with one ref cursor",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("re-open the pair's statement with one ref cursor",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!pair.fetch(firstid,1)) {
		report("fetch from the child of the re-executed statement",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the loop's first iteration handed its child back",
			pair.responseContains("XCOLVALUE"));

	// and the assertion this scenario is built around: the second iteration
	// ran too.  a loop that stopped after one child, or that lost its place
	// when the parent's row was zeroed, would leave this id out of the pool
	// and the open() would land past it
	uint32_t	secondbackid=0;
	if (!pair.open(&secondbackid)) {
		report("open a cursor after the pair was released",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("open a cursor after the pair was released",true);
	report("the loop's second iteration handed its child back",
			secondbackid==secondid);

	// both came back clean, not just the one the loop reached first.  an
	// exact fetch is the one fetch that reads the define list, so either
	// child's leftover three column list would drop a column of the two
	// column statement it lands under
	if (!pair.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				secondbackid,0,twocolumns)) {
		report("run a two column select on the second child's id",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("run a two column select on the second child's id",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!query2(&pair,5,ORA_OPTION_FETCH,secondbackid)) {
		report("exact fetch from the second child's id",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the second child's id sends its first column",
			pair.responseContains("PCOLVALUE"));
	report("the second child's id sends its second column",
			pair.responseContains("QCOLVALUE"));

	// the first child's id gets the same reading, and it takes another
	// re-execute to get there: the row the fetch above consumed is the only
	// one that statement has
	if (!openRefCursor(&pair,pairid,refcursortwo)) {
		report("re-open the pair's statement again",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("re-open the pair's statement again",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!query2(&pair,6,ORA_OPTION_FETCH,firstid)) {
		report("exact fetch from the first child's id",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the first child's id sends its first column",
			pair.responseContains("XCOLVALUE"));
	report("the first child's id sends its second column",
			pair.responseContains("YCOLVALUE"));

	// and neither release walked off the pair onto the cursor beside it,
	// either as the statement re-executed or as it closed outright
	if (!pair.fetch(sideid,1)) {
		report("fetch row two from the cursor beside the pair",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the cursor beside the pair survives the release",
			pair.responseContains("UUUROW2"));

	if (!closeCursor(&pair,7,pairid)) {
		report("close the cursor the pair hangs off",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("close the cursor the pair hangs off",
			pair.getResponseTtcCode()!=ORA_TTC_ERROR);

	if (!pair.fetch(sideid,1)) {
		report("fetch row three from the cursor beside the pair",false);
		stdoutput.printf("%s\n",pair.getError());
		return status;
	}
	report("the cursor beside the pair survives the close",
			pair.responseContains("UUUROW3"));

	pair.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
