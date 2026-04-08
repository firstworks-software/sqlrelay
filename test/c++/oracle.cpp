// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/sys.h>
#include <rudiments/process.h>
#include <rudiments/snooze.h>
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
				"testvarchar4","01-JAN-2004","testlong4"};
	const char	*arraybindvars[]={"var1","var2","var3",
					"var4","var5",NULL};
	const char	*arraybindvals[]={"7","testchar7",
				"testvarchar7","01-JAN-2007","testlong7"};
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
	const char	*nullvar;
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
	const char	*clobvar;
	uint32_t	clobvarlength;
	const char	*blobvar;
	uint32_t	blobvarlength;
	uint16_t	counter;
	#define	LARGE_BUFFER_LENGTH	8192
	char		largebuffer[LARGE_BUFFER_LENGTH+1];



	// hostname
	char	*hostname=sys::getHostName();
	char	*dot=(char *)charstring::findFirstOrEnd(hostname,'.');
	*dot='\0';


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
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
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
		"	'01-JAN-2001', "
		"	'testlong1', "
		"	'testclob1', "
		"	empty_blob())"));
	assertEquals(cur->countBindVariables(),0);
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
		"	:var5, "
		"	:var6, "
		"	:var7)");
	assertEquals(cur->countBindVariables(),7);
	cur->inputBind("1",2);
	cur->inputBind("2","testchar2");
	cur->inputBind("3","testvarchar2");
	cur->inputBind("4",2002,1,1,0,0,0,0,NULL,false);
	cur->inputBind("5","testlong2");
	cur->inputBindClob("6","testclob2",9);
	cur->inputBindBlob("7","testblob2",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",3);
	cur->inputBind("2","testchar3");
	cur->inputBind("3","testvarchar3");
	cur->inputBind("4",2003,1,1,0,0,0,0,NULL,false);
	cur->inputBind("5","testlong3");
	cur->inputBindClob("6","testclob3",9);
	cur->inputBindBlob("7","testblob3",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of input binds by position
	stdoutput.printf("ARRAY OF INPUT BINDS BY POSITION: \n");
	cur->clearBinds();
	cur->inputBinds(bindvars,bindvals);
	cur->inputBindClob("6","testclob4",9);
	cur->inputBindBlob("7","testblob4",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name
	stdoutput.printf("INPUT BIND BY NAME: \n");
	cur->clearBinds();
	assertEquals(cur->countBindVariables(),7);
	cur->inputBind("var1",5);
	cur->inputBind("var2","testchar5");
	cur->inputBind("var3","testvarchar5");
	cur->inputBind("var4",2005,1,1,0,0,0,0,NULL,false);
	cur->inputBind("var5","testlong5");
	cur->inputBindClob("var6","testclob5",9);
	cur->inputBindBlob("var7","testblob5",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("var1",6);
	cur->inputBind("var2","testchar6");
	cur->inputBind("var3","testvarchar6");
	cur->inputBind("var4",2006,1,1,0,0,0,0,NULL,false);
	cur->inputBind("var5","testlong6");
	cur->inputBindClob("var6","testclob6",9);
	cur->inputBindBlob("var7","testblob6",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of input binds by name
	stdoutput.printf("ARRAY OF INPUT BINDS BY NAME: \n");
	cur->clearBinds();
	cur->inputBinds(arraybindvars,arraybindvals);
	cur->inputBindClob("var6","testclob7",9);
	cur->inputBindBlob("var7","testblob7",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name with validation
	stdoutput.printf("INPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("var1",8);
	cur->inputBind("var2","testchar8");
	cur->inputBind("var3","testvarchar8");
	cur->inputBind("var4",2008,1,1,0,0,0,0,NULL,false);
	cur->inputBind("var5","testlong8");
	cur->inputBindClob("var6","testclob8",9);
	cur->inputBindBlob("var7","testblob8",9);
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
	assertEquals(cur->colCount(),7);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"TESTNUMBER");
	assertEquals(cur->getColumnName(1),"TESTCHAR");
	assertEquals(cur->getColumnName(2),"TESTVARCHAR");
	assertEquals(cur->getColumnName(3),"TESTDATE");
	assertEquals(cur->getColumnName(4),"TESTLONG");
	assertEquals(cur->getColumnName(5),"TESTCLOB");
	assertEquals(cur->getColumnName(6),"TESTBLOB");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTNUMBER");
	assertEquals(cols[1],"TESTCHAR");
	assertEquals(cols[2],"TESTVARCHAR");
	assertEquals(cols[3],"TESTDATE");
	assertEquals(cols[4],"TESTLONG");
	assertEquals(cols[5],"TESTCLOB");
	assertEquals(cols[6],"TESTBLOB");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"NUMBER");
	assertEquals(cur->getColumnType("TESTNUMBER"),"NUMBER");
	assertEquals(cur->getColumnType(1),"CHAR");
	assertEquals(cur->getColumnType("TESTCHAR"),"CHAR");
	assertEquals(cur->getColumnType(2),"VARCHAR2");
	assertEquals(cur->getColumnType("TESTVARCHAR"),"VARCHAR2");
	assertEquals(cur->getColumnType(3),"DATE");
	assertEquals(cur->getColumnType("TESTDATE"),"DATE");
	assertEquals(cur->getColumnType(4),"LONG");
	assertEquals(cur->getColumnType("TESTLONG"),"LONG");
	assertEquals(cur->getColumnType(5),"CLOB");
	assertEquals(cur->getColumnType("TESTCLOB"),"CLOB");
	assertEquals(cur->getColumnType(6),"BLOB");
	assertEquals(cur->getColumnType("TESTBLOB"),"BLOB");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),22);
	assertEquals(cur->getColumnLength("TESTNUMBER"),22);
	assertEquals(cur->getColumnLength(1),40);
	assertEquals(cur->getColumnLength("TESTCHAR"),40);
	assertEquals(cur->getColumnLength(2),40);
	assertEquals(cur->getColumnLength("TESTVARCHAR"),40);
	assertEquals(cur->getColumnLength(3),7);
	assertEquals(cur->getColumnLength("TESTDATE"),7);
	assertEquals(cur->getColumnLength(4),0);
	assertEquals(cur->getColumnLength("TESTLONG"),0);
	assertEquals(cur->getColumnLength(5),0);
	assertEquals(cur->getColumnLength("TESTCLOB"),0);
	assertEquals(cur->getColumnLength(6),0);
	assertEquals(cur->getColumnLength("TESTBLOB"),0);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("TESTNUMBER"),1);
	assertEquals(cur->getLongest(1),40);
	assertEquals(cur->getLongest("TESTCHAR"),40);
	assertEquals(cur->getLongest(2),12);
	assertEquals(cur->getLongest("TESTVARCHAR"),12);
	assertEquals(cur->getLongest(3),9);
	assertEquals(cur->getLongest("TESTDATE"),9);
	assertEquals(cur->getLongest(4),9);
	assertEquals(cur->getLongest("TESTLONG"),9);
	assertEquals(cur->getLongest(5),9);
	assertEquals(cur->getLongest("TESTCLOB"),9);
	assertEquals(cur->getLongest(6),9);
	assertEquals(cur->getLongest("TESTBLOB"),9);
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
	assertEquals(cur->getField(0,5),"testclob1");
	assertEquals(cur->getField(0,6),"");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"testchar8                               ");
	assertEquals(cur->getField(7,2),"testvarchar8");
	assertEquals(cur->getField(7,3),"01-JAN-08");
	assertEquals(cur->getField(7,4),"testlong8");
	assertEquals(cur->getField(7,5),"testclob8");
	assertEquals(cur->getField(7,6),"testblob8");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),40);
	assertEquals(cur->getFieldLength(0,2),12);
	assertEquals(cur->getFieldLength(0,3),9);
	assertEquals(cur->getFieldLength(0,4),9);
	assertEquals(cur->getFieldLength(0,5),9);
	assertEquals(cur->getFieldLength(0,6),0);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),40);
	assertEquals(cur->getFieldLength(7,2),12);
	assertEquals(cur->getFieldLength(7,3),9);
	assertEquals(cur->getFieldLength(7,4),9);
	assertEquals(cur->getFieldLength(7,5),9);
	assertEquals(cur->getFieldLength(7,6),9);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"TESTNUMBER"),"1");
	assertEquals(cur->getField(0,"TESTCHAR"),"testchar1                               ");
	assertEquals(cur->getField(0,"TESTVARCHAR"),"testvarchar1");
	assertEquals(cur->getField(0,"TESTDATE"),"01-JAN-01");
	assertEquals(cur->getField(0,"TESTLONG"),"testlong1");
	assertEquals(cur->getField(0,"TESTCLOB"),"testclob1");
	assertEquals(cur->getField(0,"TESTBLOB"),"");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"TESTNUMBER"),"8");
	assertEquals(cur->getField(7,"TESTCHAR"),"testchar8                               ");
	assertEquals(cur->getField(7,"TESTVARCHAR"),"testvarchar8");
	assertEquals(cur->getField(7,"TESTDATE"),"01-JAN-08");
	assertEquals(cur->getField(7,"TESTLONG"),"testlong8");
	assertEquals(cur->getField(7,"TESTCLOB"),"testclob8");
	assertEquals(cur->getField(7,"TESTBLOB"),"testblob8");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"TESTNUMBER"),1);
	assertEquals(cur->getFieldLength(0,"TESTCHAR"),40);
	assertEquals(cur->getFieldLength(0,"TESTVARCHAR"),12);
	assertEquals(cur->getFieldLength(0,"TESTDATE"),9);
	assertEquals(cur->getFieldLength(0,"TESTLONG"),9);
	assertEquals(cur->getFieldLength(0,"TESTCLOB"),9);
	assertEquals(cur->getFieldLength(0,"TESTBLOB"),0);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"TESTNUMBER"),1);
	assertEquals(cur->getFieldLength(7,"TESTCHAR"),40);
	assertEquals(cur->getFieldLength(7,"TESTVARCHAR"),12);
	assertEquals(cur->getFieldLength(7,"TESTDATE"),9);
	assertEquals(cur->getFieldLength(7,"TESTLONG"),9);
	assertEquals(cur->getFieldLength(7,"TESTCLOB"),9);
	assertEquals(cur->getFieldLength(7,"TESTBLOB"),9);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"testchar1                               ");
	assertEquals(fields[2],"testvarchar1");
	assertEquals(fields[3],"01-JAN-01");
	assertEquals(fields[4],"testlong1");
	assertEquals(fields[5],"testclob1");
	assertEquals(fields[6],"");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],40);
	assertEquals(fieldlens[2],12);
	assertEquals(fieldlens[3],9);
	assertEquals(fieldlens[4],9);
	assertEquals(fieldlens[5],9);
	assertEquals(fieldlens[6],0);
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
	assertEquals(cur->colCount(),7);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"TESTNUMBER");
	assertEquals(cur->getColumnName(1),"TESTCHAR");
	assertEquals(cur->getColumnName(2),"TESTVARCHAR");
	assertEquals(cur->getColumnName(3),"TESTDATE");
	assertEquals(cur->getColumnName(4),"TESTLONG");
	assertEquals(cur->getColumnName(5),"TESTCLOB");
	assertEquals(cur->getColumnName(6),"TESTBLOB");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTNUMBER");
	assertEquals(cols[1],"TESTCHAR");
	assertEquals(cols[2],"TESTVARCHAR");
	assertEquals(cols[3],"TESTDATE");
	assertEquals(cols[4],"TESTLONG");
	assertEquals(cols[5],"TESTCLOB");
	assertEquals(cols[6],"TESTBLOB");
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
		"	'testlong10', "
		"	'testclob10', "
		"	NULL)"));
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
		"	'testlong10', "
		"	'testclob10', "
		"	NULL)"));
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
	cur->substitution("var1","$(var11)");
	cur->substitution("var2","$(var21)");
	cur->substitution("var3","$(var31)");
	cur->substitution("var11","$(var111)");
	cur->substitution("var21","$(var211)");
	cur->substitution("var31","$(var311)");
	cur->substitution("var111",1);
	cur->substitution("var211","hello");
	cur->substitution("var311",10.5556,6,4);
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
	// oracle treats empty strings as NULL, so even though we bound
	// "" to var1 and var3, we still need to test for NULL
	assertEquals(cur->getField(0,(uint32_t)0),NULL);
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	assertEquals(cur->getField(0,3),NULL);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// long lobs
	stdoutput.printf("LONG LOBS: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery("create table testtable (testclob clob)");
	cur->prepareQuery("insert into testtable values (:clobval)");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->inputBindClob("clobval",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testclob from testtable");
	assertEquals(cur->getFieldLength(0,"TESTCLOB"),LARGE_BUFFER_LENGTH);
	assertEquals(largebuffer,cur->getField(0,"TESTCLOB"));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// output bind by position
	stdoutput.printf("OUTPUT BIND BY POSITION: \n");
	cur->prepareQuery(
		"begin "
		"	:numvar:=1; "
		"	:stringvar:='hello'; "
		"	:floatvar:=2.5; "
		"	:datevar:='03-FEB-2001'; "
		"	:nullvar:=null; "
		"end;");
	assertEquals(cur->countBindVariables(),5);
	cur->defineOutputBindInteger("1");
	cur->defineOutputBindString("2",10);
	cur->defineOutputBindDouble("3");
	cur->defineOutputBindDate("4");
	cur->defineOutputBindString("5",10);
	assertTrue(cur->executeQuery());
	numvar=cur->getOutputBindInteger("1");
	stringvar=cur->getOutputBindString("2");
	floatvar=cur->getOutputBindDouble("3");
	cur->getOutputBindDate("4",&year,&month,&day,
				&hour,&minute,&second,&microsecond,&tz,
				&isnegative);
	nullvar=cur->getOutputBindString("5");
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
	assertEquals(nullvar,NULL);
	stdoutput.printf("\n");


	// output bind by name
	stdoutput.printf("OUTPUT BIND BY NAME: \n");
	cur->clearBinds();
	assertEquals(cur->countBindVariables(),5);
	cur->defineOutputBindInteger("numvar");
	cur->defineOutputBindString("stringvar",10);
	cur->defineOutputBindDouble("floatvar");
	cur->defineOutputBindDate("datevar");
	cur->defineOutputBindString("nullvar",10);
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
	nullvar=cur->getOutputBindString("nullvar");
	assertEquals(nullvar,NULL);
	stdoutput.printf("\n");


	// output bind by name with validation
	stdoutput.printf("OUTPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->defineOutputBindInteger("numvar");
	cur->defineOutputBindString("stringvar",10);
	cur->defineOutputBindDouble("floatvar");
	cur->defineOutputBindDate("datevar");
	cur->defineOutputBindString("nullvar",10);
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
	nullvar=cur->getOutputBindString("nullvar");
	assertEquals(nullvar,NULL);
	stdoutput.printf("\n");


	// lob output bind
	stdoutput.printf("LOB OUTPUT BIND: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	cur->prepareQuery("insert into testtable values ('hello',:var1)");
	cur->inputBindBlob("var1","hello",5);
	assertTrue(cur->executeQuery());
	cur->prepareQuery(
		"begin "
		"	select testclob into :clobvar from testtable; "
		"	select testblob into :blobvar from testtable; "
		"end;");
	cur->defineOutputBindClob("clobvar");
	cur->defineOutputBindBlob("blobvar");
	assertTrue(cur->executeQuery());
	clobvar=cur->getOutputBindClob("clobvar");
	clobvarlength=cur->getOutputBindLength("clobvar");
	blobvar=cur->getOutputBindBlob("blobvar");
	blobvarlength=cur->getOutputBindLength("blobvar");
	assertEquals(clobvar,"hello",5);
	assertEquals(clobvarlength,5);
	assertEquals(blobvar,"hello",5);
	assertEquals(blobvarlength,5);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// long output bind
	stdoutput.printf("LONG OUTPUT BIND\n");
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
	stdoutput.printf("NEGATIVE INPUT BIND\n");
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
		"	return number "
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
	cur->prepareQuery(
		"begin "
		"	:out1:=testproc(:in1,:in2,:in3); "
		"end;");
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
		"begin "
		"	testproc(:in1,:in2,:in3,:out1,:out2,:out3); "
		"end;");
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
	stdoutput.printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	cur->sendQuery("drop package types");
	cur->sendQuery("drop function testproc");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create or replace package types is "
		"	type cursorType is ref cursor; "
		"end;"));
	assertTrue(cur->sendQuery(
		"create or replace "
		"function testproc(value in number) "
		"	return types.cursortype "
		"is "
		"	l_cursor    types.cursorType; "
		"begin "
		"	open l_cursor for "
		"		select "
		"			* "
		"		from "
		"			( "
		"			select 1 as testnumber from dual "
		"			union "
		"			select 2 as testnumber from dual "
		"			union "
		"			select 3 as testnumber from dual "
		"			union "
		"			select 4 as testnumber from dual "
		"			union "
		"			select 5 as testnumber from dual "
		"			union "
		"			select 6 as testnumber from dual "
		"			union "
		"			select 7 as testnumber from dual "
		"			union "
		"			select 8 as testnumber from dual "
		"			) "
		"		where "
		"			testnumber>value; "
		"	return l_cursor; "
		"end;"));
	cur->prepareQuery(
		"begin "
		"	:curs1:=testproc(5); "
		"	:curs2:=testproc(0); "
		"end;");
	cur->defineOutputBindCursor("curs1");
	cur->defineOutputBindCursor("curs2");
	assertTrue(cur->executeQuery());
	sqlrcursor	*bindcur1=cur->getOutputBindCursor("curs1");
	assertTrue(bindcur1->fetchFromBindCursor());
	assertEquals(bindcur1->getField(0,(uint32_t)0),"6");
	assertEquals(bindcur1->getField(1,(uint32_t)0),"7");
	assertEquals(bindcur1->getField(2,(uint32_t)0),"8");
	delete bindcur1;
	sqlrcursor	*bindcur2=cur->getOutputBindCursor("curs2");
	assertTrue(bindcur2->fetchFromBindCursor());
	assertEquals(bindcur2->getField(0,(uint32_t)0),"1");
	assertEquals(bindcur2->getField(1,(uint32_t)0),"2");
	assertEquals(bindcur2->getField(2,(uint32_t)0),"3");
	delete bindcur2;
	assertTrue(cur->sendQuery("drop function testproc"));
	assertTrue(cur->sendQuery("drop package types"));
	stdoutput.printf("\n");


	// temporary tables
	stdoutput.printf("TEMPORARY TABLES: \n");
	cur->prepareQuery("drop table $(HOSTNAME)_temptabledelete");
	cur->substitution("HOSTNAME",hostname);
	cur->executeQuery();
	cur->prepareQuery(
		"create global temporary table $(HOSTNAME)_temptabledelete ( "
		"	col1 number "
		") on commit delete rows");
	cur->substitution("HOSTNAME",hostname);
	cur->executeQuery();
	cur->prepareQuery("insert into $(HOSTNAME)_temptabledelete values (1)");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	cur->prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertTrue(con->commit());
	cur->prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	cur->prepareQuery("drop table $(HOSTNAME)_temptabledelete");
	cur->substitution("HOSTNAME",hostname);
	cur->executeQuery();
	stdoutput.printf("\n");
	cur->prepareQuery("truncate table $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	cur->executeQuery();
	cur->prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	cur->executeQuery();
	cur->prepareQuery(
		"create global temporary table $(HOSTNAME)_temptablepreserve ("
		"	col1 number "
		") on commit preserve rows");
	cur->substitution("HOSTNAME",hostname);
	cur->executeQuery();
	cur->prepareQuery(
		"insert into "
		"	$(HOSTNAME)_temptablepreserve "
		"values (1)");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	cur->prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertTrue(con->commit());
	cur->prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	cur->prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	cur->prepareQuery("truncate table $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	snooze::macrosnooze(2);
	cur->prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	assertTrue(cur->executeQuery());
	cur->prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
	cur->substitution("HOSTNAME",hostname);
	assertFalse(cur->executeQuery());
	stdoutput.printf("\n");


	// database is schema
	stdoutput.printf("DATABASE IS SCHEMA: \n");
	assertTrue(con->getDatabaseIsSchema());
	stdoutput.printf("\n");


	// catalog list
	stdoutput.printf("CATALOG LIST: \n");
	assertTrue(cur->getCatalogList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	assertEquals(cur->rowCount(),0);
	stdoutput.printf("\n");


	// schema list
	stdoutput.printf("SCHEMA LIST: \n");
	assertTrue(cur->getSchemaList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	bool	found=false;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		if (!charstring::compareIgnoringCase(
				cur->getField(i,"Database"),hostname)) {
			found=true;
			break;
		}
	}
	assertTrue(found);
	stdoutput.printf("\n");


	// table type list
	stdoutput.printf("TABLE TYPE LIST: \n");
	assertTrue(cur->getTableTypeList());
	assertEquals(cur->getColumnName(0),"table_type");
	assertEquals(cur->getField(0,"table_type"),"SYNONYM");
	assertEquals(cur->getField(1,"table_type"),"TABLE");
	assertEquals(cur->getField(2,"table_type"),"VIEW");
	stdoutput.printf("\n");


	// table list
	stdoutput.printf("TABLE LIST: \n");
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");
	cur->sendQuery("drop table testtable3");
	cur->sendQuery("drop table testtable4");
	assertTrue(cur->sendQuery(
		"create table testtable1 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(cur->sendQuery(
		"create table testtable2 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(cur->sendQuery(
		"create table testtable3 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(cur->sendQuery(
		"create table testtable4 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(cur->getTableList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"Tables_in_xxx");
		if (!charstring::compare(name,"TESTTABLE1") ||
			!charstring::compare(name,"TESTTABLE2") ||
			!charstring::compare(name,"TESTTABLE3") ||
			!charstring::compare(name,"TESTTABLE4")) {
			counter++;
		}
	}
	assertEquals(counter,4);
	assertTrue(cur->sendQuery("drop table testtable1"));
	assertTrue(cur->sendQuery("drop table testtable2"));
	assertTrue(cur->sendQuery("drop table testtable3"));
	assertTrue(cur->sendQuery("drop table testtable4"));
	stdoutput.printf("\n");


	// type info list
	stdoutput.printf("TYPE INFO LIST: \n");
	assertTrue(cur->getTypeInfoList("number"));
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
	assertEquals(cur->getField(0,"type_name"),"NUMBER");
	assertEquals(cur->getField(0,"data_type"),"-7");
	assertEquals(cur->getField(0,"precision"),"1");
	assertEquals(cur->getField(0,"local_type_name"),"NUMBER");
	assertTrue(cur->getTypeInfoList("char"));
	assertEquals(cur->getField(0,"type_name"),"CHAR");
	assertEquals(cur->getField(0,"data_type"),"1");
	assertEquals(cur->getField(0,"precision"),"2000");
	assertEquals(cur->getField(0,"local_type_name"),"CHAR");
	assertTrue(cur->getTypeInfoList("varchar2"));
	assertEquals(cur->getField(0,"type_name"),"VARCHAR2");
	assertEquals(cur->getField(0,"data_type"),"12");
	assertEquals(cur->getField(0,"precision"),"32767");
	assertEquals(cur->getField(0,"local_type_name"),"VARCHAR2");
	assertTrue(cur->getTypeInfoList("date"));
	assertEquals(cur->getField(0,"type_name"),"DATE");
	assertEquals(cur->getField(0,"data_type"),"92");
	assertEquals(cur->getField(0,"precision"),"7");
	assertEquals(cur->getField(0,"local_type_name"),"DATE");
	stdoutput.printf("\n");


	// column list
	stdoutput.printf("COLUMN LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
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
	assertEquals(cur->getField(0,"column_name"),"TESTNUMBER");
	assertEquals(cur->getField(1,"column_name"),"TESTCHAR");
	assertEquals(cur->getField(2,"column_name"),"TESTVARCHAR");
	assertEquals(cur->getField(3,"column_name"),"TESTDATE");
	assertEquals(cur->getField(4,"column_name"),"TESTLONG");
	assertEquals(cur->getField(5,"column_name"),"TESTCLOB");
	assertEquals(cur->getField(6,"column_name"),"TESTBLOB");
	assertEquals(cur->getField(0,"data_type"),"NUMBER");
	assertEquals(cur->getField(1,"data_type"),"CHAR");
	assertEquals(cur->getField(2,"data_type"),"VARCHAR2");
	assertEquals(cur->getField(3,"data_type"),"DATE");
	assertEquals(cur->getField(4,"data_type"),"LONG");
	assertEquals(cur->getField(5,"data_type"),"CLOB");
	assertEquals(cur->getField(6,"data_type"),"BLOB");
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// column list - auto_increment, primary key
	// oracle doesn't support auto_increment
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertTrue(charstring::contains(cur->getField(0,"column_key"),"PRI"));
	assertFalse(charstring::contains(cur->getField(1,"column_key"),"PRI"));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"));
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
	assertEquals(cur->getField(0,"table"),"TESTTABLE");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertEquals(cur->getField(0,"column_name"),"COL1");
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"));
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
	assertEquals(cur->getField(0,"table"),"TESTTABLE");
	assertEquals(cur->getField(0,"non_unique"),"0");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertEquals(cur->getField(0,"column_name"),"COL1");
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"3");
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	cur->sendQuery("drop procedure testproc1");
	cur->sendQuery("drop procedure testproc2");
	cur->sendQuery("drop procedure testproc3");
	cur->sendQuery("drop procedure testproc4");
	assertTrue(cur->sendQuery(
		"create procedure testproc1("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(cur->sendQuery(
		"create procedure testproc2("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(cur->sendQuery(
		"create procedure testproc3("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(cur->sendQuery(
		"create procedure testproc4("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(cur->getProcedureList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"routine_name");
		if (!charstring::compare(name,"TESTPROC1") ||
			!charstring::compare(name,"TESTPROC2") ||
			!charstring::compare(name,"TESTPROC3") ||
			!charstring::compare(name,"TESTPROC4")) {
			counter++;
		}
	}
	assertEquals(counter,4);
	stdoutput.printf("\n");


	// procedure parameter list
	stdoutput.printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(cur->getProcedureParameterList("testproc1",NULL));
	assertEquals(cur->getColumnName(0),"parameter_name");
	assertEquals(cur->getColumnName(1),"parameter_mode");
	assertEquals(cur->getColumnName(2),"data_type");
	assertEquals(cur->getColumnName(3),"character_maximum_length");
	assertEquals(cur->getColumnName(4),"ordinal_position");
	assertEquals(cur->rowCount(),4);
	assertEquals(cur->getField(0,"parameter_name"),"IN1");
	assertEquals(cur->getField(0,"parameter_mode"),"1");
	assertEquals(cur->getField(0,"data_type"),"NUMBER");
	assertEquals(cur->getField(0,"ordinal_position"),"1");
	assertEquals(cur->getField(1,"parameter_name"),"IN2");
	assertEquals(cur->getField(1,"parameter_mode"),"1");
	assertEquals(cur->getField(1,"data_type"),"CHAR");
	assertEquals(cur->getField(1,"ordinal_position"),"2");
	assertEquals(cur->getField(2,"parameter_name"),"IN3");
	assertEquals(cur->getField(2,"parameter_mode"),"1");
	assertEquals(cur->getField(2,"data_type"),"VARCHAR2");
	assertEquals(cur->getField(2,"ordinal_position"),"3");
	assertEquals(cur->getField(3,"parameter_name"),"IN4");
	assertEquals(cur->getField(3,"parameter_mode"),"1");
	assertEquals(cur->getField(3,"data_type"),"DATE");
	assertEquals(cur->getField(3,"ordinal_position"),"4");
	assertTrue(cur->sendQuery("drop procedure testproc1"));
	assertTrue(cur->sendQuery("drop procedure testproc2"));
	assertTrue(cur->sendQuery("drop procedure testproc3"));
	assertTrue(cur->sendQuery("drop procedure testproc4"));
	stdoutput.printf("\n");


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
	delete[] hostname;

	reportTestStatus();

	return status;
}
