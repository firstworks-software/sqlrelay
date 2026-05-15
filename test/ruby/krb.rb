#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'socket'
require 'sqlrelay'
require './asserts'



service=nil
if RbConfig::CONFIG['host_os']=~/mswin|mingw|cygwin/
	service="sqlrelay/fedora24x64.firstworks.com@AD.FIRSTWORKS.COM"
end


# hostname
hostname=Socket.gethostname.split('.')[0]


# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket","","",0,1)
cur=SQLRCursor.new(con)
con.enableKerberos(service,nil,nil)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"oracle")
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
print "\n"


# transaction state
print "TRANSACTION STATE: \n"
assertEqual(con.getDefaultTransactionModel(),"implicit")
assertEqual(con.getTransactionModel(),"implicit")
assertTrue(con.getInTransaction())
assertFalse(con.getAutoCommit())
print "\n"


# bind format
print "BIND FORMAT: \n"
assertEqual(con.bindFormat(),":*")
print "\n"


# nextval format
print "NEXTVAL FORMAT: \n"
assertEqual(con.nextvalFormat(),"%s.nextval")
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
isolationlevels=["READ COMMITTED","SERIALIZABLE"]
for il in isolationlevels
	# oracle requires the isolation level to
	# be the first query of the transaction
	assertTrue(con.commit())
	# you can set the isolation level, but to get it, you have to
	# have permissions to read from sys.v_$session and
	# sys.v_$transaction
	assertTrue(con.setIsolationLevel(il))
	print "\n"
end
# reset to the default isolation level
assertTrue(con.commit())
assertTrue(con.setIsolationLevel(isolationlevels[0]))
print "\n"


# create testtable
print "CREATE TESTTABLE: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"))
print "\n"


# insert
print "INSERT: \n"
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01-JAN-2001', "+
	"	'testlong1', "+
	"	'testclob1', "+
	"	empty_blob())"))
print "\n"


# affected rows
print "AFFECTED ROWS: \n"
assertEqual(cur.affectedRows(),1)
print "\n"


# input bind by position
print "INPUT BIND BY POSITION: \n"
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	:var1, "+
	"	:var2, "+
	"	:var3, "+
	"	:var4, "+
	"	:var5, "+
	"	:var6, "+
	"	:var7)")
assertEqual(cur.countBindVariables(),7)
cur.inputBind("1",2)
cur.inputBind("2","testchar2")
cur.inputBind("3","testvarchar2")
cur.inputBindDate("4",2002,1,1,0,0,0,0,"",0)
cur.inputBind("5","testlong2")
cur.inputBindClob("6","testclob2","testclob2".to_s.bytesize)
cur.inputBindBlob("7","testblob2","testblob2".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",3)
cur.inputBind("2","testchar3")
cur.inputBind("3","testvarchar3")
cur.inputBindDate("4",2003,1,1,0,0,0,0,"",0)
cur.inputBind("5","testlong3")
cur.inputBindClob("6","testclob3","testclob3".to_s.bytesize)
cur.inputBindBlob("7","testblob3","testblob3".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by position
print "ARRAY OF INPUT BINDS BY POSITION: \n"
cur.clearBinds()
cur.inputBinds(["1","2","3","4","5"],
	["4","testchar4","testvarchar4","01-JAN-2004","testlong4"])
cur.inputBindClob("6","testclob4","testclob4".to_s.bytesize)
cur.inputBindBlob("7","testblob4","testblob4".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# input bind by position with validation
print "INPUT BIND BY POSITION WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("1",5)
cur.inputBind("2","testchar5")
cur.inputBind("3","testvarchar5")
cur.inputBindDate("4",2005,1,1,0,0,0,0,"",0)
cur.inputBind("5","testlong5")
cur.inputBindClob("6","testclob5","testclob5".to_s.bytesize)
cur.inputBindBlob("7","testblob5","testblob5".to_s.bytesize)
cur.validateBinds()
assertTrue(cur.executeQuery())
cur.clearBinds()


# input bind by name
print "INPUT BIND BY NAME: \n"
cur.clearBinds()
cur.inputBind("var1",6)
cur.inputBind("var2","testchar6")
cur.inputBind("var3","testvarchar6")
cur.inputBindDate("var4",2006,1,1,0,0,0,0,"",0)
cur.inputBind("var5","testlong6")
cur.inputBindClob("var6","testclob6","testclob6".to_s.bytesize)
cur.inputBindBlob("var7","testblob6","testblob6".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by name
print "ARRAY OF INPUT BINDS BY NAME: \n"
cur.clearBinds()
cur.inputBinds(["var1","var2","var3","var4","var5"],
	["7","testchar7","testvarchar7","01-JAN-2007","testlong7"])
cur.inputBindClob("var6","testclob7","testclob7".to_s.bytesize)
cur.inputBindBlob("var7","testblob7","testblob7".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# input bind by name with validation
print "INPUT BIND BY NAME WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("var1",8)
cur.inputBind("var2","testchar8")
cur.inputBind("var3","testvarchar8")
cur.inputBindDate("var4",2008,1,1,0,0,0,0,"",0)
cur.inputBind("var5","testlong8")
cur.inputBindClob("var6","testclob8","testclob8".to_s.bytesize)
cur.inputBindBlob("var7","testblob8","testblob8".to_s.bytesize)
cur.inputBind("var9","junkvalue")
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# select
print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),7)
print "\n"


# column names
print "COLUMN NAMES: \n"
assertEqual(cur.getColumnName(0),"TESTNUMBER")
assertEqual(cur.getColumnName(1),"TESTCHAR")
assertEqual(cur.getColumnName(2),"TESTVARCHAR")
assertEqual(cur.getColumnName(3),"TESTDATE")
assertEqual(cur.getColumnName(4),"TESTLONG")
assertEqual(cur.getColumnName(5),"TESTCLOB")
assertEqual(cur.getColumnName(6),"TESTBLOB")
cols=cur.getColumnNames()
assertEqual(cols[0],"TESTNUMBER")
assertEqual(cols[1],"TESTCHAR")
assertEqual(cols[2],"TESTVARCHAR")
assertEqual(cols[3],"TESTDATE")
assertEqual(cols[4],"TESTLONG")
assertEqual(cols[5],"TESTCLOB")
assertEqual(cols[6],"TESTBLOB")
print "\n"


# column types
print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"NUMBER")
assertEqual(cur.getColumnType("TESTNUMBER"),"NUMBER")
assertEqual(cur.getColumnType(1),"CHAR")
assertEqual(cur.getColumnType("TESTCHAR"),"CHAR")
assertEqual(cur.getColumnType(2),"VARCHAR2")
assertEqual(cur.getColumnType("TESTVARCHAR"),"VARCHAR2")
assertEqual(cur.getColumnType(3),"DATE")
assertEqual(cur.getColumnType("TESTDATE"),"DATE")
assertEqual(cur.getColumnType(4),"LONG")
assertEqual(cur.getColumnType("TESTLONG"),"LONG")
assertEqual(cur.getColumnType(5),"CLOB")
assertEqual(cur.getColumnType("TESTCLOB"),"CLOB")
assertEqual(cur.getColumnType(6),"BLOB")
assertEqual(cur.getColumnType("TESTBLOB"),"BLOB")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),22)
assertEqual(cur.getColumnLength("TESTNUMBER"),22)
assertEqual(cur.getColumnLength(1),40)
assertEqual(cur.getColumnLength("TESTCHAR"),40)
assertEqual(cur.getColumnLength(2),40)
assertEqual(cur.getColumnLength("TESTVARCHAR"),40)
assertEqual(cur.getColumnLength(3),7)
assertEqual(cur.getColumnLength("TESTDATE"),7)
assertEqual(cur.getColumnLength(4),0)
assertEqual(cur.getColumnLength("TESTLONG"),0)
assertEqual(cur.getColumnLength(5),0)
assertEqual(cur.getColumnLength("TESTCLOB"),0)
assertEqual(cur.getColumnLength(6),0)
assertEqual(cur.getColumnLength("TESTBLOB"),0)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest("TESTNUMBER"),1)
assertEqual(cur.getLongest(1),40)
assertEqual(cur.getLongest("TESTCHAR"),40)
assertEqual(cur.getLongest(2),12)
assertEqual(cur.getLongest("TESTVARCHAR"),12)
assertEqual(cur.getLongest(3),9)
assertEqual(cur.getLongest("TESTDATE"),9)
assertEqual(cur.getLongest(4),9)
assertEqual(cur.getLongest("TESTLONG"),9)
assertEqual(cur.getLongest(5),9)
assertEqual(cur.getLongest("TESTCLOB"),9)
assertEqual(cur.getLongest(6),9)
assertEqual(cur.getLongest("TESTBLOB"),9)
print "\n"


# row count
print "ROW COUNT: \n"
assertEqual(cur.rowCount(),8)
print "\n"


# total rows
print "TOTAL ROWS: \n"
assertEqual(cur.totalRows(),0)
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
assertEqual(cur.getField(0,1),"testchar1                               ")
assertEqual(cur.getField(0,2),"testvarchar1")
assertEqual(cur.getField(0,3),"01-JAN-01")
assertEqual(cur.getField(0,4),"testlong1")
assertEqual(cur.getField(0,5),"testclob1")
assertEqual(cur.getField(0,6),"")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"testchar8                               ")
assertEqual(cur.getField(7,2),"testvarchar8")
assertEqual(cur.getField(7,3),"01-JAN-08")
assertEqual(cur.getField(7,4),"testlong8")
assertEqual(cur.getField(7,5),"testclob8")
assertEqual(cur.getField(7,6),"testblob8")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),40)
assertEqual(cur.getFieldLength(0,2),12)
assertEqual(cur.getFieldLength(0,3),9)
assertEqual(cur.getFieldLength(0,4),9)
assertEqual(cur.getFieldLength(0,5),9)
assertEqual(cur.getFieldLength(0,6),0)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),40)
assertEqual(cur.getFieldLength(7,2),12)
assertEqual(cur.getFieldLength(7,3),9)
assertEqual(cur.getFieldLength(7,4),9)
assertEqual(cur.getFieldLength(7,5),9)
assertEqual(cur.getFieldLength(7,6),9)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"TESTNUMBER"),"1")
assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                               ")
assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1")
assertEqual(cur.getField(0,"TESTDATE"),"01-JAN-01")
assertEqual(cur.getField(0,"TESTLONG"),"testlong1")
assertEqual(cur.getField(0,"TESTCLOB"),"testclob1")
assertEqual(cur.getField(0,"TESTBLOB"),"")
print "\n"
assertEqual(cur.getField(7,"TESTNUMBER"),"8")
assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                               ")
assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8")
assertEqual(cur.getField(7,"TESTDATE"),"01-JAN-08")
assertEqual(cur.getField(7,"TESTLONG"),"testlong8")
assertEqual(cur.getField(7,"TESTCLOB"),"testclob8")
assertEqual(cur.getField(7,"TESTBLOB"),"testblob8")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"TESTNUMBER"),1)
assertEqual(cur.getFieldLength(0,"TESTCHAR"),40)
assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12)
assertEqual(cur.getFieldLength(0,"TESTDATE"),9)
assertEqual(cur.getFieldLength(0,"TESTLONG"),9)
assertEqual(cur.getFieldLength(0,"TESTCLOB"),9)
assertEqual(cur.getFieldLength(0,"TESTBLOB"),0)
print "\n"
assertEqual(cur.getFieldLength(7,"TESTNUMBER"),1)
assertEqual(cur.getFieldLength(7,"TESTCHAR"),40)
assertEqual(cur.getFieldLength(7,"TESTVARCHAR"),12)
assertEqual(cur.getFieldLength(7,"TESTDATE"),9)
assertEqual(cur.getFieldLength(7,"TESTLONG"),9)
assertEqual(cur.getFieldLength(7,"TESTCLOB"),9)
assertEqual(cur.getFieldLength(7,"TESTBLOB"),9)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"testchar1                               ")
assertEqual(fields[2],"testvarchar1")
assertEqual(fields[3],"01-JAN-01")
assertEqual(fields[4],"testlong1")
assertEqual(fields[5],"testclob1")
assertEqual(fields[6],"")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],40)
assertEqual(fieldlens[2],12)
assertEqual(fieldlens[3],9)
assertEqual(fieldlens[4],9)
assertEqual(fieldlens[5],9)
assertEqual(fieldlens[6],0)
print "\n"


# result set buffer size
print "RESULT SET BUFFER SIZE: \n"
assertEqual(cur.getResultSetBufferSize(),0)
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
assertEqual(cur.getResultSetBufferSize(),2)
print "\n"
assertEqual(cur.firstRowIndex(),0)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),2)
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(1,0),"2")
assertEqual(cur.getField(2,0),"3")
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
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
assertEqual(cur.getColumnName(0),nil)
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),nil)
cur.getColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
assertEqual(cur.getColumnName(0),"TESTNUMBER")
assertEqual(cur.getColumnLength(0),22)
assertEqual(cur.getColumnType(0),"NUMBER")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
print "\n"


# column count for cached result set
print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),7)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
assertEqual(cur.getColumnName(0),"TESTNUMBER")
assertEqual(cur.getColumnName(1),"TESTCHAR")
assertEqual(cur.getColumnName(2),"TESTVARCHAR")
assertEqual(cur.getColumnName(3),"TESTDATE")
assertEqual(cur.getColumnName(4),"TESTLONG")
assertEqual(cur.getColumnName(5),"TESTCLOB")
assertEqual(cur.getColumnName(6),"TESTBLOB")
cols=cur.getColumnNames()
assertEqual(cols[0],"TESTNUMBER")
assertEqual(cols[1],"TESTCHAR")
assertEqual(cols[2],"TESTVARCHAR")
assertEqual(cols[3],"TESTDATE")
assertEqual(cols[4],"TESTLONG")
assertEqual(cols[5],"TESTCLOB")
assertEqual(cols[6],"TESTBLOB")
print "\n"


# cached result set with result set buffer size
print "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# reset transaction state
print "RESET TRANSACTION STATE: \n"
assertTrue(con.commit())
assertEqual(con.getTransactionModel(),"implicit")
assertFalse(con.getAutoCommit())
print "\n"


# transaction behavior - implicit
print "TRANSACTION BEHAVIOR - implicit: \n"
assertTrue(con.setTransactionModel("implicit"))
assertEqual(con.getTransactionModel(),"implicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket","","",0,1)
secondcur=SQLRCursor.new(secondcon)
secondcon.enableKerberos(service,nil,nil)
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
assertEqual(con.getTransactionModel(),"implicit")
assertFalse(con.getAutoCommit())
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual")
cur.substitution("var1","$(var11)")
cur.substitution("var2","$(var21)")
cur.substitution("var3","$(var31)")
cur.substitution("var11","$(var111)")
cur.substitution("var21","$(var211)")
cur.substitution("var31","$(var311)")
cur.substitution("var111",1)
cur.substitution("var211","hello")
cur.substitution("var311",10.5556,6,4)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# array substitutions
print "ARRAY SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),$(var2),$(var3) from dual")
cur.substitutions(["var1","var2","var3"],[1,2,3])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"2")
assertEqual(cur.getField(0,2),"3")
print "\n"
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)' from dual")
cur.substitutions(["var1","var2","var3"],["hi","hello","bye"])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"hi")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"bye")
print "\n"
cur.prepareQuery("select $(var1),$(var2),$(var3) from dual")
cur.substitutions(["var1","var2","var3"],
			[10.55,10.556,10.5556],[4,5,6],[2,3,4])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"10.55")
assertEqual(cur.getField(0,1),"10.556")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# nulls as nulls
print "NULLS AS NULLS: \n"
cur.getNullsAsNils()
assertTrue(cur.sendQuery("select NULL,1,NULL from dual"))
assertEqual(cur.getField(0,0),nil)
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select NULL,1,NULL from dual"))
assertEqual(cur.getField(0,0),"")
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),"")
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
largebuffer="C" * 8192
cur.inputBindClob("clobval",largebuffer,largebuffer.to_s.bytesize)
cur.inputBindBlob("blobval",largebuffer,largebuffer.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getFieldLength(0,"TESTCLOB"),8192)
assertEqual(cur.getField(0,"TESTCLOB"),largebuffer)
assertEqual(cur.getFieldLength(0,"TESTBLOB"),8192)
assertEqualLen(cur.getField(0,"TESTBLOB"),largebuffer,8192)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# output bind by position
print "OUTPUT BIND BY POSITION: \n"
cur.getNullsAsNils()
cur.prepareQuery(
	"begin "+
	"	:numvar:=1; "+
	"	:stringvar:='hello'; "+
	"	:floatvar:=2.5; "+
	"	:datevar:='03-FEB-2001'; "+
	"	:nullvar:=null; "+
	"end;")
assertEqual(cur.countBindVariables(),5)
cur.defineOutputBindInteger("1")
cur.defineOutputBindString("2",10)
cur.defineOutputBindDouble("3")
cur.defineOutputBindDate("4")
cur.defineOutputBindString("5",10)
assertTrue(cur.executeQuery())
numvar=cur.getOutputBindInteger("1")
stringvar=cur.getOutputBindString("2")
floatvar=cur.getOutputBindDouble("3")
year=cur.getOutputBindDateYear("4")
month=cur.getOutputBindDateMonth("4")
day=cur.getOutputBindDateDay("4")
hour=cur.getOutputBindDateHour("4")
minute=cur.getOutputBindDateMinute("4")
second=cur.getOutputBindDateSecond("4")
microsecond=cur.getOutputBindDateMicrosecond("4")
tz=cur.getOutputBindDateTz("4")
isnegative=cur.getOutputBindDateIsNegative("4")
nullvar=cur.getOutputBindString("5")
assertEqual(numvar,1)
assertEqual(stringvar,"hello")
assertEqual(floatvar,2.5)
assertEqual(year,2001)
assertEqual(month,2)
assertEqual(day,3)
assertEqual(hour,0)
assertEqual(minute,0)
assertEqual(second,0)
assertEqual(microsecond,0)
assertEqual(tz,"")
assertFalse(isnegative)
assertEqual(nullvar,nil)
cur.getNullsAsEmptyStrings()
print "\n"


# output bind by name
print "OUTPUT BIND BY NAME: \n"
cur.getNullsAsNils()
cur.clearBinds()
cur.defineOutputBindInteger("numvar")
cur.defineOutputBindString("stringvar",10)
cur.defineOutputBindDouble("floatvar")
cur.defineOutputBindDate("datevar")
cur.defineOutputBindString("nullvar",10)
assertTrue(cur.executeQuery())
numvar=cur.getOutputBindInteger("numvar")
stringvar=cur.getOutputBindString("stringvar")
floatvar=cur.getOutputBindDouble("floatvar")
year=cur.getOutputBindDateYear("datevar")
month=cur.getOutputBindDateMonth("datevar")
day=cur.getOutputBindDateDay("datevar")
hour=cur.getOutputBindDateHour("datevar")
minute=cur.getOutputBindDateMinute("datevar")
second=cur.getOutputBindDateSecond("datevar")
microsecond=cur.getOutputBindDateMicrosecond("datevar")
tz=cur.getOutputBindDateTz("datevar")
isnegative=cur.getOutputBindDateIsNegative("datevar")
nullvar=cur.getOutputBindString("nullvar")
assertEqual(numvar,1)
assertEqual(stringvar,"hello")
assertEqual(floatvar,2.5)
assertEqual(year,2001)
assertEqual(month,2)
assertEqual(day,3)
assertEqual(hour,0)
assertEqual(minute,0)
assertEqual(second,0)
assertEqual(microsecond,0)
assertEqual(tz,"")
assertFalse(isnegative)
assertEqual(nullvar,nil)
cur.getNullsAsEmptyStrings()
print "\n"


# output bind by name with validation
print "OUTPUT BIND BY NAME WITH VALIDATION: \n"
cur.getNullsAsNils()
cur.clearBinds()
cur.defineOutputBindInteger("numvar")
cur.defineOutputBindString("stringvar",10)
cur.defineOutputBindDouble("floatvar")
cur.defineOutputBindDate("datevar")
cur.defineOutputBindString("nullvar",10)
cur.defineOutputBindString("dummyvar",10)
cur.validateBinds()
assertTrue(cur.executeQuery())
numvar=cur.getOutputBindInteger("numvar")
stringvar=cur.getOutputBindString("stringvar")
floatvar=cur.getOutputBindDouble("floatvar")
year=cur.getOutputBindDateYear("datevar")
month=cur.getOutputBindDateMonth("datevar")
day=cur.getOutputBindDateDay("datevar")
hour=cur.getOutputBindDateHour("datevar")
minute=cur.getOutputBindDateMinute("datevar")
second=cur.getOutputBindDateSecond("datevar")
microsecond=cur.getOutputBindDateMicrosecond("datevar")
tz=cur.getOutputBindDateTz("datevar")
isnegative=cur.getOutputBindDateIsNegative("datevar")
nullvar=cur.getOutputBindString("nullvar")
assertEqual(numvar,1)
assertEqual(stringvar,"hello")
assertEqual(floatvar,2.5)
assertEqual(year,2001)
assertEqual(month,2)
assertEqual(day,3)
assertEqual(hour,0)
assertEqual(minute,0)
assertEqual(second,0)
assertEqual(microsecond,0)
assertEqual(tz,"")
assertFalse(isnegative)
assertEqual(nullvar,nil)
cur.getNullsAsEmptyStrings()
print "\n"


# lob output bind
print "LOB OUTPUT BIND: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)"))
cur.prepareQuery("insert into testtable values ('hello',:var1)")
cur.inputBindBlob("var1","hello","hello".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.prepareQuery(
	"begin "+
	"	select testclob into :clobvar from testtable; "+
	"	select testblob into :blobvar from testtable; "+
	"end;")
cur.defineOutputBindClob("clobvar")
cur.defineOutputBindBlob("blobvar")
assertTrue(cur.executeQuery())
clobvar=cur.getOutputBindClob("clobvar")
clobvarlength=cur.getOutputBindLength("clobvar")
blobvar=cur.getOutputBindBlob("blobvar")
blobvarlength=cur.getOutputBindLength("blobvar")
assertEqualLen(clobvar,"hello",5)
assertEqual(clobvarlength,5)
assertEqualLen(blobvar,"hello",5)
assertEqual(blobvarlength,5)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# long output bind
print "LONG OUTPUT BIND: \n"
largebuffer="C" * 8192
query="begin :bindval:='" + largebuffer + "'; end;"
cur.prepareQuery(query)
cur.defineOutputBindString("bindval",8192)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindLength("bindval"),8192)
assertEqual(cur.getOutputBindString("bindval"),largebuffer)
print "\n"


# negative input bind
print "NEGATIVE INPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery("create table testtable (testval number)")
cur.prepareQuery("insert into testtable values (:testval)")
cur.inputBind("testval",-1)
assertTrue(cur.executeQuery())
cur.sendQuery("select testval from testtable")
assertEqual(cur.getField(0,"TESTVAL"),"-1")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# bind validation
print "BIND VALIDATION: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	col1 varchar2(20), "+
	"	col2 varchar2(20), "+
	"	col3 varchar2(20))")
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
cur.prepareQuery(
	"begin "+
	"	:out:= :in; "+
	"end;")
cur.inputBind("in",1)
cur.defineOutputBindInteger("out")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("out"),1)
cur.inputBind("in",2)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("out"),2)
cur.inputBind("in",3)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("out"),3)
print "\n"


# reexecute
print "REEXECUTE: \n"
cur.prepareQuery("select 1 from dual")
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.prepareQuery("select :var from dual")
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
print "STORED PROCEDURE RETURNING NO VALUE: \n"
cur.sendQuery("drop function testproc")
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create or replace "+
	"procedure testproc("+
	"	in1 in number, "+
	"	in2 in number, "+
	"	in3 in varchar2) "+
	"is "+
	"begin "+
	"	return; "+
	"end;"))
cur.prepareQuery("begin testproc(:in1,:in2,:in3); end;")
cur.inputBind("in1",1)
cur.inputBind("in2",1.1,2,1)
cur.inputBind("in3","hello")
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("drop procedure testproc"))
print "\n"


# stored procedure returning single value
print "STORED PROCEDURE RETURNING SINGLE VALUE: \n"
cur.sendQuery("drop function testproc")
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create or replace "+
	"function testproc("+
	"	in1 in number, "+
	"	in2 in number, "+
	"	in3 in varchar2) "+
	"	return number "+
	"is "+
	"begin "+
	"	return in1; "+
	"end;"))
cur.prepareQuery("select testproc(:in1,:in2,:in3) from dual")
cur.inputBind("in1",1)
cur.inputBind("in2",1.1,2,1)
cur.inputBind("in3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
cur.prepareQuery(
	"begin "+
	"	:out1:=testproc(:in1,:in2,:in3); "+
	"end;")
cur.inputBind("in1",1)
cur.inputBind("in2",1.1,2,1)
cur.inputBind("in3","hello")
cur.defineOutputBindInteger("out1")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("out1"),1)
assertTrue(cur.sendQuery("drop function testproc"))
print "\n"


# stored procedure returning multiple values
print "STORED PROCEDURE RETURNING MULTIPLE VALUES: \n"
cur.sendQuery("drop function testproc")
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create or replace "+
	"procedure testproc("+
	"	in1 in number, "+
	"	in2 in number, "+
	"	in3 in varchar2, "+
	"	out1 out number, "+
	"	out2 out number, "+
	"	out3 out varchar2) "+
	"is "+
	"begin "+
	"	out1:=in1; "+
	"	out2:=in2; "+
	"	out3:=in3; "+
	"end;"))
cur.prepareQuery(
	"begin "+
	"	testproc(:in1,:in2,:in3,:out1,:out2,:out3); "+
	"end;")
cur.inputBind("in1",1)
cur.inputBind("in2",1.1,2,1)
cur.inputBind("in3","hello")
cur.defineOutputBindInteger("out1")
cur.defineOutputBindDouble("out2")
cur.defineOutputBindString("out3",20)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("out1"),1)
assertEqual(cur.getOutputBindDouble("out2"),1.1)
assertEqual(cur.getOutputBindString("out3"),"hello")
assertTrue(cur.sendQuery("drop procedure testproc"))
print "\n"


# stored procedure returning result set
print "STORED PROCEDURE RETURNING RESULT SET: \n"
cur.sendQuery("drop package types")
cur.sendQuery("drop function testproc")
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create or replace package types is "+
	"	type cursorType is ref cursor; "+
	"end;"))
assertTrue(cur.sendQuery(
	"create or replace "+
	"function testproc(value in number) "+
	"	return types.cursortype "+
	"is "+
	"	l_cursor    types.cursorType; "+
	"begin "+
	"	open l_cursor for "+
	"		select "+
	"			* "+
	"		from "+
	"			( "+
	"			select 1 as testnumber from dual "+
	"			union "+
	"			select 2 as testnumber from dual "+
	"			union "+
	"			select 3 as testnumber from dual "+
	"			union "+
	"			select 4 as testnumber from dual "+
	"			union "+
	"			select 5 as testnumber from dual "+
	"			union "+
	"			select 6 as testnumber from dual "+
	"			union "+
	"			select 7 as testnumber from dual "+
	"			union "+
	"			select 8 as testnumber from dual "+
	"			) "+
	"		where "+
	"			testnumber>value; "+
	"	return l_cursor; "+
	"end;"))
cur.prepareQuery(
	"begin "+
	"	:curs1:=testproc(5); "+
	"	:curs2:=testproc(0); "+
	"end;")
cur.defineOutputBindCursor("curs1")
cur.defineOutputBindCursor("curs2")
assertTrue(cur.executeQuery())
bindcur1=cur.getOutputBindCursor("curs1")
assertTrue(bindcur1.fetchFromBindCursor())
assertEqual(bindcur1.getField(0,0),"6")
assertEqual(bindcur1.getField(1,0),"7")
assertEqual(bindcur1.getField(2,0),"8")
bindcur2=cur.getOutputBindCursor("curs2")
assertTrue(bindcur2.fetchFromBindCursor())
assertEqual(bindcur2.getField(0,0),"1")
assertEqual(bindcur2.getField(1,0),"2")
assertEqual(bindcur2.getField(2,0),"3")
assertTrue(cur.sendQuery("drop function testproc"))
assertTrue(cur.sendQuery("drop package types"))
print "\n"


# temporary tables
print "TEMPORARY TABLES: \n"
cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete")
cur.substitution("HOSTNAME",hostname)
cur.executeQuery()
cur.prepareQuery(
	"create global temporary table $(HOSTNAME)_temptabledelete ( "+
	"	col1 number "+
	") on commit delete rows")
cur.substitution("HOSTNAME",hostname)
cur.executeQuery()
cur.prepareQuery("insert into $(HOSTNAME)_temptabledelete values (1)")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
cur.prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertTrue(con.commit())
cur.prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"0")
cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete")
cur.substitution("HOSTNAME",hostname)
cur.executeQuery()
print "\n"
cur.prepareQuery("truncate table $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
cur.executeQuery()
cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
cur.executeQuery()
cur.prepareQuery(
	"create global temporary table $(HOSTNAME)_temptablepreserve ("+
	"	col1 number "+
	") on commit preserve rows")
cur.substitution("HOSTNAME",hostname)
cur.executeQuery()
cur.prepareQuery(
	"insert into "+
	"	$(HOSTNAME)_temptablepreserve "+
	"values (1)")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertTrue(con.commit())
cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
con.endSession()
print "\n"
cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"0")
cur.prepareQuery("truncate table $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
sleep(2)
cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
assertTrue(cur.executeQuery())
cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
cur.substitution("HOSTNAME",hostname)
assertFalse(cur.executeQuery())
print "\n"


# encoded binary data
print "ENCODED BINARY DATA: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 blob)"))
buffer=(0..255).map { |j| j.chr }.join
hexstr=buffer.bytes.map { |b| "%02x" % b }.join
querystr="insert into testtable values ('" + hexstr + "')"
assertTrue(cur.sendQuery(querystr))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),256)
assertEqualLen(cur.getField(0,0),buffer,256)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes
print "QUOTES: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar2(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('''''')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"''")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# last insert id
# oracle doesn't support auto-increment


# database is schema
print "DATABASE IS SCHEMA: \n"
assertTrue(con.getDatabaseIsSchema())
print "\n"


# catalog list
print "CATALOG LIST: \n"
assertTrue(cur.getCatalogList(nil))
assertEqual(cur.getColumnName(0),"Database")
assertEqual(cur.rowCount(),0)
print "\n"


# schema list
print "SCHEMA LIST: \n"
assertTrue(cur.getSchemaList(nil))
assertEqual(cur.getColumnName(0),"Database")
found=false
for i in 0..cur.rowCount()-1
	if cur.getField(i,"Database").to_s.downcase==hostname.downcase
		found=true
		break
	end
end
assertTrue(found)
print "\n"


# table type list
print "TABLE TYPE LIST: \n"
assertTrue(cur.getTableTypeList())
assertEqual(cur.getColumnName(0),"table_type")
assertEqual(cur.getField(0,"table_type"),"SYNONYM")
assertEqual(cur.getField(1,"table_type"),"TABLE")
assertEqual(cur.getField(2,"table_type"),"VIEW")
print "\n"


# table list
print "TABLE LIST: \n"
cur.sendQuery("drop table testtable1")
cur.sendQuery("drop table testtable2")
cur.sendQuery("drop table testtable3")
cur.sendQuery("drop table testtable4")
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(cur.getTableList(nil))
counter=0
for i in 0..cur.rowCount()-1
	name=cur.getField(i,"Tables_in_xxx")
	if name=="TESTTABLE1" || name=="TESTTABLE2" ||
		name=="TESTTABLE3" || name=="TESTTABLE4"
		counter=counter+1
	end
end
assertEqual(counter,4)
assertTrue(cur.sendQuery("drop table testtable1"))
assertTrue(cur.sendQuery("drop table testtable2"))
assertTrue(cur.sendQuery("drop table testtable3"))
assertTrue(cur.sendQuery("drop table testtable4"))
print "\n"


# type info list
print "TYPE INFO LIST: \n"
assertTrue(cur.getTypeInfoList("number"))
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
assertEqual(cur.getField(0,"type_name"),"NUMBER")
assertEqual(cur.getField(0,"data_type"),"-7")
assertEqual(cur.getField(0,"precision"),"1")
assertEqual(cur.getField(0,"local_type_name"),"NUMBER")
assertTrue(cur.getTypeInfoList("char"))
assertEqual(cur.getField(0,"type_name"),"CHAR")
assertEqual(cur.getField(0,"data_type"),"1")
assertEqual(cur.getField(0,"precision"),"2000")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar2"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR2")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"32767")
assertEqual(cur.getField(0,"local_type_name"),"VARCHAR2")
assertTrue(cur.getTypeInfoList("date"))
assertEqual(cur.getField(0,"type_name"),"DATE")
assertEqual(cur.getField(0,"data_type"),"92")
assertEqual(cur.getField(0,"precision"),"7")
assertEqual(cur.getField(0,"local_type_name"),"DATE")
print "\n"


# column list
print "COLUMN LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
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
assertEqual(cur.getField(0,"column_name"),"TESTNUMBER")
assertEqual(cur.getField(1,"column_name"),"TESTCHAR")
assertEqual(cur.getField(2,"column_name"),"TESTVARCHAR")
assertEqual(cur.getField(3,"column_name"),"TESTDATE")
assertEqual(cur.getField(4,"column_name"),"TESTLONG")
assertEqual(cur.getField(5,"column_name"),"TESTCLOB")
assertEqual(cur.getField(6,"column_name"),"TESTBLOB")
assertEqual(cur.getField(0,"data_type"),"NUMBER")
assertEqual(cur.getField(1,"data_type"),"CHAR")
assertEqual(cur.getField(2,"data_type"),"VARCHAR2")
assertEqual(cur.getField(3,"data_type"),"DATE")
assertEqual(cur.getField(4,"data_type"),"LONG")
assertEqual(cur.getField(5,"data_type"),"CLOB")
assertEqual(cur.getField(6,"data_type"),"BLOB")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# column list - auto_increment, primary key
# oracle doesn't support auto_increment
print "COLUMN LIST - auto_increment, primary key: \n"
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 number primary key, "+
	"	col2 number)"))
assertTrue(cur.getColumnList("testtable",nil))
assertTrue(cur.getField(0,"column_key").to_s.include?("PRI"))
assertFalse(cur.getField(1,"column_key").to_s.include?("PRI"))
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# primary keys list
print "PRIMARY KEYS LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 number primary key, "+
	"	col2 number)"))
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
assertEqual(cur.getField(0,"table"),"TESTTABLE")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"COL1")
assertTrue(!cur.getField(0,"key_name").nil? && cur.getField(0,"key_name")!="")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# key and index list
print "KEY AND INDEX LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 number primary key, "+
	"	col2 number)"))
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
assertEqual(cur.getField(0,"table"),"TESTTABLE")
assertEqual(cur.getField(0,"non_unique"),"0")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"COL1")
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"3")
assertTrue(!cur.getField(0,"key_name").nil? && cur.getField(0,"key_name")!="")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
cur.sendQuery("drop procedure testproc1")
cur.sendQuery("drop procedure testproc2")
cur.sendQuery("drop procedure testproc3")
cur.sendQuery("drop procedure testproc4")
assertTrue(cur.sendQuery(
	"create procedure testproc1("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"))
assertTrue(cur.sendQuery(
	"create procedure testproc2("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"))
assertTrue(cur.sendQuery(
	"create procedure testproc3("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"))
assertTrue(cur.sendQuery(
	"create procedure testproc4("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"))
assertTrue(cur.getProcedureList(nil))
counter=0
for i in 0..cur.rowCount()-1
	name=cur.getField(i,"routine_name")
	if name=="TESTPROC1" || name=="TESTPROC2" ||
		name=="TESTPROC3" || name=="TESTPROC4"
		counter=counter+1
	end
end
assertEqual(counter,4)
print "\n"


# procedure parameter list
print "PROCEDURE PARAMETER LIST: \n"
assertTrue(cur.getProcedureParameterList("testproc1",nil))
assertEqual(cur.getColumnName(0),"parameter_name")
assertEqual(cur.getColumnName(1),"parameter_mode")
assertEqual(cur.getColumnName(2),"data_type")
assertEqual(cur.getColumnName(3),"character_maximum_length")
assertEqual(cur.getColumnName(4),"ordinal_position")
assertEqual(cur.rowCount(),4)
assertEqual(cur.getField(0,"parameter_name"),"IN1")
assertEqual(cur.getField(0,"parameter_mode"),"1")
assertEqual(cur.getField(0,"data_type"),"NUMBER")
assertEqual(cur.getField(0,"ordinal_position"),"1")
assertEqual(cur.getField(1,"parameter_name"),"IN2")
assertEqual(cur.getField(1,"parameter_mode"),"1")
assertEqual(cur.getField(1,"data_type"),"CHAR")
assertEqual(cur.getField(1,"ordinal_position"),"2")
assertEqual(cur.getField(2,"parameter_name"),"IN3")
assertEqual(cur.getField(2,"parameter_mode"),"1")
assertEqual(cur.getField(2,"data_type"),"VARCHAR2")
assertEqual(cur.getField(2,"ordinal_position"),"3")
assertEqual(cur.getField(3,"parameter_name"),"IN4")
assertEqual(cur.getField(3,"parameter_mode"),"1")
assertEqual(cur.getField(3,"data_type"),"DATE")
assertEqual(cur.getField(3,"ordinal_position"),"4")
assertTrue(cur.sendQuery("drop procedure testproc1"))
assertTrue(cur.sendQuery("drop procedure testproc2"))
assertTrue(cur.sendQuery("drop procedure testproc3"))
assertTrue(cur.sendQuery("drop procedure testproc4"))
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery("select * from testtable order by testnumber"))
assertFalse(cur.sendQuery("select * from testtable order by testnumber"))
assertFalse(cur.sendQuery("select * from testtable order by testnumber"))
assertFalse(cur.sendQuery("select * from testtable order by testnumber"))
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
