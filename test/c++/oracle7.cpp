// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	const char	*isolationlevels[]={
				"READ COMMITTED","SERIALIZABLE",NULL};
	const char	*bindvars[]={"1","2","3","4","5",NULL};
	const char	*bindvals[]={"4","testchar4",
				"testvarchar4","01-JAN-04","testlong4"};
	const char	*arraybindvars[]={"var1","var2","var3",
					"var4","var5",NULL};
	const char	*arraybindvals[]={"7","testchar7",
				"testvarchar7","01-JAN-07","testlong7"};
	int64_t		numvar;
	const char	*stringvar;
	double		floatvar;
	int16_t		year=0;
	int16_t		month=0;
	int16_t		day=0;
	int16_t		hour=0;
	int16_t		minute=0;
	int16_t		second=0;
	int32_t		microsecond=0;
	const char	*tz=NULL;
	bool		isnegative=false;
	const char * const *cols;
	const char * const *fields;
	uint32_t	*fieldlens;
	const char	*subvars[]={"var1","var2","var3",NULL};
	int64_t		subvallongs[]={1,2,3};
	const char	*subvalstrings[]={"hi","hello","bye"};
	double		subvaldoubles[]={10.55,10.556,10.5556};
	uint32_t	precs[]={4,5,6};
	uint32_t	scales[]={2,3,4};
	uint16_t	port;
	const char	*socket;
	uint16_t	id;
	const char	*filename;

	#define	LARGE_BUFFER_LENGTH	2000
	char		largebuffer[LARGE_BUFFER_LENGTH+1];


	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"oracle");
	stdoutput.printf("\n");


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// bind format
	stdoutput.printf("BIND FORMAT: \n");
	assertEquals(con->bindFormat(),":*");
	stdoutput.printf("\n");


	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"%s.nextval");
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	for (const char **il=isolationlevels; *il; il++) {
		// oracle requires the isolation level to
		// be the first query of the transaction
		assertTrue(con->commit());
		// you can set the isolation level, but to get it, you have to
		// have permisisons to read from sys.v_$session and
		// sys.v_$transaction
		assertTrue(con->setIsolationLevel(*il));
		stdoutput.printf("\n");
	}
	// reset to the default isolation level
	assertTrue(con->commit());
	assertTrue(con->setIsolationLevel(isolationlevels[0]));
	stdoutput.printf("\n");


	// create testtable
	stdoutput.printf("CREATE TESTTABLE: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long)"));
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01-JAN-01', "
		"	'testlong1')"));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),1);
	stdoutput.printf("\n");


	// input bind by position
	stdoutput.printf("INPUT BIND BY POSITION: \n");
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4, "
		"	:var5)");
	assertEquals(cur->countBindVariables(),5);
	cur->inputBind("1",2);
	cur->inputBind("2","testchar2");
	cur->inputBind("3","testvarchar2");
	cur->inputBind("4",2,1,1,0,0,0,0,NULL,false);
	cur->inputBind("5","testlong2");
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",3);
	cur->inputBind("2","testchar3");
	cur->inputBind("3","testvarchar3");
	cur->inputBind("4",3,1,1,0,0,0,0,NULL,false);
	cur->inputBind("5","testlong3");
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of input binds by position
	stdoutput.printf("ARRAY OF INPUT BINDS BY POSITION: \n");
	cur->clearBinds();
	cur->inputBinds(bindvars,bindvals);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by position with validation
	stdoutput.printf("INPUT BIND BY POSITION WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("1",5);
	cur->inputBind("2","testchar5");
	cur->inputBind("3","testvarchar5");
	cur->inputBind("4",5,1,1,0,0,0,0,NULL,false);
	cur->inputBind("5","testlong5");
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	cur->clearBinds();


	// input bind by name
	stdoutput.printf("INPUT BIND BY NAME: \n");
	cur->clearBinds();
	cur->inputBind("var1",6);
	cur->inputBind("var2","testchar6");
	cur->inputBind("var3","testvarchar6");
	cur->inputBind("var4",6,1,1,0,0,0,0,NULL,false);
	cur->inputBind("var5","testlong6");
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of input binds by name
	stdoutput.printf("ARRAY OF INPUT BINDS BY NAME: \n");
	cur->clearBinds();
	cur->inputBinds(arraybindvars,arraybindvals);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name with validation
	stdoutput.printf("INPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("var1",8);
	cur->inputBind("var2","testchar8");
	cur->inputBind("var3","testvarchar8");
	cur->inputBind("var4",8,1,1,0,0,0,0,NULL,false);
	cur->inputBind("var5","testlong8");
	cur->inputBind("var9","junkvalue");
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),5);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"TESTNUMBER");
	assertEquals(cur->getColumnName(1),"TESTCHAR");
	assertEquals(cur->getColumnName(2),"TESTVARCHAR");
	assertEquals(cur->getColumnName(3),"TESTDATE");
	assertEquals(cur->getColumnName(4),"TESTLONG");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTNUMBER");
	assertEquals(cols[1],"TESTCHAR");
	assertEquals(cols[2],"TESTVARCHAR");
	assertEquals(cols[3],"TESTDATE");
	assertEquals(cols[4],"TESTLONG");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"NUMBER");
	assertEquals(cur->getColumnType("testnumber"),"NUMBER");
	assertEquals(cur->getColumnType(1),"CHAR");
	assertEquals(cur->getColumnType("testchar"),"CHAR");
	assertEquals(cur->getColumnType(2),"VARCHAR2");
	assertEquals(cur->getColumnType("testvarchar"),"VARCHAR2");
	assertEquals(cur->getColumnType(3),"DATE");
	assertEquals(cur->getColumnType("testdate"),"DATE");
	assertEquals(cur->getColumnType(4),"LONG");
	assertEquals(cur->getColumnType("testlong"),"LONG");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),22);
	assertEquals(cur->getColumnLength("testnumber"),22);
	assertEquals(cur->getColumnLength(1),40);
	assertEquals(cur->getColumnLength("testchar"),40);
	assertEquals(cur->getColumnLength(2),40);
	assertEquals(cur->getColumnLength("testvarchar"),40);
	assertEquals(cur->getColumnLength(3),7);
	assertEquals(cur->getColumnLength("testdate"),7);
	assertEquals(cur->getColumnLength(4),0);
	assertEquals(cur->getColumnLength("testlong"),0);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testnumber"),1);
	assertEquals(cur->getLongest(1),40);
	assertEquals(cur->getLongest("testchar"),40);
	assertEquals(cur->getLongest(2),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(3),9);
	assertEquals(cur->getLongest("testdate"),9);
	assertEquals(cur->getLongest(4),9);
	assertEquals(cur->getLongest("testlong"),9);
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");


	// total rows
	stdoutput.printf("TOTAL ROWS: \n");
	assertEquals(cur->totalRows(),0);
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
	assertEquals(cur->getField(0,1),"testchar1                               ");
	assertEquals(cur->getField(0,2),"testvarchar1");
	assertEquals(cur->getField(0,3),"01-JAN-01");
	assertEquals(cur->getField(0,4),"testlong1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"testchar8                               ");
	assertEquals(cur->getField(7,2),"testvarchar8");
	assertEquals(cur->getField(7,3),"01-JAN-08");
	assertEquals(cur->getField(7,4),"testlong8");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),40);
	assertEquals(cur->getFieldLength(0,2),12);
	assertEquals(cur->getFieldLength(0,3),9);
	assertEquals(cur->getFieldLength(0,4),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),40);
	assertEquals(cur->getFieldLength(7,2),12);
	assertEquals(cur->getFieldLength(7,3),9);
	assertEquals(cur->getFieldLength(7,4),9);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"TESTNUMBER"),"1");
	assertEquals(cur->getField(0,"TESTCHAR"),"testchar1                               ");
	assertEquals(cur->getField(0,"TESTVARCHAR"),"testvarchar1");
	assertEquals(cur->getField(0,"TESTDATE"),"01-JAN-01");
	assertEquals(cur->getField(0,"TESTLONG"),"testlong1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"TESTNUMBER"),"8");
	assertEquals(cur->getField(7,"TESTCHAR"),"testchar8                               ");
	assertEquals(cur->getField(7,"TESTVARCHAR"),"testvarchar8");
	assertEquals(cur->getField(7,"TESTDATE"),"01-JAN-08");
	assertEquals(cur->getField(7,"TESTLONG"),"testlong8");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"TESTNUMBER"),1);
	assertEquals(cur->getFieldLength(0,"TESTCHAR"),40);
	assertEquals(cur->getFieldLength(0,"TESTVARCHAR"),12);
	assertEquals(cur->getFieldLength(0,"TESTDATE"),9);
	assertEquals(cur->getFieldLength(0,"TESTLONG"),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"TESTNUMBER"),1);
	assertEquals(cur->getFieldLength(7,"TESTCHAR"),40);
	assertEquals(cur->getFieldLength(7,"TESTVARCHAR"),12);
	assertEquals(cur->getFieldLength(7,"TESTDATE"),9);
	assertEquals(cur->getFieldLength(7,"TESTLONG"),9);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"testchar1                               ");
	assertEquals(fields[2],"testvarchar1");
	assertEquals(fields[3],"01-JAN-01");
	assertEquals(fields[4],"testlong1");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],40);
	assertEquals(fieldlens[2],12);
	assertEquals(fieldlens[3],9);
	assertEquals(fieldlens[4],9);
	stdoutput.printf("\n");


	// result set buffer size
	stdoutput.printf("RESULT SET BUFFER SIZE: \n");
	assertEquals(cur->getResultSetBufferSize(),0);
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertEquals(cur->getResultSetBufferSize(),2);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),0);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(2,(uint32_t)0),"3");
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertEquals(cur->getColumnName(0),NULL);
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),NULL);
	cur->getColumnInfo();
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertEquals(cur->getColumnName(0),"TESTNUMBER");
	assertEquals(cur->getColumnLength((uint32_t)0),22);
	assertEquals(cur->getColumnType((uint32_t)0),"NUMBER");
	stdoutput.printf("\n");


	// suspended session
	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	delete[] filename;
	stdoutput.printf("\n");


	// column count for cached result set
	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),5);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"TESTNUMBER");
	assertEquals(cur->getColumnName(1),"TESTCHAR");
	assertEquals(cur->getColumnName(2),"TESTVARCHAR");
	assertEquals(cur->getColumnName(3),"TESTDATE");
	assertEquals(cur->getColumnName(4),"TESTLONG");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTNUMBER");
	assertEquals(cols[1],"TESTCHAR");
	assertEquals(cols[2],"TESTVARCHAR");
	assertEquals(cols[3],"TESTDATE");
	assertEquals(cols[4],"TESTLONG");
	stdoutput.printf("\n");


	// cached result set with result set buffer size
	stdoutput.printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	for (uint32_t i=0; cur->getRow(i); i++) {
		secondcur=new sqlrcursor(con);
		secondcur->setResultSetBufferSize(1);
		assertTrue(secondcur->sendQuery("select * from testtable"));
		delete secondcur;
		secondcur=NULL;
	}
	cur->setResultSetBufferSize(0);
	stdoutput.printf("\n");


	// commit and rollback
	stdoutput.printf("COMMIT AND ROLLBACK: \n");
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	assertTrue(con->commit());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01-JAN-2010', "
		"	'testlong10')"));
	assertTrue(con->rollback());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	assertTrue(con->autoCommitOn());
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01-JAN-2010', "
		"	'testlong10')"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(con->autoCommitOff());
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),'$(var2)',$(var3) from dual");
	cur->substitution("var1",1);
	cur->substitution("var2","hello");
	cur->substitution("var3",10.5556,6,4);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),$(var2),$(var3) from dual");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3");
	stdoutput.printf("\n");
	cur->prepareQuery("select '$(var1)','$(var2)','$(var3)' from dual");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	stdoutput.printf("\n");
	cur->prepareQuery("select $(var1),$(var2),$(var3) from dual");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// nulls as nulls
	stdoutput.printf("NULLS AS NULLS: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery("select NULL,1,NULL from dual"));
	assertEquals(cur->getField(0,(uint32_t)0),NULL);
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select NULL,1,NULL from dual"));
	assertEquals(cur->getField(0,(uint32_t)0),"");
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),"");
	stdoutput.printf("\n");


	// null and empty lobs
	// Oracle 7 doesn't support lobs


	// long lobs
	// Oracle 7 doesn't support lobs


	// output bind by position
	stdoutput.printf("OUTPUT BIND BY POSITION: \n");
	cur->getNullsAsNulls();
	cur->prepareQuery(
		"begin "
		"	:numvar:=1; "
		"	:stringvar:='hello'; "
		"	:floatvar:=2.5; "
		"	:datevar:='03-FEB-01'; "
		"end;");
	assertEquals(cur->countBindVariables(),4);
	cur->defineOutputBindInteger("1");
	cur->defineOutputBindString("2",10);
	cur->defineOutputBindDouble("3");
	cur->defineOutputBindDate("4");
	assertTrue(cur->executeQuery());
	numvar=cur->getOutputBindInteger("1");
	stringvar=cur->getOutputBindString("2");
	floatvar=cur->getOutputBindDouble("3");
	cur->getOutputBindDate("4",&year,&month,&day,
					&hour,&minute,&second,&microsecond,&tz,
					&isnegative);
	assertEquals(numvar,1);
	assertEquals(stringvar,"hello");
	assertEquals(floatvar,2.5);
	assertEquals(year,2001);
	assertEquals(month,2);
	assertEquals(day,3);
	assertEquals(hour,0);
	assertEquals(minute,0);
	assertEquals(second,0);
	assertEquals(microsecond,0);
	assertEquals(tz,"");
	cur->getNullsAsEmptyStrings();
	stdoutput.printf("\n");


	// output bind by name
	stdoutput.printf("OUTPUT BIND BY NAME: \n");
	cur->clearBinds();
	cur->defineOutputBindInteger("numvar");
	cur->defineOutputBindString("stringvar",10);
	cur->defineOutputBindDouble("floatvar");
	cur->defineOutputBindDate("datevar");
	assertTrue(cur->executeQuery());
	numvar=cur->getOutputBindInteger("numvar");
	stringvar=cur->getOutputBindString("stringvar");
	floatvar=cur->getOutputBindDouble("floatvar");
	cur->getOutputBindDate("datevar",&year,&month,&day,
					&hour,&minute,&second,&microsecond,&tz,
					&isnegative);
	assertEquals(numvar,1);
	assertEquals(stringvar,"hello");
	assertEquals(floatvar,2.5);
	assertEquals(year,2001);
	assertEquals(month,2);
	assertEquals(day,3);
	assertEquals(hour,0);
	assertEquals(minute,0);
	assertEquals(second,0);
	assertEquals(microsecond,0);
	assertEquals(tz,"");
	stdoutput.printf("\n");


	// output bind by name with validation
	stdoutput.printf("OUTPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->defineOutputBindInteger("numvar");
	cur->defineOutputBindString("stringvar",10);
	cur->defineOutputBindDouble("floatvar");
	cur->defineOutputBindDate("datevar");
	cur->defineOutputBindString("dummyvar",10);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	numvar=cur->getOutputBindInteger("numvar");
	stringvar=cur->getOutputBindString("stringvar");
	floatvar=cur->getOutputBindDouble("floatvar");
	cur->getOutputBindDate("datevar",&year,&month,&day,
					&hour,&minute,&second,&microsecond,&tz,
					&isnegative);
	assertEquals(numvar,1);
	assertEquals(stringvar,"hello");
	assertEquals(floatvar,2.5);
	assertEquals(year,2001);
	assertEquals(month,2);
	assertEquals(day,3);
	assertEquals(hour,0);
	assertEquals(minute,0);
	assertEquals(second,0);
	assertEquals(microsecond,0);
	assertEquals(tz,"");
	stdoutput.printf("\n");


	// lob output bind
	// Oracle 7 doesn't support lobs


	// long output bind
	stdoutput.printf("LONG OUTPUT BIND: \n");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	char	query[LARGE_BUFFER_LENGTH+25];
	charstring::printf(query,sizeof(query),
				"begin :bindval:='%s'; end;",largebuffer);
	cur->prepareQuery(query);
	cur->defineOutputBindString("bindval",LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("bindval"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getOutputBindString("bindval"),largebuffer);
	stdoutput.printf("\n");


	// negative input bind
	stdoutput.printf("NEGATIVE INPUT BIND: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery("create table testtable (testval number)");
	cur->prepareQuery("insert into testtable values (:testval)");
	cur->inputBind("testval",-1);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testval from testtable");
	assertEquals(cur->getField(0,"TESTVAL"),"-1");
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// bind validation
// #7996
#if 0
	stdoutput.printf("BIND VALIDATION: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery(
		"create table testtable ("
		"	col1 varchar2(20), "
		"	col2 varchar2(20), "
		"	col3 varchar2(20))");
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))");
	cur->inputBind("var1",1);
	cur->inputBind("var2",2);
	cur->inputBind("var3",3);
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
#endif


	// rebinding
	stdoutput.printf("REBINDING: \n");
	cur->prepareQuery(
		"begin "
		"	:out:= :in; "
		"end;");
	cur->inputBind("in",1);
	cur->defineOutputBindInteger("out");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out"),1);
	cur->inputBind("in",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out"),2);
	cur->inputBind("in",3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out"),3);
	stdoutput.printf("\n");


	// reexecute
	stdoutput.printf("REEXECUTE: \n");
	cur->prepareQuery("select 1 from dual");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->prepareQuery("select :var from dual");
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
	stdoutput.printf("STORED PROCEDURE RETURNING NO VALUE: \n");
	cur->sendQuery("drop function testproc");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create or replace "
		"procedure testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2) "
		"is "
		"begin "
		"	return; "
		"end;"));
	cur->prepareQuery("begin testproc(:in1,:in2,:in3); end;");
	cur->inputBind("in1",1);
	cur->inputBind("in2",1.1,2,1);
	cur->inputBind("in3","hello");
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");


	// stored procedure returning single value
	stdoutput.printf("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	cur->sendQuery("drop function testproc");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create or replace "
		"function testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2) "
		"return number "
		"is "
		"begin "
		"	return in1; "
		"end;"));
	cur->prepareQuery("select testproc(:in1,:in2,:in3) from dual");
	cur->inputBind("in1",1);
	cur->inputBind("in2",1.1,2,1);
	cur->inputBind("in3","hello");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	cur->prepareQuery("begin  :out1:=testproc(:in1,:in2,:in3); end;");
	cur->inputBind("in1",1);
	cur->inputBind("in2",1.1,2,1);
	cur->inputBind("in3","hello");
	cur->defineOutputBindInteger("out1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out1"),1);
	assertTrue(cur->sendQuery("drop function testproc"));
	stdoutput.printf("\n");


	// stored procedure returning multiple values
	stdoutput.printf("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	cur->sendQuery("drop function testproc");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create or replace "
		"procedure testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2, "
		"	out1 out number, "
		"	out2 out number, "
		"	out3 out varchar2) "
		"is "
		"begin "
		"	out1:=in1; "
		"	out2:=in2; "
		"	out3:=in3; "
		"end;"));
	cur->prepareQuery(
		"begin testproc(:in1,:in2,:in3,:out1,:out2,:out3); "
		"	end;");
	cur->inputBind("in1",1);
	cur->inputBind("in2",1.1,2,1);
	cur->inputBind("in3","hello");
	cur->defineOutputBindInteger("out1");
	cur->defineOutputBindDouble("out2");
	cur->defineOutputBindString("out3",20);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out1"),1);
	assertEquals(cur->getOutputBindDouble("out2"),1.1);
	assertEquals(cur->getOutputBindString("out3"),"hello");
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");


	// stored procedure returning result set
	// not super important to test with Oracle 7, maybe later


	// temporary tables
	// not super important to test with Oracle 7, maybe later


	// binary data
	// FIXME: ...


	// database is schema
	// not super important to test with Oracle 7, maybe later


	// catalog list
	// not super important to test with Oracle 7, maybe later


	// schema list
	// not super important to test with Oracle 7, maybe later


	// table type list
	// not super important to test with Oracle 7, maybe later


	// table list
	// not super important to test with Oracle 7, maybe later


	// type info list
	// not super important to test with Oracle 7, maybe later


	// column list
	// not super important to test with Oracle 7, maybe later


	// column list - auto_increment, primary key
	// not super important to test with Oracle 7, maybe later


	// primary keys list
	// not super important to test with Oracle 7, maybe later


	// key and index list
	// not super important to test with Oracle 7, maybe later


	// procedure list
	// not super important to test with Oracle 7, maybe later


	// procedure parameter list
	// not super important to test with Oracle 7, maybe later


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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


	delete cur;
	delete con;

	reportTestStatus();

	return status;
}
