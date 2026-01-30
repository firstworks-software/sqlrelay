// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{assertEqual, getStatus, reportTestStatus}=require("./asserts.js");
	
	
var	dbtype;
var	bindvars=["1","2","3","4","5","6","7","8","9","10"];
var	bindvals=["4","4","4","4.4","4.4","4.4",
		"testchar4","testvarchar4","01/01/2004","04:00:00"];
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
					"db2inst1","testpassword",0,1);
var	cur=new sqlrelay.SQLRCursor(con);
	
// get database type


// identify
console.log("IDENTIFY: ");
assertEqual(con.identify(),"db2");
console.log("\n");


// ping
console.log("PING: ");
assertEqual(con.ping(),1);
console.log("\n");


// isolation levels
console.log("ISOLATION LEVELS: ");
var	isolationlevels=["CS","UR","RS","RR"];
for (var i=0; i<isolationlevels.length; i++) {
	assertEqual(con.setIsolationLevel(isolationlevels[i]),1);
	assertEqual(con.getIsolationLevel(),isolationlevels[i]);
	console.log();
}
// reset to the default isolation level
assertEqual(con.setIsolationLevel(isolationlevels[0]),1);
console.log("\n");

// drop existing table
cur.sendQuery("drop table testtable");


// create temptable
console.log("CREATE TEMPTABLE: ");
assertEqual(cur.sendQuery("create table testtable (testsmallint smallint, testint integer, testbigint bigint, testdecimal decimal(10,2), testreal real, testdouble double, testchar char(40), testvarchar varchar(40), testdate date, testtime time, testtimestamp timestamp)"),1);
console.log("\n");


// insert
console.log("INSERT: ");
assertEqual(cur.sendQuery("insert into testtable values (1,1,1,1.1,1.1,1.1,'testchar1','testvarchar1','01/01/2001','01:00:00',null)"),1);
console.log("\n");


// bind by position
console.log("BIND BY POSITION: ");
cur.prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,null)");
assertEqual(cur.countBindVariables(),10);
cur.inputBind("1",2);
cur.inputBind("2",2);
cur.inputBind("3",2);
cur.inputBind("4",2.2,4,2);
cur.inputBind("5",2.2,4,2);
cur.inputBind("6",2.2,4,2);
cur.inputBind("7","testchar2");
cur.inputBind("8","testvarchar2");
cur.inputBind("9","01/01/2002");
cur.inputBind("10","02:00:00");
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2",3);
cur.inputBind("3",3);
cur.inputBind("4",3.3,4,2);
cur.inputBind("5",3.3,4,2);
cur.inputBind("6",3.3,4,2);
cur.inputBind("7","testchar3");
cur.inputBind("8","testvarchar3");
cur.inputBind("9","01/01/2003");
cur.inputBind("10","03:00:00");
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
assertEqual(cur.sendQuery("insert into testtable values (5,5,5,5.5,5.5,5.5,'testchar5','testvarchar5','01/01/2005','05:00:00',null)"),1);
assertEqual(cur.sendQuery("insert into testtable values (6,6,6,6.6,6.6,6.6,'testchar6','testvarchar6','01/01/2006','06:00:00',null)"),1);
assertEqual(cur.sendQuery("insert into testtable values (7,7,7,7.7,7.7,7.7,'testchar7','testvarchar7','01/01/2007','07:00:00',null)"),1);
assertEqual(cur.sendQuery("insert into testtable values (8,8,8,8.8,8.8,8.8,'testchar8','testvarchar8','01/01/2008','08:00:00',null)"),1);
console.log("\n");


// affected rows
console.log("AFFECTED ROWS: ");
assertEqual(cur.affectedRows(),1);
console.log("\n");


// stored procedure
console.log("STORED PROCEDURE: ");
cur.sendQuery("drop procedure testproc");
assertEqual(cur.sendQuery("create procedure testproc(in invar int, out outvar int) language sql begin set outvar = invar; end"),1);
cur.prepareQuery("call testproc(?,?)");
cur.inputBind("1",5);
cur.defineOutputBindInteger("2");
assertEqual(cur.executeQuery(),1);
assertEqual(cur.getOutputBindInteger("2"),5);
assertEqual(cur.sendQuery("drop procedure testproc"),1);
console.log("\n");


// select
console.log("SELECT: ");
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
console.log("\n");


// column count
console.log("COLUMN COUNT: ");
assertEqual(cur.colCount(),11);
console.log("\n");


// column names
console.log("COLUMN NAMES: ");
assertEqual(cur.getColumnName(0),"TESTSMALLINT");
assertEqual(cur.getColumnName(1),"TESTINT");
assertEqual(cur.getColumnName(2),"TESTBIGINT");
assertEqual(cur.getColumnName(3),"TESTDECIMAL");
assertEqual(cur.getColumnName(4),"TESTREAL");
assertEqual(cur.getColumnName(5),"TESTDOUBLE");
assertEqual(cur.getColumnName(6),"TESTCHAR");
assertEqual(cur.getColumnName(7),"TESTVARCHAR");
assertEqual(cur.getColumnName(8),"TESTDATE");
assertEqual(cur.getColumnName(9),"TESTTIME");
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP");
cols=cur.getColumnNames();
assertEqual(cols[0],"TESTSMALLINT");
assertEqual(cols[1],"TESTINT");
assertEqual(cols[2],"TESTBIGINT");
assertEqual(cols[3],"TESTDECIMAL");
assertEqual(cols[4],"TESTREAL");
assertEqual(cols[5],"TESTDOUBLE");
assertEqual(cols[6],"TESTCHAR");
assertEqual(cols[7],"TESTVARCHAR");
assertEqual(cols[8],"TESTDATE");
assertEqual(cols[9],"TESTTIME");
assertEqual(cols[10],"TESTTIMESTAMP");
console.log("\n");


// column types
console.log("COLUMN TYPES: ");
assertEqual(cur.getColumnType(0),"SMALLINT");
assertEqual(cur.getColumnType("TESTSMALLINT"),"SMALLINT");
assertEqual(cur.getColumnType(1),"INTEGER");
assertEqual(cur.getColumnType("TESTINT"),"INTEGER");
assertEqual(cur.getColumnType(2),"BIGINT");
assertEqual(cur.getColumnType("TESTBIGINT"),"BIGINT");
assertEqual(cur.getColumnType(3),"DECIMAL");
assertEqual(cur.getColumnType("TESTDECIMAL"),"DECIMAL");
assertEqual(cur.getColumnType(4),"REAL");
assertEqual(cur.getColumnType("TESTREAL"),"REAL");
assertEqual(cur.getColumnType(5),"DOUBLE");
assertEqual(cur.getColumnType("TESTDOUBLE"),"DOUBLE");
assertEqual(cur.getColumnType(6),"CHAR");
assertEqual(cur.getColumnType("TESTCHAR"),"CHAR");
assertEqual(cur.getColumnType(7),"VARCHAR");
assertEqual(cur.getColumnType("TESTVARCHAR"),"VARCHAR");
assertEqual(cur.getColumnType(8),"DATE");
assertEqual(cur.getColumnType("TESTDATE"),"DATE");
assertEqual(cur.getColumnType(9),"TIME");
assertEqual(cur.getColumnType("TESTTIME"),"TIME");
assertEqual(cur.getColumnType(10),"TIMESTAMP");
assertEqual(cur.getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
console.log("\n");


// column length
console.log("COLUMN LENGTH: ");
assertEqual(cur.getColumnLength(0),2);
assertEqual(cur.getColumnLength("TESTSMALLINT"),2);
assertEqual(cur.getColumnLength(1),4);
assertEqual(cur.getColumnLength("TESTINT"),4);
assertEqual(cur.getColumnLength(2),8);
assertEqual(cur.getColumnLength("TESTBIGINT"),8);
assertEqual(cur.getColumnLength(3),12);
assertEqual(cur.getColumnLength("TESTDECIMAL"),12);
assertEqual(cur.getColumnLength(4),4);
assertEqual(cur.getColumnLength("TESTREAL"),4);
assertEqual(cur.getColumnLength(5),8);
assertEqual(cur.getColumnLength("TESTDOUBLE"),8);
assertEqual(cur.getColumnLength(6),40);
assertEqual(cur.getColumnLength("TESTCHAR"),40);
assertEqual(cur.getColumnLength(7),40);
assertEqual(cur.getColumnLength("TESTVARCHAR"),40);
assertEqual(cur.getColumnLength(8),6);
assertEqual(cur.getColumnLength("TESTDATE"),6);
assertEqual(cur.getColumnLength(9),6);
assertEqual(cur.getColumnLength("TESTTIME"),6);
assertEqual(cur.getColumnLength(10),16);
assertEqual(cur.getColumnLength("TESTTIMESTAMP"),16);
console.log("\n");


// longest column
console.log("LONGEST COLUMN: ");
assertEqual(cur.getLongest(0),1);
assertEqual(cur.getLongest("TESTSMALLINT"),1);
assertEqual(cur.getLongest(1),1);
assertEqual(cur.getLongest("TESTINT"),1);
assertEqual(cur.getLongest(2),1);
assertEqual(cur.getLongest("TESTBIGINT"),1);
assertEqual(cur.getLongest(3),4);
assertEqual(cur.getLongest("TESTDECIMAL"),4);
//assertEqual(cur.getLongest(4),3);
//assertEqual(cur.getLongest("TESTREAL"),3);
//assertEqual(cur.getLongest(5),3);
//assertEqual(cur.getLongest("TESTDOUBLE"),3);
assertEqual(cur.getLongest(6),40);
assertEqual(cur.getLongest("TESTCHAR"),40);
assertEqual(cur.getLongest(7),12);
assertEqual(cur.getLongest("TESTVARCHAR"),12);
assertEqual(cur.getLongest(8),10);
assertEqual(cur.getLongest("TESTDATE"),10);
assertEqual(cur.getLongest(9),8);
assertEqual(cur.getLongest("TESTTIME"),8);
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
assertEqual(cur.getField(0,2),"1");
assertEqual(cur.getField(0,3),"1.10");
//assertEqual(cur.getField(0,4),"1.1");
//assertEqual(cur.getField(0,5),"1.1");
assertEqual(cur.getField(0,6),"testchar1                               ");
assertEqual(cur.getField(0,7),"testvarchar1");
assertEqual(cur.getField(0,8),"2001-01-01");
assertEqual(cur.getField(0,9),"01:00:00");
console.log();
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(7,1),"8");
assertEqual(cur.getField(7,2),"8");
assertEqual(cur.getField(7,3),"8.80");
//assertEqual(cur.getField(7,4),"8.8");
//assertEqual(cur.getField(7,5),"8.8");
assertEqual(cur.getField(7,6),"testchar8                               ");
assertEqual(cur.getField(7,7),"testvarchar8");
assertEqual(cur.getField(7,8),"2008-01-01");
assertEqual(cur.getField(7,9),"08:00:00");
console.log("\n");


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqual(cur.getFieldLength(0,0),1);
assertEqual(cur.getFieldLength(0,1),1);
assertEqual(cur.getFieldLength(0,2),1);
assertEqual(cur.getFieldLength(0,3),4);
//assertEqual(cur.getFieldLength(0,4),3);
//assertEqual(cur.getFieldLength(0,5),3);
assertEqual(cur.getFieldLength(0,6),40);
assertEqual(cur.getFieldLength(0,7),12);
assertEqual(cur.getFieldLength(0,8),10);
assertEqual(cur.getFieldLength(0,9),8);
console.log();
assertEqual(cur.getFieldLength(7,0),1);
assertEqual(cur.getFieldLength(7,1),1);
assertEqual(cur.getFieldLength(7,2),1);
assertEqual(cur.getFieldLength(7,3),4);
//assertEqual(cur.getFieldLength(7,4),3);
//assertEqual(cur.getFieldLength(7,5),3);
assertEqual(cur.getFieldLength(7,6),40);
assertEqual(cur.getFieldLength(7,7),12);
assertEqual(cur.getFieldLength(7,8),10);
assertEqual(cur.getFieldLength(7,9),8);
console.log("\n");


// fields by name
console.log("FIELDS BY NAME: ");
assertEqual(cur.getField(0,"TESTSMALLINT"),"1");
assertEqual(cur.getField(0,"TESTINT"),"1");
assertEqual(cur.getField(0,"TESTBIGINT"),"1");
assertEqual(cur.getField(0,"TESTDECIMAL"),"1.10");
//assertEqual(cur.getField(0,"TESTREAL"),"1.1");
//assertEqual(cur.getField(0,"TESTDOUBLE"),"1.1");
assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                               ");
assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
assertEqual(cur.getField(0,"TESTDATE"),"2001-01-01");
assertEqual(cur.getField(0,"TESTTIME"),"01:00:00");
console.log();
assertEqual(cur.getField(7,"TESTSMALLINT"),"8");
assertEqual(cur.getField(7,"TESTINT"),"8");
assertEqual(cur.getField(7,"TESTBIGINT"),"8");
assertEqual(cur.getField(7,"TESTDECIMAL"),"8.80");
//assertEqual(cur.getField(7,"TESTREAL"),"8.8");
//assertEqual(cur.getField(7,"TESTDOUBLE"),"8.8");
assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                               ");
assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
assertEqual(cur.getField(7,"TESTDATE"),"2008-01-01");
assertEqual(cur.getField(7,"TESTTIME"),"08:00:00");
console.log("\n");


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqual(cur.getFieldLength(0,"TESTSMALLINT"),1);
assertEqual(cur.getFieldLength(0,"TESTINT"),1);
assertEqual(cur.getFieldLength(0,"TESTBIGINT"),1);
assertEqual(cur.getFieldLength(0,"TESTDECIMAL"),4);
//assertEqual(cur.getFieldLength(0,"TESTREAL"),3);
//assertEqual(cur.getFieldLength(0,"TESTDOUBLE"),3);
assertEqual(cur.getFieldLength(0,"TESTCHAR"),40);
assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12);
assertEqual(cur.getFieldLength(0,"TESTDATE"),10);
assertEqual(cur.getFieldLength(0,"TESTTIME"),8);
console.log();
assertEqual(cur.getFieldLength(7,"TESTSMALLINT"),1);
assertEqual(cur.getFieldLength(7,"TESTINT"),1);
assertEqual(cur.getFieldLength(7,"TESTBIGINT"),1);
assertEqual(cur.getFieldLength(7,"TESTDECIMAL"),4);
//assertEqual(cur.getFieldLength(7,"TESTREAL"),3);
//assertEqual(cur.getFieldLength(7,"TESTDOUBLE"),3);
assertEqual(cur.getFieldLength(7,"TESTCHAR"),40);
assertEqual(cur.getFieldLength(7,"TESTVARCHAR"),12);
assertEqual(cur.getFieldLength(7,"TESTDATE"),10);
assertEqual(cur.getFieldLength(7,"TESTTIME"),8);
console.log("\n");


// fields by array
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqual(fields[0],"1");
assertEqual(fields[1],"1");
assertEqual(fields[2],"1");
assertEqual(fields[3],"1.10");
//assertEqual(fields[4],"1.1");
//assertEqual(fields[5],"1.1");
assertEqual(fields[6],"testchar1                               ");
assertEqual(fields[7],"testvarchar1");
assertEqual(fields[8],"2001-01-01");
assertEqual(fields[9],"01:00:00");
console.log("\n");


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqual(fieldlens[0],1);
assertEqual(fieldlens[1],1);
assertEqual(fieldlens[2],1);
assertEqual(fieldlens[3],4);
//assertEqual(fieldlens[4],3);
//assertEqual(fieldlens[5],3);
assertEqual(fieldlens[6],40);
assertEqual(fieldlens[7],12);
assertEqual(fieldlens[8],10);
assertEqual(fieldlens[9],8);
console.log("\n");


// individual substitutions
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')");
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
cur.prepareQuery("values ('$(var1)','$(var2)','$(var3)')");
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
cur.prepareQuery("values ($(var1),$(var2),$(var3))");
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
cur.prepareQuery("values ($(var1),$(var2),$(var3))");
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
cur.sendQuery("drop table testtable1");
cur.sendQuery("create table testtable1 (col1 char(1), col2 char(1), col3 char(1))");
cur.getNullsAsNulls();
assertEqual(cur.sendQuery("insert into testtable1 values ('1',null,null)"),1);
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),null);
assertEqual(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"");
assertEqual(cur.getField(0,2),"");
cur.sendQuery("drop table testtable1");
cur.getNullsAsNulls();
console.log("\n");


// result set buffer size
console.log("RESULT SET BUFFER SIZE: ");
assertEqual(cur.getResultSetBufferSize(),0);
cur.setResultSetBufferSize(2);
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
assertEqual(cur.getColumnName(0),null);
assertEqual(cur.getColumnLength(0),0);
assertEqual(cur.getColumnType(0),null);
cur.getColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
assertEqual(cur.getColumnName(0),"TESTSMALLINT");
assertEqual(cur.getColumnLength(0),2);
assertEqual(cur.getColumnType(0),"SMALLINT");
console.log("\n");


// suspended session
console.log("SUSPENDED SESSION: ");
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
cur.cacheOff();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
console.log("\n");


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqual(cur.colCount(),11);
console.log("\n");


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqual(cur.getColumnName(0),"TESTSMALLINT");
assertEqual(cur.getColumnName(1),"TESTINT");
assertEqual(cur.getColumnName(2),"TESTBIGINT");
assertEqual(cur.getColumnName(3),"TESTDECIMAL");
assertEqual(cur.getColumnName(4),"TESTREAL");
assertEqual(cur.getColumnName(5),"TESTDOUBLE");
assertEqual(cur.getColumnName(6),"TESTCHAR");
assertEqual(cur.getColumnName(7),"TESTVARCHAR");
assertEqual(cur.getColumnName(8),"TESTDATE");
assertEqual(cur.getColumnName(9),"TESTTIME");
assertEqual(cur.getColumnName(10),"TESTTIMESTAMP");
cols=cur.getColumnNames();
assertEqual(cols[0],"TESTSMALLINT");
assertEqual(cols[1],"TESTINT");
assertEqual(cols[2],"TESTBIGINT");
assertEqual(cols[3],"TESTDECIMAL");
assertEqual(cols[4],"TESTREAL");
assertEqual(cols[5],"TESTDOUBLE");
assertEqual(cols[6],"TESTCHAR");
assertEqual(cols[7],"TESTVARCHAR");
assertEqual(cols[8],"TESTDATE");
assertEqual(cols[9],"TESTTIME");
assertEqual(cols[10],"TESTTIMESTAMP");
console.log("\n");


// cached result set with result set buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),1);
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


// finished suspended session
console.log("FINISHED SUSPENDED SESSION: ");
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
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
cur.sendQuery("drop table testtable");
console.log("\n");
	
// invalid queries...


// invalid queries
console.log("INVALID QUERIES: ");
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),0);
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),0);
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),0);
assertEqual(cur.sendQuery("select * from testtable order by testsmallint"),0);
console.log();
assertEqual(cur.sendQuery("insert into testtable values (1,2,3,4)"),0);
assertEqual(cur.sendQuery("insert into testtable values (1,2,3,4)"),0);
assertEqual(cur.sendQuery("insert into testtable values (1,2,3,4)"),0);
assertEqual(cur.sendQuery("insert into testtable values (1,2,3,4)"),0);
console.log();
assertEqual(cur.sendQuery("create table testtable"),0);
assertEqual(cur.sendQuery("create table testtable"),0);
assertEqual(cur.sendQuery("create table testtable"),0);
assertEqual(cur.sendQuery("create table testtable"),0);
console.log("\n");

reportTestStatus();

process.exit(getStatus());
