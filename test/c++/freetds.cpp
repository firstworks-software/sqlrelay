// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/sys.h>
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

	const char	*isolationlevels[]={"1","0","2","3",NULL};
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

	#define	LARGE_BUFFER_LENGTH	8192
	char		largebuffer[LARGE_BUFFER_LENGTH+1];


	// hostname
	char	*hostname=sys::getHostName();
	char	*dot=(char *)charstring::findFirstOrEnd(hostname,'.');
	*dot='\0';
	stringbuffer	dumptran;
	dumptran.append("dump tran ");
	dumptran.append(hostname);
	dumptran.append(" with truncate_only");


	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"freetds");
	stdoutput.printf("\n");


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// bind format
	stdoutput.printf("BIND FORMAT: \n");
	assertEquals(con->bindFormat(),"@*");
	stdoutput.printf("\n");


	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"%s.nextval");
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
	cur->sendQuery(dumptran.getString());
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testsmallint smallint, "
		"	testtinyint tinyint, "
		"	testreal real, "
		"	testfloat float, "
		"	testdecimal decimal(4,1), "
		"	testnumeric numeric(4,1), "
		"	testmoney money, "
		"	testsmallmoney smallmoney, "
		"	testdatetime datetime, "
		"	testsmalldatetime smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit) lock datarows"));
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(con->begin());
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
		"	1.1, "
		"	1.00, "
		"	1.00, "
		"	'01-Jan-2001 01:00:00', "
		"	'01-Jan-2001 01:00:00', "
		"	'testchar1', "
		"	'testvarchar1', "
		"	1)"));
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
		"	?, "
		"	?, "
		"	?)");
	assertEquals(cur->countBindVariables(),14);
	cur->inputBind("1",2);
	cur->inputBind("2",2);
	cur->inputBind("3",2);
	cur->inputBind("4",2.2,2,1);
	cur->inputBind("5",2.2,2,1);
	cur->inputBind("6",2.2,2,1);
	cur->inputBind("7",2.2,2,1);
	cur->inputBind("8",2.00,3,2);
	cur->inputBind("9",2.00,3,2);
	cur->inputBind("10","01-Jan-2002 02:00:00");
	cur->inputBind("11","01-Jan-2002 02:00:00");
	cur->inputBind("12","testchar2");
	cur->inputBind("13","testvarchar2");
	cur->inputBind("14",1);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",3);
	cur->inputBind("2",3);
	cur->inputBind("3",3);
	cur->inputBind("4",3.3,2,1);
	cur->inputBind("5",3.3,2,1);
	cur->inputBind("6",3.3,2,1);
	cur->inputBind("7",3.3,2,1);
	cur->inputBind("8",3.00,3,2);
	cur->inputBind("9",3.00,3,2);
	cur->inputBind("10","01-Jan-2003 03:00:00");
	cur->inputBind("11","01-Jan-2003 03:00:00");
	cur->inputBind("12","testchar3");
	cur->inputBind("13","testvarchar3");
	cur->inputBind("14",1);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of input binds by position
	// freetds doesn't support implicit conversion of string binds to other
	// data types, so arrays of binds don't generally work.


	// input bind by position with validation
	stdoutput.printf("INPUT BIND BY POSITION WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("1",4);
	cur->inputBind("2",4);
	cur->inputBind("3",4);
	cur->inputBind("4",4.4,2,1);
	cur->inputBind("5",4.4,2,1);
	cur->inputBind("6",4.4,2,1);
	cur->inputBind("7",4.4,2,1);
	cur->inputBind("8",4.00,3,2);
	cur->inputBind("9",4.00,3,2);
	cur->inputBind("10","01-Jan-2004 04:00:00");
	cur->inputBind("11","01-Jan-2004 04:00:00");
	cur->inputBind("12","testchar4");
	cur->inputBind("13","testvarchar4");
	cur->inputBind("14",1);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name
	stdoutput.printf("INPUT BIND BY NAME: \n");
	cur->clearBinds();
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	@var1, "
		"	@var2, "
		"	@var3, "
		"	@var4, "
		"	@var5, "
		"	@var6, "
		"	@var7, "
		"	@var8, "
		"	@var9, "
		"	@var10, "
		"	@var11, "
		"	@var12, "
		"	@var13, "
		"	@var14)");
	assertEquals(cur->countBindVariables(),14);
	cur->inputBind("var1",5);
	cur->inputBind("var2",5);
	cur->inputBind("var3",5);
	cur->inputBind("var4",5.5,2,1);
	cur->inputBind("var5",5.5,2,1);
	cur->inputBind("var6",5.5,2,1);
	cur->inputBind("var7",5.5,2,1);
	cur->inputBind("var8",5.00,3,2);
	cur->inputBind("var9",5.00,3,2);
	cur->inputBind("var10","01-Jan-2005 05:00:00");
	cur->inputBind("var11","01-Jan-2005 05:00:00");
	cur->inputBind("var12","testchar5");
	cur->inputBind("var13","testvarchar5");
	cur->inputBind("var14",1);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("var1",6);
	cur->inputBind("var2",6);
	cur->inputBind("var3",6);
	cur->inputBind("var4",6.6,2,1);
	cur->inputBind("var5",6.6,2,1);
	cur->inputBind("var6",6.6,2,1);
	cur->inputBind("var7",6.6,2,1);
	cur->inputBind("var8",6.00,3,2);
	cur->inputBind("var9",6.00,3,2);
	cur->inputBind("var10","01-Jan-2006 06:00:00");
	cur->inputBind("var11","01-Jan-2006 06:00:00");
	cur->inputBind("var12","testchar6");
	cur->inputBind("var13","testvarchar6");
	cur->inputBind("var14",1);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("var1",7);
	cur->inputBind("var2",7);
	cur->inputBind("var3",7);
	cur->inputBind("var4",7.7,2,1);
	cur->inputBind("var5",7.7,2,1);
	cur->inputBind("var6",7.7,2,1);
	cur->inputBind("var7",7.7,2,1);
	cur->inputBind("var8",7.00,3,2);
	cur->inputBind("var9",7.00,3,2);
	cur->inputBind("var10","01-Jan-2007 07:00:00");
	cur->inputBind("var11","01-Jan-2007 07:00:00");
	cur->inputBind("var12","testchar7");
	cur->inputBind("var13","testvarchar7");
	cur->inputBind("var14",1);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// array of input binds by name
	// freetds doesn't support implicit conversion of string binds to other
	// data types, so arrays of binds don't generally work.


	// input bind by name with validation
	stdoutput.printf("INPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("var1",8);
	cur->inputBind("var2",8);
	cur->inputBind("var3",8);
	cur->inputBind("var4",8.8,2,1);
	cur->inputBind("var5",8.8,2,1);
	cur->inputBind("var6",8.8,2,1);
	cur->inputBind("var7",8.8,2,1);
	cur->inputBind("var8",8.00,3,2);
	cur->inputBind("var9",8.00,3,2);
	cur->inputBind("var10","01-Jan-2008 08:00:00");
	cur->inputBind("var11","01-Jan-2008 08:00:00");
	cur->inputBind("var12","testchar8");
	cur->inputBind("var13","testvarchar8");
	cur->inputBind("var14",1);
	cur->inputBind("var15","junkvalue");
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),14);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testsmallint");
	assertEquals(cur->getColumnName(2),"testtinyint");
	assertEquals(cur->getColumnName(3),"testreal");
	assertEquals(cur->getColumnName(4),"testfloat");
	assertEquals(cur->getColumnName(5),"testdecimal");
	assertEquals(cur->getColumnName(6),"testnumeric");
	assertEquals(cur->getColumnName(7),"testmoney");
	assertEquals(cur->getColumnName(8),"testsmallmoney");
	assertEquals(cur->getColumnName(9),"testdatetime");
	assertEquals(cur->getColumnName(10),"testsmalldatetime");
	assertEquals(cur->getColumnName(11),"testchar");
	assertEquals(cur->getColumnName(12),"testvarchar");
	assertEquals(cur->getColumnName(13),"testbit");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testsmallint");
	assertEquals(cols[2],"testtinyint");
	assertEquals(cols[3],"testreal");
	assertEquals(cols[4],"testfloat");
	assertEquals(cols[5],"testdecimal");
	assertEquals(cols[6],"testnumeric");
	assertEquals(cols[7],"testmoney");
	assertEquals(cols[8],"testsmallmoney");
	assertEquals(cols[9],"testdatetime");
	assertEquals(cols[10],"testsmalldatetime");
	assertEquals(cols[11],"testchar");
	assertEquals(cols[12],"testvarchar");
	assertEquals(cols[13],"testbit");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"INT");
	assertEquals(cur->getColumnType("testint"),"INT");
	assertEquals(cur->getColumnType(1),"SMALLINT");
	assertEquals(cur->getColumnType("testsmallint"),"SMALLINT");
	assertEquals(cur->getColumnType(2),"TINYINT");
	assertEquals(cur->getColumnType("testtinyint"),"TINYINT");
	assertEquals(cur->getColumnType(3),"REAL");
	assertEquals(cur->getColumnType("testreal"),"REAL");
	assertEquals(cur->getColumnType(4),"FLOAT");
	assertEquals(cur->getColumnType("testfloat"),"FLOAT");
	assertEquals(cur->getColumnType(5),"DECIMAL");
	assertEquals(cur->getColumnType("testdecimal"),"DECIMAL");
	assertEquals(cur->getColumnType(6),"NUMERIC");
	assertEquals(cur->getColumnType("testnumeric"),"NUMERIC");
	assertEquals(cur->getColumnType(7),"MONEY");
	assertEquals(cur->getColumnType("testmoney"),"MONEY");
	assertEquals(cur->getColumnType(8),"SMALLMONEY");
	assertEquals(cur->getColumnType("testsmallmoney"),"SMALLMONEY");
	assertEquals(cur->getColumnType(9),"DATETIME");
	assertEquals(cur->getColumnType("testdatetime"),"DATETIME");
	assertEquals(cur->getColumnType(10),"SMALLDATETIME");
	assertEquals(cur->getColumnType("testsmalldatetime"),"SMALLDATETIME");
	assertEquals(cur->getColumnType(11),"CHAR");
	assertEquals(cur->getColumnType("testchar"),"CHAR");
	assertEquals(cur->getColumnType(12),"CHAR");
	assertEquals(cur->getColumnType("testvarchar"),"CHAR");
	assertEquals(cur->getColumnType(13),"BIT");
	assertEquals(cur->getColumnType("testbit"),"BIT");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnLength("testint"),4);
	assertEquals(cur->getColumnLength(1),2);
	assertEquals(cur->getColumnLength("testsmallint"),2);
	assertEquals(cur->getColumnLength(2),1);
	assertEquals(cur->getColumnLength("testtinyint"),1);
	assertEquals(cur->getColumnLength(3),4);
	assertEquals(cur->getColumnLength("testreal"),4);
	assertEquals(cur->getColumnLength(4),8);
	assertEquals(cur->getColumnLength("testfloat"),8);
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getColumnLength(5),3);
	//assertEquals(cur->getColumnLength("testdecimal"),3);
	//assertEquals(cur->getColumnLength(6),3);
	//assertEquals(cur->getColumnLength("testnumeric"),3);
	assertEquals(cur->getColumnLength(7),8);
	assertEquals(cur->getColumnLength("testmoney"),8);
	assertEquals(cur->getColumnLength(8),4);
	assertEquals(cur->getColumnLength("testsmallmoney"),4);
	assertEquals(cur->getColumnLength(9),8);
	assertEquals(cur->getColumnLength("testdatetime"),8);
	assertEquals(cur->getColumnLength(10),4);
	assertEquals(cur->getColumnLength("testsmalldatetime"),4);
	// these seem to fluctuate too
	//assertEquals(cur->getColumnLength(11),40);
	//assertEquals(cur->getColumnLength("testchar"),40);
	//assertEquals(cur->getColumnLength(12),40);
	//assertEquals(cur->getColumnLength("testvarchar"),40);
	assertEquals(cur->getColumnLength(13),1);
	assertEquals(cur->getColumnLength("testbit"),1);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest(1),1);
	assertEquals(cur->getLongest("testsmallint"),1);
	assertEquals(cur->getLongest(2),1);
	assertEquals(cur->getLongest("testtinyint"),1);
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getLongest(3),3);
	//assertEquals(cur->getLongest("testreal"),3);
	//assertEquals(cur->getLongest(4),17);
	//assertEquals(cur->getLongest("testfloat"),17);
	//assertEquals(cur->getLongest(5),3);
	//assertEquals(cur->getLongest("testdecimal"),3);
	//assertEquals(cur->getLongest(6),3);
	//assertEquals(cur->getLongest("testnumeric"),3);
	//assertEquals(cur->getLongest(7),4);
	//assertEquals(cur->getLongest("testmoney"),4);
	//assertEquals(cur->getLongest(8),4);
	//assertEquals(cur->getLongest("testsmallmoney"),4);
	//assertEquals(cur->getLongest(9),26);
	//assertEquals(cur->getLongest("testdatetime"),26);
	//assertEquals(cur->getLongest(10),26);
	//assertEquals(cur->getLongest("testsmalldatetime"),26);
	assertEquals(cur->getLongest(11),40);
	assertEquals(cur->getLongest("testchar"),40);
	assertEquals(cur->getLongest(12),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(13),1);
	assertEquals(cur->getLongest("testbit"),1);
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
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getField(0,3),"1.1");
	//assertEquals(cur->getField(0,4),"1.1");
	assertEquals(cur->getField(0,5),"1.1");
	assertEquals(cur->getField(0,6),"1.1");
	//assertEquals(cur->getField(0,7),"1.00");
	//assertEquals(cur->getField(0,8),"1.00");
	//assertEquals(cur->getField(0,9),"Jan  1 2001 01:00:00:000AM");
	//assertEquals(cur->getField(0,10),"Jan  1 2001 01:00:00:000AM");
	assertEquals(cur->getField(0,11),"testchar1                               ");
	assertEquals(cur->getField(0,12),"testvarchar1");
	assertEquals(cur->getField(0,13),"1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8");
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getField(7,3),"8.8");
	//assertEquals(cur->getField(7,4),"8.8");
	assertEquals(0,charstring::compare(cur->getField(7,5),"8.8",3));
	assertEquals(0,charstring::compare(cur->getField(7,6),"8.8",3));
	//assertEquals(cur->getField(7,7),"8.00");
	//assertEquals(cur->getField(7,8),"8.00");
	//assertEquals(cur->getField(7,9),"Jan  1 2008 08:00:00:000AM");
	//assertEquals(cur->getField(7,10),"Jan  1 2008 08:00:00:000AM");
	assertEquals(cur->getField(7,11),"testchar8                               ");
	assertEquals(cur->getField(7,12),"testvarchar8");
	assertEquals(cur->getField(7,13),"1");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),1);
	assertEquals(cur->getFieldLength(0,2),1);
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getFieldLength(0,3),3);
	//assertEquals(cur->getFieldLength(0,4),3);
	//assertEquals(cur->getFieldLength(0,5),3);
	//assertEquals(cur->getFieldLength(0,6),3);
	//assertEquals(cur->getFieldLength(0,7),4);
	//assertEquals(cur->getFieldLength(0,8),4);
	//assertEquals(cur->getFieldLength(0,9),26);
	//assertEquals(cur->getFieldLength(0,10),26);
	assertEquals(cur->getFieldLength(0,11),40);
	assertEquals(cur->getFieldLength(0,12),12);
	assertEquals(cur->getFieldLength(0,13),1);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),1);
	assertEquals(cur->getFieldLength(7,2),1);
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getFieldLength(7,3),3);
	//assertEquals(cur->getFieldLength(7,4),17);
	//assertEquals(cur->getFieldLength(7,5),3);
	//assertEquals(cur->getFieldLength(7,6),3);
	//assertEquals(cur->getFieldLength(7,7),4);
	//assertEquals(cur->getFieldLength(7,8),4);
	//assertEquals(cur->getFieldLength(7,9),26);
	//assertEquals(cur->getFieldLength(7,10),26);
	assertEquals(cur->getFieldLength(7,11),40);
	assertEquals(cur->getFieldLength(7,12),12);
	assertEquals(cur->getFieldLength(7,13),1);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testsmallint"),"1");
	assertEquals(cur->getField(0,"testtinyint"),"1");
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getField(0,"testreal"),"1.1");
	//assertEquals(cur->getField(0,"testfloat"),"1.1");
	assertEquals(cur->getField(0,"testdecimal"),"1.1");
	assertEquals(cur->getField(0,"testnumeric"),"1.1");
	//assertEquals(cur->getField(0,"testmoney"),"1.00");
	//assertEquals(cur->getField(0,"testsmallmoney"),"1.00");
	//assertEquals(cur->getField(0,"testdatetime"),"Jan  1 2001 01:00:00:000AM");
	//assertEquals(cur->getField(0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM");
	assertEquals(cur->getField(0,"testchar"),"testchar1                               ");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testbit"),"1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testsmallint"),"8");
	assertEquals(cur->getField(7,"testtinyint"),"8");
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getField(7,"testreal"),"8.8");
	//assertEquals(cur->getField(7,"testfloat"),"8.8");
	assertEquals(0,charstring::compare(cur->getField(7,"testdecimal"),"8.8",3));
	assertEquals(0,charstring::compare(cur->getField(7,"testnumeric"),"8.8",3));
	//assertEquals(cur->getField(7,"testmoney"),"8.00");
	//assertEquals(cur->getField(7,"testsmallmoney"),"8.00");
	//assertEquals(cur->getField(7,"testdatetime"),"Jan  1 2008 08:00:00:000AM");
	//assertEquals(cur->getField(7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM");
	assertEquals(cur->getField(7,"testchar"),"testchar8                               ");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testbit"),"1");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testsmallint"),1);
	assertEquals(cur->getFieldLength(0,"testtinyint"),1);
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getFieldLength(0,"testreal"),3);
	//assertEquals(cur->getFieldLength(0,"testfloat"),3);
	//assertEquals(cur->getFieldLength(0,"testdecimal"),3);
	//assertEquals(cur->getFieldLength(0,"testnumeric"),3);
	//assertEquals(cur->getFieldLength(0,"testmoney"),4);
	//assertEquals(cur->getFieldLength(0,"testsmallmoney"),4);
	//assertEquals(cur->getFieldLength(0,"testdatetime"),26);
	//assertEquals(cur->getFieldLength(0,"testsmalldatetime"),26);
	assertEquals(cur->getFieldLength(0,"testchar"),40);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testbit"),1);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testsmallint"),1);
	assertEquals(cur->getFieldLength(7,"testtinyint"),1);
	// these seem to fluctuate with every freetds release
	//assertEquals(cur->getFieldLength(7,"testreal"),3);
	//assertEquals(cur->getFieldLength(7,"testfloat"),17);
	//assertEquals(cur->getFieldLength(7,"testdecimal"),3);
	//assertEquals(cur->getFieldLength(7,"testnumeric"),3);
	//assertEquals(cur->getFieldLength(7,"testmoney"),4);
	//assertEquals(cur->getFieldLength(7,"testsmallmoney"),4);
	//assertEquals(cur->getFieldLength(7,"testdatetime"),26);
	//assertEquals(cur->getFieldLength(7,"testsmalldatetime"),26);
	assertEquals(cur->getFieldLength(7,"testchar"),40);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testbit"),1);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1");
	assertEquals(fields[2],"1");
	// these seem to fluctuate with every freetds release
	//assertEquals(fields[3],"1.1");
	//assertEquals(fields[4],"1.1");
	assertEquals(fields[5],"1.1");
	assertEquals(fields[6],"1.1");
	//assertEquals(fields[7],"1.00");
	//assertEquals(fields[8],"1.00");
	//assertEquals(fields[9],"Jan  1 2001 01:00:00:000AM");
	//assertEquals(fields[10],"Jan  1 2001 01:00:00:000AM");
	assertEquals(fields[11],"testchar1                               ");
	assertEquals(fields[12],"testvarchar1");
	assertEquals(fields[13],"1");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],1);
	assertEquals(fieldlens[2],1);
	// these seem to fluctuate with every freetds release
	//assertEquals(fieldlens[3],3);
	//assertEquals(fieldlens[4],3);
	//assertEquals(fieldlens[5],3);
	//assertEquals(fieldlens[6],3);
	//assertEquals(fieldlens[7],4);
	//assertEquals(fieldlens[8],4);
	//assertEquals(fieldlens[9],26);
	//assertEquals(fieldlens[10],26);
	assertEquals(fieldlens[11],40);
	assertEquals(fieldlens[12],12);
	assertEquals(fieldlens[13],1);
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
	assertEquals(cur->getColumnName((uint32_t)0),"testint");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnType((uint32_t)0),"INT");
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
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	delete[] socket;
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
	assertEquals(cur->colCount(),14);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testsmallint");
	assertEquals(cur->getColumnName(2),"testtinyint");
	assertEquals(cur->getColumnName(3),"testreal");
	assertEquals(cur->getColumnName(4),"testfloat");
	assertEquals(cur->getColumnName(5),"testdecimal");
	assertEquals(cur->getColumnName(6),"testnumeric");
	assertEquals(cur->getColumnName(7),"testmoney");
	assertEquals(cur->getColumnName(8),"testsmallmoney");
	assertEquals(cur->getColumnName(9),"testdatetime");
	assertEquals(cur->getColumnName(10),"testsmalldatetime");
	assertEquals(cur->getColumnName(11),"testchar");
	assertEquals(cur->getColumnName(12),"testvarchar");
	assertEquals(cur->getColumnName(13),"testbit");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testsmallint");
	assertEquals(cols[2],"testtinyint");
	assertEquals(cols[3],"testreal");
	assertEquals(cols[4],"testfloat");
	assertEquals(cols[5],"testdecimal");
	assertEquals(cols[6],"testnumeric");
	assertEquals(cols[7],"testmoney");
	assertEquals(cols[8],"testsmallmoney");
	assertEquals(cols[9],"testdatetime");
	assertEquals(cols[10],"testsmalldatetime");
	assertEquals(cols[11],"testchar");
	assertEquals(cols[12],"testvarchar");
	assertEquals(cols[13],"testbit");
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
	// can't do this with freetds
	//cur->setResultSetBufferSize(1);
	assertTrue(cur->sendQuery("select * from testtable"));
	for (uint32_t i=0; cur->getRow(i); i++) {
		secondcur=new sqlrcursor(con);
		secondcur->setResultSetBufferSize(1);
		assertTrue(secondcur->sendQuery("select * from testtable"));
		delete secondcur;
		secondcur=NULL;
	}
	//cur->setResultSetBufferSize(0);
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
	assertTrue(con->begin());
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
		"	10.1, "
		"	10.00, "
		"	10.00, "
		"	'01-Jan-2010 10:00:00', "
		"	'01-Jan-2010 10:00:00', "
		"	'testchar10', "
		"	'testvarchar10', "
		"	10)"));
	assertTrue(con->rollback());
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
		"	10.1, "
		"	10.00, "
		"	10.00, "
		"	'01-Jan-2010 10:00:00', "
		"	'01-Jan-2010 10:00:00', "
		"	'testchar10', "
		"	'testvarchar10', "
		"	10)"));
	assertTrue(con->commit());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),'$(var2)',$(var3)");
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
	cur->prepareQuery("select $(var1),$(var2),$(var3)");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3");
	stdoutput.printf("\n");
	cur->prepareQuery("select '$(var1)','$(var2)','$(var3)'");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	stdoutput.printf("\n");
	cur->prepareQuery("select $(var1),$(var2),$(var3)");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// nulls as nulls
	stdoutput.printf("NULLS AS NULLS: \n");
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
	stdoutput.printf("\n");



	// null and empty lobs
	stdoutput.printf("NULL AND EMPTY LOBS: \n");
	cur->getNullsAsNulls();
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob1 text NULL, "
		"	testclob2 text NULL, "
		"	testblob1 image NULL, "
		"	testblob2 image NULL)"));
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
	// sap converts empty strings to a single space.  It's possible that
	// if we had true input bind support on the backend, then this would
	// work correctly, but for now we're faking binds, and inserting an
	// empty string, so we have to check for a single space here.
	assertEquals(cur->getField(0,(uint32_t)0)," ");
	assertEquals(cur->getField(0,1),NULL);
	// sap doesn't really support inserting an empty string into a binary
	// column.  The minimum that can be inserted is a single \0.  That ends
	// up being interpreted as an empty string here, but it's actualy a
	// single \0 character, not zero characters.
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
		"	testclob text, "
		"	testblob image) lock datarows");
	cur->prepareQuery("insert into testtable values (?,?)");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
	cur->inputBindBlob("2",largebuffer,LARGE_BUFFER_LENGTH);
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
	// FreeTDS needs to support cursors for this to work
	#if 0
	stdoutput.printf("OUTPUT BIND BY POSITION: \n");
	cur->sendQuery("drop procedure testproc");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery(
		"create procedure testproc "
		"	@out1 int output, "
		"	@out2 varchar(20) output, "
		"	@out3 float output, "
		"	@out4 datetime output, "
		"	@out5 varchar(20) output as "
		"select @out1=1, "
		"	@out2='hello', "
		"	@out3=2.5, "
		"	@out4='2001-02-03', "
		"	@out5=null"));
	cur->prepareQuery("exec testproc");
	assertEquals(cur->countBindVariables(),0);
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
	stdoutput.printf("\n");
	#endif


	// output bind by name
	// FreeTDS needs to support cursors for this to work
	#if 0
	stdoutput.printf("OUTPUT BIND BY NAME: \n");
	cur->sendQuery("drop procedure testproc");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery(
		"create procedure testproc "
		"	@out1 int output, "
		"	@out2 varchar(20) output, "
		"	@out3 float output, "
		"	@out4 datetime output, "
		"	@out5 varchar(20) output as "
		"select @out1=1, "
		"	@out2='hello', "
		"	@out3=2.5, "
		"	@out4='2001-02-03', "
		"	@out5=null"));
	cur->prepareQuery("exec testproc");
	assertEquals(cur->countBindVariables(),0);
	cur->defineOutputBindInteger("out1");
	cur->defineOutputBindString("out2",20);
	cur->defineOutputBindDouble("out3");
	cur->defineOutputBindDate("out4");
	cur->defineOutputBindString("out5",20);
	assertTrue(cur->executeQuery());
	numvar=cur->getOutputBindInteger("out1");
	stringvar=cur->getOutputBindString("out2");
	floatvar=cur->getOutputBindDouble("out3");
	cur->getOutputBindDate("out4",&year,&month,&day,
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
	nullvar=cur->getOutputBindString("out5");
	assertEquals(nullvar,NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");
	#endif


	// output bind by name with validation
	// Even if FreeTDS supported cursors...
        // validateBinds() can't be used for output binds, with sap.  In sap,
        // when executing a procedure, you don't declare any bind variable
        // delimiters in the query.  eg, you just do: "exec testproc", not
        // "exec testproc(@out1,@out2)".  If you call validateBinds(), it won't
        // find any binds in the query, and will filter out any binds that you
        // declare.


	// lob output bind
	// sap doesn't support lobs as output parameters to stored procedures,
	// and there's no way to directly select into a lob bind variable


	// long output bind
	// FreeTDS needs to support cursors for this to work
	#if 0
	stdoutput.printf("LONG OUTPUT BIND: \n");
	cur->sendQuery("drop procedure testproc");
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	char	query[LARGE_BUFFER_LENGTH+256];
	charstring::printf(query,sizeof(query),
		"create procedure testproc "
		"@bindval varchar(%d) output as "
		"set @bindval='%s'",LARGE_BUFFER_LENGTH,largebuffer);
	assertTrue(cur->sendQuery(query));
	cur->prepareQuery("exec testproc");
	cur->defineOutputBindString("bindval",LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("bindval"),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getOutputBindString("bindval"),largebuffer);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");
	#endif


	// negative input bind
	stdoutput.printf("NEGATIVE INPUT BIND: \n");
	cur->sendQuery("drop table testtable");
	cur->sendQuery("create table testtable (testval int)");
	cur->prepareQuery("insert into testtable values (@testval)");
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
	cur->substitution("var1","@var1");
	assertTrue(cur->validBind("var1"));
	assertFalse(cur->validBind("var2"));
	assertFalse(cur->validBind("var3"));
	assertFalse(cur->validBind("var4"));
	stdoutput.printf("\n");
	cur->substitution("var2","@var2");
	assertTrue(cur->validBind("var1"));
	assertTrue(cur->validBind("var2"));
	assertFalse(cur->validBind("var3"));
	assertFalse(cur->validBind("var4"));
	stdoutput.printf("\n");
	cur->substitution("var3","@var3");
	assertTrue(cur->validBind("var1"));
	assertTrue(cur->validBind("var2"));
	assertTrue(cur->validBind("var3"));
	assertFalse(cur->validBind("var4"));
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// rebinding
	// FreeTDS needs to support cursors for this to work
	#if 0
	stdoutput.printf("REBINDING: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc "
		"	@in1 int, "
		"	@out1 int output as "
		"select @out1=@in1"));
	cur->prepareQuery("exec testproc");
	cur->inputBind("in1",1);
	cur->defineOutputBindInteger("out1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out1"),1);
	cur->inputBind("in1",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out1"),2);
	cur->inputBind("in1",3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out1"),3);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");
	#endif


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
	cur->prepareQuery("select cast(? as int)");
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
	// FreeTDS needs to support cursors for this to work
	#if 0
	stdoutput.printf("STORED PROCEDURE RETURNING NO VALUE: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc "
		"	@in1 int, "
		"	@in2 float, "
		"	@in3 varchar(20) as "
		"return"));
	cur->prepareQuery("exec testproc");
	cur->inputBind("in1",1);
	cur->inputBind("in2",1.1,2,1);
	cur->inputBind("in3","hello");
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");
	#endif


	// stored procedure returning single value
	// FreeTDS needs to support cursors for this to work
	#if 0
	stdoutput.printf("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc "
		"	@in1 int, "
		"	@in2 float, "
		"	@in3 varchar(20), "
		"	@out1 int output as "
		"select @out1=@in1"));
	cur->prepareQuery("exec testproc");
	cur->inputBind("in1",1);
	cur->inputBind("in2",1.1,2,1);
	cur->inputBind("in3","hello");
	cur->defineOutputBindInteger("out1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("out1"),1);
	assertTrue(cur->sendQuery("drop procedure testproc"));
	stdoutput.printf("\n");
	#endif


	// stored procedure returning multiple values
	// FreeTDS needs to support cursors for this to work
	#if 0
	stdoutput.printf("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	cur->sendQuery("drop procedure testproc");
	assertTrue(cur->sendQuery(
		"create procedure testproc @in1 int, "
		"       @in2 float, "
		"       @in3 varchar(20), "
		"       @out1 int output, "
		"       @out2 float output, "
		"       @out3 varchar(20) output as "
		"select @out1=@in1, "
		"       @out2=@in2, "
		"       @out3=@in3"));
	cur->prepareQuery("exec testproc");
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
	#endif


	// stored procedure returning result set
	stdoutput.printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	cur->sendQuery("drop procedure testselectproc");
	assertTrue(cur->sendQuery(
		"create procedure testselectproc as "
		"       select 1 "
		"       union "
		"       select 2 "
		"       union "
		"       select 3 "
		"       union "
		"       select 4 "
		"       union "
		"       select 5 "
		"       union "
		"       select 6 "
		"       union "
		"       select 7 "
		"       union "
		"       select 8"));
	assertTrue(cur->sendQuery("exec testselectproc"));
	assertEquals(cur->rowCount(),8);
	assertTrue(cur->sendQuery("drop procedure testselectproc"));
	stdoutput.printf("\n");


	// temporary tables
	stdoutput.printf("TEMPORARY TABLES: \n");
	cur->sendQuery("drop table #temptable");
	cur->sendQuery("create table #temptable (col1 int)");
	assertTrue(cur->sendQuery("insert into #temptable values (1)"));
	assertTrue(cur->sendQuery("select count(*) from #temptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("select count(*) from #temptable"));
	stdoutput.printf("\n");


	// encoded binary data
	stdoutput.printf("ENCODED BINARY DATA: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery("create table testtable (col1 image)"));
	byte_t	buffer[256];
	for (uint16_t i=0; i<256; i++) {
		buffer[i]=i;
	}
	stringbuffer	query;
	query.append("insert into testtable values (0x");
	char	hex[3];
	for (uint64_t i=0; i<sizeof(buffer); i++) {
		charstring::printf(hex,sizeof(hex),"%02x",buffer[i]);
		query.append(hex);
	}
	query.append(")");
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
			"	(col1 int identity primary key, "
			"	col2 int)"));
	assertTrue(cur->sendQuery(
			"insert into testtable (col2) values (1)"));
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
	assertTrue(cur->rowCount()>0);
	stdoutput.printf("\n");


	// schema list
	stdoutput.printf("SCHEMA LIST: \n");
	cur->sendQuery("drop table testtable");
	// the get schema list query that is used with sap will only return the
	// names of schemas that have at least one database object in them, so
	// to be sure that there is one, we'll create a table
	assertTrue(cur->sendQuery("create table testtable (col1 int)"));
	assertTrue(cur->getSchemaList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	assertTrue(cur->rowCount()>0);
	assertTrue(cur->sendQuery("drop table testtable"));
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
	assertTrue(cur->sendQuery("drop table testtable1"));
	assertTrue(cur->sendQuery("drop table testtable2"));
	assertTrue(cur->sendQuery("drop table testtable3"));
	assertTrue(cur->sendQuery("drop table testtable4"));
	stdoutput.printf("\n");


	// type info list
	stdoutput.printf("TYPE INFO LIST: \n");
	assertTrue(cur->getTypeInfoList("int"));
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
	assertEquals(cur->getField(0,"type_name"),"INT");
	assertEquals(cur->getField(0,"data_type"),"4");
	assertEquals(cur->getField(0,"precision"),"10");
	assertEquals(cur->getField(0,"local_type_name"),"INT");
	assertTrue(cur->getTypeInfoList("char"));
	assertEquals(cur->getField(0,"type_name"),"CHAR");
	assertEquals(cur->getField(0,"data_type"),"1");
	assertEquals(cur->getField(0,"precision"),"8000");
	assertEquals(cur->getField(0,"local_type_name"),"CHAR");
	assertTrue(cur->getTypeInfoList("varchar"));
	assertEquals(cur->getField(0,"type_name"),"VARCHAR");
	assertEquals(cur->getField(0,"data_type"),"12");
	assertEquals(cur->getField(0,"precision"),"8000");
	assertEquals(cur->getField(0,"local_type_name"),"VARCHAR");
	assertTrue(cur->getTypeInfoList("datetime"));
	assertEquals(cur->getField(0,"type_name"),"DATETIME");
	assertEquals(cur->getField(0,"data_type"),"93");
	assertEquals(cur->getField(0,"precision"),"23");
	assertEquals(cur->getField(0,"local_type_name"),"DATETIME");
	stdoutput.printf("\n");


	// column list
	stdoutput.printf("COLUMN LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testsmallint smallint, "
		"	testtinyint tinyint, "
		"	testreal real, "
		"	testfloat float, "
		"	testdecimal decimal(4,1), "
		"	testnumeric numeric(4,1), "
		"	testmoney money, "
		"	testsmallmoney smallmoney, "
		"	testdatetime datetime, "
		"	testsmalldatetime smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit)"));
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
			cur->getField(0,"column_name"),"testint"));
	assertTrue(!charstring::compare(
			cur->getField(1,"column_name"),"testsmallint"));
	assertTrue(!charstring::compare(
			cur->getField(2,"column_name"),"testtinyint"));
	assertTrue(!charstring::compare(
			cur->getField(3,"column_name"),"testreal"));
	assertTrue(!charstring::compare(
			cur->getField(4,"column_name"),"testfloat"));
	assertTrue(!charstring::compare(
			cur->getField(5,"column_name"),"testdecimal"));
	assertTrue(!charstring::compare(
			cur->getField(6,"column_name"),"testnumeric"));
	assertTrue(!charstring::compare(
			cur->getField(7,"column_name"),"testmoney"));
	assertTrue(!charstring::compare(
			cur->getField(8,"column_name"),"testsmallmoney"));
	assertTrue(!charstring::compare(
			cur->getField(9,"column_name"),"testdatetime"));
	assertTrue(!charstring::compare(
			cur->getField(10,"column_name"),"testsmalldatetime"));
	assertTrue(!charstring::compare(
			cur->getField(11,"column_name"),"testchar"));
	assertTrue(!charstring::compare(
			cur->getField(12,"column_name"),"testvarchar"));
	assertTrue(!charstring::compare(
			cur->getField(13,"column_name"),"testbit"));
	assertTrue(!charstring::compare(
			cur->getField(0,"data_type"),"int"));
	assertTrue(!charstring::compare(
			cur->getField(1,"data_type"),"smallint"));
	assertTrue(!charstring::compare(
			cur->getField(2,"data_type"),"tinyint"));
	assertTrue(!charstring::compare(
			cur->getField(3,"data_type"),"real"));
	assertTrue(!charstring::compare(
			cur->getField(4,"data_type"),"float"));
	assertTrue(!charstring::compare(
			cur->getField(5,"data_type"),"decimal"));
	assertTrue(!charstring::compare(
			cur->getField(6,"data_type"),"numeric"));
	assertTrue(!charstring::compare(
			cur->getField(7,"data_type"),"money"));
	assertTrue(!charstring::compare(
			cur->getField(8,"data_type"),"smallmoney"));
	assertTrue(!charstring::compare(
			cur->getField(9,"data_type"),"datetime"));
	assertTrue(!charstring::compare(
			cur->getField(10,"data_type"),"smalldatetime"));
	assertTrue(!charstring::compare(
			cur->getField(11,"data_type"),"char"));
	assertTrue(!charstring::compare(
			cur->getField(12,"data_type"),"varchar"));
	assertTrue(!charstring::compare(
			cur->getField(13,"data_type"),"bit"));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int identity primary key, "
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
	assertTrue(cur->sendQuery("drop table testtable"));
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
	assertEquals(cur->getField(0,"non_unique"),"0");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compare(cur->getField(0,"column_name"),"col1"));
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"1");
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
		"create procedure testproc1 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
	assertTrue(cur->sendQuery(
		"create procedure testproc2 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
	assertTrue(cur->sendQuery(
		"create procedure testproc3 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
	assertTrue(cur->sendQuery(
		"create procedure testproc4 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
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
	assertEquals(cur->getField(0,"parameter_name"),"@in1");
	assertEquals(cur->getField(0,"parameter_mode"),"1");
	assertEquals(cur->getField(0,"data_type"),"int");
	assertEquals(cur->getField(0,"ordinal_position"),"1");
	assertEquals(cur->getField(1,"parameter_name"),"@in2");
	assertEquals(cur->getField(1,"parameter_mode"),"1");
	assertEquals(cur->getField(1,"data_type"),"char");
	assertEquals(cur->getField(1,"ordinal_position"),"2");
	assertEquals(cur->getField(2,"parameter_name"),"@in3");
	assertEquals(cur->getField(2,"parameter_mode"),"1");
	assertEquals(cur->getField(2,"data_type"),"varchar");
	assertEquals(cur->getField(2,"ordinal_position"),"3");
	assertEquals(cur->getField(3,"parameter_name"),"@in4");
	assertEquals(cur->getField(3,"parameter_mode"),"1");
	assertEquals(cur->getField(3,"data_type"),"datetime");
	assertEquals(cur->getField(3,"ordinal_position"),"4");
	assertTrue(cur->sendQuery("drop procedure testproc1"));
	assertTrue(cur->sendQuery("drop procedure testproc2"));
	assertTrue(cur->sendQuery("drop procedure testproc3"));
	assertTrue(cur->sendQuery("drop procedure testproc4"));
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
