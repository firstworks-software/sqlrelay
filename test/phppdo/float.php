<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("asserts.php");

	$host="sqlrelay";
	$port=9000;
	$socket="/tmp/test.socket";
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

	echo("CREATE TEMPTABLE: \n");
	if (!$dbh->exec("create table testtable (testfloat float, testdouble double)")) {
		$dbh->exec("create table testtable (testfloat float, testdouble float)");
	}
	echo("\n");

	echo("INSERT: \n");
	assertEqual($dbh->exec("insert into testtable values (3.14,3.14)"),1);
	assertEqual($dbh->exec("insert into testtable values (6.28,6.28)"),1);
	assertEqual($dbh->exec("insert into testtable values (9.42,9.42)"),1);
	echo("\n");

	echo("SELECT: \n");
	$stmt=$dbh->query("select * from testtable order by testfloat");
	echo("\n");
	
	echo("FIELDS BY INDEX: \n");
	$result=$stmt->fetch(PDO::FETCH_NUM);
	assertEqual($result[0],3.14);
	assertEqual($result[1],3.14);
	echo("\n");

	echo("FIELDS BY NAME: \n");
	$result=$stmt->fetch(PDO::FETCH_ASSOC);
	assertEqual($result["testfloat"],6.28);
	assertEqual($result["testdouble"],6.28);
	var_dump($result);
	echo("\n");

	echo("FIELDS BY NAME AND INDEX: \n");
	$result=$stmt->fetch();
	assertEqual($result[0],9.42);
	assertEqual($result[1],9.42);
	echo("\n");

	$dbh->exec("drop table testtable");

	reportTestStatus();

	exit($status);
?></pre></html>
