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
#define	PACKET_ATTENTION	13
#define	PACKET_CONTROL_INFO	14

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

// checksumming types (1-3 from an 8i trace, the rest from go-ora)
#define	CS_NONE		0
#define	CS_SHA1		3
#define CS_MD5		1
#define CS_SHA512	4
#define CS_SHA256	5
#define CS_SHA384	6

// two task common (ttc) types
#define TTC_PROTOCOL_NEGOTIATION	0x01
#define TTC_DATATYPE_NEGOTIATION	0x02
#define TTC_TTI_FUNCTION		0x03
#define TTC_ERROR			0x04
#define TTC_ROW_HEADER			0x06
#define TTC_ROW_DATA			0x07
#define TTC_OK				0x08
#define TTC_STATUS			0x09
#define TTC_IO_VECTOR			0x0b
#define TTC_DESCRIBE_INFO		0x10
#define TTC_EXTENDED_TTI_FUNCTION	0x11
#define TTC_BIT_VECTOR			0x15
#define TTC_EXTPROC1			0x20
#define TTC_EXTPROC2			0x44

// data flags.  one pair of bytes at the front of a data packet, describing the
// packet rather than any one message in it.
#define DATA_FLAGS_EOF			0x0040

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

// the version the module reports as its own - oracle 11.2.0.1.0, 0x0b200100,
// which is the version the rest of it answers as.
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
#define TTI_LOGON_UNKNOWN	0x54
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
// that 11.2 server and 24 on that 12.2 one.
#define SERVER_VERSION_NO_11_2		"186646784"
#define SERVER_VERSION_SQL_11_2		"22"
#define SERVER_VERSION_NO_12_1		"202375680"
#define SERVER_VERSION_SQL_12_1		"23"

// the version that the version response's banner announces, split into its
// components so the response can also send it as a packed version number.
// these have to match the banner string in sendVersionResponse() - change one
// and change the other.
#define SERVERVERSION_MAJOR		8
#define SERVERVERSION_MINOR		0
#define SERVERVERSION_THIRD		5
#define SERVERVERSION_PATCH		0
#define SERVERVERSION_FIFTH		0

// datatype request encoding flags
#define ENCODING_MULTI_BYTE		0x01
#define ENCODING_CONV_LENGTH		0x02

// a length byte over 252 isn't a length.  0xfe introduces the chunked long
// form and 0xff marks a null.
#define MAX_SHORT_LENGTH		252
#define LONG_LENGTH_INDICATOR		0xfe
#define CHUNK_SIZE			32767

// describe info constants.  the last three are advisory - a client is free to
// ignore them - and these are what a live 11.2 server sends.
#define AL8O4_COUNT			6
#define DCB_MAX_DATA_BLOCK_SIZE		8168
#define DCB_MIN_PREFETCH		2
#define DCB_MAX_PREFETCH		2

// an oracle number is an exponent byte and up to 20 base 100 digits, and a
// column of them is described as 22 bytes wide
#define MAX_NUMBER_MANTISSA		20
#define MAX_NUMBER_SIZE			22
#define MAX_NUMBER_DIGITS		128
#define MIN_NUMBER_EXPONENT		(-193)
#define MAX_NUMBER_EXPONENT		62

// what a column with no size of its own is described as
#define MAX_VARCHAR_SIZE		4000

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

// data types
#define DATA_TYPE_STRING	0
#define DATA_TYPE_UB2ARRAY	1
#define DATA_TYPE_UB1		2
#define DATA_TYPE_UB2		3
#define DATA_TYPE_UB4		4
#define DATA_TYPE_VERSION	5
#define DATA_TYPE_STATUS	6

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
#define ORACLE_TYPE_PLSQL_INDEX_TABLE	998
#define ORACLE_TYPE_FIXED_CHAR		999


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
	(uint16_t)ORACLE_TYPE_RAW,
	// "MLSLABEL"
	(uint16_t)ORACLE_TYPE_RAW,
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
		bool	getCryptoChecksummingService(const byte_t *rp,
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
		uint16_t	putCryptoChecksummingService();
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
		void	putBytesWithLength(const char *bytes, uint32_t size);
		void	putOracleDate(byte_t *out);
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
		void	debugOptions(uint16_t options);
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
							const char **query,
							uint32_t *querysize);
		bool	sendQuery3Response(sqlrservercursor *cursor,
							uint32_t options,
							uint32_t cursorid,
							uint32_t prefetchrows);
		void	putDescribeInfo(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putColumnMetadata(sqlrservercursor *cursor,
							uint32_t column);
		void	putColumnPrecisionScale(int8_t precision,
							int8_t scale);
		uint16_t	getWireColumnType(uint16_t columntype);
		uint32_t	getWireColumnSize(sqlrservercursor *cursor,
							uint32_t column,
							uint16_t wiretype);
		void	putRowHeader(byte_t flags,
							uint32_t colcount,
							uint32_t prefetchrows);
		void	putRowData(sqlrservercursor *cursor,
							uint32_t colcount);
		void	putReturnParameters();
		void	putSummary(uint32_t cursorid,
							uint32_t oranum,
							uint32_t rowcount,
							const char *message);
		void	putSummaryExtension(uint32_t oranum,
							uint32_t rowcount);
		void	putNumberField(const char *field,
							uint32_t fieldsize);

		// execute...
		bool	execute(const byte_t *rp);
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
		void	putRow(sqlrservercursor *cursor,
						uint32_t colcount,
						bool terminator);
		void	putField(const char *field,
						uint64_t fieldsize,
						uint16_t columntype);
		void	putLobField(sqlrservercursor *cursor, uint32_t col);
		void	putError(const char *error);
		void	putError(const char *error, uint32_t errorsize);

		// close...
		bool	close(const byte_t *rp);
		void	clearParams(sqlrservercursor *cursor);
		bool	sendCloseResponse(sqlrservercursor *cursor);

		// disconnect...
		bool	disconnect(const byte_t *rp);
		bool	sendDisconnectResponse();

		// version
		bool	version(const byte_t *rp);
		bool	sendVersionResponse();

		// occa
		bool	occa(const byte_t *rp, const byte_t **rpout);
		bool	switchSession(const byte_t *rp,
						const byte_t **rpout);

		// logon unknown
		bool	logonUnknown(const byte_t *rp);
		bool	sendLogonUnknownResponse();

		void	putGenericFooter();

		bool	sendQueryError(sqlrservercursor *cursor);
		bool	sendCursorNotOpenError();
		bool	sendNotImplementedError();

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
		uint32_t	cryptochecksummingversion;

		uint16_t	*encryptiondrivers;
		uint32_t	encryptiondrivercount;
		uint16_t	*cryptochecksummingdrivers;
		uint32_t	cryptochecksummingdrivercount;

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

		char		**bindvarnames;

		char		lobbuffer[32768];

		uint16_t	*pcounts;
		uint16_t	**ptypes;
		bool		*columntypescached;
		uint16_t	**columntypes;

		// how many rows of each cursor's result set have gone out,
		// since a fetch's summary has to carry the running total
		uint32_t	*rowssent;

		// the sequence number the summary object has to echo back
		byte_t		callnumber;

		bool		query3session;
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

	maxcursorcount=cont->getConfig()->getMaxCursors();
	maxquerysize=cont->getConfig()->getMaxQuerySize();
	maxbindcount=cont->getConfig()->getMaxBindCount();

	bindvarnames=new char *[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		charstring::printf(&bindvarnames[i],":%d",i+1);
	}

	pcounts=new uint16_t[maxcursorcount];
	ptypes=new uint16_t *[maxcursorcount];
	columntypescached=new bool[maxcursorcount];
	columntypes=new uint16_t *[maxcursorcount];
	rowssent=new uint32_t[maxcursorcount];
	for (uint16_t i=0; i<maxcursorcount; i++) {
		pcounts[i]=0;
		ptypes[i]=new uint16_t[maxbindcount];
		columntypescached[i]=false;
		rowssent[i]=0;
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
	}
	delete[] pcounts;
	delete[] ptypes;
	delete[] columntypescached;
	delete[] columntypes;
	delete[] rowssent;

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
	cryptochecksummingversion=0;

	encryptiondrivers=NULL;
	encryptiondrivercount=0;
	cryptochecksummingdrivers=NULL;
	cryptochecksummingdrivercount=0;

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
}

void sqlrprotocol_oracle::free() {

	delete[] encryptiondrivers;
	encryptiondrivers=NULL;
	delete[] cryptochecksummingdrivers;
	cryptochecksummingdrivers=NULL;

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
}

uint16_t hackcursorid=65535;

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
					debugWrite("unimplemented tti "
						"function TTI_AUTOCOMMIT_ON, "
						"ending session");
					loop=false;
					break;
				case TTI_AUTOCOMMIT_OFF:
					debugWrite("unimplemented tti "
						"function TTI_AUTOCOMMIT_OFF, "
						"ending session");
					loop=false;
					break;
				case TTI_COMMIT:
					debugWrite("unimplemented tti "
						"function TTI_COMMIT, "
						"ending session");
					loop=false;
					break;
				case TTI_ROLLBACK:
					debugWrite("unimplemented tti "
						"function TTI_ROLLBACK, "
						"ending session");
					loop=false;
					break;
				case TTI_CANCEL:
					debugWrite("unimplemented tti "
						"function TTI_CANCEL, "
						"ending session");
					loop=false;
					break;
				case TTI_DESCRIBE:
				case TTI_DESCRIBE2:
					debugWrite("unimplemented tti "
						"function TTI_DESCRIBE"
						"%s, ending session",
						(ttifunction==TTI_DESCRIBE2)?
							"2":"");
					loop=false;
					break;
				case TTI_STARTUP:
				case TTI_STARTUP2:
					debugWrite("unimplemented tti "
						"function TTI_STARTUP"
						"%s, ending session",
						(ttifunction==TTI_STARTUP2)?
							"2":"");
					loop=false;
					break;
				case TTI_SHUTDOWN:
					debugWrite("unimplemented tti "
						"function TTI_SHUTDOWN, "
						"ending session");
					loop=false;
					break;
				case TTI_VERSION:
					loop=version(rp);
					rp=NULL;
					break;
				case TTI_K2_TRANSACTIONS:
					debugWrite("unimplemented tti "
						"function "
						"TTI_K2_TRANSACTIONS, "
						"ending session");
					loop=false;
					break;
				case TTI_OSQL7:
					debugWrite("unimplemented tti "
						"function TTI_OSQL7, "
						"ending session");
					loop=false;
					break;
				case TTI_OKOD:
					debugWrite("unimplemented tti "
						"function TTI_OKOD, "
						"ending session");
					loop=false;
					break;
				case TTI_LOB_OPERATIONS:
					debugWrite("unimplemented tti "
						"function "
						"TTI_LOB_OPERATIONS, "
						"ending session");
					loop=false;
					break;
				case TTI_ODNY:
					debugWrite("unimplemented tti "
						"function TTI_ODNY, "
						"ending session");
					loop=false;
					break;
				case TTI_TRANSACTION_END:
					debugWrite("unimplemented tti "
						"function "
						"TTI_TRANSACTION_END, "
						"ending session");
					loop=false;
					break;
				case TTI_TRANSACTION_BEGIN:
					debugWrite("unimplemented tti "
						"function "
						"TTI_TRANSACTION_BEGIN, "
						"ending session");
					loop=false;
					break;
				case TTI_OCCA:
					loop=occa(rp,&rp);
					break;
				case TTI_LOGON_PRESENT_PWD:
					debugWrite("unimplemented tti "
						"function "
						"TTI_LOGON_PRESENT_PWD, "
						"ending session");
					loop=false;
					break;
				case TTI_LOGON_PRESENT_USER:
					debugWrite("unimplemented tti "
						"function "
						"TTI_LOGON_PRESENT_USER, "
						"ending session");
					loop=false;
					break;
				case TTI_LOGON_UNKNOWN:
					loop=logonUnknown(rp);
					rp=NULL;
					break;
				case TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD:
					debugWrite("unimplemented tti "
						"function "
						"TTI_LOGON_PRESENT_PWD_"
						"SEND_AUTH_PASSWORD, "
						"ending session");
					loop=false;
					break;
				case TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY:
					debugWrite("unimplemented tti "
						"function "
						"TTI_LOGON_PRESENT_USER_"
						"REQ_AUTH_SESSKEY, "
						"ending session");
					loop=false;
					break;
				case TTI_OOTCM:
					debugWrite("unimplemented tti "
						"function TTI_OOTCM, "
						"ending session");
					loop=false;
					break;
				case TTI_OKPFC:
					debugWrite("unimplemented tti "
						"function TTI_OKPFC, "
						"ending session");
					loop=false;
					break;
				case TTI_SWITCH_SESSION:
					// one function code, two meanings
					// (a piggyback names the session the
					// call behind it is for; an ordinary
					// call is an 8.0.5 client asking for
					// the server version)
					if (lastttccode==
						TTC_EXTENDED_TTI_FUNCTION) {
						loop=switchSession(rp,&rp);
					} else {
						loop=version(rp);
						rp=NULL;
					}
					break;
				case TTI_OSCID:
					debugWrite("unimplemented tti "
						"function TTI_OSCID, "
						"ending session");
					loop=false;
					break;
				case TTI_OSKEYVAL:
					debugWrite("unimplemented tti "
						"function TTI_OSKEYVAL, "
						"ending session");
					loop=false;
					break;
				default:
					// FIXME: bad options...
					debugWrite("unrecognized tti "
						"function 0x%02x, "
						"ending session",
						ttifunction);
					loop=false;
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
}

bool sqlrprotocol_oracle::sendPacket() {
	return sendPacket(false);
}

bool sqlrprotocol_oracle::sendPacket(bool flush) {

	uint32_t	reqpacketsize=(uint32_t)reqpacket.getSize();
	uint16_t	packetchecksum=0;
	byte_t		packetflags=0;
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
	reqpacket.write(packetflags);
	reqpacket.write(hostToBE(headerchecksum));

	if (getDebug()) {
		debugStart("send");
		debugWrite("large header: %s",largeheader?"yes":"no");
		debugWrite("packet size: %d",reqpacketsize);
		if (!largeheader) {
			debugWrite("packet checksum: %d",packetchecksum);
		}
		debugWrite("packet type: %d",reqpackettype);
		debugWrite("packet flags: 0x%04x",packetflags);
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
	// writeLE?
	writeHost(&reqpacket,(uint16_t)1);
	writeBE(&reqpacket,datasize);
	writeBE(&reqpacket,dataoffset);
	// echo the client's ano flags back
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

	uint32_t	csdrivers=anoDriversOffered(
						cryptochecksummingdrivers,
						cryptochecksummingdrivercount);
	if (csdrivers) {
		debugWrite("declining %d offered crypto-checksumming "
						"driver(s)",csdrivers);
		cont->raiseInternalWarningEvent(NULL,
			"client requested oracle crypto-checksumming "
			"(%d algorithms).  this module doesn't implement it "
			"and has declined it.  a client with "
			"SQLNET.CRYPTO_CHECKSUM_CLIENT=REQUIRED will fail "
			"with ORA-12660; use ACCEPTED, REQUESTED or "
			"REJECTED.",
			csdrivers);
	}

	if (!encdrivers && !csdrivers) {
		debugWrite("nothing declined - client offered no "
					"encryption or checksumming drivers");
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
	if (!readMarker32(rp,0xdeadbeef,&rp)) {
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
			case 4:
				success=getSupervisorService(
							rp,end,fieldcount,&rp);
				break;
			case 1:
				success=getAuthenticationService(
							rp,end,fieldcount,&rp);
				break;
			case 2:
				success=getEncryptionService(
							rp,end,fieldcount,&rp);
				break;
			case 3:
				success=getCryptoChecksummingService(
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

bool sqlrprotocol_oracle::getCryptoChecksummingService(
						const byte_t *rp,
						const byte_t *end,
						uint16_t fieldcount,
						const byte_t **rpout) {

	debugStart("crypto-checksumming");
	debugWrite("service: ANO Crypto-Checksumming");

	uint16_t	*drivers=NULL;
	uint32_t	drivercount;
	if (!getAnoVersionField(rp,end,&cryptochecksummingversion,&rp) ||
		!getAnoDriverListField(rp,end,&drivers,&drivercount,&rp)) {
		delete[] drivers;
		debugEnd();
		return false;
	}

	// see getEncryptionService()
	delete[] cryptochecksummingdrivers;
	cryptochecksummingdrivers=drivers;
	cryptochecksummingdrivercount=drivercount;

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
		!readBE(rp,&type,"type",5,&rp)) {
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
		!readBE(rp,&type,"type",1,&rp)) {
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

	// get the field type, should be 1 (UB2Array)
	uint16_t	type;
	if (!readBE(rp,&type,"type",1,&rp)) {
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
		!readMarker32(rp,0xdeadbeef,&rp) ||
		!readMarker16(rp,0x0003,&rp)) {

		// a field sometimes has an array marker and no deadbeef, and
		// sometimes neither.  both shapes are the encryption and
		// crypto-checksumming services' driver lists - one byte per
		// algorithm id, with a field header identical to this one -
		// and they have their own reader now, so neither reaches this
		// function.  the two differ only in whether the first two
		// algorithm ids happen to spell the array marker: ojdbc 23.26
		// offers 0, 3, 4, 5 and 6 for crypto-checksumming, and the
		// first two of those are the bytes 00 03.
		//
		// the one caller left is getSupervisorService(), and both
		// clients that reach ano here send a real deadbeef ub2 array,
		// so this is a guard rather than a decoder - hence the dump.
		// returning NULL/0 without failing is deliberate: a supervisor
		// list nobody can read is not worth refusing a connection over.
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
	if (!readBE(rp,&type,"type",1,&rp)) {
		debugEnd();
		return false;
	}

	// the size counts the bytes after the size and type
	if (!anoBoundsCheck(rp,end,size,"driver list field")) {
		debugEnd();
		return false;
	}

	// the encryption and crypto-checksumming services send one byte per
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
		!readBE(rp,&type,"type",3,&rp)) {
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
		!readBE(rp,&type,"type",2,&rp)) {
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
		!readBE(rp,&type,"type",6,&rp)) {
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
	writeBE(&reqpacket,(uint32_t)0xdeadbeef);
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
	overallsize+=putCryptoChecksummingService();
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

	uint16_t drivers[]={0x0004,0x0001};

	uint16_t	size=putAnoServiceHeader(4,3)+
				putAnoVersionField(supervisorversion)+
				putAnoStatusField(0x001f)+
				putAnoArrayField(drivers,2);

	debugEnd();
	return size;
}

uint16_t sqlrprotocol_oracle::putAuthenticationService() {

	debugStart("authentication");
	debugWrite("service: ANO Authentication");

	uint16_t	size=putAnoServiceHeader(1,2)+
				putAnoVersionField(authenticationversion)+
				putAnoStatusField(0xfbff);

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
	uint16_t	size=putAnoServiceHeader(2,2)+
				putAnoVersionField(encryptionversion)+
				putAnoConstant((byte_t)ENC_NONE);

	debugEnd();
	return size;
}

uint16_t sqlrprotocol_oracle::putCryptoChecksummingService() {

	debugStart("crypto-checksumming");
	debugWrite("service: ANO Crypto-Checksumming");

	// declined for the reasons in putEncryptionService()
	uint16_t	size=putAnoServiceHeader(3,2)+
				putAnoVersionField(cryptochecksummingversion)+
				putAnoConstant((byte_t)CS_NONE);

	debugEnd();
	return size;
}

uint16_t sqlrprotocol_oracle::putAnoServiceHeader(uint16_t service,
							uint16_t fieldcount) {

	debugStart("ano service header");
	debugWrite("service: %d",service);
	debugWrite("field count: %d",fieldcount);
	switch (service) {
		case 1:
			debugWrite("service name: ANO Authentication");
			break;
		case 2:
			debugWrite("service name: ANO Encryption");
			break;
		case 3:
			debugWrite("service name: ANO Crypto-Checksumming");
			break;
		case 4:
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
	writeBE(&reqpacket,(uint32_t)0x00000000);
	return 8;
}

uint16_t sqlrprotocol_oracle::putAnoVersionField(uint32_t version) {

	debugStart("version");
	debugWrite("version: 0x%08x",version);
	debugEnd();

	// data size, field type, version, return total size
	writeBE(&reqpacket,(uint16_t)4);
	writeBE(&reqpacket,(uint16_t)5);
	writeBE(&reqpacket,version);
	return 8;
}

uint16_t sqlrprotocol_oracle::putAnoStatusField(uint16_t status) {

	debugStart("status");
	debugWrite("status: 0x%04x",status);
	debugEnd();

	// data size, field type, status, return total size
	writeBE(&reqpacket,(uint16_t)2);
	writeBE(&reqpacket,(uint16_t)6);
	writeBE(&reqpacket,status);
	return 6;
}

uint16_t sqlrprotocol_oracle::putAnoConstant(byte_t constant) {

	debugStart("constant");
	debugWrite("constant: 0x%02x",constant);
	debugEnd();

	// data size, field type, constant, return total size
	writeBE(&reqpacket,(uint16_t)1);
	writeBE(&reqpacket,(uint16_t)2);
	write(&reqpacket,constant);
	return 5;
}

uint16_t sqlrprotocol_oracle::putAnoArrayField(uint16_t *array,
						uint32_t arraycount) {

	debugStart("array field");

	// data size, field type
	uint16_t datasize=((arraycount)?(4+2+4+arraycount*2):1);
	writeBE(&reqpacket,(uint16_t)((arraycount)?(4+2+4+arraycount*2):1));
	writeBE(&reqpacket,(uint16_t)1);

	debugWrite("array count: %d",arraycount);

	if (arraycount) {

		// deadbeef marker, array marker,
		// array count, array members
		writeBE(&reqpacket,(uint32_t)0xdeadbeef);
		writeBE(&reqpacket,(uint16_t)0x003);
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


	// database charset
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

	// the client's remote-in and remote-out character sets, in host order.
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
	if (length==0xfe) {
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

void sqlrprotocol_oracle::putLenString(const char *string, uint32_t size) {
	write(&reqpacket,(byte_t)size);
	write(&reqpacket,string,(size_t)size);
}

void sqlrprotocol_oracle::putLenBytes(const char *bytes, uint32_t size) {

	if (size<=MAX_SHORT_LENGTH) {
		write(&reqpacket,(byte_t)size);
		if (size) {
			write(&reqpacket,bytes,(size_t)size);
		}
		return;
	}

	// the long form: a 0xfe marker, then a ub4 length and that many bytes
	// per chunk, then a zero length
	// (a row value needs it and an authentication field never did, which
	// is why putLenString() doesn't have it)
	write(&reqpacket,(byte_t)LONG_LENGTH_INDICATOR);
	uint32_t	offset=0;
	while (offset<size) {
		uint32_t	chunk=size-offset;
		if (chunk>CHUNK_SIZE) {
			chunk=CHUNK_SIZE;
		}
		writeLenPreInt(&reqpacket,chunk);
		write(&reqpacket,bytes+offset,(size_t)chunk);
		offset+=chunk;
	}
	writeLenPreInt(&reqpacket,0);
}

void sqlrprotocol_oracle::putBytesWithLength(const char *bytes,
							uint32_t size) {
	debugWrite("bytes with length: %d",size);
	debugHexDump((const byte_t *)bytes,size);
	writeLenPreInt(&reqpacket,size);
	if (size) {
		putLenBytes(bytes,size);
	}
}

void sqlrprotocol_oracle::putOracleDate(byte_t *out) {

	datetime	dt;
	dt.initFromSystemDateTime();

	int32_t	year=dt.getYear();

	out[0]=(byte_t)(year/100+100);
	out[1]=(byte_t)(year%100+100);
	out[2]=(byte_t)dt.getMonth();
	out[3]=(byte_t)dt.getDayOfMonth();
	out[4]=(byte_t)(dt.getHour()+1);
	out[5]=(byte_t)(dt.getMinute()+1);
	out[6]=(byte_t)(dt.getSecond()+1);

	debugWrite("date: %04d-%02d-%02d %02d:%02d:%02d",
			year,dt.getMonth(),dt.getDayOfMonth(),
			dt.getHour(),dt.getMinute(),dt.getSecond());
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

	debugStart(what);
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

	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		debugWrite("couldn't get cursor");
		return sendCursorNotOpenError();
	}

	uint16_t	cursorid=cont->getId(cursor);
hackcursorid=cursorid;

	// FIXME: decode the rest of this...
	uint16_t	options=0;
	readBE(rp,&options,&rp);

	debugStart("open request");
	debugWrite("cursor id: %d",cursorid);
	debugOptions(options);
	debugEnd();

	return sendOpenResponse(cursor);
}

bool sqlrprotocol_oracle::sendOpenResponse(sqlrservercursor *cursor) {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;
	byte_t		unknown[]={
		0x01, 0x00, 0x00, 0x00, 0x09
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	reqpacket.append(unknown,sizeof(unknown));

	uint16_t	cursorid=cont->getId(cursor);

	debugStart("open response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("cursor id: %d",cursorid);
	debugWrite("unknown: %02x %02x %02x %02x %02x",
			unknown[0],unknown[1],unknown[2],
			unknown[3],unknown[4]);
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
		case TTC_DESCRIBE_INFO:
			code="TTC_DESCRIBE_INFO";
			break;
		case TTC_BIT_VECTOR:
			code="TTC_BIT_VECTOR";
			break;
		case TTC_EXTENDED_TTI_FUNCTION:
			code="TTC_EXTENDED_TTI_FUNCTION";
			break;
		case TTC_EXTPROC1:
			code="TTC_EXTPROC1";
			break;
		case TTC_EXTPROC2:
			code="TTC_EXTPROC2";
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
		case TTI_LOGON_UNKNOWN:
			func="TTI_LOGON_UNKNOWN";
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

void sqlrprotocol_oracle::debugOptions(uint16_t options) {
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
		if (!recvPacket()) {
			return false;
		}

		if (resppackettype!=PACKET_DATA) {
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
						"end of file":"empty packet");
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

	if (!read(rp,&ttccode,"ttccode",TTC_TTI_FUNCTION,&rp) &&
		!read(rp,&ttccode,"ttccode",TTC_EXTENDED_TTI_FUNCTION,&rp)) {
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

	// sqlplus 8.0.5, 8i, 9i
	// call this to prepare some initial queries
	// sqlplus 10g+ use query3

	// parse the request...
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
cursorid=hackcursorid;

	debugStart("query request");
	debugOptions(options,moreoptions);
	debugWrite("cursor id: %d",cursorid);
	debugWrite("unknown: %02x %02x %02x",unknown3,unknown4,unknown5);
	debugWrite("query size: %d",querysize);
	debugWrite("unknown: %02x %02x",unknown6,unknown7);
	debugWrite("query: \"%*s\"",querysize,query);
	debugEnd();

	// get the requested cursor
	sqlrservercursor	*cursor=cont->getCursor(cursorid);
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError();
	}

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

	// FIXME: decode this...

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

	// sqlplus 8.0.5, 8i, 9i
	// call this to prepare/execute some initial queries
	// sqlplus 10g+ use query3
	// can apparently be used for fetch too

	// parse the request...
	uint16_t	options;
	uint16_t	moreoptions;
	uint16_t	cursorid;
	uint32_t	querysize=0;
	const char	*query=NULL;

	readBE(rp,&options,&rp);
	readBE(rp,&moreoptions,&rp);
	readBE(rp,&cursorid,&rp);
cursorid=hackcursorid;
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

	// get the requested cursor
	sqlrservercursor	*cursor=cont->getCursor(cursorid);
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError();
	}

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

		// FIXME: get these somehow...
		uint16_t	pcount=1;
		uint16_t	ptypes[]={ORACLE_TYPE_VARCHAR};
		
		if (!bindParameters(cursor,pcount,ptypes)) {
			return false;
		}
	}

	if (options&OPTION_EXECUTE) {

		// execute the query
		if (!cont->executeQuery(cursor,true,true,true,true)) {
			debugWrite("execute query failed");
			return sendQueryError(cursor);
		}
	}

	if (options&OPTION_COMMIT) {
		// FIXME: commit...
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

		// FIXME: decode this...

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

		// FIXME: decode this...

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

		uint16_t	dataflags;
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

		// FIXME: handle nulls
		if (false) {
			bv->type=SQLRSERVERBINDVARTYPE_NULL;
			bv->isnull=cont->getNullBindValue();
			debugWrite("type: NULL");
			debugWrite("isnull: true");
			debugEnd();
			continue;
		}

		// handle non-nulls
		switch (ptypes[i]) {
			/*case MYSQL_TYPE_TINY:
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->value.integerval=*((char *)rp);
				bv->isnull=cont->getNonNullBindValue();
				rp+=sizeof(char);
				break;
			case MYSQL_TYPE_SHORT:
				{
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				uint16_t	val;
				bytestring::copy(&val,rp,sizeof(uint16_t));
				val=leToHost((uint16_t)val);
				bv->value.integerval=(int16_t)val;
				bv->isnull=cont->getNonNullBindValue();
				rp+=sizeof(int16_t);
				}
				break;
			case MYSQL_TYPE_LONG:
				{
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				uint32_t	val;
				bytestring::copy(&val,rp,sizeof(uint32_t));
				val=leToHost((uint32_t)val);
				bv->value.integerval=(int32_t)val;
				bv->isnull=cont->getNonNullBindValue();
				rp+=sizeof(int32_t);
				}
				break;
			case MYSQL_TYPE_LONGLONG:
				{
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				uint64_t	val;
				bytestring::copy(&val,rp,sizeof(uint64_t));
				val=leToHost((uint64_t)val);
				bv->value.integerval=(int64_t)val;
				bv->isnull=cont->getNonNullBindValue();
				rp+=sizeof(int64_t);
				}
				break;
			case MYSQL_TYPE_FLOAT:
				bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
				bytestring::copy(&bv->value.doubleval.value,
							rp,sizeof(float));
				bv->value.doubleval.precision=0;
				bv->value.doubleval.scale=0;
				bv->isnull=cont->getNonNullBindValue();
				rp+=sizeof(float);
				break;
			case MYSQL_TYPE_DOUBLE:
				bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
				bytestring::copy(&bv->value.doubleval.value,
							rp,sizeof(double));
				bv->value.doubleval.precision=0;
				bv->value.doubleval.scale=0;
				bv->isnull=cont->getNonNullBindValue();
				rp+=sizeof(double);
				break;
			case MYSQL_TYPE_TIME:
				{
				bv->type=SQLRSERVERBINDVARTYPE_DATE;
				bv->value.dateval.year=-1;
				bv->value.dateval.month=-1;
				bv->value.dateval.day=0;
				bv->value.dateval.hour=0;
				bv->value.dateval.minute=0;
				bv->value.dateval.second=0;
				bv->value.dateval.microsecond=0;
				bv->value.dateval.tz=NULL;
				bv->value.dateval.isnegative=false;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.dateval.buffersize=64;
				bv->value.dateval.buffer=
					new char[bv->value.dateval.buffersize];

				char	size=*((char *)rp);
				rp+=sizeof(char);

				if (size) {
					char	isneg=*((char *)rp);
					bv->value.dateval.isnegative=isneg;
					rp+=sizeof(char);

					int32_t	days;
					bytestring::copy(&days,
							rp,sizeof(int32_t));
					bv->value.dateval.day=
						leToHost((uint32_t)days);
					rp+=sizeof(int32_t);
					
					bv->value.dateval.hour=
							*((char *)rp);
					rp+=sizeof(char);
					bv->value.dateval.minute=
							*((char *)rp);
					rp+=sizeof(char);
					bv->value.dateval.second=
							*((char *)rp);
					rp+=sizeof(char);
					if (size>8) {
						int32_t	ms;
						bytestring::copy(&ms,
							rp,sizeof(int32_t));
						bv->value.dateval.
							microsecond=
							leToHost((uint32_t)ms);
						rp+=sizeof(int32_t);
					}
				}
				}
				break;
			case MYSQL_TYPE_DATE:
				{
				bv->type=SQLRSERVERBINDVARTYPE_DATE;
				bv->value.dateval.year=0;
				bv->value.dateval.month=0;
				bv->value.dateval.day=0;
				bv->value.dateval.hour=-1;
				bv->value.dateval.minute=-1;
				bv->value.dateval.second=-1;
				bv->value.dateval.microsecond=-1;
				bv->value.dateval.tz=NULL;
				bv->value.dateval.isnegative=false;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.dateval.buffersize=64;
				bv->value.dateval.buffer=
					new char[bv->value.dateval.buffersize];

				char	size=*((char *)rp);
				rp+=sizeof(char);

				if (size) {
					int16_t	year;
					bytestring::copy(&year,
							rp,sizeof(int16_t));
					bv->value.dateval.year=
						leToHost((uint16_t)year);
					rp+=sizeof(int16_t);
					bv->value.dateval.month=*((char *)rp);
					rp+=sizeof(char);
					bv->value.dateval.day=*((char *)rp);
					rp+=sizeof(char);

					// ignore time parts
					if (size>4) {
						rp+=3*sizeof(char);
						if (size>7) {
							rp+=sizeof(int32_t);
						}
					}
				}
				}
				break;
			case MYSQL_TYPE_DATETIME:
			case MYSQL_TYPE_TIMESTAMP:
				{
				bv->type=SQLRSERVERBINDVARTYPE_DATE;
				bv->value.dateval.year=0;
				bv->value.dateval.month=0;
				bv->value.dateval.day=0;
				bv->value.dateval.hour=0;
				bv->value.dateval.minute=0;
				bv->value.dateval.second=0;
				bv->value.dateval.microsecond=0;
				bv->value.dateval.tz=NULL;
				bv->value.dateval.isnegative=false;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.dateval.buffersize=64;
				bv->value.dateval.buffer=
					new char[bv->value.dateval.buffersize];

				char	size=*((char *)rp);
				rp+=sizeof(char);

				if (size) {
					int16_t	year;
					bytestring::copy(&year,
							rp,sizeof(int16_t));
					bv->value.dateval.year=
						leToHost((uint16_t)year);
					rp+=sizeof(int16_t);
					bv->value.dateval.month=*((char *)rp);
					rp+=sizeof(char);
					bv->value.dateval.day=*((char *)rp);
					rp+=sizeof(char);
					if (size>4) {
						bv->value.dateval.hour=
								*((char *)rp);
						rp+=sizeof(char);
						bv->value.dateval.minute=
								*((char *)rp);
						rp+=sizeof(char);
						bv->value.dateval.second=
								*((char *)rp);
						rp+=sizeof(char);
						if (size>7) {
							int32_t	ms;
							bytestring::copy(&ms,
							rp,sizeof(int32_t));
							bv->value.dateval.
								microsecond=
							leToHost((uint32_t)ms);
							rp+=sizeof(int32_t);
						}
					}
				}
				}
				break;
			case MYSQL_TYPE_TINY_BLOB:
			case MYSQL_TYPE_MEDIUM_BLOB:
			case MYSQL_TYPE_LONG_BLOB:
			case MYSQL_TYPE_BLOB:
				bv->type=SQLRSERVERBINDVARTYPE_BLOB;
				bv->valuesize=getLenEncInt(rp,&rp);
				bv->value.stringval=charstring::duplicate(
							(const char *)rp,
							bv->valuesize);
				bv->isnull=cont->getNonNullBindValue();
				rp+=bv->valuesize;
				break;
			*/
			case ORACLE_TYPE_VARCHAR:
			// (for all other types, assume varchar)
			default:
				{
				byte_t	size;
				read(rp,&size,&rp);

				bv->type=SQLRSERVERBINDVARTYPE_STRING;
				bv->valuesize=size;
				bv->value.stringval=charstring::duplicate(
							(const char *)rp,
							bv->valuesize);
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
			}
			debugWrite("value size: %d",bv->valuesize);
			debugWrite("isnull: false");
			debugEnd();
		}
	}

	return true;
}

bool sqlrprotocol_oracle::query3(const byte_t *rp) {

	// all versions call this to open/prepare/describe/execute most queries
	// can apparently be used for fetch too

	// parse the request...
	uint32_t	options=0;
	uint32_t	cursorid=0;
	uint32_t	prefetchrows=0;
	uint32_t	querysize=0;
	const char	*query=NULL;

	if (!getQuery3Request(rp,resppacket+resppacketsize,
					&options,&cursorid,&prefetchrows,
					&query,&querysize)) {
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
		hackcursorid=cont->getId(cursor);
		cursorid=hackcursorid+1;
		debugStart("open request");
		debugWrite("cursor id: %d",cursorid);
		debugEnd();
	} else {
		cursor=cont->getCursor((uint16_t)(cursorid-1));
		if (!cursor) {
			debugWrite("cursor id %d not found",cursorid);
			return sendCursorNotOpenError();
		}
		hackcursorid=cursorid-1;
	}

	if (options&OPTION_PARSE) {

		// reset column type cache flag
		columntypescached[cont->getId(cursor)]=false;

		// re-start the running row count
		rowssent[cont->getId(cursor)]=0;

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

	if (options&OPTION_EXECUTE) {

		// execute the query
		if (!cont->executeQuery(cursor,true,true,true,true)) {
			debugWrite("execute query failed");
			return sendQueryError(cursor);
		}
	}

	if (options&OPTION_COMMIT) {
		// FIXME: commit...
	}

	return sendQuery3Response(cursor,options,cursorid,prefetchrows);
}

bool sqlrprotocol_oracle::getQuery3Request(const byte_t *rp,
						const byte_t *end,
						uint32_t *options,
						uint32_t *cursorid,
						uint32_t *prefetchrows,
						const char **query,
						uint32_t *querysize) {

	// the layout is python-oracledb's _write_execute_message(), in
	// src/oracledb/impl/thin/messages/execute.pyx - every field is either
	// a ub4 or a single raw byte, so nothing sits at a fixed offset
	*options=0;
	*cursorid=0;
	*prefetchrows=0;
	*query=NULL;
	*querysize=0;

	byte_t		sequence=0;
	byte_t		pointer=0;
	uint32_t	vectorsize=0;
	uint32_t	prefetchbuffersize=0;
	uint32_t	maxlongsize=0;
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
		!readLenPreInt(rp,end,&maxlongsize,&rp) ||
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

		if (rp<end && *rp==LONG_LENGTH_INDICATOR) {
			debugWrite("chunked query text, not supported");
			return false;
		}

		// a length byte in front of the text.  python-oracledb's
		// declared size is the byte count and matches it; OCI's is a
		// buffer size - the character count times the bytes per
		// character of its charset - the same thing it does with the
		// user name in the login, and the length byte is the one to
		// believe either way.  OCI's case is only taken when the
		// declared size is more than the packet has left, which is
		// what makes it unambiguous.
		if (rp<end && *rp && (uint32_t)(*rp)<*querysize &&
			(size_t)(end-rp)<(size_t)*querysize &&
			(*querysize==(uint32_t)(*rp)*2 ||
				*querysize==(uint32_t)(*rp)*3 ||
				*querysize==(uint32_t)(*rp)*4)) {
			*querysize=*rp;
			rp++;
		} else if (rp<end && *querysize<=MAX_SHORT_LENGTH &&
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

	// the summary object has to echo this back
	callnumber=sequence;

	if (getDebug()) {
		debugStart("query3 request");
		debugWrite("sequence: %d",sequence);
		debugWrite("options: 0x%08x",*options);
		debugOptions((uint16_t)*options);
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

	// FIXME: bind variables and defines are still ignored
	return true;
}

bool sqlrprotocol_oracle::sendQuery3Response(sqlrservercursor *cursor,
						uint32_t options,
						uint32_t cursorid,
						uint32_t prefetchrows) {

	// what a live 11.2 server answers a select with, in order: a describe,
	// a row header, one row data message per row, a return parameters
	// block, and a summary object, each with its own ttc code inside the
	// one packet
	resetSendPacketBuffer(PACKET_DATA);

	uint32_t	colcount=cont->colCount(cursor);
	cacheColumnDefinitions(cursor,colcount);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	uint32_t	rowsfetched=0;
	bool		endofrows=false;

	if (colcount) {

		// the describe, which the client needs before it can make
		// sense of a row
		if (options&OPTION_PARSE) {
			putDescribeInfo(cursor,colcount);
		}

		// the rows.  a modern client doesn't set OPTION_FETCH on the
		// first execute - it asks for a prefetch through the row count
		// in the request, and a real server sends rows for that anyway.
		uint32_t	rowstofetch=prefetchrows;
		if (!rowstofetch && (options&OPTION_FETCH)) {
			rowstofetch=1;
		}

		if (rowstofetch) {

			putRowHeader(0x22,colcount,rowstofetch);

			while (rowsfetched<rowstofetch) {

				bool	error=false;
				if (!cont->fetchRow(cursor,&error)) {
					if (error) {
						// FIXME: handle error
					}
					endofrows=true;
					break;
				}

				debugStart("query3 response row");
				putRowData(cursor,colcount);
				debugEnd();

				// FIXME: kludgy
				cont->nextRow(cursor);

				rowsfetched++;
			}
		}
	}

	putReturnParameters();

	rowssent[cont->getId(cursor)]+=rowsfetched;

	// a prefetch that ran out of rows ends in ORA-01403, which the client
	// reads as "no more rows"; one that filled it ends clean
	if (endofrows) {
		putSummary(cursorid,ORA_NO_DATA_FOUND,
					rowssent[cont->getId(cursor)],
					ORA_NO_DATA_FOUND_MESSAGE);
	} else {
		putSummary(cursorid,0,rowssent[cont->getId(cursor)],NULL);
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

	uint32_t	maxrowsize=0;
	for (uint32_t i=0; i<colcount; i++) {
		maxrowsize+=getWireColumnSize(cursor,i,
			getWireColumnType(columntypes[cont->getId(cursor)][i]));
	}

	writeLenPreInt(&reqpacket,maxrowsize);
	writeLenPreInt(&reqpacket,colcount);
	if (colcount) {
		write(&reqpacket,(byte_t)0x51);
	}

	debugStart("describe info");
	debugWrite("max row size: %d",maxrowsize);
	debugWrite("column count: %d",colcount);

	for (uint32_t i=0; i<colcount; i++) {
		putColumnMetadata(cursor,i);
	}

	debugEnd();

	byte_t	date[7];
	putOracleDate(date);
	putBytesWithLength((const char *)date,sizeof(date));

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
	uint16_t	wiretype=getWireColumnType(columntypes[curid][column]);
	uint32_t	size=getWireColumnSize(cursor,column,wiretype);

	bool	character=(wiretype!=ORACLE_TYPE_NUMBER);

	const char	*name=cont->getColumnName(cursor,column);
	uint32_t	namesize=cont->getColumnNameSize(cursor,column);

	write(&reqpacket,(byte_t)wiretype);
	write(&reqpacket,(byte_t)((character)?0x80:0x00));

	// a real server reports scale -127 for a number with no declared
	// scale, which tells the client not to rescale the value
	putColumnPrecisionScale(0,(character)?0:-127);

	// a buffer size of 0 means "this column is null by describe", so it
	// can never be sent as 0 for a column that has values
	writeLenPreInt(&reqpacket,size);

	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,(character)?charset:0);
	write(&reqpacket,(byte_t)((character)?1:0));
	writeLenPreInt(&reqpacket,(character)?size:0);
	write(&reqpacket,(byte_t)1);
	write(&reqpacket,(byte_t)namesize);
	putBytesWithLength(name,namesize);
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
		default:
			// anything the module can't encode is described as a
			// varchar2 and sent as text - describing it as its own
			// type and sending text desyncs the client
			wiretype=ORACLE_TYPE_VARCHAR;
			break;
	}

	debugStart("wire column type");
	debugWrite("input type: 0x%02x",(uint32_t)(0x000000ff&columntype));
	debugColumnType(wiretype);
	debugEnd();

	return wiretype;
}

uint32_t sqlrprotocol_oracle::getWireColumnSize(sqlrservercursor *cursor,
						uint32_t column,
						uint16_t wiretype) {

	if (wiretype==ORACLE_TYPE_NUMBER) {
		debugStart("wire column size");
		debugWrite("column: %d",column);
		debugColumnType(wiretype);
		debugWrite("size: %d",MAX_NUMBER_SIZE);
		debugEnd();
		return MAX_NUMBER_SIZE;
	}

	uint32_t	size=cont->getColumnSize(cursor,column);
	if (!size || size>MAX_VARCHAR_SIZE) {
		size=MAX_VARCHAR_SIZE;
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
	writeLenPreInt(&reqpacket,colcount);
	writeLenPreInt(&reqpacket,0);
	writeLenPreInt(&reqpacket,prefetchrows);
	writeLenPreInt(&reqpacket,0);

	// no bit vector.  a real server sends one to say which columns repeat
	// the previous row's value; without one every column is sent in full.
	writeLenPreInt(&reqpacket,0);

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

		// FIXME: lobs are sent as null
		if (null || lob || !field) {
			debugWrite("null");
			write(&reqpacket,(byte_t)0);
		} else if (getWireColumnType(ct[i])==ORACLE_TYPE_NUMBER) {
			debugWrite("number: %.*s",(int)fieldsize,field);
			putNumberField(field,(uint32_t)fieldsize);
		} else {
			debugWrite("\"%.*s\"",(int)fieldsize,field);
			putLenBytes(field,(uint32_t)fieldsize);
		}

		debugEnd();
	}
}

void sqlrprotocol_oracle::putReturnParameters() {

	// nothing in this block is load bearing - a live 11.2 server puts 19
	// session state key/value pairs here and a live 12.2 server puts none
	write(&reqpacket,(byte_t)TTC_OK);

	writeLenPreInt(&reqpacket,AL8O4_COUNT);
	for (uint16_t i=0; i<AL8O4_COUNT; i++) {
		writeLenPreInt(&reqpacket,0);
	}

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
		putLenBytes(message,charstring::getLength(message));
	}

	debugStart("summary");
	debugWrite("cursor id: %d",cursorid);
	debugWrite("call number: %d",callnumber);
	debugWrite("row count: %d",rowcount);
	debugWrite("error: %d",oranum);
	if (oranum) {
		debugWrite("message: %s",message);
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

bool sqlrprotocol_oracle::execute(const byte_t *rp) {

	// sqlplus 8.0.5 (at least)
	// call this to execute a commit at the end of initialization

	// parse the request...
	uint16_t	options;
	uint16_t	moreoptions;
	uint16_t	cursorid;

	// FIXME: decode this...
	readBE(rp,&options,&rp);
	readBE(rp,&moreoptions,&rp);
	readBE(rp,&cursorid,&rp);
cursorid=hackcursorid;

	if (getDebug()) {
		debugStart("execute request");
		debugOptions(options,moreoptions);
		debugWrite("cursor id: %d",cursorid);
		debugEnd();
	}

	// get the requested cursor
	sqlrservercursor	*cursor=cont->getCursor(cursorid);
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError();
	}

	// execute the query
	if (!cont->executeQuery(cursor,true,true,true,true)) {
		debugWrite("execute query failed");
		return sendQueryError(cursor);
	}

	return sendExecuteResponse(cursor);
}

bool sqlrprotocol_oracle::sendExecuteResponse(sqlrservercursor *cursor) {

	resetSendPacketBuffer(PACKET_DATA);

	// FIXME: decode this...

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
		return sendCursorNotOpenError();
	}

	return sendFetch3Response(cursor,cursorid,rowstofetch);
}

bool sqlrprotocol_oracle::sendFetch3Response(sqlrservercursor *cursor,
						uint32_t cursorid,
						uint32_t rowstofetch) {

	// the same as an execute's answer minus the describe and the return
	// parameters: a row header, the rows, and a summary object
	resetSendPacketBuffer(PACKET_DATA);

	uint32_t	colcount=cont->colCount(cursor);

	uint16_t	dataflags=0;
	writeBE(&reqpacket,dataflags);

	uint32_t	rowsfetched=0;
	bool		endofrows=true;

	if (colcount && rowstofetch) {

		endofrows=false;

		while (rowsfetched<rowstofetch) {

			bool	error=false;
			if (!cont->fetchRow(cursor,&error)) {
				if (error) {
					// FIXME: handle error
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

			debugStart("fetch response row");
			putRowData(cursor,colcount);
			debugEnd();

			// FIXME: kludgy
			cont->nextRow(cursor);

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

	// all versions call this to fetch

	if (query3session) {
		return fetch3(rp);
	}

	// parse the request...
	uint16_t	options;
	uint16_t	moreoptions;
	uint16_t	cursorid;

	// FIXME: decode this...
	readBE(rp,&options,&rp);
	readBE(rp,&moreoptions,&rp);
cursorid=hackcursorid;

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
		return sendCursorNotOpenError();
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

	resetSendPacketBuffer(PACKET_DATA);

	uint32_t	colcount=cont->colCount(cursor);
	cacheColumnDefinitions(cursor,colcount);

	// FIXME: get this from the client somehow
	uint32_t rowstofetch=1;

	// for each row...
	uint32_t rowsfetched=0;
	do {

		// fetch a row
		bool	error;
		if (!cont->fetchRow(cursor,&error)) {
			if (error) {
				// FIXME: handle error
			}
			break;
		}

		// ok, so there is at least one row...
		// send various headers and column definitions
		if (!rowsfetched) {

			// FIXME: the headers/col-defs appear to be very
			// different when sent from 8i

			if (define) {

				uint16_t	dataflags=0;
				// FIXME: not a valid ttccode type...
				byte_t		ttccode=16;

				writeBE(&reqpacket,dataflags);
				write(&reqpacket,ttccode);

				if (getDebug()) {
					debugStart("fetch response header");
					debugWrite("data flags: 0x%04x",
								dataflags);
					debugTtcCode(ttccode);
					debugEnd();
				}
			}

			// send column definitions...
			if (define) {
				putColumnDefinitions(cursor,colcount);
			}

			// send "iov" (whatever that is)...
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

		// no idea...
		write(&reqpacket,(byte_t)7);

		debugStart("fetch response row");
		debugWrite("row marker: 0x07");

		putRow(cursor,colcount,exactfetch);

		debugEnd();

		// FIXME: kludgy
		cont->nextRow(cursor);

		rowsfetched++;

	} while (rowsfetched<rowstofetch);

	if (rowsfetched) {

		if (exactfetch) {
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
		debugWrite("%s: %d",cont->getColumnTypeName(cursor,i),ct[i]);
	}

	columntypescached[curid]=true;

	debugEnd();
}

void sqlrprotocol_oracle::putColumnDefinitions(sqlrservercursor *cursor,
							uint32_t colcount) {

	byte_t	sizetotal=0;
	for (uint32_t i=0; i<colcount; i++) {
		sizetotal+=cont->getColumnSize(cursor,i);
	}
	uint32_t	constant=51;

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
	/*uint16_t	columnflags=getColumnFlags(cursor,column,
							sqlrcolumntype,
							columntype,
							columntypestring);*/

	// no idea
	byte_t	marker1=1;
	// no idea - 128 for char/varchar, 00 for numeric
	byte_t	marker2=128;
	byte_t	precision=cont->getColumnPrecision(cursor,column);
	byte_t	scale=cont->getColumnScale(cursor,column);
	// 16 for non-integer decimal, otherwise actual size
	byte_t	size=cont->getColumnSize(cursor,column);
	byte_t	unknown1[]={
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00
	};
	// no idea - 1 for char/varchar, 0 for numeric
	uint16_t	marker3=1;
	uint16_t	nullable=(cont->getColumnIsNullable(cursor,column))?1:0;
	// 1 for select from table
	// 2 for select from table with alias
	// 3 for select from dual
	// 4 for select from dual with alias
	byte_t		alias=1;
	const char	*name=cont->getColumnName(cursor,column);
	uint32_t	namesize=cont->getColumnNameSize(cursor,column);
	// no idea
	uint32_t	marker4=0;
	// no idea
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

void sqlrprotocol_oracle::putRow(sqlrservercursor *cursor,
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
			// FIXME: handle error
		}

		// put the field
		if (lob) {
			debugWrite("LOB");
			putLobField(cursor,i);
		} else if (!null) {
			debugWrite("\"%s\" (%lld)",field,(long long)fieldsize);
			putField(field,fieldsize,ct[i]);

			// no idea
			if (terminator) {
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
}

void sqlrprotocol_oracle::putField(const char *field,
					uint64_t fieldsize,
					uint16_t columntype) {

	switch (columntype) {
		case ORACLE_TYPE_CHAR:
		case ORACLE_TYPE_VARCHAR:
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
			break;
		case ORACLE_TYPE_NUMBER:
			// FIXME: implement this
			debugWrite("number (not implemented)");
			break;
		case ORACLE_TYPE_VARNUM:
			// FIXME: implement this
			debugWrite("varnum (not implemented)");
			break;
		case ORACLE_TYPE_LONG:
			// FIXME: implement this
			debugWrite("long (not implemented)");
			break;
		case ORACLE_TYPE_ROWID_DEPRECATED:
			// FIXME: implement this
			debugWrite("rowid (deprecated) (not implemented)");
			break;
		case ORACLE_TYPE_DATE:
			// FIXME: implement this
			debugWrite("date (not implemented)");
			break;
		case ORACLE_TYPE_RAW:
			// FIXME: implement this
			debugWrite("raw (not implemented)");
			break;
		case ORACLE_TYPE_LONG_RAW:
			// FIXME: implement this
			debugWrite("long raw (not implemented)");
			break;
		case ORACLE_TYPE_RESULT_SET:
			// FIXME: implement this
			debugWrite("result set (not implemented)");
			break;
		case ORACLE_TYPE_ROWID:
			// FIXME: implement this
			debugWrite("rowid (not implemented)");
			break;
		case ORACLE_TYPE_NAMED_TYPE:
			// FIXME: implement this
			debugWrite("named type (not implemented)");
			break;
		case ORACLE_TYPE_REF_TYPE:
			// FIXME: implement this
			debugWrite("ref type (not implemented)");
			break;
		case ORACLE_TYPE_CLOB:
			// FIXME: implement this
			debugWrite("clob (not implemented)");
			break;
		case ORACLE_TYPE_BLOB:
			// FIXME: implement this
			debugWrite("blob (not implemented)");
			break;
		case ORACLE_TYPE_BFILE:
			// FIXME: implement this
			debugWrite("bfile (not implemented)");
			break;
		case ORACLE_TYPE_TIMESTAMP:
			// FIXME: implement this
			debugWrite("timestamp (not implemented)");
			break;
		case ORACLE_TYPE_TIMESTAMPTZ:
			// FIXME: implement this
			debugWrite("timestamp tz (not implemented)");
			break;
		case ORACLE_TYPE_INTERVALYM:
			// FIXME: implement this
			debugWrite("interval year-month (not implemented)");
			break;
		case ORACLE_TYPE_INTERVALDS:
			// FIXME: implement this
			debugWrite("interval day-second (not implemented)");
			break;
		case ORACLE_TYPE_TIMESTAMPLTZ:
			// FIXME: implement this
			debugWrite("timestamp ltz (not implemented)");
			break;
		case ORACLE_TYPE_PLSQL_INDEX_TABLE:
			// FIXME: implement this
			debugWrite("plsql index table (not implemented)");
			break;
		case ORACLE_TYPE_FIXED_CHAR:
			// FIXME: implement this
			debugWrite("fixed char (not implemented)");
			break;
		default:
			debugWrite("unknown column type: %d",columntype);
			break;
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

void sqlrprotocol_oracle::putError(const char *error) {
	putError(error,charstring::getLength(error));
}

void sqlrprotocol_oracle::putError(const char *error, uint32_t errorsize) {

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_ERROR;

	const byte_t	unknown1[]={
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

	const byte_t	unknown2[]={
		0x0A
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	reqpacket.append(unknown1,sizeof(unknown1));
	write(&reqpacket,(byte_t)errorsize);
	reqpacket.append(error,errorsize);
	reqpacket.append(unknown2,sizeof(unknown2));

	debugStart("error response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("error size: %u",errorsize);
	debugWrite("error: %*s",errorsize,error);
	debugEnd();
}

bool sqlrprotocol_oracle::close(const byte_t *rp) {

	uint16_t	cursorid;

	// FIXME: decode this...
cursorid=hackcursorid;

	debugStart("close request");
	debugWrite("cursor id: %d",cursorid);
	debugEnd();

	// get the requested cursor
	sqlrservercursor	*cursor=cont->getCursor(cursorid);
	if (!cursor) {
		debugWrite("cursor id %d not found",cursorid);
		return sendCursorNotOpenError();
	}

	clearParams(cursor);
	pcounts[cont->getId(cursor)]=0;
	cont->abort(cursor);
	cont->release(cursor);
hackcursorid=65535;

	return sendCloseResponse(cursor);
}

void sqlrprotocol_oracle::clearParams(sqlrservercursor *cursor) {

	uint16_t		pcount=cont->getInputBindCount(cursor);
	sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);

	for (uint16_t i=0; i<pcount; i++) {
		sqlrserverbindvar	*bv=&(inbinds[i]);
		if (bv->type==SQLRSERVERBINDVARTYPE_STRING ||
			bv->type==SQLRSERVERBINDVARTYPE_BLOB) {
			delete[] bv->value.stringval;
		}
	}
	cont->setInputBindCount(cursor,0);
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

bool sqlrprotocol_oracle::version(const byte_t *rp) {

	// FIXME: decode this...

	debugStart("version request");
	if (rp && rp<resppacket+resppacketsize) {
		debugWrite("requested version: 0x%02x",*rp);
	}
	debugEnd();

	return sendVersionResponse();
}

bool sqlrprotocol_oracle::sendVersionResponse() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;
	// FIXME: get this from the db
	const char	*serverversion=
			"Oracle8 Release 8.0.5.0.0 - Production\n"
			"PL/SQL Release 8.0.5.0.0 - Production";
	// the packed version number of the version the banner announces.  the
	// nibbles are the version: major in bits 31-24, minor in 23-20, the
	// third component in 15-12, the patchset in 11-8, and the fifth
	// component in 7-0.  the other bits are zero.
	uint32_t	packedversion=
			(((uint32_t)SERVERVERSION_MAJOR)<<24)|
			(((uint32_t)SERVERVERSION_MINOR)<<20)|
			(((uint32_t)SERVERVERSION_THIRD)<<12)|
			(((uint32_t)SERVERVERSION_PATCH)<<8)|
			((uint32_t)SERVERVERSION_FIFTH);

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	// a real capture sends the banner length as a length-preceded int - a
	// count byte and that many big-endian magnitude bytes (01 4c for 76) -
	// rather than as a bare ub2, so writeLenPreInt() is correct here, not
	// writeHost()/writeLE()/writeBE()
	writeLenPreInt(&reqpacket,(uint32_t)charstring::getLength(serverversion));
	write(&reqpacket,serverversion);
	writeBE(&reqpacket,packedversion);

	debugStart("version response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugWrite("server version:");
	debugWrite("%s",serverversion);
	debugWrite("packed version: 0x%08x",packedversion);
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
		clearParams(cursor);
		pcounts[cont->getId(cursor)]=0;
		cont->abort(cursor);
		cont->release(cursor);
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

bool sqlrprotocol_oracle::logonUnknown(const byte_t *rp) {

	// no idea

	// FIXME: decode this...

	debugStart("logon unknown request");
	if (rp && rp<resppacket+resppacketsize) {
		debugWrite("function code: 0x%02x",*rp);
	}
	debugEnd();

	return sendLogonUnknownResponse();
}

bool sqlrprotocol_oracle::sendLogonUnknownResponse() {

	resetSendPacketBuffer(PACKET_DATA);

	uint16_t	dataflags=0;
	byte_t		ttccode=TTC_OK;

	// no idea
	byte_t	unknown[]={
		0x0C,
		0x00, 0x00, 0x00, 0x67,
		0x70, 0x00,
		0x00, 0x00, 0x00, 0x09
	};

	writeBE(&reqpacket,dataflags);
	write(&reqpacket,ttccode);
	reqpacket.append(unknown,sizeof(unknown));

	debugStart("logon unknown response");
	debugWrite("data flags: 0x%04x",dataflags);
	debugTtcCode(ttccode);
	debugHexDump(unknown,sizeof(unknown));
	debugEnd();

	return sendPacket(true);
}


void sqlrprotocol_oracle::putGenericFooter() {

	// no idea...

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

	// FIXME: implement this;

	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(cursor,
			&errorstring,
			&errorsize,
			&errnum,
			&liveconnection);
	debugWrite("%*s",errorsize,errorstring);
	return false;
}

bool sqlrprotocol_oracle::sendCursorNotOpenError() {
	// FIXME: implement this;
	return false;
}

bool sqlrprotocol_oracle::sendNotImplementedError() {
	// FIXME: implement this;
	return false;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrprotocol	*new_sqlrprotocol_oracle(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrprotocol_oracle(cont,parameters);
	}
}
