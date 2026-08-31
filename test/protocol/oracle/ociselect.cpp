// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI client: log in, run one select, print what comes back.
//
//   ./ociselect CONNECTSTRING [USER PASSWORD [QUERY]]
//
// oracle.cpp in this directory is the full OCI test; this is deliberately
// as small as an OCI login-plus-query can be, so a wire capture of it has
// nothing in it but the login and the one query.  Written for #9174, to
// point an OCI client through oraproxy's banner-rewriting mode at a real
// oracle server and see what a real server answers an OCI client with in
// the portable encoding.
//
// Set OCISELECT_SERVER_VERSION=1 in the environment to also call
// OCIServerVersion after the login and print the banner it returns.  Off
// by default, so an ordinary capture still holds only the login and the
// one query.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>
#include <config.h>

extern "C" {
	#define OCIVER_ORACLE
	#include <oci.h>
}

static OCIError	*err=NULL;

// print the last error and return
static void printError(const char *what, sword status) {
	text	message[1024];
	bytestring::zero(message,sizeof(message));
	sb4	errcode=0;
	if (err) {
		OCIErrorGet(err,1,NULL,&errcode,
				message,sizeof(message),OCI_HTYPE_ERROR);
	}
	stdoutput.printf("%s failed: status %d, code %d: %s\n",
				what,(int)status,(int)errcode,(char *)message);
}

int main(int argc, const char **argv) {

	if (argc<2) {
		stdoutput.printf("usage: %s CONNECTSTRING "
				"[USER PASSWORD [QUERY]]\n",argv[0]);
		return 1;
	}

	const char	*connectstring=argv[1];
	const char	*user=(argc>2)?argv[2]:"testuser";
	const char	*password=(argc>3)?argv[3]:"testpassword";
	const char	*query=(argc>4)?argv[4]:"select 1 from dual";

	OCIEnv	*env=NULL;
	if (OCIEnvCreate(&env,OCI_DEFAULT,NULL,NULL,NULL,NULL,0,NULL)!=
							OCI_SUCCESS) {
		stdoutput.printf("OCIEnvCreate failed\n");
		return 1;
	}

	if (OCIHandleAlloc(env,(void **)&err,
				OCI_HTYPE_ERROR,0,NULL)!=OCI_SUCCESS) {
		stdoutput.printf("OCIHandleAlloc(error) failed\n");
		return 1;
	}

	stdoutput.printf("logging in to %s as %s...\n",connectstring,user);

	OCISvcCtx	*svc=NULL;
	sword		status=OCILogon(env,err,&svc,
				(const OraText *)user,
				charstring::getLength(user),
				(const OraText *)password,
				charstring::getLength(password),
				(const OraText *)connectstring,
				charstring::getLength(connectstring));
	if (status!=OCI_SUCCESS && status!=OCI_SUCCESS_WITH_INFO) {
		printError("OCILogon",status);
		return 1;
	}
	stdoutput.printf("login ok\n");

	// OCIServerVersion, only when asked for, so the default capture is
	// still nothing but the login and the one query
	if (charstring::isYes(
		environment::getValue("OCISELECT_SERVER_VERSION"))) {

		stdoutput.printf("getting server version...\n");

		char	versionbuffer[1024];
		bytestring::zero(versionbuffer,sizeof(versionbuffer));
		status=OCIServerVersion(svc,err,
					(text *)versionbuffer,
					sizeof(versionbuffer),
					OCI_HTYPE_SVCCTX);
		if (status!=OCI_SUCCESS && status!=OCI_SUCCESS_WITH_INFO) {
			printError("OCIServerVersion",status);
			return 1;
		}
		stdoutput.printf("server version: %s\n",versionbuffer);
	}

	OCIStmt	*stmt=NULL;
	if (OCIHandleAlloc(env,(void **)&stmt,
				OCI_HTYPE_STMT,0,NULL)!=OCI_SUCCESS) {
		stdoutput.printf("OCIHandleAlloc(stmt) failed\n");
		return 1;
	}

	stdoutput.printf("running: %s\n",query);

	status=OCIStmtPrepare(stmt,err,(text *)query,
				charstring::getLength(query),
				OCI_NTV_SYNTAX,OCI_DEFAULT);
	if (status!=OCI_SUCCESS) {
		printError("OCIStmtPrepare",status);
		return 1;
	}

	// define the single column as a string, so whatever type it is
	// comes back readable
	char		buffer[1024];
	bytestring::zero(buffer,sizeof(buffer));
	sb2		indicator=0;
	OCIDefine	*def=NULL;
	status=OCIDefineByPos(stmt,&def,err,1,buffer,sizeof(buffer),
				SQLT_STR,&indicator,NULL,NULL,OCI_DEFAULT);
	if (status!=OCI_SUCCESS) {
		printError("OCIDefineByPos",status);
		return 1;
	}

	status=OCIStmtExecute(svc,stmt,err,1,0,NULL,NULL,OCI_DEFAULT);
	if (status!=OCI_SUCCESS && status!=OCI_SUCCESS_WITH_INFO) {
		printError("OCIStmtExecute",status);
		return 1;
	}
	stdoutput.printf("row 1: %s\n",buffer);

	// fetch the rest, if any
	for (;;) {
		status=OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT);
		if (status==OCI_NO_DATA) {
			break;
		}
		if (status!=OCI_SUCCESS && status!=OCI_SUCCESS_WITH_INFO) {
			printError("OCIStmtFetch2",status);
			return 1;
		}
		stdoutput.printf("row: %s\n",buffer);
	}

	stdoutput.printf("done\n");

	OCIHandleFree(stmt,OCI_HTYPE_STMT);
	OCILogoff(svc,err);

	return 0;
}
