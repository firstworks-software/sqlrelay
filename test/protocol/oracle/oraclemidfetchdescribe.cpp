// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/bytebuffer.h>
#include <rudiments/bytestring.h>
#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for ticket #9810: what describe(), fetch() and sendFetchResponse()
// in src/protocols/oracle.cpp answer when a TTI_DESCRIBE lands on a cursor
// that is already mid-fetch.
//
// A real OCI7 client segfaults inside libclntsh's ttcacs(), in an ofen() that
// comes after the "odescr - mid-fetch" section of oci7.cpp - so the answer
// that kills it is a fetch response, not a describe response.  That section
// runs oopen, oparse, odefin on column 1 only, oexec, one ofen, then odescr on
// columns 1 through 4, an out-of-range odescr on column 5, odescr 1 through 4
// again, and three more ofen calls.
//
// This drives that sequence over a raw socket, so it needs no OCI7 client.
// Every call it puts on the wire is one a real OCI7 client sends, in the same
// order and with the same options: TTI_OSQL7 to parse, TTI_QUERY2 with
// OPTION_DEFINE|OPTION_EXECUTE|OPTION_NOPLSQL to execute, TTI_FETCH to fetch,
// TTI_DESCRIBE to describe.  A real client caches the whole select list off
// one describe, and samples/9808-redhat9x86-native-fetch.oraproxy shows it
// putting only that one on the wire for a whole group of odescr calls, so the
// describes for positions 2 through 4 here are calls no capture carries - they
// are there because the answer has to be the same whichever column an odescr
// names.
//
// It also covers ticket #9973: the same describe one call earlier, between the
// TTI_QUERY2 that executes and the first TTI_FETCH, where the client has
// fetched nothing yet.  Two more arms cover that, and they run against a
// sequence rather than the table - see the predescribe arms below.
//
// It runs three arms against the same table and compares them:
//
//	- parse: a cursor that is parsed and nothing else, described once.
//	  the shape oracledescribe.cpp already covers, here only as the
//	  baseline the mid-fetch describe has to match - a real server's
//	  answer is byte-identical either way
//	- midfetch: the oci7.cpp sequence in full
//	- control: the same execute and fetches with no describe between
//	  them, which is what the midfetch arm's fetch responses get
//	  compared against
//
// The describe answers also get compared against a model of a real 10.2
// server's own, built the way oracledescribe.cpp builds one - from the column
// metadata rather than from anything the module does - so a match means the
// module agrees with a real server and not just with itself.
//
// The three arms above all send a TTI_QUERY2 with no define block behind its
// header, which is the fallback: nothing to decode, so every column of the
// select list goes out.  A real oci7 client's odefin's ride inside the
// TTI_QUERY2 its oexec() sends, and a real server answers with the columns
// they named and no others - sending the rest is what overruns the buffers
// the client set up and kills it.  So five more arms send a define block and
// check the fetch answer against it:
//
//	- all four positions defined, which is the fallback's answer too and
//	  so the arm that says the define walk didn't break the common case
//	- position 1 only, position 3 only, and positions 1 and 2 - each
//	  answered with those columns, that many in the row header, and
//	  nothing at all for the rest, not even a null marker
//	- a define block the walk has to reject, which falls back to every
//	  column rather than to a short row
//
// The wire shape of the block is packet [0027] of samples/
// 9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy, and what a
// real server answers it with is packet [0030] of samples/
// 9810-redhat9x86-native-midfetch-defines3-realserver.oraproxy - the same
// request to a real 10.2 server, whose row header says 1 column and whose row
// carries only "varchar1".

static const unsigned char	ORA_TTI_OSQL7=0x4a;
static const unsigned char	ORA_TTI_QUERY2=0x47;

static const unsigned char	ORA_TTC_STATUS=0x09;

// what a real oci7 client's oexec() sends in a query2 - packet [0027] of
// samples/9808-redhat9x86-native-fetch.oraproxy
static const uint32_t	ORA_QUERY2_EXEC_OPTIONS=
			ORA_OPTION_DEFINE|ORA_OPTION_EXECUTE|ORA_OPTION_NOPLSQL;

// the module's AL32UTF8 default, which the oracleprotocol instance this logs
// into is on
static const uint32_t	ORA_CHARSET_DEFAULT=873;

// oci7.cpp's protocoltesttable, rebuilt under a name of this test's own so a
// concurrent oci7/oci8 run's copy is left alone.  the columns, their order and
// the three rows are all oci7.cpp's, since the describe answer carries the
// column names and the fetch answers carry the row values
static const char	*createtable=
	"create table protocoltest9810midfetch ("
		"testnumber number(10),"
		"testchar char(20),"
		"testvarchar varchar2(40),"
		"testdate date)";
static const char	*droptable=
	"drop table protocoltest9810midfetch";
static const char	*insertrow1=
	"insert into protocoltest9810midfetch values "
	"(1,'char1','varchar1',"
	"to_date('2001-01-01 01:01:01','YYYY-MM-DD HH24:MI:SS'))";
static const char	*insertrow2=
	"insert into protocoltest9810midfetch values "
	"(2,'char2','varchar2',"
	"to_date('2002-02-02 02:02:02','YYYY-MM-DD HH24:MI:SS'))";
static const char	*insertrow3=
	"insert into protocoltest9810midfetch values (3,NULL,NULL,NULL)";
static const char	*selectquery=
	"select * from protocoltest9810midfetch order by testnumber";

// a sequence of this test's own, for the predescribe arm.  a describe that
// re-runs the statement rewinds the result set, and on the table above that
// is invisible - the fetch behind it answers row 1 whether the statement ran
// once or twice.  a nextval answers a different value every time it runs, so
// the arm's own fetch says how many executes it took to get there.  the start
// value is far enough from anything else on the wire that a response carrying
// it can't be carrying it by accident
static const char	*createsequence=
	"create sequence protocolseq9973 start with 7654321 increment by 1";
static const char	*dropsequence=
	"drop sequence protocolseq9973";
static const char	*sequencequery=
	"select protocolseq9973.nextval from dual";
static const char	*firstnextval="7654321";
static const char	*secondnextval="7654322";

// what describe() sends as the sequence number, which an ORA-01007 echoes
// back in the call number field
static const unsigned char	DESCRIBE_SEQUENCE_NUMBER=10;

// one column value, as a legacy fetch's row carries it
struct columnvalue {
	const unsigned char	*value;
	size_t			size;
};

// the first row of the table above, column by column.  the char column comes
// back blank padded to its full 20, and the date as the seven byte oracle
// date rather than as text - both of them the way packet [0030] of
// samples/9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy
// carries them
static const unsigned char	row1testnumber[]={ '1' };
static const unsigned char	row1testchar[]={
	'c', 'h', 'a', 'r', '1', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
};
static const unsigned char	row1testvarchar[]={
	'v', 'a', 'r', 'c', 'h', 'a', 'r', '1'
};
static const unsigned char	row1testdate[]={
	0x78, 0x65, 0x01, 0x01, 0x02, 0x02, 0x02
};
static const columnvalue	row1[]={
	{ row1testnumber, sizeof(row1testnumber) },
	{ row1testchar, sizeof(row1testchar) },
	{ row1testvarchar, sizeof(row1testvarchar) },
	{ row1testdate, sizeof(row1testdate) }
};

// the date column the way "define arm: all four positions" below gets it
// back.  that arm defines every position SQLT_STR (dty 1, ORA_DEFINE_DATATYPE
// below) with a 63 byte buffer, and #9974 taught putField()'s
// ORACLE_TYPE_DATE case (src/protocols/oracle.cpp) to honor that: a date
// defined SQLT_STR goes out as the backend's own text instead of the seven
// byte binary form above.  date_to_text_format=YYYY-MM-DD HH24:MI:SS in
// test/sqlrelay.conf.d/oracleprotocol.conf shapes that text, and insertrow1's
// to_date() call put the same string in, so the round trip lands on it
// exactly.  the other three columns are unaffected by the same define - see
// putField()'s ORACLE_TYPE_CHAR/VARCHAR/FIXED_CHAR/NUMBER/VARNUM case, which
// already answers every one of those in its natural form whatever type the
// define asked for - so row1alldefined only overrides the date entry
static const unsigned char	row1testdatevarchar2[]={
	'2', '0', '0', '1', '-', '0', '1', '-', '0', '1', ' ',
	'0', '1', ':', '0', '1', ':', '0', '1'
};
static const columnvalue	row1alldefined[]={
	{ row1testnumber, sizeof(row1testnumber) },
	{ row1testchar, sizeof(row1testchar) },
	{ row1testvarchar, sizeof(row1testvarchar) },
	{ row1testdatevarchar2, sizeof(row1testdatevarchar2) }
};

// packet [0022] of
// samples/9808-solaris8sparc-portable-realtable-outofrange.oraproxy - a real
// 10.2 server's answer to odescr() on a position past the last column
static const unsigned char	portableoutofrange[]={
	0x01, 0x01, 0x00, 0x02, 0x03, 0xef, 0x00, 0x00,
	0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27,
	0x4f, 0x52, 0x41, 0x2d, 0x30, 0x31, 0x30, 0x30,
	0x37, 0x3a, 0x20, 0x76, 0x61, 0x72, 0x69, 0x61,
	0x62, 0x6c, 0x65, 0x20, 0x6e, 0x6f, 0x74, 0x20,
	0x69, 0x6e, 0x20, 0x73, 0x65, 0x6c, 0x65, 0x63,
	0x74, 0x20, 0x6c, 0x69, 0x73, 0x74, 0x0a
};

// packet [0028] of
// samples/9808-redhat9x86-native-realtable-outofrange.oraproxy - the same
// answer in the native encoding
static const unsigned char	nativeoutofrange[]={
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0xef, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x36, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x13, 0xbd,
	0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x4f, 0x52,
	0x41, 0x2d, 0x30, 0x31, 0x30, 0x30, 0x37, 0x3a,
	0x20, 0x76, 0x61, 0x72, 0x69, 0x61, 0x62, 0x6c,
	0x65, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x69, 0x6e,
	0x20, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x20,
	0x6c, 0x69, 0x73, 0x74, 0x0a
};

// where the summary object an ORA-01007 travels in keeps the fields that are
// session data rather than protocol - the same offsets oracledescribe.cpp
// works from.  the cursor id and the call number legitimately differ per
// session; the last two are the native encoding's alone, and are the two
// fields the module answers differently from the capture
static const size_t	NATIVE_CURSOR_ID_OFFSET=15;
static const size_t	NATIVE_CALL_NUMBER_OFFSET=46;
static const size_t	NATIVE_SUCCESS_ITERATIONS_OFFSET=49;
static const size_t	NATIVE_LIVE_POINTER_OFFSET=61;
static const size_t	PORTABLE_CURSOR_ID_OFFSET=8;
static const size_t	PORTABLE_CALL_NUMBER_OFFSET=24;

// one column, as much of it as a describe answer carries - the fields
// putOci7DescribeColumn() writes in src/protocols/oracle.cpp, in its order,
// minus the ones that are constant across every capture on file
struct describecolumn {
	unsigned char	type;
	bool		character;
	unsigned char	precision;
	signed char	scale;
	uint32_t	size;
	unsigned char	nullok;
	const char	*name;
};

// the four columns above, as a real server describes them.  TESTDATE's size is
// 1 because a describe reports a date that way whatever a date really takes -
// see the same table in oracledescribe.cpp, whose model these values reproduce
// a real 10.2 server's own bytes with
static const describecolumn	tablecolumns[]={
	{ 2, false, 10, 0, 22, 1, "TESTNUMBER" },
	{ 96, true, 0, 0, 20, 1, "TESTCHAR" },
	{ 1, true, 0, 0, 40, 1, "TESTVARCHAR" },
	{ 12, false, 0, 0, 1, 1, "TESTDATE" }
};
static const size_t	tablecolumncount=
			sizeof(tablecolumns)/sizeof(tablecolumns[0]);

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static void hexDump(const char *label,
			const unsigned char *bytes, size_t size) {
	stdoutput.printf("%s (%d bytes):\n",label,(int)size);
	for (size_t i=0; i<size; i++) {
		stdoutput.printf("%02x%s",bytes[i],((i%16)==15)?"\n":" ");
	}
	if (size%16) {
		stdoutput.printf("\n");
	}
}

static void dumpResponse(const char *label, oracleprotocolclient *client) {
	stdoutput.printf("  <- ttc code 0x%02x\n",
				(int)client->getResponseTtcCode());
	hexDump(label,client->getResponse(),client->getResponseSize());
	stdoutput.printf("\n");
}

static void saveResponse(bytebuffer *out, oracleprotocolclient *client) {
	out->clear();
	out->append(client->getResponse(),client->getResponseSize());
}

// compare, and say where and how they parted company rather than just that
// they did
static bool compareBytes(const char *label,
				const unsigned char *actual, size_t actualsize,
				const unsigned char *expected,
				size_t expectedsize) {

	size_t	common=(actualsize<expectedsize)?actualsize:expectedsize;
	for (size_t i=0; i<common; i++) {
		if (actual[i]!=expected[i]) {
			stdoutput.printf("%s: first difference at offset "
					"%d (0x%02x, expected 0x%02x)\n",
					label,(int)i,actual[i],expected[i]);
			hexDump("actual",actual,actualsize);
			hexDump("expected",expected,expectedsize);
			return false;
		}
	}
	if (actualsize!=expectedsize) {
		stdoutput.printf("%s: %d bytes, expected %d\n",
				label,(int)actualsize,(int)expectedsize);
		hexDump("actual",actual,actualsize);
		hexDump("expected",expected,expectedsize);
		return false;
	}
	return true;
}

// the same, for two answers that came back on different cursors.  every
// oci7-shaped answer ends with a summary object carrying the cursor id, so a
// byte that differs by exactly that substitution is not a difference; every
// other one still is
static bool compareApartFromCursorId(const char *label,
				const unsigned char *actual, size_t actualsize,
				unsigned char actualcursorid,
				const unsigned char *expected,
				size_t expectedsize,
				unsigned char expectedcursorid) {

	size_t	common=(actualsize<expectedsize)?actualsize:expectedsize;
	for (size_t i=0; i<common; i++) {
		if (actual[i]==expected[i]) {
			continue;
		}
		if (actual[i]==actualcursorid &&
				expected[i]==expectedcursorid) {
			continue;
		}
		stdoutput.printf("%s: first difference at offset "
				"%d (0x%02x, expected 0x%02x)\n",
				label,(int)i,actual[i],expected[i]);
		hexDump("actual",actual,actualsize);
		hexDump("expected",expected,expectedsize);
		return false;
	}
	if (actualsize!=expectedsize) {
		stdoutput.printf("%s: %d bytes, expected %d\n",
				label,(int)actualsize,(int)expectedsize);
		hexDump("actual",actual,actualsize);
		hexDump("expected",expected,expectedsize);
		return false;
	}
	return true;
}


// ---- the calls oracleprotocolclient doesn't have ----

// how wide a pointer field is on the wire.  it is 1 here whatever the
// encoding: getPointer() in src/protocols/oracle.cpp sizes it off the
// representation the client offered first for DATATYPE_POINTER in the data
// type negotiation, not off the encoding, and oracleprotocolclient offers the
// universal one.  oracleprotocolclient's own appendAuthPointer() writes the
// eight byte pointer a login request carries, which is a different field
static const size_t	POINTER_SIZE=1;

// TTI_OSQL7 - an oci7 client's oparse().  the field order is osql7()'s read
// order in src/protocols/oracle.cpp: a sequence byte, two counts, then
// alternating pointers and counts, then a fixed run of bytes it skips without
// reading, and last the sql text as a clr
static bool osql7(oracleprotocolclient *client, unsigned char sequence,
					uint32_t cursorid, const char *query) {

	size_t	querysize=charstring::getLength(query);

	client->beginTtiCall(ORA_TTI_OSQL7);
	client->appendByte(sequence);
	client->appendLenPreInt(1);
	client->appendLenPreInt(cursorid);
	client->appendByte(POINTER_SIZE);
	client->appendLenPreInt((uint32_t)querysize);
	client->appendByte(POINTER_SIZE);
	client->appendLenPreInt(0);
	client->appendByte(POINTER_SIZE);
	client->appendLenPreInt(0);

	// four pointers and three counts that are zero in every capture, and
	// that osql7() skips as a run rather than parsing.  it sizes the run
	// off the pointer width, so the zeros have to be written the same way
	for (size_t i=0; i<POINTER_SIZE*4+3; i++) {
		client->appendByte(0);
	}

	client->appendLenBytes(query,querysize);

	return client->sendPacket() && client->recvPacket();
}

// TTI_QUERY2 - an oci7 client's oexec().  query2() in
// src/protocols/oracle.cpp reads three fields and skips the rest of the
// request, so three fields are all this writes
static bool query2(oracleprotocolclient *client, unsigned char sequence,
					uint32_t options, uint32_t cursorid) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

	return client->sendPacket() && client->recvPacket();
}

// what a define descriptor's flag byte says about its position.  0x80 marks
// one the client never defined, a placeholder the module has to skip over.
// a real define carries 0x07 from a 9.2 client and 0x00 from a 9.0.1 one, and
// the module reads 0x80 as a bit rather than comparing the byte whole, so
// either of those reads as defined
static const unsigned char	ORA_DEFINE_FLAG_DEFINED=0x07;
static const unsigned char	ORA_DEFINE_FLAG_SKIPPED=0x80;

// the datatype, buffer size and character set the real define in packet
// [0027] of samples/9810-redhat9x86-portable-midfetch-defines3-sqlrelay.
// oraproxy asks for.  they are the capture's so that what this builds is
// what a real client sent.  #9974 (closed) taught the module to honor the
// datatype rather than ignore it - see definedColumnType() in
// src/protocols/oracle.cpp - so a define arm that includes the date column
// (only "define arm: all four positions" below does) gets it back
// converted to this type; row1alldefined is that arm's expected row
static const unsigned char	ORA_DEFINE_DATATYPE=1;
static const uint32_t		ORA_DEFINE_BUFFER_SIZE=63;
static const uint32_t		ORA_DEFINE_CHARSET=31;

// how many counts ride behind a descriptor's four raw bytes
static const size_t	ORA_DEFINE_COUNTS=8;

// a pointer field, whose contents the module reads past without looking at
static void appendPointer(oracleprotocolclient *client) {
	for (size_t i=0; i<POINTER_SIZE; i++) {
		client->appendByte(0);
	}
}

// how many positions a define list has to name to reach the last one the
// client defined, which is what its count field carries - defining only
// position 3 of four still sends three descriptors
static size_t definedPositions(const bool *defined, size_t colcount) {
	size_t	positions=0;
	for (size_t i=0; i<colcount; i++) {
		if (defined[i]) {
			positions=i+1;
		}
	}
	return positions;
}

static size_t definedCount(const bool *defined, size_t colcount) {
	size_t	count=0;
	for (size_t i=0; i<colcount; i++) {
		if (defined[i]) {
			count++;
		}
	}
	return count;
}

// TTI_QUERY2 with the define block an oci7 client's odefin's ride in, which
// query2() above leaves out entirely.  behind the header comes a preamble of
// ten fields, the number of positions the define list names, nine more
// fields, and then one descriptor per position from 1 up.  a descriptor is
// four raw bytes - the wire datatype, a flag, a precision and a scale - then
// eight counts, of which the first is the client's buffer size and the sixth
// its character set.
//
// the whole shape is packet [0027] of samples/
// 9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy, whose 90 byte
// payload defines only position 3 of four: two 12-byte placeholders and then
// one 14-byte real descriptor, ending on the request's last byte.
//
// "junk" is how many bytes to hang off the end past that last byte, for the
// arm that checks what a block the module can't walk falls back to
static bool query2WithDefines(oracleprotocolclient *client,
					unsigned char sequence,
					uint32_t options,
					uint32_t cursorid,
					const bool *defined,
					size_t colcount,
					size_t junk) {

	size_t	positions=definedPositions(defined,colcount);

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

	for (size_t i=0; i<junk; i++) {
		client->appendByte(0);
	}

	return client->sendPacket() && client->recvPacket();
}

// TTI_FETCH - an oci7 client's ofen().  oracleprotocolclient's own fetch()
// writes its two counts as length-prefixed ints whatever the encoding, so it
// can't drive the native side of fetch()'s getAuthCount() reads
static bool legacyRowFetch(oracleprotocolclient *client,
					unsigned char sequence,
					uint32_t cursorid, uint32_t rows) {

	client->beginTtiCall(ORA_TTI_FETCH);
	client->appendByte(sequence);
	client->appendAuthCount(cursorid,4);
	client->appendAuthCount(rows,4);

	return client->sendPacket() && client->recvPacket();
}

// parse and execute one statement on a cursor of its own, for the setup the
// three arms below run against
static bool execImmediate(oracleprotocolclient *client, uint32_t cursorid,
						const char *query, bool check) {

	if (!osql7(client,1,cursorid,query)) {
		return false;
	}
	if (!query2(client,2,ORA_OPTION_EXECUTE|ORA_OPTION_NOPLSQL,cursorid)) {
		return false;
	}
	if (check && client->getResponseTtcCode()==ORA_TTC_ERROR) {
		hexDump("failed statement's response",
				client->getResponse(),
				client->getResponseSize());
		return false;
	}
	return true;
}


// ---- the model of a real server's answer ----

// a count, in whichever encoding is being modelled - the same two shapes
// putAuthCount() writes, and written out here rather than borrowed from
// oracleprotocolclient so the expected bytes come from the captures and not
// from any code the module and the client have in common
static void appendCount(bytebuffer *out, bool native,
				uint32_t value, size_t nativesize) {

	if (!native) {
		if (!value) {
			out->append((unsigned char)0);
		} else if (value<=0xff) {
			out->append((unsigned char)1);
			out->append((unsigned char)value);
		} else if (value<=0xffff) {
			out->append((unsigned char)2);
			out->append((unsigned char)((value>>8)&0xff));
			out->append((unsigned char)(value&0xff));
		} else {
			out->append((unsigned char)4);
			out->append((unsigned char)((value>>24)&0xff));
			out->append((unsigned char)((value>>16)&0xff));
			out->append((unsigned char)((value>>8)&0xff));
			out->append((unsigned char)(value&0xff));
		}
		return;
	}

	for (size_t i=0; i<nativesize; i++) {
		out->append((unsigned char)((i<sizeof(uint32_t))?
					((value>>(8*i))&0xff):0));
	}
}

// one column's metadata block - the two bytes only the native encoding
// carries are a leading flag and one between the character-set group and the
// character length, both constant across every capture on file
static void appendDescribeColumn(bytebuffer *out, bool native,
					const describecolumn *col,
					uint32_t charset) {

	if (native) {
		out->append((unsigned char)1);
	}

	out->append(col->type);
	out->append((unsigned char)((col->character)?0x80:0x00));
	out->append(col->precision);
	out->append((unsigned char)col->scale);

	appendCount(out,native,col->size,4);

	// four counts that are zero in every capture, meaning unknown
	appendCount(out,native,0,4);
	appendCount(out,native,0,4);
	appendCount(out,native,0,4);
	appendCount(out,native,0,4);

	appendCount(out,native,(col->character)?charset:0,2);
	out->append((unsigned char)((col->character)?1:0));

	if (native) {
		out->append((unsigned char)0);
	}

	appendCount(out,native,(col->character)?col->size:0,4);
	out->append(col->nullok);
	out->append((unsigned char)charstring::getLength(col->name));

	// three more zero counts, meaning unknown
	appendCount(out,native,0,4);
	appendCount(out,native,0,4);
	appendCount(out,native,0,4);
}

// the whole answer: the column count twice, one block per column, every column
// name in one blob behind them with a double quote after each, and the status
// message an oci7 call's answer ends with
static void buildDescribeResponse(bytebuffer *out, bool native,
					const describecolumn *cols,
					size_t colcount,
					uint32_t charset) {

	out->clear();

	appendCount(out,native,(uint32_t)colcount,2);
	appendCount(out,native,(uint32_t)colcount,2);

	for (size_t i=0; i<colcount; i++) {
		appendDescribeColumn(out,native,&(cols[i]),charset);
	}

	bytebuffer	names;
	for (size_t i=0; i<colcount; i++) {
		names.append(cols[i].name);
		names.append((unsigned char)'"');
	}
	size_t	namessize=names.getSize();

	appendCount(out,native,(uint32_t)namessize,2);
	out->append((unsigned char)namessize);
	out->append(names.getBuffer(),namessize);

	out->append(ORA_TTC_STATUS);
	appendCount(out,native,1,4);
}

// the ORA-01007 the model expects, which is the capture's own bytes with the
// session's cursor id and call number written over the capture's - and, in the
// native encoding, with the two fields the module answers differently from the
// capture written over too.  see the *_OFFSET constants above
static void buildOutOfRangeError(bytebuffer *out, bool native,
					uint32_t cursorid,
					unsigned char callnumber) {

	out->clear();

	if (native) {

		out->append(nativeoutofrange,sizeof(nativeoutofrange));

		unsigned char	*buffer=(unsigned char *)out->getBuffer();
		for (size_t i=0; i<4; i++) {
			buffer[NATIVE_CURSOR_ID_OFFSET+i]=
					(unsigned char)((cursorid>>(8*i))&0xff);
			buffer[NATIVE_SUCCESS_ITERATIONS_OFFSET+i]=0;
			buffer[NATIVE_LIVE_POINTER_OFFSET+i]=0;
		}
		buffer[NATIVE_CALL_NUMBER_OFFSET]=callnumber;
		return;
	}

	out->append(portableoutofrange,sizeof(portableoutofrange));

	unsigned char	*buffer=(unsigned char *)out->getBuffer();

	// a count prefixed cursor id is two bytes for any id from 1 to 255,
	// which is the only range this test ever opens into
	buffer[PORTABLE_CURSOR_ID_OFFSET]=1;
	buffer[PORTABLE_CURSOR_ID_OFFSET+1]=(unsigned char)cursorid;
	buffer[PORTABLE_CALL_NUMBER_OFFSET]=callnumber;
}


// ---- the three arms ----

// a cursor that is parsed and nothing else, described once - what a real
// server answers here is byte-identical to what it answers mid-fetch
static bool runParseArm(oracleprotocolclient *client, bytebuffer *describe,
					bytebuffer *outofrange,
					uint32_t *cursoridout) {

	uint32_t	cursorid=0;
	if (!client->open(&cursorid)) {
		report("parse arm: open cursor",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	*cursoridout=cursorid;

	stdoutput.printf("  -> TTI_OSQL7 seq 1 cursor %d\n",(int)cursorid);
	if (!osql7(client,1,cursorid,selectquery)) {
		report("parse arm: parse the select",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("parse response",client);

	stdoutput.printf("  -> TTI_DESCRIBE seq %d cursor %d position 1\n",
					(int)DESCRIBE_SEQUENCE_NUMBER,
					(int)cursorid);
	if (!client->describe(cursorid,1)) {
		report("parse arm: describe",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("describe response",client);
	report("parse arm: describe answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);
	saveResponse(describe,client);

	stdoutput.printf("  -> TTI_DESCRIBE seq %d cursor %d position %d\n",
					(int)DESCRIBE_SEQUENCE_NUMBER,
					(int)cursorid,
					(int)tablecolumncount+1);
	if (!client->describe(cursorid,(uint32_t)tablecolumncount+1)) {
		report("parse arm: describe past the last column",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("out of range describe response",client);
	report("parse arm: describe past the last column answers TTC_ERROR",
			client->getResponseTtcCode()==ORA_TTC_ERROR);
	saveResponse(outofrange,client);

	return true;
}

// the oci7.cpp sequence in full: execute, one fetch, three describes, three
// more fetches
static bool runMidFetchArm(oracleprotocolclient *client,
					bytebuffer *describe,
					bytebuffer *outofrange,
					bytebuffer *seconddescribe,
					bytebuffer *fetch2,
					bytebuffer *fetch3,
					bytebuffer *fetch4,
					uint32_t *cursoridout) {

	uint32_t	cursorid=0;
	if (!client->open(&cursorid)) {
		report("midfetch arm: open cursor",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	*cursoridout=cursorid;

	stdoutput.printf("  -> TTI_OSQL7 seq 1 cursor %d\n",(int)cursorid);
	if (!osql7(client,1,cursorid,selectquery)) {
		report("midfetch arm: parse the select",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("parse response",client);

	stdoutput.printf("  -> TTI_QUERY2 seq 2 options 0x%08x cursor %d\n",
					ORA_QUERY2_EXEC_OPTIONS,(int)cursorid);
	if (!query2(client,2,ORA_QUERY2_EXEC_OPTIONS,cursorid)) {
		report("midfetch arm: execute",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("execute response",client);
	report("midfetch arm: execute answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);

	stdoutput.printf("  -> TTI_FETCH seq 3 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,3,cursorid,1)) {
		report("midfetch arm: first fetch",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("first fetch response",client);
	report("midfetch arm: first fetch carries row 1",
					client->responseContains("char1"));

	stdoutput.printf("  -> TTI_DESCRIBE seq %d cursor %d position 1\n",
					(int)DESCRIBE_SEQUENCE_NUMBER,
					(int)cursorid);
	if (!client->describe(cursorid,1)) {
		report("midfetch arm: describe",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("mid-fetch describe response",client);
	report("midfetch arm: describe answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);
	saveResponse(describe,client);

	// the same describe for every other position in the select list.  a
	// real client caches the whole list off the first one and never asks
	// again, so only the first of these is a call any capture on file
	// carries - the rest are here because the answer has to be the same
	// whichever column an odescr names, TESTDATE included
	for (size_t i=2; i<=tablecolumncount; i++) {

		stdoutput.printf("  -> TTI_DESCRIBE seq %d cursor %d "
					"position %d\n",
					(int)DESCRIBE_SEQUENCE_NUMBER,
					(int)cursorid,(int)i);
		if (!client->describe(cursorid,(uint32_t)i)) {
			report("midfetch arm: describe another column",false);
			stdoutput.printf("%s\n",client->getError());
			return false;
		}
		dumpResponse("mid-fetch describe response",client);
		report("midfetch arm: every position answers the same",
			compareBytes("describe of a later column",
				client->getResponse(),
				client->getResponseSize(),
				(const unsigned char *)describe->getBuffer(),
				describe->getSize()));
	}

	stdoutput.printf("  -> TTI_DESCRIBE seq %d cursor %d position %d\n",
					(int)DESCRIBE_SEQUENCE_NUMBER,
					(int)cursorid,
					(int)tablecolumncount+1);
	if (!client->describe(cursorid,(uint32_t)tablecolumncount+1)) {
		report("midfetch arm: describe past the last column",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("out of range describe response",client);
	report("midfetch arm: describe past the last column answers TTC_ERROR",
			client->getResponseTtcCode()==ORA_TTC_ERROR);
	report("midfetch arm: out of range answer says ORA-01007",
			client->responseContains("ORA-01007"));
	saveResponse(outofrange,client);

	stdoutput.printf("  -> TTI_DESCRIBE seq %d cursor %d position 1\n",
					(int)DESCRIBE_SEQUENCE_NUMBER,
					(int)cursorid);
	if (!client->describe(cursorid,1)) {
		report("midfetch arm: describe again after the error",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("second mid-fetch describe response",client);
	report("midfetch arm: describe again answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);
	saveResponse(seconddescribe,client);

	// the three fetches the client makes after the describes, which is
	// where a real oci7 client dies
	stdoutput.printf("  -> TTI_FETCH seq 4 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,4,cursorid,1)) {
		report("midfetch arm: second fetch",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("second fetch response",client);
	report("midfetch arm: second fetch carries row 2",
					client->responseContains("char2"));
	report("midfetch arm: second fetch is not row 1 again",
					!client->responseContains("char1"));
	saveResponse(fetch2,client);

	stdoutput.printf("  -> TTI_FETCH seq 5 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,5,cursorid,1)) {
		report("midfetch arm: third fetch",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("third fetch response",client);
	saveResponse(fetch3,client);

	stdoutput.printf("  -> TTI_FETCH seq 6 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,6,cursorid,1)) {
		report("midfetch arm: fourth fetch",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("fourth fetch response",client);
	saveResponse(fetch4,client);

	return true;
}

// the same execute and fetches with no describe between them
static bool runControlArm(oracleprotocolclient *client,
					bytebuffer *fetch2,
					bytebuffer *fetch3,
					bytebuffer *fetch4,
					uint32_t *cursoridout) {

	uint32_t	cursorid=0;
	if (!client->open(&cursorid)) {
		report("control arm: open cursor",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	*cursoridout=cursorid;

	stdoutput.printf("  -> TTI_OSQL7 seq 1 cursor %d\n",(int)cursorid);
	if (!osql7(client,1,cursorid,selectquery)) {
		report("control arm: parse the select",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	stdoutput.printf("  -> TTI_QUERY2 seq 2 options 0x%08x cursor %d\n",
					ORA_QUERY2_EXEC_OPTIONS,(int)cursorid);
	if (!query2(client,2,ORA_QUERY2_EXEC_OPTIONS,cursorid)) {
		report("control arm: execute",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	stdoutput.printf("  -> TTI_FETCH seq 3 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,3,cursorid,1)) {
		report("control arm: first fetch",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("first fetch response",client);

	// the sequence numbers are the mid-fetch arm's, not 4, 5 and 6 - the
	// summary object echoes them back, so the two arms' answers can only
	// be compared byte for byte if the numbers agree
	unsigned char	sequences[3]={4,5,6};
	bytebuffer	*saved[3];
	saved[0]=fetch2;
	saved[1]=fetch3;
	saved[2]=fetch4;
	const char	*labels[3]={
		"second fetch response",
		"third fetch response",
		"fourth fetch response"
	};
	for (size_t i=0; i<3; i++) {
		stdoutput.printf("  -> TTI_FETCH seq %d cursor %d rows 1\n",
					(int)sequences[i],(int)cursorid);
		if (!legacyRowFetch(client,sequences[i],cursorid,1)) {
			report("control arm: fetch",false);
			stdoutput.printf("%s\n",client->getError());
			return false;
		}
		dumpResponse(labels[i],client);
		saveResponse(saved[i],client);
	}

	return true;
}


// ---- the predescribe arms ----

// #9973 - the describe one call earlier than the midfetch arm's: after the
// TTI_QUERY2 that executes and before the first TTI_FETCH.  an oci7 client
// that calls odescr() there has fetched nothing yet, so a describe that
// re-runs the statement leaves the client reading a result set that silently
// started over.  the arm asks a sequence for its first value, which pins how
// many times the statement ran: one execute answers the value the sequence
// was created with, a second answers the one behind it
static bool runPreDescribeArm(oracleprotocolclient *client,
						uint32_t cursorid) {

	stdoutput.printf("  -> TTI_OSQL7 seq 1 cursor %d\n",(int)cursorid);
	if (!osql7(client,1,cursorid,sequencequery)) {
		report("predescribe arm: parse the select",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	stdoutput.printf("  -> TTI_QUERY2 seq 2 options 0x%08x cursor %d\n",
					ORA_QUERY2_EXEC_OPTIONS,(int)cursorid);
	if (!query2(client,2,ORA_QUERY2_EXEC_OPTIONS,cursorid)) {
		report("predescribe arm: execute",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("execute response",client);
	report("predescribe arm: execute answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);

	stdoutput.printf("  -> TTI_DESCRIBE seq %d cursor %d position 1\n",
					(int)DESCRIBE_SEQUENCE_NUMBER,
					(int)cursorid);
	if (!client->describe(cursorid,1)) {
		report("predescribe arm: describe",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("pre-fetch describe response",client);
	report("predescribe arm: describe answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);

	stdoutput.printf("  -> TTI_FETCH seq 3 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,3,cursorid,1)) {
		report("predescribe arm: fetch",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("fetch response",client);
	report("predescribe arm: the fetch carries the sequence's first value",
				client->responseContains(firstnextval));
	report("predescribe arm: the describe didn't re-execute the statement",
				!client->responseContains(secondnextval));

	return true;
}

// the same parse, execute and fetch with no describe between them.  it says
// the sequence advances by exactly one value per execute, which is what makes
// the arm above's answer a count of executes rather than just a value
static bool runPreDescribeControlArm(oracleprotocolclient *client,
							uint32_t cursorid) {

	stdoutput.printf("  -> TTI_OSQL7 seq 1 cursor %d\n",(int)cursorid);
	if (!osql7(client,1,cursorid,sequencequery)) {
		report("predescribe control arm: parse the select",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	stdoutput.printf("  -> TTI_QUERY2 seq 2 options 0x%08x cursor %d\n",
					ORA_QUERY2_EXEC_OPTIONS,(int)cursorid);
	if (!query2(client,2,ORA_QUERY2_EXEC_OPTIONS,cursorid)) {
		report("predescribe control arm: execute",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	stdoutput.printf("  -> TTI_FETCH seq 3 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,3,cursorid,1)) {
		report("predescribe control arm: fetch",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("fetch response",client);
	report("predescribe control arm: the fetch carries the value behind "
			"the arm above's",
			client->responseContains(secondnextval));

	return true;
}


// ---- the define arms ----

static const size_t	MAX_FETCH_COLUMNS=16;
static const size_t	MAX_FETCH_COLUMN_SIZE=256;

// a fetch response picked apart: the column count its row header carries, the
// values behind it, and whether the row ended where it should have
struct fetchedrow {
	uint32_t	headercolcount;
	size_t		valuecount;
	bool		isnull[MAX_FETCH_COLUMNS];
	unsigned char	value[MAX_FETCH_COLUMNS][MAX_FETCH_COLUMN_SIZE];
	size_t		valuesize[MAX_FETCH_COLUMNS];
	bool		endsonsummary;
};

// a portable fetch response is the two data flag bytes, TTC_ROW_HEADER, a
// flags byte, six counts of which the first is the column count, a 0x07 row
// marker, and then one column at a time: the value as a clr, and behind it
// the two zero counts putRow() writes for the indicator and the return code.
// the summary object follows the last of them
static bool parseFetchResponse(oracleprotocolclient *client,
						fetchedrow *row) {

	row->headercolcount=0;
	row->valuecount=0;
	row->endsonsummary=false;

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	unsigned char	flags=0;
	uint32_t	unused=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		!client->readByte(&flags) ||
		!client->readLenPreInt(&(row->headercolcount)) ||
		!client->readLenPreInt(&unused) ||
		!client->readLenPreInt(&unused) ||
		!client->readLenPreInt(&unused) ||
		!client->readLenPreInt(&unused) ||
		!client->readLenPreInt(&unused)) {
		stdoutput.printf("truncated row header\n");
		return false;
	}
	if (ttccode!=ORA_TTC_ROW_HEADER) {
		stdoutput.printf("ttc code 0x%02x, expected a row header "
					"(0x%02x)\n",
					(int)ttccode,(int)ORA_TTC_ROW_HEADER);
		return false;
	}

	unsigned char	rowmarker=0;
	if (!client->readByte(&rowmarker) || rowmarker!=ORA_TTC_ROW_DATA) {
		stdoutput.printf("no 0x%02x row marker behind the row header\n",
						(int)ORA_TTC_ROW_DATA);
		return false;
	}

	if (row->headercolcount>MAX_FETCH_COLUMNS) {
		stdoutput.printf("row header says %d columns, more than this "
					"test reads\n",(int)row->headercolcount);
		return false;
	}

	for (uint32_t i=0; i<row->headercolcount; i++) {
		if (!client->readLenBytes(row->value[i],
						MAX_FETCH_COLUMN_SIZE,
						&(row->valuesize[i]),
						&(row->isnull[i])) ||
			!client->readLenPreInt(&unused) ||
			!client->readLenPreInt(&unused)) {
			stdoutput.printf("truncated column %d\n",(int)i+1);
			return false;
		}
		row->valuecount++;
	}

	// anything a column the client never defined added to the row lands
	// here, so a row that ends anywhere but on the summary object carries
	// bytes the client has no buffer for
	unsigned char	next=0;
	row->endsonsummary=(client->readByte(&next) && next==ORA_TTC_ERROR);
	if (!row->endsonsummary) {
		stdoutput.printf("the row is followed by 0x%02x, expected the "
					"summary object (0x%02x)\n",
					(int)next,(int)ORA_TTC_ERROR);
	}

	return true;
}

static void printColumnValue(const unsigned char *value, size_t size) {
	stdoutput.printf("\"");
	for (size_t i=0; i<size; i++) {
		if (value[i]>=0x20 && value[i]<0x7f) {
			stdoutput.printf("%c",value[i]);
		} else {
			stdoutput.printf("\\x%02x",value[i]);
		}
	}
	stdoutput.printf("\"");
}

// which positions a set names, for the messages below
static void printPositions(const bool *set, size_t colcount) {
	bool	any=false;
	for (size_t i=0; i<colcount; i++) {
		if (!set[i]) {
			continue;
		}
		stdoutput.printf("%s%d",(any)?",":"",(int)i+1);
		any=true;
	}
	if (!any) {
		stdoutput.printf("none");
	}
}

// the values in the row against the ones the columns in "expected" hold in
// "values" - row 1 of the table for every arm except "all four positions",
// which gets the date column back in a different shape (see row1alldefined
// above) - in order and with nothing else between them
static bool checkFetchedColumns(const fetchedrow *row,
					const bool *expected,
					size_t colcount,
					const columnvalue *values) {

	size_t	next=0;
	for (size_t i=0; i<colcount; i++) {

		if (!expected[i]) {
			continue;
		}

		if (next>=row->valuecount) {
			stdoutput.printf("column %d is missing - the row "
					"carries %d values\n",
					(int)i+1,(int)row->valuecount);
			return false;
		}

		if (row->isnull[next] ||
			row->valuesize[next]!=values[i].size ||
			bytestring::compare(row->value[next],
						values[i].value,
						values[i].size)) {
			stdoutput.printf("value %d is ",(int)next+1);
			if (row->isnull[next]) {
				stdoutput.printf("null");
			} else {
				printColumnValue(row->value[next],
							row->valuesize[next]);
			}
			stdoutput.printf(", expected column %d's ",(int)i+1);
			printColumnValue(values[i].value,values[i].size);
			stdoutput.printf("\n");
			return false;
		}

		next++;
	}

	if (next!=row->valuecount) {
		stdoutput.printf("the row carries %d values, expected %d\n",
					(int)row->valuecount,(int)next);
		return false;
	}

	return true;
}

// one define arm: parse the select, execute it with a define block naming
// "defined", fetch one row, and check it against "expected", whose columns
// should hold "values".  "defined" and "expected" are the same set for a
// block the module can walk; they part company for the one it has to
// reject, which falls back to every column.  "values" is row1 for every
// arm except "all four positions", which asks every column converted to
// a type that changes the date column's shape - see row1alldefined
static bool runDefineArm(oracleprotocolclient *client,
					uint32_t cursorid,
					const char *label,
					const bool *defined,
					const bool *expected,
					size_t colcount,
					size_t junk,
					const columnvalue *values) {

	stdoutput.printf("\n--- %s ---\n\n",label);

	char	message[256];

	stdoutput.printf("  -> TTI_OSQL7 seq 1 cursor %d\n",(int)cursorid);
	if (!osql7(client,1,cursorid,selectquery)) {
		charstring::printf(message,sizeof(message),
					"%s: parse the select",label);
		report(message,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	stdoutput.printf("  -> TTI_QUERY2 seq 2 options 0x%08x cursor %d "
				"defines ",ORA_QUERY2_EXEC_OPTIONS,
				(int)cursorid);
	printPositions(defined,colcount);
	stdoutput.printf(" of %d",(int)colcount);
	if (junk) {
		stdoutput.printf(" plus %d trailing bytes",(int)junk);
	}
	stdoutput.printf("\n");

	if (!query2WithDefines(client,2,ORA_QUERY2_EXEC_OPTIONS,cursorid,
						defined,colcount,junk)) {
		charstring::printf(message,sizeof(message),
					"%s: execute",label);
		report(message,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("execute response",client);
	charstring::printf(message,sizeof(message),
				"%s: execute answers TTC_OK",label);
	report(message,client->getResponseTtcCode()==ORA_TTC_OK);

	stdoutput.printf("  -> TTI_FETCH seq 3 cursor %d rows 1\n",
							(int)cursorid);
	if (!legacyRowFetch(client,3,cursorid,1)) {
		charstring::printf(message,sizeof(message),
					"%s: fetch",label);
		report(message,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	dumpResponse("fetch response",client);

	fetchedrow	row;
	if (!parseFetchResponse(client,&row)) {
		charstring::printf(message,sizeof(message),
				"%s: the fetch response parses as a row",label);
		report(message,false);
		return false;
	}

	uint32_t	expectedcount=(uint32_t)definedCount(expected,colcount);
	bool		countok=(row.headercolcount==expectedcount);
	if (!countok) {
		stdoutput.printf("the row header says %d columns, expected "
					"%d - positions ",
					(int)row.headercolcount,
					(int)expectedcount);
		printPositions(expected,colcount);
		stdoutput.printf("\n");
	}
	charstring::printf(message,sizeof(message),
			"%s: the row header counts the defined columns",label);
	report(message,countok);

	charstring::printf(message,sizeof(message),
			"%s: the row carries the defined columns and no others",
			label);
	report(message,checkFetchedColumns(&row,expected,colcount,values));

	charstring::printf(message,sizeof(message),
			"%s: the columns left out add nothing to the row",
			label);
	report(message,row.endsonsummary);

	return true;
}


// the setup cursor comes back so the predescribe arms can re-parse on it
// rather than open one of their own - the instance hands out five cursors at
// a time, and the arms below it have the other four
static bool setUpTable(oracleprotocolclient *client, uint32_t *cursoridout) {

	uint32_t	cursorid=0;
	if (!client->open(&cursorid)) {
		report("open setup cursor",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	*cursoridout=cursorid;

	// a drop of a table or sequence that isn't there answers with an error
	// and leaves the session running, so their results are deliberately
	// not checked
	execImmediate(client,cursorid,droptable,false);
	execImmediate(client,cursorid,dropsequence,false);

	if (!execImmediate(client,cursorid,createtable,true)) {
		report("create table",false);
		return false;
	}
	report("create table",true);

	// dropped and recreated rather than reset, so the first nextval the
	// predescribe arm asks for is the start value whatever a previous run
	// left behind
	if (!execImmediate(client,cursorid,createsequence,true)) {
		report("create sequence",false);
		return false;
	}
	report("create sequence",true);

	if (!execImmediate(client,cursorid,insertrow1,true) ||
		!execImmediate(client,cursorid,insertrow2,true) ||
		!execImmediate(client,cursorid,insertrow3,true) ||
		!execImmediate(client,cursorid,"commit",true)) {
		report("insert rows",false);
		return false;
	}
	report("insert rows",true);

	return true;
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #9810 a describe on a mid-fetch cursor "
							"======\n\n");

	bool		native=false;
	uint32_t	charset=ORA_CHARSET_DEFAULT;
	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"-native")) {
			native=true;
		} else if (!charstring::compare(argv[i],"-charset=",9)) {
			charset=(uint32_t)charstring::convertToInteger(
								argv[i]+9);
		}
	}

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  ORACLEPROTOCOLPORT1
	// names the port it ended up on, the same variable that config is
	// generated from; unset means the configure-time default
	const char	*host="127.0.0.1";
	uint16_t	port=1521;
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	const char	*portoverride=
			environment::getValue("ORACLEPROTOCOLPORT1");
	if (!charstring::isNullOrEmpty(portoverride)) {
		port=(uint16_t)charstring::convertToInteger(portoverride);
	}

	stdoutput.printf("--- the module, %s encoding, charset %d ---\n\n",
				(native)?"native":"portable",(int)charset);

	oracleprotocolclient	client;
	client.setNativeEncoding(native);

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

	stdoutput.printf("\n--- setup ---\n\n");
	uint32_t	setupcursorid=0;
	if (!setUpTable(&client,&setupcursorid)) {
		client.disconnect();
		return status;
	}

	// both run on the setup cursor, and the control one runs second so the
	// value it expects is the one behind the value the arm above it asked
	// for
	stdoutput.printf("\n--- predescribe arm ---\n\n");
	if (!runPreDescribeArm(&client,setupcursorid)) {
		client.disconnect();
		return status;
	}

	stdoutput.printf("\n--- predescribe control arm ---\n\n");
	if (!runPreDescribeControlArm(&client,setupcursorid)) {
		client.disconnect();
		return status;
	}

	stdoutput.printf("\n--- parse arm ---\n\n");
	bytebuffer	parsedescribe;
	bytebuffer	parseoutofrange;
	uint32_t	parsecursorid=0;
	if (!runParseArm(&client,&parsedescribe,&parseoutofrange,
							&parsecursorid)) {
		client.disconnect();
		return status;
	}

	stdoutput.printf("\n--- midfetch arm ---\n\n");
	bytebuffer	midfetchdescribe;
	bytebuffer	midfetchoutofrange;
	bytebuffer	midfetchseconddescribe;
	bytebuffer	midfetchfetch2;
	bytebuffer	midfetchfetch3;
	bytebuffer	midfetchfetch4;
	uint32_t	midfetchcursorid=0;
	if (!runMidFetchArm(&client,&midfetchdescribe,&midfetchoutofrange,
					&midfetchseconddescribe,
					&midfetchfetch2,&midfetchfetch3,
					&midfetchfetch4,&midfetchcursorid)) {
		client.disconnect();
		return status;
	}

	stdoutput.printf("\n--- control arm ---\n\n");
	bytebuffer	controlfetch2;
	bytebuffer	controlfetch3;
	bytebuffer	controlfetch4;
	uint32_t	controlcursorid=0;
	if (!runControlArm(&client,&controlfetch2,&controlfetch3,
					&controlfetch4,&controlcursorid)) {
		client.disconnect();
		return status;
	}

	stdoutput.printf("\n--- comparisons ---\n\n");

	report("mid-fetch describe matches the parsed cursor's describe",
		compareBytes("mid-fetch describe",
			(const unsigned char *)midfetchdescribe.getBuffer(),
			midfetchdescribe.getSize(),
			(const unsigned char *)parsedescribe.getBuffer(),
			parsedescribe.getSize()));

	report("the second mid-fetch describe matches the first",
		compareBytes("second mid-fetch describe",
			(const unsigned char *)
					midfetchseconddescribe.getBuffer(),
			midfetchseconddescribe.getSize(),
			(const unsigned char *)midfetchdescribe.getBuffer(),
			midfetchdescribe.getSize()));

	bytebuffer	expected;
	buildDescribeResponse(&expected,native,tablecolumns,
					tablecolumncount,charset);
	report("mid-fetch describe matches a real server's model",
		compareBytes("mid-fetch describe against the model",
			(const unsigned char *)midfetchdescribe.getBuffer()+3,
			midfetchdescribe.getSize()-3,
			(const unsigned char *)expected.getBuffer(),
			expected.getSize()));

	report("the mid-fetch ORA-01007 matches the parsed cursor's",
		compareApartFromCursorId("out of range answer",
			(const unsigned char *)midfetchoutofrange.getBuffer(),
			midfetchoutofrange.getSize(),
			(unsigned char)midfetchcursorid,
			(const unsigned char *)parseoutofrange.getBuffer(),
			parseoutofrange.getSize(),
			(unsigned char)parsecursorid));

	buildOutOfRangeError(&expected,native,midfetchcursorid,
					DESCRIBE_SEQUENCE_NUMBER);
	report("the mid-fetch ORA-01007 matches a real server's model",
		compareBytes("out of range answer against the model",
			(const unsigned char *)
					midfetchoutofrange.getBuffer()+3,
			midfetchoutofrange.getSize()-3,
			(const unsigned char *)expected.getBuffer(),
			expected.getSize()));

	report("the fetch after the describes matches the control's",
		compareApartFromCursorId("second fetch",
			(const unsigned char *)midfetchfetch2.getBuffer(),
			midfetchfetch2.getSize(),
			(unsigned char)midfetchcursorid,
			(const unsigned char *)controlfetch2.getBuffer(),
			controlfetch2.getSize(),
			(unsigned char)controlcursorid));

	report("the third fetch matches the control's",
		compareApartFromCursorId("third fetch",
			(const unsigned char *)midfetchfetch3.getBuffer(),
			midfetchfetch3.getSize(),
			(unsigned char)midfetchcursorid,
			(const unsigned char *)controlfetch3.getBuffer(),
			controlfetch3.getSize(),
			(unsigned char)controlcursorid));

	report("the fourth fetch matches the control's",
		compareApartFromCursorId("fourth fetch",
			(const unsigned char *)midfetchfetch4.getBuffer(),
			midfetchfetch4.getSize(),
			(unsigned char)midfetchcursorid,
			(const unsigned char *)controlfetch4.getBuffer(),
			controlfetch4.getSize(),
			(unsigned char)controlcursorid));

	// the module decodes a define list in the portable encoding only: a
	// native descriptor carries an extra leading byte and counts that
	// aren't all one width, and no capture pins that tail well enough to
	// walk, so getQuery2Defines() in src/protocols/oracle.cpp leaves the
	// list empty there and falls back to sending every column.  that costs
	// nothing - the module answers a platform banner no real client
	// matches, so no real session is ever in the native encoding - and it
	// leaves these arms nothing to assert on a native run
	if (native) {

		stdoutput.printf("\n--- define arms ---\n\n");
		stdoutput.printf("skipped - the module decodes a define list "
					"in the portable encoding only\n");

	} else {

		// every define arm re-parses the select on one cursor rather
		// than opening its own: the instance hands out five at a time
		// and the three arms above have four of them.  a re-parse
		// drops the client's defines, so each arm starts from none
		uint32_t	definecursorid=0;
		if (!client.open(&definecursorid)) {
			report("define arms: open cursor",false);
			stdoutput.printf("%s\n",client.getError());
			client.disconnect();
			return status;
		}

		const bool	allfour[]={ true, true, true, true };
		const bool	firstonly[]={ true, false, false, false };
		const bool	thirdonly[]={ false, false, true, false };
		const bool	firsttwo[]={ true, true, false, false };

		// defining every position asks for what the fallback sends
		// anyway, so this is the arm that says the walk didn't break
		// the case every session before it got.  it also defines the
		// date column, so its expected row is row1alldefined, not
		// row1 - see the comment there
		runDefineArm(&client,definecursorid,
				"define arm: all four positions",
				allfour,allfour,tablecolumncount,0,
				row1alldefined);

		runDefineArm(&client,definecursorid,
				"define arm: position 1 only",
				firstonly,firstonly,tablecolumncount,0,
				row1);

		// a define count of 3 - two placeholders and then the real
		// one, which is what packet [0027] of samples/
		// 9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy
		// sends, and what a real 10.2 server answers with one column
		runDefineArm(&client,definecursorid,
				"define arm: position 3 only",
				thirdonly,thirdonly,tablecolumncount,0,
				row1);

		runDefineArm(&client,definecursorid,
				"define arm: positions 1 and 2",
				firsttwo,firsttwo,tablecolumncount,0,
				row1);

		// a block whose descriptors don't land on the end of the
		// request.  the walk has to throw away what it read and send
		// every column - shaping a row from a bad read would desync
		// the client's parse of every row after it, which is a worse
		// failure than the overrun being fixed.  the defines it threw
		// away are what would have converted the date column, so the
		// fallback answers it in its natural, row1 form
		runDefineArm(&client,definecursorid,
				"define arm: a block the walk has to reject",
				firstonly,allfour,tablecolumncount,3,
				row1);
	}

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
