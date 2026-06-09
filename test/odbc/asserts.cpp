// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/regularexpression.h>
#include <rudiments/stdio.h>

int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

void printError(SQLHENV env, SQLHDBC dbc, SQLHSTMT stmt) {
	SQLCHAR		sqlstate[6];
	SQLINTEGER	nativeerror;
	SQLCHAR		msgtext[1024];
	SQLSMALLINT	msgtextlen;
	SQLRETURN	ret=SQLError(env,dbc,stmt,
				sqlstate,&nativeerror,
				msgtext,sizeof(msgtext),&msgtextlen);
	if ((ret==SQL_SUCCESS || ret==SQL_SUCCESS_WITH_INFO) &&
					(nativeerror || msgtextlen)) {
		stdoutput.printf("%s: %d - %.*s\n",
				sqlstate,nativeerror,msgtextlen,msgtext);
	}
}

void assertEqualEnv(SQLHENV env, const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
			printError(env,SQL_NULL_HDBC,SQL_NULL_HSTMT);
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
		printError(env,SQL_NULL_HDBC,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertEqualEnv(SQLHENV env, int actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%d\"!=\"%d\"\n",actual,expected);
		printError(env,SQL_NULL_HDBC,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertTrueEnv(SQLHENV env, bool condition) {

	if (condition) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected true, got false\n");
		printError(env,SQL_NULL_HDBC,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertFalseEnv(SQLHENV env, bool condition) {

	if (!condition) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected false, got true\n");
		printError(env,SQL_NULL_HDBC,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertSuccessEnv(SQLHENV env, SQLRETURN erg) {

	if (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected SQL_SUCCESS or "
				"SQL_SUCCESS_WITH_INFO, got %d\n",erg);
		printError(env,SQL_NULL_HDBC,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertFailureEnv(SQLHENV env, SQLRETURN erg) {

	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected not to get SQL_SUCCESS or "
				"SQL_SUCCESS_WITH_INFO, got %d\n",erg);
		printError(env,SQL_NULL_HDBC,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertEqualDbc(SQLHDBC dbc, const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
			printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
		printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertEqualDbc(SQLHDBC dbc, int actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%d\"!=\"%d\"\n",actual,expected);
		printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertTrueDbc(SQLHDBC dbc, bool condition) {

	if (condition) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected true, got false\n");
		printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertFalseDbc(SQLHDBC dbc, bool condition) {

	if (!condition) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected false, got true\n");
		printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertSuccessDbc(SQLHDBC dbc, SQLRETURN erg) {

	if (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected SQL_SUCCESS or "
				"SQL_SUCCESS_WITH_INFO, got %d\n",erg);
		printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertFailureDbc(SQLHDBC dbc, SQLRETURN erg) {

	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected not to get SQL_SUCCESS or "
				"SQL_SUCCESS_WITH_INFO, got %d\n",erg);
		printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertContainsVersionDbc(SQLHDBC dbc, const char *value) {

	// succeeds if the value contains a version number - a digit, a dot,
	// and a digit ("#.#") anywhere (bare version or banner both pass;
	// empty/text-only fails)
	if (regularexpression::match(value,"[0-9]\\.[0-9]")) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%s\" does not contain a version number\n",
						(value)?value:"(null)");
		printError(SQL_NULL_HENV,dbc,SQL_NULL_HSTMT);
		status=1;
	}
}

void assertEqualStmt(SQLHSTMT stmt, const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
			printError(SQL_NULL_HENV,SQL_NULL_HDBC,stmt);
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%s\"!=\"%s\"\n",actual,expected);
		printError(SQL_NULL_HENV,SQL_NULL_HDBC,stmt);
		status=1;
	}
}

void assertEqualStmt(SQLHSTMT stmt, int actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("\"%d\"!=\"%d\"\n",actual,expected);
		printError(SQL_NULL_HENV,SQL_NULL_HDBC,stmt);
		status=1;
	}
}

void assertTrueStmt(SQLHSTMT stmt, bool condition) {

	if (condition) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected true, got false\n");
		printError(SQL_NULL_HENV,SQL_NULL_HDBC,stmt);
		status=1;
	}
}

void assertFalseStmt(SQLHSTMT stmt, bool condition) {

	if (!condition) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected false, got true\n");
		printError(SQL_NULL_HENV,SQL_NULL_HDBC,stmt);
		status=1;
	}
}

void assertSuccessStmt(SQLHSTMT stmt, SQLRETURN erg) {

	if (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected SQL_SUCCESS or "
				"SQL_SUCCESS_WITH_INFO, got %d\n",erg);
		printError(SQL_NULL_HENV,SQL_NULL_HDBC,stmt);
		status=1;
	}
}

void assertFailureStmt(SQLHSTMT stmt, SQLRETURN erg) {

	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("expected not to get SQL_SUCCESS or "
				"SQL_SUCCESS_WITH_INFO, got %d\n",erg);
		printError(SQL_NULL_HENV,SQL_NULL_HDBC,stmt);
		status=1;
	}
}

void reportTestStatus() {

	if (status==0) {
		stdoutput.printf(alltestssucceeded);
	} else {
		stdoutput.printf(sometestsfailed);
	}
}
