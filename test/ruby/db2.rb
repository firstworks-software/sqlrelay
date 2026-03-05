#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'sqlrelay'
require './asserts'




# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket","db2inst1","testpassword",0,1)
cur=SQLRCursor.new(con)

# get database type


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"db2")
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
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

# drop existing table
cur.sendQuery("drop table testtable")


# create temptable
print "CREATE TEMPTABLE: \n"
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
	"	testtimestamp timestamp)"))
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
	"	1.1, "+
	"	1.1, "+
	"	1.1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01/01/2001', "+
	"	'01:00:00', "+
	"	NULL)"))
print "\n"


# bind by position
print "BIND BY POSITION: \n"
cur.prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,NULL)")
assertEqual(cur.countBindVariables(),10)
cur.inputBind("1",2)
cur.inputBind("2",2)
cur.inputBind("3",2)
cur.inputBind("4",2.2,4,2)
cur.inputBind("5",2.2,4,2)
cur.inputBind("6",2.2,4,2)
cur.inputBind("7","testchar2")
cur.inputBind("8","testvarchar2")
cur.inputBind("9","01/01/2002")
cur.inputBind("10","02:00:00")
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",3)
cur.inputBind("2",3)
cur.inputBind("3",3)
cur.inputBind("4",3.3,4,2)
cur.inputBind("5",3.3,4,2)
cur.inputBind("6",3.3,4,2)
cur.inputBind("7","testchar3")
cur.inputBind("8","testvarchar3")
cur.inputBind("9","01/01/2003")
cur.inputBind("10","03:00:00")
assertTrue(cur.executeQuery())
print "\n"


# array of binds by position
print "ARRAY OF BINDS BY POSITION: \n"
cur.clearBinds()
cur.inputBinds(["1","2","3","4","5","6","7","8","9","10"],
	[4,4,4,4.4,4.4,4.4,"testchar4","testvarchar4",
		"01/01/2004","04:00:00"],
	[0,0,0,4,4,4,0,0,0,0],
	[0,0,0,2,2,2,0,0,0,0])
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
	"	5, "+
	"	5.5, "+
	"	5.5, "+
	"	5.5, "+
	"	'testchar5', "+
	"	'testvarchar5', "+
	"	'01/01/2005', "+
	"	'05:00:00', "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	6, "+
	"	6, "+
	"	6, "+
	"	6.6, "+
	"	6.6, "+
	"	6.6, "+
	"	'testchar6', "+
	"	'testvarchar6', "+
	"	'01/01/2006', "+
	"	'06:00:00', "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	7, "+
	"	7, "+
	"	7, "+
	"	7.7, "+
	"	7.7, "+
	"	7.7, "+
	"	'testchar7', "+
	"	'testvarchar7', "+
	"	'01/01/2007', "+
	"	'07:00:00', "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	8, "+
	"	8, "+
	"	8, "+
	"	8.8, "+
	"	8.8, "+
	"	8.8, "+
	"	'testchar8', "+
	"	'testvarchar8', "+
	"	'01/01/2008', "+
	"	'08:00:00', "+
	"	NULL)"))
print "\n"


# affected rows
print "AFFECTED ROWS: \n"
assertEqual(cur.affectedRows(),1)
print "\n"


# stored procedure
print "STORED PROCEDURE: \n"
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in invar int, "+
	"	out outvar int) "+
	"language sql "+
	"begin "+
	"	set outvar = invar; "+
	"end"))
cur.prepareQuery("call testproc(?,?)")
cur.inputBind("1",5)
cur.defineOutputBindString("2",10)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindString("2"),"5")
assertTrue(cur.sendQuery("drop procedure testproc"))
print "\n"


# select
print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),11)
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
assertEqual(cur.getColumnType('TESTSMALLINT'),"SMALLINT")
assertEqual(cur.getColumnType(1),"INTEGER")
assertEqual(cur.getColumnType('TESTINT'),"INTEGER")
assertEqual(cur.getColumnType(2),"BIGINT")
assertEqual(cur.getColumnType('TESTBIGINT'),"BIGINT")
assertEqual(cur.getColumnType(3),"DECIMAL")
assertEqual(cur.getColumnType('TESTDECIMAL'),"DECIMAL")
assertEqual(cur.getColumnType(4),"REAL")
assertEqual(cur.getColumnType('TESTREAL'),"REAL")
assertEqual(cur.getColumnType(5),"DOUBLE")
assertEqual(cur.getColumnType('TESTDOUBLE'),"DOUBLE")
assertEqual(cur.getColumnType(6),"CHAR")
assertEqual(cur.getColumnType('TESTCHAR'),"CHAR")
assertEqual(cur.getColumnType(7),"VARCHAR")
assertEqual(cur.getColumnType('TESTVARCHAR'),"VARCHAR")
assertEqual(cur.getColumnType(8),"DATE")
assertEqual(cur.getColumnType('TESTDATE'),"DATE")
assertEqual(cur.getColumnType(9),"TIME")
assertEqual(cur.getColumnType('TESTTIME'),"TIME")
assertEqual(cur.getColumnType(10),"TIMESTAMP")
assertEqual(cur.getColumnType('TESTTIMESTAMP'),"TIMESTAMP")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),2)
assertEqual(cur.getColumnLength('TESTSMALLINT'),2)
assertEqual(cur.getColumnLength(1),4)
assertEqual(cur.getColumnLength('TESTINT'),4)
assertEqual(cur.getColumnLength(2),8)
assertEqual(cur.getColumnLength('TESTBIGINT'),8)
assertEqual(cur.getColumnLength(3),12)
assertEqual(cur.getColumnLength('TESTDECIMAL'),12)
assertEqual(cur.getColumnLength(4),4)
assertEqual(cur.getColumnLength('TESTREAL'),4)
assertEqual(cur.getColumnLength(5),8)
assertEqual(cur.getColumnLength('TESTDOUBLE'),8)
assertEqual(cur.getColumnLength(6),40)
assertEqual(cur.getColumnLength('TESTCHAR'),40)
assertEqual(cur.getColumnLength(7),40)
assertEqual(cur.getColumnLength('TESTVARCHAR'),40)
assertEqual(cur.getColumnLength(8),6)
assertEqual(cur.getColumnLength('TESTDATE'),6)
assertEqual(cur.getColumnLength(9),6)
assertEqual(cur.getColumnLength('TESTTIME'),6)
assertEqual(cur.getColumnLength(10),16)
assertEqual(cur.getColumnLength('TESTTIMESTAMP'),16)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest('TESTSMALLINT'),1)
assertEqual(cur.getLongest(1),1)
assertEqual(cur.getLongest('TESTINT'),1)
assertEqual(cur.getLongest(2),1)
assertEqual(cur.getLongest('TESTBIGINT'),1)
assertEqual(cur.getLongest(3),4)
assertEqual(cur.getLongest('TESTDECIMAL'),4)
#assertEqual(cur.getLongest(4),3)
#assertEqual(cur.getLongest('TESTREAL'),3)
#assertEqual(cur.getLongest(5),3)
#assertEqual(cur.getLongest('TESTDOUBLE'),3)
assertEqual(cur.getLongest(6),40)
assertEqual(cur.getLongest('TESTCHAR'),40)
assertEqual(cur.getLongest(7),12)
assertEqual(cur.getLongest('TESTVARCHAR'),12)
assertEqual(cur.getLongest(8),10)
assertEqual(cur.getLongest('TESTDATE'),10)
assertEqual(cur.getLongest(9),8)
assertEqual(cur.getLongest('TESTTIME'),8)
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
assertEqual(cur.getField(0,3),"1.10")
#assertEqual(cur.getField(0,4),"1.1")
#assertEqual(cur.getField(0,5),"1.1")
assertEqual(cur.getField(0,6),"testchar1                               ")
assertEqual(cur.getField(0,7),"testvarchar1")
assertEqual(cur.getField(0,8),"2001-01-01")
assertEqual(cur.getField(0,9),"01:00:00")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(7,2),"8")
assertEqual(cur.getField(7,3),"8.80")
#assertEqual(cur.getField(7,4),"8.8")
#assertEqual(cur.getField(7,5),"8.8")
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
#assertEqual(cur.getFieldLength(0,4),3)
#assertEqual(cur.getFieldLength(0,5),3)
assertEqual(cur.getFieldLength(0,6),40)
assertEqual(cur.getFieldLength(0,7),12)
assertEqual(cur.getFieldLength(0,8),10)
assertEqual(cur.getFieldLength(0,9),8)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),1)
assertEqual(cur.getFieldLength(7,2),1)
assertEqual(cur.getFieldLength(7,3),4)
#assertEqual(cur.getFieldLength(7,4),3)
#assertEqual(cur.getFieldLength(7,5),3)
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
assertEqual(cur.getField(0,"TESTDECIMAL"),"1.10")
#assertEqual(cur.getField(0,"TESTREAL"),"1.1")
#assertEqual(cur.getField(0,"TESTDOUBLE"),"1.1")
assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                               ")
assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1")
assertEqual(cur.getField(0,"TESTDATE"),"2001-01-01")
assertEqual(cur.getField(0,"TESTTIME"),"01:00:00")
print "\n"
assertEqual(cur.getField(7,"TESTSMALLINT"),"8")
assertEqual(cur.getField(7,"TESTINT"),"8")
assertEqual(cur.getField(7,"TESTBIGINT"),"8")
assertEqual(cur.getField(7,"TESTDECIMAL"),"8.80")
#assertEqual(cur.getField(7,"TESTREAL"),"8.8")
#assertEqual(cur.getField(7,"TESTDOUBLE"),"8.8")
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
#assertEqual(cur.getFieldLength(0,"TESTREAL"),3)
#assertEqual(cur.getFieldLength(0,"TESTDOUBLE"),3)
assertEqual(cur.getFieldLength(0,"TESTCHAR"),40)
assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12)
assertEqual(cur.getFieldLength(0,"TESTDATE"),10)
assertEqual(cur.getFieldLength(0,"TESTTIME"),8)
print "\n"
assertEqual(cur.getFieldLength(7,"TESTSMALLINT"),1)
assertEqual(cur.getFieldLength(7,"TESTINT"),1)
assertEqual(cur.getFieldLength(7,"TESTBIGINT"),1)
assertEqual(cur.getFieldLength(7,"TESTDECIMAL"),4)
#assertEqual(cur.getFieldLength(7,"TESTREAL"),3)
#assertEqual(cur.getFieldLength(7,"TESTDOUBLE"),3)
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
assertEqual(fields[3],"1.10")
#assertEqual(fields[4],"1.1")
#assertEqual(fields[5],"1.1")
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
#assertEqual(fieldlens[4],3)
#assertEqual(fieldlens[5],3)
assertEqual(fieldlens[6],40)
assertEqual(fieldlens[7],12)
assertEqual(fieldlens[8],10)
assertEqual(fieldlens[9],8)
print "\n"


# fields by hash
print "FIELDS BY HASH: \n"
fields=cur.getRowHash(0)
assertEqual(fields["TESTSMALLINT"],"1")
assertEqual(fields["TESTINT"],"1")
assertEqual(fields["TESTBIGINT"],"1")
assertEqual(fields["TESTDECIMAL"],"1.10")
#assertEqual(fields["TESTREAL"],"1.1")
#assertEqual(fields["TESTDOUBLE"],"1.1")
assertEqual(fields["TESTCHAR"],"testchar1                               ")
assertEqual(fields["TESTVARCHAR"],"testvarchar1")
assertEqual(fields["TESTDATE"],"2001-01-01")
assertEqual(fields["TESTTIME"],"01:00:00")
print "\n"
fields=cur.getRowHash(7)
assertEqual(fields["TESTSMALLINT"],"8")
assertEqual(fields["TESTINT"],"8")
assertEqual(fields["TESTBIGINT"],"8")
assertEqual(fields["TESTDECIMAL"],"8.80")
#assertEqual(fields["TESTREAL"],"8.8")
#assertEqual(fields["TESTDOUBLE"],"8.8")
assertEqual(fields["TESTCHAR"],"testchar8                               ")
assertEqual(fields["TESTVARCHAR"],"testvarchar8")
assertEqual(fields["TESTDATE"],"2008-01-01")
assertEqual(fields["TESTTIME"],"08:00:00")
print "\n"


# field lengths by hash
print "FIELD LENGTHS BY HASH: \n"
fieldlengths=cur.getRowLengthsHash(0)
assertEqual(fieldlengths["TESTSMALLINT"],1)
assertEqual(fieldlengths["TESTINT"],1)
assertEqual(fieldlengths["TESTBIGINT"],1)
assertEqual(fieldlengths["TESTDECIMAL"],4)
#assertEqual(fieldlengths["TESTREAL"],3)
#assertEqual(fieldlengths["TESTDOUBLE"],1)
assertEqual(fieldlengths["TESTCHAR"],40)
assertEqual(fieldlengths["TESTVARCHAR"],12)
assertEqual(fieldlengths["TESTDATE"],10)
assertEqual(fieldlengths["TESTTIME"],8)
print "\n"
fieldlengths=cur.getRowLengthsHash(7)
assertEqual(fieldlengths["TESTSMALLINT"],1)
assertEqual(fieldlengths["TESTINT"],1)
assertEqual(fieldlengths["TESTBIGINT"],1)
assertEqual(fieldlengths["TESTDECIMAL"],4)
#assertEqual(fieldlengths["TESTREAL"],3)
#assertEqual(fieldlengths["TESTDOUBLE"],1)
assertEqual(fieldlengths["TESTCHAR"],40)
assertEqual(fieldlengths["TESTVARCHAR"],12)
assertEqual(fieldlengths["TESTDATE"],10)
assertEqual(fieldlengths["TESTTIME"],8)
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')")
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
cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')")
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
cur.sendQuery("drop table testtable1")
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	col1 char(1), "+
	"	col2 char(1), "+
	"	col3 char(1))"))
assertTrue(cur.sendQuery("insert into testtable1 values ('1',NULL,NULL)"))
assertTrue(cur.sendQuery("select * from testtable1"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select * from testtable1"))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"")
assertEqual(cur.getField(0,2),"")
assertTrue(cur.sendQuery("drop table testtable1"))
cur.getNullsAsNils()
print "\n"


# result set buffer size
print "RESULT SET BUFFER SIZE: \n"
assertEqual(cur.getResultSetBufferSize(),0)
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
assertEqual(cur.getColumnName(0),nil)
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),nil)
cur.getColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
assertEqual(cur.getColumnName(0),"TESTSMALLINT")
assertEqual(cur.getColumnLength(0),2)
assertEqual(cur.getColumnType(0),"SMALLINT")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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
assertTrue(cur.sendQuery("select * from testtable order by testsmallint"))
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

# drop existing table
con.commit()
cur.sendQuery("drop table testtable")
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery("select * from testtable order by testsmallint"))
assertFalse(cur.sendQuery("select * from testtable order by testsmallint"))
assertFalse(cur.sendQuery("select * from testtable order by testsmallint"))
assertFalse(cur.sendQuery("select * from testtable order by testsmallint"))
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
