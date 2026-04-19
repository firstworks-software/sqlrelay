#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'sqlrelay'
require './asserts'




# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
cur=SQLRCursor.new(con)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"postgresql")
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
print "\n"


# bind format
print "BIND FORMAT: \n"
assertEqual(con.bindFormat(),"$1")
print "\n"


# nextval format
print "NEXTVAL FORMAT: \n"
assertEqual(con.nextvalFormat(),"nextval('%s')")
print "\n"


# isolation levels
#print "ISOLATION LEVELS: \n"
#isolationlevels=["read committed","read uncommitted","repeatable read","serializable"]
#for il in isolationlevels
#	# postgresql requires the isolation level to
#	# be the first query of the transaction
#	con.begin()
#	assertTrue(con.setIsolationLevel(il))
#	assertEqual(con.getIsolationLevel(),il)
#	con.commit()
#	print "\n"
#end
## reset to the default isolation level
#con.begin()
#assertTrue(con.setIsolationLevel(isolationlevels[0]))
#con.commit()
#print "\n"


# create testtable
print "CREATE TESTTABLE: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testfloat float, "+
	"	testreal real, "+
	"	testsmallint smallint, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testtext text, "+
	"	testbytea bytea)"))
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
	"	1.1, "+
	"	1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01/01/2001', "+
	"	'01:00:00', "+
	"	NULL, "+
	"	'testtext1', "+
	"	'testbytea1')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	2, "+
	"	2.2, "+
	"	2.2, "+
	"	2, "+
	"	'testchar2', "+
	"	'testvarchar2', "+
	"	'01/01/2002', "+
	"	'02:00:00', "+
	"	NULL, "+
	"	'testtext2', "+
	"	'testbytea2')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	3, "+
	"	3.3, "+
	"	3.3, "+
	"	3, "+
	"	'testchar3', "+
	"	'testvarchar3', "+
	"	'01/01/2003', "+
	"	'03:00:00', "+
	"	NULL, "+
	"	'testtext3', "+
	"	'testbytea3')"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	4, "+
	"	4.4, "+
	"	4.4, "+
	"	4, "+
	"	'testchar4', "+
	"	'testvarchar4', "+
	"	'01/01/2004', "+
	"	'04:00:00', "+
	"	NULL, "+
	"	'testtext4', "+
	"	'testbytea4')"))
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
	"	$1, "+
	"	$2, "+
	"	$3, "+
	"	$4, "+
	"	$5, "+
	"	$6, "+
	"	$7, "+
	"	$8, "+
	"	NULL, "+
	"	$9, "+
	"	$10)")
assertEqual(cur.countBindVariables(),10)
cur.inputBind("1",5)
cur.inputBind("2",5.5,4,2)
cur.inputBind("3",5.5,4,2)
cur.inputBind("4",5)
cur.inputBind("5","testchar5")
cur.inputBind("6","testvarchar5")
cur.inputBind("7","01/01/2005")
cur.inputBind("8","05:00:00")
cur.inputBindClob("9","testtext5","testtext5".to_s.bytesize)
cur.inputBindBlob("10","testbytea5","testbytea5".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",6)
cur.inputBind("2",6.6,4,2)
cur.inputBind("3",6.6,4,2)
cur.inputBind("4",6)
cur.inputBind("5","testchar6")
cur.inputBind("6","testvarchar6")
cur.inputBind("7","01/01/2006")
cur.inputBind("8","06:00:00")
cur.inputBindClob("9","testtext6","testtext6".to_s.bytesize)
cur.inputBindBlob("10","testbytea6","testbytea6".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",7)
cur.inputBind("2",7.7,4,2)
cur.inputBind("3",7.7,4,2)
cur.inputBind("4",7)
cur.inputBind("5","testchar7")
cur.inputBind("6","testvarchar7")
cur.inputBind("7","01/01/2007")
cur.inputBind("8","07:00:00")
cur.inputBindClob("9","testtext7","testtext7".to_s.bytesize)
cur.inputBindBlob("10","testbytea8","testbytea8".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by position
# postgresql doesn't support implicit conversion of string binds to
# other data types, so arrays of binds don't generally work.


# input bind by name
# postgresql doesn't support bind by name


# input bind by position with validation
print "BIND BY POSITION WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("1",8)
cur.inputBind("2",8.8,4,2)
cur.inputBind("3",8.8,4,2)
cur.inputBind("4",8)
cur.inputBind("5","testchar8")
cur.inputBind("6","testvarchar8")
cur.inputBind("7","01/01/2008")
cur.inputBind("8","08:00:00")
cur.inputBindClob("9","testtext8","testtext8".to_s.bytesize)
cur.inputBindClob("10","testbytea8","testbytea8".to_s.bytesize)
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by name
# postgresql doesn't support bind by name


# input bind by name with validation
# postgresql doesn't support bind by name


# select
print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),11)
print "\n"


# column names
print "COLUMN NAMES: \n"
assertEqual(cur.getColumnName(0),"testint")
assertEqual(cur.getColumnName(1),"testfloat")
assertEqual(cur.getColumnName(2),"testreal")
assertEqual(cur.getColumnName(3),"testsmallint")
assertEqual(cur.getColumnName(4),"testchar")
assertEqual(cur.getColumnName(5),"testvarchar")
assertEqual(cur.getColumnName(6),"testdate")
assertEqual(cur.getColumnName(7),"testtime")
assertEqual(cur.getColumnName(8),"testtimestamp")
assertEqual(cur.getColumnName(9),"testtext")
assertEqual(cur.getColumnName(10),"testbytea")
cols=cur.getColumnNames()
assertEqual(cols[0],"testint")
assertEqual(cols[1],"testfloat")
assertEqual(cols[2],"testreal")
assertEqual(cols[3],"testsmallint")
assertEqual(cols[4],"testchar")
assertEqual(cols[5],"testvarchar")
assertEqual(cols[6],"testdate")
assertEqual(cols[7],"testtime")
assertEqual(cols[8],"testtimestamp")
assertEqual(cols[9],"testtext")
assertEqual(cols[10],"testbytea")
print "\n"


# column types
print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"int4")
assertEqual(cur.getColumnType("testint"),"int4")
assertEqual(cur.getColumnType(1),"float8")
assertEqual(cur.getColumnType("testfloat"),"float8")
assertEqual(cur.getColumnType(2),"float4")
assertEqual(cur.getColumnType("testreal"),"float4")
assertEqual(cur.getColumnType(3),"int2")
assertEqual(cur.getColumnType("testsmallint"),"int2")
assertEqual(cur.getColumnType(4),"bpchar")
assertEqual(cur.getColumnType("testchar"),"bpchar")
assertEqual(cur.getColumnType(5),"varchar")
assertEqual(cur.getColumnType("testvarchar"),"varchar")
assertEqual(cur.getColumnType(6),"date")
assertEqual(cur.getColumnType("testdate"),"date")
assertEqual(cur.getColumnType(7),"time")
assertEqual(cur.getColumnType("testtime"),"time")
assertEqual(cur.getColumnType(8),"timestamp")
assertEqual(cur.getColumnType("testtimestamp"),"timestamp")
assertEqual(cur.getColumnType(9),"text")
assertEqual(cur.getColumnType("testtext"),"text")
assertEqual(cur.getColumnType(10),"bytea")
assertEqual(cur.getColumnType("testbytea"),"bytea")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnLength("testint"),4)
assertEqual(cur.getColumnLength(1),8)
assertEqual(cur.getColumnLength("testfloat"),8)
assertEqual(cur.getColumnLength(2),4)
assertEqual(cur.getColumnLength("testreal"),4)
assertEqual(cur.getColumnLength(3),2)
assertEqual(cur.getColumnLength("testsmallint"),2)
assertEqual(cur.getColumnLength(4),44)
assertEqual(cur.getColumnLength("testchar"),44)
assertEqual(cur.getColumnLength(5),44)
assertEqual(cur.getColumnLength("testvarchar"),44)
assertEqual(cur.getColumnLength(6),4)
assertEqual(cur.getColumnLength("testdate"),4)
assertEqual(cur.getColumnLength(7),8)
assertEqual(cur.getColumnLength("testtime"),8)
assertEqual(cur.getColumnLength(8),8)
assertEqual(cur.getColumnLength("testtimestamp"),8)
assertEqual(cur.getColumnLength(9),0)
assertEqual(cur.getColumnLength("testtext"),0)
assertEqual(cur.getColumnLength(10),0)
assertEqual(cur.getColumnLength("testbytea"),0)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest("testint"),1)
assertEqual(cur.getLongest(1),3)
assertEqual(cur.getLongest("testfloat"),3)
assertEqual(cur.getLongest(2),3)
assertEqual(cur.getLongest("testreal"),3)
assertEqual(cur.getLongest(3),1)
assertEqual(cur.getLongest("testsmallint"),1)
assertEqual(cur.getLongest(4),40)
assertEqual(cur.getLongest("testchar"),40)
assertEqual(cur.getLongest(5),12)
assertEqual(cur.getLongest("testvarchar"),12)
assertEqual(cur.getLongest(6),10)
assertEqual(cur.getLongest("testdate"),10)
assertEqual(cur.getLongest(7),8)
assertEqual(cur.getLongest("testtime"),8)
assertEqual(cur.getLongest(9),9)
assertEqual(cur.getLongest("testtext"),9)
assertEqual(cur.getLongest(10),10)
assertEqual(cur.getLongest("testbytea"),10)
print "\n"


# row count
print "ROW COUNT: \n"
assertEqual(cur.rowCount(),8)
print "\n"


# total rows
print "TOTAL ROWS: \n"
assertEqual(cur.totalRows(),8)
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
assertEqual(cur.getField(0,2),"1.1")
assertEqual(cur.getField(0,3),"1")
assertEqual(cur.getField(0,4),"testchar1                               ")
assertEqual(cur.getField(0,5),"testvarchar1")
assertEqual(cur.getField(0,6),"2001-01-01")
assertEqual(cur.getField(0,7),"01:00:00")
assertEqual(cur.getField(0,9),"testtext1")
assertEqual(cur.getField(0,10),"testbytea1")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8.8")
assertEqual(cur.getField(7,2),"8.8")
assertEqual(cur.getField(7,3),"8")
assertEqual(cur.getField(7,4),"testchar8                               ")
assertEqual(cur.getField(7,5),"testvarchar8")
assertEqual(cur.getField(7,6),"2008-01-01")
assertEqual(cur.getField(7,7),"08:00:00")
assertEqual(cur.getField(7,9),"testtext8")
assertEqual(cur.getField(7,10),"testbytea8")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),3)
assertEqual(cur.getFieldLength(0,2),3)
assertEqual(cur.getFieldLength(0,3),1)
assertEqual(cur.getFieldLength(0,4),40)
assertEqual(cur.getFieldLength(0,5),12)
assertEqual(cur.getFieldLength(0,6),10)
assertEqual(cur.getFieldLength(0,7),8)
assertEqual(cur.getFieldLength(0,9),9)
assertEqual(cur.getFieldLength(0,10),10)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),3)
assertEqual(cur.getFieldLength(7,2),3)
assertEqual(cur.getFieldLength(7,3),1)
assertEqual(cur.getFieldLength(7,4),40)
assertEqual(cur.getFieldLength(7,5),12)
assertEqual(cur.getFieldLength(7,6),10)
assertEqual(cur.getFieldLength(7,7),8)
assertEqual(cur.getFieldLength(7,9),9)
assertEqual(cur.getFieldLength(7,10),10)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"testint"),"1")
assertEqual(cur.getField(0,"testfloat"),"1.1")
assertEqual(cur.getField(0,"testreal"),"1.1")
assertEqual(cur.getField(0,"testsmallint"),"1")
assertEqual(cur.getField(0,"testchar"),"testchar1                               ")
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
assertEqual(cur.getField(0,"testdate"),"2001-01-01")
assertEqual(cur.getField(0,"testtime"),"01:00:00")
assertEqual(cur.getField(0,"testtext"),"testtext1")
assertEqual(cur.getField(0,"testbytea"),"testbytea1")
print "\n"
assertEqual(cur.getField(7,"testint"),"8")
assertEqual(cur.getField(7,"testfloat"),"8.8")
assertEqual(cur.getField(7,"testreal"),"8.8")
assertEqual(cur.getField(7,"testsmallint"),"8")
assertEqual(cur.getField(7,"testchar"),"testchar8                               ")
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
assertEqual(cur.getField(7,"testdate"),"2008-01-01")
assertEqual(cur.getField(7,"testtime"),"08:00:00")
assertEqual(cur.getField(7,"testtext"),"testtext8")
assertEqual(cur.getField(7,"testbytea"),"testbytea8")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"testint"),1)
assertEqual(cur.getFieldLength(0,"testfloat"),3)
assertEqual(cur.getFieldLength(0,"testreal"),3)
assertEqual(cur.getFieldLength(0,"testsmallint"),1)
assertEqual(cur.getFieldLength(0,"testchar"),40)
assertEqual(cur.getFieldLength(0,"testvarchar"),12)
assertEqual(cur.getFieldLength(0,"testdate"),10)
assertEqual(cur.getFieldLength(0,"testtime"),8)
assertEqual(cur.getFieldLength(0,"testtext"),9)
assertEqual(cur.getFieldLength(0,"testbytea"),10)
print "\n"
assertEqual(cur.getFieldLength(7,"testint"),1)
assertEqual(cur.getFieldLength(7,"testfloat"),3)
assertEqual(cur.getFieldLength(7,"testreal"),3)
assertEqual(cur.getFieldLength(7,"testsmallint"),1)
assertEqual(cur.getFieldLength(7,"testchar"),40)
assertEqual(cur.getFieldLength(7,"testvarchar"),12)
assertEqual(cur.getFieldLength(7,"testdate"),10)
assertEqual(cur.getFieldLength(7,"testtime"),8)
assertEqual(cur.getFieldLength(7,"testtext"),9)
assertEqual(cur.getFieldLength(7,"testbytea"),10)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1.1")
assertEqual(fields[2],"1.1")
assertEqual(fields[3],"1")
assertEqual(fields[4],"testchar1                               ")
assertEqual(fields[5],"testvarchar1")
assertEqual(fields[6],"2001-01-01")
assertEqual(fields[7],"01:00:00")
assertEqual(fields[9],"testtext1")
assertEqual(fields[10],"testbytea1")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],3)
assertEqual(fieldlens[2],3)
assertEqual(fieldlens[3],1)
assertEqual(fieldlens[4],40)
assertEqual(fieldlens[5],12)
assertEqual(fieldlens[6],10)
assertEqual(fieldlens[7],8)
assertEqual(fieldlens[9],9)
assertEqual(fieldlens[10],10)
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
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnType(0),"int4")
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
assertEqual(cur.colCount(),11)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
assertEqual(cur.getColumnName(0),"testint")
assertEqual(cur.getColumnName(1),"testfloat")
assertEqual(cur.getColumnName(2),"testreal")
assertEqual(cur.getColumnName(3),"testsmallint")
assertEqual(cur.getColumnName(4),"testchar")
assertEqual(cur.getColumnName(5),"testvarchar")
assertEqual(cur.getColumnName(6),"testdate")
assertEqual(cur.getColumnName(7),"testtime")
assertEqual(cur.getColumnName(8),"testtimestamp")
cols=cur.getColumnNames()
assertEqual(cols[0],"testint")
assertEqual(cols[1],"testfloat")
assertEqual(cols[2],"testreal")
assertEqual(cols[3],"testsmallint")
assertEqual(cols[4],"testchar")
assertEqual(cols[5],"testvarchar")
assertEqual(cols[6],"testdate")
assertEqual(cols[7],"testtime")
assertEqual(cols[8],"testtimestamp")
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
print "\n"


# commit and rollback
print "COMMIT AND ROLLBACK: \n"
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
secondcur=SQLRCursor.new(secondcon)
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
assertTrue(con.commit())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"8")
assertTrue(con.begin())
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	10, "+
	"	10.1, "+
	"	10.1, "+
	"	10, "+
	"	'testchar10', "+
	"	'testvarchar10', "+
	"	'01/01/2010', "+
	"	'10:00:00', "+
	"	NULL)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"8")
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	10, "+
	"	10.1, "+
	"	10.1, "+
	"	10, "+
	"	'testchar10', "+
	"	'testvarchar10', "+
	"	'01/01/2010', "+
	"	'10:00:00', "+
	"	NULL)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"9")
secondcon.endSession()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)',$(var3)")
cur.substitution("var1",1)
cur.substitution("var2","hello")
cur.substitution("var3",10.5556,6,4)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# array substitutions
print "ARRAY SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),$(var2),$(var3)")
cur.substitutions(["var1","var2","var3"],[1,2,3])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"2")
assertEqual(cur.getField(0,2),"3")
print "\n"
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'")
cur.substitutions(["var1","var2","var3"],["hi","hello","bye"])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"hi")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"bye")
print "\n"
cur.prepareQuery("select $(var1),$(var2),$(var3)")
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
assertTrue(cur.sendQuery("select NULL,1,NULL"))
assertEqual(cur.getField(0,0),nil)
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select NULL,1,NULL"))
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
	"	testclob1 text, "+
	"	testclob2 text, "+
	"	testblob1 bytea, "+
	"	testblob2 bytea)"))
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$1, "+
	"	$2, "+
	"	$3, "+
	"	$4)")
cur.inputBindClob("1","","".to_s.bytesize)
cur.inputBindClob("2",nil,nil.to_s.bytesize)
cur.inputBindBlob("3","","".to_s.bytesize)
cur.inputBindBlob("4",nil,nil.to_s.bytesize)
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
	"	testtext text, "+
	"	testbytea bytea)")
cur.prepareQuery("insert into testtable values ($1,$2)")
largebuffer="C" * 8192
cur.inputBindClob("1",largebuffer,largebuffer.to_s.bytesize)
cur.inputBindBlob("2",largebuffer,largebuffer.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getFieldLength(0,"testtext"),8192)
assertEqual(cur.getField(0,"testtext"),largebuffer)
assertEqual(cur.getFieldLength(0,"testbytea"),8192)
assertEqualLen(cur.getField(0,"testbytea"),largebuffer,8192)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# output bind by position
# postgresql doesn't support output binds


# output bind by name
# postgresql doesn't support output binds


# output bind by name with validation
# postgresql doesn't support output binds


# lob output bind
# postgresql doesn't support output binds


# long output bind
# postgresql doesn't support output binds


# negative input bind
print "NEGATIVE INPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery("create table testtable (testval int)")
cur.prepareQuery("insert into testtable values ($1)")
cur.inputBind("1",-1)
assertTrue(cur.executeQuery())
cur.sendQuery("select testval from testtable")
assertEqual(cur.getField(0,"testval"),"-1")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# bind validation
# postgresql doesn't support bind by name


# rebinding
print "REBINDING: \n"
cur.sendQuery("drop function testfunc(int)")
assertTrue(cur.sendQuery(
	"create function testfunc(int) returns int as "+
	"	' begin return $1; end;' language plpgsql"))
cur.prepareQuery("select * from testfunc($1)")
cur.inputBind("1",1)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
cur.inputBind("1",2)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"2")
cur.inputBind("1",3)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"3")
assertTrue(cur.sendQuery("drop function testfunc(int)"))
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
cur.prepareQuery("select $1::int")
cur.inputBind("1",1)
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.inputBind("1",2)
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"2")
print "\n"


# stored procedure returning no value
print "STORED PROCEDURE RETURNING NO VALUE: \n"
cur.sendQuery("drop function testfunc(int,float,char(20))")
assertTrue(cur.sendQuery(
	"create function testfunc("+
	"	int,float,char(20)) "+
	"returns void as ' "+
	"	declare in1 int; "+
	"	in2 float; "+
	"	in3 char(20); "+
	"begin "+
	"	in1:=$1; "+
	"	in2:=$2; "+
	"	in3:=$3; "+
	"	return; "+
	"end;' language plpgsql"))
cur.prepareQuery("select testfunc($1,$2,$3)")
cur.inputBind("1",1)
cur.inputBind("2",1.1,4,2)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("drop function testfunc(int,float,char(20))"))
print "\n"


# stored procedure returning single value
print "STORED PROCEDURE RETURNING SINGLE VALUE: \n"
cur.sendQuery("drop function testfunc(int,float,char(20))")
assertTrue(cur.sendQuery(
	"create function testfunc(int,float,char(20)) returns int as "+
	"	' begin return $1; end;' language plpgsql"))
cur.prepareQuery("select * from testfunc($1,$2,$3)")
cur.inputBind("1",1)
cur.inputBind("2",1.1,4,2)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertTrue(cur.sendQuery("drop function testfunc(int,float,char(20))"))
print "\n"


# stored procedure returning multiple values
print "STORED PROCEDURE RETURNING MULTIPLE VALUES: \n"
cur.sendQuery("drop function testfunc(int,float,char(20))")
assertTrue(cur.sendQuery(
	"create function testfunc("+
	"	int,float,char(20)) "+
	"returns record as ' "+
	"	declare output record; "+
	"begin "+
	"	select $1,$2,$3 into output; "+
	"	return output; "+
	"end;' language plpgsql"))
cur.prepareQuery(
	"select "+
	"	* "+
	"from "+
	"	testfunc($1,$2,$3) "+
	"	as (col1 int, "+
	"		col2 float, "+
	"		col3 bpchar) ")
cur.inputBind("1",1)
cur.inputBind("2",1.1,4,2)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
#assertEqual(cur.getField(0,1).to_f,1.1)
assertEqual(cur.getField(0,2),"hello")
assertTrue(cur.sendQuery("drop function testfunc(int,float,char(20))"))
print "\n"


# stored procedure returning result set
print "STORED PROCEDURE RETURNING RESULT SET: \n"
cur.sendQuery("drop function testfunc()")
assertTrue(cur.sendQuery(
	"create function testfunc() "+
	"returns setof record as ' "+
	"	declare output record; "+
	"begin "+
	"	for output in "+
	"		select 1 "+
	"		union "+
	"		select 2 "+
	"		union "+
	"		select 3 "+
	"		union "+
	"		select 4 "+
	"		union "+
	"		select 5 "+
	"		union "+
	"		select 6 "+
	"		union "+
	"		select 7 "+
	"		union "+
	"		select 8 "+
	"	loop "+
	"		return next output; "+
	"	end loop; "+
	"	return; "+
	"end;' language plpgsql"))
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testfunc() "+
	"	as (testint int)"))
assertEqual(cur.rowCount(),8)
assertTrue(cur.sendQuery("drop function testfunc()"))
print "\n"


# temporary tables
print "TEMPORARY TABLES: \n"
cur.sendQuery("drop table temptable\n")
cur.sendQuery("create temporary table temptable (col1 int)")
assertTrue(cur.sendQuery("insert into temptable values (1)"))
assertTrue(cur.sendQuery("select count(*) from temptable"))
assertEqual(cur.getField(0,0),"1")
con.endSession()
print "\n"
assertFalse(cur.sendQuery("select count(*) from temptable"))
print "\n"


# encoded binary data
print "ENCODED BINARY DATA: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 bytea)"))
buffer=(0..255).map { |j| j.chr }.join
querystr="insert into testtable values (decode('"
querystr=querystr+buffer.bytes.map { |b| "%02x" % b }.join
querystr=querystr+"','hex'))"
assertTrue(cur.sendQuery(querystr))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),256)
assertEqualLen(cur.getField(0,0),buffer,256)
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
		"	(col1 serial primary key, "+
		"	col2 int)"))
assertTrue(cur.sendQuery(
		"insert into testtable (col2) values (1)"))
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
assertTrue(cur.rowCount()>0)
print "\n"


# schema list
print "SCHEMA LIST: \n"
assertTrue(cur.getSchemaList(nil))
assertEqual(cur.getColumnName(0),"Database")
assertTrue(cur.rowCount()>0)
print "\n"


# table type list
print "TABLE TYPE LIST: \n"
assertTrue(cur.getTableTypeList())
assertEqual(cur.getColumnName(0),"table_type")
found=false
for i in 0..cur.rowCount()-1
	if cur.getField(i,"table_type")=="TABLE"
		found=true
		break
	end
end
assertTrue(found)
print "\n"


# table list
print "TABLE LIST: \n"
cur.sendQuery("drop table testtable1")
cur.sendQuery("drop table testtable2")
cur.sendQuery("drop table testtable3")
cur.sendQuery("drop table testtable4")
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
for i in 0..cur.rowCount()-1
	name=cur.getField(i,"Tables_in_xxx")
	if name=="testtable1" ||
		name=="testtable2" ||
		name=="testtable3" ||
		name=="testtable4"
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
assertEqual(cur.getField(0,"precision"),"10")
assertEqual(cur.getField(0,"local_type_name"),"INTEGER")
assertTrue(cur.getTypeInfoList("char"))
assertEqual(cur.getField(0,"type_name"),"CHAR")
assertEqual(cur.getField(0,"data_type"),"1")
assertEqual(cur.getField(0,"precision"),"255")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"255")
assertEqual(cur.getField(0,"local_type_name"),"VARCHAR")
assertTrue(cur.getTypeInfoList("date"))
assertEqual(cur.getField(0,"type_name"),"DATE")
assertEqual(cur.getField(0,"data_type"),"91")
assertEqual(cur.getField(0,"precision"),"10")
assertEqual(cur.getField(0,"local_type_name"),"DATE")
print "\n"


# column list
print "COLUMN LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testfloat float, "+
	"	testreal real, "+
	"	testsmallint smallint, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testtext text, "+
	"	testbytea bytea)"))
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
assertEqual(cur.getField(2,"column_name"),"testreal")
assertEqual(cur.getField(3,"column_name"),"testsmallint")
assertEqual(cur.getField(4,"column_name"),"testchar")
assertEqual(cur.getField(5,"column_name"),"testvarchar")
assertEqual(cur.getField(6,"column_name"),"testdate")
assertEqual(cur.getField(7,"column_name"),"testtime")
assertEqual(cur.getField(8,"column_name"),"testtimestamp")
assertEqual(cur.getField(9,"column_name"),"testtext")
assertEqual(cur.getField(10,"column_name"),"testbytea")
assertEqual(cur.getField(0,"data_type"),"integer")
assertEqual(cur.getField(1,"data_type"),"double precision")
assertEqual(cur.getField(2,"data_type"),"real")
assertEqual(cur.getField(3,"data_type"),"smallint")
assertEqual(cur.getField(4,"data_type"),"character")
assertEqual(cur.getField(5,"data_type"),"character varying")
assertEqual(cur.getField(6,"data_type"),"date")
assertEqual(cur.getField(7,"data_type"),"time without time zone")
assertEqual(cur.getField(8,"data_type"),"timestamp without time zone")
assertEqual(cur.getField(9,"data_type"),"text")
assertEqual(cur.getField(10,"data_type"),"bytea")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# column list - auto_increment, primary key
print "COLUMN LIST - auto_increment, primary key: \n"
cur.sendQuery("drop table if exists testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 serial primary key, "+
	"	col2 int)"))
assertTrue(cur.getColumnList("testtable",nil))
assertTrue(cur.getField(0,"extra").include?("auto_increment"))
assertTrue(cur.getField(0,"column_key").include?("PRI"))
assertFalse(cur.getField(1,"extra") && cur.getField(1,"extra").include?("auto_increment"))
assertFalse(cur.getField(1,"column_key") && cur.getField(1,"column_key").include?("PRI"))
print "\n"
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getColumnList("testtable",nil))
assertFalse(cur.getField(0,"extra") && cur.getField(0,"extra").include?("auto_increment"))
assertTrue(cur.getField(0,"column_key").include?("PRI"))
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# primary keys list
print "PRIMARY KEYS LIST: \n"
cur.sendQuery("drop table testtable")
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
assertEqual(cur.getField(0,"table"),"testtable")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"col1")
assertTrue(cur.getField(0,"key_name")!=nil && cur.getField(0,"key_name")!="")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# key and index list
print "KEY AND INDEX LIST: \n"
cur.sendQuery("drop table testtable")
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
assertEqual(cur.getField(0,"table"),"testtable")
assertEqual(cur.getField(0,"non_unique"),"f")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"col1")
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"3")
assertTrue(cur.getField(0,"key_name")!=nil && cur.getField(0,"key_name")!="")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
cur.sendQuery("drop function testproc1(int,char,varchar,date)")
cur.sendQuery("drop function testproc2(int,char,varchar,date)")
cur.sendQuery("drop function testproc3(int,char,varchar,date)")
cur.sendQuery("drop function testproc4(int,char,varchar,date)")
assertTrue(cur.sendQuery(
	"create function testproc1("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"returns void "+
	"as 'begin end;' "+
	"language plpgsql"))
assertTrue(cur.sendQuery(
	"create function testproc2("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"returns void "+
	"as 'begin end;' "+
	"language plpgsql"))
assertTrue(cur.sendQuery(
	"create function testproc3("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"returns void "+
	"as 'begin end;' "+
	"language plpgsql"))
assertTrue(cur.sendQuery(
	"create function testproc4("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"returns void "+
	"as 'begin end;' "+
	"language plpgsql"))
assertTrue(cur.getProcedureList(nil))
counter=0
for i in 0..cur.rowCount()-1
	name=cur.getField(i,"routine_name")
	if name=="testproc1" ||
		name=="testproc2" ||
		name=="testproc3" ||
		name=="testproc4"
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
assertEqual(cur.getField(0,"parameter_name"),"in1")
assertEqual(cur.getField(0,"parameter_mode"),"1")
assertEqual(cur.getField(0,"data_type"),"integer")
assertEqual(cur.getField(0,"ordinal_position"),"1")
assertEqual(cur.getField(1,"parameter_name"),"in2")
assertEqual(cur.getField(1,"parameter_mode"),"1")
assertEqual(cur.getField(1,"data_type"),"character")
assertEqual(cur.getField(1,"ordinal_position"),"2")
assertEqual(cur.getField(2,"parameter_name"),"in3")
assertEqual(cur.getField(2,"parameter_mode"),"1")
assertEqual(cur.getField(2,"data_type"),"character varying")
assertEqual(cur.getField(2,"ordinal_position"),"3")
assertEqual(cur.getField(3,"parameter_name"),"in4")
assertEqual(cur.getField(3,"parameter_mode"),"1")
assertEqual(cur.getField(3,"data_type"),"date")
assertEqual(cur.getField(3,"ordinal_position"),"4")
assertTrue(cur.sendQuery("drop function testproc1(int,char,varchar,date)"))
assertTrue(cur.sendQuery("drop function testproc2(int,char,varchar,date)"))
assertTrue(cur.sendQuery("drop function testproc3(int,char,varchar,date)"))
assertTrue(cur.sendQuery("drop function testproc4(int,char,varchar,date)"))
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery("select * from testtable order by testint"))
assertFalse(cur.sendQuery("select * from testtable order by testint"))
assertFalse(cur.sendQuery("select * from testtable order by testint"))
assertFalse(cur.sendQuery("select * from testtable order by testint"))
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
