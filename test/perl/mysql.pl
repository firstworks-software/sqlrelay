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
assertEqualString($con->identify(),"mysql");
print("\n");

# get the db version
$dbversion=$con->dbVersion();
$majorversion=int(substr($dbversion,0,1));


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");


# isolation levels
print("ISOLATION LEVELS: \n");
@isolationlevels=("REPEATABLE-READ","READ-UNCOMMITTED",
		"READ-COMMITTED","SERIALIZABLE");
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

# create a new table


# create temptable
print("CREATE TEMPTABLE: \n");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testtinyint tinyint, ".
	"	testsmallint smallint, ".
	"	testmediumint mediumint, ".
	"	testint int, ".
	"	testbigint bigint, ".
	"	testfloat float, ".
	"	testreal real, ".
	"	testdecimal decimal(2,1), ".
	"	testdate date, ".
	"	testtime time, ".
	"	testdatetime datetime, ".
	"	testyear year, ".
	"	testchar char(40), ".
	"	testtext text, ".
	"	testvarchar varchar(40), ".
	"	testtinytext tinytext, ".
	"	testmediumtext mediumtext, ".
	"	testlongtext longtext, ".
	"	testtimestamp timestamp)"));
print("\n");


# begin transaction
print("BEGIN TRANSACTION: \n");
assertTrue($cur->sendQuery("begin"));
print("\n");


# insert
print("INSERT: \n");
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	1, ".
	"	1, ".
	"	1, ".
	"	1, ".
	"	1, ".
	"	1.1, ".
	"	1.1, ".
	"	1.1, ".
	"	'2001-01-01', ".
	"	'01:00:00', ".
	"	'2001-01-01 01:00:00', ".
	"	'2001', ".
	"	'char1', ".
	"	'text1', ".
	"	'varchar1', ".
	"	'tinytext1', ".
	"	'mediumtext1', ".
	"	'longtext1', ".
	"	NULL)"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	2, ".
	"	2, ".
	"	2, ".
	"	2, ".
	"	2, ".
	"	2.1, ".
	"	2.1, ".
	"	2.1, ".
	"	'2002-01-01', ".
	"	'02:00:00', ".
	"	'2002-01-01 02:00:00', ".
	"	'2002', ".
	"	'char2', ".
	"	'text2', ".
	"	'varchar2', ".
	"	'tinytext2', ".
	"	'mediumtext2', ".
	"	'longtext2', ".
	"	NULL)"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	3, ".
	"	3, ".
	"	3, ".
	"	3, ".
	"	3, ".
	"	3.1, ".
	"	3.1, ".
	"	3.1, ".
	"	'2003-01-01', ".
	"	'03:00:00', ".
	"	'2003-01-01 03:00:00', ".
	"	'2003', ".
	"	'char3', ".
	"	'text3', ".
	"	'varchar3', ".
	"	'tinytext3', ".
	"	'mediumtext3', ".
	"	'longtext3', ".
	"	NULL)"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	4, ".
	"	4, ".
	"	4, ".
	"	4, ".
	"	4, ".
	"	4.1, ".
	"	4.1, ".
	"	4.1, ".
	"	'2004-01-01', ".
	"	'04:00:00', ".
	"	'2004-01-01 04:00:00', ".
	"	'2004', ".
	"	'char4', ".
	"	'text4', ".
	"	'varchar4', ".
	"	'tinytext4', ".
	"	'mediumtext4', ".
	"	'longtext4', ".
	"	NULL)"));
print("\n");


# affected rows
print("AFFECTED ROWS: \n");
assertEqual($cur->affectedRows(),1);
print("\n");


# bind by position
print("BIND BY POSITION: \n");
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
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	?, ".
	"	NULL)");
assertEqual($cur->countBindVariables(),18);
$cur->inputBind("1",5);
$cur->inputBind("2",5);
$cur->inputBind("3",5);
$cur->inputBind("4",5);
$cur->inputBind("5",5);
$cur->inputBind("6",5.1,2,1);
$cur->inputBind("7",5.1,2,1);
$cur->inputBind("8",5.1,2,1);
$cur->inputBind("9","2005-01-01");
$cur->inputBind("10","05:00:00");
$cur->inputBind("11","2005-01-01 05:00:00");
$cur->inputBind("12","2005");
$cur->inputBind("13","char5");
$cur->inputBind("14","text5");
$cur->inputBind("15","varchar5");
$cur->inputBind("16","tinytext5");
$cur->inputBind("17","mediumtext5");
$cur->inputBind("18","longtext5");
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",6);
$cur->inputBind("2",6);
$cur->inputBind("3",6);
$cur->inputBind("4",6);
$cur->inputBind("5",6);
$cur->inputBind("6",6.1,2,1);
$cur->inputBind("7",6.1,2,1);
$cur->inputBind("8",6.1,2,1);
$cur->inputBind("9",'2006-01-01');
$cur->inputBind("10",'06:00:00');
$cur->inputBind("11",'2006-01-01 06:00:00');
$cur->inputBind("12",'2006');
$cur->inputBind("13",'char6');
$cur->inputBind("14",'text6');
$cur->inputBind("15",'varchar6');
$cur->inputBind("16",'tinytext6');
$cur->inputBind("17",'mediumtext6');
$cur->inputBind("18",'longtext6');
assertTrue($cur->executeQuery());
print("\n");


# array of binds by position
print("ARRAY OF BINDS BY POSITION: \n");
$cur->clearBinds();
@vars=("1","2","3","4","5","6",
		"7","8","9","10","11","12",
		"13","14","15",
		"16","17","18");
@vals=(7,7,7,7,7,7.1,7.1,7.1,
	'2007-01-01','07:00:00','2007-01-01 07:00:00',
	'2007','char7','text7','varchar7',
	'tinytext7','mediumtext7','longtext7');
@precs=(0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0);
@scales=(0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0);
$cur->inputBinds(\@vars,\@vals,\@precs,\@scales);
assertTrue($cur->executeQuery());
print("\n");


# bind by position with validation
print("BIND BY POSITION WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("1",8);
$cur->inputBind("2",8);
$cur->inputBind("3",8);
$cur->inputBind("4",8);
$cur->inputBind("5",8);
$cur->inputBind("6",8.1,2,1);
$cur->inputBind("7",8.1,2,1);
$cur->inputBind("8",8.1,2,1);
$cur->inputBind("9",'2008-01-01');
$cur->inputBind("10",'08:00:00');
$cur->inputBind("11",'2008-01-01 08:00:00');
$cur->inputBind("12",'2008');
$cur->inputBind("13",'char8');
$cur->inputBind("14",'text8');
$cur->inputBind("15",'varchar8');
$cur->inputBind("16",'tinytext8');
$cur->inputBind("17",'mediumtext8');
$cur->inputBind("18",'longtext8');
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# select
print("SELECT: \n");
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEqual($cur->colCount(),19);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEqualString($cur->getColumnName(0),"testtinyint");
assertEqualString($cur->getColumnName(1),"testsmallint");
assertEqualString($cur->getColumnName(2),"testmediumint");
assertEqualString($cur->getColumnName(3),"testint");
assertEqualString($cur->getColumnName(4),"testbigint");
assertEqualString($cur->getColumnName(5),"testfloat");
assertEqualString($cur->getColumnName(6),"testreal");
assertEqualString($cur->getColumnName(7),"testdecimal");
assertEqualString($cur->getColumnName(8),"testdate");
assertEqualString($cur->getColumnName(9),"testtime");
assertEqualString($cur->getColumnName(10),"testdatetime");
assertEqualString($cur->getColumnName(11),"testyear");
assertEqualString($cur->getColumnName(12),"testchar");
assertEqualString($cur->getColumnName(13),"testtext");
assertEqualString($cur->getColumnName(14),"testvarchar");
assertEqualString($cur->getColumnName(15),"testtinytext");
assertEqualString($cur->getColumnName(16),"testmediumtext");
assertEqualString($cur->getColumnName(17),"testlongtext");
assertEqualString($cur->getColumnName(18),"testtimestamp");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"testtinyint");
assertEqualString($cols[1],"testsmallint");
assertEqualString($cols[2],"testmediumint");
assertEqualString($cols[3],"testint");
assertEqualString($cols[4],"testbigint");
assertEqualString($cols[5],"testfloat");
assertEqualString($cols[6],"testreal");
assertEqualString($cols[7],"testdecimal");
assertEqualString($cols[8],"testdate");
assertEqualString($cols[9],"testtime");
assertEqualString($cols[10],"testdatetime");
assertEqualString($cols[11],"testyear");
assertEqualString($cols[12],"testchar");
assertEqualString($cols[13],"testtext");
assertEqualString($cols[14],"testvarchar");
assertEqualString($cols[15],"testtinytext");
assertEqualString($cols[16],"testmediumtext");
assertEqualString($cols[17],"testlongtext");
assertEqualString($cols[18],"testtimestamp");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEqualString($cur->getColumnType(0),"TINYINT");
assertEqualString($cur->getColumnType(1),"SMALLINT");
assertEqualString($cur->getColumnType(2),"MEDIUMINT");
assertEqualString($cur->getColumnType(3),"INT");
assertEqualString($cur->getColumnType(4),"BIGINT");
assertEqualString($cur->getColumnType(5),"FLOAT");
assertEqualString($cur->getColumnType(6),"REAL");
assertEqualString($cur->getColumnType(7),"DECIMAL");
assertEqualString($cur->getColumnType(8),"DATE");
assertEqualString($cur->getColumnType(9),"TIME");
assertEqualString($cur->getColumnType(10),"DATETIME");
assertEqualString($cur->getColumnType(11),"YEAR");
if ($majorversion==3) {
	assertEqualString($cur->getColumnType(12),"VARSTRING");
} else {
	assertEqualString($cur->getColumnType(12),"STRING");
}
assertEqualString($cur->getColumnType(13),"BLOB");
assertEqualString($cur->getColumnType(14),"VARSTRING");
assertEqualString($cur->getColumnType(15),"TINYBLOB");
assertEqualString($cur->getColumnType(16),"MEDIUMBLOB");
assertEqualString($cur->getColumnType(17),"LONGBLOB");
assertEqualString($cur->getColumnType(18),"TIMESTAMP");
assertEqualString($cur->getColumnType("testtinyint"),"TINYINT");
assertEqualString($cur->getColumnType("testsmallint"),"SMALLINT");
assertEqualString($cur->getColumnType("testmediumint"),"MEDIUMINT");
assertEqualString($cur->getColumnType("testint"),"INT");
assertEqualString($cur->getColumnType("testbigint"),"BIGINT");
assertEqualString($cur->getColumnType("testfloat"),"FLOAT");
assertEqualString($cur->getColumnType("testreal"),"REAL");
assertEqualString($cur->getColumnType("testdecimal"),"DECIMAL");
assertEqualString($cur->getColumnType("testdate"),"DATE");
assertEqualString($cur->getColumnType("testtime"),"TIME");
assertEqualString($cur->getColumnType("testdatetime"),"DATETIME");
assertEqualString($cur->getColumnType("testyear"),"YEAR");
if ($majorversion==3) {
	assertEqualString($cur->getColumnType("testchar"),"VARSTRING");
} else {
	assertEqualString($cur->getColumnType("testchar"),"STRING");
}
assertEqualString($cur->getColumnType("testtext"),"BLOB");
assertEqualString($cur->getColumnType("testvarchar"),"VARSTRING");
assertEqualString($cur->getColumnType("testtinytext"),"TINYBLOB");
assertEqualString($cur->getColumnType("testmediumtext"),"MEDIUMBLOB");
assertEqualString($cur->getColumnType("testlongtext"),"LONGBLOB");
assertEqualString($cur->getColumnType("testtimestamp"),"TIMESTAMP");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEqual($cur->getColumnLength(0),1);
assertEqual($cur->getColumnLength(1),2);
assertEqual($cur->getColumnLength(2),3);
assertEqual($cur->getColumnLength(3),4);
assertEqual($cur->getColumnLength(4),8);
assertEqual($cur->getColumnLength(5),4);
assertEqual($cur->getColumnLength(6),8);
assertEqual($cur->getColumnLength(7),6);
assertEqual($cur->getColumnLength(8),3);
assertEqual($cur->getColumnLength(9),3);
assertEqual($cur->getColumnLength(10),8);
assertEqual($cur->getColumnLength(11),1);
#assertEqual($cur->getColumnLength(12),40);
assertEqual($cur->getColumnLength(13),65535);
#assertEqual($cur->getColumnLength(14),41);
assertEqual($cur->getColumnLength(15),255);
assertEqual($cur->getColumnLength(16),16777215);
assertEqual($cur->getColumnLength(17),2147483647);
assertEqual($cur->getColumnLength(18),4);
assertEqual($cur->getColumnLength("testtinyint"),1);
assertEqual($cur->getColumnLength("testsmallint"),2);
assertEqual($cur->getColumnLength("testmediumint"),3);
assertEqual($cur->getColumnLength("testint"),4);
assertEqual($cur->getColumnLength("testbigint"),8);
assertEqual($cur->getColumnLength("testfloat"),4);
assertEqual($cur->getColumnLength("testreal"),8);
assertEqual($cur->getColumnLength("testdecimal"),6);
assertEqual($cur->getColumnLength("testdate"),3);
assertEqual($cur->getColumnLength("testtime"),3);
assertEqual($cur->getColumnLength("testdatetime"),8);
assertEqual($cur->getColumnLength("testyear"),1);
#assertEqual($cur->getColumnLength("testchar"),40);
assertEqual($cur->getColumnLength("testtext"),65535);
#assertEqual($cur->getColumnLength("testvarchar"),41);
assertEqual($cur->getColumnLength("testtinytext"),255);
assertEqual($cur->getColumnLength("testmediumtext"),16777215);
assertEqual($cur->getColumnLength("testlongtext"),2147483647);
assertEqual($cur->getColumnLength("testtimestamp"),4);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEqual($cur->getLongest(0),1);
assertEqual($cur->getLongest(1),1);
assertEqual($cur->getLongest(2),1);
assertEqual($cur->getLongest(3),1);
assertEqual($cur->getLongest(4),1);
#assertEqual($cur->getLongest(5),3);
assertEqual($cur->getLongest(6),3);
assertEqual($cur->getLongest(7),3);
assertEqual($cur->getLongest(8),10);
assertEqual($cur->getLongest(9),8);
assertEqual($cur->getLongest(10),19);
assertEqual($cur->getLongest(11),4);
assertEqual($cur->getLongest(12),5);
assertEqual($cur->getLongest(13),5);
assertEqual($cur->getLongest(14),8);
assertEqual($cur->getLongest(15),9);
assertEqual($cur->getLongest(16),11);
assertEqual($cur->getLongest(17),9);
if ($majorversion==3) {
	assertEqual($cur->getLongest(18),14);
} else {
	assertEqual($cur->getLongest(18),19);
}
assertEqual($cur->getLongest("testtinyint"),1);
assertEqual($cur->getLongest("testsmallint"),1);
assertEqual($cur->getLongest("testmediumint"),1);
assertEqual($cur->getLongest("testint"),1);
assertEqual($cur->getLongest("testbigint"),1);
#assertEqual($cur->getLongest("testfloat"),3);
assertEqual($cur->getLongest("testreal"),3);
assertEqual($cur->getLongest("testdecimal"),3);
assertEqual($cur->getLongest("testdate"),10);
assertEqual($cur->getLongest("testtime"),8);
assertEqual($cur->getLongest("testdatetime"),19);
assertEqual($cur->getLongest("testyear"),4);
assertEqual($cur->getLongest("testchar"),5);
assertEqual($cur->getLongest("testtext"),5);
assertEqual($cur->getLongest("testvarchar"),8);
assertEqual($cur->getLongest("testtinytext"),9);
assertEqual($cur->getLongest("testmediumtext"),11);
assertEqual($cur->getLongest("testlongtext"),9);
if ($majorversion==3) {
	assertEqual($cur->getLongest("testtimestamp"),14);
} else {
	assertEqual($cur->getLongest("testtimestamp"),19);
}
print("\n");


# row count
print("ROW COUNT: \n");
assertEqual($cur->rowCount(),8);
print("\n");


# total rows
print("TOTAL ROWS: \n");
# older versions of mysql know this
#assertEqual($cur->totalRows(),0);
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
assertEqualString($cur->getField(0,3),"1");
assertEqualString($cur->getField(0,4),"1");
#assertEqualString($cur->getField(0,5),"1.1");
assertEqualString($cur->getField(0,6),"1.1");
assertEqualString($cur->getField(0,7),"1.1");
assertEqualString($cur->getField(0,8),"2001-01-01");
assertEqualString($cur->getField(0,9),"01:00:00");
assertEqualString($cur->getField(0,10),"2001-01-01 01:00:00");
assertEqualString($cur->getField(0,11),"2001");
assertEqualString($cur->getField(0,12),"char1");
assertEqualString($cur->getField(0,13),"text1");
assertEqualString($cur->getField(0,14),"varchar1");
assertEqualString($cur->getField(0,15),"tinytext1");
assertEqualString($cur->getField(0,16),"mediumtext1");
assertEqualString($cur->getField(0,17),"longtext1");
print("\n");
assertEqualString($cur->getField(7,0),"8");
assertEqualString($cur->getField(7,1),"8");
assertEqualString($cur->getField(7,2),"8");
assertEqualString($cur->getField(7,3),"8");
assertEqualString($cur->getField(7,4),"8");
#assertEqualString($cur->getField(7,5),"8.1");
assertEqualString($cur->getField(7,6),"8.1");
assertEqualString($cur->getField(7,7),"8.1");
assertEqualString($cur->getField(7,8),"2008-01-01");
assertEqualString($cur->getField(7,9),"08:00:00");
assertEqualString($cur->getField(7,10),"2008-01-01 08:00:00");
assertEqualString($cur->getField(7,11),"2008");
assertEqualString($cur->getField(7,12),"char8");
assertEqualString($cur->getField(7,13),"text8");
assertEqualString($cur->getField(7,14),"varchar8");
assertEqualString($cur->getField(7,15),"tinytext8");
assertEqualString($cur->getField(7,16),"mediumtext8");
assertEqualString($cur->getField(7,17),"longtext8");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEqual($cur->getFieldLength(0,0),1);
assertEqual($cur->getFieldLength(0,1),1);
assertEqual($cur->getFieldLength(0,2),1);
assertEqual($cur->getFieldLength(0,3),1);
assertEqual($cur->getFieldLength(0,4),1);
#assertEqual($cur->getFieldLength(0,5),3);
assertEqual($cur->getFieldLength(0,6),3);
assertEqual($cur->getFieldLength(0,7),3);
assertEqual($cur->getFieldLength(0,8),10);
assertEqual($cur->getFieldLength(0,9),8);
assertEqual($cur->getFieldLength(0,10),19);
assertEqual($cur->getFieldLength(0,11),4);
assertEqual($cur->getFieldLength(0,12),5);
assertEqual($cur->getFieldLength(0,13),5);
assertEqual($cur->getFieldLength(0,14),8);
assertEqual($cur->getFieldLength(0,15),9);
assertEqual($cur->getFieldLength(0,16),11);
assertEqual($cur->getFieldLength(0,17),9);
print("\n");
assertEqual($cur->getFieldLength(7,0),1);
assertEqual($cur->getFieldLength(7,1),1);
assertEqual($cur->getFieldLength(7,2),1);
assertEqual($cur->getFieldLength(7,3),1);
assertEqual($cur->getFieldLength(7,4),1);
#assertEqual($cur->getFieldLength(7,5),3);
assertEqual($cur->getFieldLength(7,6),3);
assertEqual($cur->getFieldLength(7,7),3);
assertEqual($cur->getFieldLength(7,8),10);
assertEqual($cur->getFieldLength(7,9),8);
assertEqual($cur->getFieldLength(7,10),19);
assertEqual($cur->getFieldLength(7,11),4);
assertEqual($cur->getFieldLength(7,12),5);
assertEqual($cur->getFieldLength(7,13),5);
assertEqual($cur->getFieldLength(7,14),8);
assertEqual($cur->getFieldLength(7,15),9);
assertEqual($cur->getFieldLength(7,16),11);
assertEqual($cur->getFieldLength(7,17),9);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEqualString($cur->getField(0,"testtinyint"),"1");
assertEqualString($cur->getField(0,"testsmallint"),"1");
assertEqualString($cur->getField(0,"testmediumint"),"1");
assertEqualString($cur->getField(0,"testint"),"1");
assertEqualString($cur->getField(0,"testbigint"),"1");
#assertEqualString($cur->getField(0,"testfloat"),"1.1");
assertEqualString($cur->getField(0,"testreal"),"1.1");
assertEqualString($cur->getField(0,"testdecimal"),"1.1");
assertEqualString($cur->getField(0,"testdate"),"2001-01-01");
assertEqualString($cur->getField(0,"testtime"),"01:00:00");
assertEqualString($cur->getField(0,"testdatetime"),"2001-01-01 01:00:00");
assertEqualString($cur->getField(0,"testyear"),"2001");
assertEqualString($cur->getField(0,"testchar"),"char1");
assertEqualString($cur->getField(0,"testtext"),"text1");
assertEqualString($cur->getField(0,"testvarchar"),"varchar1");
assertEqualString($cur->getField(0,"testtinytext"),"tinytext1");
assertEqualString($cur->getField(0,"testmediumtext"),"mediumtext1");
assertEqualString($cur->getField(0,"testlongtext"),"longtext1");
print("\n");
assertEqualString($cur->getField(7,"testtinyint"),"8");
assertEqualString($cur->getField(7,"testsmallint"),"8");
assertEqualString($cur->getField(7,"testmediumint"),"8");
assertEqualString($cur->getField(7,"testint"),"8");
assertEqualString($cur->getField(7,"testbigint"),"8");
#assertEqualString($cur->getField(7,"testfloat"),"8.1");
assertEqualString($cur->getField(7,"testreal"),"8.1");
assertEqualString($cur->getField(7,"testdecimal"),"8.1");
assertEqualString($cur->getField(7,"testdate"),"2008-01-01");
assertEqualString($cur->getField(7,"testtime"),"08:00:00");
assertEqualString($cur->getField(7,"testdatetime"),"2008-01-01 08:00:00");
assertEqualString($cur->getField(7,"testyear"),"2008");
assertEqualString($cur->getField(7,"testchar"),"char8");
assertEqualString($cur->getField(7,"testtext"),"text8");
assertEqualString($cur->getField(7,"testvarchar"),"varchar8");
assertEqualString($cur->getField(7,"testtinytext"),"tinytext8");
assertEqualString($cur->getField(7,"testmediumtext"),"mediumtext8");
assertEqualString($cur->getField(7,"testlongtext"),"longtext8");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEqual($cur->getFieldLength(0,"testtinyint"),1);
assertEqual($cur->getFieldLength(0,"testsmallint"),1);
assertEqual($cur->getFieldLength(0,"testmediumint"),1);
assertEqual($cur->getFieldLength(0,"testint"),1);
assertEqual($cur->getFieldLength(0,"testbigint"),1);
#assertEqual($cur->getFieldLength(0,"testfloat"),3);
assertEqual($cur->getFieldLength(0,"testreal"),3);
assertEqual($cur->getFieldLength(0,"testdecimal"),3);
assertEqual($cur->getFieldLength(0,"testdate"),10);
assertEqual($cur->getFieldLength(0,"testtime"),8);
assertEqual($cur->getFieldLength(0,"testdatetime"),19);
assertEqual($cur->getFieldLength(0,"testyear"),4);
assertEqual($cur->getFieldLength(0,"testchar"),5);
assertEqual($cur->getFieldLength(0,"testtext"),5);
assertEqual($cur->getFieldLength(0,"testvarchar"),8);
assertEqual($cur->getFieldLength(0,"testtinytext"),9);
assertEqual($cur->getFieldLength(0,"testmediumtext"),11);
assertEqual($cur->getFieldLength(0,"testlongtext"),9);
print("\n");
assertEqual($cur->getFieldLength(7,"testtinyint"),1);
assertEqual($cur->getFieldLength(7,"testsmallint"),1);
assertEqual($cur->getFieldLength(7,"testmediumint"),1);
assertEqual($cur->getFieldLength(7,"testint"),1);
assertEqual($cur->getFieldLength(7,"testbigint"),1);
#assertEqual($cur->getFieldLength(7,"testfloat"),3);
assertEqual($cur->getFieldLength(7,"testreal"),3);
assertEqual($cur->getFieldLength(7,"testdecimal"),3);
assertEqual($cur->getFieldLength(7,"testdate"),10);
assertEqual($cur->getFieldLength(7,"testtime"),8);
assertEqual($cur->getFieldLength(7,"testdatetime"),19);
assertEqual($cur->getFieldLength(7,"testyear"),4);
assertEqual($cur->getFieldLength(7,"testchar"),5);
assertEqual($cur->getFieldLength(7,"testtext"),5);
assertEqual($cur->getFieldLength(7,"testvarchar"),8);
assertEqual($cur->getFieldLength(7,"testtinytext"),9);
assertEqual($cur->getFieldLength(7,"testmediumtext"),11);
assertEqual($cur->getFieldLength(7,"testlongtext"),9);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEqual($fields[0],1);
assertEqual($fields[1],1);
assertEqual($fields[2],1);
assertEqual($fields[3],1);
assertEqual($fields[4],1);
#assertEqual($fields[5],1.1);
assertEqual($fields[6],1.1);
assertEqual($fields[7],1.1);
assertEqualString($fields[8],"2001-01-01");
assertEqualString($fields[9],"01:00:00");
assertEqualString($fields[10],"2001-01-01 01:00:00");
assertEqual($fields[11],2001);
assertEqualString($fields[12],"char1");
assertEqualString($fields[13],"text1");
assertEqualString($fields[14],"varchar1");
assertEqualString($fields[15],"tinytext1");
assertEqualString($fields[16],"mediumtext1");
assertEqualString($fields[17],"longtext1");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEqual($fieldlens[0],1);
assertEqual($fieldlens[1],1);
assertEqual($fieldlens[2],1);
assertEqual($fieldlens[3],1);
assertEqual($fieldlens[4],1);
#assertEqual($fieldlens[5],3);
assertEqual($fieldlens[6],3);
assertEqual($fieldlens[7],3);
assertEqual($fieldlens[8],10);
assertEqual($fieldlens[9],8);
assertEqual($fieldlens[10],19);
assertEqual($fieldlens[11],4);
assertEqual($fieldlens[12],5);
assertEqual($fieldlens[13],5);
assertEqual($fieldlens[14],8);
assertEqual($fieldlens[15],9);
assertEqual($fieldlens[16],11);
assertEqual($fieldlens[17],9);
print("\n");


# fields by hash
print("FIELDS BY HASH: \n");
%fields=$cur->getRowHash(0);
assertEqual($fields{"testtinyint"},1);
assertEqual($fields{"testsmallint"},1);
assertEqual($fields{"testmediumint"},1);
assertEqual($fields{"testint"},1);
assertEqual($fields{"testbigint"},1);
#assertEqual($fields{"testfloat"},1.1);
assertEqual($fields{"testreal"},1.1);
assertEqual($fields{"testdecimal"},1.1);
assertEqualString($fields{"testdate"},"2001-01-01");
assertEqualString($fields{"testtime"},"01:00:00");
assertEqualString($fields{"testdatetime"},"2001-01-01 01:00:00");
assertEqual($fields{"testyear"},2001);
assertEqualString($fields{"testchar"},"char1");
assertEqualString($fields{"testtext"},"text1");
assertEqualString($fields{"testvarchar"},"varchar1");
assertEqualString($fields{"testtinytext"},"tinytext1");
assertEqualString($fields{"testmediumtext"},"mediumtext1");
assertEqualString($fields{"testlongtext"},"longtext1");
print("\n");
%fields=$cur->getRowHash(7);
assertEqual($fields{"testtinyint"},8);
assertEqual($fields{"testsmallint"},8);
assertEqual($fields{"testmediumint"},8);
assertEqual($fields{"testint"},8);
assertEqual($fields{"testbigint"},8);
#assertEqual($fields{"testfloat"},8.1);
assertEqual($fields{"testreal"},8.1);
assertEqual($fields{"testdecimal"},8.1);
assertEqualString($fields{"testdate"},"2008-01-01");
assertEqualString($fields{"testtime"},"08:00:00");
assertEqualString($fields{"testdatetime"},"2008-01-01 08:00:00");
assertEqual($fields{"testyear"},2008);
assertEqualString($fields{"testchar"},"char8");
assertEqualString($fields{"testtext"},"text8");
assertEqualString($fields{"testvarchar"},"varchar8");
assertEqualString($fields{"testtinytext"},"tinytext8");
assertEqualString($fields{"testmediumtext"},"mediumtext8");
assertEqualString($fields{"testlongtext"},"longtext8");
print("\n");


# field lengths by hash
print("FIELD LENGTHS BY HASH: \n");
%fieldlengths=$cur->getRowLengthsHash(0);
assertEqual($fieldlengths{"testtinyint"},1);
assertEqual($fieldlengths{"testsmallint"},1);
assertEqual($fieldlengths{"testmediumint"},1);
assertEqual($fieldlengths{"testint"},1);
assertEqual($fieldlengths{"testbigint"},1);
#assertEqual($fieldlengths{"testfloat"},3);
assertEqual($fieldlengths{"testreal"},3);
assertEqual($fieldlengths{"testdecimal"},3);
assertEqual($fieldlengths{"testdate"},10);
assertEqual($fieldlengths{"testtime"},8);
assertEqual($fieldlengths{"testdatetime"},19);
assertEqual($fieldlengths{"testyear"},4);
assertEqual($fieldlengths{"testchar"},5);
assertEqual($fieldlengths{"testtext"},5);
assertEqual($fieldlengths{"testvarchar"},8);
assertEqual($fieldlengths{"testtinytext"},9);
assertEqual($fieldlengths{"testmediumtext"},11);
assertEqual($fieldlengths{"testlongtext"},9);
print("\n");
%fieldlengths=$cur->getRowLengthsHash(7);
assertEqual($fieldlengths{"testtinyint"},1);
assertEqual($fieldlengths{"testsmallint"},1);
assertEqual($fieldlengths{"testmediumint"},1);
assertEqual($fieldlengths{"testint"},1);
assertEqual($fieldlengths{"testbigint"},1);
#assertEqual($fieldlengths{"testfloat"},3);
assertEqual($fieldlengths{"testreal"},3);
assertEqual($fieldlengths{"testdecimal"},3);
assertEqual($fieldlengths{"testdate"},10);
assertEqual($fieldlengths{"testtime"},8);
assertEqual($fieldlengths{"testdatetime"},19);
assertEqual($fieldlengths{"testyear"},4);
assertEqual($fieldlengths{"testchar"},5);
assertEqual($fieldlengths{"testtext"},5);
assertEqual($fieldlengths{"testvarchar"},8);
assertEqual($fieldlengths{"testtinytext"},9);
assertEqual($fieldlengths{"testmediumtext"},11);
assertEqual($fieldlengths{"testlongtext"},9);
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
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
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
$cur->setResultSetBufferSize(0);
print("\n");


# dont get column info
print("DONT GET COLUMN INFO: \n");
$cur->dontGetColumnInfo();
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
assertUndef($cur->getColumnName(0));
assertEqual($cur->getColumnLength(0),0);
assertUndef($cur->getColumnType(0));
print("\n");
$cur->getColumnInfo();
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
assertEqualString($cur->getColumnName(0),"testtinyint");
assertEqual($cur->getColumnLength(0),1);
assertEqualString($cur->getColumnType(0),"TINYINT");
print("\n");


# suspended session
print("SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
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
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
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
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
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
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
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
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
$filename=$cur->getCacheFileName();
assertEqualString($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEqualString($cur->getField(7,0),"8");
print("\n");


# column count for cached result set
print("COLUMN COUNT FOR CACHED RESULT SET: \n");
assertEqual($cur->colCount(),19);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEqualString($cur->getColumnName(0),"testtinyint");
assertEqualString($cur->getColumnName(1),"testsmallint");
assertEqualString($cur->getColumnName(2),"testmediumint");
assertEqualString($cur->getColumnName(3),"testint");
assertEqualString($cur->getColumnName(4),"testbigint");
assertEqualString($cur->getColumnName(5),"testfloat");
assertEqualString($cur->getColumnName(6),"testreal");
assertEqualString($cur->getColumnName(7),"testdecimal");
assertEqualString($cur->getColumnName(8),"testdate");
assertEqualString($cur->getColumnName(9),"testtime");
assertEqualString($cur->getColumnName(10),"testdatetime");
assertEqualString($cur->getColumnName(11),"testyear");
assertEqualString($cur->getColumnName(12),"testchar");
assertEqualString($cur->getColumnName(13),"testtext");
assertEqualString($cur->getColumnName(14),"testvarchar");
assertEqualString($cur->getColumnName(15),"testtinytext");
assertEqualString($cur->getColumnName(16),"testmediumtext");
assertEqualString($cur->getColumnName(17),"testlongtext");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"testtinyint");
assertEqualString($cols[1],"testsmallint");
assertEqualString($cols[2],"testmediumint");
assertEqualString($cols[3],"testint");
assertEqualString($cols[4],"testbigint");
assertEqualString($cols[5],"testfloat");
assertEqualString($cols[6],"testreal");
assertEqualString($cols[7],"testdecimal");
assertEqualString($cols[8],"testdate");
assertEqualString($cols[9],"testtime");
assertEqualString($cols[10],"testdatetime");
assertEqualString($cols[11],"testyear");
assertEqualString($cols[12],"testchar");
assertEqualString($cols[13],"testtext");
assertEqualString($cols[14],"testvarchar");
assertEqualString($cols[15],"testtinytext");
assertEqualString($cols[16],"testmediumtext");
assertEqualString($cols[17],"testlongtext");
print("\n");


# cached result set with result set buffer size
print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
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
print("FROM ONE CACHE FILE TO ANOTHER ".
	"WITH RESULT SET BUFFER SIZE: \n");
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
print("CACHED RESULT SET WITH SUSPEND ".
	"AND RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testtinyint"));
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


# commit and rollback
print("COMMIT AND ROLLBACK: \n");
# Note: Mysql's default isolation level is repeatable-read,
# not read-committed like most other db's.  Both sessions must
# commit to see the changes that each other has made.
$secondcon=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$secondcur=SQLRelay::Cursor->new($secondcon);
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
if ($majorversion>3) {
	assertEqualString($secondcur->getField(0,0),"0");
} else {
	assertEqualString($secondcur->getField(0,0),"8");
}
assertTrue($con->commit());
assertTrue($secondcon->commit());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"8");
assertTrue($con->autoCommitOn());
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	10, ".
	"	10, ".
	"	10, ".
	"	10, ".
	"	10, ".
	"	10.1, ".
	"	10.1, ".
	"	1.1, ".
	"	'2010-01-01', ".
	"	'10:00:00', ".
	"	'2010-01-01 10:00:00', ".
	"	'2010', ".
	"	'char10', ".
	"	'text10', ".
	"	'varchar10', ".
	"	'tinytext10', ".
	"	'mediumtext10', ".
	"	'longtext10', ".
	"	NULL)"));
assertTrue($secondcon->commit());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"9");
assertTrue($con->autoCommitOff());
$secondcon->commit();
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
$cur->sendQuery("drop table testtable");


# invalid queries
print("INVALID QUERIES: \n");
assertFalse($cur->sendQuery("select * from testtable order by testtinyint"));
assertFalse($cur->sendQuery("select * from testtable order by testtinyint"));
assertFalse($cur->sendQuery("select * from testtable order by testtinyint"));
assertFalse($cur->sendQuery("select * from testtable order by testtinyint"));
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
