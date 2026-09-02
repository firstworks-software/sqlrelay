// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>

extern "C" {
	#define OCIVER_ORACLE
	#include <oci.h>
}

extern OCIError	*err;

int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

void printErrors() {
	text	message[1024];
	bytestring::zero(message,sizeof(message));
	sb4	errcode;
	// only print if the handle actually has a diagnostic in it - a
	// failing assertion that never touched OCI (or ran after the last
	// OCI error was cleared) has nothing relevant to show here
	if (OCIErrorGet(err,1,NULL,&errcode,
			message,sizeof(message),OCI_HTYPE_ERROR)!=OCI_SUCCESS) {
		return;
	}
	message[1023]='\0';
	stdoutput.printf("\n%s\n",message);
}

void assertEquals(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
			printErrors();
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(int actual, int expected) {
	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%d\"!=\"%d\"\n",actual,expected);
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

void reportTestStatus() {
	if (status==0) {
		stdoutput.printf("%s",alltestssucceeded);
	} else {
		stdoutput.printf("%s",sometestsfailed);
	}
}
