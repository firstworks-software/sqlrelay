#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.

require 'rbconfig'
require 'sqlrelay'
require './asserts'


isolationlevels=["0","1"]
subvars=["var1","var2","var3"]
subvalstrings=["hi","hello","bye"]
subvallongs=[1,2,3]
subvaldoubles=[10.55,10.556,10.5556]
precs=[4,5,6]
scales=[2,3,4]
counter=0


# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
					"testuser","testpassword",0,1)
cur=SQLRCursor.new(con)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"sqlite")
print "\n"


# db version
print "DB VERSION: \n"
dbversion=con.dbVersion()
issqlite3=true
if !dbversion ||
	dbversion.eql?("unknown") ||
	dbversion.to_i<3
	issqlite3=false
end
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
print "\n"


# transaction state
print "TRANSACTION STATE: \n"
assertEqual(con.getDefaultTransactionModel(),"explicit")
assertEqual(con.getTransactionModel(),"explicit")
assertFalse(con.getInTransaction())
assertTrue(con.getAutoCommit())
print "\n"


# bind format
print "BIND FORMAT: \n"
assertEqual(con.bindFormat(),":*")
print "\n"


# nextval format
print "NEXTVAL FORMAT: \n"
assertEqual(con.nextvalFormat(),"")
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
for il in isolationlevels
	assertTrue(con.setIsolationLevel(il))
	assertEqual(con.getIsolationLevel(),il)
	print "\n"
end
# reset to the default isolation level
assertTrue(con.setIsolationLevel(isolationlevels[0]))
print "\n"


# create testtable
print "CREATE TESTTABLE: \n"
con.begin()
cur.sendQuery("drop table if exists testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testfloat float, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testclob clob, "+
	"	testblob blob)"))
con.commit()
print "\n"


# insert
print "INSERT: \n"
assertTrue(con.begin())
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1.1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'testclob1', "+
	"	'testblob1')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	2, "+
	"	2.2, "+
	"	'testchar2', "+
	"	'testvarchar2', "+
	"	'testclob2', "+
	"	'testblob2')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	3, "+
	"	3.3, "+
	"	'testchar3', "+
	"	'testvarchar3', "+
	"	'testclob3', "+
	"	'testblob3')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	4, "+
	"	4.4, "+
	"	'testchar4', "+
	"	'testvarchar4', "+
	"	'testclob4', "+
	"	'testblob4')"))
print "\n"


# affected rows
print "AFFECTED ROWS: \n"
assertEqual(cur.affectedRows(),1)
print "\n"


# input bind by position
# sqlite doesn't support bind by position


# array of input binds by position
# sqlite doesn't support bind by position


# input bind by position with validation
# sqlite doesn't support bind by position


# input bind by name
print "INPUT BIND BY NAME: \n"
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	:var1, "+
	"	:var2, "+
	"	:var3, "+
	"	:var4, "+
	"	:var5, "+
	"	:var6)")
assertEqual(cur.countBindVariables(),6)
cur.inputBind("var1",5)
cur.inputBind("var2",5.5,4,1)
cur.inputBind("var3","testchar5")
cur.inputBind("var4","testvarchar5")
cur.inputBindClob("var5","testclob5","testclob5".to_s.bytesize)
cur.inputBindBlob("var6","testblob5","testblob5".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("var1",6)
cur.inputBind("var2",6.6,4,1)
cur.inputBind("var3","testchar6")
cur.inputBind("var4","testvarchar6")
cur.inputBindClob("var5","testclob6","testclob6".to_s.bytesize)
cur.inputBindBlob("var6","testblob6","testblob6".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("var1",7)
cur.inputBind("var2",7.7,4,1)
cur.inputBind("var3","testchar7")
cur.inputBind("var4","testvarchar7")
cur.inputBindClob("var5","testclob7","testclob7".to_s.bytesize)
cur.inputBindBlob("var6","testblob7","testblob7".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by name
# sqlite doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by name with validation
print "INPUT BIND BY NAME WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("var1",8)
cur.inputBind("var2",8.8,4,1)
cur.inputBind("var3","testchar8")
cur.inputBind("var4","testvarchar8")
cur.inputBindClob("var5","testclob8","testclob8".to_s.bytesize)
cur.inputBindBlob("var6","testblob8","testblob8".to_s.bytesize)
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# select
print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),6)
print "\n"


# column names
print "COLUMN NAMES: \n"
assertEqual(cur.getColumnName(0),"testint")
assertEqual(cur.getColumnName(1),"testfloat")
assertEqual(cur.getColumnName(2),"testchar")
assertEqual(cur.getColumnName(3),"testvarchar")
cols=cur.getColumnNames()
assertEqual(cols[0],"testint")
assertEqual(cols[1],"testfloat")
assertEqual(cols[2],"testchar")
assertEqual(cols[3],"testvarchar")
print "\n"


# column types
print "COLUMN TYPES: \n"
if issqlite3
	assertEqual(cur.getColumnType(0),"INTEGER")
	assertEqual(cur.getColumnType("testint"),"INTEGER")
	assertEqual(cur.getColumnType(1),"FLOAT")
	assertEqual(cur.getColumnType("testfloat"),"FLOAT")
	assertEqual(cur.getColumnType(2),"STRING")
	assertEqual(cur.getColumnType("testchar"),"STRING")
	assertEqual(cur.getColumnType(3),"STRING")
	assertEqual(cur.getColumnType("testvarchar"),"STRING")
	assertEqual(cur.getColumnType(4),"STRING")
	assertEqual(cur.getColumnType("testclob"),"STRING")
	assertEqual(cur.getColumnType(5),"STRING")
	assertEqual(cur.getColumnType("testblob"),"STRING")
else
	assertEqual(cur.getColumnType(0),"UNKNOWN")
	assertEqual(cur.getColumnType("testint"),"UNKNOWN")
	assertEqual(cur.getColumnType(1),"UNKNOWN")
	assertEqual(cur.getColumnType("testfloat"),"UNKNOWN")
	assertEqual(cur.getColumnType(2),"UNKNOWN")
	assertEqual(cur.getColumnType("testchar"),"UNKNOWN")
	assertEqual(cur.getColumnType(3),"UNKNOWN")
	assertEqual(cur.getColumnType("testvarchar"),"UNKNOWN")
	assertEqual(cur.getColumnType(4),"UNKNOWN")
	assertEqual(cur.getColumnType("testclob"),"UNKNOWN")
	assertEqual(cur.getColumnType(5),"UNKNOWN")
	assertEqual(cur.getColumnType("testblob"),"UNKNOWN")
end
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnLength("testint"),0)
assertEqual(cur.getColumnLength(1),0)
assertEqual(cur.getColumnLength("testfloat"),0)
assertEqual(cur.getColumnLength(2),0)
assertEqual(cur.getColumnLength("testchar"),0)
assertEqual(cur.getColumnLength(3),0)
assertEqual(cur.getColumnLength("testvarchar"),0)
assertEqual(cur.getColumnLength(4),0)
assertEqual(cur.getColumnLength("testclob"),0)
assertEqual(cur.getColumnLength(5),0)
assertEqual(cur.getColumnLength("testblob"),0)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest("testint"),1)
assertEqual(cur.getLongest(1),3)
assertEqual(cur.getLongest("testfloat"),3)
assertEqual(cur.getLongest(2),9)
assertEqual(cur.getLongest("testchar"),9)
assertEqual(cur.getLongest(3),12)
assertEqual(cur.getLongest("testvarchar"),12)
assertEqual(cur.getLongest(4),9)
assertEqual(cur.getLongest("testclob"),9)
assertEqual(cur.getLongest(5),9)
assertEqual(cur.getLongest("testblob"),9)
print "\n"


# row count
print "ROW COUNT: \n"
assertEqual(cur.rowCount(),8)
print "\n"


# total rows
print "TOTAL ROWS: \n"
assertEqual(cur.totalRows(),(issqlite3) ? 0 : 8)
print "\n"


# first row index
print "FIRST ROW INDEX: \n"
assertEqual(cur.firstRowIndex(),0)
print "\n"


# end of result set
print "END OF RESULT SET: \n"
assertTrue(cur.endOfResultSet())
print "\n"


# fields by index
print "FIELDS BY INDEX: \n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"1.1")
assertEqual(cur.getField(0,2),"testchar1")
assertEqual(cur.getField(0,3),"testvarchar1")
assertEqual(cur.getField(0,4),"testclob1")
assertEqual(cur.getField(0,5),"testblob1")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8.8")
assertEqual(cur.getField(7,2),"testchar8")
assertEqual(cur.getField(7,3),"testvarchar8")
assertEqual(cur.getField(7,4),"testclob8")
assertEqual(cur.getField(7,5),"testblob8")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),3)
assertEqual(cur.getFieldLength(0,2),9)
assertEqual(cur.getFieldLength(0,3),12)
assertEqual(cur.getFieldLength(0,4),9)
assertEqual(cur.getFieldLength(0,5),9)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),3)
assertEqual(cur.getFieldLength(7,2),9)
assertEqual(cur.getFieldLength(7,3),12)
assertEqual(cur.getFieldLength(7,4),9)
assertEqual(cur.getFieldLength(7,5),9)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"testint"),"1")
assertEqual(cur.getField(0,"testfloat"),"1.1")
assertEqual(cur.getField(0,"testchar"),"testchar1")
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
assertEqual(cur.getField(0,"testclob"),"testclob1")
assertEqual(cur.getField(0,"testblob"),"testblob1")
print "\n"
assertEqual(cur.getField(7,"testint"),"8")
assertEqual(cur.getField(7,"testfloat"),"8.8")
assertEqual(cur.getField(7,"testchar"),"testchar8")
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
assertEqual(cur.getField(7,"testclob"),"testclob8")
assertEqual(cur.getField(7,"testblob"),"testblob8")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"testint"),1)
assertEqual(cur.getFieldLength(0,"testfloat"),3)
assertEqual(cur.getFieldLength(0,"testchar"),9)
assertEqual(cur.getFieldLength(0,"testvarchar"),12)
assertEqual(cur.getFieldLength(0,"testclob"),9)
assertEqual(cur.getFieldLength(0,"testblob"),9)
print "\n"
assertEqual(cur.getFieldLength(7,"testint"),1)
assertEqual(cur.getFieldLength(7,"testfloat"),3)
assertEqual(cur.getFieldLength(7,"testchar"),9)
assertEqual(cur.getFieldLength(7,"testvarchar"),12)
assertEqual(cur.getFieldLength(7,"testclob"),9)
assertEqual(cur.getFieldLength(7,"testblob"),9)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1.1")
assertEqual(fields[2],"testchar1")
assertEqual(fields[3],"testvarchar1")
assertEqual(fields[4],"testclob1")
assertEqual(fields[5],"testblob1")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],3)
assertEqual(fieldlens[2],9)
assertEqual(fieldlens[3],12)
assertEqual(fieldlens[4],9)
assertEqual(fieldlens[5],9)
print "\n"


# result set buffer size
print "RESULT SET BUFFER SIZE: \n"
assertEqual(cur.getResultSetBufferSize(),0)
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getResultSetBufferSize(),2)
print "\n"
assertEqual(cur.firstRowIndex(),0)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),2)
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(1,0),"2")
assertEqual(cur.getField(2,0),"3")
print "\n"
assertEqual(cur.firstRowIndex(),2)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),4)
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,0),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.setResultSetBufferSize(0)
print "\n"


# dont get column info
print "DONT GET COLUMN INFO: \n"
cur.dontGetColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getColumnName(0),nil)
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),nil)
cur.getColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getColumnName(0),"testint")
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),
			(issqlite3) ? "INTEGER" : "UNKNOWN")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
print "\n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(1,0),"2")
assertEqual(cur.getField(2,0),"3")
assertEqual(cur.getField(3,0),"4")
assertEqual(cur.getField(4,0),"5")
assertEqual(cur.getField(5,0),"6")
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
print "\n"


# suspended result set
print "SUSPENDED RESULT SET: \n"
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getField(2,0),"3")
id=cur.getResultSetId()
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
assertTrue(cur.resumeResultSet(id))
print "\n"
assertEqual(cur.firstRowIndex(),4)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),6)
assertEqual(cur.getField(7,0),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,0),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.setResultSetBufferSize(0)
print "\n"


# cached result set
print "CACHED RESULT SET: \n"
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
print "\n"


# column count for cached result set
print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),6)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
assertEqual(cur.getColumnName(0),"testint")
assertEqual(cur.getColumnName(1),"testfloat")
assertEqual(cur.getColumnName(2),"testchar")
assertEqual(cur.getColumnName(3),"testvarchar")
assertEqual(cur.getColumnName(4),"testclob")
assertEqual(cur.getColumnName(5),"testblob")
cols=cur.getColumnNames()
assertEqual(cols[0],"testint")
assertEqual(cols[1],"testfloat")
assertEqual(cols[2],"testchar")
assertEqual(cols[3],"testvarchar")
assertEqual(cols[4],"testclob")
assertEqual(cols[5],"testblob")
print "\n"


# cached result set with result set buffer size
print "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# from one cache file to another
print "FROM ONE CACHE FILE TO ANOTHER: \n"
cur.cacheToFile("cachefile2")
assertTrue(cur.openCachedResultSet("cachefile1"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
print "\n"


# from one cache file to another with result set buffer size
print "FROM ONE CACHE FILE TO ANOTHER "+
	"WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile2")
assertTrue(cur.openCachedResultSet("cachefile1"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# cached result set with suspend and result set buffer size
print "CACHED RESULT SET WITH SUSPEND "+
	"AND RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getField(2,0),"3")
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
id=cur.getResultSetId()
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
print "\n"
assertTrue(con.resumeSession(port,socket))
assertTrue(cur.resumeCachedResultSet(id,filename))
print "\n"
assertEqual(cur.firstRowIndex(),4)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),6)
assertEqual(cur.getField(7,0),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,0),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.cacheOff()
print "\n"
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# finished suspended session
print "FINISHED SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable"))
assertEqual(cur.getField(4,0),"5")
assertEqual(cur.getField(5,0),"6")
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
id=cur.getResultSetId()
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
assertTrue(cur.resumeResultSet(id))
assertEqual(cur.getField(4,0),nil)
assertEqual(cur.getField(5,0),nil)
assertEqual(cur.getField(6,0),nil)
assertEqual(cur.getField(7,0),nil)
print "\n"


# nested selects
print "NESTED SELECTS: \n"
cur.setResultSetBufferSize(1)
assertTrue(cur.sendQuery("select * from testtable"))
i=0
while cur.getRow(i)
	secondcur=SQLRCursor.new(con)
	secondcur.setResultSetBufferSize(1)
	assertTrue(secondcur.sendQuery("select * from testtable"))
	secondcur.closeResultSet()
	i=i+1
end
cur.setResultSetBufferSize(0)
assertTrue(cur.sendQuery("drop table if exists testtable"))
print "\n"


# reset transaction state
print "RESET TRANSACTION STATE: \n"
assertTrue(con.commit())
assertEqual(con.getTransactionModel(),"explicit")
assertTrue(con.getAutoCommit())
print "\n"


# transaction behavior - implicit
print "TRANSACTION BEHAVIOR - implicit: \n"
assertTrue(con.setTransactionModel("implicit"))
assertEqual(con.getTransactionModel(),"implicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
# sqlite DDL is transactional; commit so the table is visible
# to the second connection (the commit implicitly starts a new tx)
assertTrue(con.commit())
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
					"testuser","testpassword",0,1)
secondcur=SQLRCursor.new(secondcon)
setSecondConnection(secondcon)
setSecondCursor(secondcur)
# session is in a transaction; insert is not visible until commit
assertTrue(con.getInTransaction())
assertFalse(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
# commit makes it visible, and implicitly starts a new transaction
assertTrue(con.commit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# rollback discards, and implicitly starts a new transaction
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
secondcur.closeResultSet()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit
print "TRANSACTION BEHAVIOR - explicit: \n"
assertTrue(con.setTransactionModel("explicit"))
assertEqual(con.getTransactionModel(),"explicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
# begin starts a new transaction; insert is not visible until commit
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
# commit makes it visible; no new transaction is started
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback discards; no new transaction is started
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
secondcur.closeResultSet()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit-deferred
print "TRANSACTION BEHAVIOR - explicit-deferred: \n"
assertTrue(con.setTransactionModel("explicit-deferred"))
assertEqual(con.getTransactionModel(),"explicit-deferred")
# switch to autocommit-on so the begin/commit cycles below
# bracket explicit transactions (autocommit-off semantics are
# exercised at the end of this block)
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
# begin starts a transaction; commit makes it visible
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback discards
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# during a transaction started by begin(), autoCommitOn is a
# no-op: the autocommit setting takes effect after the user
# explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (3)"))
assertTrue(con.autoCommitOn())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# explicit commit ends the tx; autocommit-on now takes effect
assertTrue(con.commit())
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autocommit is on; subsequent inserts are visible immediately
assertTrue(cur.sendQuery("insert into testtable values (4)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"3")
# autoCommitOff takes effect immediately when not in a transaction
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
# autocommit-off persists across commit/rollback; each commit or
# rollback ends the current implicit tx and a new one starts for
# the next statement
assertTrue(cur.sendQuery("insert into testtable values (5)"))
assertTrue(con.commit())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"4")
assertTrue(cur.sendQuery("insert into testtable values (6)"))
assertTrue(con.rollback())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"4")
# autoCommitOff during a transaction changes the variable
# immediately but the in-flight tx continues; only after the
# next explicit commit/rollback does the new autocommit-off
# setting drop us into a new implicit tx (mysql-asymmetric
# semantic)
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (7)"))
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"4")
assertTrue(con.commit())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"5")
secondcur.closeResultSet()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit-error
print "TRANSACTION BEHAVIOR - explicit-error: \n"
assertTrue(con.setTransactionModel("explicit-error"))
assertEqual(con.getTransactionModel(),"explicit-error")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
# begin, insert, commit
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# while in a transaction, autoCommitOn/Off throw an error
assertTrue(con.begin())
assertFalse(con.autoCommitOn())
assertFalse(con.autoCommitOff())
assertTrue(con.commit())
# outside of a transaction, autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
secondcur.closeResultSet()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - none
print "TRANSACTION BEHAVIOR - none: \n"
assertTrue(con.setTransactionModel("none"))
assertEqual(con.getTransactionModel(),"none")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
# no transactions; everything is visible immediately
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# commit and rollback are no-ops
assertTrue(con.commit())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autocommit is always on; autoCommitOff is an error
assertFalse(con.autoCommitOff())
assertTrue(con.getAutoCommit())
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
secondcur.closeResultSet()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# reset transaction behavior
print "RESET TRANSACTION BEHAVIOR: \n"
assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()))
assertEqual(con.getTransactionModel(),"explicit")
assertTrue(con.getAutoCommit())
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.sendQuery("drop table if exists testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int, "+
	"	col2 char, "+
	"	col3 float)"))
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$(var1), "+
	"	'$(var2)', "+
	"	$(var3))")
cur.substitution("var1",1)
cur.substitution("var2","hello")
cur.substitution("var3",10.5556,6,4)
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("select * from testtable"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
assertTrue(cur.sendQuery("delete from testtable"))
print "\n"


# array substitutions
print "ARRAY SUBSTITUTIONS: \n"
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	'$(var1)', "+
	"	'$(var2)', "+
	"	'$(var3)')")
cur.substitutions(subvars,subvalstrings)
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("select * from testtable"))
assertEqual(cur.getField(0,0),"hi")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"bye")
assertTrue(cur.sendQuery("delete from testtable"))
print "\n"
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$(var1), "+
	"	'$(var2)', "+
	"	$(var3))")
cur.substitutions(subvars,subvallongs)
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("select * from testtable"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"2")
assertEqual(cur.getField(0,2),"3.0")
assertTrue(cur.sendQuery("delete from testtable"))
print "\n"
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$(var1), "+
	"	'$(var2)', "+
	"	$(var3))")
cur.substitutions(subvars,subvaldoubles,precs,scales)
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("select * from testtable"))
assertEqual(cur.getField(0,0),"10.55")
assertEqual(cur.getField(0,1),"10.556")
assertEqual(cur.getField(0,2),"10.5556")
assertTrue(cur.sendQuery("delete from testtable"))
print "\n"


# nulls as nulls
print "NULLS AS NULLS: \n"
cur.getNullsAsNils()
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	NULL, "+
	"	NULL)"))
assertTrue(cur.sendQuery("select * from testtable"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select * from testtable"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"")
assertEqual(cur.getField(0,2),"")
assertTrue(cur.sendQuery("drop table if exists testtable"))
print "\n"


# null and empty lobs
print "NULL AND EMPTY LOBS: \n"
cur.getNullsAsNils()
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob1 clob, "+
	"	testclob2 clob, "+
	"	testblob1 blob, "+
	"	testblob2 blob)"))
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	:var1, "+
	"	:var2, "+
	"	:var3, "+
	"	:var4)")
cur.inputBindClob("var1","","".to_s.bytesize)
cur.inputBindClob("var2",nil,nil.to_s.bytesize)
cur.inputBindBlob("var3","","".to_s.bytesize)
cur.inputBindBlob("var4",nil,nil.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getField(0,0),"")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2),"")
assertEqual(cur.getField(0,3),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# long lobs
print "LONG LOBS: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)")
cur.prepareQuery("insert into testtable values (:clobval,:blobval)")
largebuffer = "C" * 8192
cur.inputBindClob("clobval",largebuffer,largebuffer.to_s.bytesize)
cur.inputBindBlob("blobval",largebuffer,largebuffer.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getFieldLength(0,"testclob"),8192)
assertEqual(cur.getField(0,"testclob"),largebuffer)
assertEqual(cur.getFieldLength(0,"testblob"),8192)
assertEqualLen(cur.getField(0,"testblob"),largebuffer,8192)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


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
print "NEGATIVE INPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery("create table testtable (testval int)")
cur.prepareQuery("insert into testtable values (:testval)")
cur.inputBind("testval",-1)
assertTrue(cur.executeQuery())
cur.sendQuery("select testval from testtable")
assertEqual(cur.getField(0,"testval"),"-1")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# bind validation
print "BIND VALIDATION: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	col1 varchar(20), "+
	"	col2 varchar(20), "+
	"	col3 varchar(20))")
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$(var1), "+
	"	$(var2), "+
	"	$(var3))")
cur.inputBind("var1","1")
cur.inputBind("var2","2")
cur.inputBind("var3","3")
cur.substitution("var1",":var1")
assertTrue(cur.validBind("var1"))
assertFalse(cur.validBind("var2"))
assertFalse(cur.validBind("var3"))
assertFalse(cur.validBind("var4"))
print "\n"
cur.substitution("var2",":var2")
assertTrue(cur.validBind("var1"))
assertTrue(cur.validBind("var2"))
assertFalse(cur.validBind("var3"))
assertFalse(cur.validBind("var4"))
print "\n"
cur.substitution("var3",":var3")
assertTrue(cur.validBind("var1"))
assertTrue(cur.validBind("var2"))
assertTrue(cur.validBind("var3"))
assertFalse(cur.validBind("var4"))
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# rebinding
print "REBINDING: \n"
cur.prepareQuery("select :val")
cur.inputBind("val",1)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
cur.inputBind("val",2)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"2")
cur.inputBind("val",3)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"3")
print "\n"


# reexecute
print "REEXECUTE: \n"
cur.prepareQuery("select 1")
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.prepareQuery("select :var")
cur.inputBind("var",1)
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.inputBind("var",2)
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"2")
print "\n"


# stored procedure returning no value
# sqlite doesn't support stored procedures


# stored procedure returning single value
# sqlite doesn't support stored procedures


# stored procedure returning multiple values
# sqlite doesn't support stored procedures


# stored procedure returning result set
# sqlite doesn't support stored procedures


# temporary tables
print "TEMPORARY TABLES: \n"
cur.sendQuery("drop table if exists temptable\n")
cur.sendQuery("create temporary table temptable (col1 int)")
assertTrue(cur.sendQuery("insert into temptable values (1)"))
assertTrue(cur.sendQuery("select count(*) from temptable"))
assertEqual(cur.getField(0,0),"1")
con.endSession()
print "\n"
assertFalse(cur.sendQuery("select count(*) from temptable"))
assertTrue(cur.sendQuery("drop table if exists temptable\n"))
print "\n"


# encoded binary data
print "ENCODED BINARY DATA: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 blob)"))
buffer = (0..255).map { |j| j.chr }.join
querystr = "insert into testtable values (X'"
querystr = querystr + buffer.bytes.map { |b| "%02x" % b }.join
querystr = querystr + "')"
assertTrue(cur.sendQuery(querystr))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),buffer.length)
assertEqual(cur.getField(0,0),buffer)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes
print "QUOTES: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('''''')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"''")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# last insert id
print "LAST INSERT ID: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
		"create table testtable "+
		"	(col1 integer primary key "+
		"	autoincrement, "+
		"	col2 int)"))
assertTrue(cur.sendQuery(
		"insert into testtable values (null,1)"))
assertEqual(con.getLastInsertId(),1)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# database is schema
print "DATABASE IS SCHEMA: \n"
assertFalse(con.getDatabaseIsSchema())
print "\n"


# catalog list
print "CATALOG LIST: \n"
assertTrue(cur.getCatalogList(nil))
assertEqual(cur.getColumnName(0),"Database")
print "\n"


# schema list
print "SCHEMA LIST: \n"
assertTrue(cur.getSchemaList(nil))
assertEqual(cur.getColumnName(0),"Database")
print "\n"


# table type list
print "TABLE TYPE LIST: \n"
assertTrue(cur.getTableTypeList())
assertEqual(cur.getColumnName(0),"table_type")
found=false
for i in 0...cur.rowCount()
	if cur.getField(i,"table_type").eql?("TABLE")
		found=true
		break
	end
end
assertTrue(found)
print "\n"


# table list
print "TABLE LIST: \n"
cur.sendQuery("drop table if exists testtable1")
cur.sendQuery("drop table if exists testtable2")
cur.sendQuery("drop table if exists testtable3")
cur.sendQuery("drop table if exists testtable4")
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.getTableList(nil))
counter=0
for i in 0...cur.rowCount()
	name=cur.getField(i,"Tables_in_xxx")
	if name.eql?("testtable1") ||
		name.eql?("testtable2") ||
		name.eql?("testtable3") ||
		name.eql?("testtable4")
		counter=counter+1
	end
end
assertEqual(counter,4)
assertTrue(cur.sendQuery("drop table if exists testtable1"))
assertTrue(cur.sendQuery("drop table if exists testtable2"))
assertTrue(cur.sendQuery("drop table if exists testtable3"))
assertTrue(cur.sendQuery("drop table if exists testtable4"))
print "\n"


# type info list
print "TYPE INFO LIST: \n"
assertTrue(cur.getTypeInfoList("integer"))
assertEqual(cur.getColumnName(0),"type_name")
assertEqual(cur.getColumnName(1),"data_type")
assertEqual(cur.getColumnName(2),"precision")
assertEqual(cur.getColumnName(3),"literal_prefix")
assertEqual(cur.getColumnName(4),"literal_suffix")
assertEqual(cur.getColumnName(5),"create_params")
assertEqual(cur.getColumnName(6),"nullable")
assertEqual(cur.getColumnName(7),"case_sensitive")
assertEqual(cur.getColumnName(8),"searchable")
assertEqual(cur.getColumnName(9),"unsigned_attribute")
assertEqual(cur.getColumnName(10),"fixed_prec_scale")
assertEqual(cur.getColumnName(11),"auto_increment")
assertEqual(cur.getColumnName(12),"local_type_name")
assertEqual(cur.getColumnName(13),"minumum_scale")
assertEqual(cur.getColumnName(14),"maxiumm_scale")
assertEqual(cur.getColumnName(15),"sql_data_type")
assertEqual(cur.getColumnName(16),"sql_datetime_sub")
assertEqual(cur.getColumnName(17),"num_prec_radix")
assertEqual(cur.getColumnName(18),"interval_precision")
assertEqual(cur.getField(0,"type_name"),"INTEGER")
assertEqual(cur.getField(0,"data_type"),"4")
assertEqual(cur.getField(0,"precision"),"19")
assertEqual(cur.getField(0,"local_type_name"),"INTEGER")
assertTrue(cur.getTypeInfoList("char"))
assertEqual(cur.getField(0,"type_name"),"CHAR")
assertEqual(cur.getField(0,"data_type"),"1")
assertEqual(cur.getField(0,"precision"),"2147483647")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"2147483647")
assertEqual(cur.getField(0,"local_type_name"),"VARCHAR")
assertTrue(cur.getTypeInfoList("date"))
assertEqual(cur.getField(0,"type_name"),"DATE")
assertEqual(cur.getField(0,"data_type"),"91")
assertEqual(cur.getField(0,"precision"),"10")
assertEqual(cur.getField(0,"local_type_name"),"DATE")
print "\n"


# column list
print "COLUMN LIST: \n"
cur.sendQuery("drop table if exists testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testfloat float, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getColumnName(0),"column_name")
assertEqual(cur.getColumnName(1),"data_type")
assertEqual(cur.getColumnName(2),"character_maximum_length")
assertEqual(cur.getColumnName(3),"numeric_precision")
assertEqual(cur.getColumnName(4),"numeric_scale")
assertEqual(cur.getColumnName(5),"is_nullable")
assertEqual(cur.getColumnName(6),"column_key")
assertEqual(cur.getColumnName(7),"column_default")
assertEqual(cur.getColumnName(8),"extra")
assertEqual(cur.getField(0,"column_name"),"testint")
assertEqual(cur.getField(1,"column_name"),"testfloat")
assertEqual(cur.getField(2,"column_name"),"testchar")
assertEqual(cur.getField(3,"column_name"),"testvarchar")
assertEqual(cur.getField(4,"column_name"),"testclob")
assertEqual(cur.getField(5,"column_name"),"testblob")
assertEqual(cur.getField(0,"data_type"),"INT")
assertEqual(cur.getField(1,"data_type"),"FLOAT")
assertEqual(cur.getField(2,"data_type"),"CHAR")
assertEqual(cur.getField(3,"data_type"),"VARCHAR")
assertEqual(cur.getField(4,"data_type"),"CLOB")
assertEqual(cur.getField(5,"data_type"),"BLOB")
assertTrue(cur.sendQuery("drop table if exists testtable"))
print "\n"


# column list - auto_increment, primary key
print "COLUMN LIST - auto_increment, primary key: \n"
cur.sendQuery("drop table if exists testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 integer primary key autoincrement, "+
	"	col2 int)"))
assertTrue(cur.getColumnList("testtable",nil))
assertTrue(cur.getField(0,"extra").include?("auto_increment"))
assertTrue(cur.getField(0,"column_key").include?("PRI"))
assertFalse(cur.getField(1,"extra").include?("auto_increment"))
assertFalse(cur.getField(1,"column_key").include?("PRI"))
print "\n"
assertTrue(cur.sendQuery("drop table if exists testtable"))
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getColumnList("testtable",nil))
assertFalse(cur.getField(0,"extra").include?("auto_increment"))
assertTrue(cur.getField(0,"column_key").include?("PRI"))
assertTrue(cur.sendQuery("drop table if exists testtable"))
print "\n"


# primary keys list
print "PRIMARY KEYS LIST: \n"
cur.sendQuery("drop table if exists testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getPrimaryKeysList("testtable",nil))
assertEqual(cur.getColumnName(0),"table")
assertEqual(cur.getColumnName(1),"non_unique")
assertEqual(cur.getColumnName(2),"key_name")
assertEqual(cur.getColumnName(3),"seq_in_index")
assertEqual(cur.getColumnName(4),"column_name")
assertEqual(cur.getColumnName(5),"collation")
assertEqual(cur.getColumnName(6),"cardinality")
assertEqual(cur.getColumnName(7),"sub_part")
assertEqual(cur.getColumnName(8),"packed")
assertEqual(cur.getColumnName(9),"null")
assertEqual(cur.getColumnName(10),"index_type")
assertEqual(cur.getColumnName(11),"comment")
assertEqual(cur.getColumnName(12),"index_comment")
assertEqual(cur.rowCount(),1)
assertTrue(cur.getField(0,"table").eql?("testtable"))
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertTrue(cur.getField(0,"column_name").eql?("col1"))
assertTrue(cur.sendQuery("drop table if exists testtable"))
print "\n"


# key and index list
print "KEY AND INDEX LIST: \n"
cur.sendQuery("drop table if exists testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getKeyAndIndexList("testtable",nil))
assertEqual(cur.getColumnName(0),"table")
assertEqual(cur.getColumnName(1),"non_unique")
assertEqual(cur.getColumnName(2),"key_name")
assertEqual(cur.getColumnName(3),"seq_in_index")
assertEqual(cur.getColumnName(4),"column_name")
assertEqual(cur.getColumnName(5),"collation")
assertEqual(cur.getColumnName(6),"cardinality")
assertEqual(cur.getColumnName(7),"sub_part")
assertEqual(cur.getColumnName(8),"packed")
assertEqual(cur.getColumnName(9),"null")
assertEqual(cur.getColumnName(10),"index_type")
assertEqual(cur.getColumnName(11),"comment")
assertEqual(cur.getColumnName(12),"index_comment")
assertEqual(cur.rowCount(),1)
assertTrue(cur.getField(0,"table").eql?("testtable"))
assertEqual(cur.getField(0,"non_unique"),"0")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertTrue(cur.getField(0,"column_name").eql?("col1"))
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"3")
assertTrue(!(cur.getField(0,"key_name").nil? || cur.getField(0,"key_name").empty?))
assertTrue(cur.sendQuery("drop table if exists testtable"))
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
assertTrue(cur.getProcedureList(nil))
assertEqual(cur.rowCount(),0)
print "\n"


# procedure parameter list
print "PROCEDURE PARAMETER LIST: \n"
assertTrue(cur.getProcedureParameterList("testproc1",nil))
assertEqual(cur.getColumnName(0),"parameter_name")
assertEqual(cur.getColumnName(1),"parameter_mode")
assertEqual(cur.getColumnName(2),"data_type")
assertEqual(cur.getColumnName(3),"character_maximum_length")
assertEqual(cur.getColumnName(4),"ordinal_position")
assertEqual(cur.rowCount(),0)
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery("select * from testtable"))
assertFalse(cur.sendQuery("select * from testtable"))
assertFalse(cur.sendQuery("select * from testtable"))
assertFalse(cur.sendQuery("select * from testtable"))
print "\n"
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
print "\n"
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
print "\n"

reportTestStatus()

exit($status)
