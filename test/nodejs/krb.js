// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{assertEqual, getStatus, reportTestStatus}=require("./asserts.js");


var	bindvars=["1","2","3","4","5"];
var	bindvals=["4","testchar4","testvarchar4","01-JAN-2004","testlong4"];
var	arraybindvars=["var1","var2","var3","var4","var5"];
var	arraybindvals=["7","testchar7",
			"testvarchar7","01-JAN-2007","testlong7"];
var	numvar;
var	stringvar;
var	floatvar;
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
var	clobvar;
var	clobvarlength;
var	blobvar;
var	blobvarlength;


// instantiation
var con=new sqlrelay.SQLRConnection("sqlrelay",
				9000,
				"/tmp/test.socket",
				null,null,0,1);
var cur=new sqlrelay.SQLRCursor(con);
con.enableKerberos(null,null,null);

// get database type


// identify
console.log("IDENTIFY: ");
assertEqual(con.identify(),"oracle");
console.log();


// ping
console.log("PING: ");
assertEqual(con.ping(),1);
console.log();

// drop existing table
cur.sendQuery("drop table testtable");


// create temptable
console.log("CREATE TEMPTABLE: ");
assertEqual(cur.sendQuery(
	"create table testtable ("+
	"	testnumber number, "+
	"	testchar char(40), "+
	"	testvarchar varchar2(40), "+
	"	testdate date, "+
	"	testlong long, "+
	"	testclob clob, "+
	"	testblob blob)"),1);
console.log();


// insert
console.log("INSERT: ");
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	'01-JAN-2001', "+
	"	'testlong1', "+
	"	'testclob1', "+
	"	empty_blob())"),1);
console.log();


// affected rows
console.log("AFFECTED ROWS: ");
assertEqual(cur.affectedRows(),1);
console.log();


// bind by position
console.log("BIND BY POSITION: ");
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
assertEqual(cur.countBindVariables(),7);
cur.inputBind("1",2);
cur.inputBind("2","testchar2");
cur.inputBind("3","testvarchar2");
cur.inputBind("4","01-JAN-2002");
cur.inputBind("5","testlong2");
cur.inputBindClob("6","testclob2",9);
cur.inputBindBlob("7","testblob2",9);
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2","testchar3");
cur.inputBind("3","testvarchar3");
cur.inputBind("4","01-JAN-2003");
cur.inputBind("5","testlong3");
cur.inputBindClob("6","testclob3",9);
cur.inputBindBlob("7","testblob3",9);
assertEqual(cur.executeQuery(),1);
console.log();


// array of binds by position
console.log("ARRAY OF BINDS BY POSITION: ");
cur.clearBinds();
cur.inputBinds(bindvars,bindvals);
cur.inputBindClob("var6","testclob4",9);
cur.inputBindBlob("var7","testblob4",9);
assertEqual(cur.executeQuery(),1);
console.log();


// bind by name
console.log("BIND BY NAME: ");
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
cur.inputBind("var1",5);
cur.inputBind("var2","testchar5");
cur.inputBind("var3","testvarchar5");
cur.inputBind("var4","01-JAN-2005");
cur.inputBind("var5","testlong5");
cur.inputBindClob("var6","testclob5",9);
cur.inputBindBlob("var7","testblob5",9);
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("var1",6);
cur.inputBind("var2","testchar6");
cur.inputBind("var3","testvarchar6");
cur.inputBind("var4","01-JAN-2006");
cur.inputBind("var5","testlong6");
cur.inputBindClob("var6","testclob6",9);
cur.inputBindBlob("var7","testblob6",9);
assertEqual(cur.executeQuery(),1);
console.log();


// array of binds by name
console.log("ARRAY OF BINDS BY NAME: ");
cur.clearBinds();
cur.inputBinds(arraybindvars,arraybindvals);
cur.inputBindClob("var6","testclob7",9);
cur.inputBindBlob("var7","testblob7",9);
assertEqual(cur.executeQuery(),1);
console.log();


// bind by name with validation
console.log("BIND BY NAME WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("var1",8);
cur.inputBind("var2","testchar8");
cur.inputBind("var3","testvarchar8");
cur.inputBind("var4","01-JAN-2008");
cur.inputBind("var5","testlong8");
cur.inputBindClob("var6","testclob8",9);
cur.inputBindBlob("var7","testblob8",9);
cur.inputBind("var9","junkvalue");
cur.validateBinds();
assertEqual(cur.executeQuery(),1);
console.log();


// select
console.log("SELECT: ");
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
console.log();


// column count
console.log("COLUMN COUNT: ");
assertEqual(cur.colCount(),7);
console.log();


// column names
console.log("COLUMN NAMES: ");
assertEqual(cur.getColumnName(0),"TESTNUMBER");
assertEqual(cur.getColumnName(1),"TESTCHAR");
assertEqual(cur.getColumnName(2),"TESTVARCHAR");
assertEqual(cur.getColumnName(3),"TESTDATE");
assertEqual(cur.getColumnName(4),"TESTLONG");
assertEqual(cur.getColumnName(5),"TESTCLOB");
assertEqual(cur.getColumnName(6),"TESTBLOB");
cols=cur.getColumnNames();
assertEqual(cols[0],"TESTNUMBER");
assertEqual(cols[1],"TESTCHAR");
assertEqual(cols[2],"TESTVARCHAR");
assertEqual(cols[3],"TESTDATE");
assertEqual(cols[4],"TESTLONG");
assertEqual(cols[5],"TESTCLOB");
assertEqual(cols[6],"TESTBLOB");
console.log();


// column types
console.log("COLUMN TYPES: ");
assertEqual(cur.getColumnType(0),"NUMBER");
assertEqual(cur.getColumnType("TESTNUMBER"),"NUMBER");
assertEqual(cur.getColumnType(1),"CHAR");
assertEqual(cur.getColumnType("TESTCHAR"),"CHAR");
assertEqual(cur.getColumnType(2),"VARCHAR2");
assertEqual(cur.getColumnType("TESTVARCHAR"),"VARCHAR2");
assertEqual(cur.getColumnType(3),"DATE");
assertEqual(cur.getColumnType("TESTDATE"),"DATE");
assertEqual(cur.getColumnType(4),"LONG");
assertEqual(cur.getColumnType("TESTLONG"),"LONG");
assertEqual(cur.getColumnType(5),"CLOB");
assertEqual(cur.getColumnType("TESTCLOB"),"CLOB");
assertEqual(cur.getColumnType(6),"BLOB");
assertEqual(cur.getColumnType("TESTBLOB"),"BLOB");
console.log();


// column length
console.log("COLUMN LENGTH: ");
assertEqual(cur.getColumnLength(0),22);
assertEqual(cur.getColumnLength("TESTNUMBER"),22);
assertEqual(cur.getColumnLength(1),40);
assertEqual(cur.getColumnLength("TESTCHAR"),40);
assertEqual(cur.getColumnLength(2),40);
assertEqual(cur.getColumnLength("TESTVARCHAR"),40);
assertEqual(cur.getColumnLength(3),7);
assertEqual(cur.getColumnLength("TESTDATE"),7);
assertEqual(cur.getColumnLength(4),0);
assertEqual(cur.getColumnLength("TESTLONG"),0);
assertEqual(cur.getColumnLength(5),0);
assertEqual(cur.getColumnLength("TESTCLOB"),0);
assertEqual(cur.getColumnLength(6),0);
assertEqual(cur.getColumnLength("TESTBLOB"),0);
console.log();


// longest column
console.log("LONGEST COLUMN: ");
assertEqual(cur.getLongest(0),1);
assertEqual(cur.getLongest("TESTNUMBER"),1);
assertEqual(cur.getLongest(1),40);
assertEqual(cur.getLongest("TESTCHAR"),40);
assertEqual(cur.getLongest(2),12);
assertEqual(cur.getLongest("TESTVARCHAR"),12);
assertEqual(cur.getLongest(3),9);
assertEqual(cur.getLongest("TESTDATE"),9);
assertEqual(cur.getLongest(4),9);
assertEqual(cur.getLongest("TESTLONG"),9);
assertEqual(cur.getLongest(5),9);
assertEqual(cur.getLongest("TESTCLOB"),9);
assertEqual(cur.getLongest(6),9);
assertEqual(cur.getLongest("TESTBLOB"),9);
console.log();


// row count
console.log("ROW COUNT: ");
assertEqual(cur.rowCount(),8);
console.log();


// total rows
console.log("TOTAL ROWS: ");
assertEqual(cur.totalRows(),0);
console.log();


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
assertEqual(cur.getField(0,1),"testchar1                               ");
assertEqual(cur.getField(0,2),"testvarchar1");
assertEqual(cur.getField(0,3),"01-JAN-01");
assertEqual(cur.getField(0,4),"testlong1");
assertEqual(cur.getField(0,5),"testclob1");
assertEqual(cur.getField(0,6),"");
console.log();
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(7,1),"testchar8                               ");
assertEqual(cur.getField(7,2),"testvarchar8");
assertEqual(cur.getField(7,3),"01-JAN-08");
assertEqual(cur.getField(7,4),"testlong8");
assertEqual(cur.getField(7,5),"testclob8");
assertEqual(cur.getField(7,6),"testblob8");
console.log();


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqual(cur.getFieldLength(0,0),1);
assertEqual(cur.getFieldLength(0,1),40);
assertEqual(cur.getFieldLength(0,2),12);
assertEqual(cur.getFieldLength(0,3),9);
assertEqual(cur.getFieldLength(0,4),9);
assertEqual(cur.getFieldLength(0,5),9);
assertEqual(cur.getFieldLength(0,6),0);
console.log();
assertEqual(cur.getFieldLength(7,0),1);
assertEqual(cur.getFieldLength(7,1),40);
assertEqual(cur.getFieldLength(7,2),12);
assertEqual(cur.getFieldLength(7,3),9);
assertEqual(cur.getFieldLength(7,4),9);
assertEqual(cur.getFieldLength(7,5),9);
assertEqual(cur.getFieldLength(7,6),9);
console.log();


// fields by name
console.log("FIELDS BY NAME: ");
assertEqual(cur.getField(0,"TESTNUMBER"),"1");
assertEqual(cur.getField(0,"TESTCHAR"),"testchar1                               ");
assertEqual(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
assertEqual(cur.getField(0,"TESTDATE"),"01-JAN-01");
assertEqual(cur.getField(0,"TESTLONG"),"testlong1");
assertEqual(cur.getField(0,"TESTCLOB"),"testclob1");
assertEqual(cur.getField(0,"TESTBLOB"),"");
console.log();
assertEqual(cur.getField(7,"TESTNUMBER"),"8");
assertEqual(cur.getField(7,"TESTCHAR"),"testchar8                               ");
assertEqual(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
assertEqual(cur.getField(7,"TESTDATE"),"01-JAN-08");
assertEqual(cur.getField(7,"TESTLONG"),"testlong8");
assertEqual(cur.getField(7,"TESTCLOB"),"testclob8");
assertEqual(cur.getField(7,"TESTBLOB"),"testblob8");
console.log();


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqual(cur.getFieldLength(0,"TESTNUMBER"),1);
assertEqual(cur.getFieldLength(0,"TESTCHAR"),40);
assertEqual(cur.getFieldLength(0,"TESTVARCHAR"),12);
assertEqual(cur.getFieldLength(0,"TESTDATE"),9);
assertEqual(cur.getFieldLength(0,"TESTLONG"),9);
assertEqual(cur.getFieldLength(0,"TESTCLOB"),9);
assertEqual(cur.getFieldLength(0,"TESTBLOB"),0);
console.log();
assertEqual(cur.getFieldLength(7,"TESTNUMBER"),1);
assertEqual(cur.getFieldLength(7,"TESTCHAR"),40);
assertEqual(cur.getFieldLength(7,"TESTVARCHAR"),12);
assertEqual(cur.getFieldLength(7,"TESTDATE"),9);
assertEqual(cur.getFieldLength(7,"TESTLONG"),9);
assertEqual(cur.getFieldLength(7,"TESTCLOB"),9);
assertEqual(cur.getFieldLength(7,"TESTBLOB"),9);
console.log();


// fields by array
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqual(fields[0],"1");
assertEqual(fields[1],"testchar1                               ");
assertEqual(fields[2],"testvarchar1");
assertEqual(fields[3],"01-JAN-01");
assertEqual(fields[4],"testlong1");
assertEqual(fields[5],"testclob1");
assertEqual(fields[6],"");
console.log();


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqual(fieldlens[0],1);
assertEqual(fieldlens[1],40);
assertEqual(fieldlens[2],12);
assertEqual(fieldlens[3],9);
assertEqual(fieldlens[4],9);
assertEqual(fieldlens[5],9);
assertEqual(fieldlens[6],0);
console.log();


// individual substitutions
console.log("INDIVIDUAL SUBSTITUTIONS: ");
cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual");
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
cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
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
cur.prepareQuery("select '$(var1)','$(var2)','$(var3)' from dual");
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
cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
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
assertEqual(cur.sendQuery("select null,1,null from dual"),1);
assertEqual(cur.getField(0,0),null);
assertEqual(cur.getField(0,1),"1");
assertEqual(cur.getField(0,2),null);
cur.getNullsAsEmptyStrings();
assertEqual(cur.sendQuery("select null,1,null from dual"),1);
assertEqual(cur.getField(0,0),"");
assertEqual(cur.getField(0,1),"1");
assertEqual(cur.getField(0,2),"");
cur.getNullsAsNulls();
console.log();


// result set buffer size
console.log("RESULT SET BUFFER SIZE: ");
assertEqual(cur.getResultSetBufferSize(),0);
cur.setResultSetBufferSize(2);
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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
console.log();


// dont get column info
console.log("DONT GET COLUMN INFO: ");
cur.dontGetColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
assertEqual(cur.getColumnName(0),null);
assertEqual(cur.getColumnLength(0),0);
assertEqual(cur.getColumnType(0),null);
cur.getColumnInfo();
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
assertEqual(cur.getColumnName(0),"TESTNUMBER");
assertEqual(cur.getColumnLength(0),22);
assertEqual(cur.getColumnType(0),"NUMBER");
console.log();


// suspended session
console.log("SUSPENDED SESSION: ");
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
filename=cur.getCacheFileName();
assertEqual(filename,"cachefile1");
cur.cacheOff();
assertEqual(cur.openCachedResultSet(filename),1);
assertEqual(cur.getField(7,0),"8");
console.log();


// column count for cached result set
console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
assertEqual(cur.colCount(),7);
console.log();


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqual(cur.getColumnName(0),"TESTNUMBER");
assertEqual(cur.getColumnName(1),"TESTCHAR");
assertEqual(cur.getColumnName(2),"TESTVARCHAR");
assertEqual(cur.getColumnName(3),"TESTDATE");
assertEqual(cur.getColumnName(4),"TESTLONG");
assertEqual(cur.getColumnName(5),"TESTCLOB");
assertEqual(cur.getColumnName(6),"TESTBLOB");
cols=cur.getColumnNames();
assertEqual(cols[0],"TESTNUMBER");
assertEqual(cols[1],"TESTCHAR");
assertEqual(cols[2],"TESTVARCHAR");
assertEqual(cols[3],"TESTDATE");
assertEqual(cols[4],"TESTLONG");
assertEqual(cols[5],"TESTCLOB");
assertEqual(cols[6],"TESTBLOB");
console.log();


// cached result set with result set buffer size
console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
cur.setResultSetBufferSize(2);
cur.cacheToFile("cachefile1");
cur.setCacheTtl(200);
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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
var secondcon=new sqlrelay.SQLRConnection("sqlrelay",
				9000,
				"/tmp/test.socket",
				null,null,0,1);
var secondcur=new sqlrelay.SQLRCursor(secondcon);
secondcon.enableKerberos(null,null,null);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"0");
assertEqual(con.commit(),1);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"8");
assertEqual(con.autoCommitOn(),1);
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	10, "+
	"	'testchar10', "+
	"	'testvarchar10', "+
	"	'01-JAN-2010', "+
	"	'testlong10', "+
	"	'testclob10', "+
	"	empty_blob())"),1);
assertEqual(secondcur.sendQuery("select count(*) from testtable"),1);
assertEqual(secondcur.getField(0,0),"9");
assertEqual(con.autoCommitOff(),1);
console.log();


// output bind by position
console.log("OUTPUT BIND BY POSITION: ");
cur.prepareQuery(
	"begin "+
	"	:numvar:=1; "+
	"	:stringvar:='hello'; "+
	"	:floatvar:=2.5; "+
	"end;");
cur.defineOutputBindInteger("1");
cur.defineOutputBindString("2",10);
cur.defineOutputBindDouble("3");
assertEqual(cur.executeQuery(),1);
numvar=cur.getOutputBindInteger("1");
stringvar=cur.getOutputBindString("2");
floatvar=cur.getOutputBindDouble("3");
assertEqual(numvar,1);
assertEqual(stringvar,"hello");
assertEqual(floatvar,2.5);
console.log();


// output bind by name
console.log("OUTPUT BIND BY NAME: ");
cur.clearBinds();
cur.defineOutputBindInteger("numvar");
cur.defineOutputBindString("stringvar",10);
cur.defineOutputBindDouble("floatvar");
assertEqual(cur.executeQuery(),1);
numvar=cur.getOutputBindInteger("numvar");
stringvar=cur.getOutputBindString("stringvar");
floatvar=cur.getOutputBindDouble("floatvar");
assertEqual(numvar,1);
assertEqual(stringvar,"hello");
assertEqual(floatvar,2.5);
console.log();


// output bind by name with validation
console.log("OUTPUT BIND BY NAME WITH VALIDATION: ");
cur.clearBinds();
cur.defineOutputBindInteger("numvar");
cur.defineOutputBindString("stringvar",10);
cur.defineOutputBindDouble("floatvar");
cur.defineOutputBindString("dummyvar",10);
cur.validateBinds();
assertEqual(cur.executeQuery(),1);
numvar=cur.getOutputBindInteger("numvar");
stringvar=cur.getOutputBindString("stringvar");
floatvar=cur.getOutputBindDouble("floatvar");
assertEqual(numvar,1);
assertEqual(stringvar,"hello");
assertEqual(floatvar,2.5);
console.log();


// clob and blob output bind
console.log("CLOB AND BLOB OUTPUT BIND:");
cur.sendQuery("drop table testtable1");
assertEqual(cur.sendQuery(
	"create table testtable1 ("+
	"	testclob clob, "+
	"	testblob blob)"),1);
cur.prepareQuery("insert into testtable1 values ('hello',:var1)");
cur.inputBindBlob("var1","hello",5);
assertEqual(cur.executeQuery(),1);
cur.prepareQuery(
	"begin "+
	"	select testclob into :clobvar from testtable1; "+
	"	select testblob into :blobvar from testtable1; "+
	"end;");
cur.defineOutputBindClob("clobvar");
cur.defineOutputBindBlob("blobvar");
assertEqual(cur.executeQuery(),1);
clobvar=cur.getOutputBindClob("clobvar");
clobvarlength=cur.getOutputBindLength("clobvar");
blobvar=cur.getOutputBindBlob("blobvar");
blobvarlength=cur.getOutputBindLength("blobvar");
assertEqual(clobvar,"hello",5);
assertEqual(clobvarlength,5);
assertEqual(blobvar,"hello",5);
assertEqual(blobvarlength,5);
cur.sendQuery("drop table testtable1");
console.log();


// null and empty clobs and clobs
console.log("NULL AND EMPTY CLOBS AND CLOBS:");
cur.getNullsAsNulls();
cur.sendQuery(
	"create table testtable1 ("+
	"	testclob1 clob, "+
	"	testclob2 clob, "+
	"	testblob1 blob, "+
	"	testblob2 blob)");
cur.prepareQuery("insert into testtable1 values (:var1,:var2,:var3,:var4)");
cur.inputBindClob("var1","",0);
cur.inputBindClob("var2",null,0);
cur.inputBindBlob("var3","",0);
cur.inputBindBlob("var4",null,0);
assertEqual(cur.executeQuery(),1);
cur.sendQuery("select * from testtable1");
assertEqual(cur.getField(0,0),null);
assertEqual(cur.getField(0,1),null);
assertEqual(cur.getField(0,2),null);
assertEqual(cur.getField(0,3),null);
cur.sendQuery("drop table testtable1");
console.log();


// cursor binds
console.log("CURSOR BINDS:");
assertEqual(cur.sendQuery(
	"create or replace "+
	"package types as "+
	"	type cursorType is ref cursor; "+
	"end;"),1);
assertEqual(cur.sendQuery(
	"create or replace "+
	"function sp_testtable "+
	"return types.cursortype "+
	"as "+
	"	l_cursor    types.cursorType; "+
	"begin "+
	"	open l_cursor for "+
	"		select * from testtable; "+
	"	return l_cursor; "+
	"end;"),1);
cur.prepareQuery("begin  :curs:=sp_testtable; end;");
cur.defineOutputBindCursor("curs");
assertEqual(cur.executeQuery(),1);
var	bindcur=cur.getOutputBindCursor("curs");
assertEqual(bindcur.fetchFromBindCursor(),1);
assertEqual(bindcur.getField(0,0),"1");
assertEqual(bindcur.getField(1,0),"2");
assertEqual(bindcur.getField(2,0),"3");
assertEqual(bindcur.getField(3,0),"4");
assertEqual(bindcur.getField(4,0),"5");
assertEqual(bindcur.getField(5,0),"6");
assertEqual(bindcur.getField(6,0),"7");
assertEqual(bindcur.getField(7,0),"8");
console.log();


// long clob
console.log("LONG CLOB:");
cur.sendQuery("drop table testtable2");
cur.sendQuery("create table testtable2 (testclob clob)");
cur.prepareQuery("insert into testtable2 values (:clobval)");
var clobval=""
for (var i=0; i<8*1024; i++) {
	clobval=clobval+"C";
}
cur.inputBindClob("clobval",clobval,8*1024);
assertEqual(cur.executeQuery(),1);
cur.sendQuery("select testclob from testtable2");
assertEqual(clobval,cur.getField(0,"TESTCLOB"));
cur.prepareQuery(
	"begin select testclob into :clobbindval from testtable2; "+
	"	end;");
cur.defineOutputBindClob("clobbindval");
assertEqual(cur.executeQuery(),1);
var	clobbindvar=cur.getOutputBindClob("clobbindval");
assertEqual(cur.getOutputBindLength("clobbindval"),8*1024);
assertEqual(clobval,clobbindvar);
cur.sendQuery("drop table testtable2");
console.log();


console.log("LONG OUTPUT BIND");
cur.sendQuery("drop table testtable2");
cur.sendQuery("create table testtable2 (testval varchar2(4000))");
cur.prepareQuery("insert into testtable2 values (:testval)");
var testval="";
for (var i=0; i<4000; i++) {
	testval=testval+"C";
}
cur.inputBind("testval",testval);
assertEqual(cur.executeQuery(),1);
cur.sendQuery("select testval from testtable2");
assertEqual(testval,cur.getField(0,"TESTVAL"));
var query=""
query=query+"begin :bindval:='";
query=query+testval;
query=query+"'; end;";
cur.prepareQuery(query);
cur.defineOutputBindString("bindval",4000);
assertEqual(cur.executeQuery(),1);
assertEqual(cur.getOutputBindLength("bindval"),4000);
assertEqual(cur.getOutputBindString("bindval"),testval);
cur.sendQuery("drop table testtable2");
console.log();

console.log("NEGATIVE INPUT BIND");
cur.sendQuery("create table testtable2 (testval number)");
cur.prepareQuery("insert into testtable2 values (:testval)");
cur.inputBind("testval",-1);
assertEqual(cur.executeQuery(),1);
cur.sendQuery("select testval from testtable2");
assertEqual(cur.getField(0,"TESTVAL"),"-1");
cur.sendQuery("drop table testtable2");
console.log();


// finished suspended session
console.log("FINISHED SUSPENDED SESSION: ");
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),1);
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

// drop existing table
cur.sendQuery("drop table testtable");


// bind validation
console.log("BIND VALIDATION: ");
cur.sendQuery("drop table testtable1");
cur.sendQuery(
	"create table testtable1 ("+
	"	col1 varchar2(20), "+
	"	col2 varchar2(20), "+
	"	col3 varchar2(20))");
cur.prepareQuery("insert into testtable1 values ($(var1),$(var2),$(var3))");
cur.inputBind("var1",1);
cur.inputBind("var2",2);
cur.inputBind("var3",3);
cur.substitution("var1",":var1");
assertEqual(cur.validBind("var1"),1);
assertEqual(cur.validBind("var2"),0);
assertEqual(cur.validBind("var3"),0);
assertEqual(cur.validBind("var4"),0);
console.log();
cur.substitution("var2",":var2");
assertEqual(cur.validBind("var1"),1);
assertEqual(cur.validBind("var2"),1);
assertEqual(cur.validBind("var3"),0);
assertEqual(cur.validBind("var4"),0);
console.log();
cur.substitution("var3",":var3");
assertEqual(cur.validBind("var1"),1);
assertEqual(cur.validBind("var2"),1);
assertEqual(cur.validBind("var3"),1);
assertEqual(cur.validBind("var4"),0);
assertEqual(cur.executeQuery(),1);
cur.sendQuery("drop table testtable1");
console.log();


// invalid queries
console.log("INVALID QUERIES: ");
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),0);
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),0);
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),0);
assertEqual(cur.sendQuery("select * from testtable order by testnumber"),0);
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
