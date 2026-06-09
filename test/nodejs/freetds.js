// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{
	setConnection, setCursor,
	setSecondConnection, setSecondCursor,
	assertEqStr, assertEqStrLen,
	assertEqInt, assertEqDbl, assertEqual,
	assertTrue, assertFalse,
	assertInResultSet,
	getStatus, reportTestStatus
}=require("./asserts.js");


var	isolationlevels=["1","0","2","3"];
var	subvars=["var1","var2","var3"];
var	subvallongs=[1,2,3];
var	subvalstrings=["hi","hello","bye"];
var	subvaldoubles=[10.55,10.556,10.5556];
var	precs=[4,5,6];
var	scales=[2,3,4];
var	counter=0;

var	LARGE_BUFFER_LENGTH=8192;

var	cols;
var	fields;
var	fieldlens;
var	port;
var	socket;
var	id;
var	filename;
var	fld;
var	nul;
var	largebuffer;
var	buffer;
var	query;
var	name;
var	found;
var	kn;


// hostname
var hostname=require("os").hostname();
var dot=hostname.indexOf(".");
if (dot>-1) {
	hostname=hostname.substring(0,dot);
}
var dumptran="dump tran "+hostname+" with truncate_only";


// instantiation
var	con=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket",
					"testuser","testpassword",0,1);
setConnection(con);
var	cur=new sqlrelay.SQLRCursor(con);
setCursor(cur);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"freetds");
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
	var il=isolationlevels[i];
	assertTrue(con.setIsolationLevel(il));
	assertEqStr(con.getIsolationLevel(),il);
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
	"	testbit bit) lock datarows"));
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
	"	1)"));
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
	"	?)");
assertEqInt(cur.countBindVariables(),14);
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
assertTrue(cur.executeQuery());
console.log();


// array of input binds by position
// freetds doesn't support implicit
// conversion of string binds to other
// data types, so arrays of binds don't
// generally work.


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
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log();


// input bind by name
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
	"	@var14)");
assertEqInt(cur.countBindVariables(),14);
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
assertTrue(cur.executeQuery());
console.log();


// array of input binds by name
// freetds doesn't support implicit
// conversion of string binds to other
// data types, so arrays of binds don't
// generally work.


// input bind by name with validation
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
cur.inputBind("var15","junkvalue");
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
assertEqInt(cur.colCount(),14);
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
assertEqStr(cur.getColumnType(12),"CHAR");
assertEqStr(cur.getColumnType("testvarchar"),"CHAR");
assertEqStr(cur.getColumnType(13),"BIT");
assertEqStr(cur.getColumnType("testbit"),"BIT");
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
// these seem to fluctuate with every
// freetds release
//assertEqInt(cur.getColumnLength(5),3);
//assertEqInt(cur.getColumnLength("testdecimal"),3);
//assertEqInt(cur.getColumnLength(6),3);
//assertEqInt(cur.getColumnLength("testnumeric"),3);
assertEqInt(cur.getColumnLength(7),8);
assertEqInt(cur.getColumnLength("testmoney"),8);
assertEqInt(cur.getColumnLength(8),4);
assertEqInt(cur.getColumnLength("testsmallmoney"),4);
assertEqInt(cur.getColumnLength(9),8);
assertEqInt(cur.getColumnLength("testdatetime"),8);
assertEqInt(cur.getColumnLength(10),4);
assertEqInt(cur.getColumnLength("testsmalldatetime"),4);
// these seem to fluctuate too
//assertEqInt(cur.getColumnLength(11),40);
//assertEqInt(cur.getColumnLength("testchar"),40);
//assertEqInt(cur.getColumnLength(12),40);
//assertEqInt(cur.getColumnLength("testvarchar"),40);
assertEqInt(cur.getColumnLength(13),1);
assertEqInt(cur.getColumnLength("testbit"),1);
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
assertEqInt(cur.getLongest(7),6);
assertEqInt(cur.getLongest("testmoney"),6);
assertEqInt(cur.getLongest(8),6);
assertEqInt(cur.getLongest("testsmallmoney"),6);
// datetime formatting fluctuates with
// every freetds release
//assertEqInt(cur.getLongest(9),26);
//assertEqInt(cur.getLongest("testdatetime"),26);
//assertEqInt(cur.getLongest(10),26);
//assertEqInt(cur.getLongest("testsmalldatetime"),26);
assertEqInt(cur.getLongest(11),40);
assertEqInt(cur.getLongest("testchar"),40);
assertEqInt(cur.getLongest(12),12);
assertEqInt(cur.getLongest("testvarchar"),12);
assertEqInt(cur.getLongest(13),1);
assertEqInt(cur.getLongest("testbit"),1);
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
assertEqStr(cur.getField(0,7),"1.0000");
assertEqStr(cur.getField(0,8),"1.0000");
// datetime formatting fluctuates with
// every freetds release
//assertEqStr(cur.getField(0,9),
//	"Jan  1 2001 01:00:00:000AM");
//assertEqStr(cur.getField(0,10),
//	"Jan  1 2001 01:00:00:000AM");
assertEqStr(cur.getField(0,11),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,12),"testvarchar1");
assertEqStr(cur.getField(0,13),"1");
console.log();
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(7,2),"8");
assertEqStr(cur.getField(7,3),"8.5");
assertEqStr(cur.getField(7,4),"8.5");
assertEqStr(cur.getField(7,5),"8.5");
assertEqStr(cur.getField(7,6),"8.5");
assertEqStr(cur.getField(7,7),"8.0000");
assertEqStr(cur.getField(7,8),"8.0000");
// datetime formatting fluctuates with
// every freetds release
//assertEqStr(cur.getField(7,9),
//	"Jan  1 2008 08:00:00:000AM");
//assertEqStr(cur.getField(7,10),
//	"Jan  1 2008 08:00:00:000AM");
assertEqStr(cur.getField(7,11),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,12),"testvarchar8");
assertEqStr(cur.getField(7,13),"1");
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
assertEqInt(cur.getFieldLength(0,7),6);
assertEqInt(cur.getFieldLength(0,8),6);
// datetime formatting fluctuates with
// every freetds release
//assertEqInt(cur.getFieldLength(0,9),26);
//assertEqInt(cur.getFieldLength(0,10),26);
assertEqInt(cur.getFieldLength(0,11),40);
assertEqInt(cur.getFieldLength(0,12),12);
assertEqInt(cur.getFieldLength(0,13),1);
console.log();
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),1);
assertEqInt(cur.getFieldLength(7,2),1);
assertEqInt(cur.getFieldLength(7,3),3);
assertEqInt(cur.getFieldLength(7,4),3);
assertEqInt(cur.getFieldLength(7,5),3);
assertEqInt(cur.getFieldLength(7,6),3);
assertEqInt(cur.getFieldLength(7,7),6);
assertEqInt(cur.getFieldLength(7,8),6);
// datetime formatting fluctuates with
// every freetds release
//assertEqInt(cur.getFieldLength(7,9),26);
//assertEqInt(cur.getFieldLength(7,10),26);
assertEqInt(cur.getFieldLength(7,11),40);
assertEqInt(cur.getFieldLength(7,12),12);
assertEqInt(cur.getFieldLength(7,13),1);
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
assertEqStr(cur.getField(0,"testmoney"),"1.0000");
assertEqStr(cur.getField(0,"testsmallmoney"),"1.0000");
// datetime formatting fluctuates with
// every freetds release
//assertEqStr(cur.getField(0,"testdatetime"),
//	"Jan  1 2001 01:00:00:000AM");
//assertEqStr(cur.getField(0,"testsmalldatetime"),
//	"Jan  1 2001 01:00:00:000AM");
assertEqStr(cur.getField(0,"testchar"),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,"testvarchar"),"testvarchar1");
assertEqStr(cur.getField(0,"testbit"),"1");
console.log();
assertEqStr(cur.getField(7,"testint"),"8");
assertEqStr(cur.getField(7,"testsmallint"),"8");
assertEqStr(cur.getField(7,"testtinyint"),"8");
assertEqStr(cur.getField(7,"testreal"),"8.5");
assertEqStr(cur.getField(7,"testfloat"),"8.5");
assertEqStr(cur.getField(7,"testdecimal"),"8.5");
assertEqStr(cur.getField(7,"testnumeric"),"8.5");
assertEqStr(cur.getField(7,"testmoney"),"8.0000");
assertEqStr(cur.getField(7,"testsmallmoney"),"8.0000");
// datetime formatting fluctuates with
// every freetds release
//assertEqStr(cur.getField(7,"testdatetime"),
//	"Jan  1 2008 08:00:00:000AM");
//assertEqStr(cur.getField(7,"testsmalldatetime"),
//	"Jan  1 2008 08:00:00:000AM");
assertEqStr(cur.getField(7,"testchar"),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,"testvarchar"),"testvarchar8");
assertEqStr(cur.getField(7,"testbit"),"1");
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
assertEqInt(cur.getFieldLength(0,"testmoney"),6);
assertEqInt(cur.getFieldLength(0,"testsmallmoney"),6);
// datetime formatting fluctuates with
// every freetds release
//assertEqInt(cur.getFieldLength(0,"testdatetime"),26);
//assertEqInt(cur.getFieldLength(0,"testsmalldatetime"),
//	26);
assertEqInt(cur.getFieldLength(0,"testchar"),40);
assertEqInt(cur.getFieldLength(0,"testvarchar"),12);
assertEqInt(cur.getFieldLength(0,"testbit"),1);
console.log();
assertEqInt(cur.getFieldLength(7,"testint"),1);
assertEqInt(cur.getFieldLength(7,"testsmallint"),1);
assertEqInt(cur.getFieldLength(7,"testtinyint"),1);
assertEqInt(cur.getFieldLength(7,"testreal"),3);
assertEqInt(cur.getFieldLength(7,"testfloat"),3);
assertEqInt(cur.getFieldLength(7,"testdecimal"),3);
assertEqInt(cur.getFieldLength(7,"testnumeric"),3);
assertEqInt(cur.getFieldLength(7,"testmoney"),6);
assertEqInt(cur.getFieldLength(7,"testsmallmoney"),6);
// datetime formatting fluctuates with
// every freetds release
//assertEqInt(cur.getFieldLength(7,"testdatetime"),26);
//assertEqInt(cur.getFieldLength(7,"testsmalldatetime"),
//	26);
assertEqInt(cur.getFieldLength(7,"testchar"),40);
assertEqInt(cur.getFieldLength(7,"testvarchar"),12);
assertEqInt(cur.getFieldLength(7,"testbit"),1);
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
assertEqStr(fields[7],"1.0000");
assertEqStr(fields[8],"1.0000");
// datetime formatting fluctuates with
// every freetds release
//assertEqStr(fields[9],"Jan  1 2001 01:00:00:000AM");
//assertEqStr(fields[10],"Jan  1 2001 01:00:00:000AM");
assertEqStr(fields[11],"testchar1"+
	"                               ");
assertEqStr(fields[12],"testvarchar1");
assertEqStr(fields[13],"1");
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
assertEqInt(fieldlens[7],6);
assertEqInt(fieldlens[8],6);
// datetime formatting fluctuates with
// every freetds release
//assertEqInt(fieldlens[9],26);
//assertEqInt(fieldlens[10],26);
assertEqInt(fieldlens[11],40);
assertEqInt(fieldlens[12],12);
assertEqInt(fieldlens[13],1);
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
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
console.log();


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqInt(cur.colCount(),14);
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
console.log();


// cached result set with result set
// buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// from one cache file to another
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2");
assertTrue(cur.openCachedResultSet("cachefile1"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
console.log();


// from one cache file to another with
// result set buffer size
console.log("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET "+
	"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile2");
assertTrue(cur.openCachedResultSet("cachefile1"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// cached result set with suspend and
// result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET "+
	"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(2,0),"3");
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
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
// can't do this with freetds
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
var	secondcon=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket",
	"testuser","testpassword",0,1);
setSecondConnection(secondcon);
var	secondcur=new sqlrelay.SQLRCursor(secondcon);
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
// sap converts empty strings to a single
// space.  It's possible that if we had
// true input bind support on the backend,
// then this would work correctly, but for
// now we're faking binds, and inserting an
// empty string, so we have to check for a
// single space here.
assertEqStr(cur.getField(0,0)," ");
assertEqStr(cur.getField(0,1),null);
// sap doesn't really support inserting
// an empty string into a binary column.
// The minimum that can be inserted is a
// single \0.  The C/C++ tests compare
// with strcmp which treats the leading
// \0 as an empty string; truncate at the
// first \0 before compare.
fld=cur.getField(0,2);
if (fld===false || fld===null) {
	fld="";
} else {
	nul=fld.indexOf("\0");
	if (nul!==-1) {
		fld=fld.substring(0,nul);
	}
}
assertEqStr(fld,"");
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
	"	testblob image) "+
	"lock datarows");
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


// output bind by position
// FreeTDS needs to support cursors
// for this to work


// output bind by name
// FreeTDS needs to support cursors
// for this to work


// output bind by name with validation
// Even if FreeTDS supported cursors...
// validateBinds() can't be used for
// output binds, with sap.  In sap, when
// executing a procedure, you don't
// declare any bind variable delimiters
// in the query.  eg, you just do:
// "exec testproc", not
// "exec testproc(@out1,@out2)".  If you
// call validateBinds(), it won't find
// any binds in the query, and will filter
// out any binds that you declare.


// lob output bind
// sap doesn't support lobs as output
// parameters to stored procedures, and
// there's no way to directly select into
// a lob bind variable


// long output bind
// FreeTDS needs to support cursors
// for this to work


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery("create table testtable "+
	"(testval int)");
cur.prepareQuery("insert into testtable "+
	"values (@testval)");
cur.inputBind("testval",-1);
assertTrue(cur.executeQuery());
cur.sendQuery("select testval from testtable");
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
// FreeTDS needs to support cursors
// for this to work


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
// FreeTDS needs to support cursors
// for this to work


// stored procedure returning single
// value
// FreeTDS needs to support cursors
// for this to work


// stored procedure returning multiple
// values
// FreeTDS needs to support cursors
// for this to work


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
// Verify the raw bytes round-tripped via length only. (Sybase ASE doesn't
// allow convert(varchar,image,2) for hex, and the Node.js sqlrelay binding
// returns strings via String::NewFromUtf8 which drops invalid UTF-8
// sequences that arise from raw bytes 128-255, so we can't byte-compare
// the returned string directly.)
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
cur.sendQuery("drop table testtable");
// the get schema list query that is used
// with sap will only return the names of
// schemas that have at least one database
// object in them, so to be sure that
// there is one, we'll create a table
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
assertEqStr(cur.getField(0,"precision"),"8000");
assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
assertTrue(cur.getTypeInfoList("varchar"));
assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"8000");
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
	"	testbit bit)"));
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
kn=cur.getField(0,"key_name");
assertTrue(!(!kn || !kn[0]));
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
assertEqInt(cur.rowCount(),1);
assertTrue(cur.getField(0,"table")==="testtable");
assertEqStr(cur.getField(0,"non_unique"),"0");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")==="col1");
assertEqStr(cur.getField(0,"collation"),"A");
assertEqStr(cur.getField(0,"index_type"),"1");
kn=cur.getField(0,"key_name");
assertTrue(!(!kn || !kn[0]));
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
assertInResultSet(cur,"routine_name","testproc1");
assertInResultSet(cur,"routine_name","testproc2");
assertInResultSet(cur,"routine_name","testproc3");
assertInResultSet(cur,"routine_name","testproc4");
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
