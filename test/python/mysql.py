#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.

from SQLRelay import PySQLRClient
import sys
from asserts import *
import string
from decimal import Decimal


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
	assertEqual(con.identify(),"mysql")
	print()

	# get the db version
	dbversion=con.dbVersion()
	majorversion=int(dbversion[0:1])


	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()


	# isolation levels
	print("ISOLATION LEVELS: ")
	isolationlevels=["REPEATABLE-READ","READ-UNCOMMITTED",
			"READ-COMMITTED","SERIALIZABLE"]
	for il in isolationlevels:
		assertTrue(con.setIsolationLevel(il))
		assertEqual(con.getIsolationLevel(),il)
		print()
	# reset to the default isolation level
	assertTrue(con.setIsolationLevel(isolationlevels[0]))
	print()

	# drop existing table
	cur.sendQuery("drop table testtable")

	# create a new table


	# create temptable
	print("CREATE TEMPTABLE: ")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testtinyint tinyint, "
		"	testsmallint smallint, "
		"	testmediumint mediumint, "
		"	testint int, "
		"	testbigint bigint, "
		"	testfloat float, "
		"	testreal real, "
		"	testdecimal decimal(2,1), "
		"	testdate date, "
		"	testtime time, "
		"	testdatetime datetime, "
		"	testyear year, "
		"	testchar char(40), "
		"	testtext text, "
		"	testvarchar varchar(40), "
		"	testtinytext tinytext, "
		"	testmediumtext mediumtext, "
		"	testlongtext longtext, "
		"	testtimestamp timestamp)"))
	print()


	# begin transaction
	print("BEGIN TRANSACTION: ")
	assertTrue(cur.sendQuery("begin"))
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
		"	1, "
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	'2001-01-01', "
		"	'01:00:00', "
		"	'2001-01-01 01:00:00', "
		"	'2001', "
		"	'char1', "
		"	'text1', "
		"	'varchar1', "
		"	'tinytext1', "
		"	'mediumtext1', "
		"	'longtext1', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2, "
		"	2, "
		"	2, "
		"	2, "
		"	2.1, "
		"	2.1, "
		"	2.1, "
		"	'2002-01-01', "
		"	'02:00:00', "
		"	'2002-01-01 02:00:00', "
		"	'2002', "
		"	'char2', "
		"	'text2', "
		"	'varchar2', "
		"	'tinytext2', "
		"	'mediumtext2', "
		"	'longtext2', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3, "
		"	3, "
		"	3, "
		"	3, "
		"	3.1, "
		"	3.1, "
		"	3.1, "
		"	'2003-01-01', "
		"	'03:00:00', "
		"	'2003-01-01 03:00:00', "
		"	'2003', "
		"	'char3', "
		"	'text3', "
		"	'varchar3', "
		"	'tinytext3', "
		"	'mediumtext3', "
		"	'longtext3', "
		"	NULL)"))
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4, "
		"	4, "
		"	4, "
		"	4, "
		"	4.1, "
		"	4.1, "
		"	4.1, "
		"	'2004-01-01', "
		"	'04:00:00', "
		"	'2004-01-01 04:00:00', "
		"	'2004', "
		"	'char4', "
		"	'text4', "
		"	'varchar4', "
		"	'tinytext4', "
		"	'mediumtext4', "
		"	'longtext4', "
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL)")
	assertEqual(cur.countBindVariables(),18)
	cur.inputBind("1",5)
	cur.inputBind("2",5)
	cur.inputBind("3",5)
	cur.inputBind("4",5)
	cur.inputBind("5",5)
	cur.inputBind("6",5.1,2,1)
	cur.inputBind("7",5.1,2,1)
	cur.inputBind("8",5.1,2,1)
	cur.inputBind("9","2005-01-01")
	cur.inputBind("10","05:00:00")
	cur.inputBind("11","2005-01-01 05:00:00")
	cur.inputBind("12","2005")
	cur.inputBind("13","char5")
	cur.inputBind("14","text5")
	cur.inputBind("15","varchar5")
	cur.inputBind("16","tinytext5")
	cur.inputBind("17","mediumtext5")
	cur.inputBind("18","longtext5")
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1",6)
	cur.inputBind("2",6)
	cur.inputBind("3",6)
	cur.inputBind("4",6)
	cur.inputBind("5",6)
	cur.inputBind("6",6.1,2,1)
	cur.inputBind("7",6.1,2,1)
	cur.inputBind("8",6.1,2,1)
	cur.inputBind("9",'2006-01-01')
	cur.inputBind("10",'06:00:00')
	cur.inputBind("11",'2006-01-01 06:00:00')
	cur.inputBind("12",'2006')
	cur.inputBind("13",'char6')
	cur.inputBind("14",'text6')
	cur.inputBind("15",'varchar6')
	cur.inputBind("16",'tinytext6')
	cur.inputBind("17",'mediumtext6')
	cur.inputBind("18",'longtext6')
	assertTrue(cur.executeQuery())
	print()


	# array of binds by position
	print("ARRAY OF BINDS BY POSITION: ")
	cur.clearBinds()
	cur.inputBinds(["1","2","3","4","5","6",
			"7","8","9","10","11","12",
			"13","14","15",
			"16","17","18",],
		[7,7,7,7,7,7.1,7.1,7.1,'2007-01-01','07:00:00','2007-01-01 07:00:00','2007','char7','text7','varchar7','tinytext7','mediumtext7','longtext7'],
		[0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0],
		[0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0])
	assertTrue(cur.executeQuery())
	print()


	# bind by position with validation
	print("BIND BY POSITION WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("1",8)
	cur.inputBind("2",8)
	cur.inputBind("3",8)
	cur.inputBind("4",8)
	cur.inputBind("5",8)
	cur.inputBind("6",8.1,2,1)
	cur.inputBind("7",8.1,2,1)
	cur.inputBind("8",8.1,2,1)
	cur.inputBind("9",'2008-01-01')
	cur.inputBind("10",'08:00:00')
	cur.inputBind("11",'2008-01-01 08:00:00')
	cur.inputBind("12",'2008')
	cur.inputBind("13",'char8')
	cur.inputBind("14",'text8')
	cur.inputBind("15",'varchar8')
	cur.inputBind("16",'tinytext8')
	cur.inputBind("17",'mediumtext8')
	cur.inputBind("18",'longtext8')
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	print()


	# select
	print("SELECT: ")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
	print()


	# column count
	print("COLUMN COUNT: ")
	assertEqual(cur.colCount(),19)
	print()


	# column names
	print("COLUMN NAMES: ")
	assertEqual(cur.getColumnName(0),"testtinyint")
	assertEqual(cur.getColumnName(1),"testsmallint")
	assertEqual(cur.getColumnName(2),"testmediumint")
	assertEqual(cur.getColumnName(3),"testint")
	assertEqual(cur.getColumnName(4),"testbigint")
	assertEqual(cur.getColumnName(5),"testfloat")
	assertEqual(cur.getColumnName(6),"testreal")
	assertEqual(cur.getColumnName(7),"testdecimal")
	assertEqual(cur.getColumnName(8),"testdate")
	assertEqual(cur.getColumnName(9),"testtime")
	assertEqual(cur.getColumnName(10),"testdatetime")
	assertEqual(cur.getColumnName(11),"testyear")
	assertEqual(cur.getColumnName(12),"testchar")
	assertEqual(cur.getColumnName(13),"testtext")
	assertEqual(cur.getColumnName(14),"testvarchar")
	assertEqual(cur.getColumnName(15),"testtinytext")
	assertEqual(cur.getColumnName(16),"testmediumtext")
	assertEqual(cur.getColumnName(17),"testlongtext")
	assertEqual(cur.getColumnName(18),"testtimestamp")
	cols=cur.getColumnNames()
	assertEqual(cols[0],"testtinyint")
	assertEqual(cols[1],"testsmallint")
	assertEqual(cols[2],"testmediumint")
	assertEqual(cols[3],"testint")
	assertEqual(cols[4],"testbigint")
	assertEqual(cols[5],"testfloat")
	assertEqual(cols[6],"testreal")
	assertEqual(cols[7],"testdecimal")
	assertEqual(cols[8],"testdate")
	assertEqual(cols[9],"testtime")
	assertEqual(cols[10],"testdatetime")
	assertEqual(cols[11],"testyear")
	assertEqual(cols[12],"testchar")
	assertEqual(cols[13],"testtext")
	assertEqual(cols[14],"testvarchar")
	assertEqual(cols[15],"testtinytext")
	assertEqual(cols[16],"testmediumtext")
	assertEqual(cols[17],"testlongtext")
	assertEqual(cols[18],"testtimestamp")
	print()


	# column types
	print("COLUMN TYPES: ")
	assertEqual(cur.getColumnType(0),"TINYINT")
	assertEqual(cur.getColumnType(1),"SMALLINT")
	assertEqual(cur.getColumnType(2),"MEDIUMINT")
	assertEqual(cur.getColumnType(3),"INT")
	assertEqual(cur.getColumnType(4),"BIGINT")
	assertEqual(cur.getColumnType(5),"FLOAT")
	assertEqual(cur.getColumnType(6),"REAL")
	assertEqual(cur.getColumnType(7),"DECIMAL")
	assertEqual(cur.getColumnType(8),"DATE")
	assertEqual(cur.getColumnType(9),"TIME")
	assertEqual(cur.getColumnType(10),"DATETIME")
	assertEqual(cur.getColumnType(11),"YEAR")
	if majorversion==3:
		assertEqual(cur.getColumnType(12),"VARSTRING")
	else:
		assertEqual(cur.getColumnType(12),"STRING")
	assertEqual(cur.getColumnType(13),"BLOB")
	assertEqual(cur.getColumnType(14),"VARSTRING")
	assertEqual(cur.getColumnType(15),"TINYBLOB")
	assertEqual(cur.getColumnType(16),"MEDIUMBLOB")
	assertEqual(cur.getColumnType(17),"LONGBLOB")
	assertEqual(cur.getColumnType(18),"TIMESTAMP")
	assertEqual(cur.getColumnType("testtinyint"),"TINYINT")
	assertEqual(cur.getColumnType("testsmallint"),"SMALLINT")
	assertEqual(cur.getColumnType("testmediumint"),"MEDIUMINT")
	assertEqual(cur.getColumnType("testint"),"INT")
	assertEqual(cur.getColumnType("testbigint"),"BIGINT")
	assertEqual(cur.getColumnType("testfloat"),"FLOAT")
	assertEqual(cur.getColumnType("testreal"),"REAL")
	assertEqual(cur.getColumnType("testdecimal"),"DECIMAL")
	assertEqual(cur.getColumnType("testdate"),"DATE")
	assertEqual(cur.getColumnType("testtime"),"TIME")
	assertEqual(cur.getColumnType("testdatetime"),"DATETIME")
	assertEqual(cur.getColumnType("testyear"),"YEAR")
	if majorversion==3:
		assertEqual(cur.getColumnType("testchar"),"VARSTRING")
	else:
		assertEqual(cur.getColumnType("testchar"),"STRING")
	assertEqual(cur.getColumnType("testtext"),"BLOB")
	assertEqual(cur.getColumnType("testvarchar"),"VARSTRING")
	assertEqual(cur.getColumnType("testtinytext"),"TINYBLOB")
	assertEqual(cur.getColumnType("testmediumtext"),"MEDIUMBLOB")
	assertEqual(cur.getColumnType("testlongtext"),"LONGBLOB")
	assertEqual(cur.getColumnType("testtimestamp"),"TIMESTAMP")
	print()


	# column length
	print("COLUMN LENGTH: ")
	assertEqual(cur.getColumnLength(0),1)
	assertEqual(cur.getColumnLength(1),2)
	assertEqual(cur.getColumnLength(2),3)
	assertEqual(cur.getColumnLength(3),4)
	assertEqual(cur.getColumnLength(4),8)
	assertEqual(cur.getColumnLength(5),4)
	assertEqual(cur.getColumnLength(6),8)
	assertEqual(cur.getColumnLength(7),6)
	assertEqual(cur.getColumnLength(8),3)
	assertEqual(cur.getColumnLength(9),3)
	assertEqual(cur.getColumnLength(10),8)
	assertEqual(cur.getColumnLength(11),1)
	#assertEqual(cur.getColumnLength(12),40)
	assertEqual(cur.getColumnLength(13),65535)
	#assertEqual(cur.getColumnLength(14),41)
	assertEqual(cur.getColumnLength(15),255)
	assertEqual(cur.getColumnLength(16),16777215)
	assertEqual(cur.getColumnLength(17),2147483647)
	assertEqual(cur.getColumnLength(18),4)
	assertEqual(cur.getColumnLength("testtinyint"),1)
	assertEqual(cur.getColumnLength("testsmallint"),2)
	assertEqual(cur.getColumnLength("testmediumint"),3)
	assertEqual(cur.getColumnLength("testint"),4)
	assertEqual(cur.getColumnLength("testbigint"),8)
	assertEqual(cur.getColumnLength("testfloat"),4)
	assertEqual(cur.getColumnLength("testreal"),8)
	assertEqual(cur.getColumnLength("testdecimal"),6)
	assertEqual(cur.getColumnLength("testdate"),3)
	assertEqual(cur.getColumnLength("testtime"),3)
	assertEqual(cur.getColumnLength("testdatetime"),8)
	assertEqual(cur.getColumnLength("testyear"),1)
	#assertEqual(cur.getColumnLength("testchar"),40)
	assertEqual(cur.getColumnLength("testtext"),65535)
	#assertEqual(cur.getColumnLength("testvarchar"),41)
	assertEqual(cur.getColumnLength("testtinytext"),255)
	assertEqual(cur.getColumnLength("testmediumtext"),16777215)
	assertEqual(cur.getColumnLength("testlongtext"),2147483647)
	assertEqual(cur.getColumnLength("testtimestamp"),4)
	print()


	# longest column
	print("LONGEST COLUMN: ")
	assertEqual(cur.getLongest(0),1)
	assertEqual(cur.getLongest(1),1)
	assertEqual(cur.getLongest(2),1)
	assertEqual(cur.getLongest(3),1)
	assertEqual(cur.getLongest(4),1)
	#assertEqual(cur.getLongest(5),3)
	assertEqual(cur.getLongest(6),3)
	assertEqual(cur.getLongest(7),3)
	assertEqual(cur.getLongest(8),10)
	assertEqual(cur.getLongest(9),8)
	assertEqual(cur.getLongest(10),19)
	assertEqual(cur.getLongest(11),4)
	assertEqual(cur.getLongest(12),5)
	assertEqual(cur.getLongest(13),5)
	assertEqual(cur.getLongest(14),8)
	assertEqual(cur.getLongest(15),9)
	assertEqual(cur.getLongest(16),11)
	assertEqual(cur.getLongest(17),9)
	if majorversion==3:
		assertEqual(cur.getLongest(18),14)
	else:
		assertEqual(cur.getLongest(18),19)
	assertEqual(cur.getLongest("testtinyint"),1)
	assertEqual(cur.getLongest("testsmallint"),1)
	assertEqual(cur.getLongest("testmediumint"),1)
	assertEqual(cur.getLongest("testint"),1)
	assertEqual(cur.getLongest("testbigint"),1)
	#assertEqual(cur.getLongest("testfloat"),3)
	assertEqual(cur.getLongest("testreal"),3)
	assertEqual(cur.getLongest("testdecimal"),3)
	assertEqual(cur.getLongest("testdate"),10)
	assertEqual(cur.getLongest("testtime"),8)
	assertEqual(cur.getLongest("testdatetime"),19)
	assertEqual(cur.getLongest("testyear"),4)
	assertEqual(cur.getLongest("testchar"),5)
	assertEqual(cur.getLongest("testtext"),5)
	assertEqual(cur.getLongest("testvarchar"),8)
	assertEqual(cur.getLongest("testtinytext"),9)
	assertEqual(cur.getLongest("testmediumtext"),11)
	assertEqual(cur.getLongest("testlongtext"),9)
	if majorversion==3:
		assertEqual(cur.getLongest("testtimestamp"),14)
	else:
		assertEqual(cur.getLongest("testtimestamp"),19)
	print()


	# row count
	print("ROW COUNT: ")
	assertEqual(cur.rowCount(),8)
	print()


	# total rows
	print("TOTAL ROWS: ")
	# older versions of mysql know this
	#assertEqual(cur.totalRows(),0)
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
	assertEqual(cur.getField(0,3),1)
	assertEqual(cur.getField(0,4),1)
	#assertEqual(cur.getField(0,5),Decimal("1.1"))
	#assertEqual(cur.getField(0,6),Decimal("1.1"))
	assertEqual(cur.getField(0,7),Decimal("1.1"))
	assertEqual(cur.getField(0,8),"2001-01-01")
	assertEqual(cur.getField(0,9),"01:00:00")
	assertEqual(cur.getField(0,10),"2001-01-01 01:00:00")
	assertEqual(cur.getField(0,11),2001)
	assertEqual(cur.getField(0,12),"char1")
	assertEqual(cur.getField(0,13),btos("text1"))
	assertEqual(cur.getField(0,14),"varchar1")
	assertEqual(cur.getField(0,15),btos("tinytext1"))
	assertEqual(cur.getField(0,16),btos("mediumtext1"))
	assertEqual(cur.getField(0,17),btos("longtext1"))
	print()
	assertEqual(cur.getField(7,0),8)
	assertEqual(cur.getField(7,1),8)
	assertEqual(cur.getField(7,2),8)
	assertEqual(cur.getField(7,3),8)
	assertEqual(cur.getField(7,4),8)
        #assertEqual(cur.getField(7,5),Decimal("8.1"))
        #assertEqual(cur.getField(7,6),Decimal("8.1"))
	assertEqual(cur.getField(7,7),Decimal("8.1"))
	assertEqual(cur.getField(7,8),"2008-01-01")
	assertEqual(cur.getField(7,9),"08:00:00")
	assertEqual(cur.getField(7,10),"2008-01-01 08:00:00")
	assertEqual(cur.getField(7,11),2008)
	assertEqual(cur.getField(7,12),"char8")
	assertEqual(cur.getField(7,13),btos("text8"))
	assertEqual(cur.getField(7,14),"varchar8")
	assertEqual(cur.getField(7,15),btos("tinytext8"))
	assertEqual(cur.getField(7,16),btos("mediumtext8"))
	assertEqual(cur.getField(7,17),btos("longtext8"))
	print()


	# field lengths by index
	print("FIELD LENGTHS BY INDEX: ")
	assertEqual(cur.getFieldLength(0,0),1)
	assertEqual(cur.getFieldLength(0,1),1)
	assertEqual(cur.getFieldLength(0,2),1)
	assertEqual(cur.getFieldLength(0,3),1)
	assertEqual(cur.getFieldLength(0,4),1)
	#assertEqual(cur.getFieldLength(0,5),3)
	assertEqual(cur.getFieldLength(0,6),3)
	assertEqual(cur.getFieldLength(0,7),3)
	assertEqual(cur.getFieldLength(0,8),10)
	assertEqual(cur.getFieldLength(0,9),8)
	assertEqual(cur.getFieldLength(0,10),19)
	assertEqual(cur.getFieldLength(0,11),4)
	assertEqual(cur.getFieldLength(0,12),5)
	assertEqual(cur.getFieldLength(0,13),5)
	assertEqual(cur.getFieldLength(0,14),8)
	assertEqual(cur.getFieldLength(0,15),9)
	assertEqual(cur.getFieldLength(0,16),11)
	assertEqual(cur.getFieldLength(0,17),9)
	print()
	assertEqual(cur.getFieldLength(7,0),1)
	assertEqual(cur.getFieldLength(7,1),1)
	assertEqual(cur.getFieldLength(7,2),1)
	assertEqual(cur.getFieldLength(7,3),1)
	assertEqual(cur.getFieldLength(7,4),1)
	#assertEqual(cur.getFieldLength(7,5),3)
	assertEqual(cur.getFieldLength(7,6),3)
	assertEqual(cur.getFieldLength(7,7),3)
	assertEqual(cur.getFieldLength(7,8),10)
	assertEqual(cur.getFieldLength(7,9),8)
	assertEqual(cur.getFieldLength(7,10),19)
	assertEqual(cur.getFieldLength(7,11),4)
	assertEqual(cur.getFieldLength(7,12),5)
	assertEqual(cur.getFieldLength(7,13),5)
	assertEqual(cur.getFieldLength(7,14),8)
	assertEqual(cur.getFieldLength(7,15),9)
	assertEqual(cur.getFieldLength(7,16),11)
	assertEqual(cur.getFieldLength(7,17),9)
	print()


	# fields by name
	print("FIELDS BY NAME: ")
	assertEqual(cur.getField(0,"testtinyint"),1)
	assertEqual(cur.getField(0,"testsmallint"),1)
	assertEqual(cur.getField(0,"testmediumint"),1)
	assertEqual(cur.getField(0,"testint"),1)
	assertEqual(cur.getField(0,"testbigint"),1)
	#assertEqual(cur.getField(0,"testfloat"),Decimal("1.1"))
	#assertEqual(cur.getField(0,"testreal"),Decimal("1.1"))
	assertEqual(cur.getField(0,"testdecimal"),Decimal("1.1"))
	assertEqual(cur.getField(0,"testdate"),"2001-01-01")
	assertEqual(cur.getField(0,"testtime"),"01:00:00")
	assertEqual(cur.getField(0,"testdatetime"),"2001-01-01 01:00:00")
	assertEqual(cur.getField(0,"testyear"),2001)
	assertEqual(cur.getField(0,"testchar"),"char1")
	assertEqual(cur.getField(0,"testtext"),btos("text1"))
	assertEqual(cur.getField(0,"testvarchar"),"varchar1")
	assertEqual(cur.getField(0,"testtinytext"),btos("tinytext1"))
	assertEqual(cur.getField(0,"testmediumtext"),btos("mediumtext1"))
	assertEqual(cur.getField(0,"testlongtext"),btos("longtext1"))
	print()
	assertEqual(cur.getField(7,"testtinyint"),8)
	assertEqual(cur.getField(7,"testsmallint"),8)
	assertEqual(cur.getField(7,"testmediumint"),8)
	assertEqual(cur.getField(7,"testint"),8)
	assertEqual(cur.getField(7,"testbigint"),8)
	#assertEqual(cur.getField(7,"testfloat"),Decimal("8.1"))
	#assertEqual(cur.getField(7,"testreal"),Decimal("8.1"))
	assertEqual(cur.getField(7,"testdecimal"),Decimal("8.1"))
	assertEqual(cur.getField(7,"testdate"),"2008-01-01")
	assertEqual(cur.getField(7,"testtime"),"08:00:00")
	assertEqual(cur.getField(7,"testdatetime"),"2008-01-01 08:00:00")
	assertEqual(cur.getField(7,"testyear"),2008)
	assertEqual(cur.getField(7,"testchar"),"char8")
	assertEqual(cur.getField(7,"testtext"),btos("text8"))
	assertEqual(cur.getField(7,"testvarchar"),"varchar8")
	assertEqual(cur.getField(7,"testtinytext"),btos("tinytext8"))
	assertEqual(cur.getField(7,"testmediumtext"),btos("mediumtext8"))
	assertEqual(cur.getField(7,"testlongtext"),btos("longtext8"))
	print()


	# field lengths by name
	print("FIELD LENGTHS BY NAME: ")
	assertEqual(cur.getFieldLength(0,"testtinyint"),1)
	assertEqual(cur.getFieldLength(0,"testsmallint"),1)
	assertEqual(cur.getFieldLength(0,"testmediumint"),1)
	assertEqual(cur.getFieldLength(0,"testint"),1)
	assertEqual(cur.getFieldLength(0,"testbigint"),1)
	#assertEqual(cur.getFieldLength(0,"testfloat"),3)
	assertEqual(cur.getFieldLength(0,"testreal"),3)
	assertEqual(cur.getFieldLength(0,"testdecimal"),3)
	assertEqual(cur.getFieldLength(0,"testdate"),10)
	assertEqual(cur.getFieldLength(0,"testtime"),8)
	assertEqual(cur.getFieldLength(0,"testdatetime"),19)
	assertEqual(cur.getFieldLength(0,"testyear"),4)
	assertEqual(cur.getFieldLength(0,"testchar"),5)
	assertEqual(cur.getFieldLength(0,"testtext"),5)
	assertEqual(cur.getFieldLength(0,"testvarchar"),8)
	assertEqual(cur.getFieldLength(0,"testtinytext"),9)
	assertEqual(cur.getFieldLength(0,"testmediumtext"),11)
	assertEqual(cur.getFieldLength(0,"testlongtext"),9)
	print()
	assertEqual(cur.getFieldLength(7,"testtinyint"),1)
	assertEqual(cur.getFieldLength(7,"testsmallint"),1)
	assertEqual(cur.getFieldLength(7,"testmediumint"),1)
	assertEqual(cur.getFieldLength(7,"testint"),1)
	assertEqual(cur.getFieldLength(7,"testbigint"),1)
	#assertEqual(cur.getFieldLength(7,"testfloat"),3)
	assertEqual(cur.getFieldLength(7,"testreal"),3)
	assertEqual(cur.getFieldLength(7,"testdecimal"),3)
	assertEqual(cur.getFieldLength(7,"testdate"),10)
	assertEqual(cur.getFieldLength(7,"testtime"),8)
	assertEqual(cur.getFieldLength(7,"testdatetime"),19)
	assertEqual(cur.getFieldLength(7,"testyear"),4)
	assertEqual(cur.getFieldLength(7,"testchar"),5)
	assertEqual(cur.getFieldLength(7,"testtext"),5)
	assertEqual(cur.getFieldLength(7,"testvarchar"),8)
	assertEqual(cur.getFieldLength(7,"testtinytext"),9)
	assertEqual(cur.getFieldLength(7,"testmediumtext"),11)
	assertEqual(cur.getFieldLength(7,"testlongtext"),9)
	print()


	# fields by array
	print("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEqual(fields[0],1)
	assertEqual(fields[1],1)
	assertEqual(fields[2],1)
	assertEqual(fields[3],1)
	assertEqual(fields[4],1)
	#assertEqual(fields[5],Decimal("1.1"))
	#assertEqual(fields[6],Decimal("1.1"))
	assertEqual(fields[7],Decimal("1.1"))
	assertEqual(fields[8],"2001-01-01")
	assertEqual(fields[9],"01:00:00")
	assertEqual(fields[10],"2001-01-01 01:00:00")
	assertEqual(fields[11],2001)
	assertEqual(fields[12],"char1")
	assertEqual(fields[13],btos("text1"))
	assertEqual(fields[14],"varchar1")
	assertEqual(fields[15],btos("tinytext1"))
	assertEqual(fields[16],btos("mediumtext1"))
	assertEqual(fields[17],btos("longtext1"))
	print()


	# field lengths by array
	print("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEqual(fieldlens[0],1)
	assertEqual(fieldlens[1],1)
	assertEqual(fieldlens[2],1)
	assertEqual(fieldlens[3],1)
	assertEqual(fieldlens[4],1)
	#assertEqual(fieldlens[5],3)
	assertEqual(fieldlens[6],3)
	assertEqual(fieldlens[7],3)
	assertEqual(fieldlens[8],10)
	assertEqual(fieldlens[9],8)
	assertEqual(fieldlens[10],19)
	assertEqual(fieldlens[11],4)
	assertEqual(fieldlens[12],5)
	assertEqual(fieldlens[13],5)
	assertEqual(fieldlens[14],8)
	assertEqual(fieldlens[15],9)
	assertEqual(fieldlens[16],11)
	assertEqual(fieldlens[17],9)
	print()


	# fields by dictionary
	print("FIELDS BY DICTIONARY: ")
	fields=cur.getRowDictionary(0)
	assertEqual(fields["testtinyint"],1)
	assertEqual(fields["testsmallint"],1)
	assertEqual(fields["testmediumint"],1)
	assertEqual(fields["testint"],1)
	assertEqual(fields["testbigint"],1)
	#assertEqual(fields["testfloat"],Decimal("1.1"))
	#assertEqual(fields["testreal"],Decimal("1.1"))
	assertEqual(fields["testdecimal"],Decimal("1.1"))
	assertEqual(fields["testdate"],"2001-01-01")
	assertEqual(fields["testtime"],"01:00:00")
	assertEqual(fields["testdatetime"],"2001-01-01 01:00:00")
	assertEqual(fields["testyear"],2001)
	assertEqual(fields["testchar"],"char1")
	assertEqual(fields["testtext"],btos("text1"))
	assertEqual(fields["testvarchar"],"varchar1")
	assertEqual(fields["testtinytext"],btos("tinytext1"))
	assertEqual(fields["testmediumtext"],btos("mediumtext1"))
	assertEqual(fields["testlongtext"],btos("longtext1"))
	print()
	fields=cur.getRowDictionary(7)
	assertEqual(fields["testtinyint"],8)
	assertEqual(fields["testsmallint"],8)
	assertEqual(fields["testmediumint"],8)
	assertEqual(fields["testint"],8)
	assertEqual(fields["testbigint"],8)
	#assertEqual(fields["testfloat"],Decimal("8.1"))
	#assertEqual(fields["testreal"],Decimal("8.1"))
	assertEqual(fields["testdecimal"],Decimal("8.1"))
	assertEqual(fields["testdate"],"2008-01-01")
	assertEqual(fields["testtime"],"08:00:00")
	assertEqual(fields["testdatetime"],"2008-01-01 08:00:00")
	assertEqual(fields["testyear"],2008)
	assertEqual(fields["testchar"],"char8")
	assertEqual(fields["testtext"],btos("text8"))
	assertEqual(fields["testvarchar"],"varchar8")
	assertEqual(fields["testtinytext"],btos("tinytext8"))
	assertEqual(fields["testmediumtext"],btos("mediumtext8"))
	assertEqual(fields["testlongtext"],btos("longtext8"))
	print()


	# field lengths by dictionary
	print("FIELD LENGTHS BY DICTIONARY: ")
	fieldlengths=cur.getRowLengthsDictionary(0)
	assertEqual(fieldlengths["testtinyint"],1)
	assertEqual(fieldlengths["testsmallint"],1)
	assertEqual(fieldlengths["testmediumint"],1)
	assertEqual(fieldlengths["testint"],1)
	assertEqual(fieldlengths["testbigint"],1)
	#assertEqual(fieldlengths["testfloat"],3)
	assertEqual(fieldlengths["testreal"],3)
	assertEqual(fieldlengths["testdecimal"],3)
	assertEqual(fieldlengths["testdate"],10)
	assertEqual(fieldlengths["testtime"],8)
	assertEqual(fieldlengths["testdatetime"],19)
	assertEqual(fieldlengths["testyear"],4)
	assertEqual(fieldlengths["testchar"],5)
	assertEqual(fieldlengths["testtext"],5)
	assertEqual(fieldlengths["testvarchar"],8)
	assertEqual(fieldlengths["testtinytext"],9)
	assertEqual(fieldlengths["testmediumtext"],11)
	assertEqual(fieldlengths["testlongtext"],9)
	print()
	fieldlengths=cur.getRowLengthsDictionary(7)
	assertEqual(fieldlengths["testtinyint"],1)
	assertEqual(fieldlengths["testsmallint"],1)
	assertEqual(fieldlengths["testmediumint"],1)
	assertEqual(fieldlengths["testint"],1)
	assertEqual(fieldlengths["testbigint"],1)
	#assertEqual(fieldlengths["testfloat"],3)
	assertEqual(fieldlengths["testreal"],3)
	assertEqual(fieldlengths["testdecimal"],3)
	assertEqual(fieldlengths["testdate"],10)
	assertEqual(fieldlengths["testtime"],8)
	assertEqual(fieldlengths["testdatetime"],19)
	assertEqual(fieldlengths["testyear"],4)
	assertEqual(fieldlengths["testchar"],5)
	assertEqual(fieldlengths["testtext"],5)
	assertEqual(fieldlengths["testvarchar"],8)
	assertEqual(fieldlengths["testtinytext"],9)
	assertEqual(fieldlengths["testmediumtext"],11)
	assertEqual(fieldlengths["testlongtext"],9)
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
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
	cur.setResultSetBufferSize(0)
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
		"	testtinyint "))
	assertEqual(cur.getColumnName(0),None)
	assertEqual(cur.getColumnLength(0),0)
	assertEqual(cur.getColumnType(0),None)
	print()
	cur.getColumnInfo()
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
	assertEqual(cur.getColumnName(0),"testtinyint")
	assertEqual(cur.getColumnLength(0),1)
	assertEqual(cur.getColumnType(0),"TINYINT")
	print()


	# suspended session
	print("SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
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
		"	testtinyint "))
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
		"	testtinyint "))
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
		"	testtinyint "))
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
		"	testtinyint "))
	filename=cur.getCacheFileName()
	assertEqual(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEqual(cur.getField(7,0),8)
	print()


	# column count for cached result set
	print("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEqual(cur.colCount(),19)
	print()


	# column names for cached result set
	print("COLUMN NAMES FOR CACHED RESULT SET: ")
	assertEqual(cur.getColumnName(0),"testtinyint")
	assertEqual(cur.getColumnName(1),"testsmallint")
	assertEqual(cur.getColumnName(2),"testmediumint")
	assertEqual(cur.getColumnName(3),"testint")
	assertEqual(cur.getColumnName(4),"testbigint")
	assertEqual(cur.getColumnName(5),"testfloat")
	assertEqual(cur.getColumnName(6),"testreal")
	assertEqual(cur.getColumnName(7),"testdecimal")
	assertEqual(cur.getColumnName(8),"testdate")
	assertEqual(cur.getColumnName(9),"testtime")
	assertEqual(cur.getColumnName(10),"testdatetime")
	assertEqual(cur.getColumnName(11),"testyear")
	assertEqual(cur.getColumnName(12),"testchar")
	assertEqual(cur.getColumnName(13),"testtext")
	assertEqual(cur.getColumnName(14),"testvarchar")
	assertEqual(cur.getColumnName(15),"testtinytext")
	assertEqual(cur.getColumnName(16),"testmediumtext")
	assertEqual(cur.getColumnName(17),"testlongtext")
	cols=cur.getColumnNames()
	assertEqual(cols[0],"testtinyint")
	assertEqual(cols[1],"testsmallint")
	assertEqual(cols[2],"testmediumint")
	assertEqual(cols[3],"testint")
	assertEqual(cols[4],"testbigint")
	assertEqual(cols[5],"testfloat")
	assertEqual(cols[6],"testreal")
	assertEqual(cols[7],"testdecimal")
	assertEqual(cols[8],"testdate")
	assertEqual(cols[9],"testtime")
	assertEqual(cols[10],"testdatetime")
	assertEqual(cols[11],"testyear")
	assertEqual(cols[12],"testchar")
	assertEqual(cols[13],"testtext")
	assertEqual(cols[14],"testvarchar")
	assertEqual(cols[15],"testtinytext")
	assertEqual(cols[16],"testmediumtext")
	assertEqual(cols[17],"testlongtext")
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
		"	testtinyint "))
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
		"	testtinyint "))
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
	# Note: Mysql's default isolation level is repeatable-read,
	# not read-committed like most other db's.  Both sessions must
	# commit to see the changes that each other has made.
	secondcon=PySQLRClient.sqlrconnection("sqlrelay",9000,
						"/tmp/test.socket",
						"testuser","testpassword")
	secondcur=PySQLRClient.sqlrcursor(secondcon)
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	if majorversion>3:
		assertEqual(secondcur.getField(0,0),0)
	else:
		assertEqual(secondcur.getField(0,0),8)
	assertTrue(con.commit())
	assertEqual(secondcon.commit(),1)
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEqual(secondcur.getField(0,0),8)
	assertTrue(con.autoCommitOn())
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10, "
		"	10, "
		"	10, "
		"	10, "
		"	10.1, "
		"	10.1, "
		"	1.1, "
		"	'2010-01-01', "
		"	'10:00:00', "
		"	'2010-01-01 10:00:00', "
		"	'2010', "
		"	'char10', "
		"	'text10', "
		"	'varchar10', "
		"	'tinytext10', "
		"	'mediumtext10', "
		"	'longtext10', "
		"	NULL)"))
	assertEqual(secondcon.commit(),1)
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEqual(secondcur.getField(0,0),9)
	assertTrue(con.autoCommitOff())
	secondcon.commit()
	print()


	# row range
	print("ROW RANGE:")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
	print()
	rows=cur.getRowRange(0,5)
	assertEqual(rows[0][0],1)
	assertEqual(rows[0][1],1)
	assertEqual(rows[0][2],1)
	assertEqual(rows[0][3],1)
	assertEqual(rows[0][4],1)
	#assertEqual(rows[0][5],Decimal("1.1"))
	#assertEqual(rows[0][6],Decimal("1.1"))
	assertEqual(rows[0][7],Decimal("1.1"))
	assertEqual(rows[0][8],"2001-01-01")
	assertEqual(rows[0][9],"01:00:00")
	assertEqual(rows[0][10],"2001-01-01 01:00:00")
	assertEqual(rows[0][11],2001)
	assertEqual(rows[0][12],"char1")
	assertEqual(rows[0][13],btos("text1"))
	assertEqual(rows[0][14],"varchar1")
	assertEqual(rows[0][15],btos("tinytext1"))
	assertEqual(rows[0][16],btos("mediumtext1"))
	assertEqual(rows[0][17],btos("longtext1"))
	print()
	assertEqual(rows[1][0],2)
	assertEqual(rows[1][1],2)
	assertEqual(rows[1][2],2)
	assertEqual(rows[1][3],2)
	assertEqual(rows[1][4],2)
	#assertEqual(rows[1][5],Decimal("2.1"))
	#assertEqual(rows[1][6],Decimal("2.1"))
	assertEqual(rows[1][7],Decimal("2.1"))
	assertEqual(rows[1][8],"2002-01-01")
	assertEqual(rows[1][9],"02:00:00")
	assertEqual(rows[1][10],"2002-01-01 02:00:00")
	assertEqual(rows[1][11],2002)
	assertEqual(rows[1][12],"char2")
	assertEqual(rows[1][13],btos("text2"))
	assertEqual(rows[1][14],"varchar2")
	assertEqual(rows[1][15],btos("tinytext2"))
	assertEqual(rows[1][16],btos("mediumtext2"))
	assertEqual(rows[1][17],btos("longtext2"))
	print()
	assertEqual(rows[2][0],3)
	assertEqual(rows[2][1],3)
	assertEqual(rows[2][2],3)
	assertEqual(rows[2][3],3)
	assertEqual(rows[2][4],3)
	#assertEqual(rows[2][5],Decimal("3.1"))
	#assertEqual(rows[2][6],Decimal("3.1"))
	assertEqual(rows[2][7],Decimal("3.1"))
	assertEqual(rows[2][8],"2003-01-01")
	assertEqual(rows[2][9],"03:00:00")
	assertEqual(rows[2][10],"2003-01-01 03:00:00")
	assertEqual(rows[2][11],2003)
	assertEqual(rows[2][12],"char3")
	assertEqual(rows[2][13],btos("text3"))
	assertEqual(rows[2][14],"varchar3")
	assertEqual(rows[2][15],btos("tinytext3"))
	assertEqual(rows[2][16],btos("mediumtext3"))
	assertEqual(rows[2][17],btos("longtext3"))
	print()
	assertEqual(rows[3][0],4)
	assertEqual(rows[3][1],4)
	assertEqual(rows[3][2],4)
	assertEqual(rows[3][3],4)
	assertEqual(rows[3][4],4)
	#assertEqual(rows[3][5],Decimal("4.1"))
	#assertEqual(rows[3][6],Decimal("4.1"))
	assertEqual(rows[3][7],Decimal("4.1"))
	assertEqual(rows[3][8],"2004-01-01")
	assertEqual(rows[3][9],"04:00:00")
	assertEqual(rows[3][10],"2004-01-01 04:00:00")
	assertEqual(rows[3][11],2004)
	assertEqual(rows[3][12],"char4")
	assertEqual(rows[3][13],btos("text4"))
	assertEqual(rows[3][14],"varchar4")
	assertEqual(rows[3][15],btos("tinytext4"))
	assertEqual(rows[3][16],btos("mediumtext4"))
	assertEqual(rows[3][17],btos("longtext4"))
	print()
	assertEqual(rows[4][0],5)
	assertEqual(rows[4][1],5)
	assertEqual(rows[4][2],5)
	assertEqual(rows[4][3],5)
	assertEqual(rows[4][4],5)
	#assertEqual(rows[4][5],Decimal("5.1"))
	#assertEqual(rows[4][6],Decimal("5.1"))
	assertEqual(rows[4][7],Decimal("5.1"))
	assertEqual(rows[4][8],"2005-01-01")
	assertEqual(rows[4][9],"05:00:00")
	assertEqual(rows[4][10],"2005-01-01 05:00:00")
	assertEqual(rows[4][11],2005)
	assertEqual(rows[4][12],"char5")
	assertEqual(rows[4][13],btos("text5"))
	assertEqual(rows[4][14],"varchar5")
	assertEqual(rows[4][15],btos("tinytext5"))
	assertEqual(rows[4][16],btos("mediumtext5"))
	assertEqual(rows[4][17],btos("longtext5"))
	print()
	assertEqual(rows[5][0],6)
	assertEqual(rows[5][1],6)
	assertEqual(rows[5][2],6)
	assertEqual(rows[5][3],6)
	assertEqual(rows[5][4],6)
	#assertEqual(rows[5][5],Decimal("6.1"))
	#assertEqual(rows[5][6],Decimal("6.1"))
	assertEqual(rows[5][7],Decimal("6.1"))
	assertEqual(rows[5][8],"2006-01-01")
	assertEqual(rows[5][9],"06:00:00")
	assertEqual(rows[5][10],"2006-01-01 06:00:00")
	assertEqual(rows[5][11],2006)
	assertEqual(rows[5][12],"char6")
	assertEqual(rows[5][13],btos("text6"))
	assertEqual(rows[5][14],"varchar6")
	assertEqual(rows[5][15],btos("tinytext6"))
	assertEqual(rows[5][16],btos("mediumtext6"))
	assertEqual(rows[5][17],btos("longtext6"))
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
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
	assertFalse(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "))
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
