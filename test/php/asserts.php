<?php
# Copyright (c) David Muse
# See the file COPYING for more information.
	$status=0;

	define('success', "\033[32msuccess\033[0m");
	define('failure', "\033[31mfailure\033[0m");
	define('alltestssucceeded', "\n\033[34mAll tests succeeded\033[0m\n");
	define('sometestsfailed', "\n\033[33mSome tests failed\033[0m\n");

	function printErrors() {

		global $cur;
		global $con;

		if ($cur) {
			$err=sqlrcur_errorMessage($cur);
			if ($err) {
				echo("$err\n");
				return;
			}
		}
		if ($con) {
			$err=sqlrcon_errorMessage($con);
			if ($err) {
				echo("$err\n");
				return;
			}
		}
	}

	function assertEqStr($actual,$expected) {

		global $status;

		# PHP's sqlrcur_* accessors return false (not null) for
		# NULL fields; treat false and null equivalently.
		if ($expected===NULL) {
			if ($actual===NULL || $actual===false) {
				echo(success." ");
			} else {
				echo(failure."\n");
				echo("\"$actual\"!=\"$expected\"\n");
				printErrors();
				$status=1;
			}
			return;
		}

		if (strcmp($actual,$expected)==0) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("\"$actual\"!=\"$expected\"\n");
			printErrors();
			$status=1;
		}
	}

	function assertEqStrLen($actual,$expected,$length) {

		global $status;

		if ($expected===NULL) {
			if ($actual===NULL || $actual===false) {
				echo(success." ");
			} else {
				echo(failure."\n");
				echo("\"$actual\"!=\"$expected\"\n");
				printErrors();
				$status=1;
			}
			return;
		}

		if (strncmp($actual,$expected,$length)==0) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("\"$actual\"!=\"$expected\"\n");
			printErrors();
			$status=1;
		}
	}

	function assertEqInt($actual,$expected) {

		global $status;

		if ((int)$actual===(int)$expected) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("\"$actual\"!=\"$expected\"\n");
			printErrors();
			$status=1;
		}
	}

	function assertEqDbl($actual,$expected) {

		global $status;

		if ((float)$actual===(float)$expected) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("\"$actual\"!=\"$expected\"\n");
			printErrors();
			$status=1;
		}
	}

	# Old freetds (0.91) renders SAP ASE money/smallmoney with 2
	# decimal places ("1.00") instead of 4 ("1.0000"), and reports a
	# rendered length 2 chars shorter.  These asserts tolerate a
	# leading '$', thousands commas, and trailing-zero decimals, but
	# still fail on real precision loss ("1.23" vs "1.2345").

	function normalizeMoney($v) {

		if ($v===NULL) {
			return $v;
		}

		$v=str_replace(array('$',','),"",$v);

		if (strpos($v,'.')!==false) {
			$v=rtrim($v,'0');
			$v=rtrim($v,'.');
		}

		return $v;
	}

	function assertMoneyEqStr($actual,$expected) {

		global $status;

		if ($actual===NULL || $expected===NULL) {
			assertEqStr($actual,$expected);
			return;
		}

		if (normalizeMoney($actual)===normalizeMoney($expected)) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("\"$actual\"!=\"$expected\"\n");
			printErrors();
			$status=1;
		}
	}

	function assertMoneyEqLen($actual,$expected) {

		global $status;

		# old freetds renders money with 2 decimal places instead of 4
		if ($actual===$expected || $actual===$expected-2) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("\"$actual\"!=\"$expected\"\n");
			printErrors();
			$status=1;
		}
	}

	function assertTrue($actual) {

		global $status;

		if ($actual) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("false!=true\n");
			printErrors();
			$status=1;
		}
	}

	function assertFalse($actual) {

		global $status;

		if (!$actual) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("true!=false\n");
			printErrors();
			$status=1;
		}
	}

	function assertStartsWith($actual,$prefix) {

		global $status;

		if ($actual!==false && $actual!==NULL &&
				strncmp($actual,$prefix,strlen($prefix))==0) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("\"$actual\" doesn't start with \"$prefix\"\n");
			printErrors();
			$status=1;
		}
	}

	function assertInResultSet($cur,$column,$value) {

		global $status;

		for ($i=0; $i<sqlrcur_rowCount($cur); $i++) {
			if (!strcmp(sqlrcur_getField($cur,$i,$column),
								$value)) {
				echo(success." ");
				return;
			}
		}
		echo(failure."\n");
		echo("\"$value\" not found in column \"$column\"\n");
		printErrors();
		$status=1;
	}

	function reportTestStatus() {

		global $status;

		if ($status==0) {
			echo(alltestssucceeded);
		} else {
			echo(sometestsfailed);
		}
	}
?>
