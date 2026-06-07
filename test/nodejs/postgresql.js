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


// instantiation
var con=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket","testuser",
			"testpassword",0,1);
setConnection(con);
var cur=new sqlrelay.SQLRCursor(con);
setCursor(cur);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"postgresql");
console.log("");


// ping
console.log("PING: ");
assertTrue(con.ping());
console.log("");


// transaction state
console.log("TRANSACTION STATE: ");
assertEqStr(con.getDefaultTransactionModel(),"explicit");
assertEqStr(con.getTransactionModel(),"explicit");
assertFalse(con.getInTransaction());
assertTrue(con.getAutoCommit());
console.log("");


// bind format
console.log("BIND FORMAT: ");
assertEqStr(con.bindFormat(),"$1");
console.log("");


// nextval format
console.log("NEXTVAL FORMAT: ");
assertEqStr(con.nextvalFormat(),"nextval('%s')");
console.log("");


// isolation levels
console.log("ISOLATION LEVELS: ");
var isolationlevels=["read committed","read uncommitted",
			"repeatable read","serializable"];
for (var i=0;i<isolationlevels.length;i++) {
	var il=isolationlevels[i];
	// postgresql requires the
	// isolation level to be the first
	// query of the transaction
	con.begin();
	assertTrue(con.setIsolationLevel(il));
	assertEqStr(con.getIsolationLevel(),il);
	con.commit();
	console.log("");
}
// reset to the default isolation level
con.begin();
assertTrue(con.setIsolationLevel(isolationlevels[0]));
con.commit();
console.log("");


// create testtable
console.log("CREATE TESTTABLE: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testfloat float, "+
	"	testreal real, "+
	"	testsmallint smallint, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testtext text, "+
	"	testbytea bytea)"));
console.log("");


// insert
console.log("INSERT: ");
assertTrue(con.begin());
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1.5, "+
	"	1.5, "+
	"	1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01/01/2001', "+
	"	'01:00:00', "+
	"	NULL, "+
	"	'testtext1', "+
	"	'testbytea1')"));
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	2, "+
	"	2.5, "+
	"	2.5, "+
	"	2, "+
	"	'testchar2', "+
	"	'testvarchar2', "+
	"	'01/01/2002', "+
	"	'02:00:00', "+
	"	NULL, "+
	"	'testtext2', "+
	"	'testbytea2')"));
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	3, "+
	"	3.5, "+
	"	3.5, "+
	"	3, "+
	"	'testchar3', "+
	"	'testvarchar3', "+
	"	'01/01/2003', "+
	"	'03:00:00', "+
	"	NULL, "+
	"	'testtext3', "+
	"	'testbytea3')"));
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	4, "+
	"	4.5, "+
	"	4.5, "+
	"	4, "+
	"	'testchar4', "+
	"	'testvarchar4', "+
	"	'01/01/2004', "+
	"	'04:00:00', "+
	"	NULL, "+
	"	'testtext4', "+
	"	'testbytea4')"));
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
	"	$1, "+
	"	$2, "+
	"	$3, "+
	"	$4, "+
	"	$5, "+
	"	$6, "+
	"	$7, "+
	"	$8, "+
	"	NULL, "+
	"	$9, "+
	"	$10)");
assertEqInt(cur.countBindVariables(),10);
cur.inputBind("1",5);
cur.inputBind("2",5.5,4,2);
cur.inputBind("3",5.5,4,2);
cur.inputBind("4",5);
cur.inputBind("5","testchar5");
cur.inputBind("6","testvarchar5");
cur.inputBind("7","01/01/2005");
cur.inputBind("8","05:00:00");
cur.inputBindClob("9","testtext5","testtext5".length);
cur.inputBindBlob("10","testbytea5","testbytea5".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",6);
cur.inputBind("2",6.5,4,2);
cur.inputBind("3",6.5,4,2);
cur.inputBind("4",6);
cur.inputBind("5","testchar6");
cur.inputBind("6","testvarchar6");
cur.inputBind("7","01/01/2006");
cur.inputBind("8","06:00:00");
cur.inputBindClob("9","testtext6","testtext6".length);
cur.inputBindBlob("10","testbytea6","testbytea6".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",7);
cur.inputBind("2",7.5,4,2);
cur.inputBind("3",7.5,4,2);
cur.inputBind("4",7);
cur.inputBind("5","testchar7");
cur.inputBind("6","testvarchar7");
cur.inputBind("7","01/01/2007");
cur.inputBind("8","07:00:00");
cur.inputBindClob("9","testtext7","testtext7".length);
cur.inputBindBlob("10","testbytea8","testbytea8".length);
assertTrue(cur.executeQuery());
console.log("");


// array of input binds by position
// postgresql doesn't support implicit
// conversion of string binds to other data
// types, so arrays of binds don't generally
// work.


// input bind by name
// postgresql doesn't support bind by name


// input bind by position with validation
console.log("BIND BY POSITION WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("1",8);
cur.inputBind("2",8.5,4,2);
cur.inputBind("3",8.5,4,2);
cur.inputBind("4",8);
cur.inputBind("5","testchar8");
cur.inputBind("6","testvarchar8");
cur.inputBind("7","01/01/2008");
cur.inputBind("8","08:00:00");
cur.inputBindClob("9","testtext8","testtext8".length);
cur.inputBindClob("10","testbytea8","testbytea8".length);
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log("");


// array of input binds by name
// postgresql doesn't support bind by name


// input bind by name with validation
// postgresql doesn't support bind by name


// select
console.log("SELECT: ");
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
console.log("");


// column count
console.log("COLUMN COUNT: ");
assertEqInt(cur.colCount(),11);
console.log("");


// column names
console.log("COLUMN NAMES: ");
assertEqStr(cur.getColumnName(0),"testint");
assertEqStr(cur.getColumnName(1),"testfloat");
assertEqStr(cur.getColumnName(2),"testreal");
assertEqStr(cur.getColumnName(3),"testsmallint");
assertEqStr(cur.getColumnName(4),"testchar");
assertEqStr(cur.getColumnName(5),"testvarchar");
assertEqStr(cur.getColumnName(6),"testdate");
assertEqStr(cur.getColumnName(7),"testtime");
assertEqStr(cur.getColumnName(8),"testtimestamp");
assertEqStr(cur.getColumnName(9),"testtext");
assertEqStr(cur.getColumnName(10),"testbytea");
var cols=cur.getColumnNames();
assertEqStr(cols[0],"testint");
assertEqStr(cols[1],"testfloat");
assertEqStr(cols[2],"testreal");
assertEqStr(cols[3],"testsmallint");
assertEqStr(cols[4],"testchar");
assertEqStr(cols[5],"testvarchar");
assertEqStr(cols[6],"testdate");
assertEqStr(cols[7],"testtime");
assertEqStr(cols[8],"testtimestamp");
assertEqStr(cols[9],"testtext");
assertEqStr(cols[10],"testbytea");
console.log("");


// column types
console.log("COLUMN TYPES: ");
assertEqStr(cur.getColumnType(0),"int4");
assertEqStr(cur.getColumnType("testint"),"int4");
assertEqStr(cur.getColumnType(1),"float8");
assertEqStr(cur.getColumnType("testfloat"),"float8");
assertEqStr(cur.getColumnType(2),"float4");
assertEqStr(cur.getColumnType("testreal"),"float4");
assertEqStr(cur.getColumnType(3),"int2");
assertEqStr(cur.getColumnType("testsmallint"),"int2");
assertEqStr(cur.getColumnType(4),"bpchar");
assertEqStr(cur.getColumnType("testchar"),"bpchar");
assertEqStr(cur.getColumnType(5),"varchar");
assertEqStr(cur.getColumnType("testvarchar"),"varchar");
assertEqStr(cur.getColumnType(6),"date");
assertEqStr(cur.getColumnType("testdate"),"date");
assertEqStr(cur.getColumnType(7),"time");
assertEqStr(cur.getColumnType("testtime"),"time");
assertEqStr(cur.getColumnType(8),"timestamp");
assertEqStr(cur.getColumnType("testtimestamp"),
	"timestamp");
assertEqStr(cur.getColumnType(9),"text");
assertEqStr(cur.getColumnType("testtext"),"text");
assertEqStr(cur.getColumnType(10),"bytea");
assertEqStr(cur.getColumnType("testbytea"),"bytea");
console.log("");


// column length
console.log("COLUMN LENGTH: ");
assertEqInt(cur.getColumnLength(0),4);
assertEqInt(cur.getColumnLength("testint"),4);
assertEqInt(cur.getColumnLength(1),8);
assertEqInt(cur.getColumnLength("testfloat"),8);
assertEqInt(cur.getColumnLength(2),4);
assertEqInt(cur.getColumnLength("testreal"),4);
assertEqInt(cur.getColumnLength(3),2);
assertEqInt(cur.getColumnLength("testsmallint"),2);
assertEqInt(cur.getColumnLength(4),40);
assertEqInt(cur.getColumnLength("testchar"),40);
assertEqInt(cur.getColumnLength(5),40);
assertEqInt(cur.getColumnLength("testvarchar"),40);
assertEqInt(cur.getColumnLength(6),4);
assertEqInt(cur.getColumnLength("testdate"),4);
assertEqInt(cur.getColumnLength(7),8);
assertEqInt(cur.getColumnLength("testtime"),8);
assertEqInt(cur.getColumnLength(8),8);
assertEqInt(cur.getColumnLength("testtimestamp"),8);
assertEqInt(cur.getColumnLength(9),0);
assertEqInt(cur.getColumnLength("testtext"),0);
assertEqInt(cur.getColumnLength(10),0);
assertEqInt(cur.getColumnLength("testbytea"),0);
console.log("");


// longest column
console.log("LONGEST COLUMN: ");
assertEqInt(cur.getLongest(0),1);
assertEqInt(cur.getLongest("testint"),1);
assertEqInt(cur.getLongest(1),3);
assertEqInt(cur.getLongest("testfloat"),3);
assertEqInt(cur.getLongest(2),3);
assertEqInt(cur.getLongest("testreal"),3);
assertEqInt(cur.getLongest(3),1);
assertEqInt(cur.getLongest("testsmallint"),1);
assertEqInt(cur.getLongest(4),40);
assertEqInt(cur.getLongest("testchar"),40);
assertEqInt(cur.getLongest(5),12);
assertEqInt(cur.getLongest("testvarchar"),12);
assertEqInt(cur.getLongest(6),10);
assertEqInt(cur.getLongest("testdate"),10);
assertEqInt(cur.getLongest(7),8);
assertEqInt(cur.getLongest("testtime"),8);
assertEqInt(cur.getLongest(9),9);
assertEqInt(cur.getLongest("testtext"),9);
assertEqInt(cur.getLongest(10),10);
assertEqInt(cur.getLongest("testbytea"),10);
console.log("");


// row count
console.log("ROW COUNT: ");
assertEqInt(cur.rowCount(),8);
console.log("");


// total rows
console.log("TOTAL ROWS: ");
assertEqInt(cur.totalRows(),8);
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
assertEqStr(cur.getField(0,1),"1.5");
assertEqStr(cur.getField(0,2),"1.5");
assertEqStr(cur.getField(0,3),"1");
assertEqStr(cur.getField(0,4),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,5),"testvarchar1");
assertEqStr(cur.getField(0,6),"2001-01-01");
assertEqStr(cur.getField(0,7),"01:00:00");
assertEqStr(cur.getField(0,9),"testtext1");
assertEqStr(cur.getField(0,10),"testbytea1");
console.log("");
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(7,1),"8.5");
assertEqStr(cur.getField(7,2),"8.5");
assertEqStr(cur.getField(7,3),"8");
assertEqStr(cur.getField(7,4),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,5),"testvarchar8");
assertEqStr(cur.getField(7,6),"2008-01-01");
assertEqStr(cur.getField(7,7),"08:00:00");
assertEqStr(cur.getField(7,9),"testtext8");
assertEqStr(cur.getField(7,10),"testbytea8");
console.log("");


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqInt(cur.getFieldLength(0,0),1);
assertEqInt(cur.getFieldLength(0,1),3);
assertEqInt(cur.getFieldLength(0,2),3);
assertEqInt(cur.getFieldLength(0,3),1);
assertEqInt(cur.getFieldLength(0,4),40);
assertEqInt(cur.getFieldLength(0,5),12);
assertEqInt(cur.getFieldLength(0,6),10);
assertEqInt(cur.getFieldLength(0,7),8);
assertEqInt(cur.getFieldLength(0,9),9);
assertEqInt(cur.getFieldLength(0,10),10);
console.log("");
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),3);
assertEqInt(cur.getFieldLength(7,2),3);
assertEqInt(cur.getFieldLength(7,3),1);
assertEqInt(cur.getFieldLength(7,4),40);
assertEqInt(cur.getFieldLength(7,5),12);
assertEqInt(cur.getFieldLength(7,6),10);
assertEqInt(cur.getFieldLength(7,7),8);
assertEqInt(cur.getFieldLength(7,9),9);
assertEqInt(cur.getFieldLength(7,10),10);
console.log("");


// fields by name
console.log("FIELDS BY NAME: ");
assertEqStr(cur.getField(0,"testint"),"1");
assertEqStr(cur.getField(0,"testfloat"),"1.5");
assertEqStr(cur.getField(0,"testreal"),"1.5");
assertEqStr(cur.getField(0,"testsmallint"),"1");
assertEqStr(cur.getField(0,"testchar"),"testchar1"+
	"                               ");
assertEqStr(cur.getField(0,"testvarchar"),"testvarchar1");
assertEqStr(cur.getField(0,"testdate"),"2001-01-01");
assertEqStr(cur.getField(0,"testtime"),"01:00:00");
assertEqStr(cur.getField(0,"testtext"),"testtext1");
assertEqStr(cur.getField(0,"testbytea"),"testbytea1");
console.log("");
assertEqStr(cur.getField(7,"testint"),"8");
assertEqStr(cur.getField(7,"testfloat"),"8.5");
assertEqStr(cur.getField(7,"testreal"),"8.5");
assertEqStr(cur.getField(7,"testsmallint"),"8");
assertEqStr(cur.getField(7,"testchar"),"testchar8"+
	"                               ");
assertEqStr(cur.getField(7,"testvarchar"),"testvarchar8");
assertEqStr(cur.getField(7,"testdate"),"2008-01-01");
assertEqStr(cur.getField(7,"testtime"),"08:00:00");
assertEqStr(cur.getField(7,"testtext"),"testtext8");
assertEqStr(cur.getField(7,"testbytea"),"testbytea8");
console.log("");


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqInt(cur.getFieldLength(0,"testint"),1);
assertEqInt(cur.getFieldLength(0,"testfloat"),3);
assertEqInt(cur.getFieldLength(0,"testreal"),3);
assertEqInt(cur.getFieldLength(0,"testsmallint"),1);
assertEqInt(cur.getFieldLength(0,"testchar"),40);
assertEqInt(cur.getFieldLength(0,"testvarchar"),12);
assertEqInt(cur.getFieldLength(0,"testdate"),10);
assertEqInt(cur.getFieldLength(0,"testtime"),8);
assertEqInt(cur.getFieldLength(0,"testtext"),9);
assertEqInt(cur.getFieldLength(0,"testbytea"),10);
console.log("");
assertEqInt(cur.getFieldLength(7,"testint"),1);
assertEqInt(cur.getFieldLength(7,"testfloat"),3);
assertEqInt(cur.getFieldLength(7,"testreal"),3);
assertEqInt(cur.getFieldLength(7,"testsmallint"),1);
assertEqInt(cur.getFieldLength(7,"testchar"),40);
assertEqInt(cur.getFieldLength(7,"testvarchar"),12);
assertEqInt(cur.getFieldLength(7,"testdate"),10);
assertEqInt(cur.getFieldLength(7,"testtime"),8);
assertEqInt(cur.getFieldLength(7,"testtext"),9);
assertEqInt(cur.getFieldLength(7,"testbytea"),10);
console.log("");


// fields by array
console.log("FIELDS BY ARRAY: ");
var fields=cur.getRow(0);
assertEqStr(fields[0],"1");
assertEqStr(fields[1],"1.5");
assertEqStr(fields[2],"1.5");
assertEqStr(fields[3],"1");
assertEqStr(fields[4],"testchar1"+
	"                               ");
assertEqStr(fields[5],"testvarchar1");
assertEqStr(fields[6],"2001-01-01");
assertEqStr(fields[7],"01:00:00");
assertEqStr(fields[9],"testtext1");
assertEqStr(fields[10],"testbytea1");
console.log("");


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
var fieldlens=cur.getRowLengths(0);
assertEqInt(fieldlens[0],1);
assertEqInt(fieldlens[1],3);
assertEqInt(fieldlens[2],3);
assertEqInt(fieldlens[3],1);
assertEqInt(fieldlens[4],40);
assertEqInt(fieldlens[5],12);
assertEqInt(fieldlens[6],10);
assertEqInt(fieldlens[7],8);
assertEqInt(fieldlens[9],9);
assertEqInt(fieldlens[10],10);
console.log("");


// result set buffer size
console.log("RESULT SET BUFFER SIZE: ");
assertEqInt(cur.getResultSetBufferSize(),0);
cur.setResultSetBufferSize(2);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
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
assertEqStr(cur.getColumnType(0),"int4");
console.log("");


// suspended session
console.log("SUSPENDED SESSION: ");
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
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
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
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
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
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


// suspended result set
console.log("SUSPENDED RESULT SET: ");
cur.setResultSetBufferSize(2);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(2,0),"3");
var id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
var port=con.getConnectionPort();
var socket=con.getConnectionSocket();
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
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
var filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
console.log("");


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqInt(cur.colCount(),11);
console.log("");


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqStr(cur.getColumnName(0),"testint");
assertEqStr(cur.getColumnName(1),"testfloat");
assertEqStr(cur.getColumnName(2),"testreal");
assertEqStr(cur.getColumnName(3),"testsmallint");
assertEqStr(cur.getColumnName(4),"testchar");
assertEqStr(cur.getColumnName(5),"testvarchar");
assertEqStr(cur.getColumnName(6),"testdate");
assertEqStr(cur.getColumnName(7),"testtime");
assertEqStr(cur.getColumnName(8),"testtimestamp");
var cols=cur.getColumnNames();
assertEqStr(cols[0],"testint");
assertEqStr(cols[1],"testfloat");
assertEqStr(cols[2],"testreal");
assertEqStr(cols[3],"testsmallint");
assertEqStr(cols[4],"testchar");
assertEqStr(cols[5],"testvarchar");
assertEqStr(cols[6],"testdate");
assertEqStr(cols[7],"testtime");
assertEqStr(cols[8],"testtimestamp");
console.log("");


// cached result set with result set
// buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
var filename=cur.getCacheFileName();
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
console.log("FROM ONE CACHE FILE TO ANOTHER "+
	"WITH RESULT SET BUFFER SIZE: ");
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
console.log("CACHED RESULT SET WITH SUSPEND "+
	"AND RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(2,0),"3");
var filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
var id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
var port=con.getConnectionPort();
var socket=con.getConnectionSocket();
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
assertTrue(cur.sendQuery("select * from testtable "+
	"order by testint"));
assertEqStr(cur.getField(4,0),"5");
assertEqStr(cur.getField(5,0),"6");
assertEqStr(cur.getField(6,0),"7");
assertEqStr(cur.getField(7,0),"8");
var id=cur.getResultSetId();
cur.suspendResultSet();
assertTrue(con.suspendSession());
var port=con.getConnectionPort();
var socket=con.getConnectionSocket();
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
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(con.getAutoCommit());
console.log("");


// transaction behavior - implicit
console.log("TRANSACTION BEHAVIOR - implicit: ");
assertTrue(con.setTransactionModel("implicit"));
assertEqStr(con.getTransactionModel(),"implicit");
assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
// postgresql DDL is transactional; commit so the table is visible
// to the second connection (the commit implicitly starts a new tx)
assertTrue(con.commit());
var secondcon=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket","testuser",
			"testpassword",0,1);
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
assertEqStr(con.getTransactionModel(),"explicit");
assertTrue(con.getAutoCommit());
console.log("");


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
console.log("");


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
var subvars=["var1","var2","var3"];
cur.prepareQuery("select $(var1),$(var2),$(var3)");
var subvallongs=[1,2,3];
cur.substitutions(subvars,subvallongs);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"2");
assertEqStr(cur.getField(0,2),"3");
console.log("");
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
var subvalstrings=["hi","hello","bye"];
cur.substitutions(subvars,subvalstrings);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"hi");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"bye");
console.log("");
cur.prepareQuery("select $(var1),$(var2),$(var3)");
var subvaldoubles=[10.55,10.556,10.5556];
var precs=[4,5,6];
var scales=[2,3,4];
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"10.55");
assertEqStr(cur.getField(0,1),"10.556");
assertEqStr(cur.getField(0,2),"10.5556");
console.log("");


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
console.log("");


// null and empty lobs
console.log("NULL AND EMPTY LOBS: ");
cur.getNullsAsNulls();
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob1 text, "+
	"	testclob2 text, "+
	"	testblob1 bytea, "+
	"	testblob2 bytea)"));
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	$1, "+
	"	$2, "+
	"	$3, "+
	"	$4)");
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
console.log("");


// long lobs
console.log("LONG LOBS: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(
	"create table testtable ("+
	"	testtext text, "+
	"	testbytea bytea)");
cur.prepareQuery("insert into testtable values ($1,$2)");
var largebuffer="";
for (var i=0; i<8192; i++) {
	largebuffer=largebuffer+"C";
}
cur.inputBindClob("1",largebuffer,largebuffer.length);
cur.inputBindBlob("2",largebuffer,largebuffer.length);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
assertEqInt(cur.getFieldLength(0,
				"testtext"),8192);
assertEqStr(cur.getField(0,"testtext"),largebuffer);
assertEqInt(cur.getFieldLength(0,
				"testbytea"),8192);
assertEqStrLen(cur.getField(0,"testbytea"),largebuffer,
	8192);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// output bind by position
// postgresql doesn't support output binds


// output bind by name
// postgresql doesn't support output binds


// output bind by name with validation
// postgresql doesn't support output binds


// lob output bind
// postgresql doesn't support output binds


// long output bind
// postgresql doesn't support output binds


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery("create table testtable (testval int)");
cur.prepareQuery("insert into testtable values ($1)");
cur.inputBind("1",-1);
assertTrue(cur.executeQuery());
cur.sendQuery("select testval from testtable");
assertEqStr(cur.getField(0,"testval"),"-1");
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// bind validation
// postgresql doesn't support bind by name


// rebinding
console.log("REBINDING: ");
cur.sendQuery("drop function testfunc(int)");
assertTrue(cur.sendQuery("create function testfunc(int) "+
	"returns int as "+
	"	' begin return $1; end;' language plpgsql"));
cur.prepareQuery("select * from testfunc($1)");
cur.inputBind("1",1);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
cur.inputBind("1",2);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"2");
cur.inputBind("1",3);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"3");
assertTrue(cur.sendQuery("drop function testfunc(int)"));
console.log("");


// reexecute
console.log("REEXECUTE: ");
cur.prepareQuery("select 1");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
cur.prepareQuery("select $1::int");
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
cur.sendQuery("drop function testfunc(int,float,char(20))");
assertTrue(cur.sendQuery(
	"create function testfunc("+
	"	int,float,char(20)) "+
	"returns void as ' "+
	"	declare in1 int; "+
	"	in2 float; "+
	"	in3 char(20); "+
	"begin "+
	"	in1:=$1; "+
	"	in2:=$2; "+
	"	in3:=$3; "+
	"	return; end;' language plpgsql"));
cur.prepareQuery("select testfunc($1,$2,$3)");
cur.inputBind("1",1);
cur.inputBind("2",1.5,4,2);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop function "+
	"testfunc(int,float,char(20))"));
console.log("");


// stored procedure returning single value
console.log("STORED PROCEDURE RETURNING SINGLE VALUE: ");
cur.sendQuery("drop function testfunc(int,float,char(20))");
assertTrue(cur.sendQuery("create function "+
	"testfunc(int,float,char(20)) "+
	"returns int as "+
	"	' begin return $1; end;' language plpgsql"));
cur.prepareQuery("select * from testfunc($1,$2,$3)");
cur.inputBind("1",1);
cur.inputBind("2",1.5,4,2);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertTrue(cur.sendQuery("drop function "+
	"testfunc(int,float,char(20))"));
console.log("");


// stored procedure returning
// multiple values
console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
cur.sendQuery("drop function testfunc(int,float,char(20))");
assertTrue(cur.sendQuery(
	"create function testfunc("+
	"	int,float,char(20)) "+
	"returns record as ' "+
	"	declare output record; "+
	"begin "+
	"	select $1,$2,$3 "+
	"	into output; "+
	"	return output; end;' language plpgsql"));
cur.prepareQuery(
	"select "+
	"	* "+
	"from "+
	"	testfunc($1,$2,$3) "+
	"	as (col1 int, "+
	"		col2 float, "+
	"		col3 bpchar) ");
cur.inputBind("1",1);
cur.inputBind("2",1.5,4,2);
cur.inputBind("3","hello");
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,2),"hello");
assertTrue(cur.sendQuery("drop function "+
	"testfunc(int,float,char(20))"));
console.log("");


// stored procedure returning result set
console.log("STORED PROCEDURE RETURNING RESULT SET: ");
cur.sendQuery("drop function testfunc()");
assertTrue(cur.sendQuery("create function testfunc() "+
	"returns setof record as ' "+
	"	declare output record; "+
	"begin "+
	"	for output in "+
	"		select 1 "+
	"		union "+
	"		select 2 "+
	"		union "+
	"		select 3 "+
	"		union "+
	"		select 4 "+
	"		union "+
	"		select 5 "+
	"		union "+
	"		select 6 "+
	"		union "+
	"		select 7 "+
	"		union "+
	"		select 8 "+
	"	loop "+
	"		return next output; "+
	"	end loop; "+
	"	return; end;' language plpgsql"));
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testfunc() "+
	"	as (testint int)"));
assertEqInt(cur.rowCount(),8);
assertTrue(cur.sendQuery("drop function testfunc()"));
console.log("");


// temporary tables
console.log("TEMPORARY TABLES: ");
cur.sendQuery("drop table temptable\n");
cur.sendQuery("create temporary table temptable (col1 int)");
assertTrue(cur.sendQuery("insert into temptable values (1)"));
assertTrue(cur.sendQuery("select count(*) from temptable"));
assertEqStr(cur.getField(0,0),"1");
con.endSession();
console.log("");
assertFalse(cur.sendQuery("select count(*) from temptable"));
console.log("");


// encoded binary data
console.log("ENCODED BINARY DATA: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable "+
	"(col1 bytea)"));
var buffer="";
for (var i=0; i<256; i++) {
	buffer=buffer+String.fromCharCode(i);
}
var querystr="insert into testtable values (decode('";
for (var i=0; i<buffer.length; i++) {
	querystr=querystr+("0"+buffer.charCodeAt(i).toString(16)).slice(-2);
}
querystr=querystr+"','hex'))";
assertTrue(cur.sendQuery(querystr));
// Verify round-tripped bytes via server-side encode/hex (the binding's
// getField returns strings via String::NewFromUtf8, which drops
// invalid UTF-8 byte sequences that arise from raw bytes 128-255).
assertTrue(cur.sendQuery(
	"select encode(col1,'hex') from testtable"));
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
assertEqInt((cur.getField(0,0)=="''")?0:1,0);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// last insert id
console.log("LAST INSERT ID: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable "+
	"	(col1 serial primary key, "+
	"	col2 int)"));
assertTrue(cur.sendQuery("insert into testtable "+
	"(col2) values (1)"));
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
assertTrue(cur.rowCount()>0);
console.log("");


// schema list
console.log("SCHEMA LIST: ");
assertTrue(cur.getSchemaList(null));
assertEqStr(cur.getColumnName(0),"Database");
assertTrue(cur.rowCount()>0);
console.log("");


// table type list
console.log("TABLE TYPE LIST: ");
assertTrue(cur.getTableTypeList());
assertEqStr(cur.getColumnName(0),"table_type");
var found=0;
for (var i=0; i<cur.rowCount(); i++) {
	if (cur.getField(i,"table_type")==
		"TABLE") {
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
var counter=0;
for (var i=0; i<cur.rowCount(); i++) {
	var name=cur.getField(i,"Tables_in_xxx");
	if (name=="testtable1" || name=="testtable2" ||
		name=="testtable3" ||
		name=="testtable4") {
		counter++;
	}
}
assertEqInt(counter,4);
assertTrue(cur.sendQuery("drop table testtable1"));
assertTrue(cur.sendQuery("drop table testtable2"));
assertTrue(cur.sendQuery("drop table testtable3"));
assertTrue(cur.sendQuery("drop table testtable4"));
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
assertEqStr(cur.getField(0,"precision"),"255");
assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
assertTrue(cur.getTypeInfoList("varchar"));
assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"255");
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
	"	testint int, "+
	"	testfloat float, "+
	"	testreal real, "+
	"	testsmallint smallint, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp, "+
	"	testtext text, "+
	"	testbytea bytea)"));
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
assertEqStr(cur.getField(0,"column_name"),"testint");
assertEqStr(cur.getField(1,"column_name"),"testfloat");
assertEqStr(cur.getField(2,"column_name"),"testreal");
assertEqStr(cur.getField(3,"column_name"),"testsmallint");
assertEqStr(cur.getField(4,"column_name"),"testchar");
assertEqStr(cur.getField(5,"column_name"),"testvarchar");
assertEqStr(cur.getField(6,"column_name"),"testdate");
assertEqStr(cur.getField(7,"column_name"),"testtime");
assertEqStr(cur.getField(8,"column_name"),
	"testtimestamp");
assertEqStr(cur.getField(9,"column_name"),"testtext");
assertEqStr(cur.getField(10,"column_name"),"testbytea");
assertEqStr(cur.getField(0,"data_type"),"integer");
assertEqStr(cur.getField(1,"data_type"),
	"double precision");
assertEqStr(cur.getField(2,"data_type"),"real");
assertEqStr(cur.getField(3,"data_type"),"smallint");
assertEqStr(cur.getField(4,"data_type"),"character");
assertEqStr(cur.getField(5,"data_type"),
	"character varying");
assertEqStr(cur.getField(6,"data_type"),"date");
assertEqStr(cur.getField(7,"data_type"),
	"time without time zone");
assertEqStr(cur.getField(8,"data_type"),
	"timestamp without time zone");
assertEqStr(cur.getField(9,"data_type"),"text");
assertEqStr(cur.getField(10,"data_type"),"bytea");
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// column list - auto_increment, primary key
console.log("COLUMN LIST - auto_increment, primary key: ");
cur.sendQuery("drop table if exists testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 serial primary key, "+
	"	col2 int)"));
assertTrue(cur.getColumnList("testtable",null));
assertTrue(cur.getField(0,"extra").indexOf(
	"auto_increment")!=-1);
assertTrue(cur.getField(0,"column_key").indexOf(
	"PRI")!=-1);
assertFalse(cur.getField(1,"extra").indexOf(
	"auto_increment")!=-1);
assertFalse(cur.getField(1,"column_key").indexOf(
	"PRI")!=-1);
console.log("");
assertTrue(cur.sendQuery("drop table testtable"));
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 int primary key, "+
	"	col2 int)"));
assertTrue(cur.getColumnList("testtable",null));
assertFalse(cur.getField(0,"extra").indexOf(
	"auto_increment")!=-1);
assertTrue(cur.getField(0,"column_key").indexOf(
	"PRI")!=-1);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


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
assertTrue(cur.getField(0,"table")=="testtable");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")=="col1");
var keyname=cur.getField(0,"key_name");
assertTrue(keyname && keyname[0]);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


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
assertTrue(cur.getField(0,"table")=="testtable");
assertEqStr(cur.getField(0,"non_unique"),"f");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertTrue(cur.getField(0,"column_name")=="col1");
assertEqStr(cur.getField(0,"collation"),"A");
assertEqStr(cur.getField(0,"index_type"),"3");
var keyname=cur.getField(0,"key_name");
assertTrue(keyname && keyname[0]);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// procedure list
console.log("PROCEDURE LIST: ");
cur.sendQuery("drop function testproc1(int,char,"+
	"varchar,date)");
cur.sendQuery("drop function testproc2(int,char,"+
	"varchar,date)");
cur.sendQuery("drop function testproc3(int,char,"+
	"varchar,date)");
cur.sendQuery("drop function testproc4(int,char,"+
	"varchar,date)");
assertTrue(cur.sendQuery(
	"create function testproc1("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) returns void as 'begin end;' "+
	"language plpgsql"));
assertTrue(cur.sendQuery(
	"create function testproc2("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) returns void as 'begin end;' "+
	"language plpgsql"));
assertTrue(cur.sendQuery(
	"create function testproc3("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) returns void as 'begin end;' "+
	"language plpgsql"));
assertTrue(cur.sendQuery(
	"create function testproc4("+
	"	in1 int, "+
	"	in2 char(20), "+
	"	in3 varchar(20), "+
	"	in4 date) returns void as 'begin end;' "+
	"language plpgsql"));
assertTrue(cur.getProcedureList(null));
var counter=0;
for (var i=0; i<cur.rowCount(); i++) {
	var name=cur.getField(i,"routine_name");
	if (name=="testproc1" || name=="testproc2" ||
		name=="testproc3" || name=="testproc4") {
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
assertEqStr(cur.getField(0,"parameter_name"),"in1");
assertEqStr(cur.getField(0,"parameter_mode"),"1");
assertEqStr(cur.getField(0,"data_type"),"integer");
assertEqStr(cur.getField(0,"ordinal_position"),"1");
assertEqStr(cur.getField(1,"parameter_name"),"in2");
assertEqStr(cur.getField(1,"parameter_mode"),"1");
assertEqStr(cur.getField(1,"data_type"),"character");
assertEqStr(cur.getField(1,"ordinal_position"),"2");
assertEqStr(cur.getField(2,"parameter_name"),"in3");
assertEqStr(cur.getField(2,"parameter_mode"),"1");
assertEqStr(cur.getField(2,"data_type"),
	"character varying");
assertEqStr(cur.getField(2,"ordinal_position"),"3");
assertEqStr(cur.getField(3,"parameter_name"),"in4");
assertEqStr(cur.getField(3,"parameter_mode"),"1");
assertEqStr(cur.getField(3,"data_type"),"date");
assertEqStr(cur.getField(3,"ordinal_position"),"4");
assertTrue(cur.sendQuery("drop function testproc1(int,char,"+
	"varchar,date)"));
assertTrue(cur.sendQuery("drop function testproc2(int,char,"+
	"varchar,date)"));
assertTrue(cur.sendQuery("drop function testproc3(int,char,"+
	"varchar,date)"));
assertTrue(cur.sendQuery("drop function testproc4(int,char,"+
	"varchar,date)"));
console.log("");


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
