// Copyright (c) David Muse
// See the file COPYING for more information.

var	sqlrelay=require("sqlrelay");
var	{assertEqual, getStatus, reportTestStatus}=require("./asserts.js");




var	bindvars=["1","2","3","4","5","6",
			"7","8","9","10","11","12",
			"13"];
var	bindvals=["4","4","4","4.4","4.4","4.4","4.4",
			"4.00","4.00",
			"01-Jan-2004 04:00:00",
			"01-Jan-2004 04:00:00",
			"testchar4","testvarchar4"];
var	arraybindvars=["var1","var2","var3","var4","var5","var6",
			"var7","var8","var9","var10","var11","var12",
			"var13"];
var	arraybindvals=["7","7","7","7.7","7.7","7.7","7.7",
			"7.00","7.00",
			"01-Jan-2007 07:00:00",
			"01-Jan-2007 07:00:00",
			"testchar7","testvarchar7"];
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
var con=new sqlrelay.SQLRConnection("sqlrelay",
				9000,
				"/tmp/test.socket",
				"testuser","testpassword",0,1);
var cur=new sqlrelay.SQLRCursor(con);

// get database type


// identify
console.log("IDENTIFY: ");
assertEqual(con.identify(),"sap");
console.log();


// ping
console.log("PING: ");
assertEqual(con.ping(),1);
console.log();


// isolation levels
console.log("ISOLATION LEVELS: ");
var	isolationlevels=["1","0","2","3"];
for (var i=0; i<isolationlevels.length; i++) {
	assertEqual(con.setIsolationLevel(isolationlevels[i]),1);
	assertEqual(con.getIsolationLevel(),isolationlevels[i]);
	console.log();
}
// reset to the default isolation level
assertEqual(con.setIsolationLevel(isolationlevels[0]),1);
console.log();

// drop existing table
cur.sendQuery("drop table testtable");


// create temptable
console.log("CREATE TEMPTABLE: ");
assertEqual(cur.sendQuery(
	"create table testtable ("+
	"	testint int, "+
	"	testsmallint smallint, "+
	"	testtinyint tinyint, "+
	"	testreal real, "+
	"	testfloat float, "+
	"	testdecimal decimal(4,1), "+
	"	testnumeric numeric(4,1), "+
	"	testmoney money, "+
	"	testsmallmoney smallmoney, "+
	"	testdatetime datetime, "+
	"	testsmalldatetime smalldatetime, "+
	"	testchar char(40), "+
	"	testvarchar varchar(40), "+
	"	testbit bit)"),1);
console.log();


// begin transaction
console.log("BEGIN TRANSACTION: ");
//assertEqual(cur.sendQuery("begin tran"),1);
console.log();


// insert
console.log("INSERT: ");
assertEqual(cur.sendQuery(
	"insert into "+
	"	testtable "+
	"values ("+
	"	1, "+
	"	1, "+
	"	1, "+
	"	1.1, "+
	"	1.1, "+
	"	1.1, "+
	"	1.1, "+
	"	1.00, "+
	"	1.00, "+
	"	'01-Jan-2001 01:00:00', "+
	"	'01-Jan-2001 01:00:00', "+
	"	'testchar1', "+
	"	'testvarchar1', "+
	"	1)"),1);
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
	"	@var1, "+
	"	@var2, "+
	"	@var3, "+
	"	@var4, "+
	"	@var5, "+
	"	@var6, "+
	"	@var7, "+
	"	@var8, "+
	"	@var9, "+
	"	@var10, "+
	"	@var11, "+
	"	@var12, "+
	"	@var13, "+
	"	@var14)");
assertEqual(cur.countBindVariables(),14);
cur.inputBind("1",2);
cur.inputBind("2",2);
cur.inputBind("3",2);
cur.inputBind("4",2.2,2,1);
cur.inputBind("5",2.2,2,1);
cur.inputBind("6",2.2,2,1);
cur.inputBind("7",2.2,2,1);
cur.inputBind("8",2.00,3,2);
cur.inputBind("9",2.00,3,2);
cur.inputBind("10","01-Jan-2002 02:00:00");
cur.inputBind("11","01-Jan-2002 02:00:00");
cur.inputBind("12","testchar2");
cur.inputBind("13","testvarchar2");
cur.inputBind("14",1);
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("1",3);
cur.inputBind("2",3);
cur.inputBind("3",3);
cur.inputBind("4",3.3,2,1);
cur.inputBind("5",3.3,2,1);
cur.inputBind("6",3.3,2,1);
cur.inputBind("7",3.3,2,1);
cur.inputBind("8",3.00,3,2);
cur.inputBind("9",3.00,3,2);
cur.inputBind("10","01-Jan-2003 03:00:00");
cur.inputBind("11","01-Jan-2003 03:00:00");
cur.inputBind("12","testchar3");
cur.inputBind("13","testvarchar3");
cur.inputBind("14",1);
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("1",4);
cur.inputBind("2",4);
cur.inputBind("3",4);
cur.inputBind("4",4.4,2,1);
cur.inputBind("5",4.4,2,1);
cur.inputBind("6",4.4,2,1);
cur.inputBind("7",4.4,2,1);
cur.inputBind("8",4.00,3,2);
cur.inputBind("9",4.00,3,2);
cur.inputBind("10","01-Jan-2004 04:00:00");
cur.inputBind("11","01-Jan-2004 04:00:00");
cur.inputBind("12","testchar4");
cur.inputBind("13","testvarchar4");
cur.inputBind("14",1);
assertEqual(cur.executeQuery(),1);
console.log();


// bind by name
console.log("BIND BY NAME: ");
cur.clearBinds();
cur.inputBind("var1",5);
cur.inputBind("var2",5);
cur.inputBind("var3",5);
cur.inputBind("var4",5.5,2,1);
cur.inputBind("var5",5.5,2,1);
cur.inputBind("var6",5.5,2,1);
cur.inputBind("var7",5.5,2,1);
cur.inputBind("var8",5.00,3,2);
cur.inputBind("var9",5.00,3,2);
cur.inputBind("var10","01-Jan-2005 05:00:00");
cur.inputBind("var11","01-Jan-2005 05:00:00");
cur.inputBind("var12","testchar5");
cur.inputBind("var13","testvarchar5");
cur.inputBind("var14",1);
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("var1",6);
cur.inputBind("var2",6);
cur.inputBind("var3",6);
cur.inputBind("var4",6.6,2,1);
cur.inputBind("var5",6.6,2,1);
cur.inputBind("var6",6.6,2,1);
cur.inputBind("var7",6.6,2,1);
cur.inputBind("var8",6.00,3,2);
cur.inputBind("var9",6.00,3,2);
cur.inputBind("var10","01-Jan-2006 06:00:00");
cur.inputBind("var11","01-Jan-2006 06:00:00");
cur.inputBind("var12","testchar6");
cur.inputBind("var13","testvarchar6");
cur.inputBind("var14",1);
assertEqual(cur.executeQuery(),1);
cur.clearBinds();
cur.inputBind("var1",7);
cur.inputBind("var2",7);
cur.inputBind("var3",7);
cur.inputBind("var4",7.7,2,1);
cur.inputBind("var5",7.7,2,1);
cur.inputBind("var6",7.7,2,1);
cur.inputBind("var7",7.7,2,1);
cur.inputBind("var8",7.00,3,2);
cur.inputBind("var9",7.00,3,2);
cur.inputBind("var10","01-Jan-2007 07:00:00");
cur.inputBind("var11","01-Jan-2007 07:00:00");
cur.inputBind("var12","testchar7");
cur.inputBind("var13","testvarchar7");
cur.inputBind("var14",1);
assertEqual(cur.executeQuery(),1);
console.log();


// bind by name with validation
console.log("BIND BY NAME WITH VALIDATION: ");
cur.clearBinds();
cur.inputBind("var1",8);
cur.inputBind("var2",8);
cur.inputBind("var3",8);
cur.inputBind("var4",8.8,2,1);
cur.inputBind("var5",8.8,2,1);
cur.inputBind("var6",8.8,2,1);
cur.inputBind("var7",8.8,2,1);
cur.inputBind("var8",8.00,3,2);
cur.inputBind("var9",8.00,3,2);
cur.inputBind("var10","01-Jan-2008 08:00:00");
cur.inputBind("var11","01-Jan-2008 08:00:00");
cur.inputBind("var12","testchar8");
cur.inputBind("var13","testvarchar8");
cur.inputBind("var14",1);
cur.inputBind("var15","junkvalue");
cur.validateBinds();
assertEqual(cur.executeQuery(),1);
console.log();


// select
console.log("SELECT: ");
assertEqual(cur.sendQuery("select * from testtable order by testint"),1);
console.log();


// column count
console.log("COLUMN COUNT: ");
assertEqual(cur.colCount(),14);
console.log();


// column names
console.log("COLUMN NAMES: ");
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnName(1),"testsmallint");
assertEqual(cur.getColumnName(2),"testtinyint");
assertEqual(cur.getColumnName(3),"testreal");
assertEqual(cur.getColumnName(4),"testfloat");
assertEqual(cur.getColumnName(5),"testdecimal");
assertEqual(cur.getColumnName(6),"testnumeric");
assertEqual(cur.getColumnName(7),"testmoney");
assertEqual(cur.getColumnName(8),"testsmallmoney");
assertEqual(cur.getColumnName(9),"testdatetime");
assertEqual(cur.getColumnName(10),"testsmalldatetime");
assertEqual(cur.getColumnName(11),"testchar");
assertEqual(cur.getColumnName(12),"testvarchar");
assertEqual(cur.getColumnName(13),"testbit");
cols=cur.getColumnNames();
assertEqual(cols[0],"testint");
assertEqual(cols[1],"testsmallint");
assertEqual(cols[2],"testtinyint");
assertEqual(cols[3],"testreal");
assertEqual(cols[4],"testfloat");
assertEqual(cols[5],"testdecimal");
assertEqual(cols[6],"testnumeric");
assertEqual(cols[7],"testmoney");
assertEqual(cols[8],"testsmallmoney");
assertEqual(cols[9],"testdatetime");
assertEqual(cols[10],"testsmalldatetime");
assertEqual(cols[11],"testchar");
assertEqual(cols[12],"testvarchar");
assertEqual(cols[13],"testbit");
console.log();


// column types
console.log("COLUMN TYPES: ");
assertEqual(cur.getColumnType(0),"INT");
assertEqual(cur.getColumnType("testint"),"INT");
assertEqual(cur.getColumnType(1),"SMALLINT");
assertEqual(cur.getColumnType("testsmallint"),"SMALLINT");
assertEqual(cur.getColumnType(2),"TINYINT");
assertEqual(cur.getColumnType("testtinyint"),"TINYINT");
assertEqual(cur.getColumnType(3),"REAL");
assertEqual(cur.getColumnType("testreal"),"REAL");
assertEqual(cur.getColumnType(4),"FLOAT");
assertEqual(cur.getColumnType("testfloat"),"FLOAT");
assertEqual(cur.getColumnType(5),"DECIMAL");
assertEqual(cur.getColumnType("testdecimal"),"DECIMAL");
assertEqual(cur.getColumnType(6),"NUMERIC");
assertEqual(cur.getColumnType("testnumeric"),"NUMERIC");
assertEqual(cur.getColumnType(7),"MONEY");
assertEqual(cur.getColumnType("testmoney"),"MONEY");
assertEqual(cur.getColumnType(8),"SMALLMONEY");
assertEqual(cur.getColumnType("testsmallmoney"),"SMALLMONEY");
assertEqual(cur.getColumnType(9),"DATETIME");
assertEqual(cur.getColumnType("testdatetime"),"DATETIME");
assertEqual(cur.getColumnType(10),"SMALLDATETIME");
assertEqual(cur.getColumnType("testsmalldatetime"),"SMALLDATETIME");
assertEqual(cur.getColumnType(11),"CHAR");
assertEqual(cur.getColumnType("testchar"),"CHAR");
assertEqual(cur.getColumnType(12),"CHAR");
assertEqual(cur.getColumnType("testvarchar"),"CHAR");
assertEqual(cur.getColumnType(13),"BIT");
assertEqual(cur.getColumnType("testbit"),"BIT");
console.log();


// column length
console.log("COLUMN LENGTH: ");
assertEqual(cur.getColumnLength(0),4);
assertEqual(cur.getColumnLength("testint"),4);
assertEqual(cur.getColumnLength(1),2);
assertEqual(cur.getColumnLength("testsmallint"),2);
assertEqual(cur.getColumnLength(2),1);
assertEqual(cur.getColumnLength("testtinyint"),1);
assertEqual(cur.getColumnLength(3),4);
assertEqual(cur.getColumnLength("testreal"),4);
assertEqual(cur.getColumnLength(4),8);
assertEqual(cur.getColumnLength("testfloat"),8);
assertEqual(cur.getColumnLength(5),35);
assertEqual(cur.getColumnLength("testdecimal"),35);
assertEqual(cur.getColumnLength(6),35);
assertEqual(cur.getColumnLength("testnumeric"),35);
assertEqual(cur.getColumnLength(7),8);
assertEqual(cur.getColumnLength("testmoney"),8);
assertEqual(cur.getColumnLength(8),4);
assertEqual(cur.getColumnLength("testsmallmoney"),4);
assertEqual(cur.getColumnLength(9),8);
assertEqual(cur.getColumnLength("testdatetime"),8);
assertEqual(cur.getColumnLength(10),4);
assertEqual(cur.getColumnLength("testsmalldatetime"),4);
assertEqual(cur.getColumnLength(11),40);
assertEqual(cur.getColumnLength("testchar"),40);
assertEqual(cur.getColumnLength(12),40);
assertEqual(cur.getColumnLength("testvarchar"),40);
assertEqual(cur.getColumnLength(13),1);
assertEqual(cur.getColumnLength("testbit"),1);
console.log();


// longest column
console.log("LONGEST COLUMN: ");
assertEqual(cur.getLongest(0),1);
assertEqual(cur.getLongest("testint"),1);
assertEqual(cur.getLongest(1),1);
assertEqual(cur.getLongest("testsmallint"),1);
assertEqual(cur.getLongest(2),1);
assertEqual(cur.getLongest("testtinyint"),1);
assertEqual(cur.getLongest(3),18);
assertEqual(cur.getLongest("testreal"),18);
assertEqual(cur.getLongest(4),18);
assertEqual(cur.getLongest("testfloat"),18);
assertEqual(cur.getLongest(5),3);
assertEqual(cur.getLongest("testdecimal"),3);
assertEqual(cur.getLongest(6),3);
assertEqual(cur.getLongest("testnumeric"),3);
assertEqual(cur.getLongest(7),4);
assertEqual(cur.getLongest("testmoney"),4);
assertEqual(cur.getLongest(8),4);
assertEqual(cur.getLongest("testsmallmoney"),4);
assertEqual(cur.getLongest(9),19);
assertEqual(cur.getLongest("testdatetime"),19);
assertEqual(cur.getLongest(10),19);
assertEqual(cur.getLongest("testsmalldatetime"),19);
assertEqual(cur.getLongest(11),40);
assertEqual(cur.getLongest("testchar"),40);
assertEqual(cur.getLongest(12),12);
assertEqual(cur.getLongest("testvarchar"),12);
assertEqual(cur.getLongest(13),1);
assertEqual(cur.getLongest("testbit"),1);
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
assertEqual(cur.getField(0,1),"1");
assertEqual(cur.getField(0,2),"1");
//assertEqual(cur.getField(0,3),"1.1");
//assertEqual(cur.getField(0,4),"1.1");
assertEqual(cur.getField(0,5),"1.1");
assertEqual(cur.getField(0,6),"1.1");
assertEqual(cur.getField(0,7),"1.00");
assertEqual(cur.getField(0,8),"1.00");
assertEqual(cur.getField(0,9),"Jan  1 2001  1:00AM");
assertEqual(cur.getField(0,10),"Jan  1 2001  1:00AM");
assertEqual(cur.getField(0,11),"testchar1                               ");
assertEqual(cur.getField(0,12),"testvarchar1");
assertEqual(cur.getField(0,13),"1");
console.log();
assertEqual(cur.getField(7,0),"8");
assertEqual(cur.getField(7,1),"8");
assertEqual(cur.getField(7,2),"8");
//assertEqual(cur.getField(7,3),"8.8");
//assertEqual(cur.getField(7,4),"8.8");
assertEqual(cur.getField(7,5),"8.8");
assertEqual(cur.getField(7,6),"8.8");
assertEqual(cur.getField(7,7),"8.00");
assertEqual(cur.getField(7,8),"8.00");
assertEqual(cur.getField(7,9),"Jan  1 2008  8:00AM");
assertEqual(cur.getField(7,10),"Jan  1 2008  8:00AM");
assertEqual(cur.getField(7,11),"testchar8                               ");
assertEqual(cur.getField(7,12),"testvarchar8");
assertEqual(cur.getField(7,13),"1");
console.log();


// field lengths by index
console.log("FIELD LENGTHS BY INDEX: ");
assertEqual(cur.getFieldLength(0,0),1);
assertEqual(cur.getFieldLength(0,1),1);
assertEqual(cur.getFieldLength(0,2),1);
assertEqual(cur.getFieldLength(0,3),18);
assertEqual(cur.getFieldLength(0,4),18);
assertEqual(cur.getFieldLength(0,5),3);
assertEqual(cur.getFieldLength(0,6),3);
assertEqual(cur.getFieldLength(0,7),4);
assertEqual(cur.getFieldLength(0,8),4);
assertEqual(cur.getFieldLength(0,9),19);
assertEqual(cur.getFieldLength(0,10),19);
assertEqual(cur.getFieldLength(0,11),40);
assertEqual(cur.getFieldLength(0,12),12);
assertEqual(cur.getFieldLength(0,13),1);
console.log();
assertEqual(cur.getFieldLength(7,0),1);
assertEqual(cur.getFieldLength(7,1),1);
assertEqual(cur.getFieldLength(7,2),1);
assertEqual(cur.getFieldLength(7,3),18);
assertEqual(cur.getFieldLength(7,4),18);
assertEqual(cur.getFieldLength(7,5),3);
assertEqual(cur.getFieldLength(7,6),3);
assertEqual(cur.getFieldLength(7,7),4);
assertEqual(cur.getFieldLength(7,8),4);
assertEqual(cur.getFieldLength(7,9),19);
assertEqual(cur.getFieldLength(7,10),19);
assertEqual(cur.getFieldLength(7,11),40);
assertEqual(cur.getFieldLength(7,12),12);
assertEqual(cur.getFieldLength(7,13),1);
console.log();


// fields by name
console.log("FIELDS BY NAME: ");
assertEqual(cur.getField(0,"testint"),"1");
assertEqual(cur.getField(0,"testsmallint"),"1");
assertEqual(cur.getField(0,"testtinyint"),"1");
//assertEqual(cur.getField(0,"testreal"),"1.1");
//assertEqual(cur.getField(0,"testfloat"),"1.1");
assertEqual(cur.getField(0,"testdecimal"),"1.1");
assertEqual(cur.getField(0,"testnumeric"),"1.1");
assertEqual(cur.getField(0,"testmoney"),"1.00");
assertEqual(cur.getField(0,"testsmallmoney"),"1.00");
assertEqual(cur.getField(0,"testdatetime"),"Jan  1 2001  1:00AM");
assertEqual(cur.getField(0,"testsmalldatetime"),"Jan  1 2001  1:00AM");
assertEqual(cur.getField(0,"testchar"),"testchar1                               ");
assertEqual(cur.getField(0,"testvarchar"),"testvarchar1");
assertEqual(cur.getField(0,"testbit"),"1");
console.log();
assertEqual(cur.getField(7,"testint"),"8");
assertEqual(cur.getField(7,"testsmallint"),"8");
assertEqual(cur.getField(7,"testtinyint"),"8");
//assertEqual(cur.getField(7,"testreal"),"8.8");
//assertEqual(cur.getField(7,"testfloat"),"8.8");
assertEqual(cur.getField(7,"testdecimal"),"8.8");
assertEqual(cur.getField(7,"testnumeric"),"8.8");
assertEqual(cur.getField(7,"testmoney"),"8.00");
assertEqual(cur.getField(7,"testsmallmoney"),"8.00");
assertEqual(cur.getField(7,"testdatetime"),"Jan  1 2008  8:00AM");
assertEqual(cur.getField(7,"testsmalldatetime"),"Jan  1 2008  8:00AM");
assertEqual(cur.getField(7,"testchar"),"testchar8                               ");
assertEqual(cur.getField(7,"testvarchar"),"testvarchar8");
assertEqual(cur.getField(7,"testbit"),"1");
console.log();


// field lengths by name
console.log("FIELD LENGTHS BY NAME: ");
assertEqual(cur.getFieldLength(0,"testint"),1);
assertEqual(cur.getFieldLength(0,"testsmallint"),1);
assertEqual(cur.getFieldLength(0,"testtinyint"),1);
//assertEqual(cur.getFieldLength(0,"testreal"),3);
//assertEqual(cur.getFieldLength(0,"testfloat"),3);
assertEqual(cur.getFieldLength(0,"testdecimal"),3);
assertEqual(cur.getFieldLength(0,"testnumeric"),3);
assertEqual(cur.getFieldLength(0,"testmoney"),4);
assertEqual(cur.getFieldLength(0,"testsmallmoney"),4);
assertEqual(cur.getFieldLength(0,"testdatetime"),19);
assertEqual(cur.getFieldLength(0,"testsmalldatetime"),19);
assertEqual(cur.getFieldLength(0,"testchar"),40);
assertEqual(cur.getFieldLength(0,"testvarchar"),12);
assertEqual(cur.getFieldLength(0,"testbit"),1);
console.log();
assertEqual(cur.getFieldLength(7,"testint"),1);
assertEqual(cur.getFieldLength(7,"testsmallint"),1);
assertEqual(cur.getFieldLength(7,"testtinyint"),1);
//assertEqual(cur.getFieldLength(7,"testreal"),3);
//assertEqual(cur.getFieldLength(7,"testfloat"),3);
assertEqual(cur.getFieldLength(7,"testdecimal"),3);
assertEqual(cur.getFieldLength(7,"testnumeric"),3);
assertEqual(cur.getFieldLength(7,"testmoney"),4);
assertEqual(cur.getFieldLength(7,"testsmallmoney"),4);
assertEqual(cur.getFieldLength(7,"testdatetime"),19);
assertEqual(cur.getFieldLength(7,"testsmalldatetime"),19);
assertEqual(cur.getFieldLength(7,"testchar"),40);
assertEqual(cur.getFieldLength(7,"testvarchar"),12);
assertEqual(cur.getFieldLength(7,"testbit"),1);
console.log();


// fields by array
console.log("FIELDS BY ARRAY: ");
fields=cur.getRow(0);
assertEqual(fields[0],"1");
assertEqual(fields[1],"1");
assertEqual(fields[2],"1");
//assertEqual(fields[3],"1.1");
//assertEqual(fields[4],"1.1");
assertEqual(fields[5],"1.1");
assertEqual(fields[6],"1.1");
assertEqual(fields[7],"1.00");
assertEqual(fields[8],"1.00");
assertEqual(fields[9],"Jan  1 2001  1:00AM");
assertEqual(fields[10],"Jan  1 2001  1:00AM");
assertEqual(fields[11],"testchar1                               ");
assertEqual(fields[12],"testvarchar1");
assertEqual(fields[13],"1");
console.log();


// field lengths by array
console.log("FIELD LENGTHS BY ARRAY: ");
fieldlens=cur.getRowLengths(0);
assertEqual(fieldlens[0],1);
assertEqual(fieldlens[1],1);
assertEqual(fieldlens[2],1);
//assertEqual(fieldlens[3],3);
//assertEqual(fieldlens[4],3);
assertEqual(fieldlens[5],3);
assertEqual(fieldlens[6],3);
assertEqual(fieldlens[7],4);
assertEqual(fieldlens[8],4);
assertEqual(fieldlens[9],19);
assertEqual(fieldlens[10],19);
assertEqual(fieldlens[11],40);
assertEqual(fieldlens[12],12);
assertEqual(fieldlens[13],1);
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
assertEqual(cur.getColumnType(0),"INT");
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
assertEqual(cur.colCount(),14);
console.log();


// column names for cached result set
console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
assertEqual(cur.getColumnName(0),"testint");
assertEqual(cur.getColumnName(1),"testsmallint");
assertEqual(cur.getColumnName(2),"testtinyint");
assertEqual(cur.getColumnName(3),"testreal");
assertEqual(cur.getColumnName(4),"testfloat");
assertEqual(cur.getColumnName(5),"testdecimal");
assertEqual(cur.getColumnName(6),"testnumeric");
assertEqual(cur.getColumnName(7),"testmoney");
assertEqual(cur.getColumnName(8),"testsmallmoney");
assertEqual(cur.getColumnName(9),"testdatetime");
assertEqual(cur.getColumnName(10),"testsmalldatetime");
assertEqual(cur.getColumnName(11),"testchar");
assertEqual(cur.getColumnName(12),"testvarchar");
assertEqual(cur.getColumnName(13),"testbit");
cols=cur.getColumnNames();
assertEqual(cols[0],"testint");
assertEqual(cols[1],"testsmallint");
assertEqual(cols[2],"testtinyint");
assertEqual(cols[3],"testreal");
assertEqual(cols[4],"testfloat");
assertEqual(cols[5],"testdecimal");
assertEqual(cols[6],"testnumeric");
assertEqual(cols[7],"testmoney");
assertEqual(cols[8],"testsmallmoney");
assertEqual(cols[9],"testdatetime");
assertEqual(cols[10],"testsmalldatetime");
assertEqual(cols[11],"testchar");
assertEqual(cols[12],"testvarchar");
assertEqual(cols[13],"testbit");
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
