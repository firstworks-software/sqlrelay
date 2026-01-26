<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("./assert.php");


	$host="sqlrelay";
	$port=9000;
	$socket="/tmp/test.socket";
	$user="testuser";
	$password="testpassword";

	# instantiation
	$con=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$cur=sqlrcur_alloc($con);

	# get database type
	echo("IDENTIFY: \n");
	assertEqual(sqlrcon_identify($con),"sqlite");
	echo("\n");

	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");

	# isolation levels
	echo("ISOLATION LEVELS: \n");
	$isolationlevels=array("0","1");
	foreach ($isolationlevels as $il) {
		assertTrue(sqlrcon_setIsolationLevel($con,$il));
		assertEqual(sqlrcon_getIsolationLevel($con),$il);
		echo("\n");
	}
	# reset to the default isolation level
	assertTrue(sqlrcon_setIsolationLevel($con,$isolationlevels[0]));
	echo("\n");

	# drop existing table
	sqlrcur_sendQuery($cur,"begin");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcon_commit($con);

	# create a new table
	echo("CREATE TEMPTABLE: \n");
	sqlrcur_sendQuery($cur,"begin");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable (testint int, testfloat float, testchar char(40), testvarchar varchar(40))"));
	sqlrcon_commit($con);
	echo("\n");

	echo("INSERT: \n");
	sqlrcur_sendQuery($cur,"begin");
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1,1.1,'testchar1','testvarchar1')"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (2,2.2,'testchar2','testvarchar2')"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (3,3.3,'testchar3','testvarchar3')"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (4,4.4,'testchar4','testvarchar4')"));
	echo("\n");

	echo("AFFECTED ROWS: \n");
	assertEqual(sqlrcur_affectedRows($cur),0);
	echo("\n");

	echo("BIND BY NAME: \n");
	sqlrcur_prepareQuery($cur,"insert into testtable values (:var1,:var2,:var3,:var4)");
	assertEqual(sqlrcur_countBindVariables($cur),4);
	sqlrcur_inputBind($cur,"var1",5);
	sqlrcur_inputBind($cur,"var2",5.5,4,1);
	sqlrcur_inputBind($cur,"var3","testchar5");
	sqlrcur_inputBind($cur,"var4","testvarchar5");
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",6);
	sqlrcur_inputBind($cur,"var2",6.6,4,1);
	sqlrcur_inputBind($cur,"var3","testchar6");
	sqlrcur_inputBind($cur,"var4","testvarchar6");
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");

	echo("ARRAY BIND BY NAME: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("var1","var2","var3","var4");
	$bindvals=array(7,7.7,"testchar7","testvarchar7");
	$precs=array(0,2,0,0);
	$scales=array(0,1,0,0);
	sqlrcur_inputBinds($cur,$bindvars,$bindvals,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");

	echo("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",8);
	sqlrcur_inputBind($cur,"var2",8.8,4,1);
	sqlrcur_inputBind($cur,"var3","testchar8");
	sqlrcur_inputBind($cur,"var4","testvarchar8");
	sqlrcur_validateBinds($cur);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");

	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
	echo("\n");

	echo("COLUMN COUNT: \n");
	assertEqual(sqlrcur_colCount($cur),4);
	echo("\n");

	echo("COLUMN NAMES: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,2),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,3),"testvarchar");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testint");
	assertEqual($cols[1],"testfloat");
	assertEqual($cols[2],"testchar");
	assertEqual($cols[3],"testvarchar");
	echo("\n");

	echo("COLUMN TYPES: \n");
	assertEqual(sqlrcur_getColumnType($cur,0),"INTEGER");
	assertEqual(sqlrcur_getColumnType($cur,"testint"),"INTEGER");
	assertEqual(sqlrcur_getColumnType($cur,1),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,"testfloat"),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,2),"STRING");
	assertEqual(sqlrcur_getColumnType($cur,"testchar"),"STRING");
	assertEqual(sqlrcur_getColumnType($cur,3),"STRING");
	assertEqual(sqlrcur_getColumnType($cur,"testvarchar"),"STRING");
	echo("\n");

	echo("COLUMN LENGTH: \n");
	assertEqual(sqlrcur_getColumnLength($cur,0),0);
	assertEqual(sqlrcur_getColumnLength($cur,"testint"),0);
	assertEqual(sqlrcur_getColumnLength($cur,1),0);
	assertEqual(sqlrcur_getColumnLength($cur,"testfloat"),0);
	assertEqual(sqlrcur_getColumnLength($cur,2),0);
	assertEqual(sqlrcur_getColumnLength($cur,"testchar"),0);
	assertEqual(sqlrcur_getColumnLength($cur,3),0);
	assertEqual(sqlrcur_getColumnLength($cur,"testvarchar"),0);
	echo("\n");

	echo("LONGEST COLUMN: \n");
	assertEqual(sqlrcur_getLongest($cur,0),1);
	assertEqual(sqlrcur_getLongest($cur,"testint"),1);
	assertEqual(sqlrcur_getLongest($cur,1),3);
	assertEqual(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqual(sqlrcur_getLongest($cur,2),9);
	assertEqual(sqlrcur_getLongest($cur,"testchar"),9);
	assertEqual(sqlrcur_getLongest($cur,3),12);
	assertEqual(sqlrcur_getLongest($cur,"testvarchar"),12);
	echo("\n");

	echo("ROW COUNT: \n");
	assertEqual(sqlrcur_rowCount($cur),8);
	echo("\n");

	echo("TOTAL ROWS: \n");
	assertEqual(sqlrcur_totalRows($cur),0);
	echo("\n");

	echo("FIRST ROW INDEX: \n");
	assertEqual(sqlrcur_firstRowIndex($cur),0);
	echo("\n");

	echo("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet($cur));
	echo("\n");

	echo("FIELDS BY INDEX: \n");
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"1.1");
	assertEqual(sqlrcur_getField($cur,0,2),"testchar1");
	assertEqual(sqlrcur_getField($cur,0,3),"testvarchar1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,7,1),"8.8");
	assertEqual(sqlrcur_getField($cur,7,2),"testchar8");
	assertEqual(sqlrcur_getField($cur,7,3),"testvarchar8");
	echo("\n");

	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,1),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,2),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,3),12);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,1),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,2),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,3),12);
	echo("\n");

	echo("FIELDS BY NAME: \n");
	assertEqual(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqual(sqlrcur_getField($cur,0,"testfloat"),"1.1");
	assertEqual(sqlrcur_getField($cur,0,"testchar"),"testchar1");
	assertEqual(sqlrcur_getField($cur,0,"testvarchar"),"testvarchar1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqual(sqlrcur_getField($cur,7,"testfloat"),"8.8");
	assertEqual(sqlrcur_getField($cur,7,"testchar"),"testchar8");
	assertEqual(sqlrcur_getField($cur,7,"testvarchar"),"testvarchar8");
	echo("\n");

	echo("FIELD LENGTHS BY NAME: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testchar"),9);
	assertEqual(sqlrcur_getFieldLength($cur,0,"testvarchar"),12);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testchar"),9);
	assertEqual(sqlrcur_getFieldLength($cur,7,"testvarchar"),12);
	echo("\n");

	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqual($fields[0],"1");
	assertEqual($fields[1],"1.1");
	assertEqual($fields[2],"testchar1");
	assertEqual($fields[3],"testvarchar1");
	echo("\n");

	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqual($fieldlens[0],1);
	assertEqual($fieldlens[1],3);
	assertEqual($fieldlens[2],9);
	assertEqual($fieldlens[3],12);
	echo("\n");



	echo("FIELDS BY ASSOCIATIVE ARRAY: \n");
	$fields=sqlrcur_getRowAssoc($cur,0);
	assertEqual($fields["testint"],"1");
	assertEqual($fields["testfloat"],"1.1");
	assertEqual($fields["testchar"],"testchar1");
	assertEqual($fields["testvarchar"],"testvarchar1");
	echo("\n");
	$fields=sqlrcur_getRowAssoc($cur,7);
	assertEqual($fields["testint"],"8");
	assertEqual($fields["testfloat"],"8.8");
	assertEqual($fields["testchar"],"testchar8");
	assertEqual($fields["testvarchar"],"testvarchar8");
	echo("\n");

	echo("FIELD LENGTHS BY ASSOCIATIVE ARRAY: \n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,0);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testchar"],9);
	assertEqual($fieldlengths["testvarchar"],12);
	echo("\n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,7);
	assertEqual($fieldlengths["testint"],1);
	assertEqual($fieldlengths["testfloat"],3);
	assertEqual($fieldlengths["testchar"],9);
	assertEqual($fieldlengths["testvarchar"],12);
	echo("\n");



	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_sendQuery($cur,"drop table testtable1");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable1 (col1 int, col2 char, col3 float)"));
	sqlrcur_prepareQuery($cur,"insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	sqlrcur_substitution($cur,"var1",1);
	sqlrcur_substitution($cur,"var2","hello");
	sqlrcur_substitution($cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");

	echo("FIELDS: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable1"));
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"hello");
	assertEqual(sqlrcur_getField($cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery($cur,"delete from testtable1"));
	echo("\n");

	echo("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	$vars=array("var1","var2","var3");
	$vals=array(1,"hello",10.5556);
	$precs=array(0,0,6);
	$scales=array(0,0,4);
	sqlrcur_substitutions($cur,$vars,$vals,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");

	echo("FIELDS: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable1"));
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"hello");
	assertEqual(sqlrcur_getField($cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery($cur,"delete from testtable1"));
	echo("\n");

	echo("NULLS as Nulls: \n");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable1 values (1,NULL,NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable1"));
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),NULL);
	assertEqual(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable1"));
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"");
	assertEqual(sqlrcur_getField($cur,0,2),"");
	sqlrcur_getNullsAsNulls($cur);
	echo("\n");

	echo("RESULT SET BUFFER SIZE: \n");
	assertEqual(sqlrcur_getResultSetBufferSize($cur),0);
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
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

	echo("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
	assertEqual(sqlrcur_getColumnName($cur,0),NULL);
	assertEqual(sqlrcur_getColumnLength($cur,0),0);
	assertEqual(sqlrcur_getColumnType($cur,0),NULL);
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnLength($cur,0),0);
	assertEqual(sqlrcur_getColumnType($cur,0),"INTEGER");
	echo("\n");

	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
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

	echo("SUSPENDED RESULT SET: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
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

	echo("CACHED RESULT SET: \n");
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqual($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");

	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_colCount($cur),4);
	echo("\n");

	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"testint");
	assertEqual(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqual(sqlrcur_getColumnName($cur,2),"testchar");
	assertEqual(sqlrcur_getColumnName($cur,3),"testvarchar");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"testint");
	assertEqual($cols[1],"testfloat");
	assertEqual($cols[2],"testchar");
	assertEqual($cols[3],"testvarchar");
	echo("\n");

	echo("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqual($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");

	echo("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	echo("\n");

	echo("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile1"));
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,"cachefile2"));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize($cur,0);
	echo("\n");

	echo("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testint"));
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

	echo("COMMIT AND ROLLBACK: \n");
	$secondcon=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqual(sqlrcur_getField($secondcur,0,0),"0");
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqual(sqlrcur_getField($secondcur,0,0),"8");
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (10,10.1,'testchar10','testvarchar10')"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqual(sqlrcur_getField($secondcur,0,0),"9");
	echo("\n");

	# drop existing table
	sqlrcur_sendQuery($cur,"drop table testtable");

	# invalid queries...
	echo("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable values (1,2,3,4)"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	echo("\n");

?></pre></html>
