// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/bytebuffer.h>
#include <rudiments/bytestring.h>
#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for ticket #9808: describe(), sendDescribeResponse() and
// putOci7DescribeColumn() in src/protocols/oracle.cpp, which answer the
// TTI_DESCRIBE (0x2B) an oci7 client's odescr() sends.
//
// The oci7 test in this directory can't reach them here.  oci7describe.cpp
// only builds where the legacy OCI7 call interface was found at configure
// time - headers that live in a full Oracle Client install and that Instant
// Client doesn't ship - so on a host with only Instant Client there is no
// client that speaks that call at all.  This drives the wire protocol
// directly instead, the same way oracledescribeonly.cpp does: a hand-built
// login, a parse, and then a hand-built TTI_DESCRIBE.
//
// What it asserts is the answer's bytes, against the real 10.2 server's own,
// in two steps:
//
//	- buildDescribeResponse() below, given the metadata the three
//	  columns of "select testnumber, testchar, testdate from
//	  protocoltesttypes" really have, has to reproduce packet [0028] of
//	  samples/9808-redhat9x86-native-realtable-parse.oraproxy and of its
//	  solaris8sparc portable counterpart, byte for byte.  that is what
//	  makes it a model of the real server rather than a restatement of
//	  the module
//	- and the module, asked the same question about a table with the
//	  same three columns, has to answer with what that model builds
//
// The listener's charset is the one field the two steps don't share.  A
// real 10.2 server answered these captures as charset 31, which is what a
// verifiertype="9i" listener answers with too, but the oracleprotocol
// instance this test logs into is on the module's AL32UTF8 default - so the
// charset is the model's parameter, and only its value differs between the
// capture and the live answer.
//
// The ORA-01007 path gets the same treatment against the -outofrange
// captures, where the only fields that can't match are the ones that are
// session data: the cursor id and the call number, plus, in the native
// encoding, the two the module documents itself as answering differently.

static const unsigned char	ORA_TTC_STATUS=0x09;

// the module's default charset, and the one a real 10.2 server sent - see
// the charset parameter in src/protocols/oracle.cpp, which answers 31 for a
// verifiertype="9i" listener and AL32UTF8 for every other one
static const uint32_t	ORA_CHARSET_CAPTURE=31;
static const uint32_t	ORA_CHARSET_DEFAULT=873;

// the table the -realtable captures describe, rebuilt under a name of this
// test's own so a concurrent oci7/oci8 run's protocoltesttypes is left
// alone.  the column names are the captured ones, since their lengths and
// their bytes both go out in the answer
static const char	*createtable=
	"create table protocoltest9808describe ("
		"testnumber number(10,2),"
		"testchar char(20),"
		"testdate date)";
static const char	*droptable=
	"drop table protocoltest9808describe";
static const char	*selectquery=
	"select testnumber, testchar, testdate "
	"from protocoltest9808describe";

// packet [0028] of samples/9808-redhat9x86-native-realtable-parse.oraproxy -
// a real 10.2 server's answer to odescr() on that select, native encoding
static const unsigned char	nativedescribe[]={
	0x03, 0x00, 0x03, 0x00, 0x01, 0x02, 0x00, 0x0a,
	0x02, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x60, 0x80, 0x00, 0x00,
	0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x01, 0x00,
	0x14, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x0c, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1d, 0x00, 0x1d, 0x54, 0x45, 0x53, 0x54,
	0x4e, 0x55, 0x4d, 0x42, 0x45, 0x52, 0x22, 0x54,
	0x45, 0x53, 0x54, 0x43, 0x48, 0x41, 0x52, 0x22,
	0x54, 0x45, 0x53, 0x54, 0x44, 0x41, 0x54, 0x45,
	0x22, 0x09, 0x01, 0x00, 0x00, 0x00
};

// packet [0022] of
// samples/9808-solaris8sparc-portable-realtable-parse.oraproxy - the same
// answer to the same select, portable encoding
static const unsigned char	portabledescribe[]={
	0x01, 0x03, 0x01, 0x03, 0x02, 0x00, 0x0a, 0x02,
	0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x60, 0x80,
	0x00, 0x00, 0x01, 0x14, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x1f, 0x01, 0x01, 0x14, 0x01, 0x08, 0x00,
	0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x00, 0x00, 0x00, 0x01, 0x1d, 0x1d, 0x54,
	0x45, 0x53, 0x54, 0x4e, 0x55, 0x4d, 0x42, 0x45,
	0x52, 0x22, 0x54, 0x45, 0x53, 0x54, 0x43, 0x48,
	0x41, 0x52, 0x22, 0x54, 0x45, 0x53, 0x54, 0x44,
	0x41, 0x54, 0x45, 0x22, 0x09, 0x01, 0x01
};

// packet [0028] of
// samples/9808-redhat9x86-native-realtable-outofrange.oraproxy - the same
// server's answer to odescr() on column 4 of that three-column select
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

// packet [0022] of
// samples/9808-solaris8sparc-portable-realtable-outofrange.oraproxy
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

// where the summary object an ORA-01007 travels in keeps the four fields
// that are session data rather than protocol - worked out from
// putOci7Summary() and putOci7SummaryNative() in src/protocols/oracle.cpp
// and confirmed against the two captures above.  the cursor id and the call
// number legitimately differ per session; the last two are the native
// encoding's alone, and are the two fields the module documents itself as
// answering differently from the capture
static const size_t	NATIVE_CURSOR_ID_OFFSET=15;
static const size_t	NATIVE_CALL_NUMBER_OFFSET=46;
static const size_t	NATIVE_SUCCESS_ITERATIONS_OFFSET=49;
static const size_t	NATIVE_LIVE_POINTER_OFFSET=61;
static const size_t	PORTABLE_CURSOR_ID_OFFSET=8;
static const size_t	PORTABLE_CALL_NUMBER_OFFSET=24;

// what describe() sends as the sequence number, which the ORA-01007 echoes
// back in the call number field
static const unsigned char	DESCRIBE_SEQUENCE_NUMBER=10;

// one column, as much of it as the answer carries.  the fields are
// putOci7DescribeColumn()'s in its own order, minus the ones that are
// constant across every capture on file
struct describecolumn {
	unsigned char	type;		// oracle's internal datatype code
	bool		character;
	unsigned char	precision;
	signed char	scale;
	uint32_t	size;
	unsigned char	nullok;
	const char	*name;
};

// the three columns of the -realtable captures' select, as a real 10.2
// server described them.  TESTNUMBER is NUMBER(10,2), so its precision and
// scale are genuine column metadata rather than whatever oracle reports for
// a literal expression; TESTDATE's size is 1 because a describe reports a
// date that way whatever a date really takes
static const describecolumn	realtablecolumns[]={
	{ 2, false, 10, 2, 22, 1, "TESTNUMBER" },
	{ 96, true, 0, 0, 20, 1, "TESTCHAR" },
	{ 12, false, 0, 0, 1, 1, "TESTDATE" }
};
static const size_t	realtablecolumncount=
			sizeof(realtablecolumns)/sizeof(realtablecolumns[0]);

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


// ---- the model of a real server's answer ----

// a count, in whichever encoding is being modelled - the same two shapes
// putAuthCount() writes, and deliberately written out here rather than
// borrowed from oracleprotocolclient, so the expected bytes come from the
// captures and not from any code the module and the client have in common
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

// one column's metadata block - 47 bytes in the native encoding, 18 to 20 in
// the portable one.  the two bytes only the native encoding carries are a
// leading flag and one sitting between the character-set group and the
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

// the whole answer: the column count twice, one block per column, every
// column name in one blob behind them with a double quote after each, and
// the status message an oci7 call's answer ends with
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

	// the names, and their total size twice - once as a count and once as
	// the text's own length byte
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
// session's cursor id and call number written over the capture's - and, in
// the native encoding, with the two fields the module answers differently
// from the capture written over too.  see the *_OFFSET constants above for
// what each one is and why it can't just match
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

			// success iterations, which the two captures
			// disagree about - the native one sends 1 and the
			// portable one 0, and the module sends 0 in both
			buffer[NATIVE_SUCCESS_ITERATIONS_OFFSET+i]=0;

			// a live server-side value nothing reproduces, and
			// that the module zeroes rather than guesses
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


// ---- the checks that need no server ----

// the model against the two real captures.  this is what makes the rest of
// the test mean anything: if the model can't reproduce what a real 10.2
// server sent, then matching it proves nothing about the module
static void checkModelAgainstCaptures() {

	bytebuffer	expected;

	buildDescribeResponse(&expected,true,realtablecolumns,
				realtablecolumncount,ORA_CHARSET_CAPTURE);
	report("model reproduces the native realtable-parse capture",
		compareBytes("native describe model",
			(const unsigned char *)expected.getBuffer(),
			expected.getSize(),
			nativedescribe,sizeof(nativedescribe)));

	buildDescribeResponse(&expected,false,realtablecolumns,
				realtablecolumncount,ORA_CHARSET_CAPTURE);
	report("model reproduces the portable realtable-parse capture",
		compareBytes("portable describe model",
			(const unsigned char *)expected.getBuffer(),
			expected.getSize(),
			portabledescribe,sizeof(portabledescribe)));
}


// ---- the checks that need the listener ----

// the answer's own framing, said outright, so a failure names the field
// rather than only an offset
static void reportDescribeShape(oracleprotocolclient *client,
					size_t colcount) {

	size_t	responsesize=client->getResponseSize();

	// the data flags, the ttc code, the column count twice, and the
	// trailer: a count, a length byte, the names, TTC_STATUS and a count
	size_t	namessize=0;
	for (size_t i=0; i<colcount; i++) {
		namessize+=charstring::getLength(realtablecolumns[i].name)+1;
	}

	size_t	fixed=0;
	size_t	percolumn=0;
	if (client->getNativeEncoding()) {
		fixed=3+2+2+2+1+namessize+1+4;
		percolumn=47;
	} else {

		// a portable count is as wide as its value needs, so the
		// only per-column size worth naming is the one a column
		// with no multi-byte count in it takes
		fixed=3+2+2+2+1+namessize+1+2;
		percolumn=18;
	}

	stdoutput.printf("response: %d bytes, %d fixed, "
				"%d per column at the narrowest\n",
				(int)responsesize,(int)fixed,(int)percolumn);
}

static bool runDescribeChecks(oracleprotocolclient *client,
					uint32_t charset) {

	bool	native=client->getNativeEncoding();

	uint32_t	cursorid=0;
	if (!client->open(&cursorid)) {
		report("open cursor",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report("open cursor",true);

	// the table has to exist before anything can describe it.  a drop of
	// a table that isn't there answers with an error and leaves the
	// session running, so its result is deliberately not checked
	client->query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,cursorid,0,droptable);
	if (!client->query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,cursorid,0,createtable) ||
			client->getResponseTtcCode()==ORA_TTC_ERROR) {
		report("create table",false);
		hexDump("response",client->getResponse(),
					client->getResponseSize());
		return false;
	}
	report("create table",true);

	// a fresh cursor for the select, so the describe below runs against
	// one that has been parsed and nothing else - the shape the
	// -realtable-parse captures were taken in
	uint32_t	selectcursorid=0;
	if (!client->open(&selectcursorid)) {
		report("open select cursor",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	if (!client->query3(ORA_OPTION_PARSE|ORA_OPTION_NOPLSQL,
					selectcursorid,0,selectquery) ||
			client->getResponseTtcCode()==ORA_TTC_ERROR) {
		report("parse the select",false);
		hexDump("response",client->getResponse(),
					client->getResponseSize());
		return false;
	}
	report("parse the select",true);

	// the call this test exists for
	if (!client->describe(selectcursorid,1)) {
		report("describe column 1",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report("describe answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);

	reportDescribeShape(client,realtablecolumncount);

	bytebuffer	expected;
	buildDescribeResponse(&expected,native,realtablecolumns,
				realtablecolumncount,charset);
	report("describe response matches the capture's model",
		compareBytes("describe response",
			client->getResponse()+3,
			client->getResponseSize()-3,
			(const unsigned char *)expected.getBuffer(),
			expected.getSize()));

	// and a position past the end of the select list, which is the one
	// thing about the request that changes the answer
	if (!client->describe(selectcursorid,realtablecolumncount+1)) {
		report("describe past the last column",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report("describe past the last column answers TTC_ERROR",
			client->getResponseTtcCode()==ORA_TTC_ERROR);

	buildOutOfRangeError(&expected,native,selectcursorid,
					DESCRIBE_SEQUENCE_NUMBER);
	report("ORA-01007 matches the capture's model",
		compareBytes("out of range response",
			client->getResponse()+3,
			client->getResponseSize()-3,
			(const unsigned char *)expected.getBuffer(),
			expected.getSize()));

	// the session has to survive the error - a real client goes on
	// describing other columns after one comes back out of range
	if (!client->describe(selectcursorid,1)) {
		report("describe again after the error",false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report("describe again after the error answers TTC_OK",
			client->getResponseTtcCode()==ORA_TTC_OK);

	client->query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,cursorid,0,droptable);

	return true;
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #9808 TTI_DESCRIBE against the real "
						"10.2 captures ======\n\n");

	bool		native=false;
	uint32_t	charset=ORA_CHARSET_DEFAULT;
	bool		modelonly=false;
	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"-native")) {
			native=true;
		} else if (!charstring::compare(argv[i],"-modelonly")) {
			modelonly=true;
		} else if (!charstring::compare(argv[i],"-charset=",9)) {
			charset=(uint32_t)charstring::convertToInteger(
								argv[i]+9);
		}
	}

	stdoutput.printf("--- the model against the real captures ---\n\n");
	checkModelAgainstCaptures();

	if (modelonly) {
		return status;
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

	stdoutput.printf("\n--- the module, %s encoding, charset %d ---\n\n",
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

	runDescribeChecks(&client,charset);

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
