// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for ticket #9599: query3() in src/protocols/oracle.cpp
// used to re-execute a cursor to answer a describe-only TTI_QUERY3 - one
// with OPTION_DESCRIBE and neither OPTION_PARSE nor OPTION_EXECUTE - even
// when that cursor had already been executed and had already sent rows.
// Re-executing rewinds the result set, so a client that describes a
// statement in the middle of fetching it silently starts the result set
// over.  describecanexecute now also requires !columntypescached[curid] and
// !rowssent[curid].
//
// The OCI test in this directory can't reach that.  OCI answers an
// OCI_DESCRIBE_ONLY OCIStmtExecute out of its own client-side cache once a
// statement has executed, so no describe-only request ever reaches the
// listener for an already-executed cursor - reverting the fix leaves that
// test's output unchanged.  So this drives the wire protocol directly
// instead: a hand-built login, then a hand-built TTI_QUERY3 with
// OPTION_DESCRIBE alone against a cursor that is mid-fetch.  See
// oracleprotocolclient.cpp, and test/protocol/tds/tdsdialectguard.cpp for
// the same approach against the analogous tds guard.
//
// The observable difference is not in the describe's own answer.
// sendQuery3Response() forces the row count to 0 whenever OPTION_DESCRIBE
// is set, so a describe answers with column metadata and no rows either
// way.  It is the NEXT fetch that tells the two builds apart: with the
// guard, the cursor is untouched and the fetch returns row two; without it,
// the describe rewound the cursor and the fetch returns row one again.

// two rows, distinguishable, in an order the database has to honour - the
// order by makes ROWONE come back first whatever the plan is
static const char	*query=
	"select decode(level,1,'ROWONE','ROWTWO') c1 "
	"from dual connect by level<=2 order by 1";

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #9599 describe-only re-execute guard "
							"======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521
	// is just the port it was configured with rather than anything a
	// real database is on
	const char	*host="127.0.0.1";
	uint16_t	port=1521;
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	// that instance doesn't have to be on 1521 - two sessions running
	// this suite at once can't both have it.  ORACLEPROTOCOLPORT1 names
	// the port it actually ended up on; it's the same variable
	// oracleprotocol.conf.in's @ORACLEPROTOCOLPORT1@ is generated from,
	// so one value drives both ends.  unset means 1521, as before.
	const char	*portoverride=
			environment::getValue("ORACLEPROTOCOLPORT1");
	if (!charstring::isNullOrEmpty(portoverride)) {
		port=(uint16_t)charstring::convertToInteger(portoverride);
	}

	oracleprotocolclient	client;

	if (!client.connect(host,port,sid)) {
		report("connect",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("connect",true);

	if (!client.login(user,password)) {
		report("login",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("login",true);

	uint32_t	cursorid=0;
	if (!client.open(&cursorid)) {
		report("open cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open cursor",true);

	// parse and execute, asking for one row up front.  the prefetch has
	// to be at least 1: it's what makes rowssent nonzero, and a cursor
	// that has sent no rows is one the guard lets a describe re-execute
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,1,query)) {
		report("execute",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("execute returns the first row",
			client.responseContains("ROWONE") &&
			!client.responseContains("ROWTWO"));

	// the describe-only call this test exists for: OPTION_DESCRIBE and
	// nothing else, on the cursor just executed, with no query text -
	// the request asks about the statement, not for its data
	if (!client.query3(ORA_OPTION_DESCRIBE,cursorid,0,NULL)) {
		report("describe only",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("describe only answers with column metadata",
			client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report("describe only sends no rows",
			!client.responseContains("ROWONE") &&
			!client.responseContains("ROWTWO"));

	// and the assertion the whole test is built around.  pre-fix, the
	// describe above re-executed the cursor and this comes back as
	// ROWONE
	if (!client.fetch(cursorid,1)) {
		report("fetch after describe",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("fetch after describe returns the second row",
			client.responseContains("ROWTWO") &&
			!client.responseContains("ROWONE"));

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
