// Copyright (c) 2016  David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/character.h>
#include <rudiments/wcharstring.h>
#include <rudiments/process.h>
#include <rudiments/sys.h>
#include <rudiments/datetime.h>
#include <rudiments/error.h>


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

// packet header size, and the smallest, default, and largest packet sizes,
// given that the size on the wire is 16 bits and includes the header
#define	PACKET_HEADER_SIZE		8
#define	MIN_PACKET_SIZE			512
#define	DEFAULT_PACKET_SIZE		4096
#define	MAX_PACKET_SIZE			65535

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

static byte_t	tdstypemap[]={
	// "UNKNOWN"
	(byte_t)TDS_TYPE_NULL,
	// addded by freetds
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
	// "SHORT_DATATYPE"
	(byte_t)TDS_TYPE_INTN,
	// "TINY_DATATYPE"
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
#define SP_UNPREPARE		15 

#define SP_MAX_PROCID		15

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

// Clients may send any of the procs above by name instead of by id.  Freetds
// always sends sp_execute by name, for example.  Indices match procids above.
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

// the return status that all of these procs use for success
#define RPC_STATUS_SUCCESS	0
#define RPC_STATUS_FAILURE	1

// sql server error numbers that these procs answer with, which a failed one
// also sends back as its return status
#define RPC_WRONG_PARAM_TYPE	214
#define RPC_NO_SUCH_STMT	8179
#define RPC_NO_SUCH_CURSOR	16950

// close-all cursor id for sp_cursorclose
#define CURSOR_CLOSE_ALL	0xFFFFFFFF


// TDS protocol class
class SQLRSERVER_DLLSPEC sqlrprotocol_tds : public sqlrprotocol {
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

		bool	recvPacket(byte_t *packettype);
		bool	sendPacket();

		wchar_t	*readPassword(const byte_t *rp,
					size_t charcount);

		void		getServerTdsVersion();
		uint32_t	tdsVersionHexToDec(uint32_t tdsversion);
		uint32_t	tdsVersionDecToHex(uint32_t tdsversion,
								bool client);
		void		negotiateTdsVersion();

		bool	preLogin();
		// whether optsize bytes at ploptoff bytes from the start
		// of the packet are still inside a packetsize-byte packet
		bool	preLoginOptionFits(uint16_t ploptoff,
						size_t optsize,
						size_t packetsize);

		bool	preTds7Login();

		bool	tds7Login();
		bool	auth(const wchar_t *username,
						size_t usernamelen,
						const wchar_t *password,
						size_t passwordlen);
		void	loginAck();
		void	authError(const wchar_t *username,
						size_t usernamelen);
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
		void	allHeaders(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout);
		void	colMetaData(sqlrservercursor *cursor, bool nometadata);
		void	cekTable();
		byte_t	mapType(uint16_t type);
		void	colData(sqlrservercursor *cursor, uint16_t col);
		void	userType(byte_t tdstype);
		void	colFlags(sqlrservercursor *cursor,
					uint16_t col,
					byte_t tdstype);
		void	typeInfo(sqlrservercursor *cursor,
					uint16_t col,
					byte_t tdstype);
		void	tableName(byte_t tdstype);
		void	cryptoMetaData();
		void	colName(sqlrservercursor *cursor, uint16_t col);
		bool	isCaseSensitiveType(byte_t tdstype);
		bool	isFixedLenType(byte_t tdstype);
		bool	isVarLenType(byte_t tdstype);
		bool	isPartLenType(byte_t tdstype);
		// Maps a backend-reported column size onto one of the widths
		// that "n" types (intn, bitn, fltn, moneyn, datetimn) are
		// allowed to use.  Backends don't agree about what column
		// size means for those types.  FreeTDS reports the storage
		// width in bytes, but ODBC reports SQL_DESC_LENGTH, which is
		// a digit count.
		byte_t	nTypeSize(byte_t tdstype, uint32_t colsize);
		uint64_t	rows(sqlrservercursor *cursor);
		uint64_t	rows(sqlrservercursor *cursor,
					uint64_t maxrows);
		void	lobData(byte_t tdstype);
		void	field(byte_t tdstype,
					uint32_t colsize,
					uint32_t colscale,
					const char *field,
					uint64_t fieldsize,
					bool null);
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
		void	guid(const char *field, byte_t *g);
		byte_t	charsToHex(const char *chars);

		// Answers the "insert bulk" statement that opens a bulk
		// load, rather than passing it to the backend, and returns
		// false if the statement isn't one.
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
		bool	isBinaryType(byte_t tdstype);
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
		uint16_t	procNameToProcId(const char *procname);
		bool	params(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout);
		bool	param(uint16_t param,
					const byte_t *rp,
					const byte_t **rpout,
					bool exceeded);
		void	batchFlags(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout,
					bool *more);

		// rpc parameter accessors - "param" indexes the parameters
		// as they arrived on the wire
		bool		paramIsNull(uint16_t param);
		int64_t		paramInteger(uint16_t param);
		const char	*paramString(uint16_t param);

		// binds params [first..rpcparamcount) to the cursor
		void	bindParams(sqlrservercursor *cursor, uint16_t first,
						bool returnvalue=false);

		// rpc handlers
		bool	namedProc(const char *procname, bool nometadata);
		bool	executeSql(bool nometadata);
		bool	prepare(bool prepexec, bool rpcsyntax, bool nometadata);
		bool	execute(bool nometadata);
		bool	unprepare();
		bool	cursorOpen(bool nometadata);
		bool	cursorPrepare();
		bool	cursorExecute(bool nometadata);
		bool	cursorPrepExec(bool nometadata);
		bool	cursorUnprepare();
		bool	cursorFetch(bool nometadata);
		bool	cursorOption();
		bool	cursorClose();
		bool	cursorUnsupported();

		// rpc response builders
		void	rpcResultSet(sqlrservercursor *cursor,
					bool nometadata,
					uint64_t maxrows);
		void	rpcError(sqlrservercursor *cursor,
					bool returnstatus=true);
		bool	rpcInvalidHandleError(uint32_t number,
					const char *what,
					uint32_t handle);
		bool	rpcParamTypeError(const char *procname,
					const char *param);
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

		// converts odbc {call p(?,?)} syntax to exec p ?,?
		char	*callSyntaxToExec(const char *stmt);

		void	envChange(byte_t type,
					const wchar_t *newvalue,
					size_t newvaluelen,
					const wchar_t *oldvalue,
					size_t oldvaluelen);

		// info/error token builders - these only append to the
		// response packet, the caller decides when to send it
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
		bool	sendTdsProtocolError();
		bool	sendQueryTooLargeError(size_t querysize);
		bool	sendNoCursorAvailableError();

		void	done();
		void	done(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount);
		void	done(byte_t token,
					uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount);
		void	doneInProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount);
		void	returnStatus(uint32_t value);
		uint32_t	procReturnValue(sqlrservercursor *cursor);
		void	returnValues(sqlrservercursor *cursor);
		void	returnValue(sqlrservercursor *cursor,
					uint16_t param,
					uint16_t ordinal);
		void	returnValueInteger(uint16_t ordinal,
					int32_t value,
					bool isnull);
		void	returnValueHeader(uint16_t ordinal,
						const char *name,
						uint16_t namesize);
		void	writeIntN(int64_t value, byte_t size);
		void	doneProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount);

		void	debugSystemError();

		filedescriptor	*clientsock;

		uint32_t	configtdsversion;
		uint32_t	configpacketsize;
		uint32_t	configmaxpacketsize;

		uint32_t	maxquerysize;
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

		uint32_t	oldpacketsize;
		uint32_t	negotiatedpacketsize;

		bool		dbistds;

		// rpc parameters, as they arrived on the wire.  The handlers
		// decide which are the proc's own arguments and which are
		// bind values for the statement it carries.
		memorypool		rpcparampool;
		sqlrserverbindvar	*rpcparams;
		bool			*rpcparambyref;
		char			**rpcparamnames;
		uint16_t		*rpcparamnamesizes;
		byte_t			*rpcparamtdstypes;
		uint32_t		*rpcparammaxsizes;
		uint16_t		*outbindparams;
		uint16_t		rpcparamcount;

		// whether the rpc currently being handled failed, so that
		// the done that closes it can say so
		bool			rpcfailed;

		// The table and columns that the "insert bulk" statement
		// named, and the column metadata that the bulk load packet
		// itself carries.  The two arrive in separate requests, so
		// they have to be kept between them.
		memorypool		bulkpool;
		char			*bulktable;
		char			**bulkcolumns;
		uint16_t		bulkcolumncount;
		byte_t			*bulktypes;
		uint32_t		*bulksizes;
		byte_t			*bulkscales;

		// prepared statement and cursor handles.  These are handed
		// out independently of each other and of the cursor id,
		// because a client can hold a prepared handle and a cursor
		// derived from it at the same time.  Handle 0 is invalid.
		dictionary<uint32_t, sqlrservercursor *>	stmthandles;
		dictionary<uint32_t, sqlrservercursor *>	cursorhandles;
		uint32_t					nexthandle;

		// false once a cursor has been executed, so that fetching
		// more rows doesn't run the query again
		dictionary<sqlrservercursor *, bool>		executeflag;
};

sqlrprotocol_tds::sqlrprotocol_tds(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrprotocol(cont,parameters) {

	clientsock=NULL;

	// the version that getServerTdsVersion() guesses from the backend's
	// version string can be overridden, for backends that it can't
	// work out on its own
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

	// tls="yes" would otherwise be accepted and silently ignored
	if (useTls()) {
		stderror.printf("Warning: TLS support requested but the tds "
				"protocol module doesn't support TLS\n");
	}

	const char	*dbtype=cont->getDbType();
	dbistds=(!charstring::compare(dbtype,"freetds") ||
			!charstring::compare(dbtype,"sap"));

	maxquerysize=cont->getConfig()->getMaxQuerySize();
	maxbindcount=cont->getConfig()->getMaxBindCount();

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
	rpcparammaxsizes=new uint32_t[maxbindcount];
	outbindparams=new uint16_t[maxbindcount];
	rpcparamcount=0;

	bulkcolumns=new char *[maxbindcount];
	bulktypes=new byte_t[maxbindcount];
	bulksizes=new uint32_t[maxbindcount];
	bulkscales=new byte_t[maxbindcount];

	init();
}

sqlrprotocol_tds::~sqlrprotocol_tds() {
	free();

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
	delete[] rpcparammaxsizes;
	delete[] outbindparams;

	delete[] bulkcolumns;
	delete[] bulktypes;
	delete[] bulksizes;
	delete[] bulkscales;
}

void sqlrprotocol_tds::init() {

	packetid=0;

	// the backend can change between sessions, so these have to be
	// re-read rather than cached once in the constructor
	srvname=cont->getDbHostName();
	dbversion=cont->getDbVersion();
	getServerTdsVersion();

	clienttdsversion=700;
	negotiatedtdsversion=700;

	oldpacketsize=configpacketsize;
	negotiatedpacketsize=configpacketsize;

	// handle 0 is invalid
	nexthandle=1;

	rpcparamcount=0;
	rpcfailed=false;

	bulktable=NULL;
	bulkcolumncount=0;
}

void sqlrprotocol_tds::free() {
	reqpacketpool.clear();
	reqpacket.clear();
	resppacket.clear();

	rpcparampool.clear();

	bulkpool.clear();
	bulktable=NULL;
	bulkcolumncount=0;

	// the session's cursors get released with the session, so these
	// just have to forget about them
	stmthandles.clear();
	cursorhandles.clear();
	executeflag.clear();
}

void sqlrprotocol_tds::reInit() {
	free();
	init();
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

		// sanity checks
		if (*packettype!=SQL_BATCH &&
			*packettype!=PRE_TDS7_LOGIN &&
			*packettype!=RPC &&
			*packettype!=TABULAR_RESULT &&
			*packettype!=ATTENTION_SIGNAL &&
			*packettype!=BULK_LOAD_DATA &&
			*packettype!=FEDERATED_AUTHENTICATION_TOKEN &&
			*packettype!=TRANSACTION_MANAGER_REQUEST &&
			*packettype!=TDS7_LOGIN &&
			*packettype!=SSPI &&
			*packettype!=PRE_LOGIN) {
			debugWrite("invalid packet type: 0x%02x",*packettype);
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

		// get the packet data
		// (read into a reused memorypool - it's fast, minimizes
		// heap fragmentation, and the data is copied into
		// reqpacket immediately below)
		reqpacketpool.clear();
		byte_t	*packet=reqpacketpool.allocate(packetsize);
		if (clientsock->read(packet,packetsize)!=packetsize) {
			debugWrite("read packet failed");
			debugSystemError();
			return false;
		}

		// append the data to the receive buffer
		reqpacket.append(packet,packetsize);

		debugStart("recv");
		debugWrite("packet type: 0x%02x",*packettype);
		debugWrite("packet status: 0x%02x",packetstatus);
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

bool sqlrprotocol_tds::sendPacket() {

	const byte_t	*packet=resppacket.getBuffer();
	uint64_t	remaining=resppacket.getSize();

	// the negotiated packet size includes the header
	uint32_t	maxdatasize=
			(negotiatedpacketsize>PACKET_HEADER_SIZE)?
				negotiatedpacketsize-PACKET_HEADER_SIZE:
				MIN_PACKET_SIZE-PACKET_HEADER_SIZE;

	do {

		// set header parts
		byte_t		packettype=TABULAR_RESULT;
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
		debugWrite("packet type: 0x%02x",packettype);
		debugWrite("packet status: 0x%02x",packetstatus);
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

	return true;
}

wchar_t *sqlrprotocol_tds::readPassword(const byte_t *rp,
						size_t charcount) {
	uint16_t	size=charcount*sizeof(uint16_t);
	byte_t		*temp=(byte_t *)bytestring::duplicate(rp,size);
	byte_t		*ch=temp;
	for (uint16_t i=0; i<size; i++) {
		*ch=*ch^0xA5;
		*ch=((*ch&0x0F)<<4)|((*ch&0xF0)>>4);
		ch++;
	}
	wchar_t	*password=wcharstring::duplicateUcs2((const ucs2_t *)temp,
								charcount);
	delete[] temp;
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
	switch (tdsversion) {
		case 0x00000042:
		case 0x42000000:
			// Sybase < 10
			// SQL Server 6.x
			return 420;
		case 0x00000050:
		case 0x05000000:
			// Sybase 10+
			// Sybase SQL Anywhere (all versions)
			return 500;
		case 0x00000070:
		case 0x07000000:
			// SQL Server 7.0
			return 700;
		case 0x00000071:
		case 0x07010000:
			// SQL Server 2000
			return 710;
		case 0x01000071:
		case 0x71000001:
			// SQL Server 2000 SP1
			return 711;
		case 0x02000972:
		case 0x72090002:
			// SQL Server 2005
			return 720;
		case 0x03000A73:
		case 0x730A0003:
			// SQL Server 2008
			return 730;
		case 0x03000B73:
		case 0x730B0003:
			// SQL Server 2008 R2
			return 731;
		case 0x04000074:
		case 0x74000004:
			// SQL Server 2012, 2014, 2106
			return 740;
		default:
			return 700;
	}
}

uint32_t sqlrprotocol_tds::tdsVersionDecToHex(uint32_t tdsversion,
							bool toclient) {
	if (toclient) {
		switch (tdsversion) {
			case 420:
				// Sybase < 10
				// SQL Server 6.x
				return 0x42000000;
			case 500:
				// Sybase 10+
				// Sybase SQL Anywhere (all versions)
				return 0x05000000;
			case 700:
				// SQL Server 7.0
				return 0x07000000;
			case 710:
				// SQL Server 2000
				return 0x07010000;
			case 711:
				// SQL Server 2000 SP1
				return 0x71000001;
			case 720:
				// SQL Server 2005
				return 0x72090002;
			case 730:
				// SQL Server 2008
				return 0x730A0003;
			case 731:
				// SQL Server 2008 R2
				return 0x730B0003;
			case 740:
				// SQL Server 2012, 2014, 2106
				return 0x74000004;
			default:
				return 0x07000000;
		}
	} else {
		switch (tdsversion) {
			case 420:
				// Sybase < 10
				// SQL Server 6.x
				return 0x00000042;
			case 500:
				// Sybase 10+
				// Sybase SQL Anywhere (all versions)
				return 0x00000050;
			case 700:
				// SQL Server 7.0
				return 0x00000070;
			case 710:
				// SQL Server 2000
				return 0x00000071;
			case 711:
				// SQL Server 2000 SP1
				return 0x01000071;
			case 720:
				// SQL Server 2005
				return 0x02000972;
			case 730:
				// SQL Server 2008
				return 0x03000A73;
			case 731:
				// SQL Server 2008 R2
				return 0x03000B73;
			case 740:
				// SQL Server 2012, 2014, 2106
				return 0x04000074;
			default:
				return 0x00000070;
		}
	}
}

void sqlrprotocol_tds::negotiateTdsVersion() {

	// If we couldn't work out the backend's tds version then go with
	// whatever the client asked for.  Falling back to an old version
	// hangs the client, because the width of some fields varies with the
	// version, and clients don't necessarily reconsider the version they
	// asked for.  FreeTDS, for example, sizes the done token's rowcount
	// from the version it requested, not from the one we send back.
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

		// some requests don't need a cursor...
		bool	loopback=false;
		switch (packettype) {
			case PRE_LOGIN:
				loop=preLogin();
				loopback=true;
				break;
			case PRE_TDS7_LOGIN:
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

	} while (loop);

	// close the client connection
	cont->closeClientConnection(0);

	// end the session if necessary
	if (endsession) {
		cont->endSession();
	}

	// return the status
	return status;
}

bool sqlrprotocol_tds::preLogin() {

	uint32_t	version=0;
	uint16_t	subbuild=0;
	byte_t		encryption=0;
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

	// some useful variables
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
		debugWrite("token: 0x%02x",plopttok);
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
		if (!preLoginOptionFits(ploptoff,ploptsize,packetsize)) {
			debugWrite("option data lies outside of the packet");
			badpacket=true;
			break;
		}

		// The cases below read a fixed size, whatever ploptsize
		// says, so each one needs its own bound.  Only PL_INSTOPT
		// reads exactly ploptsize, which the check above covers.

		// get the option data
		const byte_t		*dummy;
		switch (plopttok) {

			case PL_VERSION:
				// FIXME: bail if this isn't the first option
				if (!preLoginOptionFits(ploptoff,
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
				if (!preLoginOptionFits(ploptoff,
						sizeof(encryption),packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,&encryption,&dummy);
				debugWrite("pl_encryption");
				debugWrite("encryption:	0x%02x",encryption);
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
				if (!preLoginOptionFits(ploptoff,
						sizeof(threadid),packetsize)) {
					badpacket=true;
					break;
				}
				readLE(startrp+ploptoff,&threadid,&dummy);
				debugWrite("pl_threadid");
				debugWrite("threadid: %d",threadid);
				break;

			case PL_MARS:
				if (!preLoginOptionFits(ploptoff,
						sizeof(mars),packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,&mars,&dummy);
				debugWrite("mars");
				debugWrite("mars: %d",mars);
				break;

			case PL_TRACEID:
				if (!preLoginOptionFits(ploptoff,
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
						sizeof(connid),connid);
				debugWrite("activityid: %.*s",
						sizeof(activityid),activityid);
				break;

			case PL_FEDAUTHREQUIRED:
				if (!preLoginOptionFits(ploptoff,
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
				if (!preLoginOptionFits(ploptoff,
						sizeof(nonce),packetsize)) {
					badpacket=true;
					break;
				}
				read(startrp+ploptoff,
						nonce,sizeof(nonce),
						&dummy);
				debugWrite("nonceopt");
				debugWrite("nonce: %.*s",sizeof(nonce),nonce);
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

	// Ha!  You have to know ahead of time how many tokens you plan on
	// sending to set this correctly.
	// Update this accordingly if you add tokens!!!
	ploptoff=5*(sizeof(byte_t)+
				sizeof(uint16_t)+
				sizeof(uint16_t))+
			sizeof(byte_t);

	// respond in the same format as the request...
	// (the option offsets and sizes are big-endian, the option
	// data itself is little-endian)

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
	// FIXME: implement encryption
	// MS-TDS tunnels the TLS handshake inside TDS packets until the
	// handshake completes, so the server has to wrap and unwrap TLS
	// records between the TLS engine and the socket.  rudiments binds
	// OpenSSL straight to the file descriptor, leaving nowhere to put
	// that layer, so ENCRYPT_NOT_SUP is the only answer available.
	encryption=ENCRYPT_NOT_SUP;
	write(&packetdata,encryption);
	debugWrite("pl_encryption");
	debugWrite("encryption: 0x%02x",encryption);

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

	// append the packet data to the resppacket
	write(&resppacket,packetdata.getBuffer(),packetdata.getSize());

	// send the response packet
	bool	retval=sendPacket();

	debugEnd();

	// clean up
	delete[] instvalidity;

	return retval;
}

bool sqlrprotocol_tds::preLoginOptionFits(uint16_t ploptoff,
						size_t optsize,
						size_t packetsize) {
	return ((size_t)ploptoff+optsize<=packetsize);
}

bool sqlrprotocol_tds::preTds7Login() {

	debugStart("pre-tds7 login");
	debugEnd();

	// FIXME: actually implement this

	sendUnimplementedFeatureError();
	return false;
}

bool sqlrprotocol_tds::tds7Login() {

	const byte_t	*rp=reqpacket.getBuffer();
	const byte_t	*startrp=rp;

	// initialize values...
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

	char		fbyteorder=0;
	char		fcharset=0;
	uint16_t	ffloattype=0;
	char		fdumpload=0;
	char		fusedbwarn=0;
	char		fusedbfatal=0;
	char		fsetlangwarn=0;

	char		fsetlangfatal=0;
	char		fodbc=0;
	bool		ftranboundary=false;
	bool		fcachecontent=false;
	uint32_t	fusertype=0;
	char		fintsecurity=0;

	uint32_t	fsqltype=SQL_DFLT;
	char		foledb=0;
	bool		freadonlyintent=false;

	bool		fchangepassword=false;
	bool		fuserinstance=false;
	bool		fsendyukonbinaryxml=false;
	bool		funknowncollationhandling=false;
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

	// copy values out of the recv packet...
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

	// set option/type flags...
	fbyteorder=(optionflags1&(0x01));
	fcharset=(optionflags1&(0x01<<1))>>1;
	ffloattype=(optionflags1&(0x03<<2))>>2;
	fdumpload=(optionflags1&(0x01<<3))>>3;
	fusedbwarn=(optionflags1&(0x01<<4))>>4;
	fusedbfatal=(optionflags1&(0x01<<5))>>5;
	fsetlangwarn=(optionflags1&(0x01<<6))>>6;

	fsetlangfatal=(optionflags2&(0x01));
	fodbc=(optionflags2&(0x01<<1))>>1;
	ftranboundary=(optionflags2&(0x01<<2))>>2;
	fcachecontent=(optionflags2&(0x01<<3))>>3;
	fusertype=(optionflags2&(0x07<<4))>>4;
	fintsecurity=(optionflags2&(0x01<<5))>>5;

	fsqltype=(typeflags&(0x0F));
	foledb=(typeflags&(0x01<<4))>>4;
	freadonlyintent=(typeflags&(0x01<<3))>>3;

	fchangepassword=(optionflags3&(0x01));
	fuserinstance=(optionflags3&(0x01<<2))>>2;
	fsendyukonbinaryxml=(optionflags3&(0x01<<3))>>3;
	funknowncollationhandling=(optionflags3&(0x01<<4))>>4;
	fextension=(optionflags3&(0x01<<5))>>5;

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
		readLE(rp,&ibchangepassword,&rp);
		readLE(rp,&cchchangepassword,&rp);
		if (!fchangepassword) {
			ibchangepassword=0;
			cchchangepassword=0;
		}
		readLE(rp,&cbsspilong,&rp);
	}
	if (cchhostname<=128) {
		hostname=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibhostname),
					(size_t)cchhostname);
	}
	if (cchusername<=128) {
		username=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibusername),
					(size_t)cchusername);
	}
	if (cchpassword<=128) {
		password=readPassword(startrp+ibpassword,cchpassword);
	}
	if (cchappname<=128) {
		appname=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibappname),
					(size_t)cchappname);
	}
	if (cchservername<=128) {
		servername=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibservername),
					(size_t)cchservername);
	}
	if (clienttdsversion>=740 && cbextension<=255) {
		extension=(byte_t *)bytestring::duplicate(
						startrp+ibextension,
						cbextension);
		// FIXME: decode this...
	}
	if (cchcltintname<=128) {
		cltintname=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibcltintname),
					(size_t)cchcltintname);
	}
	if (cchlanguage<=128) {
		language=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+iblanguage),
					(size_t)cchlanguage);
	}
	if (cchdatabase<=128) {
		database=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibdatabase),
					(size_t)cchdatabase);
	}
	if (cchatchdbfile<=260) {
		atchdbfile=wcharstring::duplicateUcs2(
					(ucs2_t *)(startrp+ibatchdbfile),
					(size_t)cchatchdbfile);
	}
	if (clienttdsversion>=720 && cchchangepassword<=128) {
		changepassword=readPassword(startrp+ibchangepassword,
							cchchangepassword);
	}
	uint32_t	sspisize=0;
	if (cbsspi<65535) {
		sspisize=cbsspi;
	} else {
		if (cbsspilong==0) {
			sspisize=65535;
		} else {
			sspisize=cbsspilong;
		}
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
		stringbuffer	b;
		b.printBits(optionflags1);
		debugWrite("optionflags1: %s",b.getString());
		debugWrite("fbyteorder: %d",fbyteorder);
		debugWrite("fcharset: %d",fcharset);
		debugWrite("ffloattype: %d",ffloattype);
		debugWrite("fdumpload: %d",fdumpload);
		debugWrite("fusedbwarn: %d",fusedbwarn);
		debugWrite("fusedbfatal: %d",fusedbfatal);
		debugWrite("fsetlangwarn: %d",fsetlangwarn);
		b.clear();
		b.printBits(optionflags2);
		debugWrite("optionflags2: %s",b.getString());
		debugWrite("fsetlangfatal: %d",fsetlangfatal);
		debugWrite("fodbc: %d",fodbc);
		debugWrite("ftranboundary: %d",ftranboundary);
		debugWrite("fcachecontent: %d",fcachecontent);
		debugWrite("fusertype: %d",fusertype);
		debugWrite("fintsecurity: %d",fintsecurity);
		b.clear();
		b.printBits(typeflags);
		debugWrite("typeflags: %s",b.getString());
		debugWrite("fsqltype: %d",fsqltype);
		debugWrite("foledb: %d",foledb);
		debugWrite("freadonlyintent: %d",freadonlyintent);
		b.clear();
		b.printBits(optionflags3);
		debugWrite("optionflags3: %s",b.getString());
		debugWrite("fchangepassword: %d",fchangepassword);
		debugWrite("fuserinstance: %d",fuserinstance);
		debugWrite("fsendyukonbinaryxml: %d",fsendyukonbinaryxml);
		debugWrite("funknowncollationhandling: %d",
					funknowncollationhandling);
		debugWrite("fextension: %d",fextension);
		debugWrite("clienttimzone: %d",clienttimzone);
		b.clear();
		b.printBits(clientlcid);
		debugWrite("clientlcid: %s",b.getString());
		debugWrite("hostname: (%hd,%hd) %S",
					ibhostname,cchhostname,hostname);
		debugWrite("username: (%hd,%hd) %S",
					ibusername,cchusername,username);
		debugWrite("password: (%hd,%hd) %S",
					ibpassword,cchpassword,password);
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
		debugWrite("changepassword: (%hd,%hd) %S",
					ibchangepassword,cchchangepassword,
					changepassword);
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

	// auth the user...
	if (retval) {
		if (auth(username,cchusername,password,cchpassword)) {
			loginAck();
		} else {
			authError(username,cchusername);
			retval=false;
		}
	}

	// change database...
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

	// change collation...
	if (retval) {
		if (changeCollation(clientlcid)) {
			envChangeSqlCollation(clientlcid,0);
		}
	}

	// change language...
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

	char	*username8=charstring::duplicate(username,usernamelen);
	char	*password8=charstring::duplicate(password,passwordlen);

	sqlruserpasswordcredentials	cred;
	cred.setUser(username8);
	cred.setPassword(password8);

	bool	authsuccess=cont->auth(&cred);

	debugStart("authenticate");
	debugWrite("username: %s",username8);
	debugWrite("password: %s",password8);
	debugWrite((authsuccess)?"success":"failed");
	debugEnd();

	delete[] username8;
	delete[] password8;

	return authsuccess;
}

void sqlrprotocol_tds::loginAck() {

	byte_t		token=TOKEN_LOGIN_ACK;
					
	byte_t		iface=SQL_TSQL;
	// unlike the version in the login request, the version in the
	// login ack is sent big-endian
	uint32_t	tdsversion=
			tdsVersionDecToHex(negotiatedtdsversion,true);
	const char	*progname=dbversion;
	byte_t		prognamelength=(byte_t)charstring::getLength(progname);
	ucs2_t		*progname16=ucs2charstring::duplicate(progname,
							(size_t)prognamelength);
	byte_t		majorver=0;
	byte_t		minorver=0;
	byte_t		buildnumhi=0;
	byte_t		buildnumlow=0;

	uint16_t	tokensize=sizeof(byte_t)+
					sizeof(uint32_t)+
					sizeof(byte_t)+
					prognamelength*sizeof(ucs2_t)+
					sizeof(byte_t)+
					sizeof(byte_t)+
					sizeof(byte_t)+
					sizeof(byte_t);
	
	debugStart("login ack");
	debugWrite("token: 0x%02x",token);
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
	writeLE(&resppacket,tokensize);
	write(&resppacket,iface);
	writeBE(&resppacket,tdsversion);
	write(&resppacket,prognamelength);
	write(&resppacket,progname16,prognamelength);
	write(&resppacket,majorver);
	write(&resppacket,minorver);
	write(&resppacket,buildnumhi);
	write(&resppacket,buildnumlow);
}

void sqlrprotocol_tds::authError(const wchar_t *username,
					size_t usernamelen) {

	char	*username8=charstring::duplicate(username,usernamelen);

	stringbuffer	err;
	err.append("Login failed for user '");
	err.append(username8);
	err.append("'.");

	appendError(18456,1,14,err.getString(),srvname,NULL,0);

	delete[] username8;
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

	// FIXME: actually implement this

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
		debugWrite("token: 0x%02x",token);
		debugWrite("tokensize: 0x%02x (%hd)",tokensize,tokensize);
		debugWrite("type: %d",type);
		debugWrite("newvaluesize: %d",sizeof(uint32_t)+sizeof(byte_t));
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

	// The query has already run by the time the cancel arrives, so
	// there's nothing to interrupt.  MS-TDS 2.2.1.6 says to acknowledge
	// it with a done that has the attention bit set, either way.

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

bool sqlrprotocol_tds::sqlBatch() {

	// FIXME: this works for DML/DDL, but not for select,
	// ct_results() returns CS_FAIL

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
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
		debugWrite("query too large: %lld",(uint64_t)sqllength);
		debugEnd();
		cont->release(cursor);
		return sendQueryTooLargeError(sqllength);
	}

	// FIXME: Ideally we could just send the unconverted query, as long
	// as we also send the proper size in bytes.  SQL Relay really
	// appears to want ascii queries though, or at least it wants the
	// query itself (other than embedded values) to be acsii.
	char	*sql8=charstring::duplicateUcs2(sql,sqllength);

	debugWrite("sql: %s",sql8);
	debugWrite("sqllength: %lld",(uint64_t)sqllength);
	debugEnd();

	// A bulk load opens with an "insert bulk" statement.  Only the
	// protocol module can answer that - passing it through would put
	// the backend's own connection into bulk mode.
	if (insertBulk(sql8)) {

		delete[] sql8;
		cont->release(cursor);

		resppacket.clear();

		if (!bulktable) {
			appendError(0,1,16,"Malformed insert bulk statement",
							srvname,NULL,1);
			done(DONE_ERROR,0,0);
			return sendPacket();
		}

		// The client won't leave its pending state, and so won't
		// start sending bulk data, unless this done clears
		// DONE_MORE.
		done();
		return sendPacket();
	}

	// A batch has no bind variables, and the cursor may have been left
	// with some by an rpc that used it earlier.  Nothing in the batch
	// that looks like a bind variable is one either.  @name is a local
	// variable or a parameter declaration, and @@name is a global.
	cont->setInputBindCount(cursor,0);
	cont->setOutputBindCount(cursor,0);
	cont->setTranslateBindVariablesForThisQuery(cursor,false);

	// run the query
	bool	success=
		cont->prepareQuery(cursor,sql8,(uint32_t)sqllength,
						true,true,true,true) &&
		cont->executeQuery(cursor,true,true,true,true);

	// clean up
	delete[] sql8;


	// begin building the response packet
	resppacket.clear();

	if (success) {
		colMetaData(cursor,false);
		done(DONE_FINAL|DONE_COUNT,0,rows(cursor));
	} else {
		appendQueryError(cursor);
		// FIXME: this ought to be DONE_ERROR, but the ct-lib
		// test asserts CS_CMD_SUCCEED for a failed statement
		done();
	}

	// send the response packet
	bool	retval=sendPacket();

	// release the cursor
	// FIXME: kludgy
	cont->release(cursor);

	return retval;
}

void sqlrprotocol_tds::allHeaders(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout) {

	// skip the headers entirely if the packet is too short to hold
	// even the size of them
	uint32_t	allheaderssize;
	if (rpsize<sizeof(allheaderssize)) {
		debugWrite("truncated all-headers size");
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

		// The clamp above keeps allheaderssize no larger than
		// rpsize, and both drop by the same amount each pass, so
		// this also keeps the two reads below inside the packet.
		if (allheaderssize<sizeof(headersize)+sizeof(headertype)) {
			debugWrite("truncated header");
			break;
		}

		readLE(rp,&headersize,&rp);
		readLE(rp,&headertype,&rp);

		debugWrite("header size: %d",headersize);
		debugWrite("header type: 0x%04x",headertype);

		// bail on a bogus header size, otherwise we'd loop forever
		if (headersize<sizeof(headersize)+sizeof(headertype) ||
					headersize>allheaderssize) {
			debugWrite("invalid header size: %d",headersize);
			// rpsize is only decremented at the bottom of the
			// loop, so put rp back where this header started
			// rather than leaving it 6 bytes ahead of the size
			// we hand back
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
}

void sqlrprotocol_tds::colMetaData(sqlrservercursor *cursor, bool nometadata) {

	// get col count and bail if there are no columns
	uint16_t	count=cont->colCount(cursor);
	if (!count) {
		return;
	}

	byte_t	token=TOKEN_COLMETADATA;

	write(&resppacket,token);
	writeLE(&resppacket,count);

	debugStart("col meta data");
	debugWrite("token: 0x%02x",token);
	debugWrite("count: %d",count);

	cekTable();

	if (nometadata) {
		writeLE(&resppacket,(uint16_t)0xFFFF);
		debugWrite("no metadata");
	} else {
		for (uint16_t col=0; col<count; col++) {
			colData(cursor,col);
		}
	}

	debugEnd();
}

void sqlrprotocol_tds::cekTable() {

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

	// bail on a type that the map doesn't cover
	if (type>=sizeof(tdstypemap)/sizeof(tdstypemap[0])) {
		debugWrite("invalid column type: %hd",type);
		return TDS_TYPE_NULL;
	}

	byte_t	tdstype=tdstypemap[type];
	if (negotiatedtdsversion<730) {
		switch (tdstype) {
			case TDS_TYPE_DATEN:
			case TDS_TYPE_TIMEN:
			case TDS_TYPE_DATETIME2N:
			case TDS_TYPE_DATETIMEOFFSETN:
				// These four were introduced in TDS 7.3.  A real
				// sql server doesn't downgrade them to an older
				// date/time type for an older client - it
				// converts them to strings server-side and
				// sends nvarchar in the iso/odbc rendering.
				// The backends hand us that same rendering, so
				// just pass it through.
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
	return tdstype;
}

void sqlrprotocol_tds::colData(sqlrservercursor *cursor, uint16_t col) {

	debugStart("col %d",col);

	byte_t	tdstype=mapType(cont->getColumnType(cursor,col));

	userType(tdstype);
	colFlags(cursor,col,tdstype);
	typeInfo(cursor,col,tdstype);
	tableName(tdstype);
	cryptoMetaData();
	colName(cursor,col);

	debugEnd();
}

void sqlrprotocol_tds::userType(byte_t tdstype) {

	uint32_t	usertype=0;

	// * = 0x0000 by default
	// * = 0x0050 for timestamp types
	// * > 0x00FF for alias types (FIXME: how to identify these?)
	if (tdstype==TDS_TYPE_DATETIME2N) {
		usertype=0x0050;
	}

	if (negotiatedtdsversion<720) {
		writeLE(&resppacket,(uint16_t)usertype);
	} else {
		writeLE(&resppacket,usertype);
	}

	debugWrite("usertype: %d",usertype);
}

void sqlrprotocol_tds::colFlags(sqlrservercursor *cursor,
						uint16_t col,
						byte_t tdstype) {

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
}

void sqlrprotocol_tds::typeInfo(sqlrservercursor *cursor,
						uint16_t col,
						byte_t tdstype) {

	write(&resppacket,tdstype);

	debugWrite("tdstype: 0x%02x",tdstype);

	if (isFixedLenType(tdstype)) {

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
			case TDS_TYPE_XML:
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
				// the size must be sent in bytes, but the
				// backend reports it in characters
				if (size>16383) {
					size=16383;
				}
				size*=sizeof(ucs2_t);
				writeLE(&resppacket,(uint16_t)size);
				debugWrite("size: %d (16-bit)",size);
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
				size=nTypeSize(tdstype,size);
				write(&resppacket,(byte_t)size);
				debugWrite("size: %d (8-bit)",size);
				break;
			case TDS_TYPE_NUMERIC:
			case TDS_TYPE_NUMERICN:
			case TDS_TYPE_DECIMAL:
			case TDS_TYPE_DECIMALN:
				// for these, the size is the widest the value
				// can be on the wire - 1 sign byte plus up to
				// 16 bytes of magnitude - rather than the
				// precision, and real servers just send the
				// max, whatever the precision is
				size=17;
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
					{
					// FIXME: collation...
					// send negotiated lcid?
					// for now, send "raw" collation
					byte_t	coll[5]={0,0,0,0,0};
					write(&resppacket,coll,sizeof(coll));
					if (getDebug()) {
						stringbuffer	b;
						b.printBits(coll,sizeof(coll));
						debugWrite(
							"collation: %s",
							b.getString());
					}
					}
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
}

void sqlrprotocol_tds::tableName(byte_t tdstype) {

	if (tdstype!=TDS_TYPE_TEXT &&
			tdstype!=TDS_TYPE_NTEXT &&
			tdstype!=TDS_TYPE_IMAGE) {
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
}

void sqlrprotocol_tds::cryptoMetaData() {

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

	size_t 		namelen=cont->getColumnNameSize(cursor,col);
	const char	*name=cont->getColumnName(cursor,col);
	ucs2_t		*name16=ucs2charstring::duplicate(name,namelen);
	write(&resppacket,(byte_t)namelen);
	write(&resppacket,name16,namelen);

	debugWrite("namelen: %d",namelen);
	debugWrite("name: %s",name);

	delete[] name16;
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

byte_t sqlrprotocol_tds::nTypeSize(byte_t tdstype, uint32_t colsize) {

	switch (tdstype) {
		case TDS_TYPE_BITN:
			return 1;
		case TDS_TYPE_INTN:
			// storage widths pass through
			if (colsize==1 || colsize==2 ||
					colsize==4 || colsize==8) {
				return (byte_t)colsize;
			}
			// otherwise it's a digit count
			if (colsize<=3) {
				return 1;
			} else if (colsize<=5) {
				return 2;
			} else if (colsize<=10) {
				return 4;
			}
			return 8;
		case TDS_TYPE_FLTN:
		case TDS_TYPE_MONEYN:
		case TDS_TYPE_DATETIMN:
			// only 4 and 8 are valid, and column size is
			// sometimes reported as 0
			return (colsize==4)?4:8;
	}
	return (byte_t)colsize;
}

uint64_t sqlrprotocol_tds::rows(sqlrservercursor *cursor) {
	return rows(cursor,0);
}

uint64_t sqlrprotocol_tds::rows(sqlrservercursor *cursor, uint64_t maxrows) {

	// get col count and bail if there are no columns
	uint32_t	colcount=cont->colCount(cursor);
	if (!colcount) {
		// return the affected row count though
		return cont->getAffectedRows(cursor);
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
		debugWrite("token: 0x%02x",token);

		// append the fields to the packet
		for (uint32_t col=0; col<colcount; col++) {

			// get/map the column type
			// FIXME: cache this earlier and just look it up here
			byte_t	tdstype=
				mapType(cont->getColumnType(cursor,col));

			debugStart("col %d",col);
			debugWrite("tdstype: 0x%02x",tdstype);

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
			field(tdstype,
				// FIXME: cache these earlier and
				// just look them up here
				cont->getColumnSize(cursor,col),
				cont->getColumnScale(cursor,col),
				fld,fldsize,null);

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
}

void sqlrprotocol_tds::field(byte_t tdstype,
				uint32_t colsize,
				uint32_t colscale,
				const char *field,
				uint64_t fieldsize,
				bool null) {

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
				write(&resppacket,(byte_t)0xFF);
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
			case TDS_TYPE_XML:
			case TDS_TYPE_TEXT:
			case TDS_TYPE_NTEXT:
			case TDS_TYPE_IMAGE:
			case TDS_TYPE_SSVARIANT:
				writeLE(&resppacket,(uint32_t)0xFFFFFFFF);
				break;
		}

		return;
	}

	// handle variable size types by appending the size, then
	// changing the type so the switch below will append the data
	switch (tdstype) {
		case TDS_TYPE_INTN:
			{
			byte_t	size=nTypeSize(tdstype,colsize);
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
			byte_t	size=nTypeSize(tdstype,colsize);
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
			byte_t	size=nTypeSize(tdstype,colsize);
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
			byte_t	size=nTypeSize(tdstype,colsize);
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
			debugWrite("%ld (4 bytes)",data);
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
			char	*copy=charstring::duplicate(field);
			charstring::strip(copy,'.');
			int64_t	data=charstring::convertToInteger(copy)*100;
			delete[] copy;
			writeLE(&resppacket,
				(uint32_t)((data&0xFFFFFFFF00000000LL)>>32));
			writeLE(&resppacket,
				(uint32_t)(data&0x00000000FFFFFFFFLL));
			debugWrite("data: ");
			debugWrite("%ld %ld (%s)",
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
			char	*copy=charstring::duplicate(field);
			charstring::strip(copy,'.');
			int32_t	data=charstring::convertToInteger(copy)*100;
			delete[] copy;
			writeLE(&resppacket,(uint32_t)data);
			debugWrite("data: ");
			debugWrite("%lld (%s)",data,field);
			}
			break;
		case TDS_TYPE_INT8:
			{
			int64_t	data=charstring::convertToInteger(field);
			writeLE(&resppacket,(uint64_t)data);
			debugWrite("data: ");
			debugWrite("%lld (8 bytes)",data);
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
					debugWrite("%lld ",*v);
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
			debugWrite("size: %d",fieldsize);
			debugWrite("data: ");
			debugWrite("%.*s",fieldsize,field);
			}
			break;
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
			{
			// FIXME: TDS backends encode these as text, where
			// each pair of characters are the hex value of a
			// byte.  However, other databases may encode them
			// differently.
			write(&resppacket,(byte_t)(fieldsize/2));
			const char	*f=field;
			for (byte_t i=0; i<fieldsize/2; i++) {
				write(&resppacket,charsToHex(f));
				f+=2;
			}
			debugWrite("size: %d",fieldsize);
			debugWrite("data:");
			debugHexDump((byte_t *)field,fieldsize);
			}
			break;
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGVARBIN:
			{
			// FIXME: TDS backends encode these as text, where
			// each pair of characters are the hex value of a
			// byte.  However, other databases may encode them
			// differently.
			writeLE(&resppacket,(uint16_t)(fieldsize/2));
			const char	*f=field;
			for (uint16_t i=0; i<(fieldsize/2); i++) {
				write(&resppacket,charsToHex(f));
				f+=2;
			}
			debugWrite("size: %d",fieldsize);
			debugWrite("data:");
			debugHexDump((byte_t *)field,fieldsize);
			}
			break;
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_BIGVARCHR:
			{
			writeLE(&resppacket,(uint16_t)fieldsize);
			write(&resppacket,field,fieldsize);
			debugWrite("size: %d",fieldsize);
			debugWrite("data: ");
			debugWrite("%.*s",fieldsize,field);
			}
			break;
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_NVARCHAR:
			{
			// the data is ucs-2, and the size is in bytes
			ucs2_t	*field16=ucs2charstring::duplicate(
							field,fieldsize);
			writeLE(&resppacket,
				(uint16_t)(fieldsize*sizeof(ucs2_t)));
			write(&resppacket,field16,fieldsize);
			delete[] field16;
			debugWrite("size: %lld",
				(uint64_t)(fieldsize*sizeof(ucs2_t)));
			debugWrite("data: ");
			debugWrite("%.*s",fieldsize,field);
			}
			break;
		case TDS_TYPE_UDT:
			// FIXME: ???
			break;
		case TDS_TYPE_TEXT:
			{
			writeLE(&resppacket,(uint32_t)fieldsize);
			write(&resppacket,field,fieldsize);
			debugWrite("size: %d",fieldsize);
			debugWrite("data: ");
			debugWrite("%.*s",fieldsize,field);
			}
			break;
		case TDS_TYPE_XML:
		case TDS_TYPE_NTEXT:
			{
			// the data is ucs-2, and the size is in bytes
			ucs2_t	*field16=ucs2charstring::duplicate(
							field,fieldsize);
			writeLE(&resppacket,
				(uint32_t)(fieldsize*sizeof(ucs2_t)));
			write(&resppacket,field16,fieldsize);
			delete[] field16;
			debugWrite("size: %lld",
				(uint64_t)(fieldsize*sizeof(ucs2_t)));
			debugWrite("data: ");
			debugWrite("%.*s",fieldsize,field);
			}
			break;
		case TDS_TYPE_IMAGE:
		case TDS_TYPE_SSVARIANT:
			{
			// FIXME: TDS backends encode these as text, where
			// each pair of characters are the hex value of a
			// byte.  However, other databases may encode them
			// differently.
			writeLE(&resppacket,(uint32_t)(fieldsize/2));
			const char	*f=field;
			for (uint32_t i=0; i<fieldsize/2; i++) {
				write(&resppacket,charsToHex(f));
				f+=2;
			}
			debugWrite("size: %d",fieldsize);
			debugWrite("data:");
			debugHexDump((byte_t *)field,fieldsize);
			}
			break;
	}
}

static uint16_t mdays[]={31,28,31,30,31,30,31,31,30,31,30,31};

static bool isLeapYear(int32_t year) {
	return (!(year%4) && (year%100 || !(year%400)));
}

// Rudiments' datetime::parse takes a trailing "+hh:mm" for a third date/time
// part and fails, and for a string that supplies only a date or only a time it
// leaves the rest at -1.  Neither survives the writers' unsigned arithmetic, so
// split the offset off first and floor whatever the string didn't supply.
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

	// floor whatever the string didn't supply
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

uint32_t sqlrprotocol_tds::daysSince1(int16_t year, int16_t month, int16_t day) {

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
	debugWrite("increments since 12AM: %lld",*increments);
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
	debugWrite("increments since 12AM: %lld",increments);
}

void sqlrprotocol_tds::daten(const char *field) {
	uint32_t	dayssince1;
	date(field,&dayssince1);
	write(&resppacket,(byte_t)3);
	appendDate(dayssince1);
}

void sqlrprotocol_tds::timen(const char *field, byte_t scale) {
	uint64_t	increments;
	time(field,scale,&increments);
	byte_t	size=timeSize(scale);
	write(&resppacket,size);
	appendTime(increments,size);
}

void sqlrprotocol_tds::datetime2n(const char *field, byte_t scale) {

	// a datetime2 is a time followed by a date,
	// under a single size
	uint64_t	increments;
	time(field,scale,&increments);
	uint32_t	dayssince1;
	date(field,&dayssince1);

	byte_t	size=timeSize(scale);
	write(&resppacket,(byte_t)(size+3));
	appendTime(increments,size);
	appendDate(dayssince1);
}

void sqlrprotocol_tds::datetimeoffsetn(const char *field, byte_t scale) {

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

		// The date and time parts of a datetimeoffset are the utc
		// instant, not the local wall time - the offset is carried
		// alongside so the client can render it back.  Shift by the
		// offset, borrowing or carrying a day if it crosses midnight.
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
	write(&resppacket,(byte_t)(size+3+sizeof(uint16_t)));
	appendTime(increments,size);
	appendDate(dayssince1);
	writeLE(&resppacket,(uint16_t)tzoffset);

	debugWrite("utc offset in minutes: %d",tzoffset);
}

void sqlrprotocol_tds::decimal(const char *field,
				byte_t *ispositive,
				byte_t *size,
				byte_t *val) {

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
}

void sqlrprotocol_tds::guid(const char *field, byte_t *g) {

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

	// if the server is mssql/sap then parse the various parts
	// out of the errorbuffer, which looks like:
	// Server message: ... severity(...) number(...) state(...) line(...)
	// Server Name:... Procedure Name:...
	// (note 2 spaces between line(...) and Server Name and no spaces after
	// the colons after Server Name and Procedure Name)
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

		// The column name, which freetds bracket-quotes, doubling
		// any bracket in the name itself.  It gets copied out
		// verbatim, quoting and all, so that the insert below asks
		// for the same column the client did.
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

	// bulk data without an insert bulk statement to go with it
	if (!bulktable) {
		debugWrite("no insert bulk statement");
		debugEnd();
		return sendError(0,1,16,
				"Bulk load data without an insert bulk "
				"statement",1);
	}

	// The packet opens with the client's own column metadata.  A bad
	// one doesn't end the session, unlike a protocol error elsewhere -
	// each request arrives as a whole packet, so dropping this one
	// leaves the request stream in sync.
	uint16_t	colcount=0;
	if (!bulkColMetaData(&rp,&rpsize,&colcount)) {
		debugEnd();
		return sendError(0,1,16,
				"Malformed bulk load column metadata",1);
	}

	// The insert bulk statement and the column metadata describe the
	// same columns, in the same order.  If they disagree, there's no
	// way to tell which column a value belongs to.
	if (colcount!=bulkcolumncount) {
		debugWrite("column count mismatch: %d != %d",
					colcount,bulkcolumncount);
		debugEnd();
		return sendError(0,1,16,
				"Bulk load column count doesn't match the "
				"insert bulk statement",1);
	}

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
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
		debugWrite("query too large: %lld",(uint64_t)querylen);
		debugEnd();
		cont->release(cursor);
		return sendQueryTooLargeError(querylen);
	}
	cont->setOutputBindCount(cursor,0);
	cont->setInputBindCount(cursor,colcount);
	bool	success=cont->prepareQuery(cursor,query,querylen,
						true,true,true,true);
	delete[] query;

	// run it once per row
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

	debugWrite("rows: %lld",rowcount);
	debugEnd();

	if (badrow) {
		cont->release(cursor);
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
	cont->release(cursor);

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
	debugWrite("token: 0x%02x",token);
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

		// A blob column carries the table name, but without the
		// numparts byte that a server-to-client col meta data has.
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

	if (!rpsize) {
		return false;
	}
	byte_t	tdstype;
	read(rp,&tdstype,&rp);
	rpsize--;
	debugWrite("tdstype: 0x%02x",tdstype);

	bulktypes[col]=tdstype;
	bulksizes[col]=0;
	bulkscales[col]=0;

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
			case TDS_TYPE_XML:
			case TDS_TYPE_UDT:
				{
				if (rpsize<sizeof(uint16_t)) {
					return false;
				}
				uint16_t	size;
				readLE(rp,&size,&rp);
				rpsize-=sizeof(uint16_t);
				bulksizes[col]=size;
				debugWrite("size: %d (16-bit)",size);
				}
				break;
			default:
				{
				if (!rpsize) {
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
		return false;
	}

	*rpinout=rp;
	*rpsizeinout=rpsize;

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
		if (isBinaryType(bulktypes[col])) {
			// style 2 reads the string as hex digits without a
			// leading 0x - see bulkBinary() for why the value
			// arrives as hex in the first place
			uint32_t	size=bulksizes[col];
			query.append("convert(varbinary(");
			query.append((size && size<=8000)?size:8000);
			query.append("),")->append(bindvarnames[col]);
			query.append(",2)");
		} else {
			query.append(bindvarnames[col]);
		}
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

	debugWrite("token: 0x%02x",token);

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
	bv->type=SQLRSERVERBINDVARTYPE_STRING;
	bv->valuesize=(uint32_t)valuesize;
	bv->value.stringval=(char *)bindpool->allocate(valuesize+1);
	bytestring::copy(bv->value.stringval,value,valuesize);
	bv->value.stringval[valuesize]='\0';
	bv->isnull=cont->getNonNullBindValue();
	debugWrite("value: %.*s",(int32_t)valuesize,bv->value.stringval);
}

bool sqlrprotocol_tds::isBinaryType(byte_t tdstype) {

	switch (tdstype) {
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGVARBIN:
		case TDS_TYPE_IMAGE:
			return true;
		default:
			return false;
	}
}

void sqlrprotocol_tds::bulkBinary(sqlrserverbindvar *bv,
					memorypool *bindpool,
					const byte_t *value,
					size_t valuesize) {

	// Binary values are bound as hex, and converted back to bytes by
	// the insert that bulkInsert() builds.  A blob bind would be the
	// obvious way to do this, but the odbc connection module binds a
	// blob as a character string, and character strings get charset
	// converted, which raw bytes don't survive.

	stringbuffer	hex;
	for (size_t i=0; i<valuesize; i++) {
		hex.append(cont->asciiToHex(value[i]));
	}
	bulkString(bv,bindpool,hex.getString(),hex.getStringLength());
}

void sqlrprotocol_tds::bulkDouble(sqlrserverbindvar *bv, double value) {

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
}

void sqlrprotocol_tds::bulkDecimal(byte_t ispositive,
					const byte_t *val,
					byte_t size,
					byte_t scale,
					stringbuffer *strb) {

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
	charstring::printf(digits,sizeof(digits),"%lld",magnitude);
	size_t	digitcount=charstring::getLength(digits);
	if (!scale) {
		strb->append(digits);
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
}

void sqlrprotocol_tds::bulkMoney(int64_t tenthousandths, stringbuffer *strb) {
	if (tenthousandths<0) {
		strb->append('-');
		tenthousandths=-tenthousandths;
	}
	strb->append((uint64_t)(tenthousandths/10000))->append('.');
	char	fraction[8];
	charstring::printf(fraction,sizeof(fraction),"%04lld",
					(uint64_t)(tenthousandths%10000));
	strb->append(fraction);
}

void sqlrprotocol_tds::bulkYmd(int32_t days,
					int32_t startyear,
					stringbuffer *strb) {

	// A datetime counts from 1900-01-01 and can run backwards, since
	// the range starts in 1753.  The year loops are bounded by the
	// range the format itself allows, so a garbage day count can't
	// spin here.
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
					seconds/3600,(seconds/60)%60,
					seconds%60);
	strb->append(buffer);

	if (scale) {
		strb->append('.');
		charstring::printf(buffer,sizeof(buffer),"%lld",fraction);
		size_t	digitcount=charstring::getLength(buffer);
		for (size_t i=digitcount; i<scale; i++) {
			strb->append('0');
		}
		strb->append(buffer);
	}
}

void sqlrprotocol_tds::bulkGuid(const byte_t *g, stringbuffer *strb) {

	// the first three groups are little-endian, the rest big-endian
	char	buffer[40];
	charstring::printf(buffer,sizeof(buffer),
		"%02X%02X%02X%02X-%02X%02X-%02X%02X-"
		"%02X%02X-%02X%02X%02X%02X%02X%02X",
		g[3],g[2],g[1],g[0],g[5],g[4],g[7],g[6],
		g[8],g[9],g[10],g[11],g[12],g[13],g[14],g[15]);
	strb->append(buffer);
}

bool sqlrprotocol_tds::bulkField(const byte_t **rpinout,
					size_t *rpsizeinout,
					uint16_t col,
					sqlrserverbindvar *bv,
					memorypool *bindpool) {

	debugStart("col %d",col);
	debugWrite("tdstype: 0x%02x",bulktypes[col]);

	// until proven otherwise
	bv->type=SQLRSERVERBINDVARTYPE_NULL;
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

	// A text, ntext or image value arrives behind a text pointer and
	// timestamp, both of which a bulk load fills with 0xFF.  A single
	// zero length in place of the pointer means null.
	if (tdstype==TDS_TYPE_TEXT ||
		tdstype==TDS_TYPE_NTEXT ||
		tdstype==TDS_TYPE_IMAGE) {
		if (!rpsize) {
			return false;
		}
		byte_t	ptrsize;
		read(rp,&ptrsize,&rp);
		rpsize--;
		if (!ptrsize) {
			debugWrite("value: (null)");
			return true;
		}
		size_t	prefixsize=(size_t)ptrsize+sizeof(uint64_t);
		if (rpsize<prefixsize) {
			return false;
		}
		rp+=prefixsize;
		rpsize-=prefixsize;
	}

	// The "n" types carry a size that also says which concrete type
	// the value is, and a size of zero means null.
	switch (tdstype) {
		case TDS_TYPE_INTN:
			{
			if (!rpsize) {
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
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
					return false;
			}
			}
			break;
		case TDS_TYPE_BITN:
			{
			if (!rpsize) {
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				return true;
			}
			if (size!=1) {
				debugWrite("invalid size: %d",size);
				return false;
			}
			tdstype=TDS_TYPE_BIT;
			}
			break;
		case TDS_TYPE_FLTN:
			{
			if (!rpsize) {
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
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
					return false;
			}
			}
			break;
		case TDS_TYPE_MONEYN:
			{
			if (!rpsize) {
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
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
					return false;
			}
			}
			break;
		case TDS_TYPE_DATETIMN:
			{
			if (!rpsize) {
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
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
				return false;
			}
			byte_t	val;
			read(rp,&val,&rp);
			rpsize--;
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=1;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",bv->value.integerval);
			}
			break;
		case TDS_TYPE_INT2:
			{
			if (rpsize<sizeof(uint16_t)) {
				return false;
			}
			int16_t	val;
			readLE(rp,(uint16_t *)&val,&rp);
			rpsize-=sizeof(uint16_t);
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=2;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",bv->value.integerval);
			}
			break;
		case TDS_TYPE_INT4:
			{
			if (rpsize<sizeof(uint32_t)) {
				return false;
			}
			int32_t	val;
			readLE(rp,(uint32_t *)&val,&rp);
			rpsize-=sizeof(uint32_t);
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=4;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",bv->value.integerval);
			}
			break;
		case TDS_TYPE_INT8:
			{
			if (rpsize<sizeof(uint64_t)) {
				return false;
			}
			int64_t	val;
			readLE(rp,(uint64_t *)&val,&rp);
			rpsize-=sizeof(uint64_t);
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->valuesize=8;
			bv->isnull=cont->getNonNullBindValue();
			bv->value.integerval=val;
			debugWrite("value: %lld",bv->value.integerval);
			}
			break;
		case TDS_TYPE_FLT4:
			{
			if (rpsize<sizeof(float)) {
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
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (rpsize<size) {
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
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				return true;
			}
			if (rpsize<size) {
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
			bulkString(bv,bindpool,strb.getString(),
						strb.getStringLength());
			}
			break;
		case TDS_TYPE_DATEN:
			{
			if (!rpsize) {
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				return true;
			}
			if (rpsize<size) {
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
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (!size) {
				debugWrite("value: (null)");
				return true;
			}
			if (rpsize<size) {
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
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (rpsize<size) {
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
				return false;
			}
			byte_t	size;
			read(rp,&size,&rp);
			rpsize--;
			if (rpsize<size) {
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
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);
			if (size==0xFFFF) {
				debugWrite("value: (null)");
				return true;
			}
			if (rpsize<size) {
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
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);
			if (size==0xFFFF) {
				debugWrite("value: (null)");
				return true;
			}
			if (rpsize<size) {
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
				return false;
			}
			uint16_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint16_t);
			if (size==0xFFFF) {
				debugWrite("value: (null)");
				return true;
			}
			if (rpsize<size) {
				return false;
			}
			uint16_t	length=size/sizeof(ucs2_t);
			// the data isn't necessarily aligned, so copy it
			// out before converting it
			const byte_t	*dummy;
			ucs2_t		*value16=new ucs2_t[length];
			read(rp,value16,length,&dummy);
			char		*value=charstring::duplicateUcs2(
						value16,(size_t)length);
			delete[] value16;
			bulkString(bv,bindpool,value,length);
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
				return false;
			}
			uint32_t	size;
			readLE(rp,&size,&rp);
			rpsize-=sizeof(uint32_t);
			if (size==0xFFFFFFFF) {
				debugWrite("value: (null)");
				return true;
			}
			if (rpsize<size) {
				return false;
			}
			if (tdstype==TDS_TYPE_NTEXT) {
				// the size is in bytes,
				// but the data is ucs-2
				uint32_t	length=size/sizeof(ucs2_t);
				const byte_t	*dummy;
				ucs2_t		*value16=new ucs2_t[length];
				read(rp,value16,length,&dummy);
				char		*value=
					charstring::duplicateUcs2(
						value16,(size_t)length);
				delete[] value16;
				bulkString(bv,bindpool,value,length);
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
			// FIXME: partlen types arrive as a 64-bit length
			// followed by chunks, which nothing sends yet
			// because typeInfo() never declares one
			debugWrite("unsupported type");
			return false;
	}

	return true;
}

bool sqlrprotocol_tds::remoteProcedureCall() {

	const byte_t	*rp=reqpacket.getBuffer();
	size_t		rpsize=reqpacket.getSize();

	debugStart("rpc");

	// get the headers
	if (negotiatedtdsversion>=720) {
		allHeaders(rp,rpsize,&rp,&rpsize);
	}

	// begin building the response packet
	resppacket.clear();

	// a single request packet can carry a batch of rpc's
	bool	more=true;
	while (more) {
		if (!rpc(&rp,&rpsize,&more)) {
			debugEnd();
			// a protocol error means the request stream is
			// out of sync, so end the session after reporting it
			sendTdsProtocolError();
			return false;
		}
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

	// nothing left to do
	if (rpsize<sizeof(uint16_t)) {
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
			return false;
		}
		readLE(rp,&procid,&rp);
		rpsize-=sizeof(procid);

		debugWrite("procid: %hd (%s)",
				procid,procids[(procid<=SP_MAX_PROCID)?
								procid:0]);

	} else {

		// bounds checking
		if (procnamelen*sizeof(ucs2_t)>rpsize ||
					procnamelen>maxquerysize) {
			debugWrite("invalid proc name length: %hd",procnamelen);
			return false;
		}

		// get the procname
		ucs2_t	*procname16=new ucs2_t[procnamelen];
		read(rp,procname16,procnamelen,&rp);
		rpsize-=procnamelen*sizeof(ucs2_t);
		procname=charstring::duplicateUcs2(procname16,
							(size_t)procnamelen);
		delete[] procname16;

		debugWrite("procname: %s",procname);

		// a client can send any of the numbered procs by name
		procid=procNameToProcId(procname);
		if (procid) {
			debugWrite("procid: %hd (%s)",procid,procids[procid]);
		}
	}


	// get option flags
	uint16_t	optionflags=0;
	if (rpsize<sizeof(optionflags)) {
		debugWrite("truncated option flags");
		delete[] procname;
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
		return false;
	}

	// get the trailing batch flags
	batchFlags(rp,rpsize,&rp,&rpsize,more);


	// do whatever the proc asked for
	bool	retval=true;
	rpcfailed=false;
	switch (procid) {
		case SP_CURSOR:
			retval=cursorUnsupported();
			break;
		case SP_CURSOR_OPEN:
			retval=cursorOpen(nometadata);
			break;
		case SP_CURSOR_PREPARE:
			retval=cursorPrepare();
			break;
		case SP_CURSOR_EXECUTE:
			retval=cursorExecute(nometadata);
			break;
		case SP_CURSOR_PREP_EXEC:
			retval=cursorPrepExec(nometadata);
			break;
		case SP_CURSOR_UNPREPARE:
			retval=cursorUnprepare();
			break;
		case SP_CURSOR_FETCH:
			retval=cursorFetch(nometadata);
			break;
		case SP_CURSOR_OPTION:
			retval=cursorOption();
			break;
		case SP_CURSOR_CLOSE:
			retval=cursorClose();
			break;
		case SP_EXECUTE_SQL:
			retval=executeSql(nometadata);
			break;
		case SP_PREPARE:
			retval=prepare(false,false,nometadata);
			break;
		case SP_EXECUTE:
			retval=execute(nometadata);
			break;
		case SP_PREP_EXEC:
			retval=prepare(true,false,nometadata);
			break;
		case SP_PREP_EXEC_RPC:
			retval=prepare(true,true,nometadata);
			break;
		case SP_UNPREPARE:
			retval=unprepare();
			break;
		default:
			retval=namedProc(procname,nometadata);
			break;
	}

	// The done for a non-final rpc in a batch says so.  A failed one has to
	// set DONE_ERROR - the ct-lib client reports CS_CMD_FAIL for a done
	// that carries it and CS_CMD_SUCCEED for one that doesn't, so without
	// it a failed rpc reports success and the client's ct_results walk
	// falls one result out of step with the response.
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

	return retval;
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

		// Bind variables are named by position, not by whatever the
		// client called them.  Backends work out which parameter a
		// bind is from the number in its name - odbccursor::inputBind
		// does exactly that - so a client's @P1 has to become @1.
		// The names the client used in the statement itself are
		// matched up by translatebindvariables, in order.
		bv->variable=bindvarnames[bindindex];
		bv->variablesize=bindvarnamesizes[bindindex];

		// copy string values out of the rpc pool
		if (bv->type==SQLRSERVERBINDVARTYPE_STRING &&
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
}

uint32_t sqlrprotocol_tds::newHandle() {
	// handle 0 is invalid, so skip it if the counter wraps
	if (!nexthandle) {
		nexthandle=1;
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
			continue;
		}
		executeflag.remove(cursor);
		cont->release(cursor);
	}
	handles->clear();
}

char *sqlrprotocol_tds::callSyntaxToExec(const char *stmt) {

	// sp_prepexecrpc gets its statement in odbc call syntax -
	// {call procname(?,?)} or {? = call procname(?,?)} - rather than
	// as plain sql

	const char	*ptr=cont->skipWhitespaceAndComments(stmt);
	if (*ptr!='{') {
		return charstring::duplicate(stmt);
	}
	ptr++;

	// skip a return value placeholder
	const char	*eq=charstring::findFirst(ptr,'=');
	const char	*call=charstring::findFirstIgnoringCase(ptr,"call");
	if (!call) {
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

	return query.detachString();
}

bool sqlrprotocol_tds::rpcInvalidHandleError(uint32_t number,
						const char *what,
						uint32_t handle) {

	stringbuffer	err;
	err.append("Invalid ")->append(what)->append(' ')->append(handle);

	debugWrite("%s",err.getString());

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

	debugWrite("%s",err.getString());

	appendError(RPC_WRONG_PARAM_TYPE,3,16,
			err.getString(),srvname,procname,1);
	returnStatus(RPC_WRONG_PARAM_TYPE);
	rpcfailed=true;

	return true;
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
	uint32_t	number=appendQueryError(cursor);
	if (returnstatus) {
		returnStatus(number);
	}
	rpcfailed=true;
}

void sqlrprotocol_tds::rpcResultSet(sqlrservercursor *cursor,
						bool nometadata,
						uint64_t maxrows) {

	// a statement that returns no columns still has to report how many
	// rows it affected
	if (!cont->colCount(cursor)) {
		doneInProc(DONE_COUNT,0,cont->getAffectedRows(cursor));
		return;
	}

	colMetaData(cursor,nometadata);
	doneInProc(DONE_COUNT,0,rows(cursor,maxrows));
}

bool sqlrprotocol_tds::namedProc(const char *procname, bool nometadata) {

	// this is an ordinary stored procedure call, rather than one of the
	// numbered procs

	if (!procname) {
		return sendUnimplementedFeatureError();
	}

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		return sendNoCursorAvailableError();
	}

	// Build the query, naming a bind variable per parameter.  A by-ref
	// parameter gets no T-SQL "output" keyword after it.  The direction
	// is already carried by the bind itself - SQL_PARAM_OUTPUT in the
	// odbc module, CS_RETURN in the freetds and sap ones - and sql
	// server rejects "output" after a parameter marker outright.
	//
	// This is odbc call syntax rather than "exec procname ...", because
	// that has nowhere to put the procedure's return value and the
	// client expects one in the RETURNSTATUS token.  The bind variable
	// before "=call" is the return value, which is why the client's
	// parameters start at bindvarnames[1].
	//
	// The space after the brace is load-bearing: beforeBindVariable()
	// in src/common/bindvariables.h does not treat '{' as something a
	// bind variable can follow, so without it the return value's marker
	// is never translated to the backend's bind format.
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

	// build the response.  An ordinary procedure that never ran - because
	// there is no such procedure, say - sends no return status at all,
	// unlike the numbered procs, which send the error's own number.
	if (success) {
		rpcResultSet(cursor,nometadata,0);
		returnStatus(procReturnValue(cursor));
		returnValues(cursor);
	} else {
		rpcError(cursor,false);
	}

	// release the cursor
	cont->release(cursor);

	return true;
}

bool sqlrprotocol_tds::executeSql(bool nometadata) {

	// sp_executesql @stmt, [@params, [values...]]

	const char	*stmt=paramString(0);
	if (!stmt) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	// these two arguments are declared nvarchar
	if (!paramIsUnicode(0)) {
		return rpcParamTypeError("sp_executesql","@statement");
	}
	if (rpcparamcount>1 && !paramIsUnicode(1)) {
		return rpcParamTypeError("sp_executesql","@params");
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		return sendQueryTooLargeError(stmtlen);
	}

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		return sendNoCursorAvailableError();
	}

	// the parameter declaration string is only there if there are
	// parameters, and the values follow it
	uint16_t	firstvalue=(rpcparamcount>1)?2:1;

	// A statement that comes with no values has no bind variables in
	// it.  A single-@ name in one is a local variable or a parameter
	// declaration.
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
	cont->release(cursor);

	return true;
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

	const char	*stmt=paramString(stmtparam);
	if (!stmt) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	// these arguments are declared nvarchar.  A rejected call leaves the
	// handle invalid, which the client reads out of a null return value.
	const char	*pn=(rpcsyntax)?"sp_prepexecrpc":
				((prepexec)?"sp_prepexec":"sp_prepare");
	if (!rpcsyntax && !paramIsUnicode(1)) {
		rpcParamTypeError(pn,"params");
		returnValueInteger(1,0,true);
		return true;
	}
	if (!paramIsUnicode(stmtparam)) {
		rpcParamTypeError(pn,"stmt");
		returnValueInteger(1,0,true);
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
		return sendQueryTooLargeError(querylen);
	}

	// reuse the handle the client sent, if it sent a live one
	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (cursor) {
		// re-preparing a live handle replaces its statement
		stmthandles.remove(handle);
		cont->release(cursor);
	}

	// get an available cursor
	cursor=cont->getCursor();
	if (!cursor) {
		delete[] query;
		return sendNoCursorAvailableError();
	}

	handle=newHandle();

	// prepare the query
	bool	success=cont->prepareQuery(cursor,query,querylen,
							true,true,true,true);
	delete[] query;

	if (success) {
		executeflag.setValue(cursor,true);
		if (prepexec) {
			bindParams(cursor,firstvalue);
			success=cont->executeQuery(cursor,true,true,true,true);
			executeflag.setValue(cursor,false);
		}
	}

	if (!success) {
		rpcError(cursor);
		cont->release(cursor);
		// the handle never became valid
		returnValueInteger(1,0,true);
		return true;
	}

	// hang on to the cursor - sp_execute will want it
	stmthandles.setValue(handle,cursor);

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

	return true;
}

bool sqlrprotocol_tds::execute(bool nometadata) {

	// sp_execute @handle, [values...]

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,
					"prepared statement handle",
								handle);
	}

	debugWrite("prepared handle: %d",handle);

	// bind and run the prepared query
	bindParams(cursor,1);
	bool	success=cont->executeQuery(cursor,true,true,true,true);
	executeflag.setValue(cursor,false);

	// build the response
	if (success) {
		rpcResultSet(cursor,nometadata,0);
		returnStatus(RPC_STATUS_SUCCESS);
	} else {
		rpcError(cursor);
	}

	return true;
}

bool sqlrprotocol_tds::unprepare() {

	// sp_unprepare @handle

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,
					"prepared statement handle",
								handle);
	}

	debugWrite("prepared handle: %d",handle);

	stmthandles.remove(handle);
	executeflag.remove(cursor);
	cont->release(cursor);

	returnStatus(RPC_STATUS_SUCCESS);

	return true;
}

bool sqlrprotocol_tds::cursorUnsupported() {

	// sp_cursor does positioned update/delete, which needs
	// "where current of".  SQL Relay has no equivalent, so just report
	// that it didn't work.

	debugWrite("positioned update/delete is not supported");

	appendError(16957,1,16,"Positioned update/delete is not supported",
						srvname,NULL,1);
	returnStatus(16957);
	rpcfailed=true;

	return true;
}

bool sqlrprotocol_tds::cursorOpen(bool nometadata) {

	// sp_cursoropen @cursor output, @stmt, [@scrollopt output,
	//		[@ccopt output, [@rowcount output, [values...]]]]

	const char	*stmt=paramString(1);
	if (!stmt) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		return sendQueryTooLargeError(stmtlen);
	}

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		return sendNoCursorAvailableError();
	}

	// a parameterized cursor sends a declaration string, then the values
	uint16_t	firstvalue=6;

	// A statement that comes with no values has no bind variables in
	// it.  A single-@ name in one is a local variable or a parameter
	// declaration.
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
		cont->release(cursor);
		returnValueInteger(1,0,true);
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

	// There is no scrollable cursor support in the server API - only
	// forward-only skipRow/skipRows.  Both of these are in/out and the
	// spec lets the server substitute what it can actually do.
	returnValueInteger(2,CURSOR_SCROLLOPT_FORWARD_ONLY,false);
	returnValueInteger(3,CURSOR_CCOPT_READ_ONLY,false);

	// -1 means the row count isn't known yet, which is what a
	// forward-only cursor reports
	returnValueInteger(4,-1,false);

	return true;
}

bool sqlrprotocol_tds::cursorPrepare() {

	// sp_cursorprepare @handle output, @params, @stmt, @options
	//			[, @scrollopt output [, @ccopt output]]

	const char	*stmt=paramString(2);
	if (!stmt) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		return sendQueryTooLargeError(stmtlen);
	}

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		return sendNoCursorAvailableError();
	}

	bool	success=cont->prepareQuery(cursor,stmt,stmtlen,
							true,true,true,true);
	if (!success) {
		rpcError(cursor);
		cont->release(cursor);
		returnValueInteger(1,0,true);
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

	return true;
}

bool sqlrprotocol_tds::cursorExecute(bool nometadata) {

	// sp_cursorexecute @preparedhandle, @cursor output,
	//			[@scrollopt output, [@ccopt output,
	//			[@rowcount output, [values...]]]]

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
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

	if (!success) {
		rpcError(cursor);
		returnValueInteger(1,0,true);
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

	return true;
}

bool sqlrprotocol_tds::cursorPrepExec(bool nometadata) {

	// sp_cursorprepexec @handle output, @cursor output, @params, @stmt,
	//			[@scrollopt output, [@ccopt output,
	//			[@rowcount output, [values...]]]]

	const char	*stmt=paramString(3);
	if (!stmt) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,"statement",0);
	}

	debugWrite("stmt: %s",stmt);

	// bounds checking
	size_t	stmtlen=charstring::getLength(stmt);
	if (stmtlen>maxquerysize) {
		return sendQueryTooLargeError(stmtlen);
	}

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		return sendNoCursorAvailableError();
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
		cont->release(cursor);
		returnValueInteger(1,0,true);
		returnValueInteger(2,0,true);
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

	return true;
}

bool sqlrprotocol_tds::cursorUnprepare() {

	// sp_cursorunprepare @handle

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&stmthandles,handle);
	if (!cursor) {
		return rpcInvalidHandleError(RPC_NO_SUCH_STMT,
					"prepared statement handle",
								handle);
	}

	debugWrite("prepared handle: %d",handle);

	stmthandles.remove(handle);

	// the cursor may still be open against this statement
	if (!handlesContain(&cursorhandles,cursor)) {
		executeflag.remove(cursor);
		cont->release(cursor);
	}

	returnStatus(RPC_STATUS_SUCCESS);

	return true;
}

bool sqlrprotocol_tds::cursorFetch(bool nometadata) {

	// sp_cursorfetch @cursor, [@fetchtype, [@rownum, [@nrows]]]

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&cursorhandles,handle);
	if (!cursor) {
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
			return true;
		}
		executeflag.setValue(cursor,false);
	}

	// Only forward-only fetching is possible.  Anything that would have
	// to go backwards or jump is refused rather than silently answered
	// with the wrong rows.
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
			appendError(16958,1,16,
					"Only forward-only cursors "
					"are supported",
					srvname,NULL,1);
			returnStatus(16958);
			rpcfailed=true;
			return true;
	}

	// fetch-info just reports what's known about the cursor
	if (fetchtype==CURSOR_FETCH_INFO) {
		doneInProc(DONE_COUNT,0,0);
		returnStatus(RPC_STATUS_SUCCESS);
		return true;
	}

	// send the rows
	if (cont->colCount(cursor)) {
		colMetaData(cursor,nometadata);
		doneInProc(DONE_COUNT,0,
			rows(cursor,(nrows>0)?(uint64_t)nrows:0));
	} else {
		doneInProc(DONE_COUNT,0,0);
	}

	returnStatus(RPC_STATUS_SUCCESS);

	return true;
}

bool sqlrprotocol_tds::cursorOption() {

	// sp_cursoroption @cursor, @code, @value

	uint32_t		handle=(uint32_t)paramInteger(0);
	sqlrservercursor	*cursor=handleCursor(&cursorhandles,handle);
	if (!cursor) {
		return rpcInvalidHandleError(RPC_NO_SUCH_CURSOR,
						"cursor handle",handle);
	}

	debugWrite("cursor handle: %d",handle);
	debugWrite("code: %lld",paramInteger(1));
	debugWrite("value: %lld",paramInteger(2));

	// None of the options - text pointers, scroll options, cursor name -
	// change anything that this module can do, so just accept them.

	returnStatus(RPC_STATUS_SUCCESS);

	return true;
}

bool sqlrprotocol_tds::cursorClose() {

	// sp_cursorclose @cursor

	uint32_t	handle=(uint32_t)paramInteger(0);

	debugWrite("cursor handle: %d",handle);

	// -1 means close them all
	if (handle==CURSOR_CLOSE_ALL) {
		releaseHandles(&cursorhandles,&stmthandles);
		returnStatus(RPC_STATUS_SUCCESS);
		return true;
	}

	sqlrservercursor	*cursor=handleCursor(&cursorhandles,handle);
	if (!cursor) {
		return rpcInvalidHandleError(RPC_NO_SUCH_CURSOR,
						"cursor handle",handle);
	}

	cursorhandles.remove(handle);

	// the prepared statement it came from may still be live
	if (!handlesContain(&stmthandles,cursor)) {
		executeflag.remove(cursor);
		cont->release(cursor);
	}

	returnStatus(RPC_STATUS_SUCCESS);

	return true;
}

bool sqlrprotocol_tds::params(const byte_t *rp,
					size_t rpsize,
					const byte_t **rpout,
					size_t *rpsizeout) {

	// reset the pool that parameter values get copied into
	rpcparampool.clear();
	rpcparamcount=0;

	bool		exceeded=false;
	const byte_t	*newrp;
	while (rpsize) {

		// The batch flags follow the last parameter, and a parameter
		// starts with its name length.  Nothing in the packet says
		// which one comes next, but no client sends a parameter name
		// anywhere near that long.
		if (*rp==RPC_BATCH_FLAG || *rp==RPC_NO_EXEC_FLAG) {
			break;
		}

		if (!param(rpcparamcount,rp,&newrp,exceeded)) {
			// protocol error
			return false;
		}

		if (!exceeded) {
			rpcparamcount++;
			if (rpcparamcount==maxbindcount) {
				exceeded=true;
			}
		}

		// param() carries no remaining-size of its own, so it can
		// run past the end of the packet on a malformed parameter.
		// Without this the subtraction underflows, and the loop
		// above then walks the heap until it segfaults.
		size_t	consumed=newrp-rp;
		if (consumed>rpsize) {
			debugWrite("parameter ran past "
					"the end of the packet");
			return false;
		}

		rpsize-=consumed;
		rp=newrp;
	}

	debugWrite("param count: %d",rpcparamcount);

	// copy out pointer and size
	*rpout=rp;
	*rpsizeout=rpsize;

	return true;
}

bool sqlrprotocol_tds::param(uint16_t param,
					const byte_t *rp,
					const byte_t **rpout,
					bool exceeded) {

	// param name
	byte_t	pnamelen;
	read(rp,&pnamelen,&rp);
	ucs2_t	*pname16=NULL;
	char	*pname=NULL;
	if (pnamelen) {
		pname16=new ucs2_t[pnamelen];
		read(rp,pname16,pnamelen,&rp);
		pname=charstring::duplicateUcs2(pname16,(size_t)pnamelen);
	}


	// status flags
	byte_t	statusflags=0;
	read(rp,&statusflags,&rp);
	bool	byrefvalue=(statusflags&0x01);
	bool	defaultvalue=(statusflags&(0x01<<1))>>1;
	// this bit is reserved
	bool	encrypted=(statusflags&(0x01<<3))>>3;
	// these 4 bits are reserved


	// FIXME: do something if defaultvalue is set...
	// FIXME: support encryption


	// The parameters are kept as they arrived, rather than being sorted
	// into input and output binds here.  Which of them are bind values
	// at all depends on which proc was called, and only the handler for
	// that proc knows.
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


	// type info...
	byte_t	tdstype;
	read(rp,&tdstype,&rp);
	debugWrite("tdstype: 0x%02x",tdstype);

	uint32_t	maxsize=0;
	byte_t		precision=0;
	byte_t		scale=0;

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
				readLE(rp,&maxsize,&rp);
				debugWrite("maxsize: %d",maxsize);
				break;
			case TDS_TYPE_BIGCHAR:
			case TDS_TYPE_BIGVARCHR:
			case TDS_TYPE_NCHAR:
			case TDS_TYPE_NVARCHAR:
			case TDS_TYPE_BIGBINARY:
			case TDS_TYPE_BIGVARBIN:
				{
				uint16_t	size;
				readLE(rp,&size,&rp);
				maxsize=size;
				debugWrite("maxsize: %d",maxsize);
				}
				break;
			default:
				{
				byte_t	size;
				read(rp,&size,&rp);
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
				read(rp,&precision,&rp);
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
				read(rp,&scale,&rp);
				debugWrite("scale: %d",scale);
				break;
		}

	} else if (isPartLenType(tdstype)) {

		debugWrite("partlentype...");

		// FIXME: [ushortmaxlen] [collation] [xml_info] [utd_info]
	}

	// An output parameter is sent back as the type the client declared
	// it, so keep that.  tdstype is rewritten just below, to read the
	// value, so it has to be recorded here.
	if (!exceeded) {
		rpcparamtdstypes[param]=tdstype;
		rpcparammaxsizes[param]=maxsize;
	}

	// param data...

	// FIXME: handle output binds too

	// handle variable size types by getting the size, then
	// changing the type so the switch below will get the data.
	// a size of 0 means the value is null.
	switch (tdstype) {
		case TDS_TYPE_INTN:
			{
			byte_t	size;
			read(rp,&size,&rp);
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
					tdstype=TDS_TYPE_NULL;
					break;
			}
			}
			break;
		case TDS_TYPE_BITN:
			{
			byte_t	size;
			read(rp,&size,&rp);
			tdstype=(size)?TDS_TYPE_BIT:TDS_TYPE_NULL;
			}
			break;
		case TDS_TYPE_FLTN:
			{
			byte_t	size;
			read(rp,&size,&rp);
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_FLT4;
					break;
				case 8:
					tdstype=TDS_TYPE_FLT8;
					break;
				default:
					tdstype=TDS_TYPE_NULL;
					break;
			}
			}
			break;
		case TDS_TYPE_MONEYN:
			{
			byte_t	size;
			read(rp,&size,&rp);
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_MONEY4;
					break;
				case 8:
					tdstype=TDS_TYPE_MONEY;
					break;
				default:
					tdstype=TDS_TYPE_NULL;
					break;
			}
			}
			break;
		case TDS_TYPE_DATETIMN:
			{
			byte_t	size;
			read(rp,&size,&rp);
			switch (size) {
				case 4:
					tdstype=TDS_TYPE_DATETIM4;
					break;
				case 8:
					tdstype=TDS_TYPE_DATETIME;
					break;
				default:
					tdstype=TDS_TYPE_NULL;
					break;
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
				byte_t	coll[5];
				read(rp,coll,sizeof(coll),&rp);
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

	// get the data
	switch (tdstype) {
		case TDS_TYPE_INT1:
		case TDS_TYPE_BIT:
			{
			char	val;
			read(rp,(byte_t *)&val,&rp);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=1;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_INT2:
			{

			int16_t	val;
			readLE(rp,(uint16_t *)&val,&rp);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=2;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}
	
			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_INT4:
			{

			int32_t	val;
			readLE(rp,(uint32_t *)&val,&rp);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=4;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_DATETIM4:
			{
			uint16_t	days;
			uint16_t	minutes;
			readLE(rp,&days,&rp);
			readLE(rp,&minutes,&rp);
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_FLT4:
			{

			float	val;
			read(rp,(float *)&val,&rp);

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
				debugWrite("value: %f",bv->value.doubleval);
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
			uint32_t	high;
			uint32_t	low;
			readLE(rp,&high,&rp);
			readLE(rp,&low,&rp);
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_DATETIME:
			{
			int32_t		dayssince1900;
			uint32_t	threehundredths;
			readLE(rp,(uint32_t *)&dayssince1900,&rp);
			readLE(rp,&threehundredths,&rp);
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_FLT8:
			{

			double	val;
			read(rp,(double *)&val,&rp);

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
				debugWrite("value: %f",bv->value.doubleval);
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
			int32_t	val;
			readLE(rp,(uint32_t *)&val,&rp);
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_INT8:
			{

			int64_t	val;
			readLE(rp,(uint64_t *)&val,&rp);

			if (bv) {
				bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
				bv->valuesize=8;
				bv->isnull=cont->getNonNullBindValue();
				bv->value.integerval=val;
			}

			if (bv) {
				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %lld",bv->value.integerval);
			}
			}
			break;
		case TDS_TYPE_GUID:
			{
			byte_t	size;
			read(rp,&size,&rp);
			// FIXME: actually implement this
			rp+=size;
			}
			break;
		case TDS_TYPE_DECIMAL:
		case TDS_TYPE_NUMERIC:
		case TDS_TYPE_DECIMALN:
		case TDS_TYPE_NUMERICN:
			{
			byte_t	ispositive;
			byte_t	val[16];
			if (tdstype==TDS_TYPE_DECIMALN ||
				tdstype==TDS_TYPE_NUMERICN) {
				byte_t	size;
				read(rp,&size,&rp);
				read(rp,&ispositive,&rp);
				read(rp,val,size-1,&rp);
			} else {
				read(rp,&ispositive,&rp);
				read(rp,val,sizeof(val),&rp);
			}
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_DATEN:
			{
			byte_t	size;
			uint16_t	dayssince1;
			read(rp,&size,&rp);
			readLE(rp,&dayssince1,&rp);
			// FIXME: actually implement this
			}
			break;
		case TDS_TYPE_TIMEN:
			// FIXME:
			// 1 unsigned integer - number of 10^-n second
			// 			increments since 12 am
			// 			within a day.
			// 3 bytes if 0 <= n <= 2
			// 4 bytes if 3 <= n <= 4
			// 5 bytes if 5 <= n <= 7
			// FIXME: actually implement this
			break;
		case TDS_TYPE_DATETIME2N:
			// FIXME:
			// concat of timen and daten
			// FIXME: actually implement this
			break;
		case TDS_TYPE_DATETIMEOFFSETN:
			// FIXME:
			// concat of datetime2n and
			// int16_t - timezone offset - minutes from utc
			// 				(between -840 and 840)
			// FIXME: actually implement this
			break;
		case TDS_TYPE_CHAR:
		case TDS_TYPE_VARCHAR:
			{
			byte_t	size;
			read(rp,&size,&rp);
			// FIXME: actually implement this
			rp+=size;
			}
			break;
		case TDS_TYPE_BINARY:
		case TDS_TYPE_VARBINARY:
			{
			byte_t	size;
			read(rp,&size,&rp);
			// FIXME: actually implement this
			rp+=size;
			}
			break;
		case TDS_TYPE_BIGBINARY:
		case TDS_TYPE_BIGVARBIN:
			{
			uint16_t	size;
			readLE(rp,&size,&rp);
			// FIXME: actually implement this
			rp+=size;
			}
			break;
		case TDS_TYPE_BIGCHAR:
		case TDS_TYPE_BIGVARCHR:
			{
			uint16_t	size;
			readLE(rp,&size,&rp);

			// 0xFFFF means null
			if (size==0xFFFF) {
				if (bv) {
					bv->type=SQLRSERVERBINDVARTYPE_NULL;
					bv->isnull=cont->getNullBindValue();
					debugWrite("value: (null)");
				}
				break;
			}

			// a char is blank padded out to its declared
			// size, like a real sql server does.  a varchar
			// isn't.
			uint32_t	valuesize=size;
			if (tdstype==TDS_TYPE_BIGCHAR && maxsize>size) {
				valuesize=maxsize;
			}

			if (bv) {

				// copy the value out of the request packet,
				// blank filling the rest
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
			}
			break;
		case TDS_TYPE_NCHAR:
		case TDS_TYPE_NVARCHAR:
			{
			// the size is in bytes, but the data is ucs-2
			uint16_t	size;
			readLE(rp,&size,&rp);

			// 0xFFFF means null
			if (size==0xFFFF) {
				if (bv) {
					bv->type=SQLRSERVERBINDVARTYPE_NULL;
					bv->isnull=cont->getNullBindValue();
					debugWrite("value: (null)");
				}
				break;
			}

			uint16_t	length=size/sizeof(ucs2_t);

			// an nchar is blank padded out to its declared
			// size, like a real sql server does.  an nvarchar
			// isn't.  the declared size is in bytes too.
			uint32_t	maxlength=maxsize/sizeof(ucs2_t);
			uint32_t	valuelength=length;
			if (tdstype==TDS_TYPE_NCHAR && maxlength>length) {
				valuelength=maxlength;
			}

			if (bv) {

				// the data isn't necessarily aligned,
				// so copy it out before converting it
				const byte_t	*dummy;
				ucs2_t		*value16=new ucs2_t[length];
				read(rp,value16,length,&dummy);
				char		*value=
					charstring::duplicateUcs2(
							value16,
							(size_t)length);
				delete[] value16;

				bv->type=SQLRSERVERBINDVARTYPE_STRING;
				bv->valuesize=valuelength;
				bv->value.stringval=(char *)
					rpcparampool.allocate(valuelength+1);
				bytestring::copy(bv->value.stringval,
							value,length);
				bytestring::set(bv->value.stringval+length,
						' ',valuelength-length);
				bv->value.stringval[valuelength]='\0';
				bv->isnull=cont->getNonNullBindValue();

				delete[] value;

				debugWrite("valuesize: %d",bv->valuesize);
				debugWrite("value: %.*s",
						bv->valuesize,
						bv->value.stringval);
			}

			rp+=size;
			}
			break;
		case TDS_TYPE_UDT:
			// FIXME: actually implement this
			break;
		case TDS_TYPE_XML:
		case TDS_TYPE_TEXT:
		case TDS_TYPE_NTEXT:
			{
			// Freetds sends anything over 4000 characters this
			// way, including the statement and parameter
			// declaration strings that the sp_ procs take.
			uint32_t	size;
			readLE(rp,&size,&rp);

			// 0xFFFFFFFF means null
			if (size==0xFFFFFFFF) {
				debugWrite("value: (null)");
				break;
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
					char	*value=
						charstring::duplicateUcs2(
							value16,(size_t)length);
					delete[] value16;

					bv->valuesize=length;
					bv->value.stringval=(char *)
						rpcparampool.allocate(length+1);
					bytestring::copy(bv->value.stringval,
								value,length);
					bv->value.stringval[length]='\0';

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
			}
			break;
		case TDS_TYPE_IMAGE:
		case TDS_TYPE_SSVARIANT:
			{
			uint32_t	size;
			readLE(rp,&size,&rp);
			// FIXME: actually implement this
			rp+=size;
			}
			break;
		case TDS_TYPE_TVP:
			if (negotiatedtdsversion>=730) {
				// FIXME:
				// TVP_TYPENAME
				// TVP_COLMETADATA
				// [TVP_ORDER_UNIQUE]
				// [TVP_COLUMN_ORDERING]
				// TVP_END_TOKEN
				// *TVP_ROW
				// TVP_END_TOKEN
				// FIXME: actually implement this
			} else {
				// protocol error
				return false;
			}
			break;
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

	// clean up
	delete[] pname16;
	delete[] pname;

	// copy out pointer
	*rpout=rp;

	return true;
}

void sqlrprotocol_tds::envChange(byte_t type,
					const wchar_t *newvalue,
					size_t newvaluelen,
					const wchar_t *oldvalue,
					size_t oldvaluelen) {

	byte_t		token=TOKEN_ENV_CHANGE;

	ucs2_t		*newvalue16=ucs2charstring::duplicate(
						newvalue,newvaluelen);
	ucs2_t		*oldvalue16=ucs2charstring::duplicate(
						oldvalue,oldvaluelen);

	uint16_t	newvaluelensize=
			(type==ENV_CHANGE_PROMOTE_TRANSACTION)?
						sizeof(uint32_t):
						sizeof(byte_t);
	uint16_t	oldvaluelensize=sizeof(byte_t);

	uint16_t	tokensize=
				sizeof(byte_t)+
				newvaluelensize+
				newvaluelen*sizeof(uint16_t)+
				oldvaluelensize+
				oldvaluelen*sizeof(uint16_t);

	debugStart("env change");
	debugWrite("token: 0x%02x",token);
	debugWrite("tokensize: 0x%02x (%hd)",tokensize,tokensize);
	debugWrite("type: %d",type);
	debugWrite("newvaluelensize:%d",newvaluelensize);
	debugWrite("newvaluelen: %d",newvaluelen);
	debugWrite("newvalue: %S",newvalue);
	debugWrite("oldvaluelensize:%d",oldvaluelensize);
	debugWrite("oldvaluelen: %d",oldvaluelen);
	debugWrite("oldvalue: %S",oldvalue);
	debugEnd();

	write(&resppacket,token);
	writeLE(&resppacket,tokensize);
	write(&resppacket,type);
	if (newvaluelensize==sizeof(byte_t)) {
		write(&resppacket,(byte_t)newvaluelen);
	} else {
		writeLE(&resppacket,(uint32_t)newvaluelen);
	}
	write(&resppacket,newvalue16,newvaluelen);
	write(&resppacket,(byte_t)oldvaluelen);
	write(&resppacket,oldvalue16,oldvaluelen);

	delete[] newvalue16;
	delete[] oldvalue16;
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
					sizeof(byte_t)+
					srvnamelen*sizeof(ucs2_t)+
					sizeof(byte_t)+
					procnamelen*sizeof(ucs2_t)+
					((negotiatedtdsversion<720)?
						sizeof(uint16_t):
						sizeof(uint32_t));

	// truncate the message text so that the token size fits in 16 bits
	size_t		msgtextlen=charstring::getLength(msgtext);
	size_t		maxmsgtextlen=(65535-fixedsize)/sizeof(ucs2_t);
	if (msgtextlen>maxmsgtextlen) {
		msgtextlen=maxmsgtextlen;
	}

	ucs2_t		*msgtext16=ucs2charstring::duplicate(
					msgtext,msgtextlen);
	ucs2_t		*srvname16=ucs2charstring::duplicate(
					servername,srvnamelen);
	ucs2_t		*procname16=ucs2charstring::duplicate(
					procname,procnamelen);

	uint16_t	tokensize=(uint16_t)
				(fixedsize+msgtextlen*sizeof(ucs2_t));

	debugStart((token==TOKEN_INFO)?"info":"error");
	debugWrite("token: 0x%02x",token);
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
	writeLE(&resppacket,tokensize);
	writeLE(&resppacket,number);
	write(&resppacket,state);
	write(&resppacket,infoerrclass);
	writeLE(&resppacket,(uint16_t)msgtextlen);
	write(&resppacket,msgtext16,msgtextlen);
	write(&resppacket,(byte_t)srvnamelen);
	write(&resppacket,srvname16,srvnamelen);
	write(&resppacket,(byte_t)procnamelen);
	write(&resppacket,procname16,procnamelen);
	if (negotiatedtdsversion<720) {
		writeLE(&resppacket,(uint16_t)linenumber);
	} else {
		writeLE(&resppacket,linenumber);
	}
}

bool sqlrprotocol_tds::sendError(uint32_t number,
					byte_t state,
					byte_t errclass,
					const char *msgtext,
					uint32_t linenumber) {
	resppacket.clear();
	appendError(number,state,errclass,msgtext,srvname,NULL,linenumber);
	done(DONE_ERROR,0,0);
	return sendPacket();
}

bool sqlrprotocol_tds::sendUnimplementedFeatureError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,10,"Unimplemented feature",1);
}

bool sqlrprotocol_tds::sendTdsProtocolError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,0,10,"TDS Protocol Error",1);
}

bool sqlrprotocol_tds::sendQueryTooLargeError(size_t querysize) {

	stringbuffer	err;
	err.append("Query too large (");
	err.append((uint64_t)querysize);
	err.append('>');
	err.append(maxquerysize);
	err.append(')');

	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,16,err.getString(),1);
}

bool sqlrprotocol_tds::sendNoCursorAvailableError() {
	// FIXME: is there a real error message/number/state/class for this?
	return sendError(0,1,16,"No cursor available",1);
}

void sqlrprotocol_tds::done() {
	done(DONE_FINAL,0,0);
}

void sqlrprotocol_tds::done(uint16_t status,
				uint16_t curcmd,
				uint64_t donerowcount) {
	done(TOKEN_DONE,status,curcmd,donerowcount);
}

void sqlrprotocol_tds::done(byte_t token,
				uint16_t status,
				uint16_t curcmd,
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
	debugWrite("token: 0x%02x",token);
	debugWrite("status: 0x%02x",status);
	debugWrite("curcmd: 0x%02x",curcmd);
	debugWrite("donerowcount: %lld",donerowcount);
	debugEnd();

	write(&resppacket,token);
	writeLE(&resppacket,status);
	writeLE(&resppacket,curcmd);
	if (negotiatedtdsversion<720) {
		writeLE(&resppacket,(uint32_t)donerowcount);
	} else {
		writeLE(&resppacket,donerowcount);
	}
}

void sqlrprotocol_tds::doneInProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount) {

	// A done-in-proc is always followed by at least the done-proc that
	// closes the rpc, so it is never the last done in a response, and
	// sql server always sets DONE_MORE on one.  Without that bit freetds
	// takes it for the last one, stops reading there, and leaves the rest
	// of the response in its buffer, which puts every command after it
	// one response behind.
	done(TOKEN_DONEINPROC,status|DONE_MORE,curcmd,donerowcount);
}

void sqlrprotocol_tds::returnStatus(uint32_t value) {

	byte_t		token=TOKEN_RETURNSTATUS;

	write(&resppacket,token);
	writeLE(&resppacket,value);

	debugStart("return-status");
	debugWrite("token: 0x%02x",token);
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

	// for each output bind...
	uint16_t	outbindcount=cont->getOutputBindCount(cursor);
	uint16_t	ordinal=1;
	for (uint16_t i=0; i<outbindcount; i++) {

		// the return value went out in the RETURNSTATUS token, so it
		// isn't one of these, and it doesn't take up an ordinal
		if (outbindparams[i]==RPC_RETURN_VALUE_PARAM) {
			continue;
		}

		returnValue(cursor,i,ordinal);
		ordinal++;
	}
}

void sqlrprotocol_tds::returnValueHeader(uint16_t ordinal,
						const char *name,
						uint16_t namesize) {

	byte_t	token=TOKEN_RETURNVALUE;
	write(&resppacket,token);

	// param ordinal
	writeLE(&resppacket,ordinal);

	// param name - a client that looks at it, rather than matching by
	// ordinal, gets an empty name from ct_describe otherwise
	if (name && namesize) {
		ucs2_t	*name16=ucs2charstring::duplicate(name,
							(size_t)namesize);
		write(&resppacket,(byte_t)namesize);
		write(&resppacket,name16,namesize);
		delete[] name16;
		debugWrite("name: %s",name);
	} else {
		write(&resppacket,(byte_t)0);
	}

	// status - 0x01 means it's an output parameter
	write(&resppacket,(byte_t)0x01);

	// user type
	if (negotiatedtdsversion<720) {
		writeLE(&resppacket,(uint16_t)0);
	} else {
		writeLE(&resppacket,(uint32_t)0);
	}

	// Flags.  A real sql server sends none set here, and it matters more
	// than the spec suggests: the ct-lib client reads a return value's
	// user type as 4 bytes whatever the tds version, so it swallows these
	// two as well, and ct_describe reports 65536 rather than 0 for a flags
	// word of 0x0001.
	writeLE(&resppacket,(uint16_t)0x0000);

	debugWrite("ordinal: %d",ordinal);
}

void sqlrprotocol_tds::returnValueInteger(uint16_t ordinal,
						int32_t value,
						bool isnull) {

	debugStart("return-value");

	returnValueHeader(ordinal,NULL,0);

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

void sqlrprotocol_tds::writeIntN(int64_t value, byte_t size) {
	switch (size) {
		case 1:
			write(&resppacket,(byte_t)value);
			break;
		case 2:
			writeLE(&resppacket,(uint16_t)value);
			break;
		case 4:
			writeLE(&resppacket,(uint32_t)value);
			break;
		default:
			writeLE(&resppacket,(uint64_t)value);
			break;
	}
}

void sqlrprotocol_tds::returnValue(sqlrservercursor *cursor,
						uint16_t param,
						uint16_t ordinal) {

	debugStart("return-value");

	sqlrserverbindvar	*bv=&(cont->getOutputBinds(cursor)[param]);

	// the rpc parameter this output bind came from carries the name and
	// the type the client declared
	uint16_t	rpcparam=outbindparams[param];

	returnValueHeader(ordinal,rpcparamnames[rpcparam],
					rpcparamnamesizes[rpcparam]);

	// SQL Relay hands back whatever the database put in the output bind.
	// Only integers and strings can come back through this path.
	switch (bv->type) {
		case SQLRSERVERBINDVARTYPE_INTEGER:
			{
			// An integer goes back at the width the client
			// declared, the way a real sql server echoes it.
			// Sending it as an 8 byte INTN instead makes
			// ct_describe report CS_LONG_TYPE for what the client
			// sent as a CS_INT_TYPE.
			byte_t	size=(byte_t)rpcparammaxsizes[rpcparam];
			if (size!=1 && size!=2 && size!=4 && size!=8) {
				size=sizeof(int64_t);
			}
			write(&resppacket,(byte_t)TDS_TYPE_INTN);
			write(&resppacket,size);
			write(&resppacket,size);
			writeIntN(bv->value.integerval,size);
			debugWrite("value: %lld",bv->value.integerval);
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

void sqlrprotocol_tds::doneProc(uint16_t status,
					uint16_t curcmd,
					uint64_t donerowcount) {
	done(TOKEN_DONEPROC,status,curcmd,donerowcount);
}

void sqlrprotocol_tds::debugSystemError() {
	char	*err=error::getErrorString();
	debugWrite("%s",err);
	delete[] err;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrprotocol	*new_sqlrprotocol_tds(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrprotocol_tds(cont,parameters);
	}
}
