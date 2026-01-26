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

print "IDENTIFY: \n"
assertEqual(con.identify(),"postgresql")
print "\n"

# ping
print "PING: \n"
assertTrue(con.ping())
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

# drop existing table
cur.sendQuery("drop table testtable")

print "CREATE TEMPTABLE: \n"
assertTrue(cur.sendQuery("create table testtable (testint int, testfloat float, testreal real, testsmallint smallint, testchar char(40), testvarchar varchar(40), testdate date, testtime time, testtimestamp timestamp)"))
print "\n"

print "BEGIN TRANSCTION: \n"
assertTrue(cur.sendQuery("begin"))
print "\n"

print "INSERT: \n"
assertTrue(cur.sendQuery("insert into testtable values (1,1.1,1.1,1,'testchar1','testvarchar1','01/01/2001','01:00:00',NULL)"))
assertTrue(cur.sendQuery("insert into testtable values (2,2.2,2.2,2,'testchar2','testvarchar2','01/01/2002','02:00:00',NULL)"))
assertTrue(cur.sendQuery("insert into testtable values (3,3.3,3.3,3,'testchar3','testvarchar3','01/01/2003','03:00:00',NULL)"))
assertTrue(cur.sendQuery("insert into testtable values (4,4.4,4.4,4,'testchar4','testvarchar4','01/01/2004','04:00:00',NULL)"))
print "\n"

print "AFFECTED ROWS: \n"
assertEqual(cur.affectedRows(),1)
print "\n"

print "BIND BY NAME: \n"
cur.prepareQuery("insert into testtable values ($1,$2,$3,$4,$5,$6,$7,$8)")
assertEqual(cur.countBindVariables(),8)
cur.inputBind("1",5)
cur.inputBind("2",5.5,4,2)
cur.inputBind("3",5.5,4,2)
cur.inputBind("4",5)
cur.inputBind("5","testchar5")
cur.inputBind("6","testvarchar5")
cur.inputBind("7","01/01/2005")
cur.inputBind("8","05:00:00")
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
assertTrue(cur.executeQuery())
print "\n"

print "ARRAY OF BINDS BY NAME: \n"
cur.clearBinds()
cur.inputBinds(["1","2","3","4","5","6","7","8"],
	[7,7.7,7.7,7,"testchar7","testvarchar7",
		"01/01/2007","07:00:00"],
	[0,4,4,0,0,0,0,0],
	[0,2,2,0,0,0,0,0])
assertTrue(cur.executeQuery())
print "\n"

print "BIND BY NAME WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("1",8)
cur.inputBind("2",8.8,4,2)
cur.inputBind("3",8.8,4,2)
cur.inputBind("4",8)
cur.inputBind("5","testchar8")
cur.inputBind("6","testvarchar8")
cur.inputBind("7","01/01/2008")
cur.inputBind("8","08:00:00")
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"

print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
print "\n"

print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),9)
print "\n"

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

print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"int4")
assertEqual(cur.getColumnType('testint'),"int4")
assertEqual(cur.getColumnType(1),"float8")
assertEqual(cur.getColumnType('testfloat'),"float8")
assertEqual(cur.getColumnType(2),"float4")
assertEqual(cur.getColumnType('testreal'),"float4")
assertEqual(cur.getColumnType(3),"int2")
assertEqual(cur.getColumnType('testsmallint'),"int2")
assertEqual(cur.getColumnType(4),"bpchar")
assertEqual(cur.getColumnType('testchar'),"bpchar")
assertEqual(cur.getColumnType(5),"varchar")
assertEqual(cur.getColumnType('testvarchar'),"varchar")
assertEqual(cur.getColumnType(6),"date")
assertEqual(cur.getColumnType('testdate'),"date")
assertEqual(cur.getColumnType(7),"time")
assertEqual(cur.getColumnType('testtime'),"time")
assertEqual(cur.getColumnType(8),"timestamp")
assertEqual(cur.getColumnType('testtimestamp'),"timestamp")
print "\n"

print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnLength('testint'),4)
assertEqual(cur.getColumnLength(1),8)
assertEqual(cur.getColumnLength('testfloat'),8)
assertEqual(cur.getColumnLength(2),4)
assertEqual(cur.getColumnLength('testreal'),4)
assertEqual(cur.getColumnLength(3),2)
assertEqual(cur.getColumnLength('testsmallint'),2)
assertEqual(cur.getColumnLength(4),44)
assertEqual(cur.getColumnLength('testchar'),44)
assertEqual(cur.getColumnLength(5),44)
assertEqual(cur.getColumnLength('testvarchar'),44)
assertEqual(cur.getColumnLength(6),4)
assertEqual(cur.getColumnLength('testdate'),4)
assertEqual(cur.getColumnLength(7),8)
assertEqual(cur.getColumnLength('testtime'),8)
assertEqual(cur.getColumnLength(8),8)
assertEqual(cur.getColumnLength('testtimestamp'),8)
print "\n"

print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest('testint'),1)
assertEqual(cur.getLongest(1),3)
assertEqual(cur.getLongest('testfloat'),3)
assertEqual(cur.getLongest(2),3)
assertEqual(cur.getLongest('testreal'),3)
assertEqual(cur.getLongest(3),1)
assertEqual(cur.getLongest('testsmallint'),1)
assertEqual(cur.getLongest(4),40)
assertEqual(cur.getLongest('testchar'),40)
assertEqual(cur.getLongest(5),12)
assertEqual(cur.getLongest('testvarchar'),12)
assertEqual(cur.getLongest(6),10)
assertEqual(cur.getLongest('testdate'),10)
assertEqual(cur.getLongest(7),8)
assertEqual(cur.getLongest('testtime'),8)
print "\n"

print "ROW COUNT: \n"
assertEqual(cur.rowCount(),8)
print "\n"

#print "TOTAL ROWS: \n"
#assertEqual(cur.totalRows(),8)
#print "\n"

print "FIRST ROW INDEX: \n"
assertEqual(cur.firstRowIndex(),0)
print "\n"

print "END OF RESULT SET: \n"
assertTrue(cur.endOfResultSet())
print "\n"

print "FIELDS BY INDEX: \n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"1.1")
assertEqual(cur.getField(0,2),"1.1")
assertEqual(cur.getField(0,3),"1")
assertEqual(cur.getField(0,4),"testchar1                               ")
assertEqual(cur.getField(0,5),"testvarchar1")
assertEqual(cur.getField(0,6),"2001-01-01")
assertEqual(cur.getField(0,7),"01:00:00")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8.8")
assertEqual(cur.getField(7,2),"8.8")
assertEqual(cur.getField(7,3),"8")
assertEqual(cur.getField(7,4),"testchar8                               ")
assertEqual(cur.getField(7,5),"testvarchar8")
assertEqual(cur.getField(7,6),"2008-01-01")
assertEqual(cur.getField(7,7),"08:00:00")
print "\n"

print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),3)
assertEqual(cur.getFieldLength(0,2),3)
assertEqual(cur.getFieldLength(0,3),1)
assertEqual(cur.getFieldLength(0,4),40)
assertEqual(cur.getFieldLength(0,5),12)
assertEqual(cur.getFieldLength(0,6),10)
assertEqual(cur.getFieldLength(0,7),8)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),3)
assertEqual(cur.getFieldLength(7,2),3)
assertEqual(cur.getFieldLength(7,3),1)
assertEqual(cur.getFieldLength(7,4),40)
assertEqual(cur.getFieldLength(7,5),12)
assertEqual(cur.getFieldLength(7,6),10)
assertEqual(cur.getFieldLength(7,7),8)
print "\n"

print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"testint"),"1")
assertEqual(cur.getField(0,"testfloat"),"1.1")
assertEqual(cur.getField(0,"testreal"),"1.1")
assertEqual(cur.getField(0,"testsmallint"),"1")
assertEqual(cur.getField(0,"testchar"),"testchar1                               ")
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
assertEqual(cur.getField(0,"testdate"),"2001-01-01")
assertEqual(cur.getField(0,"testtime"),"01:00:00")
print "\n"
assertEqual(cur.getField(7,"testint"),"8")
assertEqual(cur.getField(7,"testfloat"),"8.8")
assertEqual(cur.getField(7,"testreal"),"8.8")
assertEqual(cur.getField(7,"testsmallint"),"8")
assertEqual(cur.getField(7,"testchar"),"testchar8                               ")
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
assertEqual(cur.getField(7,"testdate"),"2008-01-01")
assertEqual(cur.getField(7,"testtime"),"08:00:00")
print "\n"

print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"testint"),1)
assertEqual(cur.getFieldLength(0,"testfloat"),3)
assertEqual(cur.getFieldLength(0,"testreal"),3)
assertEqual(cur.getFieldLength(0,"testsmallint"),1)
assertEqual(cur.getFieldLength(0,"testchar"),40)
assertEqual(cur.getFieldLength(0,"testvarchar"),12)
assertEqual(cur.getFieldLength(0,"testdate"),10)
assertEqual(cur.getFieldLength(0,"testtime"),8)
print "\n"
assertEqual(cur.getFieldLength(7,"testint"),1)
assertEqual(cur.getFieldLength(7,"testfloat"),3)
assertEqual(cur.getFieldLength(7,"testreal"),3)
assertEqual(cur.getFieldLength(7,"testsmallint"),1)
assertEqual(cur.getFieldLength(7,"testchar"),40)
assertEqual(cur.getFieldLength(7,"testvarchar"),12)
assertEqual(cur.getFieldLength(7,"testdate"),10)
assertEqual(cur.getFieldLength(7,"testtime"),8)
print "\n"

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
print "\n"

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
print "\n"

print "FIELDS BY HASH: \n"
fields=cur.getRowHash(0)
assertEqual(fields["testint"],"1")
assertEqual(fields["testfloat"],"1.1")
assertEqual(fields["testreal"],"1.1")
assertEqual(fields["testsmallint"],"1")
assertEqual(fields["testchar"],"testchar1                               ")
assertEqual(fields["testvarchar"],"testvarchar1")
assertEqual(fields["testdate"],"2001-01-01")
assertEqual(fields["testtime"],"01:00:00")
print "\n"
fields=cur.getRowHash(7)
assertEqual(fields["testint"],"8")
assertEqual(fields["testfloat"],"8.8")
assertEqual(fields["testreal"],"8.8")
assertEqual(fields["testsmallint"],"8")
assertEqual(fields["testchar"],"testchar8                               ")
assertEqual(fields["testvarchar"],"testvarchar8")
assertEqual(fields["testdate"],"2008-01-01")
assertEqual(fields["testtime"],"08:00:00")
print "\n"

print "FIELD LENGTHS BY HASH: \n"
fieldlengths=cur.getRowLengthsHash(0)
assertEqual(fieldlengths["testint"],1)
assertEqual(fieldlengths["testfloat"],3)
assertEqual(fieldlengths["testreal"],3)
assertEqual(fieldlengths["testsmallint"],1)
assertEqual(fieldlengths["testchar"],40)
assertEqual(fieldlengths["testvarchar"],12)
assertEqual(fieldlengths["testdate"],10)
assertEqual(fieldlengths["testtime"],8)
print "\n"
fieldlengths=cur.getRowLengthsHash(7)
assertEqual(fieldlengths["testint"],1)
assertEqual(fieldlengths["testfloat"],3)
assertEqual(fieldlengths["testreal"],3)
assertEqual(fieldlengths["testsmallint"],1)
assertEqual(fieldlengths["testchar"],40)
assertEqual(fieldlengths["testvarchar"],12)
assertEqual(fieldlengths["testdate"],10)
assertEqual(fieldlengths["testtime"],8)
print "\n"

print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)',$(var3)")
cur.substitution("var1",1)
cur.substitution("var2","hello")
cur.substitution("var3",10.5556,6,4)
assertTrue(cur.executeQuery())
print "\n"

print "FIELDS: \n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"

print "ARRAY SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)',$(var3)")
cur.substitutions(["var1","var2","var3"],
			[1,"hello",10.5556],[0,0,6],[0,0,4])
assertTrue(cur.executeQuery())
print "\n"

print "FIELDS: \n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"

print "NULLS as nils: \n"
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
cur.getNullsAsNils()
print "\n"

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
print "\n"

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

print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),9)
print "\n"

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

print "FROM ONE CACHE FILE TO ANOTHER: \n"
cur.cacheToFile("cachefile2")
assertTrue(cur.openCachedResultSet("cachefile1"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
print "\n"

print "FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile2")
assertTrue(cur.openCachedResultSet("cachefile1"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"

print "CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: \n"
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

print "COMMIT AND ROLLBACK: \n"
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
secondcur=SQLRCursor.new(secondcon)
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
assertTrue(con.commit())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"8")
#assertTrue(con.autoCommitOn())
assertTrue(cur.sendQuery("insert into testtable values (10,10.1,10.1,10,'testchar10','testvarchar10','01/01/2010','10:00:00',NULL)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"9")
#assertTrue(con.autoCommitOff())
print "\n"

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

# stored procedures
print "STORED PROCEDURES: \n"
cur.sendQuery("drop function testfunc(int)")
assertTrue(cur.sendQuery("create function testfunc(int) returns int as ' begin return $1; end;' language plpgsql"))
cur.prepareQuery("select * from testfunc($1)")
cur.inputBind("1",5)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"5")
cur.sendQuery("drop function testfunc(int)")

cur.sendQuery("drop function testfunc(int,char(20))")
assertTrue(cur.sendQuery("create function testfunc(int, char(20)) returns record as ' declare output record; begin select $1,$2 into output; return output; end;' language plpgsql"))
cur.prepareQuery("select * from testfunc($1,$2) as (col1 int, col2 bpchar)")
cur.inputBind("1",5)
cur.inputBind("2","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"5")
assertEqual(cur.getField(0,1),"hello")
cur.sendQuery("drop function testfunc(int,char(20))")
print "\n"


# drop existing table
cur.sendQuery("drop table testtable")

# invalid queries...
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
