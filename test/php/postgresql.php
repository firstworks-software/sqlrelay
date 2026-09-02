<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.
	include("./asserts.php");


	# hostname
	$hostname=gethostname();
	$dot=strpos($hostname,'.');
	if ($dot) {
		$hostname=substr($hostname,0,$dot);
	}


	# instantiation
	$con=sqlrcon_alloc("sqlrelay",9003,"/tmp/postgresql.socket","testuser",
				"testpassword",0,1);
	$cur=sqlrcur_alloc($con);


	# identify
	echo("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify($con),"postgresql");
	echo("\n");


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	# transaction state
	echo("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel($con),"explicit");
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit");
	assertFalse(sqlrcon_getInTransaction($con));
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	# bind format
	echo("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat($con),"\$1");
	echo("\n");


	# nextval format
	echo("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat($con),"nextval('%s')");
	echo("\n");


	# isolation levels
	echo("ISOLATION LEVELS: \n");
	$isolationlevels=array("read committed","read uncommitted",
				"repeatable read","serializable");
	foreach ($isolationlevels as $il) {
		# postgresql requires the
		# isolation level to be the first
		# query of the transaction
		sqlrcon_begin($con);
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		assertEqStr(sqlrcon_getIsolationLevel($con),$il);
		sqlrcon_commit($con);
		echo("\n");
	}
	# reset to the default isolation level
	sqlrcon_begin($con);
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	sqlrcon_commit($con);
	echo("\n");


	# create testtable
	echo("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testint int, ".
		"	testfloat float, ".
		"	testreal real, ".
		"	testsmallint smallint, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testdate date, ".
		"	testtime time, ".
		"	testtimestamp timestamp, ".
		"	testtext text, ".
		"	testbytea bytea)"));
	echo("\n");


	# insert
	echo("INSERT: \n");
	assertTrue(sqlrcon_begin($con));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	1.5, ".
		"	1.5, ".
		"	1, ".
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	'01/01/2001', ".
		"	'01:00:00', ".
		"	NULL, ".
		"	'testtext1', ".
		"	'testbytea1')"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	2, ".
		"	2.5, ".
		"	2.5, ".
		"	2, ".
		"	'testchar2', ".
		"	'testvarchar2', ".
		"	'01/01/2002', ".
		"	'02:00:00', ".
		"	NULL, ".
		"	'testtext2', ".
		"	'testbytea2')"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	3, ".
		"	3.5, ".
		"	3.5, ".
		"	3, ".
		"	'testchar3', ".
		"	'testvarchar3', ".
		"	'01/01/2003', ".
		"	'03:00:00', ".
		"	NULL, ".
		"	'testtext3', ".
		"	'testbytea3')"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	4, ".
		"	4.5, ".
		"	4.5, ".
		"	4, ".
		"	'testchar4', ".
		"	'testvarchar4', ".
		"	'01/01/2004', ".
		"	'04:00:00', ".
		"	NULL, ".
		"	'testtext4', ".
		"	'testbytea4')"));
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
		"	\$1, ".
		"	\$2, ".
		"	\$3, ".
		"	\$4, ".
		"	\$5, ".
		"	\$6, ".
		"	\$7, ".
		"	\$8, ".
		"	NULL, ".
		"	\$9, ".
		"	\$10)");
	assertEqInt(sqlrcur_countBindVariables($cur),10);
	sqlrcur_inputBind($cur,"1",5);
	sqlrcur_inputBind($cur,"2",5.5,4,2);
	sqlrcur_inputBind($cur,"3",5.5,4,2);
	sqlrcur_inputBind($cur,"4",5);
	sqlrcur_inputBind($cur,"5","testchar5");
	sqlrcur_inputBind($cur,"6","testvarchar5");
	sqlrcur_inputBind($cur,"7","01/01/2005");
	sqlrcur_inputBind($cur,"8","05:00:00");
	sqlrcur_inputBindClob($cur,"9","testtext5",strlen("testtext5"));
	sqlrcur_inputBindBlob($cur,"10","testbytea5",strlen("testbytea5"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",6);
	sqlrcur_inputBind($cur,"2",6.5,4,2);
	sqlrcur_inputBind($cur,"3",6.5,4,2);
	sqlrcur_inputBind($cur,"4",6);
	sqlrcur_inputBind($cur,"5","testchar6");
	sqlrcur_inputBind($cur,"6","testvarchar6");
	sqlrcur_inputBind($cur,"7","01/01/2006");
	sqlrcur_inputBind($cur,"8","06:00:00");
	sqlrcur_inputBindClob($cur,"9","testtext6",strlen("testtext6"));
	sqlrcur_inputBindBlob($cur,"10","testbytea6",strlen("testbytea6"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",7);
	sqlrcur_inputBind($cur,"2",7.5,4,2);
	sqlrcur_inputBind($cur,"3",7.5,4,2);
	sqlrcur_inputBind($cur,"4",7);
	sqlrcur_inputBind($cur,"5","testchar7");
	sqlrcur_inputBind($cur,"6","testvarchar7");
	sqlrcur_inputBind($cur,"7","01/01/2007");
	sqlrcur_inputBind($cur,"8","07:00:00");
	sqlrcur_inputBindClob($cur,"9","testtext7",strlen("testtext7"));
	sqlrcur_inputBindBlob($cur,"10","testbytea8",strlen("testbytea8"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by position
	# postgresql doesn't support implicit
	# conversion of string binds to other data
	# types, so arrays of binds don't generally
	# work.


	# input bind by name
	# postgresql doesn't support bind by name


	# input bind by position with validation
	echo("BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",8);
	sqlrcur_inputBind($cur,"2",8.5,4,2);
	sqlrcur_inputBind($cur,"3",8.5,4,2);
	sqlrcur_inputBind($cur,"4",8);
	sqlrcur_inputBind($cur,"5","testchar8");
	sqlrcur_inputBind($cur,"6","testvarchar8");
	sqlrcur_inputBind($cur,"7","01/01/2008");
	sqlrcur_inputBind($cur,"8","08:00:00");
	sqlrcur_inputBindClob($cur,"9","testtext8",strlen("testtext8"));
	sqlrcur_inputBindClob($cur,"10","testbytea8",strlen("testbytea8"));
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by name
	# postgresql doesn't support bind by name


	# input bind by name with validation
	# postgresql doesn't support bind by name


	# select
	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount($cur),11);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testreal");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testdate");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testtime");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testtimestamp");
	assertEqStr(sqlrcur_getColumnName($cur,9),"testtext");
	assertEqStr(sqlrcur_getColumnName($cur,10),"testbytea");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testint");
	assertEqStr($cols[1],"testfloat");
	assertEqStr($cols[2],"testreal");
	assertEqStr($cols[3],"testsmallint");
	assertEqStr($cols[4],"testchar");
	assertEqStr($cols[5],"testvarchar");
	assertEqStr($cols[6],"testdate");
	assertEqStr($cols[7],"testtime");
	assertEqStr($cols[8],"testtimestamp");
	assertEqStr($cols[9],"testtext");
	assertEqStr($cols[10],"testbytea");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnType($cur,0),"int4");
	assertEqStr(sqlrcur_getColumnType($cur,"testint"),"int4");
	assertEqStr(sqlrcur_getColumnType($cur,1),"float8");
	assertEqStr(sqlrcur_getColumnType($cur,"testfloat"),"float8");
	assertEqStr(sqlrcur_getColumnType($cur,2),"float4");
	assertEqStr(sqlrcur_getColumnType($cur,"testreal"),"float4");
	assertEqStr(sqlrcur_getColumnType($cur,3),"int2");
	assertEqStr(sqlrcur_getColumnType($cur,"testsmallint"),"int2");
	assertEqStr(sqlrcur_getColumnType($cur,4),"bpchar");
	assertEqStr(sqlrcur_getColumnType($cur,"testchar"),"bpchar");
	assertEqStr(sqlrcur_getColumnType($cur,5),"varchar");
	assertEqStr(sqlrcur_getColumnType($cur,"testvarchar"),"varchar");
	assertEqStr(sqlrcur_getColumnType($cur,6),"date");
	assertEqStr(sqlrcur_getColumnType($cur,"testdate"),"date");
	assertEqStr(sqlrcur_getColumnType($cur,7),"time");
	assertEqStr(sqlrcur_getColumnType($cur,"testtime"),"time");
	assertEqStr(sqlrcur_getColumnType($cur,8),"timestamp");
	assertEqStr(sqlrcur_getColumnType($cur,"testtimestamp"),
		"timestamp");
	assertEqStr(sqlrcur_getColumnType($cur,9),"text");
	assertEqStr(sqlrcur_getColumnType($cur,"testtext"),"text");
	assertEqStr(sqlrcur_getColumnType($cur,10),"bytea");
	assertEqStr(sqlrcur_getColumnType($cur,"testbytea"),"bytea");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLength($cur,0),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testint"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,1),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testfloat"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,2),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testreal"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,3),2);
	assertEqInt(sqlrcur_getColumnLength($cur,"testsmallint"),2);
	assertEqInt(sqlrcur_getColumnLength($cur,4),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,5),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"testvarchar"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,6),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"testdate"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,7),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtime"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,8),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtimestamp"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,9),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testtext"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,10),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testbytea"),0);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongest($cur,0),1);
	assertEqInt(sqlrcur_getLongest($cur,"testint"),1);
	assertEqInt(sqlrcur_getLongest($cur,1),3);
	assertEqInt(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongest($cur,2),3);
	assertEqInt(sqlrcur_getLongest($cur,"testreal"),3);
	assertEqInt(sqlrcur_getLongest($cur,3),1);
	assertEqInt(sqlrcur_getLongest($cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongest($cur,4),40);
	assertEqInt(sqlrcur_getLongest($cur,"testchar"),40);
	assertEqInt(sqlrcur_getLongest($cur,5),12);
	assertEqInt(sqlrcur_getLongest($cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongest($cur,6),10);
	assertEqInt(sqlrcur_getLongest($cur,"testdate"),10);
	assertEqInt(sqlrcur_getLongest($cur,7),8);
	assertEqInt(sqlrcur_getLongest($cur,"testtime"),8);
	assertEqInt(sqlrcur_getLongest($cur,9),9);
	assertEqInt(sqlrcur_getLongest($cur,"testtext"),9);
	assertEqInt(sqlrcur_getLongest($cur,10),10);
	assertEqInt(sqlrcur_getLongest($cur,"testbytea"),10);
	echo("\n");


	# row count
	echo("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount($cur),8);
	echo("\n");


	# total rows
	echo("TOTAL ROWS: \n");
	assertEqInt(sqlrcur_totalRows($cur),8);
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
	assertEqStr(sqlrcur_getField($cur,0,1),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,2),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,3),"1");
	assertEqStr(sqlrcur_getField($cur,0,4),"testchar1".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,0,5),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,6),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,7),"01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,9),"testtext1");
	assertEqStr(sqlrcur_getField($cur,0,10),"testbytea1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,7,1),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,2),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,3),"8");
	assertEqStr(sqlrcur_getField($cur,7,4),"testchar8".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,7,5),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,6),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,7),"08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,9),"testtext8");
	assertEqStr(sqlrcur_getField($cur,7,10),"testbytea8");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,1),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,2),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,3),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,4),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,5),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,6),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,7),8);
	assertEqInt(sqlrcur_getFieldLength($cur,0,9),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,10),10);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,1),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,2),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,3),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,4),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,5),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,6),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,7),8);
	assertEqInt(sqlrcur_getFieldLength($cur,7,9),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,10),10);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testreal"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testchar"),"testchar1".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,0,"testvarchar"),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testdate"),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,"testtime"),"01:00:00");
	assertEqStr(sqlrcur_getField($cur,0,"testtext"),"testtext1");
	assertEqStr(sqlrcur_getField($cur,0,"testbytea"),"testbytea1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testreal"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testchar"),"testchar8".
		"                               ");
	assertEqStr(sqlrcur_getField($cur,7,"testvarchar"),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testdate"),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,"testtime"),"08:00:00");
	assertEqStr(sqlrcur_getField($cur,7,"testtext"),"testtext8");
	assertEqStr(sqlrcur_getField($cur,7,"testbytea"),"testbytea8");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testtext"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testbytea"),10);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testtext"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testbytea"),10);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqStr($fields[0],"1");
	assertEqStr($fields[1],"1.5");
	assertEqStr($fields[2],"1.5");
	assertEqStr($fields[3],"1");
	assertEqStr($fields[4],"testchar1".
		"                               ");
	assertEqStr($fields[5],"testvarchar1");
	assertEqStr($fields[6],"2001-01-01");
	assertEqStr($fields[7],"01:00:00");
	assertEqStr($fields[9],"testtext1");
	assertEqStr($fields[10],"testbytea1");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqInt($fieldlens[0],1);
	assertEqInt($fieldlens[1],3);
	assertEqInt($fieldlens[2],3);
	assertEqInt($fieldlens[3],1);
	assertEqInt($fieldlens[4],40);
	assertEqInt($fieldlens[5],12);
	assertEqInt($fieldlens[6],10);
	assertEqInt($fieldlens[7],8);
	assertEqInt($fieldlens[9],9);
	assertEqInt($fieldlens[10],10);
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
	assertEqStr(sqlrcur_getColumnType($cur,0),"int4");
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
	sqlrcur_cacheToFile($cur,"cachefile1-postgresql");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1-postgresql");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount($cur),11);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testreal");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testsmallint");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,6),"testdate");
	assertEqStr(sqlrcur_getColumnName($cur,7),"testtime");
	assertEqStr(sqlrcur_getColumnName($cur,8),"testtimestamp");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testint");
	assertEqStr($cols[1],"testfloat");
	assertEqStr($cols[2],"testreal");
	assertEqStr($cols[3],"testsmallint");
	assertEqStr($cols[4],"testchar");
	assertEqStr($cols[5],"testvarchar");
	assertEqStr($cols[6],"testdate");
	assertEqStr($cols[7],"testtime");
	assertEqStr($cols[8],"testtimestamp");
	echo("\n");


	# cached result set with result set
	# buffer size
	echo("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1-postgresql");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1-postgresql");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# from one cache file to another
	echo("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile($cur,"cachefile2-postgresql");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1-postgresql"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2-postgresql"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");


	# from one cache file to another
	# with result set buffer size
	echo("FROM ONE CACHE FILE TO ANOTHER ".
		"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile2-postgresql");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1-postgresql"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2-postgresql"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# cached result set with suspend
	# and result set buffer size
	echo("CACHED RESULT SET WITH SUSPEND ".
		"AND RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1-postgresql");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable ".
		"order by testint"));
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1-postgresql");
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


	# nested selects
	echo("NESTED SELECTS: \n");
	sqlrcur_setResultSetBufferSize($cur,1);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	$secondcur=sqlrcur_alloc($con);
	sqlrcur_setResultSetBufferSize($secondcur,1);
	for ($i=0; sqlrcur_getRow($cur,$i); $i++) {
		assertTrue(sqlrcur_sendQuery($secondcur,
			"select * from testtable"));
	}
	sqlrcur_closeResultSet($secondcur);
	sqlrcur_setResultSetBufferSize($cur,0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# reset transaction state
	echo("RESET TRANSACTION STATE: \n");
	assertTrue(sqlrcon_commit($con));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit");
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	# transaction behavior - implicit
	echo("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
	# postgresql DDL is transactional; commit so the table is visible
	# to the second connection (the commit implicitly starts a new tx)
	assertTrue(sqlrcon_commit($con));
	$secondcon=sqlrcon_alloc("sqlrelay",9003,"/tmp/postgresql.socket","testuser",
				"testpassword",0,1);
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
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# transaction behavior - explicit
	echo("TRANSACTION BEHAVIOR - explicit: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"explicit"));
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
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
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit");
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	# individual substitutions
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


	# array substitutions
	echo("ARRAY SUBSTITUTIONS: \n");
	$subvars=array("var1","var2","var3");
	sqlrcur_prepareQuery($cur,"select \$(var1),\$(var2),\$(var3)");
	$subvallongs=array(1,2,3);
	sqlrcur_substitutions($cur,$subvars,$subvallongs);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"2");
	assertEqStr(sqlrcur_getField($cur,0,2),"3");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select '\$(var1)','\$(var2)','\$(var3)'");
	$subvalstrings=array("hi","hello","bye");
	sqlrcur_substitutions($cur,$subvars,$subvalstrings);
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
		"	testclob1 text, ".
		"	testclob2 text, ".
		"	testblob1 bytea, ".
		"	testblob2 bytea)"));
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	\$1, ".
		"	\$2, ".
		"	\$3, ".
		"	\$4)");
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


	# long lobs
	echo("LONG LOBS: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testtext text, ".
		"	testbytea bytea)");
	sqlrcur_prepareQuery($cur,"insert into testtable values (\$1,\$2)");
	$largebuffer="";
	for ($i=0; $i<8192; $i++) {
		$largebuffer=$largebuffer."C";
	}
	sqlrcur_inputBindClob($cur,"1",$largebuffer,strlen($largebuffer));
	sqlrcur_inputBindBlob($cur,"2",$largebuffer,strlen($largebuffer));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLength($cur,0,
					"testtext"),8192);
	assertEqStr(sqlrcur_getField($cur,0,"testtext"),$largebuffer);
	assertEqInt(sqlrcur_getFieldLength($cur,0,
					"testbytea"),8192);
	assertEqStrLen(sqlrcur_getField($cur,0,"testbytea"),$largebuffer,
		8192);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# output bind by position
	# postgresql doesn't support output binds


	# output bind by name
	# postgresql doesn't support output binds


	# output bind by name with validation
	# postgresql doesn't support output binds


	# lob output bind
	# postgresql doesn't support output binds


	# long output bind
	# postgresql doesn't support output binds


	# negative input bind
	echo("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,"create table testtable (testval int)");
	sqlrcur_prepareQuery($cur,"insert into testtable values (\$1)");
	sqlrcur_inputBind($cur,"1",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable");
	assertEqStr(sqlrcur_getField($cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# bind validation
	# postgresql doesn't support bind by name


	# rebinding
	echo("REBINDING: \n");
	sqlrcur_sendQuery($cur,"drop function testfunc(int)");
	assertTrue(sqlrcur_sendQuery($cur,"create function testfunc(int) ".
		"returns int as ".
		"	' begin return \$1; end;' language plpgsql"));
	sqlrcur_prepareQuery($cur,"select * from testfunc(\$1)");
	sqlrcur_inputBind($cur,"1",1);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcur_inputBind($cur,"1",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"2");
	sqlrcur_inputBind($cur,"1",3);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"3");
	assertTrue(sqlrcur_sendQuery($cur,"drop function testfunc(int)"));
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
	sqlrcur_prepareQuery($cur,"select \$1::int");
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
	sqlrcur_sendQuery($cur,"drop function testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testfunc(".
		"	int,float,char(20)) ".
		"returns void as ' ".
		"	declare in1 int; ".
		"	in2 float; ".
		"	in3 char(20); ".
		"begin ".
		"	in1:=\$1; ".
		"	in2:=\$2; ".
		"	in3:=\$3; ".
		"	return; end;' language plpgsql"));
	sqlrcur_prepareQuery($cur,"select testfunc(\$1,\$2,\$3)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.5,4,2);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop function ".
		"testfunc(int,float,char(20))"));
	echo("\n");


	# stored procedure returning single value
	echo("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery($cur,"drop function testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery($cur,"create function ".
		"testfunc(int,float,char(20)) ".
		"returns int as ".
		"	' begin return \$1; end;' language plpgsql"));
	sqlrcur_prepareQuery($cur,"select * from testfunc(\$1,\$2,\$3)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.5,4,2);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertTrue(sqlrcur_sendQuery($cur,"drop function ".
		"testfunc(int,float,char(20))"));
	echo("\n");


	# stored procedure returning
	# multiple values
	echo("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery($cur,"drop function testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testfunc(".
		"	int,float,char(20)) ".
		"returns record as ' ".
		"	declare output record; ".
		"begin ".
		"	select \$1,\$2,\$3 ".
		"	into output; ".
		"	return output; end;' language plpgsql"));
	sqlrcur_prepareQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testfunc(\$1,\$2,\$3) ".
		"	as (col1 int, ".
		"		col2 float, ".
		"		col3 bpchar) ");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.5,4,2);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),"hello");
	assertTrue(sqlrcur_sendQuery($cur,"drop function ".
		"testfunc(int,float,char(20))"));
	echo("\n");


	# stored procedure returning result set
	echo("STORED PROCEDURE RETURNING RESULT SET: \n");
	sqlrcur_sendQuery($cur,"drop function testfunc()");
	assertTrue(sqlrcur_sendQuery($cur,"create function testfunc() ".
		"returns setof record as ' ".
		"	declare output record; ".
		"begin ".
		"	for output in ".
		"		select 1 ".
		"		union ".
		"		select 2 ".
		"		union ".
		"		select 3 ".
		"		union ".
		"		select 4 ".
		"		union ".
		"		select 5 ".
		"		union ".
		"		select 6 ".
		"		union ".
		"		select 7 ".
		"		union ".
		"		select 8 ".
		"	loop ".
		"		return next output; ".
		"	end loop; ".
		"	return; end;' language plpgsql"));
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testfunc() ".
		"	as (testint int)"));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertTrue(sqlrcur_sendQuery($cur,"drop function testfunc()"));
	echo("\n");


	# temporary tables
	echo("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery($cur,"drop table temptable\n");
	sqlrcur_sendQuery($cur,"create temporary table temptable (col1 int)");
	assertTrue(sqlrcur_sendQuery($cur,"insert into temptable values (1)"));
	assertTrue(sqlrcur_sendQuery($cur,"select count(*) from temptable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcon_endSession($con);
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"select count(*) from temptable"));
	echo("\n");


	# encoded binary data
	echo("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 bytea)"));
	$buffer="";
	for ($i=0; $i<256; $i++) {
		$buffer=$buffer.chr($i);
	}
	$querystr="insert into testtable values (decode('";
	for ($i=0; $i<strlen($buffer); $i++) {
		$querystr=$querystr.sprintf("%02x",ord($buffer[$i]));
	}
	$querystr=$querystr."','hex'))";
	assertTrue(sqlrcur_sendQuery($cur,$querystr));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),strlen($buffer));
	assertEqInt(strcmp(sqlrcur_getField($cur,0,0),$buffer),0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# quotes
	echo("QUOTES: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values ('''''')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqInt(strcmp(sqlrcur_getField($cur,0,0),"''"),0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# last insert id
	echo("LAST INSERT ID: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable ".
		"	(col1 serial primary key, ".
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
	assertTrue(sqlrcur_getSchemaList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	assertInResultSet($cur,"Database","public");
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
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"INTEGER");
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
		"	testint int, ".
		"	testfloat float, ".
		"	testreal real, ".
		"	testsmallint smallint, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testdate date, ".
		"	testtime time, ".
		"	testtimestamp timestamp, ".
		"	testtext text, ".
		"	testbytea bytea)"));
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
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),"testint");
	assertEqStr(sqlrcur_getField($cur,1,"column_name"),"testfloat");
	assertEqStr(sqlrcur_getField($cur,2,"column_name"),"testreal");
	assertEqStr(sqlrcur_getField($cur,3,"column_name"),"testsmallint");
	assertEqStr(sqlrcur_getField($cur,4,"column_name"),"testchar");
	assertEqStr(sqlrcur_getField($cur,5,"column_name"),"testvarchar");
	assertEqStr(sqlrcur_getField($cur,6,"column_name"),"testdate");
	assertEqStr(sqlrcur_getField($cur,7,"column_name"),"testtime");
	assertEqStr(sqlrcur_getField($cur,8,"column_name"),
		"testtimestamp");
	assertEqStr(sqlrcur_getField($cur,9,"column_name"),"testtext");
	assertEqStr(sqlrcur_getField($cur,10,"column_name"),"testbytea");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"integer");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),
		"double precision");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"real");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"smallint");
	assertEqStr(sqlrcur_getField($cur,4,"data_type"),"character");
	assertEqStr(sqlrcur_getField($cur,5,"data_type"),
		"character varying");
	assertEqStr(sqlrcur_getField($cur,6,"data_type"),"date");
	assertEqStr(sqlrcur_getField($cur,7,"data_type"),
		"time without time zone");
	assertEqStr(sqlrcur_getField($cur,8,"data_type"),
		"timestamp without time zone");
	assertEqStr(sqlrcur_getField($cur,9,"data_type"),"text");
	assertEqStr(sqlrcur_getField($cur,10,"data_type"),"bytea");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# column list - auto_increment, primary key
	echo("COLUMN LIST - auto_increment, primary key: \n");
	sqlrcur_sendQuery($cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 serial primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getField($cur,0,"extra"),"auto_increment");
	assertEqStr(sqlrcur_getField($cur,0,"column_key"),"PRI");
	assertEqStr(sqlrcur_getField($cur,1,"extra"),"");
	assertEqStr(sqlrcur_getField($cur,1,"column_key"),"");
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getField($cur,0,"extra"),"");
	assertEqStr(sqlrcur_getField($cur,0,"column_key"),"PRI");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# primary keys list
	echo("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int primary key, ".
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
	$keyname=sqlrcur_getField($cur,0,"key_name");
	assertEqStr($keyname,"testtable_pkey");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# key and index list
	echo("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int primary key, ".
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
	assertEqStr(sqlrcur_getField($cur,0,"non_unique"),"f");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"column_name"),"col1"));
	assertEqStr(sqlrcur_getField($cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getField($cur,0,"index_type"),"3");
	$keyname=sqlrcur_getField($cur,0,"key_name");
	assertEqStr($keyname,"testtable_pkey");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# procedure list
	echo("PROCEDURE LIST: \n");
	sqlrcur_sendQuery($cur,"drop function testproc1(int,char,".
		"varchar,date)");
	sqlrcur_sendQuery($cur,"drop function testproc2(int,char,".
		"varchar,date)");
	sqlrcur_sendQuery($cur,"drop function testproc3(int,char,".
		"varchar,date)");
	sqlrcur_sendQuery($cur,"drop function testproc4(int,char,".
		"varchar,date)");
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testproc1(".
		"	in1 int, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) returns void as 'begin end;' ".
		"language plpgsql"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testproc2(".
		"	in1 int, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) returns void as 'begin end;' ".
		"language plpgsql"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testproc3(".
		"	in1 int, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) returns void as 'begin end;' ".
		"language plpgsql"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testproc4(".
		"	in1 int, ".
		"	in2 char(20), ".
		"	in3 varchar(20), ".
		"	in4 date) returns void as 'begin end;' ".
		"language plpgsql"));
	assertTrue(sqlrcur_getProcedureList($cur,NULL));
	assertInResultSet($cur,"routine_name","testproc1");
	assertInResultSet($cur,"routine_name","testproc2");
	assertInResultSet($cur,"routine_name","testproc3");
	assertInResultSet($cur,"routine_name","testproc4");
	echo("\n");


	# procedure parameter list
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
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"integer");
	assertEqStr(sqlrcur_getField($cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_name"),"in2");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"character");
	assertEqStr(sqlrcur_getField($cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_name"),"in3");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),
		"character varying");
	assertEqStr(sqlrcur_getField($cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_name"),"in4");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"date");
	assertEqStr(sqlrcur_getField($cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery($cur,"drop function testproc1(int,char,".
		"varchar,date)"));
	assertTrue(sqlrcur_sendQuery($cur,"drop function testproc2(int,char,".
		"varchar,date)"));
	assertTrue(sqlrcur_sendQuery($cur,"drop function testproc3(int,char,".
		"varchar,date)"));
	assertTrue(sqlrcur_sendQuery($cur,"drop function testproc4(int,char,".
		"varchar,date)"));
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

	# error sqlstate
	echo("ERROR SQLSTATE: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (col1 int)"));
	assertEqStr(sqlrcur_errorSqlState($cur),"");
	assertFalse(sqlrcur_sendQuery($cur,
		"create table testtable (col1 int)"));
	assertEqStr(sqlrcur_errorSqlState($cur),"42P07");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertEqStr(sqlrcur_errorSqlState($cur),"");
	echo("\n");

	reportTestStatus();

	exit($status);
?></pre></html>
