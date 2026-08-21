#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'socket'
require 'sqlrelay'
require './asserts'




# hostname
hostname=Socket.gethostname.split(".")[0].downcase


# instantiation
con=SQLRConnection.new("sqlrelay",9010,"/tmp/informix.socket",
						"testuser","testpassword",0,1)
cur=SQLRCursor.new(con)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"informix")
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
print "\n"


# transaction state
print "TRANSACTION STATE: \n"
assertEqual(con.getDefaultTransactionModel(),"implicit")
assertEqual(con.getTransactionModel(),"implicit")
assertTrue(con.getInTransaction())
assertFalse(con.getAutoCommit())
print "\n"


# bind format
print "BIND FORMAT: \n"
assertEqual(con.bindFormat(),"?")
print "\n"


# nextval format
print "NEXTVAL FORMAT: \n"
assertEqual(con.nextvalFormat(),"%s.nextval")
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
isolationlevels=["committed read","dirty read",
			"cursor stability","repeatable read"]
for il in isolationlevels
	# you can set the isolation level, but to get it, you have to
	# have permissions to read from sysmaster:syssqlcurses
	assertTrue(con.setIsolationLevel(il))
	print "\n"
end
# reset to the default isolation level
assertTrue(con.setIsolationLevel(isolationlevels[0]))
print "\n"


# create testtable
print "CREATE TESTTABLE: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testboolean boolean, "+
	"	testsmallint smallint, "+
	"	testint integer, "+
	"	testbigint bigint, "+
	"	testint8 int8, "+
	"	testdecimal decimal(10,2), "+
	"	testmoney money, "+
	"	testsmallfloat smallfloat, "+
	"	testfloat float, "+
	"	testchar char(40), "+
	"	testnchar nchar(40), "+
	"	testvarchar varchar(40), "+
	"	testnvarchar nvarchar(40), "+
	"	testlvarchar lvarchar(40), "+
	"	testdate date, "+
	"	testdatetime datetime year to second, "+
	"	testtext text, "+
	"	testbyte byte)"))
assertTrue(con.commit())
print "\n"


# insert
print "INSERT: \n"
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	't', "+
	"	1, "+
	"	1, "+
	"	1, "+
	"	1, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	'testchar1', "+
	"	'testnchar1', "+
	"	'testvarchar1', "+
	"	'testnvarchar1', "+
	"	'testlvarchar1', "+
	"	'01/01/2001', "+
	"	'2001-01-01 01:00:00', "+
	"	'testtext1', "+
	"	null)"))
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
cur.inputBindDate("15",2002,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("16",2002,1,1,2,0,0,0,"",0)
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
cur.inputBindDate("15",2003,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("16",2003,1,1,3,0,0,0,"",0)
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
cur.inputBindDate("15",2004,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("16",2004,1,1,4,0,0,0,"",0)
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
cur.inputBindDate("15",2005,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("16",2005,1,1,5,0,0,0,"",0)
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
cur.inputBindDate("15",2006,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("16",2006,1,1,6,0,0,0,"",0)
cur.inputBindClob("17","testtext6",9)
cur.inputBindBlob("18","testbyte6",9)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by position
print "ARRAY OF INPUT BINDS BY POSITION: \n"
cur.clearBinds()
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
	"	null, "+
	"	null)")
cur.inputBinds(["1","2","3","4",
		"5","6","7","8","9","10",
		"11","12","13","14","15","16"],
		["t","7","7","7","7",
		"7.5","7.5","7.5","7.5",
		"testchar7","testnchar7",
		"testvarchar7","testnvarchar7",
		"testlvarchar7","01/01/2007",
		"2007-01-01 07:00:00"])
assertTrue(cur.executeQuery())
print "\n"


# input bind by position with validation
print "INPUT BIND BY POSITION WITH VALIDATION: \n"
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
cur.inputBindDate("15",2008,1,1,-1,-1,-1,-1,"",0)
cur.inputBindDate("16",2008,1,1,8,0,0,0,"",0)
cur.inputBindClob("17","testtext8",9)
cur.inputBindBlob("18","testbyte8",9)
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# input bind by name
# informix doesn't support bind by name


# array of input binds by name
# informix doesn't support bind by name


# input bind by name with validation
# informix doesn't support bind by name


# select
print "SELECT: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),18)
print "\n"


# column names
print "COLUMN NAMES: \n"
assertEqual(cur.getColumnName(0),"testboolean")
assertEqual(cur.getColumnName(1),"testsmallint")
assertEqual(cur.getColumnName(2),"testint")
assertEqual(cur.getColumnName(3),"testbigint")
assertEqual(cur.getColumnName(4),"testint8")
assertEqual(cur.getColumnName(5),"testdecimal")
assertEqual(cur.getColumnName(6),"testmoney")
assertEqual(cur.getColumnName(7),"testsmallfloat")
assertEqual(cur.getColumnName(8),"testfloat")
assertEqual(cur.getColumnName(9),"testchar")
assertEqual(cur.getColumnName(10),"testnchar")
assertEqual(cur.getColumnName(11),"testvarchar")
assertEqual(cur.getColumnName(12),"testnvarchar")
assertEqual(cur.getColumnName(13),"testlvarchar")
assertEqual(cur.getColumnName(14),"testdate")
assertEqual(cur.getColumnName(15),"testdatetime")
assertEqual(cur.getColumnName(16),"testtext")
assertEqual(cur.getColumnName(17),"testbyte")
cols=cur.getColumnNames()
assertEqual(cols[0],"testboolean")
assertEqual(cols[1],"testsmallint")
assertEqual(cols[2],"testint")
assertEqual(cols[3],"testbigint")
assertEqual(cols[4],"testint8")
assertEqual(cols[5],"testdecimal")
assertEqual(cols[6],"testmoney")
assertEqual(cols[7],"testsmallfloat")
assertEqual(cols[8],"testfloat")
assertEqual(cols[9],"testchar")
assertEqual(cols[10],"testnchar")
assertEqual(cols[11],"testvarchar")
assertEqual(cols[12],"testnvarchar")
assertEqual(cols[13],"testlvarchar")
assertEqual(cols[14],"testdate")
assertEqual(cols[15],"testdatetime")
assertEqual(cols[16],"testtext")
assertEqual(cols[17],"testbyte")
print "\n"


# column types
print "COLUMN TYPES: \n"
assertEqual(cur.getColumnType(0),"BOOLEAN")
assertEqual(cur.getColumnType("testboolean"),"BOOLEAN")
assertEqual(cur.getColumnType(1),"SMALLINT")
assertEqual(cur.getColumnType("testsmallint"),"SMALLINT")
assertEqual(cur.getColumnType(2),"INTEGER")
assertEqual(cur.getColumnType("testint"),"INTEGER")
assertEqual(cur.getColumnType(3),"BIGINT")
assertEqual(cur.getColumnType("testbigint"),"BIGINT")
assertEqual(cur.getColumnType(4),"INT8")
assertEqual(cur.getColumnType("testint8"),"INT8")
assertEqual(cur.getColumnType(5),"DECIMAL")
assertEqual(cur.getColumnType("testdecimal"),"DECIMAL")
assertEqual(cur.getColumnType(6),"MONEY")
assertEqual(cur.getColumnType("testmoney"),"MONEY")
assertEqual(cur.getColumnType(7),"SMALLFLOAT")
assertEqual(cur.getColumnType("testsmallfloat"),"SMALLFLOAT")
assertEqual(cur.getColumnType(8),"FLOAT")
assertEqual(cur.getColumnType("testfloat"),"FLOAT")
assertEqual(cur.getColumnType(9),"CHAR")
assertEqual(cur.getColumnType("testchar"),"CHAR")
# informix reports nchar as char, with no way to tell them apart
assertEqual(cur.getColumnType(10),"CHAR")
assertEqual(cur.getColumnType("testnchar"),"CHAR")
assertEqual(cur.getColumnType(11),"VARCHAR")
assertEqual(cur.getColumnType("testvarchar"),"VARCHAR")
# informix reports nvarchar as varchar, with no way to tell them apart
assertEqual(cur.getColumnType(12),"VARCHAR")
assertEqual(cur.getColumnType("testnvarchar"),"VARCHAR")
assertEqual(cur.getColumnType(13),"LVARCHAR")
assertEqual(cur.getColumnType("testlvarchar"),"LVARCHAR")
assertEqual(cur.getColumnType(14),"DATE")
assertEqual(cur.getColumnType("testdate"),"DATE")
assertEqual(cur.getColumnType(15),"DATETIME")
assertEqual(cur.getColumnType("testdatetime"),"DATETIME")
assertEqual(cur.getColumnType(16),"TEXT")
assertEqual(cur.getColumnType("testtext"),"TEXT")
assertEqual(cur.getColumnType(17),"BYTE")
assertEqual(cur.getColumnType("testbyte"),"BYTE")
print "\n"


# column length
print "COLUMN LENGTH: \n"
assertEqual(cur.getColumnLength(0),1)
assertEqual(cur.getColumnLength("testboolean"),1)
assertEqual(cur.getColumnLength(1),5)
assertEqual(cur.getColumnLength("testsmallint"),5)
assertEqual(cur.getColumnLength(2),10)
assertEqual(cur.getColumnLength("testint"),10)
assertEqual(cur.getColumnLength(3),20)
assertEqual(cur.getColumnLength("testbigint"),20)
assertEqual(cur.getColumnLength(4),20)
assertEqual(cur.getColumnLength("testint8"),20)
assertEqual(cur.getColumnLength(5),10)
assertEqual(cur.getColumnLength("testdecimal"),10)
assertEqual(cur.getColumnLength(6),16)
assertEqual(cur.getColumnLength("testmoney"),16)
assertEqual(cur.getColumnLength(7),7)
assertEqual(cur.getColumnLength("testsmallfloat"),7)
assertEqual(cur.getColumnLength(8),15)
assertEqual(cur.getColumnLength("testfloat"),15)
assertEqual(cur.getColumnLength(9),40)
assertEqual(cur.getColumnLength("testchar"),40)
assertEqual(cur.getColumnLength(10),40)
assertEqual(cur.getColumnLength("testnchar"),40)
assertEqual(cur.getColumnLength(11),40)
assertEqual(cur.getColumnLength("testvarchar"),40)
assertEqual(cur.getColumnLength(12),40)
assertEqual(cur.getColumnLength("testnvarchar"),40)
assertEqual(cur.getColumnLength(13),40)
assertEqual(cur.getColumnLength("testlvarchar"),40)
assertEqual(cur.getColumnLength(14),10)
assertEqual(cur.getColumnLength("testdate"),10)
assertEqual(cur.getColumnLength(15),19)
assertEqual(cur.getColumnLength("testdatetime"),19)
assertEqual(cur.getColumnLength(16),2147483647)
assertEqual(cur.getColumnLength("testtext"),2147483647)
assertEqual(cur.getColumnLength(17),2147483647)
assertEqual(cur.getColumnLength("testbyte"),2147483647)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest("testboolean"),1)
assertEqual(cur.getLongest(1),1)
assertEqual(cur.getLongest("testsmallint"),1)
assertEqual(cur.getLongest(2),1)
assertEqual(cur.getLongest("testint"),1)
assertEqual(cur.getLongest(3),1)
assertEqual(cur.getLongest("testbigint"),1)
assertEqual(cur.getLongest(4),1)
assertEqual(cur.getLongest("testint8"),1)
assertEqual(cur.getLongest(5),4)
assertEqual(cur.getLongest("testdecimal"),4)
assertEqual(cur.getLongest(6),4)
assertEqual(cur.getLongest("testmoney"),4)
assertEqual(cur.getLongest(7),3)
assertEqual(cur.getLongest("testsmallfloat"),3)
assertEqual(cur.getLongest(8),3)
assertEqual(cur.getLongest("testfloat"),3)
assertEqual(cur.getLongest(9),40)
assertEqual(cur.getLongest("testchar"),40)
assertEqual(cur.getLongest(10),40)
assertEqual(cur.getLongest("testnchar"),40)
assertEqual(cur.getLongest(11),12)
assertEqual(cur.getLongest("testvarchar"),12)
assertEqual(cur.getLongest(12),13)
assertEqual(cur.getLongest("testnvarchar"),13)
assertEqual(cur.getLongest(13),13)
assertEqual(cur.getLongest("testlvarchar"),13)
assertEqual(cur.getLongest(14),10)
assertEqual(cur.getLongest("testdate"),10)
assertEqual(cur.getLongest(15),19)
assertEqual(cur.getLongest("testdatetime"),19)
assertEqual(cur.getLongest(16),9)
assertEqual(cur.getLongest("testtext"),9)
assertEqual(cur.getLongest(17),9)
assertEqual(cur.getLongest("testbyte"),9)
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
assertEqual(cur.getField(0,3),"1")
assertEqual(cur.getField(0,4),"1")
assertEqual(cur.getField(0,5),"1.50")
assertEqual(cur.getField(0,6),"1.50")
assertEqual(cur.getField(0,7),"1.5")
assertEqual(cur.getField(0,8),"1.5")
assertEqual(cur.getField(0,9),"testchar1                               ")
assertEqual(cur.getField(0,10),"testnchar1                              ")
assertEqual(cur.getField(0,11),"testvarchar1")
assertEqual(cur.getField(0,12),"testnvarchar1")
assertEqual(cur.getField(0,13),"testlvarchar1")
assertEqual(cur.getField(0,14),"2001-01-01")
assertEqual(cur.getField(0,15),"2001-01-01 01:00:00")
assertEqual(cur.getField(0,16),"testtext1")
assertEqual(cur.getField(0,17),"")
print "\n"
assertEqual(cur.getField(7,0),"1")
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(7,2),"8")
assertEqual(cur.getField(7,3),"8")
assertEqual(cur.getField(7,4),"8")
assertEqual(cur.getField(7,5),"8.50")
assertEqual(cur.getField(7,6),"8.50")
assertEqual(cur.getField(7,7),"8.5")
assertEqual(cur.getField(7,8),"8.5")
assertEqual(cur.getField(7,9),"testchar8                               ")
assertEqual(cur.getField(7,10),"testnchar8                              ")
assertEqual(cur.getField(7,11),"testvarchar8")
assertEqual(cur.getField(7,12),"testnvarchar8")
assertEqual(cur.getField(7,13),"testlvarchar8")
assertEqual(cur.getField(7,14),"2008-01-01")
assertEqual(cur.getField(7,15),"2008-01-01 08:00:00")
assertEqual(cur.getField(7,16),"")
assertEqual(cur.getField(7,17),"")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),1)
assertEqual(cur.getFieldLength(0,2),1)
assertEqual(cur.getFieldLength(0,3),1)
assertEqual(cur.getFieldLength(0,4),1)
assertEqual(cur.getFieldLength(0,5),4)
assertEqual(cur.getFieldLength(0,6),4)
assertEqual(cur.getFieldLength(0,7),3)
assertEqual(cur.getFieldLength(0,8),3)
assertEqual(cur.getFieldLength(0,9),40)
assertEqual(cur.getFieldLength(0,10),40)
assertEqual(cur.getFieldLength(0,11),12)
assertEqual(cur.getFieldLength(0,12),13)
assertEqual(cur.getFieldLength(0,14),10)
assertEqual(cur.getFieldLength(0,15),19)
assertEqual(cur.getFieldLength(0,16),9)
assertEqual(cur.getFieldLength(0,17),0)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),1)
assertEqual(cur.getFieldLength(7,2),1)
assertEqual(cur.getFieldLength(7,3),1)
assertEqual(cur.getFieldLength(7,4),1)
assertEqual(cur.getFieldLength(7,5),4)
assertEqual(cur.getFieldLength(7,6),4)
assertEqual(cur.getFieldLength(7,7),3)
assertEqual(cur.getFieldLength(7,8),3)
assertEqual(cur.getFieldLength(7,9),40)
assertEqual(cur.getFieldLength(7,10),40)
assertEqual(cur.getFieldLength(7,11),12)
assertEqual(cur.getFieldLength(7,12),13)
assertEqual(cur.getFieldLength(7,14),10)
assertEqual(cur.getFieldLength(7,15),19)
assertEqual(cur.getFieldLength(7,16),0)
assertEqual(cur.getFieldLength(7,17),0)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"testboolean"),"1")
assertEqual(cur.getField(0,"testsmallint"),"1")
assertEqual(cur.getField(0,"testint"),"1")
assertEqual(cur.getField(0,"testbigint"),"1")
assertEqual(cur.getField(0,"testint8"),"1")
assertEqual(cur.getField(0,"testdecimal"),"1.50")
assertEqual(cur.getField(0,"testmoney"),"1.50")
assertEqual(cur.getField(0,"testsmallfloat"),"1.5")
assertEqual(cur.getField(0,"testfloat"),"1.5")
assertEqual(cur.getField(0,"testchar"),"testchar1                               ")
assertEqual(cur.getField(0,"testnchar"),"testnchar1                              ")
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1")
assertEqual(cur.getField(0,"testnvarchar"),"testnvarchar1")
assertEqual(cur.getField(0,"testlvarchar"),"testlvarchar1")
assertEqual(cur.getField(0,"testdate"),"2001-01-01")
assertEqual(cur.getField(0,"testdatetime"),"2001-01-01 01:00:00")
assertEqual(cur.getField(0,"testtext"),"testtext1")
assertEqual(cur.getField(0,"testbyte"),"")
print "\n"
assertEqual(cur.getField(7,"testboolean"),"1")
assertEqual(cur.getField(7,"testsmallint"),"8")
assertEqual(cur.getField(7,"testint"),"8")
assertEqual(cur.getField(7,"testbigint"),"8")
assertEqual(cur.getField(7,"testint8"),"8")
assertEqual(cur.getField(7,"testdecimal"),"8.50")
assertEqual(cur.getField(7,"testmoney"),"8.50")
assertEqual(cur.getField(7,"testsmallfloat"),"8.5")
assertEqual(cur.getField(7,"testfloat"),"8.5")
assertEqual(cur.getField(7,"testchar"),"testchar8                               ")
assertEqual(cur.getField(7,"testnchar"),"testnchar8                              ")
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8")
assertEqual(cur.getField(7,"testnvarchar"),"testnvarchar8")
assertEqual(cur.getField(7,"testlvarchar"),"testlvarchar8")
assertEqual(cur.getField(7,"testdate"),"2008-01-01")
assertEqual(cur.getField(7,"testdatetime"),"2008-01-01 08:00:00")
assertEqual(cur.getField(7,"testtext"),"")
assertEqual(cur.getField(7,"testbyte"),"")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"testboolean"),1)
assertEqual(cur.getFieldLength(0,"testsmallint"),1)
assertEqual(cur.getFieldLength(0,"testint"),1)
assertEqual(cur.getFieldLength(0,"testbigint"),1)
assertEqual(cur.getFieldLength(0,"testint8"),1)
assertEqual(cur.getFieldLength(0,"testdecimal"),4)
assertEqual(cur.getFieldLength(0,"testmoney"),4)
assertEqual(cur.getFieldLength(0,"testsmallfloat"),3)
assertEqual(cur.getFieldLength(0,"testfloat"),3)
assertEqual(cur.getFieldLength(0,"testchar"),40)
assertEqual(cur.getFieldLength(0,"testnchar"),40)
assertEqual(cur.getFieldLength(0,"testvarchar"),12)
assertEqual(cur.getFieldLength(0,"testnvarchar"),13)
assertEqual(cur.getFieldLength(0,"testlvarchar"),13)
assertEqual(cur.getFieldLength(0,"testdate"),10)
assertEqual(cur.getFieldLength(0,"testdatetime"),19)
assertEqual(cur.getFieldLength(0,"testtext"),9)
assertEqual(cur.getFieldLength(0,"testbyte"),0)
print "\n"
assertEqual(cur.getFieldLength(7,"testboolean"),1)
assertEqual(cur.getFieldLength(7,"testsmallint"),1)
assertEqual(cur.getFieldLength(7,"testint"),1)
assertEqual(cur.getFieldLength(7,"testbigint"),1)
assertEqual(cur.getFieldLength(7,"testint8"),1)
assertEqual(cur.getFieldLength(7,"testdecimal"),4)
assertEqual(cur.getFieldLength(7,"testmoney"),4)
assertEqual(cur.getFieldLength(7,"testsmallfloat"),3)
assertEqual(cur.getFieldLength(7,"testfloat"),3)
assertEqual(cur.getFieldLength(7,"testchar"),40)
assertEqual(cur.getFieldLength(7,"testnchar"),40)
assertEqual(cur.getFieldLength(7,"testvarchar"),12)
assertEqual(cur.getFieldLength(7,"testnvarchar"),13)
assertEqual(cur.getFieldLength(7,"testlvarchar"),13)
assertEqual(cur.getFieldLength(7,"testdate"),10)
assertEqual(cur.getFieldLength(7,"testdatetime"),19)
assertEqual(cur.getFieldLength(7,"testtext"),0)
assertEqual(cur.getFieldLength(7,"testbyte"),0)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1")
assertEqual(fields[2],"1")
assertEqual(fields[3],"1")
assertEqual(fields[4],"1")
assertEqual(fields[5],"1.50")
assertEqual(fields[6],"1.50")
assertEqual(fields[7],"1.5")
assertEqual(fields[8],"1.5")
assertEqual(fields[9],"testchar1                               ")
assertEqual(fields[10],"testnchar1                              ")
assertEqual(fields[11],"testvarchar1")
assertEqual(fields[12],"testnvarchar1")
assertEqual(fields[13],"testlvarchar1")
assertEqual(fields[14],"2001-01-01")
assertEqual(fields[15],"2001-01-01 01:00:00")
assertEqual(fields[16],"testtext1")
assertEqual(fields[17],"")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],1)
assertEqual(fieldlens[2],1)
assertEqual(fieldlens[3],1)
assertEqual(fieldlens[4],1)
assertEqual(fieldlens[5],4)
assertEqual(fieldlens[6],4)
assertEqual(fieldlens[7],3)
assertEqual(fieldlens[8],3)
assertEqual(fieldlens[9],40)
assertEqual(fieldlens[10],40)
assertEqual(fieldlens[11],12)
assertEqual(fieldlens[12],13)
assertEqual(fieldlens[14],10)
assertEqual(fieldlens[15],19)
assertEqual(fieldlens[16],9)
assertEqual(fieldlens[17],0)
print "\n"


# result set buffer size
print "RESULT SET BUFFER SIZE: \n"
assertEqual(cur.getResultSetBufferSize(),0)
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertEqual(cur.getResultSetBufferSize(),2)
print "\n"
assertEqual(cur.firstRowIndex(),0)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),2)
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(1,1),"2")
assertEqual(cur.getField(2,1),"3")
print "\n"
assertEqual(cur.firstRowIndex(),2)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),4)
assertEqual(cur.getField(6,1),"7")
assertEqual(cur.getField(7,1),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,1),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.setResultSetBufferSize(0)
print "\n"


# dont get column info
print "DONT GET COLUMN INFO: \n"
cur.dontGetColumnInfo()
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertEqual(cur.getColumnName(1),nil)
assertEqual(cur.getColumnLength(1),0)
assertEqual(cur.getColumnType(1),nil)
cur.getColumnInfo()
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertEqual(cur.getColumnName(1),"testsmallint")
assertEqual(cur.getColumnLength(1),5)
assertEqual(cur.getColumnType(1),"SMALLINT")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
print "\n"
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(1,1),"2")
assertEqual(cur.getField(2,1),"3")
assertEqual(cur.getField(3,1),"4")
assertEqual(cur.getField(4,1),"5")
assertEqual(cur.getField(5,1),"6")
assertEqual(cur.getField(6,1),"7")
assertEqual(cur.getField(7,1),"8")
print "\n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
print "\n"
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(1,1),"2")
assertEqual(cur.getField(2,1),"3")
assertEqual(cur.getField(3,1),"4")
assertEqual(cur.getField(4,1),"5")
assertEqual(cur.getField(5,1),"6")
assertEqual(cur.getField(6,1),"7")
assertEqual(cur.getField(7,1),"8")
print "\n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
print "\n"
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(1,1),"2")
assertEqual(cur.getField(2,1),"3")
assertEqual(cur.getField(3,1),"4")
assertEqual(cur.getField(4,1),"5")
assertEqual(cur.getField(5,1),"6")
assertEqual(cur.getField(6,1),"7")
assertEqual(cur.getField(7,1),"8")
print "\n"


# suspended result set
print "SUSPENDED RESULT SET: \n"
cur.setResultSetBufferSize(2)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertEqual(cur.getField(2,1),"3")
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
assertEqual(cur.getField(7,1),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,1),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.setResultSetBufferSize(0)
print "\n"


# cached result set
print "CACHED RESULT SET: \n"
cur.cacheToFile("cachefile1-informix")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1-informix")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,1),"8")
print "\n"


# column count for cached result set
print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),18)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
assertEqual(cur.getColumnName(0),"testboolean")
assertEqual(cur.getColumnName(1),"testsmallint")
assertEqual(cur.getColumnName(2),"testint")
assertEqual(cur.getColumnName(3),"testbigint")
assertEqual(cur.getColumnName(4),"testint8")
assertEqual(cur.getColumnName(5),"testdecimal")
assertEqual(cur.getColumnName(6),"testmoney")
assertEqual(cur.getColumnName(7),"testsmallfloat")
assertEqual(cur.getColumnName(8),"testfloat")
assertEqual(cur.getColumnName(9),"testchar")
assertEqual(cur.getColumnName(10),"testnchar")
assertEqual(cur.getColumnName(11),"testvarchar")
assertEqual(cur.getColumnName(12),"testnvarchar")
assertEqual(cur.getColumnName(13),"testlvarchar")
assertEqual(cur.getColumnName(14),"testdate")
assertEqual(cur.getColumnName(15),"testdatetime")
assertEqual(cur.getColumnName(16),"testtext")
assertEqual(cur.getColumnName(17),"testbyte")
cols=cur.getColumnNames()
assertEqual(cols[0],"testboolean")
assertEqual(cols[1],"testsmallint")
assertEqual(cols[2],"testint")
assertEqual(cols[3],"testbigint")
assertEqual(cols[4],"testint8")
assertEqual(cols[5],"testdecimal")
assertEqual(cols[6],"testmoney")
assertEqual(cols[7],"testsmallfloat")
assertEqual(cols[8],"testfloat")
assertEqual(cols[9],"testchar")
assertEqual(cols[10],"testnchar")
assertEqual(cols[11],"testvarchar")
assertEqual(cols[12],"testnvarchar")
assertEqual(cols[13],"testlvarchar")
assertEqual(cols[14],"testdate")
assertEqual(cols[15],"testdatetime")
assertEqual(cols[16],"testtext")
assertEqual(cols[17],"testbyte")
print "\n"


# cached result set with result set buffer size
print "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1-informix")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1-informix")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(8,1),nil)
cur.setResultSetBufferSize(0)
print "\n"


# from one cache file to another
print "FROM ONE CACHE FILE TO ANOTHER: \n"
cur.cacheToFile("cachefile2-informix")
assertTrue(cur.openCachedResultSet("cachefile1-informix"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2-informix"))
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(8,1),nil)
print "\n"


# from one cache file to another with result set buffer size
print "FROM ONE CACHE FILE TO ANOTHER "+
	"WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile2-informix")
assertTrue(cur.openCachedResultSet("cachefile1-informix"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2-informix"))
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(8,1),nil)
cur.setResultSetBufferSize(0)
print "\n"


# cached result set with suspend and result set buffer size
print "CACHED RESULT SET WITH SUSPEND "+
	"AND RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1-informix")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertEqual(cur.getField(2,1),"3")
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1-informix")
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
assertEqual(cur.getField(7,1),"8")
print "\n"
assertEqual(cur.firstRowIndex(),6)
assertFalse(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
assertEqual(cur.getField(8,1),nil)
print "\n"
assertEqual(cur.firstRowIndex(),8)
assertTrue(cur.endOfResultSet())
assertEqual(cur.rowCount(),8)
cur.cacheOff()
print "\n"
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(8,1),nil)
cur.setResultSetBufferSize(0)
print "\n"


# finished suspended session
print "FINISHED SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery("select * from testtable order by testint"))
assertEqual(cur.getField(4,1),"5")
assertEqual(cur.getField(5,1),"6")
assertEqual(cur.getField(6,1),"7")
assertEqual(cur.getField(7,1),"8")
id=cur.getResultSetId()
cur.suspendResultSet()
assertTrue(con.suspendSession())
port=con.getConnectionPort()
socket=con.getConnectionSocket()
assertTrue(con.resumeSession(port,socket))
assertTrue(cur.resumeResultSet(id))
assertEqual(cur.getField(4,1),nil)
assertEqual(cur.getField(5,1),nil)
assertEqual(cur.getField(6,1),nil)
assertEqual(cur.getField(7,1),nil)
print "\n"


# nested selects
print "NESTED SELECTS: \n"
cur.setResultSetBufferSize(1)
assertTrue(cur.sendQuery("select * from testtable"))
secondcur=SQLRCursor.new(con)
secondcur.setResultSetBufferSize(1)
i=0
while cur.getRow(i)
	assertTrue(secondcur.sendQuery("select * from testtable"))
	i=i+1
end
secondcur.closeResultSet()
cur.setResultSetBufferSize(0)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# reset transaction state
print "RESET TRANSACTION STATE: \n"
assertTrue(con.commit())
assertEqual(con.getTransactionModel(),"implicit")
assertFalse(con.getAutoCommit())
print "\n"


# transaction behavior - implicit
# Informix has no MVCC option -- the isolation level is either dirty
# reads (where the second connection sees uncommitted rows) or
# committed read (where it blocks or errors on locked rows) -- so
# the visibility assertions below may need to be revisited
print "TRANSACTION BEHAVIOR - implicit: \n"
assertTrue(con.setTransactionModel("implicit"))
assertEqual(con.getTransactionModel(),"implicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
# informix DDL is transactional in logged mode; commit so the table
# is visible to the second connection (commit implicitly starts a
# new tx)
assertTrue(con.commit())
secondcon=SQLRConnection.new("sqlrelay",9010,"/tmp/informix.socket",
					"testuser","testpassword",0,1)
secondcur=SQLRCursor.new(secondcon)
setSecondConnection(secondcon)
setSecondCursor(secondcur)
# Informix has no MVCC; under default committed-read isolation,
# secondcur's catalog/data read errors with "Cannot get system
# information for table" while cur holds row locks from the
# in-flight tx.  Use dirty-read on secondcur so it sees the
# uncommitted writes — the test then verifies dirty-read
# semantics instead of MVCC visibility.
assertTrue(secondcur.sendQuery("set isolation to dirty read"))
# session is in a transaction; insert is visible via dirty read
assertTrue(con.getInTransaction())
assertFalse(con.getAutoCommit())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
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
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit
print "TRANSACTION BEHAVIOR - explicit: \n"
assertTrue(con.setTransactionModel("explicit"))
assertEqual(con.getTransactionModel(),"explicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
# see note above re: informix dirty-read workaround
assertTrue(secondcur.sendQuery("set isolation to dirty read"))
# begin starts a new transaction; insert is visible via dirty read
assertTrue(con.begin())
assertTrue(con.getInTransaction())
assertTrue(cur.sendQuery("insert into testtable values (1)"))
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
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
assertEqual(secondcur.getField(0,0),"1")
# begin, insert, rollback discards
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (2)"))
assertTrue(con.rollback())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"1")
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
assertEqual(secondcur.getField(0,0),"2")
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
# dirty-read on secondcur sees the in-flight insert (count=5)
assertTrue(con.autoCommitOn())
assertTrue(con.getAutoCommit())
assertTrue(con.begin())
assertTrue(cur.sendQuery("insert into testtable values (7)"))
assertTrue(con.autoCommitOff())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"5")
assertTrue(con.commit())
assertFalse(con.getAutoCommit())
assertTrue(con.getInTransaction())
assertTrue(secondcur.sendQuery("select count(*) from testtable"))
assertEqual(secondcur.getField(0,0),"5")
secondcur.closeResultSet()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit-error
print "TRANSACTION BEHAVIOR - explicit-error: \n"
assertTrue(con.setTransactionModel("explicit-error"))
assertEqual(con.getTransactionModel(),"explicit-error")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
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
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - none
print "TRANSACTION BEHAVIOR - none: \n"
assertTrue(con.setTransactionModel("none"))
assertEqual(con.getTransactionModel(),"none")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
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
assertEqual(con.getTransactionModel(),"implicit")
assertFalse(con.getAutoCommit())
print "\n"


# individual substitutions
print "INDIVIDUAL SUBSTITUTIONS: \n"
cur.prepareQuery(
	"select "+
	"	$(var1), "+
	"	'$(var2)', "+
	"	'$(var3)' "+
	"from "+
	"	sysmaster:sysdual ")
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
cur.prepareQuery(
	"select "+
	"	'$(var1)', "+
	"	'$(var2)', "+
	"	'$(var3)' "+
	"from "+
	"	sysmaster:sysdual ")
cur.substitutions(["var1","var2","var3"],
			["hi","hello","bye"])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"hi")
assertEqual(cur.getField(0,1),"hello")
assertEqual(cur.getField(0,2),"bye")
print "\n"
cur.prepareQuery(
	"select "+
	"	$(var1), "+
	"	$(var2), "+
	"	$(var3) "+
	"from "+
	"	sysmaster:sysdual ")
cur.substitutions(["var1","var2","var3"],
			[1,2,3])
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"2")
assertEqual(cur.getField(0,2),"3")
print "\n"
cur.prepareQuery(
	"select "+
	"	$(var1), "+
	"	$(var2), "+
	"	$(var3) "+
	"from "+
	"	sysmaster:sysdual ")
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
assertTrue(cur.sendQuery(
	"select NULL::int,1,NULL::int from sysmaster:sysdual"))
assertEqual(cur.getField(0,0),nil)
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery(
	"select NULL::int,1,NULL::int from sysmaster:sysdual"))
assertEqual(cur.getField(0,0),"")
assertEqual(cur.getField(0,1),"1")
assertEqual(cur.getField(0,2),"")
print "\n"


# output bind by position
print "OUTPUT BIND BY POSITION: \n"
cur.sendQuery("drop procedure testproc")
cur.getNullsAsNils()
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	out out1 int, "+
	"	out out2 varchar(20), "+
	"	out out3 float, "+
	"	out out4 varchar(20)) "+
	"let out1 = 1; "+
	"	let out2 = 'hello'; "+
	"	let out3 = 2.5; "+
	"	let out4 = null; "+
	"end procedure;"))
assertTrue(con.commit())
cur.prepareQuery("{call testproc(?,?,?,?)}")
assertEqual(cur.countBindVariables(),4)
cur.defineOutputBindInteger("1")
cur.defineOutputBindString("2",20)
cur.defineOutputBindDouble("3")
cur.defineOutputBindString("4",20)
assertTrue(cur.executeQuery())
numvar=cur.getOutputBindInteger("1")
stringvar=cur.getOutputBindString("2")
floatvar=cur.getOutputBindDouble("3")
nullvar=cur.getOutputBindString("4")
assertEqual(numvar,1)
assertEqual(stringvar,"hello")
assertEqual(floatvar,2.5)
assertEqual(nullvar,nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# output bind by name
# informix doesn't support bind by name


# output bind by name with validation
# informix doesn't support bind by name


# lob output bind
print "LOB OUTPUT BIND: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)"))
assertTrue(con.commit())
cur.prepareQuery("insert into testtable values (?,?)")
cur.inputBindClob("1","hello",5)
cur.inputBindBlob("2","hello",5)
assertTrue(cur.executeQuery())
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	out out1 clob, "+
	"	out out2 blob) "+
	"select testclob, testblob "+
	"	into out1, out2 "+
	"	from testtable; "+
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
assertEqualLen(clobvar,"hello",5)
assertEqual(clobvarlength,5)
assertEqualLen(blobvar,"hello",5)
assertEqual(blobvarlength,5)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# long output bind
print "LONG OUTPUT BIND: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 clob, "+
	"	out out1 clob) "+
	"let out1 = in1; "+
	"	end procedure;"))
assertTrue(con.commit())
largebuffer="C" * (20*1024)
cur.prepareQuery("{call testproc(?,?)}")
cur.inputBindClob("1",largebuffer,20*1024)
cur.defineOutputBindClob("2")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindLength("2"),20*1024)
assertEqual(cur.getOutputBindClob("2"),largebuffer)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# negative input bind
print "NEGATIVE INPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery("create table testtable (testval int)")
assertTrue(con.commit())
cur.prepareQuery("insert into testtable values (?)")
cur.inputBind("1",-1)
assertTrue(cur.executeQuery())
cur.sendQuery("select testval from testtable")
assertEqual(cur.getField(0,"testval"),"-1")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# bind validation
# informix doesn't support bind by name

# rebinding
print "REBINDING: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 int, "+
	"	out out1 int) "+
	"let out1 = in1; "+
	"end procedure;"))
assertTrue(con.commit())
cur.prepareQuery("{call testproc(?,?)}")
cur.inputBind("1",1)
cur.defineOutputBindInteger("2")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("2"),1)
cur.inputBind("1",2)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("2"),2)
cur.inputBind("1",3)
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("2"),3)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# reexecute
print "REEXECUTE: \n"
cur.prepareQuery("select 1 from sysmaster:sysdual")
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
assertTrue(cur.executeQuery())
assertEqual(cur.rowCount(),1)
assertEqual(cur.getField(0,0),"1")
print "\n"
cur.prepareQuery("select ?::int from sysmaster:sysdual")
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
print "STORED PROCEDURE RETURNING NO VALUE: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 int, "+
	"	in2 float, "+
	"	in3 varchar(20)) "+
	"end procedure;"))
assertTrue(con.commit())
cur.prepareQuery("{call testproc(?,?,?)}")
cur.inputBind("1",1)
cur.inputBind("2",2.5,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# stored procedure returning single value
print "STORED PROCEDURE RETURNING SINGLE VALUE: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 int, "+
	"	in2 float, "+
	"	in3 varchar(20), "+
	"	out out1 int) "+
	"let out1 = in1; "+
	"end procedure;"))
assertTrue(con.commit())
cur.prepareQuery("{call testproc(?,?,?,?)}")
cur.inputBind("1",1)
cur.inputBind("2",2.5,2,1)
cur.inputBind("3","hello")
cur.defineOutputBindInteger("4")
assertTrue(cur.executeQuery())
assertEqual(cur.getOutputBindInteger("4"),1)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# stored procedure returning multiple values
print "STORED PROCEDURE RETURNING MULTIPLE VALUES: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 int, "+
	"	in2 float, "+
	"	in3 varchar(20), "+
	"	out out1 int, "+
	"	out out2 float, "+
	"	out out3 varchar(20)) "+
	"let out1 = in1; "+
	"	let out2 = in2; "+
	"	let out3 = in3; "+
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
assertEqual(cur.getOutputBindInteger("4"),1)
assertEqual(cur.getOutputBindDouble("5"),2.5)
assertEqual(cur.getOutputBindString("6"),"hello")
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# stored procedure returning result set
print "STORED PROCEDURE RETURNING RESULT SET: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc() "+
	"returning boolean, smallint, varchar(40); "+
	"	define out1 boolean; "+
	"	define out2 smallint; "+
	"	define out3 varchar(40); "+
	"	foreach "+
	"		select "+
	"			testboolean, "+
	"			testsmallint, "+
	"			testvarchar "+
	"		into out1,out2,out3 "+
	"		from ( "+
	"			select "+
	"				't' as testboolean, "+
	"				1 as testsmallint, "+
	"				'1' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"			union "+
	"			select "+
	"				't' as testboolean, "+
	"				2 as testsmallint, "+
	"				'2' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"			union "+
	"			select "+
	"				't' as testboolean, "+
	"				3 as testsmallint, "+
	"				'3' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"			union "+
	"			select "+
	"				't' as testboolean, "+
	"				4 as testsmallint, "+
	"				'4' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"			union "+
	"			select "+
	"				't' as testboolean, "+
	"				5 as testsmallint, "+
	"				'5' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"			union "+
	"			select "+
	"				't' as testboolean, "+
	"				6 as testsmallint, "+
	"				'6' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"			union "+
	"			select "+
	"				't' as testboolean, "+
	"				7 as testsmallint, "+
	"				'7' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"			union "+
	"			select "+
	"				't' as testboolean, "+
	"				8 as testsmallint, "+
	"				'8' as testvarchar "+
	"			from "+
	"				sysmaster:sysdual "+
	"		) "+
	"	return out1,out2,out3 "+
	"	with resume; "+
	"	end foreach; "+
	"	end procedure;"))
assertTrue(con.commit())
assertTrue(cur.sendQuery("{call testproc()}"))
assertEqual(cur.rowCount(),8)
assertTrue(cur.sendQuery("drop procedure testproc"))
assertTrue(con.commit())
print "\n"


# null and empty lobs
print "NULL AND EMPTY LOBS: \n"
cur.sendQuery("drop table testtable")
cur.getNullsAsNils()
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob1 clob, "+
	"	testclob2 clob, "+
	"	testblob1 blob, "+
	"	testblob2 blob)"))
assertTrue(con.commit())
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
# informix returns a single \0 for an empty lob; the C++ test passes
# via strcmp (which stops at \0) so truncate at first \0 here.
assertEqual(cur.getField(0,0).to_s.split("\0").first.to_s,"")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2).to_s.split("\0").first.to_s,"")
assertEqual(cur.getField(0,3),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# long lobs
print "LONG LOBS: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	testtext text, "+
	"	testbyte byte)")
assertTrue(con.commit())
cur.prepareQuery("insert into testtable values (?,?)")
largebuffer="C" * (20*1024)
cur.inputBindClob("1",largebuffer,20*1024)
cur.inputBindBlob("2",largebuffer,20*1024)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getFieldLength(0,"testtext"),20*1024)
assertEqual(cur.getField(0,"testtext"),largebuffer)
assertEqual(cur.getFieldLength(0,"testbyte"),20*1024)
assertEqualLen(cur.getField(0,"testbyte"),largebuffer,
					20*1024)
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# temporary tables
print "TEMPORARY TABLES: \n"
cur.sendQuery("drop table temptable")
cur.sendQuery(
	"create temp table temptable (col1 int)")
assertTrue(cur.sendQuery("insert into temptable values (1)"))
assertTrue(cur.sendQuery("select count(*) from temptable"))
assertEqual(cur.getField(0,0),"1")
con.endSession()
print "\n"
assertFalse(cur.sendQuery("select count(*) from temptable"))
print "\n"


# encoded binary data
# informix doesn't support encoded binary data


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
		"	(col1 serial primary key, "+
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
# informix requires that a table exist that is
# owned by a user for the user to be reported
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(con.commit())
assertTrue(cur.getSchemaList(nil))
assertEqual(cur.getColumnName(0),"Database")
assertInResultSet(cur,"Database","testuser")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
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
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	col1 integer, "+
	"	col2 integer)"))
assertTrue(con.commit())
assertTrue(cur.getTableList(nil))
assertInResultSet(cur,"Tables_in_xxx","testtable1")
assertInResultSet(cur,"Tables_in_xxx","testtable2")
assertInResultSet(cur,"Tables_in_xxx","testtable3")
assertInResultSet(cur,"Tables_in_xxx","testtable4")
assertTrue(cur.sendQuery("drop table testtable1"))
assertTrue(cur.sendQuery("drop table testtable2"))
assertTrue(cur.sendQuery("drop table testtable3"))
assertTrue(cur.sendQuery("drop table testtable4"))
assertTrue(con.commit())
print "\n"


# type info list
print "TYPE INFO LIST: \n"
assertTrue(cur.getTypeInfoList("integer"))
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
assertEqual(cur.getField(0,"type_name"),"INTEGER")
assertEqual(cur.getField(0,"data_type"),"4")
assertEqual(cur.getField(0,"precision"),"10")
assertEqual(cur.getField(0,"local_type_name"),"INTEGER")
assertTrue(cur.getTypeInfoList("char"))
assertEqual(cur.getField(0,"type_name"),"CHAR")
assertEqual(cur.getField(0,"data_type"),"1")
assertEqual(cur.getField(0,"precision"),"32767")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"255")
assertEqual(cur.getField(0,"local_type_name"),"VARCHAR")
assertTrue(cur.getTypeInfoList("date"))
assertEqual(cur.getField(0,"type_name"),"DATE")
assertEqual(cur.getField(0,"data_type"),"91")
assertEqual(cur.getField(0,"precision"),"10")
assertEqual(cur.getField(0,"local_type_name"),"DATE")
print "\n"


# column list
print "COLUMN LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testboolean boolean, "+
	"	testsmallint smallint, "+
	"	testint integer, "+
	"	testbigint bigint, "+
	"	testint8 int8, "+
	"	testdecimal decimal(10,2), "+
	"	testmoney money, "+
	"	testsmallfloat smallfloat, "+
	"	testfloat float, "+
	"	testchar char(40), "+
	"	testnchar nchar(40), "+
	"	testvarchar varchar(40), "+
	"	testnvarchar nvarchar(40), "+
	"	testlvarchar lvarchar(40), "+
	"	testdate date, "+
	"	testdatetime datetime year to second, "+
	"	testtext text, "+
	"	testbyte byte)"))
assertTrue(con.commit())
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
assertEqual(cur.getField(0,"column_name"),"testboolean")
assertEqual(cur.getField(1,"column_name"),"testsmallint")
assertEqual(cur.getField(2,"column_name"),"testint")
assertEqual(cur.getField(3,"column_name"),"testbigint")
assertEqual(cur.getField(4,"column_name"),"testint8")
assertEqual(cur.getField(5,"column_name"),"testdecimal")
assertEqual(cur.getField(6,"column_name"),"testmoney")
assertEqual(cur.getField(7,"column_name"),"testsmallfloat")
assertEqual(cur.getField(8,"column_name"),"testfloat")
assertEqual(cur.getField(9,"column_name"),"testchar")
assertEqual(cur.getField(10,"column_name"),"testnchar")
assertEqual(cur.getField(11,"column_name"),"testvarchar")
assertEqual(cur.getField(12,"column_name"),"testnvarchar")
assertEqual(cur.getField(13,"column_name"),"testlvarchar")
assertEqual(cur.getField(14,"column_name"),"testdate")
assertEqual(cur.getField(15,"column_name"),"testdatetime")
assertEqual(cur.getField(16,"column_name"),"testtext")
assertEqual(cur.getField(17,"column_name"),"testbyte")
assertEqual(cur.getField(0,"data_type"),"BOOLEAN")
assertEqual(cur.getField(1,"data_type"),"SMALLINT")
assertEqual(cur.getField(2,"data_type"),"INTEGER")
assertEqual(cur.getField(3,"data_type"),"BIGINT")
assertEqual(cur.getField(4,"data_type"),"INT8")
assertEqual(cur.getField(5,"data_type"),"DECIMAL")
assertEqual(cur.getField(6,"data_type"),"MONEY")
assertEqual(cur.getField(7,"data_type"),"SMALLFLOAT")
assertEqual(cur.getField(8,"data_type"),"FLOAT")
assertEqual(cur.getField(9,"data_type"),"CHAR")
assertEqual(cur.getField(10,"data_type"),"NCHAR")
assertEqual(cur.getField(11,"data_type"),"VARCHAR")
assertEqual(cur.getField(12,"data_type"),"NVARCHAR")
assertEqual(cur.getField(13,"data_type"),"LVARCHAR")
assertEqual(cur.getField(14,"data_type"),"DATE")
assertEqual(cur.getField(15,"data_type"),"DATETIME")
assertEqual(cur.getField(16,"data_type"),"TEXT")
assertEqual(cur.getField(17,"data_type"),"BYTE")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# column list - auto_increment, primary key
print "COLUMN LIST - auto_increment, primary key: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 serial primary key, "+
	"	col2 int)"))
assertTrue(con.commit())
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getField(0,"extra"),"auto_increment")
assertEqual(cur.getField(0,"column_key"),"PRI")
assertEqual(cur.getField(1,"extra"),"")
assertEqual(cur.getField(1,"column_key"),"")
print "\n"
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"))
assertTrue(cur.getColumnList("testtable",nil))
assertEqual(cur.getField(0,"extra"),"")
assertEqual(cur.getField(0,"column_key"),"PRI")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# primary keys list
print "PRIMARY KEYS LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 integer primary key, "+
	"	col2 integer)"))
assertTrue(con.commit())
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
assertEqual(cur.getField(0,"table"),"testtable")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"col1")
assertTrue(cur.getField(0,"key_name")!=nil && cur.getField(0,"key_name")!="")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# key and index list
print "KEY AND INDEX LIST: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 integer primary key, "+
	"	col2 integer)"))
assertTrue(con.commit())
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
assertEqual(cur.getField(0,"table"),"testtable")
assertEqual(cur.getField(0,"non_unique"),"0")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"col1")
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"3")
assertTrue(cur.getField(0,"key_name")!=nil && cur.getField(0,"key_name")!="")
assertTrue(cur.sendQuery("drop table testtable"))
assertTrue(con.commit())
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
cur.sendQuery("drop procedure testproc1")
cur.sendQuery("drop procedure testproc2")
cur.sendQuery("drop procedure testproc3")
cur.sendQuery("drop procedure testproc4")
assertTrue(cur.sendQuery(
	"create procedure testproc1("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"))
assertTrue(cur.sendQuery(
	"create procedure testproc2("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"))
assertTrue(cur.sendQuery(
	"create procedure testproc3("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"))
assertTrue(cur.sendQuery(
	"create procedure testproc4("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"))
assertTrue(con.commit())
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
assertEqual(cur.getField(0,"parameter_name"),"in1")
assertEqual(cur.getField(0,"parameter_mode"),"1")
assertEqual(cur.getField(0,"data_type"),"integer")
assertEqual(cur.getField(0,"ordinal_position"),"1")
assertEqual(cur.getField(1,"parameter_name"),"in2")
assertEqual(cur.getField(1,"parameter_mode"),"1")
assertEqual(cur.getField(1,"data_type"),"char")
assertEqual(cur.getField(1,"ordinal_position"),"2")
assertEqual(cur.getField(2,"parameter_name"),"in3")
assertEqual(cur.getField(2,"parameter_mode"),"1")
assertEqual(cur.getField(2,"data_type"),"varchar")
assertEqual(cur.getField(2,"ordinal_position"),"3")
assertEqual(cur.getField(3,"parameter_name"),"in4")
assertEqual(cur.getField(3,"parameter_mode"),"1")
assertEqual(cur.getField(3,"data_type"),"date")
assertEqual(cur.getField(3,"ordinal_position"),"4")
assertTrue(cur.sendQuery("drop procedure testproc1"))
assertTrue(cur.sendQuery("drop procedure testproc2"))
assertTrue(cur.sendQuery("drop procedure testproc3"))
assertTrue(cur.sendQuery("drop procedure testproc4"))
assertTrue(con.commit())
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "))
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
