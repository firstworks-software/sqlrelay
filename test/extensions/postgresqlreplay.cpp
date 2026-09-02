// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/file.h>
#include <sqlrelay/sqlrclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

#include "../c++/asserts.cpp"

// Proves that <condition sqlstate="..."/> on a replay trigger actually
// matches on sqlrservercontroller's sqlstate buffer, rather than falling
// through unmatched (in which case replay would just report the original
// error back with no log file written at all).
//
// The instance's trigger is when="after", which only runs once a query has
// executed - a query that fails during prepare never reaches it. postgresql
// resolves table/function references, and coerces literal casts, during
// prepare (parse analysis), so those more obvious error choices are no good
// here. Division-by-zero and integer overflow are only caught once the
// executor evaluates the expression (postgresql constant-folds immutable
// operators during planning, which happens on execute, not prepare), so
// they're what reliably exercises this trigger's sqlstate matching, unlike
// the deadlock/lock-wait-timeout scope="transaction" cases
// mysqldeadlockreplay.cpp covers.
//
// Each condition below is provoked by a query that reliably reproduces its
// exact sqlstate, and its own log file only gets written when
// replayCondition() picks that condition's tag - so a file that's absent
// after its query ran means matching failed, and a file that's present with
// the wrong "matching sqlstate pattern:" line means the wrong condition
// fired.
//
// This intentionally stops short of asserting what sendQuery() itself
// returns. Once replayCondition() matches, control passes to replay(),
// whose own retry-and-give-up bookkeeping is a separate mechanism this file
// isn't testing.
const char	*divisionbyzerolog=
		"/tmp/postgresqlreplay-sqlstate-divisionbyzero.log";
const char	*outofrangelog=
		"/tmp/postgresqlreplay-sqlstate-outofrange.log";

void assertLogContains(const char *filename, const char *needle) {
	if (!file::exists(filename)) {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s does not exist\n",filename);
		status=1;
		return;
	}
	char	*contents=file::getContents(filename);
	if (contents && charstring::contains(contents,needle)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s does not contain \"%s\":\n%s\n",
				filename,needle,(contents)?contents:"(null)");
		status=1;
	}
	delete[] contents;
}

void assertLogAbsent(const char *filename) {
	if (!file::exists(filename)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s exists but shouldn't\n",filename);
		status=1;
	}
}

int main(int argc, char **argv) {

	stdoutput.printf("REPLAY SQLSTATE MATCHING:\n");

	// clean slate - a previous run (or a stale file left behind by a
	// crash) could otherwise make either check a false pass
	file::remove(divisionbyzerolog);
	file::remove(outofrangelog);

	con=new sqlrconnection("sqlrelay",9037,"/tmp/postgresqlreplay.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// don't assert on sendQuery()'s return value here - the replay
	// trigger's own retry-and-give-up mechanics (independent of, and out
	// of scope for, sqlstate matching) are what ultimately decide it.
	// What this test cares about is only replayCondition() itself:
	// did it pick the right <condition> tag for the error that came
	// back, which is what actually writes the log file.
	stdoutput.printf("DIVISION_BY_ZERO (22012) MATCHES ITS CONDITION:\n");
	cur->sendQuery("select 1/0");
	assertLogContains(divisionbyzerolog,
				"matching sqlstate pattern: 22012");
	stdoutput.printf("\n");

	stdoutput.printf("DIVISION_BY_ZERO DOES NOT MATCH THE OTHER "
				"CONDITION (NEGATIVE CONTROL):\n");
	assertLogAbsent(outofrangelog);
	stdoutput.printf("\n");

	stdoutput.printf("NUMERIC_VALUE_OUT_OF_RANGE (22003) MATCHES ITS "
				"CONDITION:\n");
	cur->sendQuery("select 2147483647 * 2");
	assertLogContains(outofrangelog,
				"matching sqlstate pattern: 22003");
	stdoutput.printf("\n");

	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
