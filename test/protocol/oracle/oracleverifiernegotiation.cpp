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
//	- the 9i verifier for a client whose CCAP_LOGON_TYPES byte lacks
//	  O5LOGON (0x08), always under o3logon
//	- the 10g verifier for a client that sets O5LOGON, for a 0x52
//	  classic login or at 10.2.  o3logon for a classic login, and
//	  o5logon for a tagged 0x76 one, as a real 10.2 server does it
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
// only, so a native run checks what the 0x05 cases advertise and stops there.
//
// Both runs go through every case twice, once with the client advertising
// field version 11.2 and once 12.1, since the native struct the trailer and
// the error object share is 8 bytes longer at 12.1 and up, and the portable
// trailer and error object get a 12.1 extension at 12.1 and up.  What a
// listener sends follows the lower of its field version and the client's.
// The ORA-28040 a below-floor login gets and the ORA-01017 a wrong password
// gets in phase two are compared byte for byte, against the objects a real
// 11.2 and 12.2 server send - see checkNativeError() for the native encoding
// and checkPortableError() for the portable one.  The portable run tries a
// wrong password with the 9i verifier too.
//
// Every pass runs at two tns versions: the one a client offering a newer one
// negotiates, 0x013b, and 0x013a, which a genuine 11.2 client offers and a
// real 11.2 server accepts.  A real server sets the tns flags byte of its
// break and reset markers to 0x20 when the version it accepted is 0x013b or
// higher, which is also where the packet header grows to a 32-bit length,
// and to 0 below it.  The markers a refused login or a wrong password gets
// are checked against the version the client read off the accept, not against
// anything the listener was configured with.  The ano response data packet
// follows the same rule.  Every case connects with the connect flags of a
// real client and runs the ano exchange behind the accept, and the response
// has to be a data packet that opens with the ano marker.
// checkAnoResponseFlags() checks its flags byte.  So do the data descriptor
// packets a zero copy lob read gets.  Wherever the portable run completes a
// login below field version 12.1, the client offers zero copy, selects a
// clob and reads it, and checkLobDataFlags() checks the flags byte of the
// first of them.

// the verifier types - the VERIFIER_TYPE_* defines in
// src/protocols/oracle.cpp.  only the ones that run under o5logon go out on
// the wire
static const uint32_t	ORA_VERIFIER_TYPE_10G=0x0939;
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

// PROTOCOL_VERSION_11 in src/protocols/oracle.cpp, the version a genuine 11.2
// client offers, and the flags byte of a marker packet from a large header
// session - PACKET_FLAG_LARGE_HEADER there
static const uint16_t	ORA_CONNECT_VERSION_11=0x013a;
static const unsigned char	ORA_MARKER_PACKET_FLAGS=0x20;

// the flags byte of the ano response data packet from a large header
// session, which sendAnoResponse() in src/protocols/oracle.cpp sends
static const unsigned char	ORA_ANO_PACKET_FLAGS=0x20;

// the flags byte of a lob data descriptor packet from a large header
// session, which sendLobDataChunk() in src/protocols/oracle.cpp sends
static const unsigned char	ORA_LOB_DATA_PACKET_FLAGS=0x20;

// the start of the 114 byte clob locator buildLobLocator() in
// src/protocols/oracle.cpp writes: its length, its version, the clob type
// and its kind
static const unsigned char	ORA_CLOB_LOCATOR_HEAD[]={
	0x00, 0x70, 0x00, 0x02, 0x02, 0x0c
};
static const size_t		ORA_LOB_LOCATOR_SIZE=114;

// the clob read back, and its length
static const char		*ORA_CLOB_QUERY=
				"select to_clob(rpad('C',400,'C')) from dual";
static const uint32_t		ORA_CLOB_LENGTH=400;

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
// more zero fields.  at field version 12.1 and up it gains the 12.1 summary
// extension, two more zero bytes here
static const size_t	ORA_TRAILER_CALL_NUMBER_SIZE=31;
static const size_t	ORA_TRAILER_EXTENSION_SIZE=2;

// the portable trailer of a 9i challenge, the oci7 summary: the ttc code,
// the end of call status, 19 zero fields with no end to end field among
// them, the call number and 6 more zero fields, and the same extension
static const size_t	ORA_OCI7_SUMMARY_CALL_NUMBER=22;
static const size_t	ORA_OCI7_SUMMARY_SIZE_BASE=29;

// the native trailer: a struct with the ttc code, the end of call status,
// the end to end field at 5, a 1 at 7 that nothing names, the call number at
// 49 and the "36 01" marker at 56.  136 bytes at field version 11.2, which
// the client negotiates by default, and 144 at 12.1 and up
static const size_t	ORA_NATIVE_TRAILER_SIZE=136;
static const size_t	ORA_NATIVE_TRAILER_SIZE_12_1=144;
static const size_t	ORA_NATIVE_TRAILER_END_TO_END=5;
static const size_t	ORA_NATIVE_TRAILER_UNNAMED=7;
static const size_t	ORA_NATIVE_TRAILER_MARKER=56;

// the same struct as an error object's, counted from the byte behind the ttc
// code: the end of call status at 0, the end to end field at 4, a 1 at 6,
// the ora number at 11, the statement type at 21, the call number at 48, the
// "36 01" marker at 55 and the ora number again at 131.  135 bytes at field
// version 11.2, 143 at 12.1 and up, then a one byte message length and the
// message
static const size_t	ORA_NATIVE_ERROR_END_OF_CALL=0;
static const size_t	ORA_NATIVE_ERROR_UNNAMED=6;
static const size_t	ORA_NATIVE_ERROR_ORA_NUMBER=11;
static const size_t	ORA_NATIVE_ERROR_STATEMENT_TYPE=21;
static const size_t	ORA_NATIVE_ERROR_CALL_NUMBER=48;
static const size_t	ORA_NATIVE_ERROR_MARKER=55;
static const size_t	ORA_NATIVE_ERROR_ORA_NUMBER_AGAIN=131;
static const size_t	ORA_NATIVE_ERROR_SIZE=135;
static const size_t	ORA_NATIVE_ERROR_SIZE_12_1=143;

// the statement type a 12.1 server's phase two error carries
static const unsigned char	ORA_NATIVE_ERROR_STATEMENT_TYPE_PHASE_TWO=2;

// the portable error object, counted from the byte behind the ttc code: the
// end of call status 1 1, the end to end field (modern shape only), the rows
// processed, the ora number, four zero fields, the statement type as a raw
// byte, 12 zero bytes, the call number as a raw byte, 6 zero fields, then at
// field version 12.1 and up the ora number again and a zero row count, and
// the message behind a one byte length
static const size_t	ORA_PORTABLE_ERROR_ZEROS_BEFORE_STATEMENT_TYPE=4;
static const size_t	ORA_PORTABLE_ERROR_ZEROS_BEFORE_CALL_NUMBER=12;
static const size_t	ORA_PORTABLE_ERROR_ZEROS_AFTER_CALL_NUMBER=6;

// ORA_INVALID_USERNAME_PASSWORD in src/protocols/oracle.cpp
static const uint32_t	ORA_INVALID_USERNAME_PASSWORD=1017;

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

	// at 10.2, nothing newer than 10g
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

// the field version the client advertises, and the one the current case's
// listener settles on, which is the lower of its own and the client's
static unsigned char	clientfieldversion=ORA_CCAP_FIELD_VERSION_11_2;
static unsigned char	negotiatedfieldversion=ORA_CCAP_FIELD_VERSION_11_2;

// the tns version the client offers, 0 for the default, which settles on
// ORA_PROTOCOL_VERSION_12, and the one the current case's listener settles
// on
static uint16_t	clientconnectversion=0;
static uint16_t	negotiatedconnectversion=ORA_PROTOCOL_VERSION_12;

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

static void writeLittleEndian(unsigned char *buffer, uint32_t value) {
	for (size_t i=0; i<4; i++) {
		buffer[i]=(unsigned char)(value>>(8*i));
	}
}

static uint32_t readLittleEndian(const unsigned char *buffer) {
	return ((uint32_t)buffer[0])|((uint32_t)buffer[1]<<8)|
		((uint32_t)buffer[2]<<16)|((uint32_t)buffer[3]<<24);
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
		ok=(ok && !charstring::compare((actual)?actual:"",value));
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

static void dumpBytes(const char *what, const unsigned char *bytes,
								size_t size) {
	stdoutput.printf("    %s (%d bytes):",what,(int)size);
	for (size_t i=0; i<size; i++) {
		stdoutput.printf(" %02x",bytes[i]);
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

	// the portable trailers end in the 12.1 extension at 12.1 and up
	size_t	extensionsize=(negotiatedfieldversion>=
					ORA_CCAP_FIELD_VERSION_12_1)?
					ORA_TRAILER_EXTENSION_SIZE:0;

	unsigned char	expected[ORA_NATIVE_TRAILER_SIZE_12_1];
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
		expectedsize=(negotiatedfieldversion>=
					ORA_CCAP_FIELD_VERSION_12_1)?
					ORA_NATIVE_TRAILER_SIZE_12_1:
					ORA_NATIVE_TRAILER_SIZE;
		callnumberoffset=ORA_NATIVE_TRAILER_CALL_NUMBER;
	} else if (oci7summary) {
		expected[1]=1;
		expected[2]=1;
		expectedsize=ORA_OCI7_SUMMARY_SIZE_BASE+extensionsize;
		callnumberoffset=ORA_OCI7_SUMMARY_CALL_NUMBER;
	} else {
		expected[1]=1;
		expected[2]=1;
		expected[3]=1;
		expected[4]=phase;
		expectedsize=ORA_TRAILER_CALL_NUMBER_SIZE+extensionsize;
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
		dumpBytes("trailer",trailer,size);
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

	bool		o3logon=(expected==VERIFIER_9I);
	bool		des10g=(expected==VERIFIER_10G);
	uint32_t	expectedcount=(o3logon)?1:((des10g)?2:
				((expected==VERIFIER_12C)?6:3));
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

	} else if (des10g) {

		// the shape a real 10.2 server sends a tagged o5logon login: an
		// empty AUTH_VFR_DATA carrying the verifier type, since the des
		// verifier has no salt, and no AUTH_GLOBALLY_UNIQUE_DBID
		l.clear();
		l.append(label)->append(" AUTH_VFR_DATA is empty, flags 0x0939");
		report(l.getString(),
			checkField(client,"AUTH_VFR_DATA",0,"",
					ORA_VERIFIER_TYPE_10G));

		l.clear();
		l.append(label)->append(" has no AUTH_GLOBALLY_UNIQUE_DBID");
		report(l.getString(),
			!hasField(client,"AUTH_GLOBALLY_UNIQUE_DBID"));

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

// a native error object, byte for byte.  it is the struct the native trailer
// is, with the ora number in it and the message behind it: the end of call
// status 1, the end to end field 0, a 1 at 6, the ora number at 11, the call
// number at 48, the "36 01" marker at 55 and, when the emulated server is
// 12.1, the ora number again at 131 and the statement type 2 at 21 for an
// error that answers phase two.  the struct is 8 bytes longer when the
// negotiated field version is 12.1 or up.  the pointer at 71 is live memory
// on a real server and zero here.
//
// the expected bytes are the objects a real server sends, with that pointer
// zeroed and the call number set to the one this case sent:
//
//	- ORA-28040, field version 11.2: packet [0015] of
//	  samples/10544-centos6x64-oci11-native-loginretry-unknownuser-
//	  realserver-oracle122-allowed12a.oraproxy, from a 12.2 server
//	- ORA-01017, 12.1 emulated at field version 11.2: packet [0017] of
//	  samples/10544-centos6x64-oci11-native-loginretry-unknownuser-
//	  realserver-oracle122.oraproxy
//	- ORA-01017, 11.2 emulated: packet [0017] of
//	  samples/10544-centos6x64-oci11-native-loginretry-unknownuser-
//	  realserver-oracle112.oraproxy, from an 11.2 server
//	- field version 12.1 and up: the same, 8 zero bytes longer, as in packet
//	  [0017] of samples/10039-dev-oci23-native-loginretry-realserver.oraproxy
//
// the 12.1 server is inferred from the 12.2 one
static void checkNativeError(oracleprotocolclient *client,
				const char *label,
				const listenerinfo *li,
				uint32_t oranum,
				const char *message,
				unsigned char callnumber,
				bool secondphase) {

	bool	longstruct=(negotiatedfieldversion>=
					ORA_CCAP_FIELD_VERSION_12_1);
	bool	server121=(li->fieldversion>=ORA_CCAP_FIELD_VERSION_12_1);
	size_t	structsize=(longstruct)?ORA_NATIVE_ERROR_SIZE_12_1:
						ORA_NATIVE_ERROR_SIZE;
	size_t	messagesize=charstring::getLength(message);

	unsigned char	expected[ORA_NATIVE_ERROR_SIZE_12_1+1+255];
	bytestring::zero(expected,sizeof(expected));
	expected[ORA_NATIVE_ERROR_END_OF_CALL]=1;
	expected[ORA_NATIVE_ERROR_UNNAMED]=1;
	writeLittleEndian(expected+ORA_NATIVE_ERROR_ORA_NUMBER,oranum);
	if (secondphase && server121) {
		expected[ORA_NATIVE_ERROR_STATEMENT_TYPE]=
				ORA_NATIVE_ERROR_STATEMENT_TYPE_PHASE_TWO;
	}
	expected[ORA_NATIVE_ERROR_CALL_NUMBER]=callnumber;
	expected[ORA_NATIVE_ERROR_MARKER]=0x36;
	expected[ORA_NATIVE_ERROR_MARKER+1]=0x01;
	if (server121) {
		writeLittleEndian(expected+ORA_NATIVE_ERROR_ORA_NUMBER_AGAIN,
								oranum);
	}
	expected[structsize]=(unsigned char)messagesize;
	bytestring::copy(expected+structsize+1,message,messagesize);
	size_t	expectedsize=structsize+1+messagesize;

	// behind the data flags and the ttc code
	const size_t		prefix=3;
	size_t			responsesize=client->getResponseSize();
	const unsigned char	*object=client->getResponse()+prefix;
	size_t			objectsize=(responsesize>prefix)?
						responsesize-prefix:0;

	stringbuffer	l;
	l.append(label)->append(" ora number is ")->append(oranum);
	bool	oranumok=(objectsize>ORA_NATIVE_ERROR_ORA_NUMBER+3 &&
			readLittleEndian(object+ORA_NATIVE_ERROR_ORA_NUMBER)==
								oranum);
	report(l.getString(),oranumok);

	l.clear();
	l.append(label)->append(" error call number is the sequence number");
	bool	callnumberok=(objectsize>ORA_NATIVE_ERROR_CALL_NUMBER &&
			object[ORA_NATIVE_ERROR_CALL_NUMBER]==callnumber);
	report(l.getString(),callnumberok);

	l.clear();
	l.append(label)->append(" native error is exact");
	bool	exact=(objectsize==expectedsize &&
			!bytestring::compare(object,expected,expectedsize));
	report(l.getString(),exact);

	if (!oranumok || !callnumberok || !exact) {
		dumpBytes("error",object,objectsize);
		dumpBytes("expected",expected,expectedsize);
	}
}

// the break and reset markers a refused login got, against the flags byte a
// real server sets: 0x20 when the version the client read off the accept is
// 0x013b or higher, 0 below it
static void checkMarkerFlags(oracleprotocolclient *client,
					const char *label) {

	unsigned char	expected=(client->getAcceptVersion()>=
					ORA_PROTOCOL_VERSION_12)?
					ORA_MARKER_PACKET_FLAGS:0;

	stringbuffer	l;
	l.append(label)->append(" accepted tns version is ");
	char	version[16];
	charstring::printf(version,sizeof(version),"0x%04x",
						negotiatedconnectversion);
	l.append(version);
	report(l.getString(),
		client->getAcceptVersion()==negotiatedconnectversion);

	char	flags[16];
	charstring::printf(flags,sizeof(flags),"0x%02x",expected);

	l.clear();
	l.append(label)->append(" break marker flags are ")->append(flags);
	bool	breakok=(client->getBreakFlags()==expected);
	report(l.getString(),breakok);

	l.clear();
	l.append(label)->append(" reset marker flags are ")->append(flags);
	bool	resetok=(client->getResetFlags()==expected);
	report(l.getString(),resetok);

	if (!breakok || !resetok) {
		stdoutput.printf("    accept version 0x%04x, break flags "
					"0x%02x, reset flags 0x%02x\n",
					client->getAcceptVersion(),
					client->getBreakFlags(),
					client->getResetFlags());
	}
}

// the ano response's flags byte against the one a real server sets: 0x20 when
// the version the client read off the accept is 0x013b or higher, 0 below it
static void checkAnoResponseFlags(oracleprotocolclient *client,
					const char *label) {

	unsigned char	expected=(client->getAcceptVersion()>=
					ORA_PROTOCOL_VERSION_12)?
					ORA_ANO_PACKET_FLAGS:0;

	char	flags[16];
	charstring::printf(flags,sizeof(flags),"0x%02x",expected);

	stringbuffer	l;
	l.append(label)->append(" ano response flags are ")->append(flags);
	bool	ok=(client->getAnoResponseFlags()==expected);
	report(l.getString(),ok);

	if (!ok) {
		stdoutput.printf("    accept version 0x%04x, ano response "
					"flags 0x%02x\n",
					client->getAcceptVersion(),
					client->getAnoResponseFlags());
	}
}

// a zero copy read of a clob, and the flags byte of its first lob data
// descriptor packet against the one a real server sets: 0x20 when the
// version the client read off the accept is 0x013b or higher, 0 below it.
// the client has to have offered zero copy and logged in
static void checkLobDataFlags(oracleprotocolclient *client,
					const char *label) {

	stringbuffer	l;

	// select the clob
	uint32_t	cursorid=0;
	l.append(label)->append(" selects a clob");
	bool	ok=(client->open(&cursorid) &&
			client->query3(ORA_OPTION_PARSE|
					ORA_OPTION_EXECUTE|
					ORA_OPTION_NOPLSQL,
					cursorid,0,ORA_CLOB_QUERY) &&
			client->getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO &&
			client->fetch(cursorid,1));
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// find its locator in the row
	const unsigned char	*response=client->getResponse();
	size_t			responsesize=client->getResponseSize();
	const unsigned char	*locator=NULL;
	for (size_t i=0; i+ORA_LOB_LOCATOR_SIZE<=responsesize; i++) {
		if (!bytestring::compare(response+i,ORA_CLOB_LOCATOR_HEAD,
					sizeof(ORA_CLOB_LOCATOR_HEAD))) {
			locator=response+i;
			break;
		}
	}
	l.clear();
	l.append(label)->append(" the row carries a locator");
	report(l.getString(),locator!=NULL);
	if (!locator) {
		return;
	}
	unsigned char	copy[ORA_LOB_LOCATOR_SIZE];
	bytestring::copy(copy,locator,sizeof(copy));

	// read it
	l.clear();
	l.append(label)->append(" reads the clob in lob data packets");
	ok=(client->lobRead(copy,sizeof(copy),1,ORA_CLOB_LENGTH) &&
		client->getResponseTtcCode()==ORA_TTC_OK &&
		client->getLobDataPacketCount()>0);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	unsigned char	expected=(client->getAcceptVersion()>=
					ORA_PROTOCOL_VERSION_12)?
					ORA_LOB_DATA_PACKET_FLAGS:0;

	char	flags[16];
	charstring::printf(flags,sizeof(flags),"0x%02x",expected);

	l.clear();
	l.append(label)->append(" lob data packet flags are ")->append(flags);
	ok=(client->getLobDataPacketFlags()==expected);
	report(l.getString(),ok);

	if (!ok) {
		stdoutput.printf("    accept version 0x%04x, lob data packet "
					"flags 0x%02x\n",
					client->getAcceptVersion(),
					client->getLobDataPacketFlags());
	}
}

static void putByte(unsigned char *buffer, size_t *size,
						unsigned char value) {
	buffer[(*size)++]=value;
}

static void putZeros(unsigned char *buffer, size_t *size, size_t count) {
	bytestring::zero(buffer+*size,count);
	*size+=count;
}

// a count prefixed int: 0 is one zero byte, anything else is the count of
// its bytes and then those bytes, most significant first
static void putLenPreInt(unsigned char *buffer, size_t *size, uint32_t value) {
	size_t	count=0;
	while (count<4 && (value>>(8*count))) {
		count++;
	}
	putByte(buffer,size,(unsigned char)count);
	for (size_t i=count; i>0; i--) {
		putByte(buffer,size,(unsigned char)(value>>(8*(i-1))));
	}
}

// a portable error object, byte for byte: the count prefixed form sent to
// every client that doesn't ask for the native struct.  the 9i client's oci7
// shape ("o3logonshape") has no end to end field, which puts every byte
// behind it one lower than the modern shape's.  the statement type is a raw
// byte, 2 in a phase two error from an emulated 12.1 server and 0 in every
// other, whatever field version was negotiated.  the 12.1 extension, the
// ora number again and a row count, is there at field version 12.1 and up
// only.  the call number is the sequence number of the request the error
// answers.
//
// the expected bytes are the objects a real server sends, with the call
// number set to the one this case sent:
//
//	- modern, ORA-01017, 12.1 emulated, field version 12.1 and up: packet
//	  [0017] of samples/10717-dev-oci23-portable-loginretry-unknownuser-
//	  bannerrewrite-both-universalreps-realserver-oracle122.oraproxy, from
//	  a 12.2 server
//	- modern, ORA-01017, 12.1 emulated, field version 11.2: packet [0017]
//	  of samples/10717-fedora16x86-oci11-portable-loginretry-unknownuser-
//	  bannerrewrite-both-universalreps-realserver-oracle122.oraproxy
//	- modern, ORA-01017, 11.2 emulated: packet [0017] of
//	  samples/10717-dev-oci23-portable-loginretry-unknownuser-bannerrewrite-
//	  both-universalreps-realserver-oracle112.oraproxy, from an 11.2 server,
//	  with statement type 0
//	- modern, ORA-28040: packet [0015] of samples/10717-dev-oci23-portable-
//	  loginretry-unknownuser-ccapcf-bannerrewrite-both-universalreps-
//	  realserver-oracle122-allowed12a.oraproxy at field version 12.1 and up
//	  and of samples/10717-fedora16x86-oci11-portable-loginretry-
//	  unknownuser-bannerrewrite-both-universalreps-realserver-oracle122-
//	  allowed12a.oraproxy at 11.2
//	- o3logon, ORA-01017, 12.1 emulated: packet [0017] of samples/10717-
//	  solaris8sparc-oci7-portable-login-unknownuser-realserver-oracle122.
//	  oraproxy, from a 12.2 server and a 9.0.1 client.  with statement
//	  type 0: packet [0017] of samples/oracle102-oci7-portable-login-
//	  wrongpassword.cap, from a 10.2 server
//	- o3logon, ORA-28040: packet [0015] of samples/10717-solaris8sparc-oci7-
//	  portable-login-unknownuser-realserver-oracle122-allowed12a.oraproxy
//
// no real server sends the extension in the o3logon shape, since no real
// 9i client negotiates field version 12.1, and no 11.2 server was captured
// with a 9i client, so those bytes are inferred from the modern shape and
// from the 10.2 server
static void checkPortableError(oracleprotocolclient *client,
				const char *label,
				const listenerinfo *li,
				bool o3logonshape,
				uint32_t oranum,
				const char *message,
				unsigned char callnumber,
				bool secondphase) {

	bool	extension=(negotiatedfieldversion>=
					ORA_CCAP_FIELD_VERSION_12_1);
	bool	server121=(li->fieldversion>=ORA_CCAP_FIELD_VERSION_12_1);
	size_t	messagesize=charstring::getLength(message);

	unsigned char	expected[64+255];
	size_t		expectedsize=0;
	bytestring::zero(expected,sizeof(expected));

	// end of call status, end to end field, rows processed
	putByte(expected,&expectedsize,1);
	putByte(expected,&expectedsize,1);
	if (!o3logonshape) {
		putZeros(expected,&expectedsize,1);
	}
	putZeros(expected,&expectedsize,1);

	// ora number, four zero fields and the statement type
	size_t	oranumoffset=expectedsize;
	putLenPreInt(expected,&expectedsize,oranum);
	size_t	oranumsize=expectedsize-oranumoffset;
	putZeros(expected,&expectedsize,
			ORA_PORTABLE_ERROR_ZEROS_BEFORE_STATEMENT_TYPE);
	size_t	statementtypeoffset=expectedsize;
	unsigned char	statementtype=
			(secondphase && server121)?
			ORA_NATIVE_ERROR_STATEMENT_TYPE_PHASE_TWO:0;
	putByte(expected,&expectedsize,statementtype);

	// call number, then the tail
	putZeros(expected,&expectedsize,
			ORA_PORTABLE_ERROR_ZEROS_BEFORE_CALL_NUMBER);
	size_t	callnumberoffset=expectedsize;
	putByte(expected,&expectedsize,callnumber);
	putZeros(expected,&expectedsize,
			ORA_PORTABLE_ERROR_ZEROS_AFTER_CALL_NUMBER);

	// 12.1 extension
	if (extension) {
		putLenPreInt(expected,&expectedsize,oranum);
		putZeros(expected,&expectedsize,1);
	}

	// message
	putByte(expected,&expectedsize,(unsigned char)messagesize);
	bytestring::copy(expected+expectedsize,message,messagesize);
	expectedsize+=messagesize;

	// behind the data flags and the ttc code
	const size_t		prefix=3;
	size_t			responsesize=client->getResponseSize();
	const unsigned char	*object=client->getResponse()+prefix;
	size_t			objectsize=(responsesize>prefix)?
						responsesize-prefix:0;

	stringbuffer	l;
	l.append(label)->append(" ora number is ")->append(oranum);
	bool	oranumok=(objectsize>oranumoffset+oranumsize &&
			!bytestring::compare(object+oranumoffset,
						expected+oranumoffset,
						oranumsize));
	report(l.getString(),oranumok);

	l.clear();
	l.append(label)->append(" statement type is ");
	l.append((uint64_t)statementtype);
	bool	statementtypeok=(objectsize>statementtypeoffset &&
				object[statementtypeoffset]==statementtype);
	report(l.getString(),statementtypeok);

	l.clear();
	l.append(label)->append(" error call number is the sequence number");
	bool	callnumberok=(objectsize>callnumberoffset &&
				object[callnumberoffset]==callnumber);
	report(l.getString(),callnumberok);

	l.clear();
	l.append(label)->append(" portable error is exact");
	bool	exact=(objectsize==expectedsize &&
			!bytestring::compare(object,expected,expectedsize));
	report(l.getString(),exact);

	if (!oranumok || !statementtypeok || !callnumberok || !exact) {
		dumpBytes("error",object,objectsize);
		dumpBytes("expected",expected,expectedsize);
	}
}

// the ORA-28040 a below-floor login gets in place of the challenge.  a 9i
// session gets the narrow oci7 error shape and any other the modern one,
// which has one more zero field ahead of the ora number - see
// sendErrorPacket() in src/protocols/oracle.cpp.  in the native encoding
// every session gets the native struct
static void checkRefusal(oracleprotocolclient *client,
				const char *label,
				const listenerinfo *li,
				verifier expected,
				unsigned char seqnumber) {

	stringbuffer	l;

	l.append(label)->append(" got a break and a reset first");
	report(l.getString(),client->gotBreak());
	if (client->gotBreak()) {
		checkMarkerFlags(client,label);
	}

	l.clear();
	l.append(label)->append(" got ORA-28040");
	bool	ok=(client->getResponseTtcCode()==ORA_TTC_ERROR &&
			client->responseContains("ORA-28040: No matching "
						"authentication protocol"));
	report(l.getString(),ok);
	if (!ok) {
		return;
	}

	const char	*message="ORA-28040: No matching authentication "
					"protocol\n";
	if (nativeencoding) {
		checkNativeError(client,label,li,
				ORA_NO_MATCHING_AUTH_PROTOCOL,
				message,seqnumber,false);
	} else {
		checkPortableError(client,label,li,(expected==VERIFIER_9I),
				ORA_NO_MATCHING_AUTH_PROTOCOL,
				message,seqnumber,false);
	}
}

// the rest of an 11g login, and the trailer of its answer.  returns whether
// the login went through
static bool checkLogin(oracleprotocolclient *client,
				const char *label,
				const listenerinfo *li,
				unsigned char seqnumber) {

	stringbuffer	l;

	l.append(label)->append(" logs in");
	bool	ok=client->answerChallenge(user,password);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return false;
	}

	l.clear();
	l.append(label)->append(" AUTH_VERSION_NO is ")->append(li->versionno);
	report(l.getString(),
		checkField(client,"AUTH_VERSION_NO",0,li->versionno,0));

	l.clear();
	l.append(label)->append(" login answer trailer");
	checkTrailer(client,l.getString(),ORA_AUTH_PHASE_ANSWER,seqnumber,false);
	return true;
}

// the listeners a wrong password is tried against: 12.1, where a phase two
// error has the statement type and the second ora number, and 11.2, where
// it has neither
static const size_t	wrongpasswordlisteners[]={LISTENER_12_1,LISTENER_11_2};

// phase two with a password that isn't the user's: a break and a reset, then
// ORA-01017 in the native struct or the portable object, with the phase two
// sequence number as its call number.  "o3logon" tries it with the 9i
// verifier, portable encoding only
static void runWrongPassword(size_t index, bool o3logon) {

	const listenerinfo	*li=&listeners[wrongpasswordlisteners[index]];
	unsigned char		logontypes=(o3logon)?LOGON_TYPES_9I:
							LOGON_TYPES_11G;

	// a sequence number per case, apart from runCase()'s
	unsigned char	seqnumber=(unsigned char)((o3logon)?0x60+2*index:
							0x50+2*index);

	char	label[256];
	charstring::printf(label,sizeof(label),
			"%s, logon types 0x%02x, field version %d, "
			"tns version 0x%04x, wrong password",li->name,
			logontypes,clientfieldversion,
			negotiatedconnectversion);
	stdoutput.printf("\n%s - expect ORA-01017\n",label);

	negotiatedfieldversion=(li->fieldversion<clientfieldversion)?
					li->fieldversion:clientfieldversion;

	oracleprotocolclient	client;
	client.setNativeEncoding(nativeencoding);
	client.setFieldVersion(clientfieldversion);
	client.setConnectVersion(clientconnectversion);
	client.setLogonTypes(logontypes);
	client.setAuthSequenceNumbers(seqnumber,seqnumber+1);

	stringbuffer	l;
	l.append(label)->append(" connect");
	if (!client.connect(host,getPort(li),sid)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	l.clear();
	l.append(label)->append(" phase one");
	if (!client.requestChallenge(user) ||
			client.getResponseTtcCode()!=ORA_TTC_OK) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	// refused, so the answer fails and leaves the error in the response
	l.clear();
	l.append(label)->append(" phase two is refused");
	bool	refused=!((o3logon)?
			client.answerO3LogonChallenge(user,"notthepassword"):
			client.answerChallenge(user,"notthepassword"));
	report(l.getString(),refused);
	if (!refused) {
		client.disconnect();
		return;
	}

	l.clear();
	l.append(label)->append(" got a break and a reset first");
	report(l.getString(),client.gotBreak());
	if (client.gotBreak()) {
		checkMarkerFlags(&client,label);
	}

	l.clear();
	l.append(label)->append(" got ORA-01017");
	bool	ok=(client.getResponseTtcCode()==ORA_TTC_ERROR &&
			client.responseContains("ORA-01017: invalid "
						"username/password"));
	report(l.getString(),ok);
	if (ok) {
		const char	*message="ORA-01017: invalid "
					"username/password; logon denied\n";
		if (nativeencoding) {
			checkNativeError(&client,label,li,
					ORA_INVALID_USERNAME_PASSWORD,
					message,seqnumber+1,true);
		} else {
			checkPortableError(&client,label,li,o3logon,
					ORA_INVALID_USERNAME_PASSWORD,
					message,seqnumber+1,true);
		}
	}

	client.disconnect();
}

static void runCase(size_t index) {

	const testcase		*tc=&testcases[index];
	const listenerinfo	*li=&listeners[tc->listener];
	bool			o3logon=(tc->expected==VERIFIER_9I);

	// a sequence number per case, so an echo can't pass by accident
	unsigned char	seqnumber=(unsigned char)(0x10+2*index);

	char	label[256];
	charstring::printf(label,sizeof(label),
				"%s, logon types 0x%02x, field version %d, "
				"tns version 0x%04x",
				li->name,tc->logontypes,clientfieldversion,
				negotiatedconnectversion);
	stdoutput.printf("\n%s - expect %s%s\n",label,
				verifiernames[tc->expected],
				(tc->refused)?", refused":"");

	negotiatedfieldversion=(li->fieldversion<clientfieldversion)?
					li->fieldversion:clientfieldversion;

	oracleprotocolclient	client;
	client.setNativeEncoding(nativeencoding);
	client.setFieldVersion(clientfieldversion);
	client.setConnectVersion(clientconnectversion);
	client.setAnoNegotiation(true);
	client.setLogonTypes(tc->logontypes);
	client.setAuthSequenceNumbers(seqnumber,seqnumber+1);

	// lobRead() sends the portable request only, and query3() the one of
	// field version 11.2, which lacks the fields getQuery3Request() reads
	// at 12.1 and up
	bool	lobread=(tc->fulllogin && !nativeencoding &&
			negotiatedfieldversion<ORA_CCAP_FIELD_VERSION_12_1);
	client.setLobZeroCopy(lobread);

	stringbuffer	l;
	l.append(label)->append(" connect");
	if (!client.connect(host,getPort(li),sid)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	checkAnoResponseFlags(&client,label);

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
		checkRefusal(&client,label,li,tc->expected,seqnumber);
	} else {
		checkChallenge(&client,label,tc->expected,seqnumber);
		if (tc->fulllogin &&
				checkLogin(&client,label,li,seqnumber+1) &&
				lobread) {
			checkLobDataFlags(&client,label);
		}
	}

	client.disconnect();
}

int main(int argc, char **argv) {

	nativeencoding=(argc>1 && !charstring::compare(argv[1],"-native"));

	stdoutput.printf("\n====== #10515 verifier negotiation (%s encoding) "
				"======\n",(nativeencoding)?"native":"portable");

	// the native struct changes size at 12.1 and the portable trailer and
	// error object gain an extension, so both runs go through everything
	// at 11.2 and again at 12.1.  the markers' flags byte follows the tns
	// version, so all of that runs at the default offer, which settles on
	// 0x013b, and again offering 0x013a
	unsigned char	fieldversions[]={ORA_CCAP_FIELD_VERSION_11_2,
						ORA_CCAP_FIELD_VERSION_12_1};
	size_t		passes=sizeof(fieldversions)/sizeof(fieldversions[0]);
	uint16_t	connectversions[]={0,ORA_CONNECT_VERSION_11};
	size_t		versions=sizeof(connectversions)/
					sizeof(connectversions[0]);
	for (size_t v=0; v<versions; v++) {

		clientconnectversion=connectversions[v];
		negotiatedconnectversion=(clientconnectversion)?
					clientconnectversion:
					ORA_PROTOCOL_VERSION_12;
		stdoutput.printf("\n====== tns version 0x%04x ======\n",
						negotiatedconnectversion);

		for (size_t p=0; p<passes; p++) {

			clientfieldversion=fieldversions[p];
			stdoutput.printf("\n------ client field version %d "
						"------\n",clientfieldversion);

			for (size_t i=0;
				i<sizeof(testcases)/sizeof(testcases[0]);
				i++) {
				runCase(i);
			}

			for (size_t i=0; i<sizeof(wrongpasswordlisteners)/
				sizeof(wrongpasswordlisteners[0]); i++) {
				runWrongPassword(i,false);
				if (!nativeencoding) {
					runWrongPassword(i,true);
				}
			}
		}
	}

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
