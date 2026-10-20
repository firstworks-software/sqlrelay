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
// that answers its logoff.  The version answer at 12.1 and 11.2 is the one a
// real 11.2 and 12.2 server send ojdbc14, which names 11.1.0.0.0 in place of
// their own version, byte for byte (packet [0012] of 10750-dev-ojdbc14-
// select1-realserver-oracle112 and 10545-dev-ojdbc14-login-select1-
// realserver-oracle122).  At 10.2 it has real 10.2's packed version and the
// module's own banner, where real 10.2 sends its full banner and options line.
// The login answer carries the module's pairs, not the real servers' 34 to 36.
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
// covers it: no key/value pairs in the return parameters and a NUMBER's size
// 22.  So are two scripts:
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
// ending in a line feed.  An answer with no error has no markers.  Two fields
// follow the ceiling, as they follow the real server (10750-dev-ojdbc14-*-
// realserver-oracle112 against the oracle102 and oracle122 captures): success
// iterations in every answer to a fetch, rows or error, are 1 at 10.2 and
// 11.2 and 0 at 12.1, and ORA-00942 at a select's parse has sql type 0 at 10.2
// and 11.2 and 3 at 12.1.  The error object's layout is checked against real
// objects of all three servers first (see checkOjdbc14ErrorObjectBuilder()),
// those two fields with them, and differs from a real one only in what
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
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 nls_lang AL32UTF8","ORACLEPROTOCOLPORT8",1528,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 charset 31, oracle102 back end","ORACLEPROTOCOLPORT7",1527,
		ORA_SERVER_LOGON_TYPES_12_1,ORA_CCAP_FIELD_VERSION_12_1,
		ORA_CCAP_SIZE,"202375680"},
	{"12.1 db2 back end","ORACLEPROTOCOLPORT9",1529,
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
	LISTENER_FETCHATONCE,
	LISTENER_UTF8,
	LISTENER_OCI7,
	LISTENER_DB2
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

// what real 11.2 and 12.2 servers answer ojdbc14's version call with, in
// place of their own version (MEASURED, packet [0012] of 10750-dev-ojdbc14-
// select1-realserver-oracle112 and 10545-dev-ojdbc14-login-select1-
// realserver-oracle122)
static const char	*OJDBC14_VERSION_BANNER_11_1=
			"Oracle Database 11g Release 11.1.0.0.0 - Production";
static const uint32_t	OJDBC14_VERSION_PACKED_11_1=0x0b100000;

// the banner and packed version the module answers ojdbc14's version call
// with at a ceiling: the 11.1 ones at 12.1 and 11.2, as real 11.2 and 12.2
// servers send, and the module's own at 10.2, where real 10.2 sends its own
// version too
static void ojdbc14VersionAnswerOf(unsigned char fieldversion,
					const char *versionno,
					char *banner, size_t size,
					uint32_t *packed) {
	if (fieldversion>=ORA_CCAP_FIELD_VERSION_11_2) {
		charstring::printf(banner,size,"%s",
					OJDBC14_VERSION_BANNER_11_1);
		*packed=OJDBC14_VERSION_PACKED_11_1;
		return;
	}
	*packed=(uint32_t)charstring::convertToUnsignedInteger(versionno);
	buildBanner(banner,size,*packed);
}

// the answer to ojdbc14's version call: the data flags, the ttc code, the
// banner's size and the banner raw, the packed version, then "09 00" and the
// end of the packet, as packet [0012] of the ojdbc14 captures has it
static void putOjdbc14VersionAnswer(unsigned char *buffer, size_t *size,
						const char *banner,
						uint32_t packed) {
	size_t	bannersize=charstring::getLength(banner);
	putZeros(buffer,size,2);
	putByte(buffer,size,ORA_TTC_OK);
	putLenPreInt(buffer,size,(uint32_t)bannersize);
	bytestring::copy(buffer+*size,banner,bannersize);
	*size+=bannersize;
	putLenPreInt(buffer,size,packed);
	putByte(buffer,size,0x09);
	putZeros(buffer,size,1);
}

// ojdbc14's version call, and the answer byte for byte, with the banner and
// the packed version ojdbc14VersionAnswerOf() gives the ceiling.  returns
// whether it was answered at all
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

	char		banner[256];
	uint32_t	packed=0;
	ojdbc14VersionAnswerOf(li->fieldversion,li->versionno,
					banner,sizeof(banner),&packed);

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

	char	check[64];
	charstring::printf(check,sizeof(check),
				" packed version is 0x%08x",packed);
	l.clear();
	l.append(label)->append(check);
	report(l.getString(),shape && actualpacked==packed);

	l.clear();
	l.append(label)->append(" end to end field is 0");
	report(l.getString(),shape && !endtoend);

	unsigned char	expected[3+5+256+5+2];
	size_t		expectedsize=0;
	putOjdbc14VersionAnswer(expected,&expectedsize,banner,packed);

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

// the version answer builder against real servers' answers to ojdbc14's
// version call, before any connection, with the banner and the packed version
// ojdbc14VersionAnswerOf() gives the ceiling that stands for each: 11.2 for
// oracle112 and 12.1 for oracle122, whose answers the module's have to be byte
// for byte, and 10.2 for oracle102, whose packed version the module's has to
// be, with real 10.2's own banner in place of the module's
static void checkOjdbc14VersionAnswerBuilder() {

	stdoutput.printf("\nojdbc14 version answer builder - expect real "
						"servers' answers\n");

	// the bytes around each banner, as captured
	static const unsigned char	head11_1[]={
		0x00, 0x00, 0x08, 0x01, 0x33
	};
	static const unsigned char	tail11_1[]={
		0x04, 0x0b, 0x10, 0x00, 0x00, 0x09, 0x00
	};
	static const unsigned char	head10_2[]={
		0x00, 0x00, 0x08, 0x01, 0x7a
	};
	static const unsigned char	tail10_2[]={
		0x04, 0x0a, 0x20, 0x01, 0x00, 0x09, 0x00
	};
	static const char	*banner10_2=
		"Oracle Database 10g Enterprise Edition Release 10.2.0.1.0 - "
		"Production\nWith the Partitioning, OLAP and Data Mining "
		"options";

	struct {
		const char		*capture;
		size_t			listener;
		const char		*realbanner;
		const unsigned char	*head;
		const unsigned char	*tail;
	} vectors[]={
		{"[0012] of 10750-dev-ojdbc14-select1-realserver-oracle112",
			LISTENER_11_2,NULL,head11_1,tail11_1},
		{"[0012] of 10545-dev-ojdbc14-login-select1-realserver-"
								"oracle122",
			LISTENER_12_1,NULL,head11_1,tail11_1},
		{"[0012] of 10545-dev-ojdbc14-login-select1-realserver-"
								"oracle102",
			LISTENER_10_2,banner10_2,head10_2,tail10_2}
	};

	for (size_t i=0; i<sizeof(vectors)/sizeof(vectors[0]); i++) {

		const listenerinfo	*li=&listeners[vectors[i].listener];
		char		banner[256];
		uint32_t	packed=0;
		ojdbc14VersionAnswerOf(li->fieldversion,li->versionno,
						banner,sizeof(banner),&packed);
		if (vectors[i].realbanner) {
			charstring::printf(banner,sizeof(banner),"%s",
						vectors[i].realbanner);
		}

		unsigned char	real[256];
		size_t		realsize=0;
		bytestring::copy(real,vectors[i].head,5);
		realsize+=5;
		size_t	bannersize=charstring::getLength(banner);
		bytestring::copy(real+realsize,banner,bannersize);
		realsize+=bannersize;
		bytestring::copy(real+realsize,vectors[i].tail,7);
		realsize+=7;

		unsigned char	built[256];
		size_t		builtsize=0;
		putOjdbc14VersionAnswer(built,&builtsize,banner,packed);

		char	label[256];
		charstring::printf(label,sizeof(label),
				"ojdbc14 version answer builder reproduces %d "
				"bytes, packet %s, with the %s ceiling's %s",
				(int)realsize,vectors[i].capture,li->name,
				(vectors[i].realbanner)?"packed version":
						"banner and packed version");
		bool	ok=(builtsize==realsize &&
				!bytestring::compare(built,real,realsize));
		report(label,ok);
		if (!ok) {
			dumpBytes("built",built,builtsize);
			dumpBytes("expected",real,realsize);
		}
	}
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
// call, the way ojdbc14 cancels one.  "fieldversion" is the listener's, which
// some fields follow, and "listener" the listener, which the column sessions
// pick their real server's bytes by
struct ojdbc14session {
	oracleprotocolclient		*client;
	const char			*label;
	oracleprotocolojdbc14answer	*answer;
	bool				ok;
	bool				urgent;
	unsigned char			fieldversion;
	size_t				listener;
};

// the column types sent in the abbreviated form, with no character set,
// form of use or size in characters - see putColumnMetadata() in
// src/protocols/oracle.cpp.  DATE (12) among them, as real servers describe
// it to ojdbc14 (MEASURED, [0018] of 10745-dev-ojdbc14-types-realserver-
// oracle102 and -oracle122: 0c 00 00 00 01 01 00 00 00 00 00 00 00)
static bool ojdbc14AbbreviatedType(unsigned char type) {
	return (type==2 || type==11 || type==12 || type==23 || type==24 ||
		type==113 || type==114 || type==180 || type==181 ||
		type==182 || type==183 || type==231);
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
// success iterations: in an answer to a fetch, ojdbc14FetchSuccessIterations();
// otherwise 0 in an error answering anything but a call that executes and
// fetches (0x8068), 1 there, and 1 with no error or ORA-01403 (MEASURED,
// step 3a of #10750 and the 10750-dev-ojdbc14-*-realserver-oracle112
// captures, all three real servers).  what is the module's own choice, which
// ojdbc14 reads and ignores (CROSS-CHECK, T4CTTIoer's users): the cursor id
// (any the module opened), the array element fields (0, where a real server
// keeps 1 and 1400 or 2 and 12899 from the last dml error), the error
// position (0, where a real server sends the failing token's offset - the
// module can't learn it from the back end), the rowid's rba (0, where a real
// server sends ffffffff in and after a dml error) and the row number of a
// select's error (the rows sent, where a real server counts the rows its
// cursor produced before the failing one, sent or not)
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

// success iterations in an answer to a fetch, rows or error, at a ceiling: 1
// at 10.2 and 11.2 and 0 at 12.1, as real 10.2 and 11.2 servers send 1 and a
// real 12.2 server 0 (MEASURED, every fetch answer of 10745-dev-ojdbc14-
// rows25-realserver-oracle102 and -oracle122, 10750-dev-ojdbc14-rows25-
// realserver-oracle112 and 10750-dev-ojdbc14-fetcherr-realserver-*)
static uint32_t ojdbc14FetchSuccessIterations(unsigned char fieldversion) {
	return (fieldversion>=ORA_CCAP_FIELD_VERSION_12_1)?0:1;
}

// the sql type of ORA-00942 at a select's parse, at a ceiling: 0 at 10.2 and
// 11.2 and 3 at 12.1, as real 10.2 and 11.2 servers send 0 and a real 12.2
// server 3 (MEASURED, [0021] of 10745-dev-ojdbc14-error-realserver-oracle102
// and -oracle122 and of 10750-dev-ojdbc14-error-realserver-oracle112)
static unsigned char ojdbc14MissingTableSqlType(unsigned char fieldversion) {
	return (fieldversion>=ORA_CCAP_FIELD_VERSION_12_1)?3:0;
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

// the call built in the client, sent - in packets of the negotiated sdu if
// it's bigger than one, as ojdbc14 sends a big call - and its answer read as
// ojdbc14 reads it, markers and all.  "columncount" is the cursor's, for a fetch.  false,
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
	uint16_t	sdu=s->client->getSdu();
	bool		answered=(sdu && s->client->getRequestSize()>sdu)?
					s->client->sendSplitPacket():
					s->client->sendPacket();
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
	q.maxlong=0;
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
// byte, with "successiterations"
static void checkOjdbc14SummaryIterations(ojdbc14session *s,
					const char *what,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid,
					unsigned char sqltype,
					uint32_t successiterations) {
	checkOjdbc14NoMarkers(s,what);
	const char	*message=(oranum==1403)?ORA_01403:NULL;
	size_t		messagesize=(oranum==1403)?
					charstring::getLength(ORA_01403):0;
	unsigned char	expected[64+256];
	size_t		size=0;
	putOjdbc14Summary(expected,&size,endtoend,rows,oranum,cursorid,
					sqltype,successiterations,
					message,messagesize);
	char	check[128];
	charstring::printf(check,sizeof(check),
			" error object is exact, end to end %d, rows %d, "
			"error %d, success iterations %d",
			endtoend,rows,oranum,successiterations);
	checkOjdbc14Object(s,what,check,ORA_TTC_ERROR,expected,size);
}

// the same with success iterations 1, for an answer to anything but a fetch
static void checkOjdbc14Summary(ojdbc14session *s, const char *what,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid,
					unsigned char sqltype) {
	checkOjdbc14SummaryIterations(s,what,endtoend,rows,oranum,cursorid,
								sqltype,1);
}

// the same for an answer to a fetch, with the ceiling's success iterations
static void checkOjdbc14FetchSummary(ojdbc14session *s, const char *what,
					uint32_t endtoend, uint32_t rows,
					uint32_t oranum, uint32_t cursorid,
					unsigned char sqltype) {
	checkOjdbc14SummaryIterations(s,what,endtoend,rows,oranum,cursorid,
			sqltype,ojdbc14FetchSuccessIterations(s->fieldversion));
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

// the start of a describe check: the 23 byte block ojdbc14 skips, then the
// column count and the module's constant 0x39 (a real 11.2 and 12.2 server
// sends 0x51, a 10.2 one 0x39, and ojdbc14 drops it)
static void checkOjdbc14DescribeHead(ojdbc14session *s, const char *what,
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
}

// and its end: the date's dalc, then 00 and 02 1f e8
static void checkOjdbc14DescribeTail(ojdbc14session *s, const char *what) {
	oracleprotocolojdbc14answer	*a=s->answer;
	const unsigned char		*tail=s->client->getResponse()+
							a->describetailoffset;
	bool	ok=(a->describetailsize==14 &&
			tail[0]==0x01 && tail[1]==0x07 && tail[2]==0x07 &&
			tail[10]==0x00 && tail[11]==0x02 &&
			tail[12]==0x1f && tail[13]==0xe8);
	stringbuffer	l;
	ojdbc14Label(&l,s,what,
			" describe ends with the date, 00 and 02 1f e8");
	report(l.getString(),ok);
	if (!ok) {
		dumpBytes("tail",tail,a->describetailsize);
	}
}

// a describe: the 23 byte block ojdbc14 skips, the column count and the
// module's constant 0x39, every column byte for byte, then the date, the
// flag 0 and the max data block size 8168 (02 1f e8), as both real servers
// send them.  the max row size and the skipped block are left alone
static void checkOjdbc14Describe(ojdbc14session *s, const char *what,
					const ojdbc14column *columns,
					uint32_t columncount) {

	oracleprotocolojdbc14answer	*a=s->answer;
	char				check[128];

	checkOjdbc14DescribeHead(s,what,columncount);

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

	checkOjdbc14DescribeTail(s,what);
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
// object per row with a short clr per value, and the error object with the
// ceiling's success iterations.  rows come in a run of "rowcount",
// "columncount" values each.  "rowsasked" is what the fetch asked for
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
	uint32_t	successiterations=
			ojdbc14FetchSuccessIterations(s->fieldversion);
	putOjdbc14Summary(expected,&size,endtoend,rows,oranum,cursorid,3,
				successiterations,
				ORA_01403,charstring::getLength(ORA_01403));

	char	check[128];
	charstring::printf(check,sizeof(check),
			" answer is exact, %d rows, end to end %d, error %d, "
			"success iterations %d",
			(int)rowcount,(int)endtoend,(int)oranum,
			(int)successiterations);
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
	SCRIPT_URGENT,
	SCRIPT_COLNULL,
	SCRIPT_COLBINFLOAT,
	SCRIPT_COLNCHAR,
	SCRIPT_COLROWID,
	SCRIPT_COLSCALE,
	SCRIPT_COLLONG,
	SCRIPT_COLINTERVAL,
	SCRIPT_COLTZ,
	SCRIPT_COLDB2,
	SCRIPT_BINDNUMBER,
	SCRIPT_BINDDATE,
	SCRIPT_BINDTZ,
	SCRIPT_BINDBINFLOAT,
	SCRIPT_BINDRAW,
	SCRIPT_BINDSTRING,
	SCRIPT_BINDNCHAR,
	SCRIPT_BINDNULL,
	SCRIPT_BINDROWID,
	SCRIPT_BINDORDER,
	SCRIPT_BINDPLSQL,
	SCRIPT_BINDREEXEC,
	SCRIPT_BINDERRORS,
	SCRIPT_BINDSELECT
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
	"urgent",
	"colnull",
	"colbinfloat",
	"colnchar",
	"colrowid",
	"colscale",
	"collong",
	"colinterval",
	"coltz",
	"coldb2",
	"bindnumber",
	"binddate",
	"bindtz",
	"bindbinfloat",
	"bindraw",
	"bindstring",
	"bindnchar",
	"bindnull",
	"bindrowid",
	"bindorder",
	"bindplsql",
	"bindreexec",
	"binderrors",
	"bindselect"
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
// 1 from dual's, and the ceiling's sql type - see
// ojdbc14MissingTableSqlType().  a real server's error position here is 14,
// the module's 0
static void runError(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	const char	*what="select * from protocoltest10745_nonexistent";
	if (!ojdbc14Statement(s,what,OJDBC14_STATEMENT,0,
				"select * from protocoltest10745_nonexistent",
				AL8I4_QUERY,NULL,0)) {
		return;
	}
	uint32_t	c1=checkOjdbc14ErrorCursor(s,what,0);
	checkOjdbc14QueryError(s,what,4,0,942,c1,
				ojdbc14MissingTableSqlType(s->fieldversion),0,
				&MSG_00942);

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
	checkOjdbc14FetchSummary(s,"its fetch",8,1,1403,c2,3);
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
// alone, with row number 0, and no row reaches the client.  its success
// iterations are 0 at the 0x5e and the ceiling's for a fetch at the fetch -
// see ojdbc14FetchSuccessIterations().  "endtoend" is the 0x5e's, and comes
// back as the last answer's
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
				nextEndToEnd(endtoend,0),0,oranum,c,3,
				ojdbc14FetchSuccessIterations(s->fieldversion),
				m);
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
// the markers, alone, with a fetch's success iterations.  a real server
// delivers rows 1 and 2 at fetch sizes 2
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
		checkOjdbc14QueryError(s,what,6,1,1476,c,3,
				ojdbc14FetchSuccessIterations(s->fieldversion),
				&MSG_01476);
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
		checkOjdbc14FetchSummary(s,"its fetch",5,5,1403,c,3);
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
		checkOjdbc14FetchSummary(s,"its fetch, again",8,5,1403,c,3);
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
// 100000, so ORA-20004, behind the markers, alone, with a fetch's success
// iterations, and the cursor untouched, so that a fetch of 10 rows after it
// gets the 5 rows.  a real server has no limit (MEASURED: 100001 rows in one
// answer); the limit and its error are the module's, and its success
// iterations follow a real server's fetch errors (INFERRED)
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
				ojdbc14FetchSuccessIterations(s->fieldversion),
				&MSG_20004);
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
	checkOjdbc14FetchSummary(s,what,5,65536,0,c,3);

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
// ---- #10749: column types ----

// whether anything answers a connect on the listener's port
static bool listening(const listenerinfo *li) {
	inetsocketclient	sock;
	sock.setHost(host);
	sock.setPort(getPort(li));
	bool	up=(sock.connect()==RESULT_SUCCESS);
	sock.close();
	return up;
}

// a byte string as the captures print it, "02 c1 02", into "bytes", which
// takes "maxsize".  a string that isn't one is a mistake in this file, and
// fails the run
static size_t ojdbc14Hex(const char *hex, unsigned char *bytes, size_t maxsize) {
	size_t	size=0;
	int	digits=0;
	int	value=0;
	for (const char *h=hex; *h; h++) {
		int	d=-1;
		if (*h>='0' && *h<='9') {
			d=*h-'0';
		} else if (*h>='a' && *h<='f') {
			d=*h-'a'+10;
		} else if (*h!=' ' || digits) {
			report("a byte string of this file is hex",false);
			stdoutput.printf("    \"%s\"\n",hex);
			return size;
		}
		if (d<0) {
			continue;
		}
		value=value*16+d;
		if (++digits==2) {
			if (size==maxsize) {
				report("a byte string of this file fits",false);
				return size;
			}
			bytes[size++]=(unsigned char)value;
			digits=0;
			value=0;
		}
	}
	if (digits) {
		report("a byte string of this file is whole bytes",false);
	}
	return size;
}

// a count prefixed int's size at "b": its length byte and the bytes
static size_t ojdbc14LpiSize(const unsigned char *b) {
	return 1+(b[0]&0x7f);
}

// one column of a statement a column session sends: the block a real server
// describes it with, as the capture prints it, and, where the module's
// differs from it by design, the module's size ("mxl") and flags ("fl2"),
// -1 where they are the real server's, and why they differ.  "real" is NULL
// for a column no real server was asked about, which isn't compared
struct ojdbc14colspec {
	const char	*real;
	int		mxl;
	int		fl2;
	const char	*why;
};

// the block the module has to send for "c": the real one, with the size and
// the flags replaced where they differ by design.  the block is read in
// putColumnMetadata()'s order: the type, the flags, the precision, the
// scale (a ub2 for types 2, 180, 181, 183 and 231, a raw byte for the
// others), then the size, the max array size and the flags, each count
// prefixed, and the rest
static size_t ojdbc14ModuleBlock(const ojdbc14colspec *c,
					unsigned char *block, size_t maxsize) {
	unsigned char	real[256];
	size_t		realsize=ojdbc14Hex(c->real,real,sizeof(real));
	if (c->mxl<0 && c->fl2<0) {
		bytestring::copy(block,real,realsize);
		return realsize;
	}
	unsigned char	type=real[0];
	size_t		p=3;
	if (type==2 || type==180 || type==181 || type==183 || type==231) {
		p+=ojdbc14LpiSize(real+p);
	} else {
		p++;
	}
	size_t	mxl=p;
	size_t	mal=mxl+ojdbc14LpiSize(real+mxl);
	size_t	fl2=mal+ojdbc14LpiSize(real+mal);
	size_t	rest=fl2+ojdbc14LpiSize(real+fl2);
	size_t	size=0;
	bytestring::copy(block,real,mxl);
	size=mxl;
	if (c->mxl>=0) {
		putLenPreInt(block,&size,(uint32_t)c->mxl);
	} else {
		bytestring::copy(block+size,real+mxl,mal-mxl);
		size+=mal-mxl;
	}
	bytestring::copy(block+size,real+mal,fl2-mal);
	size+=fl2-mal;
	if (c->fl2>=0) {
		putLenPreInt(block,&size,(uint32_t)c->fl2);
	} else {
		bytestring::copy(block+size,real+fl2,rest-fl2);
		size+=rest-fl2;
	}
	if (size+realsize-rest>maxsize) {
		return 0;
	}
	bytestring::copy(block+size,real+rest,realsize-rest);
	return size+realsize-rest;
}

// a select of a column session and what comes back: the real server's
// describe ("describe" names the capture packet), its max row size and,
// where the module's differs by design, the module's and why (-1 and NULL
// where it doesn't, and -1 for both where no real server was asked), then
// the rows (the capture packets in "rowscapture"), each the values of its
// columns as the captures print them joined by '|' - "" for a column with
// no bytes on the wire and "*" for one that isn't compared - and how many
// rows each fetch asks for.  "own" says no real server stands behind the
// bytes: they are the module's own, which a check pins
struct ojdbc14colquery {
	const char		*query;
	const char		*describe;
	const ojdbc14colspec	*columns;
	uint32_t		columncount;
	int			maxrowsize;
	int			modulemaxrowsize;
	const char		*maxrowsizewhy;
	const char		*rowscapture;
	const char *const	*rows;
	uint32_t		rowcount;
	uint32_t		rowsperfetch;
	bool			own;
};

// the rows of a column query, gathered over its fetches: each value as the
// reader found it, with its bytes copied into "data" - the value's at
// "offset" and the wire's at "wireoffset"
struct ojdbc14keptrows {
	unsigned char			data[262144];
	size_t				size;
	oracleprotocolojdbc14value	values[ORA_8I_MAX_ROWS]
						[ORA_8I_MAX_COLUMNS];
	char				names[ORA_8I_MAX_COLUMNS]
						[ORA_8I_MAX_NAME+1];
	unsigned char			types[ORA_8I_MAX_COLUMNS];
	uint32_t			rowcount;
};

// a column query's describe: every column the real server's block, byte for
// byte, or the module's where it differs by design, and the max row size,
// which ojdbc14 reads and drops
static void checkOjdbc14ColumnDescribe(ojdbc14session *s,
						const ojdbc14colquery *q) {

	oracleprotocolojdbc14answer	*a=s->answer;
	const unsigned char		*response=s->client->getResponse();
	stringbuffer			l;
	char				check[512];

	checkOjdbc14DescribeHead(s,q->query,q->columncount);

	for (uint32_t i=0; i<q->columncount &&
				(int64_t)i<a->columncount; i++) {
		const ojdbc14colspec			*c=&q->columns[i];
		const oracleprotocolojdbc14column	*got=&a->columns[i];
		if (!c->real) {
			continue;
		}
		unsigned char	expected[256];
		size_t		size=ojdbc14ModuleBlock(c,expected,
							sizeof(expected));
		if (q->own) {
			charstring::printf(check,sizeof(check),
				" column %d (%s) is the module's own block, "
				"%s",(int)i,got->name,q->describe);
		} else if (c->mxl<0 && c->fl2<0) {
			charstring::printf(check,sizeof(check),
				" column %d (%s) is the real server's block, "
				"%s",(int)i,got->name,q->describe);
		} else {
			charstring::printf(check,sizeof(check),
				" column %d (%s) is the real server's block "
				"of %s but for the module's %s %d - %s",
				(int)i,got->name,q->describe,
				(c->mxl>=0)?"size":"flags",
				(c->mxl>=0)?c->mxl:c->fl2,c->why);
		}
		checkOjdbc14Bytes(s,q->query,check,response+got->offset,
						got->size,expected,size);
	}

	if (q->maxrowsize>=0) {
		if (q->modulemaxrowsize<0) {
			charstring::printf(check,sizeof(check),
				" max row size is %d, the real server's",
				q->maxrowsize);
		} else {
			charstring::printf(check,sizeof(check),
				" max row size is %d where a real server "
				"sends %d - %s",q->modulemaxrowsize,
				q->maxrowsize,q->maxrowsizewhy);
		}
		ojdbc14Label(&l,s,q->query,check);
		int	expected=(q->modulemaxrowsize<0)?q->maxrowsize:
							q->modulemaxrowsize;
		report(l.getString(),a->maxrowsize==expected);
		if (a->maxrowsize!=expected) {
			stdoutput.printf("    got %lld\n",
						(long long)a->maxrowsize);
		}
	}

	checkOjdbc14DescribeTail(s,q->query);
}

// the rows of one answer, "columncount" values each, into "kept"
static void keepOjdbc14Rows(ojdbc14session *s, ojdbc14keptrows *kept,
						uint32_t columncount) {
	oracleprotocolojdbc14answer	*a=s->answer;
	const unsigned char		*response=s->client->getResponse();
	for (size_t r=0; r<a->rowcount && kept->rowcount<ORA_8I_MAX_ROWS;
									r++) {
		for (uint32_t c=0; c<columncount && c<ORA_8I_MAX_COLUMNS; c++) {
			oracleprotocolojdbc14value	*v=&a->values[r][c];
			oracleprotocolojdbc14value	*k=
					&kept->values[kept->rowcount][c];
			*k=*v;
			if (kept->size+v->size+v->wiresize>
						sizeof(kept->data)) {
				return;
			}
			k->offset=kept->size;
			bytestring::copy(kept->data+kept->size,
					a->valuedata+v->offset,v->size);
			kept->size+=v->size;
			k->wireoffset=kept->size;
			bytestring::copy(kept->data+kept->size,
					response+v->wireoffset,v->wiresize);
			kept->size+=v->wiresize;
		}
		kept->rowcount++;
	}
}

// one expected row against a kept one, value by value
static void checkOjdbc14ColumnRow(ojdbc14session *s, const ojdbc14colquery *q,
						const ojdbc14keptrows *kept,
						uint32_t r) {
	const char	*expected=q->rows[r];
	bool		ok=true;
	bool		skipped=false;
	const char	*e=expected;
	for (uint32_t c=0; c<q->columncount; c++) {
		const char	*bar=charstring::findFirst(e,'|');
		size_t		length=(bar)?(size_t)(bar-e):
						charstring::getLength(e);
		if (!bar && c<q->columncount-1) {
			report("an expected row of this file has a value for "
							"every column",false);
			return;
		}
		char	value[16384];
		if (length>=sizeof(value)) {
			length=sizeof(value)-1;
		}
		charstring::copy(value,e,length);
		value[length]='\0';
		e+=length+((bar)?1:0);

		if (!charstring::compare(value,"*")) {
			skipped=true;
			continue;
		}
		unsigned char	bytes[8192];
		size_t		size=ojdbc14Hex(value,bytes,sizeof(bytes));
		const oracleprotocolojdbc14value	*v=&kept->values[r][c];
		if (v->wiresize!=size ||
			bytestring::compare(kept->data+v->wireoffset,
							bytes,size)) {
			ok=false;
			stdoutput.printf("    row %d, column %d (%s):\n",
						(int)r+1,(int)c,kept->names[c]);
			dumpBytes("got",kept->data+v->wireoffset,v->wiresize);
			dumpBytes("expected",bytes,size);
		}
	}
	char	check[256];
	charstring::printf(check,sizeof(check),
			", row %d is the %s bytes, %s%s",
			(int)r+1,(q->own)?"module's own":"real server's",
			q->rowscapture,
			(skipped)?", the columns marked * aside":"");
	stringbuffer	l;
	ojdbc14Label(&l,s,q->query,check);
	report(l.getString(),ok);
}

// a column query on cursor 0 behind the close of "*pending", and its fetches
// until ORA-01403, every answer checked: the describe, the return
// parameters and the error object of the 0x5e, then each fetch's row
// header (the module's: flags 0x02 and no bit vector, where a real server
// sends 0x22 and bit vectors, which ojdbc14 doesn't need), its rows and
// its error object - rows with no error while a fetch gets every row it
// asked for, and ORA-01403 with the rows of the fetch that gets fewer or
// none, as a real server answers.  then every row against the real
// server's.  the cursor is left in "*pending" for the next call to close,
// as ojdbc14 closes it, and the rows in "kept" if it isn't NULL
static bool ojdbc14ColumnQueryOptions(ojdbc14session *s,
						const ojdbc14colquery *q,
						uint32_t options,
						uint32_t *endtoend,
						uint32_t *pending,
						ojdbc14keptrows *kept) {

	uint32_t	closecount=(*pending)?1:0;
	if (!ojdbc14Statement(s,q->query,options,0,q->query,
					AL8I4_QUERY,pending,closecount)) {
		return false;
	}
	uint32_t	e=nextEndToEnd(endtoend,closecount);
	checkOjdbc14NoMarkers(s,q->query);
	checkOjdbc14Objects(s,q->query,"10 08 04");
	checkOjdbc14ColumnDescribe(s,q);

	ojdbc14keptrows	*k=(kept)?kept:new ojdbc14keptrows;
	bytestring::zero(k,sizeof(*k));
	for (int64_t i=0; i<s->answer->columncount &&
				i<(int64_t)ORA_8I_MAX_COLUMNS; i++) {
		charstring::copy(k->names[i],s->answer->columns[i].name);
	}

	uint32_t	c=checkOjdbc14ReturnParameters(s,q->query);
	checkOjdbc14Summary(s,q->query,e,0,0,c,3);
	*pending=c;

	uint32_t	total=0;
	bool		done=false;
	for (uint32_t f=0; !done && f<q->rowcount/q->rowsperfetch+2; f++) {

		char	what[600];
		charstring::printf(what,sizeof(what),"%s, fetch %d",
							q->query,(int)f+1);
		if (!ojdbc14FetchRows(s,what,c,q->columncount,
						q->rowsperfetch,NULL,0)) {
			if (!kept) {
				delete k;
			}
			return false;
		}
		e=nextEndToEnd(endtoend,0);
		oracleprotocolojdbc14answer	*a=s->answer;

		stringbuffer	codes;
		if (a->rowcount) {
			codes.append("06");
			for (size_t r=0; r<a->rowcount; r++) {
				codes.append(" 07");
			}
			codes.append(" 04");
		} else {
			codes.append("04");
		}
		checkOjdbc14Objects(s,what,codes.getString());
		if (a->rowcount) {
			unsigned char	header[32];
			size_t		size=0;
			putOjdbc14RowHeader(header,&size,q->columncount,
							q->rowsperfetch);
			checkOjdbc14Object(s,what," row header is the "
					"module's, flags 0x02 and no bit "
					"vector",ORA_TTC_ROW_HEADER,
					header,size);
		}
		keepOjdbc14Rows(s,k,q->columncount);
		total+=a->rowcount;
		uint32_t	oranum=(a->rowcount<q->rowsperfetch)?1403:0;
		done=(oranum==1403);
		checkOjdbc14FetchSummary(s,what,e,total,oranum,c,3);
	}

	stringbuffer	l;
	char		check[64];
	charstring::printf(check,sizeof(check),", %d rows come back",
							(int)q->rowcount);
	ojdbc14Label(&l,s,q->query,check);
	report(l.getString(),total==q->rowcount && done);
	if (total!=q->rowcount) {
		stdoutput.printf("    got %d\n",(int)total);
	}
	for (uint32_t r=0; r<q->rowcount && r<k->rowcount; r++) {
		checkOjdbc14ColumnRow(s,q,k,r);
	}
	if (!kept) {
		delete k;
	}
	return s->ok;
}

// ddl or dml of a column session behind the close of "*pending", with
// autocommit on, as Conn14 sends it (0x8121): the return parameters and the
// error object with "rows" touched and the statement's "sqltype".  "drop"
// says it's the drop at the start, which finds no table when the last run
// left none: ORA-00942 behind the markers, as a real server answers it.
// the cursor is left in "*pending"
static void ojdbc14ColumnDml(ojdbc14session *s, const char *query,
						uint32_t rows,
						unsigned char sqltype,
						bool drop,
						uint32_t *endtoend,
						uint32_t *pending) {
	uint32_t	closecount=(*pending)?1:0;
	if (!ojdbc14Statement(s,query,OJDBC14_STATEMENT_COMMIT,0,query,
					AL8I4_DML,pending,closecount)) {
		return;
	}
	uint32_t	e=nextEndToEnd(endtoend,closecount);
	if (drop && s->answer->objectcount==1) {
		uint32_t	c=checkOjdbc14ErrorCursor(s,query,0);
		checkOjdbc14QueryError(s,query,e,0,942,c,12,0,&MSG_00942);
		*pending=c;
		return;
	}
	checkOjdbc14NoMarkers(s,query);
	checkOjdbc14Objects(s,query,"08 04");
	uint32_t	c=checkOjdbc14ReturnParameters(s,query);
	checkOjdbc14Summary(s,query,e,rows,0,c,sqltype);
	*pending=c;
}

// the statement types of the dml and ddl the column sessions send
static const unsigned char	SQLTYPE_CREATE=1;
static const unsigned char	SQLTYPE_INSERT=2;
static const unsigned char	SQLTYPE_DROP=12;

// whether the session's listener is the charset 31 one, whose back end is
// oracle102, so that its describes are a real 10.2 server's
static bool ojdbc14AtOracle102(ojdbc14session *s) {
	return (s->listener==LISTENER_OCI7);
}

// whether the listener's back end session is AL32UTF8, the only one that
// carries text other than ascii to the module (MEASURED, step 2c of #10749:
// the others turn e-acute into e and CJK into ?)
static bool ojdbc14AtUtf8(ojdbc14session *s) {
	return (s->listener==LISTENER_UTF8);
}

// the same as a Statement's 0x5e (0x8021)
static bool ojdbc14ColumnQuery(ojdbc14session *s, const ojdbc14colquery *q,
						uint32_t *endtoend,
						uint32_t *pending,
						ojdbc14keptrows *kept) {
	return ojdbc14ColumnQueryOptions(s,q,OJDBC14_STATEMENT,
						endtoend,pending,kept);
}

// why the module's describes differ from a real server's where they do by
// design (step 3 of #10749, design A.9).  none is read by ojdbc14
static const char	*WHY_LITERAL=
	"the back end describes a NUMBER literal or an expression on one "
	"22 wide, where a real server sends the width of the value";
static const char	*WHY_LITERAL_ROWS=
	"the module's NUMBER literal sizes, see the columns";
static const char	*WHY_INTEGER=
	"a real server flags INTEGER and SMALLINT 0x40000, which the back "
	"end doesn't report";
static const char	*WHY_NULL_ROWS=
	"the module's NUMBER literal size, see the column";

// ---- colnull ----

// select null, and nulls beside values: a column described 0 bytes wide in
// charset 1, form of use 1, and no bytes at all for it in a row (MEASURED,
// 10749-dev-ojdbc14-selectnull-realserver-oracle102 and -oracle122, and at
// oracle112 in a capture not kept, all the same, and the 10745 ones)
static const ojdbc14colspec	NULL1_COLUMNS[]={
	{"01 80 00 00 00 00 00 00 00 01 01 01 00 01 04 01 04 04 4e 55 4c 4c "
		"00 00 00",-1,-1,NULL}
};
static const char *const	NULL1_ROWS[]={""};
static const ojdbc14colquery	NULL1={
	"select null from dual",
	"[0018] of 10749-dev-ojdbc14-selectnull-realserver-oracle102 and "
							"-oracle122",
	NULL1_COLUMNS,1,0,-1,NULL,
	"[0020]",NULL1_ROWS,1,OJDBC14_FETCH_ROWS
};

static const ojdbc14colspec	NULL2_COLUMNS_122[]={
	{"01 80 00 00 00 00 00 00 00 01 01 01 00 01 01 01 01 01 41 00 00 00",
		-1,-1,NULL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 "
		"01 01",22,-1,WHY_LITERAL},
	{"01 80 00 00 00 00 00 00 00 01 01 01 00 01 01 01 01 01 43 00 00 01 "
		"02",-1,-1,NULL},
	{"60 80 00 00 01 01 00 00 00 00 02 03 69 01 01 01 01 01 01 01 01 44 "
		"00 00 01 03",-1,-1,NULL}
};
static const ojdbc14colspec	NULL2_COLUMNS_102[]={
	{"01 80 00 00 00 00 00 00 00 01 01 01 00 01 01 01 01 01 41 00 00 00",
		-1,-1,NULL},
	{"02 00 00 00 01 02 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 01 "
		"01",22,-1,WHY_LITERAL},
	{"01 80 00 00 00 00 00 00 00 01 01 01 00 01 01 01 01 01 43 00 00 01 "
		"02",-1,-1,NULL},
	{"60 80 00 00 01 01 00 00 00 00 01 1f 01 01 01 01 01 01 01 01 44 00 "
		"00 01 03",-1,-1,NULL}
};
static const char *const	NULL2_ROWS[]={"|02 c1 02||01 78"};
static const ojdbc14colquery	NULL2_122={
	"select null a, 1 b, null c, 'x' d from dual",
	"[0022] of 10749-dev-ojdbc14-selectnull-realserver-oracle122",
	NULL2_COLUMNS_122,4,3,23,WHY_NULL_ROWS,
	"[0024]",NULL2_ROWS,1,OJDBC14_FETCH_ROWS
};
static const ojdbc14colquery	NULL2_102={
	"select null a, 1 b, null c, 'x' d from dual",
	"[0022] of 10749-dev-ojdbc14-selectnull-realserver-oracle102",
	NULL2_COLUMNS_102,4,3,23,WHY_NULL_ROWS,
	"[0024]",NULL2_ROWS,1,OJDBC14_FETCH_ROWS
};

// a prepared select null and its re-execute.  INFERRED: no real server was
// sent a PreparedStatement with no binds; its column is column 0 of
// [0022], "a" at position 0
static const ojdbc14colquery	NULL3={
	"select null a from dual",
	"column 0 of [0022] of 10749-dev-ojdbc14-selectnull-realserver-"
					"oracle102 and -oracle122",
	NULL2_COLUMNS_122,1,0,-1,NULL,
	"[0024]",NULL1_ROWS,1,OJDBC14_FETCH_ROWS
};

// colnull: select null, select null beside values, and a PreparedStatement's
// select null, its fetch and its re-execute (0x8068), which a real server
// answers with the row header, a bare 07 for the row, the return parameters
// and ORA-01403, the shape runPrepared() checks (INFERRED for a select whose
// every column is null: the 0x8068 of select null was never sent to a real
// server, and the module sends a 9i client's execute with no defines no
// row there, as a real 10.2 server does)
static void runColNull(ojdbc14session *s, uint32_t *open, uint32_t *count) {

	uint32_t	e=3;
	uint32_t	pending=0;
	if (!ojdbc14ColumnQuery(s,&NULL1,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,(ojdbc14AtOracle102(s))?
					&NULL2_102:&NULL2_122,
					&e,&pending,NULL) ||
		!ojdbc14ColumnQueryOptions(s,&NULL3,OJDBC14_PREPARED,
					&e,&pending,NULL)) {
		return;
	}
	uint32_t	c=pending;

	const char	*what="select null a from dual, the re-execute with fetch";
	if (!ojdbc14Query(s,what,OJDBC14_REEXECUTE_QUERY,c,NULL,
				AL8I4_REEXECUTE_QUERY,NULL,0,0,NULL,0,
				NULL,0,1,5)) {
		return;
	}
	uint32_t	ee=nextEndToEnd(&e,0);
	checkOjdbc14NoMarkers(s,what);
	checkOjdbc14Objects(s,what,"06 07 08 04");
	unsigned char	expected[32];
	size_t		size=0;
	putOjdbc14RowHeader(expected,&size,1,OJDBC14_FETCH_ROWS);
	checkOjdbc14Object(s,what," row header is the module's, flags 0x02 "
				"and no bit vector",ORA_TTC_ROW_HEADER,
				expected,size);
	size=0;
	putByte(expected,&size,ORA_TTC_ROW_DATA);
	checkOjdbc14Object(s,what," row is a bare 07, no byte for the null",
				ORA_TTC_ROW_DATA,expected,size);
	uint32_t	again=checkOjdbc14ReturnParameters(s,what);
	stringbuffer	l;
	ojdbc14Label(&l,s,what," names the same cursor");
	report(l.getString(),again==c);
	checkOjdbc14Summary(s,what,ee,1,1403,c,3);
	open[(*count)++]=c;
}

// ---- colbinfloat ----

// BINARY_FLOAT and BINARY_DOUBLE: types 100 and 101, abbreviated, size 1,
// and the 4 or 8 bytes of the sortable form (MEASURED, 10749-dev-ojdbc14-
// colbinfloat-realserver-oracle102 and -oracle122, and at oracle112 in a
// capture not kept, all the same: positive values with the sign bit set,
// negative ones with every bit inverted, and -0f stored as 0)
static const char	*BF_CREATE=
	"create table protocoltest10749tbf "
	"(id number, f binary_float, d binary_double)";
static const char	*BF_INSERTS[]={
	"insert into protocoltest10749tbf values (1,1.5f,1.5d)",
	"insert into protocoltest10749tbf values "
		"(2,binary_float_nan,binary_double_nan)",
	"insert into protocoltest10749tbf values "
		"(3,binary_float_infinity,binary_double_infinity)",
	"insert into protocoltest10749tbf values "
		"(4,-binary_float_infinity,-binary_double_infinity)",
	"insert into protocoltest10749tbf values (5,-0f,-0d)",
	"insert into protocoltest10749tbf values "
		"(6,3.4028235E38f,1.7976931348623157E308d)",
	"insert into protocoltest10749tbf values "
		"(7,1.17549435E-38f,2.2250738585072014E-308d)",
	"insert into protocoltest10749tbf values (8,1.4E-45f,4.9E-324d)",
	"insert into protocoltest10749tbf values "
		"(9,-3.4028235E38f,-1.7976931348623157E308d)",
	"insert into protocoltest10749tbf values (10,0f,0d)",
	"insert into protocoltest10749tbf values (11,null,null)"
};
static const ojdbc14colspec	BF_TABLE_COLUMNS[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
		"00 00",-1,-1,NULL},
	{"64 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 46 00 00 01 "
		"01",-1,-1,NULL},
	{"65 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 01 "
		"02",-1,-1,NULL}
};
static const char *const	BF_TABLE_ROWS[]={
	"02 c1 02|04 bf c0 00 00|08 bf f8 00 00 00 00 00 00",
	"02 c1 03|04 ff c0 00 00|08 ff f8 00 00 00 00 00 00",
	"02 c1 04|04 ff 80 00 00|08 ff f0 00 00 00 00 00 00",
	"02 c1 05|04 00 7f ff ff|08 00 0f ff ff ff ff ff ff",
	"02 c1 06|04 80 00 00 00|08 80 00 00 00 00 00 00 00",
	"02 c1 07|04 ff 7f ff ff|08 ff ef ff ff ff ff ff ff",
	"02 c1 08|04 80 80 00 00|08 80 10 00 00 00 00 00 00",
	"02 c1 09|04 80 00 00 01|08 80 00 00 00 00 00 00 01",
	"02 c1 0a|04 00 80 00 00|08 00 10 00 00 00 00 00 00",
	"02 c1 0b|04 80 00 00 00|08 80 00 00 00 00 00 00 00",
	"02 c1 0c|00|00"
};
static const ojdbc14colquery	BF_TABLE={
	"select id, f, d from protocoltest10749tbf order by id",
	"[0047] of 10749-dev-ojdbc14-colbinfloat-realserver-oracle102 and "
							"-oracle122",
	BF_TABLE_COLUMNS,3,34,-1,NULL,
	"[0049] and [0051]",BF_TABLE_ROWS,11,OJDBC14_FETCH_ROWS
};
static const ojdbc14colspec	BF_LITERAL_COLUMNS[]={
	{"64 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 00",
		-1,-1,NULL},
	{"65 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 01 "
		"01",-1,-1,NULL},
	{"64 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 43 00 00 01 "
		"02",-1,-1,NULL},
	{"65 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 01 "
		"03",-1,-1,NULL}
};
static const char *const	BF_LITERAL_ROWS[]={
	"04 bf c0 00 00|08 bf f8 00 00 00 00 00 00|04 c0 20 00 00|"
	"08 c0 04 00 00 00 00 00 00"
};
static const ojdbc14colquery	BF_LITERAL={
	"select 1.5f a, 1.5d b, to_binary_float(2.5) c, "
	"to_binary_double(2.5) d from dual",
	"[0059] of 10749-dev-ojdbc14-colbinfloat-realserver-oracle102 and "
							"-oracle122",
	BF_LITERAL_COLUMNS,4,24,-1,NULL,
	"[0061]",BF_LITERAL_ROWS,1,OJDBC14_FETCH_ROWS
};
static const char *const	BF_NULLS_ROWS[]={"00|00"};
static const ojdbc14colquery	BF_NULLS={
	"select cast(null as binary_float) a, "
	"cast(null as binary_double) b from dual",
	"[0063] of 10749-dev-ojdbc14-colbinfloat-realserver-oracle102 and "
							"-oracle122",
	BF_LITERAL_COLUMNS,2,12,-1,NULL,
	"[0065]",BF_NULLS_ROWS,1,OJDBC14_FETCH_ROWS
};
static const ojdbc14colspec	BF_EXPRESSIONS_COLUMNS[]={
	{"64 00 00 00 01 01 00 00 00 00 00 00 00 01 02 01 02 02 46 32 00 00 "
		"00",-1,-1,NULL},
	{"65 00 00 00 01 01 00 00 00 00 00 00 00 01 02 01 02 02 44 32 00 00 "
		"01 01",-1,-1,NULL},
	{"64 00 00 00 01 01 00 00 00 00 00 00 00 01 03 01 03 03 46 2b 31 00 "
		"00 01 02",-1,-1,NULL}
};
static const char *const	BF_EXPRESSIONS_ROWS[]={
	"04 c0 40 00 00|08 c0 08 00 00 00 00 00 00|04 c0 20 00 00"
};
static const ojdbc14colquery	BF_EXPRESSIONS={
	"select f*2 f2, d*2 d2, f+1 from protocoltest10749tbf where id=1",
	"[0067] of 10749-dev-ojdbc14-colbinfloat-realserver-oracle102 and "
							"-oracle122",
	BF_EXPRESSIONS_COLUMNS,3,16,-1,NULL,
	"[0069]",BF_EXPRESSIONS_ROWS,1,OJDBC14_FETCH_ROWS
};

// one check of what a kept value decodes to
static void checkOjdbc14Decoded(ojdbc14session *s, const char *what,
					const char *check, bool ok,
					const char *got) {
	stringbuffer	l;
	ojdbc14Label(&l,s,what,check);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    got %s\n",(got)?got:"nothing");
	}
}

// the kept value at "row" and "column" as a BINARY_FLOAT or BINARY_DOUBLE
static void checkOjdbc14BinaryFloat(ojdbc14session *s, const char *what,
					const ojdbc14keptrows *k,
					uint32_t row, uint32_t column,
					const char *name, double expected) {
	const oracleprotocolojdbc14value	*v=&k->values[row][column];
	double	d=0;
	bool	ok=oracleprotocolclient::decodeOjdbc14BinaryFloat(
					k->data+v->offset,v->size,&d);
	bool	same=(expected!=expected)?(d!=d):(d==expected);
	char	check[128];
	charstring::printf(check,sizeof(check),
				", row %d column %d reads as %s",
				(int)row+1,(int)column,name);
	char	got[64];
	charstring::printf(got,sizeof(got),"%.17g",d);
	checkOjdbc14Decoded(s,what,check,ok && same,got);
}

// colbinfloat: the colbinfloat action's table, its values, a literal select,
// typed nulls and expressions, all the real servers' bytes, and the values
// read back the way ojdbc14 reads them
static void runColBinaryFloat(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnDml(s,"drop table protocoltest10749tbf purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,BF_CREATE,0,SQLTYPE_CREATE,false,&e,&pending);
	for (size_t i=0; i<sizeof(BF_INSERTS)/sizeof(BF_INSERTS[0]); i++) {
		ojdbc14ColumnDml(s,BF_INSERTS[i],1,SQLTYPE_INSERT,false,
							&e,&pending);
	}
	if (!s->ok) {
		return;
	}

	ojdbc14keptrows	*k=new ojdbc14keptrows;
	if (ojdbc14ColumnQuery(s,&BF_TABLE,&e,&pending,k)) {
		double	zero=0.0;
		double	inf=1.0/zero;
		double	nan=zero/zero;
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,0,1,"1.5",1.5);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,0,2,"1.5",1.5);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,1,1,"NaN",nan);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,1,2,"NaN",nan);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,2,1,"+Inf",inf);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,2,2,"+Inf",inf);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,3,1,"-Inf",-inf);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,3,2,"-Inf",-inf);
		checkOjdbc14BinaryFloat(s,BF_TABLE.query,k,8,2,
				"-1.7976931348623157E308",
				-1.7976931348623157E308);
	}
	delete k;
	if (!s->ok ||
		!ojdbc14ColumnQuery(s,&BF_LITERAL,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,&BF_NULLS,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,&BF_EXPRESSIONS,&e,&pending,NULL)) {
		return;
	}
	ojdbc14ColumnDml(s,"drop table protocoltest10749tbf purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	open[(*count)++]=pending;
}

// ---- colnchar ----

// NCHAR and NVARCHAR2: flags 0x80, the second flags 0x1000, charset 2000
// (AL16UTF16), form of use 2, the size in bytes and the size in characters
// half of it, and utf-16be values, NCHAR padded with 00 20 (MEASURED,
// 10749-dev-ojdbc14-colnchar-realserver-oracle102 and -oracle122, and at
// oracle112 in a capture not kept, all the same but for the VARCHAR2
// column's charset)
static const char	*NCHAR_CREATE=
	"create table protocoltest10749tnchar "
	"(id number, nc nchar(10), nv nvarchar2(40), v varchar2(40))";
static const char	*NCHAR_INSERT_ASCII[]={
	"insert into protocoltest10749tnchar values "
		"(1,'abc','abc def','abc')",
	"insert into protocoltest10749tnchar values (5,null,null,null)"
};

// what only an AL32UTF8 back end session carries, and a supplementary
// character the real servers weren't sent, U+1F600 in a nvarchar2, which
// utf-16 sends as the surrogate pair d8 3d de 00 (INFERRED from AL16UTF16;
// no real server answer)
static const char	*NCHAR_INSERT_UTF8[]={
	"insert into protocoltest10749tnchar values "
		"(2,unistr('caf\\00e9'),unistr('\\00fc\\00f1\\00ee caf\\00e9'),"
		"'plain')",
	"insert into protocoltest10749tnchar values "
		"(3,unistr('\\65e5\\672c\\8a9e'),"
		"unistr('\\65e5\\672c\\8a9e\\30c6\\30b9\\30c8'),'x')",
	"insert into protocoltest10749tnchar values "
		"(4,'0123456789',rpad(unistr('\\00e9'),40,unistr('\\00e9')),"
		"'y')",
	"insert into protocoltest10749tnchar values "
		"(6,null,unistr('a\\D83D\\DE00b'),'z')"
};
static const ojdbc14colspec	NCHAR_TABLE_COLUMNS_122[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
		"00 00",-1,-1,NULL},
	{"60 80 00 00 01 14 00 02 10 00 00 00 02 07 d0 02 01 0a 01 02 01 02 "
		"02 4e 43 00 00 01 01",-1,-1,NULL},
	{"01 80 00 00 01 50 00 02 10 00 00 00 02 07 d0 02 01 28 01 02 01 02 "
		"02 4e 56 00 00 01 02",-1,-1,NULL},
	{"01 80 00 00 01 28 00 00 00 00 02 03 69 01 01 28 01 01 01 01 01 56 "
		"00 00 01 03",-1,-1,NULL}
};
static const ojdbc14colspec	NCHAR_TABLE_COLUMNS_102[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
		"00 00",-1,-1,NULL},
	{"60 80 00 00 01 14 00 02 10 00 00 00 02 07 d0 02 01 0a 01 02 01 02 "
		"02 4e 43 00 00 01 01",-1,-1,NULL},
	{"01 80 00 00 01 50 00 02 10 00 00 00 02 07 d0 02 01 28 01 02 01 02 "
		"02 4e 56 00 00 01 02",-1,-1,NULL},
	{"01 80 00 00 01 28 00 00 00 00 01 1f 01 01 28 01 01 01 01 01 56 00 "
		"00 01 03",-1,-1,NULL}
};
static const char	*NCHAR_ROW1=
	"02 c1 02|14 00 61 00 62 00 63 00 20 00 20 00 20 00 20 00 20 00 20 00 "
	"20|0e 00 61 00 62 00 63 00 20 00 64 00 65 00 66|03 61 62 63";
static const char	*NCHAR_ROW5="02 c1 06|00|00|00";
static const char *const	NCHAR_TABLE_ROWS_ASCII[]={
	NCHAR_ROW1,NCHAR_ROW5
};
static const char *const	NCHAR_TABLE_ROWS_UTF8[]={
	NCHAR_ROW1,
	"02 c1 03|14 00 63 00 61 00 66 00 e9 00 20 00 20 00 20 00 20 00 20 "
	"00 20|10 00 fc 00 f1 00 ee 00 20 00 63 00 61 00 66 00 e9|"
	"05 70 6c 61 69 6e",
	"02 c1 04|14 65 e5 67 2c 8a 9e 00 20 00 20 00 20 00 20 00 20 00 20 "
	"00 20|0c 65 e5 67 2c 8a 9e 30 c6 30 b9 30 c8|01 78",
	"02 c1 05|14 00 30 00 31 00 32 00 33 00 34 00 35 00 36 00 37 00 38 "
	"00 39|50 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 "
	"00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 "
	"e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 "
	"00 e9 00 e9 00 e9 00 e9 00 e9 00 e9 00 e9|01 79",
	NCHAR_ROW5,
	"02 c1 07|00|08 00 61 d8 3d de 00 00 62|01 7a"
};
static const ojdbc14colquery	NCHAR_TABLE_122={
	"select id, nc, nv, v from protocoltest10749tnchar order by id",
	"[0035] of 10749-dev-ojdbc14-colnchar-realserver-oracle122",
	NCHAR_TABLE_COLUMNS_122,4,162,-1,NULL,
	"[0037], rows 1 and 5",NCHAR_TABLE_ROWS_ASCII,2,OJDBC14_FETCH_ROWS
};
static const ojdbc14colquery	NCHAR_TABLE_102={
	"select id, nc, nv, v from protocoltest10749tnchar order by id",
	"[0035] of 10749-dev-ojdbc14-colnchar-realserver-oracle102",
	NCHAR_TABLE_COLUMNS_102,4,162,-1,NULL,
	"[0037], rows 1 and 5",NCHAR_TABLE_ROWS_ASCII,2,OJDBC14_FETCH_ROWS
};
static const ojdbc14colquery	NCHAR_TABLE_UTF8={
	"select id, nc, nv, v from protocoltest10749tnchar order by id",
	"[0035] of 10749-dev-ojdbc14-colnchar-realserver-oracle122",
	NCHAR_TABLE_COLUMNS_122,4,162,-1,NULL,
	"[0037], and row 6 inferred",NCHAR_TABLE_ROWS_UTF8,6,
	OJDBC14_FETCH_ROWS
};
static const ojdbc14colspec	NCHAR_LITERAL_COLUMNS[]={
	{"60 80 00 00 01 06 00 02 10 00 00 00 02 07 d0 02 01 03 01 01 01 01 "
		"01 41 00 00 00",-1,-1,NULL},
	{"01 80 00 00 01 08 00 02 10 00 00 00 02 07 d0 02 01 04 01 01 01 01 "
		"01 42 00 00 01 01",-1,-1,NULL},
	{"60 80 00 00 01 06 00 02 10 00 00 00 02 07 d0 02 01 03 01 01 01 01 "
		"01 43 00 00 01 02",-1,-1,NULL},
	{"01 80 00 00 01 0e 00 02 10 00 00 00 02 07 d0 02 01 07 01 01 01 01 "
		"01 44 00 00 01 03",-1,-1,NULL},
	{"01 80 00 00 01 02 00 02 10 00 00 00 02 07 d0 02 01 01 01 01 01 01 "
		"01 45 00 00 01 04",-1,-1,NULL}
};
static const char *const	NCHAR_LITERAL_ROWS_ASCII[]={
	"06 00 61 00 62 00 63|*|06 00 78 00 20 00 20|02 00 79|02 00 71"
};
static const char *const	NCHAR_LITERAL_ROWS_UTF8[]={
	"06 00 61 00 62 00 63|08 00 63 00 61 00 66 00 e9|"
	"06 00 78 00 20 00 20|02 00 79|02 00 71"
};
static const ojdbc14colquery	NCHAR_LITERAL_ASCII={
	"select n'abc' a, unistr('caf\\00e9') b, cast('x' as nchar(3)) c, "
	"cast('y' as nvarchar2(7)) d, to_nchar('q') e from dual",
	"[0043] of 10749-dev-ojdbc14-colnchar-realserver-oracle102 and "
							"-oracle122",
	NCHAR_LITERAL_COLUMNS,5,36,-1,NULL,
	"[0045], b's e-acute lost by the back end session",
	NCHAR_LITERAL_ROWS_ASCII,1,OJDBC14_FETCH_ROWS
};
static const ojdbc14colquery	NCHAR_LITERAL_UTF8={
	"select n'abc' a, unistr('caf\\00e9') b, cast('x' as nchar(3)) c, "
	"cast('y' as nvarchar2(7)) d, to_nchar('q') e from dual",
	"[0043] of 10749-dev-ojdbc14-colnchar-realserver-oracle102 and "
							"-oracle122",
	NCHAR_LITERAL_COLUMNS,5,36,-1,NULL,
	"[0045]",NCHAR_LITERAL_ROWS_UTF8,1,OJDBC14_FETCH_ROWS
};

// the kept value at "row" and "column" as utf-16be text
static void checkOjdbc14Nchar(ojdbc14session *s, const char *what,
					const ojdbc14keptrows *k,
					uint32_t row, uint32_t column,
					const char *name, const char *expected) {
	const oracleprotocolojdbc14value	*v=&k->values[row][column];
	stringbuffer	utf8;
	bool	ok=oracleprotocolclient::decodeOjdbc14Utf16(
					k->data+v->offset,v->size,&utf8);
	char	check[128];
	charstring::printf(check,sizeof(check),
				", row %d column %d reads as %s",
				(int)row+1,(int)column,name);
	checkOjdbc14Decoded(s,what,check,
			ok && !charstring::compare(utf8.getString(),expected),
			utf8.getString());
}

// colnchar: the colnchar action's table and its literal select.  non-ascii
// text only at the AL32UTF8 listener, which also gets the supplementary
// character; the ascii rows everywhere
static void runColNchar(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	bool		utf8=ojdbc14AtUtf8(s);
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnDml(s,"drop table protocoltest10749tnchar purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,NCHAR_CREATE,0,SQLTYPE_CREATE,false,&e,&pending);
	for (size_t i=0; i<sizeof(NCHAR_INSERT_ASCII)/
				sizeof(NCHAR_INSERT_ASCII[0]); i++) {
		ojdbc14ColumnDml(s,NCHAR_INSERT_ASCII[i],1,SQLTYPE_INSERT,
						false,&e,&pending);
	}
	for (size_t i=0; utf8 && i<sizeof(NCHAR_INSERT_UTF8)/
				sizeof(NCHAR_INSERT_UTF8[0]); i++) {
		ojdbc14ColumnDml(s,NCHAR_INSERT_UTF8[i],1,SQLTYPE_INSERT,
						false,&e,&pending);
	}
	if (!s->ok) {
		return;
	}

	const ojdbc14colquery	*table=(utf8)?&NCHAR_TABLE_UTF8:
					(ojdbc14AtOracle102(s))?
						&NCHAR_TABLE_102:
						&NCHAR_TABLE_122;
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	if (ojdbc14ColumnQuery(s,table,&e,&pending,k)) {
		checkOjdbc14Nchar(s,table->query,k,0,1,"\"abc       \"",
							"abc       ");
		checkOjdbc14Nchar(s,table->query,k,0,2,"\"abc def\"",
							"abc def");
		if (utf8) {
			checkOjdbc14Nchar(s,table->query,k,1,2,
				"u-umlaut n-tilde i-circumflex cafe-acute",
				"\xc3\xbc\xc3\xb1\xc3\xae caf\xc3\xa9");
			checkOjdbc14Nchar(s,table->query,k,2,2,
				"6 CJK characters",
				"\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e"
				"\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88");
			checkOjdbc14Nchar(s,table->query,k,5,2,
				"a, U+1F600 and b",
				"a\xf0\x9f\x98\x80""b");
		}
	}
	delete k;
	if (!s->ok ||
		!ojdbc14ColumnQuery(s,(utf8)?&NCHAR_LITERAL_UTF8:
						&NCHAR_LITERAL_ASCII,
						&e,&pending,NULL)) {
		return;
	}
	ojdbc14ColumnDml(s,"drop table protocoltest10749tnchar purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	open[(*count)++]=pending;
}

// ---- colrowid ----

// a heap table's ROWID pseudo-column: type 11, abbreviated, size 1, not
// nullable, and a value of 0e and the four numbers in their narrowest form
// (MEASURED, 10749-dev-ojdbc14-colrowid-realserver-oracle122, [0041] and
// [0043]; the same at oracle102 and oracle112, in captures not kept, but
// for the numbers, which are each server's data)
static const ojdbc14colspec	ROWID_TABLE_COLUMNS[]={
	{"0b 00 00 00 01 01 00 00 00 00 00 00 00 00 01 01 01 01 52 00 00 00",
		-1,-1,NULL},
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
		"00 01 01",-1,-1,NULL}
};
static const char *const	ROWID_TABLE_ROWS[]={
	"*|02 c1 02","*|02 c1 03","*|02 c1 04"
};
static const ojdbc14colquery	ROWID_TABLE={
	"select rowid r, id from protocoltest10749trid order by id",
	"[0041] of 10749-dev-ojdbc14-colrowid-realserver-oracle122",
	ROWID_TABLE_COLUMNS,2,38,-1,NULL,
	"[0043] but for the rowids, the data",ROWID_TABLE_ROWS,3,
	OJDBC14_FETCH_ROWS
};

// what the back end says the rowids are, which no real server was asked
static const ojdbc14colspec	ROWID_TEXT_COLUMNS[]={{NULL,-1,-1,NULL}};
static const char *const	ROWID_TEXT_ROWS[]={"*","*","*"};
static const ojdbc14colquery	ROWID_TEXT={
	"select rowidtochar(rowid) c from protocoltest10749trid order by id",
	"no capture",
	ROWID_TEXT_COLUMNS,1,-1,-1,NULL,
	"no capture",ROWID_TEXT_ROWS,3,OJDBC14_FETCH_ROWS
};
static const char *const	ROWID_DUAL_ROWS[]={"*"};
static const ojdbc14colquery	ROWID_DUAL={
	"select rowid r from dual",
	"[0082] of 10749-dev-ojdbc14-colrowid-realserver-oracle122",
	ROWID_TABLE_COLUMNS,1,16,-1,NULL,
	"[0084] but for the rowid, the data",ROWID_DUAL_ROWS,1,
	OJDBC14_FETCH_ROWS
};

// a kept rowid is the narrowest form of its four numbers: 0e, the object,
// the file, 00, the block and the row, each count prefixed (MEASURED, [0043]
// of 10749-dev-ojdbc14-colrowid-realserver-oracle122:
// 0e 03 4a c5 72 01 05 00 02 04 0e 00 for object 4900210, file 5, block 1038
// and row 0), and, if "text" isn't NULL, reads as it
static void checkOjdbc14Rowid(ojdbc14session *s, const char *what,
					const ojdbc14keptrows *k, uint32_t row,
					const char *text) {
	const oracleprotocolojdbc14value	*v=&k->values[row][0];
	unsigned char	expected[32];
	size_t		size=0;
	putByte(expected,&size,0x0e);
	putLenPreInt(expected,&size,(uint32_t)v->rowidobject);
	putLenPreInt(expected,&size,(uint32_t)v->rowidfile);
	putByte(expected,&size,0x00);
	putLenPreInt(expected,&size,(uint32_t)v->rowidblock);
	putLenPreInt(expected,&size,(uint32_t)v->rowidrow);
	char	check[160];
	charstring::printf(check,sizeof(check),
			", row %d's rowid is 0e and its four numbers, each "
			"in its narrowest form",(int)row+1);
	checkOjdbc14Bytes(s,what,check,k->data+v->wireoffset,v->wiresize,
							expected,size);
	if (text) {
		char	got[19];
		oracleprotocolclient::decodeOjdbc14Rowid(v,got);
		charstring::printf(check,sizeof(check),
				", row %d's rowid reads as rowidtochar()'s "
				"%s",(int)row+1,text);
		checkOjdbc14Decoded(s,what,check,
				!charstring::compare(got,text),got);
	}
}

// an index-organized table's rowid pseudo-column: type 208 (UROWID),
// abbreviated, size 31, not nullable, as is the primary key, and a value of
// a ub4 byte count and a clr of the logical rowid's bytes; and a UROWID cast
// of a heap rowid: size 4000 and the physical form, 01 and the four numbers
// at their full widths (MEASURED, [0049], [0051], [0057] and [0059] of
// 10749-dev-ojdbc14-colrowid-realserver-oracle122; the bytes are each
// table's data, so the values are checked against what the back end says
// they are)
static const ojdbc14colspec	IOT_TABLE_COLUMNS[]={
	{"d0 00 00 00 01 1f 00 00 00 00 00 00 00 00 01 01 01 01 52 00 00 00",
		-1,-1,NULL},
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 00 02 01 02 02 49 44 00 "
		"00 01 01",-1,-1,NULL}
};
static const ojdbc14colquery	IOT_TABLE={
	"select rowid r, id from protocoltest10749tiot order by id",
	"[0049] of 10749-dev-ojdbc14-colrowid-realserver-oracle122",
	IOT_TABLE_COLUMNS,2,53,-1,NULL,
	"[0051] but for the rowids, the data",ROWID_TABLE_ROWS,3,
	OJDBC14_FETCH_ROWS
};
static const ojdbc14colquery	IOT_TEXT={
	"select cast(rowid as varchar2(100)) c from protocoltest10749tiot "
	"order by id",
	"no capture",
	ROWID_TEXT_COLUMNS,1,-1,-1,NULL,
	"no capture",ROWID_TEXT_ROWS,3,OJDBC14_FETCH_ROWS
};
static const ojdbc14colspec	UROWID_CAST_COLUMNS[]={
	{"d0 00 00 00 02 0f a0 00 00 00 00 00 00 00 01 01 01 01 01 55 00 00 "
		"00",-1,-1,NULL},
	{"01 80 00 00 01 1e 00 00 00 00 02 03 69 01 01 1e 01 01 01 01 01 56 "
		"00 00 01 01",-1,-1,NULL}
};
static const char *const	UROWID_CAST_ROWS[]={"*|*","*|*","*|*"};
static const ojdbc14colquery	UROWID_CAST={
	"select cast(rowid as urowid) u, cast(rowid as varchar2(30)) v "
	"from protocoltest10749trid",
	"[0057] of 10749-dev-ojdbc14-colrowid-realserver-oracle122",
	UROWID_CAST_COLUMNS,2,4030,-1,NULL,
	"[0059] but for the rowids, the data",UROWID_CAST_ROWS,3,
	OJDBC14_FETCH_ROWS
};

// ROWID, UROWID and UROWID(100) columns of a table: a rowid column is
// nullable, so its block ends in 01 where the pseudo-column's ends in 00
// (MEASURED, [0074] and [0076] of the same capture)
static const ojdbc14colspec	ROWID_COLUMNS_COLUMNS[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
		"00 00",-1,-1,NULL},
	{"0b 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 52 00 00 01 "
		"01",-1,-1,NULL},
	{"d0 00 00 00 02 0f a0 00 00 00 00 00 00 00 01 01 01 01 01 55 00 00 "
		"01 02",-1,-1,NULL},
	{"d0 00 00 00 01 64 00 00 00 00 00 00 00 01 02 01 02 02 55 32 00 00 "
		"01 03",-1,-1,NULL}
};
static const char *const	ROWID_COLUMNS_ROWS[]={
	"02 c1 02|*|*|00","02 c1 03|*|*|00","02 c1 04|*|*|00",
	"02 c1 0c|00|00|*","02 c1 0d|00|00|*","02 c1 0e|00|00|*",
	"02 c1 64|00|00|00"
};
static const ojdbc14colquery	ROWID_COLUMNS={
	"select id, r, u, u2 from protocoltest10749trc order by id",
	"[0074] of 10749-dev-ojdbc14-colrowid-realserver-oracle122",
	ROWID_COLUMNS_COLUMNS,4,4138,-1,NULL,
	"[0076] but for the rowids, the data",ROWID_COLUMNS_ROWS,7,
	OJDBC14_FETCH_ROWS
};

// the value of a base 64 digit, or -1
static int ojdbc14Base64Digit(char c) {
	if (c>='A' && c<='Z') {
		return c-'A';
	} else if (c>='a' && c<='z') {
		return c-'a'+26;
	} else if (c>='0' && c<='9') {
		return c-'0'+52;
	} else if (c=='+') {
		return 62;
	} else if (c=='/') {
		return 63;
	}
	return -1;
}

// unpadded base 64 into "bytes"
static size_t ojdbc14Base64(const char *text, unsigned char *bytes,
							size_t maxsize) {
	size_t		size=0;
	uint32_t	bits=0;
	int		count=0;
	for (const char *t=text; *t && size<maxsize; t++) {
		int	d=ojdbc14Base64Digit(*t);
		if (d<0) {
			break;
		}
		bits=(bits<<6)|(uint32_t)d;
		count+=6;
		if (count>=8) {
			count-=8;
			bytes[size++]=(unsigned char)(bits>>count);
		}
	}
	return size;
}

// "count" base 64 digits of an 18 character rowid as one number
static uint64_t ojdbc14RowidField(const char *text, size_t count) {
	uint64_t	value=0;
	for (size_t i=0; i<count; i++) {
		value=(value<<6)|(uint64_t)ojdbc14Base64Digit(text[i]);
	}
	return value;
}

// the text of the kept value at "row" and "column" of "k"
static void ojdbc14KeptText(const ojdbc14keptrows *k, uint32_t row,
					uint32_t column, char *text, size_t size) {
	const oracleprotocolojdbc14value	*v=&k->values[row][column];
	size_t	length=(v->size<size)?v->size:size-1;
	charstring::copy(text,(const char *)k->data+v->offset,length);
	text[length]='\0';
}

// a kept urowid against the text the back end gives for it: a logical one
// ("*" and base 64) is 02 and the base 64's bytes, and a physical one (18
// characters) is 01 and the four numbers, 4, 2, 4 and 2 bytes big-endian
// (MEASURED, step 2c: *BAFAAWMCwQL+ is 02 04 01 40 01 63 02 c1 02 fe, and
// [0059] of 10749-dev-ojdbc14-colrowid-realserver-oracle122:
// AASsVyAAFAAAAQOAAA is 01 00 4a c5 72 00 05 00 00 04 0e 00 00), behind a
// ub4 of its size
static void checkOjdbc14Urowid(ojdbc14session *s, const char *what,
					const ojdbc14keptrows *k, uint32_t row,
					const char *text) {
	unsigned char	expected[64];
	size_t		size=0;
	if (text[0]=='*') {
		expected[size++]=0x02;
		size+=ojdbc14Base64(text+1,expected+size,sizeof(expected)-1);
	} else {
		uint64_t	fields[]={
			ojdbc14RowidField(text,6),
			ojdbc14RowidField(text+6,3),
			ojdbc14RowidField(text+9,6),
			ojdbc14RowidField(text+15,3)
		};
		size_t		widths[]={4,2,4,2};
		expected[size++]=0x01;
		for (size_t f=0; f<4; f++) {
			for (size_t b=widths[f]; b>0; b--) {
				expected[size++]=
					(unsigned char)(fields[f]>>(8*(b-1)));
			}
		}
	}
	unsigned char	wire[80];
	size_t		wiresize=0;
	putLenPreInt(wire,&wiresize,(uint32_t)size);
	putShortClr(wire,&wiresize,(const char *)expected,size);
	const oracleprotocolojdbc14value	*v=&k->values[row][0];
	char	check[160];
	charstring::printf(check,sizeof(check),
			", row %d's urowid is its size and the bytes of %s",
			(int)row+1,text);
	checkOjdbc14Bytes(s,what,check,k->data+v->wireoffset,v->wiresize,
							wire,wiresize);
}

// colrowid: the colrowid action's heap and index-organized tables, their
// rowids against what the back end says they are as text, urowids, rowid
// columns, and dual's rowid
static void runColRowid(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnDml(s,"drop table protocoltest10749trid purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,"drop table protocoltest10749tiot purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,"drop table protocoltest10749trc purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,"create table protocoltest10749trid "
				"(id number, v varchar2(10))",
				0,SQLTYPE_CREATE,false,&e,&pending);
	ojdbc14ColumnDml(s,"create table protocoltest10749tiot "
				"(id number primary key, v varchar2(10)) "
				"organization index",
				0,SQLTYPE_CREATE,false,&e,&pending);
	ojdbc14ColumnDml(s,"insert into protocoltest10749trid select level, "
				"'v'||level from dual connect by level <= 3",
				3,SQLTYPE_INSERT,false,&e,&pending);
	ojdbc14ColumnDml(s,"insert into protocoltest10749tiot select level, "
				"'v'||level from dual connect by level <= 3",
				3,SQLTYPE_INSERT,false,&e,&pending);
	if (!s->ok) {
		return;
	}

	ojdbc14keptrows	*rowids=new ojdbc14keptrows;
	ojdbc14keptrows	*texts=new ojdbc14keptrows;
	if (ojdbc14ColumnQuery(s,&ROWID_TABLE,&e,&pending,rowids) &&
		ojdbc14ColumnQuery(s,&ROWID_TEXT,&e,&pending,texts)) {
		for (uint32_t r=0; r<3; r++) {
			const oracleprotocolojdbc14value	*t=
							&texts->values[r][0];
			char	text[64];
			size_t	size=(t->size<sizeof(text))?t->size:
							sizeof(text)-1;
			charstring::copy(text,
				(const char *)texts->data+t->offset,size);
			text[size]='\0';
			checkOjdbc14Rowid(s,ROWID_TABLE.query,rowids,r,text);
		}
	}
	if (s->ok &&
		ojdbc14ColumnQuery(s,&IOT_TABLE,&e,&pending,rowids) &&
		ojdbc14ColumnQuery(s,&IOT_TEXT,&e,&pending,texts)) {
		for (uint32_t r=0; r<3; r++) {
			char	text[64];
			ojdbc14KeptText(texts,r,0,text,sizeof(text));
			checkOjdbc14Urowid(s,IOT_TABLE.query,rowids,r,text);
		}
	}
	if (s->ok && ojdbc14ColumnQuery(s,&UROWID_CAST,&e,&pending,rowids)) {
		for (uint32_t r=0; r<3; r++) {
			char	text[64];
			ojdbc14KeptText(rowids,r,1,text,sizeof(text));
			checkOjdbc14Urowid(s,UROWID_CAST.query,rowids,r,text);
		}
	}
	delete rowids;
	delete texts;
	ojdbc14ColumnDml(s,"create table protocoltest10749trc (id number, "
				"r rowid, u urowid, u2 urowid(100))",
				0,SQLTYPE_CREATE,false,&e,&pending);
	ojdbc14ColumnDml(s,"insert into protocoltest10749trc select id, "
				"rowid, rowid, null from protocoltest10749trid",
				3,SQLTYPE_INSERT,false,&e,&pending);
	ojdbc14ColumnDml(s,"insert into protocoltest10749trc select id+10, "
				"null, null, rowid from protocoltest10749tiot",
				3,SQLTYPE_INSERT,false,&e,&pending);
	ojdbc14ColumnDml(s,"insert into protocoltest10749trc values "
				"(99,null,null,null)",
				1,SQLTYPE_INSERT,false,&e,&pending);
	if (!s->ok || !ojdbc14ColumnQuery(s,&ROWID_COLUMNS,&e,&pending,NULL)) {
		return;
	}
	ojdbc14keptrows	*dual=new ojdbc14keptrows;
	if (ojdbc14ColumnQuery(s,&ROWID_DUAL,&e,&pending,dual)) {
		checkOjdbc14Rowid(s,ROWID_DUAL.query,dual,0,NULL);
	}
	delete dual;
	if (!s->ok) {
		return;
	}
	ojdbc14ColumnDml(s,"drop table protocoltest10749trid purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	ojdbc14ColumnDml(s,"drop table protocoltest10749tiot purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	ojdbc14ColumnDml(s,"drop table protocoltest10749trc purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	open[(*count)++]=pending;
}

// ---- colscale ----

// NUMBER's precisions and scales, BINARY_FLOAT and BINARY_DOUBLE columns
// beside them, INTEGER and SMALLINT, and a NOT NULL column, whose nullable
// byte is 0 (MEASURED, 10749-dev-ojdbc14-colscale-realserver-oracle102 and
// -oracle122, and at oracle112 in a capture not kept: the table's describe
// is the same at all three, a literal's scale is 00 at oracle102 and 81 7f
// at the others)
static const char	*SCALE_CREATE=
	"create table protocoltest10749tscale (n number, n10 number(10), "
	"n102 number(10,2), n38 number(38), n385 number(38,5), "
	"nm2 number(5,-2), n10z number(10,0), n1 number(1), "
	"n3837 number(3,7), f float, f10 float(10), f126 float(126), "
	"bf binary_float, bd binary_double, i integer, si smallint, "
	"dc decimal(8,3), r real, dp double precision, nn number not null)";
static const char	*SCALE_INSERTS[]={
	"insert into protocoltest10749tscale values (12345.678,1234567890,"
	"12345678.91,12345678901234567890123456789012345678,1234567890.12345,"
	"12300,1234567890,5,0.0000123,3.14159,3.14159,3.14159,1.5f,1.5d,42,"
	"7,12345.678,2.5,2.5,1)",
	"insert into protocoltest10749tscale (nn) values (2)"
};
static const ojdbc14colspec	SCALE_TABLE_COLUMNS[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 01 01 01 01 4e 00 00 "
		"00",-1,-1,NULL},
	{"02 00 0a 00 01 16 00 00 00 00 00 00 00 01 03 01 03 03 4e 31 30 00 "
		"00 01 01",-1,-1,NULL},
	{"02 00 0a 01 02 01 16 00 00 00 00 00 00 00 01 04 01 04 04 4e 31 30 "
		"32 00 00 01 02",-1,-1,NULL},
	{"02 00 26 00 01 16 00 00 00 00 00 00 00 01 03 01 03 03 4e 33 38 00 "
		"00 01 03",-1,-1,NULL},
	{"02 00 26 01 05 01 16 00 00 00 00 00 00 00 01 04 01 04 04 4e 33 38 "
		"35 00 00 01 04",-1,-1,NULL},
	{"02 00 05 81 02 01 16 00 00 00 00 00 00 00 01 03 01 03 03 4e 4d 32 "
		"00 00 01 05",-1,-1,NULL},
	{"02 00 0a 00 01 16 00 00 00 00 00 00 00 01 04 01 04 04 4e 31 30 5a "
		"00 00 01 06",-1,-1,NULL},
	{"02 00 01 00 01 16 00 00 00 00 00 00 00 01 02 01 02 02 4e 31 00 00 "
		"01 07",-1,-1,NULL},
	{"02 00 03 01 07 01 16 00 00 00 00 00 00 00 01 05 01 05 05 4e 33 38 "
		"33 37 00 00 01 08",-1,-1,NULL},
	{"02 00 7e 81 7f 01 16 00 00 00 00 00 00 00 01 01 01 01 01 46 00 00 "
		"01 09",-1,-1,NULL},
	{"02 00 0a 81 7f 01 16 00 00 00 00 00 00 00 01 03 01 03 03 46 31 30 "
		"00 00 01 0a",-1,-1,NULL},
	{"02 00 7e 81 7f 01 16 00 00 00 00 00 00 00 01 04 01 04 04 46 31 32 "
		"36 00 00 01 0b",-1,-1,NULL},
	{"64 00 00 00 01 01 00 00 00 00 00 00 00 01 02 01 02 02 42 46 00 00 "
		"01 0c",-1,-1,NULL},
	{"65 00 00 00 01 01 00 00 00 00 00 00 00 01 02 01 02 02 42 44 00 00 "
		"01 0d",-1,-1,NULL},
	{"02 00 26 00 01 16 00 03 04 00 00 00 00 00 00 00 01 01 01 01 01 49 "
		"00 00 01 0e",-1,0,WHY_INTEGER},
	{"02 00 26 00 01 16 00 03 04 00 00 00 00 00 00 00 01 02 01 02 02 53 "
		"49 00 00 01 0f",-1,0,WHY_INTEGER},
	{"02 00 08 01 03 01 16 00 00 00 00 00 00 00 01 02 01 02 02 44 43 00 "
		"00 01 10",-1,-1,NULL},
	{"02 00 3f 81 7f 01 16 00 00 00 00 00 00 00 01 01 01 01 01 52 00 00 "
		"01 11",-1,-1,NULL},
	{"02 00 7e 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 44 50 00 "
		"00 01 12",-1,-1,NULL},
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 00 02 01 02 02 4e 4e 00 "
		"00 01 13",-1,-1,NULL}
};
static const char *const	SCALE_TABLE_ROWS[]={
	"06 c3 02 18 2e 44 51|06 c5 0d 23 39 4f 5b|06 c4 0d 23 39 4f 5c|"
	"14 d3 0d 23 39 4f 5b 0d 23 39 4f 5b 0d 23 39 4f 5b 0d 23 39 4f|"
	"09 c5 0d 23 39 4f 5b 0d 23 33|03 c3 02 18|06 c5 0d 23 39 4f 5b|"
	"02 c1 06|03 be 0d 1f|05 c1 04 0f 10 5b|04 c1 04 0f 15|"
	"05 c1 04 0f 10 5b|04 bf c0 00 00|08 bf f8 00 00 00 00 00 00|02 c1 2b|"
	"02 c1 08|06 c3 02 18 2e 44 51|03 c1 03 33|03 c1 03 33|02 c1 02",
	"00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|00|02 c1 03"
};
static const ojdbc14colquery	SCALE_TABLE={
	"select * from protocoltest10749tscale order by nn",
	"[0029] of 10749-dev-ojdbc14-colscale-realserver-oracle102 and "
							"-oracle122",
	SCALE_TABLE_COLUMNS,20,408,-1,NULL,
	"[0031]",SCALE_TABLE_ROWS,2,OJDBC14_FETCH_ROWS
};

static const ojdbc14colspec	SCALE_LITERAL_COLUMNS_122[]={
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 "
		"00",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 "
		"01 01",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 01 01 01 01 43 00 00 "
		"01 02",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 01 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 "
		"01 03",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 10 00 00 00 00 00 00 00 01 01 01 01 01 45 00 00 "
		"01 04",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 46 00 00 "
		"01 05",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 47 00 00 "
		"01 06",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 15 00 00 00 00 00 00 00 01 01 01 01 01 48 00 00 "
		"01 07",22,-1,WHY_LITERAL}
};
static const ojdbc14colspec	SCALE_LITERAL_COLUMNS_102[]={
	{"02 00 00 00 01 02 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 00",
		22,-1,WHY_LITERAL},
	{"02 00 00 00 01 03 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 01 "
		"01",22,-1,WHY_LITERAL},
	{"02 00 00 00 01 03 00 00 00 00 00 00 00 01 01 01 01 01 43 00 00 01 "
		"02",22,-1,WHY_LITERAL},
	{"02 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 01 "
		"03",22,-1,WHY_LITERAL},
	{"02 00 00 00 01 10 00 00 00 00 00 00 00 01 01 01 01 01 45 00 00 01 "
		"04",22,-1,WHY_LITERAL},
	{"02 00 00 00 01 02 00 00 00 00 00 00 00 01 01 01 01 01 46 00 00 01 "
		"05",22,-1,WHY_LITERAL},
	{"02 00 00 00 01 02 00 00 00 00 00 00 00 01 01 01 01 01 47 00 00 01 "
		"06",22,-1,WHY_LITERAL},
	{"02 00 00 00 01 15 00 00 00 00 00 00 00 01 01 01 01 01 48 00 00 01 "
		"07",22,-1,WHY_LITERAL}
};
static const char *const	SCALE_LITERAL_ROWS[]={
	"02 c1 02|03 c1 02 33|03 3e 64 66|01 80|"
	"10 cf 0d 23 39 4f 5b 0d 23 39 4f 5b 0d 23 39 4f 5b|02 c6 02|02 bc 02|"
	"15 c0 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22 22"
};
static const char	*SCALE_LITERAL_QUERY=
	"select 1 a, 1.5 b, -1 c, 0 d, 123456789012345678901234567890 e, "
	"1e10 f, 1e-10 g, 1/3 h from dual";
static const ojdbc14colquery	SCALE_LITERAL_122={
	SCALE_LITERAL_QUERY,
	"[0037] of 10749-dev-ojdbc14-colscale-realserver-oracle122",
	SCALE_LITERAL_COLUMNS_122,8,50,176,WHY_LITERAL_ROWS,
	"[0039]",SCALE_LITERAL_ROWS,1,OJDBC14_FETCH_ROWS
};
static const ojdbc14colquery	SCALE_LITERAL_102={
	SCALE_LITERAL_QUERY,
	"[0037] of 10749-dev-ojdbc14-colscale-realserver-oracle102",
	SCALE_LITERAL_COLUMNS_102,8,50,176,WHY_LITERAL_ROWS,
	"[0039]",SCALE_LITERAL_ROWS,1,OJDBC14_FETCH_ROWS
};

static const ojdbc14colspec	SCALE_AGGREGATE_COLUMNS[]={
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 43 00 00 00",
		-1,-1,NULL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 53 00 00 01 "
		"01",-1,-1,NULL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 01 "
		"02",-1,-1,NULL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 4d 00 00 01 "
		"03",-1,-1,NULL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 02 01 02 02 4d 49 00 00 "
		"01 04",-1,-1,NULL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 58 00 00 01 "
		"05",-1,-1,NULL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 59 00 00 01 "
		"06",-1,-1,NULL}
};
static const char *const	SCALE_AGGREGATE_ROWS[]={
	"02 c1 02|00|00|00|00|00|00",
	"02 c1 02|06 c3 02 18 2e 44 51|06 c3 02 18 2e 44 51|"
	"06 c5 0d 23 39 4f 5b|06 c4 0d 23 39 4f 5c|06 c5 19 46 0e 3a 51|"
	"06 c4 0d 23 39 50 5c"
};
static const ojdbc14colquery	SCALE_AGGREGATE={
	"select count(*) c, sum(n) s, avg(n) a, max(n10) m, min(n102) mi, "
	"n10*2 x, n102+1 y from protocoltest10749tscale group by n10, n102",
	"[0041] of 10749-dev-ojdbc14-colscale-realserver-oracle122",
	SCALE_AGGREGATE_COLUMNS,7,154,-1,NULL,
	"[0043]",SCALE_AGGREGATE_ROWS,2,OJDBC14_FETCH_ROWS
};

static const ojdbc14colspec	SCALE_CAST_COLUMNS[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 "
		"00",-1,-1,NULL},
	{"02 00 05 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 01 "
		"01",-1,-1,NULL},
	{"02 00 05 01 02 01 16 00 00 00 00 00 00 00 01 01 01 01 01 43 00 00 "
		"01 02",-1,-1,NULL},
	{"02 00 26 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 01 "
		"03",-1,-1,NULL},
	{"02 00 05 81 02 01 16 00 00 00 00 00 00 00 01 01 01 01 01 45 00 00 "
		"01 04",-1,-1,NULL},
	{"02 00 7e 81 7f 01 16 00 00 00 00 00 00 00 01 01 01 01 01 46 00 00 "
		"01 05",-1,-1,NULL},
	{"02 00 0a 81 7f 01 16 00 00 00 00 00 00 00 01 01 01 01 01 47 00 00 "
		"01 06",-1,-1,NULL},
	{"64 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 48 00 00 01 "
		"07",-1,-1,NULL},
	{"65 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 49 00 00 01 "
		"08",-1,-1,NULL},
	{"02 00 26 00 01 16 00 03 04 00 00 00 00 00 00 00 01 01 01 01 01 4a "
		"00 00 01 09",-1,0,WHY_INTEGER},
	{"02 00 0a 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 4b 00 00 01 "
		"0a",-1,-1,NULL}
};
static const char *const	SCALE_CAST_ROWS[]={
	"02 c1 02|02 c1 02|03 c1 02 33|02 c1 02|01 80|02 c1 02|02 c1 02|"
	"04 bf 80 00 00|08 bf f0 00 00 00 00 00 00|02 c1 02|02 c1 02"
};
static const ojdbc14colquery	SCALE_CAST={
	"select cast(1 as number) a, cast(1 as number(5)) b, "
	"cast(1.5 as number(5,2)) c, cast(1 as number(38)) d, "
	"cast(1 as number(5,-2)) e, cast(1 as float) f, "
	"cast(1 as float(10)) g, cast(1 as binary_float) h, "
	"cast(1 as binary_double) i, cast(1 as integer) j, "
	"cast(1 as number(10,0)) k from dual",
	"[0045] of 10749-dev-ojdbc14-colscale-realserver-oracle122",
	SCALE_CAST_COLUMNS,11,210,-1,NULL,
	"[0047]",SCALE_CAST_ROWS,1,OJDBC14_FETCH_ROWS
};

static const ojdbc14colspec	SCALE_FUNCTION_COLUMNS[]={
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 "
		"00",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 "
		"01 01",22,-1,WHY_LITERAL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 43 00 00 01 "
		"02",-1,-1,NULL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 "
		"01 03",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 45 00 00 "
		"01 04",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 46 00 00 "
		"01 05",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 47 00 00 "
		"01 06",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 48 00 00 "
		"01 07",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 49 00 00 "
		"01 08",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 4a 00 00 "
		"01 09",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 4b 00 00 "
		"01 0a",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 01 01 01 01 4c 00 00 "
		"01 0b",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 01 01 01 01 4d 00 00 "
		"01 0c",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 01 01 01 01 4e 00 00 "
		"01 0d",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 15 00 00 00 00 00 00 00 01 01 01 01 01 4f 00 00 "
		"01 0e",22,-1,WHY_LITERAL}
};
static const char *const	SCALE_FUNCTION_ROWS[]={
	"02 c1 06|03 c1 06 33|02 c1 06|03 c1 02 18|02 c1 02|02 c1 02|02 c1 03|"
	"02 c1 03|02 c1 02|02 c1 04|02 c1 62|02 c1 04|03 3e 64 66|03 c2 0b 19|"
	"15 c1 02 2a 2b 0e 39 18 4a 0a 33 31 51 11 59 49 2b 0a 46 51 4f 3a"
};
static const ojdbc14colquery	SCALE_FUNCTION={
	"select to_number('5') a, to_number('5.5') b, to_number('5','9') c, "
	"round(1.234,2) d, trunc(1.234) e, floor(1.5) f, ceil(1.5) g, "
	"mod(5,3) h, abs(-1) i, length('abc') j, ascii('a') k, "
	"instr('abc','c') l, sign(-3) m, power(2,10) n, sqrt(2) o from dual",
	"[0049] of 10749-dev-ojdbc14-colscale-realserver-oracle122",
	SCALE_FUNCTION_COLUMNS,15,73,330,WHY_LITERAL_ROWS,
	"[0051]",SCALE_FUNCTION_ROWS,1,OJDBC14_FETCH_ROWS
};

static const ojdbc14colspec	SCALE_LEVEL_COLUMNS[]={
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 4c 00 00 00",
		-1,-1,NULL},
	{"02 00 00 00 01 16 00 00 00 00 00 00 00 01 01 01 01 01 52 00 00 01 "
		"01",-1,-1,NULL}
};
static const char *const	SCALE_LEVEL_ROWS[]={
	"02 c1 02|02 c1 02","02 c1 03|02 c1 03"
};
static const ojdbc14colquery	SCALE_LEVEL={
	"select level l, rownum r from dual connect by level <= 2",
	"[0053] of 10749-dev-ojdbc14-colscale-realserver-oracle122",
	SCALE_LEVEL_COLUMNS,2,44,-1,NULL,
	"[0055]",SCALE_LEVEL_ROWS,2,OJDBC14_FETCH_ROWS
};

// the types action of #10745, whose DATE columns are 1 wide, as a real
// server describes a DATE (MEASURED, [0018] of 10745-dev-ojdbc14-types-
// realserver-oracle122; ojdbc14 takes 7 for type 12 whatever is sent)
static const ojdbc14colspec	TYPES_COLUMNS[]={
	{"02 00 00 81 7f 01 02 00 00 00 00 00 00 00 01 02 01 02 02 4e 31 00 "
		"00 00",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 05 00 00 00 00 00 00 00 01 02 01 02 02 4e 32 00 "
		"00 01 01",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 02 01 02 02 4e 33 00 "
		"00 01 02",22,-1,WHY_LITERAL},
	{"02 00 00 81 7f 01 03 00 00 00 00 00 00 00 01 02 01 02 02 4e 34 00 "
		"00 01 03",22,-1,WHY_LITERAL},
	{"01 80 00 00 01 0a 00 00 00 00 02 03 69 01 01 0a 01 01 01 01 01 56 "
		"00 00 01 04",-1,-1,NULL},
	{"60 80 00 00 01 05 00 00 00 00 02 03 69 01 01 05 01 01 01 01 01 43 "
		"00 00 01 05",-1,-1,NULL},
	{"0c 00 00 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 01 "
		"06",-1,-1,NULL},
	{"b4 00 00 01 09 01 0b 00 00 00 00 00 00 00 01 02 01 02 02 54 53 00 "
		"00 01 07",-1,-1,NULL},
	{"17 00 00 00 01 04 00 00 00 00 00 00 00 01 01 01 01 01 52 00 00 01 "
		"08",-1,-1,NULL},
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 4e 4e 00 "
		"00 01 09",-1,-1,NULL},
	{"01 80 00 00 01 05 00 00 00 00 02 03 69 01 01 05 01 02 01 02 02 4e "
		"56 00 00 01 0a",-1,-1,NULL},
	{"0c 00 00 00 01 01 00 00 00 00 00 00 00 01 02 01 02 02 4e 44 00 00 "
		"01 0b",-1,-1,NULL}
};
static const char *const	TYPES_ROWS[]={
	"02 c1 2b|05 c1 04 0f 10 5b|03 3e 5e 66|03 3f 33 66|03 61 62 63|"
	"05 78 79 20 20 20|07 78 7c 03 05 0e 0f 10|"
	"0b 78 7c 03 05 0e 0f 10 07 5b ca 00|04 de ad be ef|00|00|00"
};
static const ojdbc14colquery	TYPES={
	"select 42 n1, 3.14159 n2, -7 n3, -0.5 n4, "
	"cast('abc' as varchar2(10)) v, "
	"cast('xy' as char(5)) c, "
	"to_date('2024-03-05 13:14:15','YYYY-MM-DD HH24:MI:SS') d, "
	"to_timestamp('2024-03-05 13:14:15.123456',"
			"'YYYY-MM-DD HH24:MI:SS.FF') ts, "
	"hextoraw('DEADBEEF') r, "
	"cast(null as number) nn, cast(null as varchar2(5)) nv, "
	"cast(null as date) nd from dual",
	"[0018] of 10745-dev-ojdbc14-types-realserver-oracle122",
	TYPES_COLUMNS,12,84,159,WHY_LITERAL_ROWS,
	"[0020]",TYPES_ROWS,1,OJDBC14_FETCH_ROWS
};

// colscale: the colscale action's table and its selects, and the types
// select of #10745.  the charset 31 listener, whose back end is oracle102,
// runs the table and the literals only, the literals with oracle102's scale
static void runColScale(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	bool		at102=ojdbc14AtOracle102(s);
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnDml(s,"drop table protocoltest10749tscale purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,SCALE_CREATE,0,SQLTYPE_CREATE,false,&e,&pending);
	for (size_t i=0; i<sizeof(SCALE_INSERTS)/sizeof(SCALE_INSERTS[0]);
									i++) {
		ojdbc14ColumnDml(s,SCALE_INSERTS[i],1,SQLTYPE_INSERT,false,
							&e,&pending);
	}
	if (!s->ok ||
		!ojdbc14ColumnQuery(s,&SCALE_TABLE,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,(at102)?&SCALE_LITERAL_102:
						&SCALE_LITERAL_122,
						&e,&pending,NULL)) {
		return;
	}
	if (!at102 &&
		(!ojdbc14ColumnQuery(s,&SCALE_AGGREGATE,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,&SCALE_CAST,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,&SCALE_FUNCTION,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,&SCALE_LEVEL,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,&TYPES,&e,&pending,NULL))) {
		return;
	}
	ojdbc14ColumnDml(s,"drop table protocoltest10749tscale purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	open[(*count)++]=pending;
}

// ---- collong ----

// LONG and LONG RAW: size 0, a max row size of 0, one row per fetch, which
// is what ojdbc14 asks for when a select has one, and values in the long
// form - fe, chunks of up to 255 bytes, 00 - then the indicator and the
// return code, 00 00, or a null's 00 81 01 02 05 7d (MEASURED, 10749-dev-
// ojdbc14-collong-realserver-oracle122, and at oracle102 and oracle112 in
// captures not kept, all the same but for the LONG column's charset)
static const char	*LONG_INSERTS[]={
	"insert into protocoltest10749tlong values (1,'short text')",
	"insert into protocoltest10749tlong values (2,rpad('a',64,'b'))",
	"insert into protocoltest10749tlong values (3,rpad('a',65,'b'))",
	"insert into protocoltest10749tlong values (4,rpad('a',300,'b'))",
	"insert into protocoltest10749tlong values (5,rpad('a',4000,'b'))",
	"insert into protocoltest10749tlong values (6,null)",
	"insert into protocoltest10749tlraw values (1,hextoraw('deadbeef'))",
	"insert into protocoltest10749tlraw values "
		"(2,hextoraw(rpad('ab',128,'ab')))",
	"insert into protocoltest10749tlraw values "
		"(3,hextoraw(rpad('ab',130,'ab')))",
	"insert into protocoltest10749tlraw values "
		"(4,hextoraw(rpad('cd',600,'cd')))",
	"insert into protocoltest10749tlraw values "
		"(5,hextoraw(rpad('ef',2000,'ef')))",
	"insert into protocoltest10749tlraw values (6,null)"
};
static const char	*LONG_ID=
	"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
	"00 00";
static const char	*LONG_ID_SECOND=
	"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
	"00 01 01";
static const ojdbc14colspec	LONG_ID_L_COLUMNS[]={
	{LONG_ID,-1,-1,NULL},
	{"08 80 00 00 00 00 00 00 00 02 03 69 01 00 01 01 01 01 01 4c 00 00 "
		"01 01",-1,-1,NULL}
};
static const ojdbc14colspec	LONG_L_ID_COLUMNS[]={
	{"08 80 00 00 00 00 00 00 00 02 03 69 01 00 01 01 01 01 01 4c 00 00 "
		"00",-1,-1,NULL},
	{LONG_ID_SECOND,-1,-1,NULL}
};
static const ojdbc14colspec	LRAW_ID_L_COLUMNS[]={
	{LONG_ID,-1,-1,NULL},
	{"18 00 00 00 00 00 00 00 00 00 00 00 01 01 01 01 01 4c 00 00 01 01",
		-1,-1,NULL}
};
static const ojdbc14colspec	LRAW_L_ID_COLUMNS[]={
	{"18 00 00 00 00 00 00 00 00 00 00 00 01 01 01 01 01 4c 00 00 00",
		-1,-1,NULL},
	{LONG_ID_SECOND,-1,-1,NULL}
};

// a long value's wire bytes as the captures print them: "size" bytes, the
// first "first" and the rest "rest", or a null if "size" is 0
static void appendOjdbc14LongHex(stringbuffer *hex, size_t size,
					unsigned char first, unsigned char rest) {
	char	b[4];
	if (!size) {
		hex->append("00 81 01 02 05 7d");
		return;
	}
	hex->append("fe");
	for (size_t i=0; i<size; i++) {
		if (!(i%255)) {
			size_t	chunk=(size-i<255)?size-i:255;
			charstring::printf(b,sizeof(b)," %02x",(int)chunk);
			hex->append(b);
		}
		charstring::printf(b,sizeof(b)," %02x",(i)?rest:first);
		hex->append(b);
	}
	hex->append(" 00 00 00");
}

// colong: the collong action's tables, each selected with the long column
// first and second, and a lone null
static void runColLong(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnDml(s,"drop table protocoltest10749tlong purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,"drop table protocoltest10749tlraw purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,"create table protocoltest10749tlong "
					"(id number, l long)",
					0,SQLTYPE_CREATE,false,&e,&pending);
	ojdbc14ColumnDml(s,"create table protocoltest10749tlraw "
					"(id number, l long raw)",
					0,SQLTYPE_CREATE,false,&e,&pending);
	for (size_t i=0; i<sizeof(LONG_INSERTS)/sizeof(LONG_INSERTS[0]);
									i++) {
		ojdbc14ColumnDml(s,LONG_INSERTS[i],1,SQLTYPE_INSERT,false,
							&e,&pending);
	}
	if (!s->ok) {
		return;
	}

	// the values: 'short text', then 'a' and 'b's, 64, 65, 300 and
	// 4000 of them, and a null, and in the LONG RAW 4 bytes of de ad be
	// ef, 64 and 65 of ab, 300 of cd, 1000 of ef and a null
	static const size_t	longsizes[]={10,64,65,300,4000,0};
	static const size_t	rawsizes[]={4,64,65,300,1000,0};
	static const unsigned char	rawbytes[]={0,0xab,0xab,0xcd,0xef,0};
	stringbuffer	longhex[6];
	stringbuffer	rawhex[6];
	for (size_t i=0; i<6; i++) {
		if (!i) {
			longhex[i].append("fe 0a 73 68 6f 72 74 20 74 65 78 74 "
								"00 00 00");
			rawhex[i].append("fe 04 de ad be ef 00 00 00");
			continue;
		}
		appendOjdbc14LongHex(&longhex[i],longsizes[i],'a','b');
		appendOjdbc14LongHex(&rawhex[i],rawsizes[i],
						rawbytes[i],rawbytes[i]);
	}

	struct {
		const char		*query;
		const char		*describe;
		const char		*rows;
		const ojdbc14colspec	*columns;
		bool			longfirst;
		bool			raw;
		uint32_t		rowcount;
	} queries[]={
		{"select id, l from protocoltest10749tlong order by id",
			"[0056]","[0058] to [0070]",LONG_ID_L_COLUMNS,
			false,false,6},
		{"select l, id from protocoltest10749tlong order by id",
			"[0074]","[0076] to [0088]",LONG_L_ID_COLUMNS,
			true,false,6},
		{"select id, l from protocoltest10749tlong where id=6",
			"[0092]","[0094]",LONG_ID_L_COLUMNS,false,false,1},
		{"select id, l from protocoltest10749tlraw order by id",
			"[0098]","[0100] to [0110]",LRAW_ID_L_COLUMNS,
			false,true,6},
		{"select l, id from protocoltest10749tlraw order by id",
			"[0114]","[0116] to [0126]",LRAW_L_ID_COLUMNS,
			true,true,6}
	};

	for (size_t i=0; i<sizeof(queries)/sizeof(queries[0]); i++) {

		stringbuffer	rows[6];
		const char	*rowpointers[6];
		for (size_t r=0; r<queries[i].rowcount; r++) {
			size_t		v=(queries[i].rowcount==1)?5:r;
			char		id[16];
			charstring::printf(id,sizeof(id),"02 c1 %02x",
								(int)v+2);
			const char	*value=(queries[i].raw)?
						rawhex[v].getString():
						longhex[v].getString();
			if (queries[i].longfirst) {
				rows[r].append(value)->append('|');
				rows[r].append(id);
			} else {
				rows[r].append(id)->append('|');
				rows[r].append(value);
			}
			rowpointers[r]=rows[r].getString();
		}

		char	describe[128];
		char	rowscapture[128];
		charstring::printf(describe,sizeof(describe),
				"%s of 10749-dev-ojdbc14-collong-realserver-"
				"oracle122",queries[i].describe);
		charstring::printf(rowscapture,sizeof(rowscapture),
				"%s",queries[i].rows);
		ojdbc14colquery	q;
		q.query=queries[i].query;
		q.describe=describe;
		q.columns=queries[i].columns;
		q.columncount=2;
		q.maxrowsize=0;
		q.modulemaxrowsize=-1;
		q.maxrowsizewhy=NULL;
		q.rowscapture=rowscapture;
		q.rows=rowpointers;
		q.rowcount=queries[i].rowcount;
		q.rowsperfetch=1;
		q.own=false;
		if (!ojdbc14ColumnQuery(s,&q,&e,&pending,NULL)) {
			return;
		}
	}
	ojdbc14ColumnDml(s,"drop table protocoltest10749tlong purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	ojdbc14ColumnDml(s,"drop table protocoltest10749tlraw purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	open[(*count)++]=pending;
}

// ---- colinterval ----

// INTERVAL YEAR TO MONTH (182) and DAY TO SECOND (183): abbreviated, the
// leading precision, the fractional seconds as the scale, size 1, and 5 and
// 11 byte values (MEASURED, 10749-dev-ojdbc14-colinterval-realserver-
// oracle122, and at oracle102 and oracle112 in captures not kept, all the
// same).  the module sent
// them right before #10749 (step 2c), so these are for regressions
static const char	*INTERVAL_STATEMENTS[]={
	"create table protocoltest10749tint (id number, "
	"ym interval year to month, ym5 interval year(5) to month, "
	"ds interval day to second, ds9 interval day(9) to second(9), "
	"ds0 interval day(1) to second(0), ds3 interval day(4) to second(3))",
	"insert into protocoltest10749tint values (1,"
	"interval '1-2' year to month,interval '12345-11' year(5) to month,"
	"interval '3 04:05:06.123456' day to second,"
	"interval '123456789 23:59:59.123456789' day(9) to second(9),"
	"interval '9 01:02:03' day(1) to second(0),"
	"interval '1234 12:00:00.001' day(4) to second(3))",
	"insert into protocoltest10749tint values (2,"
	"interval '-5-6' year to month,interval '-1-0' year(5) to month,"
	"interval '-1 00:00:00.5' day to second,"
	"interval '-1 00:00:00.000000001' day(9) to second(9),"
	"interval '-1 00:00:00' day(1) to second(0),"
	"interval '-0 00:00:00.001' day(4) to second(3))",
	"insert into protocoltest10749tint values (3,"
	"interval '0-0' year to month,interval '0-0' year(5) to month,"
	"interval '0 00:00:00' day to second,"
	"interval '0 00:00:00' day(9) to second(9),"
	"interval '0 00:00:00' day(1) to second(0),"
	"interval '0 00:00:00' day(4) to second(3))",
	"insert into protocoltest10749tint values "
	"(4,null,null,null,null,null,null)"
};
static const ojdbc14colspec	INTERVAL_TABLE_COLUMNS[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
		"00 00",-1,-1,NULL},
	{"b6 00 02 00 01 01 00 00 00 00 00 00 00 01 02 01 02 02 59 4d 00 00 "
		"01 01",-1,-1,NULL},
	{"b6 00 05 00 01 01 00 00 00 00 00 00 00 01 03 01 03 03 59 4d 35 00 "
		"00 01 02",-1,-1,NULL},
	{"b7 00 02 01 06 01 01 00 00 00 00 00 00 00 01 02 01 02 02 44 53 00 "
		"00 01 03",-1,-1,NULL},
	{"b7 00 09 01 09 01 01 00 00 00 00 00 00 00 01 03 01 03 03 44 53 39 "
		"00 00 01 04",-1,-1,NULL},
	{"b7 00 01 00 01 01 00 00 00 00 00 00 00 01 03 01 03 03 44 53 30 00 "
		"00 01 05",-1,-1,NULL},
	{"b7 00 04 01 03 01 01 00 00 00 00 00 00 00 01 03 01 03 03 44 53 33 "
		"00 00 01 06",-1,-1,NULL}
};
static const char *const	INTERVAL_TABLE_ROWS[]={
	"02 c1 02|05 80 00 00 01 3e|05 80 00 30 39 47|"
	"0b 80 00 00 03 40 41 42 87 5b ca 00|"
	"0b 87 5b cd 15 53 77 77 87 5b cd 15|"
	"0b 80 00 00 09 3d 3e 3f 80 00 00 00|"
	"0b 80 00 04 d2 48 3c 3c 80 0f 42 40",
	"02 c1 03|05 7f ff ff fb 36|05 7f ff ff ff 3c|"
	"0b 7f ff ff ff 3c 3c 3c 62 32 9b 00|"
	"0b 7f ff ff ff 3c 3c 3c 7f ff ff ff|"
	"0b 7f ff ff ff 3c 3c 3c 80 00 00 00|"
	"0b 80 00 00 00 3c 3c 3c 7f f0 bd c0",
	"02 c1 04|05 80 00 00 00 3c|05 80 00 00 00 3c|"
	"0b 80 00 00 00 3c 3c 3c 80 00 00 00|"
	"0b 80 00 00 00 3c 3c 3c 80 00 00 00|"
	"0b 80 00 00 00 3c 3c 3c 80 00 00 00|"
	"0b 80 00 00 00 3c 3c 3c 80 00 00 00",
	"02 c1 05|00|00|00|00|00|00"
};
static const ojdbc14colquery	INTERVAL_TABLE={
	"select * from protocoltest10749tint order by id",
	"[0033] of 10749-dev-ojdbc14-colinterval-realserver-oracle122",
	INTERVAL_TABLE_COLUMNS,7,76,-1,NULL,
	"[0035]",INTERVAL_TABLE_ROWS,4,OJDBC14_FETCH_ROWS
};
static const ojdbc14colspec	INTERVAL_LITERAL_COLUMNS[]={
	{"b6 00 02 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 00",
		-1,-1,NULL},
	{"b7 00 02 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 01 "
		"01",-1,-1,NULL},
	{"b7 00 02 01 06 01 01 00 00 00 00 00 00 00 01 01 01 01 01 43 00 00 "
		"01 02",-1,-1,NULL},
	{"b6 00 09 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 44 00 00 01 "
		"03",-1,-1,NULL},
	{"b7 00 09 01 09 01 01 00 00 00 00 00 00 00 01 01 01 01 01 45 00 00 "
		"01 04",-1,-1,NULL}
};
static const char *const	INTERVAL_LITERAL_ROWS[]={
	"05 80 00 00 02 3f|0b 80 00 00 05 3c 3c 3c 80 00 00 00|"
	"0b 80 00 00 01 3e 3f 40 9d cd 65 00|05 80 00 00 00 43|"
	"0b 80 00 00 00 3f 5a 3c 80 00 00 00"
};
static const ojdbc14colquery	INTERVAL_LITERAL={
	"select interval '2-3' year to month a, interval '5' day b, "
	"interval '1 2:3:4.5' day to second c, numtoyminterval(7,'month') d, "
	"numtodsinterval(3.5,'hour') e from dual",
	"[0041] of 10749-dev-ojdbc14-colinterval-realserver-oracle122",
	INTERVAL_LITERAL_COLUMNS,5,43,-1,NULL,
	"[0043]",INTERVAL_LITERAL_ROWS,1,OJDBC14_FETCH_ROWS
};
static const ojdbc14colspec	INTERVAL_NULLS_COLUMNS[]={
	{"b6 00 02 00 01 01 00 00 00 00 00 00 00 01 01 01 01 01 41 00 00 00",
		-1,-1,NULL},
	{"b7 00 02 01 06 01 01 00 00 00 00 00 00 00 01 01 01 01 01 42 00 00 "
		"01 01",-1,-1,NULL}
};
static const char *const	INTERVAL_NULLS_ROWS[]={"00|00"};
static const ojdbc14colquery	INTERVAL_NULLS={
	"select cast(null as interval year to month) a, "
	"cast(null as interval day to second) b from dual",
	"[0045] of 10749-dev-ojdbc14-colinterval-realserver-oracle122",
	INTERVAL_NULLS_COLUMNS,2,16,-1,NULL,
	"[0047]",INTERVAL_NULLS_ROWS,1,OJDBC14_FETCH_ROWS
};

// colinterval: the colinterval action's table, literals and typed nulls,
// and the values read back the way ojdbc14 reads them
static void runColInterval(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnDml(s,"drop table protocoltest10749tint purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,INTERVAL_STATEMENTS[0],0,SQLTYPE_CREATE,false,
							&e,&pending);
	for (size_t i=1; i<sizeof(INTERVAL_STATEMENTS)/
				sizeof(INTERVAL_STATEMENTS[0]); i++) {
		ojdbc14ColumnDml(s,INTERVAL_STATEMENTS[i],1,SQLTYPE_INSERT,
						false,&e,&pending);
	}
	if (!s->ok) {
		return;
	}
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	if (ojdbc14ColumnQuery(s,&INTERVAL_TABLE,&e,&pending,k)) {
		struct {
			uint32_t	row;
			uint32_t	column;
			unsigned char	type;
			const char	*text;
		} expected[]={
			{0,1,182,"+1-02"},
			{0,2,182,"+12345-11"},
			{0,3,183,"+3 04:05:06.123456000"},
			{0,4,183,"+123456789 23:59:59.123456789"},
			{1,1,182,"-5-06"},
			{1,3,183,"-1 00:00:00.500000000"}
		};
		for (size_t i=0; i<sizeof(expected)/sizeof(expected[0]); i++) {
			const oracleprotocolojdbc14value	*v=
				&k->values[expected[i].row][expected[i].column];
			stringbuffer	text;
			bool	ok=oracleprotocolclient::decodeOjdbc14Interval(
						expected[i].type,
						k->data+v->offset,v->size,
						&text);
			char	check[128];
			charstring::printf(check,sizeof(check),
					", row %d column %d reads as %s",
					(int)expected[i].row+1,
					(int)expected[i].column,
					expected[i].text);
			checkOjdbc14Decoded(s,INTERVAL_TABLE.query,check,
					ok && !charstring::compare(
							text.getString(),
							expected[i].text),
					text.getString());
		}
	}
	delete k;
	if (!s->ok ||
		!ojdbc14ColumnQuery(s,&INTERVAL_LITERAL,&e,&pending,NULL) ||
		!ojdbc14ColumnQuery(s,&INTERVAL_NULLS,&e,&pending,NULL)) {
		return;
	}
	ojdbc14ColumnDml(s,"drop table protocoltest10749tint purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	open[(*count)++]=pending;
}

// ---- coltz ----

// TIMESTAMP WITH TIME ZONE (181) and WITH LOCAL TIME ZONE (231): a 13 byte
// value in utc with the zone, an offset or a region, behind it, and an 11
// or 7 byte one in the database's time zone, utc on every server here
// (MEASURED, 10749-dev-ojdbc14-coltz-realserver-oracle122; the module sent
// them right before #10749, step 2c, so these are for regressions too)
static const char	*TZ_INSERT_ZONES[]={
	"2024-03-05 13:14:15.123456 US/Eastern",
	"2024-07-05 13:14:15.123456 US/Eastern",
	"2024-03-05 13:14:15.123456 +05:30",
	"2024-03-05 13:14:15.123456 -08:00",
	"2024-03-05 13:14:15.123456 UTC",
	"2024-03-05 13:14:15.123456 Asia/Tokyo",
	"1970-01-01 00:00:00 +00:00",
	"2024-11-03 01:30:00 US/Eastern EST",
	"2024-11-03 01:30:00 US/Eastern EDT"
};
static const ojdbc14colspec	TZ_TABLE_COLUMNS[]={
	{"02 00 00 81 7f 01 16 00 00 00 00 00 00 00 01 02 01 02 02 49 44 00 "
		"00 00",-1,-1,NULL},
	{"b5 00 00 01 06 01 01 00 00 00 00 00 00 00 01 02 01 02 02 54 5a 00 "
		"00 01 01",-1,-1,NULL},
	{"b5 00 00 01 03 01 01 00 00 00 00 00 00 00 01 03 01 03 03 54 5a 33 "
		"00 00 01 02",-1,-1,NULL},
	{"e7 00 00 01 06 01 0b 00 00 00 00 00 00 00 01 03 01 03 03 4c 54 5a "
		"00 00 01 03",-1,-1,NULL},
	{"e7 00 00 01 09 01 0b 00 00 00 00 00 00 00 01 04 01 04 04 4c 54 5a "
		"39 00 00 01 04",-1,-1,NULL},
	{"b4 00 00 01 06 01 0b 00 00 00 00 00 00 00 01 02 01 02 02 54 53 00 "
		"00 01 05",-1,-1,NULL}
};
static const char *const	TZ_TABLE_ROWS[]={
	"02 c1 02|0d 78 7c 03 05 13 0f 10 07 5b ca 00 89 90|"
	"0d 78 7c 03 05 13 0f 10 07 54 d4 c0 89 90|"
	"0b 78 7c 03 05 13 0f 10 07 5b ca 00|0b 78 7c 03 05 13 0f 10 07 5b ca 00|"
	"0b 78 7c 03 05 0e 0f 10 07 5b ca 00",
	"02 c1 03|0d 78 7c 07 05 12 0f 10 07 5b ca 00 89 90|"
	"0d 78 7c 07 05 12 0f 10 07 54 d4 c0 89 90|"
	"0b 78 7c 07 05 12 0f 10 07 5b ca 00|0b 78 7c 07 05 12 0f 10 07 5b ca 00|"
	"0b 78 7c 07 05 0e 0f 10 07 5b ca 00",
	"02 c1 04|0d 78 7c 03 05 08 2d 10 07 5b ca 00 19 5a|"
	"0d 78 7c 03 05 08 2d 10 07 54 d4 c0 19 5a|"
	"0b 78 7c 03 05 08 2d 10 07 5b ca 00|0b 78 7c 03 05 08 2d 10 07 5b ca 00|"
	"0b 78 7c 03 05 0e 0f 10 07 5b ca 00",
	"02 c1 05|0d 78 7c 03 05 16 0f 10 07 5b ca 00 0c 3c|"
	"0d 78 7c 03 05 16 0f 10 07 54 d4 c0 0c 3c|"
	"0b 78 7c 03 05 16 0f 10 07 5b ca 00|0b 78 7c 03 05 16 0f 10 07 5b ca 00|"
	"0b 78 7c 03 05 0e 0f 10 07 5b ca 00",
	"02 c1 06|0d 78 7c 03 05 0e 0f 10 07 5b ca 00 d0 04|"
	"0d 78 7c 03 05 0e 0f 10 07 54 d4 c0 d0 04|"
	"0b 78 7c 03 05 0e 0f 10 07 5b ca 00|0b 78 7c 03 05 0e 0f 10 07 5b ca 00|"
	"0b 78 7c 03 05 0e 0f 10 07 5b ca 00",
	"02 c1 07|0d 78 7c 03 05 05 0f 10 07 5b ca 00 84 2c|"
	"0d 78 7c 03 05 05 0f 10 07 54 d4 c0 84 2c|"
	"0b 78 7c 03 05 05 0f 10 07 5b ca 00|0b 78 7c 03 05 05 0f 10 07 5b ca 00|"
	"0b 78 7c 03 05 0e 0f 10 07 5b ca 00",
	"02 c1 08|0d 77 aa 01 01 01 01 01 00 00 00 00 14 3c|"
	"0d 77 aa 01 01 01 01 01 00 00 00 00 14 3c|"
	"07 77 aa 01 01 01 01 01|07 77 aa 01 01 01 01 01|07 77 aa 01 01 01 01 01",
	"02 c1 09|0d 78 7c 0b 03 07 1f 01 00 00 00 00 89 90|"
	"0d 78 7c 0b 03 07 1f 01 00 00 00 00 89 90|"
	"07 78 7c 0b 03 07 1f 01|07 78 7c 0b 03 07 1f 01|07 78 7c 0b 03 02 1f 01",
	"02 c1 0a|0d 78 7c 0b 03 06 1f 01 00 00 00 00 89 90|"
	"0d 78 7c 0b 03 06 1f 01 00 00 00 00 89 90|"
	"07 78 7c 0b 03 06 1f 01|07 78 7c 0b 03 06 1f 01|07 78 7c 0b 03 02 1f 01",
	"02 c1 64|00|00|00|00|00"
};
static const ojdbc14colquery	TZ_TABLE={
	"select id, tz, tz3, ltz, ltz9, ts from protocoltest10749ttz order by id",
	"[0051] of 10749-dev-ojdbc14-coltz-realserver-oracle122",
	TZ_TABLE_COLUMNS,6,81,-1,NULL,
	"[0053]",TZ_TABLE_ROWS,10,OJDBC14_FETCH_ROWS
};

// coltz: the coltz action's table, as the colinterval session is
static void runColTz(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnDml(s,"drop table protocoltest10749ttz purge",
					0,SQLTYPE_DROP,true,&e,&pending);
	ojdbc14ColumnDml(s,"create table protocoltest10749ttz (id number, "
			"tz timestamp with time zone, "
			"tz3 timestamp(3) with time zone, "
			"ltz timestamp with local time zone, "
			"ltz9 timestamp(9) with local time zone, ts timestamp)",
			0,SQLTYPE_CREATE,false,&e,&pending);
	for (size_t i=0; i<sizeof(TZ_INSERT_ZONES)/
				sizeof(TZ_INSERT_ZONES[0]); i++) {
		const char	*z=TZ_INSERT_ZONES[i];
		stringbuffer	insert;
		char		id[8];
		charstring::printf(id,sizeof(id),"%d",(int)i+1);
		insert.append("insert into protocoltest10749ttz values (");
		insert.append(id);
		for (int c=0; c<4; c++) {
			insert.append(",timestamp '")->append(z)->append('\'');
		}
		insert.append(",cast(timestamp '")->append(z);
		insert.append("' as timestamp))");
		ojdbc14ColumnDml(s,insert.getString(),1,SQLTYPE_INSERT,false,
							&e,&pending);
	}
	ojdbc14ColumnDml(s,"insert into protocoltest10749ttz values "
				"(99,null,null,null,null,null)",
				1,SQLTYPE_INSERT,false,&e,&pending);
	if (!s->ok) {
		return;
	}
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	if (ojdbc14ColumnQuery(s,&TZ_TABLE,&e,&pending,k)) {
		struct {
			uint32_t	row;
			uint32_t	column;
			unsigned char	type;
			const char	*text;
		} expected[]={
			{0,1,181,"2024-03-05 18:14:15.123456000 region 612"},
			{2,1,181,"2024-03-05 07:44:15.123456000 +05:30"},
			{3,2,181,"2024-03-05 21:14:15.123000000 -08:00"},
			{0,3,231,"2024-03-05 18:14:15.123456000"},
			{6,3,231,"1970-01-01 00:00:00.000000000"},
			{0,5,180,"2024-03-05 13:14:15.123456000"}
		};
		for (size_t i=0; i<sizeof(expected)/sizeof(expected[0]); i++) {
			const oracleprotocolojdbc14value	*v=
				&k->values[expected[i].row][expected[i].column];
			stringbuffer	text;
			bool	ok=oracleprotocolclient::decodeOjdbc14Timestamp(
						expected[i].type,
						k->data+v->offset,v->size,
						&text);
			char	check[128];
			charstring::printf(check,sizeof(check),
					", row %d column %d reads as %s",
					(int)expected[i].row+1,
					(int)expected[i].column,
					expected[i].text);
			checkOjdbc14Decoded(s,TZ_TABLE.query,check,
					ok && !charstring::compare(
							text.getString(),
							expected[i].text),
					text.getString());
		}
	}
	delete k;
	if (!s->ok) {
		return;
	}
	ojdbc14ColumnDml(s,"drop table protocoltest10749ttz purge",
					0,SQLTYPE_DROP,false,&e,&pending);
	open[(*count)++]=pending;
}

// ---- coldb2 ----

// what the rules of #10749 that are for an oracle back end leave alone at a
// db2 one.  db2 describes a select of null as a VARCHAR(1), not 0 wide, so
// the module's null by describe can't be reached there, and the module's
// own describe and its 00 for the value stay what they were before #10749
// (MEASURED, HEAD 9b2f4bedc's module).  the nullable byte, which ojdbc14
// gets from the back end since #10749, is what the db2 connection reports:
// IBMREQD is CHAR(1) NOT NULL (MEASURED, sqlrsh's columninfo through the
// same db2 connection: Nullable false), where the module sent 1 before
static const ojdbc14colspec	DB2_NULL_COLUMNS[]={
	{"01 80 01 00 01 01 00 00 00 00 02 03 69 01 01 01 01 01 01 01 01 31 "
		"00 00 00",-1,-1,NULL}
};
static const char *const	DB2_NULL_ROWS[]={"00"};
static const ojdbc14colquery	DB2_NULL={
	"select null from sysibm.sysdummy1",
	"a VARCHAR2 of 1, the db2 connection's describe",
	DB2_NULL_COLUMNS,1,-1,-1,NULL,
	"a null",DB2_NULL_ROWS,1,OJDBC14_FETCH_ROWS,true
};
static const ojdbc14colspec	DB2_NOT_NULL_COLUMNS[]={
	{"60 80 01 00 01 01 00 00 00 00 02 03 69 01 01 01 00 07 01 07 07 49 "
		"42 4d 52 45 51 44 00 00 00",-1,-1,NULL}
};
static const char *const	DB2_NOT_NULL_ROWS[]={"01 59"};
static const ojdbc14colquery	DB2_NOT_NULL={
	"select ibmreqd from sysibm.sysdummy1",
	"a CHAR of 1, not nullable as the db2 connection reports it",
	DB2_NOT_NULL_COLUMNS,1,-1,-1,NULL,
	"Y",DB2_NOT_NULL_ROWS,1,OJDBC14_FETCH_ROWS,true
};

static void runColDb2(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t	e=3;
	uint32_t	pending=0;
	if (ojdbc14ColumnQuery(s,&DB2_NULL,&e,&pending,NULL)) {
		ojdbc14ColumnQuery(s,&DB2_NOT_NULL,&e,&pending,NULL);
	}
	open[(*count)++]=pending;
}

// select null: what a real server sends, dty 1 described 0 bytes wide in
// charset 1, and no value byte for the null (MEASURED, 10745-dev-ojdbc14-
// selectnull-realserver-oracle102 and -oracle122, and the 10749 ones).
// before #10749 the module sent a VARCHAR2 of 4000 in the listener's
// charset and a 00, which ojdbc14 read too, with a precision of 4000
static void runNull(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t	e=3;
	uint32_t	pending=0;
	ojdbc14ColumnQuery(s,&NULL1,&e,&pending,NULL);
	open[(*count)++]=pending;
}

// ---- binds (#10749) ----

// the options of the calls only the bind sessions send (MEASURED, section 5
// of step 2b of #10749, 10749-dev-ojdbc14-preprexec- and -preptz-realserver-
// oracle122): a dml re-execute that resends the descriptors because a value
// outgrew its mxl, a query's re-parse on its cursor after a bind changed
// type, and a PL/SQL block's call
static const uint32_t	OJDBC14_REEXECUTE_GROWN=0x8128;
static const uint32_t	OJDBC14_REPARSE_QUERY=0x8069;
static const uint32_t	OJDBC14_PLSQL=0x0529;

// the al8i4 elements of the re-parse, [0197] of 10749-dev-ojdbc14-preprexec-
// realserver-oracle122
static const uint32_t	AL8I4_REPARSE_QUERY[ORA_8I_AL8I4_SIZE]={
	1,10,0,0,0,0,0,1,0,0,0,0,0
};

// the charset ojdbc14 puts in every descriptor, the database's: the
// listener's 873 (AL32UTF8), or 31 (WE8ISO8859P1) at the charset 31 one
static const uint32_t	ORA_8I_CHARSET_WE8ISO8859P1=31;
static uint32_t ojdbc14Charset(ojdbc14session *s) {
	return (ojdbc14AtOracle102(s))?ORA_8I_CHARSET_WE8ISO8859P1:
					(uint32_t)ORA_CHARSET_AL32UTF8;
}

// how many bytes ojdbc14 counts per character of a string bind's mxl: 1 at
// a single byte charset and 4 at AL32UTF8 (MEASURED, section 2 of step 2b of
// #10749: 4 characters give mxl 16 at oracle122 and 4 at oracle102)
static uint32_t ojdbc14MaxCharSize(ojdbc14session *s) {
	return (ojdbc14AtOracle102(s))?1:4;
}

// the mxl ojdbc14 sends for a bind of "type" that isn't a string (MEASURED,
// section 2 of step 2b of #10749)
static uint32_t ojdbc14BindMaxLength(unsigned char type) {
	switch (type) {
		case 6:
			return 22;
		case 12:
			return 7;
		case 180:
		case 231:
		case 183:
			return 11;
		case 181:
			return 13;
		case 182:
			return 5;
		case 100:
			return 4;
		case 101:
			return 8;
		case 23:
			return 2000;
		case 11:
			return 130;
		default:
			return 0x7fffffff;
	}
}

// the values of a session's binds: bytes decoded from the hex the tables
// give, or made the way Conn14 makes them, kept until the session ends
struct ojdbc14bindbytes {
	unsigned char	data[262144];
	size_t		size;
};

static const unsigned char *ojdbc14BindHex(ojdbc14bindbytes *p,
						const char *hex, size_t *size) {
	*size=0;
	if (!hex) {
		return NULL;
	}
	unsigned char	*b=p->data+p->size;
	*size=ojdbc14Hex(hex,b,sizeof(p->data)-p->size);
	p->size+=*size;
	return b;
}

// Conn14's str(n), 'a' to 'z' over and over, its bytes(n), byte i is
// i*13+5, and the 'A' to 'Z' and i*7+1 values of its colwide action
enum ojdbc14pattern {
	PATTERN_LOWER,
	PATTERN_BYTES,
	PATTERN_UPPER,
	PATTERN_BYTES7
};

static const unsigned char *ojdbc14BindPattern(ojdbc14bindbytes *p,
						ojdbc14pattern pattern,
						size_t size) {
	if (p->size+size>sizeof(p->data)) {
		report("a session's bind values fit",false);
		return NULL;
	}
	unsigned char	*b=p->data+p->size;
	for (size_t i=0; i<size; i++) {
		switch (pattern) {
			case PATTERN_LOWER:
				b[i]=(unsigned char)('a'+i%26);
				break;
			case PATTERN_BYTES:
				b[i]=(unsigned char)(i*13+5);
				break;
			case PATTERN_UPPER:
				b[i]=(unsigned char)('A'+i%26);
				break;
			case PATTERN_BYTES7:
				b[i]=(unsigned char)(i*7+1);
				break;
		}
	}
	p->size+=size;
	return b;
}

// setInt()'s bind: dty 6, mxl 22, its value as an oracle NUMBER
static void ojdbc14BindInt(ojdbc14session *s, ojdbc14bindbytes *p,
				oracleprotocolojdbc14bind *b, uint32_t value) {
	unsigned char	*n=p->data+p->size;
	size_t		size=0;
	putOracleNumber(n,&size,value);
	p->size+=size;
	oracleprotocolclient::setOjdbc14Bind(b,6,22,ojdbc14Charset(s),
					ORA_8I_FORM_IMPLICIT,n,size);
}

// setString()'s bind: dty 1, mxl the characters times the charset's
// maximum, 1 character for a null or an empty string, which go out null
static void ojdbc14BindString(ojdbc14session *s, oracleprotocolojdbc14bind *b,
					const unsigned char *value, size_t size,
					size_t characters) {
	uint32_t	mxl=(uint32_t)((characters)?characters:1)*
						ojdbc14MaxCharSize(s);
	oracleprotocolclient::setOjdbc14Bind(b,1,mxl,ojdbc14Charset(s),
					ORA_8I_FORM_IMPLICIT,value,size);
}

// setFormOfUse(FORM_NCHAR) and setString()'s: dty 1, utf-16be, mxl its
// size, 2 for a null, charset 2000 and form of use 2
static void ojdbc14BindNchar(oracleprotocolojdbc14bind *b,
				const unsigned char *value, size_t size) {
	oracleprotocolclient::setOjdbc14Bind(b,1,(size)?(uint32_t)size:2,
					ORA_8I_CHARSET_AL16UTF16,
					ORA_8I_FORM_NCHAR,value,size);
}

// the bytes of a value, the first 64 of a long one, for a failure
static void dumpOjdbc14Head(const char *what, const unsigned char *bytes,
								size_t size) {
	dumpBytes(what,bytes,(size>64)?64:size);
	if (size>64) {
		stdoutput.printf("    ... %d bytes in all\n",(int)size);
	}
}

// what "select dump(c,16)" - or dump(c,1016) for a national column - says
// of a value stored as "bytes": "Typ=180 Len=7: 78,7c,3,5,1,1,1", or
// "Typ=1 Len=6 CharacterSet=AL16UTF16: 0,61,0,62,0,63" (MEASURED, the dump
// columns of the 10749-dev-ojdbc14-prep*-realserver captures), and "null"
// for a null, which nvl2() turns it into
static void ojdbc14DumpText(stringbuffer *t, unsigned int dumptype,
					bool national,
					const unsigned char *bytes,
					size_t size) {
	t->clear();
	if (!bytes) {
		t->append("null");
		return;
	}
	char	b[16];
	charstring::printf(b,sizeof(b),"Typ=%u Len=%u",dumptype,
							(unsigned int)size);
	t->append(b);
	if (national) {
		t->append(" CharacterSet=AL16UTF16");
	}
	t->append(":");
	for (size_t i=0; i<size; i++) {
		charstring::printf(b,sizeof(b),"%s%x",(i)?",":" ",bytes[i]);
		t->append(b);
	}
}

// one kept value against "expected", "size" bytes, NULL for a null
static void checkOjdbc14KeptValue(ojdbc14session *s, const char *what,
					const ojdbc14keptrows *k,
					uint32_t row, uint32_t column,
					const unsigned char *expected,
					size_t size, const char *name) {
	const oracleprotocolojdbc14value	*v=&k->values[row][column];
	bool	ok=(row<k->rowcount &&
			((!expected)?v->isnull:
				(!v->isnull && v->size==size &&
				!bytestring::compare(k->data+v->offset,
							expected,size))));
	char	check[512];
	charstring::printf(check,sizeof(check),", row %d column %d is %s",
					(int)row+1,(int)column,name);
	stringbuffer	l;
	ojdbc14Label(&l,s,what,check);
	report(l.getString(),ok);
	if (!ok) {
		if (row>=k->rowcount) {
			stdoutput.printf("    only %d rows\n",(int)k->rowcount);
		} else if (v->isnull) {
			stdoutput.printf("    got a null\n");
		} else {
			dumpOjdbc14Head("got",k->data+v->offset,v->size);
		}
		if (expected) {
			dumpOjdbc14Head("expected",expected,size);
		}
	}
}

// the answer to a query's 0x5e, then its fetches until ORA-01403, each
// checked the way ojdbc14ColumnQueryOptions() checks them, rows kept in
// "k".  "first" says the 0x5e carried no rows (a first execute, whose answer
// is the describe, the return parameters and the summary); otherwise it is
// a re-execute or re-parse with fetch, whose rows and ORA-01403 came in it.
// the cursor comes back
static uint32_t ojdbc14BindQueryRows(ojdbc14session *s, const char *what,
						uint32_t columncount,
						uint32_t rowcount,
						uint32_t rowsperfetch,
						uint32_t e,
						uint32_t *endtoend,
						ojdbc14keptrows *k) {

	bytestring::zero(k,sizeof(*k));
	checkOjdbc14NoMarkers(s,what);
	checkOjdbc14Objects(s,what,"10 08 04");
	checkOjdbc14DescribeHead(s,what,columncount);
	checkOjdbc14DescribeTail(s,what);
	for (int64_t i=0; i<s->answer->columncount &&
				i<(int64_t)ORA_8I_MAX_COLUMNS; i++) {
		charstring::copy(k->names[i],s->answer->columns[i].name);
		k->types[i]=s->answer->columns[i].type;
	}
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	checkOjdbc14Summary(s,what,e,0,0,c,3);

	uint32_t	total=0;
	bool		done=false;
	for (uint32_t f=0; s->ok && !done && f<rowcount/rowsperfetch+2; f++) {
		char	fetch[600];
		charstring::printf(fetch,sizeof(fetch),"%s, fetch %d",
							what,(int)f+1);
		if (!ojdbc14FetchRows(s,fetch,c,columncount,rowsperfetch,
								NULL,0)) {
			return c;
		}
		uint32_t	ee=nextEndToEnd(endtoend,0);
		oracleprotocolojdbc14answer	*a=s->answer;
		stringbuffer	codes;
		if (a->rowcount) {
			codes.append("06");
			for (size_t r=0; r<a->rowcount; r++) {
				codes.append(" 07");
			}
			codes.append(" 04");
		} else {
			codes.append("04");
		}
		checkOjdbc14Objects(s,fetch,codes.getString());
		keepOjdbc14Rows(s,k,columncount);
		total+=(uint32_t)a->rowcount;
		uint32_t	oranum=(a->rowcount<rowsperfetch)?1403:0;
		done=(oranum==1403);
		checkOjdbc14FetchSummary(s,fetch,ee,total,oranum,c,3);
	}

	char		check[64];
	charstring::printf(check,sizeof(check),", %d rows come back",
							(int)rowcount);
	stringbuffer	l;
	ojdbc14Label(&l,s,what,check);
	report(l.getString(),total==rowcount && done);
	if (total!=rowcount) {
		stdoutput.printf("    got %d\n",(int)total);
	}
	return c;
}

// a Statement's select (0x8021) behind the close of "*pending", and its rows
static bool ojdbc14BindSelect(ojdbc14session *s, const char *query,
					uint32_t columncount, uint32_t rowcount,
					uint32_t rowsperfetch,
					uint32_t *endtoend, uint32_t *pending,
					ojdbc14keptrows *k) {
	uint32_t	closecount=(*pending)?1:0;
	if (!ojdbc14Statement(s,query,OJDBC14_STATEMENT,0,query,
					AL8I4_QUERY,pending,closecount)) {
		return false;
	}
	uint32_t	e=nextEndToEnd(endtoend,closecount);
	*pending=ojdbc14BindQueryRows(s,query,columncount,rowcount,
					rowsperfetch,e,endtoend,k);
	return s->ok;
}

// a 0x5e with binds, built from "c", sent behind the close of "*pending"
// when it opens a cursor of its own (cursor 0), and its answer read.  its
// end to end field comes back in "*e"
static bool ojdbc14BindCall(ojdbc14session *s, const char *what,
				const oracleprotocolojdbc14bindcall *c,
				uint32_t *endtoend, uint32_t *pending,
				uint32_t *e) {
	uint32_t	closecount=(*pending && !c->cursorid)?1:0;
	if (s->ok) {
		s->client->buildOjdbc14BindCall(c,pending,closecount);
	}
	if (!sendOjdbc14Call(s,what,0,false,0)) {
		return false;
	}
	*e=nextEndToEnd(endtoend,closecount);
	if (closecount) {
		*pending=0;
	}
	return true;
}

// a dml call's answer with no error: the return parameters and "rows"
// touched.  "cursorid" is the cursor a re-execute named, or 0 for one the
// module opens, which is left in "*pending"
static void checkOjdbc14BindDml(ojdbc14session *s, const char *what,
					uint32_t e, uint32_t rows,
					unsigned char sqltype,
					uint32_t cursorid, uint32_t *pending) {
	checkOjdbc14NoMarkers(s,what);
	checkOjdbc14Objects(s,what,"08 04");
	uint32_t	c=checkOjdbc14ReturnParameters(s,what);
	if (cursorid) {
		stringbuffer	l;
		ojdbc14Label(&l,s,what," names the same cursor");
		report(l.getString(),c==cursorid);
	} else {
		*pending=c;
	}
	checkOjdbc14Summary(s,what,e,rows,0,c,sqltype);
}

// an error answering a call with binds: the markers, the error object alone,
// naming the call's cursor or, on cursor 0, one the module opened, which is
// left in "*pending"
static void checkOjdbc14BindError(ojdbc14session *s, const char *what,
					uint32_t e, uint32_t oranum,
					unsigned char sqltype,
					uint32_t successiterations,
					const ojdbc14message *m,
					uint32_t cursorid, uint32_t *pending) {
	uint32_t	c=checkOjdbc14ErrorCursor(s,what,cursorid);
	if (!cursorid) {
		*pending=c;
	}
	checkOjdbc14QueryError(s,what,e,0,oranum,c,sqltype,
						successiterations,m);
}

// a first execute of a dml statement with binds (0x8129, autocommit on, as
// Conn14 runs every one), on cursor 0 behind the close of "*pending", and
// its answer: "rows" touched, or, if "m" isn't NULL, the error "oranum"
static void ojdbc14BindInsert(ojdbc14session *s, const char *what,
					const char *query,
					const oracleprotocolojdbc14bind *binds,
					uint32_t bindcount,
					uint32_t *endtoend, uint32_t *pending,
					uint32_t rows, unsigned char sqltype,
					uint32_t oranum,
					const ojdbc14message *m) {
	oracleprotocolojdbc14bindcall	c;
	bytestring::zero(&c,sizeof(c));
	c.options=OJDBC14_PREPARED_COMMIT;
	c.query=query;
	c.al8i4=AL8I4_DML;
	c.binds=binds;
	c.bindcount=bindcount;
	c.descriptors=true;
	c.values=true;
	uint32_t	e=0;
	if (!ojdbc14BindCall(s,what,&c,endtoend,pending,&e)) {
		return;
	}
	if (m) {
		checkOjdbc14BindError(s,what,e,oranum,sqltype,0,m,0,pending);
	} else {
		checkOjdbc14BindDml(s,what,e,rows,sqltype,0,pending);
	}
}

// a first execute of a query with binds (0x8029) on cursor 0 behind the
// close of "*pending", and its rows
static bool ojdbc14BindQuery(ojdbc14session *s, const char *what,
					const char *query,
					const oracleprotocolojdbc14bind *binds,
					uint32_t bindcount,
					uint32_t columncount, uint32_t rowcount,
					uint32_t *endtoend, uint32_t *pending,
					ojdbc14keptrows *k) {
	oracleprotocolojdbc14bindcall	c;
	bytestring::zero(&c,sizeof(c));
	c.options=OJDBC14_PREPARED;
	c.query=query;
	c.al8i4=AL8I4_QUERY;
	c.binds=binds;
	c.bindcount=bindcount;
	c.descriptors=true;
	c.values=true;
	uint32_t	e=0;
	if (!ojdbc14BindCall(s,what,&c,endtoend,pending,&e)) {
		return false;
	}
	*pending=ojdbc14BindQueryRows(s,what,columncount,rowcount,
					OJDBC14_FETCH_ROWS,e,endtoend,k);
	return s->ok;
}

// the drop at the start of a bind session, which finds no table when the
// last run left none, and the create.  the cursor is left in "*pending"
static void ojdbc14BindTable(ojdbc14session *s, const char *table,
					const char *columns,
					uint32_t *endtoend, uint32_t *pending) {
	stringbuffer	drop;
	drop.append("drop table ")->append(table)->append(" purge");
	ojdbc14ColumnDml(s,drop.getString(),0,SQLTYPE_DROP,true,
							endtoend,pending);
	stringbuffer	create;
	create.append("create table ")->append(table)->append(" ");
	create.append(columns);
	ojdbc14ColumnDml(s,create.getString(),0,SQLTYPE_CREATE,false,
							endtoend,pending);
}

// the drop at the end of a bind session
static void ojdbc14BindDrop(ojdbc14session *s, const char *table,
					uint32_t *endtoend, uint32_t *pending) {
	stringbuffer	drop;
	drop.append("drop table ")->append(table)->append(" purge");
	ojdbc14ColumnDml(s,drop.getString(),0,SQLTYPE_DROP,false,
							endtoend,pending);
}

// ---- bind kinds ----

// one value of a bind kind: its dty, its mxl (0 for the type's, or for a
// string its size times the charset's maximum - see ojdbc14BindString()),
// whether it's a national (FORM_NCHAR) bind, its bytes as ojdbc14 sends
// them (NULL for a null) or "pattern" bytes of Conn14's (size "patternsize"),
// and, where what the back end stores differs from them by design, what it
// stores and why.  "utf8" marks a value only an AL32UTF8 back end session
// carries
struct ojdbc14bindvalue {
	unsigned char	type;
	uint32_t	mxl;
	bool		national;
	const char	*hex;
	int		pattern;
	size_t		patternsize;
	const char	*stored;
	const char	*why;
	bool		utf8;
};

// a kind of bind: the table it goes into, the column's type, the type dump()
// names it by and whether it dumps as a national string, its values, the
// capture the bytes are MEASURED in, and which value, if any, a "where c =
// :1" probe binds.  "direct" compares the column's own value with what was
// bound in place of its dump - for RAW and VARCHAR2 values too long to dump
struct ojdbc14bindkind {
	const char		*table;
	const char		*column;
	unsigned int		dumptype;
	bool			national;
	bool			direct;
	const ojdbc14bindvalue	*values;
	uint32_t		valuecount;
	const char		*source;
	int			probe;
};

// why what the back end stores differs from what was bound, by design (step
// 3 of #10749, design B.3 and follow-up F1: the server's bind api has no
// nanoseconds and no time zone region)
static const char	*WHY_NANOSECONDS=
	"the module binds a TIMESTAMP's nanoseconds cut to microseconds";
static const char	*WHY_REGION=
	"the module binds a TIMESTAMP WITH TIME ZONE in a named region at "
	"+00:00, the region lost";
static const char	*WHY_PADDED=
	"the bytes bound padded with blanks to the column's 10 characters, "
	"as a real server stores them";

// NUMBER: setInt, setLong, setDouble, setBigDecimal and setObject's
// values and their nulls (MEASURED, section 3.3 of step 2b of #10749, and
// the dump columns of 10749-dev-ojdbc14-preprexec-realserver-*), and setInt
// on a defaultNChar connection, charset 2000 and form of use 2 on a dty 6
// (MEASURED, [0023] of 10749-dev-ojdbc14-prepncharnc-realserver-oracle122)
static const ojdbc14bindvalue	NUMBER_VALUES[]={
	{6,0,false,"c1 02",0,0,NULL,NULL,false},
	{6,0,false,"c1 2b",0,0,NULL,NULL,false},
	{6,0,false,"c1 02 33",0,0,NULL,NULL,false},
	{6,0,false,"3f 33 66",0,0,NULL,NULL,false},
	{6,0,false,"c3 02 18 2e 44 5a",0,0,NULL,NULL,false},
	{6,0,false,"c7 02 18 2e 44 5a 02 18",0,0,NULL,NULL,false},
	{6,0,false,"35 5c 4f 44 1d 62 21 2f 18 2b 5d 66",0,0,NULL,NULL,false},
	{6,0,false,"3a 51 66",0,0,NULL,NULL,false},
	{6,0,false,"c5 15",0,0,NULL,NULL,false},
	{6,0,false,"0c 63 33 66",0,0,NULL,NULL,false},
	{6,0,false,"8f 02",0,0,NULL,NULL,false},
	{6,0,false,"35 59 43 2d 17 0b 59 43 2d 17 0b 59 43 2d 17 0b 66",
						0,0,NULL,NULL,false},
	{6,0,false,"d3 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 "
				"64",0,0,NULL,NULL,false},
	{6,0,false,"80",0,0,NULL,NULL,false},
	{6,0,false,"c1 04 1a",0,0,NULL,NULL,false},
	{6,0,true,"c1 02",0,0,NULL,NULL,false},
	{6,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	NUMBER_KIND={
	"protocoltest10749tbnum","number",2,false,false,
	NUMBER_VALUES,sizeof(NUMBER_VALUES)/sizeof(NUMBER_VALUES[0]),
	"10749-dev-ojdbc14-preprexec- and -prepncharnc-realserver",4
};

// DATE: setDate's 7 bytes into a DATE column and into a TIMESTAMP one, which
// a real server stores as the same 7 bytes, and a null (MEASURED, 10749-dev-
// ojdbc14-prepdate-realserver-*)
static const ojdbc14bindvalue	DATE_VALUES[]={
	{12,0,false,"78 7c 03 05 01 01 01",0,0,NULL,NULL,false},
	{12,0,false,"77 aa 01 01 01 01 01",0,0,NULL,NULL,false},
	{12,0,false,"77 64 0c 1f 01 01 01",0,0,NULL,NULL,false},
	{12,0,false,"c7 c7 0c 1f 01 01 01",0,0,NULL,NULL,false},
	{12,0,false,"78 7c 02 1d 01 01 01",0,0,NULL,NULL,false},
	{12,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	DATE_KIND={
	"protocoltest10749tbdate","date",12,false,false,
	DATE_VALUES,sizeof(DATE_VALUES)/sizeof(DATE_VALUES[0]),
	"10749-dev-ojdbc14-prepdate-realserver",0
};
static const ojdbc14bindkind	DATE_TS_KIND={
	"protocoltest10749tbdatets","timestamp(6)",180,false,false,
	DATE_VALUES,sizeof(DATE_VALUES)/sizeof(DATE_VALUES[0]),
	"10749-dev-ojdbc14-prepdate-realserver",-1
};

// TIMESTAMP: setTimestamp's 11 bytes, or 7 when the nanoseconds are 0, into
// a TIMESTAMP(9) column, which a real server stores unchanged (MEASURED,
// section 3.4 of step 2b of #10749, 10749-dev-ojdbc14-preptimestamp-
// realserver-*; the stored bytes are INFERRED from the select ? dumps there,
// the table's columns being TIMESTAMP(6))
static const ojdbc14bindvalue	TIMESTAMP_VALUES[]={
	{180,0,false,"78 7c 03 05 0e 0f 10 07 5b cd 15",0,0,
		"78 7c 03 05 0e 0f 10 07 5b ca 00",WHY_NANOSECONDS,false},
	{180,0,false,"78 7c 03 05 0e 0f 10",0,0,NULL,NULL,false},
	{180,0,false,"77 aa 01 01 01 01 01 00 00 00 01",0,0,
		"77 aa 01 01 01 01 01",WHY_NANOSECONDS,false},
	{180,0,false,"78 7c 03 05 01 01 01 1d cd 65 00",0,0,NULL,NULL,false},
	{180,0,false,"c7 c7 0c 1f 18 3c 3c 3b 9a c9 ff",0,0,
		"c7 c7 0c 1f 18 3c 3c 3b 9a c6 18",WHY_NANOSECONDS,false},
	{180,0,false,"77 64 01 01 01 01 01",0,0,NULL,NULL,false},
	{180,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	TIMESTAMP_KIND={
	"protocoltest10749tbts","timestamp(9)",180,false,false,
	TIMESTAMP_VALUES,sizeof(TIMESTAMP_VALUES)/sizeof(TIMESTAMP_VALUES[0]),
	"10749-dev-ojdbc14-preptimestamp-realserver",3
};

// TIMESTAMP WITH TIME ZONE: the UTC date and time, the nanoseconds, then an
// offset (hour+20, minute+60) or a region (MEASURED, section 3.4 of step 2b
// of #10749, 10749-dev-ojdbc14-preptz-realserver-*: +05:30 is 19 5a, UTC
// d0 04 and US/Eastern 89 90).  -04:00 is INFERRED from +05:30's rule
static const ojdbc14bindvalue	TSTZ_VALUES[]={
	{181,0,false,"78 7c 03 05 13 0f 10 07 5b ca 00 19 5a",0,0,
							NULL,NULL,false},
	{181,0,false,"78 7c 03 05 13 0f 10 07 5b ca 00 10 3c",0,0,
							NULL,NULL,false},
	{181,0,false,"78 7c 03 05 13 0f 10 07 5b ca 00 d0 04",0,0,
		"78 7c 03 05 13 0f 10 07 5b ca 00 14 3c",WHY_REGION,false},
	{181,0,false,"78 7c 03 05 13 0f 10 07 5b ca 00 89 90",0,0,
		"78 7c 03 05 13 0f 10 07 5b ca 00 14 3c",WHY_REGION,false},
	{181,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	TSTZ_KIND={
	"protocoltest10749tbtz","timestamp(9) with time zone",181,false,false,
	TSTZ_VALUES,sizeof(TSTZ_VALUES)/sizeof(TSTZ_VALUES[0]),
	"10749-dev-ojdbc14-preptz-realserver",-1
};

// TIMESTAMP WITH LOCAL TIME ZONE, which a real server stores unchanged
// (MEASURED, [0061] of 10749-dev-ojdbc14-preptz-realserver-oracle122 and
// its dump, every DBTIMEZONE being +00:00).  the 7 byte form is INFERRED from
// TIMESTAMP's
static const ojdbc14bindvalue	LTZ_VALUES[]={
	{231,0,false,"78 7c 03 05 0f 0f 10 07 5b ca 00",0,0,NULL,NULL,false},
	{231,0,false,"78 7c 03 05 0f 0f 10",0,0,NULL,NULL,false},
	{231,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	LTZ_KIND={
	"protocoltest10749tbltz","timestamp(9) with local time zone",231,
	false,false,LTZ_VALUES,sizeof(LTZ_VALUES)/sizeof(LTZ_VALUES[0]),
	"10749-dev-ojdbc14-preptz-realserver",-1
};

// INTERVAL YEAR TO MONTH and DAY TO SECOND, into columns wide enough to
// hold every value as sent (MEASURED bytes, section 3.4 of step 2b of
// #10749, 10749-dev-ojdbc14-preptz-realserver-*: +12-05, -01-02, +01-01,
// +3 04:05:06.123456789, "-1 00:00:00.5" as ojdbc14 parses it, -1 day and
// -5 nanoseconds, and +1 01:01:01.0)
static const ojdbc14bindvalue	YM_VALUES[]={
	{182,0,false,"80 00 00 0c 41",0,0,NULL,NULL,false},
	{182,0,false,"7f ff ff ff 3a",0,0,NULL,NULL,false},
	{182,0,false,"80 00 00 01 3d",0,0,NULL,NULL,false},
	{182,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	YM_KIND={
	"protocoltest10749tbym","interval year(9) to month",182,false,false,
	YM_VALUES,sizeof(YM_VALUES)/sizeof(YM_VALUES[0]),
	"10749-dev-ojdbc14-preptz-realserver",0
};
static const ojdbc14bindvalue	DS_VALUES[]={
	{183,0,false,"80 00 00 03 40 41 42 87 5b cd 15",0,0,NULL,NULL,false},
	{183,0,false,"7f ff ff ff 3c 3c 3c 7f ff ff fb",0,0,NULL,NULL,false},
	{183,0,false,"80 00 00 01 3d 3d 3d 80 00 00 00",0,0,NULL,NULL,false},
	{183,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	DS_KIND={
	"protocoltest10749tbds","interval day(9) to second(9)",183,false,
	false,DS_VALUES,sizeof(DS_VALUES)/sizeof(DS_VALUES[0]),
	"10749-dev-ojdbc14-preptz-realserver",0
};

// BINARY_FLOAT and BINARY_DOUBLE: 1.5, NaN, +Inf, -Inf, 0, MAX_VALUE,
// MIN_VALUE and MIN_NORMAL in their sortable form, which a real server
// stores unchanged (MEASURED, section 3.5 of step 2b of #10749 and the dump
// columns of 10749-dev-ojdbc14-prepbinfloat-realserver-*), and -1.5 and 0.1,
// whose bytes are the same rule's (INFERRED, no real server was sent them)
static const ojdbc14bindvalue	BF_VALUES[]={
	{100,0,false,"bf c0 00 00",0,0,NULL,NULL,false},
	{100,0,false,"ff c0 00 00",0,0,NULL,NULL,false},
	{100,0,false,"ff 80 00 00",0,0,NULL,NULL,false},
	{100,0,false,"00 7f ff ff",0,0,NULL,NULL,false},
	{100,0,false,"80 00 00 00",0,0,NULL,NULL,false},
	{100,0,false,"ff 7f ff ff",0,0,NULL,NULL,false},
	{100,0,false,"80 00 00 01",0,0,NULL,NULL,false},
	{100,0,false,"80 80 00 00",0,0,NULL,NULL,false},
	{100,0,false,"40 3f ff ff",0,0,NULL,NULL,false},
	{100,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	BF_KIND={
	"protocoltest10749tbbf","binary_float",100,false,false,
	BF_VALUES,sizeof(BF_VALUES)/sizeof(BF_VALUES[0]),
	"10749-dev-ojdbc14-prepbinfloat-realserver",0
};
static const ojdbc14bindvalue	BD_VALUES[]={
	{101,0,false,"bf f8 00 00 00 00 00 00",0,0,NULL,NULL,false},
	{101,0,false,"ff f8 00 00 00 00 00 00",0,0,NULL,NULL,false},
	{101,0,false,"ff f0 00 00 00 00 00 00",0,0,NULL,NULL,false},
	{101,0,false,"00 0f ff ff ff ff ff ff",0,0,NULL,NULL,false},
	{101,0,false,"80 00 00 00 00 00 00 00",0,0,NULL,NULL,false},
	{101,0,false,"ff ef ff ff ff ff ff ff",0,0,NULL,NULL,false},
	{101,0,false,"80 00 00 00 00 00 00 01",0,0,NULL,NULL,false},
	{101,0,false,"80 10 00 00 00 00 00 00",0,0,NULL,NULL,false},
	{101,0,false,"bf b9 99 99 99 99 99 9a",0,0,NULL,NULL,false},
	{101,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	BD_KIND={
	"protocoltest10749tbbd","binary_double",101,false,false,
	BD_VALUES,sizeof(BD_VALUES)/sizeof(BD_VALUES[0]),
	"10749-dev-ojdbc14-prepbinfloat-realserver",0
};

// RAW: setBytes of 1, 3, 64, 65 and 2000 of Conn14's bytes, and a null, as
// ojdbc14 sends them - dty 23, mxl 2000 whatever the size (MEASURED,
// 10749-dev-ojdbc14-prepbytes-realserver-*), compared as they come back
static const ojdbc14bindvalue	RAW_VALUES[]={
	{23,0,false,NULL,PATTERN_BYTES,1,NULL,NULL,false},
	{23,0,false,NULL,PATTERN_BYTES,3,NULL,NULL,false},
	{23,0,false,NULL,PATTERN_BYTES,64,NULL,NULL,false},
	{23,0,false,NULL,PATTERN_BYTES,65,NULL,NULL,false},
	{23,0,false,NULL,PATTERN_BYTES,2000,NULL,NULL,false},
	{23,0,false,NULL,0,0,NULL,NULL,false}
};
static const ojdbc14bindkind	RAW_KIND={
	"protocoltest10749tbraw","raw(2000)",23,false,true,
	RAW_VALUES,sizeof(RAW_VALUES)/sizeof(RAW_VALUES[0]),
	"10749-dev-ojdbc14-prepbytes-realserver",2
};

// VARCHAR2: setString of Conn14's str(n) across the clr's 64 byte chunk
// (63, 64, 65, 128), the module's own 252 byte short form (252, 253) and up
// to 4000 bytes, an empty string, which ojdbc14 sends as a null, and, at the
// AL32UTF8 listener, "cafe" with an acute e, a space, u umlaut and n tilde,
// 7 characters in 10 bytes, mxl 28 (MEASURED, 10749-dev-ojdbc14-prepstring-
// and -prepnchar-realserver-oracle122).  at 873, 4000 characters have mxl
// 16000, so the value is one ojdbc14 sends after the others - the last
// anyway here
static const ojdbc14bindvalue	STRING_VALUES[]={
	{1,0,false,NULL,PATTERN_LOWER,1,NULL,NULL,false},
	{1,0,false,NULL,PATTERN_LOWER,63,NULL,NULL,false},
	{1,0,false,NULL,PATTERN_LOWER,64,NULL,NULL,false},
	{1,0,false,NULL,PATTERN_LOWER,65,NULL,NULL,false},
	{1,0,false,NULL,PATTERN_LOWER,128,NULL,NULL,false},
	{1,0,false,NULL,PATTERN_LOWER,252,NULL,NULL,false},
	{1,0,false,NULL,PATTERN_LOWER,253,NULL,NULL,false},
	{1,0,false,NULL,PATTERN_LOWER,4000,NULL,NULL,false},
	{1,0,false,NULL,0,0,NULL,NULL,false},
	{1,28,false,"63 61 66 c3 a9 20 c3 bc c3 b1",0,0,NULL,NULL,true}
};
static const ojdbc14bindkind	STRING_KIND={
	"protocoltest10749tbstr","varchar2(4000)",1,false,true,
	STRING_VALUES,sizeof(STRING_VALUES)/sizeof(STRING_VALUES[0]),
	"10749-dev-ojdbc14-prepstring-realserver",2
};

// NVARCHAR2 and NCHAR(10): FORM_NCHAR setString, utf-16be, mxl twice the
// characters, 2 for a null (MEASURED, 10749-dev-ojdbc14-prepnchar-
// realserver-*, and the dump columns there, NCHAR padded with 00 20).  the
// AL32UTF8 listener also gets prepnchar's 7 character "cafe" string, its 3
// CJK characters and U+1F600 as a surrogate pair (the last INFERRED from
// AL16UTF16, no real server was sent it)
static const ojdbc14bindvalue	NVARCHAR_VALUES[]={
	{1,0,true,"00 61 00 62 00 63",0,0,NULL,NULL,false},
	{1,0,true,"00 78",0,0,NULL,NULL,false},
	{1,0,true,NULL,0,0,NULL,NULL,false},
	{1,0,true,"00 63 00 61 00 66 00 e9 00 20 00 fc 00 f1",0,0,
							NULL,NULL,true},
	{1,0,true,"65 e5 67 2c 8a 9e",0,0,NULL,NULL,true},
	{1,0,true,"00 61 d8 3d de 00 00 62",0,0,NULL,NULL,true}
};
static const ojdbc14bindkind	NVARCHAR_KIND={
	"protocoltest10749tbnv","nvarchar2(100)",1,true,false,
	NVARCHAR_VALUES,sizeof(NVARCHAR_VALUES)/sizeof(NVARCHAR_VALUES[0]),
	"10749-dev-ojdbc14-prepnchar-realserver",0
};
static const ojdbc14bindvalue	NCHAR_VALUES[]={
	{1,0,true,"00 61 00 62 00 63",0,0,
		"00 61 00 62 00 63 00 20 00 20 00 20 00 20 00 20 00 20 00 20",
		WHY_PADDED,false},
	{1,0,true,NULL,0,0,NULL,NULL,false},
	{1,0,true,"00 63 00 61 00 66 00 e9 00 20 00 fc 00 f1",0,0,
		"00 63 00 61 00 66 00 e9 00 20 00 fc 00 f1 00 20 00 20 00 20",
		WHY_PADDED,true},
	{1,0,true,"65 e5 67 2c 8a 9e",0,0,
		"65 e5 67 2c 8a 9e 00 20 00 20 00 20 00 20 00 20 00 20 00 20",
		WHY_PADDED,true}
};
static const ojdbc14bindkind	NCHAR_KIND={
	"protocoltest10749tbnc","nchar(10)",96,true,false,
	NCHAR_VALUES,sizeof(NCHAR_VALUES)/sizeof(NCHAR_VALUES[0]),
	"10749-dev-ojdbc14-prepnchar-realserver",-1
};

// one value of a kind as its bind, and the bytes the back end has to store
// for it ("*stored", NULL for a null).  false if the value is one the
// session's back end can't carry
static bool ojdbc14KindBind(ojdbc14session *s, ojdbc14bindbytes *p,
					const ojdbc14bindvalue *v,
					oracleprotocolojdbc14bind *b,
					const unsigned char **stored,
					size_t *storedsize) {
	if (v->utf8 && !ojdbc14AtUtf8(s)) {
		return false;
	}
	size_t			size=0;
	const unsigned char	*value=(v->patternsize)?
				ojdbc14BindPattern(p,(ojdbc14pattern)v->pattern,
							v->patternsize):
				ojdbc14BindHex(p,v->hex,&size);
	if (v->patternsize) {
		size=v->patternsize;
	}
	if (v->national) {
		if (v->type==6) {
			oracleprotocolclient::setOjdbc14Bind(b,6,22,
					ORA_8I_CHARSET_AL16UTF16,
					ORA_8I_FORM_NCHAR,value,size);
		} else {
			ojdbc14BindNchar(b,value,size);
		}
	} else if (v->type==1) {
		ojdbc14BindString(s,b,value,size,size);
	} else {
		oracleprotocolclient::setOjdbc14Bind(b,v->type,
					ojdbc14BindMaxLength(v->type),
					ojdbc14Charset(s),
					ORA_8I_FORM_IMPLICIT,value,size);
	}
	if (v->mxl) {
		b->maxlength=v->mxl;
	}
	*stored=value;
	*storedsize=size;
	if (v->stored) {
		*stored=ojdbc14BindHex(p,v->stored,storedsize);
	}
	return true;
}

// one kind of bind: its table, a 0x8129 insert of each value with setInt's
// id ahead of it, then every row read back - each value's dump(), which a
// real server's shows to be the bytes it was sent, or for a "direct" kind
// the value itself - and, if the kind has one, a select whose where clause
// binds a value, which has to find each row holding it.  the table is
// dropped at the end
static void ojdbc14BindKind(ojdbc14session *s, const ojdbc14bindkind *kind,
					ojdbc14bindbytes *p,
					uint32_t *endtoend, uint32_t *pending) {

	stringbuffer	columns;
	columns.append("(id number, c ")->append(kind->column)->append(")");
	ojdbc14BindTable(s,kind->table,columns.getString(),endtoend,pending);
	if (!s->ok) {
		return;
	}

	stringbuffer	insert;
	insert.append("insert into ")->append(kind->table);
	insert.append(" (id, c) values (:1,:2)");

	const unsigned char	**stored=
			new const unsigned char *[kind->valuecount];
	size_t			*storedsize=new size_t[kind->valuecount];
	bool			*sent=new bool[kind->valuecount];
	uint32_t		rows=0;
	for (uint32_t i=0; s->ok && i<kind->valuecount; i++) {
		oracleprotocolojdbc14bind	b[2];
		ojdbc14BindInt(s,p,&b[0],i+1);
		sent[i]=ojdbc14KindBind(s,p,&kind->values[i],&b[1],
						&stored[i],&storedsize[i]);
		if (!sent[i]) {
			continue;
		}
		char	what[256];
		charstring::printf(what,sizeof(what),
				"%s, value %d (dty %d, %d bytes)",
				insert.getString(),(int)i+1,
				(int)kind->values[i].type,
				(int)b[1].valuesize);
		ojdbc14BindInsert(s,what,insert.getString(),b,2,
					endtoend,pending,1,SQLTYPE_INSERT,
					0,NULL);
		rows++;
	}

	// read them back
	stringbuffer	select;
	if (kind->direct) {
		select.append("select id, c from ")->append(kind->table);
	} else {
		select.append("select id, nvl2(c,dump(c,");
		select.append((kind->national)?"1016":"16");
		select.append("),'null') from ")->append(kind->table);
	}
	select.append(" order by id");
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	if (s->ok && ojdbc14BindSelect(s,select.getString(),2,rows,
					OJDBC14_FETCH_ROWS,endtoend,
					pending,k)) {
		uint32_t	r=0;
		for (uint32_t i=0; i<kind->valuecount; i++) {
			if (!sent[i]) {
				continue;
			}
			const ojdbc14bindvalue	*v=&kind->values[i];
			char	name[512];
			if (kind->direct) {
				charstring::printf(name,sizeof(name),
					"the %d bytes bound, as %s sent them",
					(int)storedsize[i],kind->source);
				checkOjdbc14KeptValue(s,select.getString(),k,
						r,1,stored[i],storedsize[i],
						(stored[i])?name:"null");
			} else {
				stringbuffer	dump;
				ojdbc14DumpText(&dump,kind->dumptype,
						kind->national,stored[i],
						storedsize[i]);
				const char	*why=(v->stored)?v->why:
						(stored[i])?"the bytes bound, "
						"as a real server stores them":
						"a null";
				charstring::printf(name,sizeof(name),
					"\"%.100s%s\", %s",dump.getString(),
					(dump.getSize()>100)?"...":"",why);
				stringbuffer	what;
				what.append(select.getString());
				what.append(", the ")->append(kind->source);
				what.append(" value");
				checkOjdbc14KeptValue(s,what.getString(),k,r,1,
					(const unsigned char *)dump.getString(),
					dump.getSize(),name);
			}
			r++;
		}
	}

	// a where clause that binds a value
	if (s->ok && kind->probe>=0 && sent[kind->probe]) {
		oracleprotocolojdbc14bind	b;
		const unsigned char		*ignored=NULL;
		size_t				ignoredsize=0;
		ojdbc14KindBind(s,p,&kind->values[kind->probe],&b,
						&ignored,&ignoredsize);
		stringbuffer	where;
		where.append("select id from ")->append(kind->table);
		where.append(" where c = :1 order by id");
		uint32_t	matches=0;
		for (uint32_t i=0; i<kind->valuecount; i++) {
			if (sent[i] && stored[i] &&
				storedsize[i]==storedsize[kind->probe] &&
				!bytestring::compare(stored[i],
						stored[kind->probe],
						storedsize[i])) {
				matches++;
			}
		}
		if (ojdbc14BindQuery(s,where.getString(),where.getString(),
					&b,1,1,matches,endtoend,pending,k)) {
			unsigned char	id[8];
			size_t		idsize=0;
			putOracleNumber(id,&idsize,kind->probe+1);
			checkOjdbc14KeptValue(s,where.getString(),k,0,0,
					id,idsize,"the id of the row bound");
		}
	}
	delete k;
	delete[] stored;
	delete[] storedsize;
	delete[] sent;

	ojdbc14BindDrop(s,kind->table,endtoend,pending);
}

// the bind sessions' errors: the back end's, as real servers send them or as
// the module's back end answers where a real server doesn't (MEASURED, step
// 2b of #10749 for the real servers, step 2c for the module, the prepstring
// and prepbytes actions), and the module's own for a bind it can't take
// (design B.4 of #10749)
static const ojdbc14message	MSG_12899_CHAR={
	"ORA-12899: value too large for column \"",
	"\".\"PROTOCOLTEST10749TBSTR2\".\"C\" (actual: 4000, maximum: 2000)\n"
};
static const ojdbc14message	MSG_12899_VARCHAR={
	"ORA-12899: value too large for column \"",
	"\".\"PROTOCOLTEST10749TBSTR2\".\"V\" (actual: 4001, maximum: 4000)\n"
};
static const ojdbc14message	MSG_01461={
	"ORA-01461: can bind a LONG value only for insert into a LONG "
	"column\n",NULL
};
static const ojdbc14message	MSG_01460={
	"ORA-01460: unimplemented or unreasonable conversion requested\n",NULL
};
static const ojdbc14message	MSG_22835={
	"ORA-22835: Buffer too small for CLOB to CHAR or BLOB to RAW "
	"conversion (actual: 2001, maximum: 2000)\n",NULL
};
static const ojdbc14message	MSG_20008={
	"ORA-20008: Bind variable data type not supported.\n",NULL
};
static const ojdbc14message	MSG_20009={
	"ORA-20009: Bind variable value could not be decoded.\n",NULL
};

// the statement type of a select's answer
static const unsigned char	SQLTYPE_SELECT=3;
static const unsigned char	SQLTYPE_UPDATE=6;

// ---- bindnumber, binddate, bindtz, bindbinfloat ----

// each kind's values inserted and read back, at every listener that runs it
static void runBindNumber(ojdbc14session *s, uint32_t *open,
						uint32_t *count) {
	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14BindKind(s,&NUMBER_KIND,p,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

static void runBindDate(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14BindKind(s,&DATE_KIND,p,&e,&pending);
	ojdbc14BindKind(s,&DATE_TS_KIND,p,&e,&pending);
	ojdbc14BindKind(s,&TIMESTAMP_KIND,p,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

static void runBindTz(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14BindKind(s,&TSTZ_KIND,p,&e,&pending);
	ojdbc14BindKind(s,&LTZ_KIND,p,&e,&pending);
	ojdbc14BindKind(s,&YM_KIND,p,&e,&pending);
	ojdbc14BindKind(s,&DS_KIND,p,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

static void runBindBinaryFloat(ojdbc14session *s, uint32_t *open,
							uint32_t *count) {
	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14BindKind(s,&BF_KIND,p,&e,&pending);
	ojdbc14BindKind(s,&BD_KIND,p,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

// ---- bindraw ----

// the RAW kind, then setBytes of 2001 bytes, which ojdbc14 sends as a LONG
// RAW (dty 24, mxl 0x7fffffff; MEASURED, [0046] and [0051] of 10749-dev-
// ojdbc14-prepbytes-realserver-oracle122), into a RAW(2000) and a LONG RAW
// column.  a real server answers the first ORA-12899 and stores the second;
// the module binds a LONG RAW as a temporary BLOB, and the back end answers
// both ORA-22835 (MEASURED, step 2c of #10749; by design, follow-up F1).
// then "select :1 r from dual" of 64 bytes, which a real server answers with
// the RAW and the module with its hex, upper case, as a VARCHAR2, since it
// binds a RAW as its hex (design B.3 and decision E2 of #10749)
static void runBindRaw(ojdbc14session *s, uint32_t *open, uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14BindKind(s,&RAW_KIND,p,&e,&pending);

	const char	*lraw="protocoltest10749tblraw";
	ojdbc14BindTable(s,lraw,"(id number, r raw(2000), l long raw)",
							&e,&pending);
	const unsigned char	*bytes=ojdbc14BindPattern(p,PATTERN_BYTES,2001);
	oracleprotocolojdbc14bind	b[2];
	ojdbc14BindInt(s,p,&b[0],1);
	oracleprotocolclient::setOjdbc14Bind(&b[1],24,0x7fffffff,
				ojdbc14Charset(s),ORA_8I_FORM_IMPLICIT,
				bytes,2001);
	ojdbc14BindInsert(s,"setBytes of 2001 bytes into a raw(2000), "
				"ORA-22835 where a real server answers "
				"ORA-12899 (the module binds a LONG RAW as a "
				"BLOB)",
			"insert into protocoltest10749tblraw (id, r) values "
			"(:1,:2)",b,2,&e,&pending,0,SQLTYPE_INSERT,
			22835,&MSG_22835);
	ojdbc14BindInsert(s,"setBytes of 2001 bytes into a long raw, "
				"ORA-22835 where a real server stores it (the "
				"module binds a LONG RAW as a BLOB)",
			"insert into protocoltest10749tblraw (id, l) values "
			"(:1,:2)",b,2,&e,&pending,0,SQLTYPE_INSERT,
			22835,&MSG_22835);
	ojdbc14BindDrop(s,lraw,&e,&pending);

	const unsigned char	*bytes64=ojdbc14BindPattern(p,PATTERN_BYTES,64);
	oracleprotocolclient::setOjdbc14Bind(&b[0],23,2000,ojdbc14Charset(s),
					ORA_8I_FORM_IMPLICIT,bytes64,64);
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	const char	*query="select :1 r from dual";
	if (ojdbc14BindQuery(s,query,query,b,1,1,1,&e,&pending,k)) {
		char	hex[129];
		for (size_t i=0; i<64; i++) {
			charstring::printf(hex+2*i,3,"%02X",bytes64[i]);
		}
		stringbuffer	l;
		ojdbc14Label(&l,s,query," describes a VARCHAR2, where a real "
				"server describes a RAW (the module binds a "
				"RAW as its hex)");
		report(l.getString(),k->types[0]==1);
		if (k->types[0]!=1) {
			stdoutput.printf("    got type %d\n",(int)k->types[0]);
		}
		checkOjdbc14KeptValue(s,query,k,0,0,
				(const unsigned char *)hex,128,
				"the 64 bytes' hex, upper case, where a real "
				"server sends the bytes (the module binds a "
				"RAW as its hex)");
	}
	delete k;
	delete p;
	open[(*count)++]=pending;
}

// ---- bindstring ----

// the VARCHAR2 kind, then setString past what the columns hold: 4000
// characters into a CHAR(2000), ORA-12899 from a real server and from the
// module's back end; 4001 into a VARCHAR2(4000), ORA-01461 from a real
// server and from a 10.2 back end, ORA-12899 from a 12.2 back end; and
// "select :1 v, length(:2) l from dual" of 4001 characters, ORA-01460 from a
// real server and from a 10.2 back end, where a 12.2 back end returns the
// value cut to 4000 and its length (MEASURED, 10749-dev-ojdbc14-prepstring-
// realserver-oracle122, at oracle102 and oracle112 in captures not kept,
// and step 2c of #10749; the 12.2 back end's are by design,
// design B.7 and follow-up F1: OCI binds a long string differently)
static void runBindString(ojdbc14session *s, uint32_t *open,
						uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	bool			at102=ojdbc14AtOracle102(s);
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14BindKind(s,&STRING_KIND,p,&e,&pending);

	const char	*table="protocoltest10749tbstr2";
	ojdbc14BindTable(s,table,"(id number, v varchar2(4000), c char(2000))",
							&e,&pending);
	const unsigned char	*str4000=
				ojdbc14BindPattern(p,PATTERN_LOWER,4000);
	const unsigned char	*str4001=
				ojdbc14BindPattern(p,PATTERN_LOWER,4001);
	oracleprotocolojdbc14bind	b[2];
	ojdbc14BindInt(s,p,&b[0],1);
	ojdbc14BindString(s,&b[1],str4000,4000,4000);
	ojdbc14BindInsert(s,"setString of 4000 characters into a char(2000), "
				"ORA-12899 as from a real server",
			"insert into protocoltest10749tbstr2 (id, c) values "
			"(:1,:2)",b,2,&e,&pending,0,SQLTYPE_INSERT,
			12899,&MSG_12899_CHAR);
	ojdbc14BindString(s,&b[1],str4001,4001,4001);
	ojdbc14BindInsert(s,(at102)?
			"setString of 4001 characters into a varchar2(4000), "
			"ORA-01461 as from a real server":
			"setString of 4001 characters into a varchar2(4000), "
			"ORA-12899 where a real server answers ORA-01461 (OCI "
			"binds a long string differently)",
			"insert into protocoltest10749tbstr2 (id, v) values "
			"(:1,:2)",b,2,&e,&pending,0,SQLTYPE_INSERT,
			(at102)?1461:12899,
			(at102)?&MSG_01461:&MSG_12899_VARCHAR);
	ojdbc14BindDrop(s,table,&e,&pending);

	ojdbc14BindString(s,&b[0],str4001,4001,4001);
	ojdbc14BindString(s,&b[1],str4001,4001,4001);
	const char	*query="select :1 v, length(:2) l from dual";
	if (at102) {
		oracleprotocolojdbc14bindcall	c;
		bytestring::zero(&c,sizeof(c));
		c.options=OJDBC14_PREPARED;
		c.query=query;
		c.al8i4=AL8I4_QUERY;
		c.binds=b;
		c.bindcount=2;
		c.descriptors=true;
		c.values=true;
		uint32_t	ee=0;
		const char	*what="select :1 v, length(:2) l from dual of "
					"4001 characters, ORA-01460 as from a "
					"real server";
		if (ojdbc14BindCall(s,what,&c,&e,&pending,&ee)) {
			checkOjdbc14BindError(s,what,ee,1460,SQLTYPE_SELECT,0,
						&MSG_01460,0,&pending);
		}
	} else {
		ojdbc14keptrows	*k=new ojdbc14keptrows;
		if (ojdbc14BindQuery(s,query,query,b,2,2,1,&e,&pending,k)) {
			const oracleprotocolojdbc14value	*v=
							&k->values[0][0];
			stringbuffer	l;
			ojdbc14Label(&l,s,query,", row 1 column 0 is the 4001 "
				"characters, or the first 4000, where a real "
				"server answers ORA-01460 (OCI binds a long "
				"string differently, and the back end's "
				"describe of it follows its cursor's history)");
			bool	ok=(k->rowcount==1 && !v->isnull &&
					(v->size==4000 || v->size==4001) &&
					!bytestring::compare(k->data+v->offset,
							str4001,v->size));
			report(l.getString(),ok);
			if (!ok) {
				dumpOjdbc14Head("got",k->data+v->offset,
								v->size);
			}
			checkOjdbc14KeptValue(s,query,k,0,1,
				(const unsigned char *)"\xc2\x29\x02",3,
				"4001, the length of the value bound");
		}
		delete k;
	}
	delete p;
	open[(*count)++]=pending;
}

// ---- bindnchar ----

static void runBindNchar(ojdbc14session *s, uint32_t *open, uint32_t *count) {
	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14BindKind(s,&NVARCHAR_KIND,p,&e,&pending);
	ojdbc14BindKind(s,&NCHAR_KIND,p,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

// ---- bindnull ----

// the setNull insert of prepnull, ten binds in one call: setInt, then
// setNull of VARCHAR (dty 1, mxl 1 character), NUMERIC, INTEGER (6), DATE
// (12), TIMESTAMP (180), BINARY (23), CHAR (1), DOUBLE and FLOAT (6), each
// value a lone 00 (MEASURED, [0099] of 10749-dev-ojdbc14-prepnull-
// realserver-oracle122), and the nulls of the other types, TIMESTAMP WITH
// TIME ZONE (181), WITH LOCAL TIME ZONE (231), both INTERVALs (182, 183),
// BINARY_FLOAT and BINARY_DOUBLE (100, 101) and ROWID (11), the same way
// (MEASURED, [0089] of -preptz- and the null inserts of -prepbinfloat-
// realserver-oracle122; the ROWID's, setROWID(null), [0061] of -preprowid-).
// every column has to come back null.  then prepnull's insert of nulls and
// its re-execute with values, which keeps the descriptors (0x8120; [0103]
// and [0105] there)
static void runBindNull(ojdbc14session *s, uint32_t *open, uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	uint32_t		cs=ojdbc14Charset(s);
	uint32_t		r=ojdbc14MaxCharSize(s);

	const char	*table="protocoltest10749tbnull";
	ojdbc14BindTable(s,table,"(id number, v varchar2(10), n number, "
			"i integer, d date, ts timestamp, r raw(10), "
			"c char(5), db number, f number, tz timestamp with "
			"time zone, ltz timestamp with local time zone, "
			"ym interval year to month, ds interval day to second, "
			"bf binary_float, bd binary_double, rid rowid)",
			&e,&pending);

	static const unsigned char	types[]={
		1,6,6,12,180,23,1,6,6,181,231,182,183,100,101,11
	};
	static const char		*columns[]={
		"v","n","i","d","ts","r","c","db","f","tz","ltz","ym","ds",
		"bf","bd","rid"
	};
	const uint32_t			nulls=sizeof(types)/sizeof(types[0]);
	oracleprotocolojdbc14bind	b[1+sizeof(types)/sizeof(types[0])];
	ojdbc14BindInt(s,p,&b[0],1);
	for (uint32_t i=0; i<nulls; i++) {
		oracleprotocolclient::setOjdbc14Bind(&b[1+i],types[i],
				(types[i]==1)?r:ojdbc14BindMaxLength(types[i]),
				cs,ORA_8I_FORM_IMPLICIT,NULL,0);
	}
	stringbuffer	insert;
	insert.append("insert into protocoltest10749tbnull (id");
	for (uint32_t i=0; i<nulls; i++) {
		insert.append(", ")->append(columns[i]);
	}
	insert.append(") values (:1");
	for (uint32_t i=0; i<nulls; i++) {
		char	n[8];
		charstring::printf(n,sizeof(n),",:%d",(int)i+2);
		insert.append(n);
	}
	insert.append(")");
	ojdbc14BindInsert(s,"the setNull insert, a null of every type",
				insert.getString(),b,1+nulls,&e,&pending,1,
				SQLTYPE_INSERT,0,NULL);

	stringbuffer	select;
	select.append("select id");
	for (uint32_t i=0; i<nulls; i++) {
		select.append(", nvl2(")->append(columns[i]);
		select.append(",'set','null')");
	}
	select.append(" from protocoltest10749tbnull where id = 1");
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	if (s->ok && ojdbc14BindSelect(s,select.getString(),1+nulls,1,
				OJDBC14_FETCH_ROWS,&e,&pending,k)) {
		for (uint32_t i=0; i<nulls; i++) {
			char	name[64];
			charstring::printf(name,sizeof(name),
					"null, the dty %d null bound",
					(int)types[i]);
			checkOjdbc14KeptValue(s,"the setNull insert's row",
					k,0,1+i,(const unsigned char *)"null",4,
					name);
		}
	}

	// prepnull's insert of nulls and its re-execute with values
	const char	*query="insert into protocoltest10749tbnull (id, v, n) "
							"values (:1,:2,:3)";
	ojdbc14BindInt(s,p,&b[0],3);
	ojdbc14BindString(s,&b[1],NULL,0,0);
	oracleprotocolclient::setOjdbc14Bind(&b[2],6,22,cs,
					ORA_8I_FORM_IMPLICIT,NULL,0);
	ojdbc14BindInsert(s,"setInt(3), setNull(VARCHAR), setNull(NUMERIC)",
				query,b,3,&e,&pending,1,SQLTYPE_INSERT,0,NULL);
	uint32_t	c=pending;
	ojdbc14BindInt(s,p,&b[0],4);
	b[1].value=(const unsigned char *)"x";
	b[1].valuesize=1;
	ojdbc14BindInt(s,p,&b[2],5);
	oracleprotocolojdbc14bindcall	call;
	bytestring::zero(&call,sizeof(call));
	call.options=OJDBC14_REEXECUTE_DML;
	call.cursorid=c;
	call.al8i4=AL8I4_REEXECUTE_DML;
	call.binds=b;
	call.bindcount=3;
	call.values=true;
	uint32_t	ee=0;
	const char	*what="its re-execute (0x8120) with setInt(4), "
					"setString(\"x\"), setInt(5)";
	if (ojdbc14BindCall(s,what,&call,&e,&pending,&ee)) {
		checkOjdbc14BindDml(s,what,ee,1,SQLTYPE_INSERT,c,&pending);
	}
	const char	*back="select id, v, n from protocoltest10749tbnull "
					"where id > 1 order by id";
	if (s->ok && ojdbc14BindSelect(s,back,3,2,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		checkOjdbc14KeptValue(s,back,k,0,1,NULL,0,"null");
		checkOjdbc14KeptValue(s,back,k,0,2,NULL,0,"null");
		checkOjdbc14KeptValue(s,back,k,1,1,
				(const unsigned char *)"x",1,"x");
		checkOjdbc14KeptValue(s,back,k,1,2,
				(const unsigned char *)"\xc1\x06",2,"5");
	}
	delete k;
	ojdbc14BindDrop(s,table,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

// ---- bindrowid ----

// setROWID: a rowid read back from the table, bound in its 0e form (dty 11,
// mxl 130; MEASURED, [0029] of 10749-dev-ojdbc14-preprowid-realserver-
// oracle122) in "where rowid = :1", which has to find its row, then into a
// ROWID column, whose rowidtochar() has to be its text, then setROWID(null),
// a lone 00, which has to store a null, and the rowid's text as setString
// in "where rowid = :1" (MEASURED, [0037] there)
static void runBindRowid(ojdbc14session *s, uint32_t *open, uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	uint32_t		cs=ojdbc14Charset(s);
	const char		*table="protocoltest10749tbrowid";
	ojdbc14BindTable(s,table,"(id number, v varchar2(10), r rowid)",
							&e,&pending);
	ojdbc14ColumnDml(s,"insert into protocoltest10749tbrowid select "
			"level, 'v'||level, null from dual connect by "
			"level <= 3",3,SQLTYPE_INSERT,false,&e,&pending);

	// the rowid of row 2, and its text
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	const char	*query="select rowid, rowidtochar(rowid) from "
					"protocoltest10749tbrowid where id = 2";
	if (!ojdbc14BindSelect(s,query,2,1,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		delete k;
		return;
	}
	oracleprotocolojdbc14value	rowid=k->values[0][0];
	char				text[19];
	char				own[19];
	oracleprotocolclient::decodeOjdbc14Rowid(&rowid,own);
	size_t	textsize=k->values[0][1].size;
	charstring::copy(text,(const char *)k->data+k->values[0][1].offset,
					(textsize<18)?textsize:18);
	text[(textsize<18)?textsize:18]='\0';
	stringbuffer	l;
	ojdbc14Label(&l,s,query,", the rowid read back is the one "
						"rowidtochar() names");
	report(l.getString(),!charstring::compare(own,text));

	// where rowid = :1, setROWID
	oracleprotocolojdbc14bind	b[2];
	oracleprotocolclient::setOjdbc14RowidBind(&b[0],cs,
					(uint32_t)rowid.rowidobject,
					(uint32_t)rowid.rowidfile,
					(uint32_t)rowid.rowidblock,
					(uint32_t)rowid.rowidrow);
	query="select id, v from protocoltest10749tbrowid where rowid = :1";
	if (ojdbc14BindQuery(s,"setROWID in where rowid = :1",query,b,1,2,1,
						&e,&pending,k)) {
		checkOjdbc14KeptValue(s,"setROWID in where rowid = :1",k,0,1,
				(const unsigned char *)"v2",2,"v2, row 2's");
	}

	// into a rowid column
	unsigned char	one[4];
	size_t		onesize=0;
	putOracleNumber(one,&onesize,1);
	oracleprotocolclient::setOjdbc14Bind(&b[1],6,22,cs,
					ORA_8I_FORM_IMPLICIT,one,onesize);
	ojdbc14BindInsert(s,"setROWID into a rowid column",
			"update protocoltest10749tbrowid set r = :1 "
			"where id = :2",b,2,&e,&pending,1,SQLTYPE_UPDATE,
			0,NULL);
	query="select rowidtochar(r) from protocoltest10749tbrowid "
							"where id = 1";
	if (ojdbc14BindSelect(s,query,1,1,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		checkOjdbc14KeptValue(s,query,k,0,0,
				(const unsigned char *)text,
				charstring::getLength(text),
				"the rowid bound, as text");
	}

	// setROWID(null)
	oracleprotocolclient::setOjdbc14Bind(&b[0],11,130,cs,
					ORA_8I_FORM_IMPLICIT,NULL,0);
	ojdbc14BindInsert(s,"setROWID(null) into the rowid column",
			"update protocoltest10749tbrowid set r = :1 "
			"where id = :2",b,2,&e,&pending,1,SQLTYPE_UPDATE,
			0,NULL);
	query="select nvl2(r,'set','null') from protocoltest10749tbrowid "
							"where id = 1";
	if (ojdbc14BindSelect(s,query,1,1,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		checkOjdbc14KeptValue(s,query,k,0,0,
				(const unsigned char *)"null",4,"null");
	}

	// the rowid's text, setString
	ojdbc14BindString(s,&b[0],(const unsigned char *)text,
				charstring::getLength(text),
				charstring::getLength(text));
	query="select id, v from protocoltest10749tbrowid where rowid = :1";
	if (ojdbc14BindQuery(s,"setString of the rowid's text in where "
				"rowid = :1",query,b,1,2,1,&e,&pending,k)) {
		checkOjdbc14KeptValue(s,"setString of the rowid's text in "
				"where rowid = :1",k,0,1,
				(const unsigned char *)"v2",2,"v2, row 2's");
	}
	delete k;
	ojdbc14BindDrop(s,table,&e,&pending);
	open[(*count)++]=pending;
}

// ---- bindorder ----

// the values ojdbc14 sends after the others outside PL/SQL, a LONG's (dty 8)
// and those of binds whose mxl is over 4000 (MEASURED, [0324] of 10749-dev-
// ojdbc14-colwide-realserver-oracle122 and -oracle102; CROSS-CHECK,
// T4CTTIrxd.marshal):
//	- colwide's insert of 4000 characters, 2000 upper case characters, 2000
//	  bytes and 4000 characters: at 873 the mxls are 16000, 8000, 2000
//	  and 16000, and the values go out as :1, :4, then :2, :3 and :5; at
//	  31 (oracle102's [0324]) they are 4000, 2000, 2000 and 4000 and go
//	  out in order, so exactly 4000 is not postponed (MEASURED)
//	- a string first and a number second, the string's mxl 4001 or more,
//	  which goes out second (CROSS-CHECK, if_icmple 4000; 4001 not
//	  measured), and exactly 4000, which doesn't (MEASURED, as above)
//	- setString of 32767 characters, a dty 8, first, a number and a short
//	  string behind it, the LONG last on the wire (dty 8 MEASURED, [0025] of
//	  10749-dev-ojdbc14-prepstrlimit-realserver-oracle122, where it is the
//	  last bind anyway)
//	- a dty 8 of mxl 100 first, a number second, the LONG last on the wire
//	  (CROSS-CHECK, T4CTTIrxd postpones dty 8 and 24 whatever the mxl; the
//	  driver sends dty 8 with mxl 0x7fffffff only, which the size rule
//	  postpones too)
// every value has to land in its own column
static void runBindOrder(ojdbc14session *s, uint32_t *open, uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	uint32_t		r=ojdbc14MaxCharSize(s);
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;

	const char	*wide="protocoltest10749tbwide";
	ojdbc14BindTable(s,wide,"(id number, v varchar2(4000), c char(2000), "
				"r raw(2000), v2 varchar2(4000 char))",
				&e,&pending);
	const unsigned char	*lower=ojdbc14BindPattern(p,PATTERN_LOWER,4000);
	const unsigned char	*upper=ojdbc14BindPattern(p,PATTERN_UPPER,2000);
	const unsigned char	*bytes=
				ojdbc14BindPattern(p,PATTERN_BYTES7,2000);
	oracleprotocolojdbc14bind	b[5];
	ojdbc14BindInt(s,p,&b[0],2);
	ojdbc14BindString(s,&b[1],lower,4000,4000);
	ojdbc14BindString(s,&b[2],upper,2000,2000);
	oracleprotocolclient::setOjdbc14Bind(&b[3],23,2000,ojdbc14Charset(s),
					ORA_8I_FORM_IMPLICIT,bytes,2000);
	ojdbc14BindString(s,&b[4],lower,4000,4000);
	ojdbc14BindInsert(s,(r>1)?
			"colwide's insert, the values of :2, :3 and :5 (mxl "
			"16000, 8000 and 16000) behind those of :1 and :4":
			"colwide's insert, mxl 4000, 2000, 2000 and 4000, "
			"every value in order",
			"insert into protocoltest10749tbwide values "
			"(:1,:2,:3,:4,:5)",b,5,&e,&pending,1,SQLTYPE_INSERT,
			0,NULL);
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	const char	*query="select id, v, c, r, v2 from "
						"protocoltest10749tbwide";
	if (s->ok && ojdbc14BindSelect(s,query,5,1,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		unsigned char	two[4];
		size_t		twosize=0;
		putOracleNumber(two,&twosize,2);
		checkOjdbc14KeptValue(s,query,k,0,0,two,twosize,"2, :1's");
		checkOjdbc14KeptValue(s,query,k,0,1,lower,4000,
					"the 4000 characters of :2");
		checkOjdbc14KeptValue(s,query,k,0,2,upper,2000,
					"the 2000 upper case characters of :3");
		checkOjdbc14KeptValue(s,query,k,0,3,bytes,2000,
					"the 2000 bytes of :4");
		checkOjdbc14KeptValue(s,query,k,0,4,lower,4000,
					"the 4000 characters of :5");
	}
	ojdbc14BindDrop(s,wide,&e,&pending);

	const char	*lng="protocoltest10749tblong";
	ojdbc14BindTable(s,lng,"(id number, v varchar2(10), l long)",
							&e,&pending);
	size_t			over=(r>1)?1001:4001;
	size_t			at=(r>1)?1000:4000;
	const unsigned char	*long32767=ojdbc14BindPattern(p,PATTERN_LOWER,
									32767);
	char	what[256];
	ojdbc14BindString(s,&b[0],lower,over,over);
	ojdbc14BindInt(s,p,&b[1],1);
	charstring::printf(what,sizeof(what),
			"a string of mxl %d first, its value sent second",
			(int)(over*r));
	ojdbc14BindInsert(s,what,"insert into protocoltest10749tblong (l, id) "
				"values (:1,:2)",b,2,&e,&pending,1,
				SQLTYPE_INSERT,0,NULL);
	ojdbc14BindString(s,&b[0],lower,at,at);
	ojdbc14BindInt(s,p,&b[1],2);
	charstring::printf(what,sizeof(what),
			"a string of mxl %d first, its value sent first",
			(int)(at*r));
	ojdbc14BindInsert(s,what,"insert into protocoltest10749tblong (l, id) "
				"values (:1,:2)",b,2,&e,&pending,1,
				SQLTYPE_INSERT,0,NULL);
	oracleprotocolclient::setOjdbc14Bind(&b[0],8,0x7fffffff,
				ojdbc14Charset(s),ORA_8I_FORM_IMPLICIT,
				long32767,32767);
	ojdbc14BindInt(s,p,&b[1],3);
	ojdbc14BindString(s,&b[2],(const unsigned char *)"abc",3,3);
	ojdbc14BindInsert(s,"setString of 32767 characters first, a LONG "
				"(dty 8), its value sent last",
			"insert into protocoltest10749tblong (l, id, v) values "
			"(:1,:2,:3)",b,3,&e,&pending,1,SQLTYPE_INSERT,0,NULL);

	// a LONG of mxl 100, last on the wire too
	oracleprotocolclient::setOjdbc14Bind(&b[0],8,100,ojdbc14Charset(s),
					ORA_8I_FORM_IMPLICIT,
					(const unsigned char *)"xyz",3);
	ojdbc14BindInt(s,p,&b[1],4);
	ojdbc14BindInsert(s,"a LONG (dty 8) of mxl 100 first, its value sent "
				"last",
			"insert into protocoltest10749tblong (l, id) values "
			"(:1,:2)",b,2,&e,&pending,1,SQLTYPE_INSERT,0,NULL);
	query="select id, v, l from protocoltest10749tblong order by id";
	if (s->ok && ojdbc14BindSelect(s,query,3,4,1,&e,&pending,k)) {
		for (uint32_t i=0; i<4; i++) {
			unsigned char	id[4];
			size_t		idsize=0;
			putOracleNumber(id,&idsize,i+1);
			checkOjdbc14KeptValue(s,query,k,i,0,id,idsize,
							"the id bound");
		}
		checkOjdbc14KeptValue(s,query,k,0,2,lower,over,
					"the string bound first");
		checkOjdbc14KeptValue(s,query,k,1,2,lower,at,
					"the string bound first");
		checkOjdbc14KeptValue(s,query,k,2,1,
				(const unsigned char *)"abc",3,"abc, :3's");
		checkOjdbc14KeptValue(s,query,k,2,2,long32767,32767,
					"the 32767 characters of the LONG");
		checkOjdbc14KeptValue(s,query,k,3,2,
					(const unsigned char *)"xyz",3,
					"xyz, the LONG of mxl 100");
	}
	delete k;
	ojdbc14BindDrop(s,lng,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

// ---- bindplsql ----

// a PL/SQL block's binds go out in order whatever their mxl: ojdbc14 sends a
// block with options 0x0529, which lack 0x8000 (MEASURED, [0029] of 10749-
// dev-ojdbc14-preptz-realserver-oracle122), a max long of 32760, and every
// setString bind in it with mxl 32512, and postpones only dty 8 and 24 there
// (CROSS-CHECK, T4C8Oall.setOptions and T4CTTIrxd.marshal; no real server
// was sent a block with a string bind ahead of another).  so a string ahead
// of a number has to land in its own column
static void runBindPlsql(ojdbc14session *s, uint32_t *open, uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	const char		*table="protocoltest10749tbplsql";
	ojdbc14BindTable(s,table,"(id number, v varchar2(100))",&e,&pending);

	oracleprotocolojdbc14bind	b[2];
	ojdbc14BindString(s,&b[0],(const unsigned char *)"abc",3,3);
	b[0].maxlength=32512;
	ojdbc14BindInt(s,p,&b[1],7);
	oracleprotocolojdbc14bindcall	c;
	bytestring::zero(&c,sizeof(c));
	c.options=OJDBC14_PLSQL;
	c.query="begin insert into protocoltest10749tbplsql (v, id) "
						"values (:1, :2); end;";
	c.al8i4=AL8I4_DML;
	c.maxlong=ORA_8I_MAX_LONG_PLSQL;
	c.binds=b;
	c.bindcount=2;
	c.descriptors=true;
	c.values=true;
	uint32_t	ee=0;
	const char	*what="a PL/SQL block, a string of mxl 32512 bound "
						"ahead of a number";
	if (ojdbc14BindCall(s,what,&c,&e,&pending,&ee)) {
		checkOjdbc14NoMarkers(s,what);
		stringbuffer	l;
		ojdbc14Label(&l,s,what," is answered with no error");
		size_t	n=s->answer->objectcount;
		bool	ok=(n && s->answer->objects[n-1].code==ORA_TTC_ERROR &&
						!s->answer->retcode);
		report(l.getString(),ok);
		if (!ok) {
			stdoutput.printf("    error %d: %.*s\n",
					(int)s->answer->retcode,
					(int)s->answer->messagesize,
					s->answer->message);
		}
		pending=(uint32_t)s->answer->cursorid;
	}
	// but a LONG still goes last in a block (CROSS-CHECK, T4CTTIrxd, as
	// in bindorder; the driver sends dty 8 for 32767 characters or more,
	// more than a block takes, so no real server was sent this either)
	oracleprotocolclient::setOjdbc14Bind(&b[0],8,0x7fffffff,
					ojdbc14Charset(s),ORA_8I_FORM_IMPLICIT,
					(const unsigned char *)"xyz",3);
	ojdbc14BindInt(s,p,&b[1],8);
	what="a PL/SQL block, a LONG (dty 8) bound ahead of a number, its "
							"value sent last";
	if (s->ok && ojdbc14BindCall(s,what,&c,&e,&pending,&ee)) {
		checkOjdbc14NoMarkers(s,what);
		stringbuffer	l;
		ojdbc14Label(&l,s,what," is answered with no error");
		size_t	n=s->answer->objectcount;
		bool	ok=(n && s->answer->objects[n-1].code==ORA_TTC_ERROR &&
						!s->answer->retcode);
		report(l.getString(),ok);
		if (!ok) {
			stdoutput.printf("    error %d: %.*s\n",
					(int)s->answer->retcode,
					(int)s->answer->messagesize,
					s->answer->message);
		}
		pending=(uint32_t)s->answer->cursorid;
	}

	ojdbc14keptrows	*k=new ojdbc14keptrows;
	const char	*query="select id, v from protocoltest10749tbplsql "
								"order by id";
	if (s->ok && ojdbc14BindSelect(s,query,2,2,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		checkOjdbc14KeptValue(s,query,k,0,0,
				(const unsigned char *)"\xc1\x08",2,"7, :2's");
		checkOjdbc14KeptValue(s,query,k,0,1,
				(const unsigned char *)"abc",3,"abc, :1's");
		checkOjdbc14KeptValue(s,query,k,1,0,
				(const unsigned char *)"\xc1\x09",2,
				"8, the number bound behind the LONG");
		checkOjdbc14KeptValue(s,query,k,1,1,
				(const unsigned char *)"xyz",3,"xyz, the LONG's");
	}
	delete k;
	ojdbc14BindDrop(s,table,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

// ---- bindreexec ----

// one bind of a re-execute session: its dty, whether national, and its five
// values, as preprexec binds them - the first, a second (longer where it
// can be), a null, a third and the second again or a longer one (MEASURED,
// 10749-dev-ojdbc14-preprexec-realserver-*; the nchar values are ascii here
// so that every back end carries them)
struct ojdbc14reexeckind {
	const char	*name;
	const char	*column;
	unsigned char	type;
	bool		national;
	bool		direct;
	unsigned int	dumptype;
	const char	*values[5];
	size_t		patterns[5];
};

static const ojdbc14reexeckind	REEXEC_KINDS[]={
	{"string","s",1,false,true,1,
		{"61",NULL,NULL,"62",NULL},{0,70,0,0,4000}},
	{"int","n",6,false,false,2,
		{"c1 02","3a 51 66",NULL,"c1 04","c5 15"},{0,0,0,0,0}},
	{"date","d",12,false,false,12,
		{"78 7c 03 05 01 01 01","77 aa 01 01 01 01 01",NULL,
		"78 64 02 1d 01 01 01","c7 c7 0c 1f 01 01 01"},{0,0,0,0,0}},
	{"timestamp","ts",180,false,false,180,
		{"78 7c 03 05 0e 0f 10 07 5b ca 00","77 aa 01 01 01 01 01",
		NULL,"78 64 02 1d 18 3c 3c 3b 9a c6 18",
		"78 7c 03 05 0e 0f 10"},{0,0,0,0,0}},
	{"bytes","r",23,false,true,23,
		{NULL,NULL,NULL,NULL,NULL},{3,100,0,64,2000}},
	{"binfloat","bf",100,false,false,100,
		{"bf c0 00 00","ff c0 00 00",NULL,"80 00 00 00",
		"ff 7f ff ff"},{0,0,0,0,0}},
	{"nchar","nc",1,true,false,1,
		{"00 61 00 62 00 63","00 64 00 65 00 66 00 67 00 68",NULL,
		"00 69 00 6a","00 78"},{0,0,0,0,0}}
};

// one value of a re-execute kind as its bind, NULL bytes for a null
static void ojdbc14ReexecBind(ojdbc14session *s, ojdbc14bindbytes *p,
					const ojdbc14reexeckind *kind,
					uint32_t value,
					oracleprotocolojdbc14bind *b,
					const unsigned char **bytes,
					size_t *size) {
	*size=0;
	*bytes=NULL;
	if (kind->patterns[value]) {
		*bytes=ojdbc14BindPattern(p,(kind->type==23)?PATTERN_BYTES:
							PATTERN_LOWER,
						kind->patterns[value]);
		*size=kind->patterns[value];
	} else {
		*bytes=ojdbc14BindHex(p,kind->values[value],size);
	}
	if (kind->national) {
		ojdbc14BindNchar(b,*bytes,*size);
	} else if (kind->type==1) {
		ojdbc14BindString(s,b,*bytes,*size,*size);
	} else {
		oracleprotocolclient::setOjdbc14Bind(b,kind->type,
					ojdbc14BindMaxLength(kind->type),
					ojdbc14Charset(s),
					ORA_8I_FORM_IMPLICIT,*bytes,*size);
	}
}

// the next execute of a prepared dml on "*cursorid", the way ojdbc14 picks
// it (MEASURED, section 5 of step 2b of #10749): the first is a 0x8129 with
// the text and every descriptor; a later one is a 0x8120 with the values
// alone if no value needs a bigger mxl than the descriptors last sent, and
// otherwise a 0x8128 with every descriptor again, each mxl the bigger of
// the last one sent and the value's.  "sent" are the descriptors last sent,
// updated here.  every one touches a row
static void ojdbc14BindExecute(ojdbc14session *s, const char *what,
					const char *query,
					oracleprotocolojdbc14bind *binds,
					oracleprotocolojdbc14bind *sent,
					uint32_t bindcount, bool first,
					uint32_t *cursorid,
					uint32_t *endtoend, uint32_t *pending) {
	bool	grown=false;
	for (uint32_t i=0; !first && i<bindcount; i++) {
		if (binds[i].maxlength>sent[i].maxlength) {
			grown=true;
		} else {
			binds[i].maxlength=sent[i].maxlength;
		}
	}
	for (uint32_t i=0; i<bindcount; i++) {
		sent[i]=binds[i];
	}
	oracleprotocolojdbc14bindcall	c;
	bytestring::zero(&c,sizeof(c));
	c.options=(first)?OJDBC14_PREPARED_COMMIT:
			(grown)?OJDBC14_REEXECUTE_GROWN:OJDBC14_REEXECUTE_DML;
	c.cursorid=(first)?0:*cursorid;
	c.query=(first)?query:NULL;
	c.al8i4=(first)?AL8I4_DML:AL8I4_REEXECUTE_DML;
	c.binds=binds;
	c.bindcount=bindcount;
	c.descriptors=(first || grown);
	c.values=true;
	char	label[256];
	charstring::printf(label,sizeof(label),"%s, a 0x%04x",what,
							(int)c.options);
	uint32_t	e=0;
	if (!ojdbc14BindCall(s,label,&c,endtoend,pending,&e)) {
		return;
	}
	checkOjdbc14BindDml(s,label,e,1,SQLTYPE_INSERT,
				(first)?0:*cursorid,pending);
	if (first) {
		*cursorid=*pending;
	}
}

// a query re-execute's answer, with fetch (0x8068) or re-parsed (0x8069):
// the describe if it was re-parsed, the row header, the one row, the return
// parameters naming the same cursor and ORA-01403, the rows the call asked
// for not coming back (the shape runColNull() checks).  "expected" is the
// row's value, NULL for a null, and "name" what it is, NULL where the value
// isn't checked
static void checkOjdbc14ReexecRow(ojdbc14session *s, const char *what,
					uint32_t e, uint32_t cursorid,
					bool reparsed,
					const unsigned char *expected,
					size_t size, const char *name) {
	checkOjdbc14NoMarkers(s,what);
	checkOjdbc14Objects(s,what,(reparsed)?"10 06 07 08 04":"06 07 08 04");
	uint32_t	again=checkOjdbc14ReturnParameters(s,what);
	stringbuffer	l;
	ojdbc14Label(&l,s,what," names the same cursor");
	report(l.getString(),again==cursorid);
	checkOjdbc14Summary(s,what,e,1,1403,cursorid,3);
	if (!name) {
		return;
	}
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	bytestring::zero(k,sizeof(*k));
	keepOjdbc14Rows(s,k,1);
	checkOjdbc14KeptValue(s,what,k,0,0,expected,size,name);
	delete k;
}

// preprexec: each kind's five executes on one prepared insert, then the
// rows read back.  then a string bind postponed by an mxl of 16000 that
// isn't the last bind, and its re-execute with a short value, which ojdbc14
// still sends last, since it postpones by the mxl of the descriptors last
// sent (CROSS-CHECK, T4C8Oall's oacdefBindsSent; not measured).  then
// "select :1 a from dual" re-executed with fetch (0x8068) with 70
// characters, a null and "b", and re-parsed (0x8069) with setInt(5),
// setString("y"), setNull(NUMERIC) and 100 characters (MEASURED, [0181] to
// [0203] there)
static void runBindReexec(ojdbc14session *s, uint32_t *open,
						uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	const char		*table="protocoltest10749tbrexec";
	ojdbc14BindTable(s,table,"(id number, kind varchar2(20), "
			"s varchar2(4000), n number, d date, ts timestamp(6), "
			"r raw(2000), bf binary_float, nc nvarchar2(100))",
			&e,&pending);

	ojdbc14keptrows	*k=new ojdbc14keptrows;
	for (size_t ki=0; s->ok && ki<sizeof(REEXEC_KINDS)/
					sizeof(REEXEC_KINDS[0]); ki++) {
		const ojdbc14reexeckind	*kind=&REEXEC_KINDS[ki];
		stringbuffer	query;
		query.append("insert into protocoltest10749tbrexec "
							"(id, kind, ");
		query.append(kind->column)->append(") values (:1,:2,:3)");
		oracleprotocolojdbc14bind	b[3];
		oracleprotocolojdbc14bind	sent[3];
		const unsigned char		*bytes[5];
		size_t				sizes[5];
		uint32_t			cursorid=0;
		for (uint32_t v=0; s->ok && v<5; v++) {
			ojdbc14BindInt(s,p,&b[0],(uint32_t)ki*10+v);
			ojdbc14BindString(s,&b[1],
				(const unsigned char *)kind->name,
				charstring::getLength(kind->name),
				charstring::getLength(kind->name));
			ojdbc14ReexecBind(s,p,kind,v,&b[2],&bytes[v],&sizes[v]);
			char	what[128];
			charstring::printf(what,sizeof(what),
				"%s execute %d",kind->name,(int)v+1);
			ojdbc14BindExecute(s,what,query.getString(),b,sent,3,
					v==0,&cursorid,&e,&pending);
		}
		stringbuffer	select;
		select.append("select id, ");
		if (kind->direct) {
			select.append(kind->column);
		} else {
			select.append("nvl2(")->append(kind->column);
			select.append(",dump(")->append(kind->column);
			select.append((kind->national)?",1016":",16");
			select.append("),'null')");
		}
		select.append(" from protocoltest10749tbrexec where kind = '");
		select.append(kind->name)->append("' order by id");
		if (!s->ok || !ojdbc14BindSelect(s,select.getString(),2,5,
					OJDBC14_FETCH_ROWS,&e,&pending,k)) {
			break;
		}
		for (uint32_t v=0; v<5; v++) {
			if (kind->direct) {
				checkOjdbc14KeptValue(s,select.getString(),k,
					v,1,bytes[v],sizes[v],
					(bytes[v])?"the bytes bound":"null");
				continue;
			}
			stringbuffer	dump;
			ojdbc14DumpText(&dump,kind->dumptype,kind->national,
							bytes[v],sizes[v]);
			stringbuffer	name;
			name.append("\"")->append(dump.getString());
			name.append("\", the bytes bound, as a real server "
							"stores them");
			checkOjdbc14KeptValue(s,select.getString(),k,v,1,
				(const unsigned char *)dump.getString(),
				dump.getSize(),name.getString());
		}
	}

	// a postponed string ahead of another bind, re-executed short
	const char	*query="insert into protocoltest10749tbrexec (id, s, "
					"kind) values (:1,:2,:3)";
	const unsigned char	*lower=ojdbc14BindPattern(p,PATTERN_LOWER,4000);
	static const char	*postponed[]={"a",NULL,"b"};
	oracleprotocolojdbc14bind	b[3];
	oracleprotocolojdbc14bind	sent[3];
	uint32_t			cursorid=0;
	for (uint32_t v=0; s->ok && v<3; v++) {
		ojdbc14BindInt(s,p,&b[0],100+v);
		if (postponed[v]) {
			ojdbc14BindString(s,&b[1],
				(const unsigned char *)postponed[v],1,1);
		} else {
			ojdbc14BindString(s,&b[1],lower,4000,4000);
		}
		ojdbc14BindString(s,&b[2],(const unsigned char *)"postponed",
									9,9);
		char	what[128];
		charstring::printf(what,sizeof(what),
				"a string ahead of the kind, execute %d",
				(int)v+1);
		ojdbc14BindExecute(s,what,query,b,sent,3,v==0,&cursorid,
							&e,&pending);
	}
	const char	*select="select id, s, kind from "
				"protocoltest10749tbrexec where id >= 100 "
				"order by id";
	if (s->ok && ojdbc14BindSelect(s,select,3,3,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		checkOjdbc14KeptValue(s,select,k,0,1,
					(const unsigned char *)"a",1,"a");
		checkOjdbc14KeptValue(s,select,k,1,1,lower,4000,
					"the 4000 characters");
		checkOjdbc14KeptValue(s,select,k,2,1,
				(const unsigned char *)"b",1,"b, sent last");
		for (uint32_t v=0; v<3; v++) {
			checkOjdbc14KeptValue(s,select,k,v,2,
				(const unsigned char *)"postponed",9,
				"postponed, the kind");
		}
	}
	ojdbc14BindDrop(s,table,&e,&pending);

	// a query's re-executes with fetch
	const char	*dual="select :1 a from dual";
	ojdbc14BindString(s,&b[0],(const unsigned char *)"a",1,1);
	if (s->ok && ojdbc14BindQuery(s,dual,dual,b,1,1,1,&e,&pending,k)) {
		checkOjdbc14KeptValue(s,dual,k,0,0,
					(const unsigned char *)"a",1,"a");
	}
	uint32_t	c=pending;
	const unsigned char	*str70=ojdbc14BindPattern(p,PATTERN_LOWER,70);
	struct {
		const char		*what;
		const unsigned char	*value;
		size_t			size;
		uint32_t		mxl;
		const char		*name;
	} fetches[]={
		{"its re-execute with fetch (0x8068) of 70 characters",
			str70,70,70,NULL},
		{"its re-execute with fetch (0x8068) of a null",
			NULL,0,70,"null"},
		{"its re-execute with fetch (0x8068) of \"b\"",
			(const unsigned char *)"b",1,1,"b"}
	};
	for (size_t i=0; s->ok && i<sizeof(fetches)/sizeof(fetches[0]); i++) {
		ojdbc14BindString(s,&b[0],fetches[i].value,fetches[i].size,
							fetches[i].mxl);
		oracleprotocolojdbc14bindcall	call;
		bytestring::zero(&call,sizeof(call));
		call.options=OJDBC14_REEXECUTE_QUERY;
		call.cursorid=c;
		call.al8i4=AL8I4_REEXECUTE_QUERY;
		call.binds=b;
		call.bindcount=1;
		call.descriptors=true;
		call.values=true;
		uint32_t	ee=0;
		if (ojdbc14BindCall(s,fetches[i].what,&call,&e,&pending,&ee)) {
			checkOjdbc14ReexecRow(s,fetches[i].what,ee,c,false,
					fetches[i].value,fetches[i].size,
					fetches[i].name);
		}
	}

	// a query re-parsed after a bind changed type.  the module binds a
	// NUMBER as text, so setInt(5) comes back as the text 5, where a real
	// server describes NUMBER and sends c1 06 (by design, follow-up F1)
	ojdbc14BindString(s,&b[0],(const unsigned char *)"x",1,1);
	if (s->ok && ojdbc14BindQuery(s,dual,dual,b,1,1,1,&e,&pending,k)) {
		checkOjdbc14KeptValue(s,dual,k,0,0,
					(const unsigned char *)"x",1,"x");
	}
	c=pending;
	const unsigned char	*str100=ojdbc14BindPattern(p,PATTERN_LOWER,100);
	uint32_t	cs=ojdbc14Charset(s);
	for (uint32_t i=0; s->ok && i<4; i++) {
		const char		*what=NULL;
		const unsigned char	*value=NULL;
		size_t			size=0;
		const char		*name=NULL;
		switch (i) {
			case 0:
				ojdbc14BindInt(s,p,&b[0],5);
				what="its re-parse (0x8069) with setInt(5)";
				value=(const unsigned char *)"5";
				size=1;
				name="the text 5, where a real server sends "
					"the NUMBER c1 06 (the module binds a "
					"NUMBER as text)";
				break;
			case 1:
				ojdbc14BindString(s,&b[0],
					(const unsigned char *)"y",1,1);
				what="its re-parse (0x8069) with setString(y)";
				value=(const unsigned char *)"y";
				size=1;
				name="y";
				break;
			case 2:
				oracleprotocolclient::setOjdbc14Bind(&b[0],6,
					22,cs,ORA_8I_FORM_IMPLICIT,NULL,0);
				what="its re-parse (0x8069) with "
							"setNull(NUMERIC)";
				name="null";
				break;
			case 3:
				ojdbc14BindString(s,&b[0],str100,100,100);
				what="its re-parse (0x8069) with 100 "
								"characters";
				value=str100;
				size=100;
				name="the 100 characters";
				break;
		}
		oracleprotocolojdbc14bindcall	call;
		bytestring::zero(&call,sizeof(call));
		call.options=OJDBC14_REPARSE_QUERY;
		call.cursorid=c;
		call.query=dual;
		call.al8i4=AL8I4_REPARSE_QUERY;
		call.binds=b;
		call.bindcount=1;
		call.descriptors=true;
		call.values=true;
		uint32_t	ee=0;
		if (ojdbc14BindCall(s,what,&call,&e,&pending,&ee)) {
			checkOjdbc14ReexecRow(s,what,ee,c,true,value,size,name);
		}
	}
	delete k;
	delete p;
	open[(*count)++]=pending;
}

// ---- binderrors ----

// binds the module can't take, each answered with the module's own error
// behind the markers, the session going on behind it (design B.3 and B.4
// of #10749; the module's own, so no real server's): a type ojdbc14 never
// sends, dty 109, ORA-20008; NUMBER +Inf and -Inf, which setFloat() and
// setDouble() send as ff 65 and a lone 00 (MEASURED, [0043] of 10749-dev-
// ojdbc14-prepbinfloat-realserver-oracle122) and a real server stores, but
// which the module can't bind as text, ORA-20009 (decision E3); and values
// of the wrong size for their type - an INTERVAL YEAR TO MONTH of 4 bytes,
// a DATE of 6, a BINARY_FLOAT of 3, a national string of an odd 3 bytes -
// and intervals whose fields disagree in sign, +1 year and -2 months, +1
// day and -1 hour, ORA-20009; and a ROWID whose value is a clr that isn't
// 18 characters, ORA-20008 (the wrong sizes and the mixed signs are
// INFERRED, no driver sends them)
static void runBindErrors(ojdbc14session *s, uint32_t *open,
						uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	uint32_t		cs=ojdbc14Charset(s);
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	const char		*table="protocoltest10749tberr";
	ojdbc14BindTable(s,table,"(id number, n number, "
			"ym interval year to month, ds interval day to second, "
			"d date, bf binary_float, nv nvarchar2(10), r rowid)",
			&e,&pending);

	struct {
		const char		*what;
		const char		*column;
		unsigned char		type;
		uint32_t		mxl;
		bool			national;
		const char		*hex;
		uint32_t		oranum;
		const ojdbc14message	*m;
	} failings[]={
		{"a dty 109 bind","n",109,4,false,"01 02 03 04",
			20008,&MSG_20008},
		{"NUMBER +Inf, ff 65","n",6,22,false,"ff 65",
			20009,&MSG_20009},
		{"NUMBER -Inf, a lone 00","n",6,22,false,"00",
			20009,&MSG_20009},
		{"an INTERVAL YEAR TO MONTH of 4 bytes","ym",182,5,false,
			"80 00 00 0c",20009,&MSG_20009},
		{"an INTERVAL YEAR TO MONTH of +1 year and -2 months","ym",
			182,5,false,"80 00 00 01 3a",20009,&MSG_20009},
		{"an INTERVAL DAY TO SECOND of +1 day and -1 hour","ds",
			183,11,false,"80 00 00 01 3b 3c 3c 80 00 00 00",
			20009,&MSG_20009},
		{"a DATE of 6 bytes","d",12,7,false,"78 7c 03 05 01 01",
			20009,&MSG_20009},
		{"a BINARY_FLOAT of 3 bytes","bf",100,4,false,"bf c0 00",
			20009,&MSG_20009},
		{"a national string of 3 bytes","nv",1,4,true,"00 61 00",
			20009,&MSG_20009},
		{"a ROWID whose value is the clr \"abcde\"","r",11,130,false,
			"61 62 63 64 65",20008,&MSG_20008}
	};
	for (size_t i=0; s->ok && i<sizeof(failings)/sizeof(failings[0]);
									i++) {
		oracleprotocolojdbc14bind	b[2];
		ojdbc14BindInt(s,p,&b[0],(uint32_t)i+1);
		size_t			size=0;
		const unsigned char	*value=ojdbc14BindHex(p,failings[i].hex,
									&size);
		oracleprotocolclient::setOjdbc14Bind(&b[1],failings[i].type,
				failings[i].mxl,
				(failings[i].national)?
					ORA_8I_CHARSET_AL16UTF16:cs,
				(failings[i].national)?
					ORA_8I_FORM_NCHAR:ORA_8I_FORM_IMPLICIT,
				value,size);
		stringbuffer	query;
		query.append("insert into protocoltest10749tberr (id, ");
		query.append(failings[i].column)->append(") values (:1,:2)");
		ojdbc14BindInsert(s,failings[i].what,query.getString(),b,2,
				&e,&pending,0,SQLTYPE_INSERT,
				failings[i].oranum,failings[i].m);
	}

	// and in a select
	oracleprotocolojdbc14bind	b[2];
	size_t			size=0;
	const unsigned char	*value=ojdbc14BindHex(p,"01 02 03 04",&size);
	oracleprotocolclient::setOjdbc14Bind(&b[0],109,4,cs,
					ORA_8I_FORM_IMPLICIT,value,size);
	oracleprotocolojdbc14bindcall	c;
	bytestring::zero(&c,sizeof(c));
	c.options=OJDBC14_PREPARED;
	c.query="select :1 a from dual";
	c.al8i4=AL8I4_QUERY;
	c.binds=b;
	c.bindcount=1;
	c.descriptors=true;
	c.values=true;
	uint32_t	ee=0;
	const char	*what="select :1 a from dual of a dty 109 bind";
	if (ojdbc14BindCall(s,what,&c,&e,&pending,&ee)) {
		checkOjdbc14BindError(s,what,ee,20008,SQLTYPE_SELECT,0,
						&MSG_20008,0,&pending);
	}

	// the session goes on: a good insert, and only its row is there
	ojdbc14BindInt(s,p,&b[0],100);
	ojdbc14BindInt(s,p,&b[1],1);
	ojdbc14BindInsert(s,"an insert that works","insert into "
			"protocoltest10749tberr (id, n) values (:1,:2)",b,2,
			&e,&pending,1,SQLTYPE_INSERT,0,NULL);
	ojdbc14keptrows	*k=new ojdbc14keptrows;
	const char	*query="select id from protocoltest10749tberr";
	if (s->ok && ojdbc14BindSelect(s,query,1,1,OJDBC14_FETCH_ROWS,
						&e,&pending,k)) {
		checkOjdbc14KeptValue(s,query,k,0,0,
				(const unsigned char *)"\xc2\x02",2,
				"100, the one row the errors left");
	}
	delete k;
	ojdbc14BindDrop(s,table,&e,&pending);
	delete p;
	open[(*count)++]=pending;
}

// ---- bindselect ----

// "select :1 ... from dual" of binds whose describe and value a real server
// echoes and the module's back end does too: a TIMESTAMP, described type
// 180, and a TIMESTAMP WITH TIME ZONE at +05:30, type 181, each its bytes
// back (MEASURED, section 4 of step 2b of #10749 and the select ? dumps of
// 10749-dev-ojdbc14-preptimestamp- and -preptz-realserver-*; the values
// are microsecond exact, which the module keeps), then prepchunk's select
// of 128 characters and 128 bytes, the exact multiple of the 64 byte chunk
// (MEASURED, [0016] of 10749-dev-ojdbc14-prepchunk-realserver-oracle122):
// the characters come back, and the bytes as their hex, upper case, where a
// real server sends the bytes (the module binds a RAW as its hex, design
// B.3 and decision E2)
static void runBindSelect(ojdbc14session *s, uint32_t *open,
						uint32_t *count) {

	uint32_t		e=3;
	uint32_t		pending=0;
	uint32_t		cs=ojdbc14Charset(s);
	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	p->size=0;
	ojdbc14keptrows		*k=new ojdbc14keptrows;
	const char		*dual="select :1 a from dual";

	struct {
		const char	*what;
		unsigned char	type;
		const char	*hex;
	} echoes[]={
		{"select :1 a from dual of a TIMESTAMP",180,
			"78 7c 03 05 01 01 01 1d cd 65 00"},
		{"select :1 a from dual of a TIMESTAMP WITH TIME ZONE at "
			"+05:30",181,
			"78 7c 03 05 13 0f 10 07 5b ca 00 19 5a"}
	};
	for (size_t i=0; s->ok && i<sizeof(echoes)/sizeof(echoes[0]); i++) {
		oracleprotocolojdbc14bind	b;
		size_t				size=0;
		const unsigned char		*value=ojdbc14BindHex(p,
							echoes[i].hex,&size);
		oracleprotocolclient::setOjdbc14Bind(&b,echoes[i].type,
					ojdbc14BindMaxLength(echoes[i].type),
					cs,ORA_8I_FORM_IMPLICIT,value,size);
		if (!ojdbc14BindQuery(s,echoes[i].what,dual,&b,1,1,1,
							&e,&pending,k)) {
			break;
		}
		char	check[64];
		charstring::printf(check,sizeof(check),
					" describes type %d",
					(int)echoes[i].type);
		stringbuffer	l;
		ojdbc14Label(&l,s,echoes[i].what,check);
		report(l.getString(),k->types[0]==echoes[i].type);
		if (k->types[0]!=echoes[i].type) {
			stdoutput.printf("    got type %d\n",(int)k->types[0]);
		}
		checkOjdbc14KeptValue(s,echoes[i].what,k,0,0,value,size,
					"the bytes bound, as a real server "
					"echoes them");
	}

	if (s->ok) {
		oracleprotocolojdbc14bind	b[2];
		const unsigned char	*str128=ojdbc14BindPattern(p,
							PATTERN_LOWER,128);
		const unsigned char	*bytes128=ojdbc14BindPattern(p,
							PATTERN_BYTES,128);
		ojdbc14BindString(s,&b[0],str128,128,128);
		oracleprotocolclient::setOjdbc14Bind(&b[1],23,2000,cs,
					ORA_8I_FORM_IMPLICIT,bytes128,128);
		const char	*query="select :1 v, :2 r from dual";
		if (ojdbc14BindQuery(s,query,query,b,2,2,1,&e,&pending,k)) {
			char	hex[257];
			for (size_t i=0; i<128; i++) {
				charstring::printf(hex+2*i,3,"%02X",
								bytes128[i]);
			}
			checkOjdbc14KeptValue(s,query,k,0,0,str128,128,
						"the 128 characters bound");
			checkOjdbc14KeptValue(s,query,k,0,1,
					(const unsigned char *)hex,256,
					"the 128 bytes' hex, upper case, "
					"where a real server sends the bytes "
					"(the module binds a RAW as its hex)");
		}
	}
	delete k;
	delete p;
	open[(*count)++]=pending;
}

// ---- the bind builders against real clients' calls ----

// ojdbc14's 0x5e calls with binds as it sent them to real servers, behind
// the data flags - see checkOjdbc14BindBuilders() for the packet each is
// from.  a call too big to keep whole keeps its first bytes here, and its
// last ones in the _TAIL beside it
static const char	*BIND_CALL_DATE=
	"11 69 00 01 01 01 01 05 03 5e 00 02 81 29 00 01 01 40 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 03 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 64 61 74 65 20 28 69 64 2c 20 64 2c 20 74 "
	"73 29 20 76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 2c 3a 33 29 01 "
	"01 01 01 00 00 00 00 00 00 00 00 00 00 00 06 03 00 00 01 16 00 "
	"00 00 00 02 03 69 01 00 0c 03 00 00 01 07 00 00 00 00 02 03 69 "
	"01 00 0c 03 00 00 01 07 00 00 00 00 02 03 69 01 00 07 02 c1 02 "
	"07 78 7c 03 05 01 01 01 07 78 7c 03 05 01 01 01";
static const char	*BIND_CALL_TIMESTAMP=
	"11 69 00 01 01 01 01 05 03 5e 00 02 81 29 00 01 01 46 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 04 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 74 73 20 28 69 64 2c 20 64 2c 20 74 73 2c "
	"20 74 73 39 29 20 76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 2c 3a "
	"33 2c 3a 34 29 01 01 01 01 00 00 00 00 00 00 00 00 00 00 00 06 "
	"03 00 00 01 16 00 00 00 00 02 03 69 01 00 b4 03 00 00 01 0b 00 "
	"00 00 00 02 03 69 01 00 b4 03 00 00 01 0b 00 00 00 00 02 03 69 "
	"01 00 b4 03 00 00 01 0b 00 00 00 00 02 03 69 01 00 07 02 c1 02 "
	"0b 78 7c 03 05 0e 0f 10 07 5b cd 15 0b 78 7c 03 05 0e 0f 10 07 "
	"5b cd 15 0b 78 7c 03 05 0e 0f 10 07 5b cd 15";
static const char	*BIND_CALL_NCHAR=
	"11 69 00 01 01 01 01 01 03 5e 00 02 81 29 00 01 01 48 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 04 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 6e 63 68 61 72 20 28 69 64 2c 20 6e 63 2c "
	"20 6e 76 2c 20 76 29 20 76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 "
	"2c 3a 33 2c 3a 34 29 01 01 01 01 00 00 00 00 00 00 00 00 00 00 "
	"00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 00 01 03 00 00 01 "
	"0e 00 01 10 00 00 02 07 d0 02 00 01 03 00 00 01 0e 00 01 10 00 "
	"00 02 07 d0 02 00 01 03 00 00 01 1c 00 01 10 00 00 02 03 69 01 "
	"00 07 02 c1 03 0e 00 63 00 61 00 66 00 e9 00 20 00 fc 00 f1 0e "
	"00 63 00 61 00 66 00 e9 00 20 00 fc 00 f1 0a 63 61 66 c3 a9 20 "
	"c3 bc c3 b1";
static const char	*BIND_CALL_ROWID=
	"03 5e 00 02 80 29 00 01 01 3a 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 01 01 01 00 00 00 00 00 00 00 73 65 6c 65 63 74 20 69 64 2c "
	"20 76 20 66 72 6f 6d 20 70 72 6f 74 6f 63 6f 6c 74 65 73 74 31 "
	"30 37 34 39 62 72 6f 77 69 64 20 77 68 65 72 65 20 72 6f 77 69 "
	"64 20 3d 20 3a 31 01 01 00 00 00 00 00 00 01 01 00 00 00 00 00 "
	"0b 03 00 00 01 82 00 00 00 00 02 03 69 01 00 07 0e 03 4a c5 89 "
	"01 05 00 02 01 56 01 01";
static const char	*BIND_CALL_ROWID_102=
	"03 5e 00 02 80 29 00 01 01 3a 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 01 01 01 00 00 00 00 00 00 00 73 65 6c 65 63 74 20 69 64 2c "
	"20 76 20 66 72 6f 6d 20 70 72 6f 74 6f 63 6f 6c 74 65 73 74 31 "
	"30 37 34 39 62 72 6f 77 69 64 20 77 68 65 72 65 20 72 6f 77 69 "
	"64 20 3d 20 3a 31 01 01 00 00 00 00 00 00 01 01 00 00 00 00 00 "
	"0b 03 00 00 01 82 00 00 00 00 01 1f 01 00 07 0e 03 03 bc 52 01 "
	"06 00 01 4e 01 01";
static const char	*BIND_CALL_NULLS=
	"11 69 00 01 01 01 01 04 03 5e 00 02 81 29 00 01 01 4a 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 0a 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 6e 75 6c 6c 20 76 61 6c 75 65 73 20 28 3a "
	"31 2c 3a 32 2c 3a 33 2c 3a 34 2c 3a 35 2c 3a 36 2c 3a 37 2c 3a "
	"38 2c 3a 39 2c 3a 31 30 29 01 01 01 01 00 00 00 00 00 00 00 00 "
	"00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 00 01 03 00 "
	"00 01 04 00 01 10 00 00 02 03 69 01 00 06 03 00 00 01 16 00 00 "
	"00 00 02 03 69 01 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 "
	"00 0c 03 00 00 01 07 00 00 00 00 02 03 69 01 00 b4 03 00 00 01 "
	"0b 00 00 00 00 02 03 69 01 00 17 03 00 00 02 07 d0 00 00 00 00 "
	"02 03 69 01 00 01 03 00 00 01 04 00 01 10 00 00 02 03 69 01 00 "
	"06 03 00 00 01 16 00 00 00 00 02 03 69 01 00 06 03 00 00 01 16 "
	"00 00 00 00 02 03 69 01 00 07 02 c1 02 00 00 00 00 00 00 00 00 "
	"00";
static const char	*BIND_CALL_NULLS_REEXECUTE=
	"03 5e 00 02 81 20 01 01 00 00 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 00 00 00 00 00 00 00 00 00 00 01 01 00 00 00 00 00 00 00 00 "
	"00 00 00 07 02 c1 05 01 78 02 c1 06";
static const char	*BIND_CALL_RAW3=
	"11 69 00 01 01 01 01 05 03 5e 00 02 81 29 00 01 01 3a 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 62 79 74 65 73 20 28 69 64 2c 20 72 29 20 "
	"76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 00 "
	"00 00 00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 "
	"01 00 17 03 00 00 02 07 d0 00 00 00 00 02 03 69 01 00 07 02 c1 "
	"02 03 05 12 1f";
static const char	*BIND_CALL_RAW2000=
	"11 69 00 01 01 01 01 01 03 5e 00 02 81 29 00 01 01 3a 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 62 79 74 65 73 20 28 69 64 2c 20 72 29 20 "
	"76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 00 "
	"00 00 00 00 00 00 00 00 06 03 00 00 01 16 00";
static const char	*BIND_CALL_RAW2000_TAIL=
	"77 84 91 9e ab b8 10 c5 d2 df ec f9 06 13 20 2d 3a 47 54 61 6e "
	"7b 88 00";
static const char	*BIND_CALL_LONGRAW2001=
	"11 69 00 01 01 01 01 05 03 5e 00 02 81 29 00 01 01 3a 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 62 79 74 65 73 20 28 69 64 2c 20 72 29 20 "
	"76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 00 "
	"00 00 00 00 00 00 00 00 06 03 00 00 01 16 00";
static const char	*BIND_CALL_LONGRAW2001_TAIL=
	"84 91 9e ab b8 11 c5 d2 df ec f9 06 13 20 2d 3a 47 54 61 6e 7b "
	"88 95 00";
static const char	*BIND_CALL_CHUNK128=
	"03 5e 00 02 80 29 00 01 01 1b 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 01 01 02 00 00 00 00 00 00 00 73 65 6c 65 63 74 20 3a 31 20 "
	"76 2c 20 3a 32 20 72 20 66 72 6f 6d 20 64 75 61 6c 01 01 00 00 "
	"00 00 00 00 01 01 00 00 00 00 00 01 03 00 00 02 02 00 00 01 10 "
	"00 00 02 03 69 01 00 17 03 00 00 02 07 d0 00 00 00 00 02 03 69 "
	"01 00 07 fe 40 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 "
	"71 72 73 74 75 76 77 78 79 7a 61 62 63 64 65 66 67 68 69 6a 6b "
	"6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 61 62 63 64 65 66 "
	"67 68 69 6a 6b 6c 40 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a "
	"61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 "
	"76 77 78 79 7a 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 "
	"71 72 73 74 75 76 77 78 00 fe 40 05 12 1f 2c 39 46 53 60 6d 7a "
	"87 94 a1 ae bb c8 d5 e2 ef fc 09 16 23 30 3d 4a 57 64 71 7e 8b "
	"98 a5 b2 bf cc d9 e6 f3 00 0d 1a 27 34 41 4e 5b 68 75 82 8f 9c "
	"a9 b6 c3 d0 dd ea f7 04 11 1e 2b 38 40 45 52 5f 6c 79 86 93 a0 "
	"ad ba c7 d4 e1 ee fb 08 15 22 2f 3c 49 56 63 70 7d 8a 97 a4 b1 "
	"be cb d8 e5 f2 ff 0c 19 26 33 40 4d 5a 67 74 81 8e 9b a8 b5 c2 "
	"cf dc e9 f6 03 10 1d 2a 37 44 51 5e 6b 78 00";
static const char	*BIND_CALL_LONG32767=
	"11 69 00 01 01 01 01 01 03 5e 00 02 81 29 00 01 01 3b 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 73 74 72 6c 69 6d 20 28 69 64 2c 20 6c 29 "
	"20 76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 "
	"00 00 00 00 00 00 00 00 00 06 03 00 00 01 16";
static const char	*BIND_CALL_LONG32767_TAIL=
	"6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 61 62 63 64 65 "
	"66 67 00";
static const char	*BIND_CALL_PLSQL=
	"11 69 00 01 01 01 01 01 03 5e 00 02 05 29 00 01 01 32 01 01 0d "
	"00 00 00 00 02 7f f8 01 01 02 00 00 00 00 00 00 00 62 65 67 69 "
	"6e 20 64 62 6d 73 5f 75 74 69 6c 69 74 79 2e 67 65 74 5f 74 7a "
	"5f 74 72 61 6e 73 69 74 69 6f 6e 73 28 3a 31 2c 3a 32 29 3b 20 "
	"65 6e 64 3b 01 01 01 01 00 00 00 00 00 00 00 00 00 00 00 06 03 "
	"00 00 01 16 00 00 00 00 02 03 69 01 00 17 03 00 00 02 7f ff 00 "
	"00 00 00 02 03 69 01 00 07 03 c2 07 0d 00";
static const char	*BIND_CALL_TSTZ=
	"11 69 00 01 01 01 01 04 03 5e 00 02 81 29 00 01 01 38 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 74 7a 20 28 69 64 2c 20 74 7a 29 20 76 61 "
	"6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 00 00 00 "
	"00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 00 "
	"b5 03 00 00 01 0d 00 00 00 00 02 03 69 01 00 07 02 c1 03 0d 78 "
	"7c 03 05 13 0f 10 07 5b ca 00 19 5a";
static const char	*BIND_CALL_LTZ=
	"11 69 00 01 01 01 01 04 03 5e 00 02 81 29 00 01 01 39 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 74 7a 20 28 69 64 2c 20 6c 74 7a 29 20 76 "
	"61 6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 00 00 "
	"00 00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 "
	"00 e7 03 00 00 01 0b 00 00 00 00 02 03 69 01 00 07 02 c1 0c 0b "
	"78 7c 03 05 0f 0f 10 07 5b ca 00";
static const char	*BIND_CALL_YM=
	"11 69 00 01 01 02 01 04 01 05 03 5e 00 02 81 29 00 01 01 38 01 "
	"01 0d 00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 "
	"69 6e 73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 "
	"65 73 74 31 30 37 34 39 62 74 7a 20 28 69 64 2c 20 79 6d 29 20 "
	"76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 00 "
	"00 00 00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 "
	"01 00 b6 03 00 00 01 05 00 00 00 00 02 03 69 01 00 07 02 c1 0d "
	"05 80 00 00 0c 41";
static const char	*BIND_CALL_YM_REEXECUTE=
	"03 5e 00 02 81 20 01 01 00 00 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 00 00 00 00 00 00 00 00 00 00 01 01 00 00 00 00 00 00 00 00 "
	"00 00 00 07 02 c1 0e 05 7f ff ff ff 3a";
static const char	*BIND_CALL_DS=
	"11 69 00 01 01 01 01 04 03 5e 00 02 81 29 00 01 01 38 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 02 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 74 7a 20 28 69 64 2c 20 64 73 29 20 76 61 "
	"6c 75 65 73 20 28 3a 31 2c 3a 32 29 01 01 01 01 00 00 00 00 00 "
	"00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 00 "
	"b7 03 00 00 01 0b 00 00 00 00 02 03 69 01 00 07 02 c1 0f 0b 80 "
	"00 00 03 40 41 42 87 5b cd 15";
static const char	*BIND_CALL_GROWN=
	"03 5e 00 02 81 28 01 01 00 00 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 01 01 03 00 00 00 00 00 00 00 00 01 01 00 00 00 00 00 00 00 "
	"00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 00 01 03 "
	"00 00 01 18 00 01 10 00 00 02 03 69 01 00 01 03 00 00 02 01 18 "
	"00 01 10 00 00 02 03 69 01 00 07 02 c1 02 06 73 74 72 69 6e 67 "
	"fe 40 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 "
	"74 75 76 77 78 79 7a 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e "
	"6f 70 71 72 73 74 75 76 77 78 79 7a 61 62 63 64 65 66 67 68 69 "
	"6a 6b 6c 06 6d 6e 6f 70 71 72 00";
static const char	*BIND_CALL_REEXECUTE=
	"03 5e 00 02 81 20 01 01 00 00 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 00 00 00 00 00 00 00 00 00 00 01 01 00 00 00 00 00 00 00 00 "
	"00 00 00 07 02 c1 03 06 73 74 72 69 6e 67 00";
static const char	*BIND_CALL_REEXECUTE_FETCH=
	"03 5e 00 02 80 68 01 05 00 00 01 01 0d 00 00 00 00 04 7f ff ff "
	"ff 01 01 01 00 00 00 00 00 00 00 00 01 0a 00 00 00 00 00 01 01 "
	"00 00 00 00 00 01 03 00 00 02 01 18 00 01 10 00 00 02 03 69 01 "
	"00 07 fe 40 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 "
	"72 73 74 75 76 77 78 79 7a 61 62 63 64 65 66 67 68 69 6a 6b 6c "
	"6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 61 62 63 64 65 66 67 "
	"68 69 6a 6b 6c 06 6d 6e 6f 70 71 72 00";
static const char	*BIND_CALL_REPARSE=
	"03 5e 00 02 80 69 01 01 01 01 15 01 01 0d 00 00 00 00 04 7f ff "
	"ff ff 01 01 01 00 00 00 00 00 00 00 73 65 6c 65 63 74 20 3a 31 "
	"20 61 20 66 72 6f 6d 20 64 75 61 6c 01 01 01 0a 00 00 00 00 00 "
	"01 01 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 "
	"00 07 02 c1 06";
static const char	*BIND_CALL_BINFLOAT=
	"11 69 00 01 01 01 01 05 03 5e 00 02 81 29 00 01 01 3d 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 03 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 62 66 20 28 69 64 2c 20 66 2c 20 64 29 20 "
	"76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 2c 3a 33 29 01 01 01 01 "
	"00 00 00 00 00 00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 "
	"02 03 69 01 00 64 03 00 00 01 04 00 00 00 00 02 03 69 01 00 65 "
	"03 00 00 01 08 00 00 00 00 02 03 69 01 00 07 02 c1 02 04 bf c0 "
	"00 00 08 bf f8 00 00 00 00 00 00";
static const char	*BIND_CALL_NUMBER_INFINITY=
	"11 69 00 01 01 01 01 01 03 5e 00 02 81 29 00 01 01 43 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 04 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 62 66 20 28 69 64 2c 20 66 2c 20 64 2c 20 "
	"6e 29 20 76 61 6c 75 65 73 20 28 3a 31 2c 3a 32 2c 3a 33 2c 3a "
	"34 29 01 01 01 01 00 00 00 00 00 00 00 00 00 00 00 06 03 00 00 "
	"01 16 00 00 00 00 02 03 69 01 00 06 03 00 00 01 16 00 00 00 00 "
	"02 03 69 01 00 06 03 00 00 01 16 00 00 00 00 02 03 69 01 00 06 "
	"03 00 00 01 16 00 00 00 00 02 03 69 01 00 07 03 c2 02 03 02 ff "
	"65 02 ff 65 02 ff 65";
static const char	*BIND_CALL_DEFAULTNCHAR=
	"11 69 00 01 01 01 01 05 03 5e 00 02 81 29 00 01 01 4a 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 04 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 62 6e 63 68 61 72 6e 63 20 28 69 64 2c 20 6e "
	"63 2c 20 6e 76 2c 20 76 29 20 76 61 6c 75 65 73 20 28 3a 31 2c "
	"3a 32 2c 3a 33 2c 3a 34 29 01 01 01 01 00 00 00 00 00 00 00 00 "
	"00 00 00 06 03 00 00 01 16 00 00 00 00 02 07 d0 02 00 01 03 00 "
	"00 01 0e 00 01 10 00 00 02 07 d0 02 00 01 03 00 00 01 06 00 01 "
	"10 00 00 02 07 d0 02 00 01 03 00 00 01 06 00 01 10 00 00 02 07 "
	"d0 02 00 07 02 c2 03 0e 00 63 00 61 00 66 00 e9 00 20 00 fc 00 "
	"f1 06 65 e5 67 2c 8a 9e 06 00 61 00 62 00 63";
static const char	*BIND_CALL_POSTPONED=
	"11 69 00 01 01 01 01 06 03 5e 00 02 81 29 00 01 01 3a 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 05 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 63 77 69 64 65 20 76 61 6c 75 65 73 20 28 3a "
	"31 2c 3a 32 2c 3a 33 2c 3a 34 2c 3a 35 29 01 01 01 01 00 00 00 "
	"00 00 00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 02 03 69 "
	"01 00 01 03 00 00 02 3e 80 00 01 10 00 00 02 03 69 01 00 01 03 "
	"00 00 02 1f 40 00 01 10 00 00 02 03 69 01 00 17 03 00 00 02 07 "
	"d0 00 00 00 00 02 03 69 01 00 01 03 00 00 02 3e 80 00 01 10 00 "
	"00 02 03 69 01 00 07 02 c1 03 fe 40 01 08 0f 16 1d 24 2b 32 39 "
	"40 47 4e 55 5c 63 6a 71 78 7f";
static const char	*BIND_CALL_POSTPONED_TAIL=
	"6b 6c 6d 6e 6f 70 20 71 72 73 74 75 76 77 78 79 7a 61 62 63 64 "
	"65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 00";
static const char	*BIND_CALL_POSTPONED_102=
	"11 69 00 01 01 01 01 01 03 5e 00 02 81 29 00 01 01 3a 01 01 0d "
	"00 00 00 00 04 7f ff ff ff 01 01 05 00 00 00 00 00 00 00 69 6e "
	"73 65 72 74 20 69 6e 74 6f 20 70 72 6f 74 6f 63 6f 6c 74 65 73 "
	"74 31 30 37 34 39 63 77 69 64 65 20 76 61 6c 75 65 73 20 28 3a "
	"31 2c 3a 32 2c 3a 33 2c 3a 34 2c 3a 35 29 01 01 01 01 00 00 00 "
	"00 00 00 00 00 00 00 00 06 03 00 00 01 16 00 00 00 00 01 1f 01 "
	"00 01 03 00 00 02 0f a0 00 01 10 00 00 01 1f 01 00 01 03 00 00 "
	"02 07 d0 00 01 10 00 00 01 1f 01 00 17 03 00 00 02 07 d0 00 00 "
	"00 00 01 1f 01 00 01 03 00 00 02 0f a0 00 01 10 00 00 01 1f 01 "
	"00 07 02 c1 03 fe 40 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e "
	"6f 70 71 72 73 74 75 76 77 78";
static const char	*BIND_CALL_POSTPONED_102_TAIL=
	"6b 6c 6d 6e 6f 70 20 71 72 73 74 75 76 77 78 79 7a 61 62 63 64 "
	"65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 00";

// one setInt()-style bind of "value" into "buffer", for the builder checks
static void ojdbc14BuilderInt(oracleprotocolojdbc14bind *b,
				unsigned char *buffer, uint32_t value,
				uint32_t charsetid, unsigned char formofuse) {
	size_t	size=0;
	putOracleNumber(buffer,&size,value);
	oracleprotocolclient::setOjdbc14Bind(b,6,22,charsetid,formofuse,
							buffer,size);
}

// the bind builders against ojdbc14's own 0x5e calls with binds, before any
// connection: each call built from its binds' fields has to be the real
// one byte for byte, or, for one too big to keep whole, in its size, its
// first bytes and its last ones.  every call is oracle122's but the two
// marked 102, oracle102's, whose descriptors carry charset 31
static void checkOjdbc14BindBuilders() {

	stdoutput.printf("\nojdbc14 bind builders - expect each real "
						"client's bytes\n");

	struct {
		const char	*capture;
		const char	*head;
		const char	*tail;
		size_t		size;
	} vectors[]={
		{"[0023] of 10749-dev-ojdbc14-prepdate-realserver-oracle122",
			BIND_CALL_DATE,NULL,184},
		{"[0023] of 10749-dev-ojdbc14-preptimestamp-realserver-"
			"oracle122",BIND_CALL_TIMESTAMP,NULL,225},
		{"[0031] of 10749-dev-ojdbc14-prepnchar-realserver-oracle122",
			BIND_CALL_NCHAR,NULL,235},
		{"[0029] of 10749-dev-ojdbc14-preprowid-realserver-oracle122",
			BIND_CALL_ROWID,NULL,134},
		{"[0029] of 10749-dev-ojdbc14-preprowid-realserver-oracle102",
			BIND_CALL_ROWID_102,NULL,132},
		{"[0099] of 10749-dev-ojdbc14-prepnull-realserver-oracle122",
			BIND_CALL_NULLS,NULL,295},
		{"[0105] of 10749-dev-ojdbc14-prepnull-realserver-oracle122",
			BIND_CALL_NULLS_REEXECUTE,NULL,54},
		{"[0030] of 10749-dev-ojdbc14-prepbytes-realserver-oracle122",
			BIND_CALL_RAW3,NULL,152},
		{"[0042] of 10749-dev-ojdbc14-prepbytes-realserver-oracle122",
			BIND_CALL_RAW2000,BIND_CALL_RAW2000_TAIL,2182},
		{"[0046] of 10749-dev-ojdbc14-prepbytes-realserver-oracle122",
			BIND_CALL_LONGRAW2001,BIND_CALL_LONGRAW2001_TAIL,2185},
		{"[0016] of 10749-dev-ojdbc14-prepchunk-realserver-oracle122",
			BIND_CALL_CHUNK128,NULL,372},
		{"[0025] of 10749-dev-ojdbc14-prepstrlimit-realserver-"
			"oracle122",BIND_CALL_LONG32767,
			BIND_CALL_LONG32767_TAIL,33432},
		{"[0029] of 10749-dev-ojdbc14-preptz-realserver-oracle122",
			BIND_CALL_PLSQL,NULL,140},
		{"[0037] of 10749-dev-ojdbc14-preptz-realserver-oracle122",
			BIND_CALL_TSTZ,NULL,159},
		{"[0061] of 10749-dev-ojdbc14-preptz-realserver-oracle122",
			BIND_CALL_LTZ,NULL,158},
		{"[0071] of 10749-dev-ojdbc14-preptz-realserver-oracle122",
			BIND_CALL_YM,NULL,153},
		{"[0073] of 10749-dev-ojdbc14-preptz-realserver-oracle122",
			BIND_CALL_YM_REEXECUTE,NULL,55},
		{"[0079] of 10749-dev-ojdbc14-preptz-realserver-oracle122",
			BIND_CALL_DS,NULL,157},
		{"[0025] of 10749-dev-ojdbc14-preprexec-realserver-oracle122",
			BIND_CALL_GROWN,NULL,179},
		{"[0027] of 10749-dev-ojdbc14-preprexec-realserver-oracle122",
			BIND_CALL_REEXECUTE,NULL,57},
		{"[0185] of 10749-dev-ojdbc14-preprexec-realserver-oracle122",
			BIND_CALL_REEXECUTE_FETCH,NULL,139},
		{"[0197] of 10749-dev-ojdbc14-preprexec-realserver-oracle122",
			BIND_CALL_REPARSE,NULL,89},
		{"[0023] of 10749-dev-ojdbc14-prepbinfloat-realserver-"
			"oracle122",BIND_CALL_BINFLOAT,NULL,179},
		{"[0043] of 10749-dev-ojdbc14-prepbinfloat-realserver-"
			"oracle122",BIND_CALL_NUMBER_INFINITY,NULL,196},
		{"[0023] of 10749-dev-ojdbc14-prepncharnc-realserver-"
			"oracle122",BIND_CALL_DEFAULTNCHAR,NULL,225},
		{"[0324] of 10749-dev-ojdbc14-colwide-realserver-oracle122",
			BIND_CALL_POSTPONED,BIND_CALL_POSTPONED_TAIL,12397},
		{"[0324] of 10749-dev-ojdbc14-colwide-realserver-oracle102",
			BIND_CALL_POSTPONED_102,BIND_CALL_POSTPONED_102_TAIL,
			12392}
	};

	const uint32_t	cs=ORA_CHARSET_AL32UTF8;
	const uint32_t	cs102=ORA_8I_CHARSET_WE8ISO8859P1;
	const uint32_t	nc=ORA_8I_CHARSET_AL16UTF16;
	const unsigned char	im=ORA_8I_FORM_IMPLICIT;
	const unsigned char	na=ORA_8I_FORM_NCHAR;

	ojdbc14bindbytes	*p=new ojdbc14bindbytes;
	unsigned char		*expected=new unsigned char[4096];
	oracleprotocolclient	client;
	for (size_t i=0; i<sizeof(vectors)/sizeof(vectors[0]); i++) {

		p->size=0;
		oracleprotocolojdbc14bind	b[10];
		unsigned char			n[10][8];
		size_t				size=0;
		const unsigned char		*v=NULL;
		const unsigned char		*w=NULL;
		uint32_t			closes[2]={0,0};
		uint32_t			closecount=1;
		oracleprotocolojdbc14bindcall	c;
		bytestring::zero(&c,sizeof(c));
		c.options=OJDBC14_PREPARED_COMMIT;
		c.al8i4=AL8I4_DML;
		c.binds=b;
		c.descriptors=true;
		c.values=true;

		switch (i) {
			case 0:
				// setInt(1), setDate() twice
				closes[0]=5;
				c.query="insert into protocoltest10749bdate "
					"(id, d, ts) values (:1,:2,:3)";
				ojdbc14BuilderInt(&b[0],n[0],1,cs,im);
				v=ojdbc14BindHex(p,"78 7c 03 05 01 01 01",
								&size);
				oracleprotocolclient::setOjdbc14Bind(&b[1],12,7,
							cs,im,v,size);
				b[2]=b[1];
				c.bindcount=3;
				break;
			case 1:
				// setInt(1), setTimestamp() three times
				closes[0]=5;
				c.query="insert into protocoltest10749bts "
					"(id, d, ts, ts9) values "
					"(:1,:2,:3,:4)";
				ojdbc14BuilderInt(&b[0],n[0],1,cs,im);
				v=ojdbc14BindHex(p,"78 7c 03 05 0e 0f 10 07 5b "
							"cd 15",&size);
				oracleprotocolclient::setOjdbc14Bind(&b[1],180,
							11,cs,im,v,size);
				b[2]=b[1];
				b[3]=b[1];
				c.bindcount=4;
				break;
			case 2:
				// setInt(2), FORM_NCHAR setString() twice and
				// setString() of the same 7 characters
				closes[0]=1;
				c.query="insert into protocoltest10749bnchar "
					"(id, nc, nv, v) values "
					"(:1,:2,:3,:4)";
				ojdbc14BuilderInt(&b[0],n[0],2,cs,im);
				v=ojdbc14BindHex(p,"00 63 00 61 00 66 00 e9 00 "
						"20 00 fc 00 f1",&size);
				ojdbc14BindNchar(&b[1],v,size);
				b[2]=b[1];
				w=ojdbc14BindHex(p,"63 61 66 c3 a9 20 c3 bc c3 "
								"b1",&size);
				oracleprotocolclient::setOjdbc14Bind(&b[3],1,
							28,cs,im,w,size);
				c.bindcount=4;
				break;
			case 3:
			case 4:
				// setROWID() in a select's where clause
				closecount=0;
				c.options=OJDBC14_PREPARED;
				c.al8i4=AL8I4_QUERY;
				c.query="select id, v from "
					"protocoltest10749browid where "
					"rowid = :1";
				if (i==3) {
					oracleprotocolclient::
						setOjdbc14RowidBind(&b[0],cs,
						0x4ac589,5,0x156,1);
				} else {
					oracleprotocolclient::
						setOjdbc14RowidBind(&b[0],
						cs102,0x03bc52,6,0x4e,1);
				}
				c.bindcount=1;
				break;
			case 5:
				// setInt(1) and nine setNull()s
				{
				static const unsigned char	types[]={
					1,6,6,12,180,23,1,6,6
				};
				closes[0]=4;
				c.query="insert into protocoltest10749bnull "
					"values (:1,:2,:3,:4,:5,:6,:7,:8,:9,"
					":10)";
				ojdbc14BuilderInt(&b[0],n[0],1,cs,im);
				for (uint32_t t=0; t<9; t++) {
					oracleprotocolclient::setOjdbc14Bind(
						&b[1+t],types[t],
						(types[t]==1)?4:
						ojdbc14BindMaxLength(types[t]),
						cs,im,NULL,0);
				}
				c.bindcount=10;
				}
				break;
			case 6:
				// its re-execute with setInt(4), "x", setInt(5)
				closecount=0;
				c.options=OJDBC14_REEXECUTE_DML;
				c.cursorid=1;
				c.al8i4=AL8I4_REEXECUTE_DML;
				c.descriptors=false;
				ojdbc14BuilderInt(&b[0],n[0],4,cs,im);
				oracleprotocolclient::setOjdbc14Bind(&b[1],1,4,
					cs,im,(const unsigned char *)"x",1);
				ojdbc14BuilderInt(&b[2],n[2],5,cs,im);
				c.bindcount=3;
				break;
			case 7:
			case 8:
			case 9:
				// setInt() and setBytes() of 3, 2000 and 2001
				closes[0]=(i==8)?1:5;
				c.query="insert into protocoltest10749bbytes "
					"(id, r) values (:1,:2)";
				ojdbc14BuilderInt(&b[0],n[0],
					(i==7)?1:(i==8)?4:5,cs,im);
				size=(i==7)?3:(i==8)?2000:2001;
				v=ojdbc14BindPattern(p,PATTERN_BYTES,size);
				oracleprotocolclient::setOjdbc14Bind(&b[1],
					(i==9)?24:23,(i==9)?0x7fffffff:2000,
					cs,im,v,size);
				c.bindcount=2;
				break;
			case 10:
				// 128 characters and 128 bytes, two chunks
				closecount=0;
				c.options=OJDBC14_PREPARED;
				c.al8i4=AL8I4_QUERY;
				c.query="select :1 v, :2 r from dual";
				v=ojdbc14BindPattern(p,PATTERN_LOWER,128);
				oracleprotocolclient::setOjdbc14Bind(&b[0],1,
							512,cs,im,v,128);
				w=ojdbc14BindPattern(p,PATTERN_BYTES,128);
				oracleprotocolclient::setOjdbc14Bind(&b[1],23,
							2000,cs,im,w,128);
				c.bindcount=2;
				break;
			case 11:
				// setString() of 32767 characters, a LONG
				closes[0]=1;
				c.query="insert into protocoltest10749bstrlim "
					"(id, l) values (:1,:2)";
				ojdbc14BuilderInt(&b[0],n[0],2,cs,im);
				v=ojdbc14BindPattern(p,PATTERN_LOWER,32767);
				oracleprotocolclient::setOjdbc14Bind(&b[1],8,
					0x7fffffff,cs,im,v,32767);
				c.bindcount=2;
				break;
			case 12:
				// the driver's own PL/SQL call: the region
				// id 612 and a null RAW out bind
				closes[0]=1;
				c.options=OJDBC14_PLSQL;
				c.maxlong=ORA_8I_MAX_LONG_PLSQL;
				c.query="begin dbms_utility.get_tz_transitions"
							"(:1,:2); end;";
				ojdbc14BuilderInt(&b[0],n[0],612,cs,im);
				oracleprotocolclient::setOjdbc14Bind(&b[1],23,
						32767,cs,im,NULL,0);
				c.bindcount=2;
				break;
			case 13:
			case 14:
			case 15:
			case 17:
				// setTIMESTAMPTZ() at +05:30,
				// setTIMESTAMPLTZ(), setINTERVALYM("+12-05")
				// and setINTERVALDS("+3 04:05:06.123456789")
				{
				static const char	*columns[]={
					"tz","ltz","ym","","ds"
				};
				static const unsigned char	types[]={
					181,231,182,0,183
				};
				static const char	*values[]={
					"78 7c 03 05 13 0f 10 07 5b ca 00 19 "
					"5a",
					"78 7c 03 05 0f 0f 10 07 5b ca 00",
					"80 00 00 0c 41","",
					"80 00 00 03 40 41 42 87 5b cd 15"
				};
				static const uint32_t	ids[]={2,11,12,0,14};
				size_t		t=i-13;
				closes[0]=4;
				if (i==15) {
					closes[1]=5;
					closecount=2;
				}
				stringbuffer	*q=new stringbuffer;
				q->append("insert into protocoltest10749btz "
						"(id, ")->append(columns[t]);
				q->append(") values (:1,:2)");
				c.query=q->detachString();
				delete q;
				ojdbc14BuilderInt(&b[0],n[0],ids[t],cs,im);
				v=ojdbc14BindHex(p,values[t],&size);
				oracleprotocolclient::setOjdbc14Bind(&b[1],
					types[t],ojdbc14BindMaxLength(types[t]),
					cs,im,v,size);
				c.bindcount=2;
				}
				break;
			case 16:
				// its re-execute with setINTERVALYM("-01-02")
				closecount=0;
				c.options=OJDBC14_REEXECUTE_DML;
				c.cursorid=1;
				c.al8i4=AL8I4_REEXECUTE_DML;
				c.descriptors=false;
				ojdbc14BuilderInt(&b[0],n[0],13,cs,im);
				v=ojdbc14BindHex(p,"7f ff ff ff 3a",&size);
				oracleprotocolclient::setOjdbc14Bind(&b[1],182,
							5,cs,im,v,size);
				c.bindcount=2;
				break;
			case 18:
			case 19:
				// a re-execute whose third value outgrew its
				// mxl, 1 character then 70, and the next with
				// a null, the descriptors kept
				closecount=0;
				c.options=(i==18)?OJDBC14_REEXECUTE_GROWN:
							OJDBC14_REEXECUTE_DML;
				c.cursorid=1;
				c.al8i4=AL8I4_REEXECUTE_DML;
				c.descriptors=(i==18);
				ojdbc14BuilderInt(&b[0],n[0],(i==18)?1:2,
								cs,im);
				oracleprotocolclient::setOjdbc14Bind(&b[1],1,
					24,cs,im,
					(const unsigned char *)"string",6);
				v=(i==18)?ojdbc14BindPattern(p,PATTERN_LOWER,
								70):NULL;
				oracleprotocolclient::setOjdbc14Bind(&b[2],1,
					280,cs,im,v,(v)?70:0);
				c.bindcount=3;
				break;
			case 20:
				// a query's re-execute with fetch of 70
				// characters
				closecount=0;
				c.options=OJDBC14_REEXECUTE_QUERY;
				c.cursorid=5;
				c.al8i4=AL8I4_REEXECUTE_QUERY;
				v=ojdbc14BindPattern(p,PATTERN_LOWER,70);
				oracleprotocolclient::setOjdbc14Bind(&b[0],1,
							280,cs,im,v,70);
				c.bindcount=1;
				break;
			case 21:
				// its re-parse with setInt(5)
				closecount=0;
				c.options=OJDBC14_REPARSE_QUERY;
				c.cursorid=1;
				c.query="select :1 a from dual";
				c.al8i4=AL8I4_REPARSE_QUERY;
				ojdbc14BuilderInt(&b[0],n[0],5,cs,im);
				c.bindcount=1;
				break;
			case 22:
				// setBinaryFloat(1.5f) and setBinaryDouble(1.5)
				closes[0]=5;
				c.query="insert into protocoltest10749bbf "
					"(id, f, d) values (:1,:2,:3)";
				ojdbc14BuilderInt(&b[0],n[0],1,cs,im);
				v=ojdbc14BindHex(p,"bf c0 00 00",&size);
				oracleprotocolclient::setOjdbc14Bind(&b[1],100,
							4,cs,im,v,size);
				w=ojdbc14BindHex(p,"bf f8 00 00 00 00 00 00",
								&size);
				oracleprotocolclient::setOjdbc14Bind(&b[2],101,
							8,cs,im,w,size);
				c.bindcount=3;
				break;
			case 23:
				// setFloat(), setDouble() twice of +Inf, a
				// NUMBER infinity
				closes[0]=1;
				c.query="insert into protocoltest10749bbf "
					"(id, f, d, n) values (:1,:2,:3,:4)";
				ojdbc14BuilderInt(&b[0],n[0],102,cs,im);
				v=ojdbc14BindHex(p,"ff 65",&size);
				for (uint32_t t=1; t<4; t++) {
					oracleprotocolclient::setOjdbc14Bind(
						&b[t],6,22,cs,im,v,size);
				}
				c.bindcount=4;
				break;
			case 24:
				// a defaultNChar connection's setInt(200) and
				// three setString()s, every bind national
				closes[0]=5;
				c.query="insert into protocoltest10749bncharnc "
					"(id, nc, nv, v) values "
					"(:1,:2,:3,:4)";
				ojdbc14BuilderInt(&b[0],n[0],200,nc,na);
				v=ojdbc14BindHex(p,"00 63 00 61 00 66 00 e9 00 "
						"20 00 fc 00 f1",&size);
				ojdbc14BindNchar(&b[1],v,size);
				v=ojdbc14BindHex(p,"65 e5 67 2c 8a 9e",&size);
				ojdbc14BindNchar(&b[2],v,size);
				v=ojdbc14BindHex(p,"00 61 00 62 00 63",&size);
				ojdbc14BindNchar(&b[3],v,size);
				c.bindcount=4;
				break;
			case 25:
			case 26:
				// colwide's insert at 4000 characters: the
				// values of :2, :3 and :5 behind :1's and
				// :4's at 873, every value in order at 31
				{
				uint32_t	r=(i==25)?4:1;
				uint32_t	charset=(i==25)?cs:cs102;
				closes[0]=(i==25)?6:1;
				c.query="insert into protocoltest10749cwide "
						"values (:1,:2,:3,:4,:5)";
				ojdbc14BuilderInt(&b[0],n[0],2,charset,im);
				v=ojdbc14BindPattern(p,PATTERN_LOWER,4000);
				oracleprotocolclient::setOjdbc14Bind(&b[1],1,
						4000*r,charset,im,v,4000);
				w=ojdbc14BindPattern(p,PATTERN_UPPER,2000);
				oracleprotocolclient::setOjdbc14Bind(&b[2],1,
						2000*r,charset,im,w,2000);
				w=ojdbc14BindPattern(p,PATTERN_BYTES7,2000);
				oracleprotocolclient::setOjdbc14Bind(&b[3],23,
						2000,charset,im,w,2000);
				b[4]=b[1];
				c.bindcount=5;
				}
				break;
		}

		client.buildOjdbc14BindCall(&c,(closecount)?closes:NULL,
								closecount);
		if (i>=13 && i<=17 && i!=16) {
			delete[] c.query;
		}

		// behind the header and the data flags
		const unsigned char	*request=client.getRequest()+
							ORA_FRAGMENT_OVERHEAD;
		size_t			requestsize=client.getRequestSize()-
							ORA_FRAGMENT_OVERHEAD;
		size_t	headsize=ojdbc14Hex(vectors[i].head,expected,4096);
		bool	ok=(requestsize==vectors[i].size &&
				headsize<=requestsize &&
				!bytestring::compare(request,expected,
								headsize));
		size_t	tailsize=0;
		if (ok && vectors[i].tail) {
			tailsize=ojdbc14Hex(vectors[i].tail,expected,4096);
			ok=(tailsize<=requestsize &&
				!bytestring::compare(
					request+requestsize-tailsize,
					expected,tailsize));
		}
		char	label[256];
		if (vectors[i].tail) {
			charstring::printf(label,sizeof(label),
				"ojdbc14 bind builder reproduces %d bytes, its "
				"first %d and last %d checked, packet %s",
				(int)vectors[i].size,(int)headsize,
				(int)tailsize,vectors[i].capture);
		} else {
			charstring::printf(label,sizeof(label),
				"ojdbc14 bind builder reproduces %d bytes, "
				"packet %s",(int)vectors[i].size,
				vectors[i].capture);
		}
		report(label,ok);
		if (!ok) {
			dumpOjdbc14Head("built",request,requestsize);
			headsize=ojdbc14Hex(vectors[i].head,expected,4096);
			dumpOjdbc14Head("expected",expected,headsize);
		}
	}
	delete[] expected;
	delete p;
}

// the tables of each bind session, dropped through a session of their own
// if the session stops before its own drops - see ojdbc14BindCleanup()
static const char	*BIND_TABLES_NUMBER[]={"protocoltest10749tbnum",NULL};
static const char	*BIND_TABLES_DATE[]={
	"protocoltest10749tbdate","protocoltest10749tbdatets",
	"protocoltest10749tbts",NULL
};
static const char	*BIND_TABLES_TZ[]={
	"protocoltest10749tbtz","protocoltest10749tbltz",
	"protocoltest10749tbym","protocoltest10749tbds",NULL
};
static const char	*BIND_TABLES_BINFLOAT[]={
	"protocoltest10749tbbf","protocoltest10749tbbd",NULL
};
static const char	*BIND_TABLES_RAW[]={
	"protocoltest10749tbraw","protocoltest10749tblraw",NULL
};
static const char	*BIND_TABLES_STRING[]={
	"protocoltest10749tbstr","protocoltest10749tbstr2",NULL
};
static const char	*BIND_TABLES_NCHAR[]={
	"protocoltest10749tbnv","protocoltest10749tbnc",NULL
};
static const char	*BIND_TABLES_NULL[]={"protocoltest10749tbnull",NULL};
static const char	*BIND_TABLES_ROWID[]={"protocoltest10749tbrowid",NULL};
static const char	*BIND_TABLES_ORDER[]={
	"protocoltest10749tbwide","protocoltest10749tblong",NULL
};
static const char	*BIND_TABLES_PLSQL[]={"protocoltest10749tbplsql",NULL};
static const char	*BIND_TABLES_REEXEC[]={"protocoltest10749tbrexec",NULL};
static const char	*BIND_TABLES_ERRORS[]={"protocoltest10749tberr",NULL};

// a session of its own that drops "tables", for a bind session that stopped
// before its drops did: a login, a drop of each, whatever it answers, and the
// logoff.  nothing in it is a check
static void ojdbc14BindCleanup(const listenerinfo *li, const char *user,
						const char * const *tables) {
	oracleprotocolclient	client;
	client.setConnectVersion(clientconnectversion);
	client.setDataTypeRequest(ORA_OJDBC14_DATA_TYPE_REQUEST,
					sizeof(ORA_OJDBC14_DATA_TYPE_REQUEST));
	client.setAnoNegotiation(true);
	if (!client.connect(host,getPort(li),sid) ||
		!client.versionCall() ||
		!client.requestChallenge8i(user) ||
		!client.answerChallenge8i(user,"testpassword")) {
		stdoutput.printf("    cleanup: no session, the tables are "
								"left\n");
		return;
	}
	oracleprotocolojdbc14answer	*a=new oracleprotocolojdbc14answer;
	uint32_t			open[16];
	uint32_t			count=0;
	for (const char * const *t=tables; *t; t++) {
		stringbuffer	drop;
		drop.append("drop table ")->append(*t)->append(" purge");
		oracleprotocolojdbc14query	q;
		bytestring::zero(&q,sizeof(q));
		q.options=OJDBC14_STATEMENT_COMMIT;
		q.query=drop.getString();
		q.al8i4=AL8I4_DML;
		uint32_t	closecount=(count)?1:0;
		client.buildOjdbc14Query(&q,(count)?&open[count-1]:NULL,
								closecount);
		if (!client.sendPacket() || !client.recvOjdbc14Answer() ||
			!client.readOjdbc14Answer(a,0,false)) {
			stdoutput.printf("    cleanup: %s not answered\n",
							drop.getString());
			break;
		}
		if (closecount) {
			count--;
		}
		if (a->cursorid && count<16) {
			open[count++]=(uint32_t)a->cursorid;
		}
		stdoutput.printf("    cleanup: %s: error %d\n",
					drop.getString(),(int)a->retcode);
	}
	delete a;
	client.logoff8i(open,count);
}

// ---- the reader against real servers' answers ----

// answers real servers sent genuine ojdbc14, which readOjdbc14Answer() has
// to read the way ojdbc14 does before any listener is asked anything: rows
// with columns described 0 bytes wide (select null), 0x15 bit vectors, a
// bit vector inside a row header, LONG values with the indicator and the
// return code behind them, ROWIDs in their 0e form and UROWIDs (type 208)
// [0022] of 10749-dev-ojdbc14-selectnull-realserver-oracle122, 182 bytes
static const char	*REAL_SELECTNULL_DESCRIBE=
	"10 17 7c 24 91 7e ad 00 99 8e fb 96 ad b6 50 88 2f bf 78 7e 0a "
	"13 14 29 0e 01 03 01 04 51 01 80 00 00 00 00 00 00 00 01 01 01 "
	"00 01 01 01 01 01 41 00 00 00 02 00 00 81 7f 01 02 00 00 00 00 "
	"00 00 00 01 01 01 01 01 42 00 00 01 01 01 80 00 00 00 00 00 00 "
	"00 01 01 01 00 01 01 01 01 01 43 00 00 01 02 60 80 00 00 01 01 "
	"00 00 00 00 02 03 69 01 01 01 01 01 01 01 01 44 00 00 01 03 01 "
	"07 07 78 7e 0a 13 15 06 3a 00 02 1f e8 08 01 06 04 22 d4 40 f2 "
	"00 01 01 00 00 00 00 00 04 01 07 00 00 00 00 01 01 00 03 00 00 "
	"00 00 00 00 00 00 00 00 00 00 00 00 01 01";
// [0024] of 10749-dev-ojdbc14-selectnull-realserver-oracle122, 71 bytes
static const char	*REAL_SELECTNULL_FETCH=
	"06 22 01 04 00 01 0a 00 00 00 07 02 c1 02 01 78 04 01 08 01 01 "
	"02 05 7b 00 00 01 01 00 03 00 00 00 00 00 00 00 00 00 00 00 00 "
	"00 00 00 19 4f 52 41 2d 30 31 34 30 33 3a 20 6e 6f 20 64 61 74 "
	"61 20 66 6f 75 6e 64 0a";
// [0041] of 10749-dev-ojdbc14-colrowid-realserver-oracle122, 139 bytes
static const char	*REAL_ROWID_DESCRIBE=
	"10 17 5e 8e c4 fc 76 29 3b aa 5a 7a 21 bf f8 4a 27 4e 78 7e 0a "
	"13 15 06 3a 01 26 01 02 51 0b 00 00 00 01 01 00 00 00 00 00 00 "
	"00 00 01 01 01 01 52 00 00 00 02 00 00 81 7f 01 16 00 00 00 00 "
	"00 00 00 01 02 01 02 02 49 44 00 00 01 01 01 07 07 78 7e 0a 13 "
	"15 06 3a 00 02 1f e8 08 01 06 04 22 d4 41 ae 00 01 07 00 00 00 "
	"00 00 04 01 12 00 00 00 00 01 07 01 18 03 00 00 00 00 00 04 ff "
	"ff ff ff 00 00 00 00 00 00 00 00 01 01";
// [0043] of 10749-dev-ojdbc14-colrowid-realserver-oracle122, 124 bytes
static const char	*REAL_ROWID_FETCH=
	"06 22 01 02 00 01 0a 00 00 00 07 0e 03 4a c5 72 01 05 00 02 04 "
	"0e 00 02 c1 02 07 0e 03 4a c5 72 01 05 00 02 04 0e 01 01 02 c1 "
	"03 15 01 02 03 07 0e 03 4a c5 72 01 05 00 02 04 0e 01 02 02 c1 "
	"04 04 01 13 01 03 02 05 7b 00 00 01 07 00 03 00 01 20 00 00 00 "
	"04 ff ff ff ff 00 00 00 00 00 00 00 00 00 19 4f 52 41 2d 30 31 "
	"34 30 33 3a 20 6e 6f 20 64 61 74 61 20 66 6f 75 6e 64 0a";
// [0049] of 10749-dev-ojdbc14-colrowid-realserver-oracle122, 140 bytes
static const char	*REAL_IOT_DESCRIBE=
	"10 17 db fd e8 12 60 60 db 73 ab 32 bb 16 c3 3f 0a 5e 78 7e 0a "
	"13 15 06 3a 01 35 01 02 51 d0 00 00 00 01 1f 00 00 00 00 00 00 "
	"00 00 01 01 01 01 52 00 00 00 02 00 00 81 7f 01 16 00 00 00 00 "
	"00 00 00 00 02 01 02 02 49 44 00 00 01 01 01 07 07 78 7e 0a 13 "
	"15 06 3a 01 02 02 1f e8 08 01 06 04 22 d4 41 e4 00 01 07 00 00 "
	"00 00 00 04 01 18 00 00 00 00 01 07 01 18 03 00 00 00 00 00 04 "
	"ff ff ff ff 00 00 00 00 00 00 00 00 01 01";
// [0051] of 10749-dev-ojdbc14-colrowid-realserver-oracle122, 125 bytes
static const char	*REAL_IOT_FETCH=
	"06 22 01 02 00 01 0a 00 00 00 07 01 0a 0a 02 04 01 40 04 13 02 "
	"c1 02 fe 02 c1 02 07 01 0a 0a 02 04 01 40 04 13 02 c1 03 fe 02 "
	"c1 03 15 01 02 03 07 01 0a 0a 02 04 01 40 04 13 02 c1 04 fe 02 "
	"c1 04 04 01 19 01 03 02 05 7b 00 00 01 07 00 03 00 01 20 00 00 "
	"00 04 ff ff ff ff 00 00 00 00 00 00 00 00 00 19 4f 52 41 2d 30 "
	"31 34 30 33 3a 20 6e 6f 20 64 61 74 61 20 66 6f 75 6e 64 0a";
// [0056] of 10749-dev-ojdbc14-collong-realserver-oracle122, 142 bytes
static const char	*REAL_LONG_DESCRIBE=
	"10 17 19 0f 1a 70 c8 35 e9 08 da d0 31 7d f9 85 70 1d 78 7e 0a "
	"13 15 06 3a 00 01 02 51 02 00 00 81 7f 01 16 00 00 00 00 00 00 "
	"00 01 02 01 02 02 49 44 00 00 00 08 80 00 00 00 00 00 00 00 02 "
	"03 69 01 00 01 01 01 01 01 4c 00 00 01 01 01 07 07 78 7e 0a 13 "
	"15 06 3a 00 02 1f e8 08 01 06 04 22 d4 42 2a 00 01 05 00 00 00 "
	"00 00 04 01 24 00 00 00 00 01 05 01 12 03 00 00 00 00 00 03 4a "
	"c5 73 01 05 00 02 04 2e 01 05 00 00 00 00 01 01";
// [0062] of 10749-dev-ojdbc14-collong-realserver-oracle122, 122 bytes
static const char	*REAL_LONG_FETCH3=
	"06 02 01 02 00 01 01 00 01 01 01 03 00 07 02 c1 04 fe 41 61 62 "
	"62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 "
	"62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 "
	"62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 62 "
	"00 00 00 04 01 27 01 03 00 00 00 01 05 00 03 00 01 20 00 00 00 "
	"03 4a c5 73 01 05 00 02 04 2e 01 05 00 00 00 00 00";
// [0047] of 10749-dev-ojdbc14-colbinfloat-realserver-oracle122, 166 bytes
static const char	*REAL_BF_DESCRIBE=
	"10 17 2a a5 e4 90 95 f4 8a db 29 2f 16 5f 25 2f 21 32 78 7e 0a "
	"13 15 06 3a 01 22 01 03 51 02 00 00 81 7f 01 16 00 00 00 00 00 "
	"00 00 01 02 01 02 02 49 44 00 00 00 64 00 00 00 01 01 00 00 00 "
	"00 00 00 00 01 01 01 01 01 46 00 00 01 01 65 00 00 00 01 01 00 "
	"00 00 00 00 00 00 01 01 01 01 01 44 00 00 01 02 01 07 07 78 7e "
	"0a 13 15 06 3a 01 01 02 1f e8 08 01 06 04 22 d4 42 02 00 01 01 "
	"00 00 00 00 00 04 01 1e 00 00 00 00 01 01 01 15 03 00 00 00 00 "
	"00 03 4a c5 6e 01 05 00 02 01 6d 01 0a 00 00 00 00 01 01";
// [0049] of 10749-dev-ojdbc14-colbinfloat-realserver-oracle122, 257 bytes
static const char	*REAL_BF_FETCH=
	"06 22 01 03 00 01 0a 00 00 00 07 02 c1 02 04 bf c0 00 00 08 bf "
	"f8 00 00 00 00 00 00 07 02 c1 03 04 ff c0 00 00 08 ff f8 00 00 "
	"00 00 00 00 15 01 03 07 07 02 c1 04 04 ff 80 00 00 08 ff f0 00 "
	"00 00 00 00 00 15 01 03 07 07 02 c1 05 04 00 7f ff ff 08 00 0f "
	"ff ff ff ff ff ff 15 01 03 07 07 02 c1 06 04 80 00 00 00 08 80 "
	"00 00 00 00 00 00 00 15 01 03 07 07 02 c1 07 04 ff 7f ff ff 08 "
	"ff ef ff ff ff ff ff ff 15 01 03 07 07 02 c1 08 04 80 80 00 00 "
	"08 80 10 00 00 00 00 00 00 15 01 03 07 07 02 c1 09 04 80 00 00 "
	"01 08 80 00 00 00 00 00 00 01 15 01 03 07 07 02 c1 0a 04 00 80 "
	"00 00 08 00 10 00 00 00 00 00 00 15 01 03 07 07 02 c1 0b 04 80 "
	"00 00 00 08 80 00 00 00 00 00 00 00 04 01 1f 01 0a 00 00 00 01 "
	"01 00 03 00 01 20 00 00 00 03 4a c5 6e 01 05 00 02 01 6d 01 0a "
	"00 00 00 00 00";

// [0044] of 10749-dev-ojdbc14-preptz-realserver-oracle122, 78 bytes: the
// answer to ojdbc14's own get_tz_transitions call for UTC, an io vector of
// an in and an out bind, the RAW out value behind it, and the summary
static const char	*REAL_IOV_ANSWER=
	"0b 05 01 02 00 01 01 00 00 00 20 10 07 0b 01 76 64 01 01 01 01 "
	"01 14 3c 00 00 08 01 06 04 22 d4 46 82 00 01 04 01 02 00 00 00 "
	"00 04 01 18 01 01 00 00 00 01 04 00 2f 00 00 00 00 00 03 4a c5 "
	"88 01 05 00 02 01 56 01 01 00 00 00 00 01 01";

// one captured answer, read; false, reported, if it can't be
static bool readRealOjdbc14Answer(oracleprotocolclient *client,
					oracleprotocolojdbc14answer *a,
					const char *hex, uint32_t cursorid,
					const char *what) {
	unsigned char	*bytes=new unsigned char[4096];
	size_t		size=ojdbc14Hex(hex,bytes,4096);
	client->setOjdbc14Answer(bytes,size,cursorid);
	delete[] bytes;
	bool	ok=client->readOjdbc14Answer(a,0,false);
	stringbuffer	l;
	l.append("ojdbc14 answer reader reads ")->append(what);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
	}
	return ok;
}

// one value of the last answer read: null by describe, or its bytes, and,
// for a LONG, its indicator and return code
static void checkRealOjdbc14Value(oracleprotocolojdbc14answer *a,
					const char *what, uint32_t row,
					uint32_t column, const char *hex) {
	const oracleprotocolojdbc14value	*v=&a->values[row][column];
	char	label[256];
	bool	ok;
	if (!hex) {
		charstring::printf(label,sizeof(label),
				"ojdbc14 answer reader finds row %d column "
				"%d of %s null by describe, with no bytes",
				(int)row+1,(int)column,what);
		ok=(v->isnull && v->nullbydescribe && !v->wiresize);
	} else {
		unsigned char	bytes[256];
		size_t		size=ojdbc14Hex(hex,bytes,sizeof(bytes));
		charstring::printf(label,sizeof(label),
				"ojdbc14 answer reader finds row %d column "
				"%d of %s to be %s",
				(int)row+1,(int)column,what,hex);
		ok=(!v->isnull && v->size==size &&
			!bytestring::compare(a->valuedata+v->offset,
							bytes,size));
	}
	report(label,ok);
	if (!ok) {
		dumpBytes("got",a->valuedata+v->offset,v->size);
	}
}

static void checkOjdbc14AnswerReader() {

	stdoutput.printf("\nojdbc14 answer reader - expect real servers' "
					"answers read as ojdbc14 reads them\n");

	oracleprotocolclient		client;
	oracleprotocolojdbc14answer	*a=new oracleprotocolojdbc14answer;
	char				label[256];
	char				text[19];

	// select null a, 1 b, null c, 'x' d: nothing for a and c
	const char	*what="[0022] of 10749-dev-ojdbc14-selectnull-"
						"realserver-oracle122";
	if (readRealOjdbc14Answer(&client,a,REAL_SELECTNULL_DESCRIBE,0,what)) {
		uint32_t	cursorid=(uint32_t)a->al8o4[2];
		what="[0024] of 10749-dev-ojdbc14-selectnull-realserver-"
								"oracle122";
		if (readRealOjdbc14Answer(&client,a,REAL_SELECTNULL_FETCH,
							cursorid,what)) {
			checkRealOjdbc14Value(a,what,0,0,NULL);
			checkRealOjdbc14Value(a,what,0,1,"c1 02");
			checkRealOjdbc14Value(a,what,0,2,NULL);
			checkRealOjdbc14Value(a,what,0,3,"78");
		}
	}

	// a heap table's rowids, the third behind a bit vector, which read
	// as the real server's cast of them to varchar2 said ([0059] of the
	// same capture)
	what="[0041] of 10749-dev-ojdbc14-colrowid-realserver-oracle122";
	if (readRealOjdbc14Answer(&client,a,REAL_ROWID_DESCRIBE,0,what)) {
		uint32_t	cursorid=(uint32_t)a->al8o4[2];
		what="[0043] of 10749-dev-ojdbc14-colrowid-realserver-"
								"oracle122";
		if (readRealOjdbc14Answer(&client,a,REAL_ROWID_FETCH,
							cursorid,what)) {
			const char	*rowids[]={
				"AASsVyAAFAAAAQOAAA",
				"AASsVyAAFAAAAQOAAB",
				"AASsVyAAFAAAAQOAAC"
			};
			for (uint32_t r=0; r<3; r++) {
				oracleprotocolclient::decodeOjdbc14Rowid(
						&a->values[r][0],text);
				charstring::printf(label,sizeof(label),
					"ojdbc14 answer reader reads row %d's "
					"rowid of %s as %s",(int)r+1,what,
					rowids[r]);
				report(label,a->rowcount==3 &&
					!charstring::compare(text,rowids[r]));
				if (charstring::compare(text,rowids[r])) {
					stdoutput.printf("    got %s\n",text);
				}
			}
			checkRealOjdbc14Value(a,what,2,1,"c1 04");
		}
	}

	// an index-organized table's rowids, type 208: a ub4 and a clr
	what="[0049] of 10749-dev-ojdbc14-colrowid-realserver-oracle122";
	if (readRealOjdbc14Answer(&client,a,REAL_IOT_DESCRIBE,0,what)) {
		uint32_t	cursorid=(uint32_t)a->al8o4[2];
		what="[0051] of 10749-dev-ojdbc14-colrowid-realserver-"
								"oracle122";
		if (readRealOjdbc14Answer(&client,a,REAL_IOT_FETCH,
							cursorid,what)) {
			checkRealOjdbc14Value(a,what,0,0,
					"02 04 01 40 04 13 02 c1 02 fe");
			checkRealOjdbc14Value(a,what,2,0,
					"02 04 01 40 04 13 02 c1 04 fe");
			checkRealOjdbc14Value(a,what,2,1,"c1 04");
		}
	}

	// a LONG of 65 bytes, behind a row header with a bit vector in it,
	// then 00 00 for its indicator and return code
	what="[0056] of 10749-dev-ojdbc14-collong-realserver-oracle122";
	if (readRealOjdbc14Answer(&client,a,REAL_LONG_DESCRIBE,0,what)) {
		uint32_t	cursorid=(uint32_t)a->al8o4[2];
		what="[0062] of 10749-dev-ojdbc14-collong-realserver-"
								"oracle122";
		if (readRealOjdbc14Answer(&client,a,REAL_LONG_FETCH3,
							cursorid,what)) {
			checkRealOjdbc14Value(a,what,0,0,"c1 04");
			const oracleprotocolojdbc14value	*v=
							&a->values[0][1];
			char	value[65];
			value[0]='a';
			bytestring::set(value+1,'b',64);
			charstring::printf(label,sizeof(label),
				"ojdbc14 answer reader finds row 1 column 1 "
				"of %s to be a, 64 b's, indicator 0 and "
				"return code 0",what);
			report(label,a->rowcount==1 && a->bitvectorsize==1 &&
				v->size==65 &&
				!bytestring::compare(a->valuedata+v->offset,
								value,65) &&
				!v->indicator && !v->returncode);
		}
	}

	// BINARY_FLOAT and BINARY_DOUBLE rows, 0x15 bit vectors between
	what="[0047] of 10749-dev-ojdbc14-colbinfloat-realserver-oracle122";
	if (readRealOjdbc14Answer(&client,a,REAL_BF_DESCRIBE,0,what)) {
		uint32_t	cursorid=(uint32_t)a->al8o4[2];
		what="[0049] of 10749-dev-ojdbc14-colbinfloat-realserver-"
								"oracle122";
		if (readRealOjdbc14Answer(&client,a,REAL_BF_FETCH,
							cursorid,what)) {
			struct {
				uint32_t	row;
				uint32_t	column;
				const char	*name;
				double		value;
			} values[]={
				{0,1,"1.5",1.5},
				{0,2,"1.5",1.5},
				{5,1,"3.4028234663852886E38",
						3.4028234663852886E38},
				{5,2,"1.7976931348623157E308",
						1.7976931348623157E308},
				{7,1,"1.401298464324817E-45",
						1.401298464324817E-45},
				{8,2,"-1.7976931348623157E308",
						-1.7976931348623157E308},
				{9,2,"0",0.0}
			};
			for (size_t i=0; i<sizeof(values)/sizeof(values[0]);
									i++) {
				const oracleprotocolojdbc14value	*v=
				&a->values[values[i].row][values[i].column];
				double	d=-1;
				bool	ok=oracleprotocolclient::
					decodeOjdbc14BinaryFloat(
						a->valuedata+v->offset,
						v->size,&d);
				charstring::printf(label,sizeof(label),
					"ojdbc14 answer reader reads row %d "
					"column %d of %s as %s",
					(int)values[i].row+1,
					(int)values[i].column,what,
					values[i].name);
				report(label,a->rowcount==10 && ok &&
						d==values[i].value);
				if (d!=values[i].value) {
					stdoutput.printf("    got %.17g\n",d);
				}
			}
			const oracleprotocolojdbc14value	*v=
							&a->values[1][1];
			double	d=0;
			oracleprotocolclient::decodeOjdbc14BinaryFloat(
					a->valuedata+v->offset,v->size,&d);
			charstring::printf(label,sizeof(label),
				"ojdbc14 answer reader reads row 2 column 1 "
				"of %s as NaN",what);
			report(label,d!=d);
		}
	}

	// a PL/SQL call's io vector, 0x20 for the in bind and 0x10 for the
	// out one, and the out value alone in the row behind it, a clr and
	// its indicator
	what="[0044] of 10749-dev-ojdbc14-preptz-realserver-oracle122";
	if (readRealOjdbc14Answer(&client,a,REAL_IOV_ANSWER,0,what)) {
		charstring::printf(label,sizeof(label),
				"ojdbc14 answer reader finds an io vector of an "
				"in bind and an out bind in %s",what);
		report(label,a->iov && a->iovcount==2 &&
				a->iovector[0]==0x20 && a->iovector[1]==0x10 &&
				a->iovoutcount==1 && a->rowcount==1);
		checkRealOjdbc14Value(a,what,0,0,
				"01 76 64 01 01 01 01 01 14 3c 00");
		charstring::printf(label,sizeof(label),
				"ojdbc14 answer reader finds the out value's "
				"indicator 0 and no error, sql type 47, in %s",
				what);
		report(label,!a->values[0][0].indicator && !a->retcode &&
							a->sqltype==47);
	}

	delete a;
}

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

	// test.sh starts the db2 back end's instance only where the db2
	// connection module was built, and treats it as passed otherwise
	if (listener==LISTENER_DB2 && !listening(li)) {
		stdoutput.printf("%s: skipped, nothing listens on port %d\n",
						label,(int)getPort(li));
		return;
	}

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
	s.fieldversion=li->fieldversion;
	s.listener=listener;

	uint32_t	open[8];
	uint32_t	count=0;
	const char	**tables=NULL;
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
		case SCRIPT_COLNULL:
			runColNull(&s,open,&count);
			break;
		case SCRIPT_COLBINFLOAT:
			runColBinaryFloat(&s,open,&count);
			break;
		case SCRIPT_COLNCHAR:
			runColNchar(&s,open,&count);
			break;
		case SCRIPT_COLROWID:
			runColRowid(&s,open,&count);
			break;
		case SCRIPT_COLSCALE:
			runColScale(&s,open,&count);
			break;
		case SCRIPT_COLLONG:
			runColLong(&s,open,&count);
			break;
		case SCRIPT_COLINTERVAL:
			runColInterval(&s,open,&count);
			break;
		case SCRIPT_COLTZ:
			runColTz(&s,open,&count);
			break;
		case SCRIPT_COLDB2:
			runColDb2(&s,open,&count);
			break;
		case SCRIPT_BINDNUMBER:
			runBindNumber(&s,open,&count);
			tables=BIND_TABLES_NUMBER;
			break;
		case SCRIPT_BINDDATE:
			runBindDate(&s,open,&count);
			tables=BIND_TABLES_DATE;
			break;
		case SCRIPT_BINDTZ:
			runBindTz(&s,open,&count);
			tables=BIND_TABLES_TZ;
			break;
		case SCRIPT_BINDBINFLOAT:
			runBindBinaryFloat(&s,open,&count);
			tables=BIND_TABLES_BINFLOAT;
			break;
		case SCRIPT_BINDRAW:
			runBindRaw(&s,open,&count);
			tables=BIND_TABLES_RAW;
			break;
		case SCRIPT_BINDSTRING:
			runBindString(&s,open,&count);
			tables=BIND_TABLES_STRING;
			break;
		case SCRIPT_BINDNCHAR:
			runBindNchar(&s,open,&count);
			tables=BIND_TABLES_NCHAR;
			break;
		case SCRIPT_BINDNULL:
			runBindNull(&s,open,&count);
			tables=BIND_TABLES_NULL;
			break;
		case SCRIPT_BINDROWID:
			runBindRowid(&s,open,&count);
			tables=BIND_TABLES_ROWID;
			break;
		case SCRIPT_BINDORDER:
			runBindOrder(&s,open,&count);
			tables=BIND_TABLES_ORDER;
			break;
		case SCRIPT_BINDPLSQL:
			runBindPlsql(&s,open,&count);
			tables=BIND_TABLES_PLSQL;
			break;
		case SCRIPT_BINDREEXEC:
			runBindReexec(&s,open,&count);
			tables=BIND_TABLES_REEXEC;
			break;
		case SCRIPT_BINDERRORS:
			runBindErrors(&s,open,&count);
			tables=BIND_TABLES_ERRORS;
			break;
		case SCRIPT_BINDSELECT:
			runBindSelect(&s,open,&count);
			break;
	}
	delete answer;

	// a bind session that stopped early leaves its tables behind
	if (!s.ok) {
		if (tables) {
			ojdbc14BindCleanup(li,user,tables);
		}
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
// server's layout and differ from one only where putOjdbc14Summary() says.
// the fields that follow the ceiling come from the functions the sessions
// take them from, at the ceiling that stands for each real server: 10.2 for
// oracle102, 11.2 for oracle112 and 12.1 for oracle122
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

	// ORA-00942 at a select's parse, whose sql type follows the server
	static const unsigned char	head00942of102[]={
		0x04, 0x01, 0x04, 0x00, 0x02, 0x03, 0xae, 0x00, 0x00, 0x01,
		0x02, 0x01, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28
	};
	static const unsigned char	head00942of112[]={
		0x04, 0x01, 0x04, 0x00, 0x02, 0x03, 0xae, 0x00, 0x00, 0x01,
		0x03, 0x01, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28
	};
	static const unsigned char	head00942of122[]={
		0x04, 0x01, 0x04, 0x00, 0x02, 0x03, 0xae, 0x00, 0x00, 0x01,
		0x01, 0x01, 0x0e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28
	};

	// answers to a fetch - an error, ORA-01403 and rows with no error -
	// whose success iterations follow the server
	static const unsigned char	headfetch01476of102[]={
		0x04, 0x01, 0x05, 0x01, 0x02, 0x02, 0x05, 0xc4, 0x00, 0x00,
		0x01, 0x02, 0x01, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x01, 0x24
	};
	static const unsigned char	headfetch01476of112[]={
		0x04, 0x01, 0x05, 0x01, 0x02, 0x02, 0x05, 0xc4, 0x00, 0x00,
		0x01, 0x03, 0x01, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x01, 0x24
	};
	static const unsigned char	headfetch01476of122[]={
		0x04, 0x01, 0x05, 0x01, 0x02, 0x02, 0x05, 0xc4, 0x00, 0x00,
		0x01, 0x01, 0x01, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x24
	};
	static const unsigned char	headfetch01403of112[]={
		0x04, 0x01, 0x08, 0x01, 0x01, 0x02, 0x05, 0x7b, 0x00, 0x00,
		0x01, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
		0x19
	};
	static const unsigned char	headfetch01403of122[]={
		0x04, 0x01, 0x07, 0x01, 0x01, 0x02, 0x05, 0x7b, 0x00, 0x00,
		0x01, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19
	};
	static const unsigned char	headfetchrowsof112[]={
		0x04, 0x01, 0x0b, 0x01, 0x02, 0x00, 0x00, 0x00, 0x01, 0x02,
		0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01
	};
	static const unsigned char	headfetchrowsof122[]={
		0x04, 0x01, 0x0a, 0x01, 0x02, 0x00, 0x00, 0x00, 0x01, 0x03,
		0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	// the ceilings, for the fields that follow them
	const unsigned char	at102=ORA_CCAP_FIELD_VERSION_10_2;
	const unsigned char	at112=ORA_CCAP_FIELD_VERSION_11_2;
	const unsigned char	at121=ORA_CCAP_FIELD_VERSION_12_1;

	// "rule" names the field the sessions take from the ceiling, if any,
	// and the ceiling, whose value the vector's field is
	struct {
		const char		*capture;
		const char		*rule;
		const unsigned char	*head;
		size_t			headsize;
		ojdbc14oer		e;
	} vectors[]={
		{"[0030] of 10750-dev-ojdbc14-execerr-realserver-oracle122",
			NULL,
			head00001,sizeof(head00001),
			{9,0,1,0,0,10,0,2,0xffffffff,0,
			"ORA-00001: unique constraint "
			"(TESTUSER.PROTOCOLTEST10750E_PK) violated\n",0}},
		{"[0039] of 10750-dev-ojdbc14-execerr-realserver-oracle122",
			NULL,
			head01400,sizeof(head01400),
			{14,0,1400,1,1400,5,42,2,0xffffffff,0,
			"ORA-01400: cannot insert NULL into "
			"(\"TESTUSER\".\"PROTOCOLTEST10750E\".\"NN\")\n",0}},
		{"[0029] of 10750-dev-ojdbc14-prepfetcherr-realserver-"
							"oracle122",
			NULL,
			head01476,sizeof(head01476),
			{9,2,1476,0,0,1,8,3,0,1,
			"ORA-01476: divisor is equal to zero\n",0}},
		{"[0034] of 10750-dev-ojdbc14-execselerr-realserver-"
							"oracle122",
			"12.1 fetch success iterations",
			head01722,sizeof(head01722),
			{10,0,1722,0,0,3,17,3,0,
			ojdbc14FetchSuccessIterations(at121),
			"ORA-01722: invalid number\n",0}},
		{"[0021] of 10745-dev-ojdbc14-error-realserver-oracle102",
			"10.2 sql type of ORA-00942",
			head00942of102,sizeof(head00942of102),
			{4,0,942,0,0,2,14,ojdbc14MissingTableSqlType(at102),
			0,0,MSG_00942.prefix,0}},
		{"[0021] of 10750-dev-ojdbc14-error-realserver-oracle112",
			"11.2 sql type of ORA-00942",
			head00942of112,sizeof(head00942of112),
			{4,0,942,0,0,3,14,ojdbc14MissingTableSqlType(at112),
			0,0,MSG_00942.prefix,0}},
		{"[0021] of 10745-dev-ojdbc14-error-realserver-oracle122",
			"12.1 sql type of ORA-00942",
			head00942of122,sizeof(head00942of122),
			{4,0,942,0,0,1,14,ojdbc14MissingTableSqlType(at121),
			0,0,MSG_00942.prefix,0}},
		{"[0023] of 10750-dev-ojdbc14-fetcherr-realserver-oracle102",
			"10.2 fetch success iterations",
			headfetch01476of102,sizeof(headfetch01476of102),
			{5,2,1476,0,0,2,8,3,0,
			ojdbc14FetchSuccessIterations(at102),
			MSG_01476.prefix,0}},
		{"[0023] of 10750-dev-ojdbc14-fetcherr-realserver-oracle112",
			"11.2 fetch success iterations",
			headfetch01476of112,sizeof(headfetch01476of112),
			{5,2,1476,0,0,3,8,3,0,
			ojdbc14FetchSuccessIterations(at112),
			MSG_01476.prefix,0}},
		{"[0023] of 10750-dev-ojdbc14-fetcherr-realserver-oracle122",
			"12.1 fetch success iterations",
			headfetch01476of122,sizeof(headfetch01476of122),
			{5,2,1476,0,0,1,8,3,0,
			ojdbc14FetchSuccessIterations(at121),
			MSG_01476.prefix,0}},
		{"[0027] of 10750-dev-ojdbc14-fetcherr-realserver-oracle112",
			"11.2 fetch success iterations",
			headfetch01403of112,sizeof(headfetch01403of112),
			{8,1,1403,0,0,4,0,3,0,
			ojdbc14FetchSuccessIterations(at112),
			ORA_01403,0}},
		{"[0027] of 10750-dev-ojdbc14-fetcherr-realserver-oracle122",
			"12.1 fetch success iterations",
			headfetch01403of122,sizeof(headfetch01403of122),
			{7,1,1403,0,0,3,0,3,0,
			ojdbc14FetchSuccessIterations(at121),
			ORA_01403,0}},
		{"[0031] of 10750-dev-ojdbc14-fetcherr-realserver-oracle112",
			"11.2 fetch success iterations",
			headfetchrowsof112,sizeof(headfetchrowsof112),
			{11,2,0,0,0,2,0,3,0,
			ojdbc14FetchSuccessIterations(at112),
			NULL,0}},
		{"[0031] of 10750-dev-ojdbc14-fetcherr-realserver-oracle122",
			"12.1 fetch success iterations",
			headfetchrowsof122,sizeof(headfetchrowsof122),
			{10,2,0,0,0,3,0,3,0,
			ojdbc14FetchSuccessIterations(at121),
			NULL,0}}
	};

	for (size_t i=0; i<sizeof(vectors)/sizeof(vectors[0]); i++) {

		ojdbc14oer	e=vectors[i].e;
		e.messagesize=(e.message)?charstring::getLength(e.message):0;

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
				"bytes, packet %s%s%s",(int)realsize,
				vectors[i].capture,
				(vectors[i].rule)?", with the ":"",
				(vectors[i].rule)?vectors[i].rule:"");
		bool	ok=(builtsize==realsize &&
				!bytestring::compare(built,real,realsize));
		report(label,ok);
		if (!ok) {
			dumpBytes("built",built,builtsize);
			dumpBytes("expected",real,realsize);
		}
	}
}

// ---- gatepin (#10749) ----

// What a client other than ojdbc14 gets for the column types #10749 changed
// behind use8iQueryObjects(): a hand built client at field version 11.2 gets
// an NCHAR and an NVARCHAR2 as a CHAR and a VARCHAR2 in the database
// character set, a BINARY_FLOAT and a BINARY_DOUBLE as VARCHAR2 4000 text, a
// NOT NULL column as nullable, a DATE 7 bytes wide, select null as a
// VARCHAR2 4000 with a null value, and a ROWID's numbers in the 2 and 4 byte
// forms.  The bytes are the module's own known answers, what HEAD 9b2f4bedc's
// module sent this client before #10749, not a real server's: they pin that
// no other client's answers changed.  Field version 4, the lowest whose
// select null is 4000 wide, isn't pinned: this client can't answer the 10g
// verifier a 10.2 ceiling sends.
static const char	*GATEPIN_CREATE=
	"create table protocoltest10749gp (a nchar(3), b nvarchar2(5), "
	"c binary_float, d binary_double, e number(5) not null, f date)";
static const char	*GATEPIN_INSERT=
	"insert into protocoltest10749gp values (n'ab', n'cd', 1.5, -2.25, 7, "
	"to_date('2001-02-03 04:05:06','YYYY-MM-DD HH24:MI:SS'))";
static const char	*GATEPIN_DROP=
	"begin execute immediate 'drop table protocoltest10749gp purge'; "
	"exception when others then null; end;";

// where a describe's max row size starts: behind the data flags, the ttc
// code and the prologue, a clr of 23 bytes
static const unsigned char	GATEPIN_PROLOGUE=23;
static const size_t		GATEPIN_DESCRIBE=2+1+1+GATEPIN_PROLOGUE;

// a select and the module's own answers: the describe from the max row size
// up to the date, and the rows of the first fetch from the row header up to
// the ORA-01403 that ends them, NULL for rows checked another way
struct gatepinquery {
	const char	*query;
	const char	*describe;
	const char	*rows;
};

static const gatepinquery	GATEPIN_COLUMNS={
	"select a, b, c, d, e, f from protocoltest10749gp",
	"02 1f 6d 01 06 51 60 80 00 00 01 06 00 00 00 00 02 03 69 "
	"01 01 06 01 01 01 01 01 41 00 00 00 00 01 80 00 00 01 0a "
	"00 00 00 00 02 03 69 01 01 0a 01 01 01 01 01 42 00 00 01 "
	"01 00 01 80 00 00 02 0f a0 00 00 00 00 02 03 69 01 02 0f "
	"a0 01 01 01 01 01 43 00 00 01 02 00 01 80 00 00 02 0f a0 "
	"00 00 00 00 02 03 69 01 02 0f a0 01 01 01 01 01 44 00 00 "
	"01 03 00 02 00 05 00 01 16 00 00 00 00 00 00 00 01 01 01 "
	"01 01 45 00 00 01 04 00 0c 80 00 00 01 07 00 00 00 00 02 "
	"03 69 01 01 07 01 01 01 01 01 46 00 00 01 05 00",
	"06 02 01 06 00 01 0a 00 00 00 07 03 61 62 20 02 63 64 08 "
	"31 2e 35 45 2b 30 30 30 0a 2d 32 2e 32 35 45 2b 30 30 30 "
	"02 c1 08 07 78 65 02 03 05 06 07"
};

static const gatepinquery	GATEPIN_NULL={
	"select null from dual",
	"02 0f a0 01 01 51 01 80 00 00 02 0f a0 00 00 00 00 02 03 "
	"69 01 02 0f a0 01 04 01 04 04 4e 55 4c 4c 00 00 00 00",
	"06 02 01 01 00 01 0a 00 00 00 07 00"
};

static const gatepinquery	GATEPIN_ROWID={
	"select rowid, rowidtochar(rowid) from protocoltest10749gp",
	"01 13 01 02 51 0b 00 00 00 01 01 00 00 00 00 00 00 00 01 "
	"05 01 05 05 52 4f 57 49 44 00 00 00 00 01 80 00 00 01 12 "
	"00 00 00 00 02 03 69 01 01 12 01 12 01 12 12 52 4f 57 49 "
	"44 54 4f 43 48 41 52 28 52 4f 57 49 44 29 00 00 01 01 00",
	NULL
};

// a count prefixed int at "*p" of "r", into "value", which may be NULL
static bool gatePinLpi(const unsigned char *r, size_t size, size_t *p,
							uint32_t *value) {
	if (*p>=size || r[*p]>4 || *p+1+r[*p]>size) {
		return false;
	}
	uint32_t	v=0;
	for (size_t i=0; i<r[*p]; i++) {
		v=(v<<8)|r[*p+1+i];
	}
	*p+=1+r[*p];
	if (value) {
		*value=v;
	}
	return true;
}

// "value" as the module's writeLenPreInt() writes it: 0, 1, 2 or 4 bytes
static void gatePinPutLpi(unsigned char *buffer, size_t *size,
							uint32_t value) {
	size_t	count=(!value)?0:((value<=0xff)?1:((value<=0xffff)?2:4));
	putByte(buffer,size,(unsigned char)count);
	for (size_t i=count; i>0; i--) {
		putByte(buffer,size,(unsigned char)(value>>(8*(i-1))));
	}
}

// "got" against the hex of "expected", from the start of "got"
static void gatePinBytes(const char *label, const unsigned char *got,
					size_t gotsize, const char *expected,
					unsigned char next) {
	unsigned char	bytes[1024];
	size_t		size=ojdbc14Hex(expected,bytes,sizeof(bytes));
	bool		ok=(gotsize>size && !bytestring::compare(got,bytes,size) &&
							got[size]==next);
	report(label,ok);
	if (!ok) {
		dumpBytes("got",got,gotsize);
		dumpBytes("expected",bytes,size);
	}
}

// ddl or dml on a cursor of its own, answered with no error
static void gatePinStatement(oracleprotocolclient *client, const char *label,
							const char *query,
							uint32_t options) {
	stringbuffer	l;
	l.append(label)->append(", ")->append(query)->append(" runs");
	uint32_t	cursorid=0;
	uint32_t	callstatus=0;
	uint32_t	oranum=0;
	bool		opened=client->open(&cursorid);
	bool		ok=(opened &&
				client->query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
						options,cursorid,0,query) &&
				readQuery3Summary(client,&callstatus,
							NULL,&oranum) &&
				!oranum);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s, ORA-%05d\n",client->getError(),
								(int)oranum);
	}
	if (opened) {
		client->close(cursorid);
	}
}

// a select's describe and first fetch on a cursor of its own
static void gatePinSelect(oracleprotocolclient *client, const char *label,
						const gatepinquery *q) {

	stringbuffer	l;
	uint32_t	cursorid=0;
	l.append(label)->append(", ")->append(q->query)->append(" runs");
	bool	opened=client->open(&cursorid);
	bool	ok=(opened &&
			client->query3(ORA_OPTION_PARSE|ORA_OPTION_EXECUTE|
					ORA_OPTION_NOPLSQL,cursorid,0,q->query) &&
			client->getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		if (opened) {
			client->close(cursorid);
		}
		return;
	}

	// the describe behind its 23 byte prologue, which ends in the time,
	// up to the date, whose size putDalc() sends first, as 01 07
	const unsigned char	*r=client->getResponse();
	size_t			size=client->getResponseSize();
	l.clear();
	l.append(label)->append(", ")->append(q->query);
	l.append(" describe is the module's own, HEAD 9b2f4bedc's");
	if (size>GATEPIN_DESCRIBE && r[3]==GATEPIN_PROLOGUE) {
		gatePinBytes(l.getString(),r+GATEPIN_DESCRIBE,
				size-GATEPIN_DESCRIBE,q->describe,1);
	} else {
		report(l.getString(),false);
		dumpBytes("got",r,size);
	}

	l.clear();
	l.append(label)->append(", ")->append(q->query)->append(" fetch");
	ok=(client->fetch(cursorid,10) &&
		client->getResponseTtcCode()==ORA_TTC_ROW_HEADER);
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		client->close(cursorid);
		return;
	}

	r=client->getResponse();
	size=client->getResponseSize();
	if (q->rows) {
		l.clear();
		l.append(label)->append(", ")->append(q->query);
		l.append(" rows are the module's own, HEAD 9b2f4bedc's");
		gatePinBytes(l.getString(),r+2,size-2,q->rows,ORA_TTC_ERROR);
		client->close(cursorid);
		return;
	}

	// a rowid: 0e, the object and file numbers, a zero byte, the block
	// and row numbers, each in writeLenPreInt()'s 0, 1, 2 or 4 byte
	// form, as HEAD 9b2f4bedc's module sent it, against the rowid's text
	// behind it.  where the numbers sit varies, so the text says what
	// they have to be
	size_t		p=4;		// past the row header's code and flags
	bool		walked=true;
	for (uint32_t i=0; i<6 && walked; i++) {
		walked=gatePinLpi(r,size,&p,NULL);
	}
	size_t	rowid=0;
	if (walked && p<size && r[p]==ORA_TTC_ROW_DATA) {
		rowid=++p;
		walked=(p<size && r[p++]==0x0e &&
			gatePinLpi(r,size,&p,NULL) &&
			gatePinLpi(r,size,&p,NULL) &&
			p<size && !r[p++] &&
			gatePinLpi(r,size,&p,NULL) &&
			gatePinLpi(r,size,&p,NULL) &&
			p<size && r[p]==18 && p+19<=size);
	} else {
		walked=false;
	}
	char	text[19];
	l.clear();
	l.append(label)->append(", ")->append(q->query);
	l.append(" row is a rowid and its 18 character text");
	report(l.getString(),walked);
	if (!walked) {
		dumpBytes("got",r,size);
		client->close(cursorid);
		return;
	}
	charstring::copy(text,(const char *)r+p+1,18);
	text[18]='\0';

	unsigned char	expected[32];
	size_t		expectedsize=0;
	putByte(expected,&expectedsize,0x0e);
	gatePinPutLpi(expected,&expectedsize,
				(uint32_t)ojdbc14RowidField(text,6));
	gatePinPutLpi(expected,&expectedsize,
				(uint32_t)ojdbc14RowidField(text+6,3));
	putByte(expected,&expectedsize,0);
	gatePinPutLpi(expected,&expectedsize,
				(uint32_t)ojdbc14RowidField(text+9,6));
	gatePinPutLpi(expected,&expectedsize,
				(uint32_t)ojdbc14RowidField(text+15,3));
	l.clear();
	l.append(label)->append(", ")->append(q->query);
	l.append(" rowid ")->append(text);
	l.append(" is in the module's own form, HEAD 9b2f4bedc's");
	ok=(p-rowid==expectedsize &&
		!bytestring::compare(r+rowid,expected,expectedsize));
	report(l.getString(),ok);
	if (!ok) {
		dumpBytes("got",r+rowid,p-rowid);
		dumpBytes("expected",expected,expectedsize);
	}
	client->close(cursorid);
}

// gatepin: the hand built client, field version 11.2, logs in, makes a
// table, and runs the selects
static void runGatePin(size_t listener) {

	const listenerinfo	*li=&listeners[listener];

	char	label[256];
	charstring::printf(label,sizeof(label),
			"%s, field version 11.2 client, gatepin session",
			li->name);
	stdoutput.printf("\n%s - expect the module's own answers of "
					"HEAD 9b2f4bedc\n",label);

	oracleprotocolclient	client;
	client.setConnectVersion(0);
	client.setFieldVersion(ORA_CCAP_FIELD_VERSION_11_2);
	client.setLogonTypes(LOGON_TYPES_11G);
	client.setOci1(genuineOci1(LOGON_TYPES_11G));
	client.setAnoNegotiation(true);

	stringbuffer	l;
	l.append(label)->append(" logs in");
	bool	ok=(client.connect(host,getPort(li),sid) &&
				client.login(user,password));
	report(l.getString(),ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client.getError());
		return;
	}

	gatePinStatement(&client,label,GATEPIN_DROP,0);
	gatePinStatement(&client,label,GATEPIN_CREATE,ORA_OPTION_NOPLSQL);
	gatePinStatement(&client,label,GATEPIN_INSERT,ORA_OPTION_NOPLSQL);
	gatePinSelect(&client,label,&GATEPIN_COLUMNS);
	gatePinSelect(&client,label,&GATEPIN_NULL);
	gatePinSelect(&client,label,&GATEPIN_ROWID);
	gatePinStatement(&client,label,GATEPIN_DROP,0);

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
		checkOjdbc14VersionAnswerBuilder();
		for (size_t i=0; i<sizeof(ojdbc14cases)/
					sizeof(ojdbc14cases[0]); i++) {
			runOjdbc14Case(&ojdbc14cases[i]);
		}
		runSecondPreLoginVersionCall();

		// and its queries, at every ceiling and through the
		// oracle_connectstrings auth module
		checkOjdbc14CallBuilders();
		checkOjdbc14ErrorObjectBuilder();
		checkOjdbc14AnswerReader();
		checkOjdbc14BindBuilders();
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

		// and, for #10749, the column types at the same four
		// listeners, then at the ones whose back end differs: non-
		// ascii NCHAR at the AL32UTF8 one, the describes of a 10.2
		// back end at the charset 31 one, and the rules that are
		// for an oracle back end only at the db2 one
		for (size_t i=0; i<sizeof(querylisteners)/
					sizeof(querylisteners[0]); i++) {
			for (int sc=SCRIPT_COLNULL; sc<=SCRIPT_COLTZ; sc++) {
				runOjdbc14QuerySession(querylisteners[i],
							(ojdbc14script)sc);
			}
		}
		runOjdbc14QuerySession(LISTENER_UTF8,SCRIPT_COLNCHAR);
		runOjdbc14QuerySession(LISTENER_OCI7,SCRIPT_COLNULL);
		runOjdbc14QuerySession(LISTENER_OCI7,SCRIPT_COLBINFLOAT);
		runOjdbc14QuerySession(LISTENER_OCI7,SCRIPT_COLNCHAR);
		runOjdbc14QuerySession(LISTENER_OCI7,SCRIPT_COLSCALE);
		runOjdbc14QuerySession(LISTENER_DB2,SCRIPT_COLDB2);

		// and the bind types, at the same four listeners and at the
		// charset 31 one, whose descriptors carry charset 31 and whose
		// strings' mxl is 1 byte a character, then non-ascii strings
		// at the AL32UTF8 one
		for (size_t i=0; i<sizeof(querylisteners)/
					sizeof(querylisteners[0]); i++) {
			for (int sc=SCRIPT_BINDNUMBER; sc<=SCRIPT_BINDSELECT;
									sc++) {
				runOjdbc14QuerySession(querylisteners[i],
							(ojdbc14script)sc);
			}
		}
		for (int sc=SCRIPT_BINDNUMBER; sc<=SCRIPT_BINDSELECT; sc++) {
			runOjdbc14QuerySession(LISTENER_OCI7,(ojdbc14script)sc);
		}
		runOjdbc14QuerySession(LISTENER_UTF8,SCRIPT_BINDSTRING);
		runOjdbc14QuerySession(LISTENER_UTF8,SCRIPT_BINDNCHAR);

		// and what a client other than ojdbc14 gets for the same
		// column types, at the 12.1 and 11.2 ceilings
		runGatePin(LISTENER_12_1);
		runGatePin(LISTENER_11_2);
	}

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
