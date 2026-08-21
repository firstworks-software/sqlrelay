<html><pre><?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	include("asserts.php");

	$host="sqlrelay";
	$port=9001;
	$socket="/tmp/oracle.socket";
	$user="testuser";
	$password="testpassword";
	$dsn="sqlrelay:host=$host;port=$port;socket=$socket;tries=0;retrytime=1;debug=0";


	$dbh=new PDO($dsn,$user,$password);
	if(!$dbh){
		die("new PDO failed");
	}

	$dbtype=$dbh->getAttribute(PDO::SQLRELAY_ATTR_DB_TYPE);

	if ($dbtype!="firebird") {
		try {
			$dbh->exec("drop table testtable");
		} catch (Exception $e) {
		}


		# create temptable


		echo("CREATE TEMPTABLE: \n");
		assertEqual($dbh->exec(
			"create table ".
			"	testtable (testinteger int)"),0);
		echo("\n");
	}


	# bind by position


	echo("BIND BY POSITION: \n");
	$queryvar="";
	$bindvar=1;
	switch ($dbtype) {
		case "oracle":
		case "sqlite":
			$queryvar=":1";
			break;
		case "sap":
		case "freetds":
			$queryvar="@1";
			break;
		case "db2":
		case "firebird":
		case "mysql":
			$queryvar="?";
			break;
		case "postgresql":
			$queryvar="$1";
			break;
	}
	echo("queryvar: $queryvar\n");
	$stmt=$dbh->prepare(
		"insert into testtable ".
		"	(testinteger) values ($queryvar)");
	assertTrue($stmt->bindValue(1,2,PDO::PARAM_INT));
	assertTrue($stmt->execute());
	echo("\n");


	# bind by name


	echo("BIND BY NAME: \n");
	$queryvar="";
	$bindvar="";
	switch ($dbtype) {
		case "oracle":
		case "sqlite":
			$queryvar=":var1";
			$bindvar="var1";
			break;
		case "sap":
		case "freetds":
			$queryvar="@var1";
			$bindvar="var1";
			break;
		case "db2":
		case "firebird":
		case "mysql":
			$queryvar="?";
			$bindvar="1";
			break;
		case "postgresql":
			$queryvar="$1";
			$bindvar="1";
			break;
	}
	echo("queryvar: $queryvar   bindvar: $bindvar\n");
	$stmt=$dbh->prepare(
		"insert into testtable ".
		"	(testinteger) values ($queryvar)");
	assertTrue($stmt->bindValue($bindvar,2,PDO::PARAM_INT));
	assertTrue($stmt->execute());
	echo("\n");

	$dbh->exec("delete from testtable");
	if ($dbtype!="firebird") {
		$dbh->exec("drop table testtable");
	}

	reportTestStatus();

	exit($status);
?></pre></html>
