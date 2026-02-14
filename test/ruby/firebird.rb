#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'sqlrelay'
require './asserts'




# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket","testuser","testpassword",0,1)
cur=SQLRCursor.new(con)

# get database type


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"firebird")
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
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

# clear table
cur.sendQuery("delete from testtable")
con.commit()


# insert
print "INSERT: \n"
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
	"	NULL)"))
print "\n"


# bind by position
print "BIND BY POSITION: \n"
cur.prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,NULL)")
assertEqual(cur.countBindVariables(),11)
cur.inputBind("1",2)
cur.inputBind("2",2)
cur.inputBind("3",2.2,2,1)
cur.inputBind("4",2.2,2,1)
cur.inputBind("5",2.2,2,1)
cur.inputBind("6",2.2,2,1)
cur.inputBind("7","01-JAN-2002")
cur.inputBind("8","02:00:00")
cur.inputBind("9","testchar2")
cur.inputBind("10","testvarchar2")
cur.inputBind("11",nil)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",3)
cur.inputBind("2",3)
cur.inputBind("3",3.3,2,1)
cur.inputBind("4",3.3,2,1)
cur.inputBind("5",3.3,2,1)
cur.inputBind("6",3.3,2,1)
cur.inputBind("7","01-JAN-2003")
cur.inputBind("8","03:00:00")
cur.inputBind("9","testchar3")
cur.inputBind("10","testvarchar3")
cur.inputBind("11",nil)
assertTrue(cur.executeQuery())
print "\n"


# array of binds by position
print "ARRAY OF BINDS BY POSITION: \n"
cur.clearBinds()
cur.inputBinds(["1","2","3","4","5","6",
		"7","8","9","10","11"],
	[4,4,4.4,4.4,4.4,4.4,"01-JAN-2004","04:00:00",
		"testchar4","testvarchar4",nil],
	[0,0,2,2,2,2,0,0,0,0,0],
	[0,0,1,1,1,1,0,0,0,0,0])
assertTrue(cur.executeQuery())
print "\n"


# insert
print "INSERT: \n"
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	5, "+
	"	5, "+
	"	5.5, "+
	"	5.5, "+
	"	5.5, "+
	"	5.5, "+
	"	'01-JAN-2005', "+
	"	'05:00:00', "+
	"	'testchar5', "+
	"	'testvarchar5', "+
	"	NULL, "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	6, "+
	"	6, "+
	"	6.6, "+
	"	6.6, "+
	"	6.6, "+
	"	6.6, "+
	"	'01-JAN-2006', "+
	"	'06:00:00', "+
	"	'testchar6', "+
	"	'testvarchar6', "+
	"	NULL, "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	7, "+
	"	7, "+
	"	7.7, "+
	"	7.7, "+
	"	7.7, "+
	"	7.7, "+
	"	'01-JAN-2007', "+
	"	'07:00:00', "+
	"	'testchar7', "+
	"	'testvarchar7', "+
	"	NULL, "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	8, "+
	"	8, "+
	"	8.8, "+
	"	8.8, "+
	"	8.8, "+
	"	8.8, "+
	"	'01-JAN-2008', "+
	"	'08:00:00', "+
	"	'testchar8', "+
	"	'testvarchar8', "+
	"	NULL, "+
	"	NULL)"))
print "\n"


# affected rows
print "AFFECTED ROWS: \n"
assertEqual(cur.affectedRows(),0)
print "\n"


# stored procedure
print "STORED PROCEDURE: \n"
cur.prepareQuery("select * from testproc(?,?,?,NULL)")
cur.inputBind("1",1)
cur.inputBind("2",1.1,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"1.1000")
assertEqual(cur.getField(0,2),"hello")
cur.prepareQuery("execute procedure testproc ?, ?, ?, NULL")
cur.inputBind("1",1)
cur.inputBind("2",1.1,2,1)
cur.inputBind("3","hello")
cur.defineOutputBindInteger("1")
cur.defineOutputBindDouble("2")
cur.defineOutputBindString("3",20)
cur.defineOutputBindBlob("4")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("1"),1)
#assertEqual(cur.getOutputBindDouble("2"),1.1)
assertEqual(cur.getOutputBindString("3"),"hello               ")
print "\n"


# select
print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
print "\n"


# column types
print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"INTEGER")
assertEqual(cur.getColumnType('TESTINTEGER'),"INTEGER")
assertEqual(cur.getColumnType(1),"SMALLINT")
assertEqual(cur.getColumnType('TESTSMALLINT'),"SMALLINT")
assertEqual(cur.getColumnType(2),"DECIMAL")
assertEqual(cur.getColumnType('TESTDECIMAL'),"DECIMAL")
assertEqual(cur.getColumnType(3),"NUMERIC")
assertEqual(cur.getColumnType('TESTNUMERIC'),"NUMERIC")
assertEqual(cur.getColumnType(4),"FLOAT")
assertEqual(cur.getColumnType('TESTFLOAT'),"FLOAT")
assertEqual(cur.getColumnType(5),"DOUBLE PRECISION")
assertEqual(cur.getColumnType('TESTDOUBLE'),"DOUBLE PRECISION")
assertEqual(cur.getColumnType(6),"DATE")
assertEqual(cur.getColumnType('TESTDATE'),"DATE")
assertEqual(cur.getColumnType(7),"TIME")
assertEqual(cur.getColumnType('TESTTIME'),"TIME")
assertEqual(cur.getColumnType(8),"CHAR")
assertEqual(cur.getColumnType('TESTCHAR'),"CHAR")
assertEqual(cur.getColumnType(9),"VARCHAR")
assertEqual(cur.getColumnType('TESTVARCHAR'),"VARCHAR")
assertEqual(cur.getColumnType(10),"TIMESTAMP")
assertEqual(cur.getColumnType('TESTTIMESTAMP'),"TIMESTAMP")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnLength('TESTINTEGER'),4)
assertEqual(cur.getColumnLength(1),2)
assertEqual(cur.getColumnLength('TESTSMALLINT'),2)
assertEqual(cur.getColumnLength(2),8)
assertEqual(cur.getColumnLength('TESTDECIMAL'),8)
assertEqual(cur.getColumnLength(3),8)
assertEqual(cur.getColumnLength('TESTNUMERIC'),8)
assertEqual(cur.getColumnLength(4),4)
assertEqual(cur.getColumnLength('TESTFLOAT'),4)
assertEqual(cur.getColumnLength(5),8)
assertEqual(cur.getColumnLength('TESTDOUBLE'),8)
assertEqual(cur.getColumnLength(6),4)
assertEqual(cur.getColumnLength('TESTDATE'),4)
assertEqual(cur.getColumnLength(7),4)
assertEqual(cur.getColumnLength('TESTTIME'),4)
assertEqual(cur.getColumnLength(8),50)
assertEqual(cur.getColumnLength('TESTCHAR'),50)
assertEqual(cur.getColumnLength(9),50)
assertEqual(cur.getColumnLength('TESTVARCHAR'),50)
assertEqual(cur.getColumnLength(10),8)
assertEqual(cur.getColumnLength('TESTTIMESTAMP'),8)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest('TESTINTEGER'),1)
assertEqual(cur.getLongest(1),1)
assertEqual(cur.getLongest('TESTSMALLINT'),1)
assertEqual(cur.getLongest(2),4)
assertEqual(cur.getLongest('TESTDECIMAL'),4)
assertEqual(cur.getLongest(3),4)
assertEqual(cur.getLongest('TESTNUMERIC'),4)
assertEqual(cur.getLongest(4),6)
assertEqual(cur.getLongest('TESTFLOAT'),6)
assertEqual(cur.getLongest(5),6)
assertEqual(cur.getLongest('TESTDOUBLE'),6)
assertEqual(cur.getLongest(6),10)
assertEqual(cur.getLongest('TESTDATE'),10)
assertEqual(cur.getLongest(7),8)
assertEqual(cur.getLongest('TESTTIME'),8)
assertEqual(cur.getLongest(8),50)
assertEqual(cur.getLongest('TESTCHAR'),50)
assertEqual(cur.getLongest(9),12)
assertEqual(cur.getLongest('TESTVARCHAR'),12)
assertEqual(cur.getLongest(10),0)
assertEqual(cur.getLongest('TESTTIMESTAMP'),0)
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


# fields by hash
print "FIELDS BY HASH: \n"
fields=cur.getRowHash(0)
assertEqual(fields["TESTINTEGER"],"1")
assertEqual(fields["TESTSMALLINT"],"1")
assertEqual(fields["TESTDECIMAL"],"1.10")
assertEqual(fields["TESTNUMERIC"],"1.10")
assertEqual(fields["TESTFLOAT"],"1.1000")
assertEqual(fields["TESTDOUBLE"],"1.1000")
assertEqual(fields["TESTDATE"],"2001:01:01")
assertEqual(fields["TESTTIME"],"01:00:00")
assertEqual(fields["TESTCHAR"],"testchar1                                         ")
assertEqual(fields["TESTVARCHAR"],"testvarchar1")
print "\n"
fields=cur.getRowHash(7)
assertEqual(fields["TESTINTEGER"],"8")
assertEqual(fields["TESTSMALLINT"],"8")
assertEqual(fields["TESTDECIMAL"],"8.80")
assertEqual(fields["TESTNUMERIC"],"8.80")
assertEqual(fields["TESTFLOAT"],"8.8000")
assertEqual(fields["TESTDOUBLE"],"8.8000")
assertEqual(fields["TESTDATE"],"2008:01:01")
assertEqual(fields["TESTTIME"],"08:00:00")
assertEqual(fields["TESTCHAR"],"testchar8                                         ")
assertEqual(fields["TESTVARCHAR"],"testvarchar8")
print "\n"


# field lengths by hash
print "FIELD LENGTHS BY HASH: \n"
fieldlengths=cur.getRowLengthsHash(0)
assertEqual(fieldlengths["TESTINTEGER"],1)
assertEqual(fieldlengths["TESTSMALLINT"],1)
assertEqual(fieldlengths["TESTDECIMAL"],4)
assertEqual(fieldlengths["TESTNUMERIC"],4)
assertEqual(fieldlengths["TESTFLOAT"],6)
assertEqual(fieldlengths["TESTDOUBLE"],6)
assertEqual(fieldlengths["TESTDATE"],10)
assertEqual(fieldlengths["TESTTIME"],8)
assertEqual(fieldlengths["TESTCHAR"],50)
assertEqual(fieldlengths["TESTVARCHAR"],12)
print "\n"
fieldlengths=cur.getRowLengthsHash(7)
assertEqual(fieldlengths["TESTINTEGER"],1)
assertEqual(fieldlengths["TESTSMALLINT"],1)
assertEqual(fieldlengths["TESTDECIMAL"],4)
assertEqual(fieldlengths["TESTNUMERIC"],4)
assertEqual(fieldlengths["TESTFLOAT"],6)
assertEqual(fieldlengths["TESTDOUBLE"],6)
assertEqual(fieldlengths["TESTDATE"],10)
assertEqual(fieldlengths["TESTTIME"],8)
assertEqual(fieldlengths["TESTCHAR"],50)
assertEqual(fieldlengths["TESTVARCHAR"],12)
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)','$(var3)' from rdb$database")
cur.substitution("var1",1)
cur.substitution("var2","hello")
cur.substitution("var3",10.5556,6,4)
assertTrue(cur.executeQuery())
print "\n"


# fields
print "FIELDS: \n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# array substitutions
print "ARRAY SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)','$(var3)' from rdb$database")
cur.substitutions(["var1","var2","var3"],
			[1,"hello",10.5556],[0,0,6],[0,0,4])
assertTrue(cur.executeQuery())
print "\n"


# fields
print "FIELDS: \n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# nulls as nils
print "NULLS as nils: \n"
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
cur.getNullsAsNils()
print "\n"


# result set buffer size
print "RESULT SET BUFFER SIZE: \n"
assertEqual(cur.getResultSetBufferSize(),0)
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
print "\n"


# dont get column info
print "DONT GET COLUMN INFO: \n"
cur.dontGetColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
assertEqual(cur.getColumnName(0),nil)
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),nil)
cur.getColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
assertEqual(cur.getColumnName(0),"TESTINTEGER")
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnType(0),"INTEGER")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
print "\n"


# cached result set with result set buffer size
print "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
assertTrue(con.autoCommitOn())
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	10, "+
	"	10, "+
	"	10.1, "+
	"	10.1, "+
	"	10.1, "+
	"	10.1, "+
	"	'01-JAN-2010', "+
	"	'10:00:00', "+
	"	'testchar10', "+
	"	'testvarchar10', "+
	"	NULL, "+
	"	NULL)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"9")
assertTrue(con.autoCommitOff())
print "\n"


# finished suspended session
print "FINISHED SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testinteger"))
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
con.commit()
cur.sendQuery("delete from testtable")
con.commit()
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"))
assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"))
assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"))
assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"))
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
