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
// which refuses a login ahead of the challenge, with ORA-28040, when the
// client's logon types are ones the floor doesn't allow.  A real 12.2 server
// does the same.  It decides on the client's CCAP_LOGON_TYPES bits alone, in
// phase one, whatever login form the client uses, and it never lets the floor
// change which verifier it picks:
//
//	- floor 9 refuses one that lacks all of the legacy bits (0x01 and 0x04)
//	  and O5LOGON (0x08), which includes one that advertises none
//	- floors 10 and 11 refuse one that lacks O5LOGON (0x08)
//	- floor 12 refuses one that lacks O5LOGON_NP (0x02)
//	- floor 12a refuses one that lacks O7LOGON (0x20)
//
// A classic 0x52 login that gets past the bits at 12 or 12a is a case of its
// own.  The real server sends the challenge, the client answers it, and the
// answer gets ORA-01017 whatever the password is.  That was measured for the
// genuine 11.2 client at 12 and for OCI 23 at 12 and 12a.
//
// No client on hand can cover this on its own.  Each real client sends the
// one logon types byte it was built with, and the genuine 9i clients that
// send 0x05 only run on the buildfarm.  So this sends phase one of the login
// by hand, advertising each of these bytes in turn, to listeners at each
// ceiling and to four floor listeners:
//
//	- 0x00, genuine ojdbc14 10.2.0.5, which advertises no logon types
//	- 0x05, a genuine 9i client's legacy bits alone
//	- 0x0d, a genuine 10.2 OCI client's: O5LOGON without O5LOGON_NP
//	- 0x0f, a genuine 11.2 OCI client's
//	- 0xef, OCI 23's
//
// and checks the challenge each one gets: its shape, the verifier type in
// AUTH_VFR_DATA's flags, the session key's length, the pbkdf2 fields, and
// the call number in its trailer, which has to echo the request's sequence
// number.  It also checks the logon types and field version each listener
// advertises, and completes the login wherever the 11g verifier comes back,
// since that is the one exchange oracleprotocolclient can finish.
//
// A 9i challenge's trailer is the oci7 summary.  Whether it, and every other
// object of an o3logon session, carries the end to end field follows bit 0x10
// of the client's CCAP_OCI1 byte (compile caps byte 16), not its logon types
// or its field version.  Real 10.2 and 12.2 servers send the field exactly
// when the bit is set: to the genuine 10.2 client with its logon types
// rewritten to 0x05 (samples/10735-fedora9x86-oci10-oci8-ccap05-fv4-
// realserver-oracle102 and -oracle122.oraproxy) and not once the bit is
// cleared too (10735-fedora9x86-oci10-oci8-ccap05-cap16xef-realserver-
// oracle102 and -oracle122.oraproxy).  So each case advertises its genuine
// client's CCAP_OCI1 byte along with its logon types: 0x90 for ojdbc14, 0x0f
// for a 9i client, which leaves the bit clear, and 0xff for the rest.
//
// The genuine 10.2 client's own data type request is also sent with its
// logon types byte rewritten 0x0d -> 0x05, the byte those captures rewrite,
// with its CCAP_OCI1 byte as sent (0xff) and rewritten to 0xef.  With the
// bit set, the 9i challenge's trailer, the ORA-28040 refusal and a wrong
// password's ORA-01017 have to carry the field.  Sent without it, that
// client hangs on the challenge and reports ORA-03120 for the refusal -
// samples/10545-fedora9x86-oci10-oci8-sdu8192-ccap05-correctpassword-
// sqlrelay121 and -sqlrelay121-allowed10.oraproxy.  With the bit cleared
// they have to leave it out, as a real server does, and the client hangs on
// a real server's challenge too.
//
// The listeners are all on the oracleprotocol instance - see
// test/sqlrelay.conf.d/oracleprotocol.conf.in - except the 11.2 one, which is
// oracleprotocol11g's.
//
// -native runs the same cases in the native encoding, which is where the
// trailer is a marshalled struct with the call number at offset 49.  The
// module reads and answers a 0x76 o3logon login and a classic one in the
// portable encoding only, so a native run checks what those cases advertise
// and stops there.
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
//
// The 0x0d client is also played as the genuine 10.2.0.5 OCI client sends
// itself, in sessions of their own, since its logon types aren't all that
// sets it apart.  It offers tns version 0x0139, advertises field version 4,
// and sends a 2279 byte data type request behind its ttc code.  Under its
// default sdu of 2048 that goes out as 2011 + 289 bytes, split mid list and
// short of the sdu, and the listener has to reassemble it.  Under an sdu of
// 8192 it goes out in one packet, which is how the floors are told apart
// from that.  Its classic login is the 0x52 call, and the 0x51 call that
// answers the challenge, as that client sent them to the listener.  Its
// tagged login is the one the other cases send.
//
// The expected results are what a real 12.2 server was measured to do with
// these clients: which logins each floor admits and refuses, the 64 hex
// character key of the 10g verifier in a classic challenge, and the
// ORA-01017 a classic login gets after its challenge at a floor of 12 or
// 12a.  Where a case doesn't pin something, it is left unchecked rather than
// guessed at:
//
//	- the classic login of the 0x00 and 0x05 clients, and the ORA-28040
//	  object of the 0x00 client, which no real server was measured sending
//	- the verifier of the 0x00 client, which a real server gives a 16 hex
//	  character DES exchange where the module sends the 9i one.  ojdbc14
//	  does not log in to the module - measured with the real driver, see the
//	  README - and nothing here tries it
//	- the challenge of the 0x0f classic login, whose key a real server makes
//	  96 hex characters and the module 64
//	- the ORA-28040 and ORA-01017 objects of the classic login and of the
//	  genuine 10.2 client, which a real server sends in a fixed width form
//	  and the module in the count prefixed one that its clients read
//	- which of the legacy bits, 0x01 or 0x04, floor 9 looks at.  The bytes
//	  here have both or neither
//	- the trailer of the 0x00 client's challenge, whose bit is set
//
// The verifier the 0x0d client gets at 12.1 is the module's choice too: a
// real 12.2 server gives it the 10g one where the module gives it 11g, which
// the client completes.
//
// So are some of what the rewritten 10.2 client gets.  A real server sends
// it fixed width objects, so the count prefixed ones checked here are
// inferred: each field is measured, in the modern objects and in the real
// server's, and their combination is not.  The end to end field of its 9i
// challenge is the call number, the module's choice: a real server sent 2,
// which was also the call number in every capture.  Nothing here completes
// its login, which against a real server fails in the client with ORA-28041
// when tagged and gets ORA-01017 when classic.

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

// the logon types bytes the cases advertise, each one a genuine client's
static const unsigned char	LOGON_TYPES_NONE=0x00;
static const unsigned char	LOGON_TYPES_9I=0x05;
static const unsigned char	LOGON_TYPES_10G=0x0d;
static const unsigned char	LOGON_TYPES_11G=0x0f;
static const unsigned char	LOGON_TYPES_12C=0xef;

// the CCAP_OCI1 bytes the genuine clients send: ojdbc14's, a 9i client's,
// whose fast session propagate bit is clear, and the 10.2, 11.2 and OCI 23
// clients'.  and the 10.2 client's with that bit cleared
static const unsigned char	OCI1_OJDBC14=0x90;
static const unsigned char	OCI1_9I=0x0f;
static const unsigned char	OCI1_10G=0xff;
static const unsigned char	OCI1_10G_NO_FAST_SESSION_PROPAGATE=0xef;

// PROTOCOL_VERSION_10 and PROTOCOL_VERSION_11 in src/protocols/oracle.cpp, the
// versions a genuine 10.2 and 11.2 client offer, and the flags byte of a
// marker packet from a large header session - PACKET_FLAG_LARGE_HEADER there
static const uint16_t	ORA_CONNECT_VERSION_10=0x0139;
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
// challenge and 3 in the login answer.  putOci7Summary() writes the call
// number there instead
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

// the portable trailer of a 9i challenge, the oci7 summary, for a client
// without fast session propagate: the ttc code, the end of call status, 19
// zero fields with no end to end field among them, the call number and 6
// more zero fields, and the same extension.  for a client with it, the oci7
// summary has the end to end field and the portable trailer's layout
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

// the genuine 10.2.0.5 OCI client's calls, as it sent them
//
// its data type request, behind the ttc code: the character sets, the
// encoding flags, the 36 byte compile capabilities - logon types 0x0d at 4,
// field version 4 at 7 - the runtime capabilities and the ub2 data type
// list.  It is 2279 bytes, sent as 2011 + 289 bytes under an sdu of 2048, as
// packets [0009] and [0010] of samples/10545-fedora9x86-oci10-oci8-
// correctpassword-realserver-oracle122.oraproxy, from the client to a real
// 12.2 server.  The same bytes went to the module
static const size_t		ORA_10G_DATA_TYPE_REQUEST_SPLIT=2011;
static const uint16_t		ORA_10G_SDU=2048;

// the sdu a connect descriptor with SDU=8192 settles on, under which the
// client sends the same bytes in one packet - packet [0009] of samples/10545-
// fedora9x86-oci10-oci8-sdu8192-correctpassword-sqlrelay121.oraproxy
static const uint16_t		ORA_10G_SDU_BIG=8192;

static const unsigned char	ORA_10G_DATA_TYPE_REQUEST[]={
	0x69, 0x03, 0x69, 0x03, 0x02, 0x24, 0x06, 0x01, 0x01, 0x01, 0x0d, 0x01,
	0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0xff, 0x03,
	0x08, 0x03, 0x03, 0x01, 0x00, 0x3f, 0x01, 0x47, 0x3f, 0x01, 0x02, 0x01,
	0x01, 0x03, 0x01, 0x00, 0x00, 0x01, 0x05, 0x02, 0x01, 0x00, 0x00, 0x18,
	0x80, 0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x80, 0x00, 0x00, 0x00, 0xd0,
	0x07, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x17, 0x00,
	0x17, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x18, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x19, 0x00, 0x19, 0x00, 0x18, 0x00, 0x19, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x1a, 0x00, 0x1a, 0x00, 0x19, 0x00, 0x1a, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x1b, 0x00, 0x1b, 0x00, 0x0a, 0x00, 0x1b, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x16, 0x00, 0x1c, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x1d, 0x00, 0x1d, 0x00, 0x17, 0x00, 0x1d, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x17, 0x00, 0x1e, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x19, 0x00, 0x1f, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x0a, 0x00, 0x20, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x21, 0x00, 0x21, 0x00, 0x0a, 0x00, 0x21, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x0a, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00,
	0x0b, 0x00, 0x01, 0x00, 0x00, 0x00, 0x28, 0x00, 0x28, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x29, 0x00, 0x29, 0x00, 0x01, 0x00, 0x00, 0x00, 0x75, 0x00,
	0x75, 0x00, 0x01, 0x00, 0x00, 0x00, 0x78, 0x00, 0x78, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x22, 0x01, 0x22, 0x00, 0x01, 0x00, 0x00, 0x01, 0x23, 0x01,
	0x23, 0x00, 0x01, 0x01, 0x23, 0x00, 0x01, 0x00, 0x00, 0x01, 0x24, 0x01,
	0x24, 0x00, 0x01, 0x00, 0x00, 0x01, 0x25, 0x01, 0x25, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x26, 0x01, 0x26, 0x00, 0x01, 0x00, 0x00, 0x01, 0x2a, 0x01,
	0x2a, 0x00, 0x01, 0x00, 0x00, 0x01, 0x2b, 0x01, 0x2b, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x2c, 0x01, 0x2c, 0x00, 0x01, 0x00, 0x00, 0x01, 0x2d, 0x01,
	0x2d, 0x00, 0x01, 0x00, 0x00, 0x01, 0x2e, 0x01, 0x2e, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x2f, 0x01, 0x2f, 0x00, 0x01, 0x00, 0x00, 0x01, 0x30, 0x01,
	0x30, 0x00, 0x01, 0x00, 0x00, 0x01, 0x31, 0x01, 0x31, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x32, 0x01, 0x32, 0x00, 0x01, 0x00, 0x00, 0x01, 0x33, 0x01,
	0x33, 0x00, 0x01, 0x00, 0x00, 0x01, 0x34, 0x01, 0x34, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x35, 0x01, 0x35, 0x00, 0x01, 0x00, 0x00, 0x01, 0x36, 0x01,
	0x36, 0x00, 0x01, 0x00, 0x00, 0x01, 0x37, 0x01, 0x37, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x38, 0x01, 0x38, 0x00, 0x01, 0x00, 0x00, 0x01, 0x39, 0x01,
	0x39, 0x00, 0x01, 0x00, 0x00, 0x01, 0x3b, 0x01, 0x3b, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x3c, 0x01, 0x3c, 0x00, 0x01, 0x00, 0x00, 0x01, 0x3d, 0x01,
	0x3d, 0x00, 0x01, 0x00, 0x00, 0x01, 0x3e, 0x01, 0x3e, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x3f, 0x01, 0x3f, 0x00, 0x01, 0x00, 0x00, 0x01, 0x40, 0x01,
	0x40, 0x00, 0x01, 0x00, 0x00, 0x01, 0x41, 0x01, 0x41, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x42, 0x01, 0x42, 0x00, 0x01, 0x00, 0x00, 0x01, 0x43, 0x01,
	0x43, 0x00, 0x01, 0x00, 0x00, 0x01, 0x47, 0x01, 0x47, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x48, 0x01, 0x48, 0x00, 0x01, 0x00, 0x00, 0x01, 0x49, 0x01,
	0x49, 0x00, 0x01, 0x00, 0x00, 0x01, 0x4b, 0x01, 0x4b, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x4d, 0x01, 0x4d, 0x00, 0x01, 0x00, 0x00, 0x01, 0x4e, 0x01,
	0x4e, 0x00, 0x01, 0x00, 0x00, 0x01, 0x4f, 0x01, 0x4f, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x50, 0x01, 0x50, 0x00, 0x01, 0x00, 0x00, 0x01, 0x51, 0x01,
	0x51, 0x00, 0x01, 0x00, 0x00, 0x01, 0x52, 0x01, 0x52, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x53, 0x01, 0x53, 0x00, 0x01, 0x00, 0x00, 0x01, 0x54, 0x01,
	0x54, 0x00, 0x01, 0x00, 0x00, 0x01, 0x55, 0x01, 0x55, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x56, 0x01, 0x56, 0x00, 0x01, 0x00, 0x00, 0x01, 0x57, 0x01,
	0x57, 0x00, 0x01, 0x01, 0x57, 0x00, 0x01, 0x00, 0x00, 0x01, 0x58, 0x01,
	0x58, 0x00, 0x01, 0x00, 0x00, 0x01, 0x59, 0x01, 0x59, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x5a, 0x01, 0x5a, 0x00, 0x01, 0x00, 0x00, 0x01, 0x5c, 0x01,
	0x5c, 0x00, 0x01, 0x00, 0x00, 0x01, 0x5d, 0x01, 0x5d, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x62, 0x01, 0x62, 0x00, 0x01, 0x00, 0x00, 0x01, 0x63, 0x01,
	0x63, 0x00, 0x01, 0x00, 0x00, 0x01, 0x67, 0x01, 0x67, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x6b, 0x01, 0x6b, 0x00, 0x01, 0x00, 0x00, 0x01, 0x7c, 0x01,
	0x7c, 0x00, 0x01, 0x01, 0x42, 0x00, 0x01, 0x00, 0x00, 0x01, 0x7d, 0x01,
	0x7d, 0x00, 0x01, 0x00, 0x00, 0x01, 0x7e, 0x01, 0x7e, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x7f, 0x01, 0x7f, 0x00, 0x01, 0x00, 0x00, 0x01, 0x80, 0x01,
	0x80, 0x00, 0x01, 0x00, 0x00, 0x01, 0x81, 0x01, 0x81, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x82, 0x01, 0x82, 0x00, 0x01, 0x00, 0x00, 0x01, 0x83, 0x01,
	0x83, 0x00, 0x01, 0x00, 0x00, 0x01, 0x84, 0x01, 0x84, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x85, 0x01, 0x85, 0x00, 0x01, 0x00, 0x00, 0x01, 0x86, 0x01,
	0x86, 0x00, 0x01, 0x00, 0x00, 0x01, 0x87, 0x01, 0x87, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x89, 0x01, 0x89, 0x00, 0x01, 0x00, 0x00, 0x01, 0x8a, 0x01,
	0x8a, 0x00, 0x01, 0x00, 0x00, 0x01, 0x8b, 0x01, 0x8b, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x8c, 0x01, 0x8c, 0x00, 0x01, 0x00, 0x00, 0x01, 0x8d, 0x01,
	0x8d, 0x00, 0x01, 0x00, 0x00, 0x01, 0x8e, 0x01, 0x8e, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x8f, 0x01, 0x8f, 0x00, 0x01, 0x00, 0x00, 0x01, 0x90, 0x01,
	0x90, 0x00, 0x01, 0x00, 0x00, 0x01, 0x91, 0x01, 0x91, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x94, 0x01, 0x94, 0x00, 0x01, 0x01, 0x25, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x95, 0x01, 0x95, 0x00, 0x01, 0x00, 0x00, 0x01, 0x96, 0x01,
	0x96, 0x00, 0x01, 0x00, 0x00, 0x01, 0x97, 0x01, 0x97, 0x00, 0x01, 0x00,
	0x00, 0x01, 0x9d, 0x01, 0x9d, 0x00, 0x01, 0x00, 0x00, 0x01, 0x9e, 0x01,
	0x9e, 0x00, 0x01, 0x00, 0x00, 0x01, 0x9f, 0x01, 0x9f, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xa0, 0x01, 0xa0, 0x00, 0x01, 0x00, 0x00, 0x01, 0xa1, 0x01,
	0xa1, 0x00, 0x01, 0x00, 0x00, 0x01, 0xa2, 0x01, 0xa2, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xa3, 0x01, 0xa3, 0x00, 0x01, 0x00, 0x00, 0x01, 0xa4, 0x01,
	0xa4, 0x00, 0x01, 0x00, 0x00, 0x01, 0xa5, 0x01, 0xa5, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xa6, 0x01, 0xa6, 0x00, 0x01, 0x00, 0x00, 0x01, 0xa7, 0x01,
	0xa7, 0x00, 0x01, 0x00, 0x00, 0x01, 0xa8, 0x01, 0xa8, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xa9, 0x01, 0xa9, 0x00, 0x01, 0x00, 0x00, 0x01, 0xaa, 0x01,
	0xaa, 0x00, 0x01, 0x00, 0x00, 0x01, 0xab, 0x01, 0xab, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xad, 0x01, 0xad, 0x00, 0x01, 0x00, 0x00, 0x01, 0xae, 0x01,
	0xae, 0x00, 0x01, 0x00, 0x00, 0x01, 0xaf, 0x01, 0xaf, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xb0, 0x01, 0xb0, 0x00, 0x01, 0x00, 0x00, 0x01, 0xb1, 0x01,
	0xb1, 0x00, 0x01, 0x00, 0x00, 0x01, 0xc1, 0x01, 0xc1, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xc2, 0x01, 0xc2, 0x00, 0x01, 0x01, 0x25, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xc6, 0x01, 0xc6, 0x00, 0x01, 0x00, 0x00, 0x01, 0xc7, 0x01,
	0xc7, 0x00, 0x01, 0x00, 0x00, 0x01, 0xc8, 0x01, 0xc8, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xc9, 0x01, 0xc9, 0x00, 0x01, 0x00, 0x00, 0x01, 0xca, 0x01,
	0xca, 0x00, 0x01, 0x01, 0x9f, 0x00, 0x01, 0x00, 0x00, 0x01, 0xcb, 0x01,
	0xcb, 0x00, 0x01, 0x01, 0xa0, 0x00, 0x01, 0x00, 0x00, 0x01, 0xcc, 0x01,
	0xcc, 0x00, 0x01, 0x01, 0xa2, 0x00, 0x01, 0x00, 0x00, 0x01, 0xcd, 0x01,
	0xcd, 0x00, 0x01, 0x01, 0xa3, 0x00, 0x01, 0x00, 0x00, 0x01, 0xce, 0x01,
	0xce, 0x00, 0x01, 0x01, 0xb1, 0x00, 0x01, 0x00, 0x00, 0x01, 0xcf, 0x01,
	0xcf, 0x00, 0x01, 0x01, 0x22, 0x00, 0x01, 0x00, 0x00, 0x01, 0xd2, 0x01,
	0xd2, 0x00, 0x01, 0x00, 0x00, 0x01, 0xd3, 0x01, 0xd3, 0x00, 0x01, 0x01,
	0xab, 0x00, 0x01, 0x00, 0x00, 0x01, 0xd4, 0x01, 0xd4, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xd5, 0x01, 0xd5, 0x00, 0x01, 0x00, 0x00, 0x01, 0xd6, 0x01,
	0xd6, 0x00, 0x01, 0x00, 0x00, 0x01, 0xd7, 0x01, 0xd7, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xd8, 0x01, 0xd8, 0x00, 0x01, 0x00, 0x00, 0x01, 0xd9, 0x01,
	0xd9, 0x00, 0x01, 0x00, 0x00, 0x01, 0xda, 0x01, 0xda, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xdb, 0x01, 0xdb, 0x00, 0x01, 0x00, 0x00, 0x01, 0xdc, 0x01,
	0xdc, 0x00, 0x01, 0x00, 0x00, 0x01, 0xdd, 0x01, 0xdd, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xde, 0x01, 0xde, 0x00, 0x01, 0x00, 0x00, 0x01, 0xdf, 0x01,
	0xdf, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe0, 0x01, 0xe0, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xe1, 0x01, 0xe1, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe2, 0x01,
	0xe2, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe3, 0x01, 0xe3, 0x00, 0x01, 0x01,
	0x6b, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe4, 0x01, 0xe4, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xe5, 0x01, 0xe5, 0x00, 0x01, 0x00, 0x00, 0x01, 0xe6, 0x01,
	0xe6, 0x00, 0x01, 0x00, 0x00, 0x01, 0xea, 0x01, 0xea, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xeb, 0x01, 0xeb, 0x00, 0x01, 0x00, 0x00, 0x01, 0xec, 0x01,
	0xec, 0x00, 0x01, 0x00, 0x00, 0x01, 0xed, 0x01, 0xed, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xee, 0x01, 0xee, 0x00, 0x01, 0x00, 0x00, 0x01, 0xef, 0x01,
	0xef, 0x00, 0x01, 0x00, 0x00, 0x01, 0xf0, 0x01, 0xf0, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xf2, 0x01, 0xf2, 0x00, 0x01, 0x00, 0x00, 0x01, 0xf3, 0x01,
	0xf3, 0x00, 0x01, 0x00, 0x00, 0x01, 0xf4, 0x01, 0xf4, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xf5, 0x01, 0xf5, 0x00, 0x01, 0x00, 0x00, 0x01, 0xf6, 0x01,
	0xf6, 0x00, 0x01, 0x00, 0x00, 0x01, 0xfd, 0x01, 0xfd, 0x00, 0x01, 0x00,
	0x00, 0x01, 0xfe, 0x01, 0xfe, 0x00, 0x01, 0x00, 0x00, 0x02, 0x01, 0x02,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x04, 0x02, 0x04, 0x00, 0x01, 0x00, 0x00, 0x02, 0x05, 0x02,
	0x05, 0x00, 0x01, 0x00, 0x00, 0x02, 0x06, 0x02, 0x06, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x07, 0x02, 0x07, 0x00, 0x01, 0x00, 0x00, 0x02, 0x08, 0x02,
	0x08, 0x00, 0x01, 0x00, 0x00, 0x02, 0x09, 0x02, 0x09, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x0a, 0x02, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x02, 0x0b, 0x02,
	0x0b, 0x00, 0x01, 0x00, 0x00, 0x02, 0x0c, 0x02, 0x0c, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x0d, 0x02, 0x0d, 0x00, 0x01, 0x00, 0x00, 0x02, 0x0e, 0x02,
	0x0e, 0x00, 0x01, 0x00, 0x00, 0x02, 0x0f, 0x02, 0x0f, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x10, 0x02, 0x10, 0x00, 0x01, 0x00, 0x00, 0x02, 0x11, 0x02,
	0x11, 0x00, 0x01, 0x00, 0x00, 0x02, 0x12, 0x02, 0x12, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x13, 0x02, 0x13, 0x00, 0x01, 0x00, 0x00, 0x02, 0x14, 0x02,
	0x14, 0x00, 0x01, 0x00, 0x00, 0x02, 0x15, 0x02, 0x15, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x16, 0x02, 0x16, 0x00, 0x01, 0x00, 0x00, 0x02, 0x17, 0x02,
	0x17, 0x00, 0x01, 0x00, 0x00, 0x02, 0x18, 0x02, 0x18, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x19, 0x02, 0x19, 0x00, 0x01, 0x00, 0x00, 0x02, 0x1a, 0x02,
	0x1a, 0x00, 0x01, 0x00, 0x00, 0x02, 0x1b, 0x02, 0x1b, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x06, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x07, 0x00,
	0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x09, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0f, 0x00,
	0x17, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x11, 0x00,
	0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x14, 0x00,
	0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x27, 0x00,
	0x78, 0x00, 0x01, 0x01, 0x5d, 0x00, 0x01, 0x01, 0x26, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x3a, 0x00, 0x3a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x44, 0x00,
	0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x46, 0x00,
	0x00, 0x00, 0x4a, 0x00, 0x6d, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4c, 0x00,
	0x00, 0x00, 0x5b, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x5e, 0x00,
	0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x17, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x60, 0x00, 0x01, 0x00, 0x00, 0x00, 0x61, 0x00,
	0x60, 0x00, 0x01, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x65, 0x00, 0x65, 0x00, 0x01, 0x00, 0x00, 0x00, 0x66, 0x00,
	0x66, 0x00, 0x01, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x69, 0x00,
	0x00, 0x00, 0x6a, 0x00, 0x6a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x6c, 0x00,
	0x6d, 0x00, 0x01, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x6d, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x6e, 0x00, 0x6f, 0x00, 0x01, 0x00, 0x00, 0x00, 0x6f, 0x00,
	0x6f, 0x00, 0x01, 0x00, 0x00, 0x00, 0x70, 0x00, 0x70, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x71, 0x00, 0x71, 0x00, 0x01, 0x00, 0x00, 0x00, 0x72, 0x00,
	0x72, 0x00, 0x01, 0x00, 0x00, 0x00, 0x73, 0x00, 0x73, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x74, 0x00, 0x66, 0x00, 0x01, 0x00, 0x00, 0x00, 0x76, 0x00,
	0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0x79, 0x00, 0x79, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x7a, 0x00, 0x7a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7b, 0x00,
	0x7b, 0x00, 0x01, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x92, 0x00,
	0x92, 0x00, 0x01, 0x00, 0x00, 0x00, 0x93, 0x00, 0x93, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x98, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x99, 0x00,
	0x02, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x9a, 0x00, 0x02, 0x00, 0x0a, 0x00,
	0x00, 0x00, 0x9b, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x9c, 0x00,
	0x0c, 0x00, 0x0a, 0x00, 0x00, 0x00, 0xac, 0x00, 0x02, 0x00, 0x0a, 0x00,
	0x00, 0x00, 0xb2, 0x00, 0xb2, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb3, 0x00,
	0xb3, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb4, 0x00, 0xb4, 0x00, 0x01, 0x00,
	0x00, 0x00, 0xb5, 0x00, 0xb5, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb6, 0x00,
	0xb6, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb7, 0x00, 0xb7, 0x00, 0x01, 0x00,
	0x00, 0x00, 0xb8, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x00, 0x00, 0xb9, 0x00,
	0xb2, 0x00, 0x01, 0x00, 0x00, 0x00, 0xba, 0x00, 0xb3, 0x00, 0x01, 0x00,
	0x00, 0x00, 0xbb, 0x00, 0xb4, 0x00, 0x01, 0x00, 0x00, 0x00, 0xbc, 0x00,
	0xb5, 0x00, 0x01, 0x00, 0x00, 0x00, 0xbd, 0x00, 0xb6, 0x00, 0x01, 0x00,
	0x00, 0x00, 0xbe, 0x00, 0xb7, 0x00, 0x01, 0x00, 0x00, 0x00, 0xbf, 0x00,
	0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x70, 0x00, 0x01, 0x00,
	0x00, 0x00, 0xc4, 0x00, 0x71, 0x00, 0x01, 0x00, 0x00, 0x00, 0xc5, 0x00,
	0x72, 0x00, 0x01, 0x00, 0x00, 0x00, 0xd0, 0x00, 0xd0, 0x00, 0x01, 0x00,
	0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0xe7, 0x00, 0xe7, 0x00, 0x01, 0x00,
	0x00, 0x00, 0xe8, 0x00, 0xe7, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe9, 0x00,
	0xe9, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf1, 0x00, 0x6d, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x03, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00
};

// the same request with its logon types byte rewritten 0x0d -> 0x05 and its
// CCAP_OCI1 byte set to one of the OCI1_10G* bytes, as an oraproxy rewrote
// it in packet [0009] of the 10735-fedora9x86-oci10-oci8-ccap05-* captures in
// samples/.  see rewrite10gDataTypeRequest()
static const size_t		ORA_10G_DATA_TYPE_REQUEST_LOGON_TYPES=
					ORA_DATA_TYPE_REQUEST_CCAP+
					ORA_CCAP_LOGON_TYPES;
static const size_t		ORA_10G_DATA_TYPE_REQUEST_OCI1=
					ORA_DATA_TYPE_REQUEST_CCAP+
					ORA_CCAP_OCI1;
static unsigned char	rewritten10gdatatyperequest[
					sizeof(ORA_10G_DATA_TYPE_REQUEST)];

// its classic phase one call, TTI_LOGON_PRESENT_USER, with the ttc code and
// the sequence number 2, and the TTI_LOGON_PRESENT_PWD that answers the 64 hex
// character challenge the module gave it, sequence number 3 - [0011] and
// [0013] of samples/10545-fedora9x86-oci10-olog-sdu8192-correctpassword-
// sqlrelay121-before.oraproxy.  The user is testuser, and the password it
// carries is encrypted under that session's key, which no other session has
static const unsigned char	ORA_10G_CLASSIC_PHASE_ONE[]={
	0x03, 0x52, 0x02, 0x01, 0x01, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x01, 0x4b, 0x01, 0x01, 0x0f, 0x02, 0x11, 0x30, 0x01,
	0x01, 0x0c, 0x01, 0x01, 0x90, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x01,
	0x23, 0x01, 0x08, 0x74, 0x65, 0x73, 0x74, 0x75, 0x73, 0x65, 0x72, 0x19,
	0x66, 0x65, 0x64, 0x6f, 0x72, 0x61, 0x39, 0x78, 0x38, 0x36, 0x2e, 0x66,
	0x69, 0x72, 0x73, 0x74, 0x77, 0x6f, 0x72, 0x6b, 0x73, 0x2e, 0x63, 0x6f,
	0x6d, 0x05, 0x64, 0x6d, 0x75, 0x73, 0x65, 0x04, 0x34, 0x33, 0x36, 0x32,
	0x30, 0x6f, 0x63, 0x69, 0x6c, 0x6f, 0x67, 0x69, 0x6e, 0x31, 0x30, 0x40,
	0x66, 0x65, 0x64, 0x6f, 0x72, 0x61, 0x39, 0x78, 0x38, 0x36, 0x2e, 0x66,
	0x69, 0x72, 0x73, 0x74, 0x77, 0x6f, 0x72, 0x6b, 0x73, 0x2e, 0x63, 0x6f,
	0x6d, 0x20, 0x28, 0x54, 0x4e, 0x53, 0x20, 0x56, 0x31, 0x2d, 0x56, 0x33,
	0x29
};

static const unsigned char	ORA_10G_CLASSIC_PHASE_TWO[]={
	0x03, 0x51, 0x03, 0x01, 0x01, 0x18, 0x01, 0x01, 0xc0, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x01, 0x01, 0x4b, 0x01, 0x01, 0x0f, 0x02, 0x11, 0x30,
	0x01, 0x01, 0x0c, 0x01, 0x01, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x74, 0x65, 0x73, 0x74, 0x75, 0x73, 0x65, 0x72, 0x40, 0x33,
	0x32, 0x33, 0x41, 0x39, 0x42, 0x46, 0x38, 0x37, 0x31, 0x32, 0x36, 0x35,
	0x38, 0x35, 0x32, 0x43, 0x38, 0x37, 0x39, 0x33, 0x39, 0x30, 0x33, 0x41,
	0x39, 0x37, 0x43, 0x31, 0x45, 0x39, 0x32, 0x43, 0x45, 0x30, 0x35, 0x34,
	0x43, 0x43, 0x34, 0x39, 0x43, 0x35, 0x33, 0x42, 0x37, 0x38, 0x39, 0x33,
	0x43, 0x30, 0x43, 0x32, 0x42, 0x34, 0x32, 0x36, 0x34, 0x30, 0x38, 0x32,
	0x33, 0x45, 0x35, 0x19, 0x66, 0x65, 0x64, 0x6f, 0x72, 0x61, 0x39, 0x78,
	0x38, 0x36, 0x2e, 0x66, 0x69, 0x72, 0x73, 0x74, 0x77, 0x6f, 0x72, 0x6b,
	0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x05, 0x64, 0x6d, 0x75, 0x73, 0x65, 0x04,
	0x34, 0x33, 0x36, 0x32, 0x30, 0x6f, 0x63, 0x69, 0x6c, 0x6f, 0x67, 0x69,
	0x6e, 0x31, 0x30, 0x40, 0x66, 0x65, 0x64, 0x6f, 0x72, 0x61, 0x39, 0x78,
	0x38, 0x36, 0x2e, 0x66, 0x69, 0x72, 0x73, 0x74, 0x77, 0x6f, 0x72, 0x6b,
	0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x20, 0x28, 0x54, 0x4e, 0x53, 0x20, 0x56,
	0x31, 0x2d, 0x56, 0x33, 0x29
};

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
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 allowedlogonversion=9","ORACLEPROTOCOLPORT15",1535,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 allowedlogonversion=12","ORACLEPROTOCOLPORT16",1536,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"}
};

enum {
	LISTENER_12_1,
	LISTENER_11_2,
	LISTENER_10_2,
	LISTENER_FLOOR_11,
	LISTENER_FLOOR_12A,
	LISTENER_FLOOR_9,
	LISTENER_FLOOR_12
};

// the login form a case sends: the tagged 0x76 call, which this builds, or
// the classic 0x52 one, which is replayed from the genuine 10.2 client
enum loginform {
	FORM_TAGGED,
	FORM_CLASSIC
};

// what a case expects of the listener
enum outcome {

	// a challenge of the verifier the case names
	OUTCOME_CHALLENGE,

	// the same, and then the login that answers it
	OUTCOME_LOGIN,

	// a challenge, and nothing checked about it
	OUTCOME_ADMITTED,

	// ORA-28040 behind a break and a reset, in place of the challenge, with
	// the whole error object compared to a real server's
	OUTCOME_REFUSED,

	// the same, with only the break, the reset and the message checked
	OUTCOME_REFUSED_BASIC,

	// a challenge, and then ORA-01017 for the phase two call whatever it
	// carries.  a classic login only
	OUTCOME_BARRED
};

struct testcase {
	size_t		listener;
	unsigned char	logontypes;
	loginform	form;
	verifier	expected;
	outcome		result;
};

// within each listener the tagged logins come first and the classic ones last.
// the floors are what a real 12.2 server does with these bytes, except where
// the header says a case is left unchecked
static const testcase	testcases[]={

	// at 12.1, each client gets its own verifier.  the 0x00 client's is the
	// module's, and its logon types don't ask for a challenge shape
	{LISTENER_12_1,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_9I,OUTCOME_ADMITTED},
	{LISTENER_12_1,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_12_1,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_12_1,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_12_1,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_12_1,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// at 11.2, nothing newer than 11g
	{LISTENER_11_2,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_9I,OUTCOME_ADMITTED},
	{LISTENER_11_2,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_11_2,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_11_2,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_11G,OUTCOME_CHALLENGE},
	{LISTENER_11_2,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_11_2,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// at 10.2, nothing newer than 10g
	{LISTENER_10_2,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_9I,OUTCOME_ADMITTED},
	{LISTENER_10_2,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_10_2,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// a floor of 9 refuses only a client that advertises nothing.  the
	// real server did the same to genuine ojdbc14 and let a genuine 9i
	// client and the 10.2 and OCI 23 clients in.  the 11.2 client has all
	// the bits a floor of 9 could look at, so it is in whichever it is
	{LISTENER_FLOOR_9,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_9,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_FLOOR_9,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// a floor of 11 refuses a client without O5LOGON, which is the same
	// as a floor of 10, and it admits the 10g verifier and a classic login
	{LISTENER_FLOOR_11,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_11,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED},
	{LISTENER_FLOOR_11,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_FLOOR_11,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_11,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_FLOOR_11,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// a floor of 12 refuses a client without O5LOGON_NP.  the 11.2 client
	// and OCI 23 log in with a tagged call, and answer a classic one with
	// a challenge and then ORA-01017
	{LISTENER_FLOOR_12,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED},
	{LISTENER_FLOOR_12,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_FLOOR_12,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_12,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_BARRED},
	{LISTENER_FLOOR_12,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_BARRED},

	// a floor of 12a refuses a client without O7LOGON, a tagged one that
	// doesn't get the 12c verifier, and answers OCI 23's classic login the
	// way a floor of 12 does
	{LISTENER_FLOOR_12A,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12A,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED},
	{LISTENER_FLOOR_12A,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_REFUSED},
	{LISTENER_FLOOR_12A,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_12A,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12A,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_BARRED}
};

// the genuine 10.2 client, in the session described at the top.  every
// result but the 11g and 10g verifiers is what a real 12.2 server did with
// this client: the floors it admits and refuses it at, with either login
static const testcase	genuine10gcases[]={

	// the verifier follows the ceiling, and the tagged login completes
	// where it's the 11g one
	{LISTENER_12_1,LOGON_TYPES_10G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_12_1,LOGON_TYPES_10G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_11_2,LOGON_TYPES_10G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_11_2,LOGON_TYPES_10G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_10G,FORM_TAGGED,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_10G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// floors 9 and 11 admit it, whichever login it uses
	{LISTENER_FLOOR_9,LOGON_TYPES_10G,FORM_TAGGED,VERIFIER_11G,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_10G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_11,LOGON_TYPES_10G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_FLOOR_11,LOGON_TYPES_10G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// floors 12 and 12a refuse it in phase one, whichever login it uses,
	// since it lacks O5LOGON_NP
	{LISTENER_FLOOR_12,LOGON_TYPES_10G,FORM_TAGGED,VERIFIER_11G,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12,LOGON_TYPES_10G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12A,LOGON_TYPES_10G,FORM_TAGGED,VERIFIER_11G,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12A,LOGON_TYPES_10G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_REFUSED_BASIC}
};

// the genuine 10.2 client with its logon types rewritten to 0x05 and its
// fast session propagate bit set.  it gets the 9i verifier whatever the
// ceiling, the challenge and the refusals carry the end to end field, and
// floors 10 and up refuse it, as a real 12.2 server did at 10 and 11 -
// samples/10735-fedora9x86-oci10-oci8-ccap05-realserver-oracle122-allowed10
// and -allowed11, and the olog ones beside them for the classic login
static const testcase	rewritten10gcases[]={
	{LISTENER_12_1,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_12_1,LOGON_TYPES_9I,FORM_CLASSIC,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_11_2,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_ADMITTED},
	{LISTENER_FLOOR_11,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED},
	{LISTENER_FLOOR_11,LOGON_TYPES_9I,FORM_CLASSIC,VERIFIER_9I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED},
	{LISTENER_FLOOR_12A,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED}
};

// the same with the bit cleared too, which leaves the end to end field out
// of the challenge and the refusal, as a real 12.2 server did - samples/
// 10735-fedora9x86-oci10-oci8-ccap05-cap16xef-realserver-oracle122 and
// -oracle122-allowed11
static const testcase	rewritten10gnofspcases[]={
	{LISTENER_12_1,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_11,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED}
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
// own, so a failure says which.  "endtoend" is the value of its end to end
// field, and "oci7summary" says it is the oci7 summary without one
static void checkTrailer(oracleprotocolclient *client, const char *label,
				unsigned char endtoend, unsigned char seqnumber,
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
		expected[ORA_NATIVE_TRAILER_END_TO_END]=endtoend;
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
		expected[4]=endtoend;
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

// whether phase one got a challenge, and not a refusal
static bool checkGotChallenge(oracleprotocolclient *client,
					const char *label) {

	stringbuffer	l;
	l.append(label)->append(" got a challenge");
	bool	ok=(client->getResponseTtcCode()==ORA_TTC_OK &&
						!client->gotBreak());
	report(l.getString(),ok);
	return ok;
}

// the challenge a case got, against the verifier it should have got
static void checkChallenge(oracleprotocolclient *client,
				const char *label,
				verifier expected,
				unsigned char seqnumber) {

	stringbuffer	l;

	if (!checkGotChallenge(client,label)) {
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

	// a 9i challenge's oci7 summary has the end to end field for a client
	// with fast session propagate, and carries the call number in it
	bool	oci7endtoend=(o3logon && client->getFastSessionPropagate());
	l.clear();
	l.append(label)->append(" trailer");
	if (oci7endtoend) {
		l.append(" with the end to end field");
	}
	checkTrailer(client,l.getString(),
			(o3logon)?seqnumber:ORA_AUTH_PHASE_CHALLENGE,
			seqnumber,o3logon && !oci7endtoend);
}

static bool isHexCharacter(unsigned char c) {
	return ((c>='0' && c<='9') || (c>='A' && c<='F'));
}

// the challenge a classic 0x52 login got.  it has no AUTH_ pairs, so what is
// checked is its session key: the only run of hex characters in it, and as
// long as the verifier's key is - 64 for the 10g verifier, which a real
// server sends the genuine 10.2, 11.2 and OCI 23 clients, and 32 for the 9i
// one
static void checkClassicChallenge(oracleprotocolclient *client,
					const char *label,
					verifier expected) {

	if (!checkGotChallenge(client,label)) {
		return;
	}

	size_t	keysize=(expected==VERIFIER_9I)?SESSKEY_HEX_SIZE_9I:
						SESSKEY_HEX_SIZE_10G;

	const unsigned char	*response=client->getResponse();
	size_t			responsesize=client->getResponseSize();
	size_t			runs=0;
	size_t			runsize=0;
	size_t			i=0;
	while (i<responsesize) {
		if (!isHexCharacter(response[i])) {
			i++;
			continue;
		}
		size_t	end=i;
		while (end<responsesize && isHexCharacter(response[end])) {
			end++;
		}
		if (end-i>=SESSKEY_HEX_SIZE_9I/2) {
			runs++;
			runsize=end-i;
		}
		i=end;
	}

	stringbuffer	l;
	l.append(label)->append(" session key is ");
	l.append((uint64_t)keysize)->append(" hex characters");
	bool	ok=(runs==1 && runsize==keysize);
	report(l.getString(),ok);
	if (!ok) {
		dumpBytes("challenge",response,responsesize);
	}
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
// from the 10.2 server.
//
// the modern shape that an o3logon session gets when its client sets fast
// session propagate is inferred too.  a real 12.2 server sends that client
// its fixed width form, with the end to end field 0: packet [0017] of
// samples/10735-fedora9x86-oci10-oci8-ccap05-realserver-oracle122-
// allowed11.oraproxy
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
// session whose client lacks fast session propagate gets the narrow oci7
// error shape and any other the modern one, which has one more zero field,
// the end to end field, ahead of the ora number - see
// sendErrorPacket() in src/protocols/oracle.cpp.  in the native encoding
// every session gets the native struct.  "exact" says whether to compare that
// object byte for byte: a real server's is known for the tagged login of a
// client at field version 6 or up, inferred for the rewritten 10.2 client's
// tagged login, and not known for the others
static void checkRefusal(oracleprotocolclient *client,
				const char *label,
				const listenerinfo *li,
				verifier expected,
				unsigned char seqnumber,
				bool exact) {

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

	if (!exact) {
		return;
	}

	const char	*message="ORA-28040: No matching authentication "
					"protocol\n";
	if (nativeencoding) {
		checkNativeError(client,label,li,
				ORA_NO_MATCHING_AUTH_PROTOCOL,
				message,seqnumber,false);
	} else {
		checkPortableError(client,label,li,
				(expected==VERIFIER_9I &&
					!client->getFastSessionPropagate()),
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

// a classic login the floor bars after its challenge: the phase two call gets
// a break and a reset and then ORA-01017, and a real server sends the same
// for a password that is wrong.  oracleprotocolclient can't answer a 10g
// o3logon challenge, so the call is the genuine 10.2 client's own, encrypted
// under another session's key, and the password in it is wrong for this one.
// a wrong-password reply passes this check too, so it proves the challenge is
// sent and the sequence that follows, not that a right password still gets
// ORA-01017.  that rests on the live captures at floors 12 and 12a in
// test/protocol/oracle/samples/
static void checkBarred(oracleprotocolclient *client, const char *label) {

	stringbuffer	l;

	l.append(label)->append(" phase two is answered");
	bool	ok=client->sendRawRequest(ORA_10G_CLASSIC_PHASE_TWO,
					sizeof(ORA_10G_CLASSIC_PHASE_TWO),
					"phase two");
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	l.clear();
	l.append(label)->append(" got a break and a reset first");
	report(l.getString(),client->gotBreak());
	if (client->gotBreak()) {
		checkMarkerFlags(client,label);
	}

	l.clear();
	l.append(label)->append(" got ORA-01017");
	report(l.getString(),
		client->getResponseTtcCode()==ORA_TTC_ERROR &&
		client->responseContains("ORA-01017: invalid "
						"username/password"));
}

// how a case connects: as the hand built client the other cases use, or as
// the genuine 10.2 client does, at its default sdu of 2048 with its data type
// request in two packets or, as when its connect descriptor asks for 8192, in
// one.  or as that client with its logon types rewritten, at its default
// sdu, with CCAP_OCI1's fast session propagate bit as sent or cleared
enum session {
	SESSION_HAND_BUILT,
	SESSION_GENUINE_10G,
	SESSION_GENUINE_10G_BIG_SDU,
	SESSION_REWRITTEN_10G,
	SESSION_REWRITTEN_10G_BIT_CLEAR
};

static const char *sessionLabel(session sess) {
	switch (sess) {
		case SESSION_GENUINE_10G:
			return "genuine 10.2 client, sdu 2048, ";
		case SESSION_GENUINE_10G_BIG_SDU:
			return "genuine 10.2 client, sdu 8192, ";
		case SESSION_REWRITTEN_10G:
			return "rewritten 10.2 client, CCAP_OCI1 0xff, ";
		case SESSION_REWRITTEN_10G_BIT_CLEAR:
			return "rewritten 10.2 client, CCAP_OCI1 0xef, ";
		default:
			return "";
	}
}

// the CCAP_OCI1 byte of the genuine client whose logon types a hand built
// case advertises
static unsigned char genuineOci1(unsigned char logontypes) {
	if (logontypes==LOGON_TYPES_NONE) {
		return OCI1_OJDBC14;
	}
	if (logontypes==LOGON_TYPES_9I) {
		return OCI1_9I;
	}
	return OCI1_10G;
}

// the genuine 10.2 client's data type request with its logon types
// rewritten to 0x05 and its CCAP_OCI1 byte set to "oci1"
static const unsigned char *rewrite10gDataTypeRequest(unsigned char oci1) {
	bytestring::copy(rewritten10gdatatyperequest,
				ORA_10G_DATA_TYPE_REQUEST,
				sizeof(ORA_10G_DATA_TYPE_REQUEST));
	rewritten10gdatatyperequest[ORA_10G_DATA_TYPE_REQUEST_LOGON_TYPES]=
								LOGON_TYPES_9I;
	rewritten10gdatatyperequest[ORA_10G_DATA_TYPE_REQUEST_OCI1]=oci1;
	return rewritten10gdatatyperequest;
}

// the data type negotiation a session sends.  a hand built one advertises
// "logontypes", and the CCAP_OCI1 byte of the client that sends it
static void setDataTypeRequest(oracleprotocolclient *client, session sess,
						unsigned char logontypes) {
	switch (sess) {
		case SESSION_HAND_BUILT:
			client->setFieldVersion(clientfieldversion);
			client->setLogonTypes(logontypes);
			client->setOci1(genuineOci1(logontypes));
			return;
		case SESSION_GENUINE_10G:
		case SESSION_GENUINE_10G_BIG_SDU:
			client->setDataTypeRequest(ORA_10G_DATA_TYPE_REQUEST,
					sizeof(ORA_10G_DATA_TYPE_REQUEST));
			break;
		case SESSION_REWRITTEN_10G:
			client->setDataTypeRequest(
				rewrite10gDataTypeRequest(OCI1_10G),
				sizeof(rewritten10gdatatyperequest));
			break;
		case SESSION_REWRITTEN_10G_BIT_CLEAR:
			client->setDataTypeRequest(
				rewrite10gDataTypeRequest(
					OCI1_10G_NO_FAST_SESSION_PROPAGATE),
				sizeof(rewritten10gdatatyperequest));
			break;
	}
	if (sess==SESSION_GENUINE_10G_BIG_SDU) {
		client->setSdu(ORA_10G_SDU_BIG);
	} else {
		client->setSdu(ORA_10G_SDU);
		client->setDataTypeRequestSplit(
					ORA_10G_DATA_TYPE_REQUEST_SPLIT);
	}
}

// the listeners a wrong password is tried against: 12.1, where a phase two
// error has the statement type and the second ora number, and 11.2, where
// it has neither
static const size_t	wrongpasswordlisteners[]={LISTENER_12_1,LISTENER_11_2};

// phase two with a password that isn't the user's: a break and a reset, then
// ORA-01017 in the native struct or the portable object, with the phase two
// sequence number as its call number.  "o3logon" tries it with the 9i
// verifier, portable encoding only.  the rewritten 10.2 client gets the 9i
// verifier, and the modern object since it sets fast session propagate
static void runWrongPassword(size_t index, bool o3logon, session sess) {

	const listenerinfo	*li=&listeners[wrongpasswordlisteners[index]];
	unsigned char		logontypes=(o3logon)?LOGON_TYPES_9I:
							LOGON_TYPES_11G;

	// a sequence number per case, apart from runCase()'s
	unsigned char	seqnumber=(unsigned char)(
				(sess!=SESSION_HAND_BUILT)?0x70+2*index:
				((o3logon)?0x60+2*index:0x50+2*index));

	char	label[256];
	charstring::printf(label,sizeof(label),
			"%s, %slogon types 0x%02x, field version %d, "
			"tns version 0x%04x, wrong password",li->name,
			sessionLabel(sess),logontypes,clientfieldversion,
			negotiatedconnectversion);
	stdoutput.printf("\n%s - expect ORA-01017\n",label);

	negotiatedfieldversion=(li->fieldversion<clientfieldversion)?
					li->fieldversion:clientfieldversion;

	oracleprotocolclient	client;
	client.setNativeEncoding(nativeencoding);
	client.setConnectVersion(clientconnectversion);
	setDataTypeRequest(&client,sess,logontypes);
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
			checkPortableError(&client,label,li,
					(o3logon &&
					!client.getFastSessionPropagate()),
					ORA_INVALID_USERNAME_PASSWORD,
					message,seqnumber+1,true);
		}
	}

	client.disconnect();
}

static void runCase(const testcase *tc, size_t index, session sess) {

	const listenerinfo	*li=&listeners[tc->listener];
	bool			genuine10g=(sess!=SESSION_HAND_BUILT);
	bool			o3logon=(tc->expected==VERIFIER_9I);
	bool			classic=(tc->form==FORM_CLASSIC);

	// a sequence number per case, so an echo can't pass by accident.  the
	// classic calls carry the ones they were captured with
	unsigned char	seqnumber=(unsigned char)(0x10+2*index);

	char	label[256];
	charstring::printf(label,sizeof(label),
				"%s, %slogon types 0x%02x, field version %d, "
				"tns version 0x%04x, %s login",
				li->name,sessionLabel(sess),
				tc->logontypes,clientfieldversion,
				negotiatedconnectversion,
				(classic)?"classic":"tagged");
	stringbuffer	expectation;
	switch (tc->result) {
		case OUTCOME_CHALLENGE:
		case OUTCOME_LOGIN:
			expectation.append(verifiernames[tc->expected]);
			expectation.append(" challenge");
			break;
		case OUTCOME_ADMITTED:
			expectation.append("a challenge");
			break;
		case OUTCOME_REFUSED:
		case OUTCOME_REFUSED_BASIC:
			expectation.append("ORA-28040");
			break;
		case OUTCOME_BARRED:
			expectation.append("a challenge, then ORA-01017");
			break;
	}
	stdoutput.printf("\n%s - expect %s\n",label,expectation.getString());

	// the classic call is replayed in portable form, and a 9i verifier
	// login is portable only
	if (nativeencoding && (o3logon || classic)) {
		stdoutput.printf("%s: skipped, %s is portable only\n",label,
					(classic)?"a classic login":"o3logon");
		return;
	}

	negotiatedfieldversion=(li->fieldversion<clientfieldversion)?
					li->fieldversion:clientfieldversion;

	oracleprotocolclient	client;
	client.setNativeEncoding(nativeencoding);
	client.setConnectVersion(clientconnectversion);
	client.setAuthSequenceNumbers(seqnumber,seqnumber+1);

	setDataTypeRequest(&client,sess,tc->logontypes);

	bool	lobread=false;
	if (!genuine10g) {
		client.setAnoNegotiation(true);

		// lobRead() sends the portable request only, and query3() the
		// one of field version 11.2, which lacks the fields
		// getQuery3Request() reads at 12.1 and up
		lobread=(tc->result==OUTCOME_LOGIN && !nativeencoding &&
				negotiatedfieldversion<ORA_CCAP_FIELD_VERSION_12_1);
		client.setLobZeroCopy(lobread);
	}

	stringbuffer	l;
	l.append(label)->append(" connect");
	if (!client.connect(host,getPort(li),sid)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	// the genuine 10.2 client's sessions run no ano exchange
	if (!genuine10g) {
		checkAnoResponseFlags(&client,label);
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

	l.clear();
	l.append(label)->append(" phase one");
	bool	sent=(classic)?
			client.sendRawRequest(ORA_10G_CLASSIC_PHASE_ONE,
					sizeof(ORA_10G_CLASSIC_PHASE_ONE),
					"phase one"):
			client.requestChallenge(user);
	if (!sent) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	switch (tc->result) {
		case OUTCOME_REFUSED:
		case OUTCOME_REFUSED_BASIC:
			checkRefusal(&client,label,li,tc->expected,seqnumber,
					tc->result==OUTCOME_REFUSED);
			break;
		case OUTCOME_ADMITTED:
			checkGotChallenge(&client,label);
			break;
		case OUTCOME_BARRED:
			if (checkGotChallenge(&client,label)) {
				checkBarred(&client,label);
			}
			break;
		case OUTCOME_CHALLENGE:
		case OUTCOME_LOGIN:
			if (classic) {
				checkClassicChallenge(&client,label,
								tc->expected);
				break;
			}
			checkChallenge(&client,label,tc->expected,seqnumber);
			if (tc->result==OUTCOME_LOGIN &&
				checkLogin(&client,label,li,seqnumber+1) &&
				lobread) {
				checkLobDataFlags(&client,label);
			}
			break;
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
				runCase(&testcases[i],i,SESSION_HAND_BUILT);
			}

			for (size_t i=0; i<sizeof(wrongpasswordlisteners)/
				sizeof(wrongpasswordlisteners[0]); i++) {
				runWrongPassword(i,false,SESSION_HAND_BUILT);
				if (!nativeencoding) {
					runWrongPassword(i,true,
							SESSION_HAND_BUILT);
				}
			}
		}
	}

	// the genuine 10.2 client's own session.  it speaks the portable
	// encoding and offers tns version 0x0139, so it runs once
	if (nativeencoding) {
		stdoutput.printf("\ngenuine 10.2 client: skipped, it is "
					"portable only\n");
	} else {
		clientfieldversion=ORA_CCAP_FIELD_VERSION_10_2;
		clientconnectversion=ORA_CONNECT_VERSION_10;
		negotiatedconnectversion=ORA_CONNECT_VERSION_10;
		stdoutput.printf("\n====== genuine 10.2 client, tns version "
					"0x%04x ======\n",
					negotiatedconnectversion);
		size_t	count=sizeof(genuine10gcases)/sizeof(genuine10gcases[0]);
		size_t	base=sizeof(testcases)/sizeof(testcases[0]);
		for (size_t i=0; i<count; i++) {
			runCase(&genuine10gcases[i],base+i,SESSION_GENUINE_10G);
		}
		for (size_t i=0; i<count; i++) {
			runCase(&genuine10gcases[i],base+count+i,
						SESSION_GENUINE_10G_BIG_SDU);
		}

		// and with its logon types rewritten
		base+=2*count;
		count=sizeof(rewritten10gcases)/sizeof(rewritten10gcases[0]);
		for (size_t i=0; i<count; i++) {
			runCase(&rewritten10gcases[i],base+i,
						SESSION_REWRITTEN_10G);
		}
		for (size_t i=0; i<sizeof(wrongpasswordlisteners)/
				sizeof(wrongpasswordlisteners[0]); i++) {
			runWrongPassword(i,true,SESSION_REWRITTEN_10G);
		}
		base+=count;
		count=sizeof(rewritten10gnofspcases)/
				sizeof(rewritten10gnofspcases[0]);
		for (size_t i=0; i<count; i++) {
			runCase(&rewritten10gnofspcases[i],base+i,
					SESSION_REWRITTEN_10G_BIT_CLEAR);
		}
	}

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
