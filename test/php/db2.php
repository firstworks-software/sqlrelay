<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.
	include("./asserts.php");


	$isolationlevels=array("CS","UR","RS","RR");
	$bindvars=array("1","2","3","4","5","6",
				"7","8","9","10","11","12");
	$bindvals=array("7","7","7","7.7","7.7","7.7",
				"testchar7","testvarchar7",
				"01/01/2007","07:00:00","testclob7",null);
	$subvars=array("var1","var2","var3");
	$subvallongs=array(1,2,3);
	$subvalstrings=array("hi","hello","bye");
	$subvaldoubles=array(10.55,10.556,10.5556);
	$precs=array(4,5,6);
	$scales=array(2,3,4);

	$LARGE_BUFFER_LENGTH=20*1024;


	# instantiation
	$con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
				"db2inst1","testpassword",0,1);
	$cur=sqlrcur_alloc($con);


	# identify
	echo("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify($con),"db2");
	echo("\n");


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	# bind format
	echo("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat($con),"?");
	echo("\n");


	# nextval format
	echo("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat($con),"(nextval for %s)");
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
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testsmallint smallint, ".
		"	testint integer, ".
		"	testbigint bigint, ".
		"	testdecimal decimal(10,2), ".
		"	testreal real, ".
		"	testdouble double, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testdate date, ".
		"	testtime time, ".
		"	testtimestamp timestamp, ".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcon_commit($con));
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
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	'01/01/2001', ".
		"	'01:00:00', ".
		"	NULL, ".
		"	'testclob1', ".
		"	blob('testblob1'))"));
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
		"	NULL, ".
		"	?, ".
		"	?)");
	assertEqInt(sqlrcur_countBindVariables($cur),12);
	sqlrcur_inputBind($cur,"1",2);
	sqlrcur_inputBind($cur,"2",2);
	sqlrcur_inputBind($cur,"3",2);
	sqlrcur_inputBind($cur,"4",2.2,4,2);
	sqlrcur_inputBind($cur,"5",2.2,4,2);
	sqlrcur_inputBind($cur,"6",2.2,4,2);
	sqlrcur_inputBind($cur,"7","testchar2");
	sqlrcur_inputBind($cur,"8","testvarchar2");
	sqlrcur_inputBindDate($cur,"9",2002,1,1,-1,-1,-1,-1,null,0);
	sqlrcur_inputBindDate($cur,"10",-1,-1,-1,2,0,0,0,null,0);
	sqlrcur_inputBindClob($cur,"11","testclob2",strlen("testclob2"));
	sqlrcur_inputBindBlob($cur,"12","testblob2",strlen("testblob2"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",3);
	sqlrcur_inputBind($cur,"2",3);
	sqlrcur_inputBind($cur,"3",3);
	sqlrcur_inputBind($cur,"4",3.3,4,2);
	sqlrcur_inputBind($cur,"5",3.3,4,2);
	sqlrcur_inputBind($cur,"6",3.3,4,2);
	sqlrcur_inputBind($cur,"7","testchar3");
	sqlrcur_inputBind($cur,"8","testvarchar3");
	sqlrcur_inputBindDate($cur,"9",2003,1,1,-1,-1,-1,-1,null,0);
	sqlrcur_inputBindDate($cur,"10",-1,-1,-1,3,0,0,0,null,0);
	sqlrcur_inputBindClob($cur,"11","testclob3",strlen("testclob3"));
	sqlrcur_inputBindBlob($cur,"12","testblob3",strlen("testblob3"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",4);
	sqlrcur_inputBind($cur,"2",4);
	sqlrcur_inputBind($cur,"3",4);
	sqlrcur_inputBind($cur,"4",4.4,4,2);
	sqlrcur_inputBind($cur,"5",4.4,4,2);
	sqlrcur_inputBind($cur,"6",4.4,4,2);
	sqlrcur_inputBind($cur,"7","testchar4");
	sqlrcur_inputBind($cur,"8","testvarchar4");
	sqlrcur_inputBindDate($cur,"9",2004,1,1,-1,-1,-1,-1,null,0);
	sqlrcur_inputBindDate($cur,"10",-1,-1,-1,4,0,0,0,null,0);
	sqlrcur_inputBindClob($cur,"11","testclob4",strlen("testclob4"));
	sqlrcur_inputBindBlob($cur,"12","testblob4",strlen("testblob4"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",5);
	sqlrcur_inputBind($cur,"2",5);
	sqlrcur_inputBind($cur,"3",5);
	sqlrcur_inputBind($cur,"4",5.5,4,2);
	sqlrcur_inputBind($cur,"5",5.5,4,2);
	sqlrcur_inputBind($cur,"6",5.5,4,2);
	sqlrcur_inputBind($cur,"7","testchar5");
	sqlrcur_inputBind($cur,"8","testvarchar5");
	sqlrcur_inputBindDate($cur,"9",2005,1,1,-1,-1,-1,-1,null,0);
	sqlrcur_inputBindDate($cur,"10",-1,-1,-1,5,0,0,0,null,0);
	sqlrcur_inputBindClob($cur,"11","testclob5",strlen("testclob5"));
	sqlrcur_inputBindBlob($cur,"12","testblob5",strlen("testblob5"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",6);
	sqlrcur_inputBind($cur,"2",6);
	sqlrcur_inputBind($cur,"3",6);
	sqlrcur_inputBind($cur,"4",6.6,4,2);
	sqlrcur_inputBind($cur,"5",6.6,4,2);
	sqlrcur_inputBind($cur,"6",6.6,4,2);
	sqlrcur_inputBind($cur,"7","testchar6");
	sqlrcur_inputBind($cur,"8","testvarchar6");
	sqlrcur_inputBindDate($cur,"9",2006,1,1,-1,-1,-1,-1,null,0);
	sqlrcur_inputBindDate($cur,"10",-1,-1,-1,6,0,0,0,null,0);
	sqlrcur_inputBindClob($cur,"11","testclob6",strlen("testclob6"));
	sqlrcur_inputBindBlob($cur,"12","testblob6",strlen("testblob6"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by position
	echo("ARRAY OF INPUT BINDS BY POSITION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBinds($cur,$bindvars,$bindvals);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# input bind by position with validation
	echo("INPUT BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",8);
	sqlrcur_inputBind($cur,"2",8);
	sqlrcur_inputBind($cur,"3",8);
	sqlrcur_inputBind($cur,"4",8.8,4,2);
	sqlrcur_inputBind($cur,"5",8.8,4,2);
	sqlrcur_inputBind($cur,"6",8.8,4,2);
	sqlrcur_inputBind($cur,"7","testchar8");
	sqlrcur_inputBind($cur,"8","testvarchar8");
	sqlrcur_inputBindDate($cur,"9",2008,1,1,-1,-1,-1,-1,null,0);
	sqlrcur_inputBindDate($cur,"10",-1,-1,-1,8,0,0,0,null,0);
	sqlrcur_inputBindClob($cur,"11","testclob8",strlen("testclob8"));
	sqlrcur_inputBindBlob($cur,"12","testblob8",strlen("testblob8"));
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");

	# input bind by name
	# db2 doesn't support bind by name


	# array of input binds by name
	# db2 doesn't support bind by name


	# input bind by name with validation
	# db2 doesn't support bind by name


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
	assertEqInt(sqlrcur_colCount($cur),13);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"TESTSMALLINT");
	assertEqStr(sqlrcur_getColumnName($cur,1),"TESTINT");
	assertEqStr(sqlrcur_getColumnName($cur,2),"TESTBIGINT");
	assertEqStr(sqlrcur_getColumnName($cur,3),"TESTDECIMAL");
	assertEqStr(sqlrcur_getColumnName($cur,4),"TESTREAL");
	assertEqStr(sqlrcur_getColumnName($cur,5),"TESTDOUBLE");
	assertEqStr(sqlrcur_getColumnName($cur,6),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,7),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,8),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName($cur,9),"TESTTIME");
	assertEqStr(sqlrcur_getColumnName($cur,10),"TESTTIMESTAMP");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"TESTSMALLINT");
	assertEqStr($cols[1],"TESTINT");
	assertEqStr($cols[2],"TESTBIGINT");
	assertEqStr($cols[3],"TESTDECIMAL");
	assertEqStr($cols[4],"TESTREAL");
	assertEqStr($cols[5],"TESTDOUBLE");
	assertEqStr($cols[6],"TESTCHAR");
	assertEqStr($cols[7],"TESTVARCHAR");
	assertEqStr($cols[8],"TESTDATE");
	assertEqStr($cols[9],"TESTTIME");
	assertEqStr($cols[10],"TESTTIMESTAMP");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnType($cur,0),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTSMALLINT"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnType($cur,1),"INTEGER");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTINT"),"INTEGER");
	assertEqStr(sqlrcur_getColumnType($cur,2),"BIGINT");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTBIGINT"),"BIGINT");
	assertEqStr(sqlrcur_getColumnType($cur,3),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTDECIMAL"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnType($cur,4),"REAL");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTREAL"),"REAL");
	assertEqStr(sqlrcur_getColumnType($cur,5),"DOUBLE");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTDOUBLE"),"DOUBLE");
	assertEqStr(sqlrcur_getColumnType($cur,6),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTCHAR"),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,7),"VARCHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTVARCHAR"),"VARCHAR");
	assertEqStr(sqlrcur_getColumnType($cur,8),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTDATE"),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,9),"TIME");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTTIME"),"TIME");
	assertEqStr(sqlrcur_getColumnType($cur,10),"TIMESTAMP");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTTIMESTAMP"),
		"TIMESTAMP");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLength($cur,0),2);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTSMALLINT"),2);
	assertEqInt(sqlrcur_getColumnLength($cur,1),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTINT"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,2),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTBIGINT"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,3),12);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTDECIMAL"),12);
	assertEqInt(sqlrcur_getColumnLength($cur,4),4);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTREAL"),4);
	assertEqInt(sqlrcur_getColumnLength($cur,5),8);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTDOUBLE"),8);
	assertEqInt(sqlrcur_getColumnLength($cur,6),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,7),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTVARCHAR"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,8),6);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTDATE"),6);
	assertEqInt(sqlrcur_getColumnLength($cur,9),6);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTTIME"),6);
	assertEqInt(sqlrcur_getColumnLength($cur,10),16);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTTIMESTAMP"),16);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongest($cur,0),1);
	assertEqInt(sqlrcur_getLongest($cur,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getLongest($cur,1),1);
	assertEqInt(sqlrcur_getLongest($cur,"TESTINT"),1);
	assertEqInt(sqlrcur_getLongest($cur,2),1);
	assertEqInt(sqlrcur_getLongest($cur,"TESTBIGINT"),1);
	assertEqInt(sqlrcur_getLongest($cur,3),4);
	assertEqInt(sqlrcur_getLongest($cur,"TESTDECIMAL"),4);
	#assertEqInt(sqlrcur_getLongest($cur,4),3);
	#assertEqInt(sqlrcur_getLongest($cur,"TESTREAL"),3);
	#assertEqInt(sqlrcur_getLongest($cur,5),3);
	#assertEqInt(sqlrcur_getLongest($cur,"TESTDOUBLE"),3);
	assertEqInt(sqlrcur_getLongest($cur,6),40);
	assertEqInt(sqlrcur_getLongest($cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getLongest($cur,7),12);
	assertEqInt(sqlrcur_getLongest($cur,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getLongest($cur,8),10);
	assertEqInt(sqlrcur_getLongest($cur,"TESTDATE"),10);
	assertEqInt(sqlrcur_getLongest($cur,9),8);
	assertEqInt(sqlrcur_getLongest($cur,"TESTTIME"),8);
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
	assertEqStr(sqlrcur_getField($cur,0,3),"1.10");
	#assertEqStr(sqlrcur_getField($cur,0,4),"1.1");
	#assertEqStr(sqlrcur_getField($cur,0,5),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,6),"testchar1".
					"                               ");
	assertEqStr(sqlrcur_getField($cur,0,7),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,8),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,9),"01:00:00");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,7,1),"8");
	assertEqStr(sqlrcur_getField($cur,7,2),"8");
	assertEqStr(sqlrcur_getField($cur,7,3),"8.80");
	#assertEqStr(sqlrcur_getField($cur,7,4),"8.8");
	#assertEqStr(sqlrcur_getField($cur,7,5),"8.8");
	assertEqStr(sqlrcur_getField($cur,7,6),"testchar8".
					"                               ");
	assertEqStr(sqlrcur_getField($cur,7,7),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,8),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,9),"08:00:00");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,3),4);
	#assertEqInt(sqlrcur_getFieldLength($cur,0,4),3);
	#assertEqInt(sqlrcur_getFieldLength($cur,0,5),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,6),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,7),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,8),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,9),8);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,3),4);
	#assertEqInt(sqlrcur_getFieldLength($cur,7,4),3);
	#assertEqInt(sqlrcur_getFieldLength($cur,7,5),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,6),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,7),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,8),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,9),8);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getField($cur,0,"TESTSMALLINT"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTINT"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTBIGINT"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTDECIMAL"),"1.10");
	#assertEqStr(sqlrcur_getField($cur,0,"TESTREAL"),"1.1");
	#assertEqStr(sqlrcur_getField($cur,0,"TESTDOUBLE"),"1.1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTCHAR"),"testchar1".
					"                               ");
	assertEqStr(sqlrcur_getField($cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTDATE"),"2001-01-01");
	assertEqStr(sqlrcur_getField($cur,0,"TESTTIME"),"01:00:00");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,"TESTSMALLINT"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTINT"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTBIGINT"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTDECIMAL"),"8.80");
	#assertEqStr(sqlrcur_getField($cur,7,"TESTREAL"),"8.8");
	#assertEqStr(sqlrcur_getField($cur,7,"TESTDOUBLE"),"8.8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTCHAR"),"testchar8".
					"                               ");
	assertEqStr(sqlrcur_getField($cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTDATE"),"2008-01-01");
	assertEqStr(sqlrcur_getField($cur,7,"TESTTIME"),"08:00:00");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTINT"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTBIGINT"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTDECIMAL"),4);
	#assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTREAL"),3);
	#assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTDOUBLE"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTDATE"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTTIME"),8);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTINT"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTBIGINT"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTDECIMAL"),4);
	#assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTREAL"),3);
	#assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTDOUBLE"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTDATE"),10);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTTIME"),8);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqStr($fields[0],"1");
	assertEqStr($fields[1],"1");
	assertEqStr($fields[2],"1");
	assertEqStr($fields[3],"1.10");
	#assertEqStr($fields[4],"1.1");
	#assertEqStr($fields[5],"1.1");
	assertEqStr($fields[6],"testchar1"."                               ");
	assertEqStr($fields[7],"testvarchar1");
	assertEqStr($fields[8],"2001-01-01");
	assertEqStr($fields[9],"01:00:00");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqInt($fieldlens[0],1);
	assertEqInt($fieldlens[1],1);
	assertEqInt($fieldlens[2],1);
	assertEqInt($fieldlens[3],4);
	#assertEqInt($fieldlens[4],3);
	#assertEqInt($fieldlens[5],3);
	assertEqInt($fieldlens[6],40);
	assertEqInt($fieldlens[7],12);
	assertEqInt($fieldlens[8],10);
	assertEqInt($fieldlens[9],8);
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
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName($cur,0),NULL);
	assertEqInt(sqlrcur_getColumnLength($cur,0),0);
	assertEqStr(sqlrcur_getColumnType($cur,0),NULL);
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName($cur,0),"TESTSMALLINT");
	assertEqInt(sqlrcur_getColumnLength($cur,0),2);
	assertEqStr(sqlrcur_getColumnType($cur,0),"SMALLINT");
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
		"	testsmallint "));
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
		"	testsmallint "));
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
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testsmallint "));
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
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount($cur),13);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"TESTSMALLINT");
	assertEqStr(sqlrcur_getColumnName($cur,1),"TESTINT");
	assertEqStr(sqlrcur_getColumnName($cur,2),"TESTBIGINT");
	assertEqStr(sqlrcur_getColumnName($cur,3),"TESTDECIMAL");
	assertEqStr(sqlrcur_getColumnName($cur,4),"TESTREAL");
	assertEqStr(sqlrcur_getColumnName($cur,5),"TESTDOUBLE");
	assertEqStr(sqlrcur_getColumnName($cur,6),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,7),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,8),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName($cur,9),"TESTTIME");
	assertEqStr(sqlrcur_getColumnName($cur,10),"TESTTIMESTAMP");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"TESTSMALLINT");
	assertEqStr($cols[1],"TESTINT");
	assertEqStr($cols[2],"TESTBIGINT");
	assertEqStr($cols[3],"TESTDECIMAL");
	assertEqStr($cols[4],"TESTREAL");
	assertEqStr($cols[5],"TESTDOUBLE");
	assertEqStr($cols[6],"TESTCHAR");
	assertEqStr($cols[7],"TESTVARCHAR");
	assertEqStr($cols[8],"TESTDATE");
	assertEqStr($cols[9],"TESTTIME");
	assertEqStr($cols[10],"TESTTIMESTAMP");
	echo("\n");


	# cached result set with result set
	# buffer size
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
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# from one cache file to another
	echo("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");


	# from one cache file to another
	# with result set buffer size
	echo("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET ".
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


	# cached result set with suspend
	# and result set buffer size
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
		"	testsmallint "));
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
	echo("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testint"));
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
	for ($i=0; sqlrcur_getRow($cur,$i); $i++) {
		$secondcur=sqlrcur_alloc($con);
		sqlrcur_setResultSetBufferSize($secondcur,1);
		assertTrue(sqlrcur_sendQuery($secondcur,
				"select * from testtable"));
		sqlrcur_closeResultSet($secondcur);
	}
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");


	# commit and rollback
	echo("COMMIT AND ROLLBACK: \n");
	$secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
					"db2inst1","testpassword",0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	assertTrue(sqlrcur_sendQuery(
			$secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"0");
	assertTrue(sqlrcon_commit($con));
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
		"	10.1, ".
		"	10.1, ".
		"	10.1, ".
		"	'testchar10', ".
		"	'testvarchar10', ".
		"	'01/01/2010', ".
		"	'10:00:00', ".
		"	NULL, ".
		"	'testclob10', ".
		"	blob('testblob10'))"));
	assertTrue(sqlrcon_rollback($con));
	assertTrue(sqlrcur_sendQuery(
			$secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"8");
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	10, ".
		"	10, ".
		"	10, ".
		"	10.1, ".
		"	10.1, ".
		"	10.1, ".
		"	'testchar10', ".
		"	'testvarchar10', ".
		"	'01/01/2010', ".
		"	'10:00:00', ".
		"	NULL, ".
		"	'testclob10', ".
		"	blob('testblob10'))"));
	assertTrue(sqlrcur_sendQuery(
			$secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getField($secondcur,0,0),"9");
	sqlrcon_endSession($secondcon);
	assertTrue(sqlrcon_autoCommitOff($con));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"values ($(var1),'$(var2)','$(var3)')");
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
	sqlrcur_prepareQuery($cur,"values ('$(var1)','$(var2)','$(var3)')");
	sqlrcur_substitutions($cur,$subvars,$subvalstrings);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"hi");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"bye");
	echo("\n");
	sqlrcur_prepareQuery($cur,"values ($(var1),$(var2),$(var3))");
	sqlrcur_substitutions($cur,$subvars,$subvallongs);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"2");
	assertEqStr(sqlrcur_getField($cur,0,2),"3");
	echo("\n");
	sqlrcur_prepareQuery($cur,"values ($(var1),$(var2),$(var3))");
	sqlrcur_substitutions($cur,$subvars,$subvaldoubles,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"10.55");
	assertEqStr(sqlrcur_getField($cur,0,1),"10.556");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	# nulls as nulls
	echo("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL ".
		"from sysibm.sysdummy1"));
	assertEqStr(sqlrcur_getField($cur,0,0),NULL);
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL ".
		"from sysibm.sysdummy1"));
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
	assertEqStr(sqlrcur_getField($cur,0,0),"");
	assertEqStr(sqlrcur_getField($cur,0,1),NULL);
	assertEqStr(sqlrcur_getField($cur,0,2),"");
	assertEqStr(sqlrcur_getField($cur,0,3),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# long lobs
	echo("LONG LOBS: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"insert into testtable values (?,?)");
	$largebuffer=str_repeat("C",$LARGE_BUFFER_LENGTH);
	sqlrcur_inputBindClob($cur,"1",$largebuffer,strlen($largebuffer));
	sqlrcur_inputBindBlob($cur,"2",$largebuffer,strlen($largebuffer));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTCLOB"),
		$LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getField($cur,0,"TESTCLOB"),$largebuffer);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTBLOB"),
		$LARGE_BUFFER_LENGTH);
	assertEqStrLen(sqlrcur_getField($cur,0,"TESTBLOB"),$largebuffer,
		$LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# output bind by position
	echo("OUTPUT BIND BY POSITION: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	out out1 int, ".
		"	out out2 varchar(20), ".
		"	out out3 double, ".
		"	out out4 date, ".
		"	out out5 varchar(20)) language sql ".
		"begin ".
		"	set out1 = 1; ".
		"	set out2 = 'hello'; ".
		"	set out3 = 2.5; ".
		"	set out4 = '2001-02-03'; ".
		"	set out5 = null; end"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"call testproc(?,?,?,?,?)");
	assertEqInt(sqlrcur_countBindVariables($cur),5);
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
	$nullvar=sqlrcur_getOutputBindString($cur,"5");
	assertEqStr($nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# output bind by name
	# db2 doesn't support bind by name


	# output bind by name with validation
	# db2 doesn't support bind by name


	# lob output bind
	echo("LOB OUTPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob clob, ".
		"	testblob blob)");
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"insert into testtable values ('hello',?)");
	sqlrcur_inputBindBlob($cur,"1","hello",strlen("hello"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	out out1 clob, ".
		"	out out2 blob) language sql ".
		"begin ".
		"	select testclob into out1 ".
		"		from testtable; ".
		"	select testblob into out2 ".
		"		from testtable; end"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"call testproc(?,?)");
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
		"	in in1 clob, ".
		"	out out1 clob) language sql ".
		"begin ".
		"	set out1 = in1; end"));
	assertTrue(sqlrcon_commit($con));
	$largebuffer=str_repeat("C",$LARGE_BUFFER_LENGTH);
	sqlrcur_prepareQuery($cur,"call testproc(?,?)");
	sqlrcur_inputBindClob($cur,"1",$largebuffer,strlen($largebuffer));
	sqlrcur_defineOutputBindClob($cur,"2");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindLength($cur,"2"),$LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getOutputBindClob($cur,"2"),$largebuffer);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# negative input bind
	echo("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,"create table testtable (testval integer)");
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"insert into testtable values (?)");
	sqlrcur_inputBind($cur,"1",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable");
	assertEqStr(sqlrcur_getField($cur,0,"TESTVAL"),"-1");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# bind validation
	# db2 doesn't support bind by name


	# rebinding
	echo("REBINDING: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in in1 int, ".
		"	out out1 int) language sql ".
		"begin ".
		"	set out1 = in1; end"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"call testproc(?,?)");
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
	sqlrcur_prepareQuery($cur,"select 1 from sysibm.sysdummy1");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select cast(? as integer) ".
		"from sysibm.sysdummy1");
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
		"	in in1 int, ".
		"	in in2 double, ".
		"	in in3 varchar(20)) language sql ".
		"begin ".
		"	return; end"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"call testproc(?,?,?)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# stored procedure returning single value
	echo("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery($cur,"drop function testfunc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testfunc(".
		"	in1 int, ".
		"	in2 double, ".
		"	in3 varchar(20)) returns int language sql ".
		"begin ".
		"	return in1; end"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"select testfunc(?,?,?) ".
		"from sysibm.sysdummy1");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertTrue(sqlrcur_sendQuery($cur,"drop function testfunc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# stored procedure returning
	# multiple values
	echo("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc(".
		"	in in1 int, ".
		"	in in2 double, ".
		"	in in3 varchar(20), ".
		"	in in4 clob, ".
		"	in in5 blob, ".
		"	out out1 int, ".
		"	out out2 double, ".
		"	out out3 varchar(20), ".
		"	out out4 clob, ".
		"	out out5 blob) language sql ".
		"begin ".
		"	set out1 = in1; ".
		"	set out2 = in2; ".
		"	set out3 = in3; ".
		"	set out4 = in4; ".
		"	set out5 = in5; end"));
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,"call testproc(?,?,?,?,?,?,?,?,?,?)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	sqlrcur_inputBindClob($cur,"4","clob",strlen("clob"));
	sqlrcur_inputBindBlob($cur,"5","blob",strlen("blob"));
	sqlrcur_defineOutputBindInteger($cur,"6");
	sqlrcur_defineOutputBindDouble($cur,"7");
	sqlrcur_defineOutputBindString($cur,"8",20);
	sqlrcur_defineOutputBindClob($cur,"9");
	sqlrcur_defineOutputBindBlob($cur,"10");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"6"),1);
	assertEqDbl(sqlrcur_getOutputBindDouble($cur,"7"),1.1);
	assertEqStr(sqlrcur_getOutputBindString($cur,"8"),"hello");
	assertEqStr(sqlrcur_getOutputBindClob($cur,"9"),"clob");
	assertEqStr(sqlrcur_getOutputBindBlob($cur,"10"),"blob");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# stored procedure returning result set
	echo("STORED PROCEDURE RETURNING RESULT SET: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,"create procedure testproc() ".
		"result set 1 language sql ".
		"begin ".
		"	declare c1 cursor ".
		"		with return for ".
		"		select 1 ".
		"		from sysibm.sysdummy1 ".
		"		union ".
		"		select 2 ".
		"		from sysibm.sysdummy1 ".
		"		union ".
		"		select 3 ".
		"		from sysibm.sysdummy1 ".
		"		union ".
		"		select 4 ".
		"		from sysibm.sysdummy1 ".
		"		union ".
		"		select 5 ".
		"		from sysibm.sysdummy1 ".
		"		union ".
		"		select 6 ".
		"		from sysibm.sysdummy1 ".
		"		union ".
		"		select 7 ".
		"		from sysibm.sysdummy1 ".
		"		union ".
		"		select 8 ".
		"		from sysibm.sysdummy1; ".
		"	open c1; end"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($cur,"call testproc()"));
	assertEqInt(sqlrcur_rowCount($cur),8);
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# temporary tables
	echo("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery($cur,"drop table session.temptable");
	assertTrue(sqlrcur_sendQuery($cur,
		"declare global temporary table session.temptable (".
		"	col1 int ".
		") not logged"));
	assertTrue(sqlrcur_sendQuery(
			$cur,"insert into session.temptable values (1)"));
	assertTrue(sqlrcur_sendQuery(
			$cur,"select count(*) from session.temptable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcon_endSession($con);
	echo("\n");
	assertFalse(sqlrcur_sendQuery(
			$cur,"select count(*) from session.temptable"));
	echo("\n");


	# encoded binary data
	echo("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 blob)"));
	$buffer="";
	for ($j=0; $j<256; $j++) {
		$buffer.=chr($j);
	}
	$querystr="insert into testtable values (blob(X'";
	for ($i=0; $i<strlen($buffer); $i++) {
		$querystr.=sprintf("%02x",ord($buffer[$i]));
	}
	$querystr.="'))";
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
		"	(col1 int not null ".
		"	generated always ".
		"	as identity, ".
		"	col2 int, ".
		"	primary key(col1))"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"(col2) values (1)"));
	assertEqInt(sqlrcon_getLastInsertId($con),1);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# database is schema
	echo("DATABASE IS SCHEMA: \n");
	assertTrue(sqlrcon_getDatabaseIsSchema($con));
	echo("\n");


	# catalog list
	echo("CATALOG LIST: \n");
	assertTrue(sqlrcur_getCatalogList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	assertEqInt(sqlrcur_rowCount($cur),0);
	echo("\n");


	# schema list
	echo("SCHEMA LIST: \n");
	assertTrue(sqlrcur_getSchemaList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	$found=0;
	for ($i=0; $i<sqlrcur_rowCount($cur); $i++) {
		if (!strcmp(sqlrcur_getField($cur,$i,"Database"),
			"DB2INST1")) {
			$found=1;
			break;
		}
	}
	assertTrue($found);
	echo("\n");


	# table type list
	echo("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList($cur));
	assertEqStr(sqlrcur_getColumnName($cur,0),"table_type");
	$found=0;
	for ($i=0; $i<sqlrcur_rowCount($cur); $i++) {
		if (!strcmp(sqlrcur_getField($cur,$i,"table_type"),
			"TABLE")) {
			$found=1;
			break;
		}
	}
	assertTrue($found);
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
	$counter=0;
	for ($i=0; $i<sqlrcur_rowCount($cur); $i++) {
		$name=sqlrcur_getField($cur,$i,"Tables_in_xxx");
		if (!strcmp($name,"TESTTABLE1") || !strcmp($name,"TESTTABLE2") ||
			!strcmp($name,"TESTTABLE3") ||
			!strcmp($name,"TESTTABLE4")) {
			$counter++;
		}
	}
	assertEqInt($counter,4);
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
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"INTEGER");
	assertTrue(sqlrcur_getTypeInfoList($cur,"char"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"254");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"varchar"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"32672");
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
		"	testsmallint smallint, ".
		"	testint integer, ".
		"	testbigint bigint, ".
		"	testdecimal decimal(10,2), ".
		"	testreal real, ".
		"	testdouble double, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testdate date, ".
		"	testtime time, ".
		"	testtimestamp timestamp, ".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcon_commit($con));
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
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),"TESTSMALLINT");
	assertEqStr(sqlrcur_getField($cur,1,"column_name"),"TESTINT");
	assertEqStr(sqlrcur_getField($cur,2,"column_name"),"TESTBIGINT");
	assertEqStr(sqlrcur_getField($cur,3,"column_name"),"TESTDECIMAL");
	assertEqStr(sqlrcur_getField($cur,4,"column_name"),"TESTREAL");
	assertEqStr(sqlrcur_getField($cur,5,"column_name"),"TESTDOUBLE");
	assertEqStr(sqlrcur_getField($cur,6,"column_name"),"TESTCHAR");
	assertEqStr(sqlrcur_getField($cur,7,"column_name"),"TESTVARCHAR");
	assertEqStr(sqlrcur_getField($cur,8,"column_name"),"TESTDATE");
	assertEqStr(sqlrcur_getField($cur,9,"column_name"),"TESTTIME");
	assertEqStr(sqlrcur_getField($cur,10,"column_name"),
							"TESTTIMESTAMP");
	assertEqStr(sqlrcur_getField($cur,11,"column_name"),"TESTCLOB");
	assertEqStr(sqlrcur_getField($cur,12,"column_name"),"TESTBLOB");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"SMALLINT");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"BIGINT");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"DECIMAL");
	assertEqStr(sqlrcur_getField($cur,4,"data_type"),"REAL");
	assertEqStr(sqlrcur_getField($cur,5,"data_type"),"DOUBLE");
	assertEqStr(sqlrcur_getField($cur,6,"data_type"),"CHARACTER");
	assertEqStr(sqlrcur_getField($cur,7,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,8,"data_type"),"DATE");
	assertEqStr(sqlrcur_getField($cur,9,"data_type"),"TIME");
	assertEqStr(sqlrcur_getField($cur,10,"data_type"),"TIMESTAMP");
	assertEqStr(sqlrcur_getField($cur,11,"data_type"),"CLOB");
	assertEqStr(sqlrcur_getField($cur,12,"data_type"),"BLOB");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# column list - auto_increment,
	# primary key
	echo("COLUMN LIST - auto_increment, primary key: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int generated always ".
		"	as identity primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertTrue(strstr(sqlrcur_getField($cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(sqlrcur_getField($cur,0,"column_key"),
		"PRI")!=NULL);
	assertFalse(strstr(sqlrcur_getField($cur,1,"extra"),
		"auto_increment")!=NULL);
	assertFalse(strstr(sqlrcur_getField($cur,1,"column_key"),
		"PRI")!=NULL);
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int not null ".
		"	primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertFalse(strstr(sqlrcur_getField($cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(sqlrcur_getField($cur,0,"column_key"),
		"PRI")!=NULL);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# primary keys list
	echo("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int not null ".
		"	primary key, ".
		"	col2 int)"));
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
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"table"),"TESTTABLE"));
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"column_name"),"COL1"));
	$kn=sqlrcur_getField($cur,0,"key_name");
	assertTrue(!(!$kn || !$kn[0]));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	assertTrue(sqlrcon_commit($con));
	echo("\n");


	# key and index list
	echo("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int not null ".
		"	primary key, ".
		"	col2 int)"));
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
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"table"),"TESTTABLE"));
	assertEqStr(sqlrcur_getField($cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getField($cur,0,"column_name"),"COL1"));
	assertEqStr(sqlrcur_getField($cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getField($cur,0,"index_type"),"3");
	$kn=sqlrcur_getField($cur,0,"key_name");
	assertTrue(!(!$kn || !$kn[0]));
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
		"	in in1 integer, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) language sql begin end"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc2(".
		"	in in1 integer, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) language sql begin end"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc3(".
		"	in in1 integer, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) language sql begin end"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc4(".
		"	in in1 integer, ".
		"	in in2 char(20), ".
		"	in in3 varchar(20), ".
		"	in in4 date) language sql begin end"));
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_getProcedureList($cur,NULL));
	$counter=0;
	for ($i=0; $i<sqlrcur_rowCount($cur); $i++) {
		$name=sqlrcur_getField($cur,$i,"routine_name");
		if (!strcmp($name,"TESTPROC1") || !strcmp($name,"TESTPROC2") ||
			!strcmp($name,"TESTPROC3") || !strcmp($name,"TESTPROC4")) {
			$counter++;
		}
	}
	assertEqInt($counter,4);
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
	assertEqStr(sqlrcur_getField($cur,0,"parameter_name"),"IN1");
	assertEqStr(sqlrcur_getField($cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getField($cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_name"),"IN2");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"CHARACTER");
	assertEqStr(sqlrcur_getField($cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_name"),"IN3");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_name"),"IN4");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"DATE");
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
