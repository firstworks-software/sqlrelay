#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'sqlrelay'
require './asserts'




# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"db2inst1","testpassword",0,1)
cur=SQLRCursor.new(con)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"db2")
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
assertEqual(con.bindFormat(),"?")
print "\n"


# nextval format
print "NEXTVAL FORMAT: \n"
assertEqual(con.nextvalFormat(),"(nextval for %s)")
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
isolationlevels=["CS","UR","RS","RR"]
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
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testsmallint smallint, "+
	"	testint integer, "+
	"	testbigint bigint, "+
	"	testdecimal decimal(10,2), "+
	"	testreal real, "+
	"	testdouble double, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(con.commit())
print "\n"


# insert
print "INSERT: \n"
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1, "+
	"	1, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01/01/2001', "+
	"	'01:00:00', "+
	"	NULL, "+
	"	'testclob1', "+
	"	blob('testblob1'))"))
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
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	NULL, "+
	"	?, "+
	"	?)")
assertEqual(cur.countBindVariables(),12)
cur.inputBind("1",2)
cur.inputBind("2",2)
cur.inputBind("3",2)
cur.inputBind("4",2.5,4,2)
cur.inputBind("5",2.5,4,2)
cur.inputBind("6",2.5,4,2)
cur.inputBind("7","testchar2")
cur.inputBind("8","testvarchar2")
cur.inputBindDate("9",2002,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("10",-1,-1,-1,2,0,0,0,"",0)
cur.inputBindClob("11","testclob2",9)
cur.inputBindBlob("12","testblob2",9)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",3)
cur.inputBind("2",3)
cur.inputBind("3",3)
cur.inputBind("4",3.5,4,2)
cur.inputBind("5",3.5,4,2)
cur.inputBind("6",3.5,4,2)
cur.inputBind("7","testchar3")
cur.inputBind("8","testvarchar3")
cur.inputBindDate("9",2003,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("10",-1,-1,-1,3,0,0,0,"",0)
cur.inputBindClob("11","testclob3",9)
cur.inputBindBlob("12","testblob3",9)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",4)
cur.inputBind("2",4)
cur.inputBind("3",4)
cur.inputBind("4",4.5,4,2)
cur.inputBind("5",4.5,4,2)
cur.inputBind("6",4.5,4,2)
cur.inputBind("7","testchar4")
cur.inputBind("8","testvarchar4")
cur.inputBindDate("9",2004,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("10",-1,-1,-1,4,0,0,0,"",0)
cur.inputBindClob("11","testclob4",9)
cur.inputBindBlob("12","testblob4",9)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",5)
cur.inputBind("2",5)
cur.inputBind("3",5)
cur.inputBind("4",5.5,4,2)
cur.inputBind("5",5.5,4,2)
cur.inputBind("6",5.5,4,2)
cur.inputBind("7","testchar5")
cur.inputBind("8","testvarchar5")
cur.inputBindDate("9",2005,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("10",-1,-1,-1,5,0,0,0,"",0)
cur.inputBindClob("11","testclob5",9)
cur.inputBindBlob("12","testblob5",9)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",6)
cur.inputBind("2",6)
cur.inputBind("3",6)
cur.inputBind("4",6.5,4,2)
cur.inputBind("5",6.5,4,2)
cur.inputBind("6",6.5,4,2)
cur.inputBind("7","testchar6")
cur.inputBind("8","testvarchar6")
cur.inputBindDate("9",2006,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("10",-1,-1,-1,6,0,0,0,"",0)
cur.inputBindClob("11","testclob6",9)
cur.inputBindBlob("12","testblob6",9)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by position
print "ARRAY OF INPUT BINDS BY POSITION: \n"
cur.clearBinds()
cur.inputBinds(["1","2","3","4","5","6",
			"7","8","9","10","11","12"],
		["7","7","7","7.5","7.5","7.5",
			"testchar7","testvarchar7",
			"01/01/2007","07:00:00",
			"testclob7"])
assertTrue(cur.executeQuery())
print "\n"


# input bind by position with validation
print "INPUT BIND BY POSITION WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("1",8)
cur.inputBind("2",8)
cur.inputBind("3",8)
cur.inputBind("4",8.5,4,2)
cur.inputBind("5",8.5,4,2)
cur.inputBind("6",8.5,4,2)
cur.inputBind("7","testchar8")
cur.inputBind("8","testvarchar8")
cur.inputBindDate("9",2008,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("10",-1,-1,-1,8,0,0,0,"",0)
cur.inputBindClob("11","testclob8",9)
cur.inputBindBlob("12","testblob8",9)
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"

# input bind by name
# db2 doesn't support bind by name


# array of input binds by name
# db2 doesn't support bind by name


# input bind by name with validation
# db2 doesn't support bind by name


# select
print "SELECT: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),13)
print "\n"


# column names
print "COLUMN NAMES: \n"
assertEqual(cur.getColumnName(0),"TESTSMALLINT")
assertEqual(cur.getColumnName(1),"TESTINT")
assertEqual(cur.getColumnName(2),"TESTBIGINT")
assertEqual(cur.getColumnName(3),"TESTDECIMAL")
assertEqual(cur.getColumnName(4),"TESTREAL")
assertEqual(cur.getColumnName(5),"TESTDOUBLE")
assertEqual(cur.getColumnName(6),"TESTCHAR")
assertEqual(cur.getColumnName(7),"TESTVARCHAR")
assertEqual(cur.getColumnName(8),"TESTDATE")
assertEqual(cur.getColumnName(9),"TESTTIME")
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP")
cols=cur.getColumnNames()
assertEqual(cols[0],"TESTSMALLINT")
assertEqual(cols[1],"TESTINT")
assertEqual(cols[2],"TESTBIGINT")
assertEqual(cols[3],"TESTDECIMAL")
assertEqual(cols[4],"TESTREAL")
assertEqual(cols[5],"TESTDOUBLE")
assertEqual(cols[6],"TESTCHAR")
assertEqual(cols[7],"TESTVARCHAR")
assertEqual(cols[8],"TESTDATE")
assertEqual(cols[9],"TESTTIME")
assertEqual(cols[10],"TESTTIMESTAMP")
print "\n"


# column types
print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"SMALLINT")
assertEqual(cur.getColumnType("TESTSMALLINT"),"SMALLINT")
assertEqual(cur.getColumnType(1),"INTEGER")
assertEqual(cur.getColumnType("TESTINT"),"INTEGER")
assertEqual(cur.getColumnType(2),"BIGINT")
assertEqual(cur.getColumnType("TESTBIGINT"),"BIGINT")
assertEqual(cur.getColumnType(3),"DECIMAL")
assertEqual(cur.getColumnType("TESTDECIMAL"),"DECIMAL")
assertEqual(cur.getColumnType(4),"REAL")
assertEqual(cur.getColumnType("TESTREAL"),"REAL")
assertEqual(cur.getColumnType(5),"DOUBLE")
assertEqual(cur.getColumnType("TESTDOUBLE"),"DOUBLE")
assertEqual(cur.getColumnType(6),"CHAR")
assertEqual(cur.getColumnType("TESTCHAR"),"CHAR")
assertEqual(cur.getColumnType(7),"VARCHAR")
assertEqual(cur.getColumnType("TESTVARCHAR"),"VARCHAR")
assertEqual(cur.getColumnType(8),"DATE")
assertEqual(cur.getColumnType("TESTDATE"),"DATE")
assertEqual(cur.getColumnType(9),"TIME")
assertEqual(cur.getColumnType("TESTTIME"),"TIME")
assertEqual(cur.getColumnType(10),"TIMESTAMP")
assertEqual(cur.getColumnType("TESTTIMESTAMP"),"TIMESTAMP")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),2)
assertEqual(cur.getColumnLength("TESTSMALLINT"),2)
assertEqual(cur.getColumnLength(1),4)
assertEqual(cur.getColumnLength("TESTINT"),4)
assertEqual(cur.getColumnLength(2),8)
assertEqual(cur.getColumnLength("TESTBIGINT"),8)
assertEqual(cur.getColumnLength(3),12)
assertEqual(cur.getColumnLength("TESTDECIMAL"),12)
assertEqual(cur.getColumnLength(4),4)
assertEqual(cur.getColumnLength("TESTREAL"),4)
assertEqual(cur.getColumnLength(5),8)
assertEqual(cur.getColumnLength("TESTDOUBLE"),8)
assertEqual(cur.getColumnLength(6),40)
assertEqual(cur.getColumnLength("TESTCHAR"),40)
assertEqual(cur.getColumnLength(7),40)
assertEqual(cur.getColumnLength("TESTVARCHAR"),40)
assertEqual(cur.getColumnLength(8),6)
assertEqual(cur.getColumnLength("TESTDATE"),6)
assertEqual(cur.getColumnLength(9),6)
assertEqual(cur.getColumnLength("TESTTIME"),6)
assertEqual(cur.getColumnLength(10),16)
assertEqual(cur.getColumnLength("TESTTIMESTAMP"),16)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest("TESTSMALLINT"),1)
assertEqual(cur.getLongest(1),1)
assertEqual(cur.getLongest("TESTINT"),1)
assertEqual(cur.getLongest(2),1)
assertEqual(cur.getLongest("TESTBIGINT"),1)
assertEqual(cur.getLongest(3),4)
assertEqual(cur.getLongest("TESTDECIMAL"),4)
assertEqual(cur.getLongest(4),12)
assertEqual(cur.getLongest("TESTREAL"),12)
assertEqual(cur.getLongest(5),21)
assertEqual(cur.getLongest("TESTDOUBLE"),21)
assertEqual(cur.getLongest(6),40)
assertEqual(cur.getLongest("TESTCHAR"),40)
assertEqual(cur.getLongest(7),12)
assertEqual(cur.getLongest("TESTVARCHAR"),12)
assertEqual(cur.getLongest(8),10)
assertEqual(cur.getLongest("TESTDATE"),10)
assertEqual(cur.getLongest(9),8)
assertEqual(cur.getLongest("TESTTIME"),8)
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
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),"1")
assertEqual(cur.getField(0,3),"1.50")
assertEqual(cur.getField(0,4),"1.500000E+00")
assertEqual(cur.getField(0,5),"1.50000000000000E+000")
assertEqual(cur.getField(0,6),"testchar1                               ")
assertEqual(cur.getField(0,7),"testvarchar1")
assertEqual(cur.getField(0,8),"2001-01-01")
assertEqual(cur.getField(0,9),"01:00:00")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(7,2),"8")
assertEqual(cur.getField(7,3),"8.50")
assertEqual(cur.getField(7,4),"8.500000E+00")
assertEqual(cur.getField(7,5),"8.50000000000000E+000")
assertEqual(cur.getField(7,6),"testchar8                               ")
assertEqual(cur.getField(7,7),"testvarchar8")
assertEqual(cur.getField(7,8),"2008-01-01")
assertEqual(cur.getField(7,9),"08:00:00")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),1)
assertEqual(cur.getFieldLength(0,2),1)
assertEqual(cur.getFieldLength(0,3),4)
assertEqual(cur.getFieldLength(0,4),12)
assertEqual(cur.getFieldLength(0,5),21)
assertEqual(cur.getFieldLength(0,6),40)
assertEqual(cur.getFieldLength(0,7),12)
assertEqual(cur.getFieldLength(0,8),10)
assertEqual(cur.getFieldLength(0,9),8)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),1)
assertEqual(cur.getFieldLength(7,2),1)
assertEqual(cur.getFieldLength(7,3),4)
assertEqual(cur.getFieldLength(7,4),12)
assertEqual(cur.getFieldLength(7,5),21)
assertEqual(cur.getFieldLength(7,6),40)
assertEqual(cur.getFieldLength(7,7),12)
assertEqual(cur.getFieldLength(7,8),10)
assertEqual(cur.getFieldLength(7,9),8)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"TESTSMALLINT"),"1")
assertEqual(cur.getField(0,"TESTINT"),"1")
assertEqual(cur.getField(0,"TESTBIGINT"),"1")
assertEqual(cur.getField(0,"TESTDECIMAL"),"1.50")
assertEqual(cur.getField(0,"TESTREAL"),"1.500000E+00")
assertEqual(cur.getField(0,"TESTDOUBLE"),"1.50000000000000E+000")
assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                               ")
assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1")
assertEqual(cur.getField(0,"TESTDATE"),"2001-01-01")
assertEqual(cur.getField(0,"TESTTIME"),"01:00:00")
print "\n"
assertEqual(cur.getField(7,"TESTSMALLINT"),"8")
assertEqual(cur.getField(7,"TESTINT"),"8")
assertEqual(cur.getField(7,"TESTBIGINT"),"8")
assertEqual(cur.getField(7,"TESTDECIMAL"),"8.50")
assertEqual(cur.getField(7,"TESTREAL"),"8.500000E+00")
assertEqual(cur.getField(7,"TESTDOUBLE"),"8.50000000000000E+000")
assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                               ")
assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8")
assertEqual(cur.getField(7,"TESTDATE"),"2008-01-01")
assertEqual(cur.getField(7,"TESTTIME"),"08:00:00")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"TESTSMALLINT"),1)
assertEqual(cur.getFieldLength(0,"TESTINT"),1)
assertEqual(cur.getFieldLength(0,"TESTBIGINT"),1)
assertEqual(cur.getFieldLength(0,"TESTDECIMAL"),4)
assertEqual(cur.getFieldLength(0,"TESTREAL"),12)
assertEqual(cur.getFieldLength(0,"TESTDOUBLE"),21)
assertEqual(cur.getFieldLength(0,"TESTCHAR"),40)
assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12)
assertEqual(cur.getFieldLength(0,"TESTDATE"),10)
assertEqual(cur.getFieldLength(0,"TESTTIME"),8)
print "\n"
assertEqual(cur.getFieldLength(7,"TESTSMALLINT"),1)
assertEqual(cur.getFieldLength(7,"TESTINT"),1)
assertEqual(cur.getFieldLength(7,"TESTBIGINT"),1)
assertEqual(cur.getFieldLength(7,"TESTDECIMAL"),4)
assertEqual(cur.getFieldLength(7,"TESTREAL"),12)
assertEqual(cur.getFieldLength(7,"TESTDOUBLE"),21)
assertEqual(cur.getFieldLength(7,"TESTCHAR"),40)
assertEqual(cur.getFieldLength(7,"TESTVARCHAR"),12)
assertEqual(cur.getFieldLength(7,"TESTDATE"),10)
assertEqual(cur.getFieldLength(7,"TESTTIME"),8)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1")
assertEqual(fields[2],"1")
assertEqual(fields[3],"1.50")
assertEqual(fields[4],"1.500000E+00")
assertEqual(fields[5],"1.50000000000000E+000")
assertEqual(fields[6],"testchar1                               ")
assertEqual(fields[7],"testvarchar1")
assertEqual(fields[8],"2001-01-01")
assertEqual(fields[9],"01:00:00")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],1)
assertEqual(fieldlens[2],1)
assertEqual(fieldlens[3],4)
assertEqual(fieldlens[4],12)
assertEqual(fieldlens[5],21)
assertEqual(fieldlens[6],40)
assertEqual(fieldlens[7],12)
assertEqual(fieldlens[8],10)
assertEqual(fieldlens[9],8)
print "\n"


# result set buffer size
print "RESULT SET BUFFER SIZE: \n"
assertEqual(cur.getResultSetBufferSize(),0)
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertEqual(cur.getColumnName(0),nil)
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),nil)
cur.getColumnInfo()
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertEqual(cur.getColumnName(0),"TESTSMALLINT")
assertEqual(cur.getColumnLength(0),2)
assertEqual(cur.getColumnType(0),"SMALLINT")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
print "\n"


# column count for cached result set
print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),13)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
assertEqual(cur.getColumnName(0),"TESTSMALLINT")
assertEqual(cur.getColumnName(1),"TESTINT")
assertEqual(cur.getColumnName(2),"TESTBIGINT")
assertEqual(cur.getColumnName(3),"TESTDECIMAL")
assertEqual(cur.getColumnName(4),"TESTREAL")
assertEqual(cur.getColumnName(5),"TESTDOUBLE")
assertEqual(cur.getColumnName(6),"TESTCHAR")
assertEqual(cur.getColumnName(7),"TESTVARCHAR")
assertEqual(cur.getColumnName(8),"TESTDATE")
assertEqual(cur.getColumnName(9),"TESTTIME")
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP")
cols=cur.getColumnNames()
assertEqual(cols[0],"TESTSMALLINT")
assertEqual(cols[1],"TESTINT")
assertEqual(cols[2],"TESTBIGINT")
assertEqual(cols[3],"TESTDECIMAL")
assertEqual(cols[4],"TESTREAL")
assertEqual(cols[5],"TESTDOUBLE")
assertEqual(cols[6],"TESTCHAR")
assertEqual(cols[7],"TESTVARCHAR")
assertEqual(cols[8],"TESTDATE")
assertEqual(cols[9],"TESTTIME")
assertEqual(cols[10],"TESTTIMESTAMP")
print "\n"


# cached result set with result set buffer size
print "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testint"))
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
secondcur=SQLRCursor.new(con)
secondcur.setResultSetBufferSize(1)
i=0
while cur.getRow(i)
	assertTrue(secondcur.sendQuery("select * from testtable"))
	i=i+1
end
secondcur.closeResultSet()
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
# db2 DDL is transactional; commit so the table is visible to the
# second connection (the commit implicitly starts a new tx)
assertTrue(con.commit())
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
					"db2inst1","testpassword",0,1)
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
assertEqual(con.getTransactionModel(),"implicit")
assertFalse(con.getAutoCommit())
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')")
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
cur.prepareQuery("values ('$(var1)','$(var2)','$(var3)')")
cur.substitutions(["var1","var2","var3"],["hi","hello","bye"])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"hi")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"bye")
print "\n"
cur.prepareQuery("values ($(var1),$(var2),$(var3))")
cur.substitutions(["var1","var2","var3"],[1,2,3])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"2")
assertEqual(cur.getField(0,2),"3")
print "\n"
cur.prepareQuery("values ($(var1),$(var2),$(var3))")
cur.substitutions(["var1","var2","var3"],[10.55,10.556,10.5556],[4,5,6],[2,3,4])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"10.55")
assertEqual(cur.getField(0,1),"10.556")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# nulls as nulls
print "NULLS AS NULLS: \n"
cur.getNullsAsNils()
assertTrue(cur.sendQuery("select NULL,1,NULL from sysibm.sysdummy1"))
assertEqual(cur.getField(0,0),nil)
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select NULL,1,NULL from sysibm.sysdummy1"))
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
assertTrue(con.commit())
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?)")
cur.inputBindClob("1","",0)
cur.inputBindClob("2",nil,0)
cur.inputBindBlob("3","",0)
cur.inputBindBlob("4",nil,0)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getField(0,0),"")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2),"")
assertEqual(cur.getField(0,3),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# long lobs
print "LONG LOBS: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(con.commit())
cur.prepareQuery("insert into testtable values (?,?)")
largebuffer = "C" * (20*1024)
cur.inputBindClob("1",largebuffer,20*1024)
cur.inputBindBlob("2",largebuffer,20*1024)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getFieldLength(0,"TESTCLOB"),20*1024)
assertEqual(cur.getField(0,"TESTCLOB"),largebuffer)
assertEqual(cur.getFieldLength(0,"TESTBLOB"),20*1024)
assertEqualLen(cur.getField(0,"TESTBLOB"),largebuffer,20*1024)
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# output bind by position
print "OUTPUT BIND BY POSITION: \n"
cur.sendQuery("drop procedure testproc")
cur.getNullsAsNils()
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	out out1 int, "+
	"	out out2 varchar(20), "+
	"	out out3 double, "+
	"	out out4 date, "+
	"	out out5 varchar(20)) "+
	"language sql "+
	"begin "+
	"	set out1 = 1; "+
	"	set out2 = 'hello'; "+
	"	set out3 = 2.5; "+
	"	set out4 = '2001-02-03'; "+
	"	set out5 = null; "+
	"end"))
assertTrue(con.commit())
cur.prepareQuery("call testproc(?,?,?,?,?)")
assertEqual(cur.countBindVariables(),5)
cur.defineOutputBindInteger("1")
cur.defineOutputBindString("2",20)
cur.defineOutputBindDouble("3")
cur.defineOutputBindDate("4")
cur.defineOutputBindString("5",20)
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
assertEqual(isnegative,false)
nullvar=cur.getOutputBindString("5")
assertEqual(nullvar,nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# output bind by name
# db2 doesn't support bind by name


# output bind by name with validation
# db2 doesn't support bind by name


# lob output bind
print "LOB OUTPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)")
assertTrue(con.commit())
cur.prepareQuery("insert into testtable values ('hello',?)")
cur.inputBindBlob("1","hello",5)
assertTrue(cur.executeQuery())
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	out out1 clob, "+
	"	out out2 blob) "+
	"language sql "+
	"begin "+
	"	select testclob into out1 from testtable; "+
	"	select testblob into out2 from testtable; "+
	"end"))
assertTrue(con.commit())
cur.prepareQuery("call testproc(?,?)")
cur.defineOutputBindClob("1")
cur.defineOutputBindBlob("2")
assertTrue(cur.executeQuery())
clobvar=cur.getOutputBindClob("1")
clobvarlength=cur.getOutputBindLength("1")
blobvar=cur.getOutputBindBlob("2")
blobvarlength=cur.getOutputBindLength("2")
assertEqualLen(clobvar,"hello",5)
assertEqual(clobvarlength,5)
assertEqualLen(blobvar,"hello",5)
assertEqual(blobvarlength,5)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# long output bind
print "LONG OUTPUT BIND: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 clob, "+
	"	out out1 clob) "+
	"language sql "+
	"begin "+
	"	set out1 = in1; "+
	"end"))
assertTrue(con.commit())
largebuffer = "C" * (20*1024)
cur.prepareQuery("call testproc(?,?)")
cur.inputBindClob("1",largebuffer,20*1024)
cur.defineOutputBindClob("2")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindLength("2"),20*1024)
assertEqual(cur.getOutputBindClob("2"),largebuffer)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# negative input bind
print "NEGATIVE INPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery("create table testtable (testval integer)")
assertTrue(con.commit())
cur.prepareQuery("insert into testtable values (?)")
cur.inputBind("1",-1)
assertTrue(cur.executeQuery())
cur.sendQuery("select testval from testtable")
assertEqual(cur.getField(0,"TESTVAL"),"-1")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# bind validation
# db2 doesn't support bind by name


# rebinding
print "REBINDING: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int, "+
	"	out out1 int) "+
	"language sql "+
	"begin "+
	"	set out1 = in1; "+
	"end"))
assertTrue(con.commit())
cur.prepareQuery("call testproc(?,?)")
cur.inputBind("1",1)
cur.defineOutputBindInteger("2")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("2"),1)
cur.inputBind("1",2)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("2"),2)
cur.inputBind("1",3)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("2"),3)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# reexecute
print "REEXECUTE: \n"
cur.prepareQuery("select 1 from sysibm.sysdummy1")
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.prepareQuery("select cast(? as integer) from sysibm.sysdummy1")
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
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int, "+
	"	in in2 double, "+
	"	in in3 varchar(20)) "+
	"language sql "+
	"begin "+
	"	return; "+
	"end"))
assertTrue(con.commit())
cur.prepareQuery("call testproc(?,?,?)")
cur.inputBind("1",1)
cur.inputBind("2",2.5,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# stored procedure returning single value
print "STORED PROCEDURE RETURNING SINGLE VALUE: \n"
cur.sendQuery("drop function testfunc")
assertTrue(cur.sendQuery(
	"create function testfunc("+
	"	in1 int, "+
	"	in2 double, "+
	"	in3 varchar(20)) "+
	"returns int "+
	"language sql "+
	"begin "+
	"	return in1; "+
	"end"))
assertTrue(con.commit())
cur.prepareQuery("select testfunc(?,?,?) from sysibm.sysdummy1")
cur.inputBind("1",1)
cur.inputBind("2",2.5,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertTrue(cur.sendQuery("drop function testfunc"))
assertTrue(con.commit())
print "\n"


# stored procedure returning multiple values
print "STORED PROCEDURE RETURNING MULTIPLE VALUES: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int, "+
	"	in in2 double, "+
	"	in in3 varchar(20), "+
	"	in in4 clob, "+
	"	in in5 blob, "+
	"	out out1 int, "+
	"	out out2 double, "+
	"	out out3 varchar(20), "+
	"	out out4 clob, "+
	"	out out5 blob) "+
	"language sql "+
	"begin "+
	"	set out1 = in1; "+
	"	set out2 = in2; "+
	"	set out3 = in3; "+
	"	set out4 = in4; "+
	"	set out5 = in5; "+
	"end"))
assertTrue(con.commit())
cur.prepareQuery("call testproc(?,?,?,?,?,?,?,?,?,?)")
cur.inputBind("1",1)
cur.inputBind("2",2.5,2,1)
cur.inputBind("3","hello")
cur.inputBindClob("4","clob",4)
cur.inputBindBlob("5","blob",4)
cur.defineOutputBindInteger("6")
cur.defineOutputBindDouble("7")
cur.defineOutputBindString("8",20)
cur.defineOutputBindClob("9")
cur.defineOutputBindBlob("10")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("6"),1)
assertEqual(cur.getOutputBindDouble("7"),2.5)
assertEqual(cur.getOutputBindString("8"),"hello")
assertEqual(cur.getOutputBindClob("9"),"clob")
assertEqual(cur.getOutputBindBlob("10"),"blob")
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# stored procedure returning result set
print "STORED PROCEDURE RETURNING RESULT SET: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc() "+
	"result set 1 "+
	"language sql "+
	"begin "+
	"	declare c1 cursor with return for "+
	"		select 1 from sysibm.sysdummy1 "+
	"		union "+
	"		select 2 from sysibm.sysdummy1 "+
	"		union "+
	"		select 3 from sysibm.sysdummy1 "+
	"		union "+
	"		select 4 from sysibm.sysdummy1 "+
	"		union "+
	"		select 5 from sysibm.sysdummy1 "+
	"		union "+
	"		select 6 from sysibm.sysdummy1 "+
	"		union "+
	"		select 7 from sysibm.sysdummy1 "+
	"		union "+
	"		select 8 from sysibm.sysdummy1; "+
	"	open c1; "+
	"end"))
assertTrue(con.commit())
assertTrue(cur.sendQuery("call testproc()"))
assertEqual(cur.rowCount(),8)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# temporary tables
print "TEMPORARY TABLES: \n"
cur.sendQuery("drop table session.temptable")
assertTrue(cur.sendQuery("declare global temporary table session.temptable "+
					"(col1 int) not logged"))
assertTrue(cur.sendQuery("insert into session.temptable values (1)"))
assertTrue(cur.sendQuery("select count(*) from session.temptable"))
assertEqual(cur.getField(0,0),"1")
con.endSession()
print "\n"
assertFalse(cur.sendQuery("select count(*) from session.temptable"))
print "\n"


# encoded binary data
print "ENCODED BINARY DATA: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 blob)"))
buffer = (0..255).map { |j| j.chr }.join
query = "insert into testtable values (blob(X'"
query = query + buffer.bytes.map { |b| "%02x" % b }.join
query = query + "'))"
assertTrue(cur.sendQuery(query))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),buffer.length)
assertEqualLen(cur.getField(0,0),buffer,buffer.length)
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
		"	(col1 int not null "+
		"	generated always as identity, "+
		"	col2 int, "+
		"	primary key(col1))"))
assertTrue(cur.sendQuery(
		"insert into testtable (col2) values (1)"))
assertEqual(con.getLastInsertId(),1)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


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
assertInResultSet(cur,"Database","DB2INST1")
print "\n"


# table type list
print "TABLE TYPE LIST: \n"
assertTrue(cur.getTableTypeList())
assertEqual(cur.getColumnName(0),"table_type")
assertInResultSet(cur,"table_type","TABLE")
print "\n"


# table list
print "TABLE LIST: \n"
cur.sendQuery("drop table testtable1")
cur.sendQuery("drop table testtable2")
cur.sendQuery("drop table testtable3")
cur.sendQuery("drop table testtable4")
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(con.commit())
assertTrue(cur.getTableList(nil))
assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1")
assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2")
assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3")
assertInResultSet(cur,"Tables_in_xxx","TESTTABLE4")
assertTrue(cur.sendQuery("drop table testtable1"))
assertTrue(cur.sendQuery("drop table testtable2"))
assertTrue(cur.sendQuery("drop table testtable3"))
assertTrue(cur.sendQuery("drop table testtable4"))
assertTrue(con.commit())
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
assertEqual(cur.getField(0,"precision"),"254")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"32672")
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
	"	testsmallint smallint, "+
	"	testint integer, "+
	"	testbigint bigint, "+
	"	testdecimal decimal(10,2), "+
	"	testreal real, "+
	"	testdouble double, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(con.commit())
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
assertEqual(cur.getField(0,"column_name"),"TESTSMALLINT")
assertEqual(cur.getField(1,"column_name"),"TESTINT")
assertEqual(cur.getField(2,"column_name"),"TESTBIGINT")
assertEqual(cur.getField(3,"column_name"),"TESTDECIMAL")
assertEqual(cur.getField(4,"column_name"),"TESTREAL")
assertEqual(cur.getField(5,"column_name"),"TESTDOUBLE")
assertEqual(cur.getField(6,"column_name"),"TESTCHAR")
assertEqual(cur.getField(7,"column_name"),"TESTVARCHAR")
assertEqual(cur.getField(8,"column_name"),"TESTDATE")
assertEqual(cur.getField(9,"column_name"),"TESTTIME")
assertEqual(cur.getField(10,"column_name"),"TESTTIMESTAMP")
assertEqual(cur.getField(11,"column_name"),"TESTCLOB")
assertEqual(cur.getField(12,"column_name"),"TESTBLOB")
assertEqual(cur.getField(0,"data_type"),"SMALLINT")
assertEqual(cur.getField(1,"data_type"),"INTEGER")
assertEqual(cur.getField(2,"data_type"),"BIGINT")
assertEqual(cur.getField(3,"data_type"),"DECIMAL")
assertEqual(cur.getField(4,"data_type"),"REAL")
assertEqual(cur.getField(5,"data_type"),"DOUBLE")
assertEqual(cur.getField(6,"data_type"),"CHARACTER")
assertEqual(cur.getField(7,"data_type"),"VARCHAR")
assertEqual(cur.getField(8,"data_type"),"DATE")
assertEqual(cur.getField(9,"data_type"),"TIME")
assertEqual(cur.getField(10,"data_type"),"TIMESTAMP")
assertEqual(cur.getField(11,"data_type"),"CLOB")
assertEqual(cur.getField(12,"data_type"),"BLOB")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# column list - auto_increment, primary key
print "COLUMN LIST - auto_increment, primary key: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int generated always as identity primary key, "+
	"	col2 int)"))
assertTrue(con.commit())
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getField(0,"extra"),"auto_increment")
assertEqual(cur.getField(0,"column_key"),"PRI")
assertEqual(cur.getField(1,"extra"),"")
assertEqual(cur.getField(1,"column_key"),"")
print "\n"
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int not null primary key, "+
	"	col2 int)"))
assertTrue(con.commit())
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getField(0,"extra"),"")
assertEqual(cur.getField(0,"column_key"),"PRI")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# primary keys list
print "PRIMARY KEYS LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int not null primary key, "+
	"	col2 int)"))
assertTrue(con.commit())
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
assertTrue(cur.getField(0,"table")=="TESTTABLE")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertTrue(cur.getField(0,"column_name")=="COL1")
assertTrue(!(cur.getField(0,"key_name").nil? || cur.getField(0,"key_name")==""))
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# key and index list
print "KEY AND INDEX LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int not null primary key, "+
	"	col2 int)"))
assertTrue(con.commit())
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
assertTrue(cur.getField(0,"table")=="TESTTABLE")
assertEqual(cur.getField(0,"non_unique"),"0")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertTrue(cur.getField(0,"column_name")=="COL1")
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"3")
assertTrue(!(cur.getField(0,"key_name").nil? || cur.getField(0,"key_name")==""))
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
cur.sendQuery("drop procedure testproc1")
cur.sendQuery("drop procedure testproc2")
cur.sendQuery("drop procedure testproc3")
cur.sendQuery("drop procedure testproc4")
assertTrue(cur.sendQuery(
	"create procedure testproc1("+
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"language sql begin end"))
assertTrue(cur.sendQuery(
	"create procedure testproc2("+
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"language sql begin end"))
assertTrue(cur.sendQuery(
	"create procedure testproc3("+
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"language sql begin end"))
assertTrue(cur.sendQuery(
	"create procedure testproc4("+
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"language sql begin end"))
assertTrue(con.commit())
assertTrue(cur.getProcedureList(nil))
assertInResultSet(cur,"routine_name","TESTPROC1")
assertInResultSet(cur,"routine_name","TESTPROC2")
assertInResultSet(cur,"routine_name","TESTPROC3")
assertInResultSet(cur,"routine_name","TESTPROC4")
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
assertEqual(cur.getField(0,"data_type"),"INTEGER")
assertEqual(cur.getField(0,"ordinal_position"),"1")
assertEqual(cur.getField(1,"parameter_name"),"IN2")
assertEqual(cur.getField(1,"parameter_mode"),"1")
assertEqual(cur.getField(1,"data_type"),"CHARACTER")
assertEqual(cur.getField(1,"ordinal_position"),"2")
assertEqual(cur.getField(2,"parameter_name"),"IN3")
assertEqual(cur.getField(2,"parameter_mode"),"1")
assertEqual(cur.getField(2,"data_type"),"VARCHAR")
assertEqual(cur.getField(2,"ordinal_position"),"3")
assertEqual(cur.getField(3,"parameter_name"),"IN4")
assertEqual(cur.getField(3,"parameter_mode"),"1")
assertEqual(cur.getField(3,"data_type"),"DATE")
assertEqual(cur.getField(3,"ordinal_position"),"4")
assertTrue(cur.sendQuery("drop procedure testproc1"))
assertTrue(cur.sendQuery("drop procedure testproc2"))
assertTrue(cur.sendQuery("drop procedure testproc3"))
assertTrue(cur.sendQuery("drop procedure testproc4"))
assertTrue(con.commit())
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
