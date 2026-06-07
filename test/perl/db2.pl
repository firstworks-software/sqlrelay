#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.


use SQLRelay::Connection;
use SQLRelay::Cursor;
require "./asserts.pl";


@isolationlevels=("CS","UR","RS","RR");
@bindvars=("1","2","3","4","5","6",
			"7","8","9","10","11","12");
@bindvals=("7","7","7","7.5","7.5","7.5",
			"testchar7","testvarchar7",
			"01/01/2007","07:00:00",
			"testclob7");
@subvars=("var1","var2","var3");
@subvallongs=(1,2,3);
@subvalstrings=("hi","hello","bye");
@subvaldoubles=(10.55,10.556,10.5556);
@precs=(4,5,6);
@scales=(2,3,4);
$counter=0;

$LARGE_BUFFER_LENGTH=(20*1024);


# instantiation
$con=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"db2inst1","testpassword",0,1);
$cur=SQLRelay::Cursor->new($con);


# identify
print("IDENTIFY: \n");
assertEquals($con->identify(),"db2");
print("\n");


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");


# transaction state
print("TRANSACTION STATE: \n");
assertEquals($con->getDefaultTransactionModel(),"implicit");
assertEquals($con->getTransactionModel(),"implicit");
assertTrue($con->getInTransaction());
assertFalse($con->getAutoCommit());
print("\n");


# bind format
print("BIND FORMAT: \n");
assertEquals($con->bindFormat(),"?");
print("\n");


# nextval format
print("NEXTVAL FORMAT: \n");
assertEquals($con->nextvalFormat(),"(nextval for %s)");
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
	"	testsmallint smallint, ".
	"	testint integer, ".
	"	testbigint bigint, ".
	"	testdecimal decimal(10,2), ".
	"	testreal real, ".
	"	testdouble double, ".
	"	testchar char(40), ".
	"	testvarchar varchar(40), ".
	"	testdate date, ".
	"	testtime time, ".
	"	testtimestamp timestamp, ".
	"	testclob clob, ".
	"	testblob blob)"));
assertTrue($con->commit());
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
	"	1.5, ".
	"	1.5, ".
	"	1.5, ".
	"	'testchar1', ".
	"	'testvarchar1', ".
	"	'01/01/2001', ".
	"	'01:00:00', ".
	"	NULL, ".
	"	'testclob1', ".
	"	blob('testblob1'))"));
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
	"	NULL, ".
	"	?, ".
	"	?)");
assertEquals($cur->countBindVariables(),12);
$cur->inputBind("1",2);
$cur->inputBind("2",2);
$cur->inputBind("3",2);
$cur->inputBind("4",2.5,4,2);
$cur->inputBind("5",2.5,4,2);
$cur->inputBind("6",2.5,4,2);
$cur->inputBind("7","testchar2");
$cur->inputBind("8","testvarchar2");
$cur->inputBindDate("9",2002,1,1,-1,-1,-1,-1,undef,0);
$cur->inputBindDate("10",-1,-1,-1,2,0,0,0,undef,0);
$cur->inputBindClob("11","testclob2",9);
$cur->inputBindBlob("12","testblob2",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",3);
$cur->inputBind("2",3);
$cur->inputBind("3",3);
$cur->inputBind("4",3.5,4,2);
$cur->inputBind("5",3.5,4,2);
$cur->inputBind("6",3.5,4,2);
$cur->inputBind("7","testchar3");
$cur->inputBind("8","testvarchar3");
$cur->inputBindDate("9",2003,1,1,-1,-1,-1,-1,undef,0);
$cur->inputBindDate("10",-1,-1,-1,3,0,0,0,undef,0);
$cur->inputBindClob("11","testclob3",9);
$cur->inputBindBlob("12","testblob3",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",4);
$cur->inputBind("2",4);
$cur->inputBind("3",4);
$cur->inputBind("4",4.5,4,2);
$cur->inputBind("5",4.5,4,2);
$cur->inputBind("6",4.5,4,2);
$cur->inputBind("7","testchar4");
$cur->inputBind("8","testvarchar4");
$cur->inputBindDate("9",2004,1,1,-1,-1,-1,-1,undef,0);
$cur->inputBindDate("10",-1,-1,-1,4,0,0,0,undef,0);
$cur->inputBindClob("11","testclob4",9);
$cur->inputBindBlob("12","testblob4",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",5);
$cur->inputBind("2",5);
$cur->inputBind("3",5);
$cur->inputBind("4",5.5,4,2);
$cur->inputBind("5",5.5,4,2);
$cur->inputBind("6",5.5,4,2);
$cur->inputBind("7","testchar5");
$cur->inputBind("8","testvarchar5");
$cur->inputBindDate("9",2005,1,1,-1,-1,-1,-1,undef,0);
$cur->inputBindDate("10",-1,-1,-1,5,0,0,0,undef,0);
$cur->inputBindClob("11","testclob5",9);
$cur->inputBindBlob("12","testblob5",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",6);
$cur->inputBind("2",6);
$cur->inputBind("3",6);
$cur->inputBind("4",6.5,4,2);
$cur->inputBind("5",6.5,4,2);
$cur->inputBind("6",6.5,4,2);
$cur->inputBind("7","testchar6");
$cur->inputBind("8","testvarchar6");
$cur->inputBindDate("9",2006,1,1,-1,-1,-1,-1,undef,0);
$cur->inputBindDate("10",-1,-1,-1,6,0,0,0,undef,0);
$cur->inputBindClob("11","testclob6",9);
$cur->inputBindBlob("12","testblob6",9);
assertTrue($cur->executeQuery());
print("\n");


# array of input binds by position
print("ARRAY OF INPUT BINDS BY POSITION: \n");
$cur->clearBinds();
$cur->inputBinds(\@bindvars,\@bindvals);
assertTrue($cur->executeQuery());
print("\n");


# input bind by position with validation
print("INPUT BIND BY POSITION WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("1",8);
$cur->inputBind("2",8);
$cur->inputBind("3",8);
$cur->inputBind("4",8.5,4,2);
$cur->inputBind("5",8.5,4,2);
$cur->inputBind("6",8.5,4,2);
$cur->inputBind("7","testchar8");
$cur->inputBind("8","testvarchar8");
$cur->inputBindDate("9",2008,1,1,-1,-1,-1,-1,undef,0);
$cur->inputBindDate("10",-1,-1,-1,8,0,0,0,undef,0);
$cur->inputBindClob("11","testclob8",9);
$cur->inputBindBlob("12","testblob8",9);
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");

# input bind by name
# db2 doesn't support bind by name


# array of input binds by name
# db2 doesn't support bind by name


# input bind by name with validation
# db2 doesn't support bind by name


# select
print("SELECT: \n");
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testsmallint "));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEquals($cur->colCount(),13);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEquals($cur->getColumnName(0),"TESTSMALLINT");
assertEquals($cur->getColumnName(1),"TESTINT");
assertEquals($cur->getColumnName(2),"TESTBIGINT");
assertEquals($cur->getColumnName(3),"TESTDECIMAL");
assertEquals($cur->getColumnName(4),"TESTREAL");
assertEquals($cur->getColumnName(5),"TESTDOUBLE");
assertEquals($cur->getColumnName(6),"TESTCHAR");
assertEquals($cur->getColumnName(7),"TESTVARCHAR");
assertEquals($cur->getColumnName(8),"TESTDATE");
assertEquals($cur->getColumnName(9),"TESTTIME");
assertEquals($cur->getColumnName(10),"TESTTIMESTAMP");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"TESTSMALLINT");
assertEquals($cols[1],"TESTINT");
assertEquals($cols[2],"TESTBIGINT");
assertEquals($cols[3],"TESTDECIMAL");
assertEquals($cols[4],"TESTREAL");
assertEquals($cols[5],"TESTDOUBLE");
assertEquals($cols[6],"TESTCHAR");
assertEquals($cols[7],"TESTVARCHAR");
assertEquals($cols[8],"TESTDATE");
assertEquals($cols[9],"TESTTIME");
assertEquals($cols[10],"TESTTIMESTAMP");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEquals($cur->getColumnType(0),"SMALLINT");
assertEquals($cur->getColumnType("TESTSMALLINT"),"SMALLINT");
assertEquals($cur->getColumnType(1),"INTEGER");
assertEquals($cur->getColumnType("TESTINT"),"INTEGER");
assertEquals($cur->getColumnType(2),"BIGINT");
assertEquals($cur->getColumnType("TESTBIGINT"),"BIGINT");
assertEquals($cur->getColumnType(3),"DECIMAL");
assertEquals($cur->getColumnType("TESTDECIMAL"),"DECIMAL");
assertEquals($cur->getColumnType(4),"REAL");
assertEquals($cur->getColumnType("TESTREAL"),"REAL");
assertEquals($cur->getColumnType(5),"DOUBLE");
assertEquals($cur->getColumnType("TESTDOUBLE"),"DOUBLE");
assertEquals($cur->getColumnType(6),"CHAR");
assertEquals($cur->getColumnType("TESTCHAR"),"CHAR");
assertEquals($cur->getColumnType(7),"VARCHAR");
assertEquals($cur->getColumnType("TESTVARCHAR"),"VARCHAR");
assertEquals($cur->getColumnType(8),"DATE");
assertEquals($cur->getColumnType("TESTDATE"),"DATE");
assertEquals($cur->getColumnType(9),"TIME");
assertEquals($cur->getColumnType("TESTTIME"),"TIME");
assertEquals($cur->getColumnType(10),"TIMESTAMP");
assertEquals($cur->getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEquals($cur->getColumnLength(0),2);
assertEquals($cur->getColumnLength("TESTSMALLINT"),2);
assertEquals($cur->getColumnLength(1),4);
assertEquals($cur->getColumnLength("TESTINT"),4);
assertEquals($cur->getColumnLength(2),8);
assertEquals($cur->getColumnLength("TESTBIGINT"),8);
assertEquals($cur->getColumnLength(3),12);
assertEquals($cur->getColumnLength("TESTDECIMAL"),12);
assertEquals($cur->getColumnLength(4),4);
assertEquals($cur->getColumnLength("TESTREAL"),4);
assertEquals($cur->getColumnLength(5),8);
assertEquals($cur->getColumnLength("TESTDOUBLE"),8);
assertEquals($cur->getColumnLength(6),40);
assertEquals($cur->getColumnLength("TESTCHAR"),40);
assertEquals($cur->getColumnLength(7),40);
assertEquals($cur->getColumnLength("TESTVARCHAR"),40);
assertEquals($cur->getColumnLength(8),6);
assertEquals($cur->getColumnLength("TESTDATE"),6);
assertEquals($cur->getColumnLength(9),6);
assertEquals($cur->getColumnLength("TESTTIME"),6);
assertEquals($cur->getColumnLength(10),16);
assertEquals($cur->getColumnLength("TESTTIMESTAMP"),16);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEquals($cur->getLongest(0),1);
assertEquals($cur->getLongest("TESTSMALLINT"),1);
assertEquals($cur->getLongest(1),1);
assertEquals($cur->getLongest("TESTINT"),1);
assertEquals($cur->getLongest(2),1);
assertEquals($cur->getLongest("TESTBIGINT"),1);
assertEquals($cur->getLongest(3),4);
assertEquals($cur->getLongest("TESTDECIMAL"),4);
assertEquals($cur->getLongest(4),12);
assertEquals($cur->getLongest("TESTREAL"),12);
assertEquals($cur->getLongest(5),21);
assertEquals($cur->getLongest("TESTDOUBLE"),21);
assertEquals($cur->getLongest(6),40);
assertEquals($cur->getLongest("TESTCHAR"),40);
assertEquals($cur->getLongest(7),12);
assertEquals($cur->getLongest("TESTVARCHAR"),12);
assertEquals($cur->getLongest(8),10);
assertEquals($cur->getLongest("TESTDATE"),10);
assertEquals($cur->getLongest(9),8);
assertEquals($cur->getLongest("TESTTIME"),8);
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
assertEquals($cur->getField(0,3),"1.50");
assertEquals($cur->getField(0,4),"1.500000E+00");
assertEquals($cur->getField(0,5),"1.50000000000000E+000");
assertEquals($cur->getField(0,6),"testchar1                               ");
assertEquals($cur->getField(0,7),"testvarchar1");
assertEquals($cur->getField(0,8),"2001-01-01");
assertEquals($cur->getField(0,9),"01:00:00");
print("\n");
assertEquals($cur->getField(7,0),"8");
assertEquals($cur->getField(7,1),"8");
assertEquals($cur->getField(7,2),"8");
assertEquals($cur->getField(7,3),"8.50");
assertEquals($cur->getField(7,4),"8.500000E+00");
assertEquals($cur->getField(7,5),"8.50000000000000E+000");
assertEquals($cur->getField(7,6),"testchar8                               ");
assertEquals($cur->getField(7,7),"testvarchar8");
assertEquals($cur->getField(7,8),"2008-01-01");
assertEquals($cur->getField(7,9),"08:00:00");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEquals($cur->getFieldLength(0,0),1);
assertEquals($cur->getFieldLength(0,1),1);
assertEquals($cur->getFieldLength(0,2),1);
assertEquals($cur->getFieldLength(0,3),4);
assertEquals($cur->getFieldLength(0,4),12);
assertEquals($cur->getFieldLength(0,5),21);
assertEquals($cur->getFieldLength(0,6),40);
assertEquals($cur->getFieldLength(0,7),12);
assertEquals($cur->getFieldLength(0,8),10);
assertEquals($cur->getFieldLength(0,9),8);
print("\n");
assertEquals($cur->getFieldLength(7,0),1);
assertEquals($cur->getFieldLength(7,1),1);
assertEquals($cur->getFieldLength(7,2),1);
assertEquals($cur->getFieldLength(7,3),4);
assertEquals($cur->getFieldLength(7,4),12);
assertEquals($cur->getFieldLength(7,5),21);
assertEquals($cur->getFieldLength(7,6),40);
assertEquals($cur->getFieldLength(7,7),12);
assertEquals($cur->getFieldLength(7,8),10);
assertEquals($cur->getFieldLength(7,9),8);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEquals($cur->getField(0,"TESTSMALLINT"),"1");
assertEquals($cur->getField(0,"TESTINT"),"1");
assertEquals($cur->getField(0,"TESTBIGINT"),"1");
assertEquals($cur->getField(0,"TESTDECIMAL"),"1.50");
assertEquals($cur->getField(0,"TESTREAL"),"1.500000E+00");
assertEquals($cur->getField(0,"TESTDOUBLE"),"1.50000000000000E+000");
assertEquals($cur->getField(0,"TESTCHAR"),"testchar1                               ");
assertEquals($cur->getField(0,"TESTVARCHAR"),"testvarchar1");
assertEquals($cur->getField(0,"TESTDATE"),"2001-01-01");
assertEquals($cur->getField(0,"TESTTIME"),"01:00:00");
print("\n");
assertEquals($cur->getField(7,"TESTSMALLINT"),"8");
assertEquals($cur->getField(7,"TESTINT"),"8");
assertEquals($cur->getField(7,"TESTBIGINT"),"8");
assertEquals($cur->getField(7,"TESTDECIMAL"),"8.50");
assertEquals($cur->getField(7,"TESTREAL"),"8.500000E+00");
assertEquals($cur->getField(7,"TESTDOUBLE"),"8.50000000000000E+000");
assertEquals($cur->getField(7,"TESTCHAR"),"testchar8                               ");
assertEquals($cur->getField(7,"TESTVARCHAR"),"testvarchar8");
assertEquals($cur->getField(7,"TESTDATE"),"2008-01-01");
assertEquals($cur->getField(7,"TESTTIME"),"08:00:00");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEquals($cur->getFieldLength(0,"TESTSMALLINT"),1);
assertEquals($cur->getFieldLength(0,"TESTINT"),1);
assertEquals($cur->getFieldLength(0,"TESTBIGINT"),1);
assertEquals($cur->getFieldLength(0,"TESTDECIMAL"),4);
assertEquals($cur->getFieldLength(0,"TESTREAL"),12);
assertEquals($cur->getFieldLength(0,"TESTDOUBLE"),21);
assertEquals($cur->getFieldLength(0,"TESTCHAR"),40);
assertEquals($cur->getFieldLength(0,"TESTVARCHAR"),12);
assertEquals($cur->getFieldLength(0,"TESTDATE"),10);
assertEquals($cur->getFieldLength(0,"TESTTIME"),8);
print("\n");
assertEquals($cur->getFieldLength(7,"TESTSMALLINT"),1);
assertEquals($cur->getFieldLength(7,"TESTINT"),1);
assertEquals($cur->getFieldLength(7,"TESTBIGINT"),1);
assertEquals($cur->getFieldLength(7,"TESTDECIMAL"),4);
assertEquals($cur->getFieldLength(7,"TESTREAL"),12);
assertEquals($cur->getFieldLength(7,"TESTDOUBLE"),21);
assertEquals($cur->getFieldLength(7,"TESTCHAR"),40);
assertEquals($cur->getFieldLength(7,"TESTVARCHAR"),12);
assertEquals($cur->getFieldLength(7,"TESTDATE"),10);
assertEquals($cur->getFieldLength(7,"TESTTIME"),8);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEquals($fields[0],"1");
assertEquals($fields[1],"1");
assertEquals($fields[2],"1");
assertEquals($fields[3],"1.50");
assertEquals($fields[4],"1.500000E+00");
assertEquals($fields[5],"1.50000000000000E+000");
assertEquals($fields[6],"testchar1                               ");
assertEquals($fields[7],"testvarchar1");
assertEquals($fields[8],"2001-01-01");
assertEquals($fields[9],"01:00:00");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEquals($fieldlens[0],1);
assertEquals($fieldlens[1],1);
assertEquals($fieldlens[2],1);
assertEquals($fieldlens[3],4);
assertEquals($fieldlens[4],12);
assertEquals($fieldlens[5],21);
assertEquals($fieldlens[6],40);
assertEquals($fieldlens[7],12);
assertEquals($fieldlens[8],10);
assertEquals($fieldlens[9],8);
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
	"	testsmallint "));
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
	"	testsmallint "));
assertUndef($cur->getColumnName(0));
assertEquals($cur->getColumnLength(0),0);
assertUndef($cur->getColumnType(0));
$cur->getColumnInfo();
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testsmallint "));
assertEquals($cur->getColumnName(0),"TESTSMALLINT");
assertEquals($cur->getColumnLength(0),2);
assertEquals($cur->getColumnType(0),"SMALLINT");
print("\n");


# suspended session
print("SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testsmallint "));
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
	"	testsmallint "));
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
	"	testsmallint "));
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
	"	testsmallint "));
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
	"	testsmallint "));
$filename=$cur->getCacheFileName();
assertEquals($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEquals($cur->getField(7,0),"8");
print("\n");


# column count for cached result set
print("COLUMN COUNT FOR CACHED RESULT SET: \n");
assertEquals($cur->colCount(),13);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEquals($cur->getColumnName(0),"TESTSMALLINT");
assertEquals($cur->getColumnName(1),"TESTINT");
assertEquals($cur->getColumnName(2),"TESTBIGINT");
assertEquals($cur->getColumnName(3),"TESTDECIMAL");
assertEquals($cur->getColumnName(4),"TESTREAL");
assertEquals($cur->getColumnName(5),"TESTDOUBLE");
assertEquals($cur->getColumnName(6),"TESTCHAR");
assertEquals($cur->getColumnName(7),"TESTVARCHAR");
assertEquals($cur->getColumnName(8),"TESTDATE");
assertEquals($cur->getColumnName(9),"TESTTIME");
assertEquals($cur->getColumnName(10),"TESTTIMESTAMP");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"TESTSMALLINT");
assertEquals($cols[1],"TESTINT");
assertEquals($cols[2],"TESTBIGINT");
assertEquals($cols[3],"TESTDECIMAL");
assertEquals($cols[4],"TESTREAL");
assertEquals($cols[5],"TESTDOUBLE");
assertEquals($cols[6],"TESTCHAR");
assertEquals($cols[7],"TESTVARCHAR");
assertEquals($cols[8],"TESTDATE");
assertEquals($cols[9],"TESTTIME");
assertEquals($cols[10],"TESTTIMESTAMP");
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
	"	testsmallint "));
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
	"	testsmallint "));
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
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testint"));
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
$cur->setResultSetBufferSize(1);
assertTrue($cur->sendQuery("select * from testtable"));
$secondcur=SQLRelay::Cursor->new($con);
$secondcur->setResultSetBufferSize(1);
for ($i=0; ; $i++) {
	@row=$cur->getRow($i);
	last if (!@row);
	assertTrue($secondcur->sendQuery("select * from testtable"));
}
$secondcur->closeResultSet();
$cur->setResultSetBufferSize(0);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# reset transaction state
print("RESET TRANSACTION STATE: \n");
assertTrue($con->commit());
assertEquals($con->getTransactionModel(),"implicit");
assertFalse($con->getAutoCommit());
print("\n");


# transaction behavior - implicit
print("TRANSACTION BEHAVIOR - implicit: \n");
assertTrue($con->setTransactionModel("implicit"));
assertEquals($con->getTransactionModel(),"implicit");
assertTrue($cur->sendQuery("create table testtable (col1 integer)"));
# db2 DDL is transactional; commit so the table is visible to the
# second connection (the commit implicitly starts a new tx)
assertTrue($con->commit());
$secondcon=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"db2inst1","testpassword",0,1);
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
assertEquals($con->getTransactionModel(),"implicit");
assertFalse($con->getAutoCommit());
print("\n");


# individual substitutions
print("INDIVIDUAL SUBSTITUTIONS: \n");
$cur->prepareQuery("values (\$(var1),'\$(var2)','\$(var3)')");
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
$cur->prepareQuery("values ('\$(var1)','\$(var2)','\$(var3)')");
$cur->substitutions(\@subvars,\@subvalstrings);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"hi");
assertEquals($cur->getField(0,1),"hello");
assertEquals($cur->getField(0,2),"bye");
print("\n");
$cur->prepareQuery("values (\$(var1),\$(var2),\$(var3))");
$cur->substitutions(\@subvars,\@subvallongs);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"2");
assertEquals($cur->getField(0,2),"3");
print("\n");
$cur->prepareQuery("values (\$(var1),\$(var2),\$(var3))");
$cur->substitutions(\@subvars,\@subvaldoubles,\@precs,\@scales);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"10.55");
assertEquals($cur->getField(0,1),"10.556");
assertEquals($cur->getField(0,2),"10.5556");
print("\n");


# nulls as nulls
print("NULLS AS NULLS: \n");
$cur->getNullsAsUndefined();
assertTrue($cur->sendQuery("select NULL,1,NULL from sysibm.sysdummy1"));
assertUndef($cur->getField(0,0));
assertEquals($cur->getField(0,1),"1");
assertUndef($cur->getField(0,2));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("select NULL,1,NULL from sysibm.sysdummy1"));
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
	"	testclob1 clob, ".
	"	testclob2 clob, ".
	"	testblob1 blob, ".
	"	testblob2 blob)"));
assertTrue($con->commit());
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
assertTrue($con->commit());
print("\n");


# long lobs
print("LONG LOBS: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testclob clob, ".
	"	testblob blob)"));
assertTrue($con->commit());
$cur->prepareQuery("insert into testtable values (?,?)");
$largebuffer=('C' x $LARGE_BUFFER_LENGTH);
$cur->inputBindClob("1",$largebuffer,$LARGE_BUFFER_LENGTH);
$cur->inputBindBlob("2",$largebuffer,$LARGE_BUFFER_LENGTH);
assertTrue($cur->executeQuery());
$cur->sendQuery("select * from testtable");
assertEquals($cur->getFieldLength(0,"TESTCLOB"),$LARGE_BUFFER_LENGTH);
assertEquals($cur->getField(0,"TESTCLOB"),$largebuffer);
assertEquals($cur->getFieldLength(0,"TESTBLOB"),$LARGE_BUFFER_LENGTH);
assertEqualsBytes($cur->getField(0,"TESTBLOB"),$largebuffer,
					$LARGE_BUFFER_LENGTH);
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($con->commit());
print("\n");


# output bind by position
print("OUTPUT BIND BY POSITION: \n");
$cur->sendQuery("drop procedure testproc");
$cur->getNullsAsUndefined();
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	out out1 int, ".
	"	out out2 varchar(20), ".
	"	out out3 double, ".
	"	out out4 date, ".
	"	out out5 varchar(20)) ".
	"language sql ".
	"begin ".
	"	set out1 = 1; ".
	"	set out2 = 'hello'; ".
	"	set out3 = 2.5; ".
	"	set out4 = '2001-02-03'; ".
	"	set out5 = null; ".
	"end"));
assertTrue($con->commit());
$cur->prepareQuery("call testproc(?,?,?,?,?)");
assertEquals($cur->countBindVariables(),5);
$cur->defineOutputBindInteger("1");
$cur->defineOutputBindString("2",20);
$cur->defineOutputBindDouble("3");
$cur->defineOutputBindDate("4");
$cur->defineOutputBindString("5",20);
assertTrue($cur->executeQuery());
$numvar=$cur->getOutputBindInteger("1");
$stringvar=$cur->getOutputBindString("2");
$floatvar=$cur->getOutputBindDouble("3");
$year=$cur->getOutputBindDateYear("4");
$month=$cur->getOutputBindDateMonth("4");
$day=$cur->getOutputBindDateDay("4");
$hour=$cur->getOutputBindDateHour("4");
$minute=$cur->getOutputBindDateMinute("4");
$second=$cur->getOutputBindDateSecond("4");
$microsecond=$cur->getOutputBindDateMicrosecond("4");
$tz=$cur->getOutputBindDateTz("4");
assertEquals($numvar,1);
assertEquals($stringvar,"hello");
assertEquals($floatvar,2.5);
assertEquals($year,2001);
assertEquals($month,2);
assertEquals($day,3);
assertEquals($hour,0);
assertEquals($minute,0);
assertEquals($second,0);
assertEquals($microsecond,0);
assertEquals($tz,"");
$nullvar=$cur->getOutputBindString("5");
assertUndef($nullvar);
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("drop procedure testproc"));
assertTrue($con->commit());
print("\n");


# output bind by name
# db2 doesn't support bind by name


# output bind by name with validation
# db2 doesn't support bind by name


# lob output bind
print("LOB OUTPUT BIND: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery(
	"create table testtable (".
	"	testclob clob, ".
	"	testblob blob)");
assertTrue($con->commit());
$cur->prepareQuery("insert into testtable values ('hello',?)");
$cur->inputBindBlob("1","hello",5);
assertTrue($cur->executeQuery());
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	out out1 clob, ".
	"	out out2 blob) ".
	"language sql ".
	"begin ".
	"	select testclob into out1 from testtable; ".
	"	select testblob into out2 from testtable; ".
	"end"));
assertTrue($con->commit());
$cur->prepareQuery("call testproc(?,?)");
$cur->defineOutputBindClob("1");
$cur->defineOutputBindBlob("2");
assertTrue($cur->executeQuery());
$clobvar=$cur->getOutputBindClob("1");
$clobvarlength=$cur->getOutputBindLength("1");
$blobvar=$cur->getOutputBindBlob("2");
$blobvarlength=$cur->getOutputBindLength("2");
assertEqualsBytes($clobvar,"hello",5);
assertEquals($clobvarlength,5);
assertEqualsBytes($blobvar,"hello",5);
assertEquals($blobvarlength,5);
assertTrue($cur->sendQuery("drop procedure testproc"));
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($con->commit());
print("\n");


# long output bind
print("LONG OUTPUT BIND: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	in in1 clob, ".
	"	out out1 clob) ".
	"language sql ".
	"begin ".
	"	set out1 = in1; ".
	"end"));
assertTrue($con->commit());
$largebuffer=('C' x $LARGE_BUFFER_LENGTH);
$cur->prepareQuery("call testproc(?,?)");
$cur->inputBindClob("1",$largebuffer,$LARGE_BUFFER_LENGTH);
$cur->defineOutputBindClob("2");
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindLength("2"),$LARGE_BUFFER_LENGTH);
assertEquals($cur->getOutputBindClob("2"),$largebuffer);
assertTrue($cur->sendQuery("drop procedure testproc"));
assertTrue($con->commit());
print("\n");


# negative input bind
print("NEGATIVE INPUT BIND: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery("create table testtable (testval integer)");
assertTrue($con->commit());
$cur->prepareQuery("insert into testtable values (?)");
$cur->inputBind("1",-1);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testval from testtable");
assertEquals($cur->getField(0,"TESTVAL"),"-1");
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($con->commit());
print("\n");


# bind validation
# db2 doesn't support bind by name


# rebinding
print("REBINDING: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	in in1 int, ".
	"	out out1 int) ".
	"language sql ".
	"begin ".
	"	set out1 = in1; ".
	"end"));
assertTrue($con->commit());
$cur->prepareQuery("call testproc(?,?)");
$cur->inputBind("1",1);
$cur->defineOutputBindInteger("2");
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("2"),1);
$cur->inputBind("1",2);
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("2"),2);
$cur->inputBind("1",3);
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("2"),3);
assertTrue($cur->sendQuery("drop procedure testproc"));
assertTrue($con->commit());
print("\n");


# reexecute
print("REEXECUTE: \n");
$cur->prepareQuery("select 1 from sysibm.sysdummy1");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
$cur->prepareQuery("select cast(? as integer) from sysibm.sysdummy1");
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
	"language sql ".
	"begin ".
	"	return; ".
	"end"));
assertTrue($con->commit());
$cur->prepareQuery("call testproc(?,?,?)");
$cur->inputBind("1",1);
$cur->inputBind("2",1.1,2,1);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("drop procedure testproc"));
assertTrue($con->commit());
print("\n");


# stored procedure returning single value
print("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
$cur->sendQuery("drop function testfunc");
assertTrue($cur->sendQuery(
	"create function testfunc(".
	"	in1 int, ".
	"	in2 double, ".
	"	in3 varchar(20)) ".
	"returns int ".
	"language sql ".
	"begin ".
	"	return in1; ".
	"end"));
assertTrue($con->commit());
$cur->prepareQuery("select testfunc(?,?,?) from sysibm.sysdummy1");
$cur->inputBind("1",1);
$cur->inputBind("2",1.1,2,1);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertTrue($cur->sendQuery("drop function testfunc"));
assertTrue($con->commit());
print("\n");


# stored procedure returning multiple values
print("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc(".
	"	in in1 int, ".
	"	in in2 double, ".
	"	in in3 varchar(20), ".
	"	in in4 clob, ".
	"	in in5 blob, ".
	"	out out1 int, ".
	"	out out2 double, ".
	"	out out3 varchar(20), ".
	"	out out4 clob, ".
	"	out out5 blob) ".
	"language sql ".
	"begin ".
	"	set out1 = in1; ".
	"	set out2 = in2; ".
	"	set out3 = in3; ".
	"	set out4 = in4; ".
	"	set out5 = in5; ".
	"end"));
assertTrue($con->commit());
$cur->prepareQuery("call testproc(?,?,?,?,?,?,?,?,?,?)");
$cur->inputBind("1",1);
$cur->inputBind("2",1.1,2,1);
$cur->inputBind("3","hello");
$cur->inputBindClob("4","clob",4);
$cur->inputBindBlob("5","blob",4);
$cur->defineOutputBindInteger("6");
$cur->defineOutputBindDouble("7");
$cur->defineOutputBindString("8",20);
$cur->defineOutputBindClob("9");
$cur->defineOutputBindBlob("10");
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("6"),1);
assertEquals($cur->getOutputBindDouble("7"),1.1);
assertEquals($cur->getOutputBindString("8"),"hello");
assertEquals($cur->getOutputBindClob("9"),"clob");
assertEquals($cur->getOutputBindBlob("10"),"blob");
assertTrue($cur->sendQuery("drop procedure testproc"));
assertTrue($con->commit());
print("\n");


# stored procedure returning result set
print("STORED PROCEDURE RETURNING RESULT SET: \n");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create procedure testproc() ".
	"result set 1 ".
	"language sql ".
	"begin ".
	"	declare c1 cursor with return for ".
	"		select 1 from sysibm.sysdummy1 ".
	"		union ".
	"		select 2 from sysibm.sysdummy1 ".
	"		union ".
	"		select 3 from sysibm.sysdummy1 ".
	"		union ".
	"		select 4 from sysibm.sysdummy1 ".
	"		union ".
	"		select 5 from sysibm.sysdummy1 ".
	"		union ".
	"		select 6 from sysibm.sysdummy1 ".
	"		union ".
	"		select 7 from sysibm.sysdummy1 ".
	"		union ".
	"		select 8 from sysibm.sysdummy1; ".
	"	open c1; ".
	"end"));
assertTrue($con->commit());
assertTrue($cur->sendQuery("call testproc()"));
assertEquals($cur->rowCount(),8);
assertTrue($cur->sendQuery("drop procedure testproc"));
assertTrue($con->commit());
print("\n");


# temporary tables
print("TEMPORARY TABLES: \n");
$cur->sendQuery("drop table session.temptable");
assertTrue($cur->sendQuery("declare global temporary table session.temptable ".
					"(col1 int) not logged"));
assertTrue($cur->sendQuery("insert into session.temptable values (1)"));
assertTrue($cur->sendQuery("select count(*) from session.temptable"));
assertEquals($cur->getField(0,0),"1");
$con->endSession();
print("\n");
assertFalse($cur->sendQuery("select count(*) from session.temptable"));
print("\n");


# encoded binary data
print("ENCODED BINARY DATA: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 blob)"));
$buffer=pack("C*",(0..255));
$hex=unpack("H*",$buffer);
$querystr="insert into testtable values (blob(X'$hex'))";
assertTrue($cur->sendQuery($querystr));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),256);
assertEqualsBytes($cur->getField(0,0),$buffer,256);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes
print("QUOTES: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('''''')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertTrue($cur->getField(0,0) eq "''");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# last insert id
print("LAST INSERT ID: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
		"create table testtable ".
		"	(col1 int not null ".
		"	generated always as identity, ".
		"	col2 int, ".
		"	primary key(col1))"));
assertTrue($cur->sendQuery(
		"insert into testtable (col2) values (1)"));
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
assertEquals($cur->rowCount(),0);
print("\n");


# schema list
print("SCHEMA LIST: \n");
assertTrue($cur->getSchemaList(undef));
assertEquals($cur->getColumnName(0),"Database");
$found=0;
for ($i=0; $i<$cur->rowCount(); $i++) {
	if ($cur->getField($i,"Database") eq "DB2INST1") {
		$found=1;
		last;
	}
}
assertTrue($found);
print("\n");


# table type list
print("TABLE TYPE LIST: \n");
assertTrue($cur->getTableTypeList());
assertEquals($cur->getColumnName(0),"table_type");
$found=0;
for ($i=0; $i<$cur->rowCount(); $i++) {
	if ($cur->getField($i,"table_type") eq "TABLE") {
		$found=1;
		last;
	}
}
assertTrue($found);
print("\n");


# table list
print("TABLE LIST: \n");
$cur->sendQuery("drop table testtable1");
$cur->sendQuery("drop table testtable2");
$cur->sendQuery("drop table testtable3");
$cur->sendQuery("drop table testtable4");
assertTrue($cur->sendQuery(
	"create table testtable1 (".
	"	col1 integer, ".
	"	col2 integer)"));
assertTrue($cur->sendQuery(
	"create table testtable2 (".
	"	col1 integer, ".
	"	col2 integer)"));
assertTrue($cur->sendQuery(
	"create table testtable3 (".
	"	col1 integer, ".
	"	col2 integer)"));
assertTrue($cur->sendQuery(
	"create table testtable4 (".
	"	col1 integer, ".
	"	col2 integer)"));
assertTrue($con->commit());
assertTrue($cur->getTableList(undef));
$counter=0;
for ($i=0; $i<$cur->rowCount(); $i++) {
	$name=$cur->getField($i,"Tables_in_xxx");
	if (defined($name) &&
		($name eq "TESTTABLE1" ||
		$name eq "TESTTABLE2" ||
		$name eq "TESTTABLE3" ||
		$name eq "TESTTABLE4")) {
		$counter++;
	}
}
assertEquals($counter,4);
assertTrue($cur->sendQuery("drop table testtable1"));
assertTrue($cur->sendQuery("drop table testtable2"));
assertTrue($cur->sendQuery("drop table testtable3"));
assertTrue($cur->sendQuery("drop table testtable4"));
assertTrue($con->commit());
print("\n");


# type info list
print("TYPE INFO LIST: \n");
assertTrue($cur->getTypeInfoList("integer"));
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
assertEquals($cur->getField(0,"type_name"),"INTEGER");
assertEquals($cur->getField(0,"data_type"),"4");
assertEquals($cur->getField(0,"precision"),"10");
assertEquals($cur->getField(0,"local_type_name"),"INTEGER");
assertTrue($cur->getTypeInfoList("char"));
assertEquals($cur->getField(0,"type_name"),"CHAR");
assertEquals($cur->getField(0,"data_type"),"1");
assertEquals($cur->getField(0,"precision"),"254");
assertEquals($cur->getField(0,"local_type_name"),"CHAR");
assertTrue($cur->getTypeInfoList("varchar"));
assertEquals($cur->getField(0,"type_name"),"VARCHAR");
assertEquals($cur->getField(0,"data_type"),"12");
assertEquals($cur->getField(0,"precision"),"32672");
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
	"	testsmallint smallint, ".
	"	testint integer, ".
	"	testbigint bigint, ".
	"	testdecimal decimal(10,2), ".
	"	testreal real, ".
	"	testdouble double, ".
	"	testchar char(40), ".
	"	testvarchar varchar(40), ".
	"	testdate date, ".
	"	testtime time, ".
	"	testtimestamp timestamp, ".
	"	testclob clob, ".
	"	testblob blob)"));
assertTrue($con->commit());
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
assertEquals($cur->getField(0,"column_name"),"TESTSMALLINT");
assertEquals($cur->getField(1,"column_name"),"TESTINT");
assertEquals($cur->getField(2,"column_name"),"TESTBIGINT");
assertEquals($cur->getField(3,"column_name"),"TESTDECIMAL");
assertEquals($cur->getField(4,"column_name"),"TESTREAL");
assertEquals($cur->getField(5,"column_name"),"TESTDOUBLE");
assertEquals($cur->getField(6,"column_name"),"TESTCHAR");
assertEquals($cur->getField(7,"column_name"),"TESTVARCHAR");
assertEquals($cur->getField(8,"column_name"),"TESTDATE");
assertEquals($cur->getField(9,"column_name"),"TESTTIME");
assertEquals($cur->getField(10,"column_name"),"TESTTIMESTAMP");
assertEquals($cur->getField(11,"column_name"),"TESTCLOB");
assertEquals($cur->getField(12,"column_name"),"TESTBLOB");
assertEquals($cur->getField(0,"data_type"),"SMALLINT");
assertEquals($cur->getField(1,"data_type"),"INTEGER");
assertEquals($cur->getField(2,"data_type"),"BIGINT");
assertEquals($cur->getField(3,"data_type"),"DECIMAL");
assertEquals($cur->getField(4,"data_type"),"REAL");
assertEquals($cur->getField(5,"data_type"),"DOUBLE");
assertEquals($cur->getField(6,"data_type"),"CHARACTER");
assertEquals($cur->getField(7,"data_type"),"VARCHAR");
assertEquals($cur->getField(8,"data_type"),"DATE");
assertEquals($cur->getField(9,"data_type"),"TIME");
assertEquals($cur->getField(10,"data_type"),"TIMESTAMP");
assertEquals($cur->getField(11,"data_type"),"CLOB");
assertEquals($cur->getField(12,"data_type"),"BLOB");
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($con->commit());
print("\n");


# column list - auto_increment, primary key
print("COLUMN LIST - auto_increment, primary key: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int generated always as identity primary key, ".
	"	col2 int)"));
assertTrue($con->commit());
assertTrue($cur->getColumnList("testtable",undef));
assertTrue(index($cur->getField(0,"extra"),"auto_increment")>=0);
assertTrue(index($cur->getField(0,"column_key"),"PRI")>=0);
assertFalse(index($cur->getField(1,"extra"),"auto_increment")>=0);
assertFalse(index($cur->getField(1,"column_key"),"PRI")>=0);
print("\n");
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int not null primary key, ".
	"	col2 int)"));
assertTrue($con->commit());
assertTrue($cur->getColumnList("testtable",undef));
assertFalse(index($cur->getField(0,"extra"),"auto_increment")>=0);
assertTrue(index($cur->getField(0,"column_key"),"PRI")>=0);
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($con->commit());
print("\n");


# primary keys list
print("PRIMARY KEYS LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int not null primary key, ".
	"	col2 int)"));
assertTrue($con->commit());
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
assertTrue($cur->getField(0,"table") eq "TESTTABLE");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertTrue($cur->getField(0,"column_name") eq "COL1");
$keyname=$cur->getField(0,"key_name");
assertTrue(defined($keyname) && length($keyname)>0);
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($con->commit());
print("\n");


# key and index list
print("KEY AND INDEX LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int not null primary key, ".
	"	col2 int)"));
assertTrue($con->commit());
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
assertTrue($cur->getField(0,"table") eq "TESTTABLE");
assertEquals($cur->getField(0,"non_unique"),"0");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertTrue($cur->getField(0,"column_name") eq "COL1");
assertEquals($cur->getField(0,"collation"),"A");
assertEquals($cur->getField(0,"index_type"),"3");
$keyname=$cur->getField(0,"key_name");
assertTrue(defined($keyname) && length($keyname)>0);
assertTrue($cur->sendQuery("drop table testtable"));
assertTrue($con->commit());
print("\n");


# procedure list
print("PROCEDURE LIST: \n");
$cur->sendQuery("drop procedure testproc1");
$cur->sendQuery("drop procedure testproc2");
$cur->sendQuery("drop procedure testproc3");
$cur->sendQuery("drop procedure testproc4");
assertTrue($cur->sendQuery(
	"create procedure testproc1(".
	"	in in1 integer, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"language sql begin end"));
assertTrue($cur->sendQuery(
	"create procedure testproc2(".
	"	in in1 integer, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"language sql begin end"));
assertTrue($cur->sendQuery(
	"create procedure testproc3(".
	"	in in1 integer, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"language sql begin end"));
assertTrue($cur->sendQuery(
	"create procedure testproc4(".
	"	in in1 integer, ".
	"	in in2 char(20), ".
	"	in in3 varchar(20), ".
	"	in in4 date) ".
	"language sql begin end"));
assertTrue($con->commit());
assertTrue($cur->getProcedureList(undef));
$counter=0;
for ($i=0; $i<$cur->rowCount(); $i++) {
	$name=$cur->getField($i,"routine_name");
	if (defined($name) &&
		($name eq "TESTPROC1" ||
		$name eq "TESTPROC2" ||
		$name eq "TESTPROC3" ||
		$name eq "TESTPROC4")) {
		$counter++;
	}
}
assertEquals($counter,4);
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
assertEquals($cur->getField(0,"parameter_name"),"IN1");
assertEquals($cur->getField(0,"parameter_mode"),"1");
assertEquals($cur->getField(0,"data_type"),"INTEGER");
assertEquals($cur->getField(0,"ordinal_position"),"1");
assertEquals($cur->getField(1,"parameter_name"),"IN2");
assertEquals($cur->getField(1,"parameter_mode"),"1");
assertEquals($cur->getField(1,"data_type"),"CHARACTER");
assertEquals($cur->getField(1,"ordinal_position"),"2");
assertEquals($cur->getField(2,"parameter_name"),"IN3");
assertEquals($cur->getField(2,"parameter_mode"),"1");
assertEquals($cur->getField(2,"data_type"),"VARCHAR");
assertEquals($cur->getField(2,"ordinal_position"),"3");
assertEquals($cur->getField(3,"parameter_name"),"IN4");
assertEquals($cur->getField(3,"parameter_mode"),"1");
assertEquals($cur->getField(3,"data_type"),"DATE");
assertEquals($cur->getField(3,"ordinal_position"),"4");
assertTrue($cur->sendQuery("drop procedure testproc1"));
assertTrue($cur->sendQuery("drop procedure testproc2"));
assertTrue($cur->sendQuery("drop procedure testproc3"));
assertTrue($cur->sendQuery("drop procedure testproc4"));
assertTrue($con->commit());
print("\n");


# invalid queries
print("INVALID QUERIES: \n");
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testsmallint "));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testsmallint "));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testsmallint "));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testsmallint "));
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
