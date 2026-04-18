#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.


from SQLRelay import PySQLRClient
import sys
import asserts
from asserts import *


def main():

	isolationlevels=["0","1"]
	subvars=["var1","var2","var3"]
	subvalstrings=["hi","hello","bye"]
	subvallongs=[1,2,3]
	subvaldoubles=[10.55,10.556,10.5556]
	precs=[4,5,6]
	scales=[2,3,4]

	LARGE_BUFFER_LENGTH=8192


	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
	cur=PySQLRClient.sqlrcursor(con)
	asserts.setConnection(con)
	asserts.setCursor(cur)


	# identify
	print("IDENTIFY: ")
	assertEquals(con.identify(),"sqlite")
	print()


	# db version
	print("DB VERSION: ")
	dbversion=con.dbVersion()
	issqlite3=True
	if (dbversion is None or
		dbversion=="unknown" or
		len(dbversion)==0 or
		not dbversion[0].isdigit() or
		int(dbversion[0])<3):
		issqlite3=False
	print()


	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()


	# bind format
	print("BIND FORMAT: ")
	assertEquals(con.bindFormat(),":*")
	print()


	# nextval format
	print("NEXTVAL FORMAT: ")
	assertEquals(con.nextvalFormat(),"")
	print()


	# isolation levels
	print("ISOLATION LEVELS: ")
	for il in isolationlevels:
		assertTrue(con.setIsolationLevel(il))
		assertEquals(con.getIsolationLevel(),il)
		print()
	# reset to the default isolation level
	assertTrue(con.setIsolationLevel(isolationlevels[0]))
	print()


	# create testtable
	print("CREATE TESTTABLE: ")
	con.begin()
	cur.sendQuery("drop table if exists testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testclob clob, "
		"	testblob blob)"))
	con.commit()
	print()


	# insert
	print("INSERT: ")
	assertTrue(con.begin())
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1.1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'testclob1', "
		"	'testblob1')"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2.2, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'testclob2', "
		"	'testblob2')"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3.3, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'testclob3', "
		"	'testblob3')"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4.4, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'testclob4', "
		"	'testblob4')"))
	print()


	# affected rows
	print("AFFECTED ROWS: ")
	assertEquals(cur.affectedRows(),1)
	print()


	# input bind by position
	# sqlite doesn't support bind by position


	# array of input binds by position
	# sqlite doesn't support bind by position


	# input bind by position with validation
	# sqlite doesn't support bind by position


	# input bind by name
	print("INPUT BIND BY NAME: ")
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4, "
		"	:var5, "
		"	:var6)")
	assertEquals(cur.countBindVariables(),6)
	cur.inputBind("var1",5)
	cur.inputBind("var2",5.5,4,1)
	cur.inputBind("var3","testchar5")
	cur.inputBind("var4","testvarchar5")
	cur.inputBindClob("var5","testclob5",9)
	cur.inputBindBlob("var6","testblob5",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("var1",6)
	cur.inputBind("var2",6.6,4,1)
	cur.inputBind("var3","testchar6")
	cur.inputBind("var4","testvarchar6")
	cur.inputBindClob("var5","testclob6",9)
	cur.inputBindBlob("var6","testblob6",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("var1",7)
	cur.inputBind("var2",7.7,4,1)
	cur.inputBind("var3","testchar7")
	cur.inputBind("var4","testvarchar7")
	cur.inputBindClob("var5","testclob7",9)
	cur.inputBindBlob("var6","testblob7",9)
	assertTrue(cur.executeQuery())
	print()


	# array of input binds by name
	# sqlite doesn't support implicit conversion of string binds to other
	# data types, so arrays of binds don't generally work.


	# input bind by name with validation
	print("INPUT BIND BY NAME WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("var1",8)
	cur.inputBind("var2",8.8,4,1)
	cur.inputBind("var3","testchar8")
	cur.inputBind("var4","testvarchar8")
	cur.inputBindClob("var5","testclob8",9)
	cur.inputBindBlob("var6","testblob8",9)
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	print()


	# select
	print("SELECT: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	print()


	# column count
	print("COLUMN COUNT: ")
	assertEquals(cur.colCount(),6)
	print()


	# column names
	print("COLUMN NAMES: ")
	assertEquals(cur.getColumnName(0),"testint")
	assertEquals(cur.getColumnName(1),"testfloat")
	assertEquals(cur.getColumnName(2),"testchar")
	assertEquals(cur.getColumnName(3),"testvarchar")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"testint")
	assertEquals(cols[1],"testfloat")
	assertEquals(cols[2],"testchar")
	assertEquals(cols[3],"testvarchar")
	print()


	# column types
	print("COLUMN TYPES: ")
	if issqlite3:
		assertEquals(cur.getColumnType(0),"INTEGER")
		assertEquals(cur.getColumnType("testint"),"INTEGER")
		assertEquals(cur.getColumnType(1),"FLOAT")
		assertEquals(cur.getColumnType("testfloat"),"FLOAT")
		assertEquals(cur.getColumnType(2),"STRING")
		assertEquals(cur.getColumnType("testchar"),"STRING")
		assertEquals(cur.getColumnType(3),"STRING")
		assertEquals(cur.getColumnType("testvarchar"),"STRING")
		assertEquals(cur.getColumnType(4),"STRING")
		assertEquals(cur.getColumnType("testclob"),"STRING")
		assertEquals(cur.getColumnType(5),"STRING")
		assertEquals(cur.getColumnType("testblob"),"STRING")
	else:
		assertEquals(cur.getColumnType(0),"UNKNOWN")
		assertEquals(cur.getColumnType("testint"),"UNKNOWN")
		assertEquals(cur.getColumnType(1),"UNKNOWN")
		assertEquals(cur.getColumnType("testfloat"),"UNKNOWN")
		assertEquals(cur.getColumnType(2),"UNKNOWN")
		assertEquals(cur.getColumnType("testchar"),"UNKNOWN")
		assertEquals(cur.getColumnType(3),"UNKNOWN")
		assertEquals(cur.getColumnType("testvarchar"),"UNKNOWN")
		assertEquals(cur.getColumnType(4),"UNKNOWN")
		assertEquals(cur.getColumnType("testclob"),"UNKNOWN")
		assertEquals(cur.getColumnType(5),"UNKNOWN")
		assertEquals(cur.getColumnType("testblob"),"UNKNOWN")
	print()


	# column length
	print("COLUMN LENGTH: ")
	assertEquals(cur.getColumnLength(0),0)
	assertEquals(cur.getColumnLength("testint"),0)
	assertEquals(cur.getColumnLength(1),0)
	assertEquals(cur.getColumnLength("testfloat"),0)
	assertEquals(cur.getColumnLength(2),0)
	assertEquals(cur.getColumnLength("testchar"),0)
	assertEquals(cur.getColumnLength(3),0)
	assertEquals(cur.getColumnLength("testvarchar"),0)
	assertEquals(cur.getColumnLength(4),0)
	assertEquals(cur.getColumnLength("testclob"),0)
	assertEquals(cur.getColumnLength(5),0)
	assertEquals(cur.getColumnLength("testblob"),0)
	print()


	# longest column
	print("LONGEST COLUMN: ")
	assertEquals(cur.getLongest(0),1)
	assertEquals(cur.getLongest("testint"),1)
	assertEquals(cur.getLongest(1),3)
	assertEquals(cur.getLongest("testfloat"),3)
	assertEquals(cur.getLongest(2),9)
	assertEquals(cur.getLongest("testchar"),9)
	assertEquals(cur.getLongest(3),12)
	assertEquals(cur.getLongest("testvarchar"),12)
	assertEquals(cur.getLongest(4),9)
	assertEquals(cur.getLongest("testclob"),9)
	assertEquals(cur.getLongest(5),9)
	assertEquals(cur.getLongest("testblob"),9)
	print()


	# row count
	print("ROW COUNT: ")
	assertEquals(cur.rowCount(),8)
	print()


	# total rows
	print("TOTAL ROWS: ")
	assertEquals(cur.totalRows(),0 if issqlite3 else 8)
	print()


	# first row index
	print("FIRST ROW INDEX: ")
	assertEquals(cur.firstRowIndex(),0)
	print()


	# end of result set
	print("END OF RESULT SET: ")
	assertTrue(cur.endOfResultSet())
	print()


	# fields by index
	print("FIELDS BY INDEX: ")
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"1.1")
	assertEquals(cur.getField(0,2),"testchar1")
	assertEquals(cur.getField(0,3),"testvarchar1")
	assertEquals(cur.getField(0,4),"testclob1")
	assertEquals(cur.getField(0,5),"testblob1")
	print()
	assertEquals(cur.getField(7,0),"8")
	assertEquals(cur.getField(7,1),"8.8")
	assertEquals(cur.getField(7,2),"testchar8")
	assertEquals(cur.getField(7,3),"testvarchar8")
	assertEquals(cur.getField(7,4),"testclob8")
	assertEquals(cur.getField(7,5),"testblob8")
	print()


	# field lengths by index
	print("FIELD LENGTHS BY INDEX: ")
	assertEquals(cur.getFieldLength(0,0),1)
	assertEquals(cur.getFieldLength(0,1),3)
	assertEquals(cur.getFieldLength(0,2),9)
	assertEquals(cur.getFieldLength(0,3),12)
	assertEquals(cur.getFieldLength(0,4),9)
	assertEquals(cur.getFieldLength(0,5),9)
	print()
	assertEquals(cur.getFieldLength(7,0),1)
	assertEquals(cur.getFieldLength(7,1),3)
	assertEquals(cur.getFieldLength(7,2),9)
	assertEquals(cur.getFieldLength(7,3),12)
	assertEquals(cur.getFieldLength(7,4),9)
	assertEquals(cur.getFieldLength(7,5),9)
	print()


	# fields by name
	print("FIELDS BY NAME: ")
	assertEquals(cur.getField(0,"testint"),"1")
	assertEquals(cur.getField(0,"testfloat"),"1.1")
	assertEquals(cur.getField(0,"testchar"),"testchar1")
	assertEquals(cur.getField(0,"testvarchar"),"testvarchar1")
	assertEquals(cur.getField(0,"testclob"),"testclob1")
	assertEquals(cur.getField(0,"testblob"),"testblob1")
	print()
	assertEquals(cur.getField(7,"testint"),"8")
	assertEquals(cur.getField(7,"testfloat"),"8.8")
	assertEquals(cur.getField(7,"testchar"),"testchar8")
	assertEquals(cur.getField(7,"testvarchar"),"testvarchar8")
	assertEquals(cur.getField(7,"testclob"),"testclob8")
	assertEquals(cur.getField(7,"testblob"),"testblob8")
	print()


	# field lengths by name
	print("FIELD LENGTHS BY NAME: ")
	assertEquals(cur.getFieldLength(0,"testint"),1)
	assertEquals(cur.getFieldLength(0,"testfloat"),3)
	assertEquals(cur.getFieldLength(0,"testchar"),9)
	assertEquals(cur.getFieldLength(0,"testvarchar"),12)
	assertEquals(cur.getFieldLength(0,"testclob"),9)
	assertEquals(cur.getFieldLength(0,"testblob"),9)
	print()
	assertEquals(cur.getFieldLength(7,"testint"),1)
	assertEquals(cur.getFieldLength(7,"testfloat"),3)
	assertEquals(cur.getFieldLength(7,"testchar"),9)
	assertEquals(cur.getFieldLength(7,"testvarchar"),12)
	assertEquals(cur.getFieldLength(7,"testclob"),9)
	assertEquals(cur.getFieldLength(7,"testblob"),9)
	print()


	# fields by array
	print("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEquals(fields[0],"1")
	assertEquals(fields[1],"1.1")
	assertEquals(fields[2],"testchar1")
	assertEquals(fields[3],"testvarchar1")
	assertEquals(fields[4],"testclob1")
	assertEquals(fields[5],"testblob1")
	print()


	# field lengths by array
	print("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEquals(fieldlens[0],1)
	assertEquals(fieldlens[1],3)
	assertEquals(fieldlens[2],9)
	assertEquals(fieldlens[3],12)
	assertEquals(fieldlens[4],9)
	assertEquals(fieldlens[5],9)
	print()


	# result set buffer size
	print("RESULT SET BUFFER SIZE: ")
	assertEquals(cur.getResultSetBufferSize(),0)
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEquals(cur.getResultSetBufferSize(),2)
	print()
	assertEquals(cur.firstRowIndex(),0)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),2)
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(1,0),"2")
	assertEquals(cur.getField(2,0),"3")
	print()
	assertEquals(cur.firstRowIndex(),2)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),4)
	assertEquals(cur.getField(6,0),"7")
	assertEquals(cur.getField(7,0),"8")
	print()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,0))
	print()
	assertEquals(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	cur.setResultSetBufferSize(0)
	print()


	# dont get column info
	print("DONT GET COLUMN INFO: ")
	cur.dontGetColumnInfo()
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertNone(cur.getColumnName(0))
	assertEquals(cur.getColumnLength(0),0)
	assertNone(cur.getColumnType(0))
	cur.getColumnInfo()
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEquals(cur.getColumnName(0),"testint")
	assertEquals(cur.getColumnLength(0),0)
	assertEquals(cur.getColumnType(0),"INTEGER" if issqlite3 else "UNKNOWN")
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
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(1,0),"2")
	assertEquals(cur.getField(2,0),"3")
	assertEquals(cur.getField(3,0),"4")
	assertEquals(cur.getField(4,0),"5")
	assertEquals(cur.getField(5,0),"6")
	assertEquals(cur.getField(6,0),"7")
	assertEquals(cur.getField(7,0),"8")
	print()


	# suspended result set
	print("SUSPENDED RESULT SET: ")
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEquals(cur.getField(2,0),"3")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeResultSet(id))
	print()
	assertEquals(cur.firstRowIndex(),4)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),6)
	assertEquals(cur.getField(7,0),"8")
	print()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,0))
	print()
	assertEquals(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	cur.setResultSetBufferSize(0)
	print()


	# cached result set
	print("CACHED RESULT SET: ")
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,0),"8")
	print()


	# column count for cached result set
	print("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEquals(cur.colCount(),6)
	print()


	# column names for cached result set
	print("COLUMN NAMES FOR CACHED RESULT SET: ")
	assertEquals(cur.getColumnName(0),"testint")
	assertEquals(cur.getColumnName(1),"testfloat")
	assertEquals(cur.getColumnName(2),"testchar")
	assertEquals(cur.getColumnName(3),"testvarchar")
	assertEquals(cur.getColumnName(4),"testclob")
	assertEquals(cur.getColumnName(5),"testblob")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"testint")
	assertEquals(cols[1],"testfloat")
	assertEquals(cols[2],"testchar")
	assertEquals(cols[3],"testvarchar")
	assertEquals(cols[4],"testclob")
	assertEquals(cols[5],"testblob")
	print()


	# cached result set with result set buffer size
	print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	cur.setResultSetBufferSize(0)
	print()


	# from one cache file to another
	print("FROM ONE CACHE FILE TO ANOTHER: ")
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	print()


	# from one cache file to another with result set buffer size
	print("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	cur.setResultSetBufferSize(0)
	print()


	# cached result set with suspend and result set buffer size
	print("CACHED RESULT SET WITH SUSPEND "
				"AND RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEquals(cur.getField(2,0),"3")
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	print()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeCachedResultSet(id,filename))
	print()
	assertEquals(cur.firstRowIndex(),4)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),6)
	assertEquals(cur.getField(7,0),"8")
	print()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,0))
	print()
	assertEquals(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	cur.cacheOff()
	print()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	cur.setResultSetBufferSize(0)
	print()


	# finished suspended session
	print("FINISHED SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable"))
	assertEquals(cur.getField(4,0),"5")
	assertEquals(cur.getField(5,0),"6")
	assertEquals(cur.getField(6,0),"7")
	assertEquals(cur.getField(7,0),"8")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeResultSet(id))
	assertNone(cur.getField(4,0))
	assertNone(cur.getField(5,0))
	assertNone(cur.getField(6,0))
	assertNone(cur.getField(7,0))
	print()


	# nested selects
	print("NESTED SELECTS: ")
	cur.setResultSetBufferSize(1)
	assertTrue(cur.sendQuery("select * from testtable"))
	i=0
	while True:
		row=cur.getRow(i)
		if not row:
			break
		secondcur=PySQLRClient.sqlrcursor(con)
		secondcur.setResultSetBufferSize(1)
		assertTrue(secondcur.sendQuery("select * from testtable"))
		secondcur=None
		i+=1
	cur.setResultSetBufferSize(0)
	print()


	# commit and rollback
	print("COMMIT AND ROLLBACK: ")
	secondcon=PySQLRClient.sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
	secondcur=PySQLRClient.sqlrcursor(secondcon)
	asserts.setSecondConnection(secondcon)
	asserts.setSecondCursor(secondcur)
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"0")
	assertTrue(con.commit())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"8")
	assertTrue(con.begin())
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10.1, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'testclob10', "
		"	'testblob10')"))
	assertTrue(con.rollback())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"8")
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10.1, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'testclob10', "
		"	'testblob10')"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"9")
	secondcur=None
	secondcon=None
	asserts.setSecondCursor(None)
	asserts.setSecondConnection(None)
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	print()


	# individual substitutions
	print("INDIVIDUAL SUBSTITUTIONS: ")
	cur.sendQuery("drop table if exists testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int, "
		"	col2 char, "
		"	col3 float)"))
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))")
	cur.substitution("var1",1)
	cur.substitution("var2","hello")
	cur.substitution("var3",10.5556,6,4)
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("select * from testtable"))
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"10.5556")
	assertTrue(cur.sendQuery("delete from testtable"))
	print()


	# array substitutions
	print("ARRAY SUBSTITUTIONS: ")
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)')")
	cur.substitutions(subvars,subvalstrings)
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("select * from testtable"))
	assertEquals(cur.getField(0,0),"hi")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"bye")
	assertTrue(cur.sendQuery("delete from testtable"))
	print()
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))")
	cur.substitutions(subvars,subvallongs)
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("select * from testtable"))
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"2")
	assertEquals(cur.getField(0,2),"3.0")
	assertTrue(cur.sendQuery("delete from testtable"))
	print()
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))")
	cur.substitutions(subvars,subvaldoubles,precs,scales)
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("select * from testtable"))
	assertEquals(cur.getField(0,0),"10.55")
	assertEquals(cur.getField(0,1),"10.556")
	assertEquals(cur.getField(0,2),"10.5556")
	assertTrue(cur.sendQuery("delete from testtable"))
	print()


	# nulls as nulls
	print("NULLS AS NULLS: ")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	NULL, "
		"	NULL)"))
	assertTrue(cur.sendQuery("select * from testtable"))
	assertEquals(cur.getField(0,0),"1")
	assertNone(cur.getField(0,1))
	assertNone(cur.getField(0,2))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("select * from testtable"))
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"")
	assertEquals(cur.getField(0,2),"")
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	print()


	# null and empty lobs
	print("NULL AND EMPTY LOBS: ")
	cur.getNullsAsNone()
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)"))
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4)")
	cur.inputBindClob("var1","",0)
	cur.inputBindClob("var2",None,0)
	cur.inputBindBlob("var3","",0)
	cur.inputBindBlob("var4",None,0)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select * from testtable")
	assertEquals(cur.getField(0,0),"")
	assertNone(cur.getField(0,1))
	assertEquals(cur.getField(0,2),"")
	assertNone(cur.getField(0,3))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# long lobs
	print("LONG LOBS: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)")
	cur.prepareQuery("insert into testtable values (:clobval,:blobval)")
	largebuffer='C'*LARGE_BUFFER_LENGTH
	cur.inputBindClob("clobval",largebuffer,LARGE_BUFFER_LENGTH)
	cur.inputBindBlob("blobval",largebuffer,LARGE_BUFFER_LENGTH)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select * from testtable")
	assertEquals(cur.getFieldLength(0,"testclob"),LARGE_BUFFER_LENGTH)
	assertEquals(cur.getField(0,"testclob"),largebuffer)
	assertEquals(cur.getFieldLength(0,"testblob"),LARGE_BUFFER_LENGTH)
	assertEqualsBytes(cur.getField(0,"testblob"),largebuffer,
						LARGE_BUFFER_LENGTH)
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


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
	print("NEGATIVE INPUT BIND: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery("create table testtable (testval int)")
	cur.prepareQuery("insert into testtable values (:testval)")
	cur.inputBind("testval",-1)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select testval from testtable")
	assertEquals(cur.getField(0,"testval"),"-1")
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# bind validation
	print("BIND VALIDATION: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery(
		"create table testtable ("
		"	col1 varchar(20), "
		"	col2 varchar(20), "
		"	col3 varchar(20))")
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))")
	cur.inputBind("var1","1")
	cur.inputBind("var2","2")
	cur.inputBind("var3","3")
	cur.substitution("var1",":var1")
	assertTrue(cur.validBind("var1"))
	assertFalse(cur.validBind("var2"))
	assertFalse(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	print()
	cur.substitution("var2",":var2")
	assertTrue(cur.validBind("var1"))
	assertTrue(cur.validBind("var2"))
	assertFalse(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	print()
	cur.substitution("var3",":var3")
	assertTrue(cur.validBind("var1"))
	assertTrue(cur.validBind("var2"))
	assertTrue(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# rebinding
	print("REBINDING: ")
	cur.prepareQuery("select :val")
	cur.inputBind("val",1)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	cur.inputBind("val",2)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"2")
	cur.inputBind("val",3)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"3")
	print()


	# reexecute
	print("REEXECUTE: ")
	cur.prepareQuery("select 1")
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	print()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	print()
	cur.prepareQuery("select :var")
	cur.inputBind("var",1)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	print()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	print()
	cur.inputBind("var",2)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"2")
	print()


	# stored procedure returning no value
	# sqlite doesn't support stored procedures


	# stored procedure returning single value
	# sqlite doesn't support stored procedures


	# stored procedure returning multiple values
	# sqlite doesn't support stored procedures


	# stored procedure returning result set
	# sqlite doesn't support stored procedures


	# temporary tables
	print("TEMPORARY TABLES: ")
	cur.sendQuery("drop table if exists temptable")
	cur.sendQuery("create temporary table temptable (col1 int)")
	assertTrue(cur.sendQuery("insert into temptable values (1)"))
	assertTrue(cur.sendQuery("select count(*) from temptable"))
	assertEquals(cur.getField(0,0),"1")
	con.endSession()
	print()
	assertFalse(cur.sendQuery("select count(*) from temptable"))
	assertTrue(cur.sendQuery("drop table if exists temptable"))
	print()


	# encoded binary data
	print("ENCODED BINARY DATA: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery("create table testtable (col1 blob)"))
	buffer=bytes(range(256))
	hex=buffer.hex()
	querystr="insert into testtable values (X'"+hex+"')"
	assertTrue(cur.sendQuery(querystr))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEquals(cur.getFieldLength(0,0),256)
	assertEqualsBytes(cur.getField(0,0),buffer,256)
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# quotes
	print("QUOTES: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
	assertTrue(cur.sendQuery("insert into testtable values ('''''')"))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEquals(cur.getFieldLength(0,0),2)
	assertEquals(cur.getField(0,0),"''")
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# last insert id
	print("LAST INSERT ID: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
			"create table testtable "
			"	(col1 integer primary key "
			"	autoincrement, "
			"	col2 int)"))
	assertTrue(cur.sendQuery(
			"insert into testtable values (null,1)"))
	assertEquals(con.getLastInsertId(),1)
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# database is schema
	print("DATABASE IS SCHEMA: ")
	assertFalse(con.getDatabaseIsSchema())
	print()


	# catalog list
	print("CATALOG LIST: ")
	assertTrue(cur.getCatalogList(None))
	assertEquals(cur.getColumnName(0),"Database")
	print()


	# schema list
	print("SCHEMA LIST: ")
	assertTrue(cur.getSchemaList(None))
	assertEquals(cur.getColumnName(0),"Database")
	print()


	# table type list
	print("TABLE TYPE LIST: ")
	assertTrue(cur.getTableTypeList())
	assertEquals(cur.getColumnName(0),"table_type")
	found=False
	for i in range(cur.rowCount()):
		if cur.getField(i,"table_type")=="TABLE":
			found=True
			break
	assertTrue(found)
	print()


	# table list
	print("TABLE LIST: ")
	cur.sendQuery("drop table if exists testtable1")
	cur.sendQuery("drop table if exists testtable2")
	cur.sendQuery("drop table if exists testtable3")
	cur.sendQuery("drop table if exists testtable4")
	assertTrue(cur.sendQuery(
		"create table testtable1 ("
		"	col1 int, "
		"	col2 int)"))
	assertTrue(cur.sendQuery(
		"create table testtable2 ("
		"	col1 int, "
		"	col2 int)"))
	assertTrue(cur.sendQuery(
		"create table testtable3 ("
		"	col1 int, "
		"	col2 int)"))
	assertTrue(cur.sendQuery(
		"create table testtable4 ("
		"	col1 int, "
		"	col2 int)"))
	assertTrue(cur.getTableList(None))
	counter=0
	for i in range(cur.rowCount()):
		name=cur.getField(i,"Tables_in_xxx")
		if name in ("testtable1","testtable2","testtable3","testtable4"):
			counter+=1
	assertEquals(counter,4)
	assertTrue(cur.sendQuery("drop table if exists testtable1"))
	assertTrue(cur.sendQuery("drop table if exists testtable2"))
	assertTrue(cur.sendQuery("drop table if exists testtable3"))
	assertTrue(cur.sendQuery("drop table if exists testtable4"))
	print()


	# type info list
	print("TYPE INFO LIST: ")
	assertTrue(cur.getTypeInfoList("integer"))
	assertEquals(cur.getColumnName(0),"type_name")
	assertEquals(cur.getColumnName(1),"data_type")
	assertEquals(cur.getColumnName(2),"precision")
	assertEquals(cur.getColumnName(3),"literal_prefix")
	assertEquals(cur.getColumnName(4),"literal_suffix")
	assertEquals(cur.getColumnName(5),"create_params")
	assertEquals(cur.getColumnName(6),"nullable")
	assertEquals(cur.getColumnName(7),"case_sensitive")
	assertEquals(cur.getColumnName(8),"searchable")
	assertEquals(cur.getColumnName(9),"unsigned_attribute")
	assertEquals(cur.getColumnName(10),"fixed_prec_scale")
	assertEquals(cur.getColumnName(11),"auto_increment")
	assertEquals(cur.getColumnName(12),"local_type_name")
	assertEquals(cur.getColumnName(13),"minumum_scale")
	assertEquals(cur.getColumnName(14),"maxiumm_scale")
	assertEquals(cur.getColumnName(15),"sql_data_type")
	assertEquals(cur.getColumnName(16),"sql_datetime_sub")
	assertEquals(cur.getColumnName(17),"num_prec_radix")
	assertEquals(cur.getColumnName(18),"interval_precision")
	assertEquals(cur.getField(0,"type_name"),"INTEGER")
	assertEquals(cur.getField(0,"data_type"),"4")
	assertEquals(cur.getField(0,"precision"),"19")
	assertEquals(cur.getField(0,"local_type_name"),"INTEGER")
	assertTrue(cur.getTypeInfoList("char"))
	assertEquals(cur.getField(0,"type_name"),"CHAR")
	assertEquals(cur.getField(0,"data_type"),"1")
	assertEquals(cur.getField(0,"precision"),"2147483647")
	assertEquals(cur.getField(0,"local_type_name"),"CHAR")
	assertTrue(cur.getTypeInfoList("varchar"))
	assertEquals(cur.getField(0,"type_name"),"VARCHAR")
	assertEquals(cur.getField(0,"data_type"),"12")
	assertEquals(cur.getField(0,"precision"),"2147483647")
	assertEquals(cur.getField(0,"local_type_name"),"VARCHAR")
	assertTrue(cur.getTypeInfoList("date"))
	assertEquals(cur.getField(0,"type_name"),"DATE")
	assertEquals(cur.getField(0,"data_type"),"91")
	assertEquals(cur.getField(0,"precision"),"10")
	assertEquals(cur.getField(0,"local_type_name"),"DATE")
	print()


	# column list
	print("COLUMN LIST: ")
	cur.sendQuery("drop table if exists testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testclob clob, "
		"	testblob blob)"))
	assertTrue(cur.getColumnList("testtable",None))
	assertEquals(cur.getColumnName(0),"column_name")
	assertEquals(cur.getColumnName(1),"data_type")
	assertEquals(cur.getColumnName(2),"character_maximum_length")
	assertEquals(cur.getColumnName(3),"numeric_precision")
	assertEquals(cur.getColumnName(4),"numeric_scale")
	assertEquals(cur.getColumnName(5),"is_nullable")
	assertEquals(cur.getColumnName(6),"column_key")
	assertEquals(cur.getColumnName(7),"column_default")
	assertEquals(cur.getColumnName(8),"extra")
	assertEquals(cur.getField(0,"column_name"),"testint")
	assertEquals(cur.getField(1,"column_name"),"testfloat")
	assertEquals(cur.getField(2,"column_name"),"testchar")
	assertEquals(cur.getField(3,"column_name"),"testvarchar")
	assertEquals(cur.getField(4,"column_name"),"testclob")
	assertEquals(cur.getField(5,"column_name"),"testblob")
	assertEquals(cur.getField(0,"data_type"),"INT")
	assertEquals(cur.getField(1,"data_type"),"FLOAT")
	assertEquals(cur.getField(2,"data_type"),"CHAR")
	assertEquals(cur.getField(3,"data_type"),"VARCHAR")
	assertEquals(cur.getField(4,"data_type"),"CLOB")
	assertEquals(cur.getField(5,"data_type"),"BLOB")
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	print()


	# column list - auto_increment, primary key
	print("COLUMN LIST - auto_increment, primary key: ")
	cur.sendQuery("drop table if exists testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 integer primary key autoincrement, "
		"	col2 int)"))
	assertTrue(cur.getColumnList("testtable",None))
	assertTrue("auto_increment" in cur.getField(0,"extra"))
	assertTrue("PRI" in cur.getField(0,"column_key"))
	assertFalse("auto_increment" in cur.getField(1,"extra"))
	assertFalse("PRI" in cur.getField(1,"column_key"))
	print()
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"))
	assertTrue(cur.getColumnList("testtable",None))
	assertFalse("auto_increment" in cur.getField(0,"extra"))
	assertTrue("PRI" in cur.getField(0,"column_key"))
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	print()


	# primary keys list
	print("PRIMARY KEYS LIST: ")
	cur.sendQuery("drop table if exists testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"))
	assertTrue(cur.getPrimaryKeysList("testtable",None))
	assertEquals(cur.getColumnName(0),"table")
	assertEquals(cur.getColumnName(1),"non_unique")
	assertEquals(cur.getColumnName(2),"key_name")
	assertEquals(cur.getColumnName(3),"seq_in_index")
	assertEquals(cur.getColumnName(4),"column_name")
	assertEquals(cur.getColumnName(5),"collation")
	assertEquals(cur.getColumnName(6),"cardinality")
	assertEquals(cur.getColumnName(7),"sub_part")
	assertEquals(cur.getColumnName(8),"packed")
	assertEquals(cur.getColumnName(9),"null")
	assertEquals(cur.getColumnName(10),"index_type")
	assertEquals(cur.getColumnName(11),"comment")
	assertEquals(cur.getColumnName(12),"index_comment")
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,"table"),"testtable")
	assertEquals(cur.getField(0,"seq_in_index"),"1")
	assertEquals(cur.getField(0,"column_name"),"col1")
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	print()


	# key and index list
	print("KEY AND INDEX LIST: ")
	cur.sendQuery("drop table if exists testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"))
	assertTrue(cur.getKeyAndIndexList("testtable",None))
	assertEquals(cur.getColumnName(0),"table")
	assertEquals(cur.getColumnName(1),"non_unique")
	assertEquals(cur.getColumnName(2),"key_name")
	assertEquals(cur.getColumnName(3),"seq_in_index")
	assertEquals(cur.getColumnName(4),"column_name")
	assertEquals(cur.getColumnName(5),"collation")
	assertEquals(cur.getColumnName(6),"cardinality")
	assertEquals(cur.getColumnName(7),"sub_part")
	assertEquals(cur.getColumnName(8),"packed")
	assertEquals(cur.getColumnName(9),"null")
	assertEquals(cur.getColumnName(10),"index_type")
	assertEquals(cur.getColumnName(11),"comment")
	assertEquals(cur.getColumnName(12),"index_comment")
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,"table"),"testtable")
	assertEquals(cur.getField(0,"non_unique"),"0")
	assertEquals(cur.getField(0,"seq_in_index"),"1")
	assertEquals(cur.getField(0,"column_name"),"col1")
	assertEquals(cur.getField(0,"collation"),"A")
	assertEquals(cur.getField(0,"index_type"),"3")
	keyname=cur.getField(0,"key_name")
	assertTrue(keyname is not None and len(keyname)>0)
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	print()


	# procedure list
	print("PROCEDURE LIST: ")
	assertTrue(cur.getProcedureList(None))
	assertEquals(cur.rowCount(),0)
	print()


	# procedure parameter list
	print("PROCEDURE PARAMETER LIST: ")
	assertTrue(cur.getProcedureParameterList("testproc1",None))
	assertEquals(cur.getColumnName(0),"parameter_name")
	assertEquals(cur.getColumnName(1),"parameter_mode")
	assertEquals(cur.getColumnName(2),"data_type")
	assertEquals(cur.getColumnName(3),"character_maximum_length")
	assertEquals(cur.getColumnName(4),"ordinal_position")
	assertEquals(cur.rowCount(),0)
	print()


	# invalid queries
	print("INVALID QUERIES: ")
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
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


	reportTestStatus()
	sys.exit(asserts.status)


main()
