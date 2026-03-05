// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{assertEqual, getStatus, reportTestStatus}=require("./asserts.js");


var	cols;
var	fields;
var	fieldlens;
var	subvars=["var1","var2","var3"];
var	subvallongs=[1,2,3];
var	subvalstrings=["hi","hello","bye"];
var	subvaldoubles=[10.55,10.556,10.5556];
var	precs=[4,5,6];
var	scales=[2,3,4];
var	port;
var	socket;
var	id;
var	filename;
var	numvar;
var	stringvar;
var	floatvar;
var	dbtype;

// instantiation
var	con=new sqlrelay.SQLRConnection("sqlrelay",
				9000,
				"/tmp/test.socket",
				"testuser","testpassword",0,1);
var	cur=new sqlrelay.SQLRCursor(con);


// identify
console.log("IDENTIFY: ");
assertEqual(con.identify(),"postgresql");
console.log();


// ping
console.log("PING: ");
assertEqual(con.ping(),1);
console.log();

// isolation levels
/*console.log("ISOLATION LEVELS: ");
var	isolationlevels=["read committed",
			"read uncommitted","repeatable read",
			"serializable"];
for (var i=0; i<isolationlevels.length; i++) {
	// postgresql requires the isolation level to
	// be the first query of the transaction
	con.begin();
	assertEqual(con.setIsolationLevel(isolationlevels[i]),1);
	assertEqual(con.getIsolationLevel(),isolationlevels[i]);
	con.commit();
	console.log();
}
// reset to the default isolation level
con.begin();
assertEqual(con.setIsolationLevel(isolationlevels[0]),1);
con.commit();
console.log("\n");*/

// drop existing table
cur.sendQuery("drop table testtable");


// create temptable
console.log("CREATE TEMPTABLE: ");
assertEqual(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testfloat float, "+
	"	testreal real, "+
	"	testsmallint smallint, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testdate date, "+
	"	testtime time, "+
	"	testtimestamp timestamp)"),1);
console.log();


// begin transction
console.log("BEGIN TRANSCTION: ");
assertEqual(cur.sendQuery("begin"),1);
console.log();


// insert
console.log("INSERT: ");
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1.1, "+
	"	1.1, "+
	"	1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01/01/2001', "+
	"	'01:00:00', "+
	"	null)"),1);
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	2, "+
	"	2.2, "+
	"	2.2, "+
	"	2, "+
	"	'testchar2', "+
	"	'testvarchar2', "+
	"	'01/01/2002', "+
	"	'02:00:00', "+
	"	null)"),1);
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	3, "+
	"	3.3, "+
	"	3.3, "+
	"	3, "+
	"	'testchar3', "+
	"	'testvarchar3', "+
	"	'01/01/2003', "+
	"	'03:00:00', "+
	"	null)"),1);
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	4, "+
	"	4.4, "+
	"	4.4, "+
	"	4, "+
	"	'testchar4', "+
	"	'testvarchar4', "+
	"	'01/01/2004', "+
	"	'04:00:00', "+
	"	null)"),1);
console.log();


// affected rows
console.log("AFFECTED ROWS: ");
assertEqual(cur.affectedRows(),1);
console.log();


// bind by position
console.log("BIND BY POSITION: ");
cur.prepareQuery("insert into testtable values ($1,$2,$3,$4,$5,$6,$7,$8)");
assertEqual(cur.countBindVariables(),8);
cur.inputBind("1",5);
cur.inputBind("2",5.5,4,2);
cur.inputBind("3",5.5,4,2);
cur.inputBind("4",5);
cur.inputBind("5","testchar5");
cur.inputBind("6","testvarchar5");
cur.inputBind("7","01/01/2005");
cur.inputBind("8","05:00:00");
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("1",6);
cur.inputBind("2",6.6,4,2);
cur.inputBind("3",6.6,4,2);
cur.inputBind("4",6);
cur.inputBind("5","testchar6");
cur.inputBind("6","testvarchar6");
cur.inputBind("7","01/01/2006");
cur.inputBind("8","06:00:00");
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("1",7);
cur.inputBind("2",7.7,4,2);
cur.inputBind("3",7.7,4,2);
cur.inputBind("4",7);
cur.inputBind("5","testchar7");
cur.inputBind("6","testvarchar7");
cur.inputBind("7","01/01/2007");
cur.inputBind("8","07:00:00");
assertEqual(cur.executeQuery(),1);
console.log();


// bind by position with validation
console.log("BIND BY POSITION WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("1",8);
cur.inputBind("2",8.8,4,2);
cur.inputBind("3",8.8,4,2);
cur.inputBind("4",8);
cur.inputBind("5","testchar8");
cur.inputBind("6","testvarchar8");
cur.inputBind("7","01/01/2008");
cur.inputBind("8","08:00:00");
cur.validateBinds();
assertEqual(cur.executeQuery(),1);
console.log();


// select
console.log("SELECT: ");
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
console.log();


// column count
console.log("COLUMN COUNT: ");
assertEqual(cur.colCount(),9);
console.log();


// column names
console.log("COLUMN NAMES: ");
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnName(1),"testfloat");
assertEqual(cur.getColumnName(2),"testreal");
assertEqual(cur.getColumnName(3),"testsmallint");
assertEqual(cur.getColumnName(4),"testchar");
assertEqual(cur.getColumnName(5),"testvarchar");
assertEqual(cur.getColumnName(6),"testdate");
assertEqual(cur.getColumnName(7),"testtime");
assertEqual(cur.getColumnName(8),"testtimestamp");
cols=cur.getColumnNames();
assertEqual(cols[0],"testint");
assertEqual(cols[1],"testfloat");
assertEqual(cols[2],"testreal");
assertEqual(cols[3],"testsmallint");
assertEqual(cols[4],"testchar");
assertEqual(cols[5],"testvarchar");
assertEqual(cols[6],"testdate");
assertEqual(cols[7],"testtime");
assertEqual(cols[8],"testtimestamp");
console.log();


// column types
console.log("COLUMN TYPES: ");
assertEqual(cur.getColumnType(0),"int4");
assertEqual(cur.getColumnType("testint"),"int4");
assertEqual(cur.getColumnType(1),"float8");
assertEqual(cur.getColumnType("testfloat"),"float8");
assertEqual(cur.getColumnType(2),"float4");
assertEqual(cur.getColumnType("testreal"),"float4");
assertEqual(cur.getColumnType(3),"int2");
assertEqual(cur.getColumnType("testsmallint"),"int2");
assertEqual(cur.getColumnType(4),"bpchar");
assertEqual(cur.getColumnType("testchar"),"bpchar");
assertEqual(cur.getColumnType(5),"varchar");
assertEqual(cur.getColumnType("testvarchar"),"varchar");
assertEqual(cur.getColumnType(6),"date");
assertEqual(cur.getColumnType("testdate"),"date");
assertEqual(cur.getColumnType(7),"time");
assertEqual(cur.getColumnType("testtime"),"time");
assertEqual(cur.getColumnType(8),"timestamp");
assertEqual(cur.getColumnType("testtimestamp"),"timestamp");
console.log();


// column length
console.log("COLUMN LENGTH: ");
assertEqual(cur.getColumnLength(0),4);
assertEqual(cur.getColumnLength("testint"),4);
assertEqual(cur.getColumnLength(1),8);
assertEqual(cur.getColumnLength("testfloat"),8);
assertEqual(cur.getColumnLength(2),4);
assertEqual(cur.getColumnLength("testreal"),4);
assertEqual(cur.getColumnLength(3),2);
assertEqual(cur.getColumnLength("testsmallint"),2);
assertEqual(cur.getColumnLength(4),44);
assertEqual(cur.getColumnLength("testchar"),44);
assertEqual(cur.getColumnLength(5),44);
assertEqual(cur.getColumnLength("testvarchar"),44);
assertEqual(cur.getColumnLength(6),4);
assertEqual(cur.getColumnLength("testdate"),4);
assertEqual(cur.getColumnLength(7),8);
assertEqual(cur.getColumnLength("testtime"),8);
assertEqual(cur.getColumnLength(8),8);
assertEqual(cur.getColumnLength("testtimestamp"),8);
console.log();


// longest column
console.log("LONGEST COLUMN: ");
assertEqual(cur.getLongest(0),1);
assertEqual(cur.getLongest("testint"),1);
assertEqual(cur.getLongest(1),3);
assertEqual(cur.getLongest("testfloat"),3);
assertEqual(cur.getLongest(2),3);
assertEqual(cur.getLongest("testreal"),3);
assertEqual(cur.getLongest(3),1);
assertEqual(cur.getLongest("testsmallint"),1);
assertEqual(cur.getLongest(4),40);
assertEqual(cur.getLongest("testchar"),40);
assertEqual(cur.getLongest(5),12);
assertEqual(cur.getLongest("testvarchar"),12);
assertEqual(cur.getLongest(6),10);
assertEqual(cur.getLongest("testdate"),10);
assertEqual(cur.getLongest(7),8);
assertEqual(cur.getLongest("testtime"),8);
console.log();


// row count
console.log("ROW COUNT: ");
assertEqual(cur.rowCount(),8);
console.log();

/*console.log("TOTAL ROWS: ");
assertEqual(cur.totalRows(),8);
console.log("\n");*/


// first row index
console.log("FIRST ROW INDEX: ");
assertEqual(cur.firstRowIndex(),0);
console.log();


// end of result set
console.log("END OF RESULT SET: ");
assertEqual(cur.endOfResultSet(),1);
console.log();


// fields by index
console.log("FIELDS BY INDEX: ");
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"1.1");
assertEqual(cur.getField(0,2),"1.1");
assertEqual(cur.getField(0,3),"1");
assertEqual(cur.getField(0,4),"testchar1                               ");
assertEqual(cur.getField(0,5),"testvarchar1");
assertEqual(cur.getField(0,6),"2001-01-01");
assertEqual(cur.getField(0,7),"01:00:00");
console.log();
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(7,1),"8.8");
assertEqual(cur.getField(7,2),"8.8");
assertEqual(cur.getField(7,3),"8");
assertEqual(cur.getField(7,4),"testchar8                               ");
assertEqual(cur.getField(7,5),"testvarchar8");
assertEqual(cur.getField(7,6),"2008-01-01");
assertEqual(cur.getField(7,7),"08:00:00");
console.log();


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqual(cur.getFieldLength(0,0),1);
assertEqual(cur.getFieldLength(0,1),3);
assertEqual(cur.getFieldLength(0,2),3);
assertEqual(cur.getFieldLength(0,3),1);
assertEqual(cur.getFieldLength(0,4),40);
assertEqual(cur.getFieldLength(0,5),12);
assertEqual(cur.getFieldLength(0,6),10);
assertEqual(cur.getFieldLength(0,7),8);
console.log();
assertEqual(cur.getFieldLength(7,0),1);
assertEqual(cur.getFieldLength(7,1),3);
assertEqual(cur.getFieldLength(7,2),3);
assertEqual(cur.getFieldLength(7,3),1);
assertEqual(cur.getFieldLength(7,4),40);
assertEqual(cur.getFieldLength(7,5),12);
assertEqual(cur.getFieldLength(7,6),10);
assertEqual(cur.getFieldLength(7,7),8);
console.log();


// fields by name
console.log("FIELDS BY NAME: ");
assertEqual(cur.getField(0,"testint"),"1");
assertEqual(cur.getField(0,"testfloat"),"1.1");
assertEqual(cur.getField(0,"testreal"),"1.1");
assertEqual(cur.getField(0,"testsmallint"),"1");
assertEqual(cur.getField(0,"testchar"),"testchar1                               ");
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1");
assertEqual(cur.getField(0,"testdate"),"2001-01-01");
assertEqual(cur.getField(0,"testtime"),"01:00:00");
console.log();
assertEqual(cur.getField(7,"testint"),"8");
assertEqual(cur.getField(7,"testfloat"),"8.8");
assertEqual(cur.getField(7,"testreal"),"8.8");
assertEqual(cur.getField(7,"testsmallint"),"8");
assertEqual(cur.getField(7,"testchar"),"testchar8                               ");
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8");
assertEqual(cur.getField(7,"testdate"),"2008-01-01");
assertEqual(cur.getField(7,"testtime"),"08:00:00");
console.log();


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqual(cur.getFieldLength(0,"testint"),1);
assertEqual(cur.getFieldLength(0,"testfloat"),3);
assertEqual(cur.getFieldLength(0,"testreal"),3);
assertEqual(cur.getFieldLength(0,"testsmallint"),1);
assertEqual(cur.getFieldLength(0,"testchar"),40);
assertEqual(cur.getFieldLength(0,"testvarchar"),12);
assertEqual(cur.getFieldLength(0,"testdate"),10);
assertEqual(cur.getFieldLength(0,"testtime"),8);
console.log();
assertEqual(cur.getFieldLength(7,"testint"),1);
assertEqual(cur.getFieldLength(7,"testfloat"),3);
assertEqual(cur.getFieldLength(7,"testreal"),3);
assertEqual(cur.getFieldLength(7,"testsmallint"),1);
assertEqual(cur.getFieldLength(7,"testchar"),40);
assertEqual(cur.getFieldLength(7,"testvarchar"),12);
assertEqual(cur.getFieldLength(7,"testdate"),10);
assertEqual(cur.getFieldLength(7,"testtime"),8);
console.log();


// fields by array
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqual(fields[0],"1");
assertEqual(fields[1],"1.1");
assertEqual(fields[2],"1.1");
assertEqual(fields[3],"1");
assertEqual(fields[4],"testchar1                               ");
assertEqual(fields[5],"testvarchar1");
assertEqual(fields[6],"2001-01-01");
assertEqual(fields[7],"01:00:00");
console.log();


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqual(fieldlens[0],1);
assertEqual(fieldlens[1],3);
assertEqual(fieldlens[2],3);
assertEqual(fieldlens[3],1);
assertEqual(fieldlens[4],40);
assertEqual(fieldlens[5],12);
assertEqual(fieldlens[6],10);
assertEqual(fieldlens[7],8);
console.log();


// individual substitutions
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),'$(var2)',$(var3)");
cur.substitution("var1",1);
cur.substitution("var2","hello");
cur.substitution("var3",10.5556,6,4);
assertEqual(cur.executeQuery(),1);
console.log();


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"hello");
assertEqual(cur.getField(0,2),"10.5556");
console.log();


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),$(var2),$(var3)");
cur.substitutions(subvars,subvallongs);
assertEqual(cur.executeQuery(),1);
console.log();


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"2");
assertEqual(cur.getField(0,2),"3");
console.log();


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
cur.substitutions(subvars,subvalstrings);
assertEqual(cur.executeQuery(),1);
console.log();


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"hi");
assertEqual(cur.getField(0,1),"hello");
assertEqual(cur.getField(0,2),"bye");
console.log();


// array substitutions
console.log("ARRAY SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),$(var2),$(var3)");
cur.substitutions(subvars,subvaldoubles,precs,scales);
assertEqual(cur.executeQuery(),1);
console.log();


// fields
console.log("FIELDS: ");
assertEqual(cur.getField(0,0),"10.55");
assertEqual(cur.getField(0,1),"10.556");
assertEqual(cur.getField(0,2),"10.5556");
console.log();

console.log("nullS as Nulls: ");
cur.getNullsAsNulls();
assertEqual(cur.sendQuery("select null,1,null"),1);
assertEqual(cur.getField(0,0),null);
assertEqual(cur.getField(0,1),"1");
assertEqual(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertEqual(cur.sendQuery("select null,1,null"),1);
assertEqual(cur.getField(0,0),"");
assertEqual(cur.getField(0,1),"1");
assertEqual(cur.getField(0,2),"");
cur.getNullsAsNulls();
console.log();


// result set buffer size
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
cur.setResultSetBufferSize(0);
console.log();


// dont get column info
console.log("DONT GET COLUMN INFO: ");
cur.dontGetColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
assertEqual(cur.getColumnName(0),null);
assertEqual(cur.getColumnLength(0),0);
assertEqual(cur.getColumnType(0),null);
cur.getColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnLength(0),4);
assertEqual(cur.getColumnType(0),"int4");
console.log();


// suspended session
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
console.log();


// suspended result set
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
console.log();


// cached result set
console.log("CACHED RESULT SET: ");
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
cur.cacheOff();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
console.log();


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqual(cur.colCount(),9);
console.log();


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnName(1),"testfloat");
assertEqual(cur.getColumnName(2),"testreal");
assertEqual(cur.getColumnName(3),"testsmallint");
assertEqual(cur.getColumnName(4),"testchar");
assertEqual(cur.getColumnName(5),"testvarchar");
assertEqual(cur.getColumnName(6),"testdate");
assertEqual(cur.getColumnName(7),"testtime");
assertEqual(cur.getColumnName(8),"testtimestamp");
cols=cur.getColumnNames();
assertEqual(cols[0],"testint");
assertEqual(cols[1],"testfloat");
assertEqual(cols[2],"testreal");
assertEqual(cols[3],"testsmallint");
assertEqual(cols[4],"testchar");
assertEqual(cols[5],"testvarchar");
assertEqual(cols[6],"testdate");
assertEqual(cols[7],"testtime");
assertEqual(cols[8],"testtimestamp");
console.log();


// cached result set with result set buffer size
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
console.log();


// from one cache file to another
console.log("FROM ONE CACHE FILE TO ANOTHER: ");
cur.cacheToFile("cachefile2");
assertEqual(cur.openCachedResultSet("cachefile1"),1);
cur.cacheOff();
assertEqual(cur.openCachedResultSet("cachefile2"),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
console.log();


// from one cache file to another with result set buffer size
console.log("FROM ONE CACHE FILE TO ANOTHER "+
		"WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile2");
assertEqual(cur.openCachedResultSet("cachefile1"),1);
cur.cacheOff();
assertEqual(cur.openCachedResultSet("cachefile2"),1);
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(8,0),null);
cur.setResultSetBufferSize(0);
console.log();


// cached result set with suspend and result set buffer size
console.log("CACHED RESULT SET WITH SUSPEND "+
		"AND RESULT SET BUFFER SIZE: ");
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
console.log();


// commit and rollback
console.log("COMMIT AND ROLLBACK: ");
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
//assertEqual(con.autoCommitOn(),1);
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	10, "+
	"	10.1, "+
	"	10.1, "+
	"	10, "+
	"	'testchar10', "+
	"	'testvarchar10', "+
	"	'01/01/2010', "+
	"	'10:00:00', "+
	"	null)"),1);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"9");
//assertEqual(con.autoCommitOff(),1);
console.log();


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
console.log();


// stored procedures
console.log("STORED PROCEDURES: ");
// return no values
cur.sendQuery("drop function testfunc(int,float,char(20))");
assertEqual(cur.sendQuery(
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
	"	return; "+
	"end;' language plpgsql"),1);
cur.prepareQuery("select testfunc($1,$2,$3)");
cur.inputBind("1",1);
cur.inputBind("2",1.1,4,2);
cur.inputBind("3","hello");
assertEqual(cur.executeQuery(),1);
cur.sendQuery("drop function testfunc(int,float,char(20))");
console.log();
// return single value
cur.sendQuery("drop function testfunc(int,float,char(20))");
assertEqual(cur.sendQuery(
	"create function testfunc(int,float,char(20)) returns int as "+
	"	' begin return $1; end;' language plpgsql"),1);
cur.prepareQuery("select * from testfunc($1,$2,$3)");
cur.inputBind("1",1);
cur.inputBind("2",1.1,4,2);
cur.inputBind("3","hello");
assertEqual(cur.executeQuery(),1);
assertEqual(cur.getField(0,0),"1");
cur.sendQuery("drop function testfunc(int,float,char(20))");
console.log();
// return multiple values
cur.sendQuery("drop function testfunc(int,char(20))");
assertEqual(cur.sendQuery(
	"create function testfunc("+
	"	int,float,char(20)) "+
	"returns record as ' "+
	"	declare output record; "+
	"begin "+
	"	select $1,$2,$3 into output; "+
	"	return output; "+
	"end;' language plpgsql"),1);
cur.prepareQuery(
	"select "+
	"	* "+
	"from "+
	"	testfunc($1,$2,$3) as (col1 int, col2 float, col3 bpchar) ");
cur.inputBind("1",1);
cur.inputBind("2",1.1,4,2);
cur.inputBind("3","hello");
assertEqual(cur.executeQuery(),1);
assertEqual(cur.getField(0,0),"1");
assertEqual(cur.getField(0,1),"1.1");
assertEqual(cur.getField(0,2),"hello");
cur.sendQuery("drop function testfunc(int,float,char(20))");
console.log();
// return result set
cur.sendQuery("drop function testfunc()");
assertEqual(cur.sendQuery(
	"create function testfunc() "+
	"returns setof record as ' "+
	"	declare output record; "+
	"begin "+
	"	for output in "+
	"		select * from testtable "+
	"	loop "+
	"		return next output; "+
	"	end loop; "+
	"	return; "+
	"end;' language plpgsql"),1);
assertEqual(cur.sendQuery(
	"select "+
	"	* "+
	"from "+
	"	testfunc() "+
	"	as (testint int, "+
	"		testfloat float, "+
	"		testreal real, "+
	"		testsmallint smallint, "+
	"		testchar char(40), "+
	"		testvarchar varchar(40), "+
	"		testdate date, "+
	"		testtime time, "+
	"		testtimestamp timestamp) "),1);
assertEqual(cur.getField(4,0),"5");
assertEqual(cur.getField(5,0),"6");
assertEqual(cur.getField(6,0),"7");
assertEqual(cur.getField(7,0),"8");
cur.sendQuery("drop function testfunc()");
console.log();

// drop existing table
cur.sendQuery("drop table testtable");


// invalid queries
console.log("INVALID QUERIES: ");
assertEqual(cur.sendQuery("select * from testtable order by testint"),0);
assertEqual(cur.sendQuery("select * from testtable order by testint"),0);
assertEqual(cur.sendQuery("select * from testtable order by testint"),0);
assertEqual(cur.sendQuery("select * from testtable order by testint"),0);
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
console.log();

reportTestStatus();

process.exit(getStatus());
