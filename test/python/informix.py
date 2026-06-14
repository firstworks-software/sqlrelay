#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.


from SQLRelay import PySQLRClient
import sys
from socket import gethostname
import asserts
from asserts import *


def main():

	isolationlevels=["committed read","dirty read",
					"cursor stability","repeatable read"]
	bindvars=["1","2","3","4",
					"5","6","7","8","9","10",
					"11","12","13","14","15","16"]
	bindvals=["t","7","7","7","7",
					"7.5","7.5","7.5","7.5",
					"testchar7","testnchar7",
					"testvarchar7","testnvarchar7",
					"testlvarchar7","01/01/2007",
					"2007-01-01 07:00:00"]
	subvars=["var1","var2","var3"]
	subvallongs=[1,2,3]
	subvalstrings=["hi","hello","bye"]
	subvaldoubles=[10.55,10.556,10.5556]
	precs=[4,5,6]
	scales=[2,3,4]
	counter=0

	LARGE_BUFFER_LENGTH=(20*1024)


	# hostname
	hostname=gethostname().split(".")[0]


	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
	cur=PySQLRClient.sqlrcursor(con)
	asserts.setConnection(con)
	asserts.setCursor(cur)


	# identify
	output("IDENTIFY: ")
	assertEquals(con.identify(),"informix")
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
	assertEquals(con.bindFormat(),"?")
	output()


	# nextval format
	output("NEXTVAL FORMAT: ")
	assertEquals(con.nextvalFormat(),"%s.nextval")
	output()


	# isolation levels
	output("ISOLATION LEVELS: ")
	for il in isolationlevels:
		# you can set the isolation level, but to get it, you have to
		# have permissions to read from sysmaster:syssqlcurses
		assertTrue(con.setIsolationLevel(il))
		output()
	# reset to the default isolation level
	assertTrue(con.setIsolationLevel(isolationlevels[0]))
	output()


	# create testtable
	output("CREATE TESTTABLE: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testboolean boolean, "
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testint8 int8, "
		"	testdecimal decimal(10,2), "
		"	testmoney money, "
		"	testsmallfloat smallfloat, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testnchar nchar(40), "
		"	testvarchar varchar(40), "
		"	testnvarchar nvarchar(40), "
		"	testlvarchar lvarchar(40), "
		"	testdate date, "
		"	testdatetime datetime year to second, "
		"	testtext text, "
		"	testbyte byte)"))
	assertTrue(con.commit())
	output()


	# insert
	output("INSERT: ")
	assertTrue(cur.sendQuery(
		"insert into "
		"	testtable "
		"values ("
		"	't', "
		"	1, "
		"	1, "
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'testchar1', "
		"	'testnchar1', "
		"	'testvarchar1', "
		"	'testnvarchar1', "
		"	'testlvarchar1', "
		"	'01/01/2001', "
		"	'2001-01-01 01:00:00', "
		"	'testtext1', "
		"	null)"))
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
		"	?)")
	assertEquals(cur.countBindVariables(),18)
	cur.inputBind("1","t")
	cur.inputBind("2",2)
	cur.inputBind("3",2)
	cur.inputBind("4",2)
	cur.inputBind("5",2)
	cur.inputBind("6",2.5,4,2)
	cur.inputBind("7",2.5,4,2)
	cur.inputBind("8",2.5,4,2)
	cur.inputBind("9",2.5,4,2)
	cur.inputBind("10","testchar2")
	cur.inputBind("11","testnchar2")
	cur.inputBind("12","testvarchar2")
	cur.inputBind("13","testnvarchar2")
	cur.inputBind("14","testlvarchar2")
	cur.inputBindDate("15",2002,1,1,-1,-1,-1,-1,None,False)
	cur.inputBindDate("16",2002,1,1,2,0,0,0,None,False)
	cur.inputBindClob("17","testtext2",9)
	cur.inputBindBlob("18","testbyte2",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1","t")
	cur.inputBind("2",3)
	cur.inputBind("3",3)
	cur.inputBind("4",3)
	cur.inputBind("5",3)
	cur.inputBind("6",3.5,4,2)
	cur.inputBind("7",3.5,4,2)
	cur.inputBind("8",3.5,4,2)
	cur.inputBind("9",3.5,4,2)
	cur.inputBind("10","testchar3")
	cur.inputBind("11","testnchar3")
	cur.inputBind("12","testvarchar3")
	cur.inputBind("13","testnvarchar3")
	cur.inputBind("14","testlvarchar3")
	cur.inputBindDate("15",2003,1,1,-1,-1,-1,-1,None,False)
	cur.inputBindDate("16",2003,1,1,3,0,0,0,None,False)
	cur.inputBindClob("17","testtext3",9)
	cur.inputBindBlob("18","testbyte3",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1","t")
	cur.inputBind("2",4)
	cur.inputBind("3",4)
	cur.inputBind("4",4)
	cur.inputBind("5",4)
	cur.inputBind("6",4.5,4,2)
	cur.inputBind("7",4.5,4,2)
	cur.inputBind("8",4.5,4,2)
	cur.inputBind("9",4.5,4,2)
	cur.inputBind("10","testchar4")
	cur.inputBind("11","testnchar4")
	cur.inputBind("12","testvarchar4")
	cur.inputBind("13","testnvarchar4")
	cur.inputBind("14","testlvarchar4")
	cur.inputBindDate("15",2004,1,1,-1,-1,-1,-1,None,False)
	cur.inputBindDate("16",2004,1,1,4,0,0,0,None,False)
	cur.inputBindClob("17","testtext4",9)
	cur.inputBindBlob("18","testbyte4",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1","t")
	cur.inputBind("2",5)
	cur.inputBind("3",5)
	cur.inputBind("4",5)
	cur.inputBind("5",5)
	cur.inputBind("6",5.5,4,2)
	cur.inputBind("7",5.5,4,2)
	cur.inputBind("8",5.5,4,2)
	cur.inputBind("9",5.5,4,2)
	cur.inputBind("10","testchar5")
	cur.inputBind("11","testnchar5")
	cur.inputBind("12","testvarchar5")
	cur.inputBind("13","testnvarchar5")
	cur.inputBind("14","testlvarchar5")
	cur.inputBindDate("15",2005,1,1,-1,-1,-1,-1,None,False)
	cur.inputBindDate("16",2005,1,1,5,0,0,0,None,False)
	cur.inputBindClob("17","testtext5",9)
	cur.inputBindBlob("18","testbyte5",9)
	assertTrue(cur.executeQuery())
	cur.clearBinds()
	cur.inputBind("1","t")
	cur.inputBind("2",6)
	cur.inputBind("3",6)
	cur.inputBind("4",6)
	cur.inputBind("5",6)
	cur.inputBind("6",6.5,4,2)
	cur.inputBind("7",6.5,4,2)
	cur.inputBind("8",6.5,4,2)
	cur.inputBind("9",6.5,4,2)
	cur.inputBind("10","testchar6")
	cur.inputBind("11","testnchar6")
	cur.inputBind("12","testvarchar6")
	cur.inputBind("13","testnvarchar6")
	cur.inputBind("14","testlvarchar6")
	cur.inputBindDate("15",2006,1,1,-1,-1,-1,-1,None,False)
	cur.inputBindDate("16",2006,1,1,6,0,0,0,None,False)
	cur.inputBindClob("17","testtext6",9)
	cur.inputBindBlob("18","testbyte6",9)
	assertTrue(cur.executeQuery())
	output()


	# array of input binds by position
	output("ARRAY OF INPUT BINDS BY POSITION: ")
	cur.clearBinds()
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
		"	null, "
		"	null)")
	cur.inputBinds(bindvars,bindvals)
	assertTrue(cur.executeQuery())
	output()


	# input bind by position with validation
	output("INPUT BIND BY POSITION WITH VALIDATION: ")
	cur.clearBinds()
	cur.inputBind("1","t")
	cur.inputBind("2",8)
	cur.inputBind("3",8)
	cur.inputBind("4",8)
	cur.inputBind("5",8)
	cur.inputBind("6",8.5,4,2)
	cur.inputBind("7",8.5,4,2)
	cur.inputBind("8",8.5,4,2)
	cur.inputBind("9",8.5,4,2)
	cur.inputBind("10","testchar8")
	cur.inputBind("11","testnchar8")
	cur.inputBind("12","testvarchar8")
	cur.inputBind("13","testnvarchar8")
	cur.inputBind("14","testlvarchar8")
	cur.inputBindDate("15",2008,1,1,-1,-1,-1,-1,None,False)
	cur.inputBindDate("16",2008,1,1,8,0,0,0,None,False)
	cur.inputBindClob("17","testtext8",9)
	cur.inputBindBlob("18","testbyte8",9)
	cur.validateBinds()
	assertTrue(cur.executeQuery())
	output()


	# input bind by name
	# informix doesn't support bind by name


	# array of input binds by name
	# informix doesn't support bind by name


	# input bind by name with validation
	# informix doesn't support bind by name


	# select
	output("SELECT: ")
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	output()


	# column count
	output("COLUMN COUNT: ")
	assertEquals(cur.colCount(),18)
	output()


	# column names
	output("COLUMN NAMES: ")
	assertEquals(cur.getColumnName(0),"testboolean")
	assertEquals(cur.getColumnName(1),"testsmallint")
	assertEquals(cur.getColumnName(2),"testint")
	assertEquals(cur.getColumnName(3),"testbigint")
	assertEquals(cur.getColumnName(4),"testint8")
	assertEquals(cur.getColumnName(5),"testdecimal")
	assertEquals(cur.getColumnName(6),"testmoney")
	assertEquals(cur.getColumnName(7),"testsmallfloat")
	assertEquals(cur.getColumnName(8),"testfloat")
	assertEquals(cur.getColumnName(9),"testchar")
	assertEquals(cur.getColumnName(10),"testnchar")
	assertEquals(cur.getColumnName(11),"testvarchar")
	assertEquals(cur.getColumnName(12),"testnvarchar")
	assertEquals(cur.getColumnName(13),"testlvarchar")
	assertEquals(cur.getColumnName(14),"testdate")
	assertEquals(cur.getColumnName(15),"testdatetime")
	assertEquals(cur.getColumnName(16),"testtext")
	assertEquals(cur.getColumnName(17),"testbyte")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"testboolean")
	assertEquals(cols[1],"testsmallint")
	assertEquals(cols[2],"testint")
	assertEquals(cols[3],"testbigint")
	assertEquals(cols[4],"testint8")
	assertEquals(cols[5],"testdecimal")
	assertEquals(cols[6],"testmoney")
	assertEquals(cols[7],"testsmallfloat")
	assertEquals(cols[8],"testfloat")
	assertEquals(cols[9],"testchar")
	assertEquals(cols[10],"testnchar")
	assertEquals(cols[11],"testvarchar")
	assertEquals(cols[12],"testnvarchar")
	assertEquals(cols[13],"testlvarchar")
	assertEquals(cols[14],"testdate")
	assertEquals(cols[15],"testdatetime")
	assertEquals(cols[16],"testtext")
	assertEquals(cols[17],"testbyte")
	output()


	# column types
	output("COLUMN TYPES: ")
	assertEquals(cur.getColumnType(0),"BOOLEAN")
	assertEquals(cur.getColumnType("testboolean"),"BOOLEAN")
	assertEquals(cur.getColumnType(1),"SMALLINT")
	assertEquals(cur.getColumnType("testsmallint"),"SMALLINT")
	assertEquals(cur.getColumnType(2),"INTEGER")
	assertEquals(cur.getColumnType("testint"),"INTEGER")
	assertEquals(cur.getColumnType(3),"BIGINT")
	assertEquals(cur.getColumnType("testbigint"),"BIGINT")
	assertEquals(cur.getColumnType(4),"INT8")
	assertEquals(cur.getColumnType("testint8"),"INT8")
	assertEquals(cur.getColumnType(5),"DECIMAL")
	assertEquals(cur.getColumnType("testdecimal"),"DECIMAL")
	assertEquals(cur.getColumnType(6),"MONEY")
	assertEquals(cur.getColumnType("testmoney"),"MONEY")
	assertEquals(cur.getColumnType(7),"SMALLFLOAT")
	assertEquals(cur.getColumnType("testsmallfloat"),"SMALLFLOAT")
	assertEquals(cur.getColumnType(8),"FLOAT")
	assertEquals(cur.getColumnType("testfloat"),"FLOAT")
	assertEquals(cur.getColumnType(9),"CHAR")
	assertEquals(cur.getColumnType("testchar"),"CHAR")
	# informix reports nchar as char, with no way to tell them apart
	assertEquals(cur.getColumnType(10),"CHAR")
	assertEquals(cur.getColumnType("testnchar"),"CHAR")
	assertEquals(cur.getColumnType(11),"VARCHAR")
	assertEquals(cur.getColumnType("testvarchar"),"VARCHAR")
	# informix reports nvarchar as varchar, with no way to tell them apart
	assertEquals(cur.getColumnType(12),"VARCHAR")
	assertEquals(cur.getColumnType("testnvarchar"),"VARCHAR")
	assertEquals(cur.getColumnType(13),"LVARCHAR")
	assertEquals(cur.getColumnType("testlvarchar"),"LVARCHAR")
	assertEquals(cur.getColumnType(14),"DATE")
	assertEquals(cur.getColumnType("testdate"),"DATE")
	assertEquals(cur.getColumnType(15),"DATETIME")
	assertEquals(cur.getColumnType("testdatetime"),"DATETIME")
	assertEquals(cur.getColumnType(16),"TEXT")
	assertEquals(cur.getColumnType("testtext"),"TEXT")
	assertEquals(cur.getColumnType(17),"BYTE")
	assertEquals(cur.getColumnType("testbyte"),"BYTE")
	output()


	# column length
	output("COLUMN LENGTH: ")
	assertEquals(cur.getColumnLength(0),1)
	assertEquals(cur.getColumnLength("testboolean"),1)
	assertEquals(cur.getColumnLength(1),5)
	assertEquals(cur.getColumnLength("testsmallint"),5)
	assertEquals(cur.getColumnLength(2),10)
	assertEquals(cur.getColumnLength("testint"),10)
	assertEquals(cur.getColumnLength(3),20)
	assertEquals(cur.getColumnLength("testbigint"),20)
	assertEquals(cur.getColumnLength(4),20)
	assertEquals(cur.getColumnLength("testint8"),20)
	assertEquals(cur.getColumnLength(5),10)
	assertEquals(cur.getColumnLength("testdecimal"),10)
	assertEquals(cur.getColumnLength(6),16)
	assertEquals(cur.getColumnLength("testmoney"),16)
	assertEquals(cur.getColumnLength(7),7)
	assertEquals(cur.getColumnLength("testsmallfloat"),7)
	assertEquals(cur.getColumnLength(8),15)
	assertEquals(cur.getColumnLength("testfloat"),15)
	assertEquals(cur.getColumnLength(9),40)
	assertEquals(cur.getColumnLength("testchar"),40)
	assertEquals(cur.getColumnLength(10),40)
	assertEquals(cur.getColumnLength("testnchar"),40)
	assertEquals(cur.getColumnLength(11),40)
	assertEquals(cur.getColumnLength("testvarchar"),40)
	assertEquals(cur.getColumnLength(12),40)
	assertEquals(cur.getColumnLength("testnvarchar"),40)
	assertEquals(cur.getColumnLength(13),40)
	assertEquals(cur.getColumnLength("testlvarchar"),40)
	assertEquals(cur.getColumnLength(14),10)
	assertEquals(cur.getColumnLength("testdate"),10)
	assertEquals(cur.getColumnLength(15),19)
	assertEquals(cur.getColumnLength("testdatetime"),19)
	assertEquals(cur.getColumnLength(16),2147483647)
	assertEquals(cur.getColumnLength("testtext"),2147483647)
	assertEquals(cur.getColumnLength(17),2147483647)
	assertEquals(cur.getColumnLength("testbyte"),2147483647)
	output()


	# longest column
	output("LONGEST COLUMN: ")
	assertEquals(cur.getLongest(0),1)
	assertEquals(cur.getLongest("testboolean"),1)
	assertEquals(cur.getLongest(1),1)
	assertEquals(cur.getLongest("testsmallint"),1)
	assertEquals(cur.getLongest(2),1)
	assertEquals(cur.getLongest("testint"),1)
	assertEquals(cur.getLongest(3),1)
	assertEquals(cur.getLongest("testbigint"),1)
	assertEquals(cur.getLongest(4),1)
	assertEquals(cur.getLongest("testint8"),1)
	assertEquals(cur.getLongest(5),4)
	assertEquals(cur.getLongest("testdecimal"),4)
	assertEquals(cur.getLongest(6),4)
	assertEquals(cur.getLongest("testmoney"),4)
	assertEquals(cur.getLongest(7),3)
	assertEquals(cur.getLongest("testsmallfloat"),3)
	assertEquals(cur.getLongest(8),3)
	assertEquals(cur.getLongest("testfloat"),3)
	assertEquals(cur.getLongest(9),40)
	assertEquals(cur.getLongest("testchar"),40)
	assertEquals(cur.getLongest(10),40)
	assertEquals(cur.getLongest("testnchar"),40)
	assertEquals(cur.getLongest(11),12)
	assertEquals(cur.getLongest("testvarchar"),12)
	assertEquals(cur.getLongest(12),13)
	assertEquals(cur.getLongest("testnvarchar"),13)
	assertEquals(cur.getLongest(13),13)
	assertEquals(cur.getLongest("testlvarchar"),13)
	assertEquals(cur.getLongest(14),10)
	assertEquals(cur.getLongest("testdate"),10)
	assertEquals(cur.getLongest(15),19)
	assertEquals(cur.getLongest("testdatetime"),19)
	assertEquals(cur.getLongest(16),9)
	assertEquals(cur.getLongest("testtext"),9)
	assertEquals(cur.getLongest(17),9)
	assertEquals(cur.getLongest("testbyte"),9)
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
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(0,2),"1")
	assertEquals(cur.getField(0,3),"1")
	assertEquals(cur.getField(0,4),"1")
	assertEquals(cur.getField(0,5),"1.50")
	assertEquals(cur.getField(0,6),"1.50")
	assertEquals(cur.getField(0,7),"1.5")
	assertEquals(cur.getField(0,8),"1.5")
	assertEquals(cur.getField(0,9),"testchar1                               ")
	assertEquals(cur.getField(0,10),"testnchar1                              ")
	assertEquals(cur.getField(0,11),"testvarchar1")
	assertEquals(cur.getField(0,12),"testnvarchar1")
	assertEquals(cur.getField(0,13),"testlvarchar1")
	assertEquals(cur.getField(0,14),"2001-01-01")
	assertEquals(cur.getField(0,15),"2001-01-01 01:00:00")
	assertEquals(cur.getField(0,16),"testtext1")
	assertEquals(cur.getField(0,17),"")
	output()
	assertEquals(cur.getField(7,0),"1")
	assertEquals(cur.getField(7,1),"8")
	assertEquals(cur.getField(7,2),"8")
	assertEquals(cur.getField(7,3),"8")
	assertEquals(cur.getField(7,4),"8")
	assertEquals(cur.getField(7,5),"8.50")
	assertEquals(cur.getField(7,6),"8.50")
	assertEquals(cur.getField(7,7),"8.5")
	assertEquals(cur.getField(7,8),"8.5")
	assertEquals(cur.getField(7,9),"testchar8                               ")
	assertEquals(cur.getField(7,10),"testnchar8                              ")
	assertEquals(cur.getField(7,11),"testvarchar8")
	assertEquals(cur.getField(7,12),"testnvarchar8")
	assertEquals(cur.getField(7,13),"testlvarchar8")
	assertEquals(cur.getField(7,14),"2008-01-01")
	assertEquals(cur.getField(7,15),"2008-01-01 08:00:00")
	assertEquals(cur.getField(7,16),"")
	assertEquals(cur.getField(7,17),"")
	output()


	# field lengths by index
	output("FIELD LENGTHS BY INDEX: ")
	assertEquals(cur.getFieldLength(0,0),1)
	assertEquals(cur.getFieldLength(0,1),1)
	assertEquals(cur.getFieldLength(0,2),1)
	assertEquals(cur.getFieldLength(0,3),1)
	assertEquals(cur.getFieldLength(0,4),1)
	assertEquals(cur.getFieldLength(0,5),4)
	assertEquals(cur.getFieldLength(0,6),4)
	assertEquals(cur.getFieldLength(0,7),3)
	assertEquals(cur.getFieldLength(0,8),3)
	assertEquals(cur.getFieldLength(0,9),40)
	assertEquals(cur.getFieldLength(0,10),40)
	assertEquals(cur.getFieldLength(0,11),12)
	assertEquals(cur.getFieldLength(0,12),13)
	assertEquals(cur.getFieldLength(0,14),10)
	assertEquals(cur.getFieldLength(0,15),19)
	assertEquals(cur.getFieldLength(0,16),9)
	assertEquals(cur.getFieldLength(0,17),0)
	output()
	assertEquals(cur.getFieldLength(7,0),1)
	assertEquals(cur.getFieldLength(7,1),1)
	assertEquals(cur.getFieldLength(7,2),1)
	assertEquals(cur.getFieldLength(7,3),1)
	assertEquals(cur.getFieldLength(7,4),1)
	assertEquals(cur.getFieldLength(7,5),4)
	assertEquals(cur.getFieldLength(7,6),4)
	assertEquals(cur.getFieldLength(7,7),3)
	assertEquals(cur.getFieldLength(7,8),3)
	assertEquals(cur.getFieldLength(7,9),40)
	assertEquals(cur.getFieldLength(7,10),40)
	assertEquals(cur.getFieldLength(7,11),12)
	assertEquals(cur.getFieldLength(7,12),13)
	assertEquals(cur.getFieldLength(7,14),10)
	assertEquals(cur.getFieldLength(7,15),19)
	assertEquals(cur.getFieldLength(7,16),0)
	assertEquals(cur.getFieldLength(7,17),0)
	output()


	# fields by name
	output("FIELDS BY NAME: ")
	assertEquals(cur.getField(0,"testboolean"),"1")
	assertEquals(cur.getField(0,"testsmallint"),"1")
	assertEquals(cur.getField(0,"testint"),"1")
	assertEquals(cur.getField(0,"testbigint"),"1")
	assertEquals(cur.getField(0,"testint8"),"1")
	assertEquals(cur.getField(0,"testdecimal"),"1.50")
	assertEquals(cur.getField(0,"testmoney"),"1.50")
	assertEquals(cur.getField(0,"testsmallfloat"),"1.5")
	assertEquals(cur.getField(0,"testfloat"),"1.5")
	assertEquals(cur.getField(0,"testchar"),"testchar1                               ")
	assertEquals(cur.getField(0,"testnchar"),"testnchar1                              ")
	assertEquals(cur.getField(0,"testvarchar"),"testvarchar1")
	assertEquals(cur.getField(0,"testnvarchar"),"testnvarchar1")
	assertEquals(cur.getField(0,"testlvarchar"),"testlvarchar1")
	assertEquals(cur.getField(0,"testdate"),"2001-01-01")
	assertEquals(cur.getField(0,"testdatetime"),"2001-01-01 01:00:00")
	assertEquals(cur.getField(0,"testtext"),"testtext1")
	assertEquals(cur.getField(0,"testbyte"),"")
	output()
	assertEquals(cur.getField(7,"testboolean"),"1")
	assertEquals(cur.getField(7,"testsmallint"),"8")
	assertEquals(cur.getField(7,"testint"),"8")
	assertEquals(cur.getField(7,"testbigint"),"8")
	assertEquals(cur.getField(7,"testint8"),"8")
	assertEquals(cur.getField(7,"testdecimal"),"8.50")
	assertEquals(cur.getField(7,"testmoney"),"8.50")
	assertEquals(cur.getField(7,"testsmallfloat"),"8.5")
	assertEquals(cur.getField(7,"testfloat"),"8.5")
	assertEquals(cur.getField(7,"testchar"),"testchar8                               ")
	assertEquals(cur.getField(7,"testnchar"),"testnchar8                              ")
	assertEquals(cur.getField(7,"testvarchar"),"testvarchar8")
	assertEquals(cur.getField(7,"testnvarchar"),"testnvarchar8")
	assertEquals(cur.getField(7,"testlvarchar"),"testlvarchar8")
	assertEquals(cur.getField(7,"testdate"),"2008-01-01")
	assertEquals(cur.getField(7,"testdatetime"),"2008-01-01 08:00:00")
	assertEquals(cur.getField(7,"testtext"),"")
	assertEquals(cur.getField(7,"testbyte"),"")
	output()


	# field lengths by name
	output("FIELD LENGTHS BY NAME: ")
	assertEquals(cur.getFieldLength(0,"testboolean"),1)
	assertEquals(cur.getFieldLength(0,"testsmallint"),1)
	assertEquals(cur.getFieldLength(0,"testint"),1)
	assertEquals(cur.getFieldLength(0,"testbigint"),1)
	assertEquals(cur.getFieldLength(0,"testint8"),1)
	assertEquals(cur.getFieldLength(0,"testdecimal"),4)
	assertEquals(cur.getFieldLength(0,"testmoney"),4)
	assertEquals(cur.getFieldLength(0,"testsmallfloat"),3)
	assertEquals(cur.getFieldLength(0,"testfloat"),3)
	assertEquals(cur.getFieldLength(0,"testchar"),40)
	assertEquals(cur.getFieldLength(0,"testnchar"),40)
	assertEquals(cur.getFieldLength(0,"testvarchar"),12)
	assertEquals(cur.getFieldLength(0,"testnvarchar"),13)
	assertEquals(cur.getFieldLength(0,"testlvarchar"),13)
	assertEquals(cur.getFieldLength(0,"testdate"),10)
	assertEquals(cur.getFieldLength(0,"testdatetime"),19)
	assertEquals(cur.getFieldLength(0,"testtext"),9)
	assertEquals(cur.getFieldLength(0,"testbyte"),0)
	output()
	assertEquals(cur.getFieldLength(7,"testboolean"),1)
	assertEquals(cur.getFieldLength(7,"testsmallint"),1)
	assertEquals(cur.getFieldLength(7,"testint"),1)
	assertEquals(cur.getFieldLength(7,"testbigint"),1)
	assertEquals(cur.getFieldLength(7,"testint8"),1)
	assertEquals(cur.getFieldLength(7,"testdecimal"),4)
	assertEquals(cur.getFieldLength(7,"testmoney"),4)
	assertEquals(cur.getFieldLength(7,"testsmallfloat"),3)
	assertEquals(cur.getFieldLength(7,"testfloat"),3)
	assertEquals(cur.getFieldLength(7,"testchar"),40)
	assertEquals(cur.getFieldLength(7,"testnchar"),40)
	assertEquals(cur.getFieldLength(7,"testvarchar"),12)
	assertEquals(cur.getFieldLength(7,"testnvarchar"),13)
	assertEquals(cur.getFieldLength(7,"testlvarchar"),13)
	assertEquals(cur.getFieldLength(7,"testdate"),10)
	assertEquals(cur.getFieldLength(7,"testdatetime"),19)
	assertEquals(cur.getFieldLength(7,"testtext"),0)
	assertEquals(cur.getFieldLength(7,"testbyte"),0)
	output()


	# fields by array
	output("FIELDS BY ARRAY: ")
	fields=cur.getRow(0)
	assertEquals(fields[0],"1")
	assertEquals(fields[1],"1")
	assertEquals(fields[2],"1")
	assertEquals(fields[3],"1")
	assertEquals(fields[4],"1")
	assertEquals(fields[5],"1.50")
	assertEquals(fields[6],"1.50")
	assertEquals(fields[7],"1.5")
	assertEquals(fields[8],"1.5")
	assertEquals(fields[9],"testchar1                               ")
	assertEquals(fields[10],"testnchar1                              ")
	assertEquals(fields[11],"testvarchar1")
	assertEquals(fields[12],"testnvarchar1")
	assertEquals(fields[13],"testlvarchar1")
	assertEquals(fields[14],"2001-01-01")
	assertEquals(fields[15],"2001-01-01 01:00:00")
	assertEquals(fields[16],"testtext1")
	assertEquals(fields[17],"")
	output()


	# field lengths by array
	output("FIELD LENGTHS BY ARRAY: ")
	fieldlens=cur.getRowLengths(0)
	assertEquals(fieldlens[0],1)
	assertEquals(fieldlens[1],1)
	assertEquals(fieldlens[2],1)
	assertEquals(fieldlens[3],1)
	assertEquals(fieldlens[4],1)
	assertEquals(fieldlens[5],4)
	assertEquals(fieldlens[6],4)
	assertEquals(fieldlens[7],3)
	assertEquals(fieldlens[8],3)
	assertEquals(fieldlens[9],40)
	assertEquals(fieldlens[10],40)
	assertEquals(fieldlens[11],12)
	assertEquals(fieldlens[12],13)
	assertEquals(fieldlens[14],10)
	assertEquals(fieldlens[15],19)
	assertEquals(fieldlens[16],9)
	assertEquals(fieldlens[17],0)
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
		"	testsmallint "))
	assertEquals(cur.getResultSetBufferSize(),2)
	output()
	assertEquals(cur.firstRowIndex(),0)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),2)
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(1,1),"2")
	assertEquals(cur.getField(2,1),"3")
	output()
	assertEquals(cur.firstRowIndex(),2)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),4)
	assertEquals(cur.getField(6,1),"7")
	assertEquals(cur.getField(7,1),"8")
	output()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,1))
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
		"	testsmallint "))
	assertNone(cur.getColumnName(1))
	assertEquals(cur.getColumnLength(1),0)
	assertNone(cur.getColumnType(1))
	cur.getColumnInfo()
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	assertEquals(cur.getColumnName(1),"testsmallint")
	assertEquals(cur.getColumnLength(1),5)
	assertEquals(cur.getColumnType(1),"SMALLINT")
	output()


	# suspended session
	output("SUSPENDED SESSION: ")
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
	output()
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(1,1),"2")
	assertEquals(cur.getField(2,1),"3")
	assertEquals(cur.getField(3,1),"4")
	assertEquals(cur.getField(4,1),"5")
	assertEquals(cur.getField(5,1),"6")
	assertEquals(cur.getField(6,1),"7")
	assertEquals(cur.getField(7,1),"8")
	output()
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
	output()
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(1,1),"2")
	assertEquals(cur.getField(2,1),"3")
	assertEquals(cur.getField(3,1),"4")
	assertEquals(cur.getField(4,1),"5")
	assertEquals(cur.getField(5,1),"6")
	assertEquals(cur.getField(6,1),"7")
	assertEquals(cur.getField(7,1),"8")
	output()
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
	output()
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(1,1),"2")
	assertEquals(cur.getField(2,1),"3")
	assertEquals(cur.getField(3,1),"4")
	assertEquals(cur.getField(4,1),"5")
	assertEquals(cur.getField(5,1),"6")
	assertEquals(cur.getField(6,1),"7")
	assertEquals(cur.getField(7,1),"8")
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
		"	testsmallint "))
	assertEquals(cur.getField(2,1),"3")
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
	assertEquals(cur.getField(7,1),"8")
	output()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,1))
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
	assertTrue(cur.sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "))
	filename=cur.getCacheFileName()
	assertEquals(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,1),"8")
	output()


	# column count for cached result set
	output("COLUMN COUNT FOR CACHED RESULT SET: ")
	assertEquals(cur.colCount(),18)
	output()


	# column names for cached result set
	output("COLUMN NAMES FOR CACHED RESULT SET: ")
	assertEquals(cur.getColumnName(0),"testboolean")
	assertEquals(cur.getColumnName(1),"testsmallint")
	assertEquals(cur.getColumnName(2),"testint")
	assertEquals(cur.getColumnName(3),"testbigint")
	assertEquals(cur.getColumnName(4),"testint8")
	assertEquals(cur.getColumnName(5),"testdecimal")
	assertEquals(cur.getColumnName(6),"testmoney")
	assertEquals(cur.getColumnName(7),"testsmallfloat")
	assertEquals(cur.getColumnName(8),"testfloat")
	assertEquals(cur.getColumnName(9),"testchar")
	assertEquals(cur.getColumnName(10),"testnchar")
	assertEquals(cur.getColumnName(11),"testvarchar")
	assertEquals(cur.getColumnName(12),"testnvarchar")
	assertEquals(cur.getColumnName(13),"testlvarchar")
	assertEquals(cur.getColumnName(14),"testdate")
	assertEquals(cur.getColumnName(15),"testdatetime")
	assertEquals(cur.getColumnName(16),"testtext")
	assertEquals(cur.getColumnName(17),"testbyte")
	cols=cur.getColumnNames()
	assertEquals(cols[0],"testboolean")
	assertEquals(cols[1],"testsmallint")
	assertEquals(cols[2],"testint")
	assertEquals(cols[3],"testbigint")
	assertEquals(cols[4],"testint8")
	assertEquals(cols[5],"testdecimal")
	assertEquals(cols[6],"testmoney")
	assertEquals(cols[7],"testsmallfloat")
	assertEquals(cols[8],"testfloat")
	assertEquals(cols[9],"testchar")
	assertEquals(cols[10],"testnchar")
	assertEquals(cols[11],"testvarchar")
	assertEquals(cols[12],"testnvarchar")
	assertEquals(cols[13],"testlvarchar")
	assertEquals(cols[14],"testdate")
	assertEquals(cols[15],"testdatetime")
	assertEquals(cols[16],"testtext")
	assertEquals(cols[17],"testbyte")
	output()


	# cached result set with result set buffer size
	output("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ")
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
	assertEquals(filename,"cachefile1")
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,1),"8")
	assertNone(cur.getField(8,1))
	cur.setResultSetBufferSize(0)
	output()


	# from one cache file to another
	output("FROM ONE CACHE FILE TO ANOTHER: ")
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEquals(cur.getField(7,1),"8")
	assertNone(cur.getField(8,1))
	output()


	# from one cache file to another with result set buffer size
	output("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: ")
	cur.setResultSetBufferSize(2)
	cur.cacheToFile("cachefile2")
	assertTrue(cur.openCachedResultSet("cachefile1"))
	cur.cacheOff()
	assertTrue(cur.openCachedResultSet("cachefile2"))
	assertEquals(cur.getField(7,1),"8")
	assertNone(cur.getField(8,1))
	cur.setResultSetBufferSize(0)
	output()


	# cached result set with suspend and result set buffer size
	output("CACHED RESULT SET WITH SUSPEND "
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
	assertEquals(cur.getField(2,1),"3")
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
	assertEquals(cur.getField(7,1),"8")
	output()
	assertEquals(cur.firstRowIndex(),6)
	assertFalse(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	assertNone(cur.getField(8,1))
	output()
	assertEquals(cur.firstRowIndex(),8)
	assertTrue(cur.endOfResultSet())
	assertEquals(cur.rowCount(),8)
	cur.cacheOff()
	output()
	assertTrue(cur.openCachedResultSet(filename))
	assertEquals(cur.getField(7,1),"8")
	assertNone(cur.getField(8,1))
	cur.setResultSetBufferSize(0)
	output()


	# finished suspended session
	output("FINISHED SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEquals(cur.getField(4,1),"5")
	assertEquals(cur.getField(5,1),"6")
	assertEquals(cur.getField(6,1),"7")
	assertEquals(cur.getField(7,1),"8")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeResultSet(id))
	assertNone(cur.getField(4,1))
	assertNone(cur.getField(5,1))
	assertNone(cur.getField(6,1))
	assertNone(cur.getField(7,1))
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
	# Informix has no MVCC option -- the isolation level is either dirty
	# reads (where the second connection sees uncommitted rows) or
	# committed read (where it blocks or errors on locked rows) -- so
	# the visibility assertions below may need to be revisited
	output("TRANSACTION BEHAVIOR - implicit: ")
	assertTrue(con.setTransactionModel("implicit"))
	assertEquals(con.getTransactionModel(),"implicit")
	assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
	# informix DDL is transactional in logged mode; commit so the table
	# is visible to the second connection (commit implicitly starts a
	# new tx)
	assertTrue(con.commit())
	secondcon=PySQLRClient.sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
	secondcur=PySQLRClient.sqlrcursor(secondcon)
	asserts.setSecondConnection(secondcon)
	asserts.setSecondCursor(secondcur)
	# Informix has no MVCC; under default committed-read isolation,
	# secondcur's catalog/data read errors with "Cannot get system
	# information for table" while cur holds row locks from the
	# in-flight tx.  Use dirty-read on secondcur so it sees the
	# uncommitted writes - the test then verifies dirty-read
	# semantics instead of MVCC visibility.
	assertTrue(secondcur.sendQuery("set isolation to dirty read"))
	# session is in a transaction; insert is visible via dirty read
	assertTrue(con.getInTransaction())
	assertFalse(con.getAutoCommit())
	assertTrue(cur.sendQuery("insert into testtable values (1)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
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
	# see note above re: informix dirty-read workaround
	assertTrue(secondcur.sendQuery("set isolation to dirty read"))
	# begin starts a new transaction; insert is visible via dirty read
	assertTrue(con.begin())
	assertTrue(con.getInTransaction())
	assertTrue(cur.sendQuery("insert into testtable values (1)"))
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"1")
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
	# see note in - implicit section re: informix dirty-read workaround
	assertTrue(secondcur.sendQuery("set isolation to dirty read"))
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
	# explicitly commits/rollbacks the tx (mysql-native semantic).
	# dirty-read on secondcur sees the in-flight insert (count=2)
	assertTrue(con.begin())
	assertTrue(cur.sendQuery("insert into testtable values (3)"))
	assertTrue(con.autoCommitOn())
	assertFalse(con.getAutoCommit())
	assertTrue(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"2")
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
	# dirty-read on secondcur sees the in-flight insert (count=5)
	assertTrue(con.autoCommitOn())
	assertTrue(con.getAutoCommit())
	assertTrue(con.begin())
	assertTrue(cur.sendQuery("insert into testtable values (7)"))
	assertTrue(con.autoCommitOff())
	assertFalse(con.getAutoCommit())
	assertTrue(con.getInTransaction())
	assertTrue(secondcur.sendQuery("select count(*) from testtable"))
	assertEquals(secondcur.getField(0,0),"5")
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
	cur.prepareQuery(
		"select "
		"	$(var1), "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	sysmaster:sysdual ")
	cur.substitution("var1",1)
	cur.substitution("var2","hello")
	cur.substitution("var3",10.5556,6,4)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"10.5556")
	output()


	# array substitutions
	output("ARRAY SUBSTITUTIONS: ")
	cur.prepareQuery(
		"select "
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	sysmaster:sysdual ")
	cur.substitutions(subvars,subvalstrings)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"hi")
	assertEquals(cur.getField(0,1),"hello")
	assertEquals(cur.getField(0,2),"bye")
	output()
	cur.prepareQuery(
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	sysmaster:sysdual ")
	cur.substitutions(subvars,subvallongs)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"1")
	assertEquals(cur.getField(0,1),"2")
	assertEquals(cur.getField(0,2),"3")
	output()
	cur.prepareQuery(
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	sysmaster:sysdual ")
	cur.substitutions(subvars,subvaldoubles,precs,scales)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getField(0,0),"10.55")
	assertEquals(cur.getField(0,1),"10.556")
	assertEquals(cur.getField(0,2),"10.5556")
	output()


	# nulls as nulls
	output("NULLS AS NULLS: ")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery(
		"select NULL::int,1,NULL::int from sysmaster:sysdual"))
	assertNone(cur.getField(0,0))
	assertEquals(cur.getField(0,1),"1")
	assertNone(cur.getField(0,2))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery(
		"select NULL::int,1,NULL::int from sysmaster:sysdual"))
	assertEquals(cur.getField(0,0),"")
	assertEquals(cur.getField(0,1),"1")
	assertEquals(cur.getField(0,2),"")
	output()


	# output bind by position
	output("OUTPUT BIND BY POSITION: ")
	cur.sendQuery("drop procedure testproc")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	out out1 int, "
		"	out out2 varchar(20), "
		"	out out3 float, "
		"	out out4 varchar(20)) "
		"let out1 = 1; "
		"	let out2 = 'hello'; "
		"	let out3 = 2.5; "
		"	let out4 = null; "
		"end procedure;"))
	assertTrue(con.commit())
	cur.prepareQuery("{call testproc(?,?,?,?)}")
	assertEquals(cur.countBindVariables(),4)
	cur.defineOutputBindInteger("1")
	cur.defineOutputBindString("2",20)
	cur.defineOutputBindDouble("3")
	cur.defineOutputBindString("4",20)
	assertTrue(cur.executeQuery())
	numvar=cur.getOutputBindInteger("1")
	stringvar=cur.getOutputBindString("2")
	floatvar=cur.getOutputBindDouble("3")
	nullvar=cur.getOutputBindString("4")
	assertEquals(numvar,1)
	assertEquals(stringvar,"hello")
	assertEquals(floatvar,2.5)
	assertNone(nullvar)
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(con.commit())
	output()


	# output bind by name
	# informix doesn't support bind by name


	# output bind by name with validation
	# informix doesn't support bind by name


	# lob output bind
	output("LOB OUTPUT BIND: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"))
	assertTrue(con.commit())
	cur.prepareQuery("insert into testtable values (?,?)")
	cur.inputBindClob("1","hello",5)
	cur.inputBindBlob("2","hello",5)
	assertTrue(cur.executeQuery())
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	out out1 clob, "
		"	out out2 blob) "
		"select testclob, testblob "
		"	into out1, out2 "
		"	from testtable; "
		"end procedure;"))
	assertTrue(con.commit())
	cur.prepareQuery("{call testproc(?,?)}")
	cur.defineOutputBindClob("1")
	cur.defineOutputBindBlob("2")
	assertTrue(cur.executeQuery())
	clobvar=cur.getOutputBindClob("1")
	clobvarlength=cur.getOutputBindLength("1")
	blobvar=cur.getOutputBindBlob("2")
	blobvarlength=cur.getOutputBindLength("2")
	assertEqualsBytes(clobvar,"hello",5)
	assertEquals(clobvarlength,5)
	# blob output bind comes back as bytes in Python
	assertEqualsBytes(blobvar,b"hello",5)
	assertEquals(blobvarlength,5)
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# long output bind
	output("LONG OUTPUT BIND: ")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	in1 clob, "
		"	out out1 clob) "
		"let out1 = in1; "
		"	end procedure;"))
	assertTrue(con.commit())
	cur.prepareQuery("{call testproc(?,?)}")
	largebuffer='C'*LARGE_BUFFER_LENGTH
	cur.inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH)
	cur.defineOutputBindClob("2")
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindLength("2"),LARGE_BUFFER_LENGTH)
	assertEquals(cur.getOutputBindClob("2"),largebuffer)
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(con.commit())
	output()


	# negative input bind
	output("NEGATIVE INPUT BIND: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery("create table testtable (testval int)")
	assertTrue(con.commit())
	cur.prepareQuery("insert into testtable values (?)")
	cur.inputBind("1",-1)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select testval from testtable")
	assertEquals(cur.getField(0,"testval"),"-1")
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# bind validation
	# informix doesn't support bind by name

	# rebinding
	output("REBINDING: ")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	in1 int, "
		"	out out1 int) "
		"let out1 = in1; "
		"end procedure;"))
	assertTrue(con.commit())
	cur.prepareQuery("{call testproc(?,?)}")
	cur.inputBind("1",1)
	cur.defineOutputBindInteger("2")
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("2"),1)
	cur.inputBind("1",2)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("2"),2)
	cur.inputBind("1",3)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("2"),3)
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(con.commit())
	output()


	# reexecute
	output("REEXECUTE: ")
	cur.prepareQuery("select 1 from sysmaster:sysdual")
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	cur.prepareQuery("select ?::int from sysmaster:sysdual")
	cur.inputBind("1",1)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"1")
	output()
	cur.inputBind("1",2)
	assertTrue(cur.executeQuery())
	assertEquals(cur.rowCount(),1)
	assertEquals(cur.getField(0,0),"2")
	output()


	# stored procedure returning no value
	output("STORED PROCEDURE RETURNING NO VALUE: ")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	in1 int, "
		"	in2 float, "
		"	in3 varchar(20)) "
		"end procedure;"))
	assertTrue(con.commit())
	cur.prepareQuery("{call testproc(?,?,?)}")
	cur.inputBind("1",1)
	cur.inputBind("2",2.5,2,1)
	cur.inputBind("3","hello")
	assertTrue(cur.executeQuery())
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(con.commit())
	output()


	# stored procedure returning single value
	output("STORED PROCEDURE RETURNING SINGLE VALUE: ")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	in1 int, "
		"	in2 float, "
		"	in3 varchar(20), "
		"	out out1 int) "
		"let out1 = in1; "
		"end procedure;"))
	assertTrue(con.commit())
	cur.prepareQuery("{call testproc(?,?,?,?)}")
	cur.inputBind("1",1)
	cur.inputBind("2",2.5,2,1)
	cur.inputBind("3","hello")
	cur.defineOutputBindInteger("4")
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("4"),1)
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(con.commit())
	output()


	# stored procedure returning multiple values
	output("STORED PROCEDURE RETURNING MULTIPLE VALUES: ")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("
		"	in1 int, "
		"	in2 float, "
		"	in3 varchar(20), "
		"	out out1 int, "
		"	out out2 float, "
		"	out out3 varchar(20)) "
		"let out1 = in1; "
		"	let out2 = in2; "
		"	let out3 = in3; "
		"end procedure;"))
	assertTrue(con.commit())
	cur.prepareQuery("{call testproc(?,?,?,?,?,?)}")
	cur.inputBind("1",1)
	cur.inputBind("2",2.5,2,1)
	cur.inputBind("3","hello")
	cur.defineOutputBindInteger("4")
	cur.defineOutputBindDouble("5")
	cur.defineOutputBindString("6",20)
	assertTrue(cur.executeQuery())
	assertEquals(cur.getOutputBindInteger("4"),1)
	assertEquals(cur.getOutputBindDouble("5"),2.5)
	assertEquals(cur.getOutputBindString("6"),"hello")
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(con.commit())
	output()


	# stored procedure returning result set
	output("STORED PROCEDURE RETURNING RESULT SET: ")
	cur.sendQuery("drop procedure testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc() "
		"returning boolean, smallint, varchar(40); "
		"	define out1 boolean; "
		"	define out2 smallint; "
		"	define out3 varchar(40); "
		"	foreach "
		"		select "
		"			testboolean, "
		"			testsmallint, "
		"			testvarchar "
		"		into out1,out2,out3 "
		"		from ( "
		"			select "
		"				't' as testboolean, "
		"				1 as testsmallint, "
		"				'1' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				2 as testsmallint, "
		"				'2' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				3 as testsmallint, "
		"				'3' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				4 as testsmallint, "
		"				'4' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				5 as testsmallint, "
		"				'5' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				6 as testsmallint, "
		"				'6' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				7 as testsmallint, "
		"				'7' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				8 as testsmallint, "
		"				'8' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"		) "
		"	return out1,out2,out3 "
		"	with resume; "
		"	end foreach; "
		"	end procedure;"))
	assertTrue(con.commit())
	assertTrue(cur.sendQuery("{call testproc()}"))
	assertEquals(cur.rowCount(),8)
	assertTrue(cur.sendQuery("drop procedure testproc"))
	assertTrue(con.commit())
	output()


	# null and empty lobs
	output("NULL AND EMPTY LOBS: ")
	cur.sendQuery("drop table testtable")
	cur.getNullsAsNone()
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)"))
	assertTrue(con.commit())
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
	# informix stores a single \0 byte for an empty-string clob/blob bind.
	# In C that looks like an empty string (strcmp stops at the null
	# terminator), but Python preserves the null byte, so assert on the raw
	# byte here.
	assertEquals(cur.getField(0,0),"\0")
	assertNone(cur.getField(0,1))
	# blob column comes back as bytes in Python
	assertEquals(cur.getField(0,2),b"\0")
	assertNone(cur.getField(0,3))
	cur.getNullsAsEmptyStrings()
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# long lobs
	output("LONG LOBS: ")
	cur.sendQuery("drop table testtable")
	cur.sendQuery(
		"create table testtable ("
		"	testtext text, "
		"	testbyte byte)")
	assertTrue(con.commit())
	cur.prepareQuery("insert into testtable values (?,?)")
	largebuffer='C'*LARGE_BUFFER_LENGTH
	cur.inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH)
	cur.inputBindBlob("2",largebuffer,LARGE_BUFFER_LENGTH)
	assertTrue(cur.executeQuery())
	cur.sendQuery("select * from testtable")
	assertEquals(cur.getFieldLength(0,"testtext"),LARGE_BUFFER_LENGTH)
	assertEquals(cur.getField(0,"testtext"),largebuffer)
	assertEquals(cur.getFieldLength(0,"testbyte"),LARGE_BUFFER_LENGTH)
	assertEqualsBytes(cur.getField(0,"testbyte"),largebuffer,
						LARGE_BUFFER_LENGTH)
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# temporary tables
	output("TEMPORARY TABLES: ")
	cur.sendQuery("drop table temptable")
	cur.sendQuery(
		"create temp table temptable (col1 int)")
	assertTrue(cur.sendQuery("insert into temptable values (1)"))
	assertTrue(cur.sendQuery("select count(*) from temptable"))
	assertEquals(cur.getField(0,0),"1")
	con.endSession()
	output()
	assertFalse(cur.sendQuery("select count(*) from temptable"))
	output()


	# encoded binary data
	# informix doesn't support encoded binary data


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
			"	(col1 serial primary key, "
			"	col2 int)"))
	assertTrue(cur.sendQuery(
			"insert into testtable (col2) values (1)"))
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
	assertInResultSet(cur,"Database",hostname)
	output()


	# schema list
	output("SCHEMA LIST: ")
	# informix requires that a table exist that is
	# owned by a user for the user to be reported
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 integer, "
		"	col2 integer)"))
	assertTrue(con.commit())
	assertTrue(cur.getSchemaList(None))
	assertEquals(cur.getColumnName(0),"Database")
	assertInResultSet(cur,"Database","testuser")
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# table type list
	output("TABLE TYPE LIST: ")
	assertTrue(cur.getTableTypeList())
	assertEquals(cur.getColumnName(0),"table_type")
	assertInResultSet(cur,"table_type","TABLE")
	output()


	# table list
	output("TABLE LIST: ")
	cur.sendQuery("drop table testtable1")
	cur.sendQuery("drop table testtable2")
	cur.sendQuery("drop table testtable3")
	cur.sendQuery("drop table testtable4")
	assertTrue(cur.sendQuery(
		"create table testtable1 ("
		"	col1 integer, "
		"	col2 integer)"))
	assertTrue(cur.sendQuery(
		"create table testtable2 ("
		"	col1 integer, "
		"	col2 integer)"))
	assertTrue(cur.sendQuery(
		"create table testtable3 ("
		"	col1 integer, "
		"	col2 integer)"))
	assertTrue(cur.sendQuery(
		"create table testtable4 ("
		"	col1 integer, "
		"	col2 integer)"))
	assertTrue(con.commit())
	assertTrue(cur.getTableList(None))
	assertInResultSet(cur,"Tables_in_xxx","testtable1")
	assertInResultSet(cur,"Tables_in_xxx","testtable2")
	assertInResultSet(cur,"Tables_in_xxx","testtable3")
	assertInResultSet(cur,"Tables_in_xxx","testtable4")
	assertTrue(cur.sendQuery("drop table testtable1"))
	assertTrue(cur.sendQuery("drop table testtable2"))
	assertTrue(cur.sendQuery("drop table testtable3"))
	assertTrue(cur.sendQuery("drop table testtable4"))
	assertTrue(con.commit())
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
	assertEquals(cur.getField(0,"precision"),"10")
	assertEquals(cur.getField(0,"local_type_name"),"INTEGER")
	assertTrue(cur.getTypeInfoList("char"))
	assertEquals(cur.getField(0,"type_name"),"CHAR")
	assertEquals(cur.getField(0,"data_type"),"1")
	assertEquals(cur.getField(0,"precision"),"32767")
	assertEquals(cur.getField(0,"local_type_name"),"CHAR")
	assertTrue(cur.getTypeInfoList("varchar"))
	assertEquals(cur.getField(0,"type_name"),"VARCHAR")
	assertEquals(cur.getField(0,"data_type"),"12")
	assertEquals(cur.getField(0,"precision"),"255")
	assertEquals(cur.getField(0,"local_type_name"),"VARCHAR")
	assertTrue(cur.getTypeInfoList("date"))
	assertEquals(cur.getField(0,"type_name"),"DATE")
	assertEquals(cur.getField(0,"data_type"),"91")
	assertEquals(cur.getField(0,"precision"),"10")
	assertEquals(cur.getField(0,"local_type_name"),"DATE")
	output()


	# column list
	output("COLUMN LIST: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	testboolean boolean, "
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testint8 int8, "
		"	testdecimal decimal(10,2), "
		"	testmoney money, "
		"	testsmallfloat smallfloat, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testnchar nchar(40), "
		"	testvarchar varchar(40), "
		"	testnvarchar nvarchar(40), "
		"	testlvarchar lvarchar(40), "
		"	testdate date, "
		"	testdatetime datetime year to second, "
		"	testtext text, "
		"	testbyte byte)"))
	assertTrue(con.commit())
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
	assertEquals(cur.getField(0,"column_name"),"testboolean")
	assertEquals(cur.getField(1,"column_name"),"testsmallint")
	assertEquals(cur.getField(2,"column_name"),"testint")
	assertEquals(cur.getField(3,"column_name"),"testbigint")
	assertEquals(cur.getField(4,"column_name"),"testint8")
	assertEquals(cur.getField(5,"column_name"),"testdecimal")
	assertEquals(cur.getField(6,"column_name"),"testmoney")
	assertEquals(cur.getField(7,"column_name"),"testsmallfloat")
	assertEquals(cur.getField(8,"column_name"),"testfloat")
	assertEquals(cur.getField(9,"column_name"),"testchar")
	assertEquals(cur.getField(10,"column_name"),"testnchar")
	assertEquals(cur.getField(11,"column_name"),"testvarchar")
	assertEquals(cur.getField(12,"column_name"),"testnvarchar")
	assertEquals(cur.getField(13,"column_name"),"testlvarchar")
	assertEquals(cur.getField(14,"column_name"),"testdate")
	assertEquals(cur.getField(15,"column_name"),"testdatetime")
	assertEquals(cur.getField(16,"column_name"),"testtext")
	assertEquals(cur.getField(17,"column_name"),"testbyte")
	assertEquals(cur.getField(0,"data_type"),"BOOLEAN")
	assertEquals(cur.getField(1,"data_type"),"SMALLINT")
	assertEquals(cur.getField(2,"data_type"),"INTEGER")
	assertEquals(cur.getField(3,"data_type"),"BIGINT")
	assertEquals(cur.getField(4,"data_type"),"INT8")
	assertEquals(cur.getField(5,"data_type"),"DECIMAL")
	assertEquals(cur.getField(6,"data_type"),"MONEY")
	assertEquals(cur.getField(7,"data_type"),"SMALLFLOAT")
	assertEquals(cur.getField(8,"data_type"),"FLOAT")
	assertEquals(cur.getField(9,"data_type"),"CHAR")
	assertEquals(cur.getField(10,"data_type"),"NCHAR")
	assertEquals(cur.getField(11,"data_type"),"VARCHAR")
	assertEquals(cur.getField(12,"data_type"),"NVARCHAR")
	assertEquals(cur.getField(13,"data_type"),"LVARCHAR")
	assertEquals(cur.getField(14,"data_type"),"DATE")
	assertEquals(cur.getField(15,"data_type"),"DATETIME")
	assertEquals(cur.getField(16,"data_type"),"TEXT")
	assertEquals(cur.getField(17,"data_type"),"BYTE")
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# column list - auto_increment, primary key
	output("COLUMN LIST - auto_increment, primary key: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 serial primary key, "
		"	col2 int)"))
	assertTrue(con.commit())
	assertTrue(cur.getColumnList("testtable",None))
	assertEquals(cur.getField(0,"extra"),"auto_increment")
	assertEquals(cur.getField(0,"column_key"),"PRI")
	assertEquals(cur.getField(1,"extra"),"")
	assertEquals(cur.getField(1,"column_key"),"")
	output()
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"))
	assertTrue(cur.getColumnList("testtable",None))
	assertEquals(cur.getField(0,"extra"),"")
	assertEquals(cur.getField(0,"column_key"),"PRI")
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# primary keys list
	output("PRIMARY KEYS LIST: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 integer primary key, "
		"	col2 integer)"))
	assertTrue(con.commit())
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
	assertTrue(keyname is not None and len(keyname)>0)
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# key and index list
	output("KEY AND INDEX LIST: ")
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable ("
		"	col1 integer primary key, "
		"	col2 integer)"))
	assertTrue(con.commit())
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
	assertTrue(cur.sendQuery("drop table testtable"))
	assertTrue(con.commit())
	output()


	# procedure list
	output("PROCEDURE LIST: ")
	cur.sendQuery("drop procedure testproc1")
	cur.sendQuery("drop procedure testproc2")
	cur.sendQuery("drop procedure testproc3")
	cur.sendQuery("drop procedure testproc4")
	assertTrue(cur.sendQuery(
		"create procedure testproc1("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"))
	assertTrue(cur.sendQuery(
		"create procedure testproc2("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"))
	assertTrue(cur.sendQuery(
		"create procedure testproc3("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"))
	assertTrue(cur.sendQuery(
		"create procedure testproc4("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"))
	assertTrue(con.commit())
	assertTrue(cur.getProcedureList(None))
	assertInResultSet(cur,"routine_name","testproc1")
	assertInResultSet(cur,"routine_name","testproc2")
	assertInResultSet(cur,"routine_name","testproc3")
	assertInResultSet(cur,"routine_name","testproc4")
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
	assertEquals(cur.getField(0,"parameter_name"),"in1")
	assertEquals(cur.getField(0,"parameter_mode"),"1")
	assertEquals(cur.getField(0,"data_type"),"integer")
	assertEquals(cur.getField(0,"ordinal_position"),"1")
	assertEquals(cur.getField(1,"parameter_name"),"in2")
	assertEquals(cur.getField(1,"parameter_mode"),"1")
	assertEquals(cur.getField(1,"data_type"),"char")
	assertEquals(cur.getField(1,"ordinal_position"),"2")
	assertEquals(cur.getField(2,"parameter_name"),"in3")
	assertEquals(cur.getField(2,"parameter_mode"),"1")
	assertEquals(cur.getField(2,"data_type"),"varchar")
	assertEquals(cur.getField(2,"ordinal_position"),"3")
	assertEquals(cur.getField(3,"parameter_name"),"in4")
	assertEquals(cur.getField(3,"parameter_mode"),"1")
	assertEquals(cur.getField(3,"data_type"),"date")
	assertEquals(cur.getField(3,"ordinal_position"),"4")
	assertTrue(cur.sendQuery("drop procedure testproc1"))
	assertTrue(cur.sendQuery("drop procedure testproc2"))
	assertTrue(cur.sendQuery("drop procedure testproc3"))
	assertTrue(cur.sendQuery("drop procedure testproc4"))
	assertTrue(con.commit())
	output()


	# invalid queries
	output("INVALID QUERIES: ")
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
