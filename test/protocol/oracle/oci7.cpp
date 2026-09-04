// Copyright (c) David Muse
// See the file COPYING for more information.

// the OCI7 parity counterpart to oci8.cpp (#9653).  it drives the same
// queries, tables, datatypes and operations against the oracle protocol
// module, but through the pre-8.0 call interface (olog/oopen/oparse/odefin/
// oexec/ofen/oexfet/oclose/ologof/odescr/obndrv) rather than through OCI8,
// so the wire traffic the two programs produce can be compared directly.
//
// most build environments have no OCI7-capable oracle client, so HAVE_OCI7
// is empty and the Makefile never compiles this - but that's an availability
// gate, not an untested-code disclaimer: this file does compile and link
// cleanly wherever the legacy headers and symbols are present.  what it
// cannot do anywhere short of #9654's redhat9x86 VM, with a real pre-10g
// client (Oracle9i 9.2.0.4.0), is actually log in and run against a live
// server - every expected value that could not be confirmed that way is
// marked "unverified, see #9654" where it appears.
//
// only the 12 symbols acsite.m4's FW_CHECK_OCI7 link-tests are called:
// olog oopen oparse odefin oexec ofen ofetch oexfet oclose ologof odescr
// obndrv.  several of oci8.cpp's sections need an OCI7 call outside that list
// (obndrn, obndra, oexn, ocom, orol, ocon, ocof, oerhms, odefinps), and each
// of those is omitted with a comment naming the symbol rather than called on
// the chance that it links.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>
#include <config.h>

// the oci7 headers carry no __cplusplus guards of their own, so without this
// wrap every symbol below gets c++ mangled and the link fails.  the order
// matters: ocidfn.h defines struct cda_def and ociapr.h's prototypes
// reference it.  ocikpr.h is deliberately not included - it is the K&R
// variant of the same declarations and conflicts with ociapr.h.
extern "C" {
	#include <oratypes.h>
	#include <ocidfn.h>
	#include <ociapr.h>
}


// cda.ft carries oracle's sql command code after oparse.  these are the
// standard command-type numbers (the same set v$sql.command_type uses)
// rather than OCI8's OCI_STMT_* codes.  unverified, see #9654
#define OCI7_FT_INSERT	2
#define OCI7_FT_SELECT	3
#define OCI7_FT_UPDATE	6
#define OCI7_FT_DELETE	7
#define OCI7_FT_PLSQL	47

// ORA-01403, no data found - what a fetch past the last row leaves in cda.rc.
// this is OCI7's answer to OCI8's OCI_NO_DATA return.  unverified, see #9654
#define OCI7_NO_DATA	1403


Lda_Def		lda;
// 1024 bytes, which covers HDA_SIZE 512 on 64 bit and 256 on 32 bit, and
// matches the program in #9638 comment 14
ub4		hda[256];
Cda_Def		cda;

Lda_Def		authlda;
ub4		authhda[256];

const char	*sid=NULL;
const char	*badsid=NULL;
const char	*user="testuser";
const char	*password="testpassword";


int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

// OCI7 has no error handle and no probed message-text call (oerhms is not
// among the symbols acsite.m4's FW_CHECK_OCI7 link-tests), so the best a
// failure can show is the ORA number the last call left in the cursor's own
// cda.rc.  the assert helpers print whatever was stashed here last.
int lasterror=0;

void printErrors() {
	if (!lasterror) {
		return;
	}
	stdoutput.printf("\nORA-%05d\n",lasterror);
}

void assertEquals(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
			printErrors();
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(int actual, int expected) {
	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%d\"!=\"%d\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertTrue(bool actual) {
	if (actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=true\n",(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

void assertFalse(bool actual) {
	if (!actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=false\n",(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

void reportTestStatus() {
	if (status==0) {
		stdoutput.printf("%s",alltestssucceeded);
	} else {
		stdoutput.printf("%s",sometestsfailed);
	}
}


// oci8.cpp has a clearErrors() here, and a comment explaining why it needs
// one: OCIErrorGet doesn't clear what it reads and a successful call doesn't
// overwrite it, so a tolerated failure leaves stale diagnostics for a later,
// unrelated failure to be blamed on.  OCI7 has no shared error handle at all -
// cda.rc is per-cursor and every call on that cursor overwrites it - so there
// is nothing to clear and no counterpart here.


// remember the ORA number a call left behind, so a failing assertion right
// after it can print something useful
static sword check(Cda_Def *cursor, sword result) {
	lasterror=(int)cursor->rc;
	return result;
}

// the ORA number of the most recent failure on this cursor
static int errorCode(Cda_Def *cursor) {
	lasterror=(int)cursor->rc;
	return lasterror;
}

// open a cursor.  arsize is the cursor's array size - -1 takes oracle's
// default, and a multi-row ofen needs it at least as large as the batch it
// asks for.  dbn and uid are unused, as in the program in #9638 comment 14
static sword openCursor(Cda_Def *cursor, sword arsize) {
	bytestring::zero(cursor,sizeof(Cda_Def));
	return oopen(cursor,&lda,(text *)0,-1,arsize,(text *)0,-1);
}

// run a statement, discarding whatever it returns.  a fresh cursor per call,
// matching OCI8's alloc-per-call shape and keeping state from bleeding
// between statements
static sword execImmediate(const char *query) {

	Cda_Def	c;
	if (openCursor(&c,-1)) {
		lasterror=(int)c.rc;
		return (c.rc)?(sword)c.rc:(sword)-1;
	}

	// sqllen -1 means "null terminated, measure it".  defflg 0 parses now
	// rather than deferring - a deferred parse would put nothing on the
	// wire until the execute.  lngflg 2 selects version 7 behavior
	sword	result=oparse(&c,(text *)query,(sb4)-1,0,(ub4)2);
	if (!result) {
		result=oexec(&c);
	}

	// stash the ORA number before the cursor goes away
	lasterror=(int)c.rc;

	oclose(&c);

	return result;
}

// count the rows in a table, so a commit or rollback can be shown to have
// taken
static int countRows(const char *table) {

	char	query[256];
	charstring::printf(query,sizeof(query),
				"select count(*) from %s",table);

	Cda_Def	c;
	if (openCursor(&c,-1)) {
		lasterror=(int)c.rc;
		return -1;
	}

	int	count=-1;
	char	countbuffer[64];
	sb2	countind=0;
	ub2	countlen=0;
	ub2	countcode=0;
	bytestring::zero(countbuffer,sizeof(countbuffer));

	// the count comes back as text, the same way every other number in
	// this program does.  the define has to be in place before the
	// execute
	if (!oparse(&c,(text *)query,(sb4)-1,0,(ub4)2) &&
		!odefin(&c,1,(ub1 *)countbuffer,(sword)sizeof(countbuffer),
				SQLT_STR,-1,&countind,(text *)0,-1,-1,
				&countlen,&countcode) &&
		!oexec(&c) &&
		!ofen(&c,1)) {
		count=(int)charstring::convertToInteger(countbuffer);
	}

	lasterror=(int)c.rc;

	oclose(&c);

	return count;
}

// pin a result set column's metadata.  OCI8 needs an OCIParamGet plus five
// OCIAttrGet calls for this; OCI7 gets all of it from one odescr, which works
// off the parsed cursor with no execute in the way
static void assertColumn(Cda_Def *cursor, sword pos, const char *name,
					int type, int size,
					int precision, int scale) {

	sb4	dbsize=0;
	sb2	dbtype=0;
	sb1	cbuf[128];
	sb4	cbufl=(sb4)sizeof(cbuf);
	sb4	dsize=0;
	sb2	colprecision=0;
	sb2	colscale=0;
	sb2	nullok=0;
	bytestring::zero(cbuf,sizeof(cbuf));

	assertEquals(check(cursor,
			odescr(cursor,pos,&dbsize,&dbtype,cbuf,&cbufl,
					&dsize,&colprecision,&colscale,
					&nullok)),0);

	// odescr does not null terminate the name - cbufl comes back as its
	// length, the same way OCI_ATTR_NAME hands back an explicit length
	// with no terminator (oci8.cpp:143-155)
	if (cbufl>=0 && cbufl<(sb4)sizeof(cbuf)) {
		cbuf[cbufl]='\0';
	} else {
		cbuf[sizeof(cbuf)-1]='\0';
	}
	assertEquals((const char *)cbuf,name);
	assertEquals((int)cbufl,(int)charstring::getLength(name));

	// dbtype is oracle's INTERNAL datatype code, not the SQLT_ external
	// code OCI8's OCI_ATTR_DATA_TYPE hands back.  for the common types the
	// two happen to agree (1 VARCHAR2, 2 NUMBER, 8 LONG, 12 DATE, 23 RAW,
	// 24 LONG RAW, 96 CHAR, 112 CLOB, 113 BLOB, 114 BFILE), so most of
	// oci8.cpp's expectations carry over unchanged.  the timestamp and
	// interval family does not - see the Datatypes section
	assertEquals((int)dbtype,type);
	assertEquals((int)dbsize,size);
	assertEquals((int)colprecision,precision);
	assertEquals((int)colscale,scale);
}

// OCI_ATTR_PARAM_COUNT has no OCI7 equivalent, so the column count is
// expressed the only way an OCI7 program can express it: describe 1..count
// and then show that count+1 fails
static void assertColumnCount(Cda_Def *cursor, sword count) {

	sb4	dbsize=0;
	sb2	dbtype=0;
	sb1	cbuf[128];
	sb4	cbufl=0;
	sb4	dsize=0;
	sb2	colprecision=0;
	sb2	colscale=0;
	sb2	nullok=0;

	for (sword i=1; i<=count; i++) {
		cbufl=(sb4)sizeof(cbuf);
		assertEquals(check(cursor,
				odescr(cursor,i,&dbsize,&dbtype,cbuf,&cbufl,
					&dsize,&colprecision,&colscale,
					&nullok)),0);
	}

	cbufl=(sb4)sizeof(cbuf);
	assertTrue(odescr(cursor,count+1,&dbsize,&dbtype,cbuf,&cbufl,
				&dsize,&colprecision,&colscale,&nullok)!=0);
	// ORA-01007, variable not in select list.  unverified, see #9654
	assertEquals(errorCode(cursor),1007);
}


int main(int argc, char **argv) {

	// pass "native" to test a real oracle instance instead of
	// sqlrelay's oracle protocol
	bool	issqlrelay=!(argc==2 && !charstring::compare(argv[1],"native"));

	// the oracleprotocolfetchatonce instance sets fetchatonce=1 on its
	// connection string, so the module pulls one row per backend fetch
	// rather than the default 10 - which moves where a row that fails to
	// evaluate shows up.  see the Errors section below
	bool	isfetchatonce=false;

	// select verifier-specific sqlrelay target, if given
	if (argc==2 && !charstring::compare(argv[1],"sqlrelay11g")) {
		sid="sqlrelay11g";
		badsid="sqlrelay11gbad";
	} else if (argc==2 && !charstring::compare(argv[1],"sqlrelay12c")) {
		sid="sqlrelay12c";
		badsid="sqlrelay12cbad";
	} else if (argc==2 && !charstring::compare(argv[1],"sqlrelayconnectstrings")) {
		sid="sqlrelayconnectstrings";
		badsid="sqlrelayconnectstringsbad";
	} else if (argc==2 && !charstring::compare(argv[1],"sqlrelayfetchatonce")) {
		sid="sqlrelayfetchatonce";
		badsid="sqlrelayfetchatoncebad";
		isfetchatonce=true;
	} else if (argc==2 && !charstring::compare(argv[1],"sqlrelayoci7")) {
		// the oracleprotocoloci7 instance has its own backend (#9654) -
		// an OCI7-capable client is too old to authenticate to the
		// same modern backend the other sqlrelay* instances use
		sid="sqlrelayoci7";
		badsid="sqlrelayoci7bad";
	} else {
		sid=(issqlrelay)?"sqlrelay":"ora1";
		badsid=(issqlrelay)?"sqlrelaybad":"ora1bad";
	}

	environment::setValue("ORACLE_SID",sid);
	environment::setValue("TWO_TASK",sid);


	stdoutput.printf("\n=============== Connect ==============\n\n");

	// OCI8's OCIEnvCreate/OCIInitialize+OCIEnvInit have no counterpart
	// here - OCI7 has no environment handle, and opinit() is not among the
	// symbols the configure probe link-tests.  olog is the first call.
	// OCI8's four OCIHandleAllocs have no counterpart either: the LDA, the
	// HDA and every CDA are plain structs, zeroed before use.

	stdoutput.printf("olog\n");
	bytestring::zero(&lda,sizeof(lda));
	bytestring::zero(hda,sizeof(hda));
	// OCI7 fuses OCI8's OCIServerAttach and OCISessionBegin into this one
	// call.  -1 for each length means "null terminated, measure it".
	// OCI_LM_DEF is the default (blocking) login mode.
	//
	// per #9637 comment 2, olog puts 0x52 0x51 0x3b 0x02 0x27 0x08 0x02 on
	// the wire before anything else, and src/protocols/oracle.cpp has no
	// case for TTI opcode 0x27, so this call may fail outright against
	// sqlrelay.  that is expected, and fixing it is #9654's job - the bail
	// out below keeps the failure to one line rather than a cascade
	sword	loggedin=check(&lda,
				olog(&lda,(ub1 *)hda,
					(text *)user,(sword)-1,
					(text *)password,(sword)-1,
					(text *)sid,(sword)-1,
					(ub4)OCI_LM_DEF));
	assertEquals(loggedin,0);
	stdoutput.printf("\n\n");

	// nothing below here can work without a login.  running on anyway gets
	// ORA-01012 from every call
	if (loggedin) {
		reportTestStatus();
		return status;
	}


	// OCI8 sets a transaction handle on the service context here.  OCI7
	// has no transaction handle, so there is nothing to set.

	stdoutput.printf("oopen - main cursor\n");
	// every section below needs a cursor, and in OCI7 a cursor is opened
	// against the LDA rather than allocated out of an environment
	assertEquals(check(&cda,openCursor(&cda,-1)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=========== Authentication ===========\n\n");

	// This section, run against the sqlrelay11g and sqlrelay12c targets
	// (see test.sh.in), is the verifier-type coverage.  The listener
	// behind each of those is pinned to that verifiertype, and the
	// correct-password login below succeeding is the proof that that
	// verifier's O5LOGON crypto path ran end to end.  The server dictates
	// the type and the client picks its crypto from what is presented, so
	// a mismatch surfaces as ORA-01017 even for a correct password, and
	// there is no separate api for asking which type was used.

	// Run against the sqlrelayconnectstrings target, this section covers
	// #9309: oracleprotocolconnectstrings has no <auths> block, so
	// the listener falls back to oracle_connectstrings, the module that
	// used to be the un-rewritten mysql clone.  A correct-password login
	// succeeding there is the regression check for that fix.

	// This section does not cover oracle_clear_password (implemented in
	// src/auths/oracle_userlist.cpp). #9308 confirmed it has no reachable
	// path: the oracle protocol module only ever calls
	// setMethod("O5LOGON") or setMethod("O5LOGON-SERVER-RESPONSE"), and
	// the native SQLRClient protocol never builds an oracle-typed
	// credential at all, so no shipped protocol module can select it.
	// That's documented in the config guide's Oracle Protocol Limitations
	// section and in oracle_userlist.cpp, rather than fixed, since no real
	// Oracle client offers a cleartext password on the wire either.

	// an LDA of its own, so a failed login here cannot disturb the session
	// the rest of the test runs on
	Lda_Def	badlda;
	ub4	badhda[256];


	stdoutput.printf("olog - no such service\n");
	bytestring::zero(&badlda,sizeof(badlda));
	bytestring::zero(badhda,sizeof(badhda));
	// OCI7 has no separate attach, so the bad connect string rides an
	// olog that also carries credentials.  that doesn't change the
	// expected number - the service lookup fails before the credentials
	// matter.
	// #9310: the oracle protocol module now reads CONNECT_DATA's
	// SID/SERVICE_NAME and refuses an unconfigured one with ORA-12514,
	// same as native, so this runs the same assertion either way
	assertTrue(olog(&badlda,(ub1 *)badhda,
				(text *)user,(sword)-1,
				(text *)password,(sword)-1,
				(text *)badsid,(sword)-1,
				(ub4)OCI_LM_DEF)!=0);
	// ORA-12514, the listener does not know the service being
	// asked for.  This one has to come back as a refusal from the
	// far end - a dropped socket gives ORA-12537 or ORA-03113
	// instead.
	assertEquals(errorCode(&badlda),12514);
	stdoutput.printf("\n\n");


	stdoutput.printf("olog - no such alias\n");
	bytestring::zero(&badlda,sizeof(badlda));
	bytestring::zero(badhda,sizeof(badhda));
	const char	*noalias="nosuchalias";
	assertTrue(olog(&badlda,(ub1 *)badhda,
				(text *)user,(sword)-1,
				(text *)password,(sword)-1,
				(text *)noalias,(sword)-1,
				(ub4)OCI_LM_DEF)!=0);
	// ORA-12154, resolved client side out of tnsnames.ora, so nothing
	// leaves the machine for this one
	assertEquals(errorCode(&badlda),12154);
	stdoutput.printf("\n\n");


	// #9311 is about a failed login not dropping the connection - the
	// server waits for another attempt on the same TCP connection, up to a
	// bound.  OCI8 could prove that by asking the service context which
	// server handle it still pointed at, and asserting the pointer had not
	// changed (oci8.cpp:426-435).  OCI7 has no such handle and no such
	// attribute, and a fused olog that fails leaves nothing usable at all,
	// so the reuse-across-failures shape cannot be expressed here.  the
	// four login cases below still assert the right ORA numbers; the
	// connection-identity assertion is an intentional OCI7-side gap.

	stdoutput.printf("olog - wrong password\n");
	bytestring::zero(&authlda,sizeof(authlda));
	bytestring::zero(authhda,sizeof(authhda));
	assertTrue(olog(&authlda,(ub1 *)authhda,
				(text *)user,(sword)-1,
				(text *)"wrongpassword",(sword)-1,
				(text *)sid,(sword)-1,
				(ub4)OCI_LM_DEF)!=0);
	// ORA-01017, invalid username/password.  the connect string is the
	// good one, so this is the login being refused, not the connection
	// failing.
	assertEquals(errorCode(&authlda),1017);
	stdoutput.printf("\n\n");


	stdoutput.printf("olog - unknown user\n");
	bytestring::zero(&authlda,sizeof(authlda));
	bytestring::zero(authhda,sizeof(authhda));
	assertTrue(olog(&authlda,(ub1 *)authhda,
				(text *)"nosuchuser",(sword)-1,
				(text *)"nosuchpassword",(sword)-1,
				(text *)sid,(sword)-1,
				(ub4)OCI_LM_DEF)!=0);
	// ORA-01017 again.  Oracle gives the same error for an unknown user as
	// for a wrong password on purpose, so a client cannot tell which half
	// it got wrong.
	assertEquals(errorCode(&authlda),1017);
	stdoutput.printf("\n\n");


	stdoutput.printf("olog - empty password\n");
	bytestring::zero(&authlda,sizeof(authlda));
	bytestring::zero(authhda,sizeof(authhda));
	assertTrue(olog(&authlda,(ub1 *)authhda,
				(text *)user,(sword)-1,
				(text *)"",(sword)-1,
				(text *)sid,(sword)-1,
				(ub4)OCI_LM_DEF)!=0);
	// ORA-01005, login denied due to invalid password - a different error
	// from ORA-01017.  with OCI8 the client raises it before anything is
	// sent, so it doesn't count against the connection's login-attempt
	// bound; whether OCI7's olog raises it client side too is unverified,
	// see #9654
	assertEquals(errorCode(&authlda),1005);
	stdoutput.printf("\n\n");


	stdoutput.printf("olog - correct password, after the failures\n");
	bytestring::zero(&authlda,sizeof(authlda));
	bytestring::zero(authhda,sizeof(authhda));
	// with OCI8 the failed logins and this one all rode one attached
	// connection, which is the whole point of #9311.  OCI7's olog opens
	// its own connection every time, so this case degrades from "the
	// connection survived three failures" to "a correct password still
	// works"
	sword	authloggedin=check(&authlda,
				olog(&authlda,(ub1 *)authhda,
					(text *)user,(sword)-1,
					(text *)password,(sword)-1,
					(text *)sid,(sword)-1,
					(ub4)OCI_LM_DEF));
	assertEquals(authloggedin,0);
	stdoutput.printf("\n\n");


	// #9174: against sqlrelay the OCI client, in a portable encoding, gets
	// ORA-03113 on the first row-data message, so the query can't be run
	// through the recovered session here yet
	if (!issqlrelay && !authloggedin) {

		stdoutput.printf("oexec - through the recovered session\n");
		// a login that succeeds has to be usable, not just accepted
		Cda_Def		authcda;
		bytestring::zero(&authcda,sizeof(authcda));
		assertEquals(check(&authcda,
				oopen(&authcda,&authlda,(text *)0,-1,-1,
						(text *)0,-1)),0);
		const char	*authquery="select 'authenticated' from dual";
		assertEquals(check(&authcda,
				oparse(&authcda,(text *)authquery,
						(sb4)-1,0,(ub4)2)),0);
		char		authfield[64];
		sb2		authind=0;
		ub2		authlen=0;
		ub2		authcode=0;
		bytestring::zero(authfield,sizeof(authfield));
		assertEquals(check(&authcda,
				odefin(&authcda,1,(ub1 *)authfield,
					(sword)sizeof(authfield),SQLT_STR,-1,
					&authind,(text *)0,-1,-1,
					&authlen,&authcode)),0);
		assertEquals(check(&authcda,oexec(&authcda)),0);
		assertEquals(check(&authcda,ofen(&authcda,1)),0);
		assertEquals((const char *)authfield,"authenticated");
		assertEquals(check(&authcda,oclose(&authcda)),0);
		stdoutput.printf("\n\n");
	}


	stdoutput.printf("ologof\n");
	// OCI8 needs OCISessionEnd, OCIServerDetach and three OCIHandleFrees
	// to unwind a login; OCI7 needs this one call
	assertEquals(check(&authlda,ologof(&authlda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============ Server Version ==========\n\n");

	stdoutput.printf("select - v$version\n");
	// OCIServerVersion has no probed OCI7 counterpart - oversion() is not
	// in ociapr.h on the host this was written against and is not among
	// the symbols the configure probe link-tests - so the same server side
	// value is reached through SQL instead.  that is a different code path
	// in the protocol module than OCIServerVersion's dedicated call, and
	// testuser may not have select on v$version on the native instance; if
	// that turns out to be a problem in #9654, dropping this section
	// outright is the fallback
	{
		Cda_Def	vercda;
		assertEquals(check(&vercda,openCursor(&vercda,-1)),0);
		const char	*versionquery=
					"select banner from v$version "
					"where banner like 'Oracle%'";
		assertEquals(check(&vercda,
				oparse(&vercda,(text *)versionquery,
						(sb4)-1,0,(ub4)2)),0);
		char	versionbuf[512];
		sb2	versionind=0;
		ub2	versionlen=0;
		ub2	versioncode=0;
		bytestring::zero(versionbuf,sizeof(versionbuf));
		assertEquals(check(&vercda,
				odefin(&vercda,1,(ub1 *)versionbuf,
					(sword)sizeof(versionbuf),SQLT_STR,-1,
					&versionind,(text *)0,-1,-1,
					&versionlen,&versioncode)),0);
		assertEquals(check(&vercda,oexec(&vercda)),0);
		assertEquals(check(&vercda,ofen(&vercda,1)),0);
		// the test configs all set serverversion="11.2", which the
		// protocol module packs as 0x0b200100 and expands into this
		// exact banner
		assertEquals((const char *)versionbuf,
			"Oracle Database 11g Enterprise Edition "
			"Release 11.2.0.1.0 - 64bit Production");
		stdoutput.printf("\n%s\n",versionbuf);
		assertEquals(check(&vercda,oclose(&vercda)),0);
	}
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=============== Schema ===============\n\n");

	// every commit and rollback in this program goes through
	// execImmediate() rather than through ocom()/orol().  those are real
	// OCI7 calls, but neither is among the symbols acsite.m4's
	// FW_CHECK_OCI7 link-tests, so they are not called anywhere here.
	// see the Transactions section for what that costs

	// unchecked - the tables may not be there yet
	execImmediate("drop table protocoltesttable");
	execImmediate("drop table protocoltesttran");

	stdoutput.printf("create table\n");
	assertEquals(
		execImmediate("create table protocoltesttable ("
				"testnumber number(10),"
				"testchar char(20),"
				"testvarchar varchar2(40),"
				"testdate date)"),
		0);
	assertEquals(
		execImmediate("create table protocoltesttran ("
				"testnumber number(10))"),
		0);
	assertEquals(countRows("protocoltesttable"),0);
	assertEquals(countRows("protocoltesttran"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("insert\n");
	assertEquals(
		execImmediate("insert into protocoltesttable values "
				"(1,'char1','varchar1',"
				"to_date('2001-01-01 01:01:01',"
					"'YYYY-MM-DD HH24:MI:SS'))"),
		0);
	assertEquals(
		execImmediate("insert into protocoltesttable values "
				"(2,'char2','varchar2',"
				"to_date('2002-02-02 02:02:02',"
					"'YYYY-MM-DD HH24:MI:SS'))"),
		0);
	assertEquals(
		execImmediate("insert into protocoltesttable values "
				"(3,NULL,NULL,NULL)"),
		0);
	assertEquals(execImmediate("commit"),0);
	assertEquals(countRows("protocoltesttable"),3);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============= Statements =============\n\n");

	stdoutput.printf("oparse - select\n");
	const char	*query="select * from protocoltesttable "
					"order by testnumber";
	assertEquals(check(&cda,
			oparse(&cda,(text *)query,(sb4)-1,0,(ub4)2)),0);
	// cda.ft carries the sql command code, which is what OCI8 reads
	// through OCI_ATTR_STMT_TYPE
	assertEquals((int)cda.ft,OCI7_FT_SELECT);
	stdoutput.printf("\n\n");


	stdoutput.printf("oparse - parse only\n");
	// OCI8's OCI_PARSE_ONLY execute has no OCI7 counterpart because oparse
	// IS the parse - the case above, which parses and never executes, is
	// the whole of it.  oparse's defflg could be set to 1 to defer the
	// parse instead, but that changes what goes on the wire and is
	// unverified, so it isn't exercised here
	assertEquals((int)cda.rc,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oparse - statement type\n");
	Cda_Def	typecda;
	assertEquals(check(&typecda,openCursor(&typecda,-1)),0);
	const char	*insertquery="insert into protocoltesttran values (0)";
	assertEquals(check(&typecda,
			oparse(&typecda,(text *)insertquery,
					(sb4)-1,0,(ub4)2)),0);
	assertEquals((int)typecda.ft,OCI7_FT_INSERT);
	const char	*updatequery="update protocoltesttran set testnumber=1";
	assertEquals(check(&typecda,
			oparse(&typecda,(text *)updatequery,
					(sb4)-1,0,(ub4)2)),0);
	assertEquals((int)typecda.ft,OCI7_FT_UPDATE);
	const char	*deletequery="delete from protocoltesttran";
	assertEquals(check(&typecda,
			oparse(&typecda,(text *)deletequery,
					(sb4)-1,0,(ub4)2)),0);
	assertEquals((int)typecda.ft,OCI7_FT_DELETE);
	stdoutput.printf("\n\n");


	stdoutput.printf("oexec - affected row count\n");
	assertEquals(check(&typecda,
			oparse(&typecda,(text *)insertquery,
					(sb4)-1,0,(ub4)2)),0);
	assertEquals(check(&typecda,oexec(&typecda)),0);
	// cda.rpc is the rows processed count, which is what OCI8 reads
	// through OCI_ATTR_ROW_COUNT
	assertEquals((int)typecda.rpc,1);
	assertEquals(check(&typecda,
			oparse(&typecda,(text *)deletequery,
					(sb4)-1,0,(ub4)2)),0);
	assertEquals(check(&typecda,oexec(&typecda)),0);
	assertEquals((int)typecda.rpc,1);
	assertEquals(execImmediate("commit"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oclose - statement\n");
	assertEquals(check(&typecda,oclose(&typecda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Metadata ==============\n\n");

	stdoutput.printf("odescr - column count\n");
	// OCI8 needs a describe-only execute to get here.  OCI7's odescr works
	// straight off the parsed cursor, with no execute at all, which is
	// closer to what this section is testing
	assertColumnCount(&cda,4);
	stdoutput.printf("\n\n");


	stdoutput.printf("odescr - every column\n");
	assertColumn(&cda,1,"TESTNUMBER",SQLT_NUM,22,10,0);
	assertColumn(&cda,2,"TESTCHAR",SQLT_AFC,20,0,0);
	assertColumn(&cda,3,"TESTVARCHAR",SQLT_CHR,40,0,0);
	assertColumn(&cda,4,"TESTDATE",SQLT_DAT,7,0,0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=============== Fetch ================\n\n");

	stdoutput.printf("odefin - every column\n");
	// the defines go in BEFORE the execute here, which is a visible
	// divergence from oci8.cpp - OCI8 executes and then defines, but an
	// OCI7 define has to be in place before the row data arrives
	char		number[64];
	char		charfield[64];
	char		varcharfield[64];
	char		datefield[64];
	sb2		ind[4];
	ub2		retlen[4];
	ub2		retcode[4];
	bytestring::zero(number,sizeof(number));
	bytestring::zero(charfield,sizeof(charfield));
	bytestring::zero(varcharfield,sizeof(varcharfield));
	bytestring::zero(datefield,sizeof(datefield));
	bytestring::zero(ind,sizeof(ind));
	bytestring::zero(retlen,sizeof(retlen));
	bytestring::zero(retcode,sizeof(retcode));
	assertEquals(check(&cda,
			odefin(&cda,1,(ub1 *)number,(sword)sizeof(number),
				SQLT_STR,-1,&ind[0],(text *)0,-1,-1,
				&retlen[0],&retcode[0])),0);
	assertEquals(check(&cda,
			odefin(&cda,2,(ub1 *)charfield,
				(sword)sizeof(charfield),
				SQLT_STR,-1,&ind[1],(text *)0,-1,-1,
				&retlen[1],&retcode[1])),0);
	assertEquals(check(&cda,
			odefin(&cda,3,(ub1 *)varcharfield,
				(sword)sizeof(varcharfield),
				SQLT_STR,-1,&ind[2],(text *)0,-1,-1,
				&retlen[2],&retcode[2])),0);
	assertEquals(check(&cda,
			odefin(&cda,4,(ub1 *)datefield,
				(sword)sizeof(datefield),
				SQLT_STR,-1,&ind[3],(text *)0,-1,-1,
				&retlen[3],&retcode[3])),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oexec\n");
	assertEquals(check(&cda,oexec(&cda)),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - first row\n");
	assertEquals(check(&cda,ofen(&cda,1)),0);
	assertEquals((const char *)number,"1");
	// char columns come back blank padded to their declared width
	assertEquals((const char *)charfield,"char1               ");
	assertEquals((const char *)varcharfield,"varchar1");
	assertEquals((int)ind[0],0);
	assertEquals((int)ind[1],0);
	assertEquals((int)ind[2],0);
	assertEquals((int)ind[3],0);
	assertEquals((int)cda.rpc,1);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - second row\n");
	assertEquals(check(&cda,ofen(&cda,1)),0);
	assertEquals((const char *)number,"2");
	assertEquals((const char *)charfield,"char2               ");
	assertEquals((const char *)varcharfield,"varchar2");
	assertEquals((int)cda.rpc,2);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - nulls\n");
	assertEquals(check(&cda,ofen(&cda,1)),0);
	assertEquals((const char *)number,"3");
	// -1 in an indicator is a null, the same value OCI8 spells
	// OCI_IND_NULL
	assertEquals((int)ind[0],0);
	assertEquals((int)ind[1],-1);
	assertEquals((int)ind[2],-1);
	assertEquals((int)ind[3],-1);
	assertEquals((int)retlen[1],0);
	assertEquals((int)retlen[2],0);
	assertEquals((int)retlen[3],0);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - past the last row\n");
	assertTrue(ofen(&cda,1)!=0);
	assertEquals(errorCode(&cda),OCI7_NO_DATA);
	assertEquals((int)cda.rpc,3);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - date\n");
	// re-parsing the same cursor clears its defines, which is the OCI7
	// idiom for reusing a cursor and the counterpart to oci8.cpp's
	// re-prepare
	const char	*datequery="select testdate "
				"from protocoltesttable "
				"where testnumber=1";
	assertEquals(check(&cda,
			oparse(&cda,(text *)datequery,(sb4)-1,0,(ub4)2)),0);
	ub1	datebytes[7];
	bytestring::zero(datebytes,sizeof(datebytes));
	assertEquals(check(&cda,
			odefin(&cda,1,datebytes,(sword)sizeof(datebytes),
				SQLT_DAT,-1,&ind[0],(text *)0,-1,-1,
				&retlen[0],&retcode[0])),0);
	assertEquals(check(&cda,oexec(&cda)),0);
	assertEquals(check(&cda,ofen(&cda,1)),0);
	// the 7 byte oracle date - excess-100 century and year, then month,
	// day, and excess-1 hour, minute and second
	assertEquals((int)retlen[0],7);
	assertEquals((int)datebytes[0],120);
	assertEquals((int)datebytes[1],101);
	assertEquals((int)datebytes[2],1);
	assertEquals((int)datebytes[3],1);
	assertEquals((int)datebytes[4],2);
	assertEquals((int)datebytes[5],2);
	assertEquals((int)datebytes[6],2);
	stdoutput.printf("\n\n");


	// #9599 - a describe on a mid-fetch cursor must not rewind it.
	// oci8.cpp's version of this case can only check the client-visible
	// behavior: OCI answers OCI_DESCRIBE_ONLY on an already-executed
	// statement out of its own client-side cache, so no second describe
	// ever reaches the wire there.  OCI7 has no such cache in the way and
	// odescr is a real call against the cursor, so this version has a
	// genuine chance of putting a describe on the wire mid-fetch, which is
	// exactly what the server-side guard is for.  whether it actually does
	// is unverified, see #9654
	stdoutput.printf("odescr - mid-fetch\n");
	Cda_Def	midfetchcda;
	assertEquals(check(&midfetchcda,openCursor(&midfetchcda,-1)),0);
	assertEquals(check(&midfetchcda,
			oparse(&midfetchcda,(text *)query,
					(sb4)-1,0,(ub4)2)),0);
	char	midfetchnumber[64];
	sb2	midfetchind=0;
	ub2	midfetchretlen=0;
	ub2	midfetchretcode=0;
	bytestring::zero(midfetchnumber,sizeof(midfetchnumber));
	assertEquals(check(&midfetchcda,
			odefin(&midfetchcda,1,(ub1 *)midfetchnumber,
				(sword)sizeof(midfetchnumber),SQLT_STR,-1,
				&midfetchind,(text *)0,-1,-1,
				&midfetchretlen,&midfetchretcode)),0);
	assertEquals(check(&midfetchcda,oexec(&midfetchcda)),0);
	assertEquals(check(&midfetchcda,ofen(&midfetchcda,1)),0);
	assertEquals((const char *)midfetchnumber,"1");
	assertColumnCount(&midfetchcda,4);
	assertColumn(&midfetchcda,1,"TESTNUMBER",SQLT_NUM,22,10,0);
	assertColumn(&midfetchcda,2,"TESTCHAR",SQLT_AFC,20,0,0);
	assertColumn(&midfetchcda,3,"TESTVARCHAR",SQLT_CHR,40,0,0);
	assertColumn(&midfetchcda,4,"TESTDATE",SQLT_DAT,7,0,0);
	// the fetch picks up where it left off, rather than back at row 1
	assertEquals(check(&midfetchcda,ofen(&midfetchcda,1)),0);
	assertEquals((const char *)midfetchnumber,"2");
	assertEquals(check(&midfetchcda,ofen(&midfetchcda,1)),0);
	assertEquals((const char *)midfetchnumber,"3");
	assertTrue(ofen(&midfetchcda,1)!=0);
	assertEquals(errorCode(&midfetchcda),OCI7_NO_DATA);
	assertEquals(check(&midfetchcda,oclose(&midfetchcda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=============== Binds ================\n\n");

	Cda_Def	bindcda;
	assertEquals(check(&bindcda,openCursor(&bindcda,-1)),0);

	// a table of its own, so the fetch section's fixed three rows stay
	// where they are
	execImmediate("drop table protocoltestbind");
	assertEquals(
		execImmediate("create table protocoltestbind ("
				"testnumber number(10),"
				"testchar char(20),"
				"testvarchar varchar2(40))"),
		0);

	const char	*bindinsert="insert into protocoltestbind "
					"(testnumber,testchar,testvarchar) "
					"values (:num,:chr,:vchr)";
	sb4		bindnumber=0;
	char		bindchar[32];
	char		bindvarchar[64];
	sb2		bindind[3];
	bytestring::zero(bindind,sizeof(bindind));


	stdoutput.printf("obndrv\n");
	assertEquals(check(&bindcda,
			oparse(&bindcda,(text *)bindinsert,
					(sb4)-1,0,(ub4)2)),0);
	bindnumber=10;
	charstring::copy(bindchar,"bindchar");
	charstring::copy(bindvarchar,"bindvarchar");
	// sqlvl -1 means "null terminated, measure it", which saves the
	// explicit 4/4/5 lengths OCI8's OCIBindByName needs.  the binds go in
	// after the parse and before the execute, the same order OCI8 uses
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":num",-1,
				(ub1 *)&bindnumber,(sword)sizeof(bindnumber),
				SQLT_INT,-1,&bindind[0],(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":chr",-1,
				(ub1 *)bindchar,(sword)sizeof(bindchar),
				SQLT_STR,-1,&bindind[1],(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":vchr",-1,
				(ub1 *)bindvarchar,(sword)sizeof(bindvarchar),
				SQLT_STR,-1,&bindind[2],(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,oexec(&bindcda)),0);
	assertEquals(countRows("protocoltestbind"),1);
	assertEquals(
		countRows("protocoltestbind where testnumber=10 and "
				"testvarchar='bindvarchar'"),1);
	stdoutput.printf("\n\n");


	// oci8.cpp binds the same insert by position here (OCIBindByPos).
	// OCI7's bind-by-position call is obndrn(), and it is not among the
	// symbols acsite.m4's FW_CHECK_OCI7 link-tests, so a build that passed
	// the probe is no proof it links.  positional binds are an intentional
	// OCI7-side gap rather than a call made on spec.


	stdoutput.printf("obndrv - null binds\n");
	assertEquals(check(&bindcda,
			oparse(&bindcda,(text *)bindinsert,
					(sb4)-1,0,(ub4)2)),0);
	bindnumber=30;
	// the buffers still hold the values from the case above, so a null
	// really has to come from the indicator, not from an empty buffer
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":num",-1,
				(ub1 *)&bindnumber,(sword)sizeof(bindnumber),
				SQLT_INT,-1,&bindind[0],(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":chr",-1,
				(ub1 *)bindchar,(sword)sizeof(bindchar),
				SQLT_STR,-1,&bindind[1],(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":vchr",-1,
				(ub1 *)bindvarchar,(sword)sizeof(bindvarchar),
				SQLT_STR,-1,&bindind[2],(text *)0,-1,-1)),0);
	bindind[1]=-1;
	bindind[2]=-1;
	assertEquals(check(&bindcda,oexec(&bindcda)),0);
	assertEquals(
		countRows("protocoltestbind where testnumber=30 and "
				"testchar is null and testvarchar is null"),1);
	bindind[1]=0;
	bindind[2]=0;
	stdoutput.printf("\n\n");


	stdoutput.printf("oexec - bind once, execute many\n");
	assertEquals(check(&bindcda,
			oparse(&bindcda,(text *)bindinsert,
					(sb4)-1,0,(ub4)2)),0);
	charstring::copy(bindchar,"manychar");
	charstring::copy(bindvarchar,"manyvarchar");
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":num",-1,
				(ub1 *)&bindnumber,(sword)sizeof(bindnumber),
				SQLT_INT,-1,&bindind[0],(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":chr",-1,
				(ub1 *)bindchar,(sword)sizeof(bindchar),
				SQLT_STR,-1,&bindind[1],(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":vchr",-1,
				(ub1 *)bindvarchar,(sword)sizeof(bindvarchar),
				SQLT_STR,-1,&bindind[2],(text *)0,-1,-1)),0);
	// obndrv binds by reference - the "rv" in the name - so the binds stay
	// put and only the buffer changes between executes.  this case covers
	// the bind-reuse half of what the omitted array bind below tested
	for (sb4 i=0; i<3; i++) {
		bindnumber=40+i;
		assertEquals(check(&bindcda,oexec(&bindcda)),0);
	}
	assertEquals(
		countRows("protocoltestbind where testnumber between 40 and 42"),
		3);
	stdoutput.printf("\n\n");


	// oci8.cpp runs an array bind here (oci8.cpp:1137-1185): one
	// OCIBindByName per placeholder pointing at an array base, then an
	// OCIStmtExecute with iters=3.  There is no way to say that with the
	// twelve symbols acsite.m4's FW_CHECK_OCI7 link-tests.  OCI7's array
	// bind is obndra(), its skip-parameter form is obindps(), and running
	// N iterations of a bound statement is oexn() - none of the three is
	// probed.  obndrv binds a single program variable and oexec runs a
	// single iteration, so an array bind is an intentional OCI7-side gap.
	// the bind-once-execute-many case above covers the bind-reuse half of
	// what the array case was testing, and it does work with obndrv.


	stdoutput.printf("obndrv - output bind\n");
	const char	*outblock="begin select count(*) into :cnt "
					"from protocoltestbind; end;";
	assertEquals(check(&bindcda,
			oparse(&bindcda,(text *)outblock,(sb4)-1,0,(ub4)2)),0);
	assertEquals((int)bindcda.ft,OCI7_FT_PLSQL);
	sb4	outcount=0;
	sb2	outind=0;
	// the block's :cnt is an in/out bind by reference, so the value comes
	// back in the same buffer.  whether OCI7's obndrv round-trips a PL/SQL
	// out bind against this module is exactly what #9654 is for
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":cnt",-1,
				(ub1 *)&outcount,(sword)sizeof(outcount),
				SQLT_INT,-1,&outind,(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,oexec(&bindcda)),0);
	assertEquals((int)outcount,countRows("protocoltestbind"));
	assertEquals((int)outind,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("obndrv - in-out bind\n");
	const char	*inoutblock="begin :v := :v * 2; end;";
	assertEquals(check(&bindcda,
			oparse(&bindcda,(text *)inoutblock,
					(sb4)-1,0,(ub4)2)),0);
	sb4	inoutvalue=21;
	sb2	inoutind=0;
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":v",-1,
				(ub1 *)&inoutvalue,(sword)sizeof(inoutvalue),
				SQLT_INT,-1,&inoutind,(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,oexec(&bindcda)),0);
	assertEquals((int)inoutvalue,42);
	stdoutput.printf("\n\n");


	stdoutput.printf("obndrv - output bind, null\n");
	const char	*nulloutblock="begin :v := NULL; end;";
	assertEquals(check(&bindcda,
			oparse(&bindcda,(text *)nulloutblock,
					(sb4)-1,0,(ub4)2)),0);
	inoutvalue=99;
	inoutind=0;
	assertEquals(check(&bindcda,
			obndrv(&bindcda,(text *)":v",-1,
				(ub1 *)&inoutvalue,(sword)sizeof(inoutvalue),
				SQLT_INT,-1,&inoutind,(text *)0,-1,-1)),0);
	assertEquals(check(&bindcda,oexec(&bindcda)),0);
	assertEquals((int)inoutind,-1);
	stdoutput.printf("\n\n");


	stdoutput.printf("oexec - no such placeholder\n");
	assertEquals(check(&bindcda,
			oparse(&bindcda,(text *)bindinsert,
					(sb4)-1,0,(ub4)2)),0);
	// oci8.cpp asserts the BIND fails with ORA-01036, because
	// OCIStmtPrepare parses the placeholders client side.  OCI7 has no
	// client-side placeholder check - oparse is a server round trip and
	// obndrv is local - so ORA-01036 has no counterpart here and obndrv's
	// return is deliberately not asserted.  whether OCI7 accepts this bind
	// locally is unverified, see #9654
	obndrv(&bindcda,(text *)":nosuchbind",-1,
			(ub1 *)&bindnumber,(sword)sizeof(bindnumber),
			SQLT_INT,-1,&bindind[0],(text *)0,-1,-1);
	// with nothing bound, the execute cannot go
	assertTrue(oexec(&bindcda)!=0);
	// ORA-01008, not all variables bound
	assertEquals(errorCode(&bindcda),1008);
	stdoutput.printf("\n\n");


	stdoutput.printf("oclose - statement\n");
	assertEquals(execImmediate("commit"),0);
	assertEquals(check(&bindcda,oclose(&bindcda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============= Datatypes ==============\n\n");

	Cda_Def	typecda2;
	assertEquals(check(&typecda2,openCursor(&typecda2,-1)),0);

	stdoutput.printf("create table - one column per type\n");
	execImmediate("drop table protocoltesttypes");
	execImmediate("drop table protocoltestlong");
	execImmediate("drop table protocoltestlongraw");
	assertEquals(
		execImmediate("create table protocoltesttypes ("
				"testvarchar varchar2(40),"
				"testnumber number(10,2),"
				"testdate date,"
				"testraw raw(20),"
				"testchar char(20),"
				"testrowid rowid,"
				"testtimestamp timestamp,"
				"testtimestamptz timestamp with time zone,"
				"testintervalym interval year to month,"
				"testintervalds interval day to second)"),
		0);
	// only one LONG column is allowed per table, so those get their own
	assertEquals(
		execImmediate("create table protocoltestlong "
				"(testlong long)"),
		0);
	assertEquals(
		execImmediate("create table protocoltestlongraw "
				"(testlongraw long raw)"),
		0);
	stdoutput.printf("\n\n");


	stdoutput.printf("insert - one row of every type\n");
	assertEquals(
		execImmediate("insert into protocoltesttypes values ("
			"'varchar value',"
			"123.45,"
			"to_date('2003-03-03 03:03:03',"
				"'YYYY-MM-DD HH24:MI:SS'),"
			"hextoraw('0102030405'),"
			"'char value',"
			"NULL,"
			"to_timestamp('2004-04-04 04:04:04.444444',"
				"'YYYY-MM-DD HH24:MI:SS.FF'),"
			"to_timestamp_tz('2005-05-05 05:05:05.555555 -05:00',"
				"'YYYY-MM-DD HH24:MI:SS.FF TZH:TZM'),"
			"to_yminterval('01-02'),"
			"to_dsinterval('3 04:05:06.777777'))"),
		0);
	// a rowid has to come from somewhere real, so borrow one
	assertEquals(
		execImmediate("update protocoltesttypes set testrowid="
				"(select max(rowid) from protocoltesttable)"),
		0);
	assertEquals(
		execImmediate("insert into protocoltestlong values "
				"('long value')"),
		0);
	assertEquals(
		execImmediate("insert into protocoltestlongraw values "
				"(hextoraw('0a0b0c0d0e'))"),
		0);
	assertEquals(execImmediate("commit"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("odescr - every type\n");
	const char	*typequery="select * from protocoltesttypes";
	assertEquals(check(&typecda2,
			oparse(&typecda2,(text *)typequery,
					(sb4)-1,0,(ub4)2)),0);
	assertColumnCount(&typecda2,10);
	assertColumn(&typecda2,1,"TESTVARCHAR",SQLT_CHR,40,0,0);
	assertColumn(&typecda2,2,"TESTNUMBER",SQLT_NUM,22,10,2);
	assertColumn(&typecda2,3,"TESTDATE",SQLT_DAT,7,0,0);
	assertColumn(&typecda2,4,"TESTRAW",SQLT_BIN,20,0,0);
	assertColumn(&typecda2,5,"TESTCHAR",SQLT_AFC,20,0,0);
	// oci8.cpp asserts SQLT_RDD (104) here, which is the descriptor form
	// OCI8 remaps a rowid column to.  an OCI7 describe may report 11
	// (SQLT_RID) instead.  unverified, see #9654
	assertColumn(&typecda2,6,"TESTROWID",SQLT_RDD,8,0,0);
	// oci8.cpp asserts 187 through 190 (SQLT_TIMESTAMP and friends), which
	// is OCI's own remapping of what the module actually puts on the wire.
	// the module calls these 180 through 183, at
	// src/protocols/oracle.cpp:145-148, and an OCI7 describe has no reason
	// to remap them, so 180-183 is what is expected here.  unverified,
	// see #9654
	assertColumn(&typecda2,7,"TESTTIMESTAMP",180,11,0,6);
	assertColumn(&typecda2,8,"TESTTIMESTAMPTZ",181,13,0,6);
	assertColumn(&typecda2,9,"TESTINTERVALYM",182,5,2,0);
	assertColumn(&typecda2,10,"TESTINTERVALDS",183,11,2,6);
	stdoutput.printf("\n\n");


	stdoutput.printf("odefin - every type\n");
	// OCI8 defines the rowid, timestamp and interval columns into
	// OCIRowid/OCIDateTime/OCIInterval descriptors and reads them back
	// with component getters (OCIDateTimeGetDate and friends).  OCI7 has
	// no descriptors at all and none of those calls, so those four columns
	// are pulled as text instead.  the query pins the rendering with
	// to_char rather than leaving it to the session's NLS settings, so the
	// expected strings below don't move with the environment.  the date
	// and raw columns keep their oracle-format defines, so oci8.cpp's byte
	// level assertions survive intact
	const char	*typefetchquery=
			"select testvarchar,testnumber,testdate,testraw,"
			"testchar,testrowid,"
			"to_char(testtimestamp,'YYYY-MM-DD HH24:MI:SS.FF6'),"
			"to_char(testtimestamptz,"
				"'YYYY-MM-DD HH24:MI:SS.FF6 TZH:TZM'),"
			"to_char(testintervalym),"
			"to_char(testintervalds) "
			"from protocoltesttypes";
	assertEquals(check(&typecda2,
			oparse(&typecda2,(text *)typefetchquery,
					(sb4)-1,0,(ub4)2)),0);

	sb2	typeind[10];
	ub2	typelen[10];
	ub2	typecode[10];
	char	typevarchar[64];
	char	typenumber[64];
	ub1	typedate[7];
	ub1	typeraw[32];
	char	typechar[64];
	char	typerowid[64];
	char	typetimestamp[64];
	char	typetimestamptz[64];
	char	typeintervalym[64];
	char	typeintervalds[64];
	bytestring::zero(typeind,sizeof(typeind));
	bytestring::zero(typelen,sizeof(typelen));
	bytestring::zero(typecode,sizeof(typecode));
	bytestring::zero(typevarchar,sizeof(typevarchar));
	bytestring::zero(typenumber,sizeof(typenumber));
	bytestring::zero(typedate,sizeof(typedate));
	bytestring::zero(typeraw,sizeof(typeraw));
	bytestring::zero(typechar,sizeof(typechar));
	bytestring::zero(typerowid,sizeof(typerowid));
	bytestring::zero(typetimestamp,sizeof(typetimestamp));
	bytestring::zero(typetimestamptz,sizeof(typetimestamptz));
	bytestring::zero(typeintervalym,sizeof(typeintervalym));
	bytestring::zero(typeintervalds,sizeof(typeintervalds));

	// ftype 1 (SQLT_CHR) rather than 5 (SQLT_STR) for the varchar and 96
	// (SQLT_AFC) for the char, so the returned length is the value's own
	// length with no terminator counted - which is what oci8.cpp's length
	// assertions below are written against
	assertEquals(check(&typecda2,
			odefin(&typecda2,1,(ub1 *)typevarchar,
				(sword)sizeof(typevarchar),SQLT_CHR,-1,
				&typeind[0],(text *)0,-1,-1,
				&typelen[0],&typecode[0])),0);
	// oci8.cpp defines this as SQLT_VNU and turns it into a double with
	// OCINumberToReal.  that call is OCI8-only, so the number comes back
	// as text here and is compared as text
	assertEquals(check(&typecda2,
			odefin(&typecda2,2,(ub1 *)typenumber,
				(sword)sizeof(typenumber),SQLT_STR,-1,
				&typeind[1],(text *)0,-1,-1,
				&typelen[1],&typecode[1])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,3,typedate,
				(sword)sizeof(typedate),SQLT_DAT,-1,
				&typeind[2],(text *)0,-1,-1,
				&typelen[2],&typecode[2])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,4,typeraw,
				(sword)sizeof(typeraw),SQLT_BIN,-1,
				&typeind[3],(text *)0,-1,-1,
				&typelen[3],&typecode[3])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,5,(ub1 *)typechar,
				(sword)sizeof(typechar),SQLT_AFC,-1,
				&typeind[4],(text *)0,-1,-1,
				&typelen[4],&typecode[4])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,6,(ub1 *)typerowid,
				(sword)sizeof(typerowid),SQLT_STR,-1,
				&typeind[5],(text *)0,-1,-1,
				&typelen[5],&typecode[5])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,7,(ub1 *)typetimestamp,
				(sword)sizeof(typetimestamp),SQLT_STR,-1,
				&typeind[6],(text *)0,-1,-1,
				&typelen[6],&typecode[6])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,8,(ub1 *)typetimestamptz,
				(sword)sizeof(typetimestamptz),SQLT_STR,-1,
				&typeind[7],(text *)0,-1,-1,
				&typelen[7],&typecode[7])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,9,(ub1 *)typeintervalym,
				(sword)sizeof(typeintervalym),SQLT_STR,-1,
				&typeind[8],(text *)0,-1,-1,
				&typelen[8],&typecode[8])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,10,(ub1 *)typeintervalds,
				(sword)sizeof(typeintervalds),SQLT_STR,-1,
				&typeind[9],(text *)0,-1,-1,
				&typelen[9],&typecode[9])),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - every type\n");
	assertEquals(check(&typecda2,oexec(&typecda2)),0);
	assertEquals(check(&typecda2,ofen(&typecda2,1)),0);
	for (int i=0; i<10; i++) {
		assertEquals((int)typeind[i],0);
	}
	stdoutput.printf("\n\n");


	stdoutput.printf("varchar, number, date, raw, char\n");
	// SQLT_CHR does not null terminate, so the length is what says where
	// the value stops
	assertEquals((int)typelen[0],13);
	assertEquals((const char *)typevarchar,"varchar value");
	assertEquals((const char *)typenumber,"123.45");
	// the 7 byte oracle date - excess-100 century and year, then month,
	// day, and excess-1 hour, minute and second
	assertEquals((int)typelen[2],7);
	assertEquals((int)typedate[0],120);
	assertEquals((int)typedate[1],103);
	assertEquals((int)typedate[2],3);
	assertEquals((int)typedate[3],3);
	assertEquals((int)typedate[4],4);
	assertEquals((int)typedate[5],4);
	assertEquals((int)typedate[6],4);
	assertEquals((int)typelen[3],5);
	assertEquals((int)typeraw[0],1);
	assertEquals((int)typeraw[1],2);
	assertEquals((int)typeraw[2],3);
	assertEquals((int)typeraw[3],4);
	assertEquals((int)typeraw[4],5);
	// char comes back blank padded to the declared width
	assertEquals((int)typelen[4],20);
	assertEquals((const char *)typechar,"char value          ");
	stdoutput.printf("\n\n");


	stdoutput.printf("rowid\n");
	// OCI8 gets this through OCIRowidToChar off a rowid descriptor; a
	// plain string define should give the same 18 character base 64 form.
	// unverified, see #9654
	assertEquals((int)charstring::getLength(typerowid),18);
	stdoutput.printf("\n\n");


	stdoutput.printf("timestamp\n");
	assertEquals((const char *)typetimestamp,"2004-04-04 04:04:04.444444");
	stdoutput.printf("\n\n");


	stdoutput.printf("timestamp with time zone\n");
	assertEquals((const char *)typetimestamptz,
			"2005-05-05 05:05:05.555555 -05:00");
	stdoutput.printf("\n\n");


	stdoutput.printf("interval year to month\n");
	// to_char of an interval takes no format mask, so this is oracle's own
	// default rendering of to_yminterval('01-02').  unverified, see #9654
	assertEquals((const char *)typeintervalym,"+01-02");
	stdoutput.printf("\n\n");


	stdoutput.printf("interval day to second\n");
	// same for to_dsinterval('3 04:05:06.777777').  unverified, see #9654
	assertEquals((const char *)typeintervalds,"+03 04:05:06.777777");
	stdoutput.printf("\n\n");


	stdoutput.printf("long\n");
	const char	*longquery="select testlong from protocoltestlong";
	assertEquals(check(&typecda2,
			oparse(&typecda2,(text *)longquery,
					(sb4)-1,0,(ub4)2)),0);
	// a long has no declared width, so the describe reports size 0
	assertColumn(&typecda2,1,"TESTLONG",SQLT_LNG,0,0,0);
	char	longvalue[4096];
	sb2	longind=0;
	ub2	longlen=0;
	ub2	longcode=0;
	bytestring::zero(longvalue,sizeof(longvalue));
	assertEquals(check(&typecda2,
			odefin(&typecda2,1,(ub1 *)longvalue,
				(sword)sizeof(longvalue),SQLT_LNG,-1,
				&longind,(text *)0,-1,-1,
				&longlen,&longcode)),0);
	assertEquals(check(&typecda2,oexec(&typecda2)),0);
	assertEquals(check(&typecda2,ofen(&typecda2,1)),0);
	assertEquals((int)longind,0);
	assertEquals((int)longlen,10);
	assertEquals((const char *)longvalue,"long value");
	stdoutput.printf("\n\n");


	stdoutput.printf("long raw\n");
	const char	*longrawquery="select testlongraw "
					"from protocoltestlongraw";
	assertEquals(check(&typecda2,
			oparse(&typecda2,(text *)longrawquery,
					(sb4)-1,0,(ub4)2)),0);
	assertColumn(&typecda2,1,"TESTLONGRAW",SQLT_LBI,0,0,0);
	ub1	longrawvalue[4096];
	sb2	longrawind=0;
	ub2	longrawlen=0;
	ub2	longrawcode=0;
	bytestring::zero(longrawvalue,sizeof(longrawvalue));
	assertEquals(check(&typecda2,
			odefin(&typecda2,1,longrawvalue,
				(sword)sizeof(longrawvalue),SQLT_LBI,-1,
				&longrawind,(text *)0,-1,-1,
				&longrawlen,&longrawcode)),0);
	assertEquals(check(&typecda2,oexec(&typecda2)),0);
	assertEquals(check(&typecda2,ofen(&typecda2,1)),0);
	assertEquals((int)longrawind,0);
	assertEquals((int)longrawlen,5);
	assertEquals((int)longrawvalue[0],10);
	assertEquals((int)longrawvalue[1],11);
	assertEquals((int)longrawvalue[2],12);
	assertEquals((int)longrawvalue[3],13);
	assertEquals((int)longrawvalue[4],14);
	stdoutput.printf("\n\n");


	stdoutput.printf("nulls, every type\n");
	assertEquals(execImmediate("delete from protocoltesttypes"),0);
	assertEquals(
		execImmediate("insert into protocoltesttypes "
				"(testvarchar) values (NULL)"),
		0);
	assertEquals(execImmediate("commit"),0);
	assertEquals(check(&typecda2,
			oparse(&typecda2,(text *)typequery,
					(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(typeind,sizeof(typeind));
	bytestring::zero(typelen,sizeof(typelen));
	assertEquals(check(&typecda2,
			odefin(&typecda2,1,(ub1 *)typevarchar,
				(sword)sizeof(typevarchar),SQLT_CHR,-1,
				&typeind[0],(text *)0,-1,-1,
				&typelen[0],&typecode[0])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,2,(ub1 *)typenumber,
				(sword)sizeof(typenumber),SQLT_STR,-1,
				&typeind[1],(text *)0,-1,-1,
				&typelen[1],&typecode[1])),0);
	assertEquals(check(&typecda2,
			odefin(&typecda2,3,typedate,
				(sword)sizeof(typedate),SQLT_DAT,-1,
				&typeind[2],(text *)0,-1,-1,
				&typelen[2],&typecode[2])),0);
	// position 7 is the timestamp column, defined as text here for the
	// same reason as above
	assertEquals(check(&typecda2,
			odefin(&typecda2,7,(ub1 *)typetimestamp,
				(sword)sizeof(typetimestamp),SQLT_STR,-1,
				&typeind[6],(text *)0,-1,-1,
				&typelen[6],&typecode[6])),0);
	assertEquals(check(&typecda2,oexec(&typecda2)),0);
	assertEquals(check(&typecda2,ofen(&typecda2,1)),0);
	assertEquals((int)typeind[0],-1);
	assertEquals((int)typeind[1],-1);
	assertEquals((int)typeind[2],-1);
	assertEquals((int)typeind[6],-1);
	assertEquals((int)typelen[0],0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oclose - statement\n");
	// oci8.cpp frees five descriptors here.  OCI7 has no descriptors, so
	// closing the cursor is the whole of the cleanup
	assertEquals(check(&typecda2,oclose(&typecda2)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============ Long-form CLR ===========\n\n");

	// #9594 - a value over CLR_MAX_SHORT_LENGTH (252 bytes,
	// src/protocols/oracle.cpp:366) goes out from the server using
	// oracle's long-form CLR framing: a 0xfe marker, a run of
	// length-prefixed chunks, then a zero-length chunk to end it.
	// These cases round-trip values right at that boundary, then well
	// past it, to catch a wrong chunk length or a wrong chunk boundary
	// in putLenBytes() (src/protocols/oracle.cpp:4811).
	// this is server-side framing, so it mirrors from oci8.cpp unchanged -
	// an odefin into a 4096 byte buffer reads a 253 byte value exactly as
	// OCIDefineByPos did

	Cda_Def	clrcda;
	assertEquals(check(&clrcda,openCursor(&clrcda,-1)),0);

	stdoutput.printf("create table\n");
	execImmediate("drop table protocoltestclr");
	assertEquals(
		execImmediate("create table protocoltestclr "
				"(testvarchar varchar2(4000))"),
		0);
	stdoutput.printf("\n\n");


	const char	*clrquery="select testvarchar from protocoltestclr";
	char		clrvarchar[4096];
	sb2		clrind=0;
	ub2		clrlen=0;
	ub2		clrcode=0;

	stdoutput.printf("select - 252 bytes, at the short-form boundary\n");
	assertEquals(
		execImmediate("insert into protocoltestclr values "
				"(rpad('A',252,'A'))"),
		0);
	assertEquals(execImmediate("commit"),0);
	char	clr252[253];
	bytestring::zero(clr252,sizeof(clr252));
	for (ub4 i=0; i<252; i++) {
		clr252[i]='A';
	}
	assertEquals(check(&clrcda,
			oparse(&clrcda,(text *)clrquery,(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(clrvarchar,sizeof(clrvarchar));
	assertEquals(check(&clrcda,
			odefin(&clrcda,1,(ub1 *)clrvarchar,
				(sword)sizeof(clrvarchar),SQLT_CHR,-1,
				&clrind,(text *)0,-1,-1,
				&clrlen,&clrcode)),0);
	assertEquals(check(&clrcda,oexec(&clrcda)),0);
	assertEquals(check(&clrcda,ofen(&clrcda,1)),0);
	assertEquals((int)clrind,0);
	assertEquals((int)clrlen,252);
	assertEquals((const char *)clrvarchar,(const char *)clr252);
	stdoutput.printf("\n\n");


	stdoutput.printf("select - 253 bytes, just into long form\n");
	assertEquals(execImmediate("delete from protocoltestclr"),0);
	assertEquals(
		execImmediate("insert into protocoltestclr values "
				"(rpad('B',253,'B'))"),
		0);
	assertEquals(execImmediate("commit"),0);
	char	clr253[254];
	bytestring::zero(clr253,sizeof(clr253));
	for (ub4 i=0; i<253; i++) {
		clr253[i]='B';
	}
	// re-parse before re-executing an already-fetched-to-completion
	// cursor - executing it again as-is sends a different tti function
	// (an oracle re-execute/resync call) that this module doesn't
	// implement yet, unrelated to #9594's chunk framing
	assertEquals(check(&clrcda,
			oparse(&clrcda,(text *)clrquery,(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(clrvarchar,sizeof(clrvarchar));
	assertEquals(check(&clrcda,
			odefin(&clrcda,1,(ub1 *)clrvarchar,
				(sword)sizeof(clrvarchar),SQLT_CHR,-1,
				&clrind,(text *)0,-1,-1,
				&clrlen,&clrcode)),0);
	assertEquals(check(&clrcda,oexec(&clrcda)),0);
	assertEquals(check(&clrcda,ofen(&clrcda,1)),0);
	assertEquals((int)clrind,0);
	assertEquals((int)clrlen,253);
	assertEquals((const char *)clrvarchar,(const char *)clr253);
	stdoutput.printf("\n\n");


	stdoutput.printf("select - 1000 bytes, several long-form chunks\n");
	assertEquals(execImmediate("delete from protocoltestclr"),0);
	assertEquals(
		execImmediate("insert into protocoltestclr values "
				"(rpad('C',1000,'C'))"),
		0);
	assertEquals(execImmediate("commit"),0);
	char	clr1000[1001];
	bytestring::zero(clr1000,sizeof(clr1000));
	for (ub4 i=0; i<1000; i++) {
		clr1000[i]='C';
	}
	// re-parse - see the same note above the 253-byte case
	assertEquals(check(&clrcda,
			oparse(&clrcda,(text *)clrquery,(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(clrvarchar,sizeof(clrvarchar));
	assertEquals(check(&clrcda,
			odefin(&clrcda,1,(ub1 *)clrvarchar,
				(sword)sizeof(clrvarchar),SQLT_CHR,-1,
				&clrind,(text *)0,-1,-1,
				&clrlen,&clrcode)),0);
	assertEquals(check(&clrcda,oexec(&clrcda)),0);
	assertEquals(check(&clrcda,ofen(&clrcda,1)),0);
	assertEquals((int)clrind,0);
	assertEquals((int)clrlen,1000);
	assertEquals((const char *)clrvarchar,(const char *)clr1000);
	stdoutput.printf("\n\n");


	// coverage stops at 1000 bytes.  getting a larger value into a
	// column needs either a bind over 255 bytes or a lob write, and
	// neither is implemented yet - see #9587 and #9589
	assertEquals(check(&clrcda,oclose(&clrcda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n======== Outbound long-form writers ========\n\n");

	// #9595 - putLenBytes() (src/protocols/oracle.cpp:5409) is one of
	// two writers that switch a value over CLR_MAX_SHORT_LENGTH (252
	// bytes) to the chunked long form; putLongBytes()
	// (src/protocols/oracle.cpp:10942) is the other, used only for LONG
	// and LONG RAW columns and closing with two extra zero bytes that
	// putLenBytes() doesn't write.  the "Long-form CLR" cases above
	// exercise putLenBytes() through a plain select; these round-trip
	// values well past 252 bytes through putLongBytes() (a LONG and a
	// LONG RAW column) and through putOutBindValues()
	// (src/protocols/oracle.cpp:8886), which reaches putLenBytes() for
	// a PL/SQL out bind rather than a row value.

	Cda_Def	bigcda;
	assertEquals(check(&bigcda,openCursor(&bigcda,-1)),0);

	stdoutput.printf("long - 300 bytes, putLongBytes' long form\n");
	assertEquals(execImmediate("delete from protocoltestlong"),0);
	char	longbig[301];
	for (ub4 i=0; i<300; i++) {
		longbig[i]=(char)('0'+(i%10));
	}
	longbig[300]='\0';
	char	longbiginsert[350];
	charstring::printf(longbiginsert,sizeof(longbiginsert),
			"insert into protocoltestlong values ('%s')",
			longbig);
	assertEquals(execImmediate(longbiginsert),0);
	assertEquals(execImmediate("commit"),0);
	assertEquals(check(&bigcda,
			oparse(&bigcda,(text *)longquery,(sb4)-1,0,(ub4)2)),0);
	char	longbigvalue[4096];
	sb2	longbigind=0;
	ub2	longbiglen=0;
	ub2	longbigcode=0;
	bytestring::zero(longbigvalue,sizeof(longbigvalue));
	assertEquals(check(&bigcda,
			odefin(&bigcda,1,(ub1 *)longbigvalue,
				(sword)sizeof(longbigvalue),SQLT_LNG,-1,
				&longbigind,(text *)0,-1,-1,
				&longbiglen,&longbigcode)),0);
	assertEquals(check(&bigcda,oexec(&bigcda)),0);
	assertEquals(check(&bigcda,ofen(&bigcda,1)),0);
	assertEquals((int)longbigind,0);
	assertEquals((int)longbiglen,300);
	assertEquals((const char *)longbigvalue,(const char *)longbig);
	stdoutput.printf("\n\n");


	stdoutput.printf("long raw - 300 bytes, putLongBytes' long form\n");
	assertEquals(execImmediate("delete from protocoltestlongraw"),0);
	ub1	longrawbig[300];
	for (ub4 i=0; i<300; i++) {
		longrawbig[i]=(ub1)(i%256);
	}
	char	longrawbighex[sizeof(longrawbig)*2+1];
	for (ub4 i=0; i<sizeof(longrawbig); i++) {
		charstring::printf(longrawbighex+i*2,3,"%02X",
					(int)longrawbig[i]);
	}
	char	longrawbiginsert[700];
	charstring::printf(longrawbiginsert,sizeof(longrawbiginsert),
			"insert into protocoltestlongraw values "
			"(hextoraw('%s'))",longrawbighex);
	assertEquals(execImmediate(longrawbiginsert),0);
	assertEquals(execImmediate("commit"),0);
	assertEquals(check(&bigcda,
			oparse(&bigcda,(text *)longrawquery,
					(sb4)-1,0,(ub4)2)),0);
	ub1	longrawbigvalue[4096];
	sb2	longrawbigind=0;
	ub2	longrawbiglen=0;
	ub2	longrawbigcode=0;
	bytestring::zero(longrawbigvalue,sizeof(longrawbigvalue));
	assertEquals(check(&bigcda,
			odefin(&bigcda,1,longrawbigvalue,
				(sword)sizeof(longrawbigvalue),SQLT_LBI,-1,
				&longrawbigind,(text *)0,-1,-1,
				&longrawbiglen,&longrawbigcode)),0);
	assertEquals(check(&bigcda,oexec(&bigcda)),0);
	assertEquals(check(&bigcda,ofen(&bigcda,1)),0);
	assertEquals((int)longrawbigind,0);
	assertEquals((int)longrawbiglen,300);
	assertTrue(!bytestring::compare(longrawbigvalue,longrawbig,
						sizeof(longrawbig)));
	stdoutput.printf("\n\n");


	stdoutput.printf("obndrv - output bind, 300 bytes, "
				"putOutBindValues via putLenBytes\n");
	// the same PL/SQL out bind risk as the Binds section's cases - whether
	// obndrv round-trips one against this module is what #9654 settles
	const char	*outbindbigblock="begin :v := rpad('Q',300,'Q'); "
						"end;";
	assertEquals(check(&bigcda,
			oparse(&bigcda,(text *)outbindbigblock,
					(sb4)-1,0,(ub4)2)),0);
	char	outbindbigvalue[512];
	sb2	outbindbigind=0;
	bytestring::zero(outbindbigvalue,sizeof(outbindbigvalue));
	assertEquals(check(&bigcda,
			obndrv(&bigcda,(text *)":v",-1,
				(ub1 *)outbindbigvalue,
				(sword)sizeof(outbindbigvalue),
				SQLT_STR,-1,&outbindbigind,
				(text *)0,-1,-1)),0);
	assertEquals(check(&bigcda,oexec(&bigcda)),0);
	assertEquals((int)outbindbigind,0);
	char	outbindbigexpected[301];
	for (ub4 i=0; i<300; i++) {
		outbindbigexpected[i]='Q';
	}
	outbindbigexpected[300]='\0';
	assertEquals((const char *)outbindbigvalue,
				(const char *)outbindbigexpected);
	stdoutput.printf("\n\n");


	assertEquals(check(&bigcda,oclose(&bigcda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n================ Lobs ================\n\n");

	// this section changes shape entirely from oci8.cpp's.  #9638
	// comment 13 measured what a real OCI7 client does with a lob column:
	// a plain odefin with a plain external type - STRING for a clob, RAW
	// for a blob - fetches the bytes INLINE in the row data, exactly as it
	// would against a LONG or LONG RAW column.  no locator comes back, no
	// separate lob call is issued, and the query does not error.  the only
	// wire difference from an ordinary short column is the length prefix,
	// "07 fe <len>" instead of "07 01 <byte>".
	//
	// so everything locator-shaped in oci8.cpp's Lobs section is dropped
	// here: OCILobGetLength, OCILobRead (both the one-round-trip and the
	// in-pieces case), OCILobWrite and its ORA-03001 assertions, OCILobTrim
	// and its ORA-03001 assertion, OCILobFileGetName, OCILobFileExists, and
	// the three OCIDescriptorAlloc/Free pairs.  none of them has an OCI7
	// counterpart - there are no lob locators in this API generation, so
	// there is nothing to call them on.

	Cda_Def	lobcda;
	assertEquals(check(&lobcda,openCursor(&lobcda,-1)),0);

	// oci8.cpp seeds 100000 characters, because OCILobRead can pull a lob
	// back in pieces.  an OCI7 inline fetch has one define buffer and no
	// way to ask for a second piece, and #9638 comment 13's measurement
	// only covers a value short enough to fit that buffer, so this is
	// seeded at 4000 characters instead
	const ub4	biglength=4000;
	char		bigvalue[4001];
	for (ub4 i=0; i<biglength; i++) {
		bigvalue[i]=(char)('a'+(i%25));
	}
	bigvalue[biglength]='\0';
	ub1	blobvalue[512];
	char	blobhex[sizeof(blobvalue)*2+1];
	for (int i=0; i<(int)sizeof(blobvalue); i++) {
		blobvalue[i]=(ub1)(i%256);
		charstring::printf(blobhex+i*2,3,"%02X",(int)blobvalue[i]);
	}

	stdoutput.printf("create table\n");
	execImmediate("drop table protocoltestlob");
	assertEquals(
		execImmediate("create table protocoltestlob ("
				"testclob clob,"
				"testblob blob,"
				"testbfile bfile)"),
		0);
	// DMP_DIR is the one directory object testuser can see on the native
	// instance.  Creating another needs CREATE ANY DIRECTORY, which it
	// does not have.
	// the blob goes in as a raw literal - 512 bytes is well inside what
	// one hextoraw() holds
	char	lobinsert[sizeof(blobhex)+256];
	charstring::printf(lobinsert,sizeof(lobinsert),
			"insert into protocoltestlob values "
			"(empty_clob(),hextoraw('%s'),"
			"bfilename('DMP_DIR','protocoltest.txt'))",
			blobhex);
	assertEquals(execImmediate(lobinsert),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("seed the clob\n");
	// 160 rounds of the same 25 characters, which is what bigvalue holds
	assertEquals(
		execImmediate("declare "
				"c clob; "
			"begin "
				"dbms_lob.createtemporary(c,true); "
				"for i in 1..160 loop "
					"dbms_lob.writeappend(c,25,"
					"'abcdefghijklmnopqrstuvwxy'); "
				"end loop; "
				"update protocoltestlob set testclob=c; "
				"dbms_lob.freetemporary(c); "
			"end;"),
		0);
	assertEquals(execImmediate("commit"),0);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testclob)=4000"),1);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testblob)=512"),1);
	stdoutput.printf("\n\n");


	// oci8.cpp's lob query ends in "for update", which it needs because it
	// takes locators.  an inline fetch does not, so it is dropped here
	const char	*lobquery="select testclob,testblob,testbfile "
					"from protocoltestlob";

	stdoutput.printf("odescr - the lob columns\n");
	assertEquals(check(&lobcda,
			oparse(&lobcda,(text *)lobquery,(sb4)-1,0,(ub4)2)),0);
	// unverified for an OCI7 describe, see #9654
	assertColumn(&lobcda,1,"TESTCLOB",SQLT_CLOB,4000,0,0);
	assertColumn(&lobcda,2,"TESTBLOB",SQLT_BLOB,4000,0,0);
	assertColumn(&lobcda,3,"TESTBFILE",SQLT_BFILEE,530,0,0);
	stdoutput.printf("\n\n");


	char	clobbuffer[4352];
	ub1	blobbuffer[1024];
	char	bfilebuffer[1024];
	sb2	lobind[3];
	ub2	loblen[3];
	ub2	lobcode[3];

	stdoutput.printf("odefin, oexec, ofen - the lob columns inline\n");
	bytestring::zero(clobbuffer,sizeof(clobbuffer));
	bytestring::zero(blobbuffer,sizeof(blobbuffer));
	bytestring::zero(bfilebuffer,sizeof(bfilebuffer));
	bytestring::zero(lobind,sizeof(lobind));
	bytestring::zero(loblen,sizeof(loblen));
	bytestring::zero(lobcode,sizeof(lobcode));
	// dty 5 (STRING) for the clob and 23 (RAW) for the blob are the two
	// #9638 comment 13 measured.  there is no evidence either way for a
	// bfile, so it is defined as a string and only the call and the fetch
	// are asserted - not a value
	assertEquals(check(&lobcda,
			odefin(&lobcda,1,(ub1 *)clobbuffer,
				(sword)sizeof(clobbuffer),SQLT_STR,-1,
				&lobind[0],(text *)0,-1,-1,
				&loblen[0],&lobcode[0])),0);
	assertEquals(check(&lobcda,
			odefin(&lobcda,2,blobbuffer,
				(sword)sizeof(blobbuffer),SQLT_BIN,-1,
				&lobind[1],(text *)0,-1,-1,
				&loblen[1],&lobcode[1])),0);
	assertEquals(check(&lobcda,
			odefin(&lobcda,3,(ub1 *)bfilebuffer,
				(sword)sizeof(bfilebuffer),SQLT_STR,-1,
				&lobind[2],(text *)0,-1,-1,
				&loblen[2],&lobcode[2])),0);
	assertEquals(check(&lobcda,oexec(&lobcda)),0);
	assertEquals(check(&lobcda,ofen(&lobcda,1)),0);
	assertEquals((int)lobind[0],0);
	assertEquals((int)lobind[1],0);
	assertEquals((const char *)clobbuffer,(const char *)bigvalue);
	assertEquals((int)loblen[1],(int)sizeof(blobvalue));
	assertTrue(!bytestring::compare(blobbuffer,blobvalue,
						sizeof(blobvalue)));
	stdoutput.printf("\n\n");


	stdoutput.printf("odefin, oexfet - the same lob columns\n");
	// the same select through the other legacy fetch shape.  #9638
	// comment 13 ran both, and their TTI sequences differ - oexfet's
	// carries the row data on the query response with no separate fetch
	// opcode at all - so running both here gives the module both shapes
	// over a lob column in one program
	assertEquals(check(&lobcda,
			oparse(&lobcda,(text *)lobquery,(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(clobbuffer,sizeof(clobbuffer));
	bytestring::zero(blobbuffer,sizeof(blobbuffer));
	bytestring::zero(bfilebuffer,sizeof(bfilebuffer));
	bytestring::zero(lobind,sizeof(lobind));
	bytestring::zero(loblen,sizeof(loblen));
	bytestring::zero(lobcode,sizeof(lobcode));
	assertEquals(check(&lobcda,
			odefin(&lobcda,1,(ub1 *)clobbuffer,
				(sword)sizeof(clobbuffer),SQLT_STR,-1,
				&lobind[0],(text *)0,-1,-1,
				&loblen[0],&lobcode[0])),0);
	assertEquals(check(&lobcda,
			odefin(&lobcda,2,blobbuffer,
				(sword)sizeof(blobbuffer),SQLT_BIN,-1,
				&lobind[1],(text *)0,-1,-1,
				&loblen[1],&lobcode[1])),0);
	assertEquals(check(&lobcda,
			odefin(&lobcda,3,(ub1 *)bfilebuffer,
				(sword)sizeof(bfilebuffer),SQLT_STR,-1,
				&lobind[2],(text *)0,-1,-1,
				&loblen[2],&lobcode[2])),0);
	assertEquals(check(&lobcda,oexfet(&lobcda,(ub4)1,0,1)),0);
	assertEquals((int)lobcda.rpc,1);
	assertEquals((int)lobind[0],0);
	assertEquals((int)lobind[1],0);
	assertEquals((const char *)clobbuffer,(const char *)bigvalue);
	assertTrue(!bytestring::compare(blobbuffer,blobvalue,
						sizeof(blobvalue)));
	stdoutput.printf("\n\n");


	stdoutput.printf("commit - end the lob transaction\n");
	assertEquals(execImmediate("commit"),0);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testclob)=4000"),1);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testblob)=512"),1);
	assertEquals(check(&lobcda,oclose(&lobcda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Cursors ===============\n\n");

	// oopen's arsize is the cursor's array size, and a multi-row ofen
	// cannot ask for more rows than it.  that argument, plus ofen's nrows,
	// is as close as OCI7 gets to OCI8's OCI_ATTR_PREFETCH_ROWS - there is
	// no attribute to set or read back, so oci8.cpp's set/get case
	// (oci8.cpp:2494-2514) folds into the fetches below
	Cda_Def	curcda;
	assertEquals(check(&curcda,openCursor(&curcda,10)),0);

	stdoutput.printf("create table - ten rows to fetch\n");
	execImmediate("drop table protocoltestarray");
	assertEquals(
		execImmediate("create table protocoltestarray ("
				"testnumber number(10),"
				"testvarchar varchar2(40))"),
		0);
	assertEquals(
		execImmediate("insert into protocoltestarray "
				"select level,'row'||level from dual "
				"connect by level<=10"),
		0);
	assertEquals(execImmediate("commit"),0);
	assertEquals(countRows("protocoltestarray"),10);
	stdoutput.printf("\n\n");


	// oci8.cpp binds a ref cursor here (SQLT_RSET) and then describes and
	// fetches through the nested statement handle (oci8.cpp:2415-2491).
	// OCI7 predates ref cursors.  SQLT_CUR (102) exists in ocidfn.h, but
	// there is no probed call that turns a bound Cda_Def into a fetchable
	// cursor, so the whole ref-cursor block is an intentional OCI7-side
	// gap.


	const char	*arrayquery="select testnumber,testvarchar "
					"from protocoltestarray "
					"order by testnumber";

	stdoutput.printf("ofen - multi-row fetch\n");
	// oci8.cpp fetches 4 rows at a time into 4-slot client-side arrays.
	// OCI7 cannot: striding a define across array slots needs odefinps's
	// pv_skip/ind_skip/alen_skip/rc_skip arguments, and odefinps is not
	// among the symbols acsite.m4's FW_CHECK_OCI7 link-tests; plain odefin
	// has no stride argument at all.  what is worth testing here is the
	// server side anyway - the module packing several rows into one fetch
	// response - and ofen(&curcda,4) still asks for 4 rows in one
	// TTI_FETCH.  the client just keeps the last one, so the per-row
	// assertions oci8.cpp makes across the array become last-row
	// assertions here
	assertEquals(check(&curcda,
			oparse(&curcda,(text *)arrayquery,
					(sb4)-1,0,(ub4)2)),0);
	char	arrnumber[32];
	char	arrvarchar[64];
	sb2	arrind[2];
	ub2	arrlen[2];
	ub2	arrcode[2];
	bytestring::zero(arrnumber,sizeof(arrnumber));
	bytestring::zero(arrvarchar,sizeof(arrvarchar));
	bytestring::zero(arrind,sizeof(arrind));
	bytestring::zero(arrlen,sizeof(arrlen));
	bytestring::zero(arrcode,sizeof(arrcode));
	assertEquals(check(&curcda,
			odefin(&curcda,1,(ub1 *)arrnumber,
				(sword)sizeof(arrnumber),SQLT_STR,-1,
				&arrind[0],(text *)0,-1,-1,
				&arrlen[0],&arrcode[0])),0);
	assertEquals(check(&curcda,
			odefin(&curcda,2,(ub1 *)arrvarchar,
				(sword)sizeof(arrvarchar),SQLT_STR,-1,
				&arrind[1],(text *)0,-1,-1,
				&arrlen[1],&arrcode[1])),0);
	assertEquals(check(&curcda,oexec(&curcda)),0);

	// 10 rows, 4 at a time - two full batches, then a short one
	assertEquals(check(&curcda,ofen(&curcda,4)),0);
	assertEquals((const char *)arrnumber,"4");
	assertEquals((const char *)arrvarchar,"row4");
	assertEquals((int)curcda.rpc,4);

	assertEquals(check(&curcda,ofen(&curcda,4)),0);
	assertEquals((const char *)arrnumber,"8");
	assertEquals((const char *)arrvarchar,"row8");
	assertEquals((int)curcda.rpc,8);

	// only 2 rows left, so this batch comes up short and says so
	assertTrue(ofen(&curcda,4)!=0);
	assertEquals(errorCode(&curcda),OCI7_NO_DATA);
	assertEquals((int)curcda.rpc,10);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - multi-row fetch wider than one packet\n");
	// rows wide enough that a 6-row batch can't fit in one negotiated
	// packet - this exercises the SDU bound in sendFetch3Response() and
	// sendQuery3Response()'s inline prefetch, which have to split the
	// batch across more than one on-the-wire fetch
	execImmediate("drop table protocoltestwide");
	assertEquals(
		execImmediate("create table protocoltestwide ("
				"testnumber number(10),"
				"testvarchar varchar2(2000))"),
		0);
	assertEquals(
		execImmediate("insert into protocoltestwide "
				"select level,rpad('row'||level,2000,'x') "
				"from dual connect by level<=6"),
		0);
	assertEquals(execImmediate("commit"),0);
	assertEquals(countRows("protocoltestwide"),6);

	const char	*widequery="select testnumber,testvarchar "
					"from protocoltestwide "
					"order by testnumber";
	Cda_Def	widecda;
	assertEquals(check(&widecda,openCursor(&widecda,6)),0);
	assertEquals(check(&widecda,
			oparse(&widecda,(text *)widequery,
					(sb4)-1,0,(ub4)2)),0);
	char	widenumber[32];
	char	widevarchar[2001];
	sb2	wideind[2];
	ub2	widelen[2];
	ub2	widecode[2];
	bytestring::zero(widenumber,sizeof(widenumber));
	bytestring::zero(widevarchar,sizeof(widevarchar));
	bytestring::zero(wideind,sizeof(wideind));
	bytestring::zero(widelen,sizeof(widelen));
	bytestring::zero(widecode,sizeof(widecode));
	assertEquals(check(&widecda,
			odefin(&widecda,1,(ub1 *)widenumber,
				(sword)sizeof(widenumber),SQLT_STR,-1,
				&wideind[0],(text *)0,-1,-1,
				&widelen[0],&widecode[0])),0);
	assertEquals(check(&widecda,
			odefin(&widecda,2,(ub1 *)widevarchar,
				(sword)sizeof(widevarchar),SQLT_STR,-1,
				&wideind[1],(text *)0,-1,-1,
				&widelen[1],&widecode[1])),0);
	assertEquals(check(&widecda,oexec(&widecda)),0);

	// all 6 rows in one fetch call - the module can only pack a few of
	// these into any single on-the-wire packet, so it has to send more
	// than one to satisfy the batch
	assertEquals(check(&widecda,ofen(&widecda,6)),0);
	assertEquals((const char *)widenumber,"6");
	char	wideexpected[2001];
	charstring::printf(wideexpected,sizeof(wideexpected),"row6");
	for (size_t wj=charstring::getLength(wideexpected); wj<2000; wj++) {
		wideexpected[wj]='x';
	}
	wideexpected[2000]='\0';
	assertEquals((const char *)widevarchar,wideexpected);
	assertEquals((int)widecda.rpc,6);
	assertTrue(ofen(&widecda,1)!=0);
	assertEquals(errorCode(&widecda),OCI7_NO_DATA);
	assertEquals(check(&widecda,oclose(&widecda)),0);
	assertEquals(execImmediate("drop table protocoltestwide"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oexfet - exact fetch of one row\n");
	// oexfet has no counterpart in oci8.cpp at all - it is the other of
	// the two legacy fetch shapes #9637 captured, and its TTI sequence
	// carries the row data on the query response with no separate fetch
	// opcode.  this is the coverage this program adds that oci8.cpp cannot
	// provide.  cancel is 0 and exact is 1, matching the program in #9638
	// comment 14
	Cda_Def	exfetcda;
	assertEquals(check(&exfetcda,openCursor(&exfetcda,10)),0);
	assertEquals(check(&exfetcda,
			oparse(&exfetcda,(text *)arrayquery,
					(sb4)-1,0,(ub4)2)),0);
	char	exfetnumber[32];
	char	exfetvarchar[64];
	sb2	exfetind[2];
	ub2	exfetlen[2];
	ub2	exfetcode[2];
	bytestring::zero(exfetnumber,sizeof(exfetnumber));
	bytestring::zero(exfetvarchar,sizeof(exfetvarchar));
	bytestring::zero(exfetind,sizeof(exfetind));
	bytestring::zero(exfetlen,sizeof(exfetlen));
	bytestring::zero(exfetcode,sizeof(exfetcode));
	assertEquals(check(&exfetcda,
			odefin(&exfetcda,1,(ub1 *)exfetnumber,
				(sword)sizeof(exfetnumber),SQLT_STR,-1,
				&exfetind[0],(text *)0,-1,-1,
				&exfetlen[0],&exfetcode[0])),0);
	assertEquals(check(&exfetcda,
			odefin(&exfetcda,2,(ub1 *)exfetvarchar,
				(sword)sizeof(exfetvarchar),SQLT_STR,-1,
				&exfetind[1],(text *)0,-1,-1,
				&exfetlen[1],&exfetcode[1])),0);
	assertEquals(check(&exfetcda,oexfet(&exfetcda,(ub4)1,0,1)),0);
	assertEquals((const char *)exfetnumber,"1");
	assertEquals((const char *)exfetvarchar,"row1");
	assertEquals((int)exfetcda.rpc,1);
	assertEquals(check(&exfetcda,oclose(&exfetcda)),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oexfet - exact fetch of ten rows\n");
	Cda_Def	exfetcda2;
	assertEquals(check(&exfetcda2,openCursor(&exfetcda2,10)),0);
	assertEquals(check(&exfetcda2,
			oparse(&exfetcda2,(text *)arrayquery,
					(sb4)-1,0,(ub4)2)),0);
	bytestring::zero(exfetnumber,sizeof(exfetnumber));
	bytestring::zero(exfetvarchar,sizeof(exfetvarchar));
	bytestring::zero(exfetind,sizeof(exfetind));
	bytestring::zero(exfetlen,sizeof(exfetlen));
	bytestring::zero(exfetcode,sizeof(exfetcode));
	assertEquals(check(&exfetcda2,
			odefin(&exfetcda2,1,(ub1 *)exfetnumber,
				(sword)sizeof(exfetnumber),SQLT_STR,-1,
				&exfetind[0],(text *)0,-1,-1,
				&exfetlen[0],&exfetcode[0])),0);
	assertEquals(check(&exfetcda2,
			odefin(&exfetcda2,2,(ub1 *)exfetvarchar,
				(sword)sizeof(exfetvarchar),SQLT_STR,-1,
				&exfetind[1],(text *)0,-1,-1,
				&exfetlen[1],&exfetcode[1])),0);
	// exactly as many rows as the table has.  an exact fetch that consumes
	// the whole result set may still report ORA-01403 at end of data -
	// success is what is asserted here, and #9654 pins it either way
	assertEquals(check(&exfetcda2,oexfet(&exfetcda2,(ub4)10,0,1)),0);
	assertEquals((const char *)exfetnumber,"10");
	assertEquals((const char *)exfetvarchar,"row10");
	assertEquals((int)exfetcda2.rpc,10);
	assertEquals(check(&exfetcda2,oclose(&exfetcda2)),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oexfet - four columns\n");
	// #9637 is specifically about the exact-fetch trailer for column
	// counts of 3 and up, which nobody had ground truth for.
	// protocoltesttable has exactly 4 columns, so this is that case
	Cda_Def	exfetcda3;
	assertEquals(check(&exfetcda3,openCursor(&exfetcda3,10)),0);
	assertEquals(check(&exfetcda3,
			oparse(&exfetcda3,(text *)query,(sb4)-1,0,(ub4)2)),0);
	char	exfet4number[64];
	char	exfet4char[64];
	char	exfet4varchar[64];
	char	exfet4date[64];
	sb2	exfet4ind[4];
	ub2	exfet4len[4];
	ub2	exfet4code[4];
	bytestring::zero(exfet4number,sizeof(exfet4number));
	bytestring::zero(exfet4char,sizeof(exfet4char));
	bytestring::zero(exfet4varchar,sizeof(exfet4varchar));
	bytestring::zero(exfet4date,sizeof(exfet4date));
	bytestring::zero(exfet4ind,sizeof(exfet4ind));
	bytestring::zero(exfet4len,sizeof(exfet4len));
	bytestring::zero(exfet4code,sizeof(exfet4code));
	assertEquals(check(&exfetcda3,
			odefin(&exfetcda3,1,(ub1 *)exfet4number,
				(sword)sizeof(exfet4number),SQLT_STR,-1,
				&exfet4ind[0],(text *)0,-1,-1,
				&exfet4len[0],&exfet4code[0])),0);
	assertEquals(check(&exfetcda3,
			odefin(&exfetcda3,2,(ub1 *)exfet4char,
				(sword)sizeof(exfet4char),SQLT_STR,-1,
				&exfet4ind[1],(text *)0,-1,-1,
				&exfet4len[1],&exfet4code[1])),0);
	assertEquals(check(&exfetcda3,
			odefin(&exfetcda3,3,(ub1 *)exfet4varchar,
				(sword)sizeof(exfet4varchar),SQLT_STR,-1,
				&exfet4ind[2],(text *)0,-1,-1,
				&exfet4len[2],&exfet4code[2])),0);
	assertEquals(check(&exfetcda3,
			odefin(&exfetcda3,4,(ub1 *)exfet4date,
				(sword)sizeof(exfet4date),SQLT_STR,-1,
				&exfet4ind[3],(text *)0,-1,-1,
				&exfet4len[3],&exfet4code[3])),0);
	assertEquals(check(&exfetcda3,oexfet(&exfetcda3,(ub4)1,0,1)),0);
	assertEquals((const char *)exfet4number,"1");
	assertEquals((const char *)exfet4char,"char1               ");
	assertEquals((const char *)exfet4varchar,"varchar1");
	assertEquals((int)exfetcda3.rpc,1);
	assertEquals(check(&exfetcda3,oclose(&exfetcda3)),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oclose - statement\n");
	assertEquals(check(&curcda,oclose(&curcda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============ Transactions ============\n\n");

	// this section drives the module's SQL path rather than its
	// TTI_COMMIT (0x0E) and TTI_ROLLBACK (0x0F) opcodes, which is a real
	// difference from what oci8.cpp exercises here.  OCI7's dedicated
	// calls are ocom() and orol(), and neither is among the symbols
	// acsite.m4's FW_CHECK_OCI7 link-tests, so the commits and rollbacks
	// go through execImmediate() like every other statement

	stdoutput.printf("rollback\n");
	assertEquals(
		execImmediate("insert into protocoltesttran values (1)"),0);
	assertEquals(countRows("protocoltesttran"),1);
	assertEquals(execImmediate("rollback"),0);
	assertEquals(countRows("protocoltesttran"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("commit\n");
	assertEquals(
		execImmediate("insert into protocoltesttran values (2)"),0);
	assertEquals(execImmediate("commit"),0);
	// the commit took, so the rollback after it loses nothing
	assertEquals(execImmediate("rollback"),0);
	assertEquals(countRows("protocoltesttran"),1);
	stdoutput.printf("\n\n");


	// oci8.cpp follows those with an OCI_COMMIT_ON_SUCCESS execute and an
	// autocommit-off execute (oci8.cpp:2705-2740).  OCI7's autocommit
	// control is ocon() and ocof(), and the module does implement
	// TTI_AUTOCOMMIT_ON (0x0C) and _OFF (0x0D), but neither call is among
	// the symbols acsite.m4's FW_CHECK_OCI7 link-tests.  both cases are
	// omitted, and the row counts above are the ones this shorter sequence
	// actually produces rather than oci8.cpp's.



	stdoutput.printf("\n=============== Errors ===============\n\n");

	Cda_Def	errcda;
	assertEquals(check(&errcda,openCursor(&errcda,-1)),0);

	// the first three of these surface on oparse rather than on the
	// execute, because in OCI7 the parse is the server round trip

	stdoutput.printf("oparse - no such table\n");
	const char	*badtable="select * from nosuchtable";
	assertTrue(oparse(&errcda,(text *)badtable,(sb4)-1,0,(ub4)2)!=0);
	// ORA-00942, table or view does not exist
	assertEquals(errorCode(&errcda),942);
	stdoutput.printf("\n\n");


	stdoutput.printf("oparse - no such column\n");
	const char	*badcolumn="select nosuchcolumn from protocoltesttable";
	assertTrue(oparse(&errcda,(text *)badcolumn,(sb4)-1,0,(ub4)2)!=0);
	// ORA-00904, invalid identifier
	assertEquals(errorCode(&errcda),904);
	stdoutput.printf("\n\n");


	stdoutput.printf("oparse - bad syntax\n");
	const char	*badsyntax="selectt 1 from dual";
	assertTrue(oparse(&errcda,(text *)badsyntax,(sb4)-1,0,(ub4)2)!=0);
	// ORA-00900, invalid SQL statement
	assertEquals(errorCode(&errcda),900);
	stdoutput.printf("\n\n");


	stdoutput.printf("oexec - value too wide for the column\n");
	// a runtime error rather than a parse error, so this one lands on the
	// execute, as it does in oci8.cpp
	const char	*toowide="insert into protocoltesttable (testvarchar) "
				"values ('123456789012345678901234567890"
					"12345678901234567890')";
	assertEquals(check(&errcda,
			oparse(&errcda,(text *)toowide,(sb4)-1,0,(ub4)2)),0);
	assertTrue(oexec(&errcda)!=0);
	// ORA-12899, value too large for column
	assertEquals(errorCode(&errcda),12899);
	stdoutput.printf("\n\n");


	stdoutput.printf("ofen - statement never executed\n");
	const char	*neverrun="select 1 from dual";
	assertEquals(check(&errcda,
			oparse(&errcda,(text *)neverrun,(sb4)-1,0,(ub4)2)),0);
	// oci8.cpp asserts ORA-24374 here, which OCI8's client raises for a
	// missing define ahead of the missing execute.  OCI7's client does not
	// raise 24374 and what it does raise instead is unverified, so only
	// the failure itself is asserted.  the number is printed so the run in
	// #9654 reveals it
	assertTrue(ofen(&errcda,1)!=0);
	stdoutput.printf("\nORA-%05d\n",errorCode(&errcda));
	stdoutput.printf("\n\n");


	// row 2 of this result set divides by zero.  oracle only evaluates
	// the expression as it produces each row, so how many rows the
	// connection pulls from the backend at a time - fetchatonce - decides
	// whether the failure lands on the execute or on a later fetch.  both
	// halves are covered, one instance each
	const char	*divzero="select 1/(level-2) from dual "
				"connect by level<=3";

	if (!isfetchatonce) {

		stdoutput.printf("oexec - error mid-fetch\n");
		// this result set only has 3 rows, well under the connection's
		// fetchatonce (10 by default here - see FETCH_AT_ONCE in
		// sqlrserverconnection.cpp), so the query3 protocol's inline
		// prefetch on execute pulls the whole result set in one
		// backend fetch - including the row 2 divide by zero, which
		// oracle only evaluates once it actually produces that row.
		// the error surfaces on the execute response here rather than
		// on a later, separate fetch, so this is sendQuery3Response()'s
		// fetchRow() error branch (#9585).  the sqlrelayfetchatonce
		// instance takes the other branch below
		assertEquals(check(&errcda,
				oparse(&errcda,(text *)divzero,
						(sb4)-1,0,(ub4)2)),0);
		assertTrue(oexec(&errcda)!=0);
		// ORA-01476, divisor is equal to zero
		assertEquals(errorCode(&errcda),1476);
		stdoutput.printf("\n\n");

	} else {

		stdoutput.printf("ofen - error mid-fetch\n");
		// the fetch-time counterpart of the case above (#9601).  this
		// instance's connection string sets fetchatonce=1, so the
		// connection asks the backend for one row per fetch, and the
		// execute's inline prefetch has to be held to one row as well
		// or the error lands there instead.  oci8.cpp does that with
		// OCI_ATTR_PREFETCH_ROWS=1 and OCI_ATTR_PREFETCH_MEMORY=0;
		// OCI7 has no such attributes, so the cursor is opened with an
		// array size of 1 and fetched one row at a time.  whether that
		// holds the prefetch the same way is unverified, see #9654
		Cda_Def	divzerocda;
		assertEquals(check(&divzerocda,openCursor(&divzerocda,1)),0);
		assertEquals(check(&divzerocda,
				oparse(&divzerocda,(text *)divzero,
						(sb4)-1,0,(ub4)2)),0);
		char	divzerovalue[64];
		sb2	divzeroind=0;
		ub2	divzerolen=0;
		ub2	divzerocode=0;
		bytestring::zero(divzerovalue,sizeof(divzerovalue));
		assertEquals(check(&divzerocda,
				odefin(&divzerocda,1,(ub1 *)divzerovalue,
					(sword)sizeof(divzerovalue),
					SQLT_STR,-1,&divzeroind,
					(text *)0,-1,-1,
					&divzerolen,&divzerocode)),0);
		assertEquals(check(&divzerocda,oexec(&divzerocda)),0);
		// row 1 is 1/(1-2), which evaluates fine
		assertEquals(check(&divzerocda,ofen(&divzerocda,1)),0);
		assertEquals((const char *)divzerovalue,"-1");
		// row 2 is 1/(2-2), which doesn't
		assertTrue(ofen(&divzerocda,1)!=0);
		// ORA-01476, divisor is equal to zero
		assertEquals(errorCode(&divzerocda),1476);
		assertEquals(check(&divzerocda,oclose(&divzerocda)),0);
		stdoutput.printf("\n\n");
	}


	stdoutput.printf("oclose - statement\n");
	assertEquals(check(&errcda,oclose(&errcda)),0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Teardown ==============\n\n");

	stdoutput.printf("drop table\n");
	assertEquals(execImmediate("drop table protocoltesttable"),0);
	assertEquals(execImmediate("drop table protocoltesttran"),0);
	assertEquals(execImmediate("drop table protocoltestbind"),0);
	assertEquals(execImmediate("drop table protocoltesttypes"),0);
	assertEquals(execImmediate("drop table protocoltestlong"),0);
	assertEquals(execImmediate("drop table protocoltestlongraw"),0);
	assertEquals(execImmediate("drop table protocoltestlob"),0);
	assertEquals(execImmediate("drop table protocoltestarray"),0);
	assertEquals(execImmediate("drop table protocoltestclr"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("oclose - main cursor\n");
	assertEquals(check(&cda,oclose(&cda)),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("ologof\n");
	// OCI8 needs OCISessionEnd, OCIServerDetach and five OCIHandleFrees to
	// get here; ologof is the whole of OCI7's teardown
	assertEquals(check(&lda,ologof(&lda)),0);
	stdoutput.printf("\n\n");


	reportTestStatus();
	return status;
}
