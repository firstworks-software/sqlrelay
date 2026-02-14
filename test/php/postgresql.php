<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("./asserts.php");


	$host="sqlrelay";
	$port=9000;
	$socket="/tmp/test.socket";
	$user="testuser";
	$password="testpassword";

	# instantiation
	$con=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$cur=sqlrcur_alloc($con);


	# identify


	echo("IDENTIFY: \n");
	assertEqual(sqlrcon_identify($con),"postgresql");
	echo("\n");


	# ping


	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");

	# isolation levels
	/*echo("ISOLATION LEVELS: \n");
	$isolationlevels=array(
			"read committed",
			"read uncommitted",
			"repeatable read",
			"serializable");
	foreach ($isolationlevels as $il) {
		# postgresql requires the isolation level to
		# be the first query of the transaction
		sqlrcon_begin($con);
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		assertEqual(sqlrcon_getIsolationLevel($con),$il);
		sqlrcon_commit($con);
		echo("\n");
	}
	# reset to the default isolation level
	sqlrcon_begin($con);
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	sqlrcon_commit($con);
	echo("\n");*/

	# drop existing table
	sqlrcur_sendQuery($cur,"drop table testtable");


	# create temptable


	echo("CREATE TEMPTABLE: \n");
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
		"	testtimestamp timestamp)"));
	echo("\n");


	# begin transction


	echo("BEGIN TRANSCTION: \n");
	assertTrue(sqlrcur_sendQuery($cur,"begin"));
	echo("\n");


	# insert


	echo("INSERT: \n");
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	1.1, ".
		"	1.1, ".
		"	1, ".
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	'01/01/2001', ".
		"	'01:00:00', ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	2, ".
		"	2.2, ".
		"	2.2, ".
		"	2, ".
		"	'testchar2', ".
		"	'testvarchar2', ".
		"	'01/01/2002', ".
		"	'02:00:00', ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	3, ".
		"	3.3, ".
		"	3.3, ".
		"	3, ".
		"	'testchar3', ".
		"	'testvarchar3', ".
		"	'01/01/2003', ".
		"	'03:00:00', ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	4, ".
		"	4.4, ".
		"	4.4, ".
		"	4, ".
		"	'testchar4', ".
		"	'testvarchar4', ".
		"	'01/01/2004', ".
		"	'04:00:00', ".
		"	NULL)"));
	echo("\n");


	# affected rows


	echo("AFFECTED ROWS: \n");
	assertEqual(sqlrcur_affectedRows($cur),1);
	echo("\n");


	# bind by name


	echo("BIND BY NAME: \n");
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
		"	\$8)");
	assertEqual(sqlrcur_countBindVariables($cur),8);
	sqlrcur_inputBind($cur,"1",5);
	sqlrcur_inputBind($cur,"2",5.5,4,2);
	sqlrcur_inputBind($cur,"3",5.5,4,2);
	sqlrcur_inputBind($cur,"4",5);
	sqlrcur_inputBind($cur,"5","testchar5");
	sqlrcur_inputBind($cur,"6","testvarchar5");
	sqlrcur_inputBind($cur,"7","01/01/2005");
	sqlrcur_inputBind($cur,"8","05:00:00");
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",6);
	sqlrcur_inputBind($cur,"2",6.6,4,2);
	sqlrcur_inputBind($cur,"3",6.6,4,2);
	sqlrcur_inputBind($cur,"4",6);
	sqlrcur_inputBind($cur,"5","testchar6");
	sqlrcur_inputBind($cur,"6","testvarchar6");
	sqlrcur_inputBind($cur,"7","01/01/2006");
	sqlrcur_inputBind($cur,"8","06:00:00");
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array bind by name


	echo("ARRAY BIND BY NAME: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("1","2","3","4","5","6","7","8");
	$bindvals=array(7,7.7,7.7,7,
			"testchar7","testvarchar7",
			"01/01/2007","07:00:00");
	$precs=array(0,2,2,0,0,0,0,0);
	$scales=array(0,1,1,0,0,0,0,0);
	sqlrcur_inputBinds($cur,$bindvars,$bindvals,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# bind by name with validation


	echo("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",8);
	sqlrcur_inputBind($cur,"2",8.8,4,2);
	sqlrcur_inputBind($cur,"3",8.8,4,2);
	sqlrcur_inputBind($cur,"4",8);
	sqlrcur_inputBind($cur,"5","testchar8");
	sqlrcur_inputBind($cur,"6","testvarchar8");
	sqlrcur_inputBind($cur,"7","01/01/2008");
	sqlrcur_inputBind($cur,"8","08:00:00");
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
		"	testint "));
	echo("\n");


	# column count


	echo("COLUMN COUNT: \n");
	assertEqual(sqlrcur_colCount($cur),9);
	echo("\n");


	# column names


	echo("COLUMN NAMES: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,2),"testreal");
	assertEqual(sqlrcur_getColumnName($cur,3),"testsmallint");
	assertEqual(sqlrcur_getColumnName($cur,4),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,5),"testvarchar");
	assertEqual(sqlrcur_getColumnName($cur,6),"testdate");
	assertEqual(sqlrcur_getColumnName($cur,7),"testtime");
	assertEqual(sqlrcur_getColumnName($cur,8),"testtimestamp");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testint");
	assertEqual($cols[1],"testfloat");
	assertEqual($cols[2],"testreal");
	assertEqual($cols[3],"testsmallint");
	assertEqual($cols[4],"testchar");
	assertEqual($cols[5],"testvarchar");
	assertEqual($cols[6],"testdate");
	assertEqual($cols[7],"testtime");
	assertEqual($cols[8],"testtimestamp");
	echo("\n");


	# column types


	echo("COLUMN TYPES: \n");
	assertEqual(sqlrcur_getColumnType($cur,0),"int4");
	assertEqual(sqlrcur_getColumnType($cur,"testint"),"int4");
	assertEqual(sqlrcur_getColumnType($cur,1),"float8");
	assertEqual(sqlrcur_getColumnType($cur,"testfloat"),"float8");
	assertEqual(sqlrcur_getColumnType($cur,2),"float4");
	assertEqual(sqlrcur_getColumnType($cur,"testreal"),"float4");
	assertEqual(sqlrcur_getColumnType($cur,3),"int2");
	assertEqual(sqlrcur_getColumnType($cur,"testsmallint"),"int2");
	assertEqual(sqlrcur_getColumnType($cur,4),"bpchar");
	assertEqual(sqlrcur_getColumnType($cur,"testchar"),"bpchar");
	assertEqual(sqlrcur_getColumnType($cur,5),"varchar");
	assertEqual(sqlrcur_getColumnType($cur,"testvarchar"),"varchar");
	assertEqual(sqlrcur_getColumnType($cur,6),"date");
	assertEqual(sqlrcur_getColumnType($cur,"testdate"),"date");
	assertEqual(sqlrcur_getColumnType($cur,7),"time");
	assertEqual(sqlrcur_getColumnType($cur,"testtime"),"time");
	assertEqual(sqlrcur_getColumnType($cur,8),"timestamp");
	assertEqual(sqlrcur_getColumnType($cur,"testtimestamp"),"timestamp");
	echo("\n");


	# column length


	echo("COLUMN LENGTH: \n");
	assertEqual(sqlrcur_getColumnLength($cur,0),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testint"),4);
	assertEqual(sqlrcur_getColumnLength($cur,1),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testfloat"),8);
	assertEqual(sqlrcur_getColumnLength($cur,2),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testreal"),4);
	assertEqual(sqlrcur_getColumnLength($cur,3),2);
	assertEqual(sqlrcur_getColumnLength($cur,"testsmallint"),2);
	assertEqual(sqlrcur_getColumnLength($cur,4),44);
	assertEqual(sqlrcur_getColumnLength($cur,"testchar"),44);
	assertEqual(sqlrcur_getColumnLength($cur,5),44);
	assertEqual(sqlrcur_getColumnLength($cur,"testvarchar"),44);
	assertEqual(sqlrcur_getColumnLength($cur,6),4);
	assertEqual(sqlrcur_getColumnLength($cur,"testdate"),4);
	assertEqual(sqlrcur_getColumnLength($cur,7),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testtime"),8);
	assertEqual(sqlrcur_getColumnLength($cur,8),8);
	assertEqual(sqlrcur_getColumnLength($cur,"testtimestamp"),8);
	echo("\n");


	# longest column


	echo("LONGEST COLUMN: \n");
	assertEqual(sqlrcur_getLongest($cur,0),1);
	assertEqual(sqlrcur_getLongest($cur,"testint"),1);
	assertEqual(sqlrcur_getLongest($cur,1),3);
	assertEqual(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqual(sqlrcur_getLongest($cur,2),3);
	assertEqual(sqlrcur_getLongest($cur,"testreal"),3);
	assertEqual(sqlrcur_getLongest($cur,3),1);
	assertEqual(sqlrcur_getLongest($cur,"testsmallint"),1);
	assertEqual(sqlrcur_getLongest($cur,4),40);
	assertEqual(sqlrcur_getLongest($cur,"testchar"),40);
	assertEqual(sqlrcur_getLongest($cur,5),12);
	assertEqual(sqlrcur_getLongest($cur,"testvarchar"),12);
	assertEqual(sqlrcur_getLongest($cur,6),10);
	assertEqual(sqlrcur_getLongest($cur,"testdate"),10);
	assertEqual(sqlrcur_getLongest($cur,7),8);
	assertEqual(sqlrcur_getLongest($cur,"testtime"),8);
	echo("\n");


	# row count


	echo("ROW COUNT: \n");
	assertEqual(sqlrcur_rowCount($cur),8);
	echo("\n");

	/*echo("TOTAL ROWS: \n");
	assertEqual(sqlrcur_totalRows($cur),8);
	echo("\n");*/


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
	assertEqual(sqlrcur_getField($cur,0,1),"1.1");
	assertEqual(sqlrcur_getField($cur,0,2),"1.1");
	assertEqual(sqlrcur_getField($cur,0,3),"1");
	assertEqual(sqlrcur_getField($cur,0,4),"testchar1                               ");
	assertEqual(sqlrcur_getField($cur,0,5),"testvarchar1");
	assertEqual(sqlrcur_getField($cur,0,6),"2001-01-01");
	assertEqual(sqlrcur_getField($cur,0,7),"01:00:00");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,7,1),"8.8");
	assertEqual(sqlrcur_getField($cur,7,2),"8.8");
	assertEqual(sqlrcur_getField($cur,7,3),"8");
	assertEqual(sqlrcur_getField($cur,7,4),"testchar8                               ");
	assertEqual(sqlrcur_getField($cur,7,5),"testvarchar8");
	assertEqual(sqlrcur_getField($cur,7,6),"2008-01-01");
	assertEqual(sqlrcur_getField($cur,7,7),"08:00:00");
	echo("\n");


	# field lengths by index


	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,1),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,2),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,3),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,4),40);
	assertEqual(sqlrcur_getFieldLength($cur,0,5),12);
	assertEqual(sqlrcur_getFieldLength($cur,0,6),10);
	assertEqual(sqlrcur_getFieldLength($cur,0,7),8);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,1),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,2),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,3),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,4),40);
	assertEqual(sqlrcur_getFieldLength($cur,7,5),12);
	assertEqual(sqlrcur_getFieldLength($cur,7,6),10);
	assertEqual(sqlrcur_getFieldLength($cur,7,7),8);
	echo("\n");


	# fields by name


	echo("FIELDS BY NAME: \n");
	assertEqual(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testfloat"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testreal"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testsmallint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testchar"),"testchar1                               ");
	assertEqual(sqlrcur_getField($cur,0,"testvarchar"),"testvarchar1");
	assertEqual(sqlrcur_getField($cur,0,"testdate"),"2001-01-01");
	assertEqual(sqlrcur_getField($cur,0,"testtime"),"01:00:00");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testfloat"),"8.8");
	assertEqual(sqlrcur_getField($cur,7,"testreal"),"8.8");
	assertEqual(sqlrcur_getField($cur,7,"testsmallint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testchar"),"testchar8                               ");
	assertEqual(sqlrcur_getField($cur,7,"testvarchar"),"testvarchar8");
	assertEqual(sqlrcur_getField($cur,7,"testdate"),"2008-01-01");
	assertEqual(sqlrcur_getField($cur,7,"testtime"),"08:00:00");
	echo("\n");


	# field lengths by name


	echo("FIELD LENGTHS BY NAME: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testreal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testsmallint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testchar"),40);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testvarchar"),12);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testdate"),10);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testtime"),8);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testreal"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testsmallint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testchar"),40);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testvarchar"),12);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testdate"),10);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testtime"),8);
	echo("\n");


	# fields by array


	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqual($fields[0],"1");
	assertEqual($fields[1],"1.1");
	assertEqual($fields[2],"1.1");
	assertEqual($fields[3],"1");
	assertEqual($fields[4],"testchar1                               ");
	assertEqual($fields[5],"testvarchar1");
	assertEqual($fields[6],"2001-01-01");
	assertEqual($fields[7],"01:00:00");
	echo("\n");


	# field lengths by array


	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqual($fieldlens[0],1);
	assertEqual($fieldlens[1],3);
	assertEqual($fieldlens[2],3);
	assertEqual($fieldlens[3],1);
	assertEqual($fieldlens[4],40);
	assertEqual($fieldlens[5],12);
	assertEqual($fieldlens[6],10);
	assertEqual($fieldlens[7],8);
	echo("\n");


	# fields by associative array


	echo("FIELDS BY ASSOCIATIVE ARRAY: \n");
	$fields=sqlrcur_getRowAssoc($cur,0);
	assertEqual($fields["testint"],"1");
	assertEqual($fields["testfloat"],"1.1");
	assertEqual($fields["testreal"],"1.1");
	assertEqual($fields["testsmallint"],"1");
	assertEqual($fields["testchar"],"testchar1                               ");
	assertEqual($fields["testvarchar"],"testvarchar1");
	assertEqual($fields["testdate"],"2001-01-01");
	assertEqual($fields["testtime"],"01:00:00");
	echo("\n");
	$fields=sqlrcur_getRowAssoc($cur,7);
	assertEqual($fields["testint"],"8");
	assertEqual($fields["testfloat"],"8.8");
	assertEqual($fields["testreal"],"8.8");
	assertEqual($fields["testsmallint"],"8");
	assertEqual($fields["testchar"],"testchar8                               ");
	assertEqual($fields["testvarchar"],"testvarchar8");
	assertEqual($fields["testdate"],"2008-01-01");
	assertEqual($fields["testtime"],"08:00:00");
	echo("\n");


	# field lengths by associative array


	echo("FIELD LENGTHS BY ASSOCIATIVE ARRAY: \n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,0);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testreal"],3);
	assertEqual($fieldlengths["testsmallint"],1);
	assertEqual($fieldlengths["testchar"],40);
	assertEqual($fieldlengths["testvarchar"],12);
	assertEqual($fieldlengths["testdate"],10);
	assertEqual($fieldlengths["testtime"],8);
	echo("\n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,7);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testreal"],3);
	assertEqual($fieldlengths["testsmallint"],1);
	assertEqual($fieldlengths["testchar"],40);
	assertEqual($fieldlengths["testvarchar"],12);
	assertEqual($fieldlengths["testdate"],10);
	assertEqual($fieldlengths["testtime"],8);
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
	assertEqual(sqlrcur_getColumnType($cur,0),"int4");
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
	assertEqual(sqlrcur_colCount($cur),9);
	echo("\n");


	# column names for cached result set


	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,2),"testreal");
	assertEqual(sqlrcur_getColumnName($cur,3),"testsmallint");
	assertEqual(sqlrcur_getColumnName($cur,4),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,5),"testvarchar");
	assertEqual(sqlrcur_getColumnName($cur,6),"testdate");
	assertEqual(sqlrcur_getColumnName($cur,7),"testtime");
	assertEqual(sqlrcur_getColumnName($cur,8),"testtimestamp");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testint");
	assertEqual($cols[1],"testfloat");
	assertEqual($cols[2],"testreal");
	assertEqual($cols[3],"testsmallint");
	assertEqual($cols[4],"testchar");
	assertEqual($cols[5],"testvarchar");
	assertEqual($cols[6],"testdate");
	assertEqual($cols[7],"testtime");
	assertEqual($cols[8],"testtimestamp");
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


	# commit and rollback


	echo("COMMIT AND ROLLBACK: \n");
	$secondcon=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$secondcur=sqlrcur_alloc($secondcon);
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
	#assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	10, ".
		"	10.1, ".
		"	10.1, ".
		"	10, ".
		"	'testchar10', ".
		"	'testvarchar10', ".
		"	'01/01/2010', ".
		"	'10:00:00', ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($secondcur,
		"select ".
		"	count(*) ".
		"from ".
		"	testtable "));
	assertEqual(sqlrcur_getField($secondcur,0,0),"9");
	#assertTrue(sqlrcon_autoCommitOff($con));
	echo("\n");


	# stored procedures


	echo("STORED PROCEDURES: \n");
	sqlrcur_sendQuery($cur,"drop function testfunc(int)");
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testfunc(int) returns int as ".
		"	' begin return \$1; end;' language plpgsql"));
	sqlrcur_prepareQuery($cur,"select * from testfunc(\$1)");
	sqlrcur_inputBind($cur,"1",5);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqual(sqlrcur_getField($cur,0,0),"5");
	sqlrcur_sendQuery($cur,"drop function testfunc(int)");

	sqlrcur_sendQuery($cur,"drop function testfunc(int,char(20))");
	assertTrue(sqlrcur_sendQuery($cur,
		"create function testfunc(".
		"	int, char(20)) ".
		"returns record as ' ".
		"	declare output record; ".
		"begin ".
		"	select $1,$2 into output; ".
		"	return output; ".
		"end;' language plpgsql"));
	sqlrcur_prepareQuery($cur,
		"select ".
		"	* ".
		"from ".
		"	testfunc(\$1,\$2) as (col1 int, col2 bpchar) ");
	sqlrcur_inputBind($cur,"1",5);
	sqlrcur_inputBind($cur,"2","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqual(sqlrcur_getField($cur,0,0),"5");
	assertEqual(sqlrcur_getField($cur,0,1),"hello");
	sqlrcur_sendQuery($cur,"drop function testfunc(int,char(20))");
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

	sqlrcur_free($secondcur);
	sqlrcon_free($secondcon);

	sqlrcur_free($cur);
	sqlrcon_free($con);
	reportTestStatus();

	exit($status);
?></pre></html>
