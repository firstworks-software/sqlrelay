#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.

from SQLRelay import PySQLRClient
from decimal import *
import sys
from asserts import *
import string


if sys.version < '3':
    def btos(x):
        return x
else:
    import codecs
    def btos(x):
        return codecs.latin_1_encode(x)[0]

def main():

	PySQLRClient.getNumericFieldsAsNumbers()

	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,
						"/tmp/test.socket",
						"testuser","testpassword")
	cur=PySQLRClient.sqlrcursor(con)

	# get database type


	# identify
	print("IDENTIFY: ")
	assertEqual(con.identify(),"oracle")
	print()


	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()


	# isolation levels
	print("ISOLATION LEVELS: ")
	isolationlevels=["READ COMMITTED","SERIALIZABLE"]
	for il in isolationlevels:
		# oracle requires the isolation level to
		# be the first query of the transaction
		assertTrue(con.commit())
		# you can set the isolation level, but to get it, you have to
		# have permissions to read from sys.v_$session and
		# sys.v_$transaction
		assertTrue(con.setIsolationLevel(il))
		print()
	# reset to the default isolation level
	assertTrue(con.commit())
	assertTrue(con.setIsolationLevel(isolationlevels[0]))
	print()

	# drop existing table
	cur.sendQuery("drop table testtable")


	# create temptable
	print("CREATE TEMPTABLE: ")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"))
	print()


	# insert
	print("INSERT: ")
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
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4, "
		"	:var5, "
		"	:var6, "
		"	:var7)")
	assertEqual(cur.countBindVariables(),7)
	cur.inputBind("1",2)
	cur.inputBind("2","testchar2")
	cur.inputBind("3","testvarchar2")
	cur.inputBind("4","01-JAN-2002")
	cur.inputBind("5","testlong2")
	cur.inputBindClob("6","testclob2",9)
	cur.inputBindBlob("7","testblob2",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1",3)
	cur.inputBind("2","testchar3")
	cur.inputBind("3","testvarchar3")
	cur.inputBind("4","01-JAN-2003")
	cur.inputBind("5","testlong3")
	cur.inputBindClob("6","testclob3",9)
	cur.inputBindBlob("7","testblob3",9)
	assertTrue(cur.executeQuery())
	print()


	# array of binds by position
	print("ARRAY OF BINDS BY POSITION: ")
	cur.clearBinds()
	cur.inputBinds(["1","2","3","4","5"],
		[4,"testchar4","testvarchar4",
			"01-JAN-2004","testlong4"])
	cur.inputBindClob("6","testclob7",9)
	cur.inputBindBlob("7","testblob7",9)
	assertTrue(cur.executeQuery())
	print()


	# bind by name
	print("BIND BY NAME: ")
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
	cur.inputBind("var1",5)
	cur.inputBind("var2","testchar5")
	cur.inputBind("var3","testvarchar5")
	cur.inputBind("var4","01-JAN-2005")
	cur.inputBind("var5","testlong5")
	cur.inputBindClob("var6","testclob5",9)
	cur.inputBindBlob("var7","testblob5",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("var1",6)
	cur.inputBind("var2","testchar6")
	cur.inputBind("var3","testvarchar6")
	cur.inputBind("var4","01-JAN-2006")
	cur.inputBind("var5","testlong6")
	cur.inputBindClob("var6","testclob6",9)
	cur.inputBindBlob("var7","testblob6",9)
	assertTrue(cur.executeQuery())
	print()


	# array of binds by name
	print("ARRAY OF BINDS BY NAME: ")
	cur.clearBinds()
	cur.inputBinds(["var1","var2","var3","var4","var5"],
		[7,"testchar7","testvarchar7",
			"01-JAN-2007","testlong7"])
	cur.inputBindClob("var6","testclob7",9)
	cur.inputBindBlob("var7","testblob7",9)
	assertTrue(cur.executeQuery())
	print()


	# bind by name with validation
	print("BIND BY NAME WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("var1",8)
	cur.inputBind("var2","testchar8")
	cur.inputBind("var3","testvarchar8")
	cur.inputBind("var4","01-JAN-2008")
	cur.inputBind("var5","testlong8")
	cur.inputBindClob("var6","testclob8",9)
	cur.inputBindBlob("var7","testblob8",9)
	cur.inputBind("var9","junkvalue")
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	print()


	# output bind by name with validation
	print("OUTPUT BIND BY NAME WITH VALIDATION: ")
	cur.clearBinds()
	cur.defineOutputBindInteger("numvar")
	cur.defineOutputBindString("stringvar",10)
	cur.defineOutputBindDouble("floatvar")
	cur.defineOutputBindString("nullvar",10)
	cur.defineOutputBindString("dummyvar",10)
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	numvar=cur.getOutputBindInteger("numvar")
	stringvar=cur.getOutputBindString("stringvar")
	floatvar=cur.getOutputBindDouble("floatvar")
	assertEqual(numvar,1)
	assertEqual(stringvar,'hello')
	assertEqual(floatvar,2.5)
	assertEqual(
		cur.getOutputBindString("nullvar"),
		"")
	print()


	# select
	print("SELECT: ")
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
	print()


	# column count
	print("COLUMN COUNT: ")
	assertEqual(cur.colCount(),7)
	print()


	# column names
	print("COLUMN NAMES: ")
	assertEqual(cur.getColumnName(0),"TESTNUMBER")
	assertEqual(cur.getColumnName(1),"TESTCHAR")
	assertEqual(cur.getColumnName(2),"TESTVARCHAR")
	assertEqual(cur.getColumnName(3),"TESTDATE")
	assertEqual(cur.getColumnName(4),"TESTLONG")
	assertEqual(cur.getColumnName(5),"TESTCLOB")
	assertEqual(cur.getColumnName(6),"TESTBLOB")
	cols=cur.getColumnNames()
	assertEqual(cols[0],"TESTNUMBER")
	assertEqual(cols[1],"TESTCHAR")
	assertEqual(cols[2],"TESTVARCHAR")
	assertEqual(cols[3],"TESTDATE")
	assertEqual(cols[4],"TESTLONG")
	assertEqual(cols[5],"TESTCLOB")
	assertEqual(cols[6],"TESTBLOB")
	print()


	# column types
	print("COLUMN TYPES: ")
	assertEqual(cur.getColumnType(0),"NUMBER")
	assertEqual(cur.getColumnType('TESTNUMBER'),"NUMBER")
	assertEqual(cur.getColumnType(1),"CHAR")
	assertEqual(cur.getColumnType('TESTCHAR'),"CHAR")
	assertEqual(cur.getColumnType(2),"VARCHAR2")
	assertEqual(cur.getColumnType('TESTVARCHAR'),"VARCHAR2")
	assertEqual(cur.getColumnType(3),"DATE")
	assertEqual(cur.getColumnType('TESTDATE'),"DATE")
	assertEqual(cur.getColumnType(4),"LONG")
	assertEqual(cur.getColumnType('TESTLONG'),"LONG")
	assertEqual(cur.getColumnType(5),"CLOB")
	assertEqual(cur.getColumnType('TESTCLOB'),"CLOB")
	assertEqual(cur.getColumnType(6),"BLOB")
	assertEqual(cur.getColumnType('TESTBLOB'),"BLOB")
	print()


	# column length
	print("COLUMN LENGTH: ")
	assertEqual(cur.getColumnLength(0),22)
	assertEqual(cur.getColumnLength('TESTNUMBER'),22)
	assertEqual(cur.getColumnLength(1),40)
	assertEqual(cur.getColumnLength('TESTCHAR'),40)
	assertEqual(cur.getColumnLength(2),40)
	assertEqual(cur.getColumnLength('TESTVARCHAR'),40)
	assertEqual(cur.getColumnLength(3),7)
	assertEqual(cur.getColumnLength('TESTDATE'),7)
	assertEqual(cur.getColumnLength(4),0)
	assertEqual(cur.getColumnLength('TESTLONG'),0)
	assertEqual(cur.getColumnLength(5),0)
	assertEqual(cur.getColumnLength('TESTCLOB'),0)
	assertEqual(cur.getColumnLength(6),0)
	assertEqual(cur.getColumnLength('TESTBLOB'),0)
	print()


	# longest column
	print("LONGEST COLUMN: ")
	assertEqual(cur.getLongest(0),1)
	assertEqual(cur.getLongest('TESTNUMBER'),1)
	assertEqual(cur.getLongest(1),40)
	assertEqual(cur.getLongest('TESTCHAR'),40)
	assertEqual(cur.getLongest(2),12)
	assertEqual(cur.getLongest('TESTVARCHAR'),12)
	assertEqual(cur.getLongest(3),9)
	assertEqual(cur.getLongest('TESTDATE'),9)
	assertEqual(cur.getLongest(4),9)
	assertEqual(cur.getLongest('TESTLONG'),9)
	assertEqual(cur.getLongest(5),9)
	assertEqual(cur.getLongest('TESTCLOB'),9)
	assertEqual(cur.getLongest(6),9)
	assertEqual(cur.getLongest('TESTBLOB'),9)
	print()


	# row count
	print("ROW COUNT: ")
	assertEqual(cur.rowCount(),8)
	print()


	# total rows
	print("TOTAL ROWS: ")
	assertEqual(cur.totalRows(),0)
	print()


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
	assertEqual(cur.getField(0,1),"testchar1                               ")
	assertEqual(cur.getField(0,2),"testvarchar1")
	assertEqual(cur.getField(0,3),"01-JAN-01")
	assertEqual(cur.getField(0,4),"testlong1")
	assertEqual(cur.getField(0,5),"testclob1")
	assertEqual(cur.getField(0,6),btos(""))
	print()
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(7,1),"testchar8                               ")
	assertEqual(cur.getField(7,2),"testvarchar8")
	assertEqual(cur.getField(7,3),"01-JAN-08")
	assertEqual(cur.getField(7,4),"testlong8")
	assertEqual(cur.getField(7,5),"testclob8")
	assertEqual(cur.getField(7,6),btos("testblob8"))
	print()


	# field lengths by index
	print("FIELD LENGTHS BY INDEX: ")
	assertEqual(cur.getFieldLength(0,0),1)
	assertEqual(cur.getFieldLength(0,1),40)
	assertEqual(cur.getFieldLength(0,2),12)
	assertEqual(cur.getFieldLength(0,3),9)
	assertEqual(cur.getFieldLength(0,4),9)
	assertEqual(cur.getFieldLength(0,5),9)
	assertEqual(cur.getFieldLength(0,6),0)
	print()
	assertEqual(cur.getFieldLength(7,0),1)
	assertEqual(cur.getFieldLength(7,1),40)
	assertEqual(cur.getFieldLength(7,2),12)
	assertEqual(cur.getFieldLength(7,3),9)
	assertEqual(cur.getFieldLength(7,4),9)
	assertEqual(cur.getFieldLength(7,5),9)
	assertEqual(cur.getFieldLength(7,6),9)
	print()


	# fields by name
	print("FIELDS BY NAME: ")
	assertEqual(cur.getField(0,"TESTNUMBER"),1)
	assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                               ")
	assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1")
	assertEqual(cur.getField(0,"TESTDATE"),"01-JAN-01")
	assertEqual(cur.getField(0,"TESTLONG"),"testlong1")
	assertEqual(cur.getField(0,"TESTCLOB"),"testclob1")
	assertEqual(cur.getField(0,"TESTBLOB"),btos(""))
	print()
	assertEqual(cur.getField(7,"TESTNUMBER"),8)
	assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                               ")
	assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8")
	assertEqual(cur.getField(7,"TESTDATE"),"01-JAN-08")
	assertEqual(cur.getField(7,"TESTLONG"),"testlong8")
	assertEqual(cur.getField(7,"TESTCLOB"),"testclob8")
	assertEqual(cur.getField(7,"TESTBLOB"),btos("testblob8"))
	print()


	# field lengths by name
	print("FIELD LENGTHS BY NAME: ")
	assertEqual(cur.getFieldLength(0,"TESTNUMBER"),1)
	assertEqual(cur.getFieldLength(0,"TESTCHAR"),40)
	assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12)
	assertEqual(cur.getFieldLength(0,"TESTDATE"),9)
	assertEqual(cur.getFieldLength(0,"TESTLONG"),9)
	assertEqual(cur.getFieldLength(0,"TESTCLOB"),9)
	assertEqual(cur.getFieldLength(0,"TESTBLOB"),0)
	print()
	assertEqual(cur.getFieldLength(7,"TESTNUMBER"),1)
	assertEqual(cur.getFieldLength(7,"TESTCHAR"),40)
	assertEqual(cur.getFieldLength(7,"TESTVARCHAR"),12)
	assertEqual(cur.getFieldLength(7,"TESTDATE"),9)
	assertEqual(cur.getFieldLength(7,"TESTLONG"),9)
	assertEqual(cur.getFieldLength(7,"TESTCLOB"),9)
	assertEqual(cur.getFieldLength(7,"TESTBLOB"),9)
	print()


	# fields by array
	print("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEqual(fields[0],1)
	assertEqual(fields[1],"testchar1                               ")
	assertEqual(fields[2],"testvarchar1")
	assertEqual(fields[3],"01-JAN-01")
	assertEqual(fields[4],"testlong1")
	assertEqual(fields[5],"testclob1")
	assertEqual(fields[6],btos(""))
	print()


	# field lengths by array
	print("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEqual(fieldlens[0],1)
	assertEqual(fieldlens[1],40)
	assertEqual(fieldlens[2],12)
	assertEqual(fieldlens[3],9)
	assertEqual(fieldlens[4],9)
	assertEqual(fieldlens[5],9)
	assertEqual(fieldlens[6],None)
	print()


	# fields by dictionary
	print("FIELDS BY DICTIONARY: ")
	fields=cur.getRowDictionary(0)
	assertEqual(fields["TESTNUMBER"],1)
	assertEqual(fields["TESTCHAR"],"testchar1                               ")
	assertEqual(fields["TESTVARCHAR"],"testvarchar1")
	assertEqual(fields["TESTDATE"],"01-JAN-01")
	assertEqual(fields["TESTLONG"],"testlong1")
	assertEqual(fields["TESTCLOB"],"testclob1")
	assertEqual(fields["TESTBLOB"],btos(""))
	print()
	fields=cur.getRowDictionary(7)
	assertEqual(fields["TESTNUMBER"],8)
	assertEqual(fields["TESTCHAR"],"testchar8                               ")
	assertEqual(fields["TESTVARCHAR"],"testvarchar8")
	assertEqual(fields["TESTDATE"],"01-JAN-08")
	assertEqual(fields["TESTLONG"],"testlong8")
	assertEqual(fields["TESTCLOB"],"testclob8")
	assertEqual(fields["TESTBLOB"],btos("testblob8"))
	print()


	# field lengths by dictionary
	print("FIELD LENGTHS BY DICTIONARY: ")
	fieldlengths=cur.getRowLengthsDictionary(0)
	assertEqual(fieldlengths["TESTNUMBER"],1)
	assertEqual(fieldlengths["TESTCHAR"],40)
	assertEqual(fieldlengths["TESTVARCHAR"],12)
	assertEqual(fieldlengths["TESTDATE"],9)
	assertEqual(fieldlengths["TESTLONG"],9)
	assertEqual(fieldlengths["TESTCLOB"],9)
	assertEqual(fieldlengths["TESTBLOB"],0)
	print()
	fieldlengths=cur.getRowLengthsDictionary(7)
	assertEqual(fieldlengths["TESTNUMBER"],1)
	assertEqual(fieldlengths["TESTCHAR"],40)
	assertEqual(fieldlengths["TESTVARCHAR"],12)
	assertEqual(fieldlengths["TESTDATE"],9)
	assertEqual(fieldlengths["TESTLONG"],9)
	assertEqual(fieldlengths["TESTCLOB"],9)
	assertEqual(fieldlengths["TESTBLOB"],9)
	print()


	# individual substitutions
	print("INDIVIDUAL SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual")
	cur.substitution("var1",1)
	cur.substitution("var2","hello")
	cur.substitution("var3",10.5556,6,4)
	assertTrue(cur.executeQuery())
	print()


	# fields
	print("FIELDS: ")
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(0,1),"hello")
	# oracle makes this field an integer
	#assertEqual(cur.getField(0,2),Decimal("10.5556"))
	assertEqual(cur.getField(0,2),10)
	print()


	# array substitutions
	print("ARRAY SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual")
	cur.substitutions(["var1","var2","var3"],
				[1,"hello",10.5556],[0,0,6],[0,0,4])
	assertTrue(cur.executeQuery())
	print()


	# fields
	print("FIELDS: ")
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(0,1),"hello")
	# oracle makes this field an integer
	#assertEqual(cur.getField(0,2),Decimal("10.5556"))
	assertEqual(cur.getField(0,2),10)
	print()


	# nulls as nones
	print("NULLS as Nones: ")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery("select NULL,1,NULL from dual"))
	assertEqual(cur.getField(0,0),None)
	assertEqual(cur.getField(0,1),1)
	assertEqual(cur.getField(0,2),None)
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("select NULL,1,NULL from dual"))
	assertEqual(cur.getField(0,0),"")
	assertEqual(cur.getField(0,1),1)
	assertEqual(cur.getField(0,2),"")
	print()


	# result set buffer size
	print("RESULT SET BUFFER SIZE: ")
	assertEqual(cur.getResultSetBufferSize(),0)
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
	assertEqual(cur.getColumnName(0),None)
	assertEqual(cur.getColumnLength(0),0)
	assertEqual(cur.getColumnType(0),None)
	cur.getColumnInfo()
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
	assertEqual(cur.getColumnName(0),"TESTNUMBER")
	assertEqual(cur.getColumnLength(0),22)
	assertEqual(cur.getColumnType(0),"NUMBER")
	print()


	# suspended session
	print("SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
	filename=cur.getCacheFileName()
	assertEqual(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEqual(cur.getField(7,0),8)
	print()


	# column count for cached result set
	print("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEqual(cur.colCount(),7)
	print()


	# column names for cached result set
	print("COLUMN NAMES FOR CACHED RESULT SET: ")
	assertEqual(cur.getColumnName(0),"TESTNUMBER")
	assertEqual(cur.getColumnName(1),"TESTCHAR")
	assertEqual(cur.getColumnName(2),"TESTVARCHAR")
	assertEqual(cur.getColumnName(3),"TESTDATE")
	assertEqual(cur.getColumnName(4),"TESTLONG")
	assertEqual(cur.getColumnName(5),"TESTCLOB")
	assertEqual(cur.getColumnName(6),"TESTBLOB")
	cols=cur.getColumnNames()
	assertEqual(cols[0],"TESTNUMBER")
	assertEqual(cols[1],"TESTCHAR")
	assertEqual(cols[2],"TESTVARCHAR")
	assertEqual(cols[3],"TESTDATE")
	assertEqual(cols[4],"TESTLONG")
	assertEqual(cols[5],"TESTCLOB")
	assertEqual(cols[6],"TESTBLOB")
	print()


	# cached result set with result set buffer size
	print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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
	assertTrue(con.autoCommitOn())
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01-JAN-2010', "
		"	'testlong10', "
		"	'testclob10', "
		"	empty_blob())"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEqual(secondcur.getField(0,0),9)
	assertTrue(con.autoCommitOff())
	print()


	# row range
	print("ROW RANGE:")
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
	print()
	rows=cur.getRowRange(0,5)
	assertEqual(rows[0][0],1)
	assertEqual(rows[0][1],"testchar1                               ")
	assertEqual(rows[0][2],"testvarchar1")
	assertEqual(rows[0][3],"01-JAN-01")
	assertEqual(rows[0][4],"testlong1")
	print()
	assertEqual(rows[1][0],2)
	assertEqual(rows[1][1],"testchar2                               ")
	assertEqual(rows[1][2],"testvarchar2")
	assertEqual(rows[1][3],"01-JAN-02")
	assertEqual(rows[1][4],"testlong2")
	print()
	assertEqual(rows[2][0],3)
	assertEqual(rows[2][1],"testchar3                               ")
	assertEqual(rows[2][2],"testvarchar3")
	assertEqual(rows[2][3],"01-JAN-03")
	assertEqual(rows[2][4],"testlong3")
	print()
	assertEqual(rows[3][0],4)
	assertEqual(rows[3][1],"testchar4                               ")
	assertEqual(rows[3][2],"testvarchar4")
	assertEqual(rows[3][3],"01-JAN-04")
	assertEqual(rows[3][4],"testlong4")
	print()
	assertEqual(rows[4][0],5)
	assertEqual(rows[4][1],"testchar5                               ")
	assertEqual(rows[4][2],"testvarchar5")
	assertEqual(rows[4][3],"01-JAN-05")
	assertEqual(rows[4][4],"testlong5")
	print()
	assertEqual(rows[5][0],6)
	assertEqual(rows[5][1],"testchar6                               ")
	assertEqual(rows[5][2],"testvarchar6")
	assertEqual(rows[5][3],"01-JAN-06")
	assertEqual(rows[5][4],"testlong6")
	print()


	# output bind by position
	print("OUTPUT BIND BY POSITION: ")
	cur.prepareQuery(
		"begin "
		"	:numvar:=1; "
		"	:stringvar:='hello'; "
		"	:floatvar:=2.5; "
		"	:nullvar:=null; "
		"end;")
	cur.defineOutputBindInteger("1")
	cur.defineOutputBindString("2",10)
	cur.defineOutputBindDouble("3")
	cur.defineOutputBindString("4",10)
	assertTrue(cur.executeQuery())
	numvar=cur.getOutputBindInteger("1")
	stringvar=cur.getOutputBindString("2")
	floatvar=cur.getOutputBindDouble("3")
	assertEqual(numvar,1)
	assertEqual(stringvar,'hello')
	assertEqual(floatvar,2.5)
	assertEqual(
		cur.getOutputBindString("4"),"")
	print()


	# output bind by name
	print("OUTPUT BIND BY NAME: ")
	cur.clearBinds()
	cur.defineOutputBindInteger("numvar")
	cur.defineOutputBindString("stringvar",10)
	cur.defineOutputBindDouble("floatvar")
	cur.defineOutputBindString("nullvar",10)
	assertTrue(cur.executeQuery())
	numvar=cur.getOutputBindInteger("numvar")
	stringvar=cur.getOutputBindString("stringvar")
	floatvar=cur.getOutputBindDouble("floatvar")
	assertEqual(numvar,1)
	assertEqual(stringvar,'hello')
	assertEqual(floatvar,2.5)
	assertEqual(
		cur.getOutputBindString("nullvar"),
		"")
	print()


	# clob and blob output bind
	print("CLOB AND BLOB OUTPUT BIND: ")
	cur.sendQuery("drop table testtable1")
	assertTrue(cur.sendQuery(
		"create table testtable1 ("
		"	testclob clob, "
		"	testblob blob)"))
	cur.prepareQuery("insert into testtable1 values ('hello',:var1)")
	cur.inputBindBlob("var1","hello",5)
	assertTrue(cur.executeQuery())
	cur.prepareQuery(
		"begin "
		"	select testclob into :clobvar from testtable1; "
		"	select testblob into :blobvar from testtable1; "
		"end;")
	cur.defineOutputBindClob("clobvar")
	cur.defineOutputBindBlob("blobvar")
	assertTrue(cur.executeQuery())
	clobvar=cur.getOutputBindClob("clobvar")
	clobvarlength=cur.getOutputBindLength("clobvar")
	blobvar=cur.getOutputBindBlob("blobvar")
	blobvarlength=cur.getOutputBindLength("blobvar")
	assertEqual(clobvar,"hello")
	assertEqual(clobvarlength,5)
	assertEqual(blobvar,btos("hello"))
	assertEqual(blobvarlength,5)
	cur.sendQuery("drop table testtable1")
	print()


	# null and empty clobs and clobs
	print("NULL AND EMPTY CLOBS AND CLOBS: ")
	cur.getNullsAsNone()
	cur.sendQuery(
		"create table testtable1 ("
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)")
	cur.prepareQuery(
		"insert into "
		"	testtable1 "
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
	cur.sendQuery("select * from testtable1")
	assertEqual(cur.getField(0,0),None)
	assertEqual(cur.getField(0,1),None)
	assertEqual(cur.getField(0,2),None)
	assertEqual(cur.getField(0,3),None)
	cur.sendQuery("drop table testtable1")
	print()


	# cursor binds
	print("CURSOR BINDS: ")
	assertTrue(cur.sendQuery(
		"create or replace package types as "
		"	type cursorType is ref cursor; "
		"end;"))
	assertTrue(cur.sendQuery(
		"create or replace "
		"function sp_testtable "
		"return types.cursortype "
		"as "
		"	l_cursor    types.cursorType; "
		"begin "
		"	open l_cursor for "
		"	    select * from testtable; "
		"	return l_cursor; "
		"end;"))
	cur.prepareQuery("begin  :curs:=sp_testtable; end;")
	cur.defineOutputBindCursor("curs")
	assertTrue(cur.executeQuery())
	bindcur=cur.getOutputBindCursor("curs")
	assertTrue(bindcur.fetchFromBindCursor())
	assertEqual(bindcur.getField(0,0),1)
	assertEqual(bindcur.getField(1,0),2)
	assertEqual(bindcur.getField(2,0),3)
	assertEqual(bindcur.getField(3,0),4)
	assertEqual(bindcur.getField(4,0),5)
	assertEqual(bindcur.getField(5,0),6)
	assertEqual(bindcur.getField(6,0),7)
	assertEqual(bindcur.getField(7,0),8)
	print()


	# long clob
	print("LONG CLOB: ")
	cur.sendQuery("drop table testtable2")
	cur.sendQuery("create table testtable2 (testclob clob)")
	cur.prepareQuery("insert into testtable2 values (:clobval)")
	clobval=""
	for i in range(0,8*1024):
		clobval=clobval+'C'
	cur.inputBindClob("clobval",clobval,8*1024)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select testclob from testtable2")
	assertEqual(clobval,cur.getField(0,"TESTCLOB"))
	cur.prepareQuery(
		"begin "
		"	select testclob into :clobbindval from testtable2; "
		"end;")
	cur.defineOutputBindClob("clobbindval")
	assertTrue(cur.executeQuery())
	clobbindvar=cur.getOutputBindClob("clobbindval")
	assertEqual(cur.getOutputBindLength("clobbindval"),8*1024)
	assertEqual(clobval,clobbindvar)
	cur.sendQuery("drop table testtable2")
	print()


	# finished suspended session
	print("FINISHED SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable order by testnumber"))
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


	# bind validation
	print("BIND VALIDATION: ")
	cur.sendQuery("drop table testtable1")
	cur.sendQuery(
		"create table testtable1 ("
		"	col1 varchar2(20), "
		"	col2 varchar2(20), "
		"	col3 varchar2(20))")
	cur.prepareQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))")
	cur.inputBind("var1",1)
	cur.inputBind("var2",2)
	cur.inputBind("var3",3)
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
	cur.sendQuery("drop table testtable1")
	print()


	# drop existing table
	cur.sendQuery("drop table testtable")


	# invalid queries
	print("INVALID QUERIES: ")
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
