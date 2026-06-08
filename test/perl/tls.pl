#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.


use SQLRelay::Connection;
use SQLRelay::Cursor;
use Sys::Hostname;
require "./asserts.pl";


@isolationlevels=("READ COMMITTED","SERIALIZABLE");
@bindvars=("1","2","3","4","5");
@bindvals=("4","testchar4","testvarchar4","01-JAN-2004","testlong4");
@arraybindvars=("var1","var2","var3","var4","var5");
@arraybindvals=("7","testchar7","testvarchar7","01-JAN-2007","testlong7");
@subvars=("var1","var2","var3");
@subvallongs=(1,2,3);
@subvalstrings=("hi","hello","bye");
@subvaldoubles=(10.55,10.556,10.5556);
@precs=(4,5,6);
@scales=(2,3,4);

$LARGE_BUFFER_LENGTH=8192;

$cert="../sqlrelay.conf.d/tls/client.pem";
$ca="../sqlrelay.conf.d/tls/ca.pem";
if ($^O eq "MSWin32") {
	$cert="..\\sqlrelay.conf.d\\tls\\client.pfx";
	$ca="..\\sqlrelay.conf.d\\tls\\ca.pfx";
}


# hostname
$hostname=hostname();
$hostname=~s/\..*//;


# instantiation
$con=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						undef,undef,0,1);
$cur=SQLRelay::Cursor->new($con);
$con->enableTls(undef,$cert,undef,undef,"ca",$ca,0);


# identify
print("IDENTIFY: \n");
assertEquals($con->identify(),"oracle");
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
assertEquals($con->bindFormat(),":*");
print("\n");


# nextval format
print("NEXTVAL FORMAT: \n");
assertEquals($con->nextvalFormat(),"%s.nextval");
print("\n");


# isolation levels
print("ISOLATION LEVELS: \n");
foreach $il (@isolationlevels) {
	# oracle requires the isolation level to
	# be the first query of the transaction
	assertTrue($con->commit());
	# you can set the isolation level, but to get it, you have to
	# have permisisons to read from sys.v_$session and
	# sys.v_$transaction
	assertTrue($con->setIsolationLevel($il));
	print("\n");
}
# reset to the default isolation level
assertTrue($con->commit());
assertTrue($con->setIsolationLevel($isolationlevels[0]));
print("\n");


# create testtable
print("CREATE TESTTABLE: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testnumber number, ".
	"	testchar char(40), ".
	"	testvarchar varchar2(40), ".
	"	testdate date, ".
	"	testlong long, ".
	"	testclob clob, ".
	"	testblob blob)"));
print("\n");


# insert
print("INSERT: \n");
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	1, ".
	"	'testchar1', ".
	"	'testvarchar1', ".
	"	'01-JAN-2001', ".
	"	'testlong1', ".
	"	'testclob1', ".
	"	empty_blob())"));
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
	"	:var1, ".
	"	:var2, ".
	"	:var3, ".
	"	:var4, ".
	"	:var5, ".
	"	:var6, ".
	"	:var7)");
assertEquals($cur->countBindVariables(),7);
$cur->inputBind("1",2);
$cur->inputBind("2","testchar2");
$cur->inputBind("3","testvarchar2");
$cur->inputBindDate("4",2002,1,1,0,0,0,0,undef,0);
$cur->inputBind("5","testlong2");
$cur->inputBindClob("6","testclob2",9);
$cur->inputBindBlob("7","testblob2",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",3);
$cur->inputBind("2","testchar3");
$cur->inputBind("3","testvarchar3");
$cur->inputBindDate("4",2003,1,1,0,0,0,0,undef,0);
$cur->inputBind("5","testlong3");
$cur->inputBindClob("6","testclob3",9);
$cur->inputBindBlob("7","testblob3",9);
assertTrue($cur->executeQuery());
print("\n");


# array of input binds by position
print("ARRAY OF INPUT BINDS BY POSITION: \n");
$cur->clearBinds();
$cur->inputBinds(\@bindvars,\@bindvals);
$cur->inputBindClob("6","testclob4",9);
$cur->inputBindBlob("7","testblob4",9);
assertTrue($cur->executeQuery());
print("\n");


# input bind by position with validation
print("INPUT BIND BY POSITION WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("1",5);
$cur->inputBind("2","testchar5");
$cur->inputBind("3","testvarchar5");
$cur->inputBindDate("4",2005,1,1,0,0,0,0,undef,0);
$cur->inputBind("5","testlong5");
$cur->inputBindClob("6","testclob5",9);
$cur->inputBindBlob("7","testblob5",9);
$cur->validateBinds();
assertTrue($cur->executeQuery());
$cur->clearBinds();


# input bind by name
print("INPUT BIND BY NAME: \n");
$cur->clearBinds();
$cur->inputBind("var1",6);
$cur->inputBind("var2","testchar6");
$cur->inputBind("var3","testvarchar6");
$cur->inputBindDate("var4",2006,1,1,0,0,0,0,undef,0);
$cur->inputBind("var5","testlong6");
$cur->inputBindClob("var6","testclob6",9);
$cur->inputBindBlob("var7","testblob6",9);
assertTrue($cur->executeQuery());
print("\n");


# array of input binds by name
print("ARRAY OF INPUT BINDS BY NAME: \n");
$cur->clearBinds();
$cur->inputBinds(\@arraybindvars,\@arraybindvals);
$cur->inputBindClob("var6","testclob7",9);
$cur->inputBindBlob("var7","testblob7",9);
assertTrue($cur->executeQuery());
print("\n");


# input bind by name with validation
print("INPUT BIND BY NAME WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("var1",8);
$cur->inputBind("var2","testchar8");
$cur->inputBind("var3","testvarchar8");
$cur->inputBindDate("var4",2008,1,1,0,0,0,0,undef,0);
$cur->inputBind("var5","testlong8");
$cur->inputBindClob("var6","testclob8",9);
$cur->inputBindBlob("var7","testblob8",9);
$cur->inputBind("var9","junkvalue");
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# select
print("SELECT: \n");
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testnumber"));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEquals($cur->colCount(),7);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEquals($cur->getColumnName(0),"TESTNUMBER");
assertEquals($cur->getColumnName(1),"TESTCHAR");
assertEquals($cur->getColumnName(2),"TESTVARCHAR");
assertEquals($cur->getColumnName(3),"TESTDATE");
assertEquals($cur->getColumnName(4),"TESTLONG");
assertEquals($cur->getColumnName(5),"TESTCLOB");
assertEquals($cur->getColumnName(6),"TESTBLOB");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"TESTNUMBER");
assertEquals($cols[1],"TESTCHAR");
assertEquals($cols[2],"TESTVARCHAR");
assertEquals($cols[3],"TESTDATE");
assertEquals($cols[4],"TESTLONG");
assertEquals($cols[5],"TESTCLOB");
assertEquals($cols[6],"TESTBLOB");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEquals($cur->getColumnType(0),"NUMBER");
assertEquals($cur->getColumnType("TESTNUMBER"),"NUMBER");
assertEquals($cur->getColumnType(1),"CHAR");
assertEquals($cur->getColumnType("TESTCHAR"),"CHAR");
assertEquals($cur->getColumnType(2),"VARCHAR2");
assertEquals($cur->getColumnType("TESTVARCHAR"),"VARCHAR2");
assertEquals($cur->getColumnType(3),"DATE");
assertEquals($cur->getColumnType("TESTDATE"),"DATE");
assertEquals($cur->getColumnType(4),"LONG");
assertEquals($cur->getColumnType("TESTLONG"),"LONG");
assertEquals($cur->getColumnType(5),"CLOB");
assertEquals($cur->getColumnType("TESTCLOB"),"CLOB");
assertEquals($cur->getColumnType(6),"BLOB");
assertEquals($cur->getColumnType("TESTBLOB"),"BLOB");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEquals($cur->getColumnLength(0),22);
assertEquals($cur->getColumnLength("TESTNUMBER"),22);
assertEquals($cur->getColumnLength(1),40);
assertEquals($cur->getColumnLength("TESTCHAR"),40);
assertEquals($cur->getColumnLength(2),40);
assertEquals($cur->getColumnLength("TESTVARCHAR"),40);
assertEquals($cur->getColumnLength(3),7);
assertEquals($cur->getColumnLength("TESTDATE"),7);
assertEquals($cur->getColumnLength(4),0);
assertEquals($cur->getColumnLength("TESTLONG"),0);
assertEquals($cur->getColumnLength(5),0);
assertEquals($cur->getColumnLength("TESTCLOB"),0);
assertEquals($cur->getColumnLength(6),0);
assertEquals($cur->getColumnLength("TESTBLOB"),0);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEquals($cur->getLongest(0),1);
assertEquals($cur->getLongest("TESTNUMBER"),1);
assertEquals($cur->getLongest(1),40);
assertEquals($cur->getLongest("TESTCHAR"),40);
assertEquals($cur->getLongest(2),12);
assertEquals($cur->getLongest("TESTVARCHAR"),12);
assertEquals($cur->getLongest(3),9);
assertEquals($cur->getLongest("TESTDATE"),9);
assertEquals($cur->getLongest(4),9);
assertEquals($cur->getLongest("TESTLONG"),9);
assertEquals($cur->getLongest(5),9);
assertEquals($cur->getLongest("TESTCLOB"),9);
assertEquals($cur->getLongest(6),9);
assertEquals($cur->getLongest("TESTBLOB"),9);
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
assertEquals($cur->getField(0,1),"testchar1                               ");
assertEquals($cur->getField(0,2),"testvarchar1");
assertEquals($cur->getField(0,3),"01-JAN-01");
assertEquals($cur->getField(0,4),"testlong1");
assertEquals($cur->getField(0,5),"testclob1");
assertEquals($cur->getField(0,6),"");
print("\n");
assertEquals($cur->getField(7,0),"8");
assertEquals($cur->getField(7,1),"testchar8                               ");
assertEquals($cur->getField(7,2),"testvarchar8");
assertEquals($cur->getField(7,3),"01-JAN-08");
assertEquals($cur->getField(7,4),"testlong8");
assertEquals($cur->getField(7,5),"testclob8");
assertEquals($cur->getField(7,6),"testblob8");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEquals($cur->getFieldLength(0,0),1);
assertEquals($cur->getFieldLength(0,1),40);
assertEquals($cur->getFieldLength(0,2),12);
assertEquals($cur->getFieldLength(0,3),9);
assertEquals($cur->getFieldLength(0,4),9);
assertEquals($cur->getFieldLength(0,5),9);
assertEquals($cur->getFieldLength(0,6),0);
print("\n");
assertEquals($cur->getFieldLength(7,0),1);
assertEquals($cur->getFieldLength(7,1),40);
assertEquals($cur->getFieldLength(7,2),12);
assertEquals($cur->getFieldLength(7,3),9);
assertEquals($cur->getFieldLength(7,4),9);
assertEquals($cur->getFieldLength(7,5),9);
assertEquals($cur->getFieldLength(7,6),9);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEquals($cur->getField(0,"TESTNUMBER"),"1");
assertEquals($cur->getField(0,"TESTCHAR"),"testchar1                               ");
assertEquals($cur->getField(0,"TESTVARCHAR"),"testvarchar1");
assertEquals($cur->getField(0,"TESTDATE"),"01-JAN-01");
assertEquals($cur->getField(0,"TESTLONG"),"testlong1");
assertEquals($cur->getField(0,"TESTCLOB"),"testclob1");
assertEquals($cur->getField(0,"TESTBLOB"),"");
print("\n");
assertEquals($cur->getField(7,"TESTNUMBER"),"8");
assertEquals($cur->getField(7,"TESTCHAR"),"testchar8                               ");
assertEquals($cur->getField(7,"TESTVARCHAR"),"testvarchar8");
assertEquals($cur->getField(7,"TESTDATE"),"01-JAN-08");
assertEquals($cur->getField(7,"TESTLONG"),"testlong8");
assertEquals($cur->getField(7,"TESTCLOB"),"testclob8");
assertEquals($cur->getField(7,"TESTBLOB"),"testblob8");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEquals($cur->getFieldLength(0,"TESTNUMBER"),1);
assertEquals($cur->getFieldLength(0,"TESTCHAR"),40);
assertEquals($cur->getFieldLength(0,"TESTVARCHAR"),12);
assertEquals($cur->getFieldLength(0,"TESTDATE"),9);
assertEquals($cur->getFieldLength(0,"TESTLONG"),9);
assertEquals($cur->getFieldLength(0,"TESTCLOB"),9);
assertEquals($cur->getFieldLength(0,"TESTBLOB"),0);
print("\n");
assertEquals($cur->getFieldLength(7,"TESTNUMBER"),1);
assertEquals($cur->getFieldLength(7,"TESTCHAR"),40);
assertEquals($cur->getFieldLength(7,"TESTVARCHAR"),12);
assertEquals($cur->getFieldLength(7,"TESTDATE"),9);
assertEquals($cur->getFieldLength(7,"TESTLONG"),9);
assertEquals($cur->getFieldLength(7,"TESTCLOB"),9);
assertEquals($cur->getFieldLength(7,"TESTBLOB"),9);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEquals($fields[0],"1");
assertEquals($fields[1],"testchar1                               ");
assertEquals($fields[2],"testvarchar1");
assertEquals($fields[3],"01-JAN-01");
assertEquals($fields[4],"testlong1");
assertEquals($fields[5],"testclob1");
assertEquals($fields[6],"");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEquals($fieldlens[0],1);
assertEquals($fieldlens[1],40);
assertEquals($fieldlens[2],12);
assertEquals($fieldlens[3],9);
assertEquals($fieldlens[4],9);
assertEquals($fieldlens[5],9);
assertEquals($fieldlens[6],0);
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
	"	testnumber"));
assertEquals($cur->getResultSetBufferSize(),2);
print("\n");
assertEquals($cur->firstRowIndex(),0);
assertFalse($cur->endOfResultSet());
assertEquals($cur->rowCount(),2);
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(1,0),"2");
assertEquals($cur->getField(2,0),"3");
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
	"	testnumber"));
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
	"	testnumber"));
assertEquals($cur->getColumnName(0),"TESTNUMBER");
assertEquals($cur->getColumnLength(0),22);
assertEquals($cur->getColumnType(0),"NUMBER");
print("\n");


# suspended session
print("SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testnumber"));
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
	"	testnumber"));
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
	"	testnumber"));
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
	"	testnumber"));
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
	"	testnumber"));
$filename=$cur->getCacheFileName();
assertEquals($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEquals($cur->getField(7,0),"8");
print("\n");


# column count for cached result set
print("COLUMN COUNT FOR CACHED RESULT SET: \n");
assertEquals($cur->colCount(),7);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEquals($cur->getColumnName(0),"TESTNUMBER");
assertEquals($cur->getColumnName(1),"TESTCHAR");
assertEquals($cur->getColumnName(2),"TESTVARCHAR");
assertEquals($cur->getColumnName(3),"TESTDATE");
assertEquals($cur->getColumnName(4),"TESTLONG");
assertEquals($cur->getColumnName(5),"TESTCLOB");
assertEquals($cur->getColumnName(6),"TESTBLOB");
@cols=$cur->getColumnNames();
assertEquals($cols[0],"TESTNUMBER");
assertEquals($cols[1],"TESTCHAR");
assertEquals($cols[2],"TESTVARCHAR");
assertEquals($cols[3],"TESTDATE");
assertEquals($cols[4],"TESTLONG");
assertEquals($cols[5],"TESTCLOB");
assertEquals($cols[6],"TESTBLOB");
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
	"	testnumber"));
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
	"	testnumber"));
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
	"	testnumber"));
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
$secondcon=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						undef,undef,0,1);
$secondcur=SQLRelay::Cursor->new($secondcon);
$secondcon->enableTls(undef,$cert,undef,undef,"ca",$ca,0);
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
$cur->prepareQuery("select \$(var1),'\$(var2)',\$(var3) from dual");
$cur->substitution("var1","\$(var11)");
$cur->substitution("var2","\$(var21)");
$cur->substitution("var3","\$(var31)");
$cur->substitution("var11","\$(var111)");
$cur->substitution("var21","\$(var211)");
$cur->substitution("var31","\$(var311)");
$cur->substitution("var111",1);
$cur->substitution("var211","hello");
$cur->substitution("var311",10.5556,6,4);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"hello");
assertEquals($cur->getField(0,2),"10.5556");
print("\n");


# array substitutions
print("ARRAY SUBSTITUTIONS: \n");
$cur->prepareQuery("select \$(var1),\$(var2),\$(var3) from dual");
$cur->substitutions(\@subvars,\@subvallongs);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertEquals($cur->getField(0,1),"2");
assertEquals($cur->getField(0,2),"3");
print("\n");
$cur->prepareQuery("select '\$(var1)','\$(var2)','\$(var3)' from dual");
$cur->substitutions(\@subvars,\@subvalstrings);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"hi");
assertEquals($cur->getField(0,1),"hello");
assertEquals($cur->getField(0,2),"bye");
print("\n");
$cur->prepareQuery("select \$(var1),\$(var2),\$(var3) from dual");
$cur->substitutions(\@subvars,\@subvaldoubles,\@precs,\@scales);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"10.55");
assertEquals($cur->getField(0,1),"10.556");
assertEquals($cur->getField(0,2),"10.5556");
print("\n");


# nulls as nulls
print("NULLS AS NULLS: \n");
$cur->getNullsAsUndefined();
assertTrue($cur->sendQuery("select NULL,1,NULL from dual"));
assertUndef($cur->getField(0,0));
assertEquals($cur->getField(0,1),"1");
assertUndef($cur->getField(0,2));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("select NULL,1,NULL from dual"));
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
assertEquals($cur->getFieldLength(0,"TESTCLOB"),$LARGE_BUFFER_LENGTH);
assertEquals($cur->getField(0,"TESTCLOB"),$largebuffer);
assertEquals($cur->getFieldLength(0,"TESTBLOB"),$LARGE_BUFFER_LENGTH);
assertEqualsBytes($cur->getField(0,"TESTBLOB"),$largebuffer,
					$LARGE_BUFFER_LENGTH);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# output bind by position
print("OUTPUT BIND BY POSITION: \n");
$cur->getNullsAsUndefined();
$cur->prepareQuery(
	"begin ".
	"	:numvar:=1; ".
	"	:stringvar:='hello'; ".
	"	:floatvar:=2.5; ".
	"	:datevar:='03-FEB-2001'; ".
	"	:nullvar:=null; ".
	"end;");
assertEquals($cur->countBindVariables(),5);
$cur->defineOutputBindInteger("1");
$cur->defineOutputBindString("2",10);
$cur->defineOutputBindDouble("3");
$cur->defineOutputBindDate("4");
$cur->defineOutputBindString("5",10);
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
$isnegative=$cur->getOutputBindDateIsNegative("4");
$nullvar=$cur->getOutputBindString("5");
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
assertFalse($isnegative);
assertUndef($nullvar);
$cur->getNullsAsEmptyStrings();
print("\n");


# output bind by name
print("OUTPUT BIND BY NAME: \n");
$cur->getNullsAsUndefined();
$cur->clearBinds();
$cur->defineOutputBindInteger("numvar");
$cur->defineOutputBindString("stringvar",10);
$cur->defineOutputBindDouble("floatvar");
$cur->defineOutputBindDate("datevar");
$cur->defineOutputBindString("nullvar",10);
assertTrue($cur->executeQuery());
$numvar=$cur->getOutputBindInteger("numvar");
$stringvar=$cur->getOutputBindString("stringvar");
$floatvar=$cur->getOutputBindDouble("floatvar");
$year=$cur->getOutputBindDateYear("datevar");
$month=$cur->getOutputBindDateMonth("datevar");
$day=$cur->getOutputBindDateDay("datevar");
$hour=$cur->getOutputBindDateHour("datevar");
$minute=$cur->getOutputBindDateMinute("datevar");
$second=$cur->getOutputBindDateSecond("datevar");
$microsecond=$cur->getOutputBindDateMicrosecond("datevar");
$tz=$cur->getOutputBindDateTz("datevar");
$isnegative=$cur->getOutputBindDateIsNegative("datevar");
$nullvar=$cur->getOutputBindString("nullvar");
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
assertFalse($isnegative);
assertUndef($nullvar);
$cur->getNullsAsEmptyStrings();
print("\n");


# output bind by name with validation
print("OUTPUT BIND BY NAME WITH VALIDATION: \n");
$cur->getNullsAsUndefined();
$cur->clearBinds();
$cur->defineOutputBindInteger("numvar");
$cur->defineOutputBindString("stringvar",10);
$cur->defineOutputBindDouble("floatvar");
$cur->defineOutputBindDate("datevar");
$cur->defineOutputBindString("nullvar",10);
$cur->defineOutputBindString("dummyvar",10);
$cur->validateBinds();
assertTrue($cur->executeQuery());
$numvar=$cur->getOutputBindInteger("numvar");
$stringvar=$cur->getOutputBindString("stringvar");
$floatvar=$cur->getOutputBindDouble("floatvar");
$year=$cur->getOutputBindDateYear("datevar");
$month=$cur->getOutputBindDateMonth("datevar");
$day=$cur->getOutputBindDateDay("datevar");
$hour=$cur->getOutputBindDateHour("datevar");
$minute=$cur->getOutputBindDateMinute("datevar");
$second=$cur->getOutputBindDateSecond("datevar");
$microsecond=$cur->getOutputBindDateMicrosecond("datevar");
$tz=$cur->getOutputBindDateTz("datevar");
$isnegative=$cur->getOutputBindDateIsNegative("datevar");
$nullvar=$cur->getOutputBindString("nullvar");
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
assertFalse($isnegative);
assertUndef($nullvar);
$cur->getNullsAsEmptyStrings();
print("\n");


# lob output bind
print("LOB OUTPUT BIND: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testclob clob, ".
	"	testblob blob)"));
$cur->prepareQuery("insert into testtable values ('hello',:var1)");
$cur->inputBindBlob("var1","hello",5);
assertTrue($cur->executeQuery());
$cur->prepareQuery(
	"begin ".
	"	select testclob into :clobvar from testtable; ".
	"	select testblob into :blobvar from testtable; ".
	"end;");
$cur->defineOutputBindClob("clobvar");
$cur->defineOutputBindBlob("blobvar");
assertTrue($cur->executeQuery());
$clobvar=$cur->getOutputBindClob("clobvar");
$clobvarlength=$cur->getOutputBindLength("clobvar");
$blobvar=$cur->getOutputBindBlob("blobvar");
$blobvarlength=$cur->getOutputBindLength("blobvar");
assertEqualsBytes($clobvar,"hello",5);
assertEquals($clobvarlength,5);
assertEqualsBytes($blobvar,"hello",5);
assertEquals($blobvarlength,5);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# long output bind
print("LONG OUTPUT BIND: \n");
$largebuffer=('C' x $LARGE_BUFFER_LENGTH);
$query="begin :bindval:='$largebuffer'; end;";
$cur->prepareQuery($query);
$cur->defineOutputBindString("bindval",$LARGE_BUFFER_LENGTH);
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindLength("bindval"),$LARGE_BUFFER_LENGTH);
assertEquals($cur->getOutputBindString("bindval"),$largebuffer);
print("\n");


# negative input bind
print("NEGATIVE INPUT BIND: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery("create table testtable (testval number)");
$cur->prepareQuery("insert into testtable values (:testval)");
$cur->inputBind("testval",-1);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testval from testtable");
assertEquals($cur->getField(0,"TESTVAL"),"-1");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# bind validation
print("BIND VALIDATION: \n");
$cur->sendQuery("drop table testtable");
$cur->sendQuery(
	"create table testtable (".
	"	col1 varchar2(20), ".
	"	col2 varchar2(20), ".
	"	col3 varchar2(20))");
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
$cur->prepareQuery(
	"begin ".
	"	:out:= :in; ".
	"end;");
$cur->inputBind("in",1);
$cur->defineOutputBindInteger("out");
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("out"),1);
$cur->inputBind("in",2);
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("out"),2);
$cur->inputBind("in",3);
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("out"),3);
print("\n");


# reexecute
print("REEXECUTE: \n");
$cur->prepareQuery("select 1 from dual");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
assertTrue($cur->executeQuery());
assertEquals($cur->rowCount(),1);
assertEquals($cur->getField(0,0),"1");
print("\n");
$cur->prepareQuery("select :var from dual");
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
print("STORED PROCEDURE RETURNING NO VALUE: \n");
$cur->sendQuery("drop function testproc");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create or replace ".
	"procedure testproc(".
	"	in1 in number, ".
	"	in2 in number, ".
	"	in3 in varchar2) ".
	"is ".
	"begin ".
	"	return; ".
	"end;"));
$cur->prepareQuery("begin testproc(:in1,:in2,:in3); end;");
$cur->inputBind("in1",1);
$cur->inputBind("in2",2.5,2,1);
$cur->inputBind("in3","hello");
assertTrue($cur->executeQuery());
assertTrue($cur->sendQuery("drop procedure testproc"));
print("\n");


# stored procedure returning single value
print("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
$cur->sendQuery("drop function testproc");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create or replace ".
	"function testproc(".
	"	in1 in number, ".
	"	in2 in number, ".
	"	in3 in varchar2) ".
	"	return number ".
	"is ".
	"begin ".
	"	return in1; ".
	"end;"));
$cur->prepareQuery("select testproc(:in1,:in2,:in3) from dual");
$cur->inputBind("in1",1);
$cur->inputBind("in2",2.5,2,1);
$cur->inputBind("in3","hello");
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
$cur->prepareQuery(
	"begin ".
	"	:out1:=testproc(:in1,:in2,:in3); ".
	"end;");
$cur->inputBind("in1",1);
$cur->inputBind("in2",2.5,2,1);
$cur->inputBind("in3","hello");
$cur->defineOutputBindInteger("out1");
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("out1"),1);
assertTrue($cur->sendQuery("drop function testproc"));
print("\n");


# stored procedure returning multiple values
print("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
$cur->sendQuery("drop function testproc");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create or replace ".
	"procedure testproc(".
	"	in1 in number, ".
	"	in2 in number, ".
	"	in3 in varchar2, ".
	"	out1 out number, ".
	"	out2 out number, ".
	"	out3 out varchar2) ".
	"is ".
	"begin ".
	"	out1:=in1; ".
	"	out2:=in2; ".
	"	out3:=in3; ".
	"end;"));
$cur->prepareQuery(
	"begin ".
	"	testproc(:in1,:in2,:in3,:out1,:out2,:out3); ".
	"end;");
$cur->inputBind("in1",1);
$cur->inputBind("in2",2.5,2,1);
$cur->inputBind("in3","hello");
$cur->defineOutputBindInteger("out1");
$cur->defineOutputBindDouble("out2");
$cur->defineOutputBindString("out3",20);
assertTrue($cur->executeQuery());
assertEquals($cur->getOutputBindInteger("out1"),1);
assertEquals($cur->getOutputBindDouble("out2"),2.5);
assertEquals($cur->getOutputBindString("out3"),"hello");
assertTrue($cur->sendQuery("drop procedure testproc"));
print("\n");


# stored procedure returning result set
print("STORED PROCEDURE RETURNING RESULT SET: \n");
$cur->sendQuery("drop package types");
$cur->sendQuery("drop function testproc");
$cur->sendQuery("drop procedure testproc");
assertTrue($cur->sendQuery(
	"create or replace package types is ".
	"	type cursorType is ref cursor; ".
	"end;"));
assertTrue($cur->sendQuery(
	"create or replace ".
	"function testproc(value in number) ".
	"	return types.cursortype ".
	"is ".
	"	l_cursor    types.cursorType; ".
	"begin ".
	"	open l_cursor for ".
	"		select ".
	"			* ".
	"		from ".
	"			( ".
	"			select 1 as testnumber from dual ".
	"			union ".
	"			select 2 as testnumber from dual ".
	"			union ".
	"			select 3 as testnumber from dual ".
	"			union ".
	"			select 4 as testnumber from dual ".
	"			union ".
	"			select 5 as testnumber from dual ".
	"			union ".
	"			select 6 as testnumber from dual ".
	"			union ".
	"			select 7 as testnumber from dual ".
	"			union ".
	"			select 8 as testnumber from dual ".
	"			) ".
	"		where ".
	"			testnumber>value; ".
	"	return l_cursor; ".
	"end;"));
$cur->prepareQuery(
	"begin ".
	"	:curs1:=testproc(5); ".
	"	:curs2:=testproc(0); ".
	"end;");
$cur->defineOutputBindCursor("curs1");
$cur->defineOutputBindCursor("curs2");
assertTrue($cur->executeQuery());
$bindcur1=$cur->getOutputBindCursor("curs1");
assertTrue($bindcur1->fetchFromBindCursor());
assertEquals($bindcur1->getField(0,0),"6");
assertEquals($bindcur1->getField(1,0),"7");
assertEquals($bindcur1->getField(2,0),"8");
$bindcur1=undef;
$bindcur2=$cur->getOutputBindCursor("curs2");
assertTrue($bindcur2->fetchFromBindCursor());
assertEquals($bindcur2->getField(0,0),"1");
assertEquals($bindcur2->getField(1,0),"2");
assertEquals($bindcur2->getField(2,0),"3");
$bindcur2=undef;
assertTrue($cur->sendQuery("drop function testproc"));
assertTrue($cur->sendQuery("drop package types"));
print("\n");


# temporary tables
print("TEMPORARY TABLES: \n");
$cur->prepareQuery("drop table \$(HOSTNAME)_temptabledelete");
$cur->substitution("HOSTNAME",$hostname);
$cur->executeQuery();
$cur->prepareQuery(
	"create global temporary table \$(HOSTNAME)_temptabledelete ( ".
	"	col1 number ".
	") on commit delete rows");
$cur->substitution("HOSTNAME",$hostname);
$cur->executeQuery();
$cur->prepareQuery("insert into \$(HOSTNAME)_temptabledelete values (1)");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
$cur->prepareQuery("select count(*) from \$(HOSTNAME)_temptabledelete");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertTrue($con->commit());
$cur->prepareQuery("select count(*) from \$(HOSTNAME)_temptabledelete");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"0");
$cur->prepareQuery("drop table \$(HOSTNAME)_temptabledelete");
$cur->substitution("HOSTNAME",$hostname);
$cur->executeQuery();
print("\n");
$cur->prepareQuery("truncate table \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
$cur->executeQuery();
$cur->prepareQuery("drop table \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
$cur->executeQuery();
$cur->prepareQuery(
	"create global temporary table \$(HOSTNAME)_temptablepreserve (".
	"	col1 number ".
	") on commit preserve rows");
$cur->substitution("HOSTNAME",$hostname);
$cur->executeQuery();
$cur->prepareQuery(
	"insert into ".
	"	\$(HOSTNAME)_temptablepreserve ".
	"values (1)");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
$cur->prepareQuery("select count(*) from \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
assertTrue($con->commit());
$cur->prepareQuery("select count(*) from \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"1");
$con->endSession();
print("\n");
$cur->prepareQuery("select count(*) from \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
assertEquals($cur->getField(0,0),"0");
$cur->prepareQuery("truncate table \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
sleep(2);
$cur->prepareQuery("drop table \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
assertTrue($cur->executeQuery());
$cur->prepareQuery("select count(*) from \$(HOSTNAME)_temptablepreserve");
$cur->substitution("HOSTNAME",$hostname);
assertFalse($cur->executeQuery());
print("\n");


# encoded binary data
print("ENCODED BINARY DATA: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 blob)"));
$buffer=pack("C*",(0..255));
$hex=unpack("H*",$buffer);
$querystr="insert into testtable values ('$hex')";
assertTrue($cur->sendQuery($querystr));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),256);
assertEqualsBytes($cur->getField(0,0),$buffer,256);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# quotes
print("QUOTES: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery("create table testtable (col1 varchar2(4))"));
assertTrue($cur->sendQuery("insert into testtable values ('''''')"));
assertTrue($cur->sendQuery("select col1 from testtable"));
assertEquals($cur->getFieldLength(0,0),2);
assertEquals($cur->getField(0,0),"''");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# last insert id
# oracle doesn't support auto-increment


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
assertInResultSet($cur,"Database",uc($hostname));
print("\n");


# table type list
print("TABLE TYPE LIST: \n");
assertTrue($cur->getTableTypeList());
assertEquals($cur->getColumnName(0),"table_type");
assertEquals($cur->getField(0,"table_type"),"SYNONYM");
assertEquals($cur->getField(1,"table_type"),"TABLE");
assertEquals($cur->getField(2,"table_type"),"VIEW");
print("\n");


# table list
print("TABLE LIST: \n");
$cur->sendQuery("drop table testtable1");
$cur->sendQuery("drop table testtable2");
$cur->sendQuery("drop table testtable3");
$cur->sendQuery("drop table testtable4");
assertTrue($cur->sendQuery(
	"create table testtable1 (".
	"	testnumber number, ".
	"	testchar char(40), ".
	"	testvarchar varchar2(40), ".
	"	testdate date, ".
	"	testlong long, ".
	"	testclob clob, ".
	"	testblob blob)"));
assertTrue($cur->sendQuery(
	"create table testtable2 (".
	"	testnumber number, ".
	"	testchar char(40), ".
	"	testvarchar varchar2(40), ".
	"	testdate date, ".
	"	testlong long, ".
	"	testclob clob, ".
	"	testblob blob)"));
assertTrue($cur->sendQuery(
	"create table testtable3 (".
	"	testnumber number, ".
	"	testchar char(40), ".
	"	testvarchar varchar2(40), ".
	"	testdate date, ".
	"	testlong long, ".
	"	testclob clob, ".
	"	testblob blob)"));
assertTrue($cur->sendQuery(
	"create table testtable4 (".
	"	testnumber number, ".
	"	testchar char(40), ".
	"	testvarchar varchar2(40), ".
	"	testdate date, ".
	"	testlong long, ".
	"	testclob clob, ".
	"	testblob blob)"));
assertTrue($cur->getTableList(undef));
assertInResultSet($cur,"Tables_in_xxx","TESTTABLE1");
assertInResultSet($cur,"Tables_in_xxx","TESTTABLE2");
assertInResultSet($cur,"Tables_in_xxx","TESTTABLE3");
assertInResultSet($cur,"Tables_in_xxx","TESTTABLE4");
assertTrue($cur->sendQuery("drop table testtable1"));
assertTrue($cur->sendQuery("drop table testtable2"));
assertTrue($cur->sendQuery("drop table testtable3"));
assertTrue($cur->sendQuery("drop table testtable4"));
print("\n");


# type info list
print("TYPE INFO LIST: \n");
assertTrue($cur->getTypeInfoList("number"));
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
assertEquals($cur->getField(0,"type_name"),"NUMBER");
assertEquals($cur->getField(0,"data_type"),"-7");
assertEquals($cur->getField(0,"precision"),"1");
assertEquals($cur->getField(0,"local_type_name"),"NUMBER");
assertTrue($cur->getTypeInfoList("char"));
assertEquals($cur->getField(0,"type_name"),"CHAR");
assertEquals($cur->getField(0,"data_type"),"1");
assertEquals($cur->getField(0,"precision"),"2000");
assertEquals($cur->getField(0,"local_type_name"),"CHAR");
assertTrue($cur->getTypeInfoList("varchar2"));
assertEquals($cur->getField(0,"type_name"),"VARCHAR2");
assertEquals($cur->getField(0,"data_type"),"12");
assertEquals($cur->getField(0,"precision"),"32767");
assertEquals($cur->getField(0,"local_type_name"),"VARCHAR2");
assertTrue($cur->getTypeInfoList("date"));
assertEquals($cur->getField(0,"type_name"),"DATE");
assertEquals($cur->getField(0,"data_type"),"92");
assertEquals($cur->getField(0,"precision"),"7");
assertEquals($cur->getField(0,"local_type_name"),"DATE");
print("\n");


# column list
print("COLUMN LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testnumber number, ".
	"	testchar char(40), ".
	"	testvarchar varchar2(40), ".
	"	testdate date, ".
	"	testlong long, ".
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
assertEquals($cur->getField(0,"column_name"),"TESTNUMBER");
assertEquals($cur->getField(1,"column_name"),"TESTCHAR");
assertEquals($cur->getField(2,"column_name"),"TESTVARCHAR");
assertEquals($cur->getField(3,"column_name"),"TESTDATE");
assertEquals($cur->getField(4,"column_name"),"TESTLONG");
assertEquals($cur->getField(5,"column_name"),"TESTCLOB");
assertEquals($cur->getField(6,"column_name"),"TESTBLOB");
assertEquals($cur->getField(0,"data_type"),"NUMBER");
assertEquals($cur->getField(1,"data_type"),"CHAR");
assertEquals($cur->getField(2,"data_type"),"VARCHAR2");
assertEquals($cur->getField(3,"data_type"),"DATE");
assertEquals($cur->getField(4,"data_type"),"LONG");
assertEquals($cur->getField(5,"data_type"),"CLOB");
assertEquals($cur->getField(6,"data_type"),"BLOB");
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# column list - auto_increment, primary key
# oracle doesn't support auto_increment
print("COLUMN LIST - auto_increment, primary key: \n");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 number primary key, ".
	"	col2 number)"));
assertTrue($cur->getColumnList("testtable",undef));
assertTrue(index($cur->getField(0,"column_key"),"PRI")>=0);
assertFalse(index($cur->getField(1,"column_key"),"PRI")>=0);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# primary keys list
print("PRIMARY KEYS LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 number primary key, ".
	"	col2 number)"));
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
assertEquals($cur->getField(0,"table"),"TESTTABLE");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertEquals($cur->getField(0,"column_name"),"COL1");
$keyname=$cur->getField(0,"key_name");
assertTrue(defined($keyname) && length($keyname)>0);
assertTrue($cur->sendQuery("drop table testtable"));
print("\n");


# key and index list
print("KEY AND INDEX LIST: \n");
$cur->sendQuery("drop table testtable");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	col1 number primary key, ".
	"	col2 number)"));
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
assertEquals($cur->getField(0,"table"),"TESTTABLE");
assertEquals($cur->getField(0,"non_unique"),"0");
assertEquals($cur->getField(0,"seq_in_index"),"1");
assertEquals($cur->getField(0,"column_name"),"COL1");
assertEquals($cur->getField(0,"collation"),"A");
assertEquals($cur->getField(0,"index_type"),"3");
$keyname=$cur->getField(0,"key_name");
assertTrue(defined($keyname) && length($keyname)>0);
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
	"	in1 in number, ".
	"	in2 in char, ".
	"	in3 in varchar2, ".
	"	in4 in date) as ".
	"begin ".
	"	null; ".
	"end;"));
assertTrue($cur->sendQuery(
	"create procedure testproc2(".
	"	in1 in number, ".
	"	in2 in char, ".
	"	in3 in varchar2, ".
	"	in4 in date) as ".
	"begin ".
	"	null; ".
	"end;"));
assertTrue($cur->sendQuery(
	"create procedure testproc3(".
	"	in1 in number, ".
	"	in2 in char, ".
	"	in3 in varchar2, ".
	"	in4 in date) as ".
	"begin ".
	"	null; ".
	"end;"));
assertTrue($cur->sendQuery(
	"create procedure testproc4(".
	"	in1 in number, ".
	"	in2 in char, ".
	"	in3 in varchar2, ".
	"	in4 in date) as ".
	"begin ".
	"	null; ".
	"end;"));
assertTrue($cur->getProcedureList(undef));
assertInResultSet($cur,"routine_name","TESTPROC1");
assertInResultSet($cur,"routine_name","TESTPROC2");
assertInResultSet($cur,"routine_name","TESTPROC3");
assertInResultSet($cur,"routine_name","TESTPROC4");
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
assertEquals($cur->getField(0,"data_type"),"NUMBER");
assertEquals($cur->getField(0,"ordinal_position"),"1");
assertEquals($cur->getField(1,"parameter_name"),"IN2");
assertEquals($cur->getField(1,"parameter_mode"),"1");
assertEquals($cur->getField(1,"data_type"),"CHAR");
assertEquals($cur->getField(1,"ordinal_position"),"2");
assertEquals($cur->getField(2,"parameter_name"),"IN3");
assertEquals($cur->getField(2,"parameter_mode"),"1");
assertEquals($cur->getField(2,"data_type"),"VARCHAR2");
assertEquals($cur->getField(2,"ordinal_position"),"3");
assertEquals($cur->getField(3,"parameter_name"),"IN4");
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
	"	testnumber"));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testnumber"));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testnumber"));
assertFalse($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testtable ".
	"order by ".
	"	testnumber"));
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
