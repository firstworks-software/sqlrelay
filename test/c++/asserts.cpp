// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>
#include <sqlrelay/sqlrclient.h>

extern sqlrconnection	*con;
extern sqlrcursor	*cur;
extern sqlrconnection	*secondcon;
extern sqlrcursor	*secondcur;

int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

void printErrors() {
	if (cur) {
		const char	*err=cur->errorMessage();
		if (err) {
			stdoutput.printf("%s\n",err);
			return;
		}
	}
	if (secondcur) {
		const char	*err=secondcur->errorMessage();
		if (err) {
			stdoutput.printf("%s\n",err);
			return;
		}
	}
	if (con) {
		const char	*err=con->errorMessage();
		if (err) {
			stdoutput.printf("%s\n",err);
			return;
		}
	}
	if (secondcon) {
		const char	*err=secondcon->errorMessage();
		if (err) {
			stdoutput.printf("%s\n",err);
			return;
		}
	}
}

void assertEquals(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("%s!=%s\n",actual,expected);
			printErrors();
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(const char *actual, const char *expected, size_t length) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("%s!=%s\n",actual,expected);
			printErrors();
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected,length)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertStartsWith(const char *actual, const char *prefix) {

	if (actual && !charstring::compare(actual,prefix,
						charstring::getLength(prefix))) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s doesn't start with %s\n",actual,prefix);
		printErrors();
		status=1;
	}
}

void assertEquals(bool actual, bool expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(int32_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%d!=%d\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(uint32_t actual, int expected) {

	if (actual==(uint32_t)expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%d!=%d\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(int64_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%lld!=%lld\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(uint64_t actual, int expected) {

	if (actual==(uint64_t)expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%lld!=%lld\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(double actual, double expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%f!=%f\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertTrue(bool actual) {

	if (actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=true\n",(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

void assertFalse(bool actual) {

	if (!actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=false\n",(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

void assertInResultSet(sqlrcursor *cursor, const char *column,
						const char *value) {

	for (uint64_t i=0; i<cursor->rowCount(); i++) {
		if (!charstring::compare(cursor->getField(i,column),value)) {
			stdoutput.printf("%s ",success);
			return;
		}
	}
	stdoutput.printf("%s\n",failure);
	stdoutput.printf("\"%s\" not found in column \"%s\"\n",value,column);
	printErrors();
	status=1;
}

void reportTestStatus() {

	if (status==0) {
		stdoutput.printf(alltestssucceeded);
	} else {
		stdoutput.printf(sometestsfailed);
	}
}
