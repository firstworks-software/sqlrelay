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
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,
						"/tmp/test.socket",
						"testuser","testpassword")
	cur=PySQLRClient.sqlrcursor(con)

	# get database type


	# identify
	print("IDENTIFY: ")
	assertEqual(con.identify(),"sap")
	print()


	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()


	# isolation levels
	print("ISOLATION LEVELS: ")
	isolationlevels=["1","0","2","3"]
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
	print()


	# begin transaction
	print("BEGIN TRANSACTION: ")
	#assertTrue(cur.sendQuery("begin tran"))
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
	assertEqual(cur.affectedRows(),1)
	print()


	# bind by position
	print("BIND BY POSITION: ")
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
	assertEqual(cur.countBindVariables(),14)
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
	cur.clearBinds();
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


	# array of binds by position
	print("ARRAY OF BINDS BY POSITION: ")
	cur.clearBinds();
	cur.inputBinds(["1","2","3","4","5","6",
			"7","8","9","10","11","12",
			"13","14"],
		[4,4,4,4.4,4.4,4.4,4.4,4.00,4.00,
			"01-Jan-2004 04:00:00",
			"01-Jan-2004 04:00:00",
			"testchar4","testvarchar4",1],
		[0,0,0,2,2,2,2,3,3,0,0,0,0,0],
		[0,0,0,1,1,1,1,2,2,0,0,0,0,0])
	assertTrue(cur.executeQuery())
	print()


	# bind by name
	print("BIND BY NAME: ")
	cur.clearBinds();
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
	cur.clearBinds();
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
	print()


	# array of binds by name
	print("ARRAY OF BINDS BY NAME: ")
	cur.clearBinds();
	cur.inputBinds(["var1","var2","var3","var4","var5","var6",
			"var7","var8","var9","var10","var11","var12",
			"var13","var14"],
		[7,7,7,7.7,7.7,7.7,7.7,7.00,7.00,
			"01-Jan-2007 07:00:00",
			"01-Jan-2007 07:00:00",
			"testchar7","testvarchar7",1],
		[0,0,0,2,2,2,2,3,3,0,0,0,0,0],
		[0,0,0,1,1,1,1,2,2,0,0,0,0,0])
	assertTrue(cur.executeQuery())
	print()


	# bind by name with validation
	print("BIND BY NAME WITH VALIDATION: ")
	cur.clearBinds();
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
	assertEqual(cur.colCount(),14)
	print()


	# column names
	print("COLUMN NAMES: ")
	assertEqual(cur.getColumnName(0),"testint")
	assertEqual(cur.getColumnName(1),"testsmallint")
	assertEqual(cur.getColumnName(2),"testtinyint")
	assertEqual(cur.getColumnName(3),"testreal")
	assertEqual(cur.getColumnName(4),"testfloat")
	assertEqual(cur.getColumnName(5),"testdecimal")
	assertEqual(cur.getColumnName(6),"testnumeric")
	assertEqual(cur.getColumnName(7),"testmoney")
	assertEqual(cur.getColumnName(8),"testsmallmoney")
	assertEqual(cur.getColumnName(9),"testdatetime")
	assertEqual(cur.getColumnName(10),"testsmalldatetime")
	assertEqual(cur.getColumnName(11),"testchar")
	assertEqual(cur.getColumnName(12),"testvarchar")
	assertEqual(cur.getColumnName(13),"testbit")
	cols=cur.getColumnNames()
	assertEqual(cols[0],"testint")
	assertEqual(cols[1],"testsmallint")
	assertEqual(cols[2],"testtinyint")
	assertEqual(cols[3],"testreal")
	assertEqual(cols[4],"testfloat")
	assertEqual(cols[5],"testdecimal")
	assertEqual(cols[6],"testnumeric")
	assertEqual(cols[7],"testmoney")
	assertEqual(cols[8],"testsmallmoney")
	assertEqual(cols[9],"testdatetime")
	assertEqual(cols[10],"testsmalldatetime")
	assertEqual(cols[11],"testchar")
	assertEqual(cols[12],"testvarchar")
	assertEqual(cols[13],"testbit")
	print()


	# column types
	print("COLUMN TYPES: ")
	assertEqual(cur.getColumnType(0),"INT")
	assertEqual(cur.getColumnType('testint'),"INT")
	assertEqual(cur.getColumnType(1),"SMALLINT")
	assertEqual(cur.getColumnType('testsmallint'),"SMALLINT")
	assertEqual(cur.getColumnType(2),"TINYINT")
	assertEqual(cur.getColumnType('testtinyint'),"TINYINT")
	assertEqual(cur.getColumnType(3),"REAL")
	assertEqual(cur.getColumnType('testreal'),"REAL")
	assertEqual(cur.getColumnType(4),"FLOAT")
	assertEqual(cur.getColumnType('testfloat'),"FLOAT")
	assertEqual(cur.getColumnType(5),"DECIMAL")
	assertEqual(cur.getColumnType('testdecimal'),"DECIMAL")
	assertEqual(cur.getColumnType(6),"NUMERIC")
	assertEqual(cur.getColumnType('testnumeric'),"NUMERIC")
	assertEqual(cur.getColumnType(7),"MONEY")
	assertEqual(cur.getColumnType('testmoney'),"MONEY")
	assertEqual(cur.getColumnType(8),"SMALLMONEY")
	assertEqual(cur.getColumnType('testsmallmoney'),"SMALLMONEY")
	assertEqual(cur.getColumnType(9),"DATETIME")
	assertEqual(cur.getColumnType('testdatetime'),"DATETIME")
	assertEqual(cur.getColumnType(10),"SMALLDATETIME")
	assertEqual(cur.getColumnType('testsmalldatetime'),"SMALLDATETIME")
	assertEqual(cur.getColumnType(11),"CHAR")
	assertEqual(cur.getColumnType('testchar'),"CHAR")
	assertEqual(cur.getColumnType(12),"CHAR")
	assertEqual(cur.getColumnType('testvarchar'),"CHAR")
	assertEqual(cur.getColumnType(13),"BIT")
	assertEqual(cur.getColumnType('testbit'),"BIT")
	print()


	# column length
	print("COLUMN LENGTH: ")
	assertEqual(cur.getColumnLength(0),4)
	assertEqual(cur.getColumnLength('testint'),4)
	assertEqual(cur.getColumnLength(1),2)
	assertEqual(cur.getColumnLength('testsmallint'),2)
	assertEqual(cur.getColumnLength(2),1)
	assertEqual(cur.getColumnLength('testtinyint'),1)
	assertEqual(cur.getColumnLength(3),4)
	assertEqual(cur.getColumnLength('testreal'),4)
	assertEqual(cur.getColumnLength(4),8)
	assertEqual(cur.getColumnLength('testfloat'),8)
	assertEqual(cur.getColumnLength(5),35)
	assertEqual(cur.getColumnLength('testdecimal'),35)
	assertEqual(cur.getColumnLength(6),35)
	assertEqual(cur.getColumnLength('testnumeric'),35)
	assertEqual(cur.getColumnLength(7),8)
	assertEqual(cur.getColumnLength('testmoney'),8)
	assertEqual(cur.getColumnLength(8),4)
	assertEqual(cur.getColumnLength('testsmallmoney'),4)
	assertEqual(cur.getColumnLength(9),8)
	assertEqual(cur.getColumnLength('testdatetime'),8)
	assertEqual(cur.getColumnLength(10),4)
	assertEqual(cur.getColumnLength('testsmalldatetime'),4)
	assertEqual(cur.getColumnLength(11),40)
	assertEqual(cur.getColumnLength('testchar'),40)
	assertEqual(cur.getColumnLength(12),40)
	assertEqual(cur.getColumnLength('testvarchar'),40)
	assertEqual(cur.getColumnLength(13),1)
	assertEqual(cur.getColumnLength('testbit'),1)
	print()


	# longest column
	print("LONGEST COLUMN: ")
	assertEqual(cur.getLongest(0),1)
	assertEqual(cur.getLongest('testint'),1)
	assertEqual(cur.getLongest(1),1)
	assertEqual(cur.getLongest('testsmallint'),1)
	assertEqual(cur.getLongest(2),1)
	assertEqual(cur.getLongest('testtinyint'),1)
	assertEqual(cur.getLongest(3),18)
	assertEqual(cur.getLongest('testreal'),18)
	assertEqual(cur.getLongest(4),18)
	assertEqual(cur.getLongest('testfloat'),18)
	assertEqual(cur.getLongest(5),3)
	assertEqual(cur.getLongest('testdecimal'),3)
	assertEqual(cur.getLongest(6),3)
	assertEqual(cur.getLongest('testnumeric'),3)
	assertEqual(cur.getLongest(7),4)
	assertEqual(cur.getLongest('testmoney'),4)
	assertEqual(cur.getLongest(8),4)
	assertEqual(cur.getLongest('testsmallmoney'),4)
	assertEqual(cur.getLongest(9),19)
	assertEqual(cur.getLongest('testdatetime'),19)
	assertEqual(cur.getLongest(10),19)
	assertEqual(cur.getLongest('testsmalldatetime'),19)
	assertEqual(cur.getLongest(11),40)
	assertEqual(cur.getLongest('testchar'),40)
	assertEqual(cur.getLongest(12),12)
	assertEqual(cur.getLongest('testvarchar'),12)
	assertEqual(cur.getLongest(13),1)
	assertEqual(cur.getLongest('testbit'),1)
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
	#assertEqual(cur.getField(0,3),Decimal("1.1"))
	#assertEqual(cur.getField(0,4),Decimal("1.1"))
	assertEqual(cur.getField(0,5),Decimal("1.1"))
	assertEqual(cur.getField(0,6),Decimal("1.1"))
	assertEqual(cur.getField(0,7),Decimal("1.00"))
	assertEqual(cur.getField(0,8),Decimal("1.00"))
	assertEqual(cur.getField(0,9),"Jan  1 2001  1:00AM")
	assertEqual(cur.getField(0,10),"Jan  1 2001  1:00AM")
	assertEqual(cur.getField(0,11),"testchar1                               ")
	assertEqual(cur.getField(0,12),"testvarchar1")
	assertEqual(cur.getField(0,13),1)
	print()
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(7,1),8)
	assertEqual(cur.getField(7,2),8)
	#assertEqual(cur.getField(7,3),Decimal("8.8"))
	#assertEqual(cur.getField(7,4),Decimal("8.8"))
	assertEqual(cur.getField(7,5),Decimal("8.8"))
	assertEqual(cur.getField(7,6),Decimal("8.8"))
	assertEqual(cur.getField(7,7),Decimal("8.00"))
	assertEqual(cur.getField(7,8),Decimal("8.00"))
	assertEqual(cur.getField(7,9),"Jan  1 2008  8:00AM")
	assertEqual(cur.getField(7,10),"Jan  1 2008  8:00AM")
	assertEqual(cur.getField(7,11),"testchar8                               ")
	assertEqual(cur.getField(7,12),"testvarchar8")
	assertEqual(cur.getField(7,13),1)
	print()


	# field lengths by index
	print("FIELD LENGTHS BY INDEX: ")
	assertEqual(cur.getFieldLength(0,0),1)
	assertEqual(cur.getFieldLength(0,1),1)
	assertEqual(cur.getFieldLength(0,2),1)
	assertEqual(cur.getFieldLength(0,3),18)
	assertEqual(cur.getFieldLength(0,4),18)
	assertEqual(cur.getFieldLength(0,5),3)
	assertEqual(cur.getFieldLength(0,6),3)
	assertEqual(cur.getFieldLength(0,7),4)
	assertEqual(cur.getFieldLength(0,8),4)
	assertEqual(cur.getFieldLength(0,9),19)
	assertEqual(cur.getFieldLength(0,10),19)
	assertEqual(cur.getFieldLength(0,11),40)
	assertEqual(cur.getFieldLength(0,12),12)
	assertEqual(cur.getFieldLength(0,13),1)
	print()
	assertEqual(cur.getFieldLength(7,0),1)
	assertEqual(cur.getFieldLength(7,1),1)
	assertEqual(cur.getFieldLength(7,2),1)
	assertEqual(cur.getFieldLength(7,3),18)
	assertEqual(cur.getFieldLength(7,4),18)
	assertEqual(cur.getFieldLength(7,5),3)
	assertEqual(cur.getFieldLength(7,6),3)
	assertEqual(cur.getFieldLength(7,7),4)
	assertEqual(cur.getFieldLength(7,8),4)
	assertEqual(cur.getFieldLength(7,9),19)
	assertEqual(cur.getFieldLength(7,10),19)
	assertEqual(cur.getFieldLength(7,11),40)
	assertEqual(cur.getFieldLength(7,12),12)
	assertEqual(cur.getFieldLength(7,13),1)
	print()


	# fields by name
	print("FIELDS BY NAME: ")
	assertEqual(cur.getField(0,"testint"),1)
	assertEqual(cur.getField(0,"testsmallint"),1)
	assertEqual(cur.getField(0,"testtinyint"),1)
	#assertEqual(cur.getField(0,"testreal"),Decimal("1.1"))
	#assertEqual(cur.getField(0,"testfloat"),Decimal("1.1"))
	assertEqual(cur.getField(0,"testdecimal"),Decimal("1.1"))
	assertEqual(cur.getField(0,"testnumeric"),Decimal("1.1"))
	assertEqual(cur.getField(0,"testmoney"),Decimal("1.00"))
	assertEqual(cur.getField(0,"testsmallmoney"),Decimal("1.00"))
	assertEqual(cur.getField(0,"testdatetime"),"Jan  1 2001  1:00AM")
	assertEqual(cur.getField(0,"testsmalldatetime"),"Jan  1 2001  1:00AM")
	assertEqual(cur.getField(0,"testchar"),"testchar1                               ")
	assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
	assertEqual(cur.getField(0,"testbit"),1)
	print()
	assertEqual(cur.getField(7,"testint"),8)
	assertEqual(cur.getField(7,"testsmallint"),8)
	assertEqual(cur.getField(7,"testtinyint"),8)
	#assertEqual(cur.getField(7,"testreal"),Decimal("8.8"))
	#assertEqual(cur.getField(7,"testfloat"),Decimal("8.8"))
	assertEqual(cur.getField(7,"testdecimal"),Decimal("8.8"))
	assertEqual(cur.getField(7,"testnumeric"),Decimal("8.8"))
	assertEqual(cur.getField(7,"testmoney"),Decimal("8.00"))
	assertEqual(cur.getField(7,"testsmallmoney"),Decimal("8.00"))
	assertEqual(cur.getField(7,"testdatetime"),"Jan  1 2008  8:00AM")
	assertEqual(cur.getField(7,"testsmalldatetime"),"Jan  1 2008  8:00AM")
	assertEqual(cur.getField(7,"testchar"),"testchar8                               ")
	assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
	assertEqual(cur.getField(7,"testbit"),1)
	print()


	# field lengths by name
	print("FIELD LENGTHS BY NAME: ")
	assertEqual(cur.getFieldLength(0,"testint"),1)
	assertEqual(cur.getFieldLength(0,"testsmallint"),1)
	assertEqual(cur.getFieldLength(0,"testtinyint"),1)
	#assertEqual(cur.getFieldLength(0,"testreal"),3)
	#assertEqual(cur.getFieldLength(0,"testfloat"),3)
	assertEqual(cur.getFieldLength(0,"testdecimal"),3)
	assertEqual(cur.getFieldLength(0,"testnumeric"),3)
	assertEqual(cur.getFieldLength(0,"testmoney"),4)
	assertEqual(cur.getFieldLength(0,"testsmallmoney"),4)
	assertEqual(cur.getFieldLength(0,"testdatetime"),19)
	assertEqual(cur.getFieldLength(0,"testsmalldatetime"),19)
	assertEqual(cur.getFieldLength(0,"testchar"),40)
	assertEqual(cur.getFieldLength(0,"testvarchar"),12)
	assertEqual(cur.getFieldLength(0,"testbit"),1)
	print()
	assertEqual(cur.getFieldLength(7,"testint"),1)
	assertEqual(cur.getFieldLength(7,"testsmallint"),1)
	assertEqual(cur.getFieldLength(7,"testtinyint"),1)
	#assertEqual(cur.getFieldLength(7,"testreal"),3)
	#assertEqual(cur.getFieldLength(7,"testfloat"),3)
	assertEqual(cur.getFieldLength(7,"testdecimal"),3)
	assertEqual(cur.getFieldLength(7,"testnumeric"),3)
	assertEqual(cur.getFieldLength(7,"testmoney"),4)
	assertEqual(cur.getFieldLength(7,"testsmallmoney"),4)
	assertEqual(cur.getFieldLength(7,"testdatetime"),19)
	assertEqual(cur.getFieldLength(7,"testsmalldatetime"),19)
	assertEqual(cur.getFieldLength(7,"testchar"),40)
	assertEqual(cur.getFieldLength(7,"testvarchar"),12)
	assertEqual(cur.getFieldLength(7,"testbit"),1)
	print()


	# fields by array
	print("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEqual(fields[0],1)
	assertEqual(fields[1],1)
	assertEqual(fields[2],1)
	#assertEqual(fields[3],Decimal("1.1"))
	#assertEqual(fields[4],Decimal("1.1"))
	assertEqual(fields[5],Decimal("1.1"))
	assertEqual(fields[6],Decimal("1.1"))
	assertEqual(fields[7],Decimal("1.00"))
	assertEqual(fields[8],Decimal("1.00"))
	assertEqual(fields[9],"Jan  1 2001  1:00AM")
	assertEqual(fields[10],"Jan  1 2001  1:00AM")
	assertEqual(fields[11],"testchar1                               ")
	assertEqual(fields[12],"testvarchar1")
	assertEqual(fields[13],1)
	print()


	# field lengths by array
	print("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEqual(fieldlens[0],1)
	assertEqual(fieldlens[1],1)
	assertEqual(fieldlens[2],1)
	#assertEqual(fieldlens[3],3)
	#assertEqual(fieldlens[4],3)
	assertEqual(fieldlens[5],3)
	assertEqual(fieldlens[6],3)
	assertEqual(fieldlens[7],4)
	assertEqual(fieldlens[8],4)
	assertEqual(fieldlens[9],19)
	assertEqual(fieldlens[10],19)
	assertEqual(fieldlens[11],40)
	assertEqual(fieldlens[12],12)
	assertEqual(fieldlens[13],1)
	print()


	# fields by dictionary
	print("FIELDS BY DICTIONARY: ")
	fields=cur.getRowDictionary(0)
	assertEqual(fields["testint"],1)
	assertEqual(fields["testsmallint"],1)
	assertEqual(fields["testtinyint"],1)
	#assertEqual(fields["testreal"],Decimal("1.1"))
	#assertEqual(fields["testfloat"],Decimal("1.1"))
	assertEqual(fields["testdecimal"],Decimal("1.1"))
	assertEqual(fields["testnumeric"],Decimal("1.1"))
	assertEqual(fields["testmoney"],Decimal("1.00"))
	assertEqual(fields["testsmallmoney"],Decimal("1.00"))
	assertEqual(fields["testdatetime"],"Jan  1 2001  1:00AM")
	assertEqual(fields["testsmalldatetime"],"Jan  1 2001  1:00AM")
	assertEqual(fields["testchar"],"testchar1                               ")
	assertEqual(fields["testvarchar"],"testvarchar1")
	assertEqual(fields["testbit"],1)
	print()
	fields=cur.getRowDictionary(7)
	assertEqual(fields["testint"],8)
	assertEqual(fields["testsmallint"],8)
	assertEqual(fields["testtinyint"],8)
	#assertEqual(fields["testreal"],Decimal("8.8"))
	#assertEqual(fields["testfloat"],Decimal("8.8"))
	assertEqual(fields["testdecimal"],Decimal("8.8"))
	assertEqual(fields["testnumeric"],Decimal("8.8"))
	assertEqual(fields["testmoney"],Decimal("8.00"))
	assertEqual(fields["testsmallmoney"],Decimal("8.00"))
	assertEqual(fields["testdatetime"],"Jan  1 2008  8:00AM")
	assertEqual(fields["testsmalldatetime"],"Jan  1 2008  8:00AM")
	assertEqual(fields["testchar"],"testchar8                               ")
	assertEqual(fields["testvarchar"],"testvarchar8")
	assertEqual(fields["testbit"],1)
	print()


	# field lengths by dictionary
	print("FIELD LENGTHS BY DICTIONARY: ")
	fieldlengths=cur.getRowLengthsDictionary(0)
	assertEqual(fieldlengths["testint"],1)
	assertEqual(fieldlengths["testsmallint"],1)
	assertEqual(fieldlengths["testtinyint"],1)
	#assertEqual(fieldlengths["testreal"],3)
	#assertEqual(fieldlengths["testfloat"],3)
	assertEqual(fieldlengths["testdecimal"],3)
	assertEqual(fieldlengths["testnumeric"],3)
	assertEqual(fieldlengths["testmoney"],4)
	assertEqual(fieldlengths["testsmallmoney"],4)
	assertEqual(fieldlengths["testdatetime"],19)
	assertEqual(fieldlengths["testsmalldatetime"],19)
	assertEqual(fieldlengths["testchar"],40)
	assertEqual(fieldlengths["testvarchar"],12)
	assertEqual(fieldlengths["testbit"],1)
	print()
	fieldlengths=cur.getRowLengthsDictionary(7)
	assertEqual(fieldlengths["testsmallint"],1)
	assertEqual(fieldlengths["testtinyint"],1)
	#assertEqual(fieldlengths["testreal"],3)
	#assertEqual(fieldlengths["testfloat"],3)
	assertEqual(fieldlengths["testdecimal"],3)
	assertEqual(fieldlengths["testnumeric"],3)
	assertEqual(fieldlengths["testmoney"],4)
	assertEqual(fieldlengths["testsmallmoney"],4)
	assertEqual(fieldlengths["testdatetime"],19)
	assertEqual(fieldlengths["testsmalldatetime"],19)
	assertEqual(fieldlengths["testchar"],40)
	assertEqual(fieldlengths["testvarchar"],12)
	assertEqual(fieldlengths["testbit"],1)
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
	# not a good test, sap turns all 3 of these fields into int types
	assertEqual(cur.getField(0,0),0)
	assertEqual(cur.getField(0,1),1)
	assertEqual(cur.getField(0,2),0)
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
	assertEqual(cur.getColumnType(0),"INT")
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
	assertEqual(cur.colCount(),14)
	print()


	# column names for cached result set
	print("COLUMN NAMES FOR CACHED RESULT SET: ")
	assertEqual(cur.getColumnName(0),"testint")
	assertEqual(cur.getColumnName(1),"testsmallint")
	assertEqual(cur.getColumnName(2),"testtinyint")
	assertEqual(cur.getColumnName(3),"testreal")
	assertEqual(cur.getColumnName(4),"testfloat")
	assertEqual(cur.getColumnName(5),"testdecimal")
	assertEqual(cur.getColumnName(6),"testnumeric")
	assertEqual(cur.getColumnName(7),"testmoney")
	assertEqual(cur.getColumnName(8),"testsmallmoney")
	assertEqual(cur.getColumnName(9),"testdatetime")
	assertEqual(cur.getColumnName(10),"testsmalldatetime")
	assertEqual(cur.getColumnName(11),"testchar")
	assertEqual(cur.getColumnName(12),"testvarchar")
	assertEqual(cur.getColumnName(13),"testbit")
	cols=cur.getColumnNames()
	assertEqual(cols[0],"testint")
	assertEqual(cols[1],"testsmallint")
	assertEqual(cols[2],"testtinyint")
	assertEqual(cols[3],"testreal")
	assertEqual(cols[4],"testfloat")
	assertEqual(cols[5],"testdecimal")
	assertEqual(cols[6],"testnumeric")
	assertEqual(cols[7],"testmoney")
	assertEqual(cols[8],"testsmallmoney")
	assertEqual(cols[9],"testdatetime")
	assertEqual(cols[10],"testsmalldatetime")
	assertEqual(cols[11],"testchar")
	assertEqual(cols[12],"testvarchar")
	assertEqual(cols[13],"testbit")
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


	# row range
	print("ROW RANGE:")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	print()
	rows=cur.getRowRange(0,5)
	assertEqual(rows[0][0],1)
	assertEqual(rows[0][1],1)
	assertEqual(rows[0][2],1)
	#assertEqual(rows[0][3],Decimal("1.1"))
	#assertEqual(rows[0][4],Decimal("1.1"))
	assertEqual(rows[0][5],Decimal("1.1"))
	assertEqual(rows[0][6],Decimal("1.1"))
	assertEqual(rows[0][7],Decimal("1.00"))
	assertEqual(rows[0][8],Decimal("1.00"))
	assertEqual(rows[0][9],"Jan  1 2001  1:00AM")
	assertEqual(rows[0][10],"Jan  1 2001  1:00AM")
	assertEqual(rows[0][11],"testchar1                               ")
	assertEqual(rows[0][12],"testvarchar1")
	assertEqual(rows[0][13],1)
	print()
	assertEqual(rows[1][0],2)
	assertEqual(rows[1][1],2)
	assertEqual(rows[1][2],2)
	#assertEqual(rows[1][3],Decimal("2.2"))
	#assertEqual(rows[1][4],Decimal("2.2"))
	assertEqual(rows[1][5],Decimal("2.2"))
	assertEqual(rows[1][6],Decimal("2.2"))
	assertEqual(rows[1][7],Decimal("2.00"))
	assertEqual(rows[1][8],Decimal("2.00"))
	assertEqual(rows[1][9],"Jan  1 2002  2:00AM")
	assertEqual(rows[1][10],"Jan  1 2002  2:00AM")
	assertEqual(rows[1][11],"testchar2                               ")
	assertEqual(rows[1][12],"testvarchar2")
	assertEqual(rows[1][13],1)
	print()
	assertEqual(rows[2][0],3)
	assertEqual(rows[2][1],3)
	assertEqual(rows[2][2],3)
	#assertEqual(rows[2][3],Decimal("3.3"))
	#assertEqual(rows[2][4],Decimal("3.3"))
	assertEqual(rows[2][5],Decimal("3.3"))
	assertEqual(rows[2][6],Decimal("3.3"))
	assertEqual(rows[2][7],Decimal("3.00"))
	assertEqual(rows[2][8],Decimal("3.00"))
	assertEqual(rows[2][9],"Jan  1 2003  3:00AM")
	assertEqual(rows[2][10],"Jan  1 2003  3:00AM")
	assertEqual(rows[2][11],"testchar3                               ")
	assertEqual(rows[2][12],"testvarchar3")
	assertEqual(rows[2][13],1)
	print()
	assertEqual(rows[3][0],4)
	assertEqual(rows[3][1],4)
	assertEqual(rows[3][2],4)
	#assertEqual(rows[3][3],Decimal("4.4"))
	#assertEqual(rows[3][4],Decimal("4.4"))
	assertEqual(rows[3][5],Decimal("4.4"))
	assertEqual(rows[3][6],Decimal("4.4"))
	assertEqual(rows[3][7],Decimal("4.00"))
	assertEqual(rows[3][8],Decimal("4.00"))
	assertEqual(rows[3][9],"Jan  1 2004  4:00AM")
	assertEqual(rows[3][10],"Jan  1 2004  4:00AM")
	assertEqual(rows[3][11],"testchar4                               ")
	assertEqual(rows[3][12],"testvarchar4")
	assertEqual(rows[3][13],1)
	print()
	assertEqual(rows[4][0],5)
	assertEqual(rows[4][1],5)
	assertEqual(rows[4][2],5)
	#assertEqual(rows[4][3],Decimal("5.5"))
	#assertEqual(rows[4][4],Decimal("5.5"))
	assertEqual(rows[4][5],Decimal("5.5"))
	assertEqual(rows[4][6],Decimal("5.5"))
	assertEqual(rows[4][7],Decimal("5.00"))
	assertEqual(rows[4][8],Decimal("5.00"))
	assertEqual(rows[4][9],"Jan  1 2005  5:00AM")
	assertEqual(rows[4][10],"Jan  1 2005  5:00AM")
	assertEqual(rows[4][11],"testchar5                               ")
	assertEqual(rows[4][12],"testvarchar5")
	assertEqual(rows[4][13],1)
	print()
	assertEqual(rows[5][0],6)
	assertEqual(rows[5][1],6)
	assertEqual(rows[5][2],6)
	#assertEqual(rows[5][3],Decimal("6.6"))
	#assertEqual(rows[5][4],Decimal("6.6"))
	assertEqual(rows[5][5],Decimal("6.6"))
	assertEqual(rows[5][6],Decimal("6.6"))
	assertEqual(rows[5][7],Decimal("6.00"))
	assertEqual(rows[5][8],Decimal("6.00"))
	assertEqual(rows[5][9],"Jan  1 2006  6:00AM")
	assertEqual(rows[5][10],"Jan  1 2006  6:00AM")
	assertEqual(rows[5][11],"testchar6                               ")
	assertEqual(rows[5][12],"testvarchar6")
	assertEqual(rows[5][13],1)
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
