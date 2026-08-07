#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.


from SQLRelay import PySQLRClient
import sys
import socket
import time
import asserts
from asserts import *


def main():

	isolationlevels=["READ COMMITTED","SERIALIZABLE"]
	bindvars=["1","2","3","4","5"]
	bindvals=["4","testchar4","testvarchar4","01-JAN-2004","testlong4"]
	arraybindvars=["var1","var2","var3","var4","var5"]
	arraybindvals=["7","testchar7","testvarchar7","01-JAN-2007","testlong7"]
	subvars=["var1","var2","var3"]
	subvallongs=[1,2,3]
	subvalstrings=["hi","hello","bye"]
	subvaldoubles=[10.55,10.556,10.5556]
	precs=[4,5,6]
	scales=[2,3,4]

	LARGE_BUFFER_LENGTH=8192

	cert="../sqlrelay.conf.d/tls/client.pem"
	ca="../sqlrelay.conf.d/tls/ca.pem"


	# hostname
	hostname=socket.gethostname().split(".")[0].lower()


	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9012,"/tmp/tlstest.socket",
						None,None,0,1)
	cur=PySQLRClient.sqlrcursor(con)
	con.enableTls(None,cert,None,None,"ca",ca,0)
	asserts.setConnection(con)
	asserts.setCursor(cur)


	# identify
	output("IDENTIFY: ")
	assertEquals(con.identify(),"oracle")
	output()


	# ping
	output("PING: ")
	assertTrue(con.ping())
	output()


	# transaction state
	output("TRANSACTION STATE: ")
	assertEquals(con.getDefaultTransactionModel(),"implicit")
	assertEquals(con.getTransactionModel(),"implicit")
	assertTrue(con.getInTransaction())
	assertFalse(con.getAutoCommit())
	output()


	# bind format
	output("BIND FORMAT: ")
	assertEquals(con.bindFormat(),":*")
	output()


	# nextval format
	output("NEXTVAL FORMAT: ")
	assertEquals(con.nextvalFormat(),"%s.nextval")
	output()


	# isolation levels
	output("ISOLATION LEVELS: ")
	for il in isolationlevels:
		# oracle requires the isolation level to
		# be the first query of the transaction
		assertTrue(con.commit())
		# you can set the isolation level, but to get it, you have to
		# have permisisons to read from sys.v_$session and
		# sys.v_$transaction
		assertTrue(con.setIsolationLevel(il))
		output()
	# reset to the default isolation level
	assertTrue(con.commit())
	assertTrue(con.setIsolationLevel(isolationlevels[0]))
	output()


	# create testtable
	output("CREATE TESTTABLE: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"))
	output()


	# insert
	output("INSERT: ")
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01-JAN-2001', "
		"	'testlong1', "
		"	'testclob1', "
		"	empty_blob())"))
	output()


	# affected rows
	output("AFFECTED ROWS: ")
	assertEquals(cur.affectedRows(),1)
	output()


	# input bind by position
	output("INPUT BIND BY POSITION: ")
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4, "
		"	:var5, "
		"	:var6, "
		"	:var7)")
	assertEquals(cur.countBindVariables(),7)
	cur.inputBind("1",2)
	cur.inputBind("2","testchar2")
	cur.inputBind("3","testvarchar2")
	cur.inputBindDate("4",2002,1,1,0,0,0,0,None,False)
	cur.inputBind("5","testlong2")
	cur.inputBindClob("6","testclob2",9)
	cur.inputBindBlob("7","testblob2",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1",3)
	cur.inputBind("2","testchar3")
	cur.inputBind("3","testvarchar3")
	cur.inputBindDate("4",2003,1,1,0,0,0,0,None,False)
	cur.inputBind("5","testlong3")
	cur.inputBindClob("6","testclob3",9)
	cur.inputBindBlob("7","testblob3",9)
	assertTrue(cur.executeQuery())
	output()


	# array of input binds by position
	output("ARRAY OF INPUT BINDS BY POSITION: ")
	cur.clearBinds()
	cur.inputBinds(bindvars,bindvals)
	cur.inputBindClob("6","testclob4",9)
	cur.inputBindBlob("7","testblob4",9)
	assertTrue(cur.executeQuery())
	output()


	# input bind by position with validation
	output("INPUT BIND BY POSITION WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("1",5)
	cur.inputBind("2","testchar5")
	cur.inputBind("3","testvarchar5")
	cur.inputBindDate("4",2005,1,1,0,0,0,0,None,False)
	cur.inputBind("5","testlong5")
	cur.inputBindClob("6","testclob5",9)
	cur.inputBindBlob("7","testblob5",9)
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	cur.clearBinds()


	# input bind by name
	output("INPUT BIND BY NAME: ")
	cur.clearBinds()
	cur.inputBind("var1",6)
	cur.inputBind("var2","testchar6")
	cur.inputBind("var3","testvarchar6")
	cur.inputBindDate("var4",2006,1,1,0,0,0,0,None,False)
	cur.inputBind("var5","testlong6")
	cur.inputBindClob("var6","testclob6",9)
	cur.inputBindBlob("var7","testblob6",9)
	assertTrue(cur.executeQuery())
	output()


	# array of input binds by name
	output("ARRAY OF INPUT BINDS BY NAME: ")
	cur.clearBinds()
	cur.inputBinds(arraybindvars,arraybindvals)
	cur.inputBindClob("var6","testclob7",9)
	cur.inputBindBlob("var7","testblob7",9)
	assertTrue(cur.executeQuery())
	output()


	# input bind by name with validation
	output("INPUT BIND BY NAME WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("var1",8)
	cur.inputBind("var2","testchar8")
	cur.inputBind("var3","testvarchar8")
	cur.inputBindDate("var4",2008,1,1,0,0,0,0,None,False)
	cur.inputBind("var5","testlong8")
	cur.inputBindClob("var6","testclob8",9)
	cur.inputBindBlob("var7","testblob8",9)
	cur.inputBind("var9","junkvalue")
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	output()


	# select
	output("SELECT: ")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	output()


	# column count
	output("COLUMN COUNT: ")
	assertEquals(cur.colCount(),7)
	output()


	# column names
	output("COLUMN NAMES: ")
	assertEquals(cur.getColumnName(0),"TESTNUMBER")
	assertEquals(cur.getColumnName(1),"TESTCHAR")
	assertEquals(cur.getColumnName(2),"TESTVARCHAR")
	assertEquals(cur.getColumnName(3),"TESTDATE")
	assertEquals(cur.getColumnName(4),"TESTLONG")
	assertEquals(cur.getColumnName(5),"TESTCLOB")
	assertEquals(cur.getColumnName(6),"TESTBLOB")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"TESTNUMBER")
	assertEquals(cols[1],"TESTCHAR")
	assertEquals(cols[2],"TESTVARCHAR")
	assertEquals(cols[3],"TESTDATE")
	assertEquals(cols[4],"TESTLONG")
	assertEquals(cols[5],"TESTCLOB")
	assertEquals(cols[6],"TESTBLOB")
	output()


	# column types
	output("COLUMN TYPES: ")
	assertEquals(cur.getColumnType(0),"NUMBER")
	assertEquals(cur.getColumnType("TESTNUMBER"),"NUMBER")
	assertEquals(cur.getColumnType(1),"CHAR")
	assertEquals(cur.getColumnType("TESTCHAR"),"CHAR")
	assertEquals(cur.getColumnType(2),"VARCHAR2")
	assertEquals(cur.getColumnType("TESTVARCHAR"),"VARCHAR2")
	assertEquals(cur.getColumnType(3),"DATE")
	assertEquals(cur.getColumnType("TESTDATE"),"DATE")
	assertEquals(cur.getColumnType(4),"LONG")
	assertEquals(cur.getColumnType("TESTLONG"),"LONG")
	assertEquals(cur.getColumnType(5),"CLOB")
	assertEquals(cur.getColumnType("TESTCLOB"),"CLOB")
	assertEquals(cur.getColumnType(6),"BLOB")
	assertEquals(cur.getColumnType("TESTBLOB"),"BLOB")
	output()


	# column length
	output("COLUMN LENGTH: ")
	assertEquals(cur.getColumnLength(0),22)
	assertEquals(cur.getColumnLength("TESTNUMBER"),22)
	assertEquals(cur.getColumnLength(1),40)
	assertEquals(cur.getColumnLength("TESTCHAR"),40)
	assertEquals(cur.getColumnLength(2),40)
	assertEquals(cur.getColumnLength("TESTVARCHAR"),40)
	assertEquals(cur.getColumnLength(3),7)
	assertEquals(cur.getColumnLength("TESTDATE"),7)
	assertEquals(cur.getColumnLength(4),0)
	assertEquals(cur.getColumnLength("TESTLONG"),0)
	assertEquals(cur.getColumnLength(5),0)
	assertEquals(cur.getColumnLength("TESTCLOB"),0)
	assertEquals(cur.getColumnLength(6),0)
	assertEquals(cur.getColumnLength("TESTBLOB"),0)
	output()


	# longest column
	output("LONGEST COLUMN: ")
	assertEquals(cur.getLongest(0),1)
	assertEquals(cur.getLongest("TESTNUMBER"),1)
	assertEquals(cur.getLongest(1),40)
	assertEquals(cur.getLongest("TESTCHAR"),40)
	assertEquals(cur.getLongest(2),12)
	assertEquals(cur.getLongest("TESTVARCHAR"),12)
	assertEquals(cur.getLongest(3),9)
	assertEquals(cur.getLongest("TESTDATE"),9)
	assertEquals(cur.getLongest(4),9)
	assertEquals(cur.getLongest("TESTLONG"),9)
	assertEquals(cur.getLongest(5),9)
	assertEquals(cur.getLongest("TESTCLOB"),9)
	assertEquals(cur.getLongest(6),9)
	assertEquals(cur.getLongest("TESTBLOB"),9)
	output()


	# row count
	output("ROW COUNT: ")
	assertEquals(cur.rowCount(),8)
	output()


	# total rows
	output("TOTAL ROWS: ")
	assertEquals(cur.totalRows(),0)
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
	assertEquals(cur.getField(0,1),"testchar1                               ")
	assertEquals(cur.getField(0,2),"testvarchar1")
	assertEquals(cur.getField(0,3),"01-JAN-01")
	assertEquals(cur.getField(0,4),"testlong1")
	assertEquals(cur.getField(0,5),"testclob1")
	assertEquals(cur.getField(0,6),b"")  # blob returns bytes
	output()
	assertEquals(cur.getField(7,0),"8")
	assertEquals(cur.getField(7,1),"testchar8                               ")
	assertEquals(cur.getField(7,2),"testvarchar8")
	assertEquals(cur.getField(7,3),"01-JAN-08")
	assertEquals(cur.getField(7,4),"testlong8")
	assertEquals(cur.getField(7,5),"testclob8")
	assertEquals(cur.getField(7,6),b"testblob8")
	output()


	# field lengths by index
	output("FIELD LENGTHS BY INDEX: ")
	assertEquals(cur.getFieldLength(0,0),1)
	assertEquals(cur.getFieldLength(0,1),40)
	assertEquals(cur.getFieldLength(0,2),12)
	assertEquals(cur.getFieldLength(0,3),9)
	assertEquals(cur.getFieldLength(0,4),9)
	assertEquals(cur.getFieldLength(0,5),9)
	assertEquals(cur.getFieldLength(0,6),0)
	output()
	assertEquals(cur.getFieldLength(7,0),1)
	assertEquals(cur.getFieldLength(7,1),40)
	assertEquals(cur.getFieldLength(7,2),12)
	assertEquals(cur.getFieldLength(7,3),9)
	assertEquals(cur.getFieldLength(7,4),9)
	assertEquals(cur.getFieldLength(7,5),9)
	assertEquals(cur.getFieldLength(7,6),9)
	output()


	# fields by name
	output("FIELDS BY NAME: ")
	assertEquals(cur.getField(0,"TESTNUMBER"),"1")
	assertEquals(cur.getField(0,"TESTCHAR"),"testchar1                               ")
	assertEquals(cur.getField(0,"TESTVARCHAR"),"testvarchar1")
	assertEquals(cur.getField(0,"TESTDATE"),"01-JAN-01")
	assertEquals(cur.getField(0,"TESTLONG"),"testlong1")
	assertEquals(cur.getField(0,"TESTCLOB"),"testclob1")
	assertEquals(cur.getField(0,"TESTBLOB"),b"")
	output()
	assertEquals(cur.getField(7,"TESTNUMBER"),"8")
	assertEquals(cur.getField(7,"TESTCHAR"),"testchar8                               ")
	assertEquals(cur.getField(7,"TESTVARCHAR"),"testvarchar8")
	assertEquals(cur.getField(7,"TESTDATE"),"01-JAN-08")
	assertEquals(cur.getField(7,"TESTLONG"),"testlong8")
	assertEquals(cur.getField(7,"TESTCLOB"),"testclob8")
	assertEquals(cur.getField(7,"TESTBLOB"),b"testblob8")
	output()


	# field lengths by name
	output("FIELD LENGTHS BY NAME: ")
	assertEquals(cur.getFieldLength(0,"TESTNUMBER"),1)
	assertEquals(cur.getFieldLength(0,"TESTCHAR"),40)
	assertEquals(cur.getFieldLength(0,"TESTVARCHAR"),12)
	assertEquals(cur.getFieldLength(0,"TESTDATE"),9)
	assertEquals(cur.getFieldLength(0,"TESTLONG"),9)
	assertEquals(cur.getFieldLength(0,"TESTCLOB"),9)
	assertEquals(cur.getFieldLength(0,"TESTBLOB"),0)
	output()
	assertEquals(cur.getFieldLength(7,"TESTNUMBER"),1)
	assertEquals(cur.getFieldLength(7,"TESTCHAR"),40)
	assertEquals(cur.getFieldLength(7,"TESTVARCHAR"),12)
	assertEquals(cur.getFieldLength(7,"TESTDATE"),9)
	assertEquals(cur.getFieldLength(7,"TESTLONG"),9)
	assertEquals(cur.getFieldLength(7,"TESTCLOB"),9)
	assertEquals(cur.getFieldLength(7,"TESTBLOB"),9)
	output()


	# fields by array
	output("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEquals(fields[0],"1")
	assertEquals(fields[1],"testchar1                               ")
	assertEquals(fields[2],"testvarchar1")
	assertEquals(fields[3],"01-JAN-01")
	assertEquals(fields[4],"testlong1")
	assertEquals(fields[5],"testclob1")
	assertEquals(fields[6],b"")
	output()


	# field lengths by array
	output("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEquals(fieldlens[0],1)
	assertEquals(fieldlens[1],40)
	assertEquals(fieldlens[2],12)
	assertEquals(fieldlens[3],9)
	assertEquals(fieldlens[4],9)
	assertEquals(fieldlens[5],9)
	assertEquals(fieldlens[6],0)
	output()


	# result set buffer size
	output("RESULT SET BUFFER SIZE: ")
	assertEquals(cur.getResultSetBufferSize(),0)
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertEquals(cur.getResultSetBufferSize(),2)
	output()
	assertEquals(cur.firstRowIndex(),0)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),2)
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(1,0),"2")
	assertEquals(cur.getField(2,0),"3")
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertNone(cur.getColumnName(0))
	assertEquals(cur.getColumnLength(0),0)
	assertNone(cur.getColumnType(0))
	cur.getColumnInfo()
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertEquals(cur.getColumnName(0),"TESTNUMBER")
	assertEquals(cur.getColumnLength(0),22)
	assertEquals(cur.getColumnType(0),"NUMBER")
	output()


	# suspended session
	output("SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socketname=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socketname))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socketname=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socketname))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socketname=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socketname))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertEquals(cur.getField(2,0),"3")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socketname=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socketname))
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
	cur.cacheToFile("cachefile1-tls")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1-tls")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,0),"8")
	output()


	# column count for cached result set
	output("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEquals(cur.colCount(),7)
	output()


	# column names for cached result set
	output("COLUMN NAMES FOR CACHED RESULT SET: ")
	assertEquals(cur.getColumnName(0),"TESTNUMBER")
	assertEquals(cur.getColumnName(1),"TESTCHAR")
	assertEquals(cur.getColumnName(2),"TESTVARCHAR")
	assertEquals(cur.getColumnName(3),"TESTDATE")
	assertEquals(cur.getColumnName(4),"TESTLONG")
	assertEquals(cur.getColumnName(5),"TESTCLOB")
	assertEquals(cur.getColumnName(6),"TESTBLOB")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"TESTNUMBER")
	assertEquals(cols[1],"TESTCHAR")
	assertEquals(cols[2],"TESTVARCHAR")
	assertEquals(cols[3],"TESTDATE")
	assertEquals(cols[4],"TESTLONG")
	assertEquals(cols[5],"TESTCLOB")
	assertEquals(cols[6],"TESTBLOB")
	output()


	# cached result set with result set buffer size
	output("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1-tls")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1-tls")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	cur.setResultSetBufferSize(0)
	output()


	# from one cache file to another
	output("FROM ONE CACHE FILE TO ANOTHER: ")
	cur.cacheToFile("cachefile2-tls")
	assertTrue(cur.openCachedResultSet("cachefile1-tls"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2-tls"))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	output()


	# from one cache file to another with result set buffer size
	output("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile2-tls")
	assertTrue(cur.openCachedResultSet("cachefile1-tls"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2-tls"))
	assertEquals(cur.getField(7,0),"8")
	assertNone(cur.getField(8,0))
	cur.setResultSetBufferSize(0)
	output()


	# cached result set with suspend and result set buffer size
	output("CACHED RESULT SET WITH SUSPEND "
				"AND RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1-tls")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertEquals(cur.getField(2,0),"3")
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1-tls")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socketname=con.getConnectionSocket()
	output()
	assertTrue(con.resumeSession(port,socketname))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertEquals(cur.getField(4,0),"5")
	assertEquals(cur.getField(5,0),"6")
	assertEquals(cur.getField(6,0),"7")
	assertEquals(cur.getField(7,0),"8")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socketname=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socketname))
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
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# reset transaction state
	output("RESET TRANSACTION STATE: ")
	assertTrue(con.commit())
	assertEquals(con.getTransactionModel(),"implicit")
	assertFalse(con.getAutoCommit())
	output()


	# transaction behavior - implicit
	output("TRANSACTION BEHAVIOR - implicit: ")
	assertTrue(con.setTransactionModel("implicit"))
	assertEquals(con.getTransactionModel(),"implicit")
	assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
	secondcon=PySQLRClient.sqlrconnection("sqlrelay",9012,"/tmp/tlstest.socket",
						None,None,0,1)
	secondcur=PySQLRClient.sqlrcursor(secondcon)
	secondcon.enableTls(None,cert,None,None,"ca",ca,0)
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
	assertEquals(con.getTransactionModel(),"implicit")
	assertFalse(con.getAutoCommit())
	output()


	# individual substitutions
	output("INDIVIDUAL SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual")
	cur.substitution("var1","$(var11)")
	cur.substitution("var2","$(var21)")
	cur.substitution("var3","$(var31)")
	cur.substitution("var11","$(var111)")
	cur.substitution("var21","$(var211)")
	cur.substitution("var31","$(var311)")
	cur.substitution("var111",1)
	cur.substitution("var211","hello")
	cur.substitution("var311",10.5556,6,4)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"10.5556")
	output()


	# array substitutions
	output("ARRAY SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),$(var2),$(var3) from dual")
	cur.substitutions(subvars,subvallongs)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"2")
	assertEquals(cur.getField(0,2),"3")
	output()
	cur.prepareQuery("select '$(var1)','$(var2)','$(var3)' from dual")
	cur.substitutions(subvars,subvalstrings)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"hi")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"bye")
	output()
	cur.prepareQuery("select $(var1),$(var2),$(var3) from dual")
	cur.substitutions(subvars,subvaldoubles,precs,scales)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"10.55")
	assertEquals(cur.getField(0,1),"10.556")
	assertEquals(cur.getField(0,2),"10.5556")
	output()


	# nulls as nulls
	output("NULLS AS NULLS: ")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery("select NULL,1,NULL from dual"))
	assertNone(cur.getField(0,0))
	assertEquals(cur.getField(0,1),"1")
	assertNone(cur.getField(0,2))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("select NULL,1,NULL from dual"))
	assertEquals(cur.getField(0,0),"")
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(0,2),"")
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
	assertEquals(cur.getFieldLength(0,"TESTCLOB"),LARGE_BUFFER_LENGTH)
	assertEquals(cur.getField(0,"TESTCLOB"),largebuffer)
	assertEquals(cur.getFieldLength(0,"TESTBLOB"),LARGE_BUFFER_LENGTH)
	# blob column comes back as bytes in Python
	assertEqualsBytes(cur.getField(0,"TESTBLOB"),largebuffer.encode(),
						LARGE_BUFFER_LENGTH)
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# output bind by position
	output("OUTPUT BIND BY POSITION: ")
	cur.getNullsAsNone()
	cur.prepareQuery(
		"begin "
		"	:numvar:=1; "
		"	:stringvar:='hello'; "
		"	:floatvar:=2.5; "
		"	:datevar:='03-FEB-2001'; "
		"	:nullvar:=null; "
		"end;")
	assertEquals(cur.countBindVariables(),5)
	cur.defineOutputBindInteger("1")
	cur.defineOutputBindString("2",10)
	cur.defineOutputBindDouble("3")
	cur.defineOutputBindDate("4")
	cur.defineOutputBindString("5",10)
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
	nullvar=cur.getOutputBindString("5")
	assertEquals(numvar,1)
	assertEquals(stringvar,"hello")
	assertEquals(floatvar,2.5)
	assertEquals(year,2001)
	assertEquals(month,2)
	assertEquals(day,3)
	assertEquals(hour,0)
	assertEquals(minute,0)
	assertEquals(second,0)
	assertEquals(microsecond,0)
	assertEquals(tz,"")
	assertEquals(isnegative,False)
	assertNone(nullvar)
	cur.getNullsAsEmptyStrings()
	output()


	# output bind by name
	output("OUTPUT BIND BY NAME: ")
	cur.getNullsAsNone()
	cur.clearBinds()
	cur.defineOutputBindInteger("numvar")
	cur.defineOutputBindString("stringvar",10)
	cur.defineOutputBindDouble("floatvar")
	cur.defineOutputBindDate("datevar")
	cur.defineOutputBindString("nullvar",10)
	assertTrue(cur.executeQuery())
	numvar=cur.getOutputBindInteger("numvar")
	stringvar=cur.getOutputBindString("stringvar")
	floatvar=cur.getOutputBindDouble("floatvar")
	year=cur.getOutputBindDateYear("datevar")
	month=cur.getOutputBindDateMonth("datevar")
	day=cur.getOutputBindDateDay("datevar")
	hour=cur.getOutputBindDateHour("datevar")
	minute=cur.getOutputBindDateMinute("datevar")
	second=cur.getOutputBindDateSecond("datevar")
	microsecond=cur.getOutputBindDateMicrosecond("datevar")
	tz=cur.getOutputBindDateTz("datevar")
	isnegative=cur.getOutputBindDateIsNegative("datevar")
	nullvar=cur.getOutputBindString("nullvar")
	assertEquals(numvar,1)
	assertEquals(stringvar,"hello")
	assertEquals(floatvar,2.5)
	assertEquals(year,2001)
	assertEquals(month,2)
	assertEquals(day,3)
	assertEquals(hour,0)
	assertEquals(minute,0)
	assertEquals(second,0)
	assertEquals(microsecond,0)
	assertEquals(tz,"")
	assertEquals(isnegative,False)
	assertNone(nullvar)
	cur.getNullsAsEmptyStrings()
	output()


	# output bind by name with validation
	output("OUTPUT BIND BY NAME WITH VALIDATION: ")
	cur.getNullsAsNone()
	cur.clearBinds()
	cur.defineOutputBindInteger("numvar")
	cur.defineOutputBindString("stringvar",10)
	cur.defineOutputBindDouble("floatvar")
	cur.defineOutputBindDate("datevar")
	cur.defineOutputBindString("nullvar",10)
	cur.defineOutputBindString("dummyvar",10)
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	numvar=cur.getOutputBindInteger("numvar")
	stringvar=cur.getOutputBindString("stringvar")
	floatvar=cur.getOutputBindDouble("floatvar")
	year=cur.getOutputBindDateYear("datevar")
	month=cur.getOutputBindDateMonth("datevar")
	day=cur.getOutputBindDateDay("datevar")
	hour=cur.getOutputBindDateHour("datevar")
	minute=cur.getOutputBindDateMinute("datevar")
	second=cur.getOutputBindDateSecond("datevar")
	microsecond=cur.getOutputBindDateMicrosecond("datevar")
	tz=cur.getOutputBindDateTz("datevar")
	isnegative=cur.getOutputBindDateIsNegative("datevar")
	nullvar=cur.getOutputBindString("nullvar")
	assertEquals(numvar,1)
	assertEquals(stringvar,"hello")
	assertEquals(floatvar,2.5)
	assertEquals(year,2001)
	assertEquals(month,2)
	assertEquals(day,3)
	assertEquals(hour,0)
	assertEquals(minute,0)
	assertEquals(second,0)
	assertEquals(microsecond,0)
	assertEquals(tz,"")
	assertEquals(isnegative,False)
	assertNone(nullvar)
	cur.getNullsAsEmptyStrings()
	output()


	# lob output bind
	output("LOB OUTPUT BIND: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"))
	cur.prepareQuery("insert into testtable values ('hello',:var1)")
	cur.inputBindBlob("var1","hello",5)
	assertTrue(cur.executeQuery())
	cur.prepareQuery(
		"begin "
		"	select testclob into :clobvar from testtable; "
		"	select testblob into :blobvar from testtable; "
		"end;")
	cur.defineOutputBindClob("clobvar")
	cur.defineOutputBindBlob("blobvar")
	assertTrue(cur.executeQuery())
	clobvar=cur.getOutputBindClob("clobvar")
	clobvarlength=cur.getOutputBindLength("clobvar")
	blobvar=cur.getOutputBindBlob("blobvar")
	blobvarlength=cur.getOutputBindLength("blobvar")
	assertEqualsBytes(clobvar,"hello",5)
	assertEquals(clobvarlength,5)
	# blob output bind comes back as bytes in Python
	assertEqualsBytes(blobvar,b"hello",5)
	assertEquals(blobvarlength,5)
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# long output bind
	output("LONG OUTPUT BIND: ")
	largebuffer='C'*LARGE_BUFFER_LENGTH
	query="begin :bindval:='"+largebuffer+"'; end;"
	cur.prepareQuery(query)
	cur.defineOutputBindString("bindval",LARGE_BUFFER_LENGTH)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindLength("bindval"),LARGE_BUFFER_LENGTH)
	assertEquals(cur.getOutputBindString("bindval"),largebuffer)
	output()


	# negative input bind
	output("NEGATIVE INPUT BIND: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery("create table testtable (testval number)")
	cur.prepareQuery("insert into testtable values (:testval)")
	cur.inputBind("testval",-1)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select testval from testtable")
	assertEquals(cur.getField(0,"TESTVAL"),"-1")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# bind validation
	output("BIND VALIDATION: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery(
		"create table testtable ("
		"	col1 varchar2(20), "
		"	col2 varchar2(20), "
		"	col3 varchar2(20))")
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
	cur.prepareQuery(
		"begin "
		"	:out:= :in; "
		"end;")
	cur.inputBind("in",1)
	cur.defineOutputBindInteger("out")
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("out"),1)
	cur.inputBind("in",2)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("out"),2)
	cur.inputBind("in",3)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("out"),3)
	output()


	# reexecute
	output("REEXECUTE: ")
	cur.prepareQuery("select 1 from dual")
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	cur.prepareQuery("select :var from dual")
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
	output("STORED PROCEDURE RETURNING NO VALUE: ")
	cur.sendQuery("drop function testproc")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create or replace "
		"procedure testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2) "
		"is "
		"begin "
		"	return; "
		"end;"))
	cur.prepareQuery("begin testproc(:in1,:in2,:in3); end;")
	cur.inputBind("in1",1)
	cur.inputBind("in2",2.5,2,1)
	cur.inputBind("in3","hello")
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("drop procedure testproc"))
	output()


	# stored procedure returning single value
	output("STORED PROCEDURE RETURNING SINGLE VALUE: ")
	cur.sendQuery("drop function testproc")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create or replace "
		"function testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2) "
		"	return number "
		"is "
		"begin "
		"	return in1; "
		"end;"))
	cur.prepareQuery("select testproc(:in1,:in2,:in3) from dual")
	cur.inputBind("in1",1)
	cur.inputBind("in2",2.5,2,1)
	cur.inputBind("in3","hello")
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	cur.prepareQuery(
		"begin "
		"	:out1:=testproc(:in1,:in2,:in3); "
		"end;")
	cur.inputBind("in1",1)
	cur.inputBind("in2",2.5,2,1)
	cur.inputBind("in3","hello")
	cur.defineOutputBindInteger("out1")
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("out1"),1)
	assertTrue(cur.sendQuery("drop function testproc"))
	output()


	# stored procedure returning multiple values
	output("STORED PROCEDURE RETURNING MULTIPLE VALUES: ")
	cur.sendQuery("drop function testproc")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create or replace "
		"procedure testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2, "
		"	out1 out number, "
		"	out2 out number, "
		"	out3 out varchar2) "
		"is "
		"begin "
		"	out1:=in1; "
		"	out2:=in2; "
		"	out3:=in3; "
		"end;"))
	cur.prepareQuery(
		"begin "
		"	testproc(:in1,:in2,:in3,:out1,:out2,:out3); "
		"end;")
	cur.inputBind("in1",1)
	cur.inputBind("in2",2.5,2,1)
	cur.inputBind("in3","hello")
	cur.defineOutputBindInteger("out1")
	cur.defineOutputBindDouble("out2")
	cur.defineOutputBindString("out3",20)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("out1"),1)
	assertEquals(cur.getOutputBindDouble("out2"),2.5)
	assertEquals(cur.getOutputBindString("out3"),"hello")
	assertTrue(cur.sendQuery("drop procedure testproc"))
	output()


	# stored procedure returning result set
	output("STORED PROCEDURE RETURNING RESULT SET: ")
	cur.sendQuery("drop package types")
	cur.sendQuery("drop function testproc")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create or replace package types is "
		"	type cursorType is ref cursor; "
		"end;"))
	assertTrue(cur.sendQuery(
		"create or replace "
		"function testproc(value in number) "
		"	return types.cursortype "
		"is "
		"	l_cursor    types.cursorType; "
		"begin "
		"	open l_cursor for "
		"		select "
		"			* "
		"		from "
		"			( "
		"			select 1 as testnumber from dual "
		"			union "
		"			select 2 as testnumber from dual "
		"			union "
		"			select 3 as testnumber from dual "
		"			union "
		"			select 4 as testnumber from dual "
		"			union "
		"			select 5 as testnumber from dual "
		"			union "
		"			select 6 as testnumber from dual "
		"			union "
		"			select 7 as testnumber from dual "
		"			union "
		"			select 8 as testnumber from dual "
		"			) "
		"		where "
		"			testnumber>value; "
		"	return l_cursor; "
		"end;"))
	cur.prepareQuery(
		"begin "
		"	:curs1:=testproc(5); "
		"	:curs2:=testproc(0); "
		"end;")
	cur.defineOutputBindCursor("curs1")
	cur.defineOutputBindCursor("curs2")
	assertTrue(cur.executeQuery())
	bindcur1=cur.getOutputBindCursor("curs1")
	assertTrue(bindcur1.fetchFromBindCursor())
	assertEquals(bindcur1.getField(0,0),"6")
	assertEquals(bindcur1.getField(1,0),"7")
	assertEquals(bindcur1.getField(2,0),"8")
	bindcur1=None
	bindcur2=cur.getOutputBindCursor("curs2")
	assertTrue(bindcur2.fetchFromBindCursor())
	assertEquals(bindcur2.getField(0,0),"1")
	assertEquals(bindcur2.getField(1,0),"2")
	assertEquals(bindcur2.getField(2,0),"3")
	bindcur2=None
	assertTrue(cur.sendQuery("drop function testproc"))
	assertTrue(cur.sendQuery("drop package types"))
	output()


	# temporary tables
	output("TEMPORARY TABLES: ")
	cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete")
	cur.substitution("HOSTNAME",hostname)
	cur.executeQuery()
	cur.prepareQuery(
		"create global temporary table $(HOSTNAME)_temptabledelete ( "
		"	col1 number "
		") on commit delete rows")
	cur.substitution("HOSTNAME",hostname)
	cur.executeQuery()
	cur.prepareQuery("insert into $(HOSTNAME)_temptabledelete values (1)")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	cur.prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertTrue(con.commit())
	cur.prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"0")
	cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete")
	cur.substitution("HOSTNAME",hostname)
	cur.executeQuery()
	output()
	cur.prepareQuery("truncate table $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	cur.executeQuery()
	cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	cur.executeQuery()
	cur.prepareQuery(
		"create global temporary table $(HOSTNAME)_temptablepreserve ("
		"	col1 number "
		") on commit preserve rows")
	cur.substitution("HOSTNAME",hostname)
	cur.executeQuery()
	cur.prepareQuery(
		"insert into "
		"	$(HOSTNAME)_temptablepreserve "
		"values (1)")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertTrue(con.commit())
	cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	con.endSession()
	output()
	cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"0")
	cur.prepareQuery("truncate table $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	time.sleep(2)
	cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	assertTrue(cur.executeQuery())
	cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve")
	cur.substitution("HOSTNAME",hostname)
	assertFalse(cur.executeQuery())
	output()


	# encoded binary data
	output("ENCODED BINARY DATA: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery("create table testtable (col1 blob)"))
	buffer=bytearray(range(256))
	hex="".join("%02x"%b for b in buffer)
	querystr="insert into testtable values ('"+hex+"')"
	assertTrue(cur.sendQuery(querystr))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEquals(cur.getFieldLength(0,0),256)
	assertEqualsBytes(cur.getField(0,0),buffer,256)
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# quotes
	output("QUOTES: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery("create table testtable (col1 varchar2(4))"))
	assertTrue(cur.sendQuery("insert into testtable values ('''''')"))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEquals(cur.getFieldLength(0,0),2)
	assertEquals(cur.getField(0,0),"''")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# last insert id
	# oracle doesn't support auto-increment


	# database is schema
	output("DATABASE IS SCHEMA: ")
	assertTrue(con.getDatabaseIsSchema())
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
	assertInResultSet(cur,"Database",hostname.upper())
	output()


	# table type list
	output("TABLE TYPE LIST: ")
	assertTrue(cur.getTableTypeList())
	assertEquals(cur.getColumnName(0),"table_type")
	assertEquals(cur.getField(0,"table_type"),"SYNONYM")
	assertEquals(cur.getField(1,"table_type"),"TABLE")
	assertEquals(cur.getField(2,"table_type"),"VIEW")
	output()


	# table list
	output("TABLE LIST: ")
	cur.sendQuery("drop table testtable1")
	cur.sendQuery("drop table testtable2")
	cur.sendQuery("drop table testtable3")
	cur.sendQuery("drop table testtable4")
	assertTrue(cur.sendQuery(
		"create table testtable1 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"))
	assertTrue(cur.sendQuery(
		"create table testtable2 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"))
	assertTrue(cur.sendQuery(
		"create table testtable3 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"))
	assertTrue(cur.sendQuery(
		"create table testtable4 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"))
	assertTrue(cur.getTableList(None))
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1")
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2")
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3")
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE4")
	assertTrue(cur.sendQuery("drop table testtable1"))
	assertTrue(cur.sendQuery("drop table testtable2"))
	assertTrue(cur.sendQuery("drop table testtable3"))
	assertTrue(cur.sendQuery("drop table testtable4"))
	output()


	# type info list
	output("TYPE INFO LIST: ")
	assertTrue(cur.getTypeInfoList("number"))
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
	assertEquals(cur.getField(0,"type_name"),"NUMBER")
	assertEquals(cur.getField(0,"data_type"),"-7")
	assertEquals(cur.getField(0,"precision"),"1")
	assertEquals(cur.getField(0,"local_type_name"),"NUMBER")
	assertTrue(cur.getTypeInfoList("char"))
	assertEquals(cur.getField(0,"type_name"),"CHAR")
	assertEquals(cur.getField(0,"data_type"),"1")
	assertEquals(cur.getField(0,"precision"),"2000")
	assertEquals(cur.getField(0,"local_type_name"),"CHAR")
	assertTrue(cur.getTypeInfoList("varchar2"))
	assertEquals(cur.getField(0,"type_name"),"VARCHAR2")
	assertEquals(cur.getField(0,"data_type"),"12")
	assertEquals(cur.getField(0,"precision"),"32767")
	assertEquals(cur.getField(0,"local_type_name"),"VARCHAR2")
	assertTrue(cur.getTypeInfoList("date"))
	assertEquals(cur.getField(0,"type_name"),"DATE")
	assertEquals(cur.getField(0,"data_type"),"92")
	assertEquals(cur.getField(0,"precision"),"7")
	assertEquals(cur.getField(0,"local_type_name"),"DATE")
	output()


	# column list
	output("COLUMN LIST: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
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
	assertEquals(cur.getField(0,"column_name"),"TESTNUMBER")
	assertEquals(cur.getField(1,"column_name"),"TESTCHAR")
	assertEquals(cur.getField(2,"column_name"),"TESTVARCHAR")
	assertEquals(cur.getField(3,"column_name"),"TESTDATE")
	assertEquals(cur.getField(4,"column_name"),"TESTLONG")
	assertEquals(cur.getField(5,"column_name"),"TESTCLOB")
	assertEquals(cur.getField(6,"column_name"),"TESTBLOB")
	assertEquals(cur.getField(0,"data_type"),"NUMBER")
	assertEquals(cur.getField(1,"data_type"),"CHAR")
	assertEquals(cur.getField(2,"data_type"),"VARCHAR2")
	assertEquals(cur.getField(3,"data_type"),"DATE")
	assertEquals(cur.getField(4,"data_type"),"LONG")
	assertEquals(cur.getField(5,"data_type"),"CLOB")
	assertEquals(cur.getField(6,"data_type"),"BLOB")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# column list - auto_increment, primary key
	# oracle doesn't support auto_increment
	output("COLUMN LIST - auto_increment, primary key: ")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"))
	assertTrue(cur.getColumnList("testtable",None))
	assertEquals(cur.getField(0,"column_key"),"PRI")
	assertEquals(cur.getField(1,"column_key"),"")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# primary keys list
	output("PRIMARY KEYS LIST: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"))
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
	assertEquals(cur.getField(0,"table"),"TESTTABLE")
	assertEquals(cur.getField(0,"seq_in_index"),"1")
	assertEquals(cur.getField(0,"column_name"),"COL1")
	keyname=cur.getField(0,"key_name")
	assertStartsWith(keyname,"SYS_C")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# key and index list
	output("KEY AND INDEX LIST: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"))
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
	assertEquals(cur.getField(0,"table"),"TESTTABLE")
	assertEquals(cur.getField(0,"non_unique"),"0")
	assertEquals(cur.getField(0,"seq_in_index"),"1")
	assertEquals(cur.getField(0,"column_name"),"COL1")
	assertEquals(cur.getField(0,"collation"),"A")
	assertEquals(cur.getField(0,"index_type"),"3")
	keyname=cur.getField(0,"key_name")
	assertStartsWith(keyname,"SYS_C")
	assertTrue(cur.sendQuery("drop table testtable"))
	output()


	# procedure list
	output("PROCEDURE LIST: ")
	cur.sendQuery("drop procedure testproc1")
	cur.sendQuery("drop procedure testproc2")
	cur.sendQuery("drop procedure testproc3")
	cur.sendQuery("drop procedure testproc4")
	assertTrue(cur.sendQuery(
		"create procedure testproc1("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"))
	assertTrue(cur.sendQuery(
		"create procedure testproc2("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"))
	assertTrue(cur.sendQuery(
		"create procedure testproc3("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"))
	assertTrue(cur.sendQuery(
		"create procedure testproc4("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"))
	assertTrue(cur.getProcedureList(None))
	assertInResultSet(cur,"routine_name","TESTPROC1")
	assertInResultSet(cur,"routine_name","TESTPROC2")
	assertInResultSet(cur,"routine_name","TESTPROC3")
	assertInResultSet(cur,"routine_name","TESTPROC4")
	output()


	# procedure parameter list
	output("PROCEDURE PARAMETER LIST: ")
	assertTrue(cur.getProcedureParameterList("testproc1",None))
	assertEquals(cur.getColumnName(0),"parameter_name")
	assertEquals(cur.getColumnName(1),"parameter_mode")
	assertEquals(cur.getColumnName(2),"data_type")
	assertEquals(cur.getColumnName(3),"character_maximum_length")
	assertEquals(cur.getColumnName(4),"ordinal_position")
	assertEquals(cur.rowCount(),4)
	assertEquals(cur.getField(0,"parameter_name"),"IN1")
	assertEquals(cur.getField(0,"parameter_mode"),"1")
	assertEquals(cur.getField(0,"data_type"),"NUMBER")
	assertEquals(cur.getField(0,"ordinal_position"),"1")
	assertEquals(cur.getField(1,"parameter_name"),"IN2")
	assertEquals(cur.getField(1,"parameter_mode"),"1")
	assertEquals(cur.getField(1,"data_type"),"CHAR")
	assertEquals(cur.getField(1,"ordinal_position"),"2")
	assertEquals(cur.getField(2,"parameter_name"),"IN3")
	assertEquals(cur.getField(2,"parameter_mode"),"1")
	assertEquals(cur.getField(2,"data_type"),"VARCHAR2")
	assertEquals(cur.getField(2,"ordinal_position"),"3")
	assertEquals(cur.getField(3,"parameter_name"),"IN4")
	assertEquals(cur.getField(3,"parameter_mode"),"1")
	assertEquals(cur.getField(3,"data_type"),"DATE")
	assertEquals(cur.getField(3,"ordinal_position"),"4")
	assertTrue(cur.sendQuery("drop procedure testproc1"))
	assertTrue(cur.sendQuery("drop procedure testproc2"))
	assertTrue(cur.sendQuery("drop procedure testproc3"))
	assertTrue(cur.sendQuery("drop procedure testproc4"))
	output()


	# invalid queries
	output("INVALID QUERIES: ")
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"))
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
