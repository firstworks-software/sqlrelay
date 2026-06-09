#! /usr/bin/env ruby

# Copyright (c) David Muse
# See the file COPYING for more information.



require 'rbconfig'
require 'socket'
require 'sqlrelay'
require './asserts'




# hostname
hostname=Socket.gethostname.split(".")[0]


# instantiation
con=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1)
cur=SQLRCursor.new(con)
setConnection(con)
setCursor(cur)


# identify
print "IDENTIFY: \n"
assertEqual(con.identify(),"mysql")
print "\n"


# db version
print "DB VERSION: \n"
dbversion=con.dbVersion()
majorversion=dbversion[0].to_i
print "\n"


# ping
print "PING: \n"
assertTrue(con.ping())
print "\n"


# transaction state
print "TRANSACTION STATE: \n"
assertEqual(con.getDefaultTransactionModel(),"explicit-deferred")
assertEqual(con.getTransactionModel(),"explicit-deferred")
assertFalse(con.getInTransaction())
assertTrue(con.getAutoCommit())
print "\n"


# bind format
print "BIND FORMAT: \n"
if majorversion>3
	assertEqual(con.bindFormat(),"?")
else
	assertEqual(con.bindFormat(),":*")
end
print "\n"


# nextval format
print "NEXTVAL FORMAT: \n"
assertEqual(con.nextvalFormat(),"")
print "\n"


# isolation levels
print "ISOLATION LEVELS: \n"
isolationlevels=["REPEATABLE-READ","READ-UNCOMMITTED",
		"READ-COMMITTED","SERIALIZABLE"]
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
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testtinyint tinyint, "+
	"	testsmallint smallint, "+
	"	testmediumint mediumint, "+
	"	testint int, "+
	"	testbigint bigint, "+
	"	testfloat float, "+
	"	testreal real, "+
	"	testdecimal decimal(2,1), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testdatetime datetime, "+
	"	testyear year, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testtext text, "+
	"	testtinytext tinytext, "+
	"	testmediumtext mediumtext, "+
	"	testlongtext longtext, "+
	"	testblob blob, "+
	"	testtinyblob tinyblob, "+
	"	testmediumblob mediumblob, "+
	"	testlongblob longblob, "+
	"	testtimestamp timestamp)"))
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
	"	1, "+
	"	1, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	'2001-01-01', "+
	"	'01:00:00', "+
	"	'2001-01-01 01:00:00', "+
	"	'2001', "+
	"	'char1', "+
	"	'varchar1', "+
	"	'text1', "+
	"	'tinytext1', "+
	"	'mediumtext1', "+
	"	'longtext1', "+
	"	'blob1', "+
	"	'tinyblob1', "+
	"	'mediumblob1', "+
	"	'longblob1', "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	2, "+
	"	2, "+
	"	2, "+
	"	2, "+
	"	2, "+
	"	2.5, "+
	"	2.5, "+
	"	2.5, "+
	"	'2002-01-01', "+
	"	'02:00:00', "+
	"	'2002-01-01 02:00:00', "+
	"	'2002', "+
	"	'char2', "+
	"	'varchar2', "+
	"	'text2', "+
	"	'tinytext2', "+
	"	'mediumtext2', "+
	"	'longtext2', "+
	"	'blob2', "+
	"	'tinyblob2', "+
	"	'mediumblob2', "+
	"	'longblob2', "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	3, "+
	"	3, "+
	"	3, "+
	"	3, "+
	"	3, "+
	"	3.5, "+
	"	3.5, "+
	"	3.5, "+
	"	'2003-01-01', "+
	"	'03:00:00', "+
	"	'2003-01-01 03:00:00', "+
	"	'2003', "+
	"	'char3', "+
	"	'varchar3', "+
	"	'text3', "+
	"	'tinytext3', "+
	"	'mediumtext3', "+
	"	'longtext3', "+
	"	'blob3', "+
	"	'tinyblob3', "+
	"	'mediumblob3', "+
	"	'longblob3', "+
	"	NULL)"))
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	4, "+
	"	4, "+
	"	4, "+
	"	4, "+
	"	4, "+
	"	4.5, "+
	"	4.5, "+
	"	4.5, "+
	"	'2004-01-01', "+
	"	'04:00:00', "+
	"	'2004-01-01 04:00:00', "+
	"	'2004', "+
	"	'char4', "+
	"	'varchar4', "+
	"	'text4', "+
	"	'tinytext4', "+
	"	'mediumtext4', "+
	"	'longtext4', "+
	"	'blob4', "+
	"	'tinyblob4', "+
	"	'mediumblob4', "+
	"	'longblob4', "+
	"	NULL)"))
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
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?, "+
	"	NULL)")
assertEqual(cur.countBindVariables(),22)
cur.inputBind("1",5)
cur.inputBind("2",5)
cur.inputBind("3",5)
cur.inputBind("4",5)
cur.inputBind("5",5)
cur.inputBind("6",5.5,2,1)
cur.inputBind("7",5.5,2,1)
cur.inputBind("8",5.5,2,1)
cur.inputBind("9","2005-01-01")
cur.inputBind("10","05:00:00")
cur.inputBindDate("11",2005,1,1,5,0,0,0,"",0)
cur.inputBind("12","2005")
cur.inputBind("13","char5")
cur.inputBind("14","varchar5")
cur.inputBindClob("15","text5","text5".to_s.bytesize)
cur.inputBindClob("16","tinytext5","tinytext5".to_s.bytesize)
cur.inputBindClob("17","mediumtext5","mediumtext5".to_s.bytesize)
cur.inputBindClob("18","longtext5","longtext5".to_s.bytesize)
cur.inputBindBlob("19","blob5","blob5".to_s.bytesize)
cur.inputBindBlob("20","tinyblob5","tinyblob5".to_s.bytesize)
cur.inputBindBlob("21","mediumblob5","mediumblob5".to_s.bytesize)
cur.inputBindBlob("22","longblob5","longblob5".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",6)
cur.inputBind("2",6)
cur.inputBind("3",6)
cur.inputBind("4",6)
cur.inputBind("5",6)
cur.inputBind("6",6.5,2,1)
cur.inputBind("7",6.5,2,1)
cur.inputBind("8",6.5,2,1)
cur.inputBind("9","2006-01-01")
cur.inputBind("10","06:00:00")
cur.inputBindDate("11",2006,1,1,6,0,0,0,"",0)
cur.inputBind("12","2006")
cur.inputBind("13","char6")
cur.inputBind("14","varchar6")
cur.inputBindClob("15","text6","text6".to_s.bytesize)
cur.inputBindClob("16","tinytext6","tinytext6".to_s.bytesize)
cur.inputBindClob("17","mediumtext6","mediumtext6".to_s.bytesize)
cur.inputBindClob("18","longtext6","longtext6".to_s.bytesize)
cur.inputBindBlob("19","blob6","blob6".to_s.bytesize)
cur.inputBindBlob("20","tinyblob6","tinyblob6".to_s.bytesize)
cur.inputBindBlob("21","mediumblob6","mediumblob6".to_s.bytesize)
cur.inputBindBlob("22","longblob6","longblob6".to_s.bytesize)
assertTrue(cur.executeQuery())
cur.clearBinds()
cur.inputBind("1",7)
cur.inputBind("2",7)
cur.inputBind("3",7)
cur.inputBind("4",7)
cur.inputBind("5",7)
cur.inputBind("6",7.5,2,1)
cur.inputBind("7",7.5,2,1)
cur.inputBind("8",7.5,2,1)
cur.inputBind("9","2007-01-01")
cur.inputBind("10","07:00:00")
cur.inputBindDate("11",2007,1,1,7,0,0,0,"",0)
cur.inputBind("12","2007")
cur.inputBind("13","char7")
cur.inputBind("14","varchar7")
cur.inputBindClob("15","text7","text7".to_s.bytesize)
cur.inputBindClob("16","tinytext7","tinytext7".to_s.bytesize)
cur.inputBindClob("17","mediumtext7","mediumtext7".to_s.bytesize)
cur.inputBindClob("18","longtext7","longtext7".to_s.bytesize)
cur.inputBindBlob("19","blob7","blob7".to_s.bytesize)
cur.inputBindBlob("20","tinyblob7","tinyblob7".to_s.bytesize)
cur.inputBindBlob("21","mediumblob7","mediumblob7".to_s.bytesize)
cur.inputBindBlob("22","longblob7","longblob7".to_s.bytesize)
assertTrue(cur.executeQuery())
print "\n"


# array of input binds by position
# mysql doesn't support implicit conversion of string binds to other
# data types, so arrays of binds don't generally work.


# input bind by position with validation
print "BIND BY POSITION WITH VALIDATION: \n"
cur.clearBinds()
cur.inputBind("1",8)
cur.inputBind("2",8)
cur.inputBind("3",8)
cur.inputBind("4",8)
cur.inputBind("5",8)
cur.inputBind("6",8.5,2,1)
cur.inputBind("7",8.5,2,1)
cur.inputBind("8",8.5,2,1)
cur.inputBind("9","2008-01-01")
cur.inputBind("10","08:00:00")
cur.inputBindDate("11",2008,1,1,8,0,0,0,"",0)
cur.inputBind("12","2008")
cur.inputBind("13","char8")
cur.inputBind("14","varchar8")
cur.inputBindClob("15","text8","text8".to_s.bytesize)
cur.inputBindClob("16","tinytext8","tinytext8".to_s.bytesize)
cur.inputBindClob("17","mediumtext8","mediumtext8".to_s.bytesize)
cur.inputBindClob("18","longtext8","longtext8".to_s.bytesize)
cur.inputBindBlob("19","blob8","blob8".to_s.bytesize)
cur.inputBindBlob("20","tinyblob8","tinyblob8".to_s.bytesize)
cur.inputBindBlob("21","mediumblob8","mediumblob8".to_s.bytesize)
cur.inputBindBlob("22","longblob8","longblob8".to_s.bytesize)
cur.validateBinds()
assertTrue(cur.executeQuery())
print "\n"


# input bind by name
# mysql doesn't support bind by name


# array of input binds by name
# mysql doesn't support bind by name


# input bind by name with validation
# mysql doesn't support bind by name


# select
print "SELECT: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
print "\n"


# column count
print "COLUMN COUNT: \n"
assertEqual(cur.colCount(),23)
print "\n"


# column names
print "COLUMN NAMES: \n"
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
assertEqual(cur.getColumnName(13),"testvarchar")
assertEqual(cur.getColumnName(14),"testtext")
assertEqual(cur.getColumnName(15),"testtinytext")
assertEqual(cur.getColumnName(16),"testmediumtext")
assertEqual(cur.getColumnName(17),"testlongtext")
assertEqual(cur.getColumnName(18),"testblob")
assertEqual(cur.getColumnName(19),"testtinyblob")
assertEqual(cur.getColumnName(20),"testmediumblob")
assertEqual(cur.getColumnName(21),"testlongblob")
assertEqual(cur.getColumnName(22),"testtimestamp")
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
assertEqual(cols[13],"testvarchar")
assertEqual(cols[14],"testtext")
assertEqual(cols[15],"testtinytext")
assertEqual(cols[16],"testmediumtext")
assertEqual(cols[17],"testlongtext")
assertEqual(cols[18],"testblob")
assertEqual(cols[19],"testtinyblob")
assertEqual(cols[20],"testmediumblob")
assertEqual(cols[21],"testlongblob")
assertEqual(cols[22],"testtimestamp")
print "\n"


# column types
print "COLUMN TYPES: \n"
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
if majorversion==3
	assertEqual(cur.getColumnType(12),"VARSTRING")
else
	assertEqual(cur.getColumnType(12),"STRING")
end
assertEqual(cur.getColumnType(13),"VARSTRING")
assertEqual(cur.getColumnType(14),"BLOB")
assertEqual(cur.getColumnType(15),"TINYBLOB")
assertEqual(cur.getColumnType(16),"MEDIUMBLOB")
assertEqual(cur.getColumnType(17),"LONGBLOB")
assertEqual(cur.getColumnType(18),"BLOB")
assertEqual(cur.getColumnType(19),"TINYBLOB")
assertEqual(cur.getColumnType(20),"MEDIUMBLOB")
assertEqual(cur.getColumnType(21),"LONGBLOB")
assertEqual(cur.getColumnType(22),"TIMESTAMP")
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
if majorversion==3
	assertEqual(cur.getColumnType("testchar"),"VARSTRING")
else
	assertEqual(cur.getColumnType("testchar"),"STRING")
end
assertEqual(cur.getColumnType("testvarchar"),"VARSTRING")
assertEqual(cur.getColumnType("testtext"),"BLOB")
assertEqual(cur.getColumnType("testtinytext"),"TINYBLOB")
assertEqual(cur.getColumnType("testmediumtext"),"MEDIUMBLOB")
assertEqual(cur.getColumnType("testlongtext"),"LONGBLOB")
assertEqual(cur.getColumnType("testblob"),"BLOB")
assertEqual(cur.getColumnType("testtinyblob"),"TINYBLOB")
assertEqual(cur.getColumnType("testmediumblob"),"MEDIUMBLOB")
assertEqual(cur.getColumnType("testlongblob"),"LONGBLOB")
assertEqual(cur.getColumnType("testtimestamp"),"TIMESTAMP")
print "\n"


# column length
print "COLUMN LENGTH: \n"
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
# these can be 120/121 if the db charset is utf8
#assertEqual(cur.getColumnLength(12),40)
#assertEqual(cur.getColumnLength(13),41)
assertEqual(cur.getColumnLength(14),65535)
assertEqual(cur.getColumnLength(15),255)
assertEqual(cur.getColumnLength(16),16777215)
assertEqual(cur.getColumnLength(17),2147483647)
assertEqual(cur.getColumnLength(18),65535)
assertEqual(cur.getColumnLength(19),255)
assertEqual(cur.getColumnLength(20),16777215)
assertEqual(cur.getColumnLength(21),2147483647)
assertEqual(cur.getColumnLength(22),4)
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
# these can be 120/121 if the db charset is utf8
#assertEqual(cur.getColumnLength("testchar"),40)
#assertEqual(cur.getColumnLength("testvarchar"),41)
assertEqual(cur.getColumnLength("testtext"),65535)
assertEqual(cur.getColumnLength("testtinytext"),255)
assertEqual(cur.getColumnLength("testmediumtext"),16777215)
assertEqual(cur.getColumnLength("testlongtext"),2147483647)
assertEqual(cur.getColumnLength("testblob"),65535)
assertEqual(cur.getColumnLength("testtinyblob"),255)
assertEqual(cur.getColumnLength("testmediumblob"),16777215)
assertEqual(cur.getColumnLength("testlongblob"),2147483647)
assertEqual(cur.getColumnLength("testtimestamp"),4)
print "\n"


# longest column
print "LONGEST COLUMN: \n"
assertEqual(cur.getLongest(0),1)
assertEqual(cur.getLongest(1),1)
assertEqual(cur.getLongest(2),1)
assertEqual(cur.getLongest(3),1)
assertEqual(cur.getLongest(4),1)
assertEqual(cur.getLongest(5),3)
assertEqual(cur.getLongest(6),3)
assertEqual(cur.getLongest(7),3)
assertEqual(cur.getLongest(8),10)
assertEqual(cur.getLongest(9),8)
assertEqual(cur.getLongest(10),19)
assertEqual(cur.getLongest(11),4)
assertEqual(cur.getLongest(12),5)
assertEqual(cur.getLongest(13),8)
assertEqual(cur.getLongest(14),5)
assertEqual(cur.getLongest(15),9)
assertEqual(cur.getLongest(16),11)
assertEqual(cur.getLongest(17),9)
assertEqual(cur.getLongest(18),5)
assertEqual(cur.getLongest(19),9)
assertEqual(cur.getLongest(20),11)
assertEqual(cur.getLongest(21),9)
if majorversion==3
	assertEqual(cur.getLongest(22),14)
else
	assertEqual(cur.getLongest(22),19)
end
assertEqual(cur.getLongest("testtinyint"),1)
assertEqual(cur.getLongest("testsmallint"),1)
assertEqual(cur.getLongest("testmediumint"),1)
assertEqual(cur.getLongest("testint"),1)
assertEqual(cur.getLongest("testbigint"),1)
assertEqual(cur.getLongest("testfloat"),3)
assertEqual(cur.getLongest("testreal"),3)
assertEqual(cur.getLongest("testdecimal"),3)
assertEqual(cur.getLongest("testdate"),10)
assertEqual(cur.getLongest("testtime"),8)
assertEqual(cur.getLongest("testdatetime"),19)
assertEqual(cur.getLongest("testyear"),4)
assertEqual(cur.getLongest("testchar"),5)
assertEqual(cur.getLongest("testvarchar"),8)
assertEqual(cur.getLongest("testtext"),5)
assertEqual(cur.getLongest("testtinytext"),9)
assertEqual(cur.getLongest("testmediumtext"),11)
assertEqual(cur.getLongest("testlongtext"),9)
assertEqual(cur.getLongest("testblob"),5)
assertEqual(cur.getLongest("testtinyblob"),9)
assertEqual(cur.getLongest("testmediumblob"),11)
assertEqual(cur.getLongest("testlongblob"),9)
if majorversion==3
	assertEqual(cur.getLongest("testtimestamp"),14)
else
	assertEqual(cur.getLongest("testtimestamp"),19)
end
print "\n"


# row count
print "ROW COUNT: \n"
assertEqual(cur.rowCount(),8)
print "\n"


# total rows
print "TOTAL ROWS: \n"
# older versions of mysql know this
#assertEqual(cur.totalRows(),0)
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
assertEqual(cur.getField(0,5),"1.5")
assertEqual(cur.getField(0,6),"1.5")
assertEqual(cur.getField(0,7),"1.5")
assertEqual(cur.getField(0,8),"2001-01-01")
assertEqual(cur.getField(0,9),"01:00:00")
assertEqual(cur.getField(0,10),"2001-01-01 01:00:00")
assertEqual(cur.getField(0,11),"2001")
assertEqual(cur.getField(0,12),"char1")
assertEqual(cur.getField(0,13),"varchar1")
assertEqual(cur.getField(0,14),"text1")
assertEqual(cur.getField(0,15),"tinytext1")
assertEqual(cur.getField(0,16),"mediumtext1")
assertEqual(cur.getField(0,17),"longtext1")
assertEqual(cur.getField(0,18),"blob1")
assertEqual(cur.getField(0,19),"tinyblob1")
assertEqual(cur.getField(0,20),"mediumblob1")
assertEqual(cur.getField(0,21),"longblob1")
print "\n"
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(7,1),"8")
assertEqual(cur.getField(7,2),"8")
assertEqual(cur.getField(7,3),"8")
assertEqual(cur.getField(7,4),"8")
assertEqual(cur.getField(7,5),"8.5")
assertEqual(cur.getField(7,6),"8.5")
assertEqual(cur.getField(7,7),"8.5")
assertEqual(cur.getField(7,8),"2008-01-01")
assertEqual(cur.getField(7,9),"08:00:00")
assertEqual(cur.getField(7,10),"2008-01-01 08:00:00")
assertEqual(cur.getField(7,11),"2008")
assertEqual(cur.getField(7,12),"char8")
assertEqual(cur.getField(7,13),"varchar8")
assertEqual(cur.getField(7,14),"text8")
assertEqual(cur.getField(7,15),"tinytext8")
assertEqual(cur.getField(7,16),"mediumtext8")
assertEqual(cur.getField(7,17),"longtext8")
assertEqual(cur.getField(7,18),"blob8")
assertEqual(cur.getField(7,19),"tinyblob8")
assertEqual(cur.getField(7,20),"mediumblob8")
assertEqual(cur.getField(7,21),"longblob8")
print "\n"


# field lengths by index
print "FIELD LENGTHS BY INDEX: \n"
assertEqual(cur.getFieldLength(0,0),1)
assertEqual(cur.getFieldLength(0,1),1)
assertEqual(cur.getFieldLength(0,2),1)
assertEqual(cur.getFieldLength(0,3),1)
assertEqual(cur.getFieldLength(0,4),1)
assertEqual(cur.getFieldLength(0,5),3)
assertEqual(cur.getFieldLength(0,6),3)
assertEqual(cur.getFieldLength(0,7),3)
assertEqual(cur.getFieldLength(0,8),10)
assertEqual(cur.getFieldLength(0,9),8)
assertEqual(cur.getFieldLength(0,10),19)
assertEqual(cur.getFieldLength(0,11),4)
assertEqual(cur.getFieldLength(0,12),5)
assertEqual(cur.getFieldLength(0,13),8)
assertEqual(cur.getFieldLength(0,14),5)
assertEqual(cur.getFieldLength(0,15),9)
assertEqual(cur.getFieldLength(0,16),11)
assertEqual(cur.getFieldLength(0,17),9)
assertEqual(cur.getFieldLength(0,18),5)
assertEqual(cur.getFieldLength(0,19),9)
assertEqual(cur.getFieldLength(0,20),11)
assertEqual(cur.getFieldLength(0,21),9)
print "\n"
assertEqual(cur.getFieldLength(7,0),1)
assertEqual(cur.getFieldLength(7,1),1)
assertEqual(cur.getFieldLength(7,2),1)
assertEqual(cur.getFieldLength(7,3),1)
assertEqual(cur.getFieldLength(7,4),1)
assertEqual(cur.getFieldLength(7,5),3)
assertEqual(cur.getFieldLength(7,6),3)
assertEqual(cur.getFieldLength(7,7),3)
assertEqual(cur.getFieldLength(7,8),10)
assertEqual(cur.getFieldLength(7,9),8)
assertEqual(cur.getFieldLength(7,10),19)
assertEqual(cur.getFieldLength(7,11),4)
assertEqual(cur.getFieldLength(7,12),5)
assertEqual(cur.getFieldLength(7,13),8)
assertEqual(cur.getFieldLength(7,14),5)
assertEqual(cur.getFieldLength(7,15),9)
assertEqual(cur.getFieldLength(7,16),11)
assertEqual(cur.getFieldLength(7,17),9)
assertEqual(cur.getFieldLength(7,18),5)
assertEqual(cur.getFieldLength(7,19),9)
assertEqual(cur.getFieldLength(7,20),11)
assertEqual(cur.getFieldLength(7,21),9)
print "\n"


# fields by name
print "FIELDS BY NAME: \n"
assertEqual(cur.getField(0,"testtinyint"),"1")
assertEqual(cur.getField(0,"testsmallint"),"1")
assertEqual(cur.getField(0,"testmediumint"),"1")
assertEqual(cur.getField(0,"testint"),"1")
assertEqual(cur.getField(0,"testbigint"),"1")
assertEqual(cur.getField(0,"testfloat"),"1.5")
assertEqual(cur.getField(0,"testreal"),"1.5")
assertEqual(cur.getField(0,"testdecimal"),"1.5")
assertEqual(cur.getField(0,"testdate"),"2001-01-01")
assertEqual(cur.getField(0,"testtime"),"01:00:00")
assertEqual(cur.getField(0,"testdatetime"),"2001-01-01 01:00:00")
assertEqual(cur.getField(0,"testyear"),"2001")
assertEqual(cur.getField(0,"testchar"),"char1")
assertEqual(cur.getField(0,"testvarchar"),"varchar1")
assertEqual(cur.getField(0,"testtext"),"text1")
assertEqual(cur.getField(0,"testtinytext"),"tinytext1")
assertEqual(cur.getField(0,"testmediumtext"),"mediumtext1")
assertEqual(cur.getField(0,"testlongtext"),"longtext1")
assertEqual(cur.getField(0,"testblob"),"blob1")
assertEqual(cur.getField(0,"testlongblob"),"longblob1")
assertEqual(cur.getField(0,"testtinyblob"),"tinyblob1")
assertEqual(cur.getField(0,"testmediumblob"),"mediumblob1")
print "\n"
assertEqual(cur.getField(7,"testtinyint"),"8")
assertEqual(cur.getField(7,"testsmallint"),"8")
assertEqual(cur.getField(7,"testmediumint"),"8")
assertEqual(cur.getField(7,"testint"),"8")
assertEqual(cur.getField(7,"testbigint"),"8")
assertEqual(cur.getField(7,"testfloat"),"8.5")
assertEqual(cur.getField(7,"testreal"),"8.5")
assertEqual(cur.getField(7,"testdecimal"),"8.5")
assertEqual(cur.getField(7,"testdate"),"2008-01-01")
assertEqual(cur.getField(7,"testtime"),"08:00:00")
assertEqual(cur.getField(7,"testdatetime"),"2008-01-01 08:00:00")
assertEqual(cur.getField(7,"testyear"),"2008")
assertEqual(cur.getField(7,"testchar"),"char8")
assertEqual(cur.getField(7,"testvarchar"),"varchar8")
assertEqual(cur.getField(7,"testtext"),"text8")
assertEqual(cur.getField(7,"testtinytext"),"tinytext8")
assertEqual(cur.getField(7,"testmediumtext"),"mediumtext8")
assertEqual(cur.getField(7,"testlongtext"),"longtext8")
assertEqual(cur.getField(7,"testblob"),"blob8")
assertEqual(cur.getField(7,"testlongblob"),"longblob8")
assertEqual(cur.getField(7,"testtinyblob"),"tinyblob8")
assertEqual(cur.getField(7,"testmediumblob"),"mediumblob8")
print "\n"


# field lengths by name
print "FIELD LENGTHS BY NAME: \n"
assertEqual(cur.getFieldLength(0,"testtinyint"),1)
assertEqual(cur.getFieldLength(0,"testsmallint"),1)
assertEqual(cur.getFieldLength(0,"testmediumint"),1)
assertEqual(cur.getFieldLength(0,"testint"),1)
assertEqual(cur.getFieldLength(0,"testbigint"),1)
assertEqual(cur.getFieldLength(0,"testfloat"),3)
assertEqual(cur.getFieldLength(0,"testreal"),3)
assertEqual(cur.getFieldLength(0,"testdecimal"),3)
assertEqual(cur.getFieldLength(0,"testdate"),10)
assertEqual(cur.getFieldLength(0,"testtime"),8)
assertEqual(cur.getFieldLength(0,"testdatetime"),19)
assertEqual(cur.getFieldLength(0,"testyear"),4)
assertEqual(cur.getFieldLength(0,"testchar"),5)
assertEqual(cur.getFieldLength(0,"testvarchar"),8)
assertEqual(cur.getFieldLength(0,"testtext"),5)
assertEqual(cur.getFieldLength(0,"testtinytext"),9)
assertEqual(cur.getFieldLength(0,"testmediumtext"),11)
assertEqual(cur.getFieldLength(0,"testlongtext"),9)
assertEqual(cur.getFieldLength(0,"testblob"),5)
assertEqual(cur.getFieldLength(0,"testtinyblob"),9)
assertEqual(cur.getFieldLength(0,"testmediumblob"),11)
assertEqual(cur.getFieldLength(0,"testlongblob"),9)
print "\n"
assertEqual(cur.getFieldLength(7,"testtinyint"),1)
assertEqual(cur.getFieldLength(7,"testsmallint"),1)
assertEqual(cur.getFieldLength(7,"testmediumint"),1)
assertEqual(cur.getFieldLength(7,"testint"),1)
assertEqual(cur.getFieldLength(7,"testbigint"),1)
assertEqual(cur.getFieldLength(7,"testfloat"),3)
assertEqual(cur.getFieldLength(7,"testreal"),3)
assertEqual(cur.getFieldLength(7,"testdecimal"),3)
assertEqual(cur.getFieldLength(7,"testdate"),10)
assertEqual(cur.getFieldLength(7,"testtime"),8)
assertEqual(cur.getFieldLength(7,"testdatetime"),19)
assertEqual(cur.getFieldLength(7,"testyear"),4)
assertEqual(cur.getFieldLength(7,"testchar"),5)
assertEqual(cur.getFieldLength(7,"testvarchar"),8)
assertEqual(cur.getFieldLength(7,"testtext"),5)
assertEqual(cur.getFieldLength(7,"testtinytext"),9)
assertEqual(cur.getFieldLength(7,"testmediumtext"),11)
assertEqual(cur.getFieldLength(7,"testlongtext"),9)
assertEqual(cur.getFieldLength(7,"testblob"),5)
assertEqual(cur.getFieldLength(7,"testtinyblob"),9)
assertEqual(cur.getFieldLength(7,"testmediumblob"),11)
assertEqual(cur.getFieldLength(7,"testlongblob"),9)
print "\n"


# fields by array
print "FIELDS BY ARRAY: \n"
fields=cur.getRow(0)
assertEqual(fields[0],"1")
assertEqual(fields[1],"1")
assertEqual(fields[2],"1")
assertEqual(fields[3],"1")
assertEqual(fields[4],"1")
assertEqual(fields[5],"1.5")
assertEqual(fields[6],"1.5")
assertEqual(fields[7],"1.5")
assertEqual(fields[8],"2001-01-01")
assertEqual(fields[9],"01:00:00")
assertEqual(fields[10],"2001-01-01 01:00:00")
assertEqual(fields[11],"2001")
assertEqual(fields[12],"char1")
assertEqual(fields[13],"varchar1")
assertEqual(fields[14],"text1")
assertEqual(fields[15],"tinytext1")
assertEqual(fields[16],"mediumtext1")
assertEqual(fields[17],"longtext1")
assertEqual(fields[18],"blob1")
assertEqual(fields[19],"tinyblob1")
assertEqual(fields[20],"mediumblob1")
assertEqual(fields[21],"longblob1")
print "\n"


# field lengths by array
print "FIELD LENGTHS BY ARRAY: \n"
fieldlens=cur.getRowLengths(0)
assertEqual(fieldlens[0],1)
assertEqual(fieldlens[1],1)
assertEqual(fieldlens[2],1)
assertEqual(fieldlens[3],1)
assertEqual(fieldlens[4],1)
assertEqual(fieldlens[5],3)
assertEqual(fieldlens[6],3)
assertEqual(fieldlens[7],3)
assertEqual(fieldlens[8],10)
assertEqual(fieldlens[9],8)
assertEqual(fieldlens[10],19)
assertEqual(fieldlens[11],4)
assertEqual(fieldlens[12],5)
assertEqual(fieldlens[13],8)
assertEqual(fieldlens[14],5)
assertEqual(fieldlens[15],9)
assertEqual(fieldlens[16],11)
assertEqual(fieldlens[17],9)
assertEqual(fieldlens[18],5)
assertEqual(fieldlens[19],9)
assertEqual(fieldlens[20],11)
assertEqual(fieldlens[21],9)
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
	"	testtinyint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
assertEqual(cur.getColumnName(0),nil)
assertEqual(cur.getColumnLength(0),0)
assertEqual(cur.getColumnType(0),nil)
print "\n"
cur.getColumnInfo()
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
assertEqual(cur.getColumnName(0),"testtinyint")
assertEqual(cur.getColumnLength(0),1)
assertEqual(cur.getColumnType(0),"TINYINT")
print "\n"


# suspended session
print "SUSPENDED SESSION: \n"
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
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
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
print "\n"


# column count for cached result set
print "COLUMN COUNT FOR CACHED RESULT SET: \n"
assertEqual(cur.colCount(),23)
print "\n"


# column names for cached result set
print "COLUMN NAMES FOR CACHED RESULT SET: \n"
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
assertEqual(cur.getColumnName(13),"testvarchar")
assertEqual(cur.getColumnName(14),"testtext")
assertEqual(cur.getColumnName(15),"testtinytext")
assertEqual(cur.getColumnName(16),"testmediumtext")
assertEqual(cur.getColumnName(17),"testlongtext")
assertEqual(cur.getColumnName(18),"testblob")
assertEqual(cur.getColumnName(19),"testtinyblob")
assertEqual(cur.getColumnName(20),"testmediumblob")
assertEqual(cur.getColumnName(21),"testlongblob")
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
assertEqual(cols[13],"testvarchar")
assertEqual(cols[14],"testtext")
assertEqual(cols[15],"testtinytext")
assertEqual(cols[16],"testmediumtext")
assertEqual(cols[17],"testlongtext")
assertEqual(cols[18],"testblob")
assertEqual(cols[19],"testtinyblob")
assertEqual(cols[20],"testmediumblob")
assertEqual(cols[21],"testlongblob")
print "\n"


# cached result set with result set buffer size
print "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
cur.cacheOff()
assertTrue(cur.openCachedResultSet(filename))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# from one cache file to another
print "FROM ONE CACHE FILE TO ANOTHER: \n"
cur.cacheToFile("cachefile2")
assertTrue(cur.openCachedResultSet("cachefile1"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
print "\n"


# from one cache file to another with result set buffer size
print "FROM ONE CACHE FILE TO ANOTHER "+
	"WITH RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile2")
assertTrue(cur.openCachedResultSet("cachefile1"))
cur.cacheOff()
assertTrue(cur.openCachedResultSet("cachefile2"))
assertEqual(cur.getField(7,0),"8")
assertEqual(cur.getField(8,0),nil)
cur.setResultSetBufferSize(0)
print "\n"


# cached result set with suspend and result set buffer size
print "CACHED RESULT SET WITH SUSPEND "+
	"AND RESULT SET BUFFER SIZE: \n"
cur.setResultSetBufferSize(2)
cur.cacheToFile("cachefile1")
cur.setCacheTtl(200)
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
assertEqual(cur.getField(2,0),"3")
filename=cur.getCacheFileName()
assertEqual(filename,"cachefile1")
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
# can't do this with mysql
#cur.setResultSetBufferSize(1)
assertTrue(cur.sendQuery("select * from testtable"))
secondcur=SQLRCursor.new(con)
secondcur.setResultSetBufferSize(1)
i=0
while cur.getRow(i)
	assertTrue(secondcur.sendQuery("select * from testtable"))
	i+=1
end
secondcur.closeResultSet()
#cur.setResultSetBufferSize(0)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# reset transaction state
print "RESET TRANSACTION STATE: \n"
assertTrue(con.commit())
assertEqual(con.getTransactionModel(),"explicit-deferred")
assertTrue(con.getAutoCommit())
print "\n"


# transaction behavior - implicit
print "TRANSACTION BEHAVIOR - implicit: \n"
assertTrue(con.setTransactionModel("implicit"))
assertEqual(con.getTransactionModel(),"implicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
secondcon=SQLRConnection.new("sqlrelay",9000,"/tmp/test.socket",
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
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# transaction behavior - explicit
print "TRANSACTION BEHAVIOR - explicit: \n"
assertTrue(con.setTransactionModel("explicit"))
assertEqual(con.getTransactionModel(),"explicit")
assertTrue(cur.sendQuery("create table testtable (col1 integer)"))
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
assertEqual(con.getTransactionModel(),"explicit-deferred")
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
cur.substitutions(["var1","var2","var3"],[10.55,10.556,10.5556],[4,5,6],[2,3,4])
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
	"	testclob1 longtext, "+
	"	testclob2 longtext, "+
	"	testblob1 longblob, "+
	"	testblob2 longblob)"))
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?)")
cur.inputBindClob("1","","".to_s.bytesize)
cur.inputBindClob("2",nil,nil.to_s.bytesize)
cur.inputBindBlob("3","","".to_s.bytesize)
cur.inputBindBlob("4",nil,nil.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getField(0,0),"")
assertEqual(cur.getField(0,1),nil)
assertEqual(cur.getField(0,2),"")
assertEqual(cur.getField(0,3),nil)
cur.getNullsAsEmptyStrings()
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# long lobs
print "LONG LOBS: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery(
	"create table testtable ("+
	"	testtext longtext, "+
	"	testblob longblob)")
cur.prepareQuery("insert into testtable values (?,?)")
largebuffer="C" * 8192
cur.inputBindClob("1",largebuffer,largebuffer.to_s.bytesize)
cur.inputBindBlob("2",largebuffer,largebuffer.to_s.bytesize)
assertTrue(cur.executeQuery())
cur.sendQuery("select * from testtable")
assertEqual(cur.getFieldLength(0,"testtext"),8192)
assertEqual(cur.getField(0,"testtext"),largebuffer)
assertEqual(cur.getFieldLength(0,"testblob"),8192)
assertEqual(cur.getField(0,"testblob"),largebuffer)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# output bind by position
# mysql doesn't support output binds

# output bind by name
# mysql doesn't support bind by name


# output bind by name with validation
# mysql doesn't support bind by name


# lob output bind
# mysql doesn't support output binds


# long output bind
# mysql doesn't support output binds


# negative input bind
print "NEGATIVE INPUT BIND: \n"
cur.sendQuery("drop table testtable")
cur.sendQuery("create table testtable (testval int)")
cur.prepareQuery("insert into testtable values (?)")
cur.inputBind("1",-1)
assertTrue(cur.executeQuery())
cur.sendQuery("select testval from testtable")
assertEqual(cur.getField(0,"testval"),"-1")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# bind validation
# mysql doesn't support bind by name


# rebinding
print "REBINDING: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int) "+
	"begin "+
	"	select in1; "+
	"end"))
cur.prepareQuery("call testproc(?)")
cur.inputBind("1",1)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
cur.inputBind("1",2)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"2")
cur.inputBind("1",3)
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"3")
assertTrue(cur.sendQuery("drop procedure testproc"))
print "\n"


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
cur.prepareQuery("select ?")
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
	"	in in1 int, "+
	"	in in2 double, "+
	"	in in3 varchar(20)) "+
	"begin "+
	"end"))
cur.prepareQuery("call testproc(?,?,?)")
cur.inputBind("1",1)
cur.inputBind("2",1.5,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertTrue(cur.sendQuery("drop procedure testproc"))
print "\n"


# stored procedure returning single value
print "STORED PROCEDURE RETURNING SINGLE VALUE: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int, "+
	"	in in2 double, "+
	"	in in3 varchar(20)) "+
	"begin "+
	"	select in1; "+
	"end"))
cur.prepareQuery("call testproc(?,?,?)")
cur.inputBind("1",1)
cur.inputBind("2",1.5,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertTrue(cur.sendQuery("drop procedure testproc"))
print "\n"


# stored procedure returning multiple values
print "STORED PROCEDURE RETURNING MULTIPLE VALUES: \n"
cur.sendQuery("drop procedure testproc")
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int, "+
	"	in in2 double, "+
	"	in in3 varchar(20)) "+
	"begin "+
	"	select in1, in2, in3; "+
	"end"))
cur.prepareQuery("call testproc(?,?,?)")
cur.inputBind("1",1)
cur.inputBind("2",1.5,2,1)
cur.inputBind("3","hello")
assertTrue(cur.executeQuery())
assertEqual(cur.getField(0,0),"1")
assertEqual(cur.getField(0,1),"1.5")
assertEqual(cur.getField(0,2),"hello")
assertTrue(cur.sendQuery("drop procedure testproc"))
print "\n"


# stored procedure returning result set
print "STORED PROCEDURE RETURNING RESULT SET: \n"
cur.sendQuery("drop procedure testselectproc")
assertTrue(cur.sendQuery(
	"create procedure testselectproc() "+
	"begin "+
	"	select 1 "+
	"	union "+
	"	select 2 "+
	"	union "+
	"	select 3 "+
	"	union "+
	"	select 4 "+
	"	union "+
	"	select 5 "+
	"	union "+
	"	select 6 "+
	"	union "+
	"	select 7 "+
	"	union "+
	"	select 8; "+
	"end"))
assertTrue(cur.sendQuery("call testselectproc()"))
assertEqual(cur.rowCount(),8)
assertTrue(cur.sendQuery("drop procedure testselectproc"))
print "\n"


# temporary tables
print "TEMPORARY TABLES: \n"
cur.sendQuery("drop table temptable")
cur.sendQuery("create temporary table temptable (col1 int)")
assertTrue(cur.sendQuery("insert into temptable values (1)"))
assertTrue(cur.sendQuery("select count(*) from temptable"))
assertEqual(cur.getField(0,0),"1")
con.endSession()
print "\n"
assertFalse(cur.sendQuery("select count(*) from temptable"))
print "\n"

if majorversion>3

	# stored procedure returning no value
	print "STORED PROCEDURE RETURNING NO VALUE: \n"
	cur.sendQuery("drop procedure if exists testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("+
		"	in in1 int, "+
		"	in in2 float, "+
		"	in in3 char(20)) "+
		"begin "+
		"	select in1, in2, in3; "+
		"end;"))
	cur.prepareQuery("call testproc(?,?,?)")
	cur.inputBind("1",1)
	cur.inputBind("2",1.5,4,2)
	cur.inputBind("3","hello")
	assertTrue(cur.executeQuery())
	assertEqual(cur.getField(0,0),"1")
	assertEqual(cur.getField(0,1),"1.5")
	assertEqual(cur.getField(0,2),"hello")
	cur.sendQuery("drop procedure testproc")
	print "\n"


	# stored procedure returning one value
	print "FUNCTIONS: \n"
	cur.sendQuery("drop function if exists testfunc")
	assertTrue(cur.sendQuery(
		"create function testfunc(in1 int, in2 "+
		"	int) returns int return in1+in2;"))
	cur.prepareQuery("select testfunc(?,?)")
	cur.inputBind("1",10)
	cur.inputBind("2",20)
	assertTrue(cur.executeQuery())
	assertEqual(cur.getField(0,0),"30")
	cur.sendQuery("drop function if exists testfunc")
	print "\n"


	# stored procedure returning multiple values
	print "STORED PROCEDURE RETURNING MULTIPLE VALUES: \n"
	cur.sendQuery("drop procedure if exists testproc")
	assertTrue(cur.sendQuery(
		"create procedure testproc("+
		"	out out1 int, "+
		"	out out2 float, "+
		"	out out3 char(20)) "+
		"begin "+
		"	select 1, 2.5, 'hello' "+
		"		into out1, out2, out3; "+
		"end;"))
	assertTrue(cur.sendQuery("set @out1=0, @out2=0.0, @out3=''"))
	assertTrue(cur.sendQuery("call testproc(@out1,@out2,@out3)"))
	assertTrue(cur.sendQuery("select @out1, @out2, @out3"))
	assertEqual(cur.getField(0,0),"1")
	assertEqual(cur.getFieldAsDouble(0,1),2.5)
	assertEqual(cur.getField(0,2),"hello")
	cur.sendQuery("drop procedure testproc")
	print "\n"


	# stored procedure returning result set
	print "STORED PROCEDURE RETURNING RESULT SET: \n"
	cur.sendQuery("drop procedure if exists testselectproc")
	assertTrue(cur.sendQuery(
		"create procedure testselectproc() "+
		"begin "+
		"	select 1 "+
		"	union "+
		"	select 2 "+
		"	union "+
		"	select 3 "+
		"	union "+
		"	select 4 "+
		"	union "+
		"	select 5 "+
		"	union "+
		"	select 6 "+
		"	union "+
		"	select 7 "+
		"	union "+
		"	select 8; "+
		"end"))
	assertTrue(cur.sendQuery("call testselectproc()"))
	assertEqual(cur.rowCount(),8)
	cur.sendQuery("drop procedure testselectproc")
	print "\n"
end


if majorversion>3

	# encoded binary data - all chars - \-escaped
	print "ENCODED BINARY DATA - all chars - \\-escaped: \n"
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable (col1 longblob)"))
	buffer=(0..255).map { |j| j.chr }.join
	query="insert into testtable values (_binary'"
	for i in 0..255
		if buffer[i]=="'"
			query=query+"\\"
		end
		if buffer[i]=="\\"
			query=query+"\\"
		end
		query=query+buffer[i]
	end
	query=query+"')"
	assertTrue(cur.sendQueryWithLength(query,query.bytesize))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEqual(cur.getFieldLength(0,0),256)
	assertEqual(cur.getField(0,0),buffer)
	assertTrue(cur.sendQuery("drop table testtable"))
	print "\n"


	# encoded binary data - (null)"" - unescaped
	print "ENCODED BINARY DATA - (null)\"\" - unescaped: \n"
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable (col1 longblob)"))
	assertTrue(cur.sendQueryWithLength(
		"insert into testtable values (_binary'\0\"\"')",
		43))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEqual(cur.getFieldLength(0,0),3)
	assertEqual(cur.getField(0,0),"\0\"\"")
	assertTrue(cur.sendQuery("drop table testtable"))
	print "\n"


	# encoded binary data - (null)"" - \-escaped
	print "ENCODED BINARY DATA - \\(null)\\\"\\\" - \\-escaped: \n"
	cur.sendQuery("drop table testtable")
	assertTrue(cur.sendQuery(
		"create table testtable (col1 longblob)"))
	assertTrue(cur.sendQueryWithLength(
		"insert into testtable values (_binary'\\\0\\\"\\\"')",
		46))
	assertTrue(cur.sendQuery("select col1 from testtable"))
	assertEqual(cur.getFieldLength(0,0),3)
	assertEqual(cur.getField(0,0),"\0\"\"")
	assertTrue(cur.sendQuery("drop table testtable"))
	print "\n"
end


# quotes - '' - ''-escaped
print "QUOTES - '' - ''-escaped: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('''''')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"''")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes - '' - '',\-escaped
print "QUOTES - '' - '',\\-escaped: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('''\\'')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"''")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes - '' - \,''-escaped
print "QUOTES - '' - \\,''-escaped: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('\\'''')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"''")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes - \\' - \-escaped
print "QUOTES - \\\\' - \\-escaped: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('\\\\\\'')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"\\\'")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes - "" - unescaped
print "QUOTES - \"\" - unescaped: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"))
assertTrue(cur.sendQuery("insert into testtable values ('\"\"')"))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),2)
assertEqual(cur.getField(0,0),"\"\"")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# quotes - random - '',\-escaped
print "QUOTES - random - '',\\-escaped: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery("create table testtable "+
				"(col1 varchar(512))"))
ch=["'","\"","\\","\0"]
buffer=""
for i in 0..255
	buffer=buffer+ch[rand(4)]
end
query="insert into testtable values ('"
for i in 0..255
	if buffer[i]=="'"
		# randomly escape with \ or ''
		if rand(2)==1
			query=query+"'"
		else
			query=query+"\\"
		end
	end
	if buffer[i]=="\""
		# randomly escape with \ or don't escape
		if rand(2)==1
			query=query+"\\"
		end
	end
	if buffer[i]=="\\"
		# escape with backslash
		query=query+"\\"
	end
	query=query+buffer[i]
end
query=query+"')"
assertTrue(cur.sendQueryWithLength(query,query.bytesize))
assertTrue(cur.sendQuery("select col1 from testtable"))
assertEqual(cur.getFieldLength(0,0),256)
assertEqual(cur.getField(0,0),buffer)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# last insert id
print "LAST INSERT ID: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
		"create table testtable "+
		"	(col1 int primary key auto_increment, "+
		"	col2 int)"))
assertTrue(cur.sendQuery("insert into testtable values (null,1)"))
assertEqual(con.getLastInsertId(),1)
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# database is schema
print "DATABASE IS SCHEMA: \n"
assertTrue(con.getDatabaseIsSchema())
print "\n"


# catalog list
print "CATALOG LIST: \n"
assertTrue(cur.getCatalogList(nil))
assertEqual(cur.getColumnName(0),"Database")
assertInResultSet(cur,"Database","def")
print "\n"


# schema list
print "SCHEMA LIST: \n"
assertTrue(cur.getSchemaList(nil))
assertEqual(cur.getColumnName(0),"Database")
assertInResultSet(cur,"Database",hostname)
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
assertEqual(cur.getField(0,"precision"),"255")
assertEqual(cur.getField(0,"local_type_name"),"CHAR")
assertTrue(cur.getTypeInfoList("varchar"))
assertEqual(cur.getField(0,"type_name"),"VARCHAR")
assertEqual(cur.getField(0,"data_type"),"12")
assertEqual(cur.getField(0,"precision"),"65535")
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
	"	testtinyint tinyint, "+
	"	testsmallint smallint, "+
	"	testmediumint mediumint, "+
	"	testint int, "+
	"	testbigint bigint, "+
	"	testfloat float, "+
	"	testreal real, "+
	"	testdecimal decimal(2,1), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testdatetime datetime, "+
	"	testyear year, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testtext text, "+
	"	testtinytext tinytext, "+
	"	testmediumtext mediumtext, "+
	"	testlongtext longtext, "+
	"	testblob blob, "+
	"	testtinyblob tinyblob, "+
	"	testmediumblob mediumblob, "+
	"	testlongblob longblob, "+
	"	testtimestamp timestamp)"))
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
assertEqual(cur.getField(0,"column_name"),"testtinyint")
assertEqual(cur.getField(1,"column_name"),"testsmallint")
assertEqual(cur.getField(2,"column_name"),"testmediumint")
assertEqual(cur.getField(3,"column_name"),"testint")
assertEqual(cur.getField(4,"column_name"),"testbigint")
assertEqual(cur.getField(5,"column_name"),"testfloat")
assertEqual(cur.getField(6,"column_name"),"testreal")
assertEqual(cur.getField(7,"column_name"),"testdecimal")
assertEqual(cur.getField(8,"column_name"),"testdate")
assertEqual(cur.getField(9,"column_name"),"testtime")
assertEqual(cur.getField(10,"column_name"),"testdatetime")
assertEqual(cur.getField(11,"column_name"),"testyear")
assertEqual(cur.getField(12,"column_name"),"testchar")
assertEqual(cur.getField(13,"column_name"),"testvarchar")
assertEqual(cur.getField(14,"column_name"),"testtext")
assertEqual(cur.getField(15,"column_name"),"testtinytext")
assertEqual(cur.getField(16,"column_name"),"testmediumtext")
assertEqual(cur.getField(17,"column_name"),"testlongtext")
assertEqual(cur.getField(18,"column_name"),"testblob")
assertEqual(cur.getField(19,"column_name"),"testtinyblob")
assertEqual(cur.getField(20,"column_name"),"testmediumblob")
assertEqual(cur.getField(21,"column_name"),"testlongblob")
assertEqual(cur.getField(22,"column_name"),"testtimestamp")
assertEqual(cur.getField(0,"data_type"),"TINYINT")
assertEqual(cur.getField(1,"data_type"),"SMALLINT")
assertEqual(cur.getField(2,"data_type"),"MEDIUMINT")
assertEqual(cur.getField(3,"data_type"),"INT")
assertEqual(cur.getField(4,"data_type"),"BIGINT")
assertEqual(cur.getField(5,"data_type"),"FLOAT")
assertEqual(cur.getField(6,"data_type"),"DOUBLE") # not "REAL"
assertEqual(cur.getField(7,"data_type"),"DECIMAL")
assertEqual(cur.getField(8,"data_type"),"DATE")
assertEqual(cur.getField(9,"data_type"),"TIME")
assertEqual(cur.getField(10,"data_type"),"DATETIME")
assertEqual(cur.getField(11,"data_type"),"YEAR")
assertEqual(cur.getField(12,"data_type"),"CHAR")
assertEqual(cur.getField(13,"data_type"),"VARCHAR")
assertEqual(cur.getField(14,"data_type"),"TEXT")
assertEqual(cur.getField(15,"data_type"),"TINYTEXT")
assertEqual(cur.getField(16,"data_type"),"MEDIUMTEXT")
assertEqual(cur.getField(17,"data_type"),"LONGTEXT")
assertEqual(cur.getField(18,"data_type"),"BLOB")
assertEqual(cur.getField(19,"data_type"),"TINYBLOB")
assertEqual(cur.getField(20,"data_type"),"MEDIUMBLOB")
assertEqual(cur.getField(21,"data_type"),"LONGBLOB")
assertEqual(cur.getField(22,"data_type"),"TIMESTAMP")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# column list - auto_increment, primary key
print "COLUMN LIST - auto_increment, primary key: \n"
cur.sendQuery("drop table testtable")
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int auto_increment primary key, "+
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
assertEqual(cur.getField(0,"table"),"testtable")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"col1")
assertEqual(cur.getField(0,"key_name"),"PRIMARY")
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
assertEqual(cur.getField(0,"table"),"testtable")
assertEqual(cur.getField(0,"non_unique"),"false")
assertEqual(cur.getField(0,"seq_in_index"),"1")
assertEqual(cur.getField(0,"column_name"),"col1")
assertEqual(cur.getField(0,"collation"),"A")
assertEqual(cur.getField(0,"index_type"),"3")
assertEqual(cur.getField(0,"key_name"),"PRIMARY")
assertTrue(cur.sendQuery("drop table testtable"))
print "\n"


# procedure list
print "PROCEDURE LIST: \n"
cur.sendQuery("drop procedure testproc1")
cur.sendQuery("drop procedure testproc2")
cur.sendQuery("drop procedure testproc3")
cur.sendQuery("drop procedure testproc4")
assertTrue(cur.sendQuery(
	"create procedure testproc1("+
	"	in in1 int, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"begin end"))
assertTrue(cur.sendQuery(
	"create procedure testproc2("+
	"	in in1 int, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"begin end"))
assertTrue(cur.sendQuery(
	"create procedure testproc3("+
	"	in in1 int, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"begin end"))
assertTrue(cur.sendQuery(
	"create procedure testproc4("+
	"	in in1 int, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) "+
	"begin end"))
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
assertEqual(cur.getField(0,"data_type"),"INT")
assertEqual(cur.getField(0,"ordinal_position"),"1")
assertEqual(cur.getField(1,"parameter_name"),"in2")
assertEqual(cur.getField(1,"parameter_mode"),"1")
assertEqual(cur.getField(1,"data_type"),"CHAR")
assertEqual(cur.getField(1,"ordinal_position"),"2")
assertEqual(cur.getField(2,"parameter_name"),"in3")
assertEqual(cur.getField(2,"parameter_mode"),"1")
assertEqual(cur.getField(2,"data_type"),"VARCHAR")
assertEqual(cur.getField(2,"ordinal_position"),"3")
assertEqual(cur.getField(3,"parameter_name"),"in4")
assertEqual(cur.getField(3,"parameter_mode"),"1")
assertEqual(cur.getField(3,"data_type"),"DATE")
assertEqual(cur.getField(3,"ordinal_position"),"4")
assertTrue(cur.sendQuery("drop procedure testproc1"))
assertTrue(cur.sendQuery("drop procedure testproc2"))
assertTrue(cur.sendQuery("drop procedure testproc3"))
assertTrue(cur.sendQuery("drop procedure testproc4"))
print "\n"


# invalid queries
print "INVALID QUERIES: \n"
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testtinyint "))
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
