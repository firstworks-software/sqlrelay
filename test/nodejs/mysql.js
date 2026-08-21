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


// hostname
var hostname=require("os").hostname().toLowerCase();
var dot=hostname.indexOf(".");
if (dot>-1) {
	hostname=hostname.substring(0,dot);
}


	// instantiation
	var con=new sqlrelay.SQLRConnection("sqlrelay",9002,"/tmp/mysql.socket",
			"testuser","testpassword",0,1);
	var cur=new sqlrelay.SQLRCursor(con);
	setConnection(con);
	setCursor(cur);


	// identify
	console.log("IDENTIFY: ");
	assertEqStr(con.identify(),"mysql");
	console.log("");


	// db version
	console.log("DB VERSION: ");
	var dbversion=con.dbVersion();
	var majorversion=parseInt(dbversion[0]);
	console.log("");


	// ping
	console.log("PING: ");
	assertTrue(con.ping());
	console.log("");


	// transaction state
	console.log("TRANSACTION STATE: ");
	assertEqStr(con.getDefaultTransactionModel(),"explicit-deferred");
	assertEqStr(con.getTransactionModel(),"explicit-deferred");
	assertFalse(con.getInTransaction());
	assertTrue(con.getAutoCommit());
	console.log("");


	// bind format
	console.log("BIND FORMAT: ");
	if (majorversion>3) {
		assertEqStr(con.bindFormat(),"?");
	} else {
		assertEqStr(con.bindFormat(),":*");
	}
	console.log("");


	// nextval format
	console.log("NEXTVAL FORMAT: ");
	assertEqStr(con.nextvalFormat(),"");
	console.log("");


	// isolation levels
	// (mysql before 4.0 doesn't support setting the isolation level)
	console.log("ISOLATION LEVELS: ");
	var isolationlevels=["REPEATABLE-READ","READ-UNCOMMITTED",
				"READ-COMMITTED","SERIALIZABLE"];
	if (majorversion>3) {
		for (var i=0;i<isolationlevels.length;i++) {
			var il=isolationlevels[i];
			assertTrue(con.setIsolationLevel(il));
			assertEqStr(con.getIsolationLevel(),il);
			console.log("");
		}
		// reset to the default isolation level
		assertTrue(con.setIsolationLevel(isolationlevels[0]));
	}
	console.log("");


	// create testtable
	console.log("CREATE TESTTABLE: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery(
		"create table testtable ("+
		"	testtinyint tinyint, "+
		"	testsmallint smallint, "+
		"	testmediumint mediumint,"+
		"	testint int, "+
		"	testbigint bigint, "+
		"	testfloat float, "+
		"	testreal real, "+
		"	testdecimal decimal(2,1),"+
		"	testdate date, "+
		"	testtime time, "+
		"	testdatetime datetime, "+
		"	testyear year, "+
		"	testchar char(40), "+
		"	testvarchar varchar(40),"+
		"	testtext text, "+
		"	testtinytext tinytext, "+
		"	testmediumtext "+
		"	mediumtext, "+
		"	testlongtext longtext, "+
		"	testblob blob, "+
		"	testtinyblob tinyblob, "+
		"	testmediumblob "+
		"	mediumblob, "+
		"	testlongblob longblob, "+
		"	testtimestamp timestamp)"));
	console.log("");


	// insert
	console.log("INSERT: ");
	assertTrue(con.begin());
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	1, "+
		"	1, "+
		"	1, "+
		"	1, "+
		"	1, "+
		"	1.5, "+
		"	1.5, "+
		"	1.5, "+
		"	'2001-01-01', "+
		"	'01:00:00', "+
		"	'2001-01-01 01:00:00', "+
		"	'2001', "+
		"	'char1', "+
		"	'varchar1', "+
		"	'text1', "+
		"	'tinytext1', "+
		"	'mediumtext1', "+
		"	'longtext1', "+
		"	'blob1', "+
		"	'tinyblob1', "+
		"	'mediumblob1', "+
		"	'longblob1', "+
		"	NULL)"));
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	2, "+
		"	2, "+
		"	2, "+
		"	2, "+
		"	2, "+
		"	2.5, "+
		"	2.5, "+
		"	2.5, "+
		"	'2002-01-01', "+
		"	'02:00:00', "+
		"	'2002-01-01 02:00:00', "+
		"	'2002', "+
		"	'char2', "+
		"	'varchar2', "+
		"	'text2', "+
		"	'tinytext2', "+
		"	'mediumtext2', "+
		"	'longtext2', "+
		"	'blob2', "+
		"	'tinyblob2', "+
		"	'mediumblob2', "+
		"	'longblob2', "+
		"	NULL)"));
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	3, "+
		"	3, "+
		"	3, "+
		"	3, "+
		"	3, "+
		"	3.5, "+
		"	3.5, "+
		"	3.5, "+
		"	'2003-01-01', "+
		"	'03:00:00', "+
		"	'2003-01-01 03:00:00', "+
		"	'2003', "+
		"	'char3', "+
		"	'varchar3', "+
		"	'text3', "+
		"	'tinytext3', "+
		"	'mediumtext3', "+
		"	'longtext3', "+
		"	'blob3', "+
		"	'tinyblob3', "+
		"	'mediumblob3', "+
		"	'longblob3', "+
		"	NULL)"));
	assertTrue(cur.sendQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	4, "+
		"	4, "+
		"	4, "+
		"	4, "+
		"	4, "+
		"	4.5, "+
		"	4.5, "+
		"	4.5, "+
		"	'2004-01-01', "+
		"	'04:00:00', "+
		"	'2004-01-01 04:00:00', "+
		"	'2004', "+
		"	'char4', "+
		"	'varchar4', "+
		"	'text4', "+
		"	'tinytext4', "+
		"	'mediumtext4', "+
		"	'longtext4', "+
		"	'blob4', "+
		"	'tinyblob4', "+
		"	'mediumblob4', "+
		"	'longblob4', "+
		"	NULL)"));
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
		"	NULL)");
	assertEqInt(cur.countBindVariables(),22);
	cur.inputBind("1",5);
	cur.inputBind("2",5);
	cur.inputBind("3",5);
	cur.inputBind("4",5);
	cur.inputBind("5",5);
	cur.inputBind("6",5.5,2,1);
	cur.inputBind("7",5.5,2,1);
	cur.inputBind("8",5.5,2,1);
	cur.inputBind("9","2005-01-01");
	cur.inputBind("10","05:00:00");
	cur.inputBind("11",2005,1,1,5,0,0,0,null,0);
	cur.inputBind("12","2005");
	cur.inputBind("13","char5");
	cur.inputBind("14","varchar5");
	cur.inputBindClob("15","text5","text5".length);
	cur.inputBindClob("16","tinytext5","tinytext5".length);
	cur.inputBindClob("17","mediumtext5","mediumtext5".length);
	cur.inputBindClob("18","longtext5","longtext5".length);
	cur.inputBindBlob("19","blob5","blob5".length);
	cur.inputBindBlob("20","tinyblob5","tinyblob5".length);
	cur.inputBindBlob("21","mediumblob5","mediumblob5".length);
	cur.inputBindBlob("22","longblob5","longblob5".length);
	assertTrue(cur.executeQuery());
	cur.clearBinds();
	cur.inputBind("1",6);
	cur.inputBind("2",6);
	cur.inputBind("3",6);
	cur.inputBind("4",6);
	cur.inputBind("5",6);
	cur.inputBind("6",6.5,2,1);
	cur.inputBind("7",6.5,2,1);
	cur.inputBind("8",6.5,2,1);
	cur.inputBind("9","2006-01-01");
	cur.inputBind("10","06:00:00");
	cur.inputBind("11",2006,1,1,6,0,0,0,null,0);
	cur.inputBind("12","2006");
	cur.inputBind("13","char6");
	cur.inputBind("14","varchar6");
	cur.inputBindClob("15","text6","text6".length);
	cur.inputBindClob("16","tinytext6","tinytext6".length);
	cur.inputBindClob("17","mediumtext6","mediumtext6".length);
	cur.inputBindClob("18","longtext6","longtext6".length);
	cur.inputBindBlob("19","blob6","blob6".length);
	cur.inputBindBlob("20","tinyblob6","tinyblob6".length);
	cur.inputBindBlob("21","mediumblob6","mediumblob6".length);
	cur.inputBindBlob("22","longblob6","longblob6".length);
	assertTrue(cur.executeQuery());
	cur.clearBinds();
	cur.inputBind("1",7);
	cur.inputBind("2",7);
	cur.inputBind("3",7);
	cur.inputBind("4",7);
	cur.inputBind("5",7);
	cur.inputBind("6",7.5,2,1);
	cur.inputBind("7",7.5,2,1);
	cur.inputBind("8",7.5,2,1);
	cur.inputBind("9","2007-01-01");
	cur.inputBind("10","07:00:00");
	cur.inputBind("11",2007,1,1,7,0,0,0,null,0);
	cur.inputBind("12","2007");
	cur.inputBind("13","char7");
	cur.inputBind("14","varchar7");
	cur.inputBindClob("15","text7","text7".length);
	cur.inputBindClob("16","tinytext7","tinytext7".length);
	cur.inputBindClob("17","mediumtext7","mediumtext7".length);
	cur.inputBindClob("18","longtext7","longtext7".length);
	cur.inputBindBlob("19","blob7","blob7".length);
	cur.inputBindBlob("20","tinyblob7","tinyblob7".length);
	cur.inputBindBlob("21","mediumblob7","mediumblob7".length);
	cur.inputBindBlob("22","longblob7","longblob7".length);
	assertTrue(cur.executeQuery());
	console.log("");


	// array of input binds by position
	// mysql doesn't support implicit
	// conversion of string binds to other
	// data types, so arrays of binds don't
	// generally work.


	// input bind by position with
	// validation
	console.log("BIND BY POSITION WITH VALIDATION: ");
	cur.clearBinds();
	cur.inputBind("1",8);
	cur.inputBind("2",8);
	cur.inputBind("3",8);
	cur.inputBind("4",8);
	cur.inputBind("5",8);
	cur.inputBind("6",8.5,2,1);
	cur.inputBind("7",8.5,2,1);
	cur.inputBind("8",8.5,2,1);
	cur.inputBind("9","2008-01-01");
	cur.inputBind("10","08:00:00");
	cur.inputBind("11",2008,1,1,8,0,0,0,null,0);
	cur.inputBind("12","2008");
	cur.inputBind("13","char8");
	cur.inputBind("14","varchar8");
	cur.inputBindClob("15","text8","text8".length);
	cur.inputBindClob("16","tinytext8","tinytext8".length);
	cur.inputBindClob("17","mediumtext8","mediumtext8".length);
	cur.inputBindClob("18","longtext8","longtext8".length);
	cur.inputBindBlob("19","blob8","blob8".length);
	cur.inputBindBlob("20","tinyblob8","tinyblob8".length);
	cur.inputBindBlob("21","mediumblob8","mediumblob8".length);
	cur.inputBindBlob("22","longblob8","longblob8".length);
	cur.validateBinds();
	assertTrue(cur.executeQuery());
	console.log("");


	// input bind by name
	// mysql doesn't support bind by name


	// array of input binds by name
	// mysql doesn't support bind by name


	// input bind by name with validation
	// mysql doesn't support bind by name


	// select
	console.log("SELECT: ");
	assertTrue(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	console.log("");


	// column count
	console.log("COLUMN COUNT: ");
	assertEqInt(cur.colCount(),23);
	console.log("");


	// column names
	console.log("COLUMN NAMES: ");
	assertEqStr(cur.getColumnName(0),"testtinyint");
	assertEqStr(cur.getColumnName(1),"testsmallint");
	assertEqStr(cur.getColumnName(2),"testmediumint");
	assertEqStr(cur.getColumnName(3),"testint");
	assertEqStr(cur.getColumnName(4),"testbigint");
	assertEqStr(cur.getColumnName(5),"testfloat");
	assertEqStr(cur.getColumnName(6),"testreal");
	assertEqStr(cur.getColumnName(7),"testdecimal");
	assertEqStr(cur.getColumnName(8),"testdate");
	assertEqStr(cur.getColumnName(9),"testtime");
	assertEqStr(cur.getColumnName(10),"testdatetime");
	assertEqStr(cur.getColumnName(11),"testyear");
	assertEqStr(cur.getColumnName(12),"testchar");
	assertEqStr(cur.getColumnName(13),"testvarchar");
	assertEqStr(cur.getColumnName(14),"testtext");
	assertEqStr(cur.getColumnName(15),"testtinytext");
	assertEqStr(cur.getColumnName(16),"testmediumtext");
	assertEqStr(cur.getColumnName(17),"testlongtext");
	assertEqStr(cur.getColumnName(18),"testblob");
	assertEqStr(cur.getColumnName(19),"testtinyblob");
	assertEqStr(cur.getColumnName(20),"testmediumblob");
	assertEqStr(cur.getColumnName(21),"testlongblob");
	assertEqStr(cur.getColumnName(22),"testtimestamp");
	var cols=cur.getColumnNames();
	assertEqStr(cols[0],"testtinyint");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testmediumint");
	assertEqStr(cols[3],"testint");
	assertEqStr(cols[4],"testbigint");
	assertEqStr(cols[5],"testfloat");
	assertEqStr(cols[6],"testreal");
	assertEqStr(cols[7],"testdecimal");
	assertEqStr(cols[8],"testdate");
	assertEqStr(cols[9],"testtime");
	assertEqStr(cols[10],"testdatetime");
	assertEqStr(cols[11],"testyear");
	assertEqStr(cols[12],"testchar");
	assertEqStr(cols[13],"testvarchar");
	assertEqStr(cols[14],"testtext");
	assertEqStr(cols[15],"testtinytext");
	assertEqStr(cols[16],"testmediumtext");
	assertEqStr(cols[17],"testlongtext");
	assertEqStr(cols[18],"testblob");
	assertEqStr(cols[19],"testtinyblob");
	assertEqStr(cols[20],"testmediumblob");
	assertEqStr(cols[21],"testlongblob");
	assertEqStr(cols[22],"testtimestamp");
	console.log("");


	// column types
	console.log("COLUMN TYPES: ");
	assertEqStr(cur.getColumnType(0),"TINYINT");
	assertEqStr(cur.getColumnType(1),"SMALLINT");
	assertEqStr(cur.getColumnType(2),"MEDIUMINT");
	assertEqStr(cur.getColumnType(3),"INT");
	assertEqStr(cur.getColumnType(4),"BIGINT");
	assertEqStr(cur.getColumnType(5),"FLOAT");
	assertEqStr(cur.getColumnType(6),"REAL");
	assertEqStr(cur.getColumnType(7),"DECIMAL");
	assertEqStr(cur.getColumnType(8),"DATE");
	assertEqStr(cur.getColumnType(9),"TIME");
	assertEqStr(cur.getColumnType(10),"DATETIME");
	assertEqStr(cur.getColumnType(11),"YEAR");
	if (majorversion==3) {
		assertEqStr(cur.getColumnType(12),"VARSTRING");
	} else {
		assertEqStr(cur.getColumnType(12),"STRING");
	}
	assertEqStr(cur.getColumnType(13),"VARSTRING");
	assertEqStr(cur.getColumnType(14),"TEXT");
	assertEqStr(cur.getColumnType(15),"TINYTEXT");
	assertEqStr(cur.getColumnType(16),"MEDIUMTEXT");
	assertEqStr(cur.getColumnType(17),"LONGTEXT");
	assertEqStr(cur.getColumnType(18),"BLOB");
	assertEqStr(cur.getColumnType(19),"TINYBLOB");
	assertEqStr(cur.getColumnType(20),"MEDIUMBLOB");
	assertEqStr(cur.getColumnType(21),"LONGBLOB");
	assertEqStr(cur.getColumnType(22),"TIMESTAMP");
	assertEqStr(cur.getColumnType("testtinyint"),"TINYINT");
	assertEqStr(cur.getColumnType("testsmallint"),"SMALLINT");
	assertEqStr(cur.getColumnType("testmediumint"),
		"MEDIUMINT");
	assertEqStr(cur.getColumnType("testint"),"INT");
	assertEqStr(cur.getColumnType("testbigint"),"BIGINT");
	assertEqStr(cur.getColumnType("testfloat"),"FLOAT");
	assertEqStr(cur.getColumnType("testreal"),"REAL");
	assertEqStr(cur.getColumnType("testdecimal"),"DECIMAL");
	assertEqStr(cur.getColumnType("testdate"),"DATE");
	assertEqStr(cur.getColumnType("testtime"),"TIME");
	assertEqStr(cur.getColumnType("testdatetime"),"DATETIME");
	assertEqStr(cur.getColumnType("testyear"),"YEAR");
	if (majorversion==3) {
		assertEqStr(cur.getColumnType("testchar"),
		    "VARSTRING");
	} else {
		assertEqStr(cur.getColumnType("testchar"),
		    "STRING");
	}
	assertEqStr(cur.getColumnType("testvarchar"),"VARSTRING");
	assertEqStr(cur.getColumnType("testtext"),"TEXT");
	assertEqStr(cur.getColumnType("testtinytext"),"TINYTEXT");
	assertEqStr(cur.getColumnType("testmediumtext"),
		"MEDIUMTEXT");
	assertEqStr(cur.getColumnType("testlongtext"),"LONGTEXT");
	assertEqStr(cur.getColumnType("testblob"),"BLOB");
	assertEqStr(cur.getColumnType("testtinyblob"),"TINYBLOB");
	assertEqStr(cur.getColumnType("testmediumblob"),
		"MEDIUMBLOB");
	assertEqStr(cur.getColumnType("testlongblob"),"LONGBLOB");
	assertEqStr(cur.getColumnType("testtimestamp"),
		"TIMESTAMP");
	console.log("");


	// mysql before 4 reports column lengths differently (charset)
	if (majorversion>3) {
		// column length
		console.log("COLUMN LENGTH: ");
		assertEqInt(cur.getColumnLength(0),1);
		assertEqInt(cur.getColumnLength(1),2);
		assertEqInt(cur.getColumnLength(2),3);
		assertEqInt(cur.getColumnLength(3),4);
		assertEqInt(cur.getColumnLength(4),8);
		assertEqInt(cur.getColumnLength(5),4);
		assertEqInt(cur.getColumnLength(6),8);
		assertEqInt(cur.getColumnLength(7),6);
		assertEqInt(cur.getColumnLength(8),3);
		assertEqInt(cur.getColumnLength(9),3);
		assertEqInt(cur.getColumnLength(10),8);
		assertEqInt(cur.getColumnLength(11),1);
		// testchar/testvarchar are char(40)/varchar(40); the connection
		// charset is latin1 (1 byte/char) so the lengths are 40/41
		assertEqInt(cur.getColumnLength(12),40);
		assertEqInt(cur.getColumnLength(13),41);
		assertEqInt(cur.getColumnLength(14),65535);
		assertEqInt(cur.getColumnLength(15),255);
		assertEqInt(cur.getColumnLength(16),16777215);
		assertEqInt(cur.getColumnLength(17),2147483647);
		assertEqInt(cur.getColumnLength(18),65535);
		assertEqInt(cur.getColumnLength(19),255);
		assertEqInt(cur.getColumnLength(20),16777215);
		assertEqInt(cur.getColumnLength(21),2147483647);
		assertEqInt(cur.getColumnLength(22),4);
		assertEqInt(cur.getColumnLength("testtinyint"),1);
		assertEqInt(cur.getColumnLength("testsmallint"),2);
		assertEqInt(cur.getColumnLength("testmediumint"),3);
		assertEqInt(cur.getColumnLength("testint"),4);
		assertEqInt(cur.getColumnLength("testbigint"),8);
		assertEqInt(cur.getColumnLength("testfloat"),4);
		assertEqInt(cur.getColumnLength("testreal"),8);
		assertEqInt(cur.getColumnLength("testdecimal"),6);
		assertEqInt(cur.getColumnLength("testdate"),3);
		assertEqInt(cur.getColumnLength("testtime"),3);
		assertEqInt(cur.getColumnLength("testdatetime"),8);
		assertEqInt(cur.getColumnLength("testyear"),1);
		// testchar/testvarchar are char(40)/varchar(40); the connection
		// charset is latin1 (1 byte/char) so the lengths are 40/41
		assertEqInt(cur.getColumnLength("testchar"),40);
		assertEqInt(cur.getColumnLength("testvarchar"),41);
		assertEqInt(cur.getColumnLength("testtext"),65535);
		assertEqInt(cur.getColumnLength("testtinytext"),255);
		assertEqInt(cur.getColumnLength("testmediumtext"),
			16777215);
		assertEqInt(cur.getColumnLength("testlongtext"),
			2147483647);
		assertEqInt(cur.getColumnLength("testblob"),65535);
		assertEqInt(cur.getColumnLength("testtinyblob"),255);
		assertEqInt(cur.getColumnLength("testmediumblob"),
			16777215);
		assertEqInt(cur.getColumnLength("testlongblob"),
			2147483647);
		assertEqInt(cur.getColumnLength("testtimestamp"),4);
		console.log("");
	}


	// longest column
	console.log("LONGEST COLUMN: ");
	assertEqInt(cur.getLongest(0),1);
	assertEqInt(cur.getLongest(1),1);
	assertEqInt(cur.getLongest(2),1);
	assertEqInt(cur.getLongest(3),1);
	assertEqInt(cur.getLongest(4),1);
	assertEqInt(cur.getLongest(5),3);
	assertEqInt(cur.getLongest(6),3);
	assertEqInt(cur.getLongest(7),3);
	assertEqInt(cur.getLongest(8),10);
	assertEqInt(cur.getLongest(9),8);
	assertEqInt(cur.getLongest(10),19);
	assertEqInt(cur.getLongest(11),4);
	assertEqInt(cur.getLongest(12),5);
	assertEqInt(cur.getLongest(13),8);
	assertEqInt(cur.getLongest(14),5);
	assertEqInt(cur.getLongest(15),9);
	assertEqInt(cur.getLongest(16),11);
	assertEqInt(cur.getLongest(17),9);
	assertEqInt(cur.getLongest(18),5);
	assertEqInt(cur.getLongest(19),9);
	assertEqInt(cur.getLongest(20),11);
	assertEqInt(cur.getLongest(21),9);
	if (majorversion==3) {
		assertEqInt(cur.getLongest(22),14);
	} else {
		assertEqInt(cur.getLongest(22),19);
	}
	assertEqInt(cur.getLongest("testtinyint"),1);
	assertEqInt(cur.getLongest("testsmallint"),1);
	assertEqInt(cur.getLongest("testmediumint"),1);
	assertEqInt(cur.getLongest("testint"),1);
	assertEqInt(cur.getLongest("testbigint"),1);
	assertEqInt(cur.getLongest("testfloat"),3);
	assertEqInt(cur.getLongest("testreal"),3);
	assertEqInt(cur.getLongest("testdecimal"),3);
	assertEqInt(cur.getLongest("testdate"),10);
	assertEqInt(cur.getLongest("testtime"),8);
	assertEqInt(cur.getLongest("testdatetime"),19);
	assertEqInt(cur.getLongest("testyear"),4);
	assertEqInt(cur.getLongest("testchar"),5);
	assertEqInt(cur.getLongest("testvarchar"),8);
	assertEqInt(cur.getLongest("testtext"),5);
	assertEqInt(cur.getLongest("testtinytext"),9);
	assertEqInt(cur.getLongest("testmediumtext"),11);
	assertEqInt(cur.getLongest("testlongtext"),9);
	assertEqInt(cur.getLongest("testblob"),5);
	assertEqInt(cur.getLongest("testtinyblob"),9);
	assertEqInt(cur.getLongest("testmediumblob"),11);
	assertEqInt(cur.getLongest("testlongblob"),9);
	if (majorversion==3) {
		assertEqInt(cur.getLongest("testtimestamp"),14);
	} else {
		assertEqInt(cur.getLongest("testtimestamp"),19);
	}
	console.log("");


	// row count
	console.log("ROW COUNT: ");
	assertEqInt(cur.rowCount(),8);
	console.log("");


	// total rows
	console.log("TOTAL ROWS: ");
	// older versions of mysql know this
	//assertEqInt(cur.totalRows(),0);
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
	assertEqStr(cur.getField(0,1),"1");
	assertEqStr(cur.getField(0,2),"1");
	assertEqStr(cur.getField(0,3),"1");
	assertEqStr(cur.getField(0,4),"1");
	assertEqStr(cur.getField(0,5),"1.5");
	assertEqStr(cur.getField(0,6),"1.5");
	assertEqStr(cur.getField(0,7),"1.5");
	assertEqStr(cur.getField(0,8),"2001-01-01");
	assertEqStr(cur.getField(0,9),"01:00:00");
	assertEqStr(cur.getField(0,10),"2001-01-01 01:00:00");
	assertEqStr(cur.getField(0,11),"2001");
	assertEqStr(cur.getField(0,12),"char1");
	assertEqStr(cur.getField(0,13),"varchar1");
	assertEqStr(cur.getField(0,14),"text1");
	assertEqStr(cur.getField(0,15),"tinytext1");
	assertEqStr(cur.getField(0,16),"mediumtext1");
	assertEqStr(cur.getField(0,17),"longtext1");
	assertEqStr(cur.getField(0,18),"blob1");
	assertEqStr(cur.getField(0,19),"tinyblob1");
	assertEqStr(cur.getField(0,20),"mediumblob1");
	assertEqStr(cur.getField(0,21),"longblob1");
	console.log("");
	assertEqStr(cur.getField(7,0),"8");
	assertEqStr(cur.getField(7,1),"8");
	assertEqStr(cur.getField(7,2),"8");
	assertEqStr(cur.getField(7,3),"8");
	assertEqStr(cur.getField(7,4),"8");
	assertEqStr(cur.getField(7,5),"8.5");
	assertEqStr(cur.getField(7,6),"8.5");
	assertEqStr(cur.getField(7,7),"8.5");
	assertEqStr(cur.getField(7,8),"2008-01-01");
	assertEqStr(cur.getField(7,9),"08:00:00");
	assertEqStr(cur.getField(7,10),"2008-01-01 08:00:00");
	assertEqStr(cur.getField(7,11),"2008");
	assertEqStr(cur.getField(7,12),"char8");
	assertEqStr(cur.getField(7,13),"varchar8");
	assertEqStr(cur.getField(7,14),"text8");
	assertEqStr(cur.getField(7,15),"tinytext8");
	assertEqStr(cur.getField(7,16),"mediumtext8");
	assertEqStr(cur.getField(7,17),"longtext8");
	assertEqStr(cur.getField(7,18),"blob8");
	assertEqStr(cur.getField(7,19),"tinyblob8");
	assertEqStr(cur.getField(7,20),"mediumblob8");
	assertEqStr(cur.getField(7,21),"longblob8");
	console.log("");


	// field lengths by index
	console.log("FIELD LENGTHS BY INDEX: ");
	assertEqInt(cur.getFieldLength(0,0),1);
	assertEqInt(cur.getFieldLength(0,1),1);
	assertEqInt(cur.getFieldLength(0,2),1);
	assertEqInt(cur.getFieldLength(0,3),1);
	assertEqInt(cur.getFieldLength(0,4),1);
	assertEqInt(cur.getFieldLength(0,5),3);
	assertEqInt(cur.getFieldLength(0,6),3);
	assertEqInt(cur.getFieldLength(0,7),3);
	assertEqInt(cur.getFieldLength(0,8),10);
	assertEqInt(cur.getFieldLength(0,9),8);
	assertEqInt(cur.getFieldLength(0,10),19);
	assertEqInt(cur.getFieldLength(0,11),4);
	assertEqInt(cur.getFieldLength(0,12),5);
	assertEqInt(cur.getFieldLength(0,13),8);
	assertEqInt(cur.getFieldLength(0,14),5);
	assertEqInt(cur.getFieldLength(0,15),9);
	assertEqInt(cur.getFieldLength(0,16),11);
	assertEqInt(cur.getFieldLength(0,17),9);
	assertEqInt(cur.getFieldLength(0,18),5);
	assertEqInt(cur.getFieldLength(0,19),9);
	assertEqInt(cur.getFieldLength(0,20),11);
	assertEqInt(cur.getFieldLength(0,21),9);
	console.log("");
	assertEqInt(cur.getFieldLength(7,0),1);
	assertEqInt(cur.getFieldLength(7,1),1);
	assertEqInt(cur.getFieldLength(7,2),1);
	assertEqInt(cur.getFieldLength(7,3),1);
	assertEqInt(cur.getFieldLength(7,4),1);
	assertEqInt(cur.getFieldLength(7,5),3);
	assertEqInt(cur.getFieldLength(7,6),3);
	assertEqInt(cur.getFieldLength(7,7),3);
	assertEqInt(cur.getFieldLength(7,8),10);
	assertEqInt(cur.getFieldLength(7,9),8);
	assertEqInt(cur.getFieldLength(7,10),19);
	assertEqInt(cur.getFieldLength(7,11),4);
	assertEqInt(cur.getFieldLength(7,12),5);
	assertEqInt(cur.getFieldLength(7,13),8);
	assertEqInt(cur.getFieldLength(7,14),5);
	assertEqInt(cur.getFieldLength(7,15),9);
	assertEqInt(cur.getFieldLength(7,16),11);
	assertEqInt(cur.getFieldLength(7,17),9);
	assertEqInt(cur.getFieldLength(7,18),5);
	assertEqInt(cur.getFieldLength(7,19),9);
	assertEqInt(cur.getFieldLength(7,20),11);
	assertEqInt(cur.getFieldLength(7,21),9);
	console.log("");


	// fields by name
	console.log("FIELDS BY NAME: ");
	assertEqStr(cur.getField(0,"testtinyint"),"1");
	assertEqStr(cur.getField(0,"testsmallint"),"1");
	assertEqStr(cur.getField(0,"testmediumint"),"1");
	assertEqStr(cur.getField(0,"testint"),"1");
	assertEqStr(cur.getField(0,"testbigint"),"1");
	assertEqStr(cur.getField(0,"testfloat"),"1.5");
	assertEqStr(cur.getField(0,"testreal"),"1.5");
	assertEqStr(cur.getField(0,"testdecimal"),"1.5");
	assertEqStr(cur.getField(0,"testdate"),"2001-01-01");
	assertEqStr(cur.getField(0,"testtime"),"01:00:00");
	assertEqStr(cur.getField(0,"testdatetime"),
		"2001-01-01 01:00:00");
	assertEqStr(cur.getField(0,"testyear"),"2001");
	assertEqStr(cur.getField(0,"testchar"),"char1");
	assertEqStr(cur.getField(0,"testvarchar"),"varchar1");
	assertEqStr(cur.getField(0,"testtext"),"text1");
	assertEqStr(cur.getField(0,"testtinytext"),"tinytext1");
	assertEqStr(cur.getField(0,"testmediumtext"),
		"mediumtext1");
	assertEqStr(cur.getField(0,"testlongtext"),"longtext1");
	assertEqStr(cur.getField(0,"testblob"),"blob1");
	assertEqStr(cur.getField(0,"testlongblob"),"longblob1");
	assertEqStr(cur.getField(0,"testtinyblob"),"tinyblob1");
	assertEqStr(cur.getField(0,"testmediumblob"),
		"mediumblob1");
	console.log("");
	assertEqStr(cur.getField(7,"testtinyint"),"8");
	assertEqStr(cur.getField(7,"testsmallint"),"8");
	assertEqStr(cur.getField(7,"testmediumint"),"8");
	assertEqStr(cur.getField(7,"testint"),"8");
	assertEqStr(cur.getField(7,"testbigint"),"8");
	assertEqStr(cur.getField(7,"testfloat"),"8.5");
	assertEqStr(cur.getField(7,"testreal"),"8.5");
	assertEqStr(cur.getField(7,"testdecimal"),"8.5");
	assertEqStr(cur.getField(7,"testdate"),"2008-01-01");
	assertEqStr(cur.getField(7,"testtime"),"08:00:00");
	assertEqStr(cur.getField(7,"testdatetime"),
		"2008-01-01 08:00:00");
	assertEqStr(cur.getField(7,"testyear"),"2008");
	assertEqStr(cur.getField(7,"testchar"),"char8");
	assertEqStr(cur.getField(7,"testvarchar"),"varchar8");
	assertEqStr(cur.getField(7,"testtext"),"text8");
	assertEqStr(cur.getField(7,"testtinytext"),"tinytext8");
	assertEqStr(cur.getField(7,"testmediumtext"),
		"mediumtext8");
	assertEqStr(cur.getField(7,"testlongtext"),"longtext8");
	assertEqStr(cur.getField(7,"testblob"),"blob8");
	assertEqStr(cur.getField(7,"testlongblob"),"longblob8");
	assertEqStr(cur.getField(7,"testtinyblob"),"tinyblob8");
	assertEqStr(cur.getField(7,"testmediumblob"),
		"mediumblob8");
	console.log("");


	// field lengths by name
	console.log("FIELD LENGTHS BY NAME: ");
	assertEqInt(cur.getFieldLength(0,"testtinyint"),1);
	assertEqInt(cur.getFieldLength(0,"testsmallint"),1);
	assertEqInt(cur.getFieldLength(0,"testmediumint"),1);
	assertEqInt(cur.getFieldLength(0,"testint"),1);
	assertEqInt(cur.getFieldLength(0,"testbigint"),1);
	assertEqInt(cur.getFieldLength(0,"testfloat"),3);
	assertEqInt(cur.getFieldLength(0,"testreal"),3);
	assertEqInt(cur.getFieldLength(0,"testdecimal"),3);
	assertEqInt(cur.getFieldLength(0,"testdate"),10);
	assertEqInt(cur.getFieldLength(0,"testtime"),8);
	assertEqInt(cur.getFieldLength(0,"testdatetime"),19);
	assertEqInt(cur.getFieldLength(0,"testyear"),4);
	assertEqInt(cur.getFieldLength(0,"testchar"),5);
	assertEqInt(cur.getFieldLength(0,"testvarchar"),8);
	assertEqInt(cur.getFieldLength(0,"testtext"),5);
	assertEqInt(cur.getFieldLength(0,"testtinytext"),9);
	assertEqInt(cur.getFieldLength(0,"testmediumtext"),11);
	assertEqInt(cur.getFieldLength(0,"testlongtext"),9);
	assertEqInt(cur.getFieldLength(0,"testblob"),5);
	assertEqInt(cur.getFieldLength(0,"testtinyblob"),9);
	assertEqInt(cur.getFieldLength(0,"testmediumblob"),11);
	assertEqInt(cur.getFieldLength(0,"testlongblob"),9);
	console.log("");
	assertEqInt(cur.getFieldLength(7,"testtinyint"),1);
	assertEqInt(cur.getFieldLength(7,"testsmallint"),1);
	assertEqInt(cur.getFieldLength(7,"testmediumint"),1);
	assertEqInt(cur.getFieldLength(7,"testint"),1);
	assertEqInt(cur.getFieldLength(7,"testbigint"),1);
	assertEqInt(cur.getFieldLength(7,"testfloat"),3);
	assertEqInt(cur.getFieldLength(7,"testreal"),3);
	assertEqInt(cur.getFieldLength(7,"testdecimal"),3);
	assertEqInt(cur.getFieldLength(7,"testdate"),10);
	assertEqInt(cur.getFieldLength(7,"testtime"),8);
	assertEqInt(cur.getFieldLength(7,"testdatetime"),19);
	assertEqInt(cur.getFieldLength(7,"testyear"),4);
	assertEqInt(cur.getFieldLength(7,"testchar"),5);
	assertEqInt(cur.getFieldLength(7,"testvarchar"),8);
	assertEqInt(cur.getFieldLength(7,"testtext"),5);
	assertEqInt(cur.getFieldLength(7,"testtinytext"),9);
	assertEqInt(cur.getFieldLength(7,"testmediumtext"),11);
	assertEqInt(cur.getFieldLength(7,"testlongtext"),9);
	assertEqInt(cur.getFieldLength(7,"testblob"),5);
	assertEqInt(cur.getFieldLength(7,"testtinyblob"),9);
	assertEqInt(cur.getFieldLength(7,"testmediumblob"),11);
	assertEqInt(cur.getFieldLength(7,"testlongblob"),9);
	console.log("");


	// fields by array
	console.log("FIELDS BY ARRAY: ");
	var fields=cur.getRow(0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1");
	assertEqStr(fields[2],"1");
	assertEqStr(fields[3],"1");
	assertEqStr(fields[4],"1");
	assertEqStr(fields[5],"1.5");
	assertEqStr(fields[6],"1.5");
	assertEqStr(fields[7],"1.5");
	assertEqStr(fields[8],"2001-01-01");
	assertEqStr(fields[9],"01:00:00");
	assertEqStr(fields[10],"2001-01-01 01:00:00");
	assertEqStr(fields[11],"2001");
	assertEqStr(fields[12],"char1");
	assertEqStr(fields[13],"varchar1");
	assertEqStr(fields[14],"text1");
	assertEqStr(fields[15],"tinytext1");
	assertEqStr(fields[16],"mediumtext1");
	assertEqStr(fields[17],"longtext1");
	assertEqStr(fields[18],"blob1");
	assertEqStr(fields[19],"tinyblob1");
	assertEqStr(fields[20],"mediumblob1");
	assertEqStr(fields[21],"longblob1");
	console.log("");


	// field lengths by array
	console.log("FIELD LENGTHS BY ARRAY: ");
	var fieldlens=cur.getRowLengths(0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],1);
	assertEqInt(fieldlens[2],1);
	assertEqInt(fieldlens[3],1);
	assertEqInt(fieldlens[4],1);
	assertEqInt(fieldlens[5],3);
	assertEqInt(fieldlens[6],3);
	assertEqInt(fieldlens[7],3);
	assertEqInt(fieldlens[8],10);
	assertEqInt(fieldlens[9],8);
	assertEqInt(fieldlens[10],19);
	assertEqInt(fieldlens[11],4);
	assertEqInt(fieldlens[12],5);
	assertEqInt(fieldlens[13],8);
	assertEqInt(fieldlens[14],5);
	assertEqInt(fieldlens[15],9);
	assertEqInt(fieldlens[16],11);
	assertEqInt(fieldlens[17],9);
	assertEqInt(fieldlens[18],5);
	assertEqInt(fieldlens[19],9);
	assertEqInt(fieldlens[20],11);
	assertEqInt(fieldlens[21],9);
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
		"	testtinyint "));
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
	assertTrue(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	assertEqStr(cur.getColumnName(0),null);
	assertEqInt(cur.getColumnLength(0),0);
	assertEqStr(cur.getColumnType(0),null);
	console.log("");
	cur.getColumnInfo();
	assertTrue(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	assertEqStr(cur.getColumnName(0),"testtinyint");
	assertEqInt(cur.getColumnLength(0),1);
	assertEqStr(cur.getColumnType(0),"TINYINT");
	console.log("");


	// suspended session
	console.log("SUSPENDED SESSION: ");
	assertTrue(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
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
		"	testtinyint "));
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
		"	testtinyint "));
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
		"	testtinyint "));
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
	cur.cacheToFile("cachefile1-mysql");
	cur.setCacheTtl(200);
	assertTrue(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	var filename=cur.getCacheFileName();
	assertEqStr(filename,"cachefile1-mysql");
	cur.cacheOff();
	assertTrue(cur.openCachedResultSet(filename));
	assertEqStr(cur.getField(7,0),"8");
	console.log("");


	// column count for cached result set
	console.log("COLUMN COUNT FOR CACHED RESULT SET: ");
	assertEqInt(cur.colCount(),23);
	console.log("");


	// column names for cached result set
	console.log("COLUMN NAMES FOR CACHED RESULT SET: ");
	assertEqStr(cur.getColumnName(0),"testtinyint");
	assertEqStr(cur.getColumnName(1),"testsmallint");
	assertEqStr(cur.getColumnName(2),"testmediumint");
	assertEqStr(cur.getColumnName(3),"testint");
	assertEqStr(cur.getColumnName(4),"testbigint");
	assertEqStr(cur.getColumnName(5),"testfloat");
	assertEqStr(cur.getColumnName(6),"testreal");
	assertEqStr(cur.getColumnName(7),"testdecimal");
	assertEqStr(cur.getColumnName(8),"testdate");
	assertEqStr(cur.getColumnName(9),"testtime");
	assertEqStr(cur.getColumnName(10),"testdatetime");
	assertEqStr(cur.getColumnName(11),"testyear");
	assertEqStr(cur.getColumnName(12),"testchar");
	assertEqStr(cur.getColumnName(13),"testvarchar");
	assertEqStr(cur.getColumnName(14),"testtext");
	assertEqStr(cur.getColumnName(15),"testtinytext");
	assertEqStr(cur.getColumnName(16),"testmediumtext");
	assertEqStr(cur.getColumnName(17),"testlongtext");
	assertEqStr(cur.getColumnName(18),"testblob");
	assertEqStr(cur.getColumnName(19),"testtinyblob");
	assertEqStr(cur.getColumnName(20),"testmediumblob");
	assertEqStr(cur.getColumnName(21),"testlongblob");
	cols=cur.getColumnNames();
	assertEqStr(cols[0],"testtinyint");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testmediumint");
	assertEqStr(cols[3],"testint");
	assertEqStr(cols[4],"testbigint");
	assertEqStr(cols[5],"testfloat");
	assertEqStr(cols[6],"testreal");
	assertEqStr(cols[7],"testdecimal");
	assertEqStr(cols[8],"testdate");
	assertEqStr(cols[9],"testtime");
	assertEqStr(cols[10],"testdatetime");
	assertEqStr(cols[11],"testyear");
	assertEqStr(cols[12],"testchar");
	assertEqStr(cols[13],"testvarchar");
	assertEqStr(cols[14],"testtext");
	assertEqStr(cols[15],"testtinytext");
	assertEqStr(cols[16],"testmediumtext");
	assertEqStr(cols[17],"testlongtext");
	assertEqStr(cols[18],"testblob");
	assertEqStr(cols[19],"testtinyblob");
	assertEqStr(cols[20],"testmediumblob");
	assertEqStr(cols[21],"testlongblob");
	console.log("");


	// cached result set with result set
	// buffer size
	console.log("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
	cur.setResultSetBufferSize(2);
	cur.cacheToFile("cachefile1-mysql");
	cur.setCacheTtl(200);
	assertTrue(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	filename=cur.getCacheFileName();
	assertEqStr(filename,"cachefile1-mysql");
	cur.cacheOff();
	assertTrue(cur.openCachedResultSet(filename));
	assertEqStr(cur.getField(7,0),"8");
	assertEqStr(cur.getField(8,0),null);
	cur.setResultSetBufferSize(0);
	console.log("");


	// from one cache file to another
	console.log("FROM ONE CACHE FILE TO ANOTHER: ");
	cur.cacheToFile("cachefile2-mysql");
	assertTrue(cur.openCachedResultSet("cachefile1-mysql"));
	cur.cacheOff();
	assertTrue(cur.openCachedResultSet("cachefile2-mysql"));
	assertEqStr(cur.getField(7,0),"8");
	assertEqStr(cur.getField(8,0),null);
	console.log("");


	// from one cache file to another with
	// result set buffer size
	console.log("FROM ONE CACHE FILE TO ANOTHER WITH RESULT "+
		"SET BUFFER SIZE: ");
	cur.setResultSetBufferSize(2);
	cur.cacheToFile("cachefile2-mysql");
	assertTrue(cur.openCachedResultSet("cachefile1-mysql"));
	cur.cacheOff();
	assertTrue(cur.openCachedResultSet("cachefile2-mysql"));
	assertEqStr(cur.getField(7,0),"8");
	assertEqStr(cur.getField(8,0),null);
	cur.setResultSetBufferSize(0);
	console.log("");


	// cached result set with suspend and
	// result set buffer size
	console.log("CACHED RESULT SET WITH SUSPEND AND RESULT SET "+
		"BUFFER SIZE: ");
	cur.setResultSetBufferSize(2);
	cur.cacheToFile("cachefile1-mysql");
	cur.setCacheTtl(200);
	assertTrue(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	assertEqStr(cur.getField(2,0),"3");
	filename=cur.getCacheFileName();
	assertEqStr(filename,"cachefile1-mysql");
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
	assertTrue(cur.sendQuery("select * from testtable "+
		"order by testint"));
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
	// can't do this with mysql
	//cur.setResultSetBufferSize(1);
	assertTrue(cur.sendQuery("select * from testtable"));
	var secondcur=new sqlrelay.SQLRCursor(con);
	secondcur.setResultSetBufferSize(1);
	for (var i=0;cur.getRow(i);i++) {
		assertTrue(secondcur.sendQuery("select * from "+
				"testtable"));
	}
	// the nested selects must not disturb the outer result set
	assertEqInt(i,cur.rowCount());
	secondcur.closeResultSet();
	//cur.setResultSetBufferSize(0);
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// transaction behavior differs on mysql before 4
	if (majorversion>3) {
		// reset transaction state
		console.log("RESET TRANSACTION STATE: ");
		assertTrue(con.commit());
		assertEqStr(con.getTransactionModel(),"explicit-deferred");
		assertTrue(con.getAutoCommit());
		console.log("");


		// transaction behavior - implicit
		console.log("TRANSACTION BEHAVIOR - implicit: ");
		assertTrue(con.setTransactionModel("implicit"));
		assertEqStr(con.getTransactionModel(),"implicit");
		assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
		var secondcon=new sqlrelay.SQLRConnection("sqlrelay",9002,"/tmp/mysql.socket",
			"testuser","testpassword",0,1);
		var secondcur=new sqlrelay.SQLRCursor(secondcon);
		setSecondConnection(secondcon);
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
	}


	// reset transaction behavior
	// (mysql before 4 has limited transaction support)
	console.log("RESET TRANSACTION BEHAVIOR: ");
	if (majorversion>3) {
		assertTrue(con.setTransactionModel(
				con.getDefaultTransactionModel()));
		assertEqStr(con.getTransactionModel(),"explicit-deferred");
		assertTrue(con.getAutoCommit());
	}
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
	var precs=[0,0,0];
	var scales=[0,0,0];
	cur.substitutions(subvars,subvallongs,precs,scales);
	assertTrue(cur.executeQuery());
	assertEqStr(cur.getField(0,0),"1");
	assertEqStr(cur.getField(0,1),"2");
	assertEqStr(cur.getField(0,2),"3");
	console.log("");
	cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
	var subvalstrings=["hi","hello","bye"];
	cur.substitutions(subvars,subvalstrings,precs,scales);
	assertTrue(cur.executeQuery());
	assertEqStr(cur.getField(0,0),"hi");
	assertEqStr(cur.getField(0,1),"hello");
	assertEqStr(cur.getField(0,2),"bye");
	console.log("");
	cur.prepareQuery("select $(var1),$(var2),$(var3)");
	var subvaldoubles=[10.55,10.556,10.5556];
	precs=[4,5,6];
	scales=[2,3,4];
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
		"	testclob1 longtext, "+
		"	testclob2 longtext, "+
		"	testblob1 longblob, "+
		"	testblob2 longblob)"));
	cur.prepareQuery(
		"insert into "+
		"	testtable "+
		"values ("+
		"	?, "+
		"	?, "+
		"	?, "+
		"	?)");
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
		"	testtext longtext, "+
		"	testblob longblob)");
	cur.prepareQuery("insert into testtable values (?,?)");
	var largebuffer="C".repeat(8192);
	cur.inputBindClob("1",largebuffer,largebuffer.length);
	cur.inputBindBlob("2",largebuffer,largebuffer.length);
	assertTrue(cur.executeQuery());
	cur.sendQuery("select * from testtable");
	assertEqInt(cur.getFieldLength(0,"testtext"),
		8192);
	assertEqStr(cur.getField(0,"testtext"),largebuffer);
	assertEqInt(cur.getFieldLength(0,"testblob"),
		8192);
	assertEqStr(cur.getField(0,"testblob"),largebuffer);
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// output bind by position
	// mysql doesn't support output binds

	// output bind by name
	// mysql doesn't support bind by name


	// output bind by name with validation
	// mysql doesn't support bind by name


	// lob output bind
	// mysql doesn't support output binds


	// long output bind
	// mysql doesn't support output binds


	// negative input bind
	console.log("NEGATIVE INPUT BIND: ");
	cur.sendQuery("drop table testtable");
	cur.sendQuery("create table testtable (testval int)");
	cur.prepareQuery("insert into testtable values (?)");
	cur.inputBind("1",-1);
	assertTrue(cur.executeQuery());
	cur.sendQuery("select testval from testtable");
	assertEqStr(cur.getField(0,"testval"),"-1");
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// bind validation
	// mysql doesn't support bind by name


	// mysql before 5.0 has no stored procedures
	if (majorversion>3) {
		// rebinding
		console.log("REBINDING: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 int) "+
			"begin "+
			"	select in1; end"));
		cur.prepareQuery("call testproc(?)");
		cur.inputBind("1",1);
		assertTrue(cur.executeQuery());
		assertEqStr(cur.getField(0,0),"1");
		cur.inputBind("1",2);
		assertTrue(cur.executeQuery());
		assertEqStr(cur.getField(0,0),"2");
		cur.inputBind("1",3);
		assertTrue(cur.executeQuery());
		assertEqStr(cur.getField(0,0),"3");
		assertTrue(cur.sendQuery("drop procedure testproc"));
		console.log("");
	}


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
	cur.prepareQuery("select ?");
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


	// mysql before 5.0 has no stored procedures
	if (majorversion>3) {
		// stored procedure returning no value
		console.log("STORED PROCEDURE RETURNING NO VALUE: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20)) begin end"));
		cur.prepareQuery("call testproc(?,?,?)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.5,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("drop procedure testproc"));
		console.log("");


		// stored procedure returning single
		// value
		console.log("STORED PROCEDURE RETURNING SINGLE VALUE: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20)) "+
			"begin "+
			"	select in1; end"));
		cur.prepareQuery("call testproc(?,?,?)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.5,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEqStr(cur.getField(0,0),"1");
		assertTrue(cur.sendQuery("drop procedure testproc"));
		console.log("");


		// stored procedure returning multiple
		// values
		console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20)) "+
			"begin "+
			"	select in1, in2, "+
			"	in3; end"));
		cur.prepareQuery("call testproc(?,?,?)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.5,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEqStr(cur.getField(0,0),"1");
		assertEqStr(cur.getField(0,1),"1.5");
		assertEqStr(cur.getField(0,2),"hello");
		assertTrue(cur.sendQuery("drop procedure testproc"));
		console.log("");


		// stored procedure returning result
		// set
		console.log("STORED PROCEDURE RETURNING RESULT SET: ");
		cur.sendQuery("drop procedure testselectproc");
		assertTrue(cur.sendQuery("create procedure testselectproc() "+
			"begin "+
			"	select 1 "+
			"	union "+
			"	select 2 "+
			"	union "+
			"	select 3 "+
			"	union "+
			"	select 4 "+
			"	union "+
			"	select 5 "+
			"	union "+
			"	select 6 "+
			"	union "+
			"	select 7 "+
			"	union "+
			"	select 8; end"));
		assertTrue(cur.sendQuery("call testselectproc()"));
		assertEqInt(cur.rowCount(),8);
		assertTrue(cur.sendQuery("drop procedure testselectproc"));
		console.log("");


		// temporary tables
		console.log("TEMPORARY TABLES: ");
		cur.sendQuery("drop table temptable");
		cur.sendQuery("create temporary table temptable (col1 int)");
		assertTrue(cur.sendQuery("insert into temptable values (1)"));
		assertTrue(cur.sendQuery("select count(*) from temptable"));
		assertEqStr(cur.getField(0,0),"1");
		con.endSession();
		console.log("");
		assertFalse(cur.sendQuery("select count(*) from temptable"));
		console.log("");
	}

	if (majorversion>3) {

		// stored procedure returning
		// no value
		console.log("STORED PROCEDURE RETURNING NO VALUE: ");
		cur.sendQuery("drop procedure if exists testproc");
		assertTrue(cur.sendQuery("create procedure "+
			"testproc("+
			"	in in1 int, "+
			"	in in2 float, "+
			"	in in3 "+
			"	char(20)) "+
			"begin "+
			"	select in1, "+
			"	in2, in3; end;"));
		cur.prepareQuery("call testproc(?,?,?)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.5,4,2);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEqStr(cur.getField(0,0),"1");
		assertEqStr(cur.getField(0,1),"1.5");
		assertEqStr(cur.getField(0,2),"hello");
		cur.sendQuery("drop procedure testproc");
		console.log("");


		// stored procedure returning
		// one value
		console.log("FUNCTIONS: ");
		cur.sendQuery("drop function if exists testfunc");
		assertTrue(cur.sendQuery("create function testfunc("+
			"in1 int, in2 "+
			"	int) returns int return in1+in2;"));
		cur.prepareQuery("select testfunc(?,?)");
		cur.inputBind("1",10);
		cur.inputBind("2",20);
		assertTrue(cur.executeQuery());
		assertEqStr(cur.getField(0,0),"30");
		cur.sendQuery("drop function if exists testfunc");
		console.log("");


		// stored procedure returning
		// multiple values
		console.log("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
		cur.sendQuery("drop procedure if exists testproc");
		assertTrue(cur.sendQuery("create procedure "+
			"testproc("+
			"	out out1 int, "+
			"	out out2 float,"+
			"	out out3 "+
			"	char(20)) "+
			"begin "+
			"	select 1, 2.5,"+
			"	'hello' "+
			"	into out1, "+
			"	out2, out3; end;"));
		assertTrue(cur.sendQuery("set @out1=0, @out2=0.0, "+
			"@out3=''"));
		assertTrue(cur.sendQuery("call testproc(@out1,@out2,"+
			"@out3)"));
		assertTrue(cur.sendQuery("select @out1, "+
			"@out2, @out3"));
		assertEqStr(cur.getField(0,0),"1");
		assertEqDbl(cur.getFieldAsDouble(0,1),2.5);
		assertEqStr(cur.getField(0,2),"hello");
		cur.sendQuery("drop procedure testproc");
		console.log("");


		// stored procedure returning
		// result set
		console.log("STORED PROCEDURE RETURNING RESULT SET: ");
		cur.sendQuery("drop procedure if exists "+
			"testselectproc");
		assertTrue(cur.sendQuery("create procedure "+
			"testselectproc() "+
			"begin "+
			"	select 1 "+
			"	union "+
			"	select 2 "+
			"	union "+
			"	select 3 "+
			"	union "+
			"	select 4 "+
			"	union "+
			"	select 5 "+
			"	union "+
			"	select 6 "+
			"	union "+
			"	select 7 "+
			"	union "+
			"	select 8; end"));
		assertTrue(cur.sendQuery("call testselectproc()"));
		assertEqInt(cur.rowCount(),8);
		cur.sendQuery("drop procedure testselectproc");
		console.log("");
	}


	if (majorversion>3) {

		// encoded binary data -
		// all chars - \-escaped
		console.log("ENCODED BINARY DATA - all chars - \\-escaped: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery("create table testtable "+
			"(col1 longblob)"));
		var buffer=Buffer.alloc(256);
		for (var i=0;i<256;i++) {
			buffer[i]=i;
		}
		// Build the query as a Buffer so that raw bytes (including NUL
		// and bytes 128-255) pass through unmolested. The Node.js
		// sqlrelay binding's sendQuery accepts Buffer for binary-safe
		// transport; a plain JS string would be UTF-8 encoded by V8.
		var prefix=Buffer.from("insert into testtable values (_binary'");
		var suffix=Buffer.from("')");
		var chunks=[prefix];
		for (var i=0;i<buffer.length;i++) {
			var b=buffer[i];
			if (b==0x27 || b==0x5c) {
				chunks.push(Buffer.from([0x5c]));
			}
			chunks.push(buffer.slice(i,i+1));
		}
		chunks.push(suffix);
		var query=Buffer.concat(chunks);
		assertTrue(cur.sendQuery(query,query.length));
		// Verify round-tripped bytes via server-side HEX (the binding's
		// getField returns strings via String::NewFromUtf8, which drops
		// invalid UTF-8 byte sequences, so direct byte compare won't
		// work for raw bytes 128-255).
		assertTrue(cur.sendQuery("select hex(col1) from testtable"));
		var expectedhex="";
		for (var i=0;i<buffer.length;i++) {
			expectedhex+=("0"+buffer[i].toString(16)).slice(-2);
		}
		assertEqInt(cur.getFieldLength(0,0),expectedhex.length);
		assertEqStr(String(cur.getField(0,0)).toLowerCase(),
				expectedhex);
		assertTrue(cur.sendQuery("drop table testtable"));
		console.log("");


		// encoded binary data -
		// (null)"" - unescaped
		console.log("ENCODED BINARY DATA - (null)\"\" - unescaped: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery("create table testtable "+
			"(col1 longblob)"));
		assertTrue(cur.sendQuery(Buffer.from(
			"insert into testtable values (_binary'\0\"\"')",
			"binary"),43));
		assertTrue(cur.sendQuery("select hex(col1) from testtable"));
		assertEqInt(cur.getFieldLength(0,0),6);
		assertEqStr(String(cur.getField(0,0)).toLowerCase(),"002222");
		assertTrue(cur.sendQuery("drop table testtable"));
		console.log("");


		// encoded binary data -
		// (null)"" - \-escaped
		console.log("ENCODED BINARY DATA - \\(null)\\\"\\\" - "+
			"\\-escaped: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery("create table testtable "+
			"(col1 longblob)"));
		assertTrue(cur.sendQuery(Buffer.from(
			"insert into testtable values (_binary'\\\0\\\"\\\"')",
			"binary"),46));
		assertTrue(cur.sendQuery("select hex(col1) from testtable"));
		assertEqInt(cur.getFieldLength(0,0),6);
		assertEqStr(String(cur.getField(0,0)).toLowerCase(),"002222");
		assertTrue(cur.sendQuery("drop table testtable"));
		console.log("");
	}


	// quotes - '' - ''-escaped
	console.log("QUOTES - '' - ''-escaped: ");
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


	// quotes - '' - '',\-escaped
	console.log("QUOTES - '' - '',\\-escaped: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery("create table testtable "+
		"(col1 varchar(4))"));
	assertTrue(cur.sendQuery("insert into testtable "+
		"values ('''\\'')"));
	assertTrue(cur.sendQuery("select col1 from testtable"));
	assertEqInt(cur.getFieldLength(0,0),2);
	assertEqStr(cur.getField(0,0),"''");
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// quotes - '' - \,''-escaped
	console.log("QUOTES - '' - \\,''-escaped: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery("create table testtable "+
		"(col1 varchar(4))"));
	assertTrue(cur.sendQuery("insert into testtable "+
		"values ('\\'''')"));
	assertTrue(cur.sendQuery("select col1 from testtable"));
	assertEqInt(cur.getFieldLength(0,0),2);
	assertEqStr(cur.getField(0,0),"''");
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// quotes - \\' - \-escaped
	console.log("QUOTES - \\\\' - \\-escaped: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery("create table testtable "+
		"(col1 varchar(4))"));
	assertTrue(cur.sendQuery("insert into testtable "+
		"values ('\\\\\\'')"));
	assertTrue(cur.sendQuery("select col1 from testtable"));
	assertEqInt(cur.getFieldLength(0,0),2);
	assertEqStrLen(cur.getField(0,0),"\\'",2);
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// quotes - "" - unescaped
	console.log("QUOTES - \"\" - unescaped: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery("create table testtable "+
		"(col1 varchar(4))"));
	assertTrue(cur.sendQuery("insert into testtable "+
		"values ('\"\"')"));
	assertTrue(cur.sendQuery("select col1 from testtable"));
	assertEqInt(cur.getFieldLength(0,0),2);
	assertEqStr(cur.getField(0,0),"\"\"");
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// quotes - random - '',\-escaped
	console.log("QUOTES - random - '',\\-escaped: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery("create table testtable "+
		"(col1 varchar(255))"));
	// Build a random buffer of [', ", \, \0] bytes. Keep it as a Buffer
	// so \0 doesn't truncate when passed through the Node.js binding.
	var ch=[0x27,0x22,0x5c,0x00];
	var buffer=Buffer.alloc(255);
	for (var i=0;i<255;i++) {
		buffer[i]=ch[Math.floor(Math.random()*4)];
	}
	var chunks=[Buffer.from("insert into testtable values ('")];
	for (var i=0;i<buffer.length;i++) {
		var b=buffer[i];
		if (b==0x27) {
			// randomly escape ' with \ or ''
			if (Math.floor(Math.random()*2)) {
				chunks.push(Buffer.from([0x27]));
			} else {
				chunks.push(Buffer.from([0x5c]));
			}
		}
		if (b==0x22) {
			// randomly escape " with \ or don't
			if (Math.floor(Math.random()*2)) {
				chunks.push(Buffer.from([0x5c]));
			}
		}
		if (b==0x5c) {
			// always escape \ with \
			chunks.push(Buffer.from([0x5c]));
		}
		chunks.push(buffer.slice(i,i+1));
	}
	chunks.push(Buffer.from("')"));
	var query=Buffer.concat(chunks);
	assertTrue(cur.sendQuery(query,query.length));
	// Verify via server-side HEX (see the ENCODED BINARY DATA section
	// for why direct byte compare doesn't work through the binding).
	assertTrue(cur.sendQuery("select hex(col1) from testtable"));
	var expectedhex="";
	for (var i=0;i<buffer.length;i++) {
		expectedhex+=("0"+buffer[i].toString(16)).slice(-2);
	}
	assertEqInt(cur.getFieldLength(0,0),expectedhex.length);
	assertEqStr(String(cur.getField(0,0)).toLowerCase(),expectedhex);
	assertTrue(cur.sendQuery("drop table testtable"));
	console.log("");


	// last insert id
	console.log("LAST INSERT ID: ");
	cur.sendQuery("drop table testtable");
	assertTrue(cur.sendQuery(
		"create table testtable "+
		"	(col1 int primary key"+
		"	auto_increment, "+
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


	// mysql before 5.0 has no information_schema for these metadata queries
	if (majorversion>3) {
		// catalog list
		console.log("CATALOG LIST: ");
		assertTrue(cur.getCatalogList(null));
		assertEqStr(cur.getColumnName(0),"Database");
		assertInResultSet(cur,"Database",hostname);
		console.log("");


		// schema list
		console.log("SCHEMA LIST: ");
		assertTrue(cur.getSchemaList(null));
		assertEqStr(cur.getColumnName(0),"Database");
		// mysql has no schemas
		assertEqInt(cur.rowCount(),0);
		console.log("");


		// table type list
		console.log("TABLE TYPE LIST: ");
		assertTrue(cur.getTableTypeList());
		assertEqStr(cur.getColumnName(0),"table_type");
		assertInResultSet(cur,"table_type","TABLE");
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
		assertInResultSet(cur,"Tables_in_xxx","testtable1");
		assertInResultSet(cur,"Tables_in_xxx","testtable2");
		assertInResultSet(cur,"Tables_in_xxx","testtable3");
		assertInResultSet(cur,"Tables_in_xxx","testtable4");
		assertTrue(cur.sendQuery("drop table testtable1"));
		assertTrue(cur.sendQuery("drop table testtable2"));
		assertTrue(cur.sendQuery("drop table testtable3"));
		assertTrue(cur.sendQuery("drop table testtable4"));
		console.log("");


		// type info list
		console.log("TYPE INFO LIST: ");
		assertTrue(cur.getTypeInfoList("int"));
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
		assertEqStr(cur.getField(0,"type_name"),"INT");
		assertEqStr(cur.getField(0,"data_type"),"4");
		assertEqStr(cur.getField(0,"precision"),"10");
		assertEqStr(cur.getField(0,"local_type_name"),"INT");
		assertTrue(cur.getTypeInfoList("char"));
		assertEqStr(cur.getField(0,"type_name"),"CHAR");
		assertEqStr(cur.getField(0,"data_type"),"1");
		assertEqStr(cur.getField(0,"precision"),"255");
		assertEqStr(cur.getField(0,"local_type_name"),"CHAR");
		assertTrue(cur.getTypeInfoList("varchar"));
		assertEqStr(cur.getField(0,"type_name"),"VARCHAR");
		assertEqStr(cur.getField(0,"data_type"),"12");
		assertEqStr(cur.getField(0,"precision"),"65535");
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
			"	testtinyint tinyint, "+
			"	testsmallint smallint,"+
			"	testmediumint "+
			"	mediumint, "+
			"	testint int, "+
			"	testbigint bigint, "+
			"	testfloat float, "+
			"	testreal real, "+
			"	testdecimal "+
			"	decimal(2,1), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testdatetime "+
			"	datetime, "+
			"	testyear year, "+
			"	testchar char(40), "+
			"	testvarchar "+
			"	varchar(40), "+
			"	testtext text, "+
			"	testtinytext "+
			"	tinytext, "+
			"	testmediumtext "+
			"	mediumtext, "+
			"	testlongtext "+
			"	longtext, "+
			"	testblob blob, "+
			"	testtinyblob "+
			"	tinyblob, "+
			"	testmediumblob "+
			"	mediumblob, "+
			"	testlongblob "+
			"	longblob, "+
			"	testtimestamp "+
			"	timestamp)"));
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
		assertEqStr(cur.getField(0,"column_name"),"testtinyint");
		assertEqStr(cur.getField(1,"column_name"),"testsmallint");
		assertEqStr(cur.getField(2,"column_name"),
			"testmediumint");
		assertEqStr(cur.getField(3,"column_name"),"testint");
		assertEqStr(cur.getField(4,"column_name"),"testbigint");
		assertEqStr(cur.getField(5,"column_name"),"testfloat");
		assertEqStr(cur.getField(6,"column_name"),"testreal");
		assertEqStr(cur.getField(7,"column_name"),"testdecimal");
		assertEqStr(cur.getField(8,"column_name"),"testdate");
		assertEqStr(cur.getField(9,"column_name"),"testtime");
		assertEqStr(cur.getField(10,"column_name"),
			"testdatetime");
		assertEqStr(cur.getField(11,"column_name"),"testyear");
		assertEqStr(cur.getField(12,"column_name"),"testchar");
		assertEqStr(cur.getField(13,"column_name"),"testvarchar");
		assertEqStr(cur.getField(14,"column_name"),"testtext");
		assertEqStr(cur.getField(15,"column_name"),
			"testtinytext");
		assertEqStr(cur.getField(16,"column_name"),
			"testmediumtext");
		assertEqStr(cur.getField(17,"column_name"),
			"testlongtext");
		assertEqStr(cur.getField(18,"column_name"),"testblob");
		assertEqStr(cur.getField(19,"column_name"),
			"testtinyblob");
		assertEqStr(cur.getField(20,"column_name"),
			"testmediumblob");
		assertEqStr(cur.getField(21,"column_name"),
			"testlongblob");
		assertEqStr(cur.getField(22,"column_name"),
			"testtimestamp");
		assertEqStr(cur.getField(0,"data_type"),"TINYINT");
		assertEqStr(cur.getField(1,"data_type"),"SMALLINT");
		assertEqStr(cur.getField(2,"data_type"),"MEDIUMINT");
		assertEqStr(cur.getField(3,"data_type"),"INT");
		assertEqStr(cur.getField(4,"data_type"),"BIGINT");
		assertEqStr(cur.getField(5,"data_type"),"FLOAT");
		// not "REAL"
		assertEqStr(cur.getField(6,"data_type"),"DOUBLE");
		assertEqStr(cur.getField(7,"data_type"),"DECIMAL");
		assertEqStr(cur.getField(8,"data_type"),"DATE");
		assertEqStr(cur.getField(9,"data_type"),"TIME");
		assertEqStr(cur.getField(10,"data_type"),"DATETIME");
		assertEqStr(cur.getField(11,"data_type"),"YEAR");
		assertEqStr(cur.getField(12,"data_type"),"CHAR");
		assertEqStr(cur.getField(13,"data_type"),"VARCHAR");
		assertEqStr(cur.getField(14,"data_type"),"TEXT");
		assertEqStr(cur.getField(15,"data_type"),"TINYTEXT");
		assertEqStr(cur.getField(16,"data_type"),"MEDIUMTEXT");
		assertEqStr(cur.getField(17,"data_type"),"LONGTEXT");
		assertEqStr(cur.getField(18,"data_type"),"BLOB");
		assertEqStr(cur.getField(19,"data_type"),"TINYBLOB");
		assertEqStr(cur.getField(20,"data_type"),"MEDIUMBLOB");
		assertEqStr(cur.getField(21,"data_type"),"LONGBLOB");
		assertEqStr(cur.getField(22,"data_type"),"TIMESTAMP");
		assertTrue(cur.sendQuery("drop table testtable"));
		console.log("");


		// column list - auto_increment,
		// primary key
		console.log("COLUMN LIST - auto_increment, primary key: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int "+
			"	auto_increment "+
			"	primary key, "+
			"	col2 int)"));
		assertTrue(cur.getColumnList("testtable",null));
		assertEqStr(cur.getField(0,"extra"),"auto_increment");
		assertEqStr(cur.getField(0,"column_key"),"PRI");
		assertEqStr(cur.getField(1,"extra"),"");
		assertEqStr(cur.getField(1,"column_key"),"");
		console.log("");
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int "+
			"	primary key, "+
			"	col2 int)"));
		assertTrue(cur.getColumnList("testtable",null));
		assertEqStr(cur.getField(0,"extra"),"");
		assertEqStr(cur.getField(0,"column_key"),"PRI");
		assertTrue(cur.sendQuery("drop table testtable"));
		console.log("");


		// primary keys list
		console.log("PRIMARY KEYS LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int "+
			"	primary key, "+
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
		assertEqStr(cur.getField(0,"key_name"),"PRIMARY");
		assertTrue(cur.sendQuery("drop table testtable"));
		console.log("");


		// key and index list
		console.log("KEY AND INDEX LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int "+
			"	primary key, "+
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
		assertEqStr(cur.getField(0,"non_unique"),"false");
		assertEqStr(cur.getField(0,"seq_in_index"),"1");
		assertTrue(cur.getField(0,"column_name")=="col1");
		assertEqStr(cur.getField(0,"collation"),"A");
		assertEqStr(cur.getField(0,"index_type"),"3");
		assertEqStr(cur.getField(0,"key_name"),"PRIMARY");
		assertTrue(cur.sendQuery("drop table testtable"));
		console.log("");


		// procedure list
		console.log("PROCEDURE LIST: ");
		cur.sendQuery("drop procedure testproc1");
		cur.sendQuery("drop procedure testproc2");
		cur.sendQuery("drop procedure testproc3");
		cur.sendQuery("drop procedure testproc4");
		assertTrue(cur.sendQuery("create procedure "+
			"testproc1("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) begin end"));
		assertTrue(cur.sendQuery("create procedure "+
			"testproc2("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) begin end"));
		assertTrue(cur.sendQuery("create procedure "+
			"testproc3("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) begin end"));
		assertTrue(cur.sendQuery("create procedure "+
			"testproc4("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) begin end"));
		assertTrue(cur.getProcedureList(null));
		assertInResultSet(cur,"routine_name","testproc1");
		assertInResultSet(cur,"routine_name","testproc2");
		assertInResultSet(cur,"routine_name","testproc3");
		assertInResultSet(cur,"routine_name","testproc4");
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
		assertEqStr(cur.getField(0,"data_type"),"INT");
		assertEqStr(cur.getField(0,"ordinal_position"),"1");
		assertEqStr(cur.getField(1,"parameter_name"),"in2");
		assertEqStr(cur.getField(1,"parameter_mode"),"1");
		assertEqStr(cur.getField(1,"data_type"),"CHAR");
		assertEqStr(cur.getField(1,"ordinal_position"),"2");
		assertEqStr(cur.getField(2,"parameter_name"),"in3");
		assertEqStr(cur.getField(2,"parameter_mode"),"1");
		assertEqStr(cur.getField(2,"data_type"),"VARCHAR");
		assertEqStr(cur.getField(2,"ordinal_position"),"3");
		assertEqStr(cur.getField(3,"parameter_name"),"in4");
		assertEqStr(cur.getField(3,"parameter_mode"),"1");
		assertEqStr(cur.getField(3,"data_type"),"DATE");
		assertEqStr(cur.getField(3,"ordinal_position"),"4");
		assertTrue(cur.sendQuery("drop procedure testproc1"));
		assertTrue(cur.sendQuery("drop procedure testproc2"));
		assertTrue(cur.sendQuery("drop procedure testproc3"));
		assertTrue(cur.sendQuery("drop procedure testproc4"));
		console.log("");
	}


	// invalid queries
	console.log("INVALID QUERIES: ");
	assertFalse(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	assertFalse(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	assertFalse(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
	assertFalse(cur.sendQuery(
		"select "+
		"	* "+
		"from "+
		"	testtable "+
		"order by "+
		"	testtinyint "));
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
