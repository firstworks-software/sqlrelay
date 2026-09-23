// Copyright (c) David Muse
// See the file COPYING for more information.

// #10272 - the OCI7 counterpart to oracleinlinelob.cpp (#10267).  That
// program reads a clob and a blob column from a non-oracle (db2) backend over
// the OCI8 wire protocol, where they're sent inline, shaped like a long and a
// long raw.  This one covers the same columns through the legacy OCI7 calls,
// against the oracleprotocoloci7db2 instance (test/sqlrelay.conf.d/
// oracleprotocol.conf.in), with a literal connect descriptor built from
// ORACLEPROTOCOLPORT11 rather than a tnsnames.ora alias, so it needs no
// TNS_ADMIN.
//
// A real OCI7 client files what a describe returns under the position it
// asked for, and one describe at position N answers for columns N through
// the end of the select list.  So the describes below run in several
// out-of-order sequences, each on a fresh cursor, so that the first describe
// of each sequence goes out on the wire for a column other than the first.
// An in-order describe of 1, 2, 3 can't tell a server that answers from the
// requested position from one that always answers from column 1.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/stringbuffer.h>
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

// ORA-01403, no data found
#define OCI7_NO_DATA	1403

// ORA-01007, variable not in select list
#define OCI7_NOT_IN_SELECT_LIST	1007

Lda_Def		lda;
ub4		hda[256];

const char	*user="testuser";
const char	*password="testpassword";

int	status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

// the ORA number the last call left in a cursor's cda.rc, stashed by
// check()/errorCode() so a failing assertion can print it
int	lasterror=0;

static void printErrors() {
	if (!lasterror) {
		return;
	}
	stdoutput.printf("\nORA-%05d\n",lasterror);
}

static void assertEquals(const char *actual, const char *expected) {
	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
		printErrors();
		status=1;
	}
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

// remember the ORA number a call left behind
static sword check(Cda_Def *cursor, sword result) {
	lasterror=(int)cursor->rc;
	return result;
}

// the ORA number of the most recent failure on this cursor
static int errorCode(Cda_Def *cursor) {
	lasterror=(int)cursor->rc;
	return lasterror;
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

// the select list every describe and fetch below runs against, and what
// each of its columns should describe as - a db2 integer as a NUMBER(10),
// and the clob and blob as a LONG and a LONG RAW
const char	*query="select id, c, b from oci7inlinelobtest order by id";

struct expectedcolumn {
	const char	*name;
	int		type;
	int		size;
	int		precision;
	int		scale;
};

const expectedcolumn	expected[]={
	{"ID",2,22,10,0},
	{"C",8,0,0,0},
	{"B",24,0,0,0}
};

const sword	colcount=3;

// pin one column's name, type, size, precision and scale
static void assertColumn(Cda_Def *cursor, sword pos) {

	sb4	dbsize=0;
	sb2	dbtype=0;
	sb1	cbuf[128];
	sb4	cbufl=(sb4)sizeof(cbuf);
	sb4	dsize=0;
	sb2	precision=0;
	sb2	scale=0;
	sb2	nullok=0;
	bytestring::zero(cbuf,sizeof(cbuf));

	assertEquals(check(cursor,
			odescr(cursor,pos,&dbsize,&dbtype,cbuf,&cbufl,
					&dsize,&precision,&scale,&nullok)),0);

	// odescr doesn't null terminate the name
	if (cbufl>=0 && cbufl<(sb4)sizeof(cbuf)) {
		cbuf[cbufl]='\0';
	} else {
		cbuf[sizeof(cbuf)-1]='\0';
	}

	const expectedcolumn	*e=&expected[pos-1];
	assertEquals((const char *)cbuf,e->name);
	assertEquals((int)cbufl,(int)charstring::getLength(e->name));
	assertEquals((int)dbtype,e->type);
	assertEquals((int)dbsize,e->size);
	assertEquals((int)precision,e->precision);
	assertEquals((int)scale,e->scale);
}

// describe the select list's columns in the given order, on a fresh cursor,
// then show that the position past the end fails
static void describeInOrder(const char *order) {

	stdoutput.printf("odescr - order %s\n",order);

	Cda_Def	cda;
	assertEquals(check(&cda,openCursor(&cda)),0);
	assertEquals(check(&cda,
			oparse(&cda,(text *)query,(sb4)-1,0,(ub4)2)),0);

	for (const char *o=order; *o; o++) {
		assertColumn(&cda,(sword)(*o-'0'));
	}

	sb4	dbsize=0;
	sb2	dbtype=0;
	sb1	cbuf[128];
	sb4	cbufl=(sb4)sizeof(cbuf);
	sb4	dsize=0;
	sb2	precision=0;
	sb2	scale=0;
	sb2	nullok=0;
	assertTrue(odescr(&cda,colcount+1,&dbsize,&dbtype,cbuf,&cbufl,
				&dsize,&precision,&scale,&nullok)!=0);
	assertEquals(errorCode(&cda),OCI7_NOT_IN_SELECT_LIST);

	assertEquals(check(&cda,oclose(&cda)),0);
	stdoutput.printf("\n\n");
}

int main(int argc, char **argv) {

	const char	*port=environment::getValue("ORACLEPROTOCOLPORT11");
	if (charstring::isNullOrEmpty(port)) {
		port="1531";
	}
	// ORACLEPROTOCOLHOST is unset in a normal run.  it exists for a real
	// OCI7 client (redhat9x86) pointed at a listener on another host
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


	stdoutput.printf("====== Inline clob and blob ======\n\n");

	// build expected values
	const ub4	clobsize=20000;
	const ub4	blobsize=30001;
	char		*expectedclob=new char[clobsize+1];
	for (ub4 i=0; i<clobsize; i++) {
		expectedclob[i]=(char)('A'+(i%26));
	}
	expectedclob[clobsize]='\0';
	unsigned char	*expectedblob=new unsigned char[blobsize];
	for (ub4 i=0; i<blobsize; i++) {
		expectedblob[i]=(unsigned char)(i%256);
	}

	stdoutput.printf("create table\n");
	execImmediate("drop table oci7inlinelobtest");
	assertEquals((int)
		execImmediate("create table oci7inlinelobtest "
				"(id integer, c clob(1M), b blob(1M))"),0);
	assertEquals((int)execImmediate("commit"),0);
	stdoutput.printf("\n\n");

	// A db2 string literal is capped at a few thousand bytes, so each
	// value is built from several concatenated literals.  The blob is
	// written in two statements to keep each one under sqlrelay's
	// default 64k maxquerysize.
	stdoutput.printf("insert rows\n");
	const ub4	clobchunksize=4000;
	stringbuffer	clobinsert;
	clobinsert.append("insert into oci7inlinelobtest (id,c) values (1,");
	for (ub4 i=0; i<clobsize; i+=clobchunksize) {
		if (i) {
			clobinsert.append("||");
		}
		clobinsert.append("clob('");
		clobinsert.append(expectedclob+i,clobchunksize);
		clobinsert.append("')");
	}
	clobinsert.append(")");
	assertEquals((int)execImmediate(clobinsert.getString()),0);

	const ub4	blobchunksize=2000;
	const ub4	blobhalf=16000;
	stringbuffer	blobupdate1;
	blobupdate1.append("update oci7inlinelobtest set b=");
	appendBlobChunks(&blobupdate1,expectedblob,0,blobhalf,blobchunksize);
	blobupdate1.append(" where id=1");
	assertEquals((int)execImmediate(blobupdate1.getString()),0);

	stringbuffer	blobupdate2;
	blobupdate2.append("update oci7inlinelobtest set b=b||");
	appendBlobChunks(&blobupdate2,expectedblob,
					blobhalf,blobsize,blobchunksize);
	blobupdate2.append(" where id=1");
	assertEquals((int)execImmediate(blobupdate2.getString()),0);

	assertEquals((int)
		execImmediate("insert into oci7inlinelobtest "
				"values (2,NULL,NULL)"),0);
	assertEquals((int)execImmediate("commit"),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("=============== Describe ===============\n\n");

	describeInOrder("123");
	describeInOrder("23");
	describeInOrder("3");
	describeInOrder("213");
	describeInOrder("321");
	describeInOrder("132");
	describeInOrder("31");


	stdoutput.printf("================ Fetch =================\n\n");

	char		idvalue[32];
	char		*clobvalue=new char[clobsize+64];
	unsigned char	*blobvalue=new unsigned char[blobsize+64];
	sb2		ind[3];
	ub2		len[3];
	ub2		code[3];

	// describing from the second column onward is what a real client
	// does once it reaches the first lob column
	stdoutput.printf("odescr, odefin, oexec, ofen\n");
	Cda_Def	cda;
	assertEquals(check(&cda,openCursor(&cda)),0);
	assertEquals(check(&cda,
			oparse(&cda,(text *)query,(sb4)-1,0,(ub4)2)),0);
	assertColumn(&cda,2);
	assertColumn(&cda,3);
	bytestring::zero(idvalue,sizeof(idvalue));
	bytestring::zero(clobvalue,clobsize+64);
	bytestring::zero(blobvalue,blobsize+64);
	bytestring::zero(ind,sizeof(ind));
	bytestring::zero(len,sizeof(len));
	bytestring::zero(code,sizeof(code));
	assertEquals(check(&cda,
			odefin(&cda,1,(ub1 *)idvalue,(sword)sizeof(idvalue),
				SQLT_STR,-1,&ind[0],(text *)0,-1,-1,
				&len[0],&code[0])),0);
	assertEquals(check(&cda,
			odefin(&cda,2,(ub1 *)clobvalue,(sword)(clobsize+64),
				SQLT_STR,-1,&ind[1],(text *)0,-1,-1,
				&len[1],&code[1])),0);
	assertEquals(check(&cda,
			odefin(&cda,3,blobvalue,(sword)(blobsize+64),
				SQLT_BIN,-1,&ind[2],(text *)0,-1,-1,
				&len[2],&code[2])),0);
	assertEquals(check(&cda,oexec(&cda)),0);
	stdoutput.printf("\n\n");

	stdoutput.printf("ofen - row 1, values\n");
	assertEquals(check(&cda,ofen(&cda,1)),0);
	assertEquals((int)ind[0],0);
	assertEquals((const char *)idvalue,"1");
	assertEquals((int)ind[1],0);
	assertEquals((const char *)clobvalue,(const char *)expectedclob);
	assertEquals((int)ind[2],0);
	assertEquals((int)len[2],(int)blobsize);
	assertTrue(!bytestring::compare(blobvalue,expectedblob,blobsize));
	stdoutput.printf("\n\n");

	stdoutput.printf("ofen - row 2, nulls\n");
	assertEquals(check(&cda,ofen(&cda,1)),0);
	assertEquals((int)ind[0],0);
	assertEquals((const char *)idvalue,"2");
	assertEquals((int)ind[1],-1);
	assertEquals((int)ind[2],-1);
	stdoutput.printf("\n\n");

	stdoutput.printf("ofen - no more rows\n");
	assertTrue(ofen(&cda,1)!=0);
	assertEquals(errorCode(&cda),OCI7_NO_DATA);
	stdoutput.printf("\n\n");

	// the same select through the other legacy fetch shape, with the
	// describe starting at the last column this time
	stdoutput.printf("odescr, odefin, oexfet\n");
	assertEquals(check(&cda,
			oparse(&cda,(text *)query,(sb4)-1,0,(ub4)2)),0);
	assertColumn(&cda,3);
	assertColumn(&cda,1);
	assertColumn(&cda,2);
	bytestring::zero(idvalue,sizeof(idvalue));
	bytestring::zero(clobvalue,clobsize+64);
	bytestring::zero(blobvalue,blobsize+64);
	bytestring::zero(ind,sizeof(ind));
	bytestring::zero(len,sizeof(len));
	bytestring::zero(code,sizeof(code));
	assertEquals(check(&cda,
			odefin(&cda,1,(ub1 *)idvalue,(sword)sizeof(idvalue),
				SQLT_STR,-1,&ind[0],(text *)0,-1,-1,
				&len[0],&code[0])),0);
	assertEquals(check(&cda,
			odefin(&cda,2,(ub1 *)clobvalue,(sword)(clobsize+64),
				SQLT_STR,-1,&ind[1],(text *)0,-1,-1,
				&len[1],&code[1])),0);
	assertEquals(check(&cda,
			odefin(&cda,3,blobvalue,(sword)(blobsize+64),
				SQLT_BIN,-1,&ind[2],(text *)0,-1,-1,
				&len[2],&code[2])),0);
	assertEquals(check(&cda,oexfet(&cda,(ub4)1,0,1)),0);
	assertEquals((int)cda.rpc,1);
	assertEquals((int)ind[0],0);
	assertEquals((const char *)idvalue,"1");
	assertEquals((int)ind[1],0);
	assertEquals((const char *)clobvalue,(const char *)expectedclob);
	assertEquals((int)ind[2],0);
	assertEquals((int)len[2],(int)blobsize);
	assertTrue(!bytestring::compare(blobvalue,expectedblob,blobsize));
	stdoutput.printf("\n\n");

	assertEquals(check(&cda,oclose(&cda)),0);
	stdoutput.printf("\n\n");

	delete[] blobvalue;
	delete[] clobvalue;
	delete[] expectedblob;
	delete[] expectedclob;

	execImmediate("drop table oci7inlinelobtest");
	execImmediate("commit");

	ologof(&lda);

	reportTestStatus();
	return status;
}
