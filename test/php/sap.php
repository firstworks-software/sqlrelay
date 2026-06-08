<html><pre><?php
# Copyright (c) David Muse
# See the file COPYING for more information.
include("./asserts.php");


	$isolationlevels=array("1","0","2","3");
	$subvars=array("var1","var2","var3");
	$subvallongs=array(1,2,3);
	$subvalstrings=array("hi","hello","bye");
	$subvaldoubles=array(10.55,10.556,10.5556);
	$precs=array(4,5,6);
	$scales=array(2,3,4);


	# hostname
	$hostname=gethostname();
	$dot=strpos($hostname,'.');
	if ($dot) {
		$hostname=substr($hostname,0,$dot);
	}
	$dumptran="dump tran $hostname with truncate_only";


	# instantiation
	$con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
			"testuser","testpassword",0,1);
	$cur=sqlrcur_alloc($con);


	# identify
	echo("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify($con),"sap");
	echo("\n");


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	# transaction state
	echo("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel($con),"explicit-error");
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit-error");
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	# bind format
	echo("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat($con),"@*");
	echo("\n");


	# nextval format
	echo("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat($con),"%s.nextval");
	echo("\n");


	# isolation levels
	echo("ISOLATION LEVELS: \n");
	foreach ($isolationlevels as $il) {
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		assertEqStr(sqlrcon_getIsolationLevel($con),$il);
		echo("\n");
	}
	# reset to the default isolation level
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	echo("\n");


	# create testtable
	echo("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,$dumptran);
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testint int, ".
		"	testsmallint smallint, ".
		"	testtinyint tinyint, ".
		"	testreal real, ".
		"	testfloat float, ".
		"	testdecimal decimal(4,1), ".
		"	testnumeric numeric(4,1), ".
		"	testmoney money, ".
		"	testsmallmoney smallmoney, ".
		"	testdatetime datetime, ".
		"	testsmalldatetime ".
		"smalldatetime, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testbit bit, ".
		"	testtext text) "."lock datarows"));
	echo("\n");


	# insert
	echo("INSERT: \n");
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	1, ".
		"	1, ".
		"	1.5, ".
		"	1.5, ".
		"	1.5, ".
		"	1.5, ".
		"	1.00, ".
		"	1.00, ".
		"	'01-Jan-2001 01:00:00', ".
		"	'01-Jan-2001 01:00:00', ".
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	1, ".
		"	'testtext1')"));
	echo("\n");


	# affected rows
	echo("AFFECTED ROWS: \n");
	assertEqInt(sqlrcur_affectedRows($cur),1);
	echo("\n");


	# input bind by position
	echo("INPUT BIND BY POSITION: \n");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	@var1, ".
		"	@var2, ".
		"	@var3, ".
		"	@var4, ".
		"	@var5, ".
		"	@var6, ".
		"	@var7, ".
		"	@var8, ".
		"	@var9, ".
		"	@var10, ".
		"	@var11, ".
		"	@var12, ".
		"	@var13, ".
		"	@var14, ".
		"	@var15)");
	assertEqInt(sqlrcur_countBindVariables($cur),15);
	sqlrcur_inputBind($cur,"1",2);
	sqlrcur_inputBind($cur,"2",2);
	sqlrcur_inputBind($cur,"3",2);
	sqlrcur_inputBind($cur,"4",2.5,2,1);
	sqlrcur_inputBind($cur,"5",2.5,2,1);
	sqlrcur_inputBind($cur,"6",2.5,2,1);
	sqlrcur_inputBind($cur,"7",2.5,2,1);
	sqlrcur_inputBind($cur,"8",2.00,3,2);
	sqlrcur_inputBind($cur,"9",2.00,3,2);
	sqlrcur_inputBind($cur,"10","01-Jan-2002 02:00:00");
	sqlrcur_inputBind($cur,"11","01-Jan-2002 02:00:00");
	sqlrcur_inputBind($cur,"12","testchar2");
	sqlrcur_inputBind($cur,"13","testvarchar2");
	sqlrcur_inputBind($cur,"14",1);
	sqlrcur_inputBindClob($cur,"15","testtext2",strlen("testtext2"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",3);
	sqlrcur_inputBind($cur,"2",3);
	sqlrcur_inputBind($cur,"3",3);
	sqlrcur_inputBind($cur,"4",3.5,2,1);
	sqlrcur_inputBind($cur,"5",3.5,2,1);
	sqlrcur_inputBind($cur,"6",3.5,2,1);
	sqlrcur_inputBind($cur,"7",3.5,2,1);
	sqlrcur_inputBind($cur,"8",3.00,3,2);
	sqlrcur_inputBind($cur,"9",3.00,3,2);
	sqlrcur_inputBind($cur,"10","01-Jan-2003 03:00:00");
	sqlrcur_inputBind($cur,"11","01-Jan-2003 03:00:00");
	sqlrcur_inputBind($cur,"12","testchar3");
	sqlrcur_inputBind($cur,"13","testvarchar3");
	sqlrcur_inputBind($cur,"14",1);
	sqlrcur_inputBindClob($cur,"15","testtext3",strlen("testtext3"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by position
	# sap doesn't support implicit conversion
	# of string binds to other data types, so
	# arrays of binds don't generally work.
	# Omitting the test.


	# input bind by position with validation
	echo("INPUT BIND BY POSITION "."WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",4);
	sqlrcur_inputBind($cur,"2",4);
	sqlrcur_inputBind($cur,"3",4);
	sqlrcur_inputBind($cur,"4",4.5,2,1);
	sqlrcur_inputBind($cur,"5",4.5,2,1);
	sqlrcur_inputBind($cur,"6",4.5,2,1);
	sqlrcur_inputBind($cur,"7",4.5,2,1);
	sqlrcur_inputBind($cur,"8",4.00,3,2);
	sqlrcur_inputBind($cur,"9",4.00,3,2);
	sqlrcur_inputBind($cur,"10","01-Jan-2004 04:00:00");
	sqlrcur_inputBind($cur,"11","01-Jan-2004 04:00:00");
	sqlrcur_inputBind($cur,"12","testchar4");
	sqlrcur_inputBind($cur,"13","testvarchar4");
	sqlrcur_inputBind($cur,"14",1);
	sqlrcur_inputBindClob($cur,"15","testtext4",strlen("testtext4"));
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# input bind by name
	echo("INPUT BIND BY NAME: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",5);
	sqlrcur_inputBind($cur,"var2",5);
	sqlrcur_inputBind($cur,"var3",5);
	sqlrcur_inputBind($cur,"var4",5.5,2,1);
	sqlrcur_inputBind($cur,"var5",5.5,2,1);
	sqlrcur_inputBind($cur,"var6",5.5,2,1);
	sqlrcur_inputBind($cur,"var7",5.5,2,1);
	sqlrcur_inputBind($cur,"var8",5.00,3,2);
	sqlrcur_inputBind($cur,"var9",5.00,3,2);
	sqlrcur_inputBind($cur,"var10","01-Jan-2005 05:00:00");
	sqlrcur_inputBind($cur,"var11","01-Jan-2005 05:00:00");
	sqlrcur_inputBind($cur,"var12","testchar5");
	sqlrcur_inputBind($cur,"var13","testvarchar5");
	sqlrcur_inputBind($cur,"var14",1);
	sqlrcur_inputBindClob($cur,"var15","testtext5",strlen("testtext5"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",6);
	sqlrcur_inputBind($cur,"var2",6);
	sqlrcur_inputBind($cur,"var3",6);
	sqlrcur_inputBind($cur,"var4",6.5,2,1);
	sqlrcur_inputBind($cur,"var5",6.5,2,1);
	sqlrcur_inputBind($cur,"var6",6.5,2,1);
	sqlrcur_inputBind($cur,"var7",6.5,2,1);
	sqlrcur_inputBind($cur,"var8",6.00,3,2);
	sqlrcur_inputBind($cur,"var9",6.00,3,2);
	sqlrcur_inputBind($cur,"var10","01-Jan-2006 06:00:00");
	sqlrcur_inputBind($cur,"var11","01-Jan-2006 06:00:00");
	sqlrcur_inputBind($cur,"var12","testchar6");
	sqlrcur_inputBind($cur,"var13","testvarchar6");
	sqlrcur_inputBind($cur,"var14",1);
	sqlrcur_inputBindClob($cur,"var15","testtext6",strlen("testtext6"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",7);
	sqlrcur_inputBind($cur,"var2",7);
	sqlrcur_inputBind($cur,"var3",7);
	sqlrcur_inputBind($cur,"var4",7.5,2,1);
	sqlrcur_inputBind($cur,"var5",7.5,2,1);
	sqlrcur_inputBind($cur,"var6",7.5,2,1);
	sqlrcur_inputBind($cur,"var7",7.5,2,1);
	sqlrcur_inputBind($cur,"var8",7.00,3,2);
	sqlrcur_inputBind($cur,"var9",7.00,3,2);
	sqlrcur_inputBind($cur,"var10","01-Jan-2007 07:00:00");
	sqlrcur_inputBind($cur,"var11","01-Jan-2007 07:00:00");
	sqlrcur_inputBind($cur,"var12","testchar7");
	sqlrcur_inputBind($cur,"var13","testvarchar7");
	sqlrcur_inputBind($cur,"var14",1);
	sqlrcur_inputBindClob($cur,"var15","testtext7",strlen("testtext7"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by name
	# sap doesn't support implicit conversion
	# of string binds to other data types, so
	# arrays of binds don't generally work.
	# Omitting the test.


	# input bind by name with validation
	echo("INPUT BIND BY NAME "."WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",8);
	sqlrcur_inputBind($cur,"var2",8);
	sqlrcur_inputBind($cur,"var3",8);
	sqlrcur_inputBind($cur,"var4",8.5,2,1);
	sqlrcur_inputBind($cur,"var5",8.5,2,1);
	sqlrcur_inputBind($cur,"var6",8.5,2,1);
	sqlrcur_inputBind($cur,"var7",8.5,2,1);
	sqlrcur_inputBind($cur,"var8",8.00,3,2);
	sqlrcur_inputBind($cur,"var9",8.00,3,2);
	sqlrcur_inputBind($cur,"var10","01-Jan-2008 08:00:00");
	sqlrcur_inputBind($cur,"var11","01-Jan-2008 08:00:00");
	sqlrcur_inputBind($cur,"var12","testchar8");
	sqlrcur_inputBind($cur,"var13","testvarchar8");
	sqlrcur_inputBind($cur,"var14",1);
	sqlrcur_inputBindClob($cur,"var15","testtext8",strlen("testtext8"));
	sqlrcur_inputBind($cur,"var16","junkvalue");
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# select
	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount($cur),15);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testtinyint");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testreal");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testnumeric");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testmoney");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testsmallmoney");
	assertEqStr(sqlrcur_getColumnName($cur,9),"testdatetime");
	assertEqStr(sqlrcur_getColumnName($cur,10),"testsmalldatetime");
	assertEqStr(sqlrcur_getColumnName($cur,11),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,12),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,13),"testbit");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testint");
	assertEqStr($cols[1],"testsmallint");
	assertEqStr($cols[2],"testtinyint");
	assertEqStr($cols[3],"testreal");
	assertEqStr($cols[4],"testfloat");
	assertEqStr($cols[5],"testdecimal");
	assertEqStr($cols[6],"testnumeric");
	assertEqStr($cols[7],"testmoney");
	assertEqStr($cols[8],"testsmallmoney");
	assertEqStr($cols[9],"testdatetime");
	assertEqStr($cols[10],"testsmalldatetime");
	assertEqStr($cols[11],"testchar");
	assertEqStr($cols[12],"testvarchar");
	assertEqStr($cols[13],"testbit");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnType($cur,0),"INT");
	assertEqStr(sqlrcur_getColumnType($cur,"testint"),"INT");
	assertEqStr(sqlrcur_getColumnType($cur,1),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testsmallint"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,2),"TINYINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testtinyint"),"TINYINT");
	assertEqStr(sqlrcur_getColumnType($cur,3),"REAL");
	assertEqStr(sqlrcur_getColumnType($cur,"testreal"),"REAL");
	assertEqStr(sqlrcur_getColumnType($cur,4),"FLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,"testfloat"),"FLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,5),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,"testdecimal"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,6),"NUMERIC");
	assertEqStr(sqlrcur_getColumnType($cur,"testnumeric"),"NUMERIC");
	assertEqStr(sqlrcur_getColumnType($cur,7),"MONEY");
	assertEqStr(sqlrcur_getColumnType($cur,"testmoney"),"MONEY");
	assertEqStr(sqlrcur_getColumnType($cur,8),"SMALLMONEY");
	assertEqStr(sqlrcur_getColumnType($cur,"testsmallmoney"),
		"SMALLMONEY");
	assertEqStr(sqlrcur_getColumnType($cur,9),"DATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,"testdatetime"),"DATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,10),"SMALLDATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,"testsmalldatetime"),
		"SMALLDATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,11),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"testchar"),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,12),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"testvarchar"),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,13),"BIT");
	assertEqStr(sqlrcur_getColumnType($cur,"testbit"),"BIT");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLength($cur,0),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testint"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,1),2);
	assertEqInt(sqlrcur_getColumnLength($cur,"testsmallint"),2);
	assertEqInt(sqlrcur_getColumnLength($cur,2),1);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getColumnLength($cur,3),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testreal"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,4),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testfloat"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,5),35);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdecimal"),35);
	assertEqInt(sqlrcur_getColumnLength($cur,6),35);
	assertEqInt(sqlrcur_getColumnLength($cur,"testnumeric"),35);
	assertEqInt(sqlrcur_getColumnLength($cur,7),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testmoney"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,8),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,9),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdatetime"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,10),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testsmalldatetime"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,11),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,12),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testvarchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,13),1);
	assertEqInt(sqlrcur_getColumnLength($cur,"testbit"),1);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongest($cur,0),1);
	assertEqInt(sqlrcur_getLongest($cur,"testint"),1);
	assertEqInt(sqlrcur_getLongest($cur,1),1);
	assertEqInt(sqlrcur_getLongest($cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongest($cur,2),1);
	assertEqInt(sqlrcur_getLongest($cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getLongest($cur,3),3);
	assertEqInt(sqlrcur_getLongest($cur,"testreal"),3);
	assertEqInt(sqlrcur_getLongest($cur,4),3);
	assertEqInt(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongest($cur,5),3);
	assertEqInt(sqlrcur_getLongest($cur,"testdecimal"),3);
	assertEqInt(sqlrcur_getLongest($cur,6),3);
	assertEqInt(sqlrcur_getLongest($cur,"testnumeric"),3);
	assertEqInt(sqlrcur_getLongest($cur,7),4);
	assertEqInt(sqlrcur_getLongest($cur,"testmoney"),4);
	assertEqInt(sqlrcur_getLongest($cur,8),4);
	assertEqInt(sqlrcur_getLongest($cur,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getLongest($cur,9),19);
	assertEqInt(sqlrcur_getLongest($cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getLongest($cur,10),19);
	assertEqInt(sqlrcur_getLongest($cur,"testsmalldatetime"),19);
	assertEqInt(sqlrcur_getLongest($cur,11),40);
	assertEqInt(sqlrcur_getLongest($cur,"testchar"),40);
	assertEqInt(sqlrcur_getLongest($cur,12),12);
	assertEqInt(sqlrcur_getLongest($cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongest($cur,13),1);
	assertEqInt(sqlrcur_getLongest($cur,"testbit"),1);
	echo("\n");


	# row count
	echo("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount($cur),8);
	echo("\n");


	# total rows
	echo("TOTAL ROWS: \n");
	assertEqInt(sqlrcur_totalRows($cur),0);
	echo("\n");


	# first row index
	echo("FIRST ROW INDEX: \n");
	assertEqInt(sqlrcur_firstRowIndex($cur),0);
	echo("\n");


	# end of result set
	echo("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet($cur));
	echo("\n");


	# fields by index
	echo("FIELDS BY INDEX: \n");
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),"1");
	assertEqStr(sqlrcur_getField($cur,0,3),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,4),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,5),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,6),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,7),"1.00");
	assertEqStr(sqlrcur_getField($cur,0,8),"1.00");
	assertEqStr(sqlrcur_getField($cur,0,9),"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getField($cur,0,10),"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getField($cur,0,11),"testchar1".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,0,12),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,13),"1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,7,2),"8");
	assertEqStr(sqlrcur_getField($cur,7,3),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,4),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,5),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,6),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,7),"8.00");
	assertEqStr(sqlrcur_getField($cur,7,8),"8.00");
	assertEqStr(sqlrcur_getField($cur,7,9),"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getField($cur,7,10),"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getField($cur,7,11),"testchar8".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,7,12),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,13),"1");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,3),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,4),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,5),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,6),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,7),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,8),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,9),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,10),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,11),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,12),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,13),1);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,3),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,4),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,5),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,6),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,7),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,8),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,9),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,10),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,11),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,12),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,13),1);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testtinyint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testreal"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testdecimal"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testnumeric"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testmoney"),"1.00");
	assertEqStr(sqlrcur_getField($cur,0,"testsmallmoney"),"1.00");
	assertEqStr(sqlrcur_getField($cur,0,"testdatetime"),
		"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getField($cur,0,"testsmalldatetime"),
		"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getField($cur,0,"testchar"),"testchar1".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,0,"testvarchar"),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testbit"),"1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testtinyint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testreal"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testdecimal"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testnumeric"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testmoney"),"8.00");
	assertEqStr(sqlrcur_getField($cur,7,"testsmallmoney"),"8.00");
	assertEqStr(sqlrcur_getField($cur,7,"testdatetime"),
		"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getField($cur,7,"testsmalldatetime"),
		"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getField($cur,7,"testchar"),"testchar8".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,7,"testvarchar"),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testbit"),"1");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtinyint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testnumeric"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testsmalldatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testbit"),1);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtinyint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testnumeric"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testsmalldatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testbit"),1);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqStr($fields[0],"1");
	assertEqStr($fields[1],"1");
	assertEqStr($fields[2],"1");
	assertEqStr($fields[3],"1.5");
	assertEqStr($fields[4],"1.5");
	assertEqStr($fields[5],"1.5");
	assertEqStr($fields[6],"1.5");
	assertEqStr($fields[7],"1.00");
	assertEqStr($fields[8],"1.00");
	assertEqStr($fields[9],"Jan  1 2001  1:00AM");
	assertEqStr($fields[10],"Jan  1 2001  1:00AM");
	assertEqStr($fields[11],"testchar1"."                               ");
	assertEqStr($fields[12],"testvarchar1");
	assertEqStr($fields[13],"1");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqInt($fieldlens[0],1);
	assertEqInt($fieldlens[1],1);
	assertEqInt($fieldlens[2],1);
	assertEqInt($fieldlens[3],3);
	assertEqInt($fieldlens[4],3);
	assertEqInt($fieldlens[5],3);
	assertEqInt($fieldlens[6],3);
	assertEqInt($fieldlens[7],4);
	assertEqInt($fieldlens[8],4);
	assertEqInt($fieldlens[9],19);
	assertEqInt($fieldlens[10],19);
	assertEqInt($fieldlens[11],40);
	assertEqInt($fieldlens[12],12);
	assertEqInt($fieldlens[13],1);
	echo("\n");


	# result set buffer size
	echo("RESULT SET BUFFER SIZE: \n");
	assertEqInt(sqlrcur_getResultSetBufferSize($cur),0);
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
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


	# dont get column info
	echo("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	assertEqStr(sqlrcur_getColumnName($cur,0),NULL);
	assertEqInt(sqlrcur_getColumnLength($cur,0),0);
	assertEqStr(sqlrcur_getColumnType($cur,0),NULL);
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	assertEqStr(sqlrcur_getColumnName($cur,0),"testint");
	assertEqInt(sqlrcur_getColumnLength($cur,0),4);
	assertEqStr(sqlrcur_getColumnType($cur,0),"INT");
	echo("\n");


	# suspended session
	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,1,0),"2");
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	assertEqStr(sqlrcur_getField($cur,3,0),"4");
	assertEqStr(sqlrcur_getField($cur,4,0),"5");
	assertEqStr(sqlrcur_getField($cur,5,0),"6");
	assertEqStr(sqlrcur_getField($cur,6,0),"7");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
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


	# suspended result set
	echo("SUSPENDED RESULT SET: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
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


	# cached result set
	echo("CACHED RESULT SET: \n");
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR "."CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount($cur),15);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR "."CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testtinyint");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testreal");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testnumeric");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testmoney");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testsmallmoney");
	assertEqStr(sqlrcur_getColumnName($cur,9),"testdatetime");
	assertEqStr(sqlrcur_getColumnName($cur,10),"testsmalldatetime");
	assertEqStr(sqlrcur_getColumnName($cur,11),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,12),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,13),"testbit");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testint");
	assertEqStr($cols[1],"testsmallint");
	assertEqStr($cols[2],"testtinyint");
	assertEqStr($cols[3],"testreal");
	assertEqStr($cols[4],"testfloat");
	assertEqStr($cols[5],"testdecimal");
	assertEqStr($cols[6],"testnumeric");
	assertEqStr($cols[7],"testmoney");
	assertEqStr($cols[8],"testsmallmoney");
	assertEqStr($cols[9],"testdatetime");
	assertEqStr($cols[10],"testsmalldatetime");
	assertEqStr($cols[11],"testchar");
	assertEqStr($cols[12],"testvarchar");
	assertEqStr($cols[13],"testbit");
	echo("\n");


	# cached result set with result set
	# buffer size
	echo("CACHED RESULT SET WITH "."RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# from one cache file to another
	echo("FROM ONE CACHE FILE "."TO ANOTHER: \n");
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");


	# from one cache file to another with
	# result set buffer size
	echo("FROM ONE CACHE FILE TO ANOTHER "."WITH RESULT SET ".
		"BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# cached result set with suspend and
	# result set buffer size
	echo("CACHED RESULT SET "."WITH SUSPEND "."AND RESULT SET ".
		"BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
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


	# finished suspended session
	echo("FINISHED "."SUSPENDED SESSION: \n");
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


	# nested selects
	echo("NESTED SELECTS: \n");
	sqlrcur_setResultSetBufferSize($cur,1);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	$secondcur=sqlrcur_alloc($con);
	sqlrcur_setResultSetBufferSize($secondcur,1);
	for ($i=0; sqlrcur_getRow($cur,$i); $i++) {
		assertTrue(sqlrcur_sendQuery($secondcur,"select * "."from ".
				"testtable"));
	}
	sqlrcur_closeResultSet($secondcur);
	sqlrcur_setResultSetBufferSize($cur,0);
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# reset transaction state
	echo("RESET TRANSACTION STATE: \n");
	assertTrue(sqlrcon_commit($con));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit-error");
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	# transaction behavior - implicit
	echo("TRANSACTION BEHAVIOR - implicit: \n");
	# sap ase rejects DDL inside a chained-mode (multi-statement) tx
	# unless `sp_dboption ... 'ddl in tran', true` is set on the db;
	# create the table while still in unchained mode, then switch.
	# `lock datarows` is needed so secondcur's count(*) scan doesn't
	# block on the writer's page lock from the in-flight insert
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (col1 integer) lock datarows"));
	assertTrue(sqlrcon_setTransactionModel($con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	$secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	# session is in a transaction; insert is not visible until commit
	assertTrue(sqlrcon_getInTransaction($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"0");
	# commit makes it visible, and implicitly starts a new transaction
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# rollback discards, and implicitly starts a new transaction
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (3)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"2");
	# autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	sqlrcur_closeResultSet($secondcur);
	# switch back to unchained mode so the drop isn't rejected
	assertTrue(sqlrcon_setTransactionModel($con,"explicit-error"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - explicit
	echo("TRANSACTION BEHAVIOR - explicit: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"explicit"));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer) lock datarows"));
	# begin starts a new transaction; insert is not visible until commit
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"0");
	# commit makes it visible; no new transaction is started
	assertTrue(sqlrcon_commit($con));
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# begin, insert, rollback discards; no new transaction is started
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback($con));
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (3)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"2");
	# autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	sqlrcur_closeResultSet($secondcur);
	# switch back to unchained mode so the drop isn't rejected
	assertTrue(sqlrcon_setTransactionModel($con,"explicit-error"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - explicit-deferred
	echo("TRANSACTION BEHAVIOR - explicit-deferred: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"explicit-deferred"));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit-deferred");
	# switch to autocommit-on so the begin/commit cycles below
	# bracket explicit transactions (autocommit-off semantics are
	# exercised at the end of this block)
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer) lock datarows"));
	# begin starts a transaction; commit makes it visible
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1)"));
	assertTrue(sqlrcon_commit($con));
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# begin, insert, rollback discards
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# during a transaction started by begin(), autoCommitOn is a
	# no-op: the autocommit setting takes effect after the user
	# explicitly commits/rollbacks the tx (mysql-native semantic)
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (3)"));
	assertTrue(sqlrcon_autoCommitOn($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# explicit commit ends the tx; autocommit-on now takes effect
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"2");
	# autocommit is on; subsequent inserts are visible immediately
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (4)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"3");
	# autoCommitOff takes effect immediately when not in a transaction
	assertTrue(sqlrcon_autoCommitOff($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	# autocommit-off persists across commit/rollback; each commit or
	# rollback ends the current implicit tx and a new one starts for
	# the next statement
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (5)"));
	assertTrue(sqlrcon_commit($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"4");
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (6)"));
	assertTrue(sqlrcon_rollback($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"4");
	# autoCommitOff during a transaction changes the variable
	# immediately but the in-flight tx continues; only after the
	# next explicit commit/rollback does the new autocommit-off
	# setting drop us into a new implicit tx (mysql-asymmetric
	# semantic)
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (7)"));
	assertTrue(sqlrcon_autoCommitOff($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"4");
	assertTrue(sqlrcon_commit($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"5");
	sqlrcur_closeResultSet($secondcur);
	# switch back to unchained mode so the drop isn't rejected
	assertTrue(sqlrcon_setTransactionModel($con,"explicit-error"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - explicit-error
	echo("TRANSACTION BEHAVIOR - explicit-error: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"explicit-error"));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit-error");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer) lock datarows"));
	# begin, insert, commit
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1)"));
	assertTrue(sqlrcon_commit($con));
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# begin, insert, rollback
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# while in a transaction, autoCommitOn/Off throw an error
	assertTrue(sqlrcon_begin($con));
	assertFalse(sqlrcon_autoCommitOn($con));
	assertFalse(sqlrcon_autoCommitOff($con));
	assertTrue(sqlrcon_commit($con));
	# outside of a transaction, autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (3)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"2");
	# autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	sqlrcur_closeResultSet($secondcur);
	# commit the open tx so the drop isn't rejected as DDL inside a
	# chained-mode transaction (in explicit-error model, autoCommitOn
	# from inside a tx errors out by design, so commit is the route
	# back to autocommit-on / unchained mode)
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - none
	echo("TRANSACTION BEHAVIOR - none: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"none"));
	assertEqStr(sqlrcon_getTransactionModel($con),"none");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer) lock datarows"));
	# no transactions; everything is visible immediately
	assertTrue(sqlrcon_getAutoCommit($con));
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
	# commit and rollback are no-ops
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"2");
	# autocommit is always on; autoCommitOff is an error
	assertFalse(sqlrcon_autoCommitOff($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	sqlrcur_closeResultSet($secondcur);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# reset transaction behavior
	echo("RESET TRANSACTION BEHAVIOR: \n");
	assertTrue(sqlrcon_setTransactionModel($con,sqlrcon_getDefaultTransactionModel($con)));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit-error");
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL "."SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select \$(var1),"."'\$(var2)',\$(var3)");
	sqlrcur_substitution($cur,"var1",1);
	sqlrcur_substitution($cur,"var2","hello");
	sqlrcur_substitution($cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	# array substitutions
	echo("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select \$(var1),"."\$(var2),\$(var3)");
	sqlrcur_substitutions($cur,$subvars,$subvallongs,NULL,NULL);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"2");
	assertEqStr(sqlrcur_getField($cur,0,2),"3");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select '\$(var1)',"."'\$(var2)','\$(var3)'");
	sqlrcur_substitutions($cur,$subvars,$subvalstrings,NULL,NULL);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"hi");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"bye");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select \$(var1),"."\$(var2),\$(var3)");
	sqlrcur_substitutions($cur,$subvars,$subvaldoubles,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"10.55");
	assertEqStr(sqlrcur_getField($cur,0,1),"10.556");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	# nulls as nulls
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



	# null and empty lobs
	echo("NULL AND EMPTY LOBS: \n");
	sqlrcur_getNullsAsNulls($cur);
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob1 text NULL, ".
		"	testclob2 text NULL, ".
		"	testblob1 image NULL, ".
		"	testblob2 image NULL)"));
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	@var1, ".
		"	@var2, ".
		"	@var3, ".
		"	@var4)");
	sqlrcur_inputBindClob($cur,"var1","",strlen(""));
	sqlrcur_inputBindClob($cur,"var2",NULL,0);
	sqlrcur_inputBindBlob($cur,"var3","",strlen(""));
	sqlrcur_inputBindBlob($cur,"var4",NULL,0);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	# sap converts empty strings to a single
	# space.  It's possible that if we had
	# true input bind support on the backend,
	# then this would work correctly, but for
	# now we're faking binds, and inserting
	# an empty string, so we have to check
	# for a single space here.
	assertEqStr(sqlrcur_getField($cur,0,0)," ");
	assertEqStr(sqlrcur_getField($cur,0,1),NULL);
	# see note above for why we're checking
	# for a single space
	assertEqStr(sqlrcur_getField($cur,0,2)," ");
	assertEqStr(sqlrcur_getField($cur,0,3),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# long lobs
	echo("LONG LOBS: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob text NULL, ".
		"	testblob image NULL) "."lock datarows");
	sqlrcur_prepareQuery($cur,"insert into testtable ".
		"values (@var1,@var2)");
	$largebuffer=str_repeat("C",255);
	sqlrcur_inputBindClob($cur,"var1",$largebuffer,strlen($largebuffer));
	sqlrcur_inputBindBlob($cur,"var2",$largebuffer,strlen($largebuffer));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testclob"),255);
	assertEqStr(sqlrcur_getField($cur,0,"testclob"),$largebuffer);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testblob"),255);
	assertEqStrLen(sqlrcur_getField($cur,0,"testblob"),$largebuffer,255);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# output bind by position
	echo("OUTPUT BIND "."BY POSITION: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc ".
		"	@out1 int output, ".
		"	@out2 varchar(20) output, ".
		"	@out3 float output, ".
		"	@out4 datetime output, ".
		"	@out5 varchar(20) "."output as ".
		"select @out1=1, ".
		"	@out2='hello', ".
		"	@out3=2.5, ".
		"	@out4='2001-02-03', ".
		"	@out5=null"));
	sqlrcur_prepareQuery($cur,"exec testproc");
	assertEqInt(sqlrcur_countBindVariables($cur),0);
	sqlrcur_defineOutputBindInteger($cur,"1");
	sqlrcur_defineOutputBindString($cur,"2",20);
	sqlrcur_defineOutputBindDouble($cur,"3");
	sqlrcur_defineOutputBindDate($cur,"4");
	sqlrcur_defineOutputBindString($cur,"5",20);
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"1");
	$stringvar=sqlrcur_getOutputBindString($cur,"2");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"3");
	$year=sqlrcur_getOutputBindDateYear($cur,"4");
	$month=sqlrcur_getOutputBindDateMonth($cur,"4");
	$day=sqlrcur_getOutputBindDateDay($cur,"4");
	$hour=sqlrcur_getOutputBindDateHour($cur,"4");
	$minute=sqlrcur_getOutputBindDateMinute($cur,"4");
	$second=sqlrcur_getOutputBindDateSecond($cur,"4");
	$microsecond=sqlrcur_getOutputBindDateMicroSecond($cur,"4");
	$tz=sqlrcur_getOutputBindDateTz($cur,"4");
	$isnegative=sqlrcur_getOutputBindDateIsNegative($cur,"4");
	$nullvar=sqlrcur_getOutputBindString($cur,"5");
	assertEqInt($numvar,1);
	assertEqStr($stringvar,"hello");
	assertEqDbl($floatvar,2.5);
	assertEqInt($year,2001);
	assertEqInt($month,2);
	assertEqInt($day,3);
	assertEqInt($hour,0);
	assertEqInt($minute,0);
	assertEqInt($second,0);
	assertEqInt($microsecond,0);
	assertEqStr($tz,"");
	assertFalse($isnegative);
	assertEqStr($nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# output bind by name
	echo("OUTPUT BIND BY NAME: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc ".
		"	@out1 int output, ".
		"	@out2 varchar(20) output, ".
		"	@out3 float output, ".
		"	@out4 datetime output, ".
		"	@out5 varchar(20) "."output as ".
		"select @out1=1, ".
		"	@out2='hello', ".
		"	@out3=2.5, ".
		"	@out4='2001-02-03', ".
		"	@out5=null"));
	sqlrcur_prepareQuery($cur,"exec testproc");
	assertEqInt(sqlrcur_countBindVariables($cur),0);
	sqlrcur_defineOutputBindInteger($cur,"out1");
	sqlrcur_defineOutputBindString($cur,"out2",20);
	sqlrcur_defineOutputBindDouble($cur,"out3");
	sqlrcur_defineOutputBindDate($cur,"out4");
	sqlrcur_defineOutputBindString($cur,"out5",20);
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"out1");
	$stringvar=sqlrcur_getOutputBindString($cur,"out2");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"out3");
	$year=sqlrcur_getOutputBindDateYear($cur,"out4");
	$month=sqlrcur_getOutputBindDateMonth($cur,"out4");
	$day=sqlrcur_getOutputBindDateDay($cur,"out4");
	$hour=sqlrcur_getOutputBindDateHour($cur,"out4");
	$minute=sqlrcur_getOutputBindDateMinute($cur,"out4");
	$second=sqlrcur_getOutputBindDateSecond($cur,"out4");
	$microsecond=sqlrcur_getOutputBindDateMicroSecond($cur,"out4");
	$tz=sqlrcur_getOutputBindDateTz($cur,"out4");
	$isnegative=sqlrcur_getOutputBindDateIsNegative($cur,"out4");
	$nullvar=sqlrcur_getOutputBindString($cur,"out5");
	assertEqInt($numvar,1);
	assertEqStr($stringvar,"hello");
	assertEqDbl($floatvar,2.5);
	assertEqInt($year,2001);
	assertEqInt($month,2);
	assertEqInt($day,3);
	assertEqInt($hour,0);
	assertEqInt($minute,0);
	assertEqInt($second,0);
	assertEqInt($microsecond,0);
	assertEqStr($tz,"");
	assertFalse($isnegative);
	assertEqStr($nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# output bind by name with validation
	# validateBinds() can't be used for
	# output binds, with sap.  In sap, when
	# executing a procedure, you don't declare
	# any bind variable delimiters in the
	# query.  eg, you just do:
	# "exec testproc", not
	# "exec testproc(@out1,@out2)".
	# If you call validateBinds(), it won't
	# find any binds in the query, and will
	# filter out any binds that you declare.


	# lob output bind
	# sap doesn't support lobs as output
	# parameters to stored procedures, and
	# there's no way to directly select into
	# a lob bind variable


	# long output bind
	echo("LONG OUTPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	$largebuffer=str_repeat("C",255);
	$query="create procedure testproc ".
		"@bindval varchar(255) "."output as ".
		"set @bindval='".$largebuffer."'";
	assertTrue(sqlrcur_sendQuery($cur,$query));
	sqlrcur_prepareQuery($cur,"exec testproc");
	sqlrcur_defineOutputBindString($cur,"bindval",255);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindLength($cur,"bindval"),255);
	assertEqStr(sqlrcur_getOutputBindString($cur,"bindval"),$largebuffer);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# negative input bind
	echo("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,"create table testtable "."(testval int)");
	sqlrcur_prepareQuery($cur,"insert into testtable "."values (@testval)");
	sqlrcur_inputBind($cur,"testval",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval "."from testtable");
	assertEqStr(sqlrcur_getField($cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# bind validation
	echo("BIND VALIDATION: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 varchar(20), ".
		"	col2 varchar(20), ".
		"	col3 varchar(20))");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	\$(var1), ".
		"	\$(var2), ".
		"	\$(var3))");
	sqlrcur_inputBind($cur,"var1","1");
	sqlrcur_inputBind($cur,"var2","2");
	sqlrcur_inputBind($cur,"var3","3");
	sqlrcur_substitution($cur,"var1","@var1");
	assertTrue(sqlrcur_validBind($cur,"var1"));
	assertFalse(sqlrcur_validBind($cur,"var2"));
	assertFalse(sqlrcur_validBind($cur,"var3"));
	assertFalse(sqlrcur_validBind($cur,"var4"));
	echo("\n");
	sqlrcur_substitution($cur,"var2","@var2");
	assertTrue(sqlrcur_validBind($cur,"var1"));
	assertTrue(sqlrcur_validBind($cur,"var2"));
	assertFalse(sqlrcur_validBind($cur,"var3"));
	assertFalse(sqlrcur_validBind($cur,"var4"));
	echo("\n");
	sqlrcur_substitution($cur,"var3","@var3");
	assertTrue(sqlrcur_validBind($cur,"var1"));
	assertTrue(sqlrcur_validBind($cur,"var2"));
	assertTrue(sqlrcur_validBind($cur,"var3"));
	assertFalse(sqlrcur_validBind($cur,"var4"));
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# rebinding
	echo("REBINDING: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc ".
		"	@in1 int, ".
		"	@out1 int output as "."select @out1=@in1"));
	sqlrcur_prepareQuery($cur,"exec testproc");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_defineOutputBindInteger($cur,"out1");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out1"),1);
	sqlrcur_inputBind($cur,"in1",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out1"),2);
	sqlrcur_inputBind($cur,"in1",3);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out1"),3);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# reexecute
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
	sqlrcur_prepareQuery($cur,"select cast(@1 as int)");
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


	# stored procedure returning no value
	echo("STORED PROCEDURE "."RETURNING NO VALUE: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc ".
		"	@in1 int, ".
		"	@in2 float, ".
		"	@in3 varchar(20) as "."return"));
	sqlrcur_prepareQuery($cur,"exec testproc");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_inputBind($cur,"in2",2.5,2,1);
	sqlrcur_inputBind($cur,"in3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# stored procedure returning
	# single value
	echo("STORED PROCEDURE "."RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc ".
		"	@in1 int, ".
		"	@in2 float, ".
		"	@in3 varchar(20), ".
		"	@out1 int output as "."select @out1=@in1"));
	sqlrcur_prepareQuery($cur,"exec testproc");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_inputBind($cur,"in2",2.5,2,1);
	sqlrcur_inputBind($cur,"in3","hello");
	sqlrcur_defineOutputBindInteger($cur,"out1");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out1"),1);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# stored procedure returning
	# multiple values
	echo("STORED PROCEDURE RETURNING "."MULTIPLE VALUES: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,"create procedure testproc ".
		"@in1 int, ".
		"	@in2 float, ".
		"	@in3 varchar(20), ".
		"	@out1 int output, ".
		"	@out2 float output, ".
		"	@out3 varchar(20) "."output as ".
		"select @out1=@in1, ".
		"	@out2=@in2, ".
		"	@out3=@in3"));
	sqlrcur_prepareQuery($cur,"exec testproc");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_inputBind($cur,"in2",2.5,2,1);
	sqlrcur_inputBind($cur,"in3","hello");
	sqlrcur_defineOutputBindInteger($cur,"out1");
	sqlrcur_defineOutputBindDouble($cur,"out2");
	sqlrcur_defineOutputBindString($cur,"out3",20);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out1"),1);
	assertEqDbl(sqlrcur_getOutputBindDouble($cur,"out2"),2.5);
	assertEqStr(sqlrcur_getOutputBindString($cur,"out3"),"hello");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# stored procedure returning result set
	echo("STORED PROCEDURE "."RETURNING RESULT SET: \n");
	sqlrcur_sendQuery($cur,"drop procedure "."testselectproc");
	assertTrue(sqlrcur_sendQuery($cur,"create procedure ".
		"testselectproc as ".
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
		"	select 8"));
	assertTrue(sqlrcur_sendQuery($cur,"exec testselectproc"));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure "."testselectproc"));
	echo("\n");


	# temporary tables
	echo("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery($cur,"drop table #temptable\n");
	sqlrcur_sendQuery($cur,"create table #temptable "."(col1 int)");
	assertTrue(sqlrcur_sendQuery($cur,"insert into #temptable ".
		"values (1)"));
	assertTrue(sqlrcur_sendQuery($cur,"select count(*) "."from #temptable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcon_endSession($con);
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"select count(*) "."from #temptable"));
	echo("\n");


	# encoded binary data
	echo("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 image)"));
	$buffer="";
	for ($bi=0; $bi<256; $bi++) {
		$buffer.=chr($bi);
	}
	$querystr="insert into testtable "."values (0x";
	for ($i=0; $i<256; $i++) {
		$querystr.=sprintf("%02x",ord($buffer[$i]));
	}
	$querystr.=")";
	assertTrue(sqlrcur_sendQuery($cur,$querystr));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),256);
	assertTrue(strcmp(sqlrcur_getField($cur,0,0),$buffer)==0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# quotes
	echo("QUOTES: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values ('''''')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 "."from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertTrue(strcmp(sqlrcur_getField($cur,0,0),"''")==0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# last insert id
	echo("LAST INSERT ID: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable ".
		"	(col1 int identity ".
		"primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"(col2) values (1)"));
	assertEqInt(sqlrcon_getLastInsertId($con),1);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# database is schema
	echo("DATABASE IS SCHEMA: \n");
	assertFalse(sqlrcon_getDatabaseIsSchema($con));
	echo("\n");


	# catalog list
	echo("CATALOG LIST: \n");
	assertTrue(sqlrcur_getCatalogList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	assertInResultSet($cur,"Database",$hostname);
	echo("\n");


	# schema list
	echo("SCHEMA LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	# the get schema list query that is
	# used with sap will only return the
	# names of schemas that have at least
	# one database object in them, so to
	# be sure that there is one, we'll
	# create a table
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 int)"));
	assertTrue(sqlrcur_getSchemaList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	assertInResultSet($cur,"Database","dbo");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# table type list
	echo("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList($cur));
	assertEqStr(sqlrcur_getColumnName($cur,0),"table_type");
	assertInResultSet($cur,"table_type","TABLE");
	echo("\n");


	# table list
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
	assertInResultSet($cur,"Tables_in_xxx","testtable1");
	assertInResultSet($cur,"Tables_in_xxx","testtable2");
	assertInResultSet($cur,"Tables_in_xxx","testtable3");
	assertInResultSet($cur,"Tables_in_xxx","testtable4");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable4"));
	echo("\n");


	# type info list
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
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"255");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"VARCHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"datetime"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"DATETIME");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"93");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"23");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"DATETIME");
	echo("\n");


	# column list
	echo("COLUMN LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testint int, ".
		"	testsmallint smallint, ".
		"	testtinyint tinyint, ".
		"	testreal real, ".
		"	testfloat float, ".
		"	testdecimal decimal(4,1), ".
		"	testnumeric numeric(4,1), ".
		"	testmoney money, ".
		"	testsmallmoney smallmoney, ".
		"	testdatetime datetime, ".
		"	testsmalldatetime ".
		"smalldatetime, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testbit bit, ".
		"	testtext text)"));
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
	assertTrue(strcmp(sqlrcur_getField($cur,0,"column_name"),
		"testint")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,1,"column_name"),
		"testsmallint")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,2,"column_name"),
		"testtinyint")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,3,"column_name"),
		"testreal")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,4,"column_name"),
		"testfloat")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,5,"column_name"),
		"testdecimal")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,6,"column_name"),
		"testnumeric")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,7,"column_name"),
		"testmoney")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,8,"column_name"),
		"testsmallmoney")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,9,"column_name"),
		"testdatetime")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,10,"column_name"),
		"testsmalldatetime")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,11,"column_name"),
		"testchar")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,12,"column_name"),
		"testvarchar")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,13,"column_name"),
		"testbit")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,14,"column_name"),
		"testtext")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,0,"data_type"),"int")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,1,"data_type"),
		"smallint")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,2,"data_type"),
		"tinyint")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,3,"data_type"),"real")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,4,"data_type"),"float")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,5,"data_type"),
		"decimal")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,6,"data_type"),
		"numeric")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,7,"data_type"),"money")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,8,"data_type"),
		"smallmoney")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,9,"data_type"),
		"datetime")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,10,"data_type"),
		"smalldatetime")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,11,"data_type"),"char")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,12,"data_type"),
		"varchar")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,13,"data_type"),"bit")==0);
	assertTrue(strcmp(sqlrcur_getField($cur,14,"data_type"),"text")==0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# column list - auto_increment,
	# primary key
	echo("COLUMN LIST - "."auto_increment, "."primary key: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int identity ".
		"primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertTrue(strstr(sqlrcur_getField($cur,0,"extra"),
		"auto_increment")!==false);
	assertTrue(strstr(sqlrcur_getField($cur,0,"column_key"),
		"PRI")!==false);
	assertFalse(strstr(sqlrcur_getField($cur,1,"extra"),
		"auto_increment")!==false);
	assertFalse(strstr(sqlrcur_getField($cur,1,"column_key"),
		"PRI")!==false);
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int ".
		"primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertFalse(strstr(sqlrcur_getField($cur,0,"extra"),
		"auto_increment")!==false);
	assertTrue(strstr(sqlrcur_getField($cur,0,"column_key"),
		"PRI")!==false);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# primary keys list
	echo("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int ".
		"primary key, ".
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
	assertTrue(strcmp(sqlrcur_getField($cur,0,"table"),"testtable")==0);
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(strcmp(sqlrcur_getField($cur,0,"column_name"),"col1")==0);
	$kn=sqlrcur_getField($cur,0,"key_name");
	assertTrue($kn!==NULL && strlen($kn)>0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# key and index list
	echo("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int ".
		"primary key, ".
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
	assertTrue(strcmp(sqlrcur_getField($cur,0,"table"),"testtable")==0);
	assertEqStr(sqlrcur_getField($cur,0,"non_unique"),"FALSE");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(strcmp(sqlrcur_getField($cur,0,"column_name"),"col1")==0);
	assertEqStr(sqlrcur_getField($cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getField($cur,0,"index_type"),"1");
	$kn=sqlrcur_getField($cur,0,"key_name");
	assertTrue($kn!==NULL && strlen($kn)>0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# procedure list
	echo("PROCEDURE LIST: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc1");
	sqlrcur_sendQuery($cur,"drop procedure testproc2");
	sqlrcur_sendQuery($cur,"drop procedure testproc3");
	sqlrcur_sendQuery($cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc1 ".
		"	@in1 int, ".
		"	@in2 char(20), ".
		"	@in3 varchar(20), ".
		"	@in4 datetime "."as select 1"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc2 ".
		"	@in1 int, ".
		"	@in2 char(20), ".
		"	@in3 varchar(20), ".
		"	@in4 datetime "."as select 1"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc3 ".
		"	@in1 int, ".
		"	@in2 char(20), ".
		"	@in3 varchar(20), ".
		"	@in4 datetime "."as select 1"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc4 ".
		"	@in1 int, ".
		"	@in2 char(20), ".
		"	@in3 varchar(20), ".
		"	@in4 datetime "."as select 1"));
	assertTrue(sqlrcur_getProcedureList($cur,NULL));
	assertInResultSet($cur,"routine_name","testproc1");
	assertInResultSet($cur,"routine_name","testproc2");
	assertInResultSet($cur,"routine_name","testproc3");
	assertInResultSet($cur,"routine_name","testproc4");
	echo("\n");


	# procedure parameter list
	echo("PROCEDURE "."PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList($cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName($cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName($cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName($cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName($cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount($cur),4);
	assertEqStr(sqlrcur_getField($cur,0,"parameter_name"),"@in1");
	assertEqStr(sqlrcur_getField($cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"int");
	assertEqStr(sqlrcur_getField($cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_name"),"@in2");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"char");
	assertEqStr(sqlrcur_getField($cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_name"),"@in3");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"varchar");
	assertEqStr(sqlrcur_getField($cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_name"),"@in4");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"datetime");
	assertEqStr(sqlrcur_getField($cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc4"));
	echo("\n");


	# invalid queries
	echo("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
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
