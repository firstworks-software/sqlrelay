// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	const char	*bindvars[]={"1","2","3","4","5","6",
				"7","8","9","10","11","12",NULL};
	const char	*bindvals[]={"7","7","7.5","7.5","7.5","7.5",
				"01-JAN-2007","07:00:00",
				"testchar7","testvarchar7",NULL,"testblob7"};
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
	char		*socket;
	uint16_t	id;
	char		*filename;

	#define	LARGE_BUFFER_LENGTH	(20*1024)
	char		largebuffer[LARGE_BUFFER_LENGTH+1];


	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"firebird");
	stdoutput.printf("\n");


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// transaction state
	stdoutput.printf("TRANSACTION STATE: \n");
	assertEquals(con->getDefaultTransactionModel(),"implicit");
	assertEquals(con->getTransactionModel(),"implicit");
	assertTrue(con->getInTransaction());
	assertFalse(con->getAutoCommit());
	stdoutput.printf("\n");


	// bind format
	stdoutput.printf("BIND FORMAT: \n");
	assertEquals(con->bindFormat(),"?");
	stdoutput.printf("\n");


	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"next value for %s");
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	// though firebird does support a "set transaction ..." statement to
	// set the isolation level, it looks like, in firebird, you can really
	// only set it through the TPB at the start of a transaction, so
	// attempts to set it should fail
	assertFalse(con->setIsolationLevel("read committed"));
	assertEquals(con->getIsolationLevel(),"read committed");
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	cur->sendQuery("delete from testtable");
	con->commit();
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'01-JAN-2001', "
		"	'01:00:00', "
		"	'testchar1', "
		"	'testvarchar1', "
		"	NULL, "
		"	'testblob1')"));
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?)");
	assertEquals(cur->countBindVariables(),12);
	cur->inputBind("1",2);
	cur->inputBind("2",2);
	cur->inputBind("3",2.5,2,1);
	cur->inputBind("4",2.5,2,1);
	cur->inputBind("5",2.5,2,1);
	cur->inputBind("6",2.5,2,1);
	cur->inputBind("7",2002,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,2,0,0,0,NULL,false);
	cur->inputBind("9","testchar2");
	cur->inputBind("10","testvarchar2");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("12","testblob2",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",3);
	cur->inputBind("2",3);
	cur->inputBind("3",3.5,2,1);
	cur->inputBind("4",3.5,2,1);
	cur->inputBind("5",3.5,2,1);
	cur->inputBind("6",3.5,2,1);
	cur->inputBind("7",2003,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,3,0,0,0,NULL,false);
	cur->inputBind("9","testchar3");
	cur->inputBind("10","testvarchar3");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("12","testblob3",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",4);
	cur->inputBind("2",4);
	cur->inputBind("3",4.5,2,1);
	cur->inputBind("4",4.5,2,1);
	cur->inputBind("5",4.5,2,1);
	cur->inputBind("6",4.5,2,1);
	cur->inputBind("7",2004,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,4,0,0,0,NULL,false);
	cur->inputBind("9","testchar4");
	cur->inputBind("10","testvarchar4");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("12","testblob4",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",5);
	cur->inputBind("2",5);
	cur->inputBind("3",5.5,2,1);
	cur->inputBind("4",5.5,2,1);
	cur->inputBind("5",5.5,2,1);
	cur->inputBind("6",5.5,2,1);
	cur->inputBind("7",2005,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,5,0,0,0,NULL,false);
	cur->inputBind("9","testchar5");
	cur->inputBind("10","testvarchar5");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("12","testblob5",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",6);
	cur->inputBind("2",6);
	cur->inputBind("3",6.5,2,1);
	cur->inputBind("4",6.5,2,1);
	cur->inputBind("5",6.5,2,1);
	cur->inputBind("6",6.5,2,1);
	cur->inputBind("7",2006,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,6,0,0,0,NULL,false);
	cur->inputBind("9","testchar6");
	cur->inputBind("10","testvarchar6");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("12","testblob6",9);
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
	cur->inputBind("1",8);
	cur->inputBind("2",8);
	cur->inputBind("3",8.5,2,1);
	cur->inputBind("4",8.5,2,1);
	cur->inputBind("5",8.5,2,1);
	cur->inputBind("6",8.5,2,1);
	cur->inputBind("7",2008,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,8,0,0,0,NULL,false);
	cur->inputBind("9","testchar8");
	cur->inputBind("10","testvarchar8");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("12","testblob8",9);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name
	// firebird doesn't support bind by name


	// array of input binds by name
	// firebird doesn't support bind by name


	// input bind by name with validation
	// firebird doesn't support bind by name


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testinteger "));
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),12);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"TESTINTEGER");
	assertEquals(cur->getColumnName(1),"TESTSMALLINT");
	assertEquals(cur->getColumnName(2),"TESTDECIMAL");
	assertEquals(cur->getColumnName(3),"TESTNUMERIC");
	assertEquals(cur->getColumnName(4),"TESTFLOAT");
	assertEquals(cur->getColumnName(5),"TESTDOUBLE");
	assertEquals(cur->getColumnName(6),"TESTDATE");
	assertEquals(cur->getColumnName(7),"TESTTIME");
	assertEquals(cur->getColumnName(8),"TESTCHAR");
	assertEquals(cur->getColumnName(9),"TESTVARCHAR");
	assertEquals(cur->getColumnName(10),"TESTTIMESTAMP");
	assertEquals(cur->getColumnName(11),"TESTBLOB");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTINTEGER");
	assertEquals(cols[1],"TESTSMALLINT");
	assertEquals(cols[2],"TESTDECIMAL");
	assertEquals(cols[3],"TESTNUMERIC");
	assertEquals(cols[4],"TESTFLOAT");
	assertEquals(cols[5],"TESTDOUBLE");
	assertEquals(cols[6],"TESTDATE");
	assertEquals(cols[7],"TESTTIME");
	assertEquals(cols[8],"TESTCHAR");
	assertEquals(cols[9],"TESTVARCHAR");
	assertEquals(cols[10],"TESTTIMESTAMP");
	assertEquals(cols[11],"TESTBLOB");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"INTEGER");
	assertEquals(cur->getColumnType("TESTINTEGER"),"INTEGER");
	assertEquals(cur->getColumnType(1),"SMALLINT");
	assertEquals(cur->getColumnType("TESTSMALLINT"),"SMALLINT");
	assertEquals(cur->getColumnType(2),"DECIMAL");
	assertEquals(cur->getColumnType("TESTDECIMAL"),"DECIMAL");
	assertEquals(cur->getColumnType(3),"NUMERIC");
	assertEquals(cur->getColumnType("TESTNUMERIC"),"NUMERIC");
	assertEquals(cur->getColumnType(4),"FLOAT");
	assertEquals(cur->getColumnType("TESTFLOAT"),"FLOAT");
	assertEquals(cur->getColumnType(5),"DOUBLE PRECISION");
	assertEquals(cur->getColumnType("TESTDOUBLE"),"DOUBLE PRECISION");
	assertEquals(cur->getColumnType(6),"DATE");
	assertEquals(cur->getColumnType("TESTDATE"),"DATE");
	assertEquals(cur->getColumnType(7),"TIME");
	assertEquals(cur->getColumnType("TESTTIME"),"TIME");
	assertEquals(cur->getColumnType(8),"CHAR");
	assertEquals(cur->getColumnType("TESTCHAR"),"CHAR");
	assertEquals(cur->getColumnType(9),"VARCHAR");
	assertEquals(cur->getColumnType("TESTVARCHAR"),"VARCHAR");
	assertEquals(cur->getColumnType(10),"TIMESTAMP");
	assertEquals(cur->getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
	assertEquals(cur->getColumnType(11),"BLOB");
	assertEquals(cur->getColumnType("TESTBLOB"),"BLOB");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnLength("TESTINTEGER"),4);
	assertEquals(cur->getColumnLength(1),2);
	assertEquals(cur->getColumnLength("TESTSMALLINT"),2);
	assertEquals(cur->getColumnLength(2),8);
	assertEquals(cur->getColumnLength("TESTDECIMAL"),8);
	assertEquals(cur->getColumnLength(3),8);
	assertEquals(cur->getColumnLength("TESTNUMERIC"),8);
	assertEquals(cur->getColumnLength(4),4);
	assertEquals(cur->getColumnLength("TESTFLOAT"),4);
	assertEquals(cur->getColumnLength(5),8);
	assertEquals(cur->getColumnLength("TESTDOUBLE"),8);
	assertEquals(cur->getColumnLength(6),4);
	assertEquals(cur->getColumnLength("TESTDATE"),4);
	assertEquals(cur->getColumnLength(7),4);
	assertEquals(cur->getColumnLength("TESTTIME"),4);
	assertEquals(cur->getColumnLength(8),50);
	assertEquals(cur->getColumnLength("TESTCHAR"),50);
	assertEquals(cur->getColumnLength(9),50);
	assertEquals(cur->getColumnLength("TESTVARCHAR"),50);
	assertEquals(cur->getColumnLength(10),8);
	assertEquals(cur->getColumnLength("TESTTIMESTAMP"),8);
	assertEquals(cur->getColumnLength(11),8);
	assertEquals(cur->getColumnLength("TESTBLOB"),8);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("TESTINTEGER"),1);
	assertEquals(cur->getLongest(1),1);
	assertEquals(cur->getLongest("TESTSMALLINT"),1);
	assertEquals(cur->getLongest(2),4);
	assertEquals(cur->getLongest("TESTDECIMAL"),4);
	assertEquals(cur->getLongest(3),4);
	assertEquals(cur->getLongest("TESTNUMERIC"),4);
	assertEquals(cur->getLongest(4),6);
	assertEquals(cur->getLongest("TESTFLOAT"),6);
	assertEquals(cur->getLongest(5),6);
	assertEquals(cur->getLongest("TESTDOUBLE"),6);
	assertEquals(cur->getLongest(6),10);
	assertEquals(cur->getLongest("TESTDATE"),10);
	assertEquals(cur->getLongest(7),8);
	assertEquals(cur->getLongest("TESTTIME"),8);
	assertEquals(cur->getLongest(8),50);
	assertEquals(cur->getLongest("TESTCHAR"),50);
	assertEquals(cur->getLongest(9),12);
	assertEquals(cur->getLongest("TESTVARCHAR"),12);
	assertEquals(cur->getLongest(10),0);
	assertEquals(cur->getLongest("TESTTIMESTAMP"),0);
	assertEquals(cur->getLongest(11),9);
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
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),"1.50");
	assertEquals(cur->getField(0,3),"1.50");
	assertEquals(cur->getField(0,4),"1.5000");
	assertEquals(cur->getField(0,5),"1.5000");
	assertEquals(cur->getField(0,6),"2001:01:01");
	assertEquals(cur->getField(0,7),"01:00:00");
	assertEquals(cur->getField(0,8),"testchar1                                         ");
	assertEquals(cur->getField(0,9),"testvarchar1");
	assertEquals(cur->getField(0,11),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8.50");
	assertEquals(cur->getField(7,3),"8.50");
	assertEquals(cur->getField(7,4),"8.5000");
	assertEquals(cur->getField(7,5),"8.5000");
	assertEquals(cur->getField(7,6),"2008:01:01");
	assertEquals(cur->getField(7,7),"08:00:00");
	assertEquals(cur->getField(7,8),"testchar8                                         ");
	assertEquals(cur->getField(7,9),"testvarchar8");
	assertEquals(cur->getField(7,11),"testblob8");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),1);
	assertEquals(cur->getFieldLength(0,2),4);
	assertEquals(cur->getFieldLength(0,3),4);
	assertEquals(cur->getFieldLength(0,4),6);
	assertEquals(cur->getFieldLength(0,5),6);
	assertEquals(cur->getFieldLength(0,6),10);
	assertEquals(cur->getFieldLength(0,7),8);
	assertEquals(cur->getFieldLength(0,8),50);
	assertEquals(cur->getFieldLength(0,9),12);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),1);
	assertEquals(cur->getFieldLength(7,2),4);
	assertEquals(cur->getFieldLength(7,3),4);
	assertEquals(cur->getFieldLength(7,4),6);
	assertEquals(cur->getFieldLength(7,5),6);
	assertEquals(cur->getFieldLength(7,6),10);
	assertEquals(cur->getFieldLength(7,7),8);
	assertEquals(cur->getFieldLength(7,8),50);
	assertEquals(cur->getFieldLength(7,9),12);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"TESTINTEGER"),"1");
	assertEquals(cur->getField(0,"TESTSMALLINT"),"1");
	assertEquals(cur->getField(0,"TESTDECIMAL"),"1.50");
	assertEquals(cur->getField(0,"TESTNUMERIC"),"1.50");
	assertEquals(cur->getField(0,"TESTFLOAT"),"1.5000");
	assertEquals(cur->getField(0,"TESTDOUBLE"),"1.5000");
	assertEquals(cur->getField(0,"TESTDATE"),"2001:01:01");
	assertEquals(cur->getField(0,"TESTTIME"),"01:00:00");
	assertEquals(cur->getField(0,"TESTCHAR"),"testchar1                                         ");
	assertEquals(cur->getField(0,"TESTVARCHAR"),"testvarchar1");
	assertEquals(cur->getField(0,"TESTBLOB"),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"TESTINTEGER"),"8");
	assertEquals(cur->getField(7,"TESTSMALLINT"),"8");
	assertEquals(cur->getField(7,"TESTDECIMAL"),"8.50");
	assertEquals(cur->getField(7,"TESTNUMERIC"),"8.50");
	assertEquals(cur->getField(7,"TESTFLOAT"),"8.5000");
	assertEquals(cur->getField(7,"TESTDOUBLE"),"8.5000");
	assertEquals(cur->getField(7,"TESTDATE"),"2008:01:01");
	assertEquals(cur->getField(7,"TESTTIME"),"08:00:00");
	assertEquals(cur->getField(7,"TESTCHAR"),"testchar8                                         ");
	assertEquals(cur->getField(7,"TESTVARCHAR"),"testvarchar8");
	assertEquals(cur->getField(7,"TESTBLOB"),"testblob8");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"TESTINTEGER"),1);
	assertEquals(cur->getFieldLength(0,"TESTSMALLINT"),1);
	assertEquals(cur->getFieldLength(0,"TESTDECIMAL"),4);
	assertEquals(cur->getFieldLength(0,"TESTNUMERIC"),4);
	assertEquals(cur->getFieldLength(0,"TESTFLOAT"),6);
	assertEquals(cur->getFieldLength(0,"TESTDOUBLE"),6);
	assertEquals(cur->getFieldLength(0,"TESTDATE"),10);
	assertEquals(cur->getFieldLength(0,"TESTTIME"),8);
	assertEquals(cur->getFieldLength(0,"TESTCHAR"),50);
	assertEquals(cur->getFieldLength(0,"TESTVARCHAR"),12);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"TESTINTEGER"),1);
	assertEquals(cur->getFieldLength(7,"TESTSMALLINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTDECIMAL"),4);
	assertEquals(cur->getFieldLength(7,"TESTNUMERIC"),4);
	assertEquals(cur->getFieldLength(7,"TESTFLOAT"),6);
	assertEquals(cur->getFieldLength(7,"TESTDOUBLE"),6);
	assertEquals(cur->getFieldLength(7,"TESTDATE"),10);
	assertEquals(cur->getFieldLength(7,"TESTTIME"),8);
	assertEquals(cur->getFieldLength(7,"TESTCHAR"),50);
	assertEquals(cur->getFieldLength(7,"TESTVARCHAR"),12);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1");
	assertEquals(fields[2],"1.50");
	assertEquals(fields[3],"1.50");
	assertEquals(fields[4],"1.5000");
	assertEquals(fields[5],"1.5000");
	assertEquals(fields[6],"2001:01:01");
	assertEquals(fields[7],"01:00:00");
	assertEquals(fields[8],"testchar1                                         ");
	assertEquals(fields[9],"testvarchar1");
	assertEquals(fields[11],"testblob1");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],1);
	assertEquals(fieldlens[2],4);
	assertEquals(fieldlens[3],4);
	assertEquals(fieldlens[4],6);
	assertEquals(fieldlens[5],6);
	assertEquals(fieldlens[6],10);
	assertEquals(fieldlens[7],8);
	assertEquals(fieldlens[8],50);
	assertEquals(fieldlens[9],12);
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
		"	testinteger "));
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
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testinteger "));
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
		"	testinteger "));
	assertEquals(cur->getColumnName(0),"TESTINTEGER");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnType((uint32_t)0),"INTEGER");
	stdoutput.printf("\n");


	// suspended session
	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	delete[] filename;
	stdoutput.printf("\n");


	// column count for cached result set
	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),12);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"TESTINTEGER");
	assertEquals(cur->getColumnName(1),"TESTSMALLINT");
	assertEquals(cur->getColumnName(2),"TESTDECIMAL");
	assertEquals(cur->getColumnName(3),"TESTNUMERIC");
	assertEquals(cur->getColumnName(4),"TESTFLOAT");
	assertEquals(cur->getColumnName(5),"TESTDOUBLE");
	assertEquals(cur->getColumnName(6),"TESTDATE");
	assertEquals(cur->getColumnName(7),"TESTTIME");
	assertEquals(cur->getColumnName(8),"TESTCHAR");
	assertEquals(cur->getColumnName(9),"TESTVARCHAR");
	assertEquals(cur->getColumnName(10),"TESTTIMESTAMP");
	assertEquals(cur->getColumnName(11),"TESTBLOB");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTINTEGER");
	assertEquals(cols[1],"TESTSMALLINT");
	assertEquals(cols[2],"TESTDECIMAL");
	assertEquals(cols[3],"TESTNUMERIC");
	assertEquals(cols[4],"TESTFLOAT");
	assertEquals(cols[5],"TESTDOUBLE");
	assertEquals(cols[6],"TESTDATE");
	assertEquals(cols[7],"TESTTIME");
	assertEquals(cols[8],"TESTCHAR");
	assertEquals(cols[9],"TESTVARCHAR");
	assertEquals(cols[10],"TESTTIMESTAMP");
	assertEquals(cols[11],"TESTBLOB");
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
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
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
	stdoutput.printf("\n");


	// reset transaction state
	stdoutput.printf("RESET TRANSACTION STATE: \n");
	assertTrue(con->commit());
	assertEquals(con->getTransactionModel(),"implicit");
	assertFalse(con->getAutoCommit());
	stdoutput.printf("\n");


	// transaction behavior - implicit
	stdoutput.printf("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(con->setTransactionModel("implicit"));
	assertEquals(con->getTransactionModel(),"implicit");
	// truncate testtable so this section starts with it empty;
	// firebird DDL on the table here would otherwise hit cursor-state
	// issues at the next commit, so we reuse the existing schema and
	// just write to one column (testinteger)
	assertTrue(cur->sendQuery("delete from testtable"));
	// commit so the truncation is visible to the second connection
	// (the commit implicitly starts a new tx)
	assertTrue(con->commit());
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// session is in a transaction; insert is not visible until commit
	assertTrue(con->getInTransaction());
	assertFalse(con->getAutoCommit());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (1)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	// commit makes it visible, and implicitly starts a new transaction
	assertTrue(con->commit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// rollback discards, and implicitly starts a new transaction
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (2)"));
	assertTrue(con->rollback());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	assertFalse(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (3)"));
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
	stdoutput.printf("\n");


	// transaction behavior - explicit
	stdoutput.printf("TRANSACTION BEHAVIOR - explicit: \n");
	assertTrue(con->setTransactionModel("explicit"));
	assertEquals(con->getTransactionModel(),"explicit");
	// truncate testtable so this section starts with it empty (delete
	// autocommits here since explicit-model defaults to autocommit-on)
	assertTrue(cur->sendQuery("delete from testtable"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// begin starts a new transaction; insert is not visible until commit
	assertTrue(con->begin());
	assertTrue(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (1)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	// commit makes it visible; no new transaction is started
	assertTrue(con->commit());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// begin, insert, rollback discards; no new transaction is started
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (2)"));
	assertTrue(con->rollback());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(con->autoCommitOn());
	assertTrue(con->getAutoCommit());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (3)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
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
	// truncate testtable so this section starts with it empty
	assertTrue(cur->sendQuery("delete from testtable"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// begin starts a transaction; commit makes it visible
	assertTrue(con->begin());
	assertTrue(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (1)"));
	assertTrue(con->commit());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// begin, insert, rollback discards
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (2)"));
	assertTrue(con->rollback());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// during a transaction started by begin(), autoCommitOn is a
	// no-op: the autocommit setting takes effect after the user
	// explicitly commits/rollbacks the tx (mysql-native semantic)
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (3)"));
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
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (4)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"3");
	// autoCommitOff takes effect immediately when not in a transaction
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	// autocommit-off persists across commit/rollback; each commit or
	// rollback ends the current implicit tx and a new one starts for
	// the next statement
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (5)"));
	assertTrue(con->commit());
	assertFalse(con->getAutoCommit());
	assertTrue(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"4");
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (6)"));
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
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (7)"));
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
	stdoutput.printf("\n");


	// transaction behavior - explicit-error
	stdoutput.printf("TRANSACTION BEHAVIOR - explicit-error: \n");
	assertTrue(con->setTransactionModel("explicit-error"));
	assertEquals(con->getTransactionModel(),"explicit-error");
	// truncate testtable so this section starts with it empty
	assertTrue(cur->sendQuery("delete from testtable"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// begin, insert, commit
	assertTrue(con->begin());
	assertTrue(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (1)"));
	assertTrue(con->commit());
	assertFalse(con->getInTransaction());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// begin, insert, rollback
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (2)"));
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
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (3)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(con->autoCommitOff());
	assertFalse(con->getAutoCommit());
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	stdoutput.printf("\n");


	// transaction behavior - none
	stdoutput.printf("TRANSACTION BEHAVIOR - none: \n");
	assertTrue(con->setTransactionModel("none"));
	assertEquals(con->getTransactionModel(),"none");
	// truncate testtable so this section starts with it empty
	assertTrue(cur->sendQuery("delete from testtable"));
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// no transactions; everything is visible immediately
	assertTrue(con->getAutoCommit());
	assertFalse(con->getInTransaction());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (1)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	// commit and rollback are no-ops
	assertTrue(con->commit());
	assertTrue(cur->sendQuery("insert into testtable (testinteger) values (2)"));
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
	stdoutput.printf("\n");


	// reset transaction behavior
	stdoutput.printf("RESET TRANSACTION BEHAVIOR: \n");
	assertTrue(con->setTransactionModel(con->getDefaultTransactionModel()));
	assertEquals(con->getTransactionModel(),"implicit");
	assertFalse(con->getAutoCommit());
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),'$(var2)',$(var3) from rdb$database");
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
	cur->prepareQuery(
		"select "
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	rdb$database ");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	stdoutput.printf("\n");
	cur->prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3");
	stdoutput.printf("\n");
	cur->prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// nulls as nulls
	stdoutput.printf("NULLS AS NULLS: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery("select 1,NULL,NULL from rdb$database"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select 1,NULL,NULL from rdb$database"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"");
	assertEquals(cur->getField(0,2),"");
	stdoutput.printf("\n");


	// null and empty lobs
	stdoutput.printf("NULL AND EMPTY LOBS: \n");
	cur->getNullsAsNulls();
	cur->sendQuery("delete from testtable1");
	cur->prepareQuery("insert into testtable1 values (?)");
	cur->inputBindBlob("1","",0);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select testblob from testtable1"));
	assertEquals(cur->getField(0,"TESTBLOB"),"");
	cur->sendQuery("delete from testtable1");
	cur->prepareQuery("insert into testtable1 values (?)");
	cur->inputBindBlob("1",NULL,0);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select testblob from testtable1"));
	assertEquals(cur->getField(0,"TESTBLOB"),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("delete from testtable1"));
	stdoutput.printf("\n");


	// long lobs
	stdoutput.printf("LONG LOBS: \n");
	cur->sendQuery("delete from testtable1");
	cur->prepareQuery("insert into testtable1 values (?)");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select testblob from testtable1"));
	assertEquals(cur->getFieldLength(0,"TESTBLOB"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,"TESTBLOB"),largebuffer);
	assertTrue(cur->sendQuery("delete from testtable1"));
	stdoutput.printf("\n");


	// output bind by position
	stdoutput.printf("OUTPUT BIND BY POSITION: \n");
	cur->getNullsAsNulls();
	cur->prepareQuery("execute procedure testproc ?, ?, ?, ?");
	cur->inputBind("1",1);
	cur->inputBind("2",1.5,2,1);
	cur->inputBind("3","hello");
	cur->inputBindBlob("4","blob",4);
	cur->defineOutputBindInteger("1");
	cur->defineOutputBindDouble("2");
	cur->defineOutputBindString("3",20);
	cur->defineOutputBindBlob("4");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("1"),1);
	double d=cur->getOutputBindDouble("2");
	assertEquals(d,1.5);
	assertEquals(cur->getOutputBindString("3"),"hello               ");
	assertEquals(cur->getOutputBindBlob("4"),"blob");
	cur->getNullsAsEmptyStrings();
	stdoutput.printf("\n");


	// output bind by name
	// firebird doesn't support bind by name


	// output bind by name with validation
	// firebird doesn't support bind by name


	// lob output bind
	stdoutput.printf("LOB OUTPUT BIND: \n");
	cur->prepareQuery("execute procedure testproc1 ?");
	cur->inputBindBlob("1","hello",5);
	cur->defineOutputBindBlob("1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindBlob("1"),"hello",5);
	assertEquals(cur->getOutputBindLength("1"),5);
	stdoutput.printf("\n");


	// long output bind
	stdoutput.printf("LONG OUTPUT BIND: \n");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->prepareQuery("execute procedure testproc1 ?");
	cur->inputBindBlob("1",largebuffer,LARGE_BUFFER_LENGTH);
	cur->defineOutputBindBlob("1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("1"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getOutputBindBlob("1"),largebuffer,
						LARGE_BUFFER_LENGTH);
	stdoutput.printf("\n");


	// negative input bind
	stdoutput.printf("NEGATIVE INPUT BIND: \n");
	cur->prepareQuery("select cast(? as integer) from rdb$database");
	cur->inputBind("1",-1);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"-1");
	stdoutput.printf("\n");


	// bind validation
	// firebird doesn't support bind by name


	// rebinding
	stdoutput.printf("REBINDING: \n");
	cur->prepareQuery("execute procedure testproc ?, ?, ?, ?");
	cur->inputBind("1",1);
	cur->inputBind("2",1.5,2,1);
	cur->inputBind("3","hello");
	cur->inputBindBlob("4","blob",4);
	cur->defineOutputBindInteger("1");
	cur->defineOutputBindDouble("2");
	cur->defineOutputBindString("3",20);
	cur->defineOutputBindBlob("4");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("1"),1);
	cur->inputBind("1",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("1"),2);
	cur->inputBind("1",3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("1"),3);
	stdoutput.printf("\n");


	// reexecute
	stdoutput.printf("REEXECUTE: \n");
	cur->prepareQuery("select 1 from rdb$database");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->prepareQuery("select cast(? as int) from rdb$database");
	cur->inputBind("1",1);
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->inputBind("1",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"2");
	stdoutput.printf("\n");


	// stored procedure returning no value
	stdoutput.printf("STORED PROCEDURE RETURNING NO VALUE: \n");
	cur->prepareQuery(
		"execute block (in1 int = ?, "
		"	in2 double precision = ?, "
		"	in3 varchar(20) = ?) "
		"as "
		"begin "
		"end");
	cur->inputBind("1",1);
	cur->inputBind("2",1.5,2,1);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// stored procedure returning single value
	stdoutput.printf("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	cur->prepareQuery(
		"execute block (in1 int = ?, "
		"	in2 double precision = ?, "
		"	in3 varchar(20) = ?) "
		"returns (out1 int) "
		"as "
		"begin "
		"	out1 = in1; "
		"	suspend; "
		"end");
	cur->inputBind("1",1);
	cur->inputBind("2",1.5,2,1);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");


	// stored procedure returning multiple values
	stdoutput.printf("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	cur->prepareQuery(
		"execute block (in1 int = ?, "
		"	in2 double precision = ?, "
		"	in3 varchar(20) = ?) "
		"returns (out1 int, "
		"	out2 double precision, "
		"	out3 varchar(20)) "
		"as "
		"begin "
		"	out1 = in1; "
		"	out2 = in2; "
		"	out3 = in3; "
		"	suspend; "
		"end");
	cur->inputBind("1",1);
	cur->inputBind("2",1.5,2,1);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"1.5000");
	assertEquals(cur->getField(0,2),"hello");
	stdoutput.printf("\n");


	// stored procedure returning result set
	stdoutput.printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	cur->prepareQuery(
		"execute block "
		"returns (out1 int) "
		"as "
		"declare i int; "
		"begin "
		"	i = 1; "
		"	while (i <= 8) do "
		"	begin "
		"		out1 = i; "
		"		suspend; "
		"		i = i + 1; "
		"	end "
		"end");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");


	// temporary tables
	// firebird supports temporary tables, but we're omitting this for now


	// encoded binary data
	// firebird doesn't support encoded binary data


	// quotes
	stdoutput.printf("QUOTES: \n");
	cur->sendQuery("delete from table testtable1");
	assertTrue(cur->sendQuery(
			"insert into testtable1 values ('''''')"));
	assertTrue(cur->sendQuery("select testblob from testtable1"));
	assertEquals(cur->getFieldLength(0,(uint32_t)0),2);
	assertEquals(charstring::compare(
			cur->getField(0,(uint32_t)0),"''"),0);
	assertTrue(cur->sendQuery("delete from testtable1"));
	stdoutput.printf("\n");


	// last insert id
	// firebird doesn't support auto-increment


	// database is schema
	stdoutput.printf("DATABASE IS SCHEMA: \n");
	assertFalse(con->getDatabaseIsSchema());
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
	// firebird has no schemas
	assertEquals(cur->rowCount(),0);
	stdoutput.printf("\n");


	// table type list
	stdoutput.printf("TABLE TYPE LIST: \n");
	assertTrue(cur->getTableTypeList());
	assertEquals(cur->getColumnName(0),"table_type");
	assertInResultSet(cur,"table_type","TABLE");
	stdoutput.printf("\n");


	// table list
	stdoutput.printf("TABLE LIST: \n");
	assertTrue(cur->getTableList(NULL));
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3");
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
	assertEquals(cur->getField(0,"precision"),"10");
	assertEquals(cur->getField(0,"local_type_name"),"INTEGER");
	assertTrue(cur->getTypeInfoList("char"));
	assertEquals(cur->getField(0,"type_name"),"CHAR");
	assertEquals(cur->getField(0,"data_type"),"1");
	assertEquals(cur->getField(0,"precision"),"32767");
	assertEquals(cur->getField(0,"local_type_name"),"CHAR");
	assertTrue(cur->getTypeInfoList("varchar"));
	assertEquals(cur->getField(0,"type_name"),"VARCHAR");
	assertEquals(cur->getField(0,"data_type"),"12");
	assertEquals(cur->getField(0,"precision"),"32765");
	assertEquals(cur->getField(0,"local_type_name"),"VARCHAR");
	assertTrue(cur->getTypeInfoList("date"));
	assertEquals(cur->getField(0,"type_name"),"DATE");
	assertEquals(cur->getField(0,"data_type"),"91");
	assertEquals(cur->getField(0,"precision"),"10");
	assertEquals(cur->getField(0,"local_type_name"),"DATE");
	stdoutput.printf("\n");


	// column list
	stdoutput.printf("COLUMN LIST: \n");
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
	assertTrue(!charstring::compare(
			cur->getField(0,"column_name"),"TESTINTEGER"));
	assertTrue(!charstring::compare(
			cur->getField(1,"column_name"),"TESTSMALLINT"));
	assertTrue(!charstring::compare(
			cur->getField(2,"column_name"),"TESTDECIMAL"));
	assertTrue(!charstring::compare(
			cur->getField(3,"column_name"),"TESTNUMERIC"));
	assertTrue(!charstring::compare(
			cur->getField(4,"column_name"),"TESTFLOAT"));
	assertTrue(!charstring::compare(
			cur->getField(5,"column_name"),"TESTDOUBLE"));
	assertTrue(!charstring::compare(
			cur->getField(6,"column_name"),"TESTDATE"));
	assertTrue(!charstring::compare(
			cur->getField(7,"column_name"),"TESTTIME"));
	assertTrue(!charstring::compare(
			cur->getField(8,"column_name"),"TESTCHAR"));
	assertTrue(!charstring::compare(
			cur->getField(9,"column_name"),"TESTVARCHAR"));
	assertTrue(!charstring::compare(
			cur->getField(10,"column_name"),"TESTTIMESTAMP"));
	assertTrue(!charstring::compare(
			cur->getField(11,"column_name"),"TESTBLOB"));
	assertTrue(!charstring::compare(
			cur->getField(0,"data_type"),"INTEGER"));
	assertTrue(!charstring::compare(
			cur->getField(1,"data_type"),"SMALLINT"));
	assertTrue(!charstring::compare(
			cur->getField(2,"data_type"),"DECIMAL"));
	assertTrue(!charstring::compare(
			cur->getField(3,"data_type"),"NUMERIC"));
	assertTrue(!charstring::compare(
			cur->getField(4,"data_type"),"FLOAT"));
	assertTrue(!charstring::compare(
			cur->getField(5,"data_type"),"DOUBLE PRECISION"));
	assertTrue(!charstring::compare(
			cur->getField(6,"data_type"),"DATE"));
	assertTrue(!charstring::compare(
			cur->getField(7,"data_type"),"TIME"));
	assertTrue(!charstring::compare(
			cur->getField(8,"data_type"),"CHAR"));
	assertTrue(!charstring::compare(
			cur->getField(9,"data_type"),"VARCHAR"));
	assertTrue(!charstring::compare(
			cur->getField(10,"data_type"),"TIMESTAMP"));
	assertTrue(!charstring::compare(
			cur->getField(11,"data_type"),"BLOB SUB_TYPE BINARY"));
	stdoutput.printf("\n");


	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	assertTrue(cur->getColumnList("testtable2",NULL));
	assertEquals(cur->getField(0,"extra"),"auto_increment");
	assertEquals(cur->getField(0,"column_key"),"PRI");
	assertEquals(cur->getField(1,"extra"),"");
	assertEquals(cur->getField(1,"column_key"),"");
	stdoutput.printf("\n");
	assertTrue(cur->getColumnList("testtable3",NULL));
	assertEquals(cur->getField(0,"extra"),"");
	assertEquals(cur->getField(0,"column_key"),"PRI");
	stdoutput.printf("\n");


	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	assertTrue(cur->getPrimaryKeysList("testtable2",NULL));
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
	assertTrue(!charstring::compare(cur->getField(0,"table"),"TESTTABLE2"));
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"COL1"));
	assertStartsWith(cur->getField(0,"key_name"),"INTEG_");
	stdoutput.printf("\n");


	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	assertTrue(cur->getKeyAndIndexList("testtable2",NULL));
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
	assertTrue(!charstring::compare(cur->getField(0,"table"),"TESTTABLE2"));
	assertEquals(cur->getField(0,"non_unique"),"0");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"COL1"));
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"3");
	assertStartsWith(cur->getField(0,"key_name"),"RDB$PRIMARY");
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	assertTrue(cur->getProcedureList(NULL));
	assertInResultSet(cur,"routine_name","TESTPROC");
	assertInResultSet(cur,"routine_name","TESTPROC1");
	stdoutput.printf("\n");


	// procedure parameter list
	stdoutput.printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(cur->getProcedureParameterList("testproc",NULL));
	assertEquals(cur->getColumnName(0),"parameter_name");
	assertEquals(cur->getColumnName(1),"parameter_mode");
	assertEquals(cur->getColumnName(2),"data_type");
	assertEquals(cur->getColumnName(3),"character_maximum_length");
	assertEquals(cur->getColumnName(4),"ordinal_position");
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(0,"parameter_name"),"OUT1");
	assertEquals(cur->getField(0,"parameter_mode"),"4");
	assertEquals(cur->getField(0,"data_type"),"INTEGER");
	assertEquals(cur->getField(0,"ordinal_position"),"1");
	assertEquals(cur->getField(1,"parameter_name"),"OUT2");
	assertEquals(cur->getField(1,"parameter_mode"),"4");
	assertEquals(cur->getField(1,"data_type"),"FLOAT");
	assertEquals(cur->getField(1,"ordinal_position"),"2");
	assertEquals(cur->getField(2,"parameter_name"),"OUT3");
	assertEquals(cur->getField(2,"parameter_mode"),"4");
	assertEquals(cur->getField(2,"data_type"),"VARCHAR");
	assertEquals(cur->getField(2,"ordinal_position"),"3");
	assertEquals(cur->getField(3,"parameter_name"),"OUT4");
	assertEquals(cur->getField(3,"parameter_mode"),"4");
	assertEquals(cur->getField(3,"data_type"),"BLOB SUB_TYPE BINARY");
	assertEquals(cur->getField(3,"ordinal_position"),"4");
	assertEquals(cur->getField(4,"parameter_name"),"IN1");
	assertEquals(cur->getField(4,"parameter_mode"),"1");
	assertEquals(cur->getField(4,"data_type"),"INTEGER");
	assertEquals(cur->getField(4,"ordinal_position"),"1");
	assertEquals(cur->getField(5,"parameter_name"),"IN2");
	assertEquals(cur->getField(5,"parameter_mode"),"1");
	assertEquals(cur->getField(5,"data_type"),"FLOAT");
	assertEquals(cur->getField(5,"ordinal_position"),"2");
	assertEquals(cur->getField(6,"parameter_name"),"IN3");
	assertEquals(cur->getField(6,"parameter_mode"),"1");
	assertEquals(cur->getField(6,"data_type"),"VARCHAR");
	assertEquals(cur->getField(6,"ordinal_position"),"3");
	assertEquals(cur->getField(7,"parameter_name"),"IN4");
	assertEquals(cur->getField(7,"parameter_mode"),"1");
	assertEquals(cur->getField(7,"data_type"),"BLOB SUB_TYPE BINARY");
	assertEquals(cur->getField(7,"ordinal_position"),"4");
	stdoutput.printf("\n");


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
