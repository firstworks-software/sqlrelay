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

	const char	*isolationlevels[]={
				"committed read","dirty read",
				"cursor stability","repeatable read",NULL};
	const char	*bindvars[]={"1","2","3","4",
				"5","6","7","8","9","10",
				"11","12","13","14","15","16",NULL};
	const char	*bindvals[]={"t","4","4","4","4",
				"4.4","4.4","4.4","4.4",
				"testchar4","testnchar4",
				"testvarchar4","testnvarchar4",
				"testlvarchar4","01/01/2004",
				"2004-01-01 04:00:00",NULL};
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
	assertEquals(con->identify(),"informix");
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
	assertEquals(con->nextvalFormat(),"%s.nextval");
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	for (const char **il=isolationlevels; *il; il++) {
		// you can set the isolation level, but to get it, you have to
		// have permissions to read from sysmaster:syssqlcurses
		assertTrue(con->setIsolationLevel(*il));
		stdoutput.printf("\n");
	}
	// reset to the default isolation level
	assertTrue(con->setIsolationLevel(isolationlevels[0]));
	stdoutput.printf("\n");

	// drop existing table
	cur->sendQuery("drop table testtable");


	// create temptable
	stdoutput.printf("CREATE TEMPTABLE: \n");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testboolean boolean, "
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testint8 int8, "
		"	testdecimal decimal(10,2), "
		"	testmoney money, "
		"	testsmallfloat smallfloat, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testnchar nchar(40), "
		"	testvarchar varchar(40), "
		"	testnvarchar nvarchar(40), "
		"	testlvarchar lvarchar(40), "
		"	testdate date, "
		"	testdatetime datetime year to second, "
		"	testtext text, "
		"	testbyte byte)"));
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	't', "
		"	1, "
		"	1, "
		"	1, "
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	'testchar1', "
		"	'testnchar1', "
		"	'testvarchar1', "
		"	'testnvarchar1', "
		"	'testlvarchar1', "
		"	'01/01/2001', "
		"	'2001-01-01 01:00:00', "
		"	'testtext1', "
		"	null)"));
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?)");
	assertEquals(cur->countBindVariables(),18);
	cur->inputBind("1","t");
	cur->inputBind("2",2);
	cur->inputBind("3",2);
	cur->inputBind("4",2);
	cur->inputBind("5",2);
	cur->inputBind("6",2.2,4,2);
	cur->inputBind("7",2.2,4,2);
	cur->inputBind("8",2.2,4,2);
	cur->inputBind("9",2.2,4,2);
	cur->inputBind("10","testchar2");
	cur->inputBind("11","testnchar2");
	cur->inputBind("12","testvarchar2");
	cur->inputBind("13","testvarnchar2");
	cur->inputBind("14","testvarlchar2");
	cur->inputBind("15",2002,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("16",2002,1,1,2,0,0,0,NULL,false);
	cur->inputBindClob("17","testtext1",9);
	cur->inputBindBlob("18","testbyte1",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1","t");
	cur->inputBind("2",3);
	cur->inputBind("3",3);
	cur->inputBind("4",3);
	cur->inputBind("5",3);
	cur->inputBind("6",3.3,4,2);
	cur->inputBind("7",3.3,4,2);
	cur->inputBind("8",3.3,4,2);
	cur->inputBind("9",3.3,4,2);
	cur->inputBind("10","testchar3");
	cur->inputBind("11","testnchar3");
	cur->inputBind("12","testvarchar3");
	cur->inputBind("13","testvarnchar3");
	cur->inputBind("14","testvarlchar3");
	cur->inputBind("15",2003,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("16",2003,1,1,3,0,0,0,NULL,false);
	cur->inputBindClob("17","testtext3",9);
	cur->inputBindBlob("18","testbyte3",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of binds by position
	stdoutput.printf("ARRAY OF BINDS BY POSITION: \n");
	cur->clearBinds();
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	null, "
		"	null)");
	cur->inputBinds(bindvars,bindvals);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	't', "
		"	5, "
		"	5, "
		"	5, "
		"	5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	'testchar5', "
		"	'testnchar5', "
		"	'testvarchar5', "
		"	'testnvarchar5', "
		"	'testlvarchar5', "
		"	'01/01/2005', "
		"	'2005-01-01 05:00:00', "
		"	'testtext5', "
		"	null)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	't', "
		"	6, "
		"	6, "
		"	6, "
		"	6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	'testchar6', "
		"	'testnchar6', "
		"	'testvarchar6', "
		"	'testnvarchar6', "
		"	'testlvarchar6', "
		"	'01/01/2006', "
		"	'2006-01-01 06:00:00', "
		"	'testtext6', "
		"	null)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	't', "
		"	7, "
		"	7, "
		"	7, "
		"	7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	'testchar7', "
		"	'testnchar7', "
		"	'testvarchar7', "
		"	'testnvarchar7', "
		"	'testlvarchar7', "
		"	'01/01/2007', "
		"	'2007-01-01 07:00:00', "
		"	'testtext7', "
		"	null)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	't', "
		"	8, "
		"	8, "
		"	8, "
		"	8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	'testchar8', "
		"	'testnchar8', "
		"	'testvarchar8', "
		"	'testnvarchar8', "
		"	'testlvarchar8', "
		"	'01/01/2008', "
		"	'2008-01-01 08:00:00', "
		"	'testtext8', "
		"	null)"));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),1);
	stdoutput.printf("\n");


	// stored procedure
	stdoutput.printf("STORED PROCEDURE: \n");
	// return multiple values
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in1 int, "
		"	in2 float, "
		"	in3 varchar(20), "
		"	out out1 int, "
		"	out out2 float, "
		"	out out3 varchar(20)) "
		"let out1 = in1; "
		"	let out2 = in2; "
		"	let out3 = in3; "
		"end procedure;"));
	cur->prepareQuery("{call testproc(?,?,?,?,?,?)}");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	cur->defineOutputBindInteger("4");
	cur->defineOutputBindDouble("5");
	cur->defineOutputBindString("6",20);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("4"),1);
	assertEquals(cur->getOutputBindDouble("5"),1.1);
	assertEquals(cur->getOutputBindString("6"),"hello");
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");


	// stored procedure returning result set
	stdoutput.printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	assertTrue(cur->sendQuery(
		"create procedure testproc() "
		"returning boolean, smallint, varchar(40); "
		"	define out1 boolean; "
		"	define out2 smallint; "
		"	define out3 varchar(40); "
		"	foreach "
		"		select "
		"			testboolean, "
		"			testsmallint, "
		"			testvarchar "
		"		into out1,out2,out3 "
		"		from testtable "
		"	return out1,out2,out3 "
		"	with resume; "
		"	end foreach; "
		"	end procedure;"));
	assertTrue(cur->sendQuery("{call testproc()}"));
	assertEquals(cur->rowCount(),8);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");


	// long blob
	stdoutput.printf("LONG BLOB: \n");
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("create table testtable1 (testtext text)");
	cur->prepareQuery("insert into testtable1 values (?)");
	char	textval[20*1024+1];
	for (int i=0; i<20*1024; i++) {
		textval[i]='C';
	}
	textval[20*1024]='\0';
	cur->inputBindClob("1",textval,20*1024);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testtext from testtable1");
	assertEquals(cur->getFieldLength(0,"testtext"),20*1024);
	assertEquals(cur->getField(0,"testtext"),textval);
	// for some reason stored procedures can only use clob types,
	// rather than text
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	in1 clob, "
		"	out out1 clob) "
		"let out1 = in1; "
		"	end procedure;"));
	cur->prepareQuery("{call testproc(?,?)}");
	cur->inputBindClob("1",textval,20*1024);
	cur->defineOutputBindClob("2");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("2"),20*1024);
	assertEquals(cur->getOutputBindClob("2"),textval);
	assertTrue(cur->sendQuery("drop procedure testproc"));
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
	assertEquals(cur->colCount(),18);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testboolean");
	assertEquals(cur->getColumnName(1),"testsmallint");
	assertEquals(cur->getColumnName(2),"testint");
	assertEquals(cur->getColumnName(3),"testbigint");
	assertEquals(cur->getColumnName(4),"testint8");
	assertEquals(cur->getColumnName(5),"testdecimal");
	assertEquals(cur->getColumnName(6),"testmoney");
	assertEquals(cur->getColumnName(7),"testsmallfloat");
	assertEquals(cur->getColumnName(8),"testfloat");
	assertEquals(cur->getColumnName(9),"testchar");
	assertEquals(cur->getColumnName(10),"testnchar");
	assertEquals(cur->getColumnName(11),"testvarchar");
	assertEquals(cur->getColumnName(12),"testnvarchar");
	assertEquals(cur->getColumnName(13),"testlvarchar");
	assertEquals(cur->getColumnName(14),"testdate");
	assertEquals(cur->getColumnName(15),"testdatetime");
	assertEquals(cur->getColumnName(16),"testtext");
	assertEquals(cur->getColumnName(17),"testbyte");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testboolean");
	assertEquals(cols[1],"testsmallint");
	assertEquals(cols[2],"testint");
	assertEquals(cols[3],"testbigint");
	assertEquals(cols[4],"testint8");
	assertEquals(cols[5],"testdecimal");
	assertEquals(cols[6],"testmoney");
	assertEquals(cols[7],"testsmallfloat");
	assertEquals(cols[8],"testfloat");
	assertEquals(cols[9],"testchar");
	assertEquals(cols[10],"testnchar");
	assertEquals(cols[11],"testvarchar");
	assertEquals(cols[12],"testnvarchar");
	assertEquals(cols[13],"testlvarchar");
	assertEquals(cols[14],"testdate");
	assertEquals(cols[15],"testdatetime");
	assertEquals(cols[16],"testtext");
	assertEquals(cols[17],"testbyte");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"BOOLEAN");
	assertEquals(cur->getColumnType("testboolean"),"BOOLEAN");
	assertEquals(cur->getColumnType(1),"SMALLINT");
	assertEquals(cur->getColumnType("testsmallint"),"SMALLINT");
	assertEquals(cur->getColumnType(2),"INTEGER");
	assertEquals(cur->getColumnType("testint"),"INTEGER");
	assertEquals(cur->getColumnType(3),"BIGINT");
	assertEquals(cur->getColumnType("testbigint"),"BIGINT");
	assertEquals(cur->getColumnType(4),"INT8");
	assertEquals(cur->getColumnType("testint8"),"INT8");
	assertEquals(cur->getColumnType(5),"DECIMAL");
	assertEquals(cur->getColumnType("testdecimal"),"DECIMAL");
	//assertEquals(cur->getColumnType(6),"MONEY");
	//assertEquals(cur->getColumnType("testmoney"),"MONEY");
	assertEquals(cur->getColumnType(6),"DECIMAL");
	assertEquals(cur->getColumnType("testmoney"),"DECIMAL");
	assertEquals(cur->getColumnType(7),"SMALLFLOAT");
	assertEquals(cur->getColumnType("testsmallfloat"),"SMALLFLOAT");
	assertEquals(cur->getColumnType(8),"FLOAT");
	assertEquals(cur->getColumnType("testfloat"),"FLOAT");
	assertEquals(cur->getColumnType(9),"CHAR");
	assertEquals(cur->getColumnType("testchar"),"CHAR");
	//assertEquals(cur->getColumnType(10),"NCHAR");
	//assertEquals(cur->getColumnType("testnchar"),"NCHAR");
	assertEquals(cur->getColumnType(10),"CHAR");
	assertEquals(cur->getColumnType("testnchar"),"CHAR");
	assertEquals(cur->getColumnType(11),"VARCHAR");
	assertEquals(cur->getColumnType("testvarchar"),"VARCHAR");
	//assertEquals(cur->getColumnType(12),"NVARCHAR");
	//assertEquals(cur->getColumnType("testnvarchar"),"NVARCHAR");
	assertEquals(cur->getColumnType(12),"VARCHAR");
	assertEquals(cur->getColumnType("testnvarchar"),"VARCHAR");
	//assertEquals(cur->getColumnType(13),"LVARCHAR");
	//assertEquals(cur->getColumnType("testlvarchar"),"LVARCHAR");
	assertEquals(cur->getColumnType(13),"VARCHAR");
	assertEquals(cur->getColumnType("testlvarchar"),"VARCHAR");
	assertEquals(cur->getColumnType(14),"DATE");
	assertEquals(cur->getColumnType("testdate"),"DATE");
	assertEquals(cur->getColumnType(15),"DATETIME");
	assertEquals(cur->getColumnType("testdatetime"),"DATETIME");
	assertEquals(cur->getColumnType(16),"TEXT");
	assertEquals(cur->getColumnType("testtext"),"TEXT");
	assertEquals(cur->getColumnType(17),"BYTE");
	assertEquals(cur->getColumnType("testbyte"),"BYTE");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),1);
	assertEquals(cur->getColumnLength("testboolean"),1);
	assertEquals(cur->getColumnLength(1),5);
	assertEquals(cur->getColumnLength("testsmallint"),5);
	assertEquals(cur->getColumnLength(2),10);
	assertEquals(cur->getColumnLength("testint"),10);
	assertEquals(cur->getColumnLength(3),20);
	assertEquals(cur->getColumnLength("testbigint"),20);
	assertEquals(cur->getColumnLength(4),20);
	assertEquals(cur->getColumnLength("testint8"),20);
	assertEquals(cur->getColumnLength(5),10);
	assertEquals(cur->getColumnLength("testdecimal"),10);
	assertEquals(cur->getColumnLength(6),16);
	assertEquals(cur->getColumnLength("testmoney"),16);
	assertEquals(cur->getColumnLength(7),7);
	assertEquals(cur->getColumnLength("testsmallfloat"),7);
	assertEquals(cur->getColumnLength(8),15);
	assertEquals(cur->getColumnLength("testfloat"),15);
	assertEquals(cur->getColumnLength(9),40);
	assertEquals(cur->getColumnLength("testchar"),40);
	assertEquals(cur->getColumnLength(10),40);
	assertEquals(cur->getColumnLength("testnchar"),40);
	assertEquals(cur->getColumnLength(11),40);
	assertEquals(cur->getColumnLength("testvarchar"),40);
	assertEquals(cur->getColumnLength(12),40);
	assertEquals(cur->getColumnLength("testnvarchar"),40);
	assertEquals(cur->getColumnLength(13),40);
	assertEquals(cur->getColumnLength("testlvarchar"),40);
	assertEquals(cur->getColumnLength(14),10);
	assertEquals(cur->getColumnLength("testdate"),10);
	assertEquals(cur->getColumnLength(15),19);
	assertEquals(cur->getColumnLength("testdatetime"),19);
	assertEquals(cur->getColumnLength(16),2147483647);
	assertEquals(cur->getColumnLength("testtext"),2147483647);
	//assertEquals(cur->getColumnLength(17),2157483647);
	//assertEquals(cur->getColumnLength("testbyte"),2157483647);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testboolean"),1);
	assertEquals(cur->getLongest(1),1);
	assertEquals(cur->getLongest("testsmallint"),1);
	assertEquals(cur->getLongest(2),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest(3),1);
	assertEquals(cur->getLongest("testbigint"),1);
	assertEquals(cur->getLongest(4),1);
	assertEquals(cur->getLongest("testint8"),1);
	assertEquals(cur->getLongest(5),4);
	assertEquals(cur->getLongest("testdecimal"),4);
	assertEquals(cur->getLongest(6),4);
	assertEquals(cur->getLongest("testmoney"),4);
	assertEquals(cur->getLongest(7),3);
	assertEquals(cur->getLongest("testsmallfloat"),3);
	assertEquals(cur->getLongest(8),3);
	assertEquals(cur->getLongest("testfloat"),3);
	assertEquals(cur->getLongest(9),40);
	assertEquals(cur->getLongest("testchar"),40);
	assertEquals(cur->getLongest(10),40);
	assertEquals(cur->getLongest("testnchar"),40);
	assertEquals(cur->getLongest(11),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(12),13);
	assertEquals(cur->getLongest("testnvarchar"),13);
	assertEquals(cur->getLongest(13),13);
	assertEquals(cur->getLongest("testlvarchar"),13);
	assertEquals(cur->getLongest(14),10);
	assertEquals(cur->getLongest("testdate"),10);
	assertEquals(cur->getLongest(15),19);
	assertEquals(cur->getLongest("testdatetime"),19);
	assertEquals(cur->getLongest(16),9);
	assertEquals(cur->getLongest("testtext"),9);
	assertEquals(cur->getLongest(17),9);
	assertEquals(cur->getLongest("testbyte"),9);
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
	assertEquals(cur->getField(0,3),"1");
	assertEquals(cur->getField(0,4),"1");
	assertEquals(cur->getField(0,5),"1.10");
	assertEquals(cur->getField(0,6),"1.10");
	assertEquals(cur->getField(0,7),"1.1");
	assertEquals(cur->getField(0,8),"1.1");
	assertEquals(cur->getField(0,9),"testchar1                               ");
	assertEquals(cur->getField(0,10),"testnchar1                              ");
	assertEquals(cur->getField(0,11),"testvarchar1");
	assertEquals(cur->getField(0,12),"testnvarchar1");
	assertEquals(cur->getField(0,13),"testlvarchar1");
	assertEquals(cur->getField(0,14),"2001-01-01");
	assertEquals(cur->getField(0,15),"2001-01-01 01:00:00");
	assertEquals(cur->getField(0,16),"testtext1");
	assertEquals(cur->getField(0,17),"");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"1");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8");
	assertEquals(cur->getField(7,3),"8");
	assertEquals(cur->getField(7,4),"8");
	assertEquals(cur->getField(7,5),"8.80");
	assertEquals(cur->getField(7,6),"8.80");
	assertEquals(cur->getField(7,7),"8.8");
	assertEquals(cur->getField(7,8),"8.8");
	assertEquals(cur->getField(7,9),"testchar8                               ");
	assertEquals(cur->getField(7,10),"testnchar8                              ");
	assertEquals(cur->getField(7,11),"testvarchar8");
	assertEquals(cur->getField(7,12),"testnvarchar8");
	assertEquals(cur->getField(7,13),"testlvarchar8");
	assertEquals(cur->getField(7,14),"2008-01-01");
	assertEquals(cur->getField(7,15),"2008-01-01 08:00:00");
	assertEquals(cur->getField(7,16),"testtext8");
	assertEquals(cur->getField(7,17),"");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),1);
	assertEquals(cur->getFieldLength(0,2),1);
	assertEquals(cur->getFieldLength(0,3),1);
	assertEquals(cur->getFieldLength(0,4),1);
	assertEquals(cur->getFieldLength(0,5),4);
	assertEquals(cur->getFieldLength(0,6),4);
	assertEquals(cur->getFieldLength(0,7),3);
	assertEquals(cur->getFieldLength(0,8),3);
	assertEquals(cur->getFieldLength(0,9),40);
	assertEquals(cur->getFieldLength(0,10),40);
	assertEquals(cur->getFieldLength(0,11),12);
	assertEquals(cur->getFieldLength(0,12),13);
	assertEquals(cur->getFieldLength(0,14),10);
	assertEquals(cur->getFieldLength(0,15),19);
	assertEquals(cur->getFieldLength(0,16),9);
	assertEquals(cur->getFieldLength(0,17),0);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),1);
	assertEquals(cur->getFieldLength(7,2),1);
	assertEquals(cur->getFieldLength(7,3),1);
	assertEquals(cur->getFieldLength(7,4),1);
	assertEquals(cur->getFieldLength(7,5),4);
	assertEquals(cur->getFieldLength(7,6),4);
	assertEquals(cur->getFieldLength(7,7),3);
	assertEquals(cur->getFieldLength(7,8),3);
	assertEquals(cur->getFieldLength(7,9),40);
	assertEquals(cur->getFieldLength(7,10),40);
	assertEquals(cur->getFieldLength(7,11),12);
	assertEquals(cur->getFieldLength(7,12),13);
	assertEquals(cur->getFieldLength(7,14),10);
	assertEquals(cur->getFieldLength(7,15),19);
	assertEquals(cur->getFieldLength(7,16),9);
	assertEquals(cur->getFieldLength(7,17),0);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testboolean"),"1");
	assertEquals(cur->getField(0,"testsmallint"),"1");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testbigint"),"1");
	assertEquals(cur->getField(0,"testint8"),"1");
	assertEquals(cur->getField(0,"testdecimal"),"1.10");
	assertEquals(cur->getField(0,"testmoney"),"1.10");
	assertEquals(cur->getField(0,"testsmallfloat"),"1.1");
	assertEquals(cur->getField(0,"testfloat"),"1.1");
	assertEquals(cur->getField(0,"testchar"),"testchar1                               ");
	assertEquals(cur->getField(0,"testnchar"),"testnchar1                              ");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testnvarchar"),"testnvarchar1");
	assertEquals(cur->getField(0,"testlvarchar"),"testlvarchar1");
	assertEquals(cur->getField(0,"testdate"),"2001-01-01");
	assertEquals(cur->getField(0,"testdatetime"),"2001-01-01 01:00:00");
	assertEquals(cur->getField(0,"testtext"),"testtext1");
	assertEquals(cur->getField(0,"testbyte"),"");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testboolean"),"1");
	assertEquals(cur->getField(7,"testsmallint"),"8");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testbigint"),"8");
	assertEquals(cur->getField(7,"testint8"),"8");
	assertEquals(cur->getField(7,"testdecimal"),"8.80");
	assertEquals(cur->getField(7,"testmoney"),"8.80");
	assertEquals(cur->getField(7,"testsmallfloat"),"8.8");
	assertEquals(cur->getField(7,"testfloat"),"8.8");
	assertEquals(cur->getField(7,"testchar"),"testchar8                               ");
	assertEquals(cur->getField(7,"testnchar"),"testnchar8                              ");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testnvarchar"),"testnvarchar8");
	assertEquals(cur->getField(7,"testlvarchar"),"testlvarchar8");
	assertEquals(cur->getField(7,"testdate"),"2008-01-01");
	assertEquals(cur->getField(7,"testdatetime"),"2008-01-01 08:00:00");
	assertEquals(cur->getField(7,"testtext"),"testtext8");
	assertEquals(cur->getField(7,"testbyte"),"");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testboolean"),1);
	assertEquals(cur->getFieldLength(0,"testsmallint"),1);
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testbigint"),1);
	assertEquals(cur->getFieldLength(0,"testint8"),1);
	assertEquals(cur->getFieldLength(0,"testdecimal"),4);
	assertEquals(cur->getFieldLength(0,"testmoney"),4);
	assertEquals(cur->getFieldLength(0,"testsmallfloat"),3);
	assertEquals(cur->getFieldLength(0,"testfloat"),3);
	assertEquals(cur->getFieldLength(0,"testchar"),40);
	assertEquals(cur->getFieldLength(0,"testnchar"),40);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testnvarchar"),13);
	assertEquals(cur->getFieldLength(0,"testlvarchar"),13);
	assertEquals(cur->getFieldLength(0,"testdate"),10);
	assertEquals(cur->getFieldLength(0,"testdatetime"),19);
	assertEquals(cur->getFieldLength(0,"testtext"),9);
	assertEquals(cur->getFieldLength(0,"testbyte"),0);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testboolean"),1);
	assertEquals(cur->getFieldLength(7,"testsmallint"),1);
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testbigint"),1);
	assertEquals(cur->getFieldLength(7,"testint8"),1);
	assertEquals(cur->getFieldLength(7,"testdecimal"),4);
	assertEquals(cur->getFieldLength(7,"testmoney"),4);
	assertEquals(cur->getFieldLength(7,"testsmallfloat"),3);
	assertEquals(cur->getFieldLength(7,"testfloat"),3);
	assertEquals(cur->getFieldLength(7,"testchar"),40);
	assertEquals(cur->getFieldLength(7,"testnchar"),40);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testnvarchar"),13);
	assertEquals(cur->getFieldLength(7,"testlvarchar"),13);
	assertEquals(cur->getFieldLength(7,"testdate"),10);
	assertEquals(cur->getFieldLength(7,"testdatetime"),19);
	assertEquals(cur->getFieldLength(7,"testtext"),9);
	assertEquals(cur->getFieldLength(7,"testbyte"),0);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1");
	assertEquals(fields[2],"1");
	assertEquals(fields[3],"1");
	assertEquals(fields[4],"1");
	assertEquals(fields[5],"1.10");
	assertEquals(fields[6],"1.10");
	assertEquals(fields[7],"1.1");
	assertEquals(fields[8],"1.1");
	assertEquals(fields[9],"testchar1                               ");
	assertEquals(fields[10],"testnchar1                              ");
	assertEquals(fields[11],"testvarchar1");
	assertEquals(fields[12],"testnvarchar1");
	assertEquals(fields[13],"testlvarchar1");
	assertEquals(fields[14],"2001-01-01");
	assertEquals(fields[15],"2001-01-01 01:00:00");
	assertEquals(fields[16],"testtext1");
	assertEquals(fields[17],"");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],1);
	assertEquals(fieldlens[2],1);
	assertEquals(fieldlens[3],1);
	assertEquals(fieldlens[4],1);
	assertEquals(fieldlens[5],4);
	assertEquals(fieldlens[6],4);
	assertEquals(fieldlens[7],3);
	assertEquals(fieldlens[8],3);
	assertEquals(fieldlens[9],40);
	assertEquals(fieldlens[10],40);
	assertEquals(fieldlens[11],12);
	assertEquals(fieldlens[12],13);
	assertEquals(fieldlens[14],10);
	assertEquals(fieldlens[15],19);
	assertEquals(fieldlens[16],9);
	assertEquals(fieldlens[17],0);
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery(
		"select "
		"	$(var1), "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	sysmaster:sysdual ");
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
		"	sysmaster:sysdual ");
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
	cur->prepareQuery(
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	sysmaster:sysdual ");
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
	cur->prepareQuery(
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	sysmaster:sysdual ");
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
	cur->sendQuery("drop table testtable1");
	cur->sendQuery(
		"create table testtable1 ("
		"	col1 char(1), "
		"	col2 char(1), "
		"	col3 char(1))");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	'1', "
		"	NULL, "
		"	NULL)"));
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"");
	assertEquals(cur->getField(0,2),"");
	assertTrue(cur->sendQuery("drop table testtable1"));
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
		"	testsmallint "));
	assertEquals(cur->getResultSetBufferSize(),2);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),0);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)1),"1");
	assertEquals(cur->getField(1,(uint32_t)1),"2");
	assertEquals(cur->getField(2,(uint32_t)1),"3");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),2);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),4);
	assertEquals(cur->getField(6,(uint32_t)1),"7");
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)1),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
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
	assertEquals(cur->getColumnName((uint32_t)1),NULL);
	assertEquals(cur->getColumnLength((uint32_t)1),0);
	assertEquals(cur->getColumnType((uint32_t)1),NULL);
	cur->getColumnInfo();
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertEquals(cur->getColumnName((uint32_t)1),"testsmallint");
	assertEquals(cur->getColumnLength((uint32_t)1),5);
	assertEquals(cur->getColumnType((uint32_t)1),"SMALLINT");
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
	assertEquals(cur->getField(0,(uint32_t)1),"1");
	assertEquals(cur->getField(1,(uint32_t)1),"2");
	assertEquals(cur->getField(2,(uint32_t)1),"3");
	assertEquals(cur->getField(3,(uint32_t)1),"4");
	assertEquals(cur->getField(4,(uint32_t)1),"5");
	assertEquals(cur->getField(5,(uint32_t)1),"6");
	assertEquals(cur->getField(6,(uint32_t)1),"7");
	assertEquals(cur->getField(7,(uint32_t)1),"8");
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
	assertEquals(cur->getField(0,(uint32_t)1),"1");
	assertEquals(cur->getField(1,(uint32_t)1),"2");
	assertEquals(cur->getField(2,(uint32_t)1),"3");
	assertEquals(cur->getField(3,(uint32_t)1),"4");
	assertEquals(cur->getField(4,(uint32_t)1),"5");
	assertEquals(cur->getField(5,(uint32_t)1),"6");
	assertEquals(cur->getField(6,(uint32_t)1),"7");
	assertEquals(cur->getField(7,(uint32_t)1),"8");
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
	assertEquals(cur->getField(0,(uint32_t)1),"1");
	assertEquals(cur->getField(1,(uint32_t)1),"2");
	assertEquals(cur->getField(2,(uint32_t)1),"3");
	assertEquals(cur->getField(3,(uint32_t)1),"4");
	assertEquals(cur->getField(4,(uint32_t)1),"5");
	assertEquals(cur->getField(5,(uint32_t)1),"6");
	assertEquals(cur->getField(6,(uint32_t)1),"7");
	assertEquals(cur->getField(7,(uint32_t)1),"8");
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
	assertEquals(cur->getField(2,(uint32_t)1),"3");
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
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)1),NULL);
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
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	delete[] filename;
	stdoutput.printf("\n");


	// column count for cached result set
	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),18);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"testboolean");
	assertEquals(cur->getColumnName(1),"testsmallint");
	assertEquals(cur->getColumnName(2),"testint");
	assertEquals(cur->getColumnName(3),"testbigint");
	assertEquals(cur->getColumnName(4),"testint8");
	assertEquals(cur->getColumnName(5),"testdecimal");
	assertEquals(cur->getColumnName(6),"testmoney");
	assertEquals(cur->getColumnName(7),"testsmallfloat");
	assertEquals(cur->getColumnName(8),"testfloat");
	assertEquals(cur->getColumnName(9),"testchar");
	assertEquals(cur->getColumnName(10),"testnchar");
	assertEquals(cur->getColumnName(11),"testvarchar");
	assertEquals(cur->getColumnName(12),"testnvarchar");
	assertEquals(cur->getColumnName(13),"testlvarchar");
	assertEquals(cur->getColumnName(14),"testdate");
	assertEquals(cur->getColumnName(15),"testdatetime");
	assertEquals(cur->getColumnName(16),"testtext");
	assertEquals(cur->getColumnName(17),"testbyte");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testboolean");
	assertEquals(cols[1],"testsmallint");
	assertEquals(cols[2],"testint");
	assertEquals(cols[3],"testbigint");
	assertEquals(cols[4],"testint8");
	assertEquals(cols[5],"testdecimal");
	assertEquals(cols[6],"testmoney");
	assertEquals(cols[7],"testsmallfloat");
	assertEquals(cols[8],"testfloat");
	assertEquals(cols[9],"testchar");
	assertEquals(cols[10],"testnchar");
	assertEquals(cols[11],"testvarchar");
	assertEquals(cols[12],"testnvarchar");
	assertEquals(cols[13],"testlvarchar");
	assertEquals(cols[14],"testdate");
	assertEquals(cols[15],"testdatetime");
	assertEquals(cols[16],"testtext");
	assertEquals(cols[17],"testbyte");
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
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	assertEquals(cur->getField(8,(uint32_t)1),NULL);
	cur->setResultSetBufferSize(0);
	delete[] filename;
	stdoutput.printf("\n");


	// from one cache file to another
	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	assertEquals(cur->getField(8,(uint32_t)1),NULL);
	stdoutput.printf("\n");


	// from one cache file to another with result set buffer size
	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	assertEquals(cur->getField(8,(uint32_t)1),NULL);
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
	assertEquals(cur->getField(2,(uint32_t)1),"3");
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
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)1),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	cur->cacheOff();
	stdoutput.printf("\n");
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	assertEquals(cur->getField(8,(uint32_t)1),NULL);
	cur->setResultSetBufferSize(0);
	delete[] filename;
	stdoutput.printf("\n");


	// finished suspended session
	stdoutput.printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getField(4,(uint32_t)1),"5");
	assertEquals(cur->getField(5,(uint32_t)1),"6");
	assertEquals(cur->getField(6,(uint32_t)1),"7");
	assertEquals(cur->getField(7,(uint32_t)1),"8");
	id=cur->getResultSetId();
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	assertTrue(cur->resumeResultSet(id));
	assertEquals(cur->getField(4,(uint32_t)1),NULL);
	assertEquals(cur->getField(5,(uint32_t)1),NULL);
	assertEquals(cur->getField(6,(uint32_t)1),NULL);
	assertEquals(cur->getField(7,(uint32_t)1),NULL);
	stdoutput.printf("\n");

	// drop existing table
	cur->sendQuery("drop table testtable");


	// clob/blob
	stdoutput.printf("CLOB/BLOB: \n");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	cur->prepareQuery("insert into testtable values (?,?)");
	cur->inputBindClob("1","testclobvalue",13);
	cur->inputBindBlob("2","testblobvalue",13);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"testclobvalue");
	assertEquals(cur->getField(0,1),"testblobvalue");
	assertTrue(cur->sendQuery(
		"create procedure testproc("
		"	out out1 clob, "
		"	out out2 blob) "
		"select testclob, testblob "
		"	into out1,out2 "
		"	from testtable; "
		"	end procedure;"));
	cur->prepareQuery("{call testproc(?,?)}");
	cur->defineOutputBindClob("1");
	cur->defineOutputBindBlob("2");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindClob("1"),"testclobvalue");
	assertEquals(cur->getOutputBindBlob("2"),"testblobvalue");
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


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
	// informix requires that a table exist that is
	// owned by a user for the user to be reported
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(cur->getSchemaList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	assertTrue(cur->rowCount()>0);
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// table type list
	stdoutput.printf("TABLE TYPE LIST: \n");
	assertTrue(cur->getTableTypeList());
	assertEquals(cur->getColumnName(0),"table_type");
	bool	found=false;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		if (!charstring::compareIgnoringCase(
				cur->getField(i,"table_type"),
				"TABLE")) {
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
	assertTrue(cur->getTableList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"Tables_in_xxx");
		if (!charstring::compareIgnoringCase(name,"testtable1") ||
			!charstring::compareIgnoringCase(name,"testtable2") ||
			!charstring::compareIgnoringCase(name,"testtable3") ||
			!charstring::compareIgnoringCase(name,"testtable4")) {
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
	assertEquals(cur->getField(0,"precision"),"32767");
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


	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 serial primary key, "
		"	col2 int)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertTrue(charstring::containsIgnoringCase(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::containsIgnoringCase(
			cur->getField(0,"column_key"),"PRI"));
	assertFalse(charstring::containsIgnoringCase(
			cur->getField(1,"extra"),"auto_increment"));
	assertFalse(charstring::containsIgnoringCase(
			cur->getField(1,"column_key"),"PRI"));
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertFalse(charstring::containsIgnoringCase(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::containsIgnoringCase(
			cur->getField(0,"column_key"),"PRI"));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 integer primary key, "
		"	col2 integer)"));
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
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"table"),"testtable"));
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"column_name"),"col1"));
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 integer primary key, "
		"	col2 integer)"));
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
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"table"),"testtable"));
	assertEquals(cur->getField(0,"non_unique"),"0");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"column_name"),"col1"));
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"3");
	assertTrue(!charstring::isNullOrEmpty(cur->getField(0,"key_name")));
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	cur->sendQuery("drop procedure testproc1");
	cur->sendQuery("drop procedure testproc2");
	cur->sendQuery("drop procedure testproc3");
	cur->sendQuery("drop procedure testproc4");
	assertTrue(cur->sendQuery(
		"create procedure testproc1("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(cur->sendQuery(
		"create procedure testproc2("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(cur->sendQuery(
		"create procedure testproc3("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(cur->sendQuery(
		"create procedure testproc4("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(cur->getProcedureList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"routine_name");
		if (!charstring::compareIgnoringCase(name,"testproc1") ||
			!charstring::compareIgnoringCase(name,"testproc2") ||
			!charstring::compareIgnoringCase(name,"testproc3") ||
			!charstring::compareIgnoringCase(name,"testproc4")) {
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
	assertEquals(cur->getField(0,"data_type"),"INTEGER");
	assertEquals(cur->getField(0,"ordinal_position"),"1");
	assertEquals(cur->getField(1,"parameter_name"),"in2");
	assertEquals(cur->getField(1,"parameter_mode"),"1");
	assertEquals(cur->getField(1,"data_type"),"CHAR");
	assertEquals(cur->getField(1,"ordinal_position"),"2");
	assertEquals(cur->getField(2,"parameter_name"),"in3");
	assertEquals(cur->getField(2,"parameter_mode"),"1");
	assertEquals(cur->getField(2,"data_type"),"VARCHAR");
	assertEquals(cur->getField(2,"ordinal_position"),"3");
	assertEquals(cur->getField(3,"parameter_name"),"in4");
	assertEquals(cur->getField(3,"parameter_mode"),"1");
	assertEquals(cur->getField(3,"data_type"),"DATE");
	assertEquals(cur->getField(3,"ordinal_position"),"4");
	cur->sendQuery("drop procedure testproc1");
	cur->sendQuery("drop procedure testproc2");
	cur->sendQuery("drop procedure testproc3");
	cur->sendQuery("drop procedure testproc4");
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
