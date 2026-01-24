// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>
#include <sqlrelay/sqlrclient.h>

extern sqlrconnection	*con;
extern sqlrcursor	*cur;

void assertEquals(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("expected ");
			return;
		} else {
			stdoutput.printf("%s!=%s\n",actual,expected);
			stdoutput.printf("failure: %s",cur->errorMessage());
			delete cur;
			delete con;
			process::exit(1);
		}
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%s!=%s\n",actual,expected);
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(const char *actual, const char *expected, size_t length) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("expected ");
			return;
		} else {
			stdoutput.printf("%s!=%s\n",actual,expected);
			stdoutput.printf("failure: %s",cur->errorMessage());
			delete cur;
			delete con;
			process::exit(1);
		}
	}

	if (!charstring::compare(actual,expected,length)) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%s!=%s\n",actual,expected);
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(bool actual, bool expected) {

	if (actual==expected) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%s!=%s\n",
				(actual)?"true":"false",
				(expected)?"true":"false");
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(int32_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%d!=%d\n",actual,expected);
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(uint32_t actual, int expected) {

	if (actual==(uint32_t)expected) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%d!=%d\n",actual,expected);
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(int64_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%lld!=%lld\n",actual,expected);
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(uint64_t actual, int expected) {

	if (actual==(uint64_t)expected) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%lld!=%lld\n",actual,expected);
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertEquals(double actual, double expected) {

	if (actual==expected) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%f!=%f\n",actual,expected);
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertTrue(bool actual) {

	if (actual) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%s!=true\n",(actual)?"true":"false");
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}

void assertFalse(bool actual) {

	if (!actual) {
		stdoutput.printf("expected ");
	} else {
		stdoutput.printf("%s!=false\n",(actual)?"true":"false");
		stdoutput.printf("failure: %s",cur->errorMessage());
		delete cur;
		delete con;
		process::exit(1);
	}
}
