#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.

from SQLRelay import PySQLRClient
from decimal import *
import sys
from asserts import *
import string


def main():

	getcontext().prec=1

	PySQLRClient.getNumericFieldsAsNumbers()

	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,
						"/tmp/test.socket",
						"testuser","testpassword")
	cur=PySQLRClient.sqlrcursor(con)


	# identify
	print("IDENTIFY: ")
	assertEqual(con.identify(),"postgresql")
	print()


	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()

	# isolation levels
	#print("ISOLATION LEVELS: ")
	#isolationlevels=["read committed","read uncommitted","repeatable read","serializable"]
	#for il in isolationlevels:
	#	# postgresql requires the isolation level to
	#	# be the first query of the transaction
	#	con.begin()
	#	assertTrue(con.setIsolationLevel(il))
	#	assertEqual(con.getIsolationLevel(),il)
	#	con.commit()
	#	print()
	## reset to the default isolation level
	#con.begin()
	#assertTrue(con.setIsolationLevel(isolationlevels[0]))
	#con.commit()
	#print()

	# drop existing table
	cur.sendQuery("drop table testtable")


	# create temptable
	print("CREATE TEMPTABLE: ")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testreal real, "
		"	testsmallint smallint, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp)"))
	print()


	# begin transction
	print("BEGIN TRANSCTION: ")
	assertTrue(cur.sendQuery("begin"))
	print()


	# insert
	print("INSERT: ")
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01/01/2001', "
		"	'01:00:00', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2.2, "
		"	2.2, "
		"	2, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'01/01/2002', "
		"	'02:00:00', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3.3, "
		"	3.3, "
		"	3, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'01/01/2003', "
		"	'03:00:00', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4.4, "
		"	4.4, "
		"	4, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'01/01/2004', "
		"	'04:00:00', "
		"	NULL)"))
	print()


	# affected rows
	print("AFFECTED ROWS: ")
	assertEqual(cur.affectedRows(),1)
	print()


	# bind by position
	print("BIND BY POSITION: ")
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$1, "
		"	$2, "
		"	$3, "
		"	$4, "
		"	$5, "
		"	$6, "
		"	$7, "
		"	$8)")
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
	print()


	# array of binds by position
	print("ARRAY OF BINDS BY POSITION: ")
	cur.clearBinds()
	cur.inputBinds(["1","2","3","4","5","6","7","8"],
		[7,7.7,7.7,7,"testchar7","testvarchar7",
			"01/01/2007","07:00:00"],
		[0,4,4,0,0,0,0,0],
		[0,2,2,0,0,0,0,0])
	assertTrue(cur.executeQuery())
	print()


	# bind by position with validation
	print("BIND BY POSITION WITH VALIDATION: ")
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
	print()


	# select
	print("SELECT: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	print()


	# column count
	print("COLUMN COUNT: ")
	assertEqual(cur.colCount(),9)
	print()


	# column names
	print("COLUMN NAMES: ")
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
	print()


	# column types
	print("COLUMN TYPES: ")
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
	print()


	# column length
	print("COLUMN LENGTH: ")
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
	print()


	# longest column
	print("LONGEST COLUMN: ")
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
	print()


	# row count
	print("ROW COUNT: ")
	assertEqual(cur.rowCount(),8)
	print()

	#print("TOTAL ROWS: ")
	#assertEqual(cur.totalRows(),8)
	#print()


	# first row index
	print("FIRST ROW INDEX: ")
	assertEqual(cur.firstRowIndex(),0)
	print()


	# end of result set
	print("END OF RESULT SET: ")
	assertTrue(cur.endOfResultSet())
	print()


	# fields by index
	print("FIELDS BY INDEX: ")
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(0,1),Decimal("1.1"))
	assertEqual(cur.getField(0,2),Decimal("1.1"))
	assertEqual(cur.getField(0,3),1)
	assertEqual(cur.getField(0,4),"testchar1                               ")
	assertEqual(cur.getField(0,5),"testvarchar1")
	assertEqual(cur.getField(0,6),"2001-01-01")
	assertEqual(cur.getField(0,7),"01:00:00")
	print()
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(7,1),Decimal("8.8"))
	assertEqual(cur.getField(7,2),Decimal("8.8"))
	assertEqual(cur.getField(7,3),8)
	assertEqual(cur.getField(7,4),"testchar8                               ")
	assertEqual(cur.getField(7,5),"testvarchar8")
	assertEqual(cur.getField(7,6),"2008-01-01")
	assertEqual(cur.getField(7,7),"08:00:00")
	print()


	# field lengths by index
	print("FIELD LENGTHS BY INDEX: ")
	assertEqual(cur.getFieldLength(0,0),1)
	assertEqual(cur.getFieldLength(0,1),3)
	assertEqual(cur.getFieldLength(0,2),3)
	assertEqual(cur.getFieldLength(0,3),1)
	assertEqual(cur.getFieldLength(0,4),40)
	assertEqual(cur.getFieldLength(0,5),12)
	assertEqual(cur.getFieldLength(0,6),10)
	assertEqual(cur.getFieldLength(0,7),8)
	print()
	assertEqual(cur.getFieldLength(7,0),1)
	assertEqual(cur.getFieldLength(7,1),3)
	assertEqual(cur.getFieldLength(7,2),3)
	assertEqual(cur.getFieldLength(7,3),1)
	assertEqual(cur.getFieldLength(7,4),40)
	assertEqual(cur.getFieldLength(7,5),12)
	assertEqual(cur.getFieldLength(7,6),10)
	assertEqual(cur.getFieldLength(7,7),8)
	print()


	# fields by name
	print("FIELDS BY NAME: ")
	assertEqual(cur.getField(0,"testint"),1)
	assertEqual(cur.getField(0,"testfloat"),Decimal("1.1"))
	assertEqual(cur.getField(0,"testreal"),Decimal("1.1"))
	assertEqual(cur.getField(0,"testsmallint"),1)
	assertEqual(cur.getField(0,"testchar"),"testchar1                               ")
	assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
	assertEqual(cur.getField(0,"testdate"),"2001-01-01")
	assertEqual(cur.getField(0,"testtime"),"01:00:00")
	print()
	assertEqual(cur.getField(7,"testint"),8)
	assertEqual(cur.getField(7,"testfloat"),Decimal("8.8"))
	assertEqual(cur.getField(7,"testreal"),Decimal("8.8"))
	assertEqual(cur.getField(7,"testsmallint"),8)
	assertEqual(cur.getField(7,"testchar"),"testchar8                               ")
	assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
	assertEqual(cur.getField(7,"testdate"),"2008-01-01")
	assertEqual(cur.getField(7,"testtime"),"08:00:00")
	print()


	# field lengths by name
	print("FIELD LENGTHS BY NAME: ")
	assertEqual(cur.getFieldLength(0,"testint"),1)
	assertEqual(cur.getFieldLength(0,"testfloat"),3)
	assertEqual(cur.getFieldLength(0,"testreal"),3)
	assertEqual(cur.getFieldLength(0,"testsmallint"),1)
	assertEqual(cur.getFieldLength(0,"testchar"),40)
	assertEqual(cur.getFieldLength(0,"testvarchar"),12)
	assertEqual(cur.getFieldLength(0,"testdate"),10)
	assertEqual(cur.getFieldLength(0,"testtime"),8)
	print()
	assertEqual(cur.getFieldLength(7,"testint"),1)
	assertEqual(cur.getFieldLength(7,"testfloat"),3)
	assertEqual(cur.getFieldLength(7,"testreal"),3)
	assertEqual(cur.getFieldLength(7,"testsmallint"),1)
	assertEqual(cur.getFieldLength(7,"testchar"),40)
	assertEqual(cur.getFieldLength(7,"testvarchar"),12)
	assertEqual(cur.getFieldLength(7,"testdate"),10)
	assertEqual(cur.getFieldLength(7,"testtime"),8)
	print()


	# fields by array
	print("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEqual(fields[0],1)
	assertEqual(fields[1],Decimal("1.1"))
	assertEqual(fields[2],Decimal("1.1"))
	assertEqual(fields[3],1)
	assertEqual(fields[4],"testchar1                               ")
	assertEqual(fields[5],"testvarchar1")
	assertEqual(fields[6],"2001-01-01")
	assertEqual(fields[7],"01:00:00")
	print()


	# field lengths by array
	print("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEqual(fieldlens[0],1)
	assertEqual(fieldlens[1],3)
	assertEqual(fieldlens[2],3)
	assertEqual(fieldlens[3],1)
	assertEqual(fieldlens[4],40)
	assertEqual(fieldlens[5],12)
	assertEqual(fieldlens[6],10)
	assertEqual(fieldlens[7],8)
	print()


	# fields by dictionary
	print("FIELDS BY DICTIONARY: ")
	fields=cur.getRowDictionary(0)
	assertEqual(fields["testint"],1)
	assertEqual(fields["testfloat"],Decimal("1.1"))
	assertEqual(fields["testreal"],Decimal("1.1"))
	assertEqual(fields["testsmallint"],1)
	assertEqual(fields["testchar"],"testchar1                               ")
	assertEqual(fields["testvarchar"],"testvarchar1")
	assertEqual(fields["testdate"],"2001-01-01")
	assertEqual(fields["testtime"],"01:00:00")
	print()
	fields=cur.getRowDictionary(7)
	assertEqual(fields["testint"],8)
	assertEqual(fields["testfloat"],Decimal("8.8"))
	assertEqual(fields["testreal"],Decimal("8.8"))
	assertEqual(fields["testsmallint"],8)
	assertEqual(fields["testchar"],"testchar8                               ")
	assertEqual(fields["testvarchar"],"testvarchar8")
	assertEqual(fields["testdate"],"2008-01-01")
	assertEqual(fields["testtime"],"08:00:00")
	print()


	# field lengths by dictionary
	print("FIELD LENGTHS BY DICTIONARY: ")
	fieldlengths=cur.getRowLengthsDictionary(0)
	assertEqual(fieldlengths["testint"],1)
	assertEqual(fieldlengths["testfloat"],3)
	assertEqual(fieldlengths["testreal"],3)
	assertEqual(fieldlengths["testsmallint"],1)
	assertEqual(fieldlengths["testchar"],40)
	assertEqual(fieldlengths["testvarchar"],12)
	assertEqual(fieldlengths["testdate"],10)
	assertEqual(fieldlengths["testtime"],8)
	print()
	fieldlengths=cur.getRowLengthsDictionary(7)
	assertEqual(fieldlengths["testint"],1)
	assertEqual(fieldlengths["testfloat"],3)
	assertEqual(fieldlengths["testreal"],3)
	assertEqual(fieldlengths["testsmallint"],1)
	assertEqual(fieldlengths["testchar"],40)
	assertEqual(fieldlengths["testvarchar"],12)
	assertEqual(fieldlengths["testdate"],10)
	assertEqual(fieldlengths["testtime"],8)
	print()


	# individual substitutions
	print("INDIVIDUAL SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),'$(var2)',$(var3)")
	cur.substitution("var1",1)
	cur.substitution("var2","hello")
	cur.substitution("var3",10.5556,6,4)
	assertTrue(cur.executeQuery())
	print()


	# fields
	print("FIELDS: ")
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(0,1),"hello")
	assertEqual(cur.getField(0,2),Decimal("10.5556"))
	print()


	# array substitutions
	print("ARRAY SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),'$(var2)',$(var3)")
	cur.substitutions(["var1","var2","var3"],
				[1,"hello",10.5556],[0,0,6],[0,0,4])
	assertTrue(cur.executeQuery())
	print()


	# fields
	print("FIELDS: ")
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(0,1),"hello")
	assertEqual(cur.getField(0,2),Decimal("10.5556"))
	print()


	# nulls as nones
	print("NULLS as Nones: ")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery("select NULL,1,NULL"))
	assertEqual(cur.getField(0,0),None)
	assertEqual(cur.getField(0,1),1)
	assertEqual(cur.getField(0,2),None)
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("select NULL,1,NULL"))
	assertEqual(cur.getField(0,0),"")
	assertEqual(cur.getField(0,1),1)
	assertEqual(cur.getField(0,2),"")
	cur.getNullsAsNone()
	print()


	# result set buffer size
	print("RESULT SET BUFFER SIZE: ")
	assertEqual(cur.getResultSetBufferSize(),0)
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEqual(cur.getResultSetBufferSize(),2)
	print()
	assertEqual(cur.firstRowIndex(),0)
	assertFalse(cur.endOfResultSet())
	assertEqual(cur.rowCount(),2)
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(1,0),2)
	assertEqual(cur.getField(2,0),3)
	print()
	assertEqual(cur.firstRowIndex(),2)
	assertFalse(cur.endOfResultSet())
	assertEqual(cur.rowCount(),4)
	assertEqual(cur.getField(6,0),7)
	assertEqual(cur.getField(7,0),8)
	print()
	assertEqual(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEqual(cur.rowCount(),8)
	assertEqual(cur.getField(8,0),None)
	print()
	assertEqual(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEqual(cur.rowCount(),8)
	print()


	# dont get column info
	print("DONT GET COLUMN INFO: ")
	cur.dontGetColumnInfo()
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEqual(cur.getColumnName(0),None)
	assertEqual(cur.getColumnLength(0),0)
	assertEqual(cur.getColumnType(0),None)
	cur.getColumnInfo()
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEqual(cur.getColumnName(0),"testint")
	assertEqual(cur.getColumnLength(0),4)
	assertEqual(cur.getColumnType(0),"int4")
	print()


	# suspended session
	print("SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	print()
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(1,0),2)
	assertEqual(cur.getField(2,0),3)
	assertEqual(cur.getField(3,0),4)
	assertEqual(cur.getField(4,0),5)
	assertEqual(cur.getField(5,0),6)
	assertEqual(cur.getField(6,0),7)
	assertEqual(cur.getField(7,0),8)
	print()
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	print()
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(1,0),2)
	assertEqual(cur.getField(2,0),3)
	assertEqual(cur.getField(3,0),4)
	assertEqual(cur.getField(4,0),5)
	assertEqual(cur.getField(5,0),6)
	assertEqual(cur.getField(6,0),7)
	assertEqual(cur.getField(7,0),8)
	print()
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	print()
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(1,0),2)
	assertEqual(cur.getField(2,0),3)
	assertEqual(cur.getField(3,0),4)
	assertEqual(cur.getField(4,0),5)
	assertEqual(cur.getField(5,0),6)
	assertEqual(cur.getField(6,0),7)
	assertEqual(cur.getField(7,0),8)
	print()


	# suspended result set
	print("SUSPENDED RESULT SET: ")
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEqual(cur.getField(2,0),3)
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeResultSet(id))
	print()
	assertEqual(cur.firstRowIndex(),4)
	assertFalse(cur.endOfResultSet())
	assertEqual(cur.rowCount(),6)
	assertEqual(cur.getField(7,0),8)
	print()
	assertEqual(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEqual(cur.rowCount(),8)
	assertEqual(cur.getField(8,0),None)
	print()
	assertEqual(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEqual(cur.rowCount(),8)
	cur.setResultSetBufferSize(0)
	print()


	# cached result set
	print("CACHED RESULT SET: ")
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	filename=cur.getCacheFileName()
	assertEqual(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEqual(cur.getField(7,0),8)
	print()


	# column count for cached result set
	print("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEqual(cur.colCount(),9)
	print()


	# column names for cached result set
	print("COLUMN NAMES FOR CACHED RESULT SET: ")
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
	print()


	# cached result set with result set buffer size
	print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	filename=cur.getCacheFileName()
	assertEqual(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(8,0),None)
	cur.setResultSetBufferSize(0)
	print()


	# from one cache file to another
	print("FROM ONE CACHE FILE TO ANOTHER: ")
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(8,0),None)
	print()


	# from one cache file to another with result set buffer size
	print("FROM ONE CACHE FILE TO ANOTHER "
		"WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(8,0),None)
	cur.setResultSetBufferSize(0)
	print()


	# cached result set with suspend and result set buffer size
	print("CACHED RESULT SET WITH SUSPEND "
		"AND RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEqual(cur.getField(2,0),3)
	filename=cur.getCacheFileName()
	assertEqual(filename,"cachefile1")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	print()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeCachedResultSet(id,filename))
	print()
	assertEqual(cur.firstRowIndex(),4)
	assertFalse(cur.endOfResultSet())
	assertEqual(cur.rowCount(),6)
	assertEqual(cur.getField(7,0),8)
	print()
	assertEqual(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEqual(cur.rowCount(),8)
	assertEqual(cur.getField(8,0),None)
	print()
	assertEqual(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEqual(cur.rowCount(),8)
	cur.cacheOff()
	print()
	assertTrue(cur.openCachedResultSet(filename))
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(8,0),None)
	cur.setResultSetBufferSize(0)
	print()


	# commit and rollback
	print("COMMIT AND ROLLBACK: ")
	secondcon=PySQLRClient.sqlrconnection("sqlrelay",9000,
						"/tmp/test.socket",
						"testuser","testpassword")
	secondcur=PySQLRClient.sqlrcursor(secondcon)
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEqual(secondcur.getField(0,0),0)
	assertTrue(con.commit())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEqual(secondcur.getField(0,0),8)
	#assertTrue(con.autoCommitOn())
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01/01/2010', "
		"	'10:00:00', "
		"	NULL)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEqual(secondcur.getField(0,0),9)
	#assertTrue(con.autoCommitOff())
	print()


	# row range
	print("ROW RANGE:")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	print()
	rows=cur.getRowRange(0,5)
	assertEqual(rows[0][0],1)
	assertEqual(rows[0][1],Decimal("1.1"))
	assertEqual(rows[0][2],Decimal("1.1"))
	assertEqual(rows[0][3],1)
	assertEqual(rows[0][4],"testchar1                               ")
	assertEqual(rows[0][5],"testvarchar1")
	assertEqual(rows[0][6],"2001-01-01")
	assertEqual(rows[0][7],"01:00:00")
	print()
	assertEqual(rows[1][0],2)
	assertEqual(rows[1][1],Decimal("2.2"))
	assertEqual(rows[1][2],Decimal("2.2"))
	assertEqual(rows[1][3],2)
	assertEqual(rows[1][4],"testchar2                               ")
	assertEqual(rows[1][5],"testvarchar2")
	assertEqual(rows[1][6],"2002-01-01")
	assertEqual(rows[1][7],"02:00:00")
	print()
	assertEqual(rows[2][0],3)
	assertEqual(rows[2][1],Decimal("3.3"))
	assertEqual(rows[2][2],Decimal("3.3"))
	assertEqual(rows[2][3],3)
	assertEqual(rows[2][4],"testchar3                               ")
	assertEqual(rows[2][5],"testvarchar3")
	assertEqual(rows[2][6],"2003-01-01")
	assertEqual(rows[2][7],"03:00:00")
	print()
	assertEqual(rows[3][0],4)
	assertEqual(rows[3][1],Decimal("4.4"))
	assertEqual(rows[3][2],Decimal("4.4"))
	assertEqual(rows[3][3],4)
	assertEqual(rows[3][4],"testchar4                               ")
	assertEqual(rows[3][5],"testvarchar4")
	assertEqual(rows[3][6],"2004-01-01")
	assertEqual(rows[3][7],"04:00:00")
	print()
	assertEqual(rows[4][0],5)
	assertEqual(rows[4][1],Decimal("5.5"))
	assertEqual(rows[4][2],Decimal("5.5"))
	assertEqual(rows[4][3],5)
	assertEqual(rows[4][4],"testchar5                               ")
	assertEqual(rows[4][5],"testvarchar5")
	assertEqual(rows[4][6],"2005-01-01")
	assertEqual(rows[4][7],"05:00:00")
	print()
	assertEqual(rows[5][0],6)
	assertEqual(rows[5][1],Decimal("6.6"))
	assertEqual(rows[5][2],Decimal("6.6"))
	assertEqual(rows[5][3],6)
	assertEqual(rows[5][4],"testchar6                               ")
	assertEqual(rows[5][5],"testvarchar6")
	assertEqual(rows[5][6],"2006-01-01")
	assertEqual(rows[5][7],"06:00:00")
	print()


	# finished suspended session
	print("FINISHED SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEqual(cur.getField(4,0),5)
	assertEqual(cur.getField(5,0),6)
	assertEqual(cur.getField(6,0),7)
	assertEqual(cur.getField(7,0),8)
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeResultSet(id))
	assertEqual(cur.getField(4,0),None)
	assertEqual(cur.getField(5,0),None)
	assertEqual(cur.getField(6,0),None)
	assertEqual(cur.getField(7,0),None)
	print()

	# drop existing table
	cur.sendQuery("drop table testtable")


	# stored procedures
	print("STORED PROCEDURES: ")
	cur.sendQuery("drop function testfunc(int)")
	assertTrue(cur.sendQuery(
		"create function testfunc(int) returns int as "
		"	' begin return $1; end;' language plpgsql"))
	cur.prepareQuery("select * from testfunc($1)")
	cur.inputBind("1",5)
	assertTrue(cur.executeQuery())
	assertEqual(cur.getField(0,0),5)
	cur.sendQuery("drop function testfunc(int)")

	cur.sendQuery("drop function testfunc(int,char(20))")
	assertTrue(cur.sendQuery(
		"create function testfunc("
		"	int, char(20)) "
		"returns record as ' "
		"	declare output record; "
		"begin "
		"	select $1,$2 into output; "
		"	return output; "
		"end;' language plpgsql"))
	cur.prepareQuery(
		"select "
		"	* "
		"from "
		"	testfunc($1,$2) as (col1 int, col2 bpchar) ")
	cur.inputBind("1",5)
	cur.inputBind("2","hello")
	assertTrue(cur.executeQuery())
	assertEqual(cur.getField(0,0),5)
	assertEqual(cur.getField(0,1),"hello")
	cur.sendQuery("drop function testfunc(int,char(20))")
	print()


	# invalid queries
	print("INVALID QUERIES: ")
	assertFalse(cur.sendQuery("select * from testtable order by testint"))
	assertFalse(cur.sendQuery("select * from testtable order by testint"))
	assertFalse(cur.sendQuery("select * from testtable order by testint"))
	assertFalse(cur.sendQuery("select * from testtable order by testint"))
	print()
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	print()
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	print()

if __name__ == "__main__":
	main()
	reportTestStatus()
	sys.exit(status)
