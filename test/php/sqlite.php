<html><pre><?php
# Copyright (c) David Muse
# See the file COPYING for more information.
include("./asserts.php");


	$subvars=array("var1","var2","var3");
	$subvalstrings=array("hi","hello","bye");
	$subvallongs=array(1,2,3);
	$subvaldoubles=array(10.55,10.556,10.5556);
	$precs=array(4,5,6);
	$scales=array(2,3,4);

	$isolationlevels=array("0","1");


	# instantiation
	$con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
			"testuser","testpassword",0,1);
	$cur=sqlrcur_alloc($con);


	# identify
	echo("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify($con),"sqlite");
	echo("\n");


	# db version
	echo("DB VERSION: \n");
	$dbversion=sqlrcon_dbVersion($con);
	$issqlite3=1;
	if (!$dbversion ||
		strcmp($dbversion,"unknown")==0 ||
		intval($dbversion)<3) {
		$issqlite3=0;
	}
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
	assertEqStr(sqlrcon_bindFormat($con),":*");
	echo("\n");


	# nextval format
	echo("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat($con),"");
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
	sqlrcon_begin($con);
	sqlrcur_sendQuery($cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testint int, ".
		"	testfloat float, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testclob clob, ".
		"	testblob blob)"));
	sqlrcon_commit($con);
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
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	'testclob1', ".
		"	'testblob1')"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	2, ".
		"	2.5, ".
		"	'testchar2', ".
		"	'testvarchar2', ".
		"	'testclob2', ".
		"	'testblob2')"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	3, ".
		"	3.5, ".
		"	'testchar3', ".
		"	'testvarchar3', ".
		"	'testclob3', ".
		"	'testblob3')"));
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	4, ".
		"	4.5, ".
		"	'testchar4', ".
		"	'testvarchar4', ".
		"	'testclob4', ".
		"	'testblob4')"));
	echo("\n");


	# affected rows
	echo("AFFECTED ROWS: \n");
	assertEqInt(sqlrcur_affectedRows($cur),1);
	echo("\n");


	# input bind by position
	# sqlite doesn't support bind by position


	# array of input binds by position
	# sqlite doesn't support bind by position


	# input bind by position with validation
	# sqlite doesn't support bind by position


	# input bind by name
	echo("INPUT BIND BY NAME: \n");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	:var1, ".
		"	:var2, ".
		"	:var3, ".
		"	:var4, ".
		"	:var5, ".
		"	:var6)");
	assertEqInt(sqlrcur_countBindVariables($cur),6);
	sqlrcur_inputBind($cur,"var1",5);
	sqlrcur_inputBind($cur,"var2",5.5,4,1);
	sqlrcur_inputBind($cur,"var3","testchar5");
	sqlrcur_inputBind($cur,"var4","testvarchar5");
	sqlrcur_inputBindClob($cur,"var5","testclob5",strlen("testclob5"));
	sqlrcur_inputBindBlob($cur,"var6","testblob5",strlen("testblob5"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",6);
	sqlrcur_inputBind($cur,"var2",6.5,4,1);
	sqlrcur_inputBind($cur,"var3","testchar6");
	sqlrcur_inputBind($cur,"var4","testvarchar6");
	sqlrcur_inputBindClob($cur,"var5","testclob6",strlen("testclob6"));
	sqlrcur_inputBindBlob($cur,"var6","testblob6",strlen("testblob6"));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",7);
	sqlrcur_inputBind($cur,"var2",7.5,4,1);
	sqlrcur_inputBind($cur,"var3","testchar7");
	sqlrcur_inputBind($cur,"var4","testvarchar7");
	sqlrcur_inputBindClob($cur,"var5","testclob7",strlen("testclob7"));
	sqlrcur_inputBindBlob($cur,"var6","testblob7",strlen("testblob7"));
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of input binds by name
	# sqlite doesn't support implicit conversion
	# of string binds to other data types, so
	# arrays of binds don't generally work.


	# input bind by name with validation
	echo("INPUT BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"var1",8);
	sqlrcur_inputBind($cur,"var2",8.5,4,1);
	sqlrcur_inputBind($cur,"var3","testchar8");
	sqlrcur_inputBind($cur,"var4","testvarchar8");
	sqlrcur_inputBindClob($cur,"var5","testclob8",strlen("testclob8"));
	sqlrcur_inputBindBlob($cur,"var6","testblob8",strlen("testblob8"));
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
	assertEqInt(sqlrcur_colCount($cur),6);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testvarchar");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testint");
	assertEqStr($cols[1],"testfloat");
	assertEqStr($cols[2],"testchar");
	assertEqStr($cols[3],"testvarchar");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	if ($issqlite3) {
		assertEqStr(sqlrcur_getColumnType($cur,0),"INTEGER");
		assertEqStr(sqlrcur_getColumnType($cur,"testint"),"INTEGER");
		assertEqStr(sqlrcur_getColumnType($cur,1),"FLOAT");
		assertEqStr(sqlrcur_getColumnType($cur,"testfloat"),"FLOAT");
		assertEqStr(sqlrcur_getColumnType($cur,2),"STRING");
		assertEqStr(sqlrcur_getColumnType($cur,"testchar"),"STRING");
		assertEqStr(sqlrcur_getColumnType($cur,3),"STRING");
		assertEqStr(sqlrcur_getColumnType($cur,"testvarchar"),"STRING");
		assertEqStr(sqlrcur_getColumnType($cur,4),"STRING");
		assertEqStr(sqlrcur_getColumnType($cur,"testclob"),"STRING");
		assertEqStr(sqlrcur_getColumnType($cur,5),"STRING");
		assertEqStr(sqlrcur_getColumnType($cur,"testblob"),"STRING");
	} else {
		assertEqStr(sqlrcur_getColumnType($cur,0),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,"testint"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,1),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,"testfloat"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,2),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,"testchar"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,3),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,"testvarchar"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,4),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,"testclob"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,5),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnType($cur,"testblob"),"UNKNOWN");
	}
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLength($cur,0),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testint"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,1),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testfloat"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,2),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testchar"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,3),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testvarchar"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,4),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testclob"),0);
	assertEqInt(sqlrcur_getColumnLength($cur,5),0);
	assertEqInt(sqlrcur_getColumnLength($cur,"testblob"),0);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongest($cur,0),1);
	assertEqInt(sqlrcur_getLongest($cur,"testint"),1);
	assertEqInt(sqlrcur_getLongest($cur,1),3);
	assertEqInt(sqlrcur_getLongest($cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongest($cur,2),9);
	assertEqInt(sqlrcur_getLongest($cur,"testchar"),9);
	assertEqInt(sqlrcur_getLongest($cur,3),12);
	assertEqInt(sqlrcur_getLongest($cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongest($cur,4),9);
	assertEqInt(sqlrcur_getLongest($cur,"testclob"),9);
	assertEqInt(sqlrcur_getLongest($cur,5),9);
	assertEqInt(sqlrcur_getLongest($cur,"testblob"),9);
	echo("\n");


	# row count
	echo("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount($cur),8);
	echo("\n");


	# total rows
	echo("TOTAL ROWS: \n");
	assertEqInt(sqlrcur_totalRows($cur),($issqlite3)?0:8);
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
	assertEqStr(sqlrcur_getField($cur,0,2),"testchar1");
	assertEqStr(sqlrcur_getField($cur,0,3),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,4),"testclob1");
	assertEqStr(sqlrcur_getField($cur,0,5),"testblob1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,0),"8");
	assertEqStr(sqlrcur_getField($cur,7,1),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,2),"testchar8");
	assertEqStr(sqlrcur_getField($cur,7,3),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,4),"testclob8");
	assertEqStr(sqlrcur_getField($cur,7,5),"testblob8");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,1),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,2),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,3),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,4),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,5),9);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,1),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,2),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,3),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,4),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,5),9);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getField($cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getField($cur,0,"testchar"),"testchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testvarchar"),"testvarchar1");
	assertEqStr(sqlrcur_getField($cur,0,"testclob"),"testclob1");
	assertEqStr(sqlrcur_getField($cur,0,"testblob"),"testblob1");
	echo("\n");
	assertEqStr(sqlrcur_getField($cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getField($cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getField($cur,7,"testchar"),"testchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testvarchar"),"testvarchar8");
	assertEqStr(sqlrcur_getField($cur,7,"testclob"),"testclob8");
	assertEqStr(sqlrcur_getField($cur,7,"testblob"),"testblob8");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testchar"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testclob"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testblob"),9);
	echo("\n");
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testchar"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testclob"),9);
	assertEqInt(sqlrcur_getFieldLength($cur,7,"testblob"),9);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqStr($fields[0],"1");
	assertEqStr($fields[1],"1.5");
	assertEqStr($fields[2],"testchar1");
	assertEqStr($fields[3],"testvarchar1");
	assertEqStr($fields[4],"testclob1");
	assertEqStr($fields[5],"testblob1");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqInt($fieldlens[0],1);
	assertEqInt($fieldlens[1],3);
	assertEqInt($fieldlens[2],9);
	assertEqInt($fieldlens[3],12);
	assertEqInt($fieldlens[4],9);
	assertEqInt($fieldlens[5],9);
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
	assertEqInt(sqlrcur_getColumnLength($cur,0),0);
	assertEqStr(sqlrcur_getColumnType($cur,0),
				($issqlite3)?"INTEGER":"UNKNOWN");
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
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount($cur),6);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName($cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName($cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName($cur,2),"testchar");
	assertEqStr(sqlrcur_getColumnName($cur,3),"testvarchar");
	assertEqStr(sqlrcur_getColumnName($cur,4),"testclob");
	assertEqStr(sqlrcur_getColumnName($cur,5),"testblob");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqStr($cols[0],"testint");
	assertEqStr($cols[1],"testfloat");
	assertEqStr($cols[2],"testchar");
	assertEqStr($cols[3],"testvarchar");
	assertEqStr($cols[4],"testclob");
	assertEqStr($cols[5],"testblob");
	echo("\n");


	# cached result set with result set
	# buffer size
	echo("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
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
	echo("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: \n");
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
	echo("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: \n");
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
	echo("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
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
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable"));
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
	# sqlite DDL is transactional; commit so the table is visible
	# to the second connection (the commit implicitly starts a new tx)
	assertTrue(sqlrcon_commit($con));
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
	assertEqStr(sqlrcon_getTransactionModel($con),"explicit");
	assertTrue(sqlrcon_getAutoCommit($con));
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_sendQuery($cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int, ".
		"	col2 char, ".
		"	col3 float)"));
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	$(var1), ".
		"	'$(var2)', ".
		"	$(var3))");
	sqlrcur_substitution($cur,"var1",1);
	sqlrcur_substitution($cur,"var2","hello");
	sqlrcur_substitution($cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery($cur,"delete from testtable"));
	echo("\n");


	# array substitutions
	echo("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	'$(var1)', ".
		"	'$(var2)', ".
		"	'$(var3)')");
	sqlrcur_substitutions($cur,$subvars,$subvalstrings,NULL,NULL);
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"hi");
	assertEqStr(sqlrcur_getField($cur,0,1),"hello");
	assertEqStr(sqlrcur_getField($cur,0,2),"bye");
	assertTrue(sqlrcur_sendQuery($cur,"delete from testtable"));
	echo("\n");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	$(var1), ".
		"	'$(var2)', ".
		"	$(var3))");
	sqlrcur_substitutions($cur,$subvars,$subvallongs,NULL,NULL);
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"2");
	assertEqStr(sqlrcur_getField($cur,0,2),"3.0");
	assertTrue(sqlrcur_sendQuery($cur,"delete from testtable"));
	echo("\n");
	sqlrcur_prepareQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	$(var1), ".
		"	'$(var2)', ".
		"	$(var3))");
	sqlrcur_substitutions($cur,$subvars,$subvaldoubles,$precs,$scales);
	assertTrue(sqlrcur_executeQuery($cur));
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"10.55");
	assertEqStr(sqlrcur_getField($cur,0,1),"10.556");
	assertEqStr(sqlrcur_getField($cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery($cur,"delete from testtable"));
	echo("\n");


	# nulls as nulls
	echo("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls($cur);
	assertTrue(sqlrcur_sendQuery($cur,
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	NULL, ".
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),NULL);
	assertEqStr(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	assertEqStr(sqlrcur_getField($cur,0,1),"");
	assertEqStr(sqlrcur_getField($cur,0,2),"");
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable"));
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
	sqlrcur_prepareQuery($cur,"insert into testtable ".
		"values (:clobval,:blobval)");
	$largebuffer=str_repeat("C",8192);
	sqlrcur_inputBindClob($cur,"clobval",$largebuffer,strlen($largebuffer));
	sqlrcur_inputBindBlob($cur,"blobval",$largebuffer,strlen($largebuffer));
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testclob"),8192);
	assertEqStr(sqlrcur_getField($cur,0,"testclob"),$largebuffer);
	assertEqInt(sqlrcur_getFieldLength($cur,0,"testblob"),8192);
	assertEqStrLen(sqlrcur_getField($cur,0,"testblob"),$largebuffer,8192);
	assertTrue(sqlrcur_sendQuery($cur,"drop table testtable"));
	echo("\n");


	# output bind by position
	# sqlite doesn't support output binds


	# output bind by name
	# sqlite doesn't support output binds


	# output bind by name with validation
	# sqlite doesn't support output binds


	# lob output bind
	# sqlite doesn't support output binds


	# long output bind
	# sqlite doesn't support output binds


	# negative input bind
	echo("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	sqlrcur_sendQuery($cur,"create table testtable (testval int)");
	sqlrcur_prepareQuery($cur,"insert into testtable values (:testval)");
	sqlrcur_inputBind($cur,"testval",-1);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_sendQuery($cur,"select testval from testtable");
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
		"	$(var1), ".
		"	$(var2), ".
		"	$(var3))");
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
	sqlrcur_prepareQuery($cur,"select :val");
	sqlrcur_inputBind($cur,"val",1);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcur_inputBind($cur,"val",2);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"2");
	sqlrcur_inputBind($cur,"val",3);
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqStr(sqlrcur_getField($cur,0,0),"3");
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
	sqlrcur_prepareQuery($cur,"select :var");
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
	# sqlite doesn't support stored procedures


	# stored procedure returning single value
	# sqlite doesn't support stored procedures


	# stored procedure returning multiple values
	# sqlite doesn't support stored procedures


	# stored procedure returning result set
	# sqlite doesn't support stored procedures


	# temporary tables
	echo("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery($cur,"drop table if exists temptable\n");
	sqlrcur_sendQuery($cur,"create temporary table temptable (col1 int)");
	assertTrue(sqlrcur_sendQuery($cur,"insert into temptable values (1)"));
	assertTrue(sqlrcur_sendQuery($cur,"select count(*) from temptable"));
	assertEqStr(sqlrcur_getField($cur,0,0),"1");
	sqlrcon_endSession($con);
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"select count(*) from temptable"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists temptable\n"));
	echo("\n");


	# encoded binary data
	echo("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery($cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery($cur,"create table testtable ".
		"(col1 blob)"));
	$buffer="";
	for ($j=0; $j<256; $j++) {
		$buffer.=chr($j);
	}
	$querystr="insert into testtable values (X'";
	for ($i=0; $i<256; $i++) {
		$querystr.=sprintf("%02x",ord($buffer[$i]));
	}
	$querystr.="')";
	assertTrue(sqlrcur_sendQuery($cur,$querystr));
	assertTrue(sqlrcur_sendQuery($cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLength($cur,0,0),256);
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
		"	(col1 integer primary key ".
		"	autoincrement, ".
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable ".
		"values (null,1)"));
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
	assertEqInt(sqlrcur_rowCount($cur),0);
	echo("\n");


	# schema list
	echo("SCHEMA LIST: \n");
	assertTrue(sqlrcur_getSchemaList($cur,NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"Database");
	echo("\n");


	# table type list
	echo("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList($cur));
	assertEqStr(sqlrcur_getColumnName($cur,0),"table_type");
	assertInResultSet($cur,"table_type","TABLE");
	echo("\n");


	# table list
	echo("TABLE LIST: \n");
	sqlrcur_sendQuery($cur,"drop table if exists testtable1");
	sqlrcur_sendQuery($cur,"drop table if exists testtable2");
	sqlrcur_sendQuery($cur,"drop table if exists testtable3");
	sqlrcur_sendQuery($cur,"drop table if exists testtable4");
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
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable1"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable2"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable3"));
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable4"));
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
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"19");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"INTEGER");
	assertTrue(sqlrcur_getTypeInfoList($cur,"char"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"2147483647");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"varchar"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"2147483647");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"VARCHAR");
	assertTrue(sqlrcur_getTypeInfoList($cur,"date"));
	assertEqStr(sqlrcur_getField($cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"91");
	assertEqStr(sqlrcur_getField($cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getField($cur,0,"local_type_name"),"DATE");
	echo("\n");


	# column list
	echo("COLUMN LIST: \n");
	sqlrcur_sendQuery($cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	testint int, ".
		"	testfloat float, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
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
	assertEqStr(sqlrcur_getField($cur,0,"column_name"),"testint");
	assertEqStr(sqlrcur_getField($cur,1,"column_name"),"testfloat");
	assertEqStr(sqlrcur_getField($cur,2,"column_name"),"testchar");
	assertEqStr(sqlrcur_getField($cur,3,"column_name"),"testvarchar");
	assertEqStr(sqlrcur_getField($cur,4,"column_name"),"testclob");
	assertEqStr(sqlrcur_getField($cur,5,"column_name"),"testblob");
	assertEqStr(sqlrcur_getField($cur,0,"data_type"),"INT");
	assertEqStr(sqlrcur_getField($cur,1,"data_type"),"FLOAT");
	assertEqStr(sqlrcur_getField($cur,2,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getField($cur,3,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getField($cur,4,"data_type"),"CLOB");
	assertEqStr(sqlrcur_getField($cur,5,"data_type"),"BLOB");
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable"));
	echo("\n");


	# column list - auto_increment,
	# primary key
	echo("COLUMN LIST - auto_increment, primary key: \n");
	sqlrcur_sendQuery($cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 integer primary key ".
		"	autoincrement, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getField($cur,0,"extra"),"auto_increment");
	assertEqStr(sqlrcur_getField($cur,0,"column_key"),"PRI");
	assertEqStr(sqlrcur_getField($cur,1,"extra"),"");
	assertEqStr(sqlrcur_getField($cur,1,"column_key"),"");
	echo("\n");
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable"));
	assertTrue(sqlrcur_sendQuery($cur,
		"create table testtable (".
		"	col1 int primary key, ".
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList($cur,"testtable",NULL));
	assertEqStr(sqlrcur_getField($cur,0,"extra"),"");
	assertEqStr(sqlrcur_getField($cur,0,"column_key"),"PRI");
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable"));
	echo("\n");


	# primary keys list
	echo("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery($cur,"drop table if exists testtable");
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
	assertTrue(strcmp(sqlrcur_getField($cur,0,"table"),"testtable")==0);
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(strcmp(sqlrcur_getField($cur,0,"column_name"),"col1")==0);
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable"));
	echo("\n");


	# key and index list
	echo("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery($cur,"drop table if exists testtable");
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
	assertTrue(strcmp(sqlrcur_getField($cur,0,"table"),"testtable")==0);
	assertEqStr(sqlrcur_getField($cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getField($cur,0,"seq_in_index"),"1");
	assertTrue(strcmp(sqlrcur_getField($cur,0,"column_name"),"col1")==0);
	assertEqStr(sqlrcur_getField($cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getField($cur,0,"index_type"),"3");
	$kn=sqlrcur_getField($cur,0,"key_name");
	assertEqStr($kn,"sqlite_autoindex_testtable_1");
	assertTrue(sqlrcur_sendQuery($cur,"drop table if exists testtable"));
	echo("\n");


	# procedure list
	echo("PROCEDURE LIST: \n");
	assertTrue(sqlrcur_getProcedureList($cur,NULL));
	assertEqInt(sqlrcur_rowCount($cur),0);
	echo("\n");


	# procedure parameter list
	echo("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList($cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName($cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName($cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName($cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName($cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName($cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount($cur),0);
	echo("\n");


	# invalid queries
	echo("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable"));
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
