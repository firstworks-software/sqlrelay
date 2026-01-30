// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{assertEqual, getStatus, reportTestStatus}=require("./asserts.js");


	
var	dbtype;
var	bindvars=["1","2","3","4","5","6",
			"7","8","9","10","11"];
var	bindvals=["4","4","4.4","4.4","4.4","4.4",
			"01-JAN-2004","04:00:00",
			"testchar4","testvarchar4",null];
var	subvars=["var1","var2","var3"];
var	subvalstrings=["hi","hello","bye"];
var	subvallongs=[1,2,3];
var	subvaldoubles=[10.55,10.556,10.5556];
var	precs=[4,5,6];
var	scales=[2,3,4];
var	numvar;
var	stringvar;
var	floatvar;
var	cols;
var	fields;
var	port;
var	socket;
var	id;
var	filename;
var	fieldlens;
	
// instantiation
var	con=new sqlrelay.SQLRConnection("sqlrelay",
					9000,
					"/tmp/test.socket",
					"testuser","testpassword",0,1);
var	cur=new sqlrelay.SQLRCursor(con);
	
// get database type


// identify
console.log("IDENTIFY: ");
assertEqual(con.identify(),"firebird");
console.log("\n");


// ping
console.log("PING: ");
assertEqual(con.ping(),1);
console.log("\n");


// isolation levels
console.log("ISOLATION LEVELS: ");
// though firebird does support a "set transaction ..." statement to
// set the isolation level, it looks like, in firebird, you can really
// only set it through the TPB at the start of a transaction, so
// attempts to set it should fail
assertEqual(con.setIsolationLevel("read committed"),0);
assertEqual(con.getIsolationLevel(),"read committed");
console.log("\n");

// clear table
cur.sendQuery("delete from testtable");
con.commit();


// insert
console.log("INSERT: ");
assertEqual(cur.sendQuery("insert into testtable values (1,1,1.1,1.1,1.1,1.1,'01-JAN-2001','01:00:00','testchar1','testvarchar1',null,null)"),1);
console.log("\n");


// bind by position
console.log("BIND BY POSITION: ");

cur.prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,null)");
assertEqual(cur.countBindVariables(),11);
cur.inputBind("1",2);
cur.inputBind("2",2);
cur.inputBind("3",2.2,2,1);
cur.inputBind("4",2.2,2,1);
cur.inputBind("5",2.2,2,1);
cur.inputBind("6",2.2,2,1);
cur.inputBind("7","01-JAN-2002");
cur.inputBind("8","02:00:00");
cur.inputBind("9","testchar2");
cur.inputBind("10","testvarchar2");
cur.inputBind("11",null);
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2",3);
cur.inputBind("3",3.3,2,1);
cur.inputBind("4",3.3,2,1);
cur.inputBind("5",3.3,2,1);
cur.inputBind("6",3.3,2,1);
cur.inputBind("7","01-JAN-2003");
cur.inputBind("8","03:00:00");
cur.inputBind("9","testchar3");
cur.inputBind("10","testvarchar3");
cur.inputBind("11",null);
assertEqual(cur.executeQuery(),1);
console.log("\n");


// array of binds by position
console.log("ARRAY OF BINDS BY POSITION: ");
cur.clearBinds();
cur.inputBinds(bindvars,bindvals);
assertEqual(cur.executeQuery(),1);
console.log("\n");


// insert
console.log("INSERT: ");
assertEqual(cur.sendQuery("insert into testtable values (5,5,5.5,5.5,5.5,5.5,'01-JAN-2005','05:00:00','testchar5','testvarchar5',null,null)"),1);
assertEqual(cur.sendQuery("insert into testtable values (6,6,6.6,6.6,6.6,6.6,'01-JAN-2006','06:00:00','testchar6','testvarchar6',null,null)"),1);
assertEqual(cur.sendQuery("insert into testtable values (7,7,7.7,7.7,7.7,7.7,'01-JAN-2007','07:00:00','testchar7','testvarchar7',null,null)"),1);
assertEqual(cur.sendQuery("insert into testtable values (8,8,8.8,8.8,8.8,8.8,'01-JAN-2008','08:00:00','testchar8','testvarchar8',null,null)"),1);
console.log("\n");


// affected rows
console.log("AFFECTED ROWS: ");
assertEqual(cur.affectedRows(),0);
console.log("\n");


// stored procedure
console.log("STORED PROCEDURE: ");
cur.prepareQuery("select * from testproc(?,?,?,null)");
cur.inputBind("1",1);
cur.inputBind("2",1.1,2,1);
cur.inputBind("3","hello");
assertEqual(cur.executeQuery(),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"1.1000");
assertEqual(cur.getField(0,2),"hello");
cur.prepareQuery("execute procedure testproc ?, ?, ?, null");
cur.inputBind("1",1);
cur.inputBind("2",1.1,2,1);
cur.inputBind("3","hello");
cur.defineOutputBindInteger("1");
cur.defineOutputBindDouble("2");
cur.defineOutputBindString("3",20);
cur.defineOutputBindBlob("4");
assertEqual(cur.executeQuery(),1);
assertEqual(cur.getOutputBindInteger("1"),1);
//assertEqual(cur.getOutputBindDouble("2"),1.1);
assertEqual(cur.getOutputBindString("3"),"hello               ");
console.log("\n");


// select
console.log("SELECT: ");
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
console.log("\n");


// column count
console.log("COLUMN COUNT: ");
assertEqual(cur.colCount(),12);
console.log("\n");


// column names
console.log("COLUMN NAMES: ");
assertEqual(cur.getColumnName(0),"TESTINTEGER");
assertEqual(cur.getColumnName(1),"TESTSMALLINT");
assertEqual(cur.getColumnName(2),"TESTDECIMAL");
assertEqual(cur.getColumnName(3),"TESTNUMERIC");
assertEqual(cur.getColumnName(4),"TESTFLOAT");
assertEqual(cur.getColumnName(5),"TESTDOUBLE");
assertEqual(cur.getColumnName(6),"TESTDATE");
assertEqual(cur.getColumnName(7),"TESTTIME");
assertEqual(cur.getColumnName(8),"TESTCHAR");
assertEqual(cur.getColumnName(9),"TESTVARCHAR");
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP");
cols=cur.getColumnNames();
assertEqual(cols[0],"TESTINTEGER");
assertEqual(cols[1],"TESTSMALLINT");
assertEqual(cols[2],"TESTDECIMAL");
assertEqual(cols[3],"TESTNUMERIC");
assertEqual(cols[4],"TESTFLOAT");
assertEqual(cols[5],"TESTDOUBLE");
assertEqual(cols[6],"TESTDATE");
assertEqual(cols[7],"TESTTIME");
assertEqual(cols[8],"TESTCHAR");
assertEqual(cols[9],"TESTVARCHAR");
assertEqual(cols[10],"TESTTIMESTAMP");
console.log("\n");


// column types
console.log("COLUMN TYPES: ");
assertEqual(cur.getColumnType(0),"INTEGER");
assertEqual(cur.getColumnType("TESTINTEGER"),"INTEGER");
assertEqual(cur.getColumnType(1),"SMALLINT");
assertEqual(cur.getColumnType("TESTSMALLINT"),"SMALLINT");
assertEqual(cur.getColumnType(2),"DECIMAL");
assertEqual(cur.getColumnType("TESTDECIMAL"),"DECIMAL");
assertEqual(cur.getColumnType(3),"NUMERIC");
assertEqual(cur.getColumnType("TESTNUMERIC"),"NUMERIC");
assertEqual(cur.getColumnType(4),"FLOAT");
assertEqual(cur.getColumnType("TESTFLOAT"),"FLOAT");
assertEqual(cur.getColumnType(5),"DOUBLE PRECISION");
assertEqual(cur.getColumnType("TESTDOUBLE"),"DOUBLE PRECISION");
assertEqual(cur.getColumnType(6),"DATE");
assertEqual(cur.getColumnType("TESTDATE"),"DATE");
assertEqual(cur.getColumnType(7),"TIME");
assertEqual(cur.getColumnType("TESTTIME"),"TIME");
assertEqual(cur.getColumnType(8),"CHAR");
assertEqual(cur.getColumnType("TESTCHAR"),"CHAR");
assertEqual(cur.getColumnType(9),"VARCHAR");
assertEqual(cur.getColumnType("TESTVARCHAR"),"VARCHAR");
assertEqual(cur.getColumnType(10),"TIMESTAMP");
assertEqual(cur.getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
console.log("\n");


// column length
console.log("COLUMN LENGTH: ");
assertEqual(cur.getColumnLength(0),4);
assertEqual(cur.getColumnLength("TESTINTEGER"),4);
assertEqual(cur.getColumnLength(1),2);
assertEqual(cur.getColumnLength("TESTSMALLINT"),2);
assertEqual(cur.getColumnLength(2),8);
assertEqual(cur.getColumnLength("TESTDECIMAL"),8);
assertEqual(cur.getColumnLength(3),8);
assertEqual(cur.getColumnLength("TESTNUMERIC"),8);
assertEqual(cur.getColumnLength(4),4);
assertEqual(cur.getColumnLength("TESTFLOAT"),4);
assertEqual(cur.getColumnLength(5),8);
assertEqual(cur.getColumnLength("TESTDOUBLE"),8);
assertEqual(cur.getColumnLength(6),4);
assertEqual(cur.getColumnLength("TESTDATE"),4);
assertEqual(cur.getColumnLength(7),4);
assertEqual(cur.getColumnLength("TESTTIME"),4);
assertEqual(cur.getColumnLength(8),50);
assertEqual(cur.getColumnLength("TESTCHAR"),50);
assertEqual(cur.getColumnLength(9),50);
assertEqual(cur.getColumnLength("TESTVARCHAR"),50);
assertEqual(cur.getColumnLength(10),8);
assertEqual(cur.getColumnLength("TESTTIMESTAMP"),8);
console.log("\n");


// longest column
console.log("LONGEST COLUMN: ");
assertEqual(cur.getLongest(0),1);
assertEqual(cur.getLongest("TESTINTEGER"),1);
assertEqual(cur.getLongest(1),1);
assertEqual(cur.getLongest("TESTSMALLINT"),1);
assertEqual(cur.getLongest(2),4);
assertEqual(cur.getLongest("TESTDECIMAL"),4);
assertEqual(cur.getLongest(3),4);
assertEqual(cur.getLongest("TESTNUMERIC"),4);
assertEqual(cur.getLongest(4),6);
assertEqual(cur.getLongest("TESTFLOAT"),6);
assertEqual(cur.getLongest(5),6);
assertEqual(cur.getLongest("TESTDOUBLE"),6);
assertEqual(cur.getLongest(6),10);
assertEqual(cur.getLongest("TESTDATE"),10);
assertEqual(cur.getLongest(7),8);
assertEqual(cur.getLongest("TESTTIME"),8);
assertEqual(cur.getLongest(8),50);
assertEqual(cur.getLongest("TESTCHAR"),50);
assertEqual(cur.getLongest(9),12);
assertEqual(cur.getLongest("TESTVARCHAR"),12);
assertEqual(cur.getLongest(10),0);
assertEqual(cur.getLongest("TESTTIMESTAMP"),0);
console.log("\n");


// row count
console.log("ROW COUNT: ");
assertEqual(cur.rowCount(),8);
console.log("\n");


// total rows
console.log("TOTAL ROWS: ");
assertEqual(cur.totalRows(),0);
console.log("\n");


// first row index
console.log("FIRST ROW INDEX: ");
assertEqual(cur.firstRowIndex(),0);
console.log("\n");


// end of result set
console.log("END OF RESULT SET: ");
assertEqual(cur.endOfResultSet(),1);
console.log("\n");


// fields by index
console.log("FIELDS BY INDEX: ");
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"1");
assertEqual(cur.getField(0,2),"1.10");
assertEqual(cur.getField(0,3),"1.10");
assertEqual(cur.getField(0,4),"1.1000");
assertEqual(cur.getField(0,5),"1.1000");
assertEqual(cur.getField(0,6),"2001:01:01");
assertEqual(cur.getField(0,7),"01:00:00");
assertEqual(cur.getField(0,8),"testchar1                                         ");
assertEqual(cur.getField(0,9),"testvarchar1");
console.log();
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(7,1),"8");
assertEqual(cur.getField(7,2),"8.80");
assertEqual(cur.getField(7,3),"8.80");
assertEqual(cur.getField(7,4),"8.8000");
assertEqual(cur.getField(7,5),"8.8000");
assertEqual(cur.getField(7,6),"2008:01:01");
assertEqual(cur.getField(7,7),"08:00:00");
assertEqual(cur.getField(7,8),"testchar8                                         ");
assertEqual(cur.getField(7,9),"testvarchar8");
console.log("\n");


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqual(cur.getFieldLength(0,0),1);
assertEqual(cur.getFieldLength(0,1),1);
assertEqual(cur.getFieldLength(0,2),4);
assertEqual(cur.getFieldLength(0,3),4);
assertEqual(cur.getFieldLength(0,4),6);
assertEqual(cur.getFieldLength(0,5),6);
assertEqual(cur.getFieldLength(0,6),10);
assertEqual(cur.getFieldLength(0,7),8);
assertEqual(cur.getFieldLength(0,8),50);
assertEqual(cur.getFieldLength(0,9),12);
console.log();
assertEqual(cur.getFieldLength(7,0),1);
assertEqual(cur.getFieldLength(7,1),1);
assertEqual(cur.getFieldLength(7,2),4);
assertEqual(cur.getFieldLength(7,3),4);
assertEqual(cur.getFieldLength(7,4),6);
assertEqual(cur.getFieldLength(7,5),6);
assertEqual(cur.getFieldLength(7,6),10);
assertEqual(cur.getFieldLength(7,7),8);
assertEqual(cur.getFieldLength(7,8),50);
assertEqual(cur.getFieldLength(7,9),12);
console.log("\n");


// fields by name
console.log("FIELDS BY NAME: ");
assertEqual(cur.getField(0,"TESTINTEGER"),"1");
assertEqual(cur.getField(0,"TESTSMALLINT"),"1");
assertEqual(cur.getField(0,"TESTDECIMAL"),"1.10");
assertEqual(cur.getField(0,"TESTNUMERIC"),"1.10");
assertEqual(cur.getField(0,"TESTFLOAT"),"1.1000");
assertEqual(cur.getField(0,"TESTDOUBLE"),"1.1000");
assertEqual(cur.getField(0,"TESTDATE"),"2001:01:01");
assertEqual(cur.getField(0,"TESTTIME"),"01:00:00");
assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                                         ");
assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
console.log();
assertEqual(cur.getField(7,"TESTINTEGER"),"8");
assertEqual(cur.getField(7,"TESTSMALLINT"),"8");
assertEqual(cur.getField(7,"TESTDECIMAL"),"8.80");
assertEqual(cur.getField(7,"TESTNUMERIC"),"8.80");
assertEqual(cur.getField(7,"TESTFLOAT"),"8.8000");
assertEqual(cur.getField(7,"TESTDOUBLE"),"8.8000");
assertEqual(cur.getField(7,"TESTDATE"),"2008:01:01");
assertEqual(cur.getField(7,"TESTTIME"),"08:00:00");
assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                                         ");
assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
console.log("\n");


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqual(cur.getFieldLength(0,"TESTINTEGER"),1);
assertEqual(cur.getFieldLength(0,"TESTSMALLINT"),1);
assertEqual(cur.getFieldLength(0,"TESTDECIMAL"),4);
assertEqual(cur.getFieldLength(0,"TESTNUMERIC"),4);
assertEqual(cur.getFieldLength(0,"TESTFLOAT"),6);
assertEqual(cur.getFieldLength(0,"TESTDOUBLE"),6);
assertEqual(cur.getFieldLength(0,"TESTDATE"),10);
assertEqual(cur.getFieldLength(0,"TESTTIME"),8);
assertEqual(cur.getFieldLength(0,"TESTCHAR"),50);
assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12);
console.log();
assertEqual(cur.getFieldLength(7,"TESTINTEGER"),1);
assertEqual(cur.getFieldLength(7,"TESTSMALLINT"),1);
assertEqual(cur.getFieldLength(7,"TESTDECIMAL"),4);
assertEqual(cur.getFieldLength(7,"TESTNUMERIC"),4);
assertEqual(cur.getFieldLength(7,"TESTFLOAT"),6);
assertEqual(cur.getFieldLength(7,"TESTDOUBLE"),6);
assertEqual(cur.getFieldLength(7,"TESTDATE"),10);
assertEqual(cur.getFieldLength(7,"TESTTIME"),8);
assertEqual(cur.getFieldLength(7,"TESTCHAR"),50);
assertEqual(cur.getFieldLength(7,"TESTVARCHAR"),12);
console.log("\n");


// fields by array
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqual(fields[0],"1");
assertEqual(fields[1],"1");
assertEqual(fields[2],"1.10");
assertEqual(fields[3],"1.10");
assertEqual(fields[4],"1.1000");
assertEqual(fields[5],"1.1000");
assertEqual(fields[6],"2001:01:01");
assertEqual(fields[7],"01:00:00");
assertEqual(fields[8],"testchar1                                         ");
assertEqual(fields[9],"testvarchar1");
console.log("\n");


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqual(fieldlens[0],1);
assertEqual(fieldlens[1],1);
assertEqual(fieldlens[2],4);
assertEqual(fieldlens[3],4);
assertEqual(fieldlens[4],6);
assertEqual(fieldlens[5],6);
assertEqual(fieldlens[6],10);
assertEqual(fieldlens[7],8);
assertEqual(fieldlens[8],50);
assertEqual(fieldlens[9],12);
console.log("\n");


// individual substitutions
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),'$(var2)','$(var3)' from rdb$database");
cur.substitution("var1",1);
cur.substitution("var2","hello");
cur.substitution("var3",10.5556,6,4);
assertEqual(cur.executeQuery(),1);
console.log("\n");


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"hello");
assertEqual(cur.getField(0,2),"10.5556");
console.log("\n");


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)' from rdb$database");
cur.substitutions(subvars,subvalstrings);
assertEqual(cur.executeQuery(),1);
console.log("\n");


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"hi");
assertEqual(cur.getField(0,1),"hello");
assertEqual(cur.getField(0,2),"bye");
console.log("\n");


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
cur.substitutions(subvars,subvallongs);
assertEqual(cur.executeQuery(),1);
console.log("\n");


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"2");
assertEqual(cur.getField(0,2),"3");
console.log("\n");


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertEqual(cur.executeQuery(),1);
console.log("\n");


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"10.55");
assertEqual(cur.getField(0,1),"10.556");
assertEqual(cur.getField(0,2),"10.5556");
console.log("\n");
	
console.log("nullS as Nulls: ");
cur.getNullsAsNulls();
assertEqual(cur.sendQuery("select 1,null,null from rdb$database"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),null);
assertEqual(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertEqual(cur.sendQuery("select 1,null,null from rdb$database"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"");
assertEqual(cur.getField(0,2),"");
cur.getNullsAsNulls();
console.log("\n");


// result set buffer size
console.log("RESULT SET BUFFER SIZE: ");
assertEqual(cur.getResultSetBufferSize(),0);
cur.setResultSetBufferSize(2);
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
assertEqual(cur.getResultSetBufferSize(),2);
console.log();
assertEqual(cur.firstRowIndex(),0);
assertEqual(cur.endOfResultSet(),0);
assertEqual(cur.rowCount(),2);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(1,0),"2");
assertEqual(cur.getField(2,0),"3");
console.log();
assertEqual(cur.firstRowIndex(),2);
assertEqual(cur.endOfResultSet(),0);
assertEqual(cur.rowCount(),4);
assertEqual(cur.getField(6,0),"7");
assertEqual(cur.getField(7,0),"8");
console.log();
assertEqual(cur.firstRowIndex(),6);
assertEqual(cur.endOfResultSet(),0);
assertEqual(cur.rowCount(),8);
assertEqual(cur.getField(8,0),null);
console.log();
assertEqual(cur.firstRowIndex(),8);
assertEqual(cur.endOfResultSet(),1);
assertEqual(cur.rowCount(),8);
console.log("\n");


// dont get column info
console.log("DONT GET COLUMN INFO: ");
cur.dontGetColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
assertEqual(cur.getColumnName(0),null);
assertEqual(cur.getColumnLength(0),0);
assertEqual(cur.getColumnType(0),null);
cur.getColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
assertEqual(cur.getColumnName(0),"TESTINTEGER");
assertEqual(cur.getColumnLength(0),4);
assertEqual(cur.getColumnType(0),"INTEGER");
console.log("\n");


// suspended session
console.log("SUSPENDED SESSION: ");
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
cur.suspendResultSet();
assertEqual(con.suspendSession(),1);
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertEqual(con.resumeSession(port,socket),1);
console.log();
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(1,0),"2");
assertEqual(cur.getField(2,0),"3");
assertEqual(cur.getField(3,0),"4");
assertEqual(cur.getField(4,0),"5");
assertEqual(cur.getField(5,0),"6");
assertEqual(cur.getField(6,0),"7");
assertEqual(cur.getField(7,0),"8");
console.log();
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
cur.suspendResultSet();
assertEqual(con.suspendSession(),1);
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertEqual(con.resumeSession(port,socket),1);
console.log();
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(1,0),"2");
assertEqual(cur.getField(2,0),"3");
assertEqual(cur.getField(3,0),"4");
assertEqual(cur.getField(4,0),"5");
assertEqual(cur.getField(5,0),"6");
assertEqual(cur.getField(6,0),"7");
assertEqual(cur.getField(7,0),"8");
console.log();
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
cur.suspendResultSet();
assertEqual(con.suspendSession(),1);
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertEqual(con.resumeSession(port,socket),1);
console.log();
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(1,0),"2");
assertEqual(cur.getField(2,0),"3");
assertEqual(cur.getField(3,0),"4");
assertEqual(cur.getField(4,0),"5");
assertEqual(cur.getField(5,0),"6");
assertEqual(cur.getField(6,0),"7");
assertEqual(cur.getField(7,0),"8");
console.log("\n");


// suspended result set
console.log("SUSPENDED RESULT SET: ");
cur.setResultSetBufferSize(2);
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
assertEqual(cur.getField(2,0),"3");
id=cur.getResultSetId();
cur.suspendResultSet();
assertEqual(con.suspendSession(),1);
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertEqual(con.resumeSession(port,socket),1);
assertEqual(cur.resumeResultSet(id),1);
console.log();
assertEqual(cur.firstRowIndex(),4);
assertEqual(cur.endOfResultSet(),0);
assertEqual(cur.rowCount(),6);
assertEqual(cur.getField(7,0),"8");
console.log();
assertEqual(cur.firstRowIndex(),6);
assertEqual(cur.endOfResultSet(),0);
assertEqual(cur.rowCount(),8);
assertEqual(cur.getField(8,0),null);
console.log();
assertEqual(cur.firstRowIndex(),8);
assertEqual(cur.endOfResultSet(),1);
assertEqual(cur.rowCount(),8);
cur.setResultSetBufferSize(0);
console.log("\n");


// cached result set
console.log("CACHED RESULT SET: ");
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
cur.cacheOff();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
console.log("\n");


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqual(cur.colCount(),12);
console.log("\n");


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqual(cur.getColumnName(0),"TESTINTEGER");
assertEqual(cur.getColumnName(1),"TESTSMALLINT");
assertEqual(cur.getColumnName(2),"TESTDECIMAL");
assertEqual(cur.getColumnName(3),"TESTNUMERIC");
assertEqual(cur.getColumnName(4),"TESTFLOAT");
assertEqual(cur.getColumnName(5),"TESTDOUBLE");
assertEqual(cur.getColumnName(6),"TESTDATE");
assertEqual(cur.getColumnName(7),"TESTTIME");
assertEqual(cur.getColumnName(8),"TESTCHAR");
assertEqual(cur.getColumnName(9),"TESTVARCHAR");
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP");
cols=cur.getColumnNames();
assertEqual(cols[0],"TESTINTEGER");
assertEqual(cols[1],"TESTSMALLINT");
assertEqual(cols[2],"TESTDECIMAL");
assertEqual(cols[3],"TESTNUMERIC");
assertEqual(cols[4],"TESTFLOAT");
assertEqual(cols[5],"TESTDOUBLE");
assertEqual(cols[6],"TESTDATE");
assertEqual(cols[7],"TESTTIME");
assertEqual(cols[8],"TESTCHAR");
assertEqual(cols[9],"TESTVARCHAR");
assertEqual(cols[10],"TESTTIMESTAMP");
console.log("\n");


// cached result set with result set buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
cur.cacheOff();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log("\n");


// from one cache file to another
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2");
assertEqual(cur.openCachedResultSet("cachefile1"),1);
cur.cacheOff();
assertEqual(cur.openCachedResultSet("cachefile2"),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
console.log("\n");


// from one cache file to another with result set buffer size
console.log("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile2");
assertEqual(cur.openCachedResultSet("cachefile1"),1);
cur.cacheOff();
assertEqual(cur.openCachedResultSet("cachefile2"),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log("\n");


// cached result set with suspend and result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
assertEqual(cur.getField(2,0),"3");
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
id=cur.getResultSetId();
cur.suspendResultSet();
assertEqual(con.suspendSession(),1);
port=con.getConnectionPort();
socket=con.getConnectionSocket();
console.log();
assertEqual(con.resumeSession(port,socket),1);
assertEqual(cur.resumeCachedResultSet(id,filename),1);
console.log();
assertEqual(cur.firstRowIndex(),4);
assertEqual(cur.endOfResultSet(),0);
assertEqual(cur.rowCount(),6);
assertEqual(cur.getField(7,0),"8");
console.log();
assertEqual(cur.firstRowIndex(),6);
assertEqual(cur.endOfResultSet(),0);
assertEqual(cur.rowCount(),8);
assertEqual(cur.getField(8,0),null);
console.log();
assertEqual(cur.firstRowIndex(),8);
assertEqual(cur.endOfResultSet(),1);
assertEqual(cur.rowCount(),8);
cur.cacheOff();
console.log();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log("\n");
	
//console.log("COMMIT AND ROLLBACK: ");
var	secondcon=new sqlrelay.SQLRConnection("sqlrelay",
						9000,
						"/tmp/test.socket",
						"testuser","testpassword",0,1);
var	secondcur=new sqlrelay.SQLRCursor(secondcon);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"0");
assertEqual(con.commit(),1);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"8");
assertEqual(con.autoCommitOn(),1);
assertEqual(cur.sendQuery("insert into testtable values (10,10,10.1,10.1,10.1,10.1,'01-JAN-2010','10:00:00','testchar10','testvarchar10',null,null)"),1);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"9");
assertEqual(con.autoCommitOff(),1);
console.log("\n");


// finished suspended session
console.log("FINISHED SUSPENDED SESSION: ");
assertEqual(cur.sendQuery("select * from testtable order by testinteger"),1);
assertEqual(cur.getField(4,0),"5");
assertEqual(cur.getField(5,0),"6");
assertEqual(cur.getField(6,0),"7");
assertEqual(cur.getField(7,0),"8");
id=cur.getResultSetId();
cur.suspendResultSet();
assertEqual(con.suspendSession(),1);
port=con.getConnectionPort();
socket=con.getConnectionSocket();
assertEqual(con.resumeSession(port,socket),1);
assertEqual(cur.resumeResultSet(id),1);
assertEqual(cur.getField(4,0),null);
assertEqual(cur.getField(5,0),null);
assertEqual(cur.getField(6,0),null);
assertEqual(cur.getField(7,0),null);
console.log("\n");
	
// drop existing table
con.commit();
cur.sendQuery("delete from testtable");
con.commit();
console.log("\n");
	
// invalid queries...


// invalid queries
console.log("INVALID QUERIES: ");
assertEqual(cur.sendQuery("select * from testtable1 order by testinteger"),0);
assertEqual(cur.sendQuery("select * from testtable1 order by testinteger"),0);
assertEqual(cur.sendQuery("select * from testtable1 order by testinteger"),0);
assertEqual(cur.sendQuery("select * from testtable1 order by testinteger"),0);
console.log();
assertEqual(cur.sendQuery("insert into testtable1 values (1,2,3,4)"),0);
assertEqual(cur.sendQuery("insert into testtable1 values (1,2,3,4)"),0);
assertEqual(cur.sendQuery("insert into testtable1 values (1,2,3,4)"),0);
assertEqual(cur.sendQuery("insert into testtable1 values (1,2,3,4)"),0);
console.log();
assertEqual(cur.sendQuery("create table testtable"),0);
assertEqual(cur.sendQuery("create table testtable"),0);
assertEqual(cur.sendQuery("create table testtable"),0);
assertEqual(cur.sendQuery("create table testtable"),0);
console.log("\n");

reportTestStatus();

process.exit(getStatus());
