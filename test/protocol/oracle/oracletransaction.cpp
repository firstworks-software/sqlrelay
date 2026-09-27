// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for #10293: the OCI7 commit, rollback and autocommit-on/off wire
// replies - TTI_COMMIT (0x0e), TTI_ROLLBACK (0x0f), TTI_AUTOCOMMIT_ON (0x0c)
// and TTI_AUTOCOMMIT_OFF (0x0d) - on the non-query3session path, since a
// genuine OCI7 client never sends TTI_QUERY3.  This client never does
// either: every call it makes is TTI_OPEN or the legacy TTI_QUERY/
// TTI_EXECUTE pair, so query3session stays false in
// src/protocols/oracle.cpp for the whole run, the same way
// oraclelegacyfetch.cpp's session does.
//
// The expected bytes are what a real 10.2 server sends, captured with
// oci7transaction.cpp against a real 9i client on both redhat9x86 (native)
// and solaris8sparc (portable) and decoded byte for byte in this ticket's
// "Decode everything and decide" comment:
//
//	call                    native reply                  portable reply
//	commit, rollback        00 00 09 05 00 00 00 (7 bytes) 00 00 09 01 05
//	autocommit on/off       00 00 09 01 00 00 00 (7 bytes) 00 00 09 01 01
//
// Both bytes are the whole data-packet body: two data flag bytes, the ttc
// code (TTC_STATUS, 0x09), then the call status - a fixed 4-byte little
// endian int in native, a length-prefixed int in portable.  Rollback's
// status is a fixed 5, the same value commit sends, whatever was pending
// (or nothing at all - see the empty-rollback variant in the capture);
// autocommit on and off both send a fixed 1.  See samples/
// 10293-redhat9x86-oci7-native-*-realserver.oraproxy and samples/
// 10293-solaris8sparc-oci7-portable-*-realserver.oraproxy for the archived
// captures this test's expected bytes come from.
//
// sqlrprotocol_oracle::sendTransactionResponse() and its shared
// sendOci7StatusResponse() writer (src/protocols/oracle.cpp) are what
// answer all four calls now; before the fix that landed alongside this
// test, rollback and the autocommit calls answered with a full,
// wrong-shaped summary object instead - see samples/
// 10293-redhat9x86-oci7-portable-*-sqlrelay-before.oraproxy for what that
// looked like on the wire.
//
// A dml-commit arm (commit after a real insert, rather than with nothing
// pending) runs below too, on its own connection - see the note near the
// end of main() and #10300.

// the four TTI codes this test drives directly - TTI_AUTOCOMMIT_ON,
// TTI_AUTOCOMMIT_OFF, TTI_COMMIT and TTI_ROLLBACK in src/protocols/
// oracle.cpp.  none of the four are in oracleprotocolclient's own set,
// which only defines the calls its own methods need
static const unsigned char	ORA_TTI_AUTOCOMMIT_ON=0x0c;
static const unsigned char	ORA_TTI_AUTOCOMMIT_OFF=0x0d;
static const unsigned char	ORA_TTI_COMMIT=0x0e;
static const unsigned char	ORA_TTI_ROLLBACK=0x0f;

// the status message's own ttc code - TTC_STATUS in src/protocols/
// oracle.cpp, not one of oracleprotocolclient's ORA_TTC_* constants
static const unsigned char	ORA_TTC_STATUS=0x09;

// commit and rollback both answer with a fixed call status of 5; autocommit
// on and off both answer with a fixed 1 - see the header comment above
static const unsigned char	nativeStatus5[]={
	0x00, 0x00, 0x09, 0x05, 0x00, 0x00, 0x00
};
static const unsigned char	portableStatus5[]={
	0x00, 0x00, 0x09, 0x01, 0x05
};
static const unsigned char	nativeStatus1[]={
	0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x00
};
static const unsigned char	portableStatus1[]={
	0x00, 0x00, 0x09, 0x01, 0x01
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

// judge a legacy parse/execute reply from the dml-commit arm below against
// the decoded summary object - readLegacySummary() and readLegacyError() are
// both in oracleprotocolclient.cpp, the same decoders
// checkLegacySummaryResponse()'s other callers use in oraclelegacyfetch.cpp.
// a mismatch here might be this call's own summary answering the wrong
// shape, or it might be a genuine error - the same leading TTC_ERROR byte
// covers both (#10300) - so a mismatch prints the decoded summary fields
// and, on the chance there's a real ora number and message behind them, the
// decoded error too
static bool checkLegacyDmlStep(oracleprotocolclient *client,
					const char *label,
					uint32_t expectedcursorid,
					unsigned char expectedcommandtype,
					uint32_t expectedrowsprocessed,
					uint32_t expectedsuccessiterations) {

	if (checkLegacySummaryResponse(client,expectedcursorid,
					expectedcommandtype,expectedrowsprocessed,
					expectedsuccessiterations)) {
		return true;
	}

	// re-decode to say what actually came back, rather than just that
	// it didn't match
	uint32_t	cursorid=0;
	unsigned char	commandtype=0;
	uint32_t	rowsprocessed=0;
	uint32_t	successiterations=0;
	bool		leftover=false;
	if (readLegacySummary(client,&cursorid,&commandtype,
					&rowsprocessed,&successiterations,
					&leftover)) {
		stdoutput.printf("  %s: cursor id %d, command type %d, "
				"rows processed %d, success iterations %d%s\n",
				label,(int)cursorid,(int)commandtype,
				(int)rowsprocessed,(int)successiterations,
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

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10293 commit, rollback and autocommit "
						"on/off ======\n\n");

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
	const unsigned char	*status1=(native)?nativeStatus1:portableStatus1;
	size_t			status1size=(native)?
					sizeof(nativeStatus1):
					sizeof(portableStatus1);

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
	// with no transaction in progress, not just after a commit
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
					status1,status1size)) {
		client.disconnect();
		return status;
	}
	if (!checkTransactionCall(&client,"autocommit off",
					ORA_TTI_AUTOCOMMIT_OFF,4,
					status1,status1size)) {
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
	// a DDL statement's command type comes back as 3, the same
	// value a select's parse or execute reports - that's
	// oci7CommandType()'s own fallback in src/protocols/oracle.cpp,
	// not a real Oracle DDL code - #10300's step 1 confirmed the
	// value live
	report("dml commit: create table",
			checkLegacyDmlStep(&dmlclient,
					"dml commit: create table",
					dmlcursorid,3,0,1));

	if (!dmlclient.legacyQuery(dmlcursorid,insertscratchtable) ||
		!dmlclient.legacyExecute(dmlcursorid,1,0)) {
		report("dml commit: insert",false);
		stdoutput.printf("%s\n",dmlclient.getError());
		client.disconnect();
		return status;
	}
	// an insert's execute processes one row and its command
	// type is 2 - #10300's step 1 confirmed both live
	report("dml commit: insert",
			checkLegacyDmlStep(&dmlclient,
					"dml commit: insert",
					dmlcursorid,2,1,1));

	if (!checkTransactionCall(&dmlclient,"dml commit: commit",
					ORA_TTI_COMMIT,1,
					status5,status5size)) {
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
