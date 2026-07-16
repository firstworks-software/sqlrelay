# Copyright (c) David Muse
# See the file COPYING for more information.

# keep stdout unbuffered so a crash is logged where it actually happens
$|=1;

$status=0;

$success="\e[32msuccess\e[0m";
$failure="\e[31mfailure\e[0m";
$alltestssucceeded="\n\e[34mAll tests succeeded\e[0m\n";
$sometestsfailed="\n\e[38;5;208mSome tests failed\e[0m\n";

sub printErrors {

	if (defined($cur)) {
		my $err=$cur->errorMessage();
		if ($err) {
			print("$err\n");
			return;
		}
	}
	if (defined($secondcur)) {
		my $err=$secondcur->errorMessage();
		if ($err) {
			print("$err\n");
			return;
		}
	}
	if (defined($con)) {
		my $err=$con->errorMessage();
		if ($err) {
			print("$err\n");
			return;
		}
	}
	if (defined($secondcon)) {
		my $err=$secondcon->errorMessage();
		if ($err) {
			print("$err\n");
			return;
		}
	}
}

sub assertEquals {

	my ($actual,$expected)=@_;

	if (!defined($expected)) {
		if (!defined($actual)) {
			print("$success ");
		} else {
			print("$failure\n");
			print("$actual!=<undef>\n");
			printErrors();
			$status=1;
		}
		return;
	}

	if (!defined($actual)) {
		print("$failure\n");
		print("<undef>!=$expected\n");
		printErrors();
		$status=1;
		return;
	}

	if ($actual eq $expected) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual!=$expected\n");
		printErrors();
		$status=1;
	}
}

sub assertEqualsBytes {

	my ($actual,$expected,$length)=@_;

	if (!defined($expected)) {
		if (!defined($actual)) {
			print("$success ");
		} else {
			print("$failure\n");
			printErrors();
			$status=1;
		}
		return;
	}

	if (!defined($actual)) {
		print("$failure\n");
		printErrors();
		$status=1;
		return;
	}

	if (substr($actual,0,$length) eq substr($expected,0,$length)) {
		print("$success ");
	} else {
		print("$failure\n");
		printErrors();
		$status=1;
	}
}

sub assertTrue {

	my ($actual)=@_;

	if ($actual) {
		print("$success ");
	} else {
		print("$failure\n");
		my $val=(defined($actual))?$actual:"<undef>";
		print("$val!=true\n");
		printErrors();
		$status=1;
	}
}

sub assertFalse {

	my ($actual)=@_;

	if (!$actual) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual!=false\n");
		printErrors();
		$status=1;
	}
}

sub assertStartsWith {

	my ($actual,$prefix)=@_;

	if (defined($actual) && index($actual,$prefix)==0) {
		print("$success ");
	} else {
		print("$failure\n");
		my $val=(defined($actual))?$actual:"<undef>";
		print("$val doesn't start with $prefix\n");
		printErrors();
		$status=1;
	}
}

sub assertUndef {

	my ($actual)=@_;

	if (!defined($actual)) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual!=<undef>\n");
		printErrors();
		$status=1;
	}
}

sub assertInResultSet {

	my ($cur,$column,$value)=@_;

	for (my $i=0; $i<$cur->rowCount(); $i++) {
		my $field=$cur->getField($i,$column);
		if (defined($field) && $field eq $value) {
			print("$success ");
			return;
		}
	}
	print("$failure\n");
	print("\"$value\" not found in column \"$column\"\n");
	printErrors();
	$status=1;
}

# strips a leading '$' and thousands separators, then trailing-zero decimal
# digits, so money values that differ only in formatting compare equal
sub normalizeMoney {

	my ($value)=@_;

	if (!defined($value)) {
		return $value;
	}

	# drop '$' and ','
	$value=~s/[\$,]//g;

	# drop trailing-zero decimals
	if (index($value,".")>=0) {
		$value=~s/0+$//;
		$value=~s/\.$//;
	}
	return $value;
}

# compares money values tolerantly - a leading '$', thousands separators, and
# trailing-zero decimals are ignored, so "1.00", "$1.00", and "1.0000" all
# match.  a real precision difference like "1.23" vs "1.2345" still fails.
# old freetds (0.91) renders money with 2 decimal places instead of 4.
sub assertMoneyEquals {

	my ($actual,$expected)=@_;

	if (!defined($expected) || !defined($actual)) {
		assertEquals($actual,$expected);
		return;
	}

	if (normalizeMoney($actual) eq normalizeMoney($expected)) {
		print("$success ");
	} else {
		print("$failure\n");
		print("$actual!=$expected\n");
		printErrors();
		$status=1;
	}
}

# compares a rendered money length tolerantly.  old freetds (0.91) renders
# money with 2 decimal places instead of 4, so the length may be 2 short.
sub assertMoneyLengthEquals {

	my ($actual,$expected)=@_;

	if (defined($actual) &&
		($actual==$expected || $actual==$expected-2)) {
		print("$success ");
	} else {
		print("$failure\n");
		my $val=(defined($actual))?$actual:"<undef>";
		print("$val!=$expected\n");
		printErrors();
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
