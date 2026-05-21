// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/bytestring.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	const char	*isolationlevels[]={"0","1",NULL};
	const char	*subvars[]={"var1","var2","var3",NULL};
	const char	*subvalstrings[]={"hi","hello","bye"};
	int64_t		subvallongs[]={1,2,3};
	double		subvaldoubles[]={10.55,10.556,10.5556};
	uint32_t	precs[]={4,5,6};
	uint32_t	scales[]={2,3,4};
	const char * const *cols;
	const char * const *fields;
	uint16_t	port;
	char		*socket;
	uint16_t	id;
	char		*filename;
	uint64_t	counter=0;
	uint32_t	*fieldlens;

	#define	LARGE_BUFFER_LENGTH	8192
	char		largebuffer[LARGE_BUFFER_LENGTH+1];


	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"sqlite");
	stdoutput.printf("\n");


	// db version
	stdoutput.printf("DB VERSION: \n");
	const char	*dbversion=con->dbVersion();
	bool		issqlite3=true;
	if (!dbversion ||
		!charstring::compare(dbversion,"unknown") ||
		charstring::convertToInteger(dbversion)<3) {
		issqlite3=false;
	}
	stdoutput.printf("\n");


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// transaction state
	stdoutput.printf("TRANSACTION STATE: \n");
	assertEquals(con->getDefaultTransactionModel(),"explicit");
	assertEquals(con->getTransactionModel(),"explicit");
	assertFalse(con->getInTransaction());
	assertTrue(con->getAutoCommit());
	stdoutput.printf("\n");


	// bind format
	stdoutput.printf("BIND FORMAT: \n");
	assertEquals(con->bindFormat(),":*");
	stdoutput.printf("\n");


	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"");
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	for (const char **il=isolationlevels; *il; il++) {
		assertTrue(con->setIsolationLevel(*il));
		assertEquals(con->getIsolationLevel(),*il);
		stdoutput.printf("\n");
	}
	// reset to the default isolation level
	assertTrue(con->setIsolationLevel(isolationlevels[0]));
	stdoutput.printf("\n");


	// create testtable
	stdoutput.printf("CREATE TESTTABLE: \n");
	con->begin();
	cur->sendQuery("drop table if exists testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testclob clob, "
		"	testblob blob)"));
	con->commit();
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(con->begin());
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1.1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'testclob1', "
		"	'testblob1')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2.2, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'testclob2', "
		"	'testblob2')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3.3, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'testclob3', "
		"	'testblob3')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4.4, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'testclob4', "
		"	'testblob4')"));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),1);
	stdoutput.printf("\n");


	// input bind by position
	// sqlite doesn't support bind by position


	// array of input binds by position
	// sqlite doesn't support bind by position


	// input bind by position with validation
	// sqlite doesn't support bind by position


	// input bind by name
	stdoutput.printf("INPUT BIND BY NAME: \n");
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4, "
		"	:var5, "
		"	:var6)");
	assertEquals(cur->countBindVariables(),6);
	cur->inputBind("var1",5);
	cur->inputBind("var2",5.5,4,1);
	cur->inputBind("var3","testchar5");
	cur->inputBind("var4","testvarchar5");
	cur->inputBindClob("var5","testclob5",9);
	cur->inputBindBlob("var6","testblob5",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("var1",6);
	cur->inputBind("var2",6.6,4,1);
	cur->inputBind("var3","testchar6");
	cur->inputBind("var4","testvarchar6");
	cur->inputBindClob("var5","testclob6",9);
	cur->inputBindBlob("var6","testblob6",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("var1",7);
	cur->inputBind("var2",7.7,4,1);
	cur->inputBind("var3","testchar7");
	cur->inputBind("var4","testvarchar7");
	cur->inputBindClob("var5","testclob7",9);
	cur->inputBindBlob("var6","testblob7",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of input binds by name
	// sqlite doesn't support implicit conversion of string binds to other
	// data types, so arrays of binds don't generally work.


	// input bind by name with validation
	stdoutput.printf("INPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("var1",8);
	cur->inputBind("var2",8.8,4,1);
	cur->inputBind("var3","testchar8");
	cur->inputBind("var4","testvarchar8");
	cur->inputBindClob("var5","testclob8",9);
	cur->inputBindBlob("var6","testblob8",9);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),6);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testchar");
	assertEquals(cur->getColumnName(3),"testvarchar");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testchar");
	assertEquals(cols[3],"testvarchar");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	if (issqlite3) {
		assertEquals(cur->getColumnType((uint32_t)0),"INTEGER");
		assertEquals(cur->getColumnType("testint"),"INTEGER");
		assertEquals(cur->getColumnType(1),"FLOAT");
		assertEquals(cur->getColumnType("testfloat"),"FLOAT");
		assertEquals(cur->getColumnType(2),"STRING");
		assertEquals(cur->getColumnType("testchar"),"STRING");
		assertEquals(cur->getColumnType(3),"STRING");
		assertEquals(cur->getColumnType("testvarchar"),"STRING");
		assertEquals(cur->getColumnType(4),"STRING");
		assertEquals(cur->getColumnType("testclob"),"STRING");
		assertEquals(cur->getColumnType(5),"STRING");
		assertEquals(cur->getColumnType("testblob"),"STRING");
	} else {
		assertEquals(cur->getColumnType((uint32_t)0),"UNKNOWN");
		assertEquals(cur->getColumnType("testint"),"UNKNOWN");
		assertEquals(cur->getColumnType(1),"UNKNOWN");
		assertEquals(cur->getColumnType("testfloat"),"UNKNOWN");
		assertEquals(cur->getColumnType(2),"UNKNOWN");
		assertEquals(cur->getColumnType("testchar"),"UNKNOWN");
		assertEquals(cur->getColumnType(3),"UNKNOWN");
		assertEquals(cur->getColumnType("testvarchar"),"UNKNOWN");
		assertEquals(cur->getColumnType(4),"UNKNOWN");
		assertEquals(cur->getColumnType("testclob"),"UNKNOWN");
		assertEquals(cur->getColumnType(5),"UNKNOWN");
		assertEquals(cur->getColumnType("testblob"),"UNKNOWN");
	}
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnLength("testint"),0);
	assertEquals(cur->getColumnLength(1),0);
	assertEquals(cur->getColumnLength("testfloat"),0);
	assertEquals(cur->getColumnLength(2),0);
	assertEquals(cur->getColumnLength("testchar"),0);
	assertEquals(cur->getColumnLength(3),0);
	assertEquals(cur->getColumnLength("testvarchar"),0);
	assertEquals(cur->getColumnLength(4),0);
	assertEquals(cur->getColumnLength("testclob"),0);
	assertEquals(cur->getColumnLength(5),0);
	assertEquals(cur->getColumnLength("testblob"),0);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest(1),3);
	assertEquals(cur->getLongest("testfloat"),3);
	assertEquals(cur->getLongest(2),9);
	assertEquals(cur->getLongest("testchar"),9);
	assertEquals(cur->getLongest(3),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(4),9);
	assertEquals(cur->getLongest("testclob"),9);
	assertEquals(cur->getLongest(5),9);
	assertEquals(cur->getLongest("testblob"),9);
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");


	// total rows
	stdoutput.printf("TOTAL ROWS: \n");
	assertEquals(cur->totalRows(),(issqlite3)?0:8);
	stdoutput.printf("\n");


	// first row index
	stdoutput.printf("FIRST ROW INDEX: \n");
	assertEquals(cur->firstRowIndex(),0);
	stdoutput.printf("\n");


	// end of result set
	stdoutput.printf("END OF RESULT SET: \n");
	assertTrue(cur->endOfResultSet());
	stdoutput.printf("\n");


	// fields by index
	stdoutput.printf("FIELDS BY INDEX: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"1.1");
	assertEquals(cur->getField(0,2),"testchar1");
	assertEquals(cur->getField(0,3),"testvarchar1");
	assertEquals(cur->getField(0,4),"testclob1");
	assertEquals(cur->getField(0,5),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8.8");
	assertEquals(cur->getField(7,2),"testchar8");
	assertEquals(cur->getField(7,3),"testvarchar8");
	assertEquals(cur->getField(7,4),"testclob8");
	assertEquals(cur->getField(7,5),"testblob8");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),3);
	assertEquals(cur->getFieldLength(0,2),9);
	assertEquals(cur->getFieldLength(0,3),12);
	assertEquals(cur->getFieldLength(0,4),9);
	assertEquals(cur->getFieldLength(0,5),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),3);
	assertEquals(cur->getFieldLength(7,2),9);
	assertEquals(cur->getFieldLength(7,3),12);
	assertEquals(cur->getFieldLength(7,4),9);
	assertEquals(cur->getFieldLength(7,5),9);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testfloat"),"1.1");
	assertEquals(cur->getField(0,"testchar"),"testchar1");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testclob"),"testclob1");
	assertEquals(cur->getField(0,"testblob"),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testfloat"),"8.8");
	assertEquals(cur->getField(7,"testchar"),"testchar8");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testclob"),"testclob8");
	assertEquals(cur->getField(7,"testblob"),"testblob8");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testfloat"),3);
	assertEquals(cur->getFieldLength(0,"testchar"),9);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testclob"),9);
	assertEquals(cur->getFieldLength(0,"testblob"),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testfloat"),3);
	assertEquals(cur->getFieldLength(7,"testchar"),9);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testclob"),9);
	assertEquals(cur->getFieldLength(7,"testblob"),9);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1.1");
	assertEquals(fields[2],"testchar1");
	assertEquals(fields[3],"testvarchar1");
	assertEquals(fields[4],"testclob1");
	assertEquals(fields[5],"testblob1");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],3);
	assertEquals(fieldlens[2],9);
	assertEquals(fieldlens[3],12);
	assertEquals(fieldlens[4],9);
	assertEquals(fieldlens[5],9);
	stdoutput.printf("\n");


	// result set buffer size
	stdoutput.printf("RESULT SET BUFFER SIZE: \n");
	assertEquals(cur->getResultSetBufferSize(),0);
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getResultSetBufferSize(),2);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),0);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),2);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),4);
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	cur->setResultSetBufferSize(0);
	stdoutput.printf("\n");


	// dont get column info
	stdoutput.printf("DONT GET COLUMN INFO: \n");
	cur->dontGetColumnInfo();
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getColumnName(0),NULL);
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),NULL);
	cur->getColumnInfo();
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),
				(issqlite3)?"INTEGER":"UNKNOWN");
	stdoutput.printf("\n");


	// suspended session
	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	delete[] socket;
	stdoutput.printf("\n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	assertEquals(cur->getField(3,(uint32_t)0),"4");
	assertEquals(cur->getField(4,(uint32_t)0),"5");
	assertEquals(cur->getField(5,(uint32_t)0),"6");
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");


	// suspended result set
	stdoutput.printf("SUSPENDED RESULT SET: \n");
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	id=cur->getResultSetId();
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	delete[] socket;
	assertTrue(cur->resumeResultSet(id));
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),4);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),6);
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	cur->setResultSetBufferSize(0);
	stdoutput.printf("\n");


	// cached result set
	stdoutput.printf("CACHED RESULT SET: \n");
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	delete[] filename;
	stdoutput.printf("\n");


	// column count for cached result set
	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),6);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testchar");
	assertEquals(cur->getColumnName(3),"testvarchar");
	assertEquals(cur->getColumnName(4),"testclob");
	assertEquals(cur->getColumnName(5),"testblob");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testchar");
	assertEquals(cols[3],"testvarchar");
	assertEquals(cols[4],"testclob");
	assertEquals(cols[5],"testblob");
	stdoutput.printf("\n");


	// cached result set with result set buffer size
	stdoutput.printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	delete[] filename;
	stdoutput.printf("\n");


	// from one cache file to another
	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");


	// from one cache file to another with result set buffer size
	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	stdoutput.printf("\n");


	// cached result set with suspend and result set buffer size
	stdoutput.printf("CACHED RESULT SET WITH SUSPEND "
				"AND RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	id=cur->getResultSetId();
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	stdoutput.printf("\n");
	assertTrue(con->resumeSession(port,socket));
	delete[] socket;
	assertTrue(cur->resumeCachedResultSet(id,filename));
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),4);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),6);
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	cur->cacheOff();
	stdoutput.printf("\n");
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	delete[] filename;
	stdoutput.printf("\n");


	// finished suspended session
	stdoutput.printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(4,(uint32_t)0),"5");
	assertEquals(cur->getField(5,(uint32_t)0),"6");
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	id=cur->getResultSetId();
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	delete[] socket;
	assertTrue(cur->resumeResultSet(id));
	assertEquals(cur->getField(4,(uint32_t)0),NULL);
	assertEquals(cur->getField(5,(uint32_t)0),NULL);
	assertEquals(cur->getField(6,(uint32_t)0),NULL);
	assertEquals(cur->getField(7,(uint32_t)0),NULL);
	stdoutput.printf("\n");


	// nested selects
	stdoutput.printf("NESTED SELECTS: \n");
	cur->setResultSetBufferSize(1);
	assertTrue(cur->sendQuery("select * from testtable"));
	secondcur=new sqlrcursor(con);
	secondcur->setResultSetBufferSize(1);
	for (uint32_t i=0; cur->getRow(i); i++) {
		assertTrue(secondcur->sendQuery("select * from testtable"));
	}
	delete secondcur;
	secondcur=NULL;
	cur->setResultSetBufferSize(0);
	assertTrue(cur->sendQuery("drop table if exists testtable"));
	stdoutput.printf("\n");


	// reset transaction state
	stdoutput.printf("RESET TRANSACTION STATE: \n");
	assertTrue(con->commit());
	assertEquals(con->getTransactionModel(),"explicit");
	assertTrue(con->getAutoCommit());
	stdoutput.printf("\n");


	// transaction behavior - implicit
	stdoutput.printf("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(con->setTransactionModel("implicit"));
	assertEquals(con->getTransactionModel(),"implicit");
	assertTrue(cur->sendQuery("create table testtable (col1 integer)"));
	// sqlite DDL is transactional; commit so the table is visible
	// to the second connection (the commit implicitly starts a new tx)
	assertTrue(con->commit());
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// session is in a transaction; insert is not visible until commit
	assertTrue(con->getInTransaction());
	assertFalse(con->getAutoCommit());
	assertTrue(cur->sendQuery("insert into testtable values (1)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	// commit makes it visible, and implicitly starts a new transaction
	assertTrue(con->commit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// rollback discards, and implicitly starts a new transaction
	assertTrue(cur->sendQuery("insert into testtable values (2)"));
	assertTrue(con->rollback());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	assertFalse(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable values (3)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	assertTrue(con->getInTransaction());
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// transaction behavior - explicit
	stdoutput.printf("TRANSACTION BEHAVIOR - explicit: \n");
	assertTrue(con->setTransactionModel("explicit"));
	assertEquals(con->getTransactionModel(),"explicit");
	assertTrue(cur->sendQuery("create table testtable (col1 integer)"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// begin starts a new transaction; insert is not visible until commit
	assertTrue(con->begin());
	assertTrue(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable values (1)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	// commit makes it visible; no new transaction is started
	assertTrue(con->commit());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// begin, insert, rollback discards; no new transaction is started
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable values (2)"));
	assertTrue(con->rollback());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	assertTrue(cur->sendQuery("insert into testtable values (3)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// transaction behavior - explicit-deferred
	stdoutput.printf("TRANSACTION BEHAVIOR - explicit-deferred: \n");
	assertTrue(con->setTransactionModel("explicit-deferred"));
	assertEquals(con->getTransactionModel(),"explicit-deferred");
	// switch to autocommit-on so the begin/commit cycles below
	// bracket explicit transactions (autocommit-off semantics are
	// exercised at the end of this block)
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	assertTrue(cur->sendQuery("create table testtable (col1 integer)"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// begin starts a transaction; commit makes it visible
	assertTrue(con->begin());
	assertTrue(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable values (1)"));
	assertTrue(con->commit());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// begin, insert, rollback discards
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable values (2)"));
	assertTrue(con->rollback());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// during a transaction started by begin(), autoCommitOn is a
	// no-op: the autocommit setting takes effect after the user
	// explicitly commits/rollbacks the tx (mysql-native semantic)
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable values (3)"));
	assertTrue(con->autoCommitOn());
	assertFalse(con->getAutoCommit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// explicit commit ends the tx; autocommit-on now takes effect
	assertTrue(con->commit());
	assertTrue(con->getAutoCommit());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"2");
	// autocommit is on; subsequent inserts are visible immediately
	assertTrue(cur->sendQuery("insert into testtable values (4)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"3");
	// autoCommitOff takes effect immediately when not in a transaction
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	// autocommit-off persists across commit/rollback; each commit or
	// rollback ends the current implicit tx and a new one starts for
	// the next statement
	assertTrue(cur->sendQuery("insert into testtable values (5)"));
	assertTrue(con->commit());
	assertFalse(con->getAutoCommit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"4");
	assertTrue(cur->sendQuery("insert into testtable values (6)"));
	assertTrue(con->rollback());
	assertFalse(con->getAutoCommit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"4");
	// autoCommitOff during a transaction changes the variable
	// immediately but the in-flight tx continues; only after the
	// next explicit commit/rollback does the new autocommit-off
	// setting drop us into a new implicit tx (mysql-asymmetric
	// semantic)
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable values (7)"));
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"4");
	assertTrue(con->commit());
	assertFalse(con->getAutoCommit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"5");
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// transaction behavior - explicit-error
	stdoutput.printf("TRANSACTION BEHAVIOR - explicit-error: \n");
	assertTrue(con->setTransactionModel("explicit-error"));
	assertEquals(con->getTransactionModel(),"explicit-error");
	assertTrue(cur->sendQuery("create table testtable (col1 integer)"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// begin, insert, commit
	assertTrue(con->begin());
	assertTrue(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable values (1)"));
	assertTrue(con->commit());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// begin, insert, rollback
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable values (2)"));
	assertTrue(con->rollback());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// while in a transaction, autoCommitOn/Off throw an error
	assertTrue(con->begin());
	assertFalse(con->autoCommitOn());
	assertFalse(con->autoCommitOff());
	assertTrue(con->commit());
	// outside of a transaction, autoCommitOn takes effect immediately
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	assertTrue(cur->sendQuery("insert into testtable values (3)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// transaction behavior - none
	stdoutput.printf("TRANSACTION BEHAVIOR - none: \n");
	assertTrue(con->setTransactionModel("none"));
	assertEquals(con->getTransactionModel(),"none");
	assertTrue(cur->sendQuery("create table testtable (col1 integer)"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// no transactions; everything is visible immediately
	assertTrue(con->getAutoCommit());
	assertFalse(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable values (1)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// commit and rollback are no-ops
	assertTrue(con->commit());
	assertTrue(cur->sendQuery("insert into testtable values (2)"));
	assertTrue(con->rollback());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"2");
	// autocommit is always on; autoCommitOff is an error
	assertFalse(con->autoCommitOff());
	assertTrue(con->getAutoCommit());
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// reset transaction behavior
	stdoutput.printf("RESET TRANSACTION BEHAVIOR: \n");
	assertTrue(con->setTransactionModel(con->getDefaultTransactionModel()));
	assertEquals(con->getTransactionModel(),"explicit");
	assertTrue(con->getAutoCommit());
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->sendQuery("drop table if exists testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int, "
		"	col2 char, "
		"	col3 float)"));
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))");
	cur->substitution("var1",1);
	cur->substitution("var2","hello");
	cur->substitution("var3",10.5556,6,4);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"10.5556");
	assertTrue(cur->sendQuery("delete from testtable"));
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)')");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	assertTrue(cur->sendQuery("delete from testtable"));
	stdoutput.printf("\n");
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3.0");
	assertTrue(cur->sendQuery("delete from testtable"));
	stdoutput.printf("\n");
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	assertTrue(cur->sendQuery("delete from testtable"));
	stdoutput.printf("\n");


	// nulls as nulls
	stdoutput.printf("NULLS AS NULLS: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	NULL, "
		"	NULL)"));
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"");
	assertEquals(cur->getField(0,2),"");
	assertTrue(cur->sendQuery("drop table if exists testtable"));
	stdoutput.printf("\n");


	// null and empty lobs
	stdoutput.printf("NULL AND EMPTY LOBS: \n");
	cur->getNullsAsNulls();
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)"));
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4)");
	cur->inputBindClob("var1","",0);
	cur->inputBindClob("var2",NULL,0);
	cur->inputBindBlob("var3","",0);
	cur->inputBindBlob("var4",NULL,0);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select * from testtable");
	assertEquals(cur->getField(0,(uint32_t)0),"");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),"");
	assertEquals(cur->getField(0,3),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// long lobs
	stdoutput.printf("LONG LOBS: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)");
	cur->prepareQuery("insert into testtable values (:clobval,:blobval)");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->inputBindClob("clobval",largebuffer,LARGE_BUFFER_LENGTH);
	cur->inputBindBlob("blobval",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select * from testtable");
	assertEquals(cur->getFieldLength(0,"testclob"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,"testclob"),largebuffer);
	assertEquals(cur->getFieldLength(0,"testblob"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,"testblob"),largebuffer,
						LARGE_BUFFER_LENGTH);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// output bind by position
	// sqlite doesn't support output binds


	// output bind by name
	// sqlite doesn't support output binds


	// output bind by name with validation
	// sqlite doesn't support output binds


	// lob output bind
	// sqlite doesn't support output binds


	// long output bind
	// sqlite doesn't support output binds


	// negative input bind
	stdoutput.printf("NEGATIVE INPUT BIND: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery("create table testtable (testval int)");
	cur->prepareQuery("insert into testtable values (:testval)");
	cur->inputBind("testval",-1);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testval from testtable");
	assertEquals(cur->getField(0,"testval"),"-1");
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// bind validation
	stdoutput.printf("BIND VALIDATION: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery(
		"create table testtable ("
		"	col1 varchar(20), "
		"	col2 varchar(20), "
		"	col3 varchar(20))");
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))");
	cur->inputBind("var1","1");
	cur->inputBind("var2","2");
	cur->inputBind("var3","3");
	cur->substitution("var1",":var1");
	assertTrue(cur->validBind("var1"));
	assertFalse(cur->validBind("var2"));
	assertFalse(cur->validBind("var3"));
	assertFalse(cur->validBind("var4"));
	stdoutput.printf("\n");
	cur->substitution("var2",":var2");
	assertTrue(cur->validBind("var1"));
	assertTrue(cur->validBind("var2"));
	assertFalse(cur->validBind("var3"));
	assertFalse(cur->validBind("var4"));
	stdoutput.printf("\n");
	cur->substitution("var3",":var3");
	assertTrue(cur->validBind("var1"));
	assertTrue(cur->validBind("var2"));
	assertTrue(cur->validBind("var3"));
	assertFalse(cur->validBind("var4"));
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// rebinding
	stdoutput.printf("REBINDING: \n");
	cur->prepareQuery("select :val");
	cur->inputBind("val",1);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	cur->inputBind("val",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"2");
	cur->inputBind("val",3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"3");
	stdoutput.printf("\n");


	// reexecute
	stdoutput.printf("REEXECUTE: \n");
	cur->prepareQuery("select 1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->prepareQuery("select :var");
	cur->inputBind("var",1);
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->inputBind("var",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"2");
	stdoutput.printf("\n");


	// stored procedure returning no value
	// sqlite doesn't support stored procedures


	// stored procedure returning single value
	// sqlite doesn't support stored procedures


	// stored procedure returning multiple values
	// sqlite doesn't support stored procedures


	// stored procedure returning result set
	// sqlite doesn't support stored procedures


	// temporary tables
	stdoutput.printf("TEMPORARY TABLES: \n");
	cur->sendQuery("drop table if exists temptable\n");
	cur->sendQuery("create temporary table temptable (col1 int)");
	assertTrue(cur->sendQuery("insert into temptable values (1)"));
	assertTrue(cur->sendQuery("select count(*) from temptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("select count(*) from temptable"));
	assertTrue(cur->sendQuery("drop table if exists temptable\n"));
	stdoutput.printf("\n");


	// encoded binary data
	stdoutput.printf("ENCODED BINARY DATA: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery("create table testtable (col1 blob)"));
	byte_t	buffer[256];
	for (uint16_t i=0; i<256; i++) {
		buffer[i]=i;
	}
	stringbuffer	querystr;
	querystr.append("insert into testtable values (X'");
	char	hex[3];
	for (uint64_t i=0; i<sizeof(buffer); i++) {
		charstring::printf(hex,sizeof(hex),"%02x",buffer[i]);
		querystr.append(hex);
	}
	querystr.append("')");
	assertTrue(cur->sendQuery(querystr.getString()));
	assertTrue(cur->sendQuery("select col1 from testtable"));
	assertEquals(cur->getFieldLength(0,(uint32_t)0),sizeof(buffer));
	assertEquals(bytestring::compare(cur->getField(0,(uint32_t)0),
						buffer,sizeof(buffer)),0);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// quotes
	stdoutput.printf("QUOTES: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery("create table testtable (col1 varchar(4))"));
	assertTrue(cur->sendQuery("insert into testtable values ('''''')"));
	assertTrue(cur->sendQuery("select col1 from testtable"));
	assertEquals(cur->getFieldLength(0,(uint32_t)0),2);
	assertEquals(charstring::compare(cur->getField(0,(uint32_t)0),"''"),0);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// last insert id
	stdoutput.printf("LAST INSERT ID: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
			"create table testtable "
			"	(col1 integer primary key "
			"	autoincrement, "
			"	col2 int)"));
	assertTrue(cur->sendQuery(
			"insert into testtable values (null,1)"));
	assertEquals(con->getLastInsertId(),(uint64_t)1);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// database is schema
	stdoutput.printf("DATABASE IS SCHEMA: \n");
	assertFalse(con->getDatabaseIsSchema());
	stdoutput.printf("\n");


	// catalog list
	stdoutput.printf("CATALOG LIST: \n");
	assertTrue(cur->getCatalogList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	stdoutput.printf("\n");


	// schema list
	stdoutput.printf("SCHEMA LIST: \n");
	assertTrue(cur->getSchemaList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	stdoutput.printf("\n");


	// table type list
	stdoutput.printf("TABLE TYPE LIST: \n");
	assertTrue(cur->getTableTypeList());
	assertEquals(cur->getColumnName(0),"table_type");
	bool	found=false;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		if (!charstring::compare(
				cur->getField(i,"table_type"),"TABLE")) {
			found=true;
			break;
		}
	}
	assertTrue(found);
	stdoutput.printf("\n");


	// table list
	stdoutput.printf("TABLE LIST: \n");
	cur->sendQuery("drop table if exists testtable1");
	cur->sendQuery("drop table if exists testtable2");
	cur->sendQuery("drop table if exists testtable3");
	cur->sendQuery("drop table if exists testtable4");
	assertTrue(cur->sendQuery(
		"create table testtable1 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->sendQuery(
		"create table testtable2 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->sendQuery(
		"create table testtable3 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->sendQuery(
		"create table testtable4 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->getTableList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"Tables_in_xxx");
		if (!charstring::compare(name,"testtable1") ||
			!charstring::compare(name,"testtable2") ||
			!charstring::compare(name,"testtable3") ||
			!charstring::compare(name,"testtable4")) {
			counter++;
		}
	}
	assertEquals(counter,4);
	assertTrue(cur->sendQuery("drop table if exists testtable1"));
	assertTrue(cur->sendQuery("drop table if exists testtable2"));
	assertTrue(cur->sendQuery("drop table if exists testtable3"));
	assertTrue(cur->sendQuery("drop table if exists testtable4"));
	stdoutput.printf("\n");


	// type info list
	stdoutput.printf("TYPE INFO LIST: \n");
	assertTrue(cur->getTypeInfoList("integer"));
	assertEquals(cur->getColumnName(0),"type_name");
	assertEquals(cur->getColumnName(1),"data_type");
	assertEquals(cur->getColumnName(2),"precision");
	assertEquals(cur->getColumnName(3),"literal_prefix");
	assertEquals(cur->getColumnName(4),"literal_suffix");
	assertEquals(cur->getColumnName(5),"create_params");
	assertEquals(cur->getColumnName(6),"nullable");
	assertEquals(cur->getColumnName(7),"case_sensitive");
	assertEquals(cur->getColumnName(8),"searchable");
	assertEquals(cur->getColumnName(9),"unsigned_attribute");
	assertEquals(cur->getColumnName(10),"fixed_prec_scale");
	assertEquals(cur->getColumnName(11),"auto_increment");
	assertEquals(cur->getColumnName(12),"local_type_name");
	assertEquals(cur->getColumnName(13),"minumum_scale");
	assertEquals(cur->getColumnName(14),"maxiumm_scale");
	assertEquals(cur->getColumnName(15),"sql_data_type");
	assertEquals(cur->getColumnName(16),"sql_datetime_sub");
	assertEquals(cur->getColumnName(17),"num_prec_radix");
	assertEquals(cur->getColumnName(18),"interval_precision");
	assertEquals(cur->getField(0,"type_name"),"INTEGER");
	assertEquals(cur->getField(0,"data_type"),"4");
	assertEquals(cur->getField(0,"precision"),"19");
	assertEquals(cur->getField(0,"local_type_name"),"INTEGER");
	assertTrue(cur->getTypeInfoList("char"));
	assertEquals(cur->getField(0,"type_name"),"CHAR");
	assertEquals(cur->getField(0,"data_type"),"1");
	assertEquals(cur->getField(0,"precision"),"2147483647");
	assertEquals(cur->getField(0,"local_type_name"),"CHAR");
	assertTrue(cur->getTypeInfoList("varchar"));
	assertEquals(cur->getField(0,"type_name"),"VARCHAR");
	assertEquals(cur->getField(0,"data_type"),"12");
	assertEquals(cur->getField(0,"precision"),"2147483647");
	assertEquals(cur->getField(0,"local_type_name"),"VARCHAR");
	assertTrue(cur->getTypeInfoList("date"));
	assertEquals(cur->getField(0,"type_name"),"DATE");
	assertEquals(cur->getField(0,"data_type"),"91");
	assertEquals(cur->getField(0,"precision"),"10");
	assertEquals(cur->getField(0,"local_type_name"),"DATE");
	stdoutput.printf("\n");


	// column list
	stdoutput.printf("COLUMN LIST: \n");
	cur->sendQuery("drop table if exists testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertEquals(cur->getColumnName(0),"column_name");
	assertEquals(cur->getColumnName(1),"data_type");
	assertEquals(cur->getColumnName(2),"character_maximum_length");
	assertEquals(cur->getColumnName(3),"numeric_precision");
	assertEquals(cur->getColumnName(4),"numeric_scale");
	assertEquals(cur->getColumnName(5),"is_nullable");
	assertEquals(cur->getColumnName(6),"column_key");
	assertEquals(cur->getColumnName(7),"column_default");
	assertEquals(cur->getColumnName(8),"extra");
	assertEquals(cur->getField(0,"column_name"),"testint");
	assertEquals(cur->getField(1,"column_name"),"testfloat");
	assertEquals(cur->getField(2,"column_name"),"testchar");
	assertEquals(cur->getField(3,"column_name"),"testvarchar");
	assertEquals(cur->getField(4,"column_name"),"testclob");
	assertEquals(cur->getField(5,"column_name"),"testblob");
	assertEquals(cur->getField(0,"data_type"),"INT");
	assertEquals(cur->getField(1,"data_type"),"FLOAT");
	assertEquals(cur->getField(2,"data_type"),"CHAR");
	assertEquals(cur->getField(3,"data_type"),"VARCHAR");
	assertEquals(cur->getField(4,"data_type"),"CLOB");
	assertEquals(cur->getField(5,"data_type"),"BLOB");
	assertTrue(cur->sendQuery("drop table if exists testtable"));
	stdoutput.printf("\n");


	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	cur->sendQuery("drop table if exists testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 integer primary key autoincrement, "
		"	col2 int)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertTrue(charstring::contains(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::contains(
			cur->getField(0,"column_key"),"PRI"));
	assertFalse(charstring::contains(
			cur->getField(1,"extra"),"auto_increment"));
	assertFalse(charstring::contains(
			cur->getField(1,"column_key"),"PRI"));
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("drop table if exists testtable"));
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertFalse(charstring::contains(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::contains(
			cur->getField(0,"column_key"),"PRI"));
	assertTrue(cur->sendQuery("drop table if exists testtable"));
	stdoutput.printf("\n");


	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	cur->sendQuery("drop table if exists testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getPrimaryKeysList("testtable",NULL));
	assertEquals(cur->getColumnName(0),"table");
	assertEquals(cur->getColumnName(1),"non_unique");
	assertEquals(cur->getColumnName(2),"key_name");
	assertEquals(cur->getColumnName(3),"seq_in_index");
	assertEquals(cur->getColumnName(4),"column_name");
	assertEquals(cur->getColumnName(5),"collation");
	assertEquals(cur->getColumnName(6),"cardinality");
	assertEquals(cur->getColumnName(7),"sub_part");
	assertEquals(cur->getColumnName(8),"packed");
	assertEquals(cur->getColumnName(9),"null");
	assertEquals(cur->getColumnName(10),"index_type");
	assertEquals(cur->getColumnName(11),"comment");
	assertEquals(cur->getColumnName(12),"index_comment");
	assertEquals(cur->rowCount(),1);
	assertTrue(!charstring::compare(cur->getField(0,"table"),"testtable"));
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"col1"));
	assertTrue(cur->sendQuery("drop table if exists testtable"));
	stdoutput.printf("\n");


	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	cur->sendQuery("drop table if exists testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getKeyAndIndexList("testtable",NULL));
	assertEquals(cur->getColumnName(0),"table");
	assertEquals(cur->getColumnName(1),"non_unique");
	assertEquals(cur->getColumnName(2),"key_name");
	assertEquals(cur->getColumnName(3),"seq_in_index");
	assertEquals(cur->getColumnName(4),"column_name");
	assertEquals(cur->getColumnName(5),"collation");
	assertEquals(cur->getColumnName(6),"cardinality");
	assertEquals(cur->getColumnName(7),"sub_part");
	assertEquals(cur->getColumnName(8),"packed");
	assertEquals(cur->getColumnName(9),"null");
	assertEquals(cur->getColumnName(10),"index_type");
	assertEquals(cur->getColumnName(11),"comment");
	assertEquals(cur->getColumnName(12),"index_comment");
	assertEquals(cur->rowCount(),1);
	assertTrue(!charstring::compare(cur->getField(0,"table"),"testtable"));
	assertEquals(cur->getField(0,"non_unique"),"0");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"col1"));
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"3");
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	assertTrue(cur->sendQuery("drop table if exists testtable"));
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	assertTrue(cur->getProcedureList(NULL));
	assertEquals(cur->rowCount(),0);
	stdoutput.printf("\n");


	// procedure parameter list
	stdoutput.printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(cur->getProcedureParameterList("testproc1",NULL));
	assertEquals(cur->getColumnName(0),"parameter_name");
	assertEquals(cur->getColumnName(1),"parameter_mode");
	assertEquals(cur->getColumnName(2),"data_type");
	assertEquals(cur->getColumnName(3),"character_maximum_length");
	assertEquals(cur->getColumnName(4),"ordinal_position");
	assertEquals(cur->rowCount(),0);
	stdoutput.printf("\n");


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery("select * from testtable"));
	assertFalse(cur->sendQuery("select * from testtable"));
	assertFalse(cur->sendQuery("select * from testtable"));
	assertFalse(cur->sendQuery("select * from testtable"));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
