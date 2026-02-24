<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.
	include("./asserts.php");


	$host="sqlrelay";
	$port=9000;
	$socket="/tmp/test.socket";
	$user="testuser";
	$password="testpassword";

	$bindvars=array("1","2","3","4","5","6","7","8","9",
				"10","11","12","13");
	$bindvals=array("4","4","4","4.4","4.4","4.4","4.4",
				"4.00","4.00",
				"01-Jan-2004 04:00:00",
				"01-Jan-2004 04:00:00",
				"testchar4","testvarchar4");
	$arraybindvars=array("var1","var2","var3","var4","var5","var6",
				"var7","var8","var9","var10","var11","var12",
				"var13");
	$arraybindvals=array("7","7","7","7.7","7.7","7.7","7.7",
				"7.00","7.00",
				"01-Jan-2007 07:00:00",
				"01-Jan-2007 07:00:00",
				"testchar7","testvarchar7");


	# instantiation
	$con=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$cur=sqlrcur_alloc($con);

	# get database type


	# identify
	echo("IDENTIFY: \n");
	assertEqual(sqlrcon_identify($con),"sap");
	echo("\n");


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	# isolation levels
	echo("ISOLATION LEVELS: \n");
	$isolationlevels=array("1","0","2","3");
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


	# create temptable
	echo("CREATE TEMPTABLE: \n");
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
		"	testsmalldatetime smalldatetime, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testbit bit)"));
	echo("\n");


	# create stored procedures
	echo("CREATE STORED PROCEDURES: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc @in1 int, ".
		"	@in2 float, ".
		"	@in3 varchar(20), ".
		"	@out1 int output, ".
		"	@out2 float output, ".
		"	@out3 varchar(20) output as select @out1=@in1, ".
		"	@out2=@in2, ".
		"	@out3=@in3"));
	sqlrcur_sendQuery($cur,"drop procedure testselectproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testselectproc as select ".
		"	* from testtable order by testint"));
	echo("\n");


	# begin transaction
	echo("BEGIN TRANSACTION: \n");
	#assertTrue(sqlrcur_sendQuery($cur,"begin tran"));
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
		"	1.1, ".
		"	1.1, ".
		"	1.1, ".
		"	1.1, ".
		"	1.00, ".
		"	1.00, ".
		"	'01-Jan-2001 01:00:00', ".
		"	'01-Jan-2001 01:00:00', ".
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	1)"));
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
		"	@var14)");
	assertEqual(sqlrcur_countBindVariables($cur),14);
	sqlrcur_inputBind($cur,"1",2);
	sqlrcur_inputBind($cur,"2",2);
	sqlrcur_inputBind($cur,"3",2);
	sqlrcur_inputBind($cur,"4",2.2,2,1);
	sqlrcur_inputBind($cur,"5",2.2,2,1);
	sqlrcur_inputBind($cur,"6",2.2,2,1);
	sqlrcur_inputBind($cur,"7",2.2,2,1);
	sqlrcur_inputBind($cur,"8",2.00,3,2);
	sqlrcur_inputBind($cur,"9",2.00,3,2);
	sqlrcur_inputBind($cur,"10","01-Jan-2002 02:00:00");
	sqlrcur_inputBind($cur,"11","01-Jan-2002 02:00:00");
	sqlrcur_inputBind($cur,"12","testchar2");
	sqlrcur_inputBind($cur,"13","testvarchar2");
	sqlrcur_inputBind($cur,"14",1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",3);
	sqlrcur_inputBind($cur,"2",3);
	sqlrcur_inputBind($cur,"3",3);
	sqlrcur_inputBind($cur,"4",3.3,2,1);
	sqlrcur_inputBind($cur,"5",3.3,2,1);
	sqlrcur_inputBind($cur,"6",3.3,2,1);
	sqlrcur_inputBind($cur,"7",3.3,2,1);
	sqlrcur_inputBind($cur,"8",3.00,3,2);
	sqlrcur_inputBind($cur,"9",3.00,3,2);
	sqlrcur_inputBind($cur,"10","01-Jan-2003 03:00:00");
	sqlrcur_inputBind($cur,"11","01-Jan-2003 03:00:00");
	sqlrcur_inputBind($cur,"12","testchar3");
	sqlrcur_inputBind($cur,"13","testvarchar3");
	sqlrcur_inputBind($cur,"14",1);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of binds by position
	echo("ARRAY OF BINDS BY POSITION: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("1","2","3","4","5","6","7",
			"8","9","10","11","12","13","14");
	$bindvals=array(4,4,4,4.4,4.4,4.4,4.4,
			4.00,4.00,
			"01-Jan-2004 04:00:00",
			"01-Jan-2004 04:00:00",
			"testchar4","testvarchar4",1);
	$precs=array(0,0,0,2,2,2,2,3,3,0,0,0,0,0);
	$scales=array(0,0,0,1,1,1,1,2,2,0,0,0,0,0);
	sqlrcur_inputBinds($cur,$bindvars,$bindvals,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# bind by name
	echo("BIND BY NAME: \n");
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
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",6);
	sqlrcur_inputBind($cur,"var2",6);
	sqlrcur_inputBind($cur,"var3",6);
	sqlrcur_inputBind($cur,"var4",6.6,2,1);
	sqlrcur_inputBind($cur,"var5",6.6,2,1);
	sqlrcur_inputBind($cur,"var6",6.6,2,1);
	sqlrcur_inputBind($cur,"var7",6.6,2,1);
	sqlrcur_inputBind($cur,"var8",6.00,3,2);
	sqlrcur_inputBind($cur,"var9",6.00,3,2);
	sqlrcur_inputBind($cur,"var10","01-Jan-2006 06:00:00");
	sqlrcur_inputBind($cur,"var11","01-Jan-2006 06:00:00");
	sqlrcur_inputBind($cur,"var12","testchar6");
	sqlrcur_inputBind($cur,"var13","testvarchar6");
	sqlrcur_inputBind($cur,"var14",1);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of binds by name
	echo("ARRAY OF BINDS BY NAME: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("var1","var2","var3","var4",
			"var5","var6","var7","var8",
			"var9","var10","var11","var12",
			"var13","var14");
	$bindvals=array(7,7,7,7.7,7.7,7.7,7.7,
			7.00,7.00,
			"01-Jan-2007 07:00:00",
			"01-Jan-2007 07:00:00",
			"testchar7","testvarchar7",1);
	$precs=array(0,0,0,2,2,2,2,3,3,0,0,0,0);
	$scales=array(0,0,0,1,1,1,1,2,2,0,0,0,0);
	sqlrcur_inputBinds($cur,$bindvars,$bindvals,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# bind by name with validation
	echo("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",8);
	sqlrcur_inputBind($cur,"var2",8);
	sqlrcur_inputBind($cur,"var3",8);
	sqlrcur_inputBind($cur,"var4",8.8,2,1);
	sqlrcur_inputBind($cur,"var5",8.8,2,1);
	sqlrcur_inputBind($cur,"var6",8.8,2,1);
	sqlrcur_inputBind($cur,"var7",8.8,2,1);
	sqlrcur_inputBind($cur,"var8",8.00,3,2);
	sqlrcur_inputBind($cur,"var9",8.00,3,2);
	sqlrcur_inputBind($cur,"var10","01-Jan-2008 08:00:00");
	sqlrcur_inputBind($cur,"var11","01-Jan-2008 08:00:00");
	sqlrcur_inputBind($cur,"var12","testchar8");
	sqlrcur_inputBind($cur,"var13","testvarchar8");
	sqlrcur_inputBind($cur,"var14",1);
	sqlrcur_inputBind($cur,"var15","junkvalue");
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# stored procedure
	echo("STORED PROCEDURE: \n");
	// return multiple values
	sqlrcur_prepareQuery($cur,"exec testproc");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_inputBind($cur,"in2",1.1,2,1);
	sqlrcur_inputBind($cur,"in3","hello");
	sqlrcur_defineOutputBindInteger($cur,"out1");
	sqlrcur_defineOutputBindDouble($cur,"out2");
	sqlrcur_defineOutputBindString($cur,"out3",20);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqual(sqlrcur_getOutputBindInteger($cur,"out1"),1);
	assertEqual(sqlrcur_getOutputBindDouble($cur,"out2"),1.1);
	assertEqual(sqlrcur_getOutputBindString($cur,"out3"),"hello");
	echo("\n");


	# select
	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testint "));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqual(sqlrcur_colCount($cur),14);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqual(sqlrcur_getColumnName($cur,2),"testtinyint");
	assertEqual(sqlrcur_getColumnName($cur,3),"testreal");
	assertEqual(sqlrcur_getColumnName($cur,4),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,5),"testdecimal");
	assertEqual(sqlrcur_getColumnName($cur,6),"testnumeric");
	assertEqual(sqlrcur_getColumnName($cur,7),"testmoney");
	assertEqual(sqlrcur_getColumnName($cur,8),"testsmallmoney");
	assertEqual(sqlrcur_getColumnName($cur,9),"testdatetime");
	assertEqual(sqlrcur_getColumnName($cur,10),"testsmalldatetime");
	assertEqual(sqlrcur_getColumnName($cur,11),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,12),"testvarchar");
	assertEqual(sqlrcur_getColumnName($cur,13),"testbit");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testint");
	assertEqual($cols[1],"testsmallint");
	assertEqual($cols[2],"testtinyint");
	assertEqual($cols[3],"testreal");
	assertEqual($cols[4],"testfloat");
	assertEqual($cols[5],"testdecimal");
	assertEqual($cols[6],"testnumeric");
	assertEqual($cols[7],"testmoney");
	assertEqual($cols[8],"testsmallmoney");
	assertEqual($cols[9],"testdatetime");
	assertEqual($cols[10],"testsmalldatetime");
	assertEqual($cols[11],"testchar");
	assertEqual($cols[12],"testvarchar");
	assertEqual($cols[13],"testbit");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqual(sqlrcur_getColumnType($cur,0),"INT");
	assertEqual(sqlrcur_getColumnType($cur,"testint"),"INT");
	assertEqual(sqlrcur_getColumnType($cur,1),"SMALLINT");
	assertEqual(sqlrcur_getColumnType($cur,"testsmallint"),"SMALLINT");
	assertEqual(sqlrcur_getColumnType($cur,2),"TINYINT");
	assertEqual(sqlrcur_getColumnType($cur,"testtinyint"),"TINYINT");
	assertEqual(sqlrcur_getColumnType($cur,3),"REAL");
	assertEqual(sqlrcur_getColumnType($cur,"testreal"),"REAL");
	assertEqual(sqlrcur_getColumnType($cur,4),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,"testfloat"),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,5),"DECIMAL");
	assertEqual(sqlrcur_getColumnType($cur,"testdecimal"),"DECIMAL");
	assertEqual(sqlrcur_getColumnType($cur,6),"NUMERIC");
	assertEqual(sqlrcur_getColumnType($cur,"testnumeric"),"NUMERIC");
	assertEqual(sqlrcur_getColumnType($cur,7),"MONEY");
	assertEqual(sqlrcur_getColumnType($cur,"testmoney"),"MONEY");
	assertEqual(sqlrcur_getColumnType($cur,8),"SMALLMONEY");
	assertEqual(sqlrcur_getColumnType($cur,"testsmallmoney"),"SMALLMONEY");
	assertEqual(sqlrcur_getColumnType($cur,9),"DATETIME");
	assertEqual(sqlrcur_getColumnType($cur,"testdatetime"),"DATETIME");
	assertEqual(sqlrcur_getColumnType($cur,10),"SMALLDATETIME");
	assertEqual(sqlrcur_getColumnType($cur,"testsmalldatetime"),"SMALLDATETIME");
	assertEqual(sqlrcur_getColumnType($cur,11),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,"testchar"),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,12),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,"testvarchar"),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,13),"BIT");
	assertEqual(sqlrcur_getColumnType($cur,"testbit"),"BIT");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqual(sqlrcur_getColumnLength($cur,0),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testint"),4);
	assertEqual(sqlrcur_getColumnLength($cur,1),2);
	assertEqual(sqlrcur_getColumnLength($cur,"testsmallint"),2);
	assertEqual(sqlrcur_getColumnLength($cur,2),1);
	assertEqual(sqlrcur_getColumnLength($cur,"testtinyint"),1);
	assertEqual(sqlrcur_getColumnLength($cur,3),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testreal"),4);
	assertEqual(sqlrcur_getColumnLength($cur,4),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testfloat"),8);
	assertEqual(sqlrcur_getColumnLength($cur,5),35);
	assertEqual(sqlrcur_getColumnLength($cur,"testdecimal"),35);
	assertEqual(sqlrcur_getColumnLength($cur,6),35);
	assertEqual(sqlrcur_getColumnLength($cur,"testnumeric"),35);
	assertEqual(sqlrcur_getColumnLength($cur,7),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testmoney"),8);
	assertEqual(sqlrcur_getColumnLength($cur,8),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testsmallmoney"),4);
	assertEqual(sqlrcur_getColumnLength($cur,9),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testdatetime"),8);
	assertEqual(sqlrcur_getColumnLength($cur,10),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testsmalldatetime"),4);
	assertEqual(sqlrcur_getColumnLength($cur,11),40);
	assertEqual(sqlrcur_getColumnLength($cur,"testchar"),40);
	assertEqual(sqlrcur_getColumnLength($cur,12),40);
	assertEqual(sqlrcur_getColumnLength($cur,"testvarchar"),40);
	assertEqual(sqlrcur_getColumnLength($cur,13),1);
	assertEqual(sqlrcur_getColumnLength($cur,"testbit"),1);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqual(sqlrcur_getLongest($cur,0),1);
	assertEqual(sqlrcur_getLongest($cur,"testint"),1);
	assertEqual(sqlrcur_getLongest($cur,1),1);
	assertEqual(sqlrcur_getLongest($cur,"testsmallint"),1);
	assertEqual(sqlrcur_getLongest($cur,2),1);
	assertEqual(sqlrcur_getLongest($cur,"testtinyint"),1);
	assertEqual(sqlrcur_getLongest($cur,3),18);
	assertEqual(sqlrcur_getLongest($cur,"testreal"),18);
	assertEqual(sqlrcur_getLongest($cur,4),18);
	assertEqual(sqlrcur_getLongest($cur,"testfloat"),18);
	assertEqual(sqlrcur_getLongest($cur,5),3);
	assertEqual(sqlrcur_getLongest($cur,"testdecimal"),3);
	assertEqual(sqlrcur_getLongest($cur,6),3);
	assertEqual(sqlrcur_getLongest($cur,"testnumeric"),3);
	assertEqual(sqlrcur_getLongest($cur,7),4);
	assertEqual(sqlrcur_getLongest($cur,"testmoney"),4);
	assertEqual(sqlrcur_getLongest($cur,8),4);
	assertEqual(sqlrcur_getLongest($cur,"testsmallmoney"),4);
	assertEqual(sqlrcur_getLongest($cur,9),19);
	assertEqual(sqlrcur_getLongest($cur,"testdatetime"),19);
	assertEqual(sqlrcur_getLongest($cur,10),19);
	assertEqual(sqlrcur_getLongest($cur,"testsmalldatetime"),19);
	assertEqual(sqlrcur_getLongest($cur,11),40);
	assertEqual(sqlrcur_getLongest($cur,"testchar"),40);
	assertEqual(sqlrcur_getLongest($cur,12),12);
	assertEqual(sqlrcur_getLongest($cur,"testvarchar"),12);
	assertEqual(sqlrcur_getLongest($cur,13),1);
	assertEqual(sqlrcur_getLongest($cur,"testbit"),1);
	echo("\n");


	# row count
	echo("ROW COUNT: \n");
	assertEqual(sqlrcur_rowCount($cur),8);
	echo("\n");


	# total rows
	echo("TOTAL ROWS: \n");
	assertEqual(sqlrcur_totalRows($cur),0);
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
	#assertEqual(sqlrcur_getField($cur,0,3),"1.1");
	#assertEqual(sqlrcur_getField($cur,0,4),"1.1");
	assertEqual(sqlrcur_getField($cur,0,5),"1.1");
	assertEqual(sqlrcur_getField($cur,0,6),"1.1");
	assertEqual(sqlrcur_getField($cur,0,7),"1.00");
	assertEqual(sqlrcur_getField($cur,0,8),"1.00");
	assertEqual(sqlrcur_getField($cur,0,9),"Jan  1 2001  1:00AM");
	assertEqual(sqlrcur_getField($cur,0,10),"Jan  1 2001  1:00AM");
	assertEqual(sqlrcur_getField($cur,0,11),"testchar1                               ");
	assertEqual(sqlrcur_getField($cur,0,12),"testvarchar1");
	assertEqual(sqlrcur_getField($cur,0,13),"1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,7,1),"8");
	assertEqual(sqlrcur_getField($cur,7,2),"8");
	#assertEqual(sqlrcur_getField($cur,7,3),"8.8");
	#assertEqual(sqlrcur_getField($cur,7,4),"8.8");
	assertEqual(sqlrcur_getField($cur,7,5),"8.8");
	assertEqual(sqlrcur_getField($cur,7,6),"8.8");
	assertEqual(sqlrcur_getField($cur,7,7),"8.00");
	assertEqual(sqlrcur_getField($cur,7,8),"8.00");
	assertEqual(sqlrcur_getField($cur,7,9),"Jan  1 2008  8:00AM");
	assertEqual(sqlrcur_getField($cur,7,10),"Jan  1 2008  8:00AM");
	assertEqual(sqlrcur_getField($cur,7,11),"testchar8                               ");
	assertEqual(sqlrcur_getField($cur,7,12),"testvarchar8");
	assertEqual(sqlrcur_getField($cur,7,13),"1");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,1),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,2),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,3),18);
	assertEqual(sqlrcur_getFieldLength($cur,0,4),18);
	assertEqual(sqlrcur_getFieldLength($cur,0,5),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,6),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,7),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,8),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,9),19);
	assertEqual(sqlrcur_getFieldLength($cur,0,10),19);
	assertEqual(sqlrcur_getFieldLength($cur,0,11),40);
	assertEqual(sqlrcur_getFieldLength($cur,0,12),12);
	assertEqual(sqlrcur_getFieldLength($cur,0,13),1);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,1),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,2),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,3),18);
	assertEqual(sqlrcur_getFieldLength($cur,7,4),18);
	assertEqual(sqlrcur_getFieldLength($cur,7,5),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,6),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,7),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,8),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,9),19);
	assertEqual(sqlrcur_getFieldLength($cur,7,10),19);
	assertEqual(sqlrcur_getFieldLength($cur,7,11),40);
	assertEqual(sqlrcur_getFieldLength($cur,7,12),12);
	assertEqual(sqlrcur_getFieldLength($cur,7,13),1);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqual(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testsmallint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testtinyint"),"1");
	#assertEqual(sqlrcur_getField($cur,0,"testreal"),"1.1");
	#assertEqual(sqlrcur_getField($cur,0,"testfloat"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testdecimal"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testnumeric"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testmoney"),"1.00");
	assertEqual(sqlrcur_getField($cur,0,"testsmallmoney"),"1.00");
	assertEqual(sqlrcur_getField($cur,0,"testdatetime"),"Jan  1 2001  1:00AM");
	assertEqual(sqlrcur_getField($cur,0,"testsmalldatetime"),"Jan  1 2001  1:00AM");
	assertEqual(sqlrcur_getField($cur,0,"testchar"),"testchar1                               ");
	assertEqual(sqlrcur_getField($cur,0,"testvarchar"),"testvarchar1");
	assertEqual(sqlrcur_getField($cur,0,"testbit"),"1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testsmallint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testtinyint"),"8");
	#assertEqual(sqlrcur_getField($cur,7,"testreal"),"8.8");
	#assertEqual(sqlrcur_getField($cur,7,"testfloat"),"8.8");
	assertEqual(sqlrcur_getField($cur,7,"testdecimal"),"8.8");
	assertEqual(sqlrcur_getField($cur,7,"testnumeric"),"8.8");
	assertEqual(sqlrcur_getField($cur,7,"testmoney"),"8.00");
	assertEqual(sqlrcur_getField($cur,7,"testsmallmoney"),"8.00");
	assertEqual(sqlrcur_getField($cur,7,"testdatetime"),"Jan  1 2008  8:00AM");
	assertEqual(sqlrcur_getField($cur,7,"testsmalldatetime"),"Jan  1 2008  8:00AM");
	assertEqual(sqlrcur_getField($cur,7,"testchar"),"testchar8                               ");
	assertEqual(sqlrcur_getField($cur,7,"testvarchar"),"testvarchar8");
	assertEqual(sqlrcur_getField($cur,7,"testbit"),"1");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testsmallint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testtinyint"),1);
	#assertEqual(sqlrcur_getFieldLength($cur,0,"testreal"),3);
	#assertEqual(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testdecimal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testnumeric"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testmoney"),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testsmallmoney"),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testdatetime"),19);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testsmalldatetime"),19);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testchar"),40);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testvarchar"),12);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testbit"),1);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testsmallint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testtinyint"),1);
	#assertEqual(sqlrcur_getFieldLength($cur,7,"testreal"),3);
	#assertEqual(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testdecimal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testnumeric"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testmoney"),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testsmallmoney"),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testdatetime"),19);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testsmalldatetime"),19);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testchar"),40);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testvarchar"),12);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testbit"),1);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqual($fields[0],"1");
	assertEqual($fields[1],"1");
	assertEqual($fields[2],"1");
	#assertEqual($fields[3],"1.1");
	#assertEqual($fields[4],"1.1");
	assertEqual($fields[5],"1.1");
	assertEqual($fields[6],"1.1");
	assertEqual($fields[7],"1.00");
	assertEqual($fields[8],"1.00");
	assertEqual($fields[9],"Jan  1 2001  1:00AM");
	assertEqual($fields[10],"Jan  1 2001  1:00AM");
	assertEqual($fields[11],"testchar1                               ");
	assertEqual($fields[12],"testvarchar1");
	assertEqual($fields[13],"1");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqual($fieldlens[0],1);
	assertEqual($fieldlens[1],1);
	assertEqual($fieldlens[2],1);
	#assertEqual($fieldlens[3],3);
	#assertEqual($fieldlens[4],3);
	assertEqual($fieldlens[5],3);
	assertEqual($fieldlens[6],3);
	assertEqual($fieldlens[7],4);
	assertEqual($fieldlens[8],4);
	assertEqual($fieldlens[9],19);
	assertEqual($fieldlens[10],19);
	assertEqual($fieldlens[11],40);
	assertEqual($fieldlens[12],12);
	assertEqual($fieldlens[13],1);
	echo("\n");


	# fields by associative array
	echo("FIELDS BY ASSOCIATIVE ARRAY: \n");
	$fields=sqlrcur_getRowAssoc($cur,0);
	assertEqual($fields["testint"],"1");
	assertEqual($fields["testsmallint"],"1");
	assertEqual($fields["testtinyint"],"1");
	#assertEqual($fields["testreal"],"1.1");
	#assertEqual($fields["testfloat"],"1.1");
	assertEqual($fields["testdecimal"],"1.1");
	assertEqual($fields["testnumeric"],"1.1");
	assertEqual($fields["testmoney"],"1.00");
	assertEqual($fields["testsmallmoney"],"1.00");
	assertEqual($fields["testdatetime"],"Jan  1 2001  1:00AM");
	assertEqual($fields["testsmalldatetime"],"Jan  1 2001  1:00AM");
	assertEqual($fields["testchar"],"testchar1                               ");
	assertEqual($fields["testvarchar"],"testvarchar1");
	assertEqual($fields["testbit"],"1");
	echo("\n");
	$fields=sqlrcur_getRowAssoc($cur,7);
	assertEqual($fields["testint"],"8");
	assertEqual($fields["testsmallint"],"8");
	assertEqual($fields["testtinyint"],"8");
	#assertEqual($fields["testreal"],"8.8");
	#assertEqual($fields["testfloat"],"8.8");
	assertEqual($fields["testdecimal"],"8.8");
	assertEqual($fields["testnumeric"],"8.8");
	assertEqual($fields["testmoney"],"8.00");
	assertEqual($fields["testsmallmoney"],"8.00");
	assertEqual($fields["testdatetime"],"Jan  1 2008  8:00AM");
	assertEqual($fields["testsmalldatetime"],"Jan  1 2008  8:00AM");
	assertEqual($fields["testchar"],"testchar8                               ");
	assertEqual($fields["testvarchar"],"testvarchar8");
	assertEqual($fields["testbit"],"1");
	echo("\n");


	# field lengths by associative array
	echo("FIELD LENGTHS BY ASSOCIATIVE ARRAY: \n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,0);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testsmallint"],1);
	assertEqual($fieldlengths["testtinyint"],1);
	#assertEqual($fieldlengths["testreal"],3);
	#assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testdecimal"],3);
	assertEqual($fieldlengths["testnumeric"],3);
	assertEqual($fieldlengths["testmoney"],4);
	assertEqual($fieldlengths["testsmallmoney"],4);
	assertEqual($fieldlengths["testdatetime"],19);
	assertEqual($fieldlengths["testsmalldatetime"],19);
	assertEqual($fieldlengths["testchar"],40);
	assertEqual($fieldlengths["testvarchar"],12);
	assertEqual($fieldlengths["testbit"],1);
	echo("\n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,7);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testsmallint"],1);
	assertEqual($fieldlengths["testtinyint"],1);
	#assertEqual($fieldlengths["testreal"],3);
	#assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testdecimal"],3);
	assertEqual($fieldlengths["testnumeric"],3);
	assertEqual($fieldlengths["testmoney"],4);
	assertEqual($fieldlengths["testsmallmoney"],4);
	assertEqual($fieldlengths["testdatetime"],19);
	assertEqual($fieldlengths["testsmalldatetime"],19);
	assertEqual($fieldlengths["testchar"],40);
	assertEqual($fieldlengths["testvarchar"],12);
	assertEqual($fieldlengths["testbit"],1);
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
		"	testint "));
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
		"	testint "));
	assertEqual(sqlrcur_getColumnName($cur,0),NULL);
	assertEqual(sqlrcur_getColumnLength($cur,0),0);
	assertEqual(sqlrcur_getColumnType($cur,0),NULL);
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testint "));
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnLength($cur,0),4);
	assertEqual(sqlrcur_getColumnType($cur,0),"INT");
	echo("\n");


	# suspended session
	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testint "));
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
		"	testint "));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$conport=sqlrcon_getConnectionPort($con);
	$consocket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$conport,$consocket));
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
		"	testint "));
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
		"	testint "));
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
		"	testint "));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqual($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_colCount($cur),14);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqual(sqlrcur_getColumnName($cur,2),"testtinyint");
	assertEqual(sqlrcur_getColumnName($cur,3),"testreal");
	assertEqual(sqlrcur_getColumnName($cur,4),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,5),"testdecimal");
	assertEqual(sqlrcur_getColumnName($cur,6),"testnumeric");
	assertEqual(sqlrcur_getColumnName($cur,7),"testmoney");
	assertEqual(sqlrcur_getColumnName($cur,8),"testsmallmoney");
	assertEqual(sqlrcur_getColumnName($cur,9),"testdatetime");
	assertEqual(sqlrcur_getColumnName($cur,10),"testsmalldatetime");
	assertEqual(sqlrcur_getColumnName($cur,11),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,12),"testvarchar");
	assertEqual(sqlrcur_getColumnName($cur,13),"testbit");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testint");
	assertEqual($cols[1],"testsmallint");
	assertEqual($cols[2],"testtinyint");
	assertEqual($cols[3],"testreal");
	assertEqual($cols[4],"testfloat");
	assertEqual($cols[5],"testdecimal");
	assertEqual($cols[6],"testnumeric");
	assertEqual($cols[7],"testmoney");
	assertEqual($cols[8],"testsmallmoney");
	assertEqual($cols[9],"testdatetime");
	assertEqual($cols[10],"testsmalldatetime");
	assertEqual($cols[11],"testchar");
	assertEqual($cols[12],"testvarchar");
	assertEqual($cols[13],"testbit");
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
		"	testint "));
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
		"	testint "));
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


	# stored procedure with result set
	echo("STORED PROCEDURE WITH RESULT SET: \n");
	assertTrue(sqlrcur_sendQuery($cur,"exec testselectproc"));
	echo("\n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"1");
	assertEqual(sqlrcur_getField($cur,0,2),"1");
	//assertEqual(sqlrcur_getField($cur,0,3),"1.1");
	//assertEqual(sqlrcur_getField($cur,0,4),"1.1");
	assertEqual(sqlrcur_getField($cur,0,5),"1.1");
	assertEqual(sqlrcur_getField($cur,0,6),"1.1");
	assertEqual(sqlrcur_getField($cur,0,7),"1.00");
	assertEqual(sqlrcur_getField($cur,0,8),"1.00");
	assertEqual(sqlrcur_getField($cur,0,9),"Jan  1 2001  1:00AM");
	assertEqual(sqlrcur_getField($cur,0,10),"Jan  1 2001  1:00AM");
	assertEqual(sqlrcur_getField($cur,0,11),"testchar1                               ");
	assertEqual(sqlrcur_getField($cur,0,12),"testvarchar1");
	assertEqual(sqlrcur_getField($cur,0,13),"1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,7,1),"8");
	assertEqual(sqlrcur_getField($cur,7,2),"8");
	//assertEqual(sqlrcur_getField($cur,7,3),"8.8");
	//assertEqual(sqlrcur_getField($cur,7,4),"8.8");
	assertEqual(sqlrcur_getField($cur,7,5),"8.8");
	assertEqual(sqlrcur_getField($cur,7,6),"8.8");
	assertEqual(sqlrcur_getField($cur,7,7),"8.00");
	assertEqual(sqlrcur_getField($cur,7,8),"8.00");
	assertEqual(sqlrcur_getField($cur,7,9),"Jan  1 2008  8:00AM");
	assertEqual(sqlrcur_getField($cur,7,10),"Jan  1 2008  8:00AM");
	assertEqual(sqlrcur_getField($cur,7,11),"testchar8                               ");
	assertEqual(sqlrcur_getField($cur,7,12),"testvarchar8");
	assertEqual(sqlrcur_getField($cur,7,13),"1");
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
		"	testint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testint "));
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

	reportTestStatus();

	exit($status);
?></pre></html>
