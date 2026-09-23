// Copyright (c) David Muse
// See the file COPYING for more information.

// #10267 - a clob or blob column from a non-oracle backend came back NULL
// over the OCI8 wire protocol.  The fix sends those columns inline, shaped
// like a long / long raw.  This connects through the oracleprotocoldb2
// instance (test/sqlrelay.conf.d/oracleprotocol.conf.in), whose backend is
// db2, reads back a clob and a blob that are both well over 4000 bytes
// with plain SQLT_LNG / SQLT_LBI defines, and then reads back a null of
// each.  It connects with a literal connect descriptor built from
// ORACLEPROTOCOLPORT9 rather than a tnsnames.ora alias, so it needs no
// TNS_ADMIN.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/stringbuffer.h>
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
// shape as oci8.cpp's clearErrors(), so a tolerated failure (the pre-emptive
// "drop table" below) doesn't leave stale diagnostics for a later, unrelated
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
// execImmediate()
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

// pin a result set column's type, size, precision and scale
static void assertColumn(OCIStmt *stmt, ub4 pos,
				int type, int size, int precision, int scale) {

	OCIParam	*param=NULL;
	assertEquals(OCIParamGet(stmt,OCI_HTYPE_STMT,err,
					(void **)&param,pos),OCI_SUCCESS);

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

	// read into a ub2 and an sb2, not the documented ub1 and sb1 - see
	// oci8.cpp's assertColumn() for why
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

// append blob(x'...') literals for bytes start through end-1 of data to
// query, joined by ||, in chunks of at most chunksize bytes
static void appendBlobChunks(stringbuffer *query, const unsigned char *data,
					ub4 start, ub4 end, ub4 chunksize) {
	const char	*hex="0123456789ABCDEF";
	for (ub4 i=start; i<end; i+=chunksize) {
		if (i!=start) {
			query->append("||");
		}
		query->append("blob(x'");
		for (ub4 j=i; j<i+chunksize && j<end; j++) {
			query->append(hex[data[j]>>4]);
			query->append(hex[data[j]&0x0F]);
		}
		query->append("')");
	}
}

int main(int argc, char **argv) {

	const char	*port=environment::getValue("ORACLEPROTOCOLPORT9");
	if (charstring::isNullOrEmpty(port)) {
		port="1529";
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


	stdoutput.printf("=========== Inline clob and blob ===========\n\n");

	// build expected values
	const ub4	clobsize=20000;
	const ub4	blobsize=30001;
	char		*expectedclob=new char[clobsize];
	for (ub4 i=0; i<clobsize; i++) {
		expectedclob[i]=(char)('A'+(i%26));
	}
	unsigned char	*expectedblob=new unsigned char[blobsize];
	for (ub4 i=0; i<blobsize; i++) {
		expectedblob[i]=(unsigned char)(i%256);
	}

	stdoutput.printf("create table\n");
	execImmediate("drop table testinlinelob10267");
	assertEquals(
		execImmediate("create table testinlinelob10267 "
				"(id integer, c clob(1M), b blob(1M))"),
		OCI_SUCCESS);
	assertEquals(OCITransCommit(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	stdoutput.printf("\n\n");

	// A db2 string literal is capped at a few thousand bytes, so each
	// value is built from several concatenated literals.  The blob is
	// written in two statements to keep each one under sqlrelay's
	// default 64k maxquerysize.
	stdoutput.printf("insert rows\n");
	const ub4	clobchunksize=4000;
	stringbuffer	clobinsert;
	clobinsert.append("insert into testinlinelob10267 (id,c) values (1,");
	for (ub4 i=0; i<clobsize; i+=clobchunksize) {
		if (i) {
			clobinsert.append("||");
		}
		clobinsert.append("clob('");
		clobinsert.append(expectedclob+i,clobchunksize);
		clobinsert.append("')");
	}
	clobinsert.append(")");
	assertEquals(execImmediate(clobinsert.getString()),OCI_SUCCESS);

	const ub4	blobchunksize=2000;
	const ub4	blobhalf=16000;
	stringbuffer	blobupdate1;
	blobupdate1.append("update testinlinelob10267 set b=");
	appendBlobChunks(&blobupdate1,expectedblob,
					0,blobhalf,blobchunksize);
	blobupdate1.append(" where id=1");
	assertEquals(execImmediate(blobupdate1.getString()),OCI_SUCCESS);

	stringbuffer	blobupdate2;
	blobupdate2.append("update testinlinelob10267 set b=b||");
	appendBlobChunks(&blobupdate2,expectedblob,
					blobhalf,blobsize,blobchunksize);
	blobupdate2.append(" where id=1");
	assertEquals(execImmediate(blobupdate2.getString()),OCI_SUCCESS);

	assertEquals(
		execImmediate("insert into testinlinelob10267 "
				"values (2,NULL,NULL)"),
		OCI_SUCCESS);
	assertEquals(OCITransCommit(svc,err,OCI_DEFAULT),OCI_SUCCESS);
	stdoutput.printf("\n\n");

	stdoutput.printf("select - describe\n");
	OCIStmt	*stmt=NULL;
	assertEquals(
		OCIHandleAlloc(env,(void **)&stmt,OCI_HTYPE_STMT,0,NULL),
		OCI_SUCCESS);
	const char	*query="select id, c, b from testinlinelob10267 "
								"order by id";
	assertEquals(
		OCIStmtPrepare(stmt,err,(text *)query,
				charstring::getLength(query),
				OCI_NTV_SYNTAX,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals(
		OCIStmtExecute(svc,stmt,err,0,0,NULL,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	assertColumn(stmt,2,SQLT_LNG,0,0,0);
	assertColumn(stmt,3,SQLT_LBI,0,0,0);
	stdoutput.printf("\n\n");

	stdoutput.printf("select - define\n");
	sb4		idvalue=0;
	sb2		idind=0;
	ub2		idlen=0;
	OCIDefine	*iddef=NULL;
	assertEquals(
		OCIDefineByPos(stmt,&iddef,err,1,
				&idvalue,sizeof(idvalue),SQLT_INT,
				&idind,&idlen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);

	const ub4	bufsize=65535;
	char		*clobvalue=new char[bufsize];
	sb2		clobind=0;
	ub2		cloblen=0;
	OCIDefine	*clobdef=NULL;
	assertEquals(
		OCIDefineByPos(stmt,&clobdef,err,2,
				clobvalue,bufsize,SQLT_LNG,
				&clobind,&cloblen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);

	unsigned char	*blobvalue=new unsigned char[bufsize];
	sb2		blobind=0;
	ub2		bloblen=0;
	OCIDefine	*blobdef=NULL;
	assertEquals(
		OCIDefineByPos(stmt,&blobdef,err,3,
				blobvalue,bufsize,SQLT_LBI,
				&blobind,&bloblen,NULL,OCI_DEFAULT),
		OCI_SUCCESS);
	stdoutput.printf("\n\n");

	stdoutput.printf("fetch - row 1, values\n");
	bytestring::zero(clobvalue,bufsize);
	bytestring::zero(blobvalue,bufsize);
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)idind,OCI_IND_NOTNULL);
	assertEquals((int)idvalue,1);
	assertEquals((int)clobind,OCI_IND_NOTNULL);
	assertEquals((int)cloblen,(int)clobsize);
	assertTrue(!bytestring::compare(clobvalue,expectedclob,clobsize));
	assertEquals((int)blobind,OCI_IND_NOTNULL);
	assertEquals((int)bloblen,(int)blobsize);
	assertTrue(!bytestring::compare(blobvalue,expectedblob,blobsize));
	stdoutput.printf("\n\n");

	stdoutput.printf("fetch - row 2, nulls\n");
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_SUCCESS);
	assertEquals((int)idind,OCI_IND_NOTNULL);
	assertEquals((int)idvalue,2);
	assertEquals((int)clobind,OCI_IND_NULL);
	assertEquals((int)blobind,OCI_IND_NULL);
	stdoutput.printf("\n\n");

	stdoutput.printf("fetch - no more rows\n");
	assertEquals(
		OCIStmtFetch2(stmt,err,1,OCI_FETCH_NEXT,0,OCI_DEFAULT),
		OCI_NO_DATA);
	stdoutput.printf("\n\n");

	OCIHandleFree(stmt,OCI_HTYPE_STMT);
	delete[] blobvalue;
	delete[] clobvalue;
	delete[] expectedblob;
	delete[] expectedclob;

	execImmediate("drop table testinlinelob10267");
	OCITransCommit(svc,err,OCI_DEFAULT);

	OCISessionEnd(svc,err,session,OCI_DEFAULT);
	OCIServerDetach(srv,err,OCI_DEFAULT);

	reportTestStatus();
	return status;
}
