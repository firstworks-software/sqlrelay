<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.
	include("./asserts.php");


	try {

	$host="sqlrelay";
	$port=9000;
	$socket="/tmp/test.socket";
	$user="testuser";
	$password="testpassword";

	# instantiation
	$con=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$cur=sqlrcur_alloc($con);

	# get database type


	# identify
	echo("IDENTIFY: \n");
	assertEqual(sqlrcon_identify($con),"mysql");
	echo("\n");

	# get the db version
	$dbversion=sqlrcon_dbVersion($con);
	$majorversion=intval(substr($dbversion,0,1));


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	# isolation levels
	echo("ISOLATION LEVELS: \n");
	$isolationlevels=array(
			"REPEATABLE-READ",
			"READ-UNCOMMITTED",
			"READ-COMMITTED",
			"SERIALIZABLE");
	foreach ($isolationlevels as $il) {
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		assertEqual(sqlrcon_getIsolationLevel($con),$il);
		echo("\n");
	}
	# reset to the default isolation level
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	echo("\n");

	# drop existing table
	sqlrcur_sendQuery($cur,"drop table testtable");

	# create a new table


	# create temptable
	echo("CREATE TEMPTABLE: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testtinyint tinyint, ".
		"	testsmallint smallint, ".
		"	testmediumint mediumint, ".
		"	testint int, ".
		"	testbigint bigint, ".
		"	testfloat float, ".
		"	testreal real, ".
		"	testdecimal decimal(2,1), ".
		"	testdate date, ".
		"	testtime time, ".
		"	testdatetime datetime, ".
		"	testyear year, ".
		"	testchar char(40), ".
		"	testtext text, ".
		"	testvarchar varchar(40), ".
		"	testtinytext tinytext, ".
		"	testmediumtext mediumtext, ".
		"	testlongtext longtext, ".
		"	testtimestamp timestamp)"));
	echo("\n");


	# begin transaction
	echo("BEGIN TRANSACTION: \n");
	assertTrue(sqlrcur_sendQuery($cur,"begin"));
	echo("\n");


	# insert
	echo("INSERT: \n");
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
		"	'text1', ".
		"	'varchar1', ".
		"	'tinytext1', ".
		"	'mediumtext1', ".
		"	'longtext1', ".
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
		"	'text2', ".
		"	'varchar2', ".
		"	'tinytext2', ".
		"	'mediumtext2', ".
		"	'longtext2', ".
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
		"	'text3', ".
		"	'varchar3', ".
		"	'tinytext3', ".
		"	'mediumtext3', ".
		"	'longtext3', ".
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
		"	'text4', ".
		"	'varchar4', ".
		"	'tinytext4', ".
		"	'mediumtext4', ".
		"	'longtext4', ".
		"	NULL)"));
	echo("\n");


	# affected rows
	echo("AFFECTED ROWS: \n");
	assertEqual(sqlrcur_affectedRows($cur),1);
	echo("\n");


	# bind by position
	echo("BIND BY POSITION: \n");
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
		"	NULL)");
	assertEqual(sqlrcur_countBindVariables($cur),18);
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
	sqlrcur_inputBind($cur,"11","2005-01-01 05:00:00");
	sqlrcur_inputBind($cur,"12","2005");
	sqlrcur_inputBind($cur,"13","char5");
	sqlrcur_inputBind($cur,"14","text5");
	sqlrcur_inputBind($cur,"15","varchar5");
	sqlrcur_inputBind($cur,"16","tinytext5");
	sqlrcur_inputBind($cur,"17","mediumtext5");
	sqlrcur_inputBind($cur,"18","longtext5");
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
	sqlrcur_inputBind($cur,"11","2006-01-01 06:00:00");
	sqlrcur_inputBind($cur,"12","2006");
	sqlrcur_inputBind($cur,"13","char6");
	sqlrcur_inputBind($cur,"14","text6");
	sqlrcur_inputBind($cur,"15","varchar6");
	sqlrcur_inputBind($cur,"16","tinytext6");
	sqlrcur_inputBind($cur,"17","mediumtext6");
	sqlrcur_inputBind($cur,"18","longtext6");
	assertTrue(sqlrcur_executeQuery($cur));


	# array bind by position
	echo("ARRAY BIND BY POSITION: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("1","2","3","4","5","6",
			"7","8","9","10","11","12",
			"13","14","15","16","17","18");
	$bindvals=array(7,7,7,7,7,7.1,7.1,7.1,
			"2007-01-01","07:00:00",
			"2007-01-01 07:00:00",
			"2007","char7","text7",
			"varchar7","tinytext7",
			"mediumtext7","longtext7");
	$precs=array(0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0);
	$scales=array(0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0);
	sqlrcur_inputBinds($cur,$bindvars,$bindvals,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# bind by position with validation
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
	sqlrcur_inputBind($cur,"11","2008-01-01 08:00:00");
	sqlrcur_inputBind($cur,"12","2008");
	sqlrcur_inputBind($cur,"13","char8");
	sqlrcur_inputBind($cur,"14","text8");
	sqlrcur_inputBind($cur,"15","varchar8");
	sqlrcur_inputBind($cur,"16","tinytext8");
	sqlrcur_inputBind($cur,"17","mediumtext8");
	sqlrcur_inputBind($cur,"18","longtext8");
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# select
	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqual(sqlrcur_colCount($cur),19);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testtinyint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqual(sqlrcur_getColumnName($cur,2),"testmediumint");
	assertEqual(sqlrcur_getColumnName($cur,3),"testint");
	assertEqual(sqlrcur_getColumnName($cur,4),"testbigint");
	assertEqual(sqlrcur_getColumnName($cur,5),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,6),"testreal");
	assertEqual(sqlrcur_getColumnName($cur,7),"testdecimal");
	assertEqual(sqlrcur_getColumnName($cur,8),"testdate");
	assertEqual(sqlrcur_getColumnName($cur,9),"testtime");
	assertEqual(sqlrcur_getColumnName($cur,10),"testdatetime");
	assertEqual(sqlrcur_getColumnName($cur,11),"testyear");
	assertEqual(sqlrcur_getColumnName($cur,12),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,13),"testtext");
	assertEqual(sqlrcur_getColumnName($cur,14),"testvarchar");
	assertEqual(sqlrcur_getColumnName($cur,15),"testtinytext");
	assertEqual(sqlrcur_getColumnName($cur,16),"testmediumtext");
	assertEqual(sqlrcur_getColumnName($cur,17),"testlongtext");
	assertEqual(sqlrcur_getColumnName($cur,18),"testtimestamp");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testtinyint");
	assertEqual($cols[1],"testsmallint");
	assertEqual($cols[2],"testmediumint");
	assertEqual($cols[3],"testint");
	assertEqual($cols[4],"testbigint");
	assertEqual($cols[5],"testfloat");
	assertEqual($cols[6],"testreal");
	assertEqual($cols[7],"testdecimal");
	assertEqual($cols[8],"testdate");
	assertEqual($cols[9],"testtime");
	assertEqual($cols[10],"testdatetime");
	assertEqual($cols[11],"testyear");
	assertEqual($cols[12],"testchar");
	assertEqual($cols[13],"testtext");
	assertEqual($cols[14],"testvarchar");
	assertEqual($cols[15],"testtinytext");
	assertEqual($cols[16],"testmediumtext");
	assertEqual($cols[17],"testlongtext");
	assertEqual($cols[18],"testtimestamp");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqual(sqlrcur_getColumnType($cur,0),"TINYINT");
	assertEqual(sqlrcur_getColumnType($cur,1),"SMALLINT");
	assertEqual(sqlrcur_getColumnType($cur,2),"MEDIUMINT");
	assertEqual(sqlrcur_getColumnType($cur,3),"INT");
	assertEqual(sqlrcur_getColumnType($cur,4),"BIGINT");
	assertEqual(sqlrcur_getColumnType($cur,5),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,6),"REAL");
	assertEqual(sqlrcur_getColumnType($cur,7),"DECIMAL");
	assertEqual(sqlrcur_getColumnType($cur,8),"DATE");
	assertEqual(sqlrcur_getColumnType($cur,9),"TIME");
	assertEqual(sqlrcur_getColumnType($cur,10),"DATETIME");
	assertEqual(sqlrcur_getColumnType($cur,11),"YEAR");
	if ($majorversion==3) {
		assertEqual(sqlrcur_getColumnType($cur,12),"VARSTRING");
	} else {
		assertEqual(sqlrcur_getColumnType($cur,12),"STRING");
	}
	assertEqual(sqlrcur_getColumnType($cur,13),"BLOB");
	assertEqual(sqlrcur_getColumnType($cur,14),"VARSTRING");
	assertEqual(sqlrcur_getColumnType($cur,15),"TINYBLOB");
	assertEqual(sqlrcur_getColumnType($cur,16),"MEDIUMBLOB");
	assertEqual(sqlrcur_getColumnType($cur,17),"LONGBLOB");
	assertEqual(sqlrcur_getColumnType($cur,18),"TIMESTAMP");
	assertEqual(sqlrcur_getColumnType($cur,"testtinyint"),"TINYINT");
	assertEqual(sqlrcur_getColumnType($cur,"testsmallint"),"SMALLINT");
	assertEqual(sqlrcur_getColumnType($cur,"testmediumint"),"MEDIUMINT");
	assertEqual(sqlrcur_getColumnType($cur,"testint"),"INT");
	assertEqual(sqlrcur_getColumnType($cur,"testbigint"),"BIGINT");
	assertEqual(sqlrcur_getColumnType($cur,"testfloat"),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,"testreal"),"REAL");
	assertEqual(sqlrcur_getColumnType($cur,"testdecimal"),"DECIMAL");
	assertEqual(sqlrcur_getColumnType($cur,"testdate"),"DATE");
	assertEqual(sqlrcur_getColumnType($cur,"testtime"),"TIME");
	assertEqual(sqlrcur_getColumnType($cur,"testdatetime"),"DATETIME");
	assertEqual(sqlrcur_getColumnType($cur,"testyear"),"YEAR");
	if ($majorversion==3) {
		assertEqual(sqlrcur_getColumnType($cur,"testchar"),"VARSTRING");
	} else {
		assertEqual(sqlrcur_getColumnType($cur,"testchar"),"STRING");
	}
	assertEqual(sqlrcur_getColumnType($cur,"testtext"),"BLOB");
	assertEqual(sqlrcur_getColumnType($cur,"testvarchar"),"VARSTRING");
	assertEqual(sqlrcur_getColumnType($cur,"testtinytext"),"TINYBLOB");
	assertEqual(sqlrcur_getColumnType($cur,"testmediumtext"),"MEDIUMBLOB");
	assertEqual(sqlrcur_getColumnType($cur,"testlongtext"),"LONGBLOB");
	assertEqual(sqlrcur_getColumnType($cur,"testtimestamp"),"TIMESTAMP");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqual(sqlrcur_getColumnLength($cur,0),1);
	assertEqual(sqlrcur_getColumnLength($cur,1),2);
	assertEqual(sqlrcur_getColumnLength($cur,2),3);
	assertEqual(sqlrcur_getColumnLength($cur,3),4);
	assertEqual(sqlrcur_getColumnLength($cur,4),8);
	assertEqual(sqlrcur_getColumnLength($cur,5),4);
	assertEqual(sqlrcur_getColumnLength($cur,6),8);
	assertEqual(sqlrcur_getColumnLength($cur,7),6);
	assertEqual(sqlrcur_getColumnLength($cur,8),3);
	assertEqual(sqlrcur_getColumnLength($cur,9),3);
	assertEqual(sqlrcur_getColumnLength($cur,10),8);
	assertEqual(sqlrcur_getColumnLength($cur,11),1);
	//assertEqual(sqlrcur_getColumnLength($cur,12),40);
	assertEqual(sqlrcur_getColumnLength($cur,13),65535);
	//assertEqual(sqlrcur_getColumnLength($cur,14),41);
	assertEqual(sqlrcur_getColumnLength($cur,15),255);
	assertEqual(sqlrcur_getColumnLength($cur,16),16777215);
	assertEqual(sqlrcur_getColumnLength($cur,17),2147483647);
	assertEqual(sqlrcur_getColumnLength($cur,18),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testtinyint"),1);
	assertEqual(sqlrcur_getColumnLength($cur,"testsmallint"),2);
	assertEqual(sqlrcur_getColumnLength($cur,"testmediumint"),3);
	assertEqual(sqlrcur_getColumnLength($cur,"testint"),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testbigint"),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testfloat"),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testreal"),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testdecimal"),6);
	assertEqual(sqlrcur_getColumnLength($cur,"testdate"),3);
	assertEqual(sqlrcur_getColumnLength($cur,"testtime"),3);
	assertEqual(sqlrcur_getColumnLength($cur,"testdatetime"),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testyear"),1);
	//assertEqual(sqlrcur_getColumnLength($cur,"testchar"),40);
	assertEqual(sqlrcur_getColumnLength($cur,"testtext"),65535);
	//assertEqual(sqlrcur_getColumnLength($cur,"testvarchar"),41);
	assertEqual(sqlrcur_getColumnLength($cur,"testtinytext"),255);
	assertEqual(sqlrcur_getColumnLength($cur,"testmediumtext"),16777215);
	assertEqual(sqlrcur_getColumnLength($cur,"testlongtext"),2147483647);
	assertEqual(sqlrcur_getColumnLength($cur,"testtimestamp"),4);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqual(sqlrcur_getLongest($cur,0),1);
	assertEqual(sqlrcur_getLongest($cur,1),1);
	assertEqual(sqlrcur_getLongest($cur,2),1);
	assertEqual(sqlrcur_getLongest($cur,3),1);
	assertEqual(sqlrcur_getLongest($cur,4),1);
	#assertEqual(sqlrcur_getLongest($cur,5),3);
	assertEqual(sqlrcur_getLongest($cur,6),3);
	assertEqual(sqlrcur_getLongest($cur,7),3);
	assertEqual(sqlrcur_getLongest($cur,8),10);
	assertEqual(sqlrcur_getLongest($cur,9),8);
	assertEqual(sqlrcur_getLongest($cur,10),19);
	assertEqual(sqlrcur_getLongest($cur,11),4);
	assertEqual(sqlrcur_getLongest($cur,12),5);
	assertEqual(sqlrcur_getLongest($cur,13),5);
	assertEqual(sqlrcur_getLongest($cur,14),8);
	assertEqual(sqlrcur_getLongest($cur,15),9);
	assertEqual(sqlrcur_getLongest($cur,16),11);
	assertEqual(sqlrcur_getLongest($cur,17),9);
	if ($majorversion==3) {
		assertEqual(sqlrcur_getLongest($cur,18),14);
	} else {
		assertEqual(sqlrcur_getLongest($cur,18),19);
	}
	assertEqual(sqlrcur_getLongest($cur,"testtinyint"),1);
	assertEqual(sqlrcur_getLongest($cur,"testsmallint"),1);
	assertEqual(sqlrcur_getLongest($cur,"testmediumint"),1);
	assertEqual(sqlrcur_getLongest($cur,"testint"),1);
	assertEqual(sqlrcur_getLongest($cur,"testbigint"),1);
	#assertEqual(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqual(sqlrcur_getLongest($cur,"testreal"),3);
	assertEqual(sqlrcur_getLongest($cur,"testdecimal"),3);
	assertEqual(sqlrcur_getLongest($cur,"testdate"),10);
	assertEqual(sqlrcur_getLongest($cur,"testtime"),8);
	assertEqual(sqlrcur_getLongest($cur,"testdatetime"),19);
	assertEqual(sqlrcur_getLongest($cur,"testyear"),4);
	assertEqual(sqlrcur_getLongest($cur,"testchar"),5);
	assertEqual(sqlrcur_getLongest($cur,"testtext"),5);
	assertEqual(sqlrcur_getLongest($cur,"testvarchar"),8);
	assertEqual(sqlrcur_getLongest($cur,"testtinytext"),9);
	assertEqual(sqlrcur_getLongest($cur,"testmediumtext"),11);
	assertEqual(sqlrcur_getLongest($cur,"testlongtext"),9);
	if ($majorversion==3) {
		assertEqual(sqlrcur_getLongest($cur,"testtimestamp"),14);
	} else {
		assertEqual(sqlrcur_getLongest($cur,"testtimestamp"),19);
	}
	echo("\n");


	# row count
	echo("ROW COUNT: \n");
	assertEqual(sqlrcur_rowCount($cur),8);
	echo("\n");


	# total rows
	echo("TOTAL ROWS: \n");
	// older versions of mysql know this
	//assertEqual(sqlrcur_totalRows($cur),0);
	echo("\n");


	# first row index
	echo("FIRST ROW INDEX: \n");
	assertEqual(sqlrcur_firstRowIndex($cur),0);
	echo("\n");


	# end of result set
	echo("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet($cur));
	echo("\n");


	# fields by index
	echo("FIELDS BY INDEX: \n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"1");
	assertEqual(sqlrcur_getField($cur,0,2),"1");
	assertEqual(sqlrcur_getField($cur,0,3),"1");
	assertEqual(sqlrcur_getField($cur,0,4),"1");
	#assertEqual(sqlrcur_getField($cur,0,5),"1.1");
	assertEqual(sqlrcur_getField($cur,0,6),"1.1");
	assertEqual(sqlrcur_getField($cur,0,7),"1.1");
	assertEqual(sqlrcur_getField($cur,0,8),"2001-01-01");
	assertEqual(sqlrcur_getField($cur,0,9),"01:00:00");
	assertEqual(sqlrcur_getField($cur,0,10),"2001-01-01 01:00:00");
	assertEqual(sqlrcur_getField($cur,0,11),"2001");
	assertEqual(sqlrcur_getField($cur,0,12),"char1");
	assertEqual(sqlrcur_getField($cur,0,13),"text1");
	assertEqual(sqlrcur_getField($cur,0,14),"varchar1");
	assertEqual(sqlrcur_getField($cur,0,15),"tinytext1");
	assertEqual(sqlrcur_getField($cur,0,16),"mediumtext1");
	assertEqual(sqlrcur_getField($cur,0,17),"longtext1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,7,1),"8");
	assertEqual(sqlrcur_getField($cur,7,2),"8");
	assertEqual(sqlrcur_getField($cur,7,3),"8");
	assertEqual(sqlrcur_getField($cur,7,4),"8");
	#assertEqual(sqlrcur_getField($cur,7,5),"8.1");
	assertEqual(sqlrcur_getField($cur,7,6),"8.1");
	assertEqual(sqlrcur_getField($cur,7,7),"8.1");
	assertEqual(sqlrcur_getField($cur,7,8),"2008-01-01");
	assertEqual(sqlrcur_getField($cur,7,9),"08:00:00");
	assertEqual(sqlrcur_getField($cur,7,10),"2008-01-01 08:00:00");
	assertEqual(sqlrcur_getField($cur,7,11),"2008");
	assertEqual(sqlrcur_getField($cur,7,12),"char8");
	assertEqual(sqlrcur_getField($cur,7,13),"text8");
	assertEqual(sqlrcur_getField($cur,7,14),"varchar8");
	assertEqual(sqlrcur_getField($cur,7,15),"tinytext8");
	assertEqual(sqlrcur_getField($cur,7,16),"mediumtext8");
	assertEqual(sqlrcur_getField($cur,7,17),"longtext8");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,1),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,2),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,3),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,4),1);
	#assertEqual(sqlrcur_getFieldLength($cur,0,5),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,6),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,7),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,8),10);
	assertEqual(sqlrcur_getFieldLength($cur,0,9),8);
	assertEqual(sqlrcur_getFieldLength($cur,0,10),19);
	assertEqual(sqlrcur_getFieldLength($cur,0,11),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,12),5);
	assertEqual(sqlrcur_getFieldLength($cur,0,13),5);
	assertEqual(sqlrcur_getFieldLength($cur,0,14),8);
	assertEqual(sqlrcur_getFieldLength($cur,0,15),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,16),11);
	assertEqual(sqlrcur_getFieldLength($cur,0,17),9);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,1),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,2),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,3),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,4),1);
	#assertEqual(sqlrcur_getFieldLength($cur,7,5),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,6),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,7),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,8),10);
	assertEqual(sqlrcur_getFieldLength($cur,7,9),8);
	assertEqual(sqlrcur_getFieldLength($cur,7,10),19);
	assertEqual(sqlrcur_getFieldLength($cur,7,11),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,12),5);
	assertEqual(sqlrcur_getFieldLength($cur,7,13),5);
	assertEqual(sqlrcur_getFieldLength($cur,7,14),8);
	assertEqual(sqlrcur_getFieldLength($cur,7,15),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,16),11);
	assertEqual(sqlrcur_getFieldLength($cur,7,17),9);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqual(sqlrcur_getField($cur,0,"testtinyint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testsmallint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testmediumint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testbigint"),"1");
	#assertEqual(sqlrcur_getField($cur,0,"testfloat"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testreal"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testdecimal"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testdate"),"2001-01-01");
	assertEqual(sqlrcur_getField($cur,0,"testtime"),"01:00:00");
	assertEqual(sqlrcur_getField($cur,0,"testdatetime"),"2001-01-01 01:00:00");
	assertEqual(sqlrcur_getField($cur,0,"testyear"),"2001");
	assertEqual(sqlrcur_getField($cur,0,"testchar"),"char1");
	assertEqual(sqlrcur_getField($cur,0,"testtext"),"text1");
	assertEqual(sqlrcur_getField($cur,0,"testvarchar"),"varchar1");
	assertEqual(sqlrcur_getField($cur,0,"testtinytext"),"tinytext1");
	assertEqual(sqlrcur_getField($cur,0,"testmediumtext"),"mediumtext1");
	assertEqual(sqlrcur_getField($cur,0,"testlongtext"),"longtext1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,"testtinyint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testsmallint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testmediumint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testbigint"),"8");
	#assertEqual(sqlrcur_getField($cur,7,"testfloat"),"8.1");
	assertEqual(sqlrcur_getField($cur,7,"testreal"),"8.1");
	assertEqual(sqlrcur_getField($cur,7,"testdecimal"),"8.1");
	assertEqual(sqlrcur_getField($cur,7,"testdate"),"2008-01-01");
	assertEqual(sqlrcur_getField($cur,7,"testtime"),"08:00:00");
	assertEqual(sqlrcur_getField($cur,7,"testdatetime"),"2008-01-01 08:00:00");
	assertEqual(sqlrcur_getField($cur,7,"testyear"),"2008");
	assertEqual(sqlrcur_getField($cur,7,"testchar"),"char8");
	assertEqual(sqlrcur_getField($cur,7,"testtext"),"text8");
	assertEqual(sqlrcur_getField($cur,7,"testvarchar"),"varchar8");
	assertEqual(sqlrcur_getField($cur,7,"testtinytext"),"tinytext8");
	assertEqual(sqlrcur_getField($cur,7,"testmediumtext"),"mediumtext8");
	assertEqual(sqlrcur_getField($cur,7,"testlongtext"),"longtext8");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,"testtinyint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testsmallint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testmediumint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testbigint"),1);
	#assertEqual(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testreal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testdecimal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testdate"),10);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testtime"),8);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testdatetime"),19);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testyear"),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testchar"),5);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testtext"),5);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testvarchar"),8);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testtinytext"),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testmediumtext"),11);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testlongtext"),9);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,"testtinyint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testsmallint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testmediumint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testbigint"),1);
	#assertEqual(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testreal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testdecimal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testdate"),10);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testtime"),8);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testdatetime"),19);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testyear"),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testchar"),5);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testtext"),5);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testvarchar"),8);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testtinytext"),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testmediumtext"),11);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testlongtext"),9);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqual($fields[0],"1");
	assertEqual($fields[1],"1");
	assertEqual($fields[2],"1");
	assertEqual($fields[3],"1");
	assertEqual($fields[4],"1");
	#assertEqual($fields[5],"1.1");
	assertEqual($fields[6],"1.1");
	assertEqual($fields[7],"1.1");
	assertEqual($fields[8],"2001-01-01");
	assertEqual($fields[9],"01:00:00");
	assertEqual($fields[10],"2001-01-01 01:00:00");
	assertEqual($fields[11],"2001");
	assertEqual($fields[12],"char1");
	assertEqual($fields[13],"text1");
	assertEqual($fields[14],"varchar1");
	assertEqual($fields[15],"tinytext1");
	assertEqual($fields[16],"mediumtext1");
	assertEqual($fields[17],"longtext1");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqual($fieldlens[0],1);
	assertEqual($fieldlens[1],1);
	assertEqual($fieldlens[2],1);
	assertEqual($fieldlens[3],1);
	assertEqual($fieldlens[4],1);
	#assertEqual($fieldlens[5],3);
	assertEqual($fieldlens[6],3);
	assertEqual($fieldlens[7],3);
	assertEqual($fieldlens[8],10);
	assertEqual($fieldlens[9],8);
	assertEqual($fieldlens[10],19);
	assertEqual($fieldlens[11],4);
	assertEqual($fieldlens[12],5);
	assertEqual($fieldlens[13],5);
	assertEqual($fieldlens[14],8);
	assertEqual($fieldlens[15],9);
	assertEqual($fieldlens[16],11);
	assertEqual($fieldlens[17],9);
	echo("\n");


	# fields by associative array
	echo("FIELDS BY ASSOCIATIVE ARRAY: \n");
	$fields=sqlrcur_getRowAssoc($cur,0);
	assertEqual($fields["testtinyint"],"1");
	assertEqual($fields["testsmallint"],"1");
	assertEqual($fields["testmediumint"],"1");
	assertEqual($fields["testint"],"1");
	assertEqual($fields["testbigint"],"1");
	#assertEqual($fields["testfloat"],"1.1");
	assertEqual($fields["testreal"],"1.1");
	assertEqual($fields["testdecimal"],"1.1");
	assertEqual($fields["testdate"],"2001-01-01");
	assertEqual($fields["testtime"],"01:00:00");
	assertEqual($fields["testdatetime"],"2001-01-01 01:00:00");
	assertEqual($fields["testyear"],"2001");
	assertEqual($fields["testchar"],"char1");
	assertEqual($fields["testtext"],"text1");
	assertEqual($fields["testvarchar"],"varchar1");
	assertEqual($fields["testtinytext"],"tinytext1");
	assertEqual($fields["testmediumtext"],"mediumtext1");
	assertEqual($fields["testlongtext"],"longtext1");
	echo("\n");
	$fields=sqlrcur_getRowAssoc($cur,7);
	assertEqual($fields["testtinyint"],"8");
	assertEqual($fields["testsmallint"],"8");
	assertEqual($fields["testmediumint"],"8");
	assertEqual($fields["testint"],"8");
	assertEqual($fields["testbigint"],"8");
	#assertEqual($fields["testfloat"],"8.1");
	assertEqual($fields["testreal"],"8.1");
	assertEqual($fields["testdecimal"],"8.1");
	assertEqual($fields["testdate"],"2008-01-01");
	assertEqual($fields["testtime"],"08:00:00");
	assertEqual($fields["testdatetime"],"2008-01-01 08:00:00");
	assertEqual($fields["testyear"],"2008");
	assertEqual($fields["testchar"],"char8");
	assertEqual($fields["testtext"],"text8");
	assertEqual($fields["testvarchar"],"varchar8");
	assertEqual($fields["testtinytext"],"tinytext8");
	assertEqual($fields["testmediumtext"],"mediumtext8");
	assertEqual($fields["testlongtext"],"longtext8");
	echo("\n");


	# field lengths by associative array
	echo("FIELD LENGTHS BY ASSOCIATIVE ARRAY: \n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,0);
	assertEqual($fieldlengths["testtinyint"],1);
	assertEqual($fieldlengths["testsmallint"],1);
	assertEqual($fieldlengths["testmediumint"],1);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testbigint"],1);
	#assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testreal"],3);
	assertEqual($fieldlengths["testdecimal"],3);
	assertEqual($fieldlengths["testdate"],10);
	assertEqual($fieldlengths["testtime"],8);
	assertEqual($fieldlengths["testdatetime"],19);
	assertEqual($fieldlengths["testyear"],4);
	assertEqual($fieldlengths["testchar"],5);
	assertEqual($fieldlengths["testtext"],5);
	assertEqual($fieldlengths["testvarchar"],8);
	assertEqual($fieldlengths["testtinytext"],9);
	assertEqual($fieldlengths["testmediumtext"],11);
	assertEqual($fieldlengths["testlongtext"],9);
	echo("\n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,7);
	assertEqual($fieldlengths["testtinyint"],1);
	assertEqual($fieldlengths["testsmallint"],1);
	assertEqual($fieldlengths["testmediumint"],1);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testbigint"],1);
	#assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testreal"],3);
	assertEqual($fieldlengths["testdecimal"],3);
	assertEqual($fieldlengths["testdate"],10);
	assertEqual($fieldlengths["testtime"],8);
	assertEqual($fieldlengths["testdatetime"],19);
	assertEqual($fieldlengths["testyear"],4);
	assertEqual($fieldlengths["testchar"],5);
	assertEqual($fieldlengths["testtext"],5);
	assertEqual($fieldlengths["testvarchar"],8);
	assertEqual($fieldlengths["testtinytext"],9);
	assertEqual($fieldlengths["testmediumtext"],11);
	assertEqual($fieldlengths["testlongtext"],9);
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select $(var1),'$(var2)',$(var3)");
	sqlrcur_substitution($cur,"var1",1);
	sqlrcur_substitution($cur,"var2","hello");
	sqlrcur_substitution($cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# fields
	echo("FIELDS: \n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"hello");
	assertEqual(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	# array substitutions
	echo("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select $(var1),'$(var2)',$(var3)");
	$vars=array("var1","var2","var3");
	$vals=array(1,"hello",10.5556);
	$precs=array(0,0,6);
	$scales=array(0,0,4);
	sqlrcur_substitutions($cur,$vars,$vals,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# fields
	echo("FIELDS: \n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"hello");
	assertEqual(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	# nulls as nulls
	echo("NULLS as Nulls: \n");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL"));
	assertEqual(sqlrcur_getField($cur,0,0),NULL);
	assertEqual(sqlrcur_getField($cur,0,1),"1");
	assertEqual(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL"));
	assertEqual(sqlrcur_getField($cur,0,0),"");
	assertEqual(sqlrcur_getField($cur,0,1),"1");
	assertEqual(sqlrcur_getField($cur,0,2),"");
	sqlrcur_getNullsAsNulls($cur);
	echo("\n");


	# result set buffer size
	echo("RESULT SET BUFFER SIZE: \n");
	assertEqual(sqlrcur_getResultSetBufferSize($cur),0);
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqual(sqlrcur_getResultSetBufferSize($cur),2);
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),0);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),2);
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,1,0),"2");
	assertEqual(sqlrcur_getField($cur,2,0),"3");
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),2);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),4);
	assertEqual(sqlrcur_getField($cur,6,0),"7");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),8);
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),8);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# dont get column info
	echo("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqual(sqlrcur_getColumnName($cur,0),NULL);
	assertEqual(sqlrcur_getColumnLength($cur,0),0);
	assertEqual(sqlrcur_getColumnType($cur,0),NULL);
	echo("\n");
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqual(sqlrcur_getColumnName($cur,0),"testtinyint");
	assertEqual(sqlrcur_getColumnLength($cur,0),1);
	assertEqual(sqlrcur_getColumnType($cur,0),"TINYINT");
	echo("\n");


	# suspended session
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
	$conport=sqlrcon_getConnectionPort($con);
	$consocket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$conport,$consocket));
	echo("\n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,1,0),"2");
	assertEqual(sqlrcur_getField($cur,2,0),"3");
	assertEqual(sqlrcur_getField($cur,3,0),"4");
	assertEqual(sqlrcur_getField($cur,4,0),"5");
	assertEqual(sqlrcur_getField($cur,5,0),"6");
	assertEqual(sqlrcur_getField($cur,6,0),"7");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
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
	$conport=sqlrcon_getConnectionPort($con);
	$consocket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$conport,$consocket));
	echo("\n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,1,0),"2");
	assertEqual(sqlrcur_getField($cur,2,0),"3");
	assertEqual(sqlrcur_getField($cur,3,0),"4");
	assertEqual(sqlrcur_getField($cur,4,0),"5");
	assertEqual(sqlrcur_getField($cur,5,0),"6");
	assertEqual(sqlrcur_getField($cur,6,0),"7");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
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
	$conport=sqlrcon_getConnectionPort($con);
	$consocket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$conport,$consocket));
	echo("\n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,1,0),"2");
	assertEqual(sqlrcur_getField($cur,2,0),"3");
	assertEqual(sqlrcur_getField($cur,3,0),"4");
	assertEqual(sqlrcur_getField($cur,4,0),"5");
	assertEqual(sqlrcur_getField($cur,5,0),"6");
	assertEqual(sqlrcur_getField($cur,6,0),"7");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# suspended result set
	echo("SUSPENDED RESULT SET: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testtinyint "));
	assertEqual(sqlrcur_getField($cur,2,0),"3");
	$id=sqlrcur_getResultSetId($cur);
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$conport=sqlrcon_getConnectionPort($con);
	$consocket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$conport,$consocket));
	assertTrue(sqlrcur_resumeResultSet($cur,$id));
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),4);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),6);
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),8);
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),8);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# cached result set
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
	assertEqual($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_colCount($cur),19);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testtinyint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqual(sqlrcur_getColumnName($cur,2),"testmediumint");
	assertEqual(sqlrcur_getColumnName($cur,3),"testint");
	assertEqual(sqlrcur_getColumnName($cur,4),"testbigint");
	assertEqual(sqlrcur_getColumnName($cur,5),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,6),"testreal");
	assertEqual(sqlrcur_getColumnName($cur,7),"testdecimal");
	assertEqual(sqlrcur_getColumnName($cur,8),"testdate");
	assertEqual(sqlrcur_getColumnName($cur,9),"testtime");
	assertEqual(sqlrcur_getColumnName($cur,10),"testdatetime");
	assertEqual(sqlrcur_getColumnName($cur,11),"testyear");
	assertEqual(sqlrcur_getColumnName($cur,12),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,13),"testtext");
	assertEqual(sqlrcur_getColumnName($cur,14),"testvarchar");
	assertEqual(sqlrcur_getColumnName($cur,15),"testtinytext");
	assertEqual(sqlrcur_getColumnName($cur,16),"testmediumtext");
	assertEqual(sqlrcur_getColumnName($cur,17),"testlongtext");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testtinyint");
	assertEqual($cols[1],"testsmallint");
	assertEqual($cols[2],"testmediumint");
	assertEqual($cols[3],"testint");
	assertEqual($cols[4],"testbigint");
	assertEqual($cols[5],"testfloat");
	assertEqual($cols[6],"testreal");
	assertEqual($cols[7],"testdecimal");
	assertEqual($cols[8],"testdate");
	assertEqual($cols[9],"testtime");
	assertEqual($cols[10],"testdatetime");
	assertEqual($cols[11],"testyear");
	assertEqual($cols[12],"testchar");
	assertEqual($cols[13],"testtext");
	assertEqual($cols[14],"testvarchar");
	assertEqual($cols[15],"testtinytext");
	assertEqual($cols[16],"testmediumtext");
	assertEqual($cols[17],"testlongtext");
	echo("\n");


	# cached result set with result set buffer size
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
	assertEqual($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# from one cache file to another
	echo("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");


	# from one cache file to another with result set buffer size
	echo("FROM ONE CACHE FILE TO ANOTHER ".
		"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# cached result set with suspend and result set buffer size
	echo("CACHED RESULT SET WITH SUSPEND ".
		"AND RESULT SET BUFFER SIZE: \n");
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
	assertEqual(sqlrcur_getField($cur,2,0),"3");
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqual($filename,"cachefile1");
	$id=sqlrcur_getResultSetId($cur);
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$conport=sqlrcon_getConnectionPort($con);
	$consocket=sqlrcon_getConnectionSocket($con);
	echo("\n");
	assertTrue(sqlrcon_resumeSession($con,$conport,$consocket));
	assertTrue(sqlrcur_resumeCachedResultSet($cur,$id,$filename));
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),4);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),6);
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),8);
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");
	assertEqual(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqual(sqlrcur_rowCount($cur),8);
	sqlrcur_cacheOff($cur);
	echo("\n");
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# commit
	echo("COMMIT: \n");
	$secondcon=sqlrcon_alloc($host,$port,
					$socket,$user,$password,0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	assertTrue(sqlrcur_sendQuery($secondcur,
		"select ".
		"	count(*) ".
		"from ".
		"	testtable "));
	if ($majorversion>3) {
		assertEqual(sqlrcur_getField($secondcur,0,0),"0");
	} else {
		assertEqual(sqlrcur_getField($secondcur,0,0),"8");
	}
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcon_commit($secondcon));
	assertTrue(sqlrcur_sendQuery($secondcur,
		"select ".
		"	count(*) ".
		"from ".
		"	testtable "));
	assertEqual(sqlrcur_getField($secondcur,0,0),"8");
	assertTrue(sqlrcon_autoCommitOn($con));
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
		"	'text10', ".
		"	'varchar10', ".
		"	'tinytext10', ".
		"	'mediumtext10', ".
		"	'longtext10', ".
		"	NULL)"));
	assertTrue(sqlrcon_commit($secondcon));
	assertTrue(sqlrcur_sendQuery($secondcur,
		"select ".
		"	count(*) ".
		"from ".
		"	testtable "));
	assertEqual(sqlrcur_getField($secondcur,0,0),"9");
	assertTrue(sqlrcon_autoCommitOff($con));
	sqlrcon_commit($secondcon);
	echo("\n");

	# drop existing table
	sqlrcur_sendQuery($cur,"drop table testtable");


	# invalid queries
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
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	2, ".
		"	3, ".
		"	4)"));
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	2, ".
		"	3, ".
		"	4)"));
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	2, ".
		"	3, ".
		"	4)"));
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	2, ".
		"	3, ".
		"	4)"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	echo("\n");

	} catch (Exception $e) {
		exit(1);
	}

	reportTestStatus();

	exit($status);
?></pre></html>
