// Copyright (c) David Muse
// See the file COPYING for more information.

var sqlrelay=require("sqlrelay");
var {
	setConnection, setCursor,
	setSecondConnection, setSecondCursor,
	assertEqStr, assertEqStrLen,
	assertEqInt, assertEqDbl, assertEqual,
	assertTrue, assertFalse, assertStartsWith,
	assertInResultSet,
	getStatus, reportTestStatus
}=require("./asserts.js");


var isolationlevels=["1","0","2","3"];
var subvars=["var1","var2","var3"];
var subvallongs=[1,2,3];
var subvalstrings=["hi","hello","bye"];
var subvaldoubles=[10.55,10.556,10.5556];
var precs=[4,5,6];
var scales=[2,3,4];


// hostname
var hostname=require("os").hostname().toLowerCase();
var dot=hostname.indexOf(".");
if (dot>-1) {
	hostname=hostname.substring(0,dot);
}
var dumptran="dump tran "+hostname+" with truncate_only";


// instantiation
var con=new sqlrelay.SQLRConnection("sqlrelay",9006,"/tmp/saptest.socket",
		"testuser","testpassword",0,1);
setConnection(con);
var cur=new sqlrelay.SQLRCursor(con);
setCursor(cur);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"sap");
console.log();


// ping
console.log("PING: ");
assertTrue(con.ping());
console.log();


// transaction state
console.log("TRANSACTION STATE: ");
assertEqStr(con.getDefaultTransactionModel(),"explicit-error");
assertEqStr(con.getTransactionModel(),"explicit-error");
assertFalse(con.getInTransaction());
assertTrue(con.getAutoCommit());
console.log();


// bind format
console.log("BIND FORMAT: ");
assertEqStr(con.bindFormat(),"@*");
console.log();


// nextval format
console.log("NEXTVAL FORMAT: ");
assertEqStr(con.nextvalFormat(),"%s.nextval");
console.log();


// isolation levels
console.log("ISOLATION LEVELS: ");
for (var i=0; i<isolationlevels.length; i++) {
	assertTrue(con.setIsolationLevel(isolationlevels[i]));
	assertEqStr(con.getIsolationLevel(),isolationlevels[i]);
	console.log();
}
// reset to the default isolation level
assertTrue(con.setIsolationLevel(isolationlevels[0]));
console.log();


// create testtable
console.log("CREATE TESTTABLE: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(dumptran);
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
	"	testsmalldatetime "+
	"smalldatetime, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testbit bit, "+
	"	testtext text, "+
	"	testbinary binary(20), "+
	"	testvarbinary varbinary(20), "+
	"	testunichar unichar(20), "+
	"	testunivarchar univarchar(20), "+
	"	testunitext unitext, "+
	"	testdate date, "+
	"	testtime time, "+
	"	testbigtime bigtime, "+
	"	testbigdatetime bigdatetime) "+"lock datarows"));
console.log();


// insert
console.log("INSERT: ");
assertTrue(con.begin());
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
	"	'testtext1', "+
	"	0x01, "+
	"	0x01, "+
	"	'testunichar1', "+
	"	'testunivarchar1', "+
	"	'testunitext1', "+
	"	'01-Jan-2001', "+
	"	'01:00:00', "+
	"	'01:00:00.000000', "+
	"	'01-Jan-2001 01:00:00.000000')"));
console.log();


// affected rows
console.log("AFFECTED ROWS: ");
assertEqInt(cur.affectedRows(),1);
console.log();


// input bind by position
console.log("INPUT BIND BY POSITION: ");
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
	"	@var18, "+
	"	@var19, "+
	"	@var20, "+
	"	@var21, "+
	"	@var22, "+
	"	@var23, "+
	"	@var24)");
assertEqInt(cur.countBindVariables(),24);
cur.inputBind("1",2);
cur.inputBind("2",2);
cur.inputBind("3",2);
cur.inputBind("4",2.5,2,1);
cur.inputBind("5",2.5,2,1);
cur.inputBind("6",2.5,2,1);
cur.inputBind("7",2.5,2,1);
cur.inputBind("8",2.00,3,2);
cur.inputBind("9",2.00,3,2);
cur.inputBind("10","01-Jan-2002 02:00:00");
cur.inputBind("11","01-Jan-2002 02:00:00");
cur.inputBind("12","testchar2");
cur.inputBind("13","testvarchar2");
cur.inputBind("14",1);
cur.inputBindClob("15","testtext2","testtext2".length);
cur.inputBindBlob("16","\x02","\x02".length);
cur.inputBindBlob("17","\x02","\x02".length);
cur.inputBind("18","testunichar2");
cur.inputBind("19","testunivarchar2");
cur.inputBindClob("20","testunitext2","testunitext2".length);
cur.inputBind("21","01-Jan-2002");
cur.inputBind("22","02:00:00");
cur.inputBind("23","02:00:00.000000");
cur.inputBind("24","01-Jan-2002 02:00:00.000000");
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2",3);
cur.inputBind("3",3);
cur.inputBind("4",3.5,2,1);
cur.inputBind("5",3.5,2,1);
cur.inputBind("6",3.5,2,1);
cur.inputBind("7",3.5,2,1);
cur.inputBind("8",3.00,3,2);
cur.inputBind("9",3.00,3,2);
cur.inputBind("10","01-Jan-2003 03:00:00");
cur.inputBind("11","01-Jan-2003 03:00:00");
cur.inputBind("12","testchar3");
cur.inputBind("13","testvarchar3");
cur.inputBind("14",1);
cur.inputBindClob("15","testtext3","testtext3".length);
cur.inputBindBlob("16","\x03","\x03".length);
cur.inputBindBlob("17","\x03","\x03".length);
cur.inputBind("18","testunichar3");
cur.inputBind("19","testunivarchar3");
cur.inputBindClob("20","testunitext3","testunitext3".length);
cur.inputBind("21","01-Jan-2003");
cur.inputBind("22","03:00:00");
cur.inputBind("23","03:00:00.000000");
cur.inputBind("24","01-Jan-2003 03:00:00.000000");
assertTrue(cur.executeQuery());
console.log();


// array of input binds by position
// sap doesn't support implicit conversion
// of string binds to other data types, so
// arrays of binds don't generally work.
// Omitting the test.


// input bind by position with validation
console.log("INPUT BIND BY POSITION "+"WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("1",4);
cur.inputBind("2",4);
cur.inputBind("3",4);
cur.inputBind("4",4.5,2,1);
cur.inputBind("5",4.5,2,1);
cur.inputBind("6",4.5,2,1);
cur.inputBind("7",4.5,2,1);
cur.inputBind("8",4.00,3,2);
cur.inputBind("9",4.00,3,2);
cur.inputBind("10","01-Jan-2004 04:00:00");
cur.inputBind("11","01-Jan-2004 04:00:00");
cur.inputBind("12","testchar4");
cur.inputBind("13","testvarchar4");
cur.inputBind("14",1);
cur.inputBindClob("15","testtext4","testtext4".length);
cur.inputBindBlob("16","\x04","\x04".length);
cur.inputBindBlob("17","\x04","\x04".length);
cur.inputBind("18","testunichar4");
cur.inputBind("19","testunivarchar4");
cur.inputBindClob("20","testunitext4","testunitext4".length);
cur.inputBind("21","01-Jan-2004");
cur.inputBind("22","04:00:00");
cur.inputBind("23","04:00:00.000000");
cur.inputBind("24","01-Jan-2004 04:00:00.000000");
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log();


// input bind by name
console.log("INPUT BIND BY NAME: ");
cur.clearBinds();
cur.inputBind("var1",5);
cur.inputBind("var2",5);
cur.inputBind("var3",5);
cur.inputBind("var4",5.5,2,1);
cur.inputBind("var5",5.5,2,1);
cur.inputBind("var6",5.5,2,1);
cur.inputBind("var7",5.5,2,1);
cur.inputBind("var8",5.00,3,2);
cur.inputBind("var9",5.00,3,2);
cur.inputBind("var10","01-Jan-2005 05:00:00");
cur.inputBind("var11","01-Jan-2005 05:00:00");
cur.inputBind("var12","testchar5");
cur.inputBind("var13","testvarchar5");
cur.inputBind("var14",1);
cur.inputBindClob("var15","testtext5","testtext5".length);
cur.inputBindBlob("var16","\x05","\x05".length);
cur.inputBindBlob("var17","\x05","\x05".length);
cur.inputBind("var18","testunichar5");
cur.inputBind("var19","testunivarchar5");
cur.inputBindClob("var20","testunitext5","testunitext5".length);
cur.inputBind("var21","01-Jan-2005");
cur.inputBind("var22","05:00:00");
cur.inputBind("var23","05:00:00.000000");
cur.inputBind("var24","01-Jan-2005 05:00:00.000000");
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("var1",6);
cur.inputBind("var2",6);
cur.inputBind("var3",6);
cur.inputBind("var4",6.5,2,1);
cur.inputBind("var5",6.5,2,1);
cur.inputBind("var6",6.5,2,1);
cur.inputBind("var7",6.5,2,1);
cur.inputBind("var8",6.00,3,2);
cur.inputBind("var9",6.00,3,2);
cur.inputBind("var10","01-Jan-2006 06:00:00");
cur.inputBind("var11","01-Jan-2006 06:00:00");
cur.inputBind("var12","testchar6");
cur.inputBind("var13","testvarchar6");
cur.inputBind("var14",1);
cur.inputBindClob("var15","testtext6","testtext6".length);
cur.inputBindBlob("var16","\x06","\x06".length);
cur.inputBindBlob("var17","\x06","\x06".length);
cur.inputBind("var18","testunichar6");
cur.inputBind("var19","testunivarchar6");
cur.inputBindClob("var20","testunitext6","testunitext6".length);
cur.inputBind("var21","01-Jan-2006");
cur.inputBind("var22","06:00:00");
cur.inputBind("var23","06:00:00.000000");
cur.inputBind("var24","01-Jan-2006 06:00:00.000000");
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("var1",7);
cur.inputBind("var2",7);
cur.inputBind("var3",7);
cur.inputBind("var4",7.5,2,1);
cur.inputBind("var5",7.5,2,1);
cur.inputBind("var6",7.5,2,1);
cur.inputBind("var7",7.5,2,1);
cur.inputBind("var8",7.00,3,2);
cur.inputBind("var9",7.00,3,2);
cur.inputBind("var10","01-Jan-2007 07:00:00");
cur.inputBind("var11","01-Jan-2007 07:00:00");
cur.inputBind("var12","testchar7");
cur.inputBind("var13","testvarchar7");
cur.inputBind("var14",1);
cur.inputBindClob("var15","testtext7","testtext7".length);
cur.inputBindBlob("var16","\x07","\x07".length);
cur.inputBindBlob("var17","\x07","\x07".length);
cur.inputBind("var18","testunichar7");
cur.inputBind("var19","testunivarchar7");
cur.inputBindClob("var20","testunitext7","testunitext7".length);
cur.inputBind("var21","01-Jan-2007");
cur.inputBind("var22","07:00:00");
cur.inputBind("var23","07:00:00.000000");
cur.inputBind("var24","01-Jan-2007 07:00:00.000000");
assertTrue(cur.executeQuery());
console.log();


// array of input binds by name
// sap doesn't support implicit conversion
// of string binds to other data types, so
// arrays of binds don't generally work.
// Omitting the test.


// input bind by name with validation
console.log("INPUT BIND BY NAME "+"WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("var1",8);
cur.inputBind("var2",8);
cur.inputBind("var3",8);
cur.inputBind("var4",8.5,2,1);
cur.inputBind("var5",8.5,2,1);
cur.inputBind("var6",8.5,2,1);
cur.inputBind("var7",8.5,2,1);
cur.inputBind("var8",8.00,3,2);
cur.inputBind("var9",8.00,3,2);
cur.inputBind("var10","01-Jan-2008 08:00:00");
cur.inputBind("var11","01-Jan-2008 08:00:00");
cur.inputBind("var12","testchar8");
cur.inputBind("var13","testvarchar8");
cur.inputBind("var14",1);
cur.inputBindClob("var15","testtext8","testtext8".length);
cur.inputBindBlob("var16","\x08","\x08".length);
cur.inputBindBlob("var17","\x08","\x08".length);
cur.inputBind("var18","testunichar8");
cur.inputBind("var19","testunivarchar8");
cur.inputBindClob("var20","testunitext8","testunitext8".length);
cur.inputBind("var21","01-Jan-2008");
cur.inputBind("var22","08:00:00");
cur.inputBind("var23","08:00:00.000000");
cur.inputBind("var24","01-Jan-2008 08:00:00.000000");
cur.inputBind("var25","junkvalue");
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log();


// select
console.log("SELECT: ");
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
console.log();


// column count
console.log("COLUMN COUNT: ");
assertEqInt(cur.colCount(),24);
console.log();


// column names
console.log("COLUMN NAMES: ");
assertEqStr(cur.getColumnName(0),"testint");
assertEqStr(cur.getColumnName(1),"testsmallint");
assertEqStr(cur.getColumnName(2),"testtinyint");
assertEqStr(cur.getColumnName(3),"testreal");
assertEqStr(cur.getColumnName(4),"testfloat");
assertEqStr(cur.getColumnName(5),"testdecimal");
assertEqStr(cur.getColumnName(6),"testnumeric");
assertEqStr(cur.getColumnName(7),"testmoney");
assertEqStr(cur.getColumnName(8),"testsmallmoney");
assertEqStr(cur.getColumnName(9),"testdatetime");
assertEqStr(cur.getColumnName(10),"testsmalldatetime");
assertEqStr(cur.getColumnName(11),"testchar");
assertEqStr(cur.getColumnName(12),"testvarchar");
assertEqStr(cur.getColumnName(13),"testbit");
assertEqStr(cur.getColumnName(15),"testbinary");
assertEqStr(cur.getColumnName(16),"testvarbinary");
assertEqStr(cur.getColumnName(17),"testunichar");
assertEqStr(cur.getColumnName(18),"testunivarchar");
assertEqStr(cur.getColumnName(19),"testunitext");
assertEqStr(cur.getColumnName(20),"testdate");
assertEqStr(cur.getColumnName(21),"testtime");
assertEqStr(cur.getColumnName(22),"testbigtime");
assertEqStr(cur.getColumnName(23),"testbigdatetime");
var cols=cur.getColumnNames();
assertEqStr(cols[0],"testint");
assertEqStr(cols[1],"testsmallint");
assertEqStr(cols[2],"testtinyint");
assertEqStr(cols[3],"testreal");
assertEqStr(cols[4],"testfloat");
assertEqStr(cols[5],"testdecimal");
assertEqStr(cols[6],"testnumeric");
assertEqStr(cols[7],"testmoney");
assertEqStr(cols[8],"testsmallmoney");
assertEqStr(cols[9],"testdatetime");
assertEqStr(cols[10],"testsmalldatetime");
assertEqStr(cols[11],"testchar");
assertEqStr(cols[12],"testvarchar");
assertEqStr(cols[13],"testbit");
assertEqStr(cols[15],"testbinary");
assertEqStr(cols[16],"testvarbinary");
assertEqStr(cols[17],"testunichar");
assertEqStr(cols[18],"testunivarchar");
assertEqStr(cols[19],"testunitext");
assertEqStr(cols[20],"testdate");
assertEqStr(cols[21],"testtime");
assertEqStr(cols[22],"testbigtime");
assertEqStr(cols[23],"testbigdatetime");
console.log();


// column types
console.log("COLUMN TYPES: ");
assertEqStr(cur.getColumnType(0),"INT");
assertEqStr(cur.getColumnType("testint"),"INT");
assertEqStr(cur.getColumnType(1),"SMALLINT");
assertEqStr(cur.getColumnType("testsmallint"),"SMALLINT");
assertEqStr(cur.getColumnType(2),"TINYINT");
assertEqStr(cur.getColumnType("testtinyint"),"TINYINT");
assertEqStr(cur.getColumnType(3),"REAL");
assertEqStr(cur.getColumnType("testreal"),"REAL");
assertEqStr(cur.getColumnType(4),"FLOAT");
assertEqStr(cur.getColumnType("testfloat"),"FLOAT");
assertEqStr(cur.getColumnType(5),"DECIMAL");
assertEqStr(cur.getColumnType("testdecimal"),"DECIMAL");
assertEqStr(cur.getColumnType(6),"NUMERIC");
assertEqStr(cur.getColumnType("testnumeric"),"NUMERIC");
assertEqStr(cur.getColumnType(7),"MONEY");
assertEqStr(cur.getColumnType("testmoney"),"MONEY");
assertEqStr(cur.getColumnType(8),"SMALLMONEY");
assertEqStr(cur.getColumnType("testsmallmoney"),
	"SMALLMONEY");
assertEqStr(cur.getColumnType(9),"DATETIME");
assertEqStr(cur.getColumnType("testdatetime"),"DATETIME");
assertEqStr(cur.getColumnType(10),"SMALLDATETIME");
assertEqStr(cur.getColumnType("testsmalldatetime"),
	"SMALLDATETIME");
assertEqStr(cur.getColumnType(11),"CHAR");
assertEqStr(cur.getColumnType("testchar"),"CHAR");
assertEqStr(cur.getColumnType(12),"VARCHAR");
assertEqStr(cur.getColumnType("testvarchar"),"VARCHAR");
assertEqStr(cur.getColumnType(13),"BIT");
assertEqStr(cur.getColumnType("testbit"),"BIT");
assertEqStr(cur.getColumnType(15),"BINARY");
assertEqStr(cur.getColumnType("testbinary"),"BINARY");
assertEqStr(cur.getColumnType(16),"VARBINARY");
assertEqStr(cur.getColumnType("testvarbinary"),"VARBINARY");
assertEqStr(cur.getColumnType(17),"NCHAR");
assertEqStr(cur.getColumnType("testunichar"),"NCHAR");
assertEqStr(cur.getColumnType(18),"NVARCHAR");
assertEqStr(cur.getColumnType("testunivarchar"),"NVARCHAR");
assertEqStr(cur.getColumnType(19),"NTEXT");
assertEqStr(cur.getColumnType("testunitext"),"NTEXT");
assertEqStr(cur.getColumnType(20),"DATE");
assertEqStr(cur.getColumnType("testdate"),"DATE");
assertEqStr(cur.getColumnType(21),"TIME");
assertEqStr(cur.getColumnType("testtime"),"TIME");
assertEqStr(cur.getColumnType(22),"TIME");
assertEqStr(cur.getColumnType("testbigtime"),"TIME");
assertEqStr(cur.getColumnType(23),"TIMESTAMP");
assertEqStr(cur.getColumnType("testbigdatetime"),"TIMESTAMP");
console.log();


// column length
console.log("COLUMN LENGTH: ");
assertEqInt(cur.getColumnLength(0),4);
assertEqInt(cur.getColumnLength("testint"),4);
assertEqInt(cur.getColumnLength(1),2);
assertEqInt(cur.getColumnLength("testsmallint"),2);
assertEqInt(cur.getColumnLength(2),1);
assertEqInt(cur.getColumnLength("testtinyint"),1);
assertEqInt(cur.getColumnLength(3),4);
assertEqInt(cur.getColumnLength("testreal"),4);
assertEqInt(cur.getColumnLength(4),8);
assertEqInt(cur.getColumnLength("testfloat"),8);
assertEqInt(cur.getColumnLength(5),35);
assertEqInt(cur.getColumnLength("testdecimal"),35);
assertEqInt(cur.getColumnLength(6),35);
assertEqInt(cur.getColumnLength("testnumeric"),35);
assertEqInt(cur.getColumnLength(7),8);
assertEqInt(cur.getColumnLength("testmoney"),8);
assertEqInt(cur.getColumnLength(8),4);
assertEqInt(cur.getColumnLength("testsmallmoney"),4);
assertEqInt(cur.getColumnLength(9),8);
assertEqInt(cur.getColumnLength("testdatetime"),8);
assertEqInt(cur.getColumnLength(10),4);
assertEqInt(cur.getColumnLength("testsmalldatetime"),4);
assertEqInt(cur.getColumnLength(11),40);
assertEqInt(cur.getColumnLength("testchar"),40);
assertEqInt(cur.getColumnLength(12),40);
assertEqInt(cur.getColumnLength("testvarchar"),40);
assertEqInt(cur.getColumnLength(13),1);
assertEqInt(cur.getColumnLength("testbit"),1);
assertEqInt(cur.getColumnLength(15),20);
assertEqInt(cur.getColumnLength("testbinary"),20);
assertEqInt(cur.getColumnLength(16),20);
assertEqInt(cur.getColumnLength("testvarbinary"),20);
assertEqInt(cur.getColumnLength(17),40);
assertEqInt(cur.getColumnLength("testunichar"),40);
assertEqInt(cur.getColumnLength(18),40);
assertEqInt(cur.getColumnLength("testunivarchar"),40);
assertEqInt(cur.getColumnLength(19),32768);
assertEqInt(cur.getColumnLength("testunitext"),32768);
assertEqInt(cur.getColumnLength(20),4);
assertEqInt(cur.getColumnLength("testdate"),4);
assertEqInt(cur.getColumnLength(21),4);
assertEqInt(cur.getColumnLength("testtime"),4);
assertEqInt(cur.getColumnLength(22),8);
assertEqInt(cur.getColumnLength("testbigtime"),8);
assertEqInt(cur.getColumnLength(23),8);
assertEqInt(cur.getColumnLength("testbigdatetime"),8);
console.log();


// longest column
console.log("LONGEST COLUMN: ");
assertEqInt(cur.getLongest(0),1);
assertEqInt(cur.getLongest("testint"),1);
assertEqInt(cur.getLongest(1),1);
assertEqInt(cur.getLongest("testsmallint"),1);
assertEqInt(cur.getLongest(2),1);
assertEqInt(cur.getLongest("testtinyint"),1);
assertEqInt(cur.getLongest(3),3);
assertEqInt(cur.getLongest("testreal"),3);
assertEqInt(cur.getLongest(4),3);
assertEqInt(cur.getLongest("testfloat"),3);
assertEqInt(cur.getLongest(5),3);
assertEqInt(cur.getLongest("testdecimal"),3);
assertEqInt(cur.getLongest(6),3);
assertEqInt(cur.getLongest("testnumeric"),3);
assertEqInt(cur.getLongest(7),4);
assertEqInt(cur.getLongest("testmoney"),4);
assertEqInt(cur.getLongest(8),4);
assertEqInt(cur.getLongest("testsmallmoney"),4);
assertEqInt(cur.getLongest(9),19);
assertEqInt(cur.getLongest("testdatetime"),19);
assertEqInt(cur.getLongest(10),19);
assertEqInt(cur.getLongest("testsmalldatetime"),19);
assertEqInt(cur.getLongest(11),40);
assertEqInt(cur.getLongest("testchar"),40);
assertEqInt(cur.getLongest(12),12);
assertEqInt(cur.getLongest("testvarchar"),12);
assertEqInt(cur.getLongest(13),1);
assertEqInt(cur.getLongest("testbit"),1);
assertEqInt(cur.getLongest(15),40);
assertEqInt(cur.getLongest("testbinary"),40);
assertEqInt(cur.getLongest(16),2);
assertEqInt(cur.getLongest("testvarbinary"),2);
assertEqInt(cur.getLongest(17),20);
assertEqInt(cur.getLongest("testunichar"),20);
assertEqInt(cur.getLongest(18),15);
assertEqInt(cur.getLongest("testunivarchar"),15);
assertEqInt(cur.getLongest(19),12);
assertEqInt(cur.getLongest("testunitext"),12);
assertEqInt(cur.getLongest(20),11);
assertEqInt(cur.getLongest("testdate"),11);
assertEqInt(cur.getLongest(21),7);
assertEqInt(cur.getLongest("testtime"),7);
assertEqInt(cur.getLongest(22),7);
assertEqInt(cur.getLongest("testbigtime"),7);
assertEqInt(cur.getLongest(23),19);
assertEqInt(cur.getLongest("testbigdatetime"),19);
console.log();


// row count
console.log("ROW COUNT: ");
assertEqInt(cur.rowCount(),8);
console.log();


// total rows
console.log("TOTAL ROWS: ");
assertEqInt(cur.totalRows(),0);
console.log();


// first row index
console.log("FIRST ROW INDEX: ");
assertEqInt(cur.firstRowIndex(),0);
console.log();


// end of result set
console.log("END OF RESULT SET: ");
assertTrue(cur.endOfResultSet());
console.log();


// fields by index
console.log("FIELDS BY INDEX: ");
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),"1");
assertEqStr(cur.getField(0,3),"1.5");
assertEqStr(cur.getField(0,4),"1.5");
assertEqStr(cur.getField(0,5),"1.5");
assertEqStr(cur.getField(0,6),"1.5");
assertEqStr(cur.getField(0,7),"1.00");
assertEqStr(cur.getField(0,8),"1.00");
assertEqStr(cur.getField(0,9),"Jan  1 2001  1:00AM");
assertEqStr(cur.getField(0,10),"Jan  1 2001  1:00AM");
assertEqStr(cur.getField(0,11),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,12),"testvarchar1");
assertEqStr(cur.getField(0,13),"1");
assertEqStr(cur.getField(0,15),"0100000000000000000000000000000000000000");
assertEqStr(cur.getField(0,16),"01");
assertEqStr(cur.getField(0,17),"testunichar1"+
	"        ");
assertEqStr(cur.getField(0,18),"testunivarchar1");
assertEqStr(cur.getField(0,19),"testunitext1");
assertEqStr(cur.getField(0,20),"Jan  1 2001");
assertEqStr(cur.getField(0,21)," 1:00AM");
assertEqStr(cur.getField(0,22)," 1:00AM");
assertEqStr(cur.getField(0,23),"Jan  1 2001  1:00AM");
console.log();
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(7,2),"8");
assertEqStr(cur.getField(7,3),"8.5");
assertEqStr(cur.getField(7,4),"8.5");
assertEqStr(cur.getField(7,5),"8.5");
assertEqStr(cur.getField(7,6),"8.5");
assertEqStr(cur.getField(7,7),"8.00");
assertEqStr(cur.getField(7,8),"8.00");
assertEqStr(cur.getField(7,9),"Jan  1 2008  8:00AM");
assertEqStr(cur.getField(7,10),"Jan  1 2008  8:00AM");
assertEqStr(cur.getField(7,11),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,12),"testvarchar8");
assertEqStr(cur.getField(7,13),"1");
assertEqStr(cur.getField(7,15),"0800000000000000000000000000000000000000");
assertEqStr(cur.getField(7,16),"08");
assertEqStr(cur.getField(7,17),"testunichar8"+
	"        ");
assertEqStr(cur.getField(7,18),"testunivarchar8");
assertEqStr(cur.getField(7,19),"testunitext8");
assertEqStr(cur.getField(7,20),"Jan  1 2008");
assertEqStr(cur.getField(7,21)," 8:00AM");
assertEqStr(cur.getField(7,22)," 8:00AM");
assertEqStr(cur.getField(7,23),"Jan  1 2008  8:00AM");
console.log();


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqInt(cur.getFieldLength(0,0),1);
assertEqInt(cur.getFieldLength(0,1),1);
assertEqInt(cur.getFieldLength(0,2),1);
assertEqInt(cur.getFieldLength(0,3),3);
assertEqInt(cur.getFieldLength(0,4),3);
assertEqInt(cur.getFieldLength(0,5),3);
assertEqInt(cur.getFieldLength(0,6),3);
assertEqInt(cur.getFieldLength(0,7),4);
assertEqInt(cur.getFieldLength(0,8),4);
assertEqInt(cur.getFieldLength(0,9),19);
assertEqInt(cur.getFieldLength(0,10),19);
assertEqInt(cur.getFieldLength(0,11),40);
assertEqInt(cur.getFieldLength(0,12),12);
assertEqInt(cur.getFieldLength(0,13),1);
assertEqInt(cur.getFieldLength(0,15),40);
assertEqInt(cur.getFieldLength(0,16),2);
assertEqInt(cur.getFieldLength(0,17),20);
assertEqInt(cur.getFieldLength(0,18),15);
assertEqInt(cur.getFieldLength(0,19),12);
assertEqInt(cur.getFieldLength(0,20),11);
assertEqInt(cur.getFieldLength(0,21),7);
assertEqInt(cur.getFieldLength(0,22),7);
assertEqInt(cur.getFieldLength(0,23),19);
console.log();
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),1);
assertEqInt(cur.getFieldLength(7,2),1);
assertEqInt(cur.getFieldLength(7,3),3);
assertEqInt(cur.getFieldLength(7,4),3);
assertEqInt(cur.getFieldLength(7,5),3);
assertEqInt(cur.getFieldLength(7,6),3);
assertEqInt(cur.getFieldLength(7,7),4);
assertEqInt(cur.getFieldLength(7,8),4);
assertEqInt(cur.getFieldLength(7,9),19);
assertEqInt(cur.getFieldLength(7,10),19);
assertEqInt(cur.getFieldLength(7,11),40);
assertEqInt(cur.getFieldLength(7,12),12);
assertEqInt(cur.getFieldLength(7,13),1);
assertEqInt(cur.getFieldLength(7,15),40);
assertEqInt(cur.getFieldLength(7,16),2);
assertEqInt(cur.getFieldLength(7,17),20);
assertEqInt(cur.getFieldLength(7,18),15);
assertEqInt(cur.getFieldLength(7,19),12);
assertEqInt(cur.getFieldLength(7,20),11);
assertEqInt(cur.getFieldLength(7,21),7);
assertEqInt(cur.getFieldLength(7,22),7);
assertEqInt(cur.getFieldLength(7,23),19);
console.log();


// fields by name
console.log("FIELDS BY NAME: ");
assertEqStr(cur.getField(0,"testint"),"1");
assertEqStr(cur.getField(0,"testsmallint"),"1");
assertEqStr(cur.getField(0,"testtinyint"),"1");
assertEqStr(cur.getField(0,"testreal"),"1.5");
assertEqStr(cur.getField(0,"testfloat"),"1.5");
assertEqStr(cur.getField(0,"testdecimal"),"1.5");
assertEqStr(cur.getField(0,"testnumeric"),"1.5");
assertEqStr(cur.getField(0,"testmoney"),"1.00");
assertEqStr(cur.getField(0,"testsmallmoney"),"1.00");
assertEqStr(cur.getField(0,"testdatetime"),
	"Jan  1 2001  1:00AM");
assertEqStr(cur.getField(0,"testsmalldatetime"),
	"Jan  1 2001  1:00AM");
assertEqStr(cur.getField(0,"testchar"),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,"testvarchar"),"testvarchar1");
assertEqStr(cur.getField(0,"testbit"),"1");
assertEqStr(cur.getField(0,"testbinary"),
	"0100000000000000000000000000000000000000");
assertEqStr(cur.getField(0,"testvarbinary"),"01");
assertEqStr(cur.getField(0,"testunichar"),"testunichar1"+
	"        ");
assertEqStr(cur.getField(0,"testunivarchar"),"testunivarchar1");
assertEqStr(cur.getField(0,"testunitext"),"testunitext1");
assertEqStr(cur.getField(0,"testdate"),"Jan  1 2001");
assertEqStr(cur.getField(0,"testtime")," 1:00AM");
assertEqStr(cur.getField(0,"testbigtime")," 1:00AM");
assertEqStr(cur.getField(0,"testbigdatetime"),"Jan  1 2001  1:00AM");
console.log();
assertEqStr(cur.getField(7,"testint"),"8");
assertEqStr(cur.getField(7,"testsmallint"),"8");
assertEqStr(cur.getField(7,"testtinyint"),"8");
assertEqStr(cur.getField(7,"testreal"),"8.5");
assertEqStr(cur.getField(7,"testfloat"),"8.5");
assertEqStr(cur.getField(7,"testdecimal"),"8.5");
assertEqStr(cur.getField(7,"testnumeric"),"8.5");
assertEqStr(cur.getField(7,"testmoney"),"8.00");
assertEqStr(cur.getField(7,"testsmallmoney"),"8.00");
assertEqStr(cur.getField(7,"testdatetime"),
	"Jan  1 2008  8:00AM");
assertEqStr(cur.getField(7,"testsmalldatetime"),
	"Jan  1 2008  8:00AM");
assertEqStr(cur.getField(7,"testchar"),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,"testvarchar"),"testvarchar8");
assertEqStr(cur.getField(7,"testbit"),"1");
assertEqStr(cur.getField(7,"testbinary"),
	"0800000000000000000000000000000000000000");
assertEqStr(cur.getField(7,"testvarbinary"),"08");
assertEqStr(cur.getField(7,"testunichar"),"testunichar8"+
	"        ");
assertEqStr(cur.getField(7,"testunivarchar"),"testunivarchar8");
assertEqStr(cur.getField(7,"testunitext"),"testunitext8");
assertEqStr(cur.getField(7,"testdate"),"Jan  1 2008");
assertEqStr(cur.getField(7,"testtime")," 8:00AM");
assertEqStr(cur.getField(7,"testbigtime")," 8:00AM");
assertEqStr(cur.getField(7,"testbigdatetime"),"Jan  1 2008  8:00AM");
console.log();


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqInt(cur.getFieldLength(0,"testint"),1);
assertEqInt(cur.getFieldLength(0,"testsmallint"),1);
assertEqInt(cur.getFieldLength(0,"testtinyint"),1);
assertEqInt(cur.getFieldLength(0,"testreal"),3);
assertEqInt(cur.getFieldLength(0,"testfloat"),3);
assertEqInt(cur.getFieldLength(0,"testdecimal"),3);
assertEqInt(cur.getFieldLength(0,"testnumeric"),3);
assertEqInt(cur.getFieldLength(0,"testmoney"),4);
assertEqInt(cur.getFieldLength(0,"testsmallmoney"),4);
assertEqInt(cur.getFieldLength(0,"testdatetime"),19);
assertEqInt(cur.getFieldLength(0,"testsmalldatetime"),19);
assertEqInt(cur.getFieldLength(0,"testchar"),40);
assertEqInt(cur.getFieldLength(0,"testvarchar"),12);
assertEqInt(cur.getFieldLength(0,"testbit"),1);
assertEqInt(cur.getFieldLength(0,"testbinary"),40);
assertEqInt(cur.getFieldLength(0,"testvarbinary"),2);
assertEqInt(cur.getFieldLength(0,"testunichar"),20);
assertEqInt(cur.getFieldLength(0,"testunivarchar"),15);
assertEqInt(cur.getFieldLength(0,"testunitext"),12);
assertEqInt(cur.getFieldLength(0,"testdate"),11);
assertEqInt(cur.getFieldLength(0,"testtime"),7);
assertEqInt(cur.getFieldLength(0,"testbigtime"),7);
assertEqInt(cur.getFieldLength(0,"testbigdatetime"),19);
console.log();
assertEqInt(cur.getFieldLength(7,"testint"),1);
assertEqInt(cur.getFieldLength(7,"testsmallint"),1);
assertEqInt(cur.getFieldLength(7,"testtinyint"),1);
assertEqInt(cur.getFieldLength(7,"testreal"),3);
assertEqInt(cur.getFieldLength(7,"testfloat"),3);
assertEqInt(cur.getFieldLength(7,"testdecimal"),3);
assertEqInt(cur.getFieldLength(7,"testnumeric"),3);
assertEqInt(cur.getFieldLength(7,"testmoney"),4);
assertEqInt(cur.getFieldLength(7,"testsmallmoney"),4);
assertEqInt(cur.getFieldLength(7,"testdatetime"),19);
assertEqInt(cur.getFieldLength(7,"testsmalldatetime"),19);
assertEqInt(cur.getFieldLength(7,"testchar"),40);
assertEqInt(cur.getFieldLength(7,"testvarchar"),12);
assertEqInt(cur.getFieldLength(7,"testbit"),1);
assertEqInt(cur.getFieldLength(7,"testbinary"),40);
assertEqInt(cur.getFieldLength(7,"testvarbinary"),2);
assertEqInt(cur.getFieldLength(7,"testunichar"),20);
assertEqInt(cur.getFieldLength(7,"testunivarchar"),15);
assertEqInt(cur.getFieldLength(7,"testunitext"),12);
assertEqInt(cur.getFieldLength(7,"testdate"),11);
assertEqInt(cur.getFieldLength(7,"testtime"),7);
assertEqInt(cur.getFieldLength(7,"testbigtime"),7);
assertEqInt(cur.getFieldLength(7,"testbigdatetime"),19);
console.log();


// fields by array
console.log("FIELDS BY ARRAY: ");
var fields=cur.getRow(0);
assertEqStr(fields[0],"1");
assertEqStr(fields[1],"1");
assertEqStr(fields[2],"1");
assertEqStr(fields[3],"1.5");
assertEqStr(fields[4],"1.5");
assertEqStr(fields[5],"1.5");
assertEqStr(fields[6],"1.5");
assertEqStr(fields[7],"1.00");
assertEqStr(fields[8],"1.00");
assertEqStr(fields[9],"Jan  1 2001  1:00AM");
assertEqStr(fields[10],"Jan  1 2001  1:00AM");
assertEqStr(fields[11],"testchar1"+"                               ");
assertEqStr(fields[12],"testvarchar1");
assertEqStr(fields[13],"1");
assertEqStr(fields[15],"0100000000000000000000000000000000000000");
assertEqStr(fields[16],"01");
assertEqStr(fields[17],"testunichar1"+"        ");
assertEqStr(fields[18],"testunivarchar1");
assertEqStr(fields[19],"testunitext1");
assertEqStr(fields[20],"Jan  1 2001");
assertEqStr(fields[21]," 1:00AM");
assertEqStr(fields[22]," 1:00AM");
assertEqStr(fields[23],"Jan  1 2001  1:00AM");
console.log();


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
var fieldlens=cur.getRowLengths(0);
assertEqInt(fieldlens[0],1);
assertEqInt(fieldlens[1],1);
assertEqInt(fieldlens[2],1);
assertEqInt(fieldlens[3],3);
assertEqInt(fieldlens[4],3);
assertEqInt(fieldlens[5],3);
assertEqInt(fieldlens[6],3);
assertEqInt(fieldlens[7],4);
assertEqInt(fieldlens[8],4);
assertEqInt(fieldlens[9],19);
assertEqInt(fieldlens[10],19);
assertEqInt(fieldlens[11],40);
assertEqInt(fieldlens[12],12);
assertEqInt(fieldlens[13],1);
assertEqInt(fieldlens[15],40);
assertEqInt(fieldlens[16],2);
assertEqInt(fieldlens[17],20);
assertEqInt(fieldlens[18],15);
assertEqInt(fieldlens[19],12);
assertEqInt(fieldlens[20],11);
assertEqInt(fieldlens[21],7);
assertEqInt(fieldlens[22],7);
assertEqInt(fieldlens[23],19);
console.log();


// result set buffer size
console.log("RESULT SET BUFFER SIZE: ");
assertEqInt(cur.getResultSetBufferSize(),0);
cur.setResultSetBufferSize(2);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqInt(cur.getResultSetBufferSize(),2);
console.log();
assertEqInt(cur.firstRowIndex(),0);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),2);
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
console.log();
assertEqInt(cur.firstRowIndex(),2);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),4);
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
console.log();
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,0),null);
console.log();
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.setResultSetBufferSize(0);
console.log();


// dont get column info
console.log("DONT GET COLUMN INFO: ");
cur.dontGetColumnInfo();
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getColumnName(0),null);
assertEqInt(cur.getColumnLength(0),0);
assertEqStr(cur.getColumnType(0),null);
cur.getColumnInfo();
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getColumnName(0),"testint");
assertEqInt(cur.getColumnLength(0),4);
assertEqStr(cur.getColumnType(0),"INT");
console.log();


// suspended session
console.log("SUSPENDED SESSION: ");
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
cur.suspendResultSet();
assertTrue(con.suspendSession());
var port=con.getConnectionPort();
var socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
console.log();
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
assertEqStr(cur.getField(3,0),"4");
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
console.log();
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
assertEqStr(cur.getField(3,0),"4");
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
console.log();
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
console.log();
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
assertEqStr(cur.getField(3,0),"4");
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
console.log();


// suspended result set
console.log("SUSPENDED RESULT SET: ");
cur.setResultSetBufferSize(2);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(2,0),"3");
var id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
assertTrue(cur.resumeResultSet(id));
console.log();
assertEqInt(cur.firstRowIndex(),4);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),6);
assertEqStr(cur.getField(7,0),"8");
console.log();
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,0),null);
console.log();
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.setResultSetBufferSize(0);
console.log();


// cached result set
console.log("CACHED RESULT SET: ");
cur.cacheToFile("cachefile1-sap");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
var filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-sap");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
console.log();


// column count for cached result set
console.log("COLUMN COUNT FOR "+"CACHED RESULT SET: ");
assertEqInt(cur.colCount(),24);
console.log();


// column names for cached result set
console.log("COLUMN NAMES FOR "+"CACHED RESULT SET: ");
assertEqStr(cur.getColumnName(0),"testint");
assertEqStr(cur.getColumnName(1),"testsmallint");
assertEqStr(cur.getColumnName(2),"testtinyint");
assertEqStr(cur.getColumnName(3),"testreal");
assertEqStr(cur.getColumnName(4),"testfloat");
assertEqStr(cur.getColumnName(5),"testdecimal");
assertEqStr(cur.getColumnName(6),"testnumeric");
assertEqStr(cur.getColumnName(7),"testmoney");
assertEqStr(cur.getColumnName(8),"testsmallmoney");
assertEqStr(cur.getColumnName(9),"testdatetime");
assertEqStr(cur.getColumnName(10),"testsmalldatetime");
assertEqStr(cur.getColumnName(11),"testchar");
assertEqStr(cur.getColumnName(12),"testvarchar");
assertEqStr(cur.getColumnName(13),"testbit");
assertEqStr(cur.getColumnName(15),"testbinary");
assertEqStr(cur.getColumnName(16),"testvarbinary");
assertEqStr(cur.getColumnName(17),"testunichar");
assertEqStr(cur.getColumnName(18),"testunivarchar");
assertEqStr(cur.getColumnName(19),"testunitext");
assertEqStr(cur.getColumnName(20),"testdate");
assertEqStr(cur.getColumnName(21),"testtime");
assertEqStr(cur.getColumnName(22),"testbigtime");
assertEqStr(cur.getColumnName(23),"testbigdatetime");
cols=cur.getColumnNames();
assertEqStr(cols[0],"testint");
assertEqStr(cols[1],"testsmallint");
assertEqStr(cols[2],"testtinyint");
assertEqStr(cols[3],"testreal");
assertEqStr(cols[4],"testfloat");
assertEqStr(cols[5],"testdecimal");
assertEqStr(cols[6],"testnumeric");
assertEqStr(cols[7],"testmoney");
assertEqStr(cols[8],"testsmallmoney");
assertEqStr(cols[9],"testdatetime");
assertEqStr(cols[10],"testsmalldatetime");
assertEqStr(cols[11],"testchar");
assertEqStr(cols[12],"testvarchar");
assertEqStr(cols[13],"testbit");
assertEqStr(cols[15],"testbinary");
assertEqStr(cols[16],"testvarbinary");
assertEqStr(cols[17],"testunichar");
assertEqStr(cols[18],"testunivarchar");
assertEqStr(cols[19],"testunitext");
assertEqStr(cols[20],"testdate");
assertEqStr(cols[21],"testtime");
assertEqStr(cols[22],"testbigtime");
assertEqStr(cols[23],"testbigdatetime");
console.log();


// cached result set with result set
// buffer size
console.log("CACHED RESULT SET WITH "+"RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-sap");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-sap");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// from one cache file to another
console.log("FROM ONE CACHE FILE "+"TO ANOTHER: ");
cur.cacheToFile("cachefile2-sap");
assertTrue(cur.openCachedResultSet("cachefile1-sap"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-sap"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
console.log();


// from one cache file to another with
// result set buffer size
console.log("FROM ONE CACHE FILE TO ANOTHER "+"WITH RESULT SET "+
	"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile2-sap");
assertTrue(cur.openCachedResultSet("cachefile1-sap"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-sap"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// cached result set with suspend and
// result set buffer size
console.log("CACHED RESULT SET "+"WITH SUSPEND "+"AND RESULT SET "+
	"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-sap");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(2,0),"3");
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-sap");
id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
console.log();
assertTrue(con.resumeSession(port,socket));
assertTrue(cur.resumeCachedResultSet(id,filename));
console.log();
assertEqInt(cur.firstRowIndex(),4);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),6);
assertEqStr(cur.getField(7,0),"8");
console.log();
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,0),null);
console.log();
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.cacheOff();
console.log();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// finished suspended session
console.log("FINISHED "+"SUSPENDED SESSION: ");
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
assertTrue(cur.resumeResultSet(id));
assertEqStr(cur.getField(4,0),null);
assertEqStr(cur.getField(5,0),null);
assertEqStr(cur.getField(6,0),null);
assertEqStr(cur.getField(7,0),null);
console.log();


// nested selects
console.log("NESTED SELECTS: ");
cur.setResultSetBufferSize(1);
assertTrue(cur.sendQuery("select * from testtable"));
var secondcur=new sqlrelay.SQLRCursor(con);
secondcur.setResultSetBufferSize(1);
for (var i=0; cur.getRow(i); i++) {
	assertTrue(secondcur.sendQuery("select * "+"from "+
			"testtable"));
}
// the nested selects must not disturb the outer result set
assertEqInt(i,cur.rowCount());
secondcur.closeResultSet();
cur.setResultSetBufferSize(0);
assertTrue(con.commit());
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// reset transaction state
console.log("RESET TRANSACTION STATE: ");
assertTrue(con.commit());
assertEqStr(con.getTransactionModel(),"explicit-error");
assertTrue(con.getAutoCommit());
console.log();


// transaction behavior - implicit
console.log("TRANSACTION BEHAVIOR - implicit: ");
// sap ase rejects DDL inside a chained-mode (multi-statement) tx
// unless `sp_dboption ... 'ddl in tran', true` is set on the db;
// create the table while still in unchained mode, then switch.
// `lock datarows` is needed so secondcur's count(*) scan doesn't
// block on the writer's page lock from the in-flight insert
assertTrue(cur.sendQuery(
	"create table testtable (col1 integer) lock datarows"));
assertTrue(con.setTransactionModel("implicit"));
assertEqStr(con.getTransactionModel(),"implicit");
var secondcon=new sqlrelay.SQLRConnection("sqlrelay",9006,"/tmp/saptest.socket",
	"testuser","testpassword",0,1);
setSecondConnection(secondcon);
var secondcur=new sqlrelay.SQLRCursor(secondcon);
setSecondCursor(secondcur);
// session is in a transaction; insert is not visible until commit
assertTrue(con.getInTransaction());
assertFalse(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"0");
// commit makes it visible, and implicitly starts a new transaction
assertTrue(con.commit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// rollback discards, and implicitly starts a new transaction
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
secondcur.closeResultSet();
// switch back to unchained mode so the drop isn't rejected
assertTrue(con.setTransactionModel("explicit-error"));
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// transaction behavior - explicit
console.log("TRANSACTION BEHAVIOR - explicit: ");
assertTrue(con.setTransactionModel("explicit"));
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
// begin starts a new transaction; insert is not visible until commit
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"0");
// commit makes it visible; no new transaction is started
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback discards; no new transaction is started
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
secondcur.closeResultSet();
// switch back to unchained mode so the drop isn't rejected
assertTrue(con.setTransactionModel("explicit-error"));
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// transaction behavior - explicit-deferred
console.log("TRANSACTION BEHAVIOR - explicit-deferred: ");
assertTrue(con.setTransactionModel("explicit-deferred"));
assertEqStr(con.getTransactionModel(),"explicit-deferred");
// switch to autocommit-on so the begin/commit cycles below
// bracket explicit transactions (autocommit-off semantics are
// exercised at the end of this block)
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
// begin starts a transaction; commit makes it visible
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback discards
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// during a transaction started by begin(), autoCommitOn is a
// no-op: the autocommit setting takes effect after the user
// explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(con.autoCommitOn());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// explicit commit ends the tx; autocommit-on now takes effect
assertTrue(con.commit());
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autocommit is on; subsequent inserts are visible immediately
assertTrue(cur.sendQuery("insert into testtable values (4)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"3");
// autoCommitOff takes effect immediately when not in a transaction
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
// autocommit-off persists across commit/rollback; each commit or
// rollback ends the current implicit tx and a new one starts for
// the next statement
assertTrue(cur.sendQuery("insert into testtable values (5)"));
assertTrue(con.commit());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"4");
assertTrue(cur.sendQuery("insert into testtable values (6)"));
assertTrue(con.rollback());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"4");
// autoCommitOff during a transaction changes the variable
// immediately but the in-flight tx continues; only after the
// next explicit commit/rollback does the new autocommit-off
// setting drop us into a new implicit tx (mysql-asymmetric
// semantic)
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (7)"));
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"4");
assertTrue(con.commit());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"5");
secondcur.closeResultSet();
// switch back to unchained mode so the drop isn't rejected
assertTrue(con.setTransactionModel("explicit-error"));
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// transaction behavior - explicit-error
console.log("TRANSACTION BEHAVIOR - explicit-error: ");
assertTrue(con.setTransactionModel("explicit-error"));
assertEqStr(con.getTransactionModel(),"explicit-error");
assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
// begin, insert, commit
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// while in a transaction, autoCommitOn/Off throw an error
assertTrue(con.begin());
assertFalse(con.autoCommitOn());
assertFalse(con.autoCommitOff());
assertTrue(con.commit());
// outside of a transaction, autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
secondcur.closeResultSet();
// commit the open tx so the drop isn't rejected as DDL inside a
// chained-mode transaction (in explicit-error model, autoCommitOn
// from inside a tx errors out by design, so commit is the route
// back to autocommit-on / unchained mode)
assertTrue(con.commit());
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// transaction behavior - none
console.log("TRANSACTION BEHAVIOR - none: ");
assertTrue(con.setTransactionModel("none"));
assertEqStr(con.getTransactionModel(),"none");
assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
// no transactions; everything is visible immediately
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// commit and rollback are no-ops
assertTrue(con.commit());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autocommit is always on; autoCommitOff is an error
assertFalse(con.autoCommitOff());
assertTrue(con.getAutoCommit());
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
secondcur.closeResultSet();
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// reset transaction behavior
console.log("RESET TRANSACTION BEHAVIOR: ");
assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
assertEqStr(con.getTransactionModel(),"explicit-error");
assertTrue(con.getAutoCommit());
console.log();


// individual substitutions
console.log("INDIVIDUAL "+"SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),"+"'$(var2)',$(var3)");
cur.substitution("var1",1);
cur.substitution("var2","hello");
cur.substitution("var3",10.5556,6,4);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"10.5556");
console.log();


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),"+"$(var2),$(var3)");
cur.substitutions(subvars,subvallongs,null,null);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"2");
assertEqStr(cur.getField(0,2),"3");
console.log();
cur.prepareQuery("select '$(var1)',"+"'$(var2)','$(var3)'");
cur.substitutions(subvars,subvalstrings,null,null);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"hi");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"bye");
console.log();
cur.prepareQuery("select $(var1),"+"$(var2),$(var3)");
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"10.55");
assertEqStr(cur.getField(0,1),"10.556");
assertEqStr(cur.getField(0,2),"10.5556");
console.log();


// nulls as nulls
console.log("NULLS AS NULLS: ");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery("select NULL,1,NULL"));
assertEqStr(cur.getField(0,0),null);
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("select NULL,1,NULL"));
assertEqStr(cur.getField(0,0),"");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),"");
console.log();



// null and empty lobs
console.log("NULL AND EMPTY LOBS: ");
cur.getNullsAsNulls();
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob1 text NULL, "+
	"	testclob2 text NULL, "+
	"	testblob1 image NULL, "+
	"	testblob2 image NULL)"));
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	@var1, "+
	"	@var2, "+
	"	@var3, "+
	"	@var4)");
cur.inputBindClob("var1","","".length);
cur.inputBindClob("var2",null,0);
cur.inputBindBlob("var3","","".length);
cur.inputBindBlob("var4",null,0);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
// sap converts empty strings to a single
// space.  It's possible that if we had
// true input bind support on the backend,
// then this would work correctly, but for
// now we're faking binds, and inserting
// an empty string, so we have to check
// for a single space here.
assertEqStr(cur.getField(0,0)," ");
assertEqStr(cur.getField(0,1),null);
// see note above for why we're checking
// for a single space
assertEqStr(cur.getField(0,2)," ");
assertEqStr(cur.getField(0,3),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// long lobs
console.log("LONG LOBS: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(
	"create table testtable ("+
	"	testclob text NULL, "+
	"	testblob image NULL) "+"lock datarows");
cur.prepareQuery("insert into testtable "+
	"values (@var1,@var2)");
var largebuffer="C".repeat(255);
cur.inputBindClob("var1",largebuffer,largebuffer.length);
cur.inputBindBlob("var2",largebuffer,largebuffer.length);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
assertEqInt(cur.getFieldLength(0,"testclob"),255);
assertEqStr(cur.getField(0,"testclob"),largebuffer);
assertEqInt(cur.getFieldLength(0,"testblob"),255);
assertEqStrLen(cur.getField(0,"testblob"),largebuffer,255);
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// output bind by position
console.log("OUTPUT BIND "+"BY POSITION: ");
cur.sendQuery("drop procedure testproc");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@out1 int output, "+
	"	@out2 varchar(20) output, "+
	"	@out3 float output, "+
	"	@out4 datetime output, "+
	"	@out5 varchar(20) "+"output as "+
	"select @out1=1, "+
	"	@out2='hello', "+
	"	@out3=2.5, "+
	"	@out4='2001-02-03', "+
	"	@out5=null"));
cur.prepareQuery("exec testproc");
assertEqInt(cur.countBindVariables(),0);
cur.defineOutputBindInteger("1");
cur.defineOutputBindString("2",20);
cur.defineOutputBindDouble("3");
cur.defineOutputBindDate("4");
cur.defineOutputBindString("5",20);
assertTrue(cur.executeQuery());
var numvar=cur.getOutputBindInteger("1");
var stringvar=cur.getOutputBindString("2");
var floatvar=cur.getOutputBindDouble("3");
var year=cur.getOutputBindDateYear("4");
var month=cur.getOutputBindDateMonth("4");
var day=cur.getOutputBindDateDay("4");
var hour=cur.getOutputBindDateHour("4");
var minute=cur.getOutputBindDateMinute("4");
var second=cur.getOutputBindDateSecond("4");
var microsecond=cur.getOutputBindDateMicrosecond("4");
var tz=cur.getOutputBindDateTz("4");
var isnegative=cur.getOutputBindDateIsNegative("4");
var nullvar=cur.getOutputBindString("5");
assertEqInt(numvar,1);
assertEqStr(stringvar,"hello");
assertEqDbl(floatvar,2.5);
assertEqInt(year,2001);
assertEqInt(month,2);
assertEqInt(day,3);
assertEqInt(hour,0);
assertEqInt(minute,0);
assertEqInt(second,0);
assertEqInt(microsecond,0);
assertEqStr(tz,"");
assertFalse(isnegative);
assertEqStr(nullvar,null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// output bind by name
console.log("OUTPUT BIND BY NAME: ");
cur.sendQuery("drop procedure testproc");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@out1 int output, "+
	"	@out2 varchar(20) output, "+
	"	@out3 float output, "+
	"	@out4 datetime output, "+
	"	@out5 varchar(20) "+"output as "+
	"select @out1=1, "+
	"	@out2='hello', "+
	"	@out3=2.5, "+
	"	@out4='2001-02-03', "+
	"	@out5=null"));
cur.prepareQuery("exec testproc");
assertEqInt(cur.countBindVariables(),0);
cur.defineOutputBindInteger("out1");
cur.defineOutputBindString("out2",20);
cur.defineOutputBindDouble("out3");
cur.defineOutputBindDate("out4");
cur.defineOutputBindString("out5",20);
assertTrue(cur.executeQuery());
numvar=cur.getOutputBindInteger("out1");
stringvar=cur.getOutputBindString("out2");
floatvar=cur.getOutputBindDouble("out3");
year=cur.getOutputBindDateYear("out4");
month=cur.getOutputBindDateMonth("out4");
day=cur.getOutputBindDateDay("out4");
hour=cur.getOutputBindDateHour("out4");
minute=cur.getOutputBindDateMinute("out4");
second=cur.getOutputBindDateSecond("out4");
microsecond=cur.getOutputBindDateMicrosecond("out4");
tz=cur.getOutputBindDateTz("out4");
isnegative=cur.getOutputBindDateIsNegative("out4");
nullvar=cur.getOutputBindString("out5");
assertEqInt(numvar,1);
assertEqStr(stringvar,"hello");
assertEqDbl(floatvar,2.5);
assertEqInt(year,2001);
assertEqInt(month,2);
assertEqInt(day,3);
assertEqInt(hour,0);
assertEqInt(minute,0);
assertEqInt(second,0);
assertEqInt(microsecond,0);
assertEqStr(tz,"");
assertFalse(isnegative);
assertEqStr(nullvar,null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// output bind by name with validation
// validateBinds() can't be used for
// output binds, with sap.  In sap, when
// executing a procedure, you don't declare
// any bind variable delimiters in the
// query.  eg, you just do:
// "exec testproc", not
// "exec testproc(@out1,@out2)".
// If you call validateBinds(), it won't
// find any binds in the query, and will
// filter out any binds that you declare.


// lob output bind
// sap doesn't support lobs as output
// parameters to stored procedures, and
// there's no way to directly select into
// a lob bind variable


// long output bind
console.log("LONG OUTPUT BIND: ");
cur.sendQuery("drop procedure testproc");
largebuffer="C".repeat(255);
var query="create procedure testproc "+
	"@bindval varchar(255) "+"output as "+
	"set @bindval='"+largebuffer+"'";
assertTrue(cur.sendQuery(query));
cur.prepareQuery("exec testproc");
cur.defineOutputBindString("bindval",255);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindLength("bindval"),255);
assertEqStr(cur.getOutputBindString("bindval"),largebuffer);
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery("create table testtable "+"(testval int)");
cur.prepareQuery("insert into testtable "+"values (@testval)");
cur.inputBind("testval",-1);
assertTrue(cur.executeQuery());
cur.sendQuery("select testval "+"from testtable");
assertEqStr(cur.getField(0,"testval"),"-1");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// bind validation
console.log("BIND VALIDATION: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(
	"create table testtable ("+
	"	col1 varchar(20), "+
	"	col2 varchar(20), "+
	"	col3 varchar(20))");
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$(var1), "+
	"	$(var2), "+
	"	$(var3))");
cur.inputBind("var1","1");
cur.inputBind("var2","2");
cur.inputBind("var3","3");
cur.substitution("var1","@var1");
assertTrue(cur.validBind("var1"));
assertFalse(cur.validBind("var2"));
assertFalse(cur.validBind("var3"));
assertFalse(cur.validBind("var4"));
console.log();
cur.substitution("var2","@var2");
assertTrue(cur.validBind("var1"));
assertTrue(cur.validBind("var2"));
assertFalse(cur.validBind("var3"));
assertFalse(cur.validBind("var4"));
console.log();
cur.substitution("var3","@var3");
assertTrue(cur.validBind("var1"));
assertTrue(cur.validBind("var2"));
assertTrue(cur.validBind("var3"));
assertFalse(cur.validBind("var4"));
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// rebinding
console.log("REBINDING: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@in1 int, "+
	"	@out1 int output as "+"select @out1=@in1"));
cur.prepareQuery("exec testproc");
cur.inputBind("in1",1);
cur.defineOutputBindInteger("out1");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out1"),1);
cur.inputBind("in1",2);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out1"),2);
cur.inputBind("in1",3);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out1"),3);
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// reexecute
console.log("REEXECUTE: ");
cur.prepareQuery("select 1");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log();
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log();
// ASE rejects a bind marker used as a bare select-list value with
// error 164, "The untyped variable ? is allowed only in a WHERE
// clause or the SET clause of an UPDATE statement or the VALUES
// list of an INSERT statement" - its own parser restriction, not a
// sqlrelay bug; the same query fails identically against every
// client language.
cur.prepareQuery("select cast(@1 as int)");
cur.inputBind("1",1);
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log();
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log();
cur.inputBind("1",2);
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"2");
console.log();


// stored procedure returning no value
console.log("STORED PROCEDURE "+"RETURNING NO VALUE: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@in1 int, "+
	"	@in2 float, "+
	"	@in3 varchar(20) as "+"return"));
cur.prepareQuery("exec testproc");
cur.inputBind("in1",1);
cur.inputBind("in2",2.5,2,1);
cur.inputBind("in3","hello");
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// stored procedure returning
// single value
console.log("STORED PROCEDURE "+"RETURNING SINGLE VALUE: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@in1 int, "+
	"	@in2 float, "+
	"	@in3 varchar(20), "+
	"	@out1 int output as "+"select @out1=@in1"));
cur.prepareQuery("exec testproc");
cur.inputBind("in1",1);
cur.inputBind("in2",2.5,2,1);
cur.inputBind("in3","hello");
cur.defineOutputBindInteger("out1");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out1"),1);
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// stored procedure returning
// multiple values
console.log("STORED PROCEDURE RETURNING "+"MULTIPLE VALUES: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery("create procedure testproc "+
	"@in1 int, "+
	"	@in2 float, "+
	"	@in3 varchar(20), "+
	"	@out1 int output, "+
	"	@out2 float output, "+
	"	@out3 varchar(20) "+"output as "+
	"select @out1=@in1, "+
	"	@out2=@in2, "+
	"	@out3=@in3"));
cur.prepareQuery("exec testproc");
cur.inputBind("in1",1);
cur.inputBind("in2",2.5,2,1);
cur.inputBind("in3","hello");
cur.defineOutputBindInteger("out1");
cur.defineOutputBindDouble("out2");
cur.defineOutputBindString("out3",20);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out1"),1);
assertEqDbl(cur.getOutputBindDouble("out2"),2.5);
assertEqStr(cur.getOutputBindString("out3"),"hello");
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// stored procedure returning result set
console.log("STORED PROCEDURE "+"RETURNING RESULT SET: ");
cur.sendQuery("drop procedure "+"testselectproc");
assertTrue(cur.sendQuery("create procedure "+
	"testselectproc as "+
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
	"	select 8"));
assertTrue(cur.sendQuery("exec testselectproc"));
assertEqInt(cur.rowCount(),8);
assertTrue(cur.sendQuery("drop procedure "+"testselectproc"));
console.log();


// temporary tables
console.log("TEMPORARY TABLES: ");
cur.sendQuery("drop table #temptable\n");
cur.sendQuery("create table #temptable "+"(col1 int)");
assertTrue(cur.sendQuery("insert into #temptable "+
	"values (1)"));
assertTrue(cur.sendQuery("select count(*) "+"from #temptable"));
assertEqStr(cur.getField(0,0),"1");
con.endSession();
console.log();
assertFalse(cur.sendQuery("select count(*) "+"from #temptable"));
console.log();


// encoded binary data
console.log("ENCODED BINARY DATA: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable "+
	"(col1 image)"));
var buffer="";
for (var bi=0; bi<256; bi++) {
	buffer+=String.fromCharCode(bi);
}
var querystr="insert into testtable "+"values (0x";
for (var i=0; i<256; i++) {
	querystr+=buffer.charCodeAt(i).toString(16).padStart(2,"0");
}
querystr+=")";
assertTrue(cur.sendQuery(querystr));
assertTrue(cur.sendQuery("select col1 from testtable"));
// Verify the raw bytes round-tripped via length only. (Sybase ASE
// doesn't allow convert(varchar,image,2) for hex, and the Node.js
// sqlrelay binding returns strings via String::NewFromUtf8 which drops
// invalid UTF-8 sequences that arise from raw bytes 128-255, so we
// can't byte-compare the returned string directly.)
assertEqInt(cur.getFieldLength(0,0),256);
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// quotes
console.log("QUOTES: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable "+
	"(col1 varchar(4))"));
assertTrue(cur.sendQuery("insert into testtable "+
	"values ('''''')"));
assertTrue(cur.sendQuery("select col1 "+"from testtable"));
assertEqInt(cur.getFieldLength(0,0),2);
assertTrue(cur.getField(0,0)==="''");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// last insert id
console.log("LAST INSERT ID: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable "+
	"	(col1 int identity "+
	"primary key, "+
	"	col2 int)"));
assertTrue(cur.sendQuery("insert into testtable "+
	"(col2) values (1)"));
assertEqInt(con.getLastInsertId(),1);
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// database is schema
console.log("DATABASE IS SCHEMA: ");
assertFalse(con.getDatabaseIsSchema());
console.log();


// catalog list
console.log("CATALOG LIST: ");
assertTrue(cur.getCatalogList(null));
assertEqStr(cur.getColumnName(0),"Database");
assertInResultSet(cur,"Database",hostname);
console.log();


// schema list
console.log("SCHEMA LIST: ");
cur.sendQuery("drop table testtable");
// the get schema list query that is
// used with sap will only return the
// names of schemas that have at least
// one database object in them, so to
// be sure that there is one, we'll
// create a table
assertTrue(cur.sendQuery("create table testtable "+
	"(col1 int)"));
assertTrue(cur.getSchemaList(null));
assertEqStr(cur.getColumnName(0),"Database");
assertInResultSet(cur,"Database","dbo");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// table type list
console.log("TABLE TYPE LIST: ");
assertTrue(cur.getTableTypeList());
assertEqStr(cur.getColumnName(0),"table_type");
assertInResultSet(cur,"table_type","TABLE");
console.log();


// table list
console.log("TABLE LIST: ");
cur.sendQuery("drop table testtable1");
cur.sendQuery("drop table testtable2");
cur.sendQuery("drop table testtable3");
cur.sendQuery("drop table testtable4");
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	col1 int, "+
	"	col2 int)"));
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	col1 int, "+
	"	col2 int)"));
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	col1 int, "+
	"	col2 int)"));
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	col1 int, "+
	"	col2 int)"));
assertTrue(cur.getTableList(null));
assertInResultSet(cur,"Tables_in_xxx","testtable1");
assertInResultSet(cur,"Tables_in_xxx","testtable2");
assertInResultSet(cur,"Tables_in_xxx","testtable3");
assertInResultSet(cur,"Tables_in_xxx","testtable4");
assertTrue(cur.sendQuery("drop table testtable1"));
assertTrue(cur.sendQuery("drop table testtable2"));
assertTrue(cur.sendQuery("drop table testtable3"));
assertTrue(cur.sendQuery("drop table testtable4"));
console.log();


// type info list
console.log("TYPE INFO LIST: ");
assertTrue(cur.getTypeInfoList("int"));
assertEqStr(cur.getColumnName(0),"type_name");
assertEqStr(cur.getColumnName(1),"data_type");
assertEqStr(cur.getColumnName(2),"precision");
assertEqStr(cur.getColumnName(3),"literal_prefix");
assertEqStr(cur.getColumnName(4),"literal_suffix");
assertEqStr(cur.getColumnName(5),"create_params");
assertEqStr(cur.getColumnName(6),"nullable");
assertEqStr(cur.getColumnName(7),"case_sensitive");
assertEqStr(cur.getColumnName(8),"searchable");
assertEqStr(cur.getColumnName(9),"unsigned_attribute");
assertEqStr(cur.getColumnName(10),"fixed_prec_scale");
assertEqStr(cur.getColumnName(11),"auto_increment");
assertEqStr(cur.getColumnName(12),"local_type_name");
assertEqStr(cur.getColumnName(13),"minumum_scale");
assertEqStr(cur.getColumnName(14),"maxiumm_scale");
assertEqStr(cur.getColumnName(15),"sql_data_type");
assertEqStr(cur.getColumnName(16),"sql_datetime_sub");
assertEqStr(cur.getColumnName(17),"num_prec_radix");
assertEqStr(cur.getColumnName(18),"interval_precision");
assertEqStr(cur.getField(0,"type_name"),"INT");
assertEqStr(cur.getField(0,"data_type"),"4");
assertEqStr(cur.getField(0,"precision"),"10");
assertEqStr(cur.getField(0,"local_type_name"),"INT");
assertTrue(cur.getTypeInfoList("char"));
assertEqStr(cur.getField(0,"type_name"),"CHAR");
assertEqStr(cur.getField(0,"data_type"),"1");
assertEqStr(cur.getField(0,"precision"),"255");
assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
assertTrue(cur.getTypeInfoList("varchar"));
assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"255");
assertEqStr(cur.getField(0,"local_type_name"),"VARCHAR");
assertTrue(cur.getTypeInfoList("datetime"));
assertEqStr(cur.getField(0,"type_name"),"DATETIME");
assertEqStr(cur.getField(0,"data_type"),"93");
assertEqStr(cur.getField(0,"precision"),"23");
assertEqStr(cur.getField(0,"local_type_name"),"DATETIME");
console.log();


// column list
console.log("COLUMN LIST: ");
cur.sendQuery("drop table testtable");
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
	"	testsmalldatetime "+
	"smalldatetime, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testbit bit, "+
	"	testtext text)"));
assertTrue(cur.getColumnList("testtable",null));
assertEqStr(cur.getColumnName(0),"column_name");
assertEqStr(cur.getColumnName(1),"data_type");
assertEqStr(cur.getColumnName(2),"character_maximum_length");
assertEqStr(cur.getColumnName(3),"numeric_precision");
assertEqStr(cur.getColumnName(4),"numeric_scale");
assertEqStr(cur.getColumnName(5),"is_nullable");
assertEqStr(cur.getColumnName(6),"column_key");
assertEqStr(cur.getColumnName(7),"column_default");
assertEqStr(cur.getColumnName(8),"extra");
assertTrue(cur.getField(0,"column_name")==="testint");
assertTrue(cur.getField(1,"column_name")==="testsmallint");
assertTrue(cur.getField(2,"column_name")==="testtinyint");
assertTrue(cur.getField(3,"column_name")==="testreal");
assertTrue(cur.getField(4,"column_name")==="testfloat");
assertTrue(cur.getField(5,"column_name")==="testdecimal");
assertTrue(cur.getField(6,"column_name")==="testnumeric");
assertTrue(cur.getField(7,"column_name")==="testmoney");
assertTrue(cur.getField(8,"column_name")==="testsmallmoney");
assertTrue(cur.getField(9,"column_name")==="testdatetime");
assertTrue(cur.getField(10,"column_name")==="testsmalldatetime");
assertTrue(cur.getField(11,"column_name")==="testchar");
assertTrue(cur.getField(12,"column_name")==="testvarchar");
assertTrue(cur.getField(13,"column_name")==="testbit");
assertTrue(cur.getField(14,"column_name")==="testtext");
assertTrue(cur.getField(0,"data_type")==="int");
assertTrue(cur.getField(1,"data_type")==="smallint");
assertTrue(cur.getField(2,"data_type")==="tinyint");
assertTrue(cur.getField(3,"data_type")==="real");
assertTrue(cur.getField(4,"data_type")==="float");
assertTrue(cur.getField(5,"data_type")==="decimal");
assertTrue(cur.getField(6,"data_type")==="numeric");
assertTrue(cur.getField(7,"data_type")==="money");
assertTrue(cur.getField(8,"data_type")==="smallmoney");
assertTrue(cur.getField(9,"data_type")==="datetime");
assertTrue(cur.getField(10,"data_type")==="smalldatetime");
assertTrue(cur.getField(11,"data_type")==="char");
assertTrue(cur.getField(12,"data_type")==="varchar");
assertTrue(cur.getField(13,"data_type")==="bit");
assertTrue(cur.getField(14,"data_type")==="text");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// column list - auto_increment,
// primary key
console.log("COLUMN LIST - "+"auto_increment, "+"primary key: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int identity "+
	"primary key, "+
	"	col2 int)"));
assertTrue(cur.getColumnList("testtable",null));
assertEqStr(cur.getField(0,"extra"),"auto_increment");
assertEqStr(cur.getField(0,"column_key"),"PRI");
assertEqStr(cur.getField(1,"extra"),"");
assertEqStr(cur.getField(1,"column_key"),"");
console.log();
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int "+
	"primary key, "+
	"	col2 int)"));
assertTrue(cur.getColumnList("testtable",null));
assertEqStr(cur.getField(0,"extra"),"");
assertEqStr(cur.getField(0,"column_key"),"PRI");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// primary keys list
console.log("PRIMARY KEYS LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int "+
	"primary key, "+
	"	col2 int)"));
assertTrue(cur.getPrimaryKeysList("testtable",null));
assertEqStr(cur.getColumnName(0),"table");
assertEqStr(cur.getColumnName(1),"non_unique");
assertEqStr(cur.getColumnName(2),"key_name");
assertEqStr(cur.getColumnName(3),"seq_in_index");
assertEqStr(cur.getColumnName(4),"column_name");
assertEqStr(cur.getColumnName(5),"collation");
assertEqStr(cur.getColumnName(6),"cardinality");
assertEqStr(cur.getColumnName(7),"sub_part");
assertEqStr(cur.getColumnName(8),"packed");
assertEqStr(cur.getColumnName(9),"null");
assertEqStr(cur.getColumnName(10),"index_type");
assertEqStr(cur.getColumnName(11),"comment");
assertEqStr(cur.getColumnName(12),"index_comment");
assertEqInt(cur.rowCount(),1);
assertTrue(cur.getField(0,"table")==="testtable");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")==="col1");
var kn=cur.getField(0,"key_name");
assertStartsWith(kn,"testtable_col1_");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// key and index list
console.log("KEY AND INDEX LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int "+
	"primary key, "+
	"	col2 int)"));
assertTrue(cur.getKeyAndIndexList("testtable",null));
assertEqStr(cur.getColumnName(0),"table");
assertEqStr(cur.getColumnName(1),"non_unique");
assertEqStr(cur.getColumnName(2),"key_name");
assertEqStr(cur.getColumnName(3),"seq_in_index");
assertEqStr(cur.getColumnName(4),"column_name");
assertEqStr(cur.getColumnName(5),"collation");
assertEqStr(cur.getColumnName(6),"cardinality");
assertEqStr(cur.getColumnName(7),"sub_part");
assertEqStr(cur.getColumnName(8),"packed");
assertEqStr(cur.getColumnName(9),"null");
assertEqStr(cur.getColumnName(10),"index_type");
assertEqStr(cur.getColumnName(11),"comment");
assertEqStr(cur.getColumnName(12),"index_comment");
assertEqInt(cur.rowCount(),1);
assertTrue(cur.getField(0,"table")==="testtable");
assertEqStr(cur.getField(0,"non_unique"),"FALSE");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")==="col1");
assertEqStr(cur.getField(0,"collation"),"A");
assertEqStr(cur.getField(0,"index_type"),"1");
kn=cur.getField(0,"key_name");
assertStartsWith(kn,"testtable_col1_");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// procedure list
console.log("PROCEDURE LIST: ");
cur.sendQuery("drop procedure testproc1");
cur.sendQuery("drop procedure testproc2");
cur.sendQuery("drop procedure testproc3");
cur.sendQuery("drop procedure testproc4");
assertTrue(cur.sendQuery(
	"create procedure testproc1 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+"as select 1"));
assertTrue(cur.sendQuery(
	"create procedure testproc2 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+"as select 1"));
assertTrue(cur.sendQuery(
	"create procedure testproc3 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+"as select 1"));
assertTrue(cur.sendQuery(
	"create procedure testproc4 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+"as select 1"));
assertTrue(cur.getProcedureList(null));
assertInResultSet(cur,"routine_name","testproc1");
assertInResultSet(cur,"routine_name","testproc2");
assertInResultSet(cur,"routine_name","testproc3");
assertInResultSet(cur,"routine_name","testproc4");
console.log();


// procedure parameter list
console.log("PROCEDURE "+"PARAMETER LIST: ");
assertTrue(cur.getProcedureParameterList("testproc1",null));
assertEqStr(cur.getColumnName(0),"parameter_name");
assertEqStr(cur.getColumnName(1),"parameter_mode");
assertEqStr(cur.getColumnName(2),"data_type");
assertEqStr(cur.getColumnName(3),"character_maximum_length");
assertEqStr(cur.getColumnName(4),"ordinal_position");
assertEqInt(cur.rowCount(),4);
assertEqStr(cur.getField(0,"parameter_name"),"@in1");
assertEqStr(cur.getField(0,"parameter_mode"),"1");
assertEqStr(cur.getField(0,"data_type"),"int");
assertEqStr(cur.getField(0,"ordinal_position"),"1");
assertEqStr(cur.getField(1,"parameter_name"),"@in2");
assertEqStr(cur.getField(1,"parameter_mode"),"1");
assertEqStr(cur.getField(1,"data_type"),"char");
assertEqStr(cur.getField(1,"ordinal_position"),"2");
assertEqStr(cur.getField(2,"parameter_name"),"@in3");
assertEqStr(cur.getField(2,"parameter_mode"),"1");
assertEqStr(cur.getField(2,"data_type"),"varchar");
assertEqStr(cur.getField(2,"ordinal_position"),"3");
assertEqStr(cur.getField(3,"parameter_name"),"@in4");
assertEqStr(cur.getField(3,"parameter_mode"),"1");
assertEqStr(cur.getField(3,"data_type"),"datetime");
assertEqStr(cur.getField(3,"ordinal_position"),"4");
assertTrue(cur.sendQuery("drop procedure testproc1"));
assertTrue(cur.sendQuery("drop procedure testproc2"));
assertTrue(cur.sendQuery("drop procedure testproc3"));
assertTrue(cur.sendQuery("drop procedure testproc4"));
console.log();


// invalid queries
console.log("INVALID QUERIES: ");
assertFalse(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertFalse(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertFalse(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertFalse(cur.sendQuery("select * from testtable "+
	"order by testint"));
console.log();
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
console.log();
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
console.log();

reportTestStatus();

process.exit(getStatus());
