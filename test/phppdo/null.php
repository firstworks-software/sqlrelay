<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("asserts.php");

	$host="sqlrelay";
	$port=9002;
	$socket="/tmp/mysqltest.socket";
	$user="testuser";
	$password="testpassword";
	$dsn = "sqlrelay:host=$host;port=$port;socket=$socket;tries=0;retrytime=1;debug=0";


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
	$dbh->exec(
		"create table testtable (".
		"	testint int, ".
		"	testfloat float, ".
		"	testchar varchar(20), ".
		"	testblob blob, ".
		"	testdate datetime)");
	echo("\n");


	# insert


	echo("INSERT: \n");
	assertEqual($dbh->exec(
		"insert into ".
		"	testtable ".
		"values (".
		"	NULL, ".
		"	NULL, ".
		"	NULL, ".
		"	NULL, ".
		"	NULL)"),1);
	assertEqual($dbh->exec(
		"insert into ".
		"	testtable ".
		"values (".
		"	1, ".
		"	1.5, ".
		"	'1', ".
		"	'1', ".
		"	'2001-01-01')"),1);
	assertEqual($dbh->exec(
		"insert into ".
		"	testtable ".
		"values (".
		"	0, ".
		"	0.0, ".
		"	'0', ".
		"	'0', ".
		"	'0000-01-01 00:00:00')"),1);
	echo("\n");


	# fields by index (as null)
	echo("FIELDS BY INDEX (as NULL): \n");
	$stmt=$dbh->query("select * from testtable");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"");
	assertEqual($result[1],"");
	assertEqual($result[2],"");
	assertEqual(stream_get_contents($result[3]),"");
	assertEqual($result[4],"");
echo("\n");
print_r($result);
echo("\n");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],1);
	assertEqual($result[1],1.5);
	assertEqual($result[2],"1");
	assertEqual(stream_get_contents($result[3]),"1");
	assertEqual($result[4],"2001-01-01 00:00:00");
echo("\n");
print_r($result);
echo("\n");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],0);
	assertEqual($result[1],0.0);
	assertEqual($result[2],"0");
	assertEqual(stream_get_contents($result[3]),"0");
	assertEqual($result[4],"0000-01-01 00:00:00");
echo("\n");
print_r($result);
echo("\n");
	echo("\n");


	# fields by index (as empty strings)
	echo("FIELDS BY INDEX (as empty strings): \n");
	$stmt=$dbh->prepare("select * from testtable");
	$stmt->setAttribute(
		PDO::SQLRELAY_ATTR_GET_NULLS_AS_EMPTY_STRINGS,false);
	$stmt->execute();
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],null);
	assertEqual($result[1],null);
	assertEqual($result[2],null);
	assertEqual($result[3],null);
	assertEqual($result[4],null);
echo("\n");
print_r($result);
echo("\n");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],1);
	assertEqual($result[1],1.5);
	assertEqual($result[2],"1");
	assertEqual(stream_get_contents($result[3]),"1");
	assertEqual($result[4],"2001-01-01 00:00:00");
echo("\n");
print_r($result);
echo("\n");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],0);
	assertEqual($result[1],0.0);
	assertEqual($result[2],"0");
	assertEqual(stream_get_contents($result[3]),"0");
	assertEqual($result[4],"0000-00-00 00:00:00");
echo("\n");
print_r($result);
echo("\n");
	echo("\n");


	# null integer input bind
	echo("NULL INTEGER INPUT BIND\n");
	$stmt=$dbh->prepare("select ?,?");
	$param1=null;
	$param2="";
	$stmt->bindParam("1",$param1,PDO::PARAM_INT);
	$stmt->bindParam("2",$param2,PDO::PARAM_INT);
	$stmt->setAttribute(
		PDO::SQLRELAY_ATTR_GET_NULLS_AS_EMPTY_STRINGS,true);
	$stmt->execute();
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],"");
	assertEqual($result[1],"");
	$param1=null;
	$param2="";
	$stmt->setAttribute(
		PDO::SQLRELAY_ATTR_GET_NULLS_AS_EMPTY_STRINGS,false);
	$stmt->execute();
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],null);
	assertEqual($result[1],null);
	echo("\n");

	try {
		$dbh->exec("drop table testtable");
	} catch (Exception $e) {
	}

	reportTestStatus();

	exit($status);
?></pre></html>
