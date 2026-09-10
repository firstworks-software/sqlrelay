// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI client that retries a failed login on one connection: one
// OCIServerAttach, then a selectable sequence of OCISessionBegin attempts on
// that same attach, with different credentials each time and no detach or
// reattach in between.  Nothing else.
//
//   ./ociloginretry CONNECTSTRING [--attempts=LIST] [USER PASSWORD]
//
// This is oci8.cpp's Authentication section, call for call - the #9311
// sequence - lifted out of the suite it lives in.  oci8.cpp logs in twice,
// creates tables and runs a hundred other statements around that section, so
// a capture of it is far too noisy to read the retry's byte layout out of.
// A capture of this program holds the attach, the login attempts, and the
// logoff.
//
// CONNECTSTRING is whatever OCIServerAttach takes - a tnsnames.ora alias or a
// full (DESCRIPTION=...) - so the same binary can be pointed at oraproxy in
// front of a real oracle server or at a local sqlr-listener speaking the
// oracle protocol, and the two captures diffed.
//
// --attempts= takes the attempts to run, in order, comma separated:
//
//   wrongpassword    the real user with a bad password - ORA-01017
//   unknownuser      a user that does not exist - ORA-01017, the same error
//                    a real server gives for a wrong password, on purpose
//   emptypassword    the real user with an empty password - ORA-01005, which
//                    OCI raises client side before sending anything, so this
//                    attempt puts nothing at all on the wire
//   correctpassword  the real user and password, which has to succeed on the
//                    same attach the failures above ran on - that it does is
//                    the connection-reuse result
//
// The default is all four, in that order, which is what oci8.cpp does.  A
// run against a REAL server should usually ask for less: every wrong-password
// attempt counts against the account's FAILED_LOGIN_ATTEMPTS profile limit
// and can lock the user out, while an unknown user never reaches password
// verification and so cannot lock anything.  "--attempts=unknownuser,
// correctpassword" gets the retry-on-one-attach sequence with no lockout
// risk at all, and a run that does spend a wrong-password attempt should end
// on correctpassword, which resets the failed count.
//
// After every failed attempt the program reads OCI_ATTR_SERVER back off the
// service context and reports whether it is still the handle the attach
// returned.  That is a client-side read - it puts nothing on the wire - and
// it is what says the failures were retried on one connection rather than on
// a fresh one.
//
// Written for #10039: against sqlrelay the wrong-password and unknown-user
// attempts come back ORA-03120 and ORA-24327 instead of ORA-01017, even
// though the server sends ORA-01017, and no capture on file covers a modern
// client retrying a login on one connection.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>
#include <config.h>

extern "C" {
	#define OCIVER_ORACLE
	#include <oci.h>
}

static OCIEnv		*env=NULL;
static OCIError		*err=NULL;
static OCIServer	*srv=NULL;
static OCISvcCtx	*svc=NULL;
static OCISession	*session=NULL;

static const char	*user="testuser";
static const char	*password="testpassword";

// name for an OCI return code, so the printed sequence reads without a table
static const char *statusName(sword status) {
	switch (status) {
		case OCI_SUCCESS:
			return "OCI_SUCCESS";
		case OCI_SUCCESS_WITH_INFO:
			return "OCI_SUCCESS_WITH_INFO";
		case OCI_ERROR:
			return "OCI_ERROR";
		case OCI_INVALID_HANDLE:
			return "OCI_INVALID_HANDLE";
		case OCI_NO_DATA:
			return "OCI_NO_DATA";
		case OCI_NEED_DATA:
			return "OCI_NEED_DATA";
		case OCI_STILL_EXECUTING:
			return "OCI_STILL_EXECUTING";
		default:
			return "unknown";
	}
}

// print what's in the error handle and return the ORA number
static int printError(const char *what) {

	text	message[1024];
	bytestring::zero(message,sizeof(message));
	sb4	errcode=0;
	if (err) {
		OCIErrorGet(err,1,NULL,&errcode,
				message,sizeof(message),OCI_HTYPE_ERROR);
	}
	charstring::rightTrim((char *)message,'\n');
	stdoutput.printf("  %s failed: code %d: %s\n",
				what,(int)errcode,(char *)message);
	return (int)errcode;
}

// put a username and password on the session handle
static void setCredentials(const char *u, const char *p) {
	if (OCIAttrSet(session,OCI_HTYPE_SESSION,
			(void *)u,charstring::getLength(u),
			OCI_ATTR_USERNAME,err)!=OCI_SUCCESS) {
		printError("OCIAttrSet(username)");
	}
	if (OCIAttrSet(session,OCI_HTYPE_SESSION,
			(void *)p,charstring::getLength(p),
			OCI_ATTR_PASSWORD,err)!=OCI_SUCCESS) {
		printError("OCIAttrSet(password)");
	}
}

// credentials and expected outcome for one attempt name
static bool lookupAttempt(const char *name,
				const char **u, const char **p,
				const char **describepassword,
				sword *expectedstatus, int *expectederror) {

	*expectedstatus=OCI_ERROR;
	if (!charstring::compare(name,"wrongpassword")) {
		*u=user;
		*p="wrongpassword";
		*describepassword="wrong password";
		*expectederror=1017;
	} else if (!charstring::compare(name,"unknownuser")) {
		*u="nosuchuser";
		*p="nosuchpassword";
		*describepassword="unknown user's password";
		*expectederror=1017;
	} else if (!charstring::compare(name,"emptypassword")) {
		*u=user;
		*p="";
		*describepassword="empty password";
		*expectederror=1005;
	} else if (!charstring::compare(name,"correctpassword") ||
			!charstring::compare(name,"correct")) {
		*u=user;
		*p=password;
		*describepassword="correct password";
		*expectedstatus=OCI_SUCCESS;
		*expectederror=0;
	} else {
		return false;
	}
	return true;
}

// one OCISessionBegin on the attach that's already up.  returns false if it
// didn't do what a real server does, and sets begun when a session started.
static bool runAttempt(int number, const char *name, bool *begun) {

	const char	*u=NULL;
	const char	*p=NULL;
	const char	*describepassword=NULL;
	sword		expectedstatus=OCI_ERROR;
	int		expectederror=0;
	if (!lookupAttempt(name,&u,&p,&describepassword,
				&expectedstatus,&expectederror)) {
		stdoutput.printf("attempt %d: unknown attempt \"%s\"\n",
								number,name);
		return false;
	}

	stdoutput.printf("\n=== attempt %d: %s ===\n",number,name);
	stdoutput.printf("  user %s, %s\n",u,describepassword);

	setCredentials(u,p);

	sword	status=OCISessionBegin(svc,err,session,
					OCI_CRED_RDBMS,OCI_DEFAULT);
	stdoutput.printf("  OCISessionBegin: status %d (%s)\n",
					(int)status,statusName(status));

	// a login can succeed and still leave a diagnostic behind (a password
	// about to expire, say), so print whatever's in the error handle for
	// anything but a plain success
	bool	succeeded=(status==OCI_SUCCESS ||
				status==OCI_SUCCESS_WITH_INFO);
	int	errcode=0;
	if (status!=OCI_SUCCESS) {
		errcode=printError("OCISessionBegin");
	}

	// bind the session to the service context, the way a client that got
	// this far goes on to use it
	if (succeeded) {
		*begun=true;
		if (OCIAttrSet(svc,OCI_HTYPE_SVCCTX,session,0,
					OCI_ATTR_SESSION,err)!=OCI_SUCCESS) {
			printError("OCIAttrSet(session)");
		}
	}

	// the server handle behind a failed attempt has to be the one the
	// attach returned - a connection dropped and remade would show up
	// here as a different handle
	if (!succeeded) {
		OCIServer	*serverafterfailure=NULL;
		if (OCIAttrGet(svc,OCI_HTYPE_SVCCTX,&serverafterfailure,NULL,
					OCI_ATTR_SERVER,err)!=OCI_SUCCESS) {
			printError("OCIAttrGet(server)");
		}
		stdoutput.printf("  OCI_ATTR_SERVER: %s\n",
				(serverafterfailure==srv)?
					"same handle, same connection":
					"CHANGED - not the attached handle");
	}

	bool	asexpected=false;
	if (expectedstatus==OCI_SUCCESS) {
		stdoutput.printf("  expected: %s\n",statusName(expectedstatus));
		asexpected=succeeded;
	} else {
		stdoutput.printf("  expected: %s, ORA-%05d\n",
				statusName(expectedstatus),expectederror);
		asexpected=(status==expectedstatus &&
					errcode==expectederror);
	}
	stdoutput.printf("  result: %s\n",(asexpected)?"as expected":
						"NOT what a real server does");
	return asexpected;
}

int main(int argc, const char **argv) {

	const char	*connectstring=NULL;
	const char	*attempts="wrongpassword,unknownuser,"
					"emptypassword,correctpassword";
	int		positional=0;

	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"--attempts=",11)) {
			attempts=argv[i]+11;
		} else if (positional==0) {
			connectstring=argv[i];
			positional++;
		} else if (positional==1) {
			user=argv[i];
			positional++;
		} else if (positional==2) {
			password=argv[i];
			positional++;
		}
	}

	if (!connectstring) {
		stdoutput.printf("usage: %s CONNECTSTRING "
				"[--attempts=wrongpassword,unknownuser,"
				"emptypassword,correctpassword] "
				"[USER PASSWORD]\n",argv[0]);
		return 1;
	}

	char		**attemptlist=NULL;
	uint64_t	attemptcount=0;
	charstring::split(attempts,",",true,&attemptlist,&attemptcount);

	// check the whole list before anything connects, so a misspelled
	// attempt name shows up as an error rather than as a sequence quietly
	// shorter than the one that was asked for
	for (uint64_t i=0; i<attemptcount; i++) {
		charstring::bothTrim(attemptlist[i]);
		const char	*u=NULL;
		const char	*p=NULL;
		const char	*describepassword=NULL;
		sword		expectedstatus=OCI_ERROR;
		int		expectederror=0;
		if (!lookupAttempt(attemptlist[i],&u,&p,&describepassword,
					&expectedstatus,&expectederror)) {
			stdoutput.printf("unknown attempt \"%s\"\n",
							attemptlist[i]);
			return 1;
		}
	}

	// the same env and handle set oci8.cpp's Authentication section runs
	// on - which handles get reused across the retries is exactly what
	// this program is here to capture
	#ifdef HAVE_ORACLE_8i
		if (OCIEnvCreate(&env,OCI_DEFAULT|OCI_OBJECT,
					NULL,NULL,NULL,NULL,0,NULL)!=
								OCI_SUCCESS) {
			stdoutput.printf("OCIEnvCreate failed\n");
			return 1;
		}
	#else
		if (OCIInitialize(OCI_DEFAULT,NULL,NULL,NULL,NULL)!=
							OCI_SUCCESS ||
			OCIEnvInit(&env,OCI_DEFAULT,0,NULL)!=OCI_SUCCESS) {
			stdoutput.printf("OCIInitialize/OCIEnvInit failed\n");
			return 1;
		}
	#endif

	if (OCIHandleAlloc(env,(void **)&err,
				OCI_HTYPE_ERROR,0,NULL)!=OCI_SUCCESS ||
		OCIHandleAlloc(env,(void **)&srv,
				OCI_HTYPE_SERVER,0,NULL)!=OCI_SUCCESS ||
		OCIHandleAlloc(env,(void **)&svc,
				OCI_HTYPE_SVCCTX,0,NULL)!=OCI_SUCCESS ||
		OCIHandleAlloc(env,(void **)&session,
				OCI_HTYPE_SESSION,0,NULL)!=OCI_SUCCESS) {
		stdoutput.printf("OCIHandleAlloc failed\n");
		return 1;
	}

	// the one and only attach.  nothing below reattaches, so every login
	// attempt after this runs on this one connection.
	stdoutput.printf("attaching to %s...\n",connectstring);
	sword	status=OCIServerAttach(srv,err,(text *)connectstring,
					charstring::getLength(connectstring),0);
	if (status!=OCI_SUCCESS) {
		stdoutput.printf("OCIServerAttach: status %d (%s)\n",
					(int)status,statusName(status));
		printError("OCIServerAttach");
		return 1;
	}
	if (OCIAttrSet(svc,OCI_HTYPE_SVCCTX,srv,0,
				OCI_ATTR_SERVER,err)!=OCI_SUCCESS) {
		printError("OCIAttrSet(server)");
		return 1;
	}
	stdoutput.printf("attached\n");

	bool	begun=false;
	bool	allasexpected=true;
	for (uint64_t i=0; i<attemptcount; i++) {
		if (!runAttempt((int)i+1,attemptlist[i],&begun)) {
			allasexpected=false;
		}
		delete[] attemptlist[i];
	}
	delete[] attemptlist;

	stdoutput.printf("\n");

	// end the session that took, if one did, and detach - a clean logoff
	// rather than a dropped socket, so the tail of the capture decodes
	if (begun) {
		status=OCISessionEnd(svc,err,session,OCI_DEFAULT);
		if (status!=OCI_SUCCESS) {
			printError("OCISessionEnd");
		}
	}
	status=OCIServerDetach(srv,err,OCI_DEFAULT);
	if (status!=OCI_SUCCESS) {
		printError("OCIServerDetach");
	}
	OCIHandleFree(session,OCI_HTYPE_SESSION);
	OCIHandleFree(svc,OCI_HTYPE_SVCCTX);
	OCIHandleFree(srv,OCI_HTYPE_SERVER);

	stdoutput.printf("done - %s\n",(allasexpected)?
				"every attempt did what a real server does":
				"at least one attempt did not");

	return (allasexpected)?0:1;
}
