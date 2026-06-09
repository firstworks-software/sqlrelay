<html><pre><?php
# Copyright (c) David Muse
# See the file COPYING for more information.

	include("./asserts.php");

	# hostname
	$hostname=gethostname();
	$dot=strpos($hostname,'.');
	if ($dot!==false) {
		$hostname=substr($hostname,0,$dot);
	}


	# instantiation
	$con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	$cur=sqlrcur_alloc($con);


	# identify
	echo("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify($con),"oracle");
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
	assertEqStr(sqlrcon_bindFormat($con),":*");
	echo("\n");


	# nextval format
	echo("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat($con),"%s.nextval");
	echo("\n");


	# isolation levels
	echo("ISOLATION LEVELS: \n");
	$isolationlevels=array("READ COMMITTED","SERIALIZABLE");
	foreach ($isolationlevels as $il) {
		# oracle requires the isolation level to
		# be the first query of the transaction
		assertTrue(sqlrcon_commit($con));
		# you can set the isolation level, but to get it, you have to
		# have permisisons to read from sys.v_$session and
		# sys.v_$transaction
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		echo("\n");
	}
	# reset to the default isolation level
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	echo("\n");


	# create testtable
	echo("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testnumber number, ".
		"	testchar char(40), ".
		"	testvarchar varchar2(40), ".
		"	testdate date, ".
		"	testlong long, ".
		"	testclob clob, ".
		"	testblob blob)"));
	echo("\n");


	# insert
	echo("INSERT: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	'01-JAN-2001', ".
		"	'testlong1', ".
		"	'testclob1', ".
		"	empty_blob())"));
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
		"	:var1, ".
		"	:var2, ".
		"	:var3, ".
		"	:var4, ".
		"	:var5, ".
		"	:var6, ".
		"	:var7)");
	assertEqInt(sqlrcur_countBindVariables($cur),7);
	sqlrcur_inputBind($cur,"1",2);
	sqlrcur_inputBind($cur,"2","testchar2");
	sqlrcur_inputBind($cur,"3","testvarchar2");
	sqlrcur_inputBindDate($cur,"4",2002,1,1,0,0,0,0,null,0);
	sqlrcur_inputBind($cur,"5","testlong2");
	sqlrcur_inputBindClob($cur,"6","testclob2",strlen("testclob2"));
	sqlrcur_inputBindBlob($cur,"7","testblob2",strlen("testblob2"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",3);
	sqlrcur_inputBind($cur,"2","testchar3");
	sqlrcur_inputBind($cur,"3","testvarchar3");
	sqlrcur_inputBindDate($cur,"4",2003,1,1,0,0,0,0,null,0);
	sqlrcur_inputBind($cur,"5","testlong3");
	sqlrcur_inputBindClob($cur,"6","testclob3",strlen("testclob3"));
	sqlrcur_inputBindBlob($cur,"7","testblob3",strlen("testblob3"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by position
	echo("ARRAY OF INPUT BINDS BY POSITION: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("1","2","3","4","5");
	$bindvals=array("4","testchar4","testvarchar4","01-JAN-2004","testlong4");
	sqlrcur_inputBinds($cur,$bindvars,$bindvals);
	sqlrcur_inputBindClob($cur,"6","testclob4",strlen("testclob4"));
	sqlrcur_inputBindBlob($cur,"7","testblob4",strlen("testblob4"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# input bind by position with validation
	echo("INPUT BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",5);
	sqlrcur_inputBind($cur,"2","testchar5");
	sqlrcur_inputBind($cur,"3","testvarchar5");
	sqlrcur_inputBindDate($cur,"4",2005,1,1,0,0,0,0,null,0);
	sqlrcur_inputBind($cur,"5","testlong5");
	sqlrcur_inputBindClob($cur,"6","testclob5",strlen("testclob5"));
	sqlrcur_inputBindBlob($cur,"7","testblob5",strlen("testblob5"));
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);


	# input bind by name
	echo("INPUT BIND BY NAME: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",6);
	sqlrcur_inputBind($cur,"var2","testchar6");
	sqlrcur_inputBind($cur,"var3","testvarchar6");
	sqlrcur_inputBindDate($cur,"var4",2006,1,1,0,0,0,0,null,0);
	sqlrcur_inputBind($cur,"var5","testlong6");
	sqlrcur_inputBindClob($cur,"var6","testclob6",strlen("testclob6"));
	sqlrcur_inputBindBlob($cur,"var7","testblob6",strlen("testblob6"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by name
	echo("ARRAY OF INPUT BINDS BY NAME: \n");
	sqlrcur_clearBinds($cur);
	$arraybindvars=array("var1","var2","var3","var4","var5");
	$arraybindvals=array("7","testchar7","testvarchar7",
				"01-JAN-2007","testlong7");
	sqlrcur_inputBinds($cur,$arraybindvars,$arraybindvals);
	sqlrcur_inputBindClob($cur,"var6","testclob7",strlen("testclob7"));
	sqlrcur_inputBindBlob($cur,"var7","testblob7",strlen("testblob7"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# input bind by name with validation
	echo("INPUT BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",8);
	sqlrcur_inputBind($cur,"var2","testchar8");
	sqlrcur_inputBind($cur,"var3","testvarchar8");
	sqlrcur_inputBindDate($cur,"var4",2008,1,1,0,0,0,0,null,0);
	sqlrcur_inputBind($cur,"var5","testlong8");
	sqlrcur_inputBindClob($cur,"var6","testclob8",strlen("testclob8"));
	sqlrcur_inputBindBlob($cur,"var7","testblob8",strlen("testblob8"));
	sqlrcur_inputBind($cur,"var9","junkvalue");
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
		"	testnumber"));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount($cur),7);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"TESTNUMBER");
	assertEqStr(sqlrcur_getColumnName($cur,1),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,2),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,3),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName($cur,4),"TESTLONG");
	assertEqStr(sqlrcur_getColumnName($cur,5),"TESTCLOB");
	assertEqStr(sqlrcur_getColumnName($cur,6),"TESTBLOB");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"TESTNUMBER");
	assertEqStr($cols[1],"TESTCHAR");
	assertEqStr($cols[2],"TESTVARCHAR");
	assertEqStr($cols[3],"TESTDATE");
	assertEqStr($cols[4],"TESTLONG");
	assertEqStr($cols[5],"TESTCLOB");
	assertEqStr($cols[6],"TESTBLOB");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnType($cur,0),"NUMBER");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTNUMBER"),"NUMBER");
	assertEqStr(sqlrcur_getColumnType($cur,1),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTCHAR"),"CHAR");
	assertEqStr(sqlrcur_getColumnType($cur,2),"VARCHAR2");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTVARCHAR"),"VARCHAR2");
	assertEqStr(sqlrcur_getColumnType($cur,3),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTDATE"),"DATE");
	assertEqStr(sqlrcur_getColumnType($cur,4),"LONG");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTLONG"),"LONG");
	assertEqStr(sqlrcur_getColumnType($cur,5),"CLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTCLOB"),"CLOB");
	assertEqStr(sqlrcur_getColumnType($cur,6),"BLOB");
	assertEqStr(sqlrcur_getColumnType($cur,"TESTBLOB"),"BLOB");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLength($cur,0),22);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTNUMBER"),22);
	assertEqInt(sqlrcur_getColumnLength($cur,1),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,2),40);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTVARCHAR"),40);
	assertEqInt(sqlrcur_getColumnLength($cur,3),7);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTDATE"),7);
	assertEqInt(sqlrcur_getColumnLength($cur,4),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTLONG"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,5),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTCLOB"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,6),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"TESTBLOB"),0);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongest($cur,0),1);
	assertEqInt(sqlrcur_getLongest($cur,"TESTNUMBER"),1);
	assertEqInt(sqlrcur_getLongest($cur,1),40);
	assertEqInt(sqlrcur_getLongest($cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getLongest($cur,2),12);
	assertEqInt(sqlrcur_getLongest($cur,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getLongest($cur,3),9);
	assertEqInt(sqlrcur_getLongest($cur,"TESTDATE"),9);
	assertEqInt(sqlrcur_getLongest($cur,4),9);
	assertEqInt(sqlrcur_getLongest($cur,"TESTLONG"),9);
	assertEqInt(sqlrcur_getLongest($cur,5),9);
	assertEqInt(sqlrcur_getLongest($cur,"TESTCLOB"),9);
	assertEqInt(sqlrcur_getLongest($cur,6),9);
	assertEqInt(sqlrcur_getLongest($cur,"TESTBLOB"),9);
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
	assertEqStr(sqlrcur_getField($cur,0,1),
			"testchar1                               ");
	assertEqStr(sqlrcur_getField($cur,0,2),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,3),"01-JAN-01");
	assertEqStr(sqlrcur_getField($cur,0,4),"testlong1");
	assertEqStr(sqlrcur_getField($cur,0,5),"testclob1");
	assertEqStr(sqlrcur_getField($cur,0,6),"");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,7,1),
			"testchar8                               ");
	assertEqStr(sqlrcur_getField($cur,7,2),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,3),"01-JAN-08");
	assertEqStr(sqlrcur_getField($cur,7,4),"testlong8");
	assertEqStr(sqlrcur_getField($cur,7,5),"testclob8");
	assertEqStr(sqlrcur_getField($cur,7,6),"testblob8");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,1),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,2),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,3),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,4),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,5),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,6),0);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,1),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,2),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,3),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,4),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,5),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,6),9);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getField($cur,0,"TESTNUMBER"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTCHAR"),
			"testchar1                               ");
	assertEqStr(sqlrcur_getField($cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTDATE"),"01-JAN-01");
	assertEqStr(sqlrcur_getField($cur,0,"TESTLONG"),"testlong1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTCLOB"),"testclob1");
	assertEqStr(sqlrcur_getField($cur,0,"TESTBLOB"),"");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,"TESTNUMBER"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTCHAR"),
			"testchar8                               ");
	assertEqStr(sqlrcur_getField($cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTDATE"),"01-JAN-08");
	assertEqStr(sqlrcur_getField($cur,7,"TESTLONG"),"testlong8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTCLOB"),"testclob8");
	assertEqStr(sqlrcur_getField($cur,7,"TESTBLOB"),"testblob8");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTNUMBER"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTDATE"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTLONG"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTCLOB"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTBLOB"),0);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTNUMBER"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTDATE"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTLONG"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTCLOB"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"TESTBLOB"),9);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqStr($fields[0],"1");
	assertEqStr($fields[1],"testchar1                               ");
	assertEqStr($fields[2],"testvarchar1");
	assertEqStr($fields[3],"01-JAN-01");
	assertEqStr($fields[4],"testlong1");
	assertEqStr($fields[5],"testclob1");
	assertEqStr($fields[6],"");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqInt($fieldlens[0],1);
	assertEqInt($fieldlens[1],40);
	assertEqInt($fieldlens[2],12);
	assertEqInt($fieldlens[3],9);
	assertEqInt($fieldlens[4],9);
	assertEqInt($fieldlens[5],9);
	assertEqInt($fieldlens[6],0);
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
		"	testnumber"));
	assertEqInt(sqlrcur_getResultSetBufferSize($cur),2);
	echo("\n");
	assertEqInt(sqlrcur_firstRowIndex($cur),0);
	assertFalse(sqlrcur_endOfResultSet($cur));
	assertEqInt(sqlrcur_rowCount($cur),2);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,1,0),"2");
	assertEqStr(sqlrcur_getField($cur,2,0),"3");
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
		"	testnumber"));
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
		"	testnumber"));
	assertEqStr(sqlrcur_getColumnName($cur,0),"TESTNUMBER");
	assertEqInt(sqlrcur_getColumnLength($cur,0),22);
	assertEqStr(sqlrcur_getColumnType($cur,0),"NUMBER");
	echo("\n");


	# suspended session
	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber"));
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
		"	testnumber"));
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
		"	testnumber"));
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
		"	testnumber"));
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
		"	testnumber"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqStr($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount($cur),7);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"TESTNUMBER");
	assertEqStr(sqlrcur_getColumnName($cur,1),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,2),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName($cur,3),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName($cur,4),"TESTLONG");
	assertEqStr(sqlrcur_getColumnName($cur,5),"TESTCLOB");
	assertEqStr(sqlrcur_getColumnName($cur,6),"TESTBLOB");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"TESTNUMBER");
	assertEqStr($cols[1],"TESTCHAR");
	assertEqStr($cols[2],"TESTVARCHAR");
	assertEqStr($cols[3],"TESTDATE");
	assertEqStr($cols[4],"TESTLONG");
	assertEqStr($cols[5],"TESTCLOB");
	assertEqStr($cols[6],"TESTBLOB");
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
		"	testnumber"));
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


	# from one cache file to another with result set buffer size
	echo("FROM ONE CACHE FILE TO ANOTHER ".
				"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,8,0),NULL);
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
		"	testnumber"));
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
		"	testnumber"));
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
	echo("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel($con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 integer)"));
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
	assertEqStr(sqlrcon_getTransactionModel($con),"implicit");
	assertFalse(sqlrcon_getAutoCommit($con));
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select \$(var1),'\$(var2)',\$(var3) from dual");
	sqlrcur_substitution($cur,"var1","\$(var11)");
	sqlrcur_substitution($cur,"var2","\$(var21)");
	sqlrcur_substitution($cur,"var3","\$(var31)");
	sqlrcur_substitution($cur,"var11","\$(var111)");
	sqlrcur_substitution($cur,"var21","\$(var211)");
	sqlrcur_substitution($cur,"var31","\$(var311)");
	sqlrcur_substitution($cur,"var111",1);
	sqlrcur_substitution($cur,"var211","hello");
	sqlrcur_substitution($cur,"var311",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	echo("\n");


	# array substitutions
	echo("ARRAY SUBSTITUTIONS: \n");
	$subvars=array("var1","var2","var3");
	sqlrcur_prepareQuery($cur,"select \$(var1),\$(var2),\$(var3) from dual");
	$subvallongs=array(1,2,3);
	sqlrcur_substitutions($cur,$subvars,$subvallongs);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"2");
	assertEqStr(sqlrcur_getField($cur,0,2),"3");
	echo("\n");
	sqlrcur_prepareQuery($cur,
			"select '\$(var1)','\$(var2)','\$(var3)' from dual");
	$subvalstrings=array("hi","hello","bye");
	sqlrcur_substitutions($cur,$subvars,$subvalstrings);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"hi");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"bye");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select \$(var1),\$(var2),\$(var3) from dual");
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
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL from dual"));
	assertEqStr(sqlrcur_getField($cur,0,0),NULL);
	assertEqStr(sqlrcur_getField($cur,0,1),"1");
	assertEqStr(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL from dual"));
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
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	:var1, ".
		"	:var2, ".
		"	:var3, ".
		"	:var4)");
	sqlrcur_inputBindClob($cur,"var1","",strlen(""));
	sqlrcur_inputBindClob($cur,"var2",NULL,0);
	sqlrcur_inputBindBlob($cur,"var3","",strlen(""));
	sqlrcur_inputBindBlob($cur,"var4",NULL,0);
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
		"	testclob clob, ".
		"	testblob blob)");
	sqlrcur_prepareQuery($cur,
			"insert into testtable values (:clobval,:blobval)");
	$largebuffer=str_repeat("C",8192);
	sqlrcur_inputBindClob($cur,"clobval",$largebuffer,strlen($largebuffer));
	sqlrcur_inputBindBlob($cur,"blobval",$largebuffer,strlen($largebuffer));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTCLOB"),8192);
	assertEqStr(sqlrcur_getField($cur,0,"TESTCLOB"),$largebuffer);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"TESTBLOB"),8192);
	assertEqStrLen(sqlrcur_getField($cur,0,"TESTBLOB"),$largebuffer,8192);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# output bind by position
	echo("OUTPUT BIND BY POSITION: \n");
	sqlrcur_getNullsAsNulls($cur);
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	:numvar:=1; ".
		"	:stringvar:='hello'; ".
		"	:floatvar:=2.5; ".
		"	:datevar:='03-FEB-2001'; ".
		"	:nullvar:=null; ".
		"end;");
	assertEqInt(sqlrcur_countBindVariables($cur),5);
	sqlrcur_defineOutputBindInteger($cur,"1");
	sqlrcur_defineOutputBindString($cur,"2",10);
	sqlrcur_defineOutputBindDouble($cur,"3");
	sqlrcur_defineOutputBindDate($cur,"4");
	sqlrcur_defineOutputBindString($cur,"5",10);
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
	echo("\n");


	# output bind by name
	echo("OUTPUT BIND BY NAME: \n");
	sqlrcur_getNullsAsNulls($cur);
	sqlrcur_clearBinds($cur);
	sqlrcur_defineOutputBindInteger($cur,"numvar");
	sqlrcur_defineOutputBindString($cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble($cur,"floatvar");
	sqlrcur_defineOutputBindDate($cur,"datevar");
	sqlrcur_defineOutputBindString($cur,"nullvar",10);
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"numvar");
	$stringvar=sqlrcur_getOutputBindString($cur,"stringvar");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"floatvar");
	$year=sqlrcur_getOutputBindDateYear($cur,"datevar");
	$month=sqlrcur_getOutputBindDateMonth($cur,"datevar");
	$day=sqlrcur_getOutputBindDateDay($cur,"datevar");
	$hour=sqlrcur_getOutputBindDateHour($cur,"datevar");
	$minute=sqlrcur_getOutputBindDateMinute($cur,"datevar");
	$second=sqlrcur_getOutputBindDateSecond($cur,"datevar");
	$microsecond=sqlrcur_getOutputBindDateMicroSecond($cur,"datevar");
	$tz=sqlrcur_getOutputBindDateTz($cur,"datevar");
	$isnegative=sqlrcur_getOutputBindDateIsNegative($cur,"datevar");
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
	$nullvar=sqlrcur_getOutputBindString($cur,"nullvar");
	assertEqStr($nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	echo("\n");


	# output bind by name with validation
	echo("OUTPUT BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_getNullsAsNulls($cur);
	sqlrcur_clearBinds($cur);
	sqlrcur_defineOutputBindInteger($cur,"numvar");
	sqlrcur_defineOutputBindString($cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble($cur,"floatvar");
	sqlrcur_defineOutputBindDate($cur,"datevar");
	sqlrcur_defineOutputBindString($cur,"nullvar",10);
	sqlrcur_defineOutputBindString($cur,"dummyvar",10);
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"numvar");
	$stringvar=sqlrcur_getOutputBindString($cur,"stringvar");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"floatvar");
	$year=sqlrcur_getOutputBindDateYear($cur,"datevar");
	$month=sqlrcur_getOutputBindDateMonth($cur,"datevar");
	$day=sqlrcur_getOutputBindDateDay($cur,"datevar");
	$hour=sqlrcur_getOutputBindDateHour($cur,"datevar");
	$minute=sqlrcur_getOutputBindDateMinute($cur,"datevar");
	$second=sqlrcur_getOutputBindDateSecond($cur,"datevar");
	$microsecond=sqlrcur_getOutputBindDateMicroSecond($cur,"datevar");
	$tz=sqlrcur_getOutputBindDateTz($cur,"datevar");
	$isnegative=sqlrcur_getOutputBindDateIsNegative($cur,"datevar");
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
	$nullvar=sqlrcur_getOutputBindString($cur,"nullvar");
	assertEqStr($nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	echo("\n");


	# lob output bind
	echo("LOB OUTPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testclob clob, ".
		"	testblob blob)"));
	sqlrcur_prepareQuery($cur,
			"insert into testtable values ('hello',:var1)");
	sqlrcur_inputBindBlob($cur,"var1","hello",strlen("hello"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	select testclob into :clobvar from testtable; ".
		"	select testblob into :blobvar from testtable; ".
		"end;");
	sqlrcur_defineOutputBindClob($cur,"clobvar");
	sqlrcur_defineOutputBindBlob($cur,"blobvar");
	assertTrue(sqlrcur_executeQuery($cur));
	$clobvar=sqlrcur_getOutputBindClob($cur,"clobvar");
	$clobvarlength=sqlrcur_getOutputBindLength($cur,"clobvar");
	$blobvar=sqlrcur_getOutputBindBlob($cur,"blobvar");
	$blobvarlength=sqlrcur_getOutputBindLength($cur,"blobvar");
	assertEqStrLen($clobvar,"hello",5);
	assertEqInt($clobvarlength,5);
	assertEqStrLen($blobvar,"hello",5);
	assertEqInt($blobvarlength,5);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# long output bind
	echo("LONG OUTPUT BIND: \n");
	$largebuffer=str_repeat("C",8192);
	$query="begin :bindval:='".$largebuffer."'; end;";
	sqlrcur_prepareQuery($cur,$query);
	sqlrcur_defineOutputBindString($cur,"bindval",8192);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindLength($cur,"bindval"),8192);
	assertEqStr(sqlrcur_getOutputBindString($cur,"bindval"),$largebuffer);
	echo("\n");


	# negative input bind
	echo("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,"create table testtable (testval number)");
	sqlrcur_prepareQuery($cur,"insert into testtable values (:testval)");
	sqlrcur_inputBind($cur,"testval",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable");
	assertEqStr(sqlrcur_getField($cur,0,"TESTVAL"),"-1");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# bind validation
	echo("BIND VALIDATION: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 varchar2(20), ".
		"	col2 varchar2(20), ".
		"	col3 varchar2(20))");
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
	sqlrcur_substitution($cur,"var1",":var1");
	assertTrue(sqlrcur_validBind($cur,"var1"));
	assertFalse(sqlrcur_validBind($cur,"var2"));
	assertFalse(sqlrcur_validBind($cur,"var3"));
	assertFalse(sqlrcur_validBind($cur,"var4"));
	echo("\n");
	sqlrcur_substitution($cur,"var2",":var2");
	assertTrue(sqlrcur_validBind($cur,"var1"));
	assertTrue(sqlrcur_validBind($cur,"var2"));
	assertFalse(sqlrcur_validBind($cur,"var3"));
	assertFalse(sqlrcur_validBind($cur,"var4"));
	echo("\n");
	sqlrcur_substitution($cur,"var3",":var3");
	assertTrue(sqlrcur_validBind($cur,"var1"));
	assertTrue(sqlrcur_validBind($cur,"var2"));
	assertTrue(sqlrcur_validBind($cur,"var3"));
	assertFalse(sqlrcur_validBind($cur,"var4"));
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# rebinding
	echo("REBINDING: \n");
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	:out:= :in; ".
		"end;");
	sqlrcur_inputBind($cur,"in",1);
	sqlrcur_defineOutputBindInteger($cur,"out");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out"),1);
	sqlrcur_inputBind($cur,"in",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out"),2);
	sqlrcur_inputBind($cur,"in",3);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out"),3);
	echo("\n");


	# reexecute
	echo("REEXECUTE: \n");
	sqlrcur_prepareQuery($cur,"select 1 from dual");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	sqlrcur_prepareQuery($cur,"select :var from dual");
	sqlrcur_inputBind($cur,"var",1);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	echo("\n");
	sqlrcur_inputBind($cur,"var",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_rowCount($cur),1);
	assertEqStr(sqlrcur_getField($cur,0,0),"2");
	echo("\n");


	# stored procedure returning no value
	echo("STORED PROCEDURE RETURNING NO VALUE: \n");
	sqlrcur_sendQuery($cur,"drop function testproc");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create or replace ".
		"procedure testproc(".
		"	in1 in number, ".
		"	in2 in number, ".
		"	in3 in varchar2) ".
		"is ".
		"begin ".
		"	return; ".
		"end;"));
	sqlrcur_prepareQuery($cur,"begin testproc(:in1,:in2,:in3); end;");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_inputBind($cur,"in2",2.5,2,1);
	sqlrcur_inputBind($cur,"in3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc"));
	echo("\n");


	# stored procedure returning single value
	echo("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery($cur,"drop function testproc");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create or replace ".
		"function testproc(".
		"	in1 in number, ".
		"	in2 in number, ".
		"	in3 in varchar2) ".
		"	return number ".
		"is ".
		"begin ".
		"	return in1; ".
		"end;"));
	sqlrcur_prepareQuery($cur,"select testproc(:in1,:in2,:in3) from dual");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_inputBind($cur,"in2",2.5,2,1);
	sqlrcur_inputBind($cur,"in3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	:out1:=testproc(:in1,:in2,:in3); ".
		"end;");
	sqlrcur_inputBind($cur,"in1",1);
	sqlrcur_inputBind($cur,"in2",2.5,2,1);
	sqlrcur_inputBind($cur,"in3","hello");
	sqlrcur_defineOutputBindInteger($cur,"out1");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqInt(sqlrcur_getOutputBindInteger($cur,"out1"),1);
	assertTrue(sqlrcur_sendQuery($cur,"drop function testproc"));
	echo("\n");


	# stored procedure returning multiple values
	echo("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery($cur,"drop function testproc");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create or replace ".
		"procedure testproc(".
		"	in1 in number, ".
		"	in2 in number, ".
		"	in3 in varchar2, ".
		"	out1 out number, ".
		"	out2 out number, ".
		"	out3 out varchar2) ".
		"is ".
		"begin ".
		"	out1:=in1; ".
		"	out2:=in2; ".
		"	out3:=in3; ".
		"end;"));
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	testproc(:in1,:in2,:in3,:out1,:out2,:out3); ".
		"end;");
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
	echo("STORED PROCEDURE RETURNING RESULT SET: \n");
	sqlrcur_sendQuery($cur,"drop package types");
	sqlrcur_sendQuery($cur,"drop function testproc");
	sqlrcur_sendQuery($cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery($cur,
		"create or replace package types is ".
		"	type cursorType is ref cursor; ".
		"end;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create or replace ".
		"function testproc(value in number) ".
		"	return types.cursortype ".
		"is ".
		"	l_cursor    types.cursorType; ".
		"begin ".
		"	open l_cursor for ".
		"		select ".
		"			* ".
		"		from ".
		"			( ".
		"			select 1 as testnumber from dual ".
		"			union ".
		"			select 2 as testnumber from dual ".
		"			union ".
		"			select 3 as testnumber from dual ".
		"			union ".
		"			select 4 as testnumber from dual ".
		"			union ".
		"			select 5 as testnumber from dual ".
		"			union ".
		"			select 6 as testnumber from dual ".
		"			union ".
		"			select 7 as testnumber from dual ".
		"			union ".
		"			select 8 as testnumber from dual ".
		"			) ".
		"		where ".
		"			testnumber>value; ".
		"	return l_cursor; ".
		"end;"));
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	:curs1:=testproc(5); ".
		"	:curs2:=testproc(0); ".
		"end;");
	sqlrcur_defineOutputBindCursor($cur,"curs1");
	sqlrcur_defineOutputBindCursor($cur,"curs2");
	assertTrue(sqlrcur_executeQuery($cur));
	$bindcur1=sqlrcur_getOutputBindCursor($cur,"curs1");
	assertTrue(sqlrcur_fetchFromBindCursor($bindcur1));
	assertEqStr(sqlrcur_getField($bindcur1,0,0),"6");
	assertEqStr(sqlrcur_getField($bindcur1,1,0),"7");
	assertEqStr(sqlrcur_getField($bindcur1,2,0),"8");
	$bindcur2=sqlrcur_getOutputBindCursor($cur,"curs2");
	assertTrue(sqlrcur_fetchFromBindCursor($bindcur2));
	assertEqStr(sqlrcur_getField($bindcur2,0,0),"1");
	assertEqStr(sqlrcur_getField($bindcur2,1,0),"2");
	assertEqStr(sqlrcur_getField($bindcur2,2,0),"3");
	assertTrue(sqlrcur_sendQuery($cur,"drop function testproc"));
	assertTrue(sqlrcur_sendQuery($cur,"drop package types"));
	echo("\n");


	# temporary tables
	echo("TEMPORARY TABLES: \n");
	sqlrcur_prepareQuery($cur,"drop table \$(HOSTNAME)_temptabledelete");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	sqlrcur_executeQuery($cur);
	sqlrcur_prepareQuery($cur,
		"create global temporary table \$(HOSTNAME)_temptabledelete ( ".
		"	col1 number ".
		") on commit delete rows");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	sqlrcur_executeQuery($cur);
	sqlrcur_prepareQuery($cur,
			"insert into \$(HOSTNAME)_temptabledelete values (1)");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_prepareQuery($cur,
			"select count(*) from \$(HOSTNAME)_temptabledelete");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,
			"select count(*) from \$(HOSTNAME)_temptabledelete");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"0");
	sqlrcur_prepareQuery($cur,"drop table \$(HOSTNAME)_temptabledelete");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	sqlrcur_executeQuery($cur);
	echo("\n");
	sqlrcur_prepareQuery($cur,
			"truncate table \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	sqlrcur_executeQuery($cur);
	sqlrcur_prepareQuery($cur,"drop table \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	sqlrcur_executeQuery($cur);
	sqlrcur_prepareQuery($cur,
		"create global temporary table \$(HOSTNAME)_temptablepreserve (".
		"	col1 number ".
		") on commit preserve rows");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	sqlrcur_executeQuery($cur);
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	\$(HOSTNAME)_temptablepreserve ".
		"values (1)");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_prepareQuery($cur,
			"select count(*) from \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertTrue(sqlrcon_commit($con));
	sqlrcur_prepareQuery($cur,
			"select count(*) from \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcon_endSession($con);
	echo("\n");
	sqlrcur_prepareQuery($cur,
			"select count(*) from \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"0");
	sqlrcur_prepareQuery($cur,
			"truncate table \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	sleep(2);
	sqlrcur_prepareQuery($cur,"drop table \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_prepareQuery($cur,
			"select count(*) from \$(HOSTNAME)_temptablepreserve");
	sqlrcur_substitution($cur,"HOSTNAME",$hostname);
	assertFalse(sqlrcur_executeQuery($cur));
	echo("\n");


	# encoded binary data
	echo("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (col1 blob)"));
	$buffer="";
	for ($i=0; $i<256; $i++) {
		$buffer.=chr($i);
	}
	$querystr="insert into testtable values ('";
	for ($i=0; $i<strlen($buffer); $i++) {
		$querystr.=sprintf("%02x",ord($buffer[$i]));
	}
	$querystr.="')";
	assertTrue(sqlrcur_sendQuery($cur,$querystr));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),strlen($buffer));
	assertEqInt(strcmp(sqlrcur_getField($cur,0,0),$buffer),0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# quotes
	echo("QUOTES: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(
			$cur,"create table testtable (col1 varchar2(4))"));
	assertTrue(sqlrcur_sendQuery(
			$cur,"insert into testtable values ('''''')"));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),2);
	assertEqInt(strcmp(sqlrcur_getField($cur,0,0),"''"),0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# last insert id
	# oracle doesn't support auto-increment


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
	assertInResultSet($cur,"Database",strtoupper($hostname));
	echo("\n");


	# table type list
	echo("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList($cur));
	assertEqStr(sqlrcur_getColumnName($cur,0),"table_type");
	assertEqStr(sqlrcur_getField($cur,0,"table_type"),"SYNONYM");
	assertEqStr(sqlrcur_getField($cur,1,"table_type"),"TABLE");
	assertEqStr(sqlrcur_getField($cur,2,"table_type"),"VIEW");
	echo("\n");


	# table list
	echo("TABLE LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable1");
	sqlrcur_sendQuery($cur,"drop table testtable2");
	sqlrcur_sendQuery($cur,"drop table testtable3");
	sqlrcur_sendQuery($cur,"drop table testtable4");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable1 (".
		"	testnumber number, ".
		"	testchar char(40), ".
		"	testvarchar varchar2(40), ".
		"	testdate date, ".
		"	testlong long, ".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable2 (".
		"	testnumber number, ".
		"	testchar char(40), ".
		"	testvarchar varchar2(40), ".
		"	testdate date, ".
		"	testlong long, ".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable3 (".
		"	testnumber number, ".
		"	testchar char(40), ".
		"	testvarchar varchar2(40), ".
		"	testdate date, ".
		"	testlong long, ".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable4 (".
		"	testnumber number, ".
		"	testchar char(40), ".
		"	testvarchar varchar2(40), ".
		"	testdate date, ".
		"	testlong long, ".
		"	testclob clob, ".
		"	testblob blob)"));
	assertTrue(sqlrcur_getTableList($cur,NULL));
	assertInResultSet($cur,"Tables_in_xxx","TESTTABLE1");
	assertInResultSet($cur,"Tables_in_xxx","TESTTABLE2");
	assertInResultSet($cur,"Tables_in_xxx","TESTTABLE3");
	assertInResultSet($cur,"Tables_in_xxx","TESTTABLE4");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable4"));
	echo("\n");


	# type info list
	echo("TYPE INFO LIST: \n");
	assertTrue(sqlrcur_getTypeInfoList($cur,"number"));
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
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"NUMBER");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"-7");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"NUMBER");
	assertTrue(sqlrcur_getTypeInfoList($cur,"char"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"2000");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"varchar2"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"VARCHAR2");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"32767");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"VARCHAR2");
	assertTrue(sqlrcur_getTypeInfoList($cur,"date"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"92");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"7");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"DATE");
	echo("\n");


	# column list
	echo("COLUMN LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testnumber number, ".
		"	testchar char(40), ".
		"	testvarchar varchar2(40), ".
		"	testdate date, ".
		"	testlong long, ".
		"	testclob clob, ".
		"	testblob blob)"));
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
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),"TESTNUMBER");
	assertEqStr(sqlrcur_getField($cur,1,"column_name"),"TESTCHAR");
	assertEqStr(sqlrcur_getField($cur,2,"column_name"),"TESTVARCHAR");
	assertEqStr(sqlrcur_getField($cur,3,"column_name"),"TESTDATE");
	assertEqStr(sqlrcur_getField($cur,4,"column_name"),"TESTLONG");
	assertEqStr(sqlrcur_getField($cur,5,"column_name"),"TESTCLOB");
	assertEqStr(sqlrcur_getField($cur,6,"column_name"),"TESTBLOB");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"NUMBER");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"VARCHAR2");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"DATE");
	assertEqStr(sqlrcur_getField($cur,4,"data_type"),"LONG");
	assertEqStr(sqlrcur_getField($cur,5,"data_type"),"CLOB");
	assertEqStr(sqlrcur_getField($cur,6,"data_type"),"BLOB");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# column list - auto_increment, primary key
	# oracle doesn't support auto_increment
	echo("COLUMN LIST - auto_increment, primary key: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 number primary key, ".
		"	col2 number)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getField($cur,0,"column_key"),"PRI");
	assertEqStr(sqlrcur_getField($cur,1,"column_key"),"");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# primary keys list
	echo("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 number primary key, ".
		"	col2 number)"));
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
	assertEqStr(sqlrcur_getField($cur,0,"table"),"TESTTABLE");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),"COL1");
	$keyname=sqlrcur_getField($cur,0,"key_name");
	assertStartsWith($keyname,"SYS_C");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# key and index list
	echo("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 number primary key, ".
		"	col2 number)"));
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
	assertEqStr(sqlrcur_getField($cur,0,"table"),"TESTTABLE");
	assertEqStr(sqlrcur_getField($cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),"COL1");
	assertEqStr(sqlrcur_getField($cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getField($cur,0,"index_type"),"3");
	$keyname=sqlrcur_getField($cur,0,"key_name");
	assertStartsWith($keyname,"SYS_C");
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# procedure list
	echo("PROCEDURE LIST: \n");
	sqlrcur_sendQuery($cur,"drop procedure testproc1");
	sqlrcur_sendQuery($cur,"drop procedure testproc2");
	sqlrcur_sendQuery($cur,"drop procedure testproc3");
	sqlrcur_sendQuery($cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc1(".
		"	in1 in number, ".
		"	in2 in char, ".
		"	in3 in varchar2, ".
		"	in4 in date) as ".
		"begin ".
		"	null; ".
		"end;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc2(".
		"	in1 in number, ".
		"	in2 in char, ".
		"	in3 in varchar2, ".
		"	in4 in date) as ".
		"begin ".
		"	null; ".
		"end;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc3(".
		"	in1 in number, ".
		"	in2 in char, ".
		"	in3 in varchar2, ".
		"	in4 in date) as ".
		"begin ".
		"	null; ".
		"end;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create procedure testproc4(".
		"	in1 in number, ".
		"	in2 in char, ".
		"	in3 in varchar2, ".
		"	in4 in date) as ".
		"begin ".
		"	null; ".
		"end;"));
	assertTrue(sqlrcur_getProcedureList($cur,NULL));
	assertInResultSet($cur,"routine_name","TESTPROC1");
	assertInResultSet($cur,"routine_name","TESTPROC2");
	assertInResultSet($cur,"routine_name","TESTPROC3");
	assertInResultSet($cur,"routine_name","TESTPROC4");
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
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"NUMBER");
	assertEqStr(sqlrcur_getField($cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_name"),"IN2");
	assertEqStr(sqlrcur_getField($cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_name"),"IN3");
	assertEqStr(sqlrcur_getField($cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"VARCHAR2");
	assertEqStr(sqlrcur_getField($cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_name"),"IN4");
	assertEqStr(sqlrcur_getField($cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"DATE");
	assertEqStr(sqlrcur_getField($cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop procedure testproc4"));
	echo("\n");


	# invalid queries
	echo("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber"));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber"));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber"));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery(
			$cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(
			$cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(
			$cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(
			$cur,"insert into testtable values (1,2,3,4)"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	echo("\n");


	reportTestStatus();

	exit($status);
?></pre></html>
