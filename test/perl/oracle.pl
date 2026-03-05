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
assertEqualString($con->identify(),"oracle");
print("\n");


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");


# isolation levels
print("ISOLATION LEVELS: \n");
@isolationlevels=("READ COMMITTED","SERIALIZABLE");
foreach $il (@isolationlevels) {
	# oracle requires the isolation level to
	# be the first query of the transaction
	assertTrue($con->commit());
	# you can set the isolation level, but to get it, you have to
	# have permissions to read from sys.v_$session and
	# sys.v_$transaction
	assertTrue($con->setIsolationLevel($il));
	print("\n");
}
# reset to the default isolation level
assertTrue($con->commit());
assertTrue($con->setIsolationLevel($isolationlevels[0]));
print("\n");

# drop existing table
$cur->sendQuery("drop table testtable");


# create temptable
print("CREATE TEMPTABLE: \n");
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
assertEqual($cur->affectedRows(),1);
print("\n");


# bind by position
print("BIND BY POSITION: \n");
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
assertEqual($cur->countBindVariables(),7);
$cur->inputBind("1",2);
$cur->inputBind("2","testchar2");
$cur->inputBind("3","testvarchar2");
$cur->inputBind("4","01-JAN-2002");
$cur->inputBind("5","testlong2");
$cur->inputBindClob("6","testclob2",9);
$cur->inputBindBlob("7","testblob2",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",3);
$cur->inputBind("2","testchar3");
$cur->inputBind("3","testvarchar3");
$cur->inputBind("4","01-JAN-2003");
$cur->inputBind("5","testlong3");
$cur->inputBindClob("6","testclob3",9);
$cur->inputBindBlob("7","testblob3",9);
assertTrue($cur->executeQuery());
print("\n");


# array of binds by position
print("ARRAY OF BINDS BY POSITION: \n");
$cur->clearBinds();
@vars=("1","2","3","4","5");
@vals=(4,"testchar4","testvarchar4","01-JAN-2004","testlong4");
$cur->inputBinds(\@vars,\@vals);
$cur->inputBindClob("6","testclob4",9);
$cur->inputBindBlob("7","testblob4",9);
assertTrue($cur->executeQuery());
print("\n");


# bind by name
print("BIND BY NAME: \n");
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
$cur->inputBind("var1",5);
$cur->inputBind("var2","testchar5");
$cur->inputBind("var3","testvarchar5");
$cur->inputBind("var4","01-JAN-2005");
$cur->inputBind("var5","testlong5");
$cur->inputBindClob("var6","testclob5",9);
$cur->inputBindBlob("var7","testblob5",9);
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("var1",6);
$cur->inputBind("var2","testchar6");
$cur->inputBind("var3","testvarchar6");
$cur->inputBind("var4","01-JAN-2006");
$cur->inputBind("var5","testlong6");
$cur->inputBindClob("var6","testclob6",9);
$cur->inputBindBlob("var7","testblob6",9);
assertTrue($cur->executeQuery());
print("\n");


# array of binds by name
print("ARRAY OF BINDS BY NAME: \n");
$cur->clearBinds();
@vars=("var1","var2","var3","var4","var5");
@vals=(7,"testchar7","testvarchar7","01-JAN-2007","testlong7");
$cur->inputBinds(\@vars,\@vals);
$cur->inputBindClob("6","testclob7",9);
$cur->inputBindBlob("7","testblob7",9);
assertTrue($cur->executeQuery());
print("\n");


# bind by name with validation
print("BIND BY NAME WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("var1",8);
$cur->inputBind("var2","testchar8");
$cur->inputBind("var3","testvarchar8");
$cur->inputBind("var4","01-JAN-2008");
$cur->inputBind("var5","testlong8");
$cur->inputBindClob("var6","testclob8",9);
$cur->inputBindBlob("var7","testblob8",9);
$cur->inputBind("var9","junkvalue");
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# select
print("SELECT: \n");
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEqual($cur->colCount(),7);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEqualString($cur->getColumnName(0),"TESTNUMBER");
assertEqualString($cur->getColumnName(1),"TESTCHAR");
assertEqualString($cur->getColumnName(2),"TESTVARCHAR");
assertEqualString($cur->getColumnName(3),"TESTDATE");
assertEqualString($cur->getColumnName(4),"TESTLONG");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"TESTNUMBER");
assertEqualString($cols[1],"TESTCHAR");
assertEqualString($cols[2],"TESTVARCHAR");
assertEqualString($cols[3],"TESTDATE");
assertEqualString($cols[4],"TESTLONG");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEqualString($cur->getColumnType(0),"NUMBER");
assertEqualString($cur->getColumnType('TESTNUMBER'),"NUMBER");
assertEqualString($cur->getColumnType(1),"CHAR");
assertEqualString($cur->getColumnType('TESTCHAR'),"CHAR");
assertEqualString($cur->getColumnType(2),"VARCHAR2");
assertEqualString($cur->getColumnType('TESTVARCHAR'),"VARCHAR2");
assertEqualString($cur->getColumnType(3),"DATE");
assertEqualString($cur->getColumnType('TESTDATE'),"DATE");
assertEqualString($cur->getColumnType(4),"LONG");
assertEqualString($cur->getColumnType('TESTLONG'),"LONG");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEqual($cur->getColumnLength(0),22);
assertEqual($cur->getColumnLength('TESTNUMBER'),22);
assertEqual($cur->getColumnLength(1),40);
assertEqual($cur->getColumnLength('TESTCHAR'),40);
assertEqual($cur->getColumnLength(2),40);
assertEqual($cur->getColumnLength('TESTVARCHAR'),40);
assertEqual($cur->getColumnLength(3),7);
assertEqual($cur->getColumnLength('TESTDATE'),7);
assertEqual($cur->getColumnLength(4),0);
assertEqual($cur->getColumnLength('TESTLONG'),0);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEqual($cur->getLongest(0),1);
assertEqual($cur->getLongest('TESTNUMBER'),1);
assertEqual($cur->getLongest(1),40);
assertEqual($cur->getLongest('TESTCHAR'),40);
assertEqual($cur->getLongest(2),12);
assertEqual($cur->getLongest('TESTVARCHAR'),12);
assertEqual($cur->getLongest(3),9);
assertEqual($cur->getLongest('TESTDATE'),9);
assertEqual($cur->getLongest(4),9);
assertEqual($cur->getLongest('TESTLONG'),9);
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
assertEqualString($cur->getField(0,1),"testchar1                               ");
assertEqualString($cur->getField(0,2),"testvarchar1");
assertEqualString($cur->getField(0,3),"01-JAN-01");
assertEqualString($cur->getField(0,4),"testlong1");
assertEqualString($cur->getField(0,5),"testclob1");
assertEqualString($cur->getField(0,6),"");
print("\n");
assertEqualString($cur->getField(7,0),"8");
assertEqualString($cur->getField(7,1),"testchar8                               ");
assertEqualString($cur->getField(7,2),"testvarchar8");
assertEqualString($cur->getField(7,3),"01-JAN-08");
assertEqualString($cur->getField(7,4),"testlong8");
assertEqualString($cur->getField(7,5),"testclob8");
assertEqualString($cur->getField(7,6),"testblob8");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEqual($cur->getFieldLength(0,0),1);
assertEqual($cur->getFieldLength(0,1),40);
assertEqual($cur->getFieldLength(0,2),12);
assertEqual($cur->getFieldLength(0,3),9);
assertEqual($cur->getFieldLength(0,4),9);
assertEqual($cur->getFieldLength(0,5),9);
assertEqual($cur->getFieldLength(0,6),0);
print("\n");
assertEqual($cur->getFieldLength(7,0),1);
assertEqual($cur->getFieldLength(7,1),40);
assertEqual($cur->getFieldLength(7,2),12);
assertEqual($cur->getFieldLength(7,3),9);
assertEqual($cur->getFieldLength(7,4),9);
assertEqual($cur->getFieldLength(7,5),9);
assertEqual($cur->getFieldLength(7,6),9);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEqualString($cur->getField(0,"TESTNUMBER"),"1");
assertEqualString($cur->getField(0,"TESTCHAR"),"testchar1                               ");
assertEqualString($cur->getField(0,"TESTVARCHAR"),"testvarchar1");
assertEqualString($cur->getField(0,"TESTDATE"),"01-JAN-01");
assertEqualString($cur->getField(0,"TESTLONG"),"testlong1");
assertEqualString($cur->getField(0,"TESTCLOB"),"testclob1");
assertEqualString($cur->getField(0,"TESTBLOB"),"");
print("\n");
assertEqualString($cur->getField(7,"TESTNUMBER"),"8");
assertEqualString($cur->getField(7,"TESTCHAR"),"testchar8                               ");
assertEqualString($cur->getField(7,"TESTVARCHAR"),"testvarchar8");
assertEqualString($cur->getField(7,"TESTDATE"),"01-JAN-08");
assertEqualString($cur->getField(7,"TESTLONG"),"testlong8");
assertEqualString($cur->getField(7,"TESTCLOB"),"testclob8");
assertEqualString($cur->getField(7,"TESTBLOB"),"testblob8");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEqual($cur->getFieldLength(0,"TESTNUMBER"),1);
assertEqual($cur->getFieldLength(0,"TESTCHAR"),40);
assertEqual($cur->getFieldLength(0,"TESTVARCHAR"),12);
assertEqual($cur->getFieldLength(0,"TESTDATE"),9);
assertEqual($cur->getFieldLength(0,"TESTLONG"),9);
assertEqual($cur->getFieldLength(0,"TESTCLOB"),9);
assertEqual($cur->getFieldLength(0,"TESTBLOB"),0);
print("\n");
assertEqual($cur->getFieldLength(7,"TESTNUMBER"),1);
assertEqual($cur->getFieldLength(7,"TESTCHAR"),40);
assertEqual($cur->getFieldLength(7,"TESTVARCHAR"),12);
assertEqual($cur->getFieldLength(7,"TESTDATE"),9);
assertEqual($cur->getFieldLength(7,"TESTLONG"),9);
assertEqual($cur->getFieldLength(7,"TESTCLOB"),9);
assertEqual($cur->getFieldLength(7,"TESTBLOB"),9);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEqual($fields[0],1);
assertEqualString($fields[1],"testchar1                               ");
assertEqualString($fields[2],"testvarchar1");
assertEqualString($fields[3],"01-JAN-01");
assertEqualString($fields[4],"testlong1");
assertEqualString($fields[5],"testclob1");
assertEqualString($fields[6],"");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEqual($fieldlens[0],1);
assertEqual($fieldlens[1],40);
assertEqual($fieldlens[2],12);
assertEqual($fieldlens[3],9);
assertEqual($fieldlens[4],9);
assertEqual($fieldlens[5],9);
assertEqual($fieldlens[6],0);
print("\n");


# fields by hash
print("FIELDS BY HASH: \n");
%fields=$cur->getRowHash(0);
assertEqual($fields{"TESTNUMBER"},1);
assertEqualString($fields{"TESTCHAR"},"testchar1                               ");
assertEqualString($fields{"TESTVARCHAR"},"testvarchar1");
assertEqualString($fields{"TESTDATE"},"01-JAN-01");
assertEqualString($fields{"TESTLONG"},"testlong1");
assertEqualString($fields{"TESTCLOB"},"testclob1");
assertEqualString($fields{"TESTBLOB"},"");
print("\n");
%fields=$cur->getRowHash(7);
assertEqual($fields{"TESTNUMBER"},8);
assertEqualString($fields{"TESTCHAR"},"testchar8                               ");
assertEqualString($fields{"TESTVARCHAR"},"testvarchar8");
assertEqualString($fields{"TESTDATE"},"01-JAN-08");
assertEqualString($fields{"TESTLONG"},"testlong8");
assertEqualString($fields{"TESTCLOB"},"testclob8");
assertEqualString($fields{"TESTBLOB"},"testblob8");
print("\n");


# field lengths by hash
print("FIELD LENGTHS BY HASH: \n");
%fieldlengths=$cur->getRowLengthsHash(0);
assertEqual($fieldlengths{"TESTNUMBER"},1);
assertEqual($fieldlengths{"TESTCHAR"},40);
assertEqual($fieldlengths{"TESTVARCHAR"},12);
assertEqual($fieldlengths{"TESTDATE"},9);
assertEqual($fieldlengths{"TESTLONG"},9);
assertEqual($fieldlengths{"TESTCLOB"},9);
assertEqual($fieldlengths{"TESTBLOB"},0);
print("\n");
%fieldlengths=$cur->getRowLengthsHash(7);
assertEqual($fieldlengths{"TESTNUMBER"},1);
assertEqual($fieldlengths{"TESTCHAR"},40);
assertEqual($fieldlengths{"TESTVARCHAR"},12);
assertEqual($fieldlengths{"TESTDATE"},9);
assertEqual($fieldlengths{"TESTLONG"},9);
assertEqual($fieldlengths{"TESTCLOB"},9);
assertEqual($fieldlengths{"TESTBLOB"},9);
print("\n");


# individual substitutions
print("INDIVIDUAL SUBSTITUTIONS: \n");
$cur->prepareQuery("select \$(var1),'\$(var2)',\$(var3) from dual");
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
$cur->prepareQuery("select \$(var1),'\$(var2)',\$(var3) from dual");
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
assertTrue($cur->sendQuery("select NULL,1,NULL from dual"));
assertUndef($cur->getField(0,0));
assertEqualString($cur->getField(0,1),"1");
assertUndef($cur->getField(0,2));
$cur->getNullsAsEmptyStrings();
assertTrue($cur->sendQuery("select NULL,1,NULL from dual"));
assertEqualString($cur->getField(0,0),"");
assertEqualString($cur->getField(0,1),"1");
assertEqualString($cur->getField(0,2),"");
print("\n");


# result set buffer size
print("RESULT SET BUFFER SIZE: \n");
assertEqual($cur->getResultSetBufferSize(),0);
$cur->setResultSetBufferSize(2);
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
assertUndef($cur->getColumnName(0));
assertEqual($cur->getColumnLength(0),0);
assertUndef($cur->getColumnType(0));
$cur->getColumnInfo();
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
assertEqualString($cur->getColumnName(0),"TESTNUMBER");
assertEqual($cur->getColumnLength(0),22);
assertEqualString($cur->getColumnType(0),"NUMBER");
print("\n");


# suspended session
print("SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
$filename=$cur->getCacheFileName();
assertEqualString($filename,"cachefile1");
$cur->cacheOff();
assertTrue($cur->openCachedResultSet($filename));
assertEqualString($cur->getField(7,0),"8");
print("\n");


# column count for cached result set
print("COLUMN COUNT FOR CACHED RESULT SET: \n");
assertEqual($cur->colCount(),7);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEqualString($cur->getColumnName(0),"TESTNUMBER");
assertEqualString($cur->getColumnName(1),"TESTCHAR");
assertEqualString($cur->getColumnName(2),"TESTVARCHAR");
assertEqualString($cur->getColumnName(3),"TESTDATE");
assertEqualString($cur->getColumnName(4),"TESTLONG");
assertEqualString($cur->getColumnName(5),"TESTCLOB");
assertEqualString($cur->getColumnName(6),"TESTBLOB");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"TESTNUMBER");
assertEqualString($cols[1],"TESTCHAR");
assertEqualString($cols[2],"TESTVARCHAR");
assertEqualString($cols[3],"TESTDATE");
assertEqualString($cols[4],"TESTLONG");
assertEqualString($cols[5],"TESTCLOB");
assertEqualString($cols[6],"TESTBLOB");
print("\n");


# cached result set with result set buffer size
print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
$cur->setResultSetBufferSize(2);
$cur->cacheToFile("cachefile1");
$cur->setCacheTtl(200);
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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
$secondcon=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$secondcur=SQLRelay::Cursor->new($secondcon);
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"0");
assertTrue($con->commit());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"8");
assertTrue($con->autoCommitOn());
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	10, ".
	"	'testchar10', ".
	"	'testvarchar10', ".
	"	'01-JAN-2010', ".
	"	'testlong10', ".
	"	'testclob10', ".
	"	empty_blob())"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"9");
assertTrue($con->autoCommitOff());
print("\n");


# output bind by position
print("OUTPUT BIND BY POSITION: \n");
$cur->prepareQuery(
	"begin ".
	"	:numvar:=1; ".
	"	:stringvar:='hello'; ".
	"	:floatvar:=2.5; ".
	"	:nullvar:=null; ".
	"end;");
$cur->defineOutputBindInteger("1");
$cur->defineOutputBindString("2",10);
$cur->defineOutputBindDouble("3",);
$cur->defineOutputBindString("4",10);
assertTrue($cur->executeQuery());
$numvar=$cur->getOutputBindInteger("1");
$stringvar=$cur->getOutputBindString("2");
$floatvar=$cur->getOutputBindDouble("3");
assertEqual($numvar,1);
assertEqualString($stringvar,'hello');
assertEqual($floatvar,2.5);
assertEqual($cur->getOutputBindString("4"),"");
print("\n");


# output bind by name
print("OUTPUT BIND BY NAME: \n");
$cur->clearBinds();
$cur->defineOutputBindInteger("numvar");
$cur->defineOutputBindString("stringvar",10);
$cur->defineOutputBindDouble("floatvar");
$cur->defineOutputBindString("nullvar",10);
assertTrue($cur->executeQuery());
$numvar=$cur->getOutputBindInteger("numvar");
$stringvar=$cur->getOutputBindString("stringvar");
$floatvar=$cur->getOutputBindDouble("floatvar");
assertEqual($numvar,1);
assertEqualString($stringvar,'hello');
assertEqual($floatvar,2.5);
assertEqual(
	$cur->getOutputBindString("nullvar"),"");
print("\n");


# output bind by name with validation
print("OUTPUT BIND BY NAME WITH VALIDATION: \n");
$cur->clearBinds();
$cur->defineOutputBindInteger("numvar");
$cur->defineOutputBindString("stringvar",10);
$cur->defineOutputBindDouble("floatvar");
$cur->defineOutputBindString("nullvar",10);
$cur->defineOutputBindString("dummyvar",10);
$cur->validateBinds();
assertTrue($cur->executeQuery());
$numvar=$cur->getOutputBindInteger("numvar");
$stringvar=$cur->getOutputBindString("stringvar");
$floatvar=$cur->getOutputBindDouble("floatvar");
assertEqual($numvar,1);
assertEqualString($stringvar,'hello');
assertEqual($floatvar,2.5);
assertEqual(
	$cur->getOutputBindString("nullvar"),"");
print("\n");


# clob and blob output bind
print("CLOB AND BLOB OUTPUT BIND: \n");
$cur->sendQuery("drop table testtable1");
assertTrue($cur->sendQuery(
	"create table testtable1 (".
	"	testclob clob, ".
	"	testblob blob)"));
$cur->prepareQuery("insert into testtable1 values ('hello',:var1)");
$cur->inputBindBlob("var1","hello",5);
assertTrue($cur->executeQuery());
$cur->prepareQuery(
	"begin ".
	"	select testclob into :clobvar from testtable1; ".
	"	select testblob into :blobvar from testtable1; ".
	"end;");
$cur->defineOutputBindClob("clobvar");
$cur->defineOutputBindBlob("blobvar");
assertTrue($cur->executeQuery());
$clobvar=$cur->getOutputBindClob("clobvar");
$clobvarlength=$cur->getOutputBindLength("clobvar");
$blobvar=$cur->getOutputBindBlob("blobvar");
$blobvarlength=$cur->getOutputBindLength("blobvar");
assertEqual($clobvar,"hello",5);
assertEqual($clobvarlength,5);
assertEqual($blobvar,"hello",5);
assertEqual($blobvarlength,5);
$cur->sendQuery("drop table testtable1");
print("\n");


# null and empty clobs and clobs
print("NULL AND EMPTY CLOBS AND CLOBS: \n");
$cur->getNullsAsUndefined();
$cur->sendQuery(
	"create table testtable1 (".
	"	testclob1 clob, ".
	"	testclob2 clob, ".
	"	testblob1 blob, ".
	"	testblob2 blob)");
$cur->prepareQuery("insert into testtable1 values (:var1,:var2,:var3,:var4)");
$cur->inputBindClob("var1","",0);
$cur->inputBindClob("var2",NULL,0);
$cur->inputBindBlob("var3","",0);
$cur->inputBindBlob("var4",NULL,0);
assertTrue($cur->executeQuery());
$cur->sendQuery("select * from testtable1");
assertEqual($cur->getField(0,0),NULL);
assertEqual($cur->getField(0,1),NULL);
assertEqual($cur->getField(0,2),NULL);
assertEqual($cur->getField(0,3),NULL);
$cur->sendQuery("drop table testtable1");
print("\n");


# cursor binds
print("CURSOR BINDS: \n");
assertTrue($cur->sendQuery(
	"create or replace package types as ".
	"	type cursorType is ref cursor; ".
	"end;"));
assertTrue($cur->sendQuery(
	"create or replace ".
	"function sp_testtable ".
	"return types.cursortype ".
	"as ".
	"	l_cursor    types.cursorType; ".
	"begin ".
	"	open l_cursor for ".
	"		select * from testtable; ".
	"	return l_cursor; ".
	"end;"));
$cur->prepareQuery("begin  :curs:=sp_testtable; end;");
$cur->defineOutputBindCursor("curs");
assertTrue($cur->executeQuery());
$bindcur=$cur->getOutputBindCursor("curs");
assertTrue($bindcur->fetchFromBindCursor());
assertEqual($bindcur->getField(0,0),"1");
assertEqual($bindcur->getField(1,0),"2");
assertEqual($bindcur->getField(2,0),"3");
assertEqual($bindcur->getField(3,0),"4");
assertEqual($bindcur->getField(4,0),"5");
assertEqual($bindcur->getField(5,0),"6");
assertEqual($bindcur->getField(6,0),"7");
assertEqual($bindcur->getField(7,0),"8");
print("\n");


# long clob
print("LONG CLOB: \n");
$cur->sendQuery("drop table testtable2");
$cur->sendQuery("create table testtable2 (testclob clob)");
$cur->prepareQuery("insert into testtable2 values (:clobval)");
$clobval="";
for ($i=0; $i<8*1024; $i++) {
	$clobval=$clobval.'C';
}
$cur->inputBindClob("clobval",$clobval,8*1024);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testclob from testtable2");
assertEqual($clobval,$cur->getField(0,"testclob"));
$cur->prepareQuery(
	"begin select testclob into :clobbindval from testtable2; ".
	"	end;");
$cur->defineOutputBindClob("clobbindval");
assertTrue($cur->executeQuery());
$clobbindvar=$cur->getOutputBindClob("clobbindval");
assertEqual($cur->getOutputBindLength("clobbindval"),8*1024);
assertEqual($clobval,$clobbindvar);
$cur->sendQuery("delete from testtable2");
print("\n");
$cur->prepareQuery("insert into testtable2 values (:clobval)");
$clobval="";
for ($i=0; $i<8*1024; $i++) {
	$clobval=$clobval.'C';
}
$cur->inputBindClob("clobval",$clobval,8*1024);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testclob from testtable2");
assertEqual($clobval,$cur->getField(0,"testclob"));
$cur->prepareQuery(
	"begin select testclob into :clobbindval from testtable2; ".
	"	end;");
$cur->defineOutputBindClob("clobbindval");
assertTrue($cur->executeQuery());
$clobbindvar=$cur->getOutputBindClob("clobbindval");
assertEqual($cur->getOutputBindLength("clobbindval"),8*1024);
assertEqual($clobval,$clobbindvar);
$cur->sendQuery("drop table testtable2");
print("\n");

print("LONG OUTPUT BIND\n");
$cur->sendQuery("drop table testtable2");
$cur->sendQuery("create table testtable2 (testval varchar2(4000))");
$testval="";
$cur->prepareQuery("insert into testtable2 values (:testval)");
for ($i=0; $i<4000; $i++) {
	$testval=$testval.'C';
}
$cur->inputBind("testval",$testval);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testval from testtable2");
assertEqual($testval,$cur->getField(0,"TESTVAL"));
$query="begin :bindval:='".$testval."'; end;";
$cur->prepareQuery($query);
$cur->defineOutputBindString("bindval",4000);
assertTrue($cur->executeQuery());
$bindval=$cur->getOutputBindString("bindval");
assertEqual($cur->getOutputBindLength("bindval"),4000);
assertEqual($bindval,$testval);
$cur->sendQuery("drop table testtable2");
print("\n");

print("NEGATIVE INPUT BIND\n");
$cur->sendQuery("create table testtable2 (testval number)");
$cur->prepareQuery("insert into testtable2 values (:testval)");
$cur->inputBind("testval",-1);
assertTrue($cur->executeQuery());
$cur->sendQuery("select testval from testtable2");
assertEqual($cur->getField(0,"TESTVAL"),"-1");
$cur->sendQuery("drop table testtable2");
print("\n");


# finished suspended session
print("FINISHED SUSPENDED SESSION: \n");
assertTrue($cur->sendQuery("select * from testtable order by testnumber"));
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


# bind validation
print("BIND VALIDATION: \n");
$cur->sendQuery("drop table testtable1");
$cur->sendQuery(
	"create table testtable1 (".
	"	col1 varchar2(20), ".
	"	col2 varchar2(20), ".
	"	col3 varchar2(20))");
$cur->prepareQuery(
	"insert into ".
	"	testtable1 ".
	"values (".
	"	\$(var1), ".
	"	\$(var2), ".
	"	\$(var3))");
$cur->inputBind("var1",1);
$cur->inputBind("var2",2);
$cur->inputBind("var3",3);
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
$cur->sendQuery("drop table testtable1");
print("\n");

# drop existing table
$cur->sendQuery("drop table testtable");


# invalid queries
print("INVALID QUERIES: \n");
assertFalse($cur->sendQuery("select * from testtable order by testnumber"));
assertFalse($cur->sendQuery("select * from testtable order by testnumber"));
assertFalse($cur->sendQuery("select * from testtable order by testnumber"));
assertFalse($cur->sendQuery("select * from testtable order by testnumber"));
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
