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

var	bindvars=["1","2","3","4","5","6","7","8","9","10",
				"11","12"];
var	bindvals=["7","7","7.5","7.5","7.5","7.5",
				"01-JAN-2007","07:00:00",
				"testchar7","testvarchar7",null,"testblob7"];
var	subvars=["var1","var2","var3"];
var	subvallongs=[1,2,3];
var	subvalstrings=["hi","hello","bye"];
var	subvaldoubles=[10.55,10.556,10.5556];
var	precs=[4,5,6];
var	scales=[2,3,4];

var	counter=0;

var	cols;
var	fields;
var	fieldlens;
var	port;
var	socket;
var	id;
var	filename;
var	largebuffer;
var	name;
var	found;


// instantiation
var	con=new sqlrelay.SQLRConnection("sqlrelay",9009,"/tmp/firebirdtest.socket",
			"testuser","testpassword",0,1);
var	cur=new sqlrelay.SQLRCursor(con);
setConnection(con);
setCursor(cur);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"firebird");
console.log();


// ping
console.log("PING: ");
assertTrue(con.ping());
console.log();


// transaction state
console.log("TRANSACTION STATE: ");
assertEqStr(con.getDefaultTransactionModel(),"implicit");
assertEqStr(con.getTransactionModel(),"implicit");
assertTrue(con.getInTransaction());
assertFalse(con.getAutoCommit());
console.log();


// bind format
console.log("BIND FORMAT: ");
assertEqStr(con.bindFormat(),"?");
console.log();


// nextval format
console.log("NEXTVAL FORMAT: ");
assertEqStr(con.nextvalFormat(),"next value for %s");
console.log();


// isolation levels
console.log("ISOLATION LEVELS: ");
// though firebird does support a
// "set transaction ..." statement to set the
// isolation level, it looks like, in firebird,
// you can really only set it through the TPB at
// the start of a transaction, so attempts to set
// it should fail
assertFalse(con.setIsolationLevel("read committed"));
assertEqStr(con.getIsolationLevel(),"read committed");
console.log();


// insert
console.log("INSERT: ");
cur.sendQuery("delete from testtable");
con.commit();
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	1.5, "+
	"	'01-JAN-2001', "+
	"	'01:00:00', "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	null, "+
	"	'testblob1')"));
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
	"	?)");
assertEqInt(cur.countBindVariables(),12);
cur.inputBind("1",2);
cur.inputBind("2",2);
cur.inputBind("3",2.5,2,1);
cur.inputBind("4",2.5,2,1);
cur.inputBind("5",2.5,2,1);
cur.inputBind("6",2.5,2,1);
cur.inputBind("7",2002,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("8",-1,-1,-1,2,0,0,0,null,0);
cur.inputBind("9","testchar2");
cur.inputBind("10","testvarchar2");
cur.inputBind("11",null);
cur.inputBindBlob("12","testblob2","testblob2".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2",3);
cur.inputBind("3",3.5,2,1);
cur.inputBind("4",3.5,2,1);
cur.inputBind("5",3.5,2,1);
cur.inputBind("6",3.5,2,1);
cur.inputBind("7",2003,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("8",-1,-1,-1,3,0,0,0,null,0);
cur.inputBind("9","testchar3");
cur.inputBind("10","testvarchar3");
cur.inputBind("11",null);
cur.inputBindBlob("12","testblob3","testblob3".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",4);
cur.inputBind("2",4);
cur.inputBind("3",4.5,2,1);
cur.inputBind("4",4.5,2,1);
cur.inputBind("5",4.5,2,1);
cur.inputBind("6",4.5,2,1);
cur.inputBind("7",2004,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("8",-1,-1,-1,4,0,0,0,null,0);
cur.inputBind("9","testchar4");
cur.inputBind("10","testvarchar4");
cur.inputBind("11",null);
cur.inputBindBlob("12","testblob4","testblob4".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",5);
cur.inputBind("2",5);
cur.inputBind("3",5.5,2,1);
cur.inputBind("4",5.5,2,1);
cur.inputBind("5",5.5,2,1);
cur.inputBind("6",5.5,2,1);
cur.inputBind("7",2005,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("8",-1,-1,-1,5,0,0,0,null,0);
cur.inputBind("9","testchar5");
cur.inputBind("10","testvarchar5");
cur.inputBind("11",null);
cur.inputBindBlob("12","testblob5","testblob5".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",6);
cur.inputBind("2",6);
cur.inputBind("3",6.5,2,1);
cur.inputBind("4",6.5,2,1);
cur.inputBind("5",6.5,2,1);
cur.inputBind("6",6.5,2,1);
cur.inputBind("7",2006,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("8",-1,-1,-1,6,0,0,0,null,0);
cur.inputBind("9","testchar6");
cur.inputBind("10","testvarchar6");
cur.inputBind("11",null);
cur.inputBindBlob("12","testblob6","testblob6".length);
assertTrue(cur.executeQuery());
console.log();


// array of input binds by position
console.log("ARRAY OF INPUT BINDS BY POSITION: ");
cur.clearBinds();
cur.inputBinds(bindvars,bindvals);
assertTrue(cur.executeQuery());
console.log();


// input bind by position with validation
console.log("INPUT BIND BY POSITION WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("1",8);
cur.inputBind("2",8);
cur.inputBind("3",8.5,2,1);
cur.inputBind("4",8.5,2,1);
cur.inputBind("5",8.5,2,1);
cur.inputBind("6",8.5,2,1);
cur.inputBind("7",2008,1,1,-1,-1,-1,-1,null,0);
cur.inputBind("8",-1,-1,-1,8,0,0,0,null,0);
cur.inputBind("9","testchar8");
cur.inputBind("10","testvarchar8");
cur.inputBind("11",null);
cur.inputBindBlob("12","testblob8","testblob8".length);
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log();


// input bind by name
// firebird doesn't support bind by name


// array of input binds by name
// firebird doesn't support bind by name


// input bind by name with validation
// firebird doesn't support bind by name


// select
console.log("SELECT: ");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
console.log();


// column count
console.log("COLUMN COUNT: ");
assertEqInt(cur.colCount(),12);
console.log();


// column names
console.log("COLUMN NAMES: ");
assertEqStr(cur.getColumnName(0),"TESTINTEGER");
assertEqStr(cur.getColumnName(1),"TESTSMALLINT");
assertEqStr(cur.getColumnName(2),"TESTDECIMAL");
assertEqStr(cur.getColumnName(3),"TESTNUMERIC");
assertEqStr(cur.getColumnName(4),"TESTFLOAT");
assertEqStr(cur.getColumnName(5),"TESTDOUBLE");
assertEqStr(cur.getColumnName(6),"TESTDATE");
assertEqStr(cur.getColumnName(7),"TESTTIME");
assertEqStr(cur.getColumnName(8),"TESTCHAR");
assertEqStr(cur.getColumnName(9),"TESTVARCHAR");
assertEqStr(cur.getColumnName(10),"TESTTIMESTAMP");
assertEqStr(cur.getColumnName(11),"TESTBLOB");
cols=cur.getColumnNames();
assertEqStr(cols[0],"TESTINTEGER");
assertEqStr(cols[1],"TESTSMALLINT");
assertEqStr(cols[2],"TESTDECIMAL");
assertEqStr(cols[3],"TESTNUMERIC");
assertEqStr(cols[4],"TESTFLOAT");
assertEqStr(cols[5],"TESTDOUBLE");
assertEqStr(cols[6],"TESTDATE");
assertEqStr(cols[7],"TESTTIME");
assertEqStr(cols[8],"TESTCHAR");
assertEqStr(cols[9],"TESTVARCHAR");
assertEqStr(cols[10],"TESTTIMESTAMP");
assertEqStr(cols[11],"TESTBLOB");
console.log();


// column types
console.log("COLUMN TYPES: ");
assertEqStr(cur.getColumnType(0),"INTEGER");
assertEqStr(cur.getColumnType("TESTINTEGER"),"INTEGER");
assertEqStr(cur.getColumnType(1),"SMALLINT");
assertEqStr(cur.getColumnType("TESTSMALLINT"),"SMALLINT");
assertEqStr(cur.getColumnType(2),"DECIMAL");
assertEqStr(cur.getColumnType("TESTDECIMAL"),"DECIMAL");
assertEqStr(cur.getColumnType(3),"NUMERIC");
assertEqStr(cur.getColumnType("TESTNUMERIC"),"NUMERIC");
assertEqStr(cur.getColumnType(4),"FLOAT");
assertEqStr(cur.getColumnType("TESTFLOAT"),"FLOAT");
assertEqStr(cur.getColumnType(5),"DOUBLE PRECISION");
assertEqStr(cur.getColumnType("TESTDOUBLE"),
	"DOUBLE PRECISION");
assertEqStr(cur.getColumnType(6),"DATE");
assertEqStr(cur.getColumnType("TESTDATE"),"DATE");
assertEqStr(cur.getColumnType(7),"TIME");
assertEqStr(cur.getColumnType("TESTTIME"),"TIME");
assertEqStr(cur.getColumnType(8),"CHAR");
assertEqStr(cur.getColumnType("TESTCHAR"),"CHAR");
assertEqStr(cur.getColumnType(9),"VARCHAR");
assertEqStr(cur.getColumnType("TESTVARCHAR"),"VARCHAR");
assertEqStr(cur.getColumnType(10),"TIMESTAMP");
assertEqStr(cur.getColumnType("TESTTIMESTAMP"),
	"TIMESTAMP");
assertEqStr(cur.getColumnType(11),"BLOB");
assertEqStr(cur.getColumnType("TESTBLOB"),"BLOB");
console.log();


// column length
console.log("COLUMN LENGTH: ");
assertEqInt(cur.getColumnLength(0),4);
assertEqInt(cur.getColumnLength("TESTINTEGER"),4);
assertEqInt(cur.getColumnLength(1),2);
assertEqInt(cur.getColumnLength("TESTSMALLINT"),2);
assertEqInt(cur.getColumnLength(2),8);
assertEqInt(cur.getColumnLength("TESTDECIMAL"),8);
assertEqInt(cur.getColumnLength(3),8);
assertEqInt(cur.getColumnLength("TESTNUMERIC"),8);
assertEqInt(cur.getColumnLength(4),4);
assertEqInt(cur.getColumnLength("TESTFLOAT"),4);
assertEqInt(cur.getColumnLength(5),8);
assertEqInt(cur.getColumnLength("TESTDOUBLE"),8);
assertEqInt(cur.getColumnLength(6),4);
assertEqInt(cur.getColumnLength("TESTDATE"),4);
assertEqInt(cur.getColumnLength(7),4);
assertEqInt(cur.getColumnLength("TESTTIME"),4);
assertEqInt(cur.getColumnLength(8),50);
assertEqInt(cur.getColumnLength("TESTCHAR"),50);
assertEqInt(cur.getColumnLength(9),50);
assertEqInt(cur.getColumnLength("TESTVARCHAR"),50);
assertEqInt(cur.getColumnLength(10),8);
assertEqInt(cur.getColumnLength("TESTTIMESTAMP"),8);
assertEqInt(cur.getColumnLength(11),8);
assertEqInt(cur.getColumnLength("TESTBLOB"),8);
console.log();


// longest column
console.log("LONGEST COLUMN: ");
assertEqInt(cur.getLongest(0),1);
assertEqInt(cur.getLongest("TESTINTEGER"),1);
assertEqInt(cur.getLongest(1),1);
assertEqInt(cur.getLongest("TESTSMALLINT"),1);
assertEqInt(cur.getLongest(2),4);
assertEqInt(cur.getLongest("TESTDECIMAL"),4);
assertEqInt(cur.getLongest(3),4);
assertEqInt(cur.getLongest("TESTNUMERIC"),4);
assertEqInt(cur.getLongest(4),6);
assertEqInt(cur.getLongest("TESTFLOAT"),6);
assertEqInt(cur.getLongest(5),6);
assertEqInt(cur.getLongest("TESTDOUBLE"),6);
assertEqInt(cur.getLongest(6),10);
assertEqInt(cur.getLongest("TESTDATE"),10);
assertEqInt(cur.getLongest(7),8);
assertEqInt(cur.getLongest("TESTTIME"),8);
assertEqInt(cur.getLongest(8),50);
assertEqInt(cur.getLongest("TESTCHAR"),50);
assertEqInt(cur.getLongest(9),12);
assertEqInt(cur.getLongest("TESTVARCHAR"),12);
assertEqInt(cur.getLongest(10),0);
assertEqInt(cur.getLongest("TESTTIMESTAMP"),0);
assertEqInt(cur.getLongest(11),9);
assertEqInt(cur.getLongest("TESTBLOB"),9);
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
assertEqStr(cur.getField(0,2),"1.50");
assertEqStr(cur.getField(0,3),"1.50");
assertEqStr(cur.getField(0,4),"1.5000");
assertEqStr(cur.getField(0,5),"1.5000");
assertEqStr(cur.getField(0,6),"2001:01:01");
assertEqStr(cur.getField(0,7),"01:00:00");
assertEqStr(cur.getField(0,8),"testchar1"+
	"                                         ");
assertEqStr(cur.getField(0,9),"testvarchar1");
assertEqStr(cur.getField(0,11),"testblob1");
console.log();
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(7,1),"8");
assertEqStr(cur.getField(7,2),"8.50");
assertEqStr(cur.getField(7,3),"8.50");
assertEqStr(cur.getField(7,4),"8.5000");
assertEqStr(cur.getField(7,5),"8.5000");
assertEqStr(cur.getField(7,6),"2008:01:01");
assertEqStr(cur.getField(7,7),"08:00:00");
assertEqStr(cur.getField(7,8),"testchar8"+
	"                                         ");
assertEqStr(cur.getField(7,9),"testvarchar8");
assertEqStr(cur.getField(7,11),"testblob8");
console.log();


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqInt(cur.getFieldLength(0,0),1);
assertEqInt(cur.getFieldLength(0,1),1);
assertEqInt(cur.getFieldLength(0,2),4);
assertEqInt(cur.getFieldLength(0,3),4);
assertEqInt(cur.getFieldLength(0,4),6);
assertEqInt(cur.getFieldLength(0,5),6);
assertEqInt(cur.getFieldLength(0,6),10);
assertEqInt(cur.getFieldLength(0,7),8);
assertEqInt(cur.getFieldLength(0,8),50);
assertEqInt(cur.getFieldLength(0,9),12);
console.log();
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),1);
assertEqInt(cur.getFieldLength(7,2),4);
assertEqInt(cur.getFieldLength(7,3),4);
assertEqInt(cur.getFieldLength(7,4),6);
assertEqInt(cur.getFieldLength(7,5),6);
assertEqInt(cur.getFieldLength(7,6),10);
assertEqInt(cur.getFieldLength(7,7),8);
assertEqInt(cur.getFieldLength(7,8),50);
assertEqInt(cur.getFieldLength(7,9),12);
console.log();


// fields by name
console.log("FIELDS BY NAME: ");
assertEqStr(cur.getField(0,"TESTINTEGER"),"1");
assertEqStr(cur.getField(0,"TESTSMALLINT"),"1");
assertEqStr(cur.getField(0,"TESTDECIMAL"),"1.50");
assertEqStr(cur.getField(0,"TESTNUMERIC"),"1.50");
assertEqStr(cur.getField(0,"TESTFLOAT"),"1.5000");
assertEqStr(cur.getField(0,"TESTDOUBLE"),"1.5000");
assertEqStr(cur.getField(0,"TESTDATE"),"2001:01:01");
assertEqStr(cur.getField(0,"TESTTIME"),"01:00:00");
assertEqStr(cur.getField(0,"TESTCHAR"),"testchar1"+
	"                                         ");
assertEqStr(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
assertEqStr(cur.getField(0,"TESTBLOB"),"testblob1");
console.log();
assertEqStr(cur.getField(7,"TESTINTEGER"),"8");
assertEqStr(cur.getField(7,"TESTSMALLINT"),"8");
assertEqStr(cur.getField(7,"TESTDECIMAL"),"8.50");
assertEqStr(cur.getField(7,"TESTNUMERIC"),"8.50");
assertEqStr(cur.getField(7,"TESTFLOAT"),"8.5000");
assertEqStr(cur.getField(7,"TESTDOUBLE"),"8.5000");
assertEqStr(cur.getField(7,"TESTDATE"),"2008:01:01");
assertEqStr(cur.getField(7,"TESTTIME"),"08:00:00");
assertEqStr(cur.getField(7,"TESTCHAR"),"testchar8"+
	"                                         ");
assertEqStr(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
assertEqStr(cur.getField(7,"TESTBLOB"),"testblob8");
console.log();


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqInt(cur.getFieldLength(0,"TESTINTEGER"),1);
assertEqInt(cur.getFieldLength(0,"TESTSMALLINT"),1);
assertEqInt(cur.getFieldLength(0,"TESTDECIMAL"),4);
assertEqInt(cur.getFieldLength(0,"TESTNUMERIC"),4);
assertEqInt(cur.getFieldLength(0,"TESTFLOAT"),6);
assertEqInt(cur.getFieldLength(0,"TESTDOUBLE"),6);
assertEqInt(cur.getFieldLength(0,"TESTDATE"),10);
assertEqInt(cur.getFieldLength(0,"TESTTIME"),8);
assertEqInt(cur.getFieldLength(0,"TESTCHAR"),50);
assertEqInt(cur.getFieldLength(0,"TESTVARCHAR"),12);
console.log();
assertEqInt(cur.getFieldLength(7,"TESTINTEGER"),1);
assertEqInt(cur.getFieldLength(7,"TESTSMALLINT"),1);
assertEqInt(cur.getFieldLength(7,"TESTDECIMAL"),4);
assertEqInt(cur.getFieldLength(7,"TESTNUMERIC"),4);
assertEqInt(cur.getFieldLength(7,"TESTFLOAT"),6);
assertEqInt(cur.getFieldLength(7,"TESTDOUBLE"),6);
assertEqInt(cur.getFieldLength(7,"TESTDATE"),10);
assertEqInt(cur.getFieldLength(7,"TESTTIME"),8);
assertEqInt(cur.getFieldLength(7,"TESTCHAR"),50);
assertEqInt(cur.getFieldLength(7,"TESTVARCHAR"),12);
console.log();


// fields by array
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqStr(fields[0],"1");
assertEqStr(fields[1],"1");
assertEqStr(fields[2],"1.50");
assertEqStr(fields[3],"1.50");
assertEqStr(fields[4],"1.5000");
assertEqStr(fields[5],"1.5000");
assertEqStr(fields[6],"2001:01:01");
assertEqStr(fields[7],"01:00:00");
assertEqStr(fields[8],"testchar1"+
	"                                         ");
assertEqStr(fields[9],"testvarchar1");
assertEqStr(fields[11],"testblob1");
console.log();


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqInt(fieldlens[0],1);
assertEqInt(fieldlens[1],1);
assertEqInt(fieldlens[2],4);
assertEqInt(fieldlens[3],4);
assertEqInt(fieldlens[4],6);
assertEqInt(fieldlens[5],6);
assertEqInt(fieldlens[6],10);
assertEqInt(fieldlens[7],8);
assertEqInt(fieldlens[8],50);
assertEqInt(fieldlens[9],12);
console.log();


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
	"	testinteger "));
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
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
	"	testinteger "));
assertEqStr(cur.getColumnName(0),"TESTINTEGER");
assertEqInt(cur.getColumnLength(0),4);
assertEqStr(cur.getColumnType(0),"INTEGER");
console.log();


// suspended session
console.log("SUSPENDED SESSION: ");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
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
cur.cacheToFile("cachefile1-firebird");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-firebird");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
console.log();


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqInt(cur.colCount(),12);
console.log();


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqStr(cur.getColumnName(0),"TESTINTEGER");
assertEqStr(cur.getColumnName(1),"TESTSMALLINT");
assertEqStr(cur.getColumnName(2),"TESTDECIMAL");
assertEqStr(cur.getColumnName(3),"TESTNUMERIC");
assertEqStr(cur.getColumnName(4),"TESTFLOAT");
assertEqStr(cur.getColumnName(5),"TESTDOUBLE");
assertEqStr(cur.getColumnName(6),"TESTDATE");
assertEqStr(cur.getColumnName(7),"TESTTIME");
assertEqStr(cur.getColumnName(8),"TESTCHAR");
assertEqStr(cur.getColumnName(9),"TESTVARCHAR");
assertEqStr(cur.getColumnName(10),"TESTTIMESTAMP");
assertEqStr(cur.getColumnName(11),"TESTBLOB");
cols=cur.getColumnNames();
assertEqStr(cols[0],"TESTINTEGER");
assertEqStr(cols[1],"TESTSMALLINT");
assertEqStr(cols[2],"TESTDECIMAL");
assertEqStr(cols[3],"TESTNUMERIC");
assertEqStr(cols[4],"TESTFLOAT");
assertEqStr(cols[5],"TESTDOUBLE");
assertEqStr(cols[6],"TESTDATE");
assertEqStr(cols[7],"TESTTIME");
assertEqStr(cols[8],"TESTCHAR");
assertEqStr(cols[9],"TESTVARCHAR");
assertEqStr(cols[10],"TESTTIMESTAMP");
assertEqStr(cols[11],"TESTBLOB");
console.log();


// cached result set with result set buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-firebird");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-firebird");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// from one cache file to another
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2-firebird");
assertTrue(cur.openCachedResultSet("cachefile1-firebird"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-firebird"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
console.log();


// from one cache file to another
// with result set buffer size
console.log("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET "+
		"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile2-firebird");
assertTrue(cur.openCachedResultSet("cachefile1-firebird"));
cur.cacheOff();
assertTrue(cur.openCachedResultSet("cachefile2-firebird"));
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// cached result set with suspend
// and result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET "+
		"BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1-firebird");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
assertEqStr(cur.getField(2,0),"3");
filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1-firebird");
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
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testinteger "));
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
var	secondcur=new sqlrelay.SQLRCursor(con);
secondcur.setResultSetBufferSize(1);
for (var i=0; cur.getRow(i); i++) {
	assertTrue(secondcur.sendQuery(
		"select * from testtable"));
}
// the nested selects must not disturb the outer result set
assertEqInt(i,cur.rowCount());
secondcur.closeResultSet();
cur.setResultSetBufferSize(0);
console.log();


// reset transaction state
console.log("RESET TRANSACTION STATE: ");
assertTrue(con.commit());
assertEqStr(con.getTransactionModel(),"implicit");
assertFalse(con.getAutoCommit());
console.log();


// transaction behavior - implicit
console.log("TRANSACTION BEHAVIOR - implicit: ");
assertTrue(con.setTransactionModel("implicit"));
assertEqStr(con.getTransactionModel(),"implicit");
// truncate testtable so this section starts with it empty;
// firebird DDL on the table here would otherwise hit cursor-state
// issues at the next commit, so we reuse the existing schema and
// just write to one column (testinteger)
assertTrue(cur.sendQuery("delete from testtable"));
// commit so the truncation is visible to the second connection
// (the commit implicitly starts a new tx)
assertTrue(con.commit());
var	secondcon=new sqlrelay.SQLRConnection("sqlrelay",9009,"/tmp/firebirdtest.socket",
			"testuser","testpassword",0,1);
var	secondcur=new sqlrelay.SQLRCursor(secondcon);
setSecondConnection(secondcon);
setSecondCursor(secondcur);
// session is in a transaction; insert is not visible until commit
assertTrue(con.getInTransaction());
assertFalse(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"0");
// commit makes it visible, and implicitly starts a new transaction
assertTrue(con.commit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// rollback discards, and implicitly starts a new transaction
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
assertTrue(con.rollback());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
secondcur.closeResultSet();
console.log();


// transaction behavior - explicit
console.log("TRANSACTION BEHAVIOR - explicit: ");
assertTrue(con.setTransactionModel("explicit"));
assertEqStr(con.getTransactionModel(),"explicit");
// truncate testtable so this section starts with it empty (delete
// autocommits here since explicit-model defaults to autocommit-on)
assertTrue(cur.sendQuery("delete from testtable"));
// begin starts a new transaction; insert is not visible until commit
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"0");
// commit makes it visible; no new transaction is started
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback discards; no new transaction is started
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
assertTrue(con.rollback());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// autoCommitOn takes effect immediately
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
secondcur.closeResultSet();
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
// truncate testtable so this section starts with it empty
assertTrue(cur.sendQuery("delete from testtable"));
// begin starts a transaction; commit makes it visible
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback discards
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// during a transaction started by begin(), autoCommitOn is a
// no-op: the autocommit setting takes effect after the user
// explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
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
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (4)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"3");
// autoCommitOff takes effect immediately when not in a transaction
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
// autocommit-off persists across commit/rollback; each commit or
// rollback ends the current implicit tx and a new one starts for
// the next statement
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (5)"));
assertTrue(con.commit());
assertFalse(con.getAutoCommit());
assertTrue(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"4");
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (6)"));
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
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (7)"));
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
console.log();


// transaction behavior - explicit-error
console.log("TRANSACTION BEHAVIOR - explicit-error: ");
assertTrue(con.setTransactionModel("explicit-error"));
assertEqStr(con.getTransactionModel(),"explicit-error");
// truncate testtable so this section starts with it empty
assertTrue(cur.sendQuery("delete from testtable"));
// begin, insert, commit
assertTrue(con.begin());
assertTrue(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
assertTrue(con.commit());
assertFalse(con.getInTransaction());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// begin, insert, rollback
assertTrue(con.begin());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
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
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autoCommitOff takes effect immediately
assertTrue(con.autoCommitOff());
assertFalse(con.getAutoCommit());
secondcur.closeResultSet();
console.log();


// transaction behavior - none
console.log("TRANSACTION BEHAVIOR - none: ");
assertTrue(con.setTransactionModel("none"));
assertEqStr(con.getTransactionModel(),"none");
// truncate testtable so this section starts with it empty
assertTrue(cur.sendQuery("delete from testtable"));
// no transactions; everything is visible immediately
assertTrue(con.getAutoCommit());
assertFalse(con.getInTransaction());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"1");
// commit and rollback are no-ops
assertTrue(con.commit());
assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
assertTrue(con.rollback());
assertTrue(secondcur.sendQuery("select count(*) from testtable"));
assertEqStr(secondcur.getField(0,0),"2");
// autocommit is always on; autoCommitOff is an error
assertFalse(con.autoCommitOff());
assertTrue(con.getAutoCommit());
assertTrue(con.autoCommitOn());
assertTrue(con.getAutoCommit());
secondcur.closeResultSet();
console.log();


// reset transaction behavior
console.log("RESET TRANSACTION BEHAVIOR: ");
assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
assertEqStr(con.getTransactionModel(),"implicit");
assertFalse(con.getAutoCommit());
console.log();


// individual substitutions
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),'$(var2)',$(var3) "+
	"from rdb$database");
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
cur.prepareQuery(
	"select "+
	"	'$(var1)', "+
	"	'$(var2)', "+
	"	'$(var3)' "+
	"from "+
	"	rdb$database ");
cur.substitutions(subvars,subvalstrings);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"hi");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"bye");
console.log();
cur.prepareQuery("select $(var1),$(var2),$(var3) "+
	"from rdb$database");
cur.substitutions(subvars,subvallongs);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"2");
assertEqStr(cur.getField(0,2),"3");
console.log();
cur.prepareQuery("select $(var1),$(var2),$(var3) "+
	"from rdb$database");
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"10.55");
assertEqStr(cur.getField(0,1),"10.556");
assertEqStr(cur.getField(0,2),"10.5556");
console.log();


// nulls as nulls
console.log("NULLS AS NULLS: ");
cur.getNullsAsNulls();
assertTrue(cur.sendQuery("select 1,null,null "+
	"from rdb$database"));
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),null);
assertEqStr(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("select 1,null,null "+
	"from rdb$database"));
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"");
assertEqStr(cur.getField(0,2),"");
console.log();


// null and empty lobs
console.log("NULL AND EMPTY LOBS: ");
cur.getNullsAsNulls();
cur.sendQuery("delete from testtable1");
cur.prepareQuery("insert into testtable1 values (?)");
cur.inputBindBlob("1","","".length);
assertTrue(cur.executeQuery());
cur.sendQuery("select testblob from testtable1");
assertEqStr(cur.getField(0,"TESTBLOB"),"");
cur.sendQuery("delete from testtable1");
cur.prepareQuery("insert into testtable1 values (?)");
cur.inputBindBlob("1",null,0);
assertTrue(cur.executeQuery());
cur.sendQuery("select testblob from testtable1");
assertEqStr(cur.getField(0,"TESTBLOB"),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("delete from testtable1"));
console.log();


// long lobs
console.log("LONG LOBS: ");
cur.sendQuery("delete from testtable1");
cur.prepareQuery("insert into testtable1 values (?)");
largebuffer="C".repeat(20*1024);
cur.inputBindClob("1",largebuffer,largebuffer.length);
assertTrue(cur.executeQuery());
cur.sendQuery("select testblob from testtable1");
assertEqInt(cur.getFieldLength(0,
				"TESTBLOB"),20*1024);
assertEqStr(cur.getField(0,"TESTBLOB"),largebuffer);
assertTrue(cur.sendQuery("delete from testtable1"));
console.log();


// output bind by position
console.log("OUTPUT BIND BY POSITION: ");
cur.getNullsAsNulls();
cur.prepareQuery("execute procedure testproc ?, ?, ?, ?");
cur.inputBind("1",1);
cur.inputBind("2",1.5,2,1);
cur.inputBind("3","hello");
cur.inputBindBlob("4","blob","blob".length);
cur.defineOutputBindInteger("1");
cur.defineOutputBindDouble("2");
cur.defineOutputBindString("3",20);
cur.defineOutputBindBlob("4");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("1"),1);
var d=cur.getOutputBindDouble("2");
assertEqDbl(d,1.5);
assertEqStr(cur.getOutputBindString("3"),
	"hello               ");
assertEqStr(cur.getOutputBindBlob("4"),"blob");
cur.getNullsAsEmptyStrings();
console.log();


// output bind by name
// firebird doesn't support bind by name


// output bind by name with validation
// firebird doesn't support bind by name


// lob output bind
console.log("LOB OUTPUT BIND: ");
cur.prepareQuery("execute procedure testproc1 ?");
cur.inputBindBlob("1","hello","hello".length);
cur.defineOutputBindBlob("1");
assertTrue(cur.executeQuery());
assertEqStrLen(cur.getOutputBindBlob("1"),"hello",5);
assertEqInt(cur.getOutputBindLength("1"),5);
console.log();


// long output bind
console.log("LONG OUTPUT BIND: ");
largebuffer="C".repeat(20*1024);
cur.prepareQuery("execute procedure testproc1 ?");
cur.inputBindBlob("1",largebuffer,largebuffer.length);
cur.defineOutputBindBlob("1");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindLength("1"),20*1024);
assertEqStrLen(cur.getOutputBindBlob("1"),largebuffer,
	20*1024);
console.log();


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.prepareQuery("select cast(? as integer) "+
	"from rdb$database");
cur.inputBind("1",-1);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"-1");
console.log();


// bind validation
// firebird doesn't support bind by name


// rebinding
console.log("REBINDING: ");
cur.prepareQuery("execute procedure testproc ?, ?, ?, ?");
cur.inputBind("1",1);
cur.inputBind("2",1.5,2,1);
cur.inputBind("3","hello");
cur.inputBindBlob("4","blob","blob".length);
cur.defineOutputBindInteger("1");
cur.defineOutputBindDouble("2");
cur.defineOutputBindString("3",20);
cur.defineOutputBindBlob("4");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("1"),1);
cur.inputBind("1",2);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("1"),2);
cur.inputBind("1",3);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("1"),3);
console.log();


// reexecute
console.log("REEXECUTE: ");
cur.prepareQuery("select 1 from rdb$database");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log();
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log();
cur.prepareQuery("select cast(? as int) from rdb$database");
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
cur.prepareQuery(
	"execute block (in1 int = ?, "+
	"	in2 double precision = ?, "+
	"	in3 varchar(20) = ?) as begin end");
cur.inputBind("1",1);
cur.inputBind("2",1.5,2,1);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
console.log();


// stored procedure returning single value
console.log("STORED PROCEDURE RETURNING SINGLE VALUE: ");
cur.prepareQuery(
	"execute block (in1 int = ?, "+
	"	in2 double precision = ?, "+
	"	in3 varchar(20) = ?) returns (out1 int) as "+
	"begin "+
	"	out1 = in1; "+
	"	suspend; end");
cur.inputBind("1",1);
cur.inputBind("2",1.5,2,1);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
console.log();


// stored procedure returning multiple values
console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
cur.prepareQuery(
	"execute block (in1 int = ?, "+
	"	in2 double precision = ?, "+
	"	in3 varchar(20) = ?) "+
	"returns (out1 int, "+
	"	out2 double precision, "+
	"	out3 varchar(20)) as "+
	"begin "+
	"	out1 = in1; "+
	"	out2 = in2; "+
	"	out3 = in3; "+
	"	suspend; end");
cur.inputBind("1",1);
cur.inputBind("2",1.5,2,1);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"1.5000");
assertEqStr(cur.getField(0,2),"hello");
console.log();


// stored procedure returning result set
console.log("STORED PROCEDURE RETURNING RESULT SET: ");
cur.prepareQuery("execute block returns (out1 int) as "+
	"declare i int; "+
	"begin "+
	"	i = 1; "+
	"	while (i <= 8) do "+
	"	begin "+
	"		out1 = i; "+
	"		suspend; "+
	"		i = i + 1; "+
	"	end end");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),8);
console.log();


// temporary tables
// firebird supports temporary tables,
// but we're omitting this for now


// encoded binary data
// firebird doesn't support encoded binary data


// quotes
console.log("QUOTES: ");
cur.sendQuery("delete from table testtable1");
assertTrue(cur.sendQuery("insert into testtable1 "+
	"values ('''''')"));
assertTrue(cur.sendQuery("select testblob from testtable1"));
assertEqInt(cur.getFieldLength(0,0),2);
assertTrue(cur.getField(0,0).substring(0,2)==="''");
assertTrue(cur.sendQuery("delete from testtable1"));
console.log();


// last insert id
// firebird doesn't support auto-increment


// database is schema
console.log("DATABASE IS SCHEMA: ");
assertFalse(con.getDatabaseIsSchema());
console.log();


// catalog list
console.log("CATALOG LIST: ");
assertTrue(cur.getCatalogList(null));
assertEqStr(cur.getColumnName(0),"Database");
assertEqInt(cur.rowCount(),0);
console.log();


// schema list
console.log("SCHEMA LIST: ");
assertTrue(cur.getSchemaList(null));
assertEqStr(cur.getColumnName(0),"Database");
// firebird has no schemas
assertEqInt(cur.rowCount(),0);
console.log();


// table type list
console.log("TABLE TYPE LIST: ");
assertTrue(cur.getTableTypeList());
assertEqStr(cur.getColumnName(0),"table_type");
assertInResultSet(cur,"table_type","TABLE");
console.log();


// table list
console.log("TABLE LIST: ");
assertTrue(cur.getTableList(null));
assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1");
assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2");
assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3");
console.log();


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
assertEqStr(cur.getField(0,"precision"),"32767");
assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
assertTrue(cur.getTypeInfoList("varchar"));
assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"32765");
assertEqStr(cur.getField(0,"local_type_name"),"VARCHAR");
assertTrue(cur.getTypeInfoList("date"));
assertEqStr(cur.getField(0,"type_name"),"DATE");
assertEqStr(cur.getField(0,"data_type"),"91");
assertEqStr(cur.getField(0,"precision"),"10");
assertEqStr(cur.getField(0,"local_type_name"),"DATE");
console.log();


// column list
console.log("COLUMN LIST: ");
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
assertTrue(cur.getField(0,"column_name")==="TESTINTEGER");
assertTrue(cur.getField(1,"column_name")==="TESTSMALLINT");
assertTrue(cur.getField(2,"column_name")==="TESTDECIMAL");
assertTrue(cur.getField(3,"column_name")==="TESTNUMERIC");
assertTrue(cur.getField(4,"column_name")==="TESTFLOAT");
assertTrue(cur.getField(5,"column_name")==="TESTDOUBLE");
assertTrue(cur.getField(6,"column_name")==="TESTDATE");
assertTrue(cur.getField(7,"column_name")==="TESTTIME");
assertTrue(cur.getField(8,"column_name")==="TESTCHAR");
assertTrue(cur.getField(9,"column_name")==="TESTVARCHAR");
assertTrue(cur.getField(10,"column_name")==="TESTTIMESTAMP");
assertTrue(cur.getField(11,"column_name")==="TESTBLOB");
assertTrue(cur.getField(0,"data_type")==="INTEGER");
assertTrue(cur.getField(1,"data_type")==="SMALLINT");
assertTrue(cur.getField(2,"data_type")==="DECIMAL");
assertTrue(cur.getField(3,"data_type")==="NUMERIC");
assertTrue(cur.getField(4,"data_type")==="FLOAT");
assertTrue(cur.getField(5,"data_type")==="DOUBLE PRECISION");
assertTrue(cur.getField(6,"data_type")==="DATE");
assertTrue(cur.getField(7,"data_type")==="TIME");
assertTrue(cur.getField(8,"data_type")==="CHAR");
assertTrue(cur.getField(9,"data_type")==="VARCHAR");
assertTrue(cur.getField(10,"data_type")==="TIMESTAMP");
assertTrue(cur.getField(11,"data_type")==="BLOB SUB_TYPE BINARY");
console.log();


// column list - auto_increment, primary key
console.log("COLUMN LIST - auto_increment, primary key: ");
assertTrue(cur.getColumnList("testtable2",null));
assertEqStr(cur.getField(0,"extra"),"auto_increment");
assertEqStr(cur.getField(0,"column_key"),"PRI");
assertEqStr(cur.getField(1,"extra"),"");
assertEqStr(cur.getField(1,"column_key"),"");
console.log();
assertTrue(cur.getColumnList("testtable3",null));
assertEqStr(cur.getField(0,"extra"),"");
assertEqStr(cur.getField(0,"column_key"),"PRI");
console.log();


// primary keys list
console.log("PRIMARY KEYS LIST: ");
assertTrue(cur.getPrimaryKeysList("testtable2",null));
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
assertTrue(cur.getField(0,"table")==="TESTTABLE2");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")==="COL1");
assertStartsWith(cur.getField(0,"key_name"),"INTEG_");
console.log();


// key and index list
console.log("KEY AND INDEX LIST: ");
assertTrue(cur.getKeyAndIndexList("testtable2",null));
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
assertTrue(cur.getField(0,"table")==="TESTTABLE2");
assertEqStr(cur.getField(0,"non_unique"),"0");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")==="COL1");
assertEqStr(cur.getField(0,"collation"),"A");
assertEqStr(cur.getField(0,"index_type"),"3");
assertStartsWith(cur.getField(0,"key_name"),"RDB$PRIMARY");
console.log();


// procedure list
console.log("PROCEDURE LIST: ");
assertTrue(cur.getProcedureList(null));
assertInResultSet(cur,"routine_name","TESTPROC");
assertInResultSet(cur,"routine_name","TESTPROC1");
console.log();


// procedure parameter list
console.log("PROCEDURE PARAMETER LIST: ");
assertTrue(cur.getProcedureParameterList("testproc",null));
assertEqStr(cur.getColumnName(0),"parameter_name");
assertEqStr(cur.getColumnName(1),"parameter_mode");
assertEqStr(cur.getColumnName(2),"data_type");
assertEqStr(cur.getColumnName(3),"character_maximum_length");
assertEqStr(cur.getColumnName(4),"ordinal_position");
assertEqInt(cur.rowCount(),8);
assertEqStr(cur.getField(0,"parameter_name"),"OUT1");
assertEqStr(cur.getField(0,"parameter_mode"),"4");
assertEqStr(cur.getField(0,"data_type"),"INTEGER");
assertEqStr(cur.getField(0,"ordinal_position"),"1");
assertEqStr(cur.getField(1,"parameter_name"),"OUT2");
assertEqStr(cur.getField(1,"parameter_mode"),"4");
assertEqStr(cur.getField(1,"data_type"),"FLOAT");
assertEqStr(cur.getField(1,"ordinal_position"),"2");
assertEqStr(cur.getField(2,"parameter_name"),"OUT3");
assertEqStr(cur.getField(2,"parameter_mode"),"4");
assertEqStr(cur.getField(2,"data_type"),"VARCHAR");
assertEqStr(cur.getField(2,"ordinal_position"),"3");
assertEqStr(cur.getField(3,"parameter_name"),"OUT4");
assertEqStr(cur.getField(3,"parameter_mode"),"4");
assertEqStr(cur.getField(3,"data_type"),
	"BLOB SUB_TYPE BINARY");
assertEqStr(cur.getField(3,"ordinal_position"),"4");
assertEqStr(cur.getField(4,"parameter_name"),"IN1");
assertEqStr(cur.getField(4,"parameter_mode"),"1");
assertEqStr(cur.getField(4,"data_type"),"INTEGER");
assertEqStr(cur.getField(4,"ordinal_position"),"1");
assertEqStr(cur.getField(5,"parameter_name"),"IN2");
assertEqStr(cur.getField(5,"parameter_mode"),"1");
assertEqStr(cur.getField(5,"data_type"),"FLOAT");
assertEqStr(cur.getField(5,"ordinal_position"),"2");
assertEqStr(cur.getField(6,"parameter_name"),"IN3");
assertEqStr(cur.getField(6,"parameter_mode"),"1");
assertEqStr(cur.getField(6,"data_type"),"VARCHAR");
assertEqStr(cur.getField(6,"ordinal_position"),"3");
assertEqStr(cur.getField(7,"parameter_name"),"IN4");
assertEqStr(cur.getField(7,"parameter_mode"),"1");
assertEqStr(cur.getField(7,"data_type"),
	"BLOB SUB_TYPE BINARY");
assertEqStr(cur.getField(7,"ordinal_position"),"4");
console.log();


// invalid queries
console.log("INVALID QUERIES: ");
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable1 "+
	"order by "+
	"	testinteger "));
console.log();
assertFalse(cur.sendQuery("insert into testtable1 "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable1 "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable1 "+
	"values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable1 "+
	"values (1,2,3,4)"));
console.log();
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
console.log();

reportTestStatus();

process.exit(getStatus());
