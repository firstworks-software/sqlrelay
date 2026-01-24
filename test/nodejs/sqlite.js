// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{assertEqual}=require("./assert.js");

	
var	dbtype;
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
console.log("IDENTIFY: ");
assertEqual(con.identify(),"sqlite");
console.log("\n");
	
// ping
console.log("PING: ");
assertEqual(con.ping(),1);
console.log("\n");
	
// drop existing table
cur.sendQuery("begin transaction");
cur.sendQuery("drop table testtable");
con.commit();
	
// create a new table
console.log("CREATE TEMPTABLE: ");
cur.sendQuery("begin transaction");
assertEqual(cur.sendQuery("create table testtable (testint int, testfloat float, testchar char(40), testvarchar varchar(40))"),1);
con.commit();
console.log("\n");
	
console.log("INSERT: ");
cur.sendQuery("begin transaction");
assertEqual(cur.sendQuery("insert into testtable values (1,1.1,'testchar1','testvarchar1')"),1);
assertEqual(cur.sendQuery("insert into testtable values (2,2.2,'testchar2','testvarchar2')"),1);
assertEqual(cur.sendQuery("insert into testtable values (3,3.3,'testchar3','testvarchar3')"),1);
assertEqual(cur.sendQuery("insert into testtable values (4,4.4,'testchar4','testvarchar4')"),1);
console.log("\n");
	
console.log("AFFECTED ROWS: ");
assertEqual(cur.affectedRows(),0);
console.log("\n");
	
console.log("BIND BY NAME: ");
cur.prepareQuery("insert into testtable values (:var1,:var2,:var3,:var4)");
assertEqual(cur.countBindVariables(),4);
cur.inputBind("var1",5);
cur.inputBind("var2",5.5,4,1);
cur.inputBind("var3","testchar5");
cur.inputBind("var4","testvarchar5");
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("var1",6);
cur.inputBind("var2",6.6,4,1);
cur.inputBind("var3","testchar6");
cur.inputBind("var4","testvarchar6");
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("var1",7);
cur.inputBind("var2",7.7,4,1);
cur.inputBind("var3","testchar7");
cur.inputBind("var4","testvarchar7");
assertEqual(cur.executeQuery(),1);
console.log("\n");
	
console.log("BIND BY NAME WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("var1",8);
cur.inputBind("var2",8.8,4,1);
cur.inputBind("var3","testchar8");
cur.inputBind("var4","testvarchar8");
cur.validateBinds();
assertEqual(cur.executeQuery(),1);
console.log("\n");
	
console.log("SELECT: ");
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
console.log("\n");
	
console.log("COLUMN COUNT: ");
assertEqual(cur.colCount(),4);
console.log("\n");
	
console.log("COLUMN NAMES: ");
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnName(1),"testfloat");
assertEqual(cur.getColumnName(2),"testchar");
assertEqual(cur.getColumnName(3),"testvarchar");
cols=cur.getColumnNames();
assertEqual(cols[0],"testint");
assertEqual(cols[1],"testfloat");
assertEqual(cols[2],"testchar");
assertEqual(cols[3],"testvarchar");
console.log("\n");
	
console.log("COLUMN TYPES: ");
assertEqual(cur.getColumnType(0),"INTEGER");
assertEqual(cur.getColumnType("testint"),"INTEGER");
assertEqual(cur.getColumnType(1),"FLOAT");
assertEqual(cur.getColumnType("testfloat"),"FLOAT");
assertEqual(cur.getColumnType(2),"STRING");
assertEqual(cur.getColumnType("testchar"),"STRING");
assertEqual(cur.getColumnType(3),"STRING");
assertEqual(cur.getColumnType("testvarchar"),"STRING");
console.log("\n");
	
console.log("COLUMN LENGTH: ");
assertEqual(cur.getColumnLength(0),0);
assertEqual(cur.getColumnLength("testint"),0);
assertEqual(cur.getColumnLength(1),0);
assertEqual(cur.getColumnLength("testfloat"),0);
assertEqual(cur.getColumnLength(2),0);
assertEqual(cur.getColumnLength("testchar"),0);
assertEqual(cur.getColumnLength(3),0);
assertEqual(cur.getColumnLength("testvarchar"),0);
console.log("\n");
	
console.log("LONGEST COLUMN: ");
assertEqual(cur.getLongest(0),1);
assertEqual(cur.getLongest("testint"),1);
assertEqual(cur.getLongest(1),3);
assertEqual(cur.getLongest("testfloat"),3);
assertEqual(cur.getLongest(2),9);
assertEqual(cur.getLongest("testchar"),9);
assertEqual(cur.getLongest(3),12);
assertEqual(cur.getLongest("testvarchar"),12);
console.log("\n");
	
console.log("ROW COUNT: ");
assertEqual(cur.rowCount(),8);
console.log("\n");
	
console.log("TOTAL ROWS: ");
assertEqual(cur.totalRows(),0);
console.log("\n");
	
console.log("FIRST ROW INDEX: ");
assertEqual(cur.firstRowIndex(),0);
console.log("\n");
	
console.log("END OF RESULT SET: ");
assertEqual(cur.endOfResultSet(),1);
console.log("\n");
	
console.log("FIELDS BY INDEX: ");
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"1.1");
assertEqual(cur.getField(0,2),"testchar1");
assertEqual(cur.getField(0,3),"testvarchar1");
console.log();
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(7,1),"8.8");
assertEqual(cur.getField(7,2),"testchar8");
assertEqual(cur.getField(7,3),"testvarchar8");
console.log("\n");
	
console.log("FIELD LENGTHS BY INDEX: ");
assertEqual(cur.getFieldLength(0,0),1);
assertEqual(cur.getFieldLength(0,1),3);
assertEqual(cur.getFieldLength(0,2),9);
assertEqual(cur.getFieldLength(0,3),12);
console.log();
assertEqual(cur.getFieldLength(7,0),1);
assertEqual(cur.getFieldLength(7,1),3);
assertEqual(cur.getFieldLength(7,2),9);
assertEqual(cur.getFieldLength(7,3),12);
console.log("\n");
	
console.log("FIELDS BY NAME: ");
assertEqual(cur.getField(0,"testint"),"1");
assertEqual(cur.getField(0,"testfloat"),"1.1");
assertEqual(cur.getField(0,"testchar"),"testchar1");
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1");
console.log();
assertEqual(cur.getField(7,"testint"),"8");
assertEqual(cur.getField(7,"testfloat"),"8.8");
assertEqual(cur.getField(7,"testchar"),"testchar8");
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8");
console.log("\n");
	
console.log("FIELD LENGTHS BY NAME: ");
assertEqual(cur.getFieldLength(0,"testint"),1);
assertEqual(cur.getFieldLength(0,"testfloat"),3);
assertEqual(cur.getFieldLength(0,"testchar"),9);
assertEqual(cur.getFieldLength(0,"testvarchar"),12);
console.log();
assertEqual(cur.getFieldLength(7,"testint"),1);
assertEqual(cur.getFieldLength(7,"testfloat"),3);
assertEqual(cur.getFieldLength(7,"testchar"),9);
assertEqual(cur.getFieldLength(7,"testvarchar"),12);
console.log("\n");
	
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqual(fields[0],"1");
assertEqual(fields[1],"1.1");
assertEqual(fields[2],"testchar1");
assertEqual(fields[3],"testvarchar1");
console.log("\n");
	
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqual(fieldlens[0],1);
assertEqual(fieldlens[1],3);
assertEqual(fieldlens[2],9);
assertEqual(fieldlens[3],12);
console.log("\n");
	
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.sendQuery("drop table testtable1");
assertEqual(cur.sendQuery("create table testtable1 (col1 int, col2 char, col3 float)"),1);
cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
cur.substitution("var1",1);
cur.substitution("var2","hello");
cur.substitution("var3",10.5556,6,4);
assertEqual(cur.executeQuery(),1);
console.log("\n");
	
console.log("FIELDS: ");
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"hello");
assertEqual(cur.getField(0,2),"10.5556");
assertEqual(cur.sendQuery("delete from testtable1"),1);
console.log("\n");
	
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("insert into testtable1 values ('$(var1)','$(var2)','$(var3)')");
cur.substitutions(subvars,subvalstrings);
assertEqual(cur.executeQuery(),1);
console.log("\n");
	
console.log("FIELDS: ");
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"hi");
assertEqual(cur.getField(0,1),"hello");
assertEqual(cur.getField(0,2),"bye");
assertEqual(cur.sendQuery("delete from testtable1"),1);
console.log("\n");
	
	
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
cur.substitutions(subvars,subvallongs);
assertEqual(cur.executeQuery(),1);
console.log("\n");
	
console.log("FIELDS: ");
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"2");
assertEqual(cur.getField(0,2),"3.0");
assertEqual(cur.sendQuery("delete from testtable1"),1);
console.log("\n");
	
	
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertEqual(cur.executeQuery(),1);
console.log("\n");
	
console.log("FIELDS: ");
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"10.55");
assertEqual(cur.getField(0,1),"10.556");
assertEqual(cur.getField(0,2),"10.5556");
assertEqual(cur.sendQuery("delete from testtable1"),1);
console.log("\n");
	
	
console.log("nullS as Nulls: ");
cur.getNullsAsNulls();
assertEqual(cur.sendQuery("insert into testtable1 values (1,null,null)"),1);
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),null);
assertEqual(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertEqual(cur.sendQuery("select * from testtable1"),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"");
assertEqual(cur.getField(0,2),"");
cur.getNullsAsNulls();
console.log("\n");
	
console.log("RESULT SET BUFFER SIZE: ");
assertEqual(cur.getResultSetBufferSize(),0);
cur.setResultSetBufferSize(2);
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
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
	
console.log("DONT GET COLUMN INFO: ");
cur.dontGetColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
assertEqual(cur.getColumnName(0),null);
assertEqual(cur.getColumnLength(0),0);
assertEqual(cur.getColumnType(0),null);
cur.getColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnLength(0),0);
assertEqual(cur.getColumnType(0),"INTEGER");
console.log("\n");
	
console.log("SUSPENDED SESSION: ");
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
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
	
console.log("SUSPENDED RESULT SET: ");
cur.setResultSetBufferSize(2);
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
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
	
console.log("CACHED RESULT SET: ");
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
cur.cacheOff();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
console.log("\n");
	
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqual(cur.colCount(),4);
console.log("\n");
	
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnName(1),"testfloat");
assertEqual(cur.getColumnName(2),"testchar");
assertEqual(cur.getColumnName(3),"testvarchar");
cols=cur.getColumnNames();
assertEqual(cols[0],"testint");
assertEqual(cols[1],"testfloat");
assertEqual(cols[2],"testchar");
assertEqual(cols[3],"testvarchar");
console.log("\n");
	
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
cur.cacheOff();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log("\n");
	
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2");
assertEqual(cur.openCachedResultSet("cachefile1"),1);
cur.cacheOff();
assertEqual(cur.openCachedResultSet("cachefile2"),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
console.log("\n");
	
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
	
console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
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

console.log("COMMIT AND ROLLBACK: \n");
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
assertEqual(cur.sendQuery("insert into testtable values (10,10.1,'testchar10','testvarchar10')"),1);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"9");
console.log("\n");


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
cur.sendQuery("drop table testtable");
	
// invalid queries...
console.log("INVALID QUERIES: ");
assertEqual(cur.sendQuery("select * from testtable"),0);
assertEqual(cur.sendQuery("select * from testtable"),0);
assertEqual(cur.sendQuery("select * from testtable"),0);
assertEqual(cur.sendQuery("select * from testtable"),0);
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

process.exit(0);
