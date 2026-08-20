#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'socket'
require 'sqlrelay'
require './asserts'


# hostname
hostname=Socket.gethostname.split(".")[0].downcase
dumptran="dump tran #{hostname} with truncate_only"


# instantiation
con=SQLRConnection.new("sqlrelay",9005,"/tmp/freetdstest.socket",
						"testuser","testpassword",0,1)
cur=SQLRCursor.new(con)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"freetds")
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
print "\n"


# transaction state
print "TRANSACTION STATE: \n"
assertEqual(con.getDefaultTransactionModel(),"explicit-error")
assertEqual(con.getTransactionModel(),"explicit-error")
assertFalse(con.getInTransaction())
assertTrue(con.getAutoCommit())
print "\n"


# bind format
print "BIND FORMAT: \n"
assertEqual(con.bindFormat(),"@*")
print "\n"


# nextval format
print "NEXTVAL FORMAT: \n"
assertEqual(con.nextvalFormat(),"%s.nextval")
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
isolationlevels=["1","0","2","3"]
for il in isolationlevels
	assertTrue(con.setIsolationLevel(il))
	assertEqual(con.getIsolationLevel(),il)
	print "\n"
end
# reset to the default isolation level
assertTrue(con.setIsolationLevel(isolationlevels[0]))
print "\n"


# create testtable
print "CREATE TESTTABLE: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(dumptran)
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testsmallint smallint, "+
	"	testtinyint tinyint, "+
	"	testreal real, "+
	"	testfloat float, "+
	"	testdecimal decimal(4,1), "+
	"	testnumeric numeric(4,1), "+
	"	testmoney money, "+
	"	testsmallmoney smallmoney, "+
	"	testdatetime datetime, "+
	"	testsmalldatetime smalldatetime, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testbit bit, "+
	"	testdate date, "+
	"	testtime time, "+
	"	testbigdatetime bigdatetime, "+
	"	testbigtime bigtime) lock datarows"))
print "\n"


# insert
print "INSERT: \n"
assertTrue(con.begin())
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1, "+
	"	1, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	1.00, "+
	"	1.00, "+
	"	'01-Jan-2001 01:00:00', "+
	"	'01-Jan-2001 01:00:00', "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	1, "+
	"	'01-Jan-2001', "+
	"	'13:01:01', "+
	"	'01-Jan-2001 13:01:01', "+
	"	'01:01:01.001000')"))
print "\n"


# affected rows
print "AFFECTED ROWS: \n"
assertEqual(cur.affectedRows(),1)
print "\n"


# input bind by position
print "INPUT BIND BY POSITION: \n"
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?)")
assertEqual(cur.countBindVariables(),18)
cur.inputBind("1",2)
cur.inputBind("2",2)
cur.inputBind("3",2)
cur.inputBind("4",2.5,2,1)
cur.inputBind("5",2.5,2,1)
cur.inputBind("6",2.5,2,1)
cur.inputBind("7",2.5,2,1)
cur.inputBind("8",2.00,3,2)
cur.inputBind("9",2.00,3,2)
cur.inputBind("10","01-Jan-2002 02:00:00")
cur.inputBind("11","01-Jan-2002 02:00:00")
cur.inputBind("12","testchar2")
cur.inputBind("13","testvarchar2")
cur.inputBind("14",1)
cur.inputBind("15","01-Jan-2001")
cur.inputBind("16","13:01:01")
cur.inputBind("17","01-Jan-2001 13:01:01")
cur.inputBind("18","01:01:01.001000")
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",3)
cur.inputBind("2",3)
cur.inputBind("3",3)
cur.inputBind("4",3.5,2,1)
cur.inputBind("5",3.5,2,1)
cur.inputBind("6",3.5,2,1)
cur.inputBind("7",3.5,2,1)
cur.inputBind("8",3.00,3,2)
cur.inputBind("9",3.00,3,2)
cur.inputBind("10","01-Jan-2003 03:00:00")
cur.inputBind("11","01-Jan-2003 03:00:00")
cur.inputBind("12","testchar3")
cur.inputBind("13","testvarchar3")
cur.inputBind("14",1)
cur.inputBind("15","01-Jan-2001")
cur.inputBind("16","13:01:01")
cur.inputBind("17","01-Jan-2001 13:01:01")
cur.inputBind("18","01:01:01.001000")
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by position
# freetds doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by position with validation
print "INPUT BIND BY POSITION WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("1",4)
cur.inputBind("2",4)
cur.inputBind("3",4)
cur.inputBind("4",4.5,2,1)
cur.inputBind("5",4.5,2,1)
cur.inputBind("6",4.5,2,1)
cur.inputBind("7",4.5,2,1)
cur.inputBind("8",4.00,3,2)
cur.inputBind("9",4.00,3,2)
cur.inputBind("10","01-Jan-2004 04:00:00")
cur.inputBind("11","01-Jan-2004 04:00:00")
cur.inputBind("12","testchar4")
cur.inputBind("13","testvarchar4")
cur.inputBind("14",1)
cur.inputBind("15","01-Jan-2001")
cur.inputBind("16","13:01:01")
cur.inputBind("17","01-Jan-2001 13:01:01")
cur.inputBind("18","01:01:01.001000")
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# input bind by name
print "INPUT BIND BY NAME: \n"
cur.clearBinds()
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	@var1, "+
	"	@var2, "+
	"	@var3, "+
	"	@var4, "+
	"	@var5, "+
	"	@var6, "+
	"	@var7, "+
	"	@var8, "+
	"	@var9, "+
	"	@var10, "+
	"	@var11, "+
	"	@var12, "+
	"	@var13, "+
	"	@var14, "+
	"	@var15, "+
	"	@var16, "+
	"	@var17, "+
	"	@var18)")
assertEqual(cur.countBindVariables(),18)
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
cur.inputBind("var15","01-Jan-2001")
cur.inputBind("var16","13:01:01")
cur.inputBind("var17","01-Jan-2001 13:01:01")
cur.inputBind("var18","01:01:01.001000")
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("var1",6)
cur.inputBind("var2",6)
cur.inputBind("var3",6)
cur.inputBind("var4",6.5,2,1)
cur.inputBind("var5",6.5,2,1)
cur.inputBind("var6",6.5,2,1)
cur.inputBind("var7",6.5,2,1)
cur.inputBind("var8",6.00,3,2)
cur.inputBind("var9",6.00,3,2)
cur.inputBind("var10","01-Jan-2006 06:00:00")
cur.inputBind("var11","01-Jan-2006 06:00:00")
cur.inputBind("var12","testchar6")
cur.inputBind("var13","testvarchar6")
cur.inputBind("var14",1)
cur.inputBind("var15","01-Jan-2001")
cur.inputBind("var16","13:01:01")
cur.inputBind("var17","01-Jan-2001 13:01:01")
cur.inputBind("var18","01:01:01.001000")
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("var1",7)
cur.inputBind("var2",7)
cur.inputBind("var3",7)
cur.inputBind("var4",7.5,2,1)
cur.inputBind("var5",7.5,2,1)
cur.inputBind("var6",7.5,2,1)
cur.inputBind("var7",7.5,2,1)
cur.inputBind("var8",7.00,3,2)
cur.inputBind("var9",7.00,3,2)
cur.inputBind("var10","01-Jan-2007 07:00:00")
cur.inputBind("var11","01-Jan-2007 07:00:00")
cur.inputBind("var12","testchar7")
cur.inputBind("var13","testvarchar7")
cur.inputBind("var14",1)
cur.inputBind("var15","01-Jan-2001")
cur.inputBind("var16","13:01:01")
cur.inputBind("var17","01-Jan-2001 13:01:01")
cur.inputBind("var18","01:01:01.001000")
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by name
# freetds doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by name with validation
print "INPUT BIND BY NAME WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("var1",8)
cur.inputBind("var2",8)
cur.inputBind("var3",8)
cur.inputBind("var4",8.5,2,1)
cur.inputBind("var5",8.5,2,1)
cur.inputBind("var6",8.5,2,1)
cur.inputBind("var7",8.5,2,1)
cur.inputBind("var8",8.00,3,2)
cur.inputBind("var9",8.00,3,2)
cur.inputBind("var10","01-Jan-2008 08:00:00")
cur.inputBind("var11","01-Jan-2008 08:00:00")
cur.inputBind("var12","testchar8")
cur.inputBind("var13","testvarchar8")
cur.inputBind("var14",1)
cur.inputBind("var15","01-Jan-2001")
cur.inputBind("var16","13:01:01")
cur.inputBind("var17","01-Jan-2001 13:01:01")
cur.inputBind("var18","01:01:01.001000")
cur.inputBind("var19","junkvalue")
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# select
print "SELECT: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),18)
print "\n"


# column names
print "COLUMN NAMES: \n"
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
assertEqual(cur.getColumnName(14),"testdate")
assertEqual(cur.getColumnName(15),"testtime")
assertEqual(cur.getColumnName(16),"testbigdatetime")
assertEqual(cur.getColumnName(17),"testbigtime")
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
assertEqual(cols[14],"testdate")
assertEqual(cols[15],"testtime")
assertEqual(cols[16],"testbigdatetime")
assertEqual(cols[17],"testbigtime")
print "\n"


# column types
print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"INT")
assertEqual(cur.getColumnType("testint"),"INT")
assertEqual(cur.getColumnType(1),"SMALLINT")
assertEqual(cur.getColumnType("testsmallint"),"SMALLINT")
assertEqual(cur.getColumnType(2),"TINYINT")
assertEqual(cur.getColumnType("testtinyint"),"TINYINT")
assertEqual(cur.getColumnType(3),"REAL")
assertEqual(cur.getColumnType("testreal"),"REAL")
assertEqual(cur.getColumnType(4),"FLOAT")
assertEqual(cur.getColumnType("testfloat"),"FLOAT")
assertEqual(cur.getColumnType(5),"DECIMAL")
assertEqual(cur.getColumnType("testdecimal"),"DECIMAL")
assertEqual(cur.getColumnType(6),"NUMERIC")
assertEqual(cur.getColumnType("testnumeric"),"NUMERIC")
assertEqual(cur.getColumnType(7),"MONEY")
assertEqual(cur.getColumnType("testmoney"),"MONEY")
assertEqual(cur.getColumnType(8),"SMALLMONEY")
assertEqual(cur.getColumnType("testsmallmoney"),"SMALLMONEY")
assertEqual(cur.getColumnType(9),"DATETIME")
assertEqual(cur.getColumnType("testdatetime"),"DATETIME")
assertEqual(cur.getColumnType(10),"SMALLDATETIME")
assertEqual(cur.getColumnType("testsmalldatetime"),"SMALLDATETIME")
assertEqual(cur.getColumnType(11),"CHAR")
assertEqual(cur.getColumnType("testchar"),"CHAR")
assertEqual(cur.getColumnType(12),"VARCHAR")
assertEqual(cur.getColumnType("testvarchar"),"VARCHAR")
assertEqual(cur.getColumnType(13),"BIT")
assertEqual(cur.getColumnType("testbit"),"BIT")
assertEqual(cur.getColumnType(14),"DATE")
assertEqual(cur.getColumnType("testdate"),"DATE")
assertEqual(cur.getColumnType(15),"TIME")
assertEqual(cur.getColumnType("testtime"),"TIME")
assertEqual(cur.getColumnType(16),"TIMESTAMP")
assertEqual(cur.getColumnType("testbigdatetime"),"TIMESTAMP")
assertEqual(cur.getColumnType(17),"TIME")
assertEqual(cur.getColumnType("testbigtime"),"TIME")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnLength("testint"),4)
assertEqual(cur.getColumnLength(1),2)
assertEqual(cur.getColumnLength("testsmallint"),2)
assertEqual(cur.getColumnLength(2),1)
assertEqual(cur.getColumnLength("testtinyint"),1)
assertEqual(cur.getColumnLength(3),4)
assertEqual(cur.getColumnLength("testreal"),4)
assertEqual(cur.getColumnLength(4),8)
assertEqual(cur.getColumnLength("testfloat"),8)
# freetds reports the decimal/numeric display length as 35
assertEqual(cur.getColumnLength(5),35)
assertEqual(cur.getColumnLength("testdecimal"),35)
assertEqual(cur.getColumnLength(6),35)
assertEqual(cur.getColumnLength("testnumeric"),35)
assertEqual(cur.getColumnLength(7),8)
assertEqual(cur.getColumnLength("testmoney"),8)
assertEqual(cur.getColumnLength(8),4)
assertEqual(cur.getColumnLength("testsmallmoney"),4)
assertEqual(cur.getColumnLength(9),8)
assertEqual(cur.getColumnLength("testdatetime"),8)
assertEqual(cur.getColumnLength(10),4)
assertEqual(cur.getColumnLength("testsmalldatetime"),4)
# char(40)/varchar(40) report the declared length 40 (not multiplied)
assertEqual(cur.getColumnLength(11),40)
assertEqual(cur.getColumnLength("testchar"),40)
assertEqual(cur.getColumnLength(12),40)
assertEqual(cur.getColumnLength("testvarchar"),40)
assertEqual(cur.getColumnLength(13),1)
assertEqual(cur.getColumnLength("testbit"),1)
assertEqual(cur.getColumnLength(14),4)
assertEqual(cur.getColumnLength("testdate"),4)
assertEqual(cur.getColumnLength(15),4)
assertEqual(cur.getColumnLength("testtime"),4)
assertEqual(cur.getColumnLength(16),8)
assertEqual(cur.getColumnLength("testbigdatetime"),8)
assertEqual(cur.getColumnLength(17),8)
assertEqual(cur.getColumnLength("testbigtime"),8)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest("testint"),1)
assertEqual(cur.getLongest(1),1)
assertEqual(cur.getLongest("testsmallint"),1)
assertEqual(cur.getLongest(2),1)
assertEqual(cur.getLongest("testtinyint"),1)
assertEqual(cur.getLongest(3),3)
assertEqual(cur.getLongest("testreal"),3)
assertEqual(cur.getLongest(4),3)
assertEqual(cur.getLongest("testfloat"),3)
assertEqual(cur.getLongest(5),3)
assertEqual(cur.getLongest("testdecimal"),3)
assertEqual(cur.getLongest(6),3)
assertEqual(cur.getLongest("testnumeric"),3)
assertMoneyEqualLen(cur.getLongest(7),6)
assertMoneyEqualLen(cur.getLongest("testmoney"),6)
assertMoneyEqualLen(cur.getLongest(8),6)
assertMoneyEqualLen(cur.getLongest("testsmallmoney"),6)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getLongest(9),26)
assertEqual(cur.getLongest("testdatetime"),26)
assertEqual(cur.getLongest(10),26)
assertEqual(cur.getLongest("testsmalldatetime"),26)
assertEqual(cur.getLongest(11),40)
assertEqual(cur.getLongest("testchar"),40)
assertEqual(cur.getLongest(12),12)
assertEqual(cur.getLongest("testvarchar"),12)
assertEqual(cur.getLongest(13),1)
assertEqual(cur.getLongest("testbit"),1)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getLongest(14),26)
assertEqual(cur.getLongest("testdate"),26)
assertEqual(cur.getLongest(15),26)
assertEqual(cur.getLongest("testtime"),26)
assertEqual(cur.getLongest(16),26)
assertEqual(cur.getLongest("testbigdatetime"),26)
assertEqual(cur.getLongest(17),26)
assertEqual(cur.getLongest("testbigtime"),26)
print "\n"


# row count
print "ROW COUNT: \n"
assertEqual(cur.rowCount(),8)
print "\n"


# total rows
print "TOTAL ROWS: \n"
assertEqual(cur.totalRows(),0)
print "\n"


# first row index
print "FIRST ROW INDEX: \n"
assertEqual(cur.firstRowIndex(),0)
print "\n"


# end of result set
print "END OF RESULT SET: \n"
assertTrue(cur.endOfResultSet())
print "\n"


# fields by index
print "FIELDS BY INDEX: \n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),"1")
assertEqual(cur.getField(0,3),"1.5")
assertEqual(cur.getField(0,4),"1.5")
assertEqual(cur.getField(0,5),"1.5")
assertEqual(cur.getField(0,6),"1.5")
assertMoneyEqual(cur.getField(0,7),"1.0000")
assertMoneyEqual(cur.getField(0,8),"1.0000")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(0,9),"Jan  1 2001 01:00:00:000AM")
assertEqual(cur.getField(0,10),"Jan  1 2001 01:00:00:000AM")
assertEqual(cur.getField(0,11),"testchar1                               ")
assertEqual(cur.getField(0,12),"testvarchar1")
assertEqual(cur.getField(0,13),"1")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(0,14),"Jan  1 2001 00:00:00:000AM")
assertEqual(cur.getField(0,15),"Jan  1 1900 01:01:01:000PM")
assertEqual(cur.getField(0,16),"Jan  1 2001 01:01:01:000PM")
assertEqual(cur.getField(0,17),"Jan  1 1900 01:01:01:001AM")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(7,2),"8")
assertEqual(cur.getField(7,3),"8.5")
assertEqual(cur.getField(7,4),"8.5")
assertEqual(cur.getField(7,5),"8.5")
assertEqual(cur.getField(7,6),"8.5")
assertMoneyEqual(cur.getField(7,7),"8.0000")
assertMoneyEqual(cur.getField(7,8),"8.0000")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(7,9),"Jan  1 2008 08:00:00:000AM")
assertEqual(cur.getField(7,10),"Jan  1 2008 08:00:00:000AM")
assertEqual(cur.getField(7,11),"testchar8                               ")
assertEqual(cur.getField(7,12),"testvarchar8")
assertEqual(cur.getField(7,13),"1")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(7,14),"Jan  1 2001 00:00:00:000AM")
assertEqual(cur.getField(7,15),"Jan  1 1900 01:01:01:000PM")
assertEqual(cur.getField(7,16),"Jan  1 2001 01:01:01:000PM")
assertEqual(cur.getField(7,17),"Jan  1 1900 01:01:01:001AM")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),1)
assertEqual(cur.getFieldLength(0,2),1)
assertEqual(cur.getFieldLength(0,3),3)
assertEqual(cur.getFieldLength(0,4),3)
assertEqual(cur.getFieldLength(0,5),3)
assertEqual(cur.getFieldLength(0,6),3)
assertMoneyEqualLen(cur.getFieldLength(0,7),6)
assertMoneyEqualLen(cur.getFieldLength(0,8),6)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(0,9),26)
assertEqual(cur.getFieldLength(0,10),26)
assertEqual(cur.getFieldLength(0,11),40)
assertEqual(cur.getFieldLength(0,12),12)
assertEqual(cur.getFieldLength(0,13),1)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(0,14),26)
assertEqual(cur.getFieldLength(0,15),26)
assertEqual(cur.getFieldLength(0,16),26)
assertEqual(cur.getFieldLength(0,17),26)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),1)
assertEqual(cur.getFieldLength(7,2),1)
assertEqual(cur.getFieldLength(7,3),3)
assertEqual(cur.getFieldLength(7,4),3)
assertEqual(cur.getFieldLength(7,5),3)
assertEqual(cur.getFieldLength(7,6),3)
assertMoneyEqualLen(cur.getFieldLength(7,7),6)
assertMoneyEqualLen(cur.getFieldLength(7,8),6)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(7,9),26)
assertEqual(cur.getFieldLength(7,10),26)
assertEqual(cur.getFieldLength(7,11),40)
assertEqual(cur.getFieldLength(7,12),12)
assertEqual(cur.getFieldLength(7,13),1)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(7,14),26)
assertEqual(cur.getFieldLength(7,15),26)
assertEqual(cur.getFieldLength(7,16),26)
assertEqual(cur.getFieldLength(7,17),26)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"testint"),"1")
assertEqual(cur.getField(0,"testsmallint"),"1")
assertEqual(cur.getField(0,"testtinyint"),"1")
assertEqual(cur.getField(0,"testreal"),"1.5")
assertEqual(cur.getField(0,"testfloat"),"1.5")
assertEqual(cur.getField(0,"testdecimal"),"1.5")
assertEqual(cur.getField(0,"testnumeric"),"1.5")
assertMoneyEqual(cur.getField(0,"testmoney"),"1.0000")
assertMoneyEqual(cur.getField(0,"testsmallmoney"),"1.0000")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(0,"testdatetime"),"Jan  1 2001 01:00:00:000AM")
assertEqual(cur.getField(0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM")
assertEqual(cur.getField(0,"testchar"),"testchar1                               ")
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
assertEqual(cur.getField(0,"testbit"),"1")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(0,"testdate"),"Jan  1 2001 00:00:00:000AM")
assertEqual(cur.getField(0,"testtime"),"Jan  1 1900 01:01:01:000PM")
assertEqual(cur.getField(0,"testbigdatetime"),"Jan  1 2001 01:01:01:000PM")
assertEqual(cur.getField(0,"testbigtime"),"Jan  1 1900 01:01:01:001AM")
print "\n"
assertEqual(cur.getField(7,"testint"),"8")
assertEqual(cur.getField(7,"testsmallint"),"8")
assertEqual(cur.getField(7,"testtinyint"),"8")
assertEqual(cur.getField(7,"testreal"),"8.5")
assertEqual(cur.getField(7,"testfloat"),"8.5")
assertEqual(cur.getField(7,"testdecimal"),"8.5")
assertEqual(cur.getField(7,"testnumeric"),"8.5")
assertMoneyEqual(cur.getField(7,"testmoney"),"8.0000")
assertMoneyEqual(cur.getField(7,"testsmallmoney"),"8.0000")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(7,"testdatetime"),"Jan  1 2008 08:00:00:000AM")
assertEqual(cur.getField(7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM")
assertEqual(cur.getField(7,"testchar"),"testchar8                               ")
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
assertEqual(cur.getField(7,"testbit"),"1")
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getField(7,"testdate"),"Jan  1 2001 00:00:00:000AM")
assertEqual(cur.getField(7,"testtime"),"Jan  1 1900 01:01:01:000PM")
assertEqual(cur.getField(7,"testbigdatetime"),"Jan  1 2001 01:01:01:000PM")
assertEqual(cur.getField(7,"testbigtime"),"Jan  1 1900 01:01:01:001AM")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"testint"),1)
assertEqual(cur.getFieldLength(0,"testsmallint"),1)
assertEqual(cur.getFieldLength(0,"testtinyint"),1)
assertEqual(cur.getFieldLength(0,"testreal"),3)
assertEqual(cur.getFieldLength(0,"testfloat"),3)
assertEqual(cur.getFieldLength(0,"testdecimal"),3)
assertEqual(cur.getFieldLength(0,"testnumeric"),3)
assertMoneyEqualLen(cur.getFieldLength(0,"testmoney"),6)
assertMoneyEqualLen(cur.getFieldLength(0,"testsmallmoney"),6)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(0,"testdatetime"),26)
assertEqual(cur.getFieldLength(0,"testsmalldatetime"),26)
assertEqual(cur.getFieldLength(0,"testchar"),40)
assertEqual(cur.getFieldLength(0,"testvarchar"),12)
assertEqual(cur.getFieldLength(0,"testbit"),1)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(0,"testdate"),26)
assertEqual(cur.getFieldLength(0,"testtime"),26)
assertEqual(cur.getFieldLength(0,"testbigdatetime"),26)
assertEqual(cur.getFieldLength(0,"testbigtime"),26)
print "\n"
assertEqual(cur.getFieldLength(7,"testint"),1)
assertEqual(cur.getFieldLength(7,"testsmallint"),1)
assertEqual(cur.getFieldLength(7,"testtinyint"),1)
assertEqual(cur.getFieldLength(7,"testreal"),3)
assertEqual(cur.getFieldLength(7,"testfloat"),3)
assertEqual(cur.getFieldLength(7,"testdecimal"),3)
assertEqual(cur.getFieldLength(7,"testnumeric"),3)
assertMoneyEqualLen(cur.getFieldLength(7,"testmoney"),6)
assertMoneyEqualLen(cur.getFieldLength(7,"testsmallmoney"),6)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(7,"testdatetime"),26)
assertEqual(cur.getFieldLength(7,"testsmalldatetime"),26)
assertEqual(cur.getFieldLength(7,"testchar"),40)
assertEqual(cur.getFieldLength(7,"testvarchar"),12)
assertEqual(cur.getFieldLength(7,"testbit"),1)
# freetds datetime rendering for the fixture tds version
assertEqual(cur.getFieldLength(7,"testdate"),26)
assertEqual(cur.getFieldLength(7,"testtime"),26)
assertEqual(cur.getFieldLength(7,"testbigdatetime"),26)
assertEqual(cur.getFieldLength(7,"testbigtime"),26)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1")
assertEqual(fields[2],"1")
assertEqual(fields[3],"1.5")
assertEqual(fields[4],"1.5")
assertEqual(fields[5],"1.5")
assertEqual(fields[6],"1.5")
assertMoneyEqual(fields[7],"1.0000")
assertMoneyEqual(fields[8],"1.0000")
# freetds datetime rendering for the fixture tds version
assertEqual(fields[9],"Jan  1 2001 01:00:00:000AM")
assertEqual(fields[10],"Jan  1 2001 01:00:00:000AM")
assertEqual(fields[11],"testchar1                               ")
assertEqual(fields[12],"testvarchar1")
assertEqual(fields[13],"1")
# freetds datetime rendering for the fixture tds version
assertEqual(fields[14],"Jan  1 2001 00:00:00:000AM")
assertEqual(fields[15],"Jan  1 1900 01:01:01:000PM")
assertEqual(fields[16],"Jan  1 2001 01:01:01:000PM")
assertEqual(fields[17],"Jan  1 1900 01:01:01:001AM")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],1)
assertEqual(fieldlens[2],1)
assertEqual(fieldlens[3],3)
assertEqual(fieldlens[4],3)
assertEqual(fieldlens[5],3)
assertEqual(fieldlens[6],3)
assertMoneyEqualLen(fieldlens[7],6)
assertMoneyEqualLen(fieldlens[8],6)
# freetds datetime rendering for the fixture tds version
assertEqual(fieldlens[9],26)
assertEqual(fieldlens[10],26)
assertEqual(fieldlens[11],40)
assertEqual(fieldlens[12],12)
assertEqual(fieldlens[13],1)
# freetds datetime rendering for the fixture tds version
assertEqual(fieldlens[14],26)
assertEqual(fieldlens[15],26)
assertEqual(fieldlens[16],26)
assertEqual(fieldlens[17],26)
print "\n"


# result set buffer size
print "RESULT SET BUFFER SIZE: \n"
assertEqual(cur.getResultSetBufferSize(),0)
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getResultSetBufferSize(),2)
print "\n"
assertEqual(cur.firstRowIndex(),0)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),2)
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(1,0),"2")
assertEqual(cur.getField(2,0),"3")
print "\n"
assertEqual(cur.firstRowIndex(),2)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),4)
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,0),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.setResultSetBufferSize(0)
print "\n"


# dont get column info
print "DONT GET COLUMN INFO: \n"
cur.dontGetColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getColumnName(0),nil)
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),nil)
cur.getColumnInfo()
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getColumnName(0),"testint")
assertEqual(cur.getColumnLength(0),4)
assertEqual(cur.getColumnType(0),"INT")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
print "\n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(1,0),"2")
assertEqual(cur.getField(2,0),"3")
assertEqual(cur.getField(3,0),"4")
assertEqual(cur.getField(4,0),"5")
assertEqual(cur.getField(5,0),"6")
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
print "\n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(1,0),"2")
assertEqual(cur.getField(2,0),"3")
assertEqual(cur.getField(3,0),"4")
assertEqual(cur.getField(4,0),"5")
assertEqual(cur.getField(5,0),"6")
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
print "\n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
print "\n"
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(1,0),"2")
assertEqual(cur.getField(2,0),"3")
assertEqual(cur.getField(3,0),"4")
assertEqual(cur.getField(4,0),"5")
assertEqual(cur.getField(5,0),"6")
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
print "\n"


# suspended result set
print "SUSPENDED RESULT SET: \n"
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getField(2,0),"3")
id=cur.getResultSetId()
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
assertTrue(cur.resumeResultSet(id))
print "\n"
assertEqual(cur.firstRowIndex(),4)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),6)
assertEqual(cur.getField(7,0),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,0),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.setResultSetBufferSize(0)
print "\n"


# cached result set
print "CACHED RESULT SET: \n"
cur.cacheToFile("cachefile1-freetds")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1-freetds")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
print "\n"


# column count for cached result set
print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),18)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
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
assertEqual(cur.getColumnName(14),"testdate")
assertEqual(cur.getColumnName(15),"testtime")
assertEqual(cur.getColumnName(16),"testbigdatetime")
assertEqual(cur.getColumnName(17),"testbigtime")
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
assertEqual(cols[14],"testdate")
assertEqual(cols[15],"testtime")
assertEqual(cols[16],"testbigdatetime")
assertEqual(cols[17],"testbigtime")
print "\n"


# cached result set with result set buffer size
print "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1-freetds")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1-freetds")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# from one cache file to another
print "FROM ONE CACHE FILE TO ANOTHER: \n"
cur.cacheToFile("cachefile2-freetds")
assertTrue(cur.openCachedResultSet("cachefile1-freetds"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2-freetds"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
print "\n"


# from one cache file to another with result set buffer size
print "FROM ONE CACHE FILE TO ANOTHER "+
	"WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile2-freetds")
assertTrue(cur.openCachedResultSet("cachefile1-freetds"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2-freetds"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# cached result set with suspend and result set buffer size
print "CACHED RESULT SET WITH SUSPEND "+
	"AND RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1-freetds")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getField(2,0),"3")
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1-freetds")
id=cur.getResultSetId()
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
print "\n"
assertTrue(con.resumeSession(port,socket))
assertTrue(cur.resumeCachedResultSet(id,filename))
print "\n"
assertEqual(cur.firstRowIndex(),4)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),6)
assertEqual(cur.getField(7,0),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,0),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.cacheOff()
print "\n"
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# finished suspended session
print "FINISHED SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getField(4,0),"5")
assertEqual(cur.getField(5,0),"6")
assertEqual(cur.getField(6,0),"7")
assertEqual(cur.getField(7,0),"8")
id=cur.getResultSetId()
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
assertTrue(cur.resumeResultSet(id))
assertEqual(cur.getField(4,0),nil)
assertEqual(cur.getField(5,0),nil)
assertEqual(cur.getField(6,0),nil)
assertEqual(cur.getField(7,0),nil)
print "\n"


# nested selects
print "NESTED SELECTS: \n"
# can't do this with freetds
#cur.setResultSetBufferSize(1)
assertTrue(cur.sendQuery("select * from testtable"))
secondcur=SQLRCursor.new(con)
secondcur.setResultSetBufferSize(1)
i=0
while cur.getRow(i)
	assertTrue(secondcur.sendQuery("select * from testtable"))
	i=i+1
end
secondcur.closeResultSet()
#cur.setResultSetBufferSize(0)
assertTrue(con.commit())
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# reset transaction state
print "RESET TRANSACTION STATE: \n"
assertTrue(con.commit())
assertEqual(con.getTransactionModel(),"explicit-error")
assertTrue(con.getAutoCommit())
print "\n"


# transaction behavior - implicit
print "TRANSACTION BEHAVIOR - implicit: \n"
# sap ase rejects DDL inside a chained-mode (multi-statement) tx
# unless `sp_dboption ... 'ddl in tran', true` is set on the db;
# create the table while still in unchained mode, then switch.
# `lock datarows` is needed so secondcur's count(*) scan doesn't
# block on the writer's page lock from the in-flight insert
assertTrue(cur.sendQuery(
	"create table testtable (col1 integer) lock datarows"))
assertTrue(con.setTransactionModel("implicit"))
assertEqual(con.getTransactionModel(),"implicit")
secondcon=SQLRConnection.new("sqlrelay",9005,"/tmp/freetdstest.socket",
					"testuser","testpassword",0,1)
secondcur=SQLRCursor.new(secondcon)
setSecondConnection(secondcon)
setSecondCursor(secondcur)
# session is in a transaction; insert is not visible until commit
assertTrue(con.getInTransaction())
assertFalse(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
# commit makes it visible, and implicitly starts a new transaction
assertTrue(con.commit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# rollback discards, and implicitly starts a new transaction
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
secondcur.closeResultSet()
# switch back to unchained mode so the drop isn't rejected
assertTrue(con.setTransactionModel("explicit-error"))
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit
print "TRANSACTION BEHAVIOR - explicit: \n"
assertTrue(con.setTransactionModel("explicit"))
assertEqual(con.getTransactionModel(),"explicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"))
# begin starts a new transaction; insert is not visible until commit
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"0")
# commit makes it visible; no new transaction is started
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback discards; no new transaction is started
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable values (3)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
secondcur.closeResultSet()
# switch back to unchained mode so the drop isn't rejected
assertTrue(con.setTransactionModel("explicit-error"))
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit-deferred
print "TRANSACTION BEHAVIOR - explicit-deferred: \n"
assertTrue(con.setTransactionModel("explicit-deferred"))
assertEqual(con.getTransactionModel(),"explicit-deferred")
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
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback discards
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# during a transaction started by begin(), autoCommitOn is a
# no-op: the autocommit setting takes effect after the user
# explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (3)"))
assertTrue(con.autoCommitOn())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# explicit commit ends the tx; autocommit-on now takes effect
assertTrue(con.commit())
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autocommit is on; subsequent inserts are visible immediately
assertTrue(cur.sendQuery("insert into testtable values (4)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"3")
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
assertEqual(secondcur.getField(0,0),"4")
assertTrue(cur.sendQuery("insert into testtable values (6)"))
assertTrue(con.rollback())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"4")
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
assertEqual(secondcur.getField(0,0),"4")
assertTrue(con.commit())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"5")
secondcur.closeResultSet()
# switch back to unchained mode so the drop isn't rejected
assertTrue(con.setTransactionModel("explicit-error"))
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit-error
print "TRANSACTION BEHAVIOR - explicit-error: \n"
assertTrue(con.setTransactionModel("explicit-error"))
assertEqual(con.getTransactionModel(),"explicit-error")
assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"))
# begin, insert, commit
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(con.commit())
assertFalse(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
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
assertEqual(secondcur.getField(0,0),"2")
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
print "\n"


# transaction behavior - none
print "TRANSACTION BEHAVIOR - none: \n"
assertTrue(con.setTransactionModel("none"))
assertEqual(con.getTransactionModel(),"none")
assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"))
# no transactions; everything is visible immediately
assertTrue(con.getAutoCommit())
assertFalse(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
# commit and rollback are no-ops
assertTrue(con.commit())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"2")
# autocommit is always on; autoCommitOff is an error
assertFalse(con.autoCommitOff())
assertTrue(con.getAutoCommit())
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
secondcur.closeResultSet()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# reset transaction behavior
print "RESET TRANSACTION BEHAVIOR: \n"
assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()))
assertEqual(con.getTransactionModel(),"explicit-error")
assertTrue(con.getAutoCommit())
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),'$(var2)',$(var3)")
cur.substitution("var1",1)
cur.substitution("var2","hello")
cur.substitution("var3",10.5556,6,4)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# array substitutions
print "ARRAY SUBSTITUTIONS: \n"
cur.prepareQuery("select $(var1),$(var2),$(var3)")
cur.substitutions(["var1","var2","var3"],[1,2,3])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"2")
assertEqual(cur.getField(0,2),"3")
print "\n"
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'")
cur.substitutions(["var1","var2","var3"],["hi","hello","bye"])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"hi")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"bye")
print "\n"
cur.prepareQuery("select $(var1),$(var2),$(var3)")
cur.substitutions(["var1","var2","var3"],
			[10.55,10.556,10.5556],[4,5,6],[2,3,4])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"10.55")
assertEqual(cur.getField(0,1),"10.556")
assertEqual(cur.getField(0,2),"10.5556")
print "\n"


# nulls as nulls
print "NULLS AS NULLS: \n"
cur.getNullsAsNils()
assertTrue(cur.sendQuery("select NULL,1,NULL"))
assertEqual(cur.getField(0,0),nil)
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("select NULL,1,NULL"))
assertEqual(cur.getField(0,0),"")
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),"")
print "\n"



# null and empty lobs
print "NULL AND EMPTY LOBS: \n"
cur.getNullsAsNils()
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob1 text NULL, "+
	"	testclob2 text NULL, "+
	"	testblob1 image NULL, "+
	"	testblob2 image NULL)"))
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?)")
cur.inputBindClob("1","",0)
cur.inputBindClob("2",nil,0)
cur.inputBindBlob("3","",0)
cur.inputBindBlob("4",nil,0)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
# sap converts empty strings to a single space.  It's possible that
# if we had true input bind support on the backend, then this would
# work correctly, but for now we're faking binds, and inserting an
# empty string, so we have to check for a single space here.
assertEqual(cur.getField(0,0)," ")
assertEqual(cur.getField(0,1),nil)
# sap doesn't really support inserting an empty string into a binary
# column.  The minimum that can be inserted is a single \0.  The C++
# test compares with strcmp which treats the leading \0 as an empty
# string; Ruby's == compares byte-for-byte so truncate at the first
# \0 to match the C++ semantics.
assertEqual(cur.getField(0,2).to_s.split("\0").first.to_s,"")
assertEqual(cur.getField(0,3),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# long lobs
print "LONG LOBS: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	testclob text, "+
	"	testblob image) lock datarows")
cur.prepareQuery("insert into testtable values (?,?)")
largebuffer = "C" * 8192
cur.inputBindClob("1",largebuffer,8192)
cur.inputBindBlob("2",largebuffer,8192)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getFieldLength(0,"testclob"),8192)
assertEqual(cur.getField(0,"testclob"),largebuffer)
assertEqual(cur.getFieldLength(0,"testblob"),8192)
assertEqualLen(cur.getField(0,"testblob"),largebuffer,8192)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


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
print "NEGATIVE INPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery("create table testtable (testval int)")
cur.prepareQuery("insert into testtable values (@testval)")
cur.inputBind("testval",-1)
assertTrue(cur.executeQuery())
cur.sendQuery("select testval from testtable")
assertEqual(cur.getField(0,"testval"),"-1")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# bind validation
print "BIND VALIDATION: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	col1 varchar(20), "+
	"	col2 varchar(20), "+
	"	col3 varchar(20))")
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$(var1), "+
	"	$(var2), "+
	"	$(var3))")
cur.inputBind("var1","1")
cur.inputBind("var2","2")
cur.inputBind("var3","3")
cur.substitution("var1","@var1")
assertTrue(cur.validBind("var1"))
assertFalse(cur.validBind("var2"))
assertFalse(cur.validBind("var3"))
assertFalse(cur.validBind("var4"))
print "\n"
cur.substitution("var2","@var2")
assertTrue(cur.validBind("var1"))
assertTrue(cur.validBind("var2"))
assertFalse(cur.validBind("var3"))
assertFalse(cur.validBind("var4"))
print "\n"
cur.substitution("var3","@var3")
assertTrue(cur.validBind("var1"))
assertTrue(cur.validBind("var2"))
assertTrue(cur.validBind("var3"))
assertFalse(cur.validBind("var4"))
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# rebinding
# FreeTDS needs to support cursors for this to work


# reexecute
print "REEXECUTE: \n"
cur.prepareQuery("select 1")
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.prepareQuery("select cast(? as int)")
cur.inputBind("1",1)
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.inputBind("1",2)
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"2")
print "\n"


# stored procedure returning no value
# FreeTDS needs to support cursors for this to work


# stored procedure returning single value
# FreeTDS needs to support cursors for this to work


# stored procedure returning multiple values
# FreeTDS needs to support cursors for this to work


# stored procedure returning result set
print "STORED PROCEDURE RETURNING RESULT SET: \n"
cur.sendQuery("drop procedure testselectproc")
assertTrue(cur.sendQuery(
	"create procedure testselectproc as "+
	"       select 1 "+
	"       union "+
	"       select 2 "+
	"       union "+
	"       select 3 "+
	"       union "+
	"       select 4 "+
	"       union "+
	"       select 5 "+
	"       union "+
	"       select 6 "+
	"       union "+
	"       select 7 "+
	"       union "+
	"       select 8"))
assertTrue(cur.sendQuery("exec testselectproc"))
assertEqual(cur.rowCount(),8)
assertTrue(cur.sendQuery("drop procedure testselectproc"))
print "\n"


# temporary tables
print "TEMPORARY TABLES: \n"
cur.sendQuery("drop table #temptable")
cur.sendQuery("create table #temptable (col1 int)")
assertTrue(cur.sendQuery("insert into #temptable values (1)"))
assertTrue(cur.sendQuery("select count(*) from #temptable"))
assertEqual(cur.getField(0,0),"1")
con.endSession()
print "\n"
assertFalse(cur.sendQuery("select count(*) from #temptable"))
print "\n"


# encoded binary data
print "ENCODED BINARY DATA: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 image)"))
buffer = (0..255).map { |j| j.chr }.join
query = "insert into testtable values (0x"
query = query + buffer.bytes.map { |b| "%02x" % b }.join
query = query + ")"
assertTrue(cur.sendQuery(query))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),buffer.length)
assertEqualLen(cur.getField(0,0),buffer,buffer.length)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes
print "QUOTES: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('''''')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"''")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# last insert id
print "LAST INSERT ID: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
		"create table testtable "+
		"	(col1 int identity primary key, "+
		"	col2 int)"))
assertTrue(cur.sendQuery(
		"insert into testtable (col2) values (1)"))
assertEqual(con.getLastInsertId(),1)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# database is schema
print "DATABASE IS SCHEMA: \n"
assertFalse(con.getDatabaseIsSchema())
print "\n"


# catalog list
print "CATALOG LIST: \n"
assertTrue(cur.getCatalogList(nil))
assertEqual(cur.getColumnName(0),"Database")
assertInResultSet(cur,"Database",hostname)
print "\n"


# schema list
print "SCHEMA LIST: \n"
cur.sendQuery("drop table testtable")
# the get schema list query that is used with sap will only return the
# names of schemas that have at least one database object in them, so
# to be sure that there is one, we'll create a table
assertTrue(cur.sendQuery("create table testtable (col1 int)"))
assertTrue(cur.getSchemaList(nil))
assertEqual(cur.getColumnName(0),"Database")
assertInResultSet(cur,"Database","dbo")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# table type list
print "TABLE TYPE LIST: \n"
assertTrue(cur.getTableTypeList())
assertEqual(cur.getColumnName(0),"table_type")
assertInResultSet(cur,"table_type","TABLE")
print "\n"


# table list
print "TABLE LIST: \n"
cur.sendQuery("drop table testtable1")
cur.sendQuery("drop table testtable2")
cur.sendQuery("drop table testtable3")
cur.sendQuery("drop table testtable4")
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	col1 int, "+
	"	col2 int)"))
assertTrue(cur.getTableList(nil))
assertInResultSet(cur,"Tables_in_xxx","testtable1")
assertInResultSet(cur,"Tables_in_xxx","testtable2")
assertInResultSet(cur,"Tables_in_xxx","testtable3")
assertInResultSet(cur,"Tables_in_xxx","testtable4")
assertTrue(cur.sendQuery("drop table testtable1"))
assertTrue(cur.sendQuery("drop table testtable2"))
assertTrue(cur.sendQuery("drop table testtable3"))
assertTrue(cur.sendQuery("drop table testtable4"))
print "\n"


# type info list
print "TYPE INFO LIST: \n"
assertTrue(cur.getTypeInfoList("int"))
assertEqual(cur.getColumnName(0),"type_name")
assertEqual(cur.getColumnName(1),"data_type")
assertEqual(cur.getColumnName(2),"precision")
assertEqual(cur.getColumnName(3),"literal_prefix")
assertEqual(cur.getColumnName(4),"literal_suffix")
assertEqual(cur.getColumnName(5),"create_params")
assertEqual(cur.getColumnName(6),"nullable")
assertEqual(cur.getColumnName(7),"case_sensitive")
assertEqual(cur.getColumnName(8),"searchable")
assertEqual(cur.getColumnName(9),"unsigned_attribute")
assertEqual(cur.getColumnName(10),"fixed_prec_scale")
assertEqual(cur.getColumnName(11),"auto_increment")
assertEqual(cur.getColumnName(12),"local_type_name")
assertEqual(cur.getColumnName(13),"minumum_scale")
assertEqual(cur.getColumnName(14),"maxiumm_scale")
assertEqual(cur.getColumnName(15),"sql_data_type")
assertEqual(cur.getColumnName(16),"sql_datetime_sub")
assertEqual(cur.getColumnName(17),"num_prec_radix")
assertEqual(cur.getColumnName(18),"interval_precision")
assertEqual(cur.getField(0,"type_name"),"INT")
assertEqual(cur.getField(0,"data_type"),"4")
assertEqual(cur.getField(0,"precision"),"10")
assertEqual(cur.getField(0,"local_type_name"),"INT")
assertTrue(cur.getTypeInfoList("char"))
assertEqual(cur.getField(0,"type_name"),"CHAR")
assertEqual(cur.getField(0,"data_type"),"1")
assertEqual(cur.getField(0,"precision"),"8000")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"8000")
assertEqual(cur.getField(0,"local_type_name"),"VARCHAR")
assertTrue(cur.getTypeInfoList("datetime"))
assertEqual(cur.getField(0,"type_name"),"DATETIME")
assertEqual(cur.getField(0,"data_type"),"93")
assertEqual(cur.getField(0,"precision"),"23")
assertEqual(cur.getField(0,"local_type_name"),"DATETIME")
print "\n"


# column list
print "COLUMN LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testsmallint smallint, "+
	"	testtinyint tinyint, "+
	"	testreal real, "+
	"	testfloat float, "+
	"	testdecimal decimal(4,1), "+
	"	testnumeric numeric(4,1), "+
	"	testmoney money, "+
	"	testsmallmoney smallmoney, "+
	"	testdatetime datetime, "+
	"	testsmalldatetime smalldatetime, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testbit bit, "+
	"	testdate date, "+
	"	testtime time, "+
	"	testbigdatetime bigdatetime, "+
	"	testbigtime bigtime)"))
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getColumnName(0),"column_name")
assertEqual(cur.getColumnName(1),"data_type")
assertEqual(cur.getColumnName(2),"character_maximum_length")
assertEqual(cur.getColumnName(3),"numeric_precision")
assertEqual(cur.getColumnName(4),"numeric_scale")
assertEqual(cur.getColumnName(5),"is_nullable")
assertEqual(cur.getColumnName(6),"column_key")
assertEqual(cur.getColumnName(7),"column_default")
assertEqual(cur.getColumnName(8),"extra")
assertTrue(cur.getField(0,"column_name")=="testint")
assertTrue(cur.getField(1,"column_name")=="testsmallint")
assertTrue(cur.getField(2,"column_name")=="testtinyint")
assertTrue(cur.getField(3,"column_name")=="testreal")
assertTrue(cur.getField(4,"column_name")=="testfloat")
assertTrue(cur.getField(5,"column_name")=="testdecimal")
assertTrue(cur.getField(6,"column_name")=="testnumeric")
assertTrue(cur.getField(7,"column_name")=="testmoney")
assertTrue(cur.getField(8,"column_name")=="testsmallmoney")
assertTrue(cur.getField(9,"column_name")=="testdatetime")
assertTrue(cur.getField(10,"column_name")=="testsmalldatetime")
assertTrue(cur.getField(11,"column_name")=="testchar")
assertTrue(cur.getField(12,"column_name")=="testvarchar")
assertTrue(cur.getField(13,"column_name")=="testbit")
assertTrue(cur.getField(14,"column_name")=="testdate")
assertTrue(cur.getField(15,"column_name")=="testtime")
assertTrue(cur.getField(16,"column_name")=="testbigdatetime")
assertTrue(cur.getField(17,"column_name")=="testbigtime")
assertTrue(cur.getField(0,"data_type")=="int")
assertTrue(cur.getField(1,"data_type")=="smallint")
assertTrue(cur.getField(2,"data_type")=="tinyint")
assertTrue(cur.getField(3,"data_type")=="real")
assertTrue(cur.getField(4,"data_type")=="float")
assertTrue(cur.getField(5,"data_type")=="decimal")
assertTrue(cur.getField(6,"data_type")=="numeric")
assertTrue(cur.getField(7,"data_type")=="money")
assertTrue(cur.getField(8,"data_type")=="smallmoney")
assertTrue(cur.getField(9,"data_type")=="datetime")
assertTrue(cur.getField(10,"data_type")=="smalldatetime")
assertTrue(cur.getField(11,"data_type")=="char")
assertTrue(cur.getField(12,"data_type")=="varchar")
assertTrue(cur.getField(13,"data_type")=="bit")
assertTrue(cur.getField(14,"data_type")=="date")
assertTrue(cur.getField(15,"data_type")=="time")
assertTrue(cur.getField(16,"data_type")=="bigdatetime")
assertTrue(cur.getField(17,"data_type")=="bigtime")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# column list - auto_increment, primary key
print "COLUMN LIST - auto_increment, primary key: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int identity primary key, "+
	"	col2 int)"))
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getField(0,"extra"),"auto_increment")
assertEqual(cur.getField(0,"column_key"),"PRI")
assertEqual(cur.getField(1,"extra"),"")
assertEqual(cur.getField(1,"column_key"),"")
print "\n"
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getField(0,"extra"),"")
assertEqual(cur.getField(0,"column_key"),"PRI")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# primary keys list
print "PRIMARY KEYS LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getPrimaryKeysList("testtable",nil))
assertEqual(cur.getColumnName(0),"table")
assertEqual(cur.getColumnName(1),"non_unique")
assertEqual(cur.getColumnName(2),"key_name")
assertEqual(cur.getColumnName(3),"seq_in_index")
assertEqual(cur.getColumnName(4),"column_name")
assertEqual(cur.getColumnName(5),"collation")
assertEqual(cur.getColumnName(6),"cardinality")
assertEqual(cur.getColumnName(7),"sub_part")
assertEqual(cur.getColumnName(8),"packed")
assertEqual(cur.getColumnName(9),"null")
assertEqual(cur.getColumnName(10),"index_type")
assertEqual(cur.getColumnName(11),"comment")
assertEqual(cur.getColumnName(12),"index_comment")
assertEqual(cur.rowCount(),1)
assertTrue(cur.getField(0,"table")=="testtable")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertTrue(cur.getField(0,"column_name")=="col1")
keyname=cur.getField(0,"key_name")
assertStartsWith(keyname,"testtable_col1_")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# key and index list
print "KEY AND INDEX LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getKeyAndIndexList("testtable",nil))
assertEqual(cur.getColumnName(0),"table")
assertEqual(cur.getColumnName(1),"non_unique")
assertEqual(cur.getColumnName(2),"key_name")
assertEqual(cur.getColumnName(3),"seq_in_index")
assertEqual(cur.getColumnName(4),"column_name")
assertEqual(cur.getColumnName(5),"collation")
assertEqual(cur.getColumnName(6),"cardinality")
assertEqual(cur.getColumnName(7),"sub_part")
assertEqual(cur.getColumnName(8),"packed")
assertEqual(cur.getColumnName(9),"null")
assertEqual(cur.getColumnName(10),"index_type")
assertEqual(cur.getColumnName(11),"comment")
assertEqual(cur.getColumnName(12),"index_comment")
assertEqual(cur.rowCount(),1)
assertTrue(cur.getField(0,"table")=="testtable")
assertEqual(cur.getField(0,"non_unique"),"0")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertTrue(cur.getField(0,"column_name")=="col1")
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"1")
keyname=cur.getField(0,"key_name")
assertStartsWith(keyname,"testtable_col1_")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
cur.sendQuery("drop procedure testproc1")
cur.sendQuery("drop procedure testproc2")
cur.sendQuery("drop procedure testproc3")
cur.sendQuery("drop procedure testproc4")
assertTrue(cur.sendQuery(
	"create procedure testproc1 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+
	"as select 1"))
assertTrue(cur.sendQuery(
	"create procedure testproc2 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+
	"as select 1"))
assertTrue(cur.sendQuery(
	"create procedure testproc3 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+
	"as select 1"))
assertTrue(cur.sendQuery(
	"create procedure testproc4 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+
	"as select 1"))
assertTrue(cur.getProcedureList(nil))
assertInResultSet(cur,"routine_name","testproc1")
assertInResultSet(cur,"routine_name","testproc2")
assertInResultSet(cur,"routine_name","testproc3")
assertInResultSet(cur,"routine_name","testproc4")
print "\n"


# procedure parameter list
print "PROCEDURE PARAMETER LIST: \n"
assertTrue(cur.getProcedureParameterList("testproc1",nil))
assertEqual(cur.getColumnName(0),"parameter_name")
assertEqual(cur.getColumnName(1),"parameter_mode")
assertEqual(cur.getColumnName(2),"data_type")
assertEqual(cur.getColumnName(3),"character_maximum_length")
assertEqual(cur.getColumnName(4),"ordinal_position")
assertEqual(cur.rowCount(),4)
assertEqual(cur.getField(0,"parameter_name"),"@in1")
assertEqual(cur.getField(0,"parameter_mode"),"1")
assertEqual(cur.getField(0,"data_type"),"int")
assertEqual(cur.getField(0,"ordinal_position"),"1")
assertEqual(cur.getField(1,"parameter_name"),"@in2")
assertEqual(cur.getField(1,"parameter_mode"),"1")
assertEqual(cur.getField(1,"data_type"),"char")
assertEqual(cur.getField(1,"ordinal_position"),"2")
assertEqual(cur.getField(2,"parameter_name"),"@in3")
assertEqual(cur.getField(2,"parameter_mode"),"1")
assertEqual(cur.getField(2,"data_type"),"varchar")
assertEqual(cur.getField(2,"ordinal_position"),"3")
assertEqual(cur.getField(3,"parameter_name"),"@in4")
assertEqual(cur.getField(3,"parameter_mode"),"1")
assertEqual(cur.getField(3,"data_type"),"datetime")
assertEqual(cur.getField(3,"ordinal_position"),"4")
assertTrue(cur.sendQuery("drop procedure testproc1"))
assertTrue(cur.sendQuery("drop procedure testproc2"))
assertTrue(cur.sendQuery("drop procedure testproc3"))
assertTrue(cur.sendQuery("drop procedure testproc4"))
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery("select * from testtable order by testint"))
assertFalse(cur.sendQuery("select * from testtable order by testint"))
assertFalse(cur.sendQuery("select * from testtable order by testint"))
assertFalse(cur.sendQuery("select * from testtable order by testint"))
print "\n"
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
print "\n"
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
assertFalse(cur.sendQuery("create table testtable"))
print "\n"

reportTestStatus()

exit($status)
