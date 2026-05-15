#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.


from SQLRelay import PySQLRClient
import sys
from socket import gethostname
import asserts
from asserts import *


def main():

	isolationlevels=["1","0","2","3"]
	subvars=["var1","var2","var3"]
	subvallongs=[1,2,3]
	subvalstrings=["hi","hello","bye"]
	subvaldoubles=[10.55,10.556,10.5556]
	precs=[4,5,6]
	scales=[2,3,4]

	LARGE_BUFFER_LENGTH=8192


	# hostname
	hostname=gethostname().split(".")[0]
	dumptran="dump tran "+hostname+" with truncate_only"


	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
	cur=PySQLRClient.sqlrcursor(con)
	asserts.setConnection(con)
	asserts.setCursor(cur)


	# identify
	print("IDENTIFY: ")
	assertEquals(con.identify(),"freetds")
	print()


	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()


	# transaction state
	print("TRANSACTION STATE: ")
	assertEquals(con.getDefaultTransactionModel(),"explicit-error")
	assertEquals(con.getTransactionModel(),"explicit-error")
	assertFalse(con.getInTransaction())
	assertTrue(con.getAutoCommit())
	print()


	# nextval format
	print("NEXTVAL FORMAT: ")
	assertEquals(con.nextvalFormat(),"%s.nextval")
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
	cur.sendQuery("drop table testtable")
	cur.sendQuery(dumptran)
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testsmallint smallint, "
		"	testtinyint tinyint, "
		"	testreal real, "
		"	testfloat float, "
		"	testdecimal decimal(4,1), "
		"	testnumeric numeric(4,1), "
		"	testmoney money, "
		"	testsmallmoney smallmoney, "
		"	testdatetime datetime, "
		"	testsmalldatetime smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit) lock datarows"))
	print()


	# insert
	print("INSERT: ")
	assertTrue(con.begin())
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
		"	1.1, "
		"	1.00, "
		"	1.00, "
		"	'01-Jan-2001 01:00:00', "
		"	'01-Jan-2001 01:00:00', "
		"	'testchar1', "
		"	'testvarchar1', "
		"	1)"))
	print()


	# affected rows
	print("AFFECTED ROWS: ")
	assertEquals(cur.affectedRows(),1)
	print()


	# input bind by position
	print("INPUT BIND BY POSITION: ")
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
		"	?, "
		"	?, "
		"	?, "
		"	?)")
	assertEquals(cur.countBindVariables(),14)
	cur.inputBind("1",2)
	cur.inputBind("2",2)
	cur.inputBind("3",2)
	cur.inputBind("4",2.2,2,1)
	cur.inputBind("5",2.2,2,1)
	cur.inputBind("6",2.2,2,1)
	cur.inputBind("7",2.2,2,1)
	cur.inputBind("8",2.00,3,2)
	cur.inputBind("9",2.00,3,2)
	cur.inputBind("10","01-Jan-2002 02:00:00")
	cur.inputBind("11","01-Jan-2002 02:00:00")
	cur.inputBind("12","testchar2")
	cur.inputBind("13","testvarchar2")
	cur.inputBind("14",1)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1",3)
	cur.inputBind("2",3)
	cur.inputBind("3",3)
	cur.inputBind("4",3.3,2,1)
	cur.inputBind("5",3.3,2,1)
	cur.inputBind("6",3.3,2,1)
	cur.inputBind("7",3.3,2,1)
	cur.inputBind("8",3.00,3,2)
	cur.inputBind("9",3.00,3,2)
	cur.inputBind("10","01-Jan-2003 03:00:00")
	cur.inputBind("11","01-Jan-2003 03:00:00")
	cur.inputBind("12","testchar3")
	cur.inputBind("13","testvarchar3")
	cur.inputBind("14",1)
	assertTrue(cur.executeQuery())
	print()


	# array of input binds by position
	# freetds doesn't support implicit conversion of string binds to other
	# data types, so arrays of binds don't generally work.
	# Omitting the test.


	# input bind by position with validation
	print("INPUT BIND BY POSITION WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("1",4)
	cur.inputBind("2",4)
	cur.inputBind("3",4)
	cur.inputBind("4",4.4,2,1)
	cur.inputBind("5",4.4,2,1)
	cur.inputBind("6",4.4,2,1)
	cur.inputBind("7",4.4,2,1)
	cur.inputBind("8",4.00,3,2)
	cur.inputBind("9",4.00,3,2)
	cur.inputBind("10","01-Jan-2004 04:00:00")
	cur.inputBind("11","01-Jan-2004 04:00:00")
	cur.inputBind("12","testchar4")
	cur.inputBind("13","testvarchar4")
	cur.inputBind("14",1)
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	print()


	# input bind by name
	print("INPUT BIND BY NAME: ")
	cur.clearBinds()
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	@var1, "
		"	@var2, "
		"	@var3, "
		"	@var4, "
		"	@var5, "
		"	@var6, "
		"	@var7, "
		"	@var8, "
		"	@var9, "
		"	@var10, "
		"	@var11, "
		"	@var12, "
		"	@var13, "
		"	@var14)")
	assertEquals(cur.countBindVariables(),14)
	cur.inputBind("var1",5)
	cur.inputBind("var2",5)
	cur.inputBind("var3",5)
	cur.inputBind("var4",5.5,2,1)
	cur.inputBind("var5",5.5,2,1)
	cur.inputBind("var6",5.5,2,1)
	cur.inputBind("var7",5.5,2,1)
	cur.inputBind("var8",5.00,3,2)
	cur.inputBind("var9",5.00,3,2)
	cur.inputBind("var10","01-Jan-2005 05:00:00")
	cur.inputBind("var11","01-Jan-2005 05:00:00")
	cur.inputBind("var12","testchar5")
	cur.inputBind("var13","testvarchar5")
	cur.inputBind("var14",1)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("var1",6)
	cur.inputBind("var2",6)
	cur.inputBind("var3",6)
	cur.inputBind("var4",6.6,2,1)
	cur.inputBind("var5",6.6,2,1)
	cur.inputBind("var6",6.6,2,1)
	cur.inputBind("var7",6.6,2,1)
	cur.inputBind("var8",6.00,3,2)
	cur.inputBind("var9",6.00,3,2)
	cur.inputBind("var10","01-Jan-2006 06:00:00")
	cur.inputBind("var11","01-Jan-2006 06:00:00")
	cur.inputBind("var12","testchar6")
	cur.inputBind("var13","testvarchar6")
	cur.inputBind("var14",1)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("var1",7)
	cur.inputBind("var2",7)
	cur.inputBind("var3",7)
	cur.inputBind("var4",7.7,2,1)
	cur.inputBind("var5",7.7,2,1)
	cur.inputBind("var6",7.7,2,1)
	cur.inputBind("var7",7.7,2,1)
	cur.inputBind("var8",7.00,3,2)
	cur.inputBind("var9",7.00,3,2)
	cur.inputBind("var10","01-Jan-2007 07:00:00")
	cur.inputBind("var11","01-Jan-2007 07:00:00")
	cur.inputBind("var12","testchar7")
	cur.inputBind("var13","testvarchar7")
	cur.inputBind("var14",1)
	assertTrue(cur.executeQuery())
	print()


	# array of input binds by name
	# freetds doesn't support implicit conversion of string binds to other
	# data types, so arrays of binds don't generally work.
	# Omitting the test.


	# input bind by name with validation
	print("INPUT BIND BY NAME WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("var1",8)
	cur.inputBind("var2",8)
	cur.inputBind("var3",8)
	cur.inputBind("var4",8.8,2,1)
	cur.inputBind("var5",8.8,2,1)
	cur.inputBind("var6",8.8,2,1)
	cur.inputBind("var7",8.8,2,1)
	cur.inputBind("var8",8.00,3,2)
	cur.inputBind("var9",8.00,3,2)
	cur.inputBind("var10","01-Jan-2008 08:00:00")
	cur.inputBind("var11","01-Jan-2008 08:00:00")
	cur.inputBind("var12","testchar8")
	cur.inputBind("var13","testvarchar8")
	cur.inputBind("var14",1)
	cur.inputBind("var15","junkvalue")
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	print()


	# select
	print("SELECT: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	print()


	# column count
	print("COLUMN COUNT: ")
	assertEquals(cur.colCount(),14)
	print()


	# column names
	print("COLUMN NAMES: ")
	assertEquals(cur.getColumnName(0),"testint")
	assertEquals(cur.getColumnName(1),"testsmallint")
	assertEquals(cur.getColumnName(2),"testtinyint")
	assertEquals(cur.getColumnName(3),"testreal")
	assertEquals(cur.getColumnName(4),"testfloat")
	assertEquals(cur.getColumnName(5),"testdecimal")
	assertEquals(cur.getColumnName(6),"testnumeric")
	assertEquals(cur.getColumnName(7),"testmoney")
	assertEquals(cur.getColumnName(8),"testsmallmoney")
	assertEquals(cur.getColumnName(9),"testdatetime")
	assertEquals(cur.getColumnName(10),"testsmalldatetime")
	assertEquals(cur.getColumnName(11),"testchar")
	assertEquals(cur.getColumnName(12),"testvarchar")
	assertEquals(cur.getColumnName(13),"testbit")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"testint")
	assertEquals(cols[1],"testsmallint")
	assertEquals(cols[2],"testtinyint")
	assertEquals(cols[3],"testreal")
	assertEquals(cols[4],"testfloat")
	assertEquals(cols[5],"testdecimal")
	assertEquals(cols[6],"testnumeric")
	assertEquals(cols[7],"testmoney")
	assertEquals(cols[8],"testsmallmoney")
	assertEquals(cols[9],"testdatetime")
	assertEquals(cols[10],"testsmalldatetime")
	assertEquals(cols[11],"testchar")
	assertEquals(cols[12],"testvarchar")
	assertEquals(cols[13],"testbit")
	print()


	# column types
	print("COLUMN TYPES: ")
	assertEquals(cur.getColumnType(0),"INT")
	assertEquals(cur.getColumnType("testint"),"INT")
	assertEquals(cur.getColumnType(1),"SMALLINT")
	assertEquals(cur.getColumnType("testsmallint"),"SMALLINT")
	assertEquals(cur.getColumnType(2),"TINYINT")
	assertEquals(cur.getColumnType("testtinyint"),"TINYINT")
	assertEquals(cur.getColumnType(3),"REAL")
	assertEquals(cur.getColumnType("testreal"),"REAL")
	assertEquals(cur.getColumnType(4),"FLOAT")
	assertEquals(cur.getColumnType("testfloat"),"FLOAT")
	assertEquals(cur.getColumnType(5),"DECIMAL")
	assertEquals(cur.getColumnType("testdecimal"),"DECIMAL")
	assertEquals(cur.getColumnType(6),"NUMERIC")
	assertEquals(cur.getColumnType("testnumeric"),"NUMERIC")
	assertEquals(cur.getColumnType(7),"MONEY")
	assertEquals(cur.getColumnType("testmoney"),"MONEY")
	assertEquals(cur.getColumnType(8),"SMALLMONEY")
	assertEquals(cur.getColumnType("testsmallmoney"),"SMALLMONEY")
	assertEquals(cur.getColumnType(9),"DATETIME")
	assertEquals(cur.getColumnType("testdatetime"),"DATETIME")
	assertEquals(cur.getColumnType(10),"SMALLDATETIME")
	assertEquals(cur.getColumnType("testsmalldatetime"),"SMALLDATETIME")
	assertEquals(cur.getColumnType(11),"CHAR")
	assertEquals(cur.getColumnType("testchar"),"CHAR")
	assertEquals(cur.getColumnType(12),"CHAR")
	assertEquals(cur.getColumnType("testvarchar"),"CHAR")
	assertEquals(cur.getColumnType(13),"BIT")
	assertEquals(cur.getColumnType("testbit"),"BIT")
	print()


	# column length
	print("COLUMN LENGTH: ")
	assertEquals(cur.getColumnLength(0),4)
	assertEquals(cur.getColumnLength("testint"),4)
	assertEquals(cur.getColumnLength(1),2)
	assertEquals(cur.getColumnLength("testsmallint"),2)
	assertEquals(cur.getColumnLength(2),1)
	assertEquals(cur.getColumnLength("testtinyint"),1)
	assertEquals(cur.getColumnLength(3),4)
	assertEquals(cur.getColumnLength("testreal"),4)
	assertEquals(cur.getColumnLength(4),8)
	assertEquals(cur.getColumnLength("testfloat"),8)
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getColumnLength(5),3)
	#assertEquals(cur.getColumnLength("testdecimal"),3)
	#assertEquals(cur.getColumnLength(6),3)
	#assertEquals(cur.getColumnLength("testnumeric"),3)
	assertEquals(cur.getColumnLength(7),8)
	assertEquals(cur.getColumnLength("testmoney"),8)
	assertEquals(cur.getColumnLength(8),4)
	assertEquals(cur.getColumnLength("testsmallmoney"),4)
	assertEquals(cur.getColumnLength(9),8)
	assertEquals(cur.getColumnLength("testdatetime"),8)
	assertEquals(cur.getColumnLength(10),4)
	assertEquals(cur.getColumnLength("testsmalldatetime"),4)
	# these seem to fluctuate too
	#assertEquals(cur.getColumnLength(11),40)
	#assertEquals(cur.getColumnLength("testchar"),40)
	#assertEquals(cur.getColumnLength(12),40)
	#assertEquals(cur.getColumnLength("testvarchar"),40)
	assertEquals(cur.getColumnLength(13),1)
	assertEquals(cur.getColumnLength("testbit"),1)
	print()


	# longest column
	print("LONGEST COLUMN: ")
	assertEquals(cur.getLongest(0),1)
	assertEquals(cur.getLongest("testint"),1)
	assertEquals(cur.getLongest(1),1)
	assertEquals(cur.getLongest("testsmallint"),1)
	assertEquals(cur.getLongest(2),1)
	assertEquals(cur.getLongest("testtinyint"),1)
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getLongest(3),3)
	#assertEquals(cur.getLongest("testreal"),3)
	#assertEquals(cur.getLongest(4),17)
	#assertEquals(cur.getLongest("testfloat"),17)
	#assertEquals(cur.getLongest(5),3)
	#assertEquals(cur.getLongest("testdecimal"),3)
	#assertEquals(cur.getLongest(6),3)
	#assertEquals(cur.getLongest("testnumeric"),3)
	#assertEquals(cur.getLongest(7),4)
	#assertEquals(cur.getLongest("testmoney"),4)
	#assertEquals(cur.getLongest(8),4)
	#assertEquals(cur.getLongest("testsmallmoney"),4)
	#assertEquals(cur.getLongest(9),26)
	#assertEquals(cur.getLongest("testdatetime"),26)
	#assertEquals(cur.getLongest(10),26)
	#assertEquals(cur.getLongest("testsmalldatetime"),26)
	assertEquals(cur.getLongest(11),40)
	assertEquals(cur.getLongest("testchar"),40)
	assertEquals(cur.getLongest(12),12)
	assertEquals(cur.getLongest("testvarchar"),12)
	assertEquals(cur.getLongest(13),1)
	assertEquals(cur.getLongest("testbit"),1)
	print()


	# row count
	print("ROW COUNT: ")
	assertEquals(cur.rowCount(),8)
	print()


	# total rows
	print("TOTAL ROWS: ")
	assertEquals(cur.totalRows(),0)
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
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(0,2),"1")
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getField(0,3),"1.1")
	#assertEquals(cur.getField(0,4),"1.1")
	assertEquals(cur.getField(0,5),"1.1")
	assertEquals(cur.getField(0,6),"1.1")
	#assertEquals(cur.getField(0,7),"1.00")
	#assertEquals(cur.getField(0,8),"1.00")
	#assertEquals(cur.getField(0,9),"Jan  1 2001 01:00:00:000AM")
	#assertEquals(cur.getField(0,10),"Jan  1 2001 01:00:00:000AM")
	assertEquals(cur.getField(0,11),"testchar1                               ")
	assertEquals(cur.getField(0,12),"testvarchar1")
	assertEquals(cur.getField(0,13),1)  # bit type returns int in Python
	print()
	assertEquals(cur.getField(7,0),"8")
	assertEquals(cur.getField(7,1),"8")
	assertEquals(cur.getField(7,2),"8")
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getField(7,3),"8.8")
	#assertEquals(cur.getField(7,4),"8.8")
	assertTrue(cur.getField(7,5).startswith("8.8"))
	assertTrue(cur.getField(7,6).startswith("8.8"))
	#assertEquals(cur.getField(7,7),"8.00")
	#assertEquals(cur.getField(7,8),"8.00")
	#assertEquals(cur.getField(7,9),"Jan  1 2008 08:00:00:000AM")
	#assertEquals(cur.getField(7,10),"Jan  1 2008 08:00:00:000AM")
	assertEquals(cur.getField(7,11),"testchar8                               ")
	assertEquals(cur.getField(7,12),"testvarchar8")
	assertEquals(cur.getField(7,13),1)  # bit type returns int in Python
	print()


	# field lengths by index
	print("FIELD LENGTHS BY INDEX: ")
	assertEquals(cur.getFieldLength(0,0),1)
	assertEquals(cur.getFieldLength(0,1),1)
	assertEquals(cur.getFieldLength(0,2),1)
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getFieldLength(0,3),3)
	#assertEquals(cur.getFieldLength(0,4),3)
	#assertEquals(cur.getFieldLength(0,5),3)
	#assertEquals(cur.getFieldLength(0,6),3)
	#assertEquals(cur.getFieldLength(0,7),4)
	#assertEquals(cur.getFieldLength(0,8),4)
	#assertEquals(cur.getFieldLength(0,9),26)
	#assertEquals(cur.getFieldLength(0,10),26)
	assertEquals(cur.getFieldLength(0,11),40)
	assertEquals(cur.getFieldLength(0,12),12)
	assertEquals(cur.getFieldLength(0,13),1)
	print()
	assertEquals(cur.getFieldLength(7,0),1)
	assertEquals(cur.getFieldLength(7,1),1)
	assertEquals(cur.getFieldLength(7,2),1)
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getFieldLength(7,3),3)
	#assertEquals(cur.getFieldLength(7,4),17)
	#assertEquals(cur.getFieldLength(7,5),3)
	#assertEquals(cur.getFieldLength(7,6),3)
	#assertEquals(cur.getFieldLength(7,7),4)
	#assertEquals(cur.getFieldLength(7,8),4)
	#assertEquals(cur.getFieldLength(7,9),26)
	#assertEquals(cur.getFieldLength(7,10),26)
	assertEquals(cur.getFieldLength(7,11),40)
	assertEquals(cur.getFieldLength(7,12),12)
	assertEquals(cur.getFieldLength(7,13),1)
	print()


	# fields by name
	print("FIELDS BY NAME: ")
	assertEquals(cur.getField(0,"testint"),"1")
	assertEquals(cur.getField(0,"testsmallint"),"1")
	assertEquals(cur.getField(0,"testtinyint"),"1")
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getField(0,"testreal"),"1.1")
	#assertEquals(cur.getField(0,"testfloat"),"1.1")
	assertEquals(cur.getField(0,"testdecimal"),"1.1")
	assertEquals(cur.getField(0,"testnumeric"),"1.1")
	#assertEquals(cur.getField(0,"testmoney"),"1.00")
	#assertEquals(cur.getField(0,"testsmallmoney"),"1.00")
	#assertEquals(cur.getField(0,"testdatetime"),"Jan  1 2001 01:00:00:000AM")
	#assertEquals(cur.getField(0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM")
	assertEquals(cur.getField(0,"testchar"),"testchar1                               ")
	assertEquals(cur.getField(0,"testvarchar"),"testvarchar1")
	assertEquals(cur.getField(0,"testbit"),1)  # bit type returns int in Python
	print()
	assertEquals(cur.getField(7,"testint"),"8")
	assertEquals(cur.getField(7,"testsmallint"),"8")
	assertEquals(cur.getField(7,"testtinyint"),"8")
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getField(7,"testreal"),"8.8")
	#assertEquals(cur.getField(7,"testfloat"),"8.8")
	assertTrue(cur.getField(7,"testdecimal").startswith("8.8"))
	assertTrue(cur.getField(7,"testnumeric").startswith("8.8"))
	#assertEquals(cur.getField(7,"testmoney"),"8.00")
	#assertEquals(cur.getField(7,"testsmallmoney"),"8.00")
	#assertEquals(cur.getField(7,"testdatetime"),"Jan  1 2008 08:00:00:000AM")
	#assertEquals(cur.getField(7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM")
	assertEquals(cur.getField(7,"testchar"),"testchar8                               ")
	assertEquals(cur.getField(7,"testvarchar"),"testvarchar8")
	assertEquals(cur.getField(7,"testbit"),1)  # bit type returns int in Python
	print()


	# field lengths by name
	print("FIELD LENGTHS BY NAME: ")
	assertEquals(cur.getFieldLength(0,"testint"),1)
	assertEquals(cur.getFieldLength(0,"testsmallint"),1)
	assertEquals(cur.getFieldLength(0,"testtinyint"),1)
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getFieldLength(0,"testreal"),3)
	#assertEquals(cur.getFieldLength(0,"testfloat"),3)
	#assertEquals(cur.getFieldLength(0,"testdecimal"),3)
	#assertEquals(cur.getFieldLength(0,"testnumeric"),3)
	#assertEquals(cur.getFieldLength(0,"testmoney"),4)
	#assertEquals(cur.getFieldLength(0,"testsmallmoney"),4)
	#assertEquals(cur.getFieldLength(0,"testdatetime"),26)
	#assertEquals(cur.getFieldLength(0,"testsmalldatetime"),26)
	assertEquals(cur.getFieldLength(0,"testchar"),40)
	assertEquals(cur.getFieldLength(0,"testvarchar"),12)
	assertEquals(cur.getFieldLength(0,"testbit"),1)
	print()
	assertEquals(cur.getFieldLength(7,"testint"),1)
	assertEquals(cur.getFieldLength(7,"testsmallint"),1)
	assertEquals(cur.getFieldLength(7,"testtinyint"),1)
	# these seem to fluctuate with every freetds release
	#assertEquals(cur.getFieldLength(7,"testreal"),3)
	#assertEquals(cur.getFieldLength(7,"testfloat"),17)
	#assertEquals(cur.getFieldLength(7,"testdecimal"),3)
	#assertEquals(cur.getFieldLength(7,"testnumeric"),3)
	#assertEquals(cur.getFieldLength(7,"testmoney"),4)
	#assertEquals(cur.getFieldLength(7,"testsmallmoney"),4)
	#assertEquals(cur.getFieldLength(7,"testdatetime"),26)
	#assertEquals(cur.getFieldLength(7,"testsmalldatetime"),26)
	assertEquals(cur.getFieldLength(7,"testchar"),40)
	assertEquals(cur.getFieldLength(7,"testvarchar"),12)
	assertEquals(cur.getFieldLength(7,"testbit"),1)
	print()


	# fields by array
	print("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEquals(fields[0],"1")
	assertEquals(fields[1],"1")
	assertEquals(fields[2],"1")
	# these seem to fluctuate with every freetds release
	#assertEquals(fields[3],"1.1")
	#assertEquals(fields[4],"1.1")
	assertEquals(fields[5],"1.1")
	assertEquals(fields[6],"1.1")
	#assertEquals(fields[7],"1.00")
	#assertEquals(fields[8],"1.00")
	#assertEquals(fields[9],"Jan  1 2001 01:00:00:000AM")
	#assertEquals(fields[10],"Jan  1 2001 01:00:00:000AM")
	assertEquals(fields[11],"testchar1                               ")
	assertEquals(fields[12],"testvarchar1")
	assertEquals(fields[13],1)  # bit type returns int in Python
	print()


	# field lengths by array
	print("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEquals(fieldlens[0],1)
	assertEquals(fieldlens[1],1)
	assertEquals(fieldlens[2],1)
	# these seem to fluctuate with every freetds release
	#assertEquals(fieldlens[3],3)
	#assertEquals(fieldlens[4],3)
	#assertEquals(fieldlens[5],3)
	#assertEquals(fieldlens[6],3)
	#assertEquals(fieldlens[7],4)
	#assertEquals(fieldlens[8],4)
	#assertEquals(fieldlens[9],26)
	#assertEquals(fieldlens[10],26)
	assertEquals(fieldlens[11],40)
	assertEquals(fieldlens[12],12)
	assertEquals(fieldlens[13],1)
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
	assertEquals(cur.getColumnLength(0),4)
	assertEquals(cur.getColumnType(0),"INT")
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
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(1,0),"2")
	assertEquals(cur.getField(2,0),"3")
	assertEquals(cur.getField(3,0),"4")
	assertEquals(cur.getField(4,0),"5")
	assertEquals(cur.getField(5,0),"6")
	assertEquals(cur.getField(6,0),"7")
	assertEquals(cur.getField(7,0),"8")
	print()
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
	assertEquals(cur.colCount(),14)
	print()


	# column names for cached result set
	print("COLUMN NAMES FOR CACHED RESULT SET: ")
	assertEquals(cur.getColumnName(0),"testint")
	assertEquals(cur.getColumnName(1),"testsmallint")
	assertEquals(cur.getColumnName(2),"testtinyint")
	assertEquals(cur.getColumnName(3),"testreal")
	assertEquals(cur.getColumnName(4),"testfloat")
	assertEquals(cur.getColumnName(5),"testdecimal")
	assertEquals(cur.getColumnName(6),"testnumeric")
	assertEquals(cur.getColumnName(7),"testmoney")
	assertEquals(cur.getColumnName(8),"testsmallmoney")
	assertEquals(cur.getColumnName(9),"testdatetime")
	assertEquals(cur.getColumnName(10),"testsmalldatetime")
	assertEquals(cur.getColumnName(11),"testchar")
	assertEquals(cur.getColumnName(12),"testvarchar")
	assertEquals(cur.getColumnName(13),"testbit")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"testint")
	assertEquals(cols[1],"testsmallint")
	assertEquals(cols[2],"testtinyint")
	assertEquals(cols[3],"testreal")
	assertEquals(cols[4],"testfloat")
	assertEquals(cols[5],"testdecimal")
	assertEquals(cols[6],"testnumeric")
	assertEquals(cols[7],"testmoney")
	assertEquals(cols[8],"testsmallmoney")
	assertEquals(cols[9],"testdatetime")
	assertEquals(cols[10],"testsmalldatetime")
	assertEquals(cols[11],"testchar")
	assertEquals(cols[12],"testvarchar")
	assertEquals(cols[13],"testbit")
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
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
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
	# can't do this with freetds
	#cur.setResultSetBufferSize(1)
	assertTrue(cur.sendQuery("select * from testtable"))
	i=0
	while True:
		row=cur.getRow(i)
		if not row:
			break
		secondcur=PySQLRClient.sqlrcursor(con)
		secondcur.setResultSetBufferSize(1)
		assertTrue(secondcur.sendQuery("select * from testtable"))
		secondcur.closeResultSet()
		i+=1
	#cur.setResultSetBufferSize(0)
	assertTrue(con.commit())
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# reset transaction state
	print("RESET TRANSACTION STATE: ")
	assertTrue(con.commit())
	assertEquals(con.getTransactionModel(),"explicit-error")
	assertTrue(con.getAutoCommit())
	print()


	# transaction behavior - implicit
	print("TRANSACTION BEHAVIOR - implicit: ")
	# sap ase rejects DDL inside a chained-mode (multi-statement) tx
	# unless `sp_dboption ... 'ddl in tran', true` is set on the db;
	# create the table while still in unchained mode, then switch.
	# `lock datarows` is needed so secondcur's count(*) scan doesn't
	# block on the writer's page lock from the in-flight insert
	assertTrue(cur.sendQuery(
		"create table testtable (col1 integer) lock datarows"))
	assertTrue(con.setTransactionModel("implicit"))
	assertEquals(con.getTransactionModel(),"implicit")
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
	# switch back to unchained mode so the drop isn't rejected
	assertTrue(con.setTransactionModel("explicit-error"))
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# transaction behavior - explicit
	print("TRANSACTION BEHAVIOR - explicit: ")
	assertTrue(con.setTransactionModel("explicit"))
	assertEquals(con.getTransactionModel(),"explicit")
	assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"))
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
	# switch back to unchained mode so the drop isn't rejected
	assertTrue(con.setTransactionModel("explicit-error"))
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# transaction behavior - explicit-deferred
	print("TRANSACTION BEHAVIOR - explicit-deferred: ")
	assertTrue(con.setTransactionModel("explicit-deferred"))
	assertEquals(con.getTransactionModel(),"explicit-deferred")
	# switch to autocommit-on so the begin/commit cycles below
	# bracket explicit transactions (autocommit-off semantics are
	# exercised at the end of this block)
	assertTrue(con.autoCommitOn())
	assertTrue(con.getAutoCommit())
	assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"))
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
	# switch back to unchained mode so the drop isn't rejected
	assertTrue(con.setTransactionModel("explicit-error"))
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# transaction behavior - explicit-error
	print("TRANSACTION BEHAVIOR - explicit-error: ")
	assertTrue(con.setTransactionModel("explicit-error"))
	assertEquals(con.getTransactionModel(),"explicit-error")
	assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"))
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
	# commit the open tx so the drop isn't rejected as DDL inside a
	# chained-mode transaction (in explicit-error model, autoCommitOn
	# from inside a tx errors out by design, so commit is the route
	# back to autocommit-on / unchained mode)
	assertTrue(con.commit())
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# transaction behavior - none
	print("TRANSACTION BEHAVIOR - none: ")
	assertTrue(con.setTransactionModel("none"))
	assertEquals(con.getTransactionModel(),"none")
	assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"))
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
	print()


	# reset transaction behavior
	print("RESET TRANSACTION BEHAVIOR: ")
	assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()))
	assertEquals(con.getTransactionModel(),"explicit-error")
	assertTrue(con.getAutoCommit())
	print()


	# individual substitutions
	print("INDIVIDUAL SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),'$(var2)',$(var3)")
	cur.substitution("var1",1)
	cur.substitution("var2","hello")
	cur.substitution("var3",10.5556,6,4)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"10.5556")
	print()


	# array substitutions
	print("ARRAY SUBSTITUTIONS: ")
	cur.prepareQuery("select $(var1),$(var2),$(var3)")
	cur.substitutions(subvars,subvallongs)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"2")
	assertEquals(cur.getField(0,2),"3")
	print()
	cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'")
	cur.substitutions(subvars,subvalstrings)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"hi")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"bye")
	print()
	cur.prepareQuery("select $(var1),$(var2),$(var3)")
	cur.substitutions(subvars,subvaldoubles,precs,scales)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"10.55")
	assertEquals(cur.getField(0,1),"10.556")
	assertEquals(cur.getField(0,2),"10.5556")
	print()


	# nulls as nulls
	print("NULLS AS NULLS: ")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery("select NULL,1,NULL"))
	assertNone(cur.getField(0,0))
	assertEquals(cur.getField(0,1),"1")
	assertNone(cur.getField(0,2))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("select NULL,1,NULL"))
	assertEquals(cur.getField(0,0),"")
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(0,2),"")
	print()



	# null and empty lobs
	print("NULL AND EMPTY LOBS: ")
	cur.getNullsAsNone()
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testclob1 text NULL, "
		"	testclob2 text NULL, "
		"	testblob1 image NULL, "
		"	testblob2 image NULL)"))
	cur.prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	?, "
		"	?, "
		"	?, "
		"	?)")
	cur.inputBindClob("1","",0)
	cur.inputBindClob("2",None,0)
	cur.inputBindBlob("3","",0)
	cur.inputBindBlob("4",None,0)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select * from testtable")
	# sap converts empty strings to a single space.  It's possible that
	# if we had true input bind support on the backend, then this would
	# work correctly, but for now we're faking binds, and inserting an
	# empty string, so we have to check for a single space here.
	assertEquals(cur.getField(0,0)," ")
	assertNone(cur.getField(0,1))
	# sap doesn't really support inserting an empty string into a binary
	# column.  The minimum that can be inserted is a single \0.  In c++
	# this compares equal to "" because both are zero-length null-terminated
	# strings, but in Python bytes are length-explicit so we have to check
	# for the single null byte.
	assertEquals(cur.getField(0,2),b"\x00")
	assertNone(cur.getField(0,3))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# long lobs
	print("LONG LOBS: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery(
		"create table testtable ("
		"	testclob text, "
		"	testblob image) lock datarows")
	cur.prepareQuery("insert into testtable values (?,?)")
	largebuffer='C'*LARGE_BUFFER_LENGTH
	cur.inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH)
	cur.inputBindBlob("2",largebuffer,LARGE_BUFFER_LENGTH)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select * from testtable")
	assertEquals(cur.getFieldLength(0,"testclob"),LARGE_BUFFER_LENGTH)
	assertEquals(cur.getField(0,"testclob"),largebuffer)
	assertEquals(cur.getFieldLength(0,"testblob"),LARGE_BUFFER_LENGTH)
	# image column comes back as bytes in Python
	assertEqualsBytes(cur.getField(0,"testblob"),largebuffer.encode(),
						LARGE_BUFFER_LENGTH)
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# output bind by position
	# FreeTDS needs to support cursors for this to work


	# output bind by name
	# FreeTDS needs to support cursors for this to work


	# output bind by name with validation
	# Even if FreeTDS supported cursors...
	# validateBinds() can't be used for output binds, with sap.  In sap,
	# when executing a procedure, you don't declare any bind variable
	# delimiters in the query.  eg, you just do: "exec testproc", not
	# "exec testproc(@out1,@out2)".  If you call validateBinds(), it won't
	# find any binds in the query, and will filter out any binds that you
	# declare.


	# lob output bind
	# sap doesn't support lobs as output parameters to stored procedures,
	# and there's no way to directly select into a lob bind variable


	# long output bind
	# FreeTDS needs to support cursors for this to work


	# negative input bind
	print("NEGATIVE INPUT BIND: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery("create table testtable (testval int)")
	cur.prepareQuery("insert into testtable values (@testval)")
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
	cur.substitution("var1","@var1")
	assertTrue(cur.validBind("var1"))
	assertFalse(cur.validBind("var2"))
	assertFalse(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	print()
	cur.substitution("var2","@var2")
	assertTrue(cur.validBind("var1"))
	assertTrue(cur.validBind("var2"))
	assertFalse(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	print()
	cur.substitution("var3","@var3")
	assertTrue(cur.validBind("var1"))
	assertTrue(cur.validBind("var2"))
	assertTrue(cur.validBind("var3"))
	assertFalse(cur.validBind("var4"))
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# rebinding
	# FreeTDS needs to support cursors for this to work


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
	cur.prepareQuery("select cast(? as int)")
	cur.inputBind("1",1)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	print()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	print()
	cur.inputBind("1",2)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"2")
	print()


	# stored procedure returning no value
	# FreeTDS needs to support cursors for this to work


	# stored procedure returning single value
	# FreeTDS needs to support cursors for this to work


	# stored procedure returning multiple values
	# FreeTDS needs to support cursors for this to work


	# stored procedure returning result set
	print("STORED PROCEDURE RETURNING RESULT SET: ")
	cur.sendQuery("drop procedure testselectproc")
	assertTrue(cur.sendQuery(
		"create procedure testselectproc as "
		"	select 1 "
		"	union "
		"	select 2 "
		"	union "
		"	select 3 "
		"	union "
		"	select 4 "
		"	union "
		"	select 5 "
		"	union "
		"	select 6 "
		"	union "
		"	select 7 "
		"	union "
		"	select 8"))
	assertTrue(cur.sendQuery("exec testselectproc"))
	assertEquals(cur.rowCount(),8)
	assertTrue(cur.sendQuery("drop procedure testselectproc"))
	print()


	# temporary tables
	print("TEMPORARY TABLES: ")
	cur.sendQuery("drop table #temptable")
	cur.sendQuery("create table #temptable (col1 int)")
	assertTrue(cur.sendQuery("insert into #temptable values (1)"))
	assertTrue(cur.sendQuery("select count(*) from #temptable"))
	assertEquals(cur.getField(0,0),"1")
	con.endSession()
	print()
	assertFalse(cur.sendQuery("select count(*) from #temptable"))
	print()


	# encoded binary data
	print("ENCODED BINARY DATA: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery("create table testtable (col1 image)"))
	buffer=bytes(range(256))
	hex=buffer.hex()
	querystr="insert into testtable values (0x"+hex+")"
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
			"	(col1 int identity primary key, "
			"	col2 int)"))
	assertTrue(cur.sendQuery(
			"insert into testtable (col2) values (1)"))
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
	assertTrue(cur.rowCount()>0)
	print()


	# schema list
	print("SCHEMA LIST: ")
	cur.sendQuery("drop table testtable")
	# the get schema list query that is used with sap will only return the
	# names of schemas that have at least one database object in them, so
	# to be sure that there is one, we'll create a table
	assertTrue(cur.sendQuery("create table testtable (col1 int)"))
	assertTrue(cur.getSchemaList(None))
	assertEquals(cur.getColumnName(0),"Database")
	assertTrue(cur.rowCount()>0)
	assertTrue(cur.sendQuery("drop table testtable"))
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
	cur.sendQuery("drop table testtable1")
	cur.sendQuery("drop table testtable2")
	cur.sendQuery("drop table testtable3")
	cur.sendQuery("drop table testtable4")
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
	assertTrue(cur.sendQuery("drop table testtable1"))
	assertTrue(cur.sendQuery("drop table testtable2"))
	assertTrue(cur.sendQuery("drop table testtable3"))
	assertTrue(cur.sendQuery("drop table testtable4"))
	print()


	# type info list
	print("TYPE INFO LIST: ")
	assertTrue(cur.getTypeInfoList("int"))
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
	assertEquals(cur.getField(0,"type_name"),"INT")
	assertEquals(cur.getField(0,"data_type"),"4")
	assertEquals(cur.getField(0,"precision"),"10")
	assertEquals(cur.getField(0,"local_type_name"),"INT")
	assertTrue(cur.getTypeInfoList("char"))
	assertEquals(cur.getField(0,"type_name"),"CHAR")
	assertEquals(cur.getField(0,"data_type"),"1")
	assertEquals(cur.getField(0,"precision"),"8000")
	assertEquals(cur.getField(0,"local_type_name"),"CHAR")
	assertTrue(cur.getTypeInfoList("varchar"))
	assertEquals(cur.getField(0,"type_name"),"VARCHAR")
	assertEquals(cur.getField(0,"data_type"),"12")
	assertEquals(cur.getField(0,"precision"),"8000")
	assertEquals(cur.getField(0,"local_type_name"),"VARCHAR")
	assertTrue(cur.getTypeInfoList("datetime"))
	assertEquals(cur.getField(0,"type_name"),"DATETIME")
	assertEquals(cur.getField(0,"data_type"),"93")
	assertEquals(cur.getField(0,"precision"),"23")
	assertEquals(cur.getField(0,"local_type_name"),"DATETIME")
	print()


	# column list
	print("COLUMN LIST: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testint int, "
		"	testsmallint smallint, "
		"	testtinyint tinyint, "
		"	testreal real, "
		"	testfloat float, "
		"	testdecimal decimal(4,1), "
		"	testnumeric numeric(4,1), "
		"	testmoney money, "
		"	testsmallmoney smallmoney, "
		"	testdatetime datetime, "
		"	testsmalldatetime smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit)"))
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
	assertEquals(cur.getField(1,"column_name"),"testsmallint")
	assertEquals(cur.getField(2,"column_name"),"testtinyint")
	assertEquals(cur.getField(3,"column_name"),"testreal")
	assertEquals(cur.getField(4,"column_name"),"testfloat")
	assertEquals(cur.getField(5,"column_name"),"testdecimal")
	assertEquals(cur.getField(6,"column_name"),"testnumeric")
	assertEquals(cur.getField(7,"column_name"),"testmoney")
	assertEquals(cur.getField(8,"column_name"),"testsmallmoney")
	assertEquals(cur.getField(9,"column_name"),"testdatetime")
	assertEquals(cur.getField(10,"column_name"),"testsmalldatetime")
	assertEquals(cur.getField(11,"column_name"),"testchar")
	assertEquals(cur.getField(12,"column_name"),"testvarchar")
	assertEquals(cur.getField(13,"column_name"),"testbit")
	assertEquals(cur.getField(0,"data_type"),"int")
	assertEquals(cur.getField(1,"data_type"),"smallint")
	assertEquals(cur.getField(2,"data_type"),"tinyint")
	assertEquals(cur.getField(3,"data_type"),"real")
	assertEquals(cur.getField(4,"data_type"),"float")
	assertEquals(cur.getField(5,"data_type"),"decimal")
	assertEquals(cur.getField(6,"data_type"),"numeric")
	assertEquals(cur.getField(7,"data_type"),"money")
	assertEquals(cur.getField(8,"data_type"),"smallmoney")
	assertEquals(cur.getField(9,"data_type"),"datetime")
	assertEquals(cur.getField(10,"data_type"),"smalldatetime")
	assertEquals(cur.getField(11,"data_type"),"char")
	assertEquals(cur.getField(12,"data_type"),"varchar")
	assertEquals(cur.getField(13,"data_type"),"bit")
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# column list - auto_increment, primary key
	print("COLUMN LIST - auto_increment, primary key: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int identity primary key, "
		"	col2 int)"))
	assertTrue(cur.getColumnList("testtable",None))
	extra0=cur.getField(0,"extra")
	colkey0=cur.getField(0,"column_key")
	extra1=cur.getField(1,"extra")
	colkey1=cur.getField(1,"column_key")
	assertTrue(extra0 is not None and "auto_increment" in extra0)
	assertTrue(colkey0 is not None and "PRI" in colkey0)
	assertFalse(extra1 is not None and "auto_increment" in extra1)
	assertFalse(colkey1 is not None and "PRI" in colkey1)
	print()
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"))
	assertTrue(cur.getColumnList("testtable",None))
	extra0=cur.getField(0,"extra")
	colkey0=cur.getField(0,"column_key")
	assertFalse(extra0 is not None and "auto_increment" in extra0)
	assertTrue(colkey0 is not None and "PRI" in colkey0)
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# primary keys list
	print("PRIMARY KEYS LIST: ")
	cur.sendQuery("drop table testtable")
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
	keyname=cur.getField(0,"key_name")
	assertTrue(keyname)
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# key and index list
	print("KEY AND INDEX LIST: ")
	cur.sendQuery("drop table testtable")
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
	assertEquals(cur.getField(0,"index_type"),"1")
	keyname=cur.getField(0,"key_name")
	assertTrue(keyname)
	assertTrue(cur.sendQuery("drop table testtable"))
	print()


	# procedure list
	print("PROCEDURE LIST: ")
	cur.sendQuery("drop procedure testproc1")
	cur.sendQuery("drop procedure testproc2")
	cur.sendQuery("drop procedure testproc3")
	cur.sendQuery("drop procedure testproc4")
	assertTrue(cur.sendQuery(
		"create procedure testproc1 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"))
	assertTrue(cur.sendQuery(
		"create procedure testproc2 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"))
	assertTrue(cur.sendQuery(
		"create procedure testproc3 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"))
	assertTrue(cur.sendQuery(
		"create procedure testproc4 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"))
	assertTrue(cur.getProcedureList(None))
	counter=0
	for i in range(cur.rowCount()):
		name=cur.getField(i,"routine_name")
		if name in ("testproc1","testproc2","testproc3","testproc4"):
			counter+=1
	assertEquals(counter,4)
	print()


	# procedure parameter list
	print("PROCEDURE PARAMETER LIST: ")
	assertTrue(cur.getProcedureParameterList("testproc1",None))
	assertEquals(cur.getColumnName(0),"parameter_name")
	assertEquals(cur.getColumnName(1),"parameter_mode")
	assertEquals(cur.getColumnName(2),"data_type")
	assertEquals(cur.getColumnName(3),"character_maximum_length")
	assertEquals(cur.getColumnName(4),"ordinal_position")
	assertEquals(cur.rowCount(),4)
	assertEquals(cur.getField(0,"parameter_name"),"@in1")
	assertEquals(cur.getField(0,"parameter_mode"),"1")
	assertEquals(cur.getField(0,"data_type"),"int")
	assertEquals(cur.getField(0,"ordinal_position"),"1")
	assertEquals(cur.getField(1,"parameter_name"),"@in2")
	assertEquals(cur.getField(1,"parameter_mode"),"1")
	assertEquals(cur.getField(1,"data_type"),"char")
	assertEquals(cur.getField(1,"ordinal_position"),"2")
	assertEquals(cur.getField(2,"parameter_name"),"@in3")
	assertEquals(cur.getField(2,"parameter_mode"),"1")
	assertEquals(cur.getField(2,"data_type"),"varchar")
	assertEquals(cur.getField(2,"ordinal_position"),"3")
	assertEquals(cur.getField(3,"parameter_name"),"@in4")
	assertEquals(cur.getField(3,"parameter_mode"),"1")
	assertEquals(cur.getField(3,"data_type"),"datetime")
	assertEquals(cur.getField(3,"ordinal_position"),"4")
	assertTrue(cur.sendQuery("drop procedure testproc1"))
	assertTrue(cur.sendQuery("drop procedure testproc2"))
	assertTrue(cur.sendQuery("drop procedure testproc3"))
	assertTrue(cur.sendQuery("drop procedure testproc4"))
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


	reportTestStatus()
	sys.exit(asserts.status)


main()
