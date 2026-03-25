// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con;
sqlrcursor	*cur;
sqlrconnection	*secondcon;
sqlrcursor	*secondcur;

int main(int argc, char **argv) {

	const char	*bindvars[]={"1","2","3","4","5","6",
				"7","8","9","10","11","12",NULL};
	const char	*bindvals[]={"4","4","4.4","4.4","4.4","4.4",
				"01-JAN-2004","04:00:00",
				"testchar4","testvarchar4",NULL,"testblob4"};
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
	assertEquals(con->identify(),"firebird");
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
	assertEquals(con->nextvalFormat(),"next value for %s");
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	// though firebird does support a "set transaction ..." statement to
	// set the isolation level, it looks like, in firebird, you can really
	// only set it through the TPB at the start of a transaction, so
	// attempts to set it shoud fail
	assertFalse(con->setIsolationLevel("read committed"));
	assertEquals(con->getIsolationLevel(),"read committed");
	stdoutput.printf("\n");

	// clean up table
	cur->sendQuery("delete from testtable");
	con->commit();


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	'01-JAN-2001', "
		"	'01:00:00', "
		"	'testchar1', "
		"	'testvarchar1', "
		"	NULL, "
		"	'testblob1')"));
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
		"	?, "
		"	?)");
	assertEquals(cur->countBindVariables(),12);
	cur->inputBind("1",2);
	cur->inputBind("2",2);
	cur->inputBind("3",2.2,2,1);
	cur->inputBind("4",2.2,2,1);
	cur->inputBind("5",2.2,2,1);
	cur->inputBind("6",2.2,2,1);
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
	cur->inputBind("3",3.3,2,1);
	cur->inputBind("4",3.3,2,1);
	cur->inputBind("5",3.3,2,1);
	cur->inputBind("6",3.3,2,1);
	cur->inputBind("7",2003,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,3,0,0,0,NULL,false);
	cur->inputBind("9","testchar3");
	cur->inputBind("10","testvarchar3");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("13","testblob3",9);
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
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	'01-JAN-2005', "
		"	'05:00:00', "
		"	'testchar5', "
		"	'testvarchar5', "
		"	NULL, "
		"	'testblob5')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	6, "
		"	6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	'01-JAN-2006', "
		"	'06:00:00', "
		"	'testchar6', "
		"	'testvarchar6', "
		"	NULL, "
		"	'testblob6')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	7, "
		"	7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	'01-JAN-2007', "
		"	'07:00:00', "
		"	'testchar7', "
		"	'testvarchar7', "
		"	NULL, "
		"	'testblob7')"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	8, "
		"	8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	'01-JAN-2008', "
		"	'08:00:00', "
		"	'testchar8', "
		"	'testvarchar8', "
		"	NULL, "
		"	'testblob8')"));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),1);
	stdoutput.printf("\n");


	// stored procedure
	stdoutput.printf("STORED PROCEDURE: \n");
	cur->prepareQuery("select * from testproc(?,?,?,?)");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	cur->inputBindBlob("4","blob",4);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"1.1000");
	assertEquals(cur->getField(0,2),"hello");
	assertEquals(cur->getField(0,3),"blob");
	cur->prepareQuery("execute procedure testproc ?, ?, ?, ?");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	cur->inputBindBlob("4","blob",4);
	cur->defineOutputBindInteger("1");
	cur->defineOutputBindDouble("2");
	cur->defineOutputBindString("3",20);
	cur->defineOutputBindBlob("4");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("1"),1);
	//assertEquals(cur->getOutputBindDouble("2"),1.1);
	assertEquals(cur->getOutputBindString("3"),"hello               ");
	assertEquals(cur->getOutputBindBlob("4"),"blob");
	stdoutput.printf("\n");


	// long blob
	stdoutput.printf("LONG BLOB: \n");
	cur->sendQuery("delete from testtable1");
	cur->prepareQuery("insert into testtable1 values (?)");
	char	blobval[20*1024+1];
	for (int i=0; i<20*1024; i++) {
		blobval[i]='C';
	}
	blobval[20*1024]='\0';
	cur->inputBindClob("1",blobval,20*1024);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testblob from testtable1");
	assertEquals(cur->getFieldLength(0,"TESTBLOB"),20*1024);
	assertEquals(cur->getField(0,"TESTBLOB"),blobval);
	cur->prepareQuery("execute procedure testproc1 ?");
	cur->inputBindBlob("1",blobval,20*1024);
	cur->defineOutputBindBlob("1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("1"),20*1024);
	assertEquals(cur->getOutputBindBlob("1"),blobval);
	stdoutput.printf("\n");


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
	assertEquals(cur->getField(0,2),"1.10");
	assertEquals(cur->getField(0,3),"1.10");
	assertEquals(cur->getField(0,4),"1.1000");
	assertEquals(cur->getField(0,5),"1.1000");
	assertEquals(cur->getField(0,6),"2001:01:01");
	assertEquals(cur->getField(0,7),"01:00:00");
	assertEquals(cur->getField(0,8),"testchar1                                         ");
	assertEquals(cur->getField(0,9),"testvarchar1");
	assertEquals(cur->getField(0,11),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8.80");
	assertEquals(cur->getField(7,3),"8.80");
	assertEquals(cur->getField(7,4),"8.8000");
	assertEquals(cur->getField(7,5),"8.8000");
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
	assertEquals(cur->getField(0,"TESTDECIMAL"),"1.10");
	assertEquals(cur->getField(0,"TESTNUMERIC"),"1.10");
	assertEquals(cur->getField(0,"TESTFLOAT"),"1.1000");
	assertEquals(cur->getField(0,"TESTDOUBLE"),"1.1000");
	assertEquals(cur->getField(0,"TESTDATE"),"2001:01:01");
	assertEquals(cur->getField(0,"TESTTIME"),"01:00:00");
	assertEquals(cur->getField(0,"TESTCHAR"),"testchar1                                         ");
	assertEquals(cur->getField(0,"TESTVARCHAR"),"testvarchar1");
	assertEquals(cur->getField(0,"TESTBLOB"),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"TESTINTEGER"),"8");
	assertEquals(cur->getField(7,"TESTSMALLINT"),"8");
	assertEquals(cur->getField(7,"TESTDECIMAL"),"8.80");
	assertEquals(cur->getField(7,"TESTNUMERIC"),"8.80");
	assertEquals(cur->getField(7,"TESTFLOAT"),"8.8000");
	assertEquals(cur->getField(7,"TESTDOUBLE"),"8.8000");
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
	assertEquals(fields[2],"1.10");
	assertEquals(fields[3],"1.10");
	assertEquals(fields[4],"1.1000");
	assertEquals(fields[5],"1.1000");
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


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),'$(var2)',$(var3) from rdb$database");
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
	cur->prepareQuery(
		"select "
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	rdb$database ");
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
	cur->prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
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
	cur->prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
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
	assertTrue(cur->sendQuery("select 1,NULL,NULL from rdb$database"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select 1,NULL,NULL from rdb$database"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"");
	assertEquals(cur->getField(0,2),"");
	cur->getNullsAsNulls();
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
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	'01-JAN-2010', "
		"	'10:00:00', "
		"	'testchar10', "
		"	'testvarchar10', "
		"	NULL, "
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
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	'01-JAN-2010', "
		"	'10:00:00', "
		"	'testchar10', "
		"	'testvarchar10', "
		"	NULL, "
		"	NULL)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	assertTrue(con->autoCommitOff());
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
	assertTrue(cur->resumeResultSet(id));
	assertEquals(cur->getField(4,(uint32_t)0),NULL);
	assertEquals(cur->getField(5,(uint32_t)0),NULL);
	assertEquals(cur->getField(6,(uint32_t)0),NULL);
	assertEquals(cur->getField(7,(uint32_t)0),NULL);
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
        bool    found=false;
        for (uint64_t i=0; i<cur->rowCount(); i++) {
                if (!charstring::compare(
                                cur->getField(i,"Database"),"TESTUSER")) {
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
	assertTrue(cur->getTableList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"Tables_in_xxx");
		if (!charstring::compare(name,"TESTTABLE1") ||
			!charstring::compare(name,"TESTTABLE2") ||
			!charstring::compare(name,"TESTTABLE3")) {
			counter++;
		}
	}
	assertEquals(counter,3);
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
	assertTrue(charstring::contains(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::contains(
			cur->getField(0,"column_key"),"PRI"));
	assertFalse(charstring::contains(
			cur->getField(1,"extra"),"auto_increment"));
	assertFalse(charstring::contains(
			cur->getField(1,"column_key"),"PRI"));
	stdoutput.printf("\n");
	assertTrue(cur->getColumnList("testtable3",NULL));
	assertFalse(charstring::contains(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::contains(
			cur->getField(0,"column_key"),"PRI"));
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
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
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
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	assertTrue(cur->getProcedureList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"routine_name");
		if (!charstring::compare(name,"TESTPROC") ||
			!charstring::compare(name,"TESTPROC1")) {
			counter++;
		}
	}
	assertEquals(counter,2);
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
