#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.

from SQLRelay import PySQLRClient
from decimal import *
import sys
from asserts import *
import string


def main():

	PySQLRClient.getNumericFieldsAsNumbers()

	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
								"db2inst1","testpassword")
	cur=PySQLRClient.sqlrcursor(con)

	# get database type


	# identify
	print("IDENTIFY: ")
	assertEqual(con.identify(),"db2")
	print()


	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()


	# isolation levels
	print("ISOLATION LEVELS: ")
	isolationlevels=["CS","UR","RS","RR"]
	for il in isolationlevels:
		assertTrue(con.setIsolationLevel(il))
		assertEqual(con.getIsolationLevel(),il)
		print()
	# reset to the default isolation level
	assertTrue(con.setIsolationLevel(isolationlevels[0]))
	print()

	# drop existing table
	cur.sendQuery("drop table testtable")


	# create temptable
	print("CREATE TEMPTABLE: ")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp)"))
	print()


	# insert
	print("INSERT: ")
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01/01/2001', "
		"	'01:00:00', "
		"	NULL)"))
	print()


	# bind by position
	print("BIND BY POSITION: ")
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL)")
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
	print()


	# array of binds by position
	print("ARRAY OF BINDS BY POSITION: ")
	cur.clearBinds()
	cur.inputBinds(["1","2","3","4","5","6","7","8","9","10"],
		[4,4,4,4.4,4.4,4.4,"testchar4","testvarchar4",
			"01/01/2004","04:00:00"],
		[0,0,0,4,4,4,0,0,0,0],
		[0,0,0,2,2,2,0,0,0,0])
	assertTrue(cur.executeQuery())
	print()


	# insert
	print("INSERT: ")
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	5, "
		"	5, "
		"	5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	'testchar5', "
		"	'testvarchar5', "
		"	'01/01/2005', "
		"	'05:00:00', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	6, "
		"	6, "
		"	6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	'testchar6', "
		"	'testvarchar6', "
		"	'01/01/2006', "
		"	'06:00:00', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	7, "
		"	7, "
		"	7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	'testchar7', "
		"	'testvarchar7', "
		"	'01/01/2007', "
		"	'07:00:00', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	8, "
		"	8, "
		"	8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	'testchar8', "
		"	'testvarchar8', "
		"	'01/01/2008', "
		"	'08:00:00', "
		"	NULL)"))
	print()


	# affected rows
	print("AFFECTED ROWS: ")
	assertEqual(cur.affectedRows(),1)
	print()


	# stored procedure
	print("STORED PROCEDURE: ")
	cur.sendQuery("drop procedure testproc");
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	in invar int, "
		"	out outvar int) "
		"language sql "
		"begin "
		"	set outvar = invar; "
		"end"))
	cur.prepareQuery("call testproc(?,?)")
	cur.inputBind("1",5)
	cur.defineOutputBindString("2",10)
	assertTrue(cur.executeQuery())
	assertEqual(cur.getOutputBindString("2"),"5")
	assertTrue(cur.sendQuery("drop procedure testproc"))
	print()


	# select
	print("SELECT: ")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	print()


	# column count
	print("COLUMN COUNT: ")
	assertEqual(cur.colCount(),11)
	print()


	# column names
	print("COLUMN NAMES: ")
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
	print()


	# column types
	print("COLUMN TYPES: ")
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
	print()


	# column length
	print("COLUMN LENGTH: ")
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
	print()


	# longest column
	print("LONGEST COLUMN: ")
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
	assertEqual(cur.getField(0,1),1)
	assertEqual(cur.getField(0,2),1)
	assertEqual(cur.getField(0,3),Decimal("1.10"))
	#assertEqual(cur.getField(0,4),Decimal("1.1"))
	#assertEqual(cur.getField(0,5),Decimal("1.1"))
	assertEqual(cur.getField(0,6),"testchar1                               ")
	assertEqual(cur.getField(0,7),"testvarchar1")
	assertEqual(cur.getField(0,8),"2001-01-01")
	assertEqual(cur.getField(0,9),"01:00:00")
	print()
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(7,1),8)
	assertEqual(cur.getField(7,2),8)
	assertEqual(cur.getField(7,3),Decimal("8.80"))
	#assertEqual(cur.getField(7,4),Decimal("8.8"))
	#assertEqual(cur.getField(7,5),Decimal("8.8"))
	assertEqual(cur.getField(7,6),"testchar8                               ")
	assertEqual(cur.getField(7,7),"testvarchar8")
	assertEqual(cur.getField(7,8),"2008-01-01")
	assertEqual(cur.getField(7,9),"08:00:00")
	print()


	# field lengths by index
	print("FIELD LENGTHS BY INDEX: ")
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
	print()
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
	print()


	# fields by name
	print("FIELDS BY NAME: ")
	assertEqual(cur.getField(0,"TESTSMALLINT"),1)
	assertEqual(cur.getField(0,"TESTINT"),1)
	assertEqual(cur.getField(0,"TESTBIGINT"),1)
	assertEqual(cur.getField(0,"TESTDECIMAL"),Decimal("1.10"))
	#assertEqual(cur.getField(0,"TESTREAL"),Decimal("1.1"))
	#assertEqual(cur.getField(0,"TESTDOUBLE"),Decimal("1.1"))
	assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                               ")
	assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1")
	assertEqual(cur.getField(0,"TESTDATE"),"2001-01-01")
	assertEqual(cur.getField(0,"TESTTIME"),"01:00:00")
	print()
	assertEqual(cur.getField(7,"TESTSMALLINT"),8)
	assertEqual(cur.getField(7,"TESTINT"),8)
	assertEqual(cur.getField(7,"TESTBIGINT"),8)
	assertEqual(cur.getField(7,"TESTDECIMAL"),Decimal("8.80"))
	#assertEqual(cur.getField(7,"TESTREAL"),Decimal("8.8"))
	#assertEqual(cur.getField(7,"TESTDOUBLE"),Decimal("8.8"))
	assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                               ")
	assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8")
	assertEqual(cur.getField(7,"TESTDATE"),"2008-01-01")
	assertEqual(cur.getField(7,"TESTTIME"),"08:00:00")
	print()


	# field lengths by name
	print("FIELD LENGTHS BY NAME: ")
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
	print()
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
	print()


	# fields by array
	print("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEqual(fields[0],1)
	assertEqual(fields[1],1)
	assertEqual(fields[2],1)
	assertEqual(fields[3],Decimal("1.1"))
	assertEqual(fields[4],Decimal("1.1"))
	assertEqual(fields[5],Decimal("1.1"))
	assertEqual(fields[6],"testchar1                               ")
	assertEqual(fields[7],"testvarchar1")
	assertEqual(fields[8],"2001-01-01")
	assertEqual(fields[9],"01:00:00")
	print()


	# field lengths by array
	print("FIELD LENGTHS BY ARRAY: ")
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
	print()


	# fields by dictionary
	print("FIELDS BY DICTIONARY: ")
	fields=cur.getRowDictionary(0)
	assertEqual(fields["TESTSMALLINT"],1)
	assertEqual(fields["TESTINT"],1)
	assertEqual(fields["TESTBIGINT"],1)
	assertEqual(fields["TESTDECIMAL"],Decimal("1.1"))
	#assertEqual(fields["TESTREAL"],Decimal("1.1"))
	#assertEqual(fields["TESTDOUBLE"],Decimal("1.1"))
	assertEqual(fields["TESTCHAR"],"testchar1                               ")
	assertEqual(fields["TESTVARCHAR"],"testvarchar1")
	assertEqual(fields["TESTDATE"],"2001-01-01")
	assertEqual(fields["TESTTIME"],"01:00:00")
	print()
	fields=cur.getRowDictionary(7)
	assertEqual(fields["TESTSMALLINT"],8)
	assertEqual(fields["TESTINT"],8)
	assertEqual(fields["TESTBIGINT"],8)
	assertEqual(fields["TESTDECIMAL"],Decimal("8.8"))
	#assertEqual(fields["TESTREAL"],Decimal("8.8"))
	#assertEqual(fields["TESTDOUBLE"],Decimal("8.8"))
	assertEqual(fields["TESTCHAR"],"testchar8                               ")
	assertEqual(fields["TESTVARCHAR"],"testvarchar8")
	assertEqual(fields["TESTDATE"],"2008-01-01")
	assertEqual(fields["TESTTIME"],"08:00:00")
	print()


	# field lengths by dictionary
	print("FIELD LENGTHS BY DICTIONARY: ")
	fieldlengths=cur.getRowLengthsDictionary(0)
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
	print()
	fieldlengths=cur.getRowLengthsDictionary(7)
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
	print()


	# individual substitutions
	print("INDIVIDUAL SUBSTITUTIONS: ")
	cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')")
	cur.substitution("var1",1)
	cur.substitution("var2","hello")
	cur.substitution("var3",10.5556,6,4)
	assertTrue(cur.executeQuery())
	print()


	# fields
	print("FIELDS: ")
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(0,1),"hello")
	assertEqual(cur.getField(0,2),"10.5556")
	print()


	# array substitutions
	print("ARRAY SUBSTITUTIONS: ")
	cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')")
	cur.substitutions(["var1","var2","var3"],
				[1,"hello",10.5556],[0,0,6],[0,0,4])
	assertTrue(cur.executeQuery())
	print()


	# fields
	print("FIELDS: ")
	assertEqual(cur.getField(0,0),1)
	assertEqual(cur.getField(0,1),"hello")
	assertEqual(cur.getField(0,2),"10.5556")
	print()


	# nulls as nones
	print("NULLS as Nones: ")
	cur.getNullsAsNone()
	cur.sendQuery("drop table testtable1")
	assertTrue(cur.sendQuery(
		"create table testtable1 ("
		"	col1 char(1), "
		"	col2 char(1), "
		"	col3 char(1))"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	'1', "
		"	NULL, "
		"	NULL)"))
	assertTrue(cur.sendQuery("select * from testtable1"))
	assertEqual(cur.getField(0,0),"1")
	assertEqual(cur.getField(0,1),None)
	assertEqual(cur.getField(0,2),None)
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("select * from testtable1"))
	assertEqual(cur.getField(0,0),"1")
	assertEqual(cur.getField(0,1),"")
	assertEqual(cur.getField(0,2),"")
	assertTrue(cur.sendQuery("drop table testtable1"))
	cur.getNullsAsNone()
	print()


	# result set buffer size
	print("RESULT SET BUFFER SIZE: ")
	assertEqual(cur.getResultSetBufferSize(),0)
	cur.setResultSetBufferSize(2)
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	assertEqual(cur.getColumnName(0),None)
	assertEqual(cur.getColumnLength(0),0)
	assertEqual(cur.getColumnType(0),None)
	cur.getColumnInfo()
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	assertEqual(cur.getColumnName(0),"TESTSMALLINT")
	assertEqual(cur.getColumnLength(0),2)
	assertEqual(cur.getColumnType(0),"SMALLINT")
	print()


	# suspended session
	print("SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	filename=cur.getCacheFileName()
	assertEqual(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEqual(cur.getField(7,0),8)
	print()


	# column count for cached result set
	print("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEqual(cur.colCount(),11)
	print()


	# column names for cached result set
	print("COLUMN NAMES FOR CACHED RESULT SET: ")
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
	print()


	# cached result set with result set buffer size
	print("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile1")
	cur.setCacheTtl(200)
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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


	# row range
	print("ROW RANGE:")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	print()
	rows=cur.getRowRange(0,5)
	assertEqual(rows[0][0],1);
	assertEqual(rows[0][1],1);
	assertEqual(rows[0][2],1);
	assertEqual(rows[0][3],Decimal("1.1"));
	assertEqual(rows[0][4],Decimal("1.1"));
	assertEqual(rows[0][5],Decimal("1.1"));
	assertEqual(rows[0][6],"testchar1                               ")
	assertEqual(rows[0][7],"testvarchar1")
	assertEqual(rows[0][8],"2001-01-01")
	assertEqual(rows[0][9],"01:00:00")
	print()
	assertEqual(rows[1][0],2);
	assertEqual(rows[1][1],2);
	assertEqual(rows[1][2],2);
	assertEqual(rows[1][3],Decimal("2.2"));
	assertEqual(rows[1][4],Decimal("2.2"));
	assertEqual(rows[1][5],Decimal("2.2"));
	assertEqual(rows[1][6],"testchar2                               ")
	assertEqual(rows[1][7],"testvarchar2")
	assertEqual(rows[1][8],"2002-01-01")
	assertEqual(rows[1][9],"02:00:00")
	print()
	assertEqual(rows[2][0],3);
	assertEqual(rows[2][1],3);
	assertEqual(rows[2][2],3);
	assertEqual(rows[2][3],Decimal("3.3"));
	assertEqual(rows[2][4],Decimal("3.3"));
	assertEqual(rows[2][5],Decimal("3.3"));
	assertEqual(rows[2][6],"testchar3                               ")
	assertEqual(rows[2][7],"testvarchar3")
	assertEqual(rows[2][8],"2003-01-01")
	assertEqual(rows[2][9],"03:00:00")
	print()
	assertEqual(rows[3][0],4);
	assertEqual(rows[3][1],4);
	assertEqual(rows[3][2],4);
	assertEqual(rows[3][3],Decimal("4.4"));
	assertEqual(rows[3][4],Decimal("4.4"));
	assertEqual(rows[3][5],Decimal("4.4"));
	assertEqual(rows[3][6],"testchar4                               ")
	assertEqual(rows[3][7],"testvarchar4")
	assertEqual(rows[3][8],"2004-01-01")
	assertEqual(rows[3][9],"04:00:00")
	print()
	assertEqual(rows[4][0],5);
	assertEqual(rows[4][1],5);
	assertEqual(rows[4][2],5);
	assertEqual(rows[4][3],Decimal("5.5"));
	assertEqual(rows[4][4],Decimal("5.5"));
	assertEqual(rows[4][5],Decimal("5.5"));
	assertEqual(rows[4][6],"testchar5                               ")
	assertEqual(rows[4][7],"testvarchar5")
	assertEqual(rows[4][8],"2005-01-01")
	assertEqual(rows[4][9],"05:00:00")
	print()
	assertEqual(rows[5][0],6);
	assertEqual(rows[5][1],6);
	assertEqual(rows[5][2],6);
	assertEqual(rows[5][3],Decimal("6.6"));
	assertEqual(rows[5][4],Decimal("6.6"));
	assertEqual(rows[5][5],Decimal("6.6"));
	assertEqual(rows[5][6],"testchar6                               ")
	assertEqual(rows[5][7],"testvarchar6")
	assertEqual(rows[5][8],"2006-01-01")
	assertEqual(rows[5][9],"06:00:00")
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
	con.commit()
	cur.sendQuery("drop table testtable")
	print()


	# invalid queries
	print("INVALID QUERIES: ")
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
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
