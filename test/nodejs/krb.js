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

// hostname
var hostname=require("os").hostname();
var dot=hostname.indexOf(".");
if (dot>0) {
	hostname=hostname.substring(0,dot);
}


// instantiation
var con=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket",
					null,null,0,1);
setConnection(con);
var cur=new sqlrelay.SQLRCursor(con);
setCursor(cur);
con.enableKerberos(null,null,null);


// identify
console.log("IDENTIFY: ");
assertEqStr(con.identify(),"oracle");
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
assertEqStr(con.bindFormat(),":*");
console.log("");


// nextval format
console.log("NEXTVAL FORMAT: ");
assertEqStr(con.nextvalFormat(),"%s.nextval");
console.log("");


// isolation levels
console.log("ISOLATION LEVELS: ");
var isolationlevels=["READ COMMITTED","SERIALIZABLE"];
for (var i=0; i<isolationlevels.length; i++) {
	var il=isolationlevels[i];
	// oracle requires the isolation level to
	// be the first query of the transaction
	assertTrue(con.commit());
	// you can set the isolation level, but to get it, you have to
	// have permisisons to read from sys.v_$session and
	// sys.v_$transaction
	assertTrue(con.setIsolationLevel(il));
	console.log("");
}
// reset to the default isolation level
assertTrue(con.commit());
assertTrue(con.setIsolationLevel(isolationlevels[0]));
console.log("");


// create testtable
console.log("CREATE TESTTABLE: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"));
console.log("");


// insert
console.log("INSERT: ");
assertTrue(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01-JAN-2001', "+
	"	'testlong1', "+
	"	'testclob1', "+
	"	empty_blob())"));
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
	"	:var1, "+
	"	:var2, "+
	"	:var3, "+
	"	:var4, "+
	"	:var5, "+
	"	:var6, "+
	"	:var7)");
assertEqInt(cur.countBindVariables(),7);
cur.inputBind("1",2);
cur.inputBind("2","testchar2");
cur.inputBind("3","testvarchar2");
cur.inputBind("4",2002,1,1,0,0,0,0,null,0);
cur.inputBind("5","testlong2");
cur.inputBindClob("6","testclob2","testclob2".length);
cur.inputBindBlob("7","testblob2","testblob2".length);
assertTrue(cur.executeQuery());
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2","testchar3");
cur.inputBind("3","testvarchar3");
cur.inputBind("4",2003,1,1,0,0,0,0,null,0);
cur.inputBind("5","testlong3");
cur.inputBindClob("6","testclob3","testclob3".length);
cur.inputBindBlob("7","testblob3","testblob3".length);
assertTrue(cur.executeQuery());
console.log("");


// array of input binds by position
console.log("ARRAY OF INPUT BINDS BY POSITION: ");
cur.clearBinds();
var bindvars=["1","2","3","4","5"];
var bindvals=["4","testchar4","testvarchar4","01-JAN-2004","testlong4"];
cur.inputBinds(bindvars,bindvals);
cur.inputBindClob("6","testclob4","testclob4".length);
cur.inputBindBlob("7","testblob4","testblob4".length);
assertTrue(cur.executeQuery());
console.log("");


// input bind by position with validation
console.log("INPUT BIND BY POSITION WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("1",5);
cur.inputBind("2","testchar5");
cur.inputBind("3","testvarchar5");
cur.inputBind("4",2005,1,1,0,0,0,0,null,0);
cur.inputBind("5","testlong5");
cur.inputBindClob("6","testclob5","testclob5".length);
cur.inputBindBlob("7","testblob5","testblob5".length);
cur.validateBinds();
assertTrue(cur.executeQuery());
cur.clearBinds();


// input bind by name
console.log("INPUT BIND BY NAME: ");
cur.clearBinds();
cur.inputBind("var1",6);
cur.inputBind("var2","testchar6");
cur.inputBind("var3","testvarchar6");
cur.inputBind("var4",2006,1,1,0,0,0,0,null,0);
cur.inputBind("var5","testlong6");
cur.inputBindClob("var6","testclob6","testclob6".length);
cur.inputBindBlob("var7","testblob6","testblob6".length);
assertTrue(cur.executeQuery());
console.log("");


// array of input binds by name
console.log("ARRAY OF INPUT BINDS BY NAME: ");
cur.clearBinds();
var arraybindvars=["var1","var2","var3","var4","var5"];
var arraybindvals=["7","testchar7","testvarchar7",
			"01-JAN-2007","testlong7"];
cur.inputBinds(arraybindvars,arraybindvals);
cur.inputBindClob("var6","testclob7","testclob7".length);
cur.inputBindBlob("var7","testblob7","testblob7".length);
assertTrue(cur.executeQuery());
console.log("");


// input bind by name with validation
console.log("INPUT BIND BY NAME WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("var1",8);
cur.inputBind("var2","testchar8");
cur.inputBind("var3","testvarchar8");
cur.inputBind("var4",2008,1,1,0,0,0,0,null,0);
cur.inputBind("var5","testlong8");
cur.inputBindClob("var6","testclob8","testclob8".length);
cur.inputBindBlob("var7","testblob8","testblob8".length);
cur.inputBind("var9","junkvalue");
cur.validateBinds();
assertTrue(cur.executeQuery());
console.log("");


// select
console.log("SELECT: ");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testnumber"));
console.log("");


// column count
console.log("COLUMN COUNT: ");
assertEqInt(cur.colCount(),7);
console.log("");


// column names
console.log("COLUMN NAMES: ");
assertEqStr(cur.getColumnName(0),"TESTNUMBER");
assertEqStr(cur.getColumnName(1),"TESTCHAR");
assertEqStr(cur.getColumnName(2),"TESTVARCHAR");
assertEqStr(cur.getColumnName(3),"TESTDATE");
assertEqStr(cur.getColumnName(4),"TESTLONG");
assertEqStr(cur.getColumnName(5),"TESTCLOB");
assertEqStr(cur.getColumnName(6),"TESTBLOB");
var cols=cur.getColumnNames();
assertEqStr(cols[0],"TESTNUMBER");
assertEqStr(cols[1],"TESTCHAR");
assertEqStr(cols[2],"TESTVARCHAR");
assertEqStr(cols[3],"TESTDATE");
assertEqStr(cols[4],"TESTLONG");
assertEqStr(cols[5],"TESTCLOB");
assertEqStr(cols[6],"TESTBLOB");
console.log("");


// column types
console.log("COLUMN TYPES: ");
assertEqStr(cur.getColumnType(0),"NUMBER");
assertEqStr(cur.getColumnType("TESTNUMBER"),"NUMBER");
assertEqStr(cur.getColumnType(1),"CHAR");
assertEqStr(cur.getColumnType("TESTCHAR"),"CHAR");
assertEqStr(cur.getColumnType(2),"VARCHAR2");
assertEqStr(cur.getColumnType("TESTVARCHAR"),"VARCHAR2");
assertEqStr(cur.getColumnType(3),"DATE");
assertEqStr(cur.getColumnType("TESTDATE"),"DATE");
assertEqStr(cur.getColumnType(4),"LONG");
assertEqStr(cur.getColumnType("TESTLONG"),"LONG");
assertEqStr(cur.getColumnType(5),"CLOB");
assertEqStr(cur.getColumnType("TESTCLOB"),"CLOB");
assertEqStr(cur.getColumnType(6),"BLOB");
assertEqStr(cur.getColumnType("TESTBLOB"),"BLOB");
console.log("");


// column length
console.log("COLUMN LENGTH: ");
assertEqInt(cur.getColumnLength(0),22);
assertEqInt(cur.getColumnLength("TESTNUMBER"),22);
assertEqInt(cur.getColumnLength(1),40);
assertEqInt(cur.getColumnLength("TESTCHAR"),40);
assertEqInt(cur.getColumnLength(2),40);
assertEqInt(cur.getColumnLength("TESTVARCHAR"),40);
assertEqInt(cur.getColumnLength(3),7);
assertEqInt(cur.getColumnLength("TESTDATE"),7);
assertEqInt(cur.getColumnLength(4),0);
assertEqInt(cur.getColumnLength("TESTLONG"),0);
assertEqInt(cur.getColumnLength(5),0);
assertEqInt(cur.getColumnLength("TESTCLOB"),0);
assertEqInt(cur.getColumnLength(6),0);
assertEqInt(cur.getColumnLength("TESTBLOB"),0);
console.log("");


// longest column
console.log("LONGEST COLUMN: ");
assertEqInt(cur.getLongest(0),1);
assertEqInt(cur.getLongest("TESTNUMBER"),1);
assertEqInt(cur.getLongest(1),40);
assertEqInt(cur.getLongest("TESTCHAR"),40);
assertEqInt(cur.getLongest(2),12);
assertEqInt(cur.getLongest("TESTVARCHAR"),12);
assertEqInt(cur.getLongest(3),9);
assertEqInt(cur.getLongest("TESTDATE"),9);
assertEqInt(cur.getLongest(4),9);
assertEqInt(cur.getLongest("TESTLONG"),9);
assertEqInt(cur.getLongest(5),9);
assertEqInt(cur.getLongest("TESTCLOB"),9);
assertEqInt(cur.getLongest(6),9);
assertEqInt(cur.getLongest("TESTBLOB"),9);
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
assertEqStr(cur.getField(0,1),
		"testchar1                               ");
assertEqStr(cur.getField(0,2),"testvarchar1");
assertEqStr(cur.getField(0,3),"01-JAN-01");
assertEqStr(cur.getField(0,4),"testlong1");
assertEqStr(cur.getField(0,5),"testclob1");
assertEqStr(cur.getField(0,6),"");
console.log("");
assertEqStr(cur.getField(7,0),"8");
assertEqStr(cur.getField(7,1),
		"testchar8                               ");
assertEqStr(cur.getField(7,2),"testvarchar8");
assertEqStr(cur.getField(7,3),"01-JAN-08");
assertEqStr(cur.getField(7,4),"testlong8");
assertEqStr(cur.getField(7,5),"testclob8");
assertEqStr(cur.getField(7,6),"testblob8");
console.log("");


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqInt(cur.getFieldLength(0,0),1);
assertEqInt(cur.getFieldLength(0,1),40);
assertEqInt(cur.getFieldLength(0,2),12);
assertEqInt(cur.getFieldLength(0,3),9);
assertEqInt(cur.getFieldLength(0,4),9);
assertEqInt(cur.getFieldLength(0,5),9);
assertEqInt(cur.getFieldLength(0,6),0);
console.log("");
assertEqInt(cur.getFieldLength(7,0),1);
assertEqInt(cur.getFieldLength(7,1),40);
assertEqInt(cur.getFieldLength(7,2),12);
assertEqInt(cur.getFieldLength(7,3),9);
assertEqInt(cur.getFieldLength(7,4),9);
assertEqInt(cur.getFieldLength(7,5),9);
assertEqInt(cur.getFieldLength(7,6),9);
console.log("");


// fields by name
console.log("FIELDS BY NAME: ");
assertEqStr(cur.getField(0,"TESTNUMBER"),"1");
assertEqStr(cur.getField(0,"TESTCHAR"),
		"testchar1                               ");
assertEqStr(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
assertEqStr(cur.getField(0,"TESTDATE"),"01-JAN-01");
assertEqStr(cur.getField(0,"TESTLONG"),"testlong1");
assertEqStr(cur.getField(0,"TESTCLOB"),"testclob1");
assertEqStr(cur.getField(0,"TESTBLOB"),"");
console.log("");
assertEqStr(cur.getField(7,"TESTNUMBER"),"8");
assertEqStr(cur.getField(7,"TESTCHAR"),
		"testchar8                               ");
assertEqStr(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
assertEqStr(cur.getField(7,"TESTDATE"),"01-JAN-08");
assertEqStr(cur.getField(7,"TESTLONG"),"testlong8");
assertEqStr(cur.getField(7,"TESTCLOB"),"testclob8");
assertEqStr(cur.getField(7,"TESTBLOB"),"testblob8");
console.log("");


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqInt(cur.getFieldLength(0,"TESTNUMBER"),1);
assertEqInt(cur.getFieldLength(0,"TESTCHAR"),40);
assertEqInt(cur.getFieldLength(0,"TESTVARCHAR"),12);
assertEqInt(cur.getFieldLength(0,"TESTDATE"),9);
assertEqInt(cur.getFieldLength(0,"TESTLONG"),9);
assertEqInt(cur.getFieldLength(0,"TESTCLOB"),9);
assertEqInt(cur.getFieldLength(0,"TESTBLOB"),0);
console.log("");
assertEqInt(cur.getFieldLength(7,"TESTNUMBER"),1);
assertEqInt(cur.getFieldLength(7,"TESTCHAR"),40);
assertEqInt(cur.getFieldLength(7,"TESTVARCHAR"),12);
assertEqInt(cur.getFieldLength(7,"TESTDATE"),9);
assertEqInt(cur.getFieldLength(7,"TESTLONG"),9);
assertEqInt(cur.getFieldLength(7,"TESTCLOB"),9);
assertEqInt(cur.getFieldLength(7,"TESTBLOB"),9);
console.log("");


// fields by array
console.log("FIELDS BY ARRAY: ");
var fields=cur.getRow(0);
assertEqStr(fields[0],"1");
assertEqStr(fields[1],"testchar1                               ");
assertEqStr(fields[2],"testvarchar1");
assertEqStr(fields[3],"01-JAN-01");
assertEqStr(fields[4],"testlong1");
assertEqStr(fields[5],"testclob1");
assertEqStr(fields[6],"");
console.log("");


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
var fieldlens=cur.getRowLengths(0);
assertEqInt(fieldlens[0],1);
assertEqInt(fieldlens[1],40);
assertEqInt(fieldlens[2],12);
assertEqInt(fieldlens[3],9);
assertEqInt(fieldlens[4],9);
assertEqInt(fieldlens[5],9);
assertEqInt(fieldlens[6],0);
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
	"	testnumber"));
assertEqInt(cur.getResultSetBufferSize(),2);
console.log("");
assertEqInt(cur.firstRowIndex(),0);
assertFalse(cur.endOfResultSet());
assertEqInt(cur.rowCount(),2);
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(1,0),"2");
assertEqStr(cur.getField(2,0),"3");
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
	"	testnumber"));
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
	"	testnumber"));
assertEqStr(cur.getColumnName(0),"TESTNUMBER");
assertEqInt(cur.getColumnLength(0),22);
assertEqStr(cur.getColumnType(0),"NUMBER");
console.log("");


// suspended session
console.log("SUSPENDED SESSION: ");
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testnumber"));
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
	"	testnumber"));
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
	"	testnumber"));
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
	"	testnumber"));
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
	"	testnumber"));
var filename=cur.getCacheFileName();
assertEqStr(filename,"cachefile1");
cur.cacheOff();
assertTrue(cur.openCachedResultSet(filename));
assertEqStr(cur.getField(7,0),"8");
console.log("");


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqInt(cur.colCount(),7);
console.log("");


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqStr(cur.getColumnName(0),"TESTNUMBER");
assertEqStr(cur.getColumnName(1),"TESTCHAR");
assertEqStr(cur.getColumnName(2),"TESTVARCHAR");
assertEqStr(cur.getColumnName(3),"TESTDATE");
assertEqStr(cur.getColumnName(4),"TESTLONG");
assertEqStr(cur.getColumnName(5),"TESTCLOB");
assertEqStr(cur.getColumnName(6),"TESTBLOB");
cols=cur.getColumnNames();
assertEqStr(cols[0],"TESTNUMBER");
assertEqStr(cols[1],"TESTCHAR");
assertEqStr(cols[2],"TESTVARCHAR");
assertEqStr(cols[3],"TESTDATE");
assertEqStr(cols[4],"TESTLONG");
assertEqStr(cols[5],"TESTCLOB");
assertEqStr(cols[6],"TESTBLOB");
console.log("");


// cached result set with result set buffer size
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
	"	testnumber"));
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


// from one cache file to another with result set buffer size
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


// cached result set with suspend and result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND "+
			"AND RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertTrue(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testnumber"));
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
	"	testnumber"));
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
	assertTrue(secondcur.sendQuery("select * from testtable"));
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
var secondcon=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket",
					null,null,0,1);
setSecondConnection(secondcon);
var secondcur=new sqlrelay.SQLRCursor(secondcon);
setSecondCursor(secondcur);
secondcon.enableKerberos(null,null,null);
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
cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual");
cur.substitution("var1","$(var11)");
cur.substitution("var2","$(var21)");
cur.substitution("var3","$(var31)");
cur.substitution("var11","$(var111)");
cur.substitution("var21","$(var211)");
cur.substitution("var31","$(var311)");
cur.substitution("var111",1);
cur.substitution("var211","hello");
cur.substitution("var311",10.5556,6,4);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"10.5556");
console.log("");


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
var subvars=["var1","var2","var3"];
cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
var subvallongs=[1,2,3];
cur.substitutions(subvars,subvallongs);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertEqStr(cur.getField(0,1),"2");
assertEqStr(cur.getField(0,2),"3");
console.log("");
cur.prepareQuery(
		"select '$(var1)','$(var2)','$(var3)' from dual");
var subvalstrings=["hi","hello","bye"];
cur.substitutions(subvars,subvalstrings);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"hi");
assertEqStr(cur.getField(0,1),"hello");
assertEqStr(cur.getField(0,2),"bye");
console.log("");
cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
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
assertTrue(cur.sendQuery("select NULL,1,NULL from dual"));
assertEqStr(cur.getField(0,0),null);
assertEqStr(cur.getField(0,1),"1");
assertEqStr(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertTrue(cur.sendQuery("select NULL,1,NULL from dual"));
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
cur.prepareQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	:var1, "+
	"	:var2, "+
	"	:var3, "+
	"	:var4)");
cur.inputBindClob("var1","","".length);
cur.inputBindClob("var2",null,0);
cur.inputBindBlob("var3","","".length);
cur.inputBindBlob("var4",null,0);
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
	"	testclob clob, "+
	"	testblob blob)");
cur.prepareQuery(
		"insert into testtable values (:clobval,:blobval)");
var largebuffer="C".repeat(8192);
cur.inputBindClob("clobval",largebuffer,largebuffer.length);
cur.inputBindBlob("blobval",largebuffer,largebuffer.length);
assertTrue(cur.executeQuery());
cur.sendQuery("select * from testtable");
assertEqInt(cur.getFieldLength(0,"TESTCLOB"),8192);
assertEqStr(cur.getField(0,"TESTCLOB"),largebuffer);
assertEqInt(cur.getFieldLength(0,"TESTBLOB"),8192);
assertEqStrLen(cur.getField(0,"TESTBLOB"),largebuffer,8192);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// output bind by position
console.log("OUTPUT BIND BY POSITION: ");
cur.getNullsAsNulls();
cur.prepareQuery(
	"begin "+
	"	:numvar:=1; "+
	"	:stringvar:='hello'; "+
	"	:floatvar:=2.5; "+
	"	:datevar:='03-FEB-2001'; "+
	"	:nullvar:=null; "+
	"end;");
assertEqInt(cur.countBindVariables(),5);
cur.defineOutputBindInteger("1");
cur.defineOutputBindString("2",10);
cur.defineOutputBindDouble("3");
cur.defineOutputBindDate("4");
cur.defineOutputBindString("5",10);
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
console.log("");


// output bind by name
console.log("OUTPUT BIND BY NAME: ");
cur.getNullsAsNulls();
cur.clearBinds();
cur.defineOutputBindInteger("numvar");
cur.defineOutputBindString("stringvar",10);
cur.defineOutputBindDouble("floatvar");
cur.defineOutputBindDate("datevar");
cur.defineOutputBindString("nullvar",10);
assertTrue(cur.executeQuery());
numvar=cur.getOutputBindInteger("numvar");
stringvar=cur.getOutputBindString("stringvar");
floatvar=cur.getOutputBindDouble("floatvar");
year=cur.getOutputBindDateYear("datevar");
month=cur.getOutputBindDateMonth("datevar");
day=cur.getOutputBindDateDay("datevar");
hour=cur.getOutputBindDateHour("datevar");
minute=cur.getOutputBindDateMinute("datevar");
second=cur.getOutputBindDateSecond("datevar");
microsecond=cur.getOutputBindDateMicrosecond("datevar");
tz=cur.getOutputBindDateTz("datevar");
isnegative=cur.getOutputBindDateIsNegative("datevar");
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
nullvar=cur.getOutputBindString("nullvar");
assertEqStr(nullvar,null);
cur.getNullsAsEmptyStrings();
console.log("");


// output bind by name with validation
console.log("OUTPUT BIND BY NAME WITH VALIDATION: ");
cur.getNullsAsNulls();
cur.clearBinds();
cur.defineOutputBindInteger("numvar");
cur.defineOutputBindString("stringvar",10);
cur.defineOutputBindDouble("floatvar");
cur.defineOutputBindDate("datevar");
cur.defineOutputBindString("nullvar",10);
cur.defineOutputBindString("dummyvar",10);
cur.validateBinds();
assertTrue(cur.executeQuery());
numvar=cur.getOutputBindInteger("numvar");
stringvar=cur.getOutputBindString("stringvar");
floatvar=cur.getOutputBindDouble("floatvar");
year=cur.getOutputBindDateYear("datevar");
month=cur.getOutputBindDateMonth("datevar");
day=cur.getOutputBindDateDay("datevar");
hour=cur.getOutputBindDateHour("datevar");
minute=cur.getOutputBindDateMinute("datevar");
second=cur.getOutputBindDateSecond("datevar");
microsecond=cur.getOutputBindDateMicrosecond("datevar");
tz=cur.getOutputBindDateTz("datevar");
isnegative=cur.getOutputBindDateIsNegative("datevar");
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
nullvar=cur.getOutputBindString("nullvar");
assertEqStr(nullvar,null);
cur.getNullsAsEmptyStrings();
console.log("");


// lob output bind
console.log("LOB OUTPUT BIND: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testclob clob, "+
	"	testblob blob)"));
cur.prepareQuery(
		"insert into testtable values ('hello',:var1)");
cur.inputBindBlob("var1","hello","hello".length);
assertTrue(cur.executeQuery());
cur.prepareQuery(
	"begin "+
	"	select testclob into :clobvar from testtable; "+
	"	select testblob into :blobvar from testtable; "+
	"end;");
cur.defineOutputBindClob("clobvar");
cur.defineOutputBindBlob("blobvar");
assertTrue(cur.executeQuery());
var clobvar=cur.getOutputBindClob("clobvar");
var clobvarlength=cur.getOutputBindLength("clobvar");
var blobvar=cur.getOutputBindBlob("blobvar");
var blobvarlength=cur.getOutputBindLength("blobvar");
assertEqStrLen(clobvar,"hello",5);
assertEqInt(clobvarlength,5);
assertEqStrLen(blobvar,"hello",5);
assertEqInt(blobvarlength,5);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// long output bind
console.log("LONG OUTPUT BIND: ");
largebuffer="C".repeat(8192);
var query="begin :bindval:='"+largebuffer+"'; end;";
cur.prepareQuery(query);
cur.defineOutputBindString("bindval",8192);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindLength("bindval"),8192);
assertEqStr(cur.getOutputBindString("bindval"),largebuffer);
console.log("");


// negative input bind
console.log("NEGATIVE INPUT BIND: ");
cur.sendQuery("drop table testtable");
cur.sendQuery("create table testtable (testval number)");
cur.prepareQuery("insert into testtable values (:testval)");
cur.inputBind("testval",-1);
assertTrue(cur.executeQuery());
cur.sendQuery("select testval from testtable");
assertEqStr(cur.getField(0,"TESTVAL"),"-1");
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// bind validation
console.log("BIND VALIDATION: ");
cur.sendQuery("drop table testtable");
cur.sendQuery(
	"create table testtable ("+
	"	col1 varchar2(20), "+
	"	col2 varchar2(20), "+
	"	col3 varchar2(20))");
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
cur.substitution("var1",":var1");
assertTrue(cur.validBind("var1"));
assertFalse(cur.validBind("var2"));
assertFalse(cur.validBind("var3"));
assertFalse(cur.validBind("var4"));
console.log("");
cur.substitution("var2",":var2");
assertTrue(cur.validBind("var1"));
assertTrue(cur.validBind("var2"));
assertFalse(cur.validBind("var3"));
assertFalse(cur.validBind("var4"));
console.log("");
cur.substitution("var3",":var3");
assertTrue(cur.validBind("var1"));
assertTrue(cur.validBind("var2"));
assertTrue(cur.validBind("var3"));
assertFalse(cur.validBind("var4"));
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// rebinding
console.log("REBINDING: ");
cur.prepareQuery(
	"begin "+
	"	:out:= :in; "+
	"end;");
cur.inputBind("in",1);
cur.defineOutputBindInteger("out");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out"),1);
cur.inputBind("in",2);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out"),2);
cur.inputBind("in",3);
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out"),3);
console.log("");


// reexecute
console.log("REEXECUTE: ");
cur.prepareQuery("select 1 from dual");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
cur.prepareQuery("select :var from dual");
cur.inputBind("var",1);
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"1");
console.log("");
cur.inputBind("var",2);
assertTrue(cur.executeQuery());
assertEqInt(cur.rowCount(),1);
assertEqStr(cur.getField(0,0),"2");
console.log("");


// stored procedure returning no value
console.log("STORED PROCEDURE RETURNING NO VALUE: ");
cur.sendQuery("drop function testproc");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create or replace "+
	"procedure testproc("+
	"	in1 in number, "+
	"	in2 in number, "+
	"	in3 in varchar2) "+
	"is "+
	"begin "+
	"	return; "+
	"end;"));
cur.prepareQuery("begin testproc(:in1,:in2,:in3); end;");
cur.inputBind("in1",1);
cur.inputBind("in2",2.5,2,1);
cur.inputBind("in3","hello");
assertTrue(cur.executeQuery());
assertTrue(cur.sendQuery("drop procedure testproc"));
console.log("");


// stored procedure returning single value
console.log("STORED PROCEDURE RETURNING SINGLE VALUE: ");
cur.sendQuery("drop function testproc");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create or replace "+
	"function testproc("+
	"	in1 in number, "+
	"	in2 in number, "+
	"	in3 in varchar2) "+
	"	return number "+
	"is "+
	"begin "+
	"	return in1; "+
	"end;"));
cur.prepareQuery("select testproc(:in1,:in2,:in3) from dual");
cur.inputBind("in1",1);
cur.inputBind("in2",2.5,2,1);
cur.inputBind("in3","hello");
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
cur.prepareQuery(
	"begin "+
	"	:out1:=testproc(:in1,:in2,:in3); "+
	"end;");
cur.inputBind("in1",1);
cur.inputBind("in2",2.5,2,1);
cur.inputBind("in3","hello");
cur.defineOutputBindInteger("out1");
assertTrue(cur.executeQuery());
assertEqInt(cur.getOutputBindInteger("out1"),1);
assertTrue(cur.sendQuery("drop function testproc"));
console.log("");


// stored procedure returning multiple values
console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
cur.sendQuery("drop function testproc");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create or replace "+
	"procedure testproc("+
	"	in1 in number, "+
	"	in2 in number, "+
	"	in3 in varchar2, "+
	"	out1 out number, "+
	"	out2 out number, "+
	"	out3 out varchar2) "+
	"is "+
	"begin "+
	"	out1:=in1; "+
	"	out2:=in2; "+
	"	out3:=in3; "+
	"end;"));
cur.prepareQuery(
	"begin "+
	"	testproc(:in1,:in2,:in3,:out1,:out2,:out3); "+
	"end;");
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
console.log("");


// stored procedure returning result set
console.log("STORED PROCEDURE RETURNING RESULT SET: ");
cur.sendQuery("drop package types");
cur.sendQuery("drop function testproc");
cur.sendQuery("drop procedure testproc");
assertTrue(cur.sendQuery(
	"create or replace package types is "+
	"	type cursorType is ref cursor; "+
	"end;"));
assertTrue(cur.sendQuery(
	"create or replace "+
	"function testproc(value in number) "+
	"	return types.cursortype "+
	"is "+
	"	l_cursor    types.cursorType; "+
	"begin "+
	"	open l_cursor for "+
	"		select "+
	"			* "+
	"		from "+
	"			( "+
	"			select 1 as testnumber from dual "+
	"			union "+
	"			select 2 as testnumber from dual "+
	"			union "+
	"			select 3 as testnumber from dual "+
	"			union "+
	"			select 4 as testnumber from dual "+
	"			union "+
	"			select 5 as testnumber from dual "+
	"			union "+
	"			select 6 as testnumber from dual "+
	"			union "+
	"			select 7 as testnumber from dual "+
	"			union "+
	"			select 8 as testnumber from dual "+
	"			) "+
	"		where "+
	"			testnumber>value; "+
	"	return l_cursor; "+
	"end;"));
cur.prepareQuery(
	"begin "+
	"	:curs1:=testproc(5); "+
	"	:curs2:=testproc(0); "+
	"end;");
cur.defineOutputBindCursor("curs1");
cur.defineOutputBindCursor("curs2");
assertTrue(cur.executeQuery());
var bindcur1=cur.getOutputBindCursor("curs1");
assertTrue(bindcur1.fetchFromBindCursor());
assertEqStr(bindcur1.getField(0,0),"6");
assertEqStr(bindcur1.getField(1,0),"7");
assertEqStr(bindcur1.getField(2,0),"8");
var bindcur2=cur.getOutputBindCursor("curs2");
assertTrue(bindcur2.fetchFromBindCursor());
assertEqStr(bindcur2.getField(0,0),"1");
assertEqStr(bindcur2.getField(1,0),"2");
assertEqStr(bindcur2.getField(2,0),"3");
assertTrue(cur.sendQuery("drop function testproc"));
assertTrue(cur.sendQuery("drop package types"));
console.log("");


// temporary tables
console.log("TEMPORARY TABLES: ");
cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete");
cur.substitution("HOSTNAME",hostname);
cur.executeQuery();
cur.prepareQuery(
	"create global temporary table $(HOSTNAME)_temptabledelete ( "+
	"	col1 number "+
	") on commit delete rows");
cur.substitution("HOSTNAME",hostname);
cur.executeQuery();
cur.prepareQuery(
		"insert into $(HOSTNAME)_temptabledelete values (1)");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
cur.prepareQuery(
		"select count(*) from $(HOSTNAME)_temptabledelete");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertTrue(con.commit());
cur.prepareQuery(
		"select count(*) from $(HOSTNAME)_temptabledelete");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"0");
cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete");
cur.substitution("HOSTNAME",hostname);
cur.executeQuery();
console.log("");
cur.prepareQuery(
		"truncate table $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
cur.executeQuery();
cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
cur.executeQuery();
cur.prepareQuery(
	"create global temporary table $(HOSTNAME)_temptablepreserve ("+
	"	col1 number "+
	") on commit preserve rows");
cur.substitution("HOSTNAME",hostname);
cur.executeQuery();
cur.prepareQuery(
	"insert into "+
	"	$(HOSTNAME)_temptablepreserve "+
	"values (1)");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
cur.prepareQuery(
		"select count(*) from $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
assertTrue(con.commit());
cur.prepareQuery(
		"select count(*) from $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"1");
con.endSession();
console.log("");
cur.prepareQuery(
		"select count(*) from $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
assertEqStr(cur.getField(0,0),"0");
cur.prepareQuery(
		"truncate table $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
var start=Date.now(); while (Date.now()-start<2000) {}
cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
assertTrue(cur.executeQuery());
cur.prepareQuery(
		"select count(*) from $(HOSTNAME)_temptablepreserve");
cur.substitution("HOSTNAME",hostname);
assertFalse(cur.executeQuery());
console.log("");


// encoded binary data
console.log("ENCODED BINARY DATA: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery("create table testtable (col1 blob)"));
var buffer="";
for (var i=0; i<256; i++) {
	buffer+=String.fromCharCode(i);
}
var querystr="insert into testtable values ('";
for (var i=0; i<buffer.length; i++) {
	querystr+=("0"+buffer.charCodeAt(i).toString(16)).slice(-2);
}
querystr+="')";
assertTrue(cur.sendQuery(querystr));
// Verify round-tripped bytes via server-side RAWTOHEX (the binding's
// getField returns strings via String::NewFromUtf8, which drops
// invalid UTF-8 byte sequences that arise from raw bytes 128-255).
assertTrue(cur.sendQuery(
	"select rawtohex(dbms_lob.substr(col1,4000,1)) from testtable"));
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
assertTrue(cur.sendQuery("create table testtable (col1 varchar2(4))"));
assertTrue(cur.sendQuery("insert into testtable values ('''''')"));
assertTrue(cur.sendQuery("select col1 from testtable"));
assertEqInt(cur.getFieldLength(0,0),2);
assertEqStr(cur.getField(0,0),"''");
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// last insert id
// oracle doesn't support auto-increment


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
	if (String(cur.getField(i,"Database")).toLowerCase()===
			String(hostname).toLowerCase()) {
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
assertEqStr(cur.getField(0,"table_type"),"SYNONYM");
assertEqStr(cur.getField(1,"table_type"),"TABLE");
assertEqStr(cur.getField(2,"table_type"),"VIEW");
console.log("");


// table list
console.log("TABLE LIST: ");
cur.sendQuery("drop table testtable1");
cur.sendQuery("drop table testtable2");
cur.sendQuery("drop table testtable3");
cur.sendQuery("drop table testtable4");
assertTrue(cur.sendQuery(
	"create table testtable1 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(cur.sendQuery(
	"create table testtable2 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(cur.sendQuery(
	"create table testtable3 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(cur.sendQuery(
	"create table testtable4 ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"));
assertTrue(cur.getTableList(null));
var counter=0;
for (var i=0; i<cur.rowCount(); i++) {
	var name=cur.getField(i,"Tables_in_xxx");
	if (name==="TESTTABLE1" ||
		name==="TESTTABLE2" ||
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
console.log("");


// type info list
console.log("TYPE INFO LIST: ");
assertTrue(cur.getTypeInfoList("number"));
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
assertEqStr(cur.getField(0,"type_name"),"NUMBER");
assertEqStr(cur.getField(0,"data_type"),"-7");
assertEqStr(cur.getField(0,"precision"),"1");
assertEqStr(cur.getField(0,"local_type_name"),"NUMBER");
assertTrue(cur.getTypeInfoList("char"));
assertEqStr(cur.getField(0,"type_name"),"CHAR");
assertEqStr(cur.getField(0,"data_type"),"1");
assertEqStr(cur.getField(0,"precision"),"2000");
assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
assertTrue(cur.getTypeInfoList("varchar2"));
assertEqStr(cur.getField(0,"type_name"),"VARCHAR2");
assertEqStr(cur.getField(0,"data_type"),"12");
assertEqStr(cur.getField(0,"precision"),"32767");
assertEqStr(cur.getField(0,"local_type_name"),"VARCHAR2");
assertTrue(cur.getTypeInfoList("date"));
assertEqStr(cur.getField(0,"type_name"),"DATE");
assertEqStr(cur.getField(0,"data_type"),"92");
assertEqStr(cur.getField(0,"precision"),"7");
assertEqStr(cur.getField(0,"local_type_name"),"DATE");
console.log("");


// column list
console.log("COLUMN LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"));
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
assertEqStr(cur.getField(0,"column_name"),"TESTNUMBER");
assertEqStr(cur.getField(1,"column_name"),"TESTCHAR");
assertEqStr(cur.getField(2,"column_name"),"TESTVARCHAR");
assertEqStr(cur.getField(3,"column_name"),"TESTDATE");
assertEqStr(cur.getField(4,"column_name"),"TESTLONG");
assertEqStr(cur.getField(5,"column_name"),"TESTCLOB");
assertEqStr(cur.getField(6,"column_name"),"TESTBLOB");
assertEqStr(cur.getField(0,"data_type"),"NUMBER");
assertEqStr(cur.getField(1,"data_type"),"CHAR");
assertEqStr(cur.getField(2,"data_type"),"VARCHAR2");
assertEqStr(cur.getField(3,"data_type"),"DATE");
assertEqStr(cur.getField(4,"data_type"),"LONG");
assertEqStr(cur.getField(5,"data_type"),"CLOB");
assertEqStr(cur.getField(6,"data_type"),"BLOB");
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// column list - auto_increment, primary key
// oracle doesn't support auto_increment
console.log("COLUMN LIST - auto_increment, primary key: ");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 number primary key, "+
	"	col2 number)"));
assertTrue(cur.getColumnList("testtable",null));
assertTrue(String(cur.getField(0,"column_key")).indexOf("PRI")>=0);
assertFalse(String(cur.getField(1,"column_key")).indexOf("PRI")>=0);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// primary keys list
console.log("PRIMARY KEYS LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 number primary key, "+
	"	col2 number)"));
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
assertEqStr(cur.getField(0,"table"),"TESTTABLE");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertEqStr(cur.getField(0,"column_name"),"COL1");
var keyname=cur.getField(0,"key_name");
assertTrue(keyname && keyname[0]);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// key and index list
console.log("KEY AND INDEX LIST: ");
cur.sendQuery("drop table testtable");
assertTrue(cur.sendQuery(
	"create table testtable ("+
	"	col1 number primary key, "+
	"	col2 number)"));
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
assertEqStr(cur.getField(0,"table"),"TESTTABLE");
assertEqStr(cur.getField(0,"non_unique"),"0");
assertEqStr(cur.getField(0,"seq_in_index"),"1");
assertEqStr(cur.getField(0,"column_name"),"COL1");
assertEqStr(cur.getField(0,"collation"),"A");
assertEqStr(cur.getField(0,"index_type"),"3");
keyname=cur.getField(0,"key_name");
assertTrue(keyname && keyname[0]);
assertTrue(cur.sendQuery("drop table testtable"));
console.log("");


// procedure list
console.log("PROCEDURE LIST: ");
cur.sendQuery("drop procedure testproc1");
cur.sendQuery("drop procedure testproc2");
cur.sendQuery("drop procedure testproc3");
cur.sendQuery("drop procedure testproc4");
assertTrue(cur.sendQuery(
	"create procedure testproc1("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"));
assertTrue(cur.sendQuery(
	"create procedure testproc2("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"));
assertTrue(cur.sendQuery(
	"create procedure testproc3("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"));
assertTrue(cur.sendQuery(
	"create procedure testproc4("+
	"	in1 in number, "+
	"	in2 in char, "+
	"	in3 in varchar2, "+
	"	in4 in date) as "+
	"begin "+
	"	null; "+
	"end;"));
assertTrue(cur.getProcedureList(null));
counter=0;
for (var i=0; i<cur.rowCount(); i++) {
	var name=cur.getField(i,"routine_name");
	if (name==="TESTPROC1" ||
		name==="TESTPROC2" ||
		name==="TESTPROC3" ||
		name==="TESTPROC4") {
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
assertEqStr(cur.getField(0,"data_type"),"NUMBER");
assertEqStr(cur.getField(0,"ordinal_position"),"1");
assertEqStr(cur.getField(1,"parameter_name"),"IN2");
assertEqStr(cur.getField(1,"parameter_mode"),"1");
assertEqStr(cur.getField(1,"data_type"),"CHAR");
assertEqStr(cur.getField(1,"ordinal_position"),"2");
assertEqStr(cur.getField(2,"parameter_name"),"IN3");
assertEqStr(cur.getField(2,"parameter_mode"),"1");
assertEqStr(cur.getField(2,"data_type"),"VARCHAR2");
assertEqStr(cur.getField(2,"ordinal_position"),"3");
assertEqStr(cur.getField(3,"parameter_name"),"IN4");
assertEqStr(cur.getField(3,"parameter_mode"),"1");
assertEqStr(cur.getField(3,"data_type"),"DATE");
assertEqStr(cur.getField(3,"ordinal_position"),"4");
assertTrue(cur.sendQuery("drop procedure testproc1"));
assertTrue(cur.sendQuery("drop procedure testproc2"));
assertTrue(cur.sendQuery("drop procedure testproc3"));
assertTrue(cur.sendQuery("drop procedure testproc4"));
console.log("");


// invalid queries
console.log("INVALID QUERIES: ");
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testnumber"));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testnumber"));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testnumber"));
assertFalse(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testtable "+
	"order by "+
	"	testnumber"));
console.log("");
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
console.log("");
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
assertFalse(cur.sendQuery("create table testtable"));
console.log("");


reportTestStatus();

process.exit(getStatus());
