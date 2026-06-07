// Copyright (c) David Muse
// See the file COPYING for more information.

var sqlrelay=require("sqlrelay");
var {
    setConnection, setCursor,
    setSecondConnection, setSecondCursor,
    assertEqStr, assertEqStrLen,
    assertEqInt, assertEqDbl, assertEqual,
    assertTrue, assertFalse,
    getStatus, reportTestStatus
}=require("./asserts.js");


var isolationlevels=["CS","UR","RS","RR"];
var bindvars=["1","2","3","4","5","6",
			"7","8","9","10","11","12"];
var bindvals=["7","7","7","7.5","7.5","7.5",
			"testchar7","testvarchar7",
			"01/01/2007","07:00:00","testclob7",null];
var subvars=["var1","var2","var3"];
var subvallongs=[1,2,3];
var subvalstrings=["hi","hello","bye"];
var subvaldoubles=[10.55,10.556,10.5556];
var precs=[4,5,6];
var scales=[2,3,4];

var LARGE_BUFFER_LENGTH=20*1024;


// instantiation
var con=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket",
			"db2inst1","testpassword",0,1);
setConnection(con);
var cur=new sqlrelay.SQLRCursor(con);
setCursor(cur);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"db2");
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
assertEqStr(con.nextvalFormat(),"(nextval for %s)");
console.log("");


// isolation levels
console.log("ISOLATION LEVELS: ");
for (var i=0; i<isolationlevels.length; i++) {
	var il=isolationlevels[i];
	assertTrue(con.setIsolationLevel(il));
	assertEqStr(con.getIsolationLevel(),il);
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
	"	testsmallint smallint, "+
	"	testint integer, "+
	"	testbigint bigint, "+
	"	testdecimal decimal(10,2), "+
	"	testreal real, "+
	"	testdouble double, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(con.commit());
console.log("");


// insert
console.log("INSERT: ");
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
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01/01/2001', "+
	"	'01:00:00', "+
	"	NULL, "+
	"	'testclob1', "+
	"	blob('testblob1'))"));
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
	"	NULL, "+
	"	?, "+
	"	?)");
assertEqInt(cur.countBindVariables(),12);
cur.inputBind("1",2);
cur.inputBind("2",2);
cur.inputBind("3",2);
cur.inputBind("4",2.5,4,2);
cur.inputBind("5",2.5,4,2);
cur.inputBind("6",2.5,4,2);
cur.inputBind("7","testchar2");
cur.inputBind("8","testvarchar2");
cur.inputBind("9",2002,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("10",-1,-1,-1,2,0,0,0,null,0);
cur.inputBindClob("11","testclob2","testclob2".length);
cur.inputBindBlob("12","testblob2","testblob2".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2",3);
cur.inputBind("3",3);
cur.inputBind("4",3.5,4,2);
cur.inputBind("5",3.5,4,2);
cur.inputBind("6",3.5,4,2);
cur.inputBind("7","testchar3");
cur.inputBind("8","testvarchar3");
cur.inputBind("9",2003,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("10",-1,-1,-1,3,0,0,0,null,0);
cur.inputBindClob("11","testclob3","testclob3".length);
cur.inputBindBlob("12","testblob3","testblob3".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",4);
cur.inputBind("2",4);
cur.inputBind("3",4);
cur.inputBind("4",4.5,4,2);
cur.inputBind("5",4.5,4,2);
cur.inputBind("6",4.5,4,2);
cur.inputBind("7","testchar4");
cur.inputBind("8","testvarchar4");
cur.inputBind("9",2004,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("10",-1,-1,-1,4,0,0,0,null,0);
cur.inputBindClob("11","testclob4","testclob4".length);
cur.inputBindBlob("12","testblob4","testblob4".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",5);
cur.inputBind("2",5);
cur.inputBind("3",5);
cur.inputBind("4",5.5,4,2);
cur.inputBind("5",5.5,4,2);
cur.inputBind("6",5.5,4,2);
cur.inputBind("7","testchar5");
cur.inputBind("8","testvarchar5");
cur.inputBind("9",2005,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("10",-1,-1,-1,5,0,0,0,null,0);
cur.inputBindClob("11","testclob5","testclob5".length);
cur.inputBindBlob("12","testblob5","testblob5".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",6);
cur.inputBind("2",6);
cur.inputBind("3",6);
cur.inputBind("4",6.5,4,2);
cur.inputBind("5",6.5,4,2);
cur.inputBind("6",6.5,4,2);
cur.inputBind("7","testchar6");
cur.inputBind("8","testvarchar6");
cur.inputBind("9",2006,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("10",-1,-1,-1,6,0,0,0,null,0);
cur.inputBindClob("11","testclob6","testclob6".length);
cur.inputBindBlob("12","testblob6","testblob6".length);
assertTrue(cur.executeQuery());
console.log("");


// array of input binds by position
console.log("ARRAY OF INPUT BINDS BY POSITION: ");
cur.clearBinds();
cur.inputBinds(bindvars,bindvals);
assertTrue(cur.executeQuery());
console.log("");


// input bind by position with validation
console.log("INPUT BIND BY POSITION WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("1",8);
cur.inputBind("2",8);
cur.inputBind("3",8);
cur.inputBind("4",8.5,4,2);
cur.inputBind("5",8.5,4,2);
cur.inputBind("6",8.5,4,2);
cur.inputBind("7","testchar8");
cur.inputBind("8","testvarchar8");
cur.inputBind("9",2008,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("10",-1,-1,-1,8,0,0,0,null,0);
cur.inputBindClob("11","testclob8","testclob8".length);
cur.inputBindBlob("12","testblob8","testblob8".length);
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log("");

// input bind by name
// db2 doesn't support bind by name


// array of input binds by name
// db2 doesn't support bind by name


// input bind by name with validation
// db2 doesn't support bind by name


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
assertEqInt(cur.colCount(),13);
console.log("");


// column names
console.log("COLUMN NAMES: ");
assertEqStr(cur.getColumnName(0),"TESTSMALLINT");
assertEqStr(cur.getColumnName(1),"TESTINT");
assertEqStr(cur.getColumnName(2),"TESTBIGINT");
assertEqStr(cur.getColumnName(3),"TESTDECIMAL");
assertEqStr(cur.getColumnName(4),"TESTREAL");
assertEqStr(cur.getColumnName(5),"TESTDOUBLE");
assertEqStr(cur.getColumnName(6),"TESTCHAR");
assertEqStr(cur.getColumnName(7),"TESTVARCHAR");
assertEqStr(cur.getColumnName(8),"TESTDATE");
assertEqStr(cur.getColumnName(9),"TESTTIME");
assertEqStr(cur.getColumnName(10),"TESTTIMESTAMP");
var cols=cur.getColumnNames();
assertEqStr(cols[0],"TESTSMALLINT");
assertEqStr(cols[1],"TESTINT");
assertEqStr(cols[2],"TESTBIGINT");
assertEqStr(cols[3],"TESTDECIMAL");
assertEqStr(cols[4],"TESTREAL");
assertEqStr(cols[5],"TESTDOUBLE");
assertEqStr(cols[6],"TESTCHAR");
assertEqStr(cols[7],"TESTVARCHAR");
assertEqStr(cols[8],"TESTDATE");
assertEqStr(cols[9],"TESTTIME");
assertEqStr(cols[10],"TESTTIMESTAMP");
console.log("");


// column types
console.log("COLUMN TYPES: ");
assertEqStr(cur.getColumnType(0),"SMALLINT");
assertEqStr(cur.getColumnType("TESTSMALLINT"),"SMALLINT");
assertEqStr(cur.getColumnType(1),"INTEGER");
assertEqStr(cur.getColumnType("TESTINT"),"INTEGER");
assertEqStr(cur.getColumnType(2),"BIGINT");
assertEqStr(cur.getColumnType("TESTBIGINT"),"BIGINT");
assertEqStr(cur.getColumnType(3),"DECIMAL");
assertEqStr(cur.getColumnType("TESTDECIMAL"),"DECIMAL");
assertEqStr(cur.getColumnType(4),"REAL");
assertEqStr(cur.getColumnType("TESTREAL"),"REAL");
assertEqStr(cur.getColumnType(5),"DOUBLE");
assertEqStr(cur.getColumnType("TESTDOUBLE"),"DOUBLE");
assertEqStr(cur.getColumnType(6),"CHAR");
assertEqStr(cur.getColumnType("TESTCHAR"),"CHAR");
assertEqStr(cur.getColumnType(7),"VARCHAR");
assertEqStr(cur.getColumnType("TESTVARCHAR"),"VARCHAR");
assertEqStr(cur.getColumnType(8),"DATE");
assertEqStr(cur.getColumnType("TESTDATE"),"DATE");
assertEqStr(cur.getColumnType(9),"TIME");
assertEqStr(cur.getColumnType("TESTTIME"),"TIME");
assertEqStr(cur.getColumnType(10),"TIMESTAMP");
assertEqStr(cur.getColumnType("TESTTIMESTAMP"),
	"TIMESTAMP");
console.log("");


// column length
console.log("COLUMN LENGTH: ");
assertEqInt(cur.getColumnLength(0),2);
assertEqInt(cur.getColumnLength("TESTSMALLINT"),2);
assertEqInt(cur.getColumnLength(1),4);
assertEqInt(cur.getColumnLength("TESTINT"),4);
assertEqInt(cur.getColumnLength(2),8);
assertEqInt(cur.getColumnLength("TESTBIGINT"),8);
assertEqInt(cur.getColumnLength(3),12);
assertEqInt(cur.getColumnLength("TESTDECIMAL"),12);
assertEqInt(cur.getColumnLength(4),4);
assertEqInt(cur.getColumnLength("TESTREAL"),4);
assertEqInt(cur.getColumnLength(5),8);
assertEqInt(cur.getColumnLength("TESTDOUBLE"),8);
assertEqInt(cur.getColumnLength(6),40);
assertEqInt(cur.getColumnLength("TESTCHAR"),40);
assertEqInt(cur.getColumnLength(7),40);
assertEqInt(cur.getColumnLength("TESTVARCHAR"),40);
assertEqInt(cur.getColumnLength(8),6);
assertEqInt(cur.getColumnLength("TESTDATE"),6);
assertEqInt(cur.getColumnLength(9),6);
assertEqInt(cur.getColumnLength("TESTTIME"),6);
assertEqInt(cur.getColumnLength(10),16);
assertEqInt(cur.getColumnLength("TESTTIMESTAMP"),16);
console.log("");


// longest column
console.log("LONGEST COLUMN: ");
assertEqInt(cur.getLongest(0),1);
assertEqInt(cur.getLongest("TESTSMALLINT"),1);
assertEqInt(cur.getLongest(1),1);
assertEqInt(cur.getLongest("TESTINT"),1);
assertEqInt(cur.getLongest(2),1);
assertEqInt(cur.getLongest("TESTBIGINT"),1);
assertEqInt(cur.getLongest(3),4);
assertEqInt(cur.getLongest("TESTDECIMAL"),4);
assertEqInt(cur.getLongest(4),12);
assertEqInt(cur.getLongest("TESTREAL"),12);
assertEqInt(cur.getLongest(5),21);
assertEqInt(cur.getLongest("TESTDOUBLE"),21);
assertEqInt(cur.getLongest(6),40);
assertEqInt(cur.getLongest("TESTCHAR"),40);
assertEqInt(cur.getLongest(7),12);
assertEqInt(cur.getLongest("TESTVARCHAR"),12);
assertEqInt(cur.getLongest(8),10);
assertEqInt(cur.getLongest("TESTDATE"),10);
assertEqInt(cur.getLongest(9),8);
assertEqInt(cur.getLongest("TESTTIME"),8);
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
assertEqStr(cur.getField(0,3),"1.50");
assertEqStr(cur.getField(0,4),"1.500000E+00");
assertEqStr(cur.getField(0,5),"1.50000000000000E+000");
assertEqStr(cur.getField(0,6),"testchar1"+
				"                               ");
assertEqStr(cur.getField(0,7),"testvarchar1");
assertEqStr(cur.getField(0,8),"2001-01-01");
assertEqStr(cur.getField(0,9),"01:00:00");
console.log("");
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(7,2),"8");
assertEqStr(cur.getField(7,3),"8.50");
assertEqStr(cur.getField(7,4),"8.500000E+00");
assertEqStr(cur.getField(7,5),"8.50000000000000E+000");
assertEqStr(cur.getField(7,6),"testchar8"+
				"                               ");
assertEqStr(cur.getField(7,7),"testvarchar8");
assertEqStr(cur.getField(7,8),"2008-01-01");
assertEqStr(cur.getField(7,9),"08:00:00");
console.log("");


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqInt(cur.getFieldLength(0,0),1);
assertEqInt(cur.getFieldLength(0,1),1);
assertEqInt(cur.getFieldLength(0,2),1);
assertEqInt(cur.getFieldLength(0,3),4);
assertEqInt(cur.getFieldLength(0,4),12);
assertEqInt(cur.getFieldLength(0,5),21);
assertEqInt(cur.getFieldLength(0,6),40);
assertEqInt(cur.getFieldLength(0,7),12);
assertEqInt(cur.getFieldLength(0,8),10);
assertEqInt(cur.getFieldLength(0,9),8);
console.log("");
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),1);
assertEqInt(cur.getFieldLength(7,2),1);
assertEqInt(cur.getFieldLength(7,3),4);
assertEqInt(cur.getFieldLength(7,4),12);
assertEqInt(cur.getFieldLength(7,5),21);
assertEqInt(cur.getFieldLength(7,6),40);
assertEqInt(cur.getFieldLength(7,7),12);
assertEqInt(cur.getFieldLength(7,8),10);
assertEqInt(cur.getFieldLength(7,9),8);
console.log("");


// fields by name
console.log("FIELDS BY NAME: ");
assertEqStr(cur.getField(0,"TESTSMALLINT"),"1");
assertEqStr(cur.getField(0,"TESTINT"),"1");
assertEqStr(cur.getField(0,"TESTBIGINT"),"1");
assertEqStr(cur.getField(0,"TESTDECIMAL"),"1.50");
assertEqStr(cur.getField(0,"TESTREAL"),"1.500000E+00");
assertEqStr(cur.getField(0,"TESTDOUBLE"),"1.50000000000000E+000");
assertEqStr(cur.getField(0,"TESTCHAR"),"testchar1"+
				"                               ");
assertEqStr(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
assertEqStr(cur.getField(0,"TESTDATE"),"2001-01-01");
assertEqStr(cur.getField(0,"TESTTIME"),"01:00:00");
console.log("");
assertEqStr(cur.getField(7,"TESTSMALLINT"),"8");
assertEqStr(cur.getField(7,"TESTINT"),"8");
assertEqStr(cur.getField(7,"TESTBIGINT"),"8");
assertEqStr(cur.getField(7,"TESTDECIMAL"),"8.50");
assertEqStr(cur.getField(7,"TESTREAL"),"8.500000E+00");
assertEqStr(cur.getField(7,"TESTDOUBLE"),"8.50000000000000E+000");
assertEqStr(cur.getField(7,"TESTCHAR"),"testchar8"+
				"                               ");
assertEqStr(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
assertEqStr(cur.getField(7,"TESTDATE"),"2008-01-01");
assertEqStr(cur.getField(7,"TESTTIME"),"08:00:00");
console.log("");


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqInt(cur.getFieldLength(0,"TESTSMALLINT"),1);
assertEqInt(cur.getFieldLength(0,"TESTINT"),1);
assertEqInt(cur.getFieldLength(0,"TESTBIGINT"),1);
assertEqInt(cur.getFieldLength(0,"TESTDECIMAL"),4);
assertEqInt(cur.getFieldLength(0,"TESTREAL"),12);
assertEqInt(cur.getFieldLength(0,"TESTDOUBLE"),21);
assertEqInt(cur.getFieldLength(0,"TESTCHAR"),40);
assertEqInt(cur.getFieldLength(0,"TESTVARCHAR"),12);
assertEqInt(cur.getFieldLength(0,"TESTDATE"),10);
assertEqInt(cur.getFieldLength(0,"TESTTIME"),8);
console.log("");
assertEqInt(cur.getFieldLength(7,"TESTSMALLINT"),1);
assertEqInt(cur.getFieldLength(7,"TESTINT"),1);
assertEqInt(cur.getFieldLength(7,"TESTBIGINT"),1);
assertEqInt(cur.getFieldLength(7,"TESTDECIMAL"),4);
assertEqInt(cur.getFieldLength(7,"TESTREAL"),12);
assertEqInt(cur.getFieldLength(7,"TESTDOUBLE"),21);
assertEqInt(cur.getFieldLength(7,"TESTCHAR"),40);
assertEqInt(cur.getFieldLength(7,"TESTVARCHAR"),12);
assertEqInt(cur.getFieldLength(7,"TESTDATE"),10);
assertEqInt(cur.getFieldLength(7,"TESTTIME"),8);
console.log("");


// fields by array
console.log("FIELDS BY ARRAY: ");
var fields=cur.getRow(0);
assertEqStr(fields[0],"1");
assertEqStr(fields[1],"1");
assertEqStr(fields[2],"1");
assertEqStr(fields[3],"1.50");
assertEqStr(fields[4],"1.500000E+00");
assertEqStr(fields[5],"1.50000000000000E+000");
assertEqStr(fields[6],"testchar1"+"                               ");
assertEqStr(fields[7],"testvarchar1");
assertEqStr(fields[8],"2001-01-01");
assertEqStr(fields[9],"01:00:00");
console.log("");


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
var fieldlens=cur.getRowLengths(0);
assertEqInt(fieldlens[0],1);
assertEqInt(fieldlens[1],1);
assertEqInt(fieldlens[2],1);
assertEqInt(fieldlens[3],4);
assertEqInt(fieldlens[4],12);
assertEqInt(fieldlens[5],21);
assertEqInt(fieldlens[6],40);
assertEqInt(fieldlens[7],12);
assertEqInt(fieldlens[8],10);
assertEqInt(fieldlens[9],8);
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
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
console.log("");
assertEqInt(cur.firstRowIndex(),2);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),4);
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
console.log("");
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,0),null);
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
assertEqStr(cur.getColumnName(0),null);
assertEqInt(cur.getColumnLength(0),0);
assertEqStr(cur.getColumnType(0),null);
cur.getColumnInfo();
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertEqStr(cur.getColumnName(0),"TESTSMALLINT");
assertEqInt(cur.getColumnLength(0),2);
assertEqStr(cur.getColumnType(0),"SMALLINT");
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
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
assertEqStr(cur.getField(3,0),"4");
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
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
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
assertEqStr(cur.getField(3,0),"4");
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
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
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
assertEqStr(cur.getField(3,0),"4");
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
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
assertEqStr(cur.getField(2,0),"3");
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
assertEqStr(cur.getField(7,0),"8");
console.log("");
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,0),null);
console.log("");
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.setResultSetBufferSize(0);
console.log("");


// cached result set
console.log("CACHED RESULT SET: ");
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
var filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
console.log("");


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqInt(cur.colCount(),13);
console.log("");


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqStr(cur.getColumnName(0),"TESTSMALLINT");
assertEqStr(cur.getColumnName(1),"TESTINT");
assertEqStr(cur.getColumnName(2),"TESTBIGINT");
assertEqStr(cur.getColumnName(3),"TESTDECIMAL");
assertEqStr(cur.getColumnName(4),"TESTREAL");
assertEqStr(cur.getColumnName(5),"TESTDOUBLE");
assertEqStr(cur.getColumnName(6),"TESTCHAR");
assertEqStr(cur.getColumnName(7),"TESTVARCHAR");
assertEqStr(cur.getColumnName(8),"TESTDATE");
assertEqStr(cur.getColumnName(9),"TESTTIME");
assertEqStr(cur.getColumnName(10),"TESTTIMESTAMP");
cols=cur.getColumnNames();
assertEqStr(cols[0],"TESTSMALLINT");
assertEqStr(cols[1],"TESTINT");
assertEqStr(cols[2],"TESTBIGINT");
assertEqStr(cols[3],"TESTDECIMAL");
assertEqStr(cols[4],"TESTREAL");
assertEqStr(cols[5],"TESTDOUBLE");
assertEqStr(cols[6],"TESTCHAR");
assertEqStr(cols[7],"TESTVARCHAR");
assertEqStr(cols[8],"TESTDATE");
assertEqStr(cols[9],"TESTTIME");
assertEqStr(cols[10],"TESTTIMESTAMP");
console.log("");


// cached result set with result set
// buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log("");


// from one cache file to another
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2");
assertTrue(cur.openCachedResultSet("cachefile1"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
console.log("");


// from one cache file to another
// with result set buffer size
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
console.log("");


// cached result set with suspend
// and result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET "+
	"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testsmallint "));
assertEqStr(cur.getField(2,0),"3");
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
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
assertEqStr(cur.getField(7,0),"8");
console.log("");
assertEqInt(cur.firstRowIndex(),6);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(8,0),null);
console.log("");
assertEqInt(cur.firstRowIndex(),8);
assertTrue(cur.endOfResultSet());
assertEqInt(cur.rowCount(),8);
cur.cacheOff();
console.log("");
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log("");


// finished suspended session
console.log("FINISHED SUSPENDED SESSION: ");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testint"));
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
console.log("");


// nested selects
console.log("NESTED SELECTS: ");
cur.setResultSetBufferSize(1);
assertTrue(cur.sendQuery("select * from testtable"));
var secondcur=new sqlrelay.SQLRCursor(con);
secondcur.setResultSetBufferSize(1);
for (var i=0; cur.getRow(i); i++) {
	assertTrue(secondcur.sendQuery(
			"select * from testtable"));
}
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
console.log("TRANSACTION BEHAVIOR - implicit: ");
assertTrue(con.setTransactionModel("implicit"));
assertEqStr(con.getTransactionModel(),"implicit");
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
// db2 DDL is transactional; commit so the table is visible to the
// second connection (the commit implicitly starts a new tx)
assertTrue(con.commit());
var secondcon=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket",
				"db2inst1","testpassword",0,1);
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
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// transaction behavior - explicit
console.log("TRANSACTION BEHAVIOR - explicit: ");
assertTrue(con.setTransactionModel("explicit"));
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
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
cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')");
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
cur.prepareQuery("values ('$(var1)','$(var2)','$(var3)')");
cur.substitutions(subvars,subvalstrings);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"hi");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"bye");
console.log("");
cur.prepareQuery("values ($(var1),$(var2),$(var3))");
cur.substitutions(subvars,subvallongs);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"2");
assertEqStr(cur.getField(0,2),"3");
console.log("");
cur.prepareQuery("values ($(var1),$(var2),$(var3))");
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"10.55");
assertEqStr(cur.getField(0,1),"10.556");
assertEqStr(cur.getField(0,2),"10.5556");
console.log("");


// nulls as nulls
console.log("NULLS AS NULLS: ");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery("select NULL,1,NULL "+
	"from sysibm.sysdummy1"));
assertEqStr(cur.getField(0,0),null);
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("select NULL,1,NULL "+
	"from sysibm.sysdummy1"));
assertEqStr(cur.getField(0,0),"");
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),"");
console.log("");


// null and empty lobs
console.log("NULL AND EMPTY LOBS: ");
cur.getNullsAsNulls();
cur.sendQuery("drop table testtable");
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
assertEqStr(cur.getField(0,0),"");
assertEqStr(cur.getField(0,1),null);
assertEqStr(cur.getField(0,2),"");
assertEqStr(cur.getField(0,3),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// long lobs
console.log("LONG LOBS: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(con.commit());
cur.prepareQuery("insert into testtable values (?,?)");
var largebuffer="C".repeat(LARGE_BUFFER_LENGTH);
cur.inputBindClob("1",largebuffer,largebuffer.length);
cur.inputBindBlob("2",largebuffer,largebuffer.length);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
assertEqInt(cur.getFieldLength(0,"TESTCLOB"),
	LARGE_BUFFER_LENGTH);
assertEqStr(cur.getField(0,"TESTCLOB"),largebuffer);
assertEqInt(cur.getFieldLength(0,"TESTBLOB"),
	LARGE_BUFFER_LENGTH);
assertEqStrLen(cur.getField(0,"TESTBLOB"),largebuffer,
	LARGE_BUFFER_LENGTH);
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// output bind by position
console.log("OUTPUT BIND BY POSITION: ");
cur.sendQuery("drop procedure testproc");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	out out1 int, "+
	"	out out2 varchar(20), "+
	"	out out3 double, "+
	"	out out4 date, "+
	"	out out5 varchar(20)) language sql "+
	"begin "+
	"	set out1 = 1; "+
	"	set out2 = 'hello'; "+
	"	set out3 = 2.5; "+
	"	set out4 = '2001-02-03'; "+
	"	set out5 = null; end"));
assertTrue(con.commit());
cur.prepareQuery("call testproc(?,?,?,?,?)");
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
assertTrue(con.commit());
console.log("");


// output bind by name
// db2 doesn't support bind by name


// output bind by name with validation
// db2 doesn't support bind by name


// lob output bind
console.log("LOB OUTPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)");
assertTrue(con.commit());
cur.prepareQuery("insert into testtable values ('hello',?)");
cur.inputBindBlob("1","hello","hello".length);
assertTrue(cur.executeQuery());
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	out out1 clob, "+
	"	out out2 blob) language sql "+
	"begin "+
	"	select testclob into out1 "+
	"		from testtable; "+
	"	select testblob into out2 "+
	"		from testtable; end"));
assertTrue(con.commit());
cur.prepareQuery("call testproc(?,?)");
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
	"	in in1 clob, "+
	"	out out1 clob) language sql "+
	"begin "+
	"	set out1 = in1; end"));
assertTrue(con.commit());
largebuffer="C".repeat(LARGE_BUFFER_LENGTH);
cur.prepareQuery("call testproc(?,?)");
cur.inputBindClob("1",largebuffer,largebuffer.length);
cur.defineOutputBindClob("2");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindLength("2"),LARGE_BUFFER_LENGTH);
assertEqStr(cur.getOutputBindClob("2"),largebuffer);
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery("create table testtable (testval integer)");
assertTrue(con.commit());
cur.prepareQuery("insert into testtable values (?)");
cur.inputBind("1",-1);
assertTrue(cur.executeQuery());
cur.sendQuery("select testval from testtable");
assertEqStr(cur.getField(0,"TESTVAL"),"-1");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// bind validation
// db2 doesn't support bind by name


// rebinding
console.log("REBINDING: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int, "+
	"	out out1 int) language sql "+
	"begin "+
	"	set out1 = in1; end"));
assertTrue(con.commit());
cur.prepareQuery("call testproc(?,?)");
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
cur.prepareQuery("select 1 from sysibm.sysdummy1");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
cur.prepareQuery("select cast(? as integer) "+
	"from sysibm.sysdummy1");
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
	"	in in1 int, "+
	"	in in2 double, "+
	"	in in3 varchar(20)) language sql "+
	"begin "+
	"	return; end"));
assertTrue(con.commit());
cur.prepareQuery("call testproc(?,?,?)");
cur.inputBind("1",1);
cur.inputBind("2",1.1,2,1);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// stored procedure returning single value
console.log("STORED PROCEDURE RETURNING SINGLE VALUE: ");
cur.sendQuery("drop function testfunc");
assertTrue(cur.sendQuery(
	"create function testfunc("+
	"	in1 int, "+
	"	in2 double, "+
	"	in3 varchar(20)) returns int language sql "+
	"begin "+
	"	return in1; end"));
assertTrue(con.commit());
cur.prepareQuery("select testfunc(?,?,?) "+
	"from sysibm.sysdummy1");
cur.inputBind("1",1);
cur.inputBind("2",1.1,2,1);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertTrue(cur.sendQuery("drop function testfunc"));
assertTrue(con.commit());
console.log("");


// stored procedure returning
// multiple values
console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create procedure testproc("+
	"	in in1 int, "+
	"	in in2 double, "+
	"	in in3 varchar(20), "+
	"	in in4 clob, "+
	"	in in5 blob, "+
	"	out out1 int, "+
	"	out out2 double, "+
	"	out out3 varchar(20), "+
	"	out out4 clob, "+
	"	out out5 blob) language sql "+
	"begin "+
	"	set out1 = in1; "+
	"	set out2 = in2; "+
	"	set out3 = in3; "+
	"	set out4 = in4; "+
	"	set out5 = in5; end"));
assertTrue(con.commit());
cur.prepareQuery("call testproc(?,?,?,?,?,?,?,?,?,?)");
cur.inputBind("1",1);
cur.inputBind("2",1.1,2,1);
cur.inputBind("3","hello");
cur.inputBindClob("4","clob","clob".length);
cur.inputBindBlob("5","blob","blob".length);
cur.defineOutputBindInteger("6");
cur.defineOutputBindDouble("7");
cur.defineOutputBindString("8",20);
cur.defineOutputBindClob("9");
cur.defineOutputBindBlob("10");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("6"),1);
assertEqDbl(cur.getOutputBindDouble("7"),1.1);
assertEqStr(cur.getOutputBindString("8"),"hello");
assertEqStr(cur.getOutputBindClob("9"),"clob");
assertEqStr(cur.getOutputBindBlob("10"),"blob");
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// stored procedure returning result set
console.log("STORED PROCEDURE RETURNING RESULT SET: ");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery("create procedure testproc() "+
	"result set 1 language sql "+
	"begin "+
	"	declare c1 cursor "+
	"		with return for "+
	"		select 1 "+
	"		from sysibm.sysdummy1 "+
	"		union "+
	"		select 2 "+
	"		from sysibm.sysdummy1 "+
	"		union "+
	"		select 3 "+
	"		from sysibm.sysdummy1 "+
	"		union "+
	"		select 4 "+
	"		from sysibm.sysdummy1 "+
	"		union "+
	"		select 5 "+
	"		from sysibm.sysdummy1 "+
	"		union "+
	"		select 6 "+
	"		from sysibm.sysdummy1 "+
	"		union "+
	"		select 7 "+
	"		from sysibm.sysdummy1 "+
	"		union "+
	"		select 8 "+
	"		from sysibm.sysdummy1; "+
	"	open c1; end"));
assertTrue(con.commit());
assertTrue(cur.sendQuery("call testproc()"));
assertEqInt(cur.rowCount(),8);
assertTrue(cur.sendQuery("drop procedure testproc"));
assertTrue(con.commit());
console.log("");


// temporary tables
console.log("TEMPORARY TABLES: ");
cur.sendQuery("drop table session.temptable");
assertTrue(cur.sendQuery(
	"declare global temporary table session.temptable ("+
	"	col1 int "+
	") not logged"));
assertTrue(cur.sendQuery(
		"insert into session.temptable values (1)"));
assertTrue(cur.sendQuery(
		"select count(*) from session.temptable"));
assertEqStr(cur.getField(0,0),"1");
con.endSession();
console.log("");
assertFalse(cur.sendQuery(
		"select count(*) from session.temptable"));
console.log("");


// encoded binary data
console.log("ENCODED BINARY DATA: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable (col1 blob)"));
var buffer="";
for (var j=0; j<256; j++) {
	buffer+=String.fromCharCode(j);
}
var querystr="insert into testtable values (blob(X'";
for (var i=0; i<buffer.length; i++) {
	querystr+=("0"+buffer.charCodeAt(i).toString(16)).slice(-2);
}
querystr+="'))";
assertTrue(cur.sendQuery(querystr));
// Verify the raw bytes round-tripped by asking the server to hex-encode
// the blob. (Can't byte-compare the returned string directly — the Node.js
// sqlrelay binding returns strings via String::NewFromUtf8, which drops
// invalid UTF-8 sequences that arise from raw bytes 128-255.)
assertTrue(cur.sendQuery(
	"select hex(cast(col1 as varchar(256) for bit data)) "+
	"from testtable"));
var expectedhex="";
for (var i=0; i<buffer.length; i++) {
	expectedhex+=("0"+buffer.charCodeAt(i).toString(16)).slice(-2);
}
assertEqInt(cur.getFieldLength(0,0),expectedhex.length);
assertEqStr(String(cur.getField(0,0)).toLowerCase(),expectedhex);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// quotes
console.log("QUOTES: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable "+
	"(col1 varchar(4))"));
assertTrue(cur.sendQuery("insert into testtable "+
	"values ('''''')"));
assertTrue(cur.sendQuery("select col1 from testtable"));
assertEqInt(cur.getFieldLength(0,0),2);
assertEqInt((cur.getField(0,0)==="''")?0:1,0);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// last insert id
console.log("LAST INSERT ID: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable "+
	"	(col1 int not null "+
	"	generated always "+
	"	as identity, "+
	"	col2 int, "+
	"	primary key(col1))"));
assertTrue(cur.sendQuery("insert into testtable "+
	"(col2) values (1)"));
assertEqInt(con.getLastInsertId(),1);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// database is schema
console.log("DATABASE IS SCHEMA: ");
assertTrue(con.getDatabaseIsSchema());
console.log("");


// catalog list
console.log("CATALOG LIST: ");
assertTrue(cur.getCatalogList(null));
assertEqStr(cur.getColumnName(0),"Database");
assertEqInt(cur.rowCount(),0);
console.log("");


// schema list
console.log("SCHEMA LIST: ");
assertTrue(cur.getSchemaList(null));
assertEqStr(cur.getColumnName(0),"Database");
var found=0;
for (var i=0; i<cur.rowCount(); i++) {
	if (cur.getField(i,"Database")==="DB2INST1") {
		found=1;
		break;
	}
}
assertTrue(found);
console.log("");


// table type list
console.log("TABLE TYPE LIST: ");
assertTrue(cur.getTableTypeList());
assertEqStr(cur.getColumnName(0),"table_type");
found=0;
for (var i=0; i<cur.rowCount(); i++) {
	if (cur.getField(i,"table_type")==="TABLE") {
		found=1;
		break;
	}
}
assertTrue(found);
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
var counter=0;
for (var i=0; i<cur.rowCount(); i++) {
	var name=cur.getField(i,"Tables_in_xxx");
	if (name==="TESTTABLE1" || name==="TESTTABLE2" ||
		name==="TESTTABLE3" ||
		name==="TESTTABLE4") {
		counter++;
	}
}
assertEqInt(counter,4);
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
assertEqStr(cur.getField(0,"local_type_name"),"INTEGER");
assertTrue(cur.getTypeInfoList("char"));
assertEqStr(cur.getField(0,"type_name"),"CHAR");
assertEqStr(cur.getField(0,"data_type"),"1");
assertEqStr(cur.getField(0,"precision"),"254");
assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
assertTrue(cur.getTypeInfoList("varchar"));
assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"32672");
assertEqStr(cur.getField(0,"local_type_name"),"VARCHAR");
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
	"	testsmallint smallint, "+
	"	testint integer, "+
	"	testbigint bigint, "+
	"	testdecimal decimal(10,2), "+
	"	testreal real, "+
	"	testdouble double, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(con.commit());
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
assertEqStr(cur.getField(0,"column_name"),"TESTSMALLINT");
assertEqStr(cur.getField(1,"column_name"),"TESTINT");
assertEqStr(cur.getField(2,"column_name"),"TESTBIGINT");
assertEqStr(cur.getField(3,"column_name"),"TESTDECIMAL");
assertEqStr(cur.getField(4,"column_name"),"TESTREAL");
assertEqStr(cur.getField(5,"column_name"),"TESTDOUBLE");
assertEqStr(cur.getField(6,"column_name"),"TESTCHAR");
assertEqStr(cur.getField(7,"column_name"),"TESTVARCHAR");
assertEqStr(cur.getField(8,"column_name"),"TESTDATE");
assertEqStr(cur.getField(9,"column_name"),"TESTTIME");
assertEqStr(cur.getField(10,"column_name"),
							"TESTTIMESTAMP");
assertEqStr(cur.getField(11,"column_name"),"TESTCLOB");
assertEqStr(cur.getField(12,"column_name"),"TESTBLOB");
assertEqStr(cur.getField(0,"data_type"),"SMALLINT");
assertEqStr(cur.getField(1,"data_type"),"INTEGER");
assertEqStr(cur.getField(2,"data_type"),"BIGINT");
assertEqStr(cur.getField(3,"data_type"),"DECIMAL");
assertEqStr(cur.getField(4,"data_type"),"REAL");
assertEqStr(cur.getField(5,"data_type"),"DOUBLE");
assertEqStr(cur.getField(6,"data_type"),"CHARACTER");
assertEqStr(cur.getField(7,"data_type"),"VARCHAR");
assertEqStr(cur.getField(8,"data_type"),"DATE");
assertEqStr(cur.getField(9,"data_type"),"TIME");
assertEqStr(cur.getField(10,"data_type"),"TIMESTAMP");
assertEqStr(cur.getField(11,"data_type"),"CLOB");
assertEqStr(cur.getField(12,"data_type"),"BLOB");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// column list - auto_increment,
// primary key
console.log("COLUMN LIST - auto_increment, primary key: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int generated always "+
	"	as identity primary key, "+
	"	col2 int)"));
assertTrue(con.commit());
assertTrue(cur.getColumnList("testtable",null));
assertTrue(cur.getField(0,"extra").indexOf("auto_increment")!=-1);
assertTrue(cur.getField(0,"column_key").indexOf("PRI")!=-1);
assertFalse(cur.getField(1,"extra").indexOf("auto_increment")!=-1);
assertFalse(cur.getField(1,"column_key").indexOf("PRI")!=-1);
console.log("");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int not null "+
	"	primary key, "+
	"	col2 int)"));
assertTrue(con.commit());
assertTrue(cur.getColumnList("testtable",null));
assertFalse(cur.getField(0,"extra").indexOf("auto_increment")!=-1);
assertTrue(cur.getField(0,"column_key").indexOf("PRI")!=-1);
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// primary keys list
console.log("PRIMARY KEYS LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int not null "+
	"	primary key, "+
	"	col2 int)"));
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
assertTrue(cur.getField(0,"table")==="TESTTABLE");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")==="COL1");
var kn=cur.getField(0,"key_name");
assertTrue(!(!kn || !kn[0]));
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(con.commit());
console.log("");


// key and index list
console.log("KEY AND INDEX LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int not null "+
	"	primary key, "+
	"	col2 int)"));
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
assertTrue(cur.getField(0,"table")==="TESTTABLE");
assertEqStr(cur.getField(0,"non_unique"),"0");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")==="COL1");
assertEqStr(cur.getField(0,"collation"),"A");
assertEqStr(cur.getField(0,"index_type"),"3");
kn=cur.getField(0,"key_name");
assertTrue(!(!kn || !kn[0]));
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
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) language sql begin end"));
assertTrue(cur.sendQuery(
	"create procedure testproc2("+
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) language sql begin end"));
assertTrue(cur.sendQuery(
	"create procedure testproc3("+
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) language sql begin end"));
assertTrue(cur.sendQuery(
	"create procedure testproc4("+
	"	in in1 integer, "+
	"	in in2 char(20), "+
	"	in in3 varchar(20), "+
	"	in in4 date) language sql begin end"));
assertTrue(con.commit());
assertTrue(cur.getProcedureList(null));
counter=0;
for (var i=0; i<cur.rowCount(); i++) {
	var name=cur.getField(i,"routine_name");
	if (name==="TESTPROC1" || name==="TESTPROC2" ||
		name==="TESTPROC3" || name==="TESTPROC4") {
		counter++;
	}
}
assertEqInt(counter,4);
console.log("");


// procedure parameter list
console.log("PROCEDURE PARAMETER LIST: ");
assertTrue(cur.getProcedureParameterList("testproc1",null));
assertEqStr(cur.getColumnName(0),"parameter_name");
assertEqStr(cur.getColumnName(1),"parameter_mode");
assertEqStr(cur.getColumnName(2),"data_type");
assertEqStr(cur.getColumnName(3),"character_maximum_length");
assertEqStr(cur.getColumnName(4),"ordinal_position");
assertEqInt(cur.rowCount(),4);
assertEqStr(cur.getField(0,"parameter_name"),"IN1");
assertEqStr(cur.getField(0,"parameter_mode"),"1");
assertEqStr(cur.getField(0,"data_type"),"INTEGER");
assertEqStr(cur.getField(0,"ordinal_position"),"1");
assertEqStr(cur.getField(1,"parameter_name"),"IN2");
assertEqStr(cur.getField(1,"parameter_mode"),"1");
assertEqStr(cur.getField(1,"data_type"),"CHARACTER");
assertEqStr(cur.getField(1,"ordinal_position"),"2");
assertEqStr(cur.getField(2,"parameter_name"),"IN3");
assertEqStr(cur.getField(2,"parameter_mode"),"1");
assertEqStr(cur.getField(2,"data_type"),"VARCHAR");
assertEqStr(cur.getField(2,"ordinal_position"),"3");
assertEqStr(cur.getField(3,"parameter_name"),"IN4");
assertEqStr(cur.getField(3,"parameter_mode"),"1");
assertEqStr(cur.getField(3,"data_type"),"DATE");
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
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable "+
	"values (1,2,3,4)"));
console.log("");
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
console.log("");

reportTestStatus();
process.exit(getStatus());
