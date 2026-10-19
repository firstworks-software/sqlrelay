// Copyright (c) David Muse
// See the file COPYING for more information.

// A hand-built oracle wire protocol client - the tns/tti bytes, written
// straight onto a socket, with no OCI and no thin driver in between.
//
// It exists because some paths in src/protocols/oracle.cpp can't be reached
// through a real client.  OCI answers an OCI_DESCRIBE_ONLY out of its own
// client-side cache once a statement has executed, so it never puts a
// describe-only TTI_QUERY3 on the wire for an already-executed cursor, and
// the guard in query3() that refuses to re-execute one goes untested.  This
// class puts those exact bytes on the wire instead.  It mirrors what
// test/protocol/tds/tdsdialectguard.cpp does for the analogous tds guard.
//
// This file is meant to be #include'd by a test's main(), the same way
// asserts.cpp is - see oracledescribeonly.cpp.
//
// The byte layouts here were read off two things, and both are worth
// checking against before changing anything:
//	- test/testdetails-oracleprotocol.log, which carries full hex dumps
//	  of a real OCI client's traffic against this listener, because
//	  test/sqlrelay.conf.d/oracleprotocol.conf.in sets debug="protocols"
//	- the parsers in src/protocols/oracle.cpp themselves -
//	  recvConnectRequest(), recvTtiRequest(), recvDataTypeRequest(),
//	  recvAuthenticationRequest(), open(), getQuery3Request() and
//	  fetch3()
//
// The o5logon math is the client side of src/auths/oracle_userlist.cpp's
// passwordHash(), o5logonChallenge(), o5logonComboKey() and o5logonVerify().
// Only the 11g path is implemented: the client advertises O5LOGON and not
// O7LOGON, which gets it the 11g verifier at serverversion 11.2 and 12.1 -
// see sendDataTypeNegotiation().  setLogonTypes() advertises something else,
// for a test that only asks for a challenge - see requestChallenge().
//
// The 9i verifier's o3logon answer is implemented too, as the client side of
// o3logonChallenge() and o3logonVerify() there - see
// answerO3LogonChallenge().
//
// So is the 8i verifier's single des exchange, which ojdbc14 10.2.0.5 runs:
// its version call ahead of the login, its two login calls, built around a
// computed AUTH_PASSWORD from the fields it sent a real server, and its
// logoff - see versionCall(), requestChallenge8i(), answerChallenge8i() and
// logoff8i().  So are the calls it makes once logged in, and its reader for
// their answers - see buildOjdbc14Query() and readOjdbc14Answer().

#include <rudiments/inetsocketclient.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/bytestring.h>
#include <rudiments/charstring.h>
#include <rudiments/sha1.h>
#include <rudiments/md5.h>
#include <rudiments/aes192.h>
#include <rudiments/singledes.h>
#include <rudiments/tripledes.h>
#include <rudiments/csprng.h>
#include <rudiments/stdio.h>

// ns layer packet types - the PACKET_* defines in src/protocols/oracle.cpp
static const unsigned char	ORA_PACKET_CONNECT=1;
static const unsigned char	ORA_PACKET_ACCEPT=2;
static const unsigned char	ORA_PACKET_REFUSE=4;
static const unsigned char	ORA_PACKET_DATA=6;
static const unsigned char	ORA_PACKET_RESEND=11;
static const unsigned char	ORA_PACKET_MARKER=12;
static const unsigned char	ORA_PACKET_DATA_DESCRIPTOR=15;

// marker types - the MARKER_TYPE_* defines in src/protocols/oracle.cpp
static const unsigned char	ORA_MARKER_TYPE_BREAK=1;
static const unsigned char	ORA_MARKER_TYPE_RESET=2;

// two task common codes - the TTC_* defines in src/protocols/oracle.cpp
static const unsigned char	ORA_TTC_PROTOCOL_NEGOTIATION=0x01;
static const unsigned char	ORA_TTC_DATATYPE_NEGOTIATION=0x02;
static const unsigned char	ORA_TTC_TTI_FUNCTION=0x03;
static const unsigned char	ORA_TTC_ERROR=0x04;
static const unsigned char	ORA_TTC_ROW_HEADER=0x06;
static const unsigned char	ORA_TTC_ROW_DATA=0x07;
static const unsigned char	ORA_TTC_OK=0x08;
static const unsigned char	ORA_TTC_LOB_AND_BFILE_DATA=0x0e;
static const unsigned char	ORA_TTC_DESCRIBE_INFO=0x10;

// two task interface functions - the TTI_* defines in
// src/protocols/oracle.cpp
static const unsigned char	ORA_TTI_OPEN=0x02;
static const unsigned char	ORA_TTI_QUERY=0x03;
static const unsigned char	ORA_TTI_EXECUTE=0x04;
static const unsigned char	ORA_TTI_FETCH=0x05;
static const unsigned char	ORA_TTI_REEXECUTE_AND_FETCH=0x4e;
static const unsigned char	ORA_TTI_DISCONNECT=0x09;
static const unsigned char	ORA_TTI_DESCRIBE=0x2b;
static const unsigned char	ORA_TTI_QUERY3=0x5e;
static const unsigned char	ORA_TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD=0x73;
static const unsigned char	ORA_TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY=0x76;
static const unsigned char	ORA_TTI_VERSION=0x3b;

// DATA_FLAGS_EOF in src/protocols/oracle.cpp - the data flags of the packet
// ojdbc14 sends, and nothing else, once its logoff is answered
static const uint16_t	ORA_DATA_FLAGS_EOF=0x0040;

// the tti protocol versions the module implements - TTI_VERSION_MIN and
// TTI_VERSION_MAX in src/protocols/oracle.cpp.  sendProtocolNegotiation()
// offers version 6, the highest sendTtiResponse() implements, unless
// setTtiVersion() says otherwise
static const unsigned char	ORA_TTI_VERSION_5=5;
static const unsigned char	ORA_TTI_VERSION_6=6;

// query3 options - the OPTION_* defines in src/protocols/oracle.cpp
static const uint32_t	ORA_OPTION_PARSE=(1<<0);
static const uint32_t	ORA_OPTION_BIND=(1<<3);
static const uint32_t	ORA_OPTION_DEFINE=(1<<4);
static const uint32_t	ORA_OPTION_EXECUTE=(1<<5);
static const uint32_t	ORA_OPTION_FETCH=(1<<6);
static const uint32_t	ORA_OPTION_COMMIT=(1<<8);
static const uint32_t	ORA_OPTION_EXACTFETCH=(1<<9);
static const uint32_t	ORA_OPTION_SNDIOV=(1<<10);
static const uint32_t	ORA_OPTION_NOPLSQL=(1<<15);
static const uint32_t	ORA_OPTION_DESCRIBE=(1<<17);

// reexecute()'s second options word: commit once the execute succeeds -
// python-oracledb's TNS_EXEC_OPTION_COMMIT_REEXECUTE
static const uint32_t	ORA_EXEC_OPTION_COMMIT_REEXECUTE=(1<<0);

// PROTOCOL_VERSION_12 in src/protocols/oracle.cpp.  sendAccept() switches
// to a 32-bit packet length at and above it, so a client that offers it has
// to switch too - on the packet after the accept, not on the accept itself
static const uint16_t	ORA_PROTOCOL_VERSION_12=0x013b;

// NSI_NA_NO_SERVICES in src/protocols/oracle.cpp, written into both flag
// bytes.  anoNegotiation() only runs an ano exchange for a client that set
// NSI_NA_WANTED (0x01), and this client has no use for one unless
// setAnoNegotiation() asks for it, so it says so and the handshake goes
// straight from the accept to the tti negotiation
static const uint16_t	ORA_ANO_FLAGS_NO_SERVICES=0x0808;

// the two flag bytes an OCI 23 client sends when it wants ano: NSI_NA_WANTED
// and 0x40, in both bytes.  setAnoNegotiation() sends these instead
static const uint16_t	ORA_ANO_FLAGS_WANTED=0x4141;

// the ano request an OCI 23 client sends behind the accept, the data flags
// and then the 0xdeadbeef marker, the overall size, the version, the service
// count and the four services, with the encryption and data integrity
// driver lists cut down to the "none" algorithm so the listener has
// nothing to decline
static const unsigned char	ORA_ANO_REQUEST[]={

	// data flags, marker, overall size, version, service count
	0x00,0x00,0xde,0xad,0xbe,0xef,0x00,0x85,0x00,0x00,0x00,0x00,0x00,
	0x04,0x00,

	// supervisor
	0x00,0x04,0x00,0x03,0x00,0x00,0x00,0x00,
	0x00,0x04,0x00,0x05,0x17,0x1a,0x20,0x00,
	0x00,0x08,0x00,0x01,0x00,0x0c,0xd1,0x19,0xbb,0x51,0x95,0xe1,
	0x00,0x12,0x00,0x01,0xde,0xad,0xbe,0xef,0x00,0x03,0x00,0x00,0x00,
	0x04,0x00,0x04,0x00,0x01,0x00,0x01,0x00,0x02,

	// authentication
	0x00,0x01,0x00,0x03,0x00,0x00,0x00,0x00,
	0x00,0x04,0x00,0x05,0x17,0x1a,0x20,0x00,
	0x00,0x02,0x00,0x03,0xe0,0xe1,
	0x00,0x02,0x00,0x06,0xfc,0xff,

	// encryption
	0x00,0x02,0x00,0x02,0x00,0x00,0x00,0x00,
	0x00,0x04,0x00,0x05,0x17,0x1a,0x20,0x00,
	0x00,0x01,0x00,0x01,0x00,

	// data integrity
	0x00,0x03,0x00,0x02,0x00,0x00,0x00,0x00,
	0x00,0x04,0x00,0x05,0x17,0x1a,0x20,0x00,
	0x00,0x01,0x00,0x01,0x00
};

// AL32UTF8, which is what the listener answers with whatever the client
// asks for - see the charset parameter in src/protocols/oracle.cpp
static const uint16_t	ORA_CHARSET_AL32UTF8=873;

// CCAP_LOGON_TYPES and its bits in src/protocols/oracle.cpp
static const size_t		ORA_CCAP_LOGON_TYPES=4;
static const unsigned char	ORA_CCAP_O5LOGON_NP=0x02;
static const unsigned char	ORA_CCAP_O5LOGON=0x08;
static const unsigned char	ORA_CCAP_O7LOGON=0x20;

// CCAP_FIELD_VERSION and CCAP_FIELD_VERSION_11_2 in src/protocols/oracle.cpp
static const size_t		ORA_CCAP_FIELD_VERSION=7;
static const unsigned char	ORA_CCAP_FIELD_VERSION_11_2=6;
static const size_t		ORA_CCAP_SIZE=42;
static const size_t		ORA_RCAP_SIZE=7;

// CCAP_OCI1 and CCAP_FAST_SESSION_PROPAGATE in src/protocols/oracle.cpp.  a
// real server puts the end to end field in an o3logon session's summary and
// error objects exactly when the client sets the bit - see setOci1()
static const size_t		ORA_CCAP_OCI1=16;
static const unsigned char	ORA_CCAP_FAST_SESSION_PROPAGATE=0x10;

// where the compile capabilities' size byte and the capabilities themselves
// start in a data type request, counted from behind its ttc code: past the
// two character sets and the encoding flags
static const size_t		ORA_DATA_TYPE_REQUEST_CCAP_SIZE=5;
static const size_t		ORA_DATA_TYPE_REQUEST_CCAP=6;

// RCAP_TTC and RCAP_TTC_ZERO_COPY in src/protocols/oracle.cpp.  a client
// that sets the bit, against a module that also sets it, gets a lob read's
// data in data descriptor packets rather than inline - see
// sendLobReadResponse() there
static const size_t		ORA_RCAP_TTC=6;
static const unsigned char	ORA_RCAP_TTC_ZERO_COPY=0x01;

// LOB_OP_READ, LOB_DATA_DESCRIPTOR_SIZE and LOB_DATA_LAST in
// src/protocols/oracle.cpp
static const uint32_t		ORA_LOB_OP_READ=0x00002;
static const size_t		ORA_LOB_DATA_DESCRIPTOR_SIZE=64;
static const uint32_t		ORA_LOB_DATA_LAST=3;

// CCAP_FIELD_VERSION_12_2 in src/protocols/oracle.cpp.  a bind descriptor
// grows a thirteenth field at and above it - see getQuery3BindDescriptor()
// there - so a client pinned below it, which this one is, leaves that
// field out
static const unsigned char	ORA_CCAP_FIELD_VERSION_12_2=8;

// CCAP_TTC3 and CCAP_TTC3_BIG_CHUNK_CLR in src/protocols/oracle.cpp.  a
// client that offers the bit, against a module that also offers it, gets a
// long clr's chunk lengths as count prefixed ub4s rather than raw bytes -
// see recvDataTypeRequest() there, which is where bigchunks is decided
static const size_t		ORA_CCAP_TTC3=37;
static const unsigned char	ORA_CCAP_TTC3_BIG_CHUNK_CLR=0x20;

// the clr's two forms - the CLR_* defines in src/protocols/oracle.cpp
static const size_t		ORA_CLR_MAX_SHORT_LENGTH=252;
static const unsigned char	ORA_CLR_NULL_MARKER=0xfd;
static const unsigned char	ORA_CLR_LONG_FORM_MARKER=0xfe;
static const size_t		ORA_CLR_MAX_CHUNK_SIZE=255;
static const size_t		ORA_CLR_MAX_BIG_CHUNK_SIZE=32767;

// bind and define descriptor flags - the BIND_FLAG_* defines in
// src/protocols/oracle.cpp.  USE_INDICATORS is what a descriptor with a
// real value behind it carries; UNBOUND says the placeholder was never
// bound, and getQuery3Binds() there drops the row data for the whole
// request as soon as any one descriptor sets it
static const unsigned char	ORA_BIND_FLAG_USE_INDICATORS=0x01;
static const unsigned char	ORA_BIND_FLAG_UNBOUND=0x80;

// ORACLE_TYPE_VARCHAR in src/protocols/oracle.cpp - the only type a
// character bind needs
static const unsigned char	ORA_TYPE_VARCHAR=1;

// SQLCS_IMPLICIT, a character bind's character set form.  nothing in the
// listener reads it - getQuery3BindDescriptor() consumes the byte and
// throws it away - it is here so a capture of this client's request reads
// the way a real client's does
static const unsigned char	ORA_CSFRM_IMPLICIT=1;

// how many elements go in the al8i4 vector - see appendAl8i4Vector()
static const uint32_t		ORA_AL8I4_SIZE=13;

// what an oci7 client's odescr() sends behind the cursor id and the position:
// five pointers into its own address space and two buffer sizes, 28 bytes in
// the native encoding.  describe() in src/protocols/oracle.cpp reads none of
// them, so the values are the ones packet [0027] of
// samples/9808-redhat9x86-native-realtable-parse.oraproxy carried rather than
// anything meaningful, and they go out only to prove they are discarded
static const unsigned char	ORA_DESCRIBE_TRAILING_JUNK[28]={
	0x10, 0x60, 0x05, 0x08,
	0x70, 0x2b, 0x05, 0x08,
	0x20, 0x00, 0x00, 0x00,
	0x12, 0x60, 0x05, 0x08,
	0x14, 0x60, 0x05, 0x08,
	0xc0, 0x03, 0x00, 0x00,
	0xd4, 0x63, 0x05, 0x08
};

// a pointer in the native encoding - 8 bytes, of which
// recvAuthenticationRequest() in src/protocols/oracle.cpp reads only the
// first: 0xfe is the sentinel that puts the whole session in the native
// encoding, where 0x01 leaves it portable.  the other 7 are a client-side
// address that means nothing here
static const unsigned char	ORA_NATIVE_POINTER[8]={
	0xfe, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// o5logon sizes - the 11g half of src/auths/oracle_userlist.cpp
static const size_t	ORA_SESSION_KEY_SIZE_11G=48;
static const size_t	ORA_SESSION_KEY_PAD_SIZE_11G=8;
static const size_t	ORA_PASSWORD_HASH_SIZE_11G=24;
static const size_t	ORA_COMBO_KEY_SIZE_11G=24;

// o3logon sizes and constants - the 9i half of src/auths/oracle_userlist.cpp,
// where each one is explained
static const size_t	ORA_O3LOGON_BLOCK_SIZE=8;
static const size_t	ORA_O3LOGON_HASH_SIZE=8;
static const size_t	ORA_O3LOGON_KEY_SIZE=24;
static const size_t	ORA_O3LOGON_SESSION_KEY_SIZE=16;
static const size_t	ORA_O3LOGON_PASSWORD_OVERHEAD=4;
static const unsigned char	ORA_O3LOGON_IV[]={
	0x80, 0x20, 0x40, 0x04, 0x08, 0x02, 0x10, 0x01
};
static const unsigned char	ORA_O3LOGON_SESSKEY_ENTROPY[]={
	0xa2, 0xfb, 0xe6, 0xad, 0x4c, 0x7d, 0x1e, 0x3d,
	0x6e, 0xb0, 0xb7, 0x6c, 0x97, 0xef, 0xff, 0x84,
	0x44, 0x71, 0x02, 0x84, 0xac, 0xf1, 0x3b, 0x29,
	0x5c, 0x0f, 0x0c, 0xb1, 0x87, 0x75, 0xef
};
static const unsigned char	ORA_O3LOGON_PASSWORD_ENTROPY[]={
	0xf2, 0xff, 0x97, 0x87, 0x15, 0x37, 0x07, 0x76,
	0x07, 0x27, 0xe2, 0x7f, 0xa3, 0xb1, 0xd6, 0x73,
	0x3f, 0x2f, 0xd1, 0x52, 0xab, 0xac, 0xc0
};
static const unsigned char	ORA_O3LOGON_HASH_KEY[]={
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
};
static const unsigned char	ORA_O3LOGON_HASH_IV[]={
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// the 8i exchange's sizes: an 8 byte AUTH_SESSKEY, 16 hex characters, and the
// buffer size of ojdbc14's pre-login version call
static const size_t	ORA_8I_SESSION_KEY_SIZE=8;
static const uint32_t	ORA_8I_VERSION_BUFFER_SIZE=256;

// the auth modes of ojdbc14's two login calls
static const uint32_t	ORA_8I_AUTH_MODE_PHASE_ONE=0x0001;
static const uint32_t	ORA_8I_AUTH_MODE_PHASE_TWO=0x0101;

// the AUTH_ pairs behind the raw user name in ojdbc14 10.2.0.5's phase one
// call, as it sent them in packet [0013] of samples/10545-dev-ojdbc14-login-
// select1-realserver-oracle122.oraproxy: AUTH_TERMINAL, AUTH_PROGRAM_NM,
// AUTH_MACHINE, AUTH_PID and AUTH_SID
static const uint32_t		ORA_OJDBC14_PHASE_ONE_FIELD_COUNT=5;
static const unsigned char	ORA_OJDBC14_PHASE_ONE_FIELDS[]={
	0x01, 0x0d, 0x0d, 0x41, 0x55, 0x54, 0x48, 0x5f, 0x54, 0x45, 0x52, 0x4d,
	0x49, 0x4e, 0x41, 0x4c, 0x01, 0x07, 0x07, 0x75, 0x6e, 0x6b, 0x6e, 0x6f,
	0x77, 0x6e, 0x00, 0x01, 0x0f, 0x0f, 0x41, 0x55, 0x54, 0x48, 0x5f, 0x50,
	0x52, 0x4f, 0x47, 0x52, 0x41, 0x4d, 0x5f, 0x4e, 0x4d, 0x01, 0x10, 0x10,
	0x4a, 0x44, 0x42, 0x43, 0x20, 0x54, 0x68, 0x69, 0x6e, 0x20, 0x43, 0x6c,
	0x69, 0x65, 0x6e, 0x74, 0x00, 0x01, 0x0c, 0x0c, 0x41, 0x55, 0x54, 0x48,
	0x5f, 0x4d, 0x41, 0x43, 0x48, 0x49, 0x4e, 0x45, 0x01, 0x12, 0x12, 0x64,
	0x65, 0x76, 0x2e, 0x66, 0x69, 0x72, 0x73, 0x74, 0x77, 0x6f, 0x72, 0x6b,
	0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x00, 0x01, 0x08, 0x08, 0x41, 0x55, 0x54,
	0x48, 0x5f, 0x50, 0x49, 0x44, 0x01, 0x04, 0x04, 0x31, 0x32, 0x33, 0x34,
	0x00, 0x01, 0x08, 0x08, 0x41, 0x55, 0x54, 0x48, 0x5f, 0x53, 0x49, 0x44,
	0x01, 0x05, 0x05, 0x64, 0x6d, 0x75, 0x73, 0x65, 0x00
};

// the AUTH_ pairs behind AUTH_PASSWORD in its phase two call, packet [0015]
// of the same capture: AUTH_TERMINAL with no value, AUTH_PROGRAM_NM,
// AUTH_MACHINE, AUTH_PID, AUTH_ACL, AUTH_ALTER_SESSION in 64 and 20 byte
// chunks with flags 1, and AUTH_COPYRIGHT in chunks of 64, 64, 64 and 49
// bytes, whose text holds five 0x0a bytes
static const uint32_t		ORA_OJDBC14_PHASE_TWO_FIELD_COUNT=8;
static const unsigned char	ORA_OJDBC14_PHASE_TWO_TAIL[]={
	0x01, 0x0d, 0x0d, 0x41, 0x55, 0x54, 0x48, 0x5f, 0x54, 0x45, 0x52, 0x4d,
	0x49, 0x4e, 0x41, 0x4c, 0x00, 0x00, 0x01, 0x0f, 0x0f, 0x41, 0x55, 0x54,
	0x48, 0x5f, 0x50, 0x52, 0x4f, 0x47, 0x52, 0x41, 0x4d, 0x5f, 0x4e, 0x4d,
	0x01, 0x10, 0x10, 0x4a, 0x44, 0x42, 0x43, 0x20, 0x54, 0x68, 0x69, 0x6e,
	0x20, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x00, 0x01, 0x0c, 0x0c, 0x41,
	0x55, 0x54, 0x48, 0x5f, 0x4d, 0x41, 0x43, 0x48, 0x49, 0x4e, 0x45, 0x01,
	0x12, 0x12, 0x64, 0x65, 0x76, 0x2e, 0x66, 0x69, 0x72, 0x73, 0x74, 0x77,
	0x6f, 0x72, 0x6b, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x00, 0x01, 0x08, 0x08,
	0x41, 0x55, 0x54, 0x48, 0x5f, 0x50, 0x49, 0x44, 0x01, 0x04, 0x04, 0x31,
	0x32, 0x33, 0x34, 0x00, 0x01, 0x08, 0x08, 0x41, 0x55, 0x54, 0x48, 0x5f,
	0x41, 0x43, 0x4c, 0x01, 0x04, 0x04, 0x34, 0x34, 0x30, 0x30, 0x00, 0x01,
	0x12, 0x12, 0x41, 0x55, 0x54, 0x48, 0x5f, 0x41, 0x4c, 0x54, 0x45, 0x52,
	0x5f, 0x53, 0x45, 0x53, 0x53, 0x49, 0x4f, 0x4e, 0x01, 0x54, 0xfe, 0x40,
	0x41, 0x4c, 0x54, 0x45, 0x52, 0x20, 0x53, 0x45, 0x53, 0x53, 0x49, 0x4f,
	0x4e, 0x20, 0x53, 0x45, 0x54, 0x20, 0x54, 0x49, 0x4d, 0x45, 0x5f, 0x5a,
	0x4f, 0x4e, 0x45, 0x3d, 0x27, 0x2d, 0x34, 0x3a, 0x30, 0x30, 0x27, 0x20,
	0x4e, 0x4c, 0x53, 0x5f, 0x4c, 0x41, 0x4e, 0x47, 0x55, 0x41, 0x47, 0x45,
	0x3d, 0x27, 0x41, 0x4d, 0x45, 0x52, 0x49, 0x43, 0x41, 0x4e, 0x27, 0x20,
	0x4e, 0x4c, 0x53, 0x5f, 0x14, 0x54, 0x45, 0x52, 0x52, 0x49, 0x54, 0x4f,
	0x52, 0x59, 0x3d, 0x27, 0x41, 0x4d, 0x45, 0x52, 0x49, 0x43, 0x41, 0x27,
	0x00, 0x00, 0x01, 0x01, 0x01, 0x0e, 0x0e, 0x41, 0x55, 0x54, 0x48, 0x5f,
	0x43, 0x4f, 0x50, 0x59, 0x52, 0x49, 0x47, 0x48, 0x54, 0x01, 0xf1, 0xfe,
	0x40, 0x22, 0x4f, 0x72, 0x61, 0x63, 0x6c, 0x65, 0x0a, 0x45, 0x76, 0x65,
	0x72, 0x79, 0x62, 0x6f, 0x64, 0x79, 0x20, 0x66, 0x6f, 0x6c, 0x6c, 0x6f,
	0x77, 0x73, 0x0a, 0x53, 0x70, 0x65, 0x65, 0x64, 0x79, 0x20, 0x62, 0x69,
	0x74, 0x73, 0x20, 0x65, 0x78, 0x63, 0x68, 0x61, 0x6e, 0x67, 0x65, 0x0a,
	0x53, 0x74, 0x61, 0x72, 0x73, 0x20, 0x61, 0x77, 0x61, 0x69, 0x74, 0x20,
	0x74, 0x6f, 0x20, 0x67, 0x6c, 0x40, 0x6f, 0x77, 0x22, 0x0a, 0x54, 0x68,
	0x65, 0x20, 0x70, 0x72, 0x65, 0x63, 0x65, 0x64, 0x69, 0x6e, 0x67, 0x20,
	0x6b, 0x65, 0x79, 0x20, 0x69, 0x73, 0x20, 0x63, 0x6f, 0x70, 0x79, 0x72,
	0x69, 0x67, 0x68, 0x74, 0x65, 0x64, 0x20, 0x62, 0x79, 0x20, 0x4f, 0x72,
	0x61, 0x63, 0x6c, 0x65, 0x20, 0x43, 0x6f, 0x72, 0x70, 0x6f, 0x72, 0x61,
	0x74, 0x69, 0x6f, 0x6e, 0x2e, 0x0a, 0x44, 0x75, 0x70, 0x6c, 0x40, 0x69,
	0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68,
	0x69, 0x73, 0x20, 0x6b, 0x65, 0x79, 0x20, 0x69, 0x73, 0x20, 0x6e, 0x6f,
	0x74, 0x20, 0x61, 0x6c, 0x6c, 0x6f, 0x77, 0x65, 0x64, 0x20, 0x77, 0x69,
	0x74, 0x68, 0x6f, 0x75, 0x74, 0x20, 0x70, 0x65, 0x72, 0x6d, 0x69, 0x73,
	0x73, 0x69, 0x6f, 0x6e, 0x0a, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x4f, 0x72,
	0x61, 0x63, 0x6c, 0x31, 0x65, 0x20, 0x43, 0x6f, 0x72, 0x70, 0x6f, 0x72,
	0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2e, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x72,
	0x69, 0x67, 0x68, 0x74, 0x20, 0x32, 0x30, 0x30, 0x33, 0x20, 0x4f, 0x72,
	0x61, 0x63, 0x6c, 0x65, 0x20, 0x43, 0x6f, 0x72, 0x70, 0x6f, 0x72, 0x61,
	0x74, 0x69, 0x6f, 0x6e, 0x2e, 0x00, 0x00
};

// a response is expected within this long; used on every read, so a
// listener that hangs instead of answering fails the test rather than
// hanging it
static const int32_t	ORA_RESPONSE_TIMEOUT_SEC=30;

// how long recvPacket() waits for the fragment that may follow a data packet
// that filled the sdu.  nothing on the wire says whether one is coming, so the
// read is speculative and a timeout means the message was simply that long -
// which is why this is short rather than ORA_RESPONSE_TIMEOUT_SEC
static const int32_t	ORA_CONTINUATION_TIMEOUT_SEC=2;

// what a connect packet asks for unless setSdu() says otherwise, and the
// window recvConnectRequest() in src/protocols/oracle.cpp negotiates it into -
// the smaller of the request and the module's own 4086, floored at the
// documented tns minimum of 512.  so 8192 always lands on 4086
static const uint16_t	ORA_DEFAULT_SDU=8192;
static const uint16_t	ORA_MIN_SDU=512;
static const uint16_t	ORA_MAX_SDU=4086;

// how big a message this client will reassemble out of packets.  a single
// packet can't exceed the negotiated sdu, but a message split across packets
// can be far longer, and this is what bounds it
static const size_t	ORA_MAX_PACKET_SIZE=65536;

// the two data flag bytes every data packet's body starts with, repeated on
// every fragment of a message split across packets
static const size_t	ORA_DATA_FLAGS_SIZE=2;

// the 8 byte header plus those data flags - what rides on every fragment and
// so what a fragment's own body has to fit under the sdu alongside
static const size_t	ORA_FRAGMENT_OVERHEAD=8+ORA_DATA_FLAGS_SIZE;

// how big either capability array in a tti response can be.  each is
// introduced by a single length byte - see putTtiResponse() in
// src/protocols/oracle.cpp - so 255 is the ceiling the wire itself imposes
static const size_t	ORA_MAX_CAPS_SIZE=255;

// one bind or define descriptor.  these are the twelve fields
// getQuery3BindDescriptor() in src/protocols/oracle.cpp reads, in its
// order, plus the thirteenth that only exists from field version 12.2 up.
// none of them says which placeholder the bind is for, or how many values
// follow, or which way the value travels - a bind is tied to its
// placeholder and to its value by position alone.
//
// varchar() fills the whole thing in for the common case, so a test that
// binds a string only has to say how big its buffer is; a test that needs
// some other shape starts from clear() and sets the fields itself
struct oracleprotocolbind {

	unsigned char		type;		// ORACLE_TYPE_*
	unsigned char		flags;		// ORA_BIND_FLAG_*
	unsigned char		precision;
	unsigned char		scale;
	uint32_t		buffersize;
	uint32_t		maxelements;
	uint32_t		contflags;
	uint32_t		oidlength;
	const unsigned char	*oid;		// oidlength bytes, or NULL
	uint32_t		version;
	uint32_t		charsetid;
	unsigned char		csfrm;
	uint32_t		maxdatasize;
	uint32_t		oaccolid;	// field version 12.2 and up

	void	clear();
	void	varchar(uint32_t buffersize);
};

// one bind's value for one execution iteration.  a size of 0 is how a null
// goes out: appendLenBytes() writes a lone zero length byte for it, which
// is what getLenBytes() in src/protocols/oracle.cpp reads back as a null -
// and what oracle itself makes of an empty string anyway
struct oracleprotocolbindvalue {

	const char	*value;
	size_t		size;

	void	set(const char *value);
	void	set(const char *value, size_t size);
	void	setNull();
};

// where each part of a query3 request starts, counted from the front of
// the packet - the header byte the tns length goes in is byte 0.  a test
// that sends one request as several packets needs these to say which
// structure a packet boundary fell inside; the sizes on their own only
// imply it, and getting that wrong means a test that reports coverage it
// doesn't have.
//
// a part that isn't in the request starts where the next one does, so an
// empty part is an empty range rather than a gap - a request with no
// defines has definedescriptors equal to rowdata
struct oracleprotocolquery3offsets {

	size_t	querytext;
	size_t	al8i4vector;
	size_t	binddescriptors;
	size_t	definedescriptors;
	size_t	rowdata;
	size_t	end;

	void	clear();
};

// codes and limits of ojdbc14 10.2.0.5's calls after the login, and of the
// answers to them.  ORA_8I_ names, because several test programs that
// include this file define the plain ones themselves
static const unsigned char	ORA_8I_TTC_PIGGYBACK=0x11;
static const unsigned char	ORA_8I_TTC_STATUS=0x09;
static const unsigned char	ORA_8I_TTC_BIT_VECTOR=0x15;
static const unsigned char	ORA_8I_TTI_CLOSE_CURSORS=0x69;
static const unsigned char	ORA_8I_TTI_COMMIT=0x0e;
static const unsigned char	ORA_8I_TTI_ROLLBACK=0x0f;
static const uint32_t		ORA_8I_AL8I4_SIZE=13;
static const uint32_t		ORA_8I_MAX_LONG=0x7fffffff;
static const size_t		ORA_8I_MAX_OBJECTS=96;
static const size_t		ORA_8I_MAX_COLUMNS=16;
static const size_t		ORA_8I_MAX_ROWS=32;
static const size_t		ORA_8I_MAX_NAME=64;
static const size_t		ORA_8I_MAX_VALUE_DATA=16384;
static const size_t		ORA_8I_MAX_MESSAGE=512;
static const size_t		ORA_8I_MAX_AL8O4=16;

// one ojdbc14 TTI_QUERY3 (0x5e) call.  "query" is NULL for a re-execute,
// which sends no text.  "descriptors" are the bind descriptors as ojdbc14
// wrote them, "bindcount" of them, or NULL where it doesn't resend them.
// "bindrow" is the raw 07 block of bind values, or NULL
struct oracleprotocolojdbc14query {
	uint32_t		options;
	uint32_t		cursorid;
	const char		*query;
	const uint32_t		*al8i4;
	uint32_t		bindcount;
	const unsigned char	*descriptors;
	size_t			descriptorssize;
	const unsigned char	*bindrow;
	size_t			bindrowsize;
};

// where one object of an answer is, from its ttc code on, counted from the
// front of the response, which starts with the two data flag bytes
struct oracleprotocolojdbc14object {
	unsigned char	code;
	size_t		offset;
	size_t		size;
};

// one column of a describe, as ojdbc14's T4CTTIoac and T4C8TTIuds read it,
// and where its bytes are in the response
struct oracleprotocolojdbc14column {
	size_t		offset;
	size_t		size;
	unsigned char	type;
	unsigned char	flags;
	unsigned char	precision;
	int64_t		scale;
	int64_t		maxlength;
	int64_t		maxarraylength;
	int64_t		flags2;
	int64_t		version;
	int64_t		charset;
	unsigned char	formofuse;
	int64_t		maxcharlength;
	unsigned char	nullable;
	unsigned char	namelength;
	char		name[ORA_8I_MAX_NAME+1];
	int64_t		position;
};

// one value of a row, in the answer's value data
struct oracleprotocolojdbc14value {
	size_t	offset;
	size_t	size;
	bool	isnull;
};

// an answer to one of ojdbc14's calls, as readOjdbc14Answer() found it.  each
// part is meaningful only if its ttc code is among the objects
struct oracleprotocolojdbc14answer {

	oracleprotocolojdbc14object	objects[ORA_8I_MAX_OBJECTS];
	size_t				objectcount;

	// TTC_DESCRIBE_INFO (0x10)
	size_t				prologuesize;
	int64_t				maxrowsize;
	int64_t				columncount;
	unsigned char			describeconstant;
	oracleprotocolojdbc14column	columns[ORA_8I_MAX_COLUMNS];
	size_t				describetailoffset;
	size_t				describetailsize;
	int64_t				dcbflag;
	int64_t				dcbmaxdatablocksize;

	// TTC_OK (0x08), the return parameters
	int64_t				al8o4count;
	int64_t				al8o4[ORA_8I_MAX_AL8O4];
	int64_t				al8txl;
	int64_t				paircount;

	// TTC_ROW_HEADER (0x06)
	unsigned char			rowheaderflags;
	int64_t				numrequests;
	int64_t				iterationnumber;
	int64_t				iterationsthistime;
	int64_t				uacbufferlength;
	size_t				bitvectorsize;

	// TTC_ROW_DATA (0x07), every row, a column suppressed by a bit
	// vector carrying the row above's value
	size_t				rowcount;
	oracleprotocolojdbc14value	values[ORA_8I_MAX_ROWS]
						[ORA_8I_MAX_COLUMNS];
	unsigned char			valuedata[ORA_8I_MAX_VALUE_DATA];
	size_t				valuedatasize;

	// TTC_ERROR (0x04), ojdbc14's T4CTTIoer: 23 fields, then the
	// message if retcode isn't 0
	int64_t				endtoend;
	int64_t				currentrownumber;
	int64_t				retcode;
	int64_t				arrayelementwitherror;
	int64_t				arrayelementerrno;
	int64_t				cursorid;
	int64_t				errorposition;
	unsigned char			sqltype;
	unsigned char			fatal;
	int64_t				flags;
	int64_t				usercursoroptions;
	unsigned char			upiparameter;
	unsigned char			warningflag;
	int64_t				rba;
	int64_t				partitionid;
	unsigned char			tableid;
	int64_t				blocknumber;
	int64_t				slotnumber;
	int64_t				oserror;
	unsigned char			statementnumber;
	unsigned char			callnumber;
	int64_t				pad1;
	int64_t				successiterations;
	char				message[ORA_8I_MAX_MESSAGE+1];
	size_t				messagesize;

	// TTC_STATUS (0x09), a commit's or a rollback's answer
	int64_t				statusendtoend;
};

class oracleprotocolclient {
	public:
			oracleprotocolclient();
			~oracleprotocolclient();

		// whether this client offers CCAP_TTC3_BIG_CHUNK_CLR in
		// its compile capabilities.  off by default, so a test
		// that says nothing keeps the raw-byte chunk framing every
		// test here used before the bit existed.  it has to be set
		// before connect() - the bit goes out in the data type
		// negotiation, and both ends decide there and then
		void	setBigChunkClr(bool bigchunkclr);

		// which chunk framing this client really reads and writes,
		// said outright rather than worked out from the bit it
		// offered.  without this the two move together: offering
		// the bit at tti version 6 means framing big, which is what
		// every client but one does.
		//
		// go-ora v1 is the one.  it offers the bit - byte 37 is
		// 0xb3 there - and then frames raw bytes anyway, having
		// never implemented big chunks at all.  that combination is
		// what the "bigchunks" listener attribute exists for, and
		// it can't be reached by moving setBigChunkClr() alone, so
		// a test that has to imitate go-ora v1 says
		// setBigChunkClr(true) and setBigChunkClrFraming(false).
		//
		// once set it wins over the bit and the tti version both;
		// unset, the framing follows the module's own gate.  unlike
		// the bit and the version it changes nothing on the wire by
		// itself, so it may be set at any time
		void	setBigChunkClrFraming(bool bigchunk);

		// which tti protocol version this client offers in its
		// protocol negotiation.  version 6 by default, the highest
		// the module implements.  version 5 is the other one it
		// implements, and it is the version, not the bit, that
		// decides the chunk framing there: recvDataTypeRequest()
		// only ever honors CCAP_TTC3_BIG_CHUNK_CLR at version 6 and
		// above, so a version 5 client gets the raw byte framing
		// whatever it offered.  like the bit, this has to be set
		// before connect()
		void	setTtiVersion(unsigned char ttiversion);

		// which of the two wire encodings this client speaks.
		// portable by default, which is where every real client
		// ends up - the module answers a platform banner none of
		// them match.  the native one is picked by the first
		// pointer field of the login request and nothing else (see
		// recvAuthenticationRequest() in src/protocols/oracle.cpp),
		// so it is reachable from a hand-built client that writes
		// the 0xfe sentinel there, and only from one.  in it every
		// pointer is 8 bytes and every count a fixed width little
		// endian integer, where the portable encoding writes one
		// byte and a count prefixed int.
		//
		// it has to be set before login() - the sentinel goes out in
		// phase one and both ends decide there and then
		void	setNativeEncoding(bool nativeencoding);
		bool	getNativeEncoding();

		// what this client's CCAP_LOGON_TYPES byte advertises.
		// O5LOGON alone by default, which is the one byte login()
		// can answer the challenge for.  the module picks each
		// client's verifier from this byte in phase one of the
		// login - see chooseVerifierType() in
		// src/protocols/oracle.cpp - so a test that advertises
		// anything else asks for a challenge with requestChallenge()
		// and stops there.  it has to be set before connect(), since
		// the byte goes out in the data type negotiation
		void	setLogonTypes(unsigned char logontypes);

		// what this client's CCAP_OCI1 byte advertises, 0 by default.
		// a genuine 9i client sends 0x0f or 0x07, ojdbc14 0x90 and
		// a genuine 10.2, 11.2 or OCI 23 client 0xff.  its 0x10 bit
		// decides whether an o3logon session's summary and error
		// objects carry the end to end field - see
		// chooseVerifierType() in src/protocols/oracle.cpp.  it has
		// to be set before connect(), since the byte goes out in the
		// data type negotiation
		void	setOci1(unsigned char oci1);

		// whether this client sets CCAP_OCI1's 0x10 bit, through
		// setOci1() or in the request setDataTypeRequest() gave
		bool	getFastSessionPropagate();

		// the CCAP_FIELD_VERSION byte this client advertises, 11.2
		// (6) by default.  the listener negotiates down to the lower
		// of its own and this, and the native error object and
		// trailer grow 8 bytes at 12.1 (7) and up, so a test that
		// checks them runs once at each.  it has to be set before
		// connect(), since the byte goes out in the data type
		// negotiation.  a bind descriptor's shape depends on it
		// too, so a test that sets it only logs in
		void	setFieldVersion(unsigned char fieldversion);

		// the data type negotiation's body - everything behind its
		// ttc code - sent as is in place of the one
		// sendDataTypeNegotiation() builds.  for a test that imitates
		// a client whose negotiation this class doesn't build, a
		// pre-10g one, say.  the capabilities in it, not
		// setLogonTypes() or setBigChunkClr(), are what the listener
		// sees then, and query3() and its kin still build their
		// descriptors for field version 11.2, so a test that sets
		// this builds its own calls.  the bytes aren't copied, so they
		// have to outlive connect()
		void	setDataTypeRequest(const unsigned char *request,
							size_t size);

		// the request set with setDataTypeRequest(), sent as several
		// packets instead of one: the first is "firstpacketsize" bytes
		// long, header and data flags included, and each one behind it
		// is no longer than that.  a real client splits a request at
		// the sdu it settled on or below it, and the genuine 10.2 client
		// sends its 2300 byte one (2279 behind the ttc code) as 2011 +
		// 289 bytes under an sdu of 2048, a split that is mid list and
		// short of the sdu.  0, the default, sends it in one packet
		void	setDataTypeRequestSplit(size_t firstpacketsize);

		// the sequence numbers phase one and phase two of a login
		// carry, 2 for both by default.  the module echoes each one
		// as the call number in the trailer of its answer - see
		// putAuthTrailer() and putO3LogonSummary() in
		// src/protocols/oracle.cpp
		void	setAuthSequenceNumbers(unsigned char phaseone,
						unsigned char phasetwo);

		// what this client's connect packet asks the session's
		// packets to be bounded by.  8192 by default, which
		// recvConnectRequest() in src/protocols/oracle.cpp negotiates
		// down to the 4086 the module offers; asking for less lands
		// on that instead, floored at the tns minimum of 512.  it has
		// to be set before connect() - the sdu goes out in the connect
		// packet and comes back settled in the accept.
		//
		// a session on a small sdu is how a request too big for one
		// packet is reached without a megabyte of bind values - see
		// sendSplitPacket() below
		void		setSdu(uint16_t sdu);

		// what both ends settled on, read off the accept packet
		uint16_t	getSdu();

		// the tns version this client's connect packet offers.  the
		// listener answers with the lower of it and its own, so a
		// test that needs a session below PROTOCOL_VERSION_12
		// (0x013b) offers a lower one, 0x013a say.  below
		// PROTOCOL_VERSION_12 the connect packet is the shape a
		// genuine 11.2 client sends: no 32-bit sdu and tdu, no
		// filler, and the connect descriptor at offset 58.  0, the
		// default, offers one above it.  it has to be set before
		// connect()
		void		setConnectVersion(uint16_t version);

		// the version the accept named, which is what the session
		// runs at.  0 until connect() has read the accept
		uint16_t	getAcceptVersion();

		// whether the connect packet carries the ano flags of a real
		// client, and connect() runs the ano exchange behind the
		// accept: that client's request, and the response, which has
		// to be a data packet that opens with the ano marker.  off by
		// default, which keeps the handshake going straight from the
		// accept to the tti negotiation.  it has to be set before
		// connect()
		void		setAnoNegotiation(bool anonegotiation);

		// the tns flags byte of the ano response, as the listener
		// sent it.  meaningful once connect() has run an ano exchange
		unsigned char	getAnoResponseFlags();

		// whether the data type negotiation sets RCAP_TTC_ZERO_COPY,
		// so that lobRead() gets the lob's data in data descriptor
		// packets.  off by default.  it has to be set before connect()
		void		setLobZeroCopy(bool lobzerocopy);

		// the whole handshake through the accept: the connect
		// packet (twice - the listener asks for a resend, the way a
		// real database does), then the tti protocol and data type
		// negotiations
		bool	connect(const char *host, uint16_t port,
							const char *sid);

		// the two phase o5logon exchange, 11g verifier
		bool	login(const char *user, const char *password);

		// the two halves of login().  requestChallenge() sends phase
		// one and reads the answer: the challenge, or for a refused
		// login a break marker, a reset marker and then the error.
		// it answers the markers itself, and gotBreak() says whether
		// they came.  answerChallenge() sends phase two, built from
		// the challenge still in the response, and reads the answer.
		// a phase two the listener refuses, a wrong password's, gets
		// the same break and reset first, which it answers the same
		// way: gotBreak() says they came, the error is left in the
		// response, and it returns false
		bool	requestChallenge(const char *user);
		bool	answerChallenge(const char *user, const char *password);
		bool	gotBreak();

		// a login call of a client this class doesn't build, sent as
		// is and answered the way requestChallenge() is: the markers
		// of a refusal are read and answered, gotBreak() says they
		// came, and the error or the challenge is left in the
		// response.  "request" is everything behind the two data flag
		// bytes - the ttc code, the tti function and on - and "phase"
		// names the call in an error message.  for a test that replays
		// a classic 0x52 login call or the 0x51 that answers its
		// challenge, neither of which this class builds
		bool	sendRawRequest(const unsigned char *request,
						size_t size,
						const char *phase);

		// the tns flags byte of the break marker and of the reset
		// marker a refused login got, as the listener sent them.
		// meaningful once gotBreak() says they came
		unsigned char	getBreakFlags();
		unsigned char	getResetFlags();

		// answerChallenge()'s counterpart for the 9i verifier's
		// o3logon challenge, which a client whose CCAP_LOGON_TYPES
		// lacks O5LOGON gets.  its answer is an oci7 summary object,
		// with no AUTH_ pairs, and this fails unless that summary
		// carries no error.  a phase two the listener refuses is
		// answered the way answerChallenge() answers it: gotBreak()
		// says the markers came, and the error is left in the
		// response
		bool	answerO3LogonChallenge(const char *user,
						const char *password);

		// ojdbc14 10.2.0.5's calls, for a session whose data type
		// request is ojdbc14's own, given with setDataTypeRequest().
		// every one carries the sequence number 0, as ojdbc14's do.
		//
		// versionCall() sends the bare TTI_VERSION ojdbc14 sends
		// ahead of its login, and reads the answer into the
		// response.  readVersionResponse() picks that answer apart:
		// the banner, the packed version and the end to end field
		// behind the status code.  it fails unless the answer is
		// exactly that and ends there.  "banner" is the caller's to
		// delete
		bool	versionCall();
		bool	readVersionResponse(char **banner, uint32_t *version,
							uint32_t *endtoend);

		// the 8i login: phase one carries the user name raw, with no
		// length byte, and phase two an AUTH_PASSWORD computed from
		// the 16 hex character AUTH_SESSKEY in the response, each
		// followed by the rest of the pairs ojdbc14 sent.  each reads
		// the answer and answers a refusal's markers the way
		// requestChallenge() does, and leaves the answer in the
		// response.  answerChallenge8i() returns false unless the
		// login went through.  "authpassword", if given, is sent as
		// the AUTH_PASSWORD in place of the computed one
		bool	requestChallenge8i(const char *user);
		bool	answerChallenge8i(const char *user,
						const char *password,
						const char *authpassword=NULL);

		// the AUTH_PASSWORD answerChallenge8i() sends for "user" and
		// "password" against "sesskeyhex", the listener's
		// AUTH_SESSKEY.  "authpassword" is the caller's to delete
		static bool	compute8iAuthPassword(const char *user,
						const char *password,
						const char *sesskeyhex,
						char **authpassword);

		// TTI_DISCONNECT with ojdbc14's sequence number, the answer
		// read into the response, then the eof data packet ojdbc14
		// sends behind it, and the socket closed.  "closecursors",
		// if given, are closed in front of it in the same packet,
		// as ojdbc14 closes its open statements
		bool	logoff8i(const uint32_t *closecursors=NULL,
						uint32_t closecount=0);

		// ojdbc14 10.2.0.5's calls after the login, sequence number 0
		// in every one.  each build*() leaves the request in the
		// request buffer, where getRequest() shows it, and each of
		// the others builds, sends and reads the answer into the
		// response.  "closecursors" are closed in front of the call,
		// in the same packet, the only way ojdbc14 closes a cursor:
		// 11 69 00 01, the count and the ids.  readOjdbc14Answer()
		// picks the answer apart
		void	buildOjdbc14Query(const oracleprotocolojdbc14query *q,
					const uint32_t *closecursors=NULL,
					uint32_t closecount=0);
		void	buildOjdbc14Fetch(uint32_t cursorid,
					uint32_t rowstofetch,
					const uint32_t *closecursors=NULL,
					uint32_t closecount=0);
		void	buildOjdbc14Call(unsigned char ttifunction,
					const uint32_t *closecursors=NULL,
					uint32_t closecount=0);
		bool	ojdbc14Query(const oracleprotocolojdbc14query *q,
					const uint32_t *closecursors=NULL,
					uint32_t closecount=0);
		bool	ojdbc14Fetch(uint32_t cursorid, uint32_t rowstofetch,
					const uint32_t *closecursors=NULL,
					uint32_t closecount=0);
		bool	ojdbc14Commit(const uint32_t *closecursors=NULL,
					uint32_t closecount=0);
		bool	ojdbc14Rollback(const uint32_t *closecursors=NULL,
					uint32_t closecount=0);

		// the answer in the response, walked the way ojdbc14 reads it
		// - see the definition.  "columncount" is how many columns a
		// row carries when the answer has no describe of its own.
		// "statuscall" says the call was a commit or a rollback,
		// whose answer is one TTC_STATUS or one error object.  it
		// fails, with the reason ojdbc14 would close the connection
		// for, wherever ojdbc14 would
		bool	readOjdbc14Answer(oracleprotocolojdbc14answer *answer,
						uint32_t columncount,
						bool statuscall);

		// the AUTH_ pairs in the answer to either phase, and the
		// trailer behind them.  "flags" may be NULL
		bool	getAuthFieldCount(uint32_t *count);
		bool	getAuthField(const char *name,
					char **value, uint32_t *flags);
		bool	getAuthTrailer(const unsigned char **trailer,
							size_t *size);

		// TTI_DISCONNECT, then close the socket
		void	disconnect();

		// TTI_OPEN.  "cursorid" comes back as the id the wire uses,
		// which is the listener's own id plus one
		bool	open(uint32_t *cursorid);

		// TTI_CLOSE.  "cursorid" is the wire id open() handed back
		bool	close(uint32_t cursorid);

		// TTI_QUERY3.  "query" may be NULL, which is what a call
		// that only describes or only executes an already-parsed
		// cursor sends.  this form sends no binds and no defines
		bool	query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query);

		// TTI_QUERY3 with binds.  "binds" carries one descriptor
		// per placeholder and "defines" one per define - the
		// listener reads a define's descriptor with the same code
		// and throws the result away, so 0 defines is the normal
		// case here.
		//
		// "values" is one flat array of blockcount*bindcount
		// values, block by block: the first bindcount entries are
		// the first execution iteration's, the next bindcount the
		// second's, and so on.
		//
		// "iterations" is what goes in the al8i4 vector, and it is
		// deliberately independent of blockcount - a real thin
		// client can claim 0 iterations and still send row data,
		// and telling the listener one thing while doing another
		// is the whole point of some of the tests here
		bool	query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query,
					const oracleprotocolbind *binds,
					uint32_t bindcount,
					uint32_t iterations,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount,
					const oracleprotocolbind *defines=NULL,
					uint32_t definecount=0);

		// the same request query3() sends, built into the
		// request buffer and left there.  query3() is this plus
		// sendPacket() and recvPacket(), so the bytes are the
		// same either way and only what happens to them differs.
		//
		// two things need that split apart, and neither can be
		// reached through query3():
		//
		//	- a request too big for the negotiated sdu, which
		//	  goes out through sendSplitPacket() rather than
		//	  sendPacket()
		//	- two requests written back to back before either
		//	  answer is read, which is what asks whether the
		//	  listener stops reading the first one where it
		//	  ends
		//
		// "offsets" comes back filled in - whether the request
		// is sent or not - so a test can say where a packet
		// boundary landed rather than work it out from the sizes
		bool	buildQuery3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query,
					const oracleprotocolbind *binds,
					uint32_t bindcount,
					uint32_t iterations,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount,
					const oracleprotocolbind *defines=NULL,
					uint32_t definecount=0,
					oracleprotocolquery3offsets
							*offsets=NULL);

		// TTI_EXECUTE, the modern shape: the second and later
		// executes of a statement one query3() with query text
		// already parsed.  only fresh values go out - the listener
		// remembers the descriptors - so this takes the bind count
		// and the values but no descriptors of its own.
		// "iterations" is independent of blockcount here too.
		// legacyExecute() writes the pre-query3 shape instead, and
		// which of the two the listener reads is decided by
		// query3session there, not by the call
		bool	reexecute(uint32_t cursorid, uint32_t iterations,
					uint32_t options, uint32_t moreoptions,
					uint32_t bindcount,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount);

		// TTI_REEXECUTE_AND_FETCH (0x4e) - python-oracledb's
		// TNS_FUNC_REEXECUTE_AND_FETCH.  reexecute()'s body, but
		// under this call the listener reads "iterations" as a row
		// count to prefetch in the same reply, rather than an
		// execution count - #10322
		bool	reexecuteAndFetch(uint32_t cursorid,
					uint32_t prefetchrows,
					uint32_t options, uint32_t moreoptions,
					uint32_t bindcount,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount);

		// TTI_FETCH
		bool	fetch(uint32_t cursorid, uint32_t rowstofetch);

		// a TTI_LOB_OPERATIONS read of "amount" characters or bytes,
		// from "offset" counted from 1, of the lob "locator" names,
		// in the portable encoding.  the answer is the response once
		// this returns.  with zero copy agreed, the data comes ahead
		// of it, in data descriptor packets whose count and first
		// tns flags byte are kept, and whose bytes are read past
		bool	lobRead(const unsigned char *locator,
					size_t locatorsize,
					uint32_t offset, uint32_t amount);
		uint32_t	getLobDataPacketCount();
		unsigned char	getLobDataPacketFlags();

		// TTI_DESCRIBE - an oci7 client's odescr().  "position" is
		// the column the client is asking about, counted from 1;
		// the answer carries the whole select list whatever it
		// says, and the only value it changes anything for is one
		// past the end of that list, which is an ORA-01007
		bool	describe(uint32_t cursorid, uint32_t position);

		// the pre-query3 calls: TTI_QUERY parses, TTI_EXECUTE
		// executes what it parsed, and the legacy TTI_FETCH asks
		// for the rows.  a session that only ever sends these
		// leaves query3session false in src/protocols/oracle.cpp,
		// which is what keeps fetch() on the legacy
		// sendFetchResponse() rather than handing off to fetch3().
		// "cursorid" is the wire id open() handed back, not the
		// listener's own.  none of the three carries an options
		// field the way query3()/reexecute() do - see each
		// function's own comment below for the byte layout
		bool	legacyQuery(uint32_t cursorid, const char *query);
		bool	legacyExecute(uint32_t cursorid, uint32_t iterations,
						uint32_t options);
		bool	legacyFetch(uint32_t cursorid, uint32_t rowstofetch);

		// what the last call answered with.  the ttc code is the
		// first thing in a response body, behind the two data flag
		// bytes, so it identifies the response as a whole; the
		// substring search is how a test asks whether a particular
		// column value came back, since a character column's bytes
		// go out as-is
		const unsigned char	*getResponse();
		size_t			getResponseSize();
		unsigned char		getResponseTtcCode();
		bool			responseContains(const char *text);

		// the compile capability array the listener advertised in
		// its tti protocol negotiation response, kept because that
		// response is long gone by the time a test runs a query.
		// it is the module's own compile-time array - see
		// ttiservercompilecaps and putTti6Response() in
		// src/protocols/oracle.cpp - and not an answer to anything
		// this client offered: the arrays go out before the client's
		// arrive, so there is nothing to answer yet.
		//
		// a test that wants a particular capability indexes it by
		// the module's own CCAP_* index, and has to check the size
		// first: a version 5 session gets no arrays at all, and the
		// size is 0 there
		const unsigned char	*getServerCompileCaps();
		size_t			getServerCompileCapsSize();

		// what went wrong, for a method that returned false
		const char	*getError();

		// the building blocks the calls above are made of, public so
		// a test that needs a call this class doesn't implement -
		// the legacy TTI_QUERY/TTI_EXECUTE path, say - can build one
		// without reimplementing the handshake
		void	beginPacket(unsigned char packettype);
		void	beginTtiCall(unsigned char ttifunction);
		void	appendByte(unsigned char value);
		void	appendBytes(const unsigned char *value, size_t size);
		void	appendBE16(uint16_t value);
		void	appendBE32(uint32_t value);
		void	appendLE16(uint16_t value);
		void	appendLenPreInt(uint32_t value);

		// the two encoding-aware primitives - a count, and the
		// pointer field a count often sits behind.  "nativesize" is
		// how many bytes the count takes in the native encoding,
		// which varies field by field; the portable encoding writes
		// a count prefixed int whatever it says.  the mirrors of
		// putAuthCount()/getAuthCount() and getAuthPointer() in
		// src/protocols/oracle.cpp
		void	appendAuthCount(uint32_t value, unsigned char nativesize);
		void	appendAuthPointer();
		bool	readAuthCount(uint32_t *value, unsigned char nativesize);

		void	appendLenString(const char *value, size_t size);
		void	appendLenBytes(const char *value, size_t size);
		void	appendAl8i4Vector(uint32_t iterations);
		void	appendBindDescriptor(const oracleprotocolbind *bind);
		void	appendRowDataBlocks(
					const oracleprotocolbindvalue *values,
					uint32_t bindcount,
					uint32_t blockcount);
		bool	sendPacket();

		// the same request, written out as several ordinary data
		// packets none of which exceeds the negotiated sdu - the
		// mirror of sqlrprotocol_oracle::sendSplitPacket() in
		// src/protocols/oracle.cpp, in the other direction.  every
		// fragment is a complete packet with the same type and flags
		// and repeats the two data flag bytes; nothing marks one as a
		// continuation and nothing marks the last, so a fragment may
		// end at any byte offset.
		//
		// this is what a real client's tns layer does for a request
		// its ttc layer built too big for one packet, and a test that
		// needs the listener to reassemble one calls it in place of
		// sendPacket()
		bool	sendSplitPacket();

		// how big the request built so far is, header and data flags
		// included - what sendSplitPacket() is about to divide up
		size_t	getRequestSize();

		// the request built so far, from its header placeholder on
		const unsigned char	*getRequest();

		bool	recvPacket();
		unsigned char	getPacketType();

		// response walking, in the same spirit - a test that has to
		// pick a response apart field by field, rather than just
		// search it for a value, needs these
		void	rewindResponse();
		bool	readByte(unsigned char *value);
		bool	readBytes(unsigned char *value, size_t size);
		bool	readLenPreInt(uint32_t *value);
		bool	readLenString(char **value);
		bool	readLenBytes(unsigned char *value, size_t maxsize,
						size_t *size, bool *isnull);

	private:
		void	setError(const char *message);
		void	setError(const char *message, const char *detail);

		bool	bigChunkClrFraming();

		// reads one packet and appends its body to the response.
		// "continuation" says the packet carries the rest of a message
		// already partly read, so the data flags it repeats are read
		// off and dropped rather than kept, and a header that never
		// arrives means the message ended rather than that the read
		// failed - "gotone" is how that comes back.  "packetsize" is
		// the whole packet's declared size, which is what recvPacket()
		// reads the "another fragment may follow" signal off
		bool	recvFragment(bool continuation, int32_t timeoutsec,
					bool *gotone, size_t *packetsize);

		bool	sendConnect(const char *sid);
		bool	sendProtocolNegotiation();
		bool	parseTtiResponse();
		bool	sendDataTypeNegotiation();
		bool	runAnoNegotiation();

		// sendSplitPacket() with the size of the biggest packet given
		// rather than taken from the negotiated sdu
		bool	sendSplitPacketAt(size_t maxpacketsize);

		// sends the login call built so far and reads the answer,
		// markers included
		bool	sendLoginCall(const char *phase);
		void	sendAuthRequest(unsigned char ttifunction,
					unsigned char seqnumber,
					const char *user,
					uint32_t authmode,
					uint32_t fieldcount);
		void	sendAuthRequest8i(unsigned char ttifunction,
					const char *user,
					uint32_t authmode,
					uint32_t fieldcount);
		void	appendAuthField(const char *name,
					const char *value,
					uint32_t flags);
		bool	walkAuthFields(const char *name,
					char **value, uint32_t *flags,
					uint32_t *count);
		bool	sendMarker(unsigned char markertype);
		bool	recvAfterBreak(const char *phase);

		// ojdbc14's own readers - see readOjdbc14Answer()
		void	beginOjdbc14Call(unsigned char ttifunction,
					const uint32_t *closecursors,
					uint32_t closecount);
		bool	readOjdbc14Number(int64_t *value,
					unsigned char maxlength,
					const char *field);
		bool	readOjdbc14Clr(unsigned char *value, size_t maxsize,
					size_t *size, bool *isnull,
					const char *field);
		bool	readOjdbc14Dalc(unsigned char *value, size_t maxsize,
					size_t *size, const char *field);
		bool	readOjdbc14Describe(oracleprotocolojdbc14answer *a);
		bool	readOjdbc14Summary(oracleprotocolojdbc14answer *a);

		inetsocketclient	sock;
		bool			connected;
		bool			largeheader;
		bool			bigchunkclr;
		bool			nativeencoding;
		unsigned char		logontypes;
		unsigned char		oci1;
		const unsigned char	*datatyperequest;
		size_t			datatyperequestsize;
		size_t			datatyperequestsplit;
		unsigned char		authseqphaseone;
		unsigned char		authseqphasetwo;
		bool			gotbreak;
		unsigned char		breakflags;
		unsigned char		resetflags;

		// what the connect packet asks for, and what the accept
		// answered with
		uint16_t		offeredsdu;
		uint16_t		negotiatedsdu;

		// the version the connect packet offers, 0 for the default,
		// and the one the accept answered with
		uint16_t		connectversion;
		uint16_t		acceptversion;

		// whether to run an ano exchange, and the flags byte of the
		// response
		bool			anonegotiation;
		unsigned char		anoresponseflags;

		// whether to offer zero copy lob reads, and what the last
		// lobRead() got in data descriptor packets
		bool			lobzerocopy;
		uint32_t		lobdatapacketcount;
		unsigned char		lobdatapacketflags;

		// setBigChunkClrFraming()'s answer, and whether it was ever
		// given.  two members rather than one tri-state so the
		// unset case stays the default the other tests already get
		bool			bigchunkclrframingset;
		bool			bigchunkclrframing;

		unsigned char		ttiversion;

		// what connect() kept off the tti protocol negotiation
		// response
		unsigned char		servercompilecaps[ORA_MAX_CAPS_SIZE];
		size_t			servercompilecapssize;

		// what this client offers, and so - since the listener
		// negotiates down to the lower of the two ends - what it
		// gets.  a bind descriptor's shape depends on it
		unsigned char		fieldversion;

		bytebuffer	reqpacket;
		unsigned char	reqpackettype;

		unsigned char	*resppacket;
		size_t		respsize;
		unsigned char	resppackettype;
		unsigned char	resppacketflags;
		size_t		respposition;

		char		errormessage[1024];
};

// aes-cbc with a zero iv and no padding, exactly as
// src/auths/oracle_userlist.cpp's aesCbc() does it.  the 11g verifier only
// ever uses a 24-byte key, so this is aes192 and nothing else
static bool oracleAes192Cbc(bool encrypt,
				const unsigned char *key,
				const unsigned char *in, size_t insize,
				unsigned char *out) {

	if (insize%16) {
		return false;
	}

	unsigned char	iv[16];
	bytestring::zero(iv,sizeof(iv));

	aes192	enc;
	enc.setUsePadding(false);
	if (!enc.setKey(key,24) || !enc.setIv(iv,sizeof(iv)) ||
					!enc.append(in,(uint32_t)insize)) {
		return false;
	}

	const unsigned char	*result=(encrypt)?
					enc.getEncryptedData():
					enc.getDecryptedData();
	uint64_t		resultsize=(encrypt)?
					enc.getEncryptedDataSize():
					enc.getDecryptedDataSize();
	if (!result || resultsize!=insize) {
		return false;
	}
	bytestring::copy(out,result,insize);
	return true;
}

// uppercase hex, which is what every AUTH_ field on the wire is in
static char *oracleHexEncodeUpper(const unsigned char *in, size_t insize) {
	char	*hex=charstring::hexEncode(in,insize);
	charstring::upper(hex);
	return hex;
}

// single des cbc for an 8 byte key and triple des cbc for a 24 byte one, with
// no padding, exactly as src/auths/oracle_userlist.cpp's desCbc() does it
static bool oracleDesCbc(bool encrypt,
				const unsigned char *key, size_t keysize,
				const unsigned char *iv,
				const unsigned char *in, size_t insize,
				unsigned char *out) {

	if (!insize || insize%ORA_O3LOGON_BLOCK_SIZE ||
		(keysize!=ORA_O3LOGON_HASH_SIZE &&
			keysize!=ORA_O3LOGON_KEY_SIZE)) {
		return false;
	}

	encryption	*enc=(keysize==ORA_O3LOGON_HASH_SIZE)?
				(encryption *)new singledes():
				(encryption *)new tripledes();
	enc->setUsePadding(false);

	bool	ok=false;
	if (enc->setKey(key,keysize) &&
		enc->setIv(iv,ORA_O3LOGON_BLOCK_SIZE) &&
		enc->append(in,(uint32_t)insize)) {

		// each size getter re-runs the cipher in its own direction, so
		// it has to be paired with the matching data getter
		const unsigned char	*result=(encrypt)?
						enc->getEncryptedData():
						enc->getDecryptedData();
		uint64_t		resultsize=(encrypt)?
						enc->getEncryptedDataSize():
						enc->getDecryptedDataSize();
		if (result && resultsize==insize) {
			bytestring::copy(out,result,insize);
			ok=true;
		}
	}

	delete enc;
	return ok;
}

// the oracle des password hash: uppercase(user||password) as utf-16be, zero
// padded to a block multiple, des-cbc encrypted under a fixed key, then again
// under the last block of that, whose own last block is the hash.  see
// o3logonPasswordHash() in src/auths/oracle_userlist.cpp
static bool oracleO3LogonPasswordHash(const char *user, const char *password,
							unsigned char *hash) {

	char	*upper=NULL;
	charstring::printf(&upper,"%s%s",user,password);
	charstring::upper(upper);

	size_t	upperlen=charstring::getLength(upper);
	size_t	insize=((upperlen*2)+ORA_O3LOGON_BLOCK_SIZE-1)/
				ORA_O3LOGON_BLOCK_SIZE*ORA_O3LOGON_BLOCK_SIZE;
	if (!insize) {
		insize=ORA_O3LOGON_BLOCK_SIZE;
	}

	unsigned char	*in=new unsigned char[insize];
	unsigned char	*out=new unsigned char[insize];
	bytestring::zero(in,insize);
	for (size_t i=0; i<upperlen; i++) {
		in[i*2+1]=(unsigned char)upper[i];
	}

	unsigned char	key[ORA_O3LOGON_HASH_SIZE];
	bool	ok=oracleDesCbc(true,ORA_O3LOGON_HASH_KEY,
					sizeof(ORA_O3LOGON_HASH_KEY),
					ORA_O3LOGON_HASH_IV,in,insize,out);
	if (ok) {
		bytestring::copy(key,out+insize-ORA_O3LOGON_HASH_SIZE,
							sizeof(key));
		ok=oracleDesCbc(true,key,sizeof(key),
					ORA_O3LOGON_HASH_IV,in,insize,out);
	}
	if (ok) {
		bytestring::copy(hash,out+insize-ORA_O3LOGON_HASH_SIZE,
							ORA_O3LOGON_HASH_SIZE);
	}

	delete[] in;
	delete[] out;
	delete[] upper;
	return ok;
}

// the o3logon key derivation: sha1(in||entropy), then the front of
// sha1(in||0x02||all but the first byte of that||entropy), 24 bytes in all.
// see o3logonCreateKey() in src/auths/oracle_userlist.cpp
static bool oracleO3LogonCreateKey(const unsigned char *in, size_t insize,
					const unsigned char *entropy,
					size_t entropysize,
					unsigned char *key) {

	sha1	first;
	if (!first.append(in,(uint32_t)insize) ||
		!first.append(entropy,(uint32_t)entropysize)) {
		return false;
	}
	const unsigned char	*digest=first.getHash();
	if (!digest) {
		return false;
	}
	bytestring::copy(key,digest,20);

	sha1		second;
	unsigned char	two=0x02;
	if (!second.append(in,(uint32_t)insize) ||
		!second.append(&two,1) ||
		!second.append(key+1,19) ||
		!second.append(entropy,(uint32_t)entropysize)) {
		return false;
	}
	digest=second.getHash();
	if (!digest) {
		return false;
	}
	bytestring::copy(key+20,digest,4);
	return true;
}

oracleprotocolclient::oracleprotocolclient() {
	connected=false;
	largeheader=false;
	bigchunkclr=false;
	nativeencoding=false;
	logontypes=ORA_CCAP_O5LOGON;
	oci1=0;
	datatyperequest=NULL;
	datatyperequestsize=0;
	datatyperequestsplit=0;
	authseqphaseone=2;
	authseqphasetwo=2;
	gotbreak=false;
	breakflags=0;
	resetflags=0;
	connectversion=0;
	acceptversion=0;
	anonegotiation=false;
	anoresponseflags=0;
	lobzerocopy=false;
	lobdatapacketcount=0;
	lobdatapacketflags=0;
	bigchunkclrframingset=false;
	bigchunkclrframing=false;
	offeredsdu=ORA_DEFAULT_SDU;
	negotiatedsdu=0;
	ttiversion=ORA_TTI_VERSION_6;
	bytestring::zero(servercompilecaps,sizeof(servercompilecaps));
	servercompilecapssize=0;
	fieldversion=ORA_CCAP_FIELD_VERSION_11_2;
	reqpackettype=0;
	resppacket=new unsigned char[ORA_MAX_PACKET_SIZE];
	respsize=0;
	resppackettype=0;
	resppacketflags=0;
	respposition=0;
	errormessage[0]='\0';
}

oracleprotocolclient::~oracleprotocolclient() {
	if (connected) {
		sock.close();
	}
	delete[] resppacket;
}

void oracleprotocolclient::setError(const char *message) {
	charstring::copy(errormessage,message,sizeof(errormessage)-1);
	errormessage[sizeof(errormessage)-1]='\0';
}

void oracleprotocolclient::setError(const char *message, const char *detail) {
	charstring::printf(errormessage,sizeof(errormessage),
				"%s: %s",message,detail);
}

const char *oracleprotocolclient::getError() {
	return errormessage;
}

void oracleprotocolclient::setBigChunkClr(bool bigchunkclr) {
	this->bigchunkclr=bigchunkclr;
}

void oracleprotocolclient::setBigChunkClrFraming(bool bigchunk) {
	bigchunkclrframingset=true;
	bigchunkclrframing=bigchunk;
}

void oracleprotocolclient::setTtiVersion(unsigned char ttiversion) {
	this->ttiversion=ttiversion;
}

void oracleprotocolclient::setNativeEncoding(bool nativeencoding) {
	this->nativeencoding=nativeencoding;
}

bool oracleprotocolclient::getNativeEncoding() {
	return nativeencoding;
}

void oracleprotocolclient::setLogonTypes(unsigned char logontypes) {
	this->logontypes=logontypes;
}

void oracleprotocolclient::setOci1(unsigned char oci1) {
	this->oci1=oci1;
}

bool oracleprotocolclient::getFastSessionPropagate() {
	if (!datatyperequest) {
		return (oci1&ORA_CCAP_FAST_SESSION_PROPAGATE)!=0;
	}
	return (datatyperequestsize>ORA_DATA_TYPE_REQUEST_CCAP+ORA_CCAP_OCI1 &&
		datatyperequest[ORA_DATA_TYPE_REQUEST_CCAP_SIZE]>
							ORA_CCAP_OCI1 &&
		(datatyperequest[ORA_DATA_TYPE_REQUEST_CCAP+ORA_CCAP_OCI1]&
					ORA_CCAP_FAST_SESSION_PROPAGATE)!=0);
}

void oracleprotocolclient::setFieldVersion(unsigned char fieldversion) {
	this->fieldversion=fieldversion;
}

void oracleprotocolclient::setDataTypeRequest(const unsigned char *request,
								size_t size) {
	datatyperequest=request;
	datatyperequestsize=size;
}

void oracleprotocolclient::setDataTypeRequestSplit(size_t firstpacketsize) {
	datatyperequestsplit=firstpacketsize;
}

void oracleprotocolclient::setAuthSequenceNumbers(unsigned char phaseone,
						unsigned char phasetwo) {
	authseqphaseone=phaseone;
	authseqphasetwo=phasetwo;
}

void oracleprotocolclient::setSdu(uint16_t sdu) {
	offeredsdu=sdu;
}

uint16_t oracleprotocolclient::getSdu() {
	return negotiatedsdu;
}

void oracleprotocolclient::setConnectVersion(uint16_t version) {
	connectversion=version;
}

uint16_t oracleprotocolclient::getAcceptVersion() {
	return acceptversion;
}

void oracleprotocolclient::setAnoNegotiation(bool anonegotiation) {
	this->anonegotiation=anonegotiation;
}

unsigned char oracleprotocolclient::getAnoResponseFlags() {
	return anoresponseflags;
}

void oracleprotocolclient::setLobZeroCopy(bool lobzerocopy) {
	this->lobzerocopy=lobzerocopy;
}

uint32_t oracleprotocolclient::getLobDataPacketCount() {
	return lobdatapacketcount;
}

unsigned char oracleprotocolclient::getLobDataPacketFlags() {
	return lobdatapacketflags;
}

// which framing this client has to read and write - which is not the same
// question as which bit it offers.  recvDataTypeRequest() in
// src/protocols/oracle.cpp gates the big chunk framing on the tti version as
// well as on both ends' bits:
//
//	bigchunks=(ttiversion>=6 && ...)
//
// so a client that offers the bit at version 5 is answered in raw bytes
// anyway, and has to frame its own requests in raw bytes to be understood.
// this mirrors that gate, and it is only the plumbing - what the test
// asserts is the bytes the module actually wrote, which it builds itself.
//
// setBigChunkClrFraming() overrides the whole gate, because one real client
// disagrees with it: go-ora v1 offers the bit and frames raw bytes anyway.
// the "bigchunks" listener attribute is what answers that, and a test of
// it has to be able to say the two things separately
bool oracleprotocolclient::bigChunkClrFraming() {
	if (bigchunkclrframingset) {
		return bigchunkclrframing;
	}
	return (bigchunkclr && ttiversion>=ORA_TTI_VERSION_6);
}

const unsigned char *oracleprotocolclient::getServerCompileCaps() {
	return servercompilecaps;
}

size_t oracleprotocolclient::getServerCompileCapsSize() {
	return servercompilecapssize;
}

const unsigned char *oracleprotocolclient::getResponse() {
	return resppacket;
}

size_t oracleprotocolclient::getResponseSize() {
	return respsize;
}

unsigned char oracleprotocolclient::getPacketType() {
	return resppackettype;
}

// the first byte behind the two data flag bytes every data packet starts
// with
unsigned char oracleprotocolclient::getResponseTtcCode() {
	return (respsize>=3)?resppacket[2]:0;
}

bool oracleprotocolclient::responseContains(const char *text) {
	size_t	textsize=charstring::getLength(text);
	if (!textsize || respsize<textsize) {
		return false;
	}
	for (size_t i=0; i<=respsize-textsize; i++) {
		if (!bytestring::compare(resppacket+i,text,textsize)) {
			return true;
		}
	}
	return false;
}


// ---- packet building and framing ----

// 8 placeholder bytes for the header, which sendPacket() overwrites once
// the body's size is known - the same thing
// sqlrprotocol_oracle::resetSendPacketBuffer() does
void oracleprotocolclient::beginPacket(unsigned char packettype) {
	reqpacket.clear();
	unsigned char	header[8];
	bytestring::zero(header,sizeof(header));
	reqpacket.append(header,sizeof(header));
	reqpackettype=packettype;
}

// a data packet carrying one tti call: the data flags, then the ttc code
// that says a tti function follows, then the function itself
void oracleprotocolclient::beginTtiCall(unsigned char ttifunction) {
	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendByte(ORA_TTC_TTI_FUNCTION);
	appendByte(ttifunction);
}

void oracleprotocolclient::appendByte(unsigned char value) {
	reqpacket.append(value);
}

void oracleprotocolclient::appendBytes(const unsigned char *value,
							size_t size) {
	reqpacket.append(value,size);
}

void oracleprotocolclient::appendBE16(uint16_t value) {
	reqpacket.append((unsigned char)((value>>8)&0xff));
	reqpacket.append((unsigned char)(value&0xff));
}

void oracleprotocolclient::appendBE32(uint32_t value) {
	reqpacket.append((unsigned char)((value>>24)&0xff));
	reqpacket.append((unsigned char)((value>>16)&0xff));
	reqpacket.append((unsigned char)((value>>8)&0xff));
	reqpacket.append((unsigned char)(value&0xff));
}

// the one field on the legacy query path that isn't big-endian: query()
// in src/protocols/oracle.cpp reads its query size with readLE()
void oracleprotocolclient::appendLE16(uint16_t value) {
	reqpacket.append((unsigned char)(value&0xff));
	reqpacket.append((unsigned char)((value>>8)&0xff));
}

// a count byte, then that many big-endian bytes - the mirror of
// sqlrprotocol::writeLenPreInt()/readLenPreInt() in
// src/server/sqlrprotocol.cpp
void oracleprotocolclient::appendLenPreInt(uint32_t value) {
	if (!value) {
		appendByte(0);
	} else if (value<=0xff) {
		appendByte(1);
		appendByte((unsigned char)value);
	} else if (value<=0xffff) {
		appendByte(2);
		appendBE16((uint16_t)value);
	} else {
		appendByte(4);
		appendBE32(value);
	}
}

// a count, in whichever encoding this session settled on: a count prefixed
// int in the portable one, and "nativesize" little-endian bytes in the
// native one, of which only the low four carry the value.  the mirror of
// putAuthCount() in src/protocols/oracle.cpp
void oracleprotocolclient::appendAuthCount(uint32_t value,
						unsigned char nativesize) {

	if (!nativeencoding) {
		appendLenPreInt(value);
		return;
	}

	for (unsigned char i=0; i<nativesize; i++) {
		appendByte((unsigned char)((i<sizeof(uint32_t))?
					((value>>(8*i))&0xff):0));
	}
}

// the pointer a count often sits behind.  the module reads past it without
// looking at the value, save for the first one in a login request, whose
// first byte is what picks the encoding for the whole session
void oracleprotocolclient::appendAuthPointer() {

	if (!nativeencoding) {
		appendByte(1);
		return;
	}

	appendBytes(ORA_NATIVE_POINTER,sizeof(ORA_NATIVE_POINTER));
}

bool oracleprotocolclient::readAuthCount(uint32_t *value,
						unsigned char nativesize) {

	if (!nativeencoding) {
		return readLenPreInt(value);
	}

	if ((size_t)nativesize>respsize-respposition) {
		return false;
	}

	*value=0;
	for (unsigned char i=0; i<nativesize; i++) {
		unsigned char	b=0;
		readByte(&b);
		if (i<sizeof(uint32_t)) {
			*value|=((uint32_t)b)<<(8*i);
		}
	}
	return true;
}

// a text - one length byte, then that many bytes.  there is no long form; a
// value past 252 bytes takes a clr instead.  the mirror of putLenString() in
// src/protocols/oracle.cpp
void oracleprotocolclient::appendLenString(const char *value, size_t size) {
	appendByte((unsigned char)size);
	reqpacket.append(value,size);
}

// a clr - the text-shaped short form up to 252 bytes, the chunked long form
// above that.  the mirror of putLenBytes() in src/protocols/oracle.cpp, and
// the exact shape getLenBytes() and getQuery3Request() read back there:
// a 0xfe marker, then a run of chunks, then an empty chunk to close it.
//
// a chunk's length goes out as a raw byte, capped at 255, unless this client
// negotiated CCAP_TTC3_BIG_CHUNK_CLR - see bigChunkClrFraming() - then it
// goes out as a count prefixed ub4, capped at 32767.  the closing chunk is
// one zero byte either way, since a ub4 zero is a count byte of 0 and
// nothing after it
void oracleprotocolclient::appendLenBytes(const char *value, size_t size) {

	if (size<=ORA_CLR_MAX_SHORT_LENGTH) {
		appendByte((unsigned char)size);
		if (size) {
			reqpacket.append(value,size);
		}
		return;
	}

	appendByte(ORA_CLR_LONG_FORM_MARKER);
	bool	bigchunk=bigChunkClrFraming();
	size_t	maxchunk=(bigchunk)?
			ORA_CLR_MAX_BIG_CHUNK_SIZE:ORA_CLR_MAX_CHUNK_SIZE;
	size_t	offset=0;
	while (offset<size) {
		size_t	chunk=size-offset;
		if (chunk>maxchunk) {
			chunk=maxchunk;
		}
		if (bigchunk) {
			appendLenPreInt((uint32_t)chunk);
		} else {
			appendByte((unsigned char)chunk);
		}
		reqpacket.append(value+offset,chunk);
		offset+=chunk;
	}
	appendByte(0);
}

bool oracleprotocolclient::sendPacket() {

	size_t		packetsize=reqpacket.getSize();
	unsigned char	*buffer=(unsigned char *)reqpacket.getBuffer();

	// a 32-bit length at PROTOCOL_VERSION_12 and above, a 16-bit length
	// and a zero packet checksum below it - see
	// sqlrprotocol_oracle::sendPacket()
	if (largeheader) {
		buffer[0]=(unsigned char)((packetsize>>24)&0xff);
		buffer[1]=(unsigned char)((packetsize>>16)&0xff);
		buffer[2]=(unsigned char)((packetsize>>8)&0xff);
		buffer[3]=(unsigned char)(packetsize&0xff);
	} else {
		buffer[0]=(unsigned char)((packetsize>>8)&0xff);
		buffer[1]=(unsigned char)(packetsize&0xff);
		buffer[2]=0;
		buffer[3]=0;
	}
	buffer[4]=reqpackettype;
	buffer[5]=0;		// packet flags
	buffer[6]=0;		// header checksum
	buffer[7]=0;

	if (sock.write(buffer,packetsize)!=(ssize_t)packetsize) {
		setError("failed to write packet");
		return false;
	}
	sock.flushWriteBuffer(-1,-1);
	return true;
}

size_t oracleprotocolclient::getRequestSize() {
	return reqpacket.getSize();
}

const unsigned char *oracleprotocolclient::getRequest() {
	return (const unsigned char *)reqpacket.getBuffer();
}

// the mirror of sqlrprotocol_oracle::sendSplitPacket() - see the declaration
// above.  the header and the data flags are written from locals rather than
// from the request buffer, so the buffer itself is left alone and the caller
// can still read the bytes it built
bool oracleprotocolclient::sendSplitPacket() {
	return sendSplitPacketAt((negotiatedsdu)?
				(size_t)negotiatedsdu:(size_t)ORA_MAX_SDU);
}

bool oracleprotocolclient::sendSplitPacketAt(size_t maxpacketsize) {

	if (reqpacket.getSize()<=ORA_FRAGMENT_OVERHEAD ||
				maxpacketsize<=ORA_FRAGMENT_OVERHEAD) {
		setError("nothing to split");
		return false;
	}

	unsigned char	*buffer=(unsigned char *)reqpacket.getBuffer();
	unsigned char	*dataflags=buffer+8;
	unsigned char	*body=buffer+ORA_FRAGMENT_OVERHEAD;
	size_t		remaining=reqpacket.getSize()-ORA_FRAGMENT_OVERHEAD;
	size_t		maxbodysize=maxpacketsize-ORA_FRAGMENT_OVERHEAD;

	do {

		size_t	bodysize=(remaining>maxbodysize)?maxbodysize:remaining;
		remaining=remaining-bodysize;

		size_t		packetsize=bodysize+ORA_FRAGMENT_OVERHEAD;
		unsigned char	header[8];
		if (largeheader) {
			header[0]=(unsigned char)((packetsize>>24)&0xff);
			header[1]=(unsigned char)((packetsize>>16)&0xff);
			header[2]=(unsigned char)((packetsize>>8)&0xff);
			header[3]=(unsigned char)(packetsize&0xff);
		} else {
			header[0]=(unsigned char)((packetsize>>8)&0xff);
			header[1]=(unsigned char)(packetsize&0xff);
			header[2]=0;		// packet checksum
			header[3]=0;
		}
		header[4]=reqpackettype;
		header[5]=0;			// packet flags
		header[6]=0;			// header checksum
		header[7]=0;

		if (sock.write(header,sizeof(header))!=
					(ssize_t)sizeof(header) ||
			sock.write(dataflags,ORA_DATA_FLAGS_SIZE)!=
					(ssize_t)ORA_DATA_FLAGS_SIZE ||
			sock.write(body,bodysize)!=(ssize_t)bodysize) {
			setError("failed to write packet fragment");
			return false;
		}
		body+=bodysize;

	} while (remaining);

	sock.flushWriteBuffer(-1,-1);
	return true;
}

bool oracleprotocolclient::recvFragment(bool continuation,
					int32_t timeoutsec,
					bool *gotone,
					size_t *packetsize) {

	*gotone=false;
	*packetsize=0;

	unsigned char	header[8];
	if (sock.read(header,sizeof(header),
			timeoutsec,0)!=(ssize_t)sizeof(header)) {
		if (continuation) {
			return true;
		}
		setError("failed to read packet header");
		return false;
	}

	size_t	size=0;
	if (largeheader) {
		size=((size_t)header[0]<<24)|((size_t)header[1]<<16)|
				((size_t)header[2]<<8)|(size_t)header[3];
	} else {
		size=((size_t)header[0]<<8)|(size_t)header[1];
	}

	// a fragment carries the same type as the packet it continues, and
	// nothing else can turn up mid-message
	if (continuation && header[4]!=resppackettype) {
		setError("bad continuation packet type");
		return false;
	}
	resppackettype=header[4];
	if (!continuation) {
		resppacketflags=header[5];
	}

	size_t	overhead=(continuation)?ORA_FRAGMENT_OVERHEAD:sizeof(header);
	if (size<overhead || size>ORA_MAX_PACKET_SIZE) {
		setError("bad packet size");
		return false;
	}

	// every fragment repeats the data flags, which describe the packet
	// rather than the message being reassembled, so they are read off and
	// dropped here rather than kept as message bytes
	if (continuation) {
		unsigned char	dataflags[ORA_DATA_FLAGS_SIZE];
		if (sock.read(dataflags,sizeof(dataflags),
				timeoutsec,0)!=(ssize_t)sizeof(dataflags)) {
			setError("failed to read continuation data flags");
			return false;
		}
	}

	size_t	bodysize=size-overhead;
	if (respsize+bodysize>ORA_MAX_PACKET_SIZE) {
		setError("response too large to reassemble");
		return false;
	}

	if (bodysize && sock.read(resppacket+respsize,bodysize,
				timeoutsec,0)!=(ssize_t)bodysize) {
		setError("failed to read packet body");
		return false;
	}
	respsize+=bodysize;

	*gotone=true;
	*packetsize=size;

	return true;
}

// one whole message, however many packets it arrived in.  nothing on the wire
// marks a fragment as one - there is no continuation bit, no total length and
// no end marker, and a fragment is byte for byte an ordinary packet - so the
// only sign another may follow is a data packet that filled the sdu.  one is
// read for speculatively on that sign, and a read that finds nothing means the
// message was simply that long
bool oracleprotocolclient::recvPacket() {

	respsize=0;
	respposition=0;

	bool	gotone=false;
	size_t	packetsize=0;
	if (!recvFragment(false,ORA_RESPONSE_TIMEOUT_SEC,
						&gotone,&packetsize)) {
		return false;
	}

	while (gotone && resppackettype==ORA_PACKET_DATA &&
			negotiatedsdu && packetsize==(size_t)negotiatedsdu) {
		if (!recvFragment(true,ORA_CONTINUATION_TIMEOUT_SEC,
						&gotone,&packetsize)) {
			return false;
		}
	}

	return true;
}


// ---- response walking ----

void oracleprotocolclient::rewindResponse() {
	respposition=0;
}

bool oracleprotocolclient::readByte(unsigned char *value) {
	if (respposition>=respsize) {
		return false;
	}
	*value=resppacket[respposition];
	respposition++;
	return true;
}

bool oracleprotocolclient::readBytes(unsigned char *value, size_t size) {
	if (size>respsize-respposition) {
		return false;
	}
	bytestring::copy(value,resppacket+respposition,size);
	respposition+=size;
	return true;
}

bool oracleprotocolclient::readLenPreInt(uint32_t *value) {
	unsigned char	count=0;
	if (!readByte(&count) || count>4 ||
			(size_t)count>respsize-respposition) {
		return false;
	}
	*value=0;
	for (unsigned char i=0; i<count; i++) {
		*value=((*value)<<8)|resppacket[respposition];
		respposition++;
	}
	return true;
}

bool oracleprotocolclient::readLenString(char **value) {
	unsigned char	length=0;
	if (!readByte(&length) || (size_t)length>respsize-respposition) {
		return false;
	}
	*value=charstring::duplicate((const char *)resppacket+respposition,
								length);
	respposition+=length;
	return true;
}

// a clr, the other way round - the mirror of getLenBytes() in
// src/protocols/oracle.cpp, and the inverse of appendLenBytes() above.  a
// zero length and the 0xfd marker are both nulls, a length up to 252 is the
// short form, and 0xfe introduces the chunked long form, whose chunks are
// concatenated into "value".  a chunk's length is a raw byte, or a count
// prefixed ub4 if this client negotiated CCAP_TTC3_BIG_CHUNK_CLR - see
// bigChunkClrFraming().
//
// a long form written by putLongBytes() rather than putLenBytes() - which
// is what a LONG or a LONG RAW column goes out through - has two more zero
// bytes behind its closing empty chunk.  this stops at the empty chunk
// either way and leaves whatever follows it alone
bool oracleprotocolclient::readLenBytes(unsigned char *value,
						size_t maxsize,
						size_t *size,
						bool *isnull) {

	*size=0;
	*isnull=false;

	unsigned char	length=0;
	if (!readByte(&length)) {
		return false;
	}

	if (!length) {
		*isnull=true;
		return true;
	}
	if (length==ORA_CLR_NULL_MARKER) {
		unsigned char	nullcount=0;
		if (!readByte(&nullcount)) {
			return false;
		}
		*isnull=true;
		return true;
	}

	if (length<=ORA_CLR_MAX_SHORT_LENGTH) {
		if ((size_t)length>maxsize ||
				!readBytes(value,(size_t)length)) {
			return false;
		}
		*size=length;
		return true;
	}

	if (length!=ORA_CLR_LONG_FORM_MARKER) {
		return false;
	}

	for (;;) {

		uint32_t	chunksize=0;
		if (bigChunkClrFraming()) {
			if (!readLenPreInt(&chunksize)) {
				return false;
			}
		} else {
			unsigned char	rawchunksize=0;
			if (!readByte(&rawchunksize)) {
				return false;
			}
			chunksize=rawchunksize;
		}
		if (!chunksize) {
			return true;
		}

		if ((size_t)chunksize>maxsize-(*size) ||
				!readBytes(value+(*size),(size_t)chunksize)) {
			return false;
		}
		*size+=chunksize;
	}
}


// ---- handshake ----

// the connect packet: 50 bytes of fixed fields, the 32-bit sdu and tdu a
// client at PROTOCOL_VERSION_12 repeats behind them, 8 bytes of filler,
// and then the connect descriptor.  field order and the 74-byte data
// offset are what a real OCI client sends, and what
// recvConnectRequest() reads.  a client that offers less than
// PROTOCOL_VERSION_12 sends neither the 32-bit fields nor the filler, and
// its data offset is 58, as a genuine 11.2 client's is
bool oracleprotocolclient::sendConnect(const char *sid) {

	uint16_t	version=(connectversion)?connectversion:
					ORA_PROTOCOL_VERSION_12+5;
	bool		large=(version>=ORA_PROTOCOL_VERSION_12);

	char	*connectdata=NULL;
	charstring::printf(&connectdata,
		"(DESCRIPTION=(CONNECT_DATA=(SID=%s))"
		"(ADDRESS=(PROTOCOL=TCP)(Host=localhost)(Port=1521)))",sid);
	size_t	connectdatasize=charstring::getLength(connectdata);

	beginPacket(ORA_PACKET_CONNECT);
	appendBE16(version);			// version offered
	appendBE16(0x012c);			// lowest version supported
	appendBE16(0x0c41);			// global service options
	appendBE16(offeredsdu);			// sdu
	appendBE16(0xffff);			// tdu
	appendBE16(0x7f08);			// protocol characteristics
	appendBE16(0);				// max packets before ack
	// the byte order marker, always 1, written in host byte order on
	// purpose - the listener compares the raw bytes to work out which
	// end this is.  this test only ever runs on the same machine as
	// the listener, but write it the way a little endian client does
	appendByte(1);
	appendByte(0);
	appendBE16((uint16_t)connectdatasize);
	appendBE16((large)?74:58);		// connect data offset
	appendBE32(5120);			// max connect data receivable
	appendBE16((anonegotiation)?ORA_ANO_FLAGS_WANTED:
					ORA_ANO_FLAGS_NO_SERVICES);
	appendBE32(0);				// trace cross facility item 1
	appendBE32(0);				// trace cross facility item 2
	appendBE32(0);				// trace unique connection id 1
	appendBE32(0);
	appendBE32(0);				// trace unique connection id 2
	appendBE32(0);
	if (large) {
		appendBE32(offeredsdu);		// sdu, 32 bit
		appendBE32(2097152);		// tdu, 32 bit
		unsigned char	filler[8]={0,0,0,0,0,0,0,1};
		appendBytes(filler,sizeof(filler));
	}
	reqpacket.append(connectdata,connectdatasize);

	delete[] connectdata;

	return sendPacket();
}

// one version, whichever setTtiVersion() named - 6 by default, which is the
// highest sendTtiResponse() implements
bool oracleprotocolclient::sendProtocolNegotiation() {

	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendByte(ORA_TTC_PROTOCOL_NEGOTIATION);
	appendByte(ttiversion);
	appendByte(0);				// end of the version array
	reqpacket.append("x86_64/Linux 2.4.xx");
	appendByte(0);

	return sendPacket();
}

// pick the listener's tti protocol negotiation response apart, far enough to
// keep the compile capability array off the end of it.  the layout is
// putTtiResponse() in src/protocols/oracle.cpp, field for field:
//
//	- two data flag bytes and the ttc code, which connect() has already
//	  checked
//	- the tti version, and a zero byte
//	- the server banner, NUL terminated
//	- the database character set, little endian
//	- a server flags byte
//	- the character set graph element count, little endian
//	- the fdo block: a big endian size, then exactly that many bytes
//	- the compile capabilities: a length byte, then that many bytes
//	- the runtime capabilities, the same way
//
// the two arrays are a version 6 field.  putTti5Response() passes no arrays
// at all, and putTtiResponse() then writes neither a length byte nor a zero
// one - the response simply ends at the fdo block - so a version 5 session
// leaves the size at 0 rather than failing here
bool oracleprotocolclient::parseTtiResponse() {

	servercompilecapssize=0;

	rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	unsigned char	version=0;
	unsigned char	zero=0;
	if (!readBytes(dataflags,sizeof(dataflags)) ||
			!readByte(&ttccode) ||
			!readByte(&version) ||
			!readByte(&zero)) {
		setError("truncated tti response");
		return false;
	}

	// the banner, read a byte at a time because only its terminator says
	// how long it is
	unsigned char	bannerbyte=0;
	do {
		if (!readByte(&bannerbyte)) {
			setError("truncated tti response banner");
			return false;
		}
	} while (bannerbyte);

	unsigned char	charsetlow=0;
	unsigned char	charsethigh=0;
	unsigned char	serverflags=0;
	if (!readByte(&charsetlow) || !readByte(&charsethigh) ||
						!readByte(&serverflags)) {
		setError("truncated tti response charset");
		return false;
	}

	// the character set graph elements have a shape this doesn't know how
	// to skip.  a real 11.2 server sends none and neither does the
	// module, so anything but zero here is the end of the road rather
	// than a guess at how far to jump
	unsigned char	graphlow=0;
	unsigned char	graphhigh=0;
	if (!readByte(&graphlow) || !readByte(&graphhigh)) {
		setError("truncated tti response graph element count");
		return false;
	}
	if (graphlow || graphhigh) {
		setError("tti response carries charset graph elements");
		return false;
	}

	// the fdo block, whose contents nothing here reads - only its size
	// matters, since the capabilities are behind it
	unsigned char	fdosizehigh=0;
	unsigned char	fdosizelow=0;
	if (!readByte(&fdosizehigh) || !readByte(&fdosizelow)) {
		setError("truncated tti response fdo size");
		return false;
	}
	size_t	fdosize=((size_t)fdosizehigh<<8)|(size_t)fdosizelow;
	if (fdosize>respsize-respposition) {
		setError("truncated tti response fdo block");
		return false;
	}
	respposition+=fdosize;

	// the arrays, or the end of a version 5 response
	if (respposition>=respsize) {
		return true;
	}

	unsigned char	capssize=0;
	if (!readByte(&capssize) || (size_t)capssize>respsize-respposition) {
		setError("truncated tti response compile capabilities");
		return false;
	}
	if (!readBytes(servercompilecaps,(size_t)capssize)) {
		setError("truncated tti response compile capabilities");
		return false;
	}
	servercompilecapssize=capssize;

	return true;
}

// the data type negotiation.  two things here are load-bearing:
//
//	- the compile capability array's CCAP_FIELD_VERSION byte, which
//	  recvDataTypeRequest() negotiates down to the lower of the two
//	  ends' values.  offering 11.2 lands on 11.2 against a listener at
//	  11.2 or at 12.1
//	- exactly one representation per data type.  OCI is the only client
//	  that offers two, and recvDataTypeRequest() sets ociclient from
//	  that.  offering one keeps ociclient false, which puts
//	  getQuery3Request() and putDescribeInfo() on their thin-driver
//	  branches - and the thin-driver branch is the one whose query
//	  length byte has to equal the declared query size, which is what
//	  query3() below writes
bool oracleprotocolclient::sendDataTypeNegotiation() {

	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendByte(ORA_TTC_DATATYPE_NEGOTIATION);

	// a test's own negotiation, if it gave one
	if (datatyperequest) {
		appendBytes(datatyperequest,datatyperequestsize);
		if (datatyperequestsplit) {
			return sendSplitPacketAt(datatyperequestsplit);
		}
		return sendPacket();
	}

	// the client's remote-in and remote-out character sets, little
	// endian.  the listener uses its own whatever these say
	appendByte((unsigned char)(ORA_CHARSET_AL32UTF8&0xff));
	appendByte((unsigned char)((ORA_CHARSET_AL32UTF8>>8)&0xff));
	appendByte((unsigned char)(ORA_CHARSET_AL32UTF8&0xff));
	appendByte((unsigned char)((ORA_CHARSET_AL32UTF8>>8)&0xff));

	// encoding flags: ENCODING_CONV_LENGTH, what a real OCI client and
	// 9i both send alone
	appendByte(0x02);

	// the compile capabilities.  everything but the logon types, the
	// field version, CCAP_OCI1 and CCAP_TTC3 is zero, and a zero at
	// CCAP_TTC3 leaves the time zone version out of the request, which
	// keeps this simple.
	//
	// the logon types are O5LOGON alone unless setLogonTypes() said
	// otherwise, an 11g-only client's: no O3LOGON login at 11.2 or 12.1,
	// and no O7LOGON to get the 12c verifier at 12.1, so the 11g verifier
	// at both.  10.2 runs O3LOGON for every client, which this one
	// doesn't implement.
	//
	// CCAP_TTC3_BIG_CHUNK_CLR is the one bit a test can turn on there.
	// clear - the default, and what every test here sent before the bit
	// existed - the listener frames a long clr's chunks as a raw length
	// byte apiece; set, it frames them as count prefixed ub4s.  every
	// real client sets it, so the raw-byte framing is only reachable
	// from a client like this one that deliberately leaves it clear.
	// CCAP_TTC3_TZ_VERSION is not in the bit, so the time zone version
	// stays out of the request either way
	unsigned char	compilecaps[ORA_CCAP_SIZE];
	bytestring::zero(compilecaps,sizeof(compilecaps));
	compilecaps[ORA_CCAP_LOGON_TYPES]=logontypes;
	compilecaps[ORA_CCAP_FIELD_VERSION]=fieldversion;
	compilecaps[ORA_CCAP_OCI1]=oci1;
	if (bigchunkclr) {
		compilecaps[ORA_CCAP_TTC3]|=ORA_CCAP_TTC3_BIG_CHUNK_CLR;
	}
	appendByte((unsigned char)sizeof(compilecaps));
	appendBytes(compilecaps,sizeof(compilecaps));

	// the runtime capabilities.  RCAP_DB_TIMEZONE is 0, so the db time
	// zone group that would otherwise follow the arrays is left out.
	// RCAP_TTC_ZERO_COPY is the one bit a test can turn on, which the
	// genuine 11.2 and OCI 23 clients set
	unsigned char	runtimecaps[ORA_RCAP_SIZE];
	bytestring::zero(runtimecaps,sizeof(runtimecaps));
	if (lobzerocopy) {
		runtimecaps[ORA_RCAP_TTC]|=ORA_RCAP_TTC_ZERO_COPY;
	}
	appendByte((unsigned char)sizeof(runtimecaps));
	appendBytes(runtimecaps,sizeof(runtimecaps));

	// the data type list: the type, the type it converts to, and one
	// representation of that type, ended by a zero type.  nothing in
	// the listener's answer depends on this list - it answers with its
	// own table - but the one-representation-per-type shape is what
	// keeps ociclient false
	static const uint16_t	datatypes[][3]={
		{1,1,1},	// VARCHAR
		{2,2,10},	// NUMBER
		{12,12,10},	// DATE
		{23,23,1},	// RAW
		{96,96,1}	// CHAR
	};
	for (size_t i=0; i<sizeof(datatypes)/sizeof(datatypes[0]); i++) {
		appendBE16(datatypes[i][0]);
		appendBE16(datatypes[i][1]);
		appendBE16(datatypes[i][2]);
		appendBE16(0);
	}
	appendBE16(0);

	return sendPacket();
}

// the ano request, then the response.  the response keeps its header flags
// for getAnoResponseFlags() and has to open with the data flags and the ano
// marker
bool oracleprotocolclient::runAnoNegotiation() {

	beginPacket(ORA_PACKET_DATA);
	appendBytes(ORA_ANO_REQUEST,sizeof(ORA_ANO_REQUEST));
	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_DATA || respsize<6 ||
			resppacket[0]!=0x00 || resppacket[1]!=0x00 ||
			resppacket[2]!=0xde || resppacket[3]!=0xad ||
			resppacket[4]!=0xbe || resppacket[5]!=0xef) {
		setError("bad ano response");
		return false;
	}
	anoresponseflags=resppacketflags;
	return true;
}

bool oracleprotocolclient::connect(const char *host, uint16_t port,
							const char *sid) {

	sock.setHost(host);
	sock.setPort(port);
	if (!sock.connect()) {
		setError("failed to connect to the listener");
		return false;
	}
	connected=true;

	// connect() in src/protocols/oracle.cpp reads the connect packet,
	// asks for a resend the way a real database does, and reads it again
	if (!sendConnect(sid) || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_RESEND) {
		setError("expected a resend packet");
		return false;
	}
	if (!sendConnect(sid) || !recvPacket()) {
		return false;
	}
	if (resppackettype==ORA_PACKET_REFUSE) {
		setError("the listener refused the connection");
		return false;
	}
	if (resppackettype!=ORA_PACKET_ACCEPT || respsize<6) {
		setError("expected an accept packet");
		return false;
	}

	// the accept itself keeps the 16-bit header whatever version it
	// announces - only the packets after it switch.  sendAccept() flips
	// its own end at exactly this point, so flip this one here too
	acceptversion=
			(uint16_t)(((uint16_t)resppacket[0]<<8)|resppacket[1]);
	largeheader=(acceptversion>=ORA_PROTOCOL_VERSION_12);

	// and the sdu both ends settled on, behind the version and the global
	// service options.  sendAccept() repeats it as a 32-bit field behind
	// the padding for a large-header client, but it never exceeds 4086, so
	// the 16-bit one carries it exactly either way.  every packet from
	// here on is bounded by it, in both directions
	negotiatedsdu=(uint16_t)(((uint16_t)resppacket[4]<<8)|resppacket[5]);

	// ano exchange
	if (anonegotiation && !runAnoNegotiation()) {
		return false;
	}

	if (!sendProtocolNegotiation() || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_DATA ||
			getResponseTtcCode()!=ORA_TTC_PROTOCOL_NEGOTIATION) {
		setError("bad protocol negotiation response");
		return false;
	}

	// keep the capability array the listener just advertised - the
	// response itself is overwritten by the next one, and a test that
	// asks what the module offered asks long after that
	if (!parseTtiResponse()) {
		return false;
	}

	if (!sendDataTypeNegotiation() || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_DATA) {
		setError("bad data type negotiation response");
		return false;
	}

	return true;
}


// ---- authentication ----

// one AUTH_ pair: the name's size, the name, the value's size, the value
// (left out entirely when the size is 0) and the flags ub4.  see
// sqlrprotocol_oracle::getAuthField()
void oracleprotocolclient::appendAuthField(const char *name,
						const char *value,
						uint32_t flags) {
	size_t	namesize=charstring::getLength(name);
	size_t	valuesize=charstring::getLength(value);
	appendAuthCount((uint32_t)namesize,4);
	appendLenString(name,namesize);
	appendAuthCount((uint32_t)valuesize,4);
	if (valuesize) {
		appendLenString(value,valuesize);
	}
	appendAuthCount(flags,4);
}

// the fixed part of a login request, up to and including the user name.
// the first pointer is what picks the encoding for the whole session - see
// setNativeEncoding() and appendAuthPointer().  the field count is the one
// count here that isn't 4 bytes wide in the native encoding
void oracleprotocolclient::sendAuthRequest(unsigned char ttifunction,
						unsigned char seqnumber,
						const char *user,
						uint32_t authmode,
						uint32_t fieldcount) {

	size_t	usersize=charstring::getLength(user);

	beginTtiCall(ttifunction);
	appendByte(seqnumber);
	appendAuthPointer();
	appendAuthCount((uint32_t)usersize,4);
	appendAuthCount(authmode,4);
	appendAuthPointer();
	appendAuthCount(fieldcount,8);
	appendAuthPointer();
	appendAuthPointer();
	appendLenString(user,usersize);
}

// walk every AUTH_ pair in the answer the listener just sent, keeping the
// first one called "name", if any, and leave the response positioned at the
// trailer behind the last one.  "name", "value", "flags" and "count" may each
// be NULL.  only an answer that went through has pairs - an error doesn't
bool oracleprotocolclient::walkAuthFields(const char *name,
						char **value,
						uint32_t *flags,
						uint32_t *count) {

	bool	found=false;
	if (value) {
		*value=NULL;
	}
	if (flags) {
		*flags=0;
	}
	if (count) {
		*count=0;
	}

	rewindResponse();

	unsigned char	dataflagshigh=0;
	unsigned char	dataflagslow=0;
	unsigned char	ttccode=0;
	uint32_t	paircount=0;
	if (!readByte(&dataflagshigh) || !readByte(&dataflagslow) ||
			!readByte(&ttccode) || ttccode!=ORA_TTC_OK ||
			!readAuthCount(&paircount,2)) {
		return false;
	}

	for (uint32_t i=0; i<paircount; i++) {

		uint32_t	namesize=0;
		char		*fieldname=NULL;
		uint32_t	valuesize=0;
		char		*fieldvalue=NULL;
		uint32_t	fieldflags=0;

		if (!readAuthCount(&namesize,4) || !readLenString(&fieldname)) {
			return false;
		}
		if (!readAuthCount(&valuesize,4)) {
			delete[] fieldname;
			return false;
		}
		if (valuesize && !readLenString(&fieldvalue)) {
			delete[] fieldname;
			return false;
		}
		if (!readAuthCount(&fieldflags,4)) {
			delete[] fieldname;
			delete[] fieldvalue;
			return false;
		}

		if (!found && name && !charstring::compare(fieldname,name)) {
			found=true;
			if (value) {
				*value=fieldvalue;
				fieldvalue=NULL;
			}
			if (flags) {
				*flags=fieldflags;
			}
		}

		delete[] fieldname;
		delete[] fieldvalue;
	}

	if (count) {
		*count=paircount;
	}
	return (found || !name);
}

bool oracleprotocolclient::getAuthFieldCount(uint32_t *count) {
	return walkAuthFields(NULL,NULL,NULL,count);
}

bool oracleprotocolclient::getAuthField(const char *name,
						char **value,
						uint32_t *flags) {
	return walkAuthFields(name,value,flags,NULL);
}

bool oracleprotocolclient::getAuthTrailer(const unsigned char **trailer,
							size_t *size) {
	if (!walkAuthFields(NULL,NULL,NULL,NULL)) {
		return false;
	}
	*trailer=resppacket+respposition;
	*size=respsize-respposition;
	return true;
}

bool oracleprotocolclient::login(const char *user, const char *password) {

	if (!requestChallenge(user)) {
		return false;
	}
	if (getResponseTtcCode()!=ORA_TTC_OK) {
		setError("the listener refused the login at phase one");
		return false;
	}
	return answerChallenge(user,password);
}

bool oracleprotocolclient::requestChallenge(const char *user) {

	gotbreak=false;

	// phase one names the user and asks for a challenge.  the listener
	// reads no field out of it, so it carries none
	sendAuthRequest(ORA_TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY,
					authseqphaseone,user,1,0);
	return sendLoginCall("phase one");
}

bool oracleprotocolclient::sendRawRequest(const unsigned char *request,
							size_t size,
							const char *phase) {
	gotbreak=false;
	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendBytes(request,size);
	return sendLoginCall(phase);
}

bool oracleprotocolclient::sendLoginCall(const char *phase) {
	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_MARKER) {
		return true;
	}

	// markers first for a refused login
	return recvAfterBreak(phase);
}

// a refused login: a break marker and a reset marker, which this client
// answers with a reset marker of its own, and then the error.  see
// sendAuthenticationBreak() in src/protocols/oracle.cpp.  the break is
// the packet in the response already
bool oracleprotocolclient::recvAfterBreak(const char *phase) {
	breakflags=resppacketflags;
	if (respsize!=3 || resppacket[2]!=ORA_MARKER_TYPE_BREAK ||
			!recvPacket() ||
			resppackettype!=ORA_PACKET_MARKER ||
			respsize!=3 || resppacket[2]!=ORA_MARKER_TYPE_RESET) {
		setError("bad marker exchange",phase);
		return false;
	}
	gotbreak=true;
	resetflags=resppacketflags;
	if (!sendMarker(ORA_MARKER_TYPE_RESET) || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_DATA) {
		setError("no error behind the markers",phase);
		return false;
	}
	return true;
}

bool oracleprotocolclient::gotBreak() {
	return gotbreak;
}

unsigned char oracleprotocolclient::getBreakFlags() {
	return breakflags;
}

unsigned char oracleprotocolclient::getResetFlags() {
	return resetflags;
}

// one data byte follows, a reserved byte, then the marker type - see
// sqlrprotocol_oracle::sendMarker()
bool oracleprotocolclient::sendMarker(unsigned char markertype) {
	beginPacket(ORA_PACKET_MARKER);
	appendByte(1);
	appendByte(0);
	appendByte(markertype);
	return sendPacket();
}

// the o5logon exchange, client side.  the math mirrors
// src/auths/oracle_userlist.cpp - see passwordHash(), o5logonChallenge()
// and o5logonComboKey() there:
//
//	password hash	= sha1(password || vfrdata) || 4 zero bytes
//	session key A	= aes192-cbc-decrypt(password hash,
//					the listener's AUTH_SESSKEY)
//	session key B	= 40 random bytes || 8 bytes of 0x08 - the pkcs#7
//			  padding a real client's key carries, and the
//			  listener checks for
//	AUTH_SESSKEY	= aes192-cbc-encrypt(password hash, session key B)
//	b		= A[16..39] xor B[16..39]
//	combo key	= md5(b[0..15]) || md5(b[16..23])[0..7]
//	AUTH_PASSWORD	= aes192-cbc-encrypt(combo key,
//					16 random bytes || password || pad)
bool oracleprotocolclient::answerChallenge(const char *user,
						const char *password) {

	char	*vfrdatahex=NULL;
	char	*sesskeyhex=NULL;
	if (!getAuthField("AUTH_VFR_DATA",&vfrdatahex,NULL) ||
		!getAuthField("AUTH_SESSKEY",&sesskeyhex,NULL)) {
		delete[] vfrdatahex;
		delete[] sesskeyhex;
		setError("no challenge in the phase one response");
		return false;
	}

	unsigned char	*vfrdata=NULL;
	uint64_t	vfrdatasize=0;
	charstring::hexDecode(vfrdatahex,charstring::getLength(vfrdatahex),
						&vfrdata,&vfrdatasize);

	unsigned char	*encparta=NULL;
	uint64_t	encpartasize=0;
	charstring::hexDecode(sesskeyhex,charstring::getLength(sesskeyhex),
						&encparta,&encpartasize);

	delete[] vfrdatahex;
	delete[] sesskeyhex;

	if (encpartasize!=ORA_SESSION_KEY_SIZE_11G) {
		delete[] vfrdata;
		delete[] encparta;
		setError("the challenge isn't an 11g session key");
		return false;
	}

	// the password hash, which both aes keys below are derived from
	unsigned char	passwordhash[ORA_PASSWORD_HASH_SIZE_11G];
	bytestring::zero(passwordhash,sizeof(passwordhash));
	sha1		s;
	bool		ok=(s.append((const unsigned char *)password,
				(uint32_t)charstring::getLength(password)) &&
			s.append(vfrdata,(uint32_t)vfrdatasize));
	if (ok) {
		const unsigned char	*digest=s.getHash();
		ok=(digest!=NULL);
		if (ok) {
			bytestring::copy(passwordhash,digest,20);
		}
	}
	delete[] vfrdata;
	if (!ok) {
		delete[] encparta;
		setError("failed to hash the password");
		return false;
	}

	// session key part A, and this client's own part B
	unsigned char	parta[ORA_SESSION_KEY_SIZE_11G];
	unsigned char	partb[ORA_SESSION_KEY_SIZE_11G];
	unsigned char	encpartb[ORA_SESSION_KEY_SIZE_11G];
	size_t		materialsize=ORA_SESSION_KEY_SIZE_11G-
					ORA_SESSION_KEY_PAD_SIZE_11G;
	bytestring::set(partb+materialsize,
			(unsigned char)ORA_SESSION_KEY_PAD_SIZE_11G,
			ORA_SESSION_KEY_PAD_SIZE_11G);
	csprng	csr;
	ok=(oracleAes192Cbc(false,passwordhash,
				encparta,ORA_SESSION_KEY_SIZE_11G,parta) &&
		csr.generateBytes(partb,sizeof(partb),materialsize) &&
		oracleAes192Cbc(true,passwordhash,
				partb,ORA_SESSION_KEY_SIZE_11G,encpartb));
	delete[] encparta;
	if (!ok) {
		setError("failed to build a session key");
		return false;
	}

	// the combo key the password travels under
	unsigned char	b[ORA_COMBO_KEY_SIZE_11G];
	for (size_t i=0; i<sizeof(b); i++) {
		b[i]=parta[16+i]^partb[16+i];
	}
	unsigned char	combokey[ORA_COMBO_KEY_SIZE_11G];
	md5		m1;
	md5		m2;
	ok=(m1.append(b,16) && m2.append(b+16,8));
	if (ok) {
		const unsigned char	*part1=m1.getHash();
		const unsigned char	*part2=m2.getHash();
		ok=(part1!=NULL && part2!=NULL);
		if (ok) {
			bytestring::copy(combokey,part1,16);
			bytestring::copy(combokey+16,part2,8);
		}
	}
	if (!ok) {
		setError("failed to build a combo key");
		return false;
	}

	// AUTH_PASSWORD: a 16-byte salt, the password, and pkcs#7 padding
	size_t		passwordsize=charstring::getLength(password);
	size_t		padsize=16-((16+passwordsize)%16);
	size_t		plainsize=16+passwordsize+padsize;
	unsigned char	*plainpassword=new unsigned char[plainsize];
	unsigned char	*encpassword=new unsigned char[plainsize];
	bytestring::copy(plainpassword+16,password,passwordsize);
	bytestring::set(plainpassword+16+passwordsize,
				(unsigned char)padsize,padsize);
	ok=(csr.generateBytes(plainpassword,plainsize,16) &&
		oracleAes192Cbc(true,combokey,
				plainpassword,plainsize,encpassword));
	delete[] plainpassword;
	if (!ok) {
		delete[] encpassword;
		setError("failed to encrypt the password");
		return false;
	}

	char	*sesskey=oracleHexEncodeUpper(encpartb,sizeof(encpartb));
	char	*authpassword=oracleHexEncodeUpper(encpassword,plainsize);
	delete[] encpassword;

	// phase two.  the listener reads AUTH_SESSKEY and AUTH_PASSWORD out
	// of this and ignores everything else, so those two are all it
	// carries - a real client sends around twenty
	sendAuthRequest(ORA_TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD,
				authseqphasetwo,user,0x40000101,2);
	appendAuthField("AUTH_SESSKEY",sesskey,1);
	appendAuthField("AUTH_PASSWORD",authpassword,0);
	delete[] sesskey;
	delete[] authpassword;

	gotbreak=false;
	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (resppackettype==ORA_PACKET_MARKER &&
			!recvAfterBreak("phase two")) {
		return false;
	}
	if (getResponseTtcCode()!=ORA_TTC_OK) {
		setError("the listener refused the login");
		return false;
	}

	return true;
}

// the o3logon exchange, client side.  the math mirrors o3logonChallenge() and
// o3logonVerify() in src/auths/oracle_userlist.cpp:
//
//	hash		= the des password hash of user and password
//	session key	= 3des-cbc-decrypt(createkey(hash, sesskey entropy),
//					the listener's AUTH_SESSKEY)
//	AUTH_PASSWORD	= 3des-cbc-encrypt(createkey(session key,
//					password entropy),
//				4 salt bytes || the password from its fifth
//				byte on || zero filler || its first 4 bytes)
//
// the last field is the password rotated left by 4 into a buffer with room
// for 4 more bytes, which is what o3logonVerify() undoes
bool oracleprotocolclient::answerO3LogonChallenge(const char *user,
							const char *password) {

	char	*sesskeyhex=NULL;
	if (!getAuthField("AUTH_SESSKEY",&sesskeyhex,NULL)) {
		setError("no challenge in the phase one response");
		return false;
	}
	unsigned char	*encsesskey=NULL;
	uint64_t	encsesskeysize=0;
	charstring::hexDecode(sesskeyhex,charstring::getLength(sesskeyhex),
						&encsesskey,&encsesskeysize);
	delete[] sesskeyhex;
	if (encsesskeysize!=ORA_O3LOGON_SESSION_KEY_SIZE) {
		delete[] encsesskey;
		setError("the challenge isn't a 9i session key");
		return false;
	}

	// the session key, and the key the password travels under
	unsigned char	hash[ORA_O3LOGON_HASH_SIZE];
	unsigned char	key[ORA_O3LOGON_KEY_SIZE];
	unsigned char	sesskey[ORA_O3LOGON_SESSION_KEY_SIZE];
	bool	ok=(oracleO3LogonPasswordHash(user,password,hash) &&
			oracleO3LogonCreateKey(hash,sizeof(hash),
				ORA_O3LOGON_SESSKEY_ENTROPY,
				sizeof(ORA_O3LOGON_SESSKEY_ENTROPY),key) &&
			oracleDesCbc(false,key,sizeof(key),ORA_O3LOGON_IV,
				encsesskey,ORA_O3LOGON_SESSION_KEY_SIZE,
				sesskey) &&
			oracleO3LogonCreateKey(sesskey,sizeof(sesskey),
				ORA_O3LOGON_PASSWORD_ENTROPY,
				sizeof(ORA_O3LOGON_PASSWORD_ENTROPY),key));
	delete[] encsesskey;
	if (!ok) {
		setError("failed to build an o3logon key");
		return false;
	}

	// AUTH_PASSWORD
	size_t	passwordsize=charstring::getLength(password);
	if (passwordsize<ORA_O3LOGON_PASSWORD_OVERHEAD) {
		setError("the password is too short for o3logon");
		return false;
	}
	size_t		plainsize=(passwordsize+ORA_O3LOGON_PASSWORD_OVERHEAD+
					ORA_O3LOGON_BLOCK_SIZE-1)/
					ORA_O3LOGON_BLOCK_SIZE*
					ORA_O3LOGON_BLOCK_SIZE;
	unsigned char	*plainpassword=new unsigned char[plainsize];
	unsigned char	*encpassword=new unsigned char[plainsize];
	bytestring::zero(plainpassword,plainsize);
	bytestring::copy(plainpassword+ORA_O3LOGON_PASSWORD_OVERHEAD,
				password+ORA_O3LOGON_PASSWORD_OVERHEAD,
				passwordsize-ORA_O3LOGON_PASSWORD_OVERHEAD);
	bytestring::copy(plainpassword+plainsize-
				ORA_O3LOGON_PASSWORD_OVERHEAD,
				password,ORA_O3LOGON_PASSWORD_OVERHEAD);
	csprng	csr;
	ok=(csr.generateBytes(plainpassword,plainsize,
				ORA_O3LOGON_PASSWORD_OVERHEAD) &&
		oracleDesCbc(true,key,sizeof(key),ORA_O3LOGON_IV,
				plainpassword,plainsize,encpassword));
	delete[] plainpassword;
	if (!ok) {
		delete[] encpassword;
		setError("failed to encrypt the password");
		return false;
	}
	char	*authpassword=oracleHexEncodeUpper(encpassword,plainsize);
	delete[] encpassword;

	// phase two.  the listener finds the user name and AUTH_PASSWORD at
	// the end of the request, and nothing else in it matters
	sendAuthRequest(ORA_TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD,
					authseqphasetwo,user,0x0101,1);
	appendAuthField("AUTH_PASSWORD",authpassword,0);
	delete[] authpassword;

	gotbreak=false;
	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (resppackettype==ORA_PACKET_MARKER &&
			!recvAfterBreak("phase two")) {
		return false;
	}

	// the oci7 summary: the ttc code, the end of call status, the end to
	// end field when this client set fast session propagate, the rows
	// processed and then the error number.  an error object opens the
	// same way.  read without the field, one that has it would pass with
	// its rows processed taken for the error number
	uint32_t	eocstatus=0;
	uint32_t	endtoend=0;
	uint32_t	rowsprocessed=0;
	uint32_t	oranum=0;
	unsigned char	skip[3];
	rewindResponse();
	if (resppackettype!=ORA_PACKET_DATA ||
			getResponseTtcCode()!=ORA_TTC_ERROR ||
			!readBytes(skip,sizeof(skip)) ||
			!readLenPreInt(&eocstatus) ||
			(getFastSessionPropagate() &&
				!readLenPreInt(&endtoend)) ||
			!readLenPreInt(&rowsprocessed) ||
			!readLenPreInt(&oranum) || oranum) {
		setError("the listener refused the login");
		return false;
	}

	return true;
}

// ojdbc14's version call, packet [0011] of samples/10545-dev-ojdbc14-login-
// select1-realserver-oracle122.oraproxy: the sequence number, a pointer to
// its banner buffer, the buffer's size and pointers to the returned length
// and the version
bool oracleprotocolclient::versionCall() {
	beginTtiCall(ORA_TTI_VERSION);
	appendByte(0);
	appendByte(1);
	appendLenPreInt(ORA_8I_VERSION_BUFFER_SIZE);
	appendByte(1);
	appendByte(1);
	return (sendPacket() && recvPacket());
}

// the answer ojdbc14 reads: the ttc code, the banner's size as a count
// prefixed int and the banner raw, the packed version as a count prefixed
// int, then TTC_STATUS (0x09) and the end to end field, and nothing else
bool oracleprotocolclient::readVersionResponse(char **banner,
						uint32_t *version,
						uint32_t *endtoend) {

	*banner=NULL;
	*version=0;
	*endtoend=0;

	rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	uint32_t	bannersize=0;
	if (resppackettype!=ORA_PACKET_DATA ||
			!readBytes(dataflags,sizeof(dataflags)) ||
			!readByte(&ttccode) || ttccode!=ORA_TTC_OK ||
			!readLenPreInt(&bannersize) ||
			(size_t)bannersize>respsize-respposition) {
		setError("bad version response");
		return false;
	}
	*banner=charstring::duplicate((const char *)resppacket+respposition,
								bannersize);
	respposition+=bannersize;

	unsigned char	statusttccode=0;
	if (!readLenPreInt(version) ||
			!readByte(&statusttccode) || statusttccode!=0x09 ||
			!readLenPreInt(endtoend) ||
			respposition!=respsize) {
		setError("bad version response tail");
		return false;
	}
	return true;
}

// the start of either login call, as ojdbc14 sends it: the sequence number,
// the portable pointers and counts, and the user name raw
void oracleprotocolclient::sendAuthRequest8i(unsigned char ttifunction,
							const char *user,
							uint32_t authmode,
							uint32_t fieldcount) {
	size_t	usersize=charstring::getLength(user);
	beginTtiCall(ttifunction);
	appendByte(0);
	appendByte(1);
	appendLenPreInt((uint32_t)usersize);
	appendLenPreInt(authmode);
	appendByte(1);
	appendLenPreInt(fieldcount);
	appendByte(1);
	appendByte(1);
	appendBytes((const unsigned char *)user,usersize);
}

bool oracleprotocolclient::requestChallenge8i(const char *user) {
	gotbreak=false;
	sendAuthRequest8i(ORA_TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY,
				user,ORA_8I_AUTH_MODE_PHASE_ONE,
				ORA_OJDBC14_PHASE_ONE_FIELD_COUNT);
	appendBytes(ORA_OJDBC14_PHASE_ONE_FIELDS,
			sizeof(ORA_OJDBC14_PHASE_ONE_FIELDS));
	return sendLoginCall("phase one");
}

// the 8i exchange, client side.  the math is what a real 10.2, 11.2 and 12.2
// server accepted from ojdbc14 10.2.0.5:
//
//	hash		= the des password hash of user and password
//	session key	= des-cbc-decrypt(hash, zero iv, the listener's
//					AUTH_SESSKEY)
//	AUTH_PASSWORD	= des-cbc-encrypt(session key, zero iv,
//				the password || zero bytes to a multiple of 8),
//			  in uppercase hex, then one hex digit, the number
//			  of zero bytes
//
// the password is encrypted as given, in its own case.  only the hash folds
// it to uppercase
bool oracleprotocolclient::compute8iAuthPassword(const char *user,
							const char *password,
							const char *sesskeyhex,
							char **authpassword) {

	*authpassword=NULL;

	size_t	passwordsize=charstring::getLength(password);
	if (!passwordsize ||
		charstring::getLength(sesskeyhex)!=2*ORA_8I_SESSION_KEY_SIZE) {
		return false;
	}

	unsigned char	*encsesskey=NULL;
	uint64_t	encsesskeysize=0;
	charstring::hexDecode(sesskeyhex,charstring::getLength(sesskeyhex),
						&encsesskey,&encsesskeysize);
	if (encsesskeysize!=ORA_8I_SESSION_KEY_SIZE) {
		delete[] encsesskey;
		return false;
	}

	// the session key
	unsigned char	hash[ORA_O3LOGON_HASH_SIZE];
	unsigned char	sesskey[ORA_8I_SESSION_KEY_SIZE];
	bool	ok=(oracleO3LogonPasswordHash(user,password,hash) &&
			oracleDesCbc(false,hash,sizeof(hash),
					ORA_O3LOGON_HASH_IV,
					encsesskey,ORA_8I_SESSION_KEY_SIZE,
					sesskey));
	delete[] encsesskey;
	if (!ok) {
		return false;
	}

	// the password, zero padded
	size_t		padsize=(passwordsize%ORA_O3LOGON_BLOCK_SIZE)?
				ORA_O3LOGON_BLOCK_SIZE-
				passwordsize%ORA_O3LOGON_BLOCK_SIZE:0;
	size_t		plainsize=passwordsize+padsize;
	unsigned char	*plainpassword=new unsigned char[plainsize];
	unsigned char	*encpassword=new unsigned char[plainsize];
	bytestring::zero(plainpassword,plainsize);
	bytestring::copy(plainpassword,password,passwordsize);
	ok=oracleDesCbc(true,sesskey,sizeof(sesskey),ORA_O3LOGON_HASH_IV,
					plainpassword,plainsize,encpassword);
	delete[] plainpassword;
	if (!ok) {
		delete[] encpassword;
		return false;
	}

	// hex, then the pad digit
	char	*hex=oracleHexEncodeUpper(encpassword,plainsize);
	delete[] encpassword;
	charstring::printf(authpassword,"%s%d",hex,(int)padsize);
	delete[] hex;
	return true;
}

bool oracleprotocolclient::answerChallenge8i(const char *user,
							const char *password,
							const char *authpassword) {

	char	*computed=NULL;
	if (!authpassword) {
		char	*sesskeyhex=NULL;
		if (!getAuthField("AUTH_SESSKEY",&sesskeyhex,NULL)) {
			setError("no challenge in the phase one response");
			return false;
		}
		bool	ok=compute8iAuthPassword(user,password,sesskeyhex,
								&computed);
		delete[] sesskeyhex;
		if (!ok) {
			setError("the challenge isn't an 8i session key");
			return false;
		}
		authpassword=computed;
	}

	// phase two
	sendAuthRequest8i(ORA_TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD,
				user,ORA_8I_AUTH_MODE_PHASE_TWO,
				ORA_OJDBC14_PHASE_TWO_FIELD_COUNT);
	appendAuthField("AUTH_PASSWORD",authpassword,0);
	appendBytes(ORA_OJDBC14_PHASE_TWO_TAIL,
			sizeof(ORA_OJDBC14_PHASE_TWO_TAIL));
	delete[] computed;

	gotbreak=false;
	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (resppackettype==ORA_PACKET_MARKER &&
			!recvAfterBreak("phase two")) {
		return false;
	}
	if (getResponseTtcCode()!=ORA_TTC_OK) {
		setError("the listener refused the login");
		return false;
	}
	return true;
}

bool oracleprotocolclient::logoff8i(const uint32_t *closecursors,
						uint32_t closecount) {
	if (!connected) {
		setError("not connected");
		return false;
	}
	buildOjdbc14Call(ORA_TTI_DISCONNECT,closecursors,closecount);
	bool	ok=(sendPacket() && recvPacket());
	if (ok) {
		beginPacket(ORA_PACKET_DATA);
		appendBE16(ORA_DATA_FLAGS_EOF);
		ok=sendPacket();
	}
	sock.close();
	connected=false;
	return ok;
}

// a data packet that opens with the cursors to close, piggybacked, and then
// the call: 11 69 00 01 <count> <id>... then 03 <function> 00.  every byte is
// ojdbc14's, MEASURED in samples/10734-dev-ojdbc14-* and in the 10745 real
// server captures, e.g. "11 69 00 01 01 02 01 03 01 01 03 09 00" for cursors
// 3 and 1 and the logoff
void oracleprotocolclient::beginOjdbc14Call(unsigned char ttifunction,
						const uint32_t *closecursors,
						uint32_t closecount) {
	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	if (closecount) {
		appendByte(ORA_8I_TTC_PIGGYBACK);
		appendByte(ORA_8I_TTI_CLOSE_CURSORS);
		appendByte(0);
		appendByte(1);
		appendLenPreInt(closecount);
		for (uint32_t i=0; i<closecount; i++) {
			appendLenPreInt(closecursors[i]);
		}
	}
	appendByte(ORA_TTC_TTI_FUNCTION);
	appendByte(ttifunction);
	appendByte(0);
}

// ojdbc14's 0x5e, MEASURED byte for byte in the 10745 real server captures
// (66 bytes for "select 1 from dual" with the 03): the options, the cursor,
// a text pointer and the text's length, the al8i4 pointer and its 13
// elements, two null pointers, two zero ub4s, the max long size, the bind
// pointer and count, five null pointers, a null define pointer and a define
// count of 0, then the text raw, with no length byte, the al8i4 elements, the
// bind descriptors and the bind values
void oracleprotocolclient::buildOjdbc14Query(
				const oracleprotocolojdbc14query *q,
				const uint32_t *closecursors,
				uint32_t closecount) {

	size_t	querysize=charstring::getLength(q->query);
	bool	descriptors=(q->descriptors!=NULL);

	beginOjdbc14Call(ORA_TTI_QUERY3,closecursors,closecount);
	appendLenPreInt(q->options);
	appendLenPreInt(q->cursorid);
	appendByte((q->query)?1:0);
	appendLenPreInt((uint32_t)querysize);
	appendByte(1);
	appendLenPreInt(ORA_8I_AL8I4_SIZE);
	appendByte(0);
	appendByte(0);
	appendLenPreInt(0);
	appendLenPreInt(0);
	appendLenPreInt(ORA_8I_MAX_LONG);
	appendByte((descriptors)?1:0);
	appendLenPreInt((descriptors)?q->bindcount:0);
	for (uint8_t i=0; i<5; i++) {
		appendByte(0);
	}
	appendByte(0);
	appendLenPreInt(0);
	if (querysize) {
		appendBytes((const unsigned char *)q->query,querysize);
	}
	for (uint32_t i=0; i<ORA_8I_AL8I4_SIZE; i++) {
		appendLenPreInt(q->al8i4[i]);
	}
	if (descriptors) {
		appendBytes(q->descriptors,q->descriptorssize);
	}
	if (q->bindrow) {
		appendBytes(q->bindrow,q->bindrowsize);
	}
}

// 03 05 00 <cursor> <rows>, MEASURED
void oracleprotocolclient::buildOjdbc14Fetch(uint32_t cursorid,
						uint32_t rowstofetch,
						const uint32_t *closecursors,
						uint32_t closecount) {
	beginOjdbc14Call(ORA_TTI_FETCH,closecursors,closecount);
	appendLenPreInt(cursorid);
	appendLenPreInt(rowstofetch);
}

// a call with nothing behind its sequence number: commit, rollback, logoff
void oracleprotocolclient::buildOjdbc14Call(unsigned char ttifunction,
						const uint32_t *closecursors,
						uint32_t closecount) {
	beginOjdbc14Call(ttifunction,closecursors,closecount);
}

bool oracleprotocolclient::ojdbc14Query(const oracleprotocolojdbc14query *q,
						const uint32_t *closecursors,
						uint32_t closecount) {
	buildOjdbc14Query(q,closecursors,closecount);
	return sendPacket() && recvPacket();
}

bool oracleprotocolclient::ojdbc14Fetch(uint32_t cursorid,
						uint32_t rowstofetch,
						const uint32_t *closecursors,
						uint32_t closecount) {
	buildOjdbc14Fetch(cursorid,rowstofetch,closecursors,closecount);
	return sendPacket() && recvPacket();
}

bool oracleprotocolclient::ojdbc14Commit(const uint32_t *closecursors,
						uint32_t closecount) {
	buildOjdbc14Call(ORA_8I_TTI_COMMIT,closecursors,closecount);
	return sendPacket() && recvPacket();
}

bool oracleprotocolclient::ojdbc14Rollback(const uint32_t *closecursors,
						uint32_t closecount) {
	buildOjdbc14Call(ORA_8I_TTI_ROLLBACK,closecursors,closecount);
	return sendPacket() && recvPacket();
}

// ojdbc14's compressed number - T4CMAREngine.buffer2Value(): a length byte
// whose 0x80 bit says negative, then that many big-endian bytes.  a length
// longer than the field's width is internalClose() and ORA-17412 there, so a
// ub2 or sb2 takes at most 2 bytes and a ub4, sb4 or sword at most 4
bool oracleprotocolclient::readOjdbc14Number(int64_t *value,
						unsigned char maxlength,
						const char *field) {
	unsigned char	length=0;
	if (!readByte(&length)) {
		setError("the answer ends inside",field);
		return false;
	}
	bool	negative=((length&0x80)!=0);
	length&=0x7f;
	if (length>maxlength) {
		char	detail[128];
		charstring::printf(detail,sizeof(detail),
				"%s has a %d byte length where at most %d fit "
				"(ORA-17412)",field,(int)length,(int)maxlength);
		setError("ojdbc14 would close the connection",detail);
		return false;
	}
	if ((size_t)length>respsize-respposition) {
		setError("the answer ends inside",field);
		return false;
	}
	uint64_t	v=0;
	for (unsigned char i=0; i<length; i++) {
		v=(v<<8)|resppacket[respposition++];
	}
	*value=(negative)?-(int64_t)v:(int64_t)v;
	return true;
}

// ojdbc14's clr - T4CMAREngine.unmarshalCLR(): 0 and 0xff are null, 0xfe
// opens chunks, each a raw length byte and that many bytes, up to a zero
// length, and anything else is that many bytes
bool oracleprotocolclient::readOjdbc14Clr(unsigned char *value,
						size_t maxsize,
						size_t *size, bool *isnull,
						const char *field) {
	*size=0;
	*isnull=false;
	unsigned char	length=0;
	if (!readByte(&length)) {
		setError("the answer ends inside",field);
		return false;
	}
	if (!length || length==0xff) {
		*isnull=true;
		return true;
	}
	if (length!=ORA_CLR_LONG_FORM_MARKER) {
		if ((size_t)length>maxsize || !readBytes(value,length)) {
			setError("bad clr in",field);
			return false;
		}
		*size=length;
		return true;
	}
	for (;;) {
		unsigned char	chunk=0;
		if (!readByte(&chunk)) {
			setError("the answer ends inside",field);
			return false;
		}
		if (!chunk) {
			return true;
		}
		if ((size_t)chunk>maxsize-(*size) ||
				!readBytes(value+(*size),chunk)) {
			setError("bad clr chunk in",field);
			return false;
		}
		*size+=chunk;
	}
}

// ojdbc14's dalc: a ub4 length, then a clr if the length isn't 0
bool oracleprotocolclient::readOjdbc14Dalc(unsigned char *value,
						size_t maxsize,
						size_t *size,
						const char *field) {
	*size=0;
	int64_t	length=0;
	if (!readOjdbc14Number(&length,4,field)) {
		return false;
	}
	if (length<=0) {
		return true;
	}
	bool	isnull=false;
	return readOjdbc14Clr(value,maxsize,size,&isnull,field);
}

// T4CTTIdcb, behind its ttc code, at a version number of 10000 and up, which
// every ceiling the module reports to ojdbc14 is: a length byte and the block
// it skips, the max row size, the column count and, if there are columns, a
// raw byte, then the columns, then a dalc and two ub4s.  each column is
// T4CTTIoac's fields and T4C8TTIuds's: the scale is a ub2 for types 2, 180,
// 181, 183 and 231 and a raw byte for any other, and the column position
// closes it
bool oracleprotocolclient::readOjdbc14Describe(
					oracleprotocolojdbc14answer *a) {

	unsigned char	prologuesize=0;
	unsigned char	skip[255];
	if (!readByte(&prologuesize) || !readBytes(skip,prologuesize)) {
		setError("the answer ends inside the describe prologue");
		return false;
	}
	a->prologuesize=prologuesize;
	if (!readOjdbc14Number(&a->maxrowsize,4,"the max row size") ||
		!readOjdbc14Number(&a->columncount,4,"the column count")) {
		return false;
	}
	if (a->columncount<0 ||
			a->columncount>(int64_t)ORA_8I_MAX_COLUMNS) {
		setError("too many columns described");
		return false;
	}
	if (a->columncount && !readByte(&a->describeconstant)) {
		setError("the answer ends inside the describe");
		return false;
	}

	for (int64_t i=0; i<a->columncount; i++) {

		oracleprotocolojdbc14column	*c=&a->columns[i];
		c->offset=respposition;

		if (!readByte(&c->type) || !readByte(&c->flags) ||
					!readByte(&c->precision)) {
			setError("the answer ends inside a column");
			return false;
		}

		// the scale
		if (c->type==2 || c->type==180 || c->type==181 ||
					c->type==183 || c->type==231) {
			if (!readOjdbc14Number(&c->scale,2,"a column scale")) {
				return false;
			}
		} else {
			unsigned char	scale=0;
			if (!readByte(&scale)) {
				setError("the answer ends inside a column");
				return false;
			}
			c->scale=(int64_t)(signed char)scale;
		}

		unsigned char	toid[255];
		size_t		toidsize=0;
		if (!readOjdbc14Number(&c->maxlength,4,"a column size") ||
			!readOjdbc14Number(&c->maxarraylength,4,
						"a column max array size") ||
			!readOjdbc14Number(&c->flags2,4,"a column's flags") ||
			!readOjdbc14Dalc(toid,sizeof(toid),&toidsize,
							"a column toid") ||
			!readOjdbc14Number(&c->version,2,"a column version") ||
			!readOjdbc14Number(&c->charset,2,"a column charset")) {
			return false;
		}
		if (!readByte(&c->formofuse)) {
			setError("the answer ends inside a column");
			return false;
		}
		if (!readOjdbc14Number(&c->maxcharlength,4,
					"a column's size in characters")) {
			return false;
		}
		if (!readByte(&c->nullable) || !readByte(&c->namelength)) {
			setError("the answer ends inside a column");
			return false;
		}

		// name, schema, type name
		size_t		namesize=0;
		unsigned char	other[255];
		size_t		othersize=0;
		if (!readOjdbc14Dalc((unsigned char *)c->name,ORA_8I_MAX_NAME,
					&namesize,"a column name") ||
			!readOjdbc14Dalc(other,sizeof(other),&othersize,
						"a column's schema") ||
			!readOjdbc14Dalc(other,sizeof(other),&othersize,
						"a column's type name")) {
			return false;
		}
		c->name[namesize]='\0';

		if (!readOjdbc14Number(&c->position,2,"a column position")) {
			return false;
		}
		c->size=respposition-c->offset;
	}

	// the date, then dcbflag and dcbmdbz, which ojdbc14 reads and drops
	a->describetailoffset=respposition;
	unsigned char	date[255];
	size_t		datesize=0;
	if (!readOjdbc14Dalc(date,sizeof(date),&datesize,
					"the describe's date") ||
		!readOjdbc14Number(&a->dcbflag,4,"the describe's flag") ||
		!readOjdbc14Number(&a->dcbmaxdatablocksize,4,
				"the describe's max data block size")) {
		return false;
	}
	a->describetailsize=respposition-a->describetailoffset;
	return true;
}

// T4CTTIoer at a version number of 10000 and up, behind its ttc code: the
// end to end field and the 22 fields behind it, each its own width, then the
// message as a clr when the error number isn't 0
bool oracleprotocolclient::readOjdbc14Summary(
					oracleprotocolojdbc14answer *a) {

	if (!readOjdbc14Number(&a->endtoend,2,"the end to end field") ||
		!readOjdbc14Number(&a->currentrownumber,4,"the row number") ||
		!readOjdbc14Number(&a->retcode,2,"the error number") ||
		!readOjdbc14Number(&a->arrayelementwitherror,2,
				"the array element with error") ||
		!readOjdbc14Number(&a->arrayelementerrno,2,
				"the array element error number") ||
		!readOjdbc14Number(&a->cursorid,2,"the cursor id") ||
		!readOjdbc14Number(&a->errorposition,2,"the error position") ||
		!readByte(&a->sqltype) || !readByte(&a->fatal) ||
		!readOjdbc14Number(&a->flags,2,"the flags") ||
		!readOjdbc14Number(&a->usercursoroptions,2,
				"the user cursor options") ||
		!readByte(&a->upiparameter) || !readByte(&a->warningflag) ||
		!readOjdbc14Number(&a->rba,4,"the rowid's rba") ||
		!readOjdbc14Number(&a->partitionid,2,
				"the rowid's partition id") ||
		!readByte(&a->tableid) ||
		!readOjdbc14Number(&a->blocknumber,4,
				"the rowid's block number") ||
		!readOjdbc14Number(&a->slotnumber,2,"the rowid's slot number") ||
		!readOjdbc14Number(&a->oserror,4,"the os error") ||
		!readByte(&a->statementnumber) || !readByte(&a->callnumber) ||
		!readOjdbc14Number(&a->pad1,2,"the pad") ||
		!readOjdbc14Number(&a->successiterations,4,
				"the success iterations")) {
		if (respposition>=respsize) {
			setError("the answer ends inside the error object");
		}
		return false;
	}
	if (a->retcode) {
		bool	isnull=false;
		if (!readOjdbc14Clr((unsigned char *)a->message,
					ORA_8I_MAX_MESSAGE,&a->messagesize,
					&isnull,"the error message")) {
			return false;
		}
	}
	a->message[a->messagesize]='\0';
	return true;
}

// the answer the way ojdbc14 reads it - T4C8Oall.receive() for a 0x5e or a
// 0x05, T4C7Ocommoncall.receive() for a commit or a rollback, at a version
// number of 10000 and up.  T4C8Oall.receive() takes 0x10 describe, 0x08
// return parameters, 0x06 row header, 0x15 bit vector and 0x07 row objects in
// any number, and the 0x04 error object ends the call.  any other ttc code,
// 0x09 among them, is internalClose() and ORA-17401 there.
// T4C7Ocommoncall.receive() takes one 0x09 and its end to end field, or one
// 0x04.  anything behind the object that ends the call would be read as the
// start of the next answer, so it fails here too.  it reads one value per
// column, which is not what ojdbc14 does for a column described 0 bytes wide
// (mxl 0): ojdbc14 skips it, and a real server sends no value byte for it.
// so it would mis-walk those answers, such as a real server's select null
bool oracleprotocolclient::readOjdbc14Answer(
				oracleprotocolojdbc14answer *a,
				uint32_t columncount,
				bool statuscall) {

	bytestring::zero(a,sizeof(*a));
	rewindResponse();

	unsigned char	dataflags[ORA_DATA_FLAGS_SIZE];
	if (resppackettype!=ORA_PACKET_DATA ||
			!readBytes(dataflags,sizeof(dataflags))) {
		setError("the answer isn't a data packet");
		return false;
	}

	int64_t		ncols=columncount;
	unsigned char	bitvector[ORA_8I_MAX_COLUMNS];
	bool		usebitvector=false;

	for (;;) {

		if (respposition>=respsize) {
			setError((statuscall)?
				"the answer ends without a status or an error":
				"the answer ends without an error object");
			return false;
		}
		if (a->objectcount==ORA_8I_MAX_OBJECTS) {
			setError("too many objects in the answer");
			return false;
		}

		oracleprotocolojdbc14object	*o=&a->objects[a->objectcount];
		o->offset=respposition;
		readByte(&o->code);
		a->objectcount++;

		bool	last=false;
		if (statuscall && o->code==ORA_8I_TTC_STATUS) {
			if (!readOjdbc14Number(&a->statusendtoend,2,
						"the end to end field")) {
				return false;
			}
			last=true;
		} else if (o->code==ORA_TTC_ERROR) {
			if (!readOjdbc14Summary(a)) {
				return false;
			}
			last=true;
		} else if (statuscall) {
			char	detail[64];
			charstring::printf(detail,sizeof(detail),
				"ttc code 0x%02x in a commit or rollback "
				"answer (ORA-17401)",o->code);
			setError("ojdbc14 would close the connection",detail);
			return false;
		} else if (o->code==ORA_TTC_DESCRIBE_INFO) {
			if (!readOjdbc14Describe(a)) {
				return false;
			}
			ncols=a->columncount;
		} else if (o->code==ORA_TTC_OK) {

			// return parameters: al8o4, al8txl, then the key/value
			// pairs, each a ub4, a dalc and a ub2 keyword
			if (!readOjdbc14Number(&a->al8o4count,2,
						"the al8o4 count")) {
				return false;
			}
			if (a->al8o4count<0 || a->al8o4count>
					(int64_t)ORA_8I_MAX_AL8O4) {
				setError("too many al8o4 values");
				return false;
			}
			for (int64_t i=0; i<a->al8o4count; i++) {
				if (!readOjdbc14Number(&a->al8o4[i],4,
							"an al8o4 value")) {
					return false;
				}
			}
			if (!readOjdbc14Number(&a->al8txl,2,"the al8txl size") ||
				!readOjdbc14Number(&a->paircount,2,
						"the key/value pair count")) {
				return false;
			}
			for (int64_t i=0; i<a->paircount; i++) {
				int64_t		n=0;
				unsigned char	v[255];
				size_t		vsize=0;
				if (!readOjdbc14Number(&n,4,"a pair's ub4") ||
					!readOjdbc14Dalc(v,sizeof(v),&vsize,
							"a pair's value") ||
					!readOjdbc14Number(&n,2,
							"a pair's keyword")) {
					return false;
				}
			}
		} else if (o->code==ORA_TTC_ROW_HEADER) {

			// T4C8TTIrxh: a uac buffer length other than 0 is
			// ORA-17405 there, and a bit vector applies to the
			// next row
			unsigned char	other[255];
			size_t		othersize=0;
			if (!readByte(&a->rowheaderflags) ||
				!readOjdbc14Number(&a->numrequests,2,
					"the row header's request count") ||
				!readOjdbc14Number(&a->iterationnumber,2,
					"the row header's iteration number") ||
				!readOjdbc14Number(&a->iterationsthistime,2,
					"the row header's iteration count") ||
				!readOjdbc14Number(&a->uacbufferlength,2,
					"the row header's uac buffer size") ||
				!readOjdbc14Dalc(bitvector,sizeof(bitvector),
						&a->bitvectorsize,
						"the row header's bit vector") ||
				!readOjdbc14Dalc(other,sizeof(other),&othersize,
						"the row header's last field")) {
				return false;
			}
			if (a->uacbufferlength) {
				setError("ojdbc14 would close the connection",
					"a uac buffer in the row header "
					"(ORA-17405)");
				return false;
			}
			usebitvector=(a->bitvectorsize>0);
		} else if (o->code==ORA_8I_TTC_BIT_VECTOR) {
			int64_t	count=0;
			size_t	size=(size_t)((ncols+7)/8);
			if (!readOjdbc14Number(&count,2,
					"the bit vector's column count") ||
				!readBytes(bitvector,size)) {
				setError("the answer ends inside a bit vector");
				return false;
			}
			usebitvector=true;
		} else if (o->code==ORA_TTC_ROW_DATA) {

			// T4CTTIrxd: a clr for each column, except one a bit
			// vector says repeats the row above
			if (a->rowcount==ORA_8I_MAX_ROWS) {
				setError("too many rows in the answer");
				return false;
			}
			for (int64_t c=0; c<ncols; c++) {
				oracleprotocolojdbc14value	*v=
						&a->values[a->rowcount][c];
				if (usebitvector &&
					!(bitvector[c/8]&(1<<(c%8)))) {
					if (!a->rowcount) {
						setError("a bit vector "
							"suppresses a column "
							"of the first row");
						return false;
					}
					*v=a->values[a->rowcount-1][c];
					continue;
				}
				v->offset=a->valuedatasize;
				if (!readOjdbc14Clr(
					a->valuedata+a->valuedatasize,
					ORA_8I_MAX_VALUE_DATA-
						a->valuedatasize,
					&v->size,&v->isnull,"a row value")) {
					return false;
				}
				a->valuedatasize+=v->size;
			}
			a->rowcount++;
			usebitvector=false;
		} else {
			char	detail[64];
			charstring::printf(detail,sizeof(detail),
				"ttc code 0x%02x in a query or fetch answer "
				"(ORA-17401)",o->code);
			setError("ojdbc14 would close the connection",detail);
			return false;
		}

		o->size=respposition-o->offset;

		if (last) {
			if (respposition!=respsize) {
				char	detail[64];
				charstring::printf(detail,sizeof(detail),
					"%d bytes behind the object that ends "
					"the call",(int)(respsize-respposition));
				setError("bad answer",detail);
				return false;
			}
			return true;
		}
	}
}

void oracleprotocolclient::disconnect() {
	if (!connected) {
		return;
	}
	beginTtiCall(ORA_TTI_DISCONNECT);
	sendPacket();
	sock.close();
	connected=false;
}


// ---- binds ----

// every field zero - a descriptor that claims nothing at all, and the
// starting point for anything that isn't a plain character bind
void oracleprotocolbind::clear() {
	type=0;
	flags=0;
	precision=0;
	scale=0;
	buffersize=0;
	maxelements=0;
	contflags=0;
	oidlength=0;
	oid=NULL;
	version=0;
	charsetid=0;
	csfrm=0;
	maxdatasize=0;
	oaccolid=0;
}

// a character bind of at most "buffersize" bytes.  only two of these
// fields reach anything: getQuery3Binds() in src/protocols/oracle.cpp
// keeps the type, the flags and the buffer size and reads past the rest,
// and of those it acts on the flags - BIND_FLAG_USE_INDICATORS is what
// says a real value follows, where BIND_FLAG_UNBOUND would say the
// placeholder was never bound and cost the whole request its row data.
// the character set fields carry what this client negotiated, so the
// request reads the way a real one does
void oracleprotocolbind::varchar(uint32_t buffersize) {
	clear();
	type=ORA_TYPE_VARCHAR;
	flags=ORA_BIND_FLAG_USE_INDICATORS;
	this->buffersize=buffersize;
	charsetid=ORA_CHARSET_AL32UTF8;
	csfrm=ORA_CSFRM_IMPLICIT;
}

void oracleprotocolquery3offsets::clear() {
	querytext=0;
	al8i4vector=0;
	binddescriptors=0;
	definedescriptors=0;
	rowdata=0;
	end=0;
}

// a value, sized by its own null terminator
void oracleprotocolbindvalue::set(const char *value) {
	this->value=value;
	size=charstring::getLength(value);
}

// a value of a size the caller states, which is what a value with a zero
// byte in it - or one built to be longer than a clr's short form - needs
void oracleprotocolbindvalue::set(const char *value, size_t size) {
	this->value=value;
	this->size=size;
}

void oracleprotocolbindvalue::setNull() {
	value=NULL;
	size=0;
}

// the al8i4 vector: thirteen count prefixed ub4s, back to back, right
// after the query text.  getQuery3Binds() in src/protocols/oracle.cpp
// reads as many elements as the request's vector size field declared and
// keeps exactly one of them - the second, which is the iteration count.
// every other element is read and dropped, so the twelve constants below
// are only what a real client's vector carries, not something the
// listener acts on.  they are also the bytes this client sent as a canned
// array before there was anything to vary, so a no-bind request's vector
// is byte for byte what it always was
void oracleprotocolclient::appendAl8i4Vector(uint32_t iterations) {
	appendLenPreInt(1);		// element 0
	appendLenPreInt(iterations);	// element 1 - the iteration count
	appendLenPreInt(0);		// element 2
	appendLenPreInt(0);		// element 3
	appendLenPreInt(0);		// element 4
	appendLenPreInt(0);		// element 5
	appendLenPreInt(0);		// element 6
	appendLenPreInt(6);		// element 7
	appendLenPreInt(0);		// element 8
	appendLenPreInt(32768);		// element 9
	appendLenPreInt(0);		// element 10
	appendLenPreInt(0);		// element 11
	appendLenPreInt(0);		// element 12
}

// one bind or define descriptor: four raw bytes, then a run of count
// prefixed ub4s with a lone raw byte sitting in the middle of it.  the
// order and the widths are getQuery3BindDescriptor()'s read order in
// src/protocols/oracle.cpp, and two of them are worth naming: csfrm,
// which is a plain byte between two ub4s rather than a ub4 of its own,
// and oaccolid, which only exists once the negotiated field version
// reaches 12.2 - this client negotiates 11.2, so it stays out.
//
// the oid bytes only go out behind a nonzero oid length, since that
// length is what the listener reads them by.  a define takes the
// identical shape: the listener reads it with the same function and
// throws the results away
void oracleprotocolclient::appendBindDescriptor(
				const oracleprotocolbind *bind) {

	appendByte(bind->type);				// type
	appendByte(bind->flags);			// flags
	appendByte(bind->precision);			// precision
	appendByte(bind->scale);			// scale
	appendLenPreInt(bind->buffersize);		// buffer size
	appendLenPreInt(bind->maxelements);		// max elements
	appendLenPreInt(bind->contflags);		// cont flags
	appendLenPreInt(bind->oidlength);		// oid length
	if (bind->oidlength && bind->oid) {
		appendBytes(bind->oid,bind->oidlength);	// oid
	}
	appendLenPreInt(bind->version);			// version
	appendLenPreInt(bind->charsetid);		// charset id
	appendByte(bind->csfrm);			// csfrm
	appendLenPreInt(bind->maxdatasize);		// max data size
	if (fieldversion>=ORA_CCAP_FIELD_VERSION_12_2) {
		appendLenPreInt(bind->oaccolid);	// oaccolid
	}
}

// the bind values: one TTC_ROW_DATA block per execution iteration, each a
// marker byte and then one clr per bind, in descriptor order.  there is no
// length in front of a block, nothing between two of them and nothing
// after the last - getQuery3BindValues() in src/protocols/oracle.cpp
// reads blocks until the packet runs out or it has as many as it was
// expecting.  a value carries no type of its own either; the type comes
// from the descriptor it lines up with.
//
// "values" is one flat array of blockcount*bindcount values, block by
// block, which is the order they go out in
void oracleprotocolclient::appendRowDataBlocks(
				const oracleprotocolbindvalue *values,
				uint32_t bindcount,
				uint32_t blockcount) {

	for (uint32_t block=0; block<blockcount; block++) {
		appendByte(ORA_TTC_ROW_DATA);		// row data marker
		for (uint32_t i=0; i<bindcount; i++) {
			const oracleprotocolbindvalue	*v=
					&(values[block*bindcount+i]);
			appendLenBytes(v->value,v->size);	// value
		}
	}
}


// ---- calls ----

// TTI_OPEN: a sequence number, a pointer flag for the cursor id the
// listener allocates, and an open size.  see open() in
// src/protocols/oracle.cpp
bool oracleprotocolclient::open(uint32_t *cursorid) {

	beginTtiCall(ORA_TTI_OPEN);
	appendByte(1);			// sequence number
	appendByte(1);			// cursor id pointer
	appendLenPreInt(0);		// open size

	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (getResponseTtcCode()!=ORA_TTC_OK) {
		setError("the listener refused to open a cursor");
		return false;
	}

	rewindResponse();
	unsigned char	skip=0;
	if (!readByte(&skip) || !readByte(&skip) || !readByte(&skip) ||
					!readLenPreInt(cursorid)) {
		setError("bad open response");
		return false;
	}
	return true;
}

// TTI_CLOSE: a sequence number and the cursor id to close.  see close() in
// src/protocols/oracle.cpp.  the reply is a bare TTC_STATUS (0x09) message,
// the same shape commit()/rollback()/autocommit-on/off answer with - see
// readCallStatus() below.  0x08 rather than a named ORA_TTI_CLOSE constant:
// several other test programs in this directory already define that name
// locally, and each one includes this file
bool oracleprotocolclient::close(uint32_t cursorid) {

	beginTtiCall(0x08);
	appendByte(1);			// sequence number
	appendAuthCount(cursorid,4);

	return sendPacket() && recvPacket();
}

// TTI_OPEN's reply carries a call status too, behind the cursor id open()
// already decodes: TTC_OK, the cursor id, then TTC_STATUS (0x09) and the
// status itself - see #10295.  this walks the same response fresh rather
// than picking up where open() left off, so it works whether or not the
// caller still cares about the cursor id.  0x09 rather than a named
// ORA_TTC_STATUS constant - see close() above for why.
//
// the status is read the same way open() reads the cursor id, as a
// length-prefixed int in either encoding, because that's what
// sendOpenResponse() in src/protocols/oracle.cpp sends even with -native.  a
// real server's native open reply is fixed-width instead ("01 00 00 00 09 05
// 00 00 00" at [0034] of samples/10295-redhat9x86-oci7-native-commit-select-
// realserver.oraproxy), but no real client negotiates the native encoding
// with the module - see nativeencoding there
static inline bool readOpenCallStatus(oracleprotocolclient *client,
					uint32_t *status) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	uint32_t	cursorid=0;
	unsigned char	statuscode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) || ttccode!=ORA_TTC_OK ||
		!client->readLenPreInt(&cursorid) ||
		!client->readByte(&statuscode) ||
		statuscode!=0x09) {
		return false;
	}

	return client->readLenPreInt(status);
}

// a bare TTC_STATUS (0x09) reply's status value, in this session's
// encoding - what close() above answers with, and what commit(), rollback()
// and autocommit-on/off answer with too (checkTransactionCall() in
// oracletransaction.cpp matches those against a whole fixed byte array
// instead, since their reply never carries anything else)
static inline bool readCallStatus(oracleprotocolclient *client,
					uint32_t *status) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) || ttccode!=0x09) {
		return false;
	}

	return client->readAuthCount(status,4);
}

// the same bare TTC_STATUS (0x09) reply, but for a query3 session's commit,
// rollback or autocommit-on/off - #10321.  readCallStatus() above can't be
// reused: a query3 session is never native encoded, so its call status is
// always length-prefixed rather than a fixed 4 bytes, and a real server
// puts an end-to-end sequence number behind it that readCallStatus() never
// reads.  that sequence number is always the request's own seq byte plus
// 1 - see sendTransactionResponse()'s query3 branch in
// src/protocols/oracle.cpp
static inline bool readQuery3TransactionStatus(oracleprotocolclient *client,
					uint32_t *status,
					uint32_t *endtoendseqnumber) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) || ttccode!=0x09) {
		return false;
	}

	return client->readLenPreInt(status) &&
			client->readLenPreInt(endtoendseqnumber);
}

// ---- modern reply decoding ----
//
// a query3 session's replies - to query3(), reexecute(), fetch(), and a
// commit or rollback - all end in putSummary()'s summary object in
// src/protocols/oracle.cpp, behind whatever else the reply carries.  its
// first field is the end of call status, the same session call status the
// legacy replies carry, which python-oracledb reads as call_status.
// readQuery3Summary() below walks everything ahead of it, object by object,
// then the summary itself, field by field.
//
// the helpers are "static inline" for the same reason the legacy decoders
// further down are

// a clr whose value nothing here needs
static inline bool skipLenBytes(oracleprotocolclient *client) {
	static unsigned char	value[ORA_MAX_PACKET_SIZE];
	size_t			size=0;
	bool			isnull=false;
	return client->readLenBytes(value,sizeof(value),&size,&isnull);
}

// putDalc(): a count, then a clr unless the count is 0
static inline bool skipDalc(oracleprotocolclient *client) {
	uint32_t	size=0;
	return client->readLenPreInt(&size) && (!size || skipLenBytes(client));
}

// a count prefixed int that may be negative - the sign is the count byte's
// high bit, which readLenPreInt() refuses.  an out bind's null indicator
// goes out as -1, "81 01"
static inline bool skipSignedLenPreInt(oracleprotocolclient *client) {
	unsigned char	count=0;
	unsigned char	bytes[4];
	return client->readByte(&count) && (count&0x7f)<=sizeof(bytes) &&
				client->readBytes(bytes,count&0x7f);
}

// putDescribeInfo(), as it answers this client: a thin driver's clr
// prologue rather than OCI's counted one, and a raw scale byte, since this
// client sends ENCODING_CONV_LENGTH and isn't OCI - see
// putColumnPrecisionScale()
static inline bool skipQuery3DescribeInfo(oracleprotocolclient *client) {

	uint32_t	skipint=0;
	unsigned char	skipbyte=0;
	uint32_t	colcount=0;

	// prologue, max row size, column count and the constant behind it
	if (!skipLenBytes(client) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&colcount) ||
		(colcount && !client->readByte(&skipbyte))) {
		return false;
	}

	// putColumnMetadata(), once per column
	for (uint32_t i=0; i<colcount; i++) {

		// type, flags, precision and scale
		unsigned char	fixed[4];
		if (!client->readBytes(fixed,sizeof(fixed))) {
			return false;
		}

		// size, four zeros, character set, a byte, size again
		for (uint32_t j=0; j<6; j++) {
			if (!client->readLenPreInt(&skipint)) {
				return false;
			}
		}
		if (!client->readByte(&skipbyte) ||
			!client->readLenPreInt(&skipint)) {
			return false;
		}

		// a constant, the name's size, the name
		if (!client->readByte(&skipbyte) ||
			!client->readByte(&skipbyte) ||
			!skipDalc(client)) {
			return false;
		}

		// two zeros, the position, a zero
		for (uint32_t j=0; j<4; j++) {
			if (!client->readLenPreInt(&skipint)) {
				return false;
			}
		}
	}

	// the date, then the data block and prefetch sizes
	if (!skipDalc(client)) {
		return false;
	}
	for (uint32_t j=0; j<5; j++) {
		if (!client->readLenPreInt(&skipint)) {
			return false;
		}
	}
	return true;
}

// putRowHeader(): a flags byte and six counts, the first the column count
static inline bool readQuery3RowHeader(oracleprotocolclient *client,
						uint32_t *colcount) {
	unsigned char	flags=0;
	uint32_t	skipint=0;
	if (!client->readByte(&flags) || !client->readLenPreInt(colcount)) {
		return false;
	}
	for (uint32_t j=0; j<5; j++) {
		if (!client->readLenPreInt(&skipint)) {
			return false;
		}
	}
	return true;
}

// putRowData(), for a row whose every column is a clr - a number, a date or
// any text type.  a long, a long raw and a lob locator go out in other
// shapes, and nothing that calls this selects one
static inline bool skipQuery3Row(oracleprotocolclient *client,
						uint32_t colcount) {
	if (!colcount) {
		return false;
	}
	for (uint32_t i=0; i<colcount; i++) {
		if (!skipLenBytes(client)) {
			return false;
		}
	}
	return true;
}

// putIoVector(): a constant, the bind count, five more counts, then one
// direction byte per bind.  0x0b and 0x10 are TTC_IO_VECTOR and
// BIND_DIRECTION_OUT - literals, since oracleiterations0.cpp, which
// includes this file, already defines an ORA_TTC_IO_VECTOR of its own
static inline bool readQuery3IoVector(oracleprotocolclient *client,
						uint32_t *outbindcount) {

	unsigned char	constant=0;
	uint32_t	bindcount=0;
	uint32_t	skipint=0;
	if (!client->readByte(&constant) ||
		!client->readLenPreInt(&bindcount)) {
		return false;
	}
	for (uint32_t j=0; j<5; j++) {
		if (!client->readLenPreInt(&skipint)) {
			return false;
		}
	}

	*outbindcount=0;
	for (uint32_t i=0; i<bindcount; i++) {
		unsigned char	direction=0;
		if (!client->readByte(&direction)) {
			return false;
		}
		if (direction&0x10) {
			(*outbindcount)++;
		}
	}
	return true;
}

// putOutBindValues(), for scalar out binds: a clr and a signed indicator
// apiece.  a ref cursor's slot is a whole describe instead, and nothing
// that calls this binds one
static inline bool skipQuery3OutBindValues(oracleprotocolclient *client,
						uint32_t outbindcount) {
	for (uint32_t i=0; i<outbindcount; i++) {
		if (!skipLenBytes(client) || !skipSignedLenPreInt(client)) {
			return false;
		}
	}
	return true;
}

// putReturnParameters(): a count, that many counts, then three more
static inline bool skipQuery3ReturnParameters(oracleprotocolclient *client) {
	uint32_t	count=0;
	uint32_t	skipint=0;
	if (!client->readLenPreInt(&count)) {
		return false;
	}
	for (uint32_t j=0; j<count+3; j++) {
		if (!client->readLenPreInt(&skipint)) {
			return false;
		}
	}
	return true;
}

// walk a modern reply to its summary object and decode that, requiring
// the reply to end exactly where the summary does.  what may come ahead of
// it, in any order: a describe, a row header and its rows, an io vector and
// the out bind values behind it, and the return parameters block.  a
// reexecute()'s out bind values, which come with no io vector ahead of them
// to count them, aren't handled.
//
// the summary is putSummary()'s: the end of call status, the end to end
// sequence number, the row count, the ora number, ..., the message if the
// ora number isn't 0.  the two fields putSummaryExtension() adds from field
// version 12.1 up never go out to this client, which offers 11.2.
//
// "cursorid", "oranum" and "message" are optional.  "message" gets the
// error message, truncated to fit and nul terminated, or "" if there's none
static inline bool readQuery3Summary(oracleprotocolclient *client,
						uint32_t *callstatus,
						uint32_t *cursorid=NULL,
						uint32_t *oranum=NULL,
						char *message=NULL,
						size_t messagemax=0) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	if (!client->readBytes(dataflags,sizeof(dataflags))) {
		return false;
	}

	// everything ahead of the summary
	uint32_t	colcount=0;
	uint32_t	outbindcount=0;
	for (;;) {

		unsigned char	ttccode=0;
		if (!client->readByte(&ttccode)) {
			return false;
		}
		if (ttccode==ORA_TTC_ERROR) {
			break;
		}

		bool	ok=false;
		if (ttccode==ORA_TTC_DESCRIBE_INFO) {
			ok=skipQuery3DescribeInfo(client);
		} else if (ttccode==ORA_TTC_ROW_HEADER) {
			ok=readQuery3RowHeader(client,&colcount);
		} else if (ttccode==ORA_TTC_ROW_DATA && outbindcount) {
			ok=skipQuery3OutBindValues(client,outbindcount);
			outbindcount=0;
		} else if (ttccode==ORA_TTC_ROW_DATA) {
			ok=skipQuery3Row(client,colcount);
		} else if (ttccode==0x0b) {
			ok=readQuery3IoVector(client,&outbindcount);
		} else if (ttccode==ORA_TTC_OK) {
			ok=skipQuery3ReturnParameters(client);
		}
		if (!ok) {
			return false;
		}
	}

	// end of call status, end to end sequence number, row count, ora
	// number, two zeros, cursor id, a zero
	uint32_t	status=0;
	uint32_t	number=0;
	uint32_t	curid=0;
	uint32_t	skipint=0;
	unsigned char	skipbyte=0;
	if (!client->readLenPreInt(&status) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&number) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&curid) ||
		!client->readLenPreInt(&skipint)) {
		return false;
	}

	// six raw bytes, then the rowid - a ub4, a ub2, a raw byte, a ub4 and
	// a ub2
	unsigned char	skipbytes[6];
	if (!client->readBytes(skipbytes,sizeof(skipbytes)) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint) ||
		!client->readByte(&skipbyte) ||
		!client->readLenPreInt(&skipint) ||
		!client->readLenPreInt(&skipint)) {
		return false;
	}

	// a zero, a raw zero, the call number, a zero, success iterations,
	// four zeros
	if (!client->readLenPreInt(&skipint) ||
		!client->readByte(&skipbyte) ||
		!client->readByte(&skipbyte)) {
		return false;
	}
	for (uint32_t j=0; j<6; j++) {
		if (!client->readLenPreInt(&skipint)) {
			return false;
		}
	}

	// the message
	static unsigned char	text[ORA_MAX_PACKET_SIZE];
	size_t			textsize=0;
	bool			isnull=false;
	if (number && !client->readLenBytes(text,sizeof(text),
							&textsize,&isnull)) {
		return false;
	}

	// nothing may follow
	if (client->readByte(&skipbyte)) {
		return false;
	}

	*callstatus=status;
	if (cursorid) {
		*cursorid=curid;
	}
	if (oranum) {
		*oranum=number;
	}
	if (message && messagemax) {
		if (textsize>=messagemax) {
			textsize=messagemax-1;
		}
		bytestring::copy(message,text,textsize);
		message[textsize]='\0';
	}
	return true;
}

// TTI_QUERY3 without binds, which is what a call that only parses,
// describes, executes or fetches sends
bool oracleprotocolclient::query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query) {
	// 1 iteration, the count this call has always claimed - with no
	// binds and no defines the listener never reads it anyway
	return query3(options,cursorid,prefetchrows,query,NULL,0,1,NULL,0);
}

// TTI_QUERY3 - the modern combined open/parse/describe/execute call.  the
// field order below is exactly what getQuery3Request() reads, and it was
// checked against a real OCI request captured in
// test/testdetails-oracleprotocol.log: every field is either a ub4 in the
// count-then-bytes form or a single raw byte, so nothing sits at a fixed
// offset and the sequence has to be written out in full.
//
// four things follow the header, in this order, which is the order
// getQuery3Request() and then getQuery3Binds() read them in: the query
// text, the al8i4 vector, every bind descriptor and then every define
// descriptor, and last one row data block per execution iteration
bool oracleprotocolclient::buildQuery3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query,
					const oracleprotocolbind *binds,
					uint32_t bindcount,
					uint32_t iterations,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount,
					const oracleprotocolbind *defines,
					uint32_t definecount,
					oracleprotocolquery3offsets *offsets) {

	oracleprotocolquery3offsets	ignored;
	if (!offsets) {
		offsets=&ignored;
	}
	offsets->clear();

	if ((bindcount && !binds) || (definecount && !defines)) {
		setError("query3 needs a descriptor per bind and define");
		return false;
	}
	if (blockcount && (!bindcount || !values)) {
		setError("query3 needs binds and values for a row data block");
		return false;
	}

	size_t	querysize=charstring::getLength(query);

	// getQuery3Binds() skips the al8i4 vector whole unless there is at
	// least one bind or define, so with neither it is only there because
	// a real client's request has one - which is why it also goes out
	// with a query and no binds, exactly as it always did
	bool		sendvector=(querysize || bindcount || definecount);
	uint32_t	vectorsize=(sendvector)?ORA_AL8I4_SIZE:0;

	beginTtiCall(ORA_TTI_QUERY3);
	appendByte(1);				// sequence number
	appendLenPreInt(options);
	appendLenPreInt(cursorid);
	appendByte(1);				// pointer
	appendLenPreInt((uint32_t)querysize);
	appendByte(1);				// pointer
	appendLenPreInt(vectorsize);
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendLenPreInt(0);			// prefetch buffer size
	appendLenPreInt(prefetchrows);
	appendLenPreInt(0);			// max long size
	appendByte(0);				// pointer
	appendLenPreInt(bindcount);		// bind count
	appendByte(0);				// pointer
	appendByte(1);				// pointer
	appendByte(0);				// pointer
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendLenPreInt(definecount);		// define count
	appendLenPreInt(0);
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendByte(0);				// pointer
	appendLenPreInt(0);
	appendByte(0);				// pointer
	appendLenPreInt(0);
	appendLenPreInt(0);

	// the query text, as a clr: up to 252 bytes, a length byte that has
	// to equal the declared size - which is what getQuery3Request()
	// believes for every client but OCI, and this client isn't one -
	// and past that the chunked long form, which getQuery3Request()
	// reads on its own branch
	offsets->querytext=reqpacket.getSize();
	if (querysize) {
		appendLenBytes(query,querysize);
	}

	offsets->al8i4vector=reqpacket.getSize();
	if (sendvector) {
		appendAl8i4Vector(iterations);
	}

	// every bind descriptor, then every define descriptor - they are
	// one run of identically shaped descriptors as far as the wire is
	// concerned, and only the two counts in the header tell them apart
	offsets->binddescriptors=reqpacket.getSize();
	for (uint32_t i=0; i<bindcount; i++) {
		appendBindDescriptor(&(binds[i]));
	}
	offsets->definedescriptors=reqpacket.getSize();
	for (uint32_t i=0; i<definecount; i++) {
		appendBindDescriptor(&(defines[i]));
	}

	// and the values.  the listener reads no row data at all for a
	// request whose descriptors are all unbound, but that is its call to
	// make from the flags - whatever the caller asked for goes out
	offsets->rowdata=reqpacket.getSize();
	appendRowDataBlocks(values,bindcount,blockcount);

	offsets->end=reqpacket.getSize();
	return true;
}

bool oracleprotocolclient::query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query,
					const oracleprotocolbind *binds,
					uint32_t bindcount,
					uint32_t iterations,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount,
					const oracleprotocolbind *defines,
					uint32_t definecount) {

	return buildQuery3(options,cursorid,prefetchrows,query,
				binds,bindcount,iterations,
				values,blockcount,defines,definecount) &&
		sendPacket() && recvPacket();
}

// TTI_EXECUTE, the modern shape.  once one query3() carrying query text
// has gone out, query3session is true in src/protocols/oracle.cpp and the
// same function code reaches reexecute() there rather than the legacy
// execute() that legacyExecute() writes - so this only works on a
// connection that has already parsed a statement, and legacyExecute() only
// works on one that has not.
//
// five fields, a raw sequence byte and four ub4s, and then fresh values
// for the binds the statement was parsed with.  no descriptors go out:
// restoreQuery3Binds() there remembers how many binds the original query3()
// declared and what types they were, so the caller only has to know the
// count and supply the values
bool oracleprotocolclient::reexecute(uint32_t cursorid,
					uint32_t iterations,
					uint32_t options,
					uint32_t moreoptions,
					uint32_t bindcount,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount) {

	if (blockcount && (!bindcount || !values)) {
		setError("reexecute needs binds and values "
					"for a row data block");
		return false;
	}

	beginTtiCall(ORA_TTI_EXECUTE);
	appendByte(1);				// sequence number
	appendLenPreInt(cursorid);
	appendLenPreInt(iterations);
	appendLenPreInt(options);
	appendLenPreInt(moreoptions);

	appendRowDataBlocks(values,bindcount,blockcount);

	return sendPacket() && recvPacket();
}

// TTI_REEXECUTE_AND_FETCH - byte for byte reexecute()'s own body above,
// just under a different function code and with "iterations" renamed to
// "prefetchrows" to match what reexecuteAndFetch() in src/protocols/
// oracle.cpp reads it as - #10322
bool oracleprotocolclient::reexecuteAndFetch(uint32_t cursorid,
					uint32_t prefetchrows,
					uint32_t options,
					uint32_t moreoptions,
					uint32_t bindcount,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount) {

	if (blockcount && (!bindcount || !values)) {
		setError("reexecuteAndFetch needs binds and values "
					"for a row data block");
		return false;
	}

	beginTtiCall(ORA_TTI_REEXECUTE_AND_FETCH);
	appendByte(1);				// sequence number
	appendLenPreInt(cursorid);
	appendLenPreInt(prefetchrows);
	appendLenPreInt(options);
	appendLenPreInt(moreoptions);

	appendRowDataBlocks(values,bindcount,blockcount);

	return sendPacket() && recvPacket();
}

// TTI_FETCH.  the session took the modern path with its first query3(), so
// fetch() in src/protocols/oracle.cpp hands this to fetch3()
bool oracleprotocolclient::fetch(uint32_t cursorid, uint32_t rowstofetch) {

	beginTtiCall(ORA_TTI_FETCH);
	appendByte(1);				// sequence number
	appendLenPreInt(cursorid);
	appendLenPreInt(rowstofetch);

	return sendPacket() && recvPacket();
}

// TTI_LOB_OPERATIONS, as a read.  0x60 rather than a named constant:
// oracle9ibootstrap.cpp already defines ORA_TTI_LOB_OPERATIONS, and it
// includes this file.  the fields are the ones lobOperations() in
// src/protocols/oracle.cpp reads, in the order OCI 23 sends them in the
// portable encoding - [0027] of
// samples/10713-dev-oci23-portable-lobops-sqlrelay121.oraproxy
bool oracleprotocolclient::lobRead(const unsigned char *locator,
					size_t locatorsize,
					uint32_t offset, uint32_t amount) {

	lobdatapacketcount=0;
	lobdatapacketflags=0;

	beginTtiCall(0x60);
	appendByte(1);				// sequence number
	appendByte(1);				// pointer
	appendLenPreInt((uint32_t)locatorsize);
	appendByte(0);				// pointer
	appendLenPreInt(0);			// destination locator size
	appendLenPreInt(0);			// short amount
	appendBE32(0);				// flags
	appendLenPreInt(ORA_LOB_OP_READ);
	appendByte(0);				// pointer
	appendLenPreInt(0);			// scn
	appendLenPreInt(offset);
	appendLenPreInt(4);			// amount kind
	appendByte(1);				// pointer
	appendBE16(0);
	appendBE16(0);
	appendBE16(0);
	appendBytes(locator,locatorsize);
	appendLenPreInt(amount);

	if (!sendPacket() || !recvPacket()) {
		return false;
	}

	// with zero copy, a packet with the ttc code alone says the data
	// follows, in data descriptor packets, and the answer comes after
	// the last of them.  a descriptor's second field is its byte count,
	// and the bytes ride behind the packet's declared end
	if (resppackettype!=ORA_PACKET_DATA || respsize!=3 ||
			getResponseTtcCode()!=ORA_TTC_LOB_AND_BFILE_DATA) {
		return true;
	}
	for (;;) {
		if (!recvPacket()) {
			return false;
		}
		if (resppackettype!=ORA_PACKET_DATA_DESCRIPTOR ||
				respsize<ORA_LOB_DATA_DESCRIPTOR_SIZE) {
			setError("bad lob data descriptor");
			return false;
		}
		if (!lobdatapacketcount) {
			lobdatapacketflags=resppacketflags;
		}
		lobdatapacketcount++;
		uint32_t	more=((uint32_t)resppacket[0]<<24)|
					((uint32_t)resppacket[1]<<16)|
					((uint32_t)resppacket[2]<<8)|
					(uint32_t)resppacket[3];
		uint32_t	size=((uint32_t)resppacket[4]<<24)|
					((uint32_t)resppacket[5]<<16)|
					((uint32_t)resppacket[6]<<8)|
					(uint32_t)resppacket[7];

		// read past the bytes
		unsigned char	discard[1024];
		while (size) {
			size_t	chunk=(size<sizeof(discard))?
						size:sizeof(discard);
			if (sock.read(discard,chunk,
					ORA_RESPONSE_TIMEOUT_SEC,0)!=
							(ssize_t)chunk) {
				setError("failed to read lob data");
				return false;
			}
			size=size-(uint32_t)chunk;
		}

		if (more==ORA_LOB_DATA_LAST) {
			break;
		}
	}
	return recvPacket();
}

// TTI_DESCRIBE.  the field order is describe()'s read order in
// src/protocols/oracle.cpp, and it was checked against packet [0027] of
// samples/9808-redhat9x86-native-realtable-parse.oraproxy: a sequence byte,
// the cursor id and the position, and then 28 bytes describe() never reads.
//
// the sequence number is 10 rather than the 1 the other calls here send,
// because an ORA-01007 echoes it back and a value the rest of the session
// never uses is one a comparison against the capture can't confuse for
// anything else
bool oracleprotocolclient::describe(uint32_t cursorid, uint32_t position) {

	beginTtiCall(ORA_TTI_DESCRIBE);
	appendByte(10);				// sequence number
	appendAuthCount(cursorid,4);
	appendAuthCount(position,4);

	// the client's own buffer pointers and sizes, which only the native
	// encoding has room for - the portable one packs the same seven
	// fields into whatever the capture's client wrote, and either way
	// nothing reads them
	if (nativeencoding) {
		appendBytes(ORA_DESCRIBE_TRAILING_JUNK,
				sizeof(ORA_DESCRIBE_TRAILING_JUNK));
	}

	return sendPacket() && recvPacket();
}


// ---- the pre-query3 calls ----

// TTI_QUERY: the legacy parse.  query() in src/protocols/oracle.cpp reads a
// one-byte sequence number, the cursor id as a count, a pointer field, the
// query size as a count, and then the query's own clr length byte -
// duplicating the query size, for a query this short - ahead of the text
// itself, which includes its trailing nul.  there is no options field on
// this call at all: see legacyExecute() and legacyFetch() below for where a
// real oci7 client puts options instead.
//
// the query size has to go out through appendAuthCount() rather than as a
// raw byte, the same as the cursor id above it: in the portable encoding a
// count-prefixed int for a value this small happens to look like "0x01"
// (the count) followed by the value, but a native-encoding session needs
// the fixed 4-byte form appendAuthCount() switches to instead
bool oracleprotocolclient::legacyQuery(uint32_t cursorid,
					const char *query) {

	uint32_t	querysize=(uint32_t)charstring::getLength(query)+1;

	beginTtiCall(ORA_TTI_QUERY);
	appendByte(1);			// sequence number
	appendAuthCount(cursorid,4);
	appendByte(1);			// pointer
	appendAuthCount(querysize,4);
	appendByte((unsigned char)querysize);	// the text's own clr length byte
	appendBytes((const unsigned char *)query,querysize-1);
	appendByte(0);			// the query text's own trailing nul

	return sendPacket() && recvPacket();
}

// TTI_EXECUTE: executes what a preceding query()/legacyQuery() parsed.
// execute() in src/protocols/oracle.cpp only takes this shape while
// query3session is false - once it's true the same function code means the
// modern re-execute instead.  #9807's rewrite of execute() reads a one-byte
// sequence number, then the cursor id, the iteration count and an options
// bitmask, each as a count: a real oci7 client's capture is
// "25 | 01 03 | 01 01 | 00" - sequence 0x25, cursor id 3, one iteration and
// no options
bool oracleprotocolclient::legacyExecute(uint32_t cursorid,
					uint32_t iterations,
					uint32_t options) {

	beginTtiCall(ORA_TTI_EXECUTE);
	appendByte(1);			// sequence number
	appendAuthCount(cursorid,4);
	appendAuthCount(iterations,4);
	appendAuthCount(options,4);

	return sendPacket() && recvPacket();
}

// the legacy TTI_FETCH: a one-byte sequence number, then the cursor id and
// the number of rows the client has room for, each as a count - #9698's
// rewrite of fetch().  packet [0023] of test/protocol/oracle/samples/
// oracle102-oci7-portable-login-select.cap is "05 08 01 02 01 01": TTI_FETCH,
// sequence 8, cursor id 2 and a row count of 1.  there is no options field
// on this call at all.  a rowstofetch of 0 asks for every row left, the
// same as query2()'s combined execute-and-fetch
bool oracleprotocolclient::legacyFetch(uint32_t cursorid,
					uint32_t rowstofetch) {

	beginTtiCall(ORA_TTI_FETCH);
	appendByte(1);			// sequence number
	appendAuthCount(cursorid,4);
	appendAuthCount(rowstofetch,4);

	return sendPacket() && recvPacket();
}


// ---- legacy response decoding ----
//
// shared by oraclelegacyfetch.cpp and oracletransaction.cpp so both judge a
// legacy parse/execute/fetch reply the same way rather than each carrying
// its own copy (#10300).
//
// sendQueryResponse() and sendExecuteResponse() (src/protocols/oracle.cpp)
// each answer with nothing but a two-byte data flags header and a
// putOci7Summary() object - no message, no rows, nothing else - and a
// genuine error answers with that same object, behind the same leading
// TTC_ERROR (0x04) byte, plus a message.  so the ttc code can't tell a
// success from an error here, and neither can a fixed total size: the
// summary object's own fields (cursor id, rows processed, parse error
// offset) are themselves length-prefixed and vary in width from one
// answer to the next.  what does tell them apart is walking the object
// field by field, the way readLegacyError() below walks the matching
// fields in the error object, and checking nothing is left over once the
// walk ends - a genuine success is a summary object and nothing else,
// where a genuine error is a summary object plus a message.
// readLegacySummary() below does that walk for a parse or an execute reply,
// and readLegacyError() does the matching walk for an error reply
//
// every function below is "inline" as well as "static": this file is
// #include'd by every raw oracle protocol test, not just the two that
// actually call these, and a plain "static" here would leave the rest
// warning about an unused function

// a legacy fetch's NUMBER column, back to an integer.  putField() in
// src/protocols/oracle.cpp writes a NUMBER as the ascii digits the backend
// handed back, not as putNumberField()'s base-100 form - "a legacy client
// asks the server to convert" - so this is a plain decimal parse, not
// putNumberField()'s inverse.  #9637's legacy-fetch captures against a real
// 10.2 server confirm it: "select 1, 2, 3 from dual" comes back as the clrs
// "01 31", "01 32" and "01 33" - one-byte clrs holding the ascii digits -
// where putNumberField() would have written "02 c1 02" for the first of them
static inline bool oracleNumberToInteger(const unsigned char *bytes,
					size_t size,
					int64_t *value) {

	if (!size) {
		return false;
	}

	bool	negative=(bytes[0]=='-');
	size_t	i=(negative)?1:0;

	if (i>=size) {
		return false;
	}

	int64_t	result=0;
	for (; i<size; i++) {
		if (bytes[i]<'0' || bytes[i]>'9') {
			return false;
		}
		result=result*10+(int64_t)(bytes[i]-'0');
	}

	*value=(negative)?-result:result;
	return true;
}

// walk the plainest legacy fetch response - the one a fetch with no options
// asks for - and collect the value of the single column of each row it
// carries: the data flags, then TTC_ROW_HEADER and the row header
// sendFetchResponse() builds in src/protocols/oracle.cpp.  a portable
// session's header is the flags byte and six length-prefixed counts
// (column count, iteration number, row count, uac buffer length, bit
// vector size and one more of unknown meaning) - it has no fixed size, so
// the portable branch below walks it field by field the way
// readLegacyError() above walks the error object, not skipped as a fixed
// number of bytes.  a native session's header is a
// fixed 29-byte literal instead, with only the column count (byte 4) and
// the row count (the following 4 bytes, big endian) ever varying - see
// sendFetchResponse()'s own comments for where each byte comes from.  a
// marker and a row follow for each row, either way, then a trailer that
// starts with something other than the marker.
//
// putRow() writes two more fields behind every column's value - the
// indicator and the return code odefin() gave the client a pointer for,
// both always 0 here, since this test never odefin's anything - as a
// length-prefixed int each in the portable encoding and a fixed 2 bytes
// each in the native one, the same portable-vs-native split
// appendAuthCount()/readAuthCount() already handle for requests in this
// file, so readAuthCount() below reads them in either encoding without a
// branch of its own.  skipping only the value and landing on these as
// though they were the next row's marker is what made the three-row query
// decode a single, wrong row: 0x00 isn't ORA_TTC_ROW_DATA, so the walk
// below stopped after row one every time.
//
// the row loop below reads exactly one value/indicator/returncode triple
// per marker, so it only walks a genuine one-column result - the shape
// *colcount is checked against by the caller.  a legacy fetch with more
// than one defined column would need an inner loop over *colcount here;
// nothing that calls this exercises that
//
// the header's row count comes back in "headerrowcount" rather than being
// skipped with the rest: a fetch that named a row count gets that count back
// there, and a fetch that asked for 0 - every row there is - gets the number
// of rows that really follow, which is the only thing in the response that
// says how many the client was sent
static inline bool readLegacyFetchRows(oracleprotocolclient *client,
					int64_t *values,
					size_t maxvalues,
					size_t *valuecount,
					uint32_t *colcount,
					uint32_t *headerrowcount) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ROW_HEADER) {
		return false;
	}

	if (client->getNativeEncoding()) {

		// the fixed 29-byte block: 3 bytes ahead of the column
		// count, the column count itself, the row count (4 bytes,
		// big endian), then 21 bytes of trailing padding
		unsigned char	pre[3];
		unsigned char	colcountbyte=0;
		unsigned char	rowcountbytes[4];
		unsigned char	post[21];
		if (!client->readBytes(pre,sizeof(pre)) ||
			!client->readByte(&colcountbyte) ||
			!client->readBytes(rowcountbytes,
						sizeof(rowcountbytes)) ||
			!client->readBytes(post,sizeof(post))) {
			return false;
		}
		*colcount=colcountbyte;
		*headerrowcount=
			((uint32_t)rowcountbytes[0]<<24) |
			((uint32_t)rowcountbytes[1]<<16) |
			((uint32_t)rowcountbytes[2]<<8) |
			(uint32_t)rowcountbytes[3];
	} else {
		unsigned char	flags=0;
		uint32_t	skipint=0;
		if (!client->readByte(&flags) ||
			!client->readLenPreInt(colcount) ||	// column count
			!client->readLenPreInt(&skipint) ||	// iteration number
			!client->readLenPreInt(headerrowcount) ||	// row count
			!client->readLenPreInt(&skipint) ||	// uac buffer length
			!client->readLenPreInt(&skipint) ||	// bit vector size
			!client->readLenPreInt(&skipint)) {
			return false;
		}
	}

	*valuecount=0;
	for (;;) {

		unsigned char	marker=0;
		if (!client->readByte(&marker)) {
			return false;
		}
		if (marker!=ORA_TTC_ROW_DATA) {
			// the trailer, so the rows are done
			return true;
		}

		unsigned char	numbersize=0;
		unsigned char	number[32];
		uint32_t	indicator=0;
		uint32_t	returncode=0;
		if (!client->readByte(&numbersize) ||
			numbersize>sizeof(number) ||
			!client->readBytes(number,numbersize) ||
			!client->readAuthCount(&indicator,2) ||
			!client->readAuthCount(&returncode,2)) {
			return false;
		}

		if (*valuecount>=maxvalues) {
			return false;
		}
		if (!oracleNumberToInteger(number,numbersize,
						&values[*valuecount])) {
			return false;
		}
		(*valuecount)++;
	}
}

// walk sendQueryError()'s legacy answer - the data flags, then the ttc code
// and the summary object putOci7Error() writes, field for field, then the
// message.  a portable-encoding session's object is length-prefixed ints
// with no fixed size, so the portable branch below walks it field by field
// rather than skipping it: see putOci7Summary() in src/protocols/oracle.cpp,
// which it mirrors.  a native-encoding session answers through the separate
// putOci7SummaryNative() writer instead, whose fields are fixed-width, so
// the native branch below mirrors that shape instead - the same
// portable-vs-native split appendAuthCount()/readAuthCount() already handle
// for requests in this file
//
// "leftover" says whether anything at all follows the message.  nothing
// should - #9976 found that what used to answer here ran 21 bytes past the
// end of the object the client parses, which cost the client the call and
// turned up on the next one as ORA-03120
static inline bool readLegacyError(oracleprotocolclient *client,
				uint32_t *oranum,
				char *message,
				size_t messagemax,
				size_t *messagesize,
				bool *leftover) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ERROR) {
		return false;
	}

	uint32_t	skipint=0;
	unsigned char	skipbyte=0;
	if (client->getNativeEncoding()) {

		// putOci7SummaryNative()'s fixed-width 93-byte object: end
		// of call status, an unexplained constant byte, rows
		// processed and the error number (4 bytes each), 2 bytes of
		// padding, the cursor id and parse error offset (2 bytes
		// each), the command type, 26 bytes of padding, the call
		// number, 2 bytes of padding, success iterations (4 bytes),
		// then a marker, a live pointer and trailing padding this
		// module never reproduces - see that function for the byte
		// ranges this walks
		unsigned char	pad2[2];
		unsigned char	pad4[26];
		unsigned char	pad5[2];
		unsigned char	pad6[8];
		unsigned char	livepointer[4];
		unsigned char	pad7[28];
		if (!client->readAuthCount(&skipint,4) ||	// end of call status
			!client->readByte(&skipbyte) ||	// unexplained constant
			!client->readAuthCount(&skipint,4) ||	// rows processed
			!client->readAuthCount(oranum,4) ||
			!client->readBytes(pad2,sizeof(pad2)) ||
			!client->readAuthCount(&skipint,2) ||	// cursor id
			!client->readAuthCount(&skipint,2) ||	// parse error offset
			!client->readByte(&skipbyte) ||	// command type
			!client->readBytes(pad4,sizeof(pad4)) ||
			!client->readByte(&skipbyte) ||	// call number
			!client->readBytes(pad5,sizeof(pad5)) ||
			!client->readAuthCount(&skipint,4) ||	// success iterations
			!client->readBytes(pad6,sizeof(pad6)) ||
			!client->readBytes(livepointer,sizeof(livepointer)) ||
			!client->readBytes(pad7,sizeof(pad7))) {
			return false;
		}
	} else {
		unsigned char	skipbytes[5];
		if (!client->readLenPreInt(&skipint) ||	// end of call status
			!client->readLenPreInt(&skipint) ||	// rows processed
			!client->readLenPreInt(oranum) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||	// cursor id
			!client->readLenPreInt(&skipint) ||	// parse error offset
			!client->readByte(&skipbyte) ||	// command type
			!client->readBytes(skipbytes,5) ||
			// the rowid - a ub4, a ub2, a raw byte, a ub4 and a ub2
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readByte(&skipbyte) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readByte(&skipbyte) ||
			!client->readByte(&skipbyte) ||	// call number
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||	// success iterations
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint)) {
			return false;
		}
	}

	unsigned char	size=0;
	if (!client->readByte(&size) || (size_t)size>=messagemax ||
		!client->readBytes((unsigned char *)message,size)) {
		return false;
	}
	message[size]='\0';
	*messagesize=size;

	unsigned char	extra=0;
	*leftover=client->readByte(&extra);
	return true;
}

// walk sendQueryResponse()'s and sendExecuteResponse()'s answer - the data
// flags, then the ttc code and the same putOci7Summary() fields
// readLegacyError() above walks for the error object, but nothing after
// them.  a genuine parse or execute success is this object and nothing
// else; a genuine error is this object plus a message, which is what
// "leftover" catches.  the portable and native branches below split the
// object the same way readLegacyError() above does.
//
// "callstatus" is optional (NULL by default, which is what every caller
// before #10295 passes) - the leading "end of call status" field, the same
// sticky session status commit()/rollback()/autocommit-on/off and
// open()/close() answer with (see readOpenCallStatus()/readCallStatus()
// above).  every parse or execute reply carries it too, whether or not it
// changed anything
static inline bool readLegacySummary(oracleprotocolclient *client,
				uint32_t *cursorid,
				unsigned char *commandtype,
				uint32_t *rowsprocessed,
				uint32_t *successiterations,
				bool *leftover,
				uint32_t *callstatus=NULL) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ERROR) {
		return false;
	}

	uint32_t	endofcallstatus=0;
	uint32_t	skipint=0;
	unsigned char	skipbyte=0;
	if (client->getNativeEncoding()) {

		// putOci7SummaryNative()'s fixed-width 93-byte object - see
		// the byte-range breakdown on readLegacyError() above
		unsigned char	pad2[2];
		unsigned char	pad4[26];
		unsigned char	pad5[2];
		unsigned char	pad6[8];
		unsigned char	livepointer[4];
		unsigned char	pad7[28];
		if (!client->readAuthCount(&endofcallstatus,4) ||
			!client->readByte(&skipbyte) ||	// unexplained constant
			!client->readAuthCount(rowsprocessed,4) ||
			!client->readAuthCount(&skipint,4) ||	// error number
			!client->readBytes(pad2,sizeof(pad2)) ||
			!client->readAuthCount(cursorid,2) ||
			!client->readAuthCount(&skipint,2) ||	// parse error offset
			!client->readByte(commandtype) ||
			!client->readBytes(pad4,sizeof(pad4)) ||
			!client->readByte(&skipbyte) ||	// call number
			!client->readBytes(pad5,sizeof(pad5)) ||
			!client->readAuthCount(successiterations,4) ||
			!client->readBytes(pad6,sizeof(pad6)) ||
			!client->readBytes(livepointer,sizeof(livepointer)) ||
			!client->readBytes(pad7,sizeof(pad7))) {
			return false;
		}
	} else {
		unsigned char	skipbytes[5];
		if (!client->readLenPreInt(&endofcallstatus) ||
			!client->readLenPreInt(rowsprocessed) ||
			!client->readLenPreInt(&skipint) ||	// error number
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(cursorid) ||
			!client->readLenPreInt(&skipint) ||	// parse error offset
			!client->readByte(commandtype) ||
			!client->readBytes(skipbytes,5) ||
			// the rowid - a ub4, a ub2, a raw byte, a ub4 and a ub2
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readByte(&skipbyte) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readByte(&skipbyte) ||
			!client->readByte(&skipbyte) ||	// call number
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(successiterations) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint) ||
			!client->readLenPreInt(&skipint)) {
			return false;
		}
	}

	unsigned char	extra=0;
	*leftover=client->readByte(&extra);
	if (callstatus) {
		*callstatus=endofcallstatus;
	}
	return true;
}

// a parse or execute response decodes as a summary answering the given
// cursor, with the given command type, rows-processed and
// success-iteration counts, and nothing left over.  command type is 3 for
// a select's parse or execute and 2 for an insert's execute - see
// checkLegacySummaryResponse()'s call sites for what each one expects.
// success iterations is 0 for the session's first parse and 1 for
// everything after the session's first execute, on any cursor - see
// oci7executed in src/protocols/oracle.cpp
static inline bool checkLegacySummaryResponse(oracleprotocolclient *client,
					uint32_t expectedcursorid,
					unsigned char expectedcommandtype,
					uint32_t expectedrowsprocessed,
					uint32_t expectedsuccessiterations) {

	uint32_t	curid=0;
	unsigned char	commandtype=0;
	uint32_t	rowsprocessed=0;
	uint32_t	successiterations=0;
	bool		leftover=false;
	return readLegacySummary(client,&curid,&commandtype,
					&rowsprocessed,&successiterations,
					&leftover) &&
			curid==expectedcursorid &&
			commandtype==expectedcommandtype &&
			rowsprocessed==expectedrowsprocessed &&
			successiterations==expectedsuccessiterations &&
			!leftover;
}
