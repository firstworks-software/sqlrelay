#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.


use SQLRelay::Connection;
use SQLRelay::Cursor;
use Sys::Hostname;
require "./asserts.pl";


@isolationlevels=("REPEATABLE-READ","READ-UNCOMMITTED",
			"READ-COMMITTED","SERIALIZABLE");
@subvars=("var1","var2","var3");
@subvallongs=(1,2,3);
@subvalstrings=("hi","hello","bye");
@subvaldoubles=(10.55,10.556,10.5556);
@precs=(4,5,6);
@scales=(2,3,4);

$LARGE_BUFFER_LENGTH=8192;


# hostname
$hostname=hostname();
$hostname=~s/\..*//;


# instantiation
$con=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$cur=SQLRelay::Cursor->new($con);


# identify
print("IDENTIFY: \n");
assertEquals($con->identify(),"mysql");
print("\n");


# db version
print("DB VERSION: \n");
$dbversion=$con->dbVersion();
$majorversion=int(substr($dbversion,0,1));
print("\n");


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");


# transaction state
print("TRANSACTION STATE: \n");
assertEquals($con->getDefaultTransactionModel(),"explicit-deferred");
assertEquals($con->getTransactionModel(),"explicit-deferred");
assertFalse($con->getInTransaction());
assertTrue($con->getAutoCommit());
print("\n");


# bind format
print("BIND FORMAT: \n");
if ($majorversion>3) {
	assertEquals($con->bindFormat(),"?");
} else {
	assertEquals($con->bindFormat(),":*");
}
print("\n");


# nextval format
print("NEXTVAL FORMAT: \n");
assertEquals($con->nextvalFormat(),"");
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
	"	testvarchar varchar(40), ".
	"	testtext text, ".
	"	testtinytext tinytext, ".
	"	testmediumtext mediumtext, ".
	"	testlongtext longtext, ".
	"	testblob blob, ".
	"	testtinyblob tinyblob, ".
	"	testmediumblob mediumblob, ".
	"	testlongblob longblob, ".
	"	testtimestamp timestamp)"));
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
	"	1, ".
	"	1, ".
	"	1.5, ".
	"	1.5, ".
	"	1.5, ".
	"	'2001-01-01', ".
	"	'01:00:00', ".
	"	'2001-01-01 01:00:00', ".
	"	'2001', ".
	"	'char1', ".
	"	'varchar1', ".
	"	'text1', ".
	"	'tinytext1', ".
	"	'mediumtext1', ".
	"	'longtext1', ".
	"	'blob1', ".
	"	'tinyblob1', ".
	"	'mediumblob1', ".
	"	'longblob1', ".
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
	"	2.5, ".
	"	2.5, ".
	"	2.5, ".
	"	'2002-01-01', ".
	"	'02:00:00', ".
	"	'2002-01-01 02:00:00', ".
	"	'2002', ".
	"	'char2', ".
	"	'varchar2', ".
	"	'text2', ".
	"	'tinytext2', ".
	"	'mediumtext2', ".
	"	'longtext2', ".
	"	'blob2', ".
	"	'tinyblob2', ".
	"	'mediumblob2', ".
	"	'longblob2', ".
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
	"	3.5, ".
	"	3.5, ".
	"	3.5, ".
	"	'2003-01-01', ".
	"	'03:00:00', ".
	"	'2003-01-01 03:00:00', ".
	"	'2003', ".
	"	'char3', ".
	"	'varchar3', ".
	"	'text3', ".
	"	'tinytext3', ".
	"	'mediumtext3', ".
	"	'longtext3', ".
	"	'blob3', ".
	"	'tinyblob3', ".
	"	'mediumblob3', ".
	"	'longblob3', ".
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
	"	4.5, ".
	"	4.5, ".
	"	4.5, ".
	"	'2004-01-01', ".
	"	'04:00:00', ".
	"	'2004-01-01 04:00:00', ".
	"	'2004', ".
	"	'char4', ".
	"	'varchar4', ".
	"	'text4', ".
	"	'tinytext4', ".
	"	'mediumtext4', ".
	"	'longtext4', ".
	"	'blob4', ".
	"	'tinyblob4', ".
	"	'mediumblob4', ".
	"	'longblob4', ".
	"	NULL)"));
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
assertEquals($cur->countBindVariables(),22);
$cur->inputBind("1",5);
$cur->inputBind("2",5);
$cur->inputBind("3",5);
$cur->inputBind("4",5);
$cur->inputBind("5",5);
$cur->inputBind("6",5.5,2,1);
$cur->inputBind("7",5.5,2,1);
$cur->inputBind("8",5.5,2,1);
$cur->inputBind("9","2005-01-01");
$cur->inputBind("10","05:00:00");
$cur->inputBindDate("11",2005,1,1,5,0,0,0,undef,0);
$cur->inputBind("12","2005");
$cur->inputBind("13","char5");
$cur->inputBind("14","varchar5");
$cur->inputBindClob("15","text5",5);
$cur->inputBindClob("16","tinytext5",9);
$cur->inputBindClob("17","mediumtext5",11);
$cur->inputBindClob("18","longtext5",9);
$cur->inputBindBlob("19","blob5",5);
$cur->inputBindBlob("20","tinyblob5",9);
$cur->inputBindBlob("21","mediumblob5",11);
$cur->inputBindBlob("22","longblob5",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",6);
$cur->inputBind("2",6);
$cur->inputBind("3",6);
$cur->inputBind("4",6);
$cur->inputBind("5",6);
$cur->inputBind("6",6.5,2,1);
$cur->inputBind("7",6.5,2,1);
$cur->inputBind("8",6.5,2,1);
$cur->inputBind("9","2006-01-01");
$cur->inputBind("10","06:00:00");
$cur->inputBindDate("11",2006,1,1,6,0,0,0,undef,0);
$cur->inputBind("12","2006");
$cur->inputBind("13","char6");
$cur->inputBind("14","varchar6");
$cur->inputBindClob("15","text6",5);
$cur->inputBindClob("16","tinytext6",9);
$cur->inputBindClob("17","mediumtext6",11);
$cur->inputBindClob("18","longtext6",9);
$cur->inputBindBlob("19","blob6",5);
$cur->inputBindBlob("20","tinyblob6",9);
$cur->inputBindBlob("21","mediumblob6",11);
$cur->inputBindBlob("22","longblob6",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",7);
$cur->inputBind("2",7);
$cur->inputBind("3",7);
$cur->inputBind("4",7);
$cur->inputBind("5",7);
$cur->inputBind("6",7.5,2,1);
$cur->inputBind("7",7.5,2,1);
$cur->inputBind("8",7.5,2,1);
$cur->inputBind("9","2007-01-01");
$cur->inputBind("10","07:00:00");
$cur->inputBindDate("11",2007,1,1,7,0,0,0,undef,0);
$cur->inputBind("12","2007");
$cur->inputBind("13","char7");
$cur->inputBind("14","varchar7");
$cur->inputBindClob("15","text7",5);
$cur->inputBindClob("16","tinytext7",9);
$cur->inputBindClob("17","mediumtext7",11);
$cur->inputBindClob("18","longtext7",9);
$cur->inputBindBlob("19","blob7",5);
$cur->inputBindBlob("20","tinyblob7",9);
$cur->inputBindBlob("21","mediumblob7",11);
$cur->inputBindBlob("22","longblob7",9);
assertTrue($cur->executeQuery());
print("\n");


# array of input binds by position
# mysql doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by position with validation
print("BIND BY POSITION WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("1",8);
$cur->inputBind("2",8);
$cur->inputBind("3",8);
$cur->inputBind("4",8);
$cur->inputBind("5",8);
$cur->inputBind("6",8.5,2,1);
$cur->inputBind("7",8.5,2,1);
$cur->inputBind("8",8.5,2,1);
$cur->inputBind("9","2008-01-01");
$cur->inputBind("10","08:00:00");
$cur->inputBindDate("11",2008,1,1,8,0,0,0,undef,0);
$cur->inputBind("12","2008");
$cur->inputBind("13","char8");
$cur->inputBind("14","varchar8");
$cur->inputBindClob("15","text8",5);
$cur->inputBindClob("16","tinytext8",9);
$cur->inputBindClob("17","mediumtext8",11);
$cur->inputBindClob("18","longtext8",9);
$cur->inputBindBlob("19","blob8",5);
$cur->inputBindBlob("20","tinyblob8",9);
$cur->inputBindBlob("21","mediumblob8",11);
$cur->inputBindBlob("22","longblob8",9);
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# input bind by name
# mysql doesn't support bind by name


# array of input binds by name
# mysql doesn't support bind by name


# input bind by name with validation
# mysql doesn't support bind by name


# select
print("SELECT: \n");
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEquals($cur->colCount(),23);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEquals($cur->getColumnName(0),"testtinyint");
assertEquals($cur->getColumnName(1),"testsmallint");
assertEquals($cur->getColumnName(2),"testmediumint");
assertEquals($cur->getColumnName(3),"testint");
assertEquals($cur->getColumnName(4),"testbigint");
assertEquals($cur->getColumnName(5),"testfloat");
assertEquals($cur->getColumnName(6),"testreal");
assertEquals($cur->getColumnName(7),"testdecimal");
assertEquals($cur->getColumnName(8),"testdate");
assertEquals($cur->getColumnName(9),"testtime");
assertEquals($cur->getColumnName(10),"testdatetime");
assertEquals($cur->getColumnName(11),"testyear");
assertEquals($cur->getColumnName(12),"testchar");
assertEquals($cur->getColumnName(13),"testvarchar");
assertEquals($cur->getColumnName(14),"testtext");
assertEquals($cur->getColumnName(15),"testtinytext");
assertEquals($cur->getColumnName(16),"testmediumtext");
assertEquals($cur->getColumnName(17),"testlongtext");
assertEquals($cur->getColumnName(18),"testblob");
assertEquals($cur->getColumnName(19),"testtinyblob");
assertEquals($cur->getColumnName(20),"testmediumblob");
assertEquals($cur->getColumnName(21),"testlongblob");
assertEquals($cur->getColumnName(22),"testtimestamp");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"testtinyint");
assertEquals($cols[1],"testsmallint");
assertEquals($cols[2],"testmediumint");
assertEquals($cols[3],"testint");
assertEquals($cols[4],"testbigint");
assertEquals($cols[5],"testfloat");
assertEquals($cols[6],"testreal");
assertEquals($cols[7],"testdecimal");
assertEquals($cols[8],"testdate");
assertEquals($cols[9],"testtime");
assertEquals($cols[10],"testdatetime");
assertEquals($cols[11],"testyear");
assertEquals($cols[12],"testchar");
assertEquals($cols[13],"testvarchar");
assertEquals($cols[14],"testtext");
assertEquals($cols[15],"testtinytext");
assertEquals($cols[16],"testmediumtext");
assertEquals($cols[17],"testlongtext");
assertEquals($cols[18],"testblob");
assertEquals($cols[19],"testtinyblob");
assertEquals($cols[20],"testmediumblob");
assertEquals($cols[21],"testlongblob");
assertEquals($cols[22],"testtimestamp");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEquals($cur->getColumnType(0),"TINYINT");
assertEquals($cur->getColumnType(1),"SMALLINT");
assertEquals($cur->getColumnType(2),"MEDIUMINT");
assertEquals($cur->getColumnType(3),"INT");
assertEquals($cur->getColumnType(4),"BIGINT");
assertEquals($cur->getColumnType(5),"FLOAT");
assertEquals($cur->getColumnType(6),"REAL");
assertEquals($cur->getColumnType(7),"DECIMAL");
assertEquals($cur->getColumnType(8),"DATE");
assertEquals($cur->getColumnType(9),"TIME");
assertEquals($cur->getColumnType(10),"DATETIME");
assertEquals($cur->getColumnType(11),"YEAR");
if ($majorversion==3) {
	assertEquals($cur->getColumnType(12),"VARSTRING");
} else {
	assertEquals($cur->getColumnType(12),"STRING");
}
assertEquals($cur->getColumnType(13),"VARSTRING");
assertEquals($cur->getColumnType(14),"BLOB");
assertEquals($cur->getColumnType(15),"TINYBLOB");
assertEquals($cur->getColumnType(16),"MEDIUMBLOB");
assertEquals($cur->getColumnType(17),"LONGBLOB");
assertEquals($cur->getColumnType(18),"BLOB");
assertEquals($cur->getColumnType(19),"TINYBLOB");
assertEquals($cur->getColumnType(20),"MEDIUMBLOB");
assertEquals($cur->getColumnType(21),"LONGBLOB");
assertEquals($cur->getColumnType(22),"TIMESTAMP");
assertEquals($cur->getColumnType("testtinyint"),"TINYINT");
assertEquals($cur->getColumnType("testsmallint"),"SMALLINT");
assertEquals($cur->getColumnType("testmediumint"),"MEDIUMINT");
assertEquals($cur->getColumnType("testint"),"INT");
assertEquals($cur->getColumnType("testbigint"),"BIGINT");
assertEquals($cur->getColumnType("testfloat"),"FLOAT");
assertEquals($cur->getColumnType("testreal"),"REAL");
assertEquals($cur->getColumnType("testdecimal"),"DECIMAL");
assertEquals($cur->getColumnType("testdate"),"DATE");
assertEquals($cur->getColumnType("testtime"),"TIME");
assertEquals($cur->getColumnType("testdatetime"),"DATETIME");
assertEquals($cur->getColumnType("testyear"),"YEAR");
if ($majorversion==3) {
	assertEquals($cur->getColumnType("testchar"),"VARSTRING");
} else {
	assertEquals($cur->getColumnType("testchar"),"STRING");
}
assertEquals($cur->getColumnType("testvarchar"),"VARSTRING");
assertEquals($cur->getColumnType("testtext"),"BLOB");
assertEquals($cur->getColumnType("testtinytext"),"TINYBLOB");
assertEquals($cur->getColumnType("testmediumtext"),"MEDIUMBLOB");
assertEquals($cur->getColumnType("testlongtext"),"LONGBLOB");
assertEquals($cur->getColumnType("testblob"),"BLOB");
assertEquals($cur->getColumnType("testtinyblob"),"TINYBLOB");
assertEquals($cur->getColumnType("testmediumblob"),"MEDIUMBLOB");
assertEquals($cur->getColumnType("testlongblob"),"LONGBLOB");
assertEquals($cur->getColumnType("testtimestamp"),"TIMESTAMP");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEquals($cur->getColumnLength(0),1);
assertEquals($cur->getColumnLength(1),2);
assertEquals($cur->getColumnLength(2),3);
assertEquals($cur->getColumnLength(3),4);
assertEquals($cur->getColumnLength(4),8);
assertEquals($cur->getColumnLength(5),4);
assertEquals($cur->getColumnLength(6),8);
assertEquals($cur->getColumnLength(7),6);
assertEquals($cur->getColumnLength(8),3);
assertEquals($cur->getColumnLength(9),3);
assertEquals($cur->getColumnLength(10),8);
assertEquals($cur->getColumnLength(11),1);
# testchar/testvarchar are char(40)/varchar(40); the connection charset
# is utf8mb4 (4 bytes/char) so the lengths are 160/161
assertEquals($cur->getColumnLength(12),160);
assertEquals($cur->getColumnLength(13),161);
assertEquals($cur->getColumnLength(14),65535);
assertEquals($cur->getColumnLength(15),255);
assertEquals($cur->getColumnLength(16),16777215);
assertEquals($cur->getColumnLength(17),2147483647);
assertEquals($cur->getColumnLength(18),65535);
assertEquals($cur->getColumnLength(19),255);
assertEquals($cur->getColumnLength(20),16777215);
assertEquals($cur->getColumnLength(21),2147483647);
assertEquals($cur->getColumnLength(22),4);
assertEquals($cur->getColumnLength("testtinyint"),1);
assertEquals($cur->getColumnLength("testsmallint"),2);
assertEquals($cur->getColumnLength("testmediumint"),3);
assertEquals($cur->getColumnLength("testint"),4);
assertEquals($cur->getColumnLength("testbigint"),8);
assertEquals($cur->getColumnLength("testfloat"),4);
assertEquals($cur->getColumnLength("testreal"),8);
assertEquals($cur->getColumnLength("testdecimal"),6);
assertEquals($cur->getColumnLength("testdate"),3);
assertEquals($cur->getColumnLength("testtime"),3);
assertEquals($cur->getColumnLength("testdatetime"),8);
assertEquals($cur->getColumnLength("testyear"),1);
# testchar/testvarchar are char(40)/varchar(40); the connection charset
# is utf8mb4 (4 bytes/char) so the lengths are 160/161
assertEquals($cur->getColumnLength("testchar"),160);
assertEquals($cur->getColumnLength("testvarchar"),161);
assertEquals($cur->getColumnLength("testtext"),65535);
assertEquals($cur->getColumnLength("testtinytext"),255);
assertEquals($cur->getColumnLength("testmediumtext"),16777215);
assertEquals($cur->getColumnLength("testlongtext"),2147483647);
assertEquals($cur->getColumnLength("testblob"),65535);
assertEquals($cur->getColumnLength("testtinyblob"),255);
assertEquals($cur->getColumnLength("testmediumblob"),16777215);
assertEquals($cur->getColumnLength("testlongblob"),2147483647);
assertEquals($cur->getColumnLength("testtimestamp"),4);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEquals($cur->getLongest(0),1);
assertEquals($cur->getLongest(1),1);
assertEquals($cur->getLongest(2),1);
assertEquals($cur->getLongest(3),1);
assertEquals($cur->getLongest(4),1);
assertEquals($cur->getLongest(5),3);
assertEquals($cur->getLongest(6),3);
assertEquals($cur->getLongest(7),3);
assertEquals($cur->getLongest(8),10);
assertEquals($cur->getLongest(9),8);
assertEquals($cur->getLongest(10),19);
assertEquals($cur->getLongest(11),4);
assertEquals($cur->getLongest(12),5);
assertEquals($cur->getLongest(13),8);
assertEquals($cur->getLongest(14),5);
assertEquals($cur->getLongest(15),9);
assertEquals($cur->getLongest(16),11);
assertEquals($cur->getLongest(17),9);
assertEquals($cur->getLongest(18),5);
assertEquals($cur->getLongest(19),9);
assertEquals($cur->getLongest(20),11);
assertEquals($cur->getLongest(21),9);
if ($majorversion==3) {
	assertEquals($cur->getLongest(22),14);
} else {
	assertEquals($cur->getLongest(22),19);
}
assertEquals($cur->getLongest("testtinyint"),1);
assertEquals($cur->getLongest("testsmallint"),1);
assertEquals($cur->getLongest("testmediumint"),1);
assertEquals($cur->getLongest("testint"),1);
assertEquals($cur->getLongest("testbigint"),1);
assertEquals($cur->getLongest("testfloat"),3);
assertEquals($cur->getLongest("testreal"),3);
assertEquals($cur->getLongest("testdecimal"),3);
assertEquals($cur->getLongest("testdate"),10);
assertEquals($cur->getLongest("testtime"),8);
assertEquals($cur->getLongest("testdatetime"),19);
assertEquals($cur->getLongest("testyear"),4);
assertEquals($cur->getLongest("testchar"),5);
assertEquals($cur->getLongest("testvarchar"),8);
assertEquals($cur->getLongest("testtext"),5);
assertEquals($cur->getLongest("testtinytext"),9);
assertEquals($cur->getLongest("testmediumtext"),11);
assertEquals($cur->getLongest("testlongtext"),9);
assertEquals($cur->getLongest("testblob"),5);
assertEquals($cur->getLongest("testtinyblob"),9);
assertEquals($cur->getLongest("testmediumblob"),11);
assertEquals($cur->getLongest("testlongblob"),9);
if ($majorversion==3) {
	assertEquals($cur->getLongest("testtimestamp"),14);
} else {
	assertEquals($cur->getLongest("testtimestamp"),19);
}
print("\n");


# row count
print("ROW COUNT: \n");
assertEquals($cur->rowCount(),8);
print("\n");


# total rows
print("TOTAL ROWS: \n");
# older versions of mysql know this
#assertEquals($cur->totalRows(),0);
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
assertEquals($cur->getField(0,3),"1");
assertEquals($cur->getField(0,4),"1");
assertEquals($cur->getField(0,5),"1.5");
assertEquals($cur->getField(0,6),"1.5");
assertEquals($cur->getField(0,7),"1.5");
assertEquals($cur->getField(0,8),"2001-01-01");
assertEquals($cur->getField(0,9),"01:00:00");
assertEquals($cur->getField(0,10),"2001-01-01 01:00:00");
assertEquals($cur->getField(0,11),"2001");
assertEquals($cur->getField(0,12),"char1");
assertEquals($cur->getField(0,13),"varchar1");
assertEquals($cur->getField(0,14),"text1");
assertEquals($cur->getField(0,15),"tinytext1");
assertEquals($cur->getField(0,16),"mediumtext1");
assertEquals($cur->getField(0,17),"longtext1");
assertEquals($cur->getField(0,18),"blob1");
assertEquals($cur->getField(0,19),"tinyblob1");
assertEquals($cur->getField(0,20),"mediumblob1");
assertEquals($cur->getField(0,21),"longblob1");
print("\n");
assertEquals($cur->getField(7,0),"8");
assertEquals($cur->getField(7,1),"8");
assertEquals($cur->getField(7,2),"8");
assertEquals($cur->getField(7,3),"8");
assertEquals($cur->getField(7,4),"8");
assertEquals($cur->getField(7,5),"8.5");
assertEquals($cur->getField(7,6),"8.5");
assertEquals($cur->getField(7,7),"8.5");
assertEquals($cur->getField(7,8),"2008-01-01");
assertEquals($cur->getField(7,9),"08:00:00");
assertEquals($cur->getField(7,10),"2008-01-01 08:00:00");
assertEquals($cur->getField(7,11),"2008");
assertEquals($cur->getField(7,12),"char8");
assertEquals($cur->getField(7,13),"varchar8");
assertEquals($cur->getField(7,14),"text8");
assertEquals($cur->getField(7,15),"tinytext8");
assertEquals($cur->getField(7,16),"mediumtext8");
assertEquals($cur->getField(7,17),"longtext8");
assertEquals($cur->getField(7,18),"blob8");
assertEquals($cur->getField(7,19),"tinyblob8");
assertEquals($cur->getField(7,20),"mediumblob8");
assertEquals($cur->getField(7,21),"longblob8");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEquals($cur->getFieldLength(0,0),1);
assertEquals($cur->getFieldLength(0,1),1);
assertEquals($cur->getFieldLength(0,2),1);
assertEquals($cur->getFieldLength(0,3),1);
assertEquals($cur->getFieldLength(0,4),1);
assertEquals($cur->getFieldLength(0,5),3);
assertEquals($cur->getFieldLength(0,6),3);
assertEquals($cur->getFieldLength(0,7),3);
assertEquals($cur->getFieldLength(0,8),10);
assertEquals($cur->getFieldLength(0,9),8);
assertEquals($cur->getFieldLength(0,10),19);
assertEquals($cur->getFieldLength(0,11),4);
assertEquals($cur->getFieldLength(0,12),5);
assertEquals($cur->getFieldLength(0,13),8);
assertEquals($cur->getFieldLength(0,14),5);
assertEquals($cur->getFieldLength(0,15),9);
assertEquals($cur->getFieldLength(0,16),11);
assertEquals($cur->getFieldLength(0,17),9);
assertEquals($cur->getFieldLength(0,18),5);
assertEquals($cur->getFieldLength(0,19),9);
assertEquals($cur->getFieldLength(0,20),11);
assertEquals($cur->getFieldLength(0,21),9);
print("\n");
assertEquals($cur->getFieldLength(7,0),1);
assertEquals($cur->getFieldLength(7,1),1);
assertEquals($cur->getFieldLength(7,2),1);
assertEquals($cur->getFieldLength(7,3),1);
assertEquals($cur->getFieldLength(7,4),1);
assertEquals($cur->getFieldLength(7,5),3);
assertEquals($cur->getFieldLength(7,6),3);
assertEquals($cur->getFieldLength(7,7),3);
assertEquals($cur->getFieldLength(7,8),10);
assertEquals($cur->getFieldLength(7,9),8);
assertEquals($cur->getFieldLength(7,10),19);
assertEquals($cur->getFieldLength(7,11),4);
assertEquals($cur->getFieldLength(7,12),5);
assertEquals($cur->getFieldLength(7,13),8);
assertEquals($cur->getFieldLength(7,14),5);
assertEquals($cur->getFieldLength(7,15),9);
assertEquals($cur->getFieldLength(7,16),11);
assertEquals($cur->getFieldLength(7,17),9);
assertEquals($cur->getFieldLength(7,18),5);
assertEquals($cur->getFieldLength(7,19),9);
assertEquals($cur->getFieldLength(7,20),11);
assertEquals($cur->getFieldLength(7,21),9);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEquals($cur->getField(0,"testtinyint"),"1");
assertEquals($cur->getField(0,"testsmallint"),"1");
assertEquals($cur->getField(0,"testmediumint"),"1");
assertEquals($cur->getField(0,"testint"),"1");
assertEquals($cur->getField(0,"testbigint"),"1");
assertEquals($cur->getField(0,"testfloat"),"1.5");
assertEquals($cur->getField(0,"testreal"),"1.5");
assertEquals($cur->getField(0,"testdecimal"),"1.5");
assertEquals($cur->getField(0,"testdate"),"2001-01-01");
assertEquals($cur->getField(0,"testtime"),"01:00:00");
assertEquals($cur->getField(0,"testdatetime"),"2001-01-01 01:00:00");
assertEquals($cur->getField(0,"testyear"),"2001");
assertEquals($cur->getField(0,"testchar"),"char1");
assertEquals($cur->getField(0,"testvarchar"),"varchar1");
assertEquals($cur->getField(0,"testtext"),"text1");
assertEquals($cur->getField(0,"testtinytext"),"tinytext1");
assertEquals($cur->getField(0,"testmediumtext"),"mediumtext1");
assertEquals($cur->getField(0,"testlongtext"),"longtext1");
assertEquals($cur->getField(0,"testblob"),"blob1");
assertEquals($cur->getField(0,"testlongblob"),"longblob1");
assertEquals($cur->getField(0,"testtinyblob"),"tinyblob1");
assertEquals($cur->getField(0,"testmediumblob"),"mediumblob1");
print("\n");
assertEquals($cur->getField(7,"testtinyint"),"8");
assertEquals($cur->getField(7,"testsmallint"),"8");
assertEquals($cur->getField(7,"testmediumint"),"8");
assertEquals($cur->getField(7,"testint"),"8");
assertEquals($cur->getField(7,"testbigint"),"8");
assertEquals($cur->getField(7,"testfloat"),"8.5");
assertEquals($cur->getField(7,"testreal"),"8.5");
assertEquals($cur->getField(7,"testdecimal"),"8.5");
assertEquals($cur->getField(7,"testdate"),"2008-01-01");
assertEquals($cur->getField(7,"testtime"),"08:00:00");
assertEquals($cur->getField(7,"testdatetime"),"2008-01-01 08:00:00");
assertEquals($cur->getField(7,"testyear"),"2008");
assertEquals($cur->getField(7,"testchar"),"char8");
assertEquals($cur->getField(7,"testvarchar"),"varchar8");
assertEquals($cur->getField(7,"testtext"),"text8");
assertEquals($cur->getField(7,"testtinytext"),"tinytext8");
assertEquals($cur->getField(7,"testmediumtext"),"mediumtext8");
assertEquals($cur->getField(7,"testlongtext"),"longtext8");
assertEquals($cur->getField(7,"testblob"),"blob8");
assertEquals($cur->getField(7,"testlongblob"),"longblob8");
assertEquals($cur->getField(7,"testtinyblob"),"tinyblob8");
assertEquals($cur->getField(7,"testmediumblob"),"mediumblob8");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEquals($cur->getFieldLength(0,"testtinyint"),1);
assertEquals($cur->getFieldLength(0,"testsmallint"),1);
assertEquals($cur->getFieldLength(0,"testmediumint"),1);
assertEquals($cur->getFieldLength(0,"testint"),1);
assertEquals($cur->getFieldLength(0,"testbigint"),1);
assertEquals($cur->getFieldLength(0,"testfloat"),3);
assertEquals($cur->getFieldLength(0,"testreal"),3);
assertEquals($cur->getFieldLength(0,"testdecimal"),3);
assertEquals($cur->getFieldLength(0,"testdate"),10);
assertEquals($cur->getFieldLength(0,"testtime"),8);
assertEquals($cur->getFieldLength(0,"testdatetime"),19);
assertEquals($cur->getFieldLength(0,"testyear"),4);
assertEquals($cur->getFieldLength(0,"testchar"),5);
assertEquals($cur->getFieldLength(0,"testvarchar"),8);
assertEquals($cur->getFieldLength(0,"testtext"),5);
assertEquals($cur->getFieldLength(0,"testtinytext"),9);
assertEquals($cur->getFieldLength(0,"testmediumtext"),11);
assertEquals($cur->getFieldLength(0,"testlongtext"),9);
assertEquals($cur->getFieldLength(0,"testblob"),5);
assertEquals($cur->getFieldLength(0,"testtinyblob"),9);
assertEquals($cur->getFieldLength(0,"testmediumblob"),11);
assertEquals($cur->getFieldLength(0,"testlongblob"),9);
print("\n");
assertEquals($cur->getFieldLength(7,"testtinyint"),1);
assertEquals($cur->getFieldLength(7,"testsmallint"),1);
assertEquals($cur->getFieldLength(7,"testmediumint"),1);
assertEquals($cur->getFieldLength(7,"testint"),1);
assertEquals($cur->getFieldLength(7,"testbigint"),1);
assertEquals($cur->getFieldLength(7,"testfloat"),3);
assertEquals($cur->getFieldLength(7,"testreal"),3);
assertEquals($cur->getFieldLength(7,"testdecimal"),3);
assertEquals($cur->getFieldLength(7,"testdate"),10);
assertEquals($cur->getFieldLength(7,"testtime"),8);
assertEquals($cur->getFieldLength(7,"testdatetime"),19);
assertEquals($cur->getFieldLength(7,"testyear"),4);
assertEquals($cur->getFieldLength(7,"testchar"),5);
assertEquals($cur->getFieldLength(7,"testvarchar"),8);
assertEquals($cur->getFieldLength(7,"testtext"),5);
assertEquals($cur->getFieldLength(7,"testtinytext"),9);
assertEquals($cur->getFieldLength(7,"testmediumtext"),11);
assertEquals($cur->getFieldLength(7,"testlongtext"),9);
assertEquals($cur->getFieldLength(7,"testblob"),5);
assertEquals($cur->getFieldLength(7,"testtinyblob"),9);
assertEquals($cur->getFieldLength(7,"testmediumblob"),11);
assertEquals($cur->getFieldLength(7,"testlongblob"),9);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEquals($fields[0],"1");
assertEquals($fields[1],"1");
assertEquals($fields[2],"1");
assertEquals($fields[3],"1");
assertEquals($fields[4],"1");
assertEquals($fields[5],"1.5");
assertEquals($fields[6],"1.5");
assertEquals($fields[7],"1.5");
assertEquals($fields[8],"2001-01-01");
assertEquals($fields[9],"01:00:00");
assertEquals($fields[10],"2001-01-01 01:00:00");
assertEquals($fields[11],"2001");
assertEquals($fields[12],"char1");
assertEquals($fields[13],"varchar1");
assertEquals($fields[14],"text1");
assertEquals($fields[15],"tinytext1");
assertEquals($fields[16],"mediumtext1");
assertEquals($fields[17],"longtext1");
assertEquals($fields[18],"blob1");
assertEquals($fields[19],"tinyblob1");
assertEquals($fields[20],"mediumblob1");
assertEquals($fields[21],"longblob1");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEquals($fieldlens[0],1);
assertEquals($fieldlens[1],1);
assertEquals($fieldlens[2],1);
assertEquals($fieldlens[3],1);
assertEquals($fieldlens[4],1);
assertEquals($fieldlens[5],3);
assertEquals($fieldlens[6],3);
assertEquals($fieldlens[7],3);
assertEquals($fieldlens[8],10);
assertEquals($fieldlens[9],8);
assertEquals($fieldlens[10],19);
assertEquals($fieldlens[11],4);
assertEquals($fieldlens[12],5);
assertEquals($fieldlens[13],8);
assertEquals($fieldlens[14],5);
assertEquals($fieldlens[15],9);
assertEquals($fieldlens[16],11);
assertEquals($fieldlens[17],9);
assertEquals($fieldlens[18],5);
assertEquals($fieldlens[19],9);
assertEquals($fieldlens[20],11);
assertEquals($fieldlens[21],9);
print("\n");


# result set buffer size
print("RESULT SET BUFFER SIZE: \n");
assertEquals($cur->getResultSetBufferSize(),0);
$cur->setResultSetBufferSize(2);
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
assertUndef($cur->getColumnName(0));
assertEquals($cur->getColumnLength(0),0);
assertUndef($cur->getColumnType(0));
print("\n");
$cur->getColumnInfo();
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
assertEquals($cur->getColumnName(0),"testtinyint");
assertEquals($cur->getColumnLength(0),1);
assertEquals($cur->getColumnType(0),"TINYINT");
print("\n");


# suspended session
print("SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
$filename=$cur->getCacheFileName();
assertEquals($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEquals($cur->getField(7,0),"8");
print("\n");


# column count for cached result set
print("COLUMN COUNT FOR CACHED RESULT SET: \n");
assertEquals($cur->colCount(),23);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEquals($cur->getColumnName(0),"testtinyint");
assertEquals($cur->getColumnName(1),"testsmallint");
assertEquals($cur->getColumnName(2),"testmediumint");
assertEquals($cur->getColumnName(3),"testint");
assertEquals($cur->getColumnName(4),"testbigint");
assertEquals($cur->getColumnName(5),"testfloat");
assertEquals($cur->getColumnName(6),"testreal");
assertEquals($cur->getColumnName(7),"testdecimal");
assertEquals($cur->getColumnName(8),"testdate");
assertEquals($cur->getColumnName(9),"testtime");
assertEquals($cur->getColumnName(10),"testdatetime");
assertEquals($cur->getColumnName(11),"testyear");
assertEquals($cur->getColumnName(12),"testchar");
assertEquals($cur->getColumnName(13),"testvarchar");
assertEquals($cur->getColumnName(14),"testtext");
assertEquals($cur->getColumnName(15),"testtinytext");
assertEquals($cur->getColumnName(16),"testmediumtext");
assertEquals($cur->getColumnName(17),"testlongtext");
assertEquals($cur->getColumnName(18),"testblob");
assertEquals($cur->getColumnName(19),"testtinyblob");
assertEquals($cur->getColumnName(20),"testmediumblob");
assertEquals($cur->getColumnName(21),"testlongblob");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"testtinyint");
assertEquals($cols[1],"testsmallint");
assertEquals($cols[2],"testmediumint");
assertEquals($cols[3],"testint");
assertEquals($cols[4],"testbigint");
assertEquals($cols[5],"testfloat");
assertEquals($cols[6],"testreal");
assertEquals($cols[7],"testdecimal");
assertEquals($cols[8],"testdate");
assertEquals($cols[9],"testtime");
assertEquals($cols[10],"testdatetime");
assertEquals($cols[11],"testyear");
assertEquals($cols[12],"testchar");
assertEquals($cols[13],"testvarchar");
assertEquals($cols[14],"testtext");
assertEquals($cols[15],"testtinytext");
assertEquals($cols[16],"testmediumtext");
assertEquals($cols[17],"testlongtext");
assertEquals($cols[18],"testblob");
assertEquals($cols[19],"testtinyblob");
assertEquals($cols[20],"testmediumblob");
assertEquals($cols[21],"testlongblob");
print("\n");


# cached result set with result set buffer size
print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
# can't do this with mysql
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
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# reset transaction state
print("RESET TRANSACTION STATE: \n");
assertTrue($con->commit());
assertEquals($con->getTransactionModel(),"explicit-deferred");
assertTrue($con->getAutoCommit());
print("\n");


# transaction behavior - implicit
print("TRANSACTION BEHAVIOR - implicit: \n");
assertTrue($con->setTransactionModel("implicit"));
assertEquals($con->getTransactionModel(),"implicit");
assertTrue($cur->sendQuery("create table testtable (col1 integer)"));
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
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# transaction behavior - explicit
print("TRANSACTION BEHAVIOR - explicit: \n");
assertTrue($con->setTransactionModel("explicit"));
assertEquals($con->getTransactionModel(),"explicit");
assertTrue($cur->sendQuery("create table testtable (col1 integer)"));
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
assertTrue($cur->sendQuery("create table testtable (col1 integer)"));
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
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# transaction behavior - explicit-error
print("TRANSACTION BEHAVIOR - explicit-error: \n");
assertTrue($con->setTransactionModel("explicit-error"));
assertEquals($con->getTransactionModel(),"explicit-error");
assertTrue($cur->sendQuery("create table testtable (col1 integer)"));
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
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# transaction behavior - none
print("TRANSACTION BEHAVIOR - none: \n");
assertTrue($con->setTransactionModel("none"));
assertEquals($con->getTransactionModel(),"none");
assertTrue($cur->sendQuery("create table testtable (col1 integer)"));
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
assertEquals($con->getTransactionModel(),"explicit-deferred");
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
	"	testclob1 longtext, ".
	"	testclob2 longtext, ".
	"	testblob1 longblob, ".
	"	testblob2 longblob)"));
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
assertEquals($cur->getField(0,0),"");
assertUndef($cur->getField(0,1));
assertEquals($cur->getField(0,2),"");
assertUndef($cur->getField(0,3));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# long lobs
print("LONG LOBS: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery(
	"create table testtable (".
	"	testtext longtext, ".
	"	testblob longblob)");
$cur->prepareQuery("insert into testtable values (?,?)");
$largebuffer=('C' x $LARGE_BUFFER_LENGTH);
$cur->inputBindClob("1",$largebuffer,$LARGE_BUFFER_LENGTH);
$cur->inputBindBlob("2",$largebuffer,$LARGE_BUFFER_LENGTH);
assertTrue($cur->executeQuery());
$cur->sendQuery("select * from testtable");
assertEquals($cur->getFieldLength(0,"testtext"),$LARGE_BUFFER_LENGTH);
assertEquals($cur->getField(0,"testtext"),$largebuffer);
assertEquals($cur->getFieldLength(0,"testblob"),$LARGE_BUFFER_LENGTH);
assertEquals($cur->getField(0,"testblob"),$largebuffer);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# output bind by position
# mysql doesn't support output binds

# output bind by name
# mysql doesn't support bind by name


# output bind by name with validation
# mysql doesn't support bind by name


# lob output bind
# mysql doesn't support output binds


# long output bind
# mysql doesn't support output binds


# negative input bind
print("NEGATIVE INPUT BIND: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery("create table testtable (testval int)");
$cur->prepareQuery("insert into testtable values (?)");
$cur->inputBind("1",-1);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testval from testtable");
assertEquals($cur->getField(0,"testval"),"-1");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# bind validation
# mysql doesn't support bind by name


# rebinding
print("REBINDING: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	in in1 int) ".
	"begin ".
	"	select in1; ".
	"end"));
$cur->prepareQuery("call testproc(?)");
$cur->inputBind("1",1);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
$cur->inputBind("1",2);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"2");
$cur->inputBind("1",3);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"3");
assertTrue($cur->sendQuery("drop procedure testproc"));
print("\n");


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
$cur->prepareQuery("select ?");
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
print("STORED PROCEDURE RETURNING NO VALUE: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	in in1 int, ".
	"	in in2 double, ".
	"	in in3 varchar(20)) ".
	"begin ".
	"end"));
$cur->prepareQuery("call testproc(?,?,?)");
$cur->inputBind("1",1);
$cur->inputBind("2",1.5,2,1);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("drop procedure testproc"));
print("\n");


# stored procedure returning single value
print("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	in in1 int, ".
	"	in in2 double, ".
	"	in in3 varchar(20)) ".
	"begin ".
	"	select in1; ".
	"end"));
$cur->prepareQuery("call testproc(?,?,?)");
$cur->inputBind("1",1);
$cur->inputBind("2",1.5,2,1);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertTrue($cur->sendQuery("drop procedure testproc"));
print("\n");


# stored procedure returning multiple values
print("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	in in1 int, ".
	"	in in2 double, ".
	"	in in3 varchar(20)) ".
	"begin ".
	"	select in1, in2, in3; ".
	"end"));
$cur->prepareQuery("call testproc(?,?,?)");
$cur->inputBind("1",1);
$cur->inputBind("2",1.5,2,1);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"1.5");
assertEquals($cur->getField(0,2),"hello");
assertTrue($cur->sendQuery("drop procedure testproc"));
print("\n");


# stored procedure returning result set
print("STORED PROCEDURE RETURNING RESULT SET: \n");
$cur->sendQuery("drop procedure testselectproc");
assertTrue($cur->sendQuery(
	"create procedure testselectproc() ".
	"begin ".
	"	select 1 ".
	"	union ".
	"	select 2 ".
	"	union ".
	"	select 3 ".
	"	union ".
	"	select 4 ".
	"	union ".
	"	select 5 ".
	"	union ".
	"	select 6 ".
	"	union ".
	"	select 7 ".
	"	union ".
	"	select 8; ".
	"end"));
assertTrue($cur->sendQuery("call testselectproc()"));
assertEquals($cur->rowCount(),8);
assertTrue($cur->sendQuery("drop procedure testselectproc"));
print("\n");


# temporary tables
print("TEMPORARY TABLES: \n");
$cur->sendQuery("drop table temptable");
$cur->sendQuery("create temporary table temptable (col1 int)");
assertTrue($cur->sendQuery("insert into temptable values (1)"));
assertTrue($cur->sendQuery("select count(*) from temptable"));
assertEquals($cur->getField(0,0),"1");
$con->endSession();
print("\n");
assertFalse($cur->sendQuery("select count(*) from temptable"));
print("\n");


if ($majorversion>3) {

	# stored procedure returning no value
	print("STORED PROCEDURE RETURNING NO VALUE: \n");
	$cur->sendQuery("drop procedure if exists testproc");
	assertTrue($cur->sendQuery(
		"create procedure testproc(".
		"	in in1 int, ".
		"	in in2 float, ".
		"	in in3 char(20)) ".
		"begin ".
		"	select in1, in2, in3; ".
		"end;"));
	$cur->prepareQuery("call testproc(?,?,?)");
	$cur->inputBind("1",1);
	$cur->inputBind("2",1.5,4,2);
	$cur->inputBind("3","hello");
	assertTrue($cur->executeQuery());
	assertEquals($cur->getField(0,0),"1");
	assertEquals($cur->getField(0,1),"1.5");
	assertEquals($cur->getField(0,2),"hello");
	$cur->sendQuery("drop procedure testproc");
	print("\n");


	# stored procedure returning one value
	print("FUNCTIONS: \n");
	$cur->sendQuery("drop function if exists testfunc");
	assertTrue($cur->sendQuery(
		"create function testfunc(in1 int, in2 ".
		"	int) returns int return in1+in2;"));
	$cur->prepareQuery("select testfunc(?,?)");
	$cur->inputBind("1",10);
	$cur->inputBind("2",20);
	assertTrue($cur->executeQuery());
	assertEquals($cur->getField(0,0),"30");
	$cur->sendQuery("drop function if exists testfunc");
	print("\n");


	# stored procedure returning multiple values
	print("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	$cur->sendQuery("drop procedure if exists testproc");
	assertTrue($cur->sendQuery(
		"create procedure testproc(".
		"	out out1 int, ".
		"	out out2 float, ".
		"	out out3 char(20)) ".
		"begin ".
		"	select 1, 2.5, 'hello' ".
		"		into out1, out2, out3; ".
		"end;"));
	assertTrue($cur->sendQuery("set \@out1=0, \@out2=0.0, \@out3=''"));
	assertTrue($cur->sendQuery("call testproc(\@out1,\@out2,\@out3)"));
	assertTrue($cur->sendQuery("select \@out1, \@out2, \@out3"));
	assertEquals($cur->getField(0,0),"1");
	assertEquals($cur->getFieldAsDouble(0,1),2.5);
	assertEquals($cur->getField(0,2),"hello");
	$cur->sendQuery("drop procedure testproc");
	print("\n");


	# stored procedure returning result set
	print("STORED PROCEDURE RETURNING RESULT SET: \n");
	$cur->sendQuery("drop procedure if exists testselectproc");
	assertTrue($cur->sendQuery(
		"create procedure testselectproc() ".
		"begin ".
		"	select 1 ".
		"	union ".
		"	select 2 ".
		"	union ".
		"	select 3 ".
		"	union ".
		"	select 4 ".
		"	union ".
		"	select 5 ".
		"	union ".
		"	select 6 ".
		"	union ".
		"	select 7 ".
		"	union ".
		"	select 8; ".
		"end"));
	assertTrue($cur->sendQuery("call testselectproc()"));
	assertEquals($cur->rowCount(),8);
	$cur->sendQuery("drop procedure testselectproc");
	print("\n");
}


if ($majorversion>3) {

	# encoded binary data - all chars - \-escaped
	print("ENCODED BINARY DATA - ".
				"all chars - \\-escaped: \n");
	$cur->sendQuery("drop table testtable");
	assertTrue($cur->sendQuery(
		"create table testtable (col1 longblob)"));
	$buffer=pack("C*",(0..255));
	$query="insert into testtable values (_binary'";
	for ($i=0; $i<length($buffer); $i++) {
		$byte=substr($buffer,$i,1);
		if ($byte eq "'") {
			$query.="\\";
		}
		if ($byte eq "\\") {
			$query.="\\";
		}
		$query.=$byte;
	}
	$query.="')";
	assertTrue($cur->sendQueryWithLength($query,length($query)));
	assertTrue($cur->sendQuery("select col1 from testtable"));
	assertEquals($cur->getFieldLength(0,0),256);
	assertEqualsBytes($cur->getField(0,0),$buffer,256);
	assertTrue($cur->sendQuery("drop table testtable"));
	print("\n");


	# encoded binary data - (null)"" - unescaped
	print("ENCODED BINARY DATA - ".
				"(null)\"\" - unescaped: \n");
	$cur->sendQuery("drop table testtable");
	assertTrue($cur->sendQuery(
		"create table testtable (col1 longblob)"));
	$query="insert into testtable values (_binary'\0\"\"')";
	assertTrue($cur->sendQueryWithLength($query,length($query)));
	assertTrue($cur->sendQuery("select col1 from testtable"));
	assertEquals($cur->getFieldLength(0,0),3);
	assertEqualsBytes($cur->getField(0,0),"\0\"\"",3);
	assertTrue($cur->sendQuery("drop table testtable"));
	print("\n");


	# encoded binary data - (null)"" - \-escaped
	print("ENCODED BINARY DATA - ".
				"\\(null)\\\"\\\" - \\-escaped: \n");
	$cur->sendQuery("drop table testtable");
	assertTrue($cur->sendQuery(
		"create table testtable (col1 longblob)"));
	$query="insert into testtable values (_binary'\\\0\\\"\\\"')";
	assertTrue($cur->sendQueryWithLength($query,length($query)));
	assertTrue($cur->sendQuery("select col1 from testtable"));
	assertEquals($cur->getFieldLength(0,0),3);
	assertEqualsBytes($cur->getField(0,0),"\0\"\"",3);
	assertTrue($cur->sendQuery("drop table testtable"));
	print("\n");
}


# quotes - '' - ''-escaped
print("QUOTES - '' - ''-escaped: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('''''')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertEquals($cur->getField(0,0),"''");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes - '' - '',\-escaped
print("QUOTES - '' - '',\\-escaped: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('''\\'')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertEquals($cur->getField(0,0),"''");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes - '' - \,''-escaped
print("QUOTES - '' - \\,''-escaped: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('\\'''')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertEquals($cur->getField(0,0),"''");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes - \\' - \-escaped
print("QUOTES - \\\\' - \\-escaped: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('\\\\\\'')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertEqualsBytes($cur->getField(0,0),"\\\'",2);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes - "" - unescaped
print("QUOTES - \"\" - unescaped: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('\"\"')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertEquals($cur->getField(0,0),"\"\"");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes - random - '',\-escaped
print("QUOTES - random - '',\\-escaped: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable ".
				"(col1 varchar(512))"));
@ch=("'","\"","\\","\0");
srand(1);
@choices=();
for ($i=0; $i<256; $i++) {
	push(@choices,int(rand(4)));
}
srand(2);
@escapes=();
for ($i=0; $i<256; $i++) {
	push(@escapes,int(rand(2)));
}
$buffer="";
for ($i=0; $i<256; $i++) {
	$buffer.=$ch[$choices[$i]];
}
$query="insert into testtable values ('";
for ($i=0; $i<256; $i++) {
	$byte=substr($buffer,$i,1);
	if ($byte eq "'") {
		# randomly escape with \ or ''
		if ($escapes[$i]) {
			$query.="'";
		} else {
			$query.="\\";
		}
	}
	if ($byte eq "\"") {
		# randomly escape with \ or don't escape
		if ($escapes[$i]) {
			$query.="\\";
		}
	}
	if ($byte eq "\\") {
		# escape with backslash
		$query.="\\";
	}
	$query.=$byte;
}
$query.="')";
assertTrue($cur->sendQueryWithLength($query,length($query)));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),256);
assertEqualsBytes($cur->getField(0,0),$buffer,256);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# last insert id
print("LAST INSERT ID: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
		"create table testtable ".
		"	(col1 int primary key auto_increment, ".
		"	col2 int)"));
assertTrue($cur->sendQuery("insert into testtable values (null,1)"));
assertEquals($con->getLastInsertId(),1);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# database is schema
print("DATABASE IS SCHEMA: \n");
assertTrue($con->getDatabaseIsSchema());
print("\n");


# catalog list
print("CATALOG LIST: \n");
assertTrue($cur->getCatalogList(undef));
assertEquals($cur->getColumnName(0),"Database");
assertInResultSet($cur,"Database","def");
print("\n");


# schema list
print("SCHEMA LIST: \n");
assertTrue($cur->getSchemaList(undef));
assertEquals($cur->getColumnName(0),"Database");
assertInResultSet($cur,"Database",$hostname);
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
assertEquals($cur->getField(0,"precision"),"255");
assertEquals($cur->getField(0,"local_type_name"),"CHAR");
assertTrue($cur->getTypeInfoList("varchar"));
assertEquals($cur->getField(0,"type_name"),"VARCHAR");
assertEquals($cur->getField(0,"data_type"),"12");
assertEquals($cur->getField(0,"precision"),"65535");
assertEquals($cur->getField(0,"local_type_name"),"VARCHAR");
assertTrue($cur->getTypeInfoList("date"));
assertEquals($cur->getField(0,"type_name"),"DATE");
assertEquals($cur->getField(0,"data_type"),"91");
assertEquals($cur->getField(0,"precision"),"10");
assertEquals($cur->getField(0,"local_type_name"),"DATE");
print("\n");


# column list
print("COLUMN LIST: \n");
$cur->sendQuery("drop table testtable");
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
	"	testvarchar varchar(40), ".
	"	testtext text, ".
	"	testtinytext tinytext, ".
	"	testmediumtext mediumtext, ".
	"	testlongtext longtext, ".
	"	testblob blob, ".
	"	testtinyblob tinyblob, ".
	"	testmediumblob mediumblob, ".
	"	testlongblob longblob, ".
	"	testtimestamp timestamp)"));
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
assertEquals($cur->getField(0,"column_name"),"testtinyint");
assertEquals($cur->getField(1,"column_name"),"testsmallint");
assertEquals($cur->getField(2,"column_name"),"testmediumint");
assertEquals($cur->getField(3,"column_name"),"testint");
assertEquals($cur->getField(4,"column_name"),"testbigint");
assertEquals($cur->getField(5,"column_name"),"testfloat");
assertEquals($cur->getField(6,"column_name"),"testreal");
assertEquals($cur->getField(7,"column_name"),"testdecimal");
assertEquals($cur->getField(8,"column_name"),"testdate");
assertEquals($cur->getField(9,"column_name"),"testtime");
assertEquals($cur->getField(10,"column_name"),"testdatetime");
assertEquals($cur->getField(11,"column_name"),"testyear");
assertEquals($cur->getField(12,"column_name"),"testchar");
assertEquals($cur->getField(13,"column_name"),"testvarchar");
assertEquals($cur->getField(14,"column_name"),"testtext");
assertEquals($cur->getField(15,"column_name"),"testtinytext");
assertEquals($cur->getField(16,"column_name"),"testmediumtext");
assertEquals($cur->getField(17,"column_name"),"testlongtext");
assertEquals($cur->getField(18,"column_name"),"testblob");
assertEquals($cur->getField(19,"column_name"),"testtinyblob");
assertEquals($cur->getField(20,"column_name"),"testmediumblob");
assertEquals($cur->getField(21,"column_name"),"testlongblob");
assertEquals($cur->getField(22,"column_name"),"testtimestamp");
assertEquals($cur->getField(0,"data_type"),"TINYINT");
assertEquals($cur->getField(1,"data_type"),"SMALLINT");
assertEquals($cur->getField(2,"data_type"),"MEDIUMINT");
assertEquals($cur->getField(3,"data_type"),"INT");
assertEquals($cur->getField(4,"data_type"),"BIGINT");
assertEquals($cur->getField(5,"data_type"),"FLOAT");
assertEquals($cur->getField(6,"data_type"),"DOUBLE"); # not "REAL"
assertEquals($cur->getField(7,"data_type"),"DECIMAL");
assertEquals($cur->getField(8,"data_type"),"DATE");
assertEquals($cur->getField(9,"data_type"),"TIME");
assertEquals($cur->getField(10,"data_type"),"DATETIME");
assertEquals($cur->getField(11,"data_type"),"YEAR");
assertEquals($cur->getField(12,"data_type"),"CHAR");
assertEquals($cur->getField(13,"data_type"),"VARCHAR");
assertEquals($cur->getField(14,"data_type"),"TEXT");
assertEquals($cur->getField(15,"data_type"),"TINYTEXT");
assertEquals($cur->getField(16,"data_type"),"MEDIUMTEXT");
assertEquals($cur->getField(17,"data_type"),"LONGTEXT");
assertEquals($cur->getField(18,"data_type"),"BLOB");
assertEquals($cur->getField(19,"data_type"),"TINYBLOB");
assertEquals($cur->getField(20,"data_type"),"MEDIUMBLOB");
assertEquals($cur->getField(21,"data_type"),"LONGBLOB");
assertEquals($cur->getField(22,"data_type"),"TIMESTAMP");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# column list - auto_increment, primary key
print("COLUMN LIST - auto_increment, primary key: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int auto_increment primary key, ".
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
assertEquals($cur->getField(0,"table"),"testtable");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertEquals($cur->getField(0,"column_name"),"col1");
assertEquals($cur->getField(0,"key_name"),"PRIMARY");
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
assertEquals($cur->getField(0,"table"),"testtable");
assertEquals($cur->getField(0,"non_unique"),"false");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertEquals($cur->getField(0,"column_name"),"col1");
assertEquals($cur->getField(0,"collation"),"A");
assertEquals($cur->getField(0,"index_type"),"3");
assertEquals($cur->getField(0,"key_name"),"PRIMARY");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# procedure list
print("PROCEDURE LIST: \n");
$cur->sendQuery("drop procedure testproc1");
$cur->sendQuery("drop procedure testproc2");
$cur->sendQuery("drop procedure testproc3");
$cur->sendQuery("drop procedure testproc4");
assertTrue($cur->sendQuery(
	"create procedure testproc1(".
	"	in in1 int, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"begin end"));
assertTrue($cur->sendQuery(
	"create procedure testproc2(".
	"	in in1 int, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"begin end"));
assertTrue($cur->sendQuery(
	"create procedure testproc3(".
	"	in in1 int, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"begin end"));
assertTrue($cur->sendQuery(
	"create procedure testproc4(".
	"	in in1 int, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"begin end"));
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
assertEquals($cur->getField(0,"parameter_name"),"in1");
assertEquals($cur->getField(0,"parameter_mode"),"1");
assertEquals($cur->getField(0,"data_type"),"INT");
assertEquals($cur->getField(0,"ordinal_position"),"1");
assertEquals($cur->getField(1,"parameter_name"),"in2");
assertEquals($cur->getField(1,"parameter_mode"),"1");
assertEquals($cur->getField(1,"data_type"),"CHAR");
assertEquals($cur->getField(1,"ordinal_position"),"2");
assertEquals($cur->getField(2,"parameter_name"),"in3");
assertEquals($cur->getField(2,"parameter_mode"),"1");
assertEquals($cur->getField(2,"data_type"),"VARCHAR");
assertEquals($cur->getField(2,"ordinal_position"),"3");
assertEquals($cur->getField(3,"parameter_name"),"in4");
assertEquals($cur->getField(3,"parameter_mode"),"1");
assertEquals($cur->getField(3,"data_type"),"DATE");
assertEquals($cur->getField(3,"ordinal_position"),"4");
assertTrue($cur->sendQuery("drop procedure testproc1"));
assertTrue($cur->sendQuery("drop procedure testproc2"));
assertTrue($cur->sendQuery("drop procedure testproc3"));
assertTrue($cur->sendQuery("drop procedure testproc4"));
print("\n");


# invalid queries
print("INVALID QUERIES: \n");
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testtinyint "));
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
