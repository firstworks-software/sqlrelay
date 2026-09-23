// Copyright (c) David Muse
// See the file COPYING for more information.

// #10265 - sqlrprotocol_oracle::sendLobReadResponse (src/protocols/
// oracle.cpp) mixed up bytes and characters when the native oracle
// backend's client character set is multibyte.  Every clob is labeled
// AL32UTF8 (a varying-width character set) on the wire, and OCILobRead's
// amtp comes back in bytes rather than characters for a varying-width
// client character set - confirmed live on the ticket.  This connects
// through the oracleprotocolutf8 instance (test/sqlrelay.conf.d/
// oracleprotocol.conf.in), whose connection string sets
// nls_lang=AMERICAN_AMERICA.AL32UTF8 on the backend - the setting that
// actually makes the bug reproduce - and reads a multibyte clob spanning
// several protocol chunks (LOB_CHUNK_SIZE/2 = 4030 characters each) back
// in pieces, the way a real multibyte-NLS_LANG client would.  It connects
// with a literal connect descriptor built from ORACLEPROTOCOLPORT8 rather
// than a tnsnames.ora alias, so it needs no TNS_ADMIN.

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

// discard whatever diagnostic is sitting in the shared error handle - same
// shape as oci8.cpp's clearErrors(), duplicated here for the same reason:
// OCIErrorGet doesn't clear what it reads, so a tolerated failure (the
// pre-emptive "drop table" below, expected to fail when the table is already
// gone) would otherwise leave stale diagnostics for a later, unrelated
// failure to get blamed on
static void clearErrors() {
	OCIError	*newerr=NULL;
	if (OCIHandleAlloc(env,(void **)&newerr,
				OCI_HTYPE_ERROR,0,NULL)==OCI_SUCCESS) {
		OCIHandleFree(err,OCI_HTYPE_ERROR);
		err=newerr;
	}
}

// run a statement, discarding whatever it returns - same shape as oci8.cpp's
// execImmediate(), duplicated here since this is its own small standalone
// program rather than another target inside oci8.cpp
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

int main(int argc, char **argv) {

	// the client's own NLS_LANG also has to be a multibyte charset - the
	// listener always frames a clob's wire bytes as utf-16be, and a
	// single-byte client charset would have OCI transliterate them down
	// to '?' on the way into the read buffer (as WE8ISO8859P1 does for
	// the euro sign below), which would defeat the byte-for-byte compare
	environment::setValue("NLS_LANG","AMERICAN_AMERICA.AL32UTF8");

	const char	*port=environment::getValue("ORACLEPROTOCOLPORT8");
	if (charstring::isNullOrEmpty(port)) {
		port="1528";
	}
	char	connectstr[256];
	charstring::printf(connectstr,sizeof(connectstr),
			"(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)"
			"(HOST=localhost)(PORT=%s))"
			"(CONNECT_DATA=(SID=ora1)))",port);

	stdoutput.printf("\n=============== Connect ==============\n\n");

	assertEquals(
		OCIEnvCreate((OCIEnv **)&env,OCI_DEFAULT|OCI_OBJECT,
				NULL,NULL,NULL,NULL,0,NULL),
		OCI_SUCCESS);
	assertTrue(env!=NULL);

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

	sword	attached=OCIServerAttach(srv,err,(text *)connectstr,
					charstring::getLength(connectstr),0);
	assertEquals(attached,OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(svc,OCI_HTYPE_SVCCTX,srv,0,OCI_ATTR_SERVER,err),
		OCI_SUCCESS);
	if (attached!=OCI_SUCCESS) {
		reportTestStatus();
		return status;
	}

	const char	*user="testuser";
	const char	*password="testpassword";
	assertEquals(
		OCIAttrSet(session,OCI_HTYPE_SESSION,
				(void *)user,charstring::getLength(user),
				OCI_ATTR_USERNAME,err),
		OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(session,OCI_HTYPE_SESSION,
				(void *)password,charstring::getLength(password),
				OCI_ATTR_PASSWORD,err),
		OCI_SUCCESS);
	sword	loggedin=OCISessionBegin(svc,err,session,
					OCI_CRED_RDBMS,OCI_DEFAULT);
	assertEquals(loggedin,OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(svc,OCI_HTYPE_SVCCTX,session,0,OCI_ATTR_SESSION,err),
		OCI_SUCCESS);
	if (loggedin!=OCI_SUCCESS) {
		reportTestStatus();
		return status;
	}

	assertEquals(
		OCIHandleAlloc(env,(void **)&trans,OCI_HTYPE_TRANS,0,NULL),
		OCI_SUCCESS);
	assertEquals(
		OCIAttrSet(svc,OCI_HTYPE_SVCCTX,trans,0,OCI_ATTR_TRANS,err),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");


	stdoutput.printf("=========== Multibyte clob, in pieces ===========\n\n");

	// 3000 repeats of 'a' + U+00E9 (e-acute, 2 utf-8 bytes) + U+20AC
	// (euro sign, 3 utf-8 bytes) = 9000 characters, 18000 utf-8 bytes -
	// several times LOB_CHUNK_SIZE/2 (4030 characters), so both a single
	// OCILobRead call (which the server answers in several internal wire
	// chunks) and two separate OCILobRead calls at a non-chunk-aligned
	// split get exercised
	const ub4	repeats=3000;
	const ub4	charsperrepeat=3;
	const ub4	bytesperrepeat=6;
	const ub4	totalchars=repeats*charsperrepeat;
	const ub4	totalbytes=repeats*bytesperrepeat;

	unsigned char	*expected=new unsigned char[totalbytes];
	for (ub4 r=0; r<repeats; r++) {
		unsigned char	*p=expected+(r*bytesperrepeat);
		p[0]=0x61;			// 'a'
		p[1]=0xC3; p[2]=0xA9;		// U+00E9
		p[3]=0xE2; p[4]=0x82; p[5]=0xAC;	// U+20AC
	}

	stdoutput.printf("create table\n");
	execImmediate("drop table utf8clobtest");
	assertEquals(
		execImmediate("create table utf8clobtest (testclob clob)"),
		OCI_SUCCESS);
	assertEquals(
		execImmediate("insert into utf8clobtest values (empty_clob())"),
		OCI_SUCCESS);
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
				"update utf8clobtest set testclob=c; "
				"dbms_lob.freetemporary(c); "
			"end;",(int)repeats,(int)charsperrepeat);
	assertEquals(execImmediate(seedsql),OCI_SUCCESS);
	assertEquals(execImmediate("commit"),OCI_SUCCESS);
	stdoutput.printf("\n\n");

	stdoutput.printf("select the clob\n");
	OCIStmt	*lobstmt=NULL;
	OCIHandleAlloc(env,(void **)&lobstmt,OCI_HTYPE_STMT,0,NULL);
	const char	*lobquery="select testclob from utf8clobtest "
						"for update";
	assertEquals(
		OCIStmtPrepare(lobstmt,err,(text *)lobquery,
				charstring::getLength(lobquery),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);

	OCILobLocator	*cloblocator=NULL;
	OCIDescriptorAlloc(env,(void **)&cloblocator,OCI_DTYPE_LOB,0,NULL);
	OCIDefine	*def=NULL;
	sb2		ind=0;
	assertEquals(
		OCIDefineByPos(lobstmt,&def,err,1,&cloblocator,0,SQLT_CLOB,
				&ind,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,lobstmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtFetch2(lobstmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)ind,OCI_IND_NOTNULL);
	stdoutput.printf("\n\n");

	stdoutput.printf("OCILobGetLength\n");
	ub4	loblength=0;
	assertEquals(
		OCILobGetLength(svc,err,cloblocator,&loblength),OCI_SUCCESS);
	assertEquals((int)loblength,(int)totalchars);
	stdoutput.printf("\n\n");

	stdoutput.printf("OCILobRead - in two pieces, "
				"not aligned on the 4030-character "
				"protocol chunk size\n");
	const ub4	firsthalfchars=4500;	// not a multiple of 4030
	const ub4	firsthalfbytes=(firsthalfchars/charsperrepeat)*
						bytesperrepeat;
	const ub4	secondhalfchars=totalchars-firsthalfchars;
	const ub4	secondhalfbytes=totalbytes-firsthalfbytes;

	// OCI requires the buffer to be sized for the worst case - up to
	// MAX_BYTES_PER_CHAR (4, matching src/protocols/oracle.cpp's own
	// constant) bytes per character requested - regardless of how many
	// bytes this data actually takes.  a buffer sized to just the actual
	// expected byte count (9000 for 4500 characters of this 2-bytes/char
	// average data) makes OCILobRead return OCI_NEED_DATA (99) instead of
	// OCI_SUCCESS, since OCI has no way to know in advance that the real
	// data will fit
	const ub4	maxbytesperchar=4;
	unsigned char	*readbuf=new unsigned char[totalchars*maxbytesperchar];
	bytestring::zero(readbuf,totalchars*maxbytesperchar);

	// amtp-out (clobamount here) itself comes back in bytes rather than
	// characters, under this same multibyte-client-charset rule - #10265
	// is about the native oracle backend's own OCILobRead call inside
	// sqlr-connection-oracle, not this test client's, but the OCI client
	// library used here is the very same one, so it shows the identical
	// behavior.  what's actually under test is that the wire bytes this
	// client decodes come out byte-for-byte and character-boundary
	// correct - the position/charsread fix inside sendLobReadResponse
	ub4	clobamount=firsthalfchars;
	assertEquals(
		OCILobRead(svc,err,cloblocator,&clobamount,1,
				readbuf,firsthalfchars*maxbytesperchar,
				NULL,NULL,0,SQLCS_IMPLICIT),
		OCI_SUCCESS);
	assertEquals((int)clobamount,(int)firsthalfbytes);
	assertTrue(!bytestring::compare(readbuf,expected,firsthalfbytes));

	bytestring::zero(readbuf,totalchars*maxbytesperchar);
	clobamount=secondhalfchars;
	assertEquals(
		OCILobRead(svc,err,cloblocator,&clobamount,firsthalfchars+1,
				readbuf,secondhalfchars*maxbytesperchar,
				NULL,NULL,0,SQLCS_IMPLICIT),
		OCI_SUCCESS);
	assertEquals((int)clobamount,(int)secondhalfbytes);
	assertTrue(!bytestring::compare(readbuf,expected+firsthalfbytes,
						secondhalfbytes));
	stdoutput.printf("\n\n");

	delete[] readbuf;

	OCIDescriptorFree(cloblocator,OCI_DTYPE_LOB);
	OCIHandleFree(lobstmt,OCI_HTYPE_STMT);
	delete[] expected;

	execImmediate("drop table utf8clobtest");
	execImmediate("commit");

	OCISessionEnd(svc,err,session,OCI_DEFAULT);
	OCIServerDetach(srv,err,OCI_DEFAULT);

	reportTestStatus();
	return status;
}
