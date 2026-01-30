#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.


use SQLRelay::Connection;
use SQLRelay::Cursor;
require "./asserts.pl";






# instantiation
$con=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$cur=SQLRelay::Cursor->new($con);

# get database type


# identify
print("IDENTIFY: \n");
assertEqualString($con->identify(),"freetds");
print("\n");


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");


# isolation levels
print("ISOLATION LEVELS: \n");
@isolationlevels=("1","0","2","3");
foreach $il (@isolationlevels) {
	assertTrue($con->setIsolationLevel($il));
	assertEqualString($con->getIsolationLevel(),$il);
	print("\n");
}
# reset to the default isolation level
assertTrue($con->setIsolationLevel($isolationlevels[0]));
print("\n");

# drop existing table
$cur->sendQuery("drop table testtable");


# create temptable
print("CREATE TEMPTABLE: \n");
assertTrue($cur->sendQuery("create table testtable (testint int, testsmallint smallint, testtinyint tinyint, testreal real, testfloat float, testdecimal decimal(4,1), testnumeric numeric(4,1), testmoney money, testsmallmoney smallmoney, testdatetime datetime, testsmalldatetime smalldatetime, testchar char(40), testvarchar varchar(40), testbit bit)"));
print("\n");


# begin transaction
print("BEGIN TRANSACTION: \n");
assertTrue($cur->sendQuery("begin tran"));
print("\n");


# insert
print("INSERT: \n");
assertTrue($cur->sendQuery("insert into testtable values (1,1,1,1.1,1.1,1.1,1.1,1.00,1.00,'01-Jan-2001 01:00:00','01-Jan-2001 01:00:00','testchar1','testvarchar1',1)"));
print("\n");


# affected rows
print("AFFECTED ROWS: \n");
assertEqual($cur->affectedRows(),1);
print("\n");


# bind by position
print("BIND BY POSITION: \n");
$cur->prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
assertEqual($cur->countBindVariables(),14);
$cur->inputBind("1",2);
$cur->inputBind("2",2);
$cur->inputBind("3",2);
$cur->inputBind("4",2.2,2,1);
$cur->inputBind("5",2.2,2,1);
$cur->inputBind("6",2.2,2,1);
$cur->inputBind("7",2.2,2,1);
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
$cur->inputBind("4",3.3,2,1);
$cur->inputBind("5",3.3,2,1);
$cur->inputBind("6",3.3,2,1);
$cur->inputBind("7",3.3,2,1);
$cur->inputBind("8",3.00,3,2);
$cur->inputBind("9",3.00,3,2);
$cur->inputBind("10","01-Jan-2003 03:00:00");
$cur->inputBind("11","01-Jan-2003 03:00:00");
$cur->inputBind("12","testchar3");
$cur->inputBind("13","testvarchar3");
$cur->inputBind("14",1);
assertTrue($cur->executeQuery());
print("\n");


# array of binds by position
print("ARRAY OF BINDS BY POSITION: \n");
$cur->clearBinds();
@vars=("1","2","3","4","5","6","7","8","9","10","11","12","13","14");
@vals=(4,4,4,4.4,4.4,4.4,4.4,4.00,4.00,"01-Jan-2004 04:00:00",
	"01-Jan-2004 04:00:00","testchar4","testvarchar4",1);
@precs=(0,0,0,2,2,2,2,3,3,0,0,0,0,0);
@scales=(0,0,0,1,1,1,1,2,2,0,0,0,0,0);
$cur->inputBinds(\@vars,\@vals,\@precs,\@scales);
assertTrue($cur->executeQuery());
print("\n");


# bind by name
print("BIND BY NAME: \n");
$cur->clearBinds();
$cur->prepareQuery("insert into testtable values (\@var1,\@var2,\@var3,\@var4,\@var5,\@var6,\@var7,\@var8,\@var9,\@var10,\@var11,\@var12,\@var13,\@var14)");
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
$cur->inputBind("var4",6.6,2,1);
$cur->inputBind("var5",6.6,2,1);
$cur->inputBind("var6",6.6,2,1);
$cur->inputBind("var7",6.6,2,1);
$cur->inputBind("var8",6.00,3,2);
$cur->inputBind("var9",6.00,3,2);
$cur->inputBind("var10","01-Jan-2006 06:00:00");
$cur->inputBind("var11","01-Jan-2006 06:00:00");
$cur->inputBind("var12","testchar6");
$cur->inputBind("var13","testvarchar6");
$cur->inputBind("var14",1);
assertTrue($cur->executeQuery());
print("\n");


# array of binds by name
print("ARRAY OF BINDS BY NAME: \n");
$cur->clearBinds();
@vars=("var1","var2","var3","var4","var5","var6",
		"var7","var8","var9","var10","var11","var12","var13","var14");
@vals=(7,7,7,7.7,7.7,7.7,7.7,7.00,7.00,
	"01-Jan-2007 07:00:00","01-Jan-2007 07:00:00",
	"testchar7","testvarchar7",1);
@precs=(0,0,0,2,2,2,2,3,3,0,0,0,0,0);
@scales=(0,0,0,1,1,1,1,2,2,0,0,0,0,0);
$cur->inputBinds(\@vars,\@vals,\@precs,\@scales);
assertTrue($cur->executeQuery());
print("\n");


# bind by name with validation
print("BIND BY NAME WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("var1",8);
$cur->inputBind("var2",8);
$cur->inputBind("var3",8);
$cur->inputBind("var4",8.8,2,1);
$cur->inputBind("var5",8.8,2,1);
$cur->inputBind("var6",8.8,2,1);
$cur->inputBind("var7",8.8,2,1);
$cur->inputBind("var8",8.00,3,2);
$cur->inputBind("var9",8.00,3,2);
$cur->inputBind("var10","01-Jan-2008 08:00:00");
$cur->inputBind("var11","01-Jan-2008 08:00:00");
$cur->inputBind("var12","testchar8                               ");
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
assertEqual($cur->colCount(),14);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEqualString($cur->getColumnName(0),"testint");
assertEqualString($cur->getColumnName(1),"testsmallint");
assertEqualString($cur->getColumnName(2),"testtinyint");
assertEqualString($cur->getColumnName(3),"testreal");
assertEqualString($cur->getColumnName(4),"testfloat");
assertEqualString($cur->getColumnName(5),"testdecimal");
assertEqualString($cur->getColumnName(6),"testnumeric");
assertEqualString($cur->getColumnName(7),"testmoney");
assertEqualString($cur->getColumnName(8),"testsmallmoney");
assertEqualString($cur->getColumnName(9),"testdatetime");
assertEqualString($cur->getColumnName(10),"testsmalldatetime");
assertEqualString($cur->getColumnName(11),"testchar");
assertEqualString($cur->getColumnName(12),"testvarchar");
assertEqualString($cur->getColumnName(13),"testbit");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"testint");
assertEqualString($cols[1],"testsmallint");
assertEqualString($cols[2],"testtinyint");
assertEqualString($cols[3],"testreal");
assertEqualString($cols[4],"testfloat");
assertEqualString($cols[5],"testdecimal");
assertEqualString($cols[6],"testnumeric");
assertEqualString($cols[7],"testmoney");
assertEqualString($cols[8],"testsmallmoney");
assertEqualString($cols[9],"testdatetime");
assertEqualString($cols[10],"testsmalldatetime");
assertEqualString($cols[11],"testchar");
assertEqualString($cols[12],"testvarchar");
assertEqualString($cols[13],"testbit");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEqualString($cur->getColumnType(0),"INT");
assertEqualString($cur->getColumnType('testint'),"INT");
assertEqualString($cur->getColumnType(1),"SMALLINT");
assertEqualString($cur->getColumnType('testsmallint'),"SMALLINT");
assertEqualString($cur->getColumnType(2),"TINYINT");
assertEqualString($cur->getColumnType('testtinyint'),"TINYINT");
assertEqualString($cur->getColumnType(3),"REAL");
assertEqualString($cur->getColumnType('testreal'),"REAL");
assertEqualString($cur->getColumnType(4),"FLOAT");
assertEqualString($cur->getColumnType('testfloat'),"FLOAT");
assertEqualString($cur->getColumnType(5),"DECIMAL");
assertEqualString($cur->getColumnType('testdecimal'),"DECIMAL");
assertEqualString($cur->getColumnType(6),"NUMERIC");
assertEqualString($cur->getColumnType('testnumeric'),"NUMERIC");
assertEqualString($cur->getColumnType(7),"MONEY");
assertEqualString($cur->getColumnType('testmoney'),"MONEY");
assertEqualString($cur->getColumnType(8),"SMALLMONEY");
assertEqualString($cur->getColumnType('testsmallmoney'),"SMALLMONEY");
assertEqualString($cur->getColumnType(9),"DATETIME");
assertEqualString($cur->getColumnType('testdatetime'),"DATETIME");
assertEqualString($cur->getColumnType(10),"SMALLDATETIME");
assertEqualString($cur->getColumnType('testsmalldatetime'),"SMALLDATETIME");
assertEqualString($cur->getColumnType(11),"CHAR");
assertEqualString($cur->getColumnType('testchar'),"CHAR");
assertEqualString($cur->getColumnType(12),"CHAR");
assertEqualString($cur->getColumnType('testvarchar'),"CHAR");
assertEqualString($cur->getColumnType(13),"BIT");
assertEqualString($cur->getColumnType('testbit'),"BIT");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEqual($cur->getColumnLength(0),4);
assertEqual($cur->getColumnLength('testint'),4);
assertEqual($cur->getColumnLength(1),2);
assertEqual($cur->getColumnLength('testsmallint'),2);
assertEqual($cur->getColumnLength(2),1);
assertEqual($cur->getColumnLength('testtinyint'),1);
assertEqual($cur->getColumnLength(3),4);
assertEqual($cur->getColumnLength('testreal'),4);
assertEqual($cur->getColumnLength(4),8);
assertEqual($cur->getColumnLength('testfloat'),8);
# these seem to fluctuate with every freetds release
#assertEqual($cur->getColumnLength(5),3);
#assertEqual($cur->getColumnLength('testdecimal'),3);
#assertEqual($cur->getColumnLength(6),3);
#assertEqual($cur->getColumnLength('testnumeric'),3);
assertEqual($cur->getColumnLength(7),8);
assertEqual($cur->getColumnLength('testmoney'),8);
assertEqual($cur->getColumnLength(8),4);
assertEqual($cur->getColumnLength('testsmallmoney'),4);
assertEqual($cur->getColumnLength(9),8);
assertEqual($cur->getColumnLength('testdatetime'),8);
assertEqual($cur->getColumnLength(10),4);
assertEqual($cur->getColumnLength('testsmalldatetime'),4);
# these seem to fluctuate too
#assertEqual($cur->getColumnLength(11),40);
#assertEqual($cur->getColumnLength('testchar'),40);
#assertEqual($cur->getColumnLength(12),40);
#assertEqual($cur->getColumnLength('testvarchar'),40);
assertEqual($cur->getColumnLength(13),1);
assertEqual($cur->getColumnLength('testbit'),1);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEqual($cur->getLongest(0),1);
assertEqual($cur->getLongest('testint'),1);
assertEqual($cur->getLongest(1),1);
assertEqual($cur->getLongest('testsmallint'),1);
assertEqual($cur->getLongest(2),1);
assertEqual($cur->getLongest('testtinyint'),1);
#assertEqual($cur->getLongest(3),3);
#assertEqual($cur->getLongest('testreal'),3);
#assertEqual($cur->getLongest(4),17);
#assertEqual($cur->getLongest('testfloat'),17);
#assertEqual($cur->getLongest(5),3);
#assertEqual($cur->getLongest('testdecimal'),3);
#assertEqual($cur->getLongest(6),3);
#assertEqual($cur->getLongest('testnumeric'),3);
#assertEqual($cur->getLongest(7),4);
#assertEqual($cur->getLongest('testmoney'),4);
#assertEqual($cur->getLongest(8),4);
#assertEqual($cur->getLongest('testsmallmoney'),4);
#assertEqual($cur->getLongest(9),26);
#assertEqual($cur->getLongest('testdatetime'),26);
#assertEqual($cur->getLongest(10),26);
#assertEqual($cur->getLongest('testsmalldatetime'),26);
assertEqual($cur->getLongest(11),40);
assertEqual($cur->getLongest('testchar'),40);
assertEqual($cur->getLongest(12),12);
assertEqual($cur->getLongest('testvarchar'),12);
assertEqual($cur->getLongest(13),1);
assertEqual($cur->getLongest('testbit'),1);
print("\n");


# row count
print("ROW COUNT: \n");
assertEqual($cur->rowCount(),8);
print("\n");


# total rows
print("TOTAL ROWS: \n");
assertEqual($cur->totalRows(),0);
print("\n");


# first row index
print("FIRST ROW INDEX: \n");
assertEqual($cur->firstRowIndex(),0);
print("\n");


# end of result set
print("END OF RESULT SET: \n");
assertTrue($cur->endOfResultSet());
print("\n");


# fields by index
print("FIELDS BY INDEX: \n");
assertEqualString($cur->getField(0,0),"1");
assertEqualString($cur->getField(0,1),"1");
assertEqualString($cur->getField(0,2),"1");
#assertEqualString($cur->getField(0,3),"1.1");
#assertEqualString($cur->getField(0,4),"1.1");
assertEqualString($cur->getField(0,5),"1.1");
assertEqualString($cur->getField(0,6),"1.1");
#assertEqualString($cur->getField(0,7),"1.00");
#assertEqualString($cur->getField(0,8),"1.00");
#assertEqualString($cur->getField(0,9),"Jan  1 2001 01:00:00:000AM");
#assertEqualString($cur->getField(0,10),"Jan  1 2001 01:00:00:000AM");
assertEqualString($cur->getField(0,11),"testchar1                               ");
assertEqualString($cur->getField(0,12),"testvarchar1");
assertEqualString($cur->getField(0,13),"1");
print("\n");
assertEqualString($cur->getField(7,0),"8");
assertEqualString($cur->getField(7,1),"8");
assertEqualString($cur->getField(7,2),"8");
#assertEqualString($cur->getField(7,3),"8.8");
#assertEqualString($cur->getField(7,4),"8.8");
#assertEqualString($cur->getField(7,5),"8.8");
#assertEqualString($cur->getField(7,6),"8.8");
#assertEqualString($cur->getField(7,7),"8.00");
#assertEqualString($cur->getField(7,8),"8.00");
#assertEqualString($cur->getField(7,9),"Jan  1 2008 08:00:00:000AM");
#assertEqualString($cur->getField(7,10),"Jan  1 2008 08:00:00:000AM");
assertEqualString($cur->getField(7,11),"testchar8                               ");
assertEqualString($cur->getField(7,12),"testvarchar8");
assertEqualString($cur->getField(7,13),"1");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEqual($cur->getFieldLength(0,0),1);
assertEqual($cur->getFieldLength(0,1),1);
assertEqual($cur->getFieldLength(0,2),1);
#assertEqual($cur->getFieldLength(0,3),3);
#assertEqual($cur->getFieldLength(0,4),3);
#assertEqual($cur->getFieldLength(0,5),3);
#assertEqual($cur->getFieldLength(0,6),3);
#assertEqual($cur->getFieldLength(0,7),4);
#assertEqual($cur->getFieldLength(0,8),4);
#assertEqual($cur->getFieldLength(0,9),26);
#assertEqual($cur->getFieldLength(0,10),26);
assertEqual($cur->getFieldLength(0,11),40);
assertEqual($cur->getFieldLength(0,12),12);
assertEqual($cur->getFieldLength(0,13),1);
print("\n");
assertEqual($cur->getFieldLength(7,0),1);
assertEqual($cur->getFieldLength(7,1),1);
assertEqual($cur->getFieldLength(7,2),1);
#assertEqual($cur->getFieldLength(7,3),3);
#assertEqual($cur->getFieldLength(7,4),17);
#assertEqual($cur->getFieldLength(7,5),3);
#assertEqual($cur->getFieldLength(7,6),3);
#assertEqual($cur->getFieldLength(7,7),4);
#assertEqual($cur->getFieldLength(7,8),4);
#assertEqual($cur->getFieldLength(7,9),26);
#assertEqual($cur->getFieldLength(7,10),26);
assertEqual($cur->getFieldLength(7,11),40);
assertEqual($cur->getFieldLength(7,12),12);
assertEqual($cur->getFieldLength(7,13),1);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEqualString($cur->getField(0,"testint"),"1");
assertEqualString($cur->getField(0,"testsmallint"),"1");
assertEqualString($cur->getField(0,"testtinyint"),"1");
#assertEqualString($cur->getField(0,"testreal"),"1.1");
#assertEqualString($cur->getField(0,"testfloat"),"1.1");
assertEqualString($cur->getField(0,"testdecimal"),"1.1");
assertEqualString($cur->getField(0,"testnumeric"),"1.1");
#assertEqualString($cur->getField(0,"testmoney"),"1.00");
#assertEqualString($cur->getField(0,"testsmallmoney"),"1.00");
#assertEqualString($cur->getField(0,"testdatetime"),"Jan  1 2001 01:00:00:000AM");
#assertEqualString($cur->getField(0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM");
assertEqualString($cur->getField(0,"testchar"),"testchar1                               ");
assertEqualString($cur->getField(0,"testvarchar"),"testvarchar1");
assertEqualString($cur->getField(0,"testbit"),"1");
print("\n");
assertEqualString($cur->getField(7,"testint"),"8");
assertEqualString($cur->getField(7,"testsmallint"),"8");
assertEqualString($cur->getField(7,"testtinyint"),"8");
#assertEqualString($cur->getField(7,"testreal"),"8.8");
#assertEqualString($cur->getField(7,"testfloat"),"8.8");
#assertEqualString($cur->getField(7,"testdecimal"),"8.8");
#assertEqualString($cur->getField(7,"testnumeric"),"8.8");
#assertEqualString($cur->getField(7,"testmoney"),"8.00");
#assertEqualString($cur->getField(7,"testsmallmoney"),"8.00");
#assertEqualString($cur->getField(7,"testdatetime"),"Jan  1 2008 08:00:00:000AM");
#assertEqualString($cur->getField(7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM");
assertEqualString($cur->getField(7,"testchar"),"testchar8                               ");
assertEqualString($cur->getField(7,"testvarchar"),"testvarchar8");
assertEqualString($cur->getField(7,"testbit"),"1");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEqual($cur->getFieldLength(0,"testint"),1);
assertEqual($cur->getFieldLength(0,"testsmallint"),1);
assertEqual($cur->getFieldLength(0,"testtinyint"),1);
#assertEqual($cur->getFieldLength(0,"testreal"),3);
#assertEqual($cur->getFieldLength(0,"testfloat"),3);
#assertEqual($cur->getFieldLength(0,"testdecimal"),3);
#assertEqual($cur->getFieldLength(0,"testnumeric"),3);
#assertEqual($cur->getFieldLength(0,"testmoney"),4);
#assertEqual($cur->getFieldLength(0,"testsmallmoney"),4);
#assertEqual($cur->getFieldLength(0,"testdatetime"),26);
#assertEqual($cur->getFieldLength(0,"testsmalldatetime"),26);
assertEqual($cur->getFieldLength(0,"testchar"),40);
assertEqual($cur->getFieldLength(0,"testvarchar"),12);
assertEqual($cur->getFieldLength(0,"testbit"),1);
print("\n");
assertEqual($cur->getFieldLength(7,"testint"),1);
assertEqual($cur->getFieldLength(7,"testsmallint"),1);
assertEqual($cur->getFieldLength(7,"testtinyint"),1);
#assertEqual($cur->getFieldLength(7,"testreal"),3);
#assertEqual($cur->getFieldLength(7,"testfloat"),17);
#assertEqual($cur->getFieldLength(7,"testdecimal"),3);
#assertEqual($cur->getFieldLength(7,"testnumeric"),3);
#assertEqual($cur->getFieldLength(7,"testmoney"),4);
#assertEqual($cur->getFieldLength(7,"testsmallmoney"),4);
#assertEqual($cur->getFieldLength(7,"testdatetime"),26);
#assertEqual($cur->getFieldLength(7,"testsmalldatetime"),26);
assertEqual($cur->getFieldLength(7,"testchar"),40);
assertEqual($cur->getFieldLength(7,"testvarchar"),12);
assertEqual($cur->getFieldLength(7,"testbit"),1);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEqual($fields[0],1);
assertEqual($fields[1],1);
assertEqual($fields[2],1);
#assertEqual($fields[3],1.1);
#assertEqual($fields[4],1.1);
assertEqual($fields[5],1.1);
assertEqual($fields[6],1.1);
#assertEqual($fields[7],1.0);
#assertEqual($fields[8],1.0);
#assertEqualString($fields[9],"Jan  1 2001 01:00:00:000AM");
#assertEqualString($fields[10],"Jan  1 2001 01:00:00:000AM");
assertEqualString($fields[11],"testchar1                               ");
assertEqualString($fields[12],"testvarchar1");
assertEqual($fields[13],1);
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEqual($fieldlens[0],1);
assertEqual($fieldlens[1],1);
assertEqual($fieldlens[2],1);
#assertEqual($fieldlens[3],3);
#assertEqual($fieldlens[4],3);
assertEqual($fieldlens[5],3);
assertEqual($fieldlens[6],3);
#assertEqual($fieldlens[7],4);
#assertEqual($fieldlens[8],4);
#assertEqual($fieldlens[9],26);
#assertEqual($fieldlens[10],26);
assertEqual($fieldlens[11],40);
assertEqual($fieldlens[12],12);
assertEqual($fieldlens[13],1);
print("\n");


# fields by hash
print("FIELDS BY HASH: \n");
%fields=$cur->getRowHash(0);
assertEqual($fields{"testint"},1);
assertEqual($fields{"testsmallint"},1);
assertEqual($fields{"testtinyint"},1);
#assertEqual($fields{"testreal"},1.1);
#assertEqual($fields{"testfloat"},1.1);
assertEqual($fields{"testdecimal"},1.1);
assertEqual($fields{"testnumeric"},1.1);
#assertEqual($fields{"testmoney"},1.0);
#assertEqual($fields{"testsmallmoney"},1.0);
#assertEqualString($fields{"testdatetime"},"Jan  1 2001 01:00:00:000AM");
#assertEqualString($fields{"testsmalldatetime"},"Jan  1 2001 01:00:00:000AM");
assertEqualString($fields{"testchar"},"testchar1                               ");
assertEqualString($fields{"testvarchar"},"testvarchar1");
assertEqual($fields{"testbit"},1);
print("\n");
%fields=$cur->getRowHash(7);
assertEqual($fields{"testint"},8);
assertEqual($fields{"testsmallint"},8);
assertEqual($fields{"testtinyint"},8);
#assertEqual($fields{"testreal"},8.8);
#assertEqual($fields{"testfloat"},8.8);
#assertEqual($fields{"testdecimal"},8.8);
#assertEqual($fields{"testnumeric"},8.8);
#assertEqual($fields{"testmoney"},8.0);
#assertEqual($fields{"testsmallmoney"},8.0);
#assertEqualString($fields{"testdatetime"},"Jan  1 2008 08:00:00:000AM");
#assertEqualString($fields{"testsmalldatetime"},"Jan  1 2008 08:00:00:000AM");
assertEqualString($fields{"testchar"},"testchar8                               ");
assertEqualString($fields{"testvarchar"},"testvarchar8");
assertEqual($fields{"testbit"},1);
print("\n");


# field lengths by hash
print("FIELD LENGTHS BY HASH: \n");
%fieldlengths=$cur->getRowLengthsHash(0);
assertEqual($fieldlengths{"testint"},1);
assertEqual($fieldlengths{"testsmallint"},1);
assertEqual($fieldlengths{"testtinyint"},1);
#assertEqual($fieldlengths{"testreal"},3);
#assertEqual($fieldlengths{"testfloat"},3);
#assertEqual($fieldlengths{"testdecimal"},3);
#assertEqual($fieldlengths{"testnumeric"},3);
#assertEqual($fieldlengths{"testmoney"},4);
#assertEqual($fieldlengths{"testsmallmoney"},4);
#assertEqual($fieldlengths{"testdatetime"},26);
#assertEqual($fieldlengths{"testsmalldatetime"},26);
assertEqual($fieldlengths{"testchar"},40);
assertEqual($fieldlengths{"testvarchar"},12);
assertEqual($fieldlengths{"testbit"},1);
print("\n");
%fieldlengths=$cur->getRowLengthsHash(7);
assertEqual($fieldlengths{"testsmallint"},1);
assertEqual($fieldlengths{"testtinyint"},1);
#assertEqual($fieldlengths{"testreal"},3);
#assertEqual($fieldlengths{"testfloat"},17);
#assertEqual($fieldlengths{"testdecimal"},3);
#assertEqual($fieldlengths{"testnumeric"},3);
#assertEqual($fieldlengths{"testmoney"},4);
#assertEqual($fieldlengths{"testsmallmoney"},4);
#assertEqual($fieldlengths{"testdatetime"},26);
#assertEqual($fieldlengths{"testsmalldatetime"},26);
assertEqual($fieldlengths{"testchar"},40);
assertEqual($fieldlengths{"testvarchar"},12);
assertEqual($fieldlengths{"testbit"},1);
print("\n");


# individual substitutions
print("INDIVIDUAL SUBSTITUTIONS: \n");
$cur->prepareQuery("select \$(var1),'\$(var2)',\$(var3)");
$cur->substitution("var1",1);
$cur->substitution("var2","hello");
$cur->substitution("var3",10.5556,6,4);
assertTrue($cur->executeQuery());
print("\n");


# fields
print("FIELDS: \n");
assertEqualString($cur->getField(0,0),"1");
assertEqualString($cur->getField(0,1),"hello");
assertEqualString($cur->getField(0,2),"10.5556");
print("\n");


# array substitutions
print("ARRAY SUBSTITUTIONS: \n");
$cur->prepareQuery("select \$(var1),'\$(var2)',\$(var3)");
@vars=("var1","var2","var3");
@vals=(1,"hello",10.5556);
@precs=(0,0,6);
@scales=(0,0,4);
$cur->substitutions(\@vars,\@vals,\@precs,\@scales);
assertTrue($cur->executeQuery());
print("\n");


# fields
print("FIELDS: \n");
assertEqualString($cur->getField(0,0),"1");
assertEqualString($cur->getField(0,1),"hello");
assertEqualString($cur->getField(0,2),"10.5556");
print("\n");


# nulls as undef
print("NULLS as Undef: \n");
$cur->getNullsAsUndefined();
assertTrue($cur->sendQuery("select NULL,1,NULL"));
assertUndef($cur->getField(0,0));
assertEqualString($cur->getField(0,1),"1");
assertUndef($cur->getField(0,2));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("select NULL,1,NULL"));
assertEqualString($cur->getField(0,0),"");
assertEqualString($cur->getField(0,1),"1");
assertEqualString($cur->getField(0,2),"");
$cur->getNullsAsUndefined();
print("\n");


# result set buffer size
print("RESULT SET BUFFER SIZE: \n");
assertEqual($cur->getResultSetBufferSize(),0);
$cur->setResultSetBufferSize(2);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEqual($cur->getResultSetBufferSize(),2);
print("\n");
assertEqual($cur->firstRowIndex(),0);
assertFalse($cur->endOfResultSet());
assertEqual($cur->rowCount(),2);
assertEqualString($cur->getField(0,0),"1");
assertEqualString($cur->getField(1,0),"2");
assertEqualString($cur->getField(2,0),"3");
print("\n");
assertEqual($cur->firstRowIndex(),2);
assertFalse($cur->endOfResultSet());
assertEqual($cur->rowCount(),4);
assertEqualString($cur->getField(6,0),"7");
assertEqualString($cur->getField(7,0),"8");
print("\n");
assertEqual($cur->firstRowIndex(),6);
assertFalse($cur->endOfResultSet());
assertEqual($cur->rowCount(),8);
assertUndef($cur->getField(8,0));
print("\n");
assertEqual($cur->firstRowIndex(),8);
assertTrue($cur->endOfResultSet());
assertEqual($cur->rowCount(),8);
print("\n");


# dont get column info
print("DONT GET COLUMN INFO: \n");
$cur->dontGetColumnInfo();
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertUndef($cur->getColumnName(0));
assertEqual($cur->getColumnLength(0),0);
assertUndef($cur->getColumnType(0));
$cur->getColumnInfo();
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEqualString($cur->getColumnName(0),"testint");
assertEqual($cur->getColumnLength(0),4);
assertEqualString($cur->getColumnType(0),"INT");
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
assertEqualString($cur->getField(0,0),"1");
assertEqualString($cur->getField(1,0),"2");
assertEqualString($cur->getField(2,0),"3");
assertEqualString($cur->getField(3,0),"4");
assertEqualString($cur->getField(4,0),"5");
assertEqualString($cur->getField(5,0),"6");
assertEqualString($cur->getField(6,0),"7");
assertEqualString($cur->getField(7,0),"8");
print("\n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
assertEqualString($cur->getField(0,0),"1");
assertEqualString($cur->getField(1,0),"2");
assertEqualString($cur->getField(2,0),"3");
assertEqualString($cur->getField(3,0),"4");
assertEqualString($cur->getField(4,0),"5");
assertEqualString($cur->getField(5,0),"6");
assertEqualString($cur->getField(6,0),"7");
assertEqualString($cur->getField(7,0),"8");
print("\n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
print("\n");
assertEqualString($cur->getField(0,0),"1");
assertEqualString($cur->getField(1,0),"2");
assertEqualString($cur->getField(2,0),"3");
assertEqualString($cur->getField(3,0),"4");
assertEqualString($cur->getField(4,0),"5");
assertEqualString($cur->getField(5,0),"6");
assertEqualString($cur->getField(6,0),"7");
assertEqualString($cur->getField(7,0),"8");
print("\n");


# suspended result set
print("SUSPENDED RESULT SET: \n");
$cur->setResultSetBufferSize(2);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEqualString($cur->getField(2,0),"3");
$id=$cur->getResultSetId();
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
assertTrue($con->resumeSession($port,$socket));
assertTrue($cur->resumeResultSet($id));
print("\n");
assertEqual($cur->firstRowIndex(),4);
assertFalse($cur->endOfResultSet());
assertEqual($cur->rowCount(),6);
assertEqualString($cur->getField(7,0),"8");
print("\n");
assertEqual($cur->firstRowIndex(),6);
assertFalse($cur->endOfResultSet());
assertEqual($cur->rowCount(),8);
assertUndef($cur->getField(8,0));
print("\n");
assertEqual($cur->firstRowIndex(),8);
assertTrue($cur->endOfResultSet());
assertEqual($cur->rowCount(),8);
$cur->setResultSetBufferSize(0);
print("\n");


# cached result set
print("CACHED RESULT SET: \n");
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$filename=$cur->getCacheFileName();
assertEqualString($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEqualString($cur->getField(7,0),"8");
print("\n");


# column count for cached result set
print("COLUMN COUNT FOR CACHED RESULT SET: \n");
assertEqual($cur->colCount(),14);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEqualString($cur->getColumnName(0),"testint");
assertEqualString($cur->getColumnName(1),"testsmallint");
assertEqualString($cur->getColumnName(2),"testtinyint");
assertEqualString($cur->getColumnName(3),"testreal");
assertEqualString($cur->getColumnName(4),"testfloat");
assertEqualString($cur->getColumnName(5),"testdecimal");
assertEqualString($cur->getColumnName(6),"testnumeric");
assertEqualString($cur->getColumnName(7),"testmoney");
assertEqualString($cur->getColumnName(8),"testsmallmoney");
assertEqualString($cur->getColumnName(9),"testdatetime");
assertEqualString($cur->getColumnName(10),"testsmalldatetime");
assertEqualString($cur->getColumnName(11),"testchar");
assertEqualString($cur->getColumnName(12),"testvarchar");
assertEqualString($cur->getColumnName(13),"testbit");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"testint");
assertEqualString($cols[1],"testsmallint");
assertEqualString($cols[2],"testtinyint");
assertEqualString($cols[3],"testreal");
assertEqualString($cols[4],"testfloat");
assertEqualString($cols[5],"testdecimal");
assertEqualString($cols[6],"testnumeric");
assertEqualString($cols[7],"testmoney");
assertEqualString($cols[8],"testsmallmoney");
assertEqualString($cols[9],"testdatetime");
assertEqualString($cols[10],"testsmalldatetime");
assertEqualString($cols[11],"testchar");
assertEqualString($cols[12],"testvarchar");
assertEqualString($cols[13],"testbit");
print("\n");


# cached result set with result set buffer size
print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
$filename=$cur->getCacheFileName();
assertEqualString($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEqualString($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
$cur->setResultSetBufferSize(0);
print("\n");


# from one cache file to another
print("FROM ONE CACHE FILE TO ANOTHER: \n");
$cur->cacheToFile("cachefile2");
assertTrue($cur->openCachedResultSet("cachefile1"));
$cur->cacheOff();
assertTrue($cur->openCachedResultSet("cachefile2"));
assertEqualString($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
print("\n");


# from one cache file to another with result set buffer size
print("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile2");
assertTrue($cur->openCachedResultSet("cachefile1"));
$cur->cacheOff();
assertTrue($cur->openCachedResultSet("cachefile2"));
assertEqualString($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
$cur->setResultSetBufferSize(0);
print("\n");


# cached result set with suspend and result set buffer size
print("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEqualString($cur->getField(2,0),"3");
$filename=$cur->getCacheFileName();
assertEqualString($filename,"cachefile1");
$id=$cur->getResultSetId();
$cur->suspendResultSet();
assertTrue($con->suspendSession());
$port=$con->getConnectionPort();
$socket=$con->getConnectionSocket();
print("\n");
assertTrue($con->resumeSession($port,$socket));
assertTrue($cur->resumeCachedResultSet($id,$filename));
print("\n");
assertEqual($cur->firstRowIndex(),4);
assertFalse($cur->endOfResultSet());
assertEqual($cur->rowCount(),6);
assertEqualString($cur->getField(7,0),"8");
print("\n");
assertEqual($cur->firstRowIndex(),6);
assertFalse($cur->endOfResultSet());
assertEqual($cur->rowCount(),8);
assertUndef($cur->getField(8,0));
print("\n");
assertEqual($cur->firstRowIndex(),8);
assertTrue($cur->endOfResultSet());
assertEqual($cur->rowCount(),8);
$cur->cacheOff();
print("\n");
assertTrue($cur->openCachedResultSet($filename));
assertEqualString($cur->getField(7,0),"8");
assertUndef($cur->getField(8,0));
$cur->setResultSetBufferSize(0);
print("\n");


# finished suspended session
print("FINISHED SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
assertEqualString($cur->getField(4,0),"5");
assertEqualString($cur->getField(5,0),"6");
assertEqualString($cur->getField(6,0),"7");
assertEqualString($cur->getField(7,0),"8");
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

# drop existing table
$cur->sendQuery("commit tran");
$cur->sendQuery("drop table testtable");

# invalid queries...


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
