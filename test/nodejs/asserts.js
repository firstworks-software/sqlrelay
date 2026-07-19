// Copyright (c) David Muse
// See the file COPYING for more information.

// node 4 compatibility shims - win7x64 ships node v4, but the tests use a few
// newer APIs.  each is guarded, so on a modern node nothing is redefined.
// missing Buffer.alloc marks the pre-v4.5 Buffer api; there Buffer.from is the
// inherited (typed-array) from - wrong, and a plain assignment to it silently
// no-ops since it is an inherited accessor, so define both properties.
if (typeof Buffer.alloc!=="function") {
	Object.defineProperty(Buffer,"alloc",{
		value:function(size,fill) {
			var b=new Buffer(size);
			b.fill((fill===undefined)?0:fill);
			return b;
		},
		writable:true,configurable:true
	});
	Object.defineProperty(Buffer,"from",{
		value:function(value,a,b) {
			return new Buffer(value,a,b);
		},
		writable:true,configurable:true
	});
}
if (typeof String.prototype.padStart!=="function") {
	String.prototype.padStart=function(targetlength,padstring) {
		targetlength=targetlength>>0;
		padstring=String((padstring===undefined)?" ":padstring);
		if (this.length>targetlength || padstring==="") {
			return String(this);
		}
		targetlength=targetlength-this.length;
		if (targetlength>padstring.length) {
			padstring+=padstring.repeat(targetlength/padstring.length);
		}
		return padstring.slice(0,targetlength)+String(this);
	};
}

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

function normalizeMoney(v) {
	if (v===null || v===undefined) {
		return v;
	}
	// ignore a leading '$' and thousands commas
	var s=String(v).replace(/[$,]/g,"");
	// ignore trailing-zero decimals
	if (s.indexOf(".")>-1) {
		s=s.replace(/0+$/,"");
		if (s.charAt(s.length-1)===".") {
			s=s.substring(0,s.length-1);
		}
	}
	return s;
}

function assertMoneyEqStr(actual, expected) {
	// null handling matches assertEqStr
	if (actual===null || actual===undefined ||
		expected===null || expected===undefined) {
		assertEqStr(actual,expected);
		return;
	}
	if (normalizeMoney(actual)===normalizeMoney(expected)) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("\""+actual+"\"!=\""+expected+"\"");
		printErrors();
		status=1;
	}
}

function assertMoneyEqLen(actual, expected) {
	// old freetds renders money with 2 decimal places instead of 4
	if (actual===expected || actual===expected-2) {
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

function assertStartsWith(actual, prefix) {
	if (actual!=null && String(actual).startsWith(prefix)) {
		process.stdout.write(success+" ");
	} else {
		console.log(failure);
		console.log("\""+actual+"\" doesn't start with \""+prefix+"\"");
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

function assertInResultSet(cursor, column, value) {
	for (var i=0; i<cursor.rowCount(); i++) {
		if (cursor.getField(i,column)==value) {
			process.stdout.write(success+" ");
			return;
		}
	}
	console.log(failure);
	console.log("\""+value+"\" not found in column \""+column+"\"");
	printErrors();
	status=1;
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
	assertMoneyEqStr, assertMoneyEqLen,
	assertTrue, assertFalse, assertStartsWith,
	assertInResultSet,
	getStatus, reportTestStatus
};
