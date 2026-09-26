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
// pending) was tried here and isn't included - see the note near the end
// of main() and #10300.

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

	// a dml-commit arm (a real insert on this instance's own backend,
	// then the same TTI_COMMIT call) was tried here to touch the
	// DML-then-commit path structurally, rather than only the
	// nothing-pending shape above.  it's not included: a plain legacy
	// TTI_QUERY/TTI_EXECUTE pair - the same pair, same query text and
	// cursor pattern oraclelegacyfetch.cpp already runs successfully
	// against this same instance - fails here every time, including on
	// its own fresh connection with none of the four checks above run
	// first.  the cause wasn't found; see #10300, filed separately, for
	// what was ruled out and what's still open. the four checks above
	// are what this ticket needs and they hold up on their own.
	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
