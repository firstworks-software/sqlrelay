// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

void printErrors() {
}

// line-tag every assertion call site so failures in the (very long) test
// driver can be mapped back to source, without touching each call site by
// hand: assertEquals/assertTrue/assertFalse become macros (below) that
// forward __LINE__ to the *Impl functions.
void assertEqualsImpl(const char *actual, const char *expected, int line) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("tds.cpp:%d: \"%s\"!=\"%s\"\n",
						line,actual,expected);
			printErrors();
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("tds.cpp:%d: \"%s\"!=\"%s\"\n",
					line,actual,expected);
		printErrors();
		status=1;
	}
}

void assertEqualsImpl(int actual, int expected, int line) {
	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("tds.cpp:%d: \"%d\"!=\"%d\"\n",
					line,actual,expected);
		printErrors();
		status=1;
	}
}

void assertTrueImpl(bool actual, int line) {
	if (actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("tds.cpp:%d: %s!=true\n",
					line,(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

void assertFalseImpl(bool actual, int line) {
	if (!actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("tds.cpp:%d: %s!=false\n",
					line,(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

#define assertEquals(actual,expected) assertEqualsImpl(actual,expected,__LINE__)
#define assertTrue(actual) assertTrueImpl(actual,__LINE__)
#define assertFalse(actual) assertFalseImpl(actual,__LINE__)

void reportTestStatus() {
	if (status==0) {
		stdoutput.printf("%s",alltestssucceeded);
	} else {
		stdoutput.printf("%s",sometestsfailed);
	}
}
