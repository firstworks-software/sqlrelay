#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.


use SQLRelay::Connection;
use SQLRelay::Cursor;
use Sys::Hostname;
require "./asserts.pl";


@isolationlevels=("1","0","2","3");
@subvars=("var1","var2","var3");
@subvallongs=(1,2,3);
@subvalstrings=("hi","hello","bye");
@subvaldoubles=(10.55,10.556,10.5556);
@precs=(4,5,6);
@scales=(2,3,4);
$counter=0;

$LARGE_BUFFER_LENGTH=8192;


# hostname
$hostname=hostname();
$hostname=~s/\..*//;
$dumptran="dump tran $hostname with truncate_only";


# instantiation
$con=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$cur=SQLRelay::Cursor->new($con);


# identify
print("IDENTIFY: \n");
assertEquals($con->identify(),"freetds");
print("\n");


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");


# transaction state
print("TRANSACTION STATE: \n");
assertEquals($con->getDefaultTransactionModel(),"explicit-error");
assertEquals($con->getTransactionModel(),"explicit-error");
assertFalse($con->getInTransaction());
assertTrue($con->getAutoCommit());
print("\n");


# bind format
print("BIND FORMAT: \n");
assertEquals($con->bindFormat(),"\@*");
print("\n");


# nextval format
print("NEXTVAL FORMAT: \n");
assertEquals($con->nextvalFormat(),"%s.nextval");
print("\n");


# isolation levels
print("ISOLATION LEVELS: \n");
foreach $il (@isolationlevels) {
	assertTrue($con->setIsolationLevel($il));
	assertEquals($con->getIsolationLevel(),$il);
	print("\n");
}
# reset to the default isolation level
assertTrue($con->setIsolationLevel($isolationlevels[0]));
print("\n");


# create testtable
print("CREATE TESTTABLE: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery($dumptran);
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testint int, ".
	"	testsmallint smallint, ".
	"	testtinyint tinyint, ".
	"	testreal real, ".
	"	testfloat float, ".
	"	testdecimal decimal(4,1), ".
	"	testnumeric numeric(4,1), ".
	"	testmoney money, ".
	"	testsmallmoney smallmoney, ".
	"	testdatetime datetime, ".
	"	testsmalldatetime smalldatetime, ".
	"	testchar char(40), ".
	"	testvarchar varchar(40), ".
	"	testbit bit) lock datarows"));
print("\n");


# insert
print("INSERT: \n");
assertTrue($con->begin());
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	1, ".
	"	1, ".
	"	1, ".
	"	1.5, ".
	"	1.5, ".
	"	1.5, ".
	"	1.5, ".
	"	1.00, ".
	"	1.00, ".
	"	'01-Jan-2001 01:00:00', ".
	"	'01-Jan-2001 01:00:00', ".
	"	'testchar1', ".
	"	'testvarchar1', ".
	"	1)"));
print("\n");


# affected rows
print("AFFECTED ROWS: \n");
assertEquals($cur->affectedRows(),1);
print("\n");


# input bind by position
print("INPUT BIND BY POSITION: \n");
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?)");
assertEquals($cur->countBindVariables(),14);
$cur->inputBind("1",2);
$cur->inputBind("2",2);
$cur->inputBind("3",2);
$cur->inputBind("4",2.5,2,1);
$cur->inputBind("5",2.5,2,1);
$cur->inputBind("6",2.5,2,1);
$cur->inputBind("7",2.5,2,1);
$cur->inputBind("8",2.00,3,2);
$cur->inputBind("9",2.00,3,2);
$cur->inputBind("10","01-Jan-2002 02:00:00");
$cur->inputBind("11","01-Jan-2002 02:00:00");
$cur->inputBind("12","testchar2");
$cur->inputBind("13","testvarchar2");
$cur->inputBind("14",1);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",3);
$cur->inputBind("2",3);
$cur->inputBind("3",3);
$cur->inputBind("4",3.5,2,1);
$cur->inputBind("5",3.5,2,1);
$cur->inputBind("6",3.5,2,1);
$cur->inputBind("7",3.5,2,1);
$cur->inputBind("8",3.00,3,2);
$cur->inputBind("9",3.00,3,2);
$cur->inputBind("10","01-Jan-2003 03:00:00");
$cur->inputBind("11","01-Jan-2003 03:00:00");
$cur->inputBind("12","testchar3");
$cur->inputBind("13","testvarchar3");
$cur->inputBind("14",1);
assertTrue($cur->executeQuery());
print("\n");


# array of input binds by position
# freetds doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by position with validation
print("INPUT BIND BY POSITION WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("1",4);
$cur->inputBind("2",4);
$cur->inputBind("3",4);
$cur->inputBind("4",4.5,2,1);
$cur->inputBind("5",4.5,2,1);
$cur->inputBind("6",4.5,2,1);
$cur->inputBind("7",4.5,2,1);
$cur->inputBind("8",4.00,3,2);
$cur->inputBind("9",4.00,3,2);
$cur->inputBind("10","01-Jan-2004 04:00:00");
$cur->inputBind("11","01-Jan-2004 04:00:00");
$cur->inputBind("12","testchar4");
$cur->inputBind("13","testvarchar4");
$cur->inputBind("14",1);
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# input bind by name
print("INPUT BIND BY NAME: \n");
$cur->clearBinds();
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	\@var1, ".
	"	\@var2, ".
	"	\@var3, ".
	"	\@var4, ".
	"	\@var5, ".
	"	\@var6, ".
	"	\@var7, ".
	"	\@var8, ".
	"	\@var9, ".
	"	\@var10, ".
	"	\@var11, ".
	"	\@var12, ".
	"	\@var13, ".
	"	\@var14)");
assertEquals($cur->countBindVariables(),14);
$cur->inputBind("var1",5);
$cur->inputBind("var2",5);
$cur->inputBind("var3",5);
$cur->inputBind("var4",5.5,2,1);
$cur->inputBind("var5",5.5,2,1);
$cur->inputBind("var6",5.5,2,1);
$cur->inputBind("var7",5.5,2,1);
$cur->inputBind("var8",5.00,3,2);
$cur->inputBind("var9",5.00,3,2);
$cur->inputBind("var10","01-Jan-2005 05:00:00");
$cur->inputBind("var11","01-Jan-2005 05:00:00");
$cur->inputBind("var12","testchar5");
$cur->inputBind("var13","testvarchar5");
$cur->inputBind("var14",1);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("var1",6);
$cur->inputBind("var2",6);
$cur->inputBind("var3",6);
$cur->inputBind("var4",6.5,2,1);
$cur->inputBind("var5",6.5,2,1);
$cur->inputBind("var6",6.5,2,1);
$cur->inputBind("var7",6.5,2,1);
$cur->inputBind("var8",6.00,3,2);
$cur->inputBind("var9",6.00,3,2);
$cur->inputBind("var10","01-Jan-2006 06:00:00");
$cur->inputBind("var11","01-Jan-2006 06:00:00");
$cur->inputBind("var12","testchar6");
$cur->inputBind("var13","testvarchar6");
$cur->inputBind("var14",1);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("var1",7);
$cur->inputBind("var2",7);
$cur->inputBind("var3",7);
$cur->inputBind("var4",7.5,2,1);
$cur->inputBind("var5",7.5,2,1);
$cur->inputBind("var6",7.5,2,1);
$cur->inputBind("var7",7.5,2,1);
$cur->inputBind("var8",7.00,3,2);
$cur->inputBind("var9",7.00,3,2);
$cur->inputBind("var10","01-Jan-2007 07:00:00");
$cur->inputBind("var11","01-Jan-2007 07:00:00");
$cur->inputBind("var12","testchar7");
$cur->inputBind("var13","testvarchar7");
$cur->inputBind("var14",1);
assertTrue($cur->executeQuery());
print("\n");


# array of input binds by name
# freetds doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by name with validation
print("INPUT BIND BY NAME WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("var1",8);
$cur->inputBind("var2",8);
$cur->inputBind("var3",8);
$cur->inputBind("var4",8.5,2,1);
$cur->inputBind("var5",8.5,2,1);
$cur->inputBind("var6",8.5,2,1);
$cur->inputBind("var7",8.5,2,1);
$cur->inputBind("var8",8.00,3,2);
$cur->inputBind("var9",8.00,3,2);
$cur->inputBind("var10","01-Jan-2008 08:00:00");
$cur->inputBind("var11","01-Jan-2008 08:00:00");
$cur->inputBind("var12","testchar8");
$cur->inputBind("var13","testvarchar8");
$cur->inputBind("var14",1);
$cur->inputBind("var15","junkvalue");
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# select
print("SELECT: \n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEquals($cur->colCount(),14);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEquals($cur->getColumnName(0),"testint");
assertEquals($cur->getColumnName(1),"testsmallint");
assertEquals($cur->getColumnName(2),"testtinyint");
assertEquals($cur->getColumnName(3),"testreal");
assertEquals($cur->getColumnName(4),"testfloat");
assertEquals($cur->getColumnName(5),"testdecimal");
assertEquals($cur->getColumnName(6),"testnumeric");
assertEquals($cur->getColumnName(7),"testmoney");
assertEquals($cur->getColumnName(8),"testsmallmoney");
assertEquals($cur->getColumnName(9),"testdatetime");
assertEquals($cur->getColumnName(10),"testsmalldatetime");
assertEquals($cur->getColumnName(11),"testchar");
assertEquals($cur->getColumnName(12),"testvarchar");
assertEquals($cur->getColumnName(13),"testbit");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"testint");
assertEquals($cols[1],"testsmallint");
assertEquals($cols[2],"testtinyint");
assertEquals($cols[3],"testreal");
assertEquals($cols[4],"testfloat");
assertEquals($cols[5],"testdecimal");
assertEquals($cols[6],"testnumeric");
assertEquals($cols[7],"testmoney");
assertEquals($cols[8],"testsmallmoney");
assertEquals($cols[9],"testdatetime");
assertEquals($cols[10],"testsmalldatetime");
assertEquals($cols[11],"testchar");
assertEquals($cols[12],"testvarchar");
assertEquals($cols[13],"testbit");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEquals($cur->getColumnType(0),"INT");
assertEquals($cur->getColumnType("testint"),"INT");
assertEquals($cur->getColumnType(1),"SMALLINT");
assertEquals($cur->getColumnType("testsmallint"),"SMALLINT");
assertEquals($cur->getColumnType(2),"TINYINT");
assertEquals($cur->getColumnType("testtinyint"),"TINYINT");
assertEquals($cur->getColumnType(3),"REAL");
assertEquals($cur->getColumnType("testreal"),"REAL");
assertEquals($cur->getColumnType(4),"FLOAT");
assertEquals($cur->getColumnType("testfloat"),"FLOAT");
assertEquals($cur->getColumnType(5),"DECIMAL");
assertEquals($cur->getColumnType("testdecimal"),"DECIMAL");
assertEquals($cur->getColumnType(6),"NUMERIC");
assertEquals($cur->getColumnType("testnumeric"),"NUMERIC");
assertEquals($cur->getColumnType(7),"MONEY");
assertEquals($cur->getColumnType("testmoney"),"MONEY");
assertEquals($cur->getColumnType(8),"SMALLMONEY");
assertEquals($cur->getColumnType("testsmallmoney"),"SMALLMONEY");
assertEquals($cur->getColumnType(9),"DATETIME");
assertEquals($cur->getColumnType("testdatetime"),"DATETIME");
assertEquals($cur->getColumnType(10),"SMALLDATETIME");
assertEquals($cur->getColumnType("testsmalldatetime"),"SMALLDATETIME");
assertEquals($cur->getColumnType(11),"CHAR");
assertEquals($cur->getColumnType("testchar"),"CHAR");
assertEquals($cur->getColumnType(12),"CHAR");
assertEquals($cur->getColumnType("testvarchar"),"CHAR");
assertEquals($cur->getColumnType(13),"BIT");
assertEquals($cur->getColumnType("testbit"),"BIT");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEquals($cur->getColumnLength(0),4);
assertEquals($cur->getColumnLength("testint"),4);
assertEquals($cur->getColumnLength(1),2);
assertEquals($cur->getColumnLength("testsmallint"),2);
assertEquals($cur->getColumnLength(2),1);
assertEquals($cur->getColumnLength("testtinyint"),1);
assertEquals($cur->getColumnLength(3),4);
assertEquals($cur->getColumnLength("testreal"),4);
assertEquals($cur->getColumnLength(4),8);
assertEquals($cur->getColumnLength("testfloat"),8);
# freetds reports the decimal/numeric display length as 35
assertEquals($cur->getColumnLength(5),35);
assertEquals($cur->getColumnLength("testdecimal"),35);
assertEquals($cur->getColumnLength(6),35);
assertEquals($cur->getColumnLength("testnumeric"),35);
assertEquals($cur->getColumnLength(7),8);
assertEquals($cur->getColumnLength("testmoney"),8);
assertEquals($cur->getColumnLength(8),4);
assertEquals($cur->getColumnLength("testsmallmoney"),4);
assertEquals($cur->getColumnLength(9),8);
assertEquals($cur->getColumnLength("testdatetime"),8);
assertEquals($cur->getColumnLength(10),4);
assertEquals($cur->getColumnLength("testsmalldatetime"),4);
# char(40)/varchar(40) report the declared length 40 (not multiplied)
assertEquals($cur->getColumnLength(11),40);
assertEquals($cur->getColumnLength("testchar"),40);
assertEquals($cur->getColumnLength(12),40);
assertEquals($cur->getColumnLength("testvarchar"),40);
assertEquals($cur->getColumnLength(13),1);
assertEquals($cur->getColumnLength("testbit"),1);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEquals($cur->getLongest(0),1);
assertEquals($cur->getLongest("testint"),1);
assertEquals($cur->getLongest(1),1);
assertEquals($cur->getLongest("testsmallint"),1);
assertEquals($cur->getLongest(2),1);
assertEquals($cur->getLongest("testtinyint"),1);
assertEquals($cur->getLongest(3),3);
assertEquals($cur->getLongest("testreal"),3);
assertEquals($cur->getLongest(4),3);
assertEquals($cur->getLongest("testfloat"),3);
assertEquals($cur->getLongest(5),3);
assertEquals($cur->getLongest("testdecimal"),3);
assertEquals($cur->getLongest(6),3);
assertEquals($cur->getLongest("testnumeric"),3);
assertEquals($cur->getLongest(7),6);
assertEquals($cur->getLongest("testmoney"),6);
assertEquals($cur->getLongest(8),6);
assertEquals($cur->getLongest("testsmallmoney"),6);
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getLongest(9),26);
assertEquals($cur->getLongest("testdatetime"),26);
assertEquals($cur->getLongest(10),26);
assertEquals($cur->getLongest("testsmalldatetime"),26);
assertEquals($cur->getLongest(11),40);
assertEquals($cur->getLongest("testchar"),40);
assertEquals($cur->getLongest(12),12);
assertEquals($cur->getLongest("testvarchar"),12);
assertEquals($cur->getLongest(13),1);
assertEquals($cur->getLongest("testbit"),1);
print("\n");


# row count
print("ROW COUNT: \n");
assertEquals($cur->rowCount(),8);
print("\n");


# total rows
print("TOTAL ROWS: \n");
assertEquals($cur->totalRows(),0);
print("\n");


# first row index
print("FIRST ROW INDEX: \n");
assertEquals($cur->firstRowIndex(),0);
print("\n");


# end of result set
print("END OF RESULT SET: \n");
assertTrue($cur->endOfResultSet());
print("\n");


# fields by index
print("FIELDS BY INDEX: \n");
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"1");
assertEquals($cur->getField(0,2),"1");
assertEquals($cur->getField(0,3),"1.5");
assertEquals($cur->getField(0,4),"1.5");
assertEquals($cur->getField(0,5),"1.5");
assertEquals($cur->getField(0,6),"1.5");
assertEquals($cur->getField(0,7),"1.0000");
assertEquals($cur->getField(0,8),"1.0000");
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getField(0,9),"Jan  1 2001 01:00:00:000AM");
assertEquals($cur->getField(0,10),"Jan  1 2001 01:00:00:000AM");
assertEquals($cur->getField(0,11),"testchar1                               ");
assertEquals($cur->getField(0,12),"testvarchar1");
assertEquals($cur->getField(0,13),"1");
print("\n");
assertEquals($cur->getField(7,0),"8");
assertEquals($cur->getField(7,1),"8");
assertEquals($cur->getField(7,2),"8");
assertEquals($cur->getField(7,3),"8.5");
assertEquals($cur->getField(7,4),"8.5");
assertEquals($cur->getField(7,5),"8.5");
assertEquals($cur->getField(7,6),"8.5");
assertEquals($cur->getField(7,7),"8.0000");
assertEquals($cur->getField(7,8),"8.0000");
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getField(7,9),"Jan  1 2008 08:00:00:000AM");
assertEquals($cur->getField(7,10),"Jan  1 2008 08:00:00:000AM");
assertEquals($cur->getField(7,11),"testchar8                               ");
assertEquals($cur->getField(7,12),"testvarchar8");
assertEquals($cur->getField(7,13),"1");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEquals($cur->getFieldLength(0,0),1);
assertEquals($cur->getFieldLength(0,1),1);
assertEquals($cur->getFieldLength(0,2),1);
assertEquals($cur->getFieldLength(0,3),3);
assertEquals($cur->getFieldLength(0,4),3);
assertEquals($cur->getFieldLength(0,5),3);
assertEquals($cur->getFieldLength(0,6),3);
assertEquals($cur->getFieldLength(0,7),6);
assertEquals($cur->getFieldLength(0,8),6);
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getFieldLength(0,9),26);
assertEquals($cur->getFieldLength(0,10),26);
assertEquals($cur->getFieldLength(0,11),40);
assertEquals($cur->getFieldLength(0,12),12);
assertEquals($cur->getFieldLength(0,13),1);
print("\n");
assertEquals($cur->getFieldLength(7,0),1);
assertEquals($cur->getFieldLength(7,1),1);
assertEquals($cur->getFieldLength(7,2),1);
assertEquals($cur->getFieldLength(7,3),3);
assertEquals($cur->getFieldLength(7,4),3);
assertEquals($cur->getFieldLength(7,5),3);
assertEquals($cur->getFieldLength(7,6),3);
assertEquals($cur->getFieldLength(7,7),6);
assertEquals($cur->getFieldLength(7,8),6);
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getFieldLength(7,9),26);
assertEquals($cur->getFieldLength(7,10),26);
assertEquals($cur->getFieldLength(7,11),40);
assertEquals($cur->getFieldLength(7,12),12);
assertEquals($cur->getFieldLength(7,13),1);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEquals($cur->getField(0,"testint"),"1");
assertEquals($cur->getField(0,"testsmallint"),"1");
assertEquals($cur->getField(0,"testtinyint"),"1");
assertEquals($cur->getField(0,"testreal"),"1.5");
assertEquals($cur->getField(0,"testfloat"),"1.5");
assertEquals($cur->getField(0,"testdecimal"),"1.5");
assertEquals($cur->getField(0,"testnumeric"),"1.5");
assertEquals($cur->getField(0,"testmoney"),"1.0000");
assertEquals($cur->getField(0,"testsmallmoney"),"1.0000");
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getField(0,"testdatetime"),"Jan  1 2001 01:00:00:000AM");
assertEquals($cur->getField(0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM");
assertEquals($cur->getField(0,"testchar"),"testchar1                               ");
assertEquals($cur->getField(0,"testvarchar"),"testvarchar1");
assertEquals($cur->getField(0,"testbit"),"1");
print("\n");
assertEquals($cur->getField(7,"testint"),"8");
assertEquals($cur->getField(7,"testsmallint"),"8");
assertEquals($cur->getField(7,"testtinyint"),"8");
assertEquals($cur->getField(7,"testreal"),"8.5");
assertEquals($cur->getField(7,"testfloat"),"8.5");
assertEquals($cur->getField(7,"testdecimal"),"8.5");
assertEquals($cur->getField(7,"testnumeric"),"8.5");
assertEquals($cur->getField(7,"testmoney"),"8.0000");
assertEquals($cur->getField(7,"testsmallmoney"),"8.0000");
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getField(7,"testdatetime"),"Jan  1 2008 08:00:00:000AM");
assertEquals($cur->getField(7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM");
assertEquals($cur->getField(7,"testchar"),"testchar8                               ");
assertEquals($cur->getField(7,"testvarchar"),"testvarchar8");
assertEquals($cur->getField(7,"testbit"),"1");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEquals($cur->getFieldLength(0,"testint"),1);
assertEquals($cur->getFieldLength(0,"testsmallint"),1);
assertEquals($cur->getFieldLength(0,"testtinyint"),1);
assertEquals($cur->getFieldLength(0,"testreal"),3);
assertEquals($cur->getFieldLength(0,"testfloat"),3);
assertEquals($cur->getFieldLength(0,"testdecimal"),3);
assertEquals($cur->getFieldLength(0,"testnumeric"),3);
assertEquals($cur->getFieldLength(0,"testmoney"),6);
assertEquals($cur->getFieldLength(0,"testsmallmoney"),6);
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getFieldLength(0,"testdatetime"),26);
assertEquals($cur->getFieldLength(0,"testsmalldatetime"),26);
assertEquals($cur->getFieldLength(0,"testchar"),40);
assertEquals($cur->getFieldLength(0,"testvarchar"),12);
assertEquals($cur->getFieldLength(0,"testbit"),1);
print("\n");
assertEquals($cur->getFieldLength(7,"testint"),1);
assertEquals($cur->getFieldLength(7,"testsmallint"),1);
assertEquals($cur->getFieldLength(7,"testtinyint"),1);
assertEquals($cur->getFieldLength(7,"testreal"),3);
assertEquals($cur->getFieldLength(7,"testfloat"),3);
assertEquals($cur->getFieldLength(7,"testdecimal"),3);
assertEquals($cur->getFieldLength(7,"testnumeric"),3);
assertEquals($cur->getFieldLength(7,"testmoney"),6);
assertEquals($cur->getFieldLength(7,"testsmallmoney"),6);
# freetds datetime rendering for the fixture tds version
assertEquals($cur->getFieldLength(7,"testdatetime"),26);
assertEquals($cur->getFieldLength(7,"testsmalldatetime"),26);
assertEquals($cur->getFieldLength(7,"testchar"),40);
assertEquals($cur->getFieldLength(7,"testvarchar"),12);
assertEquals($cur->getFieldLength(7,"testbit"),1);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEquals($fields[0],"1");
assertEquals($fields[1],"1");
assertEquals($fields[2],"1");
assertEquals($fields[3],"1.5");
assertEquals($fields[4],"1.5");
assertEquals($fields[5],"1.5");
assertEquals($fields[6],"1.5");
assertEquals($fields[7],"1.0000");
assertEquals($fields[8],"1.0000");
# freetds datetime rendering for the fixture tds version
assertEquals($fields[9],"Jan  1 2001 01:00:00:000AM");
assertEquals($fields[10],"Jan  1 2001 01:00:00:000AM");
assertEquals($fields[11],"testchar1                               ");
assertEquals($fields[12],"testvarchar1");
assertEquals($fields[13],"1");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEquals($fieldlens[0],1);
assertEquals($fieldlens[1],1);
assertEquals($fieldlens[2],1);
assertEquals($fieldlens[3],3);
assertEquals($fieldlens[4],3);
assertEquals($fieldlens[5],3);
assertEquals($fieldlens[6],3);
assertEquals($fieldlens[7],6);
assertEquals($fieldlens[8],6);
# freetds datetime rendering for the fixture tds version
assertEquals($fieldlens[9],26);
assertEquals($fieldlens[10],26);
assertEquals($fieldlens[11],40);
assertEquals($fieldlens[12],12);
assertEquals($fieldlens[13],1);
print("\n");


# result set buffer size
print("RESULT SET BUFFER SIZE: \n");
assertEquals($cur->getResultSetBufferSize(),0);
$cur->setResultSetBufferSize(2);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEquals($cur->getResultSetBufferSize(),2);
print("\n");
assertEquals($cur->firstRowIndex(),0);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),2);
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(1,0),"2");
assertEquals($cur->getField(2,0),"3");
print("\n");
assertEquals($cur->firstRowIndex(),2);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),4);
assertEquals($cur->getField(6,0),"7");
assertEquals($cur->getField(7,0),"8");
print("\n");
assertEquals($cur->firstRowIndex(),6);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),8);
assertUndef($cur->getField(8,0));
print("\n");
assertEquals($cur->firstRowIndex(),8);
assertTrue($cur->endOfResultSet());
assertEquals($cur->rowCount(),8);
$cur->setResultSetBufferSize(0);
print("\n");


# dont get column info
print("DONT GET COLUMN INFO: \n");
$cur->dontGetColumnInfo();
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertUndef($cur->getColumnName(0));
assertEquals($cur->getColumnLength(0),0);
assertUndef($cur->getColumnType(0));
$cur->getColumnInfo();
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEquals($cur->getColumnName(0),"testint");
assertEquals($cur->getColumnLength(0),4);
assertEquals($cur->getColumnType(0),"INT");
print("\n");


# suspended session
print("SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
print("\n");
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(1,0),"2");
assertEquals($cur->getField(2,0),"3");
assertEquals($cur->getField(3,0),"4");
assertEquals($cur->getField(4,0),"5");
assertEquals($cur->getField(5,0),"6");
assertEquals($cur->getField(6,0),"7");
assertEquals($cur->getField(7,0),"8");
print("\n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(1,0),"2");
assertEquals($cur->getField(2,0),"3");
assertEquals($cur->getField(3,0),"4");
assertEquals($cur->getField(4,0),"5");
assertEquals($cur->getField(5,0),"6");
assertEquals($cur->getField(6,0),"7");
assertEquals($cur->getField(7,0),"8");
print("\n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
print("\n");
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(1,0),"2");
assertEquals($cur->getField(2,0),"3");
assertEquals($cur->getField(3,0),"4");
assertEquals($cur->getField(4,0),"5");
assertEquals($cur->getField(5,0),"6");
assertEquals($cur->getField(6,0),"7");
assertEquals($cur->getField(7,0),"8");
print("\n");


# suspended result set
print("SUSPENDED RESULT SET: \n");
$cur->setResultSetBufferSize(2);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEquals($cur->getField(2,0),"3");
$id=$cur->getResultSetId();
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
assertTrue($cur->resumeResultSet($id));
print("\n");
assertEquals($cur->firstRowIndex(),4);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),6);
assertEquals($cur->getField(7,0),"8");
print("\n");
assertEquals($cur->firstRowIndex(),6);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),8);
assertUndef($cur->getField(8,0));
print("\n");
assertEquals($cur->firstRowIndex(),8);
assertTrue($cur->endOfResultSet());
assertEquals($cur->rowCount(),8);
$cur->setResultSetBufferSize(0);
print("\n");


# cached result set
print("CACHED RESULT SET: \n");
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$filename=$cur->getCacheFileName();
assertEquals($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEquals($cur->getField(7,0),"8");
print("\n");


# column count for cached result set
print("COLUMN COUNT FOR CACHED RESULT SET: \n");
assertEquals($cur->colCount(),14);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEquals($cur->getColumnName(0),"testint");
assertEquals($cur->getColumnName(1),"testsmallint");
assertEquals($cur->getColumnName(2),"testtinyint");
assertEquals($cur->getColumnName(3),"testreal");
assertEquals($cur->getColumnName(4),"testfloat");
assertEquals($cur->getColumnName(5),"testdecimal");
assertEquals($cur->getColumnName(6),"testnumeric");
assertEquals($cur->getColumnName(7),"testmoney");
assertEquals($cur->getColumnName(8),"testsmallmoney");
assertEquals($cur->getColumnName(9),"testdatetime");
assertEquals($cur->getColumnName(10),"testsmalldatetime");
assertEquals($cur->getColumnName(11),"testchar");
assertEquals($cur->getColumnName(12),"testvarchar");
assertEquals($cur->getColumnName(13),"testbit");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"testint");
assertEquals($cols[1],"testsmallint");
assertEquals($cols[2],"testtinyint");
assertEquals($cols[3],"testreal");
assertEquals($cols[4],"testfloat");
assertEquals($cols[5],"testdecimal");
assertEquals($cols[6],"testnumeric");
assertEquals($cols[7],"testmoney");
assertEquals($cols[8],"testsmallmoney");
assertEquals($cols[9],"testdatetime");
assertEquals($cols[10],"testsmalldatetime");
assertEquals($cols[11],"testchar");
assertEquals($cols[12],"testvarchar");
assertEquals($cols[13],"testbit");
print("\n");


# cached result set with result set buffer size
print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$filename=$cur->getCacheFileName();
assertEquals($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEquals($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
$cur->setResultSetBufferSize(0);
print("\n");


# from one cache file to another
print("FROM ONE CACHE FILE TO ANOTHER: \n");
$cur->cacheToFile("cachefile2");
assertTrue($cur->openCachedResultSet("cachefile1"));
$cur->cacheOff();
assertTrue($cur->openCachedResultSet("cachefile2"));
assertEquals($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
print("\n");


# from one cache file to another with result set buffer size
print("FROM ONE CACHE FILE TO ANOTHER ".
	"WITH RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile2");
assertTrue($cur->openCachedResultSet("cachefile1"));
$cur->cacheOff();
assertTrue($cur->openCachedResultSet("cachefile2"));
assertEquals($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
$cur->setResultSetBufferSize(0);
print("\n");


# cached result set with suspend and result set buffer size
print("CACHED RESULT SET WITH SUSPEND ".
	"AND RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEquals($cur->getField(2,0),"3");
$filename=$cur->getCacheFileName();
assertEquals($filename,"cachefile1");
$id=$cur->getResultSetId();
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
print("\n");
assertTrue($con->resumeSession($port,$socket));
assertTrue($cur->resumeCachedResultSet($id,$filename));
print("\n");
assertEquals($cur->firstRowIndex(),4);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),6);
assertEquals($cur->getField(7,0),"8");
print("\n");
assertEquals($cur->firstRowIndex(),6);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),8);
assertUndef($cur->getField(8,0));
print("\n");
assertEquals($cur->firstRowIndex(),8);
assertTrue($cur->endOfResultSet());
assertEquals($cur->rowCount(),8);
$cur->cacheOff();
print("\n");
assertTrue($cur->openCachedResultSet($filename));
assertEquals($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
$cur->setResultSetBufferSize(0);
print("\n");


# finished suspended session
print("FINISHED SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEquals($cur->getField(4,0),"5");
assertEquals($cur->getField(5,0),"6");
assertEquals($cur->getField(6,0),"7");
assertEquals($cur->getField(7,0),"8");
$id=$cur->getResultSetId();
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
assertTrue($cur->resumeResultSet($id));
assertUndef($cur->getField(4,0));
assertUndef($cur->getField(5,0));
assertUndef($cur->getField(6,0));
assertUndef($cur->getField(7,0));
print("\n");


# nested selects
print("NESTED SELECTS: \n");
# can't do this with freetds
#$cur->setResultSetBufferSize(1);
assertTrue($cur->sendQuery("select * from testtable"));
$secondcur=SQLRelay::Cursor->new($con);
$secondcur->setResultSetBufferSize(1);
for ($i=0; ; $i++) {
	@row=$cur->getRow($i);
	last if (!@row);
	assertTrue($secondcur->sendQuery("select * from testtable"));
}
$secondcur->closeResultSet();
#$cur->setResultSetBufferSize(0);
assertTrue($con->commit());
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# reset transaction state
print("RESET TRANSACTION STATE: \n");
assertTrue($con->commit());
assertEquals($con->getTransactionModel(),"explicit-error");
assertTrue($con->getAutoCommit());
print("\n");


# transaction behavior - implicit
print("TRANSACTION BEHAVIOR - implicit: \n");
# sap ase rejects DDL inside a chained-mode (multi-statement) tx
# unless `sp_dboption ... 'ddl in tran', true` is set on the db;
# create the table while still in unchained mode, then switch.
# `lock datarows` is needed so secondcur's count(*) scan doesn't
# block on the writer's page lock from the in-flight insert
assertTrue($cur->sendQuery(
	"create table testtable (col1 integer) lock datarows"));
assertTrue($con->setTransactionModel("implicit"));
assertEquals($con->getTransactionModel(),"implicit");
$secondcon=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$secondcur=SQLRelay::Cursor->new($secondcon);
# session is in a transaction; insert is not visible until commit
assertTrue($con->getInTransaction());
assertFalse($con->getAutoCommit());
assertTrue($cur->sendQuery("insert into testtable values (1)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"0");
# commit makes it visible, and implicitly starts a new transaction
assertTrue($con->commit());
assertTrue($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# rollback discards, and implicitly starts a new transaction
assertTrue($cur->sendQuery("insert into testtable values (2)"));
assertTrue($con->rollback());
assertTrue($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# autoCommitOn takes effect immediately
assertTrue($con->autoCommitOn());
assertTrue($con->getAutoCommit());
assertFalse($con->getInTransaction());
assertTrue($cur->sendQuery("insert into testtable values (3)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"2");
# autoCommitOff takes effect immediately
assertTrue($con->autoCommitOff());
assertFalse($con->getAutoCommit());
assertTrue($con->getInTransaction());
$secondcur->closeResultSet();
# switch back to unchained mode so the drop isn't rejected
assertTrue($con->setTransactionModel("explicit-error"));
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# transaction behavior - explicit
print("TRANSACTION BEHAVIOR - explicit: \n");
assertTrue($con->setTransactionModel("explicit"));
assertEquals($con->getTransactionModel(),"explicit");
assertTrue($cur->sendQuery("create table testtable (col1 integer) lock datarows"));
# begin starts a new transaction; insert is not visible until commit
assertTrue($con->begin());
assertTrue($con->getInTransaction());
assertTrue($cur->sendQuery("insert into testtable values (1)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"0");
# commit makes it visible; no new transaction is started
assertTrue($con->commit());
assertFalse($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# begin, insert, rollback discards; no new transaction is started
assertTrue($con->begin());
assertTrue($cur->sendQuery("insert into testtable values (2)"));
assertTrue($con->rollback());
assertFalse($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# autoCommitOn takes effect immediately
assertTrue($con->autoCommitOn());
assertTrue($con->getAutoCommit());
assertTrue($cur->sendQuery("insert into testtable values (3)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"2");
# autoCommitOff takes effect immediately
assertTrue($con->autoCommitOff());
assertFalse($con->getAutoCommit());
$secondcur->closeResultSet();
# switch back to unchained mode so the drop isn't rejected
assertTrue($con->setTransactionModel("explicit-error"));
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# transaction behavior - explicit-deferred
print("TRANSACTION BEHAVIOR - explicit-deferred: \n");
assertTrue($con->setTransactionModel("explicit-deferred"));
assertEquals($con->getTransactionModel(),"explicit-deferred");
# switch to autocommit-on so the begin/commit cycles below
# bracket explicit transactions (autocommit-off semantics are
# exercised at the end of this block)
assertTrue($con->autoCommitOn());
assertTrue($con->getAutoCommit());
assertTrue($cur->sendQuery("create table testtable (col1 integer) lock datarows"));
# begin starts a transaction; commit makes it visible
assertTrue($con->begin());
assertTrue($con->getInTransaction());
assertTrue($cur->sendQuery("insert into testtable values (1)"));
assertTrue($con->commit());
assertFalse($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# begin, insert, rollback discards
assertTrue($con->begin());
assertTrue($cur->sendQuery("insert into testtable values (2)"));
assertTrue($con->rollback());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# during a transaction started by begin(), autoCommitOn is a
# no-op: the autocommit setting takes effect after the user
# explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue($con->begin());
assertTrue($cur->sendQuery("insert into testtable values (3)"));
assertTrue($con->autoCommitOn());
assertFalse($con->getAutoCommit());
assertTrue($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# explicit commit ends the tx; autocommit-on now takes effect
assertTrue($con->commit());
assertTrue($con->getAutoCommit());
assertFalse($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"2");
# autocommit is on; subsequent inserts are visible immediately
assertTrue($cur->sendQuery("insert into testtable values (4)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"3");
# autoCommitOff takes effect immediately when not in a transaction
assertTrue($con->autoCommitOff());
assertFalse($con->getAutoCommit());
# autocommit-off persists across commit/rollback; each commit or
# rollback ends the current implicit tx and a new one starts for
# the next statement
assertTrue($cur->sendQuery("insert into testtable values (5)"));
assertTrue($con->commit());
assertFalse($con->getAutoCommit());
assertTrue($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"4");
assertTrue($cur->sendQuery("insert into testtable values (6)"));
assertTrue($con->rollback());
assertFalse($con->getAutoCommit());
assertTrue($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"4");
# autoCommitOff during a transaction changes the variable
# immediately but the in-flight tx continues; only after the
# next explicit commit/rollback does the new autocommit-off
# setting drop us into a new implicit tx (mysql-asymmetric
# semantic)
assertTrue($con->autoCommitOn());
assertTrue($con->getAutoCommit());
assertTrue($con->begin());
assertTrue($cur->sendQuery("insert into testtable values (7)"));
assertTrue($con->autoCommitOff());
assertFalse($con->getAutoCommit());
assertTrue($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"4");
assertTrue($con->commit());
assertFalse($con->getAutoCommit());
assertTrue($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"5");
$secondcur->closeResultSet();
# switch back to unchained mode so the drop isn't rejected
assertTrue($con->setTransactionModel("explicit-error"));
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# transaction behavior - explicit-error
print("TRANSACTION BEHAVIOR - explicit-error: \n");
assertTrue($con->setTransactionModel("explicit-error"));
assertEquals($con->getTransactionModel(),"explicit-error");
assertTrue($cur->sendQuery("create table testtable (col1 integer) lock datarows"));
# begin, insert, commit
assertTrue($con->begin());
assertTrue($con->getInTransaction());
assertTrue($cur->sendQuery("insert into testtable values (1)"));
assertTrue($con->commit());
assertFalse($con->getInTransaction());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# begin, insert, rollback
assertTrue($con->begin());
assertTrue($cur->sendQuery("insert into testtable values (2)"));
assertTrue($con->rollback());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# while in a transaction, autoCommitOn/Off throw an error
assertTrue($con->begin());
assertFalse($con->autoCommitOn());
assertFalse($con->autoCommitOff());
assertTrue($con->commit());
# outside of a transaction, autoCommitOn takes effect immediately
assertTrue($con->autoCommitOn());
assertTrue($con->getAutoCommit());
assertTrue($cur->sendQuery("insert into testtable values (3)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"2");
# autoCommitOff takes effect immediately
assertTrue($con->autoCommitOff());
assertFalse($con->getAutoCommit());
$secondcur->closeResultSet();
# commit the open tx so the drop isn't rejected as DDL inside a
# chained-mode transaction (in explicit-error model, autoCommitOn
# from inside a tx errors out by design, so commit is the route
# back to autocommit-on / unchained mode)
assertTrue($con->commit());
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# transaction behavior - none
print("TRANSACTION BEHAVIOR - none: \n");
assertTrue($con->setTransactionModel("none"));
assertEquals($con->getTransactionModel(),"none");
assertTrue($cur->sendQuery("create table testtable (col1 integer) lock datarows"));
# no transactions; everything is visible immediately
assertTrue($con->getAutoCommit());
assertFalse($con->getInTransaction());
assertTrue($cur->sendQuery("insert into testtable values (1)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"1");
# commit and rollback are no-ops
assertTrue($con->commit());
assertTrue($cur->sendQuery("insert into testtable values (2)"));
assertTrue($con->rollback());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEquals($secondcur->getField(0,0),"2");
# autocommit is always on; autoCommitOff is an error
assertFalse($con->autoCommitOff());
assertTrue($con->getAutoCommit());
assertTrue($con->autoCommitOn());
assertTrue($con->getAutoCommit());
$secondcur->closeResultSet();
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# reset transaction behavior
print("RESET TRANSACTION BEHAVIOR: \n");
assertTrue($con->setTransactionModel($con->getDefaultTransactionModel()));
assertEquals($con->getTransactionModel(),"explicit-error");
assertTrue($con->getAutoCommit());
print("\n");


# individual substitutions
print("INDIVIDUAL SUBSTITUTIONS: \n");
$cur->prepareQuery("select \$(var1),'\$(var2)',\$(var3)");
$cur->substitution("var1",1);
$cur->substitution("var2","hello");
$cur->substitution("var3",10.5556,6,4);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"hello");
assertEquals($cur->getField(0,2),"10.5556");
print("\n");


# array substitutions
print("ARRAY SUBSTITUTIONS: \n");
$cur->prepareQuery("select \$(var1),\$(var2),\$(var3)");
$cur->substitutions(\@subvars,\@subvallongs);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"2");
assertEquals($cur->getField(0,2),"3");
print("\n");
$cur->prepareQuery("select '\$(var1)','\$(var2)','\$(var3)'");
$cur->substitutions(\@subvars,\@subvalstrings);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"hi");
assertEquals($cur->getField(0,1),"hello");
assertEquals($cur->getField(0,2),"bye");
print("\n");
$cur->prepareQuery("select \$(var1),\$(var2),\$(var3)");
$cur->substitutions(\@subvars,\@subvaldoubles,\@precs,\@scales);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"10.55");
assertEquals($cur->getField(0,1),"10.556");
assertEquals($cur->getField(0,2),"10.5556");
print("\n");


# nulls as nulls
print("NULLS AS NULLS: \n");
$cur->getNullsAsUndefined();
assertTrue($cur->sendQuery("select NULL,1,NULL"));
assertUndef($cur->getField(0,0));
assertEquals($cur->getField(0,1),"1");
assertUndef($cur->getField(0,2));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("select NULL,1,NULL"));
assertEquals($cur->getField(0,0),"");
assertEquals($cur->getField(0,1),"1");
assertEquals($cur->getField(0,2),"");
print("\n");



# null and empty lobs
print("NULL AND EMPTY LOBS: \n");
$cur->getNullsAsUndefined();
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testclob1 text NULL, ".
	"	testclob2 text NULL, ".
	"	testblob1 image NULL, ".
	"	testblob2 image NULL)"));
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?)");
$cur->inputBindClob("1","",0);
$cur->inputBindClob("2",undef,0);
$cur->inputBindBlob("3","",0);
$cur->inputBindBlob("4",undef,0);
assertTrue($cur->executeQuery());
$cur->sendQuery("select * from testtable");
# sap converts empty strings to a single space.  It's possible that
# if we had true input bind support on the backend, then this would
# work correctly, but for now we're faking binds, and inserting an
# empty string, so we have to check for a single space here.
assertEquals($cur->getField(0,0)," ");
assertUndef($cur->getField(0,1));
# sap doesn't really support inserting an empty string into a binary
# column.  The minimum that can be inserted is a single \0.  That ends
# up being interpreted as an empty string in C (strcmp stops at the
# null terminator), but Perl preserves the null byte, so assert on the
# raw byte here.
assertEquals($cur->getField(0,2),"\0");
assertUndef($cur->getField(0,3));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# long lobs
print("LONG LOBS: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery(
	"create table testtable (".
	"	testclob text, ".
	"	testblob image) lock datarows");
$cur->prepareQuery("insert into testtable values (?,?)");
$largebuffer=('C' x $LARGE_BUFFER_LENGTH);
$cur->inputBindClob("1",$largebuffer,$LARGE_BUFFER_LENGTH);
$cur->inputBindBlob("2",$largebuffer,$LARGE_BUFFER_LENGTH);
assertTrue($cur->executeQuery());
$cur->sendQuery("select * from testtable");
assertEquals($cur->getFieldLength(0,"testclob"),$LARGE_BUFFER_LENGTH);
assertEquals($cur->getField(0,"testclob"),$largebuffer);
assertEquals($cur->getFieldLength(0,"testblob"),$LARGE_BUFFER_LENGTH);
assertEqualsBytes($cur->getField(0,"testblob"),$largebuffer,
					$LARGE_BUFFER_LENGTH);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# output bind by position
# FreeTDS needs to support cursors for this to work


# output bind by name
# FreeTDS needs to support cursors for this to work


# output bind by name with validation
# Even if FreeTDS supported cursors...
# validateBinds() can't be used for output binds, with sap.  In sap,
# when executing a procedure, you don't declare any bind variable
# delimiters in the query.  eg, you just do: "exec testproc", not
# "exec testproc(@out1,@out2)".  If you call validateBinds(), it won't
# find any binds in the query, and will filter out any binds that you
# declare.


# lob output bind
# sap doesn't support lobs as output parameters to stored procedures,
# and there's no way to directly select into a lob bind variable


# long output bind
# FreeTDS needs to support cursors for this to work


# negative input bind
print("NEGATIVE INPUT BIND: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery("create table testtable (testval int)");
$cur->prepareQuery("insert into testtable values (\@testval)");
$cur->inputBind("testval",-1);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testval from testtable");
assertEquals($cur->getField(0,"testval"),"-1");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# bind validation
print("BIND VALIDATION: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery(
	"create table testtable (".
	"	col1 varchar(20), ".
	"	col2 varchar(20), ".
	"	col3 varchar(20))");
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	\$(var1), ".
	"	\$(var2), ".
	"	\$(var3))");
$cur->inputBind("var1","1");
$cur->inputBind("var2","2");
$cur->inputBind("var3","3");
$cur->substitution("var1","\@var1");
assertTrue($cur->validBind("var1"));
assertFalse($cur->validBind("var2"));
assertFalse($cur->validBind("var3"));
assertFalse($cur->validBind("var4"));
print("\n");
$cur->substitution("var2","\@var2");
assertTrue($cur->validBind("var1"));
assertTrue($cur->validBind("var2"));
assertFalse($cur->validBind("var3"));
assertFalse($cur->validBind("var4"));
print("\n");
$cur->substitution("var3","\@var3");
assertTrue($cur->validBind("var1"));
assertTrue($cur->validBind("var2"));
assertTrue($cur->validBind("var3"));
assertFalse($cur->validBind("var4"));
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# rebinding
# FreeTDS needs to support cursors for this to work


# reexecute
print("REEXECUTE: \n");
$cur->prepareQuery("select 1");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
$cur->prepareQuery("select cast(? as int)");
$cur->inputBind("1",1);
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
$cur->inputBind("1",2);
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"2");
print("\n");


# stored procedure returning no value
# FreeTDS needs to support cursors for this to work


# stored procedure returning single value
# FreeTDS needs to support cursors for this to work


# stored procedure returning multiple values
# FreeTDS needs to support cursors for this to work


# stored procedure returning result set
print("STORED PROCEDURE RETURNING RESULT SET: \n");
$cur->sendQuery("drop procedure testselectproc");
assertTrue($cur->sendQuery(
	"create procedure testselectproc as ".
	"       select 1 ".
	"       union ".
	"       select 2 ".
	"       union ".
	"       select 3 ".
	"       union ".
	"       select 4 ".
	"       union ".
	"       select 5 ".
	"       union ".
	"       select 6 ".
	"       union ".
	"       select 7 ".
	"       union ".
	"       select 8"));
assertTrue($cur->sendQuery("exec testselectproc"));
assertEquals($cur->rowCount(),8);
assertTrue($cur->sendQuery("drop procedure testselectproc"));
print("\n");


# temporary tables
print("TEMPORARY TABLES: \n");
$cur->sendQuery("drop table #temptable");
$cur->sendQuery("create table #temptable (col1 int)");
assertTrue($cur->sendQuery("insert into #temptable values (1)"));
assertTrue($cur->sendQuery("select count(*) from #temptable"));
assertEquals($cur->getField(0,0),"1");
$con->endSession();
print("\n");
assertFalse($cur->sendQuery("select count(*) from #temptable"));
print("\n");


# encoded binary data
print("ENCODED BINARY DATA: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 image)"));
$buffer=pack("C*",(0..255));
$query="insert into testtable values (0x";
$query.=unpack("H*",$buffer);
$query.=")";
assertTrue($cur->sendQuery($query));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),length($buffer));
assertEqualsBytes($cur->getField(0,0),$buffer,length($buffer));
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes
print("QUOTES: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('''''')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertEquals($cur->getField(0,0),"''");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# last insert id
print("LAST INSERT ID: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
		"create table testtable ".
		"	(col1 int identity primary key, ".
		"	col2 int)"));
assertTrue($cur->sendQuery(
		"insert into testtable (col2) values (1)"));
assertEquals($con->getLastInsertId(),1);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# database is schema
print("DATABASE IS SCHEMA: \n");
assertFalse($con->getDatabaseIsSchema());
print("\n");


# catalog list
print("CATALOG LIST: \n");
assertTrue($cur->getCatalogList(undef));
assertEquals($cur->getColumnName(0),"Database");
assertInResultSet($cur,"Database",$hostname);
print("\n");


# schema list
print("SCHEMA LIST: \n");
$cur->sendQuery("drop table testtable");
# the get schema list query that is used with sap will only return the
# names of schemas that have at least one database object in them, so
# to be sure that there is one, we'll create a table
assertTrue($cur->sendQuery("create table testtable (col1 int)"));
assertTrue($cur->getSchemaList(undef));
assertEquals($cur->getColumnName(0),"Database");
assertInResultSet($cur,"Database","dbo");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# table type list
print("TABLE TYPE LIST: \n");
assertTrue($cur->getTableTypeList());
assertEquals($cur->getColumnName(0),"table_type");
assertInResultSet($cur,"table_type","TABLE");
print("\n");


# table list
print("TABLE LIST: \n");
$cur->sendQuery("drop table testtable1");
$cur->sendQuery("drop table testtable2");
$cur->sendQuery("drop table testtable3");
$cur->sendQuery("drop table testtable4");
assertTrue($cur->sendQuery(
	"create table testtable1 (".
	"	col1 int, ".
	"	col2 int)"));
assertTrue($cur->sendQuery(
	"create table testtable2 (".
	"	col1 int, ".
	"	col2 int)"));
assertTrue($cur->sendQuery(
	"create table testtable3 (".
	"	col1 int, ".
	"	col2 int)"));
assertTrue($cur->sendQuery(
	"create table testtable4 (".
	"	col1 int, ".
	"	col2 int)"));
assertTrue($cur->getTableList(undef));
assertInResultSet($cur,"Tables_in_xxx","testtable1");
assertInResultSet($cur,"Tables_in_xxx","testtable2");
assertInResultSet($cur,"Tables_in_xxx","testtable3");
assertInResultSet($cur,"Tables_in_xxx","testtable4");
assertTrue($cur->sendQuery("drop table testtable1"));
assertTrue($cur->sendQuery("drop table testtable2"));
assertTrue($cur->sendQuery("drop table testtable3"));
assertTrue($cur->sendQuery("drop table testtable4"));
print("\n");


# type info list
print("TYPE INFO LIST: \n");
assertTrue($cur->getTypeInfoList("int"));
assertEquals($cur->getColumnName(0),"type_name");
assertEquals($cur->getColumnName(1),"data_type");
assertEquals($cur->getColumnName(2),"precision");
assertEquals($cur->getColumnName(3),"literal_prefix");
assertEquals($cur->getColumnName(4),"literal_suffix");
assertEquals($cur->getColumnName(5),"create_params");
assertEquals($cur->getColumnName(6),"nullable");
assertEquals($cur->getColumnName(7),"case_sensitive");
assertEquals($cur->getColumnName(8),"searchable");
assertEquals($cur->getColumnName(9),"unsigned_attribute");
assertEquals($cur->getColumnName(10),"fixed_prec_scale");
assertEquals($cur->getColumnName(11),"auto_increment");
assertEquals($cur->getColumnName(12),"local_type_name");
assertEquals($cur->getColumnName(13),"minumum_scale");
assertEquals($cur->getColumnName(14),"maxiumm_scale");
assertEquals($cur->getColumnName(15),"sql_data_type");
assertEquals($cur->getColumnName(16),"sql_datetime_sub");
assertEquals($cur->getColumnName(17),"num_prec_radix");
assertEquals($cur->getColumnName(18),"interval_precision");
assertEquals($cur->getField(0,"type_name"),"INT");
assertEquals($cur->getField(0,"data_type"),"4");
assertEquals($cur->getField(0,"precision"),"10");
assertEquals($cur->getField(0,"local_type_name"),"INT");
assertTrue($cur->getTypeInfoList("char"));
assertEquals($cur->getField(0,"type_name"),"CHAR");
assertEquals($cur->getField(0,"data_type"),"1");
assertEquals($cur->getField(0,"precision"),"8000");
assertEquals($cur->getField(0,"local_type_name"),"CHAR");
assertTrue($cur->getTypeInfoList("varchar"));
assertEquals($cur->getField(0,"type_name"),"VARCHAR");
assertEquals($cur->getField(0,"data_type"),"12");
assertEquals($cur->getField(0,"precision"),"8000");
assertEquals($cur->getField(0,"local_type_name"),"VARCHAR");
assertTrue($cur->getTypeInfoList("datetime"));
assertEquals($cur->getField(0,"type_name"),"DATETIME");
assertEquals($cur->getField(0,"data_type"),"93");
assertEquals($cur->getField(0,"precision"),"23");
assertEquals($cur->getField(0,"local_type_name"),"DATETIME");
print("\n");


# column list
print("COLUMN LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testint int, ".
	"	testsmallint smallint, ".
	"	testtinyint tinyint, ".
	"	testreal real, ".
	"	testfloat float, ".
	"	testdecimal decimal(4,1), ".
	"	testnumeric numeric(4,1), ".
	"	testmoney money, ".
	"	testsmallmoney smallmoney, ".
	"	testdatetime datetime, ".
	"	testsmalldatetime smalldatetime, ".
	"	testchar char(40), ".
	"	testvarchar varchar(40), ".
	"	testbit bit)"));
assertTrue($cur->getColumnList("testtable",undef));
assertEquals($cur->getColumnName(0),"column_name");
assertEquals($cur->getColumnName(1),"data_type");
assertEquals($cur->getColumnName(2),"character_maximum_length");
assertEquals($cur->getColumnName(3),"numeric_precision");
assertEquals($cur->getColumnName(4),"numeric_scale");
assertEquals($cur->getColumnName(5),"is_nullable");
assertEquals($cur->getColumnName(6),"column_key");
assertEquals($cur->getColumnName(7),"column_default");
assertEquals($cur->getColumnName(8),"extra");
assertTrue($cur->getField(0,"column_name") eq "testint");
assertTrue($cur->getField(1,"column_name") eq "testsmallint");
assertTrue($cur->getField(2,"column_name") eq "testtinyint");
assertTrue($cur->getField(3,"column_name") eq "testreal");
assertTrue($cur->getField(4,"column_name") eq "testfloat");
assertTrue($cur->getField(5,"column_name") eq "testdecimal");
assertTrue($cur->getField(6,"column_name") eq "testnumeric");
assertTrue($cur->getField(7,"column_name") eq "testmoney");
assertTrue($cur->getField(8,"column_name") eq "testsmallmoney");
assertTrue($cur->getField(9,"column_name") eq "testdatetime");
assertTrue($cur->getField(10,"column_name") eq "testsmalldatetime");
assertTrue($cur->getField(11,"column_name") eq "testchar");
assertTrue($cur->getField(12,"column_name") eq "testvarchar");
assertTrue($cur->getField(13,"column_name") eq "testbit");
assertTrue($cur->getField(0,"data_type") eq "int");
assertTrue($cur->getField(1,"data_type") eq "smallint");
assertTrue($cur->getField(2,"data_type") eq "tinyint");
assertTrue($cur->getField(3,"data_type") eq "real");
assertTrue($cur->getField(4,"data_type") eq "float");
assertTrue($cur->getField(5,"data_type") eq "decimal");
assertTrue($cur->getField(6,"data_type") eq "numeric");
assertTrue($cur->getField(7,"data_type") eq "money");
assertTrue($cur->getField(8,"data_type") eq "smallmoney");
assertTrue($cur->getField(9,"data_type") eq "datetime");
assertTrue($cur->getField(10,"data_type") eq "smalldatetime");
assertTrue($cur->getField(11,"data_type") eq "char");
assertTrue($cur->getField(12,"data_type") eq "varchar");
assertTrue($cur->getField(13,"data_type") eq "bit");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# column list - auto_increment, primary key
print("COLUMN LIST - auto_increment, primary key: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int identity primary key, ".
	"	col2 int)"));
assertTrue($cur->getColumnList("testtable",undef));
assertEquals($cur->getField(0,"extra"),"auto_increment");
assertEquals($cur->getField(0,"column_key"),"PRI");
assertEquals($cur->getField(1,"extra"),"");
assertEquals($cur->getField(1,"column_key"),"");
print("\n");
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int primary key, ".
	"	col2 int)"));
assertTrue($cur->getColumnList("testtable",undef));
assertEquals($cur->getField(0,"extra"),"");
assertEquals($cur->getField(0,"column_key"),"PRI");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# primary keys list
print("PRIMARY KEYS LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int primary key, ".
	"	col2 int)"));
assertTrue($cur->getPrimaryKeysList("testtable",undef));
assertEquals($cur->getColumnName(0),"table");
assertEquals($cur->getColumnName(1),"non_unique");
assertEquals($cur->getColumnName(2),"key_name");
assertEquals($cur->getColumnName(3),"seq_in_index");
assertEquals($cur->getColumnName(4),"column_name");
assertEquals($cur->getColumnName(5),"collation");
assertEquals($cur->getColumnName(6),"cardinality");
assertEquals($cur->getColumnName(7),"sub_part");
assertEquals($cur->getColumnName(8),"packed");
assertEquals($cur->getColumnName(9),"null");
assertEquals($cur->getColumnName(10),"index_type");
assertEquals($cur->getColumnName(11),"comment");
assertEquals($cur->getColumnName(12),"index_comment");
assertEquals($cur->rowCount(),1);
assertTrue($cur->getField(0,"table") eq "testtable");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertTrue($cur->getField(0,"column_name") eq "col1");
assertTrue(defined($cur->getField(0,"key_name")) &&
		$cur->getField(0,"key_name") ne "");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# key and index list
print("KEY AND INDEX LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int primary key, ".
	"	col2 int)"));
assertTrue($cur->getKeyAndIndexList("testtable",undef));
assertEquals($cur->getColumnName(0),"table");
assertEquals($cur->getColumnName(1),"non_unique");
assertEquals($cur->getColumnName(2),"key_name");
assertEquals($cur->getColumnName(3),"seq_in_index");
assertEquals($cur->getColumnName(4),"column_name");
assertEquals($cur->getColumnName(5),"collation");
assertEquals($cur->getColumnName(6),"cardinality");
assertEquals($cur->getColumnName(7),"sub_part");
assertEquals($cur->getColumnName(8),"packed");
assertEquals($cur->getColumnName(9),"null");
assertEquals($cur->getColumnName(10),"index_type");
assertEquals($cur->getColumnName(11),"comment");
assertEquals($cur->getColumnName(12),"index_comment");
assertEquals($cur->rowCount(),1);
assertTrue($cur->getField(0,"table") eq "testtable");
assertEquals($cur->getField(0,"non_unique"),"0");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertTrue($cur->getField(0,"column_name") eq "col1");
assertEquals($cur->getField(0,"collation"),"A");
assertEquals($cur->getField(0,"index_type"),"1");
assertTrue(defined($cur->getField(0,"key_name")) &&
		$cur->getField(0,"key_name") ne "");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# procedure list
print("PROCEDURE LIST: \n");
$cur->sendQuery("drop procedure testproc1");
$cur->sendQuery("drop procedure testproc2");
$cur->sendQuery("drop procedure testproc3");
$cur->sendQuery("drop procedure testproc4");
assertTrue($cur->sendQuery(
	"create procedure testproc1 ".
	"	\@in1 int, ".
	"	\@in2 char(20), ".
	"	\@in3 varchar(20), ".
	"	\@in4 datetime ".
	"as select 1"));
assertTrue($cur->sendQuery(
	"create procedure testproc2 ".
	"	\@in1 int, ".
	"	\@in2 char(20), ".
	"	\@in3 varchar(20), ".
	"	\@in4 datetime ".
	"as select 1"));
assertTrue($cur->sendQuery(
	"create procedure testproc3 ".
	"	\@in1 int, ".
	"	\@in2 char(20), ".
	"	\@in3 varchar(20), ".
	"	\@in4 datetime ".
	"as select 1"));
assertTrue($cur->sendQuery(
	"create procedure testproc4 ".
	"	\@in1 int, ".
	"	\@in2 char(20), ".
	"	\@in3 varchar(20), ".
	"	\@in4 datetime ".
	"as select 1"));
assertTrue($cur->getProcedureList(undef));
assertInResultSet($cur,"routine_name","testproc1");
assertInResultSet($cur,"routine_name","testproc2");
assertInResultSet($cur,"routine_name","testproc3");
assertInResultSet($cur,"routine_name","testproc4");
print("\n");


# procedure parameter list
print("PROCEDURE PARAMETER LIST: \n");
assertTrue($cur->getProcedureParameterList("testproc1",undef));
assertEquals($cur->getColumnName(0),"parameter_name");
assertEquals($cur->getColumnName(1),"parameter_mode");
assertEquals($cur->getColumnName(2),"data_type");
assertEquals($cur->getColumnName(3),"character_maximum_length");
assertEquals($cur->getColumnName(4),"ordinal_position");
assertEquals($cur->rowCount(),4);
assertEquals($cur->getField(0,"parameter_name"),"\@in1");
assertEquals($cur->getField(0,"parameter_mode"),"1");
assertEquals($cur->getField(0,"data_type"),"int");
assertEquals($cur->getField(0,"ordinal_position"),"1");
assertEquals($cur->getField(1,"parameter_name"),"\@in2");
assertEquals($cur->getField(1,"parameter_mode"),"1");
assertEquals($cur->getField(1,"data_type"),"char");
assertEquals($cur->getField(1,"ordinal_position"),"2");
assertEquals($cur->getField(2,"parameter_name"),"\@in3");
assertEquals($cur->getField(2,"parameter_mode"),"1");
assertEquals($cur->getField(2,"data_type"),"varchar");
assertEquals($cur->getField(2,"ordinal_position"),"3");
assertEquals($cur->getField(3,"parameter_name"),"\@in4");
assertEquals($cur->getField(3,"parameter_mode"),"1");
assertEquals($cur->getField(3,"data_type"),"datetime");
assertEquals($cur->getField(3,"ordinal_position"),"4");
assertTrue($cur->sendQuery("drop procedure testproc1"));
assertTrue($cur->sendQuery("drop procedure testproc2"));
assertTrue($cur->sendQuery("drop procedure testproc3"));
assertTrue($cur->sendQuery("drop procedure testproc4"));
print("\n");


# invalid queries
print("INVALID QUERIES: \n");
assertFalse($cur->sendQuery("select * from testtable order by testint"));
assertFalse($cur->sendQuery("select * from testtable order by testint"));
assertFalse($cur->sendQuery("select * from testtable order by testint"));
assertFalse($cur->sendQuery("select * from testtable order by testint"));
print("\n");
assertFalse($cur->sendQuery("insert into testtable values (1,2,3,4)"));
assertFalse($cur->sendQuery("insert into testtable values (1,2,3,4)"));
assertFalse($cur->sendQuery("insert into testtable values (1,2,3,4)"));
assertFalse($cur->sendQuery("insert into testtable values (1,2,3,4)"));
print("\n");
assertFalse($cur->sendQuery("create table testtable"));
assertFalse($cur->sendQuery("create table testtable"));
assertFalse($cur->sendQuery("create table testtable"));
assertFalse($cur->sendQuery("create table testtable"));
print("\n");

reportTestStatus();
exit($status);
