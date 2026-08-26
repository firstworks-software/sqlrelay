// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/character.h>
#include <rudiments/regularexpression.h>
#include <rudiments/wcharstring.h>
#include <rudiments/iconvert.h>
#include <rudiments/process.h>
#include <rudiments/sys.h>
#include <rudiments/datetime.h>
#include <rudiments/error.h>
#include <rudiments/csprng.h>
#include <rudiments/dynamiclib.h>
#include <rudiments/environment.h>

#include <datatypes.h>


// TDS protocol definitions

// message types
#define SQL_BATCH			0x01
#define PRE_TDS7_LOGIN			0x02
#define RPC				0x03
#define TABULAR_RESULT			0x04
#define ATTENTION_SIGNAL		0x06
#define BULK_LOAD_DATA			0x07
#define FEDERATED_AUTHENTICATION_TOKEN	0x08
#define TRANSACTION_MANAGER_REQUEST	0x0E
// tds 5.0's "normal" buffer.  it carries token-framed requests - language
// commands, rpc's, dynamic sql, cursors - rather than the bare payload that
// each of the types above carries.
#define PRE_TDS7_NORMAL			0x0F
#define TDS7_LOGIN			0x10
#define SSPI				0x11
#define PRE_LOGIN			0x12

// response tokens
#define TOKEN_LOGIN_ACK			0xAD
#define TOKEN_COLMETADATA		0x81
#define TOKEN_ROW			0xD1
#define TOKEN_ENV_CHANGE		0xE3
#define TOKEN_INFO			0xAB
#define TOKEN_ERROR			0xAA
#define TOKEN_DONE			0xFD
#define TOKEN_DONEPROC			0xFE
#define TOKEN_DONEINPROC		0xFF
#define TOKEN_RETURNSTATUS		0x79
#define TOKEN_RETURNVALUE		0xAC
// The tds 5.0 counterpart of TOKEN_COLMETADATA - what describes the
// columns of a result set in that dialect.  They can't share a token
// byte, and not just because their contents differ: 0x81 is
// TDS5_TOKEN_CURDELETE in tds 5.0, so a tds 5.0 client reading an 0x81
// would take it for a cursor-delete request rather than metadata.
#define TOKEN_ROWFMT			0xEE

// Tds 5.0 request tokens - what a client can send inside a
// PRE_TDS7_NORMAL buffer.  Only TDS5_TOKEN_LANGUAGE is implemented; the
// rest are defined so that preTds7TokenLength() can name what it's
// refusing, and so that a later ticket adding one has the value already.
//
// These live in the request direction only.  Some of the values mean
// something else in the other direction, or in tds 7.x - 0x81 is
// TDS5_TOKEN_CURDELETE here but TOKEN_COLMETADATA there - and some,
// like paramfmt, travel both ways.  So don't fold these into one table
// with the response tokens above.
#define	TDS5_TOKEN_CURDECLARE3		0x10
#define	TDS5_TOKEN_PARAMFMT2		0x20
#define	TDS5_TOKEN_LANGUAGE		0x21
#define	TDS5_TOKEN_ORDERBY2		0x22
#define	TDS5_TOKEN_CURDECLARE2		0x23
#define	TDS5_TOKEN_ROWFMT2		0x61
#define	TDS5_TOKEN_OPTIONCMD2		0x63
#define	TDS5_TOKEN_MSG			0x65
#define	TDS5_TOKEN_LOGOUT		0x71
#define	TDS5_TOKEN_CURCLOSE		0x80
#define	TDS5_TOKEN_CURDELETE		0x81
#define	TDS5_TOKEN_CURFETCH		0x82
#define	TDS5_TOKEN_CURINFO		0x83
#define	TDS5_TOKEN_CUROPEN		0x84
#define	TDS5_TOKEN_CURUPDATE		0x85
#define	TDS5_TOKEN_CURDECLARE		0x86
#define	TDS5_TOKEN_CURINFO2		0x87
#define	TDS5_TOKEN_CURINFO3		0x88
#define	TDS5_TOKEN_DYNAMIC2		0xA3
#define	TDS5_TOKEN_OPTIONCMD		0xA6
#define	TDS5_TOKEN_KEY			0xCA
#define	TDS5_TOKEN_ROW			0xD1
#define	TDS5_TOKEN_PARAMS		0xD7
#define	TDS5_TOKEN_CAPABILITY		0xE2
#define	TDS5_TOKEN_DBRPC		0xE6
#define	TDS5_TOKEN_DYNAMIC		0xE7
#define	TDS5_TOKEN_DBRPC2		0xE8
#define	TDS5_TOKEN_PARAMFMT		0xEC

// How long a tds 5.0 request token's length field is.  There's no rule
// that derives this from the token byte - the tds 7.x "token&0x30"
// classification gets LANGUAGE and MSG wrong - so it's a table, and a
// token that isn't in it can't be skipped at all.  Both freetds and the
// wireshark dissector do the same thing, and freetds treats a token it
// doesn't know as fatal (src/tds/token.c, "Bad token from the server").
#define	TDS5_LENSIZE_BYTE		1
#define	TDS5_LENSIZE_USHORT		2
#define	TDS5_LENSIZE_UINT		4
// no length field, and a payload of one fixed byte rather than none -
// don't confuse this with a length of 0
#define	TDS5_LENSIZE_FIXED1		0xFE
// the token carries no length and can only be sized by replaying the
// paramfmt or rowfmt before it, so the walker can't step over one
#define	TDS5_LENSIZE_UNKNOWN		0xFF

// the tds 5.0 language token's status byte.  the only bit defined is
// "parameters follow", as a paramfmt/params pair.
#define	TDS5_LANGUAGE_PARAMS		0x01

// the tds 5.0 msg token's status byte, and how long the token's body is
// when it carries nothing but a status and a msgid, which is all the
// three messages below carry
#define	TDS5_MSG_HASARGS		0x01
#define	TDS5_MSG_SIZE			3

// The msg id's of the tds 5.0 encrypted-password exchange.  The server
// sends sec_encrypt with the key it chose; the client answers with
// sec_logpwd, and with sec_rempwd as well when it has a remote password.
#define	TDS5_MSG_SEC_ENCRYPT		0x0001
#define	TDS5_MSG_SEC_LOGPWD		0x0002
#define	TDS5_MSG_SEC_REMPWD		0x0003

// the tds 5.0 dbrpc token's option flags.  "recompile" was never seen on
// the wire; a real ct-lib client sends "params" whenever it has any and
// 0 when it hasn't.
#define	TDS5_RPC_RECOMPILE		0x0001
#define	TDS5_RPC_PARAMS			0x0002

// The tds 5.0 dynamic token's operation type.  0x20 is the server's
// answer to all four of the others; the rest are what a client sends.
// Procname, describe-input and describe-output were never seen on the
// wire from any client - ct-lib answers both describes out of the
// formats that came back with the prepare - but they're implemented
// anyway, since nothing stops a client from sending one.
#define	TDS5_DYN_PREPARE		0x01
#define	TDS5_DYN_EXEC			0x02
#define	TDS5_DYN_DEALLOC		0x04
#define	TDS5_DYN_EXEC_IMMEDIATE		0x08
#define	TDS5_DYN_PROCNAME		0x10
#define	TDS5_DYN_ACK			0x20
#define	TDS5_DYN_DESCRIBE_INPUT		0x40
#define	TDS5_DYN_DESCRIBE_OUTPUT	0x80

// The tds 5.0 dynamic token's status byte.  "suppress fmt" is advisory -
// a real ase re-sends the formats whether or not the client asked it to,
// so this module ignores the bit rather than acting on it.
#define	TDS5_DYN_HASARGS		0x01
#define	TDS5_DYN_SUPPRESS_FMT		0x02

// How many dynamic sql statement ids one session can name at once.  Each
// one holds a cursor, so the real ceiling is how many cursors the
// session has, and this only keeps a client that prepares under a fresh
// id forever from growing the map without bound.
#define	MAX_DYNAMIC_IDS			1024

// packet header size, and the smallest, default, and largest packet sizes
// (the size on the wire is 16 bits and includes the header)
#define	PACKET_HEADER_SIZE		8
#define	MIN_PACKET_SIZE			512
#define	DEFAULT_PACKET_SIZE		4096
#define	MAX_PACKET_SIZE			65535

// a floor under maxrequestsize, the ceiling recvPacket() puts on a
// reassembled request.  maxquerysize can't cap this, since it bounds the
// query text rather than the whole request.
#define	MIN_MAX_REQUEST_SIZE		(16*1024*1024)

// A ceiling on how many commands one request buffer may carry.  Neither
// maxquerysize nor maxrequestsize bounds this - the first bounds a single
// command's sql and the second bounds the buffer, and a 16mb buffer packed
// with 6-byte language tokens is millions of commands, each a backend
// round trip whose result is held in memory until the whole response is
// sent.  Well clear of any real batch: a ct-lib client typically sends one
// language command per ct_send, and an rpc batch from a driver's parameter
// arrays runs to the low hundreds before the driver chunks it.
#define	MAX_COMMANDS_PER_REQUEST	1024

// login7's fixed header, before the variable length fields it points into.
// tds 7.2 and up add ibchangepassword, cchchangepassword and cbsspilong.
#define	LOGIN7_HEADER_SIZE		86
#define	LOGIN7_HEADER_SIZE_72		94

// Where login7's fixed header declares its version and its two password
// fields.  tds7Login() finds every field by walking the header, but these
// three have to be located before the record is parsed at all, so that the
// received-packet dump can blank the passwords.  Each ib is followed
// immediately by its cch.
#define	LOGIN7_TDSVERSION_OFFSET	4
#define	LOGIN7_IBPASSWORD_OFFSET	44
#define	LOGIN7_IBCHANGEPASSWORD_OFFSET	86

// the longest each login7 field may be, in characters for the ucs-2 fields
// and in bytes for the rest
#define	MAX_LOGIN_CHARS			128
#define	MAX_LOGIN_EXTENSION_BYTES	255
#define	MAX_LOGIN_ATCHDBFILE_CHARS	260
// FIXME: cbsspilong exists for kerberos tickets larger than this, which is
// exactly what this module can't service yet - revisit when sspi is
// implemented
#define	MAX_LOGIN_SSPI_BYTES		65535

// The pre-tds7 login record is entirely fixed-length, unlike login7.
// Tds 4.2 and 5.0 lay it out at different sizes (572 and 568 bytes), but
// the record also carries the client's dialect in 4 bytes of its own, and
// that's what this module goes by, so only the 5.0 size is defined here,
// as the floor that has to have arrived.  A capability token may follow
// the record.
#define	PRE_TDS7_LOGIN_SIZE		568

// each string field in the pre-tds7 login record is a fixed run of
// nul-padded bytes followed by a trailing byte giving how many of those
// bytes are real characters.  these are the sizes of the runs, the
// trailing byte isn't counted.
#define	PRE_TDS7_NAME_SIZE		30
#define	PRE_TDS7_REMOTE_PASSWORD_SIZE	255
#define	PRE_TDS7_PROGNAME_SIZE		10
#define	PRE_TDS7_PACKET_SIZE_SIZE	6

// Where the two cleartext credential fields start in the pre-tds7 login
// record.  Each preceding string field counts as its fixed run of bytes
// plus its trailing length byte: hostname and username put password at
// 62, and hostproc, the fixed-length block, appname and servername put
// remotepassword at 202.
#define	PRE_TDS7_PASSWORD_OFFSET	62
#define	PRE_TDS7_REMOTE_PASSWORD_OFFSET	202

// the fixed-length fields in the pre-tds7 login record
#define	PRE_TDS7_TYPE_FLAGS_SIZE	6
#define	PRE_TDS7_SPARE_SIZE		3
#define	PRE_TDS7_SESSION_ID_SIZE	6
#define	PRE_TDS7_SEC_SPARE_SIZE		2
#define	PRE_TDS7_DUMMY_SIZE		4

// Which byte of the login record's typeflags block declares what.  The
// block is where a pre-tds7 client says how it lays out multi-byte
// values, and a tds 5.0 token stream follows what it says - unlike tds
// 7.x, which the ms-tds spec fixes as little-endian.
#define	PRE_TDS7_TYPE_FLAGS_INT2	0
#define	PRE_TDS7_TYPE_FLAGS_INT4	1
#define	PRE_TDS7_TYPE_FLAGS_CHAR	2
#define	PRE_TDS7_TYPE_FLAGS_FLT		3
#define	PRE_TDS7_TYPE_FLAGS_DATE	4
#define	PRE_TDS7_TYPE_FLAGS_USEDB	5

// What each of those bytes can say.  These names and numbers are the tds
// 5.0 spec's - no header on this box declares them - and they match the
// le1[] array freetds's login.c fills in.  A live capture of a real
// ct-lib client on x86 sends 03 01 06 0a 09 01, which is every "_LO"
// value below.
//
// "LSB_LO" means the least significant byte comes first (little-endian);
// "LSB_HI" means it comes last (big-endian).  The char byte names a
// character set family rather than a byte order, so it says nothing
// about how values are laid out - it's decoded only for the debug
// output.  The last byte is a usedb/notify flag, unrelated to either.
#define	TDS_INT2_LSB_HI			2
#define	TDS_INT2_LSB_LO			3
#define	TDS_INT4_LSB_HI			0
#define	TDS_INT4_LSB_LO			1
#define	TDS_CHAR_ASCII			6
#define	TDS_CHAR_EBCDIC			7
#define	TDS_FLT_IEEE_HI			4
#define	TDS_FLT_VAX_D			5
#define	TDS_FLT_IEEE_LO			10
#define	TDS_FLT_ND5000			11
#define	TDS_TWO_I4_LSB_HI		8
#define	TDS_TWO_I4_LSB_LO		9

// what preTds7ByteOrder() decoded one of those bytes into
#define	PRE_TDS7_ORDER_LE		0
#define	PRE_TDS7_ORDER_BE		1
#define	PRE_TDS7_ORDER_UNKNOWN		2

// seclogin bits in the pre-tds7 login record.  When any of these are set,
// the client leaves the password fields empty and waits for the server to
// drive a challenge/response exchange instead.
#define	PRE_TDS7_SEC_LOG_ENCRYPT	0x01
#define	PRE_TDS7_SEC_LOG_CHALLENGE	0x02
#define	PRE_TDS7_SEC_LOG_ENCRYPT2	0x20
#define	PRE_TDS7_SEC_LOG_ENCRYPT3	0x80
#define	PRE_TDS7_SEC_LOG_ENCRYPT_MASK	(PRE_TDS7_SEC_LOG_ENCRYPT| \
					PRE_TDS7_SEC_LOG_CHALLENGE| \
					PRE_TDS7_SEC_LOG_ENCRYPT2| \
					PRE_TDS7_SEC_LOG_ENCRYPT3)

// The sizes of the encrypted-password exchange: the key the server
// chooses, and the blob the client answers with - 32 bytes of ciphertext
// and a trailing byte giving how long the password inside them is.  The
// cipher clamps a password to 30 bytes, which is also as long as the
// cleartext field in the login record.
#define	SEC_ENCRYPT_KEY_SIZE		8
#define	SEC_ENCRYPT_BLOB_SIZE		33
#define	SEC_ENCRYPT_MAX_PASSWORD	30

// what a real ase declares the sec_encrypt key parameter's usertype as
#define	SEC_ENCRYPT_USERTYPE		37

// where the cipher itself comes from - see secEncryptDecryptPassword()
#define	SEC_ENCRYPT_LIB			"libsybcomn64.so"
#define	SEC_ENCRYPT_LIB_DIR		"/OCS-16_0/lib/"
#define	SEC_ENCRYPT_SYMBOL		"com__string_uninitialize"

// login-time capability token, and the capability types it carries
#define	TOKEN_CAPABILITY		0xE2
#define	CAPABILITY_REQUEST		0x01
#define	CAPABILITY_RESPONSE		0x02

// a capability mask's length is a single byte, so this is a ceiling
// rather than a policy
#define	MAX_CAPABILITY_MASK_BYTES	255

// Tds 5.0 capability numbers, as capability() and the bit helpers under
// it use them.
//
// The numbers below are freetds's wire bit positions (its enum_cap.h),
// confirmed against a live ase 16.0 capture: the bits ase itself sets
// for date/time, interval, unitext and sint1 line up with freetds's
// numbering, not with sap's own cspublic.h, whose CS_* values are
// ct_capability() api ids rather than wire positions - past
// CS_OPTION_GET=51 an api id is one higher than the wire bit it
// translates to, so cspublic.h cannot be read as this list directly.
// The response side is not in dispute - both headers agree there.
//
// A capability this module does NOT support is simply left out of
// capability()'s tables below, which clears its bit regardless of which
// scheme a reader has in mind (e.g. wide tables/columnstatus, whose
// wire position differs by one between the two schemes, are absent
// either way).
#define	TDS5_CAP_REQ_LANG		1
#define	TDS5_CAP_REQ_RPC		2
#define	TDS5_CAP_REQ_BCP		5
#define	TDS5_CAP_REQ_CURSOR		6
#define	TDS5_CAP_REQ_DYN		7
#define	TDS5_CAP_REQ_MSG		8
#define	TDS5_CAP_REQ_PARAM		9
#define	TDS5_CAP_REQ_DATA_INT1		10
#define	TDS5_CAP_REQ_DATA_INT2		11
#define	TDS5_CAP_REQ_DATA_INT4		12
#define	TDS5_CAP_REQ_DATA_BIT		13
#define	TDS5_CAP_REQ_DATA_CHAR		14
#define	TDS5_CAP_REQ_DATA_VCHAR		15
#define	TDS5_CAP_REQ_DATA_BIN		16
#define	TDS5_CAP_REQ_DATA_VBIN		17
#define	TDS5_CAP_REQ_DATA_MNY8		18
#define	TDS5_CAP_REQ_DATA_MNY4		19
#define	TDS5_CAP_REQ_DATA_DATE8		20
#define	TDS5_CAP_REQ_DATA_DATE4		21
#define	TDS5_CAP_REQ_DATA_FLT4		22
#define	TDS5_CAP_REQ_DATA_FLT8		23
#define	TDS5_CAP_REQ_DATA_NUM		24
#define	TDS5_CAP_REQ_DATA_TEXT		25
#define	TDS5_CAP_REQ_DATA_IMAGE		26
#define	TDS5_CAP_REQ_DATA_DEC		27
#define	TDS5_CAP_REQ_DATA_LCHAR		28
#define	TDS5_CAP_REQ_DATA_LBIN		29
#define	TDS5_CAP_REQ_DATA_INTN		30
#define	TDS5_CAP_REQ_DATA_DATETIMEN	31
#define	TDS5_CAP_REQ_DATA_MONEYN	32
#define	TDS5_CAP_REQ_CSR_PREV		33
#define	TDS5_CAP_REQ_CSR_FIRST		34
#define	TDS5_CAP_REQ_CSR_LAST		35
#define	TDS5_CAP_REQ_CSR_ABS		36
#define	TDS5_CAP_REQ_CSR_REL		37
#define	TDS5_CAP_REQ_CSR_MULTI		38
#define	TDS5_CAP_REQ_CON_INBAND		40
#define	TDS5_CAP_REQ_PROTO_BULK		43
#define	TDS5_CAP_REQ_DATA_SENSITIVITY	45
#define	TDS5_CAP_REQ_DATA_BOUNDARY	46
#define	TDS5_CAP_REQ_PROTO_DYNPROC	48
#define	TDS5_CAP_REQ_DATA_FLTN		49
#define	TDS5_CAP_REQ_DATA_INT8		51
#define	TDS5_CAP_REQ_DOL_BULK		53
#define	TDS5_CAP_REQ_DATA_COLUMNSTATUS	58
#define	TDS5_CAP_REQ_WIDETABLE		59
#define	TDS5_CAP_REQ_SRVPKTSIZE		79

// The response mask is inverted - a bit means "don't send me this" -
// except for the SUPPRESS_ ones, which mean "you may leave this out".
#define	TDS5_CAP_RES_NOTDSDEBUG		33
#define	TDS5_CAP_RES_DATA_NOINT8	35
#define	TDS5_CAP_RES_DATA_NOCOLUMNSTATUS	38
#define	TDS5_CAP_RES_NO_WIDETABLES	45
#define	TDS5_CAP_RES_SUPPRESS_FMT	62
#define	TDS5_CAP_RES_NO_TDSCONTROL	67

// In a tds 7.x login ack, the byte after the token size says which sql
// interface the server speaks (SQL_DFLT/SQL_TSQL).  In a tds 4.2/5.0
// login ack the same byte says how the login came out instead, and 4.2
// and 5.0 don't spell it the same way (4.2 says 1, 5.0 says 5) - which
// is one reason preTds7Login() refuses a client that declares 4.2.
// A failed pre-tds7 login gets an error token and a login ack carrying
// FAIL, which is what a real ase sends.  NEGOTIATE opens the encrypted-
// password exchange - see preTds7SecEncryptLogin().
#define	PRE_TDS7_LOGIN_ACK_SUCCEED	0x05
#define	PRE_TDS7_LOGIN_ACK_FAIL		0x06
#define	PRE_TDS7_LOGIN_ACK_NEGOTIATE	0x07

// What the login ack reports as the server program, for pre-tds7 clients.
// ct-lib decides sybase-vs-mssql from the product version's high bit, so
// keep it clear, and report a modern ase - 16.0.0 here - since older
// versions send ct-lib down compatibility paths this module doesn't
// implement.
#define	PRE_TDS7_LOGIN_ACK_PROGNAME	"ASE"
#define	PRE_TDS7_LOGIN_ACK_MAJORVER	0x10
#define	PRE_TDS7_LOGIN_ACK_MINORVER	0x00
#define	PRE_TDS7_LOGIN_ACK_BUILDNUMHI	0x00
#define	PRE_TDS7_LOGIN_ACK_BUILDNUMLOW	0x00

// status bitmap
#define	STATUS_NORMAL			0x00
#define	STATUS_EOM			0x01
#define	STATUS_IGNORE			0x02
#define	STATUS_RESETCONNECTION		0x08
#define	STATUS_RESETCONNECTIONSKIPTRAN	0x10

// pre-login option token
#define	PL_VERSION			0x00
#define	PL_ENCRYPTION			0x01
#define	PL_INSTOPT			0x02
#define	PL_THREADID			0x03
#define	PL_MARS				0x04
#define	PL_TRACEID			0x05
#define	PL_FEDAUTHREQUIRED		0x06
#define	PL_NONCEOPT			0x07
#define	PL_TERMINATOR			0xFF

// encryption options
#define ENCRYPT_OFF	0x00
#define ENCRYPT_ON	0x01
#define ENCRYPT_NOT_SUP	0x02
#define ENCRYPT_REQ	0x03

// byte order
#define	ORDER_X86	0x00
#define	ORDER_68000	0x01

// character set
#define	CHARSET_ASCII	0x00
#define	CHARSET_EBDDIC	0x01

// floating point type
#define	FLOAT_IEEE_754	0x00
#define	FLOAT_VAX	0x01
#define	FLOAT_ND5000	0x02

// dump/load
#define DUMPLOAD_ON	0x00
#define DUMPLOAD_OFF	0x01

// warn when using db
#define	USE_DB_WARN_OFF	0x00
#define	USE_DB_WARN_ON	0x01

// use db flag
#define USE_DB_WARN	0x00
#define USE_DB_FATAL	0x01

// warn when setting language
#define	SET_LANG_WARN_OFF	0x00
#define	SET_LANG_WARN_ON	0x01

// set language flag
#define	SET_LANG_WARN	0x00
#define	SET_LANG_FATAL	0x01

// odbc
#define ODBC_OFF	0x00
#define ODBC_ON		0x01

// user type
#define	USER_NORMAL	0x00
#define	USER_SERVER	0x01
#define	USER_REMUSER	0x02
#define	USER_SQLREPL	0x03

// integrated security
#define	INTEGRATED_SECURITY_OFF	0x00
#define	INTEGRATED_SECURITY_ON	0x01

// sql type
#define SQL_DFLT	0x00
#define SQL_TSQL	0x01

// oledb
#define OLEDB_OFF	0x00
#define OLEDB_ON	0x01

// the collation a real sql server running SQL_Latin1_General_CP1_CI_AS
// sends.  no back end reports a sql server collation, so this is hard
// coded, but it has to agree with TDS_NONUNICODE_CHARSET below.
#define TDS_COLLATION_LCID	0x00D00409
#define TDS_COLLATION_SORTID	0x34

// character encodings used when moving character data between the client
// and the back end
// (utf-16le rather than the spec's ucs-2, because real clients send
// surrogate pairs outside the bmp.  //TRANSLIT makes iconv substitute a
// '?' for a character cp1252 has no form for, like a real sql server)
#define TDS_UNICODE_CHARSET	"UTF-16LE"
#define TDS_NONUNICODE_CHARSET	"CP1252//TRANSLIT"
#define TDS_BACKEND_CHARSET	"UTF-8"

// The charsets a pre-tds7 client is allowed to name in its login record,
// and the iconv encodings each one maps to.  "outenc" is "inenc" with
// //TRANSLIT, so that a character the client's charset has no form for
// comes out as a substitute rather than failing the whole conversion,
// the way TDS_NONUNICODE_CHARSET does it on the tds 7.x path.
//
// A NULL encoding means "no conversion" - utf8 is what this module
// already speaks internally - and so does a name that isn't in the table
// at all.  Both leave the bytes passing through untouched, which is what
// every pre-tds7 session did before the charset field was honored.
//
// The table is deliberately restricted to single-byte charsets plus
// utf8.  preTds7Field() sizes a character value in bytes against the
// column width the rowfmt already declared - charSize() is 1 on this
// path - so a value that fits the declared width in one encoding can
// overflow it in another.  A single-byte target keeps the existing size
// cap safe even though //TRANSLIT can still substitute a multi-byte
// utf-8 sequence with several single-byte characters (e.g. one utf-8
// fraction character can expand to "1/2") - the cap can only ever cut
// at a character boundary, since every character is one byte wide, so
// truncation loses whole characters rather than corrupting one.
// Converting into a multi-byte encoding would let that same cap cut a
// value off mid-character and silently corrupt it, so no multi-byte
// charset belongs here without a way to widen the declared column size
// first.
struct pretds7charset {
	const char	*name;
	const char	*inenc;
	const char	*outenc;
};

static const pretds7charset	pretds7charsets[]={
	{"utf8",NULL,NULL},
	{"iso_1","ISO-8859-1","ISO-8859-1//TRANSLIT"},
	{"iso15","ISO-8859-15","ISO-8859-15//TRANSLIT"},
	{"ascii_8","US-ASCII","US-ASCII//TRANSLIT"},
	{"cp437","CP437","CP437//TRANSLIT"},
	{"cp850","CP850","CP850//TRANSLIT"},
	{"cp1250","CP1250","CP1250//TRANSLIT"},
	{"cp1251","CP1251","CP1251//TRANSLIT"},
	{"cp1252","CP1252","CP1252//TRANSLIT"},
	{"mac","MACINTOSH","MACINTOSH//TRANSLIT"},
	{"roman8","HP-ROMAN8","HP-ROMAN8//TRANSLIT"},
	{NULL,NULL,NULL}
};

// envchange types
#define ENV_CHANGE_DATABASE					1
#define ENV_CHANGE_LANGUAGE					2
#define ENV_CHANGE_CHARSET					3
#define ENV_CHANGE_PACKET_SIZE					4
#define ENV_CHANGE_UNICODE_DATA_SORTING_LOCAL_ID		5
#define ENV_CHANGE_UNICODE_DATA_SORTING_COMPARISON_FLAGS	6
#define ENV_CHANGE_SQL_COLLATION				7
#define ENV_CHANGE_BEGIN_TRANSACTION				8
#define ENV_CHANGE_COMMIT_TRANSACTION				9
#define ENV_CHANGE_ROLLBACK_TRANSACTION				10
#define ENV_CHANGE_ENLIST_DTC_TRANSACTION			11
#define ENV_CHANGE_DEFECT_TRANSACTION				12
#define ENV_CHANGE_REAL_TIME_LOG_SHIPPING			13
#define ENV_CHANGE_PROMOTE_TRANSACTION				15
#define ENV_CHANGE_TRANSACTION_MANAGER_ADDRESS			16
#define ENV_CHANGE_TRANSACTION_ENDED				17
#define ENV_CHANGE_RESETCONNECTION_COMPLETION_ACKNOWLEDGEMENT	18
#define ENV_GET_USER_INSTANCE					19
#define ENV_GET_ROUTING_INFORMATION				20

// done statuses
#define DONE_FINAL	0x0000
#define DONE_MORE	0x0001
#define DONE_ERROR	0x0002
#define DONE_INXACT	0x0004
#define DONE_COUNT	0x0010
#define DONE_ATTN	0x0020
#define DONE_RPCINBATCH	0x0080
#define DONE_SRVERROR	0x0100

// marks the output bind that carries a procedure's return value rather
// than one of the client's parameters
#define RPC_RETURN_VALUE_PARAM	0xFFFF

// base for the handles sqlrelay mints itself (see newHandle()).  a client
// can get a small sequential handle straight from the backend instead, by
// running "exec sp_prepare ..." in a raw batch, so keeping the two ranges
// disjoint lets a stmthandles lookup miss mean "not one of ours".  low
// enough for the counter to climb to INT32_MAX, the widest a T-SQL int holds.
#define SQLRELAY_HANDLE_BASE	0x40000000

// stream headers
#define ALL_HEADERS_QUERY_NOTIFICATIONS		0x0001
#define ALL_HEADERS_TRANSACTION_DESCRIPTOR	0x0002
#define ALL_HEADERS_TRACE_ACTIVITY		0x0003

// data types
#define TDS_TYPE_NULL			0x1F	// NULL
#define TDS_TYPE_INT1			0x30	// TinyInt
#define TDS_TYPE_BIT			0x32	// Bit
#define TDS_TYPE_INT2			0x34	// SmallInt
#define TDS_TYPE_INT4			0x38	// Int
#define TDS_TYPE_DATETIM4		0x3A	// SmallDateTime
#define TDS_TYPE_FLT4			0x3B	// Real
#define TDS_TYPE_MONEY			0x3C	// Money
#define TDS_TYPE_DATETIME		0x3D	// DateTime
#define TDS_TYPE_FLT8			0x3E	// Float
#define TDS_TYPE_MONEY4			0x7A	// SmallMoney
#define TDS_TYPE_INT8			0x7F	// BigInt
#define TDS_TYPE_GUID			0x24	// UniqueIdentifier
#define TDS_TYPE_INTN			0x26	// Int (variable length)
#define TDS_TYPE_DECIMAL		0x37	// Decimal (legacy support)
#define TDS_TYPE_NUMERIC		0x3F	// Numeric (legacy support)
#define TDS_TYPE_BITN			0x68	// Bit (variable length)
#define TDS_TYPE_DECIMALN		0x6A	// Decimal
#define TDS_TYPE_NUMERICN		0x6C	// Numeric
#define TDS_TYPE_FLTN			0x6D	// Float (variable length)
#define TDS_TYPE_MONEYN			0x6E	// Money (variable length)
#define TDS_TYPE_DATETIMN		0x6F	// DateTime (variable length)
#define TDS_TYPE_DATEN			0x28	// (introduced in TDS 7.3)
#define TDS_TYPE_TIMEN			0x29	// (introduced in TDS 7.3)
#define TDS_TYPE_DATETIME2N		0x2A	// (introduced in TDS 7.3)
#define TDS_TYPE_DATETIMEOFFSETN	0x2B	// (introduced in TDS 7.3)
#define TDS_TYPE_CHAR			0x2F	// Char (legacy support)
#define TDS_TYPE_VARCHAR		0x27	// VarChar (legacy support)
#define TDS_TYPE_BINARY			0x2D	// Binary (legacy support)
#define TDS_TYPE_VARBINARY		0x25	// VarBinary (legacy support)
#define TDS_TYPE_BIGVARBIN		0xA5	// VarBinary
#define TDS_TYPE_BIGVARCHR		0xA7	// VarChar
#define TDS_TYPE_BIGBINARY		0xAD	// Binary
#define TDS_TYPE_BIGCHAR		0xAF	// Char
#define TDS_TYPE_NVARCHAR		0xE7	// NVarChar
#define TDS_TYPE_NCHAR			0xEF	// NChar
#define TDS_TYPE_LONGBINARY		0xE1	// LongBinary (sybase-specific)
#define TDS_TYPE_XML			0xF1	// XML
						// (introduced in TDS 7.2)
#define TDS_TYPE_UDT			0xF0	// CLR UDT
						// (introduced in TDS 7.2)
#define TDS_TYPE_TEXT			0x23	// Text
#define TDS_TYPE_IMAGE			0x22	// Image
#define TDS_TYPE_NTEXT			0x63	// NText
#define TDS_TYPE_SSVARIANT		0x62	// Sql_Variant
						// (introduced in TDS 7.2)
#define TDS_TYPE_TVP			0xF3	// Table Valued Parameter
						// (introduced in TDS 7.3)

// Tds 5.0 data types - the full set from the tds 5.0 datatype summary,
// whether or not anything sends one yet.
//
// A separate block from the TDS_TYPE_* values above, rather than more
// entries in it, because several of the bytes mean something different
// in the two dialects.  0x68 is BITN in ms-tds but TDS_BOUNDARY here,
// 0x24 is uniqueidentifier in ms-tds but TDS_BLOB here, 0xAF is BIGCHAR
// (a 2-byte length) in ms-tds but TDS_LONGCHAR (a 4-byte length) here,
// and 0x7F is a fixed-length INT8 in ms-tds but falls to a 1-byte varint
// here, where the 8-byte integer is 0xBF.  Same reasoning as the
// TDS5_TOKEN_* block above.
#define	TDS5_TYPE_VOID			0x1F	// Void (unknown)
#define	TDS5_TYPE_IMAGE			0x22	// Image
#define	TDS5_TYPE_TEXT			0x23	// Text
#define	TDS5_TYPE_BLOB			0x24	// Serialized Object
#define	TDS5_TYPE_VARBINARY		0x25	// Binary
#define	TDS5_TYPE_INTN			0x26	// Integer (variable length)
#define	TDS5_TYPE_VARCHAR		0x27	// Character
#define	TDS5_TYPE_BINARY		0x2D	// Binary (blank padded)
#define	TDS5_TYPE_INTERVAL		0x2E	// Time Interval
#define	TDS5_TYPE_CHAR			0x2F	// Character (blank padded)
#define	TDS5_TYPE_INT1			0x30	// Unsigned Integer
#define	TDS5_TYPE_DATE			0x31	// Date
#define	TDS5_TYPE_BIT			0x32	// Bit
#define	TDS5_TYPE_TIME			0x33	// Time
#define	TDS5_TYPE_INT2			0x34	// Integer
#define	TDS5_TYPE_INT4			0x38	// Integer
#define	TDS5_TYPE_SHORTDATE		0x3A	// Date/time (4 byte)
#define	TDS5_TYPE_FLT4			0x3B	// Float
#define	TDS5_TYPE_MONEY			0x3C	// Money
#define	TDS5_TYPE_DATETIME		0x3D	// Date/time
#define	TDS5_TYPE_FLT8			0x3E	// Float
#define	TDS5_TYPE_UINT1			0x40	// Unsigned Integer
						// (not in the summary table,
						// but freetds treats it as
						// fixed-length like its
						// wider siblings)
#define	TDS5_TYPE_UINT2			0x41	// Unsigned Integer
#define	TDS5_TYPE_UINT4			0x42	// Unsigned Integer
#define	TDS5_TYPE_UINT8			0x43	// Unsigned Integer
#define	TDS5_TYPE_UINTN			0x44	// Unsigned Integer
						// (variable length)
#define	TDS5_TYPE_SENSITIVITY		0x67	// Sensitivity
#define	TDS5_TYPE_BOUNDARY		0x68	// Boundary
#define	TDS5_TYPE_DECN			0x6A	// Decimal
#define	TDS5_TYPE_NUMN			0x6C	// Numeric
#define	TDS5_TYPE_FLTN			0x6D	// Float (variable length)
#define	TDS5_TYPE_MONEYN		0x6E	// Money (variable length)
#define	TDS5_TYPE_DATETIMEN		0x6F	// Date/time (variable length)
#define	TDS5_TYPE_SHORTMONEY		0x7A	// Money (4 byte)
#define	TDS5_TYPE_DATEN			0x7B	// Date (variable length)
#define	TDS5_TYPE_TIMEN			0x93	// Time (variable length)
#define	TDS5_TYPE_XML			0xA3	// XML
#define	TDS5_TYPE_UNITEXT		0xAE	// Unicode UTF-16 Text
#define	TDS5_TYPE_LONGCHAR		0xAF	// Character (4 byte length)
#define	TDS5_TYPE_SINT1			0xB0	// Signed Integer
#define	TDS5_TYPE_SYB5BIGDATETIME	0xBB	// Big Date/time
#define	TDS5_TYPE_SYB5BIGTIME		0xBC	// Big Time
#define	TDS5_TYPE_INT8			0xBF	// Integer
#define	TDS5_TYPE_LONGBINARY		0xE1	// Binary (4 byte length)

// Tds 5.0 rowfmt column flags.  One byte, and not the 16-bit map that
// colFlags() writes - there's no case-sensitivity bit, no 2-bit
// updateable field, and nullable sits somewhere else.
#define	TDS5_COLFLAG_HIDDEN		0x01
#define	TDS5_COLFLAG_KEY		0x02
#define	TDS5_COLFLAG_WRITEABLE		0x10
#define	TDS5_COLFLAG_NULLABLE		0x20
#define	TDS5_COLFLAG_IDENTITY		0x40

// Tds 5.0 paramfmt parameter status.  One byte in a paramfmt (0xEC) and
// four in a paramfmt2 (0x20), but the same bits either way.
//
// Not the same namespace as the rowfmt column flags above, even though
// both describe a column-shaped block: 0x10 is writeable there and means
// nothing here, and nullable is 0x20 in both by coincidence rather than
// by design.  Only "return" was ever seen on the wire - a real ct-lib
// client leaves nullable clear even for a parameter it sends a null in,
// so nothing may gate null handling on it.
#define	TDS5_PARAM_RETURN		0x01
#define	TDS5_PARAM_COLUMNSTATUS		0x08
#define	TDS5_PARAM_NULLALLOWED		0x20

// Tds 5.0 done transaction states - the second uint16 of a done, which
// is CurCmd in ms-tds.  The values are ct-lib's CS_TRAN_* (cspublic.h),
// and ct_res_info(CS_TRANS_STATE) is what surfaces them.  The whole set
// is here for the reader's sake, though transState() only ever sends
// in-progress or completed - the failure states can make a client mark
// the connection unusable, and there's no evidence for which one a real
// ase picks when.
#define	TDS5_TRAN_UNDEFINED		0
#define	TDS5_TRAN_IN_PROGRESS		1
#define	TDS5_TRAN_COMPLETED		2
#define	TDS5_TRAN_FAIL			3
#define	TDS5_TRAN_STMT_FAIL		4

static byte_t	tdstypemap[]={
	// "UNKNOWN"
	(byte_t)TDS_TYPE_NULL,
	// added by freetds
	// "CHAR"
	(byte_t)TDS_TYPE_BIGCHAR,
	// "INT"
	(byte_t)TDS_TYPE_INTN,
	// "SMALLINT"
	(byte_t)TDS_TYPE_INTN,
	// "TINYINT"
	(byte_t)TDS_TYPE_INTN,
	// "MONEY"
	(byte_t)TDS_TYPE_MONEYN,
	// "DATETIME"
	(byte_t)TDS_TYPE_DATETIMN,
	// "NUMERIC"
	(byte_t)TDS_TYPE_NUMERICN,
	// "DECIMAL"
	(byte_t)TDS_TYPE_DECIMALN,
	// "SMALLDATETIME"
	(byte_t)TDS_TYPE_DATETIMN,
	// "SMALLMONEY"
	(byte_t)TDS_TYPE_MONEYN,
	// "IMAGE"
	(byte_t)TDS_TYPE_IMAGE,
	// "BINARY"
	(byte_t)TDS_TYPE_BIGBINARY,
	// "BIT"
	(byte_t)TDS_TYPE_BITN,
	// "REAL"
	(byte_t)TDS_TYPE_FLTN,
	// "FLOAT"
	(byte_t)TDS_TYPE_FLTN,
	// "TEXT"
	(byte_t)TDS_TYPE_TEXT,
	// "VARCHAR"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "VARBINARY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LONGCHAR"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LONGBINARY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LONG"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "ILLEGAL"
	(byte_t)TDS_TYPE_NULL,
	// "SENSITIVITY"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "BOUNDARY"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "VOID"
	(byte_t)TDS_TYPE_NULL,
	// "USHORT"
	(byte_t)TDS_TYPE_INTN,
	// added by lago
	// "UNDEFINED"
	(byte_t)TDS_TYPE_NULL,
	// "DOUBLE"
	(byte_t)TDS_TYPE_FLTN,
	// "DATE"
	(byte_t)TDS_TYPE_DATEN,
	// "TIME"
	(byte_t)TDS_TYPE_TIMEN,
	// "TIMESTAMP"
	(byte_t)TDS_TYPE_DATETIME2N,
	// added by msql
	// "UINT"
	(byte_t)TDS_TYPE_INTN,
	// "LASTREAL"
	(byte_t)TDS_TYPE_DECIMALN,
	// added by mysql
	// "STRING"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "VARSTRING"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "LONGLONG"
	(byte_t)TDS_TYPE_INTN,
	// "MEDIUMINT"
	(byte_t)TDS_TYPE_INTN,
	// "YEAR"
	(byte_t)TDS_TYPE_INTN,
	// "NEWDATE"
	(byte_t)TDS_TYPE_DATEN,
	// "NULL"
	(byte_t)TDS_TYPE_NULL,
	// "ENUM"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "SET"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TINYBLOB"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "MEDIUMBLOB"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LONGBLOB"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "BLOB"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// added by oracle
	// "VARCHAR2"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "NUMBER"
	(byte_t)TDS_TYPE_DECIMALN,
	// "ROWID"
	(byte_t)TDS_TYPE_INTN,
	// "RAW"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LONG_RAW"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "MLSLABEL"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "CLOB"
	(byte_t)TDS_TYPE_TEXT,
	// "BFILE"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// added by odbc
	// "BIGINT"
	(byte_t)TDS_TYPE_INTN,
	// "INTEGER"
	(byte_t)TDS_TYPE_INTN,
	// "LONGVARBINARY"
	(byte_t)TDS_TYPE_IMAGE,
	// "LONGVARCHAR"
	(byte_t)TDS_TYPE_TEXT,
	// added by db2
	// "GRAPHIC"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "VARGRAPHIC"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LONGVARGRAPHIC"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "DBCLOB"
	(byte_t)TDS_TYPE_TEXT,
	// "DATALINK"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "USER_DEFINED_TYPE"
	(byte_t)TDS_TYPE_UDT,
	// "SHORT"
	(byte_t)TDS_TYPE_INTN,
	// "TINY"
	(byte_t)TDS_TYPE_INTN,
	// added by firebird
	// "D_FLOAT"
	(byte_t)TDS_TYPE_INTN,
	// "ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "QUAD"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INT64"
	(byte_t)TDS_TYPE_INTN,
	// "DOUBLE PRECISION"
	(byte_t)TDS_TYPE_INTN,
	// added by postgresql
	// "BOOL"
	(byte_t)TDS_TYPE_BITN,
	// "BYTEA"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "NAME"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "INT8"
	(byte_t)TDS_TYPE_INTN,
	// "INT2"
	(byte_t)TDS_TYPE_INTN,
	// "INT2VECTOR"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INT4"
	(byte_t)TDS_TYPE_INTN,
	// "REGPROC"
	(byte_t)TDS_TYPE_INTN,
	// "OID"
	(byte_t)TDS_TYPE_INTN,
	// "TID"
	(byte_t)TDS_TYPE_INTN,
	// "XID"
	(byte_t)TDS_TYPE_INTN,
	// "CID"
	(byte_t)TDS_TYPE_INTN,
	// "OIDVECTOR"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "SMGR"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "POINT"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "LSEG"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "PATH"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "BOX"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "POLYGON"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "LINE"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "LINE_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "FLOAT4"
	(byte_t)TDS_TYPE_FLTN,
	// "FLOAT8"
	(byte_t)TDS_TYPE_FLTN,
	// "ABSTIME"
	(byte_t)TDS_TYPE_INTN,
	// "RELTIME"
	(byte_t)TDS_TYPE_INTN,
	// "TINTERVAL"
	(byte_t)TDS_TYPE_INTN,
	// "CIRCLE"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "CIRCLE_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "MONEY_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "MACADDR"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "INET"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "CIDR"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "BOOL_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "BYTEA_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "CHAR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "NAME_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INT2_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INT2VECTOR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INT4_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "REGPROC_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TEXT_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "OID_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TID_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "XID_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "CID_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "OIDVECTOR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "BPCHAR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "VARCHAR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INT8_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "POINT_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LSEG_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "PATH_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "BOX_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "FLOAT4_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "FLOAT8_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "ABSTIME_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "RELTIME_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TINTERVAL_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "POLYGON_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "ACLITEM"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "ACLITEM_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "MACADDR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INET_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "CIDR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "BPCHAR"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "TIMESTAMP_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "DATE_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TIME_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TIMESTAMPTZ"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "TIMESTAMPTZ_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "INTERVAL"
	(byte_t)TDS_TYPE_INTN,
	// "INTERVAL_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "NUMERIC_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TIMETZ"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "TIMETZ_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "BIT_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "VARBIT"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "VARBIT_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "REFCURSOR"
	(byte_t)TDS_TYPE_INTN,
	// "REFCURSOR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "REGPROCEDURE"
	(byte_t)TDS_TYPE_INTN,
	// "REGOPER"
	(byte_t)TDS_TYPE_INTN,
	// "REGOPERATOR"
	(byte_t)TDS_TYPE_INTN,
	// "REGCLASS"
	(byte_t)TDS_TYPE_INTN,
	// "REGTYPE"
	(byte_t)TDS_TYPE_INTN,
	// "REGPROCEDURE_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "REGOPER_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "REGOPERATOR_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "REGCLASS_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "REGTYPE_ARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "RECORD"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "CSTRING"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "ANY"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "ANYARRAY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "TRIGGER"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "LANGUAGE_HANDLER"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "INTERNAL"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "OPAQUE"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "ANYELEMENT"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "PG_TYPE"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "PG_ATTRIBUTE"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "PG_PROC"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// "PG_CLASS"
	(byte_t)TDS_TYPE_BIGVARCHR,
	// none added by sqlite
	// added by sqlserver
	// "UBIGINT"
	(byte_t)TDS_TYPE_INTN,
	// "UNIQUEIDENTIFIER"
	(byte_t)TDS_TYPE_GUID,
	// added by informix
	// "SMALLFLOAT"
	(byte_t)TDS_TYPE_FLTN,
	// "BYTE"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "BOOLEAN"
	(byte_t)TDS_TYPE_BITN,
	// "TINYTEXT"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "MEDIUMTEXT"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "LONGTEXT"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "JSON"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "GEOMETRY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "SDO_GEOMETRY"
	(byte_t)TDS_TYPE_BIGVARBIN,
	// "NCHAR"
	(byte_t)TDS_TYPE_NCHAR,
	// "NVARCHAR"
	(byte_t)TDS_TYPE_NVARCHAR,
	// "NTEXT"
	(byte_t)TDS_TYPE_NTEXT,
	// "XML"
	(byte_t)TDS_TYPE_XML,
	// "DATETIMEOFFSET"
	(byte_t)TDS_TYPE_DATETIMEOFFSETN,
	// also added by informix
	// "LVARCHAR"
	(byte_t)TDS_TYPE_BIGVARCHR
};

// The tds 5.0 counterpart of tdstypemap[] - the same length, and the same
// *_DATATYPE index order, but carrying tds 5.0 datatypes.
//
// It deliberately deviates from what a real ase sends for the same column
// in a few places:
// * the n-variants (intn, fltn, moneyn, datetimen) rather than the fixed
//   types, because a fixed type has no null encoding at all.  The client
//   maps an n-variant back to the fixed type by its size, so it still
//   reports the same type and the same maximum length.
// * bit stays fixed at TDS5_TYPE_BIT, because the "nullable bit" byte
//   0x68 is TDS_BOUNDARY in tds 5.0.  A null bit has to go out as 0.
// * varchar (0x27) and varbinary (0x25) rather than char (0x2F) and
//   binary (0x2D), because char and binary make the client pad the value
//   out to the declared column size.  The backend already hands us
//   whatever padding the column really has.
// * date and time as varchar, in the rendering the backend hands us,
//   matching what the tds 7.x path does for a client older than 7.3.
static byte_t	pretds7typemap[]={
	// "UNKNOWN"
	(byte_t)TDS5_TYPE_VARCHAR,
	// added by freetds
	// "CHAR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "INT"
	(byte_t)TDS5_TYPE_INTN,
	// "SMALLINT"
	(byte_t)TDS5_TYPE_INTN,
	// "TINYINT"
	(byte_t)TDS5_TYPE_INTN,
	// "MONEY"
	(byte_t)TDS5_TYPE_MONEYN,
	// "DATETIME"
	(byte_t)TDS5_TYPE_DATETIMEN,
	// "NUMERIC"
	(byte_t)TDS5_TYPE_NUMN,
	// "DECIMAL"
	(byte_t)TDS5_TYPE_DECN,
	// "SMALLDATETIME"
	(byte_t)TDS5_TYPE_DATETIMEN,
	// "SMALLMONEY"
	(byte_t)TDS5_TYPE_MONEYN,
	// "IMAGE"
	(byte_t)TDS5_TYPE_IMAGE,
	// "BINARY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BIT"
	(byte_t)TDS5_TYPE_BIT,
	// "REAL"
	(byte_t)TDS5_TYPE_FLTN,
	// "FLOAT"
	(byte_t)TDS5_TYPE_FLTN,
	// "TEXT"
	(byte_t)TDS5_TYPE_TEXT,
	// "VARCHAR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "VARBINARY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LONGCHAR"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LONGBINARY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LONG"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "ILLEGAL"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "SENSITIVITY"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "BOUNDARY"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "VOID"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "USHORT"
	(byte_t)TDS5_TYPE_INTN,
	// added by lago
	// "UNDEFINED"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "DOUBLE"
	(byte_t)TDS5_TYPE_FLTN,
	// "DATE"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "TIME"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "TIMESTAMP"
	(byte_t)TDS5_TYPE_VARCHAR,
	// added by msql
	// "UINT"
	(byte_t)TDS5_TYPE_INTN,
	// "LASTREAL"
	(byte_t)TDS5_TYPE_DECN,
	// added by mysql
	// "STRING"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "VARSTRING"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "LONGLONG"
	(byte_t)TDS5_TYPE_INTN,
	// "MEDIUMINT"
	(byte_t)TDS5_TYPE_INTN,
	// "YEAR"
	(byte_t)TDS5_TYPE_INTN,
	// "NEWDATE"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "NULL"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "ENUM"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "SET"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TINYBLOB"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "MEDIUMBLOB"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LONGBLOB"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BLOB"
	(byte_t)TDS5_TYPE_VARBINARY,
	// added by oracle
	// "VARCHAR2"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "NUMBER"
	(byte_t)TDS5_TYPE_DECN,
	// "ROWID"
	(byte_t)TDS5_TYPE_INTN,
	// "RAW"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LONG_RAW"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "MLSLABEL"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "CLOB"
	(byte_t)TDS5_TYPE_TEXT,
	// "BFILE"
	(byte_t)TDS5_TYPE_VARBINARY,
	// added by odbc
	// "BIGINT"
	(byte_t)TDS5_TYPE_INTN,
	// "INTEGER"
	(byte_t)TDS5_TYPE_INTN,
	// "LONGVARBINARY"
	(byte_t)TDS5_TYPE_IMAGE,
	// "LONGVARCHAR"
	(byte_t)TDS5_TYPE_TEXT,
	// added by db2
	// "GRAPHIC"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "VARGRAPHIC"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LONGVARGRAPHIC"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "DBCLOB"
	(byte_t)TDS5_TYPE_TEXT,
	// "DATALINK"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "USER_DEFINED_TYPE"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "SHORT"
	(byte_t)TDS5_TYPE_INTN,
	// "TINY"
	(byte_t)TDS5_TYPE_INTN,
	// added by firebird
	// "D_FLOAT"
	(byte_t)TDS5_TYPE_INTN,
	// "ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "QUAD"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INT64"
	(byte_t)TDS5_TYPE_INTN,
	// "DOUBLE PRECISION"
	(byte_t)TDS5_TYPE_INTN,
	// added by postgresql
	// "BOOL"
	(byte_t)TDS5_TYPE_BIT,
	// "BYTEA"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "NAME"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "INT8"
	(byte_t)TDS5_TYPE_INTN,
	// "INT2"
	(byte_t)TDS5_TYPE_INTN,
	// "INT2VECTOR"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INT4"
	(byte_t)TDS5_TYPE_INTN,
	// "REGPROC"
	(byte_t)TDS5_TYPE_INTN,
	// "OID"
	(byte_t)TDS5_TYPE_INTN,
	// "TID"
	(byte_t)TDS5_TYPE_INTN,
	// "XID"
	(byte_t)TDS5_TYPE_INTN,
	// "CID"
	(byte_t)TDS5_TYPE_INTN,
	// "OIDVECTOR"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "SMGR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "POINT"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "LSEG"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "PATH"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "BOX"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "POLYGON"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "LINE"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "LINE_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "FLOAT4"
	(byte_t)TDS5_TYPE_FLTN,
	// "FLOAT8"
	(byte_t)TDS5_TYPE_FLTN,
	// "ABSTIME"
	(byte_t)TDS5_TYPE_INTN,
	// "RELTIME"
	(byte_t)TDS5_TYPE_INTN,
	// "TINTERVAL"
	(byte_t)TDS5_TYPE_INTN,
	// "CIRCLE"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "CIRCLE_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "MONEY_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "MACADDR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "INET"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "CIDR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "BOOL_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BYTEA_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "CHAR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "NAME_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INT2_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INT2VECTOR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INT4_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "REGPROC_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TEXT_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "OID_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TID_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "XID_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "CID_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "OIDVECTOR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BPCHAR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "VARCHAR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INT8_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "POINT_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LSEG_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "PATH_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BOX_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "FLOAT4_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "FLOAT8_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "ABSTIME_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "RELTIME_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TINTERVAL_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "POLYGON_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "ACLITEM"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "ACLITEM_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "MACADDR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INET_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "CIDR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BPCHAR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "TIMESTAMP_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "DATE_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TIME_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TIMESTAMPTZ"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "TIMESTAMPTZ_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "INTERVAL"
	(byte_t)TDS5_TYPE_INTN,
	// "INTERVAL_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "NUMERIC_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TIMETZ"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "TIMETZ_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BIT_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "VARBIT"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "VARBIT_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "REFCURSOR"
	(byte_t)TDS5_TYPE_INTN,
	// "REFCURSOR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "REGPROCEDURE"
	(byte_t)TDS5_TYPE_INTN,
	// "REGOPER"
	(byte_t)TDS5_TYPE_INTN,
	// "REGOPERATOR"
	(byte_t)TDS5_TYPE_INTN,
	// "REGCLASS"
	(byte_t)TDS5_TYPE_INTN,
	// "REGTYPE"
	(byte_t)TDS5_TYPE_INTN,
	// "REGPROCEDURE_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "REGOPER_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "REGOPERATOR_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "REGCLASS_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "REGTYPE_ARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "RECORD"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "CSTRING"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "ANY"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "ANYARRAY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "TRIGGER"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "LANGUAGE_HANDLER"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "INTERNAL"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "OPAQUE"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "ANYELEMENT"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "PG_TYPE"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "PG_ATTRIBUTE"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "PG_PROC"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "PG_CLASS"
	(byte_t)TDS5_TYPE_VARCHAR,
	// none added by sqlite
	// added by sqlserver
	// "UBIGINT"
	(byte_t)TDS5_TYPE_INTN,
	// "UNIQUEIDENTIFIER"
	(byte_t)TDS5_TYPE_VARCHAR,
	// added by informix
	// "SMALLFLOAT"
	(byte_t)TDS5_TYPE_FLTN,
	// "BYTE"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "BOOLEAN"
	(byte_t)TDS5_TYPE_BIT,
	// "TINYTEXT"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "MEDIUMTEXT"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "LONGTEXT"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "JSON"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "GEOMETRY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "SDO_GEOMETRY"
	(byte_t)TDS5_TYPE_VARBINARY,
	// "NCHAR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "NVARCHAR"
	(byte_t)TDS5_TYPE_VARCHAR,
	// "NTEXT"
	(byte_t)TDS5_TYPE_TEXT,
	// "XML"
	(byte_t)TDS5_TYPE_TEXT,
	// "DATETIMEOFFSET"
	(byte_t)TDS5_TYPE_VARCHAR,
	// also added by informix
	// "LVARCHAR"
	(byte_t)TDS5_TYPE_VARCHAR
};

// tdstypemap[] and pretds7typemap[] have to stay the same length, since
// mapType() bounds-checks against the one and may index the other.  This
// fails to compile if they ever drift apart.
typedef char	pretds7typemapsizecheck[
			(sizeof(pretds7typemap)==sizeof(tdstypemap))?1:-1];

// rpc proc ids
#define	SP_CURSOR		1
#define	SP_CURSOR_OPEN		2
#define SP_CURSOR_PREPARE	3 
#define SP_CURSOR_EXECUTE	4 
#define SP_CURSOR_PREP_EXEC	5 
#define SP_CURSOR_UNPREPARE	6 
#define SP_CURSOR_FETCH		7 
#define SP_CURSOR_OPTION	8 
#define SP_CURSOR_CLOSE		9 
#define SP_EXECUTE_SQL		10 
#define SP_PREPARE		11 
#define SP_EXECUTE		12 
#define SP_PREP_EXEC		13 
#define SP_PREP_EXEC_RPC	14

// the widest a decimal or numeric can be on the wire - 1 sign byte plus up
// to 16 bytes of magnitude
#define TDS_DECIMAL_MAX_SIZE	17

// the most digits a decimal or numeric can carry
#define TDS_DECIMAL_MAX_PRECISION	38
#define SP_UNPREPARE		15

#define SP_MAX_PROCID		15

// the widest a bigchar or nchar value can be padded out to
#define TDS_MAX_CHAR_SIZE	8000

// the widest a binary or bigbinary value can be padded out to
#define TDS_MAX_BINARY_SIZE	8000

// how many bytes a guid occupies on the wire
#define TDS_GUID_SIZE		16

// A USHORTMAXLEN of 0xFFFF in a bigvarchr/bigvarbin/nvarchar TYPE_INFO isn't
// a length at all - it's varchar(max)/varbinary(max)/nvarchar(max), and it
// means the value that follows is partially length prefixed (MS-TDS 2.2.5.2.3)
// rather than prefixed with one plain length.
#define TDS_USHORTMAXLEN	0xFFFF

// PLP_BODY total-length sentinels.  Anything else is the real byte count, but
// the chunks and terminator follow in every case, so the total length is only
// ever a hint.
#define TDS_PLP_NULL		0xFFFFFFFFFFFFFFFFULL
#define TDS_PLP_UNKNOWN_LEN	0xFFFFFFFFFFFFFFFEULL

static const char *procids[]={
	"",
	"SP_CURSOR",
	"SP_CURSOR_OPEN",
	"SP_CURSOR_PREPARE",
	"SP_CURSOR_EXECUTE",
	"SP_CURSOR_PREP_EXEC",
	"SP_CURSOR_UNPREPARE",
	"SP_CURSOR_FETCH",
	"SP_CURSOR_OPTION",
	"SP_CURSOR_CLOSE",
	"SP_EXECUTE_SQL",
	"SP_PREPARE",
	"SP_EXECUTE",
	"SP_PREP_EXEC",
	"SP_PREP_EXEC_RPC",
	"SP_UNPREPARE"
};

// the procs above by name, which clients may send instead of the id
// (freetds always sends sp_execute by name)
static const char *procnames[]={
	"",
	"sp_cursor",
	"sp_cursoropen",
	"sp_cursorprepare",
	"sp_cursorexecute",
	"sp_cursorprepexec",
	"sp_cursorunprepare",
	"sp_cursorfetch",
	"sp_cursoroption",
	"sp_cursorclose",
	"sp_executesql",
	"sp_prepare",
	"sp_execute",
	"sp_prepexec",
	"sp_prepexecrpc",
	"sp_unprepare"
};

// rpc option flags (MS-TDS 2.2.6.6)
#define RPC_WITH_RECOMP		0x0001
#define RPC_NO_META_DATA	0x0002
#define RPC_REUSE_META_DATA	0x0004

// rpc batch flags, which follow the parameters
#define RPC_BATCH_FLAG		0x80
#define RPC_NO_EXEC_FLAG	0xFF

// sp_cursoropen scroll options
#define CURSOR_SCROLLOPT_KEYSET		0x0001
#define CURSOR_SCROLLOPT_DYNAMIC	0x0002
#define CURSOR_SCROLLOPT_FORWARD_ONLY	0x0004
#define CURSOR_SCROLLOPT_STATIC		0x0008
#define CURSOR_SCROLLOPT_FAST_FORWARD	0x0010
#define CURSOR_SCROLLOPT_PARAMETERIZED	0x1000

// sp_cursoropen concurrency control options
#define CURSOR_CCOPT_READ_ONLY		0x0001
#define CURSOR_CCOPT_SCROLL_LOCKS	0x0002
#define CURSOR_CCOPT_OPTIMISTIC		0x0004
#define CURSOR_CCOPT_ALLOW_DIRECT	0x2000

// sp_cursorfetch fetch types
#define CURSOR_FETCH_FIRST	0x0001
#define CURSOR_FETCH_NEXT	0x0002
#define CURSOR_FETCH_PREV	0x0004
#define CURSOR_FETCH_LAST	0x0008
#define CURSOR_FETCH_ABSOLUTE	0x0010
#define CURSOR_FETCH_RELATIVE	0x0020
#define CURSOR_FETCH_REFRESH	0x0080
#define CURSOR_FETCH_INFO	0x0100
#define CURSOR_FETCH_PREV_NOADJUST	0x0200
#define CURSOR_FETCH_SKIP_UPDATE_CNT	0x0400

// sp_cursor operation types
#define CURSOR_OP_UPDATE	0x0001
#define CURSOR_OP_DELETE	0x0002
#define CURSOR_OP_INSERT	0x0004
#define CURSOR_OP_REFRESH	0x0008
#define CURSOR_OP_LOCK		0x0010
#define CURSOR_OP_SETPOSITION	0x0020
#define CURSOR_OP_ABSOLUTE	0x0040

// the most rows of a fetch that are kept for sp_cursor to position on,
// which covers every rowset size an odbc driver actually uses
#define CURSOR_MAX_POSITION_ROWS	1024

// the most exec statements in one batch that get a synthesized return
// status
#define BATCH_MAX_EXECS		32

// the return status that all of these procs use for success
#define RPC_STATUS_SUCCESS	0
#define RPC_STATUS_FAILURE	1

// sql server error numbers that these procs answer with, which a failed one
// also sends back as its return status
#define RPC_INVALID_COLUMN	207
#define RPC_WRONG_PARAM_TYPE	214
#define RPC_NO_SUCH_STMT	8179
#define RPC_CURSOR_READ_ONLY	16929
#define RPC_NO_ROWS_AFFECTED	16947
#define RPC_NO_SUCH_CURSOR	16950
#define RPC_NO_SUCH_ROW		16955
#define RPC_OP_UNSUPPORTED	16957
#define RPC_FETCH_UNSUPPORTED	16958

// a wire type this module doesn't recognize, reported as a plain error
// rather than a return status - a real server never gets far enough to
// identify a proc to return one for
#define RPC_UNKNOWN_DATA_TYPE	8009

// close-all cursor id for sp_cursorclose
#define CURSOR_CLOSE_ALL	0xFFFFFFFF


// a row copied out of a result set, with a NULL value for a null field
class tdsrow {
	public:
		char		**values;
		uint64_t	*sizes;
};

// One parameter's format, as a tds 5.0 paramfmt declared it.  A params
// token carries no lengths of its own, so the whole set has to be kept
// between the two tokens and replayed to size each value.  The same
// shape drives the writers, so a paramfmt/params pair going out is
// described exactly the way one coming in is.
//
// "size" is what the paramfmt declared, which is NOT the width of the
// value behind it - a client and a server can declare the same
// decimal(9,2) at 33 and at 5, and both are legal.  Size a value from
// its own length, the way preTds7Field() does for a row field.
class tds5paramfmt {
	public:
		const char	*name;
		uint16_t	namesize;
		byte_t		status;
		uint32_t	usertype;
		// the datatype as it arrived, in the tds 5.0 namespace,
		// and the ms-tds type that means the same thing
		byte_t		tds5type;
		byte_t		mstype;
		byte_t		varintsize;
		uint32_t	size;
		byte_t		precision;
		byte_t		scale;
};

class sqlrprotocol_tds;

// the rows the most recent sp_cursorfetch returned for one cursor.  the
// values have to be copied while the row is read for the ROW token -
// sp_cursor arrives as a request of its own, by which time
// sqlrservercontroller's field pointers have been re-aimed at whichever
// cursor fetched last
class tdsrows {
	public:
				tdsrows(sqlrprotocol_tds *tds);

		void		reset(uint32_t colcount);
		tdsrow		*newRow();
		void		setField(tdsrow *row,
						uint32_t col,
						const char *value,
						uint64_t size,
						bool null);
		tdsrow		*getRow(uint64_t rownum);

		uint32_t	getColCount();
	private:
		sqlrprotocol_tds	*tds;

		memorypool		pool;
		linkedlist<tdsrow *>	rows;
		uint32_t		colcount;
};

// tdsrows' methods are defined below, after sqlrprotocol_tds is fully
// declared - they call debug methods on it through the tds pointer above


// a filedescriptor that shares another one's descriptor, without ever
// closing it out from under whoever really owns it.  the no-op
// lowLevelClose() covers an explicit close(); the destructor has to unbind
// the descriptor as well, because by the time ~filedescriptor() runs this
// sub-object is gone and the virtual call lands on the base class close.
class tdssharedfd : public filedescriptor {
	public:
			~tdssharedfd();
	protected:
		int32_t	lowLevelClose();
};

tdssharedfd::~tdssharedfd() {
	setFileDescriptor(-1);
}

int32_t tdssharedfd::lowLevelClose() {
	return 0;
}

// the tls engine's end of the client socket.  ms-tds tunnels the tls
// handshake inside tds packets, so this sits between the tls context and
// the real client socket, adding and stripping that framing.  framing is
// only on for the handshake; afterward it just passes records through, on
// the client socket's own read() and write() rather than a second
// descriptor, which would race it for bytes already in its read buffer.
class tdstlsframer : public tdssharedfd {
	public:
			tdstlsframer(sqlrprotocol_tds *tds);
			~tdstlsframer();

		void	setFraming(bool framing);
	protected:
		ssize_t	lowLevelRead(void *buf, size_t count);
		ssize_t	lowLevelWrite(const void *buf, size_t count);
	private:
		bool	readPacket();

		sqlrprotocol_tds	*tds;

		bool		framing;

		// the packet currently being served to the tls engine
		byte_t		*readbuffer;
		uint32_t	readbuffersize;
		uint32_t	readposition;

		byte_t		packetid;
};

// TDS protocol class
class SQLRSERVER_DLLSPEC sqlrprotocol_tds : public sqlrprotocol {
	friend class tdstlsframer;
	public:
		sqlrprotocol_tds(sqlrservercontroller *cont,
							domnode *parameters);
		virtual	~sqlrprotocol_tds();

		clientsessionexitstatus_t	clientSession(
							filedescriptor *cs);
	private:
		void	init();
		void	free();
		void	reInit();

		byte_t	*convertCharset(const byte_t *inbuf,
					size_t insize,
					const char *inenc,
					const char *outenc,
					size_t *outsize);
		char	*ucs2ToUtf8(const ucs2_t *str,
					size_t chars,
					size_t *size);
		ucs2_t	*utf8ToUcs2(const char *str,
					size_t size,
					size_t *chars);
		char	*utf8ToCp1252(const char *str,
					size_t size,
					size_t *outsize);
		char	*preTds7ToUtf8(const byte_t *str,
					size_t size,
					size_t *outsize);

		// Convert between the charset the pre-tds7 client declared
		// in its login record and the utf-8 this module works in.
		// Both return NULL - and leave *outsize alone - when no
		// conversion applies, either because the client declared
		// nothing, or declared utf8, or declared a charset
		// pretds7charsets[] doesn't cover, or because the
		// conversion itself failed.  Every caller answers a NULL by
		// passing the bytes through unconverted.
		//
		// The outbound one is called from the callers of
		// writeVarchar() rather than from writeVarchar() itself,
		// because a converted string isn't necessarily the same
		// number of bytes as the utf-8 it came from, and each of
		// those callers writes a length - its own, and usually a
		// token size containing it - that has to count the bytes
		// that actually go on the wire.
		char	*clientCharsetToUtf8(const byte_t *str,
					size_t size,
					size_t *outsize);
		char	*utf8ToClientCharset(const char *str,
					size_t size,
					size_t *outsize);

		bool	recvPacket(byte_t *packettype);
		// "packettype" defaults to the tabular result that every
		// response but the sec_encrypt negotiate goes out as
		bool	sendPacket(byte_t packettype=TABULAR_RESULT);

		wchar_t	*readPassword(const byte_t *rp,
					size_t charcount);

		void		getServerTdsVersion();
		uint32_t	tdsVersionHexToDec(uint32_t tdsversion);
		uint32_t	tdsVersionDecToHex(uint32_t tdsversion,
								bool client);
		void		negotiateTdsVersion();

		bool	preLogin();
		byte_t	negotiateEncryption(byte_t clientencryption);
		bool	startTls();
		void	stopTls();
		bool	fitsInPacket(uint16_t offset,
						size_t size,
						size_t packetsize);

		bool	preTds7Login();
		// Applies the charset and language a pre-tds7 login record
		// declared.  The charset is applied as soon as the record
		// is parsed, because it decides how character data is
		// converted in both directions for the rest of the session
		// - including an error sent before the login even finishes.
		void	preTds7SetCharsetAndLanguage(const char *charset,
						byte_t charsetlen,
						const char *language,
						byte_t languagelen,
						byte_t suppresslanguage);
		// The envchange (and the info message behind it) that tells
		// a pre-tds7 client what charset the session settled on
		void	envChangeCharset();
		// The pre-tds7 counterpart of the language block in
		// tds7Login()
		void	preTds7ChangeLanguage();
		// "value" must point at a buffer of at least "size"+1
		// bytes - see the note at the definition
		void	readPreTds7Field(const byte_t *rp,
						char *value,
						size_t size,
						byte_t *length,
						const byte_t **rpout);
		void	capability();

		// Capability mask arithmetic.  A mask arrives most
		// significant byte first, so bit "cap" of a mask "masklen"
		// bytes long is bit (cap&7) of mask[masklen-1-(cap>>3)].
		// Both are no-ops for a bit past the end of the mask.
		static bool	capabilityBitIsSet(const byte_t *mask,
						byte_t masklen,
						uint16_t cap);
		static void	setCapabilityBit(byte_t *mask,
						byte_t masklen,
						uint16_t cap,
						bool on);
		// sets every capability in "caps" in "mask", then
		// intersects it with "clientmask" - see capability()
		static void	buildCapabilityMask(byte_t *mask,
						byte_t masklen,
						const uint16_t *caps,
						uint16_t capcount,
						const byte_t *clientmask);

		// What the login ended up agreeing to.  All three answer
		// false when the client sent no capability token at all.
		bool	requestCapabilityGranted(uint16_t cap);
		bool	responseCapabilityGranted(uint16_t cap);
		bool	clientRequestedCapability(uint16_t cap);

		// The tds 5.0 encrypted-password exchange.
		// preTds7SecEncryptLogin() drives the whole thing and hands
		// back the cleartext password; the rest are its pieces.
		// "password" must point at a buffer of "passwordsize" bytes.
		bool	preTds7SecEncryptLogin(char *password,
					size_t passwordsize);
		void	preTds7Msg(byte_t status, uint16_t msgid);
		bool	preTds7MsgRead(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t *msgid);
		// reads the paramfmt/params pair behind one msg token and
		// copies parameter "param" out of it as a blob
		bool	preTds7SecEncryptBlob(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t param,
					byte_t *blob);
		// The cipher itself, and the only thing that knows how the
		// blob is enciphered - see the note at the definition.
		bool	secEncryptDecryptPassword(const byte_t *key,
					const byte_t *blob,
					size_t bloblen,
					char *password,
					size_t passwordsize);

		bool	sendSecEncryptUnsupportedError();
		bool	sendPreTds7VersionUnsupportedError();
		bool	sendPreTds7ByteOrderUnsupportedError();
		// decodes one byte of the login record's typeflags block
		// into PRE_TDS7_ORDER_LE/BE/UNKNOWN
		byte_t	preTds7FieldOrder(byte_t value,
					byte_t leval, byte_t beval);
		// works the whole typeflags block out into the one byte
		// order the session runs in - see the definition
		bool	preTds7ByteOrder(const byte_t *typeflags,
					bool *bigendian);
		// blank a login record's password fields, so the
		// received-packet hex dump doesn't hand out passwords the
		// field-level output takes care to hide
		void	maskTds7Passwords(byte_t *packet,
						uint32_t packetsize,
						uint64_t packetoffset);
		void	maskPreTds7Passwords(byte_t *packet,
						uint32_t packetsize,
						uint64_t packetoffset);
		// blanks the part of a request-relative range that landed
		// in this packet
		void	maskRange(byte_t *packet,
						uint32_t packetsize,
						uint64_t packetoffset,
						uint64_t start,
						uint64_t size);

		bool	preTds7Normal();
		byte_t	preTds7TokenLength(byte_t token);
		bool	preTds7SkipCommand(const byte_t **rpinout,
					size_t *rpsizeinout,
					byte_t token);
		bool	preTds7Language(const byte_t **rpinout,
					size_t *rpsizeinout);
		bool	preTds7DbRpc(const byte_t **rpinout,
					size_t *rpsizeinout);

		// Tds 5.0 dynamic sql.  preTds7Dynamic() decodes the token
		// and hands what it decoded to one of the operations below,
		// each of which appends its own done.  "more" is whether
		// another command follows in the request buffer.
		bool	preTds7Dynamic(const byte_t **rpinout,
					size_t *rpsizeinout);
		bool	preTds7DynamicPrepare(const char *id,
					const char *stmt,
					bool more);
		bool	preTds7DynamicExecute(const char *id, bool more);
		bool	preTds7DynamicDealloc(const char *id, bool more);
		bool	preTds7DynamicExecImmediate(const char *stmt,
					size_t stmtsize,
					bool more);
		void	preTds7DynamicDescribe(const char *id,
					bool output,
					bool more);
		void	preTds7DynamicAck(const char *id, size_t idsize);
		const char	*preTds7DynamicStatement(const char *stmt,
							const char *id);
		void	preTds7DynamicError(const char *msgtext, bool more);
		// the string-id-to-handle map, and the eviction that bounds
		// it.  the cursor itself lives in stmthandles.
		bool	dynamicHandle(const char *id, uint32_t *handle);
		void	setDynamicHandle(const char *id, uint32_t handle);
		void	removeDynamicHandle(const char *id);
		void	evictOldestDynamicHandle();

		// Tds 5.0 paramfmt/params, in both directions.  These know
		// nothing about what the pair is attached to - a language
		// command, a dbrpc, a dynamic execute and a server-sent msg
		// all carry the same two tokens and use them the same way.
		//
		// The token byte is read by the caller; these start at the
		// token length.  "wide" picks the paramfmt2 (0x20) shape
		// over the paramfmt (0xEC) one.
		bool	preTds7ParamFmt(const byte_t **rpinout,
					size_t *rpsizeinout,
					bool wide);
		// the walk itself, split out so that each of its dozen
		// bail-outs is one line rather than the five
		// preTds7ParamFmt() turns them into
		bool	preTds7ParamFmtRead(const byte_t **rpinout,
					size_t *rpsizeinout,
					bool wide,
					const char **err);
		bool	preTds7Params(const byte_t **rpinout,
					size_t *rpsizeinout);
		// the walk itself, split out for the same reason
		// preTds7ParamFmtRead() is
		bool	preTds7ParamsRead(const byte_t **rpinout,
					size_t *rpsizeinout);
		// the pair together, as a command token that declared one
		// carries it
		bool	preTds7ParamFmtAndParams(const byte_t **rpinout,
					size_t *rpsizeinout);
		bool	preTds7ParamValueRead(const byte_t **rpinout,
					size_t *rpsizeinout,
					const tds5paramfmt *fmt,
					sqlrserverbindvar *bv);
		bool	preTds7ParamFmtWrite(const tds5paramfmt *fmts,
					uint16_t count);
		bool	preTds7ParamsWrite(const tds5paramfmt *fmts,
					sqlrserverbindvar *bvs,
					uint16_t count);
		void	preTds7ParamValueWrite(const tds5paramfmt *fmt,
					sqlrserverbindvar *bv);
		void	preTds7ParamNullWrite(const tds5paramfmt *fmt);
		void	preTds7ParamError(const char *msgtext, bool more);

		void	preTds7UnsupportedToken(byte_t token, bool more);
		void	tooManyCommands(byte_t token);

		bool	tds7Login();
		bool	loginFieldFits(const char *name,
						uint16_t ib,
						uint16_t *cch,
						uint16_t max,
						size_t charsize,
						size_t rpsize);
		bool	auth(const wchar_t *username,
						size_t usernamelen,
						const wchar_t *password,
						size_t passwordlen);
		bool	auth(const char *username,
						const char *password);
		// "status" is one of the PRE_TDS7_LOGIN_ACK_* values, and
		// only reaches the wire for a pre-tds7 client - see the
		// definition
		void	loginAck(byte_t status);
		void	authError(const wchar_t *username,
						size_t usernamelen);
		void	authError(const char *username);
		bool	changeDatabase(const wchar_t *database,
						size_t databaselen);
		void	changeDatabaseInfo(const wchar_t *database,
						size_t databaselen);
		void	changeDatabaseError(const wchar_t *database,
						size_t databaselen,
						bool warning);
		bool	changeCollation(uint32_t lcid);
		void	envChangeSqlCollation(uint32_t lcid,
						byte_t sortid);
		bool	changeLanguage(const wchar_t *language,
						size_t languagelen);
		void	changeLanguageInfo(const wchar_t *language,
						size_t languagelen);
		void	changeLanguageError(const wchar_t *language,
						size_t languagelen,
						bool warning);
		void	negotiatePacketSize(uint32_t packetsize);
		void	envChangePacketSize();

		bool	federatedAuthenticationToken();
		bool	attention();
		bool	transactionManagerRequest();
		bool	sspi();

		bool	sqlBatch();
		uint16_t	batchExecResultSets(const char *sql,
						uint16_t *rsindex,
						uint16_t maxcount);
		bool	isSqlWordChar(char ch);
		void	allHeaders(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout);
		void	colMetaData(sqlrservercursor *cursor, bool nometadata);
		// The tds 5.0 counterpart of colMetaData().  Returns
		// false, having sent an error and a done of its own, if
		// the metadata won't fit in a rowfmt - the caller must
		// not send rows after that.  "more" is whether another
		// command follows in the request buffer, and only
		// decides the DONE_MORE of that done.
		bool	preTds7RowFmt(sqlrservercursor *cursor, bool more);
		void	preTds7ColFlags(bytebuffer *buffer,
					sqlrservercursor *cursor,
					uint16_t col);
		void	preTds7TypeInfo(bytebuffer *buffer,
					sqlrservercursor *cursor,
					uint16_t col,
					uint16_t coltype,
					byte_t tds5type);
		// The single source of the sizes a pre-tds7 result set is
		// declared at.  preTds7TypeInfo() writes what these return
		// and preTds7Field() caps every value against them, so the
		// two writers can't drift apart.
		uint32_t	preTds7DeclaredSize(sqlrservercursor *cursor,
						uint16_t col,
						uint16_t coltype,
						byte_t tds5type);
		void	preTds7DecimalInfo(sqlrservercursor *cursor,
						uint16_t col,
						byte_t *precision,
						byte_t *scale);
		void	cekTable();
		byte_t	mapType(uint16_t type);
		byte_t	preTds7VarintSize(byte_t tds5type);
		byte_t	preTds7FixedSize(byte_t tds5type);
		void	colData(sqlrservercursor *cursor, uint16_t col);
		void	userType(byte_t tdstype);
		void	colFlags(sqlrservercursor *cursor,
					uint16_t col,
					byte_t tdstype);
		void	typeInfo(sqlrservercursor *cursor,
					uint16_t col,
					uint16_t coltype,
					byte_t tdstype);
		void	writeCollation();
		void	tableName(byte_t tdstype);
		void	cryptoMetaData();
		void	colName(sqlrservercursor *cursor, uint16_t col);
		bool	isCaseSensitiveType(byte_t tdstype);
		bool	isFixedLenType(byte_t tdstype);
		bool	isVarLenType(byte_t tdstype);
		bool	isPartLenType(byte_t tdstype);
		bool	isCharType(byte_t tdstype);
		byte_t	tds5TypeToMsType(byte_t tds5type);
		byte_t	nTypeSize(uint16_t coltype,
					byte_t tdstype,
					uint32_t colsize);
		uint32_t	dateTimeStringSize(uint16_t coltype,
							uint32_t colsize);
		int64_t	moneyValue(const char *field);
		uint64_t	rows(sqlrservercursor *cursor);
		uint64_t	rows(sqlrservercursor *cursor,
					uint64_t maxrows,
					tdsrows *position=NULL);
		void	lobData(byte_t tdstype);
		void	field(uint16_t coltype,
					byte_t tdstype,
					uint32_t colsize,
					uint32_t colscale,
					const char *field,
					uint64_t fieldsize,
					bool null);
		// The tds 5.0 counterparts of rows() and field().  Separate
		// implementations rather than branches in those: the length
		// prefixes, the null forms, the character encoding and the
		// decimal layout all come out differently, and rows() and
		// field() are on the hot ms-tds path.
		uint64_t	preTds7Rows(sqlrservercursor *cursor);
		void	preTds7Field(uint16_t coltype,
					byte_t tds5type,
					uint32_t colsize,
					byte_t precision,
					const char *field,
					uint64_t fieldsize,
					bool null);
		// writes "size" bytes of a binary column value, decoding
		// "field" from hex text if "hextext" is set
		void	binary(const char *field,
					uint64_t size,
					bool hextext);
		bool	parseDateTime(const char *datetime,
					int16_t *year,
					int16_t *month,
					int16_t *day,
					int16_t *hour,
					int16_t *minute,
					int16_t *second,
					int32_t *usec,
					int16_t *tzoffset);
		void	dateTime(const char *datetime,
					int32_t *dayssince1900,
					uint32_t *threehundredths);
		void	dateTimeValue(int32_t dayssince1900,
					uint32_t threehundredths,
					sqlrserverbindvar *bv);
		void	moneyValue(int64_t tenthousandths,
					sqlrserverbindvar *bv);
		uint32_t	daysSince1(int16_t year,
						int16_t month,
						int16_t day);
		uint64_t	incrementsSince12AM(int16_t hour,
						int16_t minute,
						int16_t second,
						int32_t usec,
						byte_t scale);
		void	date(const char *datetime, uint32_t *dayssince1);
		void	time(const char *datetime,
					byte_t scale,
					uint64_t *increments);
		byte_t	timeSize(byte_t scale);
		void	appendDate(uint32_t dayssince1);
		void	appendTime(uint64_t increments, byte_t size);
		void	daten(const char *field);
		void	timen(const char *field, byte_t scale);
		void	datetime2n(const char *field, byte_t scale);
		void	datetimeoffsetn(const char *field, byte_t scale);
		void	decimal(const char *field,
					byte_t *ispositive,
					byte_t *size,
					byte_t *val);
		byte_t	decimalSize(byte_t precision);
		void	guid(const char *field, byte_t *g);
		byte_t	charsToHex(const char *chars);

		bool	insertBulk(const char *sql);
		bool	bulkLoad();
		bool	bulkColMetaData(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t *colcount);
		bool	bulkTypeInfo(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t col);
		char	*bulkInsert(uint16_t colcount);
		bool	bulkRow(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t colcount,
					sqlrservercursor *cursor);
		bool	bulkField(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t col,
					sqlrserverbindvar *bv,
					memorypool *bindpool);
		bool	bulkValue(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t col,
					sqlrserverbindvar *bv,
					memorypool *bindpool);
		void	bulkString(sqlrserverbindvar *bv,
					memorypool *bindpool,
					const char *value,
					size_t valuesize);
		void	bulkBinary(sqlrserverbindvar *bv,
					memorypool *bindpool,
					const byte_t *value,
					size_t valuesize);
		void	bulkDouble(sqlrserverbindvar *bv, double value);
		void	bulkDecimal(byte_t ispositive,
					const byte_t *val,
					byte_t size,
					byte_t scale,
					stringbuffer *strb);
		void	bulkMoney(int64_t tenthousandths, stringbuffer *strb);
		void	bulkDateTime(int32_t dayssince1900,
					uint32_t threehundredths,
					stringbuffer *strb);
		void	bulkYmd(int32_t days,
					int32_t startyear,
					stringbuffer *strb);
		void	bulkTime(uint64_t increments,
					byte_t scale,
					stringbuffer *strb);
		void	bulkGuid(const byte_t *g, stringbuffer *strb);

		bool	remoteProcedureCall();
		bool	rpc(const byte_t **rpinout,
					size_t *rpsizeinout,
					bool *more);
		// the proc dispatch, shared by rpc() and preTds7DbRpc()
		bool	runProc(uint16_t procid,
					const char *procname,
					bool nometadata);
		uint16_t	procNameToProcId(const char *procname);
		bool	params(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout);
		bool	param(uint16_t param,
					const byte_t **rpinout,
					size_t *rpsizeinout,
					bool exceeded);
		bool	paramValue(uint16_t param,
					const byte_t **rpinout,
					size_t *rpsizeinout,
					sqlrserverbindvar *bv);
		bool	parseXmlInfo(const byte_t **rpinout,
					size_t *rpsizeinout);
		bool	plpValue(const byte_t **rpinout,
					size_t *rpsizeinout,
					byte_t tdstype,
					sqlrserverbindvar *bv,
					memorypool *bindpool);
		void	batchFlags(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout,
					bool *more);

		// rpc parameter accessors, indexed as the params arrived
		bool		paramIsNull(uint16_t param);
		int64_t		paramInteger(uint16_t param);
		const char	*paramString(uint16_t param);

		void	bindParams(sqlrservercursor *cursor, uint16_t first,
						bool returnvalue=false);

		// rpc handlers
		bool	namedProc(const char *procname, bool nometadata);
		bool	backendHandleProc(const char *procname,
					uint32_t handle,
					bool resultset,
					bool nometadata);
		bool	backendCursorExecute(uint32_t handle,
						bool nometadata);
		bool	executeSql(bool nometadata);
		bool	prepare(bool prepexec, bool rpcsyntax, bool nometadata);
		bool	execute(bool nometadata);
		bool	unprepare();

		// The wire-neutral cores of the three above, shared with
		// tds 5.0 dynamic sql, which does the same three things by
		// string id rather than by numeric handle.  What's left in
		// each wrapper is the ms-tds reply tail.
		//
		// prepareStatement() returns false either when no cursor
		// was available - *cursorout is NULL then - or when the
		// prepare itself failed, with *cursorout set so the caller
		// can pull the error out of it and release it.
		bool	prepareStatement(uint32_t oldhandle,
					const char *query,
					size_t querylen,
					bool exec,
					uint16_t firstvalue,
					sqlrservercursor **cursorout,
					uint32_t *handleout);
		bool	executeStatement(sqlrservercursor *cursor,
					uint16_t firstvalue);
		void	unprepareStatement(uint32_t handle,
					sqlrservercursor *cursor);
		bool	cursorOpen(bool nometadata);
		bool	cursorPrepare();
		bool	cursorExecute(bool nometadata);
		bool	cursorPrepExec(bool nometadata);
		bool	cursorUnprepare();
		bool	cursorFetch(bool nometadata);
		bool	cursorOption();
		bool	cursorClose();
		bool	cursorPositioned();
		bool	isCursorStatement(const char *stmt);
		size_t	stripForUpdateOf(const char *stmt, size_t stmtlen);

		// sp_cursor helpers
		bool	positionedUpdate(sqlrservercursor *cursor,
						const char *table,
						tdsrow *row);
		bool	positionedDelete(sqlrservercursor *cursor,
						const char *table,
						tdsrow *row);
		bool	positionedInsert(sqlrservercursor *cursor,
						const char *table);
		bool	positionedExecute(sqlrservercursor *cursor,
						const char *query,
						size_t querysize,
						uint16_t bindcount);

		bool	positionedWhere(sqlrservercursor *cursor,
						const char *table,
						tdsrow *row,
						stringbuffer *query,
						memorypool *bindpool,
						sqlrserverbindvar *binds,
						uint16_t *bindcount);

		const char	*positionedColumn(sqlrservercursor *cursor,
							uint16_t param);
		void	positionedBind(sqlrserverbindvar *bv,
						uint16_t bindindex,
						memorypool *bindpool,
						const char *value,
						uint64_t valuesize);
		void	positionedParamBind(sqlrserverbindvar *bv,
						uint16_t bindindex,
						memorypool *bindpool,
						uint16_t param);

		tdsrows	*positionRows(sqlrservercursor *cursor, bool create);
		void	releasePositionRows(sqlrservercursor *cursor);
		void	releaseAllPositionRows();

		// rpc response builders
		void	rpcResultSet(sqlrservercursor *cursor,
					bool nometadata,
					uint64_t maxrows);
		void	rpcError(sqlrservercursor *cursor,
					bool returnstatus=true);
		bool	rpcInvalidHandleError(uint32_t number,
					const char *what,
					uint32_t handle);
		bool	rpcNumberedError(uint32_t number,
					const char *msgtext);
		bool	rpcInvalidColumnError(uint16_t param);
		bool	rpcUnnumberedError(byte_t state,
					byte_t errclass,
					const char *msgtext);
		bool	rpcUnimplementedFeatureError();
		bool	rpcQueryTooLargeError(size_t querysize);
		bool	rpcNoCursorAvailableError();
		bool	rpcParamTypeError(const char *procname,
					const char *param);
		bool	rpcUnsupportedTypeError(byte_t tdstype);
		bool	paramIsUnicode(uint16_t param);

		uint32_t	newHandle();
		sqlrservercursor	*handleCursor(
					dictionary<uint32_t,
						sqlrservercursor *> *handles,
					uint32_t handle);
		bool	handlesContain(dictionary<uint32_t,
						sqlrservercursor *> *handles,
					sqlrservercursor *cursor);
		void	releaseHandles(dictionary<uint32_t,
						sqlrservercursor *> *handles,
					dictionary<uint32_t,
						sqlrservercursor *> *other);
		void	releaseCursorHandles(sqlrservercursor *cursor);
		void	evictOldestHandle(sqlrservercursor *keep);
		sqlrservercursor	*availableCursor(
					sqlrservercursor *keep=NULL);
		void	releaseCursor(sqlrservercursor *cursor);

		char	*callSyntaxToExec(const char *stmt);

		// length-prefixed string writers - "lensize" is the size
		// of the length prefix (1, 2, or 4 bytes) and "length" is
		// a count of characters at the session's character width,
		// which is single-byte for pre-tds7 clients and ucs-2
		// otherwise.  So for a pre-tds7 session "length" is a byte
		// count, and for a tds7 session it isn't.  The characters
		// themselves are written at that same width.
		size_t	charSize();
		size_t	varcharSize(size_t lensize, size_t length);
		void	writeVarcharLength(bytebuffer *buffer,
					size_t lensize,
					size_t length);
		void	writeVarchar(bytebuffer *buffer,
					size_t lensize,
					const char *str,
					size_t length);
		void	writeVarchar(bytebuffer *buffer,
					size_t lensize,
					const wchar_t *str,
					size_t length);

		void	envChange(byte_t type,
					const wchar_t *newvalue,
					size_t newvaluelen,
					const wchar_t *oldvalue,
					size_t oldvaluelen);

		// info/error token builders - these only append to the
		// response packet, the caller sends it
		void	appendInfo(uint32_t number,
					byte_t state,
					byte_t infoclass,
					const char *msgtext,
					const char *servername,
					const char *procname,
					uint32_t linenumber);
		void	appendError(uint32_t number,
					byte_t state,
					byte_t errclass,
					const char *msgtext,
					const char *servername,
					const char *procname,
					uint32_t linenumber);
		void	appendInfoOrError(byte_t token,
					uint32_t number,
					byte_t state,
					byte_t infoerrclass,
					const char *msgtext,
					const char *servername,
					const char *procname,
					uint32_t linenumber);
		uint32_t	appendQueryError(sqlrservercursor *cursor);

		// error senders - these clear the response packet, append
		// the error, append a done, and send it
		bool	sendError(uint32_t number,
					byte_t state,
					byte_t errclass,
					const char *msgtext,
					uint32_t linenumber);
		bool	sendUnimplementedFeatureError();
		bool	sendTlsRequiredError();
		bool	sendTdsProtocolError();
		void	queryTooLargeMessage(size_t querysize,
					stringbuffer *err);
		bool	sendQueryTooLargeError(size_t querysize);
		bool	sendNoCursorAvailableError();
		bool	sendLoginRequiredError();
		bool	sendAlreadyLoggedInError();

		uint16_t	transState();
		void	done();
		void	done(uint16_t status,
					uint16_t curcmdortransstate,
					uint64_t donerowcount);
		void	done(byte_t token,
					uint16_t status,
					uint16_t curcmdortransstate,
					uint64_t donerowcount);
		// A done-in-proc and a done-proc carry the same
		// CurCmd-or-TransState second uint16 that a done does.
		// Both take a CurCmd, and both ignore it for a tds 5.0
		// session and send what transState() picks instead -
		// see their definitions.
		void	doneInProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount);
		void	returnStatus(uint32_t value);
		uint32_t	procReturnValue(sqlrservercursor *cursor);
		void	procReturnValues(sqlrservercursor *cursor);
		void	returnValues(sqlrservercursor *cursor);
		void	preTds7ReturnValues(sqlrservercursor *cursor);
		void	returnValue(sqlrservercursor *cursor,
					uint16_t param,
					uint16_t ordinal);
		void	returnValueChar(sqlrserverbindvar *bv,
					byte_t tdstype,
					uint32_t maxsize);
		void	returnValueDateTime(sqlrserverbindvar *bv,
					byte_t tdstype,
					uint32_t maxsize);
		void	returnValueDecimal(sqlrserverbindvar *bv,
					byte_t tdstype,
					byte_t precision,
					byte_t scale);
		void	returnValueInteger(uint16_t ordinal,
					int32_t value,
					bool isnull);
		void	preTds7ReturnValueInteger(int32_t value,
					bool isnull);
		void	returnValueHeader(uint16_t ordinal,
						const char *name,
						uint16_t namesize,
						uint32_t usertype);
		void	writeIntN(int64_t value, byte_t size);
		void	writeFloatN(double value, byte_t size);
		void	doneProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount);

		void	debugSystemError();

		void	debugPacketType(const char *name, byte_t type);
		void	debugPacketStatus(byte_t status);
		void	debugTokenType(byte_t token);
		void	debugPreTds7TokenType(byte_t token);
		void	debugPreLoginOption(byte_t opt);
		void	debugEncryptionOption(const char *name, byte_t enc);
		void	debugLogin7OptionFlags(byte_t optionflags1,
						byte_t optionflags2,
						byte_t typeflags,
						byte_t optionflags3);
		void	debugEnvChangeType(byte_t type);
		void	debugDoneStatus(uint16_t status);
		void	debugAllHeadersType(uint16_t type);
		void	debugColumnType(byte_t type);
		void	debugPreTds7ColumnType(byte_t type);
		void	debugCollation(uint32_t lcid, byte_t sortid);
		void	debugProcId(uint16_t procid);

		// the same socket until tls is negotiated, after which
		// clientsock points at tlsstream
		filedescriptor	*rawclientsock;
		filedescriptor	*clientsock;

		tdssharedfd	tlsstream;

		tdstlsframer	*tlsframer;

		enum	tlsmode_t {
			TLS_MODE_NONE=0,
			TLS_MODE_LOGIN,
			TLS_MODE_SESSION
		};
		tlsmode_t	tlsmode;

		bool		tlsrefused;

		bool		tlsactive;

		uint32_t	configtdsversion;
		uint32_t	configpacketsize;
		uint32_t	configmaxpacketsize;

		uint32_t	maxquerysize;
		uint64_t	maxrequestsize;
		uint16_t	maxbindcount;

		char		**bindvarnames;
		int16_t		*bindvarnamesizes;

		const char	*srvname;

		byte_t		packetid;

		memorypool	reqpacketpool;
		bytebuffer	reqpacket;
		bytebuffer	resppacket;

		const char	*dbversion;
		uint32_t	servertdsversion;
		uint32_t	clienttdsversion;
		uint32_t	negotiatedtdsversion;

		// tds 5.0 and earlier send single-byte strings where tds 7.0
		// and later send ucs-2.  This can't be derived from
		// negotiatedtdsversion because init() presets that to 700,
		// which would send anything written before the version is
		// actually negotiated out tds7-shaped.
		bool		pretds7;

		// What a pre-tds7 login record declared about character
		// data.  The module instance outlives the session, so
		// init() clears all of these - a client that gets this
		// connection next must not inherit the previous client's
		// charset.
		//
		// "clientcharset" is the name the client sent, kept because
		// the charset envchange names it back, and
		// clientcharsetinenc/clientcharsetoutenc are the encodings
		// it looked up to in pretds7charsets[] - both NULL when
		// nothing is to be converted.  A zero clientcharsetlen
		// means the client named no charset at all, which freetds
		// does on purpose ("use empty charset to handle conversions
		// on client", its login.c says), and which is answered with
		// no envchange at all.
		char		clientcharset[PRE_TDS7_NAME_SIZE+1];
		byte_t		clientcharsetlen;
		const char	*clientcharsetinenc;
		const char	*clientcharsetoutenc;

		// The language the same record declared, and its "don't
		// send me the language-change message" flag
		char		clientlanguage[PRE_TDS7_NAME_SIZE+1];
		byte_t		clientlanguagelen;
		bool		clientsuppresslanguage;

		uint32_t	oldpacketsize;
		uint32_t	negotiatedpacketsize;

		bool		dbistds;
		bool		dbisase;
		bool		binaryishextext;
		bool		imageishextext;

		bool		loggedin;

		// The tds 5.0 capability masks this session settled on.
		// "client*" is what the login's capability token declared,
		// "granted*" is what capability() answered with; the answer
		// is the declaration intersected with what this module
		// supports, so a granted bit is one both sides agreed to.
		// clientcapabilities is false when the client sent no
		// capability token at all, which leaves every mask empty.
		bool		clientcapabilities;
		byte_t		clientrequestmask[MAX_CAPABILITY_MASK_BYTES];
		byte_t		clientrequestmasklen;
		byte_t		clientresponsemask[MAX_CAPABILITY_MASK_BYTES];
		byte_t		clientresponsemasklen;
		byte_t		grantedrequestmask[MAX_CAPABILITY_MASK_BYTES];
		byte_t		grantedrequestmasklen;
		byte_t		grantedresponsemask[MAX_CAPABILITY_MASK_BYTES];
		byte_t		grantedresponsemasklen;

		// whether the packet being received is the client's answer
		// to a sec_encrypt negotiate.  the blobs in it are
		// password-equivalent, so recvPacket() masks its raw dump.
		bool		secencryptreply;

		// rpc parameters, as they arrived on the wire
		memorypool		rpcparampool;
		sqlrserverbindvar	*rpcparams;
		bool			*rpcparambyref;
		char			**rpcparamnames;
		uint16_t		*rpcparamnamesizes;
		// always an ms-tds type byte, whichever dialect the
		// parameter arrived in - see tds5TypeToMsType()
		byte_t			*rpcparamtdstypes;
		// the raw tds 5.0 type byte, when that's where it came
		// from, so that a reply can echo the type the client
		// declared rather than its ms-tds equivalent.  0 for a
		// parameter that arrived over ms-tds.
		byte_t			*rpcparamtds5types;
		uint32_t		*rpcparammaxsizes;
		byte_t			*rpcparamprecisions;
		byte_t			*rpcparamscales;
		uint16_t		*outbindparams;
		uint16_t		rpcparamcount;

		// the format the most recent tds 5.0 paramfmt declared for
		// each parameter, kept until the params token behind it has
		// been read, and the pool its names live in
		memorypool		pretds7paramfmtpool;
		tds5paramfmt		*pretds7paramfmts;
		uint16_t		pretds7paramfmtcount;

		// the output parameters of the proc being answered, packed
		// down from the cursor's output binds so that the paramfmt
		// and the params token that replays it index alike
		tds5paramfmt		*pretds7outfmts;
		sqlrserverbindvar	*pretds7outbinds;

		bool			rpcfailed;

		bool			rpcunsupportedtype;

		// the "insert bulk" statement and the bulk load packet
		// arrive in separate requests, so these are kept between them
		memorypool		bulkpool;
		char			*bulktable;
		char			**bulkcolumns;
		uint16_t		bulkcolumncount;
		byte_t			*bulktypes;
		uint32_t		*bulksizes;
		byte_t			*bulkscales;
		bool			*bulkpartlens;

		// a client can hold a prepared statement handle and a cursor
		// derived from it at the same time, so these are independent
		dictionary<uint32_t, sqlrservercursor *>	stmthandles;
		dictionary<uint32_t, sqlrservercursor *>	cursorhandles;
		uint32_t					nexthandle;

		// tds 5.0 dynamic sql names each prepared statement with a
		// string id the client picks, rather than with a handle the
		// server mints.  this maps one onto the other; the cursor
		// itself sits in stmthandles like any other prepared
		// statement's, so eviction and cleanup need no special case.
		dictionary<char *, uint32_t>			dynamicids;

		sqlrservercursor				*pendingcursor;

		regularexpression	forupdateof;

		// false once a cursor has been executed, so that fetching
		// more rows doesn't run the query again
		dictionary<sqlrservercursor *, bool>		executeflag;

		// how many bind markers each prepared statement has, so
		// that sp_execute can ignore parameters past the last one
		dictionary<sqlrservercursor *, uint16_t>	bindmarkercount;

		dictionary<sqlrservercursor *, tdsrows *>	positionrows;
};

sqlrprotocol_tds::sqlrprotocol_tds(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrprotocol(cont,parameters) {

	rawclientsock=NULL;
	clientsock=NULL;

	tlsframer=new tdstlsframer(this);
	tlsmode=TLS_MODE_NONE;
	tlsrefused=false;
	tlsactive=false;

	// overrides the version that getServerTdsVersion() guesses from the
	// backend's version string, for backends it can't work out on its own
	configtdsversion=(uint32_t)charstring::convertToInteger(
				parameters->getAttributeValue("tdsversion"));

	// the packet size to use until the client negotiates one, and the
	// largest one that it's allowed to negotiate
	configpacketsize=(uint32_t)charstring::convertToInteger(
				parameters->getAttributeValue("packetsize"));
	if (!configpacketsize) {
		configpacketsize=DEFAULT_PACKET_SIZE;
	}
	configmaxpacketsize=(uint32_t)charstring::convertToInteger(
				parameters->getAttributeValue("maxpacketsize"));
	if (!configmaxpacketsize ||
			configmaxpacketsize>MAX_PACKET_SIZE) {
		configmaxpacketsize=MAX_PACKET_SIZE;
	}
	if (configpacketsize>configmaxpacketsize) {
		configpacketsize=configmaxpacketsize;
	}

	debugStart("parameters");
	debugWrite("tdsversion: %d",configtdsversion);
	debugWrite("packetsize: %d",configpacketsize);
	debugWrite("maxpacketsize: %d",configmaxpacketsize);
	debugEnd();

	const char	*dbtype=cont->getDbType();
	dbistds=(!charstring::compare(dbtype,"freetds") ||
			!charstring::compare(dbtype,"sap"));

	// The ct-lib-based connection modules hand back binary and varbinary
	// column values as hex text rather than as bytes, because ct-lib's
	// own binary-to-char conversion renders them that way and neither
	// module decodes them.  sap does the same with image; freetds
	// decodes image itself.  Every other connection module returns the
	// raw bytes.  Either way the value has to reach the client as bytes.
	binaryishextext=dbistds;
	imageishextext=(!charstring::compare(dbtype,"sap"));

	maxquerysize=cont->getConfig()->getMaxQuerySize();
	maxrequestsize=(uint64_t)maxquerysize*16;
	if (maxrequestsize<MIN_MAX_REQUEST_SIZE) {
		maxrequestsize=MIN_MAX_REQUEST_SIZE;
	}
	maxbindcount=cont->getConfig()->getMaxBindCount();

	// the keys are duplicated on the way in, so the map owns them
	dynamicids.setManageArrayKeys(true);

	bindvarnames=new char *[maxbindcount];
	bindvarnamesizes=new int16_t[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		charstring::printf(&bindvarnames[i],"@%d",i+1);
		bindvarnamesizes[i]=charstring::getLength(bindvarnames[i]);
	}

	rpcparams=new sqlrserverbindvar[maxbindcount];
	rpcparambyref=new bool[maxbindcount];
	rpcparamnames=new char *[maxbindcount];
	rpcparamnamesizes=new uint16_t[maxbindcount];
	rpcparamtdstypes=new byte_t[maxbindcount];
	rpcparamtds5types=new byte_t[maxbindcount];
	rpcparammaxsizes=new uint32_t[maxbindcount];
	rpcparamprecisions=new byte_t[maxbindcount];
	rpcparamscales=new byte_t[maxbindcount];
	outbindparams=new uint16_t[maxbindcount];
	rpcparamcount=0;

	pretds7paramfmts=new tds5paramfmt[maxbindcount];
	pretds7paramfmtcount=0;

	pretds7outfmts=new tds5paramfmt[maxbindcount];
	pretds7outbinds=new sqlrserverbindvar[maxbindcount];

	bulkcolumns=new char *[maxbindcount];
	bulktypes=new byte_t[maxbindcount];
	bulksizes=new uint32_t[maxbindcount];
	bulkscales=new byte_t[maxbindcount];
	bulkpartlens=new bool[maxbindcount];

	forupdateof.setPattern(
		"\\sfor\\s+update(\\s+of\\s+[a-z0-9_.,\\s]+)?\\s*$",
		REGULAR_EXPRESSION_CASE_INSENSITIVE);
	forupdateof.study();

	init();
}

sqlrprotocol_tds::~sqlrprotocol_tds() {
	free();

	// the tls context keeps the framer for as long as it's bound to it,
	// and outlives this class
	getTlsContext()->setFileDescriptor(NULL);
	delete tlsframer;

	for (uint16_t i=0; i<maxbindcount; i++) {
		delete[] bindvarnames[i];
	}
	delete[] bindvarnames;
	delete[] bindvarnamesizes;

	delete[] rpcparams;
	delete[] rpcparambyref;
	delete[] rpcparamnames;
	delete[] rpcparamnamesizes;
	delete[] rpcparamtdstypes;
	delete[] rpcparamtds5types;
	delete[] rpcparammaxsizes;
	delete[] rpcparamprecisions;
	delete[] rpcparamscales;
	delete[] outbindparams;

	delete[] pretds7paramfmts;

	delete[] pretds7outfmts;
	delete[] pretds7outbinds;

	delete[] bulkcolumns;
	delete[] bulktypes;
	delete[] bulksizes;
	delete[] bulkscales;
	delete[] bulkpartlens;
}

tdsrows::tdsrows(sqlrprotocol_tds *tds) {
	this->tds=tds;
	colcount=0;
}

void tdsrows::reset(uint32_t cc) {
	tds->debugStart("position rows reset");
	tds->debugWrite("colcount: %d",cc);
	tds->debugEnd();
	rows.clear();
	pool.clear();
	colcount=cc;
}

tdsrow *tdsrows::newRow() {

	tds->debugStart("position rows new row");

	if (!colcount || rows.getCount()>=CURSOR_MAX_POSITION_ROWS) {
		tds->debugWrite("failed");
		tds->debugEnd();
		return NULL;
	}

	tdsrow	*row=(tdsrow *)pool.allocate(sizeof(tdsrow));
	row->values=(char **)pool.allocate(sizeof(char *)*colcount);
	row->sizes=(uint64_t *)pool.allocate(sizeof(uint64_t)*colcount);
	for (uint32_t i=0; i<colcount; i++) {
		row->values[i]=NULL;
		row->sizes[i]=0;
	}
	rows.append(row);

	tds->debugWrite("row: %lld",(long long)rows.getCount());
	tds->debugEnd();

	return row;
}

void tdsrows::setField(tdsrow *row, uint32_t col,
				const char *value, uint64_t size, bool null) {

	if (tds->getDebug()) {
		tds->debugStart("position rows set field");
		tds->debugWrite("col: %d",col);
		tds->debugWrite("size: %lld",(long long)size);
		tds->debugWrite("null: %d",null);
		tds->debugEnd();
	}

	if (!row || col>=colcount || null || !value) {
		return;
	}

	char	*copy=(char *)pool.allocate(size+1);
	bytestring::copy(copy,value,size);
	copy[size]='\0';
	row->values[col]=copy;
	row->sizes[col]=size;
}

tdsrow *tdsrows::getRow(uint64_t rownum) {
	tds->debugStart("position rows get row");
	tds->debugWrite("rownum: %lld",(long long)rownum);

	// rownum is 1-based, the way sp_cursor numbers rows
	if (!rownum || rownum>rows.getCount()) {
		tds->debugWrite("failed");
		tds->debugEnd();
		return NULL;
	}
	listnode<tdsrow *>	*node=rows.getFirst();
	for (uint64_t i=1; node && i<rownum; i++) {
		node=node->getNext();
	}

	tds->debugEnd();

	return (node)?node->getValue():NULL;
}

uint32_t tdsrows::getColCount() {
	return colcount;
}

tdstlsframer::tdstlsframer(sqlrprotocol_tds *tds) : tdssharedfd() {
	this->tds=tds;
	framing=false;
	readbuffer=new byte_t[MAX_PACKET_SIZE];
	readbuffersize=0;
	readposition=0;
	packetid=0;
}

tdstlsframer::~tdstlsframer() {
	delete[] readbuffer;
}

void tdstlsframer::setFraming(bool framing) {

	tds->debugStart("tls set framing");
	tds->debugWrite("framing: %d",framing);
	tds->debugEnd();

	// only a fresh handshake discards what's staged - the client's last
	// handshake packet can carry bytes past the end of the handshake,
	// which the pass-through read below still has to serve
	if (framing) {
		readbuffersize=0;
		readposition=0;
		packetid=0;
	}
	this->framing=framing;
}

ssize_t tdstlsframer::lowLevelRead(void *buf, size_t count) {

	// pass through, once the handshake is out of the way
	if (!framing) {

		ssize_t	result;

		// anything the handshake left staged comes first
		if (readposition<readbuffersize) {
			uint32_t	available=readbuffersize-readposition;
			if (count>available) {
				count=available;
			}
			bytestring::copy(buf,readbuffer+readposition,count);
			readposition+=(uint32_t)count;
			result=(ssize_t)count;
		} else {
			result=tds->rawclientsock->read((byte_t *)buf,count);
		}

		if (tds->getDebug()) {
			tds->debugStart("tls read");
			tds->debugWrite("count: %lld",(long long)count);
			tds->debugWrite("result: %lld",(long long)result);
			tds->debugEnd();
		}

		return result;
	}

	// get another packet, if the last one has been used up
	if (readposition==readbuffersize && !readPacket()) {
		return -1;
	}

	// serve as much of it as was asked for
	uint32_t	available=readbuffersize-readposition;
	if (count>available) {
		count=available;
	}
	bytestring::copy(buf,readbuffer+readposition,count);
	readposition+=(uint32_t)count;
	return (ssize_t)count;
}

bool tdstlsframer::readPacket() {

	// loop past empty packets
	// (returning 0 bytes would look like the client hung up)
	for (;;) {

		// get the header
		byte_t	header[PACKET_HEADER_SIZE];
		if (tds->rawclientsock->read(header,sizeof(header))!=
						(ssize_t)sizeof(header)) {
			tds->debugStart("tls recv");
			tds->debugWrite("read packet header failed");
			tds->debugEnd();
			return false;
		}

		// only the size matters here, and it's big-endian
		uint32_t	packetsize=
				((uint32_t)header[2]<<8)|(uint32_t)header[3];
		if (packetsize<PACKET_HEADER_SIZE) {
			tds->debugStart("tls recv");
			tds->debugWrite("invalid packet size: %d",packetsize);
			tds->debugEnd();
			return false;
		}
		uint32_t	datasize=packetsize-PACKET_HEADER_SIZE;

		// get the payload
		if (datasize && tds->rawclientsock->read(readbuffer,datasize)!=
							(ssize_t)datasize) {
			tds->debugStart("tls recv");
			tds->debugWrite("read packet data failed");
			tds->debugEnd();
			return false;
		}

		tds->debugStart("tls recv");
		tds->debugPacketType("packet type",header[0]);
		tds->debugPacketStatus(header[1]);
		tds->debugWrite("packet size: %d",packetsize);
		tds->debugHexDump(readbuffer,datasize);
		tds->debugEnd();

		if (datasize) {
			readbuffersize=datasize;
			readposition=0;
			return true;
		}
	}
}

ssize_t tdstlsframer::lowLevelWrite(const void *buf, size_t count) {

	// pass through, once the handshake is out of the way (the client
	// socket buffers writes, and nothing above knows to flush it)
	if (!framing) {
		ssize_t	result=tds->rawclientsock->write(
						(const byte_t *)buf,count);
		if (result>0 && !tds->rawclientsock->flushWriteBuffer(-1,-1)) {
			return -1;
		}
		return result;
	}

	// frame the records, splitting them across packets the way
	// sendPacket() does (they go out as tabular results, which is what
	// a real server answers a pre-login with)
	const byte_t	*data=(const byte_t *)buf;
	uint64_t	remaining=count;
	uint32_t	maxdatasize=
			(tds->negotiatedpacketsize>PACKET_HEADER_SIZE)?
				tds->negotiatedpacketsize-PACKET_HEADER_SIZE:
				MIN_PACKET_SIZE-PACKET_HEADER_SIZE;

	do {

		uint32_t	datasize=(remaining>maxdatasize)?
						maxdatasize:
						(uint32_t)remaining;
		remaining-=datasize;

		uint32_t	packetsize=datasize+PACKET_HEADER_SIZE;

		byte_t	header[PACKET_HEADER_SIZE];
		header[0]=TABULAR_RESULT;
		header[1]=(remaining)?STATUS_NORMAL:STATUS_EOM;
		header[2]=(byte_t)(packetsize>>8);
		header[3]=(byte_t)(packetsize&0xFF);
		header[4]=0;
		header[5]=0;
		header[6]=packetid;
		header[7]=0;

		tds->debugStart("tls send");
		tds->debugPacketType("packet type",header[0]);
		tds->debugPacketStatus(header[1]);
		tds->debugWrite("packet size: %d",packetsize);
		tds->debugHexDump(data,datasize);
		tds->debugEnd();

		if (tds->rawclientsock->write(header,sizeof(header))!=
					(ssize_t)sizeof(header) ||
			tds->rawclientsock->write(data,datasize)!=
					(ssize_t)datasize) {
			tds->debugStart("tls send");
			tds->debugWrite("write packet failed");
			tds->debugEnd();
			return -1;
		}

		data+=datasize;
		packetid=(byte_t)((packetid+1)%256);

	} while (remaining);

	if (!tds->rawclientsock->flushWriteBuffer(-1,-1)) {
		tds->debugStart("tls send");
		tds->debugWrite("flush write buffer failed");
		tds->debugEnd();
		return -1;
	}

	return (ssize_t)count;
}

void sqlrprotocol_tds::init() {

	debugStart("init");
	debugEnd();

	packetid=0;

	// the backend can change between sessions, so these have to be
	// re-read rather than cached once in the constructor
	srvname=cont->getDbHostName();
	dbversion=cont->getDbVersion();
	getServerTdsVersion();

	// The database in play, not the connection module, decides some
	// divergences - odbc and freetds each reach both an ASE and a SQL
	// Server, so check the version string; sap always means ASE.
	dbisase=(!charstring::compare(cont->getDbType(),"sap") ||
			charstring::contains(dbversion,
					"Adaptive Server Enterprise"));

	clienttdsversion=700;
	negotiatedtdsversion=700;
	pretds7=false;

	// likewise, the charset and language a pre-tds7 login record
	// declared say nothing about the next session on this instance,
	// and a leftover charset would convert its character data
	clientcharset[0]='\0';
	clientcharsetlen=0;
	clientcharsetinenc=NULL;
	clientcharsetoutenc=NULL;
	clientlanguage[0]='\0';
	clientlanguagelen=0;
	clientsuppresslanguage=false;

	// Tds 7.x is little-endian by spec, and a pre-tds7 client declares
	// its own order in its login record - see preTds7ByteOrder().  The
	// module instance outlives the session, so a big-endian client's
	// setting must not leak into the next session on this instance.
	setProtocolIsBigEndian(false);

	oldpacketsize=configpacketsize;
	negotiatedpacketsize=configpacketsize;

	// the previous session may have left tls in front of the socket
	if (tlsactive) {
		stopTls();
	}
	tlsframer->setFraming(false);
	tlsmode=TLS_MODE_NONE;
	tlsrefused=false;

	// the module instance outlives the session, so a client that gets
	// this connection next must not inherit the previous client's login
	loggedin=false;

	// likewise, the capabilities the previous client negotiated say
	// nothing about this one
	clientcapabilities=false;
	bytestring::zero(clientrequestmask,sizeof(clientrequestmask));
	clientrequestmasklen=0;
	bytestring::zero(clientresponsemask,sizeof(clientresponsemask));
	clientresponsemasklen=0;
	bytestring::zero(grantedrequestmask,sizeof(grantedrequestmask));
	grantedrequestmasklen=0;
	bytestring::zero(grantedresponsemask,sizeof(grantedresponsemask));
	grantedresponsemasklen=0;

	secencryptreply=false;

	// start at SQLRELAY_HANDLE_BASE, to stay disjoint from the handles
	// the real backend issues (see newHandle())
	nexthandle=SQLRELAY_HANDLE_BASE;
	pendingcursor=NULL;

	rpcparamcount=0;
	rpcfailed=false;
	rpcunsupportedtype=false;

	pretds7paramfmtcount=0;

	bulktable=NULL;
	bulkcolumncount=0;
}

void sqlrprotocol_tds::free() {

	debugStart("free");
	debugEnd();

	reqpacketpool.clear();
	reqpacket.clear();
	resppacket.clear();

	rpcparampool.clear();

	pretds7paramfmtpool.clear();
	pretds7paramfmtcount=0;

	bulkpool.clear();
	bulktable=NULL;
	bulkcolumncount=0;

	// the session's cursors get released with the session, so these
	// just have to forget about them
	stmthandles.clear();
	cursorhandles.clear();
	dynamicids.clear();
	pendingcursor=NULL;
	executeflag.clear();
	bindmarkercount.clear();
	releaseAllPositionRows();
}

void sqlrprotocol_tds::reInit() {
	debugStart("reinit");
	debugEnd();
	free();
	init();
}

byte_t *sqlrprotocol_tds::convertCharset(const byte_t *inbuf,
						size_t insize,
						const char *inenc,
						const char *outenc,
						size_t *outsize) {

	debugStart("convert charset");
	debugWrite("inenc: %s",inenc);
	debugWrite("outenc: %s",outenc);
	debugWrite("insize: %lld",(long long)insize);

	// size the output buffer - 4 bytes per input byte covers every
	// conversion done here, plus room for a 1 or 2 byte null terminator
	size_t	outbufsize=insize*4+2;
	byte_t	*outbuf=new byte_t[outbufsize];

	// convert
	// FIXME: reuse the converter rather than re-creating it over and over
	iconvert	ic;
	ic.setFromEncoding(inenc);
	ic.setFromBuffer(inbuf);
	ic.setFromBufferSize(insize);
	ic.setToEncoding(outenc);
	ic.setToBuffer(outbuf);
	ic.setToBufferSize(outbufsize);
	if (!ic.convert()) {
		debugWrite("charset conversion from %s to %s failed",
							inenc,outenc);
		debugEnd();
		delete[] outbuf;
		return NULL;
	}

	*outsize=(size_t)(ic.getToBufferPosition()-outbuf);

	// null-terminate
	outbuf[*outsize]='\0';
	outbuf[(*outsize)+1]='\0';

	debugWrite("outsize: %lld",(long long)(*outsize));
	debugEnd();

	return outbuf;
}

char *sqlrprotocol_tds::ucs2ToUtf8(const ucs2_t *str,
					size_t chars,
					size_t *size) {

	debugStart("ucs2 to utf8");
	debugWrite("chars: %lld",(long long)chars);

	char	*out=(char *)convertCharset((const byte_t *)str,
						chars*sizeof(ucs2_t),
						TDS_UNICODE_CHARSET,
						TDS_BACKEND_CHARSET,
						size);
	if (out) {
		debugWrite("size: %lld",(long long)(*size));
		debugEnd();
		return out;
	}

	// fall back to a narrowing copy, which is lossy but keeps the
	// ascii that most statements are made of
	out=charstring::duplicateUcs2(str,chars);
	*size=charstring::getLength(out);

	debugWrite("fell back to narrowing copy");
	debugWrite("size: %lld",(long long)(*size));
	debugEnd();

	return out;
}

ucs2_t *sqlrprotocol_tds::utf8ToUcs2(const char *str,
					size_t size,
					size_t *chars) {

	debugStart("utf8 to ucs2");
	debugWrite("size: %lld",(long long)size);

	size_t	outsize;
	ucs2_t	*out=(ucs2_t *)convertCharset((const byte_t *)str,size,
						TDS_BACKEND_CHARSET,
						TDS_UNICODE_CHARSET,
						&outsize);
	if (out) {
		*chars=outsize/sizeof(ucs2_t);
		debugWrite("chars: %lld",(long long)(*chars));
		debugEnd();
		return out;
	}

	// fall back to a widening copy
	out=ucs2charstring::duplicate(str,size);
	*chars=size;

	debugWrite("fell back to widening copy");
	debugWrite("chars: %lld",(long long)(*chars));
	debugEnd();

	return out;
}

char *sqlrprotocol_tds::utf8ToCp1252(const char *str,
					size_t size,
					size_t *outsize) {

	debugStart("utf8 to cp1252");
	debugWrite("size: %lld",(long long)size);

	char	*out=(char *)convertCharset((const byte_t *)str,size,
						TDS_BACKEND_CHARSET,
						TDS_NONUNICODE_CHARSET,
						outsize);
	if (out) {
		debugWrite("outsize: %lld",(long long)(*outsize));
		debugEnd();
		return out;
	}

	// fall back to passing the bytes through
	out=charstring::duplicate(str,size);
	*outsize=size;

	debugWrite("fell back to passing bytes through");
	debugWrite("outsize: %lld",(long long)(*outsize));
	debugEnd();

	return out;
}

char *sqlrprotocol_tds::clientCharsetToUtf8(const byte_t *str,
						size_t size,
						size_t *outsize) {
	if (!clientcharsetinenc || !size) {
		return NULL;
	}
	return (char *)convertCharset(str,size,
					clientcharsetinenc,
					TDS_BACKEND_CHARSET,
					outsize);
}

char *sqlrprotocol_tds::utf8ToClientCharset(const char *str,
						size_t size,
						size_t *outsize) {
	if (!clientcharsetoutenc || !size) {
		return NULL;
	}
	return (char *)convertCharset((const byte_t *)str,size,
					TDS_BACKEND_CHARSET,
					clientcharsetoutenc,
					outsize);
}

// The pre-tds7 counterpart to ucs2ToUtf8().  A tds 5.0 client sends
// single-byte characters rather than ucs-2, and they aren't nul
// terminated, so this both converts and nul-terminates.
//
// The charset converted from is the one the login record declared - see
// preTds7SetCharsetAndLanguage().  A client that declared utf8, or
// declared nothing, or declared a charset pretds7charsets[] doesn't
// cover gets its bytes passed through instead, and the outbound side
// passes the same cases through too, so the two directions agree and a
// round trip comes back intact.
char *sqlrprotocol_tds::preTds7ToUtf8(const byte_t *str,
					size_t size,
					size_t *outsize) {

	debugStart("pre-tds7 to utf8");
	debugWrite("size: %lld",(long long)size);

	char	*out=clientCharsetToUtf8(str,size,outsize);
	if (!out) {
		out=charstring::duplicate((const char *)str,size);
		*outsize=size;
		debugWrite("passed bytes through");
	}

	debugWrite("outsize: %lld",(long long)(*outsize));
	debugEnd();

	return out;
}

bool sqlrprotocol_tds::recvPacket(byte_t *packettype) {

	// clear the receive buffer
	reqpacket.clear();

	byte_t		packetstatus=0;
	uint16_t	packetsize=0;
	uint16_t	spid=0;
	byte_t		packetwindow=0;

	do {

		// get the packet type
		if (clientsock->read(packettype)!=sizeof(*packettype)) {
			debugWrite("read packet type failed");
			debugSystemError();
			return false;
		}

		// get the packet status
		if (clientsock->read(&packetstatus)!=sizeof(packetstatus)) {
			debugWrite("read packet status failed");
			debugSystemError();
			return false;
		}

		// get the packet size
		if (clientsock->read(&packetsize)!=sizeof(packetsize)) {
			debugWrite("read packet size failed");
			debugSystemError();
			return false;
		}

		// get the spid
		if (clientsock->read(&spid)!=sizeof(spid)) {
			debugWrite("read spid failed");
			debugSystemError();
			return false;
		}

		// get the packet id
		if (clientsock->read(&packetid)!=sizeof(packetid)) {
			debugWrite("read packet id failed");
			debugSystemError();
			return false;
		}

		// get the packet window
		if (clientsock->read(&packetwindow)!=sizeof(packetwindow)) {
			debugWrite("read packet window failed");
			debugSystemError();
			return false;
		}

		// Sanity checks.  TABULAR_RESULT is omitted because clients
		// never send it; it's only the type sendPacket() uses.
		if (*packettype!=SQL_BATCH &&
			*packettype!=PRE_TDS7_LOGIN &&
			*packettype!=RPC &&
			*packettype!=ATTENTION_SIGNAL &&
			*packettype!=BULK_LOAD_DATA &&
			*packettype!=FEDERATED_AUTHENTICATION_TOKEN &&
			*packettype!=TRANSACTION_MANAGER_REQUEST &&
			*packettype!=PRE_TDS7_NORMAL &&
			*packettype!=TDS7_LOGIN &&
			*packettype!=SSPI &&
			*packettype!=PRE_LOGIN) {
			debugPacketType("invalid packet type",*packettype);
			debugSystemError();
			return false;
		}
		if (packetsize<PACKET_HEADER_SIZE) {
			debugWrite("invalid packet size: %d",packetsize);
			debugSystemError();
			return false;
		}

		// bump the packet size down
		packetsize-=PACKET_HEADER_SIZE;

		// A zero-byte payload is legal only on the final packet;
		// otherwise a client could spin this loop with empty packets.
		if (!packetsize && !(packetstatus&STATUS_EOM)) {
			debugWrite("empty non-eom packet");
			debugSystemError();
			return false;
		}

		// Cap the reassembled request size; STATUS_EOM is set by the
		// client, so without this check reqpacket could grow unbounded.
		if ((uint64_t)reqpacket.getSize()+(uint64_t)packetsize>
							maxrequestsize) {
			debugWrite("request too large: %lld",
				(long long)((uint64_t)reqpacket.getSize()+packetsize));
			debugSystemError();
			return false;
		}

		// get the packet data (reqpacketpool is reused each time)
		reqpacketpool.clear();
		byte_t	*packet=reqpacketpool.allocate(packetsize);
		if (clientsock->read(packet,packetsize)!=packetsize) {
			debugWrite("read packet failed");
			debugSystemError();
			return false;
		}

		// where this packet starts in the reassembled request
		uint64_t	packetoffset=reqpacket.getSize();

		// append the data to the receive buffer
		reqpacket.append(packet,packetsize);

		// The dump below writes the packet exactly as it arrived, so
		// a login packet hands over the password that the field-level
		// output takes care to print as "(hidden)".  Blank it first.
		// reqpacket already has its own copy, so this only affects
		// the dump.
		if (getDebug()) {
			if (secencryptreply) {
				// The answer to a sec_encrypt negotiate is
				// nothing but the encrypted password and its
				// framing, so blank all of it - the tokens
				// are printed field by field as they're read.
				// Checked ahead of the packet type: whatever
				// type the client sent it as, it's still the
				// blob.
				bytestring::set(packet,'x',packetsize);
			} else if (*packettype==PRE_TDS7_LOGIN) {
				maskPreTds7Passwords(packet,packetsize,
							packetoffset);
			} else if (*packettype==TDS7_LOGIN) {
				maskTds7Passwords(packet,packetsize,
							packetoffset);
			}
		}

		debugStart("recv");
		debugPacketType("packet type",*packettype);
		debugPacketStatus(packetstatus);
		debugWrite("packet size: %d",
				packetsize+PACKET_HEADER_SIZE);
		debugWrite("spid: %d",spid);
		debugWrite("packet id: %d",packetid);
		debugWrite("packet window: %d",packetwindow);
		debugHexDump(packet,packetsize);
		debugEnd();

	} while (!(packetstatus&STATUS_EOM));

	// bump sequence
	packetid=(packetid+1)%256;

	return true;
}

bool sqlrprotocol_tds::sendPacket(byte_t packettype) {

	const byte_t	*packet=resppacket.getBuffer();
	uint64_t	remaining=resppacket.getSize();

	// the negotiated packet size includes the header
	uint32_t	maxdatasize=
			(negotiatedpacketsize>PACKET_HEADER_SIZE)?
				negotiatedpacketsize-PACKET_HEADER_SIZE:
				MIN_PACKET_SIZE-PACKET_HEADER_SIZE;

	do {

		// set header parts
		byte_t		packetstatus=0;
		uint32_t	datasize=(remaining>maxdatasize)?
						maxdatasize:
						(uint32_t)remaining;
		remaining-=datasize;
		if (!remaining) {
			packetstatus|=STATUS_EOM;
		}
		uint16_t	packetsize=datasize+PACKET_HEADER_SIZE;
		uint16_t	spid=0;
		byte_t		packetwindow=0;

		debugStart("send");
		debugPacketType("packet type",packettype);
		debugPacketStatus(packetstatus);
		debugWrite("packet size: %d",packetsize);
		debugWrite("spid: %d",spid);
		debugWrite("packet id: %d",packetid);
		debugWrite("packet window: %d",packetwindow);
		debugHexDump(packet,datasize);
		debugEnd();

		// send the packet type
		if (clientsock->write(packettype)!=sizeof(packettype)) {
			debugWrite("write packet type failed");
			debugSystemError();
			return false;
		}

		// send the packet status
		if (clientsock->write(packetstatus)!=sizeof(packetstatus)) {
			debugWrite("write packet status failed");
			debugSystemError();
			return false;
		}

		// send the packet size
		if (clientsock->write(packetsize)!=sizeof(packetsize)) {
			debugWrite("write packet size failed");
			debugSystemError();
			return false;
		}

		// send the spid
		if (clientsock->write(spid)!=sizeof(spid)) {
			debugWrite("write spid failed");
			debugSystemError();
			return false;
		}

		// send the packet id
		if (clientsock->write(packetid)!=sizeof(packetid)) {
			debugWrite("write packet id failed");
			debugSystemError();
			return false;
		}

		// send the packet window
		if (clientsock->write(packetwindow)!=sizeof(packetwindow)) {
			debugWrite("write packet window failed");
			debugSystemError();
			return false;
		}

		// send the packet data
		if (clientsock->write(packet,datasize)!=(ssize_t)datasize) {
			debugWrite("write packet data failed");
			debugSystemError();
			return false;
		}

		if (!clientsock->flushWriteBuffer(-1,-1)) {
			debugWrite("flush write buffer failed");
			debugSystemError();
			return false;
		}

		// advance past the data we just sent
		packet+=datasize;

		// bump sequence
		packetid=(packetid+1)%256;

	} while (remaining);

	// a response only goes out once - leaving it behind risks sending
	// it again if something appends to the buffer afterward
	resppacket.clear();

	return true;
}

wchar_t *sqlrprotocol_tds::readPassword(const byte_t *rp,
						size_t charcount) {

	// The decoded password itself is never logged; callers log it as
	// "(hidden)" and only its length and decode status appear here.
	debugStart("read password");
	debugWrite("charcount: %lld",(long long)charcount);

	// Callers also cap charcount, but this has to stand on its own:
	// size and i must be size_t, or a 16-bit size truncates the copy
	// and a 16-bit i with a 64-bit size never terminates the loop.
	if (charcount>MAX_LOGIN_CHARS) {
		debugWrite("charcount exceeds max login chars");
		debugEnd();
		return NULL;
	}

	size_t	size=charcount*sizeof(uint16_t);
	byte_t	*temp=(byte_t *)bytestring::duplicate(rp,size);
	if (!temp) {
		debugWrite("duplicate failed");
		debugEnd();
		return NULL;
	}
	byte_t	*ch=temp;
	for (size_t i=0; i<size; i++) {
		*ch=*ch^0xA5;
		*ch=((*ch&0x0F)<<4)|((*ch&0xF0)>>4);
		ch++;
	}
	wchar_t	*password=wcharstring::duplicateUcs2((const ucs2_t *)temp,
								charcount);
	delete[] temp;

	debugWrite("password received and decoded");
	debugEnd();

	return password;
}

void sqlrprotocol_tds::getServerTdsVersion() {

	// 0 means "couldn't tell"
	servertdsversion=0;

	// versions reported by FreeTDS...
	if (charstring::contains(dbversion,"SQL Server 2022") ||
			charstring::contains(dbversion,"SQL Server 2019") ||
			charstring::contains(dbversion,"SQL Server 2017") ||
			charstring::contains(dbversion,"SQL Server 2016") ||
			charstring::contains(dbversion,"SQL Server 2014") ||
			charstring::contains(dbversion,"SQL Server 2012")) {
		servertdsversion=740;
	} else if (charstring::contains(dbversion,"SQL Server 2008 R2")) {
		servertdsversion=731;
	} else if (charstring::contains(dbversion,"SQL Server 2008")) {
		servertdsversion=730;
	} else if (charstring::contains(dbversion,"SQL Server 2005")) {
		servertdsversion=720;
	} else if (charstring::contains(dbversion,"SQL Server 2000 SP1")) {
		servertdsversion=711;
	} else if (charstring::contains(dbversion,"SQL Server 2000")) {
		servertdsversion=710;
	} else if (charstring::contains(dbversion,"SQL Server 7.0")) {
		servertdsversion=700;
	} else if (charstring::contains(dbversion,"Adaptive Server") ||
			charstring::contains(dbversion,"SQL Anywhere")) {
		servertdsversion=500;
	} else if (charstring::contains(dbversion,"SQL Server 6.")) {
		servertdsversion=420;
	}

	// versions reported by ODBC as SQL_DBMS_VER, "##.##.####"...
	// FIXME: other versions...
	if (charstring::contains(dbversion,"12.00.2000") ||
			charstring::startsWith(dbversion,"13.") ||
			charstring::startsWith(dbversion,"14.") ||
			charstring::startsWith(dbversion,"15.") ||
			charstring::startsWith(dbversion,"16.")) {
		servertdsversion=740;
	}

	// Never report a version below 700, even for a genuinely older
	// backend.  The wire format follows the login record the client
	// sent, not the backend's version, so a LOGIN7 client has to end up
	// with a LOGIN7-era version; negotiateTdsVersion() takes the minimum
	// of the two, so an unclamped 500 here would drag a 7.x client down
	// to 500 and hang it.  This doesn't cap pre-tds7 clients - the
	// minimum keeps them at the 500 they asked for, and 500 is the only
	// pre-tds7 version that gets this far, since preTds7Login() refuses
	// the older ones outright.
	if (servertdsversion && servertdsversion<700) {
		servertdsversion=700;
	}

	// the configured version wins, for backends that we can't
	// work the version out from
	if (configtdsversion) {
		servertdsversion=configtdsversion;
	}

	debugStart("server tds version");
	debugWrite("dbversion:\n %s",dbversion);
	debugWrite("servertdsversion: %d",servertdsversion);
	debugEnd();
}

uint32_t sqlrprotocol_tds::tdsVersionHexToDec(uint32_t tdsversion) {

	debugStart("tds version hex to dec");
	debugWrite("tdsversion: 0x%08x",tdsversion);

	uint32_t	result;
	switch (tdsversion) {
		case 0x00000042:
		case 0x42000000:
		case 0x04020000:
		case 0x00000204:
			// Sybase < 10
			// SQL Server 6.x
			// (a real 4.2 client sends the version as the
			// bytes 04 02 00 00, rather than as bcd)
			result=420;
			break;
		case 0x00000050:
		case 0x05000000:
			// Sybase 10+
			// Sybase SQL Anywhere (all versions)
			result=500;
			break;
		case 0x00000070:
		case 0x07000000:
			// SQL Server 7.0
			result=700;
			break;
		case 0x00000071:
		case 0x07010000:
			// SQL Server 2000
			result=710;
			break;
		case 0x01000071:
		case 0x71000001:
			// SQL Server 2000 SP1
			result=711;
			break;
		case 0x02000972:
		case 0x72090002:
			// SQL Server 2005
			result=720;
			break;
		case 0x03000A73:
		case 0x730A0003:
			// SQL Server 2008
			result=730;
			break;
		case 0x03000B73:
		case 0x730B0003:
			// SQL Server 2008 R2
			result=731;
			break;
		case 0x04000074:
		case 0x74000004:
			// SQL Server 2012, 2014, 2016
			result=740;
			break;
		default:
			result=700;
			break;
	}

	debugWrite("result: %d",result);
	debugEnd();

	return result;
}

uint32_t sqlrprotocol_tds::tdsVersionDecToHex(uint32_t tdsversion,
							bool toclient) {

	debugStart("tds version dec to hex");
	debugWrite("tdsversion: %d",tdsversion);
	debugWrite("toclient: %d",toclient);

	uint32_t	result;
	if (toclient) {
		switch (tdsversion) {
			case 420:
				// Sybase < 10
				// SQL Server 6.x
				result=0x42000000;
				break;
			case 500:
				// Sybase 10+
				// Sybase SQL Anywhere (all versions)
				result=0x05000000;
				break;
			case 700:
				// SQL Server 7.0
				result=0x07000000;
				break;
			case 710:
				// SQL Server 2000
				result=0x07010000;
				break;
			case 711:
				// SQL Server 2000 SP1
				result=0x71000001;
				break;
			case 720:
				// SQL Server 2005
				result=0x72090002;
				break;
			case 730:
				// SQL Server 2008
				result=0x730A0003;
				break;
			case 731:
				// SQL Server 2008 R2
				result=0x730B0003;
				break;
			case 740:
				// SQL Server 2012, 2014, 2016
				result=0x74000004;
				break;
			default:
				result=0x07000000;
				break;
		}
	} else {
		switch (tdsversion) {
			case 420:
				// Sybase < 10
				// SQL Server 6.x
				result=0x00000042;
				break;
			case 500:
				// Sybase 10+
				// Sybase SQL Anywhere (all versions)
				result=0x00000050;
				break;
			case 700:
				// SQL Server 7.0
				result=0x00000070;
				break;
			case 710:
				// SQL Server 2000
				result=0x00000071;
				break;
			case 711:
				// SQL Server 2000 SP1
				result=0x01000071;
				break;
			case 720:
				// SQL Server 2005
				result=0x02000972;
				break;
			case 730:
				// SQL Server 2008
				result=0x03000A73;
				break;
			case 731:
				// SQL Server 2008 R2
				result=0x03000B73;
				break;
			case 740:
				// SQL Server 2012, 2014, 2016
				result=0x04000074;
				break;
			default:
				result=0x00000070;
				break;
		}
	}

	debugWrite("result: 0x%08x",result);
	debugEnd();

	return result;
}

void sqlrprotocol_tds::negotiateTdsVersion() {

	// If the backend's version is unknown, go with what the client
	// asked for; falling back to an older version hangs the client.
	uint32_t	stv=(servertdsversion)?servertdsversion:
					(clienttdsversion)?clienttdsversion:700;

	negotiatedtdsversion=
		(clienttdsversion<stv)?clienttdsversion:stv;

	debugStart("negotiate tds version");
	debugWrite("client: %d",clienttdsversion);
	debugWrite("server: %d",stv);
	debugWrite("negotiated: %d",negotiatedtdsversion);
	debugEnd();
}

clientsessionexitstatus_t sqlrprotocol_tds::clientSession(
							filedescriptor *cs) {

	debugStart("client session");
	debugEnd();

	rawclientsock=cs;
	clientsock=cs;

	// set up the socket
	clientsock->setTranslateByteOrder(true);
	clientsock->setNaglesAlgorithmEnabled(false);
	//clientsock->setSocketReadBufferSize(65536);
	//clientsock->setSocketWriteBufferSize(65536);
	clientsock->setReadBufferSize(65536);
	clientsock->setWriteBufferSize(65536);

	reInit();

	// state/status variables...
	bool				endsession=true;
	clientsessionexitstatus_t	status=CLIENTSESSIONEXITSTATUS_ERROR;

	// loop, getting and executing requests
	bool	loop=true;
	do {

		// get the request...
		byte_t	packettype;
		if (!recvPacket(&packettype)) {
			status=CLIENTSESSIONEXITSTATUS_CLOSED_CONNECTION;
			break;
		}

		// Reject login/non-login requests in the wrong state - a repeat
		// login request would re-auth and renegotiate mid-session.
		bool	loginrequest=(packettype==PRE_LOGIN ||
					packettype==PRE_TDS7_LOGIN ||
					packettype==TDS7_LOGIN ||
					packettype==FEDERATED_AUTHENTICATION_TOKEN ||
					packettype==SSPI);
		if (loggedin) {
			if (loginrequest) {
				sendAlreadyLoggedInError();
				break;
			}
		} else {
			if (!loginrequest) {
				sendLoginRequiredError();
				break;
			}
		}

		// some requests don't need a cursor...
		bool	loopback=false;
		switch (packettype) {
			case PRE_LOGIN:
				loop=preLogin();
				loopback=true;
				break;
			case PRE_TDS7_LOGIN:
				// a client that sends this speaks tds 5.0
				// or earlier, so everything written from
				// here on, errors included, uses single-byte
				// strings rather than ucs-2
				pretds7=true;
				loop=preTds7Login();
				loopback=true;
				break;
			case TDS7_LOGIN:
				loop=tds7Login();
				loopback=true;
				break;
			case FEDERATED_AUTHENTICATION_TOKEN:
				loop=federatedAuthenticationToken();
				loopback=true;
				break;
			case ATTENTION_SIGNAL:
				loop=attention();
				loopback=true;
				break;
			case TRANSACTION_MANAGER_REQUEST:
				loop=transactionManagerRequest();
				loopback=true;
				break;
			case SSPI:
				loop=sspi();
				loopback=true;
				break;
			default:
				break;
		}
		if (!loop) {
			break;
		}
		if (loopback) {
			continue;
		}

		// the rest of the requests operate on a cursor, which
		// they acquire or look up themselves...
		switch (packettype) {
			case SQL_BATCH:
				loop=sqlBatch();
				break;
			case PRE_TDS7_NORMAL:
				// tds 5.0's counterpart to a sql batch,
				// but token-framed, and it can carry more
				// than one command
				loop=preTds7Normal();
				break;
			case BULK_LOAD_DATA:
				loop=bulkLoad();
				break;
			case RPC:
				loop=remoteProcedureCall();
				break;
			default:
				loop=sendUnimplementedFeatureError();
				break;
		}

		// Nothing is left for an attention to cancel once the request
		// is answered; a cursor meant to stay open is in its handle map.
		pendingcursor=NULL;

	} while (loop);

	// close the client connection
	cont->closeClientConnection(0);

	// end the session if necessary
	if (endsession) {
		cont->endSession();
	}

	debugStart("client session");
	debugWrite("status: %d",(int)status);
	debugEnd();

	// return the status
	return status;
}

bool sqlrprotocol_tds::preLogin() {

	uint32_t	version=0;
	uint16_t	subbuild=0;
	byte_t		encryption=0;
	bool		sawencryption=false;
	char		*instvalidity=NULL;
	uint32_t	threadid=0;
	byte_t		mars=0;
	byte_t		connid[16];
	byte_t		activityid[20];
	byte_t		fedauthrequired=0;
	byte_t		nonce[32];

	connid[0]='\0';
	activityid[0]='\0';
	nonce[0]='\0';

	const byte_t		*rp=reqpacket.getBuffer();
	const byte_t		*startrp=rp;
	size_t			rpsize=reqpacket.getSize();
	size_t			packetsize=rpsize;

	debugStart("pre-login");
	debugWrite("receiving...");

	byte_t		plopttok;
	uint16_t	ploptoff;
	uint16_t	ploptsize;

	bool	badpacket=false;

	for (;;) {

		// get the option token
		if (!rpsize) {
			debugWrite("ran out of packet before the terminator");
			badpacket=true;
			break;
		}
		read(rp,&plopttok,&rp);
		rpsize--;
		debugPreLoginOption(plopttok);
		if (plopttok==PL_TERMINATOR) {
			break;
		}

		// get the option offset and size
		if (rpsize<sizeof(ploptoff)+sizeof(ploptsize)) {
			debugWrite("truncated option header");
			badpacket=true;
			break;
		}
		readBE(rp,&ploptoff,&rp);
		rpsize-=sizeof(ploptoff);
		debugWrite("offset: %hd",ploptoff);
		readBE(rp,&ploptsize,&rp);
		rpsize-=sizeof(ploptsize);
		debugWrite("size: %hd",ploptsize);

		// the data the option claims has to be inside the packet
		if (!fitsInPacket(ploptoff,ploptsize,packetsize)) {
			debugWrite("option data lies outside of the packet");
			badpacket=true;
			break;
		}

		// the cases below read a fixed size, whatever ploptsize
		// says, so each needs its own bound - only PL_INSTOPT reads
		// exactly ploptsize, which the check above covers

		// get the option data
		const byte_t		*dummy;
		switch (plopttok) {

			case PL_VERSION:
				// FIXME: bail if this isn't the first option
				if (!fitsInPacket(ploptoff,
						sizeof(version)+
						sizeof(subbuild),packetsize)) {
					badpacket=true;
					break;
				}
				readLE(startrp+ploptoff,&version,&dummy);
				readLE(startrp+ploptoff+sizeof(version),
							&subbuild,&dummy);
				debugWrite("pl_version");
				debugWrite("version: %d",version);
				debugWrite("subbuiild: %hd",subbuild);
				break;

			case PL_ENCRYPTION:
				if (!fitsInPacket(ploptoff,
						sizeof(encryption),packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,&encryption,&dummy);
				sawencryption=true;
				debugWrite("pl_encryption");
				debugEncryptionOption("encryption",encryption);
				break;

			case PL_INSTOPT:
				// a client can send more than one of these
				delete[] instvalidity;
				instvalidity=new char[ploptsize+1];
				read(startrp+ploptoff,
					instvalidity,ploptsize,&dummy);
				instvalidity[ploptsize]='\0';
				debugWrite("pl_instopt");
				debugWrite("instvalidity: %s",instvalidity);
				break;

			case PL_THREADID:
				if (!fitsInPacket(ploptoff,
						sizeof(threadid),packetsize)) {
					badpacket=true;
					break;
				}
				readLE(startrp+ploptoff,&threadid,&dummy);
				debugWrite("pl_threadid");
				debugWrite("threadid: %d",threadid);
				break;

			case PL_MARS:
				if (!fitsInPacket(ploptoff,
						sizeof(mars),packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,&mars,&dummy);
				debugWrite("mars");
				debugWrite("mars: %d",mars);
				break;

			case PL_TRACEID:
				if (!fitsInPacket(ploptoff,
						sizeof(connid)+
						sizeof(activityid),packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,
						connid,sizeof(connid),
						&dummy);
				read(startrp+ploptoff+sizeof(connid),
						activityid,sizeof(activityid),
						&dummy);
				debugWrite("traceid");
				debugWrite("connid: %.*s",
						(int)sizeof(connid),connid);
				debugWrite("activityid: %.*s",
						(int)sizeof(activityid),activityid);
				break;

			case PL_FEDAUTHREQUIRED:
				if (!fitsInPacket(ploptoff,
						sizeof(fedauthrequired),
						packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,
						&fedauthrequired,
						&dummy);
				debugWrite("fedauthrequired");
				debugWrite("fedauthrequired:%d",
						fedauthrequired);
				break;

			case PL_NONCEOPT:
				if (!fitsInPacket(ploptoff,
						sizeof(nonce),packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,
						nonce,sizeof(nonce),
						&dummy);
				debugWrite("nonceopt");
				debugWrite("nonce: %.*s",(int)sizeof(nonce),nonce);
				break;
		}

		if (badpacket) {
			debugWrite("option data runs past "
					"the end of the packet");
			break;
		}
	}

	// a malformed pre-login is a protocol error, and there is nothing
	// sensible to answer it with
	if (badpacket) {
		debugEnd();
		delete[] instvalidity;
		return false;
	}

	// the client may not have sent an instopt
	if (!instvalidity) {
		instvalidity=charstring::duplicate("");
	}

	debugWrite("sending...");


	// begin building the response packet
	resppacket.clear();
	bytebuffer	packetdata;

	// ploptoff must be sized for the number of tokens sent below -
	// update it if a token is added
	ploptoff=5*(sizeof(byte_t)+
				sizeof(uint16_t)+
				sizeof(uint16_t))+
			sizeof(byte_t);

	// the option offsets and sizes are big-endian, the option
	// data itself is little-endian

	// version
	write(&resppacket,(byte_t)PL_VERSION);
	writeBE(&resppacket,ploptoff);
	ploptsize=sizeof(version)+sizeof(subbuild);
	ploptoff+=ploptsize;
	writeBE(&resppacket,ploptsize);
	// FIXME: we should probably send an accurate version
	// instead of regurgitating what the client sent us
	writeLE(&packetdata,version);
	writeLE(&packetdata,subbuild);
	debugWrite("pl_version");
	debugWrite("version: %d",version);
	debugWrite("subbuiild: %hd",subbuild);

	// encryption
	write(&resppacket,(byte_t)PL_ENCRYPTION);
	writeBE(&resppacket,ploptoff);
	ploptsize=sizeof(encryption);
	ploptoff+=ploptsize;
	writeBE(&resppacket,ploptsize);
	// a client that never sent the option didn't offer a handshake, so it
	// gets the same answer as one that said it doesn't support encryption
	encryption=negotiateEncryption(
			(sawencryption)?encryption:(byte_t)ENCRYPT_NOT_SUP);
	write(&packetdata,encryption);
	debugWrite("pl_encryption");
	debugEncryptionOption("encryption",encryption);

	// instopt
	write(&resppacket,(byte_t)PL_INSTOPT);
	writeBE(&resppacket,ploptoff);
	ploptsize=charstring::getLength(instvalidity)+1;
	ploptoff+=ploptsize;
	writeBE(&resppacket,ploptsize);
	// FIXME: we should probably send an accurate instopt
	// instead of regurgitating what the client sent us
	write(&packetdata,instvalidity,ploptsize);
	debugWrite("pl_instopt");
	debugWrite("instvalidity: %s",instvalidity);
	
	// threadid
	write(&resppacket,(byte_t)PL_THREADID);
	writeBE(&resppacket,ploptoff);
	ploptsize=sizeof(threadid);
	ploptoff+=ploptsize;
	writeBE(&resppacket,ploptsize);
	threadid=process::getProcessId();
	writeLE(&packetdata,threadid);
	debugWrite("pl_threadid");
	debugWrite("threadid: %d",threadid);

	// mars
	write(&resppacket,(byte_t)PL_MARS);
	writeBE(&resppacket,ploptoff);
	ploptsize=sizeof(mars);
	ploptoff+=ploptsize;
	writeBE(&resppacket,ploptsize);
	// FIXME: SQL Relay actually does support multiple active result sets
	mars=0;
	write(&packetdata,mars);
	debugWrite("mars");
	debugWrite("mars: %d",mars);

	// no need to send traceid, fedauthrequired, or nonce in response

	// terminator
	write(&resppacket,(byte_t)PL_TERMINATOR);

	// append packet data
	write(&resppacket,packetdata.getBuffer(),packetdata.getSize());

	// send the response packet
	bool	retval=sendPacket();

	debugEnd();

	// clean up
	delete[] instvalidity;

	// the handshake follows the pre-login response immediately
	if (retval && tlsmode!=TLS_MODE_NONE) {
		retval=startTls();
	}

	return retval;
}

byte_t sqlrprotocol_tds::negotiateEncryption(byte_t clientencryption) {

	debugStart("negotiate encryption");
	debugEncryptionOption("clientencryption",clientencryption);

	tlsmode=TLS_MODE_NONE;
	tlsrefused=false;

	if (!useTls()) {
		debugWrite("tls not in use");
		debugEnd();
		return ENCRYPT_NOT_SUP;
	}

	byte_t	result;
	switch (clientencryption) {
		case ENCRYPT_OFF:
			// encrypt the login packet only
			tlsmode=TLS_MODE_LOGIN;
			result=ENCRYPT_OFF;
			break;
		case ENCRYPT_ON:
		case ENCRYPT_REQ:
			// encrypt the whole session
			tlsmode=TLS_MODE_SESSION;
			result=ENCRYPT_ON;
			break;
		default:
			// tls="yes" has no "optional" mode, and a client
			// that says it doesn't support encryption can't be
			// pushed into a handshake, so refuse the login
			tlsrefused=true;
			result=ENCRYPT_NOT_SUP;
			break;
	}

	debugEncryptionOption("result",result);
	debugEnd();

	return result;
}

bool sqlrprotocol_tds::startTls() {

	debugStart("tls");

	// cap the version, unless the operator pinned one - freetds's gnutls
	// build doesn't flush leftover buffered records after the handshake,
	// and tls 1.3's post-handshake records corrupt the stream that follows
	if (charstring::isNullOrEmpty(
				getTlsContext()->getProtocolVersion())) {
		getTlsContext()->setProtocolVersion("TLS1.2");
	}
	debugWrite("version: %s",getTlsContext()->getProtocolVersion());

	// the handshake itself arrives wrapped in tds packets
	tlsframer->setFraming(true);
	getTlsContext()->setFileDescriptor(tlsframer);
	bool	success=getTlsContext()->accept();
	tlsframer->setFraming(false);

	if (!success) {
		debugWrite("accept failed: %s",
				getTlsContext()->getErrorString());
		debugEnd();
		return false;
	}

	// from here on, reads and writes go through the tls session, which
	// reads and writes the socket through the framer
	tlsstream.setFileDescriptor(rawclientsock->getFileDescriptor());
	tlsstream.setSocketLayer(getTlsContext());
	tlsstream.setTranslateByteOrder(true);
	tlsstream.setReadBufferSize(65536);
	tlsstream.setWriteBufferSize(65536);
	clientsock=&tlsstream;
	tlsactive=true;

	debugWrite("accepted");
	debugEnd();

	return true;
}

void sqlrprotocol_tds::stopTls() {

	debugStart("tls");
	debugWrite("dropping tls");
	debugEnd();

	// go back to the socket directly, with no close-notify - the client
	// doesn't send one either and expects the next byte to start a packet
	clientsock=rawclientsock;
	tlsstream.setSocketLayer(NULL);
	tlsstream.close();
	tlsactive=false;
}

bool sqlrprotocol_tds::fitsInPacket(uint16_t offset,
						size_t size,
						size_t packetsize) {
	// subtraction rather than offset+size<=packetsize, which can wrap -
	// the sspi block's size is 32 bits, and on a 32-bit build that sum
	// overflows
	bool	result=(offset<=packetsize && size<=packetsize-offset);

	debugStart("fits in packet");
	debugWrite("offset: %hd",offset);
	debugWrite("size: %lld",(long long)size);
	debugWrite("packetsize: %lld",(long long)packetsize);
	debugWrite("result: %d",result);
	debugEnd();

	return result;
}

bool sqlrprotocol_tds::loginFieldFits(const char *name,
						uint16_t ib,
						uint16_t *cch,
						uint16_t max,
						size_t charsize,
						size_t rpsize) {

	// an empty field reads nothing, so where it points doesn't matter -
	// checking it anyway would turn an empty username or password into
	// NULL going into auth()
	if (!*cch) {
		return true;
	}

	if (*cch>max) {
		debugStart("tds7 login");
		debugWrite("%s: %hd exceeds the %hd maximum, "
				"dropping the field",name,*cch,max);
		debugEnd();
		// drop the length too - safePrint() and envChange() don't
		// survive a NULL pointer with a nonzero length
		*cch=0;
		return false;
	}

	if (!fitsInPacket(ib,(size_t)*cch*charsize,rpsize)) {
		debugStart("tds7 login");
		debugWrite("%s: (%hd,%hd) lies outside of the packet, "
				"dropping the field",name,ib,*cch);
		debugEnd();
		*cch=0;
		return false;
	}

	return true;
}

bool sqlrprotocol_tds::preTds7Login() {

	debugStart("pre-tds7 login");

	// pre-7.1 clients send this instead of a pre-login, so there's
	// never an opportunity to negotiate tls with them at all
	if (useTls()) {
		debugWrite("tls required but the client didn't negotiate it");
		debugEnd();
		sendTlsRequiredError();
		return false;
	}

	debugEnd();

	const byte_t	*rp=reqpacket.getBuffer();
	const byte_t	*startrp=rp;

	// the whole request, in bytes
	size_t		rpsize=reqpacket.getSize();

	// Unlike login7, this record has no offset/length table - every field
	// is at a fixed offset and the whole record is read straight through
	// below, so all of it has to be there.
	if (rpsize<PRE_TDS7_LOGIN_SIZE) {
		debugStart("pre-tds7 login");
		debugWrite("truncated pre-tds7 login: %lld<%d",
					(long long)rpsize,PRE_TDS7_LOGIN_SIZE);
		debugEnd();
		return false;
	}

	// initialize values...

	// the record's string fields, each with the character count that
	// came off the wire in the field's trailing length byte
	char	hostname[PRE_TDS7_NAME_SIZE+1];
	byte_t	hostnamelen=0;
	char	username[PRE_TDS7_NAME_SIZE+1];
	byte_t	usernamelen=0;
	char	password[PRE_TDS7_NAME_SIZE+1];
	byte_t	passwordlen=0;
	char	hostproc[PRE_TDS7_NAME_SIZE+1];
	byte_t	hostproclen=0;
	char	appname[PRE_TDS7_NAME_SIZE+1];
	byte_t	appnamelen=0;
	char	servername[PRE_TDS7_NAME_SIZE+1];
	byte_t	servernamelen=0;
	char	remotepassword[PRE_TDS7_REMOTE_PASSWORD_SIZE+1];
	byte_t	remotepasswordlen=0;
	char	progname[PRE_TDS7_PROGNAME_SIZE+1];
	byte_t	prognamelen=0;
	char	language[PRE_TDS7_NAME_SIZE+1];
	byte_t	languagelen=0;
	char	charset[PRE_TDS7_NAME_SIZE+1];
	byte_t	charsetlen=0;
	char	packetsizestr[PRE_TDS7_PACKET_SIZE_SIZE+1];
	byte_t	packetsizestrlen=0;

	// the record's fixed-length fields
	byte_t		typeflags[PRE_TDS7_TYPE_FLAGS_SIZE];
	byte_t		dumpload=0;
	byte_t		interfacespare=0;
	byte_t		type=0;
	uint32_t	deprecated=0;
	byte_t		spare[PRE_TDS7_SPARE_SIZE];
	uint32_t	tdsversion=0;
	uint32_t	progversion=0;
	byte_t		noshort=0;
	byte_t		flt4type=0;
	byte_t		date4type=0;
	byte_t		suppresslanguage=0;
	uint16_t	oldsecure=0;
	byte_t		seclogin=0;
	byte_t		secbulk=0;
	byte_t		halogin=0;
	byte_t		hasessionid[PRE_TDS7_SESSION_ID_SIZE];
	byte_t		secspare[PRE_TDS7_SEC_SPARE_SIZE];
	byte_t		charsetchange=0;
	byte_t		dummy[PRE_TDS7_DUMMY_SIZE];

	bytestring::zero(typeflags,sizeof(typeflags));
	bytestring::zero(spare,sizeof(spare));
	bytestring::zero(hasessionid,sizeof(hasessionid));
	bytestring::zero(secspare,sizeof(secspare));
	bytestring::zero(dummy,sizeof(dummy));

	// copy values out of the recv packet
	readPreTds7Field(rp,hostname,PRE_TDS7_NAME_SIZE,&hostnamelen,&rp);
	readPreTds7Field(rp,username,PRE_TDS7_NAME_SIZE,&usernamelen,&rp);
	readPreTds7Field(rp,password,PRE_TDS7_NAME_SIZE,&passwordlen,&rp);
	readPreTds7Field(rp,hostproc,PRE_TDS7_NAME_SIZE,&hostproclen,&rp);
	read(rp,typeflags,sizeof(typeflags),&rp);

	// Typeflags is where the client declares its byte order for 2-byte
	// ints, 4-byte ints, floats and datetimes.  The declaration covers
	// the rest of this record as much as it covers the token stream
	// after it, so decode it and set the order here, mid-parse, rather
	// than once the whole record is in hand.  A client that declared an
	// order this module can't serve is refused further down, after the
	// version check - by then the flag is little-endian, which is what
	// setProtocolIsBigEndian() gets below when the decode fails, so the
	// error tokens go out the way the common case would read them.
	bool	bigendian=false;
	bool	byteorderok=preTds7ByteOrder(typeflags,&bigendian);
	setProtocolIsBigEndian(byteorderok && bigendian);

	read(rp,&dumpload,&rp);
	read(rp,&interfacespare,&rp);
	read(rp,&type,&rp);
	read(rp,&deprecated,&rp);
	read(rp,spare,sizeof(spare),&rp);
	readPreTds7Field(rp,appname,PRE_TDS7_NAME_SIZE,&appnamelen,&rp);
	readPreTds7Field(rp,servername,PRE_TDS7_NAME_SIZE,&servernamelen,&rp);
	readPreTds7Field(rp,remotepassword,
				PRE_TDS7_REMOTE_PASSWORD_SIZE,
				&remotepasswordlen,&rp);
	readBE(rp,&tdsversion,&rp);
	readPreTds7Field(rp,progname,PRE_TDS7_PROGNAME_SIZE,&prognamelen,&rp);
	readBE(rp,&progversion,&rp);
	read(rp,&noshort,&rp);
	read(rp,&flt4type,&rp);
	read(rp,&date4type,&rp);
	readPreTds7Field(rp,language,PRE_TDS7_NAME_SIZE,&languagelen,&rp);
	read(rp,&suppresslanguage,&rp);
	read(rp,&oldsecure,&rp);
	read(rp,&seclogin,&rp);
	read(rp,&secbulk,&rp);
	read(rp,&halogin,&rp);
	read(rp,hasessionid,sizeof(hasessionid),&rp);
	read(rp,secspare,sizeof(secspare),&rp);
	readPreTds7Field(rp,charset,PRE_TDS7_NAME_SIZE,&charsetlen,&rp);
	read(rp,&charsetchange,&rp);
	readPreTds7Field(rp,packetsizestr,
				PRE_TDS7_PACKET_SIZE_SIZE,
				&packetsizestrlen,&rp);
	read(rp,dummy,sizeof(dummy),&rp);

	// the packet size arrives as ascii digits rather than as a number
	uint32_t	packetsize=(uint32_t)
				charstring::convertToUnsignedInteger(
								packetsizestr);

	// The client's tds version arrives twice - as the 4 bytes read above,
	// and as the size of the record itself (572 for 4.2, 568 for 5.0).
	// Go with the 4 bytes; tdsVersionHexToDec() knows both 0x05000000
	// (5.0) and 0x04020000 (4.2).
	clienttdsversion=tdsVersionHexToDec(tdsversion);

	// A capability token may follow the record.  What it declares is
	// kept for the whole session, in clientrequestmask/
	// clientresponsemask, because capability() answers with the
	// intersection of it and what this module supports, and because
	// what came out of that intersection decides what the client is
	// allowed to send later.
	uint16_t	capabilitiessize=0;

	// what's left of the request, after the record
	size_t	remaining=rpsize-(size_t)(rp-startrp);

	if (remaining>=sizeof(byte_t)+sizeof(uint16_t)) {

		byte_t	token=0;
		read(rp,&token,&rp);
		remaining=remaining-sizeof(byte_t);

		if (token==TOKEN_CAPABILITY) {

			clientcapabilities=true;

			read(rp,&capabilitiessize,&rp);
			remaining=remaining-sizeof(uint16_t);

			// don't trust the token's length over the
			// amount of data that actually arrived
			size_t	left=capabilitiessize;
			if (left>remaining) {
				debugStart("pre-tds7 login");
				debugWrite("truncated capability token: "
						"%lld<%lld, "
						"parsing what arrived",
						(long long)remaining,
						(long long)left);
				debugEnd();
				left=remaining;
			}

			// each capability is a type byte, a length byte,
			// and that many mask bytes
			while (left>=sizeof(byte_t)+sizeof(byte_t)) {

				byte_t	captype=0;
				byte_t	caplen=0;
				read(rp,&captype,&rp);
				read(rp,&caplen,&rp);
				left=left-sizeof(byte_t)-sizeof(byte_t);

				if ((size_t)caplen>left) {
					caplen=(byte_t)left;
				}

				byte_t	mask[MAX_CAPABILITY_MASK_BYTES];
				bytestring::zero(mask,sizeof(mask));
				read(rp,mask,(size_t)caplen,&rp);
				left=left-(size_t)caplen;

				if (captype==CAPABILITY_REQUEST) {
					bytestring::copy(clientrequestmask,
								mask,caplen);
					clientrequestmasklen=caplen;
				} else if (captype==CAPABILITY_RESPONSE) {
					bytestring::copy(clientresponsemask,
								mask,caplen);
					clientresponsemasklen=caplen;
				}
			}
		}
	}

	if (getDebug()) {
		debugStart("pre-tds7 login");
		debugWrite("request size: %lld",(long long)rpsize);
		debugWrite("hostname: (%d) %s",(int)hostnamelen,hostname);
		debugWrite("username: (%d) %s",(int)usernamelen,username);
		debugWrite("password: (%d) (hidden)",(int)passwordlen);
		debugWrite("hostproc: (%d) %s",(int)hostproclen,hostproc);
		debugWrite("typeflags: "
				"int2=%d int4=%d char=%d "
				"flt=%d date=%d usedb=%d",
				(int)typeflags[PRE_TDS7_TYPE_FLAGS_INT2],
				(int)typeflags[PRE_TDS7_TYPE_FLAGS_INT4],
				(int)typeflags[PRE_TDS7_TYPE_FLAGS_CHAR],
				(int)typeflags[PRE_TDS7_TYPE_FLAGS_FLT],
				(int)typeflags[PRE_TDS7_TYPE_FLAGS_DATE],
				(int)typeflags[PRE_TDS7_TYPE_FLAGS_USEDB]);
		debugWrite("dumpload: %d",(int)dumpload);
		debugWrite("interfacespare: %d",(int)interfacespare);
		debugWrite("type: %d",(int)type);
		debugWrite("deprecated: %d",deprecated);
		debugWrite("appname: (%d) %s",(int)appnamelen,appname);
		debugWrite("servername: (%d) %s",(int)servernamelen,servername);
		debugWrite("remotepassword: (%d) (hidden)",
						(int)remotepasswordlen);
		debugWrite("tdsversion: 0x%08x (%d)",
						tdsversion,clienttdsversion);
		debugWrite("progname: (%d) %s",(int)prognamelen,progname);
		debugWrite("progversion: 0x%08x",progversion);
		debugWrite("noshort: %d",(int)noshort);
		debugWrite("flt4type: %d",(int)flt4type);
		debugWrite("date4type: %d",(int)date4type);
		debugWrite("language: (%d) %s",(int)languagelen,language);
		debugWrite("suppresslanguage: %d",(int)suppresslanguage);
		debugWrite("oldsecure: %d",(int)oldsecure);
		debugWrite("seclogin: 0x%02x",(int)seclogin);
		debugWrite("secbulk: %d",(int)secbulk);
		debugWrite("halogin: %d",(int)halogin);
		debugWrite("hasessionid:");
		debugHexDump(hasessionid,sizeof(hasessionid));
		debugWrite("charset: (%d) %s",(int)charsetlen,charset);
		debugWrite("charsetchange: %d",(int)charsetchange);
		debugWrite("packetsize: (%d) %s (%d)",
					(int)packetsizestrlen,
					packetsizestr,packetsize);
		debugWrite("dummy:");
		debugHexDump(dummy,sizeof(dummy));
		if (clientcapabilities) {
			debugWrite("capabilities: %d bytes",
						(int)capabilitiessize);
			debugWrite("request mask: (%d)",
						(int)clientrequestmasklen);
			debugHexDump(clientrequestmask,clientrequestmasklen);
			debugWrite("response mask: (%d)",
						(int)clientresponsemasklen);
			debugHexDump(clientresponsemask,clientresponsemasklen);
		} else {
			debugWrite("capabilities: (none)");
		}
		debugEnd();
	}

	// Tds 5.0 is the only pre-tds7 dialect this module implements.  A
	// client that declares an older one (4.2, say) lays the rest of the
	// session out differently - starting with the login ack, where 4.2
	// spells success as 1 rather than as 5 - so answering it as though
	// it were 5.0 hands it a response it can't parse.  Refuse it here
	// instead.  The error itself is written pre-tds7 style, which 4.2
	// and 5.0 do have in common, so the client can display it.
	if (clienttdsversion!=500) {
		debugStart("pre-tds7 login");
		debugWrite("unsupported pre-tds7 version: 0x%08x (%d)",
					tdsversion,clienttdsversion);
		debugEnd();
		// The login is refused whether or not the error makes it
		// out, so the send result isn't the return value here, the
		// same as the tls-required refusal above.  The error tokens
		// are already shaped correctly - the only version-dependent
		// choice in them is negotiatedtdsversion<720, and every
		// pre-tds7 version is.
		sendPreTds7VersionUnsupportedError();
		return false;
	}

	// A client whose typeflags block declared more than one byte order
	// is refused for the same reason an unsupported version is:
	// answering it anyway hands it a response it can't parse.  See
	// preTds7ByteOrder() for what the decode does and doesn't refuse.
	if (!byteorderok) {
		sendPreTds7ByteOrderUnsupportedError();
		return false;
	}

	// Apply the record's charset and language fields.  This runs before
	// anything below can send character data back - an auth error, say -
	// because the charset decides how that data is encoded.  The
	// envchanges that answer both fields go out further down, once the
	// login has succeeded.
	preTds7SetCharsetAndLanguage(charset,charsetlen,
					language,languagelen,
					suppresslanguage);

	// negotiate tds version
	negotiateTdsVersion();

	// A client that asks for password encryption sets a seclogin bit and
	// sends empty password fields, then waits for the server to drive
	// the exchange that hands the password over enciphered instead.
	// Nothing below this point can tell the difference: the exchange
	// fills in the password field that the record left empty, and the
	// login goes on exactly as a cleartext one would.
	//
	// The tls and version refusals above run first, and have to: the
	// negotiate is tds 5.0 shaped, and enciphering the password is no
	// substitute for tls when tls is required.  The version negotiation
	// runs first too - the login ack that opens the exchange carries the
	// negotiated version.
	if ((seclogin&PRE_TDS7_SEC_LOG_ENCRYPT_MASK) && !passwordlen) {
		debugStart("pre-tds7 login");
		debugWrite("encrypted login requested (seclogin: 0x%02x)",
				(int)seclogin);
		debugEnd();
		if (!preTds7SecEncryptLogin(password,sizeof(password))) {
			return false;
		}
	}

	// begin building the response packet
	resppacket.clear();

	bool	retval=true;

	// auth the user
	if (auth(username,password)) {

		loggedin=true;

		// run session-start queries
		cont->beginSession();

		loginAck(PRE_TDS7_LOGIN_ACK_SUCCEED);

		// A client that sent a capability token rejects the whole
		// login response unless one comes back, so unlike the
		// envchanges a real ase also sends, this isn't optional.
		if (clientcapabilities) {
			capability();
		}

	} else {
		authError(username);

		// Unlike tds 7.x, where a failed login gets an error token
		// and nothing else, a real ase answers a failed pre-tds7
		// login with a login ack too, carrying the "failed" byte.
		loginAck(PRE_TDS7_LOGIN_ACK_FAIL);

		retval=false;
	}

	// the cleartext password isn't needed past the auth attempt, and an
	// encrypted login went to some trouble to keep it off the wire
	bytestring::zero(password,sizeof(password));

	// The envchanges that answer the login.  A real ase sends a database
	// envchange along with these, but there's nothing here to source one
	// from: a pre-tds7 login record has no database field at all - a tds
	// 5.0 client picks its database with a "use" command after the login
	// - so what a real ase reports there is just its own default-database
	// behavior.
	if (retval) {

		// change charset
		envChangeCharset();

		// change language
		preTds7ChangeLanguage();
	}

	// change packet size
	if (retval) {
		negotiatePacketSize(packetsize);
		envChangePacketSize();

		// reset "old" packet size
		oldpacketsize=negotiatedpacketsize;
	}

	// done
	done((retval)?DONE_FINAL:DONE_ERROR,0,0);

	// send the response packet, without losing a login failure
	if (!sendPacket()) {
		retval=false;
	}

	return retval;
}

void sqlrprotocol_tds::preTds7SetCharsetAndLanguage(const char *charset,
						byte_t charsetlen,
						const char *language,
						byte_t languagelen,
						byte_t suppresslanguage) {

	charstring::copy(clientcharset,charset,charsetlen);
	clientcharset[charsetlen]='\0';
	clientcharsetlen=charsetlen;

	// An empty charset isn't an error and isn't a name to look up.
	// Freetds sends one on purpose - its login.c says "use empty charset
	// to handle conversions on client" - so the bytes it sends are
	// already in whatever encoding it wants them in, and this module
	// must leave them alone.
	clientcharsetinenc=NULL;
	clientcharsetoutenc=NULL;
	if (charsetlen) {
		for (const pretds7charset *c=pretds7charsets; c->name; c++) {
			if (!charstring::compareIgnoringCase(
						clientcharset,c->name)) {
				clientcharsetinenc=c->inenc;
				clientcharsetoutenc=c->outenc;
				break;
			}
		}
	}

	charstring::copy(clientlanguage,language,languagelen);
	clientlanguage[languagelen]='\0';
	clientlanguagelen=languagelen;

	// nonzero is the client asking not to be told about a language
	// change - see preTds7ChangeLanguage()
	clientsuppresslanguage=(suppresslanguage!=0);

	debugStart("pre-tds7 charset and language");
	debugWrite("charset: %s",clientcharset);
	debugWrite("in encoding: %s",
			(clientcharsetinenc)?clientcharsetinenc:"(none)");
	debugWrite("out encoding: %s",
			(clientcharsetoutenc)?clientcharsetoutenc:"(none)");
	debugWrite("language: %s",clientlanguage);
	debugWrite("suppresslanguage: %d",(int)clientsuppresslanguage);
	debugEnd();
}

void sqlrprotocol_tds::envChangeCharset() {

	// A client that named no charset gets no answer.  Freetds names
	// none on purpose, so telling it what the session settled on would
	// be answering a question it never asked.  This is not gated on
	// whether the name was recognized: an unrecognized name leaves the
	// data passing through, and naming it back is still what a real ase
	// does with the name it was handed.
	if (!clientcharsetlen) {
		return;
	}

	debugStart("env change charset");
	debugWrite("charset: %s",clientcharset);
	debugEnd();

	// The name goes back exactly as it arrived, with no conversion:
	// clientcharset holds the client's own bytes, and writeVarchar()
	// narrows each wchar back to the byte it was widened from.  The
	// language envchange below and the packet size one after it are the
	// same - a pre-tds7 envchange value is never anything but ascii or
	// an echo of what the client sent.
	wchar_t	*charset32=wcharstring::duplicate(clientcharset,
							clientcharsetlen);

	envChange(ENV_CHANGE_CHARSET,
			charset32,clientcharsetlen,
			// This module has no charset of its own to name as
			// the old one - it speaks utf-8 internally whatever
			// the client declared - so the new value stands in,
			// the way a language envchange echoes its new value
			// as the old one.
			charset32,clientcharsetlen);

	// a real ase follows the envchange with an eed info message naming
	// the charset it changed to
	stringbuffer	inf;
	inf.append("Changed client character set setting to '");
	inf.append(clientcharset)->append("'.");

	appendInfo(5704,1,0,inf.getString(),NULL,NULL,1);

	delete[] charset32;
}

void sqlrprotocol_tds::preTds7ChangeLanguage() {

	if (!clientlanguagelen) {
		return;
	}

	wchar_t	*language32=wcharstring::duplicate(clientlanguage,
							clientlanguagelen);

	if (changeLanguage(language32,clientlanguagelen)) {

		envChange(ENV_CHANGE_LANGUAGE,
				language32,clientlanguagelen,
				// FIXME: send the actual old language
				// instead of just sending the new language
				// as the old language.  The tds7 path has
				// the same gap.
				language32,clientlanguagelen);

		// Suppresslanguage is the client saying "don't send me the
		// language-change message", and that message is all it
		// means - the envchange above still goes out.  A tds7 login
		// record has no equivalent flag, so this is the one place
		// the info message is conditional.
		if (!clientsuppresslanguage) {
			changeLanguageInfo(language32,clientlanguagelen);
		}

	} else {

		// A pre-tds7 login record carries no option flags, so
		// there's nothing in it saying whether a refused language
		// should be fatal the way fsetlangfatal does on the tds7
		// path.  Warn and carry on rather than failing a login the
		// client never said to fail.
		changeLanguageError(language32,clientlanguagelen,true);
	}

	delete[] language32;
}

bool sqlrprotocol_tds::capabilityBitIsSet(const byte_t *mask,
						byte_t masklen,
						uint16_t cap) {
	uint16_t	index=cap>>3;
	if (index>=(uint16_t)masklen) {
		return false;
	}
	return ((mask[masklen-1-index]>>(cap&7))&1)!=0;
}

void sqlrprotocol_tds::setCapabilityBit(byte_t *mask,
						byte_t masklen,
						uint16_t cap,
						bool on) {
	uint16_t	index=cap>>3;
	if (index>=(uint16_t)masklen) {
		return;
	}
	byte_t	bit=(byte_t)(1<<(cap&7));
	if (on) {
		mask[masklen-1-index]|=bit;
	} else {
		mask[masklen-1-index]&=(byte_t)~bit;
	}
}

void sqlrprotocol_tds::buildCapabilityMask(byte_t *mask,
						byte_t masklen,
						const uint16_t *caps,
						uint16_t capcount,
						const byte_t *clientmask) {

	bytestring::zero(mask,masklen);

	for (uint16_t i=0; i<capcount; i++) {
		setCapabilityBit(mask,masklen,caps[i],true);
	}

	// A real ase never answers with a bit the client didn't ask for,
	// and neither does this.  The intersection can only clear bits, so
	// nothing this module doesn't support can survive it either.
	for (byte_t i=0; i<masklen; i++) {
		mask[i]&=clientmask[i];
	}
}

bool sqlrprotocol_tds::requestCapabilityGranted(uint16_t cap) {
	return capabilityBitIsSet(grantedrequestmask,grantedrequestmasklen,cap);
}

bool sqlrprotocol_tds::responseCapabilityGranted(uint16_t cap) {
	return capabilityBitIsSet(grantedresponsemask,
					grantedresponsemasklen,cap);
}

bool sqlrprotocol_tds::clientRequestedCapability(uint16_t cap) {
	return capabilityBitIsSet(clientrequestmask,clientrequestmasklen,cap);
}

// Answers the login's capability token.
//
// A real ase answers with what the client asked for and it supports -
// never with a bit the client didn't ask for - so that's what this does:
// the tables below say what this module supports, and each one is
// intersected with the mask the client declared.  Anything else invites
// the client to send something that gets refused, or promises the client
// something that gets sent anyway.
//
// The numbers are the TDS5_CAP_* constants at the top of this file, whose
// note explains the numbering.  A capability this module doesn't support
// is simply absent here, which leaves its bit clear under either
// numbering scheme.
void sqlrprotocol_tds::capability() {

	byte_t	token=TOKEN_CAPABILITY;

	// What this module can be asked to do.
	//
	// Deliberately absent:
	// * bcp (5) and dol bulk (53) - #9480 implements bulk copy
	// * cursors (6) and the scroll bits (33-38) - #9479 implements
	//   them; preTds7Request()'s token dispatch refuses every cursor
	//   token today
	// * msg (8) - a msg token is only read inside the sec-encrypt
	//   login exchange, never accepted as a general request command
	// * sensitivity (45) and boundary (46) - tds5TypeToMsType()
	//   refuses both, since they're labels rather than values
	// * wide tables (59/60) and columnstatus (58/57) - this module
	//   writes the narrow rowfmt and paramfmt, and no columnstatus byte
	// * date/time (71/72), xml (85), sint1 (82) and the unsigned
	//   integer types - tds5TypeToMsType() maps all of these, but no
	//   rowfmt here ever carries one, and leaving out something that
	//   is supported only costs a client the chance to use it
	static const uint16_t	requestcaps[]={
		TDS5_CAP_REQ_LANG,
		TDS5_CAP_REQ_RPC,
		TDS5_CAP_REQ_DYN,
		TDS5_CAP_REQ_PARAM,
		// the datatypes tds5TypeToMsType() accepts on the way in and
		// pretds7typemap[] can send on the way out
		TDS5_CAP_REQ_DATA_INT1,
		TDS5_CAP_REQ_DATA_INT2,
		TDS5_CAP_REQ_DATA_INT4,
		TDS5_CAP_REQ_DATA_BIT,
		TDS5_CAP_REQ_DATA_CHAR,
		TDS5_CAP_REQ_DATA_VCHAR,
		TDS5_CAP_REQ_DATA_BIN,
		TDS5_CAP_REQ_DATA_VBIN,
		TDS5_CAP_REQ_DATA_MNY8,
		TDS5_CAP_REQ_DATA_MNY4,
		TDS5_CAP_REQ_DATA_DATE8,
		TDS5_CAP_REQ_DATA_DATE4,
		TDS5_CAP_REQ_DATA_FLT4,
		TDS5_CAP_REQ_DATA_FLT8,
		TDS5_CAP_REQ_DATA_NUM,
		TDS5_CAP_REQ_DATA_TEXT,
		TDS5_CAP_REQ_DATA_IMAGE,
		TDS5_CAP_REQ_DATA_DEC,
		TDS5_CAP_REQ_DATA_LCHAR,
		TDS5_CAP_REQ_DATA_LBIN,
		TDS5_CAP_REQ_DATA_INTN,
		TDS5_CAP_REQ_DATA_DATETIMEN,
		TDS5_CAP_REQ_DATA_MONEYN,
		TDS5_CAP_REQ_DATA_FLTN,
		// an attention arrives in the packet stream rather than as
		// out-of-band data - see attention()
		TDS5_CAP_REQ_CON_INBAND,
		// preTds7DynamicStatement() strips the "create proc" wrapper
		// this invites, rather than running it - see the note there
		TDS5_CAP_REQ_PROTO_DYNPROC,
		// nTypeSize() sizes a bigint at 8 and preTds7Field() writes
		// it, so this is real
		TDS5_CAP_REQ_DATA_INT8,
		// preTds7Login() ends with negotiatePacketSize() and
		// envChangePacketSize(), so the packet size the login asks
		// for really is honored
		TDS5_CAP_REQ_SRVPKTSIZE
	};

	// What this module promises not to send.  A bit here means "don't
	// send me this", so it's set for what never goes out.
	//
	// Deliberately absent are the NO<datatype> bits: an output
	// parameter is echoed back in whatever type the client declared it
	// as (see rpcparamtds5types), so promising never to send a given
	// type would be a promise this module can't keep.
	//
	// SUPPRESS_FMT is the odd one out - it means "you may leave the
	// format out", not "don't send me one" - and it's offered here
	// rather than hardcoded, so a client that doesn't ask for it
	// doesn't get told it was granted.
	static const uint16_t	responsecaps[]={
		// no debug token is ever written
		TDS5_CAP_RES_NOTDSDEBUG,
		// no columnstatus byte is ever written
		TDS5_CAP_RES_DATA_NOCOLUMNSTATUS,
		// rowfmt2/paramfmt2 are never written
		TDS5_CAP_RES_NO_WIDETABLES,
		TDS5_CAP_RES_SUPPRESS_FMT,
		// no control token is ever written
		TDS5_CAP_RES_NO_TDSCONTROL
	};

	// Answer at the length the client declared.  The intersection is
	// byte-for-byte, and a bit the client's mask is too short to hold
	// is one the client didn't ask for anyway.
	grantedrequestmasklen=clientrequestmasklen;
	grantedresponsemasklen=clientresponsemasklen;

	buildCapabilityMask(grantedrequestmask,grantedrequestmasklen,
				requestcaps,
				(uint16_t)(sizeof(requestcaps)/
						sizeof(requestcaps[0])),
				clientrequestmask);
	buildCapabilityMask(grantedresponsemask,grantedresponsemasklen,
				responsecaps,
				(uint16_t)(sizeof(responsecaps)/
						sizeof(responsecaps[0])),
				clientresponsemask);

	// NOINT8 is cleared whatever the client asked for, rather than left
	// to the intersection.  It isn't a negotiation - pretds7typemap[]
	// sends a bigint as an intn, nTypeSize() sizes that at 8, and
	// preTds7Field() writes 8 bytes for it - so granting "don't send me
	// an 8-byte integer" would contradict what this module does.  The
	// tables above leave it out, and this makes that non-negotiable.
	setCapabilityBit(grantedresponsemask,grantedresponsemasklen,
					TDS5_CAP_RES_DATA_NOINT8,false);

	// each capability is a type byte, a length byte, and that many
	// mask bytes
	uint16_t	tokensize=(uint16_t)
			(sizeof(byte_t)+sizeof(byte_t)+grantedrequestmasklen+
			sizeof(byte_t)+sizeof(byte_t)+grantedresponsemasklen);

	debugStart("capability");
	debugTokenType(token);
	debugWrite("tokensize: 0x%02x (%hd)",tokensize,tokensize);
	debugWrite("request mask: (%d)",(int)grantedrequestmasklen);
	debugHexDump(grantedrequestmask,grantedrequestmasklen);
	debugWrite("response mask: (%d)",(int)grantedresponsemasklen);
	debugHexDump(grantedresponsemask,grantedresponsemasklen);
	debugEnd();

	write(&resppacket,token);
	write(&resppacket,tokensize);
	write(&resppacket,(byte_t)CAPABILITY_REQUEST);
	write(&resppacket,grantedrequestmasklen);
	write(&resppacket,grantedrequestmask,(size_t)grantedrequestmasklen);
	write(&resppacket,(byte_t)CAPABILITY_RESPONSE);
	write(&resppacket,grantedresponsemasklen);
	write(&resppacket,grantedresponsemask,(size_t)grantedresponsemasklen);
}

// Drives the tds 5.0 encrypted-password exchange and hands back the
// cleartext password it recovers.
//
// The server opens it, in a normal buffer rather than a tabular result:
// a login ack carrying NEGOTIATE, a msg token carrying SEC_ENCRYPT, and
// a paramfmt/params pair carrying the 8-byte key the server chose.  The
// client answers with SEC_LOGPWD, and with SEC_REMPWD as well when it
// has a remote password - both msg tokens arrive in one buffer, each
// with a paramfmt/params pair of its own.
//
// This runs inline rather than through the main loop, which rejects a
// non-login packet before login.  Returns false, having sent its own
// error, on anything the client got wrong or when the cipher isn't
// available; the caller then fails the login rather than authenticating
// with an empty password.
bool sqlrprotocol_tds::preTds7SecEncryptLogin(char *password,
						size_t passwordsize) {

	// The key.  Fresh per login: a fixed one would make the blob a
	// constant function of the password, and one captured login
	// replayable forever.  Nul terminated because
	// preTds7ParamValueWrite() renders a value as text as well as as
	// bytes.
	byte_t	key[SEC_ENCRYPT_KEY_SIZE+1];
	csprng	rng;
	if (!rng.generateBytes(key,sizeof(key),SEC_ENCRYPT_KEY_SIZE)) {
		debugStart("pre-tds7 sec encrypt");
		debugWrite("failed to generate a key");
		debugEnd();
		sendSecEncryptUnsupportedError();
		return false;
	}
	key[SEC_ENCRYPT_KEY_SIZE]='\0';

	debugStart("pre-tds7 sec encrypt");
	debugWrite("key:");
	debugHexDump(key,SEC_ENCRYPT_KEY_SIZE);
	debugEnd();

	// the negotiate
	resppacket.clear();
	loginAck(PRE_TDS7_LOGIN_ACK_NEGOTIATE);
	preTds7Msg(TDS5_MSG_HASARGS,TDS5_MSG_SEC_ENCRYPT);

	tds5paramfmt	fmt;
	fmt.name="";
	fmt.namesize=0;
	fmt.status=0;
	fmt.usertype=SEC_ENCRYPT_USERTYPE;
	fmt.tds5type=TDS5_TYPE_VARBINARY;
	fmt.mstype=tds5TypeToMsType(TDS5_TYPE_VARBINARY);
	fmt.varintsize=preTds7VarintSize(TDS5_TYPE_VARBINARY);
	fmt.size=SEC_ENCRYPT_KEY_SIZE;
	fmt.precision=0;
	fmt.scale=0;

	sqlrserverbindvar	bv;
	bv.type=SQLRSERVERBINDVARTYPE_BLOB;
	bv.variable=NULL;
	bv.variablesize=0;
	bv.value.stringval=(char *)key;
	bv.valuesize=SEC_ENCRYPT_KEY_SIZE;
	bv.isnull=cont->getNonNullBindValue();

	if (!preTds7ParamFmtWrite(&fmt,1) ||
			!preTds7ParamsWrite(&fmt,&bv,1)) {
		debugStart("pre-tds7 sec encrypt");
		debugWrite("failed to write the key parameter");
		debugEnd();
		// sendSecEncryptUnsupportedError() clears the half-built
		// negotiate before it writes the error
		sendSecEncryptUnsupportedError();
		return false;
	}

	done();

	if (!sendPacket(PRE_TDS7_NORMAL)) {
		debugStart("pre-tds7 sec encrypt");
		debugWrite("failed to send the negotiate");
		debugEnd();
		return false;
	}

	// the answer
	byte_t	packettype=0;
	secencryptreply=true;
	bool	received=recvPacket(&packettype);
	secencryptreply=false;
	if (!received) {
		debugStart("pre-tds7 sec encrypt");
		debugWrite("failed to read the answer");
		debugEnd();
		return false;
	}
	if (packettype!=PRE_TDS7_NORMAL) {
		debugStart("pre-tds7 sec encrypt");
		debugPacketType("unexpected answer packet type",packettype);
		debugEnd();
		sendTdsProtocolError();
		return false;
	}

	const byte_t	*rp=reqpacket.getBuffer();
	size_t		rpsize=reqpacket.getSize();

	byte_t	logpwd[SEC_ENCRYPT_BLOB_SIZE];
	byte_t	rempwd[SEC_ENCRYPT_BLOB_SIZE];
	bool	haslogpwd=false;

	// Every token in the buffer, not just the first - sec_logpwd and
	// sec_rempwd arrive together.
	while (rpsize) {

		debugStart("pre-tds7 sec encrypt");

		byte_t	token=0;
		read(rp,&token,&rp);
		rpsize--;
		debugPreTds7TokenType(token);

		if (token!=TDS5_TOKEN_MSG) {
			debugWrite("expected a msg token");
			debugEnd();
			sendTdsProtocolError();
			return false;
		}

		uint16_t	msgid=0;
		if (!preTds7MsgRead(&rp,&rpsize,&msgid)) {
			debugWrite("malformed msg token");
			debugEnd();
			sendTdsProtocolError();
			return false;
		}

		// sec_logpwd carries the blob alone; sec_rempwd carries a
		// remote server name in front of it, empty unless the client
		// set a remote password.  Sec_logpwd comes first and comes
		// once; a second one is malformed.
		//
		// A sec_rempwd is parsed for its framing and its blob then
		// discarded.  A remote password is whatever the client's
		// ct_remote_pwd() said, independent of the login password,
		// and the module ignores remote passwords everywhere else.
		// A client can have set several, so any number of sec_rempwd
		// tokens is accepted.
		uint16_t	param=0;
		byte_t		*blob=NULL;
		if (msgid==TDS5_MSG_SEC_LOGPWD && !haslogpwd) {
			param=0;
			blob=logpwd;
			haslogpwd=true;
		} else if (msgid==TDS5_MSG_SEC_REMPWD && haslogpwd) {
			param=1;
			blob=rempwd;
		} else {
			debugWrite("unexpected msgid: %d",(int)msgid);
			debugEnd();
			sendTdsProtocolError();
			return false;
		}

		if (!preTds7SecEncryptBlob(&rp,&rpsize,param,blob)) {
			debugEnd();
			sendTdsProtocolError();
			return false;
		}

		debugEnd();
	}

	debugStart("pre-tds7 sec encrypt");

	if (!haslogpwd) {
		debugWrite("no sec_logpwd in the answer");
		debugEnd();
		sendTdsProtocolError();
		return false;
	}

	bool	decrypted=secEncryptDecryptPassword(key,logpwd,sizeof(logpwd),
							password,passwordsize);

	// the blobs are password-equivalent, so don't leave them on the stack
	bytestring::zero(logpwd,sizeof(logpwd));
	bytestring::zero(rempwd,sizeof(rempwd));

	if (!decrypted) {
		debugWrite("failed to decrypt the password");
		debugEnd();
		// The cipher may simply not be there, and that's the one
		// failure the client can do something about, so it gets the
		// message that says so.
		sendSecEncryptUnsupportedError();
		return false;
	}

	debugWrite("password: (hidden)");
	debugEnd();

	return true;
}

// Writes a tds 5.0 msg token.  All three of the encrypted-password
// exchange's messages carry nothing but a status and a msgid, so the
// token length is fixed.
void sqlrprotocol_tds::preTds7Msg(byte_t status, uint16_t msgid) {

	byte_t	token=TDS5_TOKEN_MSG;

	debugStart("pre-tds7 msg write");
	debugPreTds7TokenType(token);
	debugWrite("token length: %d",TDS5_MSG_SIZE);
	debugWrite("status: 0x%02x",(int)status);
	debugWrite("msgid: %d",(int)msgid);
	debugEnd();

	write(&resppacket,token);
	write(&resppacket,(byte_t)TDS5_MSG_SIZE);
	write(&resppacket,status);
	write(&resppacket,msgid);
}

// Reads a tds 5.0 msg token.  The token byte has already been read.
// Whatever the token declares past the msgid is stepped over rather than
// refused, so a message carrying more than the three above still parses.
bool sqlrprotocol_tds::preTds7MsgRead(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t *msgid) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	*msgid=0;

	// the token length
	if (!rpsize) {
		return false;
	}
	byte_t	tokenlength=0;
	read(rp,&tokenlength,&rp);
	rpsize--;

	// Everything below is bounded by the token's own length rather than
	// by what's left in the buffer, so a msg that lies about its size
	// can't read into the token behind it.
	if (tokenlength<TDS5_MSG_SIZE || (size_t)tokenlength>rpsize) {
		return false;
	}

	byte_t	status=0;
	read(rp,&status,&rp);
	read(rp,msgid,&rp);
	rp+=(size_t)tokenlength-TDS5_MSG_SIZE;
	rpsize-=(size_t)tokenlength;

	debugWrite("token length: %d",(int)tokenlength);
	debugWrite("status: 0x%02x",(int)status);
	debugWrite("msgid: %d",(int)(*msgid));

	return true;
}

bool sqlrprotocol_tds::preTds7SecEncryptBlob(const byte_t **rpinout,
						size_t *rpsizeinout,
						uint16_t param,
						byte_t *blob) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	// the paramfmt
	if (!rpsize) {
		debugWrite("missing paramfmt token");
		return false;
	}
	byte_t	token=0;
	read(rp,&token,&rp);
	rpsize--;
	if (token!=TDS5_TOKEN_PARAMFMT && token!=TDS5_TOKEN_PARAMFMT2) {
		debugWrite("expected a paramfmt token");
		return false;
	}
	const char	*err=NULL;
	if (!preTds7ParamFmtRead(&rp,&rpsize,
				(token==TDS5_TOKEN_PARAMFMT2),&err)) {
		debugWrite("%s",err);
		return false;
	}

	// The declared type, checked before the value is read rather than
	// after.  preTds7ParamsRead() routes a value by the type its
	// paramfmt declared, and the string types print the value into the
	// debug output - which for a blob declared as varchar would be the
	// ciphertext.  A real client declares varbinary here.
	if (pretds7paramfmtcount<=param ||
			(pretds7paramfmts[param].tds5type!=
					TDS5_TYPE_VARBINARY &&
			pretds7paramfmts[param].tds5type!=
					TDS5_TYPE_BINARY)) {
		debugWrite("parameter %d is not declared binary",(int)param);
		return false;
	}

	// the params
	if (!rpsize) {
		debugWrite("missing params token");
		return false;
	}
	read(rp,&token,&rp);
	rpsize--;
	if (token!=TDS5_TOKEN_PARAMS) {
		debugWrite("expected a params token");
		return false;
	}
	if (!preTds7ParamsRead(&rp,&rpsize)) {
		debugWrite("malformed params token");
		return false;
	}

	// the blob itself.  A paramfmt that declared a different type or a
	// different width than the exchange uses lands here as a parameter
	// that isn't a 33-byte blob, whatever it claimed to be.
	if (rpcparamcount<=param) {
		debugWrite("too few parameters: %d",rpcparamcount);
		return false;
	}
	sqlrserverbindvar	*bv=&(rpcparams[param]);
	if (bv->type!=SQLRSERVERBINDVARTYPE_BLOB ||
			bv->valuesize!=SEC_ENCRYPT_BLOB_SIZE) {
		debugWrite("parameter %d is not a %d byte blob",
				(int)param,SEC_ENCRYPT_BLOB_SIZE);
		return false;
	}
	bytestring::copy(blob,bv->value.stringval,SEC_ENCRYPT_BLOB_SIZE);

	// The blob's last byte says how long the password inside it is, and
	// the cipher clamps that to SEC_ENCRYPT_MAX_PASSWORD.  Checked here
	// as well as in secEncryptDecryptPassword() so that a malformed blob
	// gets the protocol error it deserves rather than the "encrypted
	// logins aren't supported" one.
	if (blob[SEC_ENCRYPT_BLOB_SIZE-1]>SEC_ENCRYPT_MAX_PASSWORD) {
		debugWrite("declared password length is too long: %d",
				(int)blob[SEC_ENCRYPT_BLOB_SIZE-1]);
		return false;
	}

	return true;
}


// ---- the cipher ---------------------------------------------------
//
// Everything that knows how the blob is enciphered lives between here
// and the end of secEncryptDecryptPassword(), so it can be replaced
// without touching the exchange around it.
//
// The cipher is sap's own, and is reached by calling sap's own library:
// com__string_uninitialize() is an exported symbol in open client's
// libsybcomn64.so.  The alternative is to ship the cipher's 256 x 33
// byte key table, which is a verbatim copy of an internal sap document,
// so calling the library is what interoperates without copying sap's
// data.  The cost is that an encrypted tds 5.0 login works where open
// client is installed and is refused cleanly where it isn't.
//
// The soname is what usually resolves it: a sap-backed instance already
// has open client's libraries in the process, so the loader matches the
// name against what's loaded.  The paths after it only find the library
// when open client's other libraries are on the runtime search path
// too, since libsybcomn64 needs them.
typedef int (*secencryptdecryptfunction)(const byte_t *key,
						int keylen,
						const byte_t *in,
						int inlen,
						byte_t *out,
						int *outlen);

static bool				secencrypttried=false;
static dynamiclib			secencryptlib;
static secencryptdecryptfunction	secencryptdecrypt=NULL;

static bool secEncryptLoadCipher() {

	// One attempt per process, however many logins ask for it.  The
	// check-then-set needs no lock: sqlr-connection is single threaded
	// and is exec'd per process, so the statics start out fresh.
	if (secencrypttried) {
		return (secencryptdecrypt!=NULL);
	}
	secencrypttried=true;

	stringbuffer	sybasepath;
	const char	*sybase=environment::getValue("SYBASE");
	if (!charstring::isNullOrEmpty(sybase)) {
		sybasepath.append(sybase);
		sybasepath.append(SEC_ENCRYPT_LIB_DIR);
		sybasepath.append(SEC_ENCRYPT_LIB);
	}

	const char	*libnames[3];
	libnames[0]=SEC_ENCRYPT_LIB;
	libnames[1]=sybasepath.getString();
	libnames[2]="/opt/sap" SEC_ENCRYPT_LIB_DIR SEC_ENCRYPT_LIB;

	for (uint8_t i=0; i<3; i++) {

		if (charstring::isNullOrEmpty(libnames[i])) {
			continue;
		}
		if (!secencryptlib.open(libnames[i],true,false)) {
			continue;
		}

		secencryptdecrypt=(secencryptdecryptfunction)
				secencryptlib.getSymbol(SEC_ENCRYPT_SYMBOL);
		if (secencryptdecrypt) {
			return true;
		}
		secencryptlib.close();
	}

	return false;
}

bool sqlrprotocol_tds::secEncryptDecryptPassword(const byte_t *key,
						const byte_t *blob,
						size_t bloblen,
						char *password,
						size_t passwordsize) {

	// The blob is 32 bytes of ciphertext and a trailing byte giving how
	// long the password inside them is.  Nothing about it is logged -
	// it's password-equivalent, and so is what comes out.
	if (bloblen!=SEC_ENCRYPT_BLOB_SIZE) {
		return false;
	}
	size_t	length=blob[SEC_ENCRYPT_BLOB_SIZE-1];
	if (length>SEC_ENCRYPT_MAX_PASSWORD || length>=passwordsize) {
		return false;
	}

	// an empty password is not an acceptable decrypt result
	if (!length) {
		return false;
	}

	if (!secEncryptLoadCipher()) {
		return false;
	}

	// the library writes the cleartext and its length; the scratch
	// buffer is well clear of the 30 bytes the cipher can produce
	byte_t	out[64];
	int	outlen=0;
	bytestring::zero(out,sizeof(out));

	// The cipher carries no integrity check, so this is not one.  The
	// library takes the length from the blob's own trailing byte and
	// returns 1 either way, so a blob that isn't what the client's
	// library produced decrypts to a wrong password rather than to an
	// error, and it's the authentication below that rejects it.  The
	// checks are here against a library build that behaves differently.
	bool	ok=((*secencryptdecrypt)(key,(int)SEC_ENCRYPT_KEY_SIZE,
						blob,(int)bloblen,
						out,&outlen)==1 &&
			outlen>=0 && (size_t)outlen==length);
	if (ok) {
		bytestring::copy(password,out,length);
		password[length]='\0';
	}

	bytestring::zero(out,sizeof(out));

	return ok;
}

// ---- end of the cipher --------------------------------------------


bool sqlrprotocol_tds::sendSecEncryptUnsupportedError() {
	// FIXME: is there a real error number/state for this?
	return sendError(0,1,14,
			"Encrypted logins are not supported.  "
			"Disable password encryption in the client "
			"and log in again.",1);
}

bool sqlrprotocol_tds::sendPreTds7VersionUnsupportedError() {
	// FIXME: is there a real error number/state for this?
	return sendError(0,1,14,
			"TDS 5.0 is the only supported pre-TDS-7 protocol "
			"version.  Configure the client to use TDS 5.0 "
			"and log in again.",1);
}

bool sqlrprotocol_tds::sendPreTds7ByteOrderUnsupportedError() {
	// FIXME: is there a real error number/state for this?
	return sendError(0,1,14,
			"Mixed byte orders are not supported.  Configure the "
			"client to use one byte order for integers, floats "
			"and datetimes, and log in again.",1);
}

byte_t sqlrprotocol_tds::preTds7FieldOrder(byte_t value,
					byte_t leval, byte_t beval) {
	if (value==leval) {
		return PRE_TDS7_ORDER_LE;
	}
	if (value==beval) {
		return PRE_TDS7_ORDER_BE;
	}
	return PRE_TDS7_ORDER_UNKNOWN;
}

// Works the login record's typeflags block out into the single byte order
// the rest of the session runs in, and returns false if the record didn't
// declare one.
//
// Only the exact values the spec defines are recognized, one field at a
// time.  Anything else - including a byte of 0 in a position where 0
// isn't a defined value - falls back to little-endian rather than being
// read as anything in particular.  That matters most for a block of all
// zeros, which isn't a valid declaration at all but would otherwise
// decode as big-endian, because 0 is what TDS_INT4_LSB_HI happens to be.
// Every client that reaches this in practice runs on little-endian
// hardware, so little-endian is the safe fallback, and an unrecognized
// value gets a debug line rather than a refused login.
//
// A genuine disagreement between fields is a different matter.  The
// record declares integers, floats and datetimes separately, so a client
// could in principle ask for little-endian integers and big-endian
// floats.  Nothing downstream can serve that: the byte order is one
// per-session flag, so picking either order would get the other one
// wrong in every row.  Refuse the login instead - real clients declare a
// consistent order, so this costs nothing that works today.
bool sqlrprotocol_tds::preTds7ByteOrder(const byte_t *typeflags,
					bool *bigendian) {

	// A block of all zeros declares nothing - it's what a client that
	// never filled the field in sends.  It has to be caught up front
	// rather than decoded field by field, because 0 is a valid value in
	// one of the positions: it's what TDS_INT4_LSB_HI happens to be.
	// Decoded a field at a time, an unfilled block would come out as
	// big-endian 4-byte ints and little-endian everything else, and get
	// the login refused for declaring two orders at once.
	bool	allzero=true;
	for (byte_t i=0; i<PRE_TDS7_TYPE_FLAGS_SIZE; i++) {
		if (typeflags[i]) {
			allzero=false;
			break;
		}
	}
	if (allzero) {
		debugStart("pre-tds7 byte order");
		debugWrite("typeflags not filled in - "
				"assuming little-endian");
		debugEnd();
		*bigendian=false;
		return true;
	}

	byte_t	int2=typeflags[PRE_TDS7_TYPE_FLAGS_INT2];
	byte_t	int4=typeflags[PRE_TDS7_TYPE_FLAGS_INT4];
	byte_t	chr=typeflags[PRE_TDS7_TYPE_FLAGS_CHAR];
	byte_t	flt=typeflags[PRE_TDS7_TYPE_FLAGS_FLT];
	byte_t	date=typeflags[PRE_TDS7_TYPE_FLAGS_DATE];

	// the four fields that declare a byte order
	byte_t	order[4];
	order[0]=preTds7FieldOrder(int2,TDS_INT2_LSB_LO,TDS_INT2_LSB_HI);
	order[1]=preTds7FieldOrder(int4,TDS_INT4_LSB_LO,TDS_INT4_LSB_HI);
	order[2]=preTds7FieldOrder(flt,TDS_FLT_IEEE_LO,TDS_FLT_IEEE_HI);
	order[3]=preTds7FieldOrder(date,TDS_TWO_I4_LSB_LO,TDS_TWO_I4_LSB_HI);

	const char	*name[4];
	name[0]="int2";
	name[1]="int4";
	name[2]="flt";
	name[3]="date";

	byte_t	value[4];
	value[0]=int2;
	value[1]=int4;
	value[2]=flt;
	value[3]=date;

	debugStart("pre-tds7 byte order");

	// The char field isn't a byte order, but decode it here anyway so
	// the debug output says what the whole block declared.  Nothing
	// acts on it - an ebcdic client would need a translation this
	// module doesn't do, and none has ever turned up.
	debugWrite("char: %d (%s)",(int)chr,
			(chr==TDS_CHAR_ASCII)?"ascii":
			((chr==TDS_CHAR_EBCDIC)?"ebcdic":"unrecognized"));

	// fall unrecognized fields back to little-endian, noting each one
	bool	agree=true;
	byte_t	verdict=PRE_TDS7_ORDER_LE;
	for (byte_t i=0; i<4; i++) {
		if (order[i]==PRE_TDS7_ORDER_UNKNOWN) {
			debugWrite("%s: %d (unrecognized - "
					"assuming little-endian)",
					name[i],(int)value[i]);
			order[i]=PRE_TDS7_ORDER_LE;
		} else {
			debugWrite("%s: %d (%s)",name[i],(int)value[i],
					(order[i]==PRE_TDS7_ORDER_BE)?
						"big-endian":"little-endian");
		}
		if (i) {
			if (order[i]!=verdict) {
				agree=false;
			}
		} else {
			verdict=order[i];
		}
	}

	if (!agree) {
		debugWrite("fields disagree - refusing the login");
		debugEnd();
		return false;
	}

	*bigendian=(verdict==PRE_TDS7_ORDER_BE);

	debugWrite("byte order: %s",(*bigendian)?"BE":"LE");
	debugEnd();

	return true;
}

// "value" must point at a buffer of at least "size"+1 bytes: "size" for the
// field itself, and one more for the nul written after the last real
// character.  Nothing checks that, so every call site pairs a
// char[SOMETHING+1] with a matching SOMETHING here.
void sqlrprotocol_tds::readPreTds7Field(const byte_t *rp,
					char *value,
					size_t size,
					byte_t *length,
					const byte_t **rpout) {

	// the field is a fixed run of "size" nul-padded bytes, followed by a
	// trailing byte giving how many of them are real characters
	read(rp,value,size,&rp);
	read(rp,length,&rp);

	// a bogus length would run off the end of the field
	if ((size_t)(*length)>size) {
		*length=(byte_t)size;
	}
	value[*length]='\0';

	*rpout=rp;
}

// The tds 7.x login record doesn't send its passwords in the clear, but
// what it does send is only a fixed xor-and-nibble-swap away from them -
// see readPassword() - so a raw dump of one gives them up just as surely.
// Unlike the pre-tds7 record, login7 declares where its fields are rather
// than laying them out at fixed offsets, so read the offsets back out of
// the header that's arrived so far.
void sqlrprotocol_tds::maskTds7Passwords(byte_t *packet,
					uint32_t packetsize,
					uint64_t packetoffset) {

	const byte_t	*rq=reqpacket.getBuffer();
	uint64_t	rqsize=reqpacket.getSize();

	// nothing can be located until the header declaring it has arrived
	if (rqsize<LOGIN7_HEADER_SIZE) {
		return;
	}

	// each cch counts ucs-2 characters rather than bytes
	const byte_t	*rp=rq+LOGIN7_IBPASSWORD_OFFSET;
	uint16_t	ibpassword=0;
	uint16_t	cchpassword=0;
	readLE(rp,&ibpassword,&rp);
	readLE(rp,&cchpassword,&rp);
	maskRange(packet,packetsize,packetoffset,
			ibpassword,(uint64_t)cchpassword*sizeof(ucs2_t));

	// the change-password field only exists from tds 7.2 on
	const byte_t	*vp=rq+LOGIN7_TDSVERSION_OFFSET;
	uint32_t	tdsversion=0;
	readBE(vp,&tdsversion,&vp);
	if (tdsVersionHexToDec(tdsversion)<720 ||
				rqsize<LOGIN7_HEADER_SIZE_72) {
		return;
	}

	const byte_t	*cp=rq+LOGIN7_IBCHANGEPASSWORD_OFFSET;
	uint16_t	ibchangepassword=0;
	uint16_t	cchchangepassword=0;
	readLE(cp,&ibchangepassword,&cp);
	readLE(cp,&cchchangepassword,&cp);
	maskRange(packet,packetsize,packetoffset,
			ibchangepassword,
			(uint64_t)cchchangepassword*sizeof(ucs2_t));
}

void sqlrprotocol_tds::maskPreTds7Passwords(byte_t *packet,
					uint32_t packetsize,
					uint64_t packetoffset) {
	maskRange(packet,packetsize,packetoffset,
				PRE_TDS7_PASSWORD_OFFSET,
				PRE_TDS7_NAME_SIZE);
	maskRange(packet,packetsize,packetoffset,
				PRE_TDS7_REMOTE_PASSWORD_OFFSET,
				PRE_TDS7_REMOTE_PASSWORD_SIZE);
}

void sqlrprotocol_tds::maskRange(byte_t *packet,
					uint32_t packetsize,
					uint64_t packetoffset,
					uint64_t start,
					uint64_t size) {

	// the range is given relative to the reassembled request, but only
	// the part of it that landed in this packet can be masked here
	uint64_t	end=start+size;
	uint64_t	packetend=packetoffset+packetsize;
	if (end<=packetoffset || start>=packetend) {
		return;
	}
	if (start<packetoffset) {
		start=packetoffset;
	}
	if (end>packetend) {
		end=packetend;
	}

	bytestring::set(packet+(start-packetoffset),'x',
					(size_t)(end-start));
}

bool sqlrprotocol_tds::tds7Login() {

	// nothing gets authenticated in the clear when tls is required
	if (useTls() && !tlsactive) {
		debugStart("tds7 login");
		debugWrite((tlsrefused)?
				"tls required but the client "
				"doesn't support it":
				"tls required but not negotiated");
		debugEnd();
		sendTlsRequiredError();
		return false;
	}

	// the login packet has been read by now, and in login-only mode
	// it's the only thing that was meant to be encrypted
	if (tlsmode==TLS_MODE_LOGIN) {
		stopTls();
	}

	const byte_t	*rp=reqpacket.getBuffer();
	const byte_t	*startrp=rp;

	// the whole request, in bytes - not decremented as the header is
	// read, since the fields after it are indexed off startrp
	size_t		rpsize=reqpacket.getSize();

	// the fixed header is read straight through below, so it has to be
	// there
	if (rpsize<LOGIN7_HEADER_SIZE) {
		debugStart("tds7 login");
		debugWrite("truncated login7: %lld<%d",
					(long long)rpsize,LOGIN7_HEADER_SIZE);
		debugEnd();
		return false;
	}

	// initialize values
	uint32_t	size=0;
	uint32_t	tdsversion=0;
	clienttdsversion=700;
	uint32_t	packetsize=0;
	uint32_t	clientprogver=0;
	uint32_t	clientpid=0;
	uint32_t	connectionid=0;
	byte_t		optionflags1=0;
	byte_t		optionflags2=0;
	byte_t		typeflags=0;
	byte_t		optionflags3=0;
	uint32_t	clienttimzone=0;
	uint32_t	clientlcid=0;

	uint16_t	ibhostname=0;
	uint16_t	cchhostname=0;
	uint16_t	ibusername=0;
	uint16_t	cchusername=0;
	uint16_t	ibpassword=0;
	uint16_t	cchpassword=0;
	uint16_t	ibappname=0;
	uint16_t	cchappname=0;
	uint16_t	ibservername=0;
	uint16_t	cchservername=0;
	uint16_t	ibextension=0;
	uint16_t	cbextension=0;
	uint16_t	ibcltintname=0;
	uint16_t	cchcltintname=0;
	uint16_t	iblanguage=0;
	uint16_t	cchlanguage=0;
	uint16_t	ibdatabase=0;
	uint16_t	cchdatabase=0;
	char		clientid[6];
	uint16_t	ibsspi=0;
	uint16_t	cbsspi=0;
	uint16_t	ibatchdbfile=0;
	uint16_t	cchatchdbfile=0;
	uint16_t	ibchangepassword=0;
	uint16_t	cchchangepassword=0;
	uint32_t	cbsspilong=0;

	// only the flags this module acts on are pulled out here -
	// debugLogin7OptionFlags() decodes the rest of the bitmaps
	char		fusedbwarn=0;
	char		fusedbfatal=0;
	char		fsetlangwarn=0;

	char		fsetlangfatal=0;
	char		fodbc=0;

	char		foledb=0;

	bool		fchangepassword=false;
	bool		fextension=false;

	wchar_t		*hostname=NULL;
	wchar_t		*username=NULL;
	wchar_t		*password=NULL;
	wchar_t		*appname=NULL;
	wchar_t		*servername=NULL;
	byte_t		*extension=NULL;
	wchar_t		*cltintname=NULL;
	wchar_t		*language=NULL;
	wchar_t		*database=NULL;
	wchar_t		*atchdbfile=NULL;
	wchar_t		*changepassword=NULL;
	byte_t		*sspi=NULL;

	// copy values out of the recv packet
	readLE(rp,&size,&rp);
	readBE(rp,&tdsversion,&rp);
	clienttdsversion=tdsVersionHexToDec(tdsversion);
	readLE(rp,&packetsize,&rp);
	readBE(rp,&clientprogver,&rp);
	readLE(rp,&clientpid,&rp);
	readLE(rp,&connectionid,&rp);
	read(rp,&optionflags1,&rp);
	read(rp,&optionflags2,&rp);
	read(rp,&typeflags,&rp);
	read(rp,&optionflags3,&rp);

	// set option/type flags
	fusedbwarn=(optionflags1&(0x01<<5))>>5;
	fusedbfatal=(optionflags1&(0x01<<6))>>6;
	fsetlangwarn=(optionflags1&(0x01<<7))>>7;

	fsetlangfatal=(optionflags2&(0x01));
	fodbc=(optionflags2&(0x01<<1))>>1;

	foledb=(typeflags&(0x01<<4))>>4;

	fchangepassword=(optionflags3&(0x01));
	fextension=(optionflags3&(0x01<<4))>>4;

	readLE(rp,&clienttimzone,&rp);
	readLE(rp,&clientlcid,&rp);
	readLE(rp,&ibhostname,&rp);
	readLE(rp,&cchhostname,&rp);
	readLE(rp,&ibusername,&rp);
	readLE(rp,&cchusername,&rp);
	readLE(rp,&ibpassword,&rp);
	readLE(rp,&cchpassword,&rp);
	readLE(rp,&ibappname,&rp);
	readLE(rp,&cchappname,&rp);
	readLE(rp,&ibservername,&rp);
	readLE(rp,&cchservername,&rp);
	if (clienttdsversion>=740) {
		readLE(rp,&ibextension,&rp);
		readLE(rp,&cbextension,&rp);
		if (!fextension) {
			ibextension=0;
			cbextension=0;
		}
	} else {
		rp=rp+sizeof(ibextension);
		rp=rp+sizeof(cbextension);
	}
	readLE(rp,&ibcltintname,&rp);
	readLE(rp,&cchcltintname,&rp);
	readLE(rp,&iblanguage,&rp);
	readLE(rp,&cchlanguage,&rp);
	readLE(rp,&ibdatabase,&rp);
	readLE(rp,&cchdatabase,&rp);
	read(rp,clientid,sizeof(clientid),&rp);
	readLE(rp,&ibsspi,&rp);
	readLE(rp,&cbsspi,&rp);
	readLE(rp,&ibatchdbfile,&rp);
	readLE(rp,&cchatchdbfile,&rp);
	if (clienttdsversion>=720) {

		// the version came off the wire, so this part of the header
		// couldn't be checked with the rest of it
		if (rpsize<LOGIN7_HEADER_SIZE_72) {
			debugStart("tds7 login");
			debugWrite("truncated login7: %lld<%d",
					(long long)rpsize,LOGIN7_HEADER_SIZE_72);
			debugEnd();
			return false;
		}

		readLE(rp,&ibchangepassword,&rp);
		readLE(rp,&cchchangepassword,&rp);
		if (!fchangepassword) {
			ibchangepassword=0;
			cchchangepassword=0;
		}
		readLE(rp,&cbsspilong,&rp);
	}
	if (loginFieldFits("hostname",ibhostname,&cchhostname,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		hostname=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibhostname),
					(size_t)cchhostname);
	}
	if (loginFieldFits("username",ibusername,&cchusername,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		username=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibusername),
					(size_t)cchusername);
	}
	if (loginFieldFits("password",ibpassword,&cchpassword,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		password=readPassword(startrp+ibpassword,cchpassword);
	}
	if (loginFieldFits("appname",ibappname,&cchappname,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		appname=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibappname),
					(size_t)cchappname);
	}
	if (loginFieldFits("servername",ibservername,&cchservername,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		servername=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibservername),
					(size_t)cchservername);
	}
	if (clienttdsversion>=740 &&
		loginFieldFits("extension",ibextension,&cbextension,
					MAX_LOGIN_EXTENSION_BYTES,
					sizeof(byte_t),rpsize)) {
		extension=(byte_t *)bytestring::duplicate(
						startrp+ibextension,
						cbextension);
		// FIXME: decode this...
	}
	if (loginFieldFits("cltintname",ibcltintname,&cchcltintname,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		cltintname=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibcltintname),
					(size_t)cchcltintname);
	}
	if (loginFieldFits("language",iblanguage,&cchlanguage,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		language=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+iblanguage),
					(size_t)cchlanguage);
	}
	if (loginFieldFits("database",ibdatabase,&cchdatabase,
				MAX_LOGIN_CHARS,sizeof(ucs2_t),rpsize)) {
		database=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibdatabase),
					(size_t)cchdatabase);
	}
	if (loginFieldFits("atchdbfile",ibatchdbfile,&cchatchdbfile,
				MAX_LOGIN_ATCHDBFILE_CHARS,
				sizeof(ucs2_t),rpsize)) {
		atchdbfile=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibatchdbfile),
					(size_t)cchatchdbfile);
	}
	if (clienttdsversion>=720 &&
		loginFieldFits("changepassword",ibchangepassword,
					&cchchangepassword,MAX_LOGIN_CHARS,
					sizeof(ucs2_t),rpsize)) {
		changepassword=readPassword(startrp+ibchangepassword,
							cchchangepassword);
	}
	// cbsspilong is only consulted when cbsspi is saturated, and it only
	// exists from tds 7.2 up
	// (the version test is documentation rather than a behaviour change -
	// below 7.2 cbsspilong is still 0 from its initializer)
	uint32_t	sspisize=cbsspi;
	if (clienttdsversion>=720 && cbsspi==65535 && cbsspilong) {
		sspisize=cbsspilong;
	}

	// not bounded by fitsInPacket alone - recvPacket() only caps the
	// whole request at maxrequestsize, far looser than MAX_LOGIN_SSPI_BYTES,
	// so this check has to run first.  the size is dropped along with the
	// pointer since the hex dump below walks the length either way
	if (sspisize>MAX_LOGIN_SSPI_BYTES) {
		debugStart("tds7 login");
		debugWrite("sspi: %u exceeds the %d maximum, "
					"dropping the field",
					sspisize,MAX_LOGIN_SSPI_BYTES);
		debugEnd();
		sspisize=0;
	} else if (sspisize && !fitsInPacket(ibsspi,sspisize,rpsize)) {
		debugStart("tds7 login");
		debugWrite("sspi: (%hd,%d) lies outside of the packet, "
					"dropping the field",ibsspi,sspisize);
		debugEnd();
		sspisize=0;
	}

	sspi=(byte_t *)bytestring::duplicate(startrp+ibsspi,sspisize);

	if (fodbc==ODBC_ON || foledb==OLEDB_ON) {
		// FIXME: set:
		// * ANSI_DEFAULTS ON
		// * CURSOR_CLOSE_ON_COMMIT OFF
		// * IMPLICIT_TRANSACTIONS OFF
		// * TEXTSIZE 0x7FFFFFFF for tds <= 7.2
		// * TEXTSIZE infinite for tds >= 7.3
		// * ROWCOUNT infinite
	}

	if (getDebug()) {
		debugStart("tds7 login");
		debugWrite("size: %d",size);
		debugWrite("tdsversion: 0x%08x (%d)",
					tdsversion,clienttdsversion);
		debugWrite("packetsize: %d",packetsize);
		debugWrite("clientprogver: 0x%08x (%d)",
					clientprogver,clientprogver);
		debugWrite("clientpid: %d",clientpid);
		debugWrite("connectionid: %d",connectionid);
		debugLogin7OptionFlags(optionflags1,optionflags2,
						typeflags,optionflags3);
		debugWrite("clienttimzone: %d",clienttimzone);
		stringbuffer	b;
		b.printBits(clientlcid);
		debugWrite("clientlcid: %s",b.getString());
		debugWrite("hostname: (%hd,%hd) %S",
					ibhostname,cchhostname,hostname);
		debugWrite("username: (%hd,%hd) %S",
					ibusername,cchusername,username);
		debugWrite("password: (%hd,%hd) (hidden)",
					ibpassword,cchpassword);
		debugWrite("appname: (%hd,%hd) %S",
					ibappname,cchappname,appname);
		debugWrite("servername: (%hd,%hd) %S",
					ibservername,cchservername,servername);
		b.clear();
		b.safePrint(extension,cbextension);
		debugWrite("extension: (%hd,%hd) %s",
					ibextension,cbextension,b.getString());
		debugWrite("cltintname: (%hd,%hd) %S",
					ibcltintname,cchcltintname,cltintname);
		debugWrite("language: (%hd,%hd) %S",
					iblanguage,cchlanguage,language);
		debugWrite("database: (%hd,%hd) %S",
					ibdatabase,cchdatabase,database);
		debugWrite("clientid: %02x:%02x:%02x:%02x:%02x:%02x",
					clientid[0],clientid[1],clientid[2],
					clientid[3],clientid[4],clientid[5]);
		debugWrite("atchdbfile: (%hd,%hd) %S",
					ibatchdbfile,cchatchdbfile,atchdbfile);
		debugWrite("changepassword: (%hd,%hd) (hidden)",
					ibchangepassword,cchchangepassword);
		debugWrite("sspi: (%hd,%hd,%d)",ibsspi,cbsspi,cbsspilong);
		debugHexDump((byte_t *)sspi,sspisize);
		debugEnd();
	}

	// FIXME: validate some of these values

	// negotiate tds version
	negotiateTdsVersion();


	// begin building the response packet
	resppacket.clear();

	bool	retval=true;

	// auth the user
	if (retval) {
		if (auth(username,cchusername,password,cchpassword)) {
			loggedin=true;
			// run session-start queries
			cont->beginSession();
			// loginAck() ignores the status on the tds 7.x
			// path - only a pre-tds7 login ack carries one
			loginAck(PRE_TDS7_LOGIN_ACK_SUCCEED);
		} else {
			authError(username,cchusername);
			retval=false;
		}
	}

	// change database
	if (retval && cchdatabase) {

		char		*olddatabase=cont->getCurrentDatabase();
		uint32_t	olddatabaselen=
					charstring::getLength(olddatabase);
		wchar_t		*olddatabase32=wcharstring::duplicate(
						olddatabase,olddatabaselen);

		if (changeDatabase(database,cchdatabase)) {

			envChange(ENV_CHANGE_DATABASE,
					database,
					cchdatabase,
					olddatabase32,
					olddatabaselen);

			changeDatabaseInfo(database,cchdatabase);

		} else {
			if (fusedbfatal==USE_DB_FATAL) {
				changeDatabaseError(database,cchdatabase,false);
				retval=false;
			} else if (fusedbwarn==USE_DB_WARN_ON) {
				changeDatabaseError(database,cchdatabase,true);
			}
		}

		delete[] olddatabase32;
		delete[] olddatabase;
	}

	// change collation
	// (a real server answers with the collation it actually uses rather
	// than echoing the client's lcid back)
	if (retval) {
		if (changeCollation(clientlcid)) {
			envChangeSqlCollation(TDS_COLLATION_LCID,
						TDS_COLLATION_SORTID);
		}
	}

	// change language
	if (retval && cchlanguage) {
		if (changeLanguage(language,cchlanguage)) {

			envChange(ENV_CHANGE_LANGUAGE,
					language,
					cchlanguage,
					// FIXME: send the actual old language
					// instead if just sending the new
					// language as the old language
					language,
					cchlanguage);

			changeLanguageInfo(language,cchlanguage);

		} else {
			if (fsetlangfatal==SET_LANG_FATAL) {
				changeLanguageError(language,cchlanguage,false);
				retval=false;
			} else if (fsetlangwarn==SET_LANG_WARN_ON) {
				changeLanguageError(language,cchlanguage,true);
			}
		}
	}

	// change packet size
	if (retval) {
		negotiatePacketSize(packetsize);
		envChangePacketSize();

		// reset "old" packet size
		oldpacketsize=negotiatedpacketsize;
	}

	// done
	done((retval)?DONE_FINAL:DONE_ERROR,0,0);

	// send the response packet, without losing a login failure
	if (!sendPacket()) {
		retval=false;
	}

	// clean up
	delete[] hostname;
	delete[] username;
	delete[] password;
	delete[] appname;
	delete[] servername;
	delete[] extension;
	delete[] cltintname;
	delete[] language;
	delete[] database;
	delete[] atchdbfile;
	delete[] changepassword;
	delete[] sspi;

	return retval;
}

bool sqlrprotocol_tds::auth(const wchar_t *username,
				size_t usernamelen,
				const wchar_t *password,
				size_t passwordlen) {

	// pre-tds7 clients send single-byte strings and call the narrow
	// version below directly
	char	*username8=charstring::duplicate(username,usernamelen);
	char	*password8=charstring::duplicate(password,passwordlen);

	bool	authsuccess=auth(username8,password8);

	delete[] username8;
	delete[] password8;

	return authsuccess;
}

bool sqlrprotocol_tds::auth(const char *username, const char *password) {

	sqlruserpasswordcredentials	cred;
	cred.setUser(username);
	cred.setPassword(password);

	bool	authsuccess=cont->auth(&cred);

	debugStart("authenticate");
	debugWrite("username: %s",username);
	debugWrite("password: (hidden)");
	debugWrite((authsuccess)?"success":"failed");
	debugEnd();

	return authsuccess;
}

void sqlrprotocol_tds::loginAck(byte_t status) {

	byte_t		token=TOKEN_LOGIN_ACK;

	// For a tds 7.x client this byte names the sql interface, for a
	// pre-tds7 client it reports how the login came out - so "status" is
	// ignored on the tds 7.x path.  A tds 7.x login that fails gets an
	// error token and no login ack at all, so only success reaches here
	// on that path anyway.
	byte_t		iface=(pretds7)?status:SQL_TSQL;
	// unlike the version in the login request, the version in the
	// login ack is sent big-endian
	uint32_t	tdsversion=
			tdsVersionDecToHex(negotiatedtdsversion,true);
	// A pre-tds7 client parses this as the server program name and
	// decides from it what dialect to speak, so it has to be an ase
	// product name rather than the backend's version string.
	const char	*progname=(pretds7)?
				PRE_TDS7_LOGIN_ACK_PROGNAME:dbversion;
	byte_t		prognamelength=(byte_t)charstring::getLength(progname);
	byte_t		majorver=(pretds7)?PRE_TDS7_LOGIN_ACK_MAJORVER:0;
	byte_t		minorver=(pretds7)?PRE_TDS7_LOGIN_ACK_MINORVER:0;
	byte_t		buildnumhi=(pretds7)?PRE_TDS7_LOGIN_ACK_BUILDNUMHI:0;
	byte_t		buildnumlow=(pretds7)?PRE_TDS7_LOGIN_ACK_BUILDNUMLOW:0;

	uint16_t	tokensize=(uint16_t)
				(sizeof(byte_t)+
				sizeof(uint32_t)+
				varcharSize(sizeof(byte_t),
						(size_t)prognamelength)+
				sizeof(byte_t)+
				sizeof(byte_t)+
				sizeof(byte_t)+
				sizeof(byte_t));

	debugStart("login ack");
	debugTokenType(token);
	debugWrite("tokensize: 0x%02x (%hd)",tokensize,tokensize);
	debugWrite("interface: %d",iface);
	debugWrite("tdsversion: 0x%08x (%d)",tdsversion,negotiatedtdsversion);
	debugWrite("prognamelength: %d",prognamelength);
	debugWrite("progname: %s",progname);
	debugWrite("majorver: %d",majorver);
	debugWrite("minorver: %d",minorver);
	debugWrite("buildnumhi: %d",buildnumhi);
	debugWrite("buildnumlow: %d",buildnumlow);
	debugEnd();

	write(&resppacket,token);
	write(&resppacket,tokensize);
	write(&resppacket,iface);
	writeBE(&resppacket,tdsversion);
	writeVarchar(&resppacket,sizeof(byte_t),
				progname,(size_t)prognamelength);
	write(&resppacket,majorver);
	write(&resppacket,minorver);
	write(&resppacket,buildnumhi);
	write(&resppacket,buildnumlow);
}

void sqlrprotocol_tds::authError(const wchar_t *username,
					size_t usernamelen) {

	// pre-tds7 clients send single-byte strings and call the narrow
	// version below directly
	char	*username8=charstring::duplicate(username,usernamelen);

	authError(username8);

	delete[] username8;
}

void sqlrprotocol_tds::authError(const char *username) {

	debugStart("auth error");
	debugWrite("username: %s",username);
	debugEnd();

	stringbuffer	err;
	err.append("Login failed for user '");
	err.append(username);
	err.append("'.");

	appendError(18456,1,14,err.getString(),srvname,NULL,0);
}

bool sqlrprotocol_tds::changeDatabase(const wchar_t *database,
						size_t databaselen) {

	char	*database8=charstring::duplicate(database,databaselen);

	bool	changedbsuccess=cont->selectDatabase(database8);

	debugStart("change db");
	debugWrite("db: %s",database8);
	debugWrite((changedbsuccess)?"success":"failed");
	debugEnd();

	delete[] database8;

	return changedbsuccess;
}

void sqlrprotocol_tds::changeDatabaseInfo(const wchar_t *database,
						size_t databaselen) {

	char	*database8=charstring::duplicate(database,databaselen);

	debugStart("change db info");
	debugWrite("db: %s",database8);
	debugEnd();

	stringbuffer	inf;
	inf.append("Changed database context to '");
	inf.append(database8)->append("'.");

	appendInfo(5701,2,0,inf.getString(),NULL,NULL,1);

	delete[] database8;
}

void sqlrprotocol_tds::changeDatabaseError(const wchar_t *database,
						size_t databaselen,
						bool warning) {

	char	*database8=charstring::duplicate(database,databaselen);

	debugStart("change db error");
	debugWrite("db: %s",database8);
	debugWrite("warning: %d",warning);
	debugEnd();

	// FIXME: verify this message for warning
	stringbuffer	err;
	err.append("Cannot open database '")->append(database8);
	err.append("' requested by the login.");
	if (!warning) {
		err.append(" The login failed.");
	}

	// FIXME: verify these for warning
	appendError(4060,1,(warning)?9:11,err.getString(),srvname,NULL,1);

	delete[] database8;
}

bool sqlrprotocol_tds::changeCollation(uint32_t lcid) {

	bool		lcidignorecase=(lcid&(0x01));
	bool		lcidignoreaccent=(lcid&(0x01<<1))>>1;
	bool		lcidignorewidth=(lcid&(0x01<<2))>>2;
	bool		lcidignorekana=(lcid&(0x01<<3))>>3;
	bool		lcidbinary=(lcid&(0x01<<4))>>4;
	bool		lcidbinary2=(lcid&(0x01<<5))>>5;
	byte_t		lcidversion=(lcid&(0x0F<<8))>>8;

	// the lcid in a login is a request, not a setting - there is no
	// per-session collation to switch here, so any lcid is accepted
	bool	changecollationsuccess=true;

	if (getDebug()) {
		debugStart("change collation");
		stringbuffer	b;
		b.printBits(lcid);
		debugWrite("lcid: %s",b.getString());
		debugWrite("lcidignorecase: %d",lcidignorecase);
		debugWrite("lcidignoreaccent: %d",lcidignoreaccent);
		debugWrite("lcidignorewidth: %d",lcidignorewidth);
		debugWrite("lcidignorekana: %d",lcidignorekana);
		debugWrite("lcidbinary: %d",lcidbinary);
		debugWrite("lcidbinary2: %d",lcidbinary2);
		debugWrite("lcidversion: %d",lcidversion);
		debugWrite((changecollationsuccess)?"success":"failed");
		debugEnd();
	}

	return changecollationsuccess;
}

void sqlrprotocol_tds::envChangeSqlCollation(uint32_t lcid,
						byte_t sortid) {

	byte_t		token=TOKEN_ENV_CHANGE;

	byte_t		type=ENV_CHANGE_SQL_COLLATION;
	
	uint16_t	tokensize=
				sizeof(byte_t)+
				sizeof(byte_t)+
				sizeof(uint32_t)+
				sizeof(byte_t)+
				sizeof(byte_t);

	if (getDebug()) {
		debugStart("env change");
		debugTokenType(token);
		debugWrite("tokensize: 0x%02x (%hd)",tokensize,tokensize);
		debugEnvChangeType(type);
		debugWrite("newvaluesize: %lld",
				(long long)(sizeof(uint32_t)+sizeof(byte_t)));
		stringbuffer	b;
		b.printBits(lcid);
		debugWrite("newvalue: %s %d",b.getString(),sortid);
		debugHexDump((byte_t *)&lcid,sizeof(lcid));
		debugHexDump((byte_t *)&sortid,sizeof(sortid));
		debugWrite("oldvaluesize: 0");
		debugEnd();
	}

	write(&resppacket,token);
	writeLE(&resppacket,tokensize);
	write(&resppacket,type);
	write(&resppacket,(byte_t)(sizeof(lcid)+sizeof(sortid)));
	// a collation is a little-endian 32-bit lcid/flags/version
	// bitmap followed by a sort id
	writeLE(&resppacket,lcid);
	write(&resppacket,sortid);
	write(&resppacket,(byte_t)0);
}

bool sqlrprotocol_tds::changeLanguage(const wchar_t *language,
						size_t languagelen) {

	char	*language8=charstring::duplicate(language,languagelen);

	// FIXME: actually implement this...

	bool	changelangsuccess=true;

	debugStart("change lang");
	debugWrite("lang: %s",language8);
	debugWrite((changelangsuccess)?"success":"failed");
	debugEnd();

	delete[] language8;

	return changelangsuccess;
}

void sqlrprotocol_tds::changeLanguageInfo(const wchar_t *language,
						size_t languagelen) {

	char	*language8=charstring::duplicate(language,languagelen);

	debugStart("change lang info");
	debugWrite("lang: %s",language8);
	debugEnd();

	stringbuffer	inf;
	inf.append("Changed language setting to ");
	inf.append(language8)->append(".");

	appendInfo(5703,1,0,inf.getString(),NULL,NULL,1);

	delete[] language8;
}

void sqlrprotocol_tds::changeLanguageError(const wchar_t *language,
						size_t languagelen,
						bool warning) {

	char	*language8=charstring::duplicate(language,languagelen);

	debugStart("change lang error");
	debugWrite("lang: %s",language8);
	debugWrite("warning: %d",warning);
	debugEnd();

	// FIXME: verify this message for error and warning
	stringbuffer	err;
	err.append("Cannot change language to '");
	err.append(language8);
	err.append("' requested by the login.");
	if (!warning) {
		err.append(" The login failed.");
	}

	// FIXME: verify these for warning
	appendError(0,1,(warning)?9:10,err.getString(),srvname,NULL,1);

	delete[] language8;
}

void sqlrprotocol_tds::negotiatePacketSize(uint32_t packetsize) {

	bool	changepacketsizesuccess=true;
	if (packetsize>=MIN_PACKET_SIZE) {
		negotiatedpacketsize=(packetsize>configmaxpacketsize)?
					configmaxpacketsize:packetsize;
		// FIXME: reset read/write buffer sizes?
	} else {
		changepacketsizesuccess=false;
	}

	debugStart("change packet size");
	debugWrite("requested packetsize: %d",packetsize);
	debugWrite("negotiated packetsize: %d",negotiatedpacketsize);
	debugWrite((changepacketsizesuccess)?"success":"failed");
	debugEnd();
}

void sqlrprotocol_tds::envChangePacketSize() {

	debugStart("env change packet size");
	debugWrite("negotiatedpacketsize: %d",negotiatedpacketsize);
	debugWrite("oldpacketsize: %d",oldpacketsize);
	debugEnd();

	char		*npsize=charstring::parseNumber(negotiatedpacketsize);
	uint32_t	npsizelength=charstring::getLength(npsize);
	wchar_t		*npsize32=wcharstring::duplicate(npsize,npsizelength);

	char		*opsize=charstring::parseNumber(oldpacketsize);
	uint32_t	opsizelength=charstring::getLength(opsize);
	wchar_t		*opsize32=wcharstring::duplicate(opsize,opsizelength);

	envChange(ENV_CHANGE_PACKET_SIZE,
				npsize32,npsizelength,
				opsize32,opsizelength);

	delete[] npsize32;
	delete[] npsize;
	delete[] opsize32;
	delete[] opsize;
}

bool sqlrprotocol_tds::federatedAuthenticationToken() {

	debugStart("fed auth token");
	debugEnd();

	// FIXME: actually implement this

	return sendUnimplementedFeatureError();
}

bool sqlrprotocol_tds::attention() {

	debugStart("attention");
	debugEnd();

	// the query has already run by the time the cancel arrives, so
	// there's nothing to interrupt, but MS-TDS 2.2.1.6 says to
	// acknowledge it with a done that has the attention bit set

	// the client won't send the sp_unprepare or sp_cursorclose that
	// would have released the cursor it left open, so put it back here
	// (the protocol doesn't say which handle the attention is for, so
	// it's the request's own cursor, and only if this module minted it)
	if (pendingcursor &&
		(handlesContain(&stmthandles,pendingcursor) ||
		handlesContain(&cursorhandles,pendingcursor))) {
		releaseCursorHandles(pendingcursor);
	}
	pendingcursor=NULL;

	resppacket.clear();
	done(DONE_FINAL|DONE_ATTN,0,0);
	return sendPacket();
}

bool sqlrprotocol_tds::transactionManagerRequest() {

	//const byte_t	*rp=reqpacket.getBuffer();

	debugStart("tx mgr request");
	debugEnd();

	// FIXME: actually implement this

	return sendUnimplementedFeatureError();
}

bool sqlrprotocol_tds::sspi() {

	//const byte_t	*rp=reqpacket.getBuffer();

	debugStart("sspi");
	debugEnd();

	// FIXME: actually implement this

	return sendUnimplementedFeatureError();
}

// How many bytes the length field of a tds 5.0 request token takes.
// There's no rule that derives this from the token byte - see the note
// at the TDS5_LENSIZE_* defines - so it's a table, and anything not in
// it is TDS5_LENSIZE_UNKNOWN and can't be stepped over.
//
// preTds7Normal() dispatches language, dbrpc and dynamic itself, and
// uses this to step over a command it can't answer so that the walk can
// reach the commands behind it.
//
// Re-verify a value against a capture before relying on one.  Freetds
// and the wireshark dissector agree with everything here that they
// define, but they don't define curdeclare2/3, curupdate, curinfo2/3,
// dbrpc2 or key at all, and those rest on the spec alone - and the spec
// is not reliable on its own either.  Freetds calls dynamic2 0xA3,
// which is a *type* byte here; a real sap client sends 0x62.  The
// wireshark dissector was seeded from freetds, so the two are one
// source rather than two.
byte_t sqlrprotocol_tds::preTds7TokenLength(byte_t token) {

	switch (token) {
		case TDS5_TOKEN_CURDECLARE3:
		case TDS5_TOKEN_PARAMFMT2:
		case TDS5_TOKEN_LANGUAGE:
		case TDS5_TOKEN_ORDERBY2:
		case TDS5_TOKEN_CURDECLARE2:
		case TDS5_TOKEN_ROWFMT2:
		case TDS5_TOKEN_DYNAMIC2:
			return TDS5_LENSIZE_UINT;
		case TDS5_TOKEN_MSG:
			return TDS5_LENSIZE_BYTE;
		case TDS5_TOKEN_LOGOUT:
			// one options byte, which must be 0, rather than
			// a length
			return TDS5_LENSIZE_FIXED1;
		case TDS5_TOKEN_CURCLOSE:
		case TDS5_TOKEN_CURDELETE:
		case TDS5_TOKEN_CURFETCH:
		case TDS5_TOKEN_CURINFO:
		case TDS5_TOKEN_CUROPEN:
		case TDS5_TOKEN_CURUPDATE:
		case TDS5_TOKEN_CURDECLARE:
		case TDS5_TOKEN_CURINFO2:
		case TDS5_TOKEN_CURINFO3:
		case TDS5_TOKEN_OPTIONCMD:
		case TDS5_TOKEN_CAPABILITY:
		case TDS5_TOKEN_DBRPC:
		case TDS5_TOKEN_DYNAMIC:
		case TDS5_TOKEN_DBRPC2:
		case TDS5_TOKEN_PARAMFMT:
			return TDS5_LENSIZE_USHORT;
		default:
			// params, key and row carry no length of their own -
			// they can only be sized by replaying the paramfmt or
			// rowfmt in front of them.  optioncmd2 is reserved and
			// was never defined.  and an unrecognized token could
			// be anything at all.
			return TDS5_LENSIZE_UNKNOWN;
	}
}

// Refuses the rest of a request that carries more commands than
// MAX_COMMANDS_PER_REQUEST, with its own done, so the client sees the
// refusal rather than waiting for results that won't come.
// "token" is the done token that ends the request being refused - a batch
// of rpc's is closed with doneproc, the way rpc() closes each one it ran.
void sqlrprotocol_tds::tooManyCommands(byte_t token) {

	char	countstr[11];
	charstring::printf(countstr,sizeof(countstr),"%d",
					MAX_COMMANDS_PER_REQUEST);

	stringbuffer	err;
	err.append("A request may carry at most ");
	err.append(countstr);
	err.append(" commands.");

	// FIXME: is there a real error number/state for this?
	// class 16 for the same reason preTds7UnsupportedToken() uses it -
	// the session is still perfectly usable
	appendError(0,1,16,err.getString(),srvname,NULL,1);

	done(token,DONE_ERROR|DONE_FINAL,transState(),0);
}

// Refuses one token, with its own done, so the client sees this command
// fail rather than being left waiting for a result that never comes.
// "more" is whether the walk goes on past it.
void sqlrprotocol_tds::preTds7UnsupportedToken(byte_t token, bool more) {

	char	tokenstr[3];
	charstring::printf(tokenstr,sizeof(tokenstr),"%02x",
					(uint32_t)(0x000000ff&token));

	stringbuffer	err;
	err.append("TDS 5.0 request token 0x");
	err.append(tokenstr);
	err.append(" is not supported yet.");

	// FIXME: is there a real error number/state for this?
	// class 16 rather than 20-and-up on purpose - a class 20 error is
	// fatal and the client hangs up on it, and the session is still
	// perfectly usable
	appendError(0,1,16,err.getString(),srvname,NULL,1);

	// DONE_ERROR is what turns this into a CS_CMD_FAIL - ct-lib reports
	// CS_CMD_SUCCEED for a done without it.  DONE_MORE when the walk
	// goes on past this token - ct-lib stops reading at the first done
	// without it, and the dones of the commands behind this one would be
	// left sitting in the socket.
	done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),transState(),0);
}

// Steps over a command token this module can't answer, along with the
// paramfmt/params pair behind it if it brought one, so that the commands
// behind it in the buffer can still be walked.  The token byte has
// already been read.
//
// Returns false, having appended nothing at all, if the token can't be
// stepped over - one that carries no length of its own, one that runs
// off the end of the buffer, or a paramfmt/params pair that can't be
// read.  Then there's no telling where the next token starts and the
// walk has to stop.
bool sqlrprotocol_tds::preTds7SkipCommand(const byte_t **rpinout,
						size_t *rpsizeinout,
						byte_t token) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	// A paramfmt or params is never a command of its own - it belongs to
	// the command in front of it, which consumes it below.  One turning
	// up where a command should be means the walk has already lost its
	// place, so stop rather than pretend to step over a command.
	if (token==TDS5_TOKEN_PARAMFMT || token==TDS5_TOKEN_PARAMFMT2 ||
					token==TDS5_TOKEN_PARAMS) {
		return false;
	}

	// how much of the buffer the token itself takes
	size_t	tokenlength=0;
	switch (preTds7TokenLength(token)) {
		case TDS5_LENSIZE_FIXED1:
			// no length field, one fixed byte of payload
			tokenlength=1;
			break;
		case TDS5_LENSIZE_BYTE:
			{
			byte_t	bytelength=0;
			if (rpsize<sizeof(bytelength)) {
				return false;
			}
			read(rp,&bytelength,&rp);
			rpsize-=sizeof(bytelength);
			tokenlength=bytelength;
			}
			break;
		case TDS5_LENSIZE_USHORT:
			{
			uint16_t	shortlength=0;
			if (rpsize<sizeof(shortlength)) {
				return false;
			}
			read(rp,&shortlength,&rp);
			rpsize-=sizeof(shortlength);
			tokenlength=shortlength;
			}
			break;
		case TDS5_LENSIZE_UINT:
			{
			uint32_t	intlength=0;
			if (rpsize<sizeof(intlength)) {
				return false;
			}
			read(rp,&intlength,&rp);
			rpsize-=sizeof(intlength);
			tokenlength=(size_t)intlength;
			}
			break;
		default:
			// an unrecognized token could be anything at all
			return false;
	}

	if (tokenlength>rpsize) {
		return false;
	}
	rp+=tokenlength;
	rpsize-=tokenlength;

	// Which commands can carry a paramfmt/params pair is a status bit
	// inside each one's own body, and nothing here decodes bodies, so go
	// by what's actually on the wire instead - a paramfmt is never a
	// command on its own, so one sitting here can only belong to the
	// token just stepped over.
	//
	// The pair is read rather than skipped because a params token
	// carries no lengths of its own; replaying the paramfmt in front of
	// it is the only way to find its end.  What the read leaves in the
	// format and parameter arrays is scratch - the command it belongs to
	// is being refused - and the next command that needs them fills them
	// in again.
	if (rpsize && (*rp==TDS5_TOKEN_PARAMFMT ||
					*rp==TDS5_TOKEN_PARAMFMT2)) {

		byte_t	fmttoken=0;
		read(rp,&fmttoken,&rp);
		rpsize--;

		const char	*err=NULL;
		if (!preTds7ParamFmtRead(&rp,&rpsize,
					(fmttoken==TDS5_TOKEN_PARAMFMT2),
					&err)) {
			return false;
		}

		if (!rpsize) {
			return false;
		}
		byte_t	paramstoken=0;
		read(rp,&paramstoken,&rp);
		rpsize--;
		if (paramstoken!=TDS5_TOKEN_PARAMS ||
					!preTds7ParamsRead(&rp,&rpsize)) {
			return false;
		}
	}

	*rpinout=rp;
	*rpsizeinout=rpsize;

	return true;
}

// A tds 5.0 "normal" buffer carries token-framed requests rather than the
// bare payload that a tds 7.x sql batch or rpc packet carries.  This walks
// the tokens in one.
//
// Modelled on remoteProcedureCall(), which does the same thing for a batch
// of tds 7.x rpc's: clear the response once, append a done per command as
// each is answered, and send the whole thing at the end.  The buffer can
// carry several commands and ct-lib expects one done per command, not one
// per buffer, so a command that isn't the last one sets DONE_MORE.
bool sqlrprotocol_tds::preTds7Normal() {

	const byte_t	*rp=reqpacket.getBuffer();
	size_t		rpsize=reqpacket.getSize();

	debugStart("pre-tds7 normal");

	// recvPacket() takes this packet type whatever the session logged
	// in as, but everything written back from here - errors included -
	// is sized by charSize(), which only gets the tds 5.0 shape when
	// pretds7 is set.  a tds 7.x session that sends one of these has
	// no business doing so and would get a response it can't read.
	if (!pretds7) {
		debugWrite("not a pre-tds7 session");
		debugEnd();
		return sendTdsProtocolError();
	}

	// begin building the response packet
	resppacket.clear();

	bool		anycommands=false;
	uint32_t	commandcount=0;

	while (rpsize) {

		// get the token
		byte_t	token=0;
		read(rp,&token,&rp);
		rpsize--;

		debugStart("pre-tds7 request token");
		debugPreTds7TokenType(token);
		debugEnd();

		anycommands=true;

		// A logout isn't a command to refuse - it's the client
		// hanging up, and ct_close() waits for a done before it
		// does.  Answering it with an error would put a spurious
		// message through the client's callback on a perfectly
		// normal disconnect.  It isn't counted against the command
		// limit either, so a full buffer that ends in one still gets
		// a clean disconnect rather than that same spurious error.
		if (token==TDS5_TOKEN_LOGOUT) {
			debugWrite("logout");
			done(DONE_FINAL,transState(),0);
			break;
		}

		// Too many commands in one buffer.  A parameterized command
		// is three tokens on the wire but one command, and it counts
		// as one here because whoever handles the command token
		// consumes the paramfmt/params pair behind it rather than
		// letting the walk see them.
		if (commandcount==MAX_COMMANDS_PER_REQUEST) {
			debugWrite("too many commands");
			tooManyCommands(TOKEN_DONE);
			break;
		}
		commandcount++;

		// the commands this module can answer so far
		if (token==TDS5_TOKEN_LANGUAGE) {
			if (preTds7Language(&rp,&rpsize)) {
				continue;
			}
			// the token was malformed, or asked for something
			// that isn't implemented.  it has appended its own
			// error and final done, and the rest of the buffer
			// can't be trusted, so stop here
			break;
		}
		if (token==TDS5_TOKEN_DBRPC) {
			if (preTds7DbRpc(&rp,&rpsize)) {
				continue;
			}
			break;
		}
		if (token==TDS5_TOKEN_DYNAMIC) {
			if (preTds7Dynamic(&rp,&rpsize)) {
				continue;
			}
			break;
		}

		// Anything else.  There's nothing to answer the command with
		// either way, so it gets refused either way; the only question
		// is whether the walk can go on.  A token that carries its own
		// length can be stepped over, so refuse just that command and
		// keep walking.  One that can't be stepped over ends the walk
		// - an unrecognized token could be anything at all, and
		// there's no finding the token behind it.
		// FIXME: #9479 implements the cursor tokens, #9480 bulk copy
		if (!preTds7SkipCommand(&rp,&rpsize,token)) {
			preTds7UnsupportedToken(token,false);
			break;
		}
		preTds7UnsupportedToken(token,rpsize>0);
	}

	// An empty buffer would otherwise get an empty response, which
	// leaves the client waiting forever.
	if (!anycommands) {
		debugWrite("no tokens in the buffer");
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,"Empty TDS 5.0 request buffer",
							srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
	}

	debugEnd();

	// send the response packet
	return sendPacket();
}

// Handles one tds 5.0 language token, appending its result to the
// response packet.  Returns false if the walk should stop, having
// already appended an error and a final done.
//
// The token is:
//	int32, little-endian	how much follows - the status byte
//				plus the sql
//	byte			status - 0x01 means a paramfmt/params
//				pair follows
//	bytes			the sql, as single-byte characters,
//				not nul terminated
bool sqlrprotocol_tds::preTds7Language(const byte_t **rpinout,
					size_t *rpsizeinout) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	debugStart("pre-tds7 language");

	// get the token length
	uint32_t	tokenlength=0;
	if (rpsize<sizeof(tokenlength)) {
		debugWrite("truncated token length");
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,"Malformed TDS 5.0 language token",
							srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
		return false;
	}
	read(rp,&tokenlength,&rp);
	rpsize-=sizeof(tokenlength);

	debugWrite("token length: %lld",(long long)tokenlength);

	// The length covers the status byte, so a token that doesn't have
	// room for one is malformed, as is one that runs off the end of
	// the buffer.
	if (tokenlength<sizeof(byte_t) || (size_t)tokenlength>rpsize) {
		debugWrite("invalid token length: %lld",(long long)tokenlength);
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,"Malformed TDS 5.0 language token",
							srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
		return false;
	}

	// get the status byte
	byte_t	status=0;
	read(rp,&status,&rp);
	rpsize--;

	debugWrite("status: 0x%02x",status);

	// what's left of the token is the sql
	size_t	sqllength=(size_t)tokenlength-sizeof(status);

	// bounds checking.  a single check is enough here, unlike
	// sqlBatch(), which checks again after converting - ucs-2 to utf-8
	// can grow, and passing single bytes through can't.
	if (sqllength>maxquerysize) {
		debugWrite("query too large: %lld",(long long)sqllength);
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		stringbuffer	err;
		queryTooLargeMessage(sqllength,&err);
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,err.getString(),srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
		return false;
	}

	// decode the sql
	size_t	sql8size;
	char	*sql8=preTds7ToUtf8(rp,sqllength,&sql8size);
	rp+=sqllength;
	rpsize-=sqllength;

	debugWrite("sql: %s",sql8);
	debugWrite("sqllength: %lld",(long long)sql8size);

	// copy out what's been consumed, so that the walker sees this
	// token gone whichever way the rest of this goes
	*rpinout=rp;
	*rpsizeinout=rpsize;

	// A parameterized command's values ride in the paramfmt/params pair
	// behind this token, so read both before the sql can be run.
	uint16_t	paramcount=0;
	if (status&TDS5_LANGUAGE_PARAMS) {

		if (!preTds7ParamFmtAndParams(&rp,&rpsize)) {
			debugEnd();
			delete[] sql8;
			*rpinout=rp;
			*rpsizeinout=0;
			return false;
		}

		paramcount=rpcparamcount;

		// copy out what the pair consumed too
		*rpinout=rp;
		*rpsizeinout=rpsize;
	}

	// Whether another command follows this one in the buffer.  This has
	// to be worked out here, behind the paramfmt/params pair rather than
	// in front of it - the pair is part of this command, so counting it
	// as "more" would put DONE_MORE on the last command's done, and
	// ct-lib would sit waiting for a done that never comes.
	bool	more=(rpsize>0);

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugWrite("no cursor available");
		debugEnd();
		delete[] sql8;
		*rpsizeinout=0;
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,"No cursor available",srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
		return false;
	}

	// tds 5.0 has no in/out parameter direction - a paramfmt says input
	// or return, not both - and the cursor may have been left with some
	// by something that used it earlier
	cont->setInputOutputBindCount(cursor,0);

	// A language command without parameters has no bind variables at
	// all, and @name in bare sql is a local variable or a parameter
	// declaration rather than one, so bind translation is off for that
	// case the way sqlBatch() turns it off.
	//
	// With parameters it's sp_executesql's shape instead: the names are
	// in the sql text - freetds rewrites each ? to @P1, @P2 ... and
	// names the parameters to match - and bindParams() renames the binds
	// to @1, @2 ... by position, so translation has to stay on to pair
	// the two up in order.  getCursor() has already reset it to on.
	if (!paramcount) {
		cont->setInputBindCount(cursor,0);
		cont->setOutputBindCount(cursor,0);
		cont->setTranslateBindVariablesForThisQuery(cursor,false);
	}

	// run the query.  the binds go on between prepare and execute, the
	// way sp_executesql does it
	bool	success=cont->prepareQuery(cursor,sql8,(uint32_t)sql8size,
							true,true,true,true);
	if (success) {
		if (paramcount) {
			bindParams(cursor,0);
		}
		success=cont->executeQuery(cursor,true,true,true,true);
	}

	// clean up
	delete[] sql8;

	if (success) {

		if (cont->colCount(cursor)) {

			// A result set in this dialect is a rowfmt (0xEE)
			// and rows (0xD1), not the colmetadata (0x81) that
			// colMetaData() writes - 0x81 is a cursor-delete
			// token here.
			//
			// A language token carries free text, so it can hold
			// a multi-statement batch, and a backend that walks
			// result sets can answer one with several of them.
			// Send one rowfmt/rows/done group per result set, the
			// way sqlBatch() does, with DONE_MORE on all but the
			// last - ct-lib stops reading at the first done
			// without it.
			//
			// Unlike sqlBatch() there's no return-status/done-proc
			// pairing.  Those report an "exec" inside an ms-tds
			// batch and tds 5.0 has neither token.
			for (;;) {

				// A result set that's too wide for the token
				// is refused in here, with its own error and
				// done, so don't send a second one.
				if (!preTds7RowFmt(cursor,more)) {
					break;
				}

				uint64_t	rowcount=preTds7Rows(cursor);

				bool	avail=false;
				if (!cont->nextResultSet(cursor,&avail)) {
					// "more" rather than DONE_FINAL for
					// the same reason as the failed-query
					// done below - the buffer may still
					// hold commands whose dones follow
					// this one, and ct-lib stops reading
					// at the first done without DONE_MORE
					appendQueryError(cursor);
					done(DONE_ERROR|DONE_COUNT|
						((more)?DONE_MORE:DONE_FINAL),
						transState(),rowcount);
					break;
				}

				// DONE_COUNT is what makes the count valid -
				// without it ct_res_info(CS_ROW_COUNT) reports
				// no count at all
				done(((avail || more)?
						DONE_MORE:DONE_FINAL)|
						DONE_COUNT,
						transState(),rowcount);

				if (!avail) {
					break;
				}
			}

		} else {
			// DONE_COUNT is what makes the count valid - without
			// it ct_res_info(CS_ROW_COUNT) reports no count at all
			done(((more)?DONE_MORE:DONE_FINAL)|DONE_COUNT,
					transState(),
					cont->getAffectedRows(cursor));
		}

	} else {

		// DONE_ERROR is what turns this into a CS_CMD_FAIL - ct-lib
		// reports CS_CMD_SUCCEED for a done without it, and the
		// client's result walk would fall a result out of step
		appendQueryError(cursor);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),
						transState(),0);
	}

	debugEnd();

	// release the cursor
	// FIXME: kludgy - same as sqlBatch()
	releaseCursor(cursor);

	return true;
}

// Handles one tds 5.0 dbrpc token, appending its result to the response
// packet.  Returns false if the walk should stop, having already
// appended an error and a done.
//
// A dbrpc names its procedure by string where an ms-tds rpc names the
// numbered ones by id, and its parameters ride in the paramfmt/params
// pair behind it rather than inside the token.  So this is the decode
// half of rpc() rather than a sibling of it, and it hands what it
// decoded to the same proc dispatch.
//
// The token is:
//	uint16, little-endian	how much follows - the name length byte,
//				the name and the options
//	byte			name length
//	bytes			the name, as single-byte characters, not
//				nul terminated
//	uint16, little-endian	options - TDS5_RPC_PARAMS when a
//				paramfmt/params pair follows
//
// Dbrpc2 (0xE8) gets no case of its own.  No client was ever seen
// sending one and freetds has no code that can, so it stays with the
// tokens preTds7Normal() refuses rather than being guessed at.
bool sqlrprotocol_tds::preTds7DbRpc(const byte_t **rpinout,
					size_t *rpsizeinout) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	debugStart("pre-tds7 dbrpc");

	// get the token length
	uint16_t	tokenlength=0;
	if (rpsize<sizeof(tokenlength)) {
		debugWrite("truncated token length");
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		preTds7ParamError("Malformed TDS 5.0 dbrpc token",false);
		return false;
	}
	read(rp,&tokenlength,&rp);
	rpsize-=sizeof(tokenlength);

	debugWrite("token length: %d",tokenlength);

	// The length covers the name-length byte and the options, so a
	// token without room for both is malformed, as is one that runs
	// off the end of the buffer.
	if ((size_t)tokenlength<sizeof(byte_t)+sizeof(uint16_t) ||
					(size_t)tokenlength>rpsize) {
		debugWrite("invalid token length: %d",tokenlength);
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		preTds7ParamError("Malformed TDS 5.0 dbrpc token",false);
		return false;
	}

	// everything below reads out of the token's own body, so remember
	// where that ends - a name shorter than the length allows for
	// leaves bytes to step over
	const byte_t	*body=rp;
	size_t		bodyleft=tokenlength;

	// get the proc name length
	byte_t	namelen=0;
	read(rp,&namelen,&rp);
	bodyleft-=sizeof(namelen);

	debugWrite("proc name length: %d",namelen);

	// the options still have to fit behind the name
	if ((size_t)namelen+sizeof(uint16_t)>bodyleft) {
		debugWrite("invalid proc name length: %d",namelen);
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		preTds7ParamError("Malformed TDS 5.0 dbrpc token",false);
		return false;
	}

	// the name is decoded further down - it can't be refused until the
	// paramfmt/params pair behind it has been stepped over
	const byte_t	*name=rp;
	rp+=namelen;
	bodyleft-=namelen;

	// get the option flags
	uint16_t	options=0;
	read(rp,&options,&rp);
	bodyleft-=sizeof(options);

	if (getDebug()) {
		stringbuffer	b;
		b.printBits(options);
		debugWrite("options: %s",b.getString());
	}

	// step over anything the token declared past the options
	rp=body+tokenlength;
	rpsize-=tokenlength;

	// copy out what's been consumed, so that the walker sees this
	// token gone whichever way the rest of this goes
	*rpinout=rp;
	*rpsizeinout=rpsize;

	// The values ride in the paramfmt/params pair behind the token, and
	// the count has to be reset either way - namedProc() builds its
	// query from it, and whatever ran before this left its own count
	// behind.
	rpcparamcount=0;
	if (options&TDS5_RPC_PARAMS) {

		if (!preTds7ParamFmtAndParams(&rp,&rpsize)) {
			debugEnd();
			*rpinout=rp;
			*rpsizeinout=0;
			return false;
		}

		// copy out what the pair consumed too
		*rpinout=rp;
		*rpsizeinout=rpsize;
	}

	// Whether another command follows this one in the buffer.  This has
	// to be worked out behind the paramfmt/params pair rather than in
	// front of it, for the reason preTds7Language() spells out - the
	// pair is part of this command, so counting it as "more" would put
	// DONE_MORE on the last command's done and leave ct-lib waiting for
	// a done that never comes.
	bool	more=(rpsize>0);

	// a call with no procedure to call
	if (!namelen) {
		debugWrite("empty proc name");
		debugEnd();
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,"Empty TDS 5.0 dbrpc procedure name",
							srvname,NULL,1);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),
							transState(),0);
		return true;
	}

	size_t	procnamesize;
	char	*procname=preTds7ToUtf8(name,namelen,&procnamesize);

	debugWrite("procname: %s",procname);

	// a client can send any of the numbered procs by name
	uint16_t	procid=procNameToProcId(procname);
	if (procid) {
		debugProcId(procid);
	}

	// The numbered procs whose reply tail is still ms-tds only.  Their
	// cores are wire-neutral, but each of them either writes
	// colmetadata (0x81) and rows (0xD1) straight out rather than going
	// through rpcResultSet(), or sends several output parameters, which
	// is one paramfmt/params pair in this dialect rather than one token
	// each.  Answering one would desynchronize the client rather than
	// fail it, and 0x81 is the cursor-delete token here.  A ct-lib
	// client has cursor tokens of its own and never sends these.
	// FIXME: give these a pre-tds7 reply tail along with the tds 5.0
	// cursor tokens
	if (procid==SP_CURSOR_PREPARE || procid==SP_CURSOR_OPEN ||
				procid==SP_CURSOR_EXECUTE ||
				procid==SP_CURSOR_PREP_EXEC ||
				procid==SP_CURSOR_FETCH) {
		debugWrite("proc not supported over tds 5.0");
		debugEnd();
		delete[] procname;
		stringbuffer	err;
		err.append("Procedure ")->append(procnames[procid]);
		err.append(" is not supported over TDS 5.0 yet.");
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,err.getString(),srvname,NULL,1);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),
							transState(),0);
		return true;
	}

	// do whatever the proc asked for.  there's no "no metadata" option
	// to pass along - tds 5.0 has nothing like it
	rpcfailed=false;
	rpcunsupportedtype=false;
	bool	retval=runProc(procid,procname,false);

	delete[] procname;

	// A handler that gave up can't say where the request stream now
	// stands, so end the walk after this command's own done rather
	// than leaving DONE_MORE on it.
	if (!retval) {
		more=false;
		*rpsizeinout=0;
	}

	// A failed rpc has to set DONE_ERROR - ct-lib reports
	// CS_CMD_SUCCEED for a done without it, so a failed call would
	// report success and the client's result walk would fall a result
	// out of step.  A real ase closes an rpc with a plain done (0xFD)
	// rather than with the done-proc an ms-tds server sends.
	uint16_t	donestatus=(more)?DONE_MORE:DONE_FINAL;
	if (rpcfailed) {
		donestatus|=DONE_ERROR;
	}
	done(donestatus,transState(),0);

	debugEnd();

	return retval;
}

// Refuses one dynamic sql command, with its own done, so the client sees
// that command fail rather than being left waiting for a result that
// never comes.  Class 16 for the same reason preTds7UnsupportedToken()
// uses it - the session stays usable.
void sqlrprotocol_tds::preTds7DynamicError(const char *msgtext, bool more) {
	// FIXME: is there a real error number/state for this?
	appendError(0,1,16,msgtext,srvname,NULL,1);
	done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),transState(),0);
}

// Looks a dynamic sql statement id up.  A live one names a prepared
// statement handle that still has a cursor; one whose cursor was evicted
// to make room for another request is dropped here rather than left to
// be found again.
bool sqlrprotocol_tds::dynamicHandle(const char *id, uint32_t *handle) {
	*handle=0;
	if (!dynamicids.getValue((char *)id,handle)) {
		return false;
	}
	if (!handleCursor(&stmthandles,*handle)) {
		debugWrite("dynamic id: %s (evicted)",id);
		dynamicids.remove((char *)id);
		*handle=0;
		return false;
	}
	return true;
}

// Names a prepared statement handle with a dynamic sql statement id,
// replacing whatever that id named before.
void sqlrprotocol_tds::setDynamicHandle(const char *id, uint32_t handle) {

	// the map owns its keys, so drop the old one rather than leaving
	// two entries for the same id
	removeDynamicHandle(id);

	if (dynamicids.getCount()>=MAX_DYNAMIC_IDS) {
		evictOldestDynamicHandle();
	}

	dynamicids.setValue(charstring::duplicate(id),handle);
}

void sqlrprotocol_tds::removeDynamicHandle(const char *id) {
	dynamicids.remove((char *)id);
}

// Drops the oldest dynamic sql statement id, along with the cursor its
// handle was holding.  The same thing evictOldestHandle() does for the
// handle maps, and for the same reason - a client can walk off and leave
// ids prepared forever.
void sqlrprotocol_tds::evictOldestDynamicHandle() {

	debugStart("evict-oldest-dynamic-handle");

	// the dictionary tracks insertion order, so the first key is the
	// oldest id
	listnode<char *>	*node=dynamicids.getKeys()->getFirst();
	if (!node) {
		debugEnd();
		return;
	}

	debugWrite("dynamic id: %s (evicted)",node->getValue());

	uint32_t		handle=0;
	sqlrservercursor	*cursor=NULL;
	if (dynamicids.getValue(node->getValue(),&handle)) {
		cursor=handleCursor(&stmthandles,handle);
	}

	dynamicids.remove(node->getValue());

	if (cursor) {
		unprepareStatement(handle,cursor);
	}

	debugEnd();
}

// Writes the dynamic ack that answers every dynamic sql command.  Unlike
// the request form, this one stops after the id - it has no statement
// length field at all, which is how a client tells a server's dynamic
// token from its own.
void sqlrprotocol_tds::preTds7DynamicAck(const char *id, size_t idsize) {

	byte_t	token=TDS5_TOKEN_DYNAMIC;

	// The id arrived in the client's charset and preTds7ToUtf8()
	// converted it, so it goes back converted the other way - and
	// before the sizes below, which count the bytes on the wire.
	size_t	convidsize=0;
	char	*convid=utf8ToClientCharset(id,idsize,&convidsize);
	if (convid) {
		id=convid;
		idsize=convidsize;
	}

	// the id length is a single byte, and a longer one couldn't have
	// arrived in the first place
	if (idsize>255) {
		idsize=255;
	}

	uint16_t	tokensize=(uint16_t)
				(sizeof(byte_t)*3+idsize);

	debugStart("pre-tds7 dynamic ack");
	debugTokenType(token);
	debugWrite("tokensize: %d",tokensize);
	debugWrite("id: %s",id);

	write(&resppacket,token);
	write(&resppacket,tokensize);
	write(&resppacket,(byte_t)TDS5_DYN_ACK);
	write(&resppacket,(byte_t)0);
	write(&resppacket,(byte_t)idsize);
	if (idsize) {
		write(&resppacket,(const byte_t *)id,idsize);
	}

	delete[] convid;

	debugEnd();
}

// Strips the "create proc <id> as " wrapper that a client puts in front
// of a dynamic prepare's statement.  Ct-lib writes one when the server's
// request capability mask sets bit 48, TDS_PROTO_DYNPROC, which
// capability() grants to any client that asks for it.
//
// On an ase that wrapper is real - the prepare becomes a stored
// procedure named for the id, and the dealloc drops it.  Here it can't
// be.  The backend may not be an ase at all, "create proc" is not
// portable, and a real stored procedure would outlive the session that
// asked for it if anything went wrong before the dealloc.
//
// So the wrapper is stripped rather than run, and rather than cleared
// out of the capability mask.  Clearing the bit would change what
// ct_capability() reports to the application, and it wouldn't be enough
// anyway - nothing stops a client from wrapping the statement without
// being asked, so both forms have to be handled either way.
//
// Returns a pointer into "stmt", or "stmt" itself when there's no
// wrapper to strip.  Only a wrapper naming this command's own id is
// stripped; anything else is somebody's real "create procedure" and gets
// prepared as it stands.
const char *sqlrprotocol_tds::preTds7DynamicStatement(const char *stmt,
							const char *id) {

	const char	*ptr=stmt;

	while (character::isWhitespace(*ptr)) {
		ptr++;
	}
	if (charstring::compareIgnoringCase(ptr,"create",6)) {
		return stmt;
	}
	ptr+=6;

	if (!character::isWhitespace(*ptr)) {
		return stmt;
	}
	while (character::isWhitespace(*ptr)) {
		ptr++;
	}
	if (!charstring::compareIgnoringCase(ptr,"procedure",9)) {
		ptr+=9;
	} else if (!charstring::compareIgnoringCase(ptr,"proc",4)) {
		ptr+=4;
	} else {
		return stmt;
	}

	if (!character::isWhitespace(*ptr)) {
		return stmt;
	}
	while (character::isWhitespace(*ptr)) {
		ptr++;
	}
	size_t	idlen=charstring::getLength(id);
	if (!idlen || charstring::compare(ptr,id,idlen)) {
		return stmt;
	}
	ptr+=idlen;

	if (!character::isWhitespace(*ptr)) {
		return stmt;
	}
	while (character::isWhitespace(*ptr)) {
		ptr++;
	}
	if (charstring::compareIgnoringCase(ptr,"as",2)) {
		return stmt;
	}
	ptr+=2;

	if (!character::isWhitespace(*ptr)) {
		return stmt;
	}
	while (character::isWhitespace(*ptr)) {
		ptr++;
	}

	return ptr;
}

// Handles one tds 5.0 dynamic sql token, appending its result to the
// response packet.  Returns false if the walk should stop, having
// already appended an error and a done.
//
// The token is:
//	uint16, little-endian	how much follows - everything below
//	byte			type - which operation this is
//	byte			status - TDS5_DYN_HASARGS when a
//				paramfmt/params pair follows
//	byte			id length
//	bytes			the id, as single-byte characters, not
//				nul terminated
//	uint16, little-endian	statement length
//	bytes			the statement, as single-byte characters
//
// Every operation sends that same layout.  The ones with no statement of
// their own send a zero length rather than leaving the field out, and
// exec-immediate sends a zero id length rather than an id.
//
// Dynamic2 (0xA3) gets no case of its own, for the same reason dbrpc2
// doesn't - no client was ever seen sending one and freetds has no code
// that can, so it stays with the tokens preTds7Normal() refuses.
bool sqlrprotocol_tds::preTds7Dynamic(const byte_t **rpinout,
					size_t *rpsizeinout) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	debugStart("pre-tds7 dynamic");

	// get the token length
	uint16_t	tokenlength=0;
	if (rpsize<sizeof(tokenlength)) {
		debugWrite("truncated token length");
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		preTds7ParamError("Malformed TDS 5.0 dynamic token",false);
		return false;
	}
	read(rp,&tokenlength,&rp);
	rpsize-=sizeof(tokenlength);

	debugWrite("token length: %d",tokenlength);

	// The length covers the type, status and id-length bytes and the
	// statement length, so a token without room for all four is
	// malformed, as is one that runs off the end of the buffer.
	if ((size_t)tokenlength<sizeof(byte_t)*3+sizeof(uint16_t) ||
					(size_t)tokenlength>rpsize) {
		debugWrite("invalid token length: %d",tokenlength);
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		preTds7ParamError("Malformed TDS 5.0 dynamic token",false);
		return false;
	}

	// everything below reads out of the token's own body, so remember
	// where that ends - an id or statement shorter than the length
	// allows for leaves bytes to step over
	const byte_t	*body=rp;
	size_t		bodyleft=tokenlength;

	// get the type and status
	byte_t	type=0;
	read(rp,&type,&rp);
	bodyleft-=sizeof(type);
	byte_t	status=0;
	read(rp,&status,&rp);
	bodyleft-=sizeof(status);

	debugWrite("type: 0x%02x",type);
	debugWrite("status: 0x%02x",status);

	// get the id length
	byte_t	idlen=0;
	read(rp,&idlen,&rp);
	bodyleft-=sizeof(idlen);

	debugWrite("id length: %d",idlen);

	// the statement length still has to fit behind the id
	if ((size_t)idlen+sizeof(uint16_t)>bodyleft) {
		debugWrite("invalid id length: %d",idlen);
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		preTds7ParamError("Malformed TDS 5.0 dynamic token",false);
		return false;
	}

	const byte_t	*id=rp;
	rp+=idlen;
	bodyleft-=idlen;

	// get the statement length
	uint16_t	stmtlen=0;
	read(rp,&stmtlen,&rp);
	bodyleft-=sizeof(stmtlen);

	debugWrite("statement length: %d",stmtlen);

	if ((size_t)stmtlen>bodyleft) {
		debugWrite("invalid statement length: %d",stmtlen);
		debugEnd();
		*rpinout=rp;
		*rpsizeinout=0;
		preTds7ParamError("Malformed TDS 5.0 dynamic token",false);
		return false;
	}

	const byte_t	*stmt=rp;

	// step over anything the token declared past the statement
	rp=body+tokenlength;
	rpsize-=tokenlength;

	// copy out what's been consumed, so that the walker sees this
	// token gone whichever way the rest of this goes
	*rpinout=rp;
	*rpsizeinout=rpsize;

	// An execute's values ride in the paramfmt/params pair behind the
	// token, and the count has to be reset either way - whatever ran
	// before this left its own count behind.
	rpcparamcount=0;
	if (status&TDS5_DYN_HASARGS) {

		if (!preTds7ParamFmtAndParams(&rp,&rpsize)) {
			debugEnd();
			*rpinout=rp;
			*rpsizeinout=0;
			return false;
		}

		// copy out what the pair consumed too
		*rpinout=rp;
		*rpsizeinout=rpsize;
	}

	// Whether another command follows this one in the buffer.  This has
	// to be worked out behind the paramfmt/params pair rather than in
	// front of it, for the reason preTds7Language() spells out - the
	// pair is part of this command, so counting it as "more" would put
	// DONE_MORE on the last command's done and leave ct-lib waiting for
	// a done that never comes.
	bool	more=(rpsize>0);

	// bounds checking.  a single check is enough here, the way it is in
	// preTds7Language() - passing single bytes through can't grow them.
	if ((size_t)stmtlen>maxquerysize) {
		debugWrite("query too large: %d",stmtlen);
		debugEnd();
		*rpsizeinout=0;
		stringbuffer	err;
		queryTooLargeMessage(stmtlen,&err);
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,err.getString(),srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
		return false;
	}

	size_t	idsize;
	char	*idstr=preTds7ToUtf8(id,idlen,&idsize);
	size_t	stmtsize;
	char	*stmtstr=preTds7ToUtf8(stmt,stmtlen,&stmtsize);

	debugWrite("id: %s",idstr);
	debugWrite("statement: %s",stmtstr);

	// Every operation is answered with an ack naming the same id, so
	// write it once here.  A failed operation gets one too - that's
	// what a real ase sends, with the error and the done behind it.
	preTds7DynamicAck(idstr,idsize);

	bool	retval=true;
	switch (type) {
		case TDS5_DYN_PREPARE:
			retval=preTds7DynamicPrepare(idstr,stmtstr,more);
			break;
		case TDS5_DYN_EXEC:
			retval=preTds7DynamicExecute(idstr,more);
			break;
		case TDS5_DYN_DEALLOC:
			retval=preTds7DynamicDealloc(idstr,more);
			break;
		case TDS5_DYN_EXEC_IMMEDIATE:
			retval=preTds7DynamicExecImmediate(stmtstr,
							stmtsize,more);
			break;
		case TDS5_DYN_DESCRIBE_INPUT:
		case TDS5_DYN_DESCRIBE_OUTPUT:
			preTds7DynamicDescribe(idstr,
					(type==TDS5_DYN_DESCRIBE_OUTPUT),
					more);
			break;
		default:
			// procname (0x10) and anything else.  There's nothing
			// to answer it with, so refuse just this command; the
			// token has already been stepped over, so the walk
			// can go on.
			debugWrite("unsupported dynamic type");
			preTds7DynamicError("This TDS 5.0 dynamic SQL "
						"operation is not supported "
						"yet.",more);
			break;
	}

	delete[] idstr;
	delete[] stmtstr;

	// A handler that gave up can't say where the request stream now
	// stands, so end the walk after its own done rather than leaving
	// DONE_MORE on it.
	if (!retval) {
		*rpsizeinout=0;
	}

	debugEnd();

	return retval;
}

// Prepares a dynamic sql statement under the id the client named it
// with.  The reply is the ack, the prepared statement's output column
// formats when the backend can describe them without running it, and a
// done.  Ct-lib caches those formats and answers
// ct_dynamic(CS_DESCRIBE_OUTPUT) out of them without going near the
// wire, which is why a describe is almost never seen here.
//
// No input parameter formats go with them.  A real ase sends those too,
// but the server API has no way to describe a statement's parameters
// without running it, and guessing would be worse than saying nothing -
// ct_dynamic(CS_DESCRIBE_INPUT) reports no parameters instead.
bool sqlrprotocol_tds::preTds7DynamicPrepare(const char *id,
						const char *stmt,
						bool more) {

	debugStart("pre-tds7 dynamic prepare");

	const char	*query=preTds7DynamicStatement(stmt,id);
	size_t		querylen=charstring::getLength(query);

	debugWrite("query: %s",query);

	// re-preparing a live id replaces what it named
	uint32_t	oldhandle=0;
	dynamicHandle(id,&oldhandle);

	sqlrservercursor	*cursor=NULL;
	uint32_t		handle=0;
	bool	success=prepareStatement(oldhandle,query,querylen,
						false,0,&cursor,&handle);

	if (!cursor) {
		debugWrite("no cursor available");
		debugEnd();
		removeDynamicHandle(id);
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,"No cursor available",srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
		return false;
	}

	if (!success) {
		debugEnd();
		removeDynamicHandle(id);
		appendQueryError(cursor);
		releaseCursor(cursor);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),
						transState(),0);
		return true;
	}

	setDynamicHandle(id,handle);

	debugWrite("prepared handle: %d",handle);

	// the cursor stays put - the execute will want it

	// A result set too wide for a rowfmt is refused in here, with its
	// own error and done, so don't send a second one.
	if (!preTds7RowFmt(cursor,more)) {
		debugEnd();
		return true;
	}

	done((more)?DONE_MORE:DONE_FINAL,transState(),0);

	debugEnd();

	return true;
}

// Runs a dynamic sql statement that was prepared under this id, with the
// values that rode in the paramfmt/params pair behind the token.
bool sqlrprotocol_tds::preTds7DynamicExecute(const char *id, bool more) {

	debugStart("pre-tds7 dynamic execute");

	uint32_t	handle=0;
	if (!dynamicHandle(id,&handle)) {
		debugWrite("no such statement");
		debugEnd();
		stringbuffer	err;
		err.append("Prepared statement ")->append(id);
		err.append(" does not exist.");
		preTds7DynamicError(err.getString(),more);
		return true;
	}

	debugWrite("prepared handle: %d",handle);

	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);

	// tds 5.0 has no in/out parameter direction - a paramfmt says input
	// or return, not both - and the cursor may have been left with some
	// by something that used it earlier
	cont->setInputOutputBindCount(cursor,0);
	if (!rpcparamcount) {
		cont->setInputBindCount(cursor,0);
		cont->setOutputBindCount(cursor,0);
	}

	// bind and run the prepared query
	if (!executeStatement(cursor,0)) {
		debugEnd();
		// DONE_ERROR is what turns this into a CS_CMD_FAIL - ct-lib
		// reports CS_CMD_SUCCEED for a done without it, and the
		// client's result walk would fall a result out of step
		appendQueryError(cursor);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),
						transState(),0);
		return true;
	}

	if (!cont->colCount(cursor)) {
		// DONE_COUNT is what makes the count valid - without it
		// ct_res_info(CS_ROW_COUNT) reports no count at all
		done(((more)?DONE_MORE:DONE_FINAL)|DONE_COUNT,
					transState(),
					cont->getAffectedRows(cursor));
		debugEnd();
		return true;
	}

	// One rowfmt/rows/done group per result set, the way
	// preTds7Language() sends them.  DONE_MORE on every one of them,
	// including the last: the command's own done follows behind.
	for (;;) {

		// A result set too wide for a rowfmt is refused in here,
		// with its own error and done, so don't send a second one.
		if (!preTds7RowFmt(cursor,true)) {
			debugEnd();
			return true;
		}

		uint64_t	rowcount=preTds7Rows(cursor);

		bool	avail=false;
		if (!cont->nextResultSet(cursor,&avail)) {
			appendQueryError(cursor);
			done(DONE_ERROR|DONE_COUNT|
				((more)?DONE_MORE:DONE_FINAL),
				transState(),rowcount);
			debugEnd();
			return true;
		}

		done(DONE_MORE|DONE_COUNT,transState(),rowcount);

		if (!avail) {
			break;
		}
	}

	// The execute's own done, behind the result sets it produced.  A
	// real ase sends this one too, and ct-lib reports it as an extra
	// CS_CMD_SUCCEED/CS_CMD_DONE pair after the rows - the same pair
	// sp_execute's done produces over ms-tds.
	done((more)?DONE_MORE:DONE_FINAL,transState(),0);

	debugEnd();

	return true;
}

// Drops a dynamic sql statement and the cursor it was holding.
//
// An id that was never prepared isn't an error.  Ct-lib refuses a
// dealloc of an id it doesn't know about before it ever reaches the
// wire, so one that gets here is an id whose prepare failed on this end,
// and the client is entitled to clean up after that.
bool sqlrprotocol_tds::preTds7DynamicDealloc(const char *id, bool more) {

	debugStart("pre-tds7 dynamic dealloc");

	uint32_t	handle=0;
	if (dynamicHandle(id,&handle)) {

		debugWrite("prepared handle: %d",handle);

		removeDynamicHandle(id);
		unprepareStatement(handle,handleCursor(&stmthandles,handle));

	} else {
		debugWrite("no such statement");
		removeDynamicHandle(id);
	}

	done((more)?DONE_MORE:DONE_FINAL,transState(),0);

	debugEnd();

	return true;
}

// Runs one statement immediately, without preparing it under an id.
//
// A real ase takes only statements that return no rows here and rejects
// anything else outright, and ct-lib sends no parameters with one, so
// the reply is just a done.  A statement that does return rows gets its
// result sets sent anyway rather than silently thrown away.
bool sqlrprotocol_tds::preTds7DynamicExecImmediate(const char *stmt,
							size_t stmtsize,
							bool more) {

	debugStart("pre-tds7 dynamic exec immediate");

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugWrite("no cursor available");
		debugEnd();
		// FIXME: is there a real error number/state/class for this?
		appendError(0,1,16,"No cursor available",srvname,NULL,1);
		done(DONE_ERROR|DONE_FINAL,transState(),0);
		return false;
	}

	// this statement has no parameters at all, so bind translation is
	// off, the way preTds7Language() turns it off for a language
	// command without any
	cont->setInputBindCount(cursor,0);
	cont->setOutputBindCount(cursor,0);
	cont->setInputOutputBindCount(cursor,0);
	cont->setTranslateBindVariablesForThisQuery(cursor,false);

	bool	success=cont->prepareQuery(cursor,stmt,(uint32_t)stmtsize,
							true,true,true,true) &&
			cont->executeQuery(cursor,true,true,true,true);

	if (!success) {
		appendQueryError(cursor);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),
						transState(),0);
		releaseCursor(cursor);
		debugEnd();
		return true;
	}

	if (!cont->colCount(cursor)) {
		done(((more)?DONE_MORE:DONE_FINAL)|DONE_COUNT,
					transState(),
					cont->getAffectedRows(cursor));
		releaseCursor(cursor);
		debugEnd();
		return true;
	}

	for (;;) {

		if (!preTds7RowFmt(cursor,more)) {
			break;
		}

		uint64_t	rowcount=preTds7Rows(cursor);

		bool	avail=false;
		if (!cont->nextResultSet(cursor,&avail)) {
			appendQueryError(cursor);
			done(DONE_ERROR|DONE_COUNT|
				((more)?DONE_MORE:DONE_FINAL),
				transState(),rowcount);
			break;
		}

		done(((avail || more)?DONE_MORE:DONE_FINAL)|DONE_COUNT,
						transState(),rowcount);

		if (!avail) {
			break;
		}
	}

	releaseCursor(cursor);

	debugEnd();

	return true;
}

// Describes a prepared dynamic sql statement's parameters or columns.
//
// Neither describe was ever seen on the wire - both sap's ct-lib and
// freetds answer ct_dynamic(CS_DESCRIBE_INPUT) and
// ct_dynamic(CS_DESCRIBE_OUTPUT) out of the formats that came back with
// the prepare - but nothing stops a client from asking, so both are
// answered rather than refused.
//
// The output describe sends the same rowfmt the prepare did.  The input
// describe sends no formats at all, for the same reason the prepare
// doesn't: there's no way to describe a statement's parameters without
// running it.
void sqlrprotocol_tds::preTds7DynamicDescribe(const char *id,
						bool output,
						bool more) {

	debugStart("pre-tds7 dynamic describe");

	uint32_t	handle=0;
	if (!dynamicHandle(id,&handle)) {
		debugWrite("no such statement");
		debugEnd();
		stringbuffer	err;
		err.append("Prepared statement ")->append(id);
		err.append(" does not exist.");
		preTds7DynamicError(err.getString(),more);
		return;
	}

	debugWrite("prepared handle: %d",handle);

	if (output) {
		// a result set too wide for a rowfmt is refused in here,
		// with its own error and done, so don't send a second one
		if (!preTds7RowFmt(handleCursor(&stmthandles,handle),more)) {
			debugEnd();
			return;
		}
	}

	done((more)?DONE_MORE:DONE_FINAL,transState(),0);

	debugEnd();
}

bool sqlrprotocol_tds::sqlBatch() {

	// recvPacket() takes this packet type whatever the session logged
	// in as, but the sql below is read as ucs-2 and the response is
	// sized by charSize(), which only gets the ms-tds shape when
	// pretds7 is clear.  a pre-tds7 session's single-byte query would
	// come out garbled, and the reply would be unreadable besides.
	if (pretds7) {
		debugStart("sql batch");
		debugWrite("pre-tds7 session");
		debugEnd();
		return sendTdsProtocolError();
	}

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		return sendNoCursorAvailableError();
	}

	const byte_t	*rp=reqpacket.getBuffer();
	size_t		rpsize=reqpacket.getSize();

	debugStart("sql batch");

	// get the headers
	if (negotiatedtdsversion>=720) {
		allHeaders(rp,rpsize,&rp,&rpsize);
	}

	// get the sql
	const ucs2_t	*sql=(const ucs2_t *)rp;
	size_t		sqllength=rpsize/sizeof(ucs2_t);

	// bounds checking
	if (sqllength>maxquerysize) {
		debugWrite("query too large: %lld",(long long)sqllength);
		debugEnd();
		releaseCursor(cursor);
		return sendQueryTooLargeError(sqllength);
	}

	// decode the query
	size_t	sql8size;
	char	*sql8=ucs2ToUtf8(sql,sqllength,&sql8size);

	debugWrite("sql: %s",sql8);
	debugWrite("sqllength: %lld",(long long)sql8size);
	debugEnd();

	// bounds checking again - utf-8 needs up to 3 bytes for a character
	// that took 2 on the wire, and prepareQuery silently truncates
	// rather than failing
	if (sql8size>maxquerysize) {
		delete[] sql8;
		releaseCursor(cursor);
		return sendQueryTooLargeError(sql8size);
	}

	// a bulk load opens with an "insert bulk" statement - passing it
	// through would put the backend's own connection into bulk mode
	if (insertBulk(sql8)) {

		delete[] sql8;
		releaseCursor(cursor);

		resppacket.clear();

		if (!bulktable) {
			appendError(0,1,16,"Malformed insert bulk statement",
							srvname,NULL,1);
			done(DONE_ERROR,0,0);
			return sendPacket();
		}

		// the client won't start sending bulk data unless this
		// done clears DONE_MORE
		done();
		return sendPacket();
	}

	// a batch has no bind variables, and the cursor may have been left
	// with some by an rpc that used it earlier.  @name in a batch is a
	// local variable or a parameter declaration, and @@name is a global.
	cont->setInputBindCount(cursor,0);
	cont->setOutputBindCount(cursor,0);
	cont->setInputOutputBindCount(cursor,0);
	cont->setTranslateBindVariablesForThisQuery(cursor,false);

	// find the exec statements, before the sql goes away
	uint16_t	execrs[BATCH_MAX_EXECS];
	uint16_t	execcount=batchExecResultSets(sql8,execrs,
							BATCH_MAX_EXECS);

	// run the query
	bool	success=
		cont->prepareQuery(cursor,sql8,(uint32_t)sql8size,
						true,true,true,true) &&
		cont->executeQuery(cursor,true,true,true,true);

	// clean up
	delete[] sql8;


	// begin building the response packet
	resppacket.clear();

	if (success) {

		// a batch can contain any number of statements, and the
		// client walks the result sets one at a time, so send one
		// result set per result set that the batch produced, with
		// DONE_MORE set on all but the last one
		uint16_t	rsindex=0;
		for (;;) {

			rsindex++;

			if (cont->colCount(cursor)) {
				colMetaData(cursor,false);
			}
			uint64_t	rowcount=rows(cursor);

			bool	avail=false;
			if (!cont->nextResultSet(cursor,&avail)) {
				appendQueryError(cursor);
				done(DONE_FINAL|DONE_COUNT,0,rowcount);
				break;
			}

			// how many exec return statuses follow this one
			uint16_t	pairs=0;
			for (uint16_t i=0; i<execcount; i++) {
				if (execrs[i]==rsindex) {
					pairs++;
				}
			}

			done((avail || pairs)?
				(DONE_MORE|DONE_COUNT):
				(DONE_FINAL|DONE_COUNT),0,rowcount);

			// odbc gives no way to see the return status of an
			// exec inside a batch, so a success status stands
			// in for the one clients expect.  freetds throws
			// away a return-status that anything but a done-proc
			// follows, so the two tokens have to stay adjacent.
			for (uint16_t i=0; i<pairs; i++) {
				returnStatus(RPC_STATUS_SUCCESS);
				doneProc((avail || i+1<pairs)?
						DONE_MORE:DONE_FINAL,0,0);
			}

			if (!avail) {
				break;
			}
		}

	} else {
		appendQueryError(cursor);
		// A real server closes a rejected batch with DONE_ERROR, which
		// the ct-lib client turns into CS_CMD_FAIL.  Only an ASE back
		// end gets the bit for now - an mssql back end reaches this
		// same branch for a batch that ought to have succeeded (e.g. a
		// "use db" batch rejected as a syntax error), and passing that
		// on would change the mssql baseline for a reason that has
		// nothing to do with this token.
		done((dbisase)?DONE_ERROR:DONE_FINAL,0,0);
	}

	// send the response packet
	bool	retval=sendPacket();

	// release the cursor
	// FIXME: kludgy
	releaseCursor(cursor);

	return retval;
}

bool sqlrprotocol_tds::isSqlWordChar(char ch) {
	return (character::isAlphanumeric(ch) ||
			ch=='_' || ch=='@' || ch=='#' || ch=='$');
}

uint16_t sqlrprotocol_tds::batchExecResultSets(const char *sql,
						uint16_t *rsindex,
						uint16_t maxcount) {

	debugStart("batch exec result sets");
	debugWrite("sql: %s",sql);

	uint16_t	count=0;
	uint16_t	rscount=0;
	uint32_t	depth=0;

	for (const char *ptr=sql; *ptr;) {

		// skip string literals and double-quoted identifiers,
		// either of which doubles its quote to embed one
		if (*ptr=='\'' || *ptr=='"') {
			char	quote=*ptr;
			ptr++;
			while (*ptr) {
				if (*ptr==quote) {
					ptr++;
					if (*ptr!=quote) {
						break;
					}
				}
				ptr++;
			}
			continue;
		}

		// skip bracket-quoted identifiers
		if (*ptr=='[') {
			ptr++;
			while (*ptr) {
				if (*ptr==']') {
					ptr++;
					if (*ptr!=']') {
						break;
					}
				}
				ptr++;
			}
			continue;
		}

		// skip line comments
		if (*ptr=='-' && ptr[1]=='-') {
			ptr+=2;
			while (*ptr && *ptr!='\n') {
				ptr++;
			}
			continue;
		}

		// skip block comments, which nest in t-sql
		if (*ptr=='/' && ptr[1]=='*') {
			uint32_t	commentdepth=1;
			ptr+=2;
			while (*ptr && commentdepth) {
				if (*ptr=='/' && ptr[1]=='*') {
					commentdepth++;
					ptr+=2;
				} else if (*ptr=='*' && ptr[1]=='/') {
					commentdepth--;
					ptr+=2;
				} else {
					ptr++;
				}
			}
			continue;
		}

		// track parenthesis depth
		if (*ptr=='(') {
			depth++;
			ptr++;
			continue;
		}
		if (*ptr==')') {
			if (depth) {
				depth--;
			}
			ptr++;
			continue;
		}

		if (!isSqlWordChar(*ptr)) {
			ptr++;
			continue;
		}

		// get the next word
		const char	*word=ptr;
		while (isSqlWordChar(*ptr)) {
			ptr++;
		}
		size_t		wordlen=ptr-word;

		// only top-level keywords matter, and a word that leads
		// with a digit or a sigil is a number, a variable, or a
		// temp table rather than a keyword
		if (depth || character::isDigit(*word) ||
				*word=='@' || *word=='#' || *word=='$') {
			continue;
		}

		if ((wordlen==4 &&
			!charstring::compareIgnoringCase(word,"exec",4)) ||
			(wordlen==7 &&
			!charstring::compareIgnoringCase(word,"execute",7))) {

			// "execute as ..." switches context, it doesn't
			// call anything, and it makes no result set
			const char	*next=
				cont->skipWhitespaceAndComments(ptr);
			if (!charstring::compareIgnoringCase(next,"as",2) &&
					!isSqlWordChar(next[2])) {
				continue;
			}

			if (count<maxcount) {
				rsindex[count]=rscount+1;
				count++;
			}
			rscount++;
			continue;
		}

		// the other statements that make a result set of their own
		if ((wordlen==6 &&
			(!charstring::compareIgnoringCase(word,"select",6) ||
			!charstring::compareIgnoringCase(word,"insert",6) ||
			!charstring::compareIgnoringCase(word,"update",6) ||
			!charstring::compareIgnoringCase(word,"delete",6))) ||
			(wordlen==5 &&
			!charstring::compareIgnoringCase(word,"merge",5))) {
			rscount++;
		}
	}

	for (uint16_t i=0; i<count; i++) {
		debugWrite("exec result set index: %d",rsindex[i]);
	}
	debugWrite("count: %d",count);
	debugEnd();

	return count;
}

void sqlrprotocol_tds::allHeaders(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout) {

	debugStart("all headers");

	// skip the headers entirely if the packet is too short to hold
	// even the size of them
	uint32_t	allheaderssize;
	if (rpsize<sizeof(allheaderssize)) {
		debugWrite("truncated all-headers size");
		debugEnd();
		*rpout=rp;
		if (rpsizeout) {
			*rpsizeout=rpsize;
		}
		return;
	}

	// get the size of all headers
	readLE(rp,&allheaderssize,&rp);

	debugWrite("all-headers size: %d",allheaderssize);

	// skip the headers entirely if that size is bogus
	if (allheaderssize<sizeof(allheaderssize) || allheaderssize>rpsize) {
		debugWrite("invalid all-headers size: %d",allheaderssize);
		allheaderssize=sizeof(allheaderssize);
	}

	// decrement remaining sizes
	allheaderssize-=sizeof(allheaderssize);
	rpsize-=sizeof(allheaderssize);

	while (allheaderssize) {

		// get header size and type
		const byte_t	*headerstart=rp;
		uint32_t	headersize;
		uint16_t	headertype;

		// the clamp above keeps allheaderssize no larger than
		// rpsize, and both drop by the same amount each pass, so
		// this keeps the two reads below inside the packet too
		if (allheaderssize<sizeof(headersize)+sizeof(headertype)) {
			debugWrite("truncated header");
			break;
		}

		readLE(rp,&headersize,&rp);
		readLE(rp,&headertype,&rp);

		debugWrite("header size: %d",headersize);
		debugAllHeadersType(headertype);

		// bail on a bogus header size, otherwise we'd loop forever
		if (headersize<sizeof(headersize)+sizeof(headertype) ||
					headersize>allheaderssize) {
			debugWrite("invalid header size: %d",headersize);
			// rpsize is only decremented at the bottom of the
			// loop, so put rp back where this header started
			// rather than 6 bytes ahead of the size handed back
			rp=headerstart;
			break;
		}

		// what's left of this header, after its size and type
		size_t	hsize=headersize-
				(sizeof(headersize)+sizeof(headertype));

		switch (headertype) {
			case ALL_HEADERS_QUERY_NOTIFICATIONS:
				{
				// the sizes on the wire are in bytes,
				// but read() wants characters
				uint16_t	notifyidsize;
				uint16_t	notifyidlength;
				ucs2_t		*notifyid;
				uint16_t	ssbdeploymentsize;
				uint16_t	ssbdeploymentlength;
				ucs2_t		*ssbdeployment;
				uint32_t	notifytimeout;

				if (hsize<sizeof(notifyidsize)) {
					debugWrite("truncated notify id size");
					break;
				}
				readLE(rp,&notifyidsize,&rp);
				hsize-=sizeof(notifyidsize);

				if (hsize<notifyidsize) {
					debugWrite("truncated notify id");
					break;
				}
				notifyidlength=notifyidsize/sizeof(ucs2_t);
				notifyid=new ucs2_t[notifyidlength];
				read(rp,notifyid,notifyidlength,&rp);
				hsize-=notifyidsize;

				if (hsize<sizeof(ssbdeploymentsize)) {
					debugWrite("truncated ssb "
						"deployment size");
					delete[] notifyid;
					break;
				}
				readLE(rp,&ssbdeploymentsize,&rp);
				hsize-=sizeof(ssbdeploymentsize);

				if (hsize<ssbdeploymentsize) {
					debugWrite("truncated ssb deployment");
					delete[] notifyid;
					break;
				}
				ssbdeploymentlength=
					ssbdeploymentsize/sizeof(ucs2_t);
				ssbdeployment=new ucs2_t[ssbdeploymentlength];
				read(rp,ssbdeployment,
						ssbdeploymentlength,&rp);
				hsize-=ssbdeploymentsize;

				if (hsize>=sizeof(notifytimeout)) {
					readLE(rp,&notifytimeout,&rp);
				}

				// FIXME: do something useful with this info

				delete[] notifyid;
				delete[] ssbdeployment;
				}
				break;

			case ALL_HEADERS_TRANSACTION_DESCRIPTOR:
				{
				uint64_t	transactiondescriptor;
				uint32_t	outstandingrequestcount;
				if (hsize<sizeof(transactiondescriptor)+
					sizeof(outstandingrequestcount)) {
					debugWrite("truncated transaction "
							"descriptor");
					break;
				}
				readLE(rp,&transactiondescriptor,&rp);
				readLE(rp,&outstandingrequestcount,&rp);
				// FIXME: do something useful with this info
				}
				break;

			case ALL_HEADERS_TRACE_ACTIVITY:
				{
				byte_t	activityid[20];
				if (hsize<sizeof(activityid)) {
					debugWrite("truncated activity id");
					break;
				}
				read(rp,activityid,sizeof(activityid),&rp);
				// FIXME: do something useful with this info
				}
				break;

		}

		// skip whatever part of the header wasn't parsed
		rp=headerstart+headersize;

		// decrement remaining sizes
		allheaderssize-=headersize;
		rpsize-=headersize;
	}

	// copy out pointer and size
	*rpout=rp;
	if (rpsizeout) {
		*rpsizeout=rpsize;
	}

	debugEnd();
}

void sqlrprotocol_tds::colMetaData(sqlrservercursor *cursor, bool nometadata) {

	// get col count and bail if there are no columns
	uint16_t	count=cont->colCount(cursor);
	if (!count) {
		return;
	}

	byte_t	token=TOKEN_COLMETADATA;

	write(&resppacket,token);

	debugStart("col meta data");
	debugTokenType(token);

	// count doubles as the "no metadata" signal - 0xFFFF alone, with
	// nothing else in the token, rather than the real count followed by
	// 0xFFFF.  a client that set RPC_NO_META_DATA (freetds does, on every
	// sp_cursorfetch) reads count first and stops there if it's 0xFFFF,
	// so sending the real count ahead of it misreads everything after.
	if (nometadata) {
		writeLE(&resppacket,(uint16_t)0xFFFF);
		debugWrite("no metadata");
	} else {
		writeLE(&resppacket,count);
		debugWrite("count: %d",count);
		cekTable();
		for (uint16_t col=0; col<count; col++) {
			colData(cursor,col);
		}
	}

	debugEnd();
}

// The tds 5.0 counterpart of colMetaData(), and a sibling of it rather
// than a branch inside it.  The token byte can't be shared - 0x81 is a
// cursor-delete request in tds 5.0 - and nothing after it is laid out
// the same way either: the length and column count come first, the
// names are single-byte characters, the flags are one byte with
// different bits, the usertype is always 4 bytes, and there's no
// collation anywhere.
//
//	byte			0xEE
//	uint16, little-endian	how much follows, counting the column
//				count as well as the column blocks
//	uint16, little-endian	column count
//	then per column:
//		byte		name length
//		bytes		the name, as single-byte characters
//		byte		flags
//		int32, LE	usertype
//		byte		datatype
//		...		size/precision/scale, keyed off the
//				datatype's varint class
//		byte		locale length (then that many bytes)
//
// "more" says whether another command follows this one in the request
// buffer, and only matters on the refusal paths - it decides whether
// their done gets DONE_MORE, the way every other refusal in this module
// does.  A refusal here doesn't stop the token walk, so a DONE_FINAL
// would leave the dones of those later commands unread in the socket.
bool sqlrprotocol_tds::preTds7RowFmt(sqlrservercursor *cursor, bool more) {

	// get col count and bail if there are no columns
	uint32_t	count=cont->colCount(cursor);
	if (!count) {
		return true;
	}

	byte_t	token=TOKEN_ROWFMT;

	debugStart("pre-tds7 row fmt");
	debugTokenType(token);

	// The column count is 16 bits wide here.  A wider result set needs
	// rowfmt2 (0x61), which this module doesn't write, so refuse the
	// whole thing rather than send a count that the blocks after it
	// don't match.  Class 16 for the same reason
	// preTds7UnsupportedToken() uses it - the session stays usable.
	if (count>65535) {
		debugWrite("too many columns: %d",count);
		debugEnd();
		// FIXME: is there a real error number/state for this?
		appendError(0,1,16,"Result sets with more than 65535 "
					"columns are not supported yet.",
					srvname,NULL,1);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),transState(),0);
		return false;
	}

	debugWrite("count: %d",count);

	// The token length counts bytes that aren't written yet, and every
	// column block is a different size, so build the blocks into a
	// scratch buffer and measure them.  The alternative - a second pass
	// that adds up what the writers below are going to produce - is a
	// parallel copy of the same size rules that can silently drift out
	// of step with them, and a rowfmt whose length is off by even one
	// byte desynchronizes the rest of the stream.  Building first also
	// means an oversized result set is caught before anything lands in
	// the response.
	bytebuffer	cols;

	for (uint32_t i=0; i<count; i++) {

		uint16_t	col=(uint16_t)i;

		debugStart("pre-tds7 col %d",col);

		// name.  writeVarchar() writes single-byte characters when
		// pretds7 is set, which is what tds 5.0 wants - colName()
		// always writes ucs-2 - so the name is converted to the
		// charset the login record declared first, before the
		// truncation below counts its bytes.  The length is a single
		// byte, so a longer name is truncated, the way
		// appendInfoOrError() truncates the names it sends; losing
		// the whole result set over a long column name would be
		// worse.
		size_t		namelen=cont->getColumnNameSize(cursor,col);
		const char	*name=cont->getColumnName(cursor,col);
		size_t		convnamelen=0;
		char		*convname=utf8ToClientCharset(
						name,namelen,&convnamelen);
		if (convname) {
			name=convname;
			namelen=convnamelen;
		}
		if (namelen>255) {
			namelen=255;
		}
		writeVarchar(&cols,sizeof(byte_t),name,namelen);
		debugWrite("namelen: %lld",(long long)namelen);
		debugWrite("name: %s",name);
		delete[] convname;

		// flags
		preTds7ColFlags(&cols,cursor,col);

		// usertype.  Always 4 bytes, so userType() can't be reused -
		// it writes 2 when the negotiated version is under 720, and
		// a pre-tds7 session negotiates 500.
		//
		// 0 means "no alias type", the same thing userType() sends,
		// and the client reports it as such.
		uint32_t	usertype=0;
		write(&cols,usertype);
		debugWrite("usertype: %d",usertype);

		// datatype
		uint16_t	coltype=cont->getColumnType(cursor,col);
		byte_t		tds5type=mapType(coltype);
		write(&cols,tds5type);

		// size/precision/scale
		preTds7TypeInfo(&cols,cursor,col,coltype,tds5type);

		// locale.  Mandatory even when it's empty - the client reads
		// it right after the type info, so leaving it out
		// desynchronizes everything after this column.
		write(&cols,(byte_t)0);
		debugWrite("locale length: 0");

		debugEnd();
	}

	// the length covers the column count too, not just the blocks
	size_t	tokenlength=sizeof(uint16_t)+cols.getSize();

	// Metadata too wide for the 16-bit length needs rowfmt2 (0x61),
	// whose length field is 32 bits.  Refuse rather than truncate - a
	// truncated rowfmt isn't a smaller result set, it's a stream the
	// client can't parse at all.
	if (tokenlength>65535) {
		debugWrite("token too large: %lld",(long long)tokenlength);
		debugEnd();
		// FIXME: is there a real error number/state for this?
		appendError(0,1,16,"Result set metadata larger than 65535 "
					"bytes is not supported yet.",
					srvname,NULL,1);
		done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),transState(),0);
		return false;
	}

	write(&resppacket,token);
	write(&resppacket,(uint16_t)tokenlength);
	write(&resppacket,(uint16_t)count);
	write(&resppacket,cols.getBuffer(),cols.getSize());

	debugWrite("token length: %lld",(long long)tokenlength);
	debugEnd();

	return true;
}

void sqlrprotocol_tds::cekTable() {

	debugStart("cek table");
	debugEnd();

	if (negotiatedtdsversion<730) {
		return;
	}

	// FIXME: The client doesn't seem to care that this isn't
	// being sent.  How do we decide when to send it?

	// FIXME: actually implement this.  A cek table is a count of
	// encryption key values, then per value a database id, cek id,
	// cek version, and cek metadata version, then a count of
	// encryption keys, then per key an encrypted key, key store
	// name, key path, and asymmetric algorithm.
}

byte_t sqlrprotocol_tds::mapType(uint16_t type) {

	// Some protocol versions don't support some types.  If the server
	// returned a type not supported by the protocol, then map it to a
	// type that is.

	// FIXME: just use multiple type maps instead of the switch/ifs...

	debugStart("map type");
	debugWrite("type: %hd",type);

	// bail on a type that the map doesn't cover
	// (0x1F is TDS_TYPE_NULL in ms-tds and TDS5_TYPE_VOID in tds 5.0,
	// so this is a legal answer in either dialect)
	if (type>=sizeof(tdstypemap)/sizeof(tdstypemap[0])) {
		debugWrite("invalid column type: %hd",type);
		debugEnd();
		return TDS_TYPE_NULL;
	}

	// Tds 5.0 has its own map, and none of the version downgrades below
	// apply to it.  They rewrite types to TDS_TYPE_NVARCHAR, which isn't
	// a datatype in tds 5.0 at all, and a pre-tds7 session negotiates
	// version 500, so every one of them would fire.  The date/time to
	// varchar downgrade is still wanted here - it just comes out of
	// pretds7typemap[] rather than out of a version guard.
	if (pretds7) {
		byte_t	tds5type=pretds7typemap[type];
		debugPreTds7ColumnType(tds5type);
		debugEnd();
		return tds5type;
	}

	byte_t	tdstype=tdstypemap[type];
	if (negotiatedtdsversion<730) {
		switch (tdstype) {
			case TDS_TYPE_DATEN:
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				// a real sql server doesn't downgrade these
				// to an older date/time type for an older
				// client - it sends nvarchar in the iso/odbc
				// rendering, which is what the backends hand
				// us anyway
				tdstype=TDS_TYPE_NVARCHAR;
				break;
		}
	}
	if (negotiatedtdsversion<720) {
		switch (tdstype) {
			case TDS_TYPE_XML:
				tdstype=TDS_TYPE_TEXT;
				break;
			case TDS_TYPE_UDT:
				// FIXME: do something...
				break;
			case TDS_TYPE_SSVARIANT:
				// FIXME: do something...
				break;
			case TDS_TYPE_TVP:
				// FIXME: do something...
				break;
		}
	}

	debugColumnType(tdstype);
	debugEnd();

	return tdstype;
}

// How wide a tds 5.0 datatype's length field is - its "varint class".
// It decides both the size field in a rowfmt and the length prefix in
// front of the value in a row:
//
//	0 - no length at all.  The type carries its own width, and there's
//	    no way to encode a null.
//	1 - a single byte.  The default, and a length of 0 means null.
//	4 - a 4-byte length, for the blob types.
//	5 - a 4-byte length, for longbinary and longchar.
//
// The membership is freetds's tds_get_varint_size(), restricted to its
// tds 5.0 branch.  Note that 0x7F isn't in the fixed set here the way it
// is in ms-tds - the tds 5.0 8-byte integer is TDS5_TYPE_INT8 (0xBF).
byte_t sqlrprotocol_tds::preTds7VarintSize(byte_t tds5type) {

	switch (tds5type) {
		case TDS5_TYPE_VOID:
		case TDS5_TYPE_INTERVAL:
		case TDS5_TYPE_INT1:
		case TDS5_TYPE_DATE:
		case TDS5_TYPE_BIT:
		case TDS5_TYPE_TIME:
		case TDS5_TYPE_INT2:
		case TDS5_TYPE_INT4:
		case TDS5_TYPE_SHORTDATE:
		case TDS5_TYPE_FLT4:
		case TDS5_TYPE_MONEY:
		case TDS5_TYPE_DATETIME:
		case TDS5_TYPE_FLT8:
		case TDS5_TYPE_UINT1:
		case TDS5_TYPE_UINT2:
		case TDS5_TYPE_UINT4:
		case TDS5_TYPE_UINT8:
		case TDS5_TYPE_SHORTMONEY:
		case TDS5_TYPE_SINT1:
		case TDS5_TYPE_INT8:
			return 0;
		case TDS5_TYPE_IMAGE:
		case TDS5_TYPE_TEXT:
		case TDS5_TYPE_XML:
		case TDS5_TYPE_UNITEXT:
			return 4;
		case TDS5_TYPE_LONGCHAR:
		case TDS5_TYPE_LONGBINARY:
			return 5;
		default:
			return 1;
	}
}

// How wide a varint-0 type's value is.  Nothing but the value goes on
// the wire for one of these - there's no length field in front of it -
// so this is also how many bytes a null has to be padded out to, since
// a fixed type has no way to say "null" at all.
byte_t sqlrprotocol_tds::preTds7FixedSize(byte_t tds5type) {

	switch (tds5type) {
		case TDS5_TYPE_INT1:
		case TDS5_TYPE_BIT:
		case TDS5_TYPE_UINT1:
		case TDS5_TYPE_SINT1:
			return 1;
		case TDS5_TYPE_INT2:
		case TDS5_TYPE_UINT2:
			return 2;
		case TDS5_TYPE_INT4:
		case TDS5_TYPE_UINT4:
		case TDS5_TYPE_DATE:
		case TDS5_TYPE_TIME:
		case TDS5_TYPE_SHORTDATE:
		case TDS5_TYPE_SHORTMONEY:
		case TDS5_TYPE_FLT4:
			return 4;
		case TDS5_TYPE_INT8:
		case TDS5_TYPE_UINT8:
		case TDS5_TYPE_INTERVAL:
		case TDS5_TYPE_MONEY:
		case TDS5_TYPE_DATETIME:
		case TDS5_TYPE_FLT8:
			return 8;
		default:
			// void, and everything that isn't varint 0 at all
			return 0;
	}
}

void sqlrprotocol_tds::colData(sqlrservercursor *cursor, uint16_t col) {

	debugStart("col %d",col);

	uint16_t	coltype=cont->getColumnType(cursor,col);
	byte_t		tdstype=mapType(coltype);

	userType(tdstype);
	colFlags(cursor,col,tdstype);
	typeInfo(cursor,col,coltype,tdstype);
	tableName(tdstype);
	cryptoMetaData();
	colName(cursor,col);

	debugEnd();
}

void sqlrprotocol_tds::userType(byte_t tdstype) {

	debugStart("user type");
	debugColumnType(tdstype);

	uint32_t	usertype=0;

	// * = 0x0000 by default
	// * = 0x0050 for a timestamp/rowversion column - that's an
	//   8-byte binary value, not a date/time value, and the
	//   backends don't tell us which binary columns are
	//   timestamps, so we can't tag it (FIXME)
	// * > 0x00FF for alias types (FIXME: how to identify these?)

	if (negotiatedtdsversion<720) {
		writeLE(&resppacket,(uint16_t)usertype);
	} else {
		writeLE(&resppacket,usertype);
	}

	debugWrite("usertype: %d",usertype);
	debugEnd();
}

void sqlrprotocol_tds::colFlags(sqlrservercursor *cursor,
						uint16_t col,
						byte_t tdstype) {

	debugStart("col flags");
	debugColumnType(tdstype);

	uint16_t	flags=0;

	// is nullable
	flags|=((cont->getColumnIsNullable(cursor,col))?0x0001:0);

	// case-sensitive
	flags|=((isCaseSensitiveType(tdstype))?(0x0001<<1):0);

	// updateable (0 = readonly, 1 = read/write, 2 = unknown) (FIXME)
	flags|=((true)?(0x0002<<2):0);

	// identity
	flags|=((cont->getColumnIsAutoIncrement(cursor,col))?(0x0001<<4):0);

	if (negotiatedtdsversion>=720) {

		// computed (FIXME)
		flags|=((false)?(0x0001<<5):0);

		// reserved ODBC
		flags|=((false)?(0x0003<<6):0);

		// fixed size clr type (FIXME)
		flags|=((false)?(0x0001<<8):0);

		if (negotiatedtdsversion>=740) {

			// sparse column set (FIXME)
			flags|=((false)?(0x0001<<9):0);

			// encrypted (FIXME)
			flags|=((false)?(0x0001<<10):0);

			// this bit is reserved

			// hidden (FIXME)
			flags|=((false)?(0x0001<<12):0);

			// key in select...for browse (FIXME)
			flags|=((false)?(0x0001<<13):0);

			// nullable unknown (FIXME)
			flags|=((false)?(0x0001<<14):0);

			// this bit is reserved
		}
	}
	writeLE(&resppacket,flags);

	if (getDebug()) {
		stringbuffer	b;
		b.printBits(flags);
		debugWrite("flags: %s",b.getString());
	}
	debugEnd();
}

// The tds 5.0 counterpart of colFlags().  One byte rather than two, and
// the bits mean different things, so the two can't share an
// implementation.
void sqlrprotocol_tds::preTds7ColFlags(bytebuffer *buffer,
						sqlrservercursor *cursor,
						uint16_t col) {

	debugStart("pre-tds7 col flags");

	byte_t	flags=0;

	// hidden - a column the client didn't ask for, added to a "for
	// browse" select to key the rows.  Nothing here produces one.

	// key - part of that same key.  Likewise.

	// updateable (FIXME: the backends don't tell us, and colFlags()
	// assumes the same thing)
	flags|=((true)?TDS5_COLFLAG_WRITEABLE:0);

	// is nullable
	flags|=((cont->getColumnIsNullable(cursor,col))?
					TDS5_COLFLAG_NULLABLE:0);

	// identity
	flags|=((cont->getColumnIsAutoIncrement(cursor,col))?
					TDS5_COLFLAG_IDENTITY:0);

	write(buffer,flags);

	if (getDebug()) {
		stringbuffer	b;
		b.printBits(flags);
		debugWrite("flags: %s",b.getString());
	}
	debugEnd();
}

void sqlrprotocol_tds::typeInfo(sqlrservercursor *cursor,
						uint16_t col,
						uint16_t coltype,
						byte_t tdstype) {

	debugStart("type info");

	write(&resppacket,tdstype);

	debugColumnType(tdstype);

	// XMLTYPE_INFO (MS-TDS 2.2.5.4.3) is just a SchemaPresent byte, never
	// the size/collation a varlentype gets, so this bypasses the
	// isVarLenType()/isPartLenType() chain below rather than relying on
	// it - XML is in both of those lists, and isVarLenType() is checked
	// first, so without this XML goes out exactly like ntext, with no
	// XMLTYPE_INFO at all
	if (tdstype==TDS_TYPE_XML) {

		debugWrite("xmltype...");

		// SchemaPresent: no schema collection bound
		write(&resppacket,(byte_t)0x00);

	} else if (isFixedLenType(tdstype)) {

		debugWrite("fixedlentype...");

	} else if (isVarLenType(tdstype)) {

		debugWrite("varlentype...");

		uint32_t size=cont->getColumnSize(cursor,col);
		uint32_t precision=cont->getColumnPrecision(cursor,col);
		uint32_t scale=cont->getColumnScale(cursor,col);

		// size
		switch (tdstype) {
			case TDS_TYPE_SSVARIANT:
			case TDS_TYPE_TEXT:
			case TDS_TYPE_IMAGE:
				// limit the size to 2^31-1 because the
				// client will interpret it as signed
				if (size>2147483647) {
					size=2147483647;
				}
				writeLE(&resppacket,size);
				debugWrite("size: %d (32-bit)",size);
				break;
			case TDS_TYPE_NTEXT:
				// the size must be sent in bytes, but the
				// backend reports it in characters
				if (size>1073741823) {
					size=1073741823;
				}
				size*=sizeof(ucs2_t);
				writeLE(&resppacket,size);
				debugWrite("size: %d (32-bit)",size);
				break;
			case TDS_TYPE_BIGCHAR:
			case TDS_TYPE_BIGVARCHR:
			case TDS_TYPE_BIGBINARY:
			case TDS_TYPE_BIGVARBIN:
				// limit the size to 2^15-1 because the
				// client will interpret it as signed
				if (size>32767) {
					size=32767;
				}
				writeLE(&resppacket,(uint16_t)size);
				debugWrite("size: %d (16-bit)",size);
				break;
			case TDS_TYPE_NCHAR:
			case TDS_TYPE_NVARCHAR:
				// A date/time column downgraded to nvarchar
				// reports its binary-form size (e.g. 4 bytes
				// for an ase date), so measure it by type
				// instead or the client truncates it.
				size=dateTimeStringSize(coltype,size);
				// the size must be sent in bytes, but the
				// backend reports it in characters
				if (size>16383) {
					size=16383;
				}
				size*=sizeof(ucs2_t);
				writeLE(&resppacket,(uint16_t)size);
				debugWrite("size: %d (16-bit)",size);
				break;
			case TDS_TYPE_GUID:
				// a guid is always 16 bytes on the wire -
				// the back end reports 36, the width of
				// its printed form
				write(&resppacket,(byte_t)16);
				debugWrite("size: 16 (8-bit)");
				break;
			case TDS_TYPE_DATEN:
				// don't actually send a size for this type
				break;
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				// don't actually send a size for these types,
				// we'll send a scale below instead
				break;
			case TDS_TYPE_INTN:
			case TDS_TYPE_BITN:
			case TDS_TYPE_FLTN:
			case TDS_TYPE_MONEYN:
			case TDS_TYPE_DATETIMN:
				// these only allow certain sizes
				size=nTypeSize(coltype,tdstype,size);
				write(&resppacket,(byte_t)size);
				debugWrite("size: %d (8-bit)",size);
				break;
			case TDS_TYPE_NUMERIC:
			case TDS_TYPE_NUMERICN:
			case TDS_TYPE_DECIMAL:
			case TDS_TYPE_DECIMALN:
				// the size is the widest the value can be on
				// the wire, not the precision, and real
				// servers just send the max
				size=TDS_DECIMAL_MAX_SIZE;
				write(&resppacket,(byte_t)size);
				debugWrite("size: %d (8-bit)",size);
				break;
			default:
				// limit the size to 2^7-1 because the
				// client will interpret it as signed
				if (size>127) {
					size=127;
				}
				write(&resppacket,(byte_t)size);
				debugWrite("size: %d (8-bit)",size);
				break;
		}

		// collation
		if (negotiatedtdsversion>=710) {
			switch (tdstype) {
				case TDS_TYPE_BIGCHAR:
				case TDS_TYPE_BIGVARCHR:
				case TDS_TYPE_TEXT:
				case TDS_TYPE_NTEXT:
				case TDS_TYPE_NCHAR:
				case TDS_TYPE_NVARCHAR:
					writeCollation();
					break;
			}
		}

		// precision
		switch (tdstype) {
			case TDS_TYPE_NUMERIC:
			case TDS_TYPE_NUMERICN:
			case TDS_TYPE_DECIMAL:
			case TDS_TYPE_DECIMALN:
				write(&resppacket,(byte_t)precision);
				debugWrite("precision: %d",precision);
				break;
		}

		// scale
		switch (tdstype) {
			case TDS_TYPE_NUMERIC:
			case TDS_TYPE_NUMERICN:
			case TDS_TYPE_DECIMAL:
			case TDS_TYPE_DECIMALN:
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				write(&resppacket,(byte_t)scale);
				debugWrite("scale: %d",scale);
				break;
		}

	} else if (isPartLenType(tdstype)) {

		debugWrite("partlentype...");

		// FIXME: [ushortmaxlen] [collation] [xml_info] [utd_info]
	}

	debugEnd();
}

// The tds 5.0 counterpart of the size/collation/precision/scale part of
// typeInfo().  A separate function rather than another branch in that
// one: typeInfo() is on the hot ms-tds path, and almost nothing it does
// carries over.  What's sent here is decided by the datatype's varint
// class rather than by an isVarLenType()/isPartLenType() chain, the
// sizes have different widths and different limits, and there's no
// collation in tds 5.0 at all.
void sqlrprotocol_tds::preTds7TypeInfo(bytebuffer *buffer,
						sqlrservercursor *cursor,
						uint16_t col,
						uint16_t coltype,
						byte_t tds5type) {

	debugStart("pre-tds7 type info");
	debugPreTds7ColumnType(tds5type);

	// Decimal and numeric carry a precision and a scale after their
	// size, and the size is how wide that precision makes the value on
	// the wire, not the column's declared width.
	if (tds5type==TDS5_TYPE_DECN || tds5type==TDS5_TYPE_NUMN) {

		byte_t	precision;
		byte_t	scale;
		preTds7DecimalInfo(cursor,col,&precision,&scale);

		byte_t	wiresize=decimalSize(precision);
		write(buffer,wiresize);
		write(buffer,precision);
		write(buffer,scale);

		debugWrite("size: %d (8-bit)",wiresize);
		debugWrite("precision: %d",precision);
		debugWrite("scale: %d",scale);
		debugEnd();
		return;
	}

	uint32_t	size=preTds7DeclaredSize(cursor,col,coltype,tds5type);

	switch (preTds7VarintSize(tds5type)) {

		case 0:
			// the type carries its own width, so there's no size
			// field at all
			debugWrite("fixed, no size");
			break;

		case 4:
			write(buffer,size);
			debugWrite("size: %d (32-bit)",size);

			// Only the blob types carry a table name, and
			// writing one for anything else desynchronizes the
			// stream.  Xml and unitext are varint 4 but aren't
			// blob types, so they take none.
			if (tds5type==TDS5_TYPE_TEXT ||
					tds5type==TDS5_TYPE_IMAGE) {
				// The module has no table name to give - the
				// backends don't expose one for a result-set
				// column - and an empty one is legal.
				write(buffer,(uint16_t)0);
				debugWrite("table name: (none)");
			}
			break;

		case 5:
			// longchar/longbinary - a 32-bit size, and no table
			// name, blob-sized though they are
			write(buffer,size);
			debugWrite("size: %d (32-bit)",size);
			break;

		default:
			write(buffer,(byte_t)size);
			debugWrite("size: %d (8-bit)",size);
			break;
	}

	debugEnd();
}

// The size a column's values are declared at in a pre-tds7 rowfmt.
// preTds7TypeInfo() writes what this returns, and preTds7Field() caps
// every value it writes against it, so a field can't come out wider
// than the buffer the client sized from the rowfmt.  One function
// rather than the same rules written out twice - a second copy of them
// is a copy that can drift.
//
// Decimal and numeric don't come through here.  Their width isn't a
// size at all, it's decimalSize() of the precision, so they go through
// preTds7DecimalInfo() instead.
uint32_t sqlrprotocol_tds::preTds7DeclaredSize(sqlrservercursor *cursor,
						uint16_t col,
						uint16_t coltype,
						byte_t tds5type) {

	uint32_t	size=cont->getColumnSize(cursor,col);

	switch (preTds7VarintSize(tds5type)) {

		case 0:
			// the type carries its own width, and there's no size
			// field in the rowfmt to cap anything against
			return 0;

		case 4:
		case 5:
			// limit the size to 2^31-1 because the client will
			// interpret it as signed
			if (size>2147483647) {
				size=2147483647;
			}
			return size;

		default:
			switch (tds5type) {
				case TDS5_TYPE_INTN:
				case TDS5_TYPE_FLTN:
				case TDS5_TYPE_MONEYN:
				case TDS5_TYPE_DATETIMEN:
					// these only allow certain sizes.
					// the n-type bytes happen to be the
					// same in both dialects, so nTypeSize()
					// takes one as-is
					size=nTypeSize(coltype,tds5type,size);
					break;
				case TDS5_TYPE_VARCHAR:
				case TDS5_TYPE_CHAR:
					// A date/time column mapped to
					// varchar reports its binary-form
					// size (ct-lib says 4 for a date),
					// which is too small for the rendered
					// string, so measure it by type
					// instead or the client truncates it.
					size=dateTimeStringSize(coltype,size);
					break;
			}
			// The size byte is unsigned here, so the whole 255 is
			// usable - unlike the ms-tds path, which stops at 127
			// because the client reads that one signed.
			// FIXME: a wider column should go out as longchar
			// (0xAF) or longbinary (0xE1) rather than be
			// truncated, but the datatype byte is written before
			// this, out of pretds7typemap[], and the row writer
			// has to agree with it - so the promotion belongs in
			// the mapping, not here.
			if (size>255) {
				size=255;
			}
			return size;
	}
}

// The precision and scale a decimal or numeric column is declared at in
// a pre-tds7 rowfmt, and with them the width of its values - the client
// reads the sign byte and then as many magnitude bytes as
// decimalSize(precision) calls for, whatever length it was sent.  Both
// writers call this for the same reason they both call
// preTds7DeclaredSize().
//
// The client fails the connection outright on a precision of 0, a
// precision over the maximum, or a scale wider than the precision, so
// clamp all three - a backend that reports 0 would otherwise kill the
// session.
void sqlrprotocol_tds::preTds7DecimalInfo(sqlrservercursor *cursor,
						uint16_t col,
						byte_t *precision,
						byte_t *scale) {

	uint32_t	p=cont->getColumnPrecision(cursor,col);
	uint32_t	s=cont->getColumnScale(cursor,col);

	if (!p) {
		p=18;
	} else if (p>TDS_DECIMAL_MAX_PRECISION) {
		p=TDS_DECIMAL_MAX_PRECISION;
	}
	if (s>p) {
		s=p;
	}

	*precision=(byte_t)p;
	*scale=(byte_t)s;
}

void sqlrprotocol_tds::writeCollation() {

	// a collation is a little-endian 32-bit lcid/flags/version bitmap
	// followed by a sort id
	writeLE(&resppacket,(uint32_t)TDS_COLLATION_LCID);
	write(&resppacket,(byte_t)TDS_COLLATION_SORTID);

	debugCollation((uint32_t)TDS_COLLATION_LCID,
					(byte_t)TDS_COLLATION_SORTID);
}

void sqlrprotocol_tds::tableName(byte_t tdstype) {

	debugStart("table name");
	debugColumnType(tdstype);

	if (tdstype!=TDS_TYPE_TEXT &&
			tdstype!=TDS_TYPE_NTEXT &&
			tdstype!=TDS_TYPE_IMAGE) {
		debugEnd();
		return;
	}

	// It's not really clear what this is...
	// We only send it for text, ntext, and image columns.  It's called
	// "table" name but it appears to be a list of "part names".  I assume
	// they are partition names, but why would the client need to know
	// about those, and how do we get them?

	// FIXME: how do we get this?
	byte_t	numparts=1;

	// The spec is confusing about this, but it appears that 7.1- only
	// supports 1 partname, while 7.2+ supports more than 1, and you have
	// to tell it how many you're going to send.
	if (negotiatedtdsversion<720) {
		numparts=1;
	} else {
		write(&resppacket,numparts);
	}

	for (uint16_t i=0; i<numparts; i++) {
		const char	*partname8="";
		uint16_t	partnamelen=charstring::getLength(partname8);
		ucs2_t		*partname=ucs2charstring::duplicate(
							partname8,
							(size_t)partnamelen);
		writeLE(&resppacket,partnamelen);
		write(&resppacket,partname,partnamelen);
		delete[] partname;

		debugWrite("part name: %s",partname8);
	}

	debugEnd();
}

void sqlrprotocol_tds::cryptoMetaData() {

	debugStart("crypto meta data");
	debugEnd();

	if (negotiatedtdsversion<740) {
		return;
	}

	// FIXME: The client doesn't seem to care that this isn't
	// being sent.  How do we decide when to send it?

	// FIXME: actually implement this.  Crypto metadata is the
	// 0-based index of the key info in the cek table, the usertype
	// and base type info, the encryption algorithm (and its name,
	// if it's a custom one), the encryption algorithm type, and a
	// normalization version.
}

void sqlrprotocol_tds::colName(sqlrservercursor *cursor,
						uint16_t col) {

	debugStart("col name");

	size_t 		namelen=cont->getColumnNameSize(cursor,col);
	const char	*name=cont->getColumnName(cursor,col);

	// the length is a single byte, so a longer name is truncated,
	// the way preTds7RowFmt() and appendInfoOrError() truncate the
	// names they send; losing the tail of a long column name is much
	// better than desynchronizing the rest of the result-set stream.
	if (namelen>255) {
		namelen=255;
	}

	ucs2_t		*name16=ucs2charstring::duplicate(name,namelen);
	write(&resppacket,(byte_t)namelen);
	write(&resppacket,name16,namelen);

	debugWrite("namelen: %lld",(long long)namelen);
	debugWrite("name: %s",name);

	delete[] name16;

	debugEnd();
}

bool sqlrprotocol_tds::isCaseSensitiveType(byte_t tdstype) {

	switch (tdstype) {
		case TDS_TYPE_CHAR:
		case TDS_TYPE_VARCHAR:
		case TDS_TYPE_BIGVARCHR:
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_NVARCHAR:
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_TEXT:
		case TDS_TYPE_NTEXT:
			// FIXME: should only be 1 when
			// binary collation is used
			return true;
		case TDS_TYPE_XML:
			return true;
		default:
			return false;
	}
}

bool sqlrprotocol_tds::isFixedLenType(byte_t tdstype) {

	switch (tdstype) {
		case TDS_TYPE_NULL:
		case TDS_TYPE_INT1:
		case TDS_TYPE_BIT:
		case TDS_TYPE_INT2:
		case TDS_TYPE_INT4:
		case TDS_TYPE_DATETIM4:
		case TDS_TYPE_FLT4:
		case TDS_TYPE_MONEY:
		case TDS_TYPE_DATETIME:
		case TDS_TYPE_FLT8:
		case TDS_TYPE_MONEY4:
		case TDS_TYPE_INT8:
			return true;
		default:
			return false;
	}
}

bool sqlrprotocol_tds::isVarLenType(byte_t tdstype) {

	switch (tdstype) {
		case TDS_TYPE_GUID:
		case TDS_TYPE_INTN:
		case TDS_TYPE_DECIMAL:
		case TDS_TYPE_NUMERIC:
		case TDS_TYPE_BITN:
		case TDS_TYPE_DECIMALN:
		case TDS_TYPE_NUMERICN:
		case TDS_TYPE_FLTN:
		case TDS_TYPE_MONEYN:
		case TDS_TYPE_DATETIMN:
		case TDS_TYPE_DATEN:
		case TDS_TYPE_TIMEN:
		case TDS_TYPE_DATETIME2N:
		case TDS_TYPE_DATETIMEOFFSETN:
		case TDS_TYPE_CHAR:
		case TDS_TYPE_VARCHAR:
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
		case TDS_TYPE_BIGVARBIN:
		case TDS_TYPE_BIGVARCHR:
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_NVARCHAR:
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_XML:
		case TDS_TYPE_UDT:
		case TDS_TYPE_TEXT:
		case TDS_TYPE_IMAGE:
		case TDS_TYPE_NTEXT:
		case TDS_TYPE_SSVARIANT:
			return true;
		case TDS_TYPE_LONGBINARY:
			// longbinary is a sybase type that mssql doesn't
			// have, so only take it when we're fronting an ase.
			// against mssql it stays unknown, and the client
			// gets the same 8009 a real mssql sends.
			return dbisase;
		default:
			return false;
	}
}

bool sqlrprotocol_tds::isPartLenType(byte_t tdstype) {

	switch (tdstype) {
		case TDS_TYPE_XML:
		case TDS_TYPE_BIGVARCHR:
		case TDS_TYPE_BIGVARBIN:
		case TDS_TYPE_NVARCHAR:
		case TDS_TYPE_UDT:
			return true;
		default:
			return false;
	}
}

bool sqlrprotocol_tds::isCharType(byte_t tdstype) {

	switch (tdstype) {
		case TDS_TYPE_CHAR:
		case TDS_TYPE_VARCHAR:
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_BIGVARCHR:
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_NVARCHAR:
		case TDS_TYPE_TEXT:
		case TDS_TYPE_NTEXT:
			return true;
		default:
			return false;
	}
}

// The ms-tds datatype that means what a tds 5.0 datatype means, or 0 for
// one that has no ms-tds counterpart at all.
//
// rpcparamtdstypes[] is read by ms-tds predicates - isCharType() at the
// top of this file and paramIsUnicode() - and by everything built on
// them, so whatever goes in it has to be an ms-tds type byte whichever
// dialect the parameter arrived in.  Putting a raw tds 5.0 byte there
// would be silently wrong rather than merely unrecognized:
//
// * 0xE7 and 0x63 are nvarchar and ntext in ms-tds, and neither is a
//   datatype in tds 5.0 at all - they're the dynamic and optioncmd2
//   token bytes - so paramIsUnicode() would answer false for every tds
//   5.0 parameter, and executeSql() and prepare() reject a call whose
//   parameters aren't unicode.
// * 0xAF is a blank-padded, 2-byte-counted bigchar in ms-tds and a
//   4-byte-counted longchar here, so isCharType() would answer true and
//   returnValueChar() would write the wrong length field.
//
// The raw byte is kept alongside, in rpcparamtds5types[], for a reply
// that has to echo the type the client declared.
//
// A byte that isn't a tds 5.0 datatype is refused rather than passed
// through, so nothing can land in the array in the wrong namespace.
byte_t sqlrprotocol_tds::tds5TypeToMsType(byte_t tds5type) {

	switch (tds5type) {

		// same byte, same meaning in both dialects
		case TDS5_TYPE_VOID:		// TDS_TYPE_NULL
		case TDS5_TYPE_IMAGE:
		case TDS5_TYPE_TEXT:
		case TDS5_TYPE_VARBINARY:
		case TDS5_TYPE_INTN:
		case TDS5_TYPE_VARCHAR:
		case TDS5_TYPE_BINARY:
		case TDS5_TYPE_CHAR:
		case TDS5_TYPE_INT1:
		case TDS5_TYPE_BIT:
		case TDS5_TYPE_INT2:
		case TDS5_TYPE_INT4:
		case TDS5_TYPE_SHORTDATE:	// TDS_TYPE_DATETIM4
		case TDS5_TYPE_FLT4:
		case TDS5_TYPE_MONEY:
		case TDS5_TYPE_DATETIME:
		case TDS5_TYPE_FLT8:
		case TDS5_TYPE_DECN:		// TDS_TYPE_DECIMALN
		case TDS5_TYPE_NUMN:		// TDS_TYPE_NUMERICN
		case TDS5_TYPE_FLTN:
		case TDS5_TYPE_MONEYN:
		case TDS5_TYPE_DATETIMEN:	// TDS_TYPE_DATETIMN
		case TDS5_TYPE_SHORTMONEY:	// TDS_TYPE_MONEY4
		case TDS5_TYPE_LONGBINARY:
			return tds5type;

		// The unsigned integers, which ms-tds doesn't have - each
		// one becomes the signed type of its own width.  The value
		// itself is read here rather than by anything that reads
		// this array, so nothing is lost by it.
		case TDS5_TYPE_UINT1:
			return TDS_TYPE_INT1;
		case TDS5_TYPE_UINT2:
			return TDS_TYPE_INT2;
		case TDS5_TYPE_UINT4:
			return TDS_TYPE_INT4;
		case TDS5_TYPE_UINT8:
			return TDS_TYPE_INT8;
		case TDS5_TYPE_UINTN:
			return TDS_TYPE_INTN;

		// ms-tds has no signed 1-byte type either - int1 is a
		// tinyint, and tinyint is unsigned
		case TDS5_TYPE_SINT1:
			return TDS_TYPE_INT1;

		// the 8-byte integer is 0xBF here and 0x7F in ms-tds, where
		// 0xBF isn't a datatype and 0x7F is a 1-byte varint
		case TDS5_TYPE_INT8:
			return TDS_TYPE_INT8;

		// date and time, fixed-width and nullable alike - ms-tds
		// only has the nullable forms
		case TDS5_TYPE_DATE:
		case TDS5_TYPE_DATEN:
			return TDS_TYPE_DATEN;
		case TDS5_TYPE_TIME:
		case TDS5_TYPE_TIMEN:
			return TDS_TYPE_TIMEN;

		// Longchar is 4-byte counted and not blank padded, so it's
		// a bigvarchr rather than the bigchar that happens to share
		// its byte in ms-tds.  Blank padding a value out to a
		// declared size that a longchar never meant as a fixed
		// width is the concrete difference.
		case TDS5_TYPE_LONGCHAR:
			return TDS_TYPE_BIGVARCHR;

		// unitext is utf-16 text
		case TDS5_TYPE_UNITEXT:
			return TDS_TYPE_NTEXT;

		// Xml has text's wire shape here - a 4-byte length - and
		// text is also what makes isCharType() answer correctly for
		// it.  TDS_TYPE_XML is the closer name, but no ms-tds
		// character predicate covers it, so a parameter tagged with
		// it would silently lose the output buffer bindParams()
		// preallocates for a character type.
		case TDS5_TYPE_XML:
			return TDS_TYPE_TEXT;

		default:
			// Nothing else is bindable.  Blob (0x24) is a
			// serialized object behind a class id, interval
			// (0x2E) has no counterpart, and sensitivity (0x67)
			// and boundary (0x68) are labels rather than values.
			// 0x24 and 0x68 are uniqueidentifier and bitn in
			// ms-tds, so passing either byte through would be
			// worse than refusing it.
			return 0;
	}
}

int64_t sqlrprotocol_tds::moneyValue(const char *field) {

	debugStart("money value");

	if (charstring::isNullOrEmpty(field)) {
		debugWrite("field: null or empty");
		debugWrite("value: 0");
		debugEnd();
		return 0;
	}

	debugWrite("field: %s",field);

	const char	*ch=field;
	bool		negative=false;
	if (*ch=='-') {
		negative=true;
		ch++;
	} else if (*ch=='+') {
		ch++;
	}

	int64_t	value=0;
	for (; character::isDigit(*ch); ch++) {
		value=value*10+(*ch-'0');
	}

	// exactly four fractional digits, however many the back end sent
	// (tds carries money as the value times 10^4, and the back ends
	// differ - mssql through odbc renders four places, others render two)
	uint16_t	places=0;
	if (*ch=='.') {
		ch++;
		for (; character::isDigit(*ch) && places<4; ch++) {
			value=value*10+(*ch-'0');
			places++;
		}
	}
	while (places<4) {
		value=value*10;
		places++;
	}

	int64_t	retval=(negative)?-value:value;
	debugWrite("value: %lld",(long long)retval);
	debugEnd();
	return retval;
}

byte_t sqlrprotocol_tds::nTypeSize(uint16_t coltype,
					byte_t tdstype,
					uint32_t colsize) {

	debugStart("n type size");
	debugWrite("coltype: %d",coltype);
	debugColumnType(tdstype);
	debugWrite("colsize: %d",colsize);

	byte_t	retval=colsize;
	switch (tdstype) {
		case TDS_TYPE_BITN:
			retval=1;
			break;
		case TDS_TYPE_INTN:
			// storage widths pass through
			if (colsize==1 || colsize==2 ||
					colsize==4 || colsize==8) {
				retval=(byte_t)colsize;
				break;
			}
			// otherwise it's a digit count
			if (colsize<=3) {
				retval=1;
			} else if (colsize<=5) {
				retval=2;
			} else if (colsize<=10) {
				retval=4;
			} else {
				retval=8;
			}
			break;
		case TDS_TYPE_FLTN:
		case TDS_TYPE_MONEYN:
		case TDS_TYPE_DATETIMN:
			// only 4 and 8 are valid, and the column type is the
			// only thing that says which - a back end that
			// reports a digit count rather than a storage width
			// gives 24 for a real and 19 for a money
			switch (coltype) {
				case SMALLDATETIME_DATATYPE:
				case REAL_DATATYPE:
				case SMALLMONEY_DATATYPE:
					retval=4;
					break;
				case DATETIME_DATATYPE:
				case FLOAT_DATATYPE:
				case DOUBLE_DATATYPE:
				case MONEY_DATATYPE:
					retval=8;
					break;
				default:
					// fall back on the storage width, for
					// a back end that reports one and for
					// uncovered types
					retval=(colsize==4)?4:8;
					break;
			}
			break;
	}

	debugWrite("size: %d",retval);
	debugEnd();
	return retval;
}

uint32_t sqlrprotocol_tds::dateTimeStringSize(uint16_t coltype,
						uint32_t colsize) {

	debugStart("date time string size");
	debugWrite("coltype: %d",coltype);
	debugWrite("colsize: %d",colsize);

	// A date/time column only gets here when mapType() downgraded it to
	// nvarchar for a pre-7.3 client.  Back ends report the size of the
	// binary form (odbc says 10 for a date, ct-lib says 4), which is too
	// small for the rendered string - these sizes are wide enough for
	// every rendering the back ends produce, including ase's
	// "Jan  1 2001  1:01PM".
	uint32_t	stringsize=0;
	switch (coltype) {
		case DATE_DATATYPE:
		case TIME_DATATYPE:
			stringsize=32;
			break;
		case TIMESTAMP_DATATYPE:
		case DATETIMEOFFSET_DATATYPE:
			stringsize=48;
			break;
	}
	uint32_t	retval=(stringsize>colsize)?stringsize:colsize;

	debugWrite("size: %d",retval);
	debugEnd();
	return retval;
}

uint64_t sqlrprotocol_tds::rows(sqlrservercursor *cursor) {
	return rows(cursor,0);
}

uint64_t sqlrprotocol_tds::rows(sqlrservercursor *cursor, uint64_t maxrows,
							tdsrows *position) {

	// get col count and bail if there are no columns
	uint32_t	colcount=cont->colCount(cursor);
	if (!colcount) {
		// return the affected row count though
		return cont->getAffectedRows(cursor);
	}

	if (position) {
		position->reset(colcount);
	}

	// for each row...
	uint64_t	rowcount=0;
	for (;;) {

		// stop at the row the caller asked for, if it asked
		if (maxrows && rowcount==maxrows) {
			break;
		}

		// fetch a row
		bool	error;
		if (!cont->fetchRow(cursor,&error)) {
			if (error) {
				// FIXME: handle error
			}
			break;
		}

		// append the token to the packet
		byte_t	token=TOKEN_ROW;
		write(&resppacket,token);

		debugStart("row");
		debugTokenType(token);

		tdsrow	*positionrow=(position)?position->newRow():NULL;

		// append the fields to the packet
		for (uint32_t col=0; col<colcount; col++) {

			// get/map the column type
			// FIXME: cache this earlier and just look it up here
			uint16_t	coltype=
					cont->getColumnType(cursor,col);
			byte_t		tdstype=mapType(coltype);

			debugStart("col %d",col);
			debugColumnType(tdstype);

			lobData(tdstype);

			// get the field
			const char	*fld=NULL;
			uint64_t	fldsize=0;
			bool		lob=false;
			bool		null=false;
			if (!cont->getField(cursor,col,
					&fld,&fldsize,&lob,&null)) {
				// FIXME: handle error
			}

			// send the field
			field(coltype,tdstype,
				// FIXME: cache these earlier and
				// just look them up here
				cont->getColumnSize(cursor,col),
				cont->getColumnScale(cursor,col),
				fld,fldsize,null);

			// keep the value for a positioned update to
			// match on, while it's still readable
			if (positionrow) {
				position->setField(positionrow,col,
							fld,fldsize,null);
			}

			debugEnd();
		}

		// FIXME: kludgy
		cont->nextRow(cursor);

		debugEnd();

		// bump row count
		rowcount++;
	}

	return rowcount;
}

// The tds 5.0 counterpart of rows().  The token byte is the same in
// both dialects but nothing inside the row is, so the two don't share a
// body: the column type comes out of pretds7typemap[] rather than
// tdstypemap[], the length prefix and the null form come from the
// datatype's varint class rather than from the type itself, and there's
// no separate lob-data step - a tds 5.0 blob's text pointer is part of
// its field.
//
// There's no maxrows/position pair either.  Those serve the ms-tds
// cursor tokens, and tds 5.0 cursors are somebody else's ticket.
uint64_t sqlrprotocol_tds::preTds7Rows(sqlrservercursor *cursor) {

	// get col count and bail if there are no columns
	uint32_t	colcount=cont->colCount(cursor);
	if (!colcount) {
		// return the affected row count though
		return cont->getAffectedRows(cursor);
	}

	// for each row...
	uint64_t	rowcount=0;
	for (;;) {

		// fetch a row
		bool	error;
		if (!cont->fetchRow(cursor,&error)) {
			if (error) {
				// FIXME: handle error
			}
			break;
		}

		// append the token to the packet
		byte_t	token=TOKEN_ROW;
		write(&resppacket,token);

		debugStart("pre-tds7 row");
		debugTokenType(token);

		// append the fields to the packet
		for (uint32_t i=0; i<colcount; i++) {

			uint16_t	col=(uint16_t)i;

			// get/map the column type
			// FIXME: cache this earlier and just look it up here
			uint16_t	coltype=cont->getColumnType(cursor,col);
			byte_t		tds5type=mapType(coltype);

			debugStart("pre-tds7 col %d",col);

			// get the field
			const char	*fld=NULL;
			uint64_t	fldsize=0;
			bool		lob=false;
			bool		null=false;
			if (!cont->getField(cursor,col,
					&fld,&fldsize,&lob,&null)) {
				// FIXME: handle error
			}

			// Decimal and numeric are sized by the precision
			// rather than by a column size.
			// FIXME: cache this earlier too
			byte_t	precision=0;
			byte_t	scale=0;
			if (tds5type==TDS5_TYPE_DECN ||
					tds5type==TDS5_TYPE_NUMN) {
				preTds7DecimalInfo(cursor,col,
							&precision,&scale);
			}

			// Send the field, capped against the size the rowfmt
			// declared for this column.  Both numbers come from
			// the same helpers preTds7TypeInfo() declared them
			// with, so they agree by construction rather than by
			// two writers happening to compute the same thing.
			// FIXME: cache this earlier too
			preTds7Field(coltype,tds5type,
					preTds7DeclaredSize(cursor,col,
							coltype,tds5type),
					precision,fld,fldsize,null);

			debugEnd();
		}

		// FIXME: kludgy
		cont->nextRow(cursor);

		debugEnd();

		// bump row count
		rowcount++;
	}

	return rowcount;
}

void sqlrprotocol_tds::lobData(byte_t tdstype) {

	if (tdstype!=TDS_TYPE_TEXT &&
			tdstype!=TDS_TYPE_NTEXT &&
			tdstype!=TDS_TYPE_IMAGE) {
		return;
	}

	debugStart("lob data");

	// I have no idea what these are or how to get them.  SQL Server itself
	// appears to send dummy versions of them though, so we'll do the same.

	// dummy textpointer
	const char	*textptr="dummy textptr   ";
	byte_t		textptrsize=charstring::getLength(textptr);
	write(&resppacket,textptrsize);
	write(&resppacket,textptr,textptrsize);

	// dummy timestamp
	const char	*ts="dummyTS";
	write(&resppacket,ts,8);

	debugWrite("textptrsize: %d",textptrsize);
	debugWrite("textptr: %s",textptr);
	debugWrite("ts: %s",ts);

	debugEnd();
}

void sqlrprotocol_tds::field(uint16_t coltype,
				byte_t tdstype,
				uint32_t colsize,
				uint32_t colscale,
				const char *field,
				uint64_t fieldsize,
				bool null) {

	debugStart("field");

	// the scale of a time is limited to 7 decimal places
	byte_t	scale=(colscale>7)?7:(byte_t)colscale;

	// handle nulls
	if (null) {

		debugWrite("data: null");

		switch (tdstype) {
			case TDS_TYPE_NULL:
			case TDS_TYPE_GUID:
			case TDS_TYPE_INTN:
			case TDS_TYPE_BITN:
			case TDS_TYPE_DECIMALN:
			case TDS_TYPE_NUMERICN:
			case TDS_TYPE_FLTN:
			case TDS_TYPE_MONEYN:
			case TDS_TYPE_DATETIMN:
			case TDS_TYPE_DATEN:
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				write(&resppacket,(byte_t)0x00);
				break;
			case TDS_TYPE_CHAR:
			case TDS_TYPE_VARCHAR:
			case TDS_TYPE_BINARY:
			case TDS_TYPE_VARBINARY:
				write(&resppacket,(byte_t)0x00);
				break;
			case TDS_TYPE_BIGBINARY:
			case TDS_TYPE_BIGVARBIN:
			case TDS_TYPE_BIGCHAR:
			case TDS_TYPE_BIGVARCHR:
			case TDS_TYPE_NCHAR:
			case TDS_TYPE_NVARCHAR:
				writeLE(&resppacket,(uint16_t)0xFFFF);
				break;
			case TDS_TYPE_UDT:
				// FIXME: ???
				break;
			case TDS_TYPE_TEXT:
			case TDS_TYPE_NTEXT:
			case TDS_TYPE_IMAGE:
			case TDS_TYPE_SSVARIANT:
				writeLE(&resppacket,(uint32_t)0xFFFFFFFF);
				break;
			case TDS_TYPE_XML:
				// PLP_NULL - an all-ones 64-bit length,
				// since XML is always PLP-encoded
				writeLE(&resppacket,(uint64_t)0xFFFFFFFFFFFFFFFFULL);
				break;
		}

		debugEnd();
		return;
	}

	// append size, normalize type for data switch below
	switch (tdstype) {
		case TDS_TYPE_INTN:
			{
			byte_t	size=nTypeSize(coltype,tdstype,
								colsize);
			write(&resppacket,size);
			switch (size) {
				case 1:
					tdstype=TDS_TYPE_INT1;
					break;
				case 2:
					tdstype=TDS_TYPE_INT2;
					break;
				case 4:
					tdstype=TDS_TYPE_INT4;
					break;
				case 8:
					tdstype=TDS_TYPE_INT8;
					break;
			}
			}
			break;
		case TDS_TYPE_BITN:
			write(&resppacket,(byte_t)1);
			tdstype=TDS_TYPE_BIT;
			break;
		case TDS_TYPE_FLTN:
			{
			byte_t	size=nTypeSize(coltype,tdstype,
								colsize);
			write(&resppacket,size);
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_FLT4;
					break;
				case 8:
					tdstype=TDS_TYPE_FLT8;
					break;
			}
			}
			break;
		case TDS_TYPE_MONEYN:
			{
			byte_t	size=nTypeSize(coltype,tdstype,
								colsize);
			write(&resppacket,size);
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_MONEY4;
					break;
				case 8:
					tdstype=TDS_TYPE_MONEY;
					break;
			}
			}
			break;
		case TDS_TYPE_DATETIMN:
			{
			byte_t	size=nTypeSize(coltype,tdstype,
								colsize);
			write(&resppacket,size);
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_DATETIM4;
					break;
				case 8:
					tdstype=TDS_TYPE_DATETIME;
					break;
			}
			}
			break;
	}

	// append the data
	switch (tdstype) {
		case TDS_TYPE_INT1:
		case TDS_TYPE_BIT:
			{
			char	data=charstring::convertToInteger(field);
			write(&resppacket,data);
			debugWrite("data: ");
			debugWrite("%d (1 byte)",data);
			}
			break;
		case TDS_TYPE_INT2:
			{
			int16_t	data=charstring::convertToInteger(field);
			writeLE(&resppacket,(uint16_t)data);
			debugWrite("data: ");
			debugWrite("%hd (2 bytes)",data);
			}
			break;
		case TDS_TYPE_INT4:
			{
			int32_t	data=charstring::convertToInteger(field);
			writeLE(&resppacket,(uint32_t)data);
			debugWrite("data: ");
			debugWrite("%d (4 bytes)",data);
			}
			break;
		case TDS_TYPE_DATETIM4:
			{
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			dateTime(field,&dayssince1900,&threehundredths);
			uint16_t	days=(dayssince1900>0)?
							dayssince1900:0;
			uint16_t	minutes=threehundredths/300/60;
			writeLE(&resppacket,days);
			writeLE(&resppacket,minutes);
			debugWrite("data: ");
			debugWrite("%d,%d",(uint32_t)days,(uint32_t)minutes);
			}
			break;
		case TDS_TYPE_FLT4:
			{
			float	data=charstring::convertToFloat(field);
			write(&resppacket,data);
			debugWrite("data: ");
			debugWrite("%f",data);
			}
			break;
		case TDS_TYPE_MONEY:
			{
			int64_t	data=moneyValue(field);
			writeLE(&resppacket,
				(uint32_t)((data&0xFFFFFFFF00000000LL)>>32));
			writeLE(&resppacket,
				(uint32_t)(data&0x00000000FFFFFFFFLL));
			debugWrite("data: ");
			debugWrite("%u %u (%s)",
				(uint32_t)((data&0xFFFFFFFF00000000LL)>>32),
				(uint32_t)(data&0x00000000FFFFFFFFLL),
				field);
			}
			break;
		case TDS_TYPE_DATETIME:
			{
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			dateTime(field,&dayssince1900,&threehundredths);
			writeLE(&resppacket,(uint32_t)dayssince1900);
			writeLE(&resppacket,threehundredths);
			debugWrite("data: ");
			debugWrite("%d,%d",dayssince1900,threehundredths);
			}
			break;
		case TDS_TYPE_FLT8:
			{
			double	data=charstring::convertToFloat(field);
			write(&resppacket,data);
			debugWrite("data: ");
			debugWrite("%f",data);
			}
			break;
		case TDS_TYPE_MONEY4:
			{
			int32_t	data=(int32_t)moneyValue(field);
			writeLE(&resppacket,(uint32_t)data);
			debugWrite("data: ");
			debugWrite("%lld (%s)",(long long)data,field);
			}
			break;
		case TDS_TYPE_INT8:
			{
			int64_t	data=charstring::convertToInteger(field);
			writeLE(&resppacket,(uint64_t)data);
			debugWrite("data: ");
			debugWrite("%lld (8 bytes)",(long long)data);
			}
			break;
		case TDS_TYPE_GUID:
			{
			byte_t	g[16];
			guid(field,g);
			if (getDebug()) {
				debugWrite("data: ");
				for (uint16_t i=0; i<16; i++) {
					debugWrite("%02x",g[i]);
				}
			}
			write(&resppacket,(byte_t)sizeof(g));
			write(&resppacket,g,sizeof(g));
			}
			break;
		case TDS_TYPE_DECIMAL:
		case TDS_TYPE_NUMERIC:
		case TDS_TYPE_DECIMALN:
		case TDS_TYPE_NUMERICN:
			{
			byte_t	ispositive;
			byte_t	size;
			byte_t	val[16];
			decimal(field,&ispositive,&size,val);
			if (tdstype==TDS_TYPE_DECIMALN ||
				tdstype==TDS_TYPE_NUMERICN) {
				write(&resppacket,size);
				write(&resppacket,ispositive);
				write(&resppacket,val,size-1);
			} else {
				write(&resppacket,ispositive);
				write(&resppacket,val,size);
			}
			debugWrite("data: ");
			debugWrite("%d ",ispositive);
			switch (size) {
				case 4: {
					uint32_t	*v=(uint32_t *)val;
					debugWrite("%d ",*v);
					break;
					}
				case 8: {
					uint64_t	*v=(uint64_t *)val;
					debugWrite("%lld ",(long long)*v);
					break;
					}
				case 12:
					debugWrite("... ");
					break;
				case 16:
					debugWrite("... ");
					break;
			}
			debugWrite("(%s %d)",field,size);
			}
			break;
		case TDS_TYPE_DATEN:
			daten(field);
			break;
		case TDS_TYPE_TIMEN:
			timen(field,scale);
			break;
		case TDS_TYPE_DATETIME2N:
			datetime2n(field,scale);
			break;
		case TDS_TYPE_DATETIMEOFFSETN:
			datetimeoffsetn(field,scale);
			break;
		case TDS_TYPE_CHAR:
		case TDS_TYPE_VARCHAR:
			{
			write(&resppacket,(byte_t)fieldsize);
			write(&resppacket,field,fieldsize);
			debugWrite("size: %lld",(long long)fieldsize);
			debugWrite("data: ");
			debugWrite("%.*s",(int)fieldsize,field);
			}
			break;
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
			{
			// the size can't exceed the one sent by typeInfo()
			uint64_t	size=(binaryishextext)?
						fieldsize/2:fieldsize;
			if (size>127) {
				size=127;
			}
			write(&resppacket,(byte_t)size);
			binary(field,size,binaryishextext);
			debugWrite("size: %lld",(long long)size);
			debugWrite("data:");
			debugHexDump((byte_t *)field,fieldsize);
			}
			break;
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGVARBIN:
			{
			// the size can't exceed the one sent by typeInfo()
			uint64_t	size=(binaryishextext)?
						fieldsize/2:fieldsize;
			if (size>32767) {
				size=32767;
			}
			writeLE(&resppacket,(uint16_t)size);
			binary(field,size,binaryishextext);
			debugWrite("size: %lld",(long long)size);
			debugWrite("data:");
			debugHexDump((byte_t *)field,fieldsize);
			}
			break;
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_BIGVARCHR:
			{
			// the collation declares these cp1252
			size_t	field8size;
			char	*field8=utf8ToCp1252(field,fieldsize,
							&field8size);
			writeLE(&resppacket,(uint16_t)field8size);
			write(&resppacket,field8,field8size);
			delete[] field8;
			debugWrite("size: %lld",(long long)field8size);
			debugWrite("data: ");
			debugWrite("%.*s",(int)fieldsize,field);
			}
			break;
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_NVARCHAR:
			{
			// the data is ucs-2, and the size is in bytes
			size_t	field16length;
			ucs2_t	*field16=utf8ToUcs2(field,fieldsize,
							&field16length);
			writeLE(&resppacket,
				(uint16_t)(field16length*sizeof(ucs2_t)));
			write(&resppacket,field16,field16length);
			delete[] field16;
			debugWrite("size: %lld",
				(long long)(field16length*sizeof(ucs2_t)));
			debugWrite("data: ");
			debugWrite("%.*s",(int)fieldsize,field);
			}
			break;
		case TDS_TYPE_UDT:
			// FIXME: ???
			break;
		case TDS_TYPE_TEXT:
			{
			// the collation declares this cp1252
			size_t	field8size;
			char	*field8=utf8ToCp1252(field,fieldsize,
							&field8size);
			writeLE(&resppacket,(uint32_t)field8size);
			write(&resppacket,field8,field8size);
			delete[] field8;
			debugWrite("size: %lld",(long long)field8size);
			debugWrite("data: ");
			debugWrite("%.*s",(int)fieldsize,field);
			}
			break;
		case TDS_TYPE_NTEXT:
			{
			// the data is ucs-2, and the size is in bytes
			size_t	field16length;
			ucs2_t	*field16=utf8ToUcs2(field,fieldsize,
							&field16length);
			writeLE(&resppacket,
				(uint32_t)(field16length*sizeof(ucs2_t)));
			write(&resppacket,field16,field16length);
			delete[] field16;
			debugWrite("size: %lld",
				(long long)(field16length*sizeof(ucs2_t)));
			debugWrite("data: ");
			debugWrite("%.*s",(int)fieldsize,field);
			}
			break;
		case TDS_TYPE_XML:
			{
			// PLP encoding (MS-TDS 2.2.5.2.3.2): 8-byte total
			// length, then chunks of (4-byte length, bytes),
			// ending in a 4-byte zero-length chunk.  Skip the data
			// chunk for an empty value - the terminator alone
			// means empty, and an extra chunk would corrupt what
			// follows.
			size_t	field16length;
			ucs2_t	*field16=utf8ToUcs2(field,fieldsize,
							&field16length);
			uint64_t	totalsize=
				(uint64_t)(field16length*sizeof(ucs2_t));
			writeLE(&resppacket,totalsize);
			if (totalsize) {
				writeLE(&resppacket,(uint32_t)totalsize);
				write(&resppacket,field16,field16length);
			}
			writeLE(&resppacket,(uint32_t)0);
			delete[] field16;
			debugWrite("size: %lld",(long long)totalsize);
			debugWrite("data: ");
			debugWrite("%.*s",(int)fieldsize,field);
			}
			break;
		case TDS_TYPE_IMAGE:
		case TDS_TYPE_SSVARIANT:
			{
			// the size can't exceed the one sent by typeInfo()
			uint64_t	size=(imageishextext)?
						fieldsize/2:fieldsize;
			if (size>2147483647) {
				size=2147483647;
			}
			writeLE(&resppacket,(uint32_t)size);
			binary(field,size,imageishextext);
			debugWrite("size: %lld",(long long)size);
			debugWrite("data:");
			debugHexDump((byte_t *)field,fieldsize);
			}
			break;
	}

	debugEnd();
}

// The tds 5.0 counterpart of field().  The value encodings themselves
// are the same ones field() writes - only the length in front of them,
// the way a null is spelled, and the layout of a decimal come out
// differently - but the length and the null form are driven by the
// datatype's varint class here rather than by field()'s two per-type
// switches, so there's nothing worth sharing between them.
//
// "colsize" is what preTds7DeclaredSize() gave preTds7TypeInfo() for
// this column, and "precision" what preTds7DecimalInfo() gave it, so
// capping a value against them is the same thing as capping it against
// the rowfmt.  A field wider than the rowfmt declared would overrun the
// buffer the client sized from that rowfmt.
void sqlrprotocol_tds::preTds7Field(uint16_t coltype,
					byte_t tds5type,
					uint32_t colsize,
					byte_t precision,
					const char *field,
					uint64_t fieldsize,
					bool null) {

	debugStart("pre-tds7 field");
	debugPreTds7ColumnType(tds5type);

	// handle nulls
	if (null) {

		debugWrite("data: null");

		switch (preTds7VarintSize(tds5type)) {
			case 0:
				{
				// A fixed-length type has no null form at
				// all - there's no length field to set to
				// zero - so a null has to go out as a zero
				// value.  Bit is the only varint-0 type
				// pretds7typemap[] produces, and an ase bit
				// column is never nullable, but writing the
				// right number of bytes for any of them
				// keeps a type mapped in later from
				// desynchronizing the row.
				byte_t	zero[8];
				bytestring::zero(zero,sizeof(zero));
				write(&resppacket,zero,
						preTds7FixedSize(tds5type));
				}
				break;
			case 4:
				// a text-pointer length of 0, and nothing
				// after it
				write(&resppacket,(byte_t)0);
				break;
			case 5:
				write(&resppacket,(uint32_t)0);
				break;
			default:
				// A length of 0 is a varint-1 type's only
				// null form, and it's also what an empty
				// value comes out as - so an empty varchar
				// and a null varchar are the same bytes on
				// the wire, and the client reads both as
				// null.  That's how tds 5.0 works rather
				// than something to work around; a real ase
				// sends the same thing.
				write(&resppacket,(byte_t)0);
				break;
		}

		debugEnd();
		return;
	}

	switch (tds5type) {

		// The n-types: a size byte, then the value at that width.
		// The size is the one the rowfmt declared, which nTypeSize()
		// already narrowed to a width the type allows.
		case TDS5_TYPE_INTN:
			{
			byte_t	size=(byte_t)colsize;
			write(&resppacket,size);
			int64_t	data=charstring::convertToInteger(field);
			switch (size) {
				case 1:
					write(&resppacket,(char)data);
					break;
				case 2:
					write(&resppacket,(uint16_t)data);
					break;
				case 4:
					write(&resppacket,(uint32_t)data);
					break;
				case 8:
					write(&resppacket,(uint64_t)data);
					break;
			}
			debugWrite("size: %d",size);
			debugWrite("data: ");
			debugWrite("%lld",(long long)data);
			}
			break;
		case TDS5_TYPE_FLTN:
			{
			byte_t	size=(byte_t)colsize;
			write(&resppacket,size);
			double	data=charstring::convertToFloat(field);
			writeFloatN(data,size);
			debugWrite("size: %d",size);
			debugWrite("data: ");
			debugWrite("%f",data);
			}
			break;
		case TDS5_TYPE_MONEYN:
			{
			byte_t	size=(byte_t)colsize;
			write(&resppacket,size);
			int64_t	data=moneyValue(field);
			if (size==4) {
				write(&resppacket,(uint32_t)(int32_t)data);
			} else {
				// The high half goes first, ahead of the
				// low half.  That ordering is the type's
				// own, not a byte order - each half goes
				// out in the order the login declared.
				write(&resppacket,(uint32_t)
					((data&0xFFFFFFFF00000000LL)>>32));
				write(&resppacket,(uint32_t)
					(data&0x00000000FFFFFFFFLL));
			}
			debugWrite("size: %d",size);
			debugWrite("data: ");
			debugWrite("%lld (%s)",(long long)data,field);
			}
			break;
		case TDS5_TYPE_DATETIMEN:
			{
			byte_t	size=(byte_t)colsize;
			write(&resppacket,size);
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			dateTime(field,&dayssince1900,&threehundredths);
			debugWrite("size: %d",size);
			debugWrite("data: ");
			if (size==4) {
				// days and whole minutes
				uint16_t	days=(dayssince1900>0)?
							dayssince1900:0;
				uint16_t	minutes=threehundredths/300/60;
				write(&resppacket,days);
				write(&resppacket,minutes);
				debugWrite("%d,%d",(uint32_t)days,
							(uint32_t)minutes);
			} else {
				// days and three-hundredths of a second
				write(&resppacket,(uint32_t)dayssince1900);
				write(&resppacket,threehundredths);
				debugWrite("%d,%d",dayssince1900,
							threehundredths);
			}
			}
			break;

		// varint 0 - the value alone, with no length in front of it
		case TDS5_TYPE_BIT:
			{
			char	data=charstring::convertToInteger(field);
			write(&resppacket,data);
			debugWrite("data: ");
			debugWrite("%d (1 byte)",data);
			}
			break;

		case TDS5_TYPE_DECN:
		case TDS5_TYPE_NUMN:
			{
			byte_t	ispositive;
			byte_t	size;
			byte_t	val[TDS_DECIMAL_MAX_SIZE-1];
			// decimal() only fills the low 4 or 8 bytes, so zero
			// the rest before reversing a wider window of it
			bytestring::zero(val,sizeof(val));
			decimal(field,&ispositive,&size,val);

			// Two things are inverted from the ms-tds form that
			// decimal() produces and field() writes:
			//
			// * the sign byte is 0 for positive and 1 for
			//   negative, the opposite way round from
			//   decimal()'s "ispositive"
			// * the magnitude is big-endian, and decimal()
			//   builds it little-endian
			//
			// The width is decimalSize() of the precision the
			// rowfmt declared, counting the sign byte, so
			// decimalSize(precision)-1 magnitude bytes follow it.
			// It is NOT decimal()'s "size", which is derived from
			// how many digits the value happens to have.  The
			// client reads the sign byte and then exactly as many
			// magnitude bytes as the rowfmt's precision calls
			// for, whatever length it was sent, so any other
			// width decodes to garbage - which is why the
			// precision here comes from preTds7DecimalInfo(), the
			// same place the rowfmt's did.
			byte_t	wiresize=decimalSize(precision);
			write(&resppacket,wiresize);
			write(&resppacket,(byte_t)((ispositive)?0:1));
			for (byte_t i=0; i<wiresize-1; i++) {
				write(&resppacket,val[wiresize-2-i]);
			}

			debugWrite("size: %d",wiresize);
			debugWrite("sign: %d",(ispositive)?0:1);
			debugWrite("data: ");
			debugWrite("%s (precision %d)",field,precision);
			}
			break;

		case TDS5_TYPE_VARCHAR:
		case TDS5_TYPE_CHAR:
			{
			// Character data goes out in the charset the login
			// record declared, converted from the utf-8 the back
			// end handed over.  A tds 5.0 rowfmt has no collation
			// field, so the login record is the only thing that
			// names a charset, and preTds7ToUtf8() converts the
			// other way with the same one - the two directions
			// agree.
			//
			// The conversion has to run before the size cap
			// below, not after it: the cap counts the bytes that
			// go on the wire against the width the rowfmt
			// declared, so capping the utf-8 first would measure
			// the wrong encoding and could cut a multi-byte
			// character in half.
			//
			// A client that declared utf8, nothing, or a charset
			// pretds7charsets[] doesn't cover gets the utf-8
			// bytes straight through.  One consequence there: the
			// length counts utf-8 bytes, so a multi-byte
			// character eats more than one unit of the declared
			// column size.
			size_t		convsize=0;
			char		*conv=utf8ToClientCharset(field,
						(size_t)fieldsize,&convsize);
			const char	*data=(conv)?conv:field;
			uint64_t	size=(conv)?convsize:fieldsize;
			if (size>colsize) {
				size=colsize;
			}
			write(&resppacket,(byte_t)size);
			write(&resppacket,data,size);
			debugWrite("size: %lld",(long long)size);
			debugWrite("data: ");
			debugWrite("%.*s",(int)size,data);
			delete[] conv;
			}
			break;

		case TDS5_TYPE_VARBINARY:
		case TDS5_TYPE_BINARY:
			{
			// the sap back end hands binary values back as hex
			// text, so the wire length is half the field length
			uint64_t	size=(binaryishextext)?
						fieldsize/2:fieldsize;
			if (size>colsize) {
				size=colsize;
			}
			write(&resppacket,(byte_t)size);
			binary(field,size,binaryishextext);
			debugWrite("size: %lld",(long long)size);
			debugWrite("data:");
			debugHexDump((byte_t *)field,fieldsize);
			}
			break;

		case TDS5_TYPE_TEXT:
		case TDS5_TYPE_IMAGE:
			{
			// image comes back as hex text from the sap back end
			// too, under its own config flag - text doesn't
			bool		hextext=(tds5type==TDS5_TYPE_IMAGE &&
							imageishextext);

			// text is character data, so it goes out in the
			// charset the login record declared, the way a
			// varchar does above - and before the size cap, for
			// the same reason.  image is bytes, so it doesn't.
			size_t		convsize=0;
			char		*conv=(tds5type==TDS5_TYPE_TEXT)?
						utf8ToClientCharset(field,
						(size_t)fieldsize,&convsize):
						NULL;
			const char	*data=(conv)?conv:field;
			uint64_t	size=(conv)?convsize:
						((hextext)?fieldsize/2:
								fieldsize);
			if (size>colsize) {
				size=colsize;
			}

			// A non-null varint-4 field is a 16-byte text
			// pointer, an 8-byte timestamp, a 32-bit data
			// length, and then the data.  lobData() already
			// writes the first two - the dummy values it makes
			// up are as good here as they are on the ms-tds
			// path, and the tds 5.0 type bytes for text and
			// image are the same 0x23 and 0x22 it switches on,
			// so it takes one as-is.
			lobData(tds5type);

			write(&resppacket,(uint32_t)size);
			binary(data,size,hextext);
			debugWrite("size: %lld",(long long)size);
			debugWrite("data:");
			debugHexDump((byte_t *)data,
					(conv)?convsize:(size_t)fieldsize);
			delete[] conv;
			}
			break;

		case TDS5_TYPE_LONGCHAR:
		case TDS5_TYPE_LONGBINARY:
			{
			// varint 5 - a 32-bit length rather than an 8-bit
			// one, and no text pointer.  pretds7typemap[] never
			// produces these today; they're here so that a wider
			// column promoted to one later writes a field the
			// client can walk past.
			bool		hextext=(tds5type==TDS5_TYPE_LONGBINARY &&
							binaryishextext);

			// longchar is character data, longbinary isn't - the
			// same split text and image make above
			size_t		convsize=0;
			char		*conv=(tds5type==TDS5_TYPE_LONGCHAR)?
						utf8ToClientCharset(field,
						(size_t)fieldsize,&convsize):
						NULL;
			const char	*data=(conv)?conv:field;
			uint64_t	size=(conv)?convsize:
						((hextext)?fieldsize/2:
								fieldsize);
			if (size>colsize) {
				size=colsize;
			}
			write(&resppacket,(uint32_t)size);
			binary(data,size,hextext);
			debugWrite("size: %lld",(long long)size);
			delete[] conv;
			}
			break;

		default:
			// Nothing else comes out of pretds7typemap[].  Write
			// the null form rather than nothing at all, so a type
			// added to the map without a case here costs one
			// value rather than the whole stream.
			debugWrite("unhandled type - writing null");
			preTds7Field(coltype,tds5type,colsize,precision,
						field,fieldsize,true);
			break;
	}

	debugEnd();
}

void sqlrprotocol_tds::binary(const char *field, uint64_t size, bool hextext) {
	if (hextext) {
		const char	*f=field;
		for (uint64_t i=0; i<size; i++) {
			write(&resppacket,charsToHex(f));
			f+=2;
		}
	} else {
		write(&resppacket,field,size);
	}
}

// Refuses a malformed or unsupported paramfmt/params pair, with its own
// done, so the client sees the command fail rather than being left
// waiting for a result that never comes.  Class 16 for the same reason
// preTds7UnsupportedToken() uses it - the session stays usable.
//
// "more" is whether another done still follows this one.  A refusal on
// the way in ends the walk, so nothing follows and the done is final;
// one on the way out is in the middle of a reply whose caller still
// appends its own closing done, and ct-lib stops reading at the first
// done without DONE_MORE, so that one would be left in the socket.
void sqlrprotocol_tds::preTds7ParamError(const char *msgtext, bool more) {
	// FIXME: is there a real error number/state for this?
	appendError(0,1,16,msgtext,srvname,NULL,1);
	done(DONE_ERROR|((more)?DONE_MORE:DONE_FINAL),transState(),0);
}

// Reads a tds 5.0 paramfmt into the retained format array.  The token
// byte has already been read.  Returns false with a message in "err" if
// the token can't be walked; preTds7ParamFmt() turns that into the
// refusal, so that every bail-out here is one line rather than five.
//
// "wide" picks the paramfmt2 (0x20) shape over the paramfmt (0xEC) one.
// They differ in exactly two fields - a 32-bit token length rather than
// a 16-bit one, and a 32-bit status rather than an 8-bit one - so one
// function reads both.  Paramfmt2 is NOT rowfmt2's shape: rowfmt2 also
// prepends label, catalog, schema and table names to every column.
//
// A real ase only sends the wide form to a client that echoed the wide
// tables request capability.  capability() never grants it - it's bit 59
// in freetds's numbering and 60 in cspublic's, and its table leaves both
// out - so what actually arrives here is the narrow form; the wide one is
// read anyway because nothing stops a client from sending it.
//
// The token is:
//	uint16/uint32	how much follows, counting the parameter count
//			but not the token byte or the length itself
//	uint16		parameter count
//	then per parameter:
//	byte		name length
//	bytes		name, single-byte characters, often absent
//	byte/uint32	status
//	uint32		usertype
//	byte		datatype
//	...		size, unless the type's varint class is 0
//	byte,byte	precision and scale, decimal and numeric only
//	byte		locale length
//	bytes		locale
//
// Unlike a rowfmt, there's no table name after a text or image size -
// verified against a real ase's paramfmt and against the wireshark
// dissector.
bool sqlrprotocol_tds::preTds7ParamFmtRead(const byte_t **rpinout,
						size_t *rpsizeinout,
						bool wide,
						const char **err) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	*err="Malformed TDS 5.0 paramfmt token";

	pretds7paramfmtcount=0;
	pretds7paramfmtpool.clear();

	// the token length
	size_t	lensize=(wide)?sizeof(uint32_t):sizeof(uint16_t);
	if (rpsize<lensize) {
		return false;
	}
	uint32_t	tokenlength=0;
	if (wide) {
		read(rp,&tokenlength,&rp);
	} else {
		uint16_t	narrowlength=0;
		read(rp,&narrowlength,&rp);
		tokenlength=narrowlength;
	}
	rpsize-=lensize;

	debugWrite("token length: %lld",(long long)tokenlength);

	// The length covers the parameter count, so a token without room
	// for one is malformed, as is one that runs off the end of the
	// buffer.
	if (tokenlength<sizeof(uint16_t) || (size_t)tokenlength>rpsize) {
		return false;
	}

	// Everything below is bounded by "left" rather than by rpsize, so
	// that a paramfmt claiming more parameters than it carries can't
	// read past its own end into whatever token follows it.
	size_t	left=(size_t)tokenlength;

	uint16_t	count=0;
	read(rp,&count,&rp);
	left-=sizeof(uint16_t);

	debugWrite("count: %d",count);

	// the retained array is maxbindcount long, and nothing downstream
	// can bind more than that anyway
	if (count>maxbindcount) {
		*err="Too many TDS 5.0 parameters.";
		return false;
	}

	for (uint16_t i=0; i<count; i++) {

		tds5paramfmt	*fmt=&(pretds7paramfmts[i]);

		debugStart("pre-tds7 param fmt %d",i);

		// name.  single-byte characters, not ucs-2, and frequently
		// absent - every parameter of a dynamic execute has a name
		// length of 0.
		if (!left) {
			debugEnd();
			return false;
		}
		byte_t	namelen=0;
		read(rp,&namelen,&rp);
		left--;
		if (left<namelen) {
			debugEnd();
			return false;
		}
		char	*name=(char *)pretds7paramfmtpool.allocate(namelen+1);
		if (namelen) {
			read(rp,name,namelen,&rp);
			left-=namelen;
		}
		name[namelen]='\0';
		fmt->name=name;
		fmt->namesize=namelen;

		debugWrite("namelen: %d",namelen);
		debugWrite("name: %s",name);

		// status
		size_t	statussize=(wide)?sizeof(uint32_t):sizeof(byte_t);
		if (left<statussize) {
			debugEnd();
			return false;
		}
		uint32_t	status=0;
		if (wide) {
			read(rp,&status,&rp);
		} else {
			byte_t	narrowstatus=0;
			read(rp,&narrowstatus,&rp);
			status=narrowstatus;
		}
		left-=statussize;
		fmt->status=(byte_t)status;

		debugWrite("status: 0x%02x",status);
		debugWrite("return: %d",
				(status&TDS5_PARAM_RETURN)?1:0);

		// A columnstatus byte in front of every value needs the
		// columnstatus request capability, which capability() never
		// grants - it's bit 58 in freetds's numbering and 57 in
		// cspublic's, and its table leaves both out - so no client
		// should be asking for one.  Refuse rather than mis-size
		// every value behind it if one does.
		if (status&TDS5_PARAM_COLUMNSTATUS) {
			*err="TDS 5.0 parameter column status "
						"is not supported yet.";
			debugEnd();
			return false;
		}

		// usertype
		if (left<sizeof(uint32_t)) {
			debugEnd();
			return false;
		}
		read(rp,&(fmt->usertype),&rp);
		left-=sizeof(uint32_t);

		debugWrite("usertype: %d",fmt->usertype);

		// datatype
		if (!left) {
			debugEnd();
			return false;
		}
		read(rp,&(fmt->tds5type),&rp);
		left--;

		debugPreTds7ColumnType(fmt->tds5type);

		// A type with no ms-tds counterpart can't be bound, and a
		// serialized object (0x24) carries a class id after its
		// locale that nothing here knows how to skip either.
		fmt->mstype=tds5TypeToMsType(fmt->tds5type);
		if (!fmt->mstype) {
			*err="That TDS 5.0 datatype is not supported yet.";
			debugEnd();
			return false;
		}
		debugColumnType(fmt->mstype);

		fmt->varintsize=preTds7VarintSize(fmt->tds5type);
		fmt->size=0;
		fmt->precision=0;
		fmt->scale=0;

		// size
		switch (fmt->varintsize) {
			case 0:
				// the type carries its own width, so there's
				// no size field to read
				fmt->size=preTds7FixedSize(fmt->tds5type);
				break;
			case 4:
			case 5:
				if (left<sizeof(uint32_t)) {
					debugEnd();
					return false;
				}
				read(rp,&(fmt->size),&rp);
				left-=sizeof(uint32_t);
				break;
			default:
				{
				if (!left) {
					debugEnd();
					return false;
				}
				byte_t	size=0;
				read(rp,&size,&rp);
				left--;
				fmt->size=size;
				}
				break;
		}

		debugWrite("size: %d",fmt->size);

		// precision and scale
		if (fmt->tds5type==TDS5_TYPE_DECN ||
				fmt->tds5type==TDS5_TYPE_NUMN) {

			if (left<2*sizeof(byte_t)) {
				debugEnd();
				return false;
			}
			read(rp,&(fmt->precision),&rp);
			read(rp,&(fmt->scale),&rp);
			left-=2*sizeof(byte_t);

			debugWrite("precision: %d",fmt->precision);
			debugWrite("scale: %d",fmt->scale);

			// A precision of 0, a precision past the maximum, or
			// a scale wider than the precision are all outside
			// what decimalSize() and bulkDecimal() can render -
			// preTds7DecimalInfo() clamps the same three on the
			// way out.  Note that a client may declare a
			// different precision and scale for the same
			// parameter on two executes of one prepared
			// statement; a real ct-lib client sends 9,2 for a
			// value and 18,0 for a null.
			if (!fmt->precision ||
				fmt->precision>TDS_DECIMAL_MAX_PRECISION ||
				fmt->scale>fmt->precision) {
				*err="Invalid TDS 5.0 decimal "
						"precision or scale.";
				debugEnd();
				return false;
			}
		}

		// locale.  Mandatory even when it's empty, the way it is in
		// a rowfmt - the next parameter starts right after it.
		if (!left) {
			debugEnd();
			return false;
		}
		byte_t	localelen=0;
		read(rp,&localelen,&rp);
		left--;
		if (left<localelen) {
			debugEnd();
			return false;
		}
		rp+=localelen;
		left-=localelen;

		debugWrite("locale length: %d",localelen);

		pretds7paramfmtcount++;

		debugEnd();
	}

	// A real client's blocks add up to the length exactly.  Anything
	// left over means the two disagree, and then there's no telling
	// where the next token starts.
	if (left) {
		debugWrite("%lld bytes left over",(long long)left);
		return false;
	}

	rpsize-=(size_t)tokenlength;

	return true;
}

bool sqlrprotocol_tds::preTds7ParamFmt(const byte_t **rpinout,
						size_t *rpsizeinout,
						bool wide) {

	debugStart("pre-tds7 param fmt");

	const char	*err=NULL;
	if (preTds7ParamFmtRead(rpinout,rpsizeinout,wide,&err)) {
		debugWrite("param count: %d",pretds7paramfmtcount);
		debugEnd();
		return true;
	}

	debugWrite("%s",err);
	debugEnd();

	// A params token behind a paramfmt that couldn't be walked can't be
	// walked either - it carries no length of its own - so nothing
	// after this point in the buffer can be trusted.
	*rpsizeinout=0;
	pretds7paramfmtcount=0;
	preTds7ParamError(err,false);
	return false;
}

// Reads a tds 5.0 params token, replaying the paramfmt in front of it to
// size each value.  The token byte has already been read.
//
// The results go into the rpcparams[] family, which is wire-neutral -
// bindParams() and everything above it read them the same way whichever
// dialect they arrived in.  rpcparamtdstypes[] gets the ms-tds
// equivalent of each type and rpcparamtds5types[] the raw byte; see
// tds5TypeToMsType() for why.
//
// Split the way preTds7ParamFmtRead() and preTds7ParamFmt() are - this
// is the walk, and the wrapper turns a failed walk into the refusal.
// preTds7SkipCommand() needs the walk without the refusal; it appends
// its own.
bool sqlrprotocol_tds::preTds7ParamsRead(const byte_t **rpinout,
						size_t *rpsizeinout) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	debugStart("pre-tds7 params");
	debugWrite("count: %d",pretds7paramfmtcount);

	// reset the pool that parameter values get copied into, the way the
	// ms-tds params() does
	rpcparampool.clear();
	rpcparamcount=0;

	// The loop is bounded by the format array rather than by what's
	// left in the buffer, so a value that consumed nothing can't spin
	// it the way params() has to guard against.
	for (uint16_t i=0; i<pretds7paramfmtcount; i++) {

		const tds5paramfmt	*fmt=&(pretds7paramfmts[i]);

		debugStart("pre-tds7 param %d",i);
		debugWrite("name: %s",fmt->name);
		debugPreTds7ColumnType(fmt->tds5type);

		sqlrserverbindvar	*bv=&(rpcparams[i]);
		bv->type=SQLRSERVERBINDVARTYPE_NULL;
		bv->variable=NULL;
		bv->variablesize=0;
		bv->valuesize=0;
		bv->value.stringval=NULL;
		bv->isnull=cont->getNullBindValue();

		// A binary or image parameter stays a lob even when the
		// value turns out to be null, the way bulkField() keeps it
		// one, so it doesn't lose its lob-ness before it's ever
		// bound.
		switch (fmt->tds5type) {
			case TDS5_TYPE_BINARY:
			case TDS5_TYPE_VARBINARY:
			case TDS5_TYPE_LONGBINARY:
			case TDS5_TYPE_IMAGE:
				bv->type=SQLRSERVERBINDVARTYPE_NULLBLOB;
				break;
		}

		if (!preTds7ParamValueRead(&rp,&rpsize,fmt,bv)) {
			debugEnd();
			debugEnd();
			*rpinout=rp;
			return false;
		}

		// the name, the direction and the declared type, which the
		// paramfmt carried rather than the value
		rpcparambyref[i]=((fmt->status&TDS5_PARAM_RETURN)!=0);
		rpcparamnames[i]=(char *)rpcparampool.allocate(
							fmt->namesize+1);
		if (fmt->namesize) {
			charstring::copy(rpcparamnames[i],
						fmt->name,fmt->namesize);
		}
		rpcparamnames[i][fmt->namesize]='\0';
		rpcparamnamesizes[i]=fmt->namesize;
		rpcparamtdstypes[i]=fmt->mstype;
		rpcparamtds5types[i]=fmt->tds5type;
		rpcparammaxsizes[i]=fmt->size;
		rpcparamprecisions[i]=fmt->precision;
		rpcparamscales[i]=fmt->scale;

		rpcparamcount++;

		debugEnd();
	}

	*rpinout=rp;
	*rpsizeinout=rpsize;

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::preTds7Params(const byte_t **rpinout,
						size_t *rpsizeinout) {

	if (preTds7ParamsRead(rpinout,rpsizeinout)) {
		return true;
	}

	// A params token carries no length of its own, so a walk that ran
	// aground in the middle of one leaves no way to find the token
	// behind it either.
	*rpsizeinout=0;
	rpcparamcount=0;
	preTds7ParamError("Malformed TDS 5.0 params token",false);
	return false;
}

// Reads the paramfmt/params pair that a command token declaring
// parameters carries behind it.  Returns false, having appended its own
// error and final done, if either token is missing or can't be walked.
bool sqlrprotocol_tds::preTds7ParamFmtAndParams(const byte_t **rpinout,
						size_t *rpsizeinout) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	// the paramfmt
	byte_t	fmttoken=0;
	if (rpsize) {
		read(rp,&fmttoken,&rp);
		rpsize--;
	}
	if (fmttoken!=TDS5_TOKEN_PARAMFMT && fmttoken!=TDS5_TOKEN_PARAMFMT2) {
		*rpinout=rp;
		*rpsizeinout=0;
		pretds7paramfmtcount=0;
		rpcparamcount=0;
		preTds7ParamError("Missing TDS 5.0 paramfmt token",false);
		return false;
	}
	if (!preTds7ParamFmt(&rp,&rpsize,(fmttoken==TDS5_TOKEN_PARAMFMT2))) {
		*rpinout=rp;
		*rpsizeinout=0;
		rpcparamcount=0;
		return false;
	}

	// the params
	byte_t	paramstoken=0;
	if (rpsize) {
		read(rp,&paramstoken,&rp);
		rpsize--;
	}
	if (paramstoken!=TDS5_TOKEN_PARAMS) {
		*rpinout=rp;
		*rpsizeinout=0;
		pretds7paramfmtcount=0;
		rpcparamcount=0;
		preTds7ParamError("Missing TDS 5.0 params token",false);
		return false;
	}
	if (!preTds7Params(&rp,&rpsize)) {
		*rpinout=rp;
		*rpsizeinout=0;
		return false;
	}

	*rpinout=rp;
	*rpsizeinout=rpsize;

	return true;
}

// Reads one parameter's value out of a params token.  A params token
// carries no lengths of its own, so how many bytes a value occupies
// comes entirely from the format the paramfmt declared:
//
//	varint 0	the value alone, at the type's own width.  there's
//			no length field, so there's no way to say null
//	varint 1	one length byte, then that many bytes.  a length
//			of 0 means null
//	varint 4	a text pointer, a timestamp and a 32-bit length for
//			the blob types; a bare 32-bit length otherwise
//	varint 5	a 32-bit length
//
// The declared size is not the value's size - a client and a server can
// declare the same decimal(9,2) at 33 and at 5 - so every value is sized
// from the length that arrived with it, exactly as preTds7Field() writes
// one.  This is that function inverted.
bool sqlrprotocol_tds::preTds7ParamValueRead(const byte_t **rpinout,
						size_t *rpsizeinout,
						const tds5paramfmt *fmt,
						sqlrserverbindvar *bv) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	// A text or image value arrives behind a text pointer and a
	// timestamp, the way a row field does, and a text-pointer length of
	// 0 is how it says null.  Xml and unitext are varint 4 too but
	// aren't blob types, so they carry no text pointer -
	// preTds7TypeInfo() draws the same line writing a rowfmt.
	if (fmt->tds5type==TDS5_TYPE_TEXT || fmt->tds5type==TDS5_TYPE_IMAGE) {
		if (!rpsize) {
			return false;
		}
		byte_t	textptrsize=0;
		read(rp,&textptrsize,&rp);
		rpsize--;
		if (!textptrsize) {
			debugWrite("value: (null)");
			return true;
		}
		// the text pointer, then an 8-byte timestamp
		if (rpsize<(size_t)textptrsize+8) {
			return false;
		}
		rp+=(size_t)textptrsize+8;
		rpsize-=(size_t)textptrsize+8;
	}

	// the length
	uint32_t	size=0;
	switch (fmt->varintsize) {
		case 0:
			size=preTds7FixedSize(fmt->tds5type);
			break;
		case 4:
		case 5:
			if (rpsize<sizeof(uint32_t)) {
				return false;
			}
			read(rp,&size,&rp);
			rpsize-=sizeof(uint32_t);
			break;
		default:
			{
			if (!rpsize) {
				return false;
			}
			byte_t	varint1size=0;
			read(rp,&varint1size,&rp);
			rpsize--;
			size=varint1size;
			}
			break;
	}

	if (rpsize<size) {
		return false;
	}

	debugWrite("size: %d",size);

	// A length of 0 is null for every type that has a length at all.
	// It is not gated on TDS5_PARAM_NULLALLOWED: a real ct-lib client
	// leaves that bit clear even for a parameter it sends a null in,
	// even when the parameter was declared CS_CANBENULL.
	if (fmt->varintsize && !size) {
		debugWrite("value: (null)");
		return true;
	}

	const byte_t	*value=rp;
	rp+=size;
	rpsize-=size;

	// the integer types, signed and unsigned, at every width one of
	// them can be.  sign-extended from whatever width arrived, and in
	// the byte order the login record declared.
	bool	isint=false;
	bool	issigned=true;
	switch (fmt->tds5type) {
		case TDS5_TYPE_UINT1:
		case TDS5_TYPE_UINT2:
		case TDS5_TYPE_UINT4:
		case TDS5_TYPE_UINT8:
		case TDS5_TYPE_UINTN:
		case TDS5_TYPE_INT1:
			// tds 5.0's int1 is the unsigned one - sint1 is the
			// signed 1-byte type
			issigned=false;
			// fall through
		case TDS5_TYPE_INT2:
		case TDS5_TYPE_INT4:
		case TDS5_TYPE_INT8:
		case TDS5_TYPE_INTN:
		case TDS5_TYPE_SINT1:
		case TDS5_TYPE_BIT:
			isint=true;
			break;
	}
	if (isint) {

		if (size<1 || size>8) {
			debugWrite("invalid size: %d",size);
			return false;
		}

		// The width here can be anything from 1 to 8, so this
		// assembles the value a byte at a time rather than going
		// through read(), which only has the four standard widths.
		// Which end of the field the high byte sits at is what the
		// login record declared, and the sign bit lives in it.
		uint64_t	magnitude=0;
		byte_t		msb=0;
		if (getProtocolIsBigEndian()) {
			for (byte_t i=0; i<size; i++) {
				magnitude=(magnitude<<8)|
						((uint64_t)value[i]);
			}
			msb=value[0];
		} else {
			for (byte_t i=0; i<size; i++) {
				magnitude|=((uint64_t)value[i])<<(i*8);
			}
			msb=value[size-1];
		}
		if (issigned && size<8 && (msb&0x80)) {
			magnitude|=~((uint64_t)0)<<(size*8);
		}

		bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
		bv->valuesize=size;
		bv->isnull=cont->getNonNullBindValue();
		bv->value.integerval=(int64_t)magnitude;

		debugWrite("value: %lld",(long long)bv->value.integerval);
		return true;
	}

	switch (fmt->tds5type) {

		case TDS5_TYPE_VOID:
			// no width and no value - null is all it can mean
			debugWrite("value: (null)");
			break;

		case TDS5_TYPE_FLT4:
		case TDS5_TYPE_FLT8:
		case TDS5_TYPE_FLTN:
			{
			if (size!=sizeof(float) && size!=sizeof(double)) {
				debugWrite("invalid size: %d",size);
				return false;
			}
			// An ieee float is as byte-order-sensitive as an
			// integer of the same width is, but read() has no
			// float overload that swaps - see the note in
			// preTds7Field() - so the bits come in as an
			// integer and get copied over a float afterward.
			const byte_t	*vp=value;
			double		data=0.0;
			if (size==sizeof(float)) {
				uint32_t	bits=0;
				read(vp,&bits,&vp);
				float		f=0.0;
				bytestring::copy(&f,&bits,sizeof(f));
				data=f;
			} else {
				uint64_t	bits=0;
				read(vp,&bits,&vp);
				bytestring::copy(&data,&bits,sizeof(data));
			}
			bulkDouble(bv,data);
			}
			break;

		case TDS5_TYPE_MONEY:
		case TDS5_TYPE_SHORTMONEY:
		case TDS5_TYPE_MONEYN:
			{
			// A 4-byte money is one signed count of
			// ten-thousandths; an 8-byte one is two, the high
			// half first.  That ordering is the type's own, not
			// a byte order - each half arrives in the order the
			// login declared.
			if (size!=4 && size!=8) {
				debugWrite("invalid size: %d",size);
				return false;
			}
			const byte_t	*vp=value;
			int64_t		tenthousandths=0;
			if (size==4) {
				uint32_t	low=0;
				read(vp,&low,&vp);
				tenthousandths=(int32_t)low;
			} else {
				uint32_t	high=0;
				uint32_t	low=0;
				read(vp,&high,&vp);
				read(vp,&low,&vp);
				tenthousandths=(int64_t)
					((((uint64_t)high)<<32)|low);
			}
			moneyValue(tenthousandths,bv);
			}
			break;

		case TDS5_TYPE_DATETIME:
		case TDS5_TYPE_SHORTDATE:
		case TDS5_TYPE_DATETIMEN:
			{
			// 8 bytes is days since 1900 and three-hundredths of
			// a second since midnight; 4 is days and whole
			// minutes, both unsigned
			if (size!=4 && size!=8) {
				debugWrite("invalid size: %d",size);
				return false;
			}
			const byte_t	*vp=value;
			int32_t		dayssince1900=0;
			uint32_t	threehundredths=0;
			if (size==4) {
				uint16_t	days=0;
				uint16_t	minutes=0;
				read(vp,&days,&vp);
				read(vp,&minutes,&vp);
				dayssince1900=days;
				threehundredths=((uint32_t)minutes)*60*300;
			} else {
				uint32_t	days=0;
				read(vp,&days,&vp);
				read(vp,&threehundredths,&vp);
				dayssince1900=(int32_t)days;
			}
			dateTimeValue(dayssince1900,threehundredths,bv);
			}
			break;

		case TDS5_TYPE_DECN:
		case TDS5_TYPE_NUMN:
			{
			// Two things are inverted from the ms-tds form that
			// bulkDecimal() reads, and preTds7Field() inverts
			// the same two writing one out: the sign byte is 0
			// for positive rather than 1, and the magnitude is
			// big-endian rather than little-endian.
			byte_t	magsize=(byte_t)(size-1);
			if (magsize>TDS_DECIMAL_MAX_SIZE-1) {
				debugWrite("invalid size: %d",size);
				return false;
			}
			byte_t	magnitude[TDS_DECIMAL_MAX_SIZE-1];
			bytestring::zero(magnitude,sizeof(magnitude));
			for (byte_t i=0; i<magsize; i++) {
				magnitude[i]=value[size-1-i];
			}

			stringbuffer	strb;
			// FIXME: anything wider than 8 bytes overflows, the
			// same way it does in paramValue()
			bulkDecimal((value[0])?0:1,magnitude,
					(magsize>8)?8:magsize,
					fmt->scale,&strb);

			// bound as a number rather than a string - ase
			// refuses to convert a varchar to a decimal
			// ("Implicit conversion from datatype 'VARCHAR' to
			// 'DECIMAL' is not allowed")
			bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.doubleval.value=
				(double)charstring::convertToFloat(
							strb.getString());
			// FIXME: kludgy, but the same thing bulkDouble() does
			bv->value.doubleval.precision=
				(uint32_t)charstring::getLength(
						strb.getString())-
				((charstring::contains(
					strb.getString(),'-'))?1:0)-
				((charstring::contains(
					strb.getString(),'.'))?1:0);
			bv->value.doubleval.scale=fmt->scale;
			debugWrite("value: %s",strb.getString());
			}
			break;

		case TDS5_TYPE_VARCHAR:
		case TDS5_TYPE_CHAR:
		case TDS5_TYPE_LONGCHAR:
		case TDS5_TYPE_TEXT:
		case TDS5_TYPE_XML:
			{
			// Character data arrives in the charset the login
			// record declared - a tds 5.0 paramfmt has no
			// collation field of its own, so that's the only
			// thing naming one - and preTds7ToUtf8() converts it
			// to the utf-8 the rest of this module works in, or
			// passes it through when the client declared utf8 or
			// something pretds7charsets[] doesn't cover.
			// preTds7Field() writes it back the same way.
			//
			// A char is not blank padded out to its declared
			// size here the way a bigchar is on the ms-tds path:
			// the declared size isn't a width in tds 5.0, and a
			// client that wants the padding sends it.
			size_t	value8size=0;
			char	*value8=preTds7ToUtf8(value,size,&value8size);
			bulkString(bv,&rpcparampool,value8,value8size);
			delete[] value8;
			}
			break;

		case TDS5_TYPE_UNITEXT:
			{
			// utf-16, so it needs converting rather than copying.
			// the copy is because the value isn't necessarily
			// aligned for a ucs2_t read.
			size_t	length=size/sizeof(ucs2_t);
			ucs2_t	*value16=new ucs2_t[length+1];
			bytestring::copy(value16,value,
						length*sizeof(ucs2_t));
			value16[length]=0;
			size_t	value8size=0;
			char	*value8=ucs2ToUtf8(value16,length,&value8size);
			bulkString(bv,&rpcparampool,value8,value8size);
			delete[] value8;
			delete[] value16;
			}
			break;

		case TDS5_TYPE_VARBINARY:
		case TDS5_TYPE_BINARY:
		case TDS5_TYPE_LONGBINARY:
		case TDS5_TYPE_IMAGE:
			bulkBinary(bv,&rpcparampool,value,size);
			break;

		case TDS5_TYPE_DATE:
		case TDS5_TYPE_DATEN:
		case TDS5_TYPE_TIME:
		case TDS5_TYPE_TIMEN:
			// FIXME: actually implement these.  A date is a
			// signed count of days since 1900-01-01 and a time
			// an unsigned count of three-hundredths of a second
			// since midnight, but neither has ever been seen on
			// the wire, and paramValue() leaves the ms-tds
			// versions unimplemented too.  The value has already
			// been stepped over, so the parameter just stays
			// null.
			debugWrite("unimplemented type - leaving null");
			break;

		default:
			// Nothing else survives tds5TypeToMsType().  Leave
			// the parameter null rather than guess, the way
			// preTds7Field() writes a null for a type it has no
			// case for; the value has already been stepped over,
			// so the rest of the token still parses.
			debugWrite("unhandled type - leaving null");
			break;
	}

	return true;
}

// Writes a tds 5.0 paramfmt describing "count" parameters.  The caller
// fills in each parameter's name, status, usertype, datatype, size and -
// for a decimal or numeric - precision and scale; everything else is
// derived from the datatype here, so that this and preTds7ParamsWrite()
// can't disagree about a parameter's shape.
//
// The narrow token (0xEC) rather than paramfmt2 (0x20), because that's
// what a real ase sends a client that didn't ask for wide tables, and
// capability() doesn't offer them.
//
// The declared size is not the width the values have to be written at -
// a real ase declares a decimal(9,2) at 5 where a ct-lib client declares
// the same parameter at 33 - so nothing here caps what
// preTds7ParamsWrite() puts on the wire.
bool sqlrprotocol_tds::preTds7ParamFmtWrite(const tds5paramfmt *fmts,
						uint16_t count) {

	byte_t	token=TDS5_TOKEN_PARAMFMT;

	debugStart("pre-tds7 param fmt write");
	debugPreTds7TokenType(token);
	debugWrite("count: %d",count);

	// The token length counts bytes that aren't written yet, and every
	// parameter block is a different size, so build the blocks into a
	// scratch buffer and measure them - the same reason preTds7RowFmt()
	// does it that way.
	bytebuffer	params;

	for (uint16_t i=0; i<count; i++) {

		const tds5paramfmt	*fmt=&(fmts[i]);

		debugStart("pre-tds7 param fmt %d",i);

		// name.  single-byte characters, and the length is a single
		// byte, so a longer name is truncated the way
		// preTds7RowFmt() truncates a column name.
		size_t	namelen=fmt->namesize;
		if (namelen>255) {
			namelen=255;
		}
		write(&params,(byte_t)namelen);
		if (namelen) {
			write(&params,fmt->name,namelen);
		}
		debugWrite("namelen: %lld",(long long)namelen);
		debugWrite("name: %s",fmt->name);

		// status.  one byte in this token, four in a paramfmt2.
		write(&params,fmt->status);
		debugWrite("status: 0x%02x",fmt->status);

		// usertype.  0 means "no alias type", which is what
		// preTds7RowFmt() sends for a column.
		write(&params,fmt->usertype);
		debugWrite("usertype: %d",fmt->usertype);

		// datatype
		write(&params,fmt->tds5type);
		debugPreTds7ColumnType(fmt->tds5type);

		// size.  no table name after a blob's size, unlike a rowfmt.
		byte_t	varintsize=preTds7VarintSize(fmt->tds5type);
		switch (varintsize) {
			case 0:
				debugWrite("fixed, no size");
				break;
			case 4:
			case 5:
				write(&params,fmt->size);
				debugWrite("size: %d (32-bit)",fmt->size);
				break;
			default:
				{
				byte_t	size=(fmt->size>255)?
							255:(byte_t)fmt->size;
				write(&params,size);
				debugWrite("size: %d (8-bit)",size);
				}
				break;
		}

		// precision and scale
		if (fmt->tds5type==TDS5_TYPE_DECN ||
				fmt->tds5type==TDS5_TYPE_NUMN) {
			write(&params,fmt->precision);
			write(&params,fmt->scale);
			debugWrite("precision: %d",fmt->precision);
			debugWrite("scale: %d",fmt->scale);
		}

		// locale.  Mandatory even when it's empty - the client reads
		// it right after the type info, so leaving it out
		// desynchronizes everything after this parameter.
		write(&params,(byte_t)0);
		debugWrite("locale length: 0");

		debugEnd();
	}

	// the length covers the parameter count too, not just the blocks
	size_t	tokenlength=sizeof(uint16_t)+params.getSize();

	// Refuse rather than truncate.  A truncated paramfmt isn't a
	// smaller set of parameters, it's a stream the client can't parse
	// at all, and the params token behind it has no length of its own
	// to recover from.  Class 16 for the same reason preTds7RowFmt()
	// uses it when a result set won't fit in a rowfmt.
	if (count>maxbindcount || tokenlength>65535) {
		debugWrite("token too large: %lld",(long long)tokenlength);
		debugEnd();
		preTds7ParamError("Too many TDS 5.0 parameters to send.",true);
		return false;
	}

	write(&resppacket,token);
	write(&resppacket,(uint16_t)tokenlength);
	write(&resppacket,(uint16_t)count);
	write(&resppacket,params.getBuffer(),params.getSize());

	debugWrite("token length: %lld",(long long)tokenlength);
	debugEnd();

	return true;
}

// Writes a tds 5.0 params token carrying "count" values, described by
// the same format array the paramfmt in front of them was written from.
// The token has no length field at all - it can only be parsed by
// replaying that paramfmt - so there's nothing to measure here.
bool sqlrprotocol_tds::preTds7ParamsWrite(const tds5paramfmt *fmts,
						sqlrserverbindvar *bvs,
						uint16_t count) {

	byte_t	token=TDS5_TOKEN_PARAMS;

	debugStart("pre-tds7 params write");
	debugPreTds7TokenType(token);
	debugWrite("count: %d",count);

	if (count>maxbindcount) {
		debugWrite("too many parameters: %d",count);
		debugEnd();
		preTds7ParamError("Too many TDS 5.0 parameters to send.",true);
		return false;
	}

	write(&resppacket,token);

	for (uint16_t i=0; i<count; i++) {
		preTds7ParamValueWrite(&(fmts[i]),&(bvs[i]));
	}

	debugEnd();

	return true;
}

// Writes one parameter's value into a params token.
//
// This is preTds7Field() sourced from a bind variable rather than from a
// result-set field, and it isn't that function for two reasons: a bind
// holds a value in whatever form the back end put there rather than
// always as text, and it holds binary as raw bytes rather than as the
// hex text the ct-lib back ends render a binary column as.
void sqlrprotocol_tds::preTds7ParamValueWrite(const tds5paramfmt *fmt,
						sqlrserverbindvar *bv) {

	debugStart("pre-tds7 param value write");
	debugPreTds7ColumnType(fmt->tds5type);

	// The bind's value, kept in every form the types below need it in.
	// The text rendering is what the date/time, money and decimal
	// writers parse, the same way preTds7Field() gets them.
	stringbuffer	strb;
	const char	*field=NULL;
	uint64_t	fieldsize=0;
	int64_t		intval=0;
	double		dblval=0.0;
	bool		null=false;

	switch (bv->type) {
		case SQLRSERVERBINDVARTYPE_INTEGER:
			intval=bv->value.integerval;
			dblval=(double)intval;
			strb.append(intval);
			break;
		case SQLRSERVERBINDVARTYPE_DOUBLE:
			dblval=bv->value.doubleval.value;
			intval=(int64_t)dblval;
			strb.append(dblval,
					bv->value.doubleval.precision,
					bv->value.doubleval.scale);
			break;
		case SQLRSERVERBINDVARTYPE_DATE:
			{
			char	buffer[48];
			charstring::printf(buffer,sizeof(buffer),
					"%04d-%02d-%02d %02d:%02d:%02d.%06d",
					(int32_t)bv->value.dateval.year,
					(int32_t)bv->value.dateval.month,
					(int32_t)bv->value.dateval.day,
					(int32_t)bv->value.dateval.hour,
					(int32_t)bv->value.dateval.minute,
					(int32_t)bv->value.dateval.second,
					(int32_t)bv->value.dateval.microsecond);
			strb.append(buffer);
			}
			break;
		case SQLRSERVERBINDVARTYPE_STRING:
		case SQLRSERVERBINDVARTYPE_BLOB:
		case SQLRSERVERBINDVARTYPE_CLOB:
			field=bv->value.stringval;
			fieldsize=bv->valuesize;
			intval=charstring::convertToInteger(field);
			dblval=charstring::convertToFloat(field);
			break;
		default:
			// null, nullblob, nullclob, cursor, and everything
			// else that has no value to send
			null=true;
			break;
	}
	if (!field) {
		field=strb.getString();
		fieldsize=strb.getStringLength();
	}
	if (!field) {
		field="";
		fieldsize=0;
		null=true;
	}

	if (null) {
		preTds7ParamNullWrite(fmt);
		debugEnd();
		return;
	}

	switch (fmt->tds5type) {

		// the n-types: a size byte, then the value at that width.
		// the width comes from what the paramfmt declared, narrowed
		// to one the type allows.
		case TDS5_TYPE_INTN:
		case TDS5_TYPE_UINTN:
			{
			byte_t	size=(byte_t)fmt->size;
			if (size!=1 && size!=2 && size!=4 && size!=8) {
				size=sizeof(int64_t);
			}
			write(&resppacket,size);
			writeIntN(intval,size);
			debugWrite("size: %d",size);
			debugWrite("data: %lld",(long long)intval);
			}
			break;
		case TDS5_TYPE_FLTN:
			{
			byte_t	size=(fmt->size==sizeof(float))?
						sizeof(float):sizeof(double);
			write(&resppacket,size);
			writeFloatN(dblval,size);
			debugWrite("size: %d",size);
			debugWrite("data: %f",dblval);
			}
			break;
		case TDS5_TYPE_MONEYN:
			{
			byte_t	size=(fmt->size==4)?4:8;
			write(&resppacket,size);
			int64_t	data=moneyValue(field);
			if (size==4) {
				write(&resppacket,(uint32_t)(int32_t)data);
			} else {
				// The high half goes first, ahead of the low
				// half.  That ordering is the type's own,
				// not a byte order - each half goes out in
				// the order the login declared.
				write(&resppacket,(uint32_t)
					((data&0xFFFFFFFF00000000LL)>>32));
				write(&resppacket,(uint32_t)
					(data&0x00000000FFFFFFFFLL));
			}
			debugWrite("size: %d",size);
			debugWrite("data: %lld (%s)",(long long)data,field);
			}
			break;
		case TDS5_TYPE_DATETIMEN:
			{
			byte_t	size=(fmt->size==4)?4:8;
			write(&resppacket,size);
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			dateTime(field,&dayssince1900,&threehundredths);
			debugWrite("size: %d",size);
			if (size==4) {
				// days and whole minutes
				uint16_t	days=(dayssince1900>0)?
							dayssince1900:0;
				uint16_t	minutes=threehundredths/300/60;
				write(&resppacket,days);
				write(&resppacket,minutes);
				debugWrite("data: %d,%d",(uint32_t)days,
							(uint32_t)minutes);
			} else {
				// days and three-hundredths of a second
				write(&resppacket,(uint32_t)dayssince1900);
				write(&resppacket,threehundredths);
				debugWrite("data: %d,%d",dayssince1900,
							threehundredths);
			}
			}
			break;

		// varint 0 - the value alone, with no length in front of it
		case TDS5_TYPE_INT1:
		case TDS5_TYPE_UINT1:
		case TDS5_TYPE_SINT1:
		case TDS5_TYPE_BIT:
		case TDS5_TYPE_INT2:
		case TDS5_TYPE_UINT2:
		case TDS5_TYPE_INT4:
		case TDS5_TYPE_UINT4:
		case TDS5_TYPE_INT8:
		case TDS5_TYPE_UINT8:
			writeIntN(intval,preTds7FixedSize(fmt->tds5type));
			debugWrite("data: %lld",(long long)intval);
			break;
		case TDS5_TYPE_FLT4:
			writeFloatN(dblval,sizeof(float));
			debugWrite("data: %f",dblval);
			break;
		case TDS5_TYPE_FLT8:
			writeFloatN(dblval,sizeof(double));
			debugWrite("data: %f",dblval);
			break;
		case TDS5_TYPE_MONEY:
		case TDS5_TYPE_SHORTMONEY:
			{
			int64_t	data=moneyValue(field);
			if (fmt->tds5type==TDS5_TYPE_SHORTMONEY) {
				write(&resppacket,(uint32_t)(int32_t)data);
			} else {
				write(&resppacket,(uint32_t)
					((data&0xFFFFFFFF00000000LL)>>32));
				write(&resppacket,(uint32_t)
					(data&0x00000000FFFFFFFFLL));
			}
			debugWrite("data: %lld (%s)",(long long)data,field);
			}
			break;
		case TDS5_TYPE_DATETIME:
		case TDS5_TYPE_SHORTDATE:
			{
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			dateTime(field,&dayssince1900,&threehundredths);
			if (fmt->tds5type==TDS5_TYPE_SHORTDATE) {
				uint16_t	days=(dayssince1900>0)?
							dayssince1900:0;
				uint16_t	minutes=threehundredths/300/60;
				write(&resppacket,days);
				write(&resppacket,minutes);
				debugWrite("data: %d,%d",(uint32_t)days,
							(uint32_t)minutes);
			} else {
				write(&resppacket,(uint32_t)dayssince1900);
				write(&resppacket,threehundredths);
				debugWrite("data: %d,%d",dayssince1900,
							threehundredths);
			}
			}
			break;

		case TDS5_TYPE_DECN:
		case TDS5_TYPE_NUMN:
			{
			byte_t	ispositive;
			byte_t	size;
			byte_t	val[TDS_DECIMAL_MAX_SIZE-1];
			// decimal() only fills the low 4 or 8 bytes, so zero
			// the rest before reversing a wider window of it
			bytestring::zero(val,sizeof(val));
			decimal(field,&ispositive,&size,val);

			// the sign byte is 0 for positive, the opposite way
			// round from decimal()'s "ispositive", and the
			// magnitude is big-endian where decimal() builds it
			// little-endian.  the width is decimalSize() of the
			// declared precision, counting the sign byte, and
			// not decimal()'s "size", which follows the value's
			// own digit count.
			byte_t	wiresize=decimalSize(fmt->precision);
			write(&resppacket,wiresize);
			write(&resppacket,(byte_t)((ispositive)?0:1));
			for (byte_t i=0; i<wiresize-1; i++) {
				write(&resppacket,val[wiresize-2-i]);
			}

			debugWrite("size: %d",wiresize);
			debugWrite("sign: %d",(ispositive)?0:1);
			debugWrite("data: %s (precision %d)",
						field,fmt->precision);
			}
			break;

		case TDS5_TYPE_VARCHAR:
		case TDS5_TYPE_CHAR:
			{
			// Character data goes out in the charset the login
			// record declared, the way preTds7Field() writes it -
			// a tds 5.0 paramfmt has no collation field to
			// declare anything else.  The conversion runs before
			// the 255-byte cap, so the cap counts the bytes that
			// actually go on the wire.
			size_t		convsize=0;
			char		*conv=utf8ToClientCharset(field,
						(size_t)fieldsize,&convsize);
			const char	*data=(conv)?conv:field;
			uint64_t	size=(conv)?convsize:fieldsize;
			if (size>255) {
				size=255;
			}
			write(&resppacket,(byte_t)size);
			write(&resppacket,data,size);
			debugWrite("size: %lld",(long long)size);
			debugWrite("data: %.*s",(int)size,data);
			delete[] conv;
			}
			break;

		case TDS5_TYPE_VARBINARY:
		case TDS5_TYPE_BINARY:
			{
			// raw bytes, not the hex text preTds7Field() decodes
			// a binary column value from
			uint64_t	size=(fieldsize>255)?255:fieldsize;
			write(&resppacket,(byte_t)size);
			write(&resppacket,field,size);
			debugWrite("size: %lld",(long long)size);
			debugHexDump((byte_t *)field,size);
			}
			break;

		case TDS5_TYPE_TEXT:
		case TDS5_TYPE_IMAGE:
			{
			// a non-null blob value is a text pointer, a
			// timestamp, a 32-bit length and then the data.
			// lobData() writes the first two, and the tds 5.0
			// bytes for text and image are the same 0x23 and
			// 0x22 it switches on, so it takes one as-is.
			//
			// text is character data and goes out in the charset
			// the login record declared; image is bytes.
			size_t		convsize=0;
			char		*conv=(fmt->tds5type==TDS5_TYPE_TEXT)?
						utf8ToClientCharset(field,
						(size_t)fieldsize,&convsize):
						NULL;
			const char	*data=(conv)?conv:field;
			uint64_t	size=(conv)?convsize:fieldsize;
			lobData(fmt->tds5type);
			write(&resppacket,(uint32_t)size);
			write(&resppacket,data,size);
			debugWrite("size: %lld",(long long)size);
			debugHexDump((byte_t *)data,size);
			delete[] conv;
			}
			break;

		case TDS5_TYPE_LONGCHAR:
		case TDS5_TYPE_LONGBINARY:
		case TDS5_TYPE_XML:
			{
			// varint 4 or 5 with no text pointer - a 32-bit
			// length and then the data.  longchar and xml are
			// character data, longbinary isn't.
			size_t		convsize=0;
			char		*conv=(fmt->tds5type!=
						TDS5_TYPE_LONGBINARY)?
						utf8ToClientCharset(field,
						(size_t)fieldsize,&convsize):
						NULL;
			const char	*data=(conv)?conv:field;
			uint64_t	size=(conv)?convsize:fieldsize;
			write(&resppacket,(uint32_t)size);
			write(&resppacket,data,size);
			debugWrite("size: %lld",(long long)size);
			delete[] conv;
			}
			break;

		case TDS5_TYPE_UNITEXT:
			{
			// utf-16, to match the type the paramfmt ahead of
			// this value declared - varint 4 or 5 with no text
			// pointer, a 32-bit length in bytes and then the data
			size_t	field16length;
			ucs2_t	*field16=utf8ToUcs2(field,fieldsize,
							&field16length);
			write(&resppacket,
				(uint32_t)(field16length*sizeof(ucs2_t)));
			write(&resppacket,field16,field16length);
			delete[] field16;
			debugWrite("size: %lld",
				(long long)(field16length*sizeof(ucs2_t)));
			}
			break;

		default:
			// Void, and the date and time types the reader
			// doesn't decode either.  Write the null form rather
			// than nothing at all, so a type added later without
			// a case here costs one value rather than the whole
			// token.
			// FIXME: implement date and time, both here and in
			// preTds7ParamValueRead()
			debugWrite("unhandled type - writing null");
			preTds7ParamNullWrite(fmt);
			break;
	}

	debugEnd();
}

// A parameter's null form, which is decided entirely by its type's
// varint class.  preTds7Field() writes the same shapes for a row field.
void sqlrprotocol_tds::preTds7ParamNullWrite(const tds5paramfmt *fmt) {

	debugWrite("data: null");

	switch (preTds7VarintSize(fmt->tds5type)) {
		case 0:
			{
			// A fixed-length type has no null form at all -
			// there's no length field to set to zero - so a null
			// goes out as a zero value at the type's own width.
			byte_t	zero[8];
			bytestring::zero(zero,sizeof(zero));
			write(&resppacket,zero,
					preTds7FixedSize(fmt->tds5type));
			}
			break;
		case 4:
			if (fmt->tds5type==TDS5_TYPE_TEXT ||
					fmt->tds5type==TDS5_TYPE_IMAGE) {
				// a text-pointer length of 0, and nothing
				// after it
				write(&resppacket,(byte_t)0);
			} else {
				// xml and unitext are varint 4 but carry no
				// text pointer, so their null is a length
				// of 0
				write(&resppacket,(uint32_t)0);
			}
			break;
		case 5:
			write(&resppacket,(uint32_t)0);
			break;
		default:
			// A length of 0 is a varint-1 type's only null form,
			// and it's also what an empty value comes out as -
			// so an empty varchar and a null varchar are the
			// same bytes, and the client reads both as null.
			write(&resppacket,(byte_t)0);
			break;
	}
}

static uint16_t mdays[]={31,28,31,30,31,30,31,31,30,31,30,31};

static bool isLeapYear(int32_t year) {
	return (!(year%4) && (year%100 || !(year%400)));
}

bool sqlrprotocol_tds::parseDateTime(const char *datetime,
					int16_t *year,
					int16_t *month,
					int16_t *day,
					int16_t *hour,
					int16_t *minute,
					int16_t *second,
					int32_t *usec,
					int16_t *tzoffset) {

	*tzoffset=0;

	// split off a trailing "+hh:mm" or "-hh:mm"
	// (datetime::parse takes it for a third date/time part and fails)
	char	*copy=NULL;
	const char	*str=datetime;
	const char	*sp=charstring::findLast(datetime,' ');
	if (sp && (sp[1]=='+' || sp[1]=='-') &&
			character::isDigit(sp[2]) &&
			character::isDigit(sp[3]) &&
			sp[4]==':' &&
			character::isDigit(sp[5]) &&
			character::isDigit(sp[6]) &&
			!sp[7]) {
		int16_t	offhour=(sp[2]-'0')*10+(sp[3]-'0');
		int16_t	offminute=(sp[5]-'0')*10+(sp[6]-'0');
		*tzoffset=offhour*60+offminute;
		if (sp[1]=='-') {
			*tzoffset=-(*tzoffset);
		}
		copy=charstring::duplicate(datetime,sp-datetime);
		str=copy;
	}

	// FIXME: set ddmm and yyyyddmm somehow
	bool	isnegative;
	bool	success=datetime::parse(str,false,false,"/-.:",
					year,month,day,
					hour,minute,second,
					usec,&isnegative);

	delete[] copy;

	// floor whatever the string didn't supply, since datetime::parse
	// leaves it at -1 and the writers' arithmetic is unsigned
	if (!success) {
		*year=1;
		*month=1;
		*day=1;
		*hour=0;
		*minute=0;
		*second=0;
		*usec=0;
		return false;
	}
	if (*year<1) {
		*year=1;
	}
	if (*month<1) {
		*month=1;
	}
	if (*day<1) {
		*day=1;
	}
	if (*hour<0) {
		*hour=0;
	}
	if (*minute<0) {
		*minute=0;
	}
	if (*second<0) {
		*second=0;
	}
	if (*usec<0) {
		*usec=0;
	}
	return true;
}

void sqlrprotocol_tds::dateTime(const char *datetime,
					int32_t *dayssince1900,
					uint32_t *threehundredths) {

	// parse the date/time
	int16_t	year;
	int16_t	month;
	int16_t	day;
	int16_t	hour;
	int16_t	minute;
	int16_t	second;
	int32_t	usec;
	int16_t	tzoffset;
	bool	parsed=parseDateTime(datetime,
				&year,&month,&day,
				&hour,&minute,&second,
				&usec,&tzoffset);

	// send the type's own epoch for a string we couldn't parse
	if (!parsed) {
		*dayssince1900=0;
		*threehundredths=0;
		debugWrite("unparseable datetime: %s",datetime);
		return;
	}

	// calculate days since 1900
	*dayssince1900=((year-1900)*365);
	for (uint16_t i=0; i<month-1; i++) {
		(*dayssince1900)+=mdays[i];
	}
	(*dayssince1900)+=(day-1);

	// add leap years between 1900 and the specified year
	// * years divisible by 4 are leap years
	//  * unless they are divisible by 100
	//   * unless they are divisible by 400
	for (uint16_t i=1900; i<year; i++) {
		if (i%4) {
			// common year
		} else if (i%100) {
			// leap year
			(*dayssince1900)++;
		} else if (i%400) {
			// common year
		} else {
			// leap year
			(*dayssince1900)++;
		}
		/*if (!(i%4)) {
			if (!(i%100)) {
				if (!(i%400)) {
					(*dayssince1900)++;
				}
			} else {
				(*dayssince1900)++;
			}
		}*/
	}

	// if the specified year is a leap year...
	if (month>2) {
		if (year%4) {
			// common year
		} else if (year%100) {
			// leap year
			(*dayssince1900)++;
		} else if (year%400) {
			// common year
		} else {
			// leap year
			(*dayssince1900)++;
		}
		/*if (!(year%4)) {
			if (year%100) {
				if (!(year%400)) {
					(*dayssince1900)++;
				}
			} else {
				(*dayssince1900)++;
			}
		}*/
	} else if (month==2 && day==29) {
		(*dayssince1900)++;
	}

	// FIXME: there's got to be a less iterative way to do leap years

	// calculate three-hundredths of a second since 12AM
	*threehundredths=((hour*60*60+minute*60+second)*300)+(usec*3/10000);

	debugStart("datetime");
	debugWrite("string: %s",datetime);
	debugWrite("year: %d",year);
	debugWrite("month: %d",month);
	debugWrite("day: %d",day);
	debugWrite("hour: %d",hour);
	debugWrite("minute: %d",minute);
	debugWrite("second: %d",second);
	debugWrite("usec: %d",usec);
	debugWrite("days since 1900: %d",*dayssince1900);
	debugWrite("300ths since 12AM: %d",*threehundredths);
	debugEnd();
}

void sqlrprotocol_tds::dateTimeValue(int32_t dayssince1900,
					uint32_t threehundredths,
					sqlrserverbindvar *bv) {

	debugStart("date time value");
	debugWrite("days since 1900: %d",dayssince1900);
	debugWrite("300ths since 12AM: %d",threehundredths);

	stringbuffer	strb;
	bulkDateTime(dayssince1900,threehundredths,&strb);

	int16_t	year;
	int16_t	month;
	int16_t	day;
	int16_t	hour;
	int16_t	minute;
	int16_t	second;
	int32_t	usec;
	int16_t	tzoffset;
	if (!parseDateTime(strb.getString(),
				&year,&month,&day,
				&hour,&minute,&second,
				&usec,&tzoffset)) {
		debugWrite("unparseable datetime: %s",strb.getString());
		debugEnd();
		return;
	}

	bv->type=SQLRSERVERBINDVARTYPE_DATE;
	bv->value.dateval.year=year;
	bv->value.dateval.month=month;
	bv->value.dateval.day=day;
	bv->value.dateval.hour=hour;
	bv->value.dateval.minute=minute;
	bv->value.dateval.second=second;
	bv->value.dateval.microsecond=usec;
	bv->value.dateval.tz=NULL;
	bv->value.dateval.isnegative=false;
	bv->valuesize=sizeof(bv->value.dateval);
	bv->isnull=cont->getNonNullBindValue();

	debugWrite("value: %s",strb.getString());
	debugEnd();
}

void sqlrprotocol_tds::moneyValue(int64_t tenthousandths,
					sqlrserverbindvar *bv) {

	stringbuffer	strb;
	bulkMoney(tenthousandths,&strb);

	// bound as a number rather than a string - mssql converts a varchar
	// to money on its own but ase refuses to ("Implicit conversion from
	// datatype 'VARCHAR' to 'MONEY' is not allowed")
	bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
	bv->isnull=cont->getNonNullBindValue();
	bv->value.doubleval.value=
		(double)charstring::convertToFloat(strb.getString());
	// FIXME: kludgy, but the same thing bulkDouble() does
	bv->value.doubleval.precision=
		(uint32_t)charstring::getLength(strb.getString())-
		((charstring::contains(strb.getString(),'-'))?1:0)-
		((charstring::contains(strb.getString(),'.'))?1:0);
	// money is always scaled to four decimal places
	bv->value.doubleval.scale=4;

	debugWrite("value: %s",strb.getString());
}

static bool isLeapYear(int16_t year) {
	// * years divisible by 4 are leap years
	//  * unless they are divisible by 100
	//   * unless they are divisible by 400
	if (year%4) {
		return false;
	}
	if (year%100) {
		return true;
	}
	return !(year%400);
}

uint32_t sqlrprotocol_tds::daysSince1(int16_t year,
						int16_t month, int16_t day) {

	// days in the years before this one
	int32_t	prevyears=year-1;
	if (prevyears<0) {
		prevyears=0;
	}
	uint32_t	dayssince1=prevyears*365+
					prevyears/4-
					prevyears/100+
					prevyears/400;

	// days in the months before this one
	for (int16_t i=0; i<month-1 && i<12; i++) {
		dayssince1+=mdays[i];
	}
	if (month>2 && isLeapYear(year)) {
		dayssince1++;
	}

	// days in this month
	if (day>0) {
		dayssince1+=day-1;
	}

	return dayssince1;
}

uint64_t sqlrprotocol_tds::incrementsSince12AM(int16_t hour,
						int16_t minute,
						int16_t second,
						int32_t usec,
						byte_t scale) {

	// whole seconds since 12 am, in 10^-scale increments
	uint64_t	units=1;
	for (byte_t i=0; i<scale; i++) {
		units*=10;
	}
	uint64_t	increments=
			((uint64_t)(hour*3600+minute*60+second))*units;

	// the fraction, converted from microseconds
	if (scale>6) {
		increments+=(uint64_t)usec*10;
	} else {
		uint32_t	divisor=1;
		for (byte_t i=0; i<6-scale; i++) {
			divisor*=10;
		}
		increments+=(uint64_t)usec/divisor;
	}

	return increments;
}

void sqlrprotocol_tds::date(const char *datetime, uint32_t *dayssince1) {

	// parse the date/time
	int16_t	year;
	int16_t	month;
	int16_t	day;
	int16_t	hour;
	int16_t	minute;
	int16_t	second;
	int32_t	usec;
	int16_t	tzoffset;
	if (!parseDateTime(datetime,
				&year,&month,&day,
				&hour,&minute,&second,
				&usec,&tzoffset)) {
		*dayssince1=0;
		debugWrite("unparseable date: %s",datetime);
		return;
	}

	*dayssince1=daysSince1(year,month,day);

	debugStart("date");
	debugWrite("string: %s",datetime);
	debugWrite("year: %d",year);
	debugWrite("month: %d",month);
	debugWrite("day: %d",day);
	debugWrite("hour: %d",hour);
	debugWrite("minute: %d",minute);
	debugWrite("second: %d",second);
	debugWrite("usec: %d",usec);
	debugWrite("days since 1: %d",*dayssince1);
	debugEnd();
}

void sqlrprotocol_tds::time(const char *datetime,
					byte_t scale,
					uint64_t *increments) {

	// parse the date/time
	int16_t	year;
	int16_t	month;
	int16_t	day;
	int16_t	hour;
	int16_t	minute;
	int16_t	second;
	int32_t	usec;
	int16_t	tzoffset;
	if (!parseDateTime(datetime,
				&year,&month,&day,
				&hour,&minute,&second,
				&usec,&tzoffset)) {
		*increments=0;
		debugWrite("unparseable time: %s",datetime);
		return;
	}

	*increments=incrementsSince12AM(hour,minute,second,usec,scale);

	debugStart("time");
	debugWrite("string: %s",datetime);
	debugWrite("hour: %d",hour);
	debugWrite("minute: %d",minute);
	debugWrite("second: %d",second);
	debugWrite("usec: %d",usec);
	debugWrite("scale: %d",scale);
	debugWrite("increments since 12AM: %lld",(long long)*increments);
	debugEnd();
}

byte_t sqlrprotocol_tds::timeSize(byte_t scale) {
	// 3 bytes if 0 <= scale <= 2
	// 4 bytes if 3 <= scale <= 4
	// 5 bytes if 5 <= scale <= 7
	if (scale<=2) {
		return 3;
	}
	if (scale<=4) {
		return 4;
	}
	return 5;
}

void sqlrprotocol_tds::appendDate(uint32_t dayssince1) {
	// a date is 3 little-endian bytes, without a size of its own
	write(&resppacket,(byte_t)(dayssince1&0xFF));
	write(&resppacket,(byte_t)((dayssince1>>8)&0xFF));
	write(&resppacket,(byte_t)((dayssince1>>16)&0xFF));
	debugWrite("days since 1: %d",dayssince1);
}

void sqlrprotocol_tds::appendTime(uint64_t increments, byte_t size) {
	// a time is "size" little-endian bytes, without a size of its own
	for (byte_t i=0; i<size; i++) {
		write(&resppacket,(byte_t)((increments>>(i*8))&0xFF));
	}
	debugWrite("increments since 12AM: %lld",(long long)increments);
}

void sqlrprotocol_tds::daten(const char *field) {
	debugStart("date n");
	uint32_t	dayssince1;
	date(field,&dayssince1);
	write(&resppacket,(byte_t)3);
	appendDate(dayssince1);
	debugEnd();
}

void sqlrprotocol_tds::timen(const char *field, byte_t scale) {
	debugStart("time n");
	uint64_t	increments;
	time(field,scale,&increments);
	byte_t	size=timeSize(scale);
	debugWrite("size: %d",size);
	write(&resppacket,size);
	appendTime(increments,size);
	debugEnd();
}

void sqlrprotocol_tds::datetime2n(const char *field, byte_t scale) {

	debugStart("datetime2 n");

	// a datetime2 is a time followed by a date,
	// under a single size
	uint64_t	increments;
	time(field,scale,&increments);
	uint32_t	dayssince1;
	date(field,&dayssince1);

	byte_t	size=timeSize(scale);
	debugWrite("size: %d",size);
	write(&resppacket,(byte_t)(size+3));
	appendTime(increments,size);
	appendDate(dayssince1);

	debugEnd();
}

void sqlrprotocol_tds::datetimeoffsetn(const char *field, byte_t scale) {

	debugStart("datetimeoffset n");
	debugWrite("field: %s",field);
	debugWrite("scale: %d",scale);

	// a datetimeoffset is a datetime2 followed by a timezone
	// offset, under a single size

	int16_t	year;
	int16_t	month;
	int16_t	day;
	int16_t	hour;
	int16_t	minute;
	int16_t	second;
	int32_t	usec;
	int16_t	tzoffset;
	bool	parsed=parseDateTime(field,
				&year,&month,&day,
				&hour,&minute,&second,
				&usec,&tzoffset);

	uint32_t	dayssince1=0;
	uint64_t	increments=0;
	if (parsed) {

		// the date and time parts are the utc instant, not local
		// wall time, so shift by the offset - borrowing or
		// carrying a day if it crosses midnight
		dayssince1=daysSince1(year,month,day);
		int32_t	secssince12am=hour*3600+minute*60+second-tzoffset*60;
		while (secssince12am<0) {
			secssince12am+=86400;
			if (dayssince1) {
				dayssince1--;
			}
		}
		while (secssince12am>=86400) {
			secssince12am-=86400;
			dayssince1++;
		}
		increments=incrementsSince12AM(secssince12am/3600,
						(secssince12am%3600)/60,
						secssince12am%60,
						usec,scale);
		debugWrite("year: %d",year);
		debugWrite("month: %d",month);
		debugWrite("day: %d",day);
		debugWrite("hour: %d",hour);
		debugWrite("minute: %d",minute);
		debugWrite("second: %d",second);
		debugWrite("usec: %d",usec);
		debugWrite("raw tz offset in minutes: %d",tzoffset);
		debugWrite("days since 1: %d",dayssince1);
		debugWrite("increments since 12AM: %lld",(long long)increments);
	} else {
		debugWrite("unparseable datetimeoffset: %s",field);
	}

	// the offset is minutes from utc, between -840 and 840,
	// sent as a signed 16-bit value
	if (tzoffset<-840) {
		tzoffset=-840;
	} else if (tzoffset>840) {
		tzoffset=840;
	}

	byte_t	size=timeSize(scale);
	debugWrite("size: %d",size);
	write(&resppacket,(byte_t)(size+3+sizeof(uint16_t)));
	appendTime(increments,size);
	appendDate(dayssince1);
	writeLE(&resppacket,(uint16_t)tzoffset);

	debugWrite("utc offset in minutes: %d",tzoffset);
	debugEnd();
}

void sqlrprotocol_tds::decimal(const char *field,
				byte_t *ispositive,
				byte_t *size,
				byte_t *val) {

	debugStart("decimal");
	debugWrite("field: %s",field);

	uint32_t	precision=charstring::getLength(field);

	*ispositive=1;
	if (field[0]=='-') {
		*ispositive=0;
		precision--;
	}

	char	*copy=charstring::duplicate(field);
	if (charstring::contains(copy,'.')) {
		charstring::strip(copy,'.');
		precision--;
	}

	if (precision>=1 && precision<=9) {
		*size=4;
		int32_t	v=charstring::convertToInteger((*ispositive)?copy:copy+1);
		v=hostToLE((uint32_t)v);
		bytestring::copy(val,&v,sizeof(v));
	} else if (precision>=10 && precision<=19) {
		*size=8;
		int64_t	v=charstring::convertToInteger((*ispositive)?copy:copy+1);
		v=hostToLE((uint64_t)v);
		bytestring::copy(val,&v,sizeof(v));
	} else if (precision>=20 && precision<=28) {
		*size=12;
		// FIXME: actually implement this...
	} else if (precision>=29 && precision<=38) {
		*size=16;
		// FIXME: actually implement this...
	}

	delete[] copy;

	debugWrite("precision: %d",precision);
	debugWrite("ispositive: %d",*ispositive);
	debugWrite("size: %d",*size);
	debugEnd();
}

byte_t sqlrprotocol_tds::decimalSize(byte_t precision) {

	// How many bytes a decimal of a given precision occupies on the
	// wire, counting the sign byte.  The client works this out from the
	// precision it was sent, rather than from the length byte, so a
	// value written at any other width decodes to garbage.
	static const byte_t	size[]={
		1,
		2,  2,  3,  3,  4,  4,  4,  5,  5,
		6,  6,  6,  7,  7,  8,  8,  9,  9,  9,
		10, 10, 11, 11, 11, 12, 12, 13, 13, 14,
		14, 14, 15, 15, 16, 16, 16, 17, 17
	};
	return (precision<sizeof(size))?size[precision]:TDS_DECIMAL_MAX_SIZE;
}

void sqlrprotocol_tds::guid(const char *field, byte_t *g) {

	debugStart("guid");
	debugWrite("field: %s",field);

	// convert string into 16 hex values...
	for (uint16_t i=0; i<16; i++) {
		if (*field=='-') {
			field++;
		}
		g[i]=charsToHex(field);
		field+=2;
	}

	// swap first 4 bytes (apparently)
	byte_t	tmp=g[0];
	g[0]=g[3];
	g[3]=tmp;
	tmp=g[1];
	g[1]=g[2];
	g[2]=tmp;

	// swap next 2 bytes (apparently)
	tmp=g[4];
	g[4]=g[5];
	g[5]=tmp;

	// swap next 2 bytes (apparently)
	tmp=g[6];
	g[6]=g[7];
	g[7]=tmp;

	// leave the rest alone (apparently)

	debugEnd();
}

byte_t sqlrprotocol_tds::charsToHex(const char *chars) {

	// FIXME: this method is really brute-force...

	byte_t	sixteens=0;
	byte_t	ones=0;

	char	ch=*chars;
	if (ch) {
		if (ch>='A' && ch<='F') {
			sixteens=ch-'A'+10;
		} else if (ch>='a' && ch<='f') {
			sixteens=ch-'a'+10;
		} else if (ch>='0' && ch<='9') {
			sixteens=ch-'0';
		}
	}

	chars++;

	ch=*chars;
	if (ch) {
		if (ch>='A' && ch<='F') {
			ones=ch-'A'+10;
		} else if (ch>='a' && ch<='f') {
			ones=ch-'a'+10;
		} else if (ch>='0' && ch<='9') {
			ones=ch-'0';
		}
	}

	return sixteens*16+ones;
}

uint32_t sqlrprotocol_tds::appendQueryError(sqlrservercursor *cursor) {

	debugStart("query error");

	// get the error
	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errorcode;
	bool		liveconnection;
	cont->getError(cursor,&errorstring,
				&errorsize,
				&errorcode,
				&liveconnection);
	char	*errorbuffer=charstring::duplicate(errorstring,errorsize);
	char	*errptr=errorbuffer;

	// mssql/sap error format:
	// Server message: ... severity(...) number(...) state(...) line(...)
	// Server Name:... Procedure Name:...
	// (2 spaces before "Server Name", no space after the colons)
	byte_t		state=1;
	byte_t		errclass=0;
	uint32_t	linenumber=1;
	char		*srvn=NULL;
	char		*procn=NULL;
	char		*severityptr=NULL;
	char		*procptr=NULL;
	if (dbistds &&
		!charstring::compare(errorbuffer,"Server message: ",16)) {

		errptr=errorbuffer+16;

		severityptr=charstring::findFirst(errptr," severity(");
		if (severityptr) {
			errclass=charstring::convertToInteger(severityptr+10);
		}

		char	*stateptr=charstring::findFirst(errptr," state(");
		if (stateptr) {
			state=charstring::convertToInteger(stateptr+7);
		}

		char	*lineptr=charstring::findFirst(errptr," line(");
		if (lineptr) {
			linenumber=charstring::convertToInteger(lineptr+6);
		}

		char	*srvptr=charstring::findFirst(errptr," Server Name:");
		procptr=charstring::findFirst(errptr,"  Procedure Name:");
		if (srvptr && procptr) {
			*procptr='\0';
			srvn=charstring::duplicate(srvptr+13);
			procn=charstring::duplicate(procptr+17);
		}

		*severityptr='\0';
	}

	debugWrite("errorcode: %lld",(long long)errorcode);
	debugWrite("state: %d",state);
	debugWrite("errclass: %d",errclass);
	debugWrite("linenumber: %d",linenumber);
	debugWrite("server: %s",(srvn)?srvn:srvname);
	debugWrite("procedure: %s",(procn)?procn:"");
	debugWrite("message: %s",errptr);

	// append the error to the send packet
	appendError(errorcode,
		state,errclass,
		errptr,(srvn)?srvn:srvname,procn,
		linenumber);

	// reset nulls to spaces
	if (severityptr) {
		*severityptr=' ';
	}
	if (procptr) {
		*procptr=' ';
	}

	// clean up
	delete[] srvn;
	delete[] procn;
	delete[] errorbuffer;

	debugEnd();
	return (uint32_t)errorcode;
}

bool sqlrprotocol_tds::insertBulk(const char *sql) {

	// "insert bulk <table> (<column> <type>, ...)"

	const char	*ptr=cont->skipWhitespaceAndComments(sql);
	if (charstring::compareIgnoringCase(ptr,"insert",6) ||
			!character::isWhitespace(ptr[6])) {
		return false;
	}
	ptr=cont->skipWhitespaceAndComments(ptr+6);
	if (charstring::compareIgnoringCase(ptr,"bulk",4) ||
			!character::isWhitespace(ptr[4])) {
		return false;
	}
	ptr=cont->skipWhitespaceAndComments(ptr+4);

	debugStart("insert bulk");

	// forget whatever the previous bulk load left behind
	bulkpool.clear();
	bulktable=NULL;
	bulkcolumncount=0;

	// the table name, which freetds doesn't quote
	const char	*start=ptr;
	while (*ptr && !character::isWhitespace(*ptr) && *ptr!='(') {
		ptr++;
	}
	size_t	length=ptr-start;
	if (!length) {
		debugWrite("no table name");
		debugEnd();
		return true;
	}
	bulktable=(char *)bulkpool.allocate(length+1);
	bytestring::copy(bulktable,start,length);
	bulktable[length]='\0';

	debugWrite("table: %s",bulktable);

	ptr=cont->skipWhitespaceAndComments(ptr);
	if (*ptr!='(') {
		debugWrite("no column list");
		debugEnd();
		bulktable=NULL;
		return true;
	}
	ptr++;

	// the column list, one "<column> <type>" per column
	while (*ptr && *ptr!=')') {

		ptr=cont->skipWhitespaceAndComments(ptr);

		// the column name, which freetds bracket-quotes, doubling
		// any bracket in the name itself.  copied out verbatim,
		// quoting and all, so the insert below names the same column.
		start=ptr;
		if (*ptr=='[') {
			ptr++;
			while (*ptr) {
				if (*ptr==']') {
					ptr++;
					if (*ptr!=']') {
						break;
					}
				}
				ptr++;
			}
		} else {
			while (*ptr && !character::isWhitespace(*ptr)) {
				ptr++;
			}
		}
		length=ptr-start;
		if (!length || bulkcolumncount==maxbindcount) {
			debugWrite("bad column list");
			debugEnd();
			bulktable=NULL;
			return true;
		}
		char	*column=(char *)bulkpool.allocate(length+1);
		bytestring::copy(column,start,length);
		column[length]='\0';
		bulkcolumns[bulkcolumncount]=column;
		bulkcolumncount++;

		debugWrite("column: %s",column);

		// skip the declared type, which can have parentheses of
		// its own, as decimal and numeric do
		uint16_t	depth=0;
		while (*ptr) {
			if (*ptr=='(') {
				depth++;
			} else if (*ptr==')') {
				if (!depth) {
					break;
				}
				depth--;
			} else if (*ptr==',' && !depth) {
				break;
			}
			ptr++;
		}
		if (*ptr==',') {
			ptr++;
		}
	}

	if (!bulkcolumncount) {
		debugWrite("no columns");
		bulktable=NULL;
	}

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::bulkLoad() {

	const byte_t	*rp=reqpacket.getBuffer();
	size_t		rpsize=reqpacket.getSize();

	debugStart("bulk load");

	// recvPacket() takes this packet type whatever the session logged
	// in as, but nothing below - metadata, type info, or field - has a
	// pretds7 branch, and what's written back is sized by charSize().
	// a pre-tds7 session's bulk data would be misparsed as ms-tds from
	// the first type byte on.
	if (pretds7) {
		debugWrite("pre-tds7 session");
		debugEnd();
		return sendTdsProtocolError();
	}

	// bulk data without an insert bulk statement to go with it
	if (!bulktable) {
		debugWrite("no insert bulk statement");
		debugEnd();
		return sendError(0,1,16,
				"Bulk load data without an insert bulk "
				"statement",1);
	}

	// the packet opens with the client's own column metadata.  a bad one
	// doesn't end the session - each request is a whole packet, so
	// dropping this one leaves the request stream in sync.
	uint16_t	colcount=0;
	if (!bulkColMetaData(&rp,&rpsize,&colcount)) {
		debugEnd();
		return sendError(0,1,16,
				"Malformed bulk load column metadata",1);
	}

	// the insert bulk statement and the column metadata describe the same
	// columns in the same order - if they disagree, there's no telling
	// which column a value belongs to
	if (colcount!=bulkcolumncount) {
		debugWrite("column count mismatch: %d != %d",
					colcount,bulkcolumncount);
		debugEnd();
		return sendError(0,1,16,
				"Bulk load column count doesn't match the "
				"insert bulk statement",1);
	}

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return sendNoCursorAvailableError();
	}

	// prepare the insert that the rows will be loaded with
	char	*query=bulkInsert(colcount);
	debugWrite("query: %s",query);
	size_t	querylen=charstring::getLength(query);
	if (querylen>maxquerysize) {
		delete[] query;
		debugWrite("query too large: %lld",(long long)querylen);
		debugEnd();
		releaseCursor(cursor);
		return sendQueryTooLargeError(querylen);
	}
	cont->setOutputBindCount(cursor,0);
	cont->setInputBindCount(cursor,colcount);
	bool	success=cont->prepareQuery(cursor,query,querylen,
						true,true,true,true);
	delete[] query;

	// run it once per row
	//
	// No MAX_COMMANDS_PER_REQUEST cap here, unlike preTds7Normal()'s
	// and remoteProcedureCall()'s command loops - this one builds
	// nothing into resppacket until after it ends, so there's no
	// growing in-memory response to bound, only one backend round
	// trip per row.  That round trip count is what bulk copy
	// legitimately needs - a client loading a large table needs
	// exactly that many - so a row cap would break real bcp use
	// rather than stop abuse of it.
	uint64_t	rowcount=0;
	bool		badrow=false;
	while (success && rpsize) {
		if (!bulkRow(&rp,&rpsize,colcount,cursor)) {
			badrow=true;
			break;
		}
		success=cont->executeQuery(cursor,true,true,true,true);
		if (success) {
			rowcount++;
		}
	}

	debugWrite("rows: %lld",(long long)rowcount);
	debugEnd();

	if (badrow) {
		releaseCursor(cursor);
		return sendError(0,1,16,"Malformed bulk load row",1);
	}

	// begin building the response packet
	resppacket.clear();

	if (success) {
		// blk_done reads its outrow from this row count, and only
		// takes it if DONE_COUNT is set
		done(DONE_FINAL|DONE_COUNT,0,rowcount);
	} else {
		appendQueryError(cursor);
		done(DONE_ERROR,0,0);
	}

	// send the response packet
	bool	retval=sendPacket();

	// release the cursor
	releaseCursor(cursor);

	return retval;
}

bool sqlrprotocol_tds::bulkColMetaData(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t *colcount) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	debugStart("col meta data");

	if (rpsize<sizeof(byte_t)+sizeof(uint16_t)) {
		debugWrite("short packet");
		debugEnd();
		return false;
	}

	byte_t	token;
	read(rp,&token,&rp);
	rpsize--;
	debugTokenType(token);
	if (token!=TOKEN_COLMETADATA) {
		debugEnd();
		return false;
	}

	uint16_t	count;
	readLE(rp,&count,&rp);
	rpsize-=sizeof(uint16_t);
	debugWrite("count: %d",count);
	if (!count || count>maxbindcount) {
		debugEnd();
		return false;
	}

	for (uint16_t col=0; col<count; col++) {

		debugStart("col %d",col);

		// user type and flags
		size_t	usertypesize=(negotiatedtdsversion>=720)?
					sizeof(uint32_t):sizeof(uint16_t);
		if (rpsize<usertypesize+sizeof(uint16_t)+sizeof(byte_t)) {
			debugWrite("short packet");
			debugEnd();
			debugEnd();
			return false;
		}
		if (negotiatedtdsversion>=720) {
			uint32_t	usertype;
			readLE(rp,&usertype,&rp);
			debugWrite("usertype: %d",usertype);
		} else {
			uint16_t	usertype;
			readLE(rp,&usertype,&rp);
			debugWrite("usertype: %d",usertype);
		}
		rpsize-=usertypesize;

		uint16_t	flags;
		readLE(rp,&flags,&rp);
		rpsize-=sizeof(uint16_t);
		if (getDebug()) {
			stringbuffer	b;
			b.printBits(flags);
			debugWrite("flags: %s",b.getString());
		}

		if (!bulkTypeInfo(&rp,&rpsize,col)) {
			debugEnd();
			debugEnd();
			return false;
		}

		// a blob column carries the table name, but without the
		// numparts byte that a server-to-client col meta data has
		byte_t	tdstype=bulktypes[col];
		if (tdstype==TDS_TYPE_TEXT ||
			tdstype==TDS_TYPE_NTEXT ||
			tdstype==TDS_TYPE_IMAGE) {
			if (rpsize<sizeof(uint16_t)) {
				debugWrite("short packet");
				debugEnd();
				debugEnd();
				return false;
			}
			uint16_t	tnamelen;
			readLE(rp,&tnamelen,&rp);
			rpsize-=sizeof(uint16_t);
			size_t	tnamesize=tnamelen*sizeof(ucs2_t);
			if (rpsize<tnamesize) {
				debugWrite("short packet");
				debugEnd();
				debugEnd();
				return false;
			}
			rp+=tnamesize;
			rpsize-=tnamesize;
		}

		// column name
		if (!rpsize) {
			debugWrite("short packet");
			debugEnd();
			debugEnd();
			return false;
		}
		byte_t	cnamelen;
		read(rp,&cnamelen,&rp);
		rpsize--;
		size_t	cnamesize=cnamelen*sizeof(ucs2_t);
		if (rpsize<cnamesize) {
			debugWrite("short packet");
			debugEnd();
			debugEnd();
			return false;
		}
		if (getDebug() && cnamelen) {
			ucs2_t	*cname16=new ucs2_t[cnamelen];
			const byte_t	*dummy;
			read(rp,cname16,cnamelen,&dummy);
			char	*cname=charstring::duplicateUcs2(
						cname16,(size_t)cnamelen);
			debugWrite("name: %s",cname);
			delete[] cname;
			delete[] cname16;
		}
		rp+=cnamesize;
		rpsize-=cnamesize;

		debugEnd();
	}

	debugEnd();

	*rpinout=rp;
	*rpsizeinout=rpsize;
	*colcount=count;

	return true;
}

bool sqlrprotocol_tds::bulkTypeInfo(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t col) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	debugStart("type info");

	if (!rpsize) {
		debugWrite("short packet");
		debugEnd();
		return false;
	}
	byte_t	tdstype;
	read(rp,&tdstype,&rp);
	rpsize--;
	debugColumnType(tdstype);

	bulktypes[col]=tdstype;
	bulksizes[col]=0;
	bulkscales[col]=0;
	// Whether this column's values are partially length prefixed.
	// bigvarchr, bigvarbin and nvarchar are var-len or part-len depending
	// on the USHORTMAXLEN that follows the type byte, not on the type byte
	// alone, so this can only be decided once that's been read.  xml is
	// always part-len.  Same rule paramValue() applies to rpc parameters.
	bulkpartlens[col]=false;

	if (isFixedLenType(tdstype)) {

		debugWrite("fixedlentype...");

	} else if (isVarLenType(tdstype)) {

		debugWrite("varlentype...");

		// size, precision and scale
		switch (tdstype) {
			case TDS_TYPE_DECIMAL:
			case TDS_TYPE_NUMERIC:
			case TDS_TYPE_DECIMALN:
			case TDS_TYPE_NUMERICN:
				{
				if (rpsize<3) {
					debugWrite("short packet");
					debugEnd();
					return false;
				}
				byte_t	size;
				byte_t	precision;
				read(rp,&size,&rp);
				read(rp,&precision,&rp);
				read(rp,&(bulkscales[col]),&rp);
				rpsize-=3;
				bulksizes[col]=size;
				debugWrite("size: %d",size);
				debugWrite("precision: %d",precision);
				debugWrite("scale: %d",bulkscales[col]);
				}
				break;
			case TDS_TYPE_DATEN:
				break;
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				if (!rpsize) {
					debugWrite("short packet");
					debugEnd();
					return false;
				}
				read(rp,&(bulkscales[col]),&rp);
				rpsize--;
				debugWrite("scale: %d",bulkscales[col]);
				break;
			case TDS_TYPE_SSVARIANT:
			case TDS_TYPE_TEXT:
			case TDS_TYPE_NTEXT:
			case TDS_TYPE_IMAGE:
				{
				if (rpsize<sizeof(uint32_t)) {
					debugWrite("short packet");
					debugEnd();
					return false;
				}
				uint32_t	size;
				readLE(rp,&size,&rp);
				rpsize-=sizeof(uint32_t);
				bulksizes[col]=size;
				debugWrite("size: %d (32-bit)",size);
				}
				break;
			case TDS_TYPE_BIGCHAR:
			case TDS_TYPE_BIGVARCHR:
			case TDS_TYPE_BIGBINARY:
			case TDS_TYPE_BIGVARBIN:
			case TDS_TYPE_NCHAR:
			case TDS_TYPE_NVARCHAR:
			case TDS_TYPE_UDT:
				{
				if (rpsize<sizeof(uint16_t)) {
					debugWrite("short packet");
					debugEnd();
					return false;
				}
				uint16_t	size;
				readLE(rp,&size,&rp);
				rpsize-=sizeof(uint16_t);
				if (size==TDS_USHORTMAXLEN &&
						tdstype!=TDS_TYPE_UDT) {
					// the max forms - varchar(max),
					// nvarchar(max), varbinary(max) -
					// declare no size at all and carry
					// plp values instead of plainly
					// length prefixed ones.  (a udt
					// declares a real size ahead of its
					// own type info, which nothing
					// parses, so leave it alone.)
					bulkpartlens[col]=true;
					debugWrite("size: (max)");
				} else {
					bulksizes[col]=size;
					debugWrite("size: %d (16-bit)",size);
				}
				}
				break;
			case TDS_TYPE_XML:
				// XML_INFO (MS-TDS 2.2.5.5.2) - a
				// SchemaPresent byte, then the schema
				// collection if it's set.  No size, and the
				// values are always plp.
				if (!parseXmlInfo(&rp,&rpsize)) {
					debugWrite("short packet");
					debugEnd();
					return false;
				}
				bulkpartlens[col]=true;
				break;
			default:
				{
				if (!rpsize) {
					debugWrite("short packet");
					debugEnd();
					return false;
				}
				byte_t	size;
				read(rp,&size,&rp);
				rpsize--;
				bulksizes[col]=size;
				debugWrite("size: %d (8-bit)",size);
				}
				break;
		}

		// collation
		if (negotiatedtdsversion>=710) {
			switch (tdstype) {
				case TDS_TYPE_BIGCHAR:
				case TDS_TYPE_BIGVARCHR:
				case TDS_TYPE_TEXT:
				case TDS_TYPE_NTEXT:
				case TDS_TYPE_NCHAR:
				case TDS_TYPE_NVARCHAR:
					{
					// FIXME: do something with this
					if (rpsize<5) {
						debugWrite("short packet");
						debugEnd();
						return false;
					}
					byte_t	coll[5];
					read(rp,coll,sizeof(coll),&rp);
					rpsize-=sizeof(coll);
					if (getDebug()) {
						stringbuffer	b;
						b.printBits(coll,sizeof(coll));
						debugWrite("collation: %s",
								b.getString());
					}
					}
					break;
			}
		}

	} else {

		// FIXME: [ushortmaxlen] [collation] [xml_info] [utd_info]
		debugWrite("unsupported type");
		debugEnd();
		return false;
	}

	*rpinout=rp;
	*rpsizeinout=rpsize;

	debugEnd();

	return true;
}

char *sqlrprotocol_tds::bulkInsert(uint16_t colcount) {

	stringbuffer	query;
	query.append("insert into ")->append(bulktable)->append(" (");
	for (uint16_t col=0; col<colcount; col++) {
		if (col) {
			query.append(',');
		}
		query.append(bulkcolumns[col]);
	}
	query.append(") values (");
	for (uint16_t col=0; col<colcount; col++) {
		if (col) {
			query.append(',');
		}
		query.append(bindvarnames[col]);
	}
	query.append(')');
	return query.detachString();
}

bool sqlrprotocol_tds::bulkRow(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t colcount,
					sqlrservercursor *cursor) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	debugStart("row");

	if (!rpsize) {
		debugWrite("short packet");
		debugEnd();
		return false;
	}

	byte_t	token;
	read(rp,&token,&rp);
	rpsize--;

	debugTokenType(token);

	if (token!=TOKEN_ROW) {
		debugWrite("unexpected token");
		debugEnd();
		return false;
	}

	// values get copied out of the request packet and into the
	// cursor's own bind pool
	memorypool	*bindpool=cont->getBindPool(cursor);
	bindpool->clear();

	sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);

	for (uint16_t col=0; col<colcount; col++) {
		sqlrserverbindvar	*bv=&(inbinds[col]);
		bv->variable=bindvarnames[col];
		bv->variablesize=bindvarnamesizes[col];
		if (!bulkField(&rp,&rpsize,col,bv,bindpool)) {
			debugWrite("short packet");
			debugEnd();
			return false;
		}
	}

	debugEnd();

	*rpinout=rp;
	*rpsizeinout=rpsize;

	return true;
}

void sqlrprotocol_tds::bulkString(sqlrserverbindvar *bv,
					memorypool *bindpool,
					const char *value,
					size_t valuesize) {
	debugStart("string");
	bv->type=SQLRSERVERBINDVARTYPE_STRING;
	bv->valuesize=(uint32_t)valuesize;
	bv->value.stringval=(char *)bindpool->allocate(valuesize+1);
	bytestring::copy(bv->value.stringval,value,valuesize);
	bv->value.stringval[valuesize]='\0';
	bv->isnull=cont->getNonNullBindValue();
	debugWrite("value: %.*s",(int32_t)valuesize,bv->value.stringval);
	debugEnd();
}

void sqlrprotocol_tds::bulkBinary(sqlrserverbindvar *bv,
					memorypool *bindpool,
					const byte_t *value,
					size_t valuesize) {
	debugStart("binary");
	bv->type=SQLRSERVERBINDVARTYPE_BLOB;
	bv->valuesize=(uint32_t)valuesize;
	bv->value.stringval=(char *)bindpool->allocate(valuesize+1);
	bytestring::copy(bv->value.stringval,value,valuesize);
	bv->value.stringval[valuesize]='\0';
	bv->isnull=cont->getNonNullBindValue();
	debugWrite("value: %d bytes of binary",(int32_t)valuesize);
	debugEnd();
}

void sqlrprotocol_tds::bulkDouble(sqlrserverbindvar *bv, double value) {

	debugStart("double");

	bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
	bv->isnull=cont->getNonNullBindValue();
	bv->value.doubleval.value=value;

	// FIXME: kludgy
	char	*num=charstring::parseNumber(value);
	size_t	size=charstring::getLength(num);
	bv->value.doubleval.precision=size-
			(charstring::contains(num,'-')?1:0)-
			(charstring::contains(num,'.')?1:0);
	bv->value.doubleval.scale=
			(num+size)-charstring::findFirstOrEnd(num,'.')-1;
	debugWrite("value: %s",num);
	delete[] num;

	debugEnd();
}

void sqlrprotocol_tds::bulkDecimal(byte_t ispositive,
					const byte_t *val,
					byte_t size,
					byte_t scale,
					stringbuffer *strb) {

	debugStart("decimal");
	debugWrite("ispositive: %d",ispositive);
	debugWrite("size: %d",size);
	debugWrite("scale: %d",scale);

	// the magnitude is little-endian, and the sign is 1 for positive,
	// the opposite of what the sql standard uses
	uint64_t	magnitude=0;
	for (byte_t i=0; i<size; i++) {
		magnitude|=((uint64_t)val[i])<<(i*8);
	}

	if (!ispositive) {
		strb->append('-');
	}

	// the digits, with a decimal point "scale" places from the right
	char	digits[24];
	charstring::printf(digits,sizeof(digits),"%lld",(long long)magnitude);
	size_t	digitcount=charstring::getLength(digits);
	if (!scale) {
		strb->append(digits);
		debugWrite("value: %s",strb->getString());
		debugEnd();
		return;
	}
	if (digitcount>scale) {
		strb->append(digits,digitcount-scale);
	} else {
		strb->append('0');
	}
	strb->append('.');
	for (size_t i=digitcount; i<scale; i++) {
		strb->append('0');
	}
	strb->append(digits+((digitcount>scale)?digitcount-scale:0));

	debugWrite("value: %s",strb->getString());
	debugEnd();
}

void sqlrprotocol_tds::bulkMoney(int64_t tenthousandths, stringbuffer *strb) {

	debugStart("money");
	debugWrite("tenthousandths: %lld",(long long)tenthousandths);

	if (tenthousandths<0) {
		strb->append('-');
		tenthousandths=-tenthousandths;
	}
	strb->append((uint64_t)(tenthousandths/10000))->append('.');
	char	fraction[8];
	charstring::printf(fraction,sizeof(fraction),"%04lld",
					(long long)(tenthousandths%10000));
	strb->append(fraction);

	debugWrite("value: %s",strb->getString());
	debugEnd();
}

void sqlrprotocol_tds::bulkYmd(int32_t days,
					int32_t startyear,
					stringbuffer *strb) {

	// a datetime counts from 1900-01-01 and can run backwards, since the
	// range starts in 1753.  the year loops are bounded by the range the
	// format allows, so a garbage day count can't spin here.
	int32_t	year=startyear;
	while (days<0 && year>1) {
		year--;
		days+=(isLeapYear(year))?366:365;
	}
	while (year<9999) {
		int32_t	yeardays=(isLeapYear(year))?366:365;
		if (days<yeardays) {
			break;
		}
		days-=yeardays;
		year++;
	}

	int32_t	month=1;
	while (month<12) {
		int32_t	monthdays=mdays[month-1]+
					((month==2 && isLeapYear(year))?1:0);
		if (days<monthdays) {
			break;
		}
		days-=monthdays;
		month++;
	}

	char	buffer[16];
	charstring::printf(buffer,sizeof(buffer),"%04d-%02d-%02d",
				year,month,(days<0)?1:(days+1));
	strb->append(buffer);
}

void sqlrprotocol_tds::bulkDateTime(int32_t dayssince1900,
					uint32_t threehundredths,
					stringbuffer *strb) {

	debugStart("date time");
	debugWrite("days since 1900: %d",dayssince1900);
	debugWrite("300ths since 12AM: %d",threehundredths);

	bulkYmd(dayssince1900,1900,strb);

	// the time, counted in three-hundredths of a second since 12AM
	uint32_t	seconds=threehundredths/300;
	uint32_t	milliseconds=((threehundredths%300)*1000)/300;

	char	buffer[16];
	charstring::printf(buffer,sizeof(buffer)," %02d:%02d:%02d.%03d",
				(int32_t)(seconds/3600),
				(int32_t)((seconds/60)%60),
				(int32_t)(seconds%60),
				(int32_t)milliseconds);
	strb->append(buffer);

	debugWrite("value: %s",strb->getString());
	debugEnd();
}

void sqlrprotocol_tds::bulkTime(uint64_t increments,
					byte_t scale,
					stringbuffer *strb) {

	// the increments are 10^-scale seconds since 12AM
	uint64_t	units=1;
	for (byte_t i=0; i<scale; i++) {
		units*=10;
	}
	uint64_t	seconds=increments/units;
	uint64_t	fraction=increments%units;

	char	buffer[24];
	charstring::printf(buffer,sizeof(buffer),"%02lld:%02lld:%02lld",
					(long long)(seconds/3600),
					(long long)((seconds/60)%60),
					(long long)(seconds%60));
	strb->append(buffer);

	if (scale) {
		strb->append('.');
		charstring::printf(buffer,sizeof(buffer),"%lld",(long long)fraction);
		size_t	digitcount=charstring::getLength(buffer);
		for (size_t i=digitcount; i<scale; i++) {
			strb->append('0');
		}
		strb->append(buffer);
	}
}

void sqlrprotocol_tds::bulkGuid(const byte_t *g, stringbuffer *strb) {

	debugStart("guid");

	// the first three groups are little-endian, the rest big-endian
	char	buffer[40];
	charstring::printf(buffer,sizeof(buffer),
		"%02X%02X%02X%02X-%02X%02X-%02X%02X-"
		"%02X%02X-%02X%02X%02X%02X%02X%02X",
		g[3],g[2],g[1],g[0],g[5],g[4],g[7],g[6],
		g[8],g[9],g[10],g[11],g[12],g[13],g[14],g[15]);
	strb->append(buffer);

	debugWrite("value: %s",strb->getString());
	debugEnd();
}

bool sqlrprotocol_tds::bulkField(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t col,
					sqlrserverbindvar *bv,
					memorypool *bindpool) {

	debugStart("col %d",col);
	debugWrite("tdstype: 0x%02x",bulktypes[col]);

	// until proven otherwise - a binary column stays a lob even when
	// the value turns out to be null, so bulkBinary()'s type doesn't
	// get lost before it's ever reached
	switch (bulktypes[col]) {
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGVARBIN:
		case TDS_TYPE_IMAGE:
			bv->type=SQLRSERVERBINDVARTYPE_NULLBLOB;
			break;
		default:
			bv->type=SQLRSERVERBINDVARTYPE_NULL;
			break;
	}
	bv->valuesize=0;
	bv->value.stringval=NULL;
	bv->isnull=cont->getNullBindValue();

	bool	retval=bulkValue(rpinout,rpsizeinout,col,bv,bindpool);

	debugEnd();

	return retval;
}

bool sqlrprotocol_tds::bulkValue(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t col,
					sqlrserverbindvar *bv,
					memorypool *bindpool) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	byte_t		tdstype=bulktypes[col];
	byte_t		scale=bulkscales[col];

	debugStart("value");
	debugColumnType(tdstype);

	// a max column - varchar(max), nvarchar(max), varbinary(max), xml -
	// carries plp values rather than plainly length prefixed ones.
	// bulkTypeInfo() decided that from the column's type info, since the
	// type byte alone doesn't say which shape a value has.
	if (bulkpartlens[col]) {
		bool	retval=plpValue(rpinout,rpsizeinout,
						tdstype,bv,bindpool);
		debugEnd();
		return retval;
	}

	// a text, ntext or image value arrives behind a text pointer and
	// timestamp, both filled with 0xFF by a bulk load.  a single zero
	// length in place of the pointer means null.
	if (tdstype==TDS_TYPE_TEXT ||
		tdstype==TDS_TYPE_NTEXT ||
		tdstype==TDS_TYPE_IMAGE) {
		if (!rpsize) {
			debugEnd();
			return false;
		}
		byte_t	ptrsize;
		read(rp,&ptrsize,&rp);
		rpsize--;
		if (!ptrsize) {
			debugWrite("value: (null)");
			debugEnd();
			return true;
		}
		size_t	prefixsize=(size_t)ptrsize+sizeof(uint64_t);
		if (rpsize<prefixsize) {
			debugEnd();
			return false;
		}
		rp+=prefixsize;
		rpsize-=prefixsize;
	}

	// the "n" types carry a size that also says which concrete type
	// the value is, and a size of zero means null
	switch (tdstype) {
		case TDS_TYPE_INTN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			switch (size) {
				case 1:
					tdstype=TDS_TYPE_INT1;
					break;
				case 2:
					tdstype=TDS_TYPE_INT2;
					break;
				case 4:
					tdstype=TDS_TYPE_INT4;
					break;
				case 8:
					tdstype=TDS_TYPE_INT8;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
		case TDS_TYPE_BITN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (size!=1) {
				debugWrite("invalid size: %d",size);
				debugEnd();
				return false;
			}
			tdstype=TDS_TYPE_BIT;
			}
			break;
		case TDS_TYPE_FLTN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_FLT4;
					break;
				case 8:
					tdstype=TDS_TYPE_FLT8;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
		case TDS_TYPE_MONEYN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_MONEY4;
					break;
				case 8:
					tdstype=TDS_TYPE_MONEY;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
		case TDS_TYPE_DATETIMN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_DATETIM4;
					break;
				case 8:
					tdstype=TDS_TYPE_DATETIME;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
	}

	// the value itself
	switch (tdstype) {
		case TDS_TYPE_INT1:
		case TDS_TYPE_BIT:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	val;
			read(rp,&val,&rp);
			rpsize--;
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=1;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			break;
		case TDS_TYPE_INT2:
			{
			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			int16_t	val;
			readLE(rp,(uint16_t *)&val,&rp);
			rpsize-=sizeof(uint16_t);
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=2;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			break;
		case TDS_TYPE_INT4:
			{
			if (rpsize<sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			int32_t	val;
			readLE(rp,(uint32_t *)&val,&rp);
			rpsize-=sizeof(uint32_t);
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=4;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			break;
		case TDS_TYPE_INT8:
			{
			if (rpsize<sizeof(uint64_t)) {
				debugEnd();
				return false;
			}
			int64_t	val;
			readLE(rp,(uint64_t *)&val,&rp);
			rpsize-=sizeof(uint64_t);
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=8;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			break;
		case TDS_TYPE_FLT4:
			{
			if (rpsize<sizeof(float)) {
				debugEnd();
				return false;
			}
			float	val;
			read(rp,&val,&rp);
			rpsize-=sizeof(float);
			bulkDouble(bv,val);
			}
			break;
		case TDS_TYPE_FLT8:
			{
			if (rpsize<sizeof(double)) {
				debugEnd();
				return false;
			}
			double	val;
			read(rp,&val,&rp);
			rpsize-=sizeof(double);
			bulkDouble(bv,val);
			}
			break;
		case TDS_TYPE_MONEY:
			{
			if (rpsize<2*sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			uint32_t	high;
			uint32_t	low;
			readLE(rp,&high,&rp);
			readLE(rp,&low,&rp);
			rpsize-=2*sizeof(uint32_t);
			stringbuffer	strb;
			bulkMoney((int64_t)((((uint64_t)high)<<32)|low),&strb);
			bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			}
			break;
		case TDS_TYPE_MONEY4:
			{
			if (rpsize<sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			int32_t	val;
			readLE(rp,(uint32_t *)&val,&rp);
			rpsize-=sizeof(uint32_t);
			stringbuffer	strb;
			bulkMoney(val,&strb);
			bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			}
			break;
		case TDS_TYPE_DATETIM4:
			{
			if (rpsize<2*sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	days;
			uint16_t	minutes;
			readLE(rp,&days,&rp);
			readLE(rp,&minutes,&rp);
			rpsize-=2*sizeof(uint16_t);
			stringbuffer	strb;
			bulkDateTime((int32_t)days,
					((uint32_t)minutes)*60*300,&strb);
			bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			}
			break;
		case TDS_TYPE_DATETIME:
			{
			if (rpsize<2*sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			readLE(rp,(uint32_t *)&dayssince1900,&rp);
			readLE(rp,&threehundredths,&rp);
			rpsize-=2*sizeof(uint32_t);
			stringbuffer	strb;
			bulkDateTime(dayssince1900,threehundredths,&strb);
			bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			}
			break;
		case TDS_TYPE_GUID:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			if (size==16) {
				stringbuffer	strb;
				bulkGuid(rp,&strb);
				bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			} else {
				debugWrite("value: (null)");
			}
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_DECIMAL:
		case TDS_TYPE_NUMERIC:
		case TDS_TYPE_DECIMALN:
		case TDS_TYPE_NUMERICN:
			{
			// the length counts the sign byte too
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			byte_t	ispositive;
			read(rp,&ispositive,&rp);
			size--;
			rpsize--;
			// FIXME: anything wider than 8 bytes overflows
			stringbuffer	strb;
			bulkDecimal(ispositive,rp,(size>8)?8:size,scale,&strb);
			rp+=size;
			rpsize-=size;
			// bound as a number rather than a string - mssql
			// converts a varchar to a decimal on its own but ase
			// refuses to ("Implicit conversion from datatype
			// 'VARCHAR' to 'DECIMAL' is not allowed")
			bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.doubleval.value=
				(double)charstring::convertToFloat(
							strb.getString());
			// FIXME: kludgy, but the same thing bulkDouble()
			// does - the column metadata carries no precision
			bv->value.doubleval.precision=
				(uint32_t)charstring::getLength(
							strb.getString())-
				((charstring::contains(
					strb.getString(),'-'))?1:0)-
				((charstring::contains(
					strb.getString(),'.'))?1:0);
			bv->value.doubleval.scale=scale;
			debugWrite("value: %s",strb.getString());
			}
			break;
		case TDS_TYPE_DATEN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			uint32_t	dayssince1=0;
			for (byte_t i=0; i<size && i<3; i++) {
				dayssince1|=((uint32_t)rp[i])<<(i*8);
			}
			rp+=size;
			rpsize-=size;
			stringbuffer	strb;
			bulkYmd((int32_t)dayssince1,1,&strb);
			bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			}
			break;
		case TDS_TYPE_TIMEN:
		case TDS_TYPE_DATETIME2N:
		case TDS_TYPE_DATETIMEOFFSETN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			// a datetime2 is a time followed by a date, and a
			// datetimeoffset is a datetime2 followed by an
			// offset, all under a single size
			byte_t	timesize=timeSize(scale);
			if (timesize>size) {
				timesize=size;
			}
			uint64_t	increments=0;
			for (byte_t i=0; i<timesize; i++) {
				increments|=((uint64_t)rp[i])<<(i*8);
			}
			stringbuffer	strb;
			if (tdstype!=TDS_TYPE_TIMEN && size>=timesize+3) {
				uint32_t	dayssince1=
						((uint32_t)rp[timesize])|
						(((uint32_t)rp[timesize+1])<<8)|
						(((uint32_t)rp[timesize+2])<<16);
				bulkYmd((int32_t)dayssince1,1,&strb);
				strb.append(' ');
			}
			bulkTime(increments,scale,&strb);
			rp+=size;
			rpsize-=size;
			bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			}
			break;
		case TDS_TYPE_CHAR:
		case TDS_TYPE_VARCHAR:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			// a length of 0 is how the protocol says null -
			// bulkField() already bound it as a null value
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			bulkString(bv,bindpool,(const char *)rp,size);
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			// a length of 0 is how the protocol says null -
			// bulkField() already bound it as a null lob
			if (!size) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			bulkBinary(bv,bindpool,rp,size);
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_BIGVARCHR:
			{
			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);
			if (size==0xFFFF) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			bulkString(bv,bindpool,(const char *)rp,size);
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGVARBIN:
			{
			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);
			if (size==0xFFFF) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			bulkBinary(bv,bindpool,rp,size);
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_NVARCHAR:
			{
			// the size is in bytes, but the data is ucs-2
			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);
			if (size==0xFFFF) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			uint16_t	length=size/sizeof(ucs2_t);
			// the data isn't necessarily aligned, so copy it
			// out before converting it
			const byte_t	*dummy;
			ucs2_t		*value16=new ucs2_t[length];
			read(rp,value16,length,&dummy);
			size_t		valuesize;
			char		*value=ucs2ToUtf8(value16,
						(size_t)length,&valuesize);
			delete[] value16;
			bulkString(bv,bindpool,value,valuesize);
			delete[] value;
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_TEXT:
		case TDS_TYPE_NTEXT:
		case TDS_TYPE_IMAGE:
		case TDS_TYPE_SSVARIANT:
			{
			if (rpsize<sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			uint32_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint32_t);
			if (size==0xFFFFFFFF) {
				debugWrite("value: (null)");
				debugEnd();
				return true;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			if (tdstype==TDS_TYPE_NTEXT) {
				// the size is in bytes,
				// but the data is ucs-2
				uint32_t	length=size/sizeof(ucs2_t);
				const byte_t	*dummy;
				ucs2_t		*value16=new ucs2_t[length];
				read(rp,value16,length,&dummy);
				size_t		valuesize;
				char		*value=ucs2ToUtf8(value16,
						(size_t)length,&valuesize);
				delete[] value16;
				bulkString(bv,bindpool,value,valuesize);
				delete[] value;
			} else if (tdstype==TDS_TYPE_IMAGE) {
				bulkBinary(bv,bindpool,rp,size);
			} else {
				bulkString(bv,bindpool,(const char *)rp,size);
			}
			rp+=size;
			rpsize-=size;
			}
			break;
		default:
			// the part-len types are read above rather than here.
			// what's left is udt, whose body nothing parses.
			debugWrite("unsupported type");
			debugEnd();
			return false;
	}

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::remoteProcedureCall() {

	const byte_t	*rp=reqpacket.getBuffer();
	size_t		rpsize=reqpacket.getSize();

	debugStart("rpc");

	// recvPacket() takes this packet type whatever the session logged
	// in as, but this parses ms-tds rpc's and everything it writes back
	// is sized by charSize(), which only gets the ms-tds shape when
	// pretds7 is clear.  a pre-tds7 session gets its rpc's through
	// preTds7Normal(); one arriving here would get a response shaped
	// for the wrong dialect.
	if (pretds7) {
		debugWrite("pre-tds7 session");
		debugEnd();
		return sendTdsProtocolError();
	}

	// get the headers
	if (negotiatedtdsversion>=720) {
		allHeaders(rp,rpsize,&rp,&rpsize);
	}

	// begin building the response packet
	resppacket.clear();

	// a single request packet can carry a batch of rpc's
	bool		more=true;
	uint32_t	commandcount=0;
	while (more) {

		// too many rpc's in one batch
		if (commandcount==MAX_COMMANDS_PER_REQUEST) {
			debugWrite("too many commands");
			tooManyCommands(TOKEN_DONEPROC);
			break;
		}
		commandcount++;

		if (!rpc(&rp,&rpsize,&more)) {
			debugEnd();
			// a protocol error means the request stream is
			// out of sync, so end the session after reporting it
			sendTdsProtocolError();
			return false;
		}
	}

	// An empty request - or one too short for rpc() to read even a
	// proc name length - hits rpc()'s own "nothing left to do" check
	// on the first pass and returns without appending anything.
	// Left alone that would send a response with no done token, the
	// same failure mode preTds7Normal() already guards against for an
	// empty tds 5.0 buffer.
	if (!resppacket.getSize()) {
		debugWrite("empty rpc request");
		appendError(0,1,16,"Empty TDS RPC request",srvname,NULL,1);
		doneProc(DONE_FINAL|DONE_ERROR,0,0);
	}

	debugEnd();

	// send the response packet
	return sendPacket();
}

bool sqlrprotocol_tds::rpc(const byte_t **rpinout,
					size_t *rpsizeinout,
					bool *more) {

	const byte_t	*rp=*rpinout;
	size_t		rpsize=*rpsizeinout;

	*more=false;
	rpcunsupportedtype=false;

	debugStart("rpc-call");

	// nothing left to do
	if (rpsize<sizeof(uint16_t)) {
		debugEnd();
		return true;
	}


	// get proc name/id
	uint16_t	procnamelen=0;
	char		*procname=NULL;
	uint16_t	procid=0;

	readLE(rp,&procnamelen,&rp);
	rpsize-=sizeof(procnamelen);

	if (procnamelen==0xFFFF) {

		// get the proc id
		if (rpsize<sizeof(procid)) {
			debugWrite("truncated proc id");
			debugEnd();
			return false;
		}
		readLE(rp,&procid,&rp);
		rpsize-=sizeof(procid);

		debugProcId(procid);

	} else {

		// bounds checking
		if (procnamelen*sizeof(ucs2_t)>rpsize ||
					procnamelen>maxquerysize) {
			debugWrite("invalid proc name length: %hd",procnamelen);
			debugEnd();
			return false;
		}

		// get the procname
		ucs2_t	*procname16=new ucs2_t[procnamelen];
		read(rp,procname16,procnamelen,&rp);
		rpsize-=procnamelen*sizeof(ucs2_t);
		size_t	procnamesize;
		procname=ucs2ToUtf8(procname16,(size_t)procnamelen,
							&procnamesize);
		delete[] procname16;

		debugWrite("procname: %s",procname);

		// a client can send any of the numbered procs by name
		procid=procNameToProcId(procname);
		if (procid) {
			debugProcId(procid);
		}
	}


	// get option flags
	uint16_t	optionflags=0;
	if (rpsize<sizeof(optionflags)) {
		debugWrite("truncated option flags");
		delete[] procname;
		debugEnd();
		return false;
	}
	readLE(rp,&optionflags,&rp);
	rpsize-=sizeof(optionflags);

	// parse the flags
	bool	withrecomp=(optionflags&RPC_WITH_RECOMP);
	bool	nometadata=(optionflags&RPC_NO_META_DATA);
	bool	reusemetadata=(optionflags&RPC_REUSE_META_DATA);

	if (getDebug()) {
		stringbuffer	b;
		b.printBits(optionflags);
		debugWrite("optionflags: %s",b.getString());
		debugWrite("withrecomp: %d",withrecomp);
		debugWrite("nometadata: %d",nometadata);
		debugWrite("reusemetadata: %d",reusemetadata);
	}


	// get the parameters
	if (!params(rp,rpsize,&rp,&rpsize)) {
		delete[] procname;
		if (rpcunsupportedtype) {
			// the failing parameter's own encoded length is
			// exactly what's unknown, so there's no locating
			// whatever follows it - but the error is already
			// queued, so end this rpc cleanly rather than
			// dropping the connection
			doneProc(DONE_FINAL|DONE_ERROR,0,0);
			debugEnd();
			return true;
		}
		debugEnd();
		return false;
	}

	// get the trailing batch flags
	batchFlags(rp,rpsize,&rp,&rpsize,more);


	// do whatever the proc asked for
	rpcfailed=false;
	bool	retval=runProc(procid,procname,nometadata);

	// a failed rpc has to set DONE_ERROR - the ct-lib client reports
	// CS_CMD_SUCCEED for a done without it, so a failed rpc would report
	// success and its ct_results walk would fall a result out of step
	uint16_t	donestatus=(*more)?(DONE_MORE|DONE_RPCINBATCH):DONE_FINAL;
	if (rpcfailed) {
		donestatus|=DONE_ERROR;
	}
	doneProc(donestatus,0,0);

	// clean up
	delete[] procname;

	// copy out pointer and size
	*rpinout=rp;
	*rpsizeinout=rpsize;

	debugEnd();

	return retval;
}

// Runs whichever proc the call named, once the caller has decoded it and
// filled in the rpcparams[] family.  Nothing in here reads the wire, so
// both dialects share it: an ms-tds rpc names the numbered procs by id
// and everything else by string, and a tds 5.0 dbrpc names all of them
// by string, but procNameToProcId() maps a name back to an id either
// way, so both arrive here with the same two arguments.
bool sqlrprotocol_tds::runProc(uint16_t procid,
					const char *procname,
					bool nometadata) {
	switch (procid) {
		case SP_CURSOR:
			return cursorPositioned();
		case SP_CURSOR_OPEN:
			return cursorOpen(nometadata);
		case SP_CURSOR_PREPARE:
			return cursorPrepare();
		case SP_CURSOR_EXECUTE:
			return cursorExecute(nometadata);
		case SP_CURSOR_PREP_EXEC:
			return cursorPrepExec(nometadata);
		case SP_CURSOR_UNPREPARE:
			return cursorUnprepare();
		case SP_CURSOR_FETCH:
			return cursorFetch(nometadata);
		case SP_CURSOR_OPTION:
			return cursorOption();
		case SP_CURSOR_CLOSE:
			return cursorClose();
		case SP_EXECUTE_SQL:
			return executeSql(nometadata);
		case SP_PREPARE:
			return prepare(false,false,nometadata);
		case SP_EXECUTE:
			return execute(nometadata);
		case SP_PREP_EXEC:
			return prepare(true,false,nometadata);
		case SP_PREP_EXEC_RPC:
			return prepare(true,true,nometadata);
		case SP_UNPREPARE:
			return unprepare();
		default:
			return namedProc(procname,nometadata);
	}
}

uint16_t sqlrprotocol_tds::procNameToProcId(const char *procname) {
	for (uint16_t i=1; i<=SP_MAX_PROCID; i++) {
		if (!charstring::compareIgnoringCase(procname,procnames[i])) {
			return i;
		}
	}
	return 0;
}

void sqlrprotocol_tds::batchFlags(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout,
					bool *more) {

	debugStart("batch-flags");

	// the flags are optional, and only a batch flag means another
	// rpc follows
	while (rpsize) {

		byte_t	flag;
		read(rp,&flag,&rp);
		rpsize--;

		debugWrite("batch flag: 0x%02x",flag);

		if (flag==RPC_BATCH_FLAG) {
			*more=true;
			break;
		} else if (flag==RPC_NO_EXEC_FLAG) {
			continue;
		} else {
			// not a flag at all, put it back
			rp--;
			rpsize++;
			break;
		}
	}

	debugEnd();

	// copy out pointer and size
	*rpout=rp;
	*rpsizeout=rpsize;
}

bool sqlrprotocol_tds::paramIsNull(uint16_t param) {
	return (param>=rpcparamcount ||
		rpcparams[param].type==SQLRSERVERBINDVARTYPE_NULL);
}

int64_t sqlrprotocol_tds::paramInteger(uint16_t param) {
	if (paramIsNull(param)) {
		return 0;
	}
	sqlrserverbindvar	*bv=&(rpcparams[param]);
	if (bv->type==SQLRSERVERBINDVARTYPE_INTEGER) {
		return bv->value.integerval;
	}
	if (bv->type==SQLRSERVERBINDVARTYPE_DOUBLE) {
		return (int64_t)bv->value.doubleval.value;
	}
	if (bv->type==SQLRSERVERBINDVARTYPE_STRING) {
		return charstring::convertToInteger(bv->value.stringval);
	}
	return 0;
}

const char *sqlrprotocol_tds::paramString(uint16_t param) {
	if (paramIsNull(param) ||
		rpcparams[param].type!=SQLRSERVERBINDVARTYPE_STRING) {
		return NULL;
	}
	return rpcparams[param].value.stringval;
}

void sqlrprotocol_tds::bindParams(sqlrservercursor *cursor, uint16_t first,
							bool returnvalue) {

	debugStart("bind-params");

	sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);
	sqlrserverbindvar	*outbinds=cont->getOutputBinds(cursor);
	uint16_t		inbindcount=0;
	uint16_t		outbindcount=0;

	// values get copied out of the rpc parameter pool and into the
	// cursor's own bind pool
	memorypool	*bindpool=cont->getBindPool(cursor);
	bindpool->clear();

	// odbc call syntax carries a procedure's return value as its first
	// parameter, so reserve the first output bind and the first bind
	// variable name for it, and shift the client's parameters up one
	if (returnvalue) {
		sqlrserverbindvar	*rv=&(outbinds[0]);
		rv->variable=bindvarnames[0];
		rv->variablesize=bindvarnamesizes[0];
		rv->type=SQLRSERVERBINDVARTYPE_INTEGER;
		rv->value.integerval=0;
		rv->valuesize=sizeof(int64_t);
		rv->isnull=cont->getNullBindValue();
		outbindparams[0]=RPC_RETURN_VALUE_PARAM;
		outbindcount=1;
	}

	for (uint16_t i=first; i<rpcparamcount; i++) {

		uint16_t	bindindex=i-first+((returnvalue)?1:0);
		if (bindindex>=maxbindcount) {
			break;
		}

		sqlrserverbindvar	*bv=(rpcparambyref[i])?
						&(outbinds[outbindcount]):
						&(inbinds[inbindcount]);

		*bv=rpcparams[i];

		// bind variables are named by position, not by whatever the
		// client called them - backends work out which parameter a
		// bind is from the number in its name, so a client's @P1
		// has to become @1
		// (the names the client used in the statement itself are
		// matched up by translatebindvariables, in order)
		bv->variable=bindvarnames[bindindex];
		bv->variablesize=bindvarnamesizes[bindindex];

		// an output parameter of a character type needs a buffer the
		// size the client declared, not the size of the value that
		// came in with it - an output-only parameter arrives null,
		// and without this it would get no bind at all
		if (rpcparambyref[i] && isCharType(rpcparamtdstypes[i]) &&
					rpcparammaxsizes[i]>bv->valuesize) {
			char	*value=(char *)bindpool->allocate(
						rpcparammaxsizes[i]+1);
			bytestring::set(value,0,rpcparammaxsizes[i]+1);
			if (bv->type==SQLRSERVERBINDVARTYPE_STRING &&
						bv->value.stringval) {
				bytestring::copy(value,bv->value.stringval,
							bv->valuesize);
			}
			bv->type=SQLRSERVERBINDVARTYPE_STRING;
			bv->value.stringval=value;
			bv->valuesize=rpcparammaxsizes[i];

		// copy string values out of the rpc pool
		} else if (bv->type==SQLRSERVERBINDVARTYPE_STRING &&
						bv->value.stringval) {
			char	*value=(char *)bindpool->allocate(
							bv->valuesize+1);
			bytestring::copy(value,bv->value.stringval,
							bv->valuesize);
			value[bv->valuesize]='\0';
			bv->value.stringval=value;
		}

		if (rpcparambyref[i]) {
			// remember which rpc parameter each output bind came
			// from, so its name and type can be sent back with it
			outbindparams[outbindcount]=i;
			outbindcount++;
		} else {
			inbindcount++;
		}
	}

	cont->setInputBindCount(cursor,inbindcount);
	cont->setOutputBindCount(cursor,outbindcount);

	debugWrite("input binds: %d",inbindcount);
	debugWrite("output binds: %d",outbindcount);

	debugEnd();
}

uint32_t sqlrprotocol_tds::newHandle() {
	// handle 0 is invalid, and a handle must stay positive when read
	// back as a T-SQL int (signed 32-bit), so wrap back to the base
	// rather than either extreme
	if (!nexthandle || nexthandle>0x7FFFFFFF) {
		nexthandle=SQLRELAY_HANDLE_BASE;
	}
	return nexthandle++;
}

sqlrservercursor *sqlrprotocol_tds::handleCursor(
				dictionary<uint32_t,
					sqlrservercursor *> *handles,
				uint32_t handle) {
	sqlrservercursor	*cursor=NULL;
	if (!handle || !handles->getValue(handle,&cursor)) {
		return NULL;
	}
	return cursor;
}

bool sqlrprotocol_tds::handlesContain(dictionary<uint32_t,
					sqlrservercursor *> *handles,
					sqlrservercursor *cursor) {

	linkedlist<uint32_t>	*keys=handles->getKeys();
	for (listnode<uint32_t> *node=keys->getFirst();
					node; node=node->getNext()) {
		sqlrservercursor	*c=NULL;
		if (handles->getValue(node->getValue(),&c) && c==cursor) {
			return true;
		}
	}
	return false;
}

void sqlrprotocol_tds::releaseHandles(dictionary<uint32_t,
					sqlrservercursor *> *handles,
					dictionary<uint32_t,
					sqlrservercursor *> *other) {

	debugStart("release-handles");

	// a cursor can be referred to by a prepared statement handle and a
	// cursor handle at the same time, so don't release one that the
	// other dictionary still refers to
	linkedlist<uint32_t>	*keys=handles->getKeys();
	for (listnode<uint32_t> *node=keys->getFirst();
					node; node=node->getNext()) {
		sqlrservercursor	*cursor=NULL;
		if (!handles->getValue(node->getValue(),&cursor) || !cursor) {
			continue;
		}
		if (other && handlesContain(other,cursor)) {
			debugWrite("handle: %d (shared, kept)",
							node->getValue());
			continue;
		}
		debugWrite("handle: %d (released)",node->getValue());
		executeflag.remove(cursor);
		bindmarkercount.remove(cursor);
		releasePositionRows(cursor);
		releaseCursor(cursor);
	}
	handles->clear();

	debugEnd();
}

void sqlrprotocol_tds::releaseCursorHandles(sqlrservercursor *cursor) {

	debugStart("release-cursor-handles");

	if (!cursor) {
		debugEnd();
		return;
	}

	// a cursor can be referred to by a prepared statement handle and a
	// cursor handle at the same time, so drop every handle that refers
	// to it before releasing it.  the keys are collected first, since
	// removing one invalidates the key list.
	dictionary<uint32_t, sqlrservercursor *>	*handles[2];
	handles[0]=&stmthandles;
	handles[1]=&cursorhandles;
	for (uint16_t i=0; i<2; i++) {
		linkedlist<uint32_t>	deadkeys;
		linkedlist<uint32_t>	*keys=handles[i]->getKeys();
		for (listnode<uint32_t> *node=keys->getFirst();
						node; node=node->getNext()) {
			sqlrservercursor	*c=NULL;
			if (handles[i]->getValue(node->getValue(),&c) &&
								c==cursor) {
				deadkeys.append(node->getValue());
			}
		}
		for (listnode<uint32_t> *node=deadkeys.getFirst();
						node; node=node->getNext()) {
			handles[i]->remove(node->getValue());
		}
	}

	// a tds 5.0 dynamic sql id names one of those prepared statement
	// handles, so drop any that just went away with it
	linkedlist<char *>	deadids;
	linkedlist<char *>	*idkeys=dynamicids.getKeys();
	for (listnode<char *> *node=idkeys->getFirst();
					node; node=node->getNext()) {
		uint32_t	handle=0;
		if (dynamicids.getValue(node->getValue(),&handle) &&
				!handleCursor(&stmthandles,handle)) {
			deadids.append(node->getValue());
		}
	}
	for (listnode<char *> *node=deadids.getFirst();
					node; node=node->getNext()) {
		debugWrite("dynamic id: %s (dropped)",node->getValue());
		dynamicids.remove(node->getValue());
	}

	if (pendingcursor==cursor) {
		pendingcursor=NULL;
	}

	executeflag.remove(cursor);
	bindmarkercount.remove(cursor);
	releasePositionRows(cursor);
	releaseCursor(cursor);

	debugEnd();
}

void sqlrprotocol_tds::evictOldestHandle(sqlrservercursor *keep) {

	debugStart("evict-oldest-handle");

	// the dictionaries track insertion order, so the first key is the
	// oldest handle.  cursor handles go before prepared statement
	// handles, since an abandoned cursor is the more likely leak.  keep
	// is the cursor the request that needs another one is working from -
	// evicting that would release the rows it's reading.
	dictionary<uint32_t, sqlrservercursor *>	*handles[2];
	handles[0]=&cursorhandles;
	handles[1]=&stmthandles;
	for (uint16_t i=0; i<2; i++) {
		for (listnode<uint32_t>
			*node=handles[i]->getKeys()->getFirst();
			node; node=node->getNext()) {
			sqlrservercursor	*cursor=NULL;
			if (!handles[i]->getValue(node->getValue(),&cursor) ||
								!cursor) {
				debugWrite("handle: %d (stale, dropped)",
							node->getValue());
				handles[i]->remove(node->getValue());
				debugEnd();
				return;
			}
			if (cursor!=keep) {
				debugWrite("handle: %d (evicted)",
							node->getValue());
				releaseCursorHandles(cursor);
				debugEnd();
				return;
			}
		}
	}

	debugEnd();
}

sqlrservercursor *sqlrprotocol_tds::availableCursor(sqlrservercursor *keep) {

	debugStart("available-cursor");

	sqlrservercursor	*cursor=cont->getCursor();

	// a client can walk off and leave a prepared statement or cursor
	// handle open, and each one holds a cursor forever, so evict the
	// oldest rather than starving the session
	if (!cursor) {
		debugWrite("none available, evicting");
		evictOldestHandle(keep);
		cursor=cont->getCursor();
	}

	// remember it, so an attention that abandons this request can put
	// it back
	if (cursor) {
		pendingcursor=cursor;
	}

	debugWrite((cursor)?"got cursor":"no cursor available");

	debugEnd();

	return cursor;
}

void sqlrprotocol_tds::releaseCursor(sqlrservercursor *cursor) {

	debugStart("release-cursor");

	if (!cursor) {
		debugEnd();
		return;
	}

	// close the result set before putting the cursor back.  release()
	// only marks the cursor available - it leaves whatever the backend
	// opened for the query still open.  on a backend that runs every
	// select as a real database cursor (sap, since it declares a cursor
	// per select), that cursor keeps holding the table until the sqlrelay
	// cursor happens to get reused, and a later drop of that table fails
	// with "currently in use".
	cont->closeResultSet(cursor);
	cont->release(cursor);

	debugEnd();
}

char *sqlrprotocol_tds::callSyntaxToExec(const char *stmt) {

	// sp_prepexecrpc gets its statement in odbc call syntax -
	// {call procname(?,?)} or {? = call procname(?,?)} - rather than
	// as plain sql

	debugStart("call-syntax-to-exec");
	debugWrite("stmt: %s",stmt);

	const char	*ptr=cont->skipWhitespaceAndComments(stmt);
	if (*ptr!='{') {
		debugWrite("not call syntax");
		debugEnd();
		return charstring::duplicate(stmt);
	}
	ptr++;

	// skip a return value placeholder
	const char	*eq=charstring::findFirst(ptr,'=');
	const char	*call=charstring::findFirstIgnoringCase(ptr,"call");
	if (!call) {
		debugWrite("no call keyword");
		debugEnd();
		return charstring::duplicate(stmt);
	}
	if (eq && eq<call) {
		ptr=eq+1;
	}

	call=charstring::findFirstIgnoringCase(ptr,"call");
	ptr=call+4;

	// the proc name runs up to the open paren, or to the closing brace
	// if there are no arguments
	const char	*open=charstring::findFirst(ptr,'(');
	const char	*close=charstring::findLast(ptr,'}');
	if (!close) {
		debugWrite("no closing brace");
		debugEnd();
		return charstring::duplicate(stmt);
	}

	stringbuffer	query;
	query.append("exec ");
	if (open && open<close) {
		query.append(ptr,open-ptr);
		const char	*args=open+1;
		const char	*closeparen=charstring::findLast(args,')');
		if (closeparen) {
			query.append(' ');
			query.append(args,closeparen-args);
		}
	} else {
		query.append(ptr,close-ptr);
	}

	debugWrite("exec: %s",query.getString());

	debugEnd();

	return query.detachString();
}

bool sqlrprotocol_tds::rpcInvalidHandleError(uint32_t number,
						const char *what,
						uint32_t handle) {

	stringbuffer	err;
	err.append("Invalid ")->append(what)->append(' ')->append(handle);

	debugStart("rpc-invalid-handle-error");
	debugWrite("%s",err.getString());
	debugEnd();

	appendError(number,1,16,err.getString(),srvname,NULL,1);

	// a system stored procedure that ran and failed answers with the
	// error's own number as its return status
	returnStatus(number);
	rpcfailed=true;

	return true;
}

bool sqlrprotocol_tds::rpcParamTypeError(const char *procname,
						const char *param) {

	// sp_prepare, sp_prepexec and sp_executesql declare their statement
	// and parameter-declaration arguments nvarchar, and sql server rejects
	// anything else outright rather than converting it

	stringbuffer	err;
	err.append("Procedure expects parameter '")->append(param);
	err.append("' of type 'ntext/nchar/nvarchar'.");

	debugStart("rpc-param-type-error");
	debugWrite("%s",err.getString());
	debugEnd();

	appendError(RPC_WRONG_PARAM_TYPE,3,16,
			err.getString(),srvname,procname,1);
	returnStatus(RPC_WRONG_PARAM_TYPE);
	rpcfailed=true;

	return true;
}

bool sqlrprotocol_tds::rpcUnsupportedTypeError(byte_t tdstype) {

	// a real server never gets far enough parsing an unrecognized wire
	// type to identify a proc, so unlike the errors above this carries
	// no return status of its own

	char	*hex=charstring::hexEncode(&tdstype,1);
	charstring::upper(hex);

	stringbuffer	err;
	err.append("Data type 0x")->append(hex)->append(" is unknown.");
	delete[] hex;

	debugStart("rpc-unsupported-type-error");
	debugWrite("%s",err.getString());
	debugEnd();

	appendError(RPC_UNKNOWN_DATA_TYPE,1,16,
			err.getString(),srvname,NULL,1);
	rpcunsupportedtype=true;

	return false;
}

bool sqlrprotocol_tds::rpcUnnumberedError(byte_t state,
						byte_t errclass,
						const char *msgtext) {

	// the rpc-path counterpart of the send*Error() functions.  an rpc gets
	// exactly one response, which rpc() finishes with a done-proc and
	// remoteProcedureCall() sends, so sending an error from inside a
	// handler would put a second response on the wire and leave the
	// client's packet stream a command out of step.  like
	// rpcUnsupportedTypeError(), these carry no return status - the proc
	// never ran.

	debugStart("rpc-unnumbered-error");
	debugWrite("%s",msgtext);
	debugEnd();

	appendError(0,state,errclass,msgtext,srvname,NULL,1);
	rpcfailed=true;

	return true;
}

bool sqlrprotocol_tds::rpcUnimplementedFeatureError() {
	return rpcUnnumberedError(1,10,"Unimplemented feature");
}

bool sqlrprotocol_tds::rpcQueryTooLargeError(size_t querysize) {
	stringbuffer	err;
	queryTooLargeMessage(querysize,&err);
	return rpcUnnumberedError(1,16,err.getString());
}

bool sqlrprotocol_tds::rpcNoCursorAvailableError() {
	return rpcUnnumberedError(1,16,"No cursor available");
}

bool sqlrprotocol_tds::paramIsUnicode(uint16_t param) {
	if (param>=rpcparamcount) {
		return false;
	}
	byte_t	tdstype=rpcparamtdstypes[param];
	return (tdstype==TDS_TYPE_NVARCHAR ||
		tdstype==TDS_TYPE_NCHAR ||
		tdstype==TDS_TYPE_NTEXT);
}

void sqlrprotocol_tds::rpcError(sqlrservercursor *cursor, bool returnstatus) {

	debugStart("rpc-error");

	uint32_t	number=appendQueryError(cursor);
	debugWrite("number: %d",number);
	debugWrite("returnstatus: %d",returnstatus);
	if (returnstatus) {
		returnStatus(number);
	}
	rpcfailed=true;

	debugEnd();
}

void sqlrprotocol_tds::rpcResultSet(sqlrservercursor *cursor,
						bool nometadata,
						uint64_t maxrows) {

	debugStart("rpc-result-set");

	// a statement that returns no columns still has to report how many
	// rows it affected
	if (!cont->colCount(cursor)) {
		debugWrite("no columns");
		doneInProc(DONE_COUNT,0,cont->getAffectedRows(cursor));
		debugEnd();
		return;
	}

	// A result set in tds 5.0 is a rowfmt (0xEE) and rows (0xD1), not
	// the colmetadata (0x81) that colMetaData() writes - 0x81 is a
	// cursor-delete token there.  A real ase closes an rpc's result set
	// with a done-in-proc just as sql server does, so only the two
	// tokens in front of it differ.
	if (pretds7) {
		// A result set that's too wide for the token is refused in
		// there, with its own error and done, so don't send a
		// second one.  DONE_MORE on that refusal because at least
		// the caller's own closing done still follows it.
		if (preTds7RowFmt(cursor,true)) {
			doneInProc(DONE_COUNT,0,preTds7Rows(cursor));
		}
		debugEnd();
		return;
	}

	colMetaData(cursor,nometadata);
	doneInProc(DONE_COUNT,0,rows(cursor,maxrows));

	debugEnd();
}

// Writes a proc's output parameters in whichever dialect the session
// negotiated.  The two are different tokens rather than two shapes of
// one - ms-tds gives each parameter its own self-describing returnvalue
// (0xAC), and tds 5.0 sends the whole set as a paramfmt/params pair, the
// way a result set is a rowfmt and rows.
//
// A real ase does send 0xAC, but only to a client that set response
// capability bit 45, TDS_NO_WIDETABLES.  The sap client leaves it clear,
// so the pair is what actually gets asked for.
void sqlrprotocol_tds::procReturnValues(sqlrservercursor *cursor) {
	if (pretds7) {
		preTds7ReturnValues(cursor);
	} else {
		returnValues(cursor);
	}
}

// The tds 5.0 counterpart of returnValues().  The format each parameter
// goes back in is the one the client declared it with, on the same
// reasoning returnValue() echoes the declared ms-tds type: sql relay's
// own bind type can't tell a char(20) from a varchar(max), and
// ct_describe() reports whatever arrives.  A real ase re-derives the
// type instead and sends a fixed INT4 where the client declared an
// INTN(4), but a fixed type has no null form, so echoing what came in
// keeps a null output parameter expressible.
void sqlrprotocol_tds::preTds7ReturnValues(sqlrservercursor *cursor) {

	debugStart("pre-tds7 return-values");

	uint16_t		outbindcount=cont->getOutputBindCount(cursor);
	sqlrserverbindvar	*outbinds=cont->getOutputBinds(cursor);
	debugWrite("outbindcount: %d",outbindcount);

	// The paramfmt and the params are two tokens but one set, and the
	// second can only be parsed by replaying the first, so both are
	// written from the same array - built here rather than in place,
	// since the return value drops out of the middle of the output
	// binds and the two tokens must not disagree about the count.
	uint16_t	count=0;
	for (uint16_t i=0; i<outbindcount && count<maxbindcount; i++) {

		// the return value went out in the returnstatus token, so
		// it isn't one of these
		if (outbindparams[i]==RPC_RETURN_VALUE_PARAM) {
			debugWrite("param %d: (return value, skipped)",i);
			continue;
		}

		uint16_t	rpcparam=outbindparams[i];

		tds5paramfmt	*fmt=&(pretds7outfmts[count]);
		fmt->name=rpcparamnames[rpcparam];
		fmt->namesize=rpcparamnamesizes[rpcparam];
		fmt->status=TDS5_PARAM_RETURN;
		// 0 rather than the systypes number a real ase echoes for a
		// decimal - the client asserts whatever arrives, and this
		// module has never sent one
		fmt->usertype=0;
		fmt->tds5type=rpcparamtds5types[rpcparam];
		fmt->mstype=rpcparamtdstypes[rpcparam];
		fmt->varintsize=preTds7VarintSize(fmt->tds5type);
		fmt->size=rpcparammaxsizes[rpcparam];
		fmt->precision=rpcparamprecisions[rpcparam];
		fmt->scale=rpcparamscales[rpcparam];

		pretds7outbinds[count]=outbinds[i];

		count++;
	}

	// a proc with no output parameters sends neither token, the way a
	// real ase sends neither
	if (!count) {
		debugWrite("no output parameters");
		debugEnd();
		return;
	}

	// the paramfmt refuses with its own error and done if the set won't
	// fit, and the params token behind it can't be parsed without it
	if (preTds7ParamFmtWrite(pretds7outfmts,count)) {
		preTds7ParamsWrite(pretds7outfmts,pretds7outbinds,count);
	}

	debugEnd();
}

bool sqlrprotocol_tds::namedProc(const char *procname, bool nometadata) {

	// this is an ordinary stored procedure call, rather than one of the
	// numbered procs

	debugStart("named-proc");

	if (!procname) {
		debugEnd();
		return rpcUnimplementedFeatureError();
	}

	debugWrite("procname: %s",procname);

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	// build the query, naming a bind variable per parameter.  a by-ref
	// parameter gets no T-SQL "output" keyword - the bind itself carries
	// the direction, and sql server rejects "output" after a parameter
	// marker outright.  odbc call syntax is used instead of "exec
	// procname ...", since only it has somewhere to put the return
	// value, via the bind variable before "=call" - which is why the
	// client's parameters start at bindvarnames[1].  the space after
	// the brace is required for beforeBindVariable() to recognize the
	// return value's marker.
	stringbuffer	query;
	query.append("{ ")->append(bindvarnames[0])->append("=call ");
	query.append(procname)->append('(');
	for (uint16_t i=0; i<rpcparamcount && i+1<maxbindcount; i++) {
		if (i) {
			query.append(',');
		}
		query.append(bindvarnames[i+1]);
	}
	query.append(")}");

	debugWrite("query: %s",query.getString());

	// run the query
	bool	success=cont->prepareQuery(cursor,
					query.getString(),query.getSize(),
					true,true,true,true);
	if (success) {
		bindParams(cursor,0,true);
		success=cont->executeQuery(cursor,true,true,true,true);
	}

	// build the response.  the two servers disagree about a procedure
	// that never ran: mssql sends no return status at all for it, unlike
	// the numbered procs, but ase sends one anyway.
	if (success) {
		rpcResultSet(cursor,nometadata,0);
		returnStatus(procReturnValue(cursor));
		procReturnValues(cursor);
	} else {
		rpcError(cursor,dbisase);
	}

	// release the cursor
	releaseCursor(cursor);

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::backendHandleProc(const char *procname,
						uint32_t handle,
						bool resultset,
						bool nometadata) {

	// runs "exec <procname> <handle> [,values...]" for a handle the
	// backend minted itself, from an "exec sp_prepare ..." inside a raw
	// batch that sqlBatch() passed straight through - so it was never
	// recorded in stmthandles.  only the handle has to be a literal;
	// the trailing values go through the normal bound-parameter path.
	// assumes the backend has sp_execute/sp_unprepare, which sql server
	// and sybase do.

	debugStart("backend-handle-proc");
	debugWrite("procname: %s",procname);
	debugWrite("handle: %d",handle);

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	// build the query, naming a bind variable per remaining parameter
	stringbuffer	query;
	query.append("exec ")->append(procname)->append(' ')->append(handle);
	for (uint16_t i=1; i<rpcparamcount && i-1<maxbindcount; i++) {
		query.append(',')->append(bindvarnames[i-1]);
	}

	debugWrite("query: %s",query.getString());

	// run the query
	bool	success=cont->prepareQuery(cursor,
					query.getString(),query.getSize(),
					true,true,true,true);
	if (success) {
		bindParams(cursor,1);
		success=cont->executeQuery(cursor,true,true,true,true);
	}

	// build the response.  a bad handle gets the backend's own error -
	// sql server error 8179 - rather than one synthesized here.
	if (success) {
		if (resultset) {
			rpcResultSet(cursor,nometadata,0);
		}
		returnStatus(RPC_STATUS_SUCCESS);
	} else {
		rpcError(cursor);
	}

	// release the cursor
	releaseCursor(cursor);

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::backendCursorExecute(uint32_t handle,
						bool nometadata) {

	// same idea as backendHandleProc(), for sp_cursorexecute against a
	// prepared-statement handle the backend minted itself.  unlike
	// sp_execute/sp_unprepare, sp_cursorexecute has output parameters of
	// its own, so every parameter, including the handle, has to ride as
	// an ordinary bind - on the odbc connection module a plain EXEC batch
	// never reports a proc's output parameters back to the driver, only
	// the "{call proc(...)}" escape does
	// (freetds accepts either form - see freetdscursor::prepareQuery())
	//
	// the result set (if any) is sent in full here rather than left for a
	// later sp_cursorfetch - the backend's own returned cursor id is a
	// raw backend cursor, not one sqlrelay tracks a sqlrservercursor
	// under, so there's no way to fetch from it afterward yet.

	debugStart("backend-cursor-execute");
	debugWrite("prepared handle: %d",handle);

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	// build the query, naming a bind variable per parameter, including
	// the handle itself
	stringbuffer	query;
	query.append("{call sp_cursorexecute(");
	for (uint16_t i=0; i<rpcparamcount && i<maxbindcount; i++) {
		if (i) {
			query.append(',');
		}
		query.append(bindvarnames[i]);
	}
	query.append(")}");

	debugWrite("query: %s",query.getString());

	// run the query
	bool	success=cont->prepareQuery(cursor,
					query.getString(),query.getSize(),
					true,true,true,true);
	if (success) {
		bindParams(cursor,0);
		success=cont->executeQuery(cursor,true,true,true,true);
	}

	// build the response.  a bad handle gets the backend's own error -
	// sql server error 8179 - rather than one synthesized here.
	if (success) {
		rpcResultSet(cursor,nometadata,0);
		returnStatus(RPC_STATUS_SUCCESS);
		procReturnValues(cursor);
	} else {
		rpcError(cursor);
	}

	// release the cursor
	releaseCursor(cursor);

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::executeSql(bool nometadata) {

	// sp_executesql @stmt, [@params, [values...]]

	debugStart("execute-sql");

	const char	*stmt=paramString(0);
	if (!stmt) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	// these two arguments are declared nvarchar
	if (!paramIsUnicode(0)) {
		debugEnd();
		return rpcParamTypeError("sp_executesql","@statement");
	}
	if (rpcparamcount>1 && !paramIsUnicode(1)) {
		debugEnd();
		return rpcParamTypeError("sp_executesql","@params");
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		debugEnd();
		return rpcQueryTooLargeError(stmtlen);
	}

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	// the parameter declaration string is only there if there are
	// parameters, and the values follow it
	uint16_t	firstvalue=(rpcparamcount>1)?2:1;

	// a statement that comes with no values has no bind variables - a
	// single-@ name in one is a local variable or parameter declaration
	if (rpcparamcount<=firstvalue) {
		cont->setTranslateBindVariablesForThisQuery(cursor,false);
	}

	bool	success=cont->prepareQuery(cursor,stmt,stmtlen,
							true,true,true,true);
	if (success) {
		bindParams(cursor,firstvalue);
		success=cont->executeQuery(cursor,true,true,true,true);
	}

	// build the response
	if (success) {
		rpcResultSet(cursor,nometadata,0);
		returnStatus(RPC_STATUS_SUCCESS);
	} else {
		rpcError(cursor);
	}

	// release the cursor
	releaseCursor(cursor);

	debugEnd();

	return true;
}

// Prepares a query on a cursor of its own and mints a handle for it.
// Shared with tds 5.0 dynamic sql, which prepares by string id and looks
// the handle up in a map of its own.
//
// "oldhandle" is the handle the caller is re-preparing, if it's
// re-preparing one - a live one's cursor is dropped first.  "exec" runs
// the query as well, with the parameters starting at "firstvalue", the
// way sp_prepexec does.
bool sqlrprotocol_tds::prepareStatement(uint32_t oldhandle,
					const char *query,
					size_t querylen,
					bool exec,
					uint16_t firstvalue,
					sqlrservercursor **cursorout,
					uint32_t *handleout) {

	*cursorout=NULL;
	*handleout=0;

	// re-preparing always mints a new handle - a backend-owned handle
	// isn't re-prepared on the backend either
	sqlrservercursor	*cursor=handleCursor(&stmthandles,oldhandle);
	if (cursor) {
		// drop the cursor a live handle was holding
		stmthandles.remove(oldhandle);
		releaseCursor(cursor);
	}

	// get an available cursor
	cursor=availableCursor();
	if (!cursor) {
		return false;
	}
	*cursorout=cursor;

	uint32_t	handle=newHandle();

	// prepare the query
	bool	success=cont->prepareQuery(cursor,query,querylen,
							true,true,true,true);

	if (success) {
		// remember how many bind markers the statement has, so that
		// an execute can ignore any parameters past the last one
		// (see executeStatement())
		bindmarkercount.setValue(cursor,
					cont->countBindVariables(
						cont->getQueryBuffer(cursor),
						cont->getQuerySize(cursor)));
		executeflag.setValue(cursor,true);
		if (exec) {
			bindParams(cursor,firstvalue);
			success=cont->executeQuery(cursor,true,true,true,true);
			executeflag.setValue(cursor,false);
		}
	}

	if (!success) {
		return false;
	}

	// hang on to the cursor - the execute will want it
	stmthandles.setValue(handle,cursor);

	*handleout=handle;

	return true;
}

// Binds and runs a query that prepareStatement() prepared.  Shared with
// tds 5.0 dynamic sql.  "firstvalue" is which parameter the values start
// at.
bool sqlrprotocol_tds::executeStatement(sqlrservercursor *cursor,
					uint16_t firstvalue) {

	bindParams(cursor,firstvalue);

	// A client can send more values than the statement has bind markers.
	// Sql server rejects the call outright, but sybase just ignores the
	// extras, and a backend that runs the statement as plain sql with
	// positional parameters (sap.cpp) has no way to ignore them - it
	// hands every bind to the database, which then rejects the whole
	// statement.  So drop the extras here instead, before they reach
	// the backend.
	uint16_t	markers=0;
	if (bindmarkercount.getValue(cursor,&markers) &&
			cont->getInputBindCount(cursor)>markers) {
		debugWrite("capping input binds at %d marker(s)",markers);
		cont->setInputBindCount(cursor,markers);
	}

	bool	success=cont->executeQuery(cursor,true,true,true,true);
	executeflag.setValue(cursor,false);

	return success;
}

// Drops a statement that prepareStatement() prepared, along with
// everything kept alongside its cursor.  Shared with tds 5.0 dynamic
// sql.
void sqlrprotocol_tds::unprepareStatement(uint32_t handle,
					sqlrservercursor *cursor) {
	stmthandles.remove(handle);
	executeflag.remove(cursor);
	bindmarkercount.remove(cursor);
	releasePositionRows(cursor);
	releaseCursor(cursor);
}

bool sqlrprotocol_tds::prepare(bool prepexec,
					bool rpcsyntax,
					bool nometadata) {

	// sp_prepare      @handle output, @params, @stmt, @options
	// sp_prepexec     @handle output, @params, @stmt, [values...]
	// sp_prepexecrpc  @handle output, @stmt, [values...]

	// the statement follows the parameter declaration string, except
	// for sp_prepexecrpc, which has no declaration string
	uint16_t	stmtparam=(rpcsyntax)?1:2;
	uint16_t	firstvalue=stmtparam+1;

	debugStart("prepare");

	const char	*stmt=paramString(stmtparam);
	if (!stmt) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	// these arguments are declared nvarchar.  a rejected call leaves the
	// handle invalid, which the client reads out of a null return value.
	const char	*pn=(rpcsyntax)?"sp_prepexecrpc":
				((prepexec)?"sp_prepexec":"sp_prepare");
	if (!rpcsyntax && !paramIsUnicode(1)) {
		rpcParamTypeError(pn,"params");
		returnValueInteger(1,0,true);
		debugEnd();
		return true;
	}
	if (!paramIsUnicode(stmtparam)) {
		rpcParamTypeError(pn,"stmt");
		returnValueInteger(1,0,true);
		debugEnd();
		return true;
	}

	// sp_prepexecrpc sends odbc call syntax rather than plain sql
	char	*query=(rpcsyntax)?
			callSyntaxToExec(stmt):charstring::duplicate(stmt);

	debugWrite("stmt: %s",stmt);
	debugWrite("query: %s",query);

	// bounds checking
	size_t	querylen=charstring::getLength(query);
	if (querylen>maxquerysize) {
		delete[] query;
		debugEnd();
		return rpcQueryTooLargeError(querylen);
	}

	sqlrservercursor	*cursor=NULL;
	uint32_t		handle=0;
	bool	success=prepareStatement((uint32_t)paramInteger(0),
						query,querylen,
						prepexec,firstvalue,
						&cursor,&handle);

	delete[] query;

	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	if (!success) {
		rpcError(cursor);
		releaseCursor(cursor);
		// the handle never became valid
		returnValueInteger(1,0,true);
		debugEnd();
		return true;
	}

	debugWrite("prepared handle: %d",handle);

	// build the response
	if (prepexec) {
		rpcResultSet(cursor,nometadata,0);
	} else if (cont->colCount(cursor)) {
		// sp_prepare with options 1 answers with the prepared
		// statement's column metadata as an empty result set, but
		// only a backend that can describe a statement without
		// running it has anything to send
		colMetaData(cursor,nometadata);
		doneInProc(DONE_COUNT,0,0);
	}
	returnStatus(RPC_STATUS_SUCCESS);

	// the client reads the handle out of the first return value
	returnValueInteger(1,(int32_t)handle,false);

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::execute(bool nometadata) {

	// sp_execute @handle, [values...]

	debugStart("execute");

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
		// a handle below the base is the backend's own, from an
		// sp_prepare inside a raw batch - run it there
		if (handle && handle<SQLRELAY_HANDLE_BASE) {
			debugEnd();
			return backendHandleProc("sp_execute",handle,
							true,nometadata);
		}
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,
					"prepared statement handle",
								handle);
	}

	debugWrite("prepared handle: %d",handle);

	// bind and run the prepared query
	bool	success=executeStatement(cursor,1);

	// build the response
	if (success) {
		rpcResultSet(cursor,nometadata,0);
		returnStatus(RPC_STATUS_SUCCESS);
	} else {
		rpcError(cursor);
	}

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::unprepare() {

	// sp_unprepare @handle

	debugStart("unprepare");

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
		// a handle below the base is the backend's own, from an
		// sp_prepare inside a raw batch - unprepare it there
		if (handle && handle<SQLRELAY_HANDLE_BASE) {
			debugEnd();
			return backendHandleProc("sp_unprepare",handle,
							false,false);
		}
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,
					"prepared statement handle",
								handle);
	}

	debugWrite("prepared handle: %d",handle);

	unprepareStatement(handle,cursor);

	returnStatus(RPC_STATUS_SUCCESS);

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::cursorPositioned() {

	// sp_cursor @cursor, @optype, @rownum, @table [, @column...]
	//
	// the positioned update/delete/insert proc.  a real sql server uses
	// "where current of", which the server API can't do, so the statement
	// is synthesized from the row's primary key values kept by
	// sp_cursorfetch - the same thing an odbc driver does for an
	// optimistic cursor.
	//
	// values arrive one parameter per column, named for the column with
	// a leading @; anything else is rejected.

	debugStart("cursor-positioned");

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&cursorhandles,handle);
	if (!cursor) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_CURSOR,
						"cursor handle",handle);
	}

	uint16_t	optype=(uint16_t)paramInteger(1);
	int32_t		rownum=(int32_t)paramInteger(2);
	const char	*table=paramString(3);

	debugWrite("cursor handle: %d",handle);
	debugWrite("optype: 0x%04x",optype);
	debugWrite("rownum: %d",rownum);
	debugWrite("table: %s",(table)?table:"(none)");

	if (charstring::isNullOrEmpty(table)) {
		debugEnd();
		return rpcNumberedError(RPC_WRONG_PARAM_TYPE,
					"sp_cursor requires a table name");
	}

	// an insert doesn't position on anything, the rest do
	tdsrow	*row=NULL;
	if (optype!=CURSOR_OP_INSERT) {
		tdsrows	*position=positionRows(cursor,false);
		row=(position)?position->getRow((uint64_t)rownum):NULL;
		if (!row) {
			debugEnd();
			return rpcNumberedError(RPC_NO_SUCH_ROW,
					"The cursor is not on a row that "
					"can be updated");
		}
	}

	switch (optype) {
		case CURSOR_OP_UPDATE:
			debugEnd();
			return positionedUpdate(cursor,table,row);
		case CURSOR_OP_DELETE:
			debugEnd();
			return positionedDelete(cursor,table,row);
		case CURSOR_OP_INSERT:
			debugEnd();
			return positionedInsert(cursor,table);
	}

	debugWrite("unsupported optype: 0x%04x",optype);
	debugEnd();

	return rpcNumberedError(RPC_OP_UNSUPPORTED,
			"Only positioned update, delete and insert "
			"are supported");
}

bool sqlrprotocol_tds::rpcNumberedError(uint32_t number,
					const char *msgtext) {

	debugStart("rpc-numbered-error");
	debugWrite("number: %d",number);
	debugWrite("%s",msgtext);
	debugEnd();

	appendError(number,1,16,msgtext,srvname,NULL,1);
	returnStatus(number);
	rpcfailed=true;

	return true;
}

const char *sqlrprotocol_tds::positionedColumn(sqlrservercursor *cursor,
							uint16_t param) {

	// a real server strips exactly one leading character off the
	// parameter name, whether or not it's an @, so a name that doesn't
	// start with one names the wrong column there and nothing here
	const char	*name=rpcparamnames[param];
	if (charstring::isNullOrEmpty(name) || *name!='@') {
		return NULL;
	}
	name++;

	// the column has to be one of the ones the cursor selected
	for (uint32_t col=0; col<cont->colCount(cursor); col++) {
		const char	*colname=cont->getColumnName(cursor,col);
		if (!charstring::compareIgnoringCase(colname,name)) {
			return colname;
		}
	}
	return NULL;
}

void sqlrprotocol_tds::positionedBind(sqlrserverbindvar *bv,
					uint16_t bindindex,
					memorypool *bindpool,
					const char *value,
					uint64_t valuesize) {
	bv->variable=bindvarnames[bindindex];
	bv->variablesize=bindvarnamesizes[bindindex];
	bulkString(bv,bindpool,value,(size_t)valuesize);
}

void sqlrprotocol_tds::positionedParamBind(sqlrserverbindvar *bv,
					uint16_t bindindex,
					memorypool *bindpool,
					uint16_t param) {

	*bv=rpcparams[param];
	bv->variable=bindvarnames[bindindex];
	bv->variablesize=bindvarnamesizes[bindindex];

	// the value has to outlive the rpc parameter pool
	if (bv->type==SQLRSERVERBINDVARTYPE_STRING && bv->value.stringval) {
		char	*value=(char *)bindpool->allocate(bv->valuesize+1);
		bytestring::copy(value,bv->value.stringval,bv->valuesize);
		value[bv->valuesize]='\0';
		bv->value.stringval=value;
	}
}

bool sqlrprotocol_tds::positionedWhere(sqlrservercursor *cursor,
					const char *table,
					tdsrow *row,
					stringbuffer *query,
					memorypool *bindpool,
					sqlrserverbindvar *binds,
					uint16_t *bindcount) {

	debugStart("positioned-where");
	debugWrite("table: %s",table);

	// split the table name into the parts getPrimaryKeysList() wants - it
	// matches the schema exactly, so an unqualified table is filled out
	// with the current schema rather than left empty, which matches
	// nothing
	char		*currentcatalog=cont->getCurrentCatalog();
	char		*currentschema=cont->getCurrentSchema();
	const char	*catalog=NULL;
	const char	*schema=NULL;
	const char	*object=NULL;
	cont->splitObjectName(currentcatalog,currentschema,"table",table,
					&catalog,&schema,&object);

	debugWrite("catalog: %s",(catalog)?catalog:"(null)");
	debugWrite("schema: %s",(schema)?schema:"(null)");
	debugWrite("object: %s",(object)?object:"(null)");

	sqlrservercursor	*pkcursor=availableCursor(cursor);
	if (!pkcursor) {
		delete[] currentcatalog;
		delete[] currentschema;
		debugEnd();
		return false;
	}

	// COLUMN_NAME is column 3 of an odbc SQLPrimaryKeys() result set.
	// the names are collected null-separated into one buffer.
	stringbuffer	keycols;
	uint16_t	keycolcount=0;
	if (cont->getPrimaryKeysList(pkcursor,catalog,schema,object)) {
		bool	error=false;
		while (cont->fetchRow(pkcursor,&error)) {
			const char	*fld=NULL;
			uint64_t	fldsize=0;
			bool		lob=false;
			bool		null=false;
			if (cont->getField(pkcursor,3,
					&fld,&fldsize,&lob,&null) &&
					!null && fld && fldsize) {
				if (keycolcount) {
					keycols.append('\0');
				}
				keycols.append(fld,(size_t)fldsize);
				keycolcount++;
			}
			cont->nextRow(pkcursor);
		}
	}
	releaseCursor(pkcursor);

	delete[] currentcatalog;
	delete[] currentschema;

	debugWrite("key columns: %d",keycolcount);

	if (!keycolcount) {
		debugWrite("no primary key on %s",table);
		debugEnd();
		return false;
	}

	// match key columns back to cursor columns
	const char	*keycol=keycols.getString();
	uint32_t	colcount=cont->colCount(cursor);
	bool		first=true;
	for (uint16_t i=0; i<keycolcount; i++) {

		uint32_t	col=0;
		while (col<colcount &&
			charstring::compareIgnoringCase(
				cont->getColumnName(cursor,col),keycol)) {
			col++;
		}
		if (col==colcount) {
			debugWrite("key column %s isn't in the result set",
								keycol);
			debugEnd();
			return false;
		}

		query->append((first)?" where ":" and ");
		query->append(keycol);
		first=false;

		// a null never compares equal, so it has to be matched
		// with "is null" rather than bound
		if (!row->values[col]) {
			query->append(" is null");
		} else if (*bindcount>=maxbindcount) {
			debugWrite("out of bind variables");
			debugEnd();
			return false;
		} else {
			query->append('=');
			query->append(bindvarnames[*bindcount]);
			positionedBind(&(binds[*bindcount]),*bindcount,
						bindpool,
						row->values[col],
						row->sizes[col]);
			(*bindcount)++;
		}

		keycol+=charstring::getLength(keycol)+1;
	}

	debugEnd();

	return true;
}

bool sqlrprotocol_tds::positionedUpdate(sqlrservercursor *cursor,
					const char *table,
					tdsrow *row) {

	// update <table> set <col>=@n, ... where <key>=@n and ...

	debugStart("positioned-update");
	debugWrite("table: %s",table);

	// a cursor of its own, held from here so the primary key lookup can't
	// take it.  the cursor the update is positioned on is off limits to
	// eviction - row points into its rows.
	sqlrservercursor	*dmlcursor=availableCursor(cursor);
	if (!dmlcursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	memorypool		*bindpool=cont->getBindPool(dmlcursor);
	bindpool->clear();
	sqlrserverbindvar	*binds=cont->getInputBinds(dmlcursor);
	uint16_t		bindcount=0;

	stringbuffer	query;
	query.append("update ")->append(table)->append(" set ");

	for (uint16_t i=4; i<rpcparamcount && bindcount<maxbindcount; i++) {

		const char	*colname=positionedColumn(cursor,i);
		if (!colname) {
			releaseCursor(dmlcursor);
			debugEnd();
			return rpcInvalidColumnError(i);
		}

		if (bindcount) {
			query.append(',');
		}
		query.append(colname)->append('=');
		query.append(bindvarnames[bindcount]);
		positionedParamBind(&(binds[bindcount]),bindcount,bindpool,i);
		bindcount++;
	}

	if (!bindcount) {
		releaseCursor(dmlcursor);
		debugEnd();
		return rpcNumberedError(RPC_WRONG_PARAM_TYPE,
				"sp_cursor update requires a column to set");
	}

	if (!positionedWhere(cursor,table,row,&query,
					bindpool,binds,&bindcount)) {
		releaseCursor(dmlcursor);
		debugEnd();
		return rpcNumberedError(RPC_CURSOR_READ_ONLY,
				"The cursor is read only - a positioned "
				"update needs a primary key that the "
				"cursor selected");
	}

	debugEnd();

	return positionedExecute(dmlcursor,query.getString(),
					query.getStringLength(),bindcount);
}

bool sqlrprotocol_tds::positionedDelete(sqlrservercursor *cursor,
					const char *table,
					tdsrow *row) {

	// delete from <table> where <key>=@n and ...

	debugStart("positioned-delete");
	debugWrite("table: %s",table);

	sqlrservercursor	*dmlcursor=availableCursor(cursor);
	if (!dmlcursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	memorypool		*bindpool=cont->getBindPool(dmlcursor);
	bindpool->clear();
	sqlrserverbindvar	*binds=cont->getInputBinds(dmlcursor);
	uint16_t		bindcount=0;

	stringbuffer	query;
	query.append("delete from ")->append(table);

	if (!positionedWhere(cursor,table,row,&query,
					bindpool,binds,&bindcount)) {
		releaseCursor(dmlcursor);
		debugEnd();
		return rpcNumberedError(RPC_CURSOR_READ_ONLY,
				"The cursor is read only - a positioned "
				"delete needs a primary key that the "
				"cursor selected");
	}

	debugEnd();

	return positionedExecute(dmlcursor,query.getString(),
					query.getStringLength(),bindcount);
}

bool sqlrprotocol_tds::positionedInsert(sqlrservercursor *cursor,
					const char *table) {

	// insert into <table> (<col>, ...) values (@n, ...)
	//
	// an insert isn't positioned on anything, so the cursor is only
	// used to work out which columns the client is allowed to name

	debugStart("positioned-insert");
	debugWrite("table: %s",table);

	sqlrservercursor	*dmlcursor=availableCursor(cursor);
	if (!dmlcursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	memorypool		*bindpool=cont->getBindPool(dmlcursor);
	bindpool->clear();
	sqlrserverbindvar	*binds=cont->getInputBinds(dmlcursor);
	uint16_t		bindcount=0;

	stringbuffer	cols;
	stringbuffer	values;

	for (uint16_t i=4; i<rpcparamcount && bindcount<maxbindcount; i++) {

		const char	*colname=positionedColumn(cursor,i);
		if (!colname) {
			releaseCursor(dmlcursor);
			debugEnd();
			return rpcInvalidColumnError(i);
		}

		if (bindcount) {
			cols.append(',');
			values.append(',');
		}
		cols.append(colname);
		values.append(bindvarnames[bindcount]);
		positionedParamBind(&(binds[bindcount]),bindcount,bindpool,i);
		bindcount++;
	}

	if (!bindcount) {
		releaseCursor(dmlcursor);
		debugEnd();
		return rpcNumberedError(RPC_WRONG_PARAM_TYPE,
				"sp_cursor insert requires a column value");
	}

	stringbuffer	query;
	query.append("insert into ")->append(table);
	query.append(" (")->append(cols.getString())->append(')');
	query.append(" values (")->append(values.getString())->append(')');

	debugEnd();

	return positionedExecute(dmlcursor,query.getString(),
					query.getStringLength(),bindcount);
}

bool sqlrprotocol_tds::rpcInvalidColumnError(uint16_t param) {

	debugStart("rpc-invalid-column-error");

	stringbuffer	msg;
	msg.append("Invalid column name '");
	msg.append((rpcparamnames[param])?rpcparamnames[param]:"");
	msg.append('\'');

	debugWrite("%s",msg.getString());

	debugEnd();

	return rpcNumberedError(RPC_INVALID_COLUMN,msg.getString());
}

bool sqlrprotocol_tds::positionedExecute(sqlrservercursor *cursor,
					const char *query,
					size_t querysize,
					uint16_t bindcount) {

	debugStart("positioned-execute");
	debugWrite("query: %s",query);
	debugWrite("binds: %d",bindcount);

	if (querysize>maxquerysize) {
		releaseCursor(cursor);
		debugEnd();
		return rpcQueryTooLargeError(querysize);
	}

	cont->setInputBindCount(cursor,bindcount);
	cont->setOutputBindCount(cursor,0);

	bool	success=cont->prepareQuery(cursor,query,querysize,
						true,true,true,true) &&
			cont->executeQuery(cursor,true,true,true,true);

	if (!success) {
		rpcError(cursor);
		releaseCursor(cursor);
		debugEnd();
		return true;
	}

	uint64_t	affectedrows=cont->getAffectedRows(cursor);

	releaseCursor(cursor);

	debugWrite("affected rows: %lld",(long long)affectedrows);

	// a positioned operation that matched nothing means the row is
	// gone, which is a failure rather than a no-op
	if (!affectedrows) {
		debugEnd();
		return rpcNumberedError(RPC_NO_ROWS_AFFECTED,
					"No rows were updated or deleted");
	}

	doneInProc(DONE_COUNT,0,affectedrows);
	returnStatus(RPC_STATUS_SUCCESS);

	debugEnd();

	return true;
}

tdsrows *sqlrprotocol_tds::positionRows(sqlrservercursor *cursor,
							bool create) {

	debugStart("position-rows");

	tdsrows	*position=NULL;
	if (positionrows.getValue(cursor,&position) && position) {
		debugWrite("existing");
		debugEnd();
		return position;
	}
	if (!create) {
		debugWrite("none, not creating");
		debugEnd();
		return NULL;
	}
	position=new tdsrows(this);
	positionrows.setValue(cursor,position);

	debugWrite("created");

	debugEnd();

	return position;
}

void sqlrprotocol_tds::releasePositionRows(sqlrservercursor *cursor) {

	debugStart("release-position-rows");

	tdsrows	*position=NULL;
	if (positionrows.getValue(cursor,&position)) {
		debugWrite("released");
		delete position;
		positionrows.remove(cursor);
	} else {
		debugWrite("none to release");
	}

	debugEnd();
}

void sqlrprotocol_tds::releaseAllPositionRows() {

	debugStart("release-all-position-rows");

	linkedlist<sqlrservercursor *>	*keys=positionrows.getKeys();
	for (listnode<sqlrservercursor *> *node=keys->getFirst();
					node; node=node->getNext()) {
		tdsrows	*position=NULL;
		if (positionrows.getValue(node->getValue(),&position)) {
			delete position;
		}
	}
	positionrows.clear();

	debugEnd();
}

bool sqlrprotocol_tds::isCursorStatement(const char *stmt) {

	// skip leading whitespace, the way a real parser would before
	// looking at the first keyword
	while (character::isWhitespace(*stmt)) {
		stmt++;
	}

	return !charstring::compareIgnoringCase(stmt,"select",6) ||
		!charstring::compareIgnoringCase(stmt,"exec",4) ||
		!charstring::compareIgnoringCase(stmt,"execute",7);
}

size_t sqlrprotocol_tds::stripForUpdateOf(const char *stmt, size_t stmtlen) {

	// mssql only accepts "for update [of col,...]" inside a DECLARE
	// CURSOR statement; this module prepares the cursor-declare text as
	// an ordinary statement instead, so the clause has to be stripped or
	// the backend rejects it
	if (forupdateof.match(stmt,stmtlen)) {
		return (size_t)(forupdateof.getSubstringStartOffset(0));
	}
	return stmtlen;
}

bool sqlrprotocol_tds::cursorOpen(bool nometadata) {

	// sp_cursoropen @cursor output, @stmt, [@scrollopt output,
	//		[@ccopt output, [@rowcount output, [values...]]]]

	debugStart("cursor-open");

	const char	*stmt=paramString(1);
	if (!stmt) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		debugEnd();
		return rpcQueryTooLargeError(stmtlen);
	}

	// a real server only accepts a select or an exec/execute as a cursor
	// statement - without this check, an update would just run as an
	// ordinary query below, turning a cursor declare into a live write
	if (!isCursorStatement(stmt)) {
		debugWrite("not a cursor statement, rejecting");
		debugEnd();
		return rpcNumberedError(RPC_OP_UNSUPPORTED,
				"A cursor may only be declared for a select "
				"or an exec statement");
	}

	stmtlen=stripForUpdateOf(stmt,stmtlen);
	debugWrite("stripped stmt: %.*s",(int)stmtlen,stmt);

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	// a parameterized cursor sends a declaration string, then the values
	uint16_t	firstvalue=6;

	// a statement that comes with no values has no bind variables - a
	// single-@ name in one is a local variable or parameter declaration
	if (rpcparamcount<=firstvalue) {
		cont->setTranslateBindVariablesForThisQuery(cursor,false);
	}

	bool	success=cont->prepareQuery(cursor,stmt,stmtlen,
							true,true,true,true);
	if (success) {
		if (rpcparamcount>firstvalue) {
			bindParams(cursor,firstvalue);
		} else {
			cont->setInputBindCount(cursor,0);
			cont->setOutputBindCount(cursor,0);
		}
		success=cont->executeQuery(cursor,true,true,true,true);
		executeflag.setValue(cursor,false);
	}

	if (!success) {
		rpcError(cursor);
		releaseCursor(cursor);
		returnValueInteger(1,0,true);
		debugEnd();
		return true;
	}

	uint32_t	handle=newHandle();
	cursorhandles.setValue(handle,cursor);

	debugWrite("cursor handle: %d",handle);

	// the client needs the shape of the result set before it fetches
	if (cont->colCount(cursor)) {
		colMetaData(cursor,nometadata);
		doneInProc(DONE_COUNT,0,0);
	} else {
		doneInProc(DONE_COUNT,0,cont->getAffectedRows(cursor));
	}

	returnStatus(RPC_STATUS_SUCCESS);

	// the client reads the cursor id out of the first return value
	returnValueInteger(1,(int32_t)handle,false);

	// the server API has no scrollable cursor support - only forward-only
	// skipRow/skipRows.  both of these are in/out, and the spec lets the
	// server substitute what it can do.
	returnValueInteger(2,CURSOR_SCROLLOPT_FORWARD_ONLY,false);
	returnValueInteger(3,CURSOR_CCOPT_READ_ONLY,false);

	// -1 means the row count isn't known yet, which is what a
	// forward-only cursor reports
	returnValueInteger(4,-1,false);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::cursorPrepare() {

	// sp_cursorprepare @handle output, @params, @stmt, @options
	//			[, @scrollopt output [, @ccopt output]]

	debugStart("cursor-prepare");

	const char	*stmt=paramString(2);
	if (!stmt) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		debugEnd();
		return rpcQueryTooLargeError(stmtlen);
	}

	stmtlen=stripForUpdateOf(stmt,stmtlen);
	debugWrite("stripped stmt: %.*s",(int)stmtlen,stmt);

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	bool	success=cont->prepareQuery(cursor,stmt,stmtlen,
							true,true,true,true);
	if (!success) {
		rpcError(cursor);
		releaseCursor(cursor);
		returnValueInteger(1,0,true);
		debugEnd();
		return true;
	}

	executeflag.setValue(cursor,true);

	uint32_t	handle=newHandle();
	stmthandles.setValue(handle,cursor);

	debugWrite("prepared handle: %d",handle);

	returnStatus(RPC_STATUS_SUCCESS);
	returnValueInteger(1,(int32_t)handle,false);
	returnValueInteger(2,CURSOR_SCROLLOPT_FORWARD_ONLY,false);
	returnValueInteger(3,CURSOR_CCOPT_READ_ONLY,false);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::cursorExecute(bool nometadata) {

	// sp_cursorexecute @preparedhandle, @cursor output,
	//			[@scrollopt output, [@ccopt output,
	//			[@rowcount output, [values...]]]]

	debugStart("cursor-execute");

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
		// a handle below the base is the backend's own, from an
		// sp_prepare inside a raw batch - run it there
		if (handle && handle<SQLRELAY_HANDLE_BASE) {
			debugWrite("backend handle: %d",handle);
			debugEnd();
			return backendCursorExecute(handle,nometadata);
		}
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,
					"prepared statement handle",
								handle);
	}

	debugWrite("prepared handle: %d",handle);

	// bind and run the prepared query
	if (rpcparamcount>5) {
		bindParams(cursor,5);
	}
	bool	success=cont->executeQuery(cursor,true,true,true,true);
	executeflag.setValue(cursor,false);

	// re-running the statement leaves the cursor sitting on nothing, so
	// the rows the last fetch positioned on have to go with it, or an
	// sp_cursor before the next fetch would update the wrong row
	releasePositionRows(cursor);

	if (!success) {
		rpcError(cursor);
		returnValueInteger(1,0,true);
		debugEnd();
		return true;
	}

	// the cursor gets its own handle, so that the prepared statement
	// can outlive it
	uint32_t	cursorhandle=newHandle();
	cursorhandles.setValue(cursorhandle,cursor);

	debugWrite("cursor handle: %d",cursorhandle);

	if (cont->colCount(cursor)) {
		colMetaData(cursor,nometadata);
		doneInProc(DONE_COUNT,0,0);
	} else {
		doneInProc(DONE_COUNT,0,cont->getAffectedRows(cursor));
	}

	returnStatus(RPC_STATUS_SUCCESS);
	returnValueInteger(1,(int32_t)cursorhandle,false);
	returnValueInteger(2,CURSOR_SCROLLOPT_FORWARD_ONLY,false);
	returnValueInteger(3,CURSOR_CCOPT_READ_ONLY,false);
	returnValueInteger(4,-1,false);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::cursorPrepExec(bool nometadata) {

	// sp_cursorprepexec @handle output, @cursor output, @params, @stmt,
	//			[@scrollopt output, [@ccopt output,
	//			[@rowcount output, [values...]]]]

	debugStart("cursor-prep-exec");

	const char	*stmt=paramString(3);
	if (!stmt) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		debugEnd();
		return rpcQueryTooLargeError(stmtlen);
	}

	stmtlen=stripForUpdateOf(stmt,stmtlen);
	debugWrite("stripped stmt: %.*s",(int)stmtlen,stmt);

	// get an available cursor
	sqlrservercursor	*cursor=availableCursor();
	if (!cursor) {
		debugEnd();
		return rpcNoCursorAvailableError();
	}

	bool	success=cont->prepareQuery(cursor,stmt,stmtlen,
							true,true,true,true);
	if (success) {
		if (rpcparamcount>7) {
			bindParams(cursor,7);
		} else {
			cont->setInputBindCount(cursor,0);
			cont->setOutputBindCount(cursor,0);
		}
		success=cont->executeQuery(cursor,true,true,true,true);
		executeflag.setValue(cursor,false);
	}

	if (!success) {
		rpcError(cursor);
		releaseCursor(cursor);
		returnValueInteger(1,0,true);
		returnValueInteger(2,0,true);
		debugEnd();
		return true;
	}

	uint32_t	handle=newHandle();
	stmthandles.setValue(handle,cursor);
	uint32_t	cursorhandle=newHandle();
	cursorhandles.setValue(cursorhandle,cursor);

	debugWrite("prepared handle: %d",handle);
	debugWrite("cursor handle: %d",cursorhandle);

	if (cont->colCount(cursor)) {
		colMetaData(cursor,nometadata);
		doneInProc(DONE_COUNT,0,0);
	} else {
		doneInProc(DONE_COUNT,0,cont->getAffectedRows(cursor));
	}

	returnStatus(RPC_STATUS_SUCCESS);
	returnValueInteger(1,(int32_t)handle,false);
	returnValueInteger(2,(int32_t)cursorhandle,false);
	returnValueInteger(3,CURSOR_SCROLLOPT_FORWARD_ONLY,false);
	returnValueInteger(4,CURSOR_CCOPT_READ_ONLY,false);
	returnValueInteger(5,-1,false);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::cursorUnprepare() {

	// sp_cursorunprepare @handle

	debugStart("cursor-unprepare");

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
		// a handle below the base is the backend's own, from an
		// sp_prepare inside a raw batch - unprepare it there
		if (handle && handle<SQLRELAY_HANDLE_BASE) {
			debugWrite("backend handle: %d",handle);
			debugEnd();
			return backendHandleProc("sp_cursorunprepare",handle,
							false,false);
		}
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,
					"prepared statement handle",
								handle);
	}

	debugWrite("prepared handle: %d",handle);

	stmthandles.remove(handle);

	// the cursor may still be open against this statement
	if (!handlesContain(&cursorhandles,cursor)) {
		executeflag.remove(cursor);
		releasePositionRows(cursor);
		releaseCursor(cursor);
	}

	returnStatus(RPC_STATUS_SUCCESS);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::cursorFetch(bool nometadata) {

	// sp_cursorfetch @cursor, [@fetchtype, [@rownum, [@nrows]]]

	debugStart("cursor-fetch");

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&cursorhandles,handle);
	if (!cursor) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_CURSOR,
						"cursor handle",handle);
	}

	uint16_t	fetchtype=(rpcparamcount>1)?
				(uint16_t)paramInteger(1):CURSOR_FETCH_NEXT;
	int32_t		rownum=(rpcparamcount>2)?
				(int32_t)paramInteger(2):0;
	int32_t		nrows=(rpcparamcount>3)?
				(int32_t)paramInteger(3):1;

	debugWrite("cursor handle: %d",handle);
	debugWrite("fetchtype: 0x%04x",fetchtype);
	debugWrite("rownum: %d",rownum);
	debugWrite("nrows: %d",nrows);

	// the query may not have run yet
	if (executeflag.getValue(cursor)) {
		if (!cont->executeQuery(cursor,true,true,true,true)) {
			rpcError(cursor);
			debugEnd();
			return true;
		}
		executeflag.setValue(cursor,false);
	}

	// only forward-only fetching is possible, so anything that would go
	// backwards or jump is refused rather than answered with wrong rows
	switch (fetchtype) {
		case CURSOR_FETCH_FIRST:
		case CURSOR_FETCH_NEXT:
		case CURSOR_FETCH_INFO:
			break;
		case CURSOR_FETCH_RELATIVE:
			// forward is just a skip
			if (rownum>0) {
				bool	error=false;
				cont->skipRows(cursor,(uint64_t)rownum,&error);
				break;
			}
			// fall through
		default:
			debugWrite("unsupported fetch type: 0x%04x",fetchtype);
			debugEnd();
			return rpcNumberedError(RPC_FETCH_UNSUPPORTED,
					"Only forward-only cursors "
					"are supported");
	}

	// fetch-info just reports what's known about the cursor
	if (fetchtype==CURSOR_FETCH_INFO) {
		doneInProc(DONE_COUNT,0,0);
		returnStatus(RPC_STATUS_SUCCESS);
		debugEnd();
		return true;
	}

	// send the rows, keeping their values so that an sp_cursor that
	// follows can build a where clause out of the row it lands on
	if (cont->colCount(cursor)) {
		colMetaData(cursor,nometadata);
		doneInProc(DONE_COUNT,0,
			rows(cursor,(nrows>0)?(uint64_t)nrows:0,
					positionRows(cursor,true)));
	} else {
		doneInProc(DONE_COUNT,0,0);
	}

	returnStatus(RPC_STATUS_SUCCESS);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::cursorOption() {

	// sp_cursoroption @cursor, @code, @value

	debugStart("cursor-option");

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&cursorhandles,handle);
	if (!cursor) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_CURSOR,
						"cursor handle",handle);
	}

	debugWrite("cursor handle: %d",handle);
	debugWrite("code: %lld",(long long)paramInteger(1));
	debugWrite("value: %lld",(long long)paramInteger(2));

	// none of the options - text pointers, scroll options, cursor name -
	// change anything this module can do, so just accept them

	returnStatus(RPC_STATUS_SUCCESS);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::cursorClose() {

	// sp_cursorclose @cursor

	debugStart("cursor-close");

	uint32_t	handle=(uint32_t)paramInteger(0);

	debugWrite("cursor handle: %d",handle);

	// -1 means close them all
	if (handle==CURSOR_CLOSE_ALL) {
		releaseHandles(&cursorhandles,&stmthandles);
		returnStatus(RPC_STATUS_SUCCESS);
		debugEnd();
		return true;
	}

	sqlrservercursor	*cursor=handleCursor(&cursorhandles,handle);
	if (!cursor) {
		debugEnd();
		return rpcInvalidHandleError(RPC_NO_SUCH_CURSOR,
						"cursor handle",handle);
	}

	cursorhandles.remove(handle);

	// the prepared statement it came from may still be live
	if (!handlesContain(&stmthandles,cursor)) {
		executeflag.remove(cursor);
		releasePositionRows(cursor);
		releaseCursor(cursor);
	}

	returnStatus(RPC_STATUS_SUCCESS);

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::params(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout) {

	debugStart("params");

	// reset the pool that parameter values get copied into
	rpcparampool.clear();
	rpcparamcount=0;

	bool		exceeded=false;
	while (rpsize) {

		// the batch flags follow the last parameter, and a parameter
		// starts with its name length - nothing in the packet says
		// which comes next, but no name is anywhere near that long
		if (*rp==RPC_BATCH_FLAG || *rp==RPC_NO_EXEC_FLAG) {
			break;
		}

		size_t	oldrpsize=rpsize;

		if (!param(rpcparamcount,&rp,&rpsize,exceeded)) {
			// protocol error
			debugEnd();
			return false;
		}

		if (!exceeded) {
			rpcparamcount++;
			if (rpcparamcount==maxbindcount) {
				exceeded=true;
			}
		}

		// param() only ever shrinks rpsize, so a parameter that
		// consumed nothing is the one way this loop could spin
		// forever
		if (rpsize>=oldrpsize) {
			debugWrite("parameter consumed nothing");
			debugEnd();
			return false;
		}
	}

	debugWrite("param count: %d",rpcparamcount);

	// copy out pointer and size
	*rpout=rp;
	*rpsizeout=rpsize;

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::param(uint16_t param,
					const byte_t **rpinout,
					size_t *rpsizeinout,
					bool exceeded) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	// param name
	if (!rpsize) {
		return false;
	}
	byte_t	pnamelen;
	read(rp,&pnamelen,&rp);
	rpsize--;
	ucs2_t	*pname16=NULL;
	char	*pname=NULL;
	if (pnamelen) {
		// the name length is in characters, but the data is ucs-2
		size_t	pnamesize=((size_t)pnamelen)*sizeof(ucs2_t);
		if (rpsize<pnamesize) {
			return false;
		}
		pname16=new ucs2_t[pnamelen];
		read(rp,pname16,pnamelen,&rp);
		rpsize-=pnamesize;
		size_t	pname8size;
		pname=ucs2ToUtf8(pname16,(size_t)pnamelen,&pname8size);
	}


	// status flags
	if (!rpsize) {
		delete[] pname16;
		delete[] pname;
		return false;
	}
	byte_t	statusflags=0;
	read(rp,&statusflags,&rp);
	rpsize--;
	bool	byrefvalue=(statusflags&0x01);
	bool	defaultvalue=(statusflags&(0x01<<1))>>1;
	// this bit is reserved
	bool	encrypted=(statusflags&(0x01<<3))>>3;
	// these 4 bits are reserved


	// FIXME: do something if defaultvalue is set...
	// FIXME: support encryption


	// the parameters are kept as they arrived - which of them are bind
	// values at all depends on which proc was called, and only that
	// proc's handler knows
	sqlrserverbindvar	*bv=NULL;
	if (!exceeded) {

		bv=&(rpcparams[param]);
		bv->type=SQLRSERVERBINDVARTYPE_NULL;
		bv->variable=NULL;
		bv->variablesize=0;
		bv->valuesize=0;
		bv->value.stringval=NULL;
		bv->isnull=cont->getNullBindValue();

		rpcparambyref[param]=byrefvalue;
		rpcparamnames[param]=(char *)rpcparampool.allocate(pnamelen+1);
		if (pnamelen) {
			charstring::copy(rpcparamnames[param],pname,pnamelen);
		}
		rpcparamnames[param][pnamelen]='\0';
		rpcparamnamesizes[param]=pnamelen;
	}


	// debug
	if (getDebug()) {
		debugStart("param %d",(bv)?param:-1);
		debugWrite("pnamelen: %d",pnamelen);
		debugWrite("pname: %s",pname);
		stringbuffer	b;
		b.printBits(statusflags);
		debugWrite("statusflags: %s",b.getString());
		debugWrite("byrefvalue: %d",byrefvalue);
		debugWrite("defaultvalue: %d",defaultvalue);
		debugWrite("encrypted: %d",encrypted);
	}

	bool	retval=paramValue(param,&rp,&rpsize,bv);

	debugEnd();

	// clean up
	delete[] pname16;
	delete[] pname;

	return retval;
}

bool sqlrprotocol_tds::parseXmlInfo(const byte_t **rpinout,
					size_t *rpsizeinout) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	// XML_INFO (MS-TDS 2.2.5.5.2) is a SchemaPresent byte and, only when
	// it's set, three ucs-2 strings naming the schema collection: a
	// 1-byte-counted dbname and owning schema, then a 2-byte-counted
	// collection name.  All three counts are in characters rather than
	// bytes.  Nothing here uses the names, so they're just skipped.
	if (!rpsize) {
		return false;
	}
	byte_t	schemapresent;
	read(rp,&schemapresent,&rp);
	rpsize--;
	debugWrite("schemapresent: %d",schemapresent);

	if (!schemapresent) {
		return true;
	}

	for (uint8_t i=0; i<3; i++) {

		uint16_t	length;
		if (i<2) {
			if (!rpsize) {
				return false;
			}
			byte_t	blength;
			read(rp,&blength,&rp);
			rpsize--;
			length=blength;
		} else {
			if (rpsize<sizeof(uint16_t)) {
				return false;
			}
			readLE(rp,&length,&rp);
			rpsize-=sizeof(uint16_t);
		}

		size_t	size=length*sizeof(ucs2_t);
		if (rpsize<size) {
			return false;
		}
		rp+=size;
		rpsize-=size;
	}

	return true;
}

bool sqlrprotocol_tds::plpValue(const byte_t **rpinout,
					size_t *rpsizeinout,
					byte_t tdstype,
					sqlrserverbindvar *bv,
					memorypool *bindpool) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	debugStart("plp value");

	// PLP_BODY (MS-TDS 2.2.5.2.3) is an 8-byte total length, then a run of
	// chunks - each a 4-byte length followed by that many bytes - ending
	// in a zero-length chunk.  Two total lengths are sentinels rather than
	// counts: PLP_NULL means the value is null and nothing follows at all,
	// and UNKNOWN_PLP_LEN means the sender didn't know the total up front.
	// The chunks run to the terminator either way, so the total length is
	// only ever a hint and the terminator is what actually ends the value.
	if (rpsize<sizeof(uint64_t)) {
		debugEnd();
		return false;
	}
	uint64_t	totalsize;
	readLE(rp,&totalsize,&rp);
	rpsize-=sizeof(uint64_t);

	if (totalsize==TDS_PLP_NULL) {
		if (bv) {
			// a varbinary(max)/binary(max) column is the only
			// plp type that ends up a blob below, so it's the
			// only one that needs to stay one here too
			bv->type=(tdstype==TDS_TYPE_BIGVARBIN)?
					SQLRSERVERBINDVARTYPE_NULLBLOB:
					SQLRSERVERBINDVARTYPE_NULL;
			bv->isnull=cont->getNullBindValue();
		}
		debugWrite("value: (null)");
		debugEnd();
		return true;
	}

	if (totalsize==TDS_PLP_UNKNOWN_LEN) {
		debugWrite("totalsize: (unknown)");
	} else {
		debugWrite("totalsize: %lld",(long long)totalsize);
	}

	bytebuffer	data;
	for (;;) {

		if (rpsize<sizeof(uint32_t)) {
			debugEnd();
			return false;
		}
		uint32_t	chunksize;
		readLE(rp,&chunksize,&rp);
		rpsize-=sizeof(uint32_t);

		if (!chunksize) {
			break;
		}

		if (rpsize<chunksize) {
			debugEnd();
			return false;
		}
		data.append(rp,chunksize);
		rp+=chunksize;
		rpsize-=chunksize;
	}

	size_t		size=data.getSize();
	const byte_t	*value=data.getBuffer();

	debugWrite("valuesize: %lld",(long long)size);

	if (bv) {
		switch (tdstype) {
			case TDS_TYPE_BIGVARBIN:
				bulkBinary(bv,bindpool,value,size);
				break;
			case TDS_TYPE_NVARCHAR:
			case TDS_TYPE_XML:
				{
				// the chunks are ucs-2,
				// and their lengths are in bytes
				size_t	length=size/sizeof(ucs2_t);

				// the data isn't necessarily aligned,
				// so copy it out before converting it
				const byte_t	*dummy;
				ucs2_t		*value16=new ucs2_t[length];
				read(value,value16,length,&dummy);
				size_t	utf8size;
				char	*utf8=ucs2ToUtf8(value16,
							length,&utf8size);
				delete[] value16;

				bulkString(bv,bindpool,utf8,utf8size);

				delete[] utf8;
				}
				break;
			default:
				bulkString(bv,bindpool,
						(const char *)value,size);
				break;
		}
	}

	debugEnd();
	return true;
}

bool sqlrprotocol_tds::paramValue(uint16_t param,
					const byte_t **rpinout,
					size_t *rpsizeinout,
					sqlrserverbindvar *bv) {

	const byte_t	*&rp=*rpinout;
	size_t		&rpsize=*rpsizeinout;

	debugStart("param-value");
	debugWrite("param: %d",param);

	// type info...
	if (!rpsize) {
		debugEnd();
		return false;
	}
	byte_t	tdstype;
	read(rp,&tdstype,&rp);
	rpsize--;
	debugColumnType(tdstype);

	uint32_t	maxsize=0;
	byte_t		precision=0;
	byte_t		scale=0;

	// Whether the value below is partially length prefixed.  bigvarchr,
	// bigvarbin and nvarchar are var-len or part-len depending on the
	// USHORTMAXLEN that follows the type byte, not on the type byte alone,
	// so this can only be decided once that's been read.  xml and udt are
	// always part-len.
	bool		partlen=false;

	if (isFixedLenType(tdstype)) {

		debugWrite("fixedlentype...");

	} else if (isVarLenType(tdstype)) {

		debugWrite("varlentype...");

		// maxsize
		switch (tdstype) {
			case TDS_TYPE_SSVARIANT:
			case TDS_TYPE_TEXT:
			case TDS_TYPE_NTEXT:
			case TDS_TYPE_IMAGE:
				if (rpsize<sizeof(uint32_t)) {
					debugEnd();
					return false;
				}
				readLE(rp,&maxsize,&rp);
				rpsize-=sizeof(uint32_t);
				debugWrite("maxsize: %d",maxsize);
				break;
			case TDS_TYPE_BIGCHAR:
			case TDS_TYPE_BIGVARCHR:
			case TDS_TYPE_NCHAR:
			case TDS_TYPE_NVARCHAR:
			case TDS_TYPE_BIGBINARY:
			case TDS_TYPE_BIGVARBIN:
				{
				if (rpsize<sizeof(uint16_t)) {
					debugEnd();
					return false;
				}
				uint16_t	size;
				readLE(rp,&size,&rp);
				rpsize-=sizeof(uint16_t);
				if (size==TDS_USHORTMAXLEN) {
					// the max forms - varchar(max),
					// nvarchar(max), varbinary(max) -
					// declare no maxsize at all and carry
					// a plp value instead of a plainly
					// length prefixed one
					partlen=true;
					debugWrite("maxsize: (max)");
				} else {
					maxsize=size;
					debugWrite("maxsize: %d",maxsize);
				}
				}
				break;
			case TDS_TYPE_XML:
				// XML_INFO (MS-TDS 2.2.5.5.2) - a
				// SchemaPresent byte, then the schema
				// collection if it's set.  No maxsize, and
				// the value is always plp.
				if (!parseXmlInfo(&rp,&rpsize)) {
					debugEnd();
					return false;
				}
				partlen=true;
				break;
			case TDS_TYPE_DATEN:
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				// typeInfo() never sends a maxsize for these,
				// so don't read one here either
				break;
			default:
				{
				if (!rpsize) {
					debugEnd();
					return false;
				}
				byte_t	size;
				read(rp,&size,&rp);
				rpsize--;
				maxsize=size;
				debugWrite("maxsize: %d",maxsize);
				}
				break;
		}

		// precision
		switch (tdstype) {
			case TDS_TYPE_NUMERIC:
			case TDS_TYPE_NUMERICN:
			case TDS_TYPE_DECIMAL:
			case TDS_TYPE_DECIMALN:
				if (!rpsize) {
					debugEnd();
					return false;
				}
				read(rp,&precision,&rp);
				rpsize--;
				debugWrite("precision: %d",precision);
				break;
		}

		// scale
		switch (tdstype) {
			case TDS_TYPE_NUMERIC:
			case TDS_TYPE_NUMERICN:
			case TDS_TYPE_DECIMAL:
			case TDS_TYPE_DECIMALN:
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				if (!rpsize) {
					debugEnd();
					return false;
				}
				read(rp,&scale,&rp);
				rpsize--;
				debugWrite("scale: %d",scale);
				break;
		}

	}

	// no isPartLenType() arm here on purpose - every part-len type is a
	// var-len type too, and which shape a given parameter actually has
	// depends on its maxsize, so the arm above reads the type info for
	// both and sets partlen for the value read below
	// an output parameter goes back as the type the client declared, and
	// tdstype is rewritten just below to read the value
	if (bv) {
		rpcparamtdstypes[param]=tdstype;
		// this parameter arrived over ms-tds, so it has no tds 5.0
		// type byte to echo.  0 rather than left alone: a tds 5.0
		// session that sends an ms-tds rpc packet still answers it
		// through preTds7ReturnValues(), which reads this.
		rpcparamtds5types[param]=0;
		rpcparammaxsizes[param]=maxsize;
		rpcparamprecisions[param]=precision;
		rpcparamscales[param]=scale;
	}

	// param data...

	// FIXME: handle output binds too

	// get size, resolve *n type to a concrete type
	// a size of 0 means the value is null
	switch (tdstype) {
		case TDS_TYPE_INTN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			switch (size) {
				case 0:
					tdstype=TDS_TYPE_NULL;
					break;
				case 1:
					tdstype=TDS_TYPE_INT1;
					break;
				case 2:
					tdstype=TDS_TYPE_INT2;
					break;
				case 4:
					tdstype=TDS_TYPE_INT4;
					break;
				case 8:
					tdstype=TDS_TYPE_INT8;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
		case TDS_TYPE_BITN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				tdstype=TDS_TYPE_NULL;
			} else if (size==1) {
				tdstype=TDS_TYPE_BIT;
			} else {
				debugWrite("invalid size: %d",size);
				debugEnd();
				return false;
			}
			}
			break;
		case TDS_TYPE_FLTN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			switch (size) {
				case 0:
					tdstype=TDS_TYPE_NULL;
					break;
				case 4:
					tdstype=TDS_TYPE_FLT4;
					break;
				case 8:
					tdstype=TDS_TYPE_FLT8;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
		case TDS_TYPE_MONEYN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			switch (size) {
				case 0:
					tdstype=TDS_TYPE_NULL;
					break;
				case 4:
					tdstype=TDS_TYPE_MONEY4;
					break;
				case 8:
					tdstype=TDS_TYPE_MONEY;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
		case TDS_TYPE_DATETIMN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			switch (size) {
				case 0:
					tdstype=TDS_TYPE_NULL;
					break;
				case 4:
					tdstype=TDS_TYPE_DATETIM4;
					break;
				case 8:
					tdstype=TDS_TYPE_DATETIME;
					break;
				default:
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
			}
			}
			break;
	}

	if (tdstype==TDS_TYPE_NULL) {
		debugWrite("value: (null)");
	}

	// get the collation
	// (7.0 has no collation at all - typeInfo() gates it the same way)
	if (negotiatedtdsversion>=710) {
		switch (tdstype) {
			case TDS_TYPE_BIGCHAR:
			case TDS_TYPE_BIGVARCHR:
			case TDS_TYPE_TEXT:
			case TDS_TYPE_NTEXT:
			case TDS_TYPE_NCHAR:
			case TDS_TYPE_NVARCHAR:
				{
				// FIXME: do something with this
				if (rpsize<5) {
					debugEnd();
					return false;
				}
				byte_t	coll[5];
				read(rp,coll,sizeof(coll),&rp);
				rpsize-=sizeof(coll);
				if (getDebug()) {
					stringbuffer	b;
					b.printBits(coll,sizeof(coll));
					debugWrite("collation: %s",
								b.getString());
				}
				}
				break;
		}
	}

	// a max type's value is plp framed rather than plainly
	// length prefixed, so it needs its own reader
	if (partlen && tdstype!=TDS_TYPE_NULL) {
		bool	retval=plpValue(&rp,&rpsize,tdstype,bv,
							&rpcparampool);
		debugEnd();
		return retval;
	}

	// get the data
	switch (tdstype) {
		case TDS_TYPE_NULL:
			break;
		case TDS_TYPE_INT1:
		case TDS_TYPE_BIT:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			char	val;
			read(rp,(byte_t *)&val,&rp);
			rpsize--;

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=1;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_INT2:
			{

			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			int16_t	val;
			readLE(rp,(uint16_t *)&val,&rp);
			rpsize-=sizeof(uint16_t);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=2;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}
	
			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_INT4:
			{

			if (rpsize<sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			int32_t	val;
			readLE(rp,(uint32_t *)&val,&rp);
			rpsize-=sizeof(uint32_t);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=4;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_DATETIM4:
			{
			if (rpsize<2*sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	days;
			uint16_t	minutes;
			readLE(rp,&days,&rp);
			readLE(rp,&minutes,&rp);
			rpsize-=2*sizeof(uint16_t);

			if (bv) {
				dateTimeValue(days,minutes*60*300,bv);
			}
			}
			break;
		case TDS_TYPE_FLT4:
			{

			if (rpsize<sizeof(float)) {
				debugEnd();
				return false;
			}
			float	val;
			read(rp,(float *)&val,&rp);
			rpsize-=sizeof(float);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.doubleval.value=val;

				// FIXME: kludgy
				char	*num=charstring::parseNumber(
						bv->value.doubleval.value);
				size_t	size=charstring::getLength(num);
				bv->value.doubleval.precision=size-
					(charstring::contains(num,'-')?1:0)-
					(charstring::contains(num,'.')?1:0);
				bv->value.doubleval.scale=
					(num+size)-
					charstring::findFirstOrEnd(num,'.')-1;
				delete[] num;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %f",bv->value.doubleval.value);
				debugWrite("precision: %d",
						bv->value.doubleval.
						precision);
				debugWrite("scale: %d",
						bv->value.doubleval.
						scale);
			}
			}
			break;
		case TDS_TYPE_MONEY:
			{
			if (rpsize<2*sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			uint32_t	high;
			uint32_t	low;
			readLE(rp,&high,&rp);
			readLE(rp,&low,&rp);
			rpsize-=2*sizeof(uint32_t);

			if (bv) {
				moneyValue((int64_t)((((uint64_t)high)<<32)|
								low),bv);
			}
			}
			break;
		case TDS_TYPE_DATETIME:
			{
			if (rpsize<2*sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			readLE(rp,(uint32_t *)&dayssince1900,&rp);
			readLE(rp,&threehundredths,&rp);
			rpsize-=2*sizeof(uint32_t);

			if (bv) {
				dateTimeValue(dayssince1900,
						threehundredths,bv);
			}
			}
			break;
		case TDS_TYPE_FLT8:
			{

			if (rpsize<sizeof(double)) {
				debugEnd();
				return false;
			}
			double	val;
			read(rp,(double *)&val,&rp);
			rpsize-=sizeof(double);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.doubleval.value=val;

				// FIXME: kludgy
				char	*num=charstring::parseNumber(
						bv->value.doubleval.value);
				size_t	size=charstring::getLength(num);
				bv->value.doubleval.precision=size-
					(charstring::contains(num,'-')?1:0)-
					(charstring::contains(num,'.')?1:0);
				bv->value.doubleval.scale=
					(num+size)-
					charstring::findFirstOrEnd(num,'.')-1;
				delete[] num;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %f",bv->value.doubleval.value);
				debugWrite("precision: %d",
						bv->value.doubleval.
						precision);
				debugWrite("scale: %d",
						bv->value.doubleval.
						scale);
			}
			}
			break;
		case TDS_TYPE_MONEY4:
			{
			if (rpsize<sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			int32_t	val;
			readLE(rp,(uint32_t *)&val,&rp);
			rpsize-=sizeof(uint32_t);

			if (bv) {
				moneyValue(val,bv);
			}
			}
			break;
		case TDS_TYPE_INT8:
			{

			if (rpsize<sizeof(uint64_t)) {
				debugEnd();
				return false;
			}
			int64_t	val;
			readLE(rp,(uint64_t *)&val,&rp);
			rpsize-=sizeof(uint64_t);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=8;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_GUID:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			if (size!=TDS_GUID_SIZE) {
				// any other size means null
				debugWrite("value: (null)");
				rp+=size;
				rpsize-=size;
				break;
			}

			if (bv) {

				// ms-tds sends a guid with its first three
				// fields byte-reversed with respect to the
				// canonical string form, and its last two
				// fields in string order - the reverse of
				// the swap guid() applies on the way out,
				// which is the same swap, since swapping
				// pairs is its own inverse
				static const uint16_t	order[]={
					3,2,1,0, 5,4, 7,6,
					8,9, 10,11,12,13,14,15
				};
				static const char	hex[]="0123456789ABCDEF";

				bv->type=SQLRSERVERBINDVARTYPE_STRING;
				bv->valuesize=36;
				bv->value.stringval=(char *)
					rpcparampool.allocate(
						bv->valuesize+1);
				char	*out=bv->value.stringval;
				for (uint16_t i=0; i<TDS_GUID_SIZE; i++) {
					if (i==4 || i==6 || i==8 || i==10) {
						*(out++)='-';
					}
					byte_t	b=rp[order[i]];
					*(out++)=hex[(b>>4)&0x0f];
					*(out++)=hex[b&0x0f];
				}
				*out='\0';
				bv->isnull=cont->getNonNullBindValue();

				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %s",bv->value.stringval);
			}

			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_DECIMAL:
		case TDS_TYPE_NUMERIC:
		case TDS_TYPE_DECIMALN:
		case TDS_TYPE_NUMERICN:
			{
			byte_t	ispositive;
			byte_t	val[TDS_DECIMAL_MAX_SIZE-1];
			byte_t	valsize;
			if (tdstype==TDS_TYPE_DECIMALN ||
				tdstype==TDS_TYPE_NUMERICN) {
				if (!rpsize) {
					debugEnd();
					return false;
				}
				byte_t	size;
				read(rp,&size,&rp);
				rpsize--;
				// a length of 0 is how the protocol says
				// null, and size-1 below would be -1
				if (!size) {
					debugWrite("value: (null)");
					break;
				}
				// without this, a client-chosen size writes
				// up to 254 bytes over a 16 byte stack array
				if (size>sizeof(val)+1) {
					debugWrite("invalid size: %d",size);
					debugEnd();
					return false;
				}
				if (rpsize<size) {
					debugEnd();
					return false;
				}
				read(rp,&ispositive,&rp);
				read(rp,val,size-1,&rp);
				rpsize-=size;
				valsize=size-1;
			} else {
				if (rpsize<1+sizeof(val)) {
					debugEnd();
					return false;
				}
				read(rp,&ispositive,&rp);
				read(rp,val,sizeof(val),&rp);
				rpsize-=1+sizeof(val);
				valsize=(byte_t)sizeof(val);
			}

			if (bv) {

				// FIXME: anything wider than 8 bytes
				// overflows
				stringbuffer	strb;
				bulkDecimal(ispositive,val,
						(valsize>8)?8:valsize,
						scale,&strb);

				// bound as a number rather than a string -
				// mssql converts a varchar to a decimal on
				// its own but ase refuses to ("Implicit
				// conversion from datatype 'VARCHAR' to
				// 'DECIMAL' is not allowed")
				bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.doubleval.value=
					(double)charstring::convertToFloat(
							strb.getString());
				// FIXME: kludgy, but the same thing
				// bulkDouble() does
				bv->value.doubleval.precision=
					(uint32_t)charstring::getLength(
							strb.getString())-
					((charstring::contains(
						strb.getString(),'-'))?1:0)-
					((charstring::contains(
						strb.getString(),'.'))?1:0);
				bv->value.doubleval.scale=scale;
				debugWrite("value: %s",strb.getString());
			}
			}
			break;
		case TDS_TYPE_DATEN:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				break;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			uint32_t	dayssince1=0;
			for (byte_t i=0; i<size && i<3; i++) {
				dayssince1|=((uint32_t)rp[i])<<(i*8);
			}
			rp+=size;
			rpsize-=size;
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_TIMEN:
		case TDS_TYPE_DATETIME2N:
		case TDS_TYPE_DATETIMEOFFSETN:
			{
			// FIXME:
			// timen is 1 unsigned integer - number of 10^-n
			// 			second increments since 12 am
			// 			within a day.
			// 3 bytes if 0 <= n <= 2
			// 4 bytes if 3 <= n <= 4
			// 5 bytes if 5 <= n <= 7
			// datetime2n is a concat of timen and daten
			// datetimeoffsetn is a concat of datetime2n and
			// int16_t - timezone offset - minutes from utc
			// 				(between -840 and 840)
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				break;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			rp+=size;
			rpsize-=size;
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_CHAR:
		case TDS_TYPE_VARCHAR:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				break;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			if (bv) {
				bulkString(bv,&rpcparampool,
						(const char *)rp,size);
			}
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_LONGBINARY:
			// A sybase longbinary sizes both its maxsize and its
			// value with a single byte on tds 7, exactly like
			// binary/varbinary do, so it just falls through to
			// them here and to the maxsize switch's default case
			// above.  (Only tds 5 gives it the 4-byte length its
			// name suggests.)  isVarLenType() only accepts it
			// when we're fronting an ase, and nothing read a
			// maxsize for it otherwise, so refuse it here rather
			// than misparse the rest of the packet.
			if (!dbisase) {
				debugEnd();
				return rpcUnsupportedTypeError(tdstype);
			}
			// fall through
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
			{
			if (!rpsize) {
				debugEnd();
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			// a length of 0 is how the protocol says null, and
			// a binary/varbinary parameter stays a lob even
			// when the value turns out to be null, so it
			// doesn't lose its lob-ness before it's ever bound
			if (!size) {
				if (bv) {
					bv->type=SQLRSERVERBINDVARTYPE_NULLBLOB;
					bv->isnull=cont->getNullBindValue();
				}
				debugWrite("value: (null)");
				break;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}

			// a binary is zero padded out to its declared size,
			// like a real sql server does - a varbinary isn't.
			// the declared size is client-chosen, not a count of
			// bytes that arrived, so cap the pad at
			// TDS_MAX_BINARY_SIZE
			uint32_t	valuesize=size;
			if (tdstype==TDS_TYPE_BINARY && maxsize>size &&
					maxsize<=TDS_MAX_BINARY_SIZE) {
				valuesize=maxsize;
			}

			if (bv) {
				const byte_t	*value=rp;
				if (valuesize>size) {
					byte_t	*padded=(byte_t *)
						rpcparampool.allocate(
								valuesize);
					bytestring::copy(padded,rp,size);
					bytestring::set(padded+size,0,
							valuesize-size);
					value=padded;
				}
				bulkBinary(bv,&rpcparampool,value,valuesize);
			}
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGVARBIN:
			{
			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);
			if (size==0xFFFF) {
				// a bigbinary/bigvarbin parameter stays a
				// lob even when the value turns out to be
				// null, so it doesn't lose its lob-ness
				// before it's ever bound
				if (bv) {
					bv->type=SQLRSERVERBINDVARTYPE_NULLBLOB;
					bv->isnull=cont->getNullBindValue();
				}
				debugWrite("value: (null)");
				break;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}

			// a binary is zero padded out to its declared size,
			// like a real sql server does - a varbinary isn't.
			// the declared size is client-chosen, not a count of
			// bytes that arrived, so cap the pad at
			// TDS_MAX_BINARY_SIZE
			uint32_t	valuesize=size;
			if (tdstype==TDS_TYPE_BIGBINARY && maxsize>size &&
					maxsize<=TDS_MAX_BINARY_SIZE) {
				valuesize=maxsize;
			}

			if (bv) {
				const byte_t	*value=rp;
				if (valuesize>size) {
					byte_t	*padded=(byte_t *)
						rpcparampool.allocate(
								valuesize);
					bytestring::copy(padded,rp,size);
					bytestring::set(padded+size,0,
							valuesize-size);
					value=padded;
				}
				bulkBinary(bv,&rpcparampool,value,valuesize);
			}
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_BIGVARCHR:
			{
			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);

			// 0xFFFF means null
			if (size==0xFFFF) {
				if (bv) {
					bv->type=SQLRSERVERBINDVARTYPE_NULL;
					bv->isnull=cont->getNullBindValue();
					debugWrite("value: (null)");
				}
				break;
			}

			if (rpsize<size) {
				debugEnd();
				return false;
			}

			// a char is blank padded out to its declared size,
			// like a real sql server does - a varchar isn't.
			// the declared size is client-chosen, not a count of
			// bytes that arrived, so cap the pad at TDS_MAX_CHAR_SIZE
			uint32_t	valuesize=size;
			if (tdstype==TDS_TYPE_BIGCHAR && maxsize>size &&
					maxsize<=TDS_MAX_CHAR_SIZE) {
				valuesize=maxsize;
			}

			if (bv) {

				// copy value in, blank-fill the pad
				bv->type=SQLRSERVERBINDVARTYPE_STRING;
				bv->valuesize=valuesize;
				bv->value.stringval=(char *)
					rpcparampool.allocate(valuesize+1);
				bytestring::copy(bv->value.stringval,rp,size);
				bytestring::set(bv->value.stringval+size,
							' ',valuesize-size);
				bv->value.stringval[valuesize]='\0';
				bv->isnull=cont->getNonNullBindValue();

				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %.*s",
						bv->valuesize,
						bv->value.stringval);
			}

			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_NVARCHAR:
			{
			// the size is in bytes, but the data is ucs-2
			if (rpsize<sizeof(uint16_t)) {
				debugEnd();
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);

			// 0xFFFF means null
			if (size==0xFFFF) {
				if (bv) {
					bv->type=SQLRSERVERBINDVARTYPE_NULL;
					bv->isnull=cont->getNullBindValue();
					debugWrite("value: (null)");
				}
				break;
			}

			if (rpsize<size) {
				debugEnd();
				return false;
			}

			uint16_t	length=size/sizeof(ucs2_t);

			// an nchar is blank padded out to its declared size,
			// like a real sql server does - an nvarchar isn't.
			// the declared size is in bytes and client-chosen, so
			// cap the pad at TDS_MAX_CHAR_SIZE
			uint32_t	maxlength=maxsize/sizeof(ucs2_t);
			uint32_t	valuelength=length;
			if (tdstype==TDS_TYPE_NCHAR && maxlength>length &&
					maxsize<=TDS_MAX_CHAR_SIZE) {
				valuelength=maxlength;
			}

			if (bv) {

				// the data isn't necessarily aligned,
				// so copy it out before converting it
				const byte_t	*dummy;
				ucs2_t		*value16=new ucs2_t[length];
				read(rp,value16,length,&dummy);
				size_t		valuesize;
				char		*value=ucs2ToUtf8(value16,
							(size_t)length,
							&valuesize);
				delete[] value16;

				// the pad is ascii spaces, one byte each,
				// however wide the value itself decoded
				uint32_t	padlength=valuelength-length;

				bv->type=SQLRSERVERBINDVARTYPE_STRING;
				bv->valuesize=(uint32_t)(valuesize+padlength);
				bv->value.stringval=(char *)
					rpcparampool.allocate(bv->valuesize+1);
				bytestring::copy(bv->value.stringval,
							value,valuesize);
				bytestring::set(bv->value.stringval+valuesize,
							' ',padlength);
				bv->value.stringval[bv->valuesize]='\0';
				bv->isnull=cont->getNonNullBindValue();

				delete[] value;

				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %.*s",
						bv->valuesize,
						bv->value.stringval);
			}

			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_UDT:
			// FIXME: nothing parses a udt body yet
			debugEnd();
			return rpcUnsupportedTypeError(tdstype);
		case TDS_TYPE_XML:
		case TDS_TYPE_TEXT:
		case TDS_TYPE_NTEXT:
			{
			// freetds sends anything over 4000 characters this
			// way, including the statement and parameter
			// declaration strings the sp_ procs take
			if (rpsize<sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			uint32_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint32_t);

			// 0xFFFFFFFF means null
			if (size==0xFFFFFFFF) {
				debugWrite("value: (null)");
				break;
			}

			if (rpsize<size) {
				debugEnd();
				return false;
			}

			if (bv) {

				bv->type=SQLRSERVERBINDVARTYPE_STRING;
				bv->isnull=cont->getNonNullBindValue();

				if (tdstype==TDS_TYPE_NTEXT) {

					// the size is in bytes,
					// but the data is ucs-2
					uint32_t	length=
						size/sizeof(ucs2_t);

					// the data isn't necessarily aligned,
					// so copy it out before converting it
					const byte_t	*dummy;
					ucs2_t	*value16=new ucs2_t[length];
					read(rp,value16,length,&dummy);
					size_t	valuesize;
					char	*value=ucs2ToUtf8(value16,
							(size_t)length,
							&valuesize);
					delete[] value16;

					bv->valuesize=(uint32_t)valuesize;
					bv->value.stringval=(char *)
						rpcparampool.allocate(
								valuesize+1);
					bytestring::copy(bv->value.stringval,
								value,valuesize);
					bv->value.stringval[valuesize]='\0';

					delete[] value;

				} else {

					bv->valuesize=size;
					bv->value.stringval=(char *)
						rpcparampool.allocate(size+1);
					bytestring::copy(bv->value.stringval,
								rp,size);
					bv->value.stringval[size]='\0';
				}

				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %.*s",
						bv->valuesize,
						bv->value.stringval);
			}

			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_IMAGE:
		case TDS_TYPE_SSVARIANT:
			{
			if (rpsize<sizeof(uint32_t)) {
				debugEnd();
				return false;
			}
			uint32_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint32_t);
			if (size==0xFFFFFFFF) {
				// only image is a lob here - this branch is
				// shared with ssvariant, and a null variant
				// must not be relabeled as one
				if (bv && tdstype==TDS_TYPE_IMAGE) {
					bv->type=SQLRSERVERBINDVARTYPE_NULLBLOB;
					bv->isnull=cont->getNullBindValue();
				}
				debugWrite("value: (null)");
				break;
			}
			if (rpsize<size) {
				debugEnd();
				return false;
			}
			if (bv && tdstype==TDS_TYPE_IMAGE) {
				bulkBinary(bv,&rpcparampool,rp,size);
			}
			rp+=size;
			rpsize-=size;
			}
			break;
		case TDS_TYPE_TVP:
			// FIXME:
			// TVP_TYPENAME
			// TVP_COLMETADATA
			// [TVP_ORDER_UNIQUE]
			// [TVP_COLUMN_ORDERING]
			// TVP_END_TOKEN
			// *TVP_ROW
			// TVP_END_TOKEN
			// FIXME: nothing parses a tvp body yet
			debugEnd();
			return rpcUnsupportedTypeError(tdstype);
		default:
			debugEnd();
			return rpcUnsupportedTypeError(tdstype);
	}

	if (negotiatedtdsversion>=740) {
		// FIXME:
		// paramcipherdata =
		// 	type_info
		// 	EncryptionAlgo - byte   (tds 7.4+)
		//   	[AlgoName] - b_varchar  (tds 7.4+)
		//   	EncryptionType - byte   (tds 7.4+)
		//   	CekHash - ???           (tds 7.4+)
		//   	NormVersion - byte      (tds 7.4+)
	}

	debugEnd();
	return true;
}

size_t sqlrprotocol_tds::charSize() {
	return (pretds7)?sizeof(byte_t):sizeof(ucs2_t);
}

size_t sqlrprotocol_tds::varcharSize(size_t lensize, size_t length) {
	return lensize+length*charSize();
}

void sqlrprotocol_tds::writeVarcharLength(bytebuffer *buffer,
					size_t lensize,
					size_t length) {
	if (lensize==sizeof(byte_t)) {
		write(buffer,(byte_t)length);
	} else if (lensize==sizeof(uint16_t)) {
		write(buffer,(uint16_t)length);
	} else {
		write(buffer,(uint32_t)length);
	}
}

// These write "length" units at the session's character width and
// nothing else - in particular they don't convert to the charset a
// pre-tds7 client declared, even though that's where a pre-tds7 client's
// column names and message text go out.  The conversion is the caller's,
// because a converted string isn't necessarily the same number of bytes
// as the utf-8 it came from, and every caller here has already written a
// length - its own, and usually a token size containing it - that has to
// agree with what lands in the buffer.  See utf8ToClientCharset() and its
// callers.
void sqlrprotocol_tds::writeVarchar(bytebuffer *buffer,
					size_t lensize,
					const char *str,
					size_t length) {

	writeVarcharLength(buffer,lensize,length);

	if (!length) {
		return;
	}

	if (pretds7) {
		write(buffer,str,length);
	} else {
		ucs2_t	*str16=ucs2charstring::duplicate(str,length);
		write(buffer,str16,length);
		delete[] str16;
	}
}

void sqlrprotocol_tds::writeVarchar(bytebuffer *buffer,
					size_t lensize,
					const wchar_t *str,
					size_t length) {

	writeVarcharLength(buffer,lensize,length);

	if (!length) {
		return;
	}

	if (pretds7) {
		char	*str8=charstring::duplicate(str,length);
		write(buffer,str8,length);
		delete[] str8;
	} else {
		ucs2_t	*str16=ucs2charstring::duplicate(str,length);
		write(buffer,str16,length);
		delete[] str16;
	}
}

void sqlrprotocol_tds::envChange(byte_t type,
					const wchar_t *newvalue,
					size_t newvaluelen,
					const wchar_t *oldvalue,
					size_t oldvaluelen) {

	byte_t		token=TOKEN_ENV_CHANGE;

	uint16_t	newvaluelensize=
			(type==ENV_CHANGE_PROMOTE_TRANSACTION)?
						sizeof(uint32_t):
						sizeof(byte_t);
	uint16_t	oldvaluelensize=sizeof(byte_t);

	uint16_t	tokensize=(uint16_t)
				(sizeof(byte_t)+
				varcharSize(newvaluelensize,newvaluelen)+
				varcharSize(oldvaluelensize,oldvaluelen));

	debugStart("env change");
	debugTokenType(token);
	debugWrite("tokensize: 0x%02x (%hd)",tokensize,tokensize);
	debugEnvChangeType(type);
	debugWrite("newvaluelensize:%d",newvaluelensize);
	debugWrite("newvaluelen: %lld",(long long)newvaluelen);
	debugWrite("newvalue: %S",newvalue);
	debugWrite("oldvaluelensize:%d",oldvaluelensize);
	debugWrite("oldvaluelen: %lld",(long long)oldvaluelen);
	debugWrite("oldvalue: %S",oldvalue);
	debugEnd();

	write(&resppacket,token);
	write(&resppacket,tokensize);
	write(&resppacket,type);
	writeVarchar(&resppacket,newvaluelensize,newvalue,newvaluelen);
	writeVarchar(&resppacket,oldvaluelensize,oldvalue,oldvaluelen);
}

void sqlrprotocol_tds::appendInfo(uint32_t number,
					byte_t state,
					byte_t infoclass,
					const char *msgtext,
					const char *servername,
					const char *procname,
					uint32_t linenumber) {
	appendInfoOrError(TOKEN_INFO,number,state,infoclass,
				msgtext,servername,procname,linenumber);
}

void sqlrprotocol_tds::appendError(uint32_t number,
					byte_t state,
					byte_t errclass,
					const char *msgtext,
					const char *servername,
					const char *procname,
					uint32_t linenumber) {
	appendInfoOrError(TOKEN_ERROR,number,state,errclass,
				msgtext,servername,procname,linenumber);
}

void sqlrprotocol_tds::appendInfoOrError(byte_t token,
					uint32_t number,
					byte_t state,
					byte_t infoerrclass,
					const char *msgtext,
					const char *servername,
					const char *procname,
					uint32_t linenumber) {

	// A pre-tds7 client gets these strings in the charset its login
	// record declared.  The conversion happens here rather than inside
	// writeVarchar() because the sizes below - the token size in
	// particular - have to count the bytes that actually go on the wire,
	// and a converted string isn't necessarily as long as the utf-8 it
	// came from.  Each of these is NULL when nothing is to be converted,
	// which leaves the utf-8 passing through.
	size_t		msgtextconvlen=0;
	char		*msgtextconv=utf8ToClientCharset(msgtext,
					charstring::getLength(msgtext),
					&msgtextconvlen);
	if (msgtextconv) {
		msgtext=msgtextconv;
	}
	size_t		srvnameconvlen=0;
	char		*srvnameconv=utf8ToClientCharset(servername,
					charstring::getLength(servername),
					&srvnameconvlen);
	if (srvnameconv) {
		servername=srvnameconv;
	}
	size_t		procnameconvlen=0;
	char		*procnameconv=utf8ToClientCharset(procname,
					charstring::getLength(procname),
					&procnameconvlen);
	if (procnameconv) {
		procname=procnameconv;
	}

	// the name lengths are sent as single bytes
	size_t		srvnamelen=charstring::getLength(servername);
	if (srvnamelen>255) {
		srvnamelen=255;
	}
	size_t		procnamelen=charstring::getLength(procname);
	if (procnamelen>255) {
		procnamelen=255;
	}

	// everything other than the message text
	size_t		fixedsize=sizeof(uint32_t)+
					sizeof(byte_t)+
					sizeof(byte_t)+
					sizeof(uint16_t)+
					varcharSize(sizeof(byte_t),srvnamelen)+
					varcharSize(sizeof(byte_t),procnamelen)+
					((negotiatedtdsversion<720)?
						sizeof(uint16_t):
						sizeof(uint32_t));

	// truncate the message text so that the token size fits in 16 bits
	size_t		msgtextlen=charstring::getLength(msgtext);
	size_t		maxmsgtextlen=(65535-fixedsize)/charSize();
	if (msgtextlen>maxmsgtextlen) {
		msgtextlen=maxmsgtextlen;
	}

	uint16_t	tokensize=(uint16_t)
				(fixedsize+msgtextlen*charSize());

	debugStart((token==TOKEN_INFO)?"info":"error");
	debugTokenType(token);
	debugWrite("tokensize: 0x%02x (%hd)",tokensize,tokensize);
	debugWrite("number: %d",number);
	debugWrite("state: %d",state);
	debugWrite("class: %d",infoerrclass);
	debugWrite("msgtext: %s",msgtext);
	debugWrite("srvname: %s",servername);
	debugWrite("procname: %s",procname);
	debugWrite("linenumber: %d",linenumber);
	debugEnd();

	write(&resppacket,token);
	write(&resppacket,tokensize);
	write(&resppacket,number);
	write(&resppacket,state);
	write(&resppacket,infoerrclass);
	writeVarchar(&resppacket,sizeof(uint16_t),msgtext,msgtextlen);
	writeVarchar(&resppacket,sizeof(byte_t),servername,srvnamelen);
	writeVarchar(&resppacket,sizeof(byte_t),procname,procnamelen);
	if (negotiatedtdsversion<720) {
		write(&resppacket,(uint16_t)linenumber);
	} else {
		write(&resppacket,linenumber);
	}

	delete[] msgtextconv;
	delete[] srvnameconv;
	delete[] procnameconv;
}

bool sqlrprotocol_tds::sendError(uint32_t number,
					byte_t state,
					byte_t errclass,
					const char *msgtext,
					uint32_t linenumber) {

	debugStart("send error");
	debugWrite("number: %d",number);
	debugWrite("state: %d",state);
	debugWrite("class: %d",errclass);
	debugWrite("msgtext: %s",msgtext);
	debugWrite("linenumber: %d",linenumber);
	debugEnd();

	resppacket.clear();
	appendError(number,state,errclass,msgtext,srvname,NULL,linenumber);
	done(DONE_ERROR,0,0);
	return sendPacket();
}

bool sqlrprotocol_tds::sendUnimplementedFeatureError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,10,"Unimplemented feature",1);
}

bool sqlrprotocol_tds::sendTlsRequiredError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,16,
			(getTlsContext()->getValidatePeer())?
					"TLS mutual auth required":
					"TLS required",1);
}

bool sqlrprotocol_tds::sendTdsProtocolError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,0,10,"TDS Protocol Error",1);
}

void sqlrprotocol_tds::queryTooLargeMessage(size_t querysize,
						stringbuffer *err) {
	err->append("Query too large (");
	err->append((uint64_t)querysize);
	err->append('>');
	err->append(maxquerysize);
	err->append(')');
}

bool sqlrprotocol_tds::sendQueryTooLargeError(size_t querysize) {

	stringbuffer	err;
	queryTooLargeMessage(querysize,&err);

	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,16,err.getString(),1);
}

bool sqlrprotocol_tds::sendNoCursorAvailableError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,16,"No cursor available",1);
}

bool sqlrprotocol_tds::sendLoginRequiredError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,16,"Login required",1);
}

bool sqlrprotocol_tds::sendAlreadyLoggedInError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,16,"Already logged in",1);
}

// The value done() writes for a tds 5.0 session, and 0 for an ms-tds one.
// The field means two different things in the two dialects - see done().
// A real ASE sends CS_TRAN_COMPLETED on every done that isn't inside a
// transaction, so match that rather than leaving it CS_TRAN_UNDEFINED.
uint16_t sqlrprotocol_tds::transState() {
	if (!pretds7) {
		// CurCmd.  Ms-tds clients ignore it and freetds expects
		// the 0 that this module has always sent.
		return 0;
	}
	return (cont->getInTransaction())?
			TDS5_TRAN_IN_PROGRESS:TDS5_TRAN_COMPLETED;
}

void sqlrprotocol_tds::done() {
	done(DONE_FINAL,transState(),0);
}

void sqlrprotocol_tds::done(uint16_t status,
				uint16_t curcmdortransstate,
				uint64_t donerowcount) {
	done(TOKEN_DONE,status,curcmdortransstate,donerowcount);
}

// "curcmdortransstate" is the second uint16 of the token, and it carries
// a different thing in each dialect: CurCmd in ms-tds, TransState (one of
// the TDS5_TRAN_* values) in tds 5.0.  transState() picks the right one.
void sqlrprotocol_tds::done(byte_t token,
				uint16_t status,
				uint16_t curcmdortransstate,
				uint64_t donerowcount) {

	switch (token) {
		case TOKEN_DONEINPROC:
			debugStart("done-in-proc");
			break;
		case TOKEN_DONEPROC:
			debugStart("done-proc");
			break;
		default:
			debugStart("done");
			break;
	}
	debugTokenType(token);
	debugDoneStatus(status);
	debugWrite("%s: 0x%02x",(pretds7)?"transstate":"curcmd",
						curcmdortransstate);
	debugWrite("donerowcount: %lld",(long long)donerowcount);
	debugEnd();

	write(&resppacket,token);
	write(&resppacket,status);
	write(&resppacket,curcmdortransstate);
	if (negotiatedtdsversion<720) {
		write(&resppacket,(uint32_t)donerowcount);
	} else {
		write(&resppacket,donerowcount);
	}
}

// "curcmd" is only ever a CurCmd - every caller passes 0, and the field
// is a TransState rather than a CurCmd in tds 5.0, which is why it goes
// through transState() there.  Self-guarding the same way transState()
// itself is: for an ms-tds session it hands back exactly what the caller
// asked for, so the ms-tds wire can't move.  A done-in-proc used to be
// an ms-tds-only token, but a tds 5.0 dbrpc closes its result set with
// one too.
void sqlrprotocol_tds::doneInProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount) {

	// a done-in-proc is never the last done in a response, and sql server
	// always sets DONE_MORE on one.  without that bit freetds takes it for
	// the last one and stops reading, leaving every command after it one
	// response behind.
	done(TOKEN_DONEINPROC,status|DONE_MORE,
			(pretds7)?transState():curcmd,donerowcount);
}

void sqlrprotocol_tds::returnStatus(uint32_t value) {

	// shared with the pre-tds7 path (preTds7DbRpc()->runProc()->
	// namedProc() and rpcError() both reach this), so this has to
	// honor the client's declared byte order rather than hardcode LE
	byte_t		token=TOKEN_RETURNSTATUS;

	write(&resppacket,token);
	write(&resppacket,value);

	debugStart("return-status");
	debugTokenType(token);
	debugWrite("status: %d",value);
	debugEnd();
}

uint32_t sqlrprotocol_tds::procReturnValue(sqlrservercursor *cursor) {

	// the return value rides in the output bind that bindParams()
	// reserved for it
	if (!cont->getOutputBindCount(cursor) ||
			outbindparams[0]!=RPC_RETURN_VALUE_PARAM) {
		return RPC_STATUS_SUCCESS;
	}

	sqlrserverbindvar	*rv=&(cont->getOutputBinds(cursor)[0]);
	if (rv->type!=SQLRSERVERBINDVARTYPE_INTEGER) {
		return RPC_STATUS_SUCCESS;
	}
	return (uint32_t)rv->value.integerval;
}

void sqlrprotocol_tds::returnValues(sqlrservercursor *cursor) {

	debugStart("return-values");

	// for each output bind...
	uint16_t	outbindcount=cont->getOutputBindCount(cursor);
	debugWrite("outbindcount: %d",outbindcount);
	uint16_t	ordinal=1;
	for (uint16_t i=0; i<outbindcount; i++) {

		// the return value went out in the RETURNSTATUS token, so it
		// isn't one of these, and it doesn't take up an ordinal
		if (outbindparams[i]==RPC_RETURN_VALUE_PARAM) {
			debugWrite("param %d: (return value, skipped)",i);
			continue;
		}

		returnValue(cursor,i,ordinal);
		ordinal++;
	}

	debugEnd();
}

void sqlrprotocol_tds::returnValueHeader(uint16_t ordinal,
						const char *name,
						uint16_t namesize,
						uint32_t usertype) {

	debugStart("return-value-header");

	byte_t	token=TOKEN_RETURNVALUE;
	write(&resppacket,token);
	debugTokenType(token);

	// param ordinal
	writeLE(&resppacket,ordinal);

	// param name - a client that looks at it, rather than matching by
	// ordinal, gets an empty name from ct_describe otherwise.  The
	// length is a single byte, so a longer name is truncated, the
	// same way colName() truncates an over-length column name.
	if (name && namesize) {
		if (namesize>255) {
			namesize=255;
		}
		ucs2_t	*name16=ucs2charstring::duplicate(name,
							(size_t)namesize);
		write(&resppacket,(byte_t)namesize);
		write(&resppacket,name16,namesize);
		delete[] name16;
		debugWrite("name: %s",name);
	} else {
		write(&resppacket,(byte_t)0);
		debugWrite("name: (none)");
	}

	// status - 0x01 means it's an output parameter
	write(&resppacket,(byte_t)0x01);

	// user type
	if (negotiatedtdsversion<720) {
		writeLE(&resppacket,(uint16_t)usertype);
	} else {
		writeLE(&resppacket,usertype);
	}

	// flags - a real sql server sends none set, and the ct-lib client
	// reads a return value's user type as 4 bytes whatever the tds
	// version, so it swallows these two as well and ct_describe reports
	// 65536 rather than 0 for a flags word of 0x0001
	writeLE(&resppacket,(uint16_t)0x0000);

	debugWrite("ordinal: %d",ordinal);
	debugWrite("usertype: %d",usertype);

	debugEnd();
}

// One unnamed integer output parameter, which is how the numbered procs
// hand a handle back.  A tds 5.0 session gets the paramfmt/params pair
// instead of the ms-tds returnvalue token, the same swap
// procReturnValues() makes.
//
// A pair carries the whole set rather than one parameter, so this only
// works where the proc sends exactly one - preTds7DbRpc() refuses the
// procs that send several.
void sqlrprotocol_tds::returnValueInteger(uint16_t ordinal,
						int32_t value,
						bool isnull) {

	if (pretds7) {
		preTds7ReturnValueInteger(value,isnull);
		return;
	}

	debugStart("return-value");

	returnValueHeader(ordinal,NULL,0,0);

	// type info - typeInfo() describes a column of a result set, so it
	// can't be reused for a scalar like this
	write(&resppacket,(byte_t)TDS_TYPE_INTN);
	write(&resppacket,(byte_t)sizeof(int32_t));

	// value - a length of 0 means null
	if (isnull) {
		write(&resppacket,(byte_t)0);
		debugWrite("value: (null)");
	} else {
		write(&resppacket,(byte_t)sizeof(int32_t));
		writeLE(&resppacket,(uint32_t)value);
		debugWrite("value: %d",value);
	}

	debugEnd();
}

// the tds 5.0 counterpart of returnValueInteger().  No name - the ms-tds
// token doesn't carry one here either, and a parameter is identified by
// position in this dialect anyway.
void sqlrprotocol_tds::preTds7ReturnValueInteger(int32_t value,
						bool isnull) {

	debugStart("pre-tds7 return-value integer");

	tds5paramfmt	*fmt=&(pretds7outfmts[0]);
	fmt->name="";
	fmt->namesize=0;
	fmt->status=TDS5_PARAM_RETURN;
	fmt->usertype=0;
	// intn rather than a fixed int4, so that a null can be expressed -
	// a fixed type has no length field to set to zero
	fmt->tds5type=TDS5_TYPE_INTN;
	fmt->mstype=TDS_TYPE_INTN;
	fmt->varintsize=preTds7VarintSize(fmt->tds5type);
	fmt->size=sizeof(int32_t);
	fmt->precision=0;
	fmt->scale=0;

	sqlrserverbindvar	*bv=&(pretds7outbinds[0]);
	bv->variable=NULL;
	bv->variablesize=0;
	bv->valuesize=0;
	bv->isnull=cont->getNullBindValue();
	if (isnull) {
		bv->type=SQLRSERVERBINDVARTYPE_NULL;
		bv->value.stringval=NULL;
	} else {
		bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
		bv->value.integerval=value;
	}

	if (preTds7ParamFmtWrite(pretds7outfmts,1)) {
		preTds7ParamsWrite(pretds7outfmts,pretds7outbinds,1);
	}

	debugEnd();
}

void sqlrprotocol_tds::writeIntN(int64_t value, byte_t size) {
	switch (size) {
		case 1:
			write(&resppacket,(byte_t)value);
			break;
		case 2:
			write(&resppacket,(uint16_t)value);
			break;
		case 4:
			write(&resppacket,(uint32_t)value);
			break;
		default:
			write(&resppacket,(uint64_t)value);
			break;
	}
}

// An ieee float raises the same byte-order question an integer of the
// same width does, and a pre-tds7 client answers both in the same place -
// the login record's typeflags block.  But the base class's
// write(bytebuffer *,float) and write(bytebuffer *,double) append the
// host's raw bytes without consulting the order the login settled on, and
// there is no writeLE/writeBE for either, so this hands the bits to the
// integer path instead, which does consult it.
//
// On a little-endian host serving a little-endian client - every client
// that turns up in practice - this writes the same bytes write(float)
// would have.
void sqlrprotocol_tds::writeFloatN(double value, byte_t size) {
	if (size==sizeof(float)) {
		float		f=(float)value;
		uint32_t	bits=0;
		bytestring::copy(&bits,&f,sizeof(bits));
		write(&resppacket,bits);
	} else {
		uint64_t	bits=0;
		bytestring::copy(&bits,&value,sizeof(bits));
		write(&resppacket,bits);
	}
}

void sqlrprotocol_tds::returnValueChar(sqlrserverbindvar *bv,
					byte_t tdstype,
					uint32_t maxsize) {

	debugStart("return-value-char");
	debugColumnType(tdstype);

	// unicode types go back as ucs-2, everything else as the "big"
	// (2-byte-length) form of what the client declared - the legacy
	// 1-byte forms and text/ntext fall back to the closest "big" type
	bool	unicode=(tdstype==TDS_TYPE_NCHAR ||
				tdstype==TDS_TYPE_NVARCHAR ||
				tdstype==TDS_TYPE_NTEXT);
	bool	blankpad=(tdstype==TDS_TYPE_CHAR ||
				tdstype==TDS_TYPE_BIGCHAR ||
				tdstype==TDS_TYPE_NCHAR);
	byte_t	outtype=(unicode)?
			((blankpad)?TDS_TYPE_NCHAR:TDS_TYPE_NVARCHAR):
			((blankpad)?TDS_TYPE_BIGCHAR:TDS_TYPE_BIGVARCHR);
	debugWrite("unicode: %d",unicode);
	debugWrite("blankpad: %d",blankpad);

	// the client will read the declared size as signed, and a text or
	// varchar(max) parameter declares more than that fits
	if (maxsize>32767) {
		maxsize=32767;
	}
	debugWrite("maxsize: %d",maxsize);

	bool	isnull=(bv->type!=SQLRSERVERBINDVARTYPE_STRING ||
				!bv->value.stringval ||
				cont->getBindValueIsNull(bv->isnull));
	debugWrite("isnull: %d",isnull);

	// the value is whatever the backend put in the buffer, decoded into
	// what the client's declared type takes and blank padded if that
	// type is fixed-width
	const char	*value=(isnull)?NULL:bv->value.stringval;
	size_t		valuelength=(isnull)?0:charstring::getLength(value);
	char		*value8=NULL;
	ucs2_t		*value16=NULL;
	size_t		converted=0;
	if (!isnull) {
		if (unicode) {
			value16=utf8ToUcs2(value,valuelength,&converted);
		} else {
			value8=utf8ToCp1252(value,valuelength,&converted);
		}
	}
	uint16_t	valuesize=(uint16_t)converted;
	if (valuesize>maxsize) {
		valuesize=(uint16_t)maxsize;
	}
	uint16_t	padsize=(!isnull && blankpad && maxsize>valuesize)?
					(uint16_t)(maxsize-valuesize):0;

	// type info
	write(&resppacket,outtype);
	writeLE(&resppacket,(uint16_t)((unicode)?maxsize*2:maxsize));
	if (negotiatedtdsversion>=710) {
		writeCollation();
	}

	// value - 0xFFFF means null
	if (isnull) {
		writeLE(&resppacket,(uint16_t)0xFFFF);
		debugWrite("value: (null)");
		debugEnd();
		return;
	}

	if (unicode) {
		writeLE(&resppacket,(uint16_t)((valuesize+padsize)*2));
		write(&resppacket,value16,valuesize);
		delete[] value16;
		for (uint16_t i=0; i<padsize; i++) {
			writeLE(&resppacket,(uint16_t)' ');
		}
	} else {
		writeLE(&resppacket,(uint16_t)(valuesize+padsize));
		write(&resppacket,(const byte_t *)value8,valuesize);
		delete[] value8;
		for (uint16_t i=0; i<padsize; i++) {
			write(&resppacket,(byte_t)' ');
		}
	}

	debugWrite("value: %.*s",(int32_t)valuelength,value);
	debugEnd();
}

void sqlrprotocol_tds::returnValueDateTime(sqlrserverbindvar *bv,
						byte_t tdstype,
						uint32_t maxsize) {

	debugStart("return-value-date-time");
	debugColumnType(tdstype);
	debugWrite("maxsize: %d",maxsize);

	// a client that declared smalldatetime gets 4 bytes back, one that
	// declared datetime gets 8 - the same widths datetimn allows.  the
	// fixed length types carry no maxsize, so they go by type alone.
	byte_t	size=(tdstype==TDS_TYPE_DATETIM4 || maxsize==4)?4:8;
	debugWrite("size: %d",size);

	// type info
	write(&resppacket,(byte_t)TDS_TYPE_DATETIMN);
	write(&resppacket,size);

	// value - a length of 0 means null
	if (bv->type!=SQLRSERVERBINDVARTYPE_DATE ||
			cont->getBindValueIsNull(bv->isnull)) {
		write(&resppacket,(byte_t)0);
		debugWrite("value: (null)");
		debugEnd();
		return;
	}

	// dateTime() works from a string, so render one for it to take apart
	char	datetime[40];
	charstring::printf(datetime,sizeof(datetime),
				"%04d-%02d-%02d %02d:%02d:%02d.%06d",
				(int32_t)bv->value.dateval.year,
				(int32_t)bv->value.dateval.month,
				(int32_t)bv->value.dateval.day,
				(int32_t)bv->value.dateval.hour,
				(int32_t)bv->value.dateval.minute,
				(int32_t)bv->value.dateval.second,
				(int32_t)bv->value.dateval.microsecond);

	int32_t		dayssince1900;
	uint32_t	threehundredths;
	dateTime(datetime,&dayssince1900,&threehundredths);

	write(&resppacket,size);
	if (size==4) {
		writeLE(&resppacket,(uint16_t)((dayssince1900>0)?
						dayssince1900:0));
		writeLE(&resppacket,(uint16_t)(threehundredths/300/60));
	} else {
		writeLE(&resppacket,(uint32_t)dayssince1900);
		writeLE(&resppacket,threehundredths);
	}

	debugWrite("value: %s",datetime);
	debugEnd();
}

void sqlrprotocol_tds::returnValueDecimal(sqlrserverbindvar *bv,
						byte_t tdstype,
						byte_t precision,
						byte_t scale) {

	debugStart("return-value-decimal");
	debugColumnType(tdstype);

	// The nullable forms are the only ones a real server sends back,
	// and the legacy fixed-size forms have no length byte to say null
	// with, so a legacy declaration goes back as its nullable twin.
	byte_t	outtype=(tdstype==TDS_TYPE_DECIMAL ||
				tdstype==TDS_TYPE_DECIMALN)?
				TDS_TYPE_DECIMALN:TDS_TYPE_NUMERICN;

	// a client that declared no precision still gets a usable one
	if (!precision) {
		precision=18;
	} else if (precision>TDS_DECIMAL_MAX_PRECISION) {
		precision=TDS_DECIMAL_MAX_PRECISION;
	}
	if (scale>precision) {
		scale=precision;
	}

	// type info - the size is the widest the value can be on the wire,
	// not the precision, the same way typeInfo() sends it for a column
	write(&resppacket,outtype);
	write(&resppacket,(byte_t)TDS_DECIMAL_MAX_SIZE);
	write(&resppacket,precision);
	write(&resppacket,scale);
	debugWrite("precision: %d",precision);
	debugWrite("scale: %d",scale);

	// the value, rendered at the declared scale.  the backend hands
	// these back as doubles (see sapcursor::outputBind()), but take a
	// string or an integer too, in case another backend doesn't.
	char	field[64];
	field[0]='\0';
	bool	isnull=cont->getBindValueIsNull(bv->isnull);
	if (!isnull) {
		switch (bv->type) {
			case SQLRSERVERBINDVARTYPE_DOUBLE:
				charstring::printf(field,sizeof(field),
					"%.*f",(int32_t)scale,
					bv->value.doubleval.value);
				break;
			case SQLRSERVERBINDVARTYPE_INTEGER:
				charstring::printf(field,sizeof(field),
					"%.*f",(int32_t)scale,
					(double)bv->value.integerval);
				break;
			case SQLRSERVERBINDVARTYPE_STRING:
				charstring::printf(field,sizeof(field),
					"%.*f",(int32_t)scale,
					charstring::convertToFloatC(
						(bv->value.stringval)?
						bv->value.stringval:"0"));
				break;
			default:
				isnull=true;
				break;
		}
	}

	// a length of 0 is how the protocol says null
	if (isnull) {
		write(&resppacket,(byte_t)0);
		debugWrite("value: (null)");
		debugEnd();
		return;
	}

	byte_t	ispositive;
	byte_t	size;
	byte_t	val[TDS_DECIMAL_MAX_SIZE-1];
	bytestring::zero(val,sizeof(val));
	decimal(field,&ispositive,&size,val);

	// The width on the wire comes from the declared precision rather
	// than from the value.  The client reads the sign byte and then as
	// many magnitude bytes as that precision calls for, whatever length
	// it was sent, so anything else decodes to garbage.  The magnitude
	// is little-endian, so writing fewer bytes than decimal() produced
	// only drops leading zeros, and writing more only adds them.
	byte_t	wiresize=decimalSize(precision);
	write(&resppacket,wiresize);
	write(&resppacket,ispositive);
	write(&resppacket,val,wiresize-1);

	debugWrite("value: %s",field);
	debugEnd();
}

void sqlrprotocol_tds::returnValue(sqlrservercursor *cursor,
						uint16_t param,
						uint16_t ordinal) {

	debugStart("return-value");

	sqlrserverbindvar	*bv=&(cont->getOutputBinds(cursor)[param]);

	// the rpc parameter this output bind came from carries the name and
	// the type the client declared
	uint16_t	rpcparam=outbindparams[param];

	// a character, datetime or decimal output parameter goes back as the
	// type the client declared, the way a real sql server echoes one -
	// sql relay's own bind type can't tell a char(20) from a
	// varchar(max), or a numeric(10,4) from a float, and ct_describe()
	// reports whatever type arrives
	byte_t		declaredtype=rpcparamtdstypes[rpcparam];
	uint32_t	declaredmaxsize=rpcparammaxsizes[rpcparam];
	bool		isdecimal=(declaredtype==TDS_TYPE_NUMERIC ||
					declaredtype==TDS_TYPE_NUMERICN ||
					declaredtype==TDS_TYPE_DECIMAL ||
					declaredtype==TDS_TYPE_DECIMALN);

	// A real ase echoes a decimal output parameter's systypes usertype
	// (28 for numeric, 27 for decimal) and the client asserts it.  It
	// sends 0 for every other type, as mssql does for all of them, so
	// nothing else needs one.
	uint32_t	usertype=0;
	if (isdecimal && dbisase) {
		usertype=(declaredtype==TDS_TYPE_DECIMAL ||
				declaredtype==TDS_TYPE_DECIMALN)?27:28;
	}

	returnValueHeader(ordinal,rpcparamnames[rpcparam],
					rpcparamnamesizes[rpcparam],usertype);

	if (isdecimal) {
		returnValueDecimal(bv,declaredtype,
					rpcparamprecisions[rpcparam],
					rpcparamscales[rpcparam]);
		debugEnd();
		return;
	}
	if (isCharType(declaredtype)) {
		returnValueChar(bv,declaredtype,declaredmaxsize);
		debugEnd();
		return;
	}
	if (declaredtype==TDS_TYPE_DATETIME ||
			declaredtype==TDS_TYPE_DATETIM4 ||
			declaredtype==TDS_TYPE_DATETIMN) {
		returnValueDateTime(bv,declaredtype,declaredmaxsize);
		debugEnd();
		return;
	}

	// otherwise, whatever the database put in the output bind
	switch (bv->type) {
		case SQLRSERVERBINDVARTYPE_INTEGER:
			{
			// an integer goes back at the width the client
			// declared - sending an 8 byte INTN instead makes
			// ct_describe report CS_LONG_TYPE for what the
			// client sent as a CS_INT_TYPE
			byte_t	size=(byte_t)rpcparammaxsizes[rpcparam];
			if (size!=1 && size!=2 && size!=4 && size!=8) {
				size=sizeof(int64_t);
			}
			write(&resppacket,(byte_t)TDS_TYPE_INTN);
			write(&resppacket,size);
			write(&resppacket,size);
			writeIntN(bv->value.integerval,size);
			debugWrite("value: %lld",(long long)bv->value.integerval);
			}
			break;
		case SQLRSERVERBINDVARTYPE_DOUBLE:
			write(&resppacket,(byte_t)TDS_TYPE_FLTN);
			write(&resppacket,(byte_t)sizeof(double));
			write(&resppacket,(byte_t)sizeof(double));
			write(&resppacket,bv->value.doubleval.value);
			debugWrite("value: %f",bv->value.doubleval.value);
			break;
		case SQLRSERVERBINDVARTYPE_STRING:
			{
			// limit the size to 2^15-1 because the client will
			// interpret it as signed
			uint16_t	size=(bv->valuesize>32767)?
						32767:(uint16_t)bv->valuesize;
			write(&resppacket,(byte_t)TDS_TYPE_BIGVARCHR);
			writeLE(&resppacket,(uint16_t)32767);
			if (negotiatedtdsversion>=710) {
				byte_t	coll[5]={0,0,0,0,0};
				write(&resppacket,coll,sizeof(coll));
			}
			writeLE(&resppacket,size);
			write(&resppacket,(const byte_t *)bv->value.stringval,
									size);
			debugWrite("value: %.*s",size,bv->value.stringval);
			}
			break;
		default:
			write(&resppacket,(byte_t)TDS_TYPE_INTN);
			write(&resppacket,(byte_t)sizeof(int32_t));
			write(&resppacket,(byte_t)0);
			debugWrite("value: (null)");
			break;
	}

	debugEnd();
}

// self-guarded the same way doneInProc() is, and for the same reason
void sqlrprotocol_tds::doneProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount) {
	done(TOKEN_DONEPROC,status,
			(pretds7)?transState():curcmd,donerowcount);
}

void sqlrprotocol_tds::debugSystemError() {
	char	*err=error::getErrorString();
	debugWrite("%s",err);
	delete[] err;
}

void sqlrprotocol_tds::debugPacketType(const char *name, byte_t type) {
	if (!getDebug()) {
		return;
	}
	const char	*typestring=NULL;
	switch (type) {
		case SQL_BATCH:
			typestring="SQL_BATCH";
			break;
		case PRE_TDS7_LOGIN:
			typestring="PRE_TDS7_LOGIN";
			break;
		case RPC:
			typestring="RPC";
			break;
		case TABULAR_RESULT:
			typestring="TABULAR_RESULT";
			break;
		case ATTENTION_SIGNAL:
			typestring="ATTENTION_SIGNAL";
			break;
		case BULK_LOAD_DATA:
			typestring="BULK_LOAD_DATA";
			break;
		case FEDERATED_AUTHENTICATION_TOKEN:
			typestring="FEDERATED_AUTHENTICATION_TOKEN";
			break;
		case TRANSACTION_MANAGER_REQUEST:
			typestring="TRANSACTION_MANAGER_REQUEST";
			break;
		case PRE_TDS7_NORMAL:
			typestring="PRE_TDS7_NORMAL";
			break;
		case TDS7_LOGIN:
			typestring="TDS7_LOGIN";
			break;
		case SSPI:
			typestring="SSPI";
			break;
		case PRE_LOGIN:
			typestring="PRE_LOGIN";
			break;
		default:
			typestring="unknown packet type";
			break;
	}
	debugWrite("%s: 0x%02x (%s)",
			(name)?name:"packet type",
			(uint32_t)(0x000000ff&type),
			typestring);
}

void sqlrprotocol_tds::debugPacketStatus(byte_t status) {
	if (!getDebug()) {
		return;
	}
	debugWrite("packet status: 0x%02x",(uint32_t)(0x000000ff&status));
	stringbuffer	b;
	b.printBits(status);
	debugWrite("%s",b.getString());
	if (status==STATUS_NORMAL) {
		debugWrite("STATUS_NORMAL");
	}
	if (status&STATUS_EOM) {
		debugWrite("STATUS_EOM");
	}
	if (status&STATUS_IGNORE) {
		debugWrite("STATUS_IGNORE");
	}
	if (status&STATUS_RESETCONNECTION) {
		debugWrite("STATUS_RESETCONNECTION");
	}
	if (status&STATUS_RESETCONNECTIONSKIPTRAN) {
		debugWrite("STATUS_RESETCONNECTIONSKIPTRAN");
	}
}

void sqlrprotocol_tds::debugTokenType(byte_t token) {
	if (!getDebug()) {
		return;
	}
	const char	*tokenstring=NULL;
	switch (token) {
		case TOKEN_LOGIN_ACK:
			tokenstring="TOKEN_LOGIN_ACK";
			break;
		case TOKEN_COLMETADATA:
			tokenstring="TOKEN_COLMETADATA";
			break;
		case TOKEN_ROW:
			tokenstring="TOKEN_ROW";
			break;
		case TOKEN_ENV_CHANGE:
			tokenstring="TOKEN_ENV_CHANGE";
			break;
		case TOKEN_CAPABILITY:
			tokenstring="TOKEN_CAPABILITY";
			break;
		case TOKEN_INFO:
			tokenstring="TOKEN_INFO";
			break;
		case TOKEN_ERROR:
			tokenstring="TOKEN_ERROR";
			break;
		case TOKEN_DONE:
			tokenstring="TOKEN_DONE";
			break;
		case TOKEN_DONEPROC:
			tokenstring="TOKEN_DONEPROC";
			break;
		case TOKEN_DONEINPROC:
			tokenstring="TOKEN_DONEINPROC";
			break;
		case TOKEN_RETURNSTATUS:
			tokenstring="TOKEN_RETURNSTATUS";
			break;
		case TOKEN_RETURNVALUE:
			tokenstring="TOKEN_RETURNVALUE";
			break;
		case TOKEN_ROWFMT:
			tokenstring="TOKEN_ROWFMT";
			break;
		default:
			tokenstring="unknown token";
			break;
	}
	debugWrite("token: 0x%02x (%s)",
			(uint32_t)(0x000000ff&token),tokenstring);
}

// Separate from debugTokenType() on purpose - these are request-direction
// tokens, and several of the values mean something else in a response.
void sqlrprotocol_tds::debugPreTds7TokenType(byte_t token) {
	if (!getDebug()) {
		return;
	}
	const char	*tokenstring=NULL;
	switch (token) {
		case TDS5_TOKEN_CURDECLARE3:
			tokenstring="TDS5_TOKEN_CURDECLARE3";
			break;
		case TDS5_TOKEN_PARAMFMT2:
			tokenstring="TDS5_TOKEN_PARAMFMT2";
			break;
		case TDS5_TOKEN_LANGUAGE:
			tokenstring="TDS5_TOKEN_LANGUAGE";
			break;
		case TDS5_TOKEN_ORDERBY2:
			tokenstring="TDS5_TOKEN_ORDERBY2";
			break;
		case TDS5_TOKEN_CURDECLARE2:
			tokenstring="TDS5_TOKEN_CURDECLARE2";
			break;
		case TDS5_TOKEN_ROWFMT2:
			tokenstring="TDS5_TOKEN_ROWFMT2";
			break;
		case TDS5_TOKEN_DYNAMIC2:
			tokenstring="TDS5_TOKEN_DYNAMIC2";
			break;
		case TDS5_TOKEN_OPTIONCMD2:
			tokenstring="TDS5_TOKEN_OPTIONCMD2";
			break;
		case TDS5_TOKEN_MSG:
			tokenstring="TDS5_TOKEN_MSG";
			break;
		case TDS5_TOKEN_LOGOUT:
			tokenstring="TDS5_TOKEN_LOGOUT";
			break;
		case TDS5_TOKEN_CURCLOSE:
			tokenstring="TDS5_TOKEN_CURCLOSE";
			break;
		case TDS5_TOKEN_CURDELETE:
			tokenstring="TDS5_TOKEN_CURDELETE";
			break;
		case TDS5_TOKEN_CURFETCH:
			tokenstring="TDS5_TOKEN_CURFETCH";
			break;
		case TDS5_TOKEN_CURINFO:
			tokenstring="TDS5_TOKEN_CURINFO";
			break;
		case TDS5_TOKEN_CUROPEN:
			tokenstring="TDS5_TOKEN_CUROPEN";
			break;
		case TDS5_TOKEN_CURUPDATE:
			tokenstring="TDS5_TOKEN_CURUPDATE";
			break;
		case TDS5_TOKEN_CURDECLARE:
			tokenstring="TDS5_TOKEN_CURDECLARE";
			break;
		case TDS5_TOKEN_CURINFO2:
			tokenstring="TDS5_TOKEN_CURINFO2";
			break;
		case TDS5_TOKEN_CURINFO3:
			tokenstring="TDS5_TOKEN_CURINFO3";
			break;
		case TDS5_TOKEN_OPTIONCMD:
			tokenstring="TDS5_TOKEN_OPTIONCMD";
			break;
		case TDS5_TOKEN_KEY:
			tokenstring="TDS5_TOKEN_KEY";
			break;
		case TDS5_TOKEN_ROW:
			tokenstring="TDS5_TOKEN_ROW";
			break;
		case TDS5_TOKEN_PARAMS:
			tokenstring="TDS5_TOKEN_PARAMS";
			break;
		case TDS5_TOKEN_CAPABILITY:
			tokenstring="TDS5_TOKEN_CAPABILITY";
			break;
		case TDS5_TOKEN_DBRPC:
			tokenstring="TDS5_TOKEN_DBRPC";
			break;
		case TDS5_TOKEN_DYNAMIC:
			tokenstring="TDS5_TOKEN_DYNAMIC";
			break;
		case TDS5_TOKEN_DBRPC2:
			tokenstring="TDS5_TOKEN_DBRPC2";
			break;
		case TDS5_TOKEN_PARAMFMT:
			tokenstring="TDS5_TOKEN_PARAMFMT";
			break;
		default:
			tokenstring="unknown token";
			break;
	}
	debugWrite("token: 0x%02x (%s)",
			(uint32_t)(0x000000ff&token),tokenstring);

	byte_t		lensize=preTds7TokenLength(token);
	const char	*lensizestring=NULL;
	switch (lensize) {
		case TDS5_LENSIZE_FIXED1:
			lensizestring="fixed, no length";
			break;
		case TDS5_LENSIZE_UNKNOWN:
			lensizestring="not skippable";
			break;
		default:
			lensizestring="byte length field";
			break;
	}
	debugWrite("length size: %d (%s)",(int)lensize,lensizestring);
}

void sqlrprotocol_tds::debugPreLoginOption(byte_t opt) {
	if (!getDebug()) {
		return;
	}
	const char	*optstring=NULL;
	switch (opt) {
		case PL_VERSION:
			optstring="PL_VERSION";
			break;
		case PL_ENCRYPTION:
			optstring="PL_ENCRYPTION";
			break;
		case PL_INSTOPT:
			optstring="PL_INSTOPT";
			break;
		case PL_THREADID:
			optstring="PL_THREADID";
			break;
		case PL_MARS:
			optstring="PL_MARS";
			break;
		case PL_TRACEID:
			optstring="PL_TRACEID";
			break;
		case PL_FEDAUTHREQUIRED:
			optstring="PL_FEDAUTHREQUIRED";
			break;
		case PL_NONCEOPT:
			optstring="PL_NONCEOPT";
			break;
		case PL_TERMINATOR:
			optstring="PL_TERMINATOR";
			break;
		default:
			optstring="unknown pre-login option";
			break;
	}
	debugWrite("token: 0x%02x (%s)",
			(uint32_t)(0x000000ff&opt),optstring);
}

void sqlrprotocol_tds::debugEncryptionOption(const char *name, byte_t enc) {
	if (!getDebug()) {
		return;
	}
	const char	*encstring=NULL;
	switch (enc) {
		case ENCRYPT_OFF:
			encstring="ENCRYPT_OFF";
			break;
		case ENCRYPT_ON:
			encstring="ENCRYPT_ON";
			break;
		case ENCRYPT_NOT_SUP:
			encstring="ENCRYPT_NOT_SUP";
			break;
		case ENCRYPT_REQ:
			encstring="ENCRYPT_REQ";
			break;
		default:
			encstring="unknown encryption option";
			break;
	}
	debugWrite("%s: 0x%02x (%s)",
			(name)?name:"encryption",
			(uint32_t)(0x000000ff&enc),encstring);
}

void sqlrprotocol_tds::debugLogin7OptionFlags(byte_t optionflags1,
						byte_t optionflags2,
						byte_t typeflags,
						byte_t optionflags3) {
	if (!getDebug()) {
		return;
	}

	stringbuffer	b;

	// option flags 1
	byte_t	fbyteorder=(optionflags1&(0x01));
	byte_t	fcharset=(optionflags1&(0x01<<1))>>1;
	byte_t	ffloattype=(optionflags1&(0x03<<2))>>2;
	byte_t	fdumpload=(optionflags1&(0x01<<4))>>4;
	byte_t	fusedbwarn=(optionflags1&(0x01<<5))>>5;
	byte_t	fusedbfatal=(optionflags1&(0x01<<6))>>6;
	byte_t	fsetlangwarn=(optionflags1&(0x01<<7))>>7;

	b.printBits(optionflags1);
	debugWrite("optionflags1: %s",b.getString());
	debugWrite("fbyteorder: %d (%s)",fbyteorder,
			(fbyteorder==ORDER_X86)?"ORDER_X86":
			(fbyteorder==ORDER_68000)?"ORDER_68000":
			"unknown byte order");
	debugWrite("fcharset: %d (%s)",fcharset,
			(fcharset==CHARSET_ASCII)?"CHARSET_ASCII":
			(fcharset==CHARSET_EBDDIC)?"CHARSET_EBDDIC":
			"unknown character set");
	debugWrite("ffloattype: %d (%s)",ffloattype,
			(ffloattype==FLOAT_IEEE_754)?"FLOAT_IEEE_754":
			(ffloattype==FLOAT_VAX)?"FLOAT_VAX":
			(ffloattype==FLOAT_ND5000)?"FLOAT_ND5000":
			"unknown floating point type");
	debugWrite("fdumpload: %d (%s)",fdumpload,
			(fdumpload==DUMPLOAD_ON)?"DUMPLOAD_ON":
			(fdumpload==DUMPLOAD_OFF)?"DUMPLOAD_OFF":
			"unknown dump/load");
	debugWrite("fusedbwarn: %d (%s)",fusedbwarn,
			(fusedbwarn==USE_DB_WARN_OFF)?"USE_DB_WARN_OFF":
			(fusedbwarn==USE_DB_WARN_ON)?"USE_DB_WARN_ON":
			"unknown use db warning");
	debugWrite("fusedbfatal: %d (%s)",fusedbfatal,
			(fusedbfatal==USE_DB_WARN)?"USE_DB_WARN":
			(fusedbfatal==USE_DB_FATAL)?"USE_DB_FATAL":
			"unknown use db flag");
	debugWrite("fsetlangwarn: %d (%s)",fsetlangwarn,
			(fsetlangwarn==SET_LANG_WARN_OFF)?"SET_LANG_WARN_OFF":
			(fsetlangwarn==SET_LANG_WARN_ON)?"SET_LANG_WARN_ON":
			"unknown set language warning");

	// option flags 2
	byte_t	fsetlangfatal=(optionflags2&(0x01));
	byte_t	fodbc=(optionflags2&(0x01<<1))>>1;
	byte_t	ftranboundary=(optionflags2&(0x01<<2))>>2;
	byte_t	fcachecontent=(optionflags2&(0x01<<3))>>3;
	byte_t	fusertype=(optionflags2&(0x07<<4))>>4;
	byte_t	fintsecurity=(optionflags2&(0x01<<7))>>7;

	b.clear();
	b.printBits(optionflags2);
	debugWrite("optionflags2: %s",b.getString());
	debugWrite("fsetlangfatal: %d (%s)",fsetlangfatal,
			(fsetlangfatal==SET_LANG_WARN)?"SET_LANG_WARN":
			(fsetlangfatal==SET_LANG_FATAL)?"SET_LANG_FATAL":
			"unknown set language flag");
	debugWrite("fodbc: %d (%s)",fodbc,
			(fodbc==ODBC_OFF)?"ODBC_OFF":
			(fodbc==ODBC_ON)?"ODBC_ON":
			"unknown odbc flag");
	debugWrite("ftranboundary: %d",ftranboundary);
	debugWrite("fcachecontent: %d",fcachecontent);
	debugWrite("fusertype: %d (%s)",fusertype,
			(fusertype==USER_NORMAL)?"USER_NORMAL":
			(fusertype==USER_SERVER)?"USER_SERVER":
			(fusertype==USER_REMUSER)?"USER_REMUSER":
			(fusertype==USER_SQLREPL)?"USER_SQLREPL":
			"unknown user type");
	debugWrite("fintsecurity: %d (%s)",fintsecurity,
			(fintsecurity==INTEGRATED_SECURITY_OFF)?
					"INTEGRATED_SECURITY_OFF":
			(fintsecurity==INTEGRATED_SECURITY_ON)?
					"INTEGRATED_SECURITY_ON":
			"unknown integrated security flag");

	// type flags
	byte_t	fsqltype=(typeflags&(0x0F));
	byte_t	foledb=(typeflags&(0x01<<4))>>4;
	byte_t	freadonlyintent=(typeflags&(0x01<<5))>>5;

	b.clear();
	b.printBits(typeflags);
	debugWrite("typeflags: %s",b.getString());
	debugWrite("fsqltype: %d (%s)",fsqltype,
			(fsqltype==SQL_DFLT)?"SQL_DFLT":
			(fsqltype==SQL_TSQL)?"SQL_TSQL":
			"unknown sql type");
	debugWrite("foledb: %d (%s)",foledb,
			(foledb==OLEDB_OFF)?"OLEDB_OFF":
			(foledb==OLEDB_ON)?"OLEDB_ON":
			"unknown oledb flag");
	debugWrite("freadonlyintent: %d",freadonlyintent);

	// option flags 3
	byte_t	fchangepassword=(optionflags3&(0x01));
	byte_t	fsendyukonbinaryxml=(optionflags3&(0x01<<1))>>1;
	byte_t	fuserinstance=(optionflags3&(0x01<<2))>>2;
	byte_t	funknowncollationhandling=(optionflags3&(0x01<<3))>>3;
	byte_t	fextension=(optionflags3&(0x01<<4))>>4;

	b.clear();
	b.printBits(optionflags3);
	debugWrite("optionflags3: %s",b.getString());
	debugWrite("fchangepassword: %d",fchangepassword);
	debugWrite("fsendyukonbinaryxml: %d",fsendyukonbinaryxml);
	debugWrite("fuserinstance: %d",fuserinstance);
	debugWrite("funknowncollationhandling: %d",funknowncollationhandling);
	debugWrite("fextension: %d",fextension);
}

void sqlrprotocol_tds::debugEnvChangeType(byte_t type) {
	if (!getDebug()) {
		return;
	}
	const char	*typestring=NULL;
	switch (type) {
		case ENV_CHANGE_DATABASE:
			typestring="ENV_CHANGE_DATABASE";
			break;
		case ENV_CHANGE_LANGUAGE:
			typestring="ENV_CHANGE_LANGUAGE";
			break;
		case ENV_CHANGE_CHARSET:
			typestring="ENV_CHANGE_CHARSET";
			break;
		case ENV_CHANGE_PACKET_SIZE:
			typestring="ENV_CHANGE_PACKET_SIZE";
			break;
		case ENV_CHANGE_UNICODE_DATA_SORTING_LOCAL_ID:
			typestring="ENV_CHANGE_UNICODE_DATA_"
					"SORTING_LOCAL_ID";
			break;
		case ENV_CHANGE_UNICODE_DATA_SORTING_COMPARISON_FLAGS:
			typestring="ENV_CHANGE_UNICODE_DATA_"
					"SORTING_COMPARISON_FLAGS";
			break;
		case ENV_CHANGE_SQL_COLLATION:
			typestring="ENV_CHANGE_SQL_COLLATION";
			break;
		case ENV_CHANGE_BEGIN_TRANSACTION:
			typestring="ENV_CHANGE_BEGIN_TRANSACTION";
			break;
		case ENV_CHANGE_COMMIT_TRANSACTION:
			typestring="ENV_CHANGE_COMMIT_TRANSACTION";
			break;
		case ENV_CHANGE_ROLLBACK_TRANSACTION:
			typestring="ENV_CHANGE_ROLLBACK_TRANSACTION";
			break;
		case ENV_CHANGE_ENLIST_DTC_TRANSACTION:
			typestring="ENV_CHANGE_ENLIST_DTC_TRANSACTION";
			break;
		case ENV_CHANGE_DEFECT_TRANSACTION:
			typestring="ENV_CHANGE_DEFECT_TRANSACTION";
			break;
		case ENV_CHANGE_REAL_TIME_LOG_SHIPPING:
			typestring="ENV_CHANGE_REAL_TIME_LOG_SHIPPING";
			break;
		case ENV_CHANGE_PROMOTE_TRANSACTION:
			typestring="ENV_CHANGE_PROMOTE_TRANSACTION";
			break;
		case ENV_CHANGE_TRANSACTION_MANAGER_ADDRESS:
			typestring="ENV_CHANGE_TRANSACTION_MANAGER_ADDRESS";
			break;
		case ENV_CHANGE_TRANSACTION_ENDED:
			typestring="ENV_CHANGE_TRANSACTION_ENDED";
			break;
		case ENV_CHANGE_RESETCONNECTION_COMPLETION_ACKNOWLEDGEMENT:
			typestring="ENV_CHANGE_RESETCONNECTION_"
					"COMPLETION_ACKNOWLEDGEMENT";
			break;
		case ENV_GET_USER_INSTANCE:
			typestring="ENV_GET_USER_INSTANCE";
			break;
		case ENV_GET_ROUTING_INFORMATION:
			typestring="ENV_GET_ROUTING_INFORMATION";
			break;
		default:
			typestring="unknown env change type";
			break;
	}
	debugWrite("type: %d (%s)",
			(uint32_t)(0x000000ff&type),typestring);
}

void sqlrprotocol_tds::debugDoneStatus(uint16_t status) {
	if (!getDebug()) {
		return;
	}
	debugWrite("status: 0x%04x",status);
	stringbuffer	b;
	b.printBits(status);
	debugWrite("%s",b.getString());
	if (status==DONE_FINAL) {
		debugWrite("DONE_FINAL");
	}
	if (status&DONE_MORE) {
		debugWrite("DONE_MORE");
	}
	if (status&DONE_ERROR) {
		debugWrite("DONE_ERROR");
	}
	if (status&DONE_INXACT) {
		debugWrite("DONE_INXACT");
	}
	if (status&DONE_COUNT) {
		debugWrite("DONE_COUNT");
	}
	if (status&DONE_ATTN) {
		debugWrite("DONE_ATTN");
	}
	if (status&DONE_RPCINBATCH) {
		debugWrite("DONE_RPCINBATCH");
	}
	if (status&DONE_SRVERROR) {
		debugWrite("DONE_SRVERROR");
	}
}

void sqlrprotocol_tds::debugAllHeadersType(uint16_t type) {
	if (!getDebug()) {
		return;
	}
	const char	*typestring=NULL;
	switch (type) {
		case ALL_HEADERS_QUERY_NOTIFICATIONS:
			typestring="ALL_HEADERS_QUERY_NOTIFICATIONS";
			break;
		case ALL_HEADERS_TRANSACTION_DESCRIPTOR:
			typestring="ALL_HEADERS_TRANSACTION_DESCRIPTOR";
			break;
		case ALL_HEADERS_TRACE_ACTIVITY:
			typestring="ALL_HEADERS_TRACE_ACTIVITY";
			break;
		default:
			typestring="unknown stream header type";
			break;
	}
	debugWrite("header type: 0x%04x (%s)",type,typestring);
}

void sqlrprotocol_tds::debugColumnType(byte_t type) {
	if (!getDebug()) {
		return;
	}
	const char	*typestring=NULL;
	switch (type) {
		case TDS_TYPE_NULL:
			typestring="TDS_TYPE_NULL";
			break;
		case TDS_TYPE_INT1:
			typestring="TDS_TYPE_INT1";
			break;
		case TDS_TYPE_BIT:
			typestring="TDS_TYPE_BIT";
			break;
		case TDS_TYPE_INT2:
			typestring="TDS_TYPE_INT2";
			break;
		case TDS_TYPE_INT4:
			typestring="TDS_TYPE_INT4";
			break;
		case TDS_TYPE_DATETIM4:
			typestring="TDS_TYPE_DATETIM4";
			break;
		case TDS_TYPE_FLT4:
			typestring="TDS_TYPE_FLT4";
			break;
		case TDS_TYPE_MONEY:
			typestring="TDS_TYPE_MONEY";
			break;
		case TDS_TYPE_DATETIME:
			typestring="TDS_TYPE_DATETIME";
			break;
		case TDS_TYPE_FLT8:
			typestring="TDS_TYPE_FLT8";
			break;
		case TDS_TYPE_MONEY4:
			typestring="TDS_TYPE_MONEY4";
			break;
		case TDS_TYPE_INT8:
			typestring="TDS_TYPE_INT8";
			break;
		case TDS_TYPE_GUID:
			typestring="TDS_TYPE_GUID";
			break;
		case TDS_TYPE_INTN:
			typestring="TDS_TYPE_INTN";
			break;
		case TDS_TYPE_DECIMAL:
			typestring="TDS_TYPE_DECIMAL";
			break;
		case TDS_TYPE_NUMERIC:
			typestring="TDS_TYPE_NUMERIC";
			break;
		case TDS_TYPE_BITN:
			typestring="TDS_TYPE_BITN";
			break;
		case TDS_TYPE_DECIMALN:
			typestring="TDS_TYPE_DECIMALN";
			break;
		case TDS_TYPE_NUMERICN:
			typestring="TDS_TYPE_NUMERICN";
			break;
		case TDS_TYPE_FLTN:
			typestring="TDS_TYPE_FLTN";
			break;
		case TDS_TYPE_MONEYN:
			typestring="TDS_TYPE_MONEYN";
			break;
		case TDS_TYPE_DATETIMN:
			typestring="TDS_TYPE_DATETIMN";
			break;
		case TDS_TYPE_DATEN:
			typestring="TDS_TYPE_DATEN";
			break;
		case TDS_TYPE_TIMEN:
			typestring="TDS_TYPE_TIMEN";
			break;
		case TDS_TYPE_DATETIME2N:
			typestring="TDS_TYPE_DATETIME2N";
			break;
		case TDS_TYPE_DATETIMEOFFSETN:
			typestring="TDS_TYPE_DATETIMEOFFSETN";
			break;
		case TDS_TYPE_CHAR:
			typestring="TDS_TYPE_CHAR";
			break;
		case TDS_TYPE_VARCHAR:
			typestring="TDS_TYPE_VARCHAR";
			break;
		case TDS_TYPE_BINARY:
			typestring="TDS_TYPE_BINARY";
			break;
		case TDS_TYPE_VARBINARY:
			typestring="TDS_TYPE_VARBINARY";
			break;
		case TDS_TYPE_BIGVARBIN:
			typestring="TDS_TYPE_BIGVARBIN";
			break;
		case TDS_TYPE_BIGVARCHR:
			typestring="TDS_TYPE_BIGVARCHR";
			break;
		case TDS_TYPE_BIGBINARY:
			typestring="TDS_TYPE_BIGBINARY";
			break;
		case TDS_TYPE_BIGCHAR:
			typestring="TDS_TYPE_BIGCHAR";
			break;
		case TDS_TYPE_NVARCHAR:
			typestring="TDS_TYPE_NVARCHAR";
			break;
		case TDS_TYPE_NCHAR:
			typestring="TDS_TYPE_NCHAR";
			break;
		case TDS_TYPE_LONGBINARY:
			typestring="TDS_TYPE_LONGBINARY";
			break;
		case TDS_TYPE_XML:
			typestring="TDS_TYPE_XML";
			break;
		case TDS_TYPE_UDT:
			typestring="TDS_TYPE_UDT";
			break;
		case TDS_TYPE_TEXT:
			typestring="TDS_TYPE_TEXT";
			break;
		case TDS_TYPE_IMAGE:
			typestring="TDS_TYPE_IMAGE";
			break;
		case TDS_TYPE_NTEXT:
			typestring="TDS_TYPE_NTEXT";
			break;
		case TDS_TYPE_SSVARIANT:
			typestring="TDS_TYPE_SSVARIANT";
			break;
		case TDS_TYPE_TVP:
			typestring="TDS_TYPE_TVP";
			break;
		default:
			typestring="unknown TDS_TYPE";
			break;
	}
	debugWrite("tdstype: 0x%02x (%s)",
			(uint32_t)(0x000000ff&type),typestring);
}

// Separate from debugColumnType() on purpose - these are tds 5.0
// datatypes, and several of the values name something else in ms-tds.
void sqlrprotocol_tds::debugPreTds7ColumnType(byte_t type) {
	if (!getDebug()) {
		return;
	}
	const char	*typestring=NULL;
	switch (type) {
		case TDS5_TYPE_VOID:
			typestring="TDS5_TYPE_VOID";
			break;
		case TDS5_TYPE_IMAGE:
			typestring="TDS5_TYPE_IMAGE";
			break;
		case TDS5_TYPE_TEXT:
			typestring="TDS5_TYPE_TEXT";
			break;
		case TDS5_TYPE_BLOB:
			typestring="TDS5_TYPE_BLOB";
			break;
		case TDS5_TYPE_VARBINARY:
			typestring="TDS5_TYPE_VARBINARY";
			break;
		case TDS5_TYPE_INTN:
			typestring="TDS5_TYPE_INTN";
			break;
		case TDS5_TYPE_VARCHAR:
			typestring="TDS5_TYPE_VARCHAR";
			break;
		case TDS5_TYPE_BINARY:
			typestring="TDS5_TYPE_BINARY";
			break;
		case TDS5_TYPE_INTERVAL:
			typestring="TDS5_TYPE_INTERVAL";
			break;
		case TDS5_TYPE_CHAR:
			typestring="TDS5_TYPE_CHAR";
			break;
		case TDS5_TYPE_INT1:
			typestring="TDS5_TYPE_INT1";
			break;
		case TDS5_TYPE_DATE:
			typestring="TDS5_TYPE_DATE";
			break;
		case TDS5_TYPE_BIT:
			typestring="TDS5_TYPE_BIT";
			break;
		case TDS5_TYPE_TIME:
			typestring="TDS5_TYPE_TIME";
			break;
		case TDS5_TYPE_INT2:
			typestring="TDS5_TYPE_INT2";
			break;
		case TDS5_TYPE_INT4:
			typestring="TDS5_TYPE_INT4";
			break;
		case TDS5_TYPE_SHORTDATE:
			typestring="TDS5_TYPE_SHORTDATE";
			break;
		case TDS5_TYPE_FLT4:
			typestring="TDS5_TYPE_FLT4";
			break;
		case TDS5_TYPE_MONEY:
			typestring="TDS5_TYPE_MONEY";
			break;
		case TDS5_TYPE_DATETIME:
			typestring="TDS5_TYPE_DATETIME";
			break;
		case TDS5_TYPE_FLT8:
			typestring="TDS5_TYPE_FLT8";
			break;
		case TDS5_TYPE_UINT1:
			typestring="TDS5_TYPE_UINT1";
			break;
		case TDS5_TYPE_UINT2:
			typestring="TDS5_TYPE_UINT2";
			break;
		case TDS5_TYPE_UINT4:
			typestring="TDS5_TYPE_UINT4";
			break;
		case TDS5_TYPE_UINT8:
			typestring="TDS5_TYPE_UINT8";
			break;
		case TDS5_TYPE_UINTN:
			typestring="TDS5_TYPE_UINTN";
			break;
		case TDS5_TYPE_SENSITIVITY:
			typestring="TDS5_TYPE_SENSITIVITY";
			break;
		case TDS5_TYPE_BOUNDARY:
			typestring="TDS5_TYPE_BOUNDARY";
			break;
		case TDS5_TYPE_DECN:
			typestring="TDS5_TYPE_DECN";
			break;
		case TDS5_TYPE_NUMN:
			typestring="TDS5_TYPE_NUMN";
			break;
		case TDS5_TYPE_FLTN:
			typestring="TDS5_TYPE_FLTN";
			break;
		case TDS5_TYPE_MONEYN:
			typestring="TDS5_TYPE_MONEYN";
			break;
		case TDS5_TYPE_DATETIMEN:
			typestring="TDS5_TYPE_DATETIMEN";
			break;
		case TDS5_TYPE_SHORTMONEY:
			typestring="TDS5_TYPE_SHORTMONEY";
			break;
		case TDS5_TYPE_DATEN:
			typestring="TDS5_TYPE_DATEN";
			break;
		case TDS5_TYPE_TIMEN:
			typestring="TDS5_TYPE_TIMEN";
			break;
		case TDS5_TYPE_XML:
			typestring="TDS5_TYPE_XML";
			break;
		case TDS5_TYPE_UNITEXT:
			typestring="TDS5_TYPE_UNITEXT";
			break;
		case TDS5_TYPE_LONGCHAR:
			typestring="TDS5_TYPE_LONGCHAR";
			break;
		case TDS5_TYPE_SINT1:
			typestring="TDS5_TYPE_SINT1";
			break;
		case TDS5_TYPE_INT8:
			typestring="TDS5_TYPE_INT8";
			break;
		case TDS5_TYPE_LONGBINARY:
			typestring="TDS5_TYPE_LONGBINARY";
			break;
		default:
			typestring="unknown TDS5_TYPE";
			break;
	}
	debugWrite("tds5type: 0x%02x (%s)",
			(uint32_t)(0x000000ff&type),typestring);
}

void sqlrprotocol_tds::debugCollation(uint32_t lcid, byte_t sortid) {
	if (!getDebug()) {
		return;
	}
	stringbuffer	b;
	b.printBits(lcid);
	debugWrite("collation: %s %d",b.getString(),
					(uint32_t)(0x000000ff&sortid));
	// lcid occupies the low 20 bits, the sort flags the next 8,
	// and the version the high 4 (MS-TDS 2.2.5.1.2)
	debugWrite("lcid: 0x%05x",lcid&0x000FFFFF);
	debugWrite("ignorecase: %d",(lcid&(0x01<<20))>>20);
	debugWrite("ignoreaccent: %d",(lcid&(0x01<<21))>>21);
	debugWrite("ignorewidth: %d",(lcid&(0x01<<22))>>22);
	debugWrite("ignorekana: %d",(lcid&(0x01<<23))>>23);
	debugWrite("binary: %d",(lcid&(0x01<<24))>>24);
	debugWrite("binary2: %d",(lcid&(0x01<<25))>>25);
	debugWrite("utf8: %d",(lcid&(0x01<<26))>>26);
	debugWrite("version: %d",(lcid&(0x0FU<<28))>>28);
	debugWrite("sortid: 0x%02x",(uint32_t)(0x000000ff&sortid));
}

void sqlrprotocol_tds::debugProcId(uint16_t procid) {
	if (!getDebug()) {
		return;
	}
	debugWrite("procid: %hd (%s)",procid,
			procids[(procid<=SP_MAX_PROCID)?procid:0]);
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrprotocol	*new_sqlrprotocol_tds(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrprotocol_tds(cont,parameters);
	}
}
