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


# identify
print("IDENTIFY: \n");
assertEqualString($con->identify(),"postgresql");
print("\n");


# ping
print("PING: \n");
assertTrue($con->ping());
print("\n");

# isolation levels
#print("ISOLATION LEVELS: \n");
#@isolationlevels=("read committed","read uncommitted","repeatable read","serializable");
#foreach $il (@isolationlevels) {
#	# postgresql requires the isolation level to
#	# be the first query of the transaction
#	$con->begin();
#	assertTrue($con->setIsolationLevel($il));
#	assertEqualString($con->getIsolationLevel(),$il);
#	$con->commit();
#	print("\n");
#}
## reset to the default isolation level
#$con->begin();
#assertTrue($con->setIsolationLevel($isolationlevels[0]));
#$con->commit();
#print("\n");

# drop existing table
$cur->sendQuery("drop table testtable");


# create temptable
print("CREATE TEMPTABLE: \n");
assertTrue($cur->sendQuery(
	"create table testtable (".
	"	testint int, ".
	"	testfloat float, ".
	"	testreal real, ".
	"	testsmallint smallint, ".
	"	testchar char(40), ".
	"	testvarchar varchar(40), ".
	"	testdate date, ".
	"	testtime time, ".
	"	testtimestamp timestamp)"));
print("\n");


# begin transction
print("BEGIN TRANSCTION: \n");
assertTrue($cur->sendQuery("begin"));
print("\n");


# insert
print("INSERT: \n");
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	1, ".
	"	1.1, ".
	"	1.1, ".
	"	1, ".
	"	'testchar1', ".
	"	'testvarchar1', ".
	"	'01/01/2001', ".
	"	'01:00:00', ".
	"	NULL)"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	2, ".
	"	2.2, ".
	"	2.2, ".
	"	2, ".
	"	'testchar2', ".
	"	'testvarchar2', ".
	"	'01/01/2002', ".
	"	'02:00:00', ".
	"	NULL)"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	3, ".
	"	3.3, ".
	"	3.3, ".
	"	3, ".
	"	'testchar3', ".
	"	'testvarchar3', ".
	"	'01/01/2003', ".
	"	'03:00:00', ".
	"	NULL)"));
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	4, ".
	"	4.4, ".
	"	4.4, ".
	"	4, ".
	"	'testchar4', ".
	"	'testvarchar4', ".
	"	'01/01/2004', ".
	"	'04:00:00', ".
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
	"	\$1, ".
	"	\$2, ".
	"	\$3, ".
	"	\$4, ".
	"	\$5, ".
	"	\$6, ".
	"	\$7, ".
	"	\$8)");
assertEqual($cur->countBindVariables(),8);
$cur->inputBind("1",5);
$cur->inputBind("2",5.5,4,2);
$cur->inputBind("3",5.5,4,2);
$cur->inputBind("4",5);
$cur->inputBind("5","testchar5");
$cur->inputBind("6","testvarchar5");
$cur->inputBind("7","01/01/2005");
$cur->inputBind("8","05:00:00");
assertTrue($cur->executeQuery());
$cur->clearBinds();
$cur->inputBind("1",6);
$cur->inputBind("2",6.6,4,2);
$cur->inputBind("3",6.6,4,2);
$cur->inputBind("4",6);
$cur->inputBind("5","testchar6");
$cur->inputBind("6","testvarchar6");
$cur->inputBind("7","01/01/2006");
$cur->inputBind("8","06:00:00");
assertTrue($cur->executeQuery());
print("\n");


# array of binds by position
print("ARRAY OF BINDS BY POSITION: \n");
$cur->clearBinds();
@vars=("1","2","3","4","5","6","7","8");
@vals=(7,7.7,7.7,7,"testchar7","testvarchar7","01/01/2007","07:00:00");
@precs=(0,4,4,0,0,0,0,0);
@scales=(0,2,2,0,0,0,0,0);
$cur->inputBinds(\@vars,\@vals,\@precs,\@scales);
assertTrue($cur->executeQuery());
print("\n");


# bind by position with validation
print("BIND BY POSITION WITH VALIDATION: \n");
$cur->clearBinds();
$cur->inputBind("1",8);
$cur->inputBind("2",8.8,4,2);
$cur->inputBind("3",8.8,4,2);
$cur->inputBind("4",8);
$cur->inputBind("5","testchar8");
$cur->inputBind("6","testvarchar8");
$cur->inputBind("7","01/01/2008");
$cur->inputBind("8","08:00:00");
$cur->validateBinds();
assertTrue($cur->executeQuery());
print("\n");


# select
print("SELECT: \n");
assertTrue($cur->sendQuery("select * from testtable order by testint"));
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEqual($cur->colCount(),9);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEqualString($cur->getColumnName(0),"testint");
assertEqualString($cur->getColumnName(1),"testfloat");
assertEqualString($cur->getColumnName(2),"testreal");
assertEqualString($cur->getColumnName(3),"testsmallint");
assertEqualString($cur->getColumnName(4),"testchar");
assertEqualString($cur->getColumnName(5),"testvarchar");
assertEqualString($cur->getColumnName(6),"testdate");
assertEqualString($cur->getColumnName(7),"testtime");
assertEqualString($cur->getColumnName(8),"testtimestamp");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"testint");
assertEqualString($cols[1],"testfloat");
assertEqualString($cols[2],"testreal");
assertEqualString($cols[3],"testsmallint");
assertEqualString($cols[4],"testchar");
assertEqualString($cols[5],"testvarchar");
assertEqualString($cols[6],"testdate");
assertEqualString($cols[7],"testtime");
assertEqualString($cols[8],"testtimestamp");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEqualString($cur->getColumnType(0),"int4");
assertEqualString($cur->getColumnType('testint'),"int4");
assertEqualString($cur->getColumnType(1),"float8");
assertEqualString($cur->getColumnType('testfloat'),"float8");
assertEqualString($cur->getColumnType(2),"float4");
assertEqualString($cur->getColumnType('testreal'),"float4");
assertEqualString($cur->getColumnType(3),"int2");
assertEqualString($cur->getColumnType('testsmallint'),"int2");
assertEqualString($cur->getColumnType(4),"bpchar");
assertEqualString($cur->getColumnType('testchar'),"bpchar");
assertEqualString($cur->getColumnType(5),"varchar");
assertEqualString($cur->getColumnType('testvarchar'),"varchar");
assertEqualString($cur->getColumnType(6),"date");
assertEqualString($cur->getColumnType('testdate'),"date");
assertEqualString($cur->getColumnType(7),"time");
assertEqualString($cur->getColumnType('testtime'),"time");
assertEqualString($cur->getColumnType(8),"timestamp");
assertEqualString($cur->getColumnType('testtimestamp'),"timestamp");
print("\n");


# column length
print("COLUMN LENGTH: \n");
assertEqual($cur->getColumnLength(0),4);
assertEqual($cur->getColumnLength('testint'),4);
assertEqual($cur->getColumnLength(1),8);
assertEqual($cur->getColumnLength('testfloat'),8);
assertEqual($cur->getColumnLength(2),4);
assertEqual($cur->getColumnLength('testreal'),4);
assertEqual($cur->getColumnLength(3),2);
assertEqual($cur->getColumnLength('testsmallint'),2);
assertEqual($cur->getColumnLength(4),44);
assertEqual($cur->getColumnLength('testchar'),44);
assertEqual($cur->getColumnLength(5),44);
assertEqual($cur->getColumnLength('testvarchar'),44);
assertEqual($cur->getColumnLength(6),4);
assertEqual($cur->getColumnLength('testdate'),4);
assertEqual($cur->getColumnLength(7),8);
assertEqual($cur->getColumnLength('testtime'),8);
assertEqual($cur->getColumnLength(8),8);
assertEqual($cur->getColumnLength('testtimestamp'),8);
print("\n");


# longest column
print("LONGEST COLUMN: \n");
assertEqual($cur->getLongest(0),1);
assertEqual($cur->getLongest('testint'),1);
assertEqual($cur->getLongest(1),3);
assertEqual($cur->getLongest('testfloat'),3);
assertEqual($cur->getLongest(2),3);
assertEqual($cur->getLongest('testreal'),3);
assertEqual($cur->getLongest(3),1);
assertEqual($cur->getLongest('testsmallint'),1);
assertEqual($cur->getLongest(4),40);
assertEqual($cur->getLongest('testchar'),40);
assertEqual($cur->getLongest(5),12);
assertEqual($cur->getLongest('testvarchar'),12);
assertEqual($cur->getLongest(6),10);
assertEqual($cur->getLongest('testdate'),10);
assertEqual($cur->getLongest(7),8);
assertEqual($cur->getLongest('testtime'),8);
print("\n");


# row count
print("ROW COUNT: \n");
assertEqual($cur->rowCount(),8);
print("\n");

#print("TOTAL ROWS: \n");
#assertEqual($cur->totalRows(),8);
#print("\n");


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
assertEqualString($cur->getField(0,1),"1.1");
assertEqualString($cur->getField(0,2),"1.1");
assertEqualString($cur->getField(0,3),"1");
assertEqualString($cur->getField(0,4),"testchar1                               ");
assertEqualString($cur->getField(0,5),"testvarchar1");
assertEqualString($cur->getField(0,6),"2001-01-01");
assertEqualString($cur->getField(0,7),"01:00:00");
print("\n");
assertEqualString($cur->getField(7,0),"8");
assertEqualString($cur->getField(7,1),"8.8");
assertEqualString($cur->getField(7,2),"8.8");
assertEqualString($cur->getField(7,3),"8");
assertEqualString($cur->getField(7,4),"testchar8                               ");
assertEqualString($cur->getField(7,5),"testvarchar8");
assertEqualString($cur->getField(7,6),"2008-01-01");
assertEqualString($cur->getField(7,7),"08:00:00");
print("\n");


# field lengths by index
print("FIELD LENGTHS BY INDEX: \n");
assertEqual($cur->getFieldLength(0,0),1);
assertEqual($cur->getFieldLength(0,1),3);
assertEqual($cur->getFieldLength(0,2),3);
assertEqual($cur->getFieldLength(0,3),1);
assertEqual($cur->getFieldLength(0,4),40);
assertEqual($cur->getFieldLength(0,5),12);
assertEqual($cur->getFieldLength(0,6),10);
assertEqual($cur->getFieldLength(0,7),8);
print("\n");
assertEqual($cur->getFieldLength(7,0),1);
assertEqual($cur->getFieldLength(7,1),3);
assertEqual($cur->getFieldLength(7,2),3);
assertEqual($cur->getFieldLength(7,3),1);
assertEqual($cur->getFieldLength(7,4),40);
assertEqual($cur->getFieldLength(7,5),12);
assertEqual($cur->getFieldLength(7,6),10);
assertEqual($cur->getFieldLength(7,7),8);
print("\n");


# fields by name
print("FIELDS BY NAME: \n");
assertEqualString($cur->getField(0,"testint"),"1");
assertEqualString($cur->getField(0,"testfloat"),"1.1");
assertEqualString($cur->getField(0,"testreal"),"1.1");
assertEqualString($cur->getField(0,"testsmallint"),"1");
assertEqualString($cur->getField(0,"testchar"),"testchar1                               ");
assertEqualString($cur->getField(0,"testvarchar"),"testvarchar1");
assertEqualString($cur->getField(0,"testdate"),"2001-01-01");
assertEqualString($cur->getField(0,"testtime"),"01:00:00");
print("\n");
assertEqualString($cur->getField(7,"testint"),"8");
assertEqualString($cur->getField(7,"testfloat"),"8.8");
assertEqualString($cur->getField(7,"testreal"),"8.8");
assertEqualString($cur->getField(7,"testsmallint"),"8");
assertEqualString($cur->getField(7,"testchar"),"testchar8                               ");
assertEqualString($cur->getField(7,"testvarchar"),"testvarchar8");
assertEqualString($cur->getField(7,"testdate"),"2008-01-01");
assertEqualString($cur->getField(7,"testtime"),"08:00:00");
print("\n");


# field lengths by name
print("FIELD LENGTHS BY NAME: \n");
assertEqual($cur->getFieldLength(0,"testint"),1);
assertEqual($cur->getFieldLength(0,"testfloat"),3);
assertEqual($cur->getFieldLength(0,"testreal"),3);
assertEqual($cur->getFieldLength(0,"testsmallint"),1);
assertEqual($cur->getFieldLength(0,"testchar"),40);
assertEqual($cur->getFieldLength(0,"testvarchar"),12);
assertEqual($cur->getFieldLength(0,"testdate"),10);
assertEqual($cur->getFieldLength(0,"testtime"),8);
print("\n");
assertEqual($cur->getFieldLength(7,"testint"),1);
assertEqual($cur->getFieldLength(7,"testfloat"),3);
assertEqual($cur->getFieldLength(7,"testreal"),3);
assertEqual($cur->getFieldLength(7,"testsmallint"),1);
assertEqual($cur->getFieldLength(7,"testchar"),40);
assertEqual($cur->getFieldLength(7,"testvarchar"),12);
assertEqual($cur->getFieldLength(7,"testdate"),10);
assertEqual($cur->getFieldLength(7,"testtime"),8);
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$cur->getRow(0);
assertEqual($fields[0],1);
assertEqual($fields[1],1.1);
assertEqual($fields[2],1.1);
assertEqual($fields[3],1);
assertEqualString($fields[4],"testchar1                               ");
assertEqualString($fields[5],"testvarchar1");
assertEqualString($fields[6],"2001-01-01");
assertEqualString($fields[7],"01:00:00");
print("\n");


# field lengths by array
print("FIELD LENGTHS BY ARRAY: \n");
@fieldlens=$cur->getRowLengths(0);
assertEqual($fieldlens[0],1);
assertEqual($fieldlens[1],3);
assertEqual($fieldlens[2],3);
assertEqual($fieldlens[3],1);
assertEqual($fieldlens[4],40);
assertEqual($fieldlens[5],12);
assertEqual($fieldlens[6],10);
assertEqual($fieldlens[7],8);
print("\n");


# fields by hash
print("FIELDS BY HASH: \n");
%fields=$cur->getRowHash(0);
assertEqual($fields{"testint"},1);
assertEqual($fields{"testfloat"},1.1);
assertEqual($fields{"testreal"},1.1);
assertEqual($fields{"testsmallint"},1);
assertEqualString($fields{"testchar"},"testchar1                               ");
assertEqualString($fields{"testvarchar"},"testvarchar1");
assertEqualString($fields{"testdate"},"2001-01-01");
assertEqualString($fields{"testtime"},"01:00:00");
print("\n");
%fields=$cur->getRowHash(7);
assertEqual($fields{"testint"},8);
assertEqual($fields{"testfloat"},8.8);
assertEqual($fields{"testreal"},8.8);
assertEqual($fields{"testsmallint"},8);
assertEqualString($fields{"testchar"},"testchar8                               ");
assertEqualString($fields{"testvarchar"},"testvarchar8");
assertEqualString($fields{"testdate"},"2008-01-01");
assertEqualString($fields{"testtime"},"08:00:00");
print("\n");


# field lengths by hash
print("FIELD LENGTHS BY HASH: \n");
%fieldlengths=$cur->getRowLengthsHash(0);
assertEqual($fieldlengths{"testint"},1);
assertEqual($fieldlengths{"testfloat"},3);
assertEqual($fieldlengths{"testreal"},3);
assertEqual($fieldlengths{"testsmallint"},1);
assertEqual($fieldlengths{"testchar"},40);
assertEqual($fieldlengths{"testvarchar"},12);
assertEqual($fieldlengths{"testdate"},10);
assertEqual($fieldlengths{"testtime"},8);
print("\n");
%fieldlengths=$cur->getRowLengthsHash(7);
assertEqual($fieldlengths{"testint"},1);
assertEqual($fieldlengths{"testfloat"},3);
assertEqual($fieldlengths{"testreal"},3);
assertEqual($fieldlengths{"testsmallint"},1);
assertEqual($fieldlengths{"testchar"},40);
assertEqual($fieldlengths{"testvarchar"},12);
assertEqual($fieldlengths{"testdate"},10);
assertEqual($fieldlengths{"testtime"},8);
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
@specs=(0,0,6);
@precs=(0,0,4);
$cur->substitutions(\@vars,\@vals,\@specs,\@precs);
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
$cur->setResultSetBufferSize(0);
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
assertEqualString($cur->getColumnType(0),"int4");
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
assertEqual($cur->colCount(),9);
print("\n");


# column names for cached result set
print("COLUMN NAMES FOR CACHED RESULT SET: \n");
assertEqualString($cur->getColumnName(0),"testint");
assertEqualString($cur->getColumnName(1),"testfloat");
assertEqualString($cur->getColumnName(2),"testreal");
assertEqualString($cur->getColumnName(3),"testsmallint");
assertEqualString($cur->getColumnName(4),"testchar");
assertEqualString($cur->getColumnName(5),"testvarchar");
assertEqualString($cur->getColumnName(6),"testdate");
assertEqualString($cur->getColumnName(7),"testtime");
assertEqualString($cur->getColumnName(8),"testtimestamp");
@cols=$cur->getColumnNames();
assertEqualString($cols[0],"testint");
assertEqualString($cols[1],"testfloat");
assertEqualString($cols[2],"testreal");
assertEqualString($cols[3],"testsmallint");
assertEqualString($cols[4],"testchar");
assertEqualString($cols[5],"testvarchar");
assertEqualString($cols[6],"testdate");
assertEqualString($cols[7],"testtime");
assertEqualString($cols[8],"testtimestamp");
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


# commit
print("COMMIT: \n");
$secondcon=SQLRelay::Connection->new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
$secondcur=SQLRelay::Cursor->new($secondcon);
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"0");
assertTrue($con->commit());
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"8");
#assertTrue($con->autoCommitOn());
assertTrue($cur->sendQuery(
	"insert into ".
	"	testtable ".
	"values (".
	"	10, ".
	"	10.1, ".
	"	10.1, ".
	"	10, ".
	"	'testchar10', ".
	"	'testvarchar10', ".
	"	'01/01/2010', ".
	"	'10:00:00', ".
	"	NULL)"));
assertTrue($secondcur->sendQuery("select count(*) from testtable"));
assertEqualString($secondcur->getField(0,0),"9");
#assertTrue($con->autoCommitOff());
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


# stored procedures
print("STORED PROCEDURES: \n");
# return no values
$cur->sendQuery("drop function testfunc(int,float,char(20))");
assertTrue($cur->sendQuery(
	"create function testfunc(".
	"	int,float,char(20)) ".
	"returns void as ' ".
	"	declare in1 int; ".
	"	in2 float; ".
	"	in3 char(20); ".
	"begin ".
	"	in1:=\$1; ".
	"	in2:=\$2; ".
	"	in3:=\$3; ".
	"	return; ".
	"end;' language plpgsql"));
$cur->prepareQuery("select testfunc(\$1,\$2,\$3)");
$cur->inputBind("1",1);
$cur->inputBind("2",1.1,4,2);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
$cur->sendQuery("drop function testfunc(int,float,char(20))");
print("\n");
# return single value
$cur->sendQuery("drop function testfunc(int,float,char(20))");
assertTrue($cur->sendQuery(
	"create function testfunc(int,float,char(20)) returns int as ".
	"	' begin return \$1; end;' language plpgsql"));
$cur->prepareQuery("select * from testfunc(\$1,\$2,\$3)");
$cur->inputBind("1",1);
$cur->inputBind("2",1.1,4,2);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
assertEqual($cur->getField(0,0),"1");
$cur->sendQuery("drop function testfunc(int,float,char(20))");
print("\n");
# return multiple values
$cur->sendQuery("drop function testfunc(int,char(20))");
assertTrue($cur->sendQuery(
	"create function testfunc(".
	"	int,float,char(20)) ".
	"returns record as ' ".
	"	declare output record; ".
	"begin ".
	"	select \$1,\$2,\$3 into output; ".
	"	return output; ".
	"end;' language plpgsql"));
$cur->prepareQuery(
	"select ".
	"	* ".
	"from ".
	"	testfunc(\$1,\$2,\$3) as (col1 int, col2 float, col3 bpchar) ");
$cur->inputBind("1",1);
$cur->inputBind("2",1.1,4,2);
$cur->inputBind("3","hello");
assertTrue($cur->executeQuery());
assertEqual($cur->getField(0,0),"1");
assertEqual($cur->getField(0,1),1.1);
assertEqual($cur->getField(0,2),"hello");
$cur->sendQuery("drop function testfunc(int,float,char(20))");
print("\n");
# return result set
$cur->sendQuery("drop function testfunc()");
assertTrue($cur->sendQuery(
	"create function testfunc() ".
	"returns setof record as ' ".
	"	declare output record; ".
	"begin ".
	"	for output in ".
	"		select * from testtable ".
	"	loop ".
	"		return next output; ".
	"	end loop; ".
	"	return; ".
	"end;' language plpgsql"));
assertTrue($cur->sendQuery(
	"select ".
	"	* ".
	"from ".
	"	testfunc() ".
	"	as (testint int, ".
	"		testfloat float, ".
	"		testreal real, ".
	"		testsmallint smallint, ".
	"		testchar char(40), ".
	"		testvarchar varchar(40), ".
	"		testdate date, ".
	"		testtime time, ".
	"		testtimestamp timestamp) "));
assertEqual($cur->getField(4,0),"5");
assertEqual($cur->getField(5,0),"6");
assertEqual($cur->getField(6,0),"7");
assertEqual($cur->getField(7,0),"8");
$cur->sendQuery("drop function testfunc()");
print("\n");

# drop existing table
$cur->sendQuery("drop table testtable");


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
