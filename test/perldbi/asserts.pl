# Copyright (c) David Muse
# See the file COPYING for more information.

$status=0;

$success="\e[32msuccess\e[0m";
$failure="\e[31mfailure\e[0m";
$alltestssucceeded="\n\e[34mAll tests succeeded\e[0m\n";
$sometestsfailed="\n\e[33mSome tests failed\e[0m\n";

sub assertUndef {

	$actual=shift(@_);

	if (!defined($actual)) {
		print("$success ");
	} else {
		print("$failure\n");
		$status=1;
	}
}

sub assertDefined {

	$actual=shift(@_);

	if (defined($actual)) {
		print("$success ");
	} else {
		print("$failure\n");
		$status=1;
	}
}

sub assertEqual {

	$actual=shift(@_);
	$expected=shift(@_);

	if ($actual==$expected) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual != $expected\n");
		$status=1;
	}
}

sub assertEqualString {

	$actual=shift(@_);
	$expected=shift(@_);

	if ($actual eq $expected) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual != $expected\n");
		$status=1;
	}
}

sub assertTrue {

	$actual=shift(@_);

	if ($actual==1) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual != 1\n");
		$status=1;
	}
}

sub assertFalse {

	$actual=shift(@_);

	if ($actual==0) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual != 0\n");
		$status=1;
	}
}

sub reportTestStatus {

	if ($status==0) {
		print($alltestssucceeded);
	} else {
		print($sometestsfailed);
	}
}

1;
