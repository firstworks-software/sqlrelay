<?php
	# Copyright (c) David Muse
	# See the file COPYING for more information.

	$status=0;

	define('success', "\033[32msuccess\033[0m");
	define('failure', "\033[31mfailure\033[0m");
	define('alltestssucceeded', "\n\033[34mAll tests succeeded\033[0m\n");
	define('sometestsfailed', "\n\033[33mSome tests failed\033[0m\n");

	function assertEqual($actual,$expected) {
		global $status;
		if ($actual==$expected) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("$actual != $expected\n");
			$status=1;
		}
	}

	function assertTrue($actual) {
		global $status;
		if ($actual==true) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("$actual != true\n");
			$status=1;
		}
	}

	function assertFalse($actual) {
		global $status;
		if ($actual==false) {
			echo(success." ");
		} else {
			echo(failure."\n");
			echo("$actual != false\n");
			$status=1;
		}
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
