// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/process.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>
#include <config.h>

#include "asserts.cpp"

extern "C" {
	#define OCIVER_ORACLE
	#include <oci.h>
}

OCIError	*err=NULL;
OCIEnv		*env=NULL;
OCIServer	*srv=NULL;
OCISvcCtx	*svc=NULL;
OCISession	*session=NULL;
OCITrans	*trans=NULL;

const char	*sid=NULL;
const char	*badsid=NULL;
const char	*user="testuser";
const char	*password="testpassword";

// put a username and password on a session handle
static void setCredentials(OCISession *sess, const char *u, const char *p) {
	assertEquals(
		OCIAttrSet(sess,OCI_HTYPE_SESSION,
				(void *)u,charstring::getLength(u),
				OCI_ATTR_USERNAME,err),
		OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(sess,OCI_HTYPE_SESSION,
				(void *)p,charstring::getLength(p),
				OCI_ATTR_PASSWORD,err),
		OCI_SUCCESS);
}

// discard whatever diagnostic is currently sitting in the shared error
// handle.  OCIErrorGet doesn't clear what it reads, and a successful OCI
// call doesn't overwrite it either, so a tolerated failure (e.g. a
// pre-emptive "drop table" that's expected to fail when the table is
// already gone) can leave stale diagnostics behind for a later, unrelated
// failure to get blamed on.  allocate the replacement before freeing the
// old handle, so a failed alloc leaves err usable rather than dangling.
static void clearErrors() {
	OCIError	*newerr=NULL;
	if (OCIHandleAlloc(env,(void **)&newerr,
				OCI_HTYPE_ERROR,0,NULL)==OCI_SUCCESS) {
		OCIHandleFree(err,OCI_HTYPE_ERROR);
		err=newerr;
	}
}

// run a statement, discarding whatever it returns
static sword execImmediate(const char *query) {

	clearErrors();

	OCIStmt	*stmt=NULL;
	if (OCIHandleAlloc(env,(void **)&stmt,
				OCI_HTYPE_STMT,0,NULL)!=OCI_SUCCESS) {
		return OCI_ERROR;
	}

	sword	result=OCIStmtPrepare(stmt,err,
					(text *)query,
					charstring::getLength(query),
					OCI_NTV_SYNTAX,OCI_DEFAULT);
	if (result==OCI_SUCCESS) {
		result=OCIStmtExecute(svc,stmt,err,1,0,NULL,NULL,OCI_DEFAULT);
	}

	OCIHandleFree(stmt,OCI_HTYPE_STMT);

	return result;
}

// count the rows in a table, so a commit or rollback can be shown to have
// taken
static int countRows(const char *table) {

	clearErrors();

	char	query[256];
	charstring::printf(query,sizeof(query),
				"select count(*) from %s",table);

	OCIStmt	*stmt=NULL;
	if (OCIHandleAlloc(env,(void **)&stmt,
				OCI_HTYPE_STMT,0,NULL)!=OCI_SUCCESS) {
		return -1;
	}

	int	count=-1;
	if (OCIStmtPrepare(stmt,err,(text *)query,
				charstring::getLength(query),
				OCI_NTV_SYNTAX,OCI_DEFAULT)==OCI_SUCCESS) {

		// the define has to be in place before the execute, which
		// fetches the single row into it
		OCIDefine	*def=NULL;
		sb4		countbuffer=0;
		if (OCIDefineByPos(stmt,&def,err,1,
					&countbuffer,sizeof(countbuffer),
					SQLT_INT,NULL,NULL,NULL,
					OCI_DEFAULT)==OCI_SUCCESS &&
			OCIStmtExecute(svc,stmt,err,1,0,NULL,NULL,
					OCI_DEFAULT)==OCI_SUCCESS) {
			count=(int)countbuffer;
		}
	}

	OCIHandleFree(stmt,OCI_HTYPE_STMT);

	return count;
}

// the error code of the most recent failure
static sb4 errorCode() {
	text	message[1024];
	bytestring::zero(message,sizeof(message));
	sb4	errcode=0;
	OCIErrorGet(err,1,NULL,&errcode,
			message,sizeof(message),OCI_HTYPE_ERROR);
	return errcode;
}

// pin a result set column's metadata
static void assertColumn(OCIStmt *stmt, ub4 pos, const char *name,
					int type, int size,
					int precision, int scale) {

	OCIParam	*param=NULL;
	assertEquals(OCIParamGet(stmt,OCI_HTYPE_STMT,err,
					(void **)&param,pos),OCI_SUCCESS);

	// OCI_ATTR_NAME hands back a pointer into oracle's own buffer, with an
	// explicit length and no null terminator, so it has to be copied out
	// before it can be compared as a string
	text	*colname=NULL;
	ub4	colnamelen=0;
	assertEquals(OCIAttrGet(param,OCI_DTYPE_PARAM,
					&colname,&colnamelen,
					OCI_ATTR_NAME,err),OCI_SUCCESS);
	char	colnamebuf[128];
	bytestring::zero(colnamebuf,sizeof(colnamebuf));
	if (colnamelen<sizeof(colnamebuf)) {
		bytestring::copy(colnamebuf,colname,colnamelen);
	}
	assertEquals((const char *)colnamebuf,name);
	assertEquals((int)colnamelen,(int)charstring::getLength(name));

	ub2	coltype=0;
	assertEquals(OCIAttrGet(param,OCI_DTYPE_PARAM,
					&coltype,NULL,
					OCI_ATTR_DATA_TYPE,err),OCI_SUCCESS);
	assertEquals((int)coltype,type);

	ub2	colsize=0;
	assertEquals(OCIAttrGet(param,OCI_DTYPE_PARAM,
					&colsize,NULL,
					OCI_ATTR_DATA_SIZE,err),OCI_SUCCESS);
	assertEquals((int)colsize,size);

	// OCI_ATTR_PRECISION and OCI_ATTR_SCALE are documented as ub1 and sb1
	// for an implicit describe, and ub2/sb2 only for an explicit one.  This
	// is a describe off an executed statement, so implicit, but oracle
	// writes 2 bytes anyway.  Reading them into a ub1 and an sb1 smashes
	// whatever is next on the stack - it is what made the data size above
	// read back as 0 while this was being written.
	ub2	colprecision=0;
	assertEquals(OCIAttrGet(param,OCI_DTYPE_PARAM,
					&colprecision,NULL,
					OCI_ATTR_PRECISION,err),OCI_SUCCESS);
	assertEquals((int)colprecision,precision);

	sb2	colscale=0;
	assertEquals(OCIAttrGet(param,OCI_DTYPE_PARAM,
					&colscale,NULL,
					OCI_ATTR_SCALE,err),OCI_SUCCESS);
	assertEquals((int)colscale,scale);

	OCIDescriptorFree(param,OCI_DTYPE_PARAM);
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
	} else {
		sid=(issqlrelay)?"sqlrelay":"ora1";
		badsid=(issqlrelay)?"sqlrelaybad":"ora1bad";
	}

	environment::setValue("ORACLE_SID",sid);
	environment::setValue("TWO_TASK",sid);


	stdoutput.printf("\n=============== Connect ==============\n\n");

	stdoutput.printf("OCIEnvCreate\n");
	#ifdef HAVE_ORACLE_8i
		assertEquals(
			OCIEnvCreate((OCIEnv **)&env,
					OCI_DEFAULT|OCI_OBJECT,
					NULL,NULL,NULL,NULL,0,NULL),
			OCI_SUCCESS);
	#else
		assertEquals(
			OCIInitialize(OCI_DEFAULT,NULL,NULL,NULL,NULL),
			OCI_SUCCESS);
		assertEquals(
			OCIEnvInit((OCIEnv **)&env,OCI_DEFAULT,0,NULL),
			OCI_SUCCESS);
	#endif
	assertTrue(env!=NULL);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIHandleAlloc\n");
	assertEquals(
		OCIHandleAlloc(env,(void **)&err,OCI_HTYPE_ERROR,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIHandleAlloc(env,(void **)&srv,OCI_HTYPE_SERVER,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIHandleAlloc(env,(void **)&svc,OCI_HTYPE_SVCCTX,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIHandleAlloc(env,(void **)&session,OCI_HTYPE_SESSION,0,NULL),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIServerAttach\n");
	sword	attached=OCIServerAttach(srv,err,(text *)sid,
					charstring::getLength(sid),0);
	assertEquals(attached,OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(svc,OCI_HTYPE_SVCCTX,srv,0,OCI_ATTR_SERVER,err),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");

	// Nothing below here can work without an attachment.  Running on anyway
	// gets ORA-01012 from every call, and eventually a segfault inside OCI.
	if (attached!=OCI_SUCCESS) {
		reportTestStatus();
		return status;
	}


	stdoutput.printf("OCISessionBegin\n");
	setCredentials(session,user,password);
	sword	loggedin=OCISessionBegin(svc,err,session,
					OCI_CRED_RDBMS,OCI_DEFAULT);
	assertEquals(loggedin,OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(svc,OCI_HTYPE_SVCCTX,session,0,OCI_ATTR_SESSION,err),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");

	// same again - a session that never began leaves every statement
	// below failing with ORA-01012
	if (loggedin!=OCI_SUCCESS) {
		reportTestStatus();
		return status;
	}


	stdoutput.printf("OCIAttrSet - transaction handle\n");
	assertEquals(
		OCIHandleAlloc(env,(void **)&trans,OCI_HTYPE_TRANS,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(svc,OCI_HTYPE_SVCCTX,trans,0,OCI_ATTR_TRANS,err),
		OCI_SUCCESS);
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

	// a handle set of its own, so a failed login here cannot disturb the
	// session the rest of the test runs on
	OCIServer	*authsrv=NULL;
	OCISvcCtx	*authsvc=NULL;
	OCISession	*authsession=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&authsrv,OCI_HTYPE_SERVER,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIHandleAlloc(env,(void **)&authsvc,OCI_HTYPE_SVCCTX,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIHandleAlloc(env,(void **)&authsession,
					OCI_HTYPE_SESSION,0,NULL),
		OCI_SUCCESS);


	stdoutput.printf("OCIServerAttach - no such service\n");
	OCIServer	*badsrv=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&badsrv,OCI_HTYPE_SERVER,0,NULL),
		OCI_SUCCESS);
	sword	badattached=OCIServerAttach(badsrv,err,(text *)badsid,
					charstring::getLength(badsid),0);
	// #9310: the oracle protocol module now reads CONNECT_DATA's
	// SID/SERVICE_NAME and refuses an unconfigured one with ORA-12514,
	// same as native, so this runs the same assertion either way
	assertEquals(badattached,OCI_ERROR);
	// ORA-12514, the listener does not know the service being
	// asked for.  This one has to come back as a refusal from the
	// far end - a dropped socket gives ORA-12537 or ORA-03113
	// instead.
	assertEquals((int)errorCode(),12514);
	assertEquals(OCIHandleFree(badsrv,OCI_HTYPE_SERVER),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIServerAttach - no such alias\n");
	OCIServer	*noaliassrv=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&noaliassrv,
					OCI_HTYPE_SERVER,0,NULL),
		OCI_SUCCESS);
	const char	*noalias="nosuchalias";
	assertEquals(
		OCIServerAttach(noaliassrv,err,(text *)noalias,
				charstring::getLength(noalias),0),
		OCI_ERROR);
	// ORA-12154, resolved client side out of tnsnames.ora, so nothing
	// leaves the machine for this one
	assertEquals((int)errorCode(),12154);
	assertEquals(OCIHandleFree(noaliassrv,OCI_HTYPE_SERVER),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIServerAttach - for the login cases\n");
	assertEquals(
		OCIServerAttach(authsrv,err,(text *)sid,
				charstring::getLength(sid),0),
		OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(authsvc,OCI_HTYPE_SVCCTX,authsrv,0,
				OCI_ATTR_SERVER,err),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	// #9311: a failed login no longer drops the connection - the server
	// (native or sqlrelay) waits for another attempt on the same TCP
	// connection, up to a bound (3 by default, matching real Oracle's
	// SEC_MAX_FAILED_LOGIN_ATTEMPTS).  authsrv/authsvc are attached once,
	// above, and never detached/reattached below - that they still work
	// for the final, successful login is itself the connection-reuse
	// assertion.  The OCI_ATTR_SERVER check after each failure below
	// confirms the handle sqlrelay handed back is still attached to the
	// same server, not a fresh one from a reconnect.

	stdoutput.printf("OCISessionBegin - wrong password\n");
	setCredentials(authsession,user,"wrongpassword");
	assertEquals(
		OCISessionBegin(authsvc,err,authsession,
				OCI_CRED_RDBMS,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-01017, invalid username/password.  The attach above succeeded,
	// so this is the login being refused, not the connection failing.
	assertEquals((int)errorCode(),1017);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIAttrGet - server handle unchanged, same connection\n");
	OCIServer	*serverafterfailure=NULL;
	assertEquals(
		OCIAttrGet(authsvc,OCI_HTYPE_SVCCTX,&serverafterfailure,NULL,
				OCI_ATTR_SERVER,err),
		OCI_SUCCESS);
	// still the handle attached above - a torn-down connection would have
	// forced a fresh OCIServerAttach to get this far at all
	assertTrue(serverafterfailure==authsrv);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCISessionBegin - unknown user\n");
	setCredentials(authsession,"nosuchuser","nosuchpassword");
	assertEquals(
		OCISessionBegin(authsvc,err,authsession,
				OCI_CRED_RDBMS,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-01017 again.  Oracle gives the same error for an unknown user as
	// for a wrong password on purpose, so a client cannot tell which half
	// it got wrong.  Also: the ORA number tracks *why* this attempt was
	// wrong, not which attempt number this is - a connection silently
	// replaced with a fresh one wouldn't surface as a wrong ORA number
	// here, which is why the OCI_ATTR_SERVER check above matters more
	// than the error code alone.
	assertEquals((int)errorCode(),1017);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCISessionBegin - empty password\n");
	setCredentials(authsession,user,"");
	assertEquals(
		OCISessionBegin(authsvc,err,authsession,
				OCI_CRED_RDBMS,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-01005, login denied due to invalid password - a different error
	// from ORA-01017, and one the client raises before anything is sent,
	// so it doesn't count against the connection's login-attempt bound
	assertEquals((int)errorCode(),1005);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCISessionBegin - correct password, after the failures\n");
	setCredentials(authsession,user,password);
	assertEquals(
		OCISessionBegin(authsvc,err,authsession,
				OCI_CRED_RDBMS,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(authsvc,OCI_HTYPE_SVCCTX,authsession,0,
				OCI_ATTR_SESSION,err),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	// #9174: against sqlrelay the OCI client, in a portable encoding, gets
	// ORA-03113 on the first row-data message, so the query can't be run
	// through the recovered session here yet
	if (!issqlrelay) {

		stdoutput.printf("OCIStmtExecute - "
					"through the recovered session\n");
		// a login that succeeds has to be usable, not just accepted
		OCIStmt		*authstmt=NULL;
		assertEquals(
			OCIHandleAlloc(env,(void **)&authstmt,
						OCI_HTYPE_STMT,0,NULL),
			OCI_SUCCESS);
		const char	*authquery="select 'authenticated' from dual";
		assertEquals(
			OCIStmtPrepare(authstmt,err,(text *)authquery,
					charstring::getLength(authquery),
					OCI_NTV_SYNTAX,OCI_DEFAULT),
			OCI_SUCCESS);
		assertEquals(
			OCIStmtExecute(authsvc,authstmt,err,0,0,
						NULL,NULL,OCI_DEFAULT),
			OCI_SUCCESS);
		OCIDefine	*authdef=NULL;
		char		authfield[64];
		sb2		authind=0;
		bytestring::zero(authfield,sizeof(authfield));
		assertEquals(
			OCIDefineByPos(authstmt,&authdef,err,1,
					authfield,sizeof(authfield),SQLT_STR,
					&authind,NULL,NULL,OCI_DEFAULT),
			OCI_SUCCESS);
		assertEquals(
			OCIStmtFetch2(authstmt,err,1,
					OCI_FETCH_NEXT,0,OCI_DEFAULT),
			OCI_SUCCESS);
		assertEquals((const char *)authfield,"authenticated");
		assertEquals(OCIHandleFree(authstmt,OCI_HTYPE_STMT),
				OCI_SUCCESS);
		stdoutput.printf("\n\n");
	}


	stdoutput.printf("OCISessionEnd\n");
	assertEquals(
		OCISessionEnd(authsvc,err,authsession,OCI_DEFAULT),OCI_SUCCESS);
	assertEquals(OCIServerDetach(authsrv,err,OCI_DEFAULT),OCI_SUCCESS);
	assertEquals(OCIHandleFree(authsession,OCI_HTYPE_SESSION),OCI_SUCCESS);
	assertEquals(OCIHandleFree(authsvc,OCI_HTYPE_SVCCTX),OCI_SUCCESS);
	assertEquals(OCIHandleFree(authsrv,OCI_HTYPE_SERVER),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============ Server Version ==========\n\n");

	stdoutput.printf("OCIServerVersion\n");
	char	versionbuf[512];
	bytestring::zero(versionbuf,sizeof(versionbuf));
	assertEquals(
		OCIServerVersion(svc,err,
				(text *)versionbuf,sizeof(versionbuf),
				OCI_HTYPE_SVCCTX),
		OCI_SUCCESS);
	// the test configs all set serverversion="11.2", which the protocol
	// module packs as 0x0b200100 and expands into this exact banner
	assertEquals(versionbuf,
		"Oracle Database 11g Enterprise Edition "
		"Release 11.2.0.1.0 - 64bit Production");
	stdoutput.printf("\n%s\n",versionbuf);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=============== Schema ===============\n\n");

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
		OCI_SUCCESS);
	assertEquals(
		execImmediate("create table protocoltesttran ("
				"testnumber number(10))"),
		OCI_SUCCESS);
	assertEquals(countRows("protocoltesttable"),0);
	assertEquals(countRows("protocoltesttran"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("insert\n");
	assertEquals(
		execImmediate("insert into protocoltesttable values "
				"(1,'char1','varchar1',"
				"to_date('2001-01-01 01:01:01',"
					"'YYYY-MM-DD HH24:MI:SS'))"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltesttable values "
				"(2,'char2','varchar2',"
				"to_date('2002-02-02 02:02:02',"
					"'YYYY-MM-DD HH24:MI:SS'))"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltesttable values "
				"(3,NULL,NULL,NULL)"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(countRows("protocoltesttable"),3);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============= Statements =============\n\n");

	OCIStmt	*stmt=NULL;

	stdoutput.printf("OCIHandleAlloc - statement\n");
	assertEquals(
		OCIHandleAlloc(env,(void **)&stmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);
	assertTrue(stmt!=NULL);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtPrepare - select\n");
	const char	*query="select * from protocoltesttable "
					"order by testnumber";
	assertEquals(
		OCIStmtPrepare(stmt,err,
				(text *)query,charstring::getLength(query),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	ub2	stmttype=0;
	assertEquals(
		OCIAttrGet(stmt,OCI_HTYPE_STMT,
				&stmttype,NULL,OCI_ATTR_STMT_TYPE,err),
		OCI_SUCCESS);
	assertEquals(stmttype,OCI_STMT_SELECT);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - parse only\n");
	assertEquals(
		OCIStmtExecute(svc,stmt,err,0,0,NULL,NULL,OCI_PARSE_ONLY),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIAttrGet - statement type\n");
	OCIStmt	*typestmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&typestmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);
	const char	*insertquery="insert into protocoltesttran values (0)";
	assertEquals(
		OCIStmtPrepare(typestmt,err,(text *)insertquery,
				charstring::getLength(insertquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	stmttype=0;
	assertEquals(
		OCIAttrGet(typestmt,OCI_HTYPE_STMT,
				&stmttype,NULL,OCI_ATTR_STMT_TYPE,err),
		OCI_SUCCESS);
	assertEquals(stmttype,OCI_STMT_INSERT);
	const char	*updatequery="update protocoltesttran set testnumber=1";
	assertEquals(
		OCIStmtPrepare(typestmt,err,(text *)updatequery,
				charstring::getLength(updatequery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	stmttype=0;
	assertEquals(
		OCIAttrGet(typestmt,OCI_HTYPE_STMT,
				&stmttype,NULL,OCI_ATTR_STMT_TYPE,err),
		OCI_SUCCESS);
	assertEquals(stmttype,OCI_STMT_UPDATE);
	const char	*deletequery="delete from protocoltesttran";
	assertEquals(
		OCIStmtPrepare(typestmt,err,(text *)deletequery,
				charstring::getLength(deletequery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	stmttype=0;
	assertEquals(
		OCIAttrGet(typestmt,OCI_HTYPE_STMT,
				&stmttype,NULL,OCI_ATTR_STMT_TYPE,err),
		OCI_SUCCESS);
	assertEquals(stmttype,OCI_STMT_DELETE);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIAttrGet - affected row count\n");
	assertEquals(
		OCIStmtPrepare(typestmt,err,(text *)insertquery,
				charstring::getLength(insertquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,typestmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	ub4	affectedrows=0;
	assertEquals(
		OCIAttrGet(typestmt,OCI_HTYPE_STMT,
				&affectedrows,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)affectedrows,1);
	assertEquals(
		OCIStmtPrepare(typestmt,err,(text *)deletequery,
				charstring::getLength(deletequery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,typestmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	affectedrows=0;
	assertEquals(
		OCIAttrGet(typestmt,OCI_HTYPE_STMT,
				&affectedrows,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)affectedrows,1);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIHandleFree - statement\n");
	assertEquals(OCIHandleFree(typestmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Metadata ==============\n\n");

	stdoutput.printf("OCIStmtExecute - describe only\n");
	assertEquals(
		OCIStmtExecute(svc,stmt,err,0,0,NULL,NULL,OCI_DESCRIBE_ONLY),
		OCI_SUCCESS);
	ub4	ncols=0;
	assertEquals(
		OCIAttrGet(stmt,OCI_HTYPE_STMT,
				&ncols,NULL,OCI_ATTR_PARAM_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)ncols,4);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIParamGet - every column\n");
	assertColumn(stmt,1,"TESTNUMBER",SQLT_NUM,22,10,0);
	assertColumn(stmt,2,"TESTCHAR",SQLT_AFC,20,0,0);
	assertColumn(stmt,3,"TESTVARCHAR",SQLT_CHR,40,0,0);
	assertColumn(stmt,4,"TESTDATE",SQLT_DAT,7,0,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIParamGet - past the last column\n");
	OCIParam	*noparam=NULL;
	assertEquals(
		OCIParamGet(stmt,OCI_HTYPE_STMT,err,(void **)&noparam,5),
		OCI_ERROR);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=============== Fetch ================\n\n");

	stdoutput.printf("OCIStmtExecute\n");
	assertEquals(
		OCIStmtExecute(svc,stmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIDefineByPos - every column\n");
	OCIDefine	*def[4];
	char		number[64];
	char		charfield[64];
	char		varcharfield[64];
	char		datefield[64];
	sb2		ind[4];
	ub2		retlen[4];
	ub2		retcode[4];
	bytestring::zero(def,sizeof(def));
	bytestring::zero(ind,sizeof(ind));
	bytestring::zero(retlen,sizeof(retlen));
	bytestring::zero(retcode,sizeof(retcode));
	assertEquals(
		OCIDefineByPos(stmt,&def[0],err,1,
				number,sizeof(number),SQLT_STR,
				&ind[0],&retlen[0],&retcode[0],OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(stmt,&def[1],err,2,
				charfield,sizeof(charfield),SQLT_STR,
				&ind[1],&retlen[1],&retcode[1],OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(stmt,&def[2],err,3,
				varcharfield,sizeof(varcharfield),SQLT_STR,
				&ind[2],&retlen[2],&retcode[2],OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(stmt,&def[3],err,4,
				datefield,sizeof(datefield),SQLT_STR,
				&ind[3],&retlen[3],&retcode[3],OCI_DEFAULT),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - first row\n");
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)number,"1");
	// char columns come back blank padded to their declared width
	assertEquals((const char *)charfield,"char1               ");
	assertEquals((const char *)varcharfield,"varchar1");
	assertEquals((int)ind[0],0);
	assertEquals((int)ind[1],0);
	assertEquals((int)ind[2],0);
	assertEquals((int)ind[3],0);
	ub4	currentrow=0;
	assertEquals(
		OCIAttrGet(stmt,OCI_HTYPE_STMT,
				&currentrow,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)currentrow,1);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - second row\n");
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)number,"2");
	assertEquals((const char *)charfield,"char2               ");
	assertEquals((const char *)varcharfield,"varchar2");
	currentrow=0;
	assertEquals(
		OCIAttrGet(stmt,OCI_HTYPE_STMT,
				&currentrow,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)currentrow,2);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - nulls\n");
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)number,"3");
	// -1 is OCI_IND_NULL
	assertEquals((int)ind[0],0);
	assertEquals((int)ind[1],-1);
	assertEquals((int)ind[2],-1);
	assertEquals((int)ind[3],-1);
	assertEquals((int)retlen[1],0);
	assertEquals((int)retlen[2],0);
	assertEquals((int)retlen[3],0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - past the last row\n");
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_NO_DATA);
	currentrow=0;
	assertEquals(
		OCIAttrGet(stmt,OCI_HTYPE_STMT,
				&currentrow,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)currentrow,3);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - date\n");
	const char	*datequery="select testdate "
				"from protocoltesttable "
				"where testnumber=1";
	assertEquals(
		OCIStmtPrepare(stmt,err,(text *)datequery,
				charstring::getLength(datequery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,stmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	ub1	datebytes[7];
	bytestring::zero(datebytes,sizeof(datebytes));
	assertEquals(
		OCIDefineByPos(stmt,&def[0],err,1,
				datebytes,sizeof(datebytes),SQLT_DAT,
				&ind[0],&retlen[0],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
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


	// #9599 - a describe-only on a mid-fetch cursor must not rewind it.
	// note: OCI answers OCI_DESCRIBE_ONLY on an already-executed statement
	// from its own client-side cache, so this case never actually puts a
	// second describe on the wire - it doesn't exercise the server-side
	// guard, only the client-visible behavior it protects.
	// oracledescribeonly in this directory is the test that reaches the
	// guard itself: it builds the describe-only TTI_QUERY3 by hand and
	// puts it on the wire, with no OCI in the way
	stdoutput.printf("OCIStmtExecute - describe only, mid-fetch\n");
	OCIStmt	*midfetchstmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&midfetchstmt,
					OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtPrepare(midfetchstmt,err,
				(text *)query,charstring::getLength(query),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,midfetchstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	OCIDefine	*midfetchdef=NULL;
	char		midfetchnumber[64];
	sb2		midfetchind=0;
	ub2		midfetchretlen=0;
	ub2		midfetchretcode=0;
	bytestring::zero(midfetchnumber,sizeof(midfetchnumber));
	assertEquals(
		OCIDefineByPos(midfetchstmt,&midfetchdef,err,1,
				midfetchnumber,sizeof(midfetchnumber),SQLT_STR,
				&midfetchind,&midfetchretlen,&midfetchretcode,
				OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(midfetchstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)midfetchnumber,"1");
	assertEquals(
		OCIStmtExecute(svc,midfetchstmt,err,0,0,NULL,NULL,
						OCI_DESCRIBE_ONLY),
		OCI_SUCCESS);
	ub4	midfetchncols=0;
	assertEquals(
		OCIAttrGet(midfetchstmt,OCI_HTYPE_STMT,
				&midfetchncols,NULL,OCI_ATTR_PARAM_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)midfetchncols,4);
	assertColumn(midfetchstmt,1,"TESTNUMBER",SQLT_NUM,22,10,0);
	assertColumn(midfetchstmt,2,"TESTCHAR",SQLT_AFC,20,0,0);
	assertColumn(midfetchstmt,3,"TESTVARCHAR",SQLT_CHR,40,0,0);
	assertColumn(midfetchstmt,4,"TESTDATE",SQLT_DAT,7,0,0);
	// the fetch picks up where it left off, rather than back at row 1
	assertEquals(
		OCIStmtFetch2(midfetchstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)midfetchnumber,"2");
	assertEquals(
		OCIStmtFetch2(midfetchstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)midfetchnumber,"3");
	assertEquals(
		OCIStmtFetch2(midfetchstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_NO_DATA);
	assertEquals(OCIHandleFree(midfetchstmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=============== Binds ================\n\n");

	OCIStmt	*bindstmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&bindstmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);

	// a table of its own, so the fetch section's fixed three rows stay
	// where they are
	execImmediate("drop table protocoltestbind");
	assertEquals(
		execImmediate("create table protocoltestbind ("
				"testnumber number(10),"
				"testchar char(20),"
				"testvarchar varchar2(40))"),
		OCI_SUCCESS);

	const char	*bindinsert="insert into protocoltestbind "
					"(testnumber,testchar,testvarchar) "
					"values (:num,:chr,:vchr)";
	OCIBind		*bnd[3];
	sb4		bindnumber=0;
	char		bindchar[32];
	char		bindvarchar[64];
	sb2		bindind[3];
	bytestring::zero(bnd,sizeof(bnd));
	bytestring::zero(bindind,sizeof(bindind));


	stdoutput.printf("OCIBindByName\n");
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)bindinsert,
				charstring::getLength(bindinsert),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	bindnumber=10;
	charstring::copy(bindchar,"bindchar");
	charstring::copy(bindvarchar,"bindvarchar");
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":num",4,
				&bindnumber,sizeof(bindnumber),SQLT_INT,
				&bindind[0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[1],err,(text *)":chr",4,
				bindchar,sizeof(bindchar),SQLT_STR,
				&bindind[1],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[2],err,(text *)":vchr",5,
				bindvarchar,sizeof(bindvarchar),SQLT_STR,
				&bindind[2],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(countRows("protocoltestbind"),1);
	assertEquals(
		countRows("protocoltestbind where testnumber=10 and "
				"testvarchar='bindvarchar'"),1);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIBindByPos\n");
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)bindinsert,
				charstring::getLength(bindinsert),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	bindnumber=20;
	charstring::copy(bindchar,"poschar");
	charstring::copy(bindvarchar,"posvarchar");
	assertEquals(
		OCIBindByPos(bindstmt,&bnd[0],err,1,
				&bindnumber,sizeof(bindnumber),SQLT_INT,
				&bindind[0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByPos(bindstmt,&bnd[1],err,2,
				bindchar,sizeof(bindchar),SQLT_STR,
				&bindind[1],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByPos(bindstmt,&bnd[2],err,3,
				bindvarchar,sizeof(bindvarchar),SQLT_STR,
				&bindind[2],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		countRows("protocoltestbind where testnumber=20 and "
				"testvarchar='posvarchar'"),1);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIBindByName - null binds\n");
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)bindinsert,
				charstring::getLength(bindinsert),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	bindnumber=30;
	// the buffers still hold the values from the case above, so a null
	// really has to come from the indicator, not from an empty buffer
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":num",4,
				&bindnumber,sizeof(bindnumber),SQLT_INT,
				&bindind[0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[1],err,(text *)":chr",4,
				bindchar,sizeof(bindchar),SQLT_STR,
				&bindind[1],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[2],err,(text *)":vchr",5,
				bindvarchar,sizeof(bindvarchar),SQLT_STR,
				&bindind[2],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	bindind[1]=OCI_IND_NULL;
	bindind[2]=OCI_IND_NULL;
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		countRows("protocoltestbind where testnumber=30 and "
				"testchar is null and testvarchar is null"),1);
	bindind[1]=OCI_IND_NOTNULL;
	bindind[2]=OCI_IND_NOTNULL;
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - bind once, execute many\n");
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)bindinsert,
				charstring::getLength(bindinsert),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	charstring::copy(bindchar,"manychar");
	charstring::copy(bindvarchar,"manyvarchar");
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":num",4,
				&bindnumber,sizeof(bindnumber),SQLT_INT,
				&bindind[0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[1],err,(text *)":chr",4,
				bindchar,sizeof(bindchar),SQLT_STR,
				&bindind[1],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[2],err,(text *)":vchr",5,
				bindvarchar,sizeof(bindvarchar),SQLT_STR,
				&bindind[2],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	// the binds stay put, only the buffer changes between executes
	for (sb4 i=0; i<3; i++) {
		bindnumber=40+i;
		assertEquals(
			OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,
					OCI_DEFAULT),
			OCI_SUCCESS);
	}
	assertEquals(
		countRows("protocoltestbind where testnumber between 40 and 42"),
		3);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - array bind\n");
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)bindinsert,
				charstring::getLength(bindinsert),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	sb4	arrnumber[3];
	char	arrchar[3][32];
	char	arrvarchar[3][64];
	sb2	arrind[3][3];
	bytestring::zero(arrind,sizeof(arrind));
	for (int i=0; i<3; i++) {
		arrnumber[i]=50+i;
		charstring::printf(arrchar[i],sizeof(arrchar[i]),
					"arrchar%d",i);
		charstring::printf(arrvarchar[i],sizeof(arrvarchar[i]),
					"arrvarchar%d",i);
	}
	// value_sz is the size of one element - oracle strides the array by it
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":num",4,
				arrnumber,sizeof(arrnumber[0]),SQLT_INT,
				&arrind[0][0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[1],err,(text *)":chr",4,
				arrchar,sizeof(arrchar[0]),SQLT_STR,
				&arrind[1][0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIBindByName(bindstmt,&bnd[2],err,(text *)":vchr",5,
				arrvarchar,sizeof(arrvarchar[0]),SQLT_STR,
				&arrind[2][0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,3,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	ub4	arrayrows=0;
	assertEquals(
		OCIAttrGet(bindstmt,OCI_HTYPE_STMT,
				&arrayrows,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)arrayrows,3);
	assertEquals(
		countRows("protocoltestbind where testnumber between 50 and 52"),
		3);
	assertEquals(
		countRows("protocoltestbind where testvarchar='arrvarchar1'"),1);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIBindByName - output bind\n");
	const char	*outblock="begin select count(*) into :cnt "
					"from protocoltestbind; end;";
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)outblock,
				charstring::getLength(outblock),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	stmttype=0;
	assertEquals(
		OCIAttrGet(bindstmt,OCI_HTYPE_STMT,
				&stmttype,NULL,OCI_ATTR_STMT_TYPE,err),
		OCI_SUCCESS);
	assertEquals(stmttype,OCI_STMT_BEGIN);
	sb4	outcount=0;
	sb2	outind=0;
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":cnt",4,
				&outcount,sizeof(outcount),SQLT_INT,
				&outind,NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)outcount,countRows("protocoltestbind"));
	assertEquals((int)outind,OCI_IND_NOTNULL);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIBindByName - in-out bind\n");
	const char	*inoutblock="begin :v := :v * 2; end;";
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)inoutblock,
				charstring::getLength(inoutblock),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	sb4	inoutvalue=21;
	sb2	inoutind=0;
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":v",2,
				&inoutvalue,sizeof(inoutvalue),SQLT_INT,
				&inoutind,NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)inoutvalue,42);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIBindByName - output bind, null\n");
	const char	*nulloutblock="begin :v := NULL; end;";
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)nulloutblock,
				charstring::getLength(nulloutblock),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	inoutvalue=99;
	inoutind=OCI_IND_NOTNULL;
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":v",2,
				&inoutvalue,sizeof(inoutvalue),SQLT_INT,
				&inoutind,NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)inoutind,OCI_IND_NULL);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - no such placeholder\n");
	assertEquals(
		OCIStmtPrepare(bindstmt,err,(text *)bindinsert,
				charstring::getLength(bindinsert),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	// OCIStmtPrepare parses the placeholders client side, so the bind is
	// what catches this, not the execute
	assertEquals(
		OCIBindByName(bindstmt,&bnd[0],err,(text *)":nosuchbind",11,
				&bindnumber,sizeof(bindnumber),SQLT_INT,
				&bindind[0],NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-01036, illegal variable name/number
	assertEquals((int)errorCode(),1036);
	// and with nothing bound, the execute cannot go either
	assertEquals(
		OCIStmtExecute(svc,bindstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-01008, not all variables bound
	assertEquals((int)errorCode(),1008);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIHandleFree - statement\n");
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(OCIHandleFree(bindstmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============= Datatypes ==============\n\n");

	OCIStmt	*typestmt2=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&typestmt2,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);

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
		OCI_SUCCESS);
	// only one LONG column is allowed per table, so those get their own
	assertEquals(
		execImmediate("create table protocoltestlong "
				"(testlong long)"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("create table protocoltestlongraw "
				"(testlongraw long raw)"),
		OCI_SUCCESS);
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
		OCI_SUCCESS);
	// a rowid has to come from somewhere real, so borrow one
	assertEquals(
		execImmediate("update protocoltesttypes set testrowid="
				"(select max(rowid) from protocoltesttable)"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltestlong values "
				"('long value')"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltestlongraw values "
				"(hextoraw('0a0b0c0d0e'))"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - describe every type\n");
	const char	*typequery="select * from protocoltesttypes";
	assertEquals(
		OCIStmtPrepare(typestmt2,err,(text *)typequery,
				charstring::getLength(typequery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,typestmt2,err,0,0,NULL,NULL,
				OCI_DESCRIBE_ONLY),
		OCI_SUCCESS);
	ub4	typecols=0;
	assertEquals(
		OCIAttrGet(typestmt2,OCI_HTYPE_STMT,
				&typecols,NULL,OCI_ATTR_PARAM_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)typecols,10);
	assertColumn(typestmt2,1,"TESTVARCHAR",SQLT_CHR,40,0,0);
	assertColumn(typestmt2,2,"TESTNUMBER",SQLT_NUM,22,10,2);
	assertColumn(typestmt2,3,"TESTDATE",SQLT_DAT,7,0,0);
	assertColumn(typestmt2,4,"TESTRAW",SQLT_BIN,20,0,0);
	assertColumn(typestmt2,5,"TESTCHAR",SQLT_AFC,20,0,0);
	assertColumn(typestmt2,6,"TESTROWID",SQLT_RDD,8,0,0);
	// The module calls these 180 through 183, at
	// src/protocols/oracle.cpp:145-148, and that is right on the wire.  By
	// the time a describe reaches the client, OCI has already mapped them
	// to the SQLT_ codes below.
	assertColumn(typestmt2,7,"TESTTIMESTAMP",SQLT_TIMESTAMP,11,0,6);
	assertColumn(typestmt2,8,"TESTTIMESTAMPTZ",SQLT_TIMESTAMP_TZ,13,0,6);
	assertColumn(typestmt2,9,"TESTINTERVALYM",SQLT_INTERVAL_YM,5,2,0);
	assertColumn(typestmt2,10,"TESTINTERVALDS",SQLT_INTERVAL_DS,11,2,6);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIDefineByPos - every type, matching SQLT code\n");
	assertEquals(
		OCIStmtExecute(svc,typestmt2,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);

	OCIDefine	*typedef_[10];
	sb2		typeind[10];
	ub2		typelen[10];
	char		typevarchar[64];
	OCINumber	typenumber;
	ub1		typedate[7];
	ub1		typeraw[32];
	char		typechar[64];
	OCIRowid	*typerowid=NULL;
	OCIDateTime	*typetimestamp=NULL;
	OCIDateTime	*typetimestamptz=NULL;
	OCIInterval	*typeintervalym=NULL;
	OCIInterval	*typeintervalds=NULL;
	bytestring::zero(typedef_,sizeof(typedef_));
	bytestring::zero(typeind,sizeof(typeind));
	bytestring::zero(typelen,sizeof(typelen));
	bytestring::zero(typevarchar,sizeof(typevarchar));
	bytestring::zero(&typenumber,sizeof(typenumber));
	bytestring::zero(typedate,sizeof(typedate));
	bytestring::zero(typeraw,sizeof(typeraw));
	bytestring::zero(typechar,sizeof(typechar));

	// the descriptor types each need allocating before they can be
	// defined into
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&typerowid,
					OCI_DTYPE_ROWID,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&typetimestamp,
					OCI_DTYPE_TIMESTAMP,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&typetimestamptz,
					OCI_DTYPE_TIMESTAMP_TZ,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&typeintervalym,
					OCI_DTYPE_INTERVAL_YM,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&typeintervalds,
					OCI_DTYPE_INTERVAL_DS,0,NULL),
		OCI_SUCCESS);

	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[0],err,1,
				typevarchar,sizeof(typevarchar),SQLT_CHR,
				&typeind[0],&typelen[0],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[1],err,2,
				&typenumber,sizeof(typenumber),SQLT_VNU,
				&typeind[1],&typelen[1],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[2],err,3,
				typedate,sizeof(typedate),SQLT_DAT,
				&typeind[2],&typelen[2],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[3],err,4,
				typeraw,sizeof(typeraw),SQLT_BIN,
				&typeind[3],&typelen[3],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[4],err,5,
				typechar,sizeof(typechar),SQLT_AFC,
				&typeind[4],&typelen[4],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[5],err,6,
				&typerowid,0,SQLT_RDD,
				&typeind[5],&typelen[5],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[6],err,7,
				&typetimestamp,0,SQLT_TIMESTAMP,
				&typeind[6],&typelen[6],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[7],err,8,
				&typetimestamptz,0,SQLT_TIMESTAMP_TZ,
				&typeind[7],&typelen[7],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[8],err,9,
				&typeintervalym,0,SQLT_INTERVAL_YM,
				&typeind[8],&typelen[8],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[9],err,10,
				&typeintervalds,0,SQLT_INTERVAL_DS,
				&typeind[9],&typelen[9],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - every type\n");
	assertEquals(
		OCIStmtFetch2(typestmt2,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	for (int i=0; i<10; i++) {
		assertEquals((int)typeind[i],OCI_IND_NOTNULL);
	}
	stdoutput.printf("\n\n");


	stdoutput.printf("varchar, number, date, raw, char\n");
	// SQLT_CHR does not null terminate, so the length is what says where
	// the value stops
	assertEquals((int)typelen[0],13);
	assertEquals((const char *)typevarchar,"varchar value");
	// an oracle number only becomes a C number through OCINumberToReal
	double	numbervalue=0.0;
	assertEquals(
		OCINumberToReal(err,&typenumber,sizeof(numbervalue),
				&numbervalue),
		OCI_SUCCESS);
	assertEquals((int)(numbervalue*100),12345);
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
	char	rowidbuf[64];
	ub2	rowidlen=sizeof(rowidbuf);
	bytestring::zero(rowidbuf,sizeof(rowidbuf));
	assertEquals(
		OCIRowidToChar(typerowid,(text *)rowidbuf,&rowidlen,err),
		OCI_SUCCESS);
	// the base 64 external form of a rowid is always 18 characters
	assertEquals((int)rowidlen,18);
	assertEquals((int)charstring::getLength(rowidbuf),18);
	stdoutput.printf("\n\n");


	stdoutput.printf("timestamp\n");
	sb2	tsyear=0;
	ub1	tsmonth=0;
	ub1	tsday=0;
	ub1	tshour=0;
	ub1	tsminute=0;
	ub1	tssecond=0;
	ub4	tsfsecond=0;
	assertEquals(
		OCIDateTimeGetDate(env,err,typetimestamp,
					&tsyear,&tsmonth,&tsday),
		OCI_SUCCESS);
	assertEquals(
		OCIDateTimeGetTime(env,err,typetimestamp,
					&tshour,&tsminute,&tssecond,&tsfsecond),
		OCI_SUCCESS);
	assertEquals((int)tsyear,2004);
	assertEquals((int)tsmonth,4);
	assertEquals((int)tsday,4);
	assertEquals((int)tshour,4);
	assertEquals((int)tsminute,4);
	assertEquals((int)tssecond,4);
	assertEquals((int)tsfsecond,444444000);
	stdoutput.printf("\n\n");


	stdoutput.printf("timestamp with time zone\n");
	sb2	tzyear=0;
	ub1	tzmonth=0;
	ub1	tzday=0;
	sb1	tzhouroffset=0;
	sb1	tzminuteoffset=0;
	assertEquals(
		OCIDateTimeGetDate(env,err,typetimestamptz,
					&tzyear,&tzmonth,&tzday),
		OCI_SUCCESS);
	assertEquals(
		OCIDateTimeGetTimeZoneOffset(env,err,typetimestamptz,
					&tzhouroffset,&tzminuteoffset),
		OCI_SUCCESS);
	assertEquals((int)tzyear,2005);
	assertEquals((int)tzmonth,5);
	assertEquals((int)tzday,5);
	assertEquals((int)tzhouroffset,-5);
	assertEquals((int)tzminuteoffset,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("interval year to month\n");
	sb4	ymyear=0;
	sb4	ymmonth=0;
	assertEquals(
		OCIIntervalGetYearMonth(env,err,&ymyear,&ymmonth,
					typeintervalym),
		OCI_SUCCESS);
	assertEquals((int)ymyear,1);
	assertEquals((int)ymmonth,2);
	stdoutput.printf("\n\n");


	stdoutput.printf("interval day to second\n");
	sb4	dsday=0;
	sb4	dshour=0;
	sb4	dsminute=0;
	sb4	dssecond=0;
	sb4	dsfsecond=0;
	assertEquals(
		OCIIntervalGetDaySecond(env,err,&dsday,&dshour,
					&dsminute,&dssecond,&dsfsecond,
					typeintervalds),
		OCI_SUCCESS);
	assertEquals((int)dsday,3);
	assertEquals((int)dshour,4);
	assertEquals((int)dsminute,5);
	assertEquals((int)dssecond,6);
	assertEquals((int)dsfsecond,777777000);
	stdoutput.printf("\n\n");


	stdoutput.printf("long\n");
	const char	*longquery="select testlong from protocoltestlong";
	assertEquals(
		OCIStmtPrepare(typestmt2,err,(text *)longquery,
				charstring::getLength(longquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,typestmt2,err,0,0,NULL,NULL,
				OCI_DESCRIBE_ONLY),
		OCI_SUCCESS);
	// a long has no declared width, so the describe reports size 0
	assertColumn(typestmt2,1,"TESTLONG",SQLT_LNG,0,0,0);
	assertEquals(
		OCIStmtExecute(svc,typestmt2,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	char	longvalue[4096];
	sb2	longind=0;
	ub2	longlen=0;
	OCIDefine	*longdef=NULL;
	bytestring::zero(longvalue,sizeof(longvalue));
	assertEquals(
		OCIDefineByPos(typestmt2,&longdef,err,1,
				longvalue,sizeof(longvalue),SQLT_LNG,
				&longind,&longlen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(typestmt2,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)longind,OCI_IND_NOTNULL);
	assertEquals((int)longlen,10);
	assertEquals((const char *)longvalue,"long value");
	stdoutput.printf("\n\n");


	stdoutput.printf("long raw\n");
	const char	*longrawquery="select testlongraw "
					"from protocoltestlongraw";
	assertEquals(
		OCIStmtPrepare(typestmt2,err,(text *)longrawquery,
				charstring::getLength(longrawquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,typestmt2,err,0,0,NULL,NULL,
				OCI_DESCRIBE_ONLY),
		OCI_SUCCESS);
	assertColumn(typestmt2,1,"TESTLONGRAW",SQLT_LBI,0,0,0);
	assertEquals(
		OCIStmtExecute(svc,typestmt2,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	ub1	longrawvalue[4096];
	sb2	longrawind=0;
	ub2	longrawlen=0;
	OCIDefine	*longrawdef=NULL;
	bytestring::zero(longrawvalue,sizeof(longrawvalue));
	assertEquals(
		OCIDefineByPos(typestmt2,&longrawdef,err,1,
				longrawvalue,sizeof(longrawvalue),SQLT_LBI,
				&longrawind,&longrawlen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(typestmt2,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)longrawind,OCI_IND_NOTNULL);
	assertEquals((int)longrawlen,5);
	assertEquals((int)longrawvalue[0],10);
	assertEquals((int)longrawvalue[1],11);
	assertEquals((int)longrawvalue[2],12);
	assertEquals((int)longrawvalue[3],13);
	assertEquals((int)longrawvalue[4],14);
	stdoutput.printf("\n\n");


	stdoutput.printf("nulls, every type\n");
	assertEquals(
		execImmediate("delete from protocoltesttypes"),OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltesttypes "
				"(testvarchar) values (NULL)"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(
		OCIStmtPrepare(typestmt2,err,(text *)typequery,
				charstring::getLength(typequery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,typestmt2,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	bytestring::zero(typeind,sizeof(typeind));
	bytestring::zero(typelen,sizeof(typelen));
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[0],err,1,
				typevarchar,sizeof(typevarchar),SQLT_CHR,
				&typeind[0],&typelen[0],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[1],err,2,
				&typenumber,sizeof(typenumber),SQLT_VNU,
				&typeind[1],&typelen[1],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[2],err,3,
				typedate,sizeof(typedate),SQLT_DAT,
				&typeind[2],&typelen[2],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(typestmt2,&typedef_[6],err,7,
				&typetimestamp,0,SQLT_TIMESTAMP,
				&typeind[6],&typelen[6],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(typestmt2,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)typeind[0],OCI_IND_NULL);
	assertEquals((int)typeind[1],OCI_IND_NULL);
	assertEquals((int)typeind[2],OCI_IND_NULL);
	assertEquals((int)typeind[6],OCI_IND_NULL);
	assertEquals((int)typelen[0],0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIDescriptorFree\n");
	assertEquals(
		OCIDescriptorFree(typerowid,OCI_DTYPE_ROWID),OCI_SUCCESS);
	assertEquals(
		OCIDescriptorFree(typetimestamp,OCI_DTYPE_TIMESTAMP),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorFree(typetimestamptz,OCI_DTYPE_TIMESTAMP_TZ),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorFree(typeintervalym,OCI_DTYPE_INTERVAL_YM),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorFree(typeintervalds,OCI_DTYPE_INTERVAL_DS),
		OCI_SUCCESS);
	assertEquals(OCIHandleFree(typestmt2,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============ Long-form CLR ===========\n\n");

	// #9594 - a value over CLR_MAX_SHORT_LENGTH (252 bytes,
	// src/protocols/oracle.cpp:366) goes out from the server using
	// oracle's long-form CLR framing: a 0xfe marker, a run of
	// length-prefixed chunks, then a zero-length chunk to end it.
	// These cases round-trip values right at that boundary, then well
	// past it, to catch a wrong chunk length or a wrong chunk boundary
	// in putLenBytes() (src/protocols/oracle.cpp:4811).

	OCIStmt	*clrstmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&clrstmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);

	stdoutput.printf("create table\n");
	execImmediate("drop table protocoltestclr");
	assertEquals(
		execImmediate("create table protocoltestclr "
				"(testvarchar varchar2(4000))"),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	const char	*clrquery="select testvarchar from protocoltestclr";
	char		clrvarchar[4096];
	sb2		clrind=0;
	ub2		clrlen=0;
	OCIDefine	*clrdef=NULL;

	stdoutput.printf("select - 252 bytes, at the short-form boundary\n");
	assertEquals(
		execImmediate("insert into protocoltestclr values "
				"(rpad('A',252,'A'))"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	char	clr252[253];
	bytestring::zero(clr252,sizeof(clr252));
	for (ub4 i=0; i<252; i++) {
		clr252[i]='A';
	}
	assertEquals(
		OCIStmtPrepare(clrstmt,err,(text *)clrquery,
				charstring::getLength(clrquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,clrstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	bytestring::zero(clrvarchar,sizeof(clrvarchar));
	assertEquals(
		OCIDefineByPos(clrstmt,&clrdef,err,1,
				clrvarchar,sizeof(clrvarchar),SQLT_CHR,
				&clrind,&clrlen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(clrstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)clrind,OCI_IND_NOTNULL);
	assertEquals((int)clrlen,252);
	assertEquals((const char *)clrvarchar,(const char *)clr252);
	stdoutput.printf("\n\n");


	stdoutput.printf("select - 253 bytes, just into long form\n");
	assertEquals(
		execImmediate("delete from protocoltestclr"),OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltestclr values "
				"(rpad('B',253,'B'))"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	char	clr253[254];
	bytestring::zero(clr253,sizeof(clr253));
	for (ub4 i=0; i<253; i++) {
		clr253[i]='B';
	}
	// re-prepare before re-executing an already-fetched-to-completion
	// cursor - executing it again as-is sends a different tti function
	// (an oracle re-execute/resync call) that this module doesn't
	// implement yet, unrelated to #9594's chunk framing
	assertEquals(
		OCIStmtPrepare(clrstmt,err,(text *)clrquery,
				charstring::getLength(clrquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,clrstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	bytestring::zero(clrvarchar,sizeof(clrvarchar));
	assertEquals(
		OCIDefineByPos(clrstmt,&clrdef,err,1,
				clrvarchar,sizeof(clrvarchar),SQLT_CHR,
				&clrind,&clrlen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(clrstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)clrind,OCI_IND_NOTNULL);
	assertEquals((int)clrlen,253);
	assertEquals((const char *)clrvarchar,(const char *)clr253);
	stdoutput.printf("\n\n");


	stdoutput.printf("select - 1000 bytes, several long-form chunks\n");
	assertEquals(
		execImmediate("delete from protocoltestclr"),OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltestclr values "
				"(rpad('C',1000,'C'))"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	char	clr1000[1001];
	bytestring::zero(clr1000,sizeof(clr1000));
	for (ub4 i=0; i<1000; i++) {
		clr1000[i]='C';
	}
	// re-prepare - see the same note above the 253-byte case
	assertEquals(
		OCIStmtPrepare(clrstmt,err,(text *)clrquery,
				charstring::getLength(clrquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,clrstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	bytestring::zero(clrvarchar,sizeof(clrvarchar));
	assertEquals(
		OCIDefineByPos(clrstmt,&clrdef,err,1,
				clrvarchar,sizeof(clrvarchar),SQLT_CHR,
				&clrind,&clrlen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(clrstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)clrind,OCI_IND_NOTNULL);
	assertEquals((int)clrlen,1000);
	assertEquals((const char *)clrvarchar,(const char *)clr1000);
	stdoutput.printf("\n\n");


	// coverage stops at 1000 bytes.  getting a larger value into a
	// column needs either a bind over 255 bytes or a lob write, and
	// neither is implemented yet - see #9587 and #9589
	assertEquals(OCIHandleFree(clrstmt,OCI_HTYPE_STMT),OCI_SUCCESS);
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

	OCIStmt	*bigstmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&bigstmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);

	stdoutput.printf("long - 300 bytes, putLongBytes' long form\n");
	assertEquals(
		execImmediate("delete from protocoltestlong"),OCI_SUCCESS);
	char	longbig[301];
	for (ub4 i=0; i<300; i++) {
		longbig[i]=(char)('0'+(i%10));
	}
	longbig[300]='\0';
	char	longbiginsert[350];
	charstring::printf(longbiginsert,sizeof(longbiginsert),
			"insert into protocoltestlong values ('%s')",
			longbig);
	assertEquals(execImmediate(longbiginsert),OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(
		OCIStmtPrepare(bigstmt,err,(text *)longquery,
				charstring::getLength(longquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bigstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	char	longbigvalue[4096];
	sb2	longbigind=0;
	ub2	longbiglen=0;
	OCIDefine	*longbigdef=NULL;
	bytestring::zero(longbigvalue,sizeof(longbigvalue));
	assertEquals(
		OCIDefineByPos(bigstmt,&longbigdef,err,1,
				longbigvalue,sizeof(longbigvalue),SQLT_LNG,
				&longbigind,&longbiglen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(bigstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)longbigind,OCI_IND_NOTNULL);
	assertEquals((int)longbiglen,300);
	assertEquals((const char *)longbigvalue,(const char *)longbig);
	stdoutput.printf("\n\n");


	stdoutput.printf("long raw - 300 bytes, putLongBytes' long form\n");
	assertEquals(
		execImmediate("delete from protocoltestlongraw"),
		OCI_SUCCESS);
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
	assertEquals(execImmediate(longrawbiginsert),OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(
		OCIStmtPrepare(bigstmt,err,(text *)longrawquery,
				charstring::getLength(longrawquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bigstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	ub1	longrawbigvalue[4096];
	sb2	longrawbigind=0;
	ub2	longrawbiglen=0;
	OCIDefine	*longrawbigdef=NULL;
	bytestring::zero(longrawbigvalue,sizeof(longrawbigvalue));
	assertEquals(
		OCIDefineByPos(bigstmt,&longrawbigdef,err,1,
				longrawbigvalue,sizeof(longrawbigvalue),
				SQLT_LBI,&longrawbigind,&longrawbiglen,
				NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(bigstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)longrawbigind,OCI_IND_NOTNULL);
	assertEquals((int)longrawbiglen,300);
	assertTrue(!bytestring::compare(longrawbigvalue,longrawbig,
						sizeof(longrawbig)));
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIBindByName - output bind, 300 bytes, "
				"putOutBindValues via putLenBytes\n");
	const char	*outbindbigblock="begin :v := rpad('Q',300,'Q'); "
						"end;";
	assertEquals(
		OCIStmtPrepare(bigstmt,err,(text *)outbindbigblock,
				charstring::getLength(outbindbigblock),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	char	outbindbigvalue[512];
	sb2	outbindbigind=0;
	OCIBind	*outbindbigbnd=NULL;
	bytestring::zero(outbindbigvalue,sizeof(outbindbigvalue));
	assertEquals(
		OCIBindByName(bigstmt,&outbindbigbnd,err,(text *)":v",2,
				outbindbigvalue,sizeof(outbindbigvalue),
				SQLT_STR,&outbindbigind,NULL,NULL,0,NULL,
				OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,bigstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)outbindbigind,OCI_IND_NOTNULL);
	char	outbindbigexpected[301];
	for (ub4 i=0; i<300; i++) {
		outbindbigexpected[i]='Q';
	}
	outbindbigexpected[300]='\0';
	assertEquals((const char *)outbindbigvalue,
				(const char *)outbindbigexpected);
	stdoutput.printf("\n\n");


	assertEquals(OCIHandleFree(bigstmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n================ Lobs ================\n\n");

	OCIStmt	*lobstmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&lobstmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);

	// the read side of a lob operation is answered for real and the write
	// side is refused with ORA-03001, so everything the reads below check
	// is seeded through plain sql rather than through OCILobWrite - #9589
	const ub4	biglength=100000;
	char		*bigvalue=new char[biglength+1];
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
		OCI_SUCCESS);
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
	assertEquals(execImmediate(lobinsert),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("seed the clob\n");
	// 100000 characters is past what one sql literal holds and well past
	// what one network round trip carries, so the reads below get a lob
	// that has to come back in pieces.  the same 25 characters over and
	// over, which is what bigvalue holds
	assertEquals(
		execImmediate("declare "
				"c clob; "
			"begin "
				"dbms_lob.createtemporary(c,true); "
				"for i in 1..4000 loop "
					"dbms_lob.writeappend(c,25,"
					"'abcdefghijklmnopqrstuvwxy'); "
				"end loop; "
				"update protocoltestlob set testclob=c; "
				"dbms_lob.freetemporary(c); "
			"end;"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testclob)=100000"),1);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testblob)=512"),1);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - describe the lob columns\n");
	const char	*lobquery="select testclob,testblob,testbfile "
					"from protocoltestlob for update";
	assertEquals(
		OCIStmtPrepare(lobstmt,err,(text *)lobquery,
				charstring::getLength(lobquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,lobstmt,err,0,0,NULL,NULL,OCI_DESCRIBE_ONLY),
		OCI_SUCCESS);
	assertColumn(lobstmt,1,"TESTCLOB",SQLT_CLOB,4000,0,0);
	assertColumn(lobstmt,2,"TESTBLOB",SQLT_BLOB,4000,0,0);
	assertColumn(lobstmt,3,"TESTBFILE",SQLT_BFILEE,530,0,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIDefineByPos - lob locators\n");
	OCILobLocator	*cloblocator=NULL;
	OCILobLocator	*bloblocator=NULL;
	OCILobLocator	*bfilelocator=NULL;
	OCIDefine	*lobdef[3];
	sb2		lobind[3];
	bytestring::zero(lobdef,sizeof(lobdef));
	bytestring::zero(lobind,sizeof(lobind));
	// a clob and a blob take an OCI_DTYPE_LOB locator, a bfile takes an
	// OCI_DTYPE_FILE one
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&cloblocator,
					OCI_DTYPE_LOB,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&bloblocator,
					OCI_DTYPE_LOB,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIDescriptorAlloc(env,(void **)&bfilelocator,
					OCI_DTYPE_FILE,0,NULL),
		OCI_SUCCESS);
	// the defines go in before the execute, the way countRows() does it
	// and the way OCI's own documentation writes a lob select.  measured:
	// defining after the execute leaves the descriptors empty and every
	// lob call on them returns OCI_INVALID_HANDLE, whatever the server
	// sends
	assertEquals(
		OCIDefineByPos(lobstmt,&lobdef[0],err,1,
				&cloblocator,0,SQLT_CLOB,
				&lobind[0],NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(lobstmt,&lobdef[1],err,2,
				&bloblocator,0,SQLT_BLOB,
				&lobind[1],NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(lobstmt,&lobdef[2],err,3,
				&bfilelocator,0,SQLT_BFILEE,
				&lobind[2],NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,lobstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(lobstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)lobind[0],OCI_IND_NOTNULL);
	assertEquals((int)lobind[1],OCI_IND_NOTNULL);
	assertEquals((int)lobind[2],OCI_IND_NOTNULL);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobGetLength - clob and blob\n");
	// a clob's length counts characters and a blob's counts bytes
	ub4	loblength=0;
	assertEquals(
		OCILobGetLength(svc,err,cloblocator,&loblength),OCI_SUCCESS);
	assertEquals((int)loblength,(int)biglength);
	assertEquals(
		OCILobGetLength(svc,err,bloblocator,&loblength),OCI_SUCCESS);
	assertEquals((int)loblength,(int)sizeof(blobvalue));
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobRead - clob, one round trip\n");
	// one alphabet off the front, small enough to come back in a single
	// chunk
	char	clobbuffer[256];
	bytestring::zero(clobbuffer,sizeof(clobbuffer));
	ub4	clobamount=25;
	assertEquals(
		OCILobRead(svc,err,cloblocator,&clobamount,1,
				clobbuffer,sizeof(clobbuffer)-1,
				NULL,NULL,0,SQLCS_IMPLICIT),
		OCI_SUCCESS);
	assertEquals((int)clobamount,25);
	assertEquals((const char *)clobbuffer,"abcdefghijklmnopqrstuvwxy");
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobRead - clob, in pieces\n");
	// read it back in two halves, so the offset and amount arithmetic gets
	// exercised rather than one whole-lob read
	char	*bigbuffer=new char[biglength+1];
	bytestring::zero(bigbuffer,biglength+1);
	clobamount=biglength/2;
	assertEquals(
		OCILobRead(svc,err,cloblocator,&clobamount,1,
				bigbuffer,biglength/2,
				NULL,NULL,0,SQLCS_IMPLICIT),
		OCI_SUCCESS);
	assertEquals((int)clobamount,(int)(biglength/2));
	clobamount=biglength/2;
	assertEquals(
		OCILobRead(svc,err,cloblocator,&clobamount,biglength/2+1,
				bigbuffer+biglength/2,biglength/2,
				NULL,NULL,0,SQLCS_IMPLICIT),
		OCI_SUCCESS);
	assertEquals((int)clobamount,(int)(biglength/2));
	assertEquals((const char *)bigbuffer,(const char *)bigvalue);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobRead - blob\n");
	ub1	blobbuffer[512];
	bytestring::zero(blobbuffer,sizeof(blobbuffer));
	ub4	blobamount=sizeof(blobbuffer);
	assertEquals(
		OCILobRead(svc,err,bloblocator,&blobamount,1,
				blobbuffer,sizeof(blobbuffer),
				NULL,NULL,0,0),
		OCI_SUCCESS);
	assertEquals((int)blobamount,512);
	assertTrue(!bytestring::compare(blobbuffer,blobvalue,
						sizeof(blobvalue)));
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobWrite - clob\n");
	// the write side of a lob is not implemented.  the request is read in
	// full and then refused, so the session stays in step and the client
	// gets a real oracle error rather than a wrong answer
	const char	*clobvalue="the quick brown fox jumps over the lazy dog";
	clobamount=charstring::getLength(clobvalue);
	assertEquals(
		OCILobWrite(svc,err,cloblocator,&clobamount,1,
				(void *)clobvalue,
				charstring::getLength(clobvalue),
				OCI_ONE_PIECE,NULL,NULL,0,SQLCS_IMPLICIT),
		OCI_ERROR);
	// ORA-03001, unimplemented feature
	assertEquals((int)errorCode(),3001);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobTrim - clob\n");
	assertEquals(OCILobTrim(svc,err,cloblocator,0),OCI_ERROR);
	// ORA-03001, unimplemented feature
	assertEquals((int)errorCode(),3001);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobWrite - blob\n");
	blobamount=sizeof(blobvalue);
	assertEquals(
		OCILobWrite(svc,err,bloblocator,&blobamount,1,
				blobvalue,sizeof(blobvalue),
				OCI_ONE_PIECE,NULL,NULL,0,0),
		OCI_ERROR);
	// ORA-03001, unimplemented feature
	assertEquals((int)errorCode(),3001);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobGetLength - after the refused writes\n");
	// a refused write leaves the lob alone, so the lengths are still the
	// seeded ones and the session is still in step
	assertEquals(
		OCILobGetLength(svc,err,cloblocator,&loblength),OCI_SUCCESS);
	assertEquals((int)loblength,(int)biglength);
	assertEquals(
		OCILobGetLength(svc,err,bloblocator,&loblength),OCI_SUCCESS);
	assertEquals((int)loblength,(int)sizeof(blobvalue));
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobFileGetName - bfile\n");
	// this one never reaches the server - the client reads the directory
	// alias and the file name straight out of the locator it was handed.
	// no server side call hands either of them over, so the locator
	// carries neither and the client reads back two empty names rather
	// than a made up pair.  the call itself still succeeds
	char	bfiledir[64];
	char	bfilename[256];
	ub2	bfiledirlen=sizeof(bfiledir);
	ub2	bfilenamelen=sizeof(bfilename);
	bytestring::zero(bfiledir,sizeof(bfiledir));
	bytestring::zero(bfilename,sizeof(bfilename));
	assertEquals(
		OCILobFileGetName(env,err,bfilelocator,
					(text *)bfiledir,&bfiledirlen,
					(text *)bfilename,&bfilenamelen),
		OCI_SUCCESS);
	assertEquals((const char *)bfiledir,"");
	assertEquals((int)bfiledirlen,0);
	assertEquals((const char *)bfilename,"");
	assertEquals((int)bfilenamelen,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCILobFileExists - bfile\n");
	// nothing was put in DMP_DIR, so the locator names a file that is not
	// there.  What matters is that the call reaches the server and comes
	// back with an answer, not which answer it is.
	boolean	bfileexists=1;
	assertEquals(
		OCILobFileExists(svc,err,bfilelocator,&bfileexists),
		OCI_SUCCESS);
	assertEquals((int)bfileexists,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCITransCommit - end the lob transaction\n");
	assertEquals(OCITransCommit(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testclob)=100000"),1);
	assertEquals(
		countRows("protocoltestlob where "
				"dbms_lob.getlength(testblob)=512"),1);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIDescriptorFree - lob locators\n");
	delete[] bigvalue;
	delete[] bigbuffer;
	assertEquals(
		OCIDescriptorFree(cloblocator,OCI_DTYPE_LOB),OCI_SUCCESS);
	assertEquals(
		OCIDescriptorFree(bloblocator,OCI_DTYPE_LOB),OCI_SUCCESS);
	assertEquals(
		OCIDescriptorFree(bfilelocator,OCI_DTYPE_FILE),OCI_SUCCESS);
	assertEquals(OCIHandleFree(lobstmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Cursors ===============\n\n");

	OCIStmt	*curstmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&curstmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);

	stdoutput.printf("create table - ten rows to fetch\n");
	execImmediate("drop table protocoltestarray");
	assertEquals(
		execImmediate("create table protocoltestarray ("
				"testnumber number(10),"
				"testvarchar varchar2(40))"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltestarray "
				"select level,'row'||level from dual "
				"connect by level<=10"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(countRows("protocoltestarray"),10);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIBindByName - ref cursor\n");
	// the nested handle the ref cursor comes back through
	OCIStmt	*refcursor=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&refcursor,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);
	const char	*refblock="begin open :rc for "
					"select testnumber,testvarchar "
					"from protocoltestarray "
					"order by testnumber; end;";
	assertEquals(
		OCIStmtPrepare(curstmt,err,(text *)refblock,
				charstring::getLength(refblock),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	OCIBind	*refbnd=NULL;
	assertEquals(
		OCIBindByName(curstmt,&refbnd,err,(text *)":rc",3,
				&refcursor,0,SQLT_RSET,
				NULL,NULL,NULL,0,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,curstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIParamGet - through the ref cursor\n");
	// the nested handle describes and fetches like any other statement,
	// without ever being prepared or executed itself
	ub4	refcols=0;
	assertEquals(
		OCIAttrGet(refcursor,OCI_HTYPE_STMT,
				&refcols,NULL,OCI_ATTR_PARAM_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)refcols,2);
	assertColumn(refcursor,1,"TESTNUMBER",SQLT_NUM,22,10,0);
	assertColumn(refcursor,2,"TESTVARCHAR",SQLT_CHR,40,0,0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - through the ref cursor\n");
	OCIDefine	*refdef[2];
	char		refnumber[32];
	char		refvarchar[64];
	sb2		refind[2];
	bytestring::zero(refdef,sizeof(refdef));
	bytestring::zero(refind,sizeof(refind));
	assertEquals(
		OCIDefineByPos(refcursor,&refdef[0],err,1,
				refnumber,sizeof(refnumber),SQLT_STR,
				&refind[0],NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(refcursor,&refdef[1],err,2,
				refvarchar,sizeof(refvarchar),SQLT_STR,
				&refind[1],NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	for (int i=1; i<=10; i++) {
		assertEquals(
			OCIStmtFetch2(refcursor,err,1,OCI_FETCH_NEXT,0,
					OCI_DEFAULT),
			OCI_SUCCESS);
	}
	assertEquals((const char *)refnumber,"10");
	assertEquals((const char *)refvarchar,"row10");
	assertEquals(
		OCIStmtFetch2(refcursor,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_NO_DATA);
	ub4	refrows=0;
	assertEquals(
		OCIAttrGet(refcursor,OCI_HTYPE_STMT,
				&refrows,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)refrows,10);
	assertEquals(OCIHandleFree(refcursor,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCI_ATTR_PREFETCH_ROWS\n");
	const char	*arrayquery="select testnumber,testvarchar "
					"from protocoltestarray "
					"order by testnumber";
	assertEquals(
		OCIStmtPrepare(curstmt,err,(text *)arrayquery,
				charstring::getLength(arrayquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	ub4	prefetch=5;
	assertEquals(
		OCIAttrSet(curstmt,OCI_HTYPE_STMT,
				&prefetch,0,OCI_ATTR_PREFETCH_ROWS,err),
		OCI_SUCCESS);
	prefetch=0;
	assertEquals(
		OCIAttrGet(curstmt,OCI_HTYPE_STMT,
				&prefetch,NULL,OCI_ATTR_PREFETCH_ROWS,err),
		OCI_SUCCESS);
	assertEquals((int)prefetch,5);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - array fetch\n");
	// buffers wide enough for 4 rows at a time, so oracle returns them in
	// one compressed batch rather than a row per round trip
	OCIDefine	*arrdef[2];
	char		arrnumbers[4][32];
	char		arrvarchars[4][64];
	sb2		arrinds[2][4];
	ub2		arrlens[2][4];
	bytestring::zero(arrdef,sizeof(arrdef));
	bytestring::zero(arrnumbers,sizeof(arrnumbers));
	bytestring::zero(arrvarchars,sizeof(arrvarchars));
	bytestring::zero(arrinds,sizeof(arrinds));
	bytestring::zero(arrlens,sizeof(arrlens));
	assertEquals(
		OCIStmtExecute(svc,curstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(curstmt,&arrdef[0],err,1,
				arrnumbers,sizeof(arrnumbers[0]),SQLT_STR,
				&arrinds[0][0],&arrlens[0][0],NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(curstmt,&arrdef[1],err,2,
				arrvarchars,sizeof(arrvarchars[0]),SQLT_STR,
				&arrinds[1][0],&arrlens[1][0],NULL,OCI_DEFAULT),
		OCI_SUCCESS);

	// 10 rows, 4 at a time - two full batches, then a short one
	assertEquals(
		OCIStmtFetch2(curstmt,err,4,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)arrnumbers[0],"1");
	assertEquals((const char *)arrnumbers[3],"4");
	assertEquals((const char *)arrvarchars[0],"row1");
	assertEquals((const char *)arrvarchars[3],"row4");
	ub4	arrfetched=0;
	assertEquals(
		OCIAttrGet(curstmt,OCI_HTYPE_STMT,
				&arrfetched,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)arrfetched,4);

	assertEquals(
		OCIStmtFetch2(curstmt,err,4,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((const char *)arrnumbers[0],"5");
	assertEquals((const char *)arrnumbers[3],"8");
	assertEquals(
		OCIAttrGet(curstmt,OCI_HTYPE_STMT,
				&arrfetched,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)arrfetched,8);

	// only 2 rows left, so this batch comes up short and says so
	assertEquals(
		OCIStmtFetch2(curstmt,err,4,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_NO_DATA);
	assertEquals((const char *)arrnumbers[0],"9");
	assertEquals((const char *)arrnumbers[1],"10");
	assertEquals(
		OCIAttrGet(curstmt,OCI_HTYPE_STMT,
				&arrfetched,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)arrfetched,10);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - array fetch wider than one packet\n");
	// rows wide enough that a 6-row batch can't fit in one negotiated
	// packet - this exercises the SDU bound in sendFetch3Response() and
	// sendQuery3Response()'s inline prefetch, which have to split the
	// batch across more than one on-the-wire fetch
	execImmediate("drop table protocoltestwide");
	assertEquals(
		execImmediate("create table protocoltestwide ("
				"testnumber number(10),"
				"testvarchar varchar2(2000))"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into protocoltestwide "
				"select level,rpad('row'||level,2000,'x') "
				"from dual connect by level<=6"),
		OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	assertEquals(countRows("protocoltestwide"),6);

	const char	*widequery="select testnumber,testvarchar "
					"from protocoltestwide "
					"order by testnumber";
	assertEquals(
		OCIStmtPrepare(curstmt,err,(text *)widequery,
				charstring::getLength(widequery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	ub4	wideprefetch=6;
	assertEquals(
		OCIAttrSet(curstmt,OCI_HTYPE_STMT,
				&wideprefetch,0,OCI_ATTR_PREFETCH_ROWS,err),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,curstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);

	OCIDefine	*widedef[2];
	char		widenumbers[6][32];
	char		widevarchars[6][2001];
	sb2		wideinds[2][6];
	ub2		widelens[2][6];
	bytestring::zero(widedef,sizeof(widedef));
	bytestring::zero(widenumbers,sizeof(widenumbers));
	bytestring::zero(widevarchars,sizeof(widevarchars));
	bytestring::zero(wideinds,sizeof(wideinds));
	bytestring::zero(widelens,sizeof(widelens));
	assertEquals(
		OCIDefineByPos(curstmt,&widedef[0],err,1,
				widenumbers,sizeof(widenumbers[0]),SQLT_STR,
				&wideinds[0][0],&widelens[0][0],NULL,
				OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIDefineByPos(curstmt,&widedef[1],err,2,
				widevarchars,sizeof(widevarchars[0]),SQLT_STR,
				&wideinds[1][0],&widelens[1][0],NULL,
				OCI_DEFAULT),
		OCI_SUCCESS);

	// all 6 rows in one array fetch call - the module can only pack a
	// few of these into any single on-the-wire packet, so the client has
	// to ask again to fill the array
	assertEquals(
		OCIStmtFetch2(curstmt,err,6,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	for (int wi=0; wi<6; wi++) {
		char	widenum[32];
		charstring::printf(widenum,sizeof(widenum),"%d",wi+1);
		assertEquals((const char *)widenumbers[wi],widenum);
		char	wideexpected[2001];
		charstring::printf(wideexpected,sizeof(wideexpected),
					"row%d",wi+1);
		size_t	widelen=charstring::getLength(wideexpected);
		for (size_t wj=widelen; wj<2000; wj++) {
			wideexpected[wj]='x';
		}
		wideexpected[2000]='\0';
		assertEquals((const char *)widevarchars[wi],wideexpected);
	}
	ub4	widefetched=0;
	assertEquals(
		OCIAttrGet(curstmt,OCI_HTYPE_STMT,
				&widefetched,NULL,OCI_ATTR_ROW_COUNT,err),
		OCI_SUCCESS);
	assertEquals((int)widefetched,6);
	assertEquals(
		OCIStmtFetch2(curstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_NO_DATA);
	assertEquals(execImmediate("drop table protocoltestwide"),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIHandleFree - statement\n");
	assertEquals(OCIHandleFree(curstmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============ Transactions ============\n\n");

	stdoutput.printf("OCITransRollback\n");
	assertEquals(
		execImmediate("insert into protocoltesttran values (1)"),
		OCI_SUCCESS);
	assertEquals(countRows("protocoltesttran"),1);
	assertEquals(OCITransRollback(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	assertEquals(countRows("protocoltesttran"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCITransCommit\n");
	assertEquals(
		execImmediate("insert into protocoltesttran values (2)"),
		OCI_SUCCESS);
	assertEquals(OCITransCommit(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	// the commit took, so the rollback after it loses nothing
	assertEquals(OCITransRollback(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	assertEquals(countRows("protocoltesttran"),1);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - commit on success\n");
	OCIStmt		*tranmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&tranmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);
	const char	*traninsert="insert into protocoltesttran values (3)";
	assertEquals(
		OCIStmtPrepare(tranmt,err,(text *)traninsert,
				charstring::getLength(traninsert),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,tranmt,err,1,0,NULL,NULL,
				OCI_COMMIT_ON_SUCCESS),
		OCI_SUCCESS);
	// autocommit committed the insert, so the rollback loses nothing
	assertEquals(OCITransRollback(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	assertEquals(countRows("protocoltesttran"),2);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - autocommit off\n");
	const char	*tranin4="insert into protocoltesttran values (4)";
	assertEquals(
		OCIStmtPrepare(tranmt,err,(text *)tranin4,
				charstring::getLength(tranin4),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,tranmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(countRows("protocoltesttran"),3);
	assertEquals(OCITransRollback(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	assertEquals(countRows("protocoltesttran"),2);
	assertEquals(OCIHandleFree(tranmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n=============== Errors ===============\n\n");

	OCIStmt	*errstmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&errstmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);

	stdoutput.printf("OCIStmtExecute - no such table\n");
	const char	*badtable="select * from nosuchtable";
	assertEquals(
		OCIStmtPrepare(errstmt,err,(text *)badtable,
				charstring::getLength(badtable),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,errstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-00942, table or view does not exist
	assertEquals((int)errorCode(),942);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - no such column\n");
	const char	*badcolumn="select nosuchcolumn from protocoltesttable";
	assertEquals(
		OCIStmtPrepare(errstmt,err,(text *)badcolumn,
				charstring::getLength(badcolumn),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,errstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-00904, invalid identifier
	assertEquals((int)errorCode(),904);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - bad syntax\n");
	const char	*badsyntax="selectt 1 from dual";
	assertEquals(
		OCIStmtPrepare(errstmt,err,(text *)badsyntax,
				charstring::getLength(badsyntax),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,errstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-00900, invalid SQL statement
	assertEquals((int)errorCode(),900);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtExecute - value too wide for the column\n");
	const char	*toowide="insert into protocoltesttable (testvarchar) "
				"values ('123456789012345678901234567890"
					"12345678901234567890')";
	assertEquals(
		OCIStmtPrepare(errstmt,err,(text *)toowide,
				charstring::getLength(toowide),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,errstmt,err,1,0,NULL,NULL,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-12899, value too large for column
	assertEquals((int)errorCode(),12899);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIStmtFetch2 - statement never executed\n");
	const char	*neverrun="select 1 from dual";
	assertEquals(
		OCIStmtPrepare(errstmt,err,(text *)neverrun,
				charstring::getLength(neverrun),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(errstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_ERROR);
	// ORA-24374, define not done before fetch or execute and fetch.
	// The missing define is caught client side, ahead of the missing
	// execute that would otherwise give ORA-24338.
	assertEquals((int)errorCode(),24374);
	stdoutput.printf("\n\n");


	// row 2 of this result set divides by zero.  oracle only evaluates
	// the expression as it produces each row, so how many rows the
	// connection pulls from the backend at a time - fetchatonce - decides
	// whether the failure lands on the execute or on a later fetch.  both
	// halves are covered, one instance each
	const char	*divzero="select 1/(level-2) from dual "
				"connect by level<=3";

	if (!isfetchatonce) {

		stdoutput.printf("OCIStmtExecute - error mid-fetch\n");
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
		assertEquals(
			OCIStmtPrepare(errstmt,err,(text *)divzero,
					charstring::getLength(divzero),
					OCI_NTV_SYNTAX,OCI_DEFAULT),
			OCI_SUCCESS);
		assertEquals(
			OCIStmtExecute(svc,errstmt,err,0,0,
						NULL,NULL,OCI_DEFAULT),
			OCI_ERROR);
		// ORA-01476, divisor is equal to zero
		assertEquals((int)errorCode(),1476);
		stdoutput.printf("\n\n");

	} else {

		stdoutput.printf("OCIStmtFetch2 - error mid-fetch\n");
		// the fetch-time counterpart of the case above (#9601).  this
		// instance's connection string sets fetchatonce=1, so the
		// connection asks the backend for one row per fetch, and a
		// prefetch of 1 row - with no prefetch memory to raise it -
		// holds the execute's inline prefetch to row 1 as well.  so
		// the execute succeeds, and the row 2 divide by zero is only
		// discovered on a genuine second fetch, where the error goes
		// out on a fetch response.  that's sendFetch3Response()'s
		// fetchRow() error branch rather than sendQuery3Response()'s
		assertEquals(
			OCIStmtPrepare(errstmt,err,(text *)divzero,
					charstring::getLength(divzero),
					OCI_NTV_SYNTAX,OCI_DEFAULT),
			OCI_SUCCESS);
		ub4	divzerorows=1;
		assertEquals(
			OCIAttrSet(errstmt,OCI_HTYPE_STMT,
					&divzerorows,0,
					OCI_ATTR_PREFETCH_ROWS,err),
			OCI_SUCCESS);
		ub4	divzeromemory=0;
		assertEquals(
			OCIAttrSet(errstmt,OCI_HTYPE_STMT,
					&divzeromemory,0,
					OCI_ATTR_PREFETCH_MEMORY,err),
			OCI_SUCCESS);
		assertEquals(
			OCIStmtExecute(svc,errstmt,err,0,0,
						NULL,NULL,OCI_DEFAULT),
			OCI_SUCCESS);
		OCIDefine	*divzerodef=NULL;
		char		divzerovalue[64];
		sb2		divzeroind=0;
		bytestring::zero(divzerovalue,sizeof(divzerovalue));
		assertEquals(
			OCIDefineByPos(errstmt,&divzerodef,err,1,
					divzerovalue,sizeof(divzerovalue),
					SQLT_STR,&divzeroind,NULL,NULL,
					OCI_DEFAULT),
			OCI_SUCCESS);
		// row 1 is 1/(1-2), which evaluates fine
		assertEquals(
			OCIStmtFetch2(errstmt,err,1,
					OCI_FETCH_NEXT,0,OCI_DEFAULT),
			OCI_SUCCESS);
		assertEquals((const char *)divzerovalue,"-1");
		// row 2 is 1/(2-2), which doesn't
		assertEquals(
			OCIStmtFetch2(errstmt,err,1,
					OCI_FETCH_NEXT,0,OCI_DEFAULT),
			OCI_ERROR);
		// ORA-01476, divisor is equal to zero
		assertEquals((int)errorCode(),1476);
		stdoutput.printf("\n\n");
	}


	stdoutput.printf("OCIHandleFree - statement\n");
	assertEquals(OCIHandleFree(errstmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Teardown ==============\n\n");

	stdoutput.printf("drop table\n");
	assertEquals(execImmediate("drop table protocoltesttable"),OCI_SUCCESS);
	assertEquals(execImmediate("drop table protocoltesttran"),OCI_SUCCESS);
	assertEquals(execImmediate("drop table protocoltestbind"),OCI_SUCCESS);
	assertEquals(execImmediate("drop table protocoltesttypes"),OCI_SUCCESS);
	assertEquals(execImmediate("drop table protocoltestlong"),OCI_SUCCESS);
	assertEquals(
		execImmediate("drop table protocoltestlongraw"),OCI_SUCCESS);
	assertEquals(execImmediate("drop table protocoltestlob"),OCI_SUCCESS);
	assertEquals(execImmediate("drop table protocoltestarray"),OCI_SUCCESS);
	assertEquals(execImmediate("drop table protocoltestclr"),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIHandleFree - statement\n");
	assertEquals(OCIHandleFree(stmt,OCI_HTYPE_STMT),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCISessionEnd\n");
	assertEquals(OCISessionEnd(svc,err,session,OCI_DEFAULT),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIServerDetach\n");
	assertEquals(OCIServerDetach(srv,err,OCI_DEFAULT),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("OCIHandleFree\n");
	assertEquals(OCIHandleFree(trans,OCI_HTYPE_TRANS),OCI_SUCCESS);
	assertEquals(OCIHandleFree(session,OCI_HTYPE_SESSION),OCI_SUCCESS);
	assertEquals(OCIHandleFree(svc,OCI_HTYPE_SVCCTX),OCI_SUCCESS);
	assertEquals(OCIHandleFree(srv,OCI_HTYPE_SERVER),OCI_SUCCESS);
	assertEquals(OCIHandleFree(err,OCI_HTYPE_ERROR),OCI_SUCCESS);
	stdoutput.printf("\n\n");


	reportTestStatus();
	return status;
}
