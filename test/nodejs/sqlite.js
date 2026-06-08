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


	var	subvars=["var1","var2","var3"];
	var	subvalstrings=["hi","hello","bye"];
	var	subvallongs=[1,2,3];
	var	subvaldoubles=[10.55,10.556,10.5556];
	var	precs=[4,5,6];
	var	scales=[2,3,4];

	var	isolationlevels=["0","1"];


	// instantiation
	var	con=new sqlrelay.SQLRConnection("sqlrelay",9000,"/tmp/test.socket",
			"testuser","testpassword",0,1);
	setConnection(con);
	var	cur=new sqlrelay.SQLRCursor(con);
	setCursor(cur);


	// identify
	console.log("IDENTIFY: ");
	assertEqStr(con.identify(),"sqlite");
	console.log("");


	// db version
	console.log("DB VERSION: ");
	var	dbversion=con.dbVersion();
	var	issqlite3=1;
	if (!dbversion ||
		dbversion=="unknown" ||
		parseInt(dbversion)<3) {
		issqlite3=0;
	}
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
	assertEqStr(con.bindFormat(),":*");
	console.log("");


	// nextval format
	console.log("NEXTVAL FORMAT: ");
	assertEqStr(con.nextvalFormat(),"");
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
	con.begin();
	cur.sendQuery("drop table if exists testtable");
	assertTrue(cur.sendQuery(
		"create table testtable ("+
		"	testint int, "+
		"	testfloat float, "+
		"	testchar char(40), "+
		"	testvarchar varchar(40), "+
		"	testclob clob, "+
		"	testblob blob)"));
	con.commit();
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
		"	'testchar1', "+
		"	'testvarchar1', "+
		"	'testclob1', "+
		"	'testblob1')"));
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	2, "+
		"	2.5, "+
		"	'testchar2', "+
		"	'testvarchar2', "+
		"	'testclob2', "+
		"	'testblob2')"));
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	3, "+
		"	3.5, "+
		"	'testchar3', "+
		"	'testvarchar3', "+
		"	'testclob3', "+
		"	'testblob3')"));
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	4, "+
		"	4.5, "+
		"	'testchar4', "+
		"	'testvarchar4', "+
		"	'testclob4', "+
		"	'testblob4')"));
	console.log("");


	// affected rows
	console.log("AFFECTED ROWS: ");
	assertEqInt(cur.affectedRows(),1);
	console.log("");


	// input bind by position
	// sqlite doesn't support bind by position


	// array of input binds by position
	// sqlite doesn't support bind by position


	// input bind by position with validation
	// sqlite doesn't support bind by position


	// input bind by name
	console.log("INPUT BIND BY NAME: ");
	cur.prepareQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	:var1, "+
		"	:var2, "+
		"	:var3, "+
		"	:var4, "+
		"	:var5, "+
		"	:var6)");
	assertEqInt(cur.countBindVariables(),6);
	cur.inputBind("var1",5);
	cur.inputBind("var2",5.5,4,1);
	cur.inputBind("var3","testchar5");
	cur.inputBind("var4","testvarchar5");
	cur.inputBindClob("var5","testclob5","testclob5".length);
	cur.inputBindBlob("var6","testblob5","testblob5".length);
	assertTrue(cur.executeQuery());
	cur.clearBinds();
	cur.inputBind("var1",6);
	cur.inputBind("var2",6.5,4,1);
	cur.inputBind("var3","testchar6");
	cur.inputBind("var4","testvarchar6");
	cur.inputBindClob("var5","testclob6","testclob6".length);
	cur.inputBindBlob("var6","testblob6","testblob6".length);
	assertTrue(cur.executeQuery());
	cur.clearBinds();
	cur.inputBind("var1",7);
	cur.inputBind("var2",7.5,4,1);
	cur.inputBind("var3","testchar7");
	cur.inputBind("var4","testvarchar7");
	cur.inputBindClob("var5","testclob7","testclob7".length);
	cur.inputBindBlob("var6","testblob7","testblob7".length);
	assertTrue(cur.executeQuery());
	console.log("");


	// array of input binds by name
	// sqlite doesn't support implicit conversion
	// of string binds to other data types, so
	// arrays of binds don't generally work.


	// input bind by name with validation
	console.log("INPUT BIND BY NAME WITH VALIDATION: ");
	cur.clearBinds();
	cur.inputBind("var1",8);
	cur.inputBind("var2",8.5,4,1);
	cur.inputBind("var3","testchar8");
	cur.inputBind("var4","testvarchar8");
	cur.inputBindClob("var5","testclob8","testclob8".length);
	cur.inputBindBlob("var6","testblob8","testblob8".length);
	cur.validateBinds();
	assertTrue(cur.executeQuery());
	console.log("");


	// select
	console.log("SELECT: ");
	assertTrue(cur.sendQuery("select * from testtable "+
		"order by testint"));
	console.log("");


	// column count
	console.log("COLUMN COUNT: ");
	assertEqInt(cur.colCount(),6);
	console.log("");


	// column names
	console.log("COLUMN NAMES: ");
	assertEqStr(cur.getColumnName(0),"testint");
	assertEqStr(cur.getColumnName(1),"testfloat");
	assertEqStr(cur.getColumnName(2),"testchar");
	assertEqStr(cur.getColumnName(3),"testvarchar");
	var cols=cur.getColumnNames();
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testfloat");
	assertEqStr(cols[2],"testchar");
	assertEqStr(cols[3],"testvarchar");
	console.log("");


	// column types
	console.log("COLUMN TYPES: ");
	if (issqlite3) {
		assertEqStr(cur.getColumnType(0),"INTEGER");
		assertEqStr(cur.getColumnType("testint"),"INTEGER");
		assertEqStr(cur.getColumnType(1),"FLOAT");
		assertEqStr(cur.getColumnType("testfloat"),"FLOAT");
		assertEqStr(cur.getColumnType(2),"STRING");
		assertEqStr(cur.getColumnType("testchar"),"STRING");
		assertEqStr(cur.getColumnType(3),"STRING");
		assertEqStr(cur.getColumnType("testvarchar"),"STRING");
		assertEqStr(cur.getColumnType(4),"STRING");
		assertEqStr(cur.getColumnType("testclob"),"STRING");
		assertEqStr(cur.getColumnType(5),"STRING");
		assertEqStr(cur.getColumnType("testblob"),"STRING");
	} else {
		assertEqStr(cur.getColumnType(0),"UNKNOWN");
		assertEqStr(cur.getColumnType("testint"),"UNKNOWN");
		assertEqStr(cur.getColumnType(1),"UNKNOWN");
		assertEqStr(cur.getColumnType("testfloat"),"UNKNOWN");
		assertEqStr(cur.getColumnType(2),"UNKNOWN");
		assertEqStr(cur.getColumnType("testchar"),"UNKNOWN");
		assertEqStr(cur.getColumnType(3),"UNKNOWN");
		assertEqStr(cur.getColumnType("testvarchar"),"UNKNOWN");
		assertEqStr(cur.getColumnType(4),"UNKNOWN");
		assertEqStr(cur.getColumnType("testclob"),"UNKNOWN");
		assertEqStr(cur.getColumnType(5),"UNKNOWN");
		assertEqStr(cur.getColumnType("testblob"),"UNKNOWN");
	}
	console.log("");


	// column length
	console.log("COLUMN LENGTH: ");
	assertEqInt(cur.getColumnLength(0),0);
	assertEqInt(cur.getColumnLength("testint"),0);
	assertEqInt(cur.getColumnLength(1),0);
	assertEqInt(cur.getColumnLength("testfloat"),0);
	assertEqInt(cur.getColumnLength(2),0);
	assertEqInt(cur.getColumnLength("testchar"),0);
	assertEqInt(cur.getColumnLength(3),0);
	assertEqInt(cur.getColumnLength("testvarchar"),0);
	assertEqInt(cur.getColumnLength(4),0);
	assertEqInt(cur.getColumnLength("testclob"),0);
	assertEqInt(cur.getColumnLength(5),0);
	assertEqInt(cur.getColumnLength("testblob"),0);
	console.log("");


	// longest column
	console.log("LONGEST COLUMN: ");
	assertEqInt(cur.getLongest(0),1);
	assertEqInt(cur.getLongest("testint"),1);
	assertEqInt(cur.getLongest(1),3);
	assertEqInt(cur.getLongest("testfloat"),3);
	assertEqInt(cur.getLongest(2),9);
	assertEqInt(cur.getLongest("testchar"),9);
	assertEqInt(cur.getLongest(3),12);
	assertEqInt(cur.getLongest("testvarchar"),12);
	assertEqInt(cur.getLongest(4),9);
	assertEqInt(cur.getLongest("testclob"),9);
	assertEqInt(cur.getLongest(5),9);
	assertEqInt(cur.getLongest("testblob"),9);
	console.log("");


	// row count
	console.log("ROW COUNT: ");
	assertEqInt(cur.rowCount(),8);
	console.log("");


	// total rows
	console.log("TOTAL ROWS: ");
	assertEqInt(cur.totalRows(),(issqlite3)?0:8);
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
	assertEqStr(cur.getField(0,2),"testchar1");
	assertEqStr(cur.getField(0,3),"testvarchar1");
	assertEqStr(cur.getField(0,4),"testclob1");
	assertEqStr(cur.getField(0,5),"testblob1");
	console.log("");
	assertEqStr(cur.getField(7,0),"8");
	assertEqStr(cur.getField(7,1),"8.5");
	assertEqStr(cur.getField(7,2),"testchar8");
	assertEqStr(cur.getField(7,3),"testvarchar8");
	assertEqStr(cur.getField(7,4),"testclob8");
	assertEqStr(cur.getField(7,5),"testblob8");
	console.log("");


	// field lengths by index
	console.log("FIELD LENGTHS BY INDEX: ");
	assertEqInt(cur.getFieldLength(0,0),1);
	assertEqInt(cur.getFieldLength(0,1),3);
	assertEqInt(cur.getFieldLength(0,2),9);
	assertEqInt(cur.getFieldLength(0,3),12);
	assertEqInt(cur.getFieldLength(0,4),9);
	assertEqInt(cur.getFieldLength(0,5),9);
	console.log("");
	assertEqInt(cur.getFieldLength(7,0),1);
	assertEqInt(cur.getFieldLength(7,1),3);
	assertEqInt(cur.getFieldLength(7,2),9);
	assertEqInt(cur.getFieldLength(7,3),12);
	assertEqInt(cur.getFieldLength(7,4),9);
	assertEqInt(cur.getFieldLength(7,5),9);
	console.log("");


	// fields by name
	console.log("FIELDS BY NAME: ");
	assertEqStr(cur.getField(0,"testint"),"1");
	assertEqStr(cur.getField(0,"testfloat"),"1.5");
	assertEqStr(cur.getField(0,"testchar"),"testchar1");
	assertEqStr(cur.getField(0,"testvarchar"),"testvarchar1");
	assertEqStr(cur.getField(0,"testclob"),"testclob1");
	assertEqStr(cur.getField(0,"testblob"),"testblob1");
	console.log("");
	assertEqStr(cur.getField(7,"testint"),"8");
	assertEqStr(cur.getField(7,"testfloat"),"8.5");
	assertEqStr(cur.getField(7,"testchar"),"testchar8");
	assertEqStr(cur.getField(7,"testvarchar"),"testvarchar8");
	assertEqStr(cur.getField(7,"testclob"),"testclob8");
	assertEqStr(cur.getField(7,"testblob"),"testblob8");
	console.log("");


	// field lengths by name
	console.log("FIELD LENGTHS BY NAME: ");
	assertEqInt(cur.getFieldLength(0,"testint"),1);
	assertEqInt(cur.getFieldLength(0,"testfloat"),3);
	assertEqInt(cur.getFieldLength(0,"testchar"),9);
	assertEqInt(cur.getFieldLength(0,"testvarchar"),12);
	assertEqInt(cur.getFieldLength(0,"testclob"),9);
	assertEqInt(cur.getFieldLength(0,"testblob"),9);
	console.log("");
	assertEqInt(cur.getFieldLength(7,"testint"),1);
	assertEqInt(cur.getFieldLength(7,"testfloat"),3);
	assertEqInt(cur.getFieldLength(7,"testchar"),9);
	assertEqInt(cur.getFieldLength(7,"testvarchar"),12);
	assertEqInt(cur.getFieldLength(7,"testclob"),9);
	assertEqInt(cur.getFieldLength(7,"testblob"),9);
	console.log("");


	// fields by array
	console.log("FIELDS BY ARRAY: ");
	var fields=cur.getRow(0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1.5");
	assertEqStr(fields[2],"testchar1");
	assertEqStr(fields[3],"testvarchar1");
	assertEqStr(fields[4],"testclob1");
	assertEqStr(fields[5],"testblob1");
	console.log("");


	// field lengths by array
	console.log("FIELD LENGTHS BY ARRAY: ");
	var fieldlens=cur.getRowLengths(0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],3);
	assertEqInt(fieldlens[2],9);
	assertEqInt(fieldlens[3],12);
	assertEqInt(fieldlens[4],9);
	assertEqInt(fieldlens[5],9);
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
	assertEqInt(cur.getColumnLength(0),0);
	assertEqStr(cur.getColumnType(0),
				(issqlite3)?"INTEGER":"UNKNOWN");
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
	assertEqInt(cur.colCount(),6);
	console.log("");


	// column names for cached result set
	console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
	assertEqStr(cur.getColumnName(0),"testint");
	assertEqStr(cur.getColumnName(1),"testfloat");
	assertEqStr(cur.getColumnName(2),"testchar");
	assertEqStr(cur.getColumnName(3),"testvarchar");
	assertEqStr(cur.getColumnName(4),"testclob");
	assertEqStr(cur.getColumnName(5),"testblob");
	cols=cur.getColumnNames();
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testfloat");
	assertEqStr(cols[2],"testchar");
	assertEqStr(cols[3],"testvarchar");
	assertEqStr(cols[4],"testclob");
	assertEqStr(cols[5],"testblob");
	console.log("");


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
	console.log("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: ");
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
	console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ");
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
	assertTrue(cur.sendQuery("select * from testtable"));
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
	setSecondCursor(secondcur);
	secondcur.setResultSetBufferSize(1);
	for (var i=0; cur.getRow(i); i++) {
		assertTrue(secondcur.sendQuery(
			"select * from testtable"));
	}
	// the nested selects must not disturb the outer result set
	assertEqInt(i,cur.rowCount());
	secondcur.closeResultSet();
	cur.setResultSetBufferSize(0);
	assertTrue(cur.sendQuery("drop table if exists testtable"));
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
	// sqlite DDL is transactional; commit so the table is visible
	// to the second connection (the commit implicitly starts a new tx)
	assertTrue(con.commit());
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
	cur.sendQuery("drop table if exists testtable");
	assertTrue(cur.sendQuery(
		"create table testtable ("+
		"	col1 int, "+
		"	col2 char, "+
		"	col3 float)"));
	cur.prepareQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	$(var1), "+
		"	'$(var2)', "+
		"	$(var3))");
	cur.substitution("var1",1);
	cur.substitution("var2","hello");
	cur.substitution("var3",10.5556,6,4);
	assertTrue(cur.executeQuery());
	assertTrue(cur.sendQuery("select * from testtable"));
	assertEqStr(cur.getField(0,0),"1");
	assertEqStr(cur.getField(0,1),"hello");
	assertEqStr(cur.getField(0,2),"10.5556");
	assertTrue(cur.sendQuery("delete from testtable"));
	console.log("");


	// array substitutions
	console.log("ARRAY SUBSTITUTIONS: ");
	cur.prepareQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	'$(var1)', "+
		"	'$(var2)', "+
		"	'$(var3)')");
	cur.substitutions(subvars,subvalstrings,null,null);
	assertTrue(cur.executeQuery());
	assertTrue(cur.sendQuery("select * from testtable"));
	assertEqStr(cur.getField(0,0),"hi");
	assertEqStr(cur.getField(0,1),"hello");
	assertEqStr(cur.getField(0,2),"bye");
	assertTrue(cur.sendQuery("delete from testtable"));
	console.log("");
	cur.prepareQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	$(var1), "+
		"	'$(var2)', "+
		"	$(var3))");
	cur.substitutions(subvars,subvallongs,null,null);
	assertTrue(cur.executeQuery());
	assertTrue(cur.sendQuery("select * from testtable"));
	assertEqStr(cur.getField(0,0),"1");
	assertEqStr(cur.getField(0,1),"2");
	assertEqStr(cur.getField(0,2),"3.0");
	assertTrue(cur.sendQuery("delete from testtable"));
	console.log("");
	cur.prepareQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	$(var1), "+
		"	'$(var2)', "+
		"	$(var3))");
	cur.substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur.executeQuery());
	assertTrue(cur.sendQuery("select * from testtable"));
	assertEqStr(cur.getField(0,0),"10.55");
	assertEqStr(cur.getField(0,1),"10.556");
	assertEqStr(cur.getField(0,2),"10.5556");
	assertTrue(cur.sendQuery("delete from testtable"));
	console.log("");


	// nulls as nulls
	console.log("NULLS AS NULLS: ");
	cur.getNullsAsNulls();
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	1, "+
		"	NULL, "+
		"	NULL)"));
	assertTrue(cur.sendQuery("select * from testtable"));
	assertEqStr(cur.getField(0,0),"1");
	assertEqStr(cur.getField(0,1),null);
	assertEqStr(cur.getField(0,2),null);
	cur.getNullsAsEmptyStrings();
	assertTrue(cur.sendQuery("select * from testtable"));
	assertEqStr(cur.getField(0,0),"1");
	assertEqStr(cur.getField(0,1),"");
	assertEqStr(cur.getField(0,2),"");
	assertTrue(cur.sendQuery("drop table if exists testtable"));
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
	cur.prepareQuery("insert into testtable "+
		"values (:clobval,:blobval)");
	var largebuffer="C".repeat(8192);
	cur.inputBindClob("clobval",largebuffer,largebuffer.length);
	cur.inputBindBlob("blobval",largebuffer,largebuffer.length);
	assertTrue(cur.executeQuery());
	cur.sendQuery("select * from testtable");
	assertEqInt(cur.getFieldLength(0,"testclob"),8192);
	assertEqStr(cur.getField(0,"testclob"),largebuffer);
	assertEqInt(cur.getFieldLength(0,"testblob"),8192);
	assertEqStrLen(cur.getField(0,"testblob"),largebuffer,8192);
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// output bind by position
	// sqlite doesn't support output binds


	// output bind by name
	// sqlite doesn't support output binds


	// output bind by name with validation
	// sqlite doesn't support output binds


	// lob output bind
	// sqlite doesn't support output binds


	// long output bind
	// sqlite doesn't support output binds


	// negative input bind
	console.log("NEGATIVE INPUT BIND: ");
	cur.sendQuery("drop table testtable");
	cur.sendQuery("create table testtable (testval int)");
	cur.prepareQuery("insert into testtable values (:testval)");
	cur.inputBind("testval",-1);
	assertTrue(cur.executeQuery());
	cur.sendQuery("select testval from testtable");
	assertEqStr(cur.getField(0,"testval"),"-1");
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


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
	cur.prepareQuery("select :val");
	cur.inputBind("val",1);
	assertTrue(cur.executeQuery());
	assertEqStr(cur.getField(0,0),"1");
	cur.inputBind("val",2);
	assertTrue(cur.executeQuery());
	assertEqStr(cur.getField(0,0),"2");
	cur.inputBind("val",3);
	assertTrue(cur.executeQuery());
	assertEqStr(cur.getField(0,0),"3");
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
	cur.prepareQuery("select :var");
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
	// sqlite doesn't support stored procedures


	// stored procedure returning single value
	// sqlite doesn't support stored procedures


	// stored procedure returning multiple values
	// sqlite doesn't support stored procedures


	// stored procedure returning result set
	// sqlite doesn't support stored procedures


	// temporary tables
	console.log("TEMPORARY TABLES: ");
	cur.sendQuery("drop table if exists temptable\n");
	cur.sendQuery("create temporary table temptable (col1 int)");
	assertTrue(cur.sendQuery("insert into temptable values (1)"));
	assertTrue(cur.sendQuery("select count(*) from temptable"));
	assertEqStr(cur.getField(0,0),"1");
	con.endSession();
	console.log("");
	assertFalse(cur.sendQuery("select count(*) from temptable"));
	assertTrue(cur.sendQuery("drop table if exists temptable\n"));
	console.log("");


	// encoded binary data
	console.log("ENCODED BINARY DATA: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery("create table testtable "+
		"(col1 blob)"));
	var buffer="";
	for (var j=0; j<256; j++) {
		buffer+=String.fromCharCode(j);
	}
	var querystr="insert into testtable values (X'";
	for (var i=0; i<256; i++) {
		querystr+=("0"+buffer.charCodeAt(i).toString(16)).slice(-2);
	}
	querystr+="')";
	assertTrue(cur.sendQuery(querystr));
	// Verify round-tripped bytes via server-side hex (the binding's
	// getField returns strings via String::NewFromUtf8, which drops
	// invalid UTF-8 byte sequences that arise from raw bytes 128-255).
	assertTrue(cur.sendQuery("select hex(col1) from testtable"));
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
	assertEqStr(cur.getField(0,0),"''");
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// last insert id
	console.log("LAST INSERT ID: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery(
		"create table testtable "+
		"	(col1 integer primary key "+
		"	autoincrement, "+
		"	col2 int)"));
	assertTrue(cur.sendQuery("insert into testtable "+
		"values (null,1)"));
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
	console.log("");


	// schema list
	console.log("SCHEMA LIST: ");
	assertTrue(cur.getSchemaList(null));
	assertEqStr(cur.getColumnName(0),"Database");
	console.log("");


	// table type list
	console.log("TABLE TYPE LIST: ");
	assertTrue(cur.getTableTypeList());
	assertEqStr(cur.getColumnName(0),"table_type");
	assertInResultSet(cur,"table_type","TABLE");
	console.log("");


	// table list
	console.log("TABLE LIST: ");
	cur.sendQuery("drop table if exists testtable1");
	cur.sendQuery("drop table if exists testtable2");
	cur.sendQuery("drop table if exists testtable3");
	cur.sendQuery("drop table if exists testtable4");
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
	assertTrue(cur.sendQuery("drop table if exists testtable1"));
	assertTrue(cur.sendQuery("drop table if exists testtable2"));
	assertTrue(cur.sendQuery("drop table if exists testtable3"));
	assertTrue(cur.sendQuery("drop table if exists testtable4"));
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
	assertEqStr(cur.getField(0,"precision"),"19");
	assertEqStr(cur.getField(0,"local_type_name"),"INTEGER");
	assertTrue(cur.getTypeInfoList("char"));
	assertEqStr(cur.getField(0,"type_name"),"CHAR");
	assertEqStr(cur.getField(0,"data_type"),"1");
	assertEqStr(cur.getField(0,"precision"),"2147483647");
	assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
	assertTrue(cur.getTypeInfoList("varchar"));
	assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
	assertEqStr(cur.getField(0,"data_type"),"12");
	assertEqStr(cur.getField(0,"precision"),"2147483647");
	assertEqStr(cur.getField(0,"local_type_name"),"VARCHAR");
	assertTrue(cur.getTypeInfoList("date"));
	assertEqStr(cur.getField(0,"type_name"),"DATE");
	assertEqStr(cur.getField(0,"data_type"),"91");
	assertEqStr(cur.getField(0,"precision"),"10");
	assertEqStr(cur.getField(0,"local_type_name"),"DATE");
	console.log("");


	// column list
	console.log("COLUMN LIST: ");
	cur.sendQuery("drop table if exists testtable");
	assertTrue(cur.sendQuery(
		"create table testtable ("+
		"	testint int, "+
		"	testfloat float, "+
		"	testchar char(40), "+
		"	testvarchar varchar(40), "+
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
	assertEqStr(cur.getField(0,"column_name"),"testint");
	assertEqStr(cur.getField(1,"column_name"),"testfloat");
	assertEqStr(cur.getField(2,"column_name"),"testchar");
	assertEqStr(cur.getField(3,"column_name"),"testvarchar");
	assertEqStr(cur.getField(4,"column_name"),"testclob");
	assertEqStr(cur.getField(5,"column_name"),"testblob");
	assertEqStr(cur.getField(0,"data_type"),"INT");
	assertEqStr(cur.getField(1,"data_type"),"FLOAT");
	assertEqStr(cur.getField(2,"data_type"),"CHAR");
	assertEqStr(cur.getField(3,"data_type"),"VARCHAR");
	assertEqStr(cur.getField(4,"data_type"),"CLOB");
	assertEqStr(cur.getField(5,"data_type"),"BLOB");
	assertTrue(cur.sendQuery("drop table if exists testtable"));
	console.log("");


	// column list - auto_increment,
	// primary key
	console.log("COLUMN LIST - auto_increment, primary key: ");
	cur.sendQuery("drop table if exists testtable");
	assertTrue(cur.sendQuery(
		"create table testtable ("+
		"	col1 integer primary key "+
		"	autoincrement, "+
		"	col2 int)"));
	assertTrue(cur.getColumnList("testtable",null));
	assertTrue(cur.getField(0,"extra").indexOf("auto_increment")!==-1);
	assertTrue(cur.getField(0,"column_key").indexOf("PRI")!==-1);
	assertFalse(cur.getField(1,"extra").indexOf("auto_increment")!==-1);
	assertFalse(cur.getField(1,"column_key").indexOf("PRI")!==-1);
	console.log("");
	assertTrue(cur.sendQuery("drop table if exists testtable"));
	assertTrue(cur.sendQuery(
		"create table testtable ("+
		"	col1 int primary key, "+
		"	col2 int)"));
	assertTrue(cur.getColumnList("testtable",null));
	assertFalse(cur.getField(0,"extra").indexOf("auto_increment")!==-1);
	assertTrue(cur.getField(0,"column_key").indexOf("PRI")!==-1);
	assertTrue(cur.sendQuery("drop table if exists testtable"));
	console.log("");


	// primary keys list
	console.log("PRIMARY KEYS LIST: ");
	cur.sendQuery("drop table if exists testtable");
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
	assertTrue(cur.sendQuery("drop table if exists testtable"));
	console.log("");


	// key and index list
	console.log("KEY AND INDEX LIST: ");
	cur.sendQuery("drop table if exists testtable");
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
	assertEqStr(cur.getField(0,"non_unique"),"0");
	assertEqStr(cur.getField(0,"seq_in_index"),"1");
	assertTrue(cur.getField(0,"column_name")=="col1");
	assertEqStr(cur.getField(0,"collation"),"A");
	assertEqStr(cur.getField(0,"index_type"),"3");
	var kn=cur.getField(0,"key_name");
	assertTrue(!((!kn) || (!kn[0])));
	assertTrue(cur.sendQuery("drop table if exists testtable"));
	console.log("");


	// procedure list
	console.log("PROCEDURE LIST: ");
	assertTrue(cur.getProcedureList(null));
	assertEqInt(cur.rowCount(),0);
	console.log("");


	// procedure parameter list
	console.log("PROCEDURE PARAMETER LIST: ");
	assertTrue(cur.getProcedureParameterList("testproc1",null));
	assertEqStr(cur.getColumnName(0),"parameter_name");
	assertEqStr(cur.getColumnName(1),"parameter_mode");
	assertEqStr(cur.getColumnName(2),"data_type");
	assertEqStr(cur.getColumnName(3),"character_maximum_length");
	assertEqStr(cur.getColumnName(4),"ordinal_position");
	assertEqInt(cur.rowCount(),0);
	console.log("");


	// invalid queries
	console.log("INVALID QUERIES: ");
	assertFalse(cur.sendQuery("select * from testtable"));
	assertFalse(cur.sendQuery("select * from testtable"));
	assertFalse(cur.sendQuery("select * from testtable"));
	assertFalse(cur.sendQuery("select * from testtable"));
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
