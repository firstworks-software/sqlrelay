// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con;
sqlrcursor	*cur;
sqlrconnection	*secondcon;
sqlrcursor	*secondcur;

int main(int argc, char **argv) {

	const char	*isolationlevels[]={
				"read committed","read uncommitted",
				"repeatable read","serializable",NULL};
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
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"postgresql");
	stdoutput.printf("\n");


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// bind format
	stdoutput.printf("BIND FORMAT: \n");
	assertEquals(con->bindFormat(),"$1");
	stdoutput.printf("\n");


	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"nextval('%s')");
	stdoutput.printf("\n");

	// isolation levels
	/*stdoutput.printf("ISOLATION LEVELS: \n");
	for (const char **il=isolationlevels; *il; il++) {
		// postgresql requires the isolation level to
		// be the first query of the transaction
		con->begin();
		assertTrue(con->setIsolationLevel(*il));
		assertEquals(con->getIsolationLevel(),*il);
		con->commit();
		stdoutput.printf("\n");
	}
	// reset to the default isolation level
	con->begin();
	assertTrue(con->setIsolationLevel(isolationlevels[0]));
	con->commit();
	stdoutput.printf("\n");*/

	// drop existing table
	cur->sendQuery("drop table testtable");


	// create temptable
	stdoutput.printf("CREATE TEMPTABLE: \n");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testreal real, "
		"	testsmallint smallint, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testtext text, "
		"	testbytea bytea)"));
	stdoutput.printf("\n");


	// begin transction
	stdoutput.printf("BEGIN TRANSCTION: \n");
	assertTrue(cur->sendQuery("begin"));
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01/01/2001', "
		"	'01:00:00', "
		"	NULL, "
		"	'testtext1', "
		"	'testbytea1')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2.2, "
		"	2.2, "
		"	2, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'01/01/2002', "
		"	'02:00:00', "
		"	NULL, "
		"	'testtext2', "
		"	'testbytea2')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3.3, "
		"	3.3, "
		"	3, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'01/01/2003', "
		"	'03:00:00', "
		"	NULL, "
		"	'testtext3', "
		"	'testbytea3')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4.4, "
		"	4.4, "
		"	4, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'01/01/2004', "
		"	'04:00:00', "
		"	NULL, "
		"	'testtext4', "
		"	'testbytea4')"));
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
		"	$1, "
		"	$2, "
		"	$3, "
		"	$4, "
		"	$5, "
		"	$6, "
		"	$7, "
		"	$8, "
		"	NULL, "
		"	$9, "
		"	$10)");
	assertEquals(cur->countBindVariables(),10);
	cur->inputBind("1",5);
	cur->inputBind("2",5.5,4,2);
	cur->inputBind("3",5.5,4,2);
	cur->inputBind("4",5);
	cur->inputBind("5","testchar5");
	cur->inputBind("6","testvarchar5");
	cur->inputBind("7","01/01/2005");
	cur->inputBind("8","05:00:00");
	cur->inputBindClob("9","testtext5",9);
	cur->inputBindBlob("10","testbytea5",10);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",6);
	cur->inputBind("2",6.6,4,2);
	cur->inputBind("3",6.6,4,2);
	cur->inputBind("4",6);
	cur->inputBind("5","testchar6");
	cur->inputBind("6","testvarchar6");
	cur->inputBind("7","01/01/2006");
	cur->inputBind("8","06:00:00");
	cur->inputBindClob("9","testtext6",9);
	cur->inputBindBlob("10","testbytea6",10);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",7);
	cur->inputBind("2",7.7,4,2);
	cur->inputBind("3",7.7,4,2);
	cur->inputBind("4",7);
	cur->inputBind("5","testchar7");
	cur->inputBind("6","testvarchar7");
	cur->inputBind("7","01/01/2007");
	cur->inputBind("8","07:00:00");
	cur->inputBindClob("9","testtext7",9);
	cur->inputBindBlob("10","testbytea8",10);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// bind by position with validation
	stdoutput.printf("BIND BY POSITION WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("1",8);
	cur->inputBind("2",8.8,4,2);
	cur->inputBind("3",8.8,4,2);
	cur->inputBind("4",8);
	cur->inputBind("5","testchar8");
	cur->inputBind("6","testvarchar8");
	cur->inputBind("7","01/01/2008");
	cur->inputBind("8","08:00:00");
	cur->inputBindClob("9","testtext8",9);
	cur->inputBindClob("10","testbytea8",10);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),11);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testreal");
	assertEquals(cur->getColumnName(3),"testsmallint");
	assertEquals(cur->getColumnName(4),"testchar");
	assertEquals(cur->getColumnName(5),"testvarchar");
	assertEquals(cur->getColumnName(6),"testdate");
	assertEquals(cur->getColumnName(7),"testtime");
	assertEquals(cur->getColumnName(8),"testtimestamp");
	assertEquals(cur->getColumnName(9),"testtext");
	assertEquals(cur->getColumnName(10),"testbytea");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testreal");
	assertEquals(cols[3],"testsmallint");
	assertEquals(cols[4],"testchar");
	assertEquals(cols[5],"testvarchar");
	assertEquals(cols[6],"testdate");
	assertEquals(cols[7],"testtime");
	assertEquals(cols[8],"testtimestamp");
	assertEquals(cols[9],"testtext");
	assertEquals(cols[10],"testbytea");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"int4");
	assertEquals(cur->getColumnType("testint"),"int4");
	assertEquals(cur->getColumnType(1),"float8");
	assertEquals(cur->getColumnType("testfloat"),"float8");
	assertEquals(cur->getColumnType(2),"float4");
	assertEquals(cur->getColumnType("testreal"),"float4");
	assertEquals(cur->getColumnType(3),"int2");
	assertEquals(cur->getColumnType("testsmallint"),"int2");
	assertEquals(cur->getColumnType(4),"bpchar");
	assertEquals(cur->getColumnType("testchar"),"bpchar");
	assertEquals(cur->getColumnType(5),"varchar");
	assertEquals(cur->getColumnType("testvarchar"),"varchar");
	assertEquals(cur->getColumnType(6),"date");
	assertEquals(cur->getColumnType("testdate"),"date");
	assertEquals(cur->getColumnType(7),"time");
	assertEquals(cur->getColumnType("testtime"),"time");
	assertEquals(cur->getColumnType(8),"timestamp");
	assertEquals(cur->getColumnType("testtimestamp"),"timestamp");
	assertEquals(cur->getColumnType(9),"text");
	assertEquals(cur->getColumnType("testtext"),"text");
	assertEquals(cur->getColumnType(10),"bytea");
	assertEquals(cur->getColumnType("testbytea"),"bytea");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnLength("testint"),4);
	assertEquals(cur->getColumnLength(1),8);
	assertEquals(cur->getColumnLength("testfloat"),8);
	assertEquals(cur->getColumnLength(2),4);
	assertEquals(cur->getColumnLength("testreal"),4);
	assertEquals(cur->getColumnLength(3),2);
	assertEquals(cur->getColumnLength("testsmallint"),2);
	assertEquals(cur->getColumnLength(4),44);
	assertEquals(cur->getColumnLength("testchar"),44);
	assertEquals(cur->getColumnLength(5),44);
	assertEquals(cur->getColumnLength("testvarchar"),44);
	assertEquals(cur->getColumnLength(6),4);
	assertEquals(cur->getColumnLength("testdate"),4);
	assertEquals(cur->getColumnLength(7),8);
	assertEquals(cur->getColumnLength("testtime"),8);
	assertEquals(cur->getColumnLength(8),8);
	assertEquals(cur->getColumnLength("testtimestamp"),8);
	assertEquals(cur->getColumnLength(9),0);
	assertEquals(cur->getColumnLength("testtext"),0);
	assertEquals(cur->getColumnLength(10),0);
	assertEquals(cur->getColumnLength("testbytea"),0);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest(1),3);
	assertEquals(cur->getLongest("testfloat"),3);
	assertEquals(cur->getLongest(2),3);
	assertEquals(cur->getLongest("testreal"),3);
	assertEquals(cur->getLongest(3),1);
	assertEquals(cur->getLongest("testsmallint"),1);
	assertEquals(cur->getLongest(4),40);
	assertEquals(cur->getLongest("testchar"),40);
	assertEquals(cur->getLongest(5),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(6),10);
	assertEquals(cur->getLongest("testdate"),10);
	assertEquals(cur->getLongest(7),8);
	assertEquals(cur->getLongest("testtime"),8);
	assertEquals(cur->getLongest(9),9);
	assertEquals(cur->getLongest("testtext"),9);
	assertEquals(cur->getLongest(10),22);
	assertEquals(cur->getLongest("testbytea"),22);
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");

	/*stdoutput.printf("TOTAL ROWS: \n");
	assertEquals(cur->totalRows(),8);
	stdoutput.printf("\n");*/


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
	assertEquals(cur->getField(0,2),"1.1");
	assertEquals(cur->getField(0,3),"1");
	assertEquals(cur->getField(0,4),"testchar1                               ");
	assertEquals(cur->getField(0,5),"testvarchar1");
	assertEquals(cur->getField(0,6),"2001-01-01");
	assertEquals(cur->getField(0,7),"01:00:00");
	assertEquals(cur->getField(0,9),"testtext1");
	assertEquals(cur->getField(0,10),"\\x74657374627974656131");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8.8");
	assertEquals(cur->getField(7,2),"8.8");
	assertEquals(cur->getField(7,3),"8");
	assertEquals(cur->getField(7,4),"testchar8                               ");
	assertEquals(cur->getField(7,5),"testvarchar8");
	assertEquals(cur->getField(7,6),"2008-01-01");
	assertEquals(cur->getField(7,7),"08:00:00");
	assertEquals(cur->getField(7,9),"testtext8");
	assertEquals(cur->getField(7,10),"\\x74657374627974656138");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),3);
	assertEquals(cur->getFieldLength(0,2),3);
	assertEquals(cur->getFieldLength(0,3),1);
	assertEquals(cur->getFieldLength(0,4),40);
	assertEquals(cur->getFieldLength(0,5),12);
	assertEquals(cur->getFieldLength(0,6),10);
	assertEquals(cur->getFieldLength(0,7),8);
	assertEquals(cur->getFieldLength(0,9),9);
	assertEquals(cur->getFieldLength(0,10),22);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),3);
	assertEquals(cur->getFieldLength(7,2),3);
	assertEquals(cur->getFieldLength(7,3),1);
	assertEquals(cur->getFieldLength(7,4),40);
	assertEquals(cur->getFieldLength(7,5),12);
	assertEquals(cur->getFieldLength(7,6),10);
	assertEquals(cur->getFieldLength(7,7),8);
	assertEquals(cur->getFieldLength(7,9),9);
	assertEquals(cur->getFieldLength(7,10),22);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testfloat"),"1.1");
	assertEquals(cur->getField(0,"testreal"),"1.1");
	assertEquals(cur->getField(0,"testsmallint"),"1");
	assertEquals(cur->getField(0,"testchar"),"testchar1                               ");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testdate"),"2001-01-01");
	assertEquals(cur->getField(0,"testtime"),"01:00:00");
	assertEquals(cur->getField(0,"testtext"),"testtext1");
	assertEquals(cur->getField(0,"testbytea"),"\\x74657374627974656131");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testfloat"),"8.8");
	assertEquals(cur->getField(7,"testreal"),"8.8");
	assertEquals(cur->getField(7,"testsmallint"),"8");
	assertEquals(cur->getField(7,"testchar"),"testchar8                               ");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testdate"),"2008-01-01");
	assertEquals(cur->getField(7,"testtime"),"08:00:00");
	assertEquals(cur->getField(7,"testtext"),"testtext8");
	assertEquals(cur->getField(7,"testbytea"),"\\x74657374627974656138");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testfloat"),3);
	assertEquals(cur->getFieldLength(0,"testreal"),3);
	assertEquals(cur->getFieldLength(0,"testsmallint"),1);
	assertEquals(cur->getFieldLength(0,"testchar"),40);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testdate"),10);
	assertEquals(cur->getFieldLength(0,"testtime"),8);
	assertEquals(cur->getFieldLength(0,"testtext"),9);
	assertEquals(cur->getFieldLength(0,"testbytea"),22);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testfloat"),3);
	assertEquals(cur->getFieldLength(7,"testreal"),3);
	assertEquals(cur->getFieldLength(7,"testsmallint"),1);
	assertEquals(cur->getFieldLength(7,"testchar"),40);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testdate"),10);
	assertEquals(cur->getFieldLength(7,"testtime"),8);
	assertEquals(cur->getFieldLength(7,"testtext"),9);
	assertEquals(cur->getFieldLength(7,"testbytea"),22);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1.1");
	assertEquals(fields[2],"1.1");
	assertEquals(fields[3],"1");
	assertEquals(fields[4],"testchar1                               ");
	assertEquals(fields[5],"testvarchar1");
	assertEquals(fields[6],"2001-01-01");
	assertEquals(fields[7],"01:00:00");
	assertEquals(fields[9],"testtext1");
	assertEquals(fields[10],"\\x74657374627974656131");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],3);
	assertEquals(fieldlens[2],3);
	assertEquals(fieldlens[3],1);
	assertEquals(fieldlens[4],40);
	assertEquals(fieldlens[5],12);
	assertEquals(fieldlens[6],10);
	assertEquals(fieldlens[7],8);
	assertEquals(fieldlens[9],9);
	assertEquals(fieldlens[10],22);
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),'$(var2)',$(var3)");
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
	cur->prepareQuery("select $(var1),$(var2),$(var3)");
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
	cur->prepareQuery("select '$(var1)','$(var2)','$(var3)'");
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
	cur->prepareQuery("select $(var1),$(var2),$(var3)");
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
	stdoutput.printf("NULLS as Nulls: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery("select NULL,1,NULL"));
	assertEquals(cur->getField(0,(uint32_t)0),NULL);
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select NULL,1,NULL"));
	assertEquals(cur->getField(0,(uint32_t)0),"");
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),"");
	cur->getNullsAsNulls();
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
	assertEquals(cur->getColumnName((uint32_t)0),NULL);
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),NULL);
	cur->getColumnInfo();
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getColumnName((uint32_t)0),"testint");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnType((uint32_t)0),"int4");
	stdoutput.printf("\n");


	// suspended session
	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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
	assertEquals(cur->colCount(),11);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName((uint32_t)0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testreal");
	assertEquals(cur->getColumnName(3),"testsmallint");
	assertEquals(cur->getColumnName(4),"testchar");
	assertEquals(cur->getColumnName(5),"testvarchar");
	assertEquals(cur->getColumnName(6),"testdate");
	assertEquals(cur->getColumnName(7),"testtime");
	assertEquals(cur->getColumnName(8),"testtimestamp");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testreal");
	assertEquals(cols[3],"testsmallint");
	assertEquals(cols[4],"testchar");
	assertEquals(cols[5],"testvarchar");
	assertEquals(cols[6],"testdate");
	assertEquals(cols[7],"testtime");
	assertEquals(cols[8],"testtimestamp");
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


	// commit
	stdoutput.printf("COMMIT: \n");
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	assertTrue(con->commit());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	//assertEquals(con->autoCommitOn(),1);
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01/01/2010', "
		"	'10:00:00', "
		"	NULL)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	//assertEquals(con->autoCommitOff(),1);
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


	// temporary tables
	stdoutput.printf("TEMPORARY TABLES: \n");
	cur->sendQuery("drop table temptable\n");
	cur->sendQuery("-- this should be skipped\n"
			"-- so should this\n"
			"-- and the whitespace below too\n"
			"        				\n\n"
			"create temporary table temptable (col1 int)");
	assertTrue(cur->sendQuery("insert into temptable values (1)"));
	assertTrue(cur->sendQuery("select count(*) from temptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("select count(*) from temptable"));
	cur->sendQuery("drop table temptable\n");
	stdoutput.printf("\n");


	// stored procedures
	stdoutput.printf("STORED PROCEDURES: \n");
	// return no values
	cur->sendQuery("drop function testfunc(int,float,char(20))");
	assertTrue(cur->sendQuery(
		"create function testfunc("
		"	int,float,char(20)) "
		"returns void as ' "
		"	declare in1 int; "
		"	in2 float; "
		"	in3 char(20); "
		"begin "
		"	in1:=$1; "
		"	in2:=$2; "
		"	in3:=$3; "
		"	return; "
		"end;' language plpgsql"));
	cur->prepareQuery("select testfunc($1,$2,$3)");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,4,2);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	cur->sendQuery("drop function testfunc(int,float,char(20))");
	stdoutput.printf("\n");
	// return single value
	cur->sendQuery("drop function testfunc(int,float,char(20))");
	assertTrue(cur->sendQuery(
		"create function testfunc(int,float,char(20)) returns int as "
		"	' begin return $1; end;' language plpgsql"));
	cur->prepareQuery("select * from testfunc($1,$2,$3)");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,4,2);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	cur->sendQuery("drop function testfunc(int,float,char(20))");
	stdoutput.printf("\n");
	// return multiple values
	cur->sendQuery("drop function testfunc(int,char(20))");
	assertTrue(cur->sendQuery(
		"create function testfunc("
		"	int,float,char(20)) "
		"returns record as ' "
		"	declare output record; "
		"begin "
		"	select $1,$2,$3 into output; "
		"	return output; "
		"end;' language plpgsql"));
	cur->prepareQuery(
		"select "
		"	* "
		"from "
		"	testfunc($1,$2,$3) "
		"	as (col1 int, "
		"		col2 float, "
		"		col3 bpchar) ");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,4,2);
	cur->inputBind("3","hello");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	//assertEquals(charstring::convertToFloatC(cur->getField(0,1)),1.1);
	assertEquals(cur->getField(0,2),"hello");
	cur->sendQuery("drop function testfunc(int,float,char(20))");
	stdoutput.printf("\n");
	// return result set
	cur->sendQuery("drop function testfunc()");
	assertTrue(cur->sendQuery(
		"create function testfunc() "
		"returns setof record as ' "
		"	declare output record; "
		"begin "
		"	for output in "
		"		select * from testtable "
		"	loop "
		"		return next output; "
		"	end loop; "
		"	return; "
		"end;' language plpgsql"));
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testfunc() "
		"	as (testint int, "
		"		testfloat float, "
		"		testreal real, "
		"		testsmallint smallint, "
		"		testchar char(40), "
		"		testvarchar varchar(40), "
		"		testdate date, "
		"		testtime time, "
		"		testtimestamp timestamp, "
		"		testtext text, "
		"		testbytea bytea) "));
	assertEquals(cur->getField(4,(uint32_t)0),"5");
	assertEquals(cur->getField(5,(uint32_t)0),"6");
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	cur->sendQuery("drop function testfunc()");
	stdoutput.printf("\n");

	// drop existing table
	cur->sendQuery("drop table testtable");


	// database is schema
	stdoutput.printf("DATABASE IS SCHEMA: \n");
	assertFalse(con->getDatabaseIsSchema());
	stdoutput.printf("\n");


	// catalog list
	stdoutput.printf("CATALOG LIST: \n");
	assertTrue(cur->getCatalogList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	assertTrue(cur->rowCount()>0);
	stdoutput.printf("\n");


	// schema list
	stdoutput.printf("SCHEMA LIST: \n");
	assertTrue(cur->getSchemaList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	assertTrue(cur->rowCount()>0);
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
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");
	cur->sendQuery("drop table testtable3");
	cur->sendQuery("drop table testtable4");
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
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");
	cur->sendQuery("drop table testtable3");
	cur->sendQuery("drop table testtable4");
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
	assertEquals(cur->getField(0,"precision"),"255");
	assertEquals(cur->getField(0,"local_type_name"),"CHAR");
	assertTrue(cur->getTypeInfoList("varchar"));
	assertEquals(cur->getField(0,"type_name"),"VARCHAR");
	assertEquals(cur->getField(0,"data_type"),"12");
	assertEquals(cur->getField(0,"precision"),"255");
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
		"	testint int, "
		"	testfloat float, "
		"	testreal real, "
		"	testsmallint smallint, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testtext text, "
		"	testbytea bytea)"));
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
	assertEquals(cur->getField(2,"column_name"),"testreal");
	assertEquals(cur->getField(3,"column_name"),"testsmallint");
	assertEquals(cur->getField(4,"column_name"),"testchar");
	assertEquals(cur->getField(5,"column_name"),"testvarchar");
	assertEquals(cur->getField(6,"column_name"),"testdate");
	assertEquals(cur->getField(7,"column_name"),"testtime");
	assertEquals(cur->getField(8,"column_name"),"testtimestamp");
	assertEquals(cur->getField(9,"column_name"),"testtext");
	assertEquals(cur->getField(10,"column_name"),"testbytea");
	assertEquals(cur->getField(0,"data_type"),"integer");
	assertEquals(cur->getField(1,"data_type"),"double precision");
	assertEquals(cur->getField(2,"data_type"),"real");
	assertEquals(cur->getField(3,"data_type"),"smallint");
	assertEquals(cur->getField(4,"data_type"),"character");
	assertEquals(cur->getField(5,"data_type"),"character varying");
	assertEquals(cur->getField(6,"data_type"),"date");
	assertEquals(cur->getField(7,"data_type"),"time without time zone");
	assertEquals(cur->getField(8,"data_type"),"timestamp without time zone");
	assertEquals(cur->getField(9,"data_type"),"text");
	assertEquals(cur->getField(10,"data_type"),"bytea");
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	cur->sendQuery("drop table if exists testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 serial primary key, "
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
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertFalse(charstring::contains(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::contains(
			cur->getField(0,"column_key"),"PRI"));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	cur->sendQuery("drop table testtable");
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
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	cur->sendQuery("drop table testtable");
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
	assertEquals(cur->getField(0,"non_unique"),"f");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"col1"));
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"3");
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	cur->sendQuery("drop function testproc1(int,char,varchar,date)");
	cur->sendQuery("drop function testproc2(int,char,varchar,date)");
	cur->sendQuery("drop function testproc3(int,char,varchar,date)");
	cur->sendQuery("drop function testproc4(int,char,varchar,date)");
	assertTrue(cur->sendQuery(
		"create function testproc1("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"returns void "
		"as 'begin end;' "
		"language plpgsql"));
	assertTrue(cur->sendQuery(
		"create function testproc2("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"returns void "
		"as 'begin end;' "
		"language plpgsql"));
	assertTrue(cur->sendQuery(
		"create function testproc3("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"returns void "
		"as 'begin end;' "
		"language plpgsql"));
	assertTrue(cur->sendQuery(
		"create function testproc4("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"returns void "
		"as 'begin end;' "
		"language plpgsql"));
	assertTrue(cur->getProcedureList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"routine_name");
		if (!charstring::compare(name,"testproc1") ||
			!charstring::compare(name,"testproc2") ||
			!charstring::compare(name,"testproc3") ||
			!charstring::compare(name,"testproc4")) {
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
	assertEquals(cur->getField(0,"parameter_name"),"in1");
	assertEquals(cur->getField(0,"parameter_mode"),"1");
	assertEquals(cur->getField(0,"data_type"),"integer");
	assertEquals(cur->getField(0,"ordinal_position"),"1");
	assertEquals(cur->getField(1,"parameter_name"),"in2");
	assertEquals(cur->getField(1,"parameter_mode"),"1");
	assertEquals(cur->getField(1,"data_type"),"character");
	assertEquals(cur->getField(1,"ordinal_position"),"2");
	assertEquals(cur->getField(2,"parameter_name"),"in3");
	assertEquals(cur->getField(2,"parameter_mode"),"1");
	assertEquals(cur->getField(2,"data_type"),"character varying");
	assertEquals(cur->getField(2,"ordinal_position"),"3");
	assertEquals(cur->getField(3,"parameter_name"),"in4");
	assertEquals(cur->getField(3,"parameter_mode"),"1");
	assertEquals(cur->getField(3,"data_type"),"date");
	assertEquals(cur->getField(3,"ordinal_position"),"4");
	cur->sendQuery("drop function testproc1(int,char,varchar,date)");
	cur->sendQuery("drop function testproc2(int,char,varchar,date)");
	cur->sendQuery("drop function testproc3(int,char,varchar,date)");
	cur->sendQuery("drop function testproc4(int,char,varchar,date)");
	stdoutput.printf("\n");


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery("select * from testtable order by testint"));
	assertFalse(cur->sendQuery("select * from testtable order by testint"));
	assertFalse(cur->sendQuery("select * from testtable order by testint"));
	assertFalse(cur->sendQuery("select * from testtable order by testint"));
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
