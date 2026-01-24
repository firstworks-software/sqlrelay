// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern sqlrcon	con;
extern sqlrcur	cur;

void assertEqualsString(const char *value, const char *success) {

	if (!success) {
		if (!value) {
			printf("success ");
			return;
		} else {
			printf("\"%s\"!=\"%s\"",value,success);
			printf("failure: %s",sqlrcur_errorMessage(cur));
			sqlrcur_free(cur);
			sqlrcon_free(con);
			exit(1);
		}
	}

	if (!strcmp(value,success)) {
		printf("success ");
	} else {
		printf("\"%s\"!=\"%s\"",value,success);
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertEqualsStringWithLength(const char *value, const char *success,
							int length) {

	if (!success) {
		if (!value) {
			printf("success ");
			return;
		} else {
			printf("\"%s\"!=\"%s\"",value,success);
			printf("failure: %s",sqlrcur_errorMessage(cur));
			sqlrcur_free(cur);
			sqlrcon_free(con);
			exit(1);
		}
	}

	if (!strncmp(value,success,length)) {
		printf("success ");
	} else {
		printf("\"%s\"!=\"%s\"",value,success);
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertEqualsInt(int value, int success) {

	if (value==success) {
		printf("success ");
	} else {
		printf("\"%d\"!=\"%d\"",value,success);
		printf("failure: %s",sqlrcur_errorMessage(cur));
		sqlrcur_free(cur);
		sqlrcon_free(con);
		exit(1);
	}
}

void assertEqualsDouble(double value, double success) {

	if (value==success) {
		printf("success ");
	} else {
		printf("\"%f\"!=\"%f\"",value,success);
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
