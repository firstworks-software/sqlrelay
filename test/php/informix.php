<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.
	include("./asserts.php");


	$isolationlevels=array("committed read","dirty read",
				"cursor stability","repeatable read");
	$bindvars=array("1","2","3","4",
				"5","6","7","8","9","10",
				"11","12","13","14","15","16");
	$bindvals=array("t","7","7","7","7",
				"7.5","7.5","7.5","7.5",
				"testchar7","testnchar7",
				"testvarchar7","testnvarchar7",
				"testlvarchar7","01/01/2007",
				"2007-01-01 07:00:00");
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


	# instantiation
	$con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	$cur=sqlrcur_alloc($con);


	# identify
	echo("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify($con),"informix");
	echo("\n");


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	# transaction state
	echo("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel($con),"implicit");
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	assertTrue(sqlrcon_getInTransaction($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	echo("\n");


	# bind format
	echo("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat($con),"?");
	echo("\n");


	# nextval format
	echo("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat($con),"%s.nextval");
	echo("\n");


	# isolation levels
	echo("ISOLATION LEVELS: \n");
	foreach ($isolationlevels as $il) {
		# you can set the isolation level, but to get it, you have to
		# have permissions to read from sysmaster:syssqlcurses
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		echo("\n");
	}
	# reset to the default isolation level
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	echo("\n");


	# create testtable
	echo("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testboolean boolean, ".
		"	testsmallint smallint, ".
		"	testint integer, ".
		"	testbigint bigint, ".
		"	testint8 int8, ".
		"	testdecimal decimal(10,2), ".
		"	testmoney money, ".
		"	testsmallfloat smallfloat, ".
		"	testfloat float, ".
		"	testchar char(40), ".
		"	testnchar nchar(40), ".
		"	testvarchar varchar(40), ".
		"	testnvarchar nvarchar(40), ".
		"	testlvarchar lvarchar(40), ".
		"	testdate date, ".
		"	testdatetime datetime year to second, ".
		"	testtext text, ".
		"	testbyte byte)"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# insert
	echo("INSERT: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	't', ".
		"	1, ".
		"	1, ".
		"	1, ".
		"	1, ".
		"	1.5, ".
		"	1.5, ".
		"	1.5, ".
		"	1.5, ".
		"	'testchar1', ".
		"	'testnchar1', ".
		"	'testvarchar1', ".
		"	'testnvarchar1', ".
		"	'testlvarchar1', ".
		"	'01/01/2001', ".
		"	'2001-01-01 01:00:00', ".
		"	'testtext1', ".
		"	null)"));
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
		"	?)");
	assertEqInt(sqlrcur_countBindVariables($cur),18);
	sqlrcur_inputBind($cur,"1","t");
	sqlrcur_inputBind($cur,"2",2);
	sqlrcur_inputBind($cur,"3",2);
	sqlrcur_inputBind($cur,"4",2);
	sqlrcur_inputBind($cur,"5",2);
	sqlrcur_inputBind($cur,"6",2.5,4,2);
	sqlrcur_inputBind($cur,"7",2.5,4,2);
	sqlrcur_inputBind($cur,"8",2.5,4,2);
	sqlrcur_inputBind($cur,"9",2.5,4,2);
	sqlrcur_inputBind($cur,"10","testchar2");
	sqlrcur_inputBind($cur,"11","testnchar2");
	sqlrcur_inputBind($cur,"12","testvarchar2");
	sqlrcur_inputBind($cur,"13","testnvarchar2");
	sqlrcur_inputBind($cur,"14","testlvarchar2");
	sqlrcur_inputBindDate($cur,"15",2002,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate($cur,"16",2002,1,1,2,0,0,0,NULL,0);
	sqlrcur_inputBindClob($cur,"17","testtext2",strlen("testtext2"));
	sqlrcur_inputBindBlob($cur,"18","testbyte2",strlen("testbyte2"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1","t");
	sqlrcur_inputBind($cur,"2",3);
	sqlrcur_inputBind($cur,"3",3);
	sqlrcur_inputBind($cur,"4",3);
	sqlrcur_inputBind($cur,"5",3);
	sqlrcur_inputBind($cur,"6",3.5,4,2);
	sqlrcur_inputBind($cur,"7",3.5,4,2);
	sqlrcur_inputBind($cur,"8",3.5,4,2);
	sqlrcur_inputBind($cur,"9",3.5,4,2);
	sqlrcur_inputBind($cur,"10","testchar3");
	sqlrcur_inputBind($cur,"11","testnchar3");
	sqlrcur_inputBind($cur,"12","testvarchar3");
	sqlrcur_inputBind($cur,"13","testnvarchar3");
	sqlrcur_inputBind($cur,"14","testlvarchar3");
	sqlrcur_inputBindDate($cur,"15",2003,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate($cur,"16",2003,1,1,3,0,0,0,NULL,0);
	sqlrcur_inputBindClob($cur,"17","testtext3",strlen("testtext3"));
	sqlrcur_inputBindBlob($cur,"18","testbyte3",strlen("testbyte3"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1","t");
	sqlrcur_inputBind($cur,"2",4);
	sqlrcur_inputBind($cur,"3",4);
	sqlrcur_inputBind($cur,"4",4);
	sqlrcur_inputBind($cur,"5",4);
	sqlrcur_inputBind($cur,"6",4.5,4,2);
	sqlrcur_inputBind($cur,"7",4.5,4,2);
	sqlrcur_inputBind($cur,"8",4.5,4,2);
	sqlrcur_inputBind($cur,"9",4.5,4,2);
	sqlrcur_inputBind($cur,"10","testchar4");
	sqlrcur_inputBind($cur,"11","testnchar4");
	sqlrcur_inputBind($cur,"12","testvarchar4");
	sqlrcur_inputBind($cur,"13","testnvarchar4");
	sqlrcur_inputBind($cur,"14","testlvarchar4");
	sqlrcur_inputBindDate($cur,"15",2004,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate($cur,"16",2004,1,1,4,0,0,0,NULL,0);
	sqlrcur_inputBindClob($cur,"17","testtext4",strlen("testtext4"));
	sqlrcur_inputBindBlob($cur,"18","testbyte4",strlen("testbyte4"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1","t");
	sqlrcur_inputBind($cur,"2",5);
	sqlrcur_inputBind($cur,"3",5);
	sqlrcur_inputBind($cur,"4",5);
	sqlrcur_inputBind($cur,"5",5);
	sqlrcur_inputBind($cur,"6",5.5,4,2);
	sqlrcur_inputBind($cur,"7",5.5,4,2);
	sqlrcur_inputBind($cur,"8",5.5,4,2);
	sqlrcur_inputBind($cur,"9",5.5,4,2);
	sqlrcur_inputBind($cur,"10","testchar5");
	sqlrcur_inputBind($cur,"11","testnchar5");
	sqlrcur_inputBind($cur,"12","testvarchar5");
	sqlrcur_inputBind($cur,"13","testnvarchar5");
	sqlrcur_inputBind($cur,"14","testlvarchar5");
	sqlrcur_inputBindDate($cur,"15",2005,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate($cur,"16",2005,1,1,5,0,0,0,NULL,0);
	sqlrcur_inputBindClob($cur,"17","testtext5",strlen("testtext5"));
	sqlrcur_inputBindBlob($cur,"18","testbyte5",strlen("testbyte5"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1","t");
	sqlrcur_inputBind($cur,"2",6);
	sqlrcur_inputBind($cur,"3",6);
	sqlrcur_inputBind($cur,"4",6);
	sqlrcur_inputBind($cur,"5",6);
	sqlrcur_inputBind($cur,"6",6.5,4,2);
	sqlrcur_inputBind($cur,"7",6.5,4,2);
	sqlrcur_inputBind($cur,"8",6.5,4,2);
	sqlrcur_inputBind($cur,"9",6.5,4,2);
	sqlrcur_inputBind($cur,"10","testchar6");
	sqlrcur_inputBind($cur,"11","testnchar6");
	sqlrcur_inputBind($cur,"12","testvarchar6");
	sqlrcur_inputBind($cur,"13","testnvarchar6");
	sqlrcur_inputBind($cur,"14","testlvarchar6");
	sqlrcur_inputBindDate($cur,"15",2006,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate($cur,"16",2006,1,1,6,0,0,0,NULL,0);
	sqlrcur_inputBindClob($cur,"17","testtext6",strlen("testtext6"));
	sqlrcur_inputBindBlob($cur,"18","testbyte6",strlen("testbyte6"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by position
	echo("ARRAY OF INPUT BINDS BY POSITION: \n");
	sqlrcur_clearBinds($cur);
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
		"	null, ".
		"	null)");
	sqlrcur_inputBinds($cur,$bindvars,$bindvals);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# input bind by position with validation
	echo("INPUT BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1","t");
	sqlrcur_inputBind($cur,"2",8);
	sqlrcur_inputBind($cur,"3",8);
	sqlrcur_inputBind($cur,"4",8);
	sqlrcur_inputBind($cur,"5",8);
	sqlrcur_inputBind($cur,"6",8.5,4,2);
	sqlrcur_inputBind($cur,"7",8.5,4,2);
	sqlrcur_inputBind($cur,"8",8.5,4,2);
	sqlrcur_inputBind($cur,"9",8.5,4,2);
	sqlrcur_inputBind($cur,"10","testchar8");
	sqlrcur_inputBind($cur,"11","testnchar8");
	sqlrcur_inputBind($cur,"12","testvarchar8");
	sqlrcur_inputBind($cur,"13","testnvarchar8");
	sqlrcur_inputBind($cur,"14","testlvarchar8");
	sqlrcur_inputBindDate($cur,"15",2008,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate($cur,"16",2008,1,1,8,0,0,0,NULL,0);
	sqlrcur_inputBindClob($cur,"17","testtext8",strlen("testtext8"));
	sqlrcur_inputBindBlob($cur,"18","testbyte8",strlen("testbyte8"));
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# input bind by name
	# informix doesn't support bind by name


	# array of input binds by name
	# informix doesn't support bind by name


	# input bind by name with validation
	# informix doesn't support bind by name


	# select
	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount($cur),18);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testboolean");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testbigint");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testint8");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testmoney");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testsmallfloat");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,9),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,10),"testnchar");
	assertEqStr(sqlrcur_getColumnName($cur,11),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,12),"testnvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,13),"testlvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,14),"testdate");
	assertEqStr(sqlrcur_getColumnName($cur,15),"testdatetime");
	assertEqStr(sqlrcur_getColumnName($cur,16),"testtext");
	assertEqStr(sqlrcur_getColumnName($cur,17),"testbyte");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testboolean");
	assertEqStr($cols[1],"testsmallint");
	assertEqStr($cols[2],"testint");
	assertEqStr($cols[3],"testbigint");
	assertEqStr($cols[4],"testint8");
	assertEqStr($cols[5],"testdecimal");
	assertEqStr($cols[6],"testmoney");
	assertEqStr($cols[7],"testsmallfloat");
	assertEqStr($cols[8],"testfloat");
	assertEqStr($cols[9],"testchar");
	assertEqStr($cols[10],"testnchar");
	assertEqStr($cols[11],"testvarchar");
	assertEqStr($cols[12],"testnvarchar");
	assertEqStr($cols[13],"testlvarchar");
	assertEqStr($cols[14],"testdate");
	assertEqStr($cols[15],"testdatetime");
	assertEqStr($cols[16],"testtext");
	assertEqStr($cols[17],"testbyte");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnType($cur,0),"BOOLEAN");
	assertEqStr(sqlrcur_getColumnType($cur,"testboolean"),"BOOLEAN");
	assertEqStr(sqlrcur_getColumnType($cur,1),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testsmallint"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,2),"INTEGER");
	assertEqStr(sqlrcur_getColumnType($cur,"testint"),"INTEGER");
	assertEqStr(sqlrcur_getColumnType($cur,3),"BIGINT");
	assertEqStr(sqlrcur_getColumnType($cur,"testbigint"),"BIGINT");
	assertEqStr(sqlrcur_getColumnType($cur,4),"INT8");
	assertEqStr(sqlrcur_getColumnType($cur,"testint8"),"INT8");
	assertEqStr(sqlrcur_getColumnType($cur,5),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,"testdecimal"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,6),"MONEY");
	assertEqStr(sqlrcur_getColumnType($cur,"testmoney"),"MONEY");
	assertEqStr(sqlrcur_getColumnType($cur,7),"SMALLFLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,"testsmallfloat"),
							"SMALLFLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,8),"FLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,"testfloat"),"FLOAT");
	assertEqStr(sqlrcur_getColumnType($cur,9),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"testchar"),"CHAR");
	# informix reports nchar as char, with no way to tell them apart
	assertEqStr(sqlrcur_getColumnType($cur,10),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"testnchar"),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,11),"VARCHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"testvarchar"),"VARCHAR");
	# informix reports nvarchar as varchar, with no way to tell them apart
	assertEqStr(sqlrcur_getColumnType($cur,12),"VARCHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"testnvarchar"),"VARCHAR");
	assertEqStr(sqlrcur_getColumnType($cur,13),"LVARCHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"testlvarchar"),"LVARCHAR");
	assertEqStr(sqlrcur_getColumnType($cur,14),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,"testdate"),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,15),"DATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,"testdatetime"),
							"DATETIME");
	assertEqStr(sqlrcur_getColumnType($cur,16),"TEXT");
	assertEqStr(sqlrcur_getColumnType($cur,"testtext"),"TEXT");
	assertEqStr(sqlrcur_getColumnType($cur,17),"BYTE");
	assertEqStr(sqlrcur_getColumnType($cur,"testbyte"),"BYTE");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLength($cur,0),1);
	assertEqInt(sqlrcur_getColumnLength($cur,"testboolean"),1);
	assertEqInt(sqlrcur_getColumnLength($cur,1),5);
	assertEqInt(sqlrcur_getColumnLength($cur,"testsmallint"),5);
	assertEqInt(sqlrcur_getColumnLength($cur,2),10);
	assertEqInt(sqlrcur_getColumnLength($cur,"testint"),10);
	assertEqInt(sqlrcur_getColumnLength($cur,3),20);
	assertEqInt(sqlrcur_getColumnLength($cur,"testbigint"),20);
	assertEqInt(sqlrcur_getColumnLength($cur,4),20);
	assertEqInt(sqlrcur_getColumnLength($cur,"testint8"),20);
	assertEqInt(sqlrcur_getColumnLength($cur,5),10);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdecimal"),10);
	assertEqInt(sqlrcur_getColumnLength($cur,6),16);
	assertEqInt(sqlrcur_getColumnLength($cur,"testmoney"),16);
	assertEqInt(sqlrcur_getColumnLength($cur,7),7);
	assertEqInt(sqlrcur_getColumnLength($cur,"testsmallfloat"),7);
	assertEqInt(sqlrcur_getColumnLength($cur,8),15);
	assertEqInt(sqlrcur_getColumnLength($cur,"testfloat"),15);
	assertEqInt(sqlrcur_getColumnLength($cur,9),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,10),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testnchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,11),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testvarchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,12),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testnvarchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,13),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testlvarchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,14),10);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdate"),10);
	assertEqInt(sqlrcur_getColumnLength($cur,15),19);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getColumnLength($cur,16),2147483647);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtext"),
							2147483647);
	assertEqInt(sqlrcur_getColumnLength($cur,17),2147483647);
	assertEqInt(sqlrcur_getColumnLength($cur,"testbyte"),
							2147483647);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongest($cur,0),1);
	assertEqInt(sqlrcur_getLongest($cur,"testboolean"),1);
	assertEqInt(sqlrcur_getLongest($cur,1),1);
	assertEqInt(sqlrcur_getLongest($cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongest($cur,2),1);
	assertEqInt(sqlrcur_getLongest($cur,"testint"),1);
	assertEqInt(sqlrcur_getLongest($cur,3),1);
	assertEqInt(sqlrcur_getLongest($cur,"testbigint"),1);
	assertEqInt(sqlrcur_getLongest($cur,4),1);
	assertEqInt(sqlrcur_getLongest($cur,"testint8"),1);
	assertEqInt(sqlrcur_getLongest($cur,5),4);
	assertEqInt(sqlrcur_getLongest($cur,"testdecimal"),4);
	assertEqInt(sqlrcur_getLongest($cur,6),4);
	assertEqInt(sqlrcur_getLongest($cur,"testmoney"),4);
	assertEqInt(sqlrcur_getLongest($cur,7),3);
	assertEqInt(sqlrcur_getLongest($cur,"testsmallfloat"),3);
	assertEqInt(sqlrcur_getLongest($cur,8),3);
	assertEqInt(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongest($cur,9),40);
	assertEqInt(sqlrcur_getLongest($cur,"testchar"),40);
	assertEqInt(sqlrcur_getLongest($cur,10),40);
	assertEqInt(sqlrcur_getLongest($cur,"testnchar"),40);
	assertEqInt(sqlrcur_getLongest($cur,11),12);
	assertEqInt(sqlrcur_getLongest($cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongest($cur,12),13);
	assertEqInt(sqlrcur_getLongest($cur,"testnvarchar"),13);
	assertEqInt(sqlrcur_getLongest($cur,13),13);
	assertEqInt(sqlrcur_getLongest($cur,"testlvarchar"),13);
	assertEqInt(sqlrcur_getLongest($cur,14),10);
	assertEqInt(sqlrcur_getLongest($cur,"testdate"),10);
	assertEqInt(sqlrcur_getLongest($cur,15),19);
	assertEqInt(sqlrcur_getLongest($cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getLongest($cur,16),9);
	assertEqInt(sqlrcur_getLongest($cur,"testtext"),9);
	assertEqInt(sqlrcur_getLongest($cur,17),9);
	assertEqInt(sqlrcur_getLongest($cur,"testbyte"),9);
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
	assertEqStr(sqlrcur_getField($cur,0,3),"1");
	assertEqStr(sqlrcur_getField($cur,0,4),"1");
	assertEqStr(sqlrcur_getField($cur,0,5),"1.50");
	assertEqStr(sqlrcur_getField($cur,0,6),"1.50");
	assertEqStr(sqlrcur_getField($cur,0,7),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,8),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,9),
			"testchar1                               ");
	assertEqStr(sqlrcur_getField($cur,0,10),
			"testnchar1                              ");
	assertEqStr(sqlrcur_getField($cur,0,11),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,12),"testnvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,13),"testlvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,14),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,15),
			"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,16),"testtext1");
	assertEqStr(sqlrcur_getField($cur,0,17),"");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,0),"1");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,7,2),"8");
	assertEqStr(sqlrcur_getField($cur,7,3),"8");
	assertEqStr(sqlrcur_getField($cur,7,4),"8");
	assertEqStr(sqlrcur_getField($cur,7,5),"8.50");
	assertEqStr(sqlrcur_getField($cur,7,6),"8.50");
	assertEqStr(sqlrcur_getField($cur,7,7),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,8),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,9),
			"testchar8                               ");
	assertEqStr(sqlrcur_getField($cur,7,10),
			"testnchar8                              ");
	assertEqStr(sqlrcur_getField($cur,7,11),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,12),"testnvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,13),"testlvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,14),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,15),
			"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,16),"");
	assertEqStr(sqlrcur_getField($cur,7,17),"");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,3),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,4),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,5),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,6),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,7),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,8),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,9),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,10),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,11),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,12),13);
	assertEqInt(sqlrcur_getFieldLength($cur,0,14),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,15),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,16),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,17),0);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,3),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,4),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,5),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,6),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,7),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,8),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,9),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,10),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,11),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,12),13);
	assertEqInt(sqlrcur_getFieldLength($cur,7,14),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,15),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,16),0);
	assertEqInt(sqlrcur_getFieldLength($cur,7,17),0);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getField($cur,0,"testboolean"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testbigint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testint8"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testdecimal"),"1.50");
	assertEqStr(sqlrcur_getField($cur,0,"testmoney"),"1.50");
	assertEqStr(sqlrcur_getField($cur,0,"testsmallfloat"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testchar"),
			"testchar1                               ");
	assertEqStr(sqlrcur_getField($cur,0,"testnchar"),
			"testnchar1                              ");
	assertEqStr(sqlrcur_getField($cur,0,"testvarchar"),
			"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testnvarchar"),
			"testnvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testlvarchar"),
			"testlvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testdate"),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,"testdatetime"),
			"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,"testtext"),"testtext1");
	assertEqStr(sqlrcur_getField($cur,0,"testbyte"),"");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,"testboolean"),"1");
	assertEqStr(sqlrcur_getField($cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testbigint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testint8"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testdecimal"),"8.50");
	assertEqStr(sqlrcur_getField($cur,7,"testmoney"),"8.50");
	assertEqStr(sqlrcur_getField($cur,7,"testsmallfloat"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testchar"),
			"testchar8                               ");
	assertEqStr(sqlrcur_getField($cur,7,"testnchar"),
			"testnchar8                              ");
	assertEqStr(sqlrcur_getField($cur,7,"testvarchar"),
			"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testnvarchar"),
			"testnvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testlvarchar"),
			"testlvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testdate"),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,"testdatetime"),
			"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,"testtext"),"");
	assertEqStr(sqlrcur_getField($cur,7,"testbyte"),"");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testboolean"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testbigint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testint8"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdecimal"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testsmallfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testnchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testnvarchar"),13);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testlvarchar"),13);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtext"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testbyte"),0);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testboolean"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testbigint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testint8"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdecimal"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testsmallfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testnchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testnvarchar"),13);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testlvarchar"),13);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtext"),0);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testbyte"),0);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqStr($fields[0],"1");
	assertEqStr($fields[1],"1");
	assertEqStr($fields[2],"1");
	assertEqStr($fields[3],"1");
	assertEqStr($fields[4],"1");
	assertEqStr($fields[5],"1.50");
	assertEqStr($fields[6],"1.50");
	assertEqStr($fields[7],"1.5");
	assertEqStr($fields[8],"1.5");
	assertEqStr($fields[9],"testchar1                               ");
	assertEqStr($fields[10],"testnchar1                              ");
	assertEqStr($fields[11],"testvarchar1");
	assertEqStr($fields[12],"testnvarchar1");
	assertEqStr($fields[13],"testlvarchar1");
	assertEqStr($fields[14],"2001-01-01");
	assertEqStr($fields[15],"2001-01-01 01:00:00");
	assertEqStr($fields[16],"testtext1");
	assertEqStr($fields[17],"");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqInt($fieldlens[0],1);
	assertEqInt($fieldlens[1],1);
	assertEqInt($fieldlens[2],1);
	assertEqInt($fieldlens[3],1);
	assertEqInt($fieldlens[4],1);
	assertEqInt($fieldlens[5],4);
	assertEqInt($fieldlens[6],4);
	assertEqInt($fieldlens[7],3);
	assertEqInt($fieldlens[8],3);
	assertEqInt($fieldlens[9],40);
	assertEqInt($fieldlens[10],40);
	assertEqInt($fieldlens[11],12);
	assertEqInt($fieldlens[12],13);
	assertEqInt($fieldlens[14],10);
	assertEqInt($fieldlens[15],19);
	assertEqInt($fieldlens[16],9);
	assertEqInt($fieldlens[17],0);
	echo("\n");


	# result set buffer size
	echo("RESULT SET BUFFER SIZE: \n");
	assertEqInt(sqlrcur_getResultSetBufferSize($cur),0);
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	assertEqInt(sqlrcur_getResultSetBufferSize($cur),2);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),0);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),2);
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,1,1),"2");
	assertEqStr(sqlrcur_getField($cur,2,1),"3");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),2);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),4);
	assertEqStr(sqlrcur_getField($cur,6,1),"7");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertEqStr(sqlrcur_getField($cur,8,1),NULL);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
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
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName($cur,1),NULL);
	assertEqInt(sqlrcur_getColumnLength($cur,1),0);
	assertEqStr(sqlrcur_getColumnType($cur,1),NULL);
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqInt(sqlrcur_getColumnLength($cur,1),5);
	assertEqStr(sqlrcur_getColumnType($cur,1),"SMALLINT");
	echo("\n");


	# suspended session
	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,1,1),"2");
	assertEqStr(sqlrcur_getField($cur,2,1),"3");
	assertEqStr(sqlrcur_getField($cur,3,1),"4");
	assertEqStr(sqlrcur_getField($cur,4,1),"5");
	assertEqStr(sqlrcur_getField($cur,5,1),"6");
	assertEqStr(sqlrcur_getField($cur,6,1),"7");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,1,1),"2");
	assertEqStr(sqlrcur_getField($cur,2,1),"3");
	assertEqStr(sqlrcur_getField($cur,3,1),"4");
	assertEqStr(sqlrcur_getField($cur,4,1),"5");
	assertEqStr(sqlrcur_getField($cur,5,1),"6");
	assertEqStr(sqlrcur_getField($cur,6,1),"7");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,1,1),"2");
	assertEqStr(sqlrcur_getField($cur,2,1),"3");
	assertEqStr(sqlrcur_getField($cur,3,1),"4");
	assertEqStr(sqlrcur_getField($cur,4,1),"5");
	assertEqStr(sqlrcur_getField($cur,5,1),"6");
	assertEqStr(sqlrcur_getField($cur,6,1),"7");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
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
		"	testsmallint "));
	assertEqStr(sqlrcur_getField($cur,2,1),"3");
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
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertEqStr(sqlrcur_getField($cur,8,1),NULL);
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
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount($cur),18);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testboolean");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testbigint");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testint8");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testmoney");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testsmallfloat");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,9),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,10),"testnchar");
	assertEqStr(sqlrcur_getColumnName($cur,11),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,12),"testnvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,13),"testlvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,14),"testdate");
	assertEqStr(sqlrcur_getColumnName($cur,15),"testdatetime");
	assertEqStr(sqlrcur_getColumnName($cur,16),"testtext");
	assertEqStr(sqlrcur_getColumnName($cur,17),"testbyte");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testboolean");
	assertEqStr($cols[1],"testsmallint");
	assertEqStr($cols[2],"testint");
	assertEqStr($cols[3],"testbigint");
	assertEqStr($cols[4],"testint8");
	assertEqStr($cols[5],"testdecimal");
	assertEqStr($cols[6],"testmoney");
	assertEqStr($cols[7],"testsmallfloat");
	assertEqStr($cols[8],"testfloat");
	assertEqStr($cols[9],"testchar");
	assertEqStr($cols[10],"testnchar");
	assertEqStr($cols[11],"testvarchar");
	assertEqStr($cols[12],"testnvarchar");
	assertEqStr($cols[13],"testlvarchar");
	assertEqStr($cols[14],"testdate");
	assertEqStr($cols[15],"testdatetime");
	assertEqStr($cols[16],"testtext");
	assertEqStr($cols[17],"testbyte");
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
		"	testsmallint "));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,8,1),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# from one cache file to another
	echo("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,8,1),NULL);
	echo("\n");


	# from one cache file to another with result set buffer size
	echo("FROM ONE CACHE FILE TO ANOTHER ".
				"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,8,1),NULL);
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
		"	testsmallint "));
	assertEqStr(sqlrcur_getField($cur,2,1),"3");
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
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),6);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertEqStr(sqlrcur_getField($cur,8,1),NULL);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),8);
	assertTrue(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),8);
	sqlrcur_cacheOff($cur);
	echo("\n");
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,8,1),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# finished suspended session
	echo("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select * from testtable order by testint"));
	assertEqStr(sqlrcur_getField($cur,4,1),"5");
	assertEqStr(sqlrcur_getField($cur,5,1),"6");
	assertEqStr(sqlrcur_getField($cur,6,1),"7");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	$id=sqlrcur_getResultSetId($cur);
	sqlrcur_suspendResultSet($cur);
	assertTrue(sqlrcon_suspendSession($con));
	$port=sqlrcon_getConnectionPort($con);
	$socket=sqlrcon_getConnectionSocket($con);
	assertTrue(sqlrcon_resumeSession($con,$port,$socket));
	assertTrue(sqlrcur_resumeResultSet($cur,$id));
	assertEqStr(sqlrcur_getField($cur,4,1),NULL);
	assertEqStr(sqlrcur_getField($cur,5,1),NULL);
	assertEqStr(sqlrcur_getField($cur,6,1),NULL);
	assertEqStr(sqlrcur_getField($cur,7,1),NULL);
	echo("\n");


	# nested selects
	echo("NESTED SELECTS: \n");
	sqlrcur_setResultSetBufferSize($cur,1);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	$secondcur=sqlrcur_alloc($con);
	sqlrcur_setResultSetBufferSize($secondcur,1);
	for ($i=0; sqlrcur_getRow($cur,$i); $i++) {
		assertTrue(sqlrcur_sendQuery(
				$secondcur,"select * from testtable"));
	}
	sqlrcur_closeResultSet($secondcur);
	sqlrcur_setResultSetBufferSize($cur,0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# reset transaction state
	echo("RESET TRANSACTION STATE: \n");
	assertTrue(sqlrcon_commit($con));
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	assertFalse(sqlrcon_getAutoCommit($con));
	echo("\n");


	# transaction behavior - implicit
	# Informix has no MVCC option -- the isolation level is either dirty
	# reads (where the second connection sees uncommitted rows) or
	# committed read (where it blocks or errors on locked rows) -- so
	# the visibility assertions below may need to be revisited
	echo("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
	# informix DDL is transactional in logged mode; commit so the table
	# is visible to the second connection (commit implicitly starts a
	# new tx)
	assertTrue(sqlrcon_commit($con));
	$secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	# Informix has no MVCC; under default committed-read isolation,
	# secondcur's catalog/data read errors with "Cannot get system
	# information for table" while cur holds row locks from the
	# in-flight tx.  Use dirty-read on secondcur so it sees the
	# uncommitted writes — the test then verifies dirty-read
	# semantics instead of MVCC visibility.
	assertTrue(sqlrcur_sendQuery($secondcur,"set isolation to dirty read"));
	# session is in a transaction; insert is visible via dirty read
	assertTrue(sqlrcon_getInTransaction($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
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
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - explicit
	echo("TRANSACTION BEHAVIOR - explicit: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"explicit"));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
	# see note above re: informix dirty-read workaround
	assertTrue(sqlrcur_sendQuery($secondcur,"set isolation to dirty read"));
	# begin starts a new transaction; insert is visible via dirty read
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"1");
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
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
	# see note in - implicit section re: informix dirty-read workaround
	assertTrue(sqlrcur_sendQuery($secondcur,"set isolation to dirty read"));
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
	# explicitly commits/rollbacks the tx (mysql-native semantic).
	# dirty-read on secondcur sees the in-flight insert (count=2)
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (3)"));
	assertTrue(sqlrcon_autoCommitOn($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"2");
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
	# dirty-read on secondcur sees the in-flight insert (count=5)
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (7)"));
	assertTrue(sqlrcon_autoCommitOff($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"5");
	assertTrue(sqlrcon_commit($con));
	assertFalse(sqlrcon_getAutoCommit($con));
	assertTrue(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"5");
	sqlrcur_closeResultSet($secondcur);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - explicit-error
	echo("TRANSACTION BEHAVIOR - explicit-error: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"explicit-error"));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit-error");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
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
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - none
	echo("TRANSACTION BEHAVIOR - none: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"none"));
	assertEqStr(sqlrcon_getTransactionModel($con),"none");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
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
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	assertFalse(sqlrcon_getAutoCommit($con));
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,
		"select ".
		"	$(var1), ".
		"	'$(var2)', ".
		"	'$(var3)' ".
		"from ".
		"	sysmaster:sysdual ");
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
	sqlrcur_prepareQuery($cur,
		"select ".
		"	'$(var1)', ".
		"	'$(var2)', ".
		"	'$(var3)' ".
		"from ".
		"	sysmaster:sysdual ");
	sqlrcur_substitutions($cur,$subvars,$subvalstrings);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"hi");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"bye");
	echo("\n");
	sqlrcur_prepareQuery($cur,
		"select ".
		"	$(var1), ".
		"	$(var2), ".
		"	$(var3) ".
		"from ".
		"	sysmaster:sysdual ");
	sqlrcur_substitutions($cur,$subvars,$subvallongs);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"2");
	assertEqStr(sqlrcur_getField($cur,0,2),"3");
	echo("\n");
	sqlrcur_prepareQuery($cur,
		"select ".
		"	$(var1), ".
		"	$(var2), ".
		"	$(var3) ".
		"from ".
		"	sysmaster:sysdual ");
	sqlrcur_substitutions($cur,$subvars,$subvaldoubles,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"10.55");
	assertEqStr(sqlrcur_getField($cur,0,1),"10.556");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	# nulls as nulls
	echo("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select NULL::int,1,NULL::int from sysmaster:sysdual"));
	assertEqStr(sqlrcur_getField($cur,0,0),NULL);
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select NULL::int,1,NULL::int from sysmaster:sysdual"));
	assertEqStr(sqlrcur_getField($cur,0,0),"");
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),"");
	echo("\n");


	# output bind by position
	echo("OUTPUT BIND BY POSITION: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	out out1 int, ".
		"	out out2 varchar(20), ".
		"	out out3 float, ".
		"	out out4 varchar(20)) ".
		"let out1 = 1; ".
		"	let out2 = 'hello'; ".
		"	let out3 = 2.5; ".
		"	let out4 = null; ".
		"end procedure;"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"{call testproc(?,?,?,?)}");
	assertEqInt(sqlrcur_countBindVariables($cur),4);
	sqlrcur_defineOutputBindInteger($cur,"1");
	sqlrcur_defineOutputBindString($cur,"2",20);
	sqlrcur_defineOutputBindDouble($cur,"3");
	sqlrcur_defineOutputBindString($cur,"4",20);
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"1");
	$stringvar=sqlrcur_getOutputBindString($cur,"2");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"3");
	$nullvar=sqlrcur_getOutputBindString($cur,"4");
	assertEqInt($numvar,1);
	assertEqStr($stringvar,"hello");
	assertEqDbl($floatvar,2.5);
	assertEqStr($nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# output bind by name
	# informix doesn't support bind by name


	# output bind by name with validation
	# informix doesn't support bind by name


	# lob output bind
	echo("LOB OUTPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"insert into testtable values (?,?)");
	sqlrcur_inputBindClob($cur,"1","hello",strlen("hello"));
	sqlrcur_inputBindBlob($cur,"2","hello",strlen("hello"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	out out1 clob, ".
		"	out out2 blob) ".
		"select testclob, testblob ".
		"	into out1, out2 ".
		"	from testtable; ".
		"end procedure;"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"{call testproc(?,?)}");
	sqlrcur_defineOutputBindClob($cur,"1");
	sqlrcur_defineOutputBindBlob($cur,"2");
	assertTrue(sqlrcur_executeQuery($cur));
	$clobvar=sqlrcur_getOutputBindClob($cur,"1");
	$clobvarlength=sqlrcur_getOutputBindLength($cur,"1");
	$blobvar=sqlrcur_getOutputBindBlob($cur,"2");
	$blobvarlength=sqlrcur_getOutputBindLength($cur,"2");
	assertEqStrLen($clobvar,"hello",5);
	assertEqInt($clobvarlength,5);
	assertEqStrLen($blobvar,"hello",5);
	assertEqInt($blobvarlength,5);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# long output bind
	echo("LONG OUTPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in1 clob, ".
		"	out out1 clob) ".
		"let out1 = in1; ".
		"	end procedure;"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"{call testproc(?,?)}");
	$largebuffer=str_repeat("C",20*1024);
	sqlrcur_inputBindClob($cur,"1",$largebuffer,strlen($largebuffer));
	sqlrcur_defineOutputBindClob($cur,"2");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindLength($cur,"2"),
			20*1024);
	assertEqStr(sqlrcur_getOutputBindClob($cur,"2"),$largebuffer);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# negative input bind
	echo("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,"create table testtable (testval int)");
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"insert into testtable values (?)");
	sqlrcur_inputBind($cur,"1",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable");
	assertEqStr(sqlrcur_getField($cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# bind validation
	# informix doesn't support bind by name

	# rebinding
	echo("REBINDING: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in1 int, ".
		"	out out1 int) ".
		"let out1 = in1; ".
		"end procedure;"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"{call testproc(?,?)}");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_defineOutputBindInteger($cur,"2");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"2"),1);
	sqlrcur_inputBind($cur,"1",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"2"),2);
	sqlrcur_inputBind($cur,"1",3);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"2"),3);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# reexecute
	echo("REEXECUTE: \n");
	sqlrcur_prepareQuery($cur,
		"select 1 from sysmaster:sysdual");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	sqlrcur_prepareQuery($cur,
		"select ?::int from sysmaster:sysdual");
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
	echo("STORED PROCEDURE RETURNING NO VALUE: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in1 int, ".
		"	in2 float, ".
		"	in3 varchar(20)) ".
		"end procedure;"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"{call testproc(?,?,?)}");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",2.5,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# stored procedure returning single value
	echo("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in1 int, ".
		"	in2 float, ".
		"	in3 varchar(20), ".
		"	out out1 int) ".
		"let out1 = in1; ".
		"end procedure;"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"{call testproc(?,?,?,?)}");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",2.5,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	sqlrcur_defineOutputBindInteger($cur,"4");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"4"),1);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# stored procedure returning multiple values
	echo("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in1 int, ".
		"	in2 float, ".
		"	in3 varchar(20), ".
		"	out out1 int, ".
		"	out out2 float, ".
		"	out out3 varchar(20)) ".
		"let out1 = in1; ".
		"	let out2 = in2; ".
		"	let out3 = in3; ".
		"end procedure;"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"{call testproc(?,?,?,?,?,?)}");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",2.5,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	sqlrcur_defineOutputBindInteger($cur,"4");
	sqlrcur_defineOutputBindDouble($cur,"5");
	sqlrcur_defineOutputBindString($cur,"6",20);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"4"),1);
	assertEqDbl(sqlrcur_getOutputBindDouble($cur,"5"),2.5);
	assertEqStr(sqlrcur_getOutputBindString($cur,"6"),"hello");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# stored procedure returning result set
	echo("STORED PROCEDURE RETURNING RESULT SET: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc() ".
		"returning boolean, smallint, varchar(40); ".
		"	define out1 boolean; ".
		"	define out2 smallint; ".
		"	define out3 varchar(40); ".
		"	foreach ".
		"		select ".
		"			testboolean, ".
		"			testsmallint, ".
		"			testvarchar ".
		"		into out1,out2,out3 ".
		"		from ( ".
		"			select ".
		"				't' as testboolean, ".
		"				1 as testsmallint, ".
		"				'1' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"			union ".
		"			select ".
		"				't' as testboolean, ".
		"				2 as testsmallint, ".
		"				'2' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"			union ".
		"			select ".
		"				't' as testboolean, ".
		"				3 as testsmallint, ".
		"				'3' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"			union ".
		"			select ".
		"				't' as testboolean, ".
		"				4 as testsmallint, ".
		"				'4' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"			union ".
		"			select ".
		"				't' as testboolean, ".
		"				5 as testsmallint, ".
		"				'5' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"			union ".
		"			select ".
		"				't' as testboolean, ".
		"				6 as testsmallint, ".
		"				'6' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"			union ".
		"			select ".
		"				't' as testboolean, ".
		"				7 as testsmallint, ".
		"				'7' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"			union ".
		"			select ".
		"				't' as testboolean, ".
		"				8 as testsmallint, ".
		"				'8' as testvarchar ".
		"			from ".
		"				sysmaster:sysdual ".
		"		) ".
		"	return out1,out2,out3 ".
		"	with resume; ".
		"	end foreach; ".
		"	end procedure;"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($cur,"{call testproc()}"));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# null and empty lobs
	echo("NULL AND EMPTY LOBS: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob1 clob, ".
		"	testclob2 clob, ".
		"	testblob1 blob, ".
		"	testblob2 blob)"));
	assertTrue(sqlrcon_commit($con));
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
	# informix returns a single \0 for an empty lob; the C/C++ tests
	# pass via strcmp (which stops at \0) so truncate at first \0 here.
	$f0=sqlrcur_getField($cur,0,0);
	if ($f0===false || $f0===NULL) {
		$f0="";
	} else {
		$nul=strpos($f0,"\0");
		if ($nul!==false) {
			$f0=substr($f0,0,$nul);
		}
	}
	assertEqStr($f0,"");
	assertEqStr(sqlrcur_getField($cur,0,1),NULL);
	$f2=sqlrcur_getField($cur,0,2);
	if ($f2===false || $f2===NULL) {
		$f2="";
	} else {
		$nul=strpos($f2,"\0");
		if ($nul!==false) {
			$f2=substr($f2,0,$nul);
		}
	}
	assertEqStr($f2,"");
	assertEqStr(sqlrcur_getField($cur,0,3),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# long lobs
	echo("LONG LOBS: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testtext text, ".
		"	testbyte byte)");
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,
		"insert into testtable values (?,?)");
	$largebuffer=str_repeat("C",20*1024);
	sqlrcur_inputBindClob($cur,"1",$largebuffer,strlen($largebuffer));
	sqlrcur_inputBindBlob($cur,"2",$largebuffer,strlen($largebuffer));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtext"),
			20*1024);
	assertEqStr(sqlrcur_getField($cur,0,"testtext"),$largebuffer);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testbyte"),
			20*1024);
	assertEqStrLen(sqlrcur_getField($cur,0,"testbyte"),
			$largebuffer,20*1024);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# temporary tables
	echo("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery($cur,"drop table temptable");
	sqlrcur_sendQuery($cur,
		"create temp table temptable (col1 int)");
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into temptable values (1)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"select count(*) from temptable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcon_endSession($con);
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,
		"select count(*) from temptable"));
	echo("\n");


	# encoded binary data
	# informix doesn't support encoded binary data


	# quotes
	echo("QUOTES: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into testtable values ('''''')"));
	assertTrue(sqlrcur_sendQuery($cur,
		"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqStr(sqlrcur_getField($cur,0,0),"''");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# last insert id
	echo("LAST INSERT ID: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
			"create table testtable ".
			"	(col1 serial primary key, ".
			"	col2 int)"));
	assertTrue(sqlrcur_sendQuery($cur,
			"insert into testtable (col2) values (1)"));
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
	# informix requires that a table exist that is
	# owned by a user for the user to be reported
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 integer, ".
		"	col2 integer)"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_getSchemaList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	assertInResultSet($cur,"Database","testuser");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
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
		"	col1 integer, ".
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable2 (".
		"	col1 integer, ".
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable3 (".
		"	col1 integer, ".
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable4 (".
		"	col1 integer, ".
		"	col2 integer)"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_getTableList($cur,NULL));
	assertInResultSet($cur,"Tables_in_xxx","testtable1");
	assertInResultSet($cur,"Tables_in_xxx","testtable2");
	assertInResultSet($cur,"Tables_in_xxx","testtable3");
	assertInResultSet($cur,"Tables_in_xxx","testtable4");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable4"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# type info list
	echo("TYPE INFO LIST: \n");
	assertTrue(sqlrcur_getTypeInfoList($cur,"integer"));
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
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"INTEGER");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"4");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),
							"INTEGER");
	assertTrue(sqlrcur_getTypeInfoList($cur,"char"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"32767");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"varchar"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"255");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),
							"VARCHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"date"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"91");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"DATE");
	echo("\n");


	# column list
	echo("COLUMN LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testboolean boolean, ".
		"	testsmallint smallint, ".
		"	testint integer, ".
		"	testbigint bigint, ".
		"	testint8 int8, ".
		"	testdecimal decimal(10,2), ".
		"	testmoney money, ".
		"	testsmallfloat smallfloat, ".
		"	testfloat float, ".
		"	testchar char(40), ".
		"	testnchar nchar(40), ".
		"	testvarchar varchar(40), ".
		"	testnvarchar nvarchar(40), ".
		"	testlvarchar lvarchar(40), ".
		"	testdate date, ".
		"	testdatetime datetime year to second, ".
		"	testtext text, ".
		"	testbyte byte)"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"column_name");
	assertEqStr(sqlrcur_getColumnName($cur,1),"data_type");
	assertEqStr(sqlrcur_getColumnName($cur,2),
			"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName($cur,3),"numeric_precision");
	assertEqStr(sqlrcur_getColumnName($cur,4),"numeric_scale");
	assertEqStr(sqlrcur_getColumnName($cur,5),"is_nullable");
	assertEqStr(sqlrcur_getColumnName($cur,6),"column_key");
	assertEqStr(sqlrcur_getColumnName($cur,7),"column_default");
	assertEqStr(sqlrcur_getColumnName($cur,8),"extra");
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),
							"testboolean");
	assertEqStr(sqlrcur_getField($cur,1,"column_name"),
							"testsmallint");
	assertEqStr(sqlrcur_getField($cur,2,"column_name"),
							"testint");
	assertEqStr(sqlrcur_getField($cur,3,"column_name"),
							"testbigint");
	assertEqStr(sqlrcur_getField($cur,4,"column_name"),
							"testint8");
	assertEqStr(sqlrcur_getField($cur,5,"column_name"),
							"testdecimal");
	assertEqStr(sqlrcur_getField($cur,6,"column_name"),
							"testmoney");
	assertEqStr(sqlrcur_getField($cur,7,"column_name"),
							"testsmallfloat");
	assertEqStr(sqlrcur_getField($cur,8,"column_name"),
							"testfloat");
	assertEqStr(sqlrcur_getField($cur,9,"column_name"),
							"testchar");
	assertEqStr(sqlrcur_getField($cur,10,"column_name"),
							"testnchar");
	assertEqStr(sqlrcur_getField($cur,11,"column_name"),
							"testvarchar");
	assertEqStr(sqlrcur_getField($cur,12,"column_name"),
							"testnvarchar");
	assertEqStr(sqlrcur_getField($cur,13,"column_name"),
							"testlvarchar");
	assertEqStr(sqlrcur_getField($cur,14,"column_name"),
							"testdate");
	assertEqStr(sqlrcur_getField($cur,15,"column_name"),
							"testdatetime");
	assertEqStr(sqlrcur_getField($cur,16,"column_name"),
							"testtext");
	assertEqStr(sqlrcur_getField($cur,17,"column_name"),
							"testbyte");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"BOOLEAN");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"SMALLINT");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"BIGINT");
	assertEqStr(sqlrcur_getField($cur,4,"data_type"),"INT8");
	assertEqStr(sqlrcur_getField($cur,5,"data_type"),"DECIMAL");
	assertEqStr(sqlrcur_getField($cur,6,"data_type"),"MONEY");
	assertEqStr(sqlrcur_getField($cur,7,"data_type"),
							"SMALLFLOAT");
	assertEqStr(sqlrcur_getField($cur,8,"data_type"),"FLOAT");
	assertEqStr(sqlrcur_getField($cur,9,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,10,"data_type"),"NCHAR");
	assertEqStr(sqlrcur_getField($cur,11,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,12,"data_type"),"NVARCHAR");
	assertEqStr(sqlrcur_getField($cur,13,"data_type"),"LVARCHAR");
	assertEqStr(sqlrcur_getField($cur,14,"data_type"),"DATE");
	assertEqStr(sqlrcur_getField($cur,15,"data_type"),"DATETIME");
	assertEqStr(sqlrcur_getField($cur,16,"data_type"),"TEXT");
	assertEqStr(sqlrcur_getField($cur,17,"data_type"),"BYTE");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# column list - auto_increment, primary key
	echo("COLUMN LIST - auto_increment, primary key: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 serial primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcon_commit($con));
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
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertFalse(strstr(sqlrcur_getField($cur,0,"extra"),
				"auto_increment")!==false);
	assertTrue(strstr(sqlrcur_getField($cur,0,"column_key"),
				"PRI")!==false);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# primary keys list
	echo("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 integer primary key, ".
		"	col2 integer)"));
	assertTrue(sqlrcon_commit($con));
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
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"table"),
							"testtable"));
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"column_name"),
							"col1"));
	$keyname=sqlrcur_getField($cur,0,"key_name");
	assertTrue($keyname && strlen($keyname)>0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# key and index list
	echo("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 integer primary key, ".
		"	col2 integer)"));
	assertTrue(sqlrcon_commit($con));
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
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"table"),
							"testtable"));
	assertEqStr(sqlrcur_getField($cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"column_name"),
							"col1"));
	assertEqStr(sqlrcur_getField($cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getField($cur,0,"index_type"),"3");
	$keyname=sqlrcur_getField($cur,0,"key_name");
	assertTrue($keyname && strlen($keyname)>0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# procedure list
	echo("PROCEDURE LIST: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc1");
	sqlrcur_sendQuery($cur,"drop procedure testproc2");
	sqlrcur_sendQuery($cur,"drop procedure testproc3");
	sqlrcur_sendQuery($cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc1(".
		"	in1 integer, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) ".
		"define x integer; ".
		"let x = 1; ".
		"end procedure;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc2(".
		"	in1 integer, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) ".
		"define x integer; ".
		"let x = 1; ".
		"end procedure;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc3(".
		"	in1 integer, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) ".
		"define x integer; ".
		"let x = 1; ".
		"end procedure;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc4(".
		"	in1 integer, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) ".
		"define x integer; ".
		"let x = 1; ".
		"end procedure;"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_getProcedureList($cur,NULL));
	assertInResultSet($cur,"routine_name","testproc1");
	assertInResultSet($cur,"routine_name","testproc2");
	assertInResultSet($cur,"routine_name","testproc3");
	assertInResultSet($cur,"routine_name","testproc4");
	echo("\n");


	# procedure parameter list
	echo("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList(
					$cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName($cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName($cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName($cur,3),
			"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName($cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount($cur),4);
	assertEqStr(sqlrcur_getField($cur,0,"parameter_name"),
							"in1");
	assertEqStr(sqlrcur_getField($cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"integer");
	assertEqStr(sqlrcur_getField($cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_name"),
							"in2");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"char");
	assertEqStr(sqlrcur_getField($cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_name"),
							"in3");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"varchar");
	assertEqStr(sqlrcur_getField($cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_name"),
							"in4");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"date");
	assertEqStr(sqlrcur_getField($cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc4"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# invalid queries
	echo("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,
		"insert into testtable values (1,2,3,4)"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	echo("\n");


	reportTestStatus();

	exit($status);
?></pre></html>
