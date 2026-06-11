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

	const char	*isolationlevels[]={"CS","UR","RS","RR",NULL};
	const char	*bindvars[]={"1","2","3","4","5","6",
				"7","8","9","10","11","12",NULL};
	const char	*bindvals[]={"7","7","7","7.5","7.5","7.5",
				"testchar7","testvarchar7",
				"01/01/2007","07:00:00",
				"testclob7",NULL};
	const char * const *cols;
	const char * const *fields;
	uint32_t	*fieldlens;
	const char	*subvars[]={"var1","var2","var3",NULL};
	int64_t		subvallongs[]={1,2,3};
	const char	*subvalstrings[]={"hi","hello","bye"};
	double		subvaldoubles[]={10.55,10.556,10.5556};
	uint32_t	precs[]={4,5,6};
	uint32_t	scales[]={2,3,4};
	int64_t		numvar;
	const char	*stringvar;
	const char	*nullvar;
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
	const char	*clobvar;
	uint32_t	clobvarlength;
	const char	*blobvar;
	uint32_t	blobvarlength;
	uint16_t	port;
	char		*socket;
	uint16_t	id;
	char		*filename;

	#define	LARGE_BUFFER_LENGTH	(20*1024)
	char		largebuffer[LARGE_BUFFER_LENGTH+1];


	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"db2inst1","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"db2");
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
	assertEquals(con->nextvalFormat(),"(nextval for %s)");
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
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01/01/2001', "
		"	'01:00:00', "
		"	NULL, "
		"	'testclob1', "
		"	blob('testblob1'))"));
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
		"	NULL, "
		"	?, "
		"	?)");
	assertEquals(cur->countBindVariables(),12);
	cur->inputBind("1",2);
	cur->inputBind("2",2);
	cur->inputBind("3",2);
	cur->inputBind("4",2.5,4,2);
	cur->inputBind("5",2.5,4,2);
	cur->inputBind("6",2.5,4,2);
	cur->inputBind("7","testchar2");
	cur->inputBind("8","testvarchar2");
	cur->inputBind("9",2002,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,2,0,0,0,NULL,false);
	cur->inputBindClob("11","testclob2",9);
	cur->inputBindBlob("12","testblob2",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",3);
	cur->inputBind("2",3);
	cur->inputBind("3",3);
	cur->inputBind("4",3.5,4,2);
	cur->inputBind("5",3.5,4,2);
	cur->inputBind("6",3.5,4,2);
	cur->inputBind("7","testchar3");
	cur->inputBind("8","testvarchar3");
	cur->inputBind("9",2003,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,3,0,0,0,NULL,false);
	cur->inputBindClob("11","testclob3",9);
	cur->inputBindBlob("12","testblob3",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",4);
	cur->inputBind("2",4);
	cur->inputBind("3",4);
	cur->inputBind("4",4.5,4,2);
	cur->inputBind("5",4.5,4,2);
	cur->inputBind("6",4.5,4,2);
	cur->inputBind("7","testchar4");
	cur->inputBind("8","testvarchar4");
	cur->inputBind("9",2004,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,4,0,0,0,NULL,false);
	cur->inputBindClob("11","testclob4",9);
	cur->inputBindBlob("12","testblob4",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",5);
	cur->inputBind("2",5);
	cur->inputBind("3",5);
	cur->inputBind("4",5.5,4,2);
	cur->inputBind("5",5.5,4,2);
	cur->inputBind("6",5.5,4,2);
	cur->inputBind("7","testchar5");
	cur->inputBind("8","testvarchar5");
	cur->inputBind("9",2005,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,5,0,0,0,NULL,false);
	cur->inputBindClob("11","testclob5",9);
	cur->inputBindBlob("12","testblob5",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",6);
	cur->inputBind("2",6);
	cur->inputBind("3",6);
	cur->inputBind("4",6.5,4,2);
	cur->inputBind("5",6.5,4,2);
	cur->inputBind("6",6.5,4,2);
	cur->inputBind("7","testchar6");
	cur->inputBind("8","testvarchar6");
	cur->inputBind("9",2006,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,6,0,0,0,NULL,false);
	cur->inputBindClob("11","testclob6",9);
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
	cur->inputBind("3",8);
	cur->inputBind("4",8.5,4,2);
	cur->inputBind("5",8.5,4,2);
	cur->inputBind("6",8.5,4,2);
	cur->inputBind("7","testchar8");
	cur->inputBind("8","testvarchar8");
	cur->inputBind("9",2008,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,8,0,0,0,NULL,false);
	cur->inputBindClob("11","testclob8",9);
	cur->inputBindBlob("12","testblob8",9);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	// input bind by name
	// db2 doesn't support bind by name


	// array of input binds by name
	// db2 doesn't support bind by name


	// input bind by name with validation
	// db2 doesn't support bind by name


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),13);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"TESTSMALLINT");
	assertEquals(cur->getColumnName(1),"TESTINT");
	assertEquals(cur->getColumnName(2),"TESTBIGINT");
	assertEquals(cur->getColumnName(3),"TESTDECIMAL");
	assertEquals(cur->getColumnName(4),"TESTREAL");
	assertEquals(cur->getColumnName(5),"TESTDOUBLE");
	assertEquals(cur->getColumnName(6),"TESTCHAR");
	assertEquals(cur->getColumnName(7),"TESTVARCHAR");
	assertEquals(cur->getColumnName(8),"TESTDATE");
	assertEquals(cur->getColumnName(9),"TESTTIME");
	assertEquals(cur->getColumnName(10),"TESTTIMESTAMP");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTSMALLINT");
	assertEquals(cols[1],"TESTINT");
	assertEquals(cols[2],"TESTBIGINT");
	assertEquals(cols[3],"TESTDECIMAL");
	assertEquals(cols[4],"TESTREAL");
	assertEquals(cols[5],"TESTDOUBLE");
	assertEquals(cols[6],"TESTCHAR");
	assertEquals(cols[7],"TESTVARCHAR");
	assertEquals(cols[8],"TESTDATE");
	assertEquals(cols[9],"TESTTIME");
	assertEquals(cols[10],"TESTTIMESTAMP");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"SMALLINT");
	assertEquals(cur->getColumnType("TESTSMALLINT"),"SMALLINT");
	assertEquals(cur->getColumnType(1),"INTEGER");
	assertEquals(cur->getColumnType("TESTINT"),"INTEGER");
	assertEquals(cur->getColumnType(2),"BIGINT");
	assertEquals(cur->getColumnType("TESTBIGINT"),"BIGINT");
	assertEquals(cur->getColumnType(3),"DECIMAL");
	assertEquals(cur->getColumnType("TESTDECIMAL"),"DECIMAL");
	assertEquals(cur->getColumnType(4),"REAL");
	assertEquals(cur->getColumnType("TESTREAL"),"REAL");
	assertEquals(cur->getColumnType(5),"DOUBLE");
	assertEquals(cur->getColumnType("TESTDOUBLE"),"DOUBLE");
	assertEquals(cur->getColumnType(6),"CHAR");
	assertEquals(cur->getColumnType("TESTCHAR"),"CHAR");
	assertEquals(cur->getColumnType(7),"VARCHAR");
	assertEquals(cur->getColumnType("TESTVARCHAR"),"VARCHAR");
	assertEquals(cur->getColumnType(8),"DATE");
	assertEquals(cur->getColumnType("TESTDATE"),"DATE");
	assertEquals(cur->getColumnType(9),"TIME");
	assertEquals(cur->getColumnType("TESTTIME"),"TIME");
	assertEquals(cur->getColumnType(10),"TIMESTAMP");
	assertEquals(cur->getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),2);
	assertEquals(cur->getColumnLength("TESTSMALLINT"),2);
	assertEquals(cur->getColumnLength(1),4);
	assertEquals(cur->getColumnLength("TESTINT"),4);
	assertEquals(cur->getColumnLength(2),8);
	assertEquals(cur->getColumnLength("TESTBIGINT"),8);
	assertEquals(cur->getColumnLength(3),12);
	assertEquals(cur->getColumnLength("TESTDECIMAL"),12);
	assertEquals(cur->getColumnLength(4),4);
	assertEquals(cur->getColumnLength("TESTREAL"),4);
	assertEquals(cur->getColumnLength(5),8);
	assertEquals(cur->getColumnLength("TESTDOUBLE"),8);
	assertEquals(cur->getColumnLength(6),40);
	assertEquals(cur->getColumnLength("TESTCHAR"),40);
	assertEquals(cur->getColumnLength(7),40);
	assertEquals(cur->getColumnLength("TESTVARCHAR"),40);
	assertEquals(cur->getColumnLength(8),6);
	assertEquals(cur->getColumnLength("TESTDATE"),6);
	assertEquals(cur->getColumnLength(9),6);
	assertEquals(cur->getColumnLength("TESTTIME"),6);
	assertEquals(cur->getColumnLength(10),16);
	assertEquals(cur->getColumnLength("TESTTIMESTAMP"),16);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("TESTSMALLINT"),1);
	assertEquals(cur->getLongest(1),1);
	assertEquals(cur->getLongest("TESTINT"),1);
	assertEquals(cur->getLongest(2),1);
	assertEquals(cur->getLongest("TESTBIGINT"),1);
	assertEquals(cur->getLongest(3),4);
	assertEquals(cur->getLongest("TESTDECIMAL"),4);
	assertEquals(cur->getLongest(4),12);
	assertEquals(cur->getLongest("TESTREAL"),12);
	assertEquals(cur->getLongest(5),21);
	assertEquals(cur->getLongest("TESTDOUBLE"),21);
	assertEquals(cur->getLongest(6),40);
	assertEquals(cur->getLongest("TESTCHAR"),40);
	assertEquals(cur->getLongest(7),12);
	assertEquals(cur->getLongest("TESTVARCHAR"),12);
	assertEquals(cur->getLongest(8),10);
	assertEquals(cur->getLongest("TESTDATE"),10);
	assertEquals(cur->getLongest(9),8);
	assertEquals(cur->getLongest("TESTTIME"),8);
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
	assertEquals(cur->getField(0,2),"1");
	assertEquals(cur->getField(0,3),"1.50");
	assertEquals(cur->getField(0,4),"1.500000E+00");
	assertEquals(cur->getField(0,5),"1.50000000000000E+000");
	assertEquals(cur->getField(0,6),"testchar1                               ");
	assertEquals(cur->getField(0,7),"testvarchar1");
	assertEquals(cur->getField(0,8),"2001-01-01");
	assertEquals(cur->getField(0,9),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8");
	assertEquals(cur->getField(7,3),"8.50");
	assertEquals(cur->getField(7,4),"8.500000E+00");
	assertEquals(cur->getField(7,5),"8.50000000000000E+000");
	assertEquals(cur->getField(7,6),"testchar8                               ");
	assertEquals(cur->getField(7,7),"testvarchar8");
	assertEquals(cur->getField(7,8),"2008-01-01");
	assertEquals(cur->getField(7,9),"08:00:00");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),1);
	assertEquals(cur->getFieldLength(0,2),1);
	assertEquals(cur->getFieldLength(0,3),4);
	assertEquals(cur->getFieldLength(0,4),12);
	assertEquals(cur->getFieldLength(0,5),21);
	assertEquals(cur->getFieldLength(0,6),40);
	assertEquals(cur->getFieldLength(0,7),12);
	assertEquals(cur->getFieldLength(0,8),10);
	assertEquals(cur->getFieldLength(0,9),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),1);
	assertEquals(cur->getFieldLength(7,2),1);
	assertEquals(cur->getFieldLength(7,3),4);
	assertEquals(cur->getFieldLength(7,4),12);
	assertEquals(cur->getFieldLength(7,5),21);
	assertEquals(cur->getFieldLength(7,6),40);
	assertEquals(cur->getFieldLength(7,7),12);
	assertEquals(cur->getFieldLength(7,8),10);
	assertEquals(cur->getFieldLength(7,9),8);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"TESTSMALLINT"),"1");
	assertEquals(cur->getField(0,"TESTINT"),"1");
	assertEquals(cur->getField(0,"TESTBIGINT"),"1");
	assertEquals(cur->getField(0,"TESTDECIMAL"),"1.50");
	assertEquals(cur->getField(0,"TESTREAL"),"1.500000E+00");
	assertEquals(cur->getField(0,"TESTDOUBLE"),"1.50000000000000E+000");
	assertEquals(cur->getField(0,"TESTCHAR"),"testchar1                               ");
	assertEquals(cur->getField(0,"TESTVARCHAR"),"testvarchar1");
	assertEquals(cur->getField(0,"TESTDATE"),"2001-01-01");
	assertEquals(cur->getField(0,"TESTTIME"),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"TESTSMALLINT"),"8");
	assertEquals(cur->getField(7,"TESTINT"),"8");
	assertEquals(cur->getField(7,"TESTBIGINT"),"8");
	assertEquals(cur->getField(7,"TESTDECIMAL"),"8.50");
	assertEquals(cur->getField(7,"TESTREAL"),"8.500000E+00");
	assertEquals(cur->getField(7,"TESTDOUBLE"),"8.50000000000000E+000");
	assertEquals(cur->getField(7,"TESTCHAR"),"testchar8                               ");
	assertEquals(cur->getField(7,"TESTVARCHAR"),"testvarchar8");
	assertEquals(cur->getField(7,"TESTDATE"),"2008-01-01");
	assertEquals(cur->getField(7,"TESTTIME"),"08:00:00");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"TESTSMALLINT"),1);
	assertEquals(cur->getFieldLength(0,"TESTINT"),1);
	assertEquals(cur->getFieldLength(0,"TESTBIGINT"),1);
	assertEquals(cur->getFieldLength(0,"TESTDECIMAL"),4);
	assertEquals(cur->getFieldLength(0,"TESTREAL"),12);
	assertEquals(cur->getFieldLength(0,"TESTDOUBLE"),21);
	assertEquals(cur->getFieldLength(0,"TESTCHAR"),40);
	assertEquals(cur->getFieldLength(0,"TESTVARCHAR"),12);
	assertEquals(cur->getFieldLength(0,"TESTDATE"),10);
	assertEquals(cur->getFieldLength(0,"TESTTIME"),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"TESTSMALLINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTBIGINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTDECIMAL"),4);
	assertEquals(cur->getFieldLength(7,"TESTREAL"),12);
	assertEquals(cur->getFieldLength(7,"TESTDOUBLE"),21);
	assertEquals(cur->getFieldLength(7,"TESTCHAR"),40);
	assertEquals(cur->getFieldLength(7,"TESTVARCHAR"),12);
	assertEquals(cur->getFieldLength(7,"TESTDATE"),10);
	assertEquals(cur->getFieldLength(7,"TESTTIME"),8);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1");
	assertEquals(fields[2],"1");
	assertEquals(fields[3],"1.50");
	assertEquals(fields[4],"1.500000E+00");
	assertEquals(fields[5],"1.50000000000000E+000");
	assertEquals(fields[6],"testchar1                               ");
	assertEquals(fields[7],"testvarchar1");
	assertEquals(fields[8],"2001-01-01");
	assertEquals(fields[9],"01:00:00");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],1);
	assertEquals(fieldlens[2],1);
	assertEquals(fieldlens[3],4);
	assertEquals(fieldlens[4],12);
	assertEquals(fieldlens[5],21);
	assertEquals(fieldlens[6],40);
	assertEquals(fieldlens[7],12);
	assertEquals(fieldlens[8],10);
	assertEquals(fieldlens[9],8);
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
		"	testsmallint "));
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
		"	testsmallint "));
	assertEquals(cur->getColumnName((uint32_t)0),NULL);
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),NULL);
	cur->getColumnInfo();
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertEquals(cur->getColumnName((uint32_t)0),"TESTSMALLINT");
	assertEquals(cur->getColumnLength((uint32_t)0),2);
	assertEquals(cur->getColumnType((uint32_t)0),"SMALLINT");
	stdoutput.printf("\n");


	// suspended session
	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
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
		"	testsmallint "));
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
		"	testsmallint "));
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
		"	testsmallint "));
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
		"	testsmallint "));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	delete[] filename;
	stdoutput.printf("\n");


	// column count for cached result set
	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),13);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"TESTSMALLINT");
	assertEquals(cur->getColumnName(1),"TESTINT");
	assertEquals(cur->getColumnName(2),"TESTBIGINT");
	assertEquals(cur->getColumnName(3),"TESTDECIMAL");
	assertEquals(cur->getColumnName(4),"TESTREAL");
	assertEquals(cur->getColumnName(5),"TESTDOUBLE");
	assertEquals(cur->getColumnName(6),"TESTCHAR");
	assertEquals(cur->getColumnName(7),"TESTVARCHAR");
	assertEquals(cur->getColumnName(8),"TESTDATE");
	assertEquals(cur->getColumnName(9),"TESTTIME");
	assertEquals(cur->getColumnName(10),"TESTTIMESTAMP");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTSMALLINT");
	assertEquals(cols[1],"TESTINT");
	assertEquals(cols[2],"TESTBIGINT");
	assertEquals(cols[3],"TESTDECIMAL");
	assertEquals(cols[4],"TESTREAL");
	assertEquals(cols[5],"TESTDOUBLE");
	assertEquals(cols[6],"TESTCHAR");
	assertEquals(cols[7],"TESTVARCHAR");
	assertEquals(cols[8],"TESTDATE");
	assertEquals(cols[9],"TESTTIME");
	assertEquals(cols[10],"TESTTIMESTAMP");
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
		"	testsmallint "));
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
		"	testsmallint "));
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
		"	testint"));
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
	assertTrue(cur->sendQuery("drop table testtable"));
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
	assertTrue(cur->sendQuery("create table testtable (col1 integer)"));
	// db2 DDL is transactional; commit so the table is visible to the
	// second connection (the commit implicitly starts a new tx)
	assertTrue(con->commit());
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"db2inst1","testpassword",0,1);
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
						"db2inst1","testpassword",0,1);
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
						"db2inst1","testpassword",0,1);
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
						"db2inst1","testpassword",0,1);
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
						"db2inst1","testpassword",0,1);
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
	assertEquals(con->getTransactionModel(),"implicit");
	assertFalse(con->getAutoCommit());
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("values ($(var1),'$(var2)','$(var3)')");
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
	cur->prepareQuery("values ('$(var1)','$(var2)','$(var3)')");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	stdoutput.printf("\n");
	cur->prepareQuery("values ($(var1),$(var2),$(var3))");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3");
	stdoutput.printf("\n");
	cur->prepareQuery("values ($(var1),$(var2),$(var3))");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// nulls as nulls
	stdoutput.printf("NULLS AS NULLS: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery("select NULL,1,NULL from sysibm.sysdummy1"));
	assertEquals(cur->getField(0,(uint32_t)0),NULL);
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select NULL,1,NULL from sysibm.sysdummy1"));
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
	assertTrue(con->commit());
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	?, "
		"	?, "
		"	?, "
		"	?)");
	cur->inputBindClob("1","",0);
	cur->inputBindClob("2",NULL,0);
	cur->inputBindBlob("3","",0);
	cur->inputBindBlob("4",NULL,0);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),"");
	assertEquals(cur->getField(0,3),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// long lobs
	stdoutput.printf("LONG LOBS: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(con->commit());
	cur->prepareQuery("insert into testtable values (?,?)");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
	cur->inputBindBlob("2",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getFieldLength(0,"TESTCLOB"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,"TESTCLOB"),largebuffer);
	assertEquals(cur->getFieldLength(0,"TESTBLOB"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,"TESTBLOB"),largebuffer,
						LARGE_BUFFER_LENGTH);
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// output bind by position
	stdoutput.printf("OUTPUT BIND BY POSITION: \n");
	cur->sendQuery("drop procedure testproc");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	out out1 int, "
		"	out out2 varchar(20), "
		"	out out3 double, "
		"	out out4 date, "
		"	out out5 varchar(20)) "
		"language sql "
		"begin "
		"	set out1 = 1; "
		"	set out2 = 'hello'; "
		"	set out3 = 2.5; "
		"	set out4 = '2001-02-03'; "
		"	set out5 = null; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("call testproc(?,?,?,?,?)");
	assertEquals(cur->countBindVariables(),5);
	cur->defineOutputBindInteger("1");
	cur->defineOutputBindString("2",20);
	cur->defineOutputBindDouble("3");
	cur->defineOutputBindDate("4");
	cur->defineOutputBindString("5",20);
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
	assertEquals(isnegative,false);
	nullvar=cur->getOutputBindString("5");
	assertEquals(nullvar,NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// output bind by name
	// db2 doesn't support bind by name


	// output bind by name with validation
	// db2 doesn't support bind by name


	// lob output bind
	stdoutput.printf("LOB OUTPUT BIND: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(con->commit());
	cur->prepareQuery("insert into testtable values ('hello',?)");
	cur->inputBindBlob("1","hello",5);
	assertTrue(cur->executeQuery());
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	out out1 clob, "
		"	out out2 blob) "
		"language sql "
		"begin "
		"	select testclob into out1 from testtable; "
		"	select testblob into out2 from testtable; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("call testproc(?,?)");
	cur->defineOutputBindClob("1");
	cur->defineOutputBindBlob("2");
	assertTrue(cur->executeQuery());
	clobvar=cur->getOutputBindClob("1");
	clobvarlength=cur->getOutputBindLength("1");
	blobvar=cur->getOutputBindBlob("2");
	blobvarlength=cur->getOutputBindLength("2");
	assertEquals(clobvar,"hello",5);
	assertEquals(clobvarlength,5);
	assertEquals(blobvar,"hello",5);
	assertEquals(blobvarlength,5);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// long output bind
	stdoutput.printf("LONG OUTPUT BIND: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in in1 clob, "
		"	out out1 clob) "
		"language sql "
		"begin "
		"	set out1 = in1; "
		"end"));
	assertTrue(con->commit());
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->prepareQuery("call testproc(?,?)");
	cur->inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
	cur->defineOutputBindClob("2");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("2"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getOutputBindClob("2"),largebuffer);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// negative input bind
	stdoutput.printf("NEGATIVE INPUT BIND: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery("create table testtable (testval integer)"));
	assertTrue(con->commit());
	cur->prepareQuery("insert into testtable values (?)");
	cur->inputBind("1",-1);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select testval from testtable"));
	assertEquals(cur->getField(0,"TESTVAL"),"-1");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// bind validation
	// db2 doesn't support bind by name


	// rebinding
	stdoutput.printf("REBINDING: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in in1 int, "
		"	out out1 int) "
		"language sql "
		"begin "
		"	set out1 = in1; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("call testproc(?,?)");
	cur->inputBind("1",1);
	cur->defineOutputBindInteger("2");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("2"),1);
	cur->inputBind("1",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("2"),2);
	cur->inputBind("1",3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("2"),3);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// reexecute
	stdoutput.printf("REEXECUTE: \n");
	cur->prepareQuery("select 1 from sysibm.sysdummy1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->prepareQuery("select cast(? as integer) from sysibm.sysdummy1");
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
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20)) "
		"language sql "
		"begin "
		"	return; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("call testproc(?,?,?)");
	cur->inputBind("1",1);
	cur->inputBind("2",2.5,2,1);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// stored procedure returning single value
	stdoutput.printf("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	cur->sendQuery("drop function testfunc");
	assertTrue(cur->sendQuery(
		"create function testfunc("
		"	in1 int, "
		"	in2 double, "
		"	in3 varchar(20)) "
		"returns int "
		"language sql "
		"begin "
		"	return in1; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("select testfunc(?,?,?) from sysibm.sysdummy1");
	cur->inputBind("1",1);
	cur->inputBind("2",2.5,2,1);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertTrue(cur->sendQuery("drop function testfunc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// stored procedure returning multiple values
	stdoutput.printf("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20), "
		"	in in4 clob, "
		"	in in5 blob, "
		"	out out1 int, "
		"	out out2 double, "
		"	out out3 varchar(20), "
		"	out out4 clob, "
		"	out out5 blob) "
		"language sql "
		"begin "
		"	set out1 = in1; "
		"	set out2 = in2; "
		"	set out3 = in3; "
		"	set out4 = in4; "
		"	set out5 = in5; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("call testproc(?,?,?,?,?,?,?,?,?,?)");
	cur->inputBind("1",1);
	cur->inputBind("2",2.5,2,1);
	cur->inputBind("3","hello");
	cur->inputBindClob("4","clob",4);
	cur->inputBindBlob("5","blob",4);
	cur->defineOutputBindInteger("6");
	cur->defineOutputBindDouble("7");
	cur->defineOutputBindString("8",20);
	cur->defineOutputBindClob("9");
	cur->defineOutputBindBlob("10");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("6"),1);
	assertEquals(cur->getOutputBindDouble("7"),2.5);
	assertEquals(cur->getOutputBindString("8"),"hello");
	assertEquals(cur->getOutputBindClob("9"),"clob");
	assertEquals(cur->getOutputBindBlob("10"),"blob");
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// stored procedure returning result set
	stdoutput.printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc() "
		"result set 1 "
		"language sql "
		"begin "
		"	declare c1 cursor with return for "
		"		select 1 from sysibm.sysdummy1 "
		"		union "
		"		select 2 from sysibm.sysdummy1 "
		"		union "
		"		select 3 from sysibm.sysdummy1 "
		"		union "
		"		select 4 from sysibm.sysdummy1 "
		"		union "
		"		select 5 from sysibm.sysdummy1 "
		"		union "
		"		select 6 from sysibm.sysdummy1 "
		"		union "
		"		select 7 from sysibm.sysdummy1 "
		"		union "
		"		select 8 from sysibm.sysdummy1; "
		"	open c1; "
		"end"));
	assertTrue(con->commit());
	assertTrue(cur->sendQuery("call testproc()"));
	assertEquals(cur->rowCount(),8);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// temporary tables
	stdoutput.printf("TEMPORARY TABLES: \n");
	cur->sendQuery("drop table session.temptable");
	assertTrue(cur->sendQuery(
			"declare global temporary table session.temptable ("
			"	col1 int "
			") not logged"));
	assertTrue(cur->sendQuery("insert into session.temptable values (1)"));
	assertTrue(cur->sendQuery("select count(*) from session.temptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("select count(*) from session.temptable"));
	stdoutput.printf("\n");

	// declared temp table with an unqualified name; session. must be
	// prepended for the end-of-session drop to succeed
	cur->sendQuery("drop table session.temptable");
	assertTrue(cur->sendQuery(
			"declare global temporary table temptable ("
			"	col1 int "
			") not logged"));
	assertTrue(cur->sendQuery("insert into session.temptable values (1)"));
	assertTrue(cur->sendQuery("select count(*) from session.temptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("select count(*) from session.temptable"));
	stdoutput.printf("\n");

	// created temp table; at the end of the session its rows are
	// truncated rather than the table being dropped, so the table still
	// exists afterward but is empty
	// (no drop here - dropping a created global temp table while it's
	// still instantiated on the pooled connection blocks indefinitely in
	// db2; the table goes away when the instance is shut down)
	cur->sendQuery("drop table ctemptable");
	assertTrue(cur->sendQuery(
			"create global temporary table ctemptable ("
			"	col1 int "
			") on commit preserve rows"));
	assertTrue(cur->sendQuery("insert into ctemptable values (1)"));
	assertTrue(cur->sendQuery("select count(*) from ctemptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("select count(*) from ctemptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	stdoutput.printf("\n");


	// encoded binary data
	stdoutput.printf("ENCODED BINARY DATA: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery("create table testtable (col1 blob)"));
	byte_t	buffer[256];
	for (uint16_t i=0; i<256; i++) {
		buffer[i]=i;
	}
	stringbuffer	query;
	query.append("insert into testtable values (blob(X'");
	char	hex[3];
	for (uint64_t i=0; i<sizeof(buffer); i++) {
		charstring::printf(hex,sizeof(hex),"%02x",buffer[i]);
		query.append(hex);
	}
	query.append("'))");
	assertTrue(cur->sendQuery(query.getString()));
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
			"	(col1 int not null "
			"	generated always as identity, "
			"	col2 int, "
			"	primary key(col1))"));
	assertTrue(cur->sendQuery(
			"insert into testtable (col2) values (1)"));
	assertEquals(con->getLastInsertId(),(uint64_t)1);
	assertTrue(cur->sendQuery("drop table testtable"));
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
	assertInResultSet(cur,"Database","DB2INST1");
	stdoutput.printf("\n");


	// table type list
	stdoutput.printf("TABLE TYPE LIST: \n");
	assertTrue(cur->getTableTypeList());
	assertEquals(cur->getColumnName(0),"table_type");
	assertInResultSet(cur,"table_type","TABLE");
	stdoutput.printf("\n");


	// table list
	stdoutput.printf("TABLE LIST: \n");
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");
	cur->sendQuery("drop table testtable3");
	cur->sendQuery("drop table testtable4");
	assertTrue(cur->sendQuery(
		"create table testtable1 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(cur->sendQuery(
		"create table testtable2 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(cur->sendQuery(
		"create table testtable3 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(cur->sendQuery(
		"create table testtable4 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(con->commit());
	assertTrue(cur->getTableList(NULL));
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE4");
	assertTrue(cur->sendQuery("drop table testtable1"));
	assertTrue(cur->sendQuery("drop table testtable2"));
	assertTrue(cur->sendQuery("drop table testtable3"));
	assertTrue(cur->sendQuery("drop table testtable4"));
	assertTrue(con->commit());
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
	assertEquals(cur->getField(0,"precision"),"254");
	assertEquals(cur->getField(0,"local_type_name"),"CHAR");
	assertTrue(cur->getTypeInfoList("varchar"));
	assertEquals(cur->getField(0,"type_name"),"VARCHAR");
	assertEquals(cur->getField(0,"data_type"),"12");
	assertEquals(cur->getField(0,"precision"),"32672");
	assertEquals(cur->getField(0,"local_type_name"),"VARCHAR");
	assertTrue(cur->getTypeInfoList("date"));
	assertEquals(cur->getField(0,"type_name"),"DATE");
	assertEquals(cur->getField(0,"data_type"),"91");
	assertEquals(cur->getField(0,"precision"),"10");
	assertEquals(cur->getField(0,"local_type_name"),"DATE");
	stdoutput.printf("\n");


	// column list
	stdoutput.printf("COLUMN LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(con->commit());
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
	assertEquals(cur->getField(0,"column_name"),"TESTSMALLINT");
	assertEquals(cur->getField(1,"column_name"),"TESTINT");
	assertEquals(cur->getField(2,"column_name"),"TESTBIGINT");
	assertEquals(cur->getField(3,"column_name"),"TESTDECIMAL");
	assertEquals(cur->getField(4,"column_name"),"TESTREAL");
	assertEquals(cur->getField(5,"column_name"),"TESTDOUBLE");
	assertEquals(cur->getField(6,"column_name"),"TESTCHAR");
	assertEquals(cur->getField(7,"column_name"),"TESTVARCHAR");
	assertEquals(cur->getField(8,"column_name"),"TESTDATE");
	assertEquals(cur->getField(9,"column_name"),"TESTTIME");
	assertEquals(cur->getField(10,"column_name"),"TESTTIMESTAMP");
	assertEquals(cur->getField(11,"column_name"),"TESTCLOB");
	assertEquals(cur->getField(12,"column_name"),"TESTBLOB");
	assertEquals(cur->getField(0,"data_type"),"SMALLINT");
	assertEquals(cur->getField(1,"data_type"),"INTEGER");
	assertEquals(cur->getField(2,"data_type"),"BIGINT");
	assertEquals(cur->getField(3,"data_type"),"DECIMAL");
	assertEquals(cur->getField(4,"data_type"),"REAL");
	assertEquals(cur->getField(5,"data_type"),"DOUBLE");
	assertEquals(cur->getField(6,"data_type"),"CHARACTER");
	assertEquals(cur->getField(7,"data_type"),"VARCHAR");
	assertEquals(cur->getField(8,"data_type"),"DATE");
	assertEquals(cur->getField(9,"data_type"),"TIME");
	assertEquals(cur->getField(10,"data_type"),"TIMESTAMP");
	assertEquals(cur->getField(11,"data_type"),"CLOB");
	assertEquals(cur->getField(12,"data_type"),"BLOB");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int generated always as identity primary key, "
		"	col2 int)"));
	assertTrue(con->commit());
	assertTrue(cur->getColumnList("testtable",NULL));
	assertEquals(cur->getField(0,"extra"),"auto_increment");
	assertEquals(cur->getField(0,"column_key"),"PRI");
	assertEquals(cur->getField(1,"extra"),"");
	assertEquals(cur->getField(1,"column_key"),"");
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int not null primary key, "
		"	col2 int)"));
	assertTrue(con->commit());
	assertTrue(cur->getColumnList("testtable",NULL));
	assertEquals(cur->getField(0,"extra"),"");
	assertEquals(cur->getField(0,"column_key"),"PRI");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int not null primary key, "
		"	col2 int)"));
	assertTrue(con->commit());
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
	assertTrue(!charstring::compare(cur->getField(0,"table"),"TESTTABLE"));
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"COL1"));
	assertStartsWith(cur->getField(0,"key_name"),"SQL");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int not null primary key, "
		"	col2 int)"));
	assertTrue(con->commit());
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
	assertTrue(!charstring::compare(cur->getField(0,"table"),"TESTTABLE"));
	assertEquals(cur->getField(0,"non_unique"),"0");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"COL1"));
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"3");
	assertStartsWith(cur->getField(0,"key_name"),"SQL");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	cur->sendQuery("drop procedure testproc1");
	cur->sendQuery("drop procedure testproc2");
	cur->sendQuery("drop procedure testproc3");
	cur->sendQuery("drop procedure testproc4");
	assertTrue(cur->sendQuery(
		"create procedure testproc1("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"language sql begin end"));
	assertTrue(cur->sendQuery(
		"create procedure testproc2("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"language sql begin end"));
	assertTrue(cur->sendQuery(
		"create procedure testproc3("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"language sql begin end"));
	assertTrue(cur->sendQuery(
		"create procedure testproc4("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"language sql begin end"));
	assertTrue(con->commit());
	assertTrue(cur->getProcedureList(NULL));
	assertInResultSet(cur,"routine_name","TESTPROC1");
	assertInResultSet(cur,"routine_name","TESTPROC2");
	assertInResultSet(cur,"routine_name","TESTPROC3");
	assertInResultSet(cur,"routine_name","TESTPROC4");
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
	assertEquals(cur->getField(0,"data_type"),"INTEGER");
	assertEquals(cur->getField(0,"ordinal_position"),"1");
	assertEquals(cur->getField(1,"parameter_name"),"IN2");
	assertEquals(cur->getField(1,"parameter_mode"),"1");
	assertEquals(cur->getField(1,"data_type"),"CHARACTER");
	assertEquals(cur->getField(1,"ordinal_position"),"2");
	assertEquals(cur->getField(2,"parameter_name"),"IN3");
	assertEquals(cur->getField(2,"parameter_mode"),"1");
	assertEquals(cur->getField(2,"data_type"),"VARCHAR");
	assertEquals(cur->getField(2,"ordinal_position"),"3");
	assertEquals(cur->getField(3,"parameter_name"),"IN4");
	assertEquals(cur->getField(3,"parameter_mode"),"1");
	assertEquals(cur->getField(3,"data_type"),"DATE");
	assertEquals(cur->getField(3,"ordinal_position"),"4");
	assertTrue(cur->sendQuery("drop procedure testproc1"));
	assertTrue(cur->sendQuery("drop procedure testproc2"));
	assertTrue(cur->sendQuery("drop procedure testproc3"));
	assertTrue(cur->sendQuery("drop procedure testproc4"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
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
