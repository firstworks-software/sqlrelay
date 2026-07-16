// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>
#include <sqlrelay/sqlrclient.h>

extern sqlrconnection	*con;
extern sqlrcursor	*cur;
extern sqlrconnection	*secondcon;
extern sqlrcursor	*secondcur;

int status=0;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";
const char *alltestssucceeded="\n\033[34mAll tests succeeded\033[0m\n";
const char *sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m\n";

// some platforms (e.g. solaris) crash on printf("%s",NULL); render it safely
static const char *nullSafe(const char *s) {
	return (s)?s:"(null)";
}

void printErrors() {
	if (cur) {
		const char	*err=cur->errorMessage();
		if (err) {
			stdoutput.printf("%s\n",err);
			return;
		}
	}
	if (secondcur) {
		const char	*err=secondcur->errorMessage();
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
	if (secondcon) {
		const char	*err=secondcon->errorMessage();
		if (err) {
			stdoutput.printf("%s\n",err);
			return;
		}
	}
}

void assertEquals(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("%s!=%s\n",nullSafe(actual),nullSafe(expected));
			printErrors();
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=%s\n",nullSafe(actual),nullSafe(expected));
		printErrors();
		status=1;
	}
}

void assertEquals(const char *actual, const char *expected, size_t length) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
		} else {
			stdoutput.printf("%s\n",failure);
			stdoutput.printf("%s!=%s\n",nullSafe(actual),nullSafe(expected));
			printErrors();
			status=1;
		}
		return;
	}

	if (!charstring::compare(actual,expected,length)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=%s\n",nullSafe(actual),nullSafe(expected));
		printErrors();
		status=1;
	}
}

void assertStartsWith(const char *actual, const char *prefix) {

	if (actual && !charstring::compare(actual,prefix,
						charstring::getLength(prefix))) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s doesn't start with %s\n",
					nullSafe(actual),nullSafe(prefix));
		printErrors();
		status=1;
	}
}

void assertEquals(bool actual, bool expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=%s\n",
				(actual)?"true":"false",
				(expected)?"true":"false");
		printErrors();
		status=1;
	}
}

void assertEquals(int32_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%d!=%d\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(uint32_t actual, int expected) {

	if (actual==(uint32_t)expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%d!=%d\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(int64_t actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%lld!=%lld\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(uint64_t actual, int expected) {

	if (actual==(uint64_t)expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%lld!=%lld\n",actual,expected);
		printErrors();
		status=1;
	}
}

// compare pointers as pointers, not as integers - casting a pointer to
// uint64_t and using the (uint64_t,int) overload silently truncated the
// expected value to 32 bits (a (uint64_t,uint64_t) overload instead made
// assertEquals(uint32_t,unsigned long) calls ambiguous)
void assertEquals(const void *actual, const void *expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%p!=%p\n",actual,expected);
		printErrors();
		status=1;
	}
}

void assertEquals(double actual, double expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%f!=%f\n",actual,expected);
		printErrors();
		status=1;
	}
}

// strips a leading '$' and thousands separators, then trailing-zero decimal
// digits, so money values that differ only in formatting compare equal
static char *normalizeMoney(const char *value) {

	char	*result=charstring::duplicate(value);

	// drop '$' and ','
	size_t	j=0;
	for (size_t i=0; result[i]; i++) {
		if (result[i]!='$' && result[i]!=',') {
			result[j++]=result[i];
		}
	}
	result[j]='\0';

	// drop trailing-zero decimals
	if (charstring::contains(result,'.')) {
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
void assertMoneyEquals(const char *actual, const char *expected) {

	if (!expected || !actual) {
		assertEquals(actual,expected);
		return;
	}

	char	*a=normalizeMoney(actual);
	char	*e=normalizeMoney(expected);

	if (!charstring::compare(a,e)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s!=%s\n",nullSafe(actual),nullSafe(expected));
		printErrors();
		status=1;
	}

	delete[] a;
	delete[] e;
}

// compares a rendered money length tolerantly.  old freetds (0.91) renders
// money with 2 decimal places instead of 4, so the length may be 2 short.
void assertMoneyLengthEquals(uint32_t actual, int expected) {

	if (actual==(uint32_t)expected ||
			(expected>=2 && actual==(uint32_t)(expected-2))) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%d!=%d\n",actual,expected);
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

void assertInResultSet(sqlrcursor *cursor, const char *column,
						const char *value) {

	for (uint64_t i=0; i<cursor->rowCount(); i++) {
		if (!charstring::compare(cursor->getField(i,column),value)) {
			stdoutput.printf("%s ",success);
			return;
		}
	}
	stdoutput.printf("%s\n",failure);
	stdoutput.printf("\"%s\" not found in column \"%s\"\n",
				nullSafe(value),nullSafe(column));
	printErrors();
	status=1;
}

void reportTestStatus() {

	if (status==0) {
		stdoutput.printf(alltestssucceeded);
	} else {
		stdoutput.printf(sometestsfailed);
	}
}
