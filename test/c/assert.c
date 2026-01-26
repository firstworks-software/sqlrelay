// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern sqlrcon	con;
extern sqlrcur	cur;

void assertEqualsString(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			printf("success ");
			return;
		} else {
			printf("\"%s\"!=\"%s\"",actual,expected);
			printf("failure: %s",sqlrcur_errorMessage(cur));
			sqlrcur_free(cur);
			sqlrcon_free(con);
			exit(1);
		}
	}

	if (!strcmp(actual,expected)) {
		printf("success ");
	} else {
		printf("\"%s\"!=\"%s\"",actual,expected);
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertEqualsStringWithLength(const char *actual, const char *expected,
							int length) {

	if (!expected) {
		if (!actual) {
			printf("success ");
			return;
		} else {
			printf("\"%s\"!=\"%s\"",actual,expected);
			printf("failure: %s",sqlrcur_errorMessage(cur));
			sqlrcur_free(cur);
			sqlrcon_free(con);
			exit(1);
		}
	}

	if (!strncmp(actual,expected,length)) {
		printf("success ");
	} else {
		printf("\"%s\"!=\"%s\"",actual,expected);
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertEqualsInt(int actual, int expected) {

	if (actual==expected) {
		printf("success ");
	} else {
		printf("\"%d\"!=\"%d\"",actual,expected);
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertEqualsDouble(double actual, double expected) {

	if (actual==expected) {
		printf("success ");
	} else {
		printf("\"%f\"!=\"%f\"",actual,expected);
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertTrue(int actual) {

	if (actual) {
		printf("success ");
	} else {
		printf("%s!=true\n",(actual)?"true":"false");
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertFalse(int actual) {

	if (!actual) {
		printf("success ");
	} else {
		printf("%s!=false\n",(actual)?"true":"false");
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}
