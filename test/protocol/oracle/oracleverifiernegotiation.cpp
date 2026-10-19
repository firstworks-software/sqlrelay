// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/datetime.h>
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
//	- the 8i verifier for a tagged 0x76 login whose CCAP_LOGON_TYPES byte
//	  has none of the legacy bits (0x01 and 0x04) and O5LOGON (0x08),
//	  whatever the ceiling: a 16 hex character AUTH_SESSKEY and a single
//	  des AUTH_PASSWORD, as real 10.2, 11.2 and 12.2 servers run it with
//	  ojdbc14 10.2.0.5, which advertises no logon types
//	- the 9i verifier for any other client whose CCAP_LOGON_TYPES byte
//	  lacks O5LOGON (0x08), always under o3logon
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
// oracleprotocol11g's, and the one ojdbc14 also logs in to with the
// oracle_connectstrings auth module, which is oracleprotocolconnectstrings'.
//
// -native runs the same cases in the native encoding, which is where the
// trailer is a marshalled struct with the call number at offset 49.  The
// module reads and answers a 0x76 o3logon login and a classic one in the
// portable encoding only, so a native run checks what those cases advertise
// and stops there.  It skips the 8i exchange too, which no client on file
// runs in the native encoding.
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
//	  object of the hand built 0x00 client, which advertises field version
//	  11.2 or 12.1 where ojdbc14 advertises 3, and which no real server was
//	  measured answering
//	- the challenge of the 0x0f classic login, whose key a real server makes
//	  96 hex characters and the module 64
//	- the ORA-28040 and ORA-01017 objects of the classic login and of the
//	  genuine 10.2 client, which a real server sends in a fixed width form
//	  and the module in the count prefixed one that its clients read
//	- which of the legacy bits, 0x01 or 0x04, floor 9 looks at.  The bytes
//	  here have both or neither
//
// The hand built 0x00 client's challenge is the 8i one, with the module's
// modern trailer, the end to end field set to the login phase.  No real
// server was measured answering that client either, since its field version
// is not ojdbc14's.
//
// Genuine ojdbc14 10.2.0.5 is played in sessions of its own, portable only:
// its tns version 0x0134, its own data type request (logon types 0x00, field
// version 3, CCAP_OCI1 0x90), the bare TTI_VERSION call it sends ahead of its
// login, its 0x76 and 0x73 login calls with the AUTH_PASSWORD computed for
// each session, and its logoff.  Real 10.2, 11.2 and 12.2 servers answer that
// client in field version 3 objects, which have no end of call status and
// fewer trailing fields than the module's other objects, and every byte of
// those is checked against the real servers' (samples/10545-dev-ojdbc14-* and
// samples/10734-dev-ojdbc14-*): the version answer, which ends in "09 00",
// the 65 byte challenge with its 25 byte trailer, the login answer's trailer,
// the 78 byte ORA-01017 a wrong password or an unknown user gets, the 74 byte
// ORA-28040 floors 9, 11, 12 and 12a give it in phase one, and the "09 00"
// that answers its logoff.  The banner in the version answer is the module's
// own at each ceiling, where a real 11.2 and 12.2 server send an 11.1 one, and
// the login answer carries the module's pairs, not the real servers' 34 or 36.
// Before any of that, compute8iAuthPassword() has to reproduce the
// AUTH_PASSWORD of five real logins from their AUTH_SESSKEY.
//
// Beyond what ojdbc14 sends, and so the module's choices rather than a real
// server's: a malformed AUTH_PASSWORD - pad digit 8, a pad digit one short,
// no pad digit, a non-hex character, two hex characters short - is denied
// with the same 78 byte ORA-01017 at 12.1, and a second version call ahead of
// the login drops the connection without a reply.  So does a first one from
// the hand built 0x00, 0x05 and 0x0f clients, in both encodings, since none of
// them is an 8i client.
//
// Once logged in, ojdbc14 is played through query sessions at 12.1, 11.2, 10.2
// and the oracle_connectstrings listener.  Each sends ojdbc14's own calls, as
// it sent them to real 10.2 and 12.2 servers in the 10745-dev-ojdbc14-*-
// realserver captures: its 0x5e with the text raw behind 9 fixed fields, its
// fetch, commit and rollback, and its cursor closes riding in front of the
// next call, all with sequence number 0.  The builders have to reproduce 23
// of those calls byte for byte first.  The scripts are the captures' select1,
// rows25, twoqueries, reuse, prepared, error, longtext, types, dml and
// dmlauto.  Every answer is read the way ojdbc14 reads it - see
// readOjdbc14Answer() - so a length byte too long for its field, a ttc code
// ojdbc14 doesn't take there, or a byte left over fails the way the driver
// would close the connection.  Then each object is compared byte for byte:
// the describe's columns with their position and the scale in ojdbc14's
// form, the return parameters, the row header, the rows, the 23 field error
// object, and the 09 that answers a commit or a rollback.  The end to end
// field in each is a real server's, counting every call and every close from
// 4 at the first 0x5e.  The cursor ids, the dates, the describe's skipped
// block and its max row size are the module's, and aren't compared.
//
// Some of what a query session checks is the module's choice rather than a
// real server's, since no real server answers the module's way or no capture
// covers it: no key/value pairs in the return parameters, a NUMBER's size 22,
// and success iterations 1 in an answer with no error.  So are two scripts:
// newline, a text whose first byte is a newline; and null, select null, which
// the module describes as a VARCHAR2 of 4000 with a null value where a real
// server describes it 0 bytes wide and sends no value byte (selectnull
// captures), so the test pins the module's shape.  The prepared script's int
// comes back as the text "10" in a VARCHAR2, since the module binds it to the
// backend as text.  Commitfirst, a commit before any 0x5e, is a real
// server's: it answers "09 01 04" (commitfirst captures), as does the
// rollback in dmlauto, "09 01 09" (rollbackauto captures).
//
// Errors, fetch sizes and re-executes (#10750).  An error other than
// ORA-01403 comes the way a real server sends it (10750-dev-ojdbc14-*err-
// realserver captures): a break marker and a reset marker, the client's one
// reset marker back, then the error object alone, with success iterations 0,
// or 1 answering a re-execute with fetch (0x8068), and the message one line
// ending in a line feed.  An answer with no error has no markers.  The error
// object's layout is checked against four real objects first (see
// checkOjdbc14ErrorObjectBuilder()), and differs from a real one only in what
// the module can't know and ojdbc14 ignores: the error position, the array
// element fields, the rowid and a select error's row number (see
// putOjdbc14Summary()).  A summary's row number is the rows sent since the
// statement's last execute, as a real server's (2 in the prepared script's
// re-execute was 1 more than that).  A row header names the rows the call
// asked for as their low 16 bits, and a row number of 65536 and up takes 3
// bytes.  The scripts, at the same four listeners: execerr and prepexecerr,
// inserts failing with ORA-00001, ORA-01400, ORA-01722 and ORA-12899 at the
// execute, the re-execute and the re-parse; execselerr, fetcherr and
// prepfetcherr, selects whose rows fail to evaluate, at the 0x5e, at a fetch
// or in a re-execute with fetch; fetchsizes and prepfetchsizes, fetches of
// 65535, 65536, 70000, 100000 and 100001 rows (the last over the module's
// limit, so ORA-20004); and reexecrows, the row numbers of three executes.
// Then fetcherr at the fetchatonce=1 listener, where the error comes at a
// fetch after a row, and at 12.1 only: bigfetch, 65536 rows in one answer;
// noreset, a client that sends its next call where its reset belongs, which
// the listener answers by closing the connection; and urgent, a byte of tcp
// urgent data, ojdbc14's cancel, which the module doesn't read, right behind
// a call and between two.  Not asserted as a real server's, and so pinned as
// the module's shape: where a select's error comes at the default
// fetchatonce (the back end decides, see ojdbc14SelectError()), the rows a
// real server delivers ahead of a mid-fetch error and the module loses, and
// the cancel, which a real server answers with ORA-01013.
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
static const size_t	SESSKEY_HEX_SIZE_8I=16;
static const size_t	SESSKEY_HEX_SIZE_9I=32;
static const size_t	SESSKEY_HEX_SIZE_10G=64;
static const size_t	SESSKEY_HEX_SIZE_11G=96;
static const size_t	SESSKEY_HEX_SIZE_12C=64;
static const size_t	VFR_DATA_HEX_SIZE_11G=20;
static const size_t	VFR_DATA_HEX_SIZE_12C=32;
static const size_t	PBKDF2_CSK_SALT_HEX_SIZE=32;

// which verifier the module picks for a case
enum verifier {
	VERIFIER_8I,
	VERIFIER_9I,
	VERIFIER_10G,
	VERIFIER_11G,
	VERIFIER_12C
};

static const char	*verifiernames[]={"8i","9i","10g","11g","12c"};

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

// genuine ojdbc14 10.2.0.5's session
//
// the tns version it offers, which keeps the small packet header and the
// marker flags 0
static const uint16_t	ORA_CONNECT_VERSION_OJDBC14=0x0134;

// its data type request, behind the ttc code: the character sets (873, the
// server's, echoed), the encoding flags 0x01, the 29 byte compile
// capabilities - logon types 0x00 at 4, field version 3 at 7, CCAP_OCI1 0x90
// at 16 - the one runtime capability byte and the ub1 data type list.  packet
// [0009] of samples/10545-dev-ojdbc14-login-select1-realserver-
// oracle122.oraproxy, from the driver to a real 12.2 server
static const unsigned char	ORA_OJDBC14_DATA_TYPE_REQUEST[]={
	0x69, 0x03, 0x69, 0x03, 0x01, 0x1d, 0x06, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x28, 0x90, 0x03,
	0x07, 0x03, 0x00, 0x01, 0x00, 0x0f, 0x01, 0x00, 0x04, 0x00, 0x00, 0x01,
	0x02, 0x01, 0x01, 0x01, 0x00, 0x02, 0x02, 0x0a, 0x00, 0x08, 0x08, 0x01,
	0x00, 0x0c, 0x0c, 0x0a, 0x00, 0x64, 0x64, 0x01, 0x00, 0x15, 0x64, 0x01,
	0x00, 0x65, 0x65, 0x01, 0x00, 0x16, 0x65, 0x01, 0x00, 0x17, 0x17, 0x01,
	0x00, 0x18, 0x18, 0x01, 0x00, 0x19, 0x19, 0x01, 0x00, 0x1a, 0x1a, 0x01,
	0x00, 0x1b, 0x1b, 0x01, 0x00, 0x1c, 0x1c, 0x01, 0x00, 0x1d, 0x1d, 0x01,
	0x00, 0x1e, 0x1e, 0x01, 0x00, 0x1f, 0x1f, 0x01, 0x00, 0x20, 0x20, 0x01,
	0x00, 0x21, 0x21, 0x01, 0x00, 0x0a, 0x0a, 0x01, 0x00, 0x0b, 0x0b, 0x01,
	0x00, 0x22, 0x22, 0x01, 0x00, 0x23, 0x23, 0x01, 0x00, 0x24, 0x24, 0x01,
	0x00, 0x25, 0x25, 0x01, 0x00, 0x26, 0x26, 0x01, 0x00, 0x28, 0x28, 0x01,
	0x00, 0x29, 0x29, 0x01, 0x00, 0x2a, 0x2a, 0x01, 0x00, 0x2b, 0x2b, 0x01,
	0x00, 0x2c, 0x2c, 0x01, 0x00, 0x2d, 0x2d, 0x01, 0x00, 0x2e, 0x2e, 0x01,
	0x00, 0x2f, 0x2f, 0x01, 0x00, 0x30, 0x30, 0x01, 0x00, 0x31, 0x31, 0x01,
	0x00, 0x32, 0x32, 0x01, 0x00, 0x33, 0x33, 0x01, 0x00, 0x34, 0x34, 0x01,
	0x00, 0x35, 0x35, 0x01, 0x00, 0x36, 0x36, 0x01, 0x00, 0x37, 0x37, 0x01,
	0x00, 0x38, 0x38, 0x01, 0x00, 0x39, 0x39, 0x01, 0x00, 0x3a, 0x3a, 0x01,
	0x00, 0x3b, 0x3b, 0x01, 0x00, 0x3c, 0x3c, 0x01, 0x00, 0x3d, 0x3d, 0x01,
	0x00, 0x3e, 0x3e, 0x01, 0x00, 0x3f, 0x3f, 0x01, 0x00, 0x40, 0x40, 0x01,
	0x00, 0x41, 0x41, 0x01, 0x00, 0x42, 0x42, 0x01, 0x00, 0x43, 0x43, 0x01,
	0x00, 0x47, 0x47, 0x01, 0x00, 0x48, 0x48, 0x01, 0x00, 0x49, 0x49, 0x01,
	0x00, 0x4b, 0x4b, 0x01, 0x00, 0x4d, 0x4d, 0x01, 0x00, 0x4e, 0x4e, 0x01,
	0x00, 0x4f, 0x4f, 0x01, 0x00, 0x50, 0x50, 0x01, 0x00, 0x51, 0x51, 0x01,
	0x00, 0x52, 0x52, 0x01, 0x00, 0x53, 0x53, 0x01, 0x00, 0x54, 0x54, 0x01,
	0x00, 0x55, 0x55, 0x01, 0x00, 0x56, 0x56, 0x01, 0x00, 0x57, 0x57, 0x01,
	0x00, 0x59, 0x59, 0x01, 0x00, 0x5a, 0x5a, 0x01, 0x00, 0x5c, 0x5c, 0x01,
	0x00, 0x5d, 0x5d, 0x01, 0x00, 0x62, 0x62, 0x01, 0x00, 0x63, 0x63, 0x01,
	0x00, 0x67, 0x67, 0x01, 0x00, 0x6b, 0x6b, 0x01, 0x00, 0x75, 0x75, 0x01,
	0x00, 0x78, 0x78, 0x01, 0x00, 0x7c, 0x7c, 0x01, 0x00, 0x7d, 0x7d, 0x01,
	0x00, 0x7e, 0x7e, 0x01, 0x00, 0x7f, 0x7f, 0x01, 0x00, 0x80, 0x80, 0x01,
	0x00, 0x81, 0x81, 0x01, 0x00, 0x82, 0x82, 0x01, 0x00, 0x83, 0x83, 0x01,
	0x00, 0x84, 0x84, 0x01, 0x00, 0x85, 0x85, 0x01, 0x00, 0x86, 0x86, 0x01,
	0x00, 0x87, 0x87, 0x01, 0x00, 0x88, 0x88, 0x01, 0x00, 0x89, 0x89, 0x01,
	0x00, 0x8a, 0x8a, 0x01, 0x00, 0x8b, 0x8b, 0x01, 0x00, 0x8c, 0x8c, 0x01,
	0x00, 0x8d, 0x8d, 0x01, 0x00, 0x8e, 0x8e, 0x01, 0x00, 0x8f, 0x8f, 0x01,
	0x00, 0x90, 0x90, 0x01, 0x00, 0x91, 0x91, 0x01, 0x00, 0x94, 0x94, 0x01,
	0x00, 0x95, 0x95, 0x01, 0x00, 0x96, 0x96, 0x01, 0x00, 0x97, 0x97, 0x01,
	0x00, 0x99, 0x99, 0x01, 0x00, 0x9d, 0x9d, 0x01, 0x00, 0x9e, 0x9e, 0x01,
	0x00, 0x9f, 0x9f, 0x01, 0x00, 0xa0, 0xa0, 0x01, 0x00, 0xa1, 0xa1, 0x01,
	0x00, 0xa2, 0xa2, 0x01, 0x00, 0xa3, 0xa3, 0x01, 0x00, 0xa4, 0xa4, 0x01,
	0x00, 0xa5, 0xa5, 0x01, 0x00, 0xa6, 0xa6, 0x01, 0x00, 0xa7, 0xa7, 0x01,
	0x00, 0xa8, 0xa8, 0x01, 0x00, 0xa9, 0xa9, 0x01, 0x00, 0xaa, 0xaa, 0x01,
	0x00, 0xab, 0xab, 0x01, 0x00, 0xad, 0xad, 0x01, 0x00, 0xae, 0xae, 0x01,
	0x00, 0xaf, 0xaf, 0x01, 0x00, 0xb0, 0xb0, 0x01, 0x00, 0xb1, 0xb1, 0x01,
	0x00, 0xb4, 0xb4, 0x01, 0x00, 0xb5, 0xb5, 0x01, 0x00, 0xb6, 0xb6, 0x01,
	0x00, 0xb7, 0xb7, 0x01, 0x00, 0xd0, 0xd0, 0x01, 0x00, 0xe7, 0xe7, 0x01,
	0x00, 0xe9, 0xe9, 0x01, 0x00, 0x03, 0x02, 0x0a, 0x00, 0x04, 0x02, 0x0a,
	0x00, 0x05, 0x01, 0x01, 0x00, 0x06, 0x02, 0x0a, 0x00, 0x07, 0x02, 0x0a,
	0x00, 0x09, 0x01, 0x01, 0x00, 0x0d, 0x00, 0x0e, 0x00, 0x0f, 0x17, 0x01,
	0x00, 0x10, 0x00, 0x11, 0x00, 0x12, 0x00, 0x13, 0x00, 0x14, 0x00, 0x15,
	0x00, 0x16, 0x00, 0x27, 0x78, 0x01, 0x5d, 0x01, 0x26, 0x01, 0x00, 0x3a,
	0x00, 0x44, 0x02, 0x0a, 0x00, 0x45, 0x00, 0x46, 0x00, 0x4a, 0x00, 0x4c,
	0x00, 0x58, 0x00, 0x5b, 0x02, 0x0a, 0x00, 0x5e, 0x01, 0x01, 0x00, 0x5f,
	0x17, 0x01, 0x00, 0x60, 0x60, 0x01, 0x00, 0x61, 0x60, 0x01, 0x00, 0x66,
	0x66, 0x01, 0x00, 0x68, 0x00, 0x69, 0x00, 0x6a, 0x6a, 0x01, 0x00, 0x6b,
	0x00, 0x6c, 0x6d, 0x01, 0x00, 0x6d, 0x6d, 0x01, 0x00, 0x6e, 0x6f, 0x01,
	0x00, 0x6f, 0x6f, 0x01, 0x00, 0x70, 0x70, 0x01, 0x00, 0x71, 0x71, 0x01,
	0x00, 0x72, 0x72, 0x01, 0x00, 0x73, 0x73, 0x01, 0x00, 0x74, 0x66, 0x01,
	0x00, 0x76, 0x00, 0x77, 0x00, 0x79, 0x00, 0x7a, 0x00, 0x7b, 0x00, 0x88,
	0x00, 0x8c, 0x00, 0x92, 0x92, 0x01, 0x00, 0x93, 0x00, 0x98, 0x02, 0x0a,
	0x00, 0x99, 0x02, 0x0a, 0x00, 0x9a, 0x02, 0x0a, 0x00, 0x9b, 0x01, 0x01,
	0x00, 0x9c, 0x0c, 0x0a, 0x00, 0xac, 0x02, 0x0a, 0x00, 0xae, 0x00, 0xc2,
	0xc2, 0x01, 0x00, 0xd3, 0xd3, 0x01, 0x00, 0xed, 0xed, 0x01, 0x00, 0x00
};

// the challenge a real server sends it, 65 bytes with the data flags: the
// ttc code, a pair count of 1, AUTH_SESSKEY with a 16 hex character value
// and flags 0, and then a 25 byte trailer - the ttc code, the end to end
// field 2 and 22 zero bytes.  packet [0014] of the same capture, and of the
// 10545 oracle102 one and the 10734 ones
static const unsigned char	ORA_OJDBC14_CHALLENGE_HEAD[]={
	0x00, 0x00, 0x08, 0x01, 0x01, 0x01, 0x0c, 0x0c, 0x41, 0x55, 0x54, 0x48,
	0x5f, 0x53, 0x45, 0x53, 0x53, 0x4b, 0x45, 0x59, 0x01, 0x10, 0x10
};
static const size_t	ORA_OJDBC14_CHALLENGE_SIZE=65;

// the field version 3 trailer and error object, counted from the ttc code.
// the trailer is the ttc code, the end to end field (the login phase, as a
// count prefixed int) and 22 zero bytes, the call number among them.  the
// error object is the ttc code, the end to end field 0, the rows processed
// 0, the ora number, four zero fields, the statement type as a raw byte, 12
// zero bytes, the call number, 2 zero fields, then the message behind a one
// byte length - no end of call status, and two zero fields behind the call
// number where the module's other objects have six
static const size_t	ORA_OJDBC14_TRAILER_ZEROS=22;
static const size_t	ORA_OJDBC14_ERROR_ZEROS_AFTER_CALL_NUMBER=2;

// what a real server answers its logoff with: the data flags, TTC_STATUS
// and the end to end field 0.  packet [0022] of the 10545 captures
static const unsigned char	ORA_OJDBC14_LOGOFF_ANSWER[]={
	0x00, 0x00, 0x09, 0x00
};

// how many pairs the module's login answer carries for the 8i verifier: its
// usual list without AUTH_SVR_RESPONSE
static const uint32_t	ORA_OJDBC14_LOGIN_ANSWER_PAIRS=9;

// the AUTH_PASSWORD of five real ojdbc14 logins, against the AUTH_SESSKEY the
// server sent each one.  the unknown user's are the client's own math, which
// is the same whether or not the user exists.  they cover a pad digit of 4,
// 0 with one and two blocks and no extra block, and 3
struct ojdbc14vector {
	const char	*user;
	const char	*password;
	const char	*sesskey;
	const char	*authpassword;
	const char	*capture;
};

static const ojdbc14vector	ojdbc14vectors[]={
	{"TESTUSER","testpassword","7F1C9166D3C75F94",
		"420EAA8FD84DC53B31CF894F97928FC54",
		"10545-dev-ojdbc14-login-select1-realserver-oracle102"},
	{"TESTUSER","testpassword","6F378FABDED396C7",
		"13B553AF3CD25E95FECFD5B88123E2DD4",
		"10545-dev-ojdbc14-login-select1-realserver-oracle122"},
	{"NOSUCHUSER","whatever","0142255CC841D42C",
		"95C1B773956A9EAB0",
		"10734-dev-ojdbc14-unknownuser-realserver-oracle102"},
	{"NOSUCHUSER","abcdefghijklmnop","61577F68D99CA581",
		"1A8A5328A6A01429B8663A6E8D0F134F0",
		"10734-dev-ojdbc14-unknownuser-pw16-realserver-oracle122"},
	{"DEV","wrongpassword","5F5CAB7B5DC54228",
		"C8BD3126CDBE7A3A81CBF22B9636C8DC3",
		"10734-dev-ojdbc14-wrongpassword-realserver-oracle102"}
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
		ORA_CCAP_SIZE_10_2,"169869568"},
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
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 oracle_connectstrings","ORACLEPROTOCOLPORT4",1524,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 fetchatonce=1","ORACLEPROTOCOLPORT5",1525,
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
	LISTENER_FLOOR_12,
	LISTENER_CONNECTSTRINGS,
	LISTENER_FETCHATONCE
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

	// at 12.1, each client gets its own verifier.  the 0x00 client gets the
	// 8i one at every ceiling, as ojdbc14 does from a real server
	{LISTENER_12_1,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_8I,OUTCOME_CHALLENGE},
	{LISTENER_12_1,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_12_1,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_12_1,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_12_1,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_12_1,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// at 11.2, nothing newer than 11g
	{LISTENER_11_2,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_8I,OUTCOME_CHALLENGE},
	{LISTENER_11_2,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_11_2,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_11_2,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_11G,OUTCOME_CHALLENGE},
	{LISTENER_11_2,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_11_2,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// at 10.2, nothing newer than 10g
	{LISTENER_10_2,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_8I,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_10G,OUTCOME_CHALLENGE},
	{LISTENER_10_2,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_10_2,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// a floor of 9 refuses only a client that advertises nothing.  the
	// real server did the same to genuine ojdbc14 and let a genuine 9i
	// client and the 10.2 and OCI 23 clients in.  the 11.2 client has all
	// the bits a floor of 9 could look at, so it is in whichever it is
	{LISTENER_FLOOR_9,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_8I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_9,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_9,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_FLOOR_9,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// a floor of 11 refuses a client without O5LOGON, which is the same
	// as a floor of 10, and it admits the 10g verifier and a classic login
	{LISTENER_FLOOR_11,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_8I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_11,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED},
	{LISTENER_FLOOR_11,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_FLOOR_11,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_11,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_ADMITTED},
	{LISTENER_FLOOR_11,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_CHALLENGE},

	// a floor of 12 refuses a client without O5LOGON_NP.  the 11.2 client
	// and OCI 23 log in with a tagged call, and answer a classic one with
	// a challenge and then ORA-01017
	{LISTENER_FLOOR_12,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_8I,OUTCOME_REFUSED_BASIC},
	{LISTENER_FLOOR_12,LOGON_TYPES_9I,FORM_TAGGED,VERIFIER_9I,OUTCOME_REFUSED},
	{LISTENER_FLOOR_12,LOGON_TYPES_11G,FORM_TAGGED,VERIFIER_11G,OUTCOME_LOGIN},
	{LISTENER_FLOOR_12,LOGON_TYPES_12C,FORM_TAGGED,VERIFIER_12C,OUTCOME_CHALLENGE},
	{LISTENER_FLOOR_12,LOGON_TYPES_11G,FORM_CLASSIC,VERIFIER_10G,OUTCOME_BARRED},
	{LISTENER_FLOOR_12,LOGON_TYPES_12C,FORM_CLASSIC,VERIFIER_10G,OUTCOME_BARRED},

	// a floor of 12a refuses a client without O7LOGON, a tagged one that
	// doesn't get the 12c verifier, and answers OCI 23's classic login the
	// way a floor of 12 does
	{LISTENER_FLOOR_12A,LOGON_TYPES_NONE,FORM_TAGGED,VERIFIER_8I,OUTCOME_REFUSED_BASIC},
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

// what an ojdbc14 case expects of the listener
enum ojdbc14outcome {

	// the 8i challenge, the login, and the logoff
	OJDBC14_LOGIN,

	// the 8i challenge, and then ORA-01017 for phase two
	OJDBC14_DENIED,

	// ORA-28040 behind a break and a reset, in place of the challenge
	OJDBC14_REFUSED
};

// the AUTH_PASSWORD an ojdbc14 case sends: the one computed from the
// password, or that one malformed.  ojdbc14 itself only ever sends the
// computed one, so each malformed form is the module's choice to refuse
enum ojdbc14authpassword {
	AUTH_PASSWORD_COMPUTED,

	// the pad digit replaced with 8, out of the 0-7 range
	AUTH_PASSWORD_PAD_DIGIT_8,

	// the pad digit one less than it should be, so the padding still
	// decrypts to zeros and only the length is wrong
	AUTH_PASSWORD_WRONG_PAD_DIGIT,

	// the hex alone, with no pad digit behind it
	AUTH_PASSWORD_NO_PAD_DIGIT,

	// a hex character replaced with a G
	AUTH_PASSWORD_NON_HEX,

	// the hex two characters short, an odd length that isn't a whole
	// number of blocks and a pad digit
	AUTH_PASSWORD_SHORT
};

static const char	*ojdbc14authpasswordnames[]={
	"computed",
	"pad digit 8",
	"pad digit one short",
	"no pad digit",
	"a non-hex character",
	"two hex characters short"
};

struct ojdbc14case {
	size_t			listener;
	const char		*user;
	const char		*password;
	ojdbc14authpassword	authpassword;
	ojdbc14outcome		result;
};

// genuine ojdbc14, in the session described at the top.  it logs in at every
// ceiling, as it does to a real 10.2, 11.2 and 12.2 server.  a wrong password
// of 13 characters, pad digit 3, is denied at each ceiling, and an unknown
// user and a wrong password of 16 characters, pad digit 0 and two blocks, at
// 12.1.  every floor of 9 and up refuses it in phase one, as a real 12.2
// server did at 9.  the user name is ojdbc14's, uppercase, except against the
// oracle_connectstrings auth module, whose connect string has it in lowercase.
//
// the malformed AUTH_PASSWORD cases send the right password, encrypted under
// the right session key, and then spoil the value, so any of them that got
// past the auth module's format checks would log in.  each is denied with the
// same ORA-01017 a wrong password gets.  no real server was measured with
// them, so that object is the module's choice
static const ojdbc14case	ojdbc14cases[]={
	{LISTENER_12_1,"TESTUSER","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_LOGIN},
	{LISTENER_11_2,"TESTUSER","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_LOGIN},
	{LISTENER_10_2,"TESTUSER","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_LOGIN},
	{LISTENER_CONNECTSTRINGS,"testuser","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_LOGIN},
	{LISTENER_12_1,"TESTUSER","wrongpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_DENIED},
	{LISTENER_11_2,"TESTUSER","wrongpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_DENIED},
	{LISTENER_10_2,"TESTUSER","wrongpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_DENIED},
	{LISTENER_12_1,"NOSUCHUSER","whatever",
		AUTH_PASSWORD_COMPUTED,OJDBC14_DENIED},
	{LISTENER_12_1,"TESTUSER","abcdefghijklmnop",
		AUTH_PASSWORD_COMPUTED,OJDBC14_DENIED},
	{LISTENER_12_1,"TESTUSER","testpassword",
		AUTH_PASSWORD_PAD_DIGIT_8,OJDBC14_DENIED},
	{LISTENER_12_1,"TESTUSER","testpassword",
		AUTH_PASSWORD_WRONG_PAD_DIGIT,OJDBC14_DENIED},
	{LISTENER_12_1,"TESTUSER","testpassword",
		AUTH_PASSWORD_NO_PAD_DIGIT,OJDBC14_DENIED},
	{LISTENER_12_1,"TESTUSER","testpassword",
		AUTH_PASSWORD_NON_HEX,OJDBC14_DENIED},
	{LISTENER_12_1,"TESTUSER","testpassword",
		AUTH_PASSWORD_SHORT,OJDBC14_DENIED},
	{LISTENER_CONNECTSTRINGS,"testuser","testpassword",
		AUTH_PASSWORD_WRONG_PAD_DIGIT,OJDBC14_DENIED},
	{LISTENER_FLOOR_9,"TESTUSER","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_REFUSED},
	{LISTENER_FLOOR_11,"TESTUSER","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_REFUSED},
	{LISTENER_FLOOR_12,"TESTUSER","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_REFUSED},
	{LISTENER_FLOOR_12A,"TESTUSER","testpassword",
		AUTH_PASSWORD_COMPUTED,OJDBC14_REFUSED}
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
	bool		des8i=(expected==VERIFIER_8I);
	bool		des10g=(expected==VERIFIER_10G);
	uint32_t	expectedcount=(o3logon || des8i)?1:((des10g)?2:
				((expected==VERIFIER_12C)?6:3));
	uint32_t	count=0;
	l.clear();
	l.append(label)->append(" pair count is ")->append(expectedcount);
	report(l.getString(),client->getAuthFieldCount(&count) &&
						count==expectedcount);

	size_t	sesskeysize=0;
	switch (expected) {
		case VERIFIER_8I:
			sesskeysize=SESSKEY_HEX_SIZE_8I;
			break;
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

	if (o3logon || des8i) {

		// the tagged o3logon and 8i shape: AUTH_SESSKEY alone, and
		// nothing that carries a verifier type
		l.clear();
		l.append(label)->append(" has no AUTH_VFR_DATA");
		report(l.getString(),!hasField(client,"AUTH_VFR_DATA"));

		if (des8i) {
			l.clear();
			l.append(label)->append(
				" has no AUTH_GLOBALLY_UNIQUE_DBID");
			report(l.getString(),
				!hasField(client,"AUTH_GLOBALLY_UNIQUE_DBID"));
		}

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
	bool			des8i=(tc->expected==VERIFIER_8I);
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

	// the classic call is replayed in portable form, and a 9i or 8i
	// verifier login is portable only
	if (nativeencoding && (o3logon || des8i || classic)) {
		stdoutput.printf("%s: skipped, %s is portable only\n",label,
					(classic)?"a classic login":
					((des8i)?"the 8i exchange":"o3logon"));
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

// compute8iAuthPassword() against real logins, before any connection: each
// vector's AUTH_SESSKEY has to give exactly the AUTH_PASSWORD the real
// client sent
static void checkKnownAnswers() {

	stdoutput.printf("\n8i AUTH_PASSWORD known answers - expect each "
						"real client's value\n");

	for (size_t i=0; i<sizeof(ojdbc14vectors)/sizeof(ojdbc14vectors[0]);
									i++) {

		const ojdbc14vector	*v=&ojdbc14vectors[i];

		char	*authpassword=NULL;
		bool	ok=(oracleprotocolclient::compute8iAuthPassword(
						v->user,v->password,
						v->sesskey,&authpassword) &&
				!charstring::compare(authpassword,
							v->authpassword));

		char	label[256];
		charstring::printf(label,sizeof(label),
				"8i AUTH_PASSWORD of %s, password of %d "
				"characters, is %s's",v->user,
				(int)charstring::getLength(v->password),
				v->capture);
		report(label,ok);
		if (!ok) {
			stdoutput.printf("    got %s, expected %s\n",
					(authpassword)?authpassword:"(null)",
					v->authpassword);
		}
		delete[] authpassword;
	}
}

// the banner the module sends at a ceiling, built from its AUTH_VERSION_NO
// the way the module builds it: major, minor, third, patch and fifth
// component in bits 31-24, 23-20, 15-12, 11-8 and 7-0
static void buildBanner(char *banner, size_t size, uint32_t packed) {
	int	major=(int)((packed>>24)&0xff);
	charstring::printf(banner,size,
			"Oracle Database %d%s Enterprise Edition "
			"Release %d.%d.%d.%d.%d - 64bit Production",
			major,(major>=12)?"c":"g",major,
			(int)((packed>>20)&0x0f),(int)((packed>>12)&0x0f),
			(int)((packed>>8)&0x0f),(int)(packed&0xff));
}

// ojdbc14's version call, and the answer byte for byte: the ttc code, the
// banner's size and the banner raw, the packed version, then "09 00" and the
// end of the packet, as packet [0012] of the 10545 ojdbc14 captures has it.
// the banner and the packed version are the module's own at the ceiling.
// returns whether it was answered at all
static bool checkOjdbc14Version(oracleprotocolclient *client,
					const char *label,
					const listenerinfo *li) {

	stringbuffer	l;
	l.append(label)->append(" version call is answered");
	bool	ok=client->versionCall();
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return false;
	}

	uint32_t	packed=(uint32_t)
			charstring::convertToUnsignedInteger(li->versionno);
	char		banner[256];
	buildBanner(banner,sizeof(banner),packed);
	size_t		bannersize=charstring::getLength(banner);

	char		*actualbanner=NULL;
	uint32_t	actualpacked=0;
	uint32_t	endtoend=0;
	l.clear();
	l.append(label)->append(" version answer ends in its end to end field");
	bool	shape=client->readVersionResponse(&actualbanner,
						&actualpacked,&endtoend);
	report(l.getString(),shape);

	l.clear();
	l.append(label)->append(" banner is \"")->append(banner)->append("\"");
	report(l.getString(),
		shape && !charstring::compare(actualbanner,banner));

	l.clear();
	l.append(label)->append(" packed version is ")->append(li->versionno);
	report(l.getString(),shape && actualpacked==packed);

	l.clear();
	l.append(label)->append(" end to end field is 0");
	report(l.getString(),shape && !endtoend);

	unsigned char	expected[3+5+256+5+2];
	size_t		expectedsize=0;
	putZeros(expected,&expectedsize,2);
	putByte(expected,&expectedsize,ORA_TTC_OK);
	putLenPreInt(expected,&expectedsize,(uint32_t)bannersize);
	bytestring::copy(expected+expectedsize,banner,bannersize);
	expectedsize+=bannersize;
	putLenPreInt(expected,&expectedsize,packed);
	putByte(expected,&expectedsize,0x09);
	putZeros(expected,&expectedsize,1);

	l.clear();
	l.append(label)->append(" version answer is exact");
	bool	exact=(client->getResponseSize()==expectedsize &&
			!bytestring::compare(client->getResponse(),
						expected,expectedsize));
	report(l.getString(),exact);
	if (!shape || !exact) {
		if (actualbanner) {
			stdoutput.printf("    banner: %s\n",actualbanner);
		}
		dumpBytes("answer",client->getResponse(),
					client->getResponseSize());
		dumpBytes("expected",expected,expectedsize);
	}

	delete[] actualbanner;
	return true;
}

// the challenge, byte for byte, as a real server sends it to ojdbc14: 65
// bytes, AUTH_SESSKEY alone with 16 uppercase hex characters, and the 25
// byte trailer with the end to end field 2.  returns whether it was one
static bool checkOjdbc14Challenge(oracleprotocolclient *client,
						const char *label) {

	stringbuffer	l;
	l.append(label)->append(" got a challenge");
	bool	ok=(client->getResponseTtcCode()==ORA_TTC_OK &&
						!client->gotBreak());
	report(l.getString(),ok);
	if (!ok) {
		return false;
	}

	const unsigned char	*response=client->getResponse();
	size_t			responsesize=client->getResponseSize();
	size_t			keyoffset=sizeof(ORA_OJDBC14_CHALLENGE_HEAD);

	bool	keyok=(responsesize>=keyoffset+SESSKEY_HEX_SIZE_8I);
	for (size_t i=0; keyok && i<SESSKEY_HEX_SIZE_8I; i++) {
		keyok=isHexCharacter(response[keyoffset+i]);
	}
	l.clear();
	l.append(label)->append(" AUTH_SESSKEY is ");
	l.append((uint64_t)SESSKEY_HEX_SIZE_8I);
	l.append(" uppercase hex characters");
	report(l.getString(),keyok);

	// the head, the key as sent, its flags, then the trailer
	unsigned char	expected[ORA_OJDBC14_CHALLENGE_SIZE];
	size_t		expectedsize=0;
	bytestring::copy(expected,ORA_OJDBC14_CHALLENGE_HEAD,keyoffset);
	expectedsize=keyoffset;
	if (keyok) {
		bytestring::copy(expected+expectedsize,response+keyoffset,
							SESSKEY_HEX_SIZE_8I);
	} else {
		bytestring::set(expected+expectedsize,'0',SESSKEY_HEX_SIZE_8I);
	}
	expectedsize+=SESSKEY_HEX_SIZE_8I;
	putZeros(expected,&expectedsize,1);
	putByte(expected,&expectedsize,ORA_TTC_ERROR);
	putLenPreInt(expected,&expectedsize,ORA_AUTH_PHASE_CHALLENGE);
	putZeros(expected,&expectedsize,ORA_OJDBC14_TRAILER_ZEROS);

	l.clear();
	l.append(label)->append(" challenge is exact, ");
	l.append((uint64_t)ORA_OJDBC14_CHALLENGE_SIZE)->append(" bytes");
	bool	exact=(responsesize==expectedsize &&
			!bytestring::compare(response,expected,expectedsize));
	report(l.getString(),exact);
	if (!exact) {
		dumpBytes("challenge",response,responsesize);
		dumpBytes("expected",expected,expectedsize);
	}
	return true;
}

// a field version 3 error object, byte for byte, behind the ttc code.  the
// call number is 0, ojdbc14's sequence number, and the statement type 2 in a
// phase two error from an emulated 12.1 server and 0 in every other.  the
// expected bytes are the real servers' objects:
//
//	- ORA-01017, 12.1 emulated: packet [0019] of samples/10734-dev-ojdbc14-
//	  unknownuser-realserver-oracle122.oraproxy and of the -pw16 one beside
//	  it, from a 12.2 server
//	- ORA-01017, 11.2 and 10.2 emulated: packet [0019] of samples/10734-dev-
//	  ojdbc14-unknownuser-realserver-oracle112.oraproxy and of samples/
//	  10734-dev-ojdbc14-wrongpassword-realserver-oracle102.oraproxy
//	- ORA-28040: packet [0017] of samples/10545-dev-ojdbc14-login-select1-
//	  realserver-oracle122-allowed9.oraproxy, at a floor of 9.  that floors
//	  11, 12 and 12a send the same object is inferred
static void checkOjdbc14Error(oracleprotocolclient *client,
					const char *label,
					const listenerinfo *li,
					uint32_t oranum,
					const char *message,
					bool secondphase) {

	stringbuffer	l;
	l.append(label)->append(" got a break and a reset first");
	report(l.getString(),client->gotBreak());
	if (client->gotBreak()) {
		checkMarkerFlags(client,label);
	}

	bool		server121=(li->fieldversion>=
					ORA_CCAP_FIELD_VERSION_12_1);
	size_t		messagesize=charstring::getLength(message);
	unsigned char	statementtype=(secondphase && server121)?
				ORA_NATIVE_ERROR_STATEMENT_TYPE_PHASE_TWO:0;

	unsigned char	expected[64+255];
	size_t		expectedsize=0;
	putZeros(expected,&expectedsize,2);
	putLenPreInt(expected,&expectedsize,oranum);
	putZeros(expected,&expectedsize,
			ORA_PORTABLE_ERROR_ZEROS_BEFORE_STATEMENT_TYPE);
	putByte(expected,&expectedsize,statementtype);
	putZeros(expected,&expectedsize,
			ORA_PORTABLE_ERROR_ZEROS_BEFORE_CALL_NUMBER);
	putZeros(expected,&expectedsize,1);
	putZeros(expected,&expectedsize,
			ORA_OJDBC14_ERROR_ZEROS_AFTER_CALL_NUMBER);
	putByte(expected,&expectedsize,(unsigned char)messagesize);
	bytestring::copy(expected+expectedsize,message,messagesize);
	expectedsize+=messagesize;

	// behind the data flags and the ttc code
	const size_t		prefix=3;
	size_t			responsesize=client->getResponseSize();
	const unsigned char	*object=client->getResponse()+prefix;
	size_t			objectsize=(responsesize>prefix)?
						responsesize-prefix:0;

	char	oralabel[16];
	charstring::printf(oralabel,sizeof(oralabel),"ORA-%05d",oranum);
	l.clear();
	l.append(label)->append(" got ")->append(oralabel);
	report(l.getString(),
		client->getResponseTtcCode()==ORA_TTC_ERROR &&
		client->responseContains(message));

	l.clear();
	l.append(label)->append(" field version 3 error is exact, ");
	l.append((uint64_t)(expectedsize+1))->append(" bytes");
	bool	exact=(client->getResponseTtcCode()==ORA_TTC_ERROR &&
			objectsize==expectedsize &&
			!bytestring::compare(object,expected,expectedsize));
	report(l.getString(),exact);
	if (!exact) {
		dumpBytes("error",object,objectsize);
		dumpBytes("expected",expected,expectedsize);
	}
}

// the login answer: the module's pairs, and the 25 byte trailer with the end
// to end field 3 that a real server sends ojdbc14
static void checkOjdbc14LoginAnswer(oracleprotocolclient *client,
						const char *label,
						const listenerinfo *li) {

	stringbuffer	l;
	uint32_t	count=0;
	l.append(label)->append(" pair count is ");
	l.append(ORA_OJDBC14_LOGIN_ANSWER_PAIRS);
	report(l.getString(),client->getAuthFieldCount(&count) &&
				count==ORA_OJDBC14_LOGIN_ANSWER_PAIRS);

	l.clear();
	l.append(label)->append(" has AUTH_SESSION_ID");
	report(l.getString(),hasField(client,"AUTH_SESSION_ID"));

	l.clear();
	l.append(label)->append(" has no AUTH_SVR_RESPONSE");
	report(l.getString(),!hasField(client,"AUTH_SVR_RESPONSE"));

	l.clear();
	l.append(label)->append(" AUTH_VERSION_NO is ")->append(li->versionno);
	report(l.getString(),
		checkField(client,"AUTH_VERSION_NO",0,li->versionno,0));

	unsigned char	expected[1+2+ORA_OJDBC14_TRAILER_ZEROS];
	size_t		expectedsize=0;
	putByte(expected,&expectedsize,ORA_TTC_ERROR);
	putLenPreInt(expected,&expectedsize,ORA_AUTH_PHASE_ANSWER);
	putZeros(expected,&expectedsize,ORA_OJDBC14_TRAILER_ZEROS);

	const unsigned char	*trailer=NULL;
	size_t			size=0;
	l.clear();
	l.append(label)->append(" login answer trailer is exact, ");
	l.append((uint64_t)expectedsize)->append(" bytes");
	bool	exact=(client->getAuthTrailer(&trailer,&size) &&
			size==expectedsize &&
			!bytestring::compare(trailer,expected,expectedsize));
	report(l.getString(),exact);
	if (!exact && trailer) {
		dumpBytes("trailer",trailer,size);
		dumpBytes("expected",expected,expectedsize);
	}
}

// the AUTH_PASSWORD computed against the challenge in the client's response,
// then spoiled the way the case says.  "authpassword" is the caller's to
// delete
static bool malform8iAuthPassword(oracleprotocolclient *client,
						const ojdbc14case *oc,
						char **authpassword) {

	*authpassword=NULL;

	char	*sesskeyhex=NULL;
	if (!client->getAuthField("AUTH_SESSKEY",&sesskeyhex,NULL)) {
		return false;
	}
	char	*computed=NULL;
	bool	ok=oracleprotocolclient::compute8iAuthPassword(
						oc->user,oc->password,
						sesskeyhex,&computed);
	delete[] sesskeyhex;
	if (!ok) {
		return false;
	}

	size_t	size=charstring::getLength(computed);
	char	*paddigit=computed+size-1;
	char	*nonhex=NULL;
	switch (oc->authpassword) {
		case AUTH_PASSWORD_PAD_DIGIT_8:
			*paddigit='8';
			break;
		case AUTH_PASSWORD_WRONG_PAD_DIGIT:
			if (*paddigit=='0') {
				delete[] computed;
				return false;
			}
			(*paddigit)--;
			break;
		case AUTH_PASSWORD_NO_PAD_DIGIT:
			*paddigit='\0';
			break;
		case AUTH_PASSWORD_NON_HEX:
			// a 0 where there is one, since a hex decoder that
			// reads junk as 0 would then get the right key
			nonhex=charstring::findFirst(computed,'0');
			if (!nonhex || nonhex==paddigit) {
				nonhex=computed;
			}
			*nonhex='G';
			break;
		case AUTH_PASSWORD_SHORT:
			bytestring::copy(paddigit-2,paddigit,2);
			break;
		default:
			break;
	}
	*authpassword=computed;
	return true;
}

// ojdbc14's session up to the challenge: the connect and the ano exchange,
// the version call and phase one, each checked.  false, reported, if one of
// them failed
static bool requestOjdbc14Challenge(oracleprotocolclient *client,
						const char *label,
						const listenerinfo *li,
						const char *user) {

	client->setConnectVersion(clientconnectversion);
	client->setDataTypeRequest(ORA_OJDBC14_DATA_TYPE_REQUEST,
					sizeof(ORA_OJDBC14_DATA_TYPE_REQUEST));
	client->setAnoNegotiation(true);

	stringbuffer	l;
	l.append(label)->append(" connect");
	if (!client->connect(host,getPort(li),sid)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client->getError());
		return false;
	}
	checkAnoResponseFlags(client,label);

	// the version call, ahead of the login at every floor
	if (!checkOjdbc14Version(client,label,li)) {
		return false;
	}

	l.clear();
	l.append(label)->append(" phase one");
	if (!client->requestChallenge8i(user)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client->getError());
		return false;
	}
	return true;
}

static void runOjdbc14Case(const ojdbc14case *oc) {

	const listenerinfo	*li=&listeners[oc->listener];

	char	label[256];
	charstring::printf(label,sizeof(label),
			"%s, ojdbc14, user %s, password of %d characters, "
			"tns version 0x%04x%s%s",li->name,oc->user,
			(int)charstring::getLength(oc->password),
			negotiatedconnectversion,
			(oc->authpassword!=AUTH_PASSWORD_COMPUTED)?
					", AUTH_PASSWORD with ":"",
			(oc->authpassword!=AUTH_PASSWORD_COMPUTED)?
				ojdbc14authpasswordnames[oc->authpassword]:"");
	const char	*expectation="the 8i challenge, then a login";
	if (oc->result==OJDBC14_DENIED) {
		expectation="the 8i challenge, then ORA-01017";
	} else if (oc->result==OJDBC14_REFUSED) {
		expectation="ORA-28040";
	}
	stdoutput.printf("\n%s - expect %s\n",label,expectation);

	oracleprotocolclient	client;
	if (!requestOjdbc14Challenge(&client,label,li,oc->user)) {
		return;
	}

	stringbuffer	l;
	if (oc->result==OJDBC14_REFUSED) {
		checkOjdbc14Error(&client,label,li,
				ORA_NO_MATCHING_AUTH_PROTOCOL,
				"ORA-28040: No matching authentication "
				"protocol\n",false);
		client.disconnect();
		return;
	}

	if (!checkOjdbc14Challenge(&client,label)) {
		client.disconnect();
		return;
	}

	char	*authpassword=NULL;
	if (oc->authpassword!=AUTH_PASSWORD_COMPUTED &&
		!malform8iAuthPassword(&client,oc,&authpassword)) {
		l.clear();
		l.append(label)->append(" AUTH_PASSWORD is computed");
		report(l.getString(),false);
		client.disconnect();
		return;
	}
	bool	loggedin=client.answerChallenge8i(oc->user,oc->password,
								authpassword);
	delete[] authpassword;
	if (oc->result==OJDBC14_DENIED) {
		l.clear();
		l.append(label)->append(" phase two is refused");
		report(l.getString(),!loggedin);
		if (!loggedin) {
			checkOjdbc14Error(&client,label,li,
					ORA_INVALID_USERNAME_PASSWORD,
					"ORA-01017: invalid username/password; "
					"logon denied\n",true);
		}
		client.disconnect();
		return;
	}

	l.clear();
	l.append(label)->append(" logs in");
	report(l.getString(),loggedin);
	if (!loggedin) {
		stdoutput.printf("    %s\n",client.getError());
		client.disconnect();
		return;
	}
	checkOjdbc14LoginAnswer(&client,label,li);

	// the logoff, and the "09 00" that answers it
	l.clear();
	l.append(label)->append(" logoff is answered with \"09 00\"");
	bool	ok=(client.logoff8i() &&
			client.getResponseSize()==
					sizeof(ORA_OJDBC14_LOGOFF_ANSWER) &&
			!bytestring::compare(client.getResponse(),
					ORA_OJDBC14_LOGOFF_ANSWER,
					sizeof(ORA_OJDBC14_LOGOFF_ANSWER)));
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client.getError());
		dumpBytes("answer",client.getResponse(),
					client.getResponseSize());
	}
}

// how long a dropped connection may take to show, well under the client's
// own read timeout, so a listener that holds the connection open without
// answering doesn't pass for one that dropped it
static const time_t	DROP_SECONDS=10;

// a version call that the listener should drop the connection over, without
// a reply.  the socket is left for the client's destructor, since a
// disconnect call would write to a closed connection
static void checkVersionCallDropped(oracleprotocolclient *client,
							const char *label) {
	datetime	dt;
	dt.initFromSystemDateTime();
	time_t		start=dt.getEpoch();
	bool		answered=client->versionCall();
	dt.initFromSystemDateTime();
	time_t		elapsed=dt.getEpoch()-start;

	stringbuffer	l;
	l.append(label)->append(" connection is dropped without a reply");
	report(l.getString(),!answered && elapsed<DROP_SECONDS);
	if (answered) {
		dumpBytes("answer",client->getResponse(),
					client->getResponseSize());
	} else if (elapsed>=DROP_SECONDS) {
		stdoutput.printf("    no reply, but no drop for %d seconds\n",
								(int)elapsed);
	}
}

// a version call ahead of the login from a client that isn't an 8i one: the
// hand built client, at 12.1, advertising "logontypes" and the pass's field
// version.  only an 8i client's version call is answered there, so this one
// ends the login.  a module without the 8i exchange dropped ojdbc14's the
// same way (MEASURED, samples/10734-dev-ojdbc14-*-sqlrelay121*-before), but
// no client on file other than ojdbc14 sends one, so what a real server does
// with it is unmeasured - the drop is the module's choice.  the 0x00 client
// is here because it gets the 8i verifier but isn't an 8i client, its field
// version being 11.2 or 12.1
static void runPreLoginVersionCall(unsigned char logontypes) {

	const listenerinfo	*li=&listeners[LISTENER_12_1];

	char	label[256];
	charstring::printf(label,sizeof(label),
			"%s, logon types 0x%02x, field version %d, "
			"tns version 0x%04x, version call ahead of the login",
			li->name,logontypes,clientfieldversion,
			negotiatedconnectversion);
	stdoutput.printf("\n%s - expect the connection dropped\n",label);

	oracleprotocolclient	client;
	client.setNativeEncoding(nativeencoding);
	client.setConnectVersion(clientconnectversion);
	setDataTypeRequest(&client,SESSION_HAND_BUILT,logontypes);
	client.setAnoNegotiation(true);

	stringbuffer	l;
	l.append(label)->append(" connect");
	if (!client.connect(host,getPort(li),sid)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}
	checkVersionCallDropped(&client,label);
}

// ojdbc14 sends one version call ahead of its login (MEASURED, packet [0011]
// of samples/10545-dev-ojdbc14-* and samples/10734-dev-ojdbc14-*), and the
// module answers only the first.  what a real server does with a second is
// unmeasured - the drop is the module's choice
static void runSecondPreLoginVersionCall() {

	const listenerinfo	*li=&listeners[LISTENER_12_1];

	char	label[256];
	charstring::printf(label,sizeof(label),
			"%s, ojdbc14, tns version 0x%04x, second version call "
			"ahead of the login",li->name,
			negotiatedconnectversion);
	stdoutput.printf("\n%s - expect the first answered and the "
				"connection dropped at the second\n",label);

	oracleprotocolclient	client;
	client.setConnectVersion(clientconnectversion);
	client.setDataTypeRequest(ORA_OJDBC14_DATA_TYPE_REQUEST,
					sizeof(ORA_OJDBC14_DATA_TYPE_REQUEST));
	client.setAnoNegotiation(true);

	stringbuffer	l;
	l.append(label)->append(" connect");
	if (!client.connect(host,getPort(li),sid)) {
		report(l.getString(),false);
		stdoutput.printf("    %s\n",client.getError());
		return;
	}
	if (!checkOjdbc14Version(&client,label,li)) {
		return;
	}
	checkVersionCallDropped(&client,label);
}

// ---- ojdbc14's queries ----

// the al8i4 elements ojdbc14 sends, MEASURED in the 10745 real server
// captures: a query, ddl or dml, a prepared query's re-execute (fetch 10 rows
// with it) and a prepared dml's re-execute
static const uint32_t	AL8I4_QUERY[ORA_8I_AL8I4_SIZE]={
	1,0,0,0,0,0,0,1,0,0,0,0,0
};
static const uint32_t	AL8I4_DML[ORA_8I_AL8I4_SIZE]={
	1,1,0,0,0,0,0,0,0,0,0,0,0
};
static const uint32_t	AL8I4_REEXECUTE_QUERY[ORA_8I_AL8I4_SIZE]={
	0,10,0,0,0,0,0,1,0,0,0,0,0
};
static const uint32_t	AL8I4_REEXECUTE_DML[ORA_8I_AL8I4_SIZE]={
	0,1,0,0,0,0,0,0,0,0,0,0,0
};

// the options of each 0x5e, MEASURED: a statement, one with autocommit on, a
// prepared statement's first execute, the same with autocommit on, a
// prepared query's re-execute and a prepared dml's re-execute
static const uint32_t	OJDBC14_STATEMENT=0x8021;
static const uint32_t	OJDBC14_STATEMENT_COMMIT=0x8121;
static const uint32_t	OJDBC14_PREPARED=0x8029;
static const uint32_t	OJDBC14_PREPARED_COMMIT=0x8129;
static const uint32_t	OJDBC14_REEXECUTE_QUERY=0x8068;
static const uint32_t	OJDBC14_REEXECUTE_DML=0x8120;

// a prepared dml's re-execute with autocommit off, MEASURED in #10750's
// prepexecerr captures
static const uint32_t	OJDBC14_REEXECUTE_DML_NO_COMMIT=0x8020;

// how many rows each fetch asks for, ojdbc14's default prefetch
static const uint32_t	OJDBC14_FETCH_ROWS=10;

// the bind descriptors of setInt() and setString() at charset 873, the
// second for "bind0" (5 characters, 20 bytes) and "bound2" (6, 24): packets
// [0017] of 10745-dev-ojdbc14-prepared-realserver-oracle122 and [0026] of
// -dmlauto- beside it
static const unsigned char	OJDBC14_BINDS_INT_STRING5[]={
	0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03,
	0x69, 0x01, 0x00,
	0x01, 0x03, 0x00, 0x00, 0x01, 0x14, 0x00, 0x01, 0x10, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00
};
static const unsigned char	OJDBC14_BINDS_INT_STRING6[]={
	0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03,
	0x69, 0x01, 0x00,
	0x01, 0x03, 0x00, 0x00, 0x01, 0x18, 0x00, 0x01, 0x10, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00
};

// the bind values that follow them, the number as its oracle number bytes:
// 10 and "bind0", 11 and "bind1", 2 and "bound2", 3 and "bound3"
static const unsigned char	OJDBC14_ROW_BIND0[]={
	0x07, 0x02, 0xc1, 0x0b, 0x05, 'b', 'i', 'n', 'd', '0'
};
static const unsigned char	OJDBC14_ROW_BIND1[]={
	0x07, 0x02, 0xc1, 0x0c, 0x05, 'b', 'i', 'n', 'd', '1'
};
static const unsigned char	OJDBC14_ROW_BOUND2[]={
	0x07, 0x02, 0xc1, 0x03, 0x06, 'b', 'o', 'u', 'n', 'd', '2'
};
static const unsigned char	OJDBC14_ROW_BOUND3[]={
	0x07, 0x02, 0xc1, 0x04, 0x06, 'b', 'o', 'u', 'n', 'd', '3'
};

// what the builders have to reproduce: ojdbc14's calls as it sent them to
// real servers, behind the data flags - see checkOjdbc14CallBuilders() for
// the packet each is from
static const unsigned char	OJDBC14_CALL_SELECT1[]={
	0x03, 0x5e, 0x00, 0x02, 0x80, 0x21, 0x00, 0x01, 0x01, 0x12, 0x01, 0x01,
	0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 's', 'e', 'l', 'e', 'c',
	't', ' ', '1', ' ', 'f', 'r', 'o', 'm', ' ', 'd', 'u', 'a',
	'l', 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00
};
static const unsigned char	OJDBC14_CALL_FETCH[]={
	0x03, 0x05, 0x00, 0x01, 0x01, 0x01, 0x0a
};
static const unsigned char	OJDBC14_CALL_CLOSE_LOGOFF[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x09, 0x00
};
static const unsigned char	OJDBC14_CALL_PREPARED[]={
	0x03, 0x5e, 0x00, 0x02, 0x80, 0x29, 0x00, 0x01, 0x01, 0x1b, 0x01, 0x01,
	0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x01, 0x01,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 's', 'e', 'l', 'e',
	'c', 't', ' ', ':', '1', ' ', 'n', ',', ' ', ':', '2', ' ',
	'v', ' ', 'f', 'r', 'o', 'm', ' ', 'd', 'u', 'a', 'l', 0x01,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x03, 0x69, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x01, 0x14, 0x00,
	0x01, 0x10, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07, 0x02, 0xc1,
	0x0b, 0x05, 'b', 'i', 'n', 'd', '0'
};
static const unsigned char	OJDBC14_CALL_REEXECUTE_QUERY[]={
	0x03, 0x5e, 0x00, 0x02, 0x80, 0x68, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
	0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x01, 0x01,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
	0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x69,
	0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x01, 0x14, 0x00, 0x01, 0x10, 0x00,
	0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07, 0x02, 0xc1, 0x0c, 0x05, 'b',
	'i', 'n', 'd', '1'
};
static const unsigned char	OJDBC14_CALL_CLOSE_PREPARED_DML[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x06, 0x03, 0x5e, 0x00, 0x02,
	0x81, 0x29, 0x00, 0x01, 0x01, 0x2c, 0x01, 0x01, 0x0d, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 'i', 'n', 's', 'e', 'r', 't', ' ', 'i',
	'n', 't', 'o', ' ', 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l',
	't', 'e', 's', 't', '1', '0', '7', '4', '5', ' ', 'v', 'a',
	'l', 'u', 'e', 's', ' ', '(', ':', '1', ',', ':', '2', ')',
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00,
	0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x01, 0x18,
	0x00, 0x01, 0x10, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07, 0x02,
	0xc1, 0x03, 0x06, 'b', 'o', 'u', 'n', 'd', '2'
};
static const unsigned char	OJDBC14_CALL_REEXECUTE_DML[]={
	0x03, 0x5e, 0x00, 0x02, 0x81, 0x20, 0x01, 0x05, 0x00, 0x00, 0x01, 0x01,
	0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x02, 0xc1,
	0x04, 0x06, 'b', 'o', 'u', 'n', 'd', '3'
};
static const unsigned char	OJDBC14_CALL_CLOSE_COMMIT[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x06, 0x03, 0x0e, 0x00
};
static const unsigned char	OJDBC14_CALL_CLOSE_ROLLBACK[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x05, 0x03, 0x0f, 0x00
};
static const unsigned char	OJDBC14_CALL_CLOSE2_LOGOFF[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x02, 0x01, 0x03, 0x01, 0x01, 0x03, 0x09,
	0x00
};

// the calls of #10750's sessions, as ojdbc14 sent them to real servers: the
// fetches of 65535, 65536, 70000 and 100001 rows, whose counts above 65535
// take 3 bytes where appendLenPreInt() would write 4 (packets [0019], [0027],
// [0035] and [0043] of 10750-dev-ojdbc14-fetchsizes-realserver-oracle102),
// the re-execute with fetch of 70000 rows (packet [0041] of 10750-dev-
// ojdbc14-prepfetchsizes-realserver-oracle122, which binds 3 where this
// binds 70005, the rest being the same bytes), and the rest from the
// captures checkOjdbc14CallBuilders() names
static const unsigned char	OJDBC14_CALL_FETCH_65535[]={
	0x03, 0x05, 0x00, 0x01, 0x02, 0x02, 0xff, 0xff
};
static const unsigned char	OJDBC14_CALL_FETCH_65536[]={
	0x03, 0x05, 0x00, 0x01, 0x01, 0x03, 0x01, 0x00, 0x00
};
static const unsigned char	OJDBC14_CALL_FETCH_70000[]={
	0x03, 0x05, 0x00, 0x01, 0x02, 0x03, 0x01, 0x11, 0x70
};
static const unsigned char	OJDBC14_CALL_FETCH_100001[]={
	0x03, 0x05, 0x00, 0x01, 0x01, 0x03, 0x01, 0x86, 0xa1
};
static const unsigned char	OJDBC14_CALL_REEXECUTE_70000[]={
	0x03, 0x5e, 0x00, 0x02, 0x80, 0x68, 0x01, 0x01, 0x00, 0x00, 0x01,
	0x01, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x01, 0x11, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x01, 0x16,
	0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07, 0x04,
	0xc3, 0x08, 0x01, 0x06
};
static const unsigned char	OJDBC14_CALL_PREPARED_LEVEL[]={
	0x03, 0x5e, 0x00, 0x02, 0x80, 0x29, 0x00, 0x01, 0x01, 0x2f, 0x01,
	0x01, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 's', 'e',
	'l', 'e', 'c', 't', ' ', 'l', 'e', 'v', 'e', 'l', ' ', 'n', ' ', 'f',
	'r', 'o', 'm', ' ', 'd', 'u', 'a', 'l', ' ', 'c', 'o', 'n', 'n', 'e',
	'c', 't', ' ', 'b', 'y', ' ', 'l', 'e', 'v', 'e', 'l', ' ', '<', '=',
	' ', ':', '1', 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x01,
	0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07,
	0x02, 0xc1, 0x1a
};
static const unsigned char	OJDBC14_CALL_REEXECUTE_10[]={
	0x03, 0x5e, 0x00, 0x02, 0x80, 0x68, 0x01, 0x01, 0x00, 0x00, 0x01,
	0x01, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07, 0x02, 0xc1, 0x1a
};
static const unsigned char	OJDBC14_CALL_CLOSE_PREPARED_INSERT3[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x06, 0x03, 0x5e, 0x00,
	0x02, 0x80, 0x29, 0x00, 0x01, 0x01, 0x32, 0x01, 0x01, 0x0d, 0x00,
	0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x01, 0x01, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'i', 'n', 's', 'e', 'r',
	't', ' ', 'i', 'n', 't', 'o', ' ', 'p', 'r', 'o', 't', 'o', 'c', 'o',
	'l', 't', 'e', 's', 't', '1', '0', '7', '5', '0', 'p', ' ', 'v', 'a',
	'l', 'u', 'e', 's', ' ', '(', ':', '1', ',', ' ', ':', '2', ',', ' ',
	':', '3', ')', 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x01,
	0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x01,
	0x03, 0x00, 0x00, 0x01, 0x04, 0x00, 0x01, 0x10, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x01, 0x04, 0x00,
	0x01, 0x10, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07, 0x02,
	0xc1, 0x02, 0x01, 0x62, 0x01, 0x62
};
static const unsigned char	OJDBC14_CALL_CLOSE_REEXECUTE_INSERT3[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x05, 0x03, 0x5e, 0x00,
	0x02, 0x80, 0x20, 0x01, 0x0a, 0x00, 0x00, 0x01, 0x01, 0x0d, 0x00,
	0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x02,
	0xc1, 0x02, 0x01, 0x62, 0x01, 0x62
};
static const unsigned char	OJDBC14_CALL_CLOSE_REPARSE_INSERT3[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x04, 0x03, 0x5e, 0x00,
	0x02, 0x80, 0x29, 0x01, 0x0a, 0x01, 0x01, 0x32, 0x01, 0x01, 0x0d,
	0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x01, 0x01,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'i', 'n', 's', 'e',
	'r', 't', ' ', 'i', 'n', 't', 'o', ' ', 'p', 'r', 'o', 't', 'o', 'c',
	'o', 'l', 't', 'e', 's', 't', '1', '0', '7', '5', '0', 'p', ' ', 'v',
	'a', 'l', 'u', 'e', 's', ' ', '(', ':', '1', ',', ' ', ':', '2', ',',
	' ', ':', '3', ')', 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00,
	0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00,
	0x01, 0x03, 0x00, 0x00, 0x01, 0x04, 0x00, 0x01, 0x10, 0x00, 0x00,
	0x02, 0x03, 0x69, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x01, 0x30,
	0x00, 0x01, 0x10, 0x00, 0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07,
	0x02, 0xc1, 0x1f, 0x01, 0x62, 0x0c, 't', 'o', 'o', 'l', 'o', 'n', 'g',
	'v', 'a', 'l', 'u', 'e'
};
static const unsigned char	OJDBC14_CALL_PREPARED_DIVIDE[]={
	0x03, 0x5e, 0x00, 0x02, 0x80, 0x29, 0x00, 0x01, 0x01, 0x37, 0x01,
	0x01, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 's', 'e',
	'l', 'e', 'c', 't', ' ', '1', '/', '(', ':', '1', ' ', '-', ' ', 'l',
	'e', 'v', 'e', 'l', ')', ' ', 'x', ' ', 'f', 'r', 'o', 'm', ' ', 'd',
	'u', 'a', 'l', ' ', 'c', 'o', 'n', 'n', 'e', 'c', 't', ' ', 'b', 'y',
	' ', 'l', 'e', 'v', 'e', 'l', ' ', '<', '=', ' ', '5', 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00,
	0x00, 0x02, 0x03, 0x69, 0x01, 0x00, 0x07, 0x02, 0xc1, 0x0b
};
static const unsigned char	OJDBC14_CALL_CLOSE_REEXECUTE_DIVIDE[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x5e, 0x00,
	0x02, 0x80, 0x68, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x0d, 0x00,
	0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00, 0x07, 0x02, 0xc1, 0x04
};
static const unsigned char	OJDBC14_CALL_CLOSE_INSERT[]={
	0x11, 0x69, 0x00, 0x01, 0x01, 0x01, 0x01, 0x04, 0x03, 0x5e, 0x00,
	0x02, 0x80, 0x21, 0x00, 0x01, 0x01, 0x33, 0x01, 0x01, 0x0d, 0x00,
	0x00, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'i', 'n', 's', 'e', 'r', 't', ' ',
	'i', 'n', 't', 'o', ' ', 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l', 't',
	'e', 's', 't', '1', '0', '7', '5', '0', 'e', ' ', 'v', 'a', 'l', 'u',
	'e', 's', ' ', '(', '1', ',', ' ', 0x27, 0x62, 0x27, 0x2c, 0x20, 0x27,
	0x62, 0x27, 0x29, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


// the al8i4 vectors of a prepared query's re-execute with fetch asking for
// 70000 and 100001 rows, the first MEASURED (packet [0041] above), the
// second the same with one more row than the module's MAX_FETCH_ROW_COUNT
static const uint32_t	AL8I4_REEXECUTE_70000[ORA_8I_AL8I4_SIZE]={
	0,70000,0,0,0,0,0,1,0,0,0,0,0
};
static const uint32_t	AL8I4_REEXECUTE_100001[ORA_8I_AL8I4_SIZE]={
	0,100001,0,0,0,0,0,1,0,0,0,0,0
};

// the bind descriptors of setInt() alone, and of setInt() and two
// setString()s of 1 character each, 4 bytes at charset 873, and of 1 and 12
// characters: [0017] of 10750-dev-ojdbc14-prepfetcherr-realserver-oracle122,
// and [0026] and [0050] of -prepexecerr- beside it
static const unsigned char	OJDBC14_BINDS_INT[]={
	0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03,
	0x69, 0x01, 0x00
};
static const unsigned char	OJDBC14_BINDS_INT_STRING1_STRING1[]={
	0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03,
	0x69, 0x01, 0x00,
	0x01, 0x03, 0x00, 0x00, 0x01, 0x04, 0x00, 0x01, 0x10, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00,
	0x01, 0x03, 0x00, 0x00, 0x01, 0x04, 0x00, 0x01, 0x10, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00
};
static const unsigned char	OJDBC14_BINDS_INT_STRING1_STRING12[]={
	0x06, 0x03, 0x00, 0x00, 0x01, 0x16, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03,
	0x69, 0x01, 0x00,
	0x01, 0x03, 0x00, 0x00, 0x01, 0x04, 0x00, 0x01, 0x10, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00,
	0x01, 0x03, 0x00, 0x00, 0x01, 0x30, 0x00, 0x01, 0x10, 0x00, 0x00, 0x02,
	0x03, 0x69, 0x01, 0x00
};

// one column a describe has to carry, in the shape the module sends.
// OJDBC14_ANY takes the precision, the scale or the size from the answer,
// where only the backend decides it, so what is checked there is the form
struct ojdbc14column {
	unsigned char	type;
	int		precision;
	int		scale;
	int		size;
	const char	*name;
};
static const int	OJDBC14_ANY=-1000;

// one value of a row: its bytes, or NULL for a null
struct ojdbc14value {
	const char	*bytes;
	size_t		size;
};

// a query session in progress.  "ok" goes false at the first call that isn't
// answered or that ojdbc14 couldn't read, and the rest of the session is
// skipped.  "urgent" sends one byte of urgent data right behind the next
// call, the way ojdbc14 cancels one
struct ojdbc14session {
	oracleprotocolclient		*client;
	const char			*label;
	oracleprotocolojdbc14answer	*answer;
	bool				ok;
	bool				urgent;
};

// the column types the module sends in the abbreviated form, with no
// character set, form of use or size in characters - see putColumnMetadata()
// in src/protocols/oracle.cpp
static bool ojdbc14AbbreviatedType(unsigned char type) {
	return (type==2 || type==11 || type==23 || type==24 || type==113 ||
		type==114 || type==180 || type==181 || type==182 ||
		type==183 || type==231);
}

// the types ojdbc14 reads a scale as a ub2 for, and a raw byte for the rest
static bool ojdbc14Ub2ScaleType(unsigned char type) {
	return (type==2 || type==180 || type==181 || type==183 || type==231);
}

// a ub2 in ojdbc14's form: a length byte with 0x80 for negative, then the
// magnitude
static void putOjdbc14Signed(unsigned char *buffer, size_t *size, int value) {
	if (!value) {
		putByte(buffer,size,0);
	} else if (value<0) {
		putByte(buffer,size,0x81);
		putByte(buffer,size,(unsigned char)(-value));
	} else {
		putByte(buffer,size,1);
		putByte(buffer,size,(unsigned char)value);
	}
}

// a clr of up to 252 bytes
static void putShortClr(unsigned char *buffer, size_t *size,
					const char *value, size_t valuesize) {
	putByte(buffer,size,(unsigned char)valuesize);
	bytestring::copy(buffer+*size,value,valuesize);
	*size+=valuesize;
}

// the 23 fields of an error object as ojdbc14's T4CTTIoer reads them, and the
// message behind them if the error number isn't 0
struct ojdbc14oer {
	uint32_t	endtoend;
	uint32_t	rows;
	uint32_t	oranum;
	uint32_t	arrayelementwitherror;
	uint32_t	arrayelementerrno;
	uint32_t	cursorid;
	uint32_t	errorposition;
	unsigned char	sqltype;
	uint32_t	rba;
	uint32_t	successiterations;
	const char	*message;
	size_t		messagesize;
};

// an error object, in the field order real servers send it (MEASURED, and
// checked against real objects by checkOjdbc14ErrorObjectBuilder()): the end
// to end field, the row number, the error number, the array element with an
// error and its error number, the cursor, the error position, the sql type,
// five raw zeros (fatal, flags, user cursor options, upi, warning), the
// rowid's rba, four zeros for the rest of the rowid, four for the os error,
// the statement number, the call number (0, ojdbc14's sequence number) and
// the pad, then success iterations and the message if there's an error
static void putOjdbc14Oer(unsigned char *buffer, size_t *size,
						const ojdbc14oer *e) {
	putByte(buffer,size,ORA_TTC_ERROR);
	putLenPreInt(buffer,size,e->endtoend);
	putLenPreInt(buffer,size,e->rows);
	putLenPreInt(buffer,size,e->oranum);
	putLenPreInt(buffer,size,e->arrayelementwitherror);
	putLenPreInt(buffer,size,e->arrayelementerrno);
	putLenPreInt(buffer,size,e->cursorid);
	putLenPreInt(buffer,size,e->errorposition);
	putByte(buffer,size,e->sqltype);
	putZeros(buffer,size,5);
	putLenPreInt(buffer,size,e->rba);
	putZeros(buffer,size,4);
	putZeros(buffer,size,4);
	putLenPreInt(buffer,size,e->successiterations);
	if (e->oranum) {
		putShortClr(buffer,size,e->message,e->messagesize);
	}
}

// the error object the module sends ojdbc14.  what is a real server's:
// the end to end field (the session's call count), the error number, the sql
// type, the message, the row number of an error at execute time (0), and
// success iterations: 0 in an error answering anything but a call that
// executes and fetches (0x8068), 1 there (MEASURED, step 3a of #10750, both
// real servers), and 1 with no error or ORA-01403.  what is the module's
// own choice, which ojdbc14 reads and ignores (CROSS-CHECK, T4CTTIoer's
// users): the cursor id (any the module opened), the array element fields
// (0, where a real server keeps 1 and 1400 or 2 and 12899 from the last dml
// error), the error position (0, where a real server sends the failing
// token's offset - the module can't learn it from the back end), the rowid's
// rba (0, where a real server sends ffffffff in and after a dml error), the
// row number of a select's error (the rows sent, where a real server counts
// the rows its cursor produced before the failing one, sent or not) and
// success iterations 1 in an answer with no error (oracle122 sends 0 in a
// fetch's)
static void putOjdbc14Summary(unsigned char *buffer, size_t *size,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid,
					unsigned char sqltype,
					uint32_t successiterations,
					const char *message, size_t messagesize) {
	ojdbc14oer	e;
	bytestring::zero(&e,sizeof(e));
	e.endtoend=endtoend;
	e.rows=rows;
	e.oranum=oranum;
	e.cursorid=cursorid;
	e.sqltype=sqltype;
	e.successiterations=successiterations;
	e.message=message;
	e.messagesize=messagesize;
	putOjdbc14Oer(buffer,size,&e);
}

static const char	*ORA_01403="ORA-01403: no data found\n";

// the label of one check
static void ojdbc14Label(stringbuffer *l, ojdbc14session *s,
					const char *what, const char *check) {
	l->clear();
	l->append(s->label)->append(", ")->append(what)->append(check);
}

// one byte for byte comparison, dumped on failure
static void checkOjdbc14Bytes(ojdbc14session *s, const char *what,
					const char *check,
					const unsigned char *actual,
					size_t actualsize,
					const unsigned char *expected,
					size_t expectedsize) {
	stringbuffer	l;
	ojdbc14Label(&l,s,what,check);
	bool	ok=(actualsize==expectedsize &&
			!bytestring::compare(actual,expected,expectedsize));
	report(l.getString(),ok);
	if (!ok) {
		dumpBytes("got",actual,actualsize);
		dumpBytes("expected",expected,expectedsize);
	}
}

// the call built in the client, sent, and its answer read as ojdbc14 reads
// it, markers and all.  "columncount" is the cursor's, for a fetch.  false,
// reported, if it wasn't answered or couldn't be read.  "maxseconds", if not
// 0, is how long the answer may take
static bool sendOjdbc14Call(ojdbc14session *s, const char *what,
					uint32_t columncount, bool statuscall,
					time_t maxseconds) {
	if (!s->ok) {
		return false;
	}

	stringbuffer	l;
	datetime	dt;
	dt.initFromSystemDateTime();
	time_t		start=dt.getEpoch();
	bool		answered=s->client->sendPacket();
	if (answered && s->urgent) {
		s->urgent=false;
		ojdbc14Label(&l,s,what,
			" is followed by one byte of urgent data, 0x21");
		bool	sent=s->client->sendUrgentByte(0x21);
		report(l.getString(),sent);
		answered=sent;
	}
	answered=(answered && s->client->recvOjdbc14Answer());
	dt.initFromSystemDateTime();
	time_t		elapsed=dt.getEpoch()-start;

	ojdbc14Label(&l,s,what," is answered");
	report(l.getString(),answered);
	if (!answered) {
		stdoutput.printf("    %s\n",s->client->getError());
		s->ok=false;
		return false;
	}
	if (maxseconds) {
		char	check[64];
		charstring::printf(check,sizeof(check),
				" is answered within %d seconds",
				(int)maxseconds);
		ojdbc14Label(&l,s,what,check);
		report(l.getString(),elapsed<maxseconds);
		if (elapsed>=maxseconds) {
			stdoutput.printf("    took %d seconds\n",(int)elapsed);
		}
	}

	ojdbc14Label(&l,s,what," answer reads the way ojdbc14 reads it");
	bool	read=s->client->readOjdbc14Answer(s->answer,columncount,
								statuscall);
	report(l.getString(),read);
	if (!read) {
		stdoutput.printf("    %s\n",s->client->getError());
		dumpBytes("answer",s->client->getResponse(),
					s->client->getResponseSize());
		s->ok=false;
		return false;
	}
	return true;
}

// a 0x5e of ojdbc14's.  "columncount" is the cursor's, for a re-execute that
// fetches
static bool ojdbc14Query(ojdbc14session *s, const char *what,
				uint32_t options, uint32_t cursorid,
				const char *query, const uint32_t *al8i4,
				const unsigned char *descriptors,
				size_t descriptorssize,
				uint32_t bindcount,
				const unsigned char *bindrow,
				size_t bindrowsize,
				const uint32_t *closecursors,
				uint32_t closecount,
				uint32_t columncount,
				time_t maxseconds) {
	oracleprotocolojdbc14query	q;
	q.options=options;
	q.cursorid=cursorid;
	q.query=query;
	q.al8i4=al8i4;
	q.bindcount=(descriptors)?bindcount:0;
	q.descriptors=descriptors;
	q.descriptorssize=descriptorssize;
	q.bindrow=bindrow;
	q.bindrowsize=bindrowsize;
	if (s->ok) {
		s->client->buildOjdbc14Query(&q,closecursors,closecount);
	}
	return sendOjdbc14Call(s,what,columncount,false,maxseconds);
}

// a plain statement's 0x5e, with no binds
static bool ojdbc14Statement(ojdbc14session *s, const char *what,
				uint32_t options, uint32_t cursorid,
				const char *query, const uint32_t *al8i4,
				const uint32_t *closecursors,
				uint32_t closecount) {
	return ojdbc14Query(s,what,options,cursorid,query,al8i4,
				NULL,0,0,NULL,0,closecursors,closecount,0,0);
}

// a fetch of "rowstofetch" rows, ojdbc14's fetch size
static bool ojdbc14FetchRows(ojdbc14session *s, const char *what,
					uint32_t cursorid,
					uint32_t columncount,
					uint32_t rowstofetch,
					const uint32_t *closecursors,
					uint32_t closecount) {
	if (s->ok) {
		s->client->buildOjdbc14Fetch(cursorid,rowstofetch,
						closecursors,closecount);
	}
	return sendOjdbc14Call(s,what,columncount,false,0);
}

static bool ojdbc14Fetch(ojdbc14session *s, const char *what,
					uint32_t cursorid,
					uint32_t columncount) {
	return ojdbc14FetchRows(s,what,cursorid,columncount,
					OJDBC14_FETCH_ROWS,NULL,0);
}

// which objects the answer carries, in order, as a string of ttc codes
static void checkOjdbc14Objects(ojdbc14session *s, const char *what,
							const char *codes) {
	stringbuffer	actual;
	for (size_t i=0; i<s->answer->objectcount; i++) {
		char	code[4];
		charstring::printf(code,sizeof(code),"%s%02x",
					(i)?" ":"",s->answer->objects[i].code);
		actual.append(code);
	}
	stringbuffer	l;
	ojdbc14Label(&l,s,what," objects are ");
	l.append(codes);
	bool	ok=!charstring::compare(actual.getString(),codes);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    got %s\n",actual.getString());
	}
}

// the nth object with "code", or NULL
static const oracleprotocolojdbc14object *findOjdbc14Object(
					oracleprotocolojdbc14answer *a,
					unsigned char code) {
	for (size_t i=0; i<a->objectcount; i++) {
		if (a->objects[i].code==code) {
			return &a->objects[i];
		}
	}
	return NULL;
}

// one object, byte for byte
static void checkOjdbc14Object(ojdbc14session *s, const char *what,
					const char *check, unsigned char code,
					const unsigned char *expected,
					size_t expectedsize) {
	const oracleprotocolojdbc14object	*o=
					findOjdbc14Object(s->answer,code);
	const unsigned char	*response=s->client->getResponse();
	checkOjdbc14Bytes(s,what,check,
				(o)?response+o->offset:NULL,(o)?o->size:0,
				expected,expectedsize);
}

// the return parameters: six al8o4 values, the third the cursor, and no
// al8txl or key/value pairs.  the cursor is the module's choice, read from
// there, and every later object about the statement has to name it too
static uint32_t checkOjdbc14ReturnParameters(ojdbc14session *s,
							const char *what) {
	uint32_t	cursorid=(s->answer->al8o4count>2)?
					(uint32_t)s->answer->al8o4[2]:0;
	unsigned char	expected[32];
	size_t		size=0;
	putByte(expected,&size,ORA_TTC_OK);
	putLenPreInt(expected,&size,6);
	putZeros(expected,&size,2);
	putLenPreInt(expected,&size,cursorid);
	putZeros(expected,&size,5);
	checkOjdbc14Object(s,what," return parameters are exact",
					ORA_TTC_OK,expected,size);

	stringbuffer	l;
	ojdbc14Label(&l,s,what," cursor id isn't 0");
	report(l.getString(),cursorid!=0);
	return cursorid;
}

// no break or reset marker came ahead of an answer with no error in it, as a
// real server sends none there (MEASURED, step 3a of #10750)
static void checkOjdbc14NoMarkers(ojdbc14session *s, const char *what) {
	stringbuffer	l;
	ojdbc14Label(&l,s,what," answer has no break or reset marker ahead");
	report(l.getString(),!s->client->gotBreak());
}

// the error object of an answer with no error in it, or ORA-01403, byte for
// byte, with success iterations 1
static void checkOjdbc14Summary(ojdbc14session *s, const char *what,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid,
					unsigned char sqltype) {
	checkOjdbc14NoMarkers(s,what);
	const char	*message=(oranum==1403)?ORA_01403:NULL;
	size_t		messagesize=(oranum==1403)?
					charstring::getLength(ORA_01403):0;
	unsigned char	expected[64+256];
	size_t		size=0;
	putOjdbc14Summary(expected,&size,endtoend,rows,oranum,cursorid,
						sqltype,1,message,messagesize);
	char	check[96];
	charstring::printf(check,sizeof(check),
			" error object is exact, end to end %d, rows %d, "
			"error %d",endtoend,rows,oranum);
	checkOjdbc14Object(s,what,check,ORA_TTC_ERROR,expected,size);
}

// an error's message: the whole of it, or for one that names the back end's
// schema, which differs per listener, what comes before the schema and what
// comes after it.  "suffix" is NULL for a message checked whole
struct ojdbc14message {
	const char	*prefix;
	const char	*suffix;
};

// the messages a real 12.2 server sends ojdbc14 (MEASURED, the 10750-dev-
// ojdbc14-*-realserver-oracle122 captures, there in schema TESTUSER and
// table PROTOCOLTEST10750E), each a single line ending in a line feed
static const ojdbc14message	MSG_00001={
	"ORA-00001: unique constraint (",
	".PROTOCOLTEST10750_PK) violated\n"
};
static const ojdbc14message	MSG_01400={
	"ORA-01400: cannot insert NULL into (\"",
	"\".\"PROTOCOLTEST10750\".\"NN\")\n"
};
static const ojdbc14message	MSG_01722={
	"ORA-01722: invalid number\n",NULL
};
static const ojdbc14message	MSG_12899={
	"ORA-12899: value too large for column \"",
	"\".\"PROTOCOLTEST10750\".\"V\" (actual: 12, maximum: 5)\n"
};
static const ojdbc14message	MSG_01476={
	"ORA-01476: divisor is equal to zero\n",NULL
};
static const ojdbc14message	MSG_00942={
	"ORA-00942: table or view does not exist\n",NULL
};

// the module's own, which no real server sends: ORA-20004 for a fetch of
// more rows than MAX_FETCH_ROW_COUNT, 100000, in src/protocols/oracle.cpp
static const ojdbc14message	MSG_20004={
	"ORA-20004: Maximum fetch row count exceeded.\n",NULL
};

// the break and reset markers' flags byte a real server sets: 0x20 when the
// version the client read off the accept is 0x013b or higher, 0 below it,
// which is where ojdbc14's 0x0134 is
static unsigned char ojdbc14MarkerFlags(ojdbc14session *s) {
	return (s->client->getAcceptVersion()>=ORA_PROTOCOL_VERSION_12)?
						ORA_MARKER_PACKET_FLAGS:0;
}

// an answer with an error other than ORA-01403 in it, the way a real server
// sends one (MEASURED, all 27 errors per server of step 3a of #10750, e.g.
// [0027] to [0030] of 10750-dev-ojdbc14-execerr-realserver-oracle122): a
// break marker and a reset marker, which the client answers with one reset
// marker, and then the error object alone in a data packet.  then the
// message - one line ending in a line feed, with no zero byte, and no "Help:"
// line, which the 23ai client the back end links against appends and no real
// server sends - and the object around it byte for byte.  every answer to
// the next call checks the end to end field, which a module that left the
// client's reset for its next read would get out of step
static void checkOjdbc14QueryError(ojdbc14session *s, const char *what,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid,
					unsigned char sqltype,
					uint32_t successiterations,
					const ojdbc14message *m) {

	stringbuffer	l;
	unsigned char	flags=ojdbc14MarkerFlags(s);
	char		check[160];
	charstring::printf(check,sizeof(check),
			" error comes after a break and a reset marker, "
			"flags 0x%02x, and the client's reset",flags);
	ojdbc14Label(&l,s,what,check);
	bool	ok=(s->client->gotBreak() &&
			s->client->getBreakFlags()==flags &&
			s->client->getResetFlags()==flags);
	report(l.getString(),ok);
	if (!ok) {
		if (s->client->gotBreak()) {
			stdoutput.printf("    break flags 0x%02x, reset "
					"flags 0x%02x\n",
					s->client->getBreakFlags(),
					s->client->getResetFlags());
		} else {
			stdoutput.printf("    no markers, the error came "
							"first\n");
		}
	}
	checkOjdbc14Objects(s,what,"04");

	const char	*message=s->answer->message;
	size_t		messagesize=s->answer->messagesize;
	size_t		prefixsize=charstring::getLength(m->prefix);
	size_t		suffixsize=charstring::getLength(m->suffix);

	if (!m->suffix) {
		ojdbc14Label(&l,s,what," message is exactly \"");
		l.append(m->prefix,prefixsize-1)->append("\\n\"");
		ok=(messagesize==prefixsize &&
			!bytestring::compare(message,m->prefix,prefixsize));
	} else {
		ojdbc14Label(&l,s,what," message is \"");
		l.append(m->prefix)->append("<schema>");
		l.append(m->suffix,suffixsize-1)->append("\\n\"");
		ok=(messagesize>prefixsize+suffixsize &&
			!bytestring::compare(message,m->prefix,prefixsize) &&
			!bytestring::compare(message+messagesize-suffixsize,
							m->suffix,suffixsize) &&
			!bytestring::findFirst(message+prefixsize,
					(byte_t)'\n',
					messagesize-prefixsize-suffixsize));
	}
	report(l.getString(),ok);

	ojdbc14Label(&l,s,what," message ends in a line feed and has no "
					"zero byte and no Help: line");
	bool	tidy=(messagesize && message[messagesize-1]=='\n' &&
			!bytestring::findFirst(message,(byte_t)0,
							messagesize) &&
			!bytestring::findFirst(message,messagesize,
							"Help:",5));
	report(l.getString(),tidy);
	if (!ok || !tidy) {
		dumpBytes("message",(const unsigned char *)message,
							messagesize);
	}

	unsigned char	expected[64+256];
	size_t		size=0;
	putOjdbc14Summary(expected,&size,endtoend,rows,oranum,cursorid,
					sqltype,successiterations,
					message,messagesize);
	charstring::printf(check,sizeof(check),
			" error object is exact, end to end %d, rows %d, "
			"error %d, success iterations %d",
			endtoend,rows,oranum,successiterations);
	checkOjdbc14Object(s,what,check,ORA_TTC_ERROR,expected,size);
}

// the cursor an error names, which has to be one: the cursor the module
// opened for a statement on cursor 0, or "cursorid" for one on an open
// cursor
static uint32_t checkOjdbc14ErrorCursor(ojdbc14session *s, const char *what,
							uint32_t cursorid) {
	uint32_t	c=(uint32_t)s->answer->cursorid;
	stringbuffer	l;
	if (cursorid) {
		char	check[64];
		charstring::printf(check,sizeof(check),
				" error names cursor %d",(int)cursorid);
		ojdbc14Label(&l,s,what,check);
		report(l.getString(),c==cursorid);
	} else {
		ojdbc14Label(&l,s,what," error names a cursor, not 0");
		report(l.getString(),c!=0);
	}
	return c;
}

// a describe: the 23 byte block ojdbc14 skips, the column count and the
// module's constant 0x39, every column byte for byte, then the date, the
// flag 0 and the max data block size 8168 (02 1f e8), as both real servers
// send them.  the max row size and the skipped block are left alone
static void checkOjdbc14Describe(ojdbc14session *s, const char *what,
					const ojdbc14column *columns,
					uint32_t columncount) {

	oracleprotocolojdbc14answer	*a=s->answer;
	stringbuffer			l;

	ojdbc14Label(&l,s,what," describe block ojdbc14 skips is 23 bytes");
	report(l.getString(),a->prologuesize==23);

	char	check[128];
	charstring::printf(check,sizeof(check),
			" describe has %d columns and constant 0x39",
			(int)columncount);
	ojdbc14Label(&l,s,what,check);
	report(l.getString(),a->columncount==(int64_t)columncount &&
					a->describeconstant==0x39);

	const unsigned char	*response=s->client->getResponse();
	for (uint32_t i=0; i<columncount &&
				(int64_t)i<a->columncount; i++) {

		const ojdbc14column			*e=&columns[i];
		const oracleprotocolojdbc14column	*c=&a->columns[i];
		bool	abbreviated=ojdbc14AbbreviatedType(e->type);
		int	precision=(e->precision==OJDBC14_ANY)?
					(int)c->precision:e->precision;
		int	scale=(e->scale==OJDBC14_ANY)?
					(int)c->scale:e->scale;
		uint32_t	columnsize=(e->size==OJDBC14_ANY)?
					(uint32_t)c->maxlength:
					(uint32_t)e->size;
		size_t		namesize=charstring::getLength(e->name);

		unsigned char	expected[128];
		size_t		size=0;
		putByte(expected,&size,e->type);
		putByte(expected,&size,(abbreviated)?0x00:0x80);
		putByte(expected,&size,(unsigned char)precision);
		if (ojdbc14Ub2ScaleType(e->type)) {
			putOjdbc14Signed(expected,&size,scale);
		} else {
			putByte(expected,&size,(unsigned char)scale);
		}
		putLenPreInt(expected,&size,columnsize);
		putZeros(expected,&size,4);
		putLenPreInt(expected,&size,
				(abbreviated)?0:ORA_CHARSET_AL32UTF8);
		putByte(expected,&size,(abbreviated)?0:1);
		putLenPreInt(expected,&size,(abbreviated)?0:columnsize);
		putByte(expected,&size,1);
		putByte(expected,&size,(unsigned char)namesize);
		putLenPreInt(expected,&size,(uint32_t)namesize);
		if (namesize) {
			putShortClr(expected,&size,e->name,namesize);
		}
		putZeros(expected,&size,2);
		putLenPreInt(expected,&size,i);

		charstring::printf(check,sizeof(check),
				" column %d (%s) is exact",(int)i,e->name);
		checkOjdbc14Bytes(s,what,check,
					response+c->offset,c->size,
					expected,size);
	}

	// the date's dalc, then 00 and 02 1f e8
	const unsigned char	*tail=response+a->describetailoffset;
	bool	ok=(a->describetailsize==14 &&
			tail[0]==0x01 && tail[1]==0x07 && tail[2]==0x07 &&
			tail[10]==0x00 && tail[11]==0x02 &&
			tail[12]==0x1f && tail[13]==0xe8);
	ojdbc14Label(&l,s,what,
			" describe ends with the date, 00 and 02 1f e8");
	report(l.getString(),ok);
	if (!ok) {
		dumpBytes("tail",tail,a->describetailsize);
	}
}

// the row header the module sends ojdbc14: flags 0x02, the column count, an
// iteration number of 0, the rows the call asked for, a uac buffer length of
// 0 and no bit vector.  the rows asked for go out as their low 16 bits, as a
// minimal ub2, whatever number of rows follows, as real servers send them
// (MEASURED, step 3c of #10750: 65535 is 02 ff ff, 65536 is 00, 70000 is 02
// 11 70 and 100001 is 02 86 a1, and 10 is 01 0a when 5 rows follow).
// ojdbc14 reads the field with unmarshalUB2 and closes the connection with
// ORA-17412 on a longer length byte
static void putOjdbc14RowHeader(unsigned char *buffer, size_t *size,
						uint32_t columncount,
						uint32_t rowsasked) {
	putByte(buffer,size,ORA_TTC_ROW_HEADER);
	putByte(buffer,size,0x02);
	putLenPreInt(buffer,size,columncount);
	putZeros(buffer,size,1);
	putLenPreInt(buffer,size,rowsasked&0xffff);
	putZeros(buffer,size,3);
}

// a fetch answer, the whole of it byte for byte: the row header, a row
// object per row with a short clr per value, and the error object.  rows
// come in a run of "rowcount", "columncount" values each.  "rowsasked" is
// what the fetch asked for
static void checkOjdbc14FetchRows(ojdbc14session *s, const char *what,
					uint32_t columncount,
					const ojdbc14value *values,
					uint32_t rowcount,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid,
					uint32_t rowsasked) {
	checkOjdbc14NoMarkers(s,what);
	unsigned char	expected[2048];
	size_t		size=0;
	putZeros(expected,&size,2);
	if (rowcount) {
		putOjdbc14RowHeader(expected,&size,columncount,rowsasked);
	}
	for (uint32_t r=0; r<rowcount; r++) {
		putByte(expected,&size,ORA_TTC_ROW_DATA);
		for (uint32_t c=0; c<columncount; c++) {
			const ojdbc14value	*v=&values[r*columncount+c];
			if (!v->bytes) {
				putByte(expected,&size,0);
			} else {
				putShortClr(expected,&size,v->bytes,v->size);
			}
		}
	}
	putOjdbc14Summary(expected,&size,endtoend,rows,oranum,cursorid,3,1,
				ORA_01403,charstring::getLength(ORA_01403));

	char	check[96];
	charstring::printf(check,sizeof(check),
			" answer is exact, %d rows, end to end %d, error %d",
			(int)rowcount,(int)endtoend,(int)oranum);
	checkOjdbc14Bytes(s,what,check,s->client->getResponse(),
				s->client->getResponseSize(),expected,size);
}

// the same for a fetch of OJDBC14_FETCH_ROWS rows
static void checkOjdbc14Fetch(ojdbc14session *s, const char *what,
					uint32_t columncount,
					const ojdbc14value *values,
					uint32_t rowcount,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid) {
	checkOjdbc14FetchRows(s,what,columncount,values,rowcount,
				endtoend,rows,oranum,cursorid,
				OJDBC14_FETCH_ROWS);
}

// a commit's or a rollback's answer: 09 and the end to end field, exactly
static void checkOjdbc14Status(ojdbc14session *s, const char *what,
							uint32_t endtoend) {
	checkOjdbc14NoMarkers(s,what);
	unsigned char	expected[8];
	size_t		size=0;
	putZeros(expected,&size,2);
	putByte(expected,&size,ORA_8I_TTC_STATUS);
	putLenPreInt(expected,&size,endtoend);
	char	check[64];
	charstring::printf(check,sizeof(check),
				" answer is exact, end to end %d",
				(int)endtoend);
	checkOjdbc14Bytes(s,what,check,s->client->getResponse(),
				s->client->getResponseSize(),expected,size);
}

static bool ojdbc14Transaction(ojdbc14session *s, const char *what,
					bool commit, uint32_t endtoend,
					const uint32_t *closecursors,
					uint32_t closecount) {
	if (s->ok) {
		s->client->buildOjdbc14Call((commit)?ORA_8I_TTI_COMMIT:
							ORA_8I_TTI_ROLLBACK,
							closecursors,closecount);
	}
	if (!sendOjdbc14Call(s,what,0,true,0)) {
		return false;
	}
	checkOjdbc14Status(s,what,endtoend);
	return true;
}

// the select-1 column: NUMBER, precision 0 and scale -127, as the oracle
// backend describes a literal and as a real 12.2 server answers ojdbc14
static const ojdbc14column	SELECT1_COLUMNS[]={{2,0,-127,22,"1"}};
static const ojdbc14value	SELECT1_ROW[]={{"\xc1\x02",2}};

// "select 1 from dual", or a text that means the same, at cursor 0 and its
// fetch: the describe, the return parameters and the error object, then the
// row and ORA-01403.  "endtoend" is the 0x5e's.  the cursor comes back
static uint32_t ojdbc14Select1(ojdbc14session *s, const char *what,
					const char *query, uint32_t endtoend,
					const uint32_t *closecursors,
					uint32_t closecount) {
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,query,AL8I4_QUERY,
						closecursors,closecount)) {
		return 0;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,SELECT1_COLUMNS,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,endtoend,0,0,c,3);
	if (ojdbc14Fetch(s,"its fetch",c,1)) {
		checkOjdbc14Fetch(s,"its fetch",1,SELECT1_ROW,1,
						endtoend+1,1,1403,c);
	}
	return c;
}

// a dml or ddl statement's answer: the return parameters, then the error
// object with the rows it touched.  the cursor comes back
static uint32_t ojdbc14Dml(ojdbc14session *s, const char *what,
				uint32_t options, const char *query,
				uint32_t endtoend, uint32_t rows,
				unsigned char sqltype,
				const uint32_t *closecursors,
				uint32_t closecount) {
	if (!ojdbc14Statement(s,what,options,0,query,AL8I4_DML,
						closecursors,closecount)) {
		return 0;
	}
	checkOjdbc14Objects(s,what,"08 04");
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,endtoend,rows,0,c,sqltype);
	return c;
}

// the first drop of a dml session, whether or not the table is left from
// before: ORA-00942 alone, behind the markers, or the return parameters and
// a drop's summary.  either way the cursor stays open until the logoff,
// which is what keeps every later end to end field the same, and comes back
static uint32_t ojdbc14FirstDrop(ojdbc14session *s, uint32_t options,
							const char *table) {
	stringbuffer	drop;
	drop.append("drop table ")->append(table);
	const char	*what=drop.getString();
	if (!ojdbc14Statement(s,what,options,0,what,AL8I4_DML,NULL,0)) {
		return 0;
	}
	if (s->answer->objectcount==1) {
		uint32_t	c=checkOjdbc14ErrorCursor(s,what,0);
		checkOjdbc14QueryError(s,what,4,0,942,c,12,0,&MSG_00942);
		return c;
	}
	checkOjdbc14Objects(s,what,"08 04");
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,12);
	return c;
}

// the select of a dml session: the describe of id and v, then a fetch of
// "rowcount" rows
static uint32_t ojdbc14DmlSelect(ojdbc14session *s, uint32_t endtoend,
					const ojdbc14value *values,
					uint32_t rowcount,
					const uint32_t *closecursors,
					uint32_t closecount) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"ID"},
		{1,OJDBC14_ANY,OJDBC14_ANY,OJDBC14_ANY,"V"}
	};
	const char	*what="select id,v from protocoltest10745 order by id";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
			"select id,v from protocoltest10745 order by id",
			AL8I4_QUERY,closecursors,closecount)) {
		return 0;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,2);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,endtoend,0,0,c,3);
	if (ojdbc14Fetch(s,"its fetch",c,2)) {
		checkOjdbc14Fetch(s,"its fetch",2,values,rowcount,
					endtoend+1,rowcount,1403,c);
	}
	return c;
}

// each session's script.  every end to end field is a real server's, from
// the 10745 real server capture of the same calls, except where a session's
// comment says otherwise
enum ojdbc14script {
	SCRIPT_SELECT1,
	SCRIPT_ROWS25,
	SCRIPT_TWOQUERIES,
	SCRIPT_REUSE,
	SCRIPT_PREPARED,
	SCRIPT_ERROR,
	SCRIPT_LONGTEXT,
	SCRIPT_NEWLINE,
	SCRIPT_TYPES,
	SCRIPT_NULL,
	SCRIPT_DML,
	SCRIPT_DMLAUTO,
	SCRIPT_COMMITFIRST,
	SCRIPT_EXECERR,
	SCRIPT_PREPEXECERR,
	SCRIPT_EXECSELERR,
	SCRIPT_FETCHERR,
	SCRIPT_PREPFETCHERR,
	SCRIPT_FETCHSIZES,
	SCRIPT_PREPFETCHSIZES,
	SCRIPT_REEXECROWS,
	SCRIPT_BIGFETCH,
	SCRIPT_NORESET,
	SCRIPT_URGENT
};

static const char	*ojdbc14scriptnames[]={
	"select1",
	"rows25",
	"twoqueries",
	"reuse",
	"prepared",
	"error",
	"longtext",
	"newline",
	"types",
	"null",
	"dml",
	"dmlauto",
	"commitfirst",
	"execerr",
	"prepexecerr",
	"execselerr",
	"fetcherr",
	"prepfetcherr",
	"fetchsizes",
	"prepfetchsizes",
	"reexecrows",
	"bigfetch",
	"noreset",
	"urgent"
};

// select 1 from dual, its fetch, and the close riding on the logoff
static void runSelect1(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t	c=ojdbc14Select1(s,"select 1 from dual",
					"select 1 from dual",4,NULL,0);
	open[(*count)++]=c;
}

// three fetches of 10, 10 and 5 rows: retcode 0 and the rows so far until
// the last, which ends in ORA-01403
static void runRows25(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"LEVEL"}
	};
	const char	*what="select level from dual connect by level <= 25";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
			"select level from dual connect by level <= 25",
			AL8I4_QUERY,NULL,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	open[(*count)++]=c;

	char		numbers[25][2];
	ojdbc14value	values[25];
	for (uint32_t i=0; i<25; i++) {
		numbers[i][0]=(char)0xc1;
		numbers[i][1]=(char)(i+2);
		values[i].bytes=numbers[i];
		values[i].size=2;
	}
	const char	*fetches[]={
		"first fetch","second fetch","third fetch"
	};
	for (uint32_t f=0; f<3; f++) {
		if (!ojdbc14Fetch(s,fetches[f],c,1)) {
			return;
		}
		uint32_t	rows=(f<2)?10:5;
		checkOjdbc14Fetch(s,fetches[f],1,values+10*f,rows,
				5+f,10*f+rows,(f<2)?0:1403,c);
	}
}

// select 1 from dual, then a close riding on a second query, which counts 1
// on the end to end field of its own: 6 for the close, 7 for the query
static void runTwoQueries(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"A"},
		{96,0,0,3,"B"}
	};
	static const ojdbc14value	row[]={{"\xc1\x03",2},{"two",3}};
	uint32_t	c1=ojdbc14Select1(s,"select 1 from dual",
					"select 1 from dual",4,NULL,0);
	const char	*what="select 2 a, 'two' b from dual";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
				"select 2 a, 'two' b from dual",
				AL8I4_QUERY,&c1,1)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,2);
	uint32_t	c2=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,7,0,0,c2,3);
	open[(*count)++]=c2;
	if (ojdbc14Fetch(s,"its fetch",c2,2)) {
		checkOjdbc14Fetch(s,"its fetch",2,row,1,8,1,1403,c2);
	}
}

// one statement run twice: the second 0x5e names the cursor the first got
// and has the whole text again, and gets the same cursor back
static void runReuse(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"7"}
	};
	static const ojdbc14value	row[]={{"\xc1\x08",2}};
	uint32_t	c=0;
	for (uint32_t i=0; i<2; i++) {
		const char	*what=(i)?"select 7 from dual, again":
						"select 7 from dual";
		if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,c,
				"select 7 from dual",AL8I4_QUERY,NULL,0)) {
			return;
		}
		checkOjdbc14Objects(s,what,"10 08 04");
		checkOjdbc14Describe(s,what,columns,1);
		uint32_t	got=checkOjdbc14ReturnParameters(s,what);
		if (i) {
			stringbuffer	l;
			ojdbc14Label(&l,s,what," gets the same cursor back");
			report(l.getString(),got==c);
		} else {
			open[(*count)++]=got;
		}
		c=got;
		checkOjdbc14Summary(s,what,4+2*i,0,0,c,3);
		if (ojdbc14Fetch(s,"its fetch",c,1)) {
			checkOjdbc14Fetch(s,"its fetch",1,row,1,
						5+2*i,1,1403,c);
		}
	}
}

// a prepared query with an int and a string bound, its fetch, then its
// re-execute, 0x8068, which fetches with the execute.  the re-execute's
// al8i4 asks for 10 rows and its one row of binds follows, and its answer
// is the row, the return parameters and ORA-01403, within 5 seconds.  the
// int goes to the backend as text, so the backend describes it as a
// VARCHAR2 and sends it back as "10", where a real server sends a NUMBER.
// the re-execute's row number is 1, the rows sent since that execute, as a
// real server says (MEASURED, 10745-dev-ojdbc14-prepared-realserver-*; the
// rule is step 3c's of #10750, the count starts again at every execute),
// where #10745's module counted over the cursor and said 2
static void runPrepared(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14column	columns[]={
		{1,OJDBC14_ANY,OJDBC14_ANY,OJDBC14_ANY,"N"},
		{1,OJDBC14_ANY,OJDBC14_ANY,OJDBC14_ANY,"V"}
	};
	static const ojdbc14value	row0[]={{"10",2},{"bind0",5}};
	const char	*what="select :1 n, :2 v from dual";
	if (!ojdbc14Query(s,what,OJDBC14_PREPARED,0,
				"select :1 n, :2 v from dual",AL8I4_QUERY,
				OJDBC14_BINDS_INT_STRING5,
				sizeof(OJDBC14_BINDS_INT_STRING5),2,
				OJDBC14_ROW_BIND0,sizeof(OJDBC14_ROW_BIND0),
				NULL,0,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,2);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	open[(*count)++]=c;
	if (!ojdbc14Fetch(s,"its fetch",c,2)) {
		return;
	}
	checkOjdbc14Fetch(s,"its fetch",2,row0,1,5,1,1403,c);

	what="the re-execute with fetch";
	if (!ojdbc14Query(s,what,OJDBC14_REEXECUTE_QUERY,c,NULL,
				AL8I4_REEXECUTE_QUERY,
				OJDBC14_BINDS_INT_STRING5,
				sizeof(OJDBC14_BINDS_INT_STRING5),2,
				OJDBC14_ROW_BIND1,sizeof(OJDBC14_ROW_BIND1),
				NULL,0,2,5)) {
		return;
	}
	checkOjdbc14Objects(s,what,"06 07 08 04");
	unsigned char	expected[64];
	size_t		size=0;
	putByte(expected,&size,ORA_TTC_ROW_HEADER);
	putByte(expected,&size,0x02);
	putLenPreInt(expected,&size,2);
	putZeros(expected,&size,1);
	putLenPreInt(expected,&size,OJDBC14_FETCH_ROWS);
	putZeros(expected,&size,3);
	checkOjdbc14Object(s,what," row header is exact",
					ORA_TTC_ROW_HEADER,expected,size);
	size=0;
	putByte(expected,&size,ORA_TTC_ROW_DATA);
	putShortClr(expected,&size,"11",2);
	putShortClr(expected,&size,"bind1",5);
	checkOjdbc14Object(s,what," row is 11 and bind1",
					ORA_TTC_ROW_DATA,expected,size);
	uint32_t	again=checkOjdbc14ReturnParameters(s,what);
	stringbuffer	l;
	ojdbc14Label(&l,s,what," names the same cursor");
	report(l.getString(),again==c);
	checkOjdbc14Summary(s,what,6,1,1403,c,3);
}

// ORA-00942 at the parse: a break marker and a reset marker, the client's
// reset, and then the error object alone, as a real server sends it
// (MEASURED, [0018] to [0021] of 10745-dev-ojdbc14-error-realserver-
// oracle122; #10745 sent the object bare, which ojdbc14 takes too), with the
// cursor the module opened for it, which the logoff closes along with select
// 1 from dual's.  a real server's error position here is 14, the module's 0
static void runError(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	const char	*what="select * from protocoltest10745_nonexistent";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
				"select * from protocoltest10745_nonexistent",
				AL8I4_QUERY,NULL,0)) {
		return;
	}
	uint32_t	c1=checkOjdbc14ErrorCursor(s,what,0);
	checkOjdbc14QueryError(s,what,4,0,942,c1,3,0,&MSG_00942);

	uint32_t	c2=ojdbc14Select1(s,"select 1 from dual",
					"select 1 from dual",5,NULL,0);
	open[(*count)++]=c2;
	open[(*count)++]=c1;
}

// 115 bytes whose first is 's', 0x73, which is also the length: the text
// has to arrive whole, with no length byte read off its front
static void runLongText(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	char	query[116];
	charstring::copy(query,"select 1 from dual /*");
	size_t	len=charstring::getLength(query);
	bytestring::set(query+len,' ',115-len-2);
	charstring::copy(query+115-2,"*/");

	stringbuffer	l;
	ojdbc14Label(&l,s,"the text"," is 115 bytes and starts with 0x73");
	report(l.getString(),charstring::getLength(query)==115 &&
						query[0]==0x73);

	open[(*count)++]=ojdbc14Select1(s,"select 1 from dual /* ... */",
						query,4,NULL,0);
}

// a text that starts with a newline, 0x0a, and is longer than that byte's
// value: it has to arrive whole, with no length byte read off its front, as
// in longtext - the module's case, not a capture's
static void runNewline(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	open[(*count)++]=ojdbc14Select1(s,"\\nselect 1 from dual",
					"\nselect 1 from dual",4,NULL,0);
}

// the types query of the real server capture, then a 300 byte value.  the
// values are the real servers' (MEASURED); the module's column encodings
// are its own, from the backend's describe
static void runTypes(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"N1"},
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"N2"},
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"N3"},
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"N4"},
		{1,OJDBC14_ANY,0,OJDBC14_ANY,"V"},
		{96,OJDBC14_ANY,0,OJDBC14_ANY,"C"},
		{12,OJDBC14_ANY,0,OJDBC14_ANY,"D"},
		{180,OJDBC14_ANY,9,OJDBC14_ANY,"TS"},
		{23,OJDBC14_ANY,0,OJDBC14_ANY,"R"},
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"NN"},
		{1,OJDBC14_ANY,0,OJDBC14_ANY,"NV"},
		{12,OJDBC14_ANY,0,OJDBC14_ANY,"ND"}
	};
	static const ojdbc14value	row[]={
		{"\xc1\x2b",2},
		{"\xc1\x04\x0f\x10\x5b",5},
		{"\x3e\x5e\x66",3},
		{"\x3f\x33\x66",3},
		{"abc",3},
		{"xy   ",5},
		{"\x78\x7c\x03\x05\x0e\x0f\x10",7},
		{"\x78\x7c\x03\x05\x0e\x0f\x10\x07\x5b\xca\x00",11},
		{"\xde\xad\xbe\xef",4},
		{NULL,0},
		{NULL,0},
		{NULL,0}
	};
	const char	*what="the types query";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
		"select 42 n1, 3.14159 n2, -7 n3, -0.5 n4, "
		"cast('abc' as varchar2(10)) v, "
		"cast('xy' as char(5)) c, "
		"to_date('2024-03-05 13:14:15','YYYY-MM-DD HH24:MI:SS') d, "
		"to_timestamp('2024-03-05 13:14:15.123456',"
				"'YYYY-MM-DD HH24:MI:SS.FF') ts, "
		"hextoraw('DEADBEEF') r, "
		"cast(null as number) nn, cast(null as varchar2(5)) nv, "
		"cast(null as date) nd from dual",
		AL8I4_QUERY,NULL,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,12);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	if (!ojdbc14Fetch(s,"its fetch",c,12)) {
		return;
	}
	checkOjdbc14Fetch(s,"its fetch",12,row,1,5,1,1403,c);

	// rpad('a',300,'b'), in the long form, chunked however the module
	// chunks it
	static const ojdbc14column	bigcolumns[]={
		{1,OJDBC14_ANY,0,OJDBC14_ANY,"BIG"}
	};
	what="select rpad('a',300,'b') big from dual";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
				"select rpad('a',300,'b') big from dual",
				AL8I4_QUERY,&c,1)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,bigcolumns,1);
	uint32_t	c2=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,7,0,0,c2,3);
	open[(*count)++]=c2;
	if (!ojdbc14Fetch(s,"its fetch",c2,1)) {
		return;
	}
	checkOjdbc14Objects(s,"its fetch","06 07 04");
	char	big[300];
	big[0]='a';
	bytestring::set(big+1,'b',299);
	oracleprotocolojdbc14answer	*a=s->answer;
	stringbuffer	l;
	ojdbc14Label(&l,s,"its fetch"," value is the 300 bytes");
	report(l.getString(),a->rowcount==1 && a->values[0][0].size==300 &&
		!bytestring::compare(a->valuedata+a->values[0][0].offset,
								big,300));
	checkOjdbc14Summary(s,"its fetch",8,1,1403,c2,3);
}

// select null: a VARCHAR2 of 4000 in the session's charset, and a null
// value - the module's own choice, which this pins.  a real server describes
// it 0 bytes wide and sends no value byte (selectnull captures)
static void runNull(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14column	columns[]={
		{1,OJDBC14_ANY,OJDBC14_ANY,4000,"NULL"}
	};
	static const ojdbc14value	row[]={{NULL,0}};
	const char	*what="select null from dual";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
				"select null from dual",AL8I4_QUERY,NULL,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	open[(*count)++]=c;
	if (ojdbc14Fetch(s,"its fetch",c,1)) {
		checkOjdbc14Fetch(s,"its fetch",1,row,1,5,1,1403,c);
	}
}

// autocommit off: ddl, dml, commit and rollback, with the closes riding on
// the next call where ojdbc14 put them
static void runDml(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14value	both[]={
		{"\xc1\x02",2},{"uno",3},{"\xc1\x03",2},{"two",3}
	};
	uint32_t	drop=ojdbc14FirstDrop(s,OJDBC14_STATEMENT,
						"protocoltest10745");
	uint32_t	c=ojdbc14Dml(s,"create table",OJDBC14_STATEMENT,
			"create table protocoltest10745 "
			"(id number, v varchar2(20))",5,0,1,NULL,0);
	c=ojdbc14Dml(s,"insert 1",OJDBC14_STATEMENT,
			"insert into protocoltest10745 values (1,'one')",
			7,1,2,&c,1);
	c=ojdbc14Dml(s,"insert 2",OJDBC14_STATEMENT,
			"insert into protocoltest10745 values (2,'two')",
			9,1,2,&c,1);
	c=ojdbc14Dml(s,"update",OJDBC14_STATEMENT,
			"update protocoltest10745 set v='uno' where id=1",
			11,1,6,&c,1);
	ojdbc14Transaction(s,"commit",true,13,&c,1);
	c=ojdbc14Dml(s,"delete all",OJDBC14_STATEMENT,
			"delete from protocoltest10745",14,2,7,NULL,0);
	ojdbc14Transaction(s,"rollback",false,16,&c,1);
	c=ojdbc14DmlSelect(s,17,both,2,NULL,0);
	c=ojdbc14Dml(s,"delete 2",OJDBC14_STATEMENT,
			"delete from protocoltest10745 where id=2",
			20,1,7,&c,1);
	ojdbc14Transaction(s,"commit again",true,22,&c,1);
	c=ojdbc14DmlSelect(s,23,both,1,NULL,0);
	c=ojdbc14Dml(s,"drop table",OJDBC14_STATEMENT,
			"drop table protocoltest10745",26,0,12,&c,1);
	open[(*count)++]=c;
	open[(*count)++]=drop;
}

// autocommit on: the commit option on every ddl and dml 0x5e, a prepared
// insert and its re-execute, which resends no descriptors.  the rollback
// after them has nothing to undo if the commit option committed, so the select
// still finds every row - a real server answers it "09 01 09" (MEASURED,
// 10745-dev-ojdbc14-rollbackauto-realserver-oracle102 and -oracle122)
static void runDmlAuto(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14value	rows[]={
		{"\xc1\x02",2},{"one",3},
		{"\xc1\x03",2},{"bound2",6},
		{"\xc1\x04",2},{"bound3",6}
	};
	uint32_t	drop=ojdbc14FirstDrop(s,OJDBC14_STATEMENT_COMMIT,
						"protocoltest10745");
	uint32_t	c=ojdbc14Dml(s,"create table",OJDBC14_STATEMENT_COMMIT,
			"create table protocoltest10745 "
			"(id number, v varchar2(20))",5,0,1,NULL,0);
	c=ojdbc14Dml(s,"insert 1",OJDBC14_STATEMENT_COMMIT,
			"insert into protocoltest10745 values (1,'one')",
			7,1,2,&c,1);

	const char	*what="prepared insert";
	if (!ojdbc14Query(s,what,OJDBC14_PREPARED_COMMIT,0,
			"insert into protocoltest10745 values (:1,:2)",
			AL8I4_DML,OJDBC14_BINDS_INT_STRING6,
			sizeof(OJDBC14_BINDS_INT_STRING6),2,
			OJDBC14_ROW_BOUND2,sizeof(OJDBC14_ROW_BOUND2),
			&c,1,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"08 04");
	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,9,1,0,c,2);

	what="prepared insert re-execute";
	if (!ojdbc14Query(s,what,OJDBC14_REEXECUTE_DML,c,NULL,
				AL8I4_REEXECUTE_DML,NULL,0,0,
				OJDBC14_ROW_BOUND3,sizeof(OJDBC14_ROW_BOUND3),
				NULL,0,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"08 04");
	uint32_t	again=checkOjdbc14ReturnParameters(s,what);
	stringbuffer	l;
	ojdbc14Label(&l,s,what," names the same cursor");
	report(l.getString(),again==c);
	checkOjdbc14Summary(s,what,10,1,0,c,2);

	ojdbc14Transaction(s,"rollback",false,11,NULL,0);
	c=ojdbc14DmlSelect(s,13,rows,3,&c,1);
	c=ojdbc14Dml(s,"drop table",OJDBC14_STATEMENT_COMMIT,
			"drop table protocoltest10745",16,0,12,&c,1);
	open[(*count)++]=c;
	open[(*count)++]=drop;
}

// a commit before any 0x5e, which already takes the 8i status shape and
// counts on the end to end field: 4 (MEASURED, 10745-dev-ojdbc14-commitfirst-
// realserver-oracle102 and -oracle122: "09 01 04")
static void runCommitFirst(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	ojdbc14Transaction(s,"commit first",true,4,NULL,0);
	open[(*count)++]=ojdbc14Select1(s,"select 1 from dual",
					"select 1 from dual",5,NULL,0);
}

// ---- #10750: errors, fetch sizes, re-execute row counts, cancel ----

// the table the #10750 dml sessions use, dropped first, as
// ojdbc14FirstDrop() does, and at the end
static const char	*CREATE_10750=
	"create table protocoltest10750 (id number constraint "
	"protocoltest10750_pk primary key, nn varchar2(10) not null, "
	"v varchar2(5))";
static const char	*INSERT_10750=
	"insert into protocoltest10750 values (:1, :2, :3)";

// a non-negative integer as an oracle NUMBER, the way setInt() sends it: an
// exponent byte, 192 plus the count of base 100 digits ahead of the point,
// then those digits each plus one, trailing zero digits dropped.  0 is a lone
// 0x80.  25 is c1 1a (MEASURED, the bind value of 10750-dev-ojdbc14-
// reexecrows-realserver-oracle122) and 3 is c1 04 ([0041] of 10750-dev-
// ojdbc14-prepfetchsizes-realserver-oracle122).  70005 is c3 08 01 06 by the
// same rule
static void putOracleNumber(unsigned char *buffer, size_t *size,
							uint32_t value) {
	if (!value) {
		putByte(buffer,size,0x80);
		return;
	}
	unsigned char	digits[5];
	size_t		count=0;
	for (uint32_t v=value; v; v/=100) {
		digits[count++]=(unsigned char)(v%100);
	}
	size_t	last=0;
	while (!digits[last]) {
		last++;
	}
	putByte(buffer,size,(unsigned char)(192+count));
	for (size_t i=count; i>last; i--) {
		putByte(buffer,size,(unsigned char)(digits[i-1]+1));
	}
}

// the value of a NUMBER column the module sends: putOracleNumber()'s bytes
static void setOjdbc14Number(ojdbc14value *v, char *bytes, uint32_t value) {
	size_t	size=0;
	putOracleNumber((unsigned char *)bytes,&size,value);
	v->bytes=bytes;
	v->size=size;
}

// one row of bind values behind its 07: "number" as setInt() sends it, and
// "string1" and "string2", if not NULL, as setString() sends them
static size_t putOjdbc14BindRow(unsigned char *buffer, uint32_t number,
						const char *string1,
						const char *string2) {
	size_t	size=0;
	putByte(buffer,&size,ORA_TTC_ROW_DATA);
	unsigned char	n[8];
	size_t		nsize=0;
	putOracleNumber(n,&nsize,number);
	putByte(buffer,&size,(unsigned char)nsize);
	bytestring::copy(buffer+size,n,nsize);
	size+=nsize;
	if (string1) {
		putShortClr(buffer,&size,string1,
				charstring::getLength(string1));
	}
	if (string2) {
		putShortClr(buffer,&size,string2,
				charstring::getLength(string2));
	}
	return size;
}

// the end to end field of the next call: one more for the call, and one
// more for the cursors closed in front of it (MEASURED rule, #10745 and step
// 3a of #10750; the markers of an error count none)
static uint32_t nextEndToEnd(uint32_t *endtoend, uint32_t closecount) {
	*endtoend+=1+((closecount)?1:0);
	return *endtoend;
}

// a plain statement that fails at its execute: the markers, then the error
// object alone, naming the cursor the module opened for it (or "cursorid"),
// with row number 0 and success iterations 0, as a real server sends them
static uint32_t ojdbc14StatementError(ojdbc14session *s, const char *what,
					const char *query,
					uint32_t options, uint32_t cursorid,
					const uint32_t *al8i4,
					const uint32_t *closecursors,
					uint32_t closecount,
					uint32_t endtoend, uint32_t oranum,
					unsigned char sqltype,
					const ojdbc14message *m) {
	if (!ojdbc14Statement(s,what,options,cursorid,query,al8i4,
						closecursors,closecount)) {
		return 0;
	}
	uint32_t	c=checkOjdbc14ErrorCursor(s,what,cursorid);
	checkOjdbc14QueryError(s,what,endtoend,0,oranum,c,sqltype,0,m);
	return c;
}

// a successful dml answer on a prepared statement's cursor: the return
// parameters naming that cursor, and one row touched
static void checkOjdbc14PreparedDml(ojdbc14session *s, const char *what,
						uint32_t endtoend,
						uint32_t cursorid) {
	checkOjdbc14Objects(s,what,"08 04");
	uint32_t	again=checkOjdbc14ReturnParameters(s,what);
	stringbuffer	l;
	ojdbc14Label(&l,s,what," names the same cursor");
	report(l.getString(),again==cursorid);
	checkOjdbc14Summary(s,what,endtoend,1,0,cursorid,2);
}

// a Statement select whose rows fail to evaluate.  where its error comes is
// the back end's to decide: it prefetches rows at the execute (fetchatonce,
// 10 by default), and whether OCIStmtExecute reports the failing row or
// leaves it to the first fetch varies with the back end session's history
// of the statement (MEASURED, both in one run of this test against HEAD's
// module and in step 3d of #10750).  so either is taken: the error at the
// 0x5e, or the 0x5e answered with the describe, the return parameters and
// no error, and the error at the first fetch of 10 rows - which is where a
// real server always puts it (MEASURED, 10750-dev-ojdbc14-execselerr- and
// -fetcherr-realserver-*).  either way the error comes behind the markers,
// alone, with row number 0 and success iterations 0, and no row reaches the
// client.  "endtoend" is the 0x5e's, and comes back as the last answer's
static uint32_t ojdbc14SelectError(ojdbc14session *s, const char *what,
					const char *query,
					const uint32_t *closecursors,
					uint32_t closecount,
					uint32_t *endtoend, uint32_t oranum,
					const ojdbc14message *m) {
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,query,AL8I4_QUERY,
						closecursors,closecount)) {
		return 0;
	}
	if (s->answer->objectcount==1) {
		uint32_t	c=checkOjdbc14ErrorCursor(s,what,0);
		checkOjdbc14QueryError(s,what,*endtoend,0,oranum,c,3,0,m);
		return c;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,*endtoend,0,0,c,3);
	stringbuffer	fetch;
	fetch.append(what)->append(", its fetch");
	if (ojdbc14Fetch(s,fetch.getString(),c,1)) {
		checkOjdbc14ErrorCursor(s,fetch.getString(),c);
		checkOjdbc14QueryError(s,fetch.getString(),
					nextEndToEnd(endtoend,0),0,oranum,c,
					3,0,m);
	}
	return c;
}

// execerr: inserts that fail at the execute with ORA-00001, ORA-01400,
// ORA-01722 and ORA-12899, sent as a Statement's 0x5e with autocommit off
// (0x8021) and on (0x8121), each followed by an insert that works.  every
// error comes behind the markers, alone, with success iterations 0, and
// the end to end field of the call after it is one more than the error's,
// plus one for the close in front of it (MEASURED, the 10750-dev-ojdbc14-
// execerr-realserver captures, from the same calls on table
// protocoltest10750e).  the cursor each error names is the one the module
// opened for the statement, and the next call closes it, as ojdbc14 does
static void runExecErr(ojdbc14session *s, uint32_t *open, uint32_t *count) {

	struct failing {
		const char		*query;
		uint32_t		oranum;
		const ojdbc14message	*m;
	};
	static const failing	failings[]={
		{"insert into protocoltest10750 values (1, 'b', 'b')",
			1,&MSG_00001},
		{"insert into protocoltest10750 values (2, null, 'b')",
			1400,&MSG_01400},
		{"insert into protocoltest10750 values ('x', 'b', 'b')",
			1722,&MSG_01722},
		{"insert into protocoltest10750 values (3, 'b', "
			"'toolongvalue')",12899,&MSG_12899}
	};

	uint32_t	drop=ojdbc14FirstDrop(s,OJDBC14_STATEMENT_COMMIT,
							"protocoltest10750");
	uint32_t	c=ojdbc14Dml(s,"create table protocoltest10750",
					OJDBC14_STATEMENT_COMMIT,CREATE_10750,
					5,0,1,NULL,0);
	c=ojdbc14Dml(s,"the first row",OJDBC14_STATEMENT_COMMIT,
			"insert into protocoltest10750 values (1, 'a', 'a')",
			7,1,2,&c,1);

	uint32_t	e=7;
	uint32_t	closecount=1;
	for (uint32_t pass=0; pass<2; pass++) {
		uint32_t	options=(pass)?OJDBC14_STATEMENT_COMMIT:
							OJDBC14_STATEMENT;
		const char	*how=(pass)?", autocommit on":
						", autocommit off";
		for (size_t i=0; i<sizeof(failings)/sizeof(failings[0]);
									i++) {
			stringbuffer	what;
			what.append(failings[i].query)->append(how);
			c=ojdbc14StatementError(s,what.getString(),
					failings[i].query,options,0,
					AL8I4_DML,&c,closecount,
					nextEndToEnd(&e,closecount),
					failings[i].oranum,2,failings[i].m);
			closecount=1;
		}
		char	good[64];
		charstring::printf(good,sizeof(good),
				"insert into protocoltest10750 values "
				"(%d, 'b', 'b')",(int)(10+pass));
		c=ojdbc14Dml(s,good,options,good,nextEndToEnd(&e,1),1,2,&c,1);
		if (!pass) {
			ojdbc14Transaction(s,"commit",true,
						nextEndToEnd(&e,1),&c,1);
			closecount=0;
		}
	}

	c=ojdbc14Dml(s,"drop table protocoltest10750",
			OJDBC14_STATEMENT_COMMIT,
			"drop table protocoltest10750",
			nextEndToEnd(&e,1),0,12,&c,1);
	open[(*count)++]=c;
	open[(*count)++]=drop;
}

// one execute of the prepared insert: the first or a re-parse (0x8029 or
// 0x8129) with the descriptors, or a re-execute (0x8020 or 0x8120) with the
// values alone, as ojdbc14 sent them (MEASURED, [0026], [0041] and [0050] of
// 10750-dev-ojdbc14-prepexecerr-realserver-oracle122): setInt(id),
// setString("b") and setString(v)
static bool ojdbc14PreparedInsert(ojdbc14session *s, const char *what,
					uint32_t options, uint32_t cursorid,
					uint32_t id, const char *v,
					const uint32_t *closecursors,
					uint32_t closecount) {
	unsigned char	row[64];
	size_t		rowsize=putOjdbc14BindRow(row,id,"b",v);
	bool		reexecute=!(options&ORA_OPTION_PARSE);
	bool		longvalue=(charstring::getLength(v)>1);
	const unsigned char	*descriptors=NULL;
	size_t			descriptorssize=0;
	if (!reexecute) {
		descriptors=(longvalue)?OJDBC14_BINDS_INT_STRING1_STRING12:
					OJDBC14_BINDS_INT_STRING1_STRING1;
		descriptorssize=(longvalue)?
				sizeof(OJDBC14_BINDS_INT_STRING1_STRING12):
				sizeof(OJDBC14_BINDS_INT_STRING1_STRING1);
	}
	return ojdbc14Query(s,what,options,cursorid,
				(reexecute)?NULL:INSERT_10750,
				(reexecute)?AL8I4_REEXECUTE_DML:AL8I4_DML,
				descriptors,descriptorssize,3,
				row,rowsize,closecursors,closecount,0,0);
}

// prepexecerr: a PreparedStatement insert whose first execute (0x8029 on
// cursor 0) fails with ORA-00001 and names the cursor the module opened,
// then the driver's re-parse on that cursor (0x8029 again, since ojdbc14
// re-parses after any error) which works, a re-execute (0x8020) that fails
// with ORA-00001, a re-parse with a value too long that fails with
// ORA-12899, and a re-parse that works, all on the one cursor; then the same
// with autocommit on (0x8129 and 0x8120).  the calls and the end to end
// fields are a real server's (MEASURED, the 10750-dev-ojdbc14-prepexecerr-
// realserver captures, which run a select 1 between the executes)
static void runPrepExecErr(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {

	uint32_t	drop=ojdbc14FirstDrop(s,OJDBC14_STATEMENT_COMMIT,
							"protocoltest10750");
	uint32_t	c=ojdbc14Dml(s,"create table protocoltest10750",
					OJDBC14_STATEMENT_COMMIT,CREATE_10750,
					5,0,1,NULL,0);
	c=ojdbc14Dml(s,"the first row",OJDBC14_STATEMENT_COMMIT,
			"insert into protocoltest10750 values (1, 'a', 'a')",
			7,1,2,&c,1);

	uint32_t	e=7;
	uint32_t	closecount=1;
	for (uint32_t pass=0; pass<2; pass++) {

		uint32_t	parse=(pass)?OJDBC14_PREPARED_COMMIT:
						OJDBC14_PREPARED;
		uint32_t	reexecute=(pass)?OJDBC14_REEXECUTE_DML:
					OJDBC14_REEXECUTE_DML_NO_COMMIT;
		const char	*how=(pass)?", autocommit on":
						", autocommit off";
		stringbuffer	what;

		what.append("the first execute, a duplicate id")->append(how);
		uint32_t	p=0;
		if (ojdbc14PreparedInsert(s,what.getString(),parse,0,1,"b",
							&c,closecount)) {
			p=checkOjdbc14ErrorCursor(s,what.getString(),0);
			checkOjdbc14QueryError(s,what.getString(),
					nextEndToEnd(&e,closecount),0,1,p,2,0,
					&MSG_00001);
		}

		what.clear();
		what.append("the re-parse on the error's cursor")->append(how);
		if (ojdbc14PreparedInsert(s,what.getString(),parse,p,
						20+pass,"b",NULL,0)) {
			checkOjdbc14PreparedDml(s,what.getString(),
						nextEndToEnd(&e,0),p);
		}

		what.clear();
		what.append("the re-execute, a duplicate id")->append(how);
		if (ojdbc14PreparedInsert(s,what.getString(),reexecute,p,
							1,"b",NULL,0)) {
			checkOjdbc14ErrorCursor(s,what.getString(),p);
			checkOjdbc14QueryError(s,what.getString(),
					nextEndToEnd(&e,0),0,1,p,2,0,
					&MSG_00001);
		}

		what.clear();
		what.append("the re-parse, a value too long")->append(how);
		if (ojdbc14PreparedInsert(s,what.getString(),parse,p,
					30+pass,"toolongvalue",NULL,0)) {
			checkOjdbc14ErrorCursor(s,what.getString(),p);
			checkOjdbc14QueryError(s,what.getString(),
					nextEndToEnd(&e,0),0,12899,p,2,0,
					&MSG_12899);
		}

		what.clear();
		what.append("the re-parse after it")->append(how);
		if (ojdbc14PreparedInsert(s,what.getString(),parse,p,
						40+pass,"b",NULL,0)) {
			checkOjdbc14PreparedDml(s,what.getString(),
						nextEndToEnd(&e,0),p);
		}

		c=p;
		if (!pass) {
			ojdbc14Transaction(s,"commit",true,
						nextEndToEnd(&e,1),&c,1);
			closecount=0;
		}
	}

	c=ojdbc14Dml(s,"drop table protocoltest10750",
			OJDBC14_STATEMENT_COMMIT,
			"drop table protocoltest10750",
			nextEndToEnd(&e,1),0,12,&c,1);
	open[(*count)++]=c;
	open[(*count)++]=drop;
}

// execselerr: selects whose only row fails to evaluate, select 1/0 and select
// to_number('x'), then select 1 - see ojdbc14SelectError() for where the
// error comes.  ojdbc14 throws from executeQuery() either way, since its
// first fetch runs inside it.  a real server's error position here is 8 and
// 17, the module's 0
static void runExecSelErr(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	uint32_t	e=4;
	uint32_t	c1=ojdbc14SelectError(s,"select 1/0 from dual",
					"select 1/0 from dual",NULL,0,
					&e,1476,&MSG_01476);
	nextEndToEnd(&e,1);
	uint32_t	c2=ojdbc14SelectError(s,
					"select to_number('x') from dual",
					"select to_number('x') from dual",
					&c1,1,&e,1722,&MSG_01722);
	open[(*count)++]=ojdbc14Select1(s,"select 1 from dual",
					"select 1 from dual",
					nextEndToEnd(&e,1),&c2,1);
}

static const char	*FETCHERR_QUERY=
	"select 1/(3-level) x from dual connect by level <= 5";

// fetcherr: a select whose third row fails to evaluate, then select 1.  at
// the default fetchatonce the back end has the first 10 rows at the
// execute, so the error comes at the 0x5e or at the first fetch, as in
// execselerr, and no row reaches the client.  at the
// fetchatonce=1 listener the 0x5e is answered with the describe, a fetch of
// 1 row gets row 1 (0.5), and the next fetch of 1 row gets the error, behind
// the markers, alone.  a real server delivers rows 1 and 2 at fetch sizes 2
// and 1 before the error (MEASURED, 10750-dev-ojdbc14-fetcherr-realserver-*),
// where the module loses row 2: the back end fetches ahead and drops the rows
// it fetched before a failing one, which no change to the protocol module
// can recover.  so what is asserted is the module's shape, the lost row with
// it, so that a fix shows up as a change to this test.  the error's row
// number is the module's, the rows it sent (1), where a real server says 2
static void runFetchErr(ojdbc14session *s, uint32_t *open, uint32_t *count,
							bool fetchatonce) {
	if (!fetchatonce) {
		uint32_t	e=4;
		uint32_t	c1=ojdbc14SelectError(s,FETCHERR_QUERY,
					FETCHERR_QUERY,NULL,0,&e,1476,
					&MSG_01476);
		open[(*count)++]=ojdbc14Select1(s,"select 1 from dual",
					"select 1 from dual",
					nextEndToEnd(&e,1),&c1,1);
		return;
	}

	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"X"}
	};
	static const ojdbc14value	half[]={{"\xc0\x33",2}};
	const char	*what=FETCHERR_QUERY;
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,FETCHERR_QUERY,
						AL8I4_QUERY,NULL,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	what="its fetch of 1 row";
	if (ojdbc14FetchRows(s,what,c,1,1,NULL,0)) {
		checkOjdbc14FetchRows(s,what,1,half,1,5,1,0,c,1);
	}
	what="its next fetch of 1 row, which fails";
	if (ojdbc14FetchRows(s,what,c,1,1,NULL,0)) {
		checkOjdbc14ErrorCursor(s,what,c);
		checkOjdbc14QueryError(s,what,6,1,1476,c,3,0,&MSG_01476);
	}
	open[(*count)++]=ojdbc14Select1(s,"select 1 from dual",
					"select 1 from dual",8,&c,1);
}

static const char	*PREPFETCHERR_QUERY=
	"select 1/(:1 - level) x from dual connect by level <= 5";

// prepfetcherr: a PreparedStatement select with setInt(10), whose rows all
// evaluate, and its fetch; then the re-execute with fetch (0x8068) with
// setInt(3), whose third row fails: the markers, then the error object
// alone, with success iterations 1, as a real server sends it (MEASURED,
// [0026] to [0029] of 10750-dev-ojdbc14-prepfetcherr-realserver-oracle122);
// then the re-parse on the same cursor that ojdbc14 sends after an error,
// with setInt(10) again, which works.  the error's row number is the module's
// 0 - the rows sent since that execute - where a real server says 2, the rows
// its cursor produced before the failing one.  the values, 1/9 to 1/5, are
// the back end's decimals and aren't compared
static void runPrepFetchErr(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"X"}
	};
	unsigned char	row10[16];
	size_t		row10size=putOjdbc14BindRow(row10,10,NULL,NULL);
	unsigned char	row3[16];
	size_t		row3size=putOjdbc14BindRow(row3,3,NULL,NULL);

	const char	*what="select 1/(:1 - level) x ..., with 10";
	if (!ojdbc14Query(s,what,OJDBC14_PREPARED,0,PREPFETCHERR_QUERY,
				AL8I4_QUERY,OJDBC14_BINDS_INT,
				sizeof(OJDBC14_BINDS_INT),1,row10,row10size,
				NULL,0,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	open[(*count)++]=c;
	if (ojdbc14Fetch(s,"its fetch",c,1)) {
		checkOjdbc14Objects(s,"its fetch","06 07 07 07 07 07 04");
		checkOjdbc14Summary(s,"its fetch",5,5,1403,c,3);
	}

	what="the re-execute with fetch, with 3";
	if (ojdbc14Query(s,what,OJDBC14_REEXECUTE_QUERY,c,NULL,
				AL8I4_REEXECUTE_QUERY,OJDBC14_BINDS_INT,
				sizeof(OJDBC14_BINDS_INT),1,row3,row3size,
				NULL,0,1,0)) {
		checkOjdbc14ErrorCursor(s,what,c);
		checkOjdbc14QueryError(s,what,6,0,1476,c,3,1,&MSG_01476);
	}

	what="the re-parse on the same cursor, with 10";
	if (!ojdbc14Query(s,what,OJDBC14_PREPARED,c,PREPFETCHERR_QUERY,
				AL8I4_QUERY,OJDBC14_BINDS_INT,
				sizeof(OJDBC14_BINDS_INT),1,row10,row10size,
				NULL,0,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	uint32_t	again=checkOjdbc14ReturnParameters(s,what);
	stringbuffer	l;
	ojdbc14Label(&l,s,what," names the same cursor");
	report(l.getString(),again==c);
	checkOjdbc14Summary(s,what,7,0,0,c,3);
	if (ojdbc14Fetch(s,"its fetch, again",c,1)) {
		checkOjdbc14Objects(s,"its fetch, again",
						"06 07 07 07 07 07 04");
		checkOjdbc14Summary(s,"its fetch, again",8,5,1403,c,3);
	}
}

static const char	*LEVEL5_QUERY=
	"select level x from dual connect by level <= 5";

// fetchsizes: a Statement select of 5 rows, executed again before each fetch
// on the same cursor, as a reused Statement is, and fetched with ojdbc14's
// fetch size 65535, 65536, 70000 and 100000: each answer's row header names
// the rows asked for, as its low 16 bits in a minimal ub2 (02 ff ff, 00,
// 02 11 70 and 02 86 a0), with the 5 rows and ORA-01403 behind it, as a real
// server sends it (MEASURED, step 3c of #10750: the header's count is the
// low 16 bits of the rows asked for, whatever number follows; with fewer
// rows than asked for at 10).  a header count in 4 bytes, which the module
// wrote before #10750 for anything over 65535, makes ojdbc14 close the
// connection with ORA-17412.  then 100001 rows: over the module's limit of
// 100000, so ORA-20004, behind the markers, alone, with success iterations
// 0, and the cursor untouched, so that a fetch of 10 rows after it gets the
// 5 rows.  a real server has no limit (MEASURED: 100001 rows in one answer);
// the limit and its error are the module's
static void runFetchSizes(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"X"}
	};
	static const uint32_t	sizes[]={65535,65536,70000,100000,100001};
	char		numbers[5][8];
	ojdbc14value	values[5];
	for (uint32_t i=0; i<5; i++) {
		setOjdbc14Number(&values[i],numbers[i],i+1);
	}

	uint32_t	c=0;
	uint32_t	e=3;
	for (size_t i=0; i<sizeof(sizes)/sizeof(sizes[0]); i++) {

		char	what[96];
		charstring::printf(what,sizeof(what),
				"%s, before a fetch of %d rows",
				LEVEL5_QUERY,(int)sizes[i]);
		if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,c,
					LEVEL5_QUERY,AL8I4_QUERY,NULL,0)) {
			return;
		}
		checkOjdbc14Objects(s,what,"10 08 04");
		checkOjdbc14Describe(s,what,columns,1);
		uint32_t	got=checkOjdbc14ReturnParameters(s,what);
		if (c) {
			stringbuffer	l;
			ojdbc14Label(&l,s,what," gets the same cursor back");
			report(l.getString(),got==c);
		} else {
			open[(*count)++]=got;
		}
		c=got;
		checkOjdbc14Summary(s,what,nextEndToEnd(&e,0),0,0,c,3);

		charstring::printf(what,sizeof(what),
				"its fetch of %d rows",(int)sizes[i]);
		if (!ojdbc14FetchRows(s,what,c,1,sizes[i],NULL,0)) {
			return;
		}
		if (sizes[i]<=100000) {
			checkOjdbc14FetchRows(s,what,1,values,5,
						nextEndToEnd(&e,0),5,1403,c,
						sizes[i]);
			continue;
		}
		checkOjdbc14ErrorCursor(s,what,c);
		checkOjdbc14QueryError(s,what,nextEndToEnd(&e,0),0,20004,c,3,
							0,&MSG_20004);
	}

	const char	*what="a fetch of 10 rows after the ORA-20004";
	if (ojdbc14Fetch(s,what,c,1)) {
		checkOjdbc14Fetch(s,what,1,values,5,nextEndToEnd(&e,0),
								5,1403,c);
	}
}

static const char	*LEVELBIND_QUERY=
	"select level x from dual connect by level <= :1";

// prepfetchsizes: a PreparedStatement select of 5 rows, setInt(5), and its
// fetch; then the re-execute with fetch (0x8068) asking for 70000 rows in
// its al8i4 vector, as ojdbc14 does at fetch size 70000 (MEASURED, packet
// [0041] of 10750-dev-ojdbc14-prepfetchsizes-realserver-oracle122 and its
// answer [0042], which holds 3 rows where this holds 5): the row header
// names 02 11 70, the 5 rows follow, then the return parameters and
// ORA-01403 with row number 5, the rows of that execute.  then a re-execute
// with fetch asking for 100001: ORA-20004 before anything runs, behind the
// markers, alone, where the module before #10750 ran it and sent up to
// 100000 rows.  its success iterations are 1, the rule for an error
// answering a call that executes and fetches.  ojdbc14 itself can't send it
// (INFERRED: a fetch size change makes it re-parse, and the fetch after the
// first execute already gets ORA-20004), so that answer is the module's
// alone.  then the re-parse on that cursor, which works
static void runPrepFetchSizes(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"X"}
	};
	char		numbers[5][8];
	ojdbc14value	values[5];
	for (uint32_t i=0; i<5; i++) {
		setOjdbc14Number(&values[i],numbers[i],i+1);
	}
	unsigned char	row5[16];
	size_t		row5size=putOjdbc14BindRow(row5,5,NULL,NULL);

	const char	*what="select level x ... <= :1, with 5";
	if (!ojdbc14Query(s,what,OJDBC14_PREPARED,0,LEVELBIND_QUERY,
				AL8I4_QUERY,OJDBC14_BINDS_INT,
				sizeof(OJDBC14_BINDS_INT),1,row5,row5size,
				NULL,0,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	open[(*count)++]=c;
	if (ojdbc14Fetch(s,"its fetch",c,1)) {
		checkOjdbc14Fetch(s,"its fetch",1,values,5,5,5,1403,c);
	}

	what="the re-execute with fetch of 70000 rows";
	if (ojdbc14Query(s,what,OJDBC14_REEXECUTE_QUERY,c,NULL,
				AL8I4_REEXECUTE_70000,OJDBC14_BINDS_INT,
				sizeof(OJDBC14_BINDS_INT),1,row5,row5size,
				NULL,0,1,0)) {
		checkOjdbc14Objects(s,what,"06 07 07 07 07 07 08 04");
		unsigned char	expected[16];
		size_t		size=0;
		putOjdbc14RowHeader(expected,&size,1,70000);
		checkOjdbc14Object(s,what," row header names 02 11 70",
					ORA_TTC_ROW_HEADER,expected,size);
		oracleprotocolojdbc14answer	*a=s->answer;
		bool	ok=(a->rowcount==5);
		for (size_t r=0; ok && r<5; r++) {
			ok=(a->values[r][0].size==values[r].size &&
				!bytestring::compare(
					a->valuedata+a->values[r][0].offset,
					values[r].bytes,values[r].size));
		}
		stringbuffer	l;
		ojdbc14Label(&l,s,what," rows are 1 to 5");
		report(l.getString(),ok);
		uint32_t	again=checkOjdbc14ReturnParameters(s,what);
		ojdbc14Label(&l,s,what," names the same cursor");
		report(l.getString(),again==c);
		checkOjdbc14Summary(s,what,6,5,1403,c,3);
	}

	what="the re-execute with fetch of 100001 rows";
	if (ojdbc14Query(s,what,OJDBC14_REEXECUTE_QUERY,c,NULL,
				AL8I4_REEXECUTE_100001,OJDBC14_BINDS_INT,
				sizeof(OJDBC14_BINDS_INT),1,row5,row5size,
				NULL,0,1,0)) {
		checkOjdbc14ErrorCursor(s,what,c);
		checkOjdbc14QueryError(s,what,7,0,20004,c,3,1,&MSG_20004);
	}

	what="the re-parse on the same cursor";
	if (!ojdbc14Query(s,what,OJDBC14_PREPARED,c,LEVELBIND_QUERY,
				AL8I4_QUERY,OJDBC14_BINDS_INT,
				sizeof(OJDBC14_BINDS_INT),1,row5,row5size,
				NULL,0,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	uint32_t	again=checkOjdbc14ReturnParameters(s,what);
	stringbuffer	l;
	ojdbc14Label(&l,s,what," names the same cursor");
	report(l.getString(),again==c);
	checkOjdbc14Summary(s,what,8,0,0,c,3);
	if (ojdbc14Fetch(s,"its fetch, again",c,1)) {
		checkOjdbc14Fetch(s,"its fetch, again",1,values,5,9,5,1403,c);
	}
}

// reexecrows: a PreparedStatement select of 25 rows, setInt(25), executed
// three times at fetch size 10: the first execute (0x8029) and three fetches,
// then twice the re-execute with fetch (0x8068) and two fetches.  the row
// number in each answer is the rows sent since that execute - 10, 20 and 25
// every time - and the end to end fields run 4 to 13, as a real server's
// (MEASURED, 10750-dev-ojdbc14-reexecrows-realserver-oracle102 and
// -oracle122), where the module before #10750 counted on over the cursor,
// 35, 45 and 50, then 60, 70 and 75.  ojdbc14 counts a select's rows from
// the rows it reads, so it never reads this number
static void runReexecRows(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"N"}
	};
	char		numbers[25][8];
	ojdbc14value	values[25];
	for (uint32_t i=0; i<25; i++) {
		setOjdbc14Number(&values[i],numbers[i],i+1);
	}
	unsigned char	row25[16];
	size_t		row25size=putOjdbc14BindRow(row25,25,NULL,NULL);

	const char	*what="select level n ... <= :1, with 25";
	if (!ojdbc14Query(s,what,OJDBC14_PREPARED,0,
			"select level n from dual connect by level <= :1",
			AL8I4_QUERY,OJDBC14_BINDS_INT,
			sizeof(OJDBC14_BINDS_INT),1,row25,row25size,
			NULL,0,0,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	open[(*count)++]=c;

	uint32_t	e=4;
	for (uint32_t x=0; x<3; x++) {

		uint32_t	first=0;
		if (x) {
			char	reexecute[64];
			charstring::printf(reexecute,sizeof(reexecute),
					"re-execute with fetch %d",(int)x);
			if (!ojdbc14Query(s,reexecute,OJDBC14_REEXECUTE_QUERY,
					c,NULL,AL8I4_REEXECUTE_QUERY,
					OJDBC14_BINDS_INT,
					sizeof(OJDBC14_BINDS_INT),1,
					row25,row25size,NULL,0,1,0)) {
				return;
			}
			checkOjdbc14Objects(s,reexecute,
				"06 07 07 07 07 07 07 07 07 07 07 08 04");
			unsigned char	expected[16];
			size_t		size=0;
			putOjdbc14RowHeader(expected,&size,1,
						OJDBC14_FETCH_ROWS);
			checkOjdbc14Object(s,reexecute," row header is exact",
					ORA_TTC_ROW_HEADER,expected,size);
			oracleprotocolojdbc14answer	*a=s->answer;
			bool	ok=(a->rowcount==10);
			for (size_t r=0; ok && r<10; r++) {
				ok=(a->values[r][0].size==values[r].size &&
				!bytestring::compare(
					a->valuedata+a->values[r][0].offset,
					values[r].bytes,values[r].size));
			}
			stringbuffer	l;
			ojdbc14Label(&l,s,reexecute," rows are 1 to 10");
			report(l.getString(),ok);
			uint32_t	again=
				checkOjdbc14ReturnParameters(s,reexecute);
			ojdbc14Label(&l,s,reexecute," names the same cursor");
			report(l.getString(),again==c);
			checkOjdbc14Summary(s,reexecute,nextEndToEnd(&e,0),
								10,0,c,3);
			first=1;
		}

		for (uint32_t f=first; f<3; f++) {
			char	fetch[64];
			charstring::printf(fetch,sizeof(fetch),
					"execute %d, fetch %d",
					(int)x+1,(int)f+1);
			if (!ojdbc14Fetch(s,fetch,c,1)) {
				return;
			}
			uint32_t	rows=(f<2)?10:5;
			checkOjdbc14Fetch(s,fetch,1,values+10*f,rows,
					nextEndToEnd(&e,0),10*f+rows,
					(f<2)?0:1403,c);
		}
	}
}

// bigfetch, 12.1 only: a fetch of 65536 rows, the first count over 65535,
// all in one answer: a row header naming 00, the low 16 bits (MEASURED, [0028]
// of 10750-dev-ojdbc14-fetchsizes-realserver-oracle122), 65536 rows counted,
// and a row number of 65536 in its minimal form, 03 01 00 00; then the next
// fetch, the last 5 rows and ORA-01403 with row number 65541, 03 01 00 05.
// The row numbers are MEASURED in the fetchbig captures of the real servers,
// which are not checked in; the README in this directory quotes them
static void runBigFetch(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	static const ojdbc14column	columns[]={
		{2,OJDBC14_ANY,OJDBC14_ANY,22,"X"}
	};
	const char	*what="select level x from dual connect by level <= "
								"65541";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
			"select level x from dual connect by level <= 65541",
			AL8I4_QUERY,NULL,0)) {
		return;
	}
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14Describe(s,what,columns,1);
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,4,0,0,c,3);
	open[(*count)++]=c;

	what="its fetch of 65536 rows";
	if (!ojdbc14FetchRows(s,what,c,1,65536,NULL,0)) {
		return;
	}
	unsigned char	expected[16];
	size_t		size=0;
	putOjdbc14RowHeader(expected,&size,1,65536);
	checkOjdbc14Object(s,what," row header names 00",
					ORA_TTC_ROW_HEADER,expected,size);
	oracleprotocolojdbc14answer	*a=s->answer;
	stringbuffer	l;
	ojdbc14Label(&l,s,what," has 65536 rows, the first 1 and the 32nd 32");
	char	n1[8];
	char	n32[8];
	ojdbc14value	v1;
	ojdbc14value	v32;
	setOjdbc14Number(&v1,n1,1);
	setOjdbc14Number(&v32,n32,32);
	const oracleprotocolojdbc14value	*first=&a->values[0][0];
	const oracleprotocolojdbc14value	*last=&a->values[31][0];
	bool	ok=(a->totalrowcount==65536 &&
			a->totalobjectcount==65538 &&
			a->rowcount==ORA_8I_MAX_ROWS &&
			first->size==v1.size &&
			!bytestring::compare(a->valuedata+first->offset,
							v1.bytes,v1.size) &&
			last->size==v32.size &&
			!bytestring::compare(a->valuedata+last->offset,
							v32.bytes,v32.size));
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %d rows, %d objects\n",
					(int)a->totalrowcount,
					(int)a->totalobjectcount);
	}
	checkOjdbc14Summary(s,what,5,65536,0,c,3);

	char		numbers[5][8];
	ojdbc14value	values[5];
	for (uint32_t i=0; i<5; i++) {
		setOjdbc14Number(&values[i],numbers[i],65537+i);
	}
	what="its next fetch of 65536 rows";
	if (ojdbc14FetchRows(s,what,c,1,65536,NULL,0)) {
		checkOjdbc14FetchRows(s,what,1,values,5,6,65541,1403,c,65536);
	}
}

// noreset, 12.1 only: a client that answers the break ahead of an error - an
// ORA-00942, which always comes at the 0x5e - with its next call, a data
// packet, where its reset marker belongs.  ojdbc14
// never does that (CROSS-CHECK, NSProtocol.sendReset()), so this is the
// module's choice: the listener ends the session and closes the connection
// rather than answer a call out of step, and doesn't hang waiting for a
// reset that won't come
static void runNoReset(ojdbc14session *s) {

	stringbuffer		l;
	oracleprotocolclient	*client=s->client;
	oracleprotocolojdbc14query	q;
	bytestring::zero(&q,sizeof(q));
	q.options=OJDBC14_STATEMENT;
	q.query="select * from protocoltest10750_nonexistent";
	q.al8i4=AL8I4_QUERY;
	client->buildOjdbc14Query(&q);

	const char	*what=q.query;
	bool	ok=(client->sendPacket() && client->recvPacket() &&
			client->getPacketType()==ORA_PACKET_MARKER &&
			client->getResponseSize()==3 &&
			client->getResponse()[2]==ORA_MARKER_TYPE_BREAK);
	ojdbc14Label(&l,s,what," answer starts with a break marker");
	report(l.getString(),ok);
	if (!ok) {
		dumpBytes("got",client->getResponse(),
					client->getResponseSize());
		s->ok=false;
		return;
	}
	ok=(client->recvPacket() &&
		client->getPacketType()==ORA_PACKET_MARKER &&
		client->getResponseSize()==3 &&
		client->getResponse()[2]==ORA_MARKER_TYPE_RESET);
	ojdbc14Label(&l,s,what," reset marker follows");
	report(l.getString(),ok);
	if (!ok) {
		s->ok=false;
		return;
	}

	q.query="select 1 from dual";
	client->buildOjdbc14Query(&q);
	datetime	dt;
	dt.initFromSystemDateTime();
	time_t		start=dt.getEpoch();
	bool		answered=(client->sendPacket() && client->recvPacket());
	dt.initFromSystemDateTime();
	time_t		elapsed=dt.getEpoch()-start;
	ojdbc14Label(&l,s,"select 1 from dual, sent where the reset belongs,",
				" gets the connection closed, not an answer");
	report(l.getString(),!answered && elapsed<DROP_SECONDS);
	if (answered) {
		dumpBytes("answer",client->getResponse(),
					client->getResponseSize());
	} else if (elapsed>=DROP_SECONDS) {
		stdoutput.printf("    no answer, but no close for %d seconds\n",
								(int)elapsed);
	}
	s->ok=false;
}

// urgent, 12.1 only: ojdbc14 cancels a call - Statement.cancel(), or a query
// timeout - with one byte of tcp urgent data, 0x21, and nothing in band
// (MEASURED, step 3b of #10750).  a real server answers it with a reset
// marker, takes the client's reset, and fails the call with ORA-01013.  the
// module doesn't read urgent data: the call it lands behind runs to the end
// and is answered as usual, and so is the next one, with the end to end
// field in step (the module's choice, documented in #10750; the back end
// can't interrupt a statement anyway).  this pins that the byte, right
// behind a call and between two calls, does no harm: no answer goes out of
// step and nothing hangs
static void runUrgent(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	s->urgent=true;
	uint32_t	c1=ojdbc14Select1(s,"select 1 from dual, urgent data "
					"right behind its call",
					"select 1 from dual",4,NULL,0);
	if (s->ok) {
		stringbuffer	l;
		ojdbc14Label(&l,s,"urgent data between two calls",
							" is sent");
		bool	sent=s->client->sendUrgentByte(0x21);
		report(l.getString(),sent);
		s->ok=sent;
	}
	open[(*count)++]=ojdbc14Select1(s,"select 1 from dual, after urgent "
					"data","select 1 from dual",7,&c1,1);
}

// one query session: ojdbc14's login, the script, then the logoff with every
// cursor the script left open closed in front of it, answered "09 00"
static void runOjdbc14QuerySession(size_t listener, ojdbc14script script) {

	const listenerinfo	*li=&listeners[listener];
	const char		*user=(listener==LISTENER_CONNECTSTRINGS ||
					listener==LISTENER_FETCHATONCE)?
							"testuser":"TESTUSER";

	char	label[256];
	charstring::printf(label,sizeof(label),
			"%s, ojdbc14 %s session",li->name,
			ojdbc14scriptnames[script]);
	stdoutput.printf("\n%s - expect the answers ojdbc14 reads\n",label);

	oracleprotocolclient	client;
	if (!requestOjdbc14Challenge(&client,label,li,user) ||
				!checkOjdbc14Challenge(&client,label)) {
		return;
	}
	stringbuffer	l;
	l.append(label)->append(" logs in");
	bool	loggedin=client.answerChallenge8i(user,"testpassword");
	report(l.getString(),loggedin);
	if (!loggedin) {
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	oracleprotocolojdbc14answer	*answer=
					new oracleprotocolojdbc14answer;
	ojdbc14session	s;
	s.client=&client;
	s.label=label;
	s.answer=answer;
	s.ok=true;
	s.urgent=false;

	uint32_t	open[8];
	uint32_t	count=0;
	switch (script) {
		case SCRIPT_SELECT1:
			runSelect1(&s,open,&count);
			break;
		case SCRIPT_ROWS25:
			runRows25(&s,open,&count);
			break;
		case SCRIPT_TWOQUERIES:
			runTwoQueries(&s,open,&count);
			break;
		case SCRIPT_REUSE:
			runReuse(&s,open,&count);
			break;
		case SCRIPT_PREPARED:
			runPrepared(&s,open,&count);
			break;
		case SCRIPT_ERROR:
			runError(&s,open,&count);
			break;
		case SCRIPT_LONGTEXT:
			runLongText(&s,open,&count);
			break;
		case SCRIPT_NEWLINE:
			runNewline(&s,open,&count);
			break;
		case SCRIPT_TYPES:
			runTypes(&s,open,&count);
			break;
		case SCRIPT_NULL:
			runNull(&s,open,&count);
			break;
		case SCRIPT_DML:
			runDml(&s,open,&count);
			break;
		case SCRIPT_DMLAUTO:
			runDmlAuto(&s,open,&count);
			break;
		case SCRIPT_COMMITFIRST:
			runCommitFirst(&s,open,&count);
			break;
		case SCRIPT_EXECERR:
			runExecErr(&s,open,&count);
			break;
		case SCRIPT_PREPEXECERR:
			runPrepExecErr(&s,open,&count);
			break;
		case SCRIPT_EXECSELERR:
			runExecSelErr(&s,open,&count);
			break;
		case SCRIPT_FETCHERR:
			runFetchErr(&s,open,&count,
					listener==LISTENER_FETCHATONCE);
			break;
		case SCRIPT_PREPFETCHERR:
			runPrepFetchErr(&s,open,&count);
			break;
		case SCRIPT_FETCHSIZES:
			runFetchSizes(&s,open,&count);
			break;
		case SCRIPT_PREPFETCHSIZES:
			runPrepFetchSizes(&s,open,&count);
			break;
		case SCRIPT_REEXECROWS:
			runReexecRows(&s,open,&count);
			break;
		case SCRIPT_BIGFETCH:
			runBigFetch(&s,open,&count);
			break;
		case SCRIPT_NORESET:
			runNoReset(&s);
			break;
		case SCRIPT_URGENT:
			runUrgent(&s,open,&count);
			break;
	}
	delete answer;

	if (!s.ok) {
		return;
	}

	l.clear();
	l.append(label)->append(", the close and the logoff are answered "
							"with \"09 00\"");
	bool	ok=(client.logoff8i(open,count) &&
			client.getResponseSize()==
					sizeof(ORA_OJDBC14_LOGOFF_ANSWER) &&
			!bytestring::compare(client.getResponse(),
					ORA_OJDBC14_LOGOFF_ANSWER,
					sizeof(ORA_OJDBC14_LOGOFF_ANSWER)));
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client.getError());
		dumpBytes("answer",client.getResponse(),
					client.getResponseSize());
	}
}

// the builders against ojdbc14's own calls, before any connection
static void checkOjdbc14CallBuilders() {

	stdoutput.printf("\nojdbc14 call builders - expect each real "
						"client's bytes\n");

	oracleprotocolclient	client;
	oracleprotocolojdbc14query	q;
	uint32_t			one=1;
	uint32_t			five=5;
	uint32_t			six=6;
	uint32_t			threeone[]={3,1};

	struct {
		const char		*capture;
		const unsigned char	*bytes;
		size_t			size;
	} vectors[]={
		{"[0017] of 10745-dev-ojdbc14-select1-realserver-oracle122",
			OJDBC14_CALL_SELECT1,sizeof(OJDBC14_CALL_SELECT1)},
		{"[0019] of 10745-dev-ojdbc14-select1-realserver-oracle122",
			OJDBC14_CALL_FETCH,sizeof(OJDBC14_CALL_FETCH)},
		{"[0021] of 10745-dev-ojdbc14-select1-realserver-oracle122",
			OJDBC14_CALL_CLOSE_LOGOFF,
			sizeof(OJDBC14_CALL_CLOSE_LOGOFF)},
		{"[0017] of 10745-dev-ojdbc14-prepared-realserver-oracle122",
			OJDBC14_CALL_PREPARED,sizeof(OJDBC14_CALL_PREPARED)},
		{"[0021] of 10745-dev-ojdbc14-prepared-realserver-oracle122",
			OJDBC14_CALL_REEXECUTE_QUERY,
			sizeof(OJDBC14_CALL_REEXECUTE_QUERY)},
		{"[0026] of 10745-dev-ojdbc14-dmlauto-realserver-oracle122",
			OJDBC14_CALL_CLOSE_PREPARED_DML,
			sizeof(OJDBC14_CALL_CLOSE_PREPARED_DML)},
		{"[0028] of 10745-dev-ojdbc14-dmlauto-realserver-oracle122",
			OJDBC14_CALL_REEXECUTE_DML,
			sizeof(OJDBC14_CALL_REEXECUTE_DML)},
		{"[0030] of 10745-dev-ojdbc14-dml-realserver-oracle122",
			OJDBC14_CALL_CLOSE_COMMIT,
			sizeof(OJDBC14_CALL_CLOSE_COMMIT)},
		{"[0034] of 10745-dev-ojdbc14-dml-realserver-oracle122",
			OJDBC14_CALL_CLOSE_ROLLBACK,
			sizeof(OJDBC14_CALL_CLOSE_ROLLBACK)},
		{"[0026] of 10745-dev-ojdbc14-error-realserver-oracle122",
			OJDBC14_CALL_CLOSE2_LOGOFF,
			sizeof(OJDBC14_CALL_CLOSE2_LOGOFF)},
		{"[0019] of 10750-dev-ojdbc14-fetchsizes-realserver-oracle102",
			OJDBC14_CALL_FETCH_65535,
			sizeof(OJDBC14_CALL_FETCH_65535)},
		{"[0027] of 10750-dev-ojdbc14-fetchsizes-realserver-oracle102",
			OJDBC14_CALL_FETCH_65536,
			sizeof(OJDBC14_CALL_FETCH_65536)},
		{"[0035] of 10750-dev-ojdbc14-fetchsizes-realserver-oracle102",
			OJDBC14_CALL_FETCH_70000,
			sizeof(OJDBC14_CALL_FETCH_70000)},
		{"[0043] of 10750-dev-ojdbc14-fetchsizes-realserver-oracle102",
			OJDBC14_CALL_FETCH_100001,
			sizeof(OJDBC14_CALL_FETCH_100001)},
		{"[0041] of 10750-dev-ojdbc14-prepfetchsizes-realserver-"
							"oracle122",
			OJDBC14_CALL_REEXECUTE_70000,
			sizeof(OJDBC14_CALL_REEXECUTE_70000)},
		{"[0017] of 10750-dev-ojdbc14-reexecrows-realserver-oracle122",
			OJDBC14_CALL_PREPARED_LEVEL,
			sizeof(OJDBC14_CALL_PREPARED_LEVEL)},
		{"[0025] of 10750-dev-ojdbc14-reexecrows-realserver-oracle122",
			OJDBC14_CALL_REEXECUTE_10,
			sizeof(OJDBC14_CALL_REEXECUTE_10)},
		{"[0026] of 10750-dev-ojdbc14-prepexecerr-realserver-oracle122",
			OJDBC14_CALL_CLOSE_PREPARED_INSERT3,
			sizeof(OJDBC14_CALL_CLOSE_PREPARED_INSERT3)},
		{"[0041] of 10750-dev-ojdbc14-prepexecerr-realserver-oracle122",
			OJDBC14_CALL_CLOSE_REEXECUTE_INSERT3,
			sizeof(OJDBC14_CALL_CLOSE_REEXECUTE_INSERT3)},
		{"[0050] of 10750-dev-ojdbc14-prepexecerr-realserver-oracle122",
			OJDBC14_CALL_CLOSE_REPARSE_INSERT3,
			sizeof(OJDBC14_CALL_CLOSE_REPARSE_INSERT3)},
		{"[0017] of 10750-dev-ojdbc14-prepfetcherr-realserver-"
							"oracle122",
			OJDBC14_CALL_PREPARED_DIVIDE,
			sizeof(OJDBC14_CALL_PREPARED_DIVIDE)},
		{"[0025] of 10750-dev-ojdbc14-prepfetcherr-realserver-"
							"oracle122",
			OJDBC14_CALL_CLOSE_REEXECUTE_DIVIDE,
			sizeof(OJDBC14_CALL_CLOSE_REEXECUTE_DIVIDE)},
		{"[0026] of 10750-dev-ojdbc14-execerr-realserver-oracle122",
			OJDBC14_CALL_CLOSE_INSERT,
			sizeof(OJDBC14_CALL_CLOSE_INSERT)}
	};
	uint32_t	three=3;
	uint32_t	four=4;
	unsigned char	row[64];

	for (size_t i=0; i<sizeof(vectors)/sizeof(vectors[0]); i++) {

		bytestring::zero(&q,sizeof(q));
		switch (i) {
			case 0:
				q.options=OJDBC14_STATEMENT;
				q.query="select 1 from dual";
				q.al8i4=AL8I4_QUERY;
				client.buildOjdbc14Query(&q);
				break;
			case 1:
				client.buildOjdbc14Fetch(1,OJDBC14_FETCH_ROWS);
				break;
			case 2:
				client.buildOjdbc14Call(ORA_TTI_DISCONNECT,
								&one,1);
				break;
			case 3:
			case 4:
				q.options=(i==3)?OJDBC14_PREPARED:
						OJDBC14_REEXECUTE_QUERY;
				q.cursorid=(i==3)?0:1;
				q.query=(i==3)?"select :1 n, :2 v from dual":
									NULL;
				q.al8i4=(i==3)?AL8I4_QUERY:
						AL8I4_REEXECUTE_QUERY;
				q.bindcount=2;
				q.descriptors=OJDBC14_BINDS_INT_STRING5;
				q.descriptorssize=
					sizeof(OJDBC14_BINDS_INT_STRING5);
				q.bindrow=(i==3)?OJDBC14_ROW_BIND0:
							OJDBC14_ROW_BIND1;
				q.bindrowsize=sizeof(OJDBC14_ROW_BIND0);
				client.buildOjdbc14Query(&q);
				break;
			case 5:
				q.options=OJDBC14_PREPARED_COMMIT;
				q.query="insert into protocoltest10745 "
							"values (:1,:2)";
				q.al8i4=AL8I4_DML;
				q.bindcount=2;
				q.descriptors=OJDBC14_BINDS_INT_STRING6;
				q.descriptorssize=
					sizeof(OJDBC14_BINDS_INT_STRING6);
				q.bindrow=OJDBC14_ROW_BOUND2;
				q.bindrowsize=sizeof(OJDBC14_ROW_BOUND2);
				client.buildOjdbc14Query(&q,&six,1);
				break;
			case 6:
				q.options=OJDBC14_REEXECUTE_DML;
				q.cursorid=5;
				q.al8i4=AL8I4_REEXECUTE_DML;
				q.bindrow=OJDBC14_ROW_BOUND3;
				q.bindrowsize=sizeof(OJDBC14_ROW_BOUND3);
				client.buildOjdbc14Query(&q);
				break;
			case 7:
				client.buildOjdbc14Call(ORA_8I_TTI_COMMIT,
								&six,1);
				break;
			case 8:
				client.buildOjdbc14Call(ORA_8I_TTI_ROLLBACK,
								&five,1);
				break;
			case 9:
				client.buildOjdbc14Call(ORA_TTI_DISCONNECT,
								threeone,2);
				break;
			case 10:
				client.buildOjdbc14Fetch(2,65535);
				break;
			case 11:
				client.buildOjdbc14Fetch(1,65536);
				break;
			case 12:
				client.buildOjdbc14Fetch(2,70000);
				break;
			case 13:
				client.buildOjdbc14Fetch(1,100001);
				break;
			case 14:
			case 16:
			case 21:
				// a prepared query's re-execute with fetch
				q.options=OJDBC14_REEXECUTE_QUERY;
				q.cursorid=1;
				q.al8i4=(i==14)?AL8I4_REEXECUTE_70000:
						AL8I4_REEXECUTE_QUERY;
				q.bindcount=1;
				q.descriptors=OJDBC14_BINDS_INT;
				q.descriptorssize=sizeof(OJDBC14_BINDS_INT);
				q.bindrow=row;
				q.bindrowsize=putOjdbc14BindRow(row,
						(i==14)?70005:(i==16)?25:3,
						NULL,NULL);
				client.buildOjdbc14Query(&q,
						(i==21)?&three:NULL,
						(i==21)?1:0);
				break;
			case 15:
			case 20:
				// a prepared query's first execute
				q.options=OJDBC14_PREPARED;
				q.query=(i==15)?
					"select level n from dual connect by "
					"level <= :1":
					"select 1/(:1 - level) x from dual "
					"connect by level <= 5";
				q.al8i4=AL8I4_QUERY;
				q.bindcount=1;
				q.descriptors=OJDBC14_BINDS_INT;
				q.descriptorssize=sizeof(OJDBC14_BINDS_INT);
				q.bindrow=row;
				q.bindrowsize=putOjdbc14BindRow(row,
						(i==15)?25:10,NULL,NULL);
				client.buildOjdbc14Query(&q);
				break;
			case 17:
			case 19:
				// a prepared insert's first execute, and its
				// re-parse with a 12 character value
				q.options=OJDBC14_PREPARED;
				q.cursorid=(i==17)?0:10;
				q.query="insert into protocoltest10750p values "
							"(:1, :2, :3)";
				q.al8i4=AL8I4_DML;
				q.bindcount=3;
				if (i==17) {
					q.descriptors=
					OJDBC14_BINDS_INT_STRING1_STRING1;
					q.descriptorssize=sizeof(
					OJDBC14_BINDS_INT_STRING1_STRING1);
				} else {
					q.descriptors=
					OJDBC14_BINDS_INT_STRING1_STRING12;
					q.descriptorssize=sizeof(
					OJDBC14_BINDS_INT_STRING1_STRING12);
				}
				q.bindrow=row;
				q.bindrowsize=(i==17)?
					putOjdbc14BindRow(row,1,"b","b"):
					putOjdbc14BindRow(row,30,"b",
							"toolongvalue");
				client.buildOjdbc14Query(&q,
						(i==17)?&six:&four,1);
				break;
			case 18:
				// its re-execute, autocommit off
				q.options=OJDBC14_REEXECUTE_DML_NO_COMMIT;
				q.cursorid=10;
				q.al8i4=AL8I4_REEXECUTE_DML;
				q.bindrow=row;
				q.bindrowsize=putOjdbc14BindRow(row,1,"b","b");
				client.buildOjdbc14Query(&q,&five,1);
				break;
			case 22:
				// a statement's insert
				q.options=OJDBC14_STATEMENT;
				q.query="insert into protocoltest10750e values "
							"(1, 'b', 'b')";
				q.al8i4=AL8I4_DML;
				client.buildOjdbc14Query(&q,&four,1);
				break;
		}

		// behind the header and the data flags
		const unsigned char	*request=client.getRequest()+
							ORA_FRAGMENT_OVERHEAD;
		size_t			size=client.getRequestSize()-
							ORA_FRAGMENT_OVERHEAD;
		char	label[256];
		charstring::printf(label,sizeof(label),
				"ojdbc14 call builder reproduces %d bytes, "
				"packet %s",(int)vectors[i].size,
				vectors[i].capture);
		bool	ok=(size==vectors[i].size &&
				!bytestring::compare(request,vectors[i].bytes,
								size));
		report(label,ok);
		if (!ok) {
			dumpBytes("built",request,size);
			dumpBytes("expected",vectors[i].bytes,
							vectors[i].size);
		}
	}
}

// the error object builder against real servers' objects, field for field,
// before any connection, so that the objects the sessions expect have a real
// server's layout and differ from one only where putOjdbc14Summary() says
static void checkOjdbc14ErrorObjectBuilder() {

	stdoutput.printf("\nojdbc14 error object builder - expect real "
						"servers' objects\n");

	// the fields ahead of each message, as captured
	static const unsigned char	head00001[]={
		0x04, 0x01, 0x09, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x0a,
		0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xff, 0xff,
		0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x47
	};
	static const unsigned char	head01400[]={
		0x04, 0x01, 0x0e, 0x00, 0x02, 0x05, 0x78, 0x01, 0x01, 0x02,
		0x05, 0x78, 0x01, 0x05, 0x01, 0x2a, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x04, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a
	};
	static const unsigned char	head01476[]={
		0x04, 0x01, 0x09, 0x01, 0x02, 0x02, 0x05, 0xc4, 0x00, 0x00,
		0x01, 0x01, 0x01, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x01, 0x24
	};
	static const unsigned char	head01722[]={
		0x04, 0x01, 0x0a, 0x00, 0x02, 0x06, 0xba, 0x00, 0x00, 0x01,
		0x03, 0x01, 0x11, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a
	};

	struct {
		const char		*capture;
		const unsigned char	*head;
		size_t			headsize;
		ojdbc14oer		e;
	} vectors[]={
		{"[0030] of 10750-dev-ojdbc14-execerr-realserver-oracle122",
			head00001,sizeof(head00001),
			{9,0,1,0,0,10,0,2,0xffffffff,0,
			"ORA-00001: unique constraint "
			"(TESTUSER.PROTOCOLTEST10750E_PK) violated\n",0}},
		{"[0039] of 10750-dev-ojdbc14-execerr-realserver-oracle122",
			head01400,sizeof(head01400),
			{14,0,1400,1,1400,5,42,2,0xffffffff,0,
			"ORA-01400: cannot insert NULL into "
			"(\"TESTUSER\".\"PROTOCOLTEST10750E\".\"NN\")\n",0}},
		{"[0029] of 10750-dev-ojdbc14-prepfetcherr-realserver-"
							"oracle122",
			head01476,sizeof(head01476),
			{9,2,1476,0,0,1,8,3,0,1,
			"ORA-01476: divisor is equal to zero\n",0}},
		{"[0034] of 10750-dev-ojdbc14-execselerr-realserver-"
							"oracle122",
			head01722,sizeof(head01722),
			{10,0,1722,0,0,3,17,3,0,0,
			"ORA-01722: invalid number\n",0}}
	};

	for (size_t i=0; i<sizeof(vectors)/sizeof(vectors[0]); i++) {

		ojdbc14oer	e=vectors[i].e;
		e.messagesize=charstring::getLength(e.message);

		unsigned char	real[256];
		size_t		realsize=vectors[i].headsize;
		bytestring::copy(real,vectors[i].head,realsize);
		bytestring::copy(real+realsize,e.message,e.messagesize);
		realsize+=e.messagesize;

		unsigned char	built[256];
		size_t		builtsize=0;
		putOjdbc14Oer(built,&builtsize,&e);

		char	label[256];
		charstring::printf(label,sizeof(label),
				"ojdbc14 error object builder reproduces %d "
				"bytes, packet %s",(int)realsize,
				vectors[i].capture);
		bool	ok=(builtsize==realsize &&
				!bytestring::compare(built,real,realsize));
		report(label,ok);
		if (!ok) {
			dumpBytes("built",built,builtsize);
			dumpBytes("expected",real,realsize);
		}
	}
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

			runPreLoginVersionCall(LOGON_TYPES_NONE);
			runPreLoginVersionCall(LOGON_TYPES_9I);
			runPreLoginVersionCall(LOGON_TYPES_11G);
		}
	}

	// the genuine 10.2 client's own session.  it speaks the portable
	// encoding and offers tns version 0x0139, so it runs once
	if (nativeencoding) {
		stdoutput.printf("\ngenuine 10.2 client: skipped, it is "
					"portable only\n");
		stdoutput.printf("\nojdbc14: skipped, it is portable only\n");
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

		// genuine ojdbc14's own session, which offers tns version
		// 0x0134, after its math is checked against real logins
		clientconnectversion=ORA_CONNECT_VERSION_OJDBC14;
		negotiatedconnectversion=ORA_CONNECT_VERSION_OJDBC14;
		stdoutput.printf("\n====== ojdbc14 10.2.0.5, tns version "
					"0x%04x ======\n",
					negotiatedconnectversion);
		checkKnownAnswers();
		for (size_t i=0; i<sizeof(ojdbc14cases)/
					sizeof(ojdbc14cases[0]); i++) {
			runOjdbc14Case(&ojdbc14cases[i]);
		}
		runSecondPreLoginVersionCall();

		// and its queries, at every ceiling and through the
		// oracle_connectstrings auth module
		checkOjdbc14CallBuilders();
		checkOjdbc14ErrorObjectBuilder();
		size_t	querylisteners[]={
			LISTENER_12_1,
			LISTENER_11_2,
			LISTENER_10_2,
			LISTENER_CONNECTSTRINGS
		};
		for (size_t i=0; i<sizeof(querylisteners)/
					sizeof(querylisteners[0]); i++) {
			for (int sc=SCRIPT_SELECT1;
					sc<=SCRIPT_REEXECROWS; sc++) {
				runOjdbc14QuerySession(querylisteners[i],
							(ojdbc14script)sc);
			}
		}

		// and, for #10750, the sessions that need one listener
		// only: the fetch error at the fetchatonce=1 listener, where
		// the back end's execute leaves the error to a fetch, and
		// the 65536 row fetch, the client that sends no reset and
		// the urgent data at 12.1
		runOjdbc14QuerySession(LISTENER_FETCHATONCE,SCRIPT_FETCHERR);
		for (int sc=SCRIPT_BIGFETCH; sc<=SCRIPT_URGENT; sc++) {
			runOjdbc14QuerySession(LISTENER_12_1,
						(ojdbc14script)sc);
		}
	}

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
