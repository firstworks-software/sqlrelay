<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("asserts.php");

	$host="sqlrelay";
	$port=9003;
	$socket="/tmp/postgresql.socket";
	$user="testuser";
	$password="testpassword";
	# autocommit on, so an expected error does not leave postgresql's
	# transaction aborted, with every statement after it failing with
	# 25P02.  the driver's own default for this option is off
	$dsn = "sqlrelay:host=$host;port=$port;socket=$socket;tries=0;retrytime=1;debug=0;autocommit=1";


	# instantiation
	$dbh=new PDO($dsn,$user,$password);
	if(!$dbh){
		die("new PDO failed");
	}
	$dbh->setAttribute(PDO::ATTR_ERRMODE,PDO::ERRMODE_SILENT);


	# drop existing table
	$dbh->exec("drop table testtable");


	# error sqlstate - connection handle


	echo("ERROR SQLSTATE - CONNECTION HANDLE: \n");
	assertEqual($dbh->exec("create table testtable (col1 int)"),0);
	assertEqual($dbh->errorCode(),"00000");
	# postgresql reports a duplicate table as 42P07, in every locale
	assertEqual($dbh->exec("create table testtable (col1 int)"),0);
	assertEqual($dbh->errorCode(),"42P07");
	$info=$dbh->errorInfo();
	assertEqual($info[0],"42P07");
	# and an undefined table as 42P01
	assertEqual($dbh->query("select * from nonexistenttable"),0);
	assertEqual($dbh->errorCode(),"42P01");
	$info=$dbh->errorInfo();
	assertEqual($info[0],"42P01");
	echo("\n");


	# error sqlstate - statement handle


	echo("ERROR SQLSTATE - STATEMENT HANDLE: \n");
	$stmt=$dbh->prepare("create table testtable (col1 int)");
	assertEqual($stmt->execute(),0);
	assertEqual($stmt->errorCode(),"42P07");
	$info=$stmt->errorInfo();
	assertEqual($info[0],"42P07");
	$stmt=$dbh->prepare("select * from nonexistenttable");
	assertEqual($stmt->execute(),0);
	assertEqual($stmt->errorCode(),"42P01");
	echo("\n");


	# clean up


	assertEqual($dbh->exec("drop table testtable"),0);
	assertEqual($dbh->errorCode(),"00000");

	reportTestStatus();

	exit($status);
?></pre></html>
