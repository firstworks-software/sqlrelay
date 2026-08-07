// Copyright (c) David Muse
// See the file COPYING for more information.

var sqlrelay=require("sqlrelay");
var {
    setConnection, setCursor,
    setSecondConnection, setSecondCursor,
    assertEqStr, assertEqStrLen,
    assertEqInt, assertEqDbl, assertEqual,
    assertTrue, assertFalse,
    assertInResultSet,
    getStatus, reportTestStatus
}=require("./asserts.js");


var isolationlevels=["committed read","dirty read",
			"cursor stability","repeatable read"];
var bindvars=["1","2","3","4",
			"5","6","7","8","9","10",
			"11","12","13","14","15","16"];
var bindvals=["t","7","7","7","7",
			"7.5","7.5","7.5","7.5",
			"testchar7","testnchar7",
			"testvarchar7","testnvarchar7",
			"testlvarchar7","01/01/2007",
			"2007-01-01 07:00:00"];
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


// instantiation
var con=new sqlrelay.SQLRConnection("sqlrelay",9010,"/tmp/informixtest.socket",
					"testuser","testpassword",0,1);
setConnection(con);
var cur=new sqlrelay.SQLRCursor(con);
setCursor(cur);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"informix");
console.log("");


// ping
console.log("PING: ");
assertTrue(con.ping());
console.log("");


// transaction state
console.log("TRANSACTION STATE: ");
assertEqStr(con.getDefaultTransactionModel(),"implicit");
assertEqStr(con.getTransactionModel(),"implicit");
assertTrue(con.getInTransaction());
assertFalse(con.getAutoCommit());
console.log("");


// bind format
console.log("BIND FORMAT: ");
assertEqStr(con.bindFormat(),"?");
console.log("");


// nextval format
console.log("NEXTVAL FORMAT: ");
assertEqStr(con.nextvalFormat(),"%s.nextval");
console.log("");


// isolation levels
console.log("ISOLATION LEVELS: ");
for (var i=0;i<isolationlevels.length;i++) {
	var il=isolationlevels[i];
	// you can set the isolation level, but to get it, you have to
	// have permissions to read from sysmaster:syssqlcurses
	assertTrue(con.setIsolationLevel(il));
	console.log("");
}
// reset to the default isolation level
assertTrue(con.setIsolationLevel(isolationlevels[0]));
console.log("");


// create testtable
console.log("CREATE TESTTABLE: ");
cur.sendQuery("drop table testtable");
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
	"	testbyte byte)"));
assertTrue(con.commit());
console.log("");


// insert
console.log("INSERT: ");
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
	"	null)"));
console.log("");


// affected rows
console.log("AFFECTED ROWS: ");
assertEqInt(cur.affectedRows(),1);
console.log("");


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
	"	?, "+
	"	?)");
assertEqInt(cur.countBindVariables(),18);
cur.inputBind("1","t");
cur.inputBind("2",2);
cur.inputBind("3",2);
cur.inputBind("4",2);
cur.inputBind("5",2);
cur.inputBind("6",2.5,4,2);
cur.inputBind("7",2.5,4,2);
cur.inputBind("8",2.5,4,2);
cur.inputBind("9",2.5,4,2);
cur.inputBind("10","testchar2");
cur.inputBind("11","testnchar2");
cur.inputBind("12","testvarchar2");
cur.inputBind("13","testnvarchar2");
cur.inputBind("14","testlvarchar2");
cur.inputBind("15",2002,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("16",2002,1,1,2,0,0,0,null,0);
cur.inputBindClob("17","testtext2","testtext2".length);
cur.inputBindBlob("18","testbyte2","testbyte2".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1","t");
cur.inputBind("2",3);
cur.inputBind("3",3);
cur.inputBind("4",3);
cur.inputBind("5",3);
cur.inputBind("6",3.5,4,2);
cur.inputBind("7",3.5,4,2);
cur.inputBind("8",3.5,4,2);
cur.inputBind("9",3.5,4,2);
cur.inputBind("10","testchar3");
cur.inputBind("11","testnchar3");
cur.inputBind("12","testvarchar3");
cur.inputBind("13","testnvarchar3");
cur.inputBind("14","testlvarchar3");
cur.inputBind("15",2003,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("16",2003,1,1,3,0,0,0,null,0);
cur.inputBindClob("17","testtext3","testtext3".length);
cur.inputBindBlob("18","testbyte3","testbyte3".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1","t");
cur.inputBind("2",4);
cur.inputBind("3",4);
cur.inputBind("4",4);
cur.inputBind("5",4);
cur.inputBind("6",4.5,4,2);
cur.inputBind("7",4.5,4,2);
cur.inputBind("8",4.5,4,2);
cur.inputBind("9",4.5,4,2);
cur.inputBind("10","testchar4");
cur.inputBind("11","testnchar4");
cur.inputBind("12","testvarchar4");
cur.inputBind("13","testnvarchar4");
cur.inputBind("14","testlvarchar4");
cur.inputBind("15",2004,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("16",2004,1,1,4,0,0,0,null,0);
cur.inputBindClob("17","testtext4","testtext4".length);
cur.inputBindBlob("18","testbyte4","testbyte4".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1","t");
cur.inputBind("2",5);
cur.inputBind("3",5);
cur.inputBind("4",5);
cur.inputBind("5",5);
cur.inputBind("6",5.5,4,2);
cur.inputBind("7",5.5,4,2);
cur.inputBind("8",5.5,4,2);
cur.inputBind("9",5.5,4,2);
cur.inputBind("10","testchar5");
cur.inputBind("11","testnchar5");
cur.inputBind("12","testvarchar5");
cur.inputBind("13","testnvarchar5");
cur.inputBind("14","testlvarchar5");
cur.inputBind("15",2005,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("16",2005,1,1,5,0,0,0,null,0);
cur.inputBindClob("17","testtext5","testtext5".length);
cur.inputBindBlob("18","testbyte5","testbyte5".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1","t");
cur.inputBind("2",6);
cur.inputBind("3",6);
cur.inputBind("4",6);
cur.inputBind("5",6);
cur.inputBind("6",6.5,4,2);
cur.inputBind("7",6.5,4,2);
cur.inputBind("8",6.5,4,2);
cur.inputBind("9",6.5,4,2);
cur.inputBind("10","testchar6");
cur.inputBind("11","testnchar6");
cur.inputBind("12","testvarchar6");
cur.inputBind("13","testnvarchar6");
cur.inputBind("14","testlvarchar6");
cur.inputBind("15",2006,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("16",2006,1,1,6,0,0,0,null,0);
cur.inputBindClob("17","testtext6","testtext6".length);
cur.inputBindBlob("18","testbyte6","testbyte6".length);
assertTrue(cur.executeQuery());
console.log("");


// array of input binds by position
console.log("ARRAY OF INPUT BINDS BY POSITION: ");
cur.clearBinds();
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
	"	null)");
cur.inputBinds(bindvars,bindvals);
assertTrue(cur.executeQuery());
console.log("");


// input bind by position with validation
console.log("INPUT BIND BY POSITION WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("1","t");
cur.inputBind("2",8);
cur.inputBind("3",8);
cur.inputBind("4",8);
cur.inputBind("5",8);
cur.inputBind("6",8.5,4,2);
cur.inputBind("7",8.5,4,2);
cur.inputBind("8",8.5,4,2);
cur.inputBind("9",8.5,4,2);
cur.inputBind("10","testchar8");
cur.inputBind("11","testnchar8");
cur.inputBind("12","testvarchar8");
cur.inputBind("13","testnvarchar8");
cur.inputBind("14","testlvarchar8");
cur.inputBind("15",2008,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("16",2008,1,1,8,0,0,0,null,0);
cur.inputBindClob("17","testtext8","testtext8".length);
cur.inputBindBlob("18","testbyte8","testbyte8".length);
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log("");


// input bind by name
// informix doesn't support bind by name


// array of input binds by name
// informix doesn't support bind by name


// input bind by name with validation
// informix doesn't support bind by name


// select
console.log("SELECT: ");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
console.log("");


// column count
console.log("COLUMN COUNT: ");
assertEqInt(cur.colCount(),18);
console.log("");


// column names
console.log("COLUMN NAMES: ");
assertEqStr(cur.getColumnName(0),"testboolean");
assertEqStr(cur.getColumnName(1),"testsmallint");
assertEqStr(cur.getColumnName(2),"testint");
assertEqStr(cur.getColumnName(3),"testbigint");
assertEqStr(cur.getColumnName(4),"testint8");
assertEqStr(cur.getColumnName(5),"testdecimal");
assertEqStr(cur.getColumnName(6),"testmoney");
assertEqStr(cur.getColumnName(7),"testsmallfloat");
assertEqStr(cur.getColumnName(8),"testfloat");
assertEqStr(cur.getColumnName(9),"testchar");
assertEqStr(cur.getColumnName(10),"testnchar");
assertEqStr(cur.getColumnName(11),"testvarchar");
assertEqStr(cur.getColumnName(12),"testnvarchar");
assertEqStr(cur.getColumnName(13),"testlvarchar");
assertEqStr(cur.getColumnName(14),"testdate");
assertEqStr(cur.getColumnName(15),"testdatetime");
assertEqStr(cur.getColumnName(16),"testtext");
assertEqStr(cur.getColumnName(17),"testbyte");
var cols=cur.getColumnNames();
assertEqStr(cols[0],"testboolean");
assertEqStr(cols[1],"testsmallint");
assertEqStr(cols[2],"testint");
assertEqStr(cols[3],"testbigint");
assertEqStr(cols[4],"testint8");
assertEqStr(cols[5],"testdecimal");
assertEqStr(cols[6],"testmoney");
assertEqStr(cols[7],"testsmallfloat");
assertEqStr(cols[8],"testfloat");
assertEqStr(cols[9],"testchar");
assertEqStr(cols[10],"testnchar");
assertEqStr(cols[11],"testvarchar");
assertEqStr(cols[12],"testnvarchar");
assertEqStr(cols[13],"testlvarchar");
assertEqStr(cols[14],"testdate");
assertEqStr(cols[15],"testdatetime");
assertEqStr(cols[16],"testtext");
assertEqStr(cols[17],"testbyte");
console.log("");


// column types
console.log("COLUMN TYPES: ");
assertEqStr(cur.getColumnType(0),"BOOLEAN");
assertEqStr(cur.getColumnType("testboolean"),"BOOLEAN");
assertEqStr(cur.getColumnType(1),"SMALLINT");
assertEqStr(cur.getColumnType("testsmallint"),"SMALLINT");
assertEqStr(cur.getColumnType(2),"INTEGER");
assertEqStr(cur.getColumnType("testint"),"INTEGER");
assertEqStr(cur.getColumnType(3),"BIGINT");
assertEqStr(cur.getColumnType("testbigint"),"BIGINT");
assertEqStr(cur.getColumnType(4),"INT8");
assertEqStr(cur.getColumnType("testint8"),"INT8");
assertEqStr(cur.getColumnType(5),"DECIMAL");
assertEqStr(cur.getColumnType("testdecimal"),"DECIMAL");
assertEqStr(cur.getColumnType(6),"MONEY");
assertEqStr(cur.getColumnType("testmoney"),"MONEY");
assertEqStr(cur.getColumnType(7),"SMALLFLOAT");
assertEqStr(cur.getColumnType("testsmallfloat"),
						"SMALLFLOAT");
assertEqStr(cur.getColumnType(8),"FLOAT");
assertEqStr(cur.getColumnType("testfloat"),"FLOAT");
assertEqStr(cur.getColumnType(9),"CHAR");
assertEqStr(cur.getColumnType("testchar"),"CHAR");
// informix reports nchar as char, with no way to tell them apart
assertEqStr(cur.getColumnType(10),"CHAR");
assertEqStr(cur.getColumnType("testnchar"),"CHAR");
assertEqStr(cur.getColumnType(11),"VARCHAR");
assertEqStr(cur.getColumnType("testvarchar"),"VARCHAR");
// informix reports nvarchar as varchar, with no way to tell them apart
assertEqStr(cur.getColumnType(12),"VARCHAR");
assertEqStr(cur.getColumnType("testnvarchar"),"VARCHAR");
assertEqStr(cur.getColumnType(13),"LVARCHAR");
assertEqStr(cur.getColumnType("testlvarchar"),"LVARCHAR");
assertEqStr(cur.getColumnType(14),"DATE");
assertEqStr(cur.getColumnType("testdate"),"DATE");
assertEqStr(cur.getColumnType(15),"DATETIME");
assertEqStr(cur.getColumnType("testdatetime"),
						"DATETIME");
assertEqStr(cur.getColumnType(16),"TEXT");
assertEqStr(cur.getColumnType("testtext"),"TEXT");
assertEqStr(cur.getColumnType(17),"BYTE");
assertEqStr(cur.getColumnType("testbyte"),"BYTE");
console.log("");


// column length
console.log("COLUMN LENGTH: ");
assertEqInt(cur.getColumnLength(0),1);
assertEqInt(cur.getColumnLength("testboolean"),1);
assertEqInt(cur.getColumnLength(1),5);
assertEqInt(cur.getColumnLength("testsmallint"),5);
assertEqInt(cur.getColumnLength(2),10);
assertEqInt(cur.getColumnLength("testint"),10);
assertEqInt(cur.getColumnLength(3),20);
assertEqInt(cur.getColumnLength("testbigint"),20);
assertEqInt(cur.getColumnLength(4),20);
assertEqInt(cur.getColumnLength("testint8"),20);
assertEqInt(cur.getColumnLength(5),10);
assertEqInt(cur.getColumnLength("testdecimal"),10);
assertEqInt(cur.getColumnLength(6),16);
assertEqInt(cur.getColumnLength("testmoney"),16);
assertEqInt(cur.getColumnLength(7),7);
assertEqInt(cur.getColumnLength("testsmallfloat"),7);
assertEqInt(cur.getColumnLength(8),15);
assertEqInt(cur.getColumnLength("testfloat"),15);
assertEqInt(cur.getColumnLength(9),40);
assertEqInt(cur.getColumnLength("testchar"),40);
assertEqInt(cur.getColumnLength(10),40);
assertEqInt(cur.getColumnLength("testnchar"),40);
assertEqInt(cur.getColumnLength(11),40);
assertEqInt(cur.getColumnLength("testvarchar"),40);
assertEqInt(cur.getColumnLength(12),40);
assertEqInt(cur.getColumnLength("testnvarchar"),40);
assertEqInt(cur.getColumnLength(13),40);
assertEqInt(cur.getColumnLength("testlvarchar"),40);
assertEqInt(cur.getColumnLength(14),10);
assertEqInt(cur.getColumnLength("testdate"),10);
assertEqInt(cur.getColumnLength(15),19);
assertEqInt(cur.getColumnLength("testdatetime"),19);
assertEqInt(cur.getColumnLength(16),2147483647);
assertEqInt(cur.getColumnLength("testtext"),
						2147483647);
assertEqInt(cur.getColumnLength(17),2147483647);
assertEqInt(cur.getColumnLength("testbyte"),
						2147483647);
console.log("");


// longest column
console.log("LONGEST COLUMN: ");
assertEqInt(cur.getLongest(0),1);
assertEqInt(cur.getLongest("testboolean"),1);
assertEqInt(cur.getLongest(1),1);
assertEqInt(cur.getLongest("testsmallint"),1);
assertEqInt(cur.getLongest(2),1);
assertEqInt(cur.getLongest("testint"),1);
assertEqInt(cur.getLongest(3),1);
assertEqInt(cur.getLongest("testbigint"),1);
assertEqInt(cur.getLongest(4),1);
assertEqInt(cur.getLongest("testint8"),1);
assertEqInt(cur.getLongest(5),4);
assertEqInt(cur.getLongest("testdecimal"),4);
assertEqInt(cur.getLongest(6),4);
assertEqInt(cur.getLongest("testmoney"),4);
assertEqInt(cur.getLongest(7),3);
assertEqInt(cur.getLongest("testsmallfloat"),3);
assertEqInt(cur.getLongest(8),3);
assertEqInt(cur.getLongest("testfloat"),3);
assertEqInt(cur.getLongest(9),40);
assertEqInt(cur.getLongest("testchar"),40);
assertEqInt(cur.getLongest(10),40);
assertEqInt(cur.getLongest("testnchar"),40);
assertEqInt(cur.getLongest(11),12);
assertEqInt(cur.getLongest("testvarchar"),12);
assertEqInt(cur.getLongest(12),13);
assertEqInt(cur.getLongest("testnvarchar"),13);
assertEqInt(cur.getLongest(13),13);
assertEqInt(cur.getLongest("testlvarchar"),13);
assertEqInt(cur.getLongest(14),10);
assertEqInt(cur.getLongest("testdate"),10);
assertEqInt(cur.getLongest(15),19);
assertEqInt(cur.getLongest("testdatetime"),19);
assertEqInt(cur.getLongest(16),9);
assertEqInt(cur.getLongest("testtext"),9);
assertEqInt(cur.getLongest(17),9);
assertEqInt(cur.getLongest("testbyte"),9);
console.log("");


// row count
console.log("ROW COUNT: ");
assertEqInt(cur.rowCount(),8);
console.log("");


// total rows
console.log("TOTAL ROWS: ");
assertEqInt(cur.totalRows(),0);
console.log("");


// first row index
console.log("FIRST ROW INDEX: ");
assertEqInt(cur.firstRowIndex(),0);
console.log("");


// end of result set
console.log("END OF RESULT SET: ");
assertTrue(cur.endOfResultSet());
console.log("");


// fields by index
console.log("FIELDS BY INDEX: ");
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),"1");
assertEqStr(cur.getField(0,3),"1");
assertEqStr(cur.getField(0,4),"1");
assertEqStr(cur.getField(0,5),"1.50");
assertEqStr(cur.getField(0,6),"1.50");
assertEqStr(cur.getField(0,7),"1.5");
assertEqStr(cur.getField(0,8),"1.5");
assertEqStr(cur.getField(0,9),
		"testchar1                               ");
assertEqStr(cur.getField(0,10),
		"testnchar1                              ");
assertEqStr(cur.getField(0,11),"testvarchar1");
assertEqStr(cur.getField(0,12),"testnvarchar1");
assertEqStr(cur.getField(0,13),"testlvarchar1");
assertEqStr(cur.getField(0,14),"2001-01-01");
assertEqStr(cur.getField(0,15),
		"2001-01-01 01:00:00");
assertEqStr(cur.getField(0,16),"testtext1");
assertEqStr(cur.getField(0,17),"");
console.log("");
assertEqStr(cur.getField(7,0),"1");
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(7,2),"8");
assertEqStr(cur.getField(7,3),"8");
assertEqStr(cur.getField(7,4),"8");
assertEqStr(cur.getField(7,5),"8.50");
assertEqStr(cur.getField(7,6),"8.50");
assertEqStr(cur.getField(7,7),"8.5");
assertEqStr(cur.getField(7,8),"8.5");
assertEqStr(cur.getField(7,9),
		"testchar8                               ");
assertEqStr(cur.getField(7,10),
		"testnchar8                              ");
assertEqStr(cur.getField(7,11),"testvarchar8");
assertEqStr(cur.getField(7,12),"testnvarchar8");
assertEqStr(cur.getField(7,13),"testlvarchar8");
assertEqStr(cur.getField(7,14),"2008-01-01");
assertEqStr(cur.getField(7,15),
		"2008-01-01 08:00:00");
assertEqStr(cur.getField(7,16),"");
assertEqStr(cur.getField(7,17),"");
console.log("");


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqInt(cur.getFieldLength(0,0),1);
assertEqInt(cur.getFieldLength(0,1),1);
assertEqInt(cur.getFieldLength(0,2),1);
assertEqInt(cur.getFieldLength(0,3),1);
assertEqInt(cur.getFieldLength(0,4),1);
assertEqInt(cur.getFieldLength(0,5),4);
assertEqInt(cur.getFieldLength(0,6),4);
assertEqInt(cur.getFieldLength(0,7),3);
assertEqInt(cur.getFieldLength(0,8),3);
assertEqInt(cur.getFieldLength(0,9),40);
assertEqInt(cur.getFieldLength(0,10),40);
assertEqInt(cur.getFieldLength(0,11),12);
assertEqInt(cur.getFieldLength(0,12),13);
assertEqInt(cur.getFieldLength(0,14),10);
assertEqInt(cur.getFieldLength(0,15),19);
assertEqInt(cur.getFieldLength(0,16),9);
assertEqInt(cur.getFieldLength(0,17),0);
console.log("");
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),1);
assertEqInt(cur.getFieldLength(7,2),1);
assertEqInt(cur.getFieldLength(7,3),1);
assertEqInt(cur.getFieldLength(7,4),1);
assertEqInt(cur.getFieldLength(7,5),4);
assertEqInt(cur.getFieldLength(7,6),4);
assertEqInt(cur.getFieldLength(7,7),3);
assertEqInt(cur.getFieldLength(7,8),3);
assertEqInt(cur.getFieldLength(7,9),40);
assertEqInt(cur.getFieldLength(7,10),40);
assertEqInt(cur.getFieldLength(7,11),12);
assertEqInt(cur.getFieldLength(7,12),13);
assertEqInt(cur.getFieldLength(7,14),10);
assertEqInt(cur.getFieldLength(7,15),19);
assertEqInt(cur.getFieldLength(7,16),0);
assertEqInt(cur.getFieldLength(7,17),0);
console.log("");


// fields by name
console.log("FIELDS BY NAME: ");
assertEqStr(cur.getField(0,"testboolean"),"1");
assertEqStr(cur.getField(0,"testsmallint"),"1");
assertEqStr(cur.getField(0,"testint"),"1");
assertEqStr(cur.getField(0,"testbigint"),"1");
assertEqStr(cur.getField(0,"testint8"),"1");
assertEqStr(cur.getField(0,"testdecimal"),"1.50");
assertEqStr(cur.getField(0,"testmoney"),"1.50");
assertEqStr(cur.getField(0,"testsmallfloat"),"1.5");
assertEqStr(cur.getField(0,"testfloat"),"1.5");
assertEqStr(cur.getField(0,"testchar"),
		"testchar1                               ");
assertEqStr(cur.getField(0,"testnchar"),
		"testnchar1                              ");
assertEqStr(cur.getField(0,"testvarchar"),
		"testvarchar1");
assertEqStr(cur.getField(0,"testnvarchar"),
		"testnvarchar1");
assertEqStr(cur.getField(0,"testlvarchar"),
		"testlvarchar1");
assertEqStr(cur.getField(0,"testdate"),"2001-01-01");
assertEqStr(cur.getField(0,"testdatetime"),
		"2001-01-01 01:00:00");
assertEqStr(cur.getField(0,"testtext"),"testtext1");
assertEqStr(cur.getField(0,"testbyte"),"");
console.log("");
assertEqStr(cur.getField(7,"testboolean"),"1");
assertEqStr(cur.getField(7,"testsmallint"),"8");
assertEqStr(cur.getField(7,"testint"),"8");
assertEqStr(cur.getField(7,"testbigint"),"8");
assertEqStr(cur.getField(7,"testint8"),"8");
assertEqStr(cur.getField(7,"testdecimal"),"8.50");
assertEqStr(cur.getField(7,"testmoney"),"8.50");
assertEqStr(cur.getField(7,"testsmallfloat"),"8.5");
assertEqStr(cur.getField(7,"testfloat"),"8.5");
assertEqStr(cur.getField(7,"testchar"),
		"testchar8                               ");
assertEqStr(cur.getField(7,"testnchar"),
		"testnchar8                              ");
assertEqStr(cur.getField(7,"testvarchar"),
		"testvarchar8");
assertEqStr(cur.getField(7,"testnvarchar"),
		"testnvarchar8");
assertEqStr(cur.getField(7,"testlvarchar"),
		"testlvarchar8");
assertEqStr(cur.getField(7,"testdate"),"2008-01-01");
assertEqStr(cur.getField(7,"testdatetime"),
		"2008-01-01 08:00:00");
assertEqStr(cur.getField(7,"testtext"),"");
assertEqStr(cur.getField(7,"testbyte"),"");
console.log("");


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqInt(cur.getFieldLength(0,"testboolean"),1);
assertEqInt(cur.getFieldLength(0,"testsmallint"),1);
assertEqInt(cur.getFieldLength(0,"testint"),1);
assertEqInt(cur.getFieldLength(0,"testbigint"),1);
assertEqInt(cur.getFieldLength(0,"testint8"),1);
assertEqInt(cur.getFieldLength(0,"testdecimal"),4);
assertEqInt(cur.getFieldLength(0,"testmoney"),4);
assertEqInt(cur.getFieldLength(0,"testsmallfloat"),3);
assertEqInt(cur.getFieldLength(0,"testfloat"),3);
assertEqInt(cur.getFieldLength(0,"testchar"),40);
assertEqInt(cur.getFieldLength(0,"testnchar"),40);
assertEqInt(cur.getFieldLength(0,"testvarchar"),12);
assertEqInt(cur.getFieldLength(0,"testnvarchar"),13);
assertEqInt(cur.getFieldLength(0,"testlvarchar"),13);
assertEqInt(cur.getFieldLength(0,"testdate"),10);
assertEqInt(cur.getFieldLength(0,"testdatetime"),19);
assertEqInt(cur.getFieldLength(0,"testtext"),9);
assertEqInt(cur.getFieldLength(0,"testbyte"),0);
console.log("");
assertEqInt(cur.getFieldLength(7,"testboolean"),1);
assertEqInt(cur.getFieldLength(7,"testsmallint"),1);
assertEqInt(cur.getFieldLength(7,"testint"),1);
assertEqInt(cur.getFieldLength(7,"testbigint"),1);
assertEqInt(cur.getFieldLength(7,"testint8"),1);
assertEqInt(cur.getFieldLength(7,"testdecimal"),4);
assertEqInt(cur.getFieldLength(7,"testmoney"),4);
assertEqInt(cur.getFieldLength(7,"testsmallfloat"),3);
assertEqInt(cur.getFieldLength(7,"testfloat"),3);
assertEqInt(cur.getFieldLength(7,"testchar"),40);
assertEqInt(cur.getFieldLength(7,"testnchar"),40);
assertEqInt(cur.getFieldLength(7,"testvarchar"),12);
assertEqInt(cur.getFieldLength(7,"testnvarchar"),13);
assertEqInt(cur.getFieldLength(7,"testlvarchar"),13);
assertEqInt(cur.getFieldLength(7,"testdate"),10);
assertEqInt(cur.getFieldLength(7,"testdatetime"),19);
assertEqInt(cur.getFieldLength(7,"testtext"),0);
assertEqInt(cur.getFieldLength(7,"testbyte"),0);
console.log("");


// fields by array
console.log("FIELDS BY ARRAY: ");
var fields=cur.getRow(0);
assertEqStr(fields[0],"1");
assertEqStr(fields[1],"1");
assertEqStr(fields[2],"1");
assertEqStr(fields[3],"1");
assertEqStr(fields[4],"1");
assertEqStr(fields[5],"1.50");
assertEqStr(fields[6],"1.50");
assertEqStr(fields[7],"1.5");
assertEqStr(fields[8],"1.5");
assertEqStr(fields[9],"testchar1                               ");
assertEqStr(fields[10],"testnchar1                              ");
assertEqStr(fields[11],"testvarchar1");
assertEqStr(fields[12],"testnvarchar1");
assertEqStr(fields[13],"testlvarchar1");
assertEqStr(fields[14],"2001-01-01");
assertEqStr(fields[15],"2001-01-01 01:00:00");
assertEqStr(fields[16],"testtext1");
assertEqStr(fields[17],"");
console.log("");


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
var fieldlens=cur.getRowLengths(0);
assertEqInt(fieldlens[0],1);
assertEqInt(fieldlens[1],1);
assertEqInt(fieldlens[2],1);
assertEqInt(fieldlens[3],1);
assertEqInt(fieldlens[4],1);
assertEqInt(fieldlens[5],4);
assertEqInt(fieldlens[6],4);
assertEqInt(fieldlens[7],3);
assertEqInt(fieldlens[8],3);
assertEqInt(fieldlens[9],40);
assertEqInt(fieldlens[10],40);
assertEqInt(fieldlens[11],12);
assertEqInt(fieldlens[12],13);
assertEqInt(fieldlens[14],10);
assertEqInt(fieldlens[15],19);
assertEqInt(fieldlens[16],9);
assertEqInt(fieldlens[17],0);
console.log("");


// result set buffer size
console.log("RESULT SET BUFFER SIZE: ");
assertEqInt(cur.getResultSetBufferSize(),0);
cur.setResultSetBufferSize(2);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertEqInt(cur.getResultSetBufferSize(),2);
console.log("");
assertEqInt(cur.firstRowIndex(),0);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),2);
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(1,1),"2");
assertEqStr(cur.getField(2,1),"3");
console.log("");
assertEqInt(cur.firstRowIndex(),2);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),4);
assertEqStr(cur.getField(6,1),"7");
assertEqStr(cur.getField(7,1),"8");
console.log("");
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,1),null);
console.log("");
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.setResultSetBufferSize(0);
console.log("");


// dont get column info
console.log("DONT GET COLUMN INFO: ");
cur.dontGetColumnInfo();
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertEqStr(cur.getColumnName(1),null);
assertEqInt(cur.getColumnLength(1),0);
assertEqStr(cur.getColumnType(1),null);
cur.getColumnInfo();
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertEqStr(cur.getColumnName(1),"testsmallint");
assertEqInt(cur.getColumnLength(1),5);
assertEqStr(cur.getColumnType(1),"SMALLINT");
console.log("");


// suspended session
console.log("SUSPENDED SESSION: ");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
cur.suspendResultSet();
assertTrue(con.suspendSession());
var port=con.getConnectionPort();
var socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
console.log("");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(1,1),"2");
assertEqStr(cur.getField(2,1),"3");
assertEqStr(cur.getField(3,1),"4");
assertEqStr(cur.getField(4,1),"5");
assertEqStr(cur.getField(5,1),"6");
assertEqStr(cur.getField(6,1),"7");
assertEqStr(cur.getField(7,1),"8");
console.log("");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
console.log("");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(1,1),"2");
assertEqStr(cur.getField(2,1),"3");
assertEqStr(cur.getField(3,1),"4");
assertEqStr(cur.getField(4,1),"5");
assertEqStr(cur.getField(5,1),"6");
assertEqStr(cur.getField(6,1),"7");
assertEqStr(cur.getField(7,1),"8");
console.log("");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
console.log("");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(1,1),"2");
assertEqStr(cur.getField(2,1),"3");
assertEqStr(cur.getField(3,1),"4");
assertEqStr(cur.getField(4,1),"5");
assertEqStr(cur.getField(5,1),"6");
assertEqStr(cur.getField(6,1),"7");
assertEqStr(cur.getField(7,1),"8");
console.log("");


// suspended result set
console.log("SUSPENDED RESULT SET: ");
cur.setResultSetBufferSize(2);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertEqStr(cur.getField(2,1),"3");
var id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
assertTrue(cur.resumeResultSet(id));
console.log("");
assertEqInt(cur.firstRowIndex(),4);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),6);
assertEqStr(cur.getField(7,1),"8");
console.log("");
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,1),null);
console.log("");
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.setResultSetBufferSize(0);
console.log("");


// cached result set
console.log("CACHED RESULT SET: ");
cur.cacheToFile("cachefile1-informix");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
var filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-informix");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,1),"8");
console.log("");


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqInt(cur.colCount(),18);
console.log("");


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqStr(cur.getColumnName(0),"testboolean");
assertEqStr(cur.getColumnName(1),"testsmallint");
assertEqStr(cur.getColumnName(2),"testint");
assertEqStr(cur.getColumnName(3),"testbigint");
assertEqStr(cur.getColumnName(4),"testint8");
assertEqStr(cur.getColumnName(5),"testdecimal");
assertEqStr(cur.getColumnName(6),"testmoney");
assertEqStr(cur.getColumnName(7),"testsmallfloat");
assertEqStr(cur.getColumnName(8),"testfloat");
assertEqStr(cur.getColumnName(9),"testchar");
assertEqStr(cur.getColumnName(10),"testnchar");
assertEqStr(cur.getColumnName(11),"testvarchar");
assertEqStr(cur.getColumnName(12),"testnvarchar");
assertEqStr(cur.getColumnName(13),"testlvarchar");
assertEqStr(cur.getColumnName(14),"testdate");
assertEqStr(cur.getColumnName(15),"testdatetime");
assertEqStr(cur.getColumnName(16),"testtext");
assertEqStr(cur.getColumnName(17),"testbyte");
cols=cur.getColumnNames();
assertEqStr(cols[0],"testboolean");
assertEqStr(cols[1],"testsmallint");
assertEqStr(cols[2],"testint");
assertEqStr(cols[3],"testbigint");
assertEqStr(cols[4],"testint8");
assertEqStr(cols[5],"testdecimal");
assertEqStr(cols[6],"testmoney");
assertEqStr(cols[7],"testsmallfloat");
assertEqStr(cols[8],"testfloat");
assertEqStr(cols[9],"testchar");
assertEqStr(cols[10],"testnchar");
assertEqStr(cols[11],"testvarchar");
assertEqStr(cols[12],"testnvarchar");
assertEqStr(cols[13],"testlvarchar");
assertEqStr(cols[14],"testdate");
assertEqStr(cols[15],"testdatetime");
assertEqStr(cols[16],"testtext");
assertEqStr(cols[17],"testbyte");
console.log("");


// cached result set with result set buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-informix");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-informix");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(8,1),null);
cur.setResultSetBufferSize(0);
console.log("");


// from one cache file to another
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2-informix");
assertTrue(cur.openCachedResultSet("cachefile1-informix"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-informix"));
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(8,1),null);
console.log("");


// from one cache file to another with result set buffer size
console.log("FROM ONE CACHE FILE TO ANOTHER "+
			"WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile2-informix");
assertTrue(cur.openCachedResultSet("cachefile1-informix"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-informix"));
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(8,1),null);
cur.setResultSetBufferSize(0);
console.log("");


// cached result set with suspend and result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND "+
			"AND RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-informix");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertEqStr(cur.getField(2,1),"3");
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-informix");
id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
console.log("");
assertTrue(con.resumeSession(port,socket));
assertTrue(cur.resumeCachedResultSet(id,filename));
console.log("");
assertEqInt(cur.firstRowIndex(),4);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),6);
assertEqStr(cur.getField(7,1),"8");
console.log("");
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,1),null);
console.log("");
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.cacheOff();
console.log("");
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(8,1),null);
cur.setResultSetBufferSize(0);
console.log("");


// finished suspended session
console.log("FINISHED SUSPENDED SESSION: ");
assertTrue(cur.sendQuery(
	"select * from testtable order by testint"));
assertEqStr(cur.getField(4,1),"5");
assertEqStr(cur.getField(5,1),"6");
assertEqStr(cur.getField(6,1),"7");
assertEqStr(cur.getField(7,1),"8");
id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertTrue(con.resumeSession(port,socket));
assertTrue(cur.resumeResultSet(id));
assertEqStr(cur.getField(4,1),null);
assertEqStr(cur.getField(5,1),null);
assertEqStr(cur.getField(6,1),null);
assertEqStr(cur.getField(7,1),null);
console.log("");


// nested selects
console.log("NESTED SELECTS: ");
cur.setResultSetBufferSize(1);
assertTrue(cur.sendQuery("select * from testtable"));
var secondcur=new sqlrelay.SQLRCursor(con);
secondcur.setResultSetBufferSize(1);
for (var i=0; cur.getRow(i); i++) {
	assertTrue(secondcur.sendQuery("select * from testtable"));
}
// the nested selects must not disturb the outer result set
assertEqInt(i,cur.rowCount());
secondcur.closeResultSet();
cur.setResultSetBufferSize(0);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// reset transaction state
console.log("RESET TRANSACTION STATE: ");
assertTrue(con.commit());
assertEqStr(con.getTransactionModel(),"implicit");
assertFalse(con.getAutoCommit());
console.log("");


// transaction behavior - implicit
// Informix has no MVCC option -- the isolation level is either dirty
// reads (where the second connection sees uncommitted rows) or
// committed read (where it blocks or errors on locked rows) -- so
// the visibility assertions below may need to be revisited
console.log("TRANSACTION BEHAVIOR - implicit: ");
assertTrue(con.setTransactionModel("implicit"));
assertEqStr(con.getTransactionModel(),"implicit");
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
// informix DDL is transactional in logged mode; commit so the table
// is visible to the second connection (commit implicitly starts a
// new tx)
assertTrue(con.commit());
var secondcon=new sqlrelay.SQLRConnection("sqlrelay",9010,"/tmp/informixtest.socket",
					"testuser","testpassword",0,1);
setSecondConnection(secondcon);
var secondcur=new sqlrelay.SQLRCursor(secondcon);
setSecondCursor(secondcur);
// Informix has no MVCC; under default committed-read isolation,
// secondcur's catalog/data read errors with "Cannot get system
// information for table" while cur holds row locks from the
// in-flight tx.  Use dirty-read on secondcur so it sees the
// uncommitted writes — the test then verifies dirty-read
// semantics instead of MVCC visibility.
assertTrue(secondcur.sendQuery("set isolation to dirty read"));
// session is in a transaction; insert is visible via dirty read
assertTrue(con.getInTransaction());
assertFalse(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
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
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// transaction behavior - explicit
console.log("TRANSACTION BEHAVIOR - explicit: ");
assertTrue(con.setTransactionModel("explicit"));
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
// see note above re: informix dirty-read workaround
assertTrue(secondcur.sendQuery("set isolation to dirty read"));
// begin starts a new transaction; insert is visible via dirty read
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
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
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// transaction behavior - explicit-deferred
console.log("TRANSACTION BEHAVIOR - explicit-deferred: ");
assertTrue(con.setTransactionModel("explicit-deferred"));
assertEqStr(con.getTransactionModel(),"explicit-deferred");
// switch to autocommit-on so the begin/commit cycles below
// bracket explicit transactions (autocommit-off semantics are
// exercised at the end of this block)
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
// see note in - implicit section re: informix dirty-read workaround
assertTrue(secondcur.sendQuery("set isolation to dirty read"));
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
// explicitly commits/rollbacks the tx (mysql-native semantic).
// dirty-read on secondcur sees the in-flight insert (count=2)
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (3)"));
assertTrue(con.autoCommitOn());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
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
// dirty-read on secondcur sees the in-flight insert (count=5)
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable values (7)"));
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"5");
assertTrue(con.commit());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"5");
secondcur.closeResultSet();
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// transaction behavior - explicit-error
console.log("TRANSACTION BEHAVIOR - explicit-error: ");
assertTrue(con.setTransactionModel("explicit-error"));
assertEqStr(con.getTransactionModel(),"explicit-error");
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
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
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// transaction behavior - none
console.log("TRANSACTION BEHAVIOR - none: ");
assertTrue(con.setTransactionModel("none"));
assertEqStr(con.getTransactionModel(),"none");
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
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
console.log("");


// reset transaction behavior
console.log("RESET TRANSACTION BEHAVIOR: ");
assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
assertEqStr(con.getTransactionModel(),"implicit");
assertFalse(con.getAutoCommit());
console.log("");


// individual substitutions
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.prepareQuery(
	"select "+
	"	$(var1), "+
	"	'$(var2)', "+
	"	'$(var3)' "+
	"from "+
	"	sysmaster:sysdual ");
cur.substitution("var1",1);
cur.substitution("var2","hello");
cur.substitution("var3",10.5556,6,4);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"10.5556");
console.log("");


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery(
	"select "+
	"	'$(var1)', "+
	"	'$(var2)', "+
	"	'$(var3)' "+
	"from "+
	"	sysmaster:sysdual ");
cur.substitutions(subvars,subvalstrings);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"hi");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"bye");
console.log("");
cur.prepareQuery(
	"select "+
	"	$(var1), "+
	"	$(var2), "+
	"	$(var3) "+
	"from "+
	"	sysmaster:sysdual ");
cur.substitutions(subvars,subvallongs);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"2");
assertEqStr(cur.getField(0,2),"3");
console.log("");
cur.prepareQuery(
	"select "+
	"	$(var1), "+
	"	$(var2), "+
	"	$(var3) "+
	"from "+
	"	sysmaster:sysdual ");
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"10.55");
assertEqStr(cur.getField(0,1),"10.556");
assertEqStr(cur.getField(0,2),"10.5556");
console.log("");


// nulls as nulls
console.log("NULLS AS NULLS: ");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery(
	"select NULL::int,1,NULL::int from sysmaster:sysdual"));
assertEqStr(cur.getField(0,0),null);
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery(
	"select NULL::int,1,NULL::int from sysmaster:sysdual"));
assertEqStr(cur.getField(0,0),"");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),"");
console.log("");


// output bind by position
console.log("OUTPUT BIND BY POSITION: ");
cur.sendQuery("drop procedure testproc");
cur.getNullsAsNulls();
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
	"end procedure;"));
assertTrue(con.commit());
cur.prepareQuery("{call testproc(?,?,?,?)}");
assertEqInt(cur.countBindVariables(),4);
cur.defineOutputBindInteger("1");
cur.defineOutputBindString("2",20);
cur.defineOutputBindDouble("3");
cur.defineOutputBindString("4",20);
assertTrue(cur.executeQuery());
var numvar=cur.getOutputBindInteger("1");
var stringvar=cur.getOutputBindString("2");
var floatvar=cur.getOutputBindDouble("3");
var nullvar=cur.getOutputBindString("4");
assertEqInt(numvar,1);
assertEqStr(stringvar,"hello");
assertEqDbl(floatvar,2.5);
assertEqStr(nullvar,null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// output bind by name
// informix doesn't support bind by name


// output bind by name with validation
// informix doesn't support bind by name


// lob output bind
console.log("LOB OUTPUT BIND: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(con.commit());
cur.prepareQuery("insert into testtable values (?,?)");
cur.inputBindClob("1","hello","hello".length);
cur.inputBindBlob("2","hello","hello".length);
assertTrue(cur.executeQuery());
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	out out1 clob, "+
	"	out out2 blob) "+
	"select testclob, testblob "+
	"	into out1, out2 "+
	"	from testtable; "+
	"end procedure;"));
assertTrue(con.commit());
cur.prepareQuery("{call testproc(?,?)}");
cur.defineOutputBindClob("1");
cur.defineOutputBindBlob("2");
assertTrue(cur.executeQuery());
var clobvar=cur.getOutputBindClob("1");
var clobvarlength=cur.getOutputBindLength("1");
var blobvar=cur.getOutputBindBlob("2");
var blobvarlength=cur.getOutputBindLength("2");
assertEqStrLen(clobvar,"hello",5);
assertEqInt(clobvarlength,5);
assertEqStrLen(blobvar,"hello",5);
assertEqInt(blobvarlength,5);
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// long output bind
console.log("LONG OUTPUT BIND: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 clob, "+
	"	out out1 clob) "+
	"let out1 = in1; "+
	"	end procedure;"));
assertTrue(con.commit());
cur.prepareQuery("{call testproc(?,?)}");
var largebuffer="C".repeat(20*1024);
cur.inputBindClob("1",largebuffer,largebuffer.length);
cur.defineOutputBindClob("2");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindLength("2"),
		20*1024);
assertEqStr(cur.getOutputBindClob("2"),largebuffer);
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery("create table testtable (testval int)");
assertTrue(con.commit());
cur.prepareQuery("insert into testtable values (?)");
cur.inputBind("1",-1);
assertTrue(cur.executeQuery());
cur.sendQuery("select testval from testtable");
assertEqStr(cur.getField(0,"testval"),"-1");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// bind validation
// informix doesn't support bind by name

// rebinding
console.log("REBINDING: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 int, "+
	"	out out1 int) "+
	"let out1 = in1; "+
	"end procedure;"));
assertTrue(con.commit());
cur.prepareQuery("{call testproc(?,?)}");
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
assertTrue(con.commit());
console.log("");


// reexecute
console.log("REEXECUTE: ");
cur.prepareQuery(
	"select 1 from sysmaster:sysdual");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
cur.prepareQuery(
	"select ?::int from sysmaster:sysdual");
cur.inputBind("1",1);
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
cur.inputBind("1",2);
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"2");
console.log("");


// stored procedure returning no value
console.log("STORED PROCEDURE RETURNING NO VALUE: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 int, "+
	"	in2 float, "+
	"	in3 varchar(20)) "+
	"end procedure;"));
assertTrue(con.commit());
cur.prepareQuery("{call testproc(?,?,?)}");
cur.inputBind("1",1);
cur.inputBind("2",2.5,2,1);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// stored procedure returning single value
console.log("STORED PROCEDURE RETURNING SINGLE VALUE: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in1 int, "+
	"	in2 float, "+
	"	in3 varchar(20), "+
	"	out out1 int) "+
	"let out1 = in1; "+
	"end procedure;"));
assertTrue(con.commit());
cur.prepareQuery("{call testproc(?,?,?,?)}");
cur.inputBind("1",1);
cur.inputBind("2",2.5,2,1);
cur.inputBind("3","hello");
cur.defineOutputBindInteger("4");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("4"),1);
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// stored procedure returning multiple values
console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
cur.sendQuery("drop procedure testproc");
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
	"end procedure;"));
assertTrue(con.commit());
cur.prepareQuery("{call testproc(?,?,?,?,?,?)}");
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
assertTrue(con.commit());
console.log("");


// stored procedure returning result set
console.log("STORED PROCEDURE RETURNING RESULT SET: ");
cur.sendQuery("drop procedure testproc");
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
	"	end procedure;"));
assertTrue(con.commit());
assertTrue(cur.sendQuery("{call testproc()}"));
assertEqInt(cur.rowCount(),8);
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// null and empty lobs
console.log("NULL AND EMPTY LOBS: ");
cur.sendQuery("drop table testtable");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob1 clob, "+
	"	testclob2 clob, "+
	"	testblob1 blob, "+
	"	testblob2 blob)"));
assertTrue(con.commit());
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	?, "+
	"	?, "+
	"	?, "+
	"	?)");
cur.inputBindClob("1","","".length);
cur.inputBindClob("2",null,0);
cur.inputBindBlob("3","","".length);
cur.inputBindBlob("4",null,0);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
// informix returns a single \0 for an empty lob; the C/C++ tests
// pass via strcmp (which stops at \0) so truncate at first \0 here.
var f0=cur.getField(0,0);
if (f0===false || f0===null || f0===undefined) {
	f0="";
} else {
	var nul=String(f0).indexOf("\0");
	if (nul!==-1) {
		f0=String(f0).substring(0,nul);
	}
}
assertEqStr(f0,"");
assertEqStr(cur.getField(0,1),null);
var f2=cur.getField(0,2);
if (f2===false || f2===null || f2===undefined) {
	f2="";
} else {
	var nul2=String(f2).indexOf("\0");
	if (nul2!==-1) {
		f2=String(f2).substring(0,nul2);
	}
}
assertEqStr(f2,"");
assertEqStr(cur.getField(0,3),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// long lobs
console.log("LONG LOBS: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(
	"create table testtable ("+
	"	testtext text, "+
	"	testbyte byte)");
assertTrue(con.commit());
cur.prepareQuery(
	"insert into testtable values (?,?)");
largebuffer="C".repeat(20*1024);
cur.inputBindClob("1",largebuffer,largebuffer.length);
cur.inputBindBlob("2",largebuffer,largebuffer.length);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
assertEqInt(cur.getFieldLength(0,"testtext"),
		20*1024);
assertEqStr(cur.getField(0,"testtext"),largebuffer);
assertEqInt(cur.getFieldLength(0,"testbyte"),
		20*1024);
assertEqStrLen(cur.getField(0,"testbyte"),
		largebuffer,20*1024);
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// temporary tables
console.log("TEMPORARY TABLES: ");
cur.sendQuery("drop table temptable");
cur.sendQuery(
	"create temp table temptable (col1 int)");
assertTrue(cur.sendQuery(
	"insert into temptable values (1)"));
assertTrue(cur.sendQuery(
	"select count(*) from temptable"));
assertEqStr(cur.getField(0,0),"1");
con.endSession();
console.log("");
assertFalse(cur.sendQuery(
	"select count(*) from temptable"));
console.log("");


// encoded binary data
// informix doesn't support encoded binary data


// quotes
console.log("QUOTES: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable (col1 varchar(4))"));
assertTrue(cur.sendQuery(
	"insert into testtable values ('''''')"));
assertTrue(cur.sendQuery(
	"select col1 from testtable"));
assertEqInt(cur.getFieldLength(0,0),2);
assertEqStr(cur.getField(0,0),"''");
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// last insert id
console.log("LAST INSERT ID: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
		"create table testtable "+
		"	(col1 serial primary key, "+
		"	col2 int)"));
assertTrue(cur.sendQuery(
		"insert into testtable (col2) values (1)"));
assertEqInt(con.getLastInsertId(),1);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// database is schema
console.log("DATABASE IS SCHEMA: ");
assertFalse(con.getDatabaseIsSchema());
console.log("");


// catalog list
console.log("CATALOG LIST: ");
assertTrue(cur.getCatalogList(null));
assertEqStr(cur.getColumnName(0),"Database");
assertInResultSet(cur,"Database",hostname);
console.log("");


// schema list
console.log("SCHEMA LIST: ");
// informix requires that a table exist that is
// owned by a user for the user to be reported
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 integer, "+
	"	col2 integer)"));
assertTrue(con.commit());
assertTrue(cur.getSchemaList(null));
assertEqStr(cur.getColumnName(0),"Database");
assertInResultSet(cur,"Database","testuser");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// table type list
console.log("TABLE TYPE LIST: ");
assertTrue(cur.getTableTypeList());
assertEqStr(cur.getColumnName(0),"table_type");
assertInResultSet(cur,"table_type","TABLE");
console.log("");


// table list
console.log("TABLE LIST: ");
cur.sendQuery("drop table testtable1");
cur.sendQuery("drop table testtable2");
cur.sendQuery("drop table testtable3");
cur.sendQuery("drop table testtable4");
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	col1 integer, "+
	"	col2 integer)"));
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	col1 integer, "+
	"	col2 integer)"));
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	col1 integer, "+
	"	col2 integer)"));
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	col1 integer, "+
	"	col2 integer)"));
assertTrue(con.commit());
assertTrue(cur.getTableList(null));
assertInResultSet(cur,"Tables_in_xxx","testtable1");
assertInResultSet(cur,"Tables_in_xxx","testtable2");
assertInResultSet(cur,"Tables_in_xxx","testtable3");
assertInResultSet(cur,"Tables_in_xxx","testtable4");
assertTrue(cur.sendQuery("drop table testtable1"));
assertTrue(cur.sendQuery("drop table testtable2"));
assertTrue(cur.sendQuery("drop table testtable3"));
assertTrue(cur.sendQuery("drop table testtable4"));
assertTrue(con.commit());
console.log("");


// type info list
console.log("TYPE INFO LIST: ");
assertTrue(cur.getTypeInfoList("integer"));
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
assertEqStr(cur.getField(0,"type_name"),"INTEGER");
assertEqStr(cur.getField(0,"data_type"),"4");
assertEqStr(cur.getField(0,"precision"),"10");
assertEqStr(cur.getField(0,"local_type_name"),
						"INTEGER");
assertTrue(cur.getTypeInfoList("char"));
assertEqStr(cur.getField(0,"type_name"),"CHAR");
assertEqStr(cur.getField(0,"data_type"),"1");
assertEqStr(cur.getField(0,"precision"),"32767");
assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
assertTrue(cur.getTypeInfoList("varchar"));
assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"255");
assertEqStr(cur.getField(0,"local_type_name"),
						"VARCHAR");
assertTrue(cur.getTypeInfoList("date"));
assertEqStr(cur.getField(0,"type_name"),"DATE");
assertEqStr(cur.getField(0,"data_type"),"91");
assertEqStr(cur.getField(0,"precision"),"10");
assertEqStr(cur.getField(0,"local_type_name"),"DATE");
console.log("");


// column list
console.log("COLUMN LIST: ");
cur.sendQuery("drop table testtable");
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
	"	testbyte byte)"));
assertTrue(con.commit());
assertTrue(cur.getColumnList("testtable",null));
assertEqStr(cur.getColumnName(0),"column_name");
assertEqStr(cur.getColumnName(1),"data_type");
assertEqStr(cur.getColumnName(2),
		"character_maximum_length");
assertEqStr(cur.getColumnName(3),"numeric_precision");
assertEqStr(cur.getColumnName(4),"numeric_scale");
assertEqStr(cur.getColumnName(5),"is_nullable");
assertEqStr(cur.getColumnName(6),"column_key");
assertEqStr(cur.getColumnName(7),"column_default");
assertEqStr(cur.getColumnName(8),"extra");
assertEqStr(cur.getField(0,"column_name"),
						"testboolean");
assertEqStr(cur.getField(1,"column_name"),
						"testsmallint");
assertEqStr(cur.getField(2,"column_name"),
						"testint");
assertEqStr(cur.getField(3,"column_name"),
						"testbigint");
assertEqStr(cur.getField(4,"column_name"),
						"testint8");
assertEqStr(cur.getField(5,"column_name"),
						"testdecimal");
assertEqStr(cur.getField(6,"column_name"),
						"testmoney");
assertEqStr(cur.getField(7,"column_name"),
						"testsmallfloat");
assertEqStr(cur.getField(8,"column_name"),
						"testfloat");
assertEqStr(cur.getField(9,"column_name"),
						"testchar");
assertEqStr(cur.getField(10,"column_name"),
						"testnchar");
assertEqStr(cur.getField(11,"column_name"),
						"testvarchar");
assertEqStr(cur.getField(12,"column_name"),
						"testnvarchar");
assertEqStr(cur.getField(13,"column_name"),
						"testlvarchar");
assertEqStr(cur.getField(14,"column_name"),
						"testdate");
assertEqStr(cur.getField(15,"column_name"),
						"testdatetime");
assertEqStr(cur.getField(16,"column_name"),
						"testtext");
assertEqStr(cur.getField(17,"column_name"),
						"testbyte");
assertEqStr(cur.getField(0,"data_type"),"BOOLEAN");
assertEqStr(cur.getField(1,"data_type"),"SMALLINT");
assertEqStr(cur.getField(2,"data_type"),"INTEGER");
assertEqStr(cur.getField(3,"data_type"),"BIGINT");
assertEqStr(cur.getField(4,"data_type"),"INT8");
assertEqStr(cur.getField(5,"data_type"),"DECIMAL");
assertEqStr(cur.getField(6,"data_type"),"MONEY");
assertEqStr(cur.getField(7,"data_type"),
						"SMALLFLOAT");
assertEqStr(cur.getField(8,"data_type"),"FLOAT");
assertEqStr(cur.getField(9,"data_type"),"CHAR");
assertEqStr(cur.getField(10,"data_type"),"NCHAR");
assertEqStr(cur.getField(11,"data_type"),"VARCHAR");
assertEqStr(cur.getField(12,"data_type"),"NVARCHAR");
assertEqStr(cur.getField(13,"data_type"),"LVARCHAR");
assertEqStr(cur.getField(14,"data_type"),"DATE");
assertEqStr(cur.getField(15,"data_type"),"DATETIME");
assertEqStr(cur.getField(16,"data_type"),"TEXT");
assertEqStr(cur.getField(17,"data_type"),"BYTE");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// column list - auto_increment, primary key
console.log("COLUMN LIST - auto_increment, primary key: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 serial primary key, "+
	"	col2 int)"));
assertTrue(con.commit());
assertTrue(cur.getColumnList("testtable",null));
assertEqStr(cur.getField(0,"extra"),"auto_increment");
assertEqStr(cur.getField(0,"column_key"),"PRI");
assertEqStr(cur.getField(1,"extra"),"");
assertEqStr(cur.getField(1,"column_key"),"");
console.log("");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"));
assertTrue(cur.getColumnList("testtable",null));
assertEqStr(cur.getField(0,"extra"),"");
assertEqStr(cur.getField(0,"column_key"),"PRI");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// primary keys list
console.log("PRIMARY KEYS LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 integer primary key, "+
	"	col2 integer)"));
assertTrue(con.commit());
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
var keyname=cur.getField(0,"key_name");
assertTrue(keyname && String(keyname).length>0);
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// key and index list
console.log("KEY AND INDEX LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 integer primary key, "+
	"	col2 integer)"));
assertTrue(con.commit());
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
assertEqStr(cur.getField(0,"index_type"),"3");
keyname=cur.getField(0,"key_name");
assertTrue(keyname && String(keyname).length>0);
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// procedure list
console.log("PROCEDURE LIST: ");
cur.sendQuery("drop procedure testproc1");
cur.sendQuery("drop procedure testproc2");
cur.sendQuery("drop procedure testproc3");
cur.sendQuery("drop procedure testproc4");
assertTrue(cur.sendQuery(
	"create procedure testproc1("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"));
assertTrue(cur.sendQuery(
	"create procedure testproc2("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"));
assertTrue(cur.sendQuery(
	"create procedure testproc3("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"));
assertTrue(cur.sendQuery(
	"create procedure testproc4("+
	"	in1 integer, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) "+
	"define x integer; "+
	"let x = 1; "+
	"end procedure;"));
assertTrue(con.commit());
assertTrue(cur.getProcedureList(null));
assertInResultSet(cur,"routine_name","testproc1");
assertInResultSet(cur,"routine_name","testproc2");
assertInResultSet(cur,"routine_name","testproc3");
assertInResultSet(cur,"routine_name","testproc4");
console.log("");


// procedure parameter list
console.log("PROCEDURE PARAMETER LIST: ");
assertTrue(cur.getProcedureParameterList("testproc1",null));
assertEqStr(cur.getColumnName(0),"parameter_name");
assertEqStr(cur.getColumnName(1),"parameter_mode");
assertEqStr(cur.getColumnName(2),"data_type");
assertEqStr(cur.getColumnName(3),
		"character_maximum_length");
assertEqStr(cur.getColumnName(4),"ordinal_position");
assertEqInt(cur.rowCount(),4);
assertEqStr(cur.getField(0,"parameter_name"),
						"in1");
assertEqStr(cur.getField(0,"parameter_mode"),"1");
assertEqStr(cur.getField(0,"data_type"),"integer");
assertEqStr(cur.getField(0,"ordinal_position"),"1");
assertEqStr(cur.getField(1,"parameter_name"),
						"in2");
assertEqStr(cur.getField(1,"parameter_mode"),"1");
assertEqStr(cur.getField(1,"data_type"),"char");
assertEqStr(cur.getField(1,"ordinal_position"),"2");
assertEqStr(cur.getField(2,"parameter_name"),
						"in3");
assertEqStr(cur.getField(2,"parameter_mode"),"1");
assertEqStr(cur.getField(2,"data_type"),"varchar");
assertEqStr(cur.getField(2,"ordinal_position"),"3");
assertEqStr(cur.getField(3,"parameter_name"),
						"in4");
assertEqStr(cur.getField(3,"parameter_mode"),"1");
assertEqStr(cur.getField(3,"data_type"),"date");
assertEqStr(cur.getField(3,"ordinal_position"),"4");
assertTrue(cur.sendQuery("drop procedure testproc1"));
assertTrue(cur.sendQuery("drop procedure testproc2"));
assertTrue(cur.sendQuery("drop procedure testproc3"));
assertTrue(cur.sendQuery("drop procedure testproc4"));
assertTrue(con.commit());
console.log("");


// invalid queries
console.log("INVALID QUERIES: ");
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
console.log("");
assertFalse(cur.sendQuery(
	"insert into testtable values (1,2,3,4)"));
assertFalse(cur.sendQuery(
	"insert into testtable values (1,2,3,4)"));
assertFalse(cur.sendQuery(
	"insert into testtable values (1,2,3,4)"));
assertFalse(cur.sendQuery(
	"insert into testtable values (1,2,3,4)"));
console.log("");
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
console.log("");


reportTestStatus();
process.exit(getStatus());
