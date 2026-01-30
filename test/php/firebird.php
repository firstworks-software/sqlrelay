<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("./asserts.php");


	$host="sqlrelay";
	$port=9000;
	$socket="/tmp/test.socket";
	$user="testuser";
	$password="testpassword";

	$subvars=array("var1","var2","var3");
	$subvalstrings=array("hi","hello","bye");
	$subvallongs=array(1,2,3);
	$subvaldoubles=array(10.55,10.556,10.5556);
	$precs=array(4,5,6);
	$scales=array(2,3,4);

	# instantiation
	$con=sqlrcon_alloc($host,$port,$socket,$user,$password,0,1);
	$cur=sqlrcur_alloc($con);

	# get database type


	# identify
	echo("IDENTIFY: \n");
	assertEqual(sqlrcon_identify($con),"firebird");
	echo("\n");


	# ping
	echo("PING: \n");
	assertTrue(sqlrcon_ping($con));
	echo("\n");


	# isolation levels
	echo("ISOLATION LEVELS: \n");
	# though firebird does support a "set transaction ..." statement to
	# set the isolation level, it looks like, in firebird, you can really
	# only set it through the TPB at the start of a transaction, so
	# attempts to set it should fail
	assertFalse(sqlrcon_setIsolationLevel($con,"read committed"));
	assertEqual(sqlrcon_getIsolationLevel($con),"read committed");
	echo("\n");

	# clear table
	sqlrcur_sendQuery($cur,"delete from testtable");
	sqlrcon_commit($con);


	# insert
	echo("INSERT: \n");
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (1,1,1.1,1.1,1.1,1.1,'01-JAN-2001','01:00:00','testchar1','testvarchar1',NULL,NULL)"));
	echo("\n");


	# bind by position
	echo("BIND BY POSITION: \n");
	sqlrcur_prepareQuery($cur,"insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,NULL)");
	assertEqual(sqlrcur_countBindVariables($cur),11);
	sqlrcur_inputBind($cur,"1",2);
	sqlrcur_inputBind($cur,"2",2);
	sqlrcur_inputBind($cur,"3",2.2,2,1);
	sqlrcur_inputBind($cur,"4",2.2,2,1);
	sqlrcur_inputBind($cur,"5",2.2,2,1);
	sqlrcur_inputBind($cur,"6",2.2,2,1);
	sqlrcur_inputBind($cur,"7","01-JAN-2002");
	sqlrcur_inputBind($cur,"8","02:00:00");
	sqlrcur_inputBind($cur,"9","testchar2");
	sqlrcur_inputBind($cur,"10","testvarchar2");
	sqlrcur_inputBind($cur,"11",NULL);
	assertTrue(sqlrcur_executeQuery($cur));
	sqlrcur_clearBinds($cur);
	sqlrcur_inputBind($cur,"1",3);
	sqlrcur_inputBind($cur,"2",3);
	sqlrcur_inputBind($cur,"3",3.3,2,1);
	sqlrcur_inputBind($cur,"4",3.3,2,1);
	sqlrcur_inputBind($cur,"5",3.3,2,1);
	sqlrcur_inputBind($cur,"6",3.3,2,1);
	sqlrcur_inputBind($cur,"7","01-JAN-2003");
	sqlrcur_inputBind($cur,"8","03:00:00");
	sqlrcur_inputBind($cur,"9","testchar3");
	sqlrcur_inputBind($cur,"10","testvarchar3");
	sqlrcur_inputBind($cur,"11",NULL);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# array of binds by position
	echo("ARRAY OF BINDS BY POSITION: \n");
	sqlrcur_clearBinds($cur);
	$bindvars=array("1","2","3","4","5","6",
				"7","8","9","10","11");
	$bindvals=array("4","4","4.4","4.4","4.4","4.4",
				"01-JAN-2004","04:00:00",
				"testchar4","testvarchar4",NULL);
	sqlrcur_inputBinds($cur,$bindvars,$bindvals);
	assertTrue(sqlrcur_executeQuery($cur));
	echo("\n");


	# insert
	echo("INSERT: \n");
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (5,5,5.5,5.5,5.5,5.5,'01-JAN-2005','05:00:00','testchar5','testvarchar5',NULL,NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (6,6,6.6,6.6,6.6,6.6,'01-JAN-2006','06:00:00','testchar6','testvarchar6',NULL,NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (7,7,7.7,7.7,7.7,7.7,'01-JAN-2007','07:00:00','testchar7','testvarchar7',NULL,NULL)"));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (8,8,8.8,8.8,8.8,8.8,'01-JAN-2008','08:00:00','testchar8','testvarchar8',NULL,NULL)"));
	echo("\n");


	# affected rows
	echo("AFFECTED ROWS: \n");
	assertEqual(sqlrcur_affectedRows($cur),0);
	echo("\n");


	# stored procedure
	echo("STORED PROCEDURE: \n");
	sqlrcur_prepareQuery($cur,"select * from testproc(?,?,?,NULL)");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"1.1000");
	assertEqual(sqlrcur_getField($cur,0,2),"hello");
	sqlrcur_prepareQuery($cur,"execute procedure testproc ?, ?, ?, NULL");
	sqlrcur_inputBind($cur,"1",1);
	sqlrcur_inputBind($cur,"2",1.1,2,1);
	sqlrcur_inputBind($cur,"3","hello");
	sqlrcur_defineOutputBindInteger($cur,"1");
	sqlrcur_defineOutputBindDouble($cur,"2");
	sqlrcur_defineOutputBindString($cur,"3",20);
	sqlrcur_defineOutputBindBlob($cur,"4");
	assertTrue(sqlrcur_executeQuery($cur));
	assertEqual(sqlrcur_getOutputBindInteger($cur,"1"),1);
	//assertEqual(sqlrcur_getOutputBindDouble($cur,"2"),1.1);
	assertEqual(sqlrcur_getOutputBindString($cur,"3"),"hello               ");
	echo("\n");


	# select
	echo("SELECT: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
	echo("\n");


	# column count
	echo("COLUMN COUNT: \n");
	assertEqual(sqlrcur_colCount($cur),12);
	echo("\n");


	# column names
	echo("COLUMN NAMES: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"TESTINTEGER");
	assertEqual(sqlrcur_getColumnName($cur,1),"TESTSMALLINT");
	assertEqual(sqlrcur_getColumnName($cur,2),"TESTDECIMAL");
	assertEqual(sqlrcur_getColumnName($cur,3),"TESTNUMERIC");
	assertEqual(sqlrcur_getColumnName($cur,4),"TESTFLOAT");
	assertEqual(sqlrcur_getColumnName($cur,5),"TESTDOUBLE");
	assertEqual(sqlrcur_getColumnName($cur,6),"TESTDATE");
	assertEqual(sqlrcur_getColumnName($cur,7),"TESTTIME");
	assertEqual(sqlrcur_getColumnName($cur,8),"TESTCHAR");
	assertEqual(sqlrcur_getColumnName($cur,9),"TESTVARCHAR");
	assertEqual(sqlrcur_getColumnName($cur,10),"TESTTIMESTAMP");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"TESTINTEGER");
	assertEqual($cols[1],"TESTSMALLINT");
	assertEqual($cols[2],"TESTDECIMAL");
	assertEqual($cols[3],"TESTNUMERIC");
	assertEqual($cols[4],"TESTFLOAT");
	assertEqual($cols[5],"TESTDOUBLE");
	assertEqual($cols[6],"TESTDATE");
	assertEqual($cols[7],"TESTTIME");
	assertEqual($cols[8],"TESTCHAR");
	assertEqual($cols[9],"TESTVARCHAR");
	assertEqual($cols[10],"TESTTIMESTAMP");
	echo("\n");


	# column types
	echo("COLUMN TYPES: \n");
	assertEqual(sqlrcur_getColumnType($cur,0),"INTEGER");
	assertEqual(sqlrcur_getColumnType($cur,"TESTINTEGER"),"INTEGER");
	assertEqual(sqlrcur_getColumnType($cur,1),"SMALLINT");
	assertEqual(sqlrcur_getColumnType($cur,"TESTSMALLINT"),"SMALLINT");
	assertEqual(sqlrcur_getColumnType($cur,2),"DECIMAL");
	assertEqual(sqlrcur_getColumnType($cur,"TESTDECIMAL"),"DECIMAL");
	assertEqual(sqlrcur_getColumnType($cur,3),"NUMERIC");
	assertEqual(sqlrcur_getColumnType($cur,"TESTNUMERIC"),"NUMERIC");
	assertEqual(sqlrcur_getColumnType($cur,4),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,"TESTFLOAT"),"FLOAT");
	assertEqual(sqlrcur_getColumnType($cur,5),"DOUBLE PRECISION");
	assertEqual(sqlrcur_getColumnType($cur,"TESTDOUBLE"),"DOUBLE PRECISION");
	assertEqual(sqlrcur_getColumnType($cur,6),"DATE");
	assertEqual(sqlrcur_getColumnType($cur,"TESTDATE"),"DATE");
	assertEqual(sqlrcur_getColumnType($cur,7),"TIME");
	assertEqual(sqlrcur_getColumnType($cur,"TESTTIME"),"TIME");
	assertEqual(sqlrcur_getColumnType($cur,8),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,"TESTCHAR"),"CHAR");
	assertEqual(sqlrcur_getColumnType($cur,9),"VARCHAR");
	assertEqual(sqlrcur_getColumnType($cur,"TESTVARCHAR"),"VARCHAR");
	assertEqual(sqlrcur_getColumnType($cur,10),"TIMESTAMP");
	assertEqual(sqlrcur_getColumnType($cur,"TESTTIMESTAMP"),"TIMESTAMP");
	echo("\n");


	# column length
	echo("COLUMN LENGTH: \n");
	assertEqual(sqlrcur_getColumnLength($cur,0),4);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTINTEGER"),4);
	assertEqual(sqlrcur_getColumnLength($cur,1),2);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTSMALLINT"),2);
	assertEqual(sqlrcur_getColumnLength($cur,2),8);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTDECIMAL"),8);
	assertEqual(sqlrcur_getColumnLength($cur,3),8);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTNUMERIC"),8);
	assertEqual(sqlrcur_getColumnLength($cur,4),4);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTFLOAT"),4);
	assertEqual(sqlrcur_getColumnLength($cur,5),8);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTDOUBLE"),8);
	assertEqual(sqlrcur_getColumnLength($cur,6),4);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTDATE"),4);
	assertEqual(sqlrcur_getColumnLength($cur,7),4);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTTIME"),4);
	assertEqual(sqlrcur_getColumnLength($cur,8),50);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTCHAR"),50);
	assertEqual(sqlrcur_getColumnLength($cur,9),50);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTVARCHAR"),50);
	assertEqual(sqlrcur_getColumnLength($cur,10),8);
	assertEqual(sqlrcur_getColumnLength($cur,"TESTTIMESTAMP"),8);
	echo("\n");


	# longest column
	echo("LONGEST COLUMN: \n");
	assertEqual(sqlrcur_getLongest($cur,0),1);
	assertEqual(sqlrcur_getLongest($cur,"TESTINTEGER"),1);
	assertEqual(sqlrcur_getLongest($cur,1),1);
	assertEqual(sqlrcur_getLongest($cur,"TESTSMALLINT"),1);
	assertEqual(sqlrcur_getLongest($cur,2),4);
	assertEqual(sqlrcur_getLongest($cur,"TESTDECIMAL"),4);
	assertEqual(sqlrcur_getLongest($cur,3),4);
	assertEqual(sqlrcur_getLongest($cur,"TESTNUMERIC"),4);
	assertEqual(sqlrcur_getLongest($cur,4),6);
	assertEqual(sqlrcur_getLongest($cur,"TESTFLOAT"),6);
	assertEqual(sqlrcur_getLongest($cur,5),6);
	assertEqual(sqlrcur_getLongest($cur,"TESTDOUBLE"),6);
	assertEqual(sqlrcur_getLongest($cur,6),10);
	assertEqual(sqlrcur_getLongest($cur,"TESTDATE"),10);
	assertEqual(sqlrcur_getLongest($cur,7),8);
	assertEqual(sqlrcur_getLongest($cur,"TESTTIME"),8);
	assertEqual(sqlrcur_getLongest($cur,8),50);
	assertEqual(sqlrcur_getLongest($cur,"TESTCHAR"),50);
	assertEqual(sqlrcur_getLongest($cur,9),12);
	assertEqual(sqlrcur_getLongest($cur,"TESTVARCHAR"),12);
	assertEqual(sqlrcur_getLongest($cur,10),0);
	assertEqual(sqlrcur_getLongest($cur,"TESTTIMESTAMP"),0);
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
	assertEqual(sqlrcur_getField($cur,0,2),"1.10");
	assertEqual(sqlrcur_getField($cur,0,3),"1.10");
	assertEqual(sqlrcur_getField($cur,0,4),"1.1000");
	assertEqual(sqlrcur_getField($cur,0,5),"1.1000");
	assertEqual(sqlrcur_getField($cur,0,6),"2001:01:01");
	assertEqual(sqlrcur_getField($cur,0,7),"01:00:00");
	assertEqual(sqlrcur_getField($cur,0,8),"testchar1                                         ");
	assertEqual(sqlrcur_getField($cur,0,9),"testvarchar1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	assertEqual(sqlrcur_getField($cur,7,1),"8");
	assertEqual(sqlrcur_getField($cur,7,2),"8.80");
	assertEqual(sqlrcur_getField($cur,7,3),"8.80");
	assertEqual(sqlrcur_getField($cur,7,4),"8.8000");
	assertEqual(sqlrcur_getField($cur,7,5),"8.8000");
	assertEqual(sqlrcur_getField($cur,7,6),"2008:01:01");
	assertEqual(sqlrcur_getField($cur,7,7),"08:00:00");
	assertEqual(sqlrcur_getField($cur,7,8),"testchar8                                         ");
	assertEqual(sqlrcur_getField($cur,7,9),"testvarchar8");
	echo("\n");


	# field lengths by index
	echo("FIELD LENGTHS BY INDEX: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,1),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,2),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,3),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,4),6);
	assertEqual(sqlrcur_getFieldLength($cur,0,5),6);
	assertEqual(sqlrcur_getFieldLength($cur,0,6),10);
	assertEqual(sqlrcur_getFieldLength($cur,0,7),8);
	assertEqual(sqlrcur_getFieldLength($cur,0,8),50);
	assertEqual(sqlrcur_getFieldLength($cur,0,9),12);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,0),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,1),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,2),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,3),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,4),6);
	assertEqual(sqlrcur_getFieldLength($cur,7,5),6);
	assertEqual(sqlrcur_getFieldLength($cur,7,6),10);
	assertEqual(sqlrcur_getFieldLength($cur,7,7),8);
	assertEqual(sqlrcur_getFieldLength($cur,7,8),50);
	assertEqual(sqlrcur_getFieldLength($cur,7,9),12);
	echo("\n");


	# fields by name
	echo("FIELDS BY NAME: \n");
	assertEqual(sqlrcur_getField($cur,0,"TESTINTEGER"),"1");
	assertEqual(sqlrcur_getField($cur,0,"TESTSMALLINT"),"1");
	assertEqual(sqlrcur_getField($cur,0,"TESTDECIMAL"),"1.10");
	assertEqual(sqlrcur_getField($cur,0,"TESTNUMERIC"),"1.10");
	assertEqual(sqlrcur_getField($cur,0,"TESTFLOAT"),"1.1000");
	assertEqual(sqlrcur_getField($cur,0,"TESTDOUBLE"),"1.1000");
	assertEqual(sqlrcur_getField($cur,0,"TESTDATE"),"2001:01:01");
	assertEqual(sqlrcur_getField($cur,0,"TESTTIME"),"01:00:00");
	assertEqual(sqlrcur_getField($cur,0,"TESTCHAR"),"testchar1                                         ");
	assertEqual(sqlrcur_getField($cur,0,"TESTVARCHAR"),"testvarchar1");
	echo("\n");
	assertEqual(sqlrcur_getField($cur,7,"TESTINTEGER"),"8");
	assertEqual(sqlrcur_getField($cur,7,"TESTSMALLINT"),"8");
	assertEqual(sqlrcur_getField($cur,7,"TESTDECIMAL"),"8.80");
	assertEqual(sqlrcur_getField($cur,7,"TESTNUMERIC"),"8.80");
	assertEqual(sqlrcur_getField($cur,7,"TESTFLOAT"),"8.8000");
	assertEqual(sqlrcur_getField($cur,7,"TESTDOUBLE"),"8.8000");
	assertEqual(sqlrcur_getField($cur,7,"TESTDATE"),"2008:01:01");
	assertEqual(sqlrcur_getField($cur,7,"TESTTIME"),"08:00:00");
	assertEqual(sqlrcur_getField($cur,7,"TESTCHAR"),"testchar8                                         ");
	assertEqual(sqlrcur_getField($cur,7,"TESTVARCHAR"),"testvarchar8");
	echo("\n");


	# field lengths by name
	echo("FIELD LENGTHS BY NAME: \n");
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTINTEGER"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTSMALLINT"),1);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTDECIMAL"),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTNUMERIC"),4);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTFLOAT"),6);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTDOUBLE"),6);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTDATE"),10);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTTIME"),8);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTCHAR"),50);
	assertEqual(sqlrcur_getFieldLength($cur,0,"TESTVARCHAR"),12);
	echo("\n");
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTINTEGER"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTSMALLINT"),1);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTDECIMAL"),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTNUMERIC"),4);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTFLOAT"),6);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTDOUBLE"),6);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTDATE"),10);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTTIME"),8);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTCHAR"),50);
	assertEqual(sqlrcur_getFieldLength($cur,7,"TESTVARCHAR"),12);
	echo("\n");


	# fields by array
	echo("FIELDS BY ARRAY: \n");
	$fields=sqlrcur_getRow($cur,0);
	assertEqual($fields[0],"1");
	assertEqual($fields[1],"1");
	assertEqual($fields[2],"1.10");
	assertEqual($fields[3],"1.10");
	assertEqual($fields[4],"1.1000");
	assertEqual($fields[5],"1.1000");
	assertEqual($fields[6],"2001:01:01");
	assertEqual($fields[7],"01:00:00");
	assertEqual($fields[8],"testchar1                                         ");
	assertEqual($fields[9],"testvarchar1");
	echo("\n");


	# field lengths by array
	echo("FIELD LENGTHS BY ARRAY: \n");
	$fieldlens=sqlrcur_getRowLengths($cur,0);
	assertEqual($fieldlens[0],1);
	assertEqual($fieldlens[1],1);
	assertEqual($fieldlens[2],4);
	assertEqual($fieldlens[3],4);
	assertEqual($fieldlens[4],6);
	assertEqual($fieldlens[5],6);
	assertEqual($fieldlens[6],10);
	assertEqual($fieldlens[7],8);
	assertEqual($fieldlens[8],50);
	assertEqual($fieldlens[9],12);
	echo("\n");


	# fields by associative array
	echo("FIELDS BY ASSOCIATIVE ARRAY: \n");
	$fields=sqlrcur_getRowAssoc($cur,0);
	assertEqual($fields["TESTINTEGER"],"1");
	assertEqual($fields["TESTSMALLINT"],"1");
	assertEqual($fields["TESTDECIMAL"],"1.10");
	assertEqual($fields["TESTNUMERIC"],"1.10");
	assertEqual($fields["TESTFLOAT"],"1.1000");
	assertEqual($fields["TESTDOUBLE"],"1.1000");
	assertEqual($fields["TESTDATE"],"2001:01:01");
	assertEqual($fields["TESTTIME"],"01:00:00");
	assertEqual($fields["TESTCHAR"],"testchar1                                         ");
	assertEqual($fields["TESTVARCHAR"],"testvarchar1");
	echo("\n");
	$fields=sqlrcur_getRowAssoc($cur,7);
	assertEqual($fields["TESTINTEGER"],"8");
	assertEqual($fields["TESTSMALLINT"],"8");
	assertEqual($fields["TESTDECIMAL"],"8.80");
	assertEqual($fields["TESTNUMERIC"],"8.80");
	assertEqual($fields["TESTFLOAT"],"8.8000");
	assertEqual($fields["TESTDOUBLE"],"8.8000");
	assertEqual($fields["TESTDATE"],"2008:01:01");
	assertEqual($fields["TESTTIME"],"08:00:00");
	assertEqual($fields["TESTCHAR"],"testchar8                                         ");
	assertEqual($fields["TESTVARCHAR"],"testvarchar8");
	echo("\n");


	# field lengths by associative array
	echo("FIELD LENGTHS BY ASSOCIATIVE ARRAY: \n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,0);
	assertEqual($fieldlengths["TESTINTEGER"],1);
	assertEqual($fieldlengths["TESTSMALLINT"],1);
	assertEqual($fieldlengths["TESTDECIMAL"],4);
	assertEqual($fieldlengths["TESTNUMERIC"],4);
	assertEqual($fieldlengths["TESTFLOAT"],6);
	assertEqual($fieldlengths["TESTDOUBLE"],6);
	assertEqual($fieldlengths["TESTDATE"],10);
	assertEqual($fieldlengths["TESTTIME"],8);
	assertEqual($fieldlengths["TESTCHAR"],50);
	assertEqual($fieldlengths["TESTVARCHAR"],12);
	echo("\n");
	$fieldlengths=sqlrcur_getRowLengthsAssoc($cur,7);
	assertEqual($fieldlengths["TESTINTEGER"],1);
	assertEqual($fieldlengths["TESTSMALLINT"],1);
	assertEqual($fieldlengths["TESTDECIMAL"],4);
	assertEqual($fieldlengths["TESTNUMERIC"],4);
	assertEqual($fieldlengths["TESTFLOAT"],6);
	assertEqual($fieldlengths["TESTDOUBLE"],6);
	assertEqual($fieldlengths["TESTDATE"],10);
	assertEqual($fieldlengths["TESTTIME"],8);
	assertEqual($fieldlengths["TESTCHAR"],50);
	assertEqual($fieldlengths["TESTVARCHAR"],12);
	echo("\n");


	# individual substitutions
	echo("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery($cur,"select $(var1),'$(var2)',$(var3) from rdb\$database");
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
	sqlrcur_prepareQuery($cur,"select $(var1),'$(var2)',$(var3) from rdb\$database");
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
	assertTrue(sqlrcur_sendQuery($cur,"select 1,NULL,NULL from rdb\$database"));
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),NULL);
	assertEqual(sqlrcur_getField($cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select 1,NULL,NULL from rdb\$database"));
	assertEqual(sqlrcur_getField($cur,0,0),"1");
	assertEqual(sqlrcur_getField($cur,0,1),"");
	assertEqual(sqlrcur_getField($cur,0,2),"");
	sqlrcur_getNullsAsNulls($cur);
	echo("\n");


	# result set buffer size
	echo("RESULT SET BUFFER SIZE: \n");
	assertEqual(sqlrcur_getResultSetBufferSize($cur),0);
	sqlrcur_setResultSetBufferSize($cur,2);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
	assertEqual(sqlrcur_getColumnName($cur,0),NULL);
	assertEqual(sqlrcur_getColumnLength($cur,0),0);
	assertEqual(sqlrcur_getColumnType($cur,0),NULL);
	sqlrcur_getColumnInfo($cur);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
	assertEqual(sqlrcur_getColumnName($cur,0),"TESTINTEGER");
	assertEqual(sqlrcur_getColumnLength($cur,0),4);
	assertEqual(sqlrcur_getColumnType($cur,0),"INTEGER");
	echo("\n");


	# suspended session
	echo("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
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
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
	$filename=sqlrcur_getCacheFileName($cur);
	assertEqual($filename,"cachefile1");
	sqlrcur_cacheOff($cur);
	assertTrue(sqlrcur_openCachedResultSet($cur,$filename));
	assertEqual(sqlrcur_getField($cur,7,0),"8");
	echo("\n");


	# column count for cached result set
	echo("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_colCount($cur),12);
	echo("\n");


	# column names for cached result set
	echo("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqual(sqlrcur_getColumnName($cur,0),"TESTINTEGER");
	assertEqual(sqlrcur_getColumnName($cur,1),"TESTSMALLINT");
	assertEqual(sqlrcur_getColumnName($cur,2),"TESTDECIMAL");
	assertEqual(sqlrcur_getColumnName($cur,3),"TESTNUMERIC");
	assertEqual(sqlrcur_getColumnName($cur,4),"TESTFLOAT");
	assertEqual(sqlrcur_getColumnName($cur,5),"TESTDOUBLE");
	assertEqual(sqlrcur_getColumnName($cur,6),"TESTDATE");
	assertEqual(sqlrcur_getColumnName($cur,7),"TESTTIME");
	assertEqual(sqlrcur_getColumnName($cur,8),"TESTCHAR");
	assertEqual(sqlrcur_getColumnName($cur,9),"TESTVARCHAR");
	assertEqual(sqlrcur_getColumnName($cur,10),"TESTTIMESTAMP");
	$cols=sqlrcur_getColumnNames($cur);
	assertEqual($cols[0],"TESTINTEGER");
	assertEqual($cols[1],"TESTSMALLINT");
	assertEqual($cols[2],"TESTDECIMAL");
	assertEqual($cols[3],"TESTNUMERIC");
	assertEqual($cols[4],"TESTFLOAT");
	assertEqual($cols[5],"TESTDOUBLE");
	assertEqual($cols[6],"TESTDATE");
	assertEqual($cols[7],"TESTTIME");
	assertEqual($cols[8],"TESTCHAR");
	assertEqual($cols[9],"TESTVARCHAR");
	assertEqual($cols[10],"TESTTIMESTAMP");
	echo("\n");


	# cached result set with result set buffer size
	echo("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
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


	# cached result set with suspend and result set buffer size
	echo("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize($cur,2);
	sqlrcur_cacheToFile($cur,"cachefile1");
	sqlrcur_setCacheTtl($cur,200);
	assertTrue(sqlrcur_sendQuery($cur,"select * from testtable order by testinteger"));
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

	#echo("COMMIT AND ROLLBACK: \n");
	$secondcon=sqlrcon_alloc($host,
				$port, 
				$socket,$user,$password,0,1);
	$secondcur=sqlrcur_alloc($secondcon);
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqual(sqlrcur_getField($secondcur,0,0),"0");
	assertTrue(sqlrcon_commit($con));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqual(sqlrcur_getField($secondcur,0,0),"8");
	assertTrue(sqlrcon_autoCommitOn($con));
	assertTrue(sqlrcur_sendQuery($cur,"insert into testtable values (10,10,10.1,10.1,10.1,10.1,'01-JAN-2010','10:00:00','testchar10','testvarchar10',NULL,NULL)"));
	assertTrue(sqlrcur_sendQuery($secondcur,"select count(*) from testtable"));
	assertEqual(sqlrcur_getField($secondcur,0,0),"9");
	assertTrue(sqlrcon_autoCommitOff($con));
	echo("\n");

	# drop existing table
	sqlrcon_commit($con);
	sqlrcur_sendQuery($cur,"delete from testtable");
	sqlrcon_commit($con);
	echo("\n");

	# invalid queries...


	# invalid queries
	echo("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable1 order by testinteger"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable1 order by testinteger"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable1 order by testinteger"));
	assertFalse(sqlrcur_sendQuery($cur,"select * from testtable1 order by testinteger"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable1 values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable1 values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable1 values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery($cur,"insert into testtable1 values (1,2,3,4)"));
	echo("\n");
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery($cur,"create table testtable"));
	echo("\n");

	reportTestStatus();

	exit($status);
?></pre></html>
