#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'sqlrelay'
require './asserts'



bindvars=["1","2","3","4","5","6",
		"7","8","9","10","11","12"]
bindvals=["7","7","7.7","7.7","7.7","7.7",
		"01-JAN-2007","07:00:00",
		"testchar7","testvarchar7",nil,"testblob7"]
subvars=["var1","var2","var3"]
subvallongs=[1,2,3]
subvalstrings=["hi","hello","bye"]
subvaldoubles=[10.55,10.556,10.5556]
precs=[4,5,6]
scales=[2,3,4]

largebuffer = "C" * (20*1024)


# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
cur=SQLRCursor.new(con)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"firebird")
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
assertEqual(con.nextvalFormat(),"next value for %s")
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
# though firebird does support a "set transaction ..." statement to
# set the isolation level, it looks like, in firebird, you can really
# only set it through the TPB at the start of a transaction, so
# attempts to set it should fail
assertFalse(con.setIsolationLevel("read committed"))
assertEqual(con.getIsolationLevel(),"read committed")
print "\n"


# insert
print "INSERT: \n"
cur.sendQuery("delete from testtable")
con.commit()
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1, "+
	"	1.1, "+
	"	1.1, "+
	"	1.1, "+
	"	1.1, "+
	"	'01-JAN-2001', "+
	"	'01:00:00', "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	NULL, "+
	"	'testblob1')"))
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
	"	?, "+
	"	?)")
assertEqual(cur.countBindVariables(),12)
cur.inputBind("1",2)
cur.inputBind("2",2)
cur.inputBind("3",2.2,2,1)
cur.inputBind("4",2.2,2,1)
cur.inputBind("5",2.2,2,1)
cur.inputBind("6",2.2,2,1)
cur.inputBindDate("7",2002,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("8",-1,-1,-1,2,0,0,0,"",0)
cur.inputBind("9","testchar2")
cur.inputBind("10","testvarchar2")
cur.inputBind("11",nil)
cur.inputBindBlob("12","testblob2","testblob2".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",3)
cur.inputBind("2",3)
cur.inputBind("3",3.3,2,1)
cur.inputBind("4",3.3,2,1)
cur.inputBind("5",3.3,2,1)
cur.inputBind("6",3.3,2,1)
cur.inputBindDate("7",2003,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("8",-1,-1,-1,3,0,0,0,"",0)
cur.inputBind("9","testchar3")
cur.inputBind("10","testvarchar3")
cur.inputBind("11",nil)
cur.inputBindBlob("12","testblob3","testblob3".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",4)
cur.inputBind("2",4)
cur.inputBind("3",4.4,2,1)
cur.inputBind("4",4.4,2,1)
cur.inputBind("5",4.4,2,1)
cur.inputBind("6",4.4,2,1)
cur.inputBindDate("7",2004,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("8",-1,-1,-1,4,0,0,0,"",0)
cur.inputBind("9","testchar4")
cur.inputBind("10","testvarchar4")
cur.inputBind("11",nil)
cur.inputBindBlob("12","testblob4","testblob4".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",5)
cur.inputBind("2",5)
cur.inputBind("3",5.5,2,1)
cur.inputBind("4",5.5,2,1)
cur.inputBind("5",5.5,2,1)
cur.inputBind("6",5.5,2,1)
cur.inputBindDate("7",2005,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("8",-1,-1,-1,5,0,0,0,"",0)
cur.inputBind("9","testchar5")
cur.inputBind("10","testvarchar5")
cur.inputBind("11",nil)
cur.inputBindBlob("12","testblob5","testblob5".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",6)
cur.inputBind("2",6)
cur.inputBind("3",6.6,2,1)
cur.inputBind("4",6.6,2,1)
cur.inputBind("5",6.6,2,1)
cur.inputBind("6",6.6,2,1)
cur.inputBindDate("7",2006,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("8",-1,-1,-1,6,0,0,0,"",0)
cur.inputBind("9","testchar6")
cur.inputBind("10","testvarchar6")
cur.inputBind("11",nil)
cur.inputBindBlob("12","testblob6","testblob6".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by position
print "ARRAY OF INPUT BINDS BY POSITION: \n"
cur.clearBinds()
cur.inputBinds(bindvars,bindvals)
assertTrue(cur.executeQuery())
print "\n"


# input bind by position with validation
print "INPUT BIND BY POSITION WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("1",8)
cur.inputBind("2",8)
cur.inputBind("3",8.8,2,1)
cur.inputBind("4",8.8,2,1)
cur.inputBind("5",8.8,2,1)
cur.inputBind("6",8.8,2,1)
cur.inputBindDate("7",2008,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("8",-1,-1,-1,8,0,0,0,"",0)
cur.inputBind("9","testchar8")
cur.inputBind("10","testvarchar8")
cur.inputBind("11",nil)
cur.inputBindBlob("12","testblob8","testblob8".to_s.bytesize)
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# input bind by name
# firebird doesn't support bind by name


# array of input binds by name
# firebird doesn't support bind by name


# input bind by name with validation
# firebird doesn't support bind by name


# select
print "SELECT: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),12)
print "\n"


# column names
print "COLUMN NAMES: \n"
assertEqual(cur.getColumnName(0),"TESTINTEGER")
assertEqual(cur.getColumnName(1),"TESTSMALLINT")
assertEqual(cur.getColumnName(2),"TESTDECIMAL")
assertEqual(cur.getColumnName(3),"TESTNUMERIC")
assertEqual(cur.getColumnName(4),"TESTFLOAT")
assertEqual(cur.getColumnName(5),"TESTDOUBLE")
assertEqual(cur.getColumnName(6),"TESTDATE")
assertEqual(cur.getColumnName(7),"TESTTIME")
assertEqual(cur.getColumnName(8),"TESTCHAR")
assertEqual(cur.getColumnName(9),"TESTVARCHAR")
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP")
assertEqual(cur.getColumnName(11),"TESTBLOB")
cols=cur.getColumnNames()
assertEqual(cols[0],"TESTINTEGER")
assertEqual(cols[1],"TESTSMALLINT")
assertEqual(cols[2],"TESTDECIMAL")
assertEqual(cols[3],"TESTNUMERIC")
assertEqual(cols[4],"TESTFLOAT")
assertEqual(cols[5],"TESTDOUBLE")
assertEqual(cols[6],"TESTDATE")
assertEqual(cols[7],"TESTTIME")
assertEqual(cols[8],"TESTCHAR")
assertEqual(cols[9],"TESTVARCHAR")
assertEqual(cols[10],"TESTTIMESTAMP")
assertEqual(cols[11],"TESTBLOB")
print "\n"


# column types
print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"INTEGER")
assertEqual(cur.getColumnType("TESTINTEGER"),"INTEGER")
assertEqual(cur.getColumnType(1),"SMALLINT")
assertEqual(cur.getColumnType("TESTSMALLINT"),"SMALLINT")
assertEqual(cur.getColumnType(2),"DECIMAL")
assertEqual(cur.getColumnType("TESTDECIMAL"),"DECIMAL")
assertEqual(cur.getColumnType(3),"NUMERIC")
assertEqual(cur.getColumnType("TESTNUMERIC"),"NUMERIC")
assertEqual(cur.getColumnType(4),"FLOAT")
assertEqual(cur.getColumnType("TESTFLOAT"),"FLOAT")
assertEqual(cur.getColumnType(5),"DOUBLE PRECISION")
assertEqual(cur.getColumnType("TESTDOUBLE"),"DOUBLE PRECISION")
assertEqual(cur.getColumnType(6),"DATE")
assertEqual(cur.getColumnType("TESTDATE"),"DATE")
assertEqual(cur.getColumnType(7),"TIME")
assertEqual(cur.getColumnType("TESTTIME"),"TIME")
assertEqual(cur.getColumnType(8),"CHAR")
assertEqual(cur.getColumnType("TESTCHAR"),"CHAR")
assertEqual(cur.getColumnType(9),"VARCHAR")
assertEqual(cur.getColumnType("TESTVARCHAR"),"VARCHAR")
assertEqual(cur.getColumnType(10),"TIMESTAMP")
assertEqual(cur.getColumnType("TESTTIMESTAMP"),"TIMESTAMP")
assertEqual(cur.getColumnType(11),"BLOB")
assertEqual(cur.getColumnType("TESTBLOB"),"BLOB")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnLength("TESTINTEGER"),4)
assertEqual(cur.getColumnLength(1),2)
assertEqual(cur.getColumnLength("TESTSMALLINT"),2)
assertEqual(cur.getColumnLength(2),8)
assertEqual(cur.getColumnLength("TESTDECIMAL"),8)
assertEqual(cur.getColumnLength(3),8)
assertEqual(cur.getColumnLength("TESTNUMERIC"),8)
assertEqual(cur.getColumnLength(4),4)
assertEqual(cur.getColumnLength("TESTFLOAT"),4)
assertEqual(cur.getColumnLength(5),8)
assertEqual(cur.getColumnLength("TESTDOUBLE"),8)
assertEqual(cur.getColumnLength(6),4)
assertEqual(cur.getColumnLength("TESTDATE"),4)
assertEqual(cur.getColumnLength(7),4)
assertEqual(cur.getColumnLength("TESTTIME"),4)
assertEqual(cur.getColumnLength(8),50)
assertEqual(cur.getColumnLength("TESTCHAR"),50)
assertEqual(cur.getColumnLength(9),50)
assertEqual(cur.getColumnLength("TESTVARCHAR"),50)
assertEqual(cur.getColumnLength(10),8)
assertEqual(cur.getColumnLength("TESTTIMESTAMP"),8)
assertEqual(cur.getColumnLength(11),8)
assertEqual(cur.getColumnLength("TESTBLOB"),8)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest("TESTINTEGER"),1)
assertEqual(cur.getLongest(1),1)
assertEqual(cur.getLongest("TESTSMALLINT"),1)
assertEqual(cur.getLongest(2),4)
assertEqual(cur.getLongest("TESTDECIMAL"),4)
assertEqual(cur.getLongest(3),4)
assertEqual(cur.getLongest("TESTNUMERIC"),4)
assertEqual(cur.getLongest(4),6)
assertEqual(cur.getLongest("TESTFLOAT"),6)
assertEqual(cur.getLongest(5),6)
assertEqual(cur.getLongest("TESTDOUBLE"),6)
assertEqual(cur.getLongest(6),10)
assertEqual(cur.getLongest("TESTDATE"),10)
assertEqual(cur.getLongest(7),8)
assertEqual(cur.getLongest("TESTTIME"),8)
assertEqual(cur.getLongest(8),50)
assertEqual(cur.getLongest("TESTCHAR"),50)
assertEqual(cur.getLongest(9),12)
assertEqual(cur.getLongest("TESTVARCHAR"),12)
assertEqual(cur.getLongest(10),0)
assertEqual(cur.getLongest("TESTTIMESTAMP"),0)
assertEqual(cur.getLongest(11),9)
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
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),"1.10")
assertEqual(cur.getField(0,3),"1.10")
assertEqual(cur.getField(0,4),"1.1000")
assertEqual(cur.getField(0,5),"1.1000")
assertEqual(cur.getField(0,6),"2001:01:01")
assertEqual(cur.getField(0,7),"01:00:00")
assertEqual(cur.getField(0,8),"testchar1                                         ")
assertEqual(cur.getField(0,9),"testvarchar1")
assertEqual(cur.getField(0,11),"testblob1")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(7,2),"8.80")
assertEqual(cur.getField(7,3),"8.80")
assertEqual(cur.getField(7,4),"8.8000")
assertEqual(cur.getField(7,5),"8.8000")
assertEqual(cur.getField(7,6),"2008:01:01")
assertEqual(cur.getField(7,7),"08:00:00")
assertEqual(cur.getField(7,8),"testchar8                                         ")
assertEqual(cur.getField(7,9),"testvarchar8")
assertEqual(cur.getField(7,11),"testblob8")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),1)
assertEqual(cur.getFieldLength(0,2),4)
assertEqual(cur.getFieldLength(0,3),4)
assertEqual(cur.getFieldLength(0,4),6)
assertEqual(cur.getFieldLength(0,5),6)
assertEqual(cur.getFieldLength(0,6),10)
assertEqual(cur.getFieldLength(0,7),8)
assertEqual(cur.getFieldLength(0,8),50)
assertEqual(cur.getFieldLength(0,9),12)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),1)
assertEqual(cur.getFieldLength(7,2),4)
assertEqual(cur.getFieldLength(7,3),4)
assertEqual(cur.getFieldLength(7,4),6)
assertEqual(cur.getFieldLength(7,5),6)
assertEqual(cur.getFieldLength(7,6),10)
assertEqual(cur.getFieldLength(7,7),8)
assertEqual(cur.getFieldLength(7,8),50)
assertEqual(cur.getFieldLength(7,9),12)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"TESTINTEGER"),"1")
assertEqual(cur.getField(0,"TESTSMALLINT"),"1")
assertEqual(cur.getField(0,"TESTDECIMAL"),"1.10")
assertEqual(cur.getField(0,"TESTNUMERIC"),"1.10")
assertEqual(cur.getField(0,"TESTFLOAT"),"1.1000")
assertEqual(cur.getField(0,"TESTDOUBLE"),"1.1000")
assertEqual(cur.getField(0,"TESTDATE"),"2001:01:01")
assertEqual(cur.getField(0,"TESTTIME"),"01:00:00")
assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                                         ")
assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1")
assertEqual(cur.getField(0,"TESTBLOB"),"testblob1")
print "\n"
assertEqual(cur.getField(7,"TESTINTEGER"),"8")
assertEqual(cur.getField(7,"TESTSMALLINT"),"8")
assertEqual(cur.getField(7,"TESTDECIMAL"),"8.80")
assertEqual(cur.getField(7,"TESTNUMERIC"),"8.80")
assertEqual(cur.getField(7,"TESTFLOAT"),"8.8000")
assertEqual(cur.getField(7,"TESTDOUBLE"),"8.8000")
assertEqual(cur.getField(7,"TESTDATE"),"2008:01:01")
assertEqual(cur.getField(7,"TESTTIME"),"08:00:00")
assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                                         ")
assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8")
assertEqual(cur.getField(7,"TESTBLOB"),"testblob8")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"TESTINTEGER"),1)
assertEqual(cur.getFieldLength(0,"TESTSMALLINT"),1)
assertEqual(cur.getFieldLength(0,"TESTDECIMAL"),4)
assertEqual(cur.getFieldLength(0,"TESTNUMERIC"),4)
assertEqual(cur.getFieldLength(0,"TESTFLOAT"),6)
assertEqual(cur.getFieldLength(0,"TESTDOUBLE"),6)
assertEqual(cur.getFieldLength(0,"TESTDATE"),10)
assertEqual(cur.getFieldLength(0,"TESTTIME"),8)
assertEqual(cur.getFieldLength(0,"TESTCHAR"),50)
assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12)
print "\n"
assertEqual(cur.getFieldLength(7,"TESTINTEGER"),1)
assertEqual(cur.getFieldLength(7,"TESTSMALLINT"),1)
assertEqual(cur.getFieldLength(7,"TESTDECIMAL"),4)
assertEqual(cur.getFieldLength(7,"TESTNUMERIC"),4)
assertEqual(cur.getFieldLength(7,"TESTFLOAT"),6)
assertEqual(cur.getFieldLength(7,"TESTDOUBLE"),6)
assertEqual(cur.getFieldLength(7,"TESTDATE"),10)
assertEqual(cur.getFieldLength(7,"TESTTIME"),8)
assertEqual(cur.getFieldLength(7,"TESTCHAR"),50)
assertEqual(cur.getFieldLength(7,"TESTVARCHAR"),12)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1")
assertEqual(fields[2],"1.10")
assertEqual(fields[3],"1.10")
assertEqual(fields[4],"1.1000")
assertEqual(fields[5],"1.1000")
assertEqual(fields[6],"2001:01:01")
assertEqual(fields[7],"01:00:00")
assertEqual(fields[8],"testchar1                                         ")
assertEqual(fields[9],"testvarchar1")
assertEqual(fields[11],"testblob1")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],1)
assertEqual(fieldlens[2],4)
assertEqual(fieldlens[3],4)
assertEqual(fieldlens[4],6)
assertEqual(fieldlens[5],6)
assertEqual(fieldlens[6],10)
assertEqual(fieldlens[7],8)
assertEqual(fieldlens[8],50)
assertEqual(fieldlens[9],12)
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
	"	testinteger "))
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
	"	testinteger "))
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
	"	testinteger "))
assertEqual(cur.getColumnName(0),"TESTINTEGER")
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnType(0),"INTEGER")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "))
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
	"	testinteger "))
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
	"	testinteger "))
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
	"	testinteger "))
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
	"	testinteger "))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
print "\n"


# column count for cached result set
print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),12)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
assertEqual(cur.getColumnName(0),"TESTINTEGER")
assertEqual(cur.getColumnName(1),"TESTSMALLINT")
assertEqual(cur.getColumnName(2),"TESTDECIMAL")
assertEqual(cur.getColumnName(3),"TESTNUMERIC")
assertEqual(cur.getColumnName(4),"TESTFLOAT")
assertEqual(cur.getColumnName(5),"TESTDOUBLE")
assertEqual(cur.getColumnName(6),"TESTDATE")
assertEqual(cur.getColumnName(7),"TESTTIME")
assertEqual(cur.getColumnName(8),"TESTCHAR")
assertEqual(cur.getColumnName(9),"TESTVARCHAR")
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP")
assertEqual(cur.getColumnName(11),"TESTBLOB")
cols=cur.getColumnNames()
assertEqual(cols[0],"TESTINTEGER")
assertEqual(cols[1],"TESTSMALLINT")
assertEqual(cols[2],"TESTDECIMAL")
assertEqual(cols[3],"TESTNUMERIC")
assertEqual(cols[4],"TESTFLOAT")
assertEqual(cols[5],"TESTDOUBLE")
assertEqual(cols[6],"TESTDATE")
assertEqual(cols[7],"TESTTIME")
assertEqual(cols[8],"TESTCHAR")
assertEqual(cols[9],"TESTVARCHAR")
assertEqual(cols[10],"TESTTIMESTAMP")
assertEqual(cols[11],"TESTBLOB")
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
	"	testinteger "))
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
	"	testinteger "))
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
	"	testinteger "))
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
# truncate testtable so this section starts with it empty;
# firebird DDL on the table here would otherwise hit cursor-state
# issues at the next commit, so we reuse the existing schema and
# just write to one column (testinteger)
assertTrue(cur.sendQuery("delete from testtable"))
# commit so the truncation is visible to the second connection
# (the commit implicitly starts a new tx)
assertTrue(con.commit())
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
					"testuser","testpassword",0,1)
secondcur=SQLRCursor.new(secondcon)
setSecondConnection(secondcon)
setSecondCursor(secondcur)
# session is in a transaction; insert is not visible until commit
assertTrue(con.getInTransaction())
assertFalse(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
# commit makes it visible, and implicitly starts a new transaction
assertTrue(con.commit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# rollback discards, and implicitly starts a new transaction
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"))
assertTrue(con.rollback())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
secondcur.closeResultSet()
print "\n"


# transaction behavior - explicit
print "TRANSACTION BEHAVIOR - explicit: \n"
assertTrue(con.setTransactionModel("explicit"))
assertEqual(con.getTransactionModel(),"explicit")
# truncate testtable so this section starts with it empty (delete
# autocommits here since explicit-model defaults to autocommit-on)
assertTrue(cur.sendQuery("delete from testtable"))
# begin starts a new transaction; insert is not visible until commit
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
# commit makes it visible; no new transaction is started
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback discards; no new transaction is started
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"))
assertTrue(con.rollback())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
secondcur.closeResultSet()
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
# truncate testtable so this section starts with it empty
assertTrue(cur.sendQuery("delete from testtable"))
# begin starts a transaction; commit makes it visible
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"))
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback discards
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# during a transaction started by begin(), autoCommitOn is a
# no-op: the autocommit setting takes effect after the user
# explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"))
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
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (4)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"3")
# autoCommitOff takes effect immediately when not in a transaction
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
# autocommit-off persists across commit/rollback; each commit or
# rollback ends the current implicit tx and a new one starts for
# the next statement
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (5)"))
assertTrue(con.commit())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"4")
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (6)"))
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
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (7)"))
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
print "\n"


# transaction behavior - explicit-error
print "TRANSACTION BEHAVIOR - explicit-error: \n"
assertTrue(con.setTransactionModel("explicit-error"))
assertEqual(con.getTransactionModel(),"explicit-error")
# truncate testtable so this section starts with it empty
assertTrue(cur.sendQuery("delete from testtable"))
# begin, insert, commit
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"))
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"))
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
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
secondcur.closeResultSet()
print "\n"


# transaction behavior - none
print "TRANSACTION BEHAVIOR - none: \n"
assertTrue(con.setTransactionModel("none"))
assertEqual(con.getTransactionModel(),"none")
# truncate testtable so this section starts with it empty
assertTrue(cur.sendQuery("delete from testtable"))
# no transactions; everything is visible immediately
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# commit and rollback are no-ops
assertTrue(con.commit())
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autocommit is always on; autoCommitOff is an error
assertFalse(con.autoCommitOff())
assertTrue(con.getAutoCommit())
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
secondcur.closeResultSet()
print "\n"


# reset transaction behavior
print "RESET TRANSACTION BEHAVIOR: \n"
assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()))
assertEqual(con.getTransactionModel(),"implicit")
assertFalse(con.getAutoCommit())
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)',$(var3) from rdb$database")
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
cur.prepareQuery(
	"select "+
	"	'$(var1)', "+
	"	'$(var2)', "+
	"	'$(var3)' "+
	"from "+
	"	rdb$database ")
cur.substitutions(subvars,subvalstrings)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"hi")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"bye")
print "\n"
cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database")
cur.substitutions(subvars,subvallongs)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"2")
assertEqual(cur.getField(0,2),"3")
print "\n"
cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database")
cur.substitutions(subvars,subvaldoubles,precs,scales)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"10.55")
assertEqual(cur.getField(0,1),"10.556")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# nulls as nulls
print "NULLS AS NULLS: \n"
cur.getNullsAsNils()
assertTrue(cur.sendQuery("select 1,NULL,NULL from rdb$database"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select 1,NULL,NULL from rdb$database"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"")
assertEqual(cur.getField(0,2),"")
print "\n"


# null and empty lobs
print "NULL AND EMPTY LOBS: \n"
cur.getNullsAsNils()
cur.sendQuery("delete from testtable1")
cur.prepareQuery("insert into testtable1 values (?)")
cur.inputBindBlob("1","","".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select testblob from testtable1")
assertEqual(cur.getField(0,"TESTBLOB"),"")
cur.sendQuery("delete from testtable1")
cur.prepareQuery("insert into testtable1 values (?)")
cur.inputBindBlob("1",nil,nil.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select testblob from testtable1")
assertEqual(cur.getField(0,"TESTBLOB"),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("delete from testtable1"))
print "\n"


# long lobs
print "LONG LOBS: \n"
cur.sendQuery("delete from testtable1")
cur.prepareQuery("insert into testtable1 values (?)")
cur.inputBindClob("1",largebuffer,largebuffer.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select testblob from testtable1")
assertEqual(cur.getFieldLength(0,"TESTBLOB"),20*1024)
assertEqual(cur.getField(0,"TESTBLOB"),largebuffer)
assertTrue(cur.sendQuery("delete from testtable1"))
print "\n"


# output bind by position
print "OUTPUT BIND BY POSITION: \n"
cur.getNullsAsNils()
cur.prepareQuery("execute procedure testproc ?, ?, ?, ?")
cur.inputBind("1",1)
cur.inputBind("2",1.1,2,1)
cur.inputBind("3","hello")
cur.inputBindBlob("4","blob","blob".to_s.bytesize)
cur.defineOutputBindInteger("1")
cur.defineOutputBindDouble("2")
cur.defineOutputBindString("3",20)
cur.defineOutputBindBlob("4")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("1"),1)
d=cur.getOutputBindDouble("2")
assertTrue(d>1.09 && d<1.11)
assertEqual(cur.getOutputBindString("3"),"hello               ")
assertEqual(cur.getOutputBindBlob("4"),"blob")
cur.getNullsAsEmptyStrings()
print "\n"


# output bind by name
# firebird doesn't support bind by name


# output bind by name with validation
# firebird doesn't support bind by name


# lob output bind
print "LOB OUTPUT BIND: \n"
cur.prepareQuery("execute procedure testproc1 ?")
cur.inputBindBlob("1","hello","hello".to_s.bytesize)
cur.defineOutputBindBlob("1")
assertTrue(cur.executeQuery())
assertEqualLen(cur.getOutputBindBlob("1"),"hello",5)
assertEqual(cur.getOutputBindLength("1"),5)
print "\n"


# long output bind
print "LONG OUTPUT BIND: \n"
cur.prepareQuery("execute procedure testproc1 ?")
cur.inputBindBlob("1",largebuffer,largebuffer.to_s.bytesize)
cur.defineOutputBindBlob("1")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindLength("1"),20*1024)
assertEqualLen(cur.getOutputBindBlob("1"),largebuffer,20*1024)
print "\n"


# negative input bind
print "NEGATIVE INPUT BIND: \n"
cur.prepareQuery("select cast(? as integer) from rdb$database")
cur.inputBind("1",-1)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"-1")
print "\n"


# bind validation
# firebird doesn't support bind by name


# rebinding
print "REBINDING: \n"
cur.prepareQuery("execute procedure testproc ?, ?, ?, ?")
cur.inputBind("1",1)
cur.inputBind("2",1.1,2,1)
cur.inputBind("3","hello")
cur.inputBindBlob("4","blob","blob".to_s.bytesize)
cur.defineOutputBindInteger("1")
cur.defineOutputBindDouble("2")
cur.defineOutputBindString("3",20)
cur.defineOutputBindBlob("4")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("1"),1)
cur.inputBind("1",2)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("1"),2)
cur.inputBind("1",3)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("1"),3)
print "\n"


# reexecute
print "REEXECUTE: \n"
cur.prepareQuery("select 1 from rdb$database")
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.prepareQuery("select cast(? as int) from rdb$database")
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
cur.prepareQuery(
	"execute block (in1 int = ?, "+
	"	in2 double precision = ?, "+
	"	in3 varchar(20) = ?) "+
	"as "+
	"begin "+
	"end")
cur.inputBind("1",1)
cur.inputBind("2",1.1,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
print "\n"


# stored procedure returning single value
print "STORED PROCEDURE RETURNING SINGLE VALUE: \n"
cur.prepareQuery(
	"execute block (in1 int = ?, "+
	"	in2 double precision = ?, "+
	"	in3 varchar(20) = ?) "+
	"returns (out1 int) "+
	"as "+
	"begin "+
	"	out1 = in1; "+
	"	suspend; "+
	"end")
cur.inputBind("1",1)
cur.inputBind("2",1.1,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
print "\n"


# stored procedure returning multiple values
print "STORED PROCEDURE RETURNING MULTIPLE VALUES: \n"
cur.prepareQuery(
	"execute block (in1 int = ?, "+
	"	in2 double precision = ?, "+
	"	in3 varchar(20) = ?) "+
	"returns (out1 int, "+
	"	out2 double precision, "+
	"	out3 varchar(20)) "+
	"as "+
	"begin "+
	"	out1 = in1; "+
	"	out2 = in2; "+
	"	out3 = in3; "+
	"	suspend; "+
	"end")
cur.inputBind("1",1)
cur.inputBind("2",1.1,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"1.1000")
assertEqual(cur.getField(0,2),"hello")
print "\n"


# stored procedure returning result set
print "STORED PROCEDURE RETURNING RESULT SET: \n"
cur.prepareQuery(
	"execute block "+
	"returns (out1 int) "+
	"as "+
	"declare i int; "+
	"begin "+
	"	i = 1; "+
	"	while (i <= 8) do "+
	"	begin "+
	"		out1 = i; "+
	"		suspend; "+
	"		i = i + 1; "+
	"	end "+
	"end")
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),8)
print "\n"


# temporary tables
# firebird supports temporary tables, but we're omitting this for now


# encoded binary data
# firebird doesn't support encoded binary data


# quotes
print "QUOTES: \n"
cur.sendQuery("delete from table testtable1")
assertTrue(cur.sendQuery(
		"insert into testtable1 values ('''''')"))
assertTrue(cur.sendQuery("select testblob from testtable1"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"''")
assertTrue(cur.sendQuery("delete from testtable1"))
print "\n"


# last insert id
# firebird doesn't support auto-increment


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
found=false
for i in 0..cur.rowCount()-1
	if cur.getField(i,"Database")=="TESTUSER"
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
assertTrue(cur.getTableList(nil))
counter=0
for i in 0..cur.rowCount()-1
	name=cur.getField(i,"Tables_in_xxx")
	if name=="TESTTABLE1" ||
		name=="TESTTABLE2" ||
		name=="TESTTABLE3"
		counter=counter+1
	end
end
assertEqual(counter,3)
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
assertEqual(cur.getField(0,"precision"),"32767")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"32765")
assertEqual(cur.getField(0,"local_type_name"),"VARCHAR")
assertTrue(cur.getTypeInfoList("date"))
assertEqual(cur.getField(0,"type_name"),"DATE")
assertEqual(cur.getField(0,"data_type"),"91")
assertEqual(cur.getField(0,"precision"),"10")
assertEqual(cur.getField(0,"local_type_name"),"DATE")
print "\n"


# column list
print "COLUMN LIST: \n"
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
assertEqual(cur.getField(0,"column_name"),"TESTINTEGER")
assertEqual(cur.getField(1,"column_name"),"TESTSMALLINT")
assertEqual(cur.getField(2,"column_name"),"TESTDECIMAL")
assertEqual(cur.getField(3,"column_name"),"TESTNUMERIC")
assertEqual(cur.getField(4,"column_name"),"TESTFLOAT")
assertEqual(cur.getField(5,"column_name"),"TESTDOUBLE")
assertEqual(cur.getField(6,"column_name"),"TESTDATE")
assertEqual(cur.getField(7,"column_name"),"TESTTIME")
assertEqual(cur.getField(8,"column_name"),"TESTCHAR")
assertEqual(cur.getField(9,"column_name"),"TESTVARCHAR")
assertEqual(cur.getField(10,"column_name"),"TESTTIMESTAMP")
assertEqual(cur.getField(11,"column_name"),"TESTBLOB")
assertEqual(cur.getField(0,"data_type"),"INTEGER")
assertEqual(cur.getField(1,"data_type"),"SMALLINT")
assertEqual(cur.getField(2,"data_type"),"DECIMAL")
assertEqual(cur.getField(3,"data_type"),"NUMERIC")
assertEqual(cur.getField(4,"data_type"),"FLOAT")
assertEqual(cur.getField(5,"data_type"),"DOUBLE PRECISION")
assertEqual(cur.getField(6,"data_type"),"DATE")
assertEqual(cur.getField(7,"data_type"),"TIME")
assertEqual(cur.getField(8,"data_type"),"CHAR")
assertEqual(cur.getField(9,"data_type"),"VARCHAR")
assertEqual(cur.getField(10,"data_type"),"TIMESTAMP")
assertEqual(cur.getField(11,"data_type"),"BLOB SUB_TYPE BINARY")
print "\n"


# column list - auto_increment, primary key
print "COLUMN LIST - auto_increment, primary key: \n"
assertTrue(cur.getColumnList("testtable2",nil))
assertTrue(cur.getField(0,"extra").include?("auto_increment"))
assertTrue(cur.getField(0,"column_key").include?("PRI"))
assertFalse(cur.getField(1,"extra").include?("auto_increment"))
assertFalse(cur.getField(1,"column_key").include?("PRI"))
print "\n"
assertTrue(cur.getColumnList("testtable3",nil))
assertFalse(cur.getField(0,"extra").include?("auto_increment"))
assertTrue(cur.getField(0,"column_key").include?("PRI"))
print "\n"


# primary keys list
print "PRIMARY KEYS LIST: \n"
assertTrue(cur.getPrimaryKeysList("testtable2",nil))
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
assertEqual(cur.getField(0,"table"),"TESTTABLE2")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"COL1")
assertFalse(cur.getField(0,"key_name").nil? || cur.getField(0,"key_name").empty?)
print "\n"


# key and index list
print "KEY AND INDEX LIST: \n"
assertTrue(cur.getKeyAndIndexList("testtable2",nil))
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
assertEqual(cur.getField(0,"table"),"TESTTABLE2")
assertEqual(cur.getField(0,"non_unique"),"0")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"COL1")
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"3")
assertFalse(cur.getField(0,"key_name").nil? || cur.getField(0,"key_name").empty?)
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
assertTrue(cur.getProcedureList(nil))
counter=0
for i in 0..cur.rowCount()-1
	name=cur.getField(i,"routine_name")
	if name=="TESTPROC" ||
		name=="TESTPROC1"
		counter=counter+1
	end
end
assertEqual(counter,2)
print "\n"


# procedure parameter list
print "PROCEDURE PARAMETER LIST: \n"
assertTrue(cur.getProcedureParameterList("testproc",nil))
assertEqual(cur.getColumnName(0),"parameter_name")
assertEqual(cur.getColumnName(1),"parameter_mode")
assertEqual(cur.getColumnName(2),"data_type")
assertEqual(cur.getColumnName(3),"character_maximum_length")
assertEqual(cur.getColumnName(4),"ordinal_position")
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(0,"parameter_name"),"OUT1")
assertEqual(cur.getField(0,"parameter_mode"),"4")
assertEqual(cur.getField(0,"data_type"),"INTEGER")
assertEqual(cur.getField(0,"ordinal_position"),"1")
assertEqual(cur.getField(1,"parameter_name"),"OUT2")
assertEqual(cur.getField(1,"parameter_mode"),"4")
assertEqual(cur.getField(1,"data_type"),"FLOAT")
assertEqual(cur.getField(1,"ordinal_position"),"2")
assertEqual(cur.getField(2,"parameter_name"),"OUT3")
assertEqual(cur.getField(2,"parameter_mode"),"4")
assertEqual(cur.getField(2,"data_type"),"VARCHAR")
assertEqual(cur.getField(2,"ordinal_position"),"3")
assertEqual(cur.getField(3,"parameter_name"),"OUT4")
assertEqual(cur.getField(3,"parameter_mode"),"4")
assertEqual(cur.getField(3,"data_type"),"BLOB SUB_TYPE BINARY")
assertEqual(cur.getField(3,"ordinal_position"),"4")
assertEqual(cur.getField(4,"parameter_name"),"IN1")
assertEqual(cur.getField(4,"parameter_mode"),"1")
assertEqual(cur.getField(4,"data_type"),"INTEGER")
assertEqual(cur.getField(4,"ordinal_position"),"1")
assertEqual(cur.getField(5,"parameter_name"),"IN2")
assertEqual(cur.getField(5,"parameter_mode"),"1")
assertEqual(cur.getField(5,"data_type"),"FLOAT")
assertEqual(cur.getField(5,"ordinal_position"),"2")
assertEqual(cur.getField(6,"parameter_name"),"IN3")
assertEqual(cur.getField(6,"parameter_mode"),"1")
assertEqual(cur.getField(6,"data_type"),"VARCHAR")
assertEqual(cur.getField(6,"ordinal_position"),"3")
assertEqual(cur.getField(7,"parameter_name"),"IN4")
assertEqual(cur.getField(7,"parameter_mode"),"1")
assertEqual(cur.getField(7,"data_type"),"BLOB SUB_TYPE BINARY")
assertEqual(cur.getField(7,"ordinal_position"),"4")
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "))
print "\n"
assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"))
print "\n"
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
print "\n"

reportTestStatus()

exit($status)
