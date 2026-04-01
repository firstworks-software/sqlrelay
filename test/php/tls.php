<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.
	include("./asserts.php");


	$host="sqlrelay";
	$port=9000;
	$socket="/tmp/test.socket";
	$user=null;
	$password=null;

	$cert="../sqlrelay.conf.d/tls/client.pem";
	$ca="../sqlrelay.conf.d/tls/ca.pem";
	if (strtoupper(substr(PHP_OS,0,3))==='WIN') {
		$cert="..\\sqlrelay.conf.d\\tls\\client.pfx";
		$ca="..\\sqlrelay.conf.d\\tls\\ca.pfx";
	}

	# instantiation
	$con=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$cur=sqlrcur_alloc($con);
	sqlrcon_enableTls($con,null,$cert,null,null,"ca",$ca,0);

	# get database type


	# identify
	echo("IDENTIFY: \n");
	assertEqual(sqlrcon_identify($con),"oracle");
	echo("\n");


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");

	# drop existing table
	sqlrcur_sendQuery($cur,"drop table testtable");


	# create temptable
	echo("CREATE TEMPTABLE: \n");
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
	assertEqual(sqlrcur_affectedRows($cur),1);
	echo("\n");


	# bind by position
	echo("BIND BY POSITION: \n");
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
	assertEqual(sqlrcur_countBindVariables($cur),7);
	sqlrcur_inputBind($cur,"1",2);
	sqlrcur_inputBind($cur,"2","testchar2");
	sqlrcur_inputBind($cur,"3","testvarchar2");
	sqlrcur_inputBind($cur,"4","01-JAN-2002");
	sqlrcur_inputBind($cur,"5","testlong2");
	sqlrcur_inputBindClob($cur,"6","testclob2",9);
	sqlrcur_inputBindBlob($cur,"7","testblob2",9);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",3);
	sqlrcur_inputBind($cur,"2","testchar3");
	sqlrcur_inputBind($cur,"3","testvarchar3");
	sqlrcur_inputBind($cur,"4","01-JAN-2003");
	sqlrcur_inputBind($cur,"5","testlong3");
	sqlrcur_inputBindClob($cur,"6","testclob3",9);
	sqlrcur_inputBindBlob($cur,"7","testblob3",9);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of binds by position
	echo("ARRAY OF BINDS BY POSITION: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("1","2","3","4","5");
	$bindvals=array("4","testchar4","testvarchar4",
			"01-JAN-2004","testlong4");
	sqlrcur_inputBinds($cur,$bindvars,$bindvals);
	sqlrcur_inputBindClob($cur,"6","testclob4",9);
	sqlrcur_inputBindBlob($cur,"7","testblob4",9);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# bind by name
	echo("BIND BY NAME: \n");
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
	sqlrcur_inputBind($cur,"var1",5);
	sqlrcur_inputBind($cur,"var2","testchar5");
	sqlrcur_inputBind($cur,"var3","testvarchar5");
	sqlrcur_inputBind($cur,"var4","01-JAN-2005");
	sqlrcur_inputBind($cur,"var5","testlong5");
	sqlrcur_inputBindClob($cur,"6","testclob5",9);
	sqlrcur_inputBindBlob($cur,"7","testblob5",9);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",6);
	sqlrcur_inputBind($cur,"var2","testchar6");
	sqlrcur_inputBind($cur,"var3","testvarchar6");
	sqlrcur_inputBind($cur,"var4","01-JAN-2006");
	sqlrcur_inputBind($cur,"var5","testlong6");
	sqlrcur_inputBindClob($cur,"6","testclob6",9);
	sqlrcur_inputBindBlob($cur,"7","testblob6",9);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of binds by name
	echo("ARRAY OF BINDS BY NAME: \n");
	sqlrcur_clearBinds($cur);
	$arraybindvars=array("var1","var2","var3","var4","var5");
	$arraybindvals=array("7","testchar7",
			"testvarchar7","01-JAN-2007",
			"testlong7");
	sqlrcur_inputBinds($cur,$arraybindvars,$arraybindvals);
	sqlrcur_inputBindClob($cur,"var6","testclob7",9);
	sqlrcur_inputBindBlob($cur,"var7","testblob7",9);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# bind by name with validation
	echo("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",8);
	sqlrcur_inputBind($cur,"var2","testchar8");
	sqlrcur_inputBind($cur,"var3","testvarchar8");
	sqlrcur_inputBind($cur,"var4","01-JAN-2008");
	sqlrcur_inputBind($cur,"var5","testlong8");
	sqlrcur_inputBindClob($cur,"var6","testclob8",9);
	sqlrcur_inputBindBlob($cur,"var7","testblob8",9);
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
		"	testnumber "));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqual(sqlrcur_colCount($cur),7);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"TESTNUMBER");
	assertEqual(sqlrcur_getColumnName($cur,1),"TESTCHAR");
	assertEqual(sqlrcur_getColumnName($cur,2),"TESTVARCHAR");
	assertEqual(sqlrcur_getColumnName($cur,3),"TESTDATE");
	assertEqual(sqlrcur_getColumnName($cur,4),"TESTLONG");
	assertEqual(sqlrcur_getColumnName($cur,5),"TESTCLOB");
	assertEqual(sqlrcur_getColumnName($cur,6),"TESTBLOB");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"TESTNUMBER");
	assertEqual($cols[1],"TESTCHAR");
	assertEqual($cols[2],"TESTVARCHAR");
	assertEqual($cols[3],"TESTDATE");
	assertEqual($cols[4],"TESTLONG");
	assertEqual($cols[5],"TESTCLOB");
	assertEqual($cols[6],"TESTBLOB");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqual(sqlrcur_getColumnType($cur,0),"NUMBER");
	assertEqual(sqlrcur_getColumnType($cur,"TESTNUMBER"),"NUMBER");
	assertEqual(sqlrcur_getColumnType($cur,1),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,"TESTCHAR"),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,2),"VARCHAR2");
	assertEqual(sqlrcur_getColumnType($cur,"TESTVARCHAR"),"VARCHAR2");
	assertEqual(sqlrcur_getColumnType($cur,3),"DATE");
	assertEqual(sqlrcur_getColumnType($cur,"TESTDATE"),"DATE");
	assertEqual(sqlrcur_getColumnType($cur,4),"LONG");
	assertEqual(sqlrcur_getColumnType($cur,"TESTLONG"),"LONG");
	assertEqual(sqlrcur_getColumnType($cur,5),"CLOB");
	assertEqual(sqlrcur_getColumnType($cur,"TESTCLOB"),"CLOB");
	assertEqual(sqlrcur_getColumnType($cur,6),"BLOB");
	assertEqual(sqlrcur_getColumnType($cur,"TESTBLOB"),"BLOB");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqual(sqlrcur_getColumnLength($cur,0),22);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTNUMBER"),22);
	assertEqual(sqlrcur_getColumnLength($cur,1),40);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTCHAR"),40);
	assertEqual(sqlrcur_getColumnLength($cur,2),40);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTVARCHAR"),40);
	assertEqual(sqlrcur_getColumnLength($cur,3),7);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTDATE"),7);
	assertEqual(sqlrcur_getColumnLength($cur,4),0);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTLONG"),0);
	assertEqual(sqlrcur_getColumnLength($cur,5),0);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTCLOB"),0);
	assertEqual(sqlrcur_getColumnLength($cur,6),0);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTBLOB"),0);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqual(sqlrcur_getLongest($cur,0),1);
	assertEqual(sqlrcur_getLongest($cur,"TESTNUMBER"),1);
	assertEqual(sqlrcur_getLongest($cur,1),40);
	assertEqual(sqlrcur_getLongest($cur,"TESTCHAR"),40);
	assertEqual(sqlrcur_getLongest($cur,2),12);
	assertEqual(sqlrcur_getLongest($cur,"TESTVARCHAR"),12);
	assertEqual(sqlrcur_getLongest($cur,3),9);
	assertEqual(sqlrcur_getLongest($cur,"TESTDATE"),9);
	assertEqual(sqlrcur_getLongest($cur,4),9);
	assertEqual(sqlrcur_getLongest($cur,"TESTLONG"),9);
	assertEqual(sqlrcur_getLongest($cur,5),9);
	assertEqual(sqlrcur_getLongest($cur,"TESTCLOB"),9);
	assertEqual(sqlrcur_getLongest($cur,6),9);
	assertEqual(sqlrcur_getLongest($cur,"TESTBLOB"),9);
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
	assertEqual(sqlrcur_getField($cur,0,1),"testchar1                               ");
	assertEqual(sqlrcur_getField($cur,0,2),"testvarchar1");
	assertEqual(sqlrcur_getField($cur,0,3),"01-JAN-01");
	assertEqual(sqlrcur_getField($cur,0,4),"testlong1");
	assertEqual(sqlrcur_getField($cur,0,5),"testclob1");
	assertEqual(sqlrcur_getField($cur,0,6),"");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,7,1),"testchar8                               ");
	assertEqual(sqlrcur_getField($cur,7,2),"testvarchar8");
	assertEqual(sqlrcur_getField($cur,7,3),"01-JAN-08");
	assertEqual(sqlrcur_getField($cur,7,4),"testlong8");
	assertEqual(sqlrcur_getField($cur,7,5),"testclob8");
	assertEqual(sqlrcur_getField($cur,7,6),"testblob8");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,1),40);
	assertEqual(sqlrcur_getFieldLength($cur,0,2),12);
	assertEqual(sqlrcur_getFieldLength($cur,0,3),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,4),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,5),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,6),0);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,1),40);
	assertEqual(sqlrcur_getFieldLength($cur,7,2),12);
	assertEqual(sqlrcur_getFieldLength($cur,7,3),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,4),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,5),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,6),9);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqual(sqlrcur_getField($cur,0,"TESTNUMBER"),"1");
	assertEqual(sqlrcur_getField($cur,0,"TESTCHAR"),"testchar1                               ");
	assertEqual(sqlrcur_getField($cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqual(sqlrcur_getField($cur,0,"TESTDATE"),"01-JAN-01");
	assertEqual(sqlrcur_getField($cur,0,"TESTLONG"),"testlong1");
	assertEqual(sqlrcur_getField($cur,0,"TESTCLOB"),"testclob1");
	assertEqual(sqlrcur_getField($cur,0,"TESTBLOB"),"");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,"TESTNUMBER"),"8");
	assertEqual(sqlrcur_getField($cur,7,"TESTCHAR"),"testchar8                               ");
	assertEqual(sqlrcur_getField($cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqual(sqlrcur_getField($cur,7,"TESTDATE"),"01-JAN-08");
	assertEqual(sqlrcur_getField($cur,7,"TESTLONG"),"testlong8");
	assertEqual(sqlrcur_getField($cur,7,"TESTCLOB"),"testclob8");
	assertEqual(sqlrcur_getField($cur,7,"TESTBLOB"),"testblob8");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTNUMBER"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTCHAR"),40);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTVARCHAR"),12);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTDATE"),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTLONG"),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTCLOB"),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTBLOB"),0);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTNUMBER"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTCHAR"),40);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTVARCHAR"),12);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTDATE"),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTLONG"),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTCLOB"),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTBLOB"),9);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqual($fields[0],"1");
	assertEqual($fields[1],"testchar1                               ");
	assertEqual($fields[2],"testvarchar1");
	assertEqual($fields[3],"01-JAN-01");
	assertEqual($fields[4],"testlong1");
	assertEqual($fields[5],"testclob1");
	assertEqual($fields[6],"");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqual($fieldlens[0],1);
	assertEqual($fieldlens[1],40);
	assertEqual($fieldlens[2],12);
	assertEqual($fieldlens[3],9);
	assertEqual($fieldlens[4],9);
	assertEqual($fieldlens[5],9);
	assertEqual($fieldlens[6],0);
	echo("\n");


	# fields by associative array
	echo("FIELDS BY ASSOCIATIVE ARRAY: \n");
	$fields=sqlrcur_getRowAssoc($cur,0);
	assertEqual($fields["TESTNUMBER"],1);
	assertEqual($fields["TESTCHAR"],"testchar1                               ");
	assertEqual($fields["TESTVARCHAR"],"testvarchar1");
	assertEqual($fields["TESTDATE"],"01-JAN-01");
	assertEqual($fields["TESTLONG"],"testlong1");
	assertEqual($fields["TESTCLOB"],"testclob1");
	assertEqual($fields["TESTBLOB"],"");
	echo("\n");
	$fields=sqlrcur_getRowAssoc($cur,7);
	assertEqual($fields["TESTNUMBER"],8);
	assertEqual($fields["TESTCHAR"],"testchar8                               ");
	assertEqual($fields["TESTVARCHAR"],"testvarchar8");
	assertEqual($fields["TESTDATE"],"01-JAN-08");
	assertEqual($fields["TESTLONG"],"testlong8");
	assertEqual($fields["TESTCLOB"],"testclob8");
	assertEqual($fields["TESTBLOB"],"testblob8");
	echo("\n");


	# field lengths by associative array
	echo("FIELD LENGTHS BY ASSOCIATIVE ARRAY: \n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,0);
	assertEqual($fieldlengths["TESTNUMBER"],1);
	assertEqual($fieldlengths["TESTCHAR"],40);
	assertEqual($fieldlengths["TESTVARCHAR"],12);
	assertEqual($fieldlengths["TESTDATE"],9);
	assertEqual($fieldlengths["TESTLONG"],9);
	assertEqual($fieldlengths["TESTCLOB"],9);
	assertEqual($fieldlengths["TESTBLOB"],0);
	echo("\n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,7);
	assertEqual($fieldlengths["TESTNUMBER"],1);
	assertEqual($fieldlengths["TESTCHAR"],40);
	assertEqual($fieldlengths["TESTVARCHAR"],12);
	assertEqual($fieldlengths["TESTDATE"],9);
	assertEqual($fieldlengths["TESTLONG"],9);
	assertEqual($fieldlengths["TESTCLOB"],9);
	assertEqual($fieldlengths["TESTBLOB"],9);
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select $(var1),'$(var2)',$(var3) from dual");
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
	sqlrcur_prepareQuery($cur,"select $(var1),'$(var2)',$(var3) from dual");
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
	echo("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL from dual"));
	assertEqual(sqlrcur_getField($cur,0,0),NULL);
	assertEqual(sqlrcur_getField($cur,0,1),"1");
	assertEqual(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select NULL,1,NULL from dual"));
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
		"	testnumber "));
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
		"	testnumber "));
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
		"	testnumber "));
	assertEqual(sqlrcur_getColumnName($cur,0),"TESTNUMBER");
	assertEqual(sqlrcur_getColumnLength($cur,0),22);
	assertEqual(sqlrcur_getColumnType($cur,0),"NUMBER");
	echo("\n");


	# suspended session
	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber "));
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
		"	testnumber "));
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
		"	testnumber "));
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
		"	testnumber "));
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
		"	testnumber "));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqual($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_colCount($cur),7);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"TESTNUMBER");
	assertEqual(sqlrcur_getColumnName($cur,1),"TESTCHAR");
	assertEqual(sqlrcur_getColumnName($cur,2),"TESTVARCHAR");
	assertEqual(sqlrcur_getColumnName($cur,3),"TESTDATE");
	assertEqual(sqlrcur_getColumnName($cur,4),"TESTLONG");
	assertEqual(sqlrcur_getColumnName($cur,5),"TESTCLOB");
	assertEqual(sqlrcur_getColumnName($cur,6),"TESTBLOB");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"TESTNUMBER");
	assertEqual($cols[1],"TESTCHAR");
	assertEqual($cols[2],"TESTVARCHAR");
	assertEqual($cols[3],"TESTDATE");
	assertEqual($cols[4],"TESTLONG");
	assertEqual($cols[5],"TESTCLOB");
	assertEqual($cols[6],"TESTBLOB");
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
		"	testnumber "));
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
		"	testnumber "));
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
	$secondcon=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	sqlrcon_enableTls($secondcon,null,$cert,null,null,"ca",$ca,0);
	assertTrue(sqlrcur_sendQuery($secondcur,
		"select ".
		"	count(*) ".
		"from ".
		"	testtable "));
	assertEqual(sqlrcur_getField($secondcur,0,0),"0");
	assertTrue(sqlrcon_commit($con));
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
		"	'testchar10', ".
		"	'testvarchar10', ".
		"	'01-JAN-2010', ".
		"	'testlong10', ".
		"	'testclob10', ".
		"	empty_blob())"));
	assertTrue(sqlrcur_sendQuery($secondcur,
		"select ".
		"	count(*) ".
		"from ".
		"	testtable "));
	assertEqual(sqlrcur_getField($secondcur,0,0),"9");
	assertTrue(sqlrcon_autoCommitOff($con));
	echo("\n");


	# output bind by position
	echo("OUTPUT BIND BY POSITION: \n");
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	:numvar:=1; ".
		"	:stringvar:='hello'; ".
		"	:floatvar:=2.5; ".
		"end;");
	sqlrcur_defineOutputBindInteger($cur,"1");
	sqlrcur_defineOutputBindString($cur,"2",10);
	sqlrcur_defineOutputBindDouble($cur,"3");
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"1");
	$stringvar=sqlrcur_getOutputBindString($cur,"2");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"3");
	assertEqual($numvar,1);
	assertEqual($stringvar,"hello");
	assertEqual($floatvar,2.5);
	echo("\n");


	# output bind by name
	echo("OUTPUT BIND BY NAME: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_defineOutputBindInteger($cur,"numvar");
	sqlrcur_defineOutputBindString($cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble($cur,"floatvar");
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"numvar");
	$stringvar=sqlrcur_getOutputBindString($cur,"stringvar");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"floatvar");
	assertEqual($numvar,1);
	assertEqual($stringvar,"hello");
	assertEqual($floatvar,2.5);
	echo("\n");


	# output bind by name with validation
	echo("OUTPUT BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_defineOutputBindInteger($cur,"numvar");
	sqlrcur_defineOutputBindString($cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble($cur,"floatvar");
	sqlrcur_defineOutputBindString($cur,"dummyvar",10);
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	$numvar=sqlrcur_getOutputBindInteger($cur,"numvar");
	$stringvar=sqlrcur_getOutputBindString($cur,"stringvar");
	$floatvar=sqlrcur_getOutputBindDouble($cur,"floatvar");
	assertEqual($numvar,1);
	assertEqual($stringvar,"hello");
	assertEqual($floatvar,2.5);
	echo("\n");


	# clob and blob output bind
	echo("CLOB AND BLOB OUTPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable1");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable1 (".
		"	testclob clob, ".
		"	testblob blob)"));
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable1 ".
		"values (".
		"	'hello', ".
		"	:var1)");
	sqlrcur_inputBindBlob($cur,"var1","hello",5);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_prepareQuery($cur,
		"begin ".
		"	select testclob into :clobvar from testtable1; ".
		"	select testblob into :blobvar from testtable1; ".
		"end;");
	sqlrcur_defineOutputBindClob($cur,"clobvar");
	sqlrcur_defineOutputBindBlob($cur,"blobvar");
	assertTrue(sqlrcur_executeQuery($cur));
	$clobvar=sqlrcur_getOutputBindClob($cur,"clobvar");
	$clobvarlength=sqlrcur_getOutputBindLength($cur,"clobvar");
	$blobvar=sqlrcur_getOutputBindBlob($cur,"blobvar");
	$blobvarlength=sqlrcur_getOutputBindLength($cur,"blobvar");
	assertEqual($clobvar,"hello");
	assertEqual($clobvarlength,5);
	assertEqual($blobvar,"hello",5);
	assertEqual($blobvarlength,5);
	sqlrcur_sendQuery($cur,"drop table testtable1");
	echo("\n");


	# null and empty clobs and clobs
	echo("NULL AND EMPTY CLOBS AND CLOBS: \n");
	sqlrcur_getNullsAsNulls($cur);
	sqlrcur_sendQuery($cur,
		"create table testtable1 (".
		"	testclob1 clob, ".
		"	testclob2 clob, ".
		"	testblob1 blob, ".
		"	testblob2 blob)");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable1 ".
		"values (".
		"	:var1, ".
		"	:var2, ".
		"	:var3, ".
		"	:var4)");
	sqlrcur_inputBindClob($cur,"var1","",0);
	sqlrcur_inputBindClob($cur,"var2",NULL,0);
	sqlrcur_inputBindBlob($cur,"var3","",0);
	sqlrcur_inputBindBlob($cur,"var4",NULL,0);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable1");
	assertEqual(sqlrcur_getField($cur,0,0),NULL);
	assertEqual(sqlrcur_getField($cur,0,1),NULL);
	assertEqual(sqlrcur_getField($cur,0,2),NULL);
	assertEqual(sqlrcur_getField($cur,0,3),NULL);
	sqlrcur_sendQuery($cur,"drop table testtable1");
	echo("\n");


	# cursor binds
	echo("CURSOR BINDS: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"create or replace ".
		"package types ".
		"as ".
		"	type cursorType is ref cursor; ".
		"end;"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create or replace ".
		"function sp_testtable ".
		"return types.cursortype ".
		"as ".
		"	l_cursor types.cursorType; ".
		"begin ".
		"	open l_cursor for  ".
		"		select * from testtable; ".
		"	return l_cursor; ".
		"end;"));
	sqlrcur_prepareQuery($cur,"begin  :curs:=sp_testtable; end;");
	sqlrcur_defineOutputBindCursor($cur,"curs");
	assertTrue(sqlrcur_executeQuery($cur));
	$bindcur=sqlrcur_getOutputBindCursor($cur,"curs");
	assertTrue(sqlrcur_fetchFromBindCursor($bindcur));
	assertEqual(sqlrcur_getField($bindcur,0,0),"1");
	assertEqual(sqlrcur_getField($bindcur,1,0),"2");
	assertEqual(sqlrcur_getField($bindcur,2,0),"3");
	assertEqual(sqlrcur_getField($bindcur,3,0),"4");
	assertEqual(sqlrcur_getField($bindcur,4,0),"5");
	assertEqual(sqlrcur_getField($bindcur,5,0),"6");
	assertEqual(sqlrcur_getField($bindcur,6,0),"7");
	assertEqual(sqlrcur_getField($bindcur,7,0),"8");
	sqlrcur_free($bindcur);
	echo("\n");


	# long clob
	echo("LONG CLOB: \n");
	sqlrcur_sendQuery($cur,"drop table testtable2");
	sqlrcur_sendQuery($cur,"create table testtable2 (testclob clob)");
	sqlrcur_prepareQuery($cur,"insert into testtable2 values (:clobval)");
	$clobval="";
	for ($i=0; $i<8*1024; $i++) {
		$clobval=$clobval.'C';
	}
	sqlrcur_inputBindClob($cur,"clobval",$clobval,8*1024);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testclob from testtable2");
	assertEqual($clobval,sqlrcur_getField($cur,0,"TESTCLOB"));
	sqlrcur_prepareQuery($cur,
		"begin select testclob into :clobbindval from testtable2; ".
		"	end;");
	sqlrcur_defineOutputBindClob($cur,"clobbindval");
	assertTrue(sqlrcur_executeQuery($cur));
	$clobbindvar=sqlrcur_getOutputBindClob($cur,"clobbindval");
	assertEqual(sqlrcur_getOutputBindLength($cur,"clobbindval"),8*1024);
	assertEqual($clobval,$clobbindvar);
	sqlrcur_sendQuery($cur,"drop table testtable2");
	echo("\n");


	# long output bind
	echo("LONG OUTPUT BIND\n");
	sqlrcur_sendQuery($cur,"drop table testtable2");
	sqlrcur_sendQuery($cur,
		"create table ".
		"	testtable2 (testval varchar2(4000))");
	sqlrcur_prepareQuery($cur,"insert into testtable2 values (:testval)");
	$testval="";
	for ($i=0; $i<4000; $i++) {
		$testval=$testval."C";
	}
	sqlrcur_inputBind($cur,"testval",$testval);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable2");
	assertEqual($testval,sqlrcur_getField($cur,0,"TESTVAL"));
	$query="begin :bindval:='".$testval."'; end;";
	sqlrcur_prepareQuery($cur,$query);
	sqlrcur_defineOutputBindString($cur,"bindval",4000);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqual(sqlrcur_getOutputBindLength($cur,"bindval"),4000);
	assertEqual(sqlrcur_getOutputBindString($cur,"bindval"),$testval);
	sqlrcur_sendQuery($cur,"drop table testtable2");
	echo("\n");


	# negative input bind
	echo("NEGATIVE INPUT BIND\n");
	sqlrcur_sendQuery($cur,"create table testtable2 (testval number)");
	sqlrcur_prepareQuery($cur,"insert into testtable2 values (:testval)");
	sqlrcur_inputBind($cur,"testval",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable2");
	assertEqual(sqlrcur_getField($cur,0,"TESTVAL"),"-1");
	sqlrcur_sendQuery($cur,"drop table testtable2");
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
		"	testnumber "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber "));
	assertFalse(sqlrcur_sendQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testtable ".
		"order by ".
		"	testnumber "));
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
