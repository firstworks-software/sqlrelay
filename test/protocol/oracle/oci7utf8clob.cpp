// Copyright (c) David Muse
// See the file COPYING for more information.

// #10268 - the OCI7 counterpart to oracleutf8clob.cpp (#10265).  that
// program covers sendLobReadResponse()'s fix (the query3/OCI8 LOB_OP_READ
// path); this one covers the same #10265 commit's fix to putLobField(),
// which is what a legacy OCI7 client's inline fetch of a clob column goes
// through instead.  Both bugs were the same mistake in two places:
// OCILobRead's amtp comes back in bytes rather than characters under a
// multibyte client character set, and putLobField() advanced its read
// offset by that byte count as if it were a character count.
//
// putLobField() only diverges from a correct byte-count advance once a clob
// is longer than one segment (8192 characters, src/protocols/oracle.cpp's
// lobbuffer/MAX_BYTES_PER_CHAR), so this reuses oracleutf8clob.cpp's own
// multibyte content (3000 repeats of 'a' + U+00E9 + U+20AC = 9000
// characters, 18000 bytes) unchanged - past the threshold, and small enough
// to fit in one OCI7 inline-fetch buffer.  It connects through the
// oracleprotocoloci7utf8 instance (test/sqlrelay.conf.d/
// oracleprotocol.conf.in), a verifiertype="9i" sibling of oracleprotocolutf8
// that reuses that instance's own multibyte backend, with a literal connect
// descriptor built from ORACLEPROTOCOLPORT10 rather than a tnsnames.ora
// alias, so it needs no TNS_ADMIN.
//
// The clob is fetched both SQLT_BIN and SQLT_STR.  SQLT_BIN reads the wire
// bytes raw, so it checks putLobField() on its own.  SQLT_STR has the client
// convert from the listener's declared charset into NLS_LANG, so it also
// checks that declaration.  #10273: a 9i listener declared WE8ISO8859P1 (31)
// while sending AL32UTF8 bytes, and the client converted every non-ASCII
// byte a second time, in both directions and with or without a clob.  See
// samples/10273-redhat9x86-oci7-strfetch-al32utf8-realserver-r1 (a real
// AL32UTF8 server, clean) and -strfetch-sqlrelay-before-r1 (corrupt).  So a
// multibyte varchar2 fetch and bind are checked SQLT_STR here too.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>
#include <config.h>

// see oci7.cpp for why this wrap, this include order, and leaving ocikpr.h
// out are all required
extern "C" {
	#include <oratypes.h>
	#include <ocidfn.h>
	#include <ociapr.h>
}

Lda_Def		lda;
ub4		hda[256];

const char	*user="testuser";
const char	*password="testpassword";

int	status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

// OCI7 has no error handle and no probed message-text call, so the best a
// failure can show is the ORA number the last call left in the cursor's own
// cda.rc - stashed here by check()/errorCode(), the same pattern oci7.cpp
// uses
int	lasterror=0;

static void printErrors() {
	if (!lasterror) {
		return;
	}
	stdoutput.printf("\nORA-%05d\n",lasterror);
}

static void assertEquals(int actual, int expected) {
	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%d\"!=\"%d\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

static void assertTrue(bool actual) {
	if (actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=true\n",(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

static void reportTestStatus() {
	if (status==0) {
		stdoutput.printf("%s",alltestssucceeded);
	} else {
		stdoutput.printf("%s",sometestsfailed);
	}
}

// remember the ORA number a call left behind, so a failing assertion right
// after it can print something useful
static sword check(Cda_Def *cursor, sword result) {
	lasterror=(int)cursor->rc;
	return result;
}

// open a cursor
static sword openCursor(Cda_Def *cursor) {
	bytestring::zero(cursor,sizeof(Cda_Def));
	return oopen(cursor,&lda,(text *)0,-1,-1,(text *)0,-1);
}

// run a statement, discarding whatever it returns
static sword execImmediate(const char *query) {

	Cda_Def	c;
	if (openCursor(&c)) {
		lasterror=(int)c.rc;
		return (c.rc)?(sword)c.rc:(sword)-1;
	}

	sword	result=oparse(&c,(text *)query,(sb4)-1,0,(ub4)2);
	if (!result) {
		result=oexec(&c);
	}

	lasterror=(int)c.rc;

	oclose(&c);

	return result;
}

int main(int argc, char **argv) {

	// the client's own NLS_LANG also has to be a multibyte charset - the
	// listener always labels a clob AL32UTF8 on the wire, and a
	// single-byte client charset would have OCI transliterate the
	// multibyte characters down to '?' on the way into the odefin
	// buffer, the same reason oracleutf8clob.cpp sets this
	environment::setValue("NLS_LANG","AMERICAN_AMERICA.AL32UTF8");

	const char	*port=environment::getValue("ORACLEPROTOCOLPORT10");
	if (charstring::isNullOrEmpty(port)) {
		port="1530";
	}
	// ORACLEPROTOCOLHOST is unset in every normal run - the listener is
	// always on the same host as the test client.  it exists for a real
	// OCI7 client (redhat9x86) pointed at a listener started on a
	// separate host that has the backend this instance needs
	const char	*host=environment::getValue("ORACLEPROTOCOLHOST");
	if (charstring::isNullOrEmpty(host)) {
		host="localhost";
	}
	char	connectstr[256];
	charstring::printf(connectstr,sizeof(connectstr),
			"(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)"
			"(HOST=%s)(PORT=%s))"
			"(CONNECT_DATA=(SID=ora1)))",host,port);

	stdoutput.printf("\n=============== Connect ==============\n\n");

	stdoutput.printf("olog\n");
	bytestring::zero(&lda,sizeof(lda));
	bytestring::zero(hda,sizeof(hda));
	sword	loggedin=check(&lda,
				olog(&lda,(ub1 *)hda,
					(text *)user,(sword)-1,
					(text *)password,(sword)-1,
					(text *)connectstr,(sword)-1,
					(ub4)OCI_LM_DEF));
	assertEquals((int)loggedin,0);
	stdoutput.printf("\n\n");

	if (loggedin) {
		reportTestStatus();
		return status;
	}


	stdoutput.printf("=========== Multibyte clob ===========\n\n");

	// 3000 repeats of 'a' + U+00E9 (e-acute, 2 utf-8 bytes) + U+20AC
	// (euro sign, 3 utf-8 bytes) = 9000 characters, 18000 utf-8 bytes -
	// past LOB_CHUNK_SIZE (4030 characters) and past putLobField()'s own
	// 8192-character segment size, so a single fetch forces it through
	// more than one getLobFieldSegment() call - unchanged from
	// oracleutf8clob.cpp
	const ub4	repeats=3000;
	const ub4	charsperrepeat=3;
	const ub4	bytesperrepeat=6;
	const ub4	totalbytes=repeats*bytesperrepeat;

	unsigned char	*expected=new unsigned char[totalbytes];
	for (ub4 r=0; r<repeats; r++) {
		unsigned char	*p=expected+(r*bytesperrepeat);
		p[0]=0x61;			// 'a'
		p[1]=0xC3; p[2]=0xA9;		// U+00E9
		p[3]=0xE2; p[4]=0x82; p[5]=0xAC;	// U+20AC
	}

	stdoutput.printf("create table\n");
	execImmediate("drop table oci7utf8clobtest");
	assertEquals((int)
		execImmediate("create table oci7utf8clobtest (testclob clob)"),
		0);
	assertEquals((int)
		execImmediate("insert into oci7utf8clobtest "
				"values (empty_clob())"),
		0);
	stdoutput.printf("\n\n");

	stdoutput.printf("seed the clob\n");
	char	seedsql[512];
	charstring::printf(seedsql,sizeof(seedsql),
			"declare "
				"c clob; "
			"begin "
				"dbms_lob.createtemporary(c,true); "
				"for i in 1..%d loop "
					"dbms_lob.writeappend(c,%d,"
					"unistr('a\\00E9\\20AC')); "
				"end loop; "
				"update oci7utf8clobtest set testclob=c; "
				"dbms_lob.freetemporary(c); "
			"end;",(int)repeats,(int)charsperrepeat);
	assertEquals((int)execImmediate(seedsql),0);
	assertEquals((int)execImmediate("commit"),0);
	stdoutput.printf("\n\n");

	const char	*clobquery="select testclob from oci7utf8clobtest";
	const ub4	bufsize=totalbytes+64;

	stdoutput.printf("odefin, oexec, ofen - the multibyte clob\n");
	// SQLT_BIN gets the raw wire bytes, with no client-side charset
	// conversion, so this passes whatever charset the listener declares
	Cda_Def	cda;
	assertEquals((int)check(&cda,openCursor(&cda)),0);
	assertEquals((int)check(&cda,
			oparse(&cda,(text *)clobquery,(sb4)-1,0,(ub4)2)),0);
	char	*clobbuffer=new char[bufsize];
	sb2	clobind=0;
	ub2	cloblen=0;
	ub2	clobcode=0;
	bytestring::zero(clobbuffer,bufsize);
	assertEquals((int)check(&cda,
			odefin(&cda,1,(ub1 *)clobbuffer,(sword)bufsize,
				SQLT_BIN,-1,&clobind,(text *)0,-1,-1,
				&cloblen,&clobcode)),0);
	assertEquals((int)check(&cda,oexec(&cda)),0);
	assertEquals((int)check(&cda,ofen(&cda,1)),0);
	assertEquals((int)clobind,0);
	assertTrue(!bytestring::compare(clobbuffer,expected,totalbytes));
	stdoutput.printf("\n\n");

	stdoutput.printf("odefin, oexfet - the same multibyte clob\n");
	assertEquals((int)check(&cda,
			oparse(&cda,(text *)clobquery,(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(clobbuffer,bufsize);
	clobind=0;
	cloblen=0;
	clobcode=0;
	assertEquals((int)check(&cda,
			odefin(&cda,1,(ub1 *)clobbuffer,(sword)bufsize,
				SQLT_BIN,-1,&clobind,(text *)0,-1,-1,
				&cloblen,&clobcode)),0);
	assertEquals((int)check(&cda,oexfet(&cda,(ub4)1,0,1)),0);
	assertEquals((int)cda.rpc,1);
	assertEquals((int)clobind,0);
	assertTrue(!bytestring::compare(clobbuffer,expected,totalbytes));
	stdoutput.printf("\n\n");

	stdoutput.printf("odefin, oexec, ofen - the multibyte clob, as text\n");
	// SQLT_STR has the client convert from the listener's declared charset
	// into NLS_LANG, so this only passes if the listener declares the
	// charset the bytes are really in
	assertEquals((int)check(&cda,
			oparse(&cda,(text *)clobquery,(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(clobbuffer,bufsize);
	clobind=0;
	cloblen=0;
	clobcode=0;
	assertEquals((int)check(&cda,
			odefin(&cda,1,(ub1 *)clobbuffer,(sword)bufsize,
				SQLT_STR,-1,&clobind,(text *)0,-1,-1,
				&cloblen,&clobcode)),0);
	assertEquals((int)check(&cda,oexec(&cda)),0);
	assertEquals((int)check(&cda,ofen(&cda,1)),0);
	assertEquals((int)clobind,0);
	assertEquals((int)cloblen,(int)totalbytes);
	assertTrue(!bytestring::compare(clobbuffer,expected,totalbytes));
	stdoutput.printf("\n\n");

	stdoutput.printf("odefin, oexfet - the same multibyte clob, as text\n");
	assertEquals((int)check(&cda,
			oparse(&cda,(text *)clobquery,(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(clobbuffer,bufsize);
	clobind=0;
	cloblen=0;
	clobcode=0;
	assertEquals((int)check(&cda,
			odefin(&cda,1,(ub1 *)clobbuffer,(sword)bufsize,
				SQLT_STR,-1,&clobind,(text *)0,-1,-1,
				&cloblen,&clobcode)),0);
	assertEquals((int)check(&cda,oexfet(&cda,(ub4)1,0,1)),0);
	assertEquals((int)cda.rpc,1);
	assertEquals((int)clobind,0);
	assertEquals((int)cloblen,(int)totalbytes);
	assertTrue(!bytestring::compare(clobbuffer,expected,totalbytes));
	stdoutput.printf("\n\n");

	delete[] clobbuffer;


	stdoutput.printf("========= Multibyte varchar2 =========\n\n");

	stdoutput.printf("odefin, oexec, ofen - a multibyte varchar2, "
							"as text\n");
	// one repeat of the clob's content, built the same way
	const char	*varcharquery=
			"select to_char(unistr('a\\00E9\\20AC')) from dual";
	char	varcharbuffer[64];
	sb2	varcharind=0;
	ub2	varcharlen=0;
	ub2	varcharcode=0;
	bytestring::zero(varcharbuffer,sizeof(varcharbuffer));
	assertEquals((int)check(&cda,
			oparse(&cda,(text *)varcharquery,(sb4)-1,0,(ub4)2)),0);
	assertEquals((int)check(&cda,
			odefin(&cda,1,(ub1 *)varcharbuffer,
				(sword)sizeof(varcharbuffer),
				SQLT_STR,-1,&varcharind,(text *)0,-1,-1,
				&varcharlen,&varcharcode)),0);
	assertEquals((int)check(&cda,oexec(&cda)),0);
	assertEquals((int)check(&cda,ofen(&cda,1)),0);
	assertEquals((int)varcharind,0);
	assertEquals((int)varcharlen,(int)bytesperrepeat);
	assertTrue(!bytestring::compare(varcharbuffer,expected,
							bytesperrepeat));
	stdoutput.printf("\n\n");

	stdoutput.printf("obndrv, odefin, oexec, ofen - a multibyte bind, "
							"as text\n");
	// dump() reports the bytes the database received, in ascii, so this
	// checks the bind on its own rather than a round trip, where two wrong
	// conversions could cancel out
	ub1	bindvalue[bytesperrepeat+1];
	bytestring::copy(bindvalue,expected,bytesperrepeat);
	bindvalue[bytesperrepeat]=0x00;
	sb2	bindind=0;
	char	dumpbuffer[128];
	sb2	dumpind=0;
	ub2	dumplen=0;
	ub2	dumpcode=0;
	bytestring::zero(dumpbuffer,sizeof(dumpbuffer));
	assertEquals((int)check(&cda,
			oparse(&cda,(text *)"select dump(:v,16) from dual",
							(sb4)-1,0,(ub4)2)),0);
	assertEquals((int)check(&cda,
			obndrv(&cda,(text *)":v",-1,bindvalue,
				(sword)sizeof(bindvalue),SQLT_STR,-1,
				&bindind,(text *)0,-1,-1)),0);
	assertEquals((int)check(&cda,
			odefin(&cda,1,(ub1 *)dumpbuffer,
				(sword)sizeof(dumpbuffer),
				SQLT_STR,-1,&dumpind,(text *)0,-1,-1,
				&dumplen,&dumpcode)),0);
	assertEquals((int)check(&cda,oexec(&cda)),0);
	assertEquals((int)check(&cda,ofen(&cda,1)),0);
	assertEquals((int)dumpind,0);
	assertTrue(charstring::contains(dumpbuffer,
					"Len=6: 61,c3,a9,e2,82,ac"));
	stdoutput.printf("\n\n");

	assertEquals((int)check(&cda,oclose(&cda)),0);
	delete[] expected;

	execImmediate("drop table oci7utf8clobtest");
	execImmediate("commit");

	ologof(&lda);

	reportTestStatus();
	return status;
}
