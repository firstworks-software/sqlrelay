// Copyright (c) David Muse
// See the file COPYING for more information.

var con;
var cur;
var secondcon;
var secondcur;

var status=0;

const success="\x1b[32msuccess\x1b[0m";
const failure="\x1b[31mfailure\x1b[0m";
const alltestssucceeded="\n\x1b[34mAll tests succeeded\x1b[0m";
const sometestsfailed="\n\x1b[38;5;208mSome tests failed\x1b[0m";

function setConnection(c) {
	con=c;
}

function setCursor(c) {
	cur=c;
}

function setSecondConnection(c) {
	secondcon=c;
}

function setSecondCursor(c) {
	secondcur=c;
}

function printErrors() {
	if (cur) {
		var err=cur.errorMessage();
		if (err) {
			console.log(err);
			return;
		}
	}
	if (secondcur) {
		var err=secondcur.errorMessage();
		if (err) {
			console.log(err);
			return;
		}
	}
	if (con) {
		var err=con.errorMessage();
		if (err) {
			console.log(err);
			return;
		}
	}
	if (secondcon) {
		var err=secondcon.errorMessage();
		if (err) {
			console.log(err);
			return;
		}
	}
}

function assertEqStr(actual, expected) {
	if (expected===null) {
		if (actual===null || actual===undefined) {
			process.stdout.write(success+" ");
		} else {
			console.log(failure);
			console.log("\""+actual+"\"!=\""+expected+"\"");
			printErrors();
			status=1;
		}
		return;
	}
	if (String(actual)===String(expected)) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("\""+actual+"\"!=\""+expected+"\"");
		printErrors();
		status=1;
	}
}

function assertEqStrLen(actual, expected, length) {
	if (expected===null) {
		if (actual===null || actual===undefined) {
			process.stdout.write(success+" ");
		} else {
			console.log(failure);
			console.log("\""+actual+"\"!=\""+expected+"\"");
			printErrors();
			status=1;
		}
		return;
	}
	var a=String(actual).substring(0,length);
	var e=String(expected).substring(0,length);
	if (a===e) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("\""+actual+"\"!=\""+expected+"\"");
		printErrors();
		status=1;
	}
}

function assertEqInt(actual, expected) {
	if (parseInt(actual)===parseInt(expected)) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("\""+actual+"\"!=\""+expected+"\"");
		printErrors();
		status=1;
	}
}

function assertEqDbl(actual, expected) {
	if (parseFloat(actual)===parseFloat(expected)) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("\""+actual+"\"!=\""+expected+"\"");
		printErrors();
		status=1;
	}
}

function assertEqual(actual, expected) {
	assertEqStr(actual,expected);
}

function assertTrue(actual) {
	if (actual) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("false!=true");
		printErrors();
		status=1;
	}
}

function assertFalse(actual) {
	if (!actual) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("true!=false");
		printErrors();
		status=1;
	}
}

function getStatus() {
	return status;
}

function reportTestStatus() {
	if (status==0) {
		console.log(alltestssucceeded);
	} else {
		console.log(sometestsfailed);
	}
}

module.exports={
	setConnection, setCursor,
	setSecondConnection, setSecondCursor,
	assertEqStr, assertEqStrLen,
	assertEqInt, assertEqDbl, assertEqual,
	assertTrue, assertFalse,
	getStatus, reportTestStatus
};
