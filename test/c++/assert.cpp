// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>
#include <sqlrelay/sqlrclient.h>

extern sqlrconnection	*con;
extern sqlrcursor	*cur;

void printErrors() {
	if (cur) {
		const char	*err=cur->errorMessage();
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
}

void assertEquals(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("success ");
			return;
		} else {
			stdoutput.printf("failure\n");
			stdoutput.printf("%s!=%s\n",actual,expected);
			printErrors();
			delete cur;
			delete con;
			process::exit(1);
		}
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(const char *actual, const char *expected, size_t length) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("success ");
			return;
		} else {
			stdoutput.printf("failure\n");
			stdoutput.printf("%s!=%s\n",actual,expected);
			printErrors();
			delete cur;
			delete con;
			process::exit(1);
		}
	}

	if (!charstring::compare(actual,expected,length)) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(bool actual, bool expected) {

	if (actual==expected) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(int32_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(uint32_t actual, int expected) {

	if (actual==(uint32_t)expected) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(int64_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(uint64_t actual, int expected) {

	if (actual==(uint64_t)expected) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(double actual, double expected) {

	if (actual==expected) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=%s\n",actual,expected);
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertTrue(bool actual) {

	if (actual) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=true\n",(actual)?"true":"false");
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertFalse(bool actual) {

	if (!actual) {
		stdoutput.printf("success ");
	} else {
		stdoutput.printf("failure\n");
		stdoutput.printf("%s!=true\n",(actual)?"true":"false");
		printErrors();
		delete cur;
		delete con;
		process::exit(1);
	}
}
