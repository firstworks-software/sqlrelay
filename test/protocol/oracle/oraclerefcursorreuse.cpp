// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for ticket #10000: the per-cursor state a ref cursor
// child carries back into the pool.
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
// Cursor id reuse is deterministic rather than hoped for.  getCursor() in
// src/server/sqlrservercontroller.cpp walks the pool from the front and takes
// the first cursor in SQLRCURSORSTATE_AVAILABLE, so with only the parent busy
// the child is always the next id up, and the child released by one execute
// is always the one the next execute takes back.

static const unsigned char	ORA_TTI_QUERY2=0x47;

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

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10000 ref cursor id reuse ======\n\n");

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

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
