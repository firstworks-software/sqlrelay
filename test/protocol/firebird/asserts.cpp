// Copyright (c) David Muse
// See the file COPYING for more information.

#include <ibase.h>
#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

extern ISC_STATUS fbstatus[20];

int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

// fb_interpret (firebird 2.0+) supersedes the deprecated isc_interprete,
// whose sizeless buffer walk can overflow msg
static ISC_LONG fbInterpret(char *msg, unsigned int msgsize,
					const ISC_STATUS **pvector) {
#ifdef HAVE_FB_INTERPRET
	return fb_interpret(msg,msgsize,pvector);
#else
	// isc_interprete takes a non-const ISC_STATUS**; it only advances the
	// walking pointer, so dropping const is safe
	return isc_interprete(msg,(ISC_STATUS **)pvector);
#endif
}

void printErrors() {

	// isc_arg_gds followed by 0 means no error
	if (fbstatus[0]!=isc_arg_gds || !fbstatus[1]) {
		return;
	}

	char			msg[512];
	const ISC_STATUS	*sv=fbstatus;
	while (fbInterpret(msg,sizeof(msg),&sv)) {
		stdoutput.printf("%s\n",msg);
	}
	stdoutput.printf("sqlcode: %d\n",(int)isc_sqlcode(fbstatus));
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
