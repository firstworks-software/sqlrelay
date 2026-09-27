// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for #10322: TTI_REEXECUTE_AND_FETCH (0x4e),
// python-oracledb thin's TNS_FUNC_REEXECUTE_AND_FETCH.  Before this ticket
// the oracle protocol module had no case for it at all - it fell through to
// sendUnimplementedFunctionError(), ORA-03001 - so a python-oracledb thin
// cursor could parse and run a select once, but running the very same
// select again on the same cursor failed outright.  See
// test/protocol/oracle/samples/10314-dev-pythonoracledb-thin-main-nolob-
// sqlrelay-before.out, packet [0086], for that failure captured against
// the module, and packet [0098] of test/protocol/oracle/samples/10314-dev-
// pythonoracledb-thin-main-realserver.out for what a real 12.2 server sends
// instead - a row header, rows and a summary, in one reply.
//
// reexecuteAndFetch() in src/protocols/oracle.cpp answers it now: the
// request header is byte for byte reexecute()'s (TTI_EXECUTE, 0x04) own -
// a sequence byte, then cursor id, iterations, options and moreoptions, all
// length-prefixed ints - but "iterations" is read as a row count to
// prefetch rather than an array-bind execution count, and the reply folds
// sendFetch3Response()'s row header, rows and summary in behind the
// re-execute, using the "answering an execute" row header flags (0x22)
// sendQuery3Response() uses for its own prefetch, not the "answering a
// fetch" flags (0x02) an ordinary TTI_FETCH gets.
//
// Five cases, each on its own session:
//
//	- a bindless select whose prefetch count exactly matches the rows
//	  available: every row comes back and the summary carries no error
//	- a bindless select whose prefetch count runs past the rows
//	  available: the rows that exist come back, and the summary carries
//	  ORA-01403 (no data found) - the shape of the real capture above
//	- a select with a bind, re-executed with a fresh value and a
//	  prefetch count bigger than the one row data block that goes out
//	  with it.  getQuery3BindValues() reads that field as a row-data
//	  block count for an ordinary reexecute() (TTI_EXECUTE) - #10036/
//	  #10050 taught it to keep reading blocks off the wire until it saw
//	  that many, refilling from the socket in between.  reexecuteAndFetch()
//	  hands it a literal 1 instead of the prefetch count for exactly
//	  this reason: python-oracledb only ever writes one row data block
//	  for TNS_FUNC_REEXECUTE_AND_FETCH (self.num_execs, fixed at 1 by
//	  _create_execute_message() - see execute.pyx), whatever the
//	  prefetch count says, and passing the prefetch count through
//	  unchanged would have this call wait out continuationtimeout for
//	  row data blocks the client is never going to send behind a
//	  five-row prefetch of a one-row bind.  this case is what would hang
//	  or misdecode if that fix regressed
//	- a prefetch count past MAX_FETCH_ROW_COUNT, refused with ORA-20004
//	  before the cursor is touched at all - the same guard query3() and
//	  fetch3() apply to their own row counts - and the session left in
//	  sync behind it, checked by running an ordinary query3 on a fresh
//	  cursor on the same connection afterward
//	- a select with a bind, re-executed with two row data blocks behind
//	  a single TTI_REEXECUTE_AND_FETCH request rather than the one
//	  python-oracledb always sends: refused with ORA-20006 before the
//	  cursor is touched at all, rather than reading only the first block
//	  and silently dropping the second, checked both by running an
//	  ordinary query3 on a fresh cursor afterward, the same way the
//	  over-large prefetch case does, and by fetching from the original
//	  cursor to confirm the rejected request never reached it

// putRowHeader()'s flags byte in src/protocols/oracle.cpp: 0x02 in the
// answer to a fetch and 0x22 in the answer to an execute (or, now, to a
// re-execute combined with a fetch)
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;
static const unsigned char	ORA_ROW_HEADER_FLAGS_EXECUTE=0x22;

// ORA_NO_DATA_FOUND, ORA_MAX_FETCH_ROW_COUNT_EXCEEDED and
// ORA_MULTIPLE_ROW_DATA_BLOCKS in src/protocols/oracle.cpp
static const uint32_t	ORA_NO_DATA_FOUND=1403;
static const uint32_t	ORA_MAX_FETCH_ROW_COUNT_EXCEEDED=20004;
static const uint32_t	ORA_MULTIPLE_ROW_DATA_BLOCKS=20006;

// MAX_FETCH_ROW_COUNT in src/protocols/oracle.cpp
static const uint32_t	ORA_MAX_FETCH_ROW_COUNT=100000;

// room for one decoded row's value, and the most rows any case here fetches
static const size_t	ORA_VALUE_BUFFER_SIZE=64;
static const size_t	ORA_MAX_ROWS=8;

// how wide the bound case's bind is declared
static const uint32_t	ORA_BIND_BUFFER_SIZE=512;

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static void reportResponse(oracleprotocolclient *client) {
	stdoutput.printf("response (%d bytes):\n",
				(int)client->getResponseSize());
	stdoutput.safePrint(client->getResponse(),
				(int32_t)client->getResponseSize());
	stdoutput.printf("\n");
}

// one row of a decoded batch
struct oraclerow {
	unsigned char	value[ORA_VALUE_BUFFER_SIZE];
	size_t		size;
	bool		isnull;
};

// walk a reexecuteAndFetch() reply - the data flags, a row header, one row
// data message per row, and then the trailer, which this stops short of.
// see sendFetch3Response(), putRowHeader() and putRowData() in
// src/protocols/oracle.cpp - this is the same walk readBatchRows() in
// oraclemaxfetchrowcount.cpp does, copied rather than shared since nothing
// here includes that file.
//
// the row header has no fixed size: six of its fields are length prefixed
// counts.  "flags" comes back for the caller to check rather than being
// asserted here, since 0x22 is what this call answers with but 0x02 is
// still a legitimate row header flags byte for other calls
//
// a response carrying no rows at all leads with the summary object instead
// of a row header, which is not a decode failure but an answer of zero rows
static bool readReexecuteAndFetchRows(oracleprotocolclient *client,
				unsigned char *flags,
				uint32_t *colcount,
				uint32_t *headerrowcount,
				oraclerow *rows,
				size_t maxrows,
				size_t *rowcount) {

	client->rewindResponse();

	*flags=0;
	*colcount=0;
	*headerrowcount=0;
	*rowcount=0;

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode)) {
		return false;
	}

	if (ttccode!=ORA_TTC_ROW_HEADER) {
		return true;
	}

	uint32_t	skip=0;
	if (!client->readByte(flags) ||
		!client->readLenPreInt(colcount) ||	// column count
		!client->readLenPreInt(&skip) ||	// iteration number
		!client->readLenPreInt(headerrowcount) ||
		!client->readLenPreInt(&skip) ||	// uac buffer length
		!client->readLenPreInt(&skip) ||	// bit vector size
		!client->readLenPreInt(&skip)) {	// meaning unknown
		return false;
	}

	for (;;) {

		unsigned char	marker=0;
		if (!client->readByte(&marker)) {
			return false;
		}
		if (marker!=ORA_TTC_ROW_DATA) {
			// the trailer, so the rows are done
			return true;
		}

		if (*rowcount>=maxrows) {
			return false;
		}

		oraclerow	*row=&(rows[*rowcount]);
		if (!client->readLenBytes(row->value,sizeof(row->value)-1,
					&(row->size),&(row->isnull))) {
			return false;
		}
		row->value[row->size]='\0';

		(*rowcount)++;
	}
}

static bool rowEquals(const oraclerow *row, const char *expected) {
	return !row->isnull &&
		!charstring::compare((const char *)row->value,expected);
}

// connect, log in and parse+execute QUERY on a fresh cursor, asking for no
// rows on the execute itself - the way python-oracledb's own first execute
// of a select does (s38 in callstatustest.py) - so the following
// reexecuteAndFetch() is the first call to actually fetch anything, the
// same order the real capture shows
static bool beginSelect(oracleprotocolclient *client,
				const char *scenario,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password,
				const char *query,
				const oracleprotocolbind *binds,
				uint32_t bindcount,
				const oracleprotocolbindvalue *values,
				uint32_t *cursorid) {

	char	label[160];

	charstring::printf(label,sizeof(label),"%s: connect",scenario);
	if (!client->connect(host,port,sid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),"%s: login",scenario);
	if (!client->login(user,password)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),"%s: parse and execute",scenario);
	bool	sent=(bindcount)?
		client->query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
					ORA_OPTION_NOPLSQL,
				0,0,query,binds,bindcount,1,values,1):
		client->query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
					ORA_OPTION_NOPLSQL,
				0,0,query);
	if (!sent) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	uint32_t	callstatus=0;
	uint32_t	oranum=0;
	bool	decoded=readQuery3Summary(client,&callstatus,
						cursorid,&oranum);
	report(label,decoded && !oranum && *cursorid);
	if (!decoded || oranum || !*cursorid) {
		reportResponse(client);
		return false;
	}

	return true;
}

// the exact-match case: prefetchrows equals the rows available, so every
// row comes back and the summary carries no error
static void runExactBatch(const char *host, uint16_t port, const char *sid,
				const char *user, const char *password) {

	const char	*scenario="exact batch";
	stdoutput.printf("\n--- %s ---\n\n",scenario);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!beginSelect(&client,scenario,host,port,sid,user,password,
				"select 'row'||level from dual "
					"connect by level<=3",
				NULL,0,NULL,&cursorid)) {
		client.disconnect();
		return;
	}

	char	label[160];
	charstring::printf(label,sizeof(label),
				"%s: reexecute and fetch",scenario);
	if (!client.reexecuteAndFetch(cursorid,3,ORA_OPTION_EXECUTE,0,
						0,NULL,0)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	oraclerow	rows[ORA_MAX_ROWS];
	size_t		rowcount=0;
	bool	decoded=readReexecuteAndFetchRows(&client,&flags,&colcount,
					&headerrowcount,rows,ORA_MAX_ROWS,
					&rowcount);
	charstring::printf(label,sizeof(label),
				"%s: reply decodes",scenario);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: row header flags are 0x22",scenario);
	report(label,flags==ORA_ROW_HEADER_FLAGS_EXECUTE);

	charstring::printf(label,sizeof(label),
				"%s: one column",scenario);
	report(label,colcount==1);

	charstring::printf(label,sizeof(label),
				"%s: row header names 3 rows",scenario);
	report(label,headerrowcount==3);

	charstring::printf(label,sizeof(label),
				"%s: 3 rows came back",scenario);
	report(label,rowcount==3);

	if (rowcount==3) {
		charstring::printf(label,sizeof(label),
					"%s: rows are row1, row2, row3",
					scenario);
		report(label,rowEquals(&rows[0],"row1") &&
				rowEquals(&rows[1],"row2") &&
				rowEquals(&rows[2],"row3"));
	}

	uint32_t	summarycallstatus=0;
	uint32_t	summaryoranum=0;
	decoded=readQuery3Summary(&client,&summarycallstatus,
					NULL,&summaryoranum);
	charstring::printf(label,sizeof(label),
				"%s: summary decodes",scenario);
	report(label,decoded);
	if (decoded) {
		charstring::printf(label,sizeof(label),
					"%s: summary carries no error",
					scenario);
		report(label,!summaryoranum);
	}

	client.disconnect();
}

// the partial case: prefetchrows runs past the one row available, the same
// shape as packet [0097]/[0098] of test/protocol/oracle/samples/10314-dev-
// pythonoracledb-thin-main-realserver.out
static void runPartialBatch(const char *host, uint16_t port, const char *sid,
				const char *user, const char *password) {

	const char	*scenario="partial batch";
	stdoutput.printf("\n--- %s ---\n\n",scenario);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!beginSelect(&client,scenario,host,port,sid,user,password,
				"select 'row'||level from dual "
					"connect by level<=1",
				NULL,0,NULL,&cursorid)) {
		client.disconnect();
		return;
	}

	char	label[160];
	charstring::printf(label,sizeof(label),
				"%s: reexecute and fetch",scenario);
	if (!client.reexecuteAndFetch(cursorid,2,ORA_OPTION_EXECUTE,0,
						0,NULL,0)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	oraclerow	rows[ORA_MAX_ROWS];
	size_t		rowcount=0;
	bool	decoded=readReexecuteAndFetchRows(&client,&flags,&colcount,
					&headerrowcount,rows,ORA_MAX_ROWS,
					&rowcount);
	charstring::printf(label,sizeof(label),
				"%s: reply decodes",scenario);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: row header flags are 0x22",scenario);
	report(label,flags==ORA_ROW_HEADER_FLAGS_EXECUTE);

	charstring::printf(label,sizeof(label),
				"%s: row header names the 2 rows asked for",
				scenario);
	report(label,headerrowcount==2);

	charstring::printf(label,sizeof(label),
				"%s: only 1 row came back",scenario);
	report(label,rowcount==1);
	if (rowcount==1) {
		charstring::printf(label,sizeof(label),
					"%s: the row is row1",scenario);
		report(label,rowEquals(&rows[0],"row1"));
	}

	uint32_t	summarycallstatus=0;
	uint32_t	summaryoranum=0;
	decoded=readQuery3Summary(&client,&summarycallstatus,
					NULL,&summaryoranum);
	charstring::printf(label,sizeof(label),
				"%s: summary decodes",scenario);
	report(label,decoded);
	if (decoded) {
		charstring::printf(label,sizeof(label),
					"%s: summary carries ORA-01403",
					scenario);
		report(label,summaryoranum==ORA_NO_DATA_FOUND);
	}

	client.disconnect();
}

// the bound case: a prefetch count of 5 against a single bind row data
// block, which is what would hang or misdecode if reexecuteAndFetch() read
// the prefetch count as a row-data block count the way reexecute() reads
// its own iterations field - see the header comment above
static void runBoundReexecute(const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	const char	*scenario="bound reexecute";
	stdoutput.printf("\n--- %s ---\n\n",scenario);

	oracleprotocolclient	client;

	oracleprotocolbind	bind;
	bind.varchar(ORA_BIND_BUFFER_SIZE);

	const char	*firstvalue="first value";
	oracleprotocolbindvalue	firstvalues[1];
	firstvalues[0].set(firstvalue);

	uint32_t	cursorid=0;
	if (!beginSelect(&client,scenario,host,port,sid,user,password,
				"select :b from dual",
				&bind,1,firstvalues,&cursorid)) {
		client.disconnect();
		return;
	}

	const char	*secondvalue="a second, different value";
	oracleprotocolbindvalue	secondvalues[1];
	secondvalues[0].set(secondvalue);

	char	label[160];
	charstring::printf(label,sizeof(label),
				"%s: reexecute and fetch",scenario);
	if (!client.reexecuteAndFetch(cursorid,5,ORA_OPTION_EXECUTE,0,
						1,secondvalues,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	oraclerow	rows[ORA_MAX_ROWS];
	size_t		rowcount=0;
	bool	decoded=readReexecuteAndFetchRows(&client,&flags,&colcount,
					&headerrowcount,rows,ORA_MAX_ROWS,
					&rowcount);
	charstring::printf(label,sizeof(label),
				"%s: reply decodes",scenario);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: row header names the 5 rows asked for",
				scenario);
	report(label,headerrowcount==5);

	charstring::printf(label,sizeof(label),
				"%s: 1 row came back",scenario);
	report(label,rowcount==1);

	if (rowcount==1) {
		charstring::printf(label,sizeof(label),
				"%s: the row carries the fresh bind value, "
				"not the one the first execute installed",
				scenario);
		report(label,rowEquals(&rows[0],secondvalue));
	}

	client.disconnect();
}

// the over-large prefetch case: refused with ORA-20004 before the cursor is
// touched, and the session still in sync afterward
static void runOverLargePrefetch(const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	const char	*scenario="over-large prefetch";
	stdoutput.printf("\n--- %s ---\n\n",scenario);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!beginSelect(&client,scenario,host,port,sid,user,password,
				"select 'row'||level from dual "
					"connect by level<=1",
				NULL,0,NULL,&cursorid)) {
		client.disconnect();
		return;
	}

	char	label[160];
	charstring::printf(label,sizeof(label),
				"%s: reexecute and fetch",scenario);
	if (!client.reexecuteAndFetch(cursorid,ORA_MAX_FETCH_ROW_COUNT+1,
					ORA_OPTION_EXECUTE,0,0,NULL,0)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	uint32_t	callstatus=0;
	uint32_t	oranum=0;
	bool	decoded=readQuery3Summary(&client,&callstatus,NULL,&oranum);
	charstring::printf(label,sizeof(label),
				"%s: reply decodes",scenario);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: refused with ORA-20004",scenario);
	report(label,oranum==ORA_MAX_FETCH_ROW_COUNT_EXCEEDED);

	// the session is still in sync - a fresh query3 on a new cursor,
	// on the same connection, still gets an ordinary answer
	charstring::printf(label,sizeof(label),
				"%s: session still in sync afterward",
				scenario);
	if (!client.query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,0,0,
				"select 'stillalive' from dual")) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	uint32_t	newcursorid=0;
	uint32_t	neworanum=0;
	decoded=readQuery3Summary(&client,&callstatus,
					&newcursorid,&neworanum);
	report(label,decoded && !neworanum && newcursorid &&
					newcursorid!=cursorid);
	if (!decoded || neworanum || !newcursorid) {
		reportResponse(&client);
	}

	client.disconnect();
}

// the multiple-row-data-block case: two row data blocks behind a single
// TTI_REEXECUTE_AND_FETCH request, something no real python-oracledb thin
// client ever sends (it always writes exactly one, fixed by self.num_execs -
// see the header comment above).  #10330: refused with ORA-20006 before the
// cursor is touched, rather than reading only the first block and silently
// dropping the second - checked by fetching from the original cursor
// afterward and confirming the rejected request never reached it, and by
// the session still being in sync, the same proof runOverLargePrefetch()
// above uses
static void runMultipleRowDataBlocks(const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	const char	*scenario="multiple row data blocks";
	stdoutput.printf("\n--- %s ---\n\n",scenario);

	oracleprotocolclient	client;

	oracleprotocolbind	bind;
	bind.varchar(ORA_BIND_BUFFER_SIZE);

	const char	*firstvalue="first value";
	oracleprotocolbindvalue	firstvalues[1];
	firstvalues[0].set(firstvalue);

	uint32_t	cursorid=0;
	if (!beginSelect(&client,scenario,host,port,sid,user,password,
				"select :b from dual",
				&bind,1,firstvalues,&cursorid)) {
		client.disconnect();
		return;
	}

	// two row data blocks, block-major, one bind each - the second
	// block's value should never actually be read
	oracleprotocolbindvalue	values[2];
	values[0].set("first block value");
	values[1].set("second block value");

	char	label[160];
	charstring::printf(label,sizeof(label),
				"%s: reexecute and fetch",scenario);
	if (!client.reexecuteAndFetch(cursorid,1,ORA_OPTION_EXECUTE,0,
						1,values,2)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	uint32_t	callstatus=0;
	uint32_t	oranum=0;
	bool	decoded=readQuery3Summary(&client,&callstatus,NULL,&oranum);
	charstring::printf(label,sizeof(label),
				"%s: reply decodes",scenario);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: refused with ORA-20006",scenario);
	report(label,oranum==ORA_MULTIPLE_ROW_DATA_BLOCKS);

	// the rejected request never reached the cursor - an ordinary fetch
	// on it still comes back with the value the first execute installed,
	// not a re-execution against either of the two blocks the rejected
	// request tried to send
	charstring::printf(label,sizeof(label),
				"%s: original cursor still holds its first "
				"value",scenario);
	if (!client.fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	unsigned char	fetchflags=0;
	uint32_t	fetchcolcount=0;
	uint32_t	fetchheaderrowcount=0;
	oraclerow	fetchrows[ORA_MAX_ROWS];
	size_t		fetchrowcount=0;
	decoded=readReexecuteAndFetchRows(&client,&fetchflags,&fetchcolcount,
					&fetchheaderrowcount,fetchrows,
					ORA_MAX_ROWS,&fetchrowcount);
	report(label,decoded && fetchrowcount==1 &&
				rowEquals(&fetchrows[0],firstvalue));
	if (!decoded || fetchrowcount!=1 ||
				!rowEquals(&fetchrows[0],firstvalue)) {
		reportResponse(&client);
	}

	// the session is still in sync - a fresh query3 on a new cursor,
	// on the same connection, still gets an ordinary answer
	charstring::printf(label,sizeof(label),
				"%s: session still in sync afterward",
				scenario);
	if (!client.query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,0,0,
				"select 'stillalive' from dual")) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	uint32_t	newcursorid=0;
	uint32_t	neworanum=0;
	decoded=readQuery3Summary(&client,&callstatus,
					&newcursorid,&neworanum);
	report(label,decoded && !neworanum && newcursorid &&
					newcursorid!=cursorid);
	if (!decoded || neworanum || !newcursorid) {
		reportResponse(&client);
	}

	client.disconnect();
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10322 TTI_REEXECUTE_AND_FETCH (0x4e) "
						"======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  ORACLEPROTOCOLPORT1
	// names the port it ended up on, the same variable that config is
	// generated from; unset means the configure-time default
	const char	*host="127.0.0.1";
	uint16_t	port=1521;
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	const char	*portoverride=
			environment::getValue("ORACLEPROTOCOLPORT1");
	if (!charstring::isNullOrEmpty(portoverride)) {
		port=(uint16_t)charstring::convertToInteger(portoverride);
	}

	runExactBatch(host,port,sid,user,password);
	runPartialBatch(host,port,sid,user,password);
	runBoundReexecute(host,port,sid,user,password);
	runOverLargePrefetch(host,port,sid,user,password);
	runMultipleRowDataBlocks(host,port,sid,user,password);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
