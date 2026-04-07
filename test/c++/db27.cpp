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

	const char	*isolationlevels[]={"CS","UR","RS","RR",NULL};
	const char	*bindvars[]={"1","2","3","4","5","6",
				"7","8","9","10","11","12",NULL};
	const char	*bindvals[]={"4","4","4","4.4","4.4","4.4",
				"testchar4","testvarchar4",
				"01/01/2004","04:00:00",
				"testclob4",NULL,NULL};
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
		"	testclob clob(1K), "
		"	testblob blob(1K))"));
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
		"	1.1, "
		"	1.1, "
		"	1.1, "
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


	// bind by position
	stdoutput.printf("BIND BY POSITION: \n");
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
	cur->inputBind("4",2.2,4,2);
	cur->inputBind("5",2.2,4,2);
	cur->inputBind("6",2.2,4,2);
	cur->inputBind("7","testchar2");
	cur->inputBind("8","testvarchar2");
	cur->inputBind("9","01/01/2002");
	cur->inputBind("10","02:00:00");
	cur->inputBindClob("11","testclob1",9);
	cur->inputBindBlob("12","testblob1",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",3);
	cur->inputBind("2",3);
	cur->inputBind("3",3);
	cur->inputBind("4",3.3,4,2);
	cur->inputBind("5",3.3,4,2);
	cur->inputBind("6",3.3,4,2);
	cur->inputBind("7","testchar3");
	cur->inputBind("8","testvarchar3");
	cur->inputBind("9","01/01/2003");
	cur->inputBind("10","03:00:00");
	cur->inputBindClob("11","testclob3",9);
	cur->inputBindBlob("12","testblob3",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of binds by position
	stdoutput.printf("ARRAY OF BINDS BY POSITION: \n");
	cur->clearBinds();
	cur->inputBinds(bindvars,bindvals);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// bind by name
	// db2 doesn't support bind by name


	// array of binds by name
	// db2 doesn't support bind by name


	// bind by name with validation
	// db2 doesn't support bind by name


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	5, "
		"	5, "
		"	5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	'testchar5', "
		"	'testvarchar5', "
		"	'01/01/2005', "
		"	'05:00:00', "
		"	NULL, "
		"	'testclob5', "
		"	blob('testblob5'))"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	6, "
		"	6, "
		"	6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	'testchar6', "
		"	'testvarchar6', "
		"	'01/01/2006', "
		"	'06:00:00', "
		"	NULL, "
		"	'testclob6', "
		"	blob('testblob6'))"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	7, "
		"	7, "
		"	7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	'testchar7', "
		"	'testvarchar7', "
		"	'01/01/2007', "
		"	'07:00:00', "
		"	NULL, "
		"	'testclob7', "
		"	blob('testblob7'))"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	8, "
		"	8, "
		"	8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	'testchar8', "
		"	'testvarchar8', "
		"	'01/01/2008', "
		"	'08:00:00', "
		"	NULL, "
		"	'testclob8', "
		"	blob('testblob8'))"));
	stdoutput.printf("\n");


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
	//assertEquals(cur->getLongest(4),3);
	//assertEquals(cur->getLongest("TESTREAL"),3);
	//assertEquals(cur->getLongest(5),3);
	//assertEquals(cur->getLongest("TESTDOUBLE"),3);
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
	assertEquals(cur->getField(0,3),"1.10");
	//assertEquals(cur->getField(0,4),"1.1");
	//assertEquals(cur->getField(0,5),"1.1");
	assertEquals(cur->getField(0,6),"testchar1                               ");
	assertEquals(cur->getField(0,7),"testvarchar1");
	assertEquals(cur->getField(0,8),"2001-01-01");
	assertEquals(cur->getField(0,9),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8");
	assertEquals(cur->getField(7,3),"8.80");
	//assertEquals(cur->getField(7,4),"8.8");
	//assertEquals(cur->getField(7,5),"8.8");
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
	//assertEquals(cur->getFieldLength(0,4),3);
	//assertEquals(cur->getFieldLength(0,5),3);
	assertEquals(cur->getFieldLength(0,6),40);
	assertEquals(cur->getFieldLength(0,7),12);
	assertEquals(cur->getFieldLength(0,8),10);
	assertEquals(cur->getFieldLength(0,9),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),1);
	assertEquals(cur->getFieldLength(7,2),1);
	assertEquals(cur->getFieldLength(7,3),4);
	//assertEquals(cur->getFieldLength(7,4),3);
	//assertEquals(cur->getFieldLength(7,5),3);
	assertEquals(cur->getFieldLength(7,6),40);
	assertEquals(cur->getFieldLength(7,7),12);
	assertEquals(cur->getFieldLength(7,8),10);
	assertEquals(cur->getFieldLength(7,9),8);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testsmallint"),"1");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testbigint"),"1");
	assertEquals(cur->getField(0,"testdecimal"),"1.10");
	//assertEquals(cur->getField(0,"testreal"),"1.1");
	//assertEquals(cur->getField(0,"testdouble"),"1.1");
	assertEquals(cur->getField(0,"testchar"),"testchar1                               ");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testdate"),"2001-01-01");
	assertEquals(cur->getField(0,"testtime"),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testsmallint"),"8");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testbigint"),"8");
	assertEquals(cur->getField(7,"testdecimal"),"8.80");
	//assertEquals(cur->getField(7,"testreal"),"8.8");
	//assertEquals(cur->getField(7,"testdouble"),"8.8");
	assertEquals(cur->getField(7,"testchar"),"testchar8                               ");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testdate"),"2008-01-01");
	assertEquals(cur->getField(7,"testtime"),"08:00:00");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testsmallint"),1);
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testbigint"),1);
	assertEquals(cur->getFieldLength(0,"testdecimal"),4);
	//assertEquals(cur->getFieldLength(0,"testreal"),3);
	//assertEquals(cur->getFieldLength(0,"testdouble"),3);
	assertEquals(cur->getFieldLength(0,"testchar"),40);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testdate"),10);
	assertEquals(cur->getFieldLength(0,"testtime"),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testsmallint"),1);
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testbigint"),1);
	assertEquals(cur->getFieldLength(7,"testdecimal"),4);
	//assertEquals(cur->getFieldLength(7,"testreal"),3);
	//assertEquals(cur->getFieldLength(7,"testdouble"),3);
	assertEquals(cur->getFieldLength(7,"testchar"),40);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testdate"),10);
	assertEquals(cur->getFieldLength(7,"testtime"),8);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1");
	assertEquals(fields[2],"1");
	assertEquals(fields[3],"1.10");
	//assertEquals(fields[4],"1.1");
	//assertEquals(fields[5],"1.1");
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
	//assertEquals(fieldlens[4],3);
	//assertEquals(fieldlens[5],3);
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
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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
						"db2inst1","testpassword",0,1);
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
		"	10, "
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01/01/2010', "
		"	'10:00:00', "
		"	NULL, "
		"	'testclob10', "
		"	blob('testblob10'))"));
	assertTrue(con->rollback());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	assertTrue(con->autoCommitOn());
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10, "
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01/01/2010', "
		"	'10:00:00', "
		"	NULL, "
		"	'testclob10', "
		"	blob('testblob10'))"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(con->autoCommitOff());
	cur->sendQuery("drop table testtable");
	assertTrue(con->commit());
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
		"	testclob1 clob(1K), "
		"	testclob2 clob(1K), "
		"	testblob1 blob(1K), "
		"	testblob2 blob(1K))"));
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
	cur->sendQuery("select * from testtable");
	assertEquals(cur->getField(0,(uint32_t)0),NULL);
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	assertEquals(cur->getField(0,3),NULL);
	cur->sendQuery("drop table testtable");
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// long lobs
	// fails for no obvious reason
#if 0
	stdoutput.printf("LONG LOBS: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery("create table testtable (testclob clob(25K))");
	assertTrue(con->commit());
	cur->prepareQuery("insert into testtable values (?)");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testclob from testtable");
	assertEquals(cur->getFieldLength(0,"testclob"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,"testclob"),largebuffer);
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(con->commit());
	stdoutput.printf("\n");
#endif


	// output bind by position
	stdoutput.printf("OUTPUT BIND BY POSITION: \n");
	cur->sendQuery("drop procedure testproc");
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
	assertEquals(cur->getOutputBindString("5"),NULL);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// output bind by name
	// FIXME: ...


	// output bind by name with validation
	// FIXME: ...


	// lob output bind
	stdoutput.printf("LOB OUTPUT BIND: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery(
		"create table testtable ("
		"	testclob clob(1K), "
		"	testblob blob(1K))");
	assertTrue(con->commit());
	cur->prepareQuery("insert into testtable values ('hello',?)");
	cur->inputBindBlob("1","hello",5);
	assertTrue(cur->executeQuery());
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	out out1 clob(1K), "
		"	out out2 blob(1K)) "
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
	cur->sendQuery("drop table testtable");
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// long output bind
	stdoutput.printf("LONG OUTPUT BIND: \n");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in in1 clob(25K), "
		"	out out1 clob(25K)) "
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
	stdoutput.printf("NEGATIVE INPUT BIND\n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery("create table testtable (testval integer)");
	assertTrue(con->commit());
	cur->prepareQuery("insert into testtable values (?)");
	cur->inputBind("1",-1);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testval from testtable");
	assertEquals(cur->getField(0,"TESTVAL"),"-1");
	cur->sendQuery("drop table testtable");
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// bind validation
	// FIXME: ...


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
	cur->inputBind("2",1.1,2,1);
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
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertTrue(cur->sendQuery("drop function testfunc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// stored procedure returning multiple values
	stdoutput.printf("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(con->commit());
	/*assertEquals(cur->sendQuery(
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20), "
		"	in in4 clob(1K), "
		"	in in5 blob(1K), "
		"	out out1 int, "
		"	out out2 double, "
		"	out out3 varchar(20), "
		"	out out4 clob(1K), "
		"	out out5 blob(1K)) "
		"language sql "
		"begin "
		"	set out1 = in1; "
		"	set out2 = in2; "
		"	set out3 = in3; "
		"	set out4 = in4; "
		"	set out5 = in5; "
		"end"),1);
	cur->prepareQuery(
		"call testproc(?,?,?,?,blob(cast(? "
		"	as char(4))),?,?,?,?,?)");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	cur->inputBindClob("4","clob",4);
	cur->inputBind("5","blob");
	cur->defineOutputBindInteger("6");
	cur->defineOutputBindDouble("7");
	cur->defineOutputBindString("8",20);
	cur->defineOutputBindClob("9");
	cur->defineOutputBindBlob("10");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("6"),1);
	assertEquals(cur->getOutputBindDouble("7"),1.1);
	assertEquals(cur->getOutputBindString("8"),"hello");
	assertEquals(cur->getOutputBindClob("9"),"clob");
	assertEquals(cur->getOutputBindBlob("10"),"blob");
	assertTrue(cur->sendQuery("drop procedure testproc"));*/
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20), "
		"	in in4 clob(1K), "
		"	out out1 int, "
		"	out out2 double, "
		"	out out3 varchar(20), "
		"	out out4 clob(1K)) "
		"language sql "
		"begin "
		"	set out1 = in1; "
		"	set out2 = in2; "
		"	set out3 = in3; "
		"	set out4 = in4; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("call testproc(?,?,?,?,?,?,?,?)");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	cur->inputBindClob("4","clob",4);
	cur->defineOutputBindInteger("5");
	cur->defineOutputBindDouble("6");
	cur->defineOutputBindString("7",20);
	cur->defineOutputBindClob("8");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("5"),1);
	assertEquals(cur->getOutputBindDouble("6"),1.1);
	assertEquals(cur->getOutputBindString("7"),"hello");
	assertEquals(cur->getOutputBindClob("8"),"clob");
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// stored procedure returning result set
	stdoutput.printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(cur->sendQuery(
		"create procedure testproc() "
		"result set 1 "
		"language sql "
		"begin "
		"	declare c1 cursor with return for "
		"               select 1 from sysibm.sysdummy1 "
		"               union "
		"               select 2 from sysibm.sysdummy1 "
		"               union "
		"               select 3 from sysibm.sysdummy1 "
		"               union "
		"               select 4 from sysibm.sysdummy1 "
		"               union "
		"               select 5 from sysibm.sysdummy1 "
		"               union "
		"               select 6 from sysibm.sysdummy1 "
		"               union "
		"               select 7 from sysibm.sysdummy1 "
		"               union "
		"               select 8 from sysibm.sysdummy1; "
		"	open c1; "
		"end"));
	assertTrue(con->commit());
	assertTrue(cur->sendQuery("call testproc()"));
	assertEquals(cur->rowCount(),8);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// direct transactsql
	// FIXME: ...

	// temporary tables
	// the declare below fails with:
	// requires a user temporary table space with a page size
	// of at least 4096 that the user is authorized to use
#if 0
	stdoutput.printf("TEMPORARY TABLES: \n");
	cur->sendQuery("drop table session.temptable");
	assertTrue(cur->sendQuery(
		"declare global temporary table temptable "
		"(col1 int) not logged"));
	assertTrue(cur->sendQuery(
		"insert into session.temptable values (1)"));
	assertTrue(cur->sendQuery(
		"select count(*) from session.temptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery(
		"select count(*) from session.temptable"));
	stdoutput.printf("\n");
#endif


	// database is schema
	// FIXME: ...


	// catalog list
	// FIXME: ...


	// schema list
	// FIXME: ...


	// table type list
	// FIXME: ...


	// table list
	// FIXME: ...


	// type info list
	// FIXME: ...


	// column list
	// FIXME: ...


	// column list - auto_increment, primary key
	// FIXME: ...


	// primary keys list
	// FIXME: ...


	// key and index list
	// FIXME: ...


	// procedure list
	// FIXME: ...


	// procedure parameter list
	// FIXME: ...


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
