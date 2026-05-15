#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.


use SQLRelay::Connection;
use SQLRelay::Cursor;
require "./asserts.pl";


@isolationlevels=("0","1");
@subvars=("var1","var2","var3");
@subvalstrings=("hi","hello","bye");
@subvallongs=(1,2,3);
@subvaldoubles=(10.55,10.556,10.5556);
@precs=(4,5,6);
@scales=(2,3,4);

$LARGE_BUFFER_LENGTH=8192;


# instantiation
$con=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$cur=SQLRelay::Cursor->new($con);


# identify
print("IDENTIFY: \n");
assertEquals($con->identify(),"sqlite");
print("\n");


# db version
print("DB VERSION: \n");
$dbversion=$con->dbVersion();
$issqlite3=1;
if (!defined($dbversion) ||
	$dbversion eq "unknown" ||
	int($dbversion)<3) {
	$issqlite3=0;
}
print("\n");


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");


# transaction state
print("TRANSACTION STATE: \n");
assertEquals($con->getDefaultTransactionModel(),"explicit");
assertEquals($con->getTransactionModel(),"explicit");
assertFalse($con->getInTransaction());
assertTrue($con->getAutoCommit());
print("\n");


# bind format
print("BIND FORMAT: \n");
assertEquals($con->bindFormat(),":*");
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
$con->begin();
$cur->sendQuery("drop table if exists testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testint int, ".
	"	testfloat float, ".
	"	testchar char(40), ".
	"	testvarchar varchar(40), ".
	"	testclob clob, ".
	"	testblob blob)"));
$con->commit();
print("\n");


# insert
print("INSERT: \n");
assertTrue($con->begin());
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	1, ".
	"	1.1, ".
	"	'testchar1', ".
	"	'testvarchar1', ".
	"	'testclob1', ".
	"	'testblob1')"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	2, ".
	"	2.2, ".
	"	'testchar2', ".
	"	'testvarchar2', ".
	"	'testclob2', ".
	"	'testblob2')"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	3, ".
	"	3.3, ".
	"	'testchar3', ".
	"	'testvarchar3', ".
	"	'testclob3', ".
	"	'testblob3')"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	4, ".
	"	4.4, ".
	"	'testchar4', ".
	"	'testvarchar4', ".
	"	'testclob4', ".
	"	'testblob4')"));
print("\n");


# affected rows
print("AFFECTED ROWS: \n");
assertEquals($cur->affectedRows(),1);
print("\n");


# input bind by position
# sqlite doesn't support bind by position


# array of input binds by position
# sqlite doesn't support bind by position


# input bind by position with validation
# sqlite doesn't support bind by position


# input bind by name
print("INPUT BIND BY NAME: \n");
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	:var1, ".
	"	:var2, ".
	"	:var3, ".
	"	:var4, ".
	"	:var5, ".
	"	:var6)");
assertEquals($cur->countBindVariables(),6);
$cur->inputBind("var1",5);
$cur->inputBind("var2",5.5,4,1);
$cur->inputBind("var3","testchar5");
$cur->inputBind("var4","testvarchar5");
$cur->inputBindClob("var5","testclob5",9);
$cur->inputBindBlob("var6","testblob5",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("var1",6);
$cur->inputBind("var2",6.6,4,1);
$cur->inputBind("var3","testchar6");
$cur->inputBind("var4","testvarchar6");
$cur->inputBindClob("var5","testclob6",9);
$cur->inputBindBlob("var6","testblob6",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("var1",7);
$cur->inputBind("var2",7.7,4,1);
$cur->inputBind("var3","testchar7");
$cur->inputBind("var4","testvarchar7");
$cur->inputBindClob("var5","testclob7",9);
$cur->inputBindBlob("var6","testblob7",9);
assertTrue($cur->executeQuery());
print("\n");


# array of input binds by name
# sqlite doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by name with validation
print("INPUT BIND BY NAME WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("var1",8);
$cur->inputBind("var2",8.8,4,1);
$cur->inputBind("var3","testchar8");
$cur->inputBind("var4","testvarchar8");
$cur->inputBindClob("var5","testclob8",9);
$cur->inputBindBlob("var6","testblob8",9);
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# select
print("SELECT: \n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEquals($cur->colCount(),6);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEquals($cur->getColumnName(0),"testint");
assertEquals($cur->getColumnName(1),"testfloat");
assertEquals($cur->getColumnName(2),"testchar");
assertEquals($cur->getColumnName(3),"testvarchar");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"testint");
assertEquals($cols[1],"testfloat");
assertEquals($cols[2],"testchar");
assertEquals($cols[3],"testvarchar");
print("\n");


# column types
print("COLUMN TYPES: \n");
if ($issqlite3) {
	assertEquals($cur->getColumnType(0),"INTEGER");
	assertEquals($cur->getColumnType("testint"),"INTEGER");
	assertEquals($cur->getColumnType(1),"FLOAT");
	assertEquals($cur->getColumnType("testfloat"),"FLOAT");
	assertEquals($cur->getColumnType(2),"STRING");
	assertEquals($cur->getColumnType("testchar"),"STRING");
	assertEquals($cur->getColumnType(3),"STRING");
	assertEquals($cur->getColumnType("testvarchar"),"STRING");
	assertEquals($cur->getColumnType(4),"STRING");
	assertEquals($cur->getColumnType("testclob"),"STRING");
	assertEquals($cur->getColumnType(5),"STRING");
	assertEquals($cur->getColumnType("testblob"),"STRING");
} else {
	assertEquals($cur->getColumnType(0),"UNKNOWN");
	assertEquals($cur->getColumnType("testint"),"UNKNOWN");
	assertEquals($cur->getColumnType(1),"UNKNOWN");
	assertEquals($cur->getColumnType("testfloat"),"UNKNOWN");
	assertEquals($cur->getColumnType(2),"UNKNOWN");
	assertEquals($cur->getColumnType("testchar"),"UNKNOWN");
	assertEquals($cur->getColumnType(3),"UNKNOWN");
	assertEquals($cur->getColumnType("testvarchar"),"UNKNOWN");
	assertEquals($cur->getColumnType(4),"UNKNOWN");
	assertEquals($cur->getColumnType("testclob"),"UNKNOWN");
	assertEquals($cur->getColumnType(5),"UNKNOWN");
	assertEquals($cur->getColumnType("testblob"),"UNKNOWN");
}
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEquals($cur->getColumnLength(0),0);
assertEquals($cur->getColumnLength("testint"),0);
assertEquals($cur->getColumnLength(1),0);
assertEquals($cur->getColumnLength("testfloat"),0);
assertEquals($cur->getColumnLength(2),0);
assertEquals($cur->getColumnLength("testchar"),0);
assertEquals($cur->getColumnLength(3),0);
assertEquals($cur->getColumnLength("testvarchar"),0);
assertEquals($cur->getColumnLength(4),0);
assertEquals($cur->getColumnLength("testclob"),0);
assertEquals($cur->getColumnLength(5),0);
assertEquals($cur->getColumnLength("testblob"),0);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEquals($cur->getLongest(0),1);
assertEquals($cur->getLongest("testint"),1);
assertEquals($cur->getLongest(1),3);
assertEquals($cur->getLongest("testfloat"),3);
assertEquals($cur->getLongest(2),9);
assertEquals($cur->getLongest("testchar"),9);
assertEquals($cur->getLongest(3),12);
assertEquals($cur->getLongest("testvarchar"),12);
assertEquals($cur->getLongest(4),9);
assertEquals($cur->getLongest("testclob"),9);
assertEquals($cur->getLongest(5),9);
assertEquals($cur->getLongest("testblob"),9);
print("\n");


# row count
print("ROW COUNT: \n");
assertEquals($cur->rowCount(),8);
print("\n");


# total rows
print("TOTAL ROWS: \n");
assertEquals($cur->totalRows(),($issqlite3)?0:8);
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
assertEquals($cur->getField(0,1),"1.1");
assertEquals($cur->getField(0,2),"testchar1");
assertEquals($cur->getField(0,3),"testvarchar1");
assertEquals($cur->getField(0,4),"testclob1");
assertEquals($cur->getField(0,5),"testblob1");
print("\n");
assertEquals($cur->getField(7,0),"8");
assertEquals($cur->getField(7,1),"8.8");
assertEquals($cur->getField(7,2),"testchar8");
assertEquals($cur->getField(7,3),"testvarchar8");
assertEquals($cur->getField(7,4),"testclob8");
assertEquals($cur->getField(7,5),"testblob8");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEquals($cur->getFieldLength(0,0),1);
assertEquals($cur->getFieldLength(0,1),3);
assertEquals($cur->getFieldLength(0,2),9);
assertEquals($cur->getFieldLength(0,3),12);
assertEquals($cur->getFieldLength(0,4),9);
assertEquals($cur->getFieldLength(0,5),9);
print("\n");
assertEquals($cur->getFieldLength(7,0),1);
assertEquals($cur->getFieldLength(7,1),3);
assertEquals($cur->getFieldLength(7,2),9);
assertEquals($cur->getFieldLength(7,3),12);
assertEquals($cur->getFieldLength(7,4),9);
assertEquals($cur->getFieldLength(7,5),9);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEquals($cur->getField(0,"testint"),"1");
assertEquals($cur->getField(0,"testfloat"),"1.1");
assertEquals($cur->getField(0,"testchar"),"testchar1");
assertEquals($cur->getField(0,"testvarchar"),"testvarchar1");
assertEquals($cur->getField(0,"testclob"),"testclob1");
assertEquals($cur->getField(0,"testblob"),"testblob1");
print("\n");
assertEquals($cur->getField(7,"testint"),"8");
assertEquals($cur->getField(7,"testfloat"),"8.8");
assertEquals($cur->getField(7,"testchar"),"testchar8");
assertEquals($cur->getField(7,"testvarchar"),"testvarchar8");
assertEquals($cur->getField(7,"testclob"),"testclob8");
assertEquals($cur->getField(7,"testblob"),"testblob8");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEquals($cur->getFieldLength(0,"testint"),1);
assertEquals($cur->getFieldLength(0,"testfloat"),3);
assertEquals($cur->getFieldLength(0,"testchar"),9);
assertEquals($cur->getFieldLength(0,"testvarchar"),12);
assertEquals($cur->getFieldLength(0,"testclob"),9);
assertEquals($cur->getFieldLength(0,"testblob"),9);
print("\n");
assertEquals($cur->getFieldLength(7,"testint"),1);
assertEquals($cur->getFieldLength(7,"testfloat"),3);
assertEquals($cur->getFieldLength(7,"testchar"),9);
assertEquals($cur->getFieldLength(7,"testvarchar"),12);
assertEquals($cur->getFieldLength(7,"testclob"),9);
assertEquals($cur->getFieldLength(7,"testblob"),9);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEquals($fields[0],"1");
assertEquals($fields[1],"1.1");
assertEquals($fields[2],"testchar1");
assertEquals($fields[3],"testvarchar1");
assertEquals($fields[4],"testclob1");
assertEquals($fields[5],"testblob1");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEquals($fieldlens[0],1);
assertEquals($fieldlens[1],3);
assertEquals($fieldlens[2],9);
assertEquals($fieldlens[3],12);
assertEquals($fieldlens[4],9);
assertEquals($fieldlens[5],9);
print("\n");


# fields by hash
print("FIELDS BY HASH: \n");
%fieldshash=$cur->getRowHash(0);
assertEquals($fieldshash{"testint"},"1");
assertEquals($fieldshash{"testfloat"},"1.1");
assertEquals($fieldshash{"testchar"},"testchar1");
assertEquals($fieldshash{"testvarchar"},"testvarchar1");
assertEquals($fieldshash{"testclob"},"testclob1");
assertEquals($fieldshash{"testblob"},"testblob1");
print("\n");
%fieldshash=$cur->getRowHash(7);
assertEquals($fieldshash{"testint"},"8");
assertEquals($fieldshash{"testfloat"},"8.8");
assertEquals($fieldshash{"testchar"},"testchar8");
assertEquals($fieldshash{"testvarchar"},"testvarchar8");
assertEquals($fieldshash{"testclob"},"testclob8");
assertEquals($fieldshash{"testblob"},"testblob8");
print("\n");


# field lengths by hash
print("FIELD LENGTHS BY HASH: \n");
%fieldlenshash=$cur->getRowLengthsHash(0);
assertEquals($fieldlenshash{"testint"},1);
assertEquals($fieldlenshash{"testfloat"},3);
assertEquals($fieldlenshash{"testchar"},9);
assertEquals($fieldlenshash{"testvarchar"},12);
assertEquals($fieldlenshash{"testclob"},9);
assertEquals($fieldlenshash{"testblob"},9);
print("\n");
%fieldlenshash=$cur->getRowLengthsHash(7);
assertEquals($fieldlenshash{"testint"},1);
assertEquals($fieldlenshash{"testfloat"},3);
assertEquals($fieldlenshash{"testchar"},9);
assertEquals($fieldlenshash{"testvarchar"},12);
assertEquals($fieldlenshash{"testclob"},9);
assertEquals($fieldlenshash{"testblob"},9);
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
assertEquals($cur->getColumnLength(0),0);
assertEquals($cur->getColumnType(0),($issqlite3)?"INTEGER":"UNKNOWN");
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
assertEquals($cur->colCount(),6);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEquals($cur->getColumnName(0),"testint");
assertEquals($cur->getColumnName(1),"testfloat");
assertEquals($cur->getColumnName(2),"testchar");
assertEquals($cur->getColumnName(3),"testvarchar");
assertEquals($cur->getColumnName(4),"testclob");
assertEquals($cur->getColumnName(5),"testblob");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"testint");
assertEquals($cols[1],"testfloat");
assertEquals($cols[2],"testchar");
assertEquals($cols[3],"testvarchar");
assertEquals($cols[4],"testclob");
assertEquals($cols[5],"testblob");
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
assertTrue($cur->sendQuery("select * from testtable"));
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
for ($i=0; ; $i++) {
	@row=$cur->getRow($i);
	last if (!@row);
	$secondcur=SQLRelay::Cursor->new($con);
	$secondcur->setResultSetBufferSize(1);
	assertTrue($secondcur->sendQuery("select * from testtable"));
	$secondcur->closeResultSet();
}
$cur->setResultSetBufferSize(0);
assertTrue($cur->sendQuery("drop table if exists testtable"));
print("\n");


# reset transaction state
print("RESET TRANSACTION STATE: \n");
assertTrue($con->commit());
assertEquals($con->getTransactionModel(),"explicit");
assertTrue($con->getAutoCommit());
print("\n");


# transaction behavior - implicit
print("TRANSACTION BEHAVIOR - implicit: \n");
assertTrue($con->setTransactionModel("implicit"));
assertEquals($con->getTransactionModel(),"implicit");
assertTrue($cur->sendQuery("create table testtable (col1 integer)"));
# sqlite DDL is transactional; commit so the table is visible
# to the second connection (the commit implicitly starts a new tx)
assertTrue($con->commit());
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
assertEquals($con->getTransactionModel(),"explicit");
assertTrue($con->getAutoCommit());
print("\n");


# individual substitutions
print("INDIVIDUAL SUBSTITUTIONS: \n");
$cur->sendQuery("drop table if exists testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int, ".
	"	col2 char, ".
	"	col3 float)"));
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	\$(var1), ".
	"	'\$(var2)', ".
	"	\$(var3))");
$cur->substitution("var1",1);
$cur->substitution("var2","hello");
$cur->substitution("var3",10.5556,6,4);
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("select * from testtable"));
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"hello");
assertEquals($cur->getField(0,2),"10.5556");
assertTrue($cur->sendQuery("delete from testtable"));
print("\n");


# array substitutions
print("ARRAY SUBSTITUTIONS: \n");
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	'\$(var1)', ".
	"	'\$(var2)', ".
	"	'\$(var3)')");
$cur->substitutions(\@subvars,\@subvalstrings);
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("select * from testtable"));
assertEquals($cur->getField(0,0),"hi");
assertEquals($cur->getField(0,1),"hello");
assertEquals($cur->getField(0,2),"bye");
assertTrue($cur->sendQuery("delete from testtable"));
print("\n");
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	\$(var1), ".
	"	'\$(var2)', ".
	"	\$(var3))");
$cur->substitutions(\@subvars,\@subvallongs);
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("select * from testtable"));
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"2");
assertEquals($cur->getField(0,2),"3.0");
assertTrue($cur->sendQuery("delete from testtable"));
print("\n");
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	\$(var1), ".
	"	'\$(var2)', ".
	"	\$(var3))");
$cur->substitutions(\@subvars,\@subvaldoubles,\@precs,\@scales);
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("select * from testtable"));
assertEquals($cur->getField(0,0),"10.55");
assertEquals($cur->getField(0,1),"10.556");
assertEquals($cur->getField(0,2),"10.5556");
assertTrue($cur->sendQuery("delete from testtable"));
print("\n");


# nulls as nulls
print("NULLS AS NULLS: \n");
$cur->getNullsAsUndefined();
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	1, ".
	"	NULL, ".
	"	NULL)"));
assertTrue($cur->sendQuery("select * from testtable"));
assertEquals($cur->getField(0,0),"1");
assertUndef($cur->getField(0,1));
assertUndef($cur->getField(0,2));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("select * from testtable"));
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"");
assertEquals($cur->getField(0,2),"");
assertTrue($cur->sendQuery("drop table if exists testtable"));
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
$cur->prepareQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	:var1, ".
	"	:var2, ".
	"	:var3, ".
	"	:var4)");
$cur->inputBindClob("var1","",0);
$cur->inputBindClob("var2",undef,0);
$cur->inputBindBlob("var3","",0);
$cur->inputBindBlob("var4",undef,0);
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
	"	testclob clob, ".
	"	testblob blob)");
$cur->prepareQuery("insert into testtable values (:clobval,:blobval)");
$largebuffer=('C' x $LARGE_BUFFER_LENGTH);
$cur->inputBindClob("clobval",$largebuffer,$LARGE_BUFFER_LENGTH);
$cur->inputBindBlob("blobval",$largebuffer,$LARGE_BUFFER_LENGTH);
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
# sqlite doesn't support output binds


# output bind by name
# sqlite doesn't support output binds


# output bind by name with validation
# sqlite doesn't support output binds


# lob output bind
# sqlite doesn't support output binds


# long output bind
# sqlite doesn't support output binds


# negative input bind
print("NEGATIVE INPUT BIND: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery("create table testtable (testval int)");
$cur->prepareQuery("insert into testtable values (:testval)");
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
$cur->substitution("var1",":var1");
assertTrue($cur->validBind("var1"));
assertFalse($cur->validBind("var2"));
assertFalse($cur->validBind("var3"));
assertFalse($cur->validBind("var4"));
print("\n");
$cur->substitution("var2",":var2");
assertTrue($cur->validBind("var1"));
assertTrue($cur->validBind("var2"));
assertFalse($cur->validBind("var3"));
assertFalse($cur->validBind("var4"));
print("\n");
$cur->substitution("var3",":var3");
assertTrue($cur->validBind("var1"));
assertTrue($cur->validBind("var2"));
assertTrue($cur->validBind("var3"));
assertFalse($cur->validBind("var4"));
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# rebinding
print("REBINDING: \n");
$cur->prepareQuery("select :val");
$cur->inputBind("val",1);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
$cur->inputBind("val",2);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"2");
$cur->inputBind("val",3);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"3");
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
$cur->prepareQuery("select :var");
$cur->inputBind("var",1);
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
$cur->inputBind("var",2);
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"2");
print("\n");


# stored procedure returning no value
# sqlite doesn't support stored procedures


# stored procedure returning single value
# sqlite doesn't support stored procedures


# stored procedure returning multiple values
# sqlite doesn't support stored procedures


# stored procedure returning result set
# sqlite doesn't support stored procedures


# temporary tables
print("TEMPORARY TABLES: \n");
$cur->sendQuery("drop table if exists temptable\n");
$cur->sendQuery("create temporary table temptable (col1 int)");
assertTrue($cur->sendQuery("insert into temptable values (1)"));
assertTrue($cur->sendQuery("select count(*) from temptable"));
assertEquals($cur->getField(0,0),"1");
$con->endSession();
print("\n");
assertFalse($cur->sendQuery("select count(*) from temptable"));
assertTrue($cur->sendQuery("drop table if exists temptable\n"));
print("\n");


# encoded binary data
print("ENCODED BINARY DATA: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 blob)"));
$buffer=pack("C*",(0..255));
$hex=unpack("H*",$buffer);
$querystr="insert into testtable values (X'$hex')";
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
assertEquals($cur->getField(0,0),"''");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# last insert id
print("LAST INSERT ID: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
		"create table testtable ".
		"	(col1 integer primary key ".
		"	autoincrement, ".
		"	col2 int)"));
assertTrue($cur->sendQuery(
		"insert into testtable values (null,1)"));
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
print("\n");


# schema list
print("SCHEMA LIST: \n");
assertTrue($cur->getSchemaList(undef));
assertEquals($cur->getColumnName(0),"Database");
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
$cur->sendQuery("drop table if exists testtable1");
$cur->sendQuery("drop table if exists testtable2");
$cur->sendQuery("drop table if exists testtable3");
$cur->sendQuery("drop table if exists testtable4");
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
$counter=0;
for ($i=0; $i<$cur->rowCount(); $i++) {
	$name=$cur->getField($i,"Tables_in_xxx");
	if (defined($name) &&
		($name eq "testtable1" ||
		$name eq "testtable2" ||
		$name eq "testtable3" ||
		$name eq "testtable4")) {
		$counter++;
	}
}
assertEquals($counter,4);
assertTrue($cur->sendQuery("drop table if exists testtable1"));
assertTrue($cur->sendQuery("drop table if exists testtable2"));
assertTrue($cur->sendQuery("drop table if exists testtable3"));
assertTrue($cur->sendQuery("drop table if exists testtable4"));
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
assertEquals($cur->getField(0,"precision"),"19");
assertEquals($cur->getField(0,"local_type_name"),"INTEGER");
assertTrue($cur->getTypeInfoList("char"));
assertEquals($cur->getField(0,"type_name"),"CHAR");
assertEquals($cur->getField(0,"data_type"),"1");
assertEquals($cur->getField(0,"precision"),"2147483647");
assertEquals($cur->getField(0,"local_type_name"),"CHAR");
assertTrue($cur->getTypeInfoList("varchar"));
assertEquals($cur->getField(0,"type_name"),"VARCHAR");
assertEquals($cur->getField(0,"data_type"),"12");
assertEquals($cur->getField(0,"precision"),"2147483647");
assertEquals($cur->getField(0,"local_type_name"),"VARCHAR");
assertTrue($cur->getTypeInfoList("date"));
assertEquals($cur->getField(0,"type_name"),"DATE");
assertEquals($cur->getField(0,"data_type"),"91");
assertEquals($cur->getField(0,"precision"),"10");
assertEquals($cur->getField(0,"local_type_name"),"DATE");
print("\n");


# column list
print("COLUMN LIST: \n");
$cur->sendQuery("drop table if exists testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testint int, ".
	"	testfloat float, ".
	"	testchar char(40), ".
	"	testvarchar varchar(40), ".
	"	testclob clob, ".
	"	testblob blob)"));
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
assertEquals($cur->getField(0,"column_name"),"testint");
assertEquals($cur->getField(1,"column_name"),"testfloat");
assertEquals($cur->getField(2,"column_name"),"testchar");
assertEquals($cur->getField(3,"column_name"),"testvarchar");
assertEquals($cur->getField(4,"column_name"),"testclob");
assertEquals($cur->getField(5,"column_name"),"testblob");
assertEquals($cur->getField(0,"data_type"),"INT");
assertEquals($cur->getField(1,"data_type"),"FLOAT");
assertEquals($cur->getField(2,"data_type"),"CHAR");
assertEquals($cur->getField(3,"data_type"),"VARCHAR");
assertEquals($cur->getField(4,"data_type"),"CLOB");
assertEquals($cur->getField(5,"data_type"),"BLOB");
assertTrue($cur->sendQuery("drop table if exists testtable"));
print("\n");


# column list - auto_increment, primary key
print("COLUMN LIST - auto_increment, primary key: \n");
$cur->sendQuery("drop table if exists testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 integer primary key autoincrement, ".
	"	col2 int)"));
assertTrue($cur->getColumnList("testtable",undef));
assertTrue(index($cur->getField(0,"extra"),"auto_increment")>=0);
assertTrue(index($cur->getField(0,"column_key"),"PRI")>=0);
assertFalse(index($cur->getField(1,"extra"),"auto_increment")>=0);
assertFalse(index($cur->getField(1,"column_key"),"PRI")>=0);
print("\n");
assertTrue($cur->sendQuery("drop table if exists testtable"));
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 int primary key, ".
	"	col2 int)"));
assertTrue($cur->getColumnList("testtable",undef));
assertFalse(index($cur->getField(0,"extra"),"auto_increment")>=0);
assertTrue(index($cur->getField(0,"column_key"),"PRI")>=0);
assertTrue($cur->sendQuery("drop table if exists testtable"));
print("\n");


# primary keys list
print("PRIMARY KEYS LIST: \n");
$cur->sendQuery("drop table if exists testtable");
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
assertTrue($cur->sendQuery("drop table if exists testtable"));
print("\n");


# key and index list
print("KEY AND INDEX LIST: \n");
$cur->sendQuery("drop table if exists testtable");
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
assertEquals($cur->getField(0,"non_unique"),"0");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertEquals($cur->getField(0,"column_name"),"col1");
assertEquals($cur->getField(0,"collation"),"A");
assertEquals($cur->getField(0,"index_type"),"3");
$keyname=$cur->getField(0,"key_name");
assertTrue(defined($keyname) && length($keyname)>0);
assertTrue($cur->sendQuery("drop table if exists testtable"));
print("\n");


# procedure list
print("PROCEDURE LIST: \n");
assertTrue($cur->getProcedureList(undef));
assertEquals($cur->rowCount(),0);
print("\n");


# procedure parameter list
print("PROCEDURE PARAMETER LIST: \n");
assertTrue($cur->getProcedureParameterList("testproc1",undef));
assertEquals($cur->getColumnName(0),"parameter_name");
assertEquals($cur->getColumnName(1),"parameter_mode");
assertEquals($cur->getColumnName(2),"data_type");
assertEquals($cur->getColumnName(3),"character_maximum_length");
assertEquals($cur->getColumnName(4),"ordinal_position");
assertEquals($cur->rowCount(),0);
print("\n");


# invalid queries
print("INVALID QUERIES: \n");
assertFalse($cur->sendQuery("select * from testtable"));
assertFalse($cur->sendQuery("select * from testtable"));
assertFalse($cur->sendQuery("select * from testtable"));
assertFalse($cur->sendQuery("select * from testtable"));
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
