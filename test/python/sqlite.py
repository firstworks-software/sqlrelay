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
	output("IDENTIFY: ")
	assertEquals(con.identify(),"sqlite")
	output()


	# db version
	output("DB VERSION: ")
	dbversion=con.dbVersion()
	issqlite3=True
	if (dbversion is None or
		dbversion=="unknown" or
		len(dbversion)==0 or
		not dbversion[0].isdigit() or
		int(dbversion[0])<3):
		issqlite3=False
	output()


	# ping
	output("PING: ")
	assertTrue(con.ping())
	output()


	# transaction state
	output("TRANSACTION STATE: ")
	assertEquals(con.getDefaultTransactionModel(),"explicit")
	assertEquals(con.getTransactionModel(),"explicit")
	assertFalse(con.getInTransaction())
	assertTrue(con.getAutoCommit())
	output()


	# bind format
	output("BIND FORMAT: ")
	assertEquals(con.bindFormat(),":*")
	output()


	# nextval format
	output("NEXTVAL FORMAT: ")
	assertEquals(con.nextvalFormat(),"")
	output()


	# isolation levels
	output("ISOLATION LEVELS: ")
	for il in isolationlevels:
		assertTrue(con.setIsolationLevel(il))
		assertEquals(con.getIsolationLevel(),il)
		output()
	# reset to the default isolation level
	assertTrue(con.setIsolationLevel(isolationlevels[0]))
	output()


	# create testtable
	output("CREATE TESTTABLE: ")
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
	output()


	# insert
	output("INSERT: ")
	assertTrue(con.begin())
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1.5, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'testclob1', "
		"	'testblob1')"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2.5, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'testclob2', "
		"	'testblob2')"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3.5, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'testclob3', "
		"	'testblob3')"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4.5, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'testclob4', "
		"	'testblob4')"))
	output()


	# affected rows
	output("AFFECTED ROWS: ")
	assertEquals(cur.affectedRows(),1)
	output()


	# input bind by position
	# sqlite doesn't support bind by position


	# array of input binds by position
	# sqlite doesn't support bind by position


	# input bind by position with validation
	# sqlite doesn't support bind by position


	# input bind by name
	output("INPUT BIND BY NAME: ")
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
	cur.inputBind("var2",6.5,4,1)
	cur.inputBind("var3","testchar6")
	cur.inputBind("var4","testvarchar6")
	cur.inputBindClob("var5","testclob6",9)
	cur.inputBindBlob("var6","testblob6",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("var1",7)
	cur.inputBind("var2",7.5,4,1)
	cur.inputBind("var3","testchar7")
	cur.inputBind("var4","testvarchar7")
	cur.inputBindClob("var5","testclob7",9)
	cur.inputBindBlob("var6","testblob7",9)
	assertTrue(cur.executeQuery())
	output()


	# array of input binds by name
	# sqlite doesn't support implicit conversion of string binds to other
	# data types, so arrays of binds don't generally work.


	# input bind by name with validation
	output("INPUT BIND BY NAME WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("var1",8)
	cur.inputBind("var2",8.5,4,1)
	cur.inputBind("var3","testchar8")
	cur.inputBind("var4","testvarchar8")
	cur.inputBindClob("var5","testclob8",9)
	cur.inputBindBlob("var6","testblob8",9)
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	output()


	# select
	output("SELECT: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	output()


	# column count
	output("COLUMN COUNT: ")
	assertEquals(cur.colCount(),6)
	output()


	# column names
	output("COLUMN NAMES: ")
	assertEquals(cur.getColumnName(0),"testint")
	assertEquals(cur.getColumnName(1),"testfloat")
	assertEquals(cur.getColumnName(2),"testchar")
	assertEquals(cur.getColumnName(3),"testvarchar")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"testint")
	assertEquals(cols[1],"testfloat")
	assertEquals(cols[2],"testchar")
	assertEquals(cols[3],"testvarchar")
	output()


	# column types
	output("COLUMN TYPES: ")
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
	output()


	# column length
	output("COLUMN LENGTH: ")
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
	output()


	# longest column
	output("LONGEST COLUMN: ")
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
	output()


	# row count
	output("ROW COUNT: ")
	assertEquals(cur.rowCount(),8)
	output()


	# total rows
	output("TOTAL ROWS: ")
	assertEquals(cur.totalRows(),0 if issqlite3 else 8)
	output()


	# first row index
	output("FIRST ROW INDEX: ")
	assertEquals(cur.firstRowIndex(),0)
	output()


	# end of result set
	output("END OF RESULT SET: ")
	assertTrue(cur.endOfResultSet())
	output()


	# fields by index
	output("FIELDS BY INDEX: ")
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"1.5")
	assertEquals(cur.getField(0,2),"testchar1")
	assertEquals(cur.getField(0,3),"testvarchar1")
	assertEquals(cur.getField(0,4),"testclob1")
	assertEquals(cur.getField(0,5),"testblob1")
	output()
	assertEquals(cur.getField(7,0),"8")
	assertEquals(cur.getField(7,1),"8.5")
	assertEquals(cur.getField(7,2),"testchar8")
	assertEquals(cur.getField(7,3),"testvarchar8")
	assertEquals(cur.getField(7,4),"testclob8")
	assertEquals(cur.getField(7,5),"testblob8")
	output()


	# field lengths by index
	output("FIELD LENGTHS BY INDEX: ")
	assertEquals(cur.getFieldLength(0,0),1)
	assertEquals(cur.getFieldLength(0,1),3)
	assertEquals(cur.getFieldLength(0,2),9)
	assertEquals(cur.getFieldLength(0,3),12)
	assertEquals(cur.getFieldLength(0,4),9)
	assertEquals(cur.getFieldLength(0,5),9)
	output()
	assertEquals(cur.getFieldLength(7,0),1)
	assertEquals(cur.getFieldLength(7,1),3)
	assertEquals(cur.getFieldLength(7,2),9)
	assertEquals(cur.getFieldLength(7,3),12)
	assertEquals(cur.getFieldLength(7,4),9)
	assertEquals(cur.getFieldLength(7,5),9)
	output()


	# fields by name
	output("FIELDS BY NAME: ")
	assertEquals(cur.getField(0,"testint"),"1")
	assertEquals(cur.getField(0,"testfloat"),"1.5")
	assertEquals(cur.getField(0,"testchar"),"testchar1")
	assertEquals(cur.getField(0,"testvarchar"),"testvarchar1")
	assertEquals(cur.getField(0,"testclob"),"testclob1")
	assertEquals(cur.getField(0,"testblob"),"testblob1")
	output()
	assertEquals(cur.getField(7,"testint"),"8")
	assertEquals(cur.getField(7,"testfloat"),"8.5")
	assertEquals(cur.getField(7,"testchar"),"testchar8")
	assertEquals(cur.getField(7,"testvarchar"),"testvarchar8")
	assertEquals(cur.getField(7,"testclob"),"testclob8")
	assertEquals(cur.getField(7,"testblob"),"testblob8")
	output()


	# field lengths by name
	output("FIELD LENGTHS BY NAME: ")
	assertEquals(cur.getFieldLength(0,"testint"),1)
	assertEquals(cur.getFieldLength(0,"testfloat"),3)
	assertEquals(cur.getFieldLength(0,"testchar"),9)
	assertEquals(cur.getFieldLength(0,"testvarchar"),12)
	assertEquals(cur.getFieldLength(0,"testclob"),9)
	assertEquals(cur.getFieldLength(0,"testblob"),9)
	output()
	assertEquals(cur.getFieldLength(7,"testint"),1)
	assertEquals(cur.getFieldLength(7,"testfloat"),3)
	assertEquals(cur.getFieldLength(7,"testchar"),9)
	assertEquals(cur.getFieldLength(7,"testvarchar"),12)
	assertEquals(cur.getFieldLength(7,"testclob"),9)
	assertEquals(cur.getFieldLength(7,"testblob"),9)
	output()


	# fields by array
	output("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEquals(fields[0],"1")
	assertEquals(fields[1],"1.5")
	assertEquals(fields[2],"testchar1")
	assertEquals(fields[3],"testvarchar1")
	assertEquals(fields[4],"testclob1")
	assertEquals(fields[5],"testblob1")
	output()


	# field lengths by array
	output("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEquals(fieldlens[0],1)
	assertEquals(fieldlens[1],3)
	assertEquals(fieldlens[2],9)
	assertEquals(fieldlens[3],12)
	assertEquals(fieldlens[4],9)
	assertEquals(fieldlens[5],9)
	output()


	# result set buffer size
	output("RESULT SET BUFFER SIZE: ")
	assertEquals(cur.getResultSetBufferSize(),0)
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEquals(cur.getResultSetBufferSize(),2)
	output()
	assertEquals(cur.firstRowIndex(),0)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),2)
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(1,0),"2")
	assertEquals(cur.getField(2,0),"3")
	output()
	assertEquals(cur.firstRowIndex(),2)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),4)
	assertEquals(cur.getField(6,0),"7")
	assertEquals(cur.getField(7,0),"8")
	output()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,0))
	output()
	assertEquals(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	cur.setResultSetBufferSize(0)
	output()


	# dont get column info
	output("DONT GET COLUMN INFO: ")
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
	output()


	# suspended session
	output("SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	output()
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(1,0),"2")
	assertEquals(cur.getField(2,0),"3")
	assertEquals(cur.getField(3,0),"4")
	assertEquals(cur.getField(4,0),"5")
	assertEquals(cur.getField(5,0),"6")
	assertEquals(cur.getField(6,0),"7")
	assertEquals(cur.getField(7,0),"8")
	output()


	# suspended result set
	output("SUSPENDED RESULT SET: ")
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
	output()
	assertEquals(cur.firstRowIndex(),4)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),6)
	assertEquals(cur.getField(7,0),"8")
	output()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,0))
	output()
	assertEquals(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	cur.setResultSetBufferSize(0)
	output()


	# cached result set
	output("CACHED RESULT SET: ")
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,0),"8")
	output()


	# column count for cached result set
	output("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEquals(cur.colCount(),6)
	output()


	# column names for cached result set
	output("COLUMN NAMES FOR CACHED RESULT SET: ")
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
	output()


	# cached result set with result set buffer size
	output("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ")
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
	output()


	# from one cache file to another
	output("FROM ONE CACHE FILE TO ANOTHER: ")
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	output()


	# from one cache file to another with result set buffer size
	output("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	cur.setResultSetBufferSize(0)
	output()


	# cached result set with suspend and result set buffer size
	output("CACHED RESULT SET WITH SUSPEND "
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
	output()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeCachedResultSet(id,filename))
	output()
	assertEquals(cur.firstRowIndex(),4)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),6)
	assertEquals(cur.getField(7,0),"8")
	output()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,0))
	output()
	assertEquals(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	cur.cacheOff()
	output()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	cur.setResultSetBufferSize(0)
	output()


	# finished suspended session
	output("FINISHED SUSPENDED SESSION: ")
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
	output()


	# nested selects
	output("NESTED SELECTS: ")
	cur.setResultSetBufferSize(1)
	assertTrue(cur.sendQuery("select * from testtable"))
	secondcur=PySQLRClient.sqlrcursor(con)
	secondcur.setResultSetBufferSize(1)
	i=0
	while True:
		row=cur.getRow(i)
		if not row:
			break
		assertTrue(secondcur.sendQuery("select * from testtable"))
		i+=1
	secondcur.closeResultSet()
	cur.setResultSetBufferSize(0)
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	output()


	# reset transaction state
	output("RESET TRANSACTION STATE: ")
	assertTrue(con.commit())
	assertEquals(con.getTransactionModel(),"explicit")
	assertTrue(con.getAutoCommit())
	output()


	# transaction behavior - implicit
	output("TRANSACTION BEHAVIOR - implicit: ")
	assertTrue(con.setTransactionModel("implicit"))
	assertEquals(con.getTransactionModel(),"implicit")
	assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
	# sqlite DDL is transactional; commit so the table is visible
	# to the second connection (the commit implicitly starts a new tx)
	assertTrue(con.commit())
	secondcon=PySQLRClient.sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
	secondcur=PySQLRClient.sqlrcursor(secondcon)
	asserts.setSecondConnection(secondcon)
	asserts.setSecondCursor(secondcur)
	# session is in a transaction; insert is not visible until commit
	assertTrue(con.getInTransaction())
	assertFalse(con.getAutoCommit())
	assertTrue(cur.sendQuery("insert into testtable values (1)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"0")
	# commit makes it visible, and implicitly starts a new transaction
	assertTrue(con.commit())
	assertTrue(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# rollback discards, and implicitly starts a new transaction
	assertTrue(cur.sendQuery("insert into testtable values (2)"))
	assertTrue(con.rollback())
	assertTrue(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# autoCommitOn takes effect immediately
	assertTrue(con.autoCommitOn())
	assertTrue(con.getAutoCommit())
	assertFalse(con.getInTransaction())
	assertTrue(cur.sendQuery("insert into testtable values (3)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"2")
	# autoCommitOff takes effect immediately
	assertTrue(con.autoCommitOff())
	assertFalse(con.getAutoCommit())
	assertTrue(con.getInTransaction())
	secondcur.closeResultSet()
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# transaction behavior - explicit
	output("TRANSACTION BEHAVIOR - explicit: ")
	assertTrue(con.setTransactionModel("explicit"))
	assertEquals(con.getTransactionModel(),"explicit")
	assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
	# begin starts a new transaction; insert is not visible until commit
	assertTrue(con.begin())
	assertTrue(con.getInTransaction())
	assertTrue(cur.sendQuery("insert into testtable values (1)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"0")
	# commit makes it visible; no new transaction is started
	assertTrue(con.commit())
	assertFalse(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# begin, insert, rollback discards; no new transaction is started
	assertTrue(con.begin())
	assertTrue(cur.sendQuery("insert into testtable values (2)"))
	assertTrue(con.rollback())
	assertFalse(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# autoCommitOn takes effect immediately
	assertTrue(con.autoCommitOn())
	assertTrue(con.getAutoCommit())
	assertTrue(cur.sendQuery("insert into testtable values (3)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"2")
	# autoCommitOff takes effect immediately
	assertTrue(con.autoCommitOff())
	assertFalse(con.getAutoCommit())
	secondcur.closeResultSet()
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# transaction behavior - explicit-deferred
	output("TRANSACTION BEHAVIOR - explicit-deferred: ")
	assertTrue(con.setTransactionModel("explicit-deferred"))
	assertEquals(con.getTransactionModel(),"explicit-deferred")
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
	assertEquals(secondcur.getField(0,0),"1")
	# begin, insert, rollback discards
	assertTrue(con.begin())
	assertTrue(cur.sendQuery("insert into testtable values (2)"))
	assertTrue(con.rollback())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# during a transaction started by begin(), autoCommitOn is a
	# no-op: the autocommit setting takes effect after the user
	# explicitly commits/rollbacks the tx (mysql-native semantic)
	assertTrue(con.begin())
	assertTrue(cur.sendQuery("insert into testtable values (3)"))
	assertTrue(con.autoCommitOn())
	assertFalse(con.getAutoCommit())
	assertTrue(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# explicit commit ends the tx; autocommit-on now takes effect
	assertTrue(con.commit())
	assertTrue(con.getAutoCommit())
	assertFalse(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"2")
	# autocommit is on; subsequent inserts are visible immediately
	assertTrue(cur.sendQuery("insert into testtable values (4)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"3")
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
	assertEquals(secondcur.getField(0,0),"4")
	assertTrue(cur.sendQuery("insert into testtable values (6)"))
	assertTrue(con.rollback())
	assertFalse(con.getAutoCommit())
	assertTrue(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"4")
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
	assertEquals(secondcur.getField(0,0),"4")
	assertTrue(con.commit())
	assertFalse(con.getAutoCommit())
	assertTrue(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"5")
	secondcur.closeResultSet()
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# transaction behavior - explicit-error
	output("TRANSACTION BEHAVIOR - explicit-error: ")
	assertTrue(con.setTransactionModel("explicit-error"))
	assertEquals(con.getTransactionModel(),"explicit-error")
	assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
	# begin, insert, commit
	assertTrue(con.begin())
	assertTrue(con.getInTransaction())
	assertTrue(cur.sendQuery("insert into testtable values (1)"))
	assertTrue(con.commit())
	assertFalse(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# begin, insert, rollback
	assertTrue(con.begin())
	assertTrue(cur.sendQuery("insert into testtable values (2)"))
	assertTrue(con.rollback())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
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
	assertEquals(secondcur.getField(0,0),"2")
	# autoCommitOff takes effect immediately
	assertTrue(con.autoCommitOff())
	assertFalse(con.getAutoCommit())
	secondcur.closeResultSet()
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# transaction behavior - none
	output("TRANSACTION BEHAVIOR - none: ")
	assertTrue(con.setTransactionModel("none"))
	assertEquals(con.getTransactionModel(),"none")
	assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
	# no transactions; everything is visible immediately
	assertTrue(con.getAutoCommit())
	assertFalse(con.getInTransaction())
	assertTrue(cur.sendQuery("insert into testtable values (1)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
	# commit and rollback are no-ops
	assertTrue(con.commit())
	assertTrue(cur.sendQuery("insert into testtable values (2)"))
	assertTrue(con.rollback())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"2")
	# autocommit is always on; autoCommitOff is an error
	assertFalse(con.autoCommitOff())
	assertTrue(con.getAutoCommit())
	assertTrue(con.autoCommitOn())
	assertTrue(con.getAutoCommit())
	secondcur.closeResultSet()
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# reset transaction behavior
	output("RESET TRANSACTION BEHAVIOR: ")
	assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()))
	assertEquals(con.getTransactionModel(),"explicit")
	assertTrue(con.getAutoCommit())
	output()


	# individual substitutions
	output("INDIVIDUAL SUBSTITUTIONS: ")
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
	output()


	# array substitutions
	output("ARRAY SUBSTITUTIONS: ")
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
	output()
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
	output()
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
	output()


	# nulls as nulls
	output("NULLS AS NULLS: ")
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
	output()


	# null and empty lobs
	output("NULL AND EMPTY LOBS: ")
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
	# blob column comes back as bytes in Python
	assertEquals(cur.getField(0,2),b"")
	assertNone(cur.getField(0,3))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# long lobs
	output("LONG LOBS: ")
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
	# blob column comes back as bytes in Python
	assertEqualsBytes(cur.getField(0,"testblob"),largebuffer.encode(),
						LARGE_BUFFER_LENGTH)
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


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
	output("NEGATIVE INPUT BIND: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery("create table testtable (testval int)")
	cur.prepareQuery("insert into testtable values (:testval)")
	cur.inputBind("testval",-1)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select testval from testtable")
	assertEquals(cur.getField(0,"testval"),"-1")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# bind validation
	output("BIND VALIDATION: ")
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
	output()
	cur.substitution("var2",":var2")
	assertTrue(cur.validBind("var1"))
	assertTrue(cur.validBind("var2"))
	assertFalse(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	output()
	cur.substitution("var3",":var3")
	assertTrue(cur.validBind("var1"))
	assertTrue(cur.validBind("var2"))
	assertTrue(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# rebinding
	output("REBINDING: ")
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
	output()


	# reexecute
	output("REEXECUTE: ")
	cur.prepareQuery("select 1")
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	cur.prepareQuery("select :var")
	cur.inputBind("var",1)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	cur.inputBind("var",2)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"2")
	output()


	# stored procedure returning no value
	# sqlite doesn't support stored procedures


	# stored procedure returning single value
	# sqlite doesn't support stored procedures


	# stored procedure returning multiple values
	# sqlite doesn't support stored procedures


	# stored procedure returning result set
	# sqlite doesn't support stored procedures


	# temporary tables
	output("TEMPORARY TABLES: ")
	cur.sendQuery("drop table if exists temptable")
	cur.sendQuery("create temporary table temptable (col1 int)")
	assertTrue(cur.sendQuery("insert into temptable values (1)"))
	assertTrue(cur.sendQuery("select count(*) from temptable"))
	assertEquals(cur.getField(0,0),"1")
	con.endSession()
	output()
	assertFalse(cur.sendQuery("select count(*) from temptable"))
	assertTrue(cur.sendQuery("drop table if exists temptable"))
	output()


	# encoded binary data
	output("ENCODED BINARY DATA: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery("create table testtable (col1 blob)"))
	buffer=bytearray(range(256))
	hex="".join("%02x"%b for b in buffer)
	querystr="insert into testtable values (X'"+hex+"')"
	assertTrue(cur.sendQuery(querystr))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEquals(cur.getFieldLength(0,0),256)
	assertEqualsBytes(cur.getField(0,0),buffer,256)
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# quotes
	output("QUOTES: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
	assertTrue(cur.sendQuery("insert into testtable values ('''''')"))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEquals(cur.getFieldLength(0,0),2)
	assertEquals(cur.getField(0,0),"''")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# last insert id
	output("LAST INSERT ID: ")
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
	output()


	# database is schema
	output("DATABASE IS SCHEMA: ")
	assertFalse(con.getDatabaseIsSchema())
	output()


	# catalog list
	output("CATALOG LIST: ")
	assertTrue(cur.getCatalogList(None))
	assertEquals(cur.getColumnName(0),"Database")
	assertEquals(cur.rowCount(),0)
	output()


	# schema list
	output("SCHEMA LIST: ")
	assertTrue(cur.getSchemaList(None))
	assertEquals(cur.getColumnName(0),"Database")
	output()


	# table type list
	output("TABLE TYPE LIST: ")
	assertTrue(cur.getTableTypeList())
	assertEquals(cur.getColumnName(0),"table_type")
	assertInResultSet(cur,"table_type","TABLE")
	output()


	# table list
	output("TABLE LIST: ")
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
	assertInResultSet(cur,"Tables_in_xxx","testtable1")
	assertInResultSet(cur,"Tables_in_xxx","testtable2")
	assertInResultSet(cur,"Tables_in_xxx","testtable3")
	assertInResultSet(cur,"Tables_in_xxx","testtable4")
	assertTrue(cur.sendQuery("drop table if exists testtable1"))
	assertTrue(cur.sendQuery("drop table if exists testtable2"))
	assertTrue(cur.sendQuery("drop table if exists testtable3"))
	assertTrue(cur.sendQuery("drop table if exists testtable4"))
	output()


	# type info list
	output("TYPE INFO LIST: ")
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
	output()


	# column list
	output("COLUMN LIST: ")
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
	output()


	# column list - auto_increment, primary key
	output("COLUMN LIST - auto_increment, primary key: ")
	cur.sendQuery("drop table if exists testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 integer primary key autoincrement, "
		"	col2 int)"))
	assertTrue(cur.getColumnList("testtable",None))
	assertEquals(cur.getField(0,"extra"),"auto_increment")
	assertEquals(cur.getField(0,"column_key"),"PRI")
	assertEquals(cur.getField(1,"extra"),"")
	assertEquals(cur.getField(1,"column_key"),"")
	output()
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"))
	assertTrue(cur.getColumnList("testtable",None))
	assertEquals(cur.getField(0,"extra"),"")
	assertEquals(cur.getField(0,"column_key"),"PRI")
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	output()


	# primary keys list
	output("PRIMARY KEYS LIST: ")
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
	output()


	# key and index list
	output("KEY AND INDEX LIST: ")
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
	assertEquals(keyname,"sqlite_autoindex_testtable_1")
	assertTrue(cur.sendQuery("drop table if exists testtable"))
	output()


	# procedure list
	output("PROCEDURE LIST: ")
	assertTrue(cur.getProcedureList(None))
	assertEquals(cur.rowCount(),0)
	output()


	# procedure parameter list
	output("PROCEDURE PARAMETER LIST: ")
	assertTrue(cur.getProcedureParameterList("testproc1",None))
	assertEquals(cur.getColumnName(0),"parameter_name")
	assertEquals(cur.getColumnName(1),"parameter_mode")
	assertEquals(cur.getColumnName(2),"data_type")
	assertEquals(cur.getColumnName(3),"character_maximum_length")
	assertEquals(cur.getColumnName(4),"ordinal_position")
	assertEquals(cur.rowCount(),0)
	output()


	# invalid queries
	output("INVALID QUERIES: ")
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	output()
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	output()
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	output()


	reportTestStatus()
	sys.exit(asserts.status)


main()
