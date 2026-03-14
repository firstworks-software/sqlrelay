#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'sqlrelay'
require './asserts'




# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
cur=SQLRCursor.new(con)

# get database type


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"sqlite")
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
isolationlevels=["0","1"]
for il in isolationlevels
	assertTrue(con.setIsolationLevel(il))
	assertEqual(con.getIsolationLevel(),il)
	print "\n"
end
# reset to the default isolation level
assertTrue(con.setIsolationLevel(isolationlevels[0]))
print "\n"

# drop existing table
cur.sendQuery("begin")
cur.sendQuery("drop table testtable")
con.commit()

# create a new table


# create temptable
print "CREATE TEMPTABLE: \n"
cur.sendQuery("begin")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testfloat float, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40))"))
con.commit()
print "\n"


# insert
print "INSERT: \n"
cur.sendQuery("begin")
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1.1, "+
	"	'testchar1', "+
	"	'testvarchar1')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	2, "+
	"	2.2, "+
	"	'testchar2', "+
	"	'testvarchar2')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	3, "+
	"	3.3, "+
	"	'testchar3', "+
	"	'testvarchar3')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	4, "+
	"	4.4, "+
	"	'testchar4', "+
	"	'testvarchar4')"))
print "\n"


# affected rows
print "AFFECTED ROWS: \n"
assertEqual(cur.affectedRows(),1)
print "\n"


# bind by name
print "BIND BY NAME: \n"
cur.prepareQuery("insert into testtable values (:var1,:var2,:var3,:var4)")
assertEqual(cur.countBindVariables(),4)
cur.inputBind("var1",5)
cur.inputBind("var2",5.5,4,1)
cur.inputBind("var3","testchar5")
cur.inputBind("var4","testvarchar5")
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("var1",6)
cur.inputBind("var2",6.6,4,1)
cur.inputBind("var3","testchar6")
cur.inputBind("var4","testvarchar6")
assertTrue(cur.executeQuery())
print "\n"


# array of binds by name
print "ARRAY OF BINDS BY NAME: \n"
cur.clearBinds()
cur.inputBinds(["var1","var2","var3","var4"],
		[7,7.7,"testchar7","testvarchar7"],[0,4,0,0],[0,1,0,0])
assertTrue(cur.executeQuery())
print "\n"


# bind by name with validation
print "BIND BY NAME WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("var1",8)
cur.inputBind("var2",8.8,4,1)
cur.inputBind("var3","testchar8")
cur.inputBind("var4","testvarchar8")
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# select
print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),4)
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
assertEqual(cur.getColumnType(0),"INTEGER")
assertEqual(cur.getColumnType('testint'),"INTEGER")
assertEqual(cur.getColumnType(1),"FLOAT")
assertEqual(cur.getColumnType('testfloat'),"FLOAT")
assertEqual(cur.getColumnType(2),"STRING")
assertEqual(cur.getColumnType('testchar'),"STRING")
assertEqual(cur.getColumnType(3),"STRING")
assertEqual(cur.getColumnType('testvarchar'),"STRING")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnLength('testint'),0)
assertEqual(cur.getColumnLength(1),0)
assertEqual(cur.getColumnLength('testfloat'),0)
assertEqual(cur.getColumnLength(2),0)
assertEqual(cur.getColumnLength('testchar'),0)
assertEqual(cur.getColumnLength(3),0)
assertEqual(cur.getColumnLength('testvarchar'),0)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest('testint'),1)
assertEqual(cur.getLongest(1),3)
assertEqual(cur.getLongest('testfloat'),3)
assertEqual(cur.getLongest(2),9)
assertEqual(cur.getLongest('testchar'),9)
assertEqual(cur.getLongest(3),12)
assertEqual(cur.getLongest('testvarchar'),12)
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
assertEqual(cur.getField(0,1),"1.1")
assertEqual(cur.getField(0,2),"testchar1")
assertEqual(cur.getField(0,3),"testvarchar1")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8.8")
assertEqual(cur.getField(7,2),"testchar8")
assertEqual(cur.getField(7,3),"testvarchar8")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),3)
assertEqual(cur.getFieldLength(0,2),9)
assertEqual(cur.getFieldLength(0,3),12)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),3)
assertEqual(cur.getFieldLength(7,2),9)
assertEqual(cur.getFieldLength(7,3),12)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"testint"),"1")
assertEqual(cur.getField(0,"testfloat"),"1.1")
assertEqual(cur.getField(0,"testchar"),"testchar1")
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
print "\n"
assertEqual(cur.getField(7,"testint"),"8")
assertEqual(cur.getField(7,"testfloat"),"8.8")
assertEqual(cur.getField(7,"testchar"),"testchar8")
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"testint"),1)
assertEqual(cur.getFieldLength(0,"testfloat"),3)
assertEqual(cur.getFieldLength(0,"testchar"),9)
assertEqual(cur.getFieldLength(0,"testvarchar"),12)
print "\n"
assertEqual(cur.getFieldLength(7,"testint"),1)
assertEqual(cur.getFieldLength(7,"testfloat"),3)
assertEqual(cur.getFieldLength(7,"testchar"),9)
assertEqual(cur.getFieldLength(7,"testvarchar"),12)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1.1")
assertEqual(fields[2],"testchar1")
assertEqual(fields[3],"testvarchar1")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],3)
assertEqual(fieldlens[2],9)
assertEqual(fieldlens[3],12)
print "\n"


# fields by hash
print "FIELDS BY HASH: \n"
fields=cur.getRowHash(0)
assertEqual(fields["testint"],"1")
assertEqual(fields["testfloat"],"1.1")
assertEqual(fields["testchar"],"testchar1")
assertEqual(fields["testvarchar"],"testvarchar1")
print "\n"
fields=cur.getRowHash(7)
assertEqual(fields["testint"],"8")
assertEqual(fields["testfloat"],"8.8")
assertEqual(fields["testchar"],"testchar8")
assertEqual(fields["testvarchar"],"testvarchar8")
print "\n"


# field lengths by hash
print "FIELD LENGTHS BY HASH: \n"
fieldlengths=cur.getRowLengthsHash(0)
assertEqual(fieldlengths["testint"],1)
assertEqual(fieldlengths["testfloat"],3)
assertEqual(fieldlengths["testchar"],9)
assertEqual(fieldlengths["testvarchar"],12)
print "\n"
fieldlengths=cur.getRowLengthsHash(7)
assertEqual(fieldlengths["testint"],1)
assertEqual(fieldlengths["testfloat"],3)
assertEqual(fieldlengths["testchar"],9)
assertEqual(fieldlengths["testvarchar"],12)
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.sendQuery("drop table testtable1")
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	col1 int, "+
	"	col2 char, "+
	"	col3 float)"))
cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))")
cur.substitution("var1",1)
cur.substitution("var2","hello")
cur.substitution("var3",10.5556,6,4)
assertTrue(cur.executeQuery())
print "\n"


# fields
print "FIELDS: \n"
assertTrue(cur.sendQuery("select * from testtable1"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
assertTrue(cur.sendQuery("delete from testtable1"))
print "\n"


# array substitutions
print "ARRAY SUBSTITUTIONS: \n"
cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))")
cur.substitutions(["var1","var2","var3"],
			[1,"hello",10.5556],[0,0,6],[0,0,4])
assertTrue(cur.executeQuery())
print "\n"


# fields
print "FIELDS: \n"
assertTrue(cur.sendQuery("select * from testtable1"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
assertTrue(cur.sendQuery("delete from testtable1"))
print "\n"


# nulls as nils
print "NULLS as nils: \n"
cur.getNullsAsNils()
assertTrue(cur.sendQuery("insert into testtable1 values (1,NULL,NULL)"))
assertTrue(cur.sendQuery("select * from testtable1"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select * from testtable1"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"")
assertEqual(cur.getField(0,2),"")
cur.getNullsAsNils()
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
assertEqual(cur.getColumnType(0),"INTEGER")
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
assertEqual(cur.colCount(),4)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
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


# commit
print "COMMIT: \n"
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
secondcur=SQLRCursor.new(secondcon)
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
assertTrue(con.commit())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"8")
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	10, "+
	"	10.1, "+
	"	'testchar10', "+
	"	'testvarchar10')"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"9")
print "\n"


# finished suspended session
print "FINISHED SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
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

# drop existing table
cur.sendQuery("drop table testtable")


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
