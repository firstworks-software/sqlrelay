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
	return (actual && substring)?(strstr(actual,substring)!=NULL):0;
}

void assertContains(const char *actual, const char *substring) {

	// a null actual (e.g. a null field from a failed or empty query) is a
	// mismatch, not a reason to crash
	if (!actual) {
		printf("%s\n",failure);
		printf("\"(null)\" doesn't contain \"%s\"\n",substring);
		printErrors();
		status=1;
		return;
	}

	if (contains(actual,substring)) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%s\" doesn't contain \"%s\"\n",actual,substring);
		printErrors();
		status=1;
	}
}

void assertNotContains(const char *actual, const char *substring) {

	// A null actual has to fail here rather than pass.  It contains no
	// substring, so it would satisfy a not-contains test, but a null field
	// means the query returned nothing and the test never ran.  Passing on
	// it would hide the empty result set instead of reporting it.
	if (!actual) {
		printf("%s\n",failure);
		printf("\"(null)\" - no data to check for \"%s\"\n",substring);
		printErrors();
		status=1;
		return;
	}

	if (!contains(actual,substring)) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%s\" contains \"%s\"\n",actual,substring);
		printErrors();
		status=1;
	}
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

void assertEqBin(const char *actual, const void *expected, int length) {

	// a null actual (e.g. a null field from a failed or empty query) is a
	// mismatch, not a reason to crash
	if (!actual) {
		printf("%s\n",failure);
		printf("\"(null)\"!=(%d bytes)\n",length);
		printErrors();
		status=1;
		return;
	}

	// The data is binary and can contain nulls, so neither side can be
	// printed as a string.  Report the length instead.
	if (!memcmp(actual,expected,length)) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("(%d bytes) don't match\n",length);
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

// strips a leading '$' and thousands separators, then trailing-zero decimals,
// so money values that differ only in formatting compare equal.  the returned
// string is malloc'd; the caller frees it.
static char *normalizeMoney(const char *value) {

	char	*result;
	int	i;
	int	j;

	result=(char *)malloc(strlen(value)+1);
	strcpy(result,value);

	// drop '$' and ','
	j=0;
	for (i=0; result[i]; i++) {
		if (result[i]!='$' && result[i]!=',') {
			result[j++]=result[i];
		}
	}
	result[j]='\0';

	// drop trailing-zero decimals
	if (strchr(result,'.')) {
		while (j>0 && result[j-1]=='0') {
			result[--j]='\0';
		}
		if (j>0 && result[j-1]=='.') {
			result[--j]='\0';
		}
	}
	return result;
}

// compares money values tolerantly - a leading '$', thousands separators, and
// trailing-zero decimals are ignored, so "1.00", "$1.00", and "1.0000" all
// match.  a real precision difference like "1.23" vs "1.2345" still fails.
// old freetds (0.91) renders money with 2 decimal places instead of 4.
void assertMoneyEqStr(const char *actual, const char *expected) {

	char	*a;
	char	*e;

	if (!expected || !actual) {
		assertEqStr(actual,expected);
		return;
	}

	a=normalizeMoney(actual);
	e=normalizeMoney(expected);

	if (!strcmp(a,e)) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%s\"!=\"%s\"\n",actual,expected);
		printErrors();
		status=1;
	}

	free(a);
	free(e);
}

// compares a rendered money length tolerantly.  old freetds (0.91) renders
// money with 2 decimal places instead of 4, so the length may be 2 short.
void assertMoneyEqLen(int actual, int expected) {

	if (actual==expected || (expected>=2 && actual==expected-2)) {
		printf("%s ",success);
	} else {
		printf("%s\n",failure);
		printf("\"%d\"!=\"%d\"\n",actual,expected);
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
