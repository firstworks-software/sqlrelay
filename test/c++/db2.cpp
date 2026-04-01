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
				"testclob4",NULL};
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
	uint64_t	counter=0;

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


	// create temptable
	stdoutput.printf("CREATE TEMPTABLE: \n");
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
	cur->inputBind("9",2002,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,2,0,0,0,NULL,false);
	cur->inputBindClob("11","testclob2",9);
	cur->inputBindBlob("12","testblob2",9);
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
	cur->inputBind("9",2003,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("10",-1,-1,-1,3,0,0,0,NULL,false);
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


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),1);
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
	assertEquals(cur->getField(0,"TESTSMALLINT"),"1");
	assertEquals(cur->getField(0,"TESTINT"),"1");
	assertEquals(cur->getField(0,"TESTBIGINT"),"1");
	assertEquals(cur->getField(0,"TESTDECIMAL"),"1.10");
	//assertEquals(cur->getField(0,"TESTREAL"),"1.1");
	//assertEquals(cur->getField(0,"TESTDOUBLE"),"1.1");
	assertEquals(cur->getField(0,"TESTCHAR"),"testchar1                               ");
	assertEquals(cur->getField(0,"TESTVARCHAR"),"testvarchar1");
	assertEquals(cur->getField(0,"TESTDATE"),"2001-01-01");
	assertEquals(cur->getField(0,"TESTTIME"),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"TESTSMALLINT"),"8");
	assertEquals(cur->getField(7,"TESTINT"),"8");
	assertEquals(cur->getField(7,"TESTBIGINT"),"8");
	assertEquals(cur->getField(7,"TESTDECIMAL"),"8.80");
	//assertEquals(cur->getField(7,"TESTREAL"),"8.8");
	//assertEquals(cur->getField(7,"TESTDOUBLE"),"8.8");
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
	//assertEquals(cur->getFieldLength(0,"TESTREAL"),3);
	//assertEquals(cur->getFieldLength(0,"TESTDOUBLE"),3);
	assertEquals(cur->getFieldLength(0,"TESTCHAR"),40);
	assertEquals(cur->getFieldLength(0,"TESTVARCHAR"),12);
	assertEquals(cur->getFieldLength(0,"TESTDATE"),10);
	assertEquals(cur->getFieldLength(0,"TESTTIME"),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"TESTSMALLINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTBIGINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTDECIMAL"),4);
	//assertEquals(cur->getFieldLength(7,"TESTREAL"),3);
	//assertEquals(cur->getFieldLength(7,"TESTDOUBLE"),3);
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


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("values ($(var1),'$(var2)','$(var3)')");
	cur->substitution("var1",1);
	cur->substitution("var2","hello");
	cur->substitution("var3",10.5556,6,4);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("values ('$(var1)','$(var2)','$(var3)')");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("values ($(var1),$(var2),$(var3))");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3");
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("values ($(var1),$(var2),$(var3))");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
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
	assertTrue(cur->resumeResultSet(id));
	assertEquals(cur->getField(4,(uint32_t)0),NULL);
	assertEquals(cur->getField(5,(uint32_t)0),NULL);
	assertEquals(cur->getField(6,(uint32_t)0),NULL);
	assertEquals(cur->getField(7,(uint32_t)0),NULL);
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
	assertTrue(con->autoCommitOff());
	cur->sendQuery("drop table testtable");
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// stored procedure
	stdoutput.printf("STORED PROCEDURE: \n");
	// return multiple values
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
	cur->inputBind("2",1.1,2,1);
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
	assertEquals(cur->getOutputBindDouble("7"),1.1);
	assertEquals(cur->getOutputBindString("8"),"hello");
	assertEquals(cur->getOutputBindClob("9"),"clob");
	assertEquals(cur->getOutputBindBlob("10"),"blob");
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
	stdoutput.printf("\n");


	// stored procedure returning result set
	stdoutput.printf("STORED PROCEDURE RETURNING RESULT SET: \n");
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


	// long blob
	stdoutput.printf("LONG BLOB: \n");
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery("create table testtable1 (testclob clob)"));
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in in1 clob, "
		"	out out1 clob) "
		"language sql "
		"begin "
		"	set out1 = in1; "
		"end"));
	assertTrue(con->commit());
	cur->prepareQuery("insert into testtable1 values (?)");
	char	clobval[20*1024+1];
	for (int i=0; i<20*1024; i++) {
		clobval[i]='C';
	}
	clobval[20*1024]='\0';
	cur->inputBindClob("1",clobval,20*1024);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testclob from testtable1");
	assertEquals(cur->getFieldLength(0,"TESTCLOB"),20*1024);
	assertEquals(cur->getField(0,"TESTCLOB"),clobval);
	cur->prepareQuery("call testproc(?,?)");
	cur->inputBindClob("1",clobval,20*1024);
	cur->defineOutputBindClob("2");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("2"),20*1024);
	assertEquals(cur->getOutputBindClob("2"),clobval);
	assertTrue(cur->sendQuery("drop table testtable1"));
	assertTrue(cur->sendQuery("drop procedure testproc"));
	assertTrue(con->commit());
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
		if (!charstring::compare(
				cur->getField(i,"Database"),"DB2INST1")) {
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
	found=false;
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
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");
	cur->sendQuery("drop table testtable3");
	cur->sendQuery("drop table testtable4");
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
	cur->sendQuery("drop table testtable");
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
	assertTrue(charstring::contains(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::contains(
			cur->getField(0,"column_key"),"PRI"));
	assertFalse(charstring::contains(
			cur->getField(1,"extra"),"auto_increment"));
	assertFalse(charstring::contains(
			cur->getField(1,"column_key"),"PRI"));
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int not null primary key, "
		"	col2 int)"));
	assertTrue(con->commit());
	assertTrue(cur->getColumnList("testtable",NULL));
	assertFalse(charstring::contains(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::contains(
			cur->getField(0,"column_key"),"PRI"));
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
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	cur->sendQuery("drop table testtable");
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
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	cur->sendQuery("drop table testtable");
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
	cur->sendQuery("drop procedure testproc1");
	cur->sendQuery("drop procedure testproc2");
	cur->sendQuery("drop procedure testproc3");
	cur->sendQuery("drop procedure testproc4");
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
