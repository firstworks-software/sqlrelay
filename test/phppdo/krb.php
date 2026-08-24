<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("asserts.php");

	$host="sqlrelay";
	$port=9013;
	$socket="/tmp/krb.socket";
	$user="";
	$password="";
	$dsn = "sqlrelay:host=$host;port=$port;socket=$socket;tries=0;retrytime=1;krb=yes;debug=0";


	# instantiation
	$dbh=new PDO($dsn,$user,$password);
	if(!$dbh){
		die("new PDO failed");
	}

	# drop existing table
	try {
		$dbh->exec("drop table testtable");
	} catch (Exception $e) {
	}


	# create temptable


	echo("CREATE TEMPTABLE: \n");
	assertEqual($dbh->exec(
		"create table testtable (".
		"	testnumber number, ".
		"	testchar char(40), ".
		"	testvarchar varchar(40), ".
		"	testdate date, ".
		"	testlong long, ".
		"	testclob clob, ".
		"	testblob blob)"),0);
	echo("\n");


	# insert


	echo("INSERT: \n");
	assertEqual($dbh->exec(
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	'testchar1', ".
		"	'testvarchar1', ".
		"	'01-JAN-2001', ".
		"	'testlong1', ".
		"	'testclob1', ".
		"	empty_blob())"),1);
	echo("\n");


	# last insert id


	echo("LAST INSERT ID: \n");
	assertEqual(intval($dbh->lastInsertId()),0);
	echo("\n");

	# doesn't work with oracle unless translatebindvariables="yes" is set


	# bind by position


	echo("BIND BY POSITION: \n");
	$stmt=$dbh->prepare(
		"insert into ".
		"	testtable ".
		"values (".
		"	:1, ".
		"	:2, ".
		"	:3, ".
		"	:4, ".
		"	:5, ".
		"	:6, ".
		"	:7)");
	assertTrue($stmt->bindValue(1,2,PDO::PARAM_INT));
	assertTrue($stmt->bindValue(2,"testchar2"));
	assertTrue($stmt->bindValue(3,"testvarchar2"));
	assertTrue($stmt->bindValue(4,"01-JAN-2002"));
	assertTrue($stmt->bindValue(5,"testlong2"));
	assertTrue($stmt->bindValue(6,"testclob2"));
	assertTrue($stmt->bindValue(7,"testblob2",PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	$param1=3;
	$param2="testchar3";
	$param3="testvarchar3";
	$param4="01-JAN-2003";
	$param5="testlong3";
	$param6="testclob3";
	$param7="testblob3";
	assertTrue($stmt->bindParam(1,$param1,PDO::PARAM_INT));
	assertTrue($stmt->bindParam(2,$param2));
	assertTrue($stmt->bindParam(3,$param3));
	assertTrue($stmt->bindParam(4,$param4));
	assertTrue($stmt->bindValue(5,$param5));
	assertTrue($stmt->bindValue(6,$param6));
	assertTrue($stmt->bindValue(7,$param7,PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	echo("\n");


	# row count


	echo("ROW COUNT: \n");
	assertEqual($stmt->rowCount(),1);
	echo("\n");


	# array of binds by position


	echo("ARRAY OF BINDS BY POSITION: \n");
	$stmt=$dbh->prepare(
		"insert into ".
		"	testtable ".
		"values (".
		"	:1, ".
		"	:2, ".
		"	:3, ".
		"	:4, ".
		"	:5, ".
		"	:6, ".
		"	empty_blob())");
	$param1=4;
	$param2="testchar4";
	$param3="testvarchar4";
	$param4="01-JAN-2004";
	$param5="testlong4";
	$param6="testclob4";
	assertTrue($stmt->execute(array($param1,$param2,$param3,$param4,$param5,$param6)));
	echo("\n");


	# bind by name


	echo("BIND BY NAME: \n");
	$stmt=$dbh->prepare(
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
	assertTrue($stmt->bindValue("var1",5,PDO::PARAM_INT));
	assertTrue($stmt->bindValue("var2","testchar5"));
	assertTrue($stmt->bindValue("var3","testvarchar5"));
	assertTrue($stmt->bindValue("var4","01-JAN-2005"));
	assertTrue($stmt->bindValue("var5","testlong5"));
	assertTrue($stmt->bindValue("var6","testclob5"));
	assertTrue($stmt->bindValue("var7","testblob5",PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	$param1=6;
	$param2="testchar6";
	$param3="testvarchar6";
	$param4="01-JAN-2006";
	$param5="testlong6";
	$param6="testclob6";
	$param7="testblob6";
	assertTrue($stmt->bindParam("var1",$param1,PDO::PARAM_INT));
	assertTrue($stmt->bindParam("var2",$param2));
	assertTrue($stmt->bindParam("var3",$param3));
	assertTrue($stmt->bindParam("var4",$param4));
	assertTrue($stmt->bindValue("var5",$param5));
	assertTrue($stmt->bindValue("var6",$param6));
	assertTrue($stmt->bindValue("var7",$param7,PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	echo("\n");


	# array of binds by name


	echo("ARRAY OF BINDS BY NAME: \n");
	$stmt=$dbh->prepare(
		"insert into ".
		"	testtable ".
		"values (".
		"	:var1, ".
		"	:var2, ".
		"	:var3, ".
		"	:var4, ".
		"	:var5, ".
		"	:var6, ".
		"	empty_blob())");
	$param1=7;
	$param2="testchar7";
	$param3="testvarchar7";
	$param4="01-JAN-2007";
	$param5="testlong7";
	$param6="testclob7";
	assertTrue($stmt->execute(array("var1"=>$param1,"var2"=>$param2,"var3"=>$param3,"var4"=>$param4,"var5"=>$param5,"var6"=>$param6)));
	echo("\n");


	# select


	echo("SELECT: \n");
	$stmt=$dbh->query("select * from testtable order by testnumber");
	echo("\n");


	# column count


	echo("COLUMN COUNT: \n");
	assertEqual($stmt->columnCount(),7);
	$meta0=$stmt->getColumnMeta(0);
	$meta1=$stmt->getColumnMeta(1);
	$meta2=$stmt->getColumnMeta(2);
	$meta3=$stmt->getColumnMeta(3);
	$meta4=$stmt->getColumnMeta(4);
	$meta5=$stmt->getColumnMeta(5);
	$meta6=$stmt->getColumnMeta(6);
	echo("\n");


	# column names


	echo("COLUMN NAMES: \n");
	assertEqual($meta0["name"],"TESTNUMBER");
	assertEqual($meta1["name"],"TESTCHAR");
	assertEqual($meta2["name"],"TESTVARCHAR");
	assertEqual($meta3["name"],"TESTDATE");
	assertEqual($meta4["name"],"TESTLONG");
	assertEqual($meta5["name"],"TESTCLOB");
	assertEqual($meta6["name"],"TESTBLOB");
	echo("\n");


	# column types


	echo("COLUMN TYPES: \n");
	assertEqual($meta0["native_type"],"NUMBER");
	assertEqual($meta1["native_type"],"CHAR");
	assertEqual($meta2["native_type"],"VARCHAR2");
	assertEqual($meta3["native_type"],"DATE");
	assertEqual($meta4["native_type"],"LONG");
	assertEqual($meta5["native_type"],"CLOB");
	assertEqual($meta6["native_type"],"BLOB");
	assertEqual($meta0["pdo_type"],PDO::PARAM_INT);
	assertEqual($meta1["pdo_type"],PDO::PARAM_STR);
	assertEqual($meta2["pdo_type"],PDO::PARAM_STR);
	assertEqual($meta3["pdo_type"],PDO::PARAM_STR);
	assertEqual($meta4["pdo_type"],PDO::PARAM_LOB);
	assertEqual($meta5["pdo_type"],PDO::PARAM_LOB);
	assertEqual($meta6["pdo_type"],PDO::PARAM_LOB);
	echo("\n");


	# column types


	echo("COLUMN TYPES: \n");
	assertEqual($meta0["len"],22);
	assertEqual($meta1["len"],40);
	assertEqual($meta2["len"],40);
	assertEqual($meta3["len"],7);
	assertEqual($meta4["len"],0);
	assertEqual($meta5["len"],0);
	assertEqual($meta6["len"],0);
	echo("\n");


	# fields by index


	echo("FIELDS BY INDEX: \n");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],1);
	assertEqual($result[1],"testchar1                               ");
	assertEqual($result[2],"testvarchar1");
	assertEqual($result[3],"01-JAN-01");
	assertEqual(stream_get_contents($result[4]),"testlong1");
	assertEqual(stream_get_contents($result[5]),"testclob1");
	assertEqual(stream_get_contents($result[6]),"");
	echo("\n");


	# fields by name


	echo("FIELDS BY NAME: \n");
	$result=$stmt->fetch(PDO::FETCH_ASSOC);
	assertEqual($result["TESTNUMBER"],2);
	assertEqual($result["TESTCHAR"],"testchar2                               ");
	assertEqual($result["TESTVARCHAR"],"testvarchar2");
	assertEqual($result["TESTDATE"],"01-JAN-02");
	assertEqual(stream_get_contents($result["TESTLONG"]),"testlong2");
	assertEqual(stream_get_contents($result["TESTCLOB"]),"testclob2");
	assertEqual(stream_get_contents($result["TESTBLOB"]),"testblob2");
	echo("\n");


	# fields by name and index


	echo("FIELDS BY NAME AND INDEX: \n");
	$result=$stmt->fetch();
	assertEqual($result[0],3);
	assertEqual($result[1],"testchar3                               ");
	assertEqual($result[2],"testvarchar3");
	assertEqual($result[3],"01-JAN-03");
	assertEqual(stream_get_contents($result[4]),"testlong3");
	rewind($result[4]);
	assertEqual(stream_get_contents($result[5]),"testclob3");
	rewind($result[5]);
	assertEqual(stream_get_contents($result[6]),"testblob3");
	rewind($result[6]);
	assertEqual($result["TESTNUMBER"],3);
	assertEqual($result["TESTCHAR"],"testchar3                               ");
	assertEqual($result["TESTVARCHAR"],"testvarchar3");
	assertEqual($result["TESTDATE"],"01-JAN-03");
	assertEqual(stream_get_contents($result["TESTLONG"]),"testlong3");
	assertEqual(stream_get_contents($result["TESTCLOB"]),"testclob3");
	assertEqual(stream_get_contents($result["TESTBLOB"]),"testblob3");
	echo("\n");


	# fetch column


	echo("FETCH COLUMN: \n");
	assertEqual($stmt->fetchColumn(0),"4");
	assertEqual($stmt->fetchColumn(0),"5");
	assertEqual($stmt->fetchColumn(0),"6");
	echo("\n");


	# fetch all


	echo("FETCH ALL: \n");
	$stmt=$dbh->query("select * from testtable order by testnumber");
	$result=$stmt->fetchAll();
	assertEqual($result[0][0],1);
	assertEqual($result[1][0],2);
	assertEqual($result[2][0],3);
	assertEqual($result[3][0],4);
	assertEqual($result[4][0],5);
	assertEqual($result[5][0],6);
	assertEqual($result[6][0],7);
	assertEqual($result[0][2],"testvarchar1");
	assertEqual($result[1][2],"testvarchar2");
	assertEqual($result[2][2],"testvarchar3");
	assertEqual($result[3][2],"testvarchar4");
	assertEqual($result[4][2],"testvarchar5");
	assertEqual($result[5][2],"testvarchar6");
	assertEqual($result[6][2],"testvarchar7");
	assertEqual($result[0]["TESTNUMBER"],1);
	assertEqual($result[1]["TESTNUMBER"],2);
	assertEqual($result[2]["TESTNUMBER"],3);
	assertEqual($result[3]["TESTNUMBER"],4);
	assertEqual($result[4]["TESTNUMBER"],5);
	assertEqual($result[5]["TESTNUMBER"],6);
	assertEqual($result[6]["TESTNUMBER"],7);
	assertEqual($result[0]["TESTVARCHAR"],"testvarchar1");
	assertEqual($result[1]["TESTVARCHAR"],"testvarchar2");
	assertEqual($result[2]["TESTVARCHAR"],"testvarchar3");
	assertEqual($result[3]["TESTVARCHAR"],"testvarchar4");
	assertEqual($result[4]["TESTVARCHAR"],"testvarchar5");
	assertEqual($result[5]["TESTVARCHAR"],"testvarchar6");
	assertEqual($result[6]["TESTVARCHAR"],"testvarchar7");
	echo("\n");


	# fetch object


	echo("FETCH OBJECT: \n");
	$stmt=$dbh->query("select * from testtable order by testnumber");
	$result=$stmt->fetchObject();
	assertEqual($result->TESTNUMBER,1);
	assertEqual($result->TESTCHAR,"testchar1                               ");
	assertEqual($result->TESTVARCHAR,"testvarchar1");
	assertEqual($result->TESTDATE,"01-JAN-01");
	assertEqual(stream_get_contents($result->TESTLONG),"testlong1");
	assertEqual(stream_get_contents($result->TESTCLOB),"testclob1");
	assertEqual(stream_get_contents($result->TESTBLOB),"");
	echo("\n");


	# fetch orientations


	echo("FETCH ORIENTATIONS: \n");
	$stmt=$dbh->query("select * from testtable order by testnumber");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"1");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_FIRST);
	assertEqual($result[0],"1");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_NEXT);
	assertEqual($result[0],"2");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_PRIOR);
	assertEqual($result[0],"1");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_LAST);
	assertEqual($result[0],"7");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,3);
	assertEqual($result[0],"4");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,4);
	assertEqual($result[0],"5");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,5);
	assertEqual($result[0],"6");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_REL,1);
	assertEqual($result[0],"7");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_REL,-1);
	assertEqual($result[0],"6");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_REL,-1);
	assertEqual($result[0],"5");
	echo("\n");


	# fetch forward only


	echo("FETCH FORWARD ONLY: \n");
	$stmt=$dbh->prepare("select * from testtable order by testnumber",
				array(PDO::ATTR_CURSOR=>PDO::CURSOR_FWDONLY));
	assertTrue($stmt->execute());
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_FIRST);
	assertEqual($result[0],"1");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_FIRST);
	assertFalse($result);
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,1);
	assertEqual($result[0],"2");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,1);
	assertFalse($result);
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_REL,1);
	assertEqual($result[0],"3");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_REL,2);
	assertEqual($result[0],"5");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_REL,-1);
	assertFalse($result);
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_PRIOR);
	assertFalse($result);
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_NEXT);
	assertEqual($result[0],"6");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_LAST);
	assertEqual($result[0],"7");
	echo("\n");


	# result set buffer size


	echo("RESULT SET BUFFER SIZE: \n");
	$stmt=$dbh->prepare("select * from testtable order by testnumber");
	assertEqual($stmt->setAttribute(
				PDO::SQLRELAY_ATTR_RESULT_SET_BUFFER_SIZE,2),1);
	assertEqual($stmt->getAttribute(
				PDO::SQLRELAY_ATTR_RESULT_SET_BUFFER_SIZE),2);
	assertTrue($stmt->execute());
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"1");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"2");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"3");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_LAST);
	assertEqual($result[0],"7");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_FIRST);
	assertFalse($result);
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,0);
	assertFalse($result);
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,6);
	assertEqual($result[0],"7");
	$result=$stmt->fetch(PDO::FETCH_NUM,PDO::FETCH_ORI_ABS,5);
	assertFalse($result);
	echo("\n");

	echo("DON'T GET COLUMN INFO: \n");
	$stmt=$dbh->prepare("select * from testtable order by testnumber");
	assertEqual($stmt->setAttribute(
				PDO::SQLRELAY_ATTR_DONT_GET_COLUMN_INFO,1),1);
	assertTrue($stmt->execute());
	$meta0=$stmt->getColumnMeta(0);
	assertEqual($meta0["name"],null);
	assertEqual($meta0["native_type"],null);
	assertEqual($meta0["pdo_type"],PDO::PARAM_STR);
	assertEqual($meta0["len"],null);
	$stmt=$dbh->prepare("select * from testtable order by testnumber");
	assertEqual($stmt->setAttribute(
				PDO::SQLRELAY_ATTR_DONT_GET_COLUMN_INFO,0),1);
	assertTrue($stmt->execute());
	$meta0=$stmt->getColumnMeta(0);
	assertEqual($meta0["name"],"TESTNUMBER");
	assertEqual($meta0["native_type"],"NUMBER");
	assertEqual($meta0["pdo_type"],PDO::PARAM_INT);
	assertEqual($meta0["len"],22);
	echo("\n");


	# other driver-specific options


	echo("OTHER DRIVER-SPECIFIC OPTIONS: \n");
	assertEqual($dbh->getAttribute(PDO::SQLRELAY_ATTR_BIND_FORMAT),":*");
	assertEqual($dbh->getAttribute(
				PDO::SQLRELAY_ATTR_DB_TYPE),"oracle");
	echo("db version: ".$dbh->getAttribute(
				PDO::SQLRELAY_ATTR_DB_VERSION)."\n");
	echo("db host name: ".$dbh->getAttribute(
				PDO::SQLRELAY_ATTR_DB_HOST_NAME)."\n");
	echo("db ip address: ".$dbh->getAttribute(
				PDO::SQLRELAY_ATTR_DB_IP_ADDRESS)."\n");
	echo("current db: ".$dbh->getAttribute(
				PDO::SQLRELAY_ATTR_CURRENT_DB)."\n");
	echo("\n");


	# bound columns


	echo("BOUND COLUMNS: \n");
	$stmt=$dbh->prepare("select * from testtable order by testnumber");
	$col1=0;
	$col2=0;
	$col3=0;
	$col4=0;
	$col5=0;
	$col6=0;
	$col7=0;
	$stmt->bindColumn(1,$col1);
	$stmt->bindColumn(2,$col2);
	$stmt->bindColumn(3,$col3);
	$stmt->bindColumn(4,$col4);
	$stmt->bindColumn(5,$col5,PDO::PARAM_LOB);
	$stmt->bindColumn(6,$col6,PDO::PARAM_LOB);
	$stmt->bindColumn(7,$col7,PDO::PARAM_LOB);
	assertTrue($stmt->execute());
	assertEqual($stmt->fetch(PDO::FETCH_BOUND),TRUE);
	assertEqual($col1,1);
	assertEqual($col2,"testchar1                               ");
	assertEqual($col3,"testvarchar1");
	assertEqual($col4,"01-JAN-01");
	assertEqual(stream_get_contents($col5),"testlong1");
	assertEqual(stream_get_contents($col6),"testclob1");
	assertEqual(stream_get_contents($col7),"");
	assertEqual($stmt->fetch(PDO::FETCH_BOUND),TRUE);
	assertEqual($col1,2);
	assertEqual($col2,"testchar2                               ");
	assertEqual($col3,"testvarchar2");
	assertEqual($col4,"01-JAN-02");
	assertEqual(stream_get_contents($col5),"testlong2");
	assertEqual(stream_get_contents($col6),"testclob2");
	assertEqual(stream_get_contents($col7),"testblob2");
	echo("\n");


	# stringify


	echo("STRINGIFY: \n");
	assertEqual($dbh->setAttribute(PDO::ATTR_STRINGIFY_FETCHES,TRUE),1);
	$stmt=$dbh->query("select * from testtable order by testnumber");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"1");
	assertEqual($result[1],"testchar1                               ");
	assertEqual($result[2],"testvarchar1");
	assertEqual($result[3],"01-JAN-01");
	assertEqual($result[4],"testlong1");
	assertEqual($result[5],"testclob1");
	assertEqual($result[6],"");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"2");
	assertEqual($result[1],"testchar2                               ");
	assertEqual($result[2],"testvarchar2");
	assertEqual($result[3],"01-JAN-02");
	assertEqual($result[4],"testlong2");
	assertEqual($result[5],"testclob2");
	assertEqual($result[6],"testblob2");
	assertEqual($dbh->setAttribute(PDO::ATTR_STRINGIFY_FETCHES,FALSE),1);
	echo("\n");


	# suspended session


	echo("SUSPENDED SESSION: \n");
	$stmt=$dbh->query("select * from testtable order by testnumber");
	$stmt->suspendResultSet();
	assertTrue($dbh->suspendSession());
	$port=$dbh->getConnectionPort();
	$socket=$dbh->getConnectionSocket();
	assertTrue($dbh->resumeSession($port,$socket));
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"1");
	assertEqual($result[1],"testchar1                               ");
	assertEqual($result[2],"testvarchar1");
	assertEqual($result[3],"01-JAN-01");
	assertEqual(stream_get_contents($result[4]),"testlong1");
	assertEqual(stream_get_contents($result[5]),"testclob1");
	assertEqual(stream_get_contents($result[6]),"");
	echo("\n");
	$stmt=$dbh->query("select * from testtable order by testnumber");
	$stmt->suspendResultSet();
	assertTrue($dbh->suspendSession());
	$port=$dbh->getConnectionPort();
	$socket=$dbh->getConnectionSocket();
	assertTrue($dbh->resumeSession($port,$socket));
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"1");
	assertEqual($result[1],"testchar1                               ");
	assertEqual($result[2],"testvarchar1");
	assertEqual($result[3],"01-JAN-01");
	assertEqual(stream_get_contents($result[4]),"testlong1");
	assertEqual(stream_get_contents($result[5]),"testclob1");
	assertEqual(stream_get_contents($result[6]),"");
	echo("\n");
	$stmt=$dbh->query("select * from testtable order by testnumber");
	$stmt->suspendResultSet();
	assertTrue($dbh->suspendSession());
	$port=$dbh->getConnectionPort();
	$socket=$dbh->getConnectionSocket();
	assertTrue($dbh->resumeSession($port,$socket));
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"1");
	assertEqual($result[1],"testchar1                               ");
	assertEqual($result[2],"testvarchar1");
	assertEqual($result[3],"01-JAN-01");
	assertEqual(stream_get_contents($result[4]),"testlong1");
	assertEqual(stream_get_contents($result[5]),"testclob1");
	assertEqual(stream_get_contents($result[6]),"");
	echo("\n");


	# suspended result set


	echo("SUSPENDED RESULT SET: \n");
	$stmt=$dbh->prepare("select * from testtable order by testnumber");
	assertEqual($stmt->setAttribute(
				PDO::SQLRELAY_ATTR_RESULT_SET_BUFFER_SIZE,1),1);
	assertTrue($stmt->execute());
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"1");
	$id=$stmt->getResultSetId();
	$stmt->suspendResultSet();
	assertTrue($dbh->suspendSession());
	$port=$dbh->getConnectionPort();
	$socket=$dbh->getConnectionSocket();
	$dbh=new PDO($dsn,$user,$password);
	$stmt=$dbh->prepare("placeholder");
	assertTrue($dbh->resumeSession($port,$socket));
	$stmt->resumeResultSet($id);
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"2");
	$result=$stmt->fetchAll();
	assertEqual($result[0][0],"3");
	assertEqual($result[1][0],"4");
	assertEqual($result[2][0],"5");
	assertEqual($result[3][0],"6");
	assertEqual($result[4][0],"7");
	echo("\n");


	# commit and rollback


	echo("COMMIT AND ROLLBACK: \n");
	try {
		$dbh->exec("drop table testtable1");
	} catch (Exception $e) {
	}
	assertEqual($dbh->exec(
		"create table ".
		"	testtable1 (testnumber number)"),0);
	if (method_exists($dbh,"inTransaction")) {
		assertEqual($dbh->inTransaction(),0);
	}
	$dbh->beginTransaction();
	if (method_exists($dbh,"inTransaction")) {
		assertEqual($dbh->inTransaction(),1);
	}
	assertEqual($dbh->exec("insert into testtable1 values (1)"),1);
	$dbh2=new PDO($dsn,$user,$password);
	$stmt2=$dbh2->query("select count(*) from testtable1");
	$result2=$stmt2->fetch();
	assertEqual($result2[0],0);
	$dbh->commit();
	$stmt2=$dbh2->query("select count(*) from testtable1");
	$result2=$stmt2->fetch();
	assertEqual($result2[0],1);
	$dbh->beginTransaction();
	assertEqual($dbh->exec("insert into testtable1 values (1)"),1);
	$stmt2=$dbh2->query("select count(*) from testtable1");
	$result2=$stmt2->fetch();
	assertEqual($result2[0],1);
	$dbh->rollback();
	$stmt2=$dbh2->query("select count(*) from testtable1");
	$result2=$stmt2->fetch();
	assertEqual($result2[0],1);
	echo("\n");


	# autocommit


	echo("AUTOCOMMIT: \n");
	if (method_exists($dbh,"inTransaction")) {
		assertEqual($dbh->inTransaction(),0);
	}
	$dbh->setAttribute(PDO::ATTR_AUTOCOMMIT,TRUE);
	assertEqual($dbh->exec("insert into testtable1 values (1)"),1);
	$stmt2=$dbh2->query("select count(*) from testtable1");
	$result2=$stmt2->fetch();
	assertEqual($result2[0],2);
	if (method_exists($dbh,"inTransaction")) {
		assertEqual($dbh->inTransaction(),0);
	}
	$dbh->setAttribute(PDO::ATTR_AUTOCOMMIT,FALSE);
	$dbh->beginTransaction();
	assertEqual($dbh->exec("insert into testtable1 values (1)"),1);
	$stmt2=$dbh2->query("select count(*) from testtable1");
	$result2=$stmt2->fetch();
	assertEqual($result2[0],2);
	$dbh->commit();
	$stmt2=$dbh2->query("select count(*) from testtable1");
	$result2=$stmt2->fetch();
	assertEqual($result2[0],3);
	echo("\n");


	# close cursor
	echo("CLOSE CURSOR\n");
	$stmt=$dbh2->prepare("select * from testtable");
	assertTrue($stmt->execute());
	assertTrue($stmt->closeCursor());
	assertTrue($stmt->execute());
	echo("\n");


	# client and server versions


	echo("CLIENT AND SERVER VERSIONS: \n");
	assertEqual($dbh->getAttribute(PDO::ATTR_CLIENT_VERSION),
			$dbh->getAttribute(PDO::ATTR_SERVER_VERSION));
	echo("\n");

	# drop testtables
	try {
		$dbh->exec("drop table testtable");
	} catch (Exception $e) {
	}
	try {
		$dbh->exec("drop table testtable1");
	} catch (Exception $e) {
	}

# output binds don't appear to work with PDO for PHP7
if (PHP_VERSION_ID < 70000) {


	# output bind by position


	echo("OUTPUT BIND BY POSITION: \n");
	$stmt=$dbh->prepare("begin  :numvar:=1; :stringvar:='hello'; end;");
	$stmt=$dbh->prepare("begin  :1:=1; :2:='hello'; end;");
	$param1=0;
	$param2="";
	assertTrue($stmt->bindParam(1,$param1,PDO::PARAM_INT|PDO::PARAM_INPUT_OUTPUT));
	assertTrue($stmt->bindParam(2,$param2,PDO::PARAM_STR|PDO::PARAM_INPUT_OUTPUT,10));
	assertTrue($stmt->execute());
	assertEqual($param1,1);
	assertEqual($param2,"hello");
	echo("\n");


	# output bind by name


	echo("OUTPUT BIND BY NAME: \n");
	$param1=0;
	$param2="";
	assertTrue($stmt->bindParam(":numvar",$param1,PDO::PARAM_INT|PDO::PARAM_INPUT_OUTPUT));
	assertTrue($stmt->bindParam(":stringvar",$param2,PDO::PARAM_STR|PDO::PARAM_INPUT_OUTPUT,10));
	assertTrue($stmt->execute());
	assertEqual($param1,1);
	assertEqual($param2,"hello");
	echo("\n");


	# clob and blob output bind


	echo("CLOB AND BLOB OUTPUT BIND: \n");
	try {
		$dbh->exec("drop table testtable1");
	} catch (Exception $e) {
	}
	assertEqual($dbh->exec(
		"create table testtable1 (".
		"	testclob clob, ".
		"	testblob blob)"),0);
	$stmt=$dbh->prepare("insert into testtable1 values ('hello',:var1)");
	assertTrue($stmt->bindValue("var1","hello",PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	$stmt=$dbh->prepare(
		"begin  select testblob into :blobvar from testtable1; ".
		"	end;");
	$param1="";
	assertTrue($stmt->bindParam(":blobvar",$param1,PDO::PARAM_LOB|PDO::PARAM_INPUT_OUTPUT));
	assertTrue($stmt->execute());
	assertEqual(stream_get_contents($param1),"hello");
	echo("\n");


	# null and empty lobs


	echo("NULL AND EMPTY LOBS: \n");
	try {
		$dbh->exec("drop table testtable1");
	} catch (Exception $e) {
	}
	assertEqual($dbh->exec(
		"create table testtable1 (".
		"	testclob clob, ".
		"	testblob blob)"),0);
	$stmt=$dbh->prepare("insert into testtable1 values (:var1,:var2)");
	assertTrue($stmt->bindValue("var1","",PDO::PARAM_LOB));
	assertTrue($stmt->bindValue("var2","",PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	$stmt=$dbh->query("select * from testtable1");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual(stream_get_contents($result[0]),"");
	assertEqual(stream_get_contents($result[1]),"");
	$dbh->exec("delete from testtable1");
	$stmt=$dbh->prepare("insert into testtable1 values (:var1,:var2)");
	assertTrue($stmt->bindValue("var1",null,PDO::PARAM_LOB));
	assertTrue($stmt->bindValue("var2",null,PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	$stmt=$dbh->query("select * from testtable1");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],null);
	assertEqual($result[1],null);
	$dbh->exec("delete from testtable1");
	echo("\n");


	# clob and blob output bind to and from file


	echo("CLOB AND BLOB OUTPUT BIND TO AND FROM FILE: \n");
	try {
		$dbh->exec("drop table testtable1");
	} catch (Exception $e) {
	}
	assertEqual($dbh->exec(
		"create table testtable1 (".
		"	testclob clob, ".
		"	testblob blob)"),0);
	$stmt=$dbh->prepare("insert into testtable1 values ('hello',:var1)");
	$stream=fopen("test.blob","w+b");
	fwrite($stream,"hello");
	fclose($stream);
	$stream=fopen("test.blob","rb");
	assertTrue($stmt->bindValue("var1",$stream,PDO::PARAM_LOB));
	assertTrue($stmt->execute());
	fclose($stream);
	unlink("test.blob");
	$stmt=$dbh->prepare(
		"begin  select testblob into :blobvar from testtable1; ".
		"	end;");
	$stream=fopen("test.blob","w+b");
	assertTrue($stmt->bindParam(":blobvar",$stream,PDO::PARAM_LOB|PDO::PARAM_INPUT_OUTPUT));
	assertTrue($stmt->execute());
	assertEqual(stream_get_contents($stream),"hello");
	fclose($stream);
	unlink("test.blob");
	echo("\n");
}

	$dbh->setAttribute(PDO::ATTR_ERRMODE,PDO::ERRMODE_SILENT);

	# this throws an execption and doesn't continue on PHP7
	try {


		# non-lazy connect


		echo("NON-LAZY CONNECT: \n");
		$dsn = "sqlrelay:host=invalidhost;port=0;socket=/invalidsocket;tries=1;retrytime=1;krb=yes;debug=0;lazyconnect=0";
		assertEqual(new PDO($dsn,$user,$password),0);
		echo("\n");

	} catch (Exception $e) {
		echo($e->getMessage());
		echo("\n");
	}


	# invalid queries


	echo("INVALID QUERIES: \n");
	assertEqual($dbh->query("select 1"),0);
	assertEqual($dbh->errorCode(),"HY000");
	$info=$dbh->errorInfo();
	assertEqual($info[0],"HY000");
	assertEqual($info[1],923);
	assertEqual($info[2],"ORA-00923: FROM keyword not found where expected");
	$stmt=$dbh->prepare("select 1");
	assertEqual($stmt->execute(),0);
	assertEqual($stmt->errorCode(),"HY000");
	$info=$stmt->errorInfo();
	assertEqual($info[0],"HY000");
	assertEqual($info[1],923);
	assertEqual($info[2],"ORA-00923: FROM keyword not found where expected");
	echo("\n");


	# invalid operations


	echo("INVALID OPERATIONS: \n");
	assertEqual($stmt->nextRowset(),0);
	assertEqual($stmt->setAttribute(PDO::ATTR_AUTOCOMMIT,FALSE),0);
	assertEqual($stmt->getAttribute(PDO::ATTR_AUTOCOMMIT),0);
	assertEqual($dbh->quote("select * from table"),null);
	# bindValue coerces an invalid PDO::PARAM type to PARAM_STR (#8101)
	assertTrue($stmt->bindValue(1,1,9999));
	echo("\n");

	try {
		$dbh->exec("drop table testtable");
	} catch (Exception $e) {
	}

	reportTestStatus();

	exit($status);
?></pre></html>
