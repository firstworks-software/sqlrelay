// Copyright (c) 2017  David Muse
// See the file COPYING for more information

// The Oracle wire protocol is unspecified, so this module is developed against
// other open source implementations. Material may be taken from python-oracledb
// or node-oracledb under the Universal Permissive License 1.0, or from go-ora
// under MIT. Do not take the Apache 2.0 option that python-oracledb and
// node-oracledb also offer; it is incompatible with GPLv2. Anything taken must
// carry its origin and license notice here. See COPYING.

#include <sqlrelay/sqlrserver.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/character.h>
#include <rudiments/prng.h>
#include <rudiments/datetime.h>
#include <rudiments/process.h>
#include <rudiments/error.h>

// the wire carries binds by position, so a bind's name has to come back out
// of the query text, walked the same way the rest of the server walks it
#define NEED_BEFORE_BIND_VARIABLE
#define NEED_IS_BIND_DELIMITER
#define NEED_AFTER_BIND_VARIABLE
#include <bindvariables.h>

// This module is developed against the Oracle Wire Protocol doc set on the
// Firstworks trac wiki, at http://trac.firstworks.com/trac/wiki/ - start at
// the "Oracle Wire Protocol" index page, which links "Oracle Wire Protocol -
// Sources", "Oracle Wire Protocol - Packet Structure", "Oracle Wire Protocol
// - Known Unknowns", and the rest of the set.
//
// The docs are derived from open source clients, from one 2003 capture of a
// live 8i session, and from proxy captures against live 11.2 and 12.1
// servers. What that adds up to is the client side. A server's half of a call
// is mirrored from what a client sends and parses, and no source enumerates
// every byte a server may legitimately send. Blobs that are still unexplained
// carry a pointer at the site to the page that covers them, and the ones the
// docs themselves track are indexed on "Oracle Wire Protocol - Known
// Unknowns". Either way they need captures against a real Oracle server to
// resolve. Native network encryption is a gap the docs inherit:
// python-oracledb does not implement it at all, and go-ora's advanced_nego is
// the only source that does. Bind and define descriptors, modern path lob
// encoding, the clr long form for query text, and tls are not covered either.

// packet types
#define	PACKET_CONNECT		1
#define	PACKET_ACCEPT 		2
#define	PACKET_ACKNOWLEDGE	3
#define	PACKET_REFUSE		4
#define	PACKET_REDIRECT		5
#define	PACKET_DATA		6
#define	PACKET_NULL		7
#define	PACKET_ABORT		9
#define	PACKET_RESEND		11
#define	PACKET_MARKER		12
#define	PACKET_ATTENTION	13
#define	PACKET_CONTROL_INFO	14
#define	PACKET_DATA_DESCRIPTOR	15

// marker types - a marker packet's 3-byte body is always
// { 1, 0, marker type }; see python-oracledb's
// src/oracledb/impl/thin/protocol.pyx BaseProtocol._send_marker() and
// constants.pxi (Universal Permissive License 1.0)
#define	MARKER_TYPE_BREAK	1
#define	MARKER_TYPE_RESET	2
#define	MARKER_TYPE_INTERRUPT	3

// a marker packet also carries this bit in the packet header's normally
// unused reserved byte; see go-ora's v2/network/marker_packet.go
// newMarkerPacket() (MIT license), which sets it on every marker it
// sends - a real client's marker (captured for this ticket) carries it
// too, and a reply without it goes unrecognized, leaving the client
// blocked in read()
#define	PACKET_FLAG_MARKER	0x20

// protocol versions
#define PROTOCOL_VERSION_7		0x0134
#define PROTOCOL_VERSION_8		0x0136
#define PROTOCOL_VERSION_9		0x0138
#define PROTOCOL_VERSION_10		0x0139
#define PROTOCOL_VERSION_11		0x013A
#define PROTOCOL_VERSION_12		0x013B

// tti protocol versions that this module implements
#define TTI_VERSION_MIN			5
#define TTI_VERSION_MAX			6

// ns layer nsi flags.  the pair of flag bytes the client sends in the connect
// packet, and the server sends back in the accept.  "na" is oracle's name for
// the native services layer, which is what carries ano.  names and values
// from node-oracledb's lib/thin/sqlnet/constants.js; python-oracledb has
// NSI_NA_REQUIRED, NSI_NA_DISABLED and NSI_SUP_SEC_RENEG under its own names,
// with the same values.
#define NSI_NA_WANTED		0x01
#define NSI_NA_INTERCHANGE	0x02
#define NSI_NA_DISABLED		0x04
#define NSI_NA_NO_SERVICES	0x08
#define NSI_NA_REQUIRED		0x10
#define NSI_NA_AUTH_WANTED	0x20
#define NSI_SUP_SEC_RENEG	0x80

// authentication adapter table "nautab" supports:
// SECURID
// KERBEROS5
// IDENTIX
// RADIUS
// don't know the id's for each though

// encryption types (1-12 from an 8i trace, the aes ids from go-ora)
#define ENC_NONE	0
#define ENC_RC4_40	1
#define ENC_RC4_56	8
#define	ENC_DES		2
#define ENC_DES40	3
#define ENC_RC4_256	6
#define ENC_RC4_128	10
#define ENC_3DES168	12
#define ENC_3DES112	11
#define ENC_AES128	15
#define ENC_AES192	16
#define ENC_AES256	17

// data integrity algorithm ids (1-3 from an 8i trace, the rest from go-ora)
#define	DI_NONE		0
#define	DI_SHA1		3
#define DI_MD5		1
#define DI_SHA512	4
#define DI_SHA256	5
#define DI_SHA384	6

// two task common (ttc) types.  0x20 and 0x44 are unverified - no capture
// and no client source assigns either value.
#define TTC_PROTOCOL_NEGOTIATION	0x01
#define TTC_DATATYPE_NEGOTIATION	0x02
#define TTC_TTI_FUNCTION		0x03
#define TTC_ERROR			0x04
#define TTC_ACCESS_USER_ADDRESS_SPACE	0x05
#define TTC_ROW_HEADER			0x06
#define TTC_ROW_DATA			0x07
#define TTC_OK				0x08
#define TTC_STATUS			0x09
#define TTC_NUMBER_OF_ERROR_RECORDS	0x0a
#define TTC_IO_VECTOR			0x0b
#define TTC_SEND_LONG			0x0c
#define TTC_ORACLE_ACCESSOR		0x0d
#define TTC_LOB_AND_BFILE_DATA		0x0e
#define TTC_WARNING_MESSAGES		0x0f
#define TTC_DESCRIBE_INFO		0x10
#define TTC_PIGGYBACK_TTI_FUNCTION	0x11
#define TTC_UNTRUSTED_CALLOUTS		0x12
#define TTC_FLUSH_OUT_BINDS		0x13
#define TTC_BIT_VECTOR			0x15
#define TTC_END_OF_BIND			0x16
#define TTC_SERVER_PIGGYBACK_FUNCTION	0x17
#define TTC_ONE_WAY_FUNCTION		0x1a
#define TTC_IMPLICIT_RESULT_SET		0x1b
#define TTC_RENEGOTIATE			0x1c
#define TTC_END_OF_RESPONSE		0x1d
#define TTC_EXTPROC1			0x20
#define TTC_TOKEN			0x21
#define TTC_FAST_AUTHENTICATION		0x22
#define TTC_EXTPROC2			0x44
#define TTC_SECURE_NETWORK_SERVICES	0xde

// data flags.  one pair of bytes at the front of a data packet, describing the
// packet rather than any one message in it.
#define DATA_FLAGS_EOF			0x0040
#define DATA_FLAGS_COMPRESSED		0x0400
#define DATA_FLAGS_END_OF_REQUEST	0x0800
#define DATA_FLAGS_BEGIN_PIPELINE	0x1000
#define DATA_FLAGS_END_OF_RESPONSE	0x2000

// o5logon verifier types, and the corresponding session key sizes, which are
// what a client tells the two verifier types apart by
#define VERIFIER_TYPE_11G_1	0xb152
#define VERIFIER_TYPE_11G_2	0x1b25
#define VERIFIER_TYPE_12C	0x4815
#define VFR_DATA_SIZE_11G	10
#define VFR_DATA_SIZE_12C	16
#define SESSION_KEY_SIZE_11G	48
#define SESSION_KEY_SIZE_12C	32

// what real oracle sends for the 12c pbkdf2 parameters.  the auth module uses
// whatever it's handed, but matching oracle is safer for a real client.
#define PBKDF2_CSK_SALT_SIZE	16
#define PBKDF2_VGEN_COUNT	"4096"
#define PBKDF2_SDER_COUNT	"3"

// oracle errors the authentication exchange can end in
#define ORA_INVALID_USERNAME_PASSWORD	1017
#define ORA_NULL_PASSWORD		1005

// the oracle error that ends a fetch, which a client reads as "no more rows"
// rather than as a failure
#define ORA_NO_DATA_FOUND		1403
#define ORA_NO_DATA_FOUND_MESSAGE	"ORA-01403: no data found\n"

// the oracle error for a cursor id the module has no cursor open for
#define ORA_INVALID_CURSOR		1001
#define ORA_INVALID_CURSOR_MESSAGE	"ORA-01001: invalid cursor\n"

// the oracle errors a bind can end a call in - one for a placeholder the
// statement has but the client never bound, one for a bind naming a
// placeholder the statement doesn't have
#define ORA_NOT_ALL_VARIABLES_BOUND	1008
#define ORA_NOT_ALL_VARIABLES_BOUND_MESSAGE \
	"ORA-01008: not all variables bound\n"
#define ORA_ILLEGAL_VARIABLE_NAME	1036
#define ORA_ILLEGAL_VARIABLE_NAME_MESSAGE \
	"ORA-01036: illegal variable name/number\n"

// what completes the call a client's marker interrupted (see the
// "Oracle Wire Protocol - Cancel" wiki page and #9591) - a real server's
// documented response to a break/reset
#define ORA_USER_REQUESTED_CANCEL	1013
#define ORA_USER_REQUESTED_CANCEL_MESSAGE \
	"ORA-01013: user requested cancel of current operation\n"

// what sendUnimplementedFunctionError() sends for a tti function this
// module doesn't implement, or doesn't recognize at all - a real ora
// number, unlike ORA_QUERY_FAILED below
#define ORA_UNIMPLEMENTED_FEATURE		3001
#define ORA_UNIMPLEMENTED_FEATURE_MESSAGE	"ORA-03001: unimplemented feature\n"

// what a lob operation gets back when the locator it quotes doesn't decode
// to a row the module is still holding - a locator from a cursor that has
// since been closed, re-executed or fetched past, or one this module never
// minted at all
#define ORA_INVALID_LOB_LOCATOR			22275
#define ORA_INVALID_LOB_LOCATOR_MESSAGE \
	"ORA-22275: invalid LOB locator specified\n"

// what sendQueryError() sends when the backend left no usable error number
// (0, which putSummary() reads as success) or message - ORA-20000 through
// ORA-20999 is oracle's user-defined exception range, the closest fit for
// an error this module can't attribute to a real ORA number
#define ORA_QUERY_FAILED		20000
#define ORA_QUERY_FAILED_MESSAGE	"ORA-20000: query failed\n"

// what sendTransactionError() sends when a commit, rollback or autocommit
// change fails and the backend left no usable error number or message -
// same rationale and range as ORA_QUERY_FAILED above
#define ORA_TRANSACTION_FAILED		20001
#define ORA_TRANSACTION_FAILED_MESSAGE	"ORA-20001: transaction failed\n"

// the two ways the handshake can say no.  a refuse packet carries a tns error
// number, which is what a listener reports; an error packet after the accept
// carries an oracle error number, which is what a server reports.
#define TNS_CONNECTION_REFUSED		12564
#define ORA_VERSION_NOT_SUPPORTED	3134
#define ORA_VERSION_NOT_SUPPORTED_MESSAGE \
	"ORA-03134: Connections to this server version are no longer " \
	"supported.\n"

// what a real listener refuses an attach with, when the client's
// CONNECT_DATA names a SID/SERVICE_NAME the listener isn't configured for
#define TNS_NO_SUCH_SERVICE		12514

// the vsnnum both refuse message texts carry - oracle 11.2.0.1.0,
// 0x0b200100.  it is fixed at 11.2 whatever the serverversion attribute
// says, and it duplicates SERVER_VERSION_NO_11_2 below.
#define SERVER_VERSION_NUMBER		"186646784"

// what a live 11.2 listener puts in the two reason bytes of a refuse
#define REFUSE_REASON_USER		0x22
#define REFUSE_REASON_SYSTEM		0x00

// two task interface (tti) functions
#define TTI_OPEN		0x02
#define TTI_QUERY		0x03
#define TTI_EXECUTE		0x04
#define TTI_FETCH		0x05
#define TTI_CLOSE		0x08
#define TTI_DISCONNECT		0x09
#define TTI_AUTOCOMMIT_ON	0x0C
#define TTI_AUTOCOMMIT_OFF	0x0D
#define TTI_COMMIT		0x0E
#define TTI_ROLLBACK		0x0F
#define TTI_CANCEL		0x14
#define TTI_DESCRIBE		0x2B
#define TTI_STARTUP		0x30
#define TTI_SHUTDOWN		0x31
#define TTI_VERSION		0x3B
#define TTI_K2_TRANSACTIONS	0x43
#define TTI_QUERY2		0x47
#define TTI_OSQL7		0x4A
#define TTI_OKOD		0x5C
#define TTI_QUERY3		0x5E
#define TTI_LOB_OPERATIONS	0x60
#define TTI_ODNY		0x62
#define TTI_TRANSACTION_END	0x67
#define TTI_TRANSACTION_BEGIN	0x68
#define TTI_OCCA		0x69
#define TTI_STARTUP2		0x6D
#define TTI_LOGON_PRESENT_PWD	0x51
#define TTI_LOGON_PRESENT_USER	0x52
#define TTI_UNIDENTIFIED_0X54	0x54
#define TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD	0x73
#define TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY		0x76
#define TTI_DESCRIBE2		0x77
#define TTI_OOTCM		0x7F
#define TTI_OKPFC		0x8B
#define TTI_SWITCH_SESSION	0x6B
#define TTI_CLOSE2		0x78
#define TTI_OSCID		0x87
#define TTI_OSKEYVAL		0x9A

// server compile-time capability array indices
// (python-oracledb's TNS_CCAP_* with the TNS_ prefix dropped, since this
// file has no common prefix of its own)
#define CCAP_SQL_VERSION		0
#define CCAP_LOGON_TYPES		4
#define CCAP_FEATURE_BACKPORT		5
#define CCAP_FIELD_VERSION		7
#define CCAP_SERVER_DEFINE_CONV		8
#define CCAP_DEQUEUE_WITH_SELECTOR	9
#define CCAP_TTC1			15
#define CCAP_OCI1			16
#define CCAP_TDS_VERSION		17
#define CCAP_RPC_VERSION		18
#define CCAP_RPC_SIG			19
#define CCAP_DBF_VERSION		21
#define CCAP_LOB			23
#define CCAP_TTC2			26
#define CCAP_UB2_DTY			27
#define CCAP_OCI2			31
#define CCAP_CLIENT_FN			34
#define CCAP_OCI3			35
#define CCAP_TTC3			37
#define CCAP_SESS_SIGNATURE_VERSION	39
#define CCAP_TTC4			40
#define CCAP_MAX			55

// server compile-time capability values
#define CCAP_SQL_VERSION_MAX		6
#define CCAP_FIELD_VERSION_11_2		6
#define CCAP_FIELD_VERSION_12_1		7
#define CCAP_FIELD_VERSION_12_2		8
#define CCAP_FIELD_VERSION_12_2_EXT1	9
#define CCAP_TDS_VERSION_MAX		3
#define CCAP_RPC_SIG_VALUE		3
#define CCAP_DBF_VERSION_MAX		1
#define CCAP_O5LOGON_NP			0x02
#define CCAP_O5LOGON			0x08
#define CCAP_O7LOGON			0x20
// CCAP_TTC1 bit 0x01, and CCAP_OCI1 bit 0x01 per go-ora - another reading
// names fast session propagate as 0x10 in that same index, and nothing on
// the wire settles which is right
#define CCAP_END_OF_CALL_STATUS		0x01
#define CCAP_FAST_SESSION_PROPAGATE	0x10
#define CCAP_END_OF_RESPONSE		0x20
#define CCAP_EXPLICIT_BOUNDARY		0x40
#define CCAP_TTC3_TZ_VERSION		0x02

// server runtime capability array indices
#define RCAP_COMPAT			0
#define RCAP_DB_TIMEZONE		1
#define RCAP_TTC			6
#define RCAP_MAX			11

// server runtime capability values
#define RCAP_COMPAT_81			2
#define RCAP_DB_TIMEZONE_REQUESTED	0x01
#define RCAP_TTC_ZERO_COPY		0x01
#define RCAP_TTC_32K			0x04
#define RCAP_TTC_SESSION_STATE_OPS	0x10

// the platform banner in the tti protocol negotiation response.  it names no
// platform on purpose - see putTtiResponse().
#define SERVER_BANNER			"SQLRelay/PortableTTC"

// the two oracle versions the module can imitate, as the listener's
// serverversion attribute selects them.  AUTH_VERSION_NO's nibbles are the
// version: a live 11.2 server reports 0x0b200100 and a live 12.2 server
// reports 0x0c200100, so 0x0c100200 is 12.1.0.2.0.  AUTH_VERSION_SQL is 22 on
// that 11.2 server and 24 on that 12.2 one.  the banner and packed version
// number that the version response sends are derived from AUTH_VERSION_NO, so
// that a client is told the same version after the login that it was told
// during it.
#define SERVER_VERSION_NO_11_2		"186646784"
#define SERVER_VERSION_SQL_11_2		"22"
#define SERVER_VERSION_NO_12_1		"202375680"
#define SERVER_VERSION_SQL_12_1		"23"

// datatype request encoding flags
#define ENCODING_MULTI_BYTE		0x01
#define ENCODING_CONV_LENGTH		0x02

// a length byte over 252 isn't a length.  0xfd introduces a null, 0xfe the
// chunked long form.
#define CLR_MAX_SHORT_LENGTH		252
#define CLR_NULL_MARKER			0xfd
#define CLR_LONG_FORM_MARKER		0xfe
#define CLR_MAX_CHUNK_SIZE		255

// bind descriptor flags.  0x01 says a normal bound value follows, 0x80 says
// the statement has the placeholder but the client never bound anything to
// it - and that no row data follows at all
#define BIND_FLAG_USE_INDICATORS	0x01
#define BIND_FLAG_UNBOUND		0x80

// which way a bind's value travels, as the io vector reports it.  the wire's
// bind descriptors say nothing about direction - every one of them looks the
// same whether the client bound it in, out, or both - so the module works it
// out from the statement itself
// see "Oracle Wire Protocol - Query3"
#define BIND_DIRECTION_OUT		0x10
#define BIND_DIRECTION_IN		0x20
#define BIND_DIRECTION_INOUT		0x30

// the io vector's second byte, which never varied in any capture
#define IO_VECTOR_CONSTANT		0x05

// what a null out bind's indicator goes out as - the count byte's high bit
// means negative, so 81 01 is -1, which is oci's own OCI_IND_NULL
#define OUT_BIND_NULL_INDICATOR_COUNT	0x81
#define OUT_BIND_NULL_INDICATOR_VALUE	0x01

// how big an out bind's buffer is, when the descriptor doesn't ask for more
#define MIN_OUT_BIND_SIZE		128

// how many bind values one query3 request may carry, across all of its
// execution iterations
#define MAX_QUERY3_BIND_VALUES		65536

// describe info constants.  the last three are advisory - a client is free to
// ignore them - and these are what a live 11.2 server sends.
#define AL8O4_COUNT			6
#define DCB_MAX_DATA_BLOCK_SIZE		8168
#define DCB_MIN_PREFETCH		2
#define DCB_MAX_PREFETCH		2

// the two unrelated 51 values the docs warn about: describe info carries
// the byte 0x51, column definitions carries the ub4 51 decimal.  neither
// meaning is sourced.
#define DESCRIBE_INFO_CONSTANT		0x51
#define COLUMN_DEFINITIONS_CONSTANT	51

// what a ref cursor's out bind slot leads with.  it isn't a length - it's
// 0x4c whatever the cursor describes - and no source explains it.  thin
// drivers skip exactly one byte here and call it a fixed value
#define REF_CURSOR_CONSTANT		0x4c

// an oracle number is an exponent byte and up to 20 base 100 digits, and a
// column of them is described as 22 bytes wide
#define MAX_NUMBER_MANTISSA		20
#define MAX_NUMBER_SIZE			22
#define MAX_NUMBER_DIGITS		128
#define MIN_NUMBER_EXPONENT		(-193)
#define MAX_NUMBER_EXPONENT		62

// the widest text an oracle number decodes to: 40 digits, a sign, a decimal
// point, and the run of zeros the smallest exponent puts in front of them
#define MAX_NUMBER_TEXT_SIZE		512

// an oracle date is a fixed 7 bytes, and a column of them is described that
// wide
#define ORACLE_DATE_SIZE		7

// an oracle rowid's external form is 18 base 64 characters: 6 for the data
// object number, 3 for the relative file number, 6 for the block number and
// 3 for the row number.  a live 12.2 server describes such a column 1 byte
// wide and puts a constant 0x0e in front of the value, whatever the four
// numbers in it come to - see putRowidField()
#define ORACLE_ROWID_TEXT_SIZE		18
#define ORACLE_ROWID_OBJECT_DIGITS	6
#define ORACLE_ROWID_FILE_DIGITS	3
#define ORACLE_ROWID_BLOCK_DIGITS	6
#define ORACLE_ROWID_ROW_DIGITS		3
#define ORACLE_ROWID_PARTS		4
#define ORACLE_ROWID_SIZE		1
#define ORACLE_ROWID_LENGTH_BYTE	0x0e

// a raw's external form is two hexadecimal characters per byte
#define ORACLE_RAW_HEX_PER_BYTE		2

// what a raw column whose width the backend doesn't report is described as
#define MAX_RAW_SIZE			2000

// what a long or a long raw is described as.  neither has a declared width
// and a live 12.2 server describes both of them 0 bytes wide, and sends 0
// for the whole row's width too
#define ORACLE_LONG_SIZE		0

// an interval's binary form is a fixed width - 5 bytes for a year to month
// and 11 for a day to second - and every field of it is biased: the leading
// field by 2^31 over 4 bytes, and each of the smaller ones by 60 over 1
// byte.  see putIntervalField()
#define ORACLE_INTERVALYM_SIZE		5
#define ORACLE_INTERVALDS_SIZE		11
#define ORACLE_INTERVAL_LEADING_BIAS	0x80000000
#define ORACLE_INTERVAL_FIELD_BIAS	60
#define ORACLE_INTERVAL_FRACTION_DIGITS	9
#define MAX_INTERVAL_LEADING		999999999

// and, like a rowid, an interval column is described 1 byte wide rather
// than as the width of the value.  a live 12.2 server sends 1 for either
// interval, and oci scales what it is given by the type's own width to get
// the size it reports back: given the 1 it reports 5 and 11, and given a 5
// and an 11 it reports 25 and 121
#define ORACLE_INTERVAL_SIZE		1

// a timestamp's binary form is a date's 7 bytes and then 4 more for the
// nanoseconds, and a timestamp with time zone's is those 11 and then 2 more
// for the offset - its hours biased by 84 and its minutes by 60.  see
// putTimestampField()
#define ORACLE_TIMESTAMP_SIZE		11
#define ORACLE_TIMESTAMPTZ_SIZE		13
#define ORACLE_TZ_HOUR_BIAS		84
#define ORACLE_TZ_MINUTE_BIAS		60
#define ORACLE_TIMESTAMP_FRACTION_DIGITS	9
#define MAX_TIMESTAMP_DATE_TEXT		32

// a timestamp column is described the 11 bytes its value really takes, but
// a timestamp with time zone is described 1 byte wide the way an interval
// is, and oci works the 13 it reports back out from the type.  a live 12.2
// server sends both of those
#define ORACLE_TIMESTAMPTZ_WIRE_SIZE	1

// what a column with no size of its own is described as
#define MAX_VARCHAR_SIZE		4000

// a lob column is described the width of the buffer the client reads a
// locator into rather than the width of the lob itself - a clob and a blob
// 4000 bytes wide and a bfile 530, which is what oci reports back for one
#define ORACLE_LOB_SIZE			4000
#define ORACLE_BFILE_SIZE		530

// a lob column's value is a locator rather than the lob's bytes.  the
// client never looks inside one beyond the type byte - it echoes the whole
// thing back in every lob operation request it makes - so the layout below
// is a real 12.2 server's skeleton with the module's own bookkeeping in the
// fields a real server fills with a per-lob id.
// see "Oracle Wire Protocol - Lob Locator"
#define LOB_LOCATOR_SIZE		114
#define BFILE_LOCATOR_SIZE		43
#define LOB_LOCATOR_CLOB		0x02
#define LOB_LOCATOR_BLOB		0x01
#define LOB_LOCATOR_BFILE		0x08
#define LOB_LOCATOR_CHARSET_PRESENT	0x80
#define LOB_LOCATOR_LENGTH_OFFSET	0x00
#define LOB_LOCATOR_VERSION_OFFSET	0x02
#define LOB_LOCATOR_TYPE_OFFSET		0x04
#define LOB_LOCATOR_KIND_OFFSET		0x05
#define LOB_LOCATOR_FLAGS_OFFSET	0x06
#define LOB_LOCATOR_INIT_OFFSET		0x07
#define LOB_LOCATOR_TYPE2_OFFSET	0x09
#define LOB_LOCATOR_SEQUENCE_OFFSET	0x0c
#define LOB_LOCATOR_ID_OFFSET		0x10
#define LOB_LOCATOR_CHARSET_OFFSET	0x20
#define LOB_LOCATOR_MAGIC_OFFSET	0x22
#define LOB_LOCATOR_GENERATION_OFFSET	0x26
#define LOB_LOCATOR_TRAILER_OFFSET	0x46

// a bfile locator carries a directory alias and a file name where a clob
// or a blob carries a charset, so its bookkeeping sits elsewhere
#define BFILE_LOCATOR_DIRECTORY_OFFSET	0x10
#define BFILE_LOCATOR_FILENAME_OFFSET	0x19
#define BFILE_LOCATOR_ID_OFFSET		0x1c
#define BFILE_LOCATOR_MAGIC_OFFSET	0x20
#define BFILE_LOCATOR_GENERATION_OFFSET	0x24

// what marks a locator as this module's own, so one left over from a
// cursor that has since been closed and reused can be told apart
#define LOB_LOCATOR_MAGIC		"SQLR"
#define LOB_LOCATOR_MAGIC_SIZE		4

// the constant a real 12.2 server leaves in a locator's tail
#define LOB_LOCATOR_TRAILER		0xdeadbeef

// the rest of a real 12.2 server's locator header, taken from the capture.
// the meanings aren't known - a version of 2 for a clob or a blob and 1 for
// a bfile, a kind byte, an initialized byte and a sequence of 1 is what the
// bytes look like - and no client has been seen to read them, but a locator
// is the one thing in a row a client hands straight back to the server, so
// it goes out as captured rather than as a guess.
// see "Oracle Wire Protocol - Lob Locator"
#define LOB_LOCATOR_VERSION		2
#define BFILE_LOCATOR_VERSION		1
#define LOB_LOCATOR_KIND		0x0c
#define BFILE_LOCATOR_KIND		0x08
#define LOB_LOCATOR_INIT		0x80
#define LOB_LOCATOR_SEQUENCE		1
#define BFILE_LOCATOR_TYPE2		0x01

// the chunk size that goes out in front of a locator.  the client reads it
// back as OCI_ATTR_CHUNK_SIZE and reads the lob in multiples of it
#define LOB_CHUNK_SIZE			8060

// what a lob operation request asks for.  every one of these was read off a
// real 12.2 exchange except the write side ones, which are only recognized
// so they can be refused cleanly.
// see "Oracle Wire Protocol - Lob Operations"
#define LOB_OP_GET_LENGTH		0x00001
#define LOB_OP_READ			0x00002
#define LOB_OP_WRITE			0x00040
#define LOB_OP_TRIM			0x00080
#define LOB_OP_FILE_OPEN		0x00100
#define LOB_OP_FILE_IS_OPEN		0x00200
#define LOB_OP_FILE_CLOSE		0x00400
#define LOB_OP_FILE_EXISTS		0x00800
#define LOB_OP_FILE_GET_NAME		0x01000
#define LOB_OP_CREATE_TEMPORARY		0x00110
#define LOB_OP_FREE_TEMPORARY		0x00111
#define LOB_OP_IS_OPEN			0x01100
#define LOB_OP_OPEN			0x08000
#define LOB_OP_CLOSE			0x10000

// what a lob operation's answer carries between the locator and the
// summary object: a count prefixed 8 byte value, or nothing at all for a
// close.  a file exists answers with the same count prefixed value as the
// rest, carrying 1 or 0 - measured, a bare boolean byte instead gets
// ORA-03108 from OCI 23.26 and kills the call
#define LOB_RESULT_NONE			0
#define LOB_RESULT_UB8			1

// a lob data packet's fixed parts: the 64 byte descriptor block in front of
// the data, and the more-data flag that says whether another chunk follows.
// the tns length of one of these packets counts the descriptor alone - the
// lob bytes ride after the packet's declared end
#define LOB_DATA_DESCRIPTOR_SIZE	64
#define LOB_DATA_PADDING_SIZE		50
#define LOB_DATA_MORE			2
#define LOB_DATA_LAST			3
#define LOB_DATA_CONSTANT		1

// the bit a lob data packet carries in the packet header's normally unused
// reserved byte - the same one a marker packet sets.  a real 12.2 server
// sets it on every lob data packet it sends
#define LOB_DATA_PACKET_FLAGS		0x20

// a clob's bytes go out two per character, utf-16 big endian, whatever
// charset the session negotiated - a real 12.2 server answers a 400
// character read with 800 bytes
#define LOB_CLOB_BYTES_PER_CHAR		2

// what a number with no declared scale is described as, which tells the
// client not to rescale the value.  the controller hands oracle's scale back
// through an unsigned byte, so -127 arrives from it as 129.
#define NO_SCALE			(-127)
#define NO_SCALE_UNSIGNED		129

// character set ids
#define CHARSET_US7ASCII		1
#define CHARSET_WE8MSWIN1252		178
#define CHARSET_AL32UTF8		873
#define CHARSET_AL16UTF16		2000

// options
#define OPTION_PARSE		(1<<0)
#define OPTION_BIND		(1<<3)
#define OPTION_DEFINE		(1<<4)
#define OPTION_EXECUTE		(1<<5)
#define OPTION_FETCH		(1<<6)
#define OPTION_CANCEL		(1<<7)
#define OPTION_COMMIT		(1<<8)
#define OPTION_EXACTFETCH	(1<<9)
#define OPTION_SNDIOV		(1<<10)
#define OPTION_NOPLSQL		(1<<15)
// a describe-only execute sets this instead of OPTION_EXECUTE
#define OPTION_DESCRIBE		(1<<17)

// ano field types.  no source names 0 or 4.
#define ANO_FIELD_TYPE_STRING		0
#define ANO_FIELD_TYPE_RAW_BYTES	1
#define ANO_FIELD_TYPE_UB1		2
#define ANO_FIELD_TYPE_UB2		3
#define ANO_FIELD_TYPE_UB4		4
#define ANO_FIELD_TYPE_VERSION		5
#define ANO_FIELD_TYPE_STATUS		6

// ano service ids
#define ANO_SERVICE_AUTHENTICATION	1
#define ANO_SERVICE_ENCRYPTION		2
#define ANO_SERVICE_DATA_INTEGRITY	3
#define ANO_SERVICE_SUPERVISOR		4

// ano status values.  each reports success, and each is the only value
// that's been seen for its service.
#define ANO_STATUS_SUPERVISOR_OK	0x001f
#define ANO_STATUS_AUTHENTICATION_OK	0xfbff

// ano markers.  the deadbeef marker heads the request and response, and
// heads a ub2 array inside a raw bytes field, where the array marker
// follows it.
#define ANO_MARKER			0xdeadbeef
#define ANO_ARRAY_MARKER		0x0003

// column types
#define ORACLE_TYPE_VARCHAR		1
#define ORACLE_TYPE_NUMBER		2
#define ORACLE_TYPE_VARNUM		6
#define ORACLE_TYPE_LONG		8
#define ORACLE_TYPE_ROWID_DEPRECATED	11
#define ORACLE_TYPE_DATE		12
#define ORACLE_TYPE_RAW			23
#define ORACLE_TYPE_LONG_RAW		24
#define ORACLE_TYPE_CHAR		96
#define ORACLE_TYPE_RESULT_SET		102
#define ORACLE_TYPE_ROWID		104
#define ORACLE_TYPE_NAMED_TYPE		109
#define ORACLE_TYPE_REF_TYPE		111
#define ORACLE_TYPE_CLOB		112
#define ORACLE_TYPE_BLOB		113
#define ORACLE_TYPE_BFILE		114
#define ORACLE_TYPE_TIMESTAMP		180
#define ORACLE_TYPE_TIMESTAMPTZ		181
#define ORACLE_TYPE_INTERVALYM		182
#define ORACLE_TYPE_INTERVALDS		183
#define ORACLE_TYPE_TIMESTAMPLTZ	231
// internal sentinels, not wire codes - the describe info and column
// definition type fields are both ub1, and getWireColumnType() folds these
// two away before anything is written
#define ORACLE_TYPE_PLSQL_INDEX_TABLE	998
#define ORACLE_TYPE_FIXED_CHAR		999

// and three more, for the three lob types when the column really is a lob.
// a column only reaches these once getLobColumnType() has established that
// the backend is oracle and that the column is one of its own lob types -
// a blob is what the type map folds a good many unrelated types onto as a
// catch-all, from a bytea to a json to a postgresql array
#define ORACLE_TYPE_LOB_CLOB		995
#define ORACLE_TYPE_LOB_BLOB		996
#define ORACLE_TYPE_LOB_BFILE		997


static uint16_t	oracletypemap[]={
	// "UNKNOWN"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// added by freetds
	// "CHAR"
	(uint16_t)ORACLE_TYPE_CHAR,
	// "INT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "SMALLINT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "TINYINT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "MONEY"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "DATETIME"
	(uint16_t)ORACLE_TYPE_TIMESTAMP,
	// "NUMERIC"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "DECIMAL"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "SMALLDATETIME"
	(uint16_t)ORACLE_TYPE_TIMESTAMP,
	// "SMALLMONEY"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "IMAGE"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BINARY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BIT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REAL"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "FLOAT"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "TEXT"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "VARCHAR"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "VARBINARY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LONGCHAR"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LONGBINARY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LONG"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "ILLEGAL"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "SENSITIVITY"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "BOUNDARY"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "VOID"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "USHORT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// added by lago
	// "UNDEFINED"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "DOUBLE"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "DATE"
	(uint16_t)ORACLE_TYPE_DATE,
	// "TIME"
	(uint16_t)ORACLE_TYPE_DATE,
	// "TIMESTAMP"
	(uint16_t)ORACLE_TYPE_TIMESTAMP,
	// added by msql
	// "UINT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "LASTREAL"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// added by oracle
	// "STRING"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "VARSTRING"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "LONGLONG"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "MEDIUMINT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "YEAR"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "NEWDATE"
	(uint16_t)ORACLE_TYPE_DATE,
	// "NULL"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "ENUM"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "SET"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TINYBLOB"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "MEDIUMBLOB"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LONGBLOB"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BLOB"
	(uint16_t)ORACLE_TYPE_BLOB,
	// added by oracle
	// "VARCHAR2"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "NUMBER"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "ROWID"
	(uint16_t)ORACLE_TYPE_ROWID,
	// "RAW"
	(uint16_t)ORACLE_TYPE_RAW,
	// "LONG_RAW"
	(uint16_t)ORACLE_TYPE_LONG_RAW,
	// "MLSLABEL"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "CLOB"
	(uint16_t)ORACLE_TYPE_CLOB,
	// "BFILE"
	(uint16_t)ORACLE_TYPE_BFILE,
	// added by odbc
	// "BIGINT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "INTEGER"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "LONGVARBINARY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LONGVARCHAR"
	(uint16_t)ORACLE_TYPE_BLOB,
	// added by db2
	// "GRAPHIC"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "VARGRAPHIC"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LONGVARGRAPHIC"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "DBCLOB"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "DATALINK"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "USER_DEFINED_TYPE"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "SHORT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "TINY"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// added by firebird
	// "D_FLOAT"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "QUAD"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INT64"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "DOUBLE PRECISION"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// added by postgresql
	// "BOOL"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "BYTEA"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "NAME"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "INT8"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "INT2"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "INT2VECTOR"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INT4"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REGPROC"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "OID"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "TID"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "XID"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "CID"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "OIDVECTOR"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "SMGR"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "POINT"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "LSEG"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "PATH"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "BOX"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "POLYGON"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "LINE"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "LINE_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "FLOAT4"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "FLOAT8"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "ABSTIME"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "RELTIME"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "TINTERVAL"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "CIRCLE"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "CIRCLE_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "MONEY_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "MACADDR"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "INET"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "CIDR"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "BOOL_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BYTEA_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "CHAR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "NAME_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INT2_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INT2VECTOR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INT4_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "REGPROC_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TEXT_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "OID_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TID_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "XID_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "CID_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "OIDVECTOR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BPCHAR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "VARCHAR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INT8_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "POINT_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LSEG_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "PATH_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BOX_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "FLOAT4_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "FLOAT8_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "ABSTIME_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "RELTIME_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TINTERVAL_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "POLYGON_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "ACLITEM"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "ACLITEM_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "MACADDR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INET_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "CIDR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BPCHAR"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "TIMESTAMP_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "DATE_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TIME_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TIMESTAMPTZ"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "TIMESTAMPTZ_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "INTERVAL"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "INTERVAL_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "NUMERIC_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TIMETZ"
	(uint16_t)ORACLE_TYPE_TIMESTAMPTZ,
	// "TIMETZ_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BIT_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "VARBIT"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "VARBIT_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "REFCURSOR"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REFCURSOR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "REGPROCEDURE"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REGOPER"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REGOPERATOR"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REGCLASS"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REGTYPE"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "REGPROCEDURE_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "REGOPER_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "REGOPERATOR_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "REGCLASS_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "REGTYPE_ARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "RECORD"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "CSTRING"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "ANY"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "ANYARRAY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "TRIGGER"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "LANGUAGE_HANDLER"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "INTERNAL"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "OPAQUE"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "ANYELEMENT"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "PG_TYPE"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "PG_ATTRIBUTE"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "PG_PROC"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "PG_CLASS"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// none added by sqlite
	// added by sqlserver
	// "UBIGINT"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "UNIQUEIDENTIFIER"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// added by informix
	// "SMALLFLOAT"
	(uint16_t)ORACLE_TYPE_VARNUM,
	// "BYTE"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "BOOLEAN"
	(uint16_t)ORACLE_TYPE_NUMBER,
	// "TINYTEXT"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "MEDIUMTEXT"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "LONGTEXT"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "JSON"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "GEOMETRY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "SDO_GEOMETRY"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "NCHAR"
	(uint16_t)ORACLE_TYPE_CHAR,
	// "NVARCHAR"
	(uint16_t)ORACLE_TYPE_VARCHAR,
	// "NTEXT"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "XML"
	(uint16_t)ORACLE_TYPE_BLOB,
	// "DATETIMEOFFSET"
	(uint16_t)ORACLE_TYPE_TIMESTAMP,
	// "LVARCHAR"
	(uint16_t)ORACLE_TYPE_VARCHAR
};

enum oraclelisttype_t {
	ORACLELISTTYPE_DATABASE_LIST=0,
	ORACLELISTTYPE_TABLE_LIST,
	ORACLELISTTYPE_COLUMN_LIST
};

// what a query3 request's bind section says about one placeholder.  the
// descriptor carries no name and no position - binds are strictly positional,
// in the order the placeholders appear in the query text
struct oraclequery3bind {
	byte_t		type;
	byte_t		flags;
	uint32_t	buffersize;
	// BIND_DIRECTION_*, worked out from the statement rather than read
	// off the wire
	byte_t		direction;
	// which of the cursor's output binds this descriptor filled, or -1
	int16_t		outindex;
};

// one placeholder's value for one execution iteration.  the bytes are left
// where they are in the request packet, which outlives the call reading it
struct oraclequery3bindvalue {
	const byte_t	*value;
	uint32_t	size;
	bool		isnull;
};

class SQLRSERVER_DLLSPEC sqlrprotocol_oracle : public sqlrprotocol {
	public:
		sqlrprotocol_oracle(sqlrservercontroller *cont,
							domnode *parameters);
		virtual	~sqlrprotocol_oracle();

		clientsessionexitstatus_t	clientSession(
							filedescriptor *cs);

	private:
		void	init();
		void	free();
		void	reInit();

		void	resetSendPacketBuffer(byte_t packettype);
		bool	sendPacket();
		bool	sendPacket(bool flush);
		bool	recvPacket();
		void	readHost(const byte_t *rp,
					uint16_t *value,
					const byte_t **rpout);
		void	readHost(const byte_t *rp,
					uint32_t *value,
					const byte_t **rpout);
		bool	readMarker16(const byte_t *rp,
					uint16_t expected,
					const byte_t **rpout);
		bool	readMarker32(const byte_t *rp,
					uint32_t expected,
					const byte_t **rpout);
		bool	getNullTerminatedArray(const byte_t *rp,
						const byte_t *end,
						byte_t **array,
						uint32_t *arraycount,
						const byte_t **rpout);
		bool	getString(const byte_t *rp,
						const byte_t *end,
						char **string,
						const byte_t **rpout);
		bool	getString(const byte_t *rp,
						char **string,
						uint32_t size,
						const byte_t **rpout);

		void	writeHost(bytebuffer *buffer, uint16_t value);
		void	writeHost(bytebuffer *buffer, uint32_t value);

		char	*generateHex(uint16_t bytes);


		// handshake...
		bool	initialHandshake();
		bool	connect();
		bool	recvConnectRequest();
		const char	*findConnectDataKey(const char *haystack,
							const char *key);
		bool	requestedServiceKnown();
		bool	sendConnectResponse();
		bool	sendAccept();
		bool	sendAccept(const byte_t *data, uint16_t datasize);
		bool	sendResend();
		bool	sendRefuse(uint32_t tnserror);
		bool	sendMarker(byte_t markertype);

		bool	anoNegotiation();
		bool	recvAnoRequest();
		bool	anoBoundsCheck(const byte_t *rp,
						const byte_t *end,
						size_t size,
						const char *name);
		bool	getAnoServiceHeader(const byte_t *rp,
						const byte_t *end,
						uint16_t *service,
						uint16_t *fieldcount,
						const byte_t **rpout);
		bool	getSupervisorService(const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout);
		bool	getAuthenticationService(const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout);
		bool	getEncryptionService(const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout);
		bool	getDataIntegrityService(const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout);
		bool	getAnoVersionField(const byte_t *rp,
						const byte_t *end,
						uint32_t *version,
						const byte_t **rpout);
		bool	getAnoConnectionInfoField(const byte_t *rp,
						const byte_t *end,
						uint32_t *pid,
						uint32_t *connectiontype,
						const byte_t **rpout);
		bool	getAnoArrayField(const byte_t *rp,
						const byte_t *end,
						uint16_t **array,
						uint32_t *arraycount,
						const byte_t **rpout);
		bool	getAnoDriverListField(const byte_t *rp,
						const byte_t *end,
						uint16_t **drivers,
						uint32_t *drivercount,
						const byte_t **rpout);
		bool	getAnoConstantField(const byte_t *rp,
						const byte_t *end,
						uint16_t *constant,
						const byte_t **rpout);
		bool	getAnoConstantField(const byte_t *rp,
						const byte_t *end,
						byte_t *constant,
						const byte_t **rpout);
		bool	getAnoStatusField(const byte_t *rp,
						const byte_t *end,
						uint16_t *status,
						const byte_t **rpout);
		void		warnAnoDeclined();
		uint32_t	anoDriversOffered(uint16_t *drivers,
							uint32_t drivercount);
		bool		sendAnoResponse();
		uint16_t	putSupervisorService();
		uint16_t	putAuthenticationService();
		uint16_t	putEncryptionService();
		uint16_t	putDataIntegrityService();
		uint16_t	putAnoServiceHeader(uint16_t type,
							uint16_t fieldcount);
		uint16_t	putAnoVersionField(uint32_t version);
		uint16_t	putAnoStatusField(uint16_t status);
		uint16_t	putAnoConstant(byte_t constant);
		uint16_t	putAnoArrayField(uint16_t *array,
							uint32_t arraycount);

		bool	ttiNegotiation();
		bool	recvTtiRequest();
		bool	sendTtiResponse();
		void	putTtiResponse(byte_t version,
						const byte_t *compilecaps,
						byte_t compilecapssize,
						const byte_t *runtimecaps,
						byte_t runtimecapssize);
		void	putTti6Response();
		void	putTti5Response();

		bool	dataTypeNegotiation();
		bool	recvDataTypeRequest();
		bool	getCapabilities(const byte_t *rp,
						const byte_t *end,
						const byte_t **caps,
						byte_t *capssize,
						const byte_t **rpout);
		uint16_t	countDataTypes(const byte_t *rp,
						const byte_t *end,
						uint16_t *multirepcount);
		bool	sendDataTypeResponse();

		bool	authenticate();
		void	resetLoginAttempt();
		bool	getLenString(const byte_t *rp,
						const byte_t *end,
						char **string,
						uint32_t *size,
						const byte_t **rpout);
		bool	getAuthField(const byte_t *rp,
						const byte_t *end,
						char **fieldname,
						char **field,
						uint32_t *fieldsize,
						uint32_t *flags,
						const byte_t **rpout);
		bool	getPointer(const byte_t *rp,
						const byte_t *end,
						byte_t *value,
						const byte_t **rpout);
		bool	getAuthPointer(const byte_t *rp,
						const byte_t *end,
						const byte_t **rpout);
		bool	getAuthCount(const byte_t *rp,
						const byte_t *end,
						uint32_t *value,
						byte_t nativesize,
						const byte_t **rpout);
		void	putAuthCount(uint32_t value, byte_t nativesize);
		void	putLenString(const char *string, uint32_t size);
		void	putLenBytes(const char *bytes, uint32_t size);
		bool	getLenBytes(const byte_t *rp,
						const byte_t *end,
						const byte_t **bytes,
						uint32_t *size,
						bool *isnull,
						const byte_t **rpout);
		void	putDalc(const char *bytes, uint32_t size);
		bool	getOracleDate(const char *field,
						uint64_t fieldsize,
						byte_t *out);
		void	putOracleDate(byte_t *out);
		void	putOracleDate(byte_t *out,
						int16_t year,
						int16_t month,
						int16_t day,
						int16_t hour,
						int16_t minute,
						int16_t second);
		void	putAuthField(const char *fieldname,
						const char *field,
						uint32_t flags);
		void	putAuthField(const char *fieldname, const char *field);
		void	putAuthTrailer(const byte_t *portable,
						size_t portablesize,
						bool secondphase);
		void	putAuthExtra(stringbuffer *extra, bool secondphase);
		bool	recvAuthenticationRequest(bool secondphase);
		bool	sendAuthenticationChallenge();
		bool	sendAuthenticationResponse();
		bool	sendErrorPacket(const char *what,
						uint32_t oranum,
						const char *message);
		bool	sendAuthenticationError(uint32_t oranum,
						const char *message);

		void	debugTtcCode(byte_t ttccode);
		void	debugTtiFunction(byte_t ttifunction);
		void	debugOptions(uint16_t options, uint16_t moreoptions);
		void	debugOptions(uint32_t options);
		void	debugCharacterSet(byte_t characterset);
		void	debugStatusFlags(uint16_t statusflags);
		void	debugColumnType(const char *name, uint16_t columntype);
		void	debugColumnType(uint16_t columntype);
		void	debugSystemError();

		bool	getTtiFunction(const byte_t *rp,
						byte_t *ttifunction,
						const byte_t **rpout);

		// open...
		bool	open(const byte_t *rp);
		bool	sendOpenResponse(sqlrservercursor *cursor);

		// query...
		bool	query(const byte_t *rp);
		bool	sendQueryResponse(sqlrservercursor *cursor);
		bool	query2(const byte_t *rp);
		bool	sendQuery2Response(sqlrservercursor *cursor,
							bool binds);
		bool	bindParameters(sqlrservercursor *cursor,
							uint16_t pcount,
							uint16_t *ptypes);
		bool	query3(const byte_t *rp);
		bool	getQuery3Request(const byte_t *rp,
							const byte_t *end,
							uint32_t *options,
							uint32_t *cursorid,
							uint32_t *prefetchrows,
							uint32_t *maxlongsize,
							const char **query,
							uint32_t *querysize);
		bool	getQuery3Binds(const byte_t *rp,
							const byte_t *end,
							uint32_t vectorsize,
							uint32_t bindcount,
							uint32_t definecount,
							uint32_t options,
							const char *query,
							uint32_t querysize);
		bool	getQuery3BindDescriptor(const byte_t *rp,
							const byte_t *end,
							byte_t *type,
							byte_t *flags,
							uint32_t *buffersize,
							const byte_t **rpout);
		void	classifyQuery3Binds(uint32_t options,
							const char *query,
							uint32_t querysize);
		bool	hasQuery3OutBinds();
		bool	getQuery3BindValues(const byte_t *rp,
							const byte_t *end,
							uint32_t bindcount,
							uint32_t iterations);
		void	saveQuery3Binds(sqlrservercursor *cursor);
		void	restoreQuery3Binds(sqlrservercursor *cursor);
		bool	installQuery3Binds(sqlrservercursor *cursor,
							uint32_t block);
		bool	getBindVariableName(const char *query,
							uint32_t querysize,
							uint16_t index,
							const char **name,
							uint16_t *namesize);
		bool	sendNotAllVariablesBoundError(uint32_t cursorid);
		bool	sendQuery3Response(sqlrservercursor *cursor,
							uint32_t options,
							uint32_t cursorid,
							uint32_t prefetchrows,
							uint32_t maxlongsize);
		bool	hasLongColumn(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putIoVector();
		void	putOutBindValues(sqlrservercursor *cursor);
		void	putRefCursorBindValue(sqlrservercursor *child);
		bool	fetchFromRefCursors(sqlrservercursor *cursor,
						sqlrservercursor **failed);
		void	releaseRefCursors(uint16_t parentid);
		void	forgetRefCursor(uint16_t childid);
		void	putDescribeInfo(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putDescribeInfoBody(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putColumnMetadata(sqlrservercursor *cursor,
							uint32_t column);
		void	putColumnPrecisionScale(int8_t precision,
							int8_t scale);
		uint16_t	getWireColumnType(uint16_t columntype);
		bool		isCharacterColumn(
							const char *columntypestring,
							uint16_t columntype);
		uint32_t	getWireColumnSize(sqlrservercursor *cursor,
							uint32_t column,
							const char *columntypestring,
							uint16_t columntype,
							uint16_t wiretype);
		void	putRowHeader(byte_t flags,
							uint32_t colcount,
							uint32_t prefetchrows);
		void	putRowData(sqlrservercursor *cursor,
							uint32_t colcount);
		bool	isLobColumnType(uint16_t columntype);
		bool	hasLobColumn(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putLenPreUB8(uint64_t value);
		uint32_t	buildLobLocator(sqlrservercursor *cursor,
							uint32_t column,
							uint16_t wiretype,
							byte_t *locator);
		void	putLobLocator(sqlrservercursor *cursor,
							uint32_t column,
							uint16_t wiretype,
							bool null);
		void	pinLobRow(sqlrservercursor *cursor,
							uint32_t colcount);
		void	releaseLobPin(sqlrservercursor *cursor);
		void	clearLobPin(uint16_t curid);

		// lob operations...
		bool	lobOperations(const byte_t *rp);
		bool	readLenPreUB8(const byte_t *rp,
							const byte_t *end,
							uint64_t *value,
							const byte_t **rpout);
		bool	decodeLobLocator(const byte_t *locator,
							uint32_t locatorsize,
							sqlrservercursor **cursor,
							uint32_t *column,
							uint16_t *wiretype);
		uint32_t	putUtf16Chars(const char *in,
							uint64_t insize,
							uint64_t chars,
							byte_t *out);
		bool	discardLobDataPacket();
		bool	sendLobDataMarker();
		bool	sendLobDataChunk(const byte_t *data,
							uint32_t size,
							bool last);
		bool	sendLobReadResponse(sqlrservercursor *cursor,
							uint32_t column,
							uint16_t wiretype,
							const byte_t *locator,
							uint32_t locatorsize,
							uint64_t offset,
							uint64_t amount);
		bool	sendLobOperationResponse(const byte_t *locator,
							uint32_t locatorsize,
							byte_t resulttype,
							uint64_t result);
		bool	sendLobOperationError(uint32_t oranum,
							const char *message);
		void	putReturnParameters();
		void	putSummary(uint32_t cursorid,
							uint32_t oranum,
							uint32_t rowcount,
							const char *message);
		void	putSummary(uint32_t cursorid,
							uint32_t oranum,
							uint32_t rowcount,
							const char *message,
							uint32_t messagesize);
		void	putSummaryExtension(uint32_t oranum,
							uint32_t rowcount);
		void	putNumberField(const char *field,
							uint32_t fieldsize);
		bool	getNumberField(const byte_t *bytes,
							uint32_t size,
							char *out,
							uint32_t outsize,
							uint32_t *outlen);
		bool	putRowidField(const char *field,
							uint64_t fieldsize);
		int16_t	getRowidDigit(char c);
		bool	putRawField(const char *field,
							uint64_t fieldsize,
							bool longraw);
		int16_t	getRawDigit(char c);
		void	putLongBytes(const char *bytes, uint32_t size);
		void	putNullLongField();
		bool	putIntervalField(const char *field,
							uint64_t fieldsize,
							bool daytosecond);
		bool	getIntervalNumber(const char **f,
							const char *end,
							char separator,
							uint32_t *value);
		void	putIntervalLeading(byte_t *out,
							uint32_t value,
							bool negative);
		bool	putTimestampField(const char *field,
							uint64_t fieldsize,
							bool withtimezone);
		uint16_t	getTimestampDigits(const char **f,
							const char *end,
							uint32_t *value);

		// execute...
		bool	execute(const byte_t *rp);
		bool	reexecute(const byte_t *rp);
		bool	sendReexecuteResponse(sqlrservercursor *cursor,
							uint32_t cursorid);
		bool	sendExecuteResponse(sqlrservercursor *cursor);

		// fetch...
		bool	fetch(const byte_t *rp);
		bool	fetch3(const byte_t *rp);
		bool	sendFetch3Response(sqlrservercursor *cursor,
							uint32_t cursorid,
							uint32_t rowstofetch);
		bool	sendFetchResponse(sqlrservercursor *cursor,
							bool parse,
							bool define,
							bool sndiov,
							bool exactfetch);
		void	cacheColumnDefinitions(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putColumnDefinitions(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putColumnDefinition(sqlrservercursor *cursor,
							uint32_t column);
		uint16_t	getColumnType(const char *columntypestring,
						uint16_t columntypesize,
						uint32_t scale);
		uint16_t	getUnknownColumnType(
						sqlrservercursor *cursor,
						uint32_t column,
						uint16_t columntype);
		uint16_t	getLongColumnType(
						sqlrservercursor *cursor,
						uint32_t column,
						uint16_t columntype);
		uint16_t	getLobColumnType(
						sqlrservercursor *cursor,
						uint32_t column,
						uint16_t columntype);
		uint16_t	getColumnFlags(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t sqlrcolumntype,
						uint16_t columntype,
						const char *columntypestring);
		uint16_t	getColumnFlags(sqlrservercursor *cursor,
						uint16_t sqlrcolumntype,
						uint16_t columntype,
						const char *columntypestring,
						bool isnullable,
						bool isprimarykey,
						bool isunique,
						bool ispartofkey,
						bool isunsigned,
						bool iszerofilled,
						bool isbinary,
						bool isautoincrement);
		void	putIov();
		bool	putRow(sqlrservercursor *cursor,
						uint32_t colcount,
						bool terminator);
		bool	putField(const char *field,
						uint64_t fieldsize,
						uint16_t columntype);
		void	putLobField(sqlrservercursor *cursor, uint32_t col);
		void	putError(const char *error,
					uint32_t oranum=ORA_NO_DATA_FOUND);
		void	putError(const char *error, uint32_t errorsize,
					uint32_t oranum);

		// close...
		bool	close(const byte_t *rp);
		void	clearParams(sqlrservercursor *cursor);
		bool	sendCloseResponse(sqlrservercursor *cursor);

		// disconnect...
		bool	disconnect(const byte_t *rp);
		bool	sendDisconnectResponse();

		// commit, rollback, autocommit...
		bool	commit(const byte_t *rp);
		bool	rollback(const byte_t *rp);
		bool	autoCommitOn(const byte_t *rp);
		bool	autoCommitOff(const byte_t *rp);
		bool	sendTransactionResponse();
		bool	sendTransactionError(uint32_t cursorid=0);

		// version
		bool	version(const byte_t *rp, bool istticall);
		bool	sendVersionResponse(uint32_t bufferlength);

		// occa
		bool	occa(const byte_t *rp, const byte_t **rpout);
		bool	switchSession(const byte_t *rp,
						const byte_t **rpout);

		// the unidentified 0x54 call
		bool	unidentified54(const byte_t *rp);
		bool	sendUnidentified54Response();

		void	putGenericFooter();

		bool	sendQueryError(sqlrservercursor *cursor);
		bool	sendCursorNotOpenError(uint32_t cursorid=0);
		bool	sendUnimplementedFunctionError();
		bool	sendMarkerCancelError();

		uint16_t	connectversion;
		uint16_t	connectlowestversion;
		uint16_t	gso;
		uint16_t	anoflags;

		uint32_t	sdu;
		uint32_t	tdu;

		// whether the tns packet header carries a 32-bit length
		bool		largeheader;

		uint32_t	anorequestversion;
		uint32_t	supervisorversion;
		uint32_t	authenticationversion;
		uint32_t	encryptionversion;
		uint32_t	dataintegrityversion;

		uint16_t	*encryptiondrivers;
		uint32_t	encryptiondrivercount;
		uint16_t	*dataintegritydrivers;
		uint32_t	dataintegritydrivercount;

		byte_t		*ttiversions;
		uint32_t	ttiversioncount;
		byte_t		ttiversion;

		char		*clientstring;
		const char	*serverstring;

		uint16_t	charset;
		uint16_t	nationalcharset;
		uint32_t	verifiertype;

		// the SID/SERVICE_NAME(s) this listener answers to, from the "sid"
		// listener attribute, comma-separated; NULL means accept anything
		char		*sids;

		// how many logins one connection may fail before it's
		// dropped, from the "maxloginattempts" listener attribute
		uint16_t	maxloginattempts;

		// the SID/SERVICE_NAME the client's CONNECT_DATA asked for, parsed
		// out by recvConnectRequest(); NULL if it didn't name one
		char		*requestedservice;

		// the oracle version the module imitates
		byte_t		serverfieldversion;
		const char	*serverversionno;
		const char	*serverversionsql;
		uint32_t	serverversionpacked;
		char		serverversionbanner[128];

		// whether the client marshals in its own memory layout
		bool		nativeencoding;

		// the ttc code the last tti function came in under
		byte_t		lastttccode;

		uint16_t	clientcharsetin;
		uint16_t	clientcharsetout;
		uint16_t	clientnationalcharset;
		byte_t		encodingflags;
		bool		ociclient;
		byte_t		clientfieldversion;
		byte_t		fieldversion;
		bool		clientwantsdbtimezone;
		bool		clientwantstzversion;
		uint32_t	clienttzversion;

		const byte_t	*datatypes;
		uint16_t	datatypessize;
		uint16_t	datatypecount;

		filedescriptor	*clientsock;

		bytebuffer	reqpacket;
		byte_t		reqpackettype;
		byte_t		reqpacketflags;

		memorypool	*resppacketpool;
		byte_t		*resppacket;
		uint32_t	resppacketsize;
		byte_t		resppackettype;

		prng	r;
		//uint32_t	seed;

		char		*username;
		char		*response;
		//uint64_t	responsesize;

		// o5logon...
		char		*authvfrdata;
		char		*authpbkdf2csksalt;
		char		*serverauthsesskey;
		char		*clientauthsesskey;
		char		*authpassword;
		bool		gotauthpassword;
		bool		fabricatedchallenge;

		// whether the login was refused, as opposed to the exchange
		// failing some other way, which decides whether another
		// login may be attempted on the same connection
		bool		loginrefused;

		uint16_t	maxcursorcount;
		uint32_t	maxquerysize;
		uint16_t	maxbindcount;
		uint32_t	maxstringbindvaluesize;

		char		**bindvarnames;

		char		lobbuffer[32768];

		uint16_t	**ptypes;
		bool		*columntypescached;
		uint16_t	**columntypes;

		// how many rows of each cursor's result set have gone out,
		// since a fetch's summary has to carry the running total
		uint32_t	*rowssent;

		// a row already fetched and formatted, but not sent because
		// it didn't fit in the current packet.  the connection has
		// already advanced past it - fetchRow()/nextRow() can't
		// un-fetch a row on every backend - so it's held here and
		// sent first on the next fetch/prefetch instead of being
		// re-fetched
		bytebuffer	*pendingrow;

		// the row a lob locator was sent for.  the controller's lob
		// calls only ever address a cursor's current row, so the row a
		// locator points at has to stay current until the client is
		// done with it - the connection is held back a row instead of
		// being advanced past it.  see releaseLobPin()
		bool		*lobpinned;
		uint32_t	*lobpincolcount;

		// bumped every time a pin comes off, and carried in every
		// locator, so one echoed back after its row has gone can be
		// told from one that's still good
		uint16_t	*lobpingeneration;

		// whether the row putRowData() just wrote handed out a locator
		bool		rowhaslob;

		// the sequence number the summary object has to echo back
		byte_t		callnumber;

		bool		query3session;

		// the bind section of the query3 request being handled: one
		// descriptor per placeholder in the statement, and one value
		// per placeholder per execution iteration
		oraclequery3bind	*query3binds;
		uint32_t		query3bindavail;
		uint32_t		query3binddescs;
		oraclequery3bindvalue	*query3bindvalues;
		uint32_t		query3bindvalueavail;
		uint32_t		query3blocks;

		// each cursor's descriptors from its last full execute.  a
		// re-execute sends fresh values but no descriptors, so the
		// ones that came with the statement have to be kept
		oraclequery3bind	**cursorbinds;
		uint32_t		*cursorbindcounts;

		// the cursors each statement's ref cursor binds are holding.
		// one comes out of the pool per bind at execute time and
		// nothing else gives it back, so the statement that took it
		// has to remember it
		uint16_t		**refcursorids;
		uint16_t		*refcursorcounts;

		// whether the statement has a placeholder the client never
		// bound anything to, which is an ORA-01008 rather than an
		// execute
		bool		query3unbound;

		// the rows the last query3 execute affected, summed over its
		// iterations, and whether the backend knew the count.  an
		// array bind runs the statement once per iteration, and the
		// summary object has to report the total
		uint32_t	query3affectedrows;
		bool		query3knowsaffectedrows;

		// the cursor id most recently touched by open(), query(),
		// query2(), query3() or execute() - two calls fall back to
		// this instead of the id on the wire: the legacy (non-query3)
		// Fetch request carries no cursor id of its own, and execute()
		// doesn't branch by session, so it misparses a query3
		// session's LPI-encoded request as the legacy layout and its
		// own parsed id isn't trustworthy there
		uint16_t	lastcursorid;
};

sqlrprotocol_oracle::sqlrprotocol_oracle(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrprotocol(cont,parameters) {

	clientsock=NULL;

	// the SID/SERVICE_NAME(s) this listener presents itself as, so an
	// attach naming anything else can be refused with ORA-12514; unset
	// (the pre-existing default) accepts whatever the client asks for
	const char	*sidattr=parameters->getAttributeValue("sid");
	sids=(charstring::isNullOrEmpty(sidattr))?
				NULL:charstring::duplicate(sidattr);
	requestedservice=NULL;

	// how many times a client may fail to log in before the connection is
	// dropped.  3 is what a real server allows by default, its
	// SEC_MAX_FAILED_LOGIN_ATTEMPTS.
	maxloginattempts=(uint16_t)charstring::convertToUnsignedInteger(
			parameters->getAttributeValue("maxloginattempts"));
	if (!maxloginattempts) {
		maxloginattempts=3;
	}

	charset=charstring::convertToInteger(
				parameters->getAttributeValue("charset"));
	if (!charset) {
		charset=CHARSET_AL32UTF8;
	}

	nationalcharset=charstring::convertToInteger(
				parameters->getAttributeValue(
						"nationalcharset"));
	if (!nationalcharset) {
		nationalcharset=CHARSET_AL16UTF16;
	}

	// which oracle version the module imitates.  everything a client picks
	// a reader from moves with it: the capability field version, the shape
	// of every summary object, the version the module reports, and the
	// default verifier type.  answering one client as one version and
	// another as another is what breaks the login and the first query.
	//
	// no one version serves every client.  python-oracledb and
	// node-oracledb don't support a server older than 12.1 and read the
	// two extra fields of a 12.1 summary object unconditionally, OCI 23.26
	// in the portable encoding reads the 11.2 shape and nothing else, and
	// ojdbc 23.26 works either way.  so 12.1 is the default, and 11.2 is
	// there for a deployment whose clients are OCI.
	const char	*sv=parameters->getAttributeValue("serverversion");
	if (!charstring::compare(sv,"11.2")) {
		serverfieldversion=CCAP_FIELD_VERSION_11_2;
		serverversionno=SERVER_VERSION_NO_11_2;
		serverversionsql=SERVER_VERSION_SQL_11_2;
	} else {
		serverfieldversion=CCAP_FIELD_VERSION_12_1;
		serverversionno=SERVER_VERSION_NO_12_1;
		serverversionsql=SERVER_VERSION_SQL_12_1;
	}

	// build the version response's banner from that version.  the nibbles
	// of AUTH_VERSION_NO are the version: major in bits 31-24, minor in
	// 23-20, the third component in 15-12, the patchset in 11-8, and the
	// fifth component in 7-0.  the other bits are zero.
	serverversionpacked=(uint32_t)
			charstring::convertToUnsignedInteger(serverversionno);
	int	major=(int)((serverversionpacked>>24)&0xff);
	int	minor=(int)((serverversionpacked>>20)&0x0f);
	int	third=(int)((serverversionpacked>>12)&0x0f);
	int	patch=(int)((serverversionpacked>>8)&0x0f);
	int	fifth=(int)(serverversionpacked&0xff);
	charstring::printf(serverversionbanner,sizeof(serverversionbanner),
			"Oracle Database %d%s Enterprise Edition "
			"Release %d.%d.%d.%d.%d - 64bit Production",
			major,(major>=12)?"c":"g",
			major,minor,third,patch,fifth);

	// which o5logon verifier type to offer
	// (a 12c verifier can't exist on an 11.2 database, so the default
	// follows the version the module reports; an explicit setting wins)
	uint32_t	defaultverifiertype=
			(serverfieldversion<CCAP_FIELD_VERSION_12_1)?
					VERIFIER_TYPE_11G_2:VERIFIER_TYPE_12C;
	const char	*vt=parameters->getAttributeValue("verifiertype");
	if (!charstring::compare(vt,"11g")) {
		verifiertype=VERIFIER_TYPE_11G_2;
	} else if (!charstring::compare(vt,"12c")) {
		verifiertype=VERIFIER_TYPE_12C;
	} else if (charstring::isNullOrEmpty(vt)) {
		verifiertype=defaultverifiertype;
	} else {
		verifiertype=(uint32_t)charstring::convertToUnsignedInteger(vt);
		if (verifiertype!=VERIFIER_TYPE_11G_1 &&
			verifiertype!=VERIFIER_TYPE_11G_2 &&
			verifiertype!=VERIFIER_TYPE_12C) {
			verifiertype=defaultverifiertype;
		}
	}

	if (getDebug()) {
		debugStart("parameters");
		debugWrite("charset: %d",charset);
		debugWrite("nationalcharset: %d",nationalcharset);
		debugWrite("verifiertype: 0x%04x",verifiertype);
		debugWrite("server version: %s",serverversionno);
		debugWrite("server field version: %d",serverfieldversion);
		debugWrite("sid: %s",(sids)?sids:"(any)");
		debugWrite("max login attempts: %d",maxloginattempts);
		debugEnd();
	}

	r.setSeed(prng::getSeed());

	resppacketpool=new memorypool(1024,1024,10240);

	query3binds=NULL;
	query3bindavail=0;
	query3binddescs=0;
	query3bindvalues=NULL;
	query3bindvalueavail=0;
	query3blocks=0;
	query3unbound=false;
	query3affectedrows=0;
	query3knowsaffectedrows=false;

	maxcursorcount=cont->getConfig()->getMaxCursors();
	maxquerysize=cont->getConfig()->getMaxQuerySize();
	maxbindcount=cont->getConfig()->getMaxBindCount();
	maxstringbindvaluesize=
			cont->getConfig()->getMaxStringBindValueSize();

	bindvarnames=new char *[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		charstring::printf(&bindvarnames[i],":%d",i+1);
	}

	ptypes=new uint16_t *[maxcursorcount];
	columntypescached=new bool[maxcursorcount];
	columntypes=new uint16_t *[maxcursorcount];
	rowssent=new uint32_t[maxcursorcount];
	pendingrow=new bytebuffer[maxcursorcount];
	lobpinned=new bool[maxcursorcount];
	lobpincolcount=new uint32_t[maxcursorcount];
	lobpingeneration=new uint16_t[maxcursorcount];
	cursorbinds=new oraclequery3bind *[maxcursorcount];
	cursorbindcounts=new uint32_t[maxcursorcount];
	refcursorids=new uint16_t *[maxcursorcount];
	refcursorcounts=new uint16_t[maxcursorcount];
	for (uint16_t i=0; i<maxcursorcount; i++) {
		ptypes[i]=new uint16_t[maxbindcount];
		columntypescached[i]=false;
		rowssent[i]=0;
		lobpinned[i]=false;
		lobpincolcount[i]=0;
		lobpingeneration[i]=0;
		cursorbinds[i]=new oraclequery3bind[maxbindcount];
		cursorbindcounts[i]=0;
		refcursorids[i]=new uint16_t[maxbindcount];
		refcursorcounts[i]=0;
		if (cont->getMaxColumnCount()) {
			columntypes[i]=new uint16_t[cont->getMaxColumnCount()];
		} else {
			columntypes[i]=NULL;
		}
	}

	init();
}

sqlrprotocol_oracle::~sqlrprotocol_oracle() {
	free();

	delete[] sids;
	delete[] requestedservice;

	for (uint16_t i=0; i<maxbindcount; i++) {
		delete[] bindvarnames[i];
	}
	delete[] bindvarnames;

	for (uint16_t i=0; i<maxcursorcount; i++) {
		delete[] ptypes[i];
		delete[] columntypes[i];
		delete[] cursorbinds[i];
		delete[] refcursorids[i];
	}
	delete[] cursorbinds;
	delete[] cursorbindcounts;
	delete[] refcursorids;
	delete[] refcursorcounts;
	delete[] ptypes;
	delete[] columntypescached;
	delete[] columntypes;
	delete[] rowssent;
	delete[] pendingrow;
	delete[] lobpinned;
	delete[] lobpincolcount;
	delete[] lobpingeneration;

	delete[] query3binds;
	delete[] query3bindvalues;

	delete resppacketpool;
}

void sqlrprotocol_oracle::init() {

	sdu=4086;
	tdu=32767;
	largeheader=false;

	connectversion=0;
	connectlowestversion=0;
	gso=0;
	anoflags=0;

	anorequestversion=0;
	supervisorversion=0;
	authenticationversion=0;
	encryptionversion=0;
	dataintegrityversion=0;

	encryptiondrivers=NULL;
	encryptiondrivercount=0;
	dataintegritydrivers=NULL;
	dataintegritydrivercount=0;

	ttiversions=NULL;
	ttiversioncount=0;
	ttiversion=0;

	clientstring=NULL;
	serverstring=NULL;

	clientcharsetin=0;
	clientcharsetout=0;
	clientnationalcharset=0;
	encodingflags=0;
	ociclient=false;
	clientfieldversion=0;

	// the module's own, until a data type negotiation lowers it to a
	// client's.  it can't start at zero - the authentication exchange
	// writes summary objects before any negotiation happens.
	fieldversion=serverfieldversion;
	clientwantsdbtimezone=false;
	clientwantstzversion=false;
	clienttzversion=0;

	datatypes=NULL;
	datatypessize=0;
	datatypecount=0;

	query3session=false;
	lastcursorid=65535;
	callnumber=0;

	resppacket=NULL;
	resppacketsize=0;
	username=NULL;
	response=NULL;

	authvfrdata=NULL;
	authpbkdf2csksalt=NULL;
	serverauthsesskey=NULL;
	clientauthsesskey=NULL;
	authpassword=NULL;
	gotauthpassword=false;
	fabricatedchallenge=false;
	loginrefused=false;

	nativeencoding=false;
	lastttccode=0;

	rowhaslob=false;
}

void sqlrprotocol_oracle::free() {

	delete[] encryptiondrivers;
	encryptiondrivers=NULL;
	delete[] dataintegritydrivers;
	dataintegritydrivers=NULL;

	delete[] ttiversions;
	ttiversions=NULL;

	delete[] username;
	delete[] response;

	delete[] authvfrdata;
	delete[] authpbkdf2csksalt;
	delete[] serverauthsesskey;
	delete[] clientauthsesskey;
	delete[] authpassword;

	resppacketpool->clear();

	delete[] clientstring;
}

void sqlrprotocol_oracle::reInit() {
	free();
	init();

	// the per-cursor arrays outlive a session, so anything a previous
	// one pinned has to go
	for (uint16_t i=0; i<maxcursorcount; i++) {
		clearLobPin(i);
	}
}

clientsessionexitstatus_t sqlrprotocol_oracle::clientSession(
						filedescriptor *cs) {

	clientsock=cs;

	// set up the socket
	clientsock->setNaglesAlgorithmEnabled(false);
	clientsock->setSocketReadBufferSize(65536);
	clientsock->setSocketWriteBufferSize(65536);
	clientsock->setReadBufferSize(65536);
	clientsock->setWriteBufferSize(65536);

	reInit();

	bool	endsession=true;

	clientsessionexitstatus_t	status=CLIENTSESSIONEXITSTATUS_ERROR;
	if (initialHandshake()) {

		// run session-start queries, now that the client is
		// authenticated
		cont->beginSession();

		// loop, getting and executing commands
		bool		loop=true;
		const byte_t	*rp=NULL;
		do {

			// get the tti function...
			byte_t		ttifunction;
			if (!getTtiFunction(rp,&ttifunction,&rp)) {
				break;
			}

			// do the appropriate thing...
			switch (ttifunction) {
				case TTI_OPEN:
					loop=open(rp);
					rp=NULL;
					break;
				case TTI_QUERY:
					loop=query(rp);
					rp=NULL;
					break;
				case TTI_QUERY2:
					loop=query2(rp);
					rp=NULL;
					break;
				case TTI_QUERY3:
					loop=query3(rp);
					rp=NULL;
					break;
				case TTI_EXECUTE:
					loop=execute(rp);
					rp=NULL;
					break;
				case TTI_FETCH:
					loop=fetch(rp);
					rp=NULL;
					break;
				case TTI_CLOSE:
				case TTI_CLOSE2:
					loop=close(rp);
					rp=NULL;
					break;
				case TTI_DISCONNECT:
					loop=disconnect(rp);
					rp=NULL;
					break;
				case TTI_AUTOCOMMIT_ON:
					loop=autoCommitOn(rp);
					rp=NULL;
					break;
				case TTI_AUTOCOMMIT_OFF:
					loop=autoCommitOff(rp);
					rp=NULL;
					break;
				case TTI_COMMIT:
					loop=commit(rp);
					rp=NULL;
					break;
				case TTI_ROLLBACK:
					loop=rollback(rp);
					rp=NULL;
					break;
				case TTI_CANCEL:
				case TTI_DESCRIBE:
				case TTI_DESCRIBE2:
				case TTI_STARTUP:
				case TTI_STARTUP2:
				case TTI_SHUTDOWN:
					// unimplemented - return an oracle error and
					// keep the session alive instead of dropping
					// it; rp is discarded, the call's body (and
					// anything piggybacked behind it) go unread
					loop=sendUnimplementedFunctionError();
					rp=NULL;
					break;
				case TTI_VERSION:
					loop=version(rp,true);
					rp=NULL;
					break;
				case TTI_LOB_OPERATIONS:
					loop=lobOperations(rp);
					rp=NULL;
					break;
				case TTI_K2_TRANSACTIONS:
				case TTI_OSQL7:
				case TTI_OKOD:
				case TTI_ODNY:
				case TTI_TRANSACTION_END:
				case TTI_TRANSACTION_BEGIN:
					loop=sendUnimplementedFunctionError();
					rp=NULL;
					break;
				case TTI_OCCA:
					loop=occa(rp,&rp);
					break;
				case TTI_LOGON_PRESENT_PWD:
				case TTI_LOGON_PRESENT_USER:
					loop=sendUnimplementedFunctionError();
					rp=NULL;
					break;
				case TTI_UNIDENTIFIED_0X54:
					loop=unidentified54(rp);
					rp=NULL;
					break;
				case TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD:
				case TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY:
				case TTI_OOTCM:
				case TTI_OKPFC:
					loop=sendUnimplementedFunctionError();
					rp=NULL;
					break;
				case TTI_SWITCH_SESSION:
					// one function code, two meanings
					// (a piggyback names the session the
					// call behind it is for; an ordinary
					// call is an 8.0.5 client asking for
					// the server version)
					if (lastttccode==
						TTC_PIGGYBACK_TTI_FUNCTION) {
						loop=switchSession(rp,&rp);
					} else {
						loop=version(rp,false);
						rp=NULL;
					}
					break;
				case TTI_OSCID:
				case TTI_OSKEYVAL:
					loop=sendUnimplementedFunctionError();
					rp=NULL;
					break;
				default:
					// an unrecognized function code - same
					// treatment as an unimplemented one
					loop=sendUnimplementedFunctionError();
					rp=NULL;
					break;
			}

			// release the cursor
			// FIXME: kludgy
			//cont->release(cursor);

		} while (loop);
	}

	// close the client connection
	cont->closeClientConnection(0);

	// end the session if necessary
	if (endsession) {
		cont->endSession();
	}

	return status;
}

void sqlrprotocol_oracle::resetSendPacketBuffer(byte_t packettype) {
	reqpacket.clear();
	reqpacket.append((uint64_t)0);
	reqpackettype=packettype;
	reqpacketflags=0;
}

bool sqlrprotocol_oracle::sendPacket() {
	return sendPacket(false);
}

bool sqlrprotocol_oracle::sendPacket(bool flush) {

	uint32_t	reqpacketsize=(uint32_t)reqpacket.getSize();
	uint16_t	packetchecksum=0;
	uint16_t	headerchecksum=0;

	// overwrite the first 8 bytes of the reqpacket with the packet header
	// (8 bytes either way - a 32-bit length at PROTOCOL_VERSION_12 and
	// above, a 16-bit length and a packet checksum below it; see
	// python-oracledb's src/oracledb/impl/thin/packet.pyx, which switches
	// on TNS_VERSION_MIN_LARGE_SDU, 315)
	reqpacket.setPositionRelativeToBeginning(0);
	if (largeheader) {
		reqpacket.write(hostToBE(reqpacketsize));
	} else {
		reqpacket.write(hostToBE((uint16_t)reqpacketsize));
		reqpacket.write(hostToBE(packetchecksum));
	}
	reqpacket.write(reqpackettype);
	reqpacket.write(reqpacketflags);
	reqpacket.write(hostToBE(headerchecksum));

	if (getDebug()) {
		debugStart("send");
		debugWrite("large header: %s",largeheader?"yes":"no");
		debugWrite("packet size: %d",reqpacketsize);
		if (!largeheader) {
			debugWrite("packet checksum: %d",packetchecksum);
		}
		debugWrite("packet type: %d",reqpackettype);
		debugWrite("packet flags: 0x%04x",reqpacketflags);
		debugWrite("header checksum: %d",headerchecksum);
		debugWrite("body size: %d",reqpacketsize-8);
		debugHexDump(reqpacket.getBuffer()+8,reqpacketsize-8);
		debugEnd();
	}

	// send the packet
	if (clientsock->write(reqpacket.getBuffer(),
				reqpacket.getSize())!=
				(ssize_t)reqpacket.getSize()) {
		debugWrite("write packet data failed");
		debugSystemError();
		return false;
	}
	debugWrite("wrote %d bytes",(uint32_t)reqpacket.getSize());

	if (flush) {
		clientsock->flushWriteBuffer(-1,-1);
		debugWrite("send packet flush...");
	} else {
		debugWrite("no flush...");
	}

	return true;
}

bool sqlrprotocol_oracle::recvPacket() {

	uint16_t	packetchecksum=0;

	if (largeheader) {

		// size
		// 4 bytes (big endian)
		if (clientsock->read(&resppacketsize)!=sizeof(uint32_t)) {
			debugWrite("read packet size failed");
			debugSystemError();
			return false;
		}
		resppacketsize=beToHost(resppacketsize);

	} else {

		// size
		// 2 bytes (big endian)
		uint16_t	smallsize;
		if (clientsock->read(&smallsize)!=sizeof(uint16_t)) {
			debugWrite("read packet size failed");
			debugSystemError();
			return false;
		}
		resppacketsize=beToHost(smallsize);

		// packet checksum
		// 2 bytes (big endian) (always 0)
		if (clientsock->read(&packetchecksum)!=sizeof(uint16_t)) {
			debugWrite("read packet checksum failed");
			debugSystemError();
			return false;
		}
		packetchecksum=beToHost(packetchecksum);
	}

	// sanity check
	if (resppacketsize<8 || resppacketsize>sdu) {
		debugWrite("invalid packet size: %d",resppacketsize);
		debugSystemError();
		return false;
	}

	// packet type
	// 1 byte
	if (clientsock->read(&resppackettype)!=sizeof(byte_t)) {
		debugWrite("read packet type failed");
		debugSystemError();
		return false;
	}

	// packet flags
	// 1 byte
	byte_t	packetflags;
	if (clientsock->read(&packetflags)!=sizeof(byte_t)) {
		debugWrite("read packet flags failed");
		debugSystemError();
		return false;
	}

	// header checksum
	// 2 bytes (big endian) (always 0)
	uint16_t	headerchecksum;
	if (clientsock->read(&headerchecksum)!=sizeof(uint16_t)) {
		debugWrite("read header checksum failed");
		debugSystemError();
		return false;
	}
	headerchecksum=beToHost(headerchecksum);

	// we've already received 8 bytes...
	resppacketsize-=8;

	// reallocate recv buffer
	resppacketpool->clear();
	resppacket=resppacketpool->allocate(resppacketsize);

	// packet
	if (clientsock->read(resppacket,resppacketsize)!=
					(ssize_t)resppacketsize) {
		debugWrite("read packet failed");
		debugSystemError();
		return false;
	}

	if (getDebug()) {
		debugStart("recv");
		debugWrite("large header: %s",largeheader?"yes":"no");
		debugWrite("packet size: %d",resppacketsize+8);
		if (!largeheader) {
			debugWrite("packet checksum: %d",packetchecksum);
		}
		debugWrite("packet type: %d",resppackettype);
		debugWrite("packet flags: %d",packetflags);
		debugWrite("header checksum: %d",headerchecksum);
		debugWrite("body size: %d",resppacketsize);
		debugHexDump(resppacket,resppacketsize);
		debugEnd();
	}

	return true;
}

void sqlrprotocol_oracle::readHost(const byte_t *rp,
					uint16_t *value,
					const byte_t **rpout) {
	bytestring::copy(value,rp,sizeof(uint16_t));
	*rpout=rp+sizeof(uint16_t);
}

void sqlrprotocol_oracle::readHost(const byte_t *rp,
					uint32_t *value,
					const byte_t **rpout) {
	bytestring::copy(value,rp,sizeof(uint32_t));
	*rpout=rp+sizeof(uint32_t);
}

bool sqlrprotocol_oracle::readMarker16(const byte_t *rp,
					uint16_t expected,
					const byte_t **rpout) {
	uint16_t	marker;
	return readBE(rp,&marker,"marker",expected,rpout);
}

bool sqlrprotocol_oracle::readMarker32(const byte_t *rp,
					uint32_t expected,
					const byte_t **rpout) {
	uint32_t	marker;
	return readBE(rp,&marker,"marker",expected,rpout);
}

bool sqlrprotocol_oracle::getNullTerminatedArray(const byte_t *rp,
						const byte_t *end,
						byte_t **array,
						uint32_t *arraycount,
						const byte_t **rpout) {
	*array=NULL;
	*arraycount=0;

	// count the array items
	const byte_t	*start=rp;
	for (;;) {
		if (rp>=end) {
			debugWrite("bad null terminated array, "
					"no null terminator found");
			return false;
		}
		if (!(*rp)) {
			break;
		}
		(*arraycount)++;
		rp++;
	}

	// reset rp
	rp=start;

	// get the array items
	*array=new byte_t[*arraycount];
	for (uint32_t i=0; i<*arraycount; i++) {
		read(rp,&((*array)[i]),&rp);
	}

	// skip the null terminator
	rp++;

	*rpout=rp;

	debugWrite("null terminated array count: %d",*arraycount);

	return true;
}

bool sqlrprotocol_oracle::getString(const byte_t *rp,
					const byte_t *end,
					char **string,
					const byte_t **rpout) {
	*string=NULL;
	uint32_t	stringsize=0;

	// count the bytes
	const byte_t	*start=rp;
	for (;;) {
		if (rp>=end) {
			debugWrite("bad string, "
					"no null terminator found");
			return false;
		}
		if (!(*rp)) {
			break;
		}
		stringsize++;
		rp++;
	}

	// reset rp
	rp=start;

	// get the string
	*string=new char[stringsize+1];
	for (uint32_t i=0; i<stringsize; i++) {
		read(rp,&((*string)[i]),&rp);
	}
	(*string)[stringsize]='\0';

	// skip the null terminator
	rp++;

	*rpout=rp;

	debugWrite("string: \"%s\"",*string);

	return true;
}

bool sqlrprotocol_oracle::getString(const byte_t *rp,
					char **string,
					uint32_t size,
					const byte_t **rpout) {
	*string=charstring::duplicate((const char *)rp,size);
	*rpout=rp+size;
	debugWrite("string: \"%s\", size: %d",*string,size);
	return true;
}

void sqlrprotocol_oracle::writeHost(bytebuffer *buffer, uint16_t value) {
	buffer->append(value);
}

void sqlrprotocol_oracle::writeHost(bytebuffer *buffer, uint32_t value) {
	buffer->append(value);
}

char *sqlrprotocol_oracle::generateHex(uint16_t bytes) {

	stringbuffer	str;
	uint32_t	number;
	for (uint16_t i=0; i<bytes*2; i++) {
		r.generate(&number);
		int32_t	nibble=prng::scale(number,0,15);
		str.append((char)(nibble+((nibble<10)?'0':'A'-10)));
	}
	return str.detachString();
}

bool sqlrprotocol_oracle::initialHandshake() {

	debugStart("initial handshake");

	bool	result=connect() &&
		anoNegotiation() &&
		ttiNegotiation() &&
		dataTypeNegotiation() &&
		authenticate();

	debugWrite("result: %d",result);
	debugEnd();

	return result;
}

bool sqlrprotocol_oracle::connect() {

	debugStart("connect");

	if (!recvConnectRequest() ||
		// the database always requests a resend here, for some reason
		!sendResend() ||
		!recvConnectRequest()) {
		debugWrite("failed receiving connect request");
		debugEnd();
		return false;
	}

	// a client naming a SID/SERVICE_NAME this listener isn't configured
	// for gets refused here, the same way a real listener would, rather
	// than being let through to whatever backend connection is on hand
	if (!requestedServiceKnown()) {
		debugWrite("requested service unknown, refusing");
		debugEnd();
		sendRefuse(TNS_NO_SUCH_SERVICE);
		return false;
	}

	bool	result=sendConnectResponse();

	debugWrite("result: %d",result);
	debugEnd();

	return result;
}

bool sqlrprotocol_oracle::requestedServiceKnown() {

	// nothing configured to check against - accept whatever was asked for
	if (!sids) {
		return true;
	}

	// the client's descriptor didn't name a SID/SERVICE_NAME at all -
	// nothing to compare, so let it through
	if (!requestedservice) {
		return true;
	}

	char		**sidlist=NULL;
	uint64_t	sidcount=0;
	charstring::split(sids,",",true,&sidlist,&sidcount);

	bool	known=false;
	for (uint64_t i=0; i<sidcount; i++) {
		// tolerate "ora1, ora2" as well as "ora1,ora2"
		charstring::bothTrim(sidlist[i]);
		if (!charstring::compareIgnoringCase(
					sidlist[i],requestedservice)) {
			known=true;
			break;
		}
	}

	for (uint64_t i=0; i<sidcount; i++) {
		delete[] sidlist[i];
	}
	delete[] sidlist;

	debugStart("requested service known");
	debugWrite("requested service: \"%s\"",
			(requestedservice)?requestedservice:"(none)");
	debugWrite("configured sids: \"%s\"",(sids)?sids:"(none)");
	debugWrite("known: %d",known);
	debugEnd();

	return known;
}

const char *sqlrprotocol_oracle::findConnectDataKey(const char *haystack,
							const char *key) {

	// finds "key" in "haystack", but only a standalone occurrence -
	// immediately preceded by '(' (skipping whitespace) and immediately
	// followed by '=' (skipping whitespace) - the shape every key takes
	// in a tns connect descriptor, eg. "(SID = ora1)" or
	// "(SERVICE_NAME=ora1)".  without those checks, "SID" also matches
	// inside "westside" or "PROTOCOL_VERSION_12", refusing a client whose
	// descriptor just happens to contain the substring.
	size_t	keylen=charstring::getLength(key);
	const char	*p=haystack;
	while (p && *p) {
		const char	*hit=charstring::findFirstIgnoringCase(p,key);
		if (!hit) {
			return NULL;
		}

		const char	*before=hit;
		while (before>haystack &&
				character::isWhitespace(*(before-1))) {
			before--;
		}

		const char	*after=hit+keylen;
		while (character::isWhitespace(*after)) {
			after++;
		}

		if (before>haystack && *(before-1)=='(' && *after=='=') {
			return hit;
		}

		p=hit+1;
	}
	return NULL;
}

bool sqlrprotocol_oracle::recvConnectRequest() {

	if (!recvPacket()) {
		return false;
	}

	if (resppackettype!=PACKET_CONNECT) {
		debugWrite("bad packet type %d, expected %d",
					resppackettype,PACKET_CONNECT);
		return false;
	}

	const byte_t	*rp=resppacket;

	// the fixed part, up to and including the trace connection ids
	if (resppacketsize<50) {
		debugWrite("truncated connect packet: %d",resppacketsize);
		return false;
	}

	uint16_t	protocolcharacteristics;
	uint16_t	maxpacketsbeforeack;
	uint16_t	one;
	uint16_t	connectdatasize;
	uint16_t	connectdataoffset;
	uint32_t	maxconnectdatathatcanbereceived;
	uint32_t	tracecrossfacilityitem1;
	uint32_t	tracecrossfacilityitem2;
	uint64_t	traceuniqueconnectionid1;
	uint64_t	traceuniqueconnectionid2;
	uint16_t	smallsdu;
	uint16_t	smalltdu;

	readBE(rp,&connectversion,&rp);
	readBE(rp,&connectlowestversion,&rp);
	readBE(rp,&gso,&rp);
	readBE(rp,&smallsdu,&rp);
	readBE(rp,&smalltdu,&rp);
	readBE(rp,&protocolcharacteristics,&rp);
	readBE(rp,&maxpacketsbeforeack,&rp);
	readHost(rp,&one,&rp);
	readBE(rp,&connectdatasize,&rp);
	readBE(rp,&connectdataoffset,&rp);
	readBE(rp,&maxconnectdatathatcanbereceived,&rp);
	readBE(rp,&anoflags,&rp);
	readBE(rp,&tracecrossfacilityitem1,&rp);
	readBE(rp,&tracecrossfacilityitem2,&rp);
	readBE(rp,&traceuniqueconnectionid1,&rp);
	readBE(rp,&traceuniqueconnectionid2,&rp);

	sdu=smallsdu;
	tdu=smalltdu;

	// a client at PROTOCOL_VERSION_12 or higher repeats the sdu and tdu as
	// 32-bit values behind the trace ids, and those are the ones it means
	if (connectversion>=PROTOCOL_VERSION_12 && resppacketsize>=58) {
		readBE(rp,&sdu,&rp);
		readBE(rp,&tdu,&rp);
	}

	// connect data
	// (a connect string too long for the connect packet arrives in a data
	// packet behind it - two data flag bytes and then the string - and the
	// offset then points past the end of the connect packet)
	const char	*connectdata=NULL;
	uint32_t	dataoffset=(connectdataoffset>=8)?
					(uint32_t)connectdataoffset-8:0;
	if ((uint64_t)dataoffset+connectdatasize<=(uint64_t)resppacketsize) {
		connectdata=(const char *)resppacket+dataoffset;
	} else if (recvPacket() && resppackettype==PACKET_DATA &&
						resppacketsize>=2) {
		connectdata=(const char *)resppacket+2;
		if ((uint64_t)connectdatasize>(uint64_t)resppacketsize-2) {
			connectdatasize=(uint16_t)(resppacketsize-2);
		}
	} else {
		debugWrite("couldn't read connect data");
		return false;
	}

	debugStart("connect");
	debugWrite("version: 0x%04x",connectversion);
	debugWrite("lowest supported version: 0x%04x",connectlowestversion);
	debugWrite("gso: 0x%04x",gso);
	debugWrite("sdu: %d",sdu);
	debugWrite("tdu: %d",tdu);
	debugWrite("protocol characteristics: 0x%04x",protocolcharacteristics);
	debugWrite("max packets before ack: %d",maxpacketsbeforeack);
	debugWrite("client is little endian: %d",(one==1));
	debugWrite("connect data size: %d",connectdatasize);
	debugWrite("connect data offset: %d",connectdataoffset);
	debugWrite("max connect data that can be received: %d",
					maxconnectdatathatcanbereceived);
	debugWrite("ANO flags: 0x%04x",anoflags);
	debugWrite("trace cross facility item 1: 0x%08x",
					tracecrossfacilityitem1);
	debugWrite("trace cross facility item 2: 0x%08x",
					tracecrossfacilityitem2);
	debugWrite("trace unique connection id 1: 0x%016llx",
					(unsigned long long)traceuniqueconnectionid1);
	debugWrite("trace unique connection id 2: 0x%016llx",
					(unsigned long long)traceuniqueconnectionid2);
	debugWrite("connect data: %*s",connectdatasize,connectdata);

	// pull the SID/SERVICE_NAME the client asked for out of the connect
	// descriptor (they're interchangeable to a client - it names whichever
	// one its tnsnames.ora entry used), bounded to connectdatasize since
	// connectdata isn't necessarily NUL-terminated
	delete[] requestedservice;
	requestedservice=NULL;
	if (connectdata && connectdatasize) {
		char	*cd=charstring::duplicate(connectdata,connectdatasize);

		// SID/SERVICE_NAME only mean anything inside the CONNECT_DATA
		// clause - search there, not the whole descriptor, so a host,
		// program, or user name that happens to contain "sid" (eg.
		// "westside") can't be mistaken for the key
		const char	*searchstart=findConnectDataKey(cd,"CONNECT_DATA");
		if (!searchstart) {
			searchstart=cd;
		}

		const char	*key=findConnectDataKey(searchstart,"SERVICE_NAME");
		if (!key) {
			key=findConnectDataKey(searchstart,"SID");
		}
		const char	*eq=(key)?charstring::findFirst(key,'='):NULL;
		if (eq) {
			eq++;
			while (character::isWhitespace(*eq)) {
				eq++;
			}
			const char	*valend=eq;
			while (*valend && *valend!=')' &&
					!character::isWhitespace(*valend)) {
				valend++;
			}
			if (valend>eq) {
				requestedservice=charstring::duplicate(
							eq,(size_t)(valend-eq));
			}
		}
		delete[] cd;
	}
	debugWrite("requested service: %s",
			(requestedservice)?requestedservice:"(none)");
	debugEnd();

	return true;
}

bool sqlrprotocol_oracle::sendConnectResponse() {

	debugStart("connect response");

	// answer with the highest protocol version we support that the client
	// can speak
	// (python-oracledb and node-oracledb refuse anything under
	// PROTOCOL_VERSION_12 outright, so 12 is what makes them connect)
	if (connectlowestversion<=PROTOCOL_VERSION_12 &&
			connectversion>=PROTOCOL_VERSION_12) {
		connectversion=PROTOCOL_VERSION_12;
		debugWrite("negotiated version: 0x%04x (12)",connectversion);
	} else if (connectlowestversion<=PROTOCOL_VERSION_11 &&
			connectversion>=PROTOCOL_VERSION_11) {
		connectversion=PROTOCOL_VERSION_11;
		debugWrite("negotiated version: 0x%04x (11)",connectversion);
	} else if (connectlowestversion<=PROTOCOL_VERSION_8) {
		connectversion=PROTOCOL_VERSION_8;
		debugWrite("negotiated version: 0x%04x (8)",connectversion);
	} else {
		debugWrite("no supported connect protocol version found");
		debugEnd();
		sendRefuse(TNS_CONNECTION_REFUSED);
		return false;
	}

	debugWrite("sending accept");
	debugEnd();

	return sendAccept();
}

bool sqlrprotocol_oracle::sendAccept() {
	return sendAccept(NULL,0);
}

bool sqlrprotocol_oracle::sendAccept(const byte_t *data, uint16_t datasize) {

	bool	large=(connectversion>=PROTOCOL_VERSION_12);

	// data offset:
	// (8 bytes for the header +
	//  16 bytes of this stuff +
	//  8 bytes of padding, and 9 more for the large form)
	uint16_t	dataoffset=(large)?41:32;
	// NOTE: 0x0801 returned by 8.0 server
	//gso=0x0801;
	uint64_t	padding=0;

	// debug
	debugStart("accept");
	debugWrite("version: 0x%04x",connectversion);
	debugWrite("large header: %d",large);
	debugWrite("gso: 0x%04x",gso);
	debugWrite("sdu: %d",sdu);
	debugWrite("tdu: %d",tdu);
	debugWrite("data size: %d",datasize);
	debugWrite("data offset: %d",dataoffset);
	debugWrite("ANO flags: 0x%04x",anoflags);
	debugHexDump(data,datasize);
	debugEnd();

	// build packet
	resetSendPacketBuffer(PACKET_ACCEPT);
	writeBE(&reqpacket,connectversion);
	writeBE(&reqpacket,gso);
	writeBE(&reqpacket,(uint16_t)((sdu>0xffff)?0xffff:sdu));
	writeBE(&reqpacket,(uint16_t)((tdu>0xffff)?0xffff:tdu));
	// byte order marker; always 1, written in host byte order on purpose
	// (not writeLE()/writeBE() - the client compares the raw, unconverted
	// bytes to detect our endianness; see the matching readHost() in
	// recvConnectRequest())
	writeHost(&reqpacket,(uint16_t)1);
	writeBE(&reqpacket,datasize);
	writeBE(&reqpacket,dataoffset);
	// echo the client's ano flags back.  the docs call this a pair of
	// ub1s, the bitmask and the same bitmask repeated; every client
	// writes them identically, so one big-endian ub2 says the same thing.
	// (the client decides whether to send an ano request from the flags in
	// the accept, so echoing means both ends reach the same decision from
	// the same bits)
	writeBE(&reqpacket,anoflags);
	writeBE(&reqpacket,padding);
	if (large) {
		writeBE(&reqpacket,sdu);
		writeBE(&reqpacket,tdu);
		write(&reqpacket,(byte_t)0);
	}
	reqpacket.append(data,datasize);

	if (!sendPacket(true)) {
		return false;
	}

	// the accept itself keeps the 16-bit header whatever version it
	// announces - only the packets after it switch
	largeheader=large;

	return true;
}

bool sqlrprotocol_oracle::sendResend() {

	// debug
	debugStart("resend");
	debugWrite("packet type: %d",PACKET_RESEND);
	debugWrite("packet size: %d (no body)",0);
	debugEnd();

	// build packet
	resetSendPacketBuffer(PACKET_RESEND);

	return sendPacket(true);
}

bool sqlrprotocol_oracle::sendMarker(byte_t markertype) {

	// debug
	debugStart("marker");
	debugWrite("packet type: %d",PACKET_MARKER);
	debugWrite("marker type: %d",markertype);
	debugEnd();

	// build packet - 1 (one data byte follows), 0 (reserved), then
	// the marker type.  the header's reserved byte carries the marker
	// flag, same as a real client's marker does
	resetSendPacketBuffer(PACKET_MARKER);
	reqpacketflags=PACKET_FLAG_MARKER;
	write(&reqpacket,(byte_t)1);
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,markertype);

	return sendPacket(true);
}

bool sqlrprotocol_oracle::sendRefuse(uint32_t tnserror) {

	// the client digs the ERR= out of the message rather than reading the
	// two reason bytes, so an empty body leaves it with no reason at all
	stringbuffer	message;
	message.append("(DESCRIPTION=(TMP=)(VSNNUM=");
	message.append(SERVER_VERSION_NUMBER);
	message.append(")(ERR=");
	message.append(tnserror);
	message.append(")(ERROR_STACK=(ERROR=(CODE=");
	message.append(tnserror);
	message.append(")(EMFI=4))))");

	uint16_t	messagesize=(uint16_t)message.getStringLength();

	// debug
	debugStart("refuse");
	debugWrite("error: 0x%08x (%d)",tnserror,tnserror);
	debugWrite("meaning: %s",
		(tnserror==TNS_NO_SUCH_SERVICE)?
			"no such service/sid" :
		(tnserror==TNS_CONNECTION_REFUSED)?
			"connection refused" :
			"unknown");
	debugWrite("message: %s",message.getString());
	debugEnd();

	// build packet
	resetSendPacketBuffer(PACKET_REFUSE);
	write(&reqpacket,(byte_t)REFUSE_REASON_USER);
	write(&reqpacket,(byte_t)REFUSE_REASON_SYSTEM);
	writeBE(&reqpacket,messagesize);
	write(&reqpacket,message.getString(),(size_t)messagesize);

	return sendPacket(true);
}

bool sqlrprotocol_oracle::anoNegotiation() {

	// ano is optional - only a client that set NSI_NA_WANTED sends an ano
	// request, and waiting for one that never comes reads the client's
	// ttipro as one and kills the handshake.  the two flag bytes are
	// written identically by every client seen, but check both anyway.
	//
	// oracle's own thin drivers don't ask for it - captured against an
	// oracle 12.2 server, python-oracledb sends 0x84,
	// NSI_SUP_SEC_RENEG|NSI_NA_DISABLED, and node-oracledb sends 0x08,
	// NSI_NA_NO_SERVICES, and both go straight to the tti negotiation.
	if (!(((anoflags>>8)|anoflags)&NSI_NA_WANTED)) {
		debugStart("ano negotiation");
		debugWrite("client didn't ask for ano, skipping it");
		debugEnd();
		return true;
	}

	debugStart("ano negotiation");
	debugWrite("client requested ano, negotiating");
	debugEnd();

	if (!recvAnoRequest()) {
		return false;
	}

	warnAnoDeclined();

	bool	result=sendAnoResponse();

	debugStart("ano negotiation");
	debugWrite("result: %d",result);
	debugEnd();

	return result;
}

void sqlrprotocol_oracle::warnAnoDeclined() {

	debugStart("ano declined");

	// through the logger modules rather than stderror.printf(), because
	// this fires per connection rather than once at construction.  there's
	// no cursor during the handshake, hence the NULL.
	uint32_t	encdrivers=anoDriversOffered(encryptiondrivers,
							encryptiondrivercount);
	if (encdrivers) {
		debugWrite("declining %d offered encryption driver(s)",
								encdrivers);
		cont->raiseInternalWarningEvent(NULL,
			"client requested oracle native network encryption "
			"(%d algorithms).  this module doesn't implement it "
			"and has declined it.  a client with "
			"SQLNET.ENCRYPTION_CLIENT=REQUIRED will fail with "
			"ORA-12660; use ACCEPTED, REQUESTED or REJECTED.",
			encdrivers);
	}

	uint32_t	didrivers=anoDriversOffered(
						dataintegritydrivers,
						dataintegritydrivercount);
	if (didrivers) {
		debugWrite("declining %d offered data integrity "
						"driver(s)",didrivers);
		cont->raiseInternalWarningEvent(NULL,
			"client requested oracle crypto-checksumming "
			"(%d algorithms).  this module doesn't implement it "
			"and has declined it.  a client with "
			"SQLNET.CRYPTO_CHECKSUM_CLIENT=REQUIRED will fail "
			"with ORA-12660; use ACCEPTED, REQUESTED or "
			"REJECTED.",
			didrivers);
	}

	if (!encdrivers && !didrivers) {
		debugWrite("nothing declined - client offered no "
				"encryption or data integrity drivers");
	}

	debugEnd();
}

uint32_t sqlrprotocol_oracle::anoDriversOffered(uint16_t *drivers,
						uint32_t drivercount) {

	// algorithm 0 is "none", and every client sends it whether it wants
	// encryption or not, so it isn't an offer of anything
	uint32_t	offered=0;
	for (uint32_t i=0; i<drivercount; i++) {
		if (drivers[i]) {
			debugStart("ano driver offered");
			debugWrite("index: %d",i);
			debugWrite("driver id: 0x%04x",drivers[i]);
			debugEnd();
			offered++;
		}
	}
	return offered;
}

bool sqlrprotocol_oracle::recvAnoRequest() {

	if (!recvPacket()) {
		return false;
	}

	if (resppackettype!=PACKET_DATA) {
		debugWrite("bad packet type %d, expected %d",
						resppackettype,PACKET_DATA);
		return false;
	}

	const byte_t	*rp=resppacket;
	const byte_t	*end=rp+resppacketsize;

	// ano request header...
	uint16_t	dataflags;
	uint16_t	overallsize;
	uint16_t	servicecount;
	byte_t		desiredoptionsflag;

	if (!anoBoundsCheck(rp,end,15,"request header")) {
		return false;
	}

	readBE(rp,&dataflags,&rp);
	if (!readMarker32(rp,ANO_MARKER,&rp)) {
		return false;
	}
	readBE(rp,&overallsize,&rp);
	readBE(rp,&anorequestversion,&rp);
	readBE(rp,&servicecount,&rp);
	read(rp,&desiredoptionsflag,&rp);

	debugStart("ano request header");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("overall size: %d",overallsize);
	debugWrite("version: 0x%08x",anorequestversion);
	debugWrite("service count %d",servicecount);
	debugWrite("desired options flag: 0x%02x",desiredoptionsflag);

	// every service is at least an 8-byte header, and they all have to
	// fit in what's left of the packet
	if ((size_t)servicecount>(size_t)(end-rp)/8) {
		debugWrite("bad ano service count: %d",servicecount);
		debugEnd();
		return false;
	}

	// service count ...
	bool	success=true;
	for (uint16_t i=0; i<servicecount; i++) {

		success=false;

		uint16_t	service;
		uint16_t	fieldcount;
		if (!getAnoServiceHeader(rp,end,&service,&fieldcount,&rp)) {
			break;
		}

		switch (service) {
			case ANO_SERVICE_SUPERVISOR:
				success=getSupervisorService(
							rp,end,fieldcount,&rp);
				break;
			case ANO_SERVICE_AUTHENTICATION:
				success=getAuthenticationService(
							rp,end,fieldcount,&rp);
				break;
			case ANO_SERVICE_ENCRYPTION:
				success=getEncryptionService(
							rp,end,fieldcount,&rp);
				break;
			case ANO_SERVICE_DATA_INTEGRITY:
				success=getDataIntegrityService(
							rp,end,fieldcount,&rp);
				break;
			default:
				debugWrite("bad ano service: %d",service);
				break;
		}
		if (!success) {
			break;
		}
	}

	debugEnd();

	// bail if something failed
	if (!success) {
		return false;
	}

	return true;
}

bool sqlrprotocol_oracle::anoBoundsCheck(const byte_t *rp,
						const byte_t *end,
						size_t size,
						const char *name) {

	// the rp>end test is belt and braces - if rp ever did get past end
	// then end-rp would be negative and the unsigned comparison would pass
	if (rp>end || (size_t)(end-rp)<size) {
		debugWrite("bad ano %s, truncated, needed %d bytes, "
				"%d available",
				name,(uint32_t)size,
				(rp>end)?(uint32_t)0:(uint32_t)(end-rp));
		return false;
	}
	debugWrite("ano %s bounds ok, needed %d bytes",name,(uint32_t)size);
	return true;
}

bool sqlrprotocol_oracle::getAnoServiceHeader(const byte_t *rp,
						const byte_t *end,
						uint16_t *service,
						uint16_t *fieldcount,
						const byte_t **rpout) {

	if (!anoBoundsCheck(rp,end,8,"service header")) {
		return false;
	}

	readBE(rp,service,&rp);
	readBE(rp,fieldcount,&rp);
	if (!readMarker32(rp,0x00000000,&rp)) {
		return false;
	}

	debugStart("ano service header");
	debugWrite("service: %d",*service);
	debugWrite("field count: %d",*fieldcount);
	debugEnd();

	*rpout=rp;

	return true;
}


bool sqlrprotocol_oracle::getSupervisorService(const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout) {

	debugStart("supervisor");
	debugWrite("service: ANO Supervisor");

	uint32_t	pid;
	uint32_t	connectiontype;
	uint16_t	*drivers=NULL;
	uint32_t	drivercount;
	if (!getAnoVersionField(rp,end,&supervisorversion,&rp) ||
		!getAnoConnectionInfoField(rp,end,&pid,&connectiontype,&rp) ||
		!getAnoArrayField(rp,end,&drivers,&drivercount,&rp)) {
		delete[] drivers;
		debugEnd();
		return false;
	}

	delete[] drivers;

	debugEnd();

	*rpout=rp;
	
	return true;
}

bool sqlrprotocol_oracle::getAuthenticationService(const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout) {

	debugStart("authentication");
	debugWrite("service: ANO Authentication");

	uint16_t	constant;
	uint16_t	status;
	if (!getAnoVersionField(rp,end,&authenticationversion,&rp) ||
		!getAnoConstantField(rp,end,&constant,&rp) ||
		!getAnoStatusField(rp,end,&status,&rp)) {
		debugEnd();
		return false;
	}

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getEncryptionService(const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout) {

	debugStart("encryption");
	debugWrite("service: ANO Encryption");

	uint16_t	*drivers=NULL;
	uint32_t	drivercount;
	byte_t		constant;
	if (!getAnoVersionField(rp,end,&encryptionversion,&rp) ||
		!getAnoDriverListField(rp,end,&drivers,&drivercount,&rp)) {
		delete[] drivers;
		debugEnd();
		return false;
	}
	if (fieldcount>2) {
		if (!getAnoConstantField(rp,end,&constant,&rp)) {
			delete[] drivers;
			debugEnd();
			return false;
		}
	}

	// keep the offer rather than dropping it, so warnAnoDeclined() can
	// report what was turned down.  a session can negotiate twice.
	delete[] encryptiondrivers;
	encryptiondrivers=drivers;
	encryptiondrivercount=drivercount;

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getDataIntegrityService(
						const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout) {

	debugStart("data integrity");
	debugWrite("service: ANO Data Integrity");

	uint16_t	*drivers=NULL;
	uint32_t	drivercount;
	if (!getAnoVersionField(rp,end,&dataintegrityversion,&rp) ||
		!getAnoDriverListField(rp,end,&drivers,&drivercount,&rp)) {
		delete[] drivers;
		debugEnd();
		return false;
	}

	// see getEncryptionService()
	delete[] dataintegritydrivers;
	dataintegritydrivers=drivers;
	dataintegritydrivercount=drivercount;

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getAnoVersionField(const byte_t *rp,
						const byte_t *end,
						uint32_t *version,
						const byte_t **rpout) {
	if (!anoBoundsCheck(rp,end,8,"version field")) {
		return false;
	}

	debugStart("version");

	uint16_t	size;
	uint16_t	type;
	if (!readBE(rp,&size,"size",4,&rp) ||
		!readBE(rp,&type,"type",ANO_FIELD_TYPE_VERSION,&rp)) {
		debugEnd();
		return false;
	}
	readBE(rp,version,&rp);

	if (getDebug()) {
		debugWrite("version: 0x%08x",*version);
		// 8.0 -> 10g send a version string, 11i+ sends all 0's
	}

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getAnoConnectionInfoField(
						const byte_t *rp,
						const byte_t *end,
						uint32_t *pid,
						uint32_t *connectiontype,
						const byte_t **rpout) {
	if (!anoBoundsCheck(rp,end,12,"connection info field")) {
		return false;
	}

	debugStart("connection info");

	uint16_t	size;
	uint16_t	type;
	if (!readBE(rp,&size,"size",8,&rp) ||
		!readBE(rp,&type,"type",ANO_FIELD_TYPE_RAW_BYTES,&rp)) {
		debugEnd();
		return false;
	}
	readBE(rp,pid,&rp);
	readBE(rp,connectiontype,&rp);

	// we consistently get 0x1788dda1 or 0x1784574b for the connection
	// type, but 8.0.5 (at least) consistently sends 0x1784574b to the
	// real db.

	if (getDebug()) {
		debugWrite("pid: %d",*pid);
		debugWrite("connection type: 0x%08x",*connectiontype);
	}

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getAnoArrayField(const byte_t *rp,
						const byte_t *end,
						uint16_t **array,
						uint32_t *arraycount,
						const byte_t **rpout) {

	*array=NULL;
	*arraycount=0;

	// bounds-check the size and type
	if (end-rp<4) {
		debugWrite("bad array field, truncated header");
		return false;
	}

	// get the total field size
	uint16_t	size;
	readBE(rp,&size,&rp);

	// get the field type, should be raw bytes.  a raw bytes field whose
	// contents are a deadbeef-marked ub2 array is what this reads.
	// see "Oracle Wire Protocol - ANO Negotiation"
	uint16_t	type;
	if (!readBE(rp,&type,"type",ANO_FIELD_TYPE_RAW_BYTES,&rp)) {
		return false;
	}

	// the size counts the bytes after the size and type, and they all
	// have to be in the packet
	if ((size_t)(end-rp)<(size_t)size) {
		debugWrite("bad array field size: %d",size);
		return false;
	}

	// the end of this field, measured before anything reads past the
	// header - readMarker32/16 only rewind the read pointer when they fail
	const byte_t	*fieldend=rp+size;

	// if size was 1, then there is just a null terminator
	// skip it and bail
	if (size==1) {
		*rpout=fieldend;
		return true;
	}

	// look for a deadbeef marker, followed by an array marker.
	// 10 bytes for the two markers and the array count.
	if (size<10 ||
		!readMarker32(rp,ANO_MARKER,&rp) ||
		!readMarker16(rp,ANO_ARRAY_MARKER,&rp)) {

		// a field sometimes has an array marker and no deadbeef, and
		// sometimes neither.  both shapes are the encryption and
		// data integrity services' driver lists - one byte per
		// algorithm id, with a field header identical to this one -
		// and they have their own reader now, so neither reaches this
		// function.  the two differ only in whether the first two
		// algorithm ids happen to spell the array marker: ojdbc 23.26
		// offers 0, 3, 4, 5 and 6 for data integrity, and the
		// first two of those are the bytes 00 03.
		//
		// the one caller left is getSupervisorService(), and both
		// clients that reach ano here send a real deadbeef ub2 array,
		// so this is a guard rather than a decoder - hence the dump.
		// returning NULL/0 without failing is deliberate: a supervisor
		// list nobody can read is not worth refusing a connection over.
		//
		// the dumped bytes are unexplained
		// see "Oracle Wire Protocol - ANO Negotiation"
		debugStart("unrecognized array field");
		debugHexDump(fieldend-size,size);
		debugEnd();

		*rpout=fieldend;
		return true;
	}

	// get the array count
	readBE(rp,arraycount,&rp);

	debugWrite("array count: %d",*arraycount);

	// the members have to fit in what's left of the field
	if (*arraycount>(uint32_t)(fieldend-rp)/sizeof(uint16_t)) {
		debugWrite("bad array count: %d",*arraycount);
		return false;
	}

	// get the array members
	if (*arraycount) {
		*array=new uint16_t[*arraycount];
		for (uint32_t i=0; i<*arraycount; i++) {
			readBE(rp,&((*array)[i]),&rp);
			debugWrite("array[%d]: %d",i,(*array)[i]);
		}
	}

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getAnoDriverListField(const byte_t *rp,
						const byte_t *end,
						uint16_t **drivers,
						uint32_t *drivercount,
						const byte_t **rpout) {

	*drivers=NULL;
	*drivercount=0;

	if (!anoBoundsCheck(rp,end,4,"driver list field")) {
		return false;
	}

	debugStart("driver list");

	uint16_t	size;
	readBE(rp,&size,&rp);

	uint16_t	type;
	if (!readBE(rp,&type,"type",ANO_FIELD_TYPE_RAW_BYTES,&rp)) {
		debugEnd();
		return false;
	}

	// the size counts the bytes after the size and type
	if (!anoBoundsCheck(rp,end,size,"driver list field")) {
		debugEnd();
		return false;
	}

	// the encryption and data integrity services send one byte per
	// algorithm id even though the field header is identical to the ub2
	// array getAnoArrayField() reads - and a field of this shape could
	// carry that instead, so skip it rather than report nonsense
	//
	// three sources agree on the byte form.  Redfern's 8i capture on the
	// Oracle Protocol wiki page sends "00 01 00 01 00" for the encryption
	// service, one byte, annotated "AlgID (0=none)".  node-oracledb's
	// EncryptionService and DataIntegrityService both send that same
	// single byte through sendRaw(), which writes a size, a type of 1,
	// and then raw bytes.  and ojdbc8 sends "00 04 00 01 00 0f 10 11",
	// which is none, aes128, aes192 and aes256.
	// the array itself is unexplained
	// see "Oracle Wire Protocol - ANO Negotiation"
	if (size>=4 && rp[0]==0xde && rp[1]==0xad &&
					rp[2]==0xbe && rp[3]==0xef) {
		debugWrite("driver list is a ub2 array, not decoded");
		debugEnd();
		*rpout=rp+size;
		return true;
	}

	*drivercount=size;
	if (size) {
		*drivers=new uint16_t[size];
		for (uint16_t i=0; i<size; i++) {
			byte_t	driver;
			read(rp,&driver,&rp);
			(*drivers)[i]=driver;
			debugWrite("driver[%d]: %d",i,driver);
		}
	}

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getAnoConstantField(const byte_t *rp,
						const byte_t *end,
						uint16_t *constant,
						const byte_t **rpout) {
	if (!anoBoundsCheck(rp,end,6,"constant field")) {
		return false;
	}

	debugStart("constant");

	uint16_t	size;
	uint16_t	type;
	if (!readBE(rp,&size,"size",2,&rp) ||
		!readBE(rp,&type,"type",ANO_FIELD_TYPE_UB2,&rp)) {
		debugEnd();
		return false;
	}
	readBE(rp,constant,&rp);

	debugWrite("constant: 0x%04x",*constant);

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getAnoConstantField(const byte_t *rp,
						const byte_t *end,
						byte_t *constant,
						const byte_t **rpout) {
	if (!anoBoundsCheck(rp,end,5,"constant field")) {
		return false;
	}

	debugStart("constant");

	uint16_t	size;
	uint16_t	type;
	if (!readBE(rp,&size,"size",1,&rp) ||
		!readBE(rp,&type,"type",ANO_FIELD_TYPE_UB1,&rp)) {
		debugEnd();
		return false;
	}
	read(rp,constant,&rp);

	debugWrite("constant: 0x%02x",*constant);

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::getAnoStatusField(const byte_t *rp,
						const byte_t *end,
						uint16_t *status,
						const byte_t **rpout) {
	if (!anoBoundsCheck(rp,end,6,"status field")) {
		return false;
	}

	debugStart("status");

	uint16_t	size;
	uint16_t	type;
	if (!readBE(rp,&size,"size",2,&rp) ||
		!readBE(rp,&type,"type",ANO_FIELD_TYPE_STATUS,&rp)) {
		debugEnd();
		return false;
	}
	readBE(rp,status,&rp);

	debugWrite("status: 0x%04x",*status);

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::sendAnoResponse() {

	resetSendPacketBuffer(PACKET_DATA);

	// ano response header...
	uint16_t	dataflags=0;
	uint16_t	overallsize=13;
	uint64_t	overallsizepos=0;
	uint32_t	version=anorequestversion;
	uint16_t	servicecount=0;
	uint64_t	servicecountpos=0;
	byte_t		servicestobeused=0;

	writeBE(&reqpacket,dataflags);
	writeBE(&reqpacket,(uint32_t)ANO_MARKER);
	overallsizepos=reqpacket.getPosition();
	writeBE(&reqpacket,overallsize);
	writeBE(&reqpacket,version);
	servicecountpos=reqpacket.getPosition();
	writeBE(&reqpacket,servicecount);
	write(&reqpacket,servicestobeused);

	debugStart("ano response header");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("version: 0x%08x",version);
	debugWrite("services to be used: %d",servicestobeused);


	// services...
	overallsize+=putSupervisorService();
	servicecount++;
	overallsize+=putAuthenticationService();
	servicecount++;
	overallsize+=putEncryptionService();
	servicecount++;
	overallsize+=putDataIntegrityService();
	servicecount++;


	// backpatch the overall size and servicecount
	reqpacket.setPositionRelativeToBeginning(overallsizepos);
	reqpacket.write(hostToBE(overallsize));
	reqpacket.setPositionRelativeToBeginning(servicecountpos);
	reqpacket.write(hostToBE(servicecount));

	debugWrite("overall size: %d",overallsize);
	debugWrite("service count: %d",servicecount);
	debugEnd();

	return sendPacket(true);
}

uint16_t sqlrprotocol_oracle::putSupervisorService() {

	debugStart("supervisor");
	debugWrite("service: ANO Supervisor");

	// the driver pair is unexplained.  the status reports success
	// see "Oracle Wire Protocol - ANO Negotiation"
	uint16_t drivers[]={0x0004,0x0001};

	uint16_t	size=putAnoServiceHeader(ANO_SERVICE_SUPERVISOR,3)+
				putAnoVersionField(supervisorversion)+
				putAnoStatusField(ANO_STATUS_SUPERVISOR_OK)+
				putAnoArrayField(drivers,2);

	debugEnd();
	return size;
}

uint16_t sqlrprotocol_oracle::putAuthenticationService() {

	debugStart("authentication");
	debugWrite("service: ANO Authentication");

	// the status reports success.  its bit layout is unexplained
	// see "Oracle Wire Protocol - ANO Negotiation"
	uint16_t	size=putAnoServiceHeader(ANO_SERVICE_AUTHENTICATION,2)+
				putAnoVersionField(authenticationversion)+
				putAnoStatusField(
					ANO_STATUS_AUTHENTICATION_OK);

	debugEnd();
	return size;
}

uint16_t sqlrprotocol_oracle::putEncryptionService() {

	debugStart("encryption");
	debugWrite("service: ANO Encryption");

	// oracle's native network encryption is a diffie-hellman key
	// agreement whose shared secret keys aes, rc4 or 3des for every tns
	// packet after the handshake, with a separate sha or md5 mac.  the
	// ciphers a client that enforces it actually asks for - aes256 above
	// all - aren't in rudiments, which has aes128 and nothing above it,
	// and there's no server-side implementation anywhere to work from:
	// go-ora has only the client half, and the thin drivers don't
	// implement ano at all because it's a thick-mode feature.
	// ENC_NONE is the documented way to decline, and it's what redfern's
	// captured canned response sends.  a client only fails on it if
	// SQLNET.ENCRYPTION_CLIENT is REQUIRED, which raises ORA-12660 on the
	// client; REJECTED, ACCEPTED and REQUESTED all connect unencrypted.
	uint16_t	size=putAnoServiceHeader(ANO_SERVICE_ENCRYPTION,2)+
				putAnoVersionField(encryptionversion)+
				putAnoConstant((byte_t)ENC_NONE);

	debugEnd();
	return size;
}

uint16_t sqlrprotocol_oracle::putDataIntegrityService() {

	debugStart("data integrity");
	debugWrite("service: ANO Data Integrity");

	// declined for the reasons in putEncryptionService()
	uint16_t	size=putAnoServiceHeader(ANO_SERVICE_DATA_INTEGRITY,2)+
				putAnoVersionField(dataintegrityversion)+
				putAnoConstant((byte_t)DI_NONE);

	debugEnd();
	return size;
}

uint16_t sqlrprotocol_oracle::putAnoServiceHeader(uint16_t service,
							uint16_t fieldcount) {

	debugStart("ano service header");
	debugWrite("service: %d",service);
	debugWrite("field count: %d",fieldcount);
	switch (service) {
		case ANO_SERVICE_AUTHENTICATION:
			debugWrite("service name: ANO Authentication");
			break;
		case ANO_SERVICE_ENCRYPTION:
			debugWrite("service name: ANO Encryption");
			break;
		case ANO_SERVICE_DATA_INTEGRITY:
			debugWrite("service name: ANO Data Integrity");
			break;
		case ANO_SERVICE_SUPERVISOR:
			debugWrite("service name: ANO Supervisor");
			break;
		default:
			break;
	}
	debugEnd();

	// service, field count, marker, return total size
	writeBE(&reqpacket,service);
	writeBE(&reqpacket,fieldcount);
	// FIXME; send something other than 0x00000000 if there was an error...
	// no wiki page covers what else may go here
	writeBE(&reqpacket,(uint32_t)0x00000000);
	return 8;
}

uint16_t sqlrprotocol_oracle::putAnoVersionField(uint32_t version) {

	debugStart("version");
	debugWrite("version: 0x%08x",version);
	debugEnd();

	// data size, field type, version, return total size
	writeBE(&reqpacket,(uint16_t)4);
	writeBE(&reqpacket,(uint16_t)ANO_FIELD_TYPE_VERSION);
	writeBE(&reqpacket,version);
	return 8;
}

uint16_t sqlrprotocol_oracle::putAnoStatusField(uint16_t status) {

	debugStart("status");
	debugWrite("status: 0x%04x",status);
	debugEnd();

	// data size, field type, status, return total size
	writeBE(&reqpacket,(uint16_t)2);
	writeBE(&reqpacket,(uint16_t)ANO_FIELD_TYPE_STATUS);
	writeBE(&reqpacket,status);
	return 6;
}

uint16_t sqlrprotocol_oracle::putAnoConstant(byte_t constant) {

	debugStart("constant");
	debugWrite("constant: 0x%02x",constant);
	debugEnd();

	// data size, field type, constant, return total size
	writeBE(&reqpacket,(uint16_t)1);
	writeBE(&reqpacket,(uint16_t)ANO_FIELD_TYPE_UB1);
	write(&reqpacket,constant);
	return 5;
}

uint16_t sqlrprotocol_oracle::putAnoArrayField(uint16_t *array,
						uint32_t arraycount) {

	debugStart("array field");

	// data size, field type
	uint16_t datasize=((arraycount)?(4+2+4+arraycount*2):1);
	writeBE(&reqpacket,(uint16_t)((arraycount)?(4+2+4+arraycount*2):1));
	writeBE(&reqpacket,(uint16_t)ANO_FIELD_TYPE_RAW_BYTES);

	debugWrite("array count: %d",arraycount);

	if (arraycount) {

		// deadbeef marker, array marker,
		// array count, array members
		writeBE(&reqpacket,(uint32_t)ANO_MARKER);
		writeBE(&reqpacket,(uint16_t)ANO_ARRAY_MARKER);
		writeBE(&reqpacket,arraycount);
		for (uint32_t i=0; i<arraycount; i++) {
			debugWrite("array[%d]: %d",i,array[i]);
			writeBE(&reqpacket,array[i]);
		}

	} else {
		// null terminator
		write(&reqpacket,(byte_t)0x00);
	}

	debugEnd();

	// return total size
	return 4+datasize;
}

bool sqlrprotocol_oracle::ttiNegotiation() {
	return recvTtiRequest() && sendTtiResponse();
}

bool sqlrprotocol_oracle::recvTtiRequest() {

	if (!recvPacket()) {
		return false;
	}

	if (resppackettype!=PACKET_DATA) {
		debugWrite("bad packet type %d, expected %d",
						resppackettype,PACKET_DATA);
		return false;
	}

	const byte_t	*rp=resppacket;
	const byte_t	*end=rp+resppacketsize;

	uint16_t	dataflags;
	byte_t		ttccode;
	delete[] clientstring;

	readBE(rp,&dataflags,&rp);
	if (!read(rp,&ttccode,"ttccode",TTC_PROTOCOL_NEGOTIATION,&rp) ||
		!getNullTerminatedArray(rp,end,
					&ttiversions,
					&ttiversioncount,&rp) ||
		!getString(rp,end,&clientstring,&rp)) {
		return false;
	}

	if (getDebug()) {
		debugStart("tti request");
		debugWrite("data flags: 0x%04x",dataflags);
		debugTtcCode(ttccode);
		for (uint32_t i=0; i<ttiversioncount; i++) {
			debugWrite("version[%d]: %d",i,ttiversions[i]);
		}
		debugWrite("client string: \"%s\"",clientstring);
		debugEnd();
	}

	return true;
}

bool sqlrprotocol_oracle::sendTtiResponse() {

	// pick the highest version the client offers that we implement
	// (nothing requires the client's list to be in descending order;
	// anything below TTI_VERSION_MIN is unsupported and rejected below)
	ttiversion=0;
	for (uint32_t i=0; i<ttiversioncount; i++) {
		if (ttiversions[i]>=TTI_VERSION_MIN &&
			ttiversions[i]<=TTI_VERSION_MAX &&
			ttiversions[i]>ttiversion) {
			ttiversion=ttiversions[i];
		}
	}
	debugWrite("selected tti version: %d",ttiversion);

	if (!ttiversion) {
		debugWrite("no supported tti protocol version found");

		// not a refuse packet - the accept has already gone out, and a
		// refuse is only valid before it
		return sendErrorPacket("tti version error",
					ORA_VERSION_NOT_SUPPORTED,
					ORA_VERSION_NOT_SUPPORTED_MESSAGE);
	}

	resetSendPacketBuffer(PACKET_DATA);

	switch (ttiversion) {
		case 6:
			debugWrite("calling putTti6Response");
			putTti6Response();
			break;
		case 5:
			debugWrite("calling putTti5Response");
			putTti5Response();
			break;
	}

	return sendPacket(true);
}

// server compile-time capabilities, captured from a live oracle 11.2 server,
// which reports CCAP_FIELD_VERSION_11_2, with one change.
//
// CCAP_TTC1 bit 0x01 and CCAP_OCI1 bit 0x01 are that server's, 0x7f and 0xff,
// and they have to stay that way.  go-ora reads them as end-of-call-status and
// fast-session-propagate, and then reads an extra field for each off the front
// of every summary object.  they must not be cleared on the grounds that the
// module sends neither field: the module doesn't build its footers field by
// field, it appends byte strings captured from that same server, so both
// fields are in them, unnamed.  measured, with the bits clear: ojdbc 23.26
// hangs forever on a correct login and reports the module's ORA-01017 as an
// ArrayIndexOutOfBoundsException.  the two move together - clearing either bit
// means taking the fields out of every footer.
//
// the array is 42 bytes rather than the 39 a real 11.2 server sends, because
// python-oracledb reads CCAP_TTC4 with bounds checking disabled and no length
// guard.  zero there is also the value we want: it leaves CCAP_END_OF_RESPONSE
// and CCAP_EXPLICIT_BOUNDARY clear, so the client uses the older framing.
//
// CCAP_FIELD_VERSION is a ceiling on what any client will ask of the module
// rather than a promise to it, and putTti6Response() overwrites the value here
// with the version the listener is configured to imitate.  nothing above
// CCAP_FIELD_VERSION_12_1 is offered: it would oblige an oaccolid in every
// describe-info column and five more fields in every execute, and it breaks
// ojdbc 23.26, which logs in at 8 or 9 and then fails in the describe with
// ORA-17401.
static const byte_t	ttiservercompilecaps[]={
	0x06, 0x01, 0x01, 0x01, 0x0d, 0x01, 0x01, CCAP_FIELD_VERSION_11_2,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7f,
	0xff, 0x03, 0x0a, 0x03, 0x03, 0x01, 0x00, 0x7f,
	0x01, 0x7f, 0xff, 0x01, 0x05, 0x01, 0x01, 0x3f,
	0x01, 0x03, 0x06, 0x00, 0x01, 0x03, 0x01, 0x00,
	0x00, 0x00
};

// server runtime capabilities, from the same server
// (RCAP_TTC is RCAP_TTC_ZERO_COPY plus one unnamed bit; RCAP_TTC_32K and
// RCAP_TTC_SESSION_STATE_OPS are clear because the module supports neither
// 32k varchars nor request boundaries)
static const byte_t	ttiserverruntimecaps[]={
	0x02, 0x01, 0x00, 0x01, 0x18, 0x00, 0x03
};

void sqlrprotocol_oracle::putTtiResponse(byte_t version,
					const byte_t *compilecaps,
					byte_t compilecapssize,
					const byte_t *runtimecaps,
					byte_t runtimecapssize) {

	// the layout of this response - the field order, and the
	// ix=6+fdo[5]+fdo[6] rule for finding the character set ids inside
	// the fdo block - follows python-oracledb,
	// src/oracledb/impl/thin/messages/protocol.pyx,
	// _process_protocol_info(), and the capability array index names
	// follow its src/oracledb/impl/thin/constants.pxi.
	//
	// Copyright (c) 2020, 2026, Oracle and/or its affiliates.
	// Taken under the Universal Permissive License 1.0 only, not under
	// python-oracledb's Apache 2.0 option.  See
	// https://oss.oracle.com/licenses/upl and COPYING.

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_PROTOCOL_NEGOTIATION;

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	// protocol version and server banner...
	//
	// this is the server's platform, and it is what an OCI client picks
	// its wire encoding from: one whose own platform matches marshals
	// every request in its own memory layout - 8 byte pointer sentinels,
	// fixed width little endian counts, buffer sizes rather than byte
	// counts - and one whose platform doesn't marshals portably, for the
	// whole session.  this module implements the portable encoding
	// everywhere, so the string it sends has to be one that can never
	// match, and naming a platform SQL Relay merely isn't - solaris, say -
	// is a promise it can't keep, since it builds there.
	//
	// a real server's is its platform - a live 11.2 on centos 5 x64 and a
	// live 12.2 on centos 7 x64 both send "x86_64/Linux 2.4.xx", an 8i,
	// 9i, 10g or 11g on x86 sends "Linuxi386/Linux-2.0.34-8.1.0", and an
	// 8.0.5 sends "Linuxi386/Linux-2.0.34 ", where dropping the trailing
	// space makes the client send a marker after the first phase of
	// authentication.  sending any of them brings the problem back for a
	// client on the same platform, which on a typical deployment is most
	// of them.
	//
	// a client compares this string; it doesn't parse it.  measured:
	// OCI 23.26 goes portable for "Solaris64/SunOS 5.9", for "SQLRelay"
	// and for "SQL Relay 2.3.0" alike, and ojdbc, python-oracledb and
	// node-oracledb take a non-platform string too - their own are
	// "Java_TTC-8.2.0", "python-oracledb" and "node-oracledb".
	serverstring=SERVER_BANNER;

	write(&reqpacket,version);
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,serverstring);
	write(&reqpacket,'\0');


	// database charset.  it goes out twice in two byte orders - this one
	// little-endian, and again big-endian inside the fdo block below.
	// the 8i capture calls this one native byte order.
	writeLE(&reqpacket,charset);


	// server flags
	// (real 11.2 and 12.2 servers send 1; the 8.0-era client this module
	// was originally developed against saw 0, so if that path regresses,
	// this is the first byte to put back)
	write(&reqpacket,(byte_t)1);


	// charset graph elements
	// (a real 11.2 server sends none, and both python-oracledb and go-ora
	// skip the list without reading it, so none is safe)
	uint16_t	charsetgraphelementcount=0;

	writeLE(&reqpacket,charsetgraphelementcount);


	// fdo... (whatever that is)
	// see "Oracle Wire Protocol - TTI Protocol Negotiation"
	uint16_t	fdosize=100;
	uint32_t	fdodatasize=fdosize-4;
	// meaning unknown - only the sizes matter, since the client adds them
	// up to find the charsets that follow
	byte_t	part1[]={
		0x05, 0x0b, 0x0c, 0x03, 0x0c, 0x0c, 0x05, 0x04,
		0x05, 0x0d, 0x06, 0x09, 0x07, 0x08, 0x05, 0x0e,
		0x05, 0x06, 0x05, 0x0f, 0x02, 0xec, 0xeb, 0xed,
		0x05, 0x0a, 0x05, 0x05, 0x05, 0x05, 0x05
	};
	byte_t	part1size=sizeof(part1);
	byte_t	part2[]={
		0x08, 0x23, 0x43, 0x23, 0x23, 0x08, 0x11, 0x23,
		0x08, 0x11, 0x41, 0xb0, 0x23, 0x00, 0x83
	};
	byte_t	part2size=sizeof(part2);

	// 4 bytes of fdodatasize, 3 more of the 1, part1size and part2size,
	// the two parts themselves, then 2+2+1 of charsets and trailer
	uint16_t	fdopadsize=(uint16_t)(fdosize-12-part1size-part2size);

	writeBE(&reqpacket,fdosize);
	writeBE(&reqpacket,fdodatasize);
	write(&reqpacket,(byte_t)1);
	write(&reqpacket,part1size);
	write(&reqpacket,part2size);
	reqpacket.append(part1,part1size);
	reqpacket.append(part2,part2size);


	// charsets, which the client looks for at 6+part1size+part2size
	// bytes into the fdo block
	writeBE(&reqpacket,charset);
	writeBE(&reqpacket,nationalcharset);

	// meaning unknown, but real servers send 0x03 here too
	write(&reqpacket,(byte_t)0x03);

	// pad the fdo block out to fdosize
	for (uint16_t i=0; i<fdopadsize; i++) {
		write(&reqpacket,(byte_t)0);
	}


	// capabilities, each prefixed with a 1-byte length
	// (a version 6 field - a version 5 response ends at the fdo block,
	// with no length byte for either array rather than a zero one)
	if (compilecapssize) {
		write(&reqpacket,compilecapssize);
		reqpacket.append(compilecaps,compilecapssize);
		write(&reqpacket,runtimecapssize);
		reqpacket.append(runtimecaps,runtimecapssize);
	}

	if (getDebug()) {
		debugStart("tti response");
		debugWrite("data flags: 0x%04x",dataflags);
		debugTtcCode(ttccode);
		debugWrite("version: %d",version);
		debugWrite("server string: \"%s\"",serverstring);
		debugWrite("charset: %d",charset);
		debugWrite("national charset: %d",nationalcharset);
		debugWrite("charset graph elements: %d",
					charsetgraphelementcount);
		debugWrite("fdo size: %d",fdosize);
		debugWrite("compile caps size: %d",compilecapssize);
		debugWrite("field version: %d",
			(compilecapssize>CCAP_FIELD_VERSION)?
				compilecaps[CCAP_FIELD_VERSION]:0);
		debugWrite("runtime caps size: %d",runtimecapssize);
		debugEnd();
	}
}

void sqlrprotocol_oracle::putTti6Response() {

	// oracle 8i+ supports TTI 6 (and lower)
	//
	// CCAP_LOGON_TYPES has to track the verifier type the challenge is
	// going to carry.  ojdbc 23.26 picks its logon code path from the bit
	// and its crypto from the verifier type, and refuses the login when
	// they disagree.  it takes two bits, not one: OCI 23.26 reads
	// CCAP_O5LOGON_NP as well, and with only one of the two set it takes
	// the 11g path for the session key size while taking the 12c path for
	// the crypto, which no auth module can verify.  measured one bit at a
	// time: 0x2d and 0x0f both give ORA-01017 for a password that is
	// right, and 0x2f logs in.  a live 12.2 server sets both, and a live
	// 11.2 server sets neither.
	byte_t	compilecaps[sizeof(ttiservercompilecaps)];
	bytestring::copy(compilecaps,ttiservercompilecaps,
					sizeof(ttiservercompilecaps));
	if (verifiertype==VERIFIER_TYPE_12C) {
		compilecaps[CCAP_LOGON_TYPES]|=CCAP_O7LOGON|CCAP_O5LOGON_NP;
	}
	compilecaps[CCAP_FIELD_VERSION]=serverfieldversion;

	if (getDebug()) {
		debugStart("tti 6 response");
		debugWrite("protocol version: %d",ttiversion);
		debugWrite("charset: %d",charset);
		debugWrite("national charset: %d",nationalcharset);
		debugWrite("server flags: 0x%02x",(byte_t)1);
		debugWrite("logon types: 0x%02x",
					compilecaps[CCAP_LOGON_TYPES]);
		debugWrite("field version: 0x%02x",
					compilecaps[CCAP_FIELD_VERSION]);
		debugWrite("compile caps size: %d",
					(int)sizeof(compilecaps));
		debugWrite("runtime caps size: %d",
					(int)sizeof(ttiserverruntimecaps));
		debugEnd();
	}

	putTtiResponse(ttiversion,
			compilecaps,(byte_t)sizeof(compilecaps),
			ttiserverruntimecaps,
			(byte_t)sizeof(ttiserverruntimecaps));
}

void sqlrprotocol_oracle::putTti5Response() {

	// oracle 8.0 supports TTI 5 (and lower)
	// (no capability arrays - they are a version 6 field, and appending
	// them anyway gets ORA-28547 from OCI 23.26 and ORA-17401 from ojdbc
	// 23.26)
	if (getDebug()) {
		debugStart("tti 5 response");
		debugWrite("protocol version: %d",ttiversion);
		debugWrite("charset: %d",charset);
		debugWrite("national charset: %d",nationalcharset);
		debugWrite("server flags: 0x%02x",(byte_t)1);
		debugWrite("no compile or runtime caps sent "
						"(version 5 field)");
		debugEnd();
	}
	putTtiResponse(ttiversion,NULL,0,NULL,0);
}

// the layout of this exchange - the request header, the two length-prefixed
// capability arrays, and the type list, where an entry is a uint16 type and a
// uint16 conversion type, followed by a uint16 representation and a uint16 0
// only when the conversion type is not 0, and the list ends at a type of 0 -
// follows python-oracledb, src/oracledb/impl/thin/messages/data_types.pyx,
// DataTypesMessage._write_message() and _process_message().
//
// Copyright (c) 2021, 2025, Oracle and/or its affiliates.
// Taken under the Universal Permissive License 1.0 only, not under
// python-oracledb's Apache 2.0 option.  See https://oss.oracle.com/licenses/upl
// and COPYING.
//
// the db time zone group is not in python-oracledb, which leaves the runtime
// capability that asks for it clear and so never sees it.  go-ora,
// v2/data_type_nego.go, reads it, and OCI 23.26 asks for it.

// the db time zone, byte for byte as both a live 11.2 and a live 12.2 server
// send it.  go-ora reads bytes 4, 5 and 6 as hours, minutes and seconds biased
// by 60, so three 0x3c is an offset of +00:00.
static const byte_t	dbtimezone[]={
	0x80, 0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x80,
	0x00, 0x00, 0x00
};

// the time zone data file version.  a live 11.2 server sends 11 and a live
// 12.2 server sends 26; the lower version claims fewer time zone regions.
#define DB_TIMEZONE_VERSION	11

bool sqlrprotocol_oracle::dataTypeNegotiation() {

	debugStart("data type negotiation");

	if (!recvDataTypeRequest()) {
		debugWrite("outcome: failed receiving datatype request");
		debugEnd();
		return false;
	}

	if (!sendDataTypeResponse()) {
		debugWrite("outcome: failed sending datatype response");
		debugEnd();
		return false;
	}

	debugWrite("outcome: succeeded");
	debugEnd();
	return true;
}

bool sqlrprotocol_oracle::getCapabilities(const byte_t *rp,
					const byte_t *end,
					const byte_t **caps,
					byte_t *capssize,
					const byte_t **rpout) {

	*caps=NULL;
	*capssize=0;

	if (end-rp<1) {
		debugWrite("truncated capability array size");
		return false;
	}

	byte_t	size;
	read(rp,&size,&rp);

	if ((size_t)(end-rp)<(size_t)size) {
		debugWrite("truncated capability array");
		return false;
	}

	// points into the response packet rather than copying - nothing reads
	// another packet between here and the response
	*caps=rp;
	*capssize=size;

	if (getDebug()) {
		for (byte_t i=0; i<size; i++) {
			debugWrite("capability[%d]: 0x%02x",i,rp[i]);
		}
	}

	*rpout=rp+size;

	return true;
}

uint16_t sqlrprotocol_oracle::countDataTypes(const byte_t *rp,
						const byte_t *end,
						uint16_t *multirepcount) {

	uint16_t	count=0;
	uint16_t	multireps=0;
	for (;;) {

		if (end-rp<2) {
			break;
		}

		uint16_t	datatype;
		readBE(rp,&datatype,&rp);
		if (!datatype) {
			break;
		}

		if (end-rp<2) {
			break;
		}

		uint16_t	convdatatype;
		readBE(rp,&convdatatype,&rp);

		// a type that converts to something is followed by the
		// zero-terminated list of representations the client will
		// accept for it, which is usually one entry long
		if (convdatatype) {
			uint16_t	reps=0;
			for (;;) {
				if (end-rp<2) {
					break;
				}
				uint16_t	rep;
				readBE(rp,&rep,&rp);
				if (!rep) {
					break;
				}
				reps++;
			}
			if (reps>1) {
				multireps++;
			}
		}

		count++;
	}

	debugWrite("total data type count: %d",count);
	debugWrite("data types offered in more than "
			"one representation: %d",multireps);

	*multirepcount=multireps;
	return count;
}

bool sqlrprotocol_oracle::recvDataTypeRequest() {

	if (!recvPacket()) {
		return false;
	}

	if (resppackettype!=PACKET_DATA) {
		debugWrite("bad packet type %d, expected %d",
					resppackettype,PACKET_DATA);
		return false;
	}

	const byte_t	*rp=resppacket;
	const byte_t	*end=rp+resppacketsize;

	uint16_t	dataflags;
	byte_t		ttccode;

	// data flags, ttc code, 2 character sets and the encoding flags
	if (end-rp<8) {
		debugWrite("truncated datatype request header");
		return false;
	}

	readBE(rp,&dataflags,&rp);
	if (!read(rp,&ttccode,"ttccode",TTC_DATATYPE_NEGOTIATION,&rp)) {
		return false;
	}

	// the client's remote-in and remote-out character sets, two ub2s,
	// little-endian.  the 8i capture calls them native byte order rather
	// than little-endian, which is where the two words get used
	// interchangeably; no big-endian client has been measured.
	// every client measured writes the same value in both.  neither is the
	// national character set, which is a separate field further down,
	// inside the db time zone group.
	readLE(rp,&clientcharsetin,&rp);
	readLE(rp,&clientcharsetout,&rp);

	// a bit field, not a marker - 9i and OCI 23.26 send
	// ENCODING_CONV_LENGTH alone, ojdbc 23.26 sends ENCODING_MULTI_BYTE
	// alone, python-oracledb and node-oracledb send both
	read(rp,&encodingflags,&rp);

	// the client's capability arrays
	const byte_t	*compilecaps=NULL;
	byte_t		compilecapssize=0;
	const byte_t	*runtimecaps=NULL;
	byte_t		runtimecapssize=0;
	if (!getCapabilities(rp,end,&compilecaps,&compilecapssize,&rp) ||
		!getCapabilities(rp,end,&runtimecaps,&runtimecapssize,&rp)) {
		return false;
	}

	// the field version is negotiated as the lower of the two
	clientfieldversion=(compilecapssize>CCAP_FIELD_VERSION)?
				compilecaps[CCAP_FIELD_VERSION]:0;
	fieldversion=serverfieldversion;
	if (clientfieldversion<fieldversion) {
		fieldversion=clientfieldversion;
	}

	// the db time zone group, there only if the client asked for it
	clientwantsdbtimezone=(runtimecapssize>RCAP_DB_TIMEZONE &&
				(runtimecaps[RCAP_DB_TIMEZONE]&
					RCAP_DB_TIMEZONE_REQUESTED)!=0);
	clientwantstzversion=(compilecapssize>CCAP_TTC3 &&
				(compilecaps[CCAP_TTC3]&
					CCAP_TTC3_TZ_VERSION)!=0);
	if (clientwantsdbtimezone) {

		size_t	groupsize=sizeof(dbtimezone)+sizeof(uint16_t)+
			((clientwantstzversion)?sizeof(uint32_t):0);
		if ((size_t)(end-rp)<groupsize) {
			debugWrite("truncated db time zone group");
			return false;
		}

		rp+=sizeof(dbtimezone);
		if (clientwantstzversion) {
			readBE(rp,&clienttzversion,&rp);
		}
		readLE(rp,&clientnationalcharset,&rp);
	}

	// the client's data type list, which nothing in the response depends
	// on - a real server answers every client with its own table, so a
	// short or empty list is counted and reported rather than refused
	//
	// talking to the db directly, 8.0.5 sends/receives almost nothing, but
	// talking to relay it sends/receives a ton of stuff - what triggers
	// the difference isn't clear
	datatypes=rp;
	datatypessize=end-rp;
	uint16_t	multirepcount=0;
	datatypecount=countDataTypes(rp,end,&multirepcount);

	// an oci client is the only client measured that offers more than one
	// representation for a type - it offers its platform's and then the
	// universal one for the integer types 25-33, where a real server,
	// ojdbc and the thin drivers all offer exactly one.  it is also the
	// one client that reads a describe's lengths and a negative column
	// scale differently from the rest, so this is what those two
	// decisions are made on.  see putDescribeInfo() and
	// putColumnPrecisionScale().
	ociclient=(multirepcount>0);

	if (getDebug()) {
		debugStart("datatype request");
		debugWrite("data flags: 0x%04x",dataflags);
		debugTtcCode(ttccode);
		debugWrite("client charset in: %d",clientcharsetin);
		debugWrite("client charset out: %d",clientcharsetout);
		debugWrite("encoding flags: 0x%02x%s%s",encodingflags,
			(encodingflags&ENCODING_MULTI_BYTE)?" multibyte":"",
			(encodingflags&ENCODING_CONV_LENGTH)?" convlength":"");
		debugWrite("oci client: %s",(ociclient)?"true":"false");
		debugWrite("client compile caps size: %d",compilecapssize);
		debugHexDump(compilecaps,compilecapssize);
		debugWrite("client runtime caps size: %d",runtimecapssize);
		debugHexDump(runtimecaps,runtimecapssize);
		debugWrite("client field version: %d",clientfieldversion);
		debugWrite("negotiated field version: %d",fieldversion);
		if (clientwantsdbtimezone) {
			debugWrite("client wants the db time zone");
			if (clientwantstzversion) {
				debugWrite("client time zone version: %d",
							clienttzversion);
			}
			debugWrite("client national charset: %d",
						clientnationalcharset);
		}
		debugWrite("data types: %d",datatypecount);
		debugHexDump(datatypes,datatypessize);
		debugWrite("using charset: %d, national charset: %d "
				"(the listener's)",charset,nationalcharset);
		debugEnd();
	}

	// the client states what it will send rather than asking for anything,
	// so this is not a negotiation the module can lose - but a client that
	// disagrees with the listener is worth saying out loud
	if (clientcharsetin!=charset || clientcharsetout!=charset) {
		debugWrite("client charsets %d/%d differ from the "
				"listener's %d, using the listener's",
				clientcharsetin,clientcharsetout,charset);
	}

	return true;
}

// the data types the module supports, captured from a live oracle 11.2 server
// - the same server the capability arrays above came from, and the version the
// module answers as.  each row is the type, the type it converts to, and the
// representation of that type, 1 universal or 10 native.  a conversion type of
// 0 means the type is not exchanged, and its row is 4 bytes on the wire rather
// than 8.
//
// a 12.2 server sends 50 more types than this and disagrees about 11 of them,
// and python-oracledb's client-side table is different again, so this is one
// server's answer rather than a canonical list.  it covers every column type
// the module names.
static const uint16_t	ttidatatypes[][3]={
	// VARCHAR
	{1,1,1},
	// NUMBER
	{2,2,10},
	// LONG
	{8,8,1},
	// DATE
	{12,12,10},
	// RAW
	{23,23,1},
	// LONG_RAW
	{24,24,1},
	{25,25,1},
	{26,26,1},
	{27,27,1},
	{28,28,1},
	{29,29,1},
	{30,30,1},
	{31,31,1},
	{32,32,1},
	{33,33,1},
	{10,10,1},
	// ROWID_DEPRECATED
	{11,11,1},
	{40,40,1},
	{41,41,1},
	{117,117,1},
	{120,120,1},
	{290,290,1},
	{291,291,1},
	{292,292,1},
	{293,293,1},
	{294,294,1},
	{298,298,1},
	{299,299,1},
	{300,300,1},
	{301,301,1},
	{302,302,1},
	{303,303,1},
	{304,304,1},
	{305,305,1},
	{306,306,1},
	{307,307,1},
	{308,308,1},
	{309,309,1},
	{310,310,1},
	{311,311,1},
	{312,312,1},
	{313,313,1},
	{315,315,1},
	{316,316,1},
	{317,317,1},
	{318,318,1},
	{319,319,1},
	{320,320,1},
	{321,321,1},
	{322,322,1},
	{323,323,1},
	{327,327,1},
	{328,328,1},
	{329,329,1},
	{331,331,1},
	{333,333,1},
	{334,334,1},
	{335,335,1},
	{336,336,1},
	{337,337,1},
	{338,338,1},
	{339,339,1},
	{340,340,1},
	{341,341,1},
	{342,342,1},
	{343,343,1},
	{344,344,1},
	{345,345,1},
	{346,346,1},
	{348,348,1},
	{349,349,1},
	{354,354,1},
	{355,355,1},
	{359,359,1},
	{363,363,1},
	{380,380,1},
	{381,381,1},
	{382,382,1},
	{383,383,1},
	{384,384,1},
	{385,385,1},
	{386,386,1},
	{387,387,1},
	{388,388,1},
	{389,389,1},
	{390,390,1},
	{391,391,1},
	{393,393,1},
	{394,394,1},
	{395,395,1},
	{396,396,1},
	{397,397,1},
	{398,398,1},
	{399,399,1},
	{400,400,1},
	{401,401,1},
	{404,404,1},
	{405,405,1},
	{406,406,1},
	{407,407,1},
	{413,413,1},
	{414,414,1},
	{415,415,1},
	{416,416,1},
	{417,417,1},
	{418,418,1},
	{419,419,1},
	{420,420,1},
	{421,421,1},
	{422,422,1},
	{423,423,1},
	{424,424,1},
	{425,425,1},
	{426,426,1},
	{427,427,1},
	{429,429,1},
	{430,430,1},
	{431,431,1},
	{432,432,1},
	{433,433,1},
	{449,449,1},
	{450,450,1},
	{454,454,1},
	{455,455,1},
	{456,456,1},
	{457,457,1},
	{458,458,1},
	{459,459,1},
	{460,460,1},
	{461,461,1},
	{462,462,1},
	{463,463,1},
	{466,466,1},
	{467,467,1},
	{468,468,1},
	{469,469,1},
	{470,470,1},
	{471,471,1},
	{472,472,1},
	{473,473,1},
	{474,474,1},
	{475,475,1},
	{476,476,1},
	{477,477,1},
	{478,478,1},
	{479,479,1},
	{480,480,1},
	{481,481,1},
	{482,482,1},
	{483,483,1},
	{484,484,1},
	{485,485,1},
	{486,486,1},
	{490,490,1},
	{491,491,1},
	{492,492,1},
	{493,493,1},
	{494,494,1},
	{495,495,1},
	{496,496,1},
	{498,498,1},
	{499,499,1},
	{500,500,1},
	{501,501,1},
	{502,502,1},
	{509,509,1},
	{510,510,1},
	{513,513,1},
	{514,514,1},
	{516,516,1},
	{517,517,1},
	{518,518,1},
	{519,519,1},
	{520,520,1},
	{521,521,1},
	{522,522,1},
	{523,523,1},
	{524,524,1},
	{525,525,1},
	{526,526,1},
	{527,527,1},
	{528,528,1},
	{529,529,1},
	{530,530,1},
	{531,531,1},
	{532,532,1},
	{533,533,1},
	{534,534,1},
	{535,535,1},
	{536,536,1},
	{537,537,1},
	{538,538,1},
	{539,539,1},
	{540,540,1},
	{541,541,1},
	{542,542,1},
	{543,543,1},
	{544,0,0},
	{545,0,0},
	{546,0,0},
	{547,0,0},
	{548,0,0},
	{549,0,0},
	{550,0,0},
	{551,0,0},
	{552,0,0},
	{553,0,0},
	{554,0,0},
	{555,0,0},
	{556,0,0},
	{557,0,0},
	{558,0,0},
	{559,0,0},
	{560,560,1},
	{561,0,0},
	{562,0,0},
	{563,563,1},
	{564,564,1},
	{565,565,1},
	{566,0,0},
	{567,0,0},
	{568,0,0},
	{569,0,0},
	{570,0,0},
	{571,0,0},
	{572,572,1},
	{573,573,1},
	{574,574,1},
	{575,575,1},
	{576,576,1},
	{577,0,0},
	{578,578,1},
	{579,579,1},
	{580,580,1},
	{581,581,1},
	{582,582,1},
	{583,583,1},
	{584,584,1},
	{585,585,1},
	{3,2,10},
	{4,2,10},
	{5,1,1},
	// VARNUM
	{6,2,10},
	{7,2,10},
	{9,1,1},
	{13,0,0},
	{14,0,0},
	{15,23,1},
	{16,0,0},
	{17,0,0},
	{18,0,0},
	{19,0,0},
	{20,0,0},
	{21,0,0},
	{22,0,0},
	{39,0,0},
	{58,0,0},
	{68,2,10},
	{69,0,0},
	{70,0,0},
	{74,0,0},
	{76,0,0},
	{91,2,10},
	{94,1,1},
	{95,23,1},
	// CHAR
	{96,96,1},
	{97,96,1},
	{100,100,1},
	{101,101,1},
	// RESULT_SET
	{102,102,1},
	// ROWID
	{104,0,0},
	{105,0,0},
	{106,106,1},
	{108,109,1},
	// NAMED_TYPE
	{109,109,1},
	{110,111,1},
	// REF_TYPE
	{111,111,1},
	// CLOB
	{112,112,1},
	// BLOB
	{113,113,1},
	// BFILE
	{114,114,1},
	{115,115,1},
	{116,102,1},
	{118,0,0},
	{119,0,0},
	{121,0,0},
	{122,0,0},
	{123,0,0},
	{136,0,0},
	{146,146,1},
	{147,0,0},
	{152,2,10},
	{153,2,10},
	{154,2,10},
	{155,1,1},
	{156,12,10},
	{172,2,10},
	{178,178,1},
	{179,179,1},
	// TIMESTAMP
	{180,180,1},
	// TIMESTAMPTZ
	{181,181,1},
	// INTERVALYM
	{182,182,1},
	// INTERVALDS
	{183,183,1},
	{184,12,10},
	{185,0,0},
	{186,0,0},
	{187,0,0},
	{188,0,0},
	{189,0,0},
	{190,0,0},
	{191,0,0},
	{192,0,0},
	{195,112,1},
	{196,113,1},
	{197,114,1},
	{208,208,1},
	{209,0,0},
	// TIMESTAMPLTZ
	{231,231,1},
	{232,231,1},
	{233,233,1},
	{241,109,1},
	{515,0,0}
};

bool sqlrprotocol_oracle::sendDataTypeResponse() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_DATATYPE_NEGOTIATION;

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	// the db time zone, if the client asked for it
	if (clientwantsdbtimezone) {
		reqpacket.append(dbtimezone,sizeof(dbtimezone));
		if (clientwantstzversion) {
			writeBE(&reqpacket,(uint32_t)DB_TIMEZONE_VERSION);
		}
	}

	// the data types, if the client sent a list of its own
	uint16_t	count=0;
	if (datatypessize) {
		count=sizeof(ttidatatypes)/sizeof(ttidatatypes[0]);
		for (uint16_t i=0; i<count; i++) {
			writeBE(&reqpacket,ttidatatypes[i][0]);
			writeBE(&reqpacket,ttidatatypes[i][1]);
			if (ttidatatypes[i][1]) {
				writeBE(&reqpacket,ttidatatypes[i][2]);
				writeBE(&reqpacket,(uint16_t)0);
			}
		}
		writeBE(&reqpacket,(uint16_t)0);
	}

	if (getDebug()) {
		debugStart("datatype response");
		debugWrite("data flags: 0x%04x",dataflags);
		debugTtcCode(ttccode);
		if (clientwantsdbtimezone) {
			debugWrite("db time zone:");
			debugHexDump(dbtimezone,sizeof(dbtimezone));
			if (clientwantstzversion) {
				debugWrite("db time zone version: %d",
						DB_TIMEZONE_VERSION);
			}
		}
		debugWrite("negotiated field version: %d",fieldversion);
		debugWrite("data types: %d",count);
		debugEnd();
	}

	return sendPacket(true);
}

bool sqlrprotocol_oracle::authenticate() {

	for (uint16_t attempt=1;; attempt++) {

		loginrefused=false;

		debugStart("authentication attempt %d",attempt);

		if (recvAuthenticationRequest(false) &&
			sendAuthenticationChallenge() &&
			recvAuthenticationRequest(true) &&
			sendAuthenticationResponse()) {
			debugWrite("outcome: authenticated");
			debugEnd();
			return true;
		}

		// a refused login gets another try, like a real server gives.
		// any other failure doesn't - the exchange broke down partway
		// through, so what the client sends next isn't a login.
		if (!loginrefused || attempt>=maxloginattempts) {
			debugWrite("outcome: %s",(loginrefused)?
					"refused, no attempts left":
					"exchange failed");
			debugEnd();
			return false;
		}

		debugWrite("outcome: refused, retrying");
		debugEnd();

		resetLoginAttempt();
	}
}

void sqlrprotocol_oracle::resetLoginAttempt() {

	debugStart("reset login attempt");
	debugEnd();

	// only what one login built - the connection's negotiated state
	// carries over to the next attempt
	delete[] username;
	username=NULL;
	delete[] response;
	response=NULL;
	delete[] authvfrdata;
	authvfrdata=NULL;
	delete[] authpbkdf2csksalt;
	authpbkdf2csksalt=NULL;
	delete[] serverauthsesskey;
	serverauthsesskey=NULL;
	delete[] clientauthsesskey;
	clientauthsesskey=NULL;
	delete[] authpassword;
	authpassword=NULL;
	gotauthpassword=false;
	fabricatedchallenge=false;
}

// reads a text
// see "Oracle Wire Protocol - Data Types"
bool sqlrprotocol_oracle::getLenString(const byte_t *rp,
					const byte_t *end,
					char **string,
					uint32_t *size,
					const byte_t **rpout) {

	*string=NULL;
	*size=0;

	if (end-rp<1) {
		debugWrite("malformed string: truncated size");
		return false;
	}

	byte_t	length;
	read(rp,&length,&rp);

	// 0xfe introduces a chunked long form that nothing in the
	// authentication exchange uses, so bail rather than desync
	if (length==CLR_LONG_FORM_MARKER) {
		debugWrite("malformed string: chunked, not supported");
		return false;
	}
	if ((size_t)(end-rp)<(size_t)length) {
		debugWrite("malformed string: truncated");
		return false;
	}

	*size=length;
	getString(rp,string,length,&rp);

	*rpout=rp;

	debugWrite("string: %s",*string);

	return true;
}

bool sqlrprotocol_oracle::getAuthCount(const byte_t *rp,
					const byte_t *end,
					uint32_t *value,
					byte_t nativesize,
					const byte_t **rpout) {

	if (!nativeencoding) {
		return readLenPreInt(rp,end,value,rpout);
	}

	*value=0;

	if ((size_t)(end-rp)<(size_t)nativesize) {
		debugWrite("malformed count: truncated");
		return false;
	}

	for (byte_t i=0; i<nativesize; i++) {
		byte_t	b;
		read(rp,&b,&rp);
		if (i<sizeof(uint32_t)) {
			*value|=((uint32_t)b)<<(8*i);
		}
	}

	*rpout=rp;

	debugWrite("count: %d",*value);

	return true;
}

bool sqlrprotocol_oracle::getAuthPointer(const byte_t *rp,
					const byte_t *end,
					const byte_t **rpout) {

	byte_t	size=(nativeencoding)?8:1;

	if ((size_t)(end-rp)<(size_t)size) {
		debugWrite("malformed pointer: truncated");
		return false;
	}

	*rpout=rp+size;

	return true;
}

void sqlrprotocol_oracle::putAuthCount(uint32_t value, byte_t nativesize) {

	debugWrite("count: %d",value);

	if (!nativeencoding) {
		writeLenPreInt(&reqpacket,value);
		return;
	}

	for (byte_t i=0; i<nativesize; i++) {
		write(&reqpacket,(byte_t)((i<sizeof(uint32_t))?
					((value>>(8*i))&0xff):0));
	}
}

bool sqlrprotocol_oracle::getAuthField(const byte_t *rp,
					const byte_t *end,
					char **fieldname,
					char **field,
					uint32_t *fieldsize,
					uint32_t *flags,
					const byte_t **rpout) {

	*fieldname=NULL;
	*field=NULL;
	*fieldsize=0;
	*flags=0;

	debugStart("auth field");

	uint32_t	fieldnamesize=0;
	uint32_t	namesize=0;
	if (!getAuthCount(rp,end,&fieldnamesize,4,&rp) ||
		!getLenString(rp,end,fieldname,&namesize,&rp) ||
		!getAuthCount(rp,end,fieldsize,4,&rp)) {
		debugWrite("malformed auth field: name/size");
		debugEnd();
		return false;
	}

	// the value and its length byte are both omitted when the value size
	// is 0, which is what a client sends when it refuses an AUTH_PASSWORD.
	// a native encoding client's sizes are buffer sizes rather than byte
	// counts, so the string's own length byte is the one that matters.
	uint32_t	valuesize=0;
	if (*fieldsize && !getLenString(rp,end,field,&valuesize,&rp)) {
		debugWrite("malformed auth field: value");
		debugEnd();
		return false;
	}

	if (!getAuthCount(rp,end,flags,4,&rp)) {
		debugWrite("malformed auth field: flags");
		debugEnd();
		return false;
	}

	*rpout=rp;

	debugWrite("%s: %s (flags 0x%04x)",
			*fieldname,(*field)?*field:"",*flags);
	debugEnd();

	return true;
}

bool sqlrprotocol_oracle::getPointer(const byte_t *rp,
					const byte_t *end,
					byte_t *value,
					const byte_t **rpout) {

	debugStart("pointer");

	*value=0;

	if (end-rp<1) {
		debugWrite("malformed pointer: truncated");
		debugEnd();
		return false;
	}

	read(rp,value,&rp);

	*rpout=rp;

	debugWrite("pointer: 0x%02x",*value);
	debugEnd();

	return true;
}

// a text - one length byte, then that many bytes.  there is no long form;
// a value over 252 bytes takes a clr instead
// see "Oracle Wire Protocol - Data Types"
void sqlrprotocol_oracle::putLenString(const char *string, uint32_t size) {
	write(&reqpacket,(byte_t)size);
	write(&reqpacket,string,(size_t)size);
}

// a clr - the text-shaped short form up to 252 bytes, the chunked long form
// above that
// see "Oracle Wire Protocol - Data Types"
void sqlrprotocol_oracle::putLenBytes(const char *bytes, uint32_t size) {

	if (size<=CLR_MAX_SHORT_LENGTH) {
		write(&reqpacket,(byte_t)size);
		if (size) {
			write(&reqpacket,bytes,(size_t)size);
		}
		return;
	}

	// the long form: a 0xfe marker, then a single raw length byte and
	// that many bytes per chunk, then a zero-length chunk
	// (a row value needs it and an authentication field never did, which
	// is why putLenString() doesn't have it)
	write(&reqpacket,(byte_t)CLR_LONG_FORM_MARKER);
	uint32_t	offset=0;
	while (offset<size) {
		uint32_t	chunk=size-offset;
		if (chunk>CLR_MAX_CHUNK_SIZE) {
			chunk=CLR_MAX_CHUNK_SIZE;
		}
		write(&reqpacket,(byte_t)chunk);
		write(&reqpacket,bytes+offset,(size_t)chunk);
		offset+=chunk;
	}
	write(&reqpacket,(byte_t)0);
}

// reads a clr - a length byte, then that many bytes.  0xfd introduces a null
// and 0xfe the chunked long form, which isn't contiguous and so is
// reassembled into the response packet pool, which lives as long as the
// packet the value came out of
// see "Oracle Wire Protocol - Data Types"
bool sqlrprotocol_oracle::getLenBytes(const byte_t *rp,
					const byte_t *end,
					const byte_t **bytes,
					uint32_t *size,
					bool *isnull,
					const byte_t **rpout) {

	*bytes=NULL;
	*size=0;
	*isnull=false;
	*rpout=rp;

	if (end-rp<1) {
		debugWrite("malformed clr: truncated length");
		return false;
	}

	byte_t	length;
	read(rp,&length,&rp);

	// a zero length is a null, and so is the 0xfd marker, which carries a
	// count byte of its own
	if (!length) {
		*isnull=true;
		*rpout=rp;
		return true;
	}
	if (length==CLR_NULL_MARKER) {
		if (end-rp<1) {
			debugWrite("malformed clr: truncated null");
			return false;
		}
		byte_t	nullcount;
		read(rp,&nullcount,&rp);
		*isnull=true;
		*rpout=rp;
		return true;
	}

	// the short form
	if (length<=CLR_MAX_SHORT_LENGTH) {
		if ((size_t)(end-rp)<(size_t)length) {
			debugWrite("malformed clr: truncated");
			return false;
		}
		*bytes=rp;
		*size=length;
		*rpout=rp+length;
		return true;
	}

	if (length!=CLR_LONG_FORM_MARKER) {
		debugWrite("malformed clr: bad length 0x%02x",length);
		return false;
	}

	// the long form: a run of chunks, each a single raw length byte and
	// that many bytes, ended by a zero-length chunk.  (end-rp) is a safe
	// upper bound on the reassembled size, since the length bytes only
	// take space away from it
	if (end-rp<1) {
		debugWrite("malformed clr: truncated chunk");
		return false;
	}
	byte_t		*value=(byte_t *)resppacketpool->allocate(
							(size_t)(end-rp));
	uint32_t	valuesize=0;
	for (;;) {
		if (rp>=end) {
			debugWrite("malformed clr: truncated chunk");
			return false;
		}
		byte_t	chunksize;
		read(rp,&chunksize,&rp);
		if (!chunksize) {
			break;
		}
		if ((size_t)(end-rp)<(size_t)chunksize) {
			debugWrite("malformed clr: truncated chunk");
			return false;
		}
		bytestring::copy(value+valuesize,rp,chunksize);
		rp+=chunksize;
		valuesize+=chunksize;
	}

	*bytes=value;
	*size=valuesize;
	*rpout=rp;

	return true;
}

// a dalc - an lpi total size, then the value as a clr
// see "Oracle Wire Protocol - Data Types"
void sqlrprotocol_oracle::putDalc(const char *bytes, uint32_t size) {
	debugWrite("dalc: %d",size);
	debugHexDump((const byte_t *)bytes,size);
	writeLenPreInt(&reqpacket,size);
	if (size) {
		putLenBytes(bytes,size);
	}
}

bool sqlrprotocol_oracle::getOracleDate(const char *field,
					uint64_t fieldsize,
					byte_t *out) {

	int16_t	year;
	int16_t	month;
	int16_t	day;
	int16_t	hour;
	int16_t	minute;
	int16_t	second;
	int32_t	usec;
	bool	isnegative;
	// FIXME: set ddmm and yyyyddmm somehow
	//
	// a 2-digit year needs century inference - oracle's default date
	// text conversion (eg. "03-MAR-03") carries no century of its own
	if (!datetime::parse(field,false,false,"/-.:",true,
				&year,&month,&day,
				&hour,&minute,&second,
				&usec,&isnegative)) {
		debugWrite("date (failed to parse): \"%.*s\"",
					(int)fieldsize,field);
		return false;
	}

	// parse returns -1 for parts that the text didn't have.  a date
	// column ought to always have a date part, but default everything
	// that's missing, just in case.
	if (year==-1) {
		year=0;
	}
	if (month==-1) {
		month=1;
	}
	if (day==-1) {
		day=1;
	}
	if (hour==-1) {
		hour=0;
	}
	if (minute==-1) {
		minute=0;
	}
	if (second==-1) {
		second=0;
	}

	putOracleDate(out,year,month,day,hour,minute,second);

	return true;
}

void sqlrprotocol_oracle::putOracleDate(byte_t *out) {

	datetime	dt;
	dt.initFromSystemDateTime();

	putOracleDate(out,
			(int16_t)dt.getYear(),
			(int16_t)dt.getMonth(),
			(int16_t)dt.getDayOfMonth(),
			(int16_t)dt.getHour(),
			(int16_t)dt.getMinute(),
			(int16_t)dt.getSecond());
}

void sqlrprotocol_oracle::putOracleDate(byte_t *out,
					int16_t year,
					int16_t month,
					int16_t day,
					int16_t hour,
					int16_t minute,
					int16_t second) {

	out[0]=(byte_t)(year/100+100);
	out[1]=(byte_t)(year%100+100);
	out[2]=(byte_t)month;
	out[3]=(byte_t)day;
	out[4]=(byte_t)(hour+1);
	out[5]=(byte_t)(minute+1);
	out[6]=(byte_t)(second+1);

	debugWrite("date: %04d-%02d-%02d %02d:%02d:%02d",
			year,month,day,hour,minute,second);
}

void sqlrprotocol_oracle::putAuthField(const char *fieldname,
						const char *field,
						uint32_t flags) {

	debugStart("auth field");

	uint32_t	fieldnamesize=charstring::getLength(fieldname);
	uint32_t	fieldsize=charstring::getLength(field);

	putAuthCount(fieldnamesize,4);
	putLenString(fieldname,fieldnamesize);
	putAuthCount(fieldsize,4);
	if (fieldsize) {
		putLenString(field,fieldsize);
	}
	putAuthCount(flags,4);

	debugWrite("%s: %s (flags 0x%04x)",
			fieldname,(field)?field:"",flags);
	debugEnd();
}

void sqlrprotocol_oracle::putAuthField(const char *fieldname,
						const char *field) {
	putAuthField(fieldname,field,0);
}

void sqlrprotocol_oracle::putAuthExtra(stringbuffer *extra, bool secondphase) {

	// the o5logon inputs ride in the credentials' "extra" field because
	// sqlroraclecredentials has 5 fields and o5logon needs 8.  the
	// contract is documented at the top of src/auths/oracle_userlist.cpp.
	bool	pbkdf2=(verifiertype==VERIFIER_TYPE_12C);

	debugStart("auth extra (phase %d)",(secondphase)?2:1);
	debugWrite("field count: %d",(secondphase)?
			((pbkdf2)?7:5):((pbkdf2)?3:2));

	extra->append("verifiertype=")->append(verifiertype);
	debugWrite("verifiertype: %d",verifiertype);
	extra->append(";authvfrdata=")->append(authvfrdata);
	debugWrite("authvfrdata: %s",authvfrdata);
	if (pbkdf2) {
		extra->append(";authpbkdf2vgencount=")->
						append(PBKDF2_VGEN_COUNT);
		debugWrite("authpbkdf2vgencount: %s",PBKDF2_VGEN_COUNT);
	}

	if (!secondphase) {
		debugEnd();
		return;
	}

	// serverauthsesskey is the module's own challenge, handed straight
	// back.  challenge() keeps no state, so decrypting what it produced is
	// the only way the auth module can recover session key part A.
	extra->append(";serverauthsesskey=")->append(serverauthsesskey);
	debugWrite("serverauthsesskey: %s",serverauthsesskey);
	extra->append(";clientauthsesskey=")->append(clientauthsesskey);
	debugWrite("clientauthsesskey: %s",clientauthsesskey);
	if (pbkdf2) {
		extra->append(";authpbkdf2csksalt=")->append(authpbkdf2csksalt);
		debugWrite("authpbkdf2csksalt: %s",authpbkdf2csksalt);
		extra->append(";authpbkdf2sdercount=")->
						append(PBKDF2_SDER_COUNT);
		debugWrite("authpbkdf2sdercount: %s",PBKDF2_SDER_COUNT);
	}

	debugEnd();
}

bool sqlrprotocol_oracle::recvAuthenticationRequest(bool secondphase) {

	if (!recvPacket()) {
		return false;
	}

	if (resppackettype!=PACKET_DATA) {
		debugWrite("bad packet type %d, expected %d",
						resppackettype,PACKET_DATA);
		return false;
	}

	const byte_t	*rp=resppacket;
	const byte_t	*end=rp+resppacketsize;

	uint16_t	dataflags;
	byte_t		ttccode;
	byte_t		ttifunction;
	byte_t		seqnumber;

	// data flags, ttc code, tti function, sequence number.
	// the two tti function reads share one byte - read() rewinds the
	// read pointer when the value doesn't match.
	if (end-rp<5) {
		debugWrite("bad authentication request, truncated header");
		return false;
	}

	readBE(rp,&dataflags,&rp);
	if (!read(rp,&ttccode,"ttccode",TTC_TTI_FUNCTION,&rp)) {
		return false;
	}
	if (!secondphase) {
		if (!read(rp,&ttifunction,"ttifunction",
				TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY,&rp) &&
			!read(rp,&ttifunction,"ttifunction",
				TTI_LOGON_PRESENT_USER,&rp)) {
			return false;
		}
	} else {
		if (!read(rp,&ttifunction,"ttifunction",
				TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD,&rp) &&
			!read(rp,&ttifunction,"ttifunction",
				TTI_LOGON_PRESENT_PWD,&rp)) {
			return false;
		}
	}
	read(rp,&seqnumber,&rp);

	// which of the two wire encodings the client uses is decided by the
	// platform banner the module answered the tti protocol negotiation
	// with, and the first pointer field tells them apart - 0x01 against
	// the first byte of the native sentinel, 0xfe.  phase two keeps
	// whatever phase one decided.
	// (the native encoding isn't something the module provokes - an OCI
	// client sends the same bytes to a real oracle server, and a real
	// server answers in kind)
	if (!secondphase) {
		nativeencoding=(end-rp>=1 && *rp==0xfe);
	}

	debugStart("authentication request (phase %d)",(secondphase)?2:1);
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugTtiFunction(ttifunction);
	debugWrite("seq number: %d",seqnumber);
	debugWrite("encoding: %s",(nativeencoding)?"native":"portable");

	uint32_t	usersize=0;
	uint32_t	authmode=0;
	uint32_t	fieldcount=0;
	if (!getAuthPointer(rp,end,&rp) ||
		!getAuthCount(rp,end,&usersize,4,&rp) ||
		!getAuthCount(rp,end,&authmode,4,&rp) ||
		!getAuthPointer(rp,end,&rp) ||
		!getAuthCount(rp,end,&fieldcount,8,&rp) ||
		!getAuthPointer(rp,end,&rp) ||
		!getAuthPointer(rp,end,&rp)) {
		debugEnd();
		return false;
	}

	debugWrite("auth mode: 0x%08x",authmode);
	debugWrite("field count: %d",fieldcount);

	// whether the user name is length prefixed is a client difference, not
	// an encoding one.  python-oracledb, node-oracledb and OCI prefix it,
	// and ojdbc writes it raw and takes its length from the count above.
	// nor is the count the same thing for all of them: for the first three
	// it is the name's byte count, and for OCI it is a buffer size - the
	// character count times the bytes per character of its charset - so an
	// 8 character name in AL32UTF8 is declared as 24.  these are OCI
	// properties, not native encoding properties, and OCI does them in
	// the portable encoding too, which is where every client ends up now
	// that the module answers a banner none of them match.
	//
	// so the prefix is taken when the next byte can be one: the bytes are
	// there for it, and either it is below a space - no user name starts
	// with a control character - or the count in front of it is that byte
	// times the 1, 2, 3 or 4 bytes per character a charset can have, which
	// is all a buffer size ever is.
	bool	lengthprefixed=nativeencoding;
	if (!lengthprefixed && rp<end && (size_t)(end-rp)>(size_t)*rp) {
		uint32_t	prefix=*rp;
		lengthprefixed=(prefix<' ' ||
				(prefix &&
					(usersize==prefix ||
					usersize==prefix*2 ||
					usersize==prefix*3 ||
					usersize==prefix*4)));
	}

	char	*user=NULL;
	if (lengthprefixed) {
		uint32_t	realusersize=0;
		if (!getLenString(rp,end,&user,&realusersize,&rp)) {
			debugEnd();
			return false;
		}
	} else {
		if ((size_t)(end-rp)<(size_t)usersize) {
			debugWrite("bad user size: %d",usersize);
			debugEnd();
			return false;
		}
		getString(rp,&user,usersize,&rp);
	}
	debugWrite("user: %s",user);
	if (secondphase) {
		// phase two names the user again - refuse one that answers
		// a challenge built for somebody else
		bool	sameuser=!charstring::compare(user,username);
		delete[] user;
		if (!sameuser) {
			debugWrite("user changed between phases");
			debugEnd();
			return false;
		}
	} else {
		delete[] username;
		username=user;
	}

	// declared out here so every exit from the loop frees them
	char	*fieldname=NULL;
	char	*field=NULL;

	for (uint32_t i=0; i<fieldcount && rp<end; i++) {

		// free what the previous pass allocated
		delete[] fieldname;
		fieldname=NULL;
		delete[] field;
		field=NULL;

		uint32_t	fieldsize=0;
		uint32_t	flags=0;
		if (!getAuthField(rp,end,&fieldname,&field,
						&fieldsize,&flags,&rp)) {
			break;
		}

		debugWrite("%s: %s (flags 0x%04x)",
				fieldname,(field)?field:"",flags);

		if (!secondphase) {
			continue;
		}

		// a zero length AUTH_PASSWORD is a normal thing to receive, so
		// record that it arrived separately from what it held
		if (!charstring::compare(fieldname,"AUTH_PASSWORD")) {
			gotauthpassword=true;
			delete[] authpassword;
			authpassword=charstring::duplicate(field);
		} else if (!charstring::compare(fieldname,"AUTH_SESSKEY")) {
			delete[] clientauthsesskey;
			clientauthsesskey=charstring::duplicate(field);
		}
	}

	delete[] fieldname;
	delete[] field;

	debugEnd();

	return true;
}

bool sqlrprotocol_oracle::sendAuthenticationChallenge() {

	bool	pbkdf2=(verifiertype==VERIFIER_TYPE_12C);

	// a real server's AUTH_VFR_DATA is the user's stored verifier salt.
	// SQL Relay has none, so it generates a fresh one per login.
	delete[] authvfrdata;
	authvfrdata=generateHex((pbkdf2)?VFR_DATA_SIZE_12C:VFR_DATA_SIZE_11G);
	delete[] authpbkdf2csksalt;
	authpbkdf2csksalt=(pbkdf2)?generateHex(PBKDF2_CSK_SALT_SIZE):NULL;

	// let an auth module build session key part A
	stringbuffer	extra;
	putAuthExtra(&extra,false);

	sqlroraclecredentials	cred;
	cred.setUser(username);
	cred.setMethod("O5LOGON");
	cred.setExtra(extra.getString());

	// a false return means no auth module knows the user, or has its
	// password under a one-way encryption, or supports the method.
	// answering the error here would end the exchange a round trip early
	// and tell a client which user names exist.  real oracle fabricates a
	// verifier for a user it doesn't have and runs the whole exchange
	// anyway, so an unknown user looks exactly like a wrong password.
	stringbuffer	challenge;
	fabricatedchallenge=!cont->challenge(&cred,&challenge);
	if (fabricatedchallenge) {
		debugWrite("challenge failed, fabricating one");
		char	*fake=generateHex((pbkdf2)?
					SESSION_KEY_SIZE_12C:
					SESSION_KEY_SIZE_11G);
		challenge.append(fake);
		delete[] fake;
	}

	delete[] serverauthsesskey;
	serverauthsesskey=challenge.detachString();

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;

	// trailer after the last pair, from an 11.2 capture.  it is a summary
	// object, so putAuthTrailer() adds the 12.1 fields to it.
	// its fields are unexplained
	// see "Oracle Wire Protocol - Authentication - Username"
	static const byte_t	trailer[]={
		0x04, 0x01, 0x01, 0x01, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	debugStart("authentication challenge");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("verifier type: %d (%s)",
			verifiertype,(pbkdf2)?"12c":"11g");
	debugWrite("authvfrdata: %s",authvfrdata);
	if (pbkdf2) {
		debugWrite("authpbkdf2csksalt: %s",authpbkdf2csksalt);
	}
	debugWrite("serverauthsesskey: %s",serverauthsesskey);
	debugWrite("fabricated challenge: %s",
			(fabricatedchallenge)?"yes":"no");

	// a request's pair count is 8 bytes in the native encoding, a
	// response's is 2
	putAuthCount((pbkdf2)?6:3,2);

	// AUTH_SESSKEY is 48 bytes for an 11g verifier and 32 for a 12c one,
	// and the client picks its code path from that length, not from the
	// verifier type it was told
	putAuthField("AUTH_SESSKEY",serverauthsesskey);

	// the verifier type travels in AUTH_VFR_DATA's flags ub4, and nowhere
	// else
	putAuthField("AUTH_VFR_DATA",authvfrdata,verifiertype);

	if (pbkdf2) {
		putAuthField("AUTH_PBKDF2_CSK_SALT",authpbkdf2csksalt);
		putAuthField("AUTH_PBKDF2_VGEN_COUNT",PBKDF2_VGEN_COUNT);
		putAuthField("AUTH_PBKDF2_SDER_COUNT",PBKDF2_SDER_COUNT);
	}

	// constant per database on a real server, and only informational
	putAuthField("AUTH_GLOBALLY_UNIQUE_DBID",
			"00000000000000000000000000000000");

	putAuthTrailer(trailer,sizeof(trailer),false);

	debugEnd();

	return sendPacket(true);
}

void sqlrprotocol_oracle::putAuthTrailer(const byte_t *portable,
						size_t portablesize,
						bool secondphase) {

	debugStart("auth trailer (phase %d)",(secondphase)?2:1);
	debugWrite("encoding: %s",(nativeencoding)?"native":"portable");

	// the portable trailer is a summary object, so it owes the two fields
	// a 12.1 server adds
	if (!nativeencoding) {
		debugWrite("bytes: %d",(uint32_t)portablesize);
		debugHexDump(portable,portablesize);
		reqpacket.append(portable,portablesize);
		putSummaryExtension(0,0);
		debugEnd();
		return;
	}

	// a marshalled struct rather than a field stream, so it gets no
	// summary extension.  no client reaches it now that the module answers
	// a banner none of them match; it is kept as the fallback if one ever
	// does.  the live pointer value the 11.2 capture carried is zeroed.
	// the rest of the trailer is unexplained
	// see "Oracle Wire Protocol - Authentication - Username"
	static const byte_t	nativetrailer[]={
		0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x36, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	// the two bytes that differ between the phases
	byte_t	phase=(secondphase)?3:2;

	debugWrite("bytes: %d",(uint32_t)sizeof(nativetrailer));
	debugWrite("phase byte: 0x%02x",phase);
	debugHexDump(nativetrailer,sizeof(nativetrailer));

	reqpacket.append(nativetrailer,5);
	write(&reqpacket,phase);
	reqpacket.append(nativetrailer+6,43);
	write(&reqpacket,phase);
	reqpacket.append(nativetrailer+50,sizeof(nativetrailer)-50);

	debugEnd();
}

bool sqlrprotocol_oracle::sendAuthenticationResponse() {

	// the unknown-user answer comes first, ahead of the empty-password
	// one, because that's the order a real server uses: an unknown user
	// with an empty AUTH_PASSWORD gets ORA-01017 where a known one gets
	// ORA-01005.  checking the other way round would hand back the
	// distinction between an unknown user and a wrong password.
	if (fabricatedchallenge) {
		debugWrite("fabricated challenge, refusing");
		return sendAuthenticationError(
				ORA_INVALID_USERNAME_PASSWORD,
				"ORA-01017: invalid username/password; "
				"logon denied\n");
	}

	// a zero length AUTH_PASSWORD is what an 11g client sends when it
	// couldn't validate the padding in the challenge, and a real server
	// answers it with its own error too
	if (!gotauthpassword || charstring::isNullOrEmpty(authpassword)) {
		debugWrite("no auth password");
		return sendAuthenticationError(ORA_NULL_PASSWORD,
				"ORA-01005: null password given; "
				"logon denied\n");
	}

	stringbuffer	extra;
	putAuthExtra(&extra,true);

	sqlroraclecredentials	cred;
	cred.setUser(username);
	cred.setPassword(authpassword);
	cred.setPasswordSize(charstring::getLength(authpassword));
	cred.setMethod("O5LOGON");
	cred.setExtra(extra.getString());

	if (!cont->auth(&cred)) {
		debugWrite("auth failed");
		return sendAuthenticationError(
				ORA_INVALID_USERNAME_PASSWORD,
				"ORA-01017: invalid username/password; "
				"logon denied\n");
	}
	debugWrite("auth succeeded");

	// AUTH_SVR_RESPONSE proves to the client that the server knew the
	// password too, and a real client refuses the login without it.  the
	// combo key it's built from lives in the auth module, so it comes back
	// through challenge() under a second method name.
	cred.setMethod("O5LOGON-SERVER-RESPONSE");
	stringbuffer	svrresponse;
	if (!cont->challenge(&cred,&svrresponse)) {
		debugWrite("server response failed");
		return sendAuthenticationError(
				ORA_INVALID_USERNAME_PASSWORD,
				"ORA-01017: invalid username/password; "
				"logon denied\n");
	}

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;

	// the phase two trailer, from an 11.2 capture, and not identified
	// either.  it differs from the phase one trailer in two bytes.
	// see "Oracle Wire Protocol - Authentication - Password"
	static const byte_t	trailer[]={
		0x04, 0x01, 0x01, 0x01, 0x03,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	debugStart("authentication response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("user: %s",username);
	debugWrite("server version sql: %s",serverversionsql);
	debugWrite("server version no: %s",serverversionno);

	// a real server sends 39 to 44 pairs here, mostly nls settings.  only
	// AUTH_SVR_RESPONSE is known to be required.
	putAuthCount(9,2);

	// what both live servers send here
	putAuthField("AUTH_VERSION_STRING","- 64bit Production");

	putAuthField("AUTH_VERSION_SQL",serverversionsql);
	putAuthField("AUTH_XACTION_TRAITS","3");

	// the version a client reports as the server's, and it is not cosmetic.
	// ojdbc 23.26 picks its result set reader from it - told 8.0.5, it
	// reads the module's 11.2 shaped describe with an 8.0 era reader and
	// throws ORA-17401 on the first query.  it moves with the field version
	// and the verifier type, since telling a client one version and
	// answering it as another breaks the login and the first query.
	putAuthField("AUTH_VERSION_NO",serverversionno);
	putAuthField("AUTH_VERSION_STATUS","0");
	putAuthField("AUTH_CAPABILITY_TABLE","");
	putAuthField("AUTH_SESSION_ID","9");
	putAuthField("AUTH_SERIAL_NUM","1981");
	putAuthField("AUTH_SVR_RESPONSE",svrresponse.getString());

	putAuthTrailer(trailer,sizeof(trailer),true);

	debugEnd();

	return sendPacket(true);
}

bool sqlrprotocol_oracle::sendAuthenticationError(uint32_t oranum,
						const char *message) {
	loginrefused=true;
	return sendErrorPacket("authentication error",oranum,message);
}

bool sqlrprotocol_oracle::sendErrorPacket(const char *what,
						uint32_t oranum,
						const char *message) {

	// a refuse packet can't be used after the accept - it's an ns layer
	// packet type, and a client that has already been accepted reads one
	// as a data packet and desyncs.  returning false without writing this
	// reads as a dropped socket, ORA-03113 or ORA-12537, not a refusal.
	// putError() can't be reused either; its layout doesn't parse as a ub4
	// stream.
	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_ERROR;

	// a ub4 stream: a 1, two zeros, the ora number, then a run of zero
	// ub4s, then the message.  reproduced byte for byte from an 11.2
	// server, since what the client's parser keys off isn't known.
	// see "Oracle Wire Protocol - Authentication - Password"
	static const byte_t	prefix[]={
		0x01, 0x01, 0x00, 0x00
	};
	static const byte_t	suffix[]={
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00,
		0x02, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};

	// the same packet in the native encoding, a marshalled struct whose
	// only identifiable field is the ora number, at offset 11 as a little
	// endian uint32.  the live pointer value the capture carried is zeroed.
	static const byte_t	nativeprefix[]={
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x00
	};
	static const byte_t	nativesuffix[]={
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x36, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	debugStart("%s",what);
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("encoding: %s",(nativeencoding)?"native":"portable");
	debugWrite("error: %d",oranum);
	debugWrite("message: %s",message);

	if (nativeencoding) {
		reqpacket.append(nativeprefix,sizeof(nativeprefix));
		putAuthCount(oranum,4);
		reqpacket.append(nativesuffix,sizeof(nativesuffix));
	} else {
		reqpacket.append(prefix,sizeof(prefix));
		writeLenPreInt(&reqpacket,oranum);
		reqpacket.append(suffix,sizeof(suffix));
		putSummaryExtension(oranum,0);
	}
	putLenString(message,charstring::getLength(message));

	debugEnd();

	sendPacket(true);

	// the error is sent, but the exchange it interrupted has failed
	return false;
}
bool sqlrprotocol_oracle::open(const byte_t *rp) {

	// sqlplus 8.0.5, 8i, 9i
	// call this to open a cursor
	// sqlplus 10g+ use query3

	const byte_t	*end=resppacket+resppacketsize;

	byte_t		seqnumber=0;
	byte_t		cursoridpointer=0;
	uint32_t	opesiz=0;

	if (end-rp<1) {
		debugWrite("truncated open sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	// a pointer flag for the cursor id, always 1 in the capture - the
	// server allocates the cursor and returns its id, so no cursor id
	// follows it here - then the open size (opesiz), meaning unknown
	// see "Oracle Wire Protocol - Open"
	if (!getPointer(rp,end,&cursoridpointer,&rp) ||
		!readLenPreInt(rp,end,&opesiz,&rp)) {
		return false;
	}

	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		debugWrite("couldn't get cursor");
		return sendCursorNotOpenError();
	}

	uint16_t	cursorid=cont->getId(cursor);
	lastcursorid=cursorid;

	debugStart("open request");
	debugWrite("seq number: %d",seqnumber);
	debugWrite("cursor id pointer: 0x%02x",cursoridpointer);
	debugWrite("open size: %d",opesiz);
	debugWrite("cursor id: %d",cursorid);
	debugEnd();

	return sendOpenResponse(cursor);
}

bool sqlrprotocol_oracle::sendOpenResponse(sqlrservercursor *cursor) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;
	uint16_t	cursorid=cont->getId(cursor);
	// unexplained.  see "Oracle Wire Protocol - Open"
	byte_t		unknown[]={0x00, 0x00};
	byte_t		status=TTC_STATUS;

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	// the id on the wire is this module's own cursor id plus 1 - the
	// same convention close() and occa() use when reading a cursor id
	// back, so a client that echoes this id back in a later close()
	// finds the cursor again
	writeLenPreInt(&reqpacket,(uint32_t)(cursorid+1));
	reqpacket.append(unknown,sizeof(unknown));
	write(&reqpacket,status);

	debugStart("open response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("cursor id: %d",cursorid);
	debugWrite("unknown: %02x %02x",unknown[0],unknown[1]);
	debugTtcCode(status);
	debugEnd();

	return sendPacket(true);
}

void sqlrprotocol_oracle::debugTtcCode(byte_t ttccode) {
	if (!getDebug()) {
		return;
	}
	const char	*code=NULL;
	switch (ttccode) {
		case TTC_PROTOCOL_NEGOTIATION:
			code="TTC_PROTOCOL_NEGOTIATION";
			break;
		case TTC_DATATYPE_NEGOTIATION:
			code="TTC_DATATYPE_NEGOTIATION";
			break;
		case TTC_TTI_FUNCTION:
			code="TTC_TTI_FUNCTION";
			break;
		case TTC_ERROR:
			code="TTC_ERROR";
			break;
		case TTC_ACCESS_USER_ADDRESS_SPACE:
			code="TTC_ACCESS_USER_ADDRESS_SPACE";
			break;
		case TTC_ROW_HEADER:
			code="TTC_ROW_HEADER";
			break;
		case TTC_ROW_DATA:
			code="TTC_ROW_DATA";
			break;
		case TTC_OK:
			code="TTC_OK";
			break;
		case TTC_STATUS:
			code="TTC_STATUS";
			break;
		case TTC_NUMBER_OF_ERROR_RECORDS:
			code="TTC_NUMBER_OF_ERROR_RECORDS";
			break;
		case TTC_IO_VECTOR:
			code="TTC_IO_VECTOR";
			break;
		case TTC_SEND_LONG:
			code="TTC_SEND_LONG";
			break;
		case TTC_ORACLE_ACCESSOR:
			code="TTC_ORACLE_ACCESSOR";
			break;
		case TTC_LOB_AND_BFILE_DATA:
			code="TTC_LOB_AND_BFILE_DATA";
			break;
		case TTC_WARNING_MESSAGES:
			code="TTC_WARNING_MESSAGES";
			break;
		case TTC_DESCRIBE_INFO:
			code="TTC_DESCRIBE_INFO";
			break;
		case TTC_BIT_VECTOR:
			code="TTC_BIT_VECTOR";
			break;
		case TTC_END_OF_BIND:
			code="TTC_END_OF_BIND";
			break;
		case TTC_SERVER_PIGGYBACK_FUNCTION:
			code="TTC_SERVER_PIGGYBACK_FUNCTION";
			break;
		case TTC_ONE_WAY_FUNCTION:
			code="TTC_ONE_WAY_FUNCTION";
			break;
		case TTC_IMPLICIT_RESULT_SET:
			code="TTC_IMPLICIT_RESULT_SET";
			break;
		case TTC_RENEGOTIATE:
			code="TTC_RENEGOTIATE";
			break;
		case TTC_END_OF_RESPONSE:
			code="TTC_END_OF_RESPONSE";
			break;
		case TTC_PIGGYBACK_TTI_FUNCTION:
			code="TTC_PIGGYBACK_TTI_FUNCTION";
			break;
		case TTC_UNTRUSTED_CALLOUTS:
			code="TTC_UNTRUSTED_CALLOUTS";
			break;
		case TTC_FLUSH_OUT_BINDS:
			code="TTC_FLUSH_OUT_BINDS";
			break;
		case TTC_EXTPROC1:
			code="TTC_EXTPROC1";
			break;
		case TTC_TOKEN:
			code="TTC_TOKEN";
			break;
		case TTC_FAST_AUTHENTICATION:
			code="TTC_FAST_AUTHENTICATION";
			break;
		case TTC_EXTPROC2:
			code="TTC_EXTPROC2";
			break;
		case TTC_SECURE_NETWORK_SERVICES:
			code="TTC_SECURE_NETWORK_SERVICES";
			break;
		default:
			code="UNKNOWN";
			break;
	}
	debugWrite("ttc code: (0x%02x) %s",ttccode,code);
}

void sqlrprotocol_oracle::debugTtiFunction(byte_t ttifunction) {
	if (!getDebug()) {
		return;
	}
	const char	*func=NULL;
	switch (ttifunction) {
		case TTI_OPEN:
			func="TTI_OPEN";
			break;
		case TTI_QUERY:
			func="TTI_QUERY";
			break;
		case TTI_EXECUTE:
			func="TTI_EXECUTE";
			break;
		case TTI_FETCH:
			func="TTI_FETCH";
			break;
		case TTI_CLOSE:
			func="TTI_CLOSE";
			break;
		case TTI_DISCONNECT:
			func="TTI_DISCONNECT";
			break;
		case TTI_AUTOCOMMIT_ON:
			func="TTI_AUTOCOMMIT_ON";
			break;
		case TTI_AUTOCOMMIT_OFF:
			func="TTI_AUTOCOMMIT_OFF";
			break;
		case TTI_COMMIT:
			func="TTI_COMMIT";
			break;
		case TTI_ROLLBACK:
			func="TTI_ROLLBACK";
			break;
		case TTI_CANCEL:
			func="TTI_CANCEL";
			break;
		case TTI_DESCRIBE:
			func="TTI_DESCRIBE";
			break;
		case TTI_STARTUP:
			func="TTI_STARTUP";
			break;
		case TTI_SHUTDOWN:
			func="TTI_SHUTDOWN";
			break;
		case TTI_VERSION:
			func="TTI_VERSION";
			break;
		case TTI_K2_TRANSACTIONS:
			func="TTI_K2_TRANSACTIONS";
			break;
		case TTI_QUERY2:
			func="TTI_QUERY2";
			break;
		case TTI_OSQL7:
			func="TTI_OSQL7";
			break;
		case TTI_OKOD:
			func="TTI_OKOD";
			break;
		case TTI_QUERY3:
			func="TTI_QUERY3";
			break;
		case TTI_LOB_OPERATIONS:
			func="TTI_LOB_OPERATIONS";
			break;
		case TTI_ODNY:
			func="TTI_ODNY";
			break;
		case TTI_TRANSACTION_END:
			func="TTI_TRANSACTION_END";
			break;
		case TTI_TRANSACTION_BEGIN:
			func="TTI_TRANSACTION_BEGIN";
			break;
		case TTI_OCCA:
			func="TTI_OCCA";
			break;
		case TTI_STARTUP2:
			func="TTI_STARTUP2";
			break;
		case TTI_LOGON_PRESENT_PWD:
			func="TTI_LOGON_PRESENT_PWD";
			break;
		case TTI_LOGON_PRESENT_USER:
			func="TTI_LOGON_PRESENT_USER";
			break;
		case TTI_UNIDENTIFIED_0X54:
			func="TTI_UNIDENTIFIED_0X54";
			break;
		case TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD:
			func="TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD";
			break;
		case TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY:
			func="TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY";
			break;
		case TTI_DESCRIBE2:
			func="TTI_DESCRIBE2";
			break;
		case TTI_OOTCM:
			func="TTI_OOTCM";
			break;
		case TTI_OKPFC:
			func="TTI_OKPFC";
			break;
		case TTI_SWITCH_SESSION:
			func="TTI_SWITCH_SESSION";
			break;
		case TTI_CLOSE2:
			func="TTI_CLOSE2";
			break;
		case TTI_OSCID:
			func="TTI_OSCID";
			break;
		case TTI_OSKEYVAL:
			func="TTI_OSKEYVAL";
			break;
		default:
			func="UNKNOWN";
			break;
	}
	debugWrite("tti function: (0x%02x) %s",ttifunction,func);
}

void sqlrprotocol_oracle::debugOptions(uint16_t options,
					uint16_t moreoptions) {
	if (!getDebug()) {
		return;
	}

	debugStart("options");
	debugWrite("0x%04x",options);
	stringbuffer	b;
	b.printBits(hostToBE(options));
	debugWrite("%s",b.getString());
	debugOptions(options);
	debugEnd();

	debugStart("moreoptions");
	debugWrite("0x%04x",moreoptions);
	b.clear();
	b.printBits(hostToBE(moreoptions));
	debugWrite("%s",b.getString());
	debugOptions(moreoptions);
	debugEnd();
}

void sqlrprotocol_oracle::debugOptions(uint32_t options) {
	if (options&OPTION_PARSE) {
		debugWrite("OPTION_PARSE");
	}
	if (options&OPTION_BIND) {
		debugWrite("OPTION_BIND");
	}
	if (options&OPTION_DEFINE) {
		debugWrite("OPTION_DEFINE");
	}
	if (options&OPTION_EXECUTE) {
		debugWrite("OPTION_EXECUTE");
	}
	if (options&OPTION_FETCH) {
		debugWrite("OPTION_FETCH");
	}
	if (options&OPTION_CANCEL) {
		debugWrite("OPTION_CANCEL");
	}
	if (options&OPTION_COMMIT) {
		debugWrite("OPTION_COMMIT");
	}
	if (options&OPTION_EXACTFETCH) {
		debugWrite("OPTION_EXACTFETCH");
	}
	if (options&OPTION_SNDIOV) {
		debugWrite("OPTION_SNDIOV");
	}
	if (options&OPTION_NOPLSQL) {
		debugWrite("OPTION_NOPLSQL");
	}
	if (options&OPTION_DESCRIBE) {
		debugWrite("OPTION_DESCRIBE");
	}
}

void sqlrprotocol_oracle::debugCharacterSet(byte_t characterset) {
	if (!getDebug()) {
		return;
	}
	debugWrite("character set: 0x%02x",(uint32_t)(0x000000ff&characterset));
}

void sqlrprotocol_oracle::debugStatusFlags(uint16_t statusflags) {
	if (!getDebug()) {
		return;
	}
	debugStart("status flags");
	debugWrite("0x%04x",statusflags);
	stringbuffer	b;
	b.printBits(statusflags);
	debugWrite("%s",b.getString());
	/*if (statusflags&SERVER_STATUS_IN_TRANS) {
		debugWrite("SERVER_STATUS_IN_TRANS");
	}*/
	debugEnd();
}

void sqlrprotocol_oracle::debugColumnType(const char *name,
						uint16_t columntype) {
	if (!getDebug()) {
		return;
	}
	debugWrite("type: %s (0x%02x)",name,(uint32_t)(0x000000ff&columntype));
	debugColumnType(columntype);
}

void sqlrprotocol_oracle::debugColumnType(uint16_t columntype) {
	if (!getDebug()) {
		return;
	}
	switch (columntype) {
		case ORACLE_TYPE_VARCHAR:
			debugWrite("ORACLE_TYPE_VARCHAR");
			break;
		case ORACLE_TYPE_NUMBER:
			debugWrite("ORACLE_TYPE_NUMBER");
			break;
		case ORACLE_TYPE_VARNUM:
			debugWrite("ORACLE_TYPE_VARNUM");
			break;
		case ORACLE_TYPE_LONG:
			debugWrite("ORACLE_TYPE_LONG");
			break;
		case ORACLE_TYPE_ROWID_DEPRECATED:
			debugWrite("ORACLE_TYPE_ROWID_DEPRECATED");
			break;
		case ORACLE_TYPE_DATE:
			debugWrite("ORACLE_TYPE_DATE");
			break;
		case ORACLE_TYPE_RAW:
			debugWrite("ORACLE_TYPE_RAW");
			break;
		case ORACLE_TYPE_LONG_RAW:
			debugWrite("ORACLE_TYPE_LONG_RAW");
			break;
		case ORACLE_TYPE_CHAR:
			debugWrite("ORACLE_TYPE_CHAR");
			break;
		case ORACLE_TYPE_RESULT_SET:
			debugWrite("ORACLE_TYPE_RESULT_SET");
			break;
		case ORACLE_TYPE_ROWID:
			debugWrite("ORACLE_TYPE_ROWID");
			break;
		case ORACLE_TYPE_NAMED_TYPE:
			debugWrite("ORACLE_TYPE_NAMED_TYPE");
			break;
		case ORACLE_TYPE_REF_TYPE:
			debugWrite("ORACLE_TYPE_REF_TYPE");
			break;
		case ORACLE_TYPE_CLOB:
			debugWrite("ORACLE_TYPE_CLOB");
			break;
		case ORACLE_TYPE_BLOB:
			debugWrite("ORACLE_TYPE_BLOB");
			break;
		case ORACLE_TYPE_BFILE:
			debugWrite("ORACLE_TYPE_BFILE");
			break;
		case ORACLE_TYPE_LOB_CLOB:
			debugWrite("ORACLE_TYPE_LOB_CLOB");
			break;
		case ORACLE_TYPE_LOB_BLOB:
			debugWrite("ORACLE_TYPE_LOB_BLOB");
			break;
		case ORACLE_TYPE_LOB_BFILE:
			debugWrite("ORACLE_TYPE_LOB_BFILE");
			break;
		case ORACLE_TYPE_TIMESTAMP:
			debugWrite("ORACLE_TYPE_TIMESTAMP");
			break;
		case ORACLE_TYPE_TIMESTAMPTZ:
			debugWrite("ORACLE_TYPE_TIMESTAMPTZ");
			break;
		case ORACLE_TYPE_INTERVALYM:
			debugWrite("ORACLE_TYPE_INTERVALYM");
			break;
		case ORACLE_TYPE_INTERVALDS:
			debugWrite("ORACLE_TYPE_INTERVALDS");
			break;
		case ORACLE_TYPE_TIMESTAMPLTZ:
			debugWrite("ORACLE_TYPE_TIMESTAMPLTZ");
			break;
		case ORACLE_TYPE_PLSQL_INDEX_TABLE:
			debugWrite("ORACLE_TYPE_PLSQL_INDEX_TABLE");
			break;
		case ORACLE_TYPE_FIXED_CHAR:
			debugWrite("ORACLE_TYPE_FIXED_CHAR");
			break;
		default:
			debugWrite("unknown ORACLE_TYPE");
			break;
	}
}

void sqlrprotocol_oracle::debugSystemError() {
	char	*err=error::getErrorString();
	debugWrite("%s",err);
	delete[] err;
}

bool sqlrprotocol_oracle::getTtiFunction(const byte_t *rp,
						byte_t *ttifunction,
						const byte_t **rpout) {

	// a read pointer that has reached the end of the packet it was in has
	// nothing left to hand back, so get another packet
	bool	newpacket=(!rp || !resppacket ||
					rp>=resppacket+resppacketsize);

	if (newpacket) {

		// a modern client cancels an abandoned call (eg. a fetch it
		// stopped reading) with a marker rather than a tti call.  it
		// is not a framing error and not end of session - answer
		// with a reset marker of our own, so the client's resync
		// loop sees it and the interrupted call is discarded, then
		// go back to waiting for the real next tti function
		for (;;) {

			if (!recvPacket()) {
				return false;
			}

			if (resppackettype==PACKET_DATA) {
				break;
			}

			if (resppackettype==PACKET_MARKER) {
				if (!sendMarker(MARKER_TYPE_RESET)) {
					return false;
				}

				// the client is waiting to read the interrupted
				// call's result, not another marker, before it
				// will send anything else
				if (!sendMarkerCancelError()) {
					return false;
				}
				continue;
			}

			// the data a lob write sends behind its request.  it
			// goes out whether the write is going to be answered
			// or not, so a refused one leaves it here, in front
			// of the next tti call.  read it and throw it away
			if (resppackettype==PACKET_DATA_DESCRIPTOR) {
				if (!discardLobDataPacket()) {
					return false;
				}
				continue;
			}

			debugWrite("bad packet type %d, expected %d",
						resppackettype,PACKET_DATA);
			return false;
		}

		rp=resppacket;
	}

	const byte_t	*rpin=rp;
	const byte_t	*end=resppacket+resppacketsize;

	uint16_t	dataflags=0;
	byte_t		ttccode;

	// the data flags belong to the packet, not to the message - a client
	// can pack more than one message into one packet, and only the first
	// of them follows the flags
	if (newpacket) {
		if (end-rp<2) {
			debugWrite("truncated data flags");
			*rpout=rpin;
			return false;
		}
		readBE(rp,&dataflags,&rp);

		// a client's last packet is the flags and nothing else
		if (rp==end) {
			debugStart("get tti function");
			debugWrite("data flags: 0x%04x",dataflags);
			debugWrite("%s",(dataflags&DATA_FLAGS_EOF)?
						"eof flag":"empty packet");
			debugEnd();
			*rpout=rpin;
			return false;
		}
	}

	if (end-rp<2) {
		debugWrite("truncated tti message");
		*rpout=rpin;
		return false;
	}

	// both ttc types are legitimate here - an ordinary tti function call
	// and a piggybacked one.  the shared read() that verifies an expected
	// value logs every mismatch, so probing for one type and falling back
	// to the other makes an ordinary piggybacked call look like an error
	read(rp,&ttccode,&rp);
	if (ttccode!=TTC_TTI_FUNCTION &&
		ttccode!=TTC_PIGGYBACK_TTI_FUNCTION) {
		debugWrite("bad ttccode 0x%02x, expected 0x%02x or 0x%02x",
					ttccode,TTC_TTI_FUNCTION,
					TTC_PIGGYBACK_TTI_FUNCTION);
		*rpout=rpin;
		return false;
	}
	read(rp,ttifunction,&rp);
	lastttccode=ttccode;
	*rpout=rp;

	if (getDebug()) {
		debugStart("get tti function");
		if (newpacket) {
			debugWrite("data flags: 0x%04x",dataflags);
		}
		debugTtcCode(ttccode);
		debugTtiFunction(*ttifunction);
		debugEnd();
	}

	return true;
}

bool sqlrprotocol_oracle::query(const byte_t *rp) {

	// legacy path (pre-10g): parse only - execution and row transfer
	// happen on a later execute() or fetch()

	// parse the request...
	// moreoptions and unknown3-7 are unexplained
	// see "Oracle Wire Protocol - Query"
	uint16_t	options;
	uint16_t	moreoptions;
	uint16_t	cursorid;
	byte_t		unknown3;
	byte_t		unknown4;
	byte_t		unknown5;
	uint16_t	querysize;
	byte_t		unknown6;
	byte_t		unknown7;
	const char	*query;

	readBE(rp,&options,&rp);
	readBE(rp,&moreoptions,&rp);
	readBE(rp,&cursorid,&rp);
	read(rp,&unknown3,&rp);
	read(rp,&unknown4,&rp);
	read(rp,&unknown5,&rp);
	readLE(rp,&querysize,&rp);
	read(rp,&unknown6,&rp);
	read(rp,&unknown7,&rp);
	query=(char *)rp;

	debugStart("query request");
	debugOptions(options,moreoptions);
	debugWrite("cursor id: %d",cursorid);
	debugWrite("unknown: %02x %02x %02x",unknown3,unknown4,unknown5);
	debugWrite("query size: %d",querysize);
	debugWrite("unknown: %02x %02x",unknown6,unknown7);
	debugWrite("query: \"%*s\"",querysize,query);
	debugEnd();

	// the id on the wire is the controller's plus 1
	sqlrservercursor	*cursor=(cursorid)?
			cont->getCursor((uint16_t)(cursorid-1)):NULL;
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError(cursorid);
	}
	lastcursorid=cont->getId(cursor);

	// reset column type cache flag
	columntypescached[cont->getId(cursor)]=false;

	// and any row it was pinning for a lob read
	clearLobPin(cont->getId(cursor));

	// bounds checking
	if (querysize>maxquerysize) {
		// FIXME: implement this
		//return sendErrPacket(1105,"Unknown error","24000");
		return false;
	}

	// copy the query into the cursor's query buffer
	char	*querybuffer=cont->getQueryBuffer(cursor);
	bytestring::copy(querybuffer,query,querysize);
	querybuffer[querysize]='\0';
	cont->setQuerySize(cursor,querysize);

	// prepare the query
	if (!cont->prepareQuery(cursor,cont->getQueryBuffer(cursor),
					cont->getQuerySize(cursor),
					true,true,true,true)) {
		debugWrite("prepare query failed");
		return sendQueryError(cursor);
	}
	return sendQueryResponse(cursor);
}

bool sqlrprotocol_oracle::sendQueryResponse(sqlrservercursor *cursor) {

	resetSendPacketBuffer(PACKET_DATA);

	// FIXME: decode this... see "Oracle Wire Protocol - Query"

	uint16_t	dataflags=0;
	byte_t	ttccode=TTC_ERROR;
	byte_t unknown1[]={
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00
	};
	byte_t unknown2[]={
		// not cursor id
		0x01, 0x00
	};
	byte_t unknown3[]={
		0x11, 0x00, 0x03, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	reqpacket.append(unknown1,sizeof(unknown1));
	reqpacket.append(unknown2,sizeof(unknown2));
	reqpacket.append(unknown3,sizeof(unknown3));

	putGenericFooter();

	if (getDebug()) {
		debugStart("query response");
		debugWrite("data flags: 0x%04x",dataflags);
		debugTtcCode(ttccode);
		debugEnd();
	}

	return sendPacket(true);
}

bool sqlrprotocol_oracle::query2(const byte_t *rp) {

	// legacy path (pre-10g): combined parse/bind/execute
	// can apparently be used for fetch too

	// parse the request...
	// see "Oracle Wire Protocol - Query2"
	uint16_t	options;
	uint16_t	moreoptions;
	uint16_t	cursorid;
	uint32_t	querysize=0;
	const char	*query=NULL;

	readBE(rp,&options,&rp);
	readBE(rp,&moreoptions,&rp);
	readBE(rp,&cursorid,&rp);
	if (options&OPTION_PARSE) {
		// no idea...
		for (uint16_t i=0; i<7; i++) {
			byte_t	unknown;
			read(rp,&unknown,&rp);
		}
		readLE(rp,&querysize,&rp);
		// no idea...
		for (uint16_t i=0; i<44; i++) {
			byte_t	unknown;
			read(rp,&unknown,&rp);
		}
		query=(char *)rp;
		rp+=querysize;
	}
	// no idea...

	if (getDebug()) {
		debugStart("query2 request");
		debugOptions(options,moreoptions);
		debugWrite("cursor id: %d",cursorid);
		if (options&OPTION_PARSE) {
			debugWrite("query size: %d",querysize);
			debugWrite("query: \"%*s\"",querysize,query);
		}
		debugEnd();
	}

	// the id on the wire is the controller's plus 1
	sqlrservercursor	*cursor=(cursorid)?
			cont->getCursor((uint16_t)(cursorid-1)):NULL;
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError(cursorid);
	}
	lastcursorid=cont->getId(cursor);

	if (options&OPTION_PARSE) {

		// reset column type cache flag
		columntypescached[cont->getId(cursor)]=false;

		// bounds checking
		if (querysize>maxquerysize) {
			// FIXME: implement this
			//return sendErrPacket(1105,"Unknown error","24000");
			return false;
		}

		// copy the query into the cursor's query buffer
		char	*querybuffer=cont->getQueryBuffer(cursor);
		bytestring::copy(querybuffer,query,querysize);
		querybuffer[querysize]='\0';
		cont->setQuerySize(cursor,querysize);
	
		// prepare the query
		if (!cont->prepareQuery(cursor,
					cont->getQueryBuffer(cursor),
					cont->getQuerySize(cursor),
					true,true,true,true)) {
			debugWrite("prepare query failed");
			return sendQueryError(cursor);
		}
	}

	if (options&OPTION_BIND) {

		if (!sendQuery2Response(cursor,true)) {
			return false;
		}

		// free binds from any previous bind exchange on this cursor
		clearParams(cursor);

		uint16_t	cursorid_idx=cont->getId(cursor);

		// the query buffer reflects whatever was prepared most
		// recently, whether by the parse phase above (this call or a
		// prior one) or by the older query() call on this cursor, so
		// recompute the count here rather than caching it - caching
		// it against the cursor could go stale if a different query
		// gets prepared into the same cursor between calls
		uint16_t	pcount=cont->countBindVariables(
						cont->getQueryBuffer(cursor),
						cont->getQuerySize(cursor));

		// clamp to the number of binds we can actually track
		uint16_t	clampedpcount=
				(pcount<=maxbindcount)?pcount:maxbindcount;
		if (pcount>maxbindcount) {
			debugWrite("query has %d binds, "
					"truncating to maxbindcount %d",
					pcount,maxbindcount);
		}

		// the Query2 Bind Value Request carries each value as
		// length-prefixed text with no per-value type tag on the
		// wire (see the Trac wiki: "Oracle Wire Protocol - Query2"),
		// so there's no real type to source here - assume varchar
		for (uint16_t i=0; i<clampedpcount; i++) {
			ptypes[cursorid_idx][i]=ORACLE_TYPE_VARCHAR;
		}

		// pass the unclamped pcount - bindParameters() clamps
		// internally, but needs the real count to know how many
		// bind-value packets the client will actually send
		if (!bindParameters(cursor,pcount,ptypes[cursorid_idx])) {
			return false;
		}
	}

	if (options&OPTION_EXECUTE) {

		// a fresh execute means a new result set - drop any row
		// held over from a previous one on this cursor
		pendingrow[cont->getId(cursor)].clear();

		// and any row it was pinning for a lob read
		clearLobPin(cont->getId(cursor));

		// execute the query
		if (!cont->executeQuery(cursor,true,true,true,true)) {
			debugWrite("execute query failed");
			return sendQueryError(cursor);
		}
	}

	if (options&OPTION_COMMIT) {
		// the execute above already returned on failure, so this
		// only runs on success - which is what OCI_COMMIT_ON_SUCCESS
		// asks for
		if (!cont->commit()) {
			return sendTransactionError(cont->getId(cursor)+1);
		}
	}

	return sendQuery2Response(cursor,false);
}

bool sqlrprotocol_oracle::sendQuery2Response(sqlrservercursor *cursor,
								bool binds) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags;
	byte_t		ttccode;

	if (binds) {

		dataflags=0;
		ttccode=TTC_IO_VECTOR;

		// FIXME: decode this... see "Oracle Wire Protocol - Query2"

		byte_t unknown[]={
			0x05, 0xFE, 0x01, 0x00, 0x00,
			0x00, 0x01, 0x00, 0x00, 0x00, 0x20
		};

		writeBE(&reqpacket,dataflags);
		write(&reqpacket,ttccode);
		reqpacket.append(unknown,sizeof(unknown));

	} else {

		dataflags=0;
		ttccode=TTC_OK;

		// FIXME: decode this... see "Oracle Wire Protocol - Query2"

		byte_t unknown[]={

			// varies...
			0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
			0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x01,
			0x00, 0x00, 0x00

			// it's this from 8i on redhat 6.2,
			// though the 8i client accepts the above...
			/*
 			0x02, 0x00, 0x79, 0xE6, 0x19 0x00, 0x00, 0x00,
			0x00, 0x00, 0x04, 0x01, 0x00 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00 0x00, 0x01, 0x00,
			0x00, 0x00, 0x03, 0x00, 0x40 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00 0x09, 0x00, 0x00,
			0x01, 0x00, 0x00
			*/
		};

		writeBE(&reqpacket,dataflags);
		write(&reqpacket,ttccode);
		reqpacket.append(unknown,sizeof(unknown));
	}

	putGenericFooter();

	debugStart("query2 response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugEnd();

	return sendPacket(true);
}

bool sqlrprotocol_oracle::bindParameters(sqlrservercursor *cursor,
							uint16_t pcount,
							uint16_t *ptypes) {

	cont->setInputBindCount(cursor,
				(pcount<=maxbindcount)?pcount:maxbindcount);

	// the pool owns every bind value, for the life of the statement -
	// clear it once, in front of the whole set
	memorypool	*bindpool=cont->getBindPool(cursor);
	bindpool->clear();
	sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);

	for (uint16_t i=0; i<pcount; i++) {

		if (!recvPacket()) {
			return false;
		}

		if (resppackettype!=PACKET_DATA) {
			debugWrite("bad packet type %d, expected %d",
						resppackettype,PACKET_DATA);
			return false;
		}

		if (i>=maxbindcount) {
			continue;
		}

		const byte_t	*rp=resppacket;
		const byte_t	*end=resppacket+resppacketsize;

		// bounds check before reading the fixed-size header below
		if ((size_t)(end-rp)<sizeof(uint16_t)+sizeof(byte_t)+
							sizeof(byte_t)) {
			debugWrite("bind value packet too short");
			return false;
		}

		uint16_t	dataflags;
		// unexplained.  see "Oracle Wire Protocol - Query2"
		byte_t		unknown;

		readBE(rp,&dataflags,&rp);
		read(rp,&unknown,&rp);


		sqlrserverbindvar	*bv=&(inbinds[i]);

		// the bind variable name should be something like :1, :2, etc.
		bv->variable=bindvarnames[i];
		bv->variablesize=charstring::getLength(bv->variable);

		debugStart("bind %d",i);
		debugWrite("data flags: 0x%04x",dataflags);
		debugWrite("variable: %s",bv->variable);

		// FIXME: handle nulls - the null wire form is unexplained
		if (false) {
			bv->type=SQLRSERVERBINDVARTYPE_NULL;
			bv->isnull=cont->getNullBindValue();
			debugWrite("type: NULL");
			debugWrite("isnull: true");
			debugEnd();
			continue;
		}

		// every Bind Value Request value arrives on the wire as
		// length-prefixed text - a UB1 size byte followed by that
		// many raw bytes - with no per-value type tag, so the cases
		// below only differ in which SQLRSERVERBINDVARTYPE the value
		// gets bound as, not in how the bytes are read
		switch (ptypes[i]) {
			case ORACLE_TYPE_VARCHAR:
			case ORACLE_TYPE_CHAR:
			case ORACLE_TYPE_NUMBER:
			case ORACLE_TYPE_VARNUM:
			case ORACLE_TYPE_LONG:
			case ORACLE_TYPE_DATE:
			case ORACLE_TYPE_ROWID:
			case ORACLE_TYPE_ROWID_DEPRECATED:
			case ORACLE_TYPE_TIMESTAMP:
			case ORACLE_TYPE_TIMESTAMPTZ:
			case ORACLE_TYPE_TIMESTAMPLTZ:
			case ORACLE_TYPE_INTERVALYM:
			case ORACLE_TYPE_INTERVALDS:
			// an unrecognized/non-bindable type code is taken as text
			default:
				{
				byte_t	size;
				read(rp,&size,&rp);

				// bounds check before copying "size" bytes
				if (rp>end || (size_t)(end-rp)<(size_t)size) {
					debugWrite("bind value truncated, "
						"needed %d bytes",
						(uint32_t)size);
					return false;
				}

				bv->type=SQLRSERVERBINDVARTYPE_STRING;
				bv->valuesize=size;
				bv->value.stringval=(char *)
					bindpool->allocate(bv->valuesize+1);
				bytestring::copy(bv->value.stringval,
							rp,(size_t)size);
				bv->value.stringval[bv->valuesize]='\0';
				bv->isnull=cont->getNonNullBindValue();
				rp+=bv->valuesize;
				break;
				}
			case ORACLE_TYPE_RAW:
			case ORACLE_TYPE_LONG_RAW:
			case ORACLE_TYPE_BLOB:
			case ORACLE_TYPE_BFILE:
				{
				byte_t	size;
				read(rp,&size,&rp);

				// bounds check before copying "size" bytes
				if (rp>end || (size_t)(end-rp)<(size_t)size) {
					debugWrite("bind value truncated, "
						"needed %d bytes",
						(uint32_t)size);
					return false;
				}

				bv->type=SQLRSERVERBINDVARTYPE_BLOB;
				bv->valuesize=size;
				bv->value.stringval=(char *)
					bindpool->allocate(bv->valuesize+1);
				bytestring::copy(bv->value.stringval,
							rp,(size_t)size);
				bv->value.stringval[bv->valuesize]='\0';
				bv->isnull=cont->getNonNullBindValue();
				rp+=bv->valuesize;
				break;
				}
			case ORACLE_TYPE_CLOB:
				{
				byte_t	size;
				read(rp,&size,&rp);

				// bounds check before copying "size" bytes
				if (rp>end || (size_t)(end-rp)<(size_t)size) {
					debugWrite("bind value truncated, "
						"needed %d bytes",
						(uint32_t)size);
					return false;
				}

				bv->type=SQLRSERVERBINDVARTYPE_CLOB;
				bv->valuesize=size;
				bv->value.stringval=(char *)
					bindpool->allocate(bv->valuesize+1);
				bytestring::copy(bv->value.stringval,
							rp,(size_t)size);
				bv->value.stringval[bv->valuesize]='\0';
				bv->isnull=cont->getNonNullBindValue();
				rp+=bv->valuesize;
				break;
				}
		}

		if (getDebug()) {
			if (bv->type==SQLRSERVERBINDVARTYPE_STRING) {
				debugWrite("type: STRING");
				debugWrite("value: %s",
						bv->value.stringval);
			} else if (bv->type==SQLRSERVERBINDVARTYPE_INTEGER) {
				debugWrite("type: INTEGER");
				debugWrite("value: %lld",
						(long long)bv->value.integerval);
			} else if (bv->type==SQLRSERVERBINDVARTYPE_DOUBLE) {
				debugWrite("type: DOUBLE");
				debugWrite("value: %f (%d,%d)",
						bv->value.doubleval.value,
						bv->value.doubleval.precision,
						bv->value.doubleval.scale);
			} else if (bv->type==SQLRSERVERBINDVARTYPE_DATE) {
				// FIXME: print date...
			} else if (bv->type==SQLRSERVERBINDVARTYPE_BLOB) {
				debugWrite("type: BLOB");
				stringbuffer	b;
				b.safePrint(bv->value.stringval,
							bv->valuesize);
				debugWrite("value: %s",b.getString());
			} else if (bv->type==SQLRSERVERBINDVARTYPE_CLOB) {
				debugWrite("type: CLOB");
				debugWrite("value: %s",
						bv->value.stringval);
			}
			debugWrite("value size: %d",bv->valuesize);
			debugWrite("isnull: false");
			debugEnd();
		}
	}

	return true;
}

bool sqlrprotocol_oracle::query3(const byte_t *rp) {

	// modern path (10g and later): combined open/prepare/describe/execute
	// the first one puts the session on the modern path for good
	// can apparently be used for fetch too

	// parse the request...
	uint32_t	options=0;
	uint32_t	cursorid=0;
	uint32_t	prefetchrows=0;
	uint32_t	maxlongsize=0;
	uint32_t	querysize=0;
	const char	*query=NULL;

	query3affectedrows=0;
	query3knowsaffectedrows=false;

	if (!getQuery3Request(rp,resppacket+resppacketsize,
					&options,&cursorid,&prefetchrows,
					&maxlongsize,&query,&querysize)) {
		return false;
	}

	// which layout a fetch that follows this is in
	query3session=true;

	// get the requested cursor
	// (cursor id 0 means "open one for me", and the ids on the wire are
	// the controller's plus 1, since the controller's start at 0)
	sqlrservercursor	*cursor;
	if (!cursorid) {
		cursor=cont->getCursor();
		if (!cursor) {
			debugWrite("couldn't get cursor");
			return sendCursorNotOpenError();
		}
		lastcursorid=cont->getId(cursor);
		cursorid=lastcursorid+1;
		debugStart("open request");
		debugWrite("cursor id: %d",cursorid);
		debugEnd();
	} else {
		cursor=cont->getCursor((uint16_t)(cursorid-1));
		if (!cursor) {
			debugWrite("cursor id %d not found",cursorid);
			return sendCursorNotOpenError(cursorid);
		}
		lastcursorid=cursorid-1;
	}

	// a re-execute of this statement will send values without
	// descriptors, so keep the ones that came with it
	if (query3binddescs || (options&OPTION_PARSE)) {
		saveQuery3Binds(cursor);
	}

	if (options&OPTION_PARSE) {

		// reset column type cache flag
		columntypescached[cont->getId(cursor)]=false;

		// re-start the running row count
		rowssent[cont->getId(cursor)]=0;

		// a re-parse means a new result set - drop any row held
		// over from the previous one
		pendingrow[cont->getId(cursor)].clear();

		// and any row it was pinning for a lob read
		clearLobPin(cont->getId(cursor));

		// bounds checking
		if (querysize>maxquerysize) {
			// FIXME: implement this
			//return sendErrPacket(1105,"Unknown error","24000");
			return false;
		}

		// copy the query into the cursor's query buffer
		char	*querybuffer=cont->getQueryBuffer(cursor);
		bytestring::copy(querybuffer,query,querysize);
		querybuffer[querysize]='\0';
		cont->setQuerySize(cursor,querysize);

		// prepare the query
		if (!cont->prepareQuery(cursor,
					cont->getQueryBuffer(cursor),
					cont->getQuerySize(cursor),
					true,true,true,true)) {
			debugWrite("prepare query failed");
			return sendQueryError(cursor);
		}
	}

	// a describe executes too - the column info it answers with only
	// comes from an executed statement.  but a describe alone (no
	// explicit execute) should never run anything other than a select -
	// running dml just to answer a describe would perform work the
	// client never asked for
	bool	describecanexecute=(options&OPTION_DESCRIBE) &&
					cursor->getQueryType()==SQLRQUERYTYPE_SELECT;
	if ((options&OPTION_EXECUTE) || describecanexecute) {

		// a placeholder the client never bound anything to
		if (query3unbound) {
			debugWrite("not all variables bound");
			return sendNotAllVariablesBoundError(cursorid);
		}

		// a fresh execute means a new result set - drop any row
		// held over from a previous one on this cursor
		pendingrow[cont->getId(cursor)].clear();

		// and any row it was pinning for a lob read
		clearLobPin(cont->getId(cursor));

		// an array bind sends one row data block per element, and a
		// re-parse without one means the previous statement's binds
		// have to go
		uint32_t	blocks=query3blocks;
		if (!blocks && (options&OPTION_PARSE)) {
			cont->setInputBindCount(cursor,0);
			cont->setOutputBindCount(cursor,0);
		}

		// execute the query, once per row data block - the wire's
		// array bind is many complete bind rows rather than one bind
		// with many elements, so each block is its own execution
		for (uint32_t block=0; block<blocks || !block; block++) {

			if (block<blocks &&
				!installQuery3Binds(cursor,block)) {
				return sendNotAllVariablesBoundError(cursorid);
			}

			if (!cont->executeQuery(cursor,true,true,true,true)) {
				debugWrite("execute query failed");
				return sendQueryError(cursor);
			}

			// a ref cursor bind's result set isn't readable
			// until the execute that opened it has run
			sqlrservercursor	*failed=NULL;
			if (!fetchFromRefCursors(cursor,&failed)) {
				return sendQueryError((failed)?
							failed:cursor);
			}

			if (cont->knowsAffectedRows(cursor)) {
				query3knowsaffectedrows=true;
				query3affectedrows+=(uint32_t)
					cont->getAffectedRows(cursor);
			}
		}
	}

	if (options&OPTION_COMMIT) {
		// the execute above already returned on failure, so this
		// only runs on success - which is what OCI_COMMIT_ON_SUCCESS
		// asks for
		if (!cont->commit()) {
			return sendTransactionError(cont->getId(cursor)+1);
		}
	}

	return sendQuery3Response(cursor,options,cursorid,
					prefetchrows,maxlongsize);
}

bool sqlrprotocol_oracle::getQuery3Request(const byte_t *rp,
						const byte_t *end,
						uint32_t *options,
						uint32_t *cursorid,
						uint32_t *prefetchrows,
						uint32_t *maxlongsize,
						const char **query,
						uint32_t *querysize) {

	// the layout is python-oracledb's _write_execute_message(), in
	// src/oracledb/impl/thin/messages/execute.pyx - every field is either
	// a ub4 or a single raw byte, so nothing sits at a fixed offset
	*options=0;
	*cursorid=0;
	*prefetchrows=0;
	*maxlongsize=0;
	*query=NULL;
	*querysize=0;

	byte_t		sequence=0;
	// pointer and unused stand in for unexplained flags and counts
	// see "Oracle Wire Protocol - Query3"
	byte_t		pointer=0;
	uint32_t	vectorsize=0;
	uint32_t	prefetchbuffersize=0;
	uint32_t	bindcount=0;
	uint32_t	definecount=0;
	uint32_t	unused=0;

	if (!getPointer(rp,end,&sequence,&rp) ||
		!readLenPreInt(rp,end,options,&rp) ||
		!readLenPreInt(rp,end,cursorid,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,querysize,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,&vectorsize,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,&prefetchbuffersize,&rp) ||
		!readLenPreInt(rp,end,prefetchrows,&rp) ||
		!readLenPreInt(rp,end,maxlongsize,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,&bindcount,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,&definecount,&rp) ||
		!readLenPreInt(rp,end,&unused,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,&unused,&rp) ||
		!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,&unused,&rp) ||
		!readLenPreInt(rp,end,&unused,&rp)) {
		debugWrite("truncated query3 request");
		return false;
	}

	if (*querysize) {

		// the tail between the registration id and the query text is
		// not fixed - it grows with the negotiated field version, and
		// all of it is zero for a query with no binds, so skip it as a
		// run of zeros rather than count it (a query's text never
		// starts with a zero byte)
		while (rp<end && !(*rp)) {
			rp++;
		}

		if (rp<end && *rp==CLR_LONG_FORM_MARKER) {

			// chunked (long form) query text: the marker, then a
			// run of chunks - each a single raw length byte
			// (0-255) followed by that many bytes - concatenated
			// together and ended by a zero-length chunk.  A
			// client's chunk length is one plain byte, confirmed
			// against a capture: 0xfe, 0xff (255), 255 bytes of
			// text, 0x1f (31), 31 more bytes, 0x00 to end - a
			// 286-byte "create table" statement split at the
			// 255-byte mark.  the chunks aren't contiguous (each
			// is separated from the next by its length byte), so
			// reassemble them into one buffer; (end-rp) is a safe
			// upper bound on the reassembled size, since the
			// length bytes only take space away from it
			rp++;
			char	*querytext=(char *)resppacketpool->allocate(
							(size_t)(end-rp));
			uint32_t	querytextsize=0;
			for (;;) {
				if (rp>=end) {
					debugWrite("truncated chunked "
							"query text");
					return false;
				}
				byte_t	chunksize=0;
				read(rp,&chunksize,&rp);
				if (!chunksize) {
					break;
				}
				if ((size_t)(end-rp)<(size_t)chunksize) {
					debugWrite("truncated chunked "
							"query text");
					return false;
				}
				bytestring::copy(querytext+querytextsize,
							rp,chunksize);
				rp+=chunksize;
				querytextsize+=chunksize;
			}
			*query=querytext;
			*querysize=querytextsize;

		} else {

			// a length byte in front of the text.  python-oracledb and
			// node-oracledb declare the byte count and write a length
			// byte that matches it, OCI declares a buffer size - the
			// character count times the bytes per character of its
			// charset, the same thing it does with the user name in the
			// login - and writes a length byte with the real byte count,
			// and ojdbc declares the byte count and writes no length byte
			// at all.  so the declared size is only wrong for OCI, and
			// only OCI's length byte can be believed over it - believing
			// ojdbc's first character as a length would eat it.  telling
			// OCI's case from ojdbc's by whether the declared size
			// overflows the packet works for a long statement, but not
			// for one short enough that both fit - "commit" on a 4 byte
			// per character charset declares 24 with 24 bytes still in
			// the packet, and the 18 bytes past the text went to the
			// backend as part of the query, which is the ORA-00911 this
			// fixes - so fall back on the module's own oci discriminator
			if (rp<end && *rp && (uint32_t)(*rp)<*querysize &&
				(ociclient ||
					(size_t)(end-rp)<(size_t)*querysize)) {
				*querysize=*rp;
				rp++;
			} else if (rp<end && *querysize<=CLR_MAX_SHORT_LENGTH &&
						(uint32_t)(*rp)==*querysize) {
				rp++;
			}

			if ((size_t)(end-rp)<(size_t)*querysize) {
				debugWrite("truncated query text");
				return false;
			}

			*query=(const char *)rp;
			rp+=*querysize;
		}
	}

	// the summary object has to echo this back
	callnumber=sequence;

	if (getDebug()) {
		debugStart("query3 request");
		debugWrite("sequence: %d",sequence);
		debugWrite("options: 0x%08x",*options);
		debugOptions(*options);
		debugWrite("cursor id: %d",*cursorid);
		debugWrite("prefetch rows: %d",*prefetchrows);
		debugWrite("bind count: %d",bindcount);
		debugWrite("define count: %d",definecount);
		debugWrite("query size: %d",*querysize);
		if (*query) {
			debugWrite("query: \"%.*s\"",(int)*querysize,*query);
		}
		debugEnd();
	}

	// the al8i4 vector, the bind and define descriptors, and the bind
	// values all follow the query text
	return getQuery3Binds(rp,end,vectorsize,bindcount,definecount,
						*options,*query,*querysize);
}

// reads the tail of a query3 request: the al8i4 vector, then one descriptor
// per bind and one per define, then one row data block per execution
// iteration.  binds are positional, so a descriptor and its values are only
// tied to a placeholder by their order.
// see "Oracle Wire Protocol - Query3"
bool sqlrprotocol_oracle::getQuery3Binds(const byte_t *rp,
						const byte_t *end,
						uint32_t vectorsize,
						uint32_t bindcount,
						uint32_t definecount,
						uint32_t options,
						const char *query,
						uint32_t querysize) {

	query3binddescs=0;
	query3blocks=0;
	query3unbound=false;

	// nothing else in the request needs reading
	if (!bindcount && !definecount) {
		return true;
	}

	debugStart("query3 binds");

	// the al8i4 vector, whose second element is the iteration count
	uint32_t	iterations=0;
	for (uint32_t i=0; i<vectorsize; i++) {
		uint32_t	value=0;
		if (!readLenPreInt(rp,end,&value,&rp)) {
			debugWrite("truncated al8i4 vector");
			debugEnd();
			return false;
		}
		if (i==1) {
			iterations=value;
		}
	}
	debugWrite("iterations: %d",iterations);

	// the bind descriptors
	if (bindcount>query3bindavail) {
		delete[] query3binds;
		query3binds=new oraclequery3bind[bindcount];
		query3bindavail=bindcount;
	}
	for (uint32_t i=0; i<bindcount; i++) {
		if (!getQuery3BindDescriptor(rp,end,
						&(query3binds[i].type),
						&(query3binds[i].flags),
						&(query3binds[i].buffersize),
						&rp)) {
			debugEnd();
			return false;
		}
		query3binds[i].direction=BIND_DIRECTION_IN;
		query3binds[i].outindex=-1;
		if (getDebug()) {
			debugWrite("bind %d:",i+1);
			debugColumnType(query3binds[i].type);
			debugWrite("flags: 0x%02x",query3binds[i].flags);
		}
		if (query3binds[i].flags&BIND_FLAG_UNBOUND) {
			query3unbound=true;
		}
	}
	query3binddescs=bindcount;

	// the define descriptors, which need consuming but nothing else
	for (uint32_t i=0; i<definecount; i++) {
		byte_t		type=0;
		byte_t		flags=0;
		uint32_t	buffersize=0;
		if (!getQuery3BindDescriptor(rp,end,&type,&flags,
							&buffersize,&rp)) {
			debugEnd();
			return false;
		}
	}

	// which way each of them travels
	classifyQuery3Binds(options,query,querysize);

	// an unbound placeholder means no row data at all
	if (!bindcount || query3unbound) {
		debugEnd();
		return true;
	}

	if (!getQuery3BindValues(rp,end,bindcount,iterations)) {
		debugEnd();
		return false;
	}

	debugEnd();

	return true;
}

// one row data block per execution iteration, each carrying a value for
// every bind, in descriptor order.  there is no per-value type tag - the
// type comes from the matching descriptor
// see "Oracle Wire Protocol - Query3"
bool sqlrprotocol_oracle::getQuery3BindValues(const byte_t *rp,
						const byte_t *end,
						uint32_t bindcount,
						uint32_t iterations) {

	query3blocks=0;

	uint64_t	valuecount=(uint64_t)iterations*(uint64_t)bindcount;
	if (valuecount>MAX_QUERY3_BIND_VALUES) {
		debugWrite("too many bind values: %lld",(long long)valuecount);
		return false;
	}
	if ((uint32_t)valuecount>query3bindvalueavail) {
		delete[] query3bindvalues;
		query3bindvalues=new
				oraclequery3bindvalue[(uint32_t)valuecount];
		query3bindvalueavail=(uint32_t)valuecount;
	}

	while (rp<end && *rp==TTC_ROW_DATA && query3blocks<iterations) {
		rp++;
		for (uint32_t i=0; i<bindcount; i++) {
			oraclequery3bindvalue	*v=&(query3bindvalues[
						query3blocks*bindcount+i]);
			if (!getLenBytes(rp,end,&(v->value),&(v->size),
							&(v->isnull),&rp)) {
				debugWrite("truncated bind value");
				return false;
			}
		}
		query3blocks++;
	}
	debugWrite("row data blocks: %d",query3blocks);

	return true;
}

// a bind or define descriptor - twelve fields, no name and no position, and
// a thirteenth once the negotiated field version reaches 12.2
// see "Oracle Wire Protocol - Query3"
bool sqlrprotocol_oracle::getQuery3BindDescriptor(const byte_t *rp,
						const byte_t *end,
						byte_t *type,
						byte_t *flags,
						uint32_t *buffersize,
						const byte_t **rpout) {

	*type=0;
	*flags=0;
	*buffersize=0;
	*rpout=rp;

	byte_t		precision=0;
	byte_t		scale=0;
	byte_t		csfrm=0;
	uint32_t	maxelements=0;
	uint32_t	contflags=0;
	uint32_t	oidlength=0;
	uint32_t	version=0;
	uint32_t	charsetid=0;
	uint32_t	maxdatasize=0;
	uint32_t	oaccolid=0;

	if ((size_t)(end-rp)<4) {
		debugWrite("truncated bind descriptor");
		return false;
	}
	read(rp,type,&rp);
	read(rp,flags,&rp);
	read(rp,&precision,&rp);
	read(rp,&scale,&rp);

	if (!readLenPreInt(rp,end,buffersize,&rp) ||
		!readLenPreInt(rp,end,&maxelements,&rp) ||
		!readLenPreInt(rp,end,&contflags,&rp) ||
		!readLenPreInt(rp,end,&oidlength,&rp)) {
		debugWrite("truncated bind descriptor");
		return false;
	}

	// no capture has a non-zero oid length, but a declared length that
	// isn't followed by its bytes would be the odd one out
	if (oidlength) {
		if ((size_t)(end-rp)<(size_t)oidlength) {
			debugWrite("truncated bind descriptor oid");
			return false;
		}
		rp+=oidlength;
	}

	if (!readLenPreInt(rp,end,&version,&rp) ||
		!readLenPreInt(rp,end,&charsetid,&rp)) {
		debugWrite("truncated bind descriptor");
		return false;
	}

	if ((size_t)(end-rp)<1) {
		debugWrite("truncated bind descriptor");
		return false;
	}
	read(rp,&csfrm,&rp);

	if (!readLenPreInt(rp,end,&maxdatasize,&rp)) {
		debugWrite("truncated bind descriptor");
		return false;
	}

	// 12.2 and later append an oaccolid
	if (fieldversion>=CCAP_FIELD_VERSION_12_2 &&
			!readLenPreInt(rp,end,&oaccolid,&rp)) {
		debugWrite("truncated bind descriptor");
		return false;
	}

	*rpout=rp;

	return true;
}

// which way each bind's value travels.  the wire can't say - an out-only, an
// in-out and an in-only descriptor are byte-identical, right down to the
// garbage value every one of them carries - so the direction has to come off
// the statement.  a placeholder in a pl/sql block is declared in-out, which
// is the safe reading of an unknowable: a value goes in either way, so a
// block that only reads it behaves as it always did, and one that writes it
// has somewhere to write to.  keeping the in bit also keeps a re-execute's
// request shape unchanged - oci sends a value for every bind that has it
// see "Oracle Wire Protocol - Query3"
void sqlrprotocol_oracle::classifyQuery3Binds(uint32_t options,
						const char *query,
						uint32_t querysize) {

	// a client that didn't ask for an io vector isn't expecting out binds,
	// and a request that didn't carry the text says nothing about what
	// the statement does with its placeholders
	if (!(options&OPTION_SNDIOV) || (options&OPTION_NOPLSQL) ||
						!query || !querysize) {
		return;
	}

	// only oracle's own wire types round trip through the encoders below
	if (charstring::compare(cont->getNativeDbType(),"oracle")) {
		return;
	}

	// a block, rather than the dml-with-returning-into that also asks
	// for an io vector - that one needs a row count in front of its
	// values, which nothing here writes
	const char	*ptr=query;
	const char	*endptr=query+querysize;
	while (ptr<endptr && character::isWhitespace(*ptr)) {
		ptr++;
	}
	size_t	left=(size_t)(endptr-ptr);
	if (!(left>=5 && !charstring::compareIgnoringCase(ptr,"begin",5)) &&
		!(left>=7 && !charstring::compareIgnoringCase(ptr,
							"declare",7))) {
		return;
	}

	for (uint32_t i=0; i<query3binddescs; i++) {

		oraclequery3bind	*bd=&(query3binds[i]);

		// a ref cursor carries no indicator flag - the client binds
		// a statement handle rather than a buffer - so it's read off
		// the type alone.  it goes out in-out, the way a live server
		// sends one
		if (bd->type==ORACLE_TYPE_RESULT_SET) {
			bd->direction=BIND_DIRECTION_INOUT;
			continue;
		}

		if (!(bd->flags&BIND_FLAG_USE_INDICATORS)) {
			continue;
		}

		switch (bd->type) {
			case ORACLE_TYPE_NUMBER:
			case ORACLE_TYPE_VARNUM:
			case ORACLE_TYPE_VARCHAR:
			case ORACLE_TYPE_CHAR:
				bd->direction=BIND_DIRECTION_INOUT;
				break;
			default:
				break;
		}
	}

	if (getDebug()) {
		debugStart("bind directions");
		for (uint32_t i=0; i<query3binddescs; i++) {
			debugWrite("bind %d: 0x%02x",
					i+1,query3binds[i].direction);
		}
		debugEnd();
	}
}

bool sqlrprotocol_oracle::hasQuery3OutBinds() {
	for (uint32_t i=0; i<query3binddescs; i++) {
		if (query3binds[i].direction&BIND_DIRECTION_OUT) {
			return true;
		}
	}
	return false;
}

// the name of the "index"th bind variable in the query text, without its
// leading marker.  the wire carries no names, so a positional descriptor's
// name has to come back out of the query.  the walk matches the one
// countBindVariables() does in src/common/bindvariables.h
bool sqlrprotocol_oracle::getBindVariableName(const char *query,
						uint32_t querysize,
						uint16_t index,
						const char **name,
						uint16_t *namesize) {

	*name=NULL;
	*namesize=0;

	if (!query || !querysize) {
		return false;
	}

	queryparsestate_t	parsestate=IN_QUERY;
	const char		*ptr=query;
	const char		*endptr=query+querysize;
	const char		*bindstart=NULL;
	char			prev='\0';
	uint16_t		count=0;

	while (ptr<endptr) {

		if (parsestate==IN_QUERY) {
			if (*ptr=='\'') {
				parsestate=IN_QUOTES;
			}
			if (beforeBindVariable(ptr)) {
				parsestate=BEFORE_BIND;
			}
			prev=(*ptr=='\\' && prev=='\\')?'\0':*ptr;
			ptr++;
			continue;
		}

		if (parsestate==IN_QUOTES) {
			if (*ptr=='\'' && prev!='\\') {
				parsestate=IN_QUERY;
			}
			prev=(*ptr=='\\' && prev=='\\')?'\0':*ptr;
			ptr++;
			continue;
		}

		if (parsestate==BEFORE_BIND) {
			if (isBindDelimiter(ptr,false,true,false,false)) {
				bindstart=ptr+1;
				count++;
				parsestate=IN_BIND;
			} else {
				parsestate=IN_QUERY;
			}
			continue;
		}

		// in a bind variable
		if (afterBindVariable(ptr)) {
			if (count==index+1) {
				*name=bindstart;
				*namesize=(uint16_t)(ptr-bindstart);
				return true;
			}
			parsestate=IN_QUERY;
			continue;
		}
		prev=(*ptr=='\\' && prev=='\\')?'\0':*ptr;
		ptr++;
	}

	// a bind variable can run to the end of the query
	if (parsestate==IN_BIND && count==index+1) {
		*name=bindstart;
		*namesize=(uint16_t)(ptr-bindstart);
		return true;
	}

	return false;
}

// fills the cursor's input binds from one row data block of the query3
// request being handled
bool sqlrprotocol_oracle::installQuery3Binds(sqlrservercursor *cursor,
						uint32_t block) {

	// the names come out of the query the cursor last prepared, which is
	// the one about to run, whether or not this request re-parsed it
	const char	*query=cont->getQueryBuffer(cursor);
	uint32_t	querysize=cont->getQuerySize(cursor);

	memorypool		*bindpool=cont->getBindPool(cursor);
	bindpool->clear();
	sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);
	sqlrserverbindvar	*outbinds=cont->getOutputBinds(cursor);

	debugStart("installing binds");

	// whatever the previous execute's ref cursor binds took out of the
	// pool goes back before this one takes any
	uint16_t	parentid=cont->getId(cursor);
	releaseRefCursors(parentid);

	uint16_t	incount=0;
	uint16_t	outcount=0;
	for (uint32_t i=0; i<query3binddescs && incount<maxbindcount; i++) {

		oraclequery3bind	*bd=&(query3binds[i]);

		bd->outindex=-1;

		// an output-only bind still takes a value slot on the wire,
		// but there's no input value to install
		if (!(bd->flags&BIND_FLAG_USE_INDICATORS) &&
			bd->type!=ORACLE_TYPE_RESULT_SET) {
			continue;
		}

		const char	*name=NULL;
		uint16_t	namesize=0;
		if (!getBindVariableName(query,querysize,
						(uint16_t)i,&name,&namesize)) {
			debugWrite("no placeholder %d in the query",i+1);
			debugEnd();
			return false;
		}

		// a ref cursor's value is a cursor of its own rather than a
		// buffer.  the statement opens it, and the client drives it
		// afterwards from the id the response hands back, so one
		// comes out of the pool here and the statement holds it
		// until it re-executes or closes
		if (bd->type==ORACLE_TYPE_RESULT_SET) {

			if (outcount>=maxbindcount) {
				continue;
			}

			sqlrservercursor	*child=cont->getCursor();
			if (!child) {
				debugWrite("couldn't get cursor");
				debugEnd();
				return false;
			}
			uint16_t	childid=cont->getId(child);

			// the cursor comes out of the pool with whatever its
			// last user left on it, and nothing else clears it
			cont->setInputBindCount(child,0);
			cont->setOutputBindCount(child,0);
			cont->setInputOutputBindCount(child,0);
			cursorbindcounts[childid]=0;
			columntypescached[childid]=false;
			rowssent[childid]=0;
			pendingrow[childid].clear();
			clearLobPin(childid);

			sqlrserverbindvar	*cbv=&(outbinds[outcount]);

			cbv->variablesize=(int16_t)(namesize+1);
			cbv->variable=(char *)bindpool->allocate(
						(size_t)(namesize+2));
			cbv->variable[0]=cont->getBindFormat()[0];
			bytestring::copy(cbv->variable+1,name,
						(size_t)namesize);
			cbv->variable[namesize+1]='\0';
			cbv->type=SQLRSERVERBINDVARTYPE_CURSOR;
			cbv->valuesize=0;
			cbv->segmentlengths=NULL;
			cbv->segmentcount=0;
			cbv->value.cursorid=childid;
			cbv->isnull=cont->getNonNullBindValue();

			refcursorids[parentid][refcursorcounts[parentid]]=
								childid;
			refcursorcounts[parentid]++;

			bd->outindex=(int16_t)outcount;
			outcount++;

			debugWrite("ref cursor bind: %s (cursor %d)",
						cbv->variable,childid);
			continue;
		}

		sqlrserverbindvar	*bv=&(inbinds[incount]);

		bv->variablesize=(int16_t)(namesize+1);
		bv->variable=(char *)bindpool->allocate(
						(size_t)(namesize+2));
		bv->variable[0]=cont->getBindFormat()[0];
		bytestring::copy(bv->variable+1,name,(size_t)namesize);
		bv->variable[namesize+1]='\0';

		// the bind var array is allocated once per cursor and nothing
		// else clears these, so a slot can still hold a pointer from
		// whatever segmented a bind here last
		bv->segmentlengths=NULL;
		bv->segmentcount=0;

		oraclequery3bindvalue	*v=&(query3bindvalues[
					block*query3binddescs+i]);

		bv->valuesize=0;
		bv->isnull=cont->getNonNullBindValue();

		char		numbertext[MAX_NUMBER_TEXT_SIZE];
		uint32_t	numbertextlen=0;

		if (v->isnull) {
			bv->type=SQLRSERVERBINDVARTYPE_NULL;
		} else {
			switch (bd->type) {
				case ORACLE_TYPE_NUMBER:
				case ORACLE_TYPE_VARNUM:
					if (!getNumberField(v->value,v->size,
							numbertext,
							sizeof(numbertext),
							&numbertextlen)) {
						debugWrite("undecodable "
								"number");
						bv->type=
						SQLRSERVERBINDVARTYPE_NULL;
						break;
					}
					bv->type=SQLRSERVERBINDVARTYPE_STRING;
					bv->valuesize=numbertextlen;
					bv->value.stringval=(char *)
						bindpool->allocate(
							numbertextlen+1);
					bytestring::copy(bv->value.stringval,
							numbertext,
							(size_t)numbertextlen);
					bv->value.stringval[numbertextlen]=
									'\0';
					break;
				case ORACLE_TYPE_DATE:
				case ORACLE_TYPE_TIMESTAMP:
				case ORACLE_TYPE_TIMESTAMPTZ:
				case ORACLE_TYPE_TIMESTAMPLTZ:
					if (v->size<ORACLE_DATE_SIZE) {
						debugWrite("truncated date");
						bv->type=
						SQLRSERVERBINDVARTYPE_NULL;
						break;
					}
					bv->type=SQLRSERVERBINDVARTYPE_DATE;
					bv->value.dateval.year=(int16_t)
						((v->value[0]-100)*100+
							v->value[1]-100);
					bv->value.dateval.month=
						(int16_t)v->value[2];
					bv->value.dateval.day=
						(int16_t)v->value[3];
					bv->value.dateval.hour=
						(int16_t)(v->value[4]-1);
					bv->value.dateval.minute=
						(int16_t)(v->value[5]-1);
					bv->value.dateval.second=
						(int16_t)(v->value[6]-1);
					bv->value.dateval.microsecond=0;
					bv->value.dateval.tz=NULL;
					bv->value.dateval.isnegative=false;
					break;
				case ORACLE_TYPE_RAW:
				case ORACLE_TYPE_LONG_RAW:
				case ORACLE_TYPE_BLOB:
				case ORACLE_TYPE_BFILE:
				case ORACLE_TYPE_CLOB:
					bv->type=(bd->type==ORACLE_TYPE_CLOB)?
						SQLRSERVERBINDVARTYPE_CLOB:
						SQLRSERVERBINDVARTYPE_BLOB;
					bv->valuesize=v->size;
					bv->value.stringval=(char *)
						bindpool->allocate(v->size+1);
					bytestring::copy(bv->value.stringval,
							v->value,
							(size_t)v->size);
					bv->value.stringval[v->size]='\0';
					break;
				default:
					bv->type=SQLRSERVERBINDVARTYPE_STRING;
					bv->valuesize=v->size;
					bv->value.stringval=(char *)
						bindpool->allocate(v->size+1);
					bytestring::copy(bv->value.stringval,
							v->value,
							(size_t)v->size);
					bv->value.stringval[v->size]='\0';
					break;
			}
		}

		if (bv->type==SQLRSERVERBINDVARTYPE_NULL) {
			bv->value.stringval=(char *)bindpool->allocate(1);
			bv->value.stringval[0]='\0';
			bv->valuesize=0;
			bv->isnull=cont->getNullBindValue();
		}

		if (getDebug()) {
			debugWrite("variable: %s",bv->variable);
			if (bv->type==SQLRSERVERBINDVARTYPE_NULL) {
				debugWrite("value: NULL");
			} else if (bv->type==SQLRSERVERBINDVARTYPE_DATE) {
				debugWrite("value: %d-%d-%d %d:%d:%d",
					bv->value.dateval.year,
					bv->value.dateval.month,
					bv->value.dateval.day,
					bv->value.dateval.hour,
					bv->value.dateval.minute,
					bv->value.dateval.second);
			} else {
				debugWrite("value: %.*s",
					(int)bv->valuesize,
					bv->value.stringval);
			}
		}

		incount++;

		// an in-out bind gets a second, writable slot.  the backend
		// has no in-out bind of its own - the base class's is a
		// no-op - so the value goes out through the output bind,
		// whose buffer is a plain bidirectional OCIBindByName over a
		// buffer this side owns.  pre-filling that buffer with the
		// value the wire sent is what makes it behave as in-out:
		// what's in the buffer at execute time is what the statement
		// reads, and what the statement writes is what's in it after
		if (bd->direction!=BIND_DIRECTION_INOUT ||
						outcount>=maxbindcount) {
			continue;
		}

		sqlrserverbindvar	*obv=&(outbinds[outcount]);

		obv->variable=bv->variable;
		obv->variablesize=bv->variablesize;
		obv->type=SQLRSERVERBINDVARTYPE_STRING;
		obv->segmentlengths=NULL;
		obv->segmentcount=0;

		// room for whatever the statement writes back, which can be
		// wider than what came in
		uint32_t	outsize=bv->valuesize;
		if (bd->buffersize>outsize) {
			outsize=bd->buffersize;
		}
		if (outsize<MIN_OUT_BIND_SIZE) {
			outsize=MIN_OUT_BIND_SIZE;
		}
		if (maxstringbindvaluesize && outsize>maxstringbindvaluesize) {
			outsize=maxstringbindvaluesize;
		}

		obv->valuesize=outsize;
		obv->value.stringval=(char *)bindpool->allocate(outsize+1);
		bytestring::zero(obv->value.stringval,(size_t)outsize+1);

		// the value going in.  a null one is still bound not-null
		// with an empty buffer, since the statement may well be
		// about to write something into it
		if (bv->type==SQLRSERVERBINDVARTYPE_STRING &&
						bv->valuesize<=outsize) {
			bytestring::copy(obv->value.stringval,
					bv->value.stringval,
					(size_t)bv->valuesize);
		}
		obv->isnull=cont->getNonNullBindValue();

		bd->outindex=(int16_t)outcount;
		outcount++;

		debugWrite("out bind: %s (%d bytes)",obv->variable,outsize);
	}

	cont->setInputBindCount(cursor,incount);
	cont->setOutputBindCount(cursor,outcount);

	debugWrite("bind count: %d",incount);
	debugWrite("out bind count: %d",outcount);
	debugEnd();

	return true;
}

// opens each ref cursor bind's result set.  the statement's execute is what
// fills the cursor in, but nothing reads its column metadata until this
// runs - and the response can't describe a cursor it hasn't described yet
bool sqlrprotocol_oracle::fetchFromRefCursors(sqlrservercursor *cursor,
						sqlrservercursor **failed) {

	*failed=NULL;

	for (uint32_t i=0; i<query3binddescs; i++) {

		oraclequery3bind	*bd=&(query3binds[i]);

		if (bd->type!=ORACLE_TYPE_RESULT_SET || bd->outindex<0) {
			continue;
		}

		sqlrserverbindvar	*outbinds=cont->getOutputBinds(cursor);
		if (bd->outindex>=(int16_t)cont->getOutputBindCount(cursor)) {
			continue;
		}

		uint16_t	childid=(uint16_t)
				outbinds[bd->outindex].value.cursorid;
		sqlrservercursor	*child=cont->getCursor(childid);
		if (!child) {
			continue;
		}

		debugWrite("fetching from ref cursor %d",childid);

		if (!cont->fetchFromBindCursor(child)) {
			debugWrite("fetch from ref cursor failed");
			*failed=child;
			return false;
		}
	}

	return true;
}

// hands back what a statement's ref cursor binds took out of the pool.  a
// client that opens ref cursors in a loop runs the pool dry without this
void sqlrprotocol_oracle::releaseRefCursors(uint16_t parentid) {

	for (uint16_t i=0; i<refcursorcounts[parentid]; i++) {

		uint16_t	childid=refcursorids[parentid][i];

		sqlrservercursor	*child=cont->getCursor(childid);
		if (!child) {
			continue;
		}

		debugWrite("releasing ref cursor %d",childid);

		cont->abort(child);
		cont->release(child);
		columntypescached[childid]=false;
		rowssent[childid]=0;
		pendingrow[childid].clear();
		clearLobPin(childid);
		if (lastcursorid==childid) {
			lastcursorid=65535;
		}
	}

	refcursorcounts[parentid]=0;
}

// drops a cursor the client closed itself from whatever statement was
// holding it, so the statement doesn't hand back a cursor the pool has
// since given to something else
void sqlrprotocol_oracle::forgetRefCursor(uint16_t childid) {

	for (uint16_t i=0; i<maxcursorcount; i++) {

		uint16_t	count=refcursorcounts[i];
		uint16_t	kept=0;
		for (uint16_t j=0; j<count; j++) {
			if (refcursorids[i][j]!=childid) {
				refcursorids[i][kept]=refcursorids[i][j];
				kept++;
			}
		}
		refcursorcounts[i]=kept;
	}
}

// keeps the descriptors that came with the statement, since a re-execute
// sends fresh values without them
void sqlrprotocol_oracle::saveQuery3Binds(sqlrservercursor *cursor) {
	uint32_t	count=(query3binddescs<maxbindcount)?
					query3binddescs:maxbindcount;
	oraclequery3bind	*saved=cursorbinds[cont->getId(cursor)];
	for (uint32_t i=0; i<count; i++) {
		saved[i]=query3binds[i];
	}
	cursorbindcounts[cont->getId(cursor)]=count;
}

void sqlrprotocol_oracle::restoreQuery3Binds(sqlrservercursor *cursor) {
	uint32_t	count=cursorbindcounts[cont->getId(cursor)];
	if (count>query3bindavail) {
		delete[] query3binds;
		query3binds=new oraclequery3bind[count];
		query3bindavail=count;
	}
	oraclequery3bind	*saved=cursorbinds[cont->getId(cursor)];
	for (uint32_t i=0; i<count; i++) {
		query3binds[i]=saved[i];
	}
	query3binddescs=count;
}

bool sqlrprotocol_oracle::sendQuery3Response(sqlrservercursor *cursor,
						uint32_t options,
						uint32_t cursorid,
						uint32_t prefetchrows,
						uint32_t maxlongsize) {

	// what a live 11.2 server answers a select with, in order: a describe,
	// a row header, one row data message per row, a return parameters
	// block, and a summary object, each with its own ttc code inside the
	// one packet
	resetSendPacketBuffer(PACKET_DATA);

	uint32_t	colcount=cont->colCount(cursor);
	cacheColumnDefinitions(cursor,colcount);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	// an out bind's direction and its value go in front of everything
	// else the response carries.  a statement with out binds has no
	// result set of its own, so this never runs alongside the describe
	// and rows below
	if ((options&OPTION_SNDIOV) && hasQuery3OutBinds()) {
		putIoVector();
		putOutBindValues(cursor);
	}

	uint32_t	rowsfetched=0;
	bool		endofrows=false;

	if (colcount) {

		// the describe, which the client needs before it can make
		// sense of a row
		if (options&(OPTION_PARSE|OPTION_DESCRIBE)) {
			putDescribeInfo(cursor,colcount);
		}

		// the rows.  a modern client doesn't set OPTION_FETCH on the
		// first execute - it asks for a prefetch through the row count
		// in the request, and a real server sends rows for that anyway.
		uint32_t	rowstofetch=prefetchrows;
		if (!rowstofetch && (options&OPTION_FETCH)) {
			rowstofetch=1;
		}

		// A describe asks about the statement, not for its data.  A
		// row sent in answer to one leaves the client a row ahead
		// for the rest of the result set.
		if (options&OPTION_DESCRIBE) {
			rowstofetch=0;
		}

		// a long or a long raw has no bounded width, and the client
		// says in its request how much of one it has room for.  a
		// live 12.2 server sends no rows at all until it does -
		// answering an execute that asks for a row but names no size
		// with the describe and nothing else, and sending the row
		// when the fetch that follows names one.  OCI defines its
		// columns after that execute and crashes on a row that
		// arrives before it does
		if (!maxlongsize && hasLongColumn(cursor,colcount)) {
			debugWrite("no long size, no rows");
			rowstofetch=0;
		}

		// a lob's contents are read out of the connection's current
		// row - there's no way to address a row it has moved on from -
		// so a result set with a lob column in it sends one row per
		// response, and that row stays current until the pin comes off.
		//
		// none at all go out on an execute, the same way none go out
		// for a long: OCI allocates the lob descriptors its defines
		// name as the answer to the execute comes in, and a locator
		// that arrives ahead of them is ORA-24813 - "cannot send or
		// receive an unsupported LOB" - which kills the call.  the
		// fetch that follows carries the row instead
		if (hasLobColumn(cursor,colcount)) {
			debugWrite("lob column, no rows on the execute");
			rowstofetch=0;
		}

		if (rowstofetch) {

			putRowHeader(0x22,colcount,rowstofetch);

			// the only bound on how many rows to send back in
			// this packet is the negotiated packet size, less
			// enough room for the return parameters and the
			// trailing summary
			const uint32_t	trailerreserve=128;
			uint32_t	curid=cont->getId(cursor);

			while (rowsfetched<rowstofetch) {

				// a row held over from a previous
				// packet-full response goes out first,
				// ahead of anything freshly fetched
				if (pendingrow[curid].getSize()) {
					if (rowsfetched &&
						reqpacket.getSize()+
						pendingrow[curid].getSize()+
						trailerreserve>=sdu) {
						debugWrite("packet full");
						break;
					}
					reqpacket.append(
						pendingrow[curid].getBuffer(),
						pendingrow[curid].getSize());
					pendingrow[curid].clear();
					rowsfetched++;
					continue;
				}

				uint32_t	sizebeforerow=
						(uint32_t)reqpacket.getSize();

				bool	error=false;
				if (!cont->fetchRow(cursor,&error)) {
					if (error) {
						return sendQueryError(cursor);
					}
					endofrows=true;
					break;
				}

				rowhaslob=false;

				debugStart("query3 response row");
				putRowData(cursor,colcount);
				debugEnd();

				// a row that handed out a locator pins the
				// connection to itself, so the cursor doesn't
				// advance past it and the row never goes into
				// pendingrow - which assumes the connection has
				// already moved on
				if (rowhaslob) {
					pinLobRow(cursor,colcount);
					rowsfetched++;
					break;
				}

				// the row is consumed from the result set as
				// soon as it's fetched - fetchRow()/nextRow()
				// can't un-fetch it on every backend, so a row
				// that doesn't fit here is stashed in
				// pendingrow and sent first next time, rather
				// than left for a re-fetch that some backends
				// can't do
				// FIXME: kludgy
				cont->nextRow(cursor);

				// a row's size isn't known until it's
				// written, so the check comes after - stash
				// it and stop if it doesn't fit, unless it's
				// the first row, which goes out regardless of
				// its size since the packet can't say "zero
				// rows" when more remain
				if (rowsfetched &&
					reqpacket.getSize()+trailerreserve
									>=sdu) {
					debugWrite("packet full");
					uint32_t	rowsize=(uint32_t)
						reqpacket.getSize()-
						sizebeforerow;
					pendingrow[curid].append(
						reqpacket.getBuffer()+
							sizebeforerow,
						(size_t)rowsize);
					reqpacket.truncate(
							(size_t)sizebeforerow);
					break;
				}

				rowsfetched++;
			}
		}
	}

	putReturnParameters();

	rowssent[cont->getId(cursor)]+=rowsfetched;

	// the row count the summary object carries, which a client reads back
	// as OCI_ATTR_ROW_COUNT: the rows sent so far for a statement with a
	// result set, and the affected row count for one without - an insert
	// or a delete sends no rows, so it answered 0.  the affected count is
	// summed over the request's execution iterations by query3(), and is
	// only meaningful when it actually executed - a parse alone leaves
	// the last execution's count sitting on the cursor
	uint32_t	rowcount=rowssent[cont->getId(cursor)];
	if (!colcount && (options&OPTION_EXECUTE) && query3knowsaffectedrows) {
		rowcount=query3affectedrows;
	}

	// a prefetch that ran out of rows ends in ORA-01403, which the client
	// reads as "no more rows"; one that filled it ends clean
	if (endofrows) {
		putSummary(cursorid,ORA_NO_DATA_FOUND,
					rowcount,
					ORA_NO_DATA_FOUND_MESSAGE);
	} else {
		putSummary(cursorid,0,rowcount,NULL);
	}

	if (getDebug()) {
		debugStart("query3 response");
		debugWrite("data flags: 0x%04x",dataflags);
		debugWrite("column count: %d",colcount);
		debugWrite("rows: %d",rowsfetched);
		debugWrite("end of rows: %s",(endofrows)?"true":"false");
		debugEnd();
	}

	return sendPacket(true);
}

// tells the client which way each bind went.  four of the six header fields
// were zero in every capture and nothing explains them; the third was always
// one.  they go out exactly as the live server sends them
// see "Oracle Wire Protocol - Query3"
void sqlrprotocol_oracle::putIoVector() {

	write(&reqpacket,(byte_t)TTC_IO_VECTOR);
	write(&reqpacket,(byte_t)IO_VECTOR_CONSTANT);
	writeLenPreInt(&reqpacket,query3binddescs);
	writeLenPreInt(&reqpacket,(uint32_t)0);
	writeLenPreInt(&reqpacket,(uint32_t)1);
	writeLenPreInt(&reqpacket,(uint32_t)0);
	writeLenPreInt(&reqpacket,(uint32_t)0);
	writeLenPreInt(&reqpacket,(uint32_t)0);

	debugStart("io vector");
	debugWrite("bind count: %d",query3binddescs);
	for (uint32_t i=0; i<query3binddescs; i++) {
		write(&reqpacket,query3binds[i].direction);
		debugWrite("bind %d: 0x%02x",i+1,query3binds[i].direction);
	}
	debugEnd();
}

// what the statement left in each out bind, in descriptor order.  an in-only
// bind contributes nothing here - the client isn't expecting one for it -
// and every value carries a signed indicator behind it
// see "Oracle Wire Protocol - Query3"
void sqlrprotocol_oracle::putOutBindValues(sqlrservercursor *cursor) {

	write(&reqpacket,(byte_t)TTC_ROW_DATA);

	sqlrserverbindvar	*outbinds=cont->getOutputBinds(cursor);
	uint16_t		outbindcount=cont->getOutputBindCount(cursor);

	debugStart("out bind values");

	for (uint32_t i=0; i<query3binddescs; i++) {

		oraclequery3bind	*bd=&(query3binds[i]);

		if (!(bd->direction&BIND_DIRECTION_OUT)) {
			continue;
		}

		// a descriptor the install didn't get to still owes the
		// client a slot, and null is the honest answer for one
		sqlrserverbindvar	*bv=(bd->outindex>=0 &&
					bd->outindex<(int16_t)outbindcount)?
					&(outbinds[bd->outindex]):NULL;

		// a ref cursor's slot is a whole describe and the id of the
		// cursor it describes, rather than a value
		if (bv && bv->type==SQLRSERVERBINDVARTYPE_CURSOR) {
			sqlrservercursor	*child=cont->getCursor(
					(uint16_t)bv->value.cursorid);
			if (child) {
				debugWrite("bind %d: cursor %d",
					i+1,(uint16_t)bv->value.cursorid);
				putRefCursorBindValue(child);
			} else {
				// the child cursor is gone - answer null rather
				// than reading the value union as a stringval
				debugWrite("bind %d: NULL (cursor gone)",i+1);
				write(&reqpacket,(byte_t)0);
				write(&reqpacket,
					(byte_t)OUT_BIND_NULL_INDICATOR_COUNT);
				write(&reqpacket,
					(byte_t)OUT_BIND_NULL_INDICATOR_VALUE);
			}
			continue;
		}

		if (!bv || cont->getBindValueIsNull(bv->isnull)) {
			debugWrite("bind %d: NULL",i+1);
			write(&reqpacket,(byte_t)0);
			write(&reqpacket,
				(byte_t)OUT_BIND_NULL_INDICATOR_COUNT);
			write(&reqpacket,
				(byte_t)OUT_BIND_NULL_INDICATOR_VALUE);
			continue;
		}

		// the buffer is bound the width of the whole allocation, so
		// what came back out of it is however much of it is text
		const char	*value=bv->value.stringval;
		uint32_t	size=(uint32_t)charstring::getLength(value);

		debugWrite("bind %d: \"%.*s\"",i+1,(int)size,value);

		if (bd->type==ORACLE_TYPE_NUMBER ||
				bd->type==ORACLE_TYPE_VARNUM) {
			putNumberField(value,size);
		} else {
			putLenBytes(value,size);
		}
		write(&reqpacket,(byte_t)0);
	}

	debugEnd();
}

// what a ref cursor's out bind slot carries: a constant, the describe of
// the cursor the statement opened, and the id the client drives it with.
// unlike a scalar out bind's slot, no null indicator follows
// see "Oracle Wire Protocol - Query3"
void sqlrprotocol_oracle::putRefCursorBindValue(sqlrservercursor *child) {

	write(&reqpacket,(byte_t)REF_CURSOR_CONSTANT);

	uint32_t	colcount=cont->colCount(child);
	cacheColumnDefinitions(child,colcount);
	putDescribeInfoBody(child,colcount);

	// the ids on the wire are the controller's plus 1
	writeLenPreInt(&reqpacket,(uint32_t)(cont->getId(child)+1));
}

void sqlrprotocol_oracle::putDescribeInfo(sqlrservercursor *cursor,
						uint32_t colcount) {

	write(&reqpacket,(byte_t)TTC_DESCRIBE_INFO);

	// a block the client skips whole - 16 bytes of query hash and a 7 byte
	// date, and the module has no hash to report.
	//
	// the length in front of it is the one field the two client families
	// disagree about.  a thin driver reads a single length byte -
	// node-oracledb's skipBytesChunked() in processMessage(), in
	// lib/thin/protocol/messages/withData.js - and OCI reads a count
	// prefixed integer.  captured from a live 12.2 server answering the
	// same "select 1 from dual" through the same proxy: 17 to ojdbc and
	// 01 17 to OCI, with the 23 bytes identical either way.  given the
	// wrong one, OCI takes the block for an integer, runs 22 bytes past
	// the end of the field, and answers with a marker packet - the
	// ORA-03113 this fixes
	byte_t	prologue[23];
	bytestring::zero(prologue,sizeof(prologue));
	putOracleDate(prologue+16);
	if (ociclient) {
		writeLenPreInt(&reqpacket,(uint32_t)sizeof(prologue));
		write(&reqpacket,(const char *)prologue,sizeof(prologue));
	} else {
		putLenBytes((const char *)prologue,sizeof(prologue));
	}

	putDescribeInfoBody(cursor,colcount);
}

// everything a describe says about a statement's columns, from the max row
// size on.  a ref cursor's out bind slot carries the same fields with no ttc
// code and no prologue in front of them, so both writers share this
void sqlrprotocol_oracle::putDescribeInfoBody(sqlrservercursor *cursor,
						uint32_t colcount) {

	uint16_t	curid=cont->getId(cursor);
	uint32_t	maxrowsize=0;
	for (uint32_t i=0; i<colcount; i++) {
		maxrowsize+=getWireColumnSize(cursor,i,
				cont->getColumnTypeName(cursor,i),
				columntypes[curid][i],
				getWireColumnType(columntypes[curid][i]));
	}

	writeLenPreInt(&reqpacket,maxrowsize);
	writeLenPreInt(&reqpacket,colcount);
	if (colcount) {
		// unexplained.  see "Oracle Wire Protocol - Describe Info"
		write(&reqpacket,(byte_t)DESCRIBE_INFO_CONSTANT);
	}

	debugStart("describe info");
	debugWrite("max row size: %d",maxrowsize);
	debugWrite("column count: %d",colcount);

	for (uint32_t i=0; i<colcount; i++) {
		putColumnMetadata(cursor,i);
	}

	debugEnd();

	byte_t	date[ORACLE_DATE_SIZE];
	putOracleDate(date);
	putDalc((const char *)date,sizeof(date));

	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,DCB_MAX_DATA_BLOCK_SIZE);
	writeLenPreInt(&reqpacket,DCB_MIN_PREFETCH);
	writeLenPreInt(&reqpacket,DCB_MAX_PREFETCH);
	writeLenPreInt(&reqpacket,0);
}

void sqlrprotocol_oracle::putColumnMetadata(sqlrservercursor *cursor,
						uint32_t column) {

	uint16_t	curid=cont->getId(cursor);
	const char	*columntypestring=
				cont->getColumnTypeName(cursor,column);
	uint16_t	columntype=columntypes[curid][column];
	uint16_t	wiretype=getWireColumnType(columntype);
	uint32_t	size=getWireColumnSize(cursor,column,
						columntypestring,
						columntype,wiretype);

	// the wire protocol's "0x80" flag and several fields below it are
	// gated on the type rather than on being a character type.  a live
	// 12.2 server sends the abbreviated encoding - a 0x00 flag byte, no
	// character set and no second size - for the types that have no
	// character set of their own, and the full encoding for everything
	// else.  a raw, a long raw, both interval types and both timestamp
	// types are binary and get the abbreviated form, but a long is text
	// and gets the full one
	bool	fullencoding=(wiretype!=ORACLE_TYPE_NUMBER &&
				wiretype!=ORACLE_TYPE_ROWID_DEPRECATED &&
				wiretype!=ORACLE_TYPE_RAW &&
				wiretype!=ORACLE_TYPE_LONG_RAW &&
				wiretype!=ORACLE_TYPE_INTERVALYM &&
				wiretype!=ORACLE_TYPE_INTERVALDS &&
				wiretype!=ORACLE_TYPE_TIMESTAMP &&
				wiretype!=ORACLE_TYPE_TIMESTAMPTZ &&
				wiretype!=ORACLE_TYPE_BLOB &&
				wiretype!=ORACLE_TYPE_BFILE);

	const char	*name=cont->getColumnName(cursor,column);
	uint32_t	namesize=cont->getColumnNameSize(cursor,column);

	// the precision and scale, as the backend reports them
	uint32_t	precision=cont->getColumnPrecision(cursor,column);
	uint32_t	scale=cont->getColumnScale(cursor,column);
	int8_t		wirescale=(scale==NO_SCALE_UNSIGNED)?
					NO_SCALE:(int8_t)scale;

	write(&reqpacket,(byte_t)wiretype);
	write(&reqpacket,(byte_t)((fullencoding)?0x80:0x00));

	putColumnPrecisionScale((int8_t)precision,wirescale);

	// a buffer size of 0 means "this column is null by describe", so it
	// can never be sent as 0 for a column that has values
	writeLenPreInt(&reqpacket,size);

	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,(fullencoding)?charset:0);
	write(&reqpacket,(byte_t)((fullencoding)?1:0));
	writeLenPreInt(&reqpacket,(fullencoding)?size:0);
	write(&reqpacket,(byte_t)1);
	write(&reqpacket,(byte_t)namesize);
	putDalc(name,namesize);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,column);
	writeLenPreInt(&reqpacket,0);

	debugStart("column %d",column);
	debugColumnType(columntypestring,wiretype);
	debugWrite("size: %d",size);
	debugWrite("name: %.*s",(int)namesize,name);
	debugEnd();
}

void sqlrprotocol_oracle::putColumnPrecisionScale(int8_t precision,
							int8_t scale) {

	debugStart("precision/scale");
	debugWrite("precision: %d",(int32_t)precision);
	debugWrite("scale: %d",(int32_t)scale);
	debugEnd();

	write(&reqpacket,(byte_t)precision);

	// the scale is the one place the two integer encodings differ - a
	// client that set ENCODING_CONV_LENGTH reads it as a raw signed byte
	// and one that didn't reads it as a count prefixed integer.  the
	// precision is a raw signed byte to both.
	//
	// verified against a live 12.2 server: it answers the scale of a
	// number(10,2) with 02 when the flag is set and 01 02 when it is not,
	// and the precision with 0a either way, regardless of field version.
	// most clients set the flag; ojdbc 23.26 doesn't, and a client that
	// gets the form it did not ask for is one byte out for the rest of
	// the packet - node-oracledb reports "read integer of length 127
	// when expecting integer of no more than length 4", and
	// python-oracledb and OCI answer with a marker packet and close.
	//
	// OCI sets the flag and still reads the count prefixed form: the same
	// live 12.2 server answers the scale -127 of "select 1 from dual"
	// with 81 7f to OCI and to ojdbc, and node-oracledb reads it as a raw
	// signed byte (readInt8() in processColumnInfo()), so no one form
	// serves all three
	if ((encodingflags&ENCODING_CONV_LENGTH) && !ociclient) {
		write(&reqpacket,(byte_t)scale);
	} else {
		writeLenPreInt(&reqpacket,scale);
	}
}

uint16_t sqlrprotocol_oracle::getWireColumnType(uint16_t columntype) {

	uint16_t	wiretype;
	switch (columntype) {
		case ORACLE_TYPE_NUMBER:
		case ORACLE_TYPE_VARNUM:
			wiretype=ORACLE_TYPE_NUMBER;
			break;
		case ORACLE_TYPE_CHAR:
		case ORACLE_TYPE_FIXED_CHAR:
			wiretype=ORACLE_TYPE_CHAR;
			break;
		case ORACLE_TYPE_DATE:
			wiretype=ORACLE_TYPE_DATE;
			break;
		case ORACLE_TYPE_ROWID:
		case ORACLE_TYPE_ROWID_DEPRECATED:
			// a live 12.2 server describes a rowid column, real
			// or the pseudo-column, as 11 rather than 104, to an
			// oci client and to a thin driver alike
			wiretype=ORACLE_TYPE_ROWID_DEPRECATED;
			break;
		case ORACLE_TYPE_RAW:
			wiretype=ORACLE_TYPE_RAW;
			break;
		case ORACLE_TYPE_LONG_RAW:
			wiretype=ORACLE_TYPE_LONG_RAW;
			break;
		case ORACLE_TYPE_LONG:
			wiretype=ORACLE_TYPE_LONG;
			break;
		case ORACLE_TYPE_INTERVALYM:
			wiretype=ORACLE_TYPE_INTERVALYM;
			break;
		case ORACLE_TYPE_INTERVALDS:
			wiretype=ORACLE_TYPE_INTERVALDS;
			break;
		case ORACLE_TYPE_TIMESTAMP:
			wiretype=ORACLE_TYPE_TIMESTAMP;
			break;
		case ORACLE_TYPE_TIMESTAMPTZ:
			wiretype=ORACLE_TYPE_TIMESTAMPTZ;
			break;
		case ORACLE_TYPE_LOB_CLOB:
			wiretype=ORACLE_TYPE_CLOB;
			break;
		case ORACLE_TYPE_LOB_BLOB:
			wiretype=ORACLE_TYPE_BLOB;
			break;
		case ORACLE_TYPE_LOB_BFILE:
			wiretype=ORACLE_TYPE_BFILE;
			break;
		default:
			// anything the module can't encode is described as a
			// varchar2 and sent as text - describing it as its own
			// type and sending text desyncs the client - so a type
			// only moves out of here once putRowData() can write
			// its binary form
			wiretype=ORACLE_TYPE_VARCHAR;
			break;
	}

	debugStart("wire column type");
	debugWrite("input type: 0x%02x",(uint32_t)(0x000000ff&columntype));
	debugColumnType(wiretype);
	debugEnd();

	return wiretype;
}

bool sqlrprotocol_oracle::isCharacterColumn(const char *columntypestring,
						uint16_t columntype) {

	if (columntype!=ORACLE_TYPE_CHAR &&
		columntype!=ORACLE_TYPE_FIXED_CHAR &&
		columntype!=ORACLE_TYPE_VARCHAR) {
		return false;
	}

	// a type the backend didn't recognize comes through named UNKNOWN,
	// and getColumnType() maps that to a varchar2 like it does a real
	// one.  nothing the backend says about such a column is dependable,
	// its size least of all
	const char * const	*datatypestring=cont->dataTypeStrings();
	return charstring::compareIgnoringCase(columntypestring,
						datatypestring[0])!=0;
}

bool sqlrprotocol_oracle::hasLongColumn(sqlrservercursor *cursor,
						uint32_t colcount) {

	uint16_t	*ct=columntypes[cont->getId(cursor)];
	for (uint32_t i=0; i<colcount; i++) {
		uint16_t	wiretype=getWireColumnType(ct[i]);
		if (wiretype==ORACLE_TYPE_LONG ||
			wiretype==ORACLE_TYPE_LONG_RAW) {
			return true;
		}
	}
	return false;
}

bool sqlrprotocol_oracle::isLobColumnType(uint16_t columntype) {
	return (columntype==ORACLE_TYPE_LOB_CLOB ||
		columntype==ORACLE_TYPE_LOB_BLOB ||
		columntype==ORACLE_TYPE_LOB_BFILE);
}

bool sqlrprotocol_oracle::hasLobColumn(sqlrservercursor *cursor,
						uint32_t colcount) {

	uint16_t	*ct=columntypes[cont->getId(cursor)];
	if (!ct) {
		return false;
	}
	for (uint32_t i=0; i<colcount; i++) {
		if (isLobColumnType(ct[i])) {
			return true;
		}
	}
	return false;
}

uint32_t sqlrprotocol_oracle::getWireColumnSize(sqlrservercursor *cursor,
						uint32_t column,
						const char *columntypestring,
						uint16_t columntype,
						uint16_t wiretype) {

	// the types the module encodes itself are always the same width on
	// the wire, whatever the backend reports for them
	uint32_t	size;
	if (wiretype==ORACLE_TYPE_NUMBER) {
		size=MAX_NUMBER_SIZE;
	} else if (wiretype==ORACLE_TYPE_DATE) {
		size=ORACLE_DATE_SIZE;
	} else if (wiretype==ORACLE_TYPE_ROWID_DEPRECATED) {
		// not the 18 characters a rowid prints as, and not the 8
		// bytes it stores in - a live 12.2 server describes one as 1
		// byte wide, and oci works the display size out from the type
		size=ORACLE_ROWID_SIZE;
	} else if (wiretype==ORACLE_TYPE_RAW) {
		// the declared width in bytes, which is what the backend
		// reports, not the two characters per byte it hands the
		// value over as - putRawField() decodes those back to bytes
		size=cont->getColumnSize(cursor,column);
		if (!size) {
			size=MAX_RAW_SIZE;
		}
	} else if (wiretype==ORACLE_TYPE_LONG_RAW ||
			wiretype==ORACLE_TYPE_LONG) {
		size=ORACLE_LONG_SIZE;
	} else if (wiretype==ORACLE_TYPE_INTERVALYM ||
			wiretype==ORACLE_TYPE_INTERVALDS) {
		// not the 5 or 11 bytes the binary form takes - a live 12.2
		// server describes either interval 1 byte wide, and oci
		// works the size it reports out from the type
		size=ORACLE_INTERVAL_SIZE;
	} else if (wiretype==ORACLE_TYPE_TIMESTAMP) {
		// the 11 bytes the binary form really takes, which is what
		// a live 12.2 server describes a timestamp column as
		size=ORACLE_TIMESTAMP_SIZE;
	} else if (wiretype==ORACLE_TYPE_TIMESTAMPTZ) {
		// not the 13 bytes the binary form takes - a live 12.2
		// server describes a timestamp with time zone 1 byte wide,
		// the way it does an interval, and oci works the 13 it
		// reports back out from the type
		size=ORACLE_TIMESTAMPTZ_WIRE_SIZE;
	} else if (wiretype==ORACLE_TYPE_CLOB ||
			wiretype==ORACLE_TYPE_BLOB) {
		// not the width of the lob, which has no bound - the width of
		// the buffer the client reads a locator into
		size=ORACLE_LOB_SIZE;
	} else if (wiretype==ORACLE_TYPE_BFILE) {
		// same, and a bfile's locator is described wider still since
		// it carries a directory alias and a file name
		size=ORACLE_BFILE_SIZE;
	} else if (!isCharacterColumn(columntypestring,columntype)) {
		// everything else goes out as text, and this size is the
		// buffer the client reads that text into.  the backend's
		// size is the width of the column's own storage, and that
		// only matches the text for a character column - a rowid
		// stores in 8 bytes and prints as 18, a timestamp stores in
		// 11 and prints as 28, a raw(20) prints as 40.  describing
		// the storage width hands the client a buffer too small for
		// the value it gets, and it stops reading part way through
		// the row and waits for bytes that never come - so describe
		// a column whose text width isn't known as the widest a
		// varchar2 can be
		size=MAX_VARCHAR_SIZE;
	} else {
		size=cont->getColumnSize(cursor,column);
		if (!size || size>MAX_VARCHAR_SIZE) {
			size=MAX_VARCHAR_SIZE;
		}
	}

	debugStart("wire column size");
	debugWrite("column: %d",column);
	debugColumnType(wiretype);
	debugWrite("size: %d",size);
	debugEnd();

	return size;
}

void sqlrprotocol_oracle::putRowHeader(byte_t flags,
						uint32_t colcount,
						uint32_t prefetchrows) {

	write(&reqpacket,(byte_t)TTC_ROW_HEADER);

	// 0x22 in the answer to an execute and 0x02 in the answer to a fetch
	write(&reqpacket,flags);

	// column count (rxh.numRqsts), iteration number (rxh.iterNum), row
	// count (rxh.numItersThisTime), uac buffer length (rxh.uacBufLength)
	writeLenPreInt(&reqpacket,colcount);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,prefetchrows);
	writeLenPreInt(&reqpacket,0);

	// bit vector size.  a real server sends a bit vector here to say which
	// columns repeat the previous row's value; a zero size sends every
	// column of every row in full.
	writeLenPreInt(&reqpacket,0);

	// meaning unknown.  the 8i capture ends its row header at the uac
	// buffer length, so this and the bit vector size are later additions.
	// see "Oracle Wire Protocol - Row Header"
	writeLenPreInt(&reqpacket,0);

	debugStart("row header");
	debugWrite("flags: 0x%02x",(uint32_t)flags);
	debugWrite("column count: %d",colcount);
	debugWrite("prefetch rows: %d",prefetchrows);
	debugEnd();
}

void sqlrprotocol_oracle::putRowData(sqlrservercursor *cursor,
						uint32_t colcount) {

	write(&reqpacket,(byte_t)TTC_ROW_DATA);

	uint16_t	*ct=columntypes[cont->getId(cursor)];

	for (uint32_t i=0; i<colcount; i++) {

		const char	*field=NULL;
		uint64_t	fieldsize=0;
		bool		lob=false;
		bool		null=false;
		if (!cont->getField(cursor,i,&field,&fieldsize,&lob,&null)) {
			null=true;
		}

		debugStart("col %d",i);

		uint16_t	wiretype=getWireColumnType(ct[i]);

		// a lob column's value is a locator rather than the lob's
		// bytes - the client reads the bytes themselves back with lob
		// operation requests of its own, quoting the locator
		if (wiretype==ORACLE_TYPE_CLOB ||
			wiretype==ORACLE_TYPE_BLOB ||
			wiretype==ORACLE_TYPE_BFILE) {
			debugWrite("lob");
			putLobLocator(cursor,i,wiretype,(null || !lob));
			if (!null && lob) {
				rowhaslob=true;
			}

		// one zero byte stands for null and unreadable alike.
		// see "Oracle Wire Protocol - Row Data"
		} else if (null || lob || !field) {
			debugWrite("null");
			if (wiretype==ORACLE_TYPE_LONG ||
				wiretype==ORACLE_TYPE_LONG_RAW) {
				putNullLongField();
			} else {
				write(&reqpacket,(byte_t)0);
			}
		} else if (wiretype==ORACLE_TYPE_NUMBER) {
			debugWrite("number: %.*s",(int)fieldsize,field);
			putNumberField(field,(uint32_t)fieldsize);
		} else if (wiretype==ORACLE_TYPE_DATE) {
			debugWrite("date: %.*s",(int)fieldsize,field);
			byte_t	date[ORACLE_DATE_SIZE];
			if (getOracleDate(field,fieldsize,date)) {
				putLenBytes((const char *)date,sizeof(date));
			} else {
				// every column has to write something, and
				// a date that won't parse is no more use to
				// the client than a null
				write(&reqpacket,(byte_t)0);
			}
		} else if (wiretype==ORACLE_TYPE_ROWID_DEPRECATED) {
			debugWrite("rowid: %.*s",(int)fieldsize,field);
			if (!putRowidField(field,fieldsize)) {
				// same as a date that won't parse - a rowid
				// the module can't take apart is no more use
				// to the client than a null
				write(&reqpacket,(byte_t)0);
			}
		} else if (wiretype==ORACLE_TYPE_RAW ||
				wiretype==ORACLE_TYPE_LONG_RAW) {
			debugWrite("raw: %.*s",(int)fieldsize,field);
			bool	longraw=(wiretype==ORACLE_TYPE_LONG_RAW);
			if (!putRawField(field,fieldsize,longraw)) {
				// same as a date that won't parse - bytes
				// the module can't decode are no more use to
				// the client than a null
				if (longraw) {
					putNullLongField();
				} else {
					write(&reqpacket,(byte_t)0);
				}
			}
		} else if (wiretype==ORACLE_TYPE_LONG) {
			debugWrite("long: %.*s",(int)fieldsize,field);
			// a long is text, so nothing has to be decoded the
			// way a long raw does - but it goes out in the same
			// long form, since a long can be far bigger than
			// the short form's 252 bytes
			putLongBytes(field,(uint32_t)fieldsize);
		} else if (wiretype==ORACLE_TYPE_INTERVALYM ||
				wiretype==ORACLE_TYPE_INTERVALDS) {
			debugWrite("interval: %.*s",(int)fieldsize,field);
			// putIntervalField() hand-parses oracle's own
			// interval text - a column that reaches this wire
			// type via the backend-agnostic type name table (see
			// oracletypemap[]'s "INTERVAL" entry) can come from a
			// postgresql column fronted through this module, and
			// postgresql's interval text doesn't look like
			// oracle's - encoding it as though it did risks
			// silently plausible-but-wrong bytes, so only a
			// genuine oracle backend gets the real encoder
			bool	daytosecond=
				(wiretype==ORACLE_TYPE_INTERVALDS);
			if (charstring::compare(
					cont->getNativeDbType(),"oracle") ||
				!putIntervalField(field,fieldsize,
							daytosecond)) {
				// same as a date that won't parse - an
				// interval the module can't take apart is no
				// more use to the client than a null
				write(&reqpacket,(byte_t)0);
			}
		} else if (wiretype==ORACLE_TYPE_TIMESTAMP ||
				wiretype==ORACLE_TYPE_TIMESTAMPTZ) {
			debugWrite("timestamp: %.*s",(int)fieldsize,field);
			// same reasoning as the interval case just above -
			// db2, mysql, firebird, postgresql, sap, odbc and
			// freetds all report their own native timestamp
			// columns under the same "TIMESTAMP"/"TIMESTAMPTZ"
			// names oracle's real ones use, and their timestamp
			// text doesn't parse like oracle's - only encode the
			// real binary form for a genuine oracle backend
			bool	withtimezone=
				(wiretype==ORACLE_TYPE_TIMESTAMPTZ);
			if (charstring::compare(
					cont->getNativeDbType(),"oracle") ||
				!putTimestampField(field,fieldsize,
							withtimezone)) {
				// same as a date that won't parse - a
				// timestamp the module can't take apart is
				// no more use to the client than a null
				write(&reqpacket,(byte_t)0);
			}
		} else {
			debugWrite("\"%.*s\"",(int)fieldsize,field);
			putLenBytes(field,(uint32_t)fieldsize);
		}

		debugEnd();
	}
}

// writes "value" as a count prefixed 8 byte integer, which is what a lob
// length goes out as.  the base class only writes the 4 byte form
void sqlrprotocol_oracle::putLenPreUB8(uint64_t value) {

	if (!value) {
		write(&reqpacket,(byte_t)0);
		return;
	}

	byte_t	bytes[8];
	for (uint16_t i=0; i<8; i++) {
		bytes[i]=(byte_t)((value>>((7-i)*8))&0xff);
	}

	uint16_t	first=0;
	while (!bytes[first]) {
		first++;
	}

	write(&reqpacket,(byte_t)(8-first));
	write(&reqpacket,(const byte_t *)(bytes+first),(size_t)(8-first));
}

static void putLocatorUB2(byte_t *bytes, uint16_t value) {
	bytes[0]=(byte_t)(value>>8);
	bytes[1]=(byte_t)(value&0xff);
}

// builds the locator a lob column's value is made of, and returns how many
// bytes of "locator" it filled in.  what identifies the lob is the cursor
// and the column it came from, in the four bytes a real server fills with a
// per-lob id - two locators in the same row have to differ there - and the
// cursor's pin generation, so a locator echoed back after its row has gone
// can be told from one that's still good.  which row is never encoded: only
// one row per cursor is ever pinned, so the row is whichever one the pin is
// on
uint32_t sqlrprotocol_oracle::buildLobLocator(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t wiretype,
						byte_t *locator) {

	uint16_t	curid=cont->getId(cursor);

	if (wiretype==ORACLE_TYPE_BFILE) {

		bytestring::zero(locator,BFILE_LOCATOR_SIZE);

		// the byte count of everything after the count itself
		putLocatorUB2(locator+LOB_LOCATOR_LENGTH_OFFSET,
				(uint16_t)(BFILE_LOCATOR_SIZE-2));

		putLocatorUB2(locator+LOB_LOCATOR_VERSION_OFFSET,
						BFILE_LOCATOR_VERSION);
		locator[LOB_LOCATOR_TYPE_OFFSET]=LOB_LOCATOR_BFILE;
		locator[LOB_LOCATOR_KIND_OFFSET]=BFILE_LOCATOR_KIND;
		locator[LOB_LOCATOR_TYPE2_OFFSET]=BFILE_LOCATOR_TYPE2;

		// the directory alias and the file name, both empty - no
		// server side call hands either of them over
		putLocatorUB2(locator+BFILE_LOCATOR_DIRECTORY_OFFSET,0);
		putLocatorUB2(locator+BFILE_LOCATOR_FILENAME_OFFSET,0);

		putLocatorUB2(locator+BFILE_LOCATOR_ID_OFFSET,curid);
		putLocatorUB2(locator+BFILE_LOCATOR_ID_OFFSET+2,
						(uint16_t)column);
		bytestring::copy(locator+BFILE_LOCATOR_MAGIC_OFFSET,
					LOB_LOCATOR_MAGIC,
					LOB_LOCATOR_MAGIC_SIZE);
		putLocatorUB2(locator+BFILE_LOCATOR_GENERATION_OFFSET,
						lobpingeneration[curid]);

		return BFILE_LOCATOR_SIZE;
	}

	bytestring::zero(locator,LOB_LOCATOR_SIZE);

	byte_t	loctype=(wiretype==ORACLE_TYPE_CLOB)?
				LOB_LOCATOR_CLOB:LOB_LOCATOR_BLOB;

	putLocatorUB2(locator+LOB_LOCATOR_LENGTH_OFFSET,
			(uint16_t)(LOB_LOCATOR_SIZE-2));

	putLocatorUB2(locator+LOB_LOCATOR_VERSION_OFFSET,LOB_LOCATOR_VERSION);
	locator[LOB_LOCATOR_TYPE_OFFSET]=loctype;
	locator[LOB_LOCATOR_KIND_OFFSET]=LOB_LOCATOR_KIND;
	locator[LOB_LOCATOR_INIT_OFFSET]=LOB_LOCATOR_INIT;
	locator[LOB_LOCATOR_TYPE2_OFFSET]=loctype;
	putLocatorUB2(locator+LOB_LOCATOR_SEQUENCE_OFFSET,LOB_LOCATOR_SEQUENCE);

	// only a clob has a character set
	if (wiretype==ORACLE_TYPE_CLOB) {
		locator[LOB_LOCATOR_FLAGS_OFFSET]=LOB_LOCATOR_CHARSET_PRESENT;
		putLocatorUB2(locator+LOB_LOCATOR_CHARSET_OFFSET,
						(uint16_t)CHARSET_AL32UTF8);
	}

	putLocatorUB2(locator+LOB_LOCATOR_ID_OFFSET,curid);
	putLocatorUB2(locator+LOB_LOCATOR_ID_OFFSET+2,(uint16_t)column);
	bytestring::copy(locator+LOB_LOCATOR_MAGIC_OFFSET,
				LOB_LOCATOR_MAGIC,
				LOB_LOCATOR_MAGIC_SIZE);
	putLocatorUB2(locator+LOB_LOCATOR_GENERATION_OFFSET,
						lobpingeneration[curid]);

	// the tail a real server leaves behind
	putLocatorUB2(locator+LOB_LOCATOR_TRAILER_OFFSET,
			(uint16_t)(LOB_LOCATOR_TRAILER>>16));
	putLocatorUB2(locator+LOB_LOCATOR_TRAILER_OFFSET+2,
			(uint16_t)(LOB_LOCATOR_TRAILER&0xffff));

	return LOB_LOCATOR_SIZE;
}

// writes what a lob column carries in a row: the locator's length, the
// lob's length, the chunk size, the length again as a raw byte, and the
// locator itself.  a null lob is the length alone, as zero
void sqlrprotocol_oracle::putLobLocator(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t wiretype,
						bool null) {

	if (null) {
		debugWrite("null lob");
		writeLenPreInt(&reqpacket,(uint32_t)0);
		return;
	}

	// characters for a clob and bytes for a blob, which is what
	// getLobFieldLength() answers either way.  a bfile's length isn't
	// available without opening the file, and a real server sends 0 for
	// one
	uint64_t	loblength=0;
	if (wiretype!=ORACLE_TYPE_BFILE &&
		!cont->getLobFieldLength(cursor,column,&loblength)) {
		loblength=0;
	}

	byte_t		locator[LOB_LOCATOR_SIZE];
	uint32_t	locatorsize=
			buildLobLocator(cursor,column,wiretype,locator);

	writeLenPreInt(&reqpacket,locatorsize);
	putLenPreUB8(loblength);
	if (wiretype!=ORACLE_TYPE_BFILE) {
		writeLenPreInt(&reqpacket,(uint32_t)LOB_CHUNK_SIZE);
	}
	write(&reqpacket,(byte_t)locatorsize);
	write(&reqpacket,(const byte_t *)locator,(size_t)locatorsize);

	debugWrite("lob length: %lld",(long long)loblength);
	debugWrite("locator size: %d",locatorsize);
}

// holds the connection on the row a locator was just sent for.  the
// controller's lob calls only ever address a cursor's current row, so the
// row a locator points at has to stay current until the client is done
// reading it
void sqlrprotocol_oracle::pinLobRow(sqlrservercursor *cursor,
						uint32_t colcount) {
	uint16_t	curid=cont->getId(cursor);
	lobpinned[curid]=true;
	lobpincolcount[curid]=colcount;
	debugWrite("lob row pinned on cursor %d",curid);
}

// takes the pin off and catches the connection up.  a client closing a
// locator isn't visible on the wire, so this runs at the cursor's next
// fetch instead - by then the client has moved on from the row whatever it
// did with the locator
void sqlrprotocol_oracle::releaseLobPin(sqlrservercursor *cursor) {

	uint16_t	curid=cont->getId(cursor);
	if (!lobpinned[curid]) {
		return;
	}

	debugWrite("lob row released on cursor %d",curid);

	// close whatever the row left open, then step over it
	uint16_t	*ct=columntypes[curid];
	if (ct) {
		for (uint32_t i=0; i<lobpincolcount[curid]; i++) {
			if (isLobColumnType(ct[i])) {
				cont->closeLobField(cursor,i);
			}
		}
	}
	cont->nextRow(cursor);

	clearLobPin(curid);
}

// forgets a pin without stepping over the row, for the places the result
// set itself is going away
void sqlrprotocol_oracle::clearLobPin(uint16_t curid) {
	lobpinned[curid]=false;
	lobpincolcount[curid]=0;
	lobpingeneration[curid]++;
}

static uint16_t getLocatorUB2(const byte_t *bytes) {
	return (uint16_t)((((uint16_t)bytes[0])<<8)|bytes[1]);
}

// reads a count prefixed 8 byte integer, the counterpart of putLenPreUB8().
// the base class only reads the 4 byte form, and a lob offset or amount
// doesn't fit in one
bool sqlrprotocol_oracle::readLenPreUB8(const byte_t *rp,
						const byte_t *end,
						uint64_t *value,
						const byte_t **rpout) {

	*value=0;

	if (end-rp<1) {
		*rpout=rp;
		return false;
	}

	const byte_t	*start=rp;
	byte_t		count;
	read(rp,&count,&rp);

	if (count>sizeof(uint64_t) || (size_t)(end-rp)<(size_t)count) {
		*rpout=start;
		return false;
	}

	for (byte_t i=0; i<count; i++) {
		byte_t	b;
		read(rp,&b,&rp);
		*value=((*value)<<8)|b;
	}
	*rpout=rp;
	return true;
}

// works out which cursor and column minted the locator a lob operation
// request quotes.  a locator only means anything while the row it came from
// is still pinned, and only if the pin is the one that minted it - the
// generation counter goes up every time a pin comes off, so a locator from a
// cursor that has since been fetched past, re-executed, or closed and reused
// doesn't decode, rather than being read against whatever row is there now
bool sqlrprotocol_oracle::decodeLobLocator(const byte_t *locator,
						uint32_t locatorsize,
						sqlrservercursor **cursor,
						uint32_t *column,
						uint16_t *wiretype) {

	*cursor=NULL;
	*column=0;
	*wiretype=0;

	if (!locator) {
		debugWrite("no locator");
		return false;
	}

	uint16_t	idoffset;
	uint16_t	magicoffset;
	uint16_t	generationoffset;
	if (locatorsize==LOB_LOCATOR_SIZE) {
		idoffset=LOB_LOCATOR_ID_OFFSET;
		magicoffset=LOB_LOCATOR_MAGIC_OFFSET;
		generationoffset=LOB_LOCATOR_GENERATION_OFFSET;
	} else if (locatorsize==BFILE_LOCATOR_SIZE) {
		idoffset=BFILE_LOCATOR_ID_OFFSET;
		magicoffset=BFILE_LOCATOR_MAGIC_OFFSET;
		generationoffset=BFILE_LOCATOR_GENERATION_OFFSET;
	} else {
		debugWrite("locator size %d isn't one this module mints",
								locatorsize);
		return false;
	}

	// a locator without the magic is one this module never handed out
	if (bytestring::compare(locator+magicoffset,
					LOB_LOCATOR_MAGIC,
					LOB_LOCATOR_MAGIC_SIZE)) {
		debugWrite("locator isn't this module's");
		return false;
	}

	uint16_t	curid=getLocatorUB2(locator+idoffset);
	uint32_t	col=getLocatorUB2(locator+idoffset+2);
	uint16_t	generation=getLocatorUB2(locator+generationoffset);

	debugWrite("locator cursor: %d",curid);
	debugWrite("locator column: %d",col);
	debugWrite("locator generation: %d",generation);

	if (curid>=maxcursorcount) {
		debugWrite("locator cursor out of range");
		return false;
	}
	if (!lobpinned[curid]) {
		debugWrite("cursor %d isn't holding a lob row",curid);
		return false;
	}
	if (generation!=lobpingeneration[curid]) {
		debugWrite("locator is from generation %d, cursor %d is on %d",
					generation,curid,lobpingeneration[curid]);
		return false;
	}
	if (col>=lobpincolcount[curid]) {
		debugWrite("locator column out of range");
		return false;
	}

	uint16_t	*ct=columntypes[curid];
	if (!ct || !isLobColumnType(ct[col])) {
		debugWrite("locator column isn't a lob");
		return false;
	}

	sqlrservercursor	*c=cont->getCursor(curid);
	if (!c) {
		debugWrite("cursor id %d not found",curid);
		return false;
	}

	*cursor=c;
	*column=col;
	*wiretype=getWireColumnType(ct[col]);
	return true;
}

// converts "chars" characters of utf-8 "in" to utf-16 big endian in "out",
// which has room for two bytes per character, and returns how many bytes it
// wrote.  a character above the basic multilingual plane, and a byte that
// doesn't start a valid sequence, go out as the lead byte's own value
// widened - two bytes per character has to hold either way, since the count
// is what the client works the character count back out from
uint32_t sqlrprotocol_oracle::putUtf16Chars(const char *in,
						uint64_t insize,
						uint64_t chars,
						byte_t *out) {

	const byte_t	*i=(const byte_t *)in;
	const byte_t	*end=i+insize;
	uint32_t	outsize=0;

	for (uint64_t c=0; c<chars; c++) {

		// a character count the buffer doesn't back up pads out
		// with nulls rather than running off the end
		if (i>=end) {
			out[outsize++]=0;
			out[outsize++]=0;
			continue;
		}

		const byte_t	*start=i;
		uint32_t	ch=*i;
		uint16_t	extra=0;
		if ((ch&0xe0)==0xc0) {
			ch=ch&0x1f;
			extra=1;
		} else if ((ch&0xf0)==0xe0) {
			ch=ch&0x0f;
			extra=2;
		} else if ((ch&0xf8)==0xf0) {
			ch=ch&0x07;
			extra=3;
		}
		i++;

		bool	valid=true;
		for (uint16_t e=0; e<extra; e++) {
			if (i>=end || ((*i)&0xc0)!=0x80) {
				valid=false;
				break;
			}
			ch=(ch<<6)|((*i)&0x3f);
			i++;
		}

		if (!valid || ch>0xffff) {
			i=start+1;
			ch=*start;
		}

		out[outsize++]=(byte_t)(ch>>8);
		out[outsize++]=(byte_t)(ch&0xff);
	}

	return outsize;
}

// reads and throws away one of the lob data packets a client sends behind
// a lob write request.  the layout is the one sendLobDataChunk() writes -
// a 64 byte descriptor whose second field is the byte count, and the bytes
// themselves riding after the packet's declared end
bool sqlrprotocol_oracle::discardLobDataPacket() {

	if (resppacketsize<LOB_DATA_DESCRIPTOR_SIZE) {
		debugWrite("truncated lob data descriptor: %d",resppacketsize);
		return false;
	}

	const byte_t	*rp=resppacket;
	uint32_t	flags=0;
	uint32_t	size=0;
	readBE(rp,&flags,&rp);
	readBE(rp,&size,&rp);

	debugStart("discarding lob data");
	debugWrite("bytes: %d",size);
	debugWrite("last: %s",(flags==LOB_DATA_LAST)?"true":"false");
	debugEnd();

	byte_t	discard[1024];
	while (size) {
		uint32_t	chunk=(size<(uint32_t)sizeof(discard))?
					size:(uint32_t)sizeof(discard);
		if (clientsock->read(discard,(size_t)chunk)!=(ssize_t)chunk) {
			debugWrite("read lob data failed");
			debugSystemError();
			return false;
		}
		size=size-chunk;
	}

	return true;
}

// the packet that says lob data follows - the ttc code and nothing else
bool sqlrprotocol_oracle::sendLobDataMarker() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_LOB_AND_BFILE_DATA;

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	debugStart("lob data marker");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugEnd();

	return sendPacket();
}

// one chunk of a lob read's answer.  the packet's own length counts the
// 64 byte descriptor alone - the lob bytes ride after the packet's declared
// end, which is what a real 12.2 server does and what the client reads
bool sqlrprotocol_oracle::sendLobDataChunk(const byte_t *data,
						uint32_t size,
						bool last) {

	resetSendPacketBuffer(PACKET_DATA_DESCRIPTOR);
	reqpacketflags=LOB_DATA_PACKET_FLAGS;

	// whether another chunk follows, this chunk's byte count, a constant,
	// the byte count again as a ub2, then padding out to 64 bytes
	writeBE(&reqpacket,(uint32_t)((last)?LOB_DATA_LAST:LOB_DATA_MORE));
	writeBE(&reqpacket,size);
	writeBE(&reqpacket,(uint32_t)LOB_DATA_CONSTANT);
	writeBE(&reqpacket,(uint16_t)size);
	for (uint16_t i=0; i<LOB_DATA_PADDING_SIZE; i++) {
		write(&reqpacket,(byte_t)0);
	}

	debugStart("lob data chunk");
	debugWrite("bytes: %d",size);
	debugWrite("last: %s",(last)?"true":"false");
	debugEnd();

	if (!sendPacket()) {
		return false;
	}

	if (size && clientsock->write(data,(size_t)size)!=(ssize_t)size) {
		debugWrite("write lob data failed");
		debugSystemError();
		return false;
	}

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

// a read of the pinned row's lob column, sent as the marker, then a chunk
// per packet, then the ordinary answer carrying how much came back
bool sqlrprotocol_oracle::sendLobReadResponse(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t wiretype,
						const byte_t *locator,
						uint32_t locatorsize,
						uint64_t offset,
						uint64_t amount) {

	// a clob's characters go out two bytes each and a blob's or a
	// bfile's one, so a chunk of the negotiated size carries half as
	// many characters of a clob as it does of a blob
	bool		clob=(wiretype==ORACLE_TYPE_CLOB);
	uint64_t	charsperchunk=(clob)?
				LOB_CHUNK_SIZE/LOB_CLOB_BYTES_PER_CHAR:
				LOB_CHUNK_SIZE;

	// the client counts from 1 where the controller counts from 0
	uint64_t	position=(offset)?offset-1:0;
	uint64_t	remaining=amount;
	uint64_t	charsread=0;

	debugStart("lob read");
	debugWrite("column: %d",column);
	debugColumnType(wiretype);
	debugWrite("offset: %lld",(long long)offset);
	debugWrite("amount: %lld",(long long)amount);
	debugEnd();

	// the chunk held back, so that the one before it can go out with the
	// right "more follows" flag - which isn't known until the read after
	// it comes back empty
	byte_t		chunk[LOB_CHUNK_SIZE];
	uint32_t	chunksize=0;
	bool		held=false;
	bool		markersent=false;

	for (;;) {

		// read the next chunk's worth
		uint64_t	charstoread=charsperchunk;
		if (remaining<charstoread) {
			charstoread=remaining;
		}
		uint64_t	got=0;
		if (charstoread &&
			!cont->getLobFieldSegment(cursor,column,
					lobbuffer,sizeof(lobbuffer),
					position,charstoread,&got)) {
			got=0;
		}

		// send the chunk held back, now that whether another follows
		// is known
		if (held) {
			if (!markersent) {
				if (!sendLobDataMarker()) {
					return false;
				}
				markersent=true;
			}
			if (!sendLobDataChunk(chunk,chunksize,!got)) {
				return false;
			}
			held=false;
		}

		if (!got) {
			break;
		}

		// hold this one back
		if (clob) {
			chunksize=putUtf16Chars(lobbuffer,
						sizeof(lobbuffer),got,chunk);
		} else {
			chunksize=(uint32_t)got;
			bytestring::copy(chunk,lobbuffer,(size_t)chunksize);
		}
		held=true;

		charsread=charsread+got;
		position=position+got;
		remaining=remaining-got;
	}

	debugWrite("lob read %lld",(long long)charsread);

	return sendLobOperationResponse(locator,locatorsize,
					LOB_RESULT_UB8,charsread);
}

// what every lob operation that worked gets back: the locator it quoted,
// the operation's result, and the same summary object the rest of the
// modern path ends a call with
bool sqlrprotocol_oracle::sendLobOperationResponse(const byte_t *locator,
						uint32_t locatorsize,
						byte_t resulttype,
						uint64_t result) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	// the locator goes back exactly as it came in, and with no length in
	// front of it - it carries its own
	write(&reqpacket,locator,(size_t)locatorsize);

	if (resulttype==LOB_RESULT_UB8) {
		putLenPreUB8(result);
	}

	debugStart("lob operation response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("locator size: %d",locatorsize);
	if (resulttype!=LOB_RESULT_NONE) {
		debugWrite("result: %lld",(long long)result);
	}
	debugEnd();

	if (query3session) {
		putSummary(0,0,0,NULL);
	} else {
		putError("",0,0);
		putGenericFooter();
	}

	return sendPacket(true);
}

// what a lob operation that can't be answered gets back.  unlike
// sendUnimplementedFunctionError(), the request has already been read in
// full by the time this runs, so the session carries on in step
bool sqlrprotocol_oracle::sendLobOperationError(uint32_t oranum,
						const char *message) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("lob operation error");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("error: %d",oranum);
	debugWrite("message: %s",message);
	debugEnd();

	if (query3session) {
		putSummary(0,oranum,0,message);
	} else {
		putError(message,oranum);
		putGenericFooter();
	}

	return sendPacket(true);
}

// the call a client makes to work with a lob it was handed a locator for.
// the read side operations are answered for real; the write side ones are
// parsed in full and then refused, so the stream stays in step either way -
// falling through to sendUnimplementedFunctionError() wouldn't read the
// request body at all, and the session would desync on the next call
// see "Oracle Wire Protocol - Lob Operations"
bool sqlrprotocol_oracle::lobOperations(const byte_t *rp) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t		sequence=0;
	byte_t		sourcepresent=0;
	uint32_t	sourcesize=0;
	byte_t		destpresent=0;
	uint32_t	destsize=0;
	uint32_t	shortamount=0;
	byte_t		opflags[4];
	uint32_t	operation=0;
	byte_t		scnpresent=0;
	uint32_t	scn=0;
	uint64_t	sourceoffset=0;
	uint32_t	amountkind=0;
	byte_t		amountpresent=0;
	uint64_t	amount=0;

	// the request's fixed part: the sequence number, a length for each
	// locator that follows, four flag bytes - the last of which is set
	// for a file exists - and then the operation itself
	if (end-rp<1) {
		debugWrite("truncated lob operation request");
		return false;
	}
	read(rp,&sequence,&rp);
	if (end-rp<1) {
		debugWrite("truncated lob operation request");
		return false;
	}
	read(rp,&sourcepresent,&rp);
	if (!readLenPreInt(rp,end,&sourcesize,&rp)) {
		debugWrite("truncated source locator size");
		return false;
	}
	if (end-rp<1) {
		debugWrite("truncated lob operation request");
		return false;
	}
	read(rp,&destpresent,&rp);
	if (!readLenPreInt(rp,end,&destsize,&rp) ||
		!readLenPreInt(rp,end,&shortamount,&rp)) {
		debugWrite("truncated destination locator size");
		return false;
	}
	if (end-rp<(ssize_t)sizeof(opflags)) {
		debugWrite("truncated lob operation flags");
		return false;
	}
	for (uint16_t i=0; i<sizeof(opflags); i++) {
		read(rp,&(opflags[i]),&rp);
	}
	if (!readLenPreInt(rp,end,&operation,&rp)) {
		debugWrite("truncated lob operation code");
		return false;
	}

	// the scn, the offset to work from, what the amount counts, and
	// whether an amount follows the locators at all
	if (end-rp<1) {
		debugWrite("truncated lob operation request");
		return false;
	}
	read(rp,&scnpresent,&rp);
	if (!readLenPreInt(rp,end,&scn,&rp) ||
		!readLenPreUB8(rp,end,&sourceoffset,&rp) ||
		!readLenPreInt(rp,end,&amountkind,&rp)) {
		debugWrite("truncated lob operation offset");
		return false;
	}
	if (end-rp<1) {
		debugWrite("truncated lob operation request");
		return false;
	}
	read(rp,&amountpresent,&rp);

	// three zero ub2s that a real client always sends and a real server
	// appears to ignore
	if (end-rp<6) {
		debugWrite("truncated lob operation request");
		return false;
	}
	rp=rp+6;

	// the locators themselves, echoed back exactly as this module minted
	// them
	const byte_t	*sourcelocator=NULL;
	if (sourcepresent && sourcesize) {
		if ((uint32_t)(end-rp)<sourcesize) {
			debugWrite("truncated source locator");
			return false;
		}
		sourcelocator=rp;
		rp=rp+sourcesize;
	}
	if (destpresent && destsize) {
		if ((uint32_t)(end-rp)<destsize) {
			debugWrite("truncated destination locator");
			return false;
		}
		rp=rp+destsize;
	}

	if (amountpresent && !readLenPreUB8(rp,end,&amount,&rp)) {
		debugWrite("truncated lob operation amount");
		return false;
	}

	// the summary object has to echo this back
	callnumber=sequence;

	if (getDebug()) {
		debugStart("lob operation request");
		debugWrite("sequence: %d",sequence);
		debugWrite("operation: 0x%05x",operation);
		debugWrite("source locator size: %d",
					(sourcelocator)?sourcesize:0);
		debugWrite("destination locator size: %d",
					(destpresent)?destsize:0);
		debugWrite("short amount: %d",shortamount);
		debugWrite("flags: 0x%02x 0x%02x 0x%02x 0x%02x",
					opflags[0],opflags[1],
					opflags[2],opflags[3]);
		debugWrite("scn present: %d",scnpresent);
		debugWrite("scn: %d",scn);
		debugWrite("source offset: %lld",(long long)sourceoffset);
		debugWrite("amount kind: %d",amountkind);
		debugWrite("amount: %lld",
					(long long)((amountpresent)?amount:0));
		debugEnd();
	}

	// only the read side is implemented.  a write, a temporary lob, a
	// trim or an erase gets a real oracle error rather than a wrong
	// answer, and so does a file's name, which no server side call hands
	// over
	if (operation!=LOB_OP_GET_LENGTH &&
		operation!=LOB_OP_READ &&
		operation!=LOB_OP_FILE_EXISTS &&
		operation!=LOB_OP_OPEN &&
		operation!=LOB_OP_FILE_OPEN &&
		operation!=LOB_OP_CLOSE &&
		operation!=LOB_OP_FILE_CLOSE) {
		debugWrite("lob operation 0x%05x not implemented",operation);
		return sendLobOperationError(ORA_UNIMPLEMENTED_FEATURE,
					ORA_UNIMPLEMENTED_FEATURE_MESSAGE);
	}

	// the locator says which cursor and column to work on, and whether
	// the row it came from is still there
	sqlrservercursor	*cursor=NULL;
	uint32_t		column=0;
	uint16_t		wiretype=0;
	if (!decodeLobLocator(sourcelocator,sourcesize,
					&cursor,&column,&wiretype)) {
		return sendLobOperationError(ORA_INVALID_LOB_LOCATOR,
					ORA_INVALID_LOB_LOCATOR_MESSAGE);
	}

	// the length, in characters for a clob and bytes for a blob, which
	// is what getLobFieldLength() answers either way
	if (operation==LOB_OP_GET_LENGTH) {
		uint64_t	loblength=0;
		if (!cont->getLobFieldLength(cursor,column,&loblength)) {
			loblength=0;
		}
		return sendLobOperationResponse(sourcelocator,sourcesize,
						LOB_RESULT_UB8,loblength);
	}

	if (operation==LOB_OP_READ) {
		return sendLobReadResponse(cursor,column,wiretype,
						sourcelocator,sourcesize,
						sourceoffset,
						(amountpresent)?amount:0);
	}

	// no server side call opens a bfile and asks whether the file is
	// there, so the length stands in for it: a bfile whose file isn't
	// there has none.  getLobFieldLength() itself answers a bfile
	// column either way, so its success alone says nothing
	if (operation==LOB_OP_FILE_EXISTS) {
		uint64_t	loblength=0;
		uint64_t	exists=(cont->getLobFieldLength(cursor,column,
						&loblength) && loblength)?1:0;
		return sendLobOperationResponse(sourcelocator,sourcesize,
						LOB_RESULT_UB8,exists);
	}

	// opening a lob is a client side state change - there's nothing to
	// do here but agree to it, and hand back the mode it asked for
	if (operation==LOB_OP_OPEN || operation==LOB_OP_FILE_OPEN) {
		return sendLobOperationResponse(sourcelocator,sourcesize,
					LOB_RESULT_UB8,
					(amountpresent)?amount:1);
	}

	// an explicit close of one locator.  the pin stays on - the row can
	// have handed out more than one locator, and the others are still
	// good - and comes off at the cursor's next fetch, the way it would
	// have without this call.  see releaseLobPin()
	cont->closeLobField(cursor,column);
	return sendLobOperationResponse(sourcelocator,sourcesize,
						LOB_RESULT_NONE,0);
}

void sqlrprotocol_oracle::putReturnParameters() {

	// nothing in this block is load bearing - a live 11.2 server puts 19
	// session state key/value pairs here and a live 12.2 server puts none
	write(&reqpacket,(byte_t)TTC_OK);

	writeLenPreInt(&reqpacket,AL8O4_COUNT);
	for (uint16_t i=0; i<AL8O4_COUNT; i++) {
		writeLenPreInt(&reqpacket,0);
	}

	// al8txl size, session-state key/value pair count, registration id
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);

	debugStart("return parameters");
	debugWrite("count: %d",(uint32_t)AL8O4_COUNT);
	for (uint16_t i=0; i<AL8O4_COUNT; i++) {
		debugWrite("param %d: 0",i);
	}
	debugEnd();
}

void sqlrprotocol_oracle::putSummaryExtension(uint32_t oranum,
						uint32_t rowcount) {

	// the two fields a summary object grew at oracle 12.1: the error
	// number again, as a ub4, and a ub8 row count, immediately before the
	// message.  a client picks its reader from the negotiated field
	// version - ojdbc 23.26 uses T4CTTIoer19 from 12.1 up and an older
	// reader below it, and python-oracledb reads both fields
	// unconditionally, in _process_error_info() in
	// impl/thin/messages/base.pyx, which is the same fact as its refusal
	// to talk to a server older than 12.1.  told the wrong shape a client
	// does not recover: ojdbc reads the next message two bytes out and
	// reports ORA-17401, python-oracledb and node-oracledb block on the
	// socket forever.  so this owes every ttc 0x04 the module writes: both
	// authentication trailers, the error packet and putSummary().
	//
	// checked against both live servers, which differ here and nowhere
	// else in the object: 11.2 ends at the message, and 12.2 puts
	// 02 05 7b 01 01 in front of it for an ORA-01403 after one row, and
	// 02 03 f9 00 for an ORA-01017.
	debugStart("summary extension");
	if (fieldversion<CCAP_FIELD_VERSION_12_1) {
		debugWrite("field version below 12.1, omitted");
		debugEnd();
		return;
	}

	writeLenPreInt(&reqpacket,oranum);
	writeLenPreInt(&reqpacket,rowcount);

	debugWrite("error: %d",oranum);
	debugWrite("row count: %d",rowcount);
	debugEnd();
}

void sqlrprotocol_oracle::putSummary(uint32_t cursorid,
						uint32_t oranum,
						uint32_t rowcount,
						const char *message) {
	// message is only read when oranum is set, so it's only safe to
	// measure then - the other caller passes NULL along with oranum 0
	putSummary(cursorid,oranum,rowcount,message,
			(oranum)?charstring::getLength(message):0);
}

void sqlrprotocol_oracle::putSummary(uint32_t cursorid,
						uint32_t oranum,
						uint32_t rowcount,
						const char *message,
						uint32_t messagesize) {

	// the same field sequence sendAuthenticationError() emits - captured
	// whole from a live 11.2 server - written out one field at a time.
	// the end of call status and the ecid sequence at the front are the
	// two fields CCAP_TTC1 and CCAP_OCI1 bit 0x01 promise, and the module
	// has to send them because it sets those bits
	write(&reqpacket,(byte_t)TTC_ERROR);

	writeLenPreInt(&reqpacket,1);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,rowcount);
	writeLenPreInt(&reqpacket,oranum);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,cursorid);
	writeLenPreInt(&reqpacket,0);
	write(&reqpacket,(byte_t)3);
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,(byte_t)0);

	// rowid: a ub4, a ub2, one raw byte, a ub4 and a ub2
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	write(&reqpacket,(byte_t)0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);

	writeLenPreInt(&reqpacket,0);
	write(&reqpacket,(byte_t)0);

	// the sequence number of the request being answered - a client matches
	// the answer to the call it made with it
	write(&reqpacket,callnumber);

	writeLenPreInt(&reqpacket,0);

	// success iterations, which is one execution rather than one row - a
	// live server sends 1 for a fetch of 10 rows and for a fetch of none
	writeLenPreInt(&reqpacket,1);

	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);

	putSummaryExtension(oranum,rowcount);

	if (oranum) {
		putLenBytes(message,messagesize);
	}

	debugStart("summary");
	debugWrite("cursor id: %d",cursorid);
	debugWrite("call number: %d",callnumber);
	debugWrite("row count: %d",rowcount);
	debugWrite("error: %d",oranum);
	if (oranum) {
		debugWrite("message: %.*s",(int)messagesize,message);
	}
	debugEnd();
}

void sqlrprotocol_oracle::putNumberField(const char *field,
						uint32_t fieldsize) {

	debugStart("number field");
	debugWrite("input: %.*s",(int)fieldsize,field);

	// oracle's number format: an exponent byte, then up to 20 base 100
	// mantissa digits.  a positive number's exponent byte is 193+e and its
	// digits are each digit+1; a negative number's is 62-e, its digits are
	// each 101-digit, and a 0x66 terminator follows unless the mantissa
	// fills 20 bytes.  zero is a single 0x80.  1 is c1 02, -7 is 3e 5e 66
	// and 12345.678 is c3 02 18 2e 44 51.
	//
	// the sign, the digits, and how many of them are in front of the
	// decimal point once any exponent has been applied
	bool		negative=false;
	byte_t		digits[MAX_NUMBER_DIGITS];
	uint16_t	digitcount=0;
	int32_t		point=0;
	bool		afterpoint=false;
	bool		inexponent=false;
	bool		exponentnegative=false;
	int32_t		exponent=0;

	for (uint32_t i=0; i<fieldsize; i++) {
		char	c=field[i];
		if (inexponent) {
			if (c=='-') {
				exponentnegative=true;
			} else if (c>='0' && c<='9') {
				exponent=exponent*10+(c-'0');
			}
		} else if (c=='-') {
			negative=true;
		} else if (c=='.') {
			afterpoint=true;
		} else if (c=='e' || c=='E') {
			inexponent=true;
		} else if (c>='0' && c<='9') {
			if (digitcount<MAX_NUMBER_DIGITS) {
				digits[digitcount++]=(byte_t)(c-'0');
			}
			if (!afterpoint) {
				point++;
			}
		}
	}
	point+=(exponentnegative)?-exponent:exponent;

	// leading zeros move the point rather than the value, trailing ones
	// aren't sent at all
	uint16_t	first=0;
	while (first<digitcount && !digits[first]) {
		first++;
		point--;
	}
	while (digitcount>first && !digits[digitcount-1]) {
		digitcount--;
	}

	if (first==digitcount) {
		debugWrite("zero");
		write(&reqpacket,(byte_t)1);
		write(&reqpacket,(byte_t)0x80);
		debugEnd();
		return;
	}

	// the base 100 digits straddle the decimal point, so the count of
	// digits in front of it has to be even, and so does the whole run
	byte_t		padded[MAX_NUMBER_DIGITS+2];
	uint16_t	paddedcount=0;
	if (point%2) {
		padded[paddedcount++]=0;
		point++;
	}
	for (uint16_t i=first; i<digitcount; i++) {
		padded[paddedcount++]=digits[i];
	}
	if (paddedcount%2) {
		padded[paddedcount++]=0;
	}

	int32_t	e=point/2-1;
	if (e<MIN_NUMBER_EXPONENT || e>MAX_NUMBER_EXPONENT) {
		debugWrite("number out of range: %.*s",(int)fieldsize,field);
		write(&reqpacket,(byte_t)1);
		write(&reqpacket,(byte_t)0x80);
		debugEnd();
		return;
	}

	byte_t		out[MAX_NUMBER_SIZE];
	uint16_t	outcount=0;
	uint16_t	mantissacount=0;

	out[outcount++]=(byte_t)((negative)?62-e:193+e);

	for (uint16_t i=0; i<paddedcount && mantissacount<MAX_NUMBER_MANTISSA;
								i+=2) {
		byte_t	d=(byte_t)(padded[i]*10+padded[i+1]);
		out[outcount++]=(byte_t)((negative)?101-d:d+1);
		mantissacount++;
	}

	if (negative && mantissacount<MAX_NUMBER_MANTISSA) {
		out[outcount++]=0x66;
	}

	debugHexDump(out,outcount);
	debugEnd();

	putLenBytes((const char *)out,outcount);
}

// the inverse of putNumberField() - oracle's internal number format back to
// decimal text.  a bound number goes to the database as that text, which the
// database implicitly converts, rather than as an oracle number the server
// side has no bind type for
bool sqlrprotocol_oracle::getNumberField(const byte_t *bytes,
						uint32_t size,
						char *out,
						uint32_t outsize,
						uint32_t *outlen) {

	*outlen=0;

	if (!bytes || !size || outsize<2) {
		return false;
	}

	// zero is a single 0x80
	if (size==1 && bytes[0]==0x80) {
		out[0]='0';
		out[1]='\0';
		*outlen=1;
		return true;
	}

	// the exponent byte carries the sign in its high bit
	bool	negative=!(bytes[0]&0x80);
	int32_t	e=(negative)?(62-(int32_t)bytes[0]):((int32_t)bytes[0]-193);
	if (e<MIN_NUMBER_EXPONENT || e>MAX_NUMBER_EXPONENT) {
		return false;
	}

	// each mantissa byte is one base 100 digit, so two decimal digits
	const int32_t	maxdigits=MAX_NUMBER_MANTISSA*2;
	char		digits[maxdigits];
	int32_t		digitcount=0;
	for (uint32_t i=1; i<size && digitcount+2<=maxdigits; i++) {
		int32_t	d=(int32_t)bytes[i];
		// a negative number ends in a 0x66 terminator, unless the
		// mantissa fills all 20 bytes
		if (negative && d==0x66) {
			break;
		}
		d=(negative)?(101-d):(d-1);
		if (d<0 || d>99) {
			return false;
		}
		digits[digitcount++]=(char)('0'+d/10);
		digits[digitcount++]=(char)('0'+d%10);
	}
	if (!digitcount) {
		return false;
	}

	// where the decimal point falls in that run of digits
	int32_t	point=2*(e+1);

	// a leading zero moves the point rather than the value, and a
	// trailing one past the point isn't part of the value at all - both
	// come from the base 100 padding putNumberField() adds
	int32_t	first=0;
	while (first+1<digitcount && digits[first]=='0' && point>1) {
		first++;
		point--;
	}
	while (digitcount>first+1 && digitcount-first>point &&
					digits[digitcount-1]=='0') {
		digitcount--;
	}
	int32_t	dcount=digitcount-first;

	// how much room the text needs, sign and terminator included
	uint32_t	needed=(negative)?2:1;
	if (point<=0) {
		needed+=(uint32_t)(2-point+dcount);
	} else if (point>=dcount) {
		needed+=(uint32_t)point;
	} else {
		needed+=(uint32_t)(dcount+1);
	}
	if (needed>outsize) {
		return false;
	}

	uint32_t	len=0;
	if (negative) {
		out[len++]='-';
	}
	if (point<=0) {
		out[len++]='0';
		out[len++]='.';
		for (int32_t i=0; i<-point; i++) {
			out[len++]='0';
		}
		for (int32_t i=0; i<dcount; i++) {
			out[len++]=digits[first+i];
		}
	} else if (point>=dcount) {
		for (int32_t i=0; i<dcount; i++) {
			out[len++]=digits[first+i];
		}
		for (int32_t i=dcount; i<point; i++) {
			out[len++]='0';
		}
	} else {
		for (int32_t i=0; i<point; i++) {
			out[len++]=digits[first+i];
		}
		out[len++]='.';
		for (int32_t i=point; i<dcount; i++) {
			out[len++]=digits[first+i];
		}
	}
	out[len]='\0';

	*outlen=len;

	return true;
}

int16_t sqlrprotocol_oracle::getRowidDigit(char c) {
	if (c>='A' && c<='Z') {
		return (int16_t)(c-'A');
	}
	if (c>='a' && c<='z') {
		return (int16_t)(c-'a'+26);
	}
	if (c>='0' && c<='9') {
		return (int16_t)(c-'0'+52);
	}
	if (c=='+') {
		return 62;
	}
	if (c=='/') {
		return 63;
	}
	return -1;
}

bool sqlrprotocol_oracle::putRowidField(const char *field,
						uint64_t fieldsize) {

	debugStart("rowid field");
	debugWrite("input: %.*s",(int)fieldsize,field);

	// the backend hands a rowid over in the 18 character base 64 form
	// oracle prints it as, and the wire wants the four numbers packed
	// into it back: a constant length byte, then the data object number
	// and the relative file number, a zero byte, and the block number
	// and the row number, each a count prefixed integer.  captured from
	// a live 12.2 server, which sends the same bytes whether the client
	// defined the column a rowid or a string:
	//
	//	AAScpAAAFAAAAQvAAh ->
	//		0e 03 49 ca 40 01 05 00 02 04 2f 01 21
	//
	// the length byte is 0x0e in every capture, whatever the numbers
	// after it come to, so it is a constant rather than a count of them.
	// the zero byte is one the thin drivers skip without reading
	if (fieldsize!=ORACLE_ROWID_TEXT_SIZE) {
		debugWrite("not %d characters",
				(uint32_t)ORACLE_ROWID_TEXT_SIZE);
		debugEnd();
		return false;
	}

	static const uint16_t	partdigits[ORACLE_ROWID_PARTS]={
					ORACLE_ROWID_OBJECT_DIGITS,
					ORACLE_ROWID_FILE_DIGITS,
					ORACLE_ROWID_BLOCK_DIGITS,
					ORACLE_ROWID_ROW_DIGITS};

	// decode, most significant digit first
	uint32_t	parts[ORACLE_ROWID_PARTS];
	uint16_t	pos=0;
	for (uint16_t i=0; i<ORACLE_ROWID_PARTS; i++) {
		parts[i]=0;
		for (uint16_t j=0; j<partdigits[i]; j++) {
			int16_t	digit=getRowidDigit(field[pos]);
			if (digit<0) {
				debugWrite("bad character at %d",
							(uint32_t)pos);
				debugEnd();
				return false;
			}
			parts[i]=(parts[i]<<6)+(uint32_t)digit;
			pos++;
		}
	}

	write(&reqpacket,(byte_t)ORACLE_ROWID_LENGTH_BYTE);
	writeLenPreInt(&reqpacket,parts[0]);
	writeLenPreInt(&reqpacket,parts[1]);
	write(&reqpacket,(byte_t)0);
	writeLenPreInt(&reqpacket,parts[2]);
	writeLenPreInt(&reqpacket,parts[3]);

	debugWrite("object: %d",parts[0]);
	debugWrite("file: %d",parts[1]);
	debugWrite("block: %d",parts[2]);
	debugWrite("row: %d",parts[3]);
	debugEnd();

	return true;
}

int16_t sqlrprotocol_oracle::getRawDigit(char c) {
	if (c>='0' && c<='9') {
		return (int16_t)(c-'0');
	}
	if (c>='A' && c<='F') {
		return (int16_t)(c-'A'+10);
	}
	if (c>='a' && c<='f') {
		return (int16_t)(c-'a'+10);
	}
	return -1;
}

bool sqlrprotocol_oracle::putRawField(const char *field,
						uint64_t fieldsize,
						bool longraw) {

	debugStart("raw field");
	debugWrite("input: %.*s",(int)fieldsize,field);

	// the backend hands a raw or a long raw over in the two characters
	// per byte hexadecimal form oracle prints it as, and the wire wants
	// the bytes themselves back.  a raw goes out as a plain length
	// prefixed blob, the same as a string - a live 12.2 server answers
	// a raw(20) holding 0102030405 with
	//
	//	05 01 02 03 04 05
	//
	// and a long raw goes out as the long form of one - see
	// putLongBytes()
	if (fieldsize%ORACLE_RAW_HEX_PER_BYTE) {
		debugWrite("odd number of characters");
		debugEnd();
		return false;
	}

	uint32_t	bytecount=(uint32_t)
				(fieldsize/ORACLE_RAW_HEX_PER_BYTE);
	byte_t		*bytes=new byte_t[bytecount+1];

	// decode, high order character of each byte first
	for (uint32_t i=0; i<bytecount; i++) {
		int16_t	high=getRawDigit(field[i*ORACLE_RAW_HEX_PER_BYTE]);
		int16_t	low=getRawDigit(field[i*ORACLE_RAW_HEX_PER_BYTE+1]);
		if (high<0 || low<0) {
			debugWrite("bad character at %d",
					i*ORACLE_RAW_HEX_PER_BYTE);
			debugEnd();
			delete[] bytes;
			return false;
		}
		bytes[i]=(byte_t)((high<<4)+low);
	}

	if (longraw) {
		putLongBytes((const char *)bytes,bytecount);
	} else {
		putLenBytes((const char *)bytes,bytecount);
	}

	debugWrite("bytes: %d",bytecount);
	debugHexDump(bytes,bytecount);
	debugEnd();

	delete[] bytes;

	return true;
}

void sqlrprotocol_oracle::putNullLongField() {

	// a null long or long raw isn't the single zero byte every other
	// type's null is - a live 12.2 server sends five more bytes behind
	// it, the last three of them the count prefixed 1405 of ORA-01405,
	// "fetched column value is NULL".  the same six bytes for a long as
	// for a long raw, and whatever column follows in the row:
	//
	//	null long raw -> 00 81 01 02 05 7d
	//
	// a client given the plain zero byte instead reads the rest of the
	// row one field short and answers with a marker packet
	static const byte_t	nulllong[]={0x00,0x81,0x01,0x02,0x05,0x7d};
	write(&reqpacket,(const char *)nulllong,sizeof(nulllong));
}

void sqlrprotocol_oracle::putLongBytes(const char *bytes, uint32_t size) {

	// putLenBytes()' long form, taken whatever the size, and with two
	// more zero bytes past the empty chunk that closes it.  a live 12.2
	// server sends a long or a long raw this way even when it is short
	// enough for the plain form, and sends those two bytes for a long
	// column and for nothing else.
	//
	// each chunk's length goes out as a raw byte, the way putLenBytes()
	// writes one.  a live server's answer to a long raw holding
	// 0a0b0c0d0e reads as though the length were count prefixed -
	//
	//	fe 01 05 0a 0b 0c 0d 0e 00 00 00
	//
	// - but sending that shape puts OCI a byte out: it takes the 01 for
	// a one byte chunk and the 0a for the next chunk's length, reads 11
	// bytes where there are 5, and answers with a marker packet
	write(&reqpacket,(byte_t)CLR_LONG_FORM_MARKER);
	uint32_t	offset=0;
	while (offset<size) {
		uint32_t	chunk=size-offset;
		if (chunk>CLR_MAX_CHUNK_SIZE) {
			chunk=CLR_MAX_CHUNK_SIZE;
		}
		write(&reqpacket,(byte_t)chunk);
		write(&reqpacket,bytes+offset,(size_t)chunk);
		offset+=chunk;
	}
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,(byte_t)0);
	write(&reqpacket,(byte_t)0);
}

bool sqlrprotocol_oracle::getIntervalNumber(const char **f,
						const char *end,
						char separator,
						uint32_t *value) {

	// reads the digits at *f, then the separator that has to end them -
	// or, for a separator of 0, the end of the text.  a value in some
	// other shape fails here rather than half way through
	*value=0;

	uint16_t	digits=0;
	while (*f<end && character::isDigit(**f)) {
		if (*value>MAX_INTERVAL_LEADING/10) {
			return false;
		}
		*value=(*value)*10+(uint32_t)(**f-'0');
		digits++;
		(*f)++;
	}
	if (!digits) {
		return false;
	}

	if (!separator) {
		return (*f==end);
	}
	if (*f==end || **f!=separator) {
		return false;
	}
	(*f)++;
	return true;
}

void sqlrprotocol_oracle::putIntervalLeading(byte_t *out,
						uint32_t value,
						bool negative) {

	// the years, the days and the nanoseconds each go out over 4 bytes,
	// most significant first, biased by 2^31
	uint32_t	biased=(negative)?
				(ORACLE_INTERVAL_LEADING_BIAS-value):
				(ORACLE_INTERVAL_LEADING_BIAS+value);
	out[0]=(byte_t)((biased>>24)&0xff);
	out[1]=(byte_t)((biased>>16)&0xff);
	out[2]=(byte_t)((biased>>8)&0xff);
	out[3]=(byte_t)(biased&0xff);
}

bool sqlrprotocol_oracle::putIntervalField(const char *field,
						uint64_t fieldsize,
						bool daytosecond) {

	debugStart("interval field");
	debugWrite("input: %.*s",(int)fieldsize,field);

	// the backend hands an interval over in the text form oracle prints
	// it as - a sign, then the leading field, then the smaller ones -
	// and the wire wants the fixed width binary form behind a length
	// byte.  captured from a live 12.2 server, which puts the same
	// bytes on the wire whether the value came out of a declared
	// interval column or out of to_yminterval()/to_dsinterval():
	//
	//	"+01-02"		-> 05 80 00 00 01 3e
	//	"-1234-11"		-> 05 7f ff fb 2e 31
	//	"+03 04:05:06.777777"	-> 0b 80 00 00 03 40 41 42
	//					ae 5b ef 68
	//	"-1234 23:59:58.123456789"
	//				-> 0b 7f ff fb 2e 25 01 02
	//					78 a4 32 eb
	//
	// so a year to month is the years biased by 2^31 over 4 bytes and
	// the months biased by 60 over 1, and a day to second is the days
	// the same way, then the hours, the minutes and the seconds biased
	// by 60 over a byte each, then the nanoseconds biased by 2^31 over
	// 4.  a negative interval carries one sign in the text and every
	// field of it goes out negative.
	//
	// the text pads the leading field out to the column's declared
	// precision ("+01-02" for an interval year to month, "-1234-11" for
	// an interval year(4) to month) and the fraction out to the
	// seconds', so the fields are read as digits up to the separator
	// rather than at fixed offsets
	const char	*f=field;
	const char	*end=field+fieldsize;

	bool	negative=false;
	if (f<end && (*f=='+' || *f=='-')) {
		negative=(*f=='-');
		f++;
	}

	byte_t		out[ORACLE_INTERVALDS_SIZE];
	uint32_t	outsize=(daytosecond)?
				ORACLE_INTERVALDS_SIZE:
				ORACLE_INTERVALYM_SIZE;
	bytestring::zero(out,sizeof(out));

	uint32_t	leading=0;
	if (!getIntervalNumber(&f,end,(daytosecond)?' ':'-',&leading)) {
		debugWrite("bad leading field");
		debugEnd();
		return false;
	}
	putIntervalLeading(out,leading,negative);

	if (!daytosecond) {

		uint32_t	months=0;
		if (!getIntervalNumber(&f,end,0,&months)) {
			debugWrite("bad months");
			debugEnd();
			return false;
		}
		out[4]=(byte_t)((negative)?
				(ORACLE_INTERVAL_FIELD_BIAS-months):
				(ORACLE_INTERVAL_FIELD_BIAS+months));

		debugWrite("years: %s%d",(negative)?"-":"",leading);
		debugWrite("months: %s%d",(negative)?"-":"",months);

	} else {

		uint32_t	hours=0;
		uint32_t	minutes=0;
		uint32_t	seconds=0;
		if (!getIntervalNumber(&f,end,':',&hours) ||
			!getIntervalNumber(&f,end,':',&minutes)) {
			debugWrite("bad time fields");
			debugEnd();
			return false;
		}

		// the seconds run either to a decimal point or to the end
		uint16_t	digits=0;
		while (f<end && character::isDigit(*f)) {
			seconds=seconds*10+(uint32_t)(*f-'0');
			digits++;
			f++;
		}
		if (!digits) {
			debugWrite("bad seconds");
			debugEnd();
			return false;
		}

		// the fraction is however many digits the column's seconds
		// precision calls for, and the wire always wants
		// nanoseconds, so it's scaled up to 9 digits
		uint32_t	nanoseconds=0;
		if (f<end && *f=='.') {
			f++;
			digits=0;
			while (f<end && character::isDigit(*f)) {
				if (digits<ORACLE_INTERVAL_FRACTION_DIGITS) {
					nanoseconds=nanoseconds*10+
						(uint32_t)(*f-'0');
					digits++;
				}
				f++;
			}
			while (digits<ORACLE_INTERVAL_FRACTION_DIGITS) {
				nanoseconds*=10;
				digits++;
			}
		}
		if (f!=end) {
			debugWrite("trailing characters");
			debugEnd();
			return false;
		}

		out[4]=(byte_t)((negative)?
				(ORACLE_INTERVAL_FIELD_BIAS-hours):
				(ORACLE_INTERVAL_FIELD_BIAS+hours));
		out[5]=(byte_t)((negative)?
				(ORACLE_INTERVAL_FIELD_BIAS-minutes):
				(ORACLE_INTERVAL_FIELD_BIAS+minutes));
		out[6]=(byte_t)((negative)?
				(ORACLE_INTERVAL_FIELD_BIAS-seconds):
				(ORACLE_INTERVAL_FIELD_BIAS+seconds));
		putIntervalLeading(out+7,nanoseconds,negative);

		debugWrite("days: %s%d",(negative)?"-":"",leading);
		debugWrite("hours: %s%d",(negative)?"-":"",hours);
		debugWrite("minutes: %s%d",(negative)?"-":"",minutes);
		debugWrite("seconds: %s%d",(negative)?"-":"",seconds);
		debugWrite("nanoseconds: %s%d",
				(negative)?"-":"",nanoseconds);
	}

	putLenBytes((const char *)out,outsize);

	debugHexDump(out,outsize);
	debugEnd();

	return true;
}

uint16_t sqlrprotocol_oracle::getTimestampDigits(const char **f,
						const char *end,
						uint32_t *value) {

	// reads the run of digits at *f, and answers how many there were so
	// the caller can tell an empty field from a zero one and can scale
	// a fraction out to nanoseconds
	*value=0;

	uint16_t	digits=0;
	while (*f<end && character::isDigit(**f) &&
			digits<ORACLE_TIMESTAMP_FRACTION_DIGITS) {
		*value=(*value)*10+(uint32_t)(**f-'0');
		digits++;
		(*f)++;
	}
	return digits;
}

bool sqlrprotocol_oracle::putTimestampField(const char *field,
						uint64_t fieldsize,
						bool withtimezone) {

	debugStart("timestamp field");
	debugWrite("input: %.*s",(int)fieldsize,field);

	// the backend hands a timestamp over in the text form oracle prints
	// one as - the session's date, then the time on a 12 hour clock,
	// then the fraction, then the half of the day, and then, for a
	// timestamp with time zone, the offset - and the wire wants the
	// fixed width binary form behind a length byte.  captured from a
	// live 12.2 server:
	//
	//	2004-04-04 04:04:04.444444
	//		-> 0b 78 68 04 04 05 05 05 1a 7d ad 60
	//	1899-12-31 23:59:58.000001
	//		-> 0b 76 c7 0c 1f 18 3c 3b 00 00 03 e8
	//	2004-04-04 12:00:00.5
	//		-> 0b 78 68 04 04 0d 01 01 1d cd 65 00
	//	2005-05-05 05:05:05.555555 -05:00
	//		-> 0d 78 69 05 05 06 06 06 21 1d 18 b8 4f 3c
	//	2005-05-05 05:05:05.555555 -09:30
	//		-> 0d 78 69 05 05 06 06 06 21 1d 18 b8 4b 1e
	//	2006-06-06 06:06:06.666666 +05:30
	//		-> 0d 78 6a 06 06 07 07 07 27 bc 84 10 59 5a
	//	2005-05-05 00:30:00 +00:00
	//		-> 0d 78 69 05 05 01 1f 01 00 00 00 00 54 3c
	//
	// so the first 7 bytes are a date's, biased the same ways, the next
	// 4 are the nanoseconds most significant byte first and unbiased,
	// and a timestamp with time zone adds the offset's hours biased by
	// 84 and its minutes biased by 60.  a negative offset carries one
	// sign in the text and both of its fields go out negative.  the
	// date and the time are the local ones, not the utc ones a
	// timestamp with time zone is stored as
	const char	*f=field;
	const char	*end=field+fieldsize;

	// the date runs to the first space, and datetime::parse() reads it
	// the same way getOracleDate() reads a date column, century
	// inference for a 2 digit year and all
	const char	*datestart=f;
	while (f<end && *f!=' ') {
		f++;
	}
	uint64_t	datesize=(uint64_t)(f-datestart);
	if (datesize>=MAX_TIMESTAMP_DATE_TEXT) {
		debugWrite("date too long");
		debugEnd();
		return false;
	}
	char	datebuffer[MAX_TIMESTAMP_DATE_TEXT];
	bytestring::copy(datebuffer,datestart,datesize);
	datebuffer[datesize]='\0';

	int16_t	year;
	int16_t	month;
	int16_t	day;
	int16_t	parsedhour;
	int16_t	parsedminute;
	int16_t	parsedsecond;
	int32_t	usec;
	bool	isnegative;
	if (!datetime::parse(datebuffer,false,false,"/-.:",true,
				&year,&month,&day,
				&parsedhour,&parsedminute,&parsedsecond,
				&usec,&isnegative)) {
		debugWrite("bad date");
		debugEnd();
		return false;
	}
	if (year==-1) {
		year=0;
	}
	if (month==-1) {
		month=1;
	}
	if (day==-1) {
		day=1;
	}

	uint32_t	hours=0;
	uint32_t	minutes=0;
	uint32_t	seconds=0;
	uint32_t	nanoseconds=0;

	if (f<end) {

		// the hours, minutes and seconds, in whatever the session's
		// time format separates them with - a dot by default, but a
		// colon in plenty of other formats
		f++;
		if (!getTimestampDigits(&f,end,&hours) ||
			f==end || (*f!='.' && *f!=':')) {
			debugWrite("bad hours");
			debugEnd();
			return false;
		}
		f++;
		if (!getTimestampDigits(&f,end,&minutes) ||
			f==end || (*f!='.' && *f!=':')) {
			debugWrite("bad minutes");
			debugEnd();
			return false;
		}
		f++;
		if (!getTimestampDigits(&f,end,&seconds)) {
			debugWrite("bad seconds");
			debugEnd();
			return false;
		}

		// the fraction is however many digits the column's seconds
		// precision calls for, and the wire always wants
		// nanoseconds, so it's scaled up to 9 digits
		if (f<end && (*f=='.' || *f==':')) {
			f++;
			uint16_t	digits=
				getTimestampDigits(&f,end,&nanoseconds);
			while (digits<ORACLE_TIMESTAMP_FRACTION_DIGITS) {
				nanoseconds*=10;
				digits++;
			}
		}

		// the half of the day, if the session's time format is a 12
		// hour one.  noon is 12 PM and midnight is 12 AM
		while (f<end && *f==' ') {
			f++;
		}
		if (f+1<end &&
			character::upper(f[1])=='M' &&
			(character::upper(f[0])=='A' ||
				character::upper(f[0])=='P')) {
			bool	pm=(character::upper(f[0])=='P');
			if (hours==12) {
				hours=(pm)?12:0;
			} else if (pm) {
				hours+=12;
			}
			f+=2;
		}
	}

	int16_t	tzhour=0;
	int16_t	tzminute=0;

	if (withtimezone) {

		while (f<end && *f==' ') {
			f++;
		}

		// oracle prints a fixed offset as a sign and an hours:minutes
		// pair, but a named region as its name - and a region only
		// goes on the wire as an id out of oracle's own time zone
		// table, which the module has no copy of
		bool	tznegative=false;
		if (f<end && (*f=='+' || *f=='-')) {
			tznegative=(*f=='-');
			f++;
		} else {
			debugWrite("time zone is not an offset");
			debugEnd();
			return false;
		}

		uint32_t	offsethours=0;
		uint32_t	offsetminutes=0;
		if (!getTimestampDigits(&f,end,&offsethours) ||
			f==end || *f!=':') {
			debugWrite("bad time zone hours");
			debugEnd();
			return false;
		}
		f++;
		if (!getTimestampDigits(&f,end,&offsetminutes)) {
			debugWrite("bad time zone minutes");
			debugEnd();
			return false;
		}

		tzhour=(int16_t)((tznegative)?
				-(int32_t)offsethours:(int32_t)offsethours);
		tzminute=(int16_t)((tznegative)?
				-(int32_t)offsetminutes:(int32_t)offsetminutes);
	}

	while (f<end && *f==' ') {
		f++;
	}
	if (f!=end) {
		debugWrite("trailing characters");
		debugEnd();
		return false;
	}

	byte_t		out[ORACLE_TIMESTAMPTZ_SIZE];
	uint32_t	outsize=(withtimezone)?
				ORACLE_TIMESTAMPTZ_SIZE:
				ORACLE_TIMESTAMP_SIZE;
	bytestring::zero(out,sizeof(out));

	putOracleDate(out,year,month,day,
			(int16_t)hours,(int16_t)minutes,(int16_t)seconds);

	out[7]=(byte_t)((nanoseconds>>24)&0xff);
	out[8]=(byte_t)((nanoseconds>>16)&0xff);
	out[9]=(byte_t)((nanoseconds>>8)&0xff);
	out[10]=(byte_t)(nanoseconds&0xff);

	debugWrite("nanoseconds: %d",nanoseconds);

	if (withtimezone) {
		out[11]=(byte_t)(tzhour+ORACLE_TZ_HOUR_BIAS);
		out[12]=(byte_t)(tzminute+ORACLE_TZ_MINUTE_BIAS);
		debugWrite("time zone: %d:%d",
				(int32_t)tzhour,(int32_t)tzminute);
	}

	putLenBytes((const char *)out,outsize);

	debugHexDump(out,outsize);
	debugEnd();

	return true;
}

bool sqlrprotocol_oracle::execute(const byte_t *rp) {

	// executes a statement query2() or query3() already parsed.  the two
	// paths share the function code and nothing else: a query3 session's
	// is the modern re-execute, in the lpi-encoded layout, and carries
	// the statement's bind values again
	if (query3session) {
		return reexecute(rp);
	}

	// parse the request...
	uint16_t	options;
	uint16_t	moreoptions;
	uint16_t	cursorid;

	// FIXME: decode this... see "Oracle Wire Protocol - Execute"
	readBE(rp,&options,&rp);
	readBE(rp,&moreoptions,&rp);
	readBE(rp,&cursorid,&rp);

	if (getDebug()) {
		debugStart("execute request");
		debugOptions(options,moreoptions);
		debugWrite("cursor id: %d",cursorid);
		debugEnd();
	}

	// the id on the wire is the controller's plus 1
	sqlrservercursor	*cursor=(cursorid)?
			cont->getCursor((uint16_t)(cursorid-1)):NULL;
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError(cursorid);
	}
	lastcursorid=cont->getId(cursor);

	// a fresh execute means a new result set - drop any row held
	// over from a previous one on this cursor
	pendingrow[cont->getId(cursor)].clear();

	// and any row it was pinning for a lob read
	clearLobPin(cont->getId(cursor));

	// execute the query
	if (!cont->executeQuery(cursor,true,true,true,true)) {
		debugWrite("execute query failed");
		return sendQueryError(cursor);
	}

	return sendExecuteResponse(cursor);
}

// the modern path's second and later executes of a statement whose binds
// changed: fresh values for the descriptors the statement was parsed with,
// and no query text, descriptors or defines of its own
// see "Oracle Wire Protocol - Execute"
bool sqlrprotocol_oracle::reexecute(const byte_t *rp) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t		sequence=0;
	uint32_t	cursorid=0;
	uint32_t	iterations=0;
	uint32_t	options=0;
	uint32_t	moreoptions=0;

	if (!getPointer(rp,end,&sequence,&rp) ||
		!readLenPreInt(rp,end,&cursorid,&rp) ||
		!readLenPreInt(rp,end,&iterations,&rp) ||
		!readLenPreInt(rp,end,&options,&rp) ||
		!readLenPreInt(rp,end,&moreoptions,&rp)) {
		debugWrite("truncated re-execute request");
		return false;
	}

	// the summary object has to echo this back
	callnumber=sequence;

	if (getDebug()) {
		debugStart("re-execute request");
		debugWrite("sequence: %d",sequence);
		debugWrite("cursor id: %d",cursorid);
		debugWrite("iterations: %d",iterations);
		debugWrite("options: 0x%08x",options);
		debugWrite("more options: 0x%08x",moreoptions);
		debugEnd();
	}

	// the id on the wire is the controller's plus 1
	sqlrservercursor	*cursor=(cursorid)?
			cont->getCursor((uint16_t)(cursorid-1)):NULL;
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError(cursorid);
	}
	lastcursorid=cont->getId(cursor);

	query3affectedrows=0;
	query3knowsaffectedrows=false;

	// only the values are on the wire
	restoreQuery3Binds(cursor);
	if (!getQuery3BindValues(rp,end,query3binddescs,iterations)) {
		return false;
	}

	// a fresh execute means a new result set - re-start the running row
	// count and drop any row held over from the previous one
	rowssent[cont->getId(cursor)]=0;
	pendingrow[cont->getId(cursor)].clear();

	// and any row it was pinning for a lob read
	clearLobPin(cont->getId(cursor));

	// one execution per row data block, as in query3()
	for (uint32_t block=0; block<query3blocks || !block; block++) {

		if (block<query3blocks &&
			!installQuery3Binds(cursor,block)) {
			return sendNotAllVariablesBoundError(cursorid);
		}

		if (!cont->executeQuery(cursor,true,true,true,true)) {
			debugWrite("execute query failed");
			return sendQueryError(cursor);
		}

		// a ref cursor bind's result set isn't readable until the
		// execute that opened it has run
		sqlrservercursor	*failed=NULL;
		if (!fetchFromRefCursors(cursor,&failed)) {
			return sendQueryError((failed)?failed:cursor);
		}

		if (cont->knowsAffectedRows(cursor)) {
			query3knowsaffectedrows=true;
			query3affectedrows+=(uint32_t)
					cont->getAffectedRows(cursor);
		}
	}

	return sendReexecuteResponse(cursor,cursorid);
}

// a re-execute answers with the summary object alone - the return
// parameters block belongs to a full execute
bool sqlrprotocol_oracle::sendReexecuteResponse(sqlrservercursor *cursor,
						uint32_t cursorid) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	// the out bind values, but no io vector - the client learned the
	// directions from the full execute that came before this
	if (hasQuery3OutBinds()) {
		putOutBindValues(cursor);
	}

	uint32_t	rowcount=(query3knowsaffectedrows)?query3affectedrows:0;

	putSummary(cursorid,0,rowcount,NULL);

	debugStart("re-execute response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("row count: %d",rowcount);
	debugEnd();

	return sendPacket(true);
}

bool sqlrprotocol_oracle::sendExecuteResponse(sqlrservercursor *cursor) {

	resetSendPacketBuffer(PACKET_DATA);

	// FIXME: decode this... see "Oracle Wire Protocol - Execute"

	uint16_t	dataflags=0;
	byte_t	ttccode=TTC_ERROR;
	byte_t	unknown[]={
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	reqpacket.append(unknown,sizeof(unknown));

	debugStart("execute response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugHexDump(unknown,sizeof(unknown));
	debugEnd();

	return sendPacket(true);
}

bool sqlrprotocol_oracle::fetch3(const byte_t *rp) {

	// the modern path fetch request body

	const byte_t	*end=resppacket+resppacketsize;

	byte_t		sequence=0;
	uint32_t	cursorid=0;
	uint32_t	rowstofetch=0;

	if (!getPointer(rp,end,&sequence,&rp) ||
		!readLenPreInt(rp,end,&cursorid,&rp) ||
		!readLenPreInt(rp,end,&rowstofetch,&rp)) {
		debugWrite("truncated fetch request");
		return false;
	}

	// the summary object has to echo this back
	callnumber=sequence;

	if (getDebug()) {
		debugStart("fetch request");
		debugWrite("sequence: %d",sequence);
		debugWrite("cursor id: %d",cursorid);
		debugWrite("rows to fetch: %d",rowstofetch);
		debugEnd();
	}

	if (!cursorid) {
		debugWrite("no cursor id");
		return sendCursorNotOpenError();
	}

	sqlrservercursor	*cursor=cont->getCursor((uint16_t)(cursorid-1));
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError(cursorid);
	}

	// whatever row the cursor was holding for a lob read, the client has
	// moved on from it - it's asking for the next one
	releaseLobPin(cursor);

	return sendFetch3Response(cursor,cursorid,rowstofetch);
}

bool sqlrprotocol_oracle::sendFetch3Response(sqlrservercursor *cursor,
						uint32_t cursorid,
						uint32_t rowstofetch) {

	// the modern path body: the same as an execute's answer minus the
	// describe and the return parameters - a row header, the rows, and a
	// summary object
	resetSendPacketBuffer(PACKET_DATA);

	uint32_t	colcount=cont->colCount(cursor);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	uint32_t	rowsfetched=0;
	bool		endofrows=true;

	if (colcount && rowstofetch) {

		endofrows=false;

		// a lob's contents are read out of the connection's current
		// row - there's no way to address a row it has moved on from -
		// so a result set with a lob column in it sends one row per
		// response, and that row stays current until the pin comes off
		if (rowstofetch>1 && hasLobColumn(cursor,colcount)) {
			debugWrite("lob column, one row at a time");
			rowstofetch=1;
		}

		// the only bound on how many rows to send back in this
		// packet is the negotiated packet size, less enough room
		// for the trailing summary
		const uint32_t	trailerreserve=128;
		uint32_t	curid=cont->getId(cursor);

		while (rowsfetched<rowstofetch) {

			// a row held over from a previous packet-full
			// response goes out first, ahead of anything
			// freshly fetched
			if (pendingrow[curid].getSize()) {
				if (rowsfetched &&
					reqpacket.getSize()+
					pendingrow[curid].getSize()+
					trailerreserve>=sdu) {
					debugWrite("packet full");
					break;
				}
				// a fetch with no rows left is answered
				// with the summary object alone, so the row
				// header is only written once the first row
				// is in hand
				if (!rowsfetched) {
					putRowHeader(0x02,colcount,
								rowstofetch);
				}
				reqpacket.append(
					pendingrow[curid].getBuffer(),
					pendingrow[curid].getSize());
				pendingrow[curid].clear();
				rowsfetched++;
				continue;
			}

			uint32_t	sizebeforerow=(uint32_t)reqpacket.getSize();

			bool	error=false;
			if (!cont->fetchRow(cursor,&error)) {
				if (error) {
					return sendQueryError(cursor);
				}
				endofrows=true;
				break;
			}

			// a fetch with no rows left is answered with the
			// summary object alone, so the row header is only
			// written once the first row is in hand
			if (!rowsfetched) {
				putRowHeader(0x02,colcount,rowstofetch);
			}

			rowhaslob=false;

			debugStart("fetch response row");
			putRowData(cursor,colcount);
			debugEnd();

			// a row that handed out a locator pins the connection
			// to itself, so the cursor doesn't advance past it and
			// the row never goes into pendingrow - which assumes
			// the connection has already moved on
			if (rowhaslob) {
				pinLobRow(cursor,colcount);
				rowsfetched++;
				break;
			}

			// the row is consumed from the result set as soon
			// as it's fetched - fetchRow()/nextRow() can't
			// un-fetch it on every backend, so a row that
			// doesn't fit here is stashed in pendingrow and sent
			// first next time, rather than left for a re-fetch
			// that some backends can't do
			// FIXME: kludgy
			cont->nextRow(cursor);

			// a row's size isn't known until it's written, so
			// the check comes after - stash it and stop if it
			// doesn't fit, unless it's the first row, which
			// goes out regardless of its size since the packet
			// can't say "zero rows" when more remain
			if (rowsfetched &&
				reqpacket.getSize()+trailerreserve>=sdu) {
				debugWrite("packet full");
				uint32_t	rowsize=(uint32_t)
					reqpacket.getSize()-sizebeforerow;
				pendingrow[curid].append(
					reqpacket.getBuffer()+sizebeforerow,
					(size_t)rowsize);
				reqpacket.truncate((size_t)sizebeforerow);
				break;
			}

			rowsfetched++;
		}
	}

	rowssent[cont->getId(cursor)]+=rowsfetched;

	if (endofrows) {
		putSummary(cursorid,ORA_NO_DATA_FOUND,
					rowssent[cont->getId(cursor)],
					ORA_NO_DATA_FOUND_MESSAGE);
	} else {
		putSummary(cursorid,0,rowssent[cont->getId(cursor)],NULL);
	}

	if (getDebug()) {
		debugStart("fetch response");
		debugWrite("data flags: 0x%04x",dataflags);
		debugWrite("column count: %d",colcount);
		debugWrite("rows: %d",rowsfetched);
		debugWrite("end of rows: %s",(endofrows)?"true":"false");
		debugEnd();
	}

	return sendPacket(true);
}

bool sqlrprotocol_oracle::fetch(const byte_t *rp) {

	// path-independent, but which shape a fetch takes is a property of
	// the session, not of the packet - once a session has sent one
	// query3(), every later fetch on every cursor takes the modern shape

	if (query3session) {
		return fetch3(rp);
	}

	// parse the request...
	uint16_t	options;
	uint16_t	moreoptions;
	uint16_t	cursorid;

	// FIXME: decode this... see "Oracle Wire Protocol - Fetch"
	readBE(rp,&options,&rp);
	readBE(rp,&moreoptions,&rp);

	// no cursor id follows on the wire here - use whichever cursor
	// open(), query(), query2() or execute() touched last
	cursorid=lastcursorid;

	if (getDebug()) {
		debugStart("fetch request");
		debugOptions(options,moreoptions);
		debugWrite("cursor id: %d",cursorid);
		debugEnd();
	}

	// get the requested cursor
	sqlrservercursor	*cursor=cont->getCursor(cursorid);
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError(cursorid);
	}

	return sendFetchResponse(cursor,
				(options&OPTION_PARSE),
				(options&OPTION_DEFINE),
				(options&OPTION_SNDIOV),
				(options&OPTION_EXACTFETCH));
}

bool sqlrprotocol_oracle::sendFetchResponse(sqlrservercursor *cursor,
							bool parse,
							bool define,
							bool sndiov,
							bool exactfetch) {

	// the legacy path body
	resetSendPacketBuffer(PACKET_DATA);

	// a captured legacy response that returns rows without DEFINE set
	// carries no data flags word at all.  every other type 6 packet in
	// the protocol carries it unconditionally, so the gap looks like an
	// accident, and sending it here is the safer read.
	// see "Oracle Wire Protocol - Fetch"
	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	if (getDebug()) {
		debugStart("fetch response header");
		debugWrite("data flags: 0x%04x",dataflags);
		debugEnd();
	}

	uint32_t	colcount=cont->colCount(cursor);
	cacheColumnDefinitions(cursor,colcount);

	// the row count in the legacy fetch request still isn't decoded - where
	// it sits in the request body is unidentified, and finding it would take
	// a live capture or the legacy client source.  until then, send every
	// row that's left in the result set, rather than hard-stopping at one
	// row per round trip.  the only bound is the negotiated packet size,
	// less enough room for the largest trailer sent after this loop.
	const uint32_t	trailerreserve=128;

	// for each row...
	uint32_t rowsfetched=0;
	do {

		// stop if there's no room left in the packet for another row
		// and the trailer
		if (reqpacket.getSize()+trailerreserve>=sdu) {
			debugWrite("packet full");
			break;
		}

		// fetch a row
		bool	error=false;
		if (!cont->fetchRow(cursor,&error)) {
			if (error) {
				return sendQueryError(cursor);
			}
			break;
		}

		// ok, so there is at least one row...
		// send various headers and column definitions
		if (!rowsfetched) {

			// FIXME: the headers/col-defs appear to be very
			// different when sent from 8i
			// see "Oracle Wire Protocol - Column Definitions"

			if (define) {

				// ttc type 0x10 reused bare as a response
				// header - not a describe info message
				byte_t		ttccode=TTC_DESCRIBE_INFO;

				write(&reqpacket,ttccode);

				if (getDebug()) {
					debugStart("fetch response header");
					debugTtcCode(ttccode);
					debugEnd();
				}
			}

			// send column definitions...
			if (define) {
				putColumnDefinitions(cursor,colcount);
			}

			// send "iov" (whatever that is)...
			// see "Oracle Wire Protocol - Fetch"
			if (sndiov) {
				if (getDebug()) {
					debugStart("fetch response header");
					debugWrite("iov");
					debugEnd();
				}
				putIov();
			} else {
				const byte_t	unknown[]={
					0x00, 0x00,
				};
				reqpacket.append(unknown,sizeof(unknown));
				if (getDebug()) {
					debugStart("fetch response header");
					debugWrite("no iov");
					debugHexDump(unknown,sizeof(unknown));
					debugEnd();
				}
			}

			// unknown2 through unknown4 are unexplained
			// see "Oracle Wire Protocol - Fetch"
			// always appears to be the same...
			const byte_t	unknown2[]={
				0x06, 0x02,
			};
			reqpacket.append(unknown2,sizeof(unknown2));

			// FIXME: this varies, but it's not clear with what
			const byte_t	unknown3[]={
				0x8C
			};
			reqpacket.append(unknown3,sizeof(unknown3));

			if (getDebug()) {
				debugStart("fetch response header");
				debugHexDump(unknown2,sizeof(unknown2));
				debugHexDump(unknown3,sizeof(unknown3));
				debugEnd();
			}

			write(&reqpacket,(byte_t)colcount);

			// always appears to be the same...
			const byte_t	unknown4[]={
				0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00
			};
			reqpacket.append(unknown4,sizeof(unknown4));

			if (getDebug()) {
				debugStart("fetch response header");
				debugWrite("column count: %d",colcount);
				debugHexDump(unknown4,sizeof(unknown4));
				debugEnd();
			}
		}

		// row marker, written ahead of each row
		// see "Oracle Wire Protocol - Row Data"
		write(&reqpacket,(byte_t)7);

		debugStart("fetch response row");
		debugWrite("row marker: 0x07");

		if (!putRow(cursor,colcount,exactfetch)) {
			debugEnd();
			return sendQueryError(cursor);
		}

		debugEnd();

		// FIXME: kludgy
		cont->nextRow(cursor);

		rowsfetched++;

	} while (true);

	if (rowsfetched) {

		// unknown6 and unknown8 below were only captured for 1- and
		// 2-column cursors, and how to compute them for wider ones
		// isn't known, so fall back to the non-exact-fetch trailer
		// rather than index past the end of them.
		if (exactfetch && colcount && colcount<=2) {
			// unknown5 through unknown11 are unexplained
			// see "Oracle Wire Protocol - Fetch"
			const byte_t	unknown5[]={
				// ???
				0x08, 0x04, 0x00
			};

			const byte_t	unknown6[][1]={
				// 1 columns
				{0xA6},
				// 2 columns
				{0xA5}
			};

			const byte_t	unknown7[]={
				0x5C,
	
				// error code?
				0x16, 0x00,
	
				// ???
				0x00, 0x00, 0x00, 0x00,
				0x01,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x04,
				0x01, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x01, 0x00
			};
	
			const byte_t	unknown8[][4]={
				// 1 column
				{0x11, 0x00, 0x03, 0x00},
				// 2 columns
				{0x0E, 0x00, 0x03, 0x00}
			};
	
			const byte_t	unknown9[]={
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00,
		
				// ???
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
			};
	
			// this appears to increment with each response
			const byte_t	unknown10[]={
				0x27
			};
	
			const byte_t	unknown11[]={
 				0x00, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00
			};
	
			reqpacket.append(unknown5,sizeof(unknown5));
			reqpacket.append(unknown6[colcount-1],
						sizeof(unknown6[colcount-1]));
			reqpacket.append(unknown7,sizeof(unknown7));
			reqpacket.append(unknown8[colcount-1],
						sizeof(unknown8[colcount-1]));
			reqpacket.append(unknown9,sizeof(unknown9));
			reqpacket.append(unknown10,sizeof(unknown10));
			reqpacket.append(unknown11,sizeof(unknown11));

			if (getDebug()) {
				debugStart("fetch response footer");
				debugWrite("exact fetch");
				debugHexDump(unknown5,sizeof(unknown5));
				debugHexDump(unknown6[colcount-1],
						sizeof(unknown6[colcount-1]));
				debugHexDump(unknown7,sizeof(unknown7));
				debugHexDump(unknown8[colcount-1],
						sizeof(unknown8[colcount-1]));
				debugHexDump(unknown9,sizeof(unknown9));
				debugHexDump(unknown10,sizeof(unknown10));
				debugHexDump(unknown11,sizeof(unknown11));
				debugEnd();
			}

		} else {

			// unexplained.  see "Oracle Wire Protocol - Fetch"
			const byte_t	unknown[]={
				0x04, 0x01, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00,
				0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			};

			reqpacket.append(unknown,sizeof(unknown));

			if (getDebug()) {
				debugStart("fetch response footer");
				debugWrite("not exact fetch");
				debugHexDump(unknown,sizeof(unknown));
				debugEnd();
			}
		}

	} else {

		// end of result set
		debugWrite("no rows fetched");
		putError("ORA-01403: no data found");
	}

	putGenericFooter();
	
	return sendPacket(true);
}

void sqlrprotocol_oracle::cacheColumnDefinitions(sqlrservercursor *cursor,
							uint32_t colcount) {
	debugStart("cache column definitions");
	debugWrite("column count: %d",colcount);
	if (!colcount) {
		debugWrite("no columns");
	}

	uint16_t	curid=cont->getId(cursor);
	debugWrite("cursor id: %d",curid);

	if (columntypescached[curid]) {
		debugWrite("already cached");
		debugEnd();
		return;
	}

	if (!cont->getMaxColumnCount()) {
		delete[] columntypes[curid];
		if (colcount) {
			columntypes[curid]=new uint16_t[colcount];
		} else {
			columntypes[curid]=NULL;
		}
	}

	uint16_t	*ct=columntypes[curid];

	for (uint32_t i=0; i<colcount; i++) {
		ct[i]=getColumnType(cont->getColumnTypeName(cursor,i),
					cont->getColumnTypeNameSize(cursor,i),
					cont->getColumnScale(cursor,i));
		ct[i]=getUnknownColumnType(cursor,i,ct[i]);
		ct[i]=getLongColumnType(cursor,i,ct[i]);
		ct[i]=getLobColumnType(cursor,i,ct[i]);
		debugWrite("%s: %d",cont->getColumnTypeName(cursor,i),ct[i]);
	}

	// A cursor with no columns has nothing to cache.  Marking it cached
	// anyway hands every later execute on the cursor an array that was
	// never filled in - which is what a parse-only request does, since it
	// executes nothing.
	if (colcount) {
		columntypescached[curid]=true;
	}

	debugEnd();
}

void sqlrprotocol_oracle::putColumnDefinitions(sqlrservercursor *cursor,
							uint32_t colcount) {

	byte_t	sizetotal=0;
	for (uint32_t i=0; i<colcount; i++) {
		sizetotal+=cont->getColumnSize(cursor,i);
	}
	// unexplained.  see "Oracle Wire Protocol - Column Definitions"
	uint32_t	constant=COLUMN_DEFINITIONS_CONSTANT;

	write(&reqpacket,sizetotal);
	writeBE(&reqpacket,colcount);
	writeBE(&reqpacket,constant);

	debugStart("column definitions header");
	debugWrite("size total: %d",sizetotal);
	debugWrite("column count: %d",colcount);
	debugWrite("constant: %d",constant);
	debugEnd();

	debugStart("column definitions");

	for (uint32_t i=0; i<colcount; i++) {
		putColumnDefinition(cursor,i);
	}

	debugEnd();
}

void sqlrprotocol_oracle::putColumnDefinition(sqlrservercursor *cursor,
							uint32_t column) {
	uint16_t	curid=cont->getId(cursor);

	//uint16_t	sqlrcolumntype=cont->getColumnType(cursor,column);
	const char	*columntypestring=
				cont->getColumnTypeName(cursor,column);
	uint16_t	columntype=columntypes[curid][column];
	bool	character=(getWireColumnType(columntype)!=ORACLE_TYPE_NUMBER);
	/*uint16_t	columnflags=getColumnFlags(cursor,column,
							sqlrcolumntype,
							columntype,
							columntypestring);*/

	// see "Oracle Wire Protocol - Column Definitions"
	// meaning unknown
	byte_t	marker1=1;
	// 128 for char/varchar, 0 for numeric
	byte_t	marker2=(character)?128:0;
	byte_t	precision=cont->getColumnPrecision(cursor,column);
	byte_t	scale=cont->getColumnScale(cursor,column);
	// 16 for non-integer decimal, otherwise actual size
	byte_t	size=cont->getColumnSize(cursor,column);
	// its 18 zero bytes are unexplained
	byte_t	unknown1[]={
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00
	};
	// 1 for char/varchar, 0 for numeric
	uint16_t	marker3=(character)?1:0;
	uint16_t	nullable=(cont->getColumnIsNullable(cursor,column))?1:0;
	// 1 for select from table
	// 2 for select from table with alias
	// 3 for select from dual
	// 4 for select from dual with alias
	byte_t		alias=1;
	const char	*name=cont->getColumnName(cursor,column);
	uint32_t	namesize=cont->getColumnNameSize(cursor,column);
	// see "Oracle Wire Protocol - Column Definitions"
	// meaning unknown
	uint32_t	marker4=0;
	// meaning unknown
	uint32_t	marker5=0;

	write(&reqpacket,marker1);
	write(&reqpacket,(byte_t)columntype);
	write(&reqpacket,marker2);
	write(&reqpacket,precision);
	write(&reqpacket,scale);
	write(&reqpacket,size);
	reqpacket.append(unknown1,sizeof(unknown1));
	// yes, twice
	writeBE(&reqpacket,marker3);
	writeBE(&reqpacket,marker3);
	writeBE(&reqpacket,nullable);
	// yes, twice
	write(&reqpacket,alias);
	write(&reqpacket,alias);
	writeBE(&reqpacket,namesize);
	write(&reqpacket,name,namesize);
	writeBE(&reqpacket,marker4);
	writeBE(&reqpacket,marker5);

	debugStart("column %d",column);
	debugWrite("marker1: %d",marker1);
	debugColumnType(columntypestring,columntype);
	debugWrite("marker2: %d",marker2);
	debugWrite("precision: %d",precision);
	debugWrite("scale: %d",scale);
	debugWrite("size: %d",size);
	debugWrite("marker3: %d",marker3);
	debugWrite("nullable: %d",nullable);
	debugWrite("name size: %u",namesize);
	debugWrite("name: %s",name);
	debugWrite("marker4: %d",marker4);
	debugWrite("marker5: %d",marker5);
	debugEnd();
}

uint16_t sqlrprotocol_oracle::getColumnType(const char *columntypestring,
						uint16_t columntypesize,
						uint32_t scale) {

	// sometimes column types have parentheses, like CHAR(40)
	const char	*leftparen=charstring::findFirst(columntypestring,"(");
	if (leftparen) {
		columntypesize=leftparen-columntypestring;
	}

	const char * const 	*datatypestring=cont->dataTypeStrings();

	for (uint32_t index=0; datatypestring[index]; index++) {

		// compare "columntypesize" bytes but also make sure that the
		// byte afterward is a NULL, we don't want "DATE" to match
		// "DATETIME" for example
		if (!charstring::compareIgnoringCase(
					datatypestring[index],
					columntypestring,
					columntypesize) &&
				datatypestring[index][columntypesize]=='\0') {

			// bail on a type that the map doesn't cover.
			// dataTypeStrings() and oracletypemap[] are
			// maintained separately, so a type added to one
			// and not the other would index past the end.
			if (index>=sizeof(oracletypemap)/
					sizeof(oracletypemap[0])) {
				debugWrite("invalid column type: %s",
							columntypestring);
				return ORACLE_TYPE_VARCHAR;
			}

			uint16_t	retval=oracletypemap[index];

			// some db's, like oracle, don't distinguish decimal
			// from integer types - a numeric field may or may not
			// have decimal points. those fields get translated to
			// "decimal" here; with 0 decimal points they should
			// translate to an integer type instead.
			// FIXME:
			/*if ((retval==MYSQL_TYPE_DECIMAL ||
				retval==MYSQL_TYPE_NEWDECIMAL) && !scale) {
				retval=MYSQL_TYPE_LONG;
			}*/

			// some db's, like oracle, don't have separate DATE and
			// DATETIME types - a DATE can store both date and time,
			// and which components it reports depends on something
			// like NLS_DATE_FORMAT. by default we map DATE to
			// MYSQL_TYPE_DATE, but also offer mapping it to
			// MYSQL_TYPE_DATETIME.
			// FIXME:
			/*if (retval==MYSQL_TYPE_DATE && datetodatetime) {
				retval=MYSQL_TYPE_DATETIME;
			}*/
			return retval;
		}
	}
	// FIXME:
	//return MYSQL_TYPE_NULL;
	return ORACLE_TYPE_VARCHAR;
}

uint16_t sqlrprotocol_oracle::getUnknownColumnType(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t columntype) {

	// a type the backend has no datatype of its own for comes through
	// named UNKNOWN, and getColumnType() takes that for a varchar2.  a
	// plain timestamp and a timestamp with local time zone are picked
	// out of that earlier, in the connection module, by their own oci
	// type codes (see getColumnType()'s TIMESTAMP_TYPE/TIMESTAMP_LTZ_TYPE
	// cases in src/connections/oracle.cpp) - what's left to recover here
	// is interval year to month, interval day to second, and timestamp
	// with time zone, none of which have an oci type code of their own
	// in the connection module yet, and a varchar2 is a lie about all
	// three - a client that asks what the column is gets 1 and 4000
	// back instead of the type and the width the value really has.
	//
	// the size the backend reports tells the three apart, and, now that
	// a plain/local-tz timestamp is resolved before reaching here, tells
	// them apart from everything else that arrives UNKNOWN too:
	//
	//	interval year to month		size 5
	//	interval day to second		size 11
	//	timestamp with time zone	size 13
	//
	// precision isn't part of that test.  it was, in an earlier version
	// of this function, as a stand-in for "is this really an interval,
	// or a plain timestamp that happens to also be 11 bytes wide" - but
	// oracle allows an interval's leading field precision to be declared
	// 0 ("interval day(0) to second"), which reports precision 0, the
	// same as a timestamp's - a live server confirms both read (size 11,
	// precision 0) identically, so precision can't tell them apart, and
	// doesn't need to now that the timestamp side of that collision is
	// resolved earlier instead
	if (columntype!=ORACLE_TYPE_VARCHAR) {
		return columntype;
	}

	// this size/name-based recovery is only safe for a genuine oracle
	// backend.  postgresql reports its own interval and timestamp with
	// time zone columns under the same "INTERVAL"/"TIMESTAMPTZ" names a
	// real oracle column would (see oracletypemap[]), and there's no
	// guarantee its own reported column size ever collides with 5, 11 or
	// 13 - but there's no guarantee it doesn't, either, and getting this
	// wrong would silently mis-type a postgresql column, not just
	// mis-encode its value (that part putRowData() already gates - see
	// the callers of putIntervalField()/putTimestampField())
	if (charstring::compare(cont->getNativeDbType(),"oracle")) {
		return columntype;
	}

	const char * const	*datatypestring=cont->dataTypeStrings();
	if (charstring::compareIgnoringCase(
				cont->getColumnTypeName(cursor,column),
				datatypestring[0])) {
		return columntype;
	}

	uint32_t	size=cont->getColumnSize(cursor,column);

	if (size==ORACLE_INTERVALYM_SIZE) {
		return ORACLE_TYPE_INTERVALYM;
	}
	if (size==ORACLE_INTERVALDS_SIZE) {
		return ORACLE_TYPE_INTERVALDS;
	}
	if (size==ORACLE_TIMESTAMPTZ_SIZE) {
		return ORACLE_TYPE_TIMESTAMPTZ;
	}
	return columntype;
}

uint16_t sqlrprotocol_oracle::getLongColumnType(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t columntype) {

	// "LONG" is one type name in the controller's table that means two
	// different things.  oracle returns it for its own LONG, which is
	// character data, and sap/freetds return it for CS_LONG_TYPE, which
	// is binary - see isBlobTypeChar() in src/common/datatypes.h, which
	// keeps it among the binary types for exactly that reason.
	// oracletypemap[] has to answer for both backends at once, so it
	// leaves the name mapped to a blob, and a real oracle LONG is
	// picked back out here instead.
	//
	// which means the real backend has to be oracle.  any backend can
	// be fronted by any protocol module, so this same describe runs
	// over a sap column named LONG too, and encoding that column's
	// binary as oracle's text LONG would quietly corrupt it.  nothing
	// narrower tells the two apart - both arrive mapped to a blob,
	// neither has a declared width - so it is the backend's own
	// identity that decides, which is what getNativeDbType() answers
	// (getDbType() answers what the client was told to expect, which
	// is not the same question)
	if (columntype!=ORACLE_TYPE_BLOB) {
		return columntype;
	}
	if (charstring::compare(cont->getNativeDbType(),"oracle")) {
		return columntype;
	}
	if (charstring::compareIgnoringCase(
			cont->getColumnTypeName(cursor,column),"LONG")) {
		return columntype;
	}
	return ORACLE_TYPE_LONG;
}

uint16_t sqlrprotocol_oracle::getLobColumnType(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t columntype) {

	// a locator is only any use if there's a real lob behind it - the
	// module reads one back through the controller's lob calls, and only
	// a backend that hands the field over as a lob answers those.  a blob
	// is also what the type map folds a good many unrelated types onto as
	// a catch-all - a bytea, a json, a postgresql array - so it's the
	// backend's own identity that decides here, the way it does in
	// getLongColumnType()
	if (columntype!=ORACLE_TYPE_CLOB &&
		columntype!=ORACLE_TYPE_BLOB &&
		columntype!=ORACLE_TYPE_BFILE) {
		return columntype;
	}
	if (charstring::compare(cont->getNativeDbType(),"oracle")) {
		return columntype;
	}

	const char	*name=cont->getColumnTypeName(cursor,column);
	if (!charstring::compareIgnoringCase(name,"CLOB")) {
		return ORACLE_TYPE_LOB_CLOB;
	}
	if (!charstring::compareIgnoringCase(name,"BLOB")) {
		return ORACLE_TYPE_LOB_BLOB;
	}
	if (!charstring::compareIgnoringCase(name,"BFILE")) {
		return ORACLE_TYPE_LOB_BFILE;
	}
	return columntype;
}

uint16_t sqlrprotocol_oracle::getColumnFlags(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t sqlrcolumntype,
						uint16_t columntype,
						const char *columntypestring) {
	return getColumnFlags(cursor,
				sqlrcolumntype,
				columntype,
				columntypestring,
				cont->getColumnIsNullable(cursor,column),
				cont->getColumnIsPrimaryKey(cursor,column),
				cont->getColumnIsUnique(cursor,column),
				cont->getColumnIsPartOfKey(cursor,column),
				cont->getColumnIsUnsigned(cursor,column),
				cont->getColumnIsZeroFilled(cursor,column),
				cont->getColumnIsBinary(cursor,column),
				cont->getColumnIsAutoIncrement(cursor,column));
}

uint16_t sqlrprotocol_oracle::getColumnFlags(sqlrservercursor *cursor,
						uint16_t sqlrcolumntype,
						uint16_t columntype,
						const char *columntypestring,
						bool isnullable,
						bool isprimarykey,
						bool isunique,
						bool ispartofkey,
						bool isunsigned,
						bool iszerofilled,
						bool isbinary,
						bool isautoincrement) {
	uint16_t	flags=0;
	/*if (!isnullable) {
		flags|=NOT_NULL_FLAG;
	}
	if (isprimarykey) {
		flags|=PRI_KEY_FLAG;
	}
	if (isunique) {
		flags|=UNIQUE_KEY_FLAG;
	}
	if (ispartofkey) {
		flags|=MULTIPLE_KEY_FLAG;
	}
	if (columntype==MYSQL_TYPE_TINY_BLOB ||
		columntype==MYSQL_TYPE_MEDIUM_BLOB ||
		columntype==MYSQL_TYPE_LONG_BLOB ||
		columntype==MYSQL_TYPE_BLOB) {
		flags|=BLOB_FLAG;
	}
	if (isunsigned || (sqlrcolumntype!=(uint16_t)-1)?
				cont->isUnsignedType(sqlrcolumntype):
				cont->isUnsignedType(columntypestring)) {
		flags|=UNSIGNED_FLAG;
	}
	if (iszerofilled) {
		flags|=ZEROFILL_FLAG;
	}
	if (isbinary || (sqlrcolumntype!=(uint16_t)-1)?
				cont->isBinaryType(sqlrcolumntype):
				cont->isBinaryType(columntypestring)) {
		flags|=BINARY_FLAG;
	}
	if (columntype==MYSQL_TYPE_ENUM) {
		flags|=ENUM_FLAG;
	}
	if (isautoincrement) {
		flags|=AUTO_INCREMENT_FLAG;
	}
	if (columntype==MYSQL_TYPE_TIMESTAMP ||
		columntype==MYSQL_TYPE_TIMESTAMP2) {
		flags|=TIMESTAMP_FLAG|ON_UPDATE_NOW_FLAG;
	}
	if (columntype==MYSQL_TYPE_SET) {
		flags|=SET_FLAG;
	}
	if ((sqlrcolumntype!=(uint16_t)-1)?
			cont->isNumberType(sqlrcolumntype):
			cont->isNumberType(columntypestring)) {
		flags|=NUM_FLAG;
	}*/
	return flags;
}

void sqlrprotocol_oracle::putIov() {

	debugStart("iov");

	// unexplained.  see "Oracle Wire Protocol - Fetch"
	// always appears to be the same...
	const byte_t	unknown[]={
		0x07, 0x00, 0x00, 0x00,
		// timestamp?  if so, it's
		// 12/21/1973 10:13:14 EST
		0x07, 0x78, 0x75, 0x0A
	};
	reqpacket.append(unknown,sizeof(unknown));
	debugHexDump(unknown,sizeof(unknown));

	// this appears to be a seconds-since timestamp
	// what's the significance of this date?
	// it's suspiciously close to my 8.0.5
	// software/db install date,
	// which was Oct 4, 2012.
	datetime	dtsince;
	dtsince.init("12/15/2012 11:15:00 EST");
	datetime	dt;
	dt.initFromSystemDateTime();
	uint32_t	timestamp=dt.getEpoch()-dtsince.getEpoch();
	writeBE(&reqpacket,timestamp);
	debugWrite("timestamp: %u",timestamp);

	debugEnd();
}

bool sqlrprotocol_oracle::putRow(sqlrservercursor *cursor,
						uint32_t colcount,
						bool terminator) {

	// get the column types
	uint16_t	*ct=columntypes[cont->getId(cursor)];

	// field pointers
	const char	*field;
	uint64_t	fieldsize;
	bool		lob;
	bool		null;

	// put the fields
	for (uint32_t i=0; i<colcount; i++) {

		debugStart("col %d",i);
		debugColumnType(ct[i]);

		// get the field (again)
		fieldsize=0;
		lob=false;
		null=false;
		if (!cont->getField(cursor,i,&field,&fieldsize,&lob,&null)) {
			debugEnd();
			return false;
		}

		// put the field
		if (lob) {
			debugWrite("LOB");
			putLobField(cursor,i);
		} else if (!null) {
			debugWrite("\"%s\" (%lld)",field,(long long)fieldsize);
			bool	wrote=putField(field,fieldsize,ct[i]);

			// the terminator: a ub4 after every column but the
			// last, a ub2 after the last.  it belongs to the row
			// rather than to the value, but write it only if the
			// value itself was written, otherwise it desyncs the
			// rest of the row.
			// see "Oracle Wire Protocol - Row Data"
			if (terminator && wrote) {
				if (i==colcount-1) {
					writeBE(&reqpacket,(uint16_t)0);
				} else {
					writeBE(&reqpacket,(uint32_t)0);
				}
				debugWrite("terminator");
			}
		} else {
			debugWrite("null");
		}

		debugEnd();
	}

	return true;
}

bool sqlrprotocol_oracle::putField(const char *field,
					uint64_t fieldsize,
					uint16_t columntype) {

	switch (columntype) {
		case ORACLE_TYPE_CHAR:
		case ORACLE_TYPE_VARCHAR:
		case ORACLE_TYPE_FIXED_CHAR:
			{
			// The legacy form writes a one-byte size and that
			// many bytes.  What a real server sends for a value
			// longer than that hasn't been confirmed, so clamp
			// rather than guess.  Writing the full value under a
			// truncated size byte would desync the stream for the
			// rest of the packet.
			byte_t	size=(fieldsize>255)?255:(byte_t)fieldsize;
			write(&reqpacket,size);
			write(&reqpacket,field,(size_t)size);
			debugWrite("field size: %d",size);
			debugWrite("field: \"%.*s\"",(int)size,field);
			}
			return true;
		case ORACLE_TYPE_NUMBER:
		case ORACLE_TYPE_VARNUM:
			// putNumberField wraps its own output in a CLR
			putNumberField(field,(uint32_t)fieldsize);
			return true;
		case ORACLE_TYPE_LONG:
			// FIXME: implement this
			debugWrite("long (not implemented)");
			return false;
		case ORACLE_TYPE_ROWID_DEPRECATED:
			// FIXME: implement this
			debugWrite("rowid (deprecated) (not implemented)");
			return false;
		case ORACLE_TYPE_DATE:
			{
			// a fixed 7 raw bytes, no length prefix
			byte_t	date[ORACLE_DATE_SIZE];
			if (!getOracleDate(field,fieldsize,date)) {
				// better to send nothing at all than
				// to send 7 bytes of garbage
				return false;
			}
			write(&reqpacket,(const char *)date,sizeof(date));
			}
			return true;
		case ORACLE_TYPE_RAW:
			// FIXME: implement this
			debugWrite("raw (not implemented)");
			return false;
		case ORACLE_TYPE_LONG_RAW:
			// FIXME: implement this
			debugWrite("long raw (not implemented)");
			return false;
		case ORACLE_TYPE_RESULT_SET:
			// FIXME: implement this
			debugWrite("result set (not implemented)");
			return false;
		case ORACLE_TYPE_ROWID:
			// FIXME: implement this
			debugWrite("rowid (not implemented)");
			return false;
		case ORACLE_TYPE_NAMED_TYPE:
			// FIXME: implement this
			debugWrite("named type (not implemented)");
			return false;
		case ORACLE_TYPE_REF_TYPE:
			// FIXME: implement this
			debugWrite("ref type (not implemented)");
			return false;
		// putRow handles lobs itself, before it ever gets here,
		// so these three only come up if a column is declared a
		// lob but the driver didn't flag it as one
		case ORACLE_TYPE_CLOB:
			// FIXME: implement this
			debugWrite("clob (not implemented)");
			return false;
		case ORACLE_TYPE_BLOB:
			// FIXME: implement this
			debugWrite("blob (not implemented)");
			return false;
		case ORACLE_TYPE_BFILE:
			// FIXME: implement this
			debugWrite("bfile (not implemented)");
			return false;
		case ORACLE_TYPE_TIMESTAMP:
			// FIXME: implement this
			debugWrite("timestamp (not implemented)");
			return false;
		case ORACLE_TYPE_TIMESTAMPTZ:
			// FIXME: implement this
			debugWrite("timestamp tz (not implemented)");
			return false;
		case ORACLE_TYPE_INTERVALYM:
			// FIXME: implement this
			debugWrite("interval year-month (not implemented)");
			return false;
		case ORACLE_TYPE_INTERVALDS:
			// FIXME: implement this
			debugWrite("interval day-second (not implemented)");
			return false;
		case ORACLE_TYPE_TIMESTAMPLTZ:
			// FIXME: implement this
			debugWrite("timestamp ltz (not implemented)");
			return false;
		case ORACLE_TYPE_PLSQL_INDEX_TABLE:
			// FIXME: implement this
			debugWrite("plsql index table (not implemented)");
			return false;
		default:
			debugWrite("unknown column type: %d",columntype);
			return false;
	}
}

#define MAX_BYTES_PER_CHAR	4

void sqlrprotocol_oracle::putLobField(sqlrservercursor *cursor, uint32_t col) {

	debugStart("lob field");

	// get lob size
	uint64_t	loblength;
	if (!cont->getLobFieldLength(cursor,col,&loblength)) {
		debugWrite("null");
		// send NULL as 0xfb, recognized by readLenEncInt()'s
		// isnull out-param (see src/server/sqlrprotocol.cpp)
		reqpacket.append((char)0xfb);
		cont->closeLobField(cursor,col);
		debugEnd();
		return;
	}

	debugWrite("lob length: %lld",(long long)loblength);

	// for lobs of 0 length
	if (!loblength) {
		writeLenEncInt(&reqpacket,0);
		cont->closeLobField(cursor,col);
		debugEnd();
		return;
	}

	// initialize sizes and status
	uint64_t	charstoread=sizeof(lobbuffer)/MAX_BYTES_PER_CHAR;
	uint64_t	charsread=0;
	uint64_t	offset=0;
	bool		start=true;

	for (;;) {

		// read a segment from the lob
		if (!cont->getLobFieldSegment(cursor,col,
					lobbuffer,sizeof(lobbuffer),
					offset,charstoread,&charsread) ||
					!charsread) {

			// no data - send null if nothing sent yet, else end
			if (start) {
				debugWrite("null");
				// send NULL as 0xfb (see the comment
				// above the other 0xfb send in this
				// function)
				reqpacket.append((char)0xfb);
			}
			cont->closeLobField(cursor,col);
			debugEnd();
			return;

		} else {

			// start sending
			if (start) {
				writeLenEncInt(&reqpacket,loblength);
				start=false;
			}

			// put the segment we just got
			reqpacket.append(lobbuffer,charsread);
			debugWrite("chunk size: %lld",(long long)charsread);

			offset=offset+charstoread;
		}
	}
}

void sqlrprotocol_oracle::putError(const char *error, uint32_t oranum) {
	putError(error,charstring::getLength(error),oranum);
}

void sqlrprotocol_oracle::putError(const char *error, uint32_t errorsize,
							uint32_t oranum) {

	// the data flags word is per-packet, not per-message, so it's up
	// to the caller to have already written it
	byte_t		ttccode=TTC_ERROR;

	// unknown1 and unknown2 are unexplained
	// see "Oracle Wire Protocol - Fetch"
	byte_t	unknown1[]={
		0x00, 0x00, 0x00, 0x00, 0x7B,
		0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,

		// varies... sequence?
		0x10,

		0x00, 0x00, 0x01, 0x00, 0x00, 0x00
	};

	// the ora number, little-endian, at offsets 4 and 5 - 0x7b 0x05 above
	// is 1403, the default
	unknown1[4]=(byte_t)(oranum&0xff);
	unknown1[5]=(byte_t)((oranum>>8)&0xff);

	const byte_t	unknown2[]={
		0x0A
	};

	// the message size on the wire is a single byte (ub1); truncate
	// rather than let a longer message desync the length byte from the
	// bytes actually appended after it
	if (errorsize>255) {
		errorsize=255;
	}

	write(&reqpacket,ttccode);
	reqpacket.append(unknown1,sizeof(unknown1));
	write(&reqpacket,(byte_t)errorsize);
	reqpacket.append(error,errorsize);
	reqpacket.append(unknown2,sizeof(unknown2));

	debugStart("error response");
	debugTtcCode(ttccode);
	debugWrite("error: %u",oranum);
	debugWrite("error size: %u",errorsize);
	debugWrite("error: %.*s",(int)errorsize,error);
	debugEnd();
}

bool sqlrprotocol_oracle::close(const byte_t *rp) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t		seqnumber=0;
	uint32_t	cursorid=0;

	if (end-rp<1) {
		debugWrite("truncated close sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	if (!readLenPreInt(rp,end,&cursorid,&rp)) {
		debugWrite("truncated close cursor id");
		return false;
	}

	debugStart("close request");
	debugWrite("seq number: %d",seqnumber);
	debugWrite("cursor id: %d",cursorid);
	debugEnd();

	// the id on the wire is the controller's plus 1
	sqlrservercursor	*cursor=(cursorid)?
			cont->getCursor((uint16_t)(cursorid-1)):NULL;
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError(cursorid);
	}

	uint16_t	closingid=cont->getId(cursor);
	forgetRefCursor(closingid);
	clearParams(cursor);
	cont->abort(cursor);
	cont->release(cursor);
	pendingrow[closingid].clear();
	clearLobPin(closingid);
	if (lastcursorid==closingid) {
		lastcursorid=65535;
	}

	return sendCloseResponse(cursor);
}

// forgets the cursor's binds.  the values themselves come out of the
// cursor's bind pool, which owns them - freeing one individually is a free
// of pool memory, and glibc rejects it
void sqlrprotocol_oracle::clearParams(sqlrservercursor *cursor) {
	// counts only - the bind pool owns the values and frees them itself
	cont->setInputBindCount(cursor,0);
	cont->setOutputBindCount(cursor,0);
	// a ref cursor bind holds a real cursor, which isn't the bind pool's
	// to give back
	releaseRefCursors(cont->getId(cursor));
}

bool sqlrprotocol_oracle::sendCloseResponse(sqlrservercursor *cursor) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_STATUS;

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);

	debugStart("close response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugEnd();

	return sendPacket(true);
}

bool sqlrprotocol_oracle::disconnect(const byte_t *rp) {

	byte_t	seqnumber=0;
	read(rp,&seqnumber,&rp);

	debugStart("disconnect request");
	debugWrite("seq number: %d",seqnumber);
	debugEnd();

	return sendDisconnectResponse();
}

bool sqlrprotocol_oracle::sendDisconnectResponse() {

	// a logoff is answered with a status message: a call status and an
	// end-to-end sequence number, as a live 11.2 server answers ojdbc
	// 23.26's logoff - 14 bytes on the wire.  python-oracledb reads them
	// in _process_message(), in impl/thin/messages/base.pyx.  a client
	// that reads past a truncated one gets nothing and reports ORA-03113
	// for a logoff that actually succeeded.
	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_STATUS;
	uint32_t	callstatus=1;
	uint32_t	endtoendseqnumber=0;

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	writeLenPreInt(&reqpacket,callstatus);
	writeLenPreInt(&reqpacket,endtoendseqnumber);

	debugStart("disconnect response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("call status: %d",callstatus);
	debugWrite("end to end seq number: %d",endtoendseqnumber);
	debugEnd();

	return sendPacket(true);
}

bool sqlrprotocol_oracle::commit(const byte_t *rp) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t	seqnumber=0;
	if (end-rp<1) {
		debugWrite("truncated commit sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	// the summary object has to echo this back
	callnumber=seqnumber;

	debugStart("commit request");
	debugWrite("seq number: %d",seqnumber);
	debugEnd();

	if (!cont->commit()) {
		return sendTransactionError();
	}
	return sendTransactionResponse();
}

bool sqlrprotocol_oracle::rollback(const byte_t *rp) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t	seqnumber=0;
	if (end-rp<1) {
		debugWrite("truncated rollback sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	callnumber=seqnumber;

	debugStart("rollback request");
	debugWrite("seq number: %d",seqnumber);
	debugEnd();

	if (!cont->rollback()) {
		return sendTransactionError();
	}
	return sendTransactionResponse();
}

bool sqlrprotocol_oracle::autoCommitOn(const byte_t *rp) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t	seqnumber=0;
	if (end-rp<1) {
		debugWrite("truncated autocommit-on sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	callnumber=seqnumber;

	debugStart("autocommit-on request");
	debugWrite("seq number: %d",seqnumber);
	debugEnd();

	if (!cont->setAutoCommitOn()) {
		return sendTransactionError();
	}
	return sendTransactionResponse();
}

bool sqlrprotocol_oracle::autoCommitOff(const byte_t *rp) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t	seqnumber=0;
	if (end-rp<1) {
		debugWrite("truncated autocommit-off sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	callnumber=seqnumber;

	debugStart("autocommit-off request");
	debugWrite("seq number: %d",seqnumber);
	debugEnd();

	if (!cont->setAutoCommitOff()) {
		return sendTransactionError();
	}
	return sendTransactionResponse();
}

// what a bare commit, rollback or autocommit change gets back on success -
// the same summary-object/putError+footer split every other cursorless ack
// in this module uses (sendCursorNotOpenError, sendMarkerCancelError,
// sendUnimplementedFunctionError), with success field values in place of
// an error
bool sqlrprotocol_oracle::sendTransactionResponse() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("transaction response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugEnd();

	if (query3session) {
		putSummary(0,0,0,NULL);
	} else {
		putError("",0,0);
		putGenericFooter();
	}

	return sendPacket(true);
}

// what a commit, rollback or autocommit change gets back when the
// controller call fails.  unlike sendQueryError(), there's no cursor here -
// a bare commit isn't tied to one - so this reads the connection-level
// error instead of the cursor-level one
bool sqlrprotocol_oracle::sendTransactionError(uint32_t cursorid) {

	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(&errorstring,&errorsize,&errnum,&liveconnection);

	uint32_t	oranum=ORA_TRANSACTION_FAILED;
	if (errnum>0 && errnum<=65535) {
		oranum=(uint32_t)errnum;
	}
	const char	*message=ORA_TRANSACTION_FAILED_MESSAGE;
	uint32_t	messagesize=
			charstring::getLength(ORA_TRANSACTION_FAILED_MESSAGE);
	if (errorsize && errorstring) {
		message=errorstring;
		messagesize=errorsize;
	}

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("transaction error");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("error: %lld",(long long)errnum);
	debugWrite("ora number sent: %u",oranum);
	debugWrite("%.*s",(int)messagesize,message);
	debugEnd();

	if (query3session) {
		putSummary(cursorid,oranum,0,message,messagesize);
	} else {
		putError(message,messagesize,oranum);
		putGenericFooter();
	}

	return sendPacket(true);
}

bool sqlrprotocol_oracle::version(const byte_t *rp, bool istticall) {

	const byte_t	*end=resppacket+resppacketsize;

	byte_t		seqnumber=0;
	byte_t		rdbmsversion=0;
	uint32_t	bufferlength=0;
	byte_t		returnversionlength=0;
	byte_t		returnversionnumber=0;

	if (end-rp<1) {
		debugWrite("truncated version sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	debugStart("version request");
	debugWrite("seq number: %d",seqnumber);

	// this handler answers both TTI_VERSION and a bare (non-piggybacked)
	// TTI_SWITCH_SESSION, and only the sequence number above is known to
	// be common to both - so a short body here just means fewer fields
	// to print, not a truncated request
	if (end-rp>=1) {
		read(rp,&rdbmsversion,&rp);
		debugWrite("rdbms version: %d",rdbmsversion);
	}
	if (readLenPreInt(rp,end,&bufferlength,&rp)) {
		debugWrite("buffer length: %d",bufferlength);
	}
	if (end-rp>=1) {
		read(rp,&returnversionlength,&rp);
		debugWrite("return version length: %d",returnversionlength);
	}
	if (end-rp>=1) {
		read(rp,&returnversionnumber,&rp);
		debugWrite("return version number: %d",returnversionnumber);
	}
	debugEnd();

	// only a genuine tti version call states the size of the buffer the
	// client passed in - a bare switch session reuses this response, but
	// its body has a different shape, so the bytes read above aren't a
	// buffer length.  0, or absent, means don't cap the banner.
	return sendVersionResponse((istticall)?bufferlength:0);
}

bool sqlrprotocol_oracle::sendVersionResponse(uint32_t bufferlength) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;
	byte_t		statusttccode=TTC_STATUS;
	uint32_t	callstatus=1;
	uint32_t	endtoendseqnumber=0;

	// don't overrun the buffer the client passed in
	uint32_t	bannerlength=
			(uint32_t)charstring::getLength(serverversionbanner);
	if (bufferlength && bufferlength<bannerlength) {
		bannerlength=bufferlength;
	}

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	// a capture of a real 12.2 server answering an oci client settles the
	// three fields below: the banner is a dalc - an lpi total size, then
	// the text as a clr - the packed version number after it is an lpi
	// rather than a bare ub4, and the response ends with the same status
	// message a logoff gets
	// see "Oracle Wire Protocol - Version"
	putDalc(serverversionbanner,bannerlength);
	writeLenPreInt(&reqpacket,serverversionpacked);
	write(&reqpacket,statusttccode);
	writeLenPreInt(&reqpacket,callstatus);
	writeLenPreInt(&reqpacket,endtoendseqnumber);

	debugStart("version response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("server version:");
	debugWrite("%s",serverversionbanner);
	debugWrite("packed version: 0x%08x",serverversionpacked);
	debugTtcCode(statusttccode);
	debugWrite("call status: %d",callstatus);
	debugWrite("end to end seq number: %d",endtoendseqnumber);
	debugEnd();

	return sendPacket(true);
}

bool sqlrprotocol_oracle::occa(const byte_t *rp, const byte_t **rpout) {

	// the close-cursors piggyback, python-oracledb's
	// _write_close_cursors_piggyback().  a client packs it in front of
	// another call rather than sending it on its own, so the read has to
	// stop exactly where the body ends or the call behind it is misread.
	// there is no response.
	const byte_t	*end=resppacket+resppacketsize;

	byte_t		seqnumber=0;
	byte_t		pointer=0;
	uint32_t	cursorcount=0;

	if (end-rp<1) {
		debugWrite("truncated occa sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	if (!getPointer(rp,end,&pointer,&rp) ||
		!readLenPreInt(rp,end,&cursorcount,&rp)) {
		return false;
	}

	debugStart("occa");
	debugWrite("seq number: %d",seqnumber);
	debugWrite("cursor count: %d",cursorcount);

	for (uint32_t i=0; i<cursorcount; i++) {

		uint32_t	cursorid=0;
		if (!readLenPreInt(rp,end,&cursorid,&rp)) {
			debugEnd();
			return false;
		}
		debugWrite("cursor id: %d",cursorid);

		// the ids on the wire are the controller's plus 1
		if (!cursorid) {
			continue;
		}
		sqlrservercursor	*cursor=
				cont->getCursor((uint16_t)(cursorid-1));
		if (!cursor) {
			debugWrite("cursor id %d not found",cursorid);
			continue;
		}
		uint16_t	closingid=cont->getId(cursor);
		forgetRefCursor(closingid);
		clearParams(cursor);
		cont->abort(cursor);
		cont->release(cursor);
		pendingrow[closingid].clear();
		clearLobPin(closingid);
		if (lastcursorid==closingid) {
			lastcursorid=65535;
		}
	}

	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::switchSession(const byte_t *rp,
						const byte_t **rpout) {

	// the session switch piggyback, which an OCI client packs in front of
	// every call, naming the session it is for.  SQL Relay has one session
	// per connection, so the fields are read only to stay in step - but
	// the read still has to stop exactly where the body ends or the call
	// behind it is lost.  there is no response.
	const byte_t	*end=resppacket+resppacketsize;

	byte_t		seqnumber=0;
	uint32_t	sessionid=0;
	uint32_t	serialnumber=0;
	// unexplained.  see "Oracle Wire Protocol - Switch Session"
	uint32_t	unknown=0;

	if (end-rp<1) {
		debugWrite("truncated switch session sequence number");
		return false;
	}
	read(rp,&seqnumber,&rp);

	if (!readLenPreInt(rp,end,&sessionid,&rp) ||
		!readLenPreInt(rp,end,&serialnumber,&rp) ||
		!readLenPreInt(rp,end,&unknown,&rp)) {
		return false;
	}

	debugStart("switch session");
	debugWrite("seq number: %d",seqnumber);
	debugWrite("session id: %d",sessionid);
	debugWrite("serial number: %d",serialnumber);
	debugWrite("unknown: 0x%08x",unknown);
	debugEnd();

	*rpout=rp;

	return true;
}

bool sqlrprotocol_oracle::unidentified54(const byte_t *rp) {

	// no idea

	// FIXME: decode this...
	// see "Oracle Wire Protocol - The Unidentified 0x54 Call"

	debugStart("unidentified 0x54 request");
	if (rp && rp<resppacket+resppacketsize) {
		debugWrite("seq number: 0x%02x",*rp);
	}
	debugEnd();

	return sendUnidentified54Response();
}

bool sqlrprotocol_oracle::sendUnidentified54Response() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;

	// no idea
	// see "Oracle Wire Protocol - The Unidentified 0x54 Call"
	byte_t	unknown[]={
		0x0C,
		0x00, 0x00, 0x00, 0x67,
		0x70, 0x00,
		0x00, 0x00, 0x00, 0x09
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	reqpacket.append(unknown,sizeof(unknown));

	debugStart("unidentified 0x54 response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugHexDump(unknown,sizeof(unknown));
	debugEnd();

	return sendPacket(true);
}


void sqlrprotocol_oracle::putGenericFooter() {

	// no idea...
	// see "Oracle Wire Protocol - Known Unknowns"

	// not on the query path - a 10g-or-later client's query goes through
	// query3() and fetch3(), which build a summary object field by field,
	// and none of sendQueryResponse(), sendQuery2Response() and
	// sendFetchResponse() is reachable from one.  so CCAP_TTC1 and
	// CCAP_OCI1 bit 0x01 don't apply to it: the two fields those bits
	// promise live at the front of a summary object, and this is not one -
	// it parses as no field sequence any client reads, and two of its
	// bytes look like an 8i server's pointers.  it stays as it is, for the
	// 8.0.5 and 8i paths, which have no client on this host to check it
	// against.

	// 8i server sends this to 8i client
	//
	// 8.0.5 server doesn't send it to 8.0.5 client,
	// but 8.0.5 client tolerates it
	byte_t	footer[]={
		0x00,
		0x36, 0x01, 0x00, 0x00, 0xA0, 0x0D, 0x6C, 0x09,
		0xC0, 0x54, 0x6C, 0x09, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	reqpacket.append(footer,sizeof(footer));

	debugStart("footer");
	debugHexDump(footer,sizeof(footer));
	debugEnd();
}

bool sqlrprotocol_oracle::sendQueryError(sqlrservercursor *cursor) {

	// get the error the failed prepare/execute left on the cursor
	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(cursor,
			&errorstring,
			&errorsize,
			&errnum,
			&liveconnection);

	// the backend's error number isn't necessarily a real, in-range ora
	// number - it may be another backend's native code (positive or
	// negative), or 0 (some filters set no error number at all).  0
	// specifically can't be sent as-is: putSummary() reads an oranum of
	// 0 as success.  fall back to a generic ora number - and, since the
	// backend can likewise leave no message, a generic message - rather
	// than send something a client would misread or that wouldn't fit
	// putError()'s ub2 number and ub1 message length.  the modern path's
	// summary object writes an lpi number and a clr message, neither of
	// which is capped that way, but sendQueryError() feeds both paths.
	uint32_t	oranum=ORA_QUERY_FAILED;
	if (errnum>0 && errnum<=65535) {
		oranum=(uint32_t)errnum;
	}
	const char	*message=ORA_QUERY_FAILED_MESSAGE;
	uint32_t	messagesize=charstring::getLength(ORA_QUERY_FAILED_MESSAGE);
	if (errorsize && errorstring) {
		message=errorstring;
		messagesize=errorsize;
	}

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("query error");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("error: %lld",(long long)errnum);
	debugWrite("ora number sent: %u",oranum);
	debugWrite("%.*s",(int)messagesize,message);
	debugEnd();

	// a query3 session gets a summary object, like a fetch does; an
	// older session gets putError()'s capture, like a fetch does
	if (query3session) {
		putSummary(cont->getId(cursor)+1,oranum,
					rowssent[cont->getId(cursor)],
					message,messagesize);
	} else {
		putError(message,messagesize,oranum);
		putGenericFooter();
	}

	return sendPacket(true);
}

// answers an execute that the statement's placeholders weren't all bound
// for.  only the query3 path can see it - the descriptor flag that says so
// is part of that request's bind section
bool sqlrprotocol_oracle::sendNotAllVariablesBoundError(uint32_t cursorid) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("not all variables bound error");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("cursor id: %d",cursorid);
	debugEnd();

	putSummary(cursorid,ORA_NOT_ALL_VARIABLES_BOUND,0,
				ORA_NOT_ALL_VARIABLES_BOUND_MESSAGE);

	return sendPacket(true);
}

bool sqlrprotocol_oracle::sendCursorNotOpenError(uint32_t cursorid) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("cursor not open error");
	debugWrite("data flags: 0x%04x",dataflags);
	debugWrite("cursor id: %d",cursorid);
	debugEnd();

	if (query3session) {
		putSummary(cursorid,ORA_INVALID_CURSOR,0,
					ORA_INVALID_CURSOR_MESSAGE);
	} else {
		putError("ORA-01001: invalid cursor",ORA_INVALID_CURSOR);
		putGenericFooter();
	}

	return sendPacket(true);
}

// what completes the call a client's marker packet interrupted - a real
// server answers a break/reset with ora-01013 for whatever was in flight,
// and the client is waiting to read that, not another marker, before it
// will send anything else (see #9591)
bool sqlrprotocol_oracle::sendMarkerCancelError() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("marker cancel error");
	debugWrite("data flags: 0x%04x",dataflags);
	debugEnd();

	if (query3session) {
		putSummary(0,ORA_USER_REQUESTED_CANCEL,0,
					ORA_USER_REQUESTED_CANCEL_MESSAGE);
	} else {
		putError(ORA_USER_REQUESTED_CANCEL_MESSAGE,
					ORA_USER_REQUESTED_CANCEL);
		putGenericFooter();
	}

	return sendPacket(true);
}

// what a tti function this module doesn't implement (or doesn't recognize
// at all) gets back, instead of the caller dropping the session - keeps
// the client connected so the rest of the session can still run
bool sqlrprotocol_oracle::sendUnimplementedFunctionError() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	debugStart("unimplemented function error");
	debugWrite("data flags: 0x%04x",dataflags);
	debugEnd();

	if (query3session) {
		putSummary(0,ORA_UNIMPLEMENTED_FEATURE,0,
					ORA_UNIMPLEMENTED_FEATURE_MESSAGE);
	} else {
		putError(ORA_UNIMPLEMENTED_FEATURE_MESSAGE,
					ORA_UNIMPLEMENTED_FEATURE);
		putGenericFooter();
	}

	return sendPacket(true);
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrprotocol	*new_sqlrprotocol_oracle(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrprotocol_oracle(cont,parameters);
	}
}
