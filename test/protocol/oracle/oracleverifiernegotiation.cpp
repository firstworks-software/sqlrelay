// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for the oracle protocol module's logon negotiation - #10515.
//
// The "serverversion" listener attribute is a ceiling.  It fixes what goes
// out before any login - the capability field version and the logon types
// the module advertises - and the newest verifier any client is offered.
// Which verifier a client actually gets is picked per client, in phase one
// of each login, by chooseVerifierType() in src/protocols/oracle.cpp:
//
//	- o3logon for a 0x52 classic login, for a client whose
//	  CCAP_LOGON_TYPES byte lacks O5LOGON (0x08), and for every client
//	  at 10.2.  the 10g verifier for a client that sets O5LOGON, the 9i
//	  one for a client that doesn't
//	- otherwise o5logon: the 12c verifier at 12.1 for a client that sets
//	  O7LOGON (0x20), and the 11g verifier for any other
//
// and the "allowedlogonversion" listener attribute is a floor under it,
// which refuses a login whose verifier is older than it allows with
// ORA-28040, in place of the challenge.
//
// No client on hand can cover this on its own.  Each real client sends the
// one logon types byte it was built with, and the genuine 9i clients that
// send 0x05 only run on the buildfarm.  So this sends phase one of the login
// by hand, advertising each of three bytes in turn, to listeners at each
// ceiling and to two floor listeners:
//
//	- 0x05, a genuine 9i client's legacy bits alone
//	- 0x08, O5LOGON alone, an 11g client's
//	- 0x2a, O7LOGON, O5LOGON and O5LOGON_NP, a 12c client's
//
// and checks the challenge each one gets: its shape, the verifier type in
// AUTH_VFR_DATA's flags, the session key's length, the pbkdf2 fields, and
// the call number in its trailer, which has to echo the request's sequence
// number.  A 9i challenge's trailer is the oci7 summary, which has no end to
// end field, since that is how a genuine 9i client reads it.  It also checks
// the logon types and field version each listener advertises, and completes
// the login wherever the 11g verifier comes back, since that is the one
// exchange oracleprotocolclient can finish.
//
// The listeners are all on the oracleprotocol instance - see
// test/sqlrelay.conf.d/oracleprotocol.conf.in - except the 11.2 one, which is
// oracleprotocol11g's.
//
// -native runs the same cases in the native encoding, which is where the
// trailer is a marshalled struct with the call number at offset 49.  The
// module reads and answers a 0x76 o3logon login in the portable encoding
// only, so a native run checks what those cases advertise and stops there.

// the verifier types - the VERIFIER_TYPE_* defines in
// src/protocols/oracle.cpp.  only the two o5logon ones go out on the wire
static const uint32_t	ORA_VERIFIER_TYPE_11G=0x1b25;
static const uint32_t	ORA_VERIFIER_TYPE_12C=0x4815;

// CCAP_FIELD_VERSION_10_2 and CCAP_FIELD_VERSION_12_1 in
// src/protocols/oracle.cpp
static const unsigned char	ORA_CCAP_FIELD_VERSION_10_2=4;
static const unsigned char	ORA_CCAP_FIELD_VERSION_12_1=7;

// the size of the compile capabilities a real 10.2 server sends, which is
// what the module sends at 10.2 - ttiservercompilecaps9i there
static const size_t	ORA_CCAP_SIZE_10_2=33;

// the logon types putTti6Response() advertises: 0x0d below 12.1, and that
// plus CCAP_O7LOGON and CCAP_O5LOGON_NP at 12.1
static const unsigned char	ORA_SERVER_LOGON_TYPES=0x0d;
static const unsigned char	ORA_SERVER_LOGON_TYPES_12_1=
				ORA_SERVER_LOGON_TYPES|
				ORA_CCAP_O7LOGON|ORA_CCAP_O5LOGON_NP;

// the logon types bytes the cases advertise
static const unsigned char	LOGON_TYPES_9I=0x05;
static const unsigned char	LOGON_TYPES_11G=ORA_CCAP_O5LOGON;
static const unsigned char	LOGON_TYPES_12C=
				ORA_CCAP_O7LOGON|ORA_CCAP_O5LOGON|
				ORA_CCAP_O5LOGON_NP;

// ORA_NO_MATCHING_AUTH_PROTOCOL in src/protocols/oracle.cpp
static const uint32_t	ORA_NO_MATCHING_AUTH_PROTOCOL=28040;

// the end to end field putAuthTrailer() writes: the login phase, 2 in the
// challenge and 3 in the login answer
static const unsigned char	ORA_AUTH_PHASE_CHALLENGE=2;
static const unsigned char	ORA_AUTH_PHASE_ANSWER=3;

// where the call number sits in each encoding's trailer, counted from its
// ttc code
static const size_t	ORA_TRAILER_CALL_NUMBER=24;
static const size_t	ORA_NATIVE_TRAILER_CALL_NUMBER=49;

// the portable trailer: the ttc code, the end of call status and the end to
// end field as count prefixed ints, 19 zero fields, the call number and 6
// more zero fields.  this client negotiates field version 11.2, so the 12.1
// summary extension stays out of it
static const size_t	ORA_TRAILER_SIZE=31;

// the portable trailer of a 9i challenge, the oci7 summary: the ttc code,
// the end of call status, 19 zero fields with no end to end field among
// them, the call number and 6 more zero fields
static const size_t	ORA_OCI7_SUMMARY_CALL_NUMBER=22;
static const size_t	ORA_OCI7_SUMMARY_SIZE=29;

// the native trailer: a 136 byte struct with the ttc code, the end of call
// status, the end to end field at 5, a 1 at 7 that nothing names, the call
// number at 49 and the "36 01" marker at 56
static const size_t	ORA_NATIVE_TRAILER_SIZE=136;
static const size_t	ORA_NATIVE_TRAILER_END_TO_END=5;
static const size_t	ORA_NATIVE_TRAILER_UNNAMED=7;
static const size_t	ORA_NATIVE_TRAILER_MARKER=56;

// the refusal's ora number, as a little endian uint32 behind the 9 byte
// native prefix sendErrorPacket() writes
static const size_t	ORA_NATIVE_ERROR_ORA_NUMBER=9;

// AUTH_SESSKEY and AUTH_VFR_DATA sizes in hex characters - twice the
// SESSION_KEY_SIZE_* and VFR_DATA_SIZE_* defines in src/protocols/oracle.cpp
static const size_t	SESSKEY_HEX_SIZE_9I=32;
static const size_t	SESSKEY_HEX_SIZE_10G=64;
static const size_t	SESSKEY_HEX_SIZE_11G=96;
static const size_t	SESSKEY_HEX_SIZE_12C=64;
static const size_t	VFR_DATA_HEX_SIZE_11G=20;
static const size_t	VFR_DATA_HEX_SIZE_12C=32;
static const size_t	PBKDF2_CSK_SALT_HEX_SIZE=32;

// which verifier the module picks for a case
enum verifier {
	VERIFIER_9I,
	VERIFIER_10G,
	VERIFIER_11G,
	VERIFIER_12C
};

static const char	*verifiernames[]={"9i","10g","11g","12c"};

struct listenerinfo {
	const char	*name;
	const char	*portvariable;
	uint16_t	defaultport;
	unsigned char	logontypes;
	unsigned char	fieldversion;
	size_t		capssize;
	const char	*versionno;
};

// the defaults are what configure substitutes, ORACLEPROTOCOLPORTBASE 1521
// plus the token's offset
static const listenerinfo	listeners[]={
	{"12.1","ORACLEPROTOCOLPORT1",1521,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"},
	{"11.2","ORACLEPROTOCOLPORT2",1522,
		ORA_SERVER_LOGON_TYPES,ORA_CCAP_FIELD_VERSION_11_2,
		ORA_CCAP_SIZE,"186646784"},
	{"10.2","ORACLEPROTOCOLPORT12",1532,
		ORA_SERVER_LOGON_TYPES,ORA_CCAP_FIELD_VERSION_10_2,
		ORA_CCAP_SIZE_10_2,NULL},
	{"12.1 allowedlogonversion=11","ORACLEPROTOCOLPORT13",1533,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 allowedlogonversion=12a","ORACLEPROTOCOLPORT14",1534,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"}
};

enum {
	LISTENER_12_1,
	LISTENER_11_2,
	LISTENER_10_2,
	LISTENER_FLOOR_11,
	LISTENER_FLOOR_12A
};

struct testcase {
	size_t		listener;
	unsigned char	logontypes;
	verifier	expected;
	bool		refused;
	bool		fulllogin;
};

static const testcase	testcases[]={

	// at 12.1, each client gets its own verifier
	{LISTENER_12_1,LOGON_TYPES_9I,VERIFIER_9I,false,false},
	{LISTENER_12_1,LOGON_TYPES_11G,VERIFIER_11G,false,true},
	{LISTENER_12_1,LOGON_TYPES_12C,VERIFIER_12C,false,false},

	// at 11.2, nothing newer than 11g
	{LISTENER_11_2,LOGON_TYPES_9I,VERIFIER_9I,false,false},
	{LISTENER_11_2,LOGON_TYPES_11G,VERIFIER_11G,false,true},
	{LISTENER_11_2,LOGON_TYPES_12C,VERIFIER_11G,false,false},

	// at 10.2, o3logon for everybody, 10g for an o5logon client
	{LISTENER_10_2,LOGON_TYPES_9I,VERIFIER_9I,false,false},
	{LISTENER_10_2,LOGON_TYPES_11G,VERIFIER_10G,false,false},
	{LISTENER_10_2,LOGON_TYPES_12C,VERIFIER_10G,false,false},

	// a floor of 11 refuses o3logon only
	{LISTENER_FLOOR_11,LOGON_TYPES_9I,VERIFIER_9I,true,false},
	{LISTENER_FLOOR_11,LOGON_TYPES_11G,VERIFIER_11G,false,true},
	{LISTENER_FLOOR_11,LOGON_TYPES_12C,VERIFIER_12C,false,false},

	// a floor of 12a refuses everything but 12c
	{LISTENER_FLOOR_12A,LOGON_TYPES_9I,VERIFIER_9I,true,false},
	{LISTENER_FLOOR_12A,LOGON_TYPES_11G,VERIFIER_11G,true,false},
	{LISTENER_FLOOR_12A,LOGON_TYPES_12C,VERIFIER_12C,false,false}
};

static const char	*host="127.0.0.1";
static const char	*sid="ora1";
static const char	*user="testuser";
static const char	*password="testpassword";

static bool	nativeencoding=false;

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static uint16_t getPort(const listenerinfo *li) {
	const char	*value=environment::getValue(li->portvariable);
	if (charstring::isNullOrEmpty(value)) {
		return li->defaultport;
	}
	return (uint16_t)charstring::convertToInteger(value);
}

static bool isHex(const char *value, size_t size) {
	if (charstring::getLength(value)!=size) {
		return false;
	}
	for (size_t i=0; i<size; i++) {
		char	c=value[i];
		if (!((c>='0' && c<='9') || (c>='a' && c<='f') ||
						(c>='A' && c<='F'))) {
			return false;
		}
	}
	return true;
}

// whether the field is there, and if "hexsize" isn't 0, whether it's that
// many hex characters.  "flags" is checked either way
static bool checkField(oracleprotocolclient *client, const char *name,
					size_t hexsize, const char *value,
					uint32_t expectedflags) {
	char		*actual=NULL;
	uint32_t	flags=0;
	if (!client->getAuthField(name,&actual,&flags)) {
		return false;
	}
	bool	ok=(flags==expectedflags);
	if (hexsize) {
		ok=(ok && isHex(actual,hexsize));
	}
	if (value) {
		ok=(ok && !charstring::compare(actual,value));
	}
	if (!ok) {
		stdoutput.printf("    %s: %s (flags 0x%04x)\n",
					name,(actual)?actual:"(null)",flags);
	}
	delete[] actual;
	return ok;
}

static bool hasField(oracleprotocolclient *client, const char *name) {
	return client->getAuthField(name,NULL,NULL);
}

static void dumpTrailer(const unsigned char *trailer, size_t size) {
	stdoutput.printf("    trailer (%d bytes):",(int)size);
	for (size_t i=0; i<size; i++) {
		stdoutput.printf(" %02x",trailer[i]);
	}
	stdoutput.printf("\n");
}

// the trailer behind the pairs, byte for byte, and its call number on its
// own, so a failure says which
static void checkTrailer(oracleprotocolclient *client, const char *label,
				unsigned char phase, unsigned char seqnumber,
				bool oci7summary) {

	const unsigned char	*trailer=NULL;
	size_t			size=0;
	if (!client->getAuthTrailer(&trailer,&size)) {
		report(label,false);
		return;
	}

	unsigned char	expected[ORA_NATIVE_TRAILER_SIZE];
	bytestring::zero(expected,sizeof(expected));
	size_t		expectedsize=0;
	size_t		callnumberoffset=0;
	expected[0]=ORA_TTC_ERROR;
	if (nativeencoding) {
		expected[1]=1;
		expected[ORA_NATIVE_TRAILER_END_TO_END]=phase;
		expected[ORA_NATIVE_TRAILER_UNNAMED]=1;
		expected[ORA_NATIVE_TRAILER_MARKER]=0x36;
		expected[ORA_NATIVE_TRAILER_MARKER+1]=0x01;
		expectedsize=ORA_NATIVE_TRAILER_SIZE;
		callnumberoffset=ORA_NATIVE_TRAILER_CALL_NUMBER;
	} else if (oci7summary) {
		expected[1]=1;
		expected[2]=1;
		expectedsize=ORA_OCI7_SUMMARY_SIZE;
		callnumberoffset=ORA_OCI7_SUMMARY_CALL_NUMBER;
	} else {
		expected[1]=1;
		expected[2]=1;
		expected[3]=1;
		expected[4]=phase;
		expectedsize=ORA_TRAILER_SIZE;
		callnumberoffset=ORA_TRAILER_CALL_NUMBER;
	}
	expected[callnumberoffset]=seqnumber;

	stringbuffer	l;
	l.append(label)->append(" call number is the sequence number");
	bool	ok=(size>callnumberoffset &&
			trailer[callnumberoffset]==seqnumber);
	report(l.getString(),ok);

	l.clear();
	l.append(label)->append(" is exact");
	bool	exact=(size==expectedsize &&
			!bytestring::compare(trailer,expected,expectedsize));
	report(l.getString(),exact);

	if (!ok || !exact) {
		dumpTrailer(trailer,size);
	}
}

// the challenge a case got, against the verifier it should have got
static void checkChallenge(oracleprotocolclient *client,
				const char *label,
				verifier expected,
				unsigned char seqnumber) {

	stringbuffer	l;

	l.append(label)->append(" got a challenge");
	bool	ok=(client->getResponseTtcCode()==ORA_TTC_OK &&
						!client->gotBreak());
	report(l.getString(),ok);
	if (!ok) {
		return;
	}

	bool		o3logon=(expected==VERIFIER_9I ||
					expected==VERIFIER_10G);
	uint32_t	expectedcount=(o3logon)?1:
				((expected==VERIFIER_12C)?6:3);
	uint32_t	count=0;
	l.clear();
	l.append(label)->append(" pair count is ")->append(expectedcount);
	report(l.getString(),client->getAuthFieldCount(&count) &&
						count==expectedcount);

	size_t	sesskeysize=0;
	switch (expected) {
		case VERIFIER_9I:
			sesskeysize=SESSKEY_HEX_SIZE_9I;
			break;
		case VERIFIER_10G:
			sesskeysize=SESSKEY_HEX_SIZE_10G;
			break;
		case VERIFIER_11G:
			sesskeysize=SESSKEY_HEX_SIZE_11G;
			break;
		case VERIFIER_12C:
			sesskeysize=SESSKEY_HEX_SIZE_12C;
			break;
	}
	l.clear();
	l.append(label)->append(" AUTH_SESSKEY is ");
	l.append((uint64_t)sesskeysize)->append(" hex characters");
	report(l.getString(),
		checkField(client,"AUTH_SESSKEY",sesskeysize,NULL,0));

	if (o3logon) {

		// the tagged o3logon shape: AUTH_SESSKEY alone, and nothing
		// that carries a verifier type
		l.clear();
		l.append(label)->append(" has no AUTH_VFR_DATA");
		report(l.getString(),!hasField(client,"AUTH_VFR_DATA"));

	} else {

		bool	pbkdf2=(expected==VERIFIER_12C);

		l.clear();
		l.append(label)->append(" AUTH_VFR_DATA flags are ");
		l.append((pbkdf2)?"0x4815":"0x1b25");
		report(l.getString(),
			checkField(client,"AUTH_VFR_DATA",
				(pbkdf2)?VFR_DATA_HEX_SIZE_12C:
					VFR_DATA_HEX_SIZE_11G,
				NULL,
				(pbkdf2)?ORA_VERIFIER_TYPE_12C:
					ORA_VERIFIER_TYPE_11G));

		if (pbkdf2) {
			l.clear();
			l.append(label)->append(" has the pbkdf2 fields");
			report(l.getString(),
				checkField(client,"AUTH_PBKDF2_CSK_SALT",
					PBKDF2_CSK_SALT_HEX_SIZE,NULL,0) &&
				checkField(client,"AUTH_PBKDF2_VGEN_COUNT",
							0,"4096",0) &&
				checkField(client,"AUTH_PBKDF2_SDER_COUNT",
							0,"3",0));
		} else {
			l.clear();
			l.append(label)->append(" has no pbkdf2 fields");
			report(l.getString(),
				!hasField(client,"AUTH_PBKDF2_CSK_SALT") &&
				!hasField(client,"AUTH_PBKDF2_VGEN_COUNT") &&
				!hasField(client,"AUTH_PBKDF2_SDER_COUNT"));
		}

		l.clear();
		l.append(label)->append(" has AUTH_GLOBALLY_UNIQUE_DBID");
		report(l.getString(),
			hasField(client,"AUTH_GLOBALLY_UNIQUE_DBID"));
	}

	l.clear();
	l.append(label)->append(" trailer");
	checkTrailer(client,l.getString(),
			ORA_AUTH_PHASE_CHALLENGE,seqnumber,expected==VERIFIER_9I);
}

// the ORA-28040 a below-floor login gets in place of the challenge.  a 9i
// session gets the narrow oci7 error shape and any other the modern one,
// which has one more zero field ahead of the ora number - see
// sendErrorPacket() in src/protocols/oracle.cpp
static void checkRefusal(oracleprotocolclient *client,
				const char *label,
				verifier expected,
				unsigned char seqnumber) {

	stringbuffer	l;

	l.append(label)->append(" got a break and a reset first");
	report(l.getString(),client->gotBreak());

	l.clear();
	l.append(label)->append(" got ORA-28040");
	bool	ok=(client->getResponseTtcCode()==ORA_TTC_ERROR &&
			client->responseContains("ORA-28040: No matching "
						"authentication protocol"));
	report(l.getString(),ok);
	if (!ok) {
		return;
	}

	if (nativeencoding) {
		const unsigned char	*r=client->getResponse()+3+
						ORA_NATIVE_ERROR_ORA_NUMBER;
		uint32_t		oranum=((uint32_t)r[0])|
						((uint32_t)r[1]<<8)|
						((uint32_t)r[2]<<16)|
						((uint32_t)r[3]<<24);
		l.clear();
		l.append(label)->append(" ora number is 28040");
		report(l.getString(),
			client->getResponseSize()>3+
				ORA_NATIVE_ERROR_ORA_NUMBER+4 &&
			oranum==ORA_NO_MATCHING_AUTH_PROTOCOL);
		return;
	}

	// the data flags and the ttc code, then the end of call status, then
	// one or two zero fields
	unsigned char	skip[3];
	uint32_t	eocstatus=0;
	uint32_t	zero1=0;
	uint32_t	zero2=0;
	uint32_t	oranum=0;
	client->rewindResponse();
	ok=(client->readBytes(skip,sizeof(skip)) &&
		client->readLenPreInt(&eocstatus) && eocstatus==1 &&
		client->readLenPreInt(&zero1) && !zero1 &&
		(expected==VERIFIER_9I ||
			(client->readLenPreInt(&zero2) && !zero2)) &&
		client->readLenPreInt(&oranum) &&
		oranum==ORA_NO_MATCHING_AUTH_PROTOCOL);
	l.clear();
	l.append(label)->append(" error is in the ");
	l.append((expected==VERIFIER_9I)?"oci7":"modern")->append(" shape");
	report(l.getString(),ok);
	if (!ok) {
		return;
	}

	unsigned char	zeros[17];
	unsigned char	callnumber=0;
	ok=(client->readBytes(zeros,sizeof(zeros)) &&
		client->readByte(&callnumber) &&
		callnumber==seqnumber);
	l.clear();
	l.append(label)->append(" error call number is the sequence number");
	report(l.getString(),ok);
}

// the rest of an 11g login, and the trailer of its answer
static void checkLogin(oracleprotocolclient *client,
				const char *label,
				const listenerinfo *li,
				unsigned char seqnumber) {

	stringbuffer	l;

	l.append(label)->append(" logs in");
	bool	ok=client->answerChallenge(user,password);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	l.clear();
	l.append(label)->append(" AUTH_VERSION_NO is ")->append(li->versionno);
	report(l.getString(),
		checkField(client,"AUTH_VERSION_NO",0,li->versionno,0));

	l.clear();
	l.append(label)->append(" login answer trailer");
	checkTrailer(client,l.getString(),ORA_AUTH_PHASE_ANSWER,seqnumber,false);
}

static void runCase(size_t index) {

	const testcase		*tc=&testcases[index];
	const listenerinfo	*li=&listeners[tc->listener];
	bool			o3logon=(tc->expected==VERIFIER_9I ||
					tc->expected==VERIFIER_10G);

	// a sequence number per case, so an echo can't pass by accident
	unsigned char	seqnumber=(unsigned char)(0x10+2*index);

	char	label[256];
	charstring::printf(label,sizeof(label),"%s, logon types 0x%02x",
						li->name,tc->logontypes);
	stdoutput.printf("\n%s - expect %s%s\n",label,
				verifiernames[tc->expected],
				(tc->refused)?", refused":"");

	oracleprotocolclient	client;
	client.setNativeEncoding(nativeencoding);
	client.setLogonTypes(tc->logontypes);
	client.setAuthSequenceNumbers(seqnumber,seqnumber+1);

	stringbuffer	l;
	l.append(label)->append(" connect");
	if (!client.connect(host,getPort(li),sid)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	// what the listener advertised, which follows the ceiling and not
	// the client
	const unsigned char	*caps=client.getServerCompileCaps();
	size_t			capssize=client.getServerCompileCapsSize();
	l.clear();
	l.append(label)->append(" compile caps are ");
	l.append((uint64_t)li->capssize)->append(" bytes");
	report(l.getString(),capssize==li->capssize);
	if (capssize>ORA_CCAP_FIELD_VERSION) {
		char	adv[256];
		charstring::printf(adv,sizeof(adv),
				"%s advertises logon types 0x%02x",
				label,li->logontypes);
		report(adv,caps[ORA_CCAP_LOGON_TYPES]==li->logontypes);
		charstring::printf(adv,sizeof(adv),
				"%s advertises field version %d",
				label,li->fieldversion);
		report(adv,caps[ORA_CCAP_FIELD_VERSION]==li->fieldversion);
	}

	if (nativeencoding && o3logon) {
		stdoutput.printf("%s: skipped phase one, o3logon "
					"is portable only\n",label);
		client.disconnect();
		return;
	}

	l.clear();
	l.append(label)->append(" phase one");
	if (!client.requestChallenge(user)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	if (tc->refused) {
		checkRefusal(&client,label,tc->expected,seqnumber);
	} else {
		checkChallenge(&client,label,tc->expected,seqnumber);
		if (tc->fulllogin) {
			checkLogin(&client,label,li,seqnumber+1);
		}
	}

	client.disconnect();
}

int main(int argc, char **argv) {

	nativeencoding=(argc>1 && !charstring::compare(argv[1],"-native"));

	stdoutput.printf("\n====== #10515 verifier negotiation (%s encoding) "
				"======\n",(nativeencoding)?"native":"portable");

	for (size_t i=0; i<sizeof(testcases)/sizeof(testcases[0]); i++) {
		runCase(i);
	}

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
