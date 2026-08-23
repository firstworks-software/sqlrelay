// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{
	setConnection, setCursor,
	setSecondConnection, setSecondCursor,
	assertEqStr, assertEqStrLen,
	assertEqInt, assertEqDbl, assertEqual,
	assertMoneyEqStr, assertMoneyEqLen,
	assertTrue, assertFalse, assertStartsWith,
	assertInResultSet,
	getStatus, reportTestStatus
}=require("./asserts.js");


var	isolationlevels=["READ COMMITTED",
				"READ UNCOMMITTED",
				"REPEATABLE READ",
				"SERIALIZABLE"];
var	subvars=["var1","var2","var3"];
var	subvallongs=[1,2,3];
var	subvalstrings=["hi","hello","bye"];
var	subvaldoubles=[10.55,10.556,10.5556];
var	precs=[4,5,6];
var	scales=[2,3,4];
var	counter=0;

var	LARGE_BUFFER_LENGTH=8192;

// SQL Server caps a varchar output parameter at 8000 characters
var	LONG_OUTPUT_BIND_LENGTH=8000;

// nvarchar(4000) is the widest an nvarchar column can be declared
// without switching to nvarchar(max)
var	WIDE_NCHAR_LENGTH=4000;

var	cols;
var	fields;
var	fieldlens;
var	port;
var	socket;
var	id;
var	filename;
var	largebuffer;
var	longoutputbindbuffer;
var	widencharbuffer;
var	buffer;
var	query;
var	name;
var	found;
var	kn;


// hostname
var hostname=require("os").hostname().toLowerCase();
var dot=hostname.indexOf(".");
if (dot>-1) {
	hostname=hostname.substring(0,dot);
}


// instantiation
var	con=new sqlrelay.SQLRConnection("sqlrelay",9007,"/tmp/odbc-mssql.socket",
					"testuser","testpassword",0,1);
setConnection(con);
var	cur=new sqlrelay.SQLRCursor(con);
setCursor(cur);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"odbc");
console.log();


// ping
console.log("PING: ");
assertTrue(con.ping());
console.log();


// transaction state
console.log("TRANSACTION STATE: ");
assertEqStr(con.getDefaultTransactionModel(),"explicit");
assertEqStr(con.getTransactionModel(),"explicit");
assertFalse(con.getInTransaction());
assertTrue(con.getAutoCommit());
console.log();


// bind format
console.log("BIND FORMAT: ");
assertEqStr(con.bindFormat(),"?");
console.log();


// nextval format
console.log("NEXTVAL FORMAT: ");
assertEqStr(con.nextvalFormat(),"");
console.log();


// isolation levels
console.log("ISOLATION LEVELS: ");
// the odbc module has no getIsolationLevelQuery() override, so the
// readback always comes back "unknown" regardless of what was set
for (var i=0; i<isolationlevels.length; i++) {
	var il=isolationlevels[i];
	assertTrue(con.setIsolationLevel(il));
	assertEqStr(con.getIsolationLevel(),"unknown");
	console.log();
}
// reset to the default isolation level
assertTrue(con.setIsolationLevel(isolationlevels[0]));
console.log();


// create testtable
console.log("CREATE TESTTABLE: ");
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
	"	testdate date, "+
	"	testtime time, "+
	"	testdatetime2 datetime2)"));
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
	"	'01-Jan-2001', "+
	"	'13:01:01', "+
	"	'01-Jan-2001 13:01:01')"));
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
	"	?)");
assertEqInt(cur.countBindVariables(),17);
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
cur.inputBind("15","01-Jan-2001");
cur.inputBind("16","13:01:01");
cur.inputBind("17","01-Jan-2001 13:01:01");
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
cur.inputBind("15","01-Jan-2001");
cur.inputBind("16","13:01:01");
cur.inputBind("17","01-Jan-2001 13:01:01");
assertTrue(cur.executeQuery());
console.log();


// array of input binds by position
// arrays of binds do work here - odbc binds them all as strings and
// mssql converts - but the fixture is already 8 rows without them,
// so there is nothing left for this section to insert


// input bind by position with validation
console.log("INPUT BIND BY POSITION WITH VALIDATION: ");
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
cur.inputBind("15","01-Jan-2001");
cur.inputBind("16","13:01:01");
cur.inputBind("17","01-Jan-2001 13:01:01");
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log();


// input bind by name
// odbc binds positionally, with "?", so there is nothing to bind by
// name.  that is a contract rather than a defect: @varN gives "Must
// declare the scalar variable" and :varN gives "Incorrect syntax near
// ':'".  translatebindvariables=yes would rewrite the binds, but it
// also mangles every create procedure below, so it isn't usable here.
// the block below is parked rather than deleted, but note that it
// inserts fixture rows 5 through 7, which the REMAINING FIXTURE ROWS
// section below already inserts by position - switching this on means
// taking those out
/*
console.log("INPUT BIND BY NAME: ");
cur.clearBinds();
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
	"	@var17)");
assertEqInt(cur.countBindVariables(),17);
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
cur.inputBind("var15","01-Jan-2001");
cur.inputBind("var16","13:01:01");
cur.inputBind("var17","01-Jan-2001 13:01:01");
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
cur.inputBind("var15","01-Jan-2001");
cur.inputBind("var16","13:01:01");
cur.inputBind("var17","01-Jan-2001 13:01:01");
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
cur.inputBind("var15","01-Jan-2001");
cur.inputBind("var16","13:01:01");
cur.inputBind("var17","01-Jan-2001 13:01:01");
assertTrue(cur.executeQuery());
console.log();
*/


// array of input binds by name
// odbc binds positionally, so there is nothing to bind by name.


// input bind by name with validation
// odbc binds positionally, so there is nothing to bind by name.  this
// inserts fixture row 8, which REMAINING FIXTURE ROWS below already
// inserts by position
/*
console.log("INPUT BIND BY NAME WITH VALIDATION: ");
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
cur.inputBind("var15","01-Jan-2001");
cur.inputBind("var16","13:01:01");
cur.inputBind("var17","01-Jan-2001 13:01:01");
cur.inputBind("var18","junkvalue");
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log();
*/


// remaining fixture rows
// the freetds test puts rows 5 through 8 in by name.  they go in by
// position here instead, so the fixture is still 8 rows and every
// count and row index below carries over unchanged
console.log("REMAINING FIXTURE ROWS: ");
cur.clearBinds();
cur.inputBind("1",5);
cur.inputBind("2",5);
cur.inputBind("3",5);
cur.inputBind("4",5.5,2,1);
cur.inputBind("5",5.5,2,1);
cur.inputBind("6",5.5,2,1);
cur.inputBind("7",5.5,2,1);
cur.inputBind("8",5.00,3,2);
cur.inputBind("9",5.00,3,2);
cur.inputBind("10","01-Jan-2005 05:00:00");
cur.inputBind("11","01-Jan-2005 05:00:00");
cur.inputBind("12","testchar5");
cur.inputBind("13","testvarchar5");
cur.inputBind("14",1);
cur.inputBind("15","01-Jan-2001");
cur.inputBind("16","13:01:01");
cur.inputBind("17","01-Jan-2001 13:01:01");
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",6);
cur.inputBind("2",6);
cur.inputBind("3",6);
cur.inputBind("4",6.5,2,1);
cur.inputBind("5",6.5,2,1);
cur.inputBind("6",6.5,2,1);
cur.inputBind("7",6.5,2,1);
cur.inputBind("8",6.00,3,2);
cur.inputBind("9",6.00,3,2);
cur.inputBind("10","01-Jan-2006 06:00:00");
cur.inputBind("11","01-Jan-2006 06:00:00");
cur.inputBind("12","testchar6");
cur.inputBind("13","testvarchar6");
cur.inputBind("14",1);
cur.inputBind("15","01-Jan-2001");
cur.inputBind("16","13:01:01");
cur.inputBind("17","01-Jan-2001 13:01:01");
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",7);
cur.inputBind("2",7);
cur.inputBind("3",7);
cur.inputBind("4",7.5,2,1);
cur.inputBind("5",7.5,2,1);
cur.inputBind("6",7.5,2,1);
cur.inputBind("7",7.5,2,1);
cur.inputBind("8",7.00,3,2);
cur.inputBind("9",7.00,3,2);
cur.inputBind("10","01-Jan-2007 07:00:00");
cur.inputBind("11","01-Jan-2007 07:00:00");
cur.inputBind("12","testchar7");
cur.inputBind("13","testvarchar7");
cur.inputBind("14",1);
cur.inputBind("15","01-Jan-2001");
cur.inputBind("16","13:01:01");
cur.inputBind("17","01-Jan-2001 13:01:01");
assertTrue(cur.executeQuery());
console.log();
cur.clearBinds();
cur.inputBind("1",8);
cur.inputBind("2",8);
cur.inputBind("3",8);
cur.inputBind("4",8.5,2,1);
cur.inputBind("5",8.5,2,1);
cur.inputBind("6",8.5,2,1);
cur.inputBind("7",8.5,2,1);
cur.inputBind("8",8.00,3,2);
cur.inputBind("9",8.00,3,2);
cur.inputBind("10","01-Jan-2008 08:00:00");
cur.inputBind("11","01-Jan-2008 08:00:00");
cur.inputBind("12","testchar8");
cur.inputBind("13","testvarchar8");
cur.inputBind("14",1);
cur.inputBind("15","01-Jan-2001");
cur.inputBind("16","13:01:01");
cur.inputBind("17","01-Jan-2001 13:01:01");
assertTrue(cur.executeQuery());
console.log();


// select
console.log("SELECT: ");
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
console.log();


// column count
console.log("COLUMN COUNT: ");
assertEqInt(cur.colCount(),17);
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
assertEqStr(cur.getColumnName(14),"testdate");
assertEqStr(cur.getColumnName(15),"testtime");
assertEqStr(cur.getColumnName(16),"testdatetime2");
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
assertEqStr(cols[14],"testdate");
assertEqStr(cols[15],"testtime");
assertEqStr(cols[16],"testdatetime2");
console.log();


// column types
console.log("COLUMN TYPES: ");
assertEqStr(cur.getColumnType(0),"INTEGER");
assertEqStr(cur.getColumnType("testint"),"INTEGER");
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
assertEqStr(cur.getColumnType(14),"DATE");
assertEqStr(cur.getColumnType("testdate"),"DATE");
assertEqStr(cur.getColumnType(15),"TIME");
assertEqStr(cur.getColumnType("testtime"),"TIME");
assertEqStr(cur.getColumnType(16),"TIMESTAMP");
assertEqStr(cur.getColumnType("testdatetime2"),"TIMESTAMP");
console.log();


// column length
console.log("COLUMN LENGTH: ");
// odbc reports the ODBC column size - the number of characters it
// takes to display the value - where freetds reports the storage
// size in bytes, so every one of these differs from the freetds test
assertEqInt(cur.getColumnLength(0),10);
assertEqInt(cur.getColumnLength("testint"),10);
assertEqInt(cur.getColumnLength(1),5);
assertEqInt(cur.getColumnLength("testsmallint"),5);
assertEqInt(cur.getColumnLength(2),3);
assertEqInt(cur.getColumnLength("testtinyint"),3);
assertEqInt(cur.getColumnLength(3),24);
assertEqInt(cur.getColumnLength("testreal"),24);
assertEqInt(cur.getColumnLength(4),53);
assertEqInt(cur.getColumnLength("testfloat"),53);
assertEqInt(cur.getColumnLength(5),4);
assertEqInt(cur.getColumnLength("testdecimal"),4);
assertEqInt(cur.getColumnLength(6),4);
assertEqInt(cur.getColumnLength("testnumeric"),4);
assertEqInt(cur.getColumnLength(7),19);
assertEqInt(cur.getColumnLength("testmoney"),19);
assertEqInt(cur.getColumnLength(8),10);
assertEqInt(cur.getColumnLength("testsmallmoney"),10);
assertEqInt(cur.getColumnLength(9),23);
assertEqInt(cur.getColumnLength("testdatetime"),23);
assertEqInt(cur.getColumnLength(10),16);
assertEqInt(cur.getColumnLength("testsmalldatetime"),16);
// char(40)/varchar(40) report the declared length 40 (not multiplied)
assertEqInt(cur.getColumnLength(11),40);
assertEqInt(cur.getColumnLength("testchar"),40);
assertEqInt(cur.getColumnLength(12),40);
assertEqInt(cur.getColumnLength("testvarchar"),40);
assertEqInt(cur.getColumnLength(13),1);
assertEqInt(cur.getColumnLength("testbit"),1);
assertEqInt(cur.getColumnLength(14),10);
assertEqInt(cur.getColumnLength("testdate"),10);
assertEqInt(cur.getColumnLength(15),16);
assertEqInt(cur.getColumnLength("testtime"),16);
assertEqInt(cur.getColumnLength(16),27);
assertEqInt(cur.getColumnLength("testdatetime2"),27);
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
assertMoneyEqLen(cur.getLongest(7),6);
assertMoneyEqLen(cur.getLongest("testmoney"),6);
assertMoneyEqLen(cur.getLongest(8),6);
assertMoneyEqLen(cur.getLongest("testsmallmoney"),6);
assertEqInt(cur.getLongest(9),23);
assertEqInt(cur.getLongest("testdatetime"),23);
assertEqInt(cur.getLongest(10),19);
assertEqInt(cur.getLongest("testsmalldatetime"),19);
assertEqInt(cur.getLongest(11),40);
assertEqInt(cur.getLongest("testchar"),40);
assertEqInt(cur.getLongest(12),12);
assertEqInt(cur.getLongest("testvarchar"),12);
assertEqInt(cur.getLongest(13),1);
assertEqInt(cur.getLongest("testbit"),1);
assertEqInt(cur.getLongest(14),10);
assertEqInt(cur.getLongest("testdate"),10);
assertEqInt(cur.getLongest(15),16);
assertEqInt(cur.getLongest("testtime"),16);
assertEqInt(cur.getLongest(16),27);
assertEqInt(cur.getLongest("testdatetime2"),27);
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
assertMoneyEqStr(cur.getField(0,7),"1.0000");
assertMoneyEqStr(cur.getField(0,8),"1.0000");
assertEqStr(cur.getField(0,9),"2001-01-01 01:00:00.000");
assertEqStr(cur.getField(0,10),"2001-01-01 01:00:00");
assertEqStr(cur.getField(0,11),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,12),"testvarchar1");
assertEqStr(cur.getField(0,13),"1");
assertEqStr(cur.getField(0,14),"2001-01-01");
assertEqStr(cur.getField(0,15),"13:01:01.0000000");
assertEqStr(cur.getField(0,16),"2001-01-01 13:01:01.0000000");
console.log();
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(7,2),"8");
assertEqStr(cur.getField(7,3),"8.5");
assertEqStr(cur.getField(7,4),"8.5");
assertEqStr(cur.getField(7,5),"8.5");
assertEqStr(cur.getField(7,6),"8.5");
assertMoneyEqStr(cur.getField(7,7),"8.0000");
assertMoneyEqStr(cur.getField(7,8),"8.0000");
assertEqStr(cur.getField(7,9),"2008-01-01 08:00:00.000");
assertEqStr(cur.getField(7,10),"2008-01-01 08:00:00");
assertEqStr(cur.getField(7,11),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,12),"testvarchar8");
assertEqStr(cur.getField(7,13),"1");
assertEqStr(cur.getField(7,14),"2001-01-01");
assertEqStr(cur.getField(7,15),"13:01:01.0000000");
assertEqStr(cur.getField(7,16),"2001-01-01 13:01:01.0000000");
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
assertMoneyEqLen(cur.getFieldLength(0,7),6);
assertMoneyEqLen(cur.getFieldLength(0,8),6);
assertEqInt(cur.getFieldLength(0,9),23);
assertEqInt(cur.getFieldLength(0,10),19);
assertEqInt(cur.getFieldLength(0,11),40);
assertEqInt(cur.getFieldLength(0,12),12);
assertEqInt(cur.getFieldLength(0,13),1);
assertEqInt(cur.getFieldLength(0,14),10);
assertEqInt(cur.getFieldLength(0,15),16);
assertEqInt(cur.getFieldLength(0,16),27);
console.log();
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),1);
assertEqInt(cur.getFieldLength(7,2),1);
assertEqInt(cur.getFieldLength(7,3),3);
assertEqInt(cur.getFieldLength(7,4),3);
assertEqInt(cur.getFieldLength(7,5),3);
assertEqInt(cur.getFieldLength(7,6),3);
assertMoneyEqLen(cur.getFieldLength(7,7),6);
assertMoneyEqLen(cur.getFieldLength(7,8),6);
assertEqInt(cur.getFieldLength(7,9),23);
assertEqInt(cur.getFieldLength(7,10),19);
assertEqInt(cur.getFieldLength(7,11),40);
assertEqInt(cur.getFieldLength(7,12),12);
assertEqInt(cur.getFieldLength(7,13),1);
assertEqInt(cur.getFieldLength(7,14),10);
assertEqInt(cur.getFieldLength(7,15),16);
assertEqInt(cur.getFieldLength(7,16),27);
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
assertMoneyEqStr(cur.getField(0,"testmoney"),"1.0000");
assertMoneyEqStr(cur.getField(0,"testsmallmoney"),"1.0000");
assertEqStr(cur.getField(0,"testdatetime"),
	"2001-01-01 01:00:00.000");
assertEqStr(cur.getField(0,"testsmalldatetime"),
	"2001-01-01 01:00:00");
assertEqStr(cur.getField(0,"testchar"),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,"testvarchar"),"testvarchar1");
assertEqStr(cur.getField(0,"testbit"),"1");
assertEqStr(cur.getField(0,"testdate"),"2001-01-01");
assertEqStr(cur.getField(0,"testtime"),"13:01:01.0000000");
assertEqStr(cur.getField(0,"testdatetime2"),
	"2001-01-01 13:01:01.0000000");
console.log();
assertEqStr(cur.getField(7,"testint"),"8");
assertEqStr(cur.getField(7,"testsmallint"),"8");
assertEqStr(cur.getField(7,"testtinyint"),"8");
assertEqStr(cur.getField(7,"testreal"),"8.5");
assertEqStr(cur.getField(7,"testfloat"),"8.5");
assertEqStr(cur.getField(7,"testdecimal"),"8.5");
assertEqStr(cur.getField(7,"testnumeric"),"8.5");
assertMoneyEqStr(cur.getField(7,"testmoney"),"8.0000");
assertMoneyEqStr(cur.getField(7,"testsmallmoney"),"8.0000");
assertEqStr(cur.getField(7,"testdatetime"),
	"2008-01-01 08:00:00.000");
assertEqStr(cur.getField(7,"testsmalldatetime"),
	"2008-01-01 08:00:00");
assertEqStr(cur.getField(7,"testchar"),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,"testvarchar"),"testvarchar8");
assertEqStr(cur.getField(7,"testbit"),"1");
assertEqStr(cur.getField(7,"testdate"),"2001-01-01");
assertEqStr(cur.getField(7,"testtime"),"13:01:01.0000000");
assertEqStr(cur.getField(7,"testdatetime2"),
	"2001-01-01 13:01:01.0000000");
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
assertMoneyEqLen(cur.getFieldLength(0,"testmoney"),6);
assertMoneyEqLen(cur.getFieldLength(0,"testsmallmoney"),6);
assertEqInt(cur.getFieldLength(0,"testdatetime"),23);
assertEqInt(cur.getFieldLength(0,"testsmalldatetime"),
	19);
assertEqInt(cur.getFieldLength(0,"testchar"),40);
assertEqInt(cur.getFieldLength(0,"testvarchar"),12);
assertEqInt(cur.getFieldLength(0,"testbit"),1);
assertEqInt(cur.getFieldLength(0,"testdate"),10);
assertEqInt(cur.getFieldLength(0,"testtime"),16);
assertEqInt(cur.getFieldLength(0,"testdatetime2"),27);
console.log();
assertEqInt(cur.getFieldLength(7,"testint"),1);
assertEqInt(cur.getFieldLength(7,"testsmallint"),1);
assertEqInt(cur.getFieldLength(7,"testtinyint"),1);
assertEqInt(cur.getFieldLength(7,"testreal"),3);
assertEqInt(cur.getFieldLength(7,"testfloat"),3);
assertEqInt(cur.getFieldLength(7,"testdecimal"),3);
assertEqInt(cur.getFieldLength(7,"testnumeric"),3);
assertMoneyEqLen(cur.getFieldLength(7,"testmoney"),6);
assertMoneyEqLen(cur.getFieldLength(7,"testsmallmoney"),6);
assertEqInt(cur.getFieldLength(7,"testdatetime"),23);
assertEqInt(cur.getFieldLength(7,"testsmalldatetime"),
	19);
assertEqInt(cur.getFieldLength(7,"testchar"),40);
assertEqInt(cur.getFieldLength(7,"testvarchar"),12);
assertEqInt(cur.getFieldLength(7,"testbit"),1);
assertEqInt(cur.getFieldLength(7,"testdate"),10);
assertEqInt(cur.getFieldLength(7,"testtime"),16);
assertEqInt(cur.getFieldLength(7,"testdatetime2"),27);
console.log();


// fields by array
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqStr(fields[0],"1");
assertEqStr(fields[1],"1");
assertEqStr(fields[2],"1");
assertEqStr(fields[3],"1.5");
assertEqStr(fields[4],"1.5");
assertEqStr(fields[5],"1.5");
assertEqStr(fields[6],"1.5");
assertMoneyEqStr(fields[7],"1.0000");
assertMoneyEqStr(fields[8],"1.0000");
assertEqStr(fields[9],"2001-01-01 01:00:00.000");
assertEqStr(fields[10],"2001-01-01 01:00:00");
assertEqStr(fields[11],"testchar1"+
	"                               ");
assertEqStr(fields[12],"testvarchar1");
assertEqStr(fields[13],"1");
assertEqStr(fields[14],"2001-01-01");
assertEqStr(fields[15],"13:01:01.0000000");
assertEqStr(fields[16],"2001-01-01 13:01:01.0000000");
console.log();


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqInt(fieldlens[0],1);
assertEqInt(fieldlens[1],1);
assertEqInt(fieldlens[2],1);
assertEqInt(fieldlens[3],3);
assertEqInt(fieldlens[4],3);
assertEqInt(fieldlens[5],3);
assertEqInt(fieldlens[6],3);
assertMoneyEqLen(fieldlens[7],6);
assertMoneyEqLen(fieldlens[8],6);
assertEqInt(fieldlens[9],23);
assertEqInt(fieldlens[10],19);
assertEqInt(fieldlens[11],40);
assertEqInt(fieldlens[12],12);
assertEqInt(fieldlens[13],1);
assertEqInt(fieldlens[14],10);
assertEqInt(fieldlens[15],16);
assertEqInt(fieldlens[16],27);
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
assertEqInt(cur.getColumnLength(0),10);
assertEqStr(cur.getColumnType(0),"INTEGER");
console.log();


// suspended session
console.log("SUSPENDED SESSION: ");
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
id=cur.getResultSetId();
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
cur.cacheToFile("cachefile1-odbc-mssql");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-odbc-mssql");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
console.log();


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqInt(cur.colCount(),17);
console.log();


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
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
assertEqStr(cur.getColumnName(14),"testdate");
assertEqStr(cur.getColumnName(15),"testtime");
assertEqStr(cur.getColumnName(16),"testdatetime2");
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
assertEqStr(cols[14],"testdate");
assertEqStr(cols[15],"testtime");
assertEqStr(cols[16],"testdatetime2");
console.log();


// cached result set with result set
// buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-odbc-mssql");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-odbc-mssql");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// from one cache file to another
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2-odbc-mssql");
assertTrue(cur.openCachedResultSet("cachefile1-odbc-mssql"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-odbc-mssql"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
console.log();


// from one cache file to another with
// result set buffer size
console.log("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET "+
	"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile2-odbc-mssql");
assertTrue(cur.openCachedResultSet("cachefile1-odbc-mssql"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-odbc-mssql"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// cached result set with suspend and
// result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET "+
	"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-odbc-mssql");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(2,0),"3");
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-odbc-mssql");
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
console.log("FINISHED SUSPENDED SESSION: ");
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
// can't do this with odbc
//cur.setResultSetBufferSize(1);
assertTrue(cur.sendQuery("select * from testtable"));
var secondcur=new sqlrelay.SQLRCursor(con);
secondcur.setResultSetBufferSize(1);
for (var i=0; cur.getRow(i); i++) {
	assertTrue(secondcur.sendQuery(
		"select * from testtable"));
}
// the nested selects must not disturb the outer result set
assertEqInt(i,cur.rowCount());
secondcur.closeResultSet();
//cur.setResultSetBufferSize(0);
assertTrue(con.commit());
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// reset transaction state
console.log("RESET TRANSACTION STATE: ");
assertTrue(con.commit());
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(con.getAutoCommit());
console.log();


// transaction behavior - implicit
console.log("TRANSACTION BEHAVIOR - implicit: ");
// switching to the implicit model turns autocommit off, so a table
// created after the switch stays inside an uncommitted transaction,
// and mssql holds a schema lock on it that blocks secondcon's reads -
// a lock that readpast can't skip.  create it while autocommit is
// still on, then switch
assertTrue(cur.sendQuery(
	"create table testtable (col1 integer)"));
assertTrue(con.setTransactionModel("implicit"));
assertEqStr(con.getTransactionModel(),"implicit");
var	secondcon=new sqlrelay.SQLRConnection("sqlrelay",9007,"/tmp/odbc-mssql.socket",
	"testuser","testpassword",0,1);
setSecondConnection(secondcon);
var	secondcur=new sqlrelay.SQLRCursor(secondcon);
setSecondCursor(secondcur);
// session is in a transaction; insert is not visible until commit
assertTrue(con.getInTransaction());
assertFalse(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
// at read committed, a plain count(*) scan blocks on the writer's
// uncommitted row until the transaction ends, so the test would hang
// rather than fail.  readpast skips the locked row instead, which
// still counts only committed rows and so still catches a premature
// commit.  it does assume the writer's locks stay at row granularity;
// were they to escalate, committed rows would be skipped too and the
// counts would come back low
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"0");
// commit makes it visible, and implicitly starts a new transaction
assertTrue(con.commit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// rollback discards, and implicitly starts a new transaction
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
secondcur.closeResultSet();
// autocommit-off left a transaction open, and switching the
// transaction model doesn't end it here the way it does under
// freetds.  the drop below would then sit in that transaction,
// holding a schema lock that the next section's reader blocks on
// rather than fails on, so put autocommit back on first
assertTrue(con.autoCommitOn());
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// transaction behavior - explicit
console.log("TRANSACTION BEHAVIOR - explicit: ");
assertTrue(con.setTransactionModel("explicit"));
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(cur.sendQuery(
	"create table testtable (col1 integer)"));
// begin starts a new transaction; insert is not visible until commit
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"0");
// commit makes it visible; no new transaction is started
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback discards; no new transaction is started
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
secondcur.closeResultSet();
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
assertTrue(cur.sendQuery(
	"create table testtable (col1 integer)"));
// begin starts a transaction; commit makes it visible
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback discards
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// during a transaction started by begin(), autoCommitOn is a
// no-op: the autocommit setting takes effect after the user
// explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(con.autoCommitOn());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// explicit commit ends the tx; autocommit-on now takes effect
assertTrue(con.commit());
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"2");
// autocommit is on; subsequent inserts are visible immediately
assertTrue(cur.sendQuery("insert into testtable values (4)"));
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
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
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"4");
assertTrue(cur.sendQuery("insert into testtable values (6)"));
assertTrue(con.rollback());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
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
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"4");
assertTrue(con.commit());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"5");
secondcur.closeResultSet();
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// transaction behavior - explicit-error
console.log("TRANSACTION BEHAVIOR - explicit-error: ");
assertTrue(con.setTransactionModel("explicit-error"));
assertEqStr(con.getTransactionModel(),"explicit-error");
assertTrue(cur.sendQuery(
	"create table testtable (col1 integer)"));
// begin, insert, commit
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
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
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
secondcur.closeResultSet();
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// transaction behavior - none
console.log("TRANSACTION BEHAVIOR - none: ");
assertTrue(con.setTransactionModel("none"));
assertEqStr(con.getTransactionModel(),"none");
assertTrue(cur.sendQuery(
	"create table testtable (col1 integer)"));
// no transactions; everything is visible immediately
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
assertEqStr(secondcur.getField(0,0),"1");
// commit and rollback are no-ops
assertTrue(con.commit());
assertTrue(cur.sendQuery("insert into testtable values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery(
	"select count(*) from testtable with (readpast)"));
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
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(con.getAutoCommit());
console.log();


// individual substitutions
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),'$(var2)',$(var3)");
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
cur.prepareQuery("select $(var1),$(var2),$(var3)");
cur.substitutions(subvars,subvallongs);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"2");
assertEqStr(cur.getField(0,2),"3");
console.log();
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
cur.substitutions(subvars,subvalstrings);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"hi");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"bye");
console.log();
cur.prepareQuery("select $(var1),$(var2),$(var3)");
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
	"	?, "+
	"	?, "+
	"	?, "+
	"	?)");
cur.inputBindClob("1","",0);
cur.inputBindClob("2",null,0);
cur.inputBindBlob("3","",0);
cur.inputBindBlob("4",null,0);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
// the empty clob and the empty blob both come back as true zero-length
// fields here, where freetds gives the blob the single 0x00 byte its
// encoder emits.  both read as "" here because the nodejs api stops
// strings at the first null, so the lengths are asserted too.
assertEqStr(cur.getField(0,0),"");
assertEqInt(cur.getFieldLength(0,0),0);
assertEqStr(cur.getField(0,1),null);
assertEqStr(cur.getField(0,2),"");
assertEqInt(cur.getFieldLength(0,2),0);
assertEqStr(cur.getField(0,3),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// long lobs
console.log("LONG LOBS: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(
	"create table testtable ("+
	"	testclob text, "+
	"	testblob image)");
cur.prepareQuery("insert into testtable "+
	"values (?,?)");
largebuffer="C".repeat(LARGE_BUFFER_LENGTH);
cur.inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
cur.inputBindBlob("2",largebuffer,LARGE_BUFFER_LENGTH);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
assertEqInt(cur.getFieldLength(0,"testclob"),
	LARGE_BUFFER_LENGTH);
assertEqStr(cur.getField(0,"testclob"),largebuffer);
assertEqInt(cur.getFieldLength(0,"testblob"),
	LARGE_BUFFER_LENGTH);
assertEqStrLen(cur.getField(0,"testblob"),largebuffer,
	LARGE_BUFFER_LENGTH);
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// wide nchar column
// #9411 - SQLBindCol was binding the driver's UCS-2 output directly
// into the caller's UTF-8-sized buffer, truncating wide nvarchar
// columns to roughly half their length in unicode mode.  a 4000-char
// value still fits inside a half-truncated buffer sized against the
// default 32768 maxfieldsize, so this connects to a second instance
// whose maxfieldsize is reduced to 4096, where the truncation is
// reproducible at a practical column length
console.log("WIDE NCHAR COLUMN: ");
var	widenchcon=new sqlrelay.SQLRConnection("sqlrelay",9033,
	"/tmp/odbcmssqlmaxfieldsize.socket",
	"testuser","testpassword",0,1);
var	widenchcur=new sqlrelay.SQLRCursor(widenchcon);
widenchcur.sendQuery("drop table testtable");
assertTrue(widenchcur.sendQuery(
	"create table testtable (testnchar nvarchar(4000))"));
widencharbuffer="N".repeat(WIDE_NCHAR_LENGTH);
widenchcur.prepareQuery("insert into testtable values (?)");
widenchcur.inputBind("1",widencharbuffer,WIDE_NCHAR_LENGTH);
assertTrue(widenchcur.executeQuery());
assertTrue(widenchcur.sendQuery("select testnchar from testtable"));
assertEqInt(widenchcur.getFieldLength(0,"testnchar"),
	WIDE_NCHAR_LENGTH);
assertEqStr(widenchcur.getField(0,"testnchar"),widencharbuffer);
assertTrue(widenchcur.sendQuery("drop table testtable"));
console.log();


// output bind by position
// the odbc module needs a placeholder for each parameter in the
// query - "exec testproc" on its own counts 0 bind variables and
// fails to execute.  "{call testproc(?,?,?,?,?)}" works too
console.log("OUTPUT BIND BY POSITION: ");
cur.sendQuery("drop procedure testproc");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@out1 int output, "+
	"	@out2 varchar(20) output, "+
	"	@out3 float output, "+
	"	@out4 datetime output, "+
	"	@out5 varchar(20) output as "+
	"select @out1=1, "+
	"	@out2='hello', "+
	"	@out3=2.5, "+
	"	@out4='2001-02-03', "+
	"	@out5=null"));
cur.prepareQuery("exec testproc ?,?,?,?,?");
assertEqInt(cur.countBindVariables(),5);
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
assertEqInt(numvar,1);
assertEqStr(stringvar,"hello");
assertEqInt(cur.getOutputBindLength("2"),5);
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
var nullvar=cur.getOutputBindString("5");
assertEqStr(nullvar,null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// failed execute after output bind date
// ticket #9408 - an unbraced odbc call escape ("call testproc(...)")
// fails to execute.  reusing this cursor's date output bind
// (successfully populated by the execute above) across a
// prepareQuery/executeQuery pair that fails to execute, followed by
// another prepareQuery, used to double free a stale timezone pointer
// and abort the client
console.log("FAILED EXECUTE AFTER OUTPUT BIND DATE: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@out1 int output, "+
	"	@out2 varchar(20) output, "+
	"	@out3 float output, "+
	"	@out4 datetime output, "+
	"	@out5 varchar(20) output as "+
	"select @out1=1, "+
	"	@out2='hello', "+
	"	@out3=2.5, "+
	"	@out4='2001-02-03', "+
	"	@out5=null"));
cur.prepareQuery("call testproc(?,?,?,?,?)");
cur.defineOutputBindInteger("1");
cur.defineOutputBindString("2",20);
cur.defineOutputBindDouble("3");
cur.defineOutputBindDate("4");
cur.defineOutputBindString("5",20);
assertFalse(cur.executeQuery());
cur.prepareQuery("select 1");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log();


// output bind by name
// odbc binds positionally, so there is nothing to bind by name


// output bind by name with validation
// odbc binds positionally, so there is nothing to bind by name.
// even if there were, validateBinds() can't be used for output binds
// here.  When executing a procedure you don't declare any bind
// variable delimiters in the query.  eg, you just do:
// "exec testproc", not "exec testproc(@out1,@out2)".  If you
// call validateBinds(), it won't find any binds in the query, and
// will filter out any binds that you declare.


// lob output bind
// the deprecated text, ntext and image types can't be output
// parameters, and there's no way to directly select into a lob
// bind variable


// long output bind
console.log("LONG OUTPUT BIND: ");
cur.sendQuery("drop procedure testproc");
longoutputbindbuffer="C".repeat(LONG_OUTPUT_BIND_LENGTH);
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"@bindval varchar("+LONG_OUTPUT_BIND_LENGTH+") output as "+
	"set @bindval='"+longoutputbindbuffer+"'"));
cur.prepareQuery("exec testproc ?");
cur.defineOutputBindString("1",LONG_OUTPUT_BIND_LENGTH);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindLength("1"),LONG_OUTPUT_BIND_LENGTH);
assertEqStr(cur.getOutputBindString("1"),longoutputbindbuffer);
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery("create table testtable "+
	"(testval int)");
cur.prepareQuery("insert into testtable "+
	"values (?)");
cur.inputBind("1",-1);
assertTrue(cur.executeQuery());
cur.sendQuery("select testval from testtable");
assertEqStr(cur.getField(0,"testval"),"-1");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// bind validation
// odbc binds positionally, and validateBinds() skips bind-by-position
// variables, so there is nothing to validate
/*
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
*/


// rebinding
console.log("REBINDING: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@in1 int, "+
	"	@out1 int output as "+
	"select @out1=@in1"));
cur.prepareQuery("exec testproc ?,?");
cur.inputBind("1",1);
cur.defineOutputBindInteger("2");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("2"),1);
cur.inputBind("1",2);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("2"),2);
cur.inputBind("1",3);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("2"),3);
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
cur.prepareQuery("select cast(? as int)");
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
console.log("STORED PROCEDURE RETURNING NO VALUE: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@in1 int, "+
	"	@in2 float, "+
	"	@in3 varchar(20) as "+
	"return"));
cur.prepareQuery("exec testproc ?,?,?");
cur.inputBind("1",1);
cur.inputBind("2",2.5,2,1);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// stored procedure returning single
// value
console.log("STORED PROCEDURE RETURNING SINGLE VALUE: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc "+
	"	@in1 int, "+
	"	@in2 float, "+
	"	@in3 varchar(20), "+
	"	@out1 int output as "+
	"select @out1=@in1"));
cur.prepareQuery("exec testproc ?,?,?,?");
cur.inputBind("1",1);
cur.inputBind("2",2.5,2,1);
cur.inputBind("3","hello");
cur.defineOutputBindInteger("4");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("4"),1);
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// stored procedure returning multiple
// values
console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc @in1 int, "+
	"       @in2 float, "+
	"       @in3 varchar(20), "+
	"       @out1 int output, "+
	"       @out2 float output, "+
	"       @out3 varchar(20) output as "+
	"select @out1=@in1, "+
	"       @out2=@in2, "+
	"       @out3=@in3"));
cur.prepareQuery("exec testproc ?,?,?,?,?,?");
cur.inputBind("1",1);
cur.inputBind("2",2.5,2,1);
cur.inputBind("3","hello");
cur.defineOutputBindInteger("4");
cur.defineOutputBindDouble("5");
cur.defineOutputBindString("6",20);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("4"),1);
assertEqDbl(cur.getOutputBindDouble("5"),2.5);
assertEqStr(cur.getOutputBindString("6"),"hello");
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log();


// stored procedure returning result set
console.log("STORED PROCEDURE RETURNING RESULT SET: ");
cur.sendQuery("drop procedure testselectproc");
assertTrue(cur.sendQuery("create procedure testselectproc as "+
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
	"       select 8"));
assertTrue(cur.sendQuery("exec testselectproc"));
assertEqInt(cur.rowCount(),8);
assertTrue(cur.sendQuery("drop procedure testselectproc"));
console.log();


// temporary tables
console.log("TEMPORARY TABLES: ");
cur.sendQuery("drop table #temptable");
cur.sendQuery("create table #temptable "+
	"(col1 int)");
assertTrue(cur.sendQuery("insert into #temptable "+
	"values (1)"));
assertTrue(cur.sendQuery("select count(*) "+
	"from #temptable"));
assertEqStr(cur.getField(0,0),"1");
con.endSession();
console.log();
assertFalse(cur.sendQuery("select count(*) "+
	"from #temptable"));
console.log();


// encoded binary data
console.log("ENCODED BINARY DATA: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable "+
	"(col1 image)"));
buffer="";
for (var i=0; i<256; i++) {
	buffer+=String.fromCharCode(i);
}
query="insert into testtable values (0x";
for (var i=0; i<buffer.length; i++) {
	query+=("0"+buffer.charCodeAt(i).toString(16)).slice(-2);
}
query+=")";
assertTrue(cur.sendQuery(query));
assertTrue(cur.sendQuery("select col1 from testtable"));
// check the round trip by length only.  the nodejs binding builds strings
// with String::NewFromUtf8, which drops the invalid utf-8 sequences that
// raw bytes 128-255 produce, so the returned string can't be byte-compared
assertEqInt(cur.getFieldLength(0,0),buffer.length);
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// quotes
console.log("QUOTES: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable "+
	"(col1 varchar(4))"));
assertTrue(cur.sendQuery("insert into testtable "+
	"values ('''''')"));
assertTrue(cur.sendQuery("select col1 from testtable"));
assertEqInt(cur.getFieldLength(0,0),2);
assertEqInt(cur.getField(0,0)==="''"?0:1,0);
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
assertTrue(cur.getSchemaList(null));
assertEqStr(cur.getColumnName(0),"Database");
// odbc lists INFORMATION_SCHEMA, sys and testuser - the schemas that
// own an object - rather than every schema, so dbo isn't there
assertInResultSet(cur,"Database","testuser");
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
// the odbc module maps odbc type names, not sql server ones, so "int"
// and "datetime" both fail - INTEGER and TIMESTAMP are the names to
// ask for.  the names it returns are the sql server ones, lowercased
assertTrue(cur.getTypeInfoList("INTEGER"));
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
assertEqStr(cur.getField(0,"type_name"),"int");
assertEqStr(cur.getField(0,"data_type"),"4");
assertEqStr(cur.getField(0,"precision"),"10");
assertEqStr(cur.getField(0,"local_type_name"),"int");
assertTrue(cur.getTypeInfoList("CHAR"));
assertEqStr(cur.getField(0,"type_name"),"char");
assertEqStr(cur.getField(0,"data_type"),"1");
assertEqStr(cur.getField(0,"precision"),"8000");
assertEqStr(cur.getField(0,"local_type_name"),"char");
assertTrue(cur.getTypeInfoList("VARCHAR"));
assertEqStr(cur.getField(0,"type_name"),"varchar");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"8000");
assertEqStr(cur.getField(0,"local_type_name"),"varchar");
// TIMESTAMP comes back as three rows - datetime2, datetime and
// smalldatetime, in that order - so datetime has to be searched for
// rather than read out of row 0
assertTrue(cur.getTypeInfoList("TIMESTAMP"));
assertInResultSet(cur,"type_name","datetime");
assertInResultSet(cur,"type_name","datetime2");
assertInResultSet(cur,"type_name","smalldatetime");
assertEqStr(cur.getField(1,"type_name"),"datetime");
assertEqStr(cur.getField(1,"data_type"),"93");
assertEqStr(cur.getField(1,"precision"),"23");
assertEqStr(cur.getField(1,"local_type_name"),"datetime");
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
	"	testdate date, "+
	"	testtime time, "+
	"	testdatetime2 datetime2)"));
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
assertTrue(cur.getField(14,"column_name")==="testdate");
assertTrue(cur.getField(15,"column_name")==="testtime");
assertTrue(cur.getField(16,"column_name")==="testdatetime2");
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
assertTrue(cur.getField(14,"data_type")==="date");
assertTrue(cur.getField(15,"data_type")==="time");
assertTrue(cur.getField(16,"data_type")==="datetime2");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// column list - auto_increment,
// primary key
console.log("COLUMN LIST - auto_increment, primary key: ");
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
	"	col1 int primary key, "+
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
	"	col1 int primary key, "+
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
// mssql auto-names an unnamed primary key constraint
// PK__<table name, truncated to 8 chars>__<hex>, and the hex is
// generated per creation, so only the prefix is stable
kn=cur.getField(0,"key_name");
assertStartsWith(kn,"PK__testtabl__");
assertTrue(cur.sendQuery("drop table testtable"));
console.log();


// key and index list
console.log("KEY AND INDEX LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
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
// the odbc module emits a leading SQL_TABLE_STAT row - table
// statistics rather than an index - so the index itself is row 1
assertEqInt(cur.rowCount(),2);
assertTrue(cur.getField(0,"table")==="testtable");
assertEqStr(cur.getField(0,"key_name"),"");
assertEqStr(cur.getField(0,"cardinality"),"0");
assertEqStr(cur.getField(0,"index_type"),"0");
assertTrue(cur.getField(1,"table")==="testtable");
assertEqStr(cur.getField(1,"non_unique"),"0");
assertEqStr(cur.getField(1,"seq_in_index"),"1");
assertTrue(cur.getField(1,"column_name")==="col1");
assertEqStr(cur.getField(1,"collation"),"A");
assertEqStr(cur.getField(1,"index_type"),"1");
// mssql auto-names an unnamed primary key constraint
// PK__<table name, truncated to 8 chars>__<hex>, and the hex is
// generated per creation, so only the prefix is stable
kn=cur.getField(1,"key_name");
assertStartsWith(kn,"PK__testtabl__");
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
	"	@in4 datetime "+
	"as select 1"));
assertTrue(cur.sendQuery(
	"create procedure testproc2 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+
	"as select 1"));
assertTrue(cur.sendQuery(
	"create procedure testproc3 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+
	"as select 1"));
assertTrue(cur.sendQuery(
	"create procedure testproc4 "+
	"	@in1 int, "+
	"	@in2 char(20), "+
	"	@in3 varchar(20), "+
	"	@in4 datetime "+
	"as select 1"));
assertTrue(cur.getProcedureList(null));
// odbc reports the procedure group number too - mssql lets several
// procedures share a name, distinguished by the number after the
// semicolon, and an ungrouped procedure is number 1
assertInResultSet(cur,"routine_name","testproc1;1");
assertInResultSet(cur,"routine_name","testproc2;1");
assertInResultSet(cur,"routine_name","testproc3;1");
assertInResultSet(cur,"routine_name","testproc4;1");
console.log();


// procedure parameter list
console.log("PROCEDURE PARAMETER LIST: ");
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
