<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.
	include("./asserts.php");


	// instantiation
	$con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
			"testuser","testpassword",0,1);
	$cur=sqlrcur_alloc($con);


	// identify
	echo("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify($con),"mysql");
	echo("\n");


	// db version
	echo("DB VERSION: \n");
	$dbversion=sqlrcon_dbVersion($con);
	$majorversion=(int)$dbversion[0];
	echo("\n");


	// ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	// transaction state
	echo("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel($con),"explicit-deferred");
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit-deferred");
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	// bind format
	echo("BIND FORMAT: \n");
	if ($majorversion>3) {
		assertEqStr(sqlrcon_bindFormat($con),"?");
	} else {
		assertEqStr(sqlrcon_bindFormat($con),":*");
	}
	echo("\n");


	// nextval format
	echo("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat($con),"");
	echo("\n");


	// isolation levels
	echo("ISOLATION LEVELS: \n");
	$isolationlevels=array("REPEATABLE-READ","READ-UNCOMMITTED",
				"READ-COMMITTED","SERIALIZABLE");
	foreach ($isolationlevels as $il) {
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		assertEqStr(sqlrcon_getIsolationLevel($con),$il);
		echo("\n");
	}
	// reset to the default isolation level
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	echo("\n");


	// create testtable
	echo("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testtinyint tinyint, ".
		"	testsmallint smallint, ".
		"	testmediumint mediumint,".
		"	testint int, ".
		"	testbigint bigint, ".
		"	testfloat float, ".
		"	testreal real, ".
		"	testdecimal decimal(2,1),".
		"	testdate date, ".
		"	testtime time, ".
		"	testdatetime datetime, ".
		"	testyear year, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40),".
		"	testtext text, ".
		"	testtinytext tinytext, ".
		"	testmediumtext ".
		"	mediumtext, ".
		"	testlongtext longtext, ".
		"	testblob blob, ".
		"	testtinyblob tinyblob, ".
		"	testmediumblob ".
		"	mediumblob, ".
		"	testlongblob longblob, ".
		"	testtimestamp timestamp)"));
	echo("\n");


	// insert
	echo("INSERT: \n");
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	1, ".
		"	1, ".
		"	1, ".
		"	1, ".
		"	1.1, ".
		"	1.1, ".
		"	1.1, ".
		"	'2001-01-01', ".
		"	'01:00:00', ".
		"	'2001-01-01 01:00:00', ".
		"	'2001', ".
		"	'char1', ".
		"	'varchar1', ".
		"	'text1', ".
		"	'tinytext1', ".
		"	'mediumtext1', ".
		"	'longtext1', ".
		"	'blob1', ".
		"	'tinyblob1', ".
		"	'mediumblob1', ".
		"	'longblob1', ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	2, ".
		"	2, ".
		"	2, ".
		"	2, ".
		"	2, ".
		"	2.1, ".
		"	2.1, ".
		"	2.1, ".
		"	'2002-01-01', ".
		"	'02:00:00', ".
		"	'2002-01-01 02:00:00', ".
		"	'2002', ".
		"	'char2', ".
		"	'varchar2', ".
		"	'text2', ".
		"	'tinytext2', ".
		"	'mediumtext2', ".
		"	'longtext2', ".
		"	'blob2', ".
		"	'tinyblob2', ".
		"	'mediumblob2', ".
		"	'longblob2', ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	3, ".
		"	3, ".
		"	3, ".
		"	3, ".
		"	3, ".
		"	3.1, ".
		"	3.1, ".
		"	3.1, ".
		"	'2003-01-01', ".
		"	'03:00:00', ".
		"	'2003-01-01 03:00:00', ".
		"	'2003', ".
		"	'char3', ".
		"	'varchar3', ".
		"	'text3', ".
		"	'tinytext3', ".
		"	'mediumtext3', ".
		"	'longtext3', ".
		"	'blob3', ".
		"	'tinyblob3', ".
		"	'mediumblob3', ".
		"	'longblob3', ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	4, ".
		"	4, ".
		"	4, ".
		"	4, ".
		"	4, ".
		"	4.1, ".
		"	4.1, ".
		"	4.1, ".
		"	'2004-01-01', ".
		"	'04:00:00', ".
		"	'2004-01-01 04:00:00', ".
		"	'2004', ".
		"	'char4', ".
		"	'varchar4', ".
		"	'text4', ".
		"	'tinytext4', ".
		"	'mediumtext4', ".
		"	'longtext4', ".
		"	'blob4', ".
		"	'tinyblob4', ".
		"	'mediumblob4', ".
		"	'longblob4', ".
		"	NULL)"));
	echo("\n");


	// affected rows
	echo("AFFECTED ROWS: \n");
	assertEqInt(sqlrcur_affectedRows($cur),1);
	echo("\n");


	// input bind by position
	echo("INPUT BIND BY POSITION: \n");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?, ".
		"	NULL)");
	assertEqInt(sqlrcur_countBindVariables($cur),22);
	sqlrcur_inputBind($cur,"1",5);
	sqlrcur_inputBind($cur,"2",5);
	sqlrcur_inputBind($cur,"3",5);
	sqlrcur_inputBind($cur,"4",5);
	sqlrcur_inputBind($cur,"5",5);
	sqlrcur_inputBind($cur,"6",5.1,2,1);
	sqlrcur_inputBind($cur,"7",5.1,2,1);
	sqlrcur_inputBind($cur,"8",5.1,2,1);
	sqlrcur_inputBind($cur,"9","2005-01-01");
	sqlrcur_inputBind($cur,"10","05:00:00");
	sqlrcur_inputBindDate($cur,"11",2005,1,1,5,0,0,0,NULL,0);
	sqlrcur_inputBind($cur,"12","2005");
	sqlrcur_inputBind($cur,"13","char5");
	sqlrcur_inputBind($cur,"14","varchar5");
	sqlrcur_inputBindClob($cur,"15","text5",strlen("text5"));
	sqlrcur_inputBindClob($cur,"16","tinytext5",strlen("tinytext5"));
	sqlrcur_inputBindClob($cur,"17","mediumtext5",strlen("mediumtext5"));
	sqlrcur_inputBindClob($cur,"18","longtext5",strlen("longtext5"));
	sqlrcur_inputBindBlob($cur,"19","blob5",strlen("blob5"));
	sqlrcur_inputBindBlob($cur,"20","tinyblob5",strlen("tinyblob5"));
	sqlrcur_inputBindBlob($cur,"21","mediumblob5",strlen("mediumblob5"));
	sqlrcur_inputBindBlob($cur,"22","longblob5",strlen("longblob5"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",6);
	sqlrcur_inputBind($cur,"2",6);
	sqlrcur_inputBind($cur,"3",6);
	sqlrcur_inputBind($cur,"4",6);
	sqlrcur_inputBind($cur,"5",6);
	sqlrcur_inputBind($cur,"6",6.1,2,1);
	sqlrcur_inputBind($cur,"7",6.1,2,1);
	sqlrcur_inputBind($cur,"8",6.1,2,1);
	sqlrcur_inputBind($cur,"9","2006-01-01");
	sqlrcur_inputBind($cur,"10","06:00:00");
	sqlrcur_inputBindDate($cur,"11",2006,1,1,6,0,0,0,NULL,0);
	sqlrcur_inputBind($cur,"12","2006");
	sqlrcur_inputBind($cur,"13","char6");
	sqlrcur_inputBind($cur,"14","varchar6");
	sqlrcur_inputBindClob($cur,"15","text6",strlen("text6"));
	sqlrcur_inputBindClob($cur,"16","tinytext6",strlen("tinytext6"));
	sqlrcur_inputBindClob($cur,"17","mediumtext6",strlen("mediumtext6"));
	sqlrcur_inputBindClob($cur,"18","longtext6",strlen("longtext6"));
	sqlrcur_inputBindBlob($cur,"19","blob6",strlen("blob6"));
	sqlrcur_inputBindBlob($cur,"20","tinyblob6",strlen("tinyblob6"));
	sqlrcur_inputBindBlob($cur,"21","mediumblob6",strlen("mediumblob6"));
	sqlrcur_inputBindBlob($cur,"22","longblob6",strlen("longblob6"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",7);
	sqlrcur_inputBind($cur,"2",7);
	sqlrcur_inputBind($cur,"3",7);
	sqlrcur_inputBind($cur,"4",7);
	sqlrcur_inputBind($cur,"5",7);
	sqlrcur_inputBind($cur,"6",7.1,2,1);
	sqlrcur_inputBind($cur,"7",7.1,2,1);
	sqlrcur_inputBind($cur,"8",7.1,2,1);
	sqlrcur_inputBind($cur,"9","2007-01-01");
	sqlrcur_inputBind($cur,"10","07:00:00");
	sqlrcur_inputBindDate($cur,"11",2007,1,1,7,0,0,0,NULL,0);
	sqlrcur_inputBind($cur,"12","2007");
	sqlrcur_inputBind($cur,"13","char7");
	sqlrcur_inputBind($cur,"14","varchar7");
	sqlrcur_inputBindClob($cur,"15","text7",strlen("text7"));
	sqlrcur_inputBindClob($cur,"16","tinytext7",strlen("tinytext7"));
	sqlrcur_inputBindClob($cur,"17","mediumtext7",strlen("mediumtext7"));
	sqlrcur_inputBindClob($cur,"18","longtext7",strlen("longtext7"));
	sqlrcur_inputBindBlob($cur,"19","blob7",strlen("blob7"));
	sqlrcur_inputBindBlob($cur,"20","tinyblob7",strlen("tinyblob7"));
	sqlrcur_inputBindBlob($cur,"21","mediumblob7",strlen("mediumblob7"));
	sqlrcur_inputBindBlob($cur,"22","longblob7",strlen("longblob7"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	// array of input binds by position
	// mysql doesn't support implicit
	// conversion of string binds to other
	// data types, so arrays of binds don't
	// generally work.


	// input bind by position with
	// validation
	echo("BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",8);
	sqlrcur_inputBind($cur,"2",8);
	sqlrcur_inputBind($cur,"3",8);
	sqlrcur_inputBind($cur,"4",8);
	sqlrcur_inputBind($cur,"5",8);
	sqlrcur_inputBind($cur,"6",8.1,2,1);
	sqlrcur_inputBind($cur,"7",8.1,2,1);
	sqlrcur_inputBind($cur,"8",8.1,2,1);
	sqlrcur_inputBind($cur,"9","2008-01-01");
	sqlrcur_inputBind($cur,"10","08:00:00");
	sqlrcur_inputBindDate($cur,"11",2008,1,1,8,0,0,0,NULL,0);
	sqlrcur_inputBind($cur,"12","2008");
	sqlrcur_inputBind($cur,"13","char8");
	sqlrcur_inputBind($cur,"14","varchar8");
	sqlrcur_inputBindClob($cur,"15","text8",strlen("text8"));
	sqlrcur_inputBindClob($cur,"16","tinytext8",strlen("tinytext8"));
	sqlrcur_inputBindClob($cur,"17","mediumtext8",strlen("mediumtext8"));
	sqlrcur_inputBindClob($cur,"18","longtext8",strlen("longtext8"));
	sqlrcur_inputBindBlob($cur,"19","blob8",strlen("blob8"));
	sqlrcur_inputBindBlob($cur,"20","tinyblob8",strlen("tinyblob8"));
	sqlrcur_inputBindBlob($cur,"21","mediumblob8",strlen("mediumblob8"));
	sqlrcur_inputBindBlob($cur,"22","longblob8",strlen("longblob8"));
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	// input bind by name
	// mysql doesn't support bind by name


	// array of input binds by name
	// mysql doesn't support bind by name


	// input bind by name with validation
	// mysql doesn't support bind by name


	// select
	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	echo("\n");


	// column count
	echo("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount($cur),23);
	echo("\n");


	// column names
	echo("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testtinyint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testmediumint");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testbigint");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testreal");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testdecimal");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testdate");
	assertEqStr(sqlrcur_getColumnName($cur,9),"testtime");
	assertEqStr(sqlrcur_getColumnName($cur,10),"testdatetime");
	assertEqStr(sqlrcur_getColumnName($cur,11),"testyear");
	assertEqStr(sqlrcur_getColumnName($cur,12),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,13),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,14),"testtext");
	assertEqStr(sqlrcur_getColumnName($cur,15),"testtinytext");
	assertEqStr(sqlrcur_getColumnName($cur,16),"testmediumtext");
	assertEqStr(sqlrcur_getColumnName($cur,17),"testlongtext");
	assertEqStr(sqlrcur_getColumnName($cur,18),"testblob");
	assertEqStr(sqlrcur_getColumnName($cur,19),"testtinyblob");
	assertEqStr(sqlrcur_getColumnName($cur,20),"testmediumblob");
	assertEqStr(sqlrcur_getColumnName($cur,21),"testlongblob");
	assertEqStr(sqlrcur_getColumnName($cur,22),"testtimestamp");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testtinyint");
	assertEqStr($cols[1],"testsmallint");
	assertEqStr($cols[2],"testmediumint");
	assertEqStr($cols[3],"testint");
	assertEqStr($cols[4],"testbigint");
	assertEqStr($cols[5],"testfloat");
	assertEqStr($cols[6],"testreal");
	assertEqStr($cols[7],"testdecimal");
	assertEqStr($cols[8],"testdate");
	assertEqStr($cols[9],"testtime");
	assertEqStr($cols[10],"testdatetime");
	assertEqStr($cols[11],"testyear");
	assertEqStr($cols[12],"testchar");
	assertEqStr($cols[13],"testvarchar");
	assertEqStr($cols[14],"testtext");
	assertEqStr($cols[15],"testtinytext");
	assertEqStr($cols[16],"testmediumtext");
	assertEqStr($cols[17],"testlongtext");
	assertEqStr($cols[18],"testblob");
	assertEqStr($cols[19],"testtinyblob");
	assertEqStr($cols[20],"testmediumblob");
	assertEqStr($cols[21],"testlongblob");
	assertEqStr($cols[22],"testtimestamp");
	echo("\n");


	// column types
	echo("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnType($cur,0),"TINYINT");
	assertEqStr(sqlrcur_getColumnType($cur,1),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,2),"MEDIUMINT");
	assertEqStr(sqlrcur_getColumnType($cur,3),"INT");
	assertEqStr(sqlrcur_getColumnType($cur,4),"BIGINT");
	assertEqStr(sqlrcur_getColumnType($cur,5),"FLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,6),"REAL");
	assertEqStr(sqlrcur_getColumnType($cur,7),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,8),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,9),"TIME");
	assertEqStr(sqlrcur_getColumnType($cur,10),"DATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,11),"YEAR");
	if ($majorversion==3) {
		assertEqStr(sqlrcur_getColumnType($cur,12),"VARSTRING");
	} else {
		assertEqStr(sqlrcur_getColumnType($cur,12),"STRING");
	}
	assertEqStr(sqlrcur_getColumnType($cur,13),"VARSTRING");
	assertEqStr(sqlrcur_getColumnType($cur,14),"BLOB");
	assertEqStr(sqlrcur_getColumnType($cur,15),"TINYBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,16),"MEDIUMBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,17),"LONGBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,18),"BLOB");
	assertEqStr(sqlrcur_getColumnType($cur,19),"TINYBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,20),"MEDIUMBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,21),"LONGBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,22),"TIMESTAMP");
	assertEqStr(sqlrcur_getColumnType($cur,"testtinyint"),"TINYINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testsmallint"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testmediumint"),
		"MEDIUMINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testint"),"INT");
	assertEqStr(sqlrcur_getColumnType($cur,"testbigint"),"BIGINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testfloat"),"FLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,"testreal"),"REAL");
	assertEqStr(sqlrcur_getColumnType($cur,"testdecimal"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,"testdate"),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,"testtime"),"TIME");
	assertEqStr(sqlrcur_getColumnType($cur,"testdatetime"),"DATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,"testyear"),"YEAR");
	if ($majorversion==3) {
		assertEqStr(sqlrcur_getColumnType($cur,"testchar"),
		    "VARSTRING");
	} else {
		assertEqStr(sqlrcur_getColumnType($cur,"testchar"),
		    "STRING");
	}
	assertEqStr(sqlrcur_getColumnType($cur,"testvarchar"),"VARSTRING");
	assertEqStr(sqlrcur_getColumnType($cur,"testtext"),"BLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testtinytext"),"TINYBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testmediumtext"),
		"MEDIUMBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testlongtext"),"LONGBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testblob"),"BLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testtinyblob"),"TINYBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testmediumblob"),
		"MEDIUMBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testlongblob"),"LONGBLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"testtimestamp"),
		"TIMESTAMP");
	echo("\n");


	// column length
	echo("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLength($cur,0),1);
	assertEqInt(sqlrcur_getColumnLength($cur,1),2);
	assertEqInt(sqlrcur_getColumnLength($cur,2),3);
	assertEqInt(sqlrcur_getColumnLength($cur,3),4);
	assertEqInt(sqlrcur_getColumnLength($cur,4),8);
	assertEqInt(sqlrcur_getColumnLength($cur,5),4);
	assertEqInt(sqlrcur_getColumnLength($cur,6),8);
	assertEqInt(sqlrcur_getColumnLength($cur,7),6);
	assertEqInt(sqlrcur_getColumnLength($cur,8),3);
	assertEqInt(sqlrcur_getColumnLength($cur,9),3);
	assertEqInt(sqlrcur_getColumnLength($cur,10),8);
	assertEqInt(sqlrcur_getColumnLength($cur,11),1);
	// these can be 120/121 if the db
	// charset is utf8
	//assertEqInt(sqlrcur_getColumnLength($cur,12),40);
	//assertEqInt(sqlrcur_getColumnLength($cur,13),41);
	assertEqInt(sqlrcur_getColumnLength($cur,14),65535);
	assertEqInt(sqlrcur_getColumnLength($cur,15),255);
	assertEqInt(sqlrcur_getColumnLength($cur,16),16777215);
	assertEqInt(sqlrcur_getColumnLength($cur,17),2147483647);
	assertEqInt(sqlrcur_getColumnLength($cur,18),65535);
	assertEqInt(sqlrcur_getColumnLength($cur,19),255);
	assertEqInt(sqlrcur_getColumnLength($cur,20),16777215);
	assertEqInt(sqlrcur_getColumnLength($cur,21),2147483647);
	assertEqInt(sqlrcur_getColumnLength($cur,22),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getColumnLength($cur,"testsmallint"),2);
	assertEqInt(sqlrcur_getColumnLength($cur,"testmediumint"),3);
	assertEqInt(sqlrcur_getColumnLength($cur,"testint"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testbigint"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testfloat"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testreal"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdecimal"),6);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdate"),3);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtime"),3);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdatetime"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testyear"),1);
	// these can be 120/121 if the db
	// charset is utf8
	//assertEqInt(sqlrcur_getColumnLength($cur,"testchar"),40);
	//assertEqInt(sqlrcur_getColumnLength($cur,"testvarchar"),41);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtext"),65535);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtinytext"),255);
	assertEqInt(sqlrcur_getColumnLength($cur,"testmediumtext"),
		16777215);
	assertEqInt(sqlrcur_getColumnLength($cur,"testlongtext"),
		2147483647);
	assertEqInt(sqlrcur_getColumnLength($cur,"testblob"),65535);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtinyblob"),255);
	assertEqInt(sqlrcur_getColumnLength($cur,"testmediumblob"),
		16777215);
	assertEqInt(sqlrcur_getColumnLength($cur,"testlongblob"),
		2147483647);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtimestamp"),4);
	echo("\n");


	// longest column
	echo("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongest($cur,0),1);
	assertEqInt(sqlrcur_getLongest($cur,1),1);
	assertEqInt(sqlrcur_getLongest($cur,2),1);
	assertEqInt(sqlrcur_getLongest($cur,3),1);
	assertEqInt(sqlrcur_getLongest($cur,4),1);
	//assertEqInt(sqlrcur_getLongest($cur,5),3);
	assertEqInt(sqlrcur_getLongest($cur,6),3);
	assertEqInt(sqlrcur_getLongest($cur,7),3);
	assertEqInt(sqlrcur_getLongest($cur,8),10);
	assertEqInt(sqlrcur_getLongest($cur,9),8);
	assertEqInt(sqlrcur_getLongest($cur,10),19);
	assertEqInt(sqlrcur_getLongest($cur,11),4);
	assertEqInt(sqlrcur_getLongest($cur,12),5);
	assertEqInt(sqlrcur_getLongest($cur,13),8);
	assertEqInt(sqlrcur_getLongest($cur,14),5);
	assertEqInt(sqlrcur_getLongest($cur,15),9);
	assertEqInt(sqlrcur_getLongest($cur,16),11);
	assertEqInt(sqlrcur_getLongest($cur,17),9);
	assertEqInt(sqlrcur_getLongest($cur,18),5);
	assertEqInt(sqlrcur_getLongest($cur,19),9);
	assertEqInt(sqlrcur_getLongest($cur,20),11);
	assertEqInt(sqlrcur_getLongest($cur,21),9);
	if ($majorversion==3) {
		assertEqInt(sqlrcur_getLongest($cur,22),14);
	} else {
		assertEqInt(sqlrcur_getLongest($cur,22),19);
	}
	assertEqInt(sqlrcur_getLongest($cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getLongest($cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongest($cur,"testmediumint"),1);
	assertEqInt(sqlrcur_getLongest($cur,"testint"),1);
	assertEqInt(sqlrcur_getLongest($cur,"testbigint"),1);
	//assertEqInt(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongest($cur,"testreal"),3);
	assertEqInt(sqlrcur_getLongest($cur,"testdecimal"),3);
	assertEqInt(sqlrcur_getLongest($cur,"testdate"),10);
	assertEqInt(sqlrcur_getLongest($cur,"testtime"),8);
	assertEqInt(sqlrcur_getLongest($cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getLongest($cur,"testyear"),4);
	assertEqInt(sqlrcur_getLongest($cur,"testchar"),5);
	assertEqInt(sqlrcur_getLongest($cur,"testvarchar"),8);
	assertEqInt(sqlrcur_getLongest($cur,"testtext"),5);
	assertEqInt(sqlrcur_getLongest($cur,"testtinytext"),9);
	assertEqInt(sqlrcur_getLongest($cur,"testmediumtext"),11);
	assertEqInt(sqlrcur_getLongest($cur,"testlongtext"),9);
	assertEqInt(sqlrcur_getLongest($cur,"testblob"),5);
	assertEqInt(sqlrcur_getLongest($cur,"testtinyblob"),9);
	assertEqInt(sqlrcur_getLongest($cur,"testmediumblob"),11);
	assertEqInt(sqlrcur_getLongest($cur,"testlongblob"),9);
	if ($majorversion==3) {
		assertEqInt(sqlrcur_getLongest($cur,"testtimestamp"),14);
	} else {
		assertEqInt(sqlrcur_getLongest($cur,"testtimestamp"),19);
	}
	echo("\n");


	// row count
	echo("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount($cur),8);
	echo("\n");


	// total rows
	echo("TOTAL ROWS: \n");
	// older versions of mysql know this
	//assertEqInt(sqlrcur_totalRows($cur),0);
	echo("\n");


	// first row index
	echo("FIRST ROW INDEX: \n");
	assertEqInt(sqlrcur_firstRowIndex($cur),0);
	echo("\n");


	// end of result set
	echo("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet($cur));
	echo("\n");


	// fields by index
	echo("FIELDS BY INDEX: \n");
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),"1");
	assertEqStr(sqlrcur_getField($cur,0,3),"1");
	assertEqStr(sqlrcur_getField($cur,0,4),"1");
	//assertEqStr(sqlrcur_getField($cur,0,5),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,6),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,7),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,8),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,9),"01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,10),"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,11),"2001");
	assertEqStr(sqlrcur_getField($cur,0,12),"char1");
	assertEqStr(sqlrcur_getField($cur,0,13),"varchar1");
	assertEqStr(sqlrcur_getField($cur,0,14),"text1");
	assertEqStr(sqlrcur_getField($cur,0,15),"tinytext1");
	assertEqStr(sqlrcur_getField($cur,0,16),"mediumtext1");
	assertEqStr(sqlrcur_getField($cur,0,17),"longtext1");
	assertEqStr(sqlrcur_getField($cur,0,18),"blob1");
	assertEqStr(sqlrcur_getField($cur,0,19),"tinyblob1");
	assertEqStr(sqlrcur_getField($cur,0,20),"mediumblob1");
	assertEqStr(sqlrcur_getField($cur,0,21),"longblob1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,7,2),"8");
	assertEqStr(sqlrcur_getField($cur,7,3),"8");
	assertEqStr(sqlrcur_getField($cur,7,4),"8");
	//assertEqStr(sqlrcur_getField($cur,7,5),"8.1");
	assertEqStr(sqlrcur_getField($cur,7,6),"8.1");
	assertEqStr(sqlrcur_getField($cur,7,7),"8.1");
	assertEqStr(sqlrcur_getField($cur,7,8),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,9),"08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,10),"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,11),"2008");
	assertEqStr(sqlrcur_getField($cur,7,12),"char8");
	assertEqStr(sqlrcur_getField($cur,7,13),"varchar8");
	assertEqStr(sqlrcur_getField($cur,7,14),"text8");
	assertEqStr(sqlrcur_getField($cur,7,15),"tinytext8");
	assertEqStr(sqlrcur_getField($cur,7,16),"mediumtext8");
	assertEqStr(sqlrcur_getField($cur,7,17),"longtext8");
	assertEqStr(sqlrcur_getField($cur,7,18),"blob8");
	assertEqStr(sqlrcur_getField($cur,7,19),"tinyblob8");
	assertEqStr(sqlrcur_getField($cur,7,20),"mediumblob8");
	assertEqStr(sqlrcur_getField($cur,7,21),"longblob8");
	echo("\n");


	// field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,3),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,4),1);
	//assertEqInt(sqlrcur_getFieldLength($cur,0,5),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,6),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,7),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,8),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,9),8);
	assertEqInt(sqlrcur_getFieldLength($cur,0,10),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,11),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,12),5);
	assertEqInt(sqlrcur_getFieldLength($cur,0,13),8);
	assertEqInt(sqlrcur_getFieldLength($cur,0,14),5);
	assertEqInt(sqlrcur_getFieldLength($cur,0,15),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,16),11);
	assertEqInt(sqlrcur_getFieldLength($cur,0,17),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,18),5);
	assertEqInt(sqlrcur_getFieldLength($cur,0,19),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,20),11);
	assertEqInt(sqlrcur_getFieldLength($cur,0,21),9);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,3),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,4),1);
	//assertEqInt(sqlrcur_getFieldLength($cur,7,5),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,6),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,7),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,8),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,9),8);
	assertEqInt(sqlrcur_getFieldLength($cur,7,10),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,11),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,12),5);
	assertEqInt(sqlrcur_getFieldLength($cur,7,13),8);
	assertEqInt(sqlrcur_getFieldLength($cur,7,14),5);
	assertEqInt(sqlrcur_getFieldLength($cur,7,15),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,16),11);
	assertEqInt(sqlrcur_getFieldLength($cur,7,17),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,18),5);
	assertEqInt(sqlrcur_getFieldLength($cur,7,19),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,20),11);
	assertEqInt(sqlrcur_getFieldLength($cur,7,21),9);
	echo("\n");


	// fields by name
	echo("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getField($cur,0,"testtinyint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testmediumint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testbigint"),"1");
	//assertEqStr(sqlrcur_getField($cur,0,"testfloat"),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,"testreal"),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,"testdecimal"),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,"testdate"),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,"testtime"),"01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,"testdatetime"),
		"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,"testyear"),"2001");
	assertEqStr(sqlrcur_getField($cur,0,"testchar"),"char1");
	assertEqStr(sqlrcur_getField($cur,0,"testvarchar"),"varchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testtext"),"text1");
	assertEqStr(sqlrcur_getField($cur,0,"testtinytext"),"tinytext1");
	assertEqStr(sqlrcur_getField($cur,0,"testmediumtext"),
		"mediumtext1");
	assertEqStr(sqlrcur_getField($cur,0,"testlongtext"),"longtext1");
	assertEqStr(sqlrcur_getField($cur,0,"testblob"),"blob1");
	assertEqStr(sqlrcur_getField($cur,0,"testlongblob"),"longblob1");
	assertEqStr(sqlrcur_getField($cur,0,"testtinyblob"),"tinyblob1");
	assertEqStr(sqlrcur_getField($cur,0,"testmediumblob"),
		"mediumblob1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,"testtinyint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testmediumint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testbigint"),"8");
	//assertEqStr(sqlrcur_getField($cur,7,"testfloat"),"8.1");
	assertEqStr(sqlrcur_getField($cur,7,"testreal"),"8.1");
	assertEqStr(sqlrcur_getField($cur,7,"testdecimal"),"8.1");
	assertEqStr(sqlrcur_getField($cur,7,"testdate"),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,"testtime"),"08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,"testdatetime"),
		"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,"testyear"),"2008");
	assertEqStr(sqlrcur_getField($cur,7,"testchar"),"char8");
	assertEqStr(sqlrcur_getField($cur,7,"testvarchar"),"varchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testtext"),"text8");
	assertEqStr(sqlrcur_getField($cur,7,"testtinytext"),"tinytext8");
	assertEqStr(sqlrcur_getField($cur,7,"testmediumtext"),
		"mediumtext8");
	assertEqStr(sqlrcur_getField($cur,7,"testlongtext"),"longtext8");
	assertEqStr(sqlrcur_getField($cur,7,"testblob"),"blob8");
	assertEqStr(sqlrcur_getField($cur,7,"testlongblob"),"longblob8");
	assertEqStr(sqlrcur_getField($cur,7,"testtinyblob"),"tinyblob8");
	assertEqStr(sqlrcur_getField($cur,7,"testmediumblob"),
		"mediumblob8");
	echo("\n");


	// field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtinyint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testmediumint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testbigint"),1);
	//assertEqInt(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testyear"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testchar"),5);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testvarchar"),8);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtext"),5);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtinytext"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testmediumtext"),11);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testlongtext"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testblob"),5);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtinyblob"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testmediumblob"),11);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testlongblob"),9);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtinyint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testmediumint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testbigint"),1);
	//assertEqInt(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testyear"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testchar"),5);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testvarchar"),8);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtext"),5);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtinytext"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testmediumtext"),11);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testlongtext"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testblob"),5);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtinyblob"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testmediumblob"),11);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testlongblob"),9);
	echo("\n");


	// fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqStr($fields[0],"1");
	assertEqStr($fields[1],"1");
	assertEqStr($fields[2],"1");
	assertEqStr($fields[3],"1");
	assertEqStr($fields[4],"1");
	//assertEqStr($fields[5],"1.1");
	assertEqStr($fields[6],"1.1");
	assertEqStr($fields[7],"1.1");
	assertEqStr($fields[8],"2001-01-01");
	assertEqStr($fields[9],"01:00:00");
	assertEqStr($fields[10],"2001-01-01 01:00:00");
	assertEqStr($fields[11],"2001");
	assertEqStr($fields[12],"char1");
	assertEqStr($fields[13],"varchar1");
	assertEqStr($fields[14],"text1");
	assertEqStr($fields[15],"tinytext1");
	assertEqStr($fields[16],"mediumtext1");
	assertEqStr($fields[17],"longtext1");
	assertEqStr($fields[18],"blob1");
	assertEqStr($fields[19],"tinyblob1");
	assertEqStr($fields[20],"mediumblob1");
	assertEqStr($fields[21],"longblob1");
	echo("\n");


	// field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqInt($fieldlens[0],1);
	assertEqInt($fieldlens[1],1);
	assertEqInt($fieldlens[2],1);
	assertEqInt($fieldlens[3],1);
	assertEqInt($fieldlens[4],1);
	//assertEqInt($fieldlens[5],3);
	assertEqInt($fieldlens[6],3);
	assertEqInt($fieldlens[7],3);
	assertEqInt($fieldlens[8],10);
	assertEqInt($fieldlens[9],8);
	assertEqInt($fieldlens[10],19);
	assertEqInt($fieldlens[11],4);
	assertEqInt($fieldlens[12],5);
	assertEqInt($fieldlens[13],8);
	assertEqInt($fieldlens[14],5);
	assertEqInt($fieldlens[15],9);
	assertEqInt($fieldlens[16],11);
	assertEqInt($fieldlens[17],9);
	assertEqInt($fieldlens[18],5);
	assertEqInt($fieldlens[19],9);
	assertEqInt($fieldlens[20],11);
	assertEqInt($fieldlens[21],9);
	echo("\n");


	// result set buffer size
	echo("RESULT SET BUFFER SIZE: \n");
	assertEqInt(sqlrcur_getResultSetBufferSize($cur),0);
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqInt(sqlrcur_getResultSetBufferSize($cur),2);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),0);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),2);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,1,0),"2");
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),2);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),4);
	assertEqStr(sqlrcur_getField($cur,6,0),"7");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	// dont get column info
	echo("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqStr(sqlrcur_getColumnName($cur,0),NULL);
	assertEqInt(sqlrcur_getColumnLength($cur,0),0);
	assertEqStr(sqlrcur_getColumnType($cur,0),NULL);
	echo("\n");
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqStr(sqlrcur_getColumnName($cur,0),"testtinyint");
	assertEqInt(sqlrcur_getColumnLength($cur,0),1);
	assertEqStr(sqlrcur_getColumnType($cur,0),"TINYINT");
	echo("\n");


	// suspended session
	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,1,0),"2");
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	assertEqStr(sqlrcur_getField($cur,3,0),"4");
	assertEqStr(sqlrcur_getField($cur,4,0),"5");
	assertEqStr(sqlrcur_getField($cur,5,0),"6");
	assertEqStr(sqlrcur_getField($cur,6,0),"7");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,1,0),"2");
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	assertEqStr(sqlrcur_getField($cur,3,0),"4");
	assertEqStr(sqlrcur_getField($cur,4,0),"5");
	assertEqStr(sqlrcur_getField($cur,5,0),"6");
	assertEqStr(sqlrcur_getField($cur,6,0),"7");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,1,0),"2");
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	assertEqStr(sqlrcur_getField($cur,3,0),"4");
	assertEqStr(sqlrcur_getField($cur,4,0),"5");
	assertEqStr(sqlrcur_getField($cur,5,0),"6");
	assertEqStr(sqlrcur_getField($cur,6,0),"7");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	// suspended result set
	echo("SUSPENDED RESULT SET: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	$id=sqlrcur_getResultSetId($cur);
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	assertTrue(sqlrcur_resumeResultSet($cur,$id));
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),4);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),6);
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	// cached result set
	echo("CACHED RESULT SET: \n");
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	// column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount($cur),23);
	echo("\n");


	// column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testtinyint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testmediumint");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testbigint");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testreal");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testdecimal");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testdate");
	assertEqStr(sqlrcur_getColumnName($cur,9),"testtime");
	assertEqStr(sqlrcur_getColumnName($cur,10),"testdatetime");
	assertEqStr(sqlrcur_getColumnName($cur,11),"testyear");
	assertEqStr(sqlrcur_getColumnName($cur,12),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,13),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,14),"testtext");
	assertEqStr(sqlrcur_getColumnName($cur,15),"testtinytext");
	assertEqStr(sqlrcur_getColumnName($cur,16),"testmediumtext");
	assertEqStr(sqlrcur_getColumnName($cur,17),"testlongtext");
	assertEqStr(sqlrcur_getColumnName($cur,18),"testblob");
	assertEqStr(sqlrcur_getColumnName($cur,19),"testtinyblob");
	assertEqStr(sqlrcur_getColumnName($cur,20),"testmediumblob");
	assertEqStr(sqlrcur_getColumnName($cur,21),"testlongblob");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testtinyint");
	assertEqStr($cols[1],"testsmallint");
	assertEqStr($cols[2],"testmediumint");
	assertEqStr($cols[3],"testint");
	assertEqStr($cols[4],"testbigint");
	assertEqStr($cols[5],"testfloat");
	assertEqStr($cols[6],"testreal");
	assertEqStr($cols[7],"testdecimal");
	assertEqStr($cols[8],"testdate");
	assertEqStr($cols[9],"testtime");
	assertEqStr($cols[10],"testdatetime");
	assertEqStr($cols[11],"testyear");
	assertEqStr($cols[12],"testchar");
	assertEqStr($cols[13],"testvarchar");
	assertEqStr($cols[14],"testtext");
	assertEqStr($cols[15],"testtinytext");
	assertEqStr($cols[16],"testmediumtext");
	assertEqStr($cols[17],"testlongtext");
	assertEqStr($cols[18],"testblob");
	assertEqStr($cols[19],"testtinyblob");
	assertEqStr($cols[20],"testmediumblob");
	assertEqStr($cols[21],"testlongblob");
	echo("\n");


	// cached result set with result set
	// buffer size
	echo("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	// from one cache file to another
	echo("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");


	// from one cache file to another with
	// result set buffer size
	echo("FROM ONE CACHE FILE TO ANOTHER WITH RESULT ".
		"SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	// cached result set with suspend and
	// result set buffer size
	echo("CACHED RESULT SET WITH SUSPEND AND RESULT SET ".
		"BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	$id=sqlrcur_getResultSetId($cur);
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	echo("\n");
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	assertTrue(sqlrcur_resumeCachedResultSet($cur,$id,$filename));
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),4);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),6);
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	sqlrcur_cacheOff($cur);
	echo("\n");
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	// finished suspended session
	echo("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	assertEqStr(sqlrcur_getField($cur,4,0),"5");
	assertEqStr(sqlrcur_getField($cur,5,0),"6");
	assertEqStr(sqlrcur_getField($cur,6,0),"7");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	$id=sqlrcur_getResultSetId($cur);
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	assertTrue(sqlrcur_resumeResultSet($cur,$id));
	assertEqStr(sqlrcur_getField($cur,4,0),NULL);
	assertEqStr(sqlrcur_getField($cur,5,0),NULL);
	assertEqStr(sqlrcur_getField($cur,6,0),NULL);
	assertEqStr(sqlrcur_getField($cur,7,0),NULL);
	echo("\n");


	// nested selects
	echo("NESTED SELECTS: \n");
	// can't do this with mysql
	//sqlrcur_setResultSetBufferSize($cur,1);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	for ($i=0;sqlrcur_getRow($cur,$i);$i++) {
		$secondcur=sqlrcur_alloc($con);
		sqlrcur_setResultSetBufferSize($secondcur,1);
		assertTrue(sqlrcur_sendQuery($secondcur,"select * from ".
				"testtable"));
		sqlrcur_closeResultSet($secondcur);
	}
	//sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	// commit and rollback
	echo("COMMIT AND ROLLBACK: \n");
	// Note: Mysql's default isolation
	// level is repeatable-read, not
	// read-committed like most other
	// db's.  Both sessions must commit
	// to see the changes that each other
	// has made.
	$secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
		"testuser","testpassword",0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	assertTrue(sqlrcur_sendQuery(
			$secondcur,"select count(*) from testtable"));
	if ($majorversion>3) {
		assertEqStr(sqlrcur_getField($secondcur,0,0),"0");
	} else {
		assertEqStr(sqlrcur_getField($secondcur,0,0),"8");
	}
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcon_commit($secondcon));
	assertTrue(sqlrcur_sendQuery(
			$secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"8");
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcon_begin($secondcon));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	10, ".
		"	10, ".
		"	10, ".
		"	10, ".
		"	10, ".
		"	10.1, ".
		"	10.1, ".
		"	1.1, ".
		"	'2010-01-01', ".
		"	'10:00:00', ".
		"	'2010-01-01 10:00:00', ".
		"	'2010', ".
		"	'char10', ".
		"	'varchar10', ".
		"	'text10', ".
		"	'tinytext10', ".
		"	'mediumtext10', ".
		"	'longtext10', ".
		"	'blob10', ".
		"	'tinyblob10', ".
		"	'mediumblob10', ".
		"	'longblob10', ".
		"	NULL)"));
	assertTrue(sqlrcon_rollback($con));
	assertTrue(sqlrcon_commit($secondcon));
	assertTrue(sqlrcur_sendQuery(
			$secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"8");
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	10, ".
		"	10, ".
		"	10, ".
		"	10, ".
		"	10, ".
		"	10.1, ".
		"	10.1, ".
		"	1.1, ".
		"	'2010-01-01', ".
		"	'10:00:00', ".
		"	'2010-01-01 10:00:00', ".
		"	'2010', ".
		"	'char10', ".
		"	'varchar10', ".
		"	'text10', ".
		"	'tinytext10', ".
		"	'mediumtext10', ".
		"	'longtext10', ".
		"	'blob10', ".
		"	'tinyblob10', ".
		"	'mediumblob10', ".
		"	'longblob10', ".
		"	NULL)"));
	assertTrue(sqlrcon_commit($secondcon));
	assertTrue(sqlrcur_sendQuery(
			$secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"9");
	sqlrcon_endSession($secondcon);
	sqlrcon_commit($secondcon);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select \$(var1),'\$(var2)',\$(var3)");
	sqlrcur_substitution($cur,"var1",1);
	sqlrcur_substitution($cur,"var2","hello");
	sqlrcur_substitution($cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	// array substitutions
	echo("ARRAY SUBSTITUTIONS: \n");
	$subvars=array("var1","var2","var3");
	sqlrcur_prepareQuery($cur,"select \$(var1),\$(var2),\$(var3)");
	$subvallongs=array(1,2,3);
	$precs=array(0,0,0);
	$scales=array(0,0,0);
	sqlrcur_substitutions($cur,$subvars,$subvallongs,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"2");
	assertEqStr(sqlrcur_getField($cur,0,2),"3");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select '\$(var1)','\$(var2)','\$(var3)'");
	$subvalstrings=array("hi","hello","bye");
	sqlrcur_substitutions($cur,$subvars,$subvalstrings,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"hi");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"bye");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select \$(var1),\$(var2),\$(var3)");
	$subvaldoubles=array(10.55,10.556,10.5556);
	$precs=array(4,5,6);
	$scales=array(2,3,4);
	sqlrcur_substitutions($cur,$subvars,$subvaldoubles,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"10.55");
	assertEqStr(sqlrcur_getField($cur,0,1),"10.556");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	// nulls as nulls
	echo("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL"));
	assertEqStr(sqlrcur_getField($cur,0,0),NULL);
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL"));
	assertEqStr(sqlrcur_getField($cur,0,0),"");
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),"");
	echo("\n");


	// null and empty lobs
	echo("NULL AND EMPTY LOBS: \n");
	sqlrcur_getNullsAsNulls($cur);
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob1 longtext, ".
		"	testclob2 longtext, ".
		"	testblob1 longblob, ".
		"	testblob2 longblob)"));
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	?, ".
		"	?, ".
		"	?, ".
		"	?)");
	sqlrcur_inputBindClob($cur,"1","",strlen(""));
	sqlrcur_inputBindClob($cur,"2",NULL,0);
	sqlrcur_inputBindBlob($cur,"3","",strlen(""));
	sqlrcur_inputBindBlob($cur,"4",NULL,0);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqStr(sqlrcur_getField($cur,0,0),"");
	assertEqStr(sqlrcur_getField($cur,0,1),NULL);
	assertEqStr(sqlrcur_getField($cur,0,2),"");
	assertEqStr(sqlrcur_getField($cur,0,3),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// long lobs
	echo("LONG LOBS: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testtext longtext, ".
		"	testblob longblob)");
	sqlrcur_prepareQuery($cur,"insert into testtable values (?,?)");
	$largebuffer=str_repeat("C",8192);
	sqlrcur_inputBindClob($cur,"1",$largebuffer,strlen($largebuffer));
	sqlrcur_inputBindBlob($cur,"2",$largebuffer,strlen($largebuffer));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtext"),
		8192);
	assertEqStr(sqlrcur_getField($cur,0,"testtext"),$largebuffer);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testblob"),
		8192);
	assertEqStr(sqlrcur_getField($cur,0,"testblob"),$largebuffer);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


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
	echo("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,"create table testtable (testval int)");
	sqlrcur_prepareQuery($cur,"insert into testtable values (?)");
	sqlrcur_inputBind($cur,"1",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable");
	assertEqStr(sqlrcur_getField($cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// bind validation
	// mysql doesn't support bind by name


	// rebinding
	echo("REBINDING: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in in1 int) ".
		"begin ".
		"	select in1; end"));
	sqlrcur_prepareQuery($cur,"call testproc(?)");
	sqlrcur_inputBind($cur,"1",1);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcur_inputBind($cur,"1",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"2");
	sqlrcur_inputBind($cur,"1",3);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"3");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	// reexecute
	echo("REEXECUTE: \n");
	sqlrcur_prepareQuery($cur,"select 1");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select ?");
	sqlrcur_inputBind($cur,"1",1);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	sqlrcur_inputBind($cur,"1",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"2");
	echo("\n");


	// stored procedure returning no value
	echo("STORED PROCEDURE RETURNING NO VALUE: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in in1 int, ".
		"	in in2 double, ".
		"	in in3 varchar(20)) begin end"));
	sqlrcur_prepareQuery($cur,"call testproc(?,?,?)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	// stored procedure returning single
	// value
	echo("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in in1 int, ".
		"	in in2 double, ".
		"	in in3 varchar(20)) ".
		"begin ".
		"	select in1; end"));
	sqlrcur_prepareQuery($cur,"call testproc(?,?,?)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	// stored procedure returning multiple
	// values
	echo("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in in1 int, ".
		"	in in2 double, ".
		"	in in3 varchar(20)) ".
		"begin ".
		"	select in1, in2, ".
		"	in3; end"));
	sqlrcur_prepareQuery($cur,"call testproc(?,?,?)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,2),"hello");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	// stored procedure returning result
	// set
	echo("STORED PROCEDURE RETURNING RESULT SET: \n");
	sqlrcur_sendQuery($cur,"drop procedure testselectproc");
	assertTrue(sqlrcur_sendQuery($cur,"create procedure testselectproc() ".
		"begin ".
		"	select 1 ".
		"	union ".
		"	select 2 ".
		"	union ".
		"	select 3 ".
		"	union ".
		"	select 4 ".
		"	union ".
		"	select 5 ".
		"	union ".
		"	select 6 ".
		"	union ".
		"	select 7 ".
		"	union ".
		"	select 8; end"));
	assertTrue(sqlrcur_sendQuery($cur,"call testselectproc()"));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testselectproc"));
	echo("\n");


	// temporary tables
	echo("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery($cur,"drop table temptable");
	sqlrcur_sendQuery($cur,"create temporary table temptable (col1 int)");
	assertTrue(sqlrcur_sendQuery($cur,"insert into temptable values (1)"));
	assertTrue(sqlrcur_sendQuery($cur,"select count(*) from temptable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcon_endSession($con);
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"select count(*) from temptable"));
	echo("\n");

	if ($majorversion>3) {

		// stored procedure returning
		// no value
		echo("STORED PROCEDURE RETURNING NO VALUE: \n");
		sqlrcur_sendQuery($cur,"drop procedure if exists testproc");
		assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
			"testproc(".
			"	in in1 int, ".
			"	in in2 float, ".
			"	in in3 ".
			"	char(20)) ".
			"begin ".
			"	select in1, ".
			"	in2, in3; end;"));
		sqlrcur_prepareQuery($cur,"call testproc(?,?,?)");
		sqlrcur_inputBind($cur,"1",1);
		sqlrcur_inputBind($cur,"2",1.1,4,2);
		sqlrcur_inputBind($cur,"3","hello");
		assertTrue(sqlrcur_executeQuery($cur));
		assertEqStr(sqlrcur_getField($cur,0,0),"1");
		assertEqStr(sqlrcur_getField($cur,0,1),"1.1");
		assertEqStr(sqlrcur_getField($cur,0,2),"hello");
		sqlrcur_sendQuery($cur,"drop procedure testproc");
		echo("\n");


		// stored procedure returning
		// one value
		echo("FUNCTIONS: \n");
		sqlrcur_sendQuery($cur,"drop function if exists testfunc");
		assertTrue(sqlrcur_sendQuery($cur,"create function testfunc(".
			"in1 int, in2 ".
			"	int) returns int return in1+in2;"));
		sqlrcur_prepareQuery($cur,"select testfunc(?,?)");
		sqlrcur_inputBind($cur,"1",10);
		sqlrcur_inputBind($cur,"2",20);
		assertTrue(sqlrcur_executeQuery($cur));
		assertEqStr(sqlrcur_getField($cur,0,0),"30");
		sqlrcur_sendQuery($cur,"drop function if exists testfunc");
		echo("\n");


		// stored procedure returning
		// multiple values
		echo("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
		sqlrcur_sendQuery($cur,"drop procedure if exists testproc");
		assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
			"testproc(".
			"	out out1 int, ".
			"	out out2 float,".
			"	out out3 ".
			"	char(20)) ".
			"begin ".
			"	select 1, 1.1,".
			"	'hello' ".
			"	into out1, ".
			"	out2, out3; end;"));
		assertTrue(sqlrcur_sendQuery($cur,"set @out1=0, @out2=0.0, ".
			"@out3=''"));
		assertTrue(sqlrcur_sendQuery($cur,"call testproc(@out1,@out2,".
			"@out3)"));
		assertTrue(sqlrcur_sendQuery($cur,"select @out1, ".
			"@out2, @out3"));
		assertEqStr(sqlrcur_getField($cur,0,0),"1");
		//assertEqDbl(sqlrcur_getFieldAsDouble($cur,0,1),1.1);
		assertEqStr(sqlrcur_getField($cur,0,2),"hello");
		sqlrcur_sendQuery($cur,"drop procedure testproc");
		echo("\n");


		// stored procedure returning
		// result set
		echo("STORED PROCEDURE RETURNING RESULT SET: \n");
		sqlrcur_sendQuery($cur,"drop procedure if exists ".
			"testselectproc");
		assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
			"testselectproc() ".
			"begin ".
			"	select 1 ".
			"	union ".
			"	select 2 ".
			"	union ".
			"	select 3 ".
			"	union ".
			"	select 4 ".
			"	union ".
			"	select 5 ".
			"	union ".
			"	select 6 ".
			"	union ".
			"	select 7 ".
			"	union ".
			"	select 8; end"));
		assertTrue(sqlrcur_sendQuery($cur,"call testselectproc()"));
		assertEqInt(sqlrcur_rowCount($cur),8);
		sqlrcur_sendQuery($cur,"drop procedure testselectproc");
		echo("\n");
	}


	if ($majorversion>3) {

		// encoded binary data -
		// all chars - \-escaped
		echo("ENCODED BINARY DATA - all chars - \\-escaped: \n");
		sqlrcur_sendQuery($cur,"drop table testtable");
		assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
			"(col1 longblob)"));
		$buffer="";
		for ($i=0; $i<256; $i++) {
			$buffer.=chr($i);
		}
		$query="insert into testtable values (_binary'";
		for ($i=0;$i<strlen($buffer);$i++) {
			$c=$buffer[$i];
			if ($c=="'") {
				$query.="\\";
			}
			if ($c=="\\") {
				$query.="\\";
			}
			$query.=$c;
		}
		$query.="')";
		assertTrue(sqlrcur_sendQueryWithLength($cur,$query,strlen($query)));
		assertTrue(sqlrcur_sendQuery($cur,"select col1 ".
			"from testtable"));
		assertEqInt(sqlrcur_getFieldLength($cur,0,0),
		    strlen($buffer));
		assertEqStrLen(sqlrcur_getField($cur,0,0),$buffer,
		    strlen($buffer));
		assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
		echo("\n");


		// encoded binary data -
		// (null)"" - unescaped
		echo("ENCODED BINARY DATA - (null)\"\" - unescaped: \n");
		sqlrcur_sendQuery($cur,"drop table testtable");
		assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
			"(col1 longblob)"));
		assertTrue(sqlrcur_sendQueryWithLength($cur,"insert into ".
			"testtable values (_binary'\0\"\"')",43));
		assertTrue(sqlrcur_sendQuery($cur,"select col1 ".
			"from testtable"));
		assertEqInt(sqlrcur_getFieldLength($cur,0,0),3);
		assertEqStrLen(sqlrcur_getField($cur,0,0),
		    "\0\"\"",3);
		assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
		echo("\n");


		// encoded binary data -
		// (null)"" - \-escaped
		echo("ENCODED BINARY DATA - \\(null)\\\"\\\" - ".
			"\\-escaped: \n");
		sqlrcur_sendQuery($cur,"drop table testtable");
		assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
			"(col1 longblob)"));
		assertTrue(sqlrcur_sendQueryWithLength($cur,"insert into ".
			"testtable values (_binary'\\\0\\\"\\\"')",46));
		assertTrue(sqlrcur_sendQuery($cur,"select col1 ".
			"from testtable"));
		assertEqInt(sqlrcur_getFieldLength($cur,0,0),3);
		assertEqStrLen(sqlrcur_getField($cur,0,0),
		    "\0\"\"",3);
		assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
		echo("\n");
	}


	// quotes - '' - ''-escaped
	echo("QUOTES - '' - ''-escaped: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values ('''''')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqStr(sqlrcur_getField($cur,0,0),"''");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// quotes - '' - '',\-escaped
	echo("QUOTES - '' - '',\\-escaped: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values ('''\\'')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqStr(sqlrcur_getField($cur,0,0),"''");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// quotes - '' - \,''-escaped
	echo("QUOTES - '' - \\,''-escaped: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values ('\\'''')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqStr(sqlrcur_getField($cur,0,0),"''");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// quotes - \\' - \-escaped
	echo("QUOTES - \\\\' - \\-escaped: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values ('\\\\\\'')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqStrLen(sqlrcur_getField($cur,0,0),"\\'",2);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// quotes - "" - unescaped
	echo("QUOTES - \"\" - unescaped: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values ('\"\"')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqStr(sqlrcur_getField($cur,0,0),"\"\"");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// quotes - random - '',\-escaped
	echo("QUOTES - random - '',\\-escaped: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(512))"));
	$ch=array("'","\"","\\","\0");
	srand(time());
	$seed1=rand();
	$seed2=rand();
	srand($seed1);
	$buffer="";
	for ($i=0; $i<256; $i++) {
		$result1=rand();
		srand($result1);
		$buffer.=$ch[$result1%4];
	}
	$query="insert into testtable values ('";
	srand($seed2);
	for ($i=0; $i<strlen($buffer); $i++) {
		$result2=rand();
		srand($result2);
		if ($buffer[$i]=="'") {
			// randomly escape
			// with \ or ''
			if ($result2%2) {
				$query.="'";
			} else {
				$query.="\\";
			}
		}
		if ($buffer[$i]=="\"") {
			// randomly escape
			// with \ or don't
			if ($result2%2) {
				$query.="\\";
			}
		}
		if ($buffer[$i]=="\\") {
			// escape with
			// backslash
			$query.="\\";
		}
		$query.=$buffer[$i];
	}
	$query.="')";
	assertTrue(sqlrcur_sendQueryWithLength($cur,$query,strlen($query)));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),strlen($buffer));
	assertEqStrLen(sqlrcur_getField($cur,0,0),
		$buffer,strlen($buffer));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// last insert id
	echo("LAST INSERT ID: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable ".
		"	(col1 int primary key".
		"	auto_increment, ".
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values (null,1)"));
	assertEqInt(sqlrcon_getLastInsertId($con),1);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// database is schema
	echo("DATABASE IS SCHEMA: \n");
	assertTrue(sqlrcon_getDatabaseIsSchema($con));
	echo("\n");


	// catalog list
	echo("CATALOG LIST: \n");
	assertTrue(sqlrcur_getCatalogList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	assertTrue(sqlrcur_rowCount($cur)>0);
	echo("\n");


	// schema list
	echo("SCHEMA LIST: \n");
	assertTrue(sqlrcur_getSchemaList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	assertTrue(sqlrcur_rowCount($cur)>0);
	echo("\n");


	// table type list
	echo("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList($cur));
	assertEqStr(sqlrcur_getColumnName($cur,0),"table_type");
	$found=0;
	for ($i=0;$i<sqlrcur_rowCount($cur);$i++) {
		if (!strcmp(sqlrcur_getField($cur,$i,"table_type"),
			"TABLE")) {
			$found=1;
			break;
		}
	}
	assertTrue($found);
	echo("\n");


	// table list
	echo("TABLE LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable1");
	sqlrcur_sendQuery($cur,"drop table testtable2");
	sqlrcur_sendQuery($cur,"drop table testtable3");
	sqlrcur_sendQuery($cur,"drop table testtable4");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable1 (".
		"	col1 int, ".
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable2 (".
		"	col1 int, ".
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable3 (".
		"	col1 int, ".
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable4 (".
		"	col1 int, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getTableList($cur,NULL));
	$counter=0;
	for ($i=0;$i<sqlrcur_rowCount($cur);$i++) {
		$name=sqlrcur_getField($cur,$i,"Tables_in_xxx");
		if (!strcmp($name,"testtable1") ||!strcmp($name,
			"testtable2") ||!strcmp($name,
			"testtable3") ||!strcmp($name,"testtable4")) {
			$counter++;
		}
	}
	assertEqInt($counter,4);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable4"));
	echo("\n");


	// type info list
	echo("TYPE INFO LIST: \n");
	assertTrue(sqlrcur_getTypeInfoList($cur,"int"));
	assertEqStr(sqlrcur_getColumnName($cur,0),"type_name");
	assertEqStr(sqlrcur_getColumnName($cur,1),"data_type");
	assertEqStr(sqlrcur_getColumnName($cur,2),"precision");
	assertEqStr(sqlrcur_getColumnName($cur,3),"literal_prefix");
	assertEqStr(sqlrcur_getColumnName($cur,4),"literal_suffix");
	assertEqStr(sqlrcur_getColumnName($cur,5),"create_params");
	assertEqStr(sqlrcur_getColumnName($cur,6),"nullable");
	assertEqStr(sqlrcur_getColumnName($cur,7),"case_sensitive");
	assertEqStr(sqlrcur_getColumnName($cur,8),"searchable");
	assertEqStr(sqlrcur_getColumnName($cur,9),"unsigned_attribute");
	assertEqStr(sqlrcur_getColumnName($cur,10),"fixed_prec_scale");
	assertEqStr(sqlrcur_getColumnName($cur,11),"auto_increment");
	assertEqStr(sqlrcur_getColumnName($cur,12),"local_type_name");
	assertEqStr(sqlrcur_getColumnName($cur,13),"minumum_scale");
	assertEqStr(sqlrcur_getColumnName($cur,14),"maxiumm_scale");
	assertEqStr(sqlrcur_getColumnName($cur,15),"sql_data_type");
	assertEqStr(sqlrcur_getColumnName($cur,16),"sql_datetime_sub");
	assertEqStr(sqlrcur_getColumnName($cur,17),"num_prec_radix");
	assertEqStr(sqlrcur_getColumnName($cur,18),"interval_precision");
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"INT");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"4");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"INT");
	assertTrue(sqlrcur_getTypeInfoList($cur,"char"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"255");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"varchar"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"65535");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"VARCHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"date"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"91");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"DATE");
	echo("\n");


	// column list
	echo("COLUMN LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testtinyint tinyint, ".
		"	testsmallint smallint,".
		"	testmediumint ".
		"	mediumint, ".
		"	testint int, ".
		"	testbigint bigint, ".
		"	testfloat float, ".
		"	testreal real, ".
		"	testdecimal ".
		"	decimal(2,1), ".
		"	testdate date, ".
		"	testtime time, ".
		"	testdatetime ".
		"	datetime, ".
		"	testyear year, ".
		"	testchar char(40), ".
		"	testvarchar ".
		"	varchar(40), ".
		"	testtext text, ".
		"	testtinytext ".
		"	tinytext, ".
		"	testmediumtext ".
		"	mediumtext, ".
		"	testlongtext ".
		"	longtext, ".
		"	testblob blob, ".
		"	testtinyblob ".
		"	tinyblob, ".
		"	testmediumblob ".
		"	mediumblob, ".
		"	testlongblob ".
		"	longblob, ".
		"	testtimestamp ".
		"	timestamp)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"column_name");
	assertEqStr(sqlrcur_getColumnName($cur,1),"data_type");
	assertEqStr(sqlrcur_getColumnName($cur,2),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName($cur,3),"numeric_precision");
	assertEqStr(sqlrcur_getColumnName($cur,4),"numeric_scale");
	assertEqStr(sqlrcur_getColumnName($cur,5),"is_nullable");
	assertEqStr(sqlrcur_getColumnName($cur,6),"column_key");
	assertEqStr(sqlrcur_getColumnName($cur,7),"column_default");
	assertEqStr(sqlrcur_getColumnName($cur,8),"extra");
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),"testtinyint");
	assertEqStr(sqlrcur_getField($cur,1,"column_name"),"testsmallint");
	assertEqStr(sqlrcur_getField($cur,2,"column_name"),
		"testmediumint");
	assertEqStr(sqlrcur_getField($cur,3,"column_name"),"testint");
	assertEqStr(sqlrcur_getField($cur,4,"column_name"),"testbigint");
	assertEqStr(sqlrcur_getField($cur,5,"column_name"),"testfloat");
	assertEqStr(sqlrcur_getField($cur,6,"column_name"),"testreal");
	assertEqStr(sqlrcur_getField($cur,7,"column_name"),"testdecimal");
	assertEqStr(sqlrcur_getField($cur,8,"column_name"),"testdate");
	assertEqStr(sqlrcur_getField($cur,9,"column_name"),"testtime");
	assertEqStr(sqlrcur_getField($cur,10,"column_name"),
		"testdatetime");
	assertEqStr(sqlrcur_getField($cur,11,"column_name"),"testyear");
	assertEqStr(sqlrcur_getField($cur,12,"column_name"),"testchar");
	assertEqStr(sqlrcur_getField($cur,13,"column_name"),"testvarchar");
	assertEqStr(sqlrcur_getField($cur,14,"column_name"),"testtext");
	assertEqStr(sqlrcur_getField($cur,15,"column_name"),
		"testtinytext");
	assertEqStr(sqlrcur_getField($cur,16,"column_name"),
		"testmediumtext");
	assertEqStr(sqlrcur_getField($cur,17,"column_name"),
		"testlongtext");
	assertEqStr(sqlrcur_getField($cur,18,"column_name"),"testblob");
	assertEqStr(sqlrcur_getField($cur,19,"column_name"),
		"testtinyblob");
	assertEqStr(sqlrcur_getField($cur,20,"column_name"),
		"testmediumblob");
	assertEqStr(sqlrcur_getField($cur,21,"column_name"),
		"testlongblob");
	assertEqStr(sqlrcur_getField($cur,22,"column_name"),
		"testtimestamp");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"TINYINT");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"SMALLINT");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"MEDIUMINT");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"INT");
	assertEqStr(sqlrcur_getField($cur,4,"data_type"),"BIGINT");
	assertEqStr(sqlrcur_getField($cur,5,"data_type"),"FLOAT");
	// not "REAL"
	assertEqStr(sqlrcur_getField($cur,6,"data_type"),"DOUBLE");
	assertEqStr(sqlrcur_getField($cur,7,"data_type"),"DECIMAL");
	assertEqStr(sqlrcur_getField($cur,8,"data_type"),"DATE");
	assertEqStr(sqlrcur_getField($cur,9,"data_type"),"TIME");
	assertEqStr(sqlrcur_getField($cur,10,"data_type"),"DATETIME");
	assertEqStr(sqlrcur_getField($cur,11,"data_type"),"YEAR");
	assertEqStr(sqlrcur_getField($cur,12,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,13,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,14,"data_type"),"TEXT");
	assertEqStr(sqlrcur_getField($cur,15,"data_type"),"TINYTEXT");
	assertEqStr(sqlrcur_getField($cur,16,"data_type"),"MEDIUMTEXT");
	assertEqStr(sqlrcur_getField($cur,17,"data_type"),"LONGTEXT");
	assertEqStr(sqlrcur_getField($cur,18,"data_type"),"BLOB");
	assertEqStr(sqlrcur_getField($cur,19,"data_type"),"TINYBLOB");
	assertEqStr(sqlrcur_getField($cur,20,"data_type"),"MEDIUMBLOB");
	assertEqStr(sqlrcur_getField($cur,21,"data_type"),"LONGBLOB");
	assertEqStr(sqlrcur_getField($cur,22,"data_type"),"TIMESTAMP");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// column list - auto_increment,
	// primary key
	echo("COLUMN LIST - auto_increment, primary key: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int ".
		"	auto_increment ".
		"	primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertTrue(strstr(sqlrcur_getField($cur,0,"extra"),
		"auto_increment")!==FALSE);
	assertTrue(strstr(sqlrcur_getField($cur,0,"column_key"),
		"PRI")!==FALSE);
	assertFalse(strstr(sqlrcur_getField($cur,1,"extra"),
		"auto_increment")!==FALSE);
	assertFalse(strstr(sqlrcur_getField($cur,1,"column_key"),
		"PRI")!==FALSE);
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int ".
		"	primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertFalse(strstr(sqlrcur_getField($cur,0,"extra"),
		"auto_increment")!==FALSE);
	assertTrue(strstr(sqlrcur_getField($cur,0,"column_key"),
		"PRI")!==FALSE);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// primary keys list
	echo("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int ".
		"	primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getPrimaryKeysList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"table");
	assertEqStr(sqlrcur_getColumnName($cur,1),"non_unique");
	assertEqStr(sqlrcur_getColumnName($cur,2),"key_name");
	assertEqStr(sqlrcur_getColumnName($cur,3),"seq_in_index");
	assertEqStr(sqlrcur_getColumnName($cur,4),"column_name");
	assertEqStr(sqlrcur_getColumnName($cur,5),"collation");
	assertEqStr(sqlrcur_getColumnName($cur,6),"cardinality");
	assertEqStr(sqlrcur_getColumnName($cur,7),"sub_part");
	assertEqStr(sqlrcur_getColumnName($cur,8),"packed");
	assertEqStr(sqlrcur_getColumnName($cur,9),"null");
	assertEqStr(sqlrcur_getColumnName($cur,10),"index_type");
	assertEqStr(sqlrcur_getColumnName($cur,11),"comment");
	assertEqStr(sqlrcur_getColumnName($cur,12),"index_comment");
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"table"),"testtable"));
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"column_name"),"col1"));
	assertEqStr(sqlrcur_getField($cur,0,"key_name"),"PRIMARY");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// key and index list
	echo("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int ".
		"	primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getKeyAndIndexList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"table");
	assertEqStr(sqlrcur_getColumnName($cur,1),"non_unique");
	assertEqStr(sqlrcur_getColumnName($cur,2),"key_name");
	assertEqStr(sqlrcur_getColumnName($cur,3),"seq_in_index");
	assertEqStr(sqlrcur_getColumnName($cur,4),"column_name");
	assertEqStr(sqlrcur_getColumnName($cur,5),"collation");
	assertEqStr(sqlrcur_getColumnName($cur,6),"cardinality");
	assertEqStr(sqlrcur_getColumnName($cur,7),"sub_part");
	assertEqStr(sqlrcur_getColumnName($cur,8),"packed");
	assertEqStr(sqlrcur_getColumnName($cur,9),"null");
	assertEqStr(sqlrcur_getColumnName($cur,10),"index_type");
	assertEqStr(sqlrcur_getColumnName($cur,11),"comment");
	assertEqStr(sqlrcur_getColumnName($cur,12),"index_comment");
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"table"),"testtable"));
	assertEqStr(sqlrcur_getField($cur,0,"non_unique"),"false");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"column_name"),"col1"));
	assertEqStr(sqlrcur_getField($cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getField($cur,0,"index_type"),"3");
	assertEqStr(sqlrcur_getField($cur,0,"key_name"),"PRIMARY");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	// procedure list
	echo("PROCEDURE LIST: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc1");
	sqlrcur_sendQuery($cur,"drop procedure testproc2");
	sqlrcur_sendQuery($cur,"drop procedure testproc3");
	sqlrcur_sendQuery($cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
		"testproc1(".
		"	in in1 int, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) begin end"));
	assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
		"testproc2(".
		"	in in1 int, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) begin end"));
	assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
		"testproc3(".
		"	in in1 int, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) begin end"));
	assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
		"testproc4(".
		"	in in1 int, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) begin end"));
	assertTrue(sqlrcur_getProcedureList($cur,NULL));
	$counter=0;
	for ($i=0;$i<sqlrcur_rowCount($cur);$i++) {
		$name=sqlrcur_getField($cur,$i,"routine_name");
		if (!strcmp($name,"testproc1") ||!strcmp($name,
			"testproc2") ||!strcmp($name,"testproc3") ||!strcmp($name,
			"testproc4")) {
			$counter++;
		}
	}
	assertEqInt($counter,4);
	echo("\n");


	// procedure parameter list
	echo("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList($cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName($cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName($cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName($cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName($cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount($cur),4);
	assertEqStr(sqlrcur_getField($cur,0,"parameter_name"),"in1");
	assertEqStr(sqlrcur_getField($cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"INT");
	assertEqStr(sqlrcur_getField($cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_name"),"in2");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_name"),"in3");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_name"),"in4");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"DATE");
	assertEqStr(sqlrcur_getField($cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc4"));
	echo("\n");


	// invalid queries
	echo("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values (1,2,3,4)"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	echo("\n");

	reportTestStatus();

	exit($status);
?></pre></html>
