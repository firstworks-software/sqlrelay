// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for #10293/#10295: the OCI7 commit, rollback and autocommit-on/
// off wire replies - TTI_COMMIT (0x0e), TTI_ROLLBACK (0x0f),
// TTI_AUTOCOMMIT_ON (0x0c) and TTI_AUTOCOMMIT_OFF (0x0d) - on the
// non-query3session path, since a genuine OCI7 client never sends
// TTI_QUERY3.  This client never does either, outside of the query3 arm
// described below: every call it makes is TTI_OPEN or the legacy
// TTI_QUERY/TTI_EXECUTE pair, so query3session stays false in
// src/protocols/oracle.cpp for those sessions, the same way
// oraclelegacyfetch.cpp's session does.
//
// #10295 found that the call status these four calls answer with - and
// open(), close() and every legacy parse/execute summary too - is sticky
// session state, not a value fixed by which call sent it.  It starts at 1;
// a DML with autocommit off sets it to (status&4)|2; a commit, a DDL
// statement or a DML with autocommit on sets it to 5; everything else -
// select, open, close, a non-DDL parse, a failed DML, PL/SQL - reports
// whatever it already was.  ocon/ocof answer with that current status too,
// not a fixed value: this test's own commit and rollback below (both run
// before ocon/ocof, on the same fresh session) leave it at 5, so ocon/ocof
// answer 5 here, not the 1 a session that called them first, with nothing
// before them, would get - see samples/10295-redhat9x86-oci7-native-
// rollback-autocommit-toggle-realserver.oraproxy [0022]/[0024].
//
// The expected bytes below are what a real 10.2 server sends, captured with
// oci7transaction.cpp against a real 9i client on both redhat9x86 (native)
// and solaris8sparc (portable) and decoded byte for byte in this ticket's
// "Decode everything and decide" comment:
//
//	call                     native reply                  portable reply
//	commit, rollback, DDL    00 00 09 05 00 00 00 (7 bytes) 00 00 09 01 05
//
// Both bytes are the whole data-packet body: two data flag bytes, the ttc
// code (TTC_STATUS, 0x09), then the call status - a fixed 4-byte little
// endian int in native, a length-prefixed int in portable.  See samples/
// 10293-redhat9x86-oci7-native-*-realserver.oraproxy, samples/
// 10293-solaris8sparc-oci7-portable-*-realserver.oraproxy and samples/
// 10295-redhat9x86-oci7-native-*-realserver.oraproxy for the archived
// captures this test's expected values come from.
//
// sqlrprotocol_oracle::sendTransactionResponse() and its shared
// sendOci7StatusResponse() writer (src/protocols/oracle.cpp) are what
// answer all four calls now; before the #10293 fix, rollback and the
// autocommit calls answered with a full, wrong-shaped summary object
// instead - see samples/10293-redhat9x86-oci7-portable-*-sqlrelay-
// before.oraproxy for what that looked like on the wire.  open() and
// close() carry the same call status behind their own reply shape - see
// readOpenCallStatus() and readCallStatus() in oracleprotocolclient.cpp -
// and a parse or execute reply carries it as the leading field of the
// legacy summary object readLegacySummary() there decodes.
//
// A dml-commit arm (commit after a real insert, rather than with nothing
// pending) runs below too, on its own connection, now also covering a DDL
// statement's implicit commit and the open/close status it leaves behind -
// see the note near the end of main() and #10300.
//
// A pending-dml arm after it, on a third connection, walks a session
// through 1 -> 2 (a pending insert) -> 5 (rollback) -> 6 (another insert,
// on top of a session a rollback already touched) -> 5 (rollback again),
// checking that open, close and a select in between only ever report
// whatever the status already was.
//
// An other-statements arm after that, on a fourth connection, covers the
// statements #10315 added rules for: a select for update opens a
// transaction like DML does (1 -> 2 from idle, 5 -> 6 later), a merge is
// DML (5 -> 6), and grant and comment are DDL that commit (6 -> 5).
//
// A query3 arm last, on a fifth connection, for #10314, in the portable
// run only.  Its first call after login is a TTI_QUERY3, so query3session
// is true for the whole session, the way it is for python-oracledb.  A
// query3(), a reexecute() or a fetch ends in putSummary()'s summary
// object, whose first field is the call status - readQuery3Summary() in
// oracleprotocolclient.cpp walks to it.  A commit or rollback gets the
// same bare TTC_STATUS message an oci7 session does instead, not that
// summary object - #10321 - which runQuery3TransactionStep() and
// readQuery3TransactionStatus() in oracleprotocolclient.cpp check.  The
// modern rules differ from the OCI7 ones above in one way: a successful
// non-DDL parse clears 0x04, so a DML right after a commit is 2, not 6.
// The walk:
//
//	select 1 -> insert 2 -> commit 5 -> select 1
//	insert 2 -> rollback 5 -> reexecute of the insert 6 -> rollback 5
//	insert 2 -> comment (DDL) 1 -> failing drop 5 -> failing insert 1
//	select with OPTION_COMMIT 5 -> insert with OPTION_COMMIT 5
//	reexecute with the commit-reexecute bit 5 -> rollback 5 -> count 1
//	insert 2 -> open/close 2 -> rollback 5 -> delete with OPTION_COMMIT 5
//
// The count checks that both commit-on-success forms really committed.
// The expected values come from python-oracledb thin against a real 12.2
// server - see samples/10314-dev-pythonoracledb-thin-*-realserver.out and
// callstatustest.py.
//
// All of these arms need a fresh session's first DML to land on a table that
// already exists, or the DDL that creates it would set the status to 5
// before the DML ever ran - see the ordering note on protocoltable10300
// below.
//
// This test instance (test/sqlrelay.conf.d/oracleprotocol.conf.in, instance
// oracleprotocol) sets no autocommit= on its connection string, so every
// session below starts with autocommit off: oracleconnection never
// overrides sqlrserverconnection::getDefaultAutoCommit() (src/server/
// sqlrserverconnection.cpp), which defers to getNativeTransactionModel() -
// SQLRTXMODEL_IMPLICIT for oracle (src/connections/oracle.cpp) - and
// getDefaultAutoCommit() answers false whenever that model is implicit.

// the four TTI codes this test drives directly - TTI_AUTOCOMMIT_ON,
// TTI_AUTOCOMMIT_OFF, TTI_COMMIT and TTI_ROLLBACK in src/protocols/
// oracle.cpp.  none of the four are in oracleprotocolclient's own set,
// which only defines the calls its own methods need
static const unsigned char	ORA_TTI_AUTOCOMMIT_ON=0x0c;
static const unsigned char	ORA_TTI_AUTOCOMMIT_OFF=0x0d;
static const unsigned char	ORA_TTI_COMMIT=0x0e;
static const unsigned char	ORA_TTI_ROLLBACK=0x0f;

// the status message's own ttc code - TTC_STATUS in src/protocols/
// oracle.cpp.  not one of oracleprotocolclient's ORA_TTC_* constants: its
// own readOpenCallStatus()/readCallStatus() use the literal 0x09 instead,
// since several other test programs that include that file already define
// an ORA_TTC_STATUS of their own
static const unsigned char	ORA_TTC_STATUS=0x09;

// commit and rollback both answer with a fixed call status of 5, and so
// does any DDL - see the header comment above
static const unsigned char	nativeStatus5[]={
	0x00, 0x00, 0x09, 0x05, 0x00, 0x00, 0x00
};
static const unsigned char	portableStatus5[]={
	0x00, 0x00, 0x09, 0x01, 0x05
};

// the dml-commit arm's own scratch table, dropped and recreated each run
// the same way oraclelegacyfetch.cpp's sequence is - see dropsequence
// there.  10300 rather than 10293 in the name: the table exists to touch
// the DML-then-commit path #10300 is about, not the transaction-call
// shapes #10293 covers
static const char	*dropscratchtable=
	"drop table protocoltable10300";
static const char	*createscratchtable=
	"create table protocoltable10300 (id number)";
static const char	*insertscratchtable=
	"insert into protocoltable10300 (id) values (10300)";
static const char	*selectscratchtable=
	"select id from protocoltable10300";
static const int64_t	scratchtablevalue=10300;

// #10315's statements: a merge, two DDL verbs determineQueryType() calls
// "etc", and a select for update - all on protocoltable10300.  the merge
// always inserts one row, one past the table's highest id - a merge whose
// ON clause is a constant like (1=0) makes a real 10.2 server drop the
// connection
static const char	*mergescratchtable=
	"merge into protocoltable10300 t "
	"using (select nvl(max(id),0)+1 id from protocoltable10300) s "
	"on (t.id=s.id) when not matched then insert (id) values (s.id)";
static const char	*grantscratchtable=
	"grant select on protocoltable10300 to public";
static const char	*commentscratchtable=
	"comment on table protocoltable10300 is 'protocoltable10300'";
static const char	*selectforupdatescratchtable=
	"select id from protocoltable10300 for update";

// the query3 arm's statements.  its rows get an id of their own, so the
// count only sees them and the delete only removes them.  the bad insert
// fails at execute with ORA-01722, and the drop names a table that doesn't
// exist
static const char	*query3selectdual=
	"select 1 from dual";
static const char	*query3insertscratchtable=
	"insert into protocoltable10300 (id) values (10314)";
static const char	*query3badinsertscratchtable=
	"insert into protocoltable10300 (id) values ('protocoltable10314')";
static const char	*query3dropmissingtable=
	"drop table protocoltable10314missing";
static const char	*query3countscratchtable=
	"select 'rows:'||count(*) from protocoltable10300 where id=10314";
static const char	*query3expectedcount="rows:4";
static const char	*query3deletescratchtable=
	"delete from protocoltable10300 where id=10314";
static const uint32_t	ORA_NO_DATA_FOUND=1403;
static const uint32_t	ORA_TABLE_DOES_NOT_EXIST=942;
static const uint32_t	ORA_INVALID_NUMBER=1722;

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static void hexDump(const char *label,
			const unsigned char *bytes, size_t size) {
	stdoutput.printf("%s (%d bytes):\n",label,(int)size);
	for (size_t i=0; i<size; i++) {
		stdoutput.printf("%02x%s",bytes[i],((i%16)==15)?"\n":" ");
	}
	if (size%16) {
		stdoutput.printf("\n");
	}
}

// compare, and say where and how they parted company rather than just that
// they did
static bool compareBytes(const char *label,
				const unsigned char *actual, size_t actualsize,
				const unsigned char *expected,
				size_t expectedsize) {

	size_t	common=(actualsize<expectedsize)?actualsize:expectedsize;
	for (size_t i=0; i<common; i++) {
		if (actual[i]!=expected[i]) {
			stdoutput.printf("%s: first difference at offset "
					"%d (0x%02x, expected 0x%02x)\n",
					label,(int)i,actual[i],expected[i]);
			hexDump("actual",actual,actualsize);
			hexDump("expected",expected,expectedsize);
			return false;
		}
	}
	if (actualsize!=expectedsize) {
		stdoutput.printf("%s: %d bytes, expected %d\n",
				label,(int)actualsize,(int)expectedsize);
		hexDump("actual",actual,actualsize);
		hexDump("expected",expected,expectedsize);
		return false;
	}
	return true;
}

// TTI_COMMIT/TTI_ROLLBACK/TTI_AUTOCOMMIT_ON/TTI_AUTOCOMMIT_OFF - a sequence
// byte is all any of the four reads (see commit()/rollback()/
// autoCommitOn()/autoCommitOff() in src/protocols/oracle.cpp), and none of
// the four echo it back into the short status reply this test checks
static bool sendTransactionCall(oracleprotocolclient *client,
					unsigned char tti,
					unsigned char sequence) {

	client->beginTtiCall(tti);
	client->appendByte(sequence);

	return client->sendPacket() && client->recvPacket();
}

// one call: send it, report the TTC_STATUS code, and compare the whole
// reply against whichever expected array matches this run's encoding
static bool checkTransactionCall(oracleprotocolclient *client,
					const char *label,
					unsigned char tti,
					unsigned char sequence,
					const unsigned char *expected,
					size_t expectedsize) {

	stdoutput.printf("  -> TTI 0x%02x seq %d\n",(int)tti,(int)sequence);
	if (!sendTransactionCall(client,tti,sequence)) {
		char	message[128];
		charstring::printf(message,sizeof(message),
					"%s: send",label);
		report(message,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	char	message[128];
	charstring::printf(message,sizeof(message),
				"%s: answers TTC_STATUS",label);
	report(message,client->getResponseTtcCode()==ORA_TTC_STATUS);

	charstring::printf(message,sizeof(message),
				"%s: reply matches the real server's",label);
	bool	matched=compareBytes(label,
				client->getResponse(),client->getResponseSize(),
				expected,expectedsize);
	report(message,matched);

	return true;
}

// like checkTransactionCall() above, but judges the reply by its decoded
// status value instead of matching a whole fixed byte array - the
// pending-dml arm below cycles the status through values commit and
// rollback's fixed arrays don't cover
static bool checkTransactionCallStatus(oracleprotocolclient *client,
					const char *label,
					unsigned char tti,
					unsigned char sequence,
					uint32_t expectedstatus) {

	stdoutput.printf("  -> TTI 0x%02x seq %d\n",(int)tti,(int)sequence);
	if (!sendTransactionCall(client,tti,sequence)) {
		char	message[128];
		charstring::printf(message,sizeof(message),
					"%s: send",label);
		report(message,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	uint32_t	callstatus=0;
	bool		decoded=readCallStatus(client,&callstatus);
	char	message[160];
	charstring::printf(message,sizeof(message),
				"%s: call status is %d",label,(int)expectedstatus);
	report(message,decoded && callstatus==expectedstatus);
	if (!decoded || callstatus!=expectedstatus) {
		hexDump(label,client->getResponse(),client->getResponseSize());
	}

	return true;
}

// open()/close() carry the session's call status too, behind their own
// reply shape - readOpenCallStatus() and readCallStatus() in
// oracleprotocolclient.cpp decode it
static bool checkOpenStatus(oracleprotocolclient *client,
					const char *label,
					uint32_t *cursorid,
					uint32_t expectedstatus) {

	char	message[160];
	charstring::printf(message,sizeof(message),"%s: open cursor",label);
	if (!client->open(cursorid)) {
		report(message,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(message,true);

	uint32_t	callstatus=0;
	bool		decoded=readOpenCallStatus(client,&callstatus);
	charstring::printf(message,sizeof(message),
				"%s: open call status is %d",label,(int)expectedstatus);
	report(message,decoded && callstatus==expectedstatus);
	if (!decoded || callstatus!=expectedstatus) {
		hexDump(label,client->getResponse(),client->getResponseSize());
	}
	return true;
}

static bool checkCloseStatus(oracleprotocolclient *client,
					const char *label,
					uint32_t cursorid,
					uint32_t expectedstatus) {

	char	message[160];
	charstring::printf(message,sizeof(message),"%s: close cursor",label);
	if (!client->close(cursorid)) {
		report(message,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(message,true);

	uint32_t	callstatus=0;
	bool		decoded=readCallStatus(client,&callstatus);
	charstring::printf(message,sizeof(message),
				"%s: close call status is %d",label,(int)expectedstatus);
	report(message,decoded && callstatus==expectedstatus);
	if (!decoded || callstatus!=expectedstatus) {
		hexDump(label,client->getResponse(),client->getResponseSize());
	}
	return true;
}

// a throwaway cursor's open and close replies, checked against the
// session's current call status without disturbing whatever cursor the
// caller is still using
static bool checkOpenCloseStatus(oracleprotocolclient *client,
					const char *label,
					uint32_t expectedstatus) {

	uint32_t	cursorid=0;
	return checkOpenStatus(client,label,&cursorid,expectedstatus) &&
			checkCloseStatus(client,label,cursorid,expectedstatus);
}

// judge a legacy parse/execute reply from the dml-commit and pending-dml
// arms below against the decoded summary object - readLegacySummary() and
// readLegacyError() are both in oracleprotocolclient.cpp, the same decoders
// checkLegacySummaryResponse()'s other callers use in oraclelegacyfetch.cpp.
// a mismatch here might be this call's own summary answering the wrong
// shape, or it might be a genuine error - the same leading TTC_ERROR byte
// covers both (#10300) - so a mismatch prints the decoded summary fields,
// including the call status #10295 added, and, on the chance there's a real
// ora number and message behind them, the decoded error too
static bool checkLegacyDmlStep(oracleprotocolclient *client,
					const char *label,
					uint32_t expectedcursorid,
					unsigned char expectedcommandtype,
					uint32_t expectedrowsprocessed,
					uint32_t expectedsuccessiterations,
					uint32_t expectedcallstatus) {

	uint32_t	cursorid=0;
	unsigned char	commandtype=0;
	uint32_t	rowsprocessed=0;
	uint32_t	successiterations=0;
	uint32_t	callstatus=0;
	bool		leftover=false;
	if (readLegacySummary(client,&cursorid,&commandtype,
					&rowsprocessed,&successiterations,
					&leftover,&callstatus) &&
		cursorid==expectedcursorid &&
		commandtype==expectedcommandtype &&
		rowsprocessed==expectedrowsprocessed &&
		successiterations==expectedsuccessiterations &&
		callstatus==expectedcallstatus &&
		!leftover) {
		return true;
	}

	// re-decode to say what actually came back, rather than just that it
	// didn't match - readLegacySummary() rewinds the response itself, so
	// walking the same buffered reply again from the start is safe
	if (readLegacySummary(client,&cursorid,&commandtype,
					&rowsprocessed,&successiterations,
					&leftover,&callstatus)) {
		stdoutput.printf("  %s: cursor id %d, command type %d, "
				"rows processed %d, success iterations %d, "
				"call status %d%s\n",
				label,(int)cursorid,(int)commandtype,
				(int)rowsprocessed,(int)successiterations,
				(int)callstatus,
				(leftover)?", with a message trailing it":"");
	}

	uint32_t	oranum=0;
	char		errmessage[512];
	size_t		errmessagesize=0;
	bool		errleftover=false;
	if (readLegacyError(client,&oranum,errmessage,sizeof(errmessage),
					&errmessagesize,&errleftover)) {
		stdoutput.printf("  %s: ora-%d: %s\n",label,
					(int)oranum,errmessage);
	}
	return false;
}

// parse and execute QUERY, then judge the execute's reply with
// checkLegacyDmlStep() - false only if the calls couldn't be made at all
static bool runLegacyDmlStep(oracleprotocolclient *client,
					const char *label,
					uint32_t cursorid,
					const char *query,
					unsigned char expectedcommandtype,
					uint32_t expectedrowsprocessed,
					uint32_t expectedcallstatus) {

	if (!client->legacyQuery(cursorid,query) ||
		!client->legacyExecute(cursorid,1,0)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,checkLegacyDmlStep(client,label,cursorid,
						expectedcommandtype,
						expectedrowsprocessed,1,
						expectedcallstatus));
	return true;
}

// judge a query3 session's reply - to a query3(), a reexecute(), or a
// commit or rollback - by the summary object readQuery3Summary() in
// oracleprotocolclient.cpp decodes: the ora number it carries, 0 for
// success, and the end of call status.  "cursorid", if not NULL, gets the
// cursor id the summary names
static void checkQuery3Reply(oracleprotocolclient *client,
					const char *label,
					uint32_t expectedoranum,
					uint32_t expectedstatus,
					uint32_t *cursorid=NULL) {

	uint32_t	callstatus=0;
	uint32_t	curid=0;
	uint32_t	oranum=0;
	char		errmessage[512];
	bool		decoded=readQuery3Summary(client,&callstatus,&curid,
						&oranum,errmessage,
						sizeof(errmessage));

	char	message[192];
	charstring::printf(message,sizeof(message),
				"%s: reply decodes",label);
	report(message,decoded);
	if (!decoded) {
		hexDump(label,client->getResponse(),client->getResponseSize());
		return;
	}

	if (expectedoranum) {
		charstring::printf(message,sizeof(message),
					"%s: fails with ora-%05d",
					label,(int)expectedoranum);
	} else {
		charstring::printf(message,sizeof(message),
					"%s: succeeds",label);
	}
	report(message,oranum==expectedoranum);
	if (oranum!=expectedoranum && oranum) {
		stdoutput.printf("  %s: ora-%05d: %s\n",
					label,(int)oranum,errmessage);
	}

	charstring::printf(message,sizeof(message),
				"%s: call status is %d",label,(int)expectedstatus);
	report(message,callstatus==expectedstatus);
	if (callstatus!=expectedstatus) {
		stdoutput.printf("  %s: call status %d\n",
					label,(int)callstatus);
	}

	if (cursorid) {
		*cursorid=curid;
	}
}

// send one query3() and judge its reply with checkQuery3Reply() - false
// only if the call couldn't be made at all
static bool runQuery3Step(oracleprotocolclient *client,
					const char *label,
					uint32_t options,
					uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query,
					uint32_t expectedoranum,
					uint32_t expectedstatus,
					uint32_t *newcursorid=NULL) {

	if (!client->query3(options,cursorid,prefetchrows,query)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	checkQuery3Reply(client,label,expectedoranum,expectedstatus,
								newcursorid);
	return true;
}

// the same for a reexecute() of whatever the cursor last parsed, with no
// binds
static bool runReexecuteStep(oracleprotocolclient *client,
					const char *label,
					uint32_t cursorid,
					uint32_t moreoptions,
					uint32_t expectedstatus) {

	if (!client->reexecute(cursorid,1,ORA_OPTION_EXECUTE,moreoptions,
								0,NULL,0)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	checkQuery3Reply(client,label,0,expectedstatus);
	return true;
}

// and for a commit or rollback, which a query3 session answers with the
// same bare TTC_STATUS an oci7 session gets - #10321 - not the summary
// object this test used to expect.  the reply's end-to-end sequence
// number is always this call's own seq byte plus 1 -
// readQuery3TransactionStatus() in oracleprotocolclient.cpp decodes both
// fields
static bool runQuery3TransactionStep(oracleprotocolclient *client,
					const char *label,
					unsigned char tti,
					unsigned char sequence,
					uint32_t expectedstatus) {

	if (!sendTransactionCall(client,tti,sequence)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	char	message[160];
	charstring::printf(message,sizeof(message),
				"%s: answers TTC_STATUS",label);
	report(message,client->getResponseTtcCode()==ORA_TTC_STATUS);

	uint32_t	callstatus=0;
	uint32_t	endtoendseqnumber=0;
	bool		decoded=readQuery3TransactionStatus(client,
					&callstatus,&endtoendseqnumber);
	uint32_t	expectedseqnumber=(unsigned char)(sequence+1);

	charstring::printf(message,sizeof(message),
				"%s: call status is %d",label,(int)expectedstatus);
	report(message,decoded && callstatus==expectedstatus);

	charstring::printf(message,sizeof(message),
				"%s: end to end seq number is %d",
				label,(int)expectedseqnumber);
	report(message,decoded && endtoendseqnumber==expectedseqnumber);

	if (!decoded || callstatus!=expectedstatus ||
				endtoendseqnumber!=expectedseqnumber) {
		hexDump(label,client->getResponse(),client->getResponseSize());
	}
	return true;
}

// the query3 arm - #10314.  a python-oracledb-style session, whose first
// call after login is a query3, walked through the modern call status
// rules.  false if a call couldn't be made at all, so the arm stopped part
// way through
static bool runQuery3Arm(const char *host, uint16_t port, const char *sid,
				const char *user, const char *password) {

	oracleprotocolclient	q3client;

	if (!q3client.connect(host,port,sid) ||
		!q3client.login(user,password)) {
		report("query3: connect",false);
		stdoutput.printf("%s\n",q3client.getError());
		return false;
	}
	report("query3: connect",true);

	// every statement here goes on the one cursor the first query3 opens
	// - cursor id 0 asks for a new one, and the summary names it.  a
	// select's prefetch of 2 runs past its only row, so its summary
	// carries ORA-01403 as well as the call status
	const uint32_t	parseexecute=ORA_OPTION_PARSE|
					ORA_OPTION_EXECUTE|
					ORA_OPTION_NOPLSQL;
	uint32_t	cursorid=0;
	bool		sent=runQuery3Step(&q3client,"query3: select",
					parseexecute,0,2,
					query3selectdual,
					ORA_NO_DATA_FOUND,1,&cursorid);
	if (sent && !cursorid) {
		report("query3: the select's summary names a cursor",false);
	}
	if (!sent || !cursorid) {
		q3client.disconnect();
		return false;
	}

	// an insert opens a transaction and a commit ends it.  a select's
	// parse after that clears the "a transaction ended" bit
	if (!runQuery3Step(&q3client,"query3: insert",
				parseexecute,cursorid,0,
				query3insertscratchtable,0,2) ||
		!runQuery3TransactionStep(&q3client,"query3: commit",
				ORA_TTI_COMMIT,1,5) ||
		!runQuery3Step(&q3client,"query3: select after commit",
				parseexecute,cursorid,2,
				query3selectdual,ORA_NO_DATA_FOUND,1)) {
		q3client.disconnect();
		return false;
	}

	// a reexecute doesn't parse, so it keeps the bit the rollback before
	// it set: 5 -> 6
	if (!runQuery3Step(&q3client,"query3: second insert",
				parseexecute,cursorid,0,
				query3insertscratchtable,0,2) ||
		!runQuery3TransactionStep(&q3client,"query3: rollback",
				ORA_TTI_ROLLBACK,2,5) ||
		!runReexecuteStep(&q3client,
				"query3: reexecute after rollback",
				cursorid,0,6) ||
		!runQuery3TransactionStep(&q3client,"query3: second rollback",
				ORA_TTI_ROLLBACK,3,5)) {
		q3client.disconnect();
		return false;
	}

	// a ddl commits the insert ahead of it and its own parse clears the
	// bit, so 1.  a failing ddl ends in 5.  a failing dml after that
	// still parsed, so 1
	if (!runQuery3Step(&q3client,"query3: third insert",
				parseexecute,cursorid,0,
				query3insertscratchtable,0,2) ||
		!runQuery3Step(&q3client,"query3: comment",
				parseexecute,cursorid,0,
				commentscratchtable,0,1) ||
		!runQuery3Step(&q3client,"query3: failing drop",
				parseexecute,cursorid,0,
				query3dropmissingtable,
				ORA_TABLE_DOES_NOT_EXIST,5) ||
		!runQuery3Step(&q3client,"query3: failing insert",
				parseexecute,cursorid,0,
				query3badinsertscratchtable,
				ORA_INVALID_NUMBER,1)) {
		q3client.disconnect();
		return false;
	}

	// commit-on-success, on a select, an insert, and a reexecute of that
	// insert.  the rollback after them has nothing left to undo, so the
	// count finds every committed insert: the first, the third (the
	// comment committed it), and these two
	if (!runQuery3Step(&q3client,"query3: select with commit",
				parseexecute|ORA_OPTION_COMMIT,cursorid,2,
				query3selectdual,ORA_NO_DATA_FOUND,5) ||
		!runQuery3Step(&q3client,"query3: insert with commit",
				parseexecute|ORA_OPTION_COMMIT,cursorid,0,
				query3insertscratchtable,0,5) ||
		!runReexecuteStep(&q3client,"query3: reexecute with commit",
				cursorid,ORA_EXEC_OPTION_COMMIT_REEXECUTE,5) ||
		!runQuery3TransactionStep(&q3client,
				"query3: rollback after commits",
				ORA_TTI_ROLLBACK,4,5) ||
		!runQuery3Step(&q3client,"query3: count",
				parseexecute,cursorid,2,
				query3countscratchtable,ORA_NO_DATA_FOUND,1)) {
		q3client.disconnect();
		return false;
	}
	report("query3: the commits took",
			q3client.responseContains(query3expectedcount));

	// open and close echo the status of a transaction in progress
	if (!runQuery3Step(&q3client,"query3: fourth insert",
				parseexecute,cursorid,0,
				query3insertscratchtable,0,2) ||
		!checkOpenCloseStatus(&q3client,
				"query3: open/close while pending",2) ||
		!runQuery3TransactionStep(&q3client,"query3: third rollback",
				ORA_TTI_ROLLBACK,5,5)) {
		q3client.disconnect();
		return false;
	}

	// clean up
	runQuery3Step(&q3client,"query3: delete with commit",
				parseexecute|ORA_OPTION_COMMIT,cursorid,0,
				query3deletescratchtable,0,5);

	q3client.disconnect();
	return true;
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10293/#10295/#10314 commit, rollback, "
						"autocommit on/off and call status "
						"======\n\n");

	bool	native=false;
	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"-native")) {
			native=true;
		}
	}

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

	stdoutput.printf("--- %s encoding ---\n\n",
				(native)?"native":"portable");

	const unsigned char	*status5=(native)?nativeStatus5:portableStatus5;
	size_t			status5size=(native)?
					sizeof(nativeStatus5):
					sizeof(portableStatus5);

	oracleprotocolclient	client;
	client.setNativeEncoding(native);

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

	// four calls with nothing pending - the empty-rollback and
	// autocommit-toggle shapes from the captures.  a fresh session has
	// nothing to commit or roll back either, so this exercises rollback
	// with no transaction in progress, not just after a commit.  commit
	// and rollback both leave the session's call status at 5, and
	// ocon/ocof both echo that same current status rather than a fixed
	// value - see the header comment above - so all four calls here
	// expect 5, not just the first two
	if (!checkTransactionCall(&client,"commit with nothing pending",
					ORA_TTI_COMMIT,1,
					status5,status5size)) {
		client.disconnect();
		return status;
	}
	if (!checkTransactionCall(&client,"rollback with nothing pending",
					ORA_TTI_ROLLBACK,2,
					status5,status5size)) {
		client.disconnect();
		return status;
	}
	if (!checkTransactionCall(&client,"autocommit on",
					ORA_TTI_AUTOCOMMIT_ON,3,
					status5,status5size)) {
		client.disconnect();
		return status;
	}
	if (!checkTransactionCall(&client,"autocommit off",
					ORA_TTI_AUTOCOMMIT_OFF,4,
					status5,status5size)) {
		client.disconnect();
		return status;
	}

	// #10300's dml-commit arm: a real insert on this instance's own
	// backend, then TTI_COMMIT, rather than only the nothing-pending
	// shape above - on its own connection, so nothing above can explain
	// its answer.  #10300's step 1 found no server bug: the "stub error"
	// that ticket opened against is the ordinary summary-object reply to
	// a legacy TTI_QUERY/TTI_EXECUTE, misread as an error because a
	// genuine error and a genuine success share the same leading
	// TTC_ERROR byte - see the header comment on readLegacySummary() in
	// oracleprotocolclient.cpp.  this arm judges every reply through
	// that decoder instead of the ttc code alone.
	oracleprotocolclient	dmlclient;
	dmlclient.setNativeEncoding(native);

	if (!dmlclient.connect(host,port,sid)) {
		report("dml commit: connect",false);
		stdoutput.printf("%s\n",dmlclient.getError());
		client.disconnect();
		return status;
	}
	report("dml commit: connect",true);

	if (!dmlclient.login(user,password)) {
		report("dml commit: login",false);
		stdoutput.printf("%s\n",dmlclient.getError());
		client.disconnect();
		return status;
	}
	report("dml commit: login",true);

	uint32_t	dmlcursorid=0;
	if (!dmlclient.open(&dmlcursorid)) {
		report("dml commit: open cursor",false);
		stdoutput.printf("%s\n",dmlclient.getError());
		client.disconnect();
		return status;
	}
	report("dml commit: open cursor",true);

	// a drop of a table that isn't there errors, so its result is
	// deliberately not checked - the same pattern
	// oraclelegacyfetch.cpp's dropsequence step uses
	dmlclient.legacyQuery(dmlcursorid,dropscratchtable);
	dmlclient.legacyExecute(dmlcursorid,1,0);

	if (!dmlclient.legacyQuery(dmlcursorid,createscratchtable) ||
		!dmlclient.legacyExecute(dmlcursorid,1,0)) {
		report("dml commit: create table",false);
		stdoutput.printf("%s\n",dmlclient.getError());
		client.disconnect();
		return status;
	}
	// a create table's command type is oracle's own code for it, 1 -
	// #10300's step 1 confirmed 3 (a select's) live, before #10319 gave
	// oci7CommandType() the real ddl codes samples/10315-redhat9x86-
	// oci7-native-dml-ddl-misc-realserver.oraproxy pins.  a DDL statement
	// also sets the call status to 5, already at this execute reply,
	// whether or not the statement itself succeeds - #10295, confirmed
	// against samples/10295-redhat9x86-oci7-native-ddl-alone-
	// realserver.oraproxy
	report("dml commit: create table",
			checkLegacyDmlStep(&dmlclient,
					"dml commit: create table",
					dmlcursorid,1,0,1,5));

	if (!dmlclient.legacyQuery(dmlcursorid,insertscratchtable) ||
		!dmlclient.legacyExecute(dmlcursorid,1,0)) {
		report("dml commit: insert",false);
		stdoutput.printf("%s\n",dmlclient.getError());
		client.disconnect();
		return status;
	}
	// an insert's execute processes one row and its command type is 2 -
	// #10300's step 1 confirmed both live.  the call status is 6 here.
	// autocommit is off on this instance (see the header comment above),
	// so a DML's own rule would leave it at (status&4)|2 - but the create
	// table just above already set it to 5, so (5&4)|2 is 6, not the 2 a
	// DML would leave behind on a session that had never run a DDL or a
	// commit - see samples/10295-redhat9x86-oci7-native-rollback-dml-
	// commit-realserver.oraproxy, which shows the same 5 -> 6 transition
	// after a rollback instead of a DDL
	report("dml commit: insert",
			checkLegacyDmlStep(&dmlclient,
					"dml commit: insert",
					dmlcursorid,2,1,1,6));

	if (!checkTransactionCall(&dmlclient,"dml commit: commit",
					ORA_TTI_COMMIT,1,
					status5,status5size)) {
		client.disconnect();
		return status;
	}

	// an open/close pair on this same session still shows the call
	// status the commit just above left behind
	if (!checkOpenCloseStatus(&dmlclient,
					"dml commit: extra open/close",5)) {
		client.disconnect();
		return status;
	}

	// the commit actually took: read the row back from a second,
	// separate connection, rather than from dmlclient's own -
	// if the insert were still only visible within its own
	// session, this select would come back empty
	oracleprotocolclient	verifyclient;
	verifyclient.setNativeEncoding(native);

	if (!verifyclient.connect(host,port,sid) ||
		!verifyclient.login(user,password)) {
		report("dml commit: verify connect",false);
		stdoutput.printf("%s\n",verifyclient.getError());
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	report("dml commit: verify connect",true);

	uint32_t	verifycursorid=0;
	if (!verifyclient.open(&verifycursorid)) {
		report("dml commit: verify open cursor",false);
		stdoutput.printf("%s\n",verifyclient.getError());
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	report("dml commit: verify open cursor",true);

	if (!verifyclient.legacyQuery(verifycursorid,selectscratchtable) ||
		!verifyclient.legacyExecute(verifycursorid,1,0) ||
		!verifyclient.legacyFetch(verifycursorid,0)) {
		report("dml commit: verify select",false);
		stdoutput.printf("%s\n",verifyclient.getError());
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	int64_t	verifyvalues[4];
	size_t	verifyvaluecount=0;
	uint32_t	verifycolcount=0;
	uint32_t	verifyheaderrows=0;
	bool	verifydecoded=readLegacyFetchRows(&verifyclient,
				verifyvalues,
				sizeof(verifyvalues)/
					sizeof(verifyvalues[0]),
				&verifyvaluecount,&verifycolcount,
				&verifyheaderrows);
	report("dml commit: verify fetch response decodes",
			verifydecoded);
	if (!verifydecoded) {
		stdoutput.printf("response (%d bytes):\n",
					(int)verifyclient.getResponseSize());
		stdoutput.safePrint(verifyclient.getResponse(),
					verifyclient.getResponseSize());
		stdoutput.printf("\n");
	} else {
		report("dml commit: verify fetch response has one "
				"column",verifycolcount==1);
		report("dml commit: the committed row is visible "
				"from a separate connection",
				verifyvaluecount==1 &&
				verifyvalues[0]==scratchtablevalue);
	}

	verifyclient.disconnect();

	// the pending-dml arm: a fresh session's own insert, still pending
	// (not yet committed or rolled back), and what open, close, a select
	// and a rollback each report about it - #10295.  it runs here, on
	// protocoltable10300 while dmlclient still hasn't dropped it, so this
	// session's first-ever DML lands on a table that already exists - if
	// this ran on a table it had to create first, the create's own DDL
	// would set the call status to 5 before the insert ever ran, and the
	// 1 -> 2 transition below would never happen
	oracleprotocolclient	pendingclient;
	pendingclient.setNativeEncoding(native);

	if (!pendingclient.connect(host,port,sid)) {
		report("pending dml: connect",false);
		stdoutput.printf("%s\n",pendingclient.getError());
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	report("pending dml: connect",true);

	if (!pendingclient.login(user,password)) {
		report("pending dml: login",false);
		stdoutput.printf("%s\n",pendingclient.getError());
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	report("pending dml: login",true);

	// a fresh session starts at call status 1
	uint32_t	pendingcursorid=0;
	if (!checkOpenStatus(&pendingclient,"pending dml: open cursor",
					&pendingcursorid,1)) {
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	if (!pendingclient.legacyQuery(pendingcursorid,insertscratchtable) ||
		!pendingclient.legacyExecute(pendingcursorid,1,0)) {
		report("pending dml: insert",false);
		stdoutput.printf("%s\n",pendingclient.getError());
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	// autocommit is off (see the header comment above) and nothing has
	// touched this session's call status yet, so the insert's own rule -
	// (status&4)|2 - leaves it at 2
	report("pending dml: insert",
			checkLegacyDmlStep(&pendingclient,
					"pending dml: insert",
					pendingcursorid,2,1,1,2));

	// turning autocommit on and back off while the insert is still
	// pending answers with that same 2 both times - it doesn't commit
	// the pending insert, and it doesn't touch the call status either
	if (!checkTransactionCallStatus(&pendingclient,
					"pending dml: autocommit on while pending",
					ORA_TTI_AUTOCOMMIT_ON,2,2)) {
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	if (!checkTransactionCallStatus(&pendingclient,
					"pending dml: autocommit off while pending",
					ORA_TTI_AUTOCOMMIT_OFF,3,2)) {
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	// a second, throwaway cursor's open and close still show 2 while the
	// insert on pendingcursorid is pending
	if (!checkOpenCloseStatus(&pendingclient,
					"pending dml: open/close while pending",2)) {
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	if (!checkTransactionCall(&pendingclient,"pending dml: rollback",
					ORA_TTI_ROLLBACK,4,
					status5,status5size)) {
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	// open and close after the rollback both show 5
	if (!checkOpenCloseStatus(&pendingclient,
					"pending dml: open/close after rollback",5)) {
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	if (!pendingclient.legacyQuery(pendingcursorid,selectscratchtable) ||
		!pendingclient.legacyExecute(pendingcursorid,1,0)) {
		report("pending dml: select",false);
		stdoutput.printf("%s\n",pendingclient.getError());
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	// a select doesn't change the call status - it just reports whatever
	// the rollback above left behind
	report("pending dml: select",
			checkLegacyDmlStep(&pendingclient,
					"pending dml: select",
					pendingcursorid,3,0,1,5));

	if (!pendingclient.legacyQuery(pendingcursorid,insertscratchtable) ||
		!pendingclient.legacyExecute(pendingcursorid,1,0)) {
		report("pending dml: second insert",false);
		stdoutput.printf("%s\n",pendingclient.getError());
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	// (status&4)|2 on top of the rollback's 5 is 6, the same transition
	// the dml-commit arm's insert-after-create-table shows above
	report("pending dml: second insert",
			checkLegacyDmlStep(&pendingclient,
					"pending dml: second insert",
					pendingcursorid,2,1,1,6));

	if (!checkTransactionCall(&pendingclient,"pending dml: second rollback",
					ORA_TTI_ROLLBACK,5,
					status5,status5size)) {
		pendingclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	pendingclient.disconnect();

	// the other-statements arm: a select for update, a merge and two DDL
	// verbs determineQueryType() calls "etc", each moving the call status
	// the way samples/10315-redhat9x86-oci7-native-*-realserver.oraproxy
	// show a real server's moving - #10315.  it runs on a fresh session,
	// while protocoltable10300 still exists, for the same reason the
	// pending-dml arm does
	oracleprotocolclient	otherclient;
	otherclient.setNativeEncoding(native);

	if (!otherclient.connect(host,port,sid) ||
		!otherclient.login(user,password)) {
		report("other statements: connect",false);
		stdoutput.printf("%s\n",otherclient.getError());
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}
	report("other statements: connect",true);

	uint32_t	othercursorid=0;
	if (!checkOpenStatus(&otherclient,"other statements: open cursor",
						&othercursorid,1)) {
		otherclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	// a select for update takes row locks, so it opens a transaction the
	// way dml does, even from idle - select-for-update-idle [0022].  its
	// command type and rows processed are a select's.  after the
	// rollback, a merge is dml too - rollback-merge-commit [0024].  its
	// command type is oracle's own code for it, 189 - #10319, pinned by
	// samples/10315-redhat9x86-oci7-native-merge-commit-realserver.
	// oraproxy
	if (!runLegacyDmlStep(&otherclient,
				"other statements: select for update from idle",
				othercursorid,selectforupdatescratchtable,
				3,0,2) ||
		!checkTransactionCall(&otherclient,
				"other statements: rollback",
				ORA_TTI_ROLLBACK,1,status5,status5size) ||
		!runLegacyDmlStep(&otherclient,
				"other statements: merge",
				othercursorid,mergescratchtable,189,1,6)) {
		otherclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	// a grant and a comment are both ddl, so each commits the merge
	// before it - dml-ddl-misc [0032] and [0048].  their command types
	// are oracle's own codes too, 17 and 29 - #10319
	if (!runLegacyDmlStep(&otherclient,
				"other statements: grant",
				othercursorid,grantscratchtable,17,0,5) ||
		!runLegacyDmlStep(&otherclient,
				"other statements: second merge",
				othercursorid,mergescratchtable,189,1,6) ||
		!runLegacyDmlStep(&otherclient,
				"other statements: comment",
				othercursorid,commentscratchtable,29,0,5)) {
		otherclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	// a select for update after that is (5&4)|2 -
	// commit-select-for-update [0028]
	if (!runLegacyDmlStep(&otherclient,
				"other statements: select for update",
				othercursorid,selectforupdatescratchtable,
				3,0,6) ||
		!checkTransactionCall(&otherclient,
				"other statements: second rollback",
				ORA_TTI_ROLLBACK,2,status5,status5size)) {
		otherclient.disconnect();
		dmlclient.disconnect();
		client.disconnect();
		return status;
	}

	otherclient.disconnect();

	// the query3 arm, while protocoltable10300 still exists.  no modern
	// client negotiates the native encoding, so it runs in the portable
	// run only
	if (!native) {
		runQuery3Arm(host,port,sid,user,password);
	}

	// leave the backend clean for the next run - not checked,
	// same reasoning as the drop before create table above
	dmlclient.legacyQuery(dmlcursorid,dropscratchtable);
	dmlclient.legacyExecute(dmlcursorid,1,0);
	dmlclient.disconnect();

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
