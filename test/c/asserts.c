// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern sqlrcon	con;
extern sqlrcur	cur;

int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[33mSome tests failed\033[0m\n";

void printErrors() {
	if (cur) {
		const char *err=sqlrcur_errorMessage(cur);
		if (err) {
			printf("%s\n",err);
			return;
		}
	}
	if (con) {
		const char *err=sqlrcon_errorMessage(con);
		if (err) {
			printf("%s\n",err);
			return;
		}
	}
}

int contains(const char *actual, const char *substring) {

	// a null actual (e.g. a null field from a failed or empty query)
	// contains nothing, and isn't a reason to crash inside strstr()
	return (actual)?(strstr(actual,substring)!=NULL):0;
}

void assertEqStr(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			printf("%s ",success);
		} else {
			printf("%s\n",failure);
			printf("\"%s\"!=\"%s\"\n",actual,expected);
			printErrors();
			status=1;
		}
		return;
	}

	// a null actual (e.g. a null field from a failed or empty query) is a
	// mismatch, not a reason to crash
	if (!actual) {
		printf("%s\n",failure);
		printf("\"(null)\"!=\"%s\"\n",expected);
		printErrors();
		status=1;
		return;
	}

	if (!strcmp(actual,expected)) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%s\"!=\"%s\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEqStrLen(const char *actual, const char *expected,
					int length) {

	if (!expected) {
		if (!actual) {
			printf("%s ",success);
		} else {
			printf("%s\n",failure);
			printf("\"%s\"!=\"%s\"\n",actual,expected);
			printErrors();
			status=1;
		}
		return;
	}

	// a null actual (e.g. a null field from a failed or empty query) is a
	// mismatch, not a reason to crash
	if (!actual) {
		printf("%s\n",failure);
		printf("\"(null)\"!=\"%s\"\n",expected);
		printErrors();
		status=1;
		return;
	}

	if (!strncmp(actual,expected,length)) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%s\"!=\"%s\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEqInt(int actual, int expected) {

	if (actual==expected) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%d\"!=\"%d\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEqDbl(double actual, double expected) {

	if (actual==expected) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%f\"!=\"%f\"\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertTrue(int actual) {

	if (actual) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("%s!=true\n",(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

void assertFalse(int actual) {

	if (!actual) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("%s!=false\n",(actual)?"true":"false");
		printErrors();
		status=1;
	}
}

void assertInResultSet(sqlrcur cursor, const char *column,
						const char *value) {

	uint64_t	i;
	const char	*field;

	for (i=0; i<sqlrcur_rowCount(cursor); i++) {
		field=sqlrcur_getFieldByName(cursor,i,column);
		if (field && !strcmp(field,value)) {
			printf("%s ",success);
			return;
		}
	}
	printf("%s\n",failure);
	printf("\"%s\" not found in column \"%s\"\n",value,column);
	printErrors();
	status=1;
}

void reportTestStatus() {

	if (status==0) {
		printf(alltestssucceeded);
	} else {
		printf(sometestsfailed);
	}
}
