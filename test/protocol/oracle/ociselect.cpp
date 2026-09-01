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
//
// Set OCISELECT_LOB_SETUP=1 to create and populate the ociselectlob
// table (a clob, a blob and a bfile column), and OCISELECT_LOB=1 to
// select that row's locators and run OCILobGetLength, a single-call
// OCILobRead on the clob, a piecewise OCILobRead on the blob,
// OCILobFileGetName/OCILobFileExists on the bfile, and
// OCIDescriptorFree on all three.  Two switches, so the table can be
// set up in one run and only the lob traffic captured in the next.
// For #9589.

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

// run one statement that returns no rows (the ddl and dml the lob test
// table needs).  ignoreerrors just reports and carries on - the "drop
// table" that the setup starts with fails the first time by design.
static bool runStatement(OCISvcCtx *svc, OCIEnv *env,
				const char *sql, bool ignoreerrors) {

	OCIStmt	*st=NULL;
	if (OCIHandleAlloc(env,(void **)&st,
				OCI_HTYPE_STMT,0,NULL)!=OCI_SUCCESS) {
		stdoutput.printf("OCIHandleAlloc(stmt) failed\n");
		return false;
	}

	sword	status=OCIStmtPrepare(st,err,(text *)sql,
					charstring::getLength(sql),
					OCI_NTV_SYNTAX,OCI_DEFAULT);
	if (status==OCI_SUCCESS) {
		status=OCIStmtExecute(svc,st,err,1,0,NULL,NULL,OCI_DEFAULT);
	}
	OCIHandleFree(st,OCI_HTYPE_STMT);

	if (status!=OCI_SUCCESS && status!=OCI_SUCCESS_WITH_INFO) {
		printError(sql,status);
		return ignoreerrors;
	}
	return true;
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

	// create and populate the table the lob section reads, only when
	// asked for.  This is a separate switch from OCISELECT_LOB so the
	// table can be set up in one run and the lob operations captured
	// in another, with nothing but the lob traffic in the capture.
	if (charstring::isYes(environment::getValue("OCISELECT_LOB_SETUP"))) {

		stdoutput.printf("setting up the lob test table...\n");

		// the first drop is expected to fail
		runStatement(svc,env,"drop table ociselectlob",true);
		if (!runStatement(svc,env,
			"create table ociselectlob "
			"(id number, c clob, b blob, f bfile)",false) ||
			!runStatement(svc,env,
			"insert into ociselectlob values "
			"(1,rpad('C',400,'C'),"
			"to_blob(utl_raw.cast_to_raw(rpad('B',2000,'B'))),"
			"bfilename('DMP_DIR','ociselectlob.txt'))",false) ||
			!runStatement(svc,env,
			"insert into ociselectlob (id,c) values (2,"
			"to_clob(rpad('D',4000,'D'))||"
			"to_clob(rpad('D',4000,'D'))||"
			"to_clob(rpad('D',4000,'D'))||"
			"to_clob(rpad('D',4000,'D'))||"
			"to_clob(rpad('D',4000,'D'))||"
			"to_clob(rpad('D',4000,'D'))||"
			"to_clob(rpad('D',4000,'D'))||"
			"to_clob(rpad('D',4000,'D')))",false) ||
			!runStatement(svc,env,"commit",false)) {
			return 1;
		}

		stdoutput.printf("lob test table ready\n");
	}

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


	// lob operations, only when asked for, so an ordinary capture
	// still holds only the login and the one query.  For #9589 - see
	// the OCISELECT_LOB comment at the top of this file.
	if (charstring::isYes(environment::getValue("OCISELECT_LOB"))) {

		stdoutput.printf("\n=== lob operations ===\n");

		OCILobLocator	*clob=NULL;
		OCILobLocator	*blob=NULL;
		OCILobLocator	*bfile=NULL;
		if (OCIDescriptorAlloc(env,(void **)&clob,
					OCI_DTYPE_LOB,0,NULL)!=OCI_SUCCESS ||
			OCIDescriptorAlloc(env,(void **)&blob,
					OCI_DTYPE_LOB,0,NULL)!=OCI_SUCCESS ||
			OCIDescriptorAlloc(env,(void **)&bfile,
					OCI_DTYPE_FILE,0,NULL)!=OCI_SUCCESS) {
			stdoutput.printf("OCIDescriptorAlloc failed\n");
			return 1;
		}

		OCIStmt	*lobstmt=NULL;
		if (OCIHandleAlloc(env,(void **)&lobstmt,
				OCI_HTYPE_STMT,0,NULL)!=OCI_SUCCESS) {
			stdoutput.printf("OCIHandleAlloc(lobstmt) failed\n");
			return 1;
		}

		const char	*lobquery=
			"select c,b,f from ociselectlob where id=1";
		stdoutput.printf("running: %s\n",lobquery);
		status=OCIStmtPrepare(lobstmt,err,(text *)lobquery,
					charstring::getLength(lobquery),
					OCI_NTV_SYNTAX,OCI_DEFAULT);
		if (status!=OCI_SUCCESS) {
			printError("OCIStmtPrepare(lob)",status);
			return 1;
		}

		OCIDefine	*cdef=NULL;
		OCIDefine	*bdef=NULL;
		OCIDefine	*fdef=NULL;
		sb2		cind=0;
		sb2		bind=0;
		sb2		find=0;
		if (OCIDefineByPos(lobstmt,&cdef,err,1,&clob,
				(sb4)sizeof(OCILobLocator *),SQLT_CLOB,
				&cind,NULL,NULL,OCI_DEFAULT)!=OCI_SUCCESS ||
			OCIDefineByPos(lobstmt,&bdef,err,2,&blob,
				(sb4)sizeof(OCILobLocator *),SQLT_BLOB,
				&bind,NULL,NULL,OCI_DEFAULT)!=OCI_SUCCESS ||
			OCIDefineByPos(lobstmt,&fdef,err,3,&bfile,
				(sb4)sizeof(OCILobLocator *),SQLT_BFILE,
				&find,NULL,NULL,OCI_DEFAULT)!=OCI_SUCCESS) {
			printError("OCIDefineByPos(lob)",OCI_ERROR);
			return 1;
		}

		status=OCIStmtExecute(svc,lobstmt,err,1,0,
					NULL,NULL,OCI_DEFAULT);
		if (status!=OCI_SUCCESS && status!=OCI_SUCCESS_WITH_INFO) {
			printError("OCIStmtExecute(lob)",status);
			return 1;
		}
		stdoutput.printf("got locators (indicators %d/%d/%d)\n",
					(int)cind,(int)bind,(int)find);

		// get length of each
		ub4	clen=0;
		status=OCILobGetLength(svc,err,clob,&clen);
		if (status!=OCI_SUCCESS) {
			printError("OCILobGetLength(clob)",status);
		} else {
			stdoutput.printf("clob length: %d\n",(int)clen);
		}

		ub4	blen=0;
		status=OCILobGetLength(svc,err,blob,&blen);
		if (status!=OCI_SUCCESS) {
			printError("OCILobGetLength(blob)",status);
		} else {
			stdoutput.printf("blob length: %d\n",(int)blen);
		}

		// read the whole clob in one call
		char	cbuffer[4096];
		bytestring::zero(cbuffer,sizeof(cbuffer));
		ub4	camt=clen;
		status=OCILobRead(svc,err,clob,&camt,1,
					cbuffer,sizeof(cbuffer)-1,
					NULL,NULL,0,SQLCS_IMPLICIT);
		if (status!=OCI_SUCCESS) {
			printError("OCILobRead(clob)",status);
		} else {
			stdoutput.printf("clob read %d chars, "
					"first 32: %.32s\n",
					(int)camt,cbuffer);
		}

		// read the blob a piece at a time, with an explicit
		// offset and amount for each piece, so each piece is its
		// own round trip.  (Handing OCILobRead amt 0 and a small
		// buffer gets OCI_NEED_DATA and repeated calls too, but
		// oci asks the server for the whole lob up front and
		// dribbles it out of its own buffer - one round trip, so
		// nothing new on the wire.)
		char	bbuffer[512];
		ub4	total=0;
		for (int piece=0; piece<4; piece++) {
			bytestring::zero(bbuffer,sizeof(bbuffer));
			ub4	bamt=500;
			status=OCILobRead(svc,err,blob,&bamt,piece*500+1,
						bbuffer,sizeof(bbuffer),
						NULL,NULL,0,SQLCS_IMPLICIT);
			total=total+bamt;
			stdoutput.printf("blob read piece %d "
					"(offset %d, amount 500): "
					"status %d, %d bytes\n",
					piece+1,piece*500+1,
					(int)status,(int)bamt);
			if (status!=OCI_SUCCESS &&
					status!=OCI_NEED_DATA) {
				printError("OCILobRead(blob)",status);
				break;
			}
		}
		stdoutput.printf("blob read %d bytes in 4 pieces\n",
					(int)total);

		// a bfile has no length in the row data, so this one
		// really does go to the server - the only GET LENGTH
		// request in the capture
		ub4	flen=0;
		status=OCILobGetLength(svc,err,bfile,&flen);
		if (status!=OCI_SUCCESS) {
			printError("OCILobGetLength(bfile)",status);
		} else {
			stdoutput.printf("bfile length: %d\n",(int)flen);
		}

		// bfile: get the name out of the locator and ask the
		// server whether the file is there
		char	dirbuffer[64];
		char	namebuffer[256];
		bytestring::zero(dirbuffer,sizeof(dirbuffer));
		bytestring::zero(namebuffer,sizeof(namebuffer));
		ub2	dirlen=sizeof(dirbuffer)-1;
		ub2	namelen=sizeof(namebuffer)-1;
		status=OCILobFileGetName(env,err,bfile,
					(text *)dirbuffer,&dirlen,
					(text *)namebuffer,&namelen);
		if (status!=OCI_SUCCESS) {
			printError("OCILobFileGetName",status);
		} else {
			stdoutput.printf("bfile name: %s / %s\n",
						dirbuffer,namebuffer);
		}

		boolean	exists=0;
		status=OCILobFileExists(svc,err,bfile,&exists);
		if (status!=OCI_SUCCESS) {
			printError("OCILobFileExists",status);
		} else {
			stdoutput.printf("bfile exists: %d\n",(int)exists);
		}

		// OCILobGetLength on a freshly selected lob is answered
		// out of the row data (the length comes back with the
		// locator), so it never reaches the server.  Opening the
		// lob first makes oci ask.  This also shows what an
		// explicit lob open and close put on the wire, as opposed
		// to OCIDescriptorFree below.
		stdoutput.printf("opening the clob...\n");
		status=OCILobOpen(svc,err,clob,OCI_LOB_READONLY);
		if (status!=OCI_SUCCESS) {
			printError("OCILobOpen",status);
		}
		ub4	clen2=0;
		status=OCILobGetLength(svc,err,clob,&clen2);
		if (status!=OCI_SUCCESS) {
			printError("OCILobGetLength(open clob)",status);
		} else {
			stdoutput.printf("clob length (opened): %d\n",
						(int)clen2);
		}
		status=OCILobClose(svc,err,clob);
		if (status!=OCI_SUCCESS) {
			printError("OCILobClose",status);
		}
		stdoutput.printf("clob closed\n");

		// a temporary lob: oci has no cached length for one of
		// these, so this is where a GET LENGTH request finally
		// shows up on the wire (and a CREATE TEMPORARY, a WRITE
		// and a FREE TEMPORARY with it)
		OCILobLocator	*tmplob=NULL;
		if (OCIDescriptorAlloc(env,(void **)&tmplob,
					OCI_DTYPE_LOB,0,NULL)!=OCI_SUCCESS) {
			stdoutput.printf("OCIDescriptorAlloc(tmp) failed\n");
			return 1;
		}
		stdoutput.printf("creating a temporary clob...\n");
		status=OCILobCreateTemporary(svc,err,tmplob,
					OCI_DEFAULT,SQLCS_IMPLICIT,
					OCI_TEMP_CLOB,OCI_ATTR_NOCACHE,
					OCI_DURATION_SESSION);
		if (status!=OCI_SUCCESS) {
			printError("OCILobCreateTemporary",status);
		} else {
			char	tmpdata[11];
			charstring::copy(tmpdata,"0123456789");
			ub4	tmpamt=10;
			status=OCILobWrite(svc,err,tmplob,&tmpamt,1,
						tmpdata,10,OCI_ONE_PIECE,
						NULL,NULL,0,SQLCS_IMPLICIT);
			if (status!=OCI_SUCCESS) {
				printError("OCILobWrite(tmp)",status);
			}
			ub4	tmplen=0;
			status=OCILobGetLength(svc,err,tmplob,&tmplen);
			if (status!=OCI_SUCCESS) {
				printError("OCILobGetLength(tmp)",status);
			} else {
				stdoutput.printf("temp clob length: %d\n",
							(int)tmplen);
			}
			status=OCILobFreeTemporary(svc,err,tmplob);
			if (status!=OCI_SUCCESS) {
				printError("OCILobFreeTemporary",status);
			}
		}
		OCIDescriptorFree(tmplob,OCI_DTYPE_LOB);

		// a lob too big for one tns packet, to see how the server
		// chunks the answer
		OCILobLocator	*bigclob=NULL;
		if (OCIDescriptorAlloc(env,(void **)&bigclob,
					OCI_DTYPE_LOB,0,NULL)!=OCI_SUCCESS) {
			stdoutput.printf("OCIDescriptorAlloc(big) failed\n");
			return 1;
		}

		OCIStmt	*bigstmt=NULL;
		if (OCIHandleAlloc(env,(void **)&bigstmt,
				OCI_HTYPE_STMT,0,NULL)!=OCI_SUCCESS) {
			stdoutput.printf("OCIHandleAlloc(bigstmt) failed\n");
			return 1;
		}

		const char	*bigquery=
			"select c from ociselectlob where id=2";
		stdoutput.printf("running: %s\n",bigquery);
		status=OCIStmtPrepare(bigstmt,err,(text *)bigquery,
					charstring::getLength(bigquery),
					OCI_NTV_SYNTAX,OCI_DEFAULT);
		if (status!=OCI_SUCCESS) {
			printError("OCIStmtPrepare(big)",status);
			return 1;
		}

		OCIDefine	*bigdef=NULL;
		sb2		bigind=0;
		if (OCIDefineByPos(bigstmt,&bigdef,err,1,&bigclob,
				(sb4)sizeof(OCILobLocator *),SQLT_CLOB,
				&bigind,NULL,NULL,OCI_DEFAULT)!=OCI_SUCCESS) {
			printError("OCIDefineByPos(big)",OCI_ERROR);
			return 1;
		}

		status=OCIStmtExecute(svc,bigstmt,err,1,0,
					NULL,NULL,OCI_DEFAULT);
		if (status!=OCI_SUCCESS && status!=OCI_SUCCESS_WITH_INFO) {
			printError("OCIStmtExecute(big)",status);
			return 1;
		}

		ub4	biglen=0;
		status=OCILobGetLength(svc,err,bigclob,&biglen);
		if (status!=OCI_SUCCESS) {
			printError("OCILobGetLength(big)",status);
		} else {
			stdoutput.printf("big clob length: %d\n",(int)biglen);
		}

		char	*bigbuffer=new char[biglen+1];
		bytestring::zero(bigbuffer,biglen+1);
		ub4	bigamt=biglen;
		status=OCILobRead(svc,err,bigclob,&bigamt,1,
					bigbuffer,biglen,
					NULL,NULL,0,SQLCS_IMPLICIT);
		stdoutput.printf("big clob read: status %d, %d chars\n",
					(int)status,(int)bigamt);
		if (status!=OCI_SUCCESS && status!=OCI_NEED_DATA) {
			printError("OCILobRead(big)",status);
		}
		delete[] bigbuffer;

		OCIDescriptorFree(bigclob,OCI_DTYPE_LOB);
		OCIHandleFree(bigstmt,OCI_HTYPE_STMT);

		// free the locators - a capture shows whether this puts
		// anything at all on the wire
		stdoutput.printf("freeing locators...\n");
		OCIDescriptorFree(clob,OCI_DTYPE_LOB);
		OCIDescriptorFree(blob,OCI_DTYPE_LOB);
		OCIDescriptorFree(bfile,OCI_DTYPE_FILE);
		stdoutput.printf("locators freed\n");

		OCIHandleFree(lobstmt,OCI_HTYPE_STMT);
	}

	stdoutput.printf("done\n");

	OCIHandleFree(stmt,OCI_HTYPE_STMT);
	OCILogoff(svc,err);

	return 0;
}
