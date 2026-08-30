// Copyright (c) 2016  David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/inetsocketclient.h>
#include <rudiments/bytestring.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/snooze.h>
#include <rudiments/error.h>
#include <rudiments/dynamicarray.h>
#include <rudiments/process.h>
#include <rudiments/character.h>
#include <rudiments/sys.h>
#include <rudiments/sha1.h>
#include <rudiments/sha256.h>
#include <rudiments/aes128.h>
#include <rudiments/dh.h>

// NOTE:
// Teradata CLIv2 refers to:
// Teradata Call-Level Interface Version 2 Release 16.10 B035-2418-058K May 2017
//
// Teradata CLIv2 Debug Facility and Wire Protocol refers to:
// https://downloads.teradata.com/connectivity/articles/cliv2-debug-facility-and-wire-protocol (no longer available)
//
// parcel.h refers to:
// /opt/teradata/client/16.20/include/parcel.h
//
// TdgssLibraryConfigFile.xml refers to:
// /opt/teradata/tdat/tdgss/16.20.12.01/etc/TdgssLibraryConfigFile.xml
//
// TdgssUserConfigFile.xml refers to:
// /opt/teradata/tdat/tdgss/site/TdgssUserConfigFile.xml


//#define DEBUG_CLIENT_SEND_RECV 1
//#define DEBUG_PARCEL_END 1


// passthrough modes
//
// enabled - sends all packets through to the backend, requires a "teradata"
//		backend that just opens a socket to the server and implements
//		send() and recv()
//
// disabled - handles the initialHandshake() internally (not fully implemented
//		yet), then afterwards processes and interprets packets and uses
//		the sqlrelay server API to execute SQL and other commands
//
// hybrid - handles the initialHandshake() via the teradata_sidechannel auth
//		module (which internally does passthrough), but then afterwards
//		processes and interprets packets and uses the sqlrelay server
//		API to execute SQL and other commands, the default
enum passthroughmode_t {
	PASSTHROUGHMODE_ENABLED,
	PASSTHROUGHMODE_DISABLED,
	PASSTHROUGHMODE_HYBRID
};


// algorithm names
// see TdgssLibraryConfigFile.xml <LegalValues><AlgorithmName>
#define	ALG_NONE	0
#define	ALG_BLOWFISH	1
#define	ALG_AES		2
#define	ALG_MD5		3
#define	ALG_SHA1	4
#define	ALG_DH		5
#define	ALG_SHA256	6
#define	ALG_SHA512	7
const char	*algstr[]={
	"none",
	"Blowfish",
	"AES",
	"MD5",
	"SHA1",
	"DH",
	"SHA256",
	"SHA512"
};

// confidentiality algorithm modes
// see TdgssLibraryConfigFile.xml <LegalValues><Mode>
#define	CONF_ALG_MODE_NONE	0
#define	CONF_ALG_MODE_CBC	1
#define	CONF_ALG_MODE_CFB	2
#define	CONF_ALG_MODE_ECB	3
#define	CONF_ALG_MODE_OFB	4
#define	CONF_ALG_MODE_GCM	5
#define	CONF_ALG_MODE_CCM	6
#define	CONF_ALG_MODE_CTR	7
const char	*confalgmodestr[]={
	"none",
	"CBC",
	"CFB",
	"ECB",
	"OFB",
	"GCM",
	"CCM",
	"CTR"
};

// confidentiality algorithm padding
// see TdgssLibraryConfigFile.xml <LegalValues><Padding>
#define	CONF_ALG_PADDING_NONE		0
#define	CONF_ALG_PADDING_OAEP		1
#define	CONF_ALG_PADDING_PKCS1		3
#define	CONF_ALG_PADDING_PKCS5		4
#define	CONF_ALG_PADDING_SSL3		5
const char	*confalgpaddingstr[]={
	"none",
	"OAEP with digest and MGF padding",
	"",
	"PKCS1 padding",
	"PKCS5 padding",
	"SSL3 padding"
};

// auth mechanism fields
// see TdgssLibraryConfigFile.xml <LegalValues><MechanismProperties>
// many more are defined, but these are the ones we use
#define MECHCONFIGFIELD_DEFAULT	16
#define MECHCONFIGFIELD_RANK	17

// quality-of-protections
// see TdgssLibraryConfigFile.xml <GlobalQOPs>
#define	QOP_NONE				0
#define	QOP_GLOBAL_QOP_0			1
#define	QOP_GLOBAL_QOP_1			2
#define	QOP_AES128_CBC_PKCS5_SHA1_DH2048	3
#define	QOP_AES192_CBC_PKCS5_SHA1_DH2048	4
#define	QOP_AES256_CBC_PKCS5_SHA1_DH2048	5
#define	QOP_AES128_GCM_PKCS5_SHA2_DH2048	6
#define	QOP_AES192_GCM_PKCS5_SHA2_DH2048	7
#define	QOP_AES256_GCM_PKCS5_SHA2_DH2048	8
#define	QOP_AES128_CCM_PKCS5_SHA2_DH2048	9
#define	QOP_AES192_CCM_PKCS5_SHA2_DH2048	10
#define	QOP_AES256_CCM_PKCS5_SHA2_DH2048	11
#define	QOP_AES128_CTR_PKCS5_SHA2_DH2048	12
#define	QOP_AES192_CTR_PKCS5_SHA2_DH2048	13
#define	QOP_AES256_CTR_PKCS5_SHA2_DH2048	14
const char	*qopstr[]={
	"NONE",
	"GLOBAL_QOP_0",
	"GLOBAL_QOP_1",
	"AES128_CBC_PKCS5_SHA1_DH2048",
	"AES192_CBC_PKCS5_SHA1_DH2048",
	"AES256_CBC_PKCS5_SHA1_DH2048",
	"AES128_GCM_PKCS5_SHA2_DH2048",
	"AES192_GCM_PKCS5_SHA2_DH2048",
	"AES256_GCM_PKCS5_SHA2_DH2048",
	"AES128_CCM_PKCS5_SHA2_DH2048",
	"AES192_CCM_PKCS5_SHA2_DH2048",
	"AES256_CCM_PKCS5_SHA2_DH2048",
	"AES128_CTR_PKCS5_SHA2_DH2048",
	"AES192_CTR_PKCS5_SHA2_DH2048",
	"AES256_CTR_PKCS5_SHA2_DH2048"
};
uint32_t	qopsharedkeysize[]={
	0,
	0, // probably not correct
	0, // probably not correct
	16,
	24,
	32,
	16,
	24,
	32,
	16,
	24,
	32,
	16,
	24,
	32
};


// mechanisms
// see TdgssLibraryConfigFile.xml <Mechanisms>
#define	MECH_NONE	0
#define	MECH_TD1	1
#define	MECH_TD2	2
#define	MECH_KRB5	3
#define	MECH_SPNEGO	4
#define	MECH_LDAP	5
#define	MECH_PROXY	6
#define	MECH_TDNEGO	7
#define	MECH_JWT	8
const char	*mechstr[]={
	"none",
	"td1",
	"td2",
	"krb5",
	"spnego",
	"ldap",
	"proxy",
	"tdnego",
	"jwt"
};
byte_t	td1mechoid[]={
	0x2B, 0x06, 0x01, 0x04, 0x01, 0x81, 0x3F, 0x01,
	0x87, 0x74, 0x01, 0x01, 0x08
};
byte_t	td2mechoid[]={
	0x2B, 0x06, 0x01, 0x04, 0x01, 0x81, 0x3F, 0x01,
	0x87, 0x74, 0x01, 0x01, 0x09
};
byte_t	krb5mechoid[]={
	0x2A, 0x86, 0x48, 0x86, 0xF7, 0x12, 0x01, 0x02,
	0x02
};
byte_t	spnegomechoid[]={
	0x2B, 0x06, 0x01, 0x05, 0x05, 0x02
};
byte_t	ldapmechoid[]={
	0x2B, 0x06, 0x01, 0x04, 0x01, 0x81, 0x3F, 0x01,
	0x87, 0x74, 0x01, 0x14
};
byte_t	proxymechoid[]={
	0x2B, 0x06, 0x01, 0x04, 0x01, 0x81, 0xE0, 0x1A,
	0x04, 0x82, 0x2E, 0x01, 0x02
};
byte_t	tdnegomechoid[]={
	0x2B, 0x06, 0x01, 0x04, 0x01, 0x81, 0xE0, 0x1A,
	0x04, 0x82, 0x2E, 0x01, 0x03
};
byte_t	jwtmechoid[]={
	0x2B, 0x06, 0x01, 0x04, 0x01, 0x81, 0xE0, 0x1A,
	0x04, 0x82, 0x2E, 0x01, 0x04
};


#define LAN_HEADER_SIZE	52

// in an encrypted message the lan header stops after the session number
// (request auth, request number, gateway byte, host character set and spare
// are encrypted along with the parcels and turn up at the front of the
// plaintext, so the values read out of the lan header are garbage and have
// to be re-read - see parseEncryptedLanHeader())
#define ENCRYPTED_LAN_HEADER_SIZE	24


// kinds of messages
// see Teradata CLIv2 Debug Facility and Wire Protocol
#define COPKIND_ASSIGN		1
#define COPKIND_REASSIGN	2
#define COPKIND_CONNECT		3
#define COPKIND_RECONNECT	4
#define COPKIND_START		5
#define COPKIND_CONTINUE	6
#define COPKIND_ABORT		7
#define COPKIND_LOGOFF		8
#define COPKIND_TEST		9
#define COPKIND_CFG		10
#define COPKIND_AUTHMETHODS	11
#define COPKIND_SSOREQ		12
#define COPKIND_ELICITDATA	13
#define COPKIND_DEFAULTCONNECT	254
#define COPKIND_DIRECT		255


// client config fields
// see parcel.h
#define	CLIENTCONFIGFIELD_VERSION		1
#define	CLIENTCONFIGFIELD_GSS_VERSION		2
#define	CLIENTCONFIGFIELD_RECOVERABLE_PROTOCOL	3
#define	CLIENTCONFIGFIELD_CONTROL_DATA		4
#define	CLIENTCONFIGFIELD_REDRIVE		5
#define	CLIENTCONFIGFIELD_SECURITY_POLICY	8
#define	CLIENTCONFIGFIELD_ESS			9
#define	CLIENTCONFIGFIELD_NEGOTIATE_MECH	11

// gateway config fields
// see parcel.h
#define	GWCONFIGFIELD_SSO			1
#define	GWCONFIGFIELD_GSS_VERSION		2
#define	GWCONFIGFIELD_UTF			3
#define	GWCONFIGFIELD_SESSION_ID		4
#define	GWCONFIGFIELD_RECOVERABLE_PROTOCOL	5
#define	GWCONFIGFIELD_CONTROL_DATA		6
#define	GWCONFIGFIELD_REDRIVE			7
#define	GWCONFIGFIELD_SECURITY_POLICY		10
#define	GWCONFIGFIELD_NEGOTIATE_MECH		12


// sso authdata fields
// no known reference (just had to study the trace)
#define	SSO_GSS_DATA_VERSION_1	0x01
#define	SSO_GSS_DATA_VERSION_3	0x03

#define	SSO_GSS_CLASS_1		0x01
#define	SSO_GSS_CLASS_2		0x02
#define	SSO_GSS_CLASS_5		0x05

#define	SSO_GSS_KIND_1		0x01
#define	SSO_GSS_KIND_2		0x02

#define	SSO_GSS_STRUCTURE	0xE0
#define	SSO_GSS_REPLY_STRUCTURE	0xE1

#define	SSO_ALGORITHMS	0xE1
#define	SSO_ALGORITHM	0xE2

#define SSO_MECH	0xC0
#define SSO_C1		0xC1
#define SSO_C2		0xC2
#define SSO_C3		0xC3
#define SSO_C4		0xC4
#define SSO_C5		0xC5
#define SSO_C6		0xC6

#define	SSOREQ_MECH		0x06

#define	SSORESP_NEGOTIATED_QOPS	0xE3
#define	SSORESP_NEGOTIATED_QOP1	0xE4
#define	SSORESP_NEGOTIATED_QOP2	0xE5
#define	SSORESP_NEGOTIATED_QOP3	0xE6
#define	SSORESP_NEGOTIATED_QOP4	0xE7
#define	SSORESP_NEGOTIATED_QOP_COUNT	4

// td2 token header
// (the last 16 bytes of the encrypted data - see decrypt())
//
// no known reference, this came out of the jdbc driver's
// com.teradata.tdgss.jgssp2td2.Td2Token class
#define	TD2TOKEN_VERSION_3	3
#define	TD2TOKEN_TYPE_WRAP	7
#define	TD2TOKEN_TYPE_MIC	8
#define	TD2TOKEN_FLAG_PRIVACY	0x04

#define CONF_ALG		0xD0
#define INT_ALG			0xD1
#define KEX_ALG			0xD2
#define CONF_ALG_MODE		0xD3
#define CONF_ALG_PADDING	0xD4
#define CONF_ALG_KEY_SIZE	0xD5
#define KEX_ALG_KEY_SIZE	0xD6
const char	*algdfieldname[]={
	"conf alg",
	"int alg",
	"kex alg",
	"mode",
	"padding",
	"conf alg key size",
	"kex alg key size"
};


// activity types
// see Teradata CLIv2
#define NOT_AVAILABLE 0
#define SQL_SELECT 1
#define SQL_INSERT 2
#define SQL_UPDATE 3
#define UPDATE__RETRIEVING 4
#define SQL_DELETE 5
#define SQL_CREATE_TABLE 6
#define SQL_ALTER_TABLE 7
#define SQL_CREATE_VIEW 8
#define SQL_CREATE_MACRO 9
#define SQL_DROP_TABLE 10
#define SQL_DROP_VIEW 11
#define SQL_DROP_MACRO 12
#define SQL_DROP_INDEX 13
#define SQL_RENAME_TABLE 14
#define SQL_RENAME_VIEW 15
#define SQL_RENAME_MACRO 16
#define SQL_CREATE_INDEX 17
#define SQL_CREATE_DATABASE 18
#define SQL_CREATE_USER 19
#define SQL_GRANT 20
#define SQL_REVOKE 21
#define GIVE 22
#define SQL_DROP_DATABASE 23
#define SQL_MODIFY_DATABASE 24
#define SQL_DATABASE 25
#define SQL_BEGIN_TRANSACTION 26
#define SQL_END_TRANSACTION 27
#define SQL_ABORT 28
#define SQL_NULL 29
#define SQL_EXECUTE 30
#define SQL_COMMENT_SET 31
#define SQL_COMMENT 32
#define SQL_ECHO 33
#define REPLACE_VIEW 34
#define REPLACE_MACRO 35
#define SQL_CHECKPOINT 36
#define DELETE_JOURNAL 37
#define SQL_ROLLBACK 38
#define RELEASE_LOCK 39
#define HUT_CONFIG 40
#define VERIFYCHECKPOINT 41
#define DUMP_JOURNAL 42
#define DUMP 43
#define RESTORE 44
#define ROLLFORWARD 45
#define SQL_DELETE_DATABASE 46
#define INTERNAL_USE_ONLY_FOR_CRASH_DUMPS1 47
#define INTERNAL_USE_ONLY_FOR_CRASH_DUMPS2 48
#define SQL_SHOW 49
#define SQL_HELP 50
#define BEGIN_LOADING 51
#define CHECK_POINT_LOAD 52
#define END_LOADING 53
#define INSERT 54
#define GRANT_LOGON 55
#define REVOKE_LOGON 56
#define BEGIN_ACCESS_LOG 57
#define END_ACCESS_LOG 58
#define COLLECT_STATISTICS 59
#define DROP_STATISTICS 60
#define SESSION_SET 61
#define BEGIN_MULTILOAD 62
#define MULTILOAD 63
#define EXECUTE_MULTILOAD 64
#define END_MULTILOAD 65
#define RELEASE_MULTILOAD 66
#define MULTILOAD_DELETE 67
#define MULTILOAD_INSERT 68
#define MULTILOAD_UPDATE 69
#define BEGIN_DELETE_MULTILOAD 70
#define DATA_STATUS 71
#define RESERVED_FOR_B1_SECURITY_1 72
#define RESERVED_FOR_B1_SECURITY_2 73
#define BEGIN_EXPORT 74
#define END_EXPORT 75
#define _2PC_VOTE_REQUEST 76
#define _2PC_VOTE_AND_TERMINATE_REQUEST 77
#define _2PC_COMMIT 78
#define _2PC_ABORT 79
#define _2PC_YES_DONE_RESPONSE_TO_VOTE_REQUEST 80
#define OBSOLETE_1 81
#define OBSOLETE_2 82
#define SET_SESSION_RATE 83
#define MONITOR_SESSION_1 84
#define IDENTIFY 85
#define ABORT_SESSION 86
#define SET_RESOURCE_RATE 87
#define OBSOLETE_3 88
#define REVALIDATE_RI_REFERENCES 89
#define ANSI_SQL_COMMIT_WORK 90
#define MONITOR_VIRTUAL_CONFIG 91
#define MONITOR_PHYSICAL_CONFIG 92
#define MONITOR_VIRTUAL_SUMMARY 93
#define MONITOR_PHYSICAL_SUMMARY 94
#define MONITOR_VIRTUAL_RESOURCE 95
#define SQL_CREATE_TRIGGER 97
#define SQL_DROP_TRIGGER 98
#define SQL_RENAME_TRIGGER 99
#define REPLACE_TRIGGER 100
#define SQL_ALTER_TRIGGER 101
#define REPLICATION 102
#define DROP_PROCEDURE 103
#define CREATE_PROCEDURE 104
#define CALL 105
#define SQL_RENAME_PROCEDURE 106
#define REPLACE_PROCEDURE 107
#define SET_SESSION_ACCOUNT 108
#define LOCKING_LOGGER 109
#define MONITOR_SESSION_2 110
#define MONITOR_VERSION 111
#define BEGIN_DATABASE_QUERY_LOG 112
#define END_DATABASE_QUERY_LOG 113
#define SQL_CREATE_ROLE 114
#define SQL_DROP_ROLE 115
#define GRANT_ROLE 116
#define REVOKE_ROLE 117
#define SQL_CREATE_PROFILE 118
#define SQL_MODIFY_PROFILE 119
#define SQL_DROP_PROFILE 120
#define SQL_SET_ROLE 121
#define CREATE_UDF 122
#define REPLACE_UDF 123
#define DROP_UDF 124
#define ALTER_UDF 125
#define RENAME_UDF 126
#define SQL_MERGE_INTO_UPDATES_AND_INSERTS 127
#define SQL_MERGE_INTO_UPDATES_NO_INSERTS 128
#define SQL_MERGE_INTO_ALL_INSERTS_NO_UPDATES 129
#define SQL_ALTER_PROCEDURE 130
#define PM_API_REQUEST_TDWM_ENABLE 131
#define PM_API_REQUEST_TDWM_STATISTICS 132
#define TDWM_PERF_GROUPS 133
#define CREATE_UDT 134
#define DROP_UDT 135
#define ALTER_UDT 136
#define REPLACE_UDT 137
#define SQL_CREATE_METHOD 138
#define ALTER_METHOD 139
#define REPLACE_METHOD 140
#define CREATE_CAST 141
#define REPLACE_CAST 142
#define DROP_CAST 143
#define SQL_CREATE_ORDERING 144
#define REPLACE_ORDERING 145
#define DROP_ORDERING 146
#define SQL_CREATE_TRANSFORM 147
#define REPLACE_TRANSFORM 148
#define SQL_DROP_TRANSFORM 149
#define CREATE_AUTHORIZATION 150
#define DROP_AUTHORIZATION 151
#define CREATE_REPLICATION_GROUP 152
#define ALTER_REPLICATION_GROUP 153
#define DROP_REPLICATION_GROUP 154
#define TDWM_DELETE_REQUEST_CHANGE_STMT 155
#define TDWM_SUMMARY_STMT 156
#define TDWM_DYN_RULE_STMT 157
#define TDWM_DYN_OBJ_STMT 158
#define TDWM_WD_ASSIGNMENT_STMT 159
#define TDWM_DYN_BUILD 160
#define TDWM_LIST_WD_STMT 161
#define SET_SESSION_ISOLATION_LEVEL 162
#define INITIATE_INDEX_ANALYSIS 163
#define REPLACE_AUTH_STMT 164
#define SET_QUERY_BAND_STMT 165
#define LOGGING_ONLINE_ARCHIVE_ON 166
#define LOGGING_ONLINE_ARCHIVE_OFF 167
#define MONITOR_QUERYBAND 168
#define CREATE_COLUMN_CORRELATION 169
#define REPLACE_COLUMN_CORRELATION 170
#define DROP_COLUMN_CORRELATION 171
#define ALTER_COLUMN_CORRELATION 172
#define USER_EVENT_CONTROL 173
#define EVENT_STATUS 174
#define MONITOR_AWT_RESOURCE 175
#define SP_DYNAMIC_RESULT_SET 176
#define CREATE_REPLICATION_RULE_SET 177
#define REPLACE_REPLICATION_RULE_SET 178
#define DROP_REPLICATION_RULE_SET 179
#define CREATE_OPERATOR 180
#define REPLACE_OPERATOR 181
#define RENAME_OPERATOR 182
#define DROP_OPERATOR 183
#define GRANT_CONNECT_THROUGH 184
#define REVOKE_CONNECT_THROUGH 185
#define CREATE_GLOP_SET 186
#define DROP_GLOP_SET 187
#define CREATE_SECURITY_CONSTRAINT 188
#define ALTER_SECURITY_RESTRAINT 189
#define DROP_SECURITY_RESTRAINT 190
#define CREATE_INDEX_TYPE 191
#define DROP_INDEX_TYPE 192
#define REPLACE_INDEX_TYPE 193
#define ALTER_INDEX 194
#define CHECK_WORKLOAD_FOR 195
#define FASTEXPORT_NO_SPOOLING 196
#define CHECK_WORKLOAD_END 197
#define FLUSH_DBQL_CACHE 198
#define TDWMEXCEPTION_PM_API 199
#define TDWMTEST_PM_API 200
#define MONITOR_TDWM_RESORCE_PM_API 201
#define MONITOR_WD_PM_API 202
#define REGISTER_XML_SCHEMA_XSLT_STYLESHEET_XQUERY_MODULE 203
#define CALENDAR_CHANGED 204
#define MONITOR_REQUEST_PM_API 205
#define MERGE_INTO_DELETE 206
#define BEGIN_QUERY_CAPTURE 207
#define END_QUERY_CAPTURE 208
#define SHOW_IN_XML_DB_OBJECT_DML 209
#define GTW_HOSTGROUP_PROPERTIES 210
#define PROXYCONFIG___FROM_CLIENT_WHEN_IT_SUPPORTS_PROXY 211
#define SECUREATTRIBUTE_PARCEL 212
#define VH_FSG_CACHE 213
#define UNITY_SQL_STMT 214
#define ASTER_SUPPORT 215
#define CREATE_ZONE 216
#define ALTER_ZONE 217
#define DROP_ZONE 218
#define CREATE_FORE_SRV 219
#define ALTER_FORE_SRV 220
#define DROP_FOREIGN_SRV 221
#define BEGIN_ISO_LOAD 222
#define CHECKPT_ISO_LOAD 223
#define END_ISO_LOAD 224
#define SET_SESS_LOAD 226
#define GRANT_ZONE 225
#define REVOKE_ZONE 227
#define FOR_VIEWPOINT 228
#define SET_SESS_JSON 229
#define CHECKJI_ACT_TYPE 230
#define CREATE_MAP 231
#define SET_SESSION_UPT 232
#define DROP_MAP 233
#define GRANT_MAP 234
#define REVOKE_MAP 235
#define SESS_DOT_NO  236
#define CREATE_SCHEMA 237
#define DROP_SCHEMA 238
#define DEBUG_FUNCTION 239
#define FOREIGN_SERVER 240


class bindtype {
	public:
		const char	*type;
		uint16_t	typelen;
};

class request {
	public:
		request(uint16_t maxbindcount);
		~request();

		sqlrservercursor	*cur;

		char	requestmode;
		char	function;
		char	selectdata;
		char	continuedcharactersstate;
		char	aphresponse;
		char	returnstatementinfo;
		char	udttransformsoff;
		char	maxdecprec;
		char	identitycolumnretrieval;
		char	dynamicresultsets;
		char	spreturnresult;
		char	periodstructon;
		char	columninfo;
		char	trustedsessions;
		char	multistatementerrors;
		char	arraytransformsoff;
		char	xmlresponseformat;
		char	tasmfastfailreq;

		bool		runstartup;

		uint32_t	querylen;
		const char	*query;

		bool		setposition;

		bool		bindvars;
		bindtype	*bindtypes;
		bool		bindvals;

		uint16_t	activity;
		size_t		activitycountpos;
		uint16_t	activitycountsize;
		uint64_t	activitycount;
		uint32_t	fieldcount;
		uint64_t	currentfield;

		dynamicarray<byte_t>	nibuffer;
		bytebuffer		rowbuffer;

		size_t		parcelsizepos;

		bool		resendrow;

		bool		fudgecommitwork;
		bool		fudgeselect;
};

request::request(uint16_t maxbindcount) {

	cur=NULL;

	requestmode=0;
	function=0;
	selectdata=0;
	continuedcharactersstate=0;
	aphresponse=0;
	returnstatementinfo=0;
	udttransformsoff=0;
	maxdecprec=0;
	identitycolumnretrieval=0;
	dynamicresultsets=0;
	spreturnresult=0;
	periodstructon=0;
	columninfo=0;
	trustedsessions=0;
	multistatementerrors=0;
	arraytransformsoff=0;
	xmlresponseformat=0;
	tasmfastfailreq=0;

	runstartup=false;

	querylen=0;
	query=NULL;

	setposition=false;
	bindvars=false;
	bindtypes=new bindtype[maxbindcount];
	bindvals=false;

	activity=0;
	activitycountpos=0;
	activitycountsize=0;
	activitycount=0;
	fieldcount=0;
	currentfield=0;
	parcelsizepos=0;
	resendrow=0;

	fudgecommitwork=false;
	fudgeselect=false;
}

request::~request() {
	delete[] bindtypes;
}

class SQLRSERVER_DLLSPEC sqlrprotocol_teradata : public sqlrprotocol {
	public:
		sqlrprotocol_teradata(sqlrservercontroller *cont,
							domnode *parameters);
		virtual	~sqlrprotocol_teradata();

		clientsessionexitstatus_t	clientSession(
							filedescriptor *cs);

	private:
		void	init();
		void	free(bool releasecursor=true);
		void	reInit();

		bool	initialHandshake();

		bool	copKindCfg();
		bool	copKindAssign();
		bool	copKindSsoReq();
		bool	copKindConnect();

		bool	copKindReAssign();
		bool	copKindReConnect();
		bool	copKindContinue();
		bool	copKindAbort();
		bool	copKindLogoff();
		bool	copKindTest();
		bool	copKindAuthMethods();
		bool	copKindElicitData();
		bool	copKindDefaultConnect();
		bool	copKindStart();
		bool	copKindDirect();

		bool	recvRequestFromClient();
		bool	sendResponseToClient();

		bool	passthrough();
		bool	forwardClientRequestToBackend();
		bool	recvResponseFromBackend();
		bool	forwardBackendResponseToClient();

		uint16_t	getParcelFlavor(const byte_t *parcel);

		void	parseParcelHeader(const byte_t *parcel,
					uint16_t *flavor,
					uint32_t *datasize,
					const byte_t **parcelout);
		bool	parseClientConfigParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseConfigParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseAssignParcel(const byte_t *parcel,
					const byte_t **parcelout);
		void	confAlg(byte_t val);
		void	intAlg(byte_t val);
		void	kexAlg(byte_t val);
		void	confAlgMode(byte_t val);
		void	confAlgPadding(byte_t val);
		void	confAlgKeySize(byte_t conf, uint16_t val);
		void	kexAlgKeySize(byte_t kex, uint16_t val);
		bool	parseSsoRequestParcel(const byte_t *parcel,
						const byte_t **parcelout);
		bool	parseSsoGssData(const byte_t *ptr,
						const byte_t **ptrout);
		bool	parseSsoClientPublicKey(const byte_t *ptr,
						uint32_t size,
						const byte_t **ptrout);
		bool	parseSsoGssStructure(const byte_t *ptr,
						bool reply,
						const byte_t **ptrout);
		bool	parseMechField(const byte_t *ptr,
						const byte_t **ptrout);
		bool	parseGenericCField(const byte_t *ptr,
						const byte_t **ptrout);
		bool	parseC6Field(const byte_t *ptr,
						const byte_t **ptrout);
		bool	parseSsoQops(const byte_t *ptr,
						uint32_t size,
						const byte_t **ptrout);
		bool	parseSsoMech(const byte_t *ptr,
						const byte_t **ptrout);
		bool	parseSsoMechParameters(const byte_t *ptr,
						const byte_t *end,
						const byte_t **ptrout);
		bool	parseLogonParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseSessionOptionParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseConnectParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseConnectDataParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseClientAttributeParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseSsoUsernameRequestParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseLogoffParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseOptionsParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseGenericReqParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseGenericRunStartupParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseGenericRespParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	isBulkLoadData(const byte_t *parcel);
		bool	parseSetPositionParcel(const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseDataParcel(const byte_t *parcel,
					const byte_t **parcelout);
		void	parseTinyIntBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseSmallIntBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseIntegerBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseBigIntBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseCharBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseVarCharBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseByteBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseVarByteBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseFloatBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseDateBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseTimeBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		void	parseTimestampBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr);
		bool	parseStatementInfoParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseStatementInfoExtensions(
						const byte_t *ext,
						uint32_t extlen);
		bool	parseParameterExtension(const byte_t *ext,
							uint32_t extlen,
							uint16_t ibcount);
		bool	parseStatementInfoEndParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseMultipartIndicDataParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseEndMultipartIndicDataParcel(
					const byte_t *parcel,
					const byte_t **parcelout);
		bool	parseSlobResponseParcel(const byte_t *parcel,
					const byte_t **parcelout);
		void	parseUsing();
		void	translateInsertToSelect();
		bool	prepareQuery();
		bool	executeQuery();
		bool	parseCancelParcel(const byte_t *parcel,
					const byte_t **parcelout);
		void	parseGenericParcels(const byte_t *parcel,
					const byte_t *end);
		bool	parseGenericParcel(const byte_t *parcel,
					const byte_t **parcelout);
		void	appendParcelHeader(uint16_t flavor,
						uint32_t datasize);
		void	appendLargeParcelHeader(uint16_t flavor,
						uint32_t datasize);
		void	appendSmallParcelHeader(uint16_t flavor,
						uint32_t datasize);
		void	appendParcelHeader(uint16_t flavor);
		void	endParcel();
		void	appendConfigResponseParcel();
		void	appendConfigResponseFixedPortion();
		void	appendConfigResponseIFPs();
		void	appendConfigResponseAMPs();
		void	appendConfigResponseCharSets();
		void	appendConfigResponseInDoubt();
		void	appendConfigResponseHasFields();
		void	appendConfigResponseTransactionSemantics();
		void	appendConfigResponseField7();
		void	appendConfigResponseField9();
		void	appendConfigResponseField10();
		void	appendConfigResponseField11();
		void	appendConfigResponseField12();
		void	appendConfigResponseField13();
		void	appendConfigResponseField14();
		void	appendConfigResponseField15();
		void	appendConfigResponseField16();
		void	appendConfigResponseField6();
		void	appendGatewayConfigParcel();
		void	appendHasFields();
		void	appendTd1MechanismParcel();
		void	appendMechOid(byte_t *oid, uint32_t size);
		void	appendDefaultMech();
		void	appendMechRank(uint32_t rank);
		void	appendTd2MechanismParcel();
		void	appendKrb5MechanismParcel();
		void	appendSpnegoMechanismParcel();
		void	appendLdapMechanismParcel();
		void	appendProxyMechanismParcel();
		void	appendTdnegoMechanismParcel();
		void	appendJwtMechanismParcel();
		void	appendLogonFailureParcel(uint16_t code,
						const char *errorstring);
		void	setSessionNumber();
		void	appendAssignResponseParcel();
		void	appendSsoResponseParcel(byte_t trip);
		void	appendSsoTdnegoSet();
		void	appendSsoGssReplyStructure(uint64_t size);
		void	appendSsoGssStructure(uint64_t size);
		void	appendSsoSpnegoSet();
		void	appendSsoLdapSet();
		void	appendSsoTd2Set();
		void	appendSsoMech(const byte_t *mech, size_t mechsize);
		void	appendGenericCField(byte_t field,
						const byte_t *value,
						uint64_t size);
		void	appendC6Field(byte_t mech);
		void	appendSsoGssData(byte_t mech);
		void	appendSsoGssKeys();
		void	appendSsoGssQops();
		void	appendSuccessParcel();
		void	updateActivityCount();
		void	appendStatementStatusParcel(uint32_t statementnumber);
		void	appendStatementStatusParcel();
		void	appendColumnParcels();
		void	appendFieldColumnParcels();
		void	getFieldFormat(bytebuffer *fieldformat, uint16_t col);
		void	appendFieldParcel(const char *data, uint16_t size);
		void	appendDataInfoParcel();
		void	appendStatementInfoParcel();
		void	appendEstimatedProcessingTimeExtension(uint64_t time);
		void	appendEndEstimatedProcessingTimeExtension();
		void	appendQueryExtension(uint16_t col);
		void	appendEndQueryExtension();
		void	appendStatementInfoEndParcel();
		void	appendRowParcels(bool *eors);
		void	backpatchActivityCount();
		void	appendTitleStartParcel();
		void	appendTitleEndParcel();
		void	appendFormatStartParcel();
		void	appendFormatEndParcel();
		void	appendSizeStartParcel();
		void	appendSizeEndParcel();
		void	appendSizeParcel(uint16_t size);
		void	appendRecStartParcel();
		void	appendRecEndParcel();
		void	appendField(uint16_t col, 
					const char *field,
					uint64_t fieldsize,
					bool null);
		void	appendRecordModeField(uint16_t col, 
						const char *field,
						uint64_t fieldsize,
						bool null);
		void	appendIndicatorModeField(uint16_t col, 
						const char *field,
						uint64_t fieldsize,
						bool null);
		void	appendRecordParcel();
		void	appendFailureParcel(const char *errorstring,
						uint16_t errorsize);
		void	appendErrorParcel(const char *errorstring);
		void	appendEndStatementParcel();
		void	appendEndStatementParcel(uint16_t statementnumber);
		void	appendEndRequestParcel();
		void	appendCursorErrorParcel();
		void	appendConnectionErrorParcel();

		void	unexpectedParcel(uint16_t parcelflavor);
		bool	noParcelFound(const byte_t *parcel);
		bool	noParcelFound(const byte_t *parcel,
						const char *expected);

		uint16_t	getActivity();
		bool		activityReturnsResults();
		const char	*getColumnTypeName(uint16_t col);
		uint16_t	getColumnTypeNameSize(uint16_t col);
		uint16_t	getColumnType(uint16_t col);

		void	debugParcelStart(const char *direction,
						const char *flavorname,
						uint16_t parcelflavor,
						uint32_t parceldatasize);
		void	debugParcelEnd(const byte_t *parceldata,
						uint32_t parceldatasize);
		void	debugParcelStart(const char *direction,
						const char *flavorname,
						uint16_t parcelflavor);
		void	debugParcelEnd();
		void	debugExtStart(const char *extname);
		void	debugExtEnd();
		void	debugMech(const byte_t *oid, size_t size);
		void	debugSessionOption(const char *name, char value);

		bool	parseEncryptedLanHeader(const byte_t *ptr,
						const byte_t **ptrout);

		bool	generateEphemeralKeys();
		bool	generateSharedSecret();
		bool	setSharedKey(byte_t qopindex);
		bool	decrypt(const byte_t *encdata,
					uint64_t encdatasize,
					bytebuffer *decdata);
		bool	encrypt(const byte_t *decdata,
					uint64_t decdatasize,
					bytebuffer *encdata);

		passthroughmode_t	passthroughmode;

		// request buffers
		filedescriptor	*clientsock;
		memorypool	*clientreqmessagepool;
		byte_t		*clientreqheader;
		byte_t		*clientreqdata;
		uint32_t	clientreqdatasize;

		// passthrough buffers
		memorypool	*backendreqmessagepool;
		byte_t		*backendreqheader;
		byte_t		*backendreqdata;
		uint32_t	backendreqdatasize;

		// response buffers
		bytebuffer	respheader;
		bytebuffer	respdata;

		// message
		byte_t		messagekind;
		uint32_t	sessionno;
		byte_t		requestauth[8];
		uint32_t	requestno;
		byte_t		gtwbyte;
		byte_t		hostcharset;
		datetime	sadt;
		byte_t		responseauth[8];

		// requests
		request		*req;

		// max message size
		uint32_t	maxmessagesize;

		// auth mechs
		bool		td1enabled;
		bool		td2enabled;
		bool		krb5enabled;
		bool		spnegoenabled;
		bool		ldapenabled;
		bool		proxyenabled;
		bool		tdnegoenabled;
		bool		jwtenabled;

		// encryption
		bool		blowfishsupported;
		bool		aessupported;
		bool		md5supported;
		bool		sha1supported;
		bool		sha256supported;
		bool		sha512supported;
		bool		dhsupported;
		bool		cbcsupported;
		bool		cfbsupported;
		bool		ecbsupported;
		bool		ofbsupported;
		bool		gcmsupported;
		bool		ccmsupported;
		bool		ctrsupported;
		bool		oaepsupported;
		bool		pkcs1supported;
		bool		pkcs5supported;
		bool		ssl3supported;
		bool		aes128supported;
		bool		aes192supported;
		bool		aes256supported;
		bool		dh2048supported;
		uint16_t	negotiatedmech;
		uint16_t	negotiatedqop;
		byte_t		dhp[256];
		byte_t		dhg[256];
		const byte_t	*serverpubkey;
		size_t		serverpubkeysize;
		const byte_t	*serverprivkey;
		size_t		serverprivkeysize;
		byte_t		clientpubkey[256];
		class dh	dh;
		const byte_t	*sharedsecret;
		uint32_t	sharedsecretsize;
		byte_t		salt[16];
		byte_t		sharedkey[32];
		uint32_t	sharedkeysize;
		byte_t		hsharedkey[64];
		uint32_t	hsharedkeysize;
};

sqlrprotocol_teradata::sqlrprotocol_teradata(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrprotocol(cont,parameters) {

	// configure passthrough mode
	debugWrite("passthrough mode - ");
	if (!charstring::compare(
		parameters->getAttributeValue("passthrough"),"enabled")) {

		if (!charstring::compare(cont->getDbType(),"teradata")) {
			passthroughmode=PASSTHROUGHMODE_ENABLED;
			debugWrite("enabled...");
		} else {
			passthroughmode=PASSTHROUGHMODE_HYBRID;
			debugWrite("disabled (db!=teradata)...");
		}

	} else if (!charstring::compare(
		parameters->getAttributeValue("passthrough"),"disabled")) {

		passthroughmode=PASSTHROUGHMODE_DISABLED;
		debugWrite("disabled...");

	} else {
		passthroughmode=PASSTHROUGHMODE_HYBRID;
		debugWrite("hybrid...");
	}

	// request buffers
	clientsock=NULL;
	clientreqmessagepool=new memorypool(1024,1024,10240);
	clientreqheader=NULL;
	clientreqdata=NULL;
	clientreqdatasize=0;

	// backend buffers
	backendreqmessagepool=new memorypool(1024,1024,10240);
	backendreqheader=NULL;
	backendreqdata=NULL;
	backendreqdatasize=0;

	// message
	messagekind=0;
	sessionno=0;
	bytestring::zero(requestauth,sizeof(requestauth));
	requestno=0;
	bytestring::zero(responseauth,sizeof(responseauth));

	// request
	req=NULL;

	// max message size
	maxmessagesize=0;

	// auth mechs
	// (currently, we only support TD2)
	td1enabled=false;
	td2enabled=true;
	krb5enabled=false;
	spnegoenabled=false;
	ldapenabled=false;
	proxyenabled=false;
	tdnegoenabled=false;
	jwtenabled=false;

	// encryption
	blowfishsupported=false;
	aessupported=false;
	md5supported=false;
	sha1supported=false;
	sha256supported=false;
	sha512supported=false;
	dhsupported=false;
	cbcsupported=false;
	cfbsupported=false;
	ecbsupported=false;
	ofbsupported=false;
	gcmsupported=false;
	ccmsupported=false;
	ctrsupported=false;
	oaepsupported=false;
	pkcs1supported=false;
	pkcs5supported=false;
	ssl3supported=false;
	aes128supported=false;
	aes192supported=false;
	aes256supported=false;
	dh2048supported=false;
	negotiatedmech=MECH_NONE;
	negotiatedqop=QOP_NONE;
	byte_t	dhpdefault[]={
		// DHKeyP2048 from Teradata 2 section in:
		// /opt/teradata/tdgss/etc/TdgssLibraryConfigFile.xml
		// or
		// /opt/teradata/tdgss/etc/TdgssUserConfigFile.xml
		// (DH prime modulus ("p"))
		// FIXME: make this configurable
		0x8A, 0xB3, 0xF8, 0x6E, 0x8D, 0x37, 0x4B, 0x78,
		0x2F, 0x31, 0xDA, 0xD5, 0xF2, 0x7D, 0x6A, 0xFD,
		0xA3, 0x01, 0x50, 0xC1, 0x1A, 0x20, 0xCF, 0x63,
		0x46, 0x71, 0x2A, 0xE2, 0xD2, 0xC6, 0xB7, 0x0A,
		0x5B, 0x79, 0xD4, 0x5D, 0x4C, 0x0C, 0x23, 0x2A,
		0x06, 0x5B, 0x20, 0x7B, 0x12, 0x1B, 0x2C, 0x33,
		0xE1, 0x47, 0xB5, 0x98, 0x3C, 0x38, 0xA1, 0x08,
		0x7F, 0x27, 0x27, 0x03, 0xB0, 0xB8, 0x39, 0xCB,
		0xA6, 0xF7, 0x1C, 0x5D, 0x0E, 0xB5, 0x1E, 0xC8,
		0x90, 0x93, 0x4E, 0xAC, 0xF2, 0xC7, 0xDD, 0x2A,
		0x1D, 0xF6, 0xF5, 0x5E, 0x89, 0xB1, 0x45, 0xA0,
		0x35, 0x9D, 0x35, 0xEF, 0x8F, 0xB6, 0xC5, 0x61,
		0xE1, 0x57, 0xB1, 0x3F, 0xF9, 0x27, 0xA3, 0x5E,
		0x69, 0x96, 0x36, 0x48, 0x61, 0x49, 0x02, 0xB1,
		0x03, 0x4E, 0xF7, 0x11, 0x97, 0xF5, 0x45, 0xDE,
		0xF3, 0x23, 0x62, 0x44, 0xEA, 0xDA, 0xE0, 0x68,
		0x9E, 0x62, 0x4C, 0xF1, 0x24, 0x59, 0x53, 0x63,
		0x0A, 0xE0, 0x42, 0xBD, 0x79, 0x7C, 0x40, 0x25,
		0xE3, 0x7C, 0x51, 0xD9, 0xF6, 0xCB, 0xDA, 0x0B,
		0x22, 0x78, 0xFA, 0x7D, 0x5C, 0xA2, 0xD9, 0xCA,
		0x93, 0x0B, 0xE2, 0x96, 0x83, 0x30, 0xC8, 0x11,
		0xA4, 0xBA, 0x4D, 0x08, 0x45, 0x33, 0x3C, 0x0D,
		0x62, 0xE3, 0xEE, 0x74, 0x21, 0x54, 0xF6, 0xB6,
		0x2F, 0x29, 0x51, 0xCD, 0x8C, 0x73, 0xC4, 0x3B,
		0x5A, 0xA1, 0xC7, 0x81, 0x9D, 0xEF, 0x1D, 0x7C,
		0x93, 0x14, 0x41, 0x1E, 0x46, 0x5F, 0x8E, 0x47,
		0x96, 0x66, 0x65, 0x94, 0xAA, 0xDE, 0x0A, 0xEB,
		0x3F, 0x12, 0x56, 0xE5, 0x71, 0x9E, 0x7A, 0xE5,
		0x4D, 0xD3, 0x4F, 0xFD, 0xA9, 0x49, 0x63, 0x4E,
		0x4A, 0x29, 0x3C, 0x5B, 0xC6, 0x0A, 0xF2, 0x58,
		0xBB, 0x9F, 0xE5, 0x58, 0x08, 0x6E, 0x83, 0xB3,
		0xDD, 0x3D, 0x74, 0x91, 0x96, 0x6D, 0xEE, 0x93
	};
	bytestring::copy(dhp,dhpdefault,sizeof(dhpdefault));

	byte_t	dhgdefault[]={
		// DHKeyG2048 from Teradata 2 section in:
		// /opt/teradata/tdgss/etc/TdgssLibraryConfigFile.xml
		// or
		// /opt/teradata/tdgss/etc/TdgssUserConfigFile.xml
		// (DH generator/base ("g"))
		// FIXME: make this configurable
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05
	};
	bytestring::copy(dhg,dhgdefault,sizeof(dhgdefault));

	serverpubkey=NULL;
	serverpubkeysize=0;
	serverprivkey=NULL;
	serverprivkeysize=0;
	sharedsecret=NULL;
	sharedsecretsize=0;
	bytestring::zero(sharedkey,sizeof(sharedkey));
	sharedkeysize=0;
	bytestring::zero(hsharedkey,sizeof(hsharedkey));
	hsharedkeysize=0;

	init();
}

sqlrprotocol_teradata::~sqlrprotocol_teradata() {
	// don't release the cursor here, the controller has already torn
	// down all cursors by the time it gets around to deleting protocol
	// modules, so req->cur would be a dangling pointer by now
	free(false);
	delete clientreqmessagepool;
	delete backendreqmessagepool;
}

void sqlrprotocol_teradata::init() {
}

void sqlrprotocol_teradata::free(bool releasecursor) {

	// release any request left over from a client that disconnected
	// mid-sequence (eg. mid-COPKIND_START), so it isn't leaked
	if (req) {
		if (releasecursor && req->cur) {
			cont->closeResultSet(req->cur);
			cont->release(req->cur);
		}
		delete req;
		req=NULL;
	}

	clientreqmessagepool->clear();
	backendreqmessagepool->clear();
}

void sqlrprotocol_teradata::reInit() {
	free();
	init();
}

clientsessionexitstatus_t sqlrprotocol_teradata::clientSession(
						filedescriptor *cs) {

	debugWrite("starting client session");

	clientsock=cs;
	clientsock->setNaglesAlgorithmEnabled(false);
	clientsock->setSocketReadBufferSize(65536);
	clientsock->setSocketWriteBufferSize(65536);
	clientsock->setReadBufferSize(65536);
	clientsock->setWriteBufferSize(65536);

	reInit();

	clientsessionexitstatus_t	status=CLIENTSESSIONEXITSTATUS_ERROR;

	if (initialHandshake()) {

		// run session-start queries, now that the client is
		// authenticated
		cont->beginSession();

		bool	loop=true;
		do {

			if (!recvRequestFromClient()) {
				status=
				CLIENTSESSIONEXITSTATUS_CLOSED_CONNECTION;
				break;
			}

			switch (messagekind) {
				case COPKIND_REASSIGN:
					loop=copKindReAssign();
					break;
				case COPKIND_RECONNECT:
					loop=copKindReConnect();
					break;
				case COPKIND_CONTINUE:
					loop=copKindContinue();
					break;
				case COPKIND_ABORT:
					loop=copKindAbort();
					break;
				case COPKIND_LOGOFF:
					copKindLogoff();
					loop=false;
					status=
					CLIENTSESSIONEXITSTATUS_ENDED_SESSION;
					break;
				case COPKIND_TEST:
					loop=copKindTest();
					break;
				case COPKIND_AUTHMETHODS:
					loop=copKindAuthMethods();
					break;
				case COPKIND_ELICITDATA:
					loop=copKindElicitData();
					break;
				case COPKIND_DEFAULTCONNECT:
					loop=copKindDefaultConnect();
					break;
				case COPKIND_START:
					loop=copKindStart();
					break;
				case COPKIND_DIRECT:
					loop=copKindDirect();
					break;
				default:
					debugWrite("INVALID MESSAGE");
					break;
			}

		} while (loop);
	}

	cont->closeClientConnection(0);
	cont->endSession();

	return status;
}

bool sqlrprotocol_teradata::initialHandshake() {

	if (passthroughmode==PASSTHROUGHMODE_HYBRID) {

		// FIXME: presumes we're using teradata_sidechannel auth
		sqlrteradatacredentials	cred;
		cred.setClientFileDescriptor(clientsock);
		return cont->auth(&cred);

	} else {

		for (;;) {
			if (!recvRequestFromClient()) {
				return false;
			}

			switch (messagekind) {
				case COPKIND_CFG:
					if (!copKindCfg()) {
						return false;
					}
					break;
				case COPKIND_ASSIGN:
					if (!copKindAssign()) {
						return false;
					}
					break;
				case COPKIND_SSOREQ:
					if (!copKindSsoReq()) {
						return false;
					}
					break;
				case COPKIND_CONNECT:
					return copKindConnect();
				default:
					return false;
			}
		}
	}
}

bool sqlrprotocol_teradata::copKindCfg() {

	// parse request
	debugStart("copkind_cfg");

	// parse parcels
	const byte_t	*parcel=clientreqdata;
	if (!parseClientConfigParcel(parcel,&parcel)) {
		debugEnd();
		return false;
	}

	// The config parcel is optional.  When we do receive it, it's
	// generally empty.  Some clients (JDBC) don't send it at all.
	if (!noParcelFound(parcel,"config (42)")) {
		if (!parseConfigParcel(parcel,&parcel)) {
			debugEnd();
			return false;
		}
	}

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// respond
	respdata.clear();

	appendConfigResponseParcel();
	appendGatewayConfigParcel();

	// send a set of supported mechs
	// (the client will choose one in sso request - trip 0)
	if (td1enabled) {
		appendTd1MechanismParcel();
	}
	if (td2enabled) {
		appendTd2MechanismParcel();
	}
	if (krb5enabled) {
		appendKrb5MechanismParcel();
	}
	if (spnegoenabled) {
		appendSpnegoMechanismParcel();
	}
	if (ldapenabled) {
		appendLdapMechanismParcel();
	}
	if (proxyenabled) {
		appendProxyMechanismParcel();
	}
	if (tdnegoenabled) {
		appendTdnegoMechanismParcel();
	}
	if (jwtenabled) {
		appendJwtMechanismParcel();
	}

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindAssign() {

	// parse request
	debugStart("copkind_assign");

	// parse parcels (in whatever order they occur)
	const byte_t	*parcel=clientreqdata;
	for (;;) {
		if (noParcelFound(parcel)) {
			break;
		}
		uint16_t	flavor=getParcelFlavor(parcel);
		if (flavor==100) {
			if (!parseAssignParcel(parcel,&parcel)) {
				debugEnd();
				return false;
			}
		} else if (flavor==132) {
			if (!parseSsoRequestParcel(parcel,&parcel)) {
				debugEnd();
				return false;
			}
		}
	}

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	if (!generateEphemeralKeys()) {
		debugEnd();
		return false;
	}

	// build and send responses
	respdata.clear();

	setSessionNumber();

	// handle failure to negotiate mech
	if (negotiatedmech==MECH_NONE) {
		appendLogonFailureParcel(
			507,"Requested logon mechanism is not available.");
		debugEnd();
		return sendResponseToClient();
	}
	if (negotiatedqop==QOP_NONE) {
		appendLogonFailureParcel(
			507,"Requested QOP not supported.");
		debugEnd();
		return sendResponseToClient();
	}

	appendAssignResponseParcel();
	appendSsoResponseParcel(1);

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindSsoReq() {

	// parse request
	debugStart("copkind_ssoreq");

	// parse parcels
	const byte_t	*parcel=clientreqdata;
	if (!parseSsoRequestParcel(parcel,&parcel)) {
		debugEnd();
		return false;
	}

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// build response
	respdata.clear();

	appendSsoResponseParcel(3);

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindConnect() {

	// parse request
	debugStart("copkind_connect");

	// the ciphertext starts inside the lan header
	// (see ENCRYPTED_LAN_HEADER_SIZE)
	bytebuffer	encdata;
	encdata.append(clientreqheader+ENCRYPTED_LAN_HEADER_SIZE,
			LAN_HEADER_SIZE-ENCRYPTED_LAN_HEADER_SIZE);
	encdata.append(clientreqdata,clientreqdatasize);

	// decrypt the request
	bytebuffer	decdata;
	if (!decrypt(encdata.getBuffer(),encdata.getSize(),&decdata)) {
		debugEnd();
		return false;
	}

	// parse decrypted request
	// (decrypt() has already stripped the mic and the copy of the
	// td2 token header off of the end)
	const byte_t	*parcel=decdata.getBuffer();
	const byte_t	*end=parcel+decdata.getSize();

	// the rest of the lan header comes first
	if (decdata.getSize()<LAN_HEADER_SIZE-ENCRYPTED_LAN_HEADER_SIZE) {
		debugWrite("decrypted data too small");
		debugEnd();
		return false;
	}
	if (!parseEncryptedLanHeader(parcel,&parcel)) {
		debugEnd();
		return false;
	}

	// parse parcels (in whatever order they occur)
	for (;;) {
		if (parcel>=end) {
			break;
		}
		uint16_t	flavor=getParcelFlavor(parcel);
		switch (flavor) {
			case 36:
				if (!parseLogonParcel(parcel,&parcel)) {
					debugEnd();
					return false;
				}
				break;
			case 114:
				if (!parseSessionOptionParcel(
							parcel,&parcel)) {
					debugEnd();
					return false;
				}
				break;
			case 88:
				if (!parseConnectParcel(parcel,&parcel)) {
					debugEnd();
					return false;
				}
				break;
			case 3:
				if (!parseConnectDataParcel(parcel,&parcel)) {
					debugEnd();
					return false;
				}
				break;
			case 189:
				if (!parseClientAttributeParcel(
							parcel,&parcel)) {
					debugEnd();
					return false;
				}
				break;
			case 136:
				if (!parseSsoUsernameRequestParcel(
							parcel,&parcel)) {
					debugEnd();
					return false;
				}
				break;
			default:
				unexpectedParcel(flavor);
				debugEnd();
				return false;
		}
	}

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// build response
	respdata.clear();

#if 0
	// FIXME: append header (what should it contain?)

	// FIXME: append:
	// * success parcel - 8 (see Teradata CLIv2, page 312)
	// * sso username response parcel - 137 (see Teradata CLIv2, page 314)
	// * end request parcel - 12 (see Teradata CLIv2, page 257)

	// FIXME: append footer (what should it contain?)

	// FIXME: encrypt all of that
#else

	// FIXME: don't just send this...
	byte_t	response[]={
		0x18, 0xe1, 0xaf, 0xc0, 0xa6, 0xe8, 0xad, 0x83,
		0xf7, 0x17, 0xa2, 0xf7, 0x18, 0x18, 0x21, 0xfe,
		0xcb, 0xcd, 0xbc, 0x77, 0x17, 0x25, 0xfe, 0x15,
		0xea, 0x89, 0x78, 0xcf, 0x06, 0xb1, 0x45, 0x0a,
		0xba, 0xd3, 0x64, 0xef, 0x94, 0xfc, 0xd4, 0x83,
		0x3d, 0x1f, 0x7b, 0x8c, 0x8e, 0xa5, 0xaf, 0x06,
		0xda, 0x4d, 0xdc, 0x60, 0x03, 0xe5, 0xb1, 0x43,
		0xde, 0xf0, 0x67, 0x16, 0x3e, 0x23, 0x43, 0x67,
		0x50, 0xbd, 0x87, 0x9b, 0x01, 0x7f, 0x39, 0xcb,
		0xcd, 0xa3, 0xc6, 0x86, 0xa9, 0x7d, 0xb0, 0x60,
		0xcd, 0xb7, 0xfa, 0x9f, 0x81, 0xf8, 0x00, 0xd2,
		0x58, 0x49, 0xc1, 0x72, 0xdd, 0x83, 0xa6, 0x8f,
		0x0e, 0x2a, 0x92, 0x43, 0x57, 0x43, 0x2e, 0x20,
		0xed, 0xd8, 0x4a, 0x10, 0x83, 0xb2, 0x0a, 0x3c,
		0x03, 0x21, 0xbd, 0xa9, 0x11, 0x4c, 0x0d, 0x8c,
		0x9a, 0x72, 0x09, 0xb1, 0x1c, 0x2a, 0x1e, 0x11,
		0xb4, 0x74, 0xc4, 0xc4, 0xe1, 0x13, 0xc0, 0x01,
		0x03, 0xc1, 0x01, 0x07, 0xc2, 0x01, 0x84, 0xc3,
		0x01, 0x00, 0xc4, 0x02, 0x00, 0xaa, 0xc5, 0x01,
		0x01, 0xc2, 0x20, 0x74, 0x72, 0x84, 0x8e, 0x31,
		0x7d, 0x16, 0xf6, 0x10, 0xb1, 0x0e, 0x2f, 0xe1,
		0xdb, 0xad, 0xcb, 0xcb, 0x59, 0x45, 0x00, 0xb8,
		0x74, 0x68, 0x0e, 0x6a, 0x10, 0xf5, 0x42, 0xd3,
		0xaa, 0x79, 0x0f, 0xc3, 0x10, 0x18, 0xf0, 0x18,
		0x3f, 0x6a, 0x5d, 0x5e, 0x2c, 0x5c, 0xb4, 0x38,
		0xc8, 0xd6, 0x3c, 0x91, 0x20
	};
	write(&respdata,response,sizeof(response));
	debugStart("response");
	debugHexDump(response,sizeof(response));
	debugEnd();
#endif

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindReAssign() {

	// parse request
	debugStart("copkind_reassign");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindReConnect() {

	// parse request
	debugStart("copkind_reconnect");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindContinue() {

	// parse request
	debugStart("copkind_continue");

	// parse parcels
	bool		cancel=false;
	const byte_t	*parcel=clientreqdata;
	if (!parseGenericRespParcel(parcel,&parcel)) {
		if (parseCancelParcel(parcel,&parcel)) {
			cancel=true;
		}
	}

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// bail if there's no request in progress
	if (!req) {
		appendConnectionErrorParcel();
		debugEnd();
		return sendResponseToClient();
	}

	// respond
	respdata.clear();
	bool	eors=true;
	if (cancel) {
		appendEndRequestParcel();
	} else {
		appendRowParcels(&eors);
		if (eors) {
			appendEndStatementParcel();
			appendEndRequestParcel();
		}
	}

	// release request, if appropriate
	if (eors) {
		if (req && req->cur) {
			cont->closeResultSet(req->cur);
			debugWrite("releasing request");
			cont->release(req->cur);
		}
		delete req;
		req=NULL;
	}

	debugEnd();

	// return appropriate result
	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindAbort() {

	// parse request
	debugStart("copkind_abort");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindLogoff() {

	// parse request
	debugStart("copkind_logoff");

	// parse parcels
	const byte_t	*parcel=clientreqdata;
	if (!parseLogoffParcel(parcel,&parcel)) {
		debugEnd();
		return false;
	}

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// decrement the request number (for some reason)
	if (requestno==2) {
		requestno=0;
	} else {
		requestno--;
	}

	// respond
	respdata.clear();
	appendSuccessParcel();
	debugEnd();
	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindTest() {

	// parse request
	debugStart("copkind_test");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindAuthMethods() {

	// parse request
	debugStart("copkind_authmethods");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindElicitData() {

	// parse request
	debugStart("copkind_elicitdata");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindDefaultConnect() {

	// parse request
	debugStart("copkind_defaultconnect");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::copKindStart() {

	// parse request
	debugStart("copkind_start");

	// initialize end-of-result-set flag
	bool	eors=true;

	// parse parcels
	bool		retval=true;
	const byte_t	*parcel=clientreqdata;
	if (parseOptionsParcel(parcel,&parcel)) {

		if (!parseGenericReqParcel(parcel,&parcel) &&
			!parseGenericRunStartupParcel(parcel,&parcel)) {
			retval=false;
			goto end;
		}

		parseSetPositionParcel(parcel,&parcel);


		// check for data...

		// If the query contained a USING clause then it will
		// have defined the bind variables and a data parcel
		// will provide the values.
		//
		// Actually, there are even cases where a data parcel is sent
		// even if the query didn't include a USING clause.
		// (eg. the DELETE FROM SYSADMIN.FASTLOG following a fastload)
		// It's not clear what the data is in these cases, but we need
		// to handle the parcel either way.
		parseDataParcel(parcel,&parcel);

		// if the query doesn't contain USING clause then these
		// statement info parcels will define the bind variables
		parseStatementInfoParcel(parcel,&parcel);
		parseStatementInfoEndParcel(parcel,&parcel);
		// ...and these multipart-indic-data parcels
		// will provide the values
		while (parseMultipartIndicDataParcel(parcel,&parcel) &&
			parseEndMultipartIndicDataParcel(parcel,&parcel)) {
			// FIXME: Currently, successive calls to
			// parseMultipartIndicDataParcel just overwrite
			// the bind values with whatever was provided
			// by the most recent call.
			//
			// Instead, multiple calls are supposed to
			// create an array of bind values.
			// (eg. for a bulk insert).
			//
			// A separate status parcel and end-result
			// should be returned for each set of values
			// too.
			//
			// SQL Relay doesn't currently support
			// array-binds though, so there's no good way
			// to implement this.
		}

		parseSlobResponseParcel(parcel,&parcel);

		if (!parseGenericRespParcel(parcel,&parcel)) {
			retval=false;
			goto end;
		}

	} else if (isBulkLoadData(parcel)) {

		if (passthroughmode!=PASSTHROUGHMODE_ENABLED) {

			// handle actual bulk load...

			respdata.clear();

			const byte_t	*parcel=clientreqdata;


			// generate the id
			// FIXME: It's not at all clear how a teradata backend
			// associates fastload sessions.  For now, we'll assume
			// one-fastload-per-client and use the client hostname,
			// but I'm sure this is wrong...
			// query band maybe?
			// (priority)
			char	*hostname=sys::getHostName();

			// join bulk load
			bool	success=cont->bulkLoadJoin(hostname);

			// clean up
			delete[] hostname;

			if (!success) {
				appendConnectionErrorParcel();
				goto end;
			}

			// release any request left over from an overlapping,
			// unfinished start sequence, so its cursor isn't leaked
			if (req) {
				if (req->cur) {
					cont->closeResultSet(req->cur);
					cont->release(req->cur);
				}
				delete req;
			}

			// get a req/cursor so the stuff below will work...
			// (for now, assume that getCursor() succeeds)
			req=new request(cont->getConfig()->getMaxBindCount());
			req->cur=cont->getCursor();

			// bind the data
			for (;;) {
				uint16_t	parcelflavor;
				const byte_t	*parceldata;
				uint32_t	parceldatasize;
				parseParcelHeader(parcel,&parcelflavor,
							&parceldatasize,
							&parceldata);

				if (parcelflavor==3) {
					if (!cont->bulkLoadInputBind(
							parceldata,
							parceldatasize)) {
						appendConnectionErrorParcel();
						goto end;
					}
				} else {
					break;
				}
				parcel=parceldata+parceldatasize;

				debugParcelStart("recv","data",
						parcelflavor,parceldatasize);
				debugParcelEnd();

				req->activitycount++;
			}
			parseGenericRespParcel(parcel,&parcel);

			if (!cont->bulkLoadExecuteQuery()) {
				appendConnectionErrorParcel();
				goto end;
			}

			appendSuccessParcel();
			appendEndRequestParcel();

			goto end;
		}

	} else {
		retval=false;
		goto end;
	}

	// skip everything if passthrough is enabled
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		goto end;
	}

	respdata.clear();

	// get the activity
	req->activity=getActivity();

	if (req->runstartup) {

		// intercept run-startup requests
		appendErrorParcel("No startup string defined for this user.");

	} else if (

		// intercept transaction queries
		req->activity==SQL_BEGIN_TRANSACTION ||
		req->activity==SQL_END_TRANSACTION ||
		req->activity==SQL_ROLLBACK) {

		bool	result=false;
		if (req->activity==SQL_BEGIN_TRANSACTION) {
			result=cont->begin();
		} else if (req->activity==SQL_END_TRANSACTION) {
			result=cont->commit();
		} else if (req->activity==SQL_ROLLBACK) {
			result=cont->rollback();
		}
		if (result) {
			appendStatementStatusParcel();
			appendEndStatementParcel();
			appendEndRequestParcel();
		} else {
			appendConnectionErrorParcel();
		}

	} else if (

		// intercept fastload-begin queries
		req->activity==BEGIN_LOADING) {

		// generate the id
		// FIXME: It's not at all clear how a teradata backend
		// associates fastload sessions.  For now, we'll assume
		// one-fastload-per-client and use the client hostname,
		// but I'm sure this is wrong...
		// query band maybe?
		// (priority)
		char	*hostname=sys::getHostName();

		// copy the query...
		char	*buffer=charstring::duplicate(req->query,req->querylen);

		// get the table name
		char	*table=buffer+14;
		char	*endtable=charstring::findFirst(table," ERRORFILES ");
		if (endtable) {
			*endtable='\0';
		} else {
			// FIXME: error
		}

		// get the first error table
		char	*error1=endtable+12;
		while (*error1 && character::isWhitespace(*error1)) {
			error1++;
		}
		char	*enderror1=error1;
		while (*enderror1 &&
				!character::isWhitespace(*enderror1) &&
				*enderror1!=',') {
			enderror1++;
		}
		if (*enderror1) {
			*enderror1='\0';
		} else {
			// FIXME: error
		}

		// get the second error table
		char	*error2=enderror1+1;
		while (*error2 &&
			(character::isWhitespace(*error2) || *error1==',')) {
			error2++;
		}
		char	*enderror2=error2;
		while (*enderror2 &&
				!character::isWhitespace(*enderror2) &&
				*enderror2!=',') {
			enderror2++;
		}
		if (*enderror2) {
			*enderror2='\0';
		}

		// FIXME: optional NODROP
		uint64_t nodrop=false;

		// FIXME: optional CHECKPOINT <row count>
		// FIXME: optional INDICATORS
		// FIXME: optional DATAENCRYPTION ON|OFF

		// get the max error count
		// FIXME: from where? (priority)
		uint64_t maxerrorcount=25;

		// begin bulk load
		bool	success=cont->bulkLoadBegin(hostname,
							error1,
							error2,
							maxerrorcount,
							!nodrop);

		// clean up
		delete[] hostname;
		delete[] buffer;

		if (success) {

			// statement 1 response
			appendStatementStatusParcel();
			req->nibuffer.clear();
			req->rowbuffer.clear();
			const byte_t unknown1[]={
				0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x30, 0x09, 0x01, 0x00, 0x01, 0x00,
				0x00, 0x00
			};
			write(&req->rowbuffer,unknown1,sizeof(unknown1));
			appendRecordParcel();
			appendEndStatementParcel(1);

			// statement 2 response (for some reason)
			appendStatementStatusParcel();
			req->nibuffer.clear();
			req->rowbuffer.clear();
			const byte_t unknown2[]={
				0x00, 0x00
			};
			write(&req->rowbuffer,unknown2,sizeof(unknown2));
			appendRecordParcel();
			appendEndStatementParcel(2);

			appendEndRequestParcel();
		} else {
			appendConnectionErrorParcel();
		}

	} else if (

		// intercept fastload-checkpoint queries
		req->activity==CHECK_POINT_LOAD) {

		char	*id=charstring::duplicate(req->query+19,
							req->querylen-19);
		bool	success=cont->bulkLoadCheckpoint(id);
		delete[] id;

		if (success) {
			appendStatementStatusParcel();
			appendEndStatementParcel();
			appendEndRequestParcel();
		} else {
			appendConnectionErrorParcel();
		}

	} else if (

		// intercept fastload-end queries
		req->activity==END_LOADING) {

		if (cont->bulkLoadEnd()) {
			appendStatementStatusParcel();
			appendEndStatementParcel();
			appendEndRequestParcel();
		} else {
			appendConnectionErrorParcel();
		}

	} else if (

		// intercept the bulk load query...
		// this will have a using clause (bindvars will be true)
		// but no associated data parcel (bindvals will be false)
		req->bindvars && !req->bindvals) {

		if (cont->bulkLoadPrepareQuery(req->query,
					req->querylen,
					cont->getInputBindCount(req->cur),
					cont->getInputBinds(req->cur))) {
			appendStatementStatusParcel();
			appendEndStatementParcel();
			appendEndRequestParcel();
		} else {
			appendConnectionErrorParcel();
		}

	} else {

		// handle all other queries normally...
		if (req->function=='P' || req->function=='S') {

			if (prepareQuery()) {
				appendStatementStatusParcel();
				if (activityReturnsResults()) {
					appendColumnParcels();
				}
				appendEndStatementParcel();
				appendEndRequestParcel();
			} else {
				appendCursorErrorParcel();
			}

		} else if (req->function=='E' || req->function=='B') {

			if (prepareQuery() && executeQuery()) {

				// FIXME: if we had multiple result sets
				// or did an array-bind, then we need to
				// return sets of these, like:
				// * status, (columns, rows), end-stmt
				// * status, (columns, rows), end-stmt
				// * status, (columns, rows), end-stmt
				// ...
				// * end-req

				appendStatementStatusParcel();
				if (activityReturnsResults()) {
					appendColumnParcels();
					appendRowParcels(&eors);
					backpatchActivityCount();
				}
				if (eors) {
					appendEndStatementParcel();
// FIXME: ODBC doesn't like receiving COMMIT WORK queries but
// Teradata Studio sends one during the initial handshake...
if (req->fudgecommitwork) {
	req->activity=ANSI_SQL_COMMIT_WORK;
	appendStatementStatusParcel(2);
	appendEndStatementParcel(2);
}
					appendEndRequestParcel();
				}
			} else {
				appendCursorErrorParcel();
			}
		}
	}

end:

	// release request, if appropriate
	if (eors) {
		if (req && req->cur) {
			cont->closeResultSet(req->cur);
			debugWrite("releasing request");
			cont->release(req->cur);
		}
		delete req;
		req=NULL;
	}

	debugEnd();

	// return appropriate result
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		return passthrough();
	}
	if (retval) {
		return sendResponseToClient();
	}
	return retval;
}

bool sqlrprotocol_teradata::copKindDirect() {

	// parse request
	debugStart("copkind_direct");

	// FIXME: parse parcels
	debugWrite("...");

	// if passthrough is enabled then just do that
	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugEnd();
		return passthrough();
	}

	// FIXME: build response
	respdata.clear();

	debugEnd();

	return sendResponseToClient();
}

bool sqlrprotocol_teradata::recvRequestFromClient() {

	clientreqmessagepool->clear();

	// receive lan header
	clientreqheader=clientreqmessagepool->allocate(LAN_HEADER_SIZE);
	if (clientsock->read(clientreqheader,LAN_HEADER_SIZE)!=
							LAN_HEADER_SIZE) {
		debugWrite("read header from client failed");
		return false;
	}

	// lan header fields
	byte_t		version;
	// message classes:
	// 0x01 = request?
	// 0x02 = response?
	// 0x81 = encrypted request?
	// 0x82 = encrypted response?
	byte_t		messageclass;
	uint16_t	highordermessagesize;
	byte_t		bytevar;
	uint16_t	wordvar;
	uint16_t	lowordermessagesize;
	uint16_t 	resforexpan[3];
	uint16_t	corrtag[2];
	byte_t		spare[14];

	// copy out values from lan header
	const byte_t	*ptr=clientreqheader;
	read(ptr,&version,&ptr);
	read(ptr,&messageclass,&ptr);
	read(ptr,&messagekind,&ptr);
	readBE(ptr,&highordermessagesize,&ptr);
	read(ptr,&bytevar,&ptr);
	readBE(ptr,&wordvar,&ptr);
	readBE(ptr,&lowordermessagesize,&ptr);
	read(ptr,(byte_t *)resforexpan,sizeof(resforexpan),&ptr);
	read(ptr,(byte_t *)corrtag,sizeof(corrtag),&ptr);
	readBE(ptr,&sessionno,&ptr);
	read(ptr,(byte_t *)requestauth,sizeof(requestauth),&ptr);
	readBE(ptr,&requestno,&ptr);
	read(ptr,&gtwbyte,&ptr);
	read(ptr,&hostcharset,&ptr);
	read(ptr,(byte_t *)spare,sizeof(spare),&ptr);

	clientreqdatasize=(((uint32_t)highordermessagesize)<<16)|
					((uint32_t)lowordermessagesize);

#ifdef DEBUG_CLIENT_SEND_RECV
	debugStart("client recv header");
	debugWrite("version: %d",(int)version);
	debugWrite("class: %d",(int)messageclass);
	debugWrite("kind: %d",(int)messagekind);
	debugWrite("high order message size: %d",(int)highordermessagesize);
	debugWrite("bytevar: %d",(int)bytevar);
	debugWrite("wordvar: %d",(int)wordvar);
	debugWrite("low order message size: %d",(int)lowordermessagesize);
	stringbuffer	b;
	b.safePrint((byte_t *)resforexpan,sizeof(resforexpan));
	debugWrite("res for expan: %s",b.getString());
	b.clear();
	b.safePrint((byte_t *)corrtag,sizeof(corrtag));
	debugWrite("correlation tag: %s",b.getString());
	debugWrite("session no: %d",(int)sessionno);
	debugWrite("request auth: %03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d",
					requestauth[0],requestauth[1],
					requestauth[2],requestauth[3],
					requestauth[4],requestauth[5],
					requestauth[6],requestauth[7]);
	debugWrite("request auth: %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
					requestauth[0],requestauth[1],
					requestauth[2],requestauth[3],
					requestauth[4],requestauth[5],
					requestauth[6],requestauth[7]);
	debugWrite("request no: %d",(int)requestno);
	debugWrite("gateway byte: %d",(int)gtwbyte);
	debugWrite("host charset: %d",(int)hostcharset);
	debugWrite("clientreqdatasize: %d",(int)clientreqdatasize);
	debugHexDump(clientreqheader,LAN_HEADER_SIZE);
	debugEnd();
#endif


	// receive lan data
	clientreqdata=clientreqmessagepool->allocate(clientreqdatasize);
	if (clientsock->read(clientreqdata,clientreqdatasize)!=
						(ssize_t)clientreqdatasize) {
		debugWrite("read data from client failed");
		return false;
	}

#ifdef DEBUG_CLIENT_SEND_RECV
	debugStart("client recv data");
	debugHexDump(clientreqdata,clientreqdatasize);
	debugEnd();
#endif

	return true;
}

bool sqlrprotocol_teradata::parseEncryptedLanHeader(const byte_t *ptr,
							const byte_t **ptrout) {

	// see ENCRYPTED_LAN_HEADER_SIZE

	debugStart("encrypted lan header");

	byte_t	spare[14];
	read(ptr,(byte_t *)requestauth,sizeof(requestauth),&ptr);
	readBE(ptr,&requestno,&ptr);
	read(ptr,&gtwbyte,&ptr);
	read(ptr,&hostcharset,&ptr);
	read(ptr,(byte_t *)spare,sizeof(spare),&ptr);

	debugWrite("request auth: %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
					requestauth[0],requestauth[1],
					requestauth[2],requestauth[3],
					requestauth[4],requestauth[5],
					requestauth[6],requestauth[7]);
	debugWrite("request no: %d",(int)requestno);
	debugWrite("gateway byte: %d",(int)gtwbyte);
	debugWrite("host charset: %d",(int)hostcharset);

	debugEnd();

	*ptrout=ptr;
	return true;
}

bool sqlrprotocol_teradata::sendResponseToClient() {

	// lan header fields
	byte_t		version=3;
	// message classes:
	// 0x01 = request?
	// 0x02 = response?
	// 0x81 = encrypted request?
	// 0x82 = encrypted response?
	byte_t		messageclass=2;
	uint32_t	messagesize=respdata.getSize();
	uint16_t	highordermessagesize=(messagesize>>16);
	// FIXME: There are cfg/assign cases where bytevar should be 8.
	byte_t		bytevar=0;
	uint16_t	wordvar=0;
	uint16_t	lowordermessagesize=((messagesize<<16)>>16);
	uint16_t 	resforexpan[3]={0,0,0};
	uint16_t	corrtag[2]={0,0};

	// calculate response authentication...
	// (if bytes 3-6 are non-zero, otherwise just zero it out)
	uint32_t	*timeptr=(uint32_t *)&requestauth[3];
	uint32_t	time=*timeptr;
	if (time) {

		// FIXME: the below is incorrect for kind == CONNECT

		// [0,1,2]   - always 0
		// [3,4,5,6] - seconds since 1970, little endian
		// [7]       - appears to be a sequence number

		// initialize a datetime from [3,4,5,6]
		sadt.init(leToHost(time));

		// responseauth is:
		// [0] - always 0
		// [1] - always 0
		// [2] - changes every request and appears to be random
		// [3] - changes every request and appears to be random
		// [4] - minute_of_hour%4*64+second_of_minute
		// 		starts at 0,
		// 		increments each second,
		// 		ranges: 0-59, 64-123, 128-187, 192-251
		//
		// if (kind == START) {
		// 	[5] - minute_of_hour/4+176
		// 		starts at 176
		// 		increments when [4] rolls over (every 4 minutes)
		// 		range: 176-...
		// } else {
		// 	[5] - minute_of_hour/4+128
		// 		starts at 128
		// 		increments when [4] rolls over (every 4 minutes)
		// 		range: 128-142
		// }
		//
		// [6] - hour_of_day%8*32+day_of_month
		// 		starts on day of month
		// 		increments by 32 when [5] rolls over (each hour)
		// 		rolls over every 8 hours
		//
		// if (kind == START) {
		//	[7] - 211 + sequence number
		// 		rolls over automatically
		// } else {
		// 	[7] - 255-day_of_month*2+1
		//		(actually, this could be
		//		255-day_of_month*2-1+[7]
		//		because it's incremented by 1
		//		in the ssoreq message)
		// 		decrements by 2 at midnight
		// 		...
		// 		8/21 (233) - 214
		// 		8/22 (234) - 212
		// 		...
		// }
		responseauth[0]=0;
		responseauth[1]=0;
		responseauth[2]=0x72;
		responseauth[3]=0x77;
		responseauth[4]=sadt.getMinute()%4*64+sadt.getSecond();
		if (messagekind==COPKIND_START) {
			responseauth[5]=sadt.getMinute()/4+176;
		} else {
			responseauth[5]=sadt.getMinute()/4+128;
		}
		responseauth[6]=
			sadt.getHour()%8*32+sadt.getDayOfMonth();
		if (messagekind==COPKIND_START) {
			responseauth[7]=211+requestauth[7];
		} else {
			responseauth[7]=255-sadt.getDayOfMonth()*2+1;
		}

	} else {
		bytestring::zero(responseauth,
					sizeof(responseauth));
	}

	gtwbyte=(messagekind==COPKIND_CFG || messagekind==COPKIND_ASSIGN)?5:0;
	hostcharset=0x7F;
	byte_t	spare[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	// build lan header
	respheader.clear();
	write(&respheader,version);
	write(&respheader,messageclass);
	write(&respheader,messagekind);
	writeBE(&respheader,highordermessagesize);
	write(&respheader,bytevar);
	writeBE(&respheader,wordvar);
	writeBE(&respheader,lowordermessagesize);
	write(&respheader,(byte_t *)resforexpan,sizeof(resforexpan));
	write(&respheader,(byte_t *)corrtag,sizeof(corrtag));
	writeBE(&respheader,sessionno);
	write(&respheader,responseauth,sizeof(responseauth));
	// FIXME: requestno, gtwbyte, and hostcharset
	// are set differently for kind == CONNECT
	writeBE(&respheader,requestno);
	write(&respheader,gtwbyte);
	write(&respheader,hostcharset);
	write(&respheader,spare,sizeof(spare));

#ifdef DEBUG_CLIENT_SEND_RECV
	debugStart("client send header");
	debugWrite("version: %d",(int)version);
	debugWrite("class: %d",(int)messageclass);
	debugWrite("kind: %d",(int)messagekind);
	debugWrite("high order message size: %d",(int)highordermessagesize);
	debugWrite("bytevar: %d",(int)bytevar);
	debugWrite("wordvar: %d",(int)wordvar);
	debugWrite("low order message size: %d",(int)lowordermessagesize);
	stringbuffer	b;
	b.safePrint((byte_t *)resforexpan,sizeof(resforexpan));
	debugWrite("res for expan: %s",b.getString());
	b.clear();
	b.safePrint((byte_t *)corrtag,sizeof(corrtag));
	debugWrite("correlation tag: %s",b.getString());
	debugWrite("session no: %d",(int)sessionno);
	debugWrite("response auth: %03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d",
					responseauth[0],responseauth[1],
					responseauth[2],responseauth[3],
					responseauth[4],responseauth[5],
					responseauth[6],responseauth[7]);
	debugWrite("response auth: %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
					responseauth[0],responseauth[1],
					responseauth[2],responseauth[3],
					responseauth[4],responseauth[5],
					responseauth[6],responseauth[7]);
	debugWrite("request no: %d",(int)requestno);
	debugWrite("gateway byte: %d",(int)gtwbyte);
	debugWrite("host charset: %d",(int)hostcharset);
	debugWrite("messagesize: %d",(int)messagesize);
	debugHexDump(respheader.getBuffer(),respheader.getSize());
	debugEnd();
#endif

	// send lan header
	if (clientsock->write(respheader.getBuffer(),
				respheader.getSize())!=
				(ssize_t)respheader.getSize()) {
		debugWrite("write header to client failed");
		return false;
	}

#ifdef DEBUG_CLIENT_SEND_RECV
	debugStart("client send data");
	debugWrite("size: %d",respdata.getSize());
	debugHexDump(respdata.getBuffer(),respdata.getSize());
	debugEnd();
#endif

	if (clientsock->write(respdata.getBuffer(),
				respdata.getSize())!=
				(ssize_t)respdata.getSize()) {
		debugWrite("write data to client failed");
		return false;
	}

	clientsock->flushWriteBuffer(-1,-1);
	return true;
}

bool sqlrprotocol_teradata::passthrough() {
	return forwardClientRequestToBackend() &&
		recvResponseFromBackend() &&
		forwardBackendResponseToClient();
}

bool sqlrprotocol_teradata::forwardClientRequestToBackend() {

	// pass whatever we received from the client through to the backend
	/*debugStart("backend send header");
	debugWrite("size: %d",LAN_HEADER_SIZE);
	debugHexDump(clientreqheader,LAN_HEADER_SIZE);
	debugEnd();
	debugStart("backend send data");
	debugWrite("size: %d",clientreqdatasize);
	debugHexDump(clientreqdata,clientreqdatasize);
	debugEnd();*/
	if (!cont->send(clientreqheader,LAN_HEADER_SIZE)) {
		debugWrite("send client header to backend failed");
		return false;
	}
	if (!cont->send(clientreqdata,clientreqdatasize)) {
		debugWrite("send client data to backend failed");
		return false;
	}
	return true;
}

bool sqlrprotocol_teradata::recvResponseFromBackend() {

	// receive message
	byte_t	*backendreqmessage=NULL;
	size_t	backendreqmessagesize=0;
	if (!cont->recv(&backendreqmessage,&backendreqmessagesize)) {
		debugWrite("recv message from backend failed");
	}

	// parse lan header...
	backendreqheader=backendreqmessage;

	// lan header fields
	byte_t		version;
	byte_t		messageclass;
	uint16_t	highordermessagesize;
	byte_t		bytevar;
	uint16_t	wordvar;
	uint16_t	lowordermessagesize;
	uint16_t 	resforexpan[3];
	uint16_t	corrtag[2];
	uint32_t	sessionno;
	uint32_t	berequestno;
	byte_t		begtwbyte;
	byte_t		behostcharset;
	byte_t		spare[14];

	// copy out values from lan header
	const byte_t	*ptr=backendreqheader;
	read(ptr,&version,&ptr);
	read(ptr,&messageclass,&ptr);
	read(ptr,&messagekind,&ptr);
	readBE(ptr,&highordermessagesize,&ptr);
	read(ptr,&bytevar,&ptr);
	readBE(ptr,&wordvar,&ptr);
	readBE(ptr,&lowordermessagesize,&ptr);
	read(ptr,(byte_t *)resforexpan,sizeof(resforexpan),&ptr);
	read(ptr,(byte_t *)corrtag,sizeof(corrtag),&ptr);
	readBE(ptr,&sessionno,&ptr);
	read(ptr,(byte_t *)responseauth,sizeof(responseauth),&ptr);
	readBE(ptr,&berequestno,&ptr);
	read(ptr,&begtwbyte,&ptr);
	read(ptr,&behostcharset,&ptr);
	read(ptr,(byte_t *)spare,sizeof(spare),&ptr);

	backendreqdatasize=(((uint32_t)highordermessagesize)<<16)|
				((uint32_t)lowordermessagesize);

	debugStart("backend recv header");
	debugWrite("version: %d",(int)version);
	debugWrite("class: %d",(int)messageclass);
	debugWrite("kind: %d",(int)messagekind);
	debugWrite("high order message size: %d",(int)highordermessagesize);
	debugWrite("bytevar: %d",(int)bytevar);
	debugWrite("wordvar: %d",(int)wordvar);
	debugWrite("low order message size: %d",(int)lowordermessagesize);
	stringbuffer	b;
	b.safePrint((byte_t *)resforexpan,sizeof(resforexpan));
	debugWrite("res for expan: %s",b.getString());
	b.clear();
	b.safePrint((byte_t *)corrtag,sizeof(corrtag));
	debugWrite("correlation tag: %s",b.getString());
	debugWrite("session no: %d",(int)sessionno);
	debugWrite("response auth: %03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d",
					responseauth[0],responseauth[1],
					responseauth[2],responseauth[3],
					responseauth[4],responseauth[5],
					responseauth[6],responseauth[7]);
	debugWrite("response auth: %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x",
					responseauth[0],responseauth[1],
					responseauth[2],responseauth[3],
					responseauth[4],responseauth[5],
					responseauth[6],responseauth[7]);
	debugWrite("request no: %d",(int)berequestno);
	debugWrite("gateway byte: %d",(int)begtwbyte);
	debugWrite("host charset: %d",(int)behostcharset);
	debugWrite("backendreqdatasize: %d",(int)backendreqdatasize);
	debugHexDump(backendreqheader,LAN_HEADER_SIZE);
	debugEnd();


	// receive lan data
	backendreqdata=backendreqmessage+LAN_HEADER_SIZE;

	debugStart("backend recv data");
	debugHexDump(backendreqdata,backendreqdatasize);
	if (messagekind!=COPKIND_CONNECT) {
		parseGenericParcels(backendreqdata,
				backendreqdata+backendreqdatasize);
	}
	debugEnd();

	return true;
}

bool sqlrprotocol_teradata::forwardBackendResponseToClient() {

	// send whatever we received from the backend to the client

#ifdef DEBUG_CLIENT_SEND_RECV
	debugStart("client send header");
	debugWrite("size: %d",LAN_HEADER_SIZE);
	debugHexDump(backendreqheader,LAN_HEADER_SIZE);
	debugEnd();
	debugStart("client send data");
	debugWrite("size: %d",backendreqdatasize);
	debugHexDump(backendreqdata,backendreqdatasize);
	debugEnd();
#endif

	if (clientsock->write(backendreqheader,
				LAN_HEADER_SIZE)!=LAN_HEADER_SIZE) {
		debugWrite("clientsock write failed");
		return false;
	}
	if (clientsock->write(backendreqdata,backendreqdatasize)!=
						(ssize_t)backendreqdatasize) {
		debugWrite("clientsock write failed");
		return false;
	}
	clientsock->flushWriteBuffer(-1,-1);
	return true;
}

uint16_t sqlrprotocol_teradata::getParcelFlavor(const byte_t *parcel) {
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,&parceldatasize,&parceldata);
	return parcelflavor;
}

void sqlrprotocol_teradata::parseParcelHeader(const byte_t *parcel,
					uint16_t *flavor,
					uint32_t *datasize,
					const byte_t **parcelout) {
	
	// get the parcel flavor
	const byte_t	*start=parcel;
	read(parcel,flavor,&parcel);
	// if it's invalid, then try the other endianness
	if ((*flavor&0x7fff)>512) {
		parcel=start;
		setProtocolIsBigEndian(!getProtocolIsBigEndian());
		read(parcel,flavor,&parcel);
	}

	// get the total parcel size...
	// * if the leftmost bit of the flavor is 0,
	//   then the size is stored in the 2 bytes following the flavor
	// * if the leftmost bit of the flavor is 0,
	//   then the size is stored in the 4 bytes following 2 unused bytes
	//   after the flavor, and we need to remove the leftmost bit
	// and subtract the size of the flavor and size-bytes themselves to
	// get the size of the parcel data
	*datasize=0;
	if (*flavor&0x8000) {
		*flavor&=0x7fff;
		parcel+=sizeof(uint16_t);
		read(parcel,datasize,&parcel);
		*datasize=*datasize-sizeof(uint16_t)-
					sizeof(uint16_t)-
					sizeof(uint32_t);
	} else {
		uint16_t	temp;
		read(parcel,&temp,&parcel);
		*datasize=temp-sizeof(uint16_t)-sizeof(uint16_t);
	}

	*parcelout=parcel;
}

bool sqlrprotocol_teradata::parseClientConfigParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// see parcel.h  - pclclientconfig_t

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=166) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","client config",parcelflavor,parceldatasize);

	// parse parcel data
	const byte_t	*ptr=parceldata;
	const byte_t	*end=parceldata+parceldatasize;

	uint32_t	hasfields;
	read(ptr,&hasfields,&ptr);
	debugWrite("has fields: %d",hasfields);

	while (ptr!=end) {

		uint16_t	field;
		uint16_t	size;
		read(ptr,&field,&ptr);
		read(ptr,&size,&ptr);

		switch (field) {
			case CLIENTCONFIGFIELD_VERSION:
				debugWrite("version: %.*s",size,ptr);
				break;
			case CLIENTCONFIGFIELD_GSS_VERSION:
				debugWrite("gss version:");
				debugHexDump(ptr,size);
				break;
			case CLIENTCONFIGFIELD_RECOVERABLE_PROTOCOL:
				debugWrite("recoverable protocol: true");
				break;
			case CLIENTCONFIGFIELD_CONTROL_DATA:
				debugWrite("control data: true");
				break;
			case CLIENTCONFIGFIELD_REDRIVE:
				debugWrite("redrive: true");
				break;
			case CLIENTCONFIGFIELD_SECURITY_POLICY:
				debugWrite("security policy level: %d",*ptr);
				break;
			case CLIENTCONFIGFIELD_ESS:
				debugWrite("ess flag: %d",*ptr);
				break;
			case CLIENTCONFIGFIELD_NEGOTIATE_MECH:
				debugWrite("negotiate mech level: %d",*ptr);
				break;
			default:
				debugStart("field: %d",field);
				debugWrite("size: %d",size);
				debugWrite("data:");
				debugHexDump(ptr,size);
				debugEnd();
				break;
		}
		ptr+=size;
	}

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseConfigParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// see parcel.h - PclConfigType

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=42) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","config",parcelflavor,parceldatasize);

	// no parcel data to parse

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseAssignParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 248

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=100) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","assign",parcelflavor,parceldatasize);

	// parse parcel data
	const char	*username=(const char *)parceldata;
	uint32_t	usernamesize=parceldatasize;

	// debug
	debugWrite("username: %.*s",usernamesize,username);
	debugHexDump((const byte_t *)username,usernamesize);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

void sqlrprotocol_teradata::confAlg(byte_t val) {
	switch (val) {
		case ALG_BLOWFISH:
			blowfishsupported=true;
			break;
		case ALG_AES:
			aessupported=true;
			break;
	}
	debugWrite("%s",algstr[val]);
}

void sqlrprotocol_teradata::intAlg(byte_t val) {
	switch (val) {
		case ALG_MD5:
			md5supported=true;
			break;
		case ALG_SHA1:
			sha1supported=true;
			break;
		case ALG_SHA256:
			sha256supported=true;
			break;
		case ALG_SHA512:
			sha512supported=true;
			break;
	}
	debugWrite("%s",algstr[val]);
}

void sqlrprotocol_teradata::kexAlg(byte_t val) {
	switch (val) {
		case ALG_DH:
			dhsupported=true;
			break;
	}
	debugWrite("%s",algstr[val]);
}

void sqlrprotocol_teradata::confAlgMode(byte_t val) {
	switch (val) {
		case CONF_ALG_MODE_CBC:
			cbcsupported=true;
			break;
		case CONF_ALG_MODE_CFB:
			cfbsupported=true;
			break;
		case CONF_ALG_MODE_ECB:
			ecbsupported=true;
			break;
		case CONF_ALG_MODE_OFB:
			ofbsupported=true;
			break;
		case CONF_ALG_MODE_GCM:
			gcmsupported=true;
			break;
		case CONF_ALG_MODE_CCM:
			ccmsupported=true;
			break;
		case CONF_ALG_MODE_CTR:
			ctrsupported=true;
			break;
	}
	debugWrite("%s",confalgmodestr[val]);
}

void sqlrprotocol_teradata::confAlgPadding(byte_t val) {
	switch (val) {
		case CONF_ALG_PADDING_OAEP:
			oaepsupported=true;
			break;
		case CONF_ALG_PADDING_PKCS1:
			pkcs1supported=true;
			break;
		case CONF_ALG_PADDING_PKCS5:
			pkcs5supported=true;
			break;
		case CONF_ALG_PADDING_SSL3:
			ssl3supported=true;
			break;
	}
	debugWrite("%s",confalgpaddingstr[val]);
}

void sqlrprotocol_teradata::confAlgKeySize(byte_t conf, uint16_t val) {
	if (conf==ALG_AES) {
		switch (val) {
			case 128:
				aes128supported=true;
				break;
			case 192:
				aes192supported=true;
				break;
			case 256:
				aes256supported=true;
				break;
		}
	}
	debugWrite("%d",val);
}

void sqlrprotocol_teradata::kexAlgKeySize(byte_t kex, uint16_t val) {
	if (kex==ALG_DH) {
		switch (val) {
			case 2048:
				dh2048supported=true;
		}
	}
	debugWrite("%d",val);
}

bool sqlrprotocol_teradata::parseSsoRequestParcel(const byte_t *parcel,
						const byte_t **parcelout) {

	// see parcel.h - pclssoreq_t

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=132) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","sso request",parcelflavor,parceldatasize);

	// parse parcel data
	const byte_t	*ptr=parceldata;
	const byte_t	*end=parceldata+parceldatasize;

	byte_t		method;
	byte_t		trip;
	uint16_t	authdatalen;

	read(ptr,&method,&ptr);
	read(ptr,&trip,&ptr);
	read(ptr,&authdatalen,&ptr);

	debugWrite("method: %d",method);
	debugWrite("trip: %d",trip);
	debugWrite("auth data len: %d",authdatalen);

	// parse the authdata...

	// no known reference (just had to study the trace)

	// If we get a gss data (0x01) then the client has selected a mech.
	// We'll get a gss data, mainly containing supported qops, followed by
	// the mech oid and mech parameters.  In the subsequent response, we'll
	// get another gss data (0x03) containing the client's shared key.
	//
	// If we get a gss data structure (0xE0) then the client has selected
	// the tdnego or spnego mech and will go through a negotiation process
	// over the next several trips.  The gss data structure will contain
	// nested data about all mechs the client supports, and will be followed
	// by the mech oid and mech parameters of the tdnego/spnego mech.
	// Subsequent responses are somehow involved in further negotiation.
	// I assume that ultimately, when the final mech has been selected,
	// we'll get a gss data (0x01) and follow that flow from there, but I
	// haven't sorted that process out yet.
	//
	// This code can kind-of parse whatever we get, but for now we only
	// support TD2.
	while (ptr!=end) {

		switch (*ptr) {
			case SSO_GSS_DATA_VERSION_1:
			case SSO_GSS_DATA_VERSION_3:
				if (!parseSsoGssData(ptr,&ptr)) {
					*parcelout=parceldata+parceldatasize;
					debugParcelEnd(
						parceldata,
						parceldatasize);
				}
				break;
			case SSO_GSS_STRUCTURE:
				if (!parseSsoGssStructure(ptr,false,&ptr)) {
					*parcelout=parceldata+parceldatasize;
					debugParcelEnd(
						parceldata,
						parceldatasize);
				}
				break;
			case SSO_GSS_REPLY_STRUCTURE:
				if (!parseSsoGssStructure(ptr,true,&ptr)) {
					*parcelout=parceldata+parceldatasize;
					debugParcelEnd(
						parceldata,
						parceldatasize);
				}
				break;
			case SSOREQ_MECH:
				if (!parseSsoMech(ptr,&ptr)) {
					*parcelout=parceldata+parceldatasize;
					debugParcelEnd(
						parceldata,
						parceldatasize);
				}
				break;
			default:
				if (!parseSsoMechParameters(ptr,end,&ptr)) {
					*parcelout=parceldata+parceldatasize;
					debugParcelEnd(
						parceldata,
						parceldatasize);
				}
				break;
		}
	}

	if (trip==2) {

		// now that we have the client public key,
		// we can generate the shared secret
		if (!generateSharedSecret()) {
			*parcelout=parceldata+parceldatasize;
			debugParcelEnd(parceldata,parceldatasize);
			return false;
		}
	}

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseSsoGssData(const byte_t *ptr,
						const byte_t **ptrout) {

	// no known reference (just had to study the trace)
	// (looks similar to packet header)

	// gss data header
	byte_t		version;
	byte_t		messageclass;
	byte_t		messagekind;
	byte_t		flag;

	// size of gss data block sizes + gss data blocks
	uint32_t	datasize;

	byte_t		capabilities[4];
	byte_t		unknown[4];

	read(ptr,&version,&ptr);
	read(ptr,&messageclass,&ptr);
	read(ptr,&messagekind,&ptr);
	read(ptr,&flag,&ptr);
	readBE(ptr,&datasize,&ptr);
	// for capabilities...
	//
	// in sso request parcel (trip 0), we get:
	// td2:
	// 00 00 00 15 from bteq
	// 00 00 00 05 from jdbc
	// other mechs:
	// 00 00 00 1D from bteq
	// 00 00 00 0D from jdbc
	// See notes in appendSsoGssData()
	//
	// in sso request parcel (trip 2), we get:
	// 00 00 00 00 from bteq
	// 00 00 00 00 from jdbc
	read(ptr,capabilities,sizeof(capabilities),&ptr);
	// more capabilities?
	//
	// in sso request parcel (trip 0 and 2), we get:
	// 00 00 00 00 from bteq
	// 02 00 00 00 from jdbc
	read(ptr,unknown,sizeof(unknown),&ptr);

	debugStart("gss data header");
	debugWrite("version: %d",(int)version);
	debugWrite("class: %d",(int)messageclass);
	debugWrite("kind: %d",(int)messagekind);
	debugWrite("flag: %d",(int)flag);
	debugWrite("data size: %d",(int)datasize);
	debugWrite("capabilities:");
	debugHexDump(capabilities,sizeof(capabilities));
	debugWrite("unknown:");
	debugHexDump(unknown,sizeof(unknown));
	debugEnd();

	// bail if we got an unsupported kind
	if (messagekind!=SSO_GSS_KIND_1 &&
		messagekind!=SSO_GSS_KIND_2) {
		debugWrite("unsupported kind");
		*ptrout=ptr;
		return false;
	}

	// bail if we got an unsupported class
	if (messageclass!=SSO_GSS_CLASS_1 &&
		messageclass!=SSO_GSS_CLASS_2 &&
		messageclass!=SSO_GSS_CLASS_5) {
		debugWrite("unsupported class");
		*ptrout=ptr;
		return false;
	}


	// class-1/kind-2 has no gss data block sizes,
	// only a gss data block - the client's public key
	// (we see these in sso request - trip 2 for td2)
	if (messageclass==SSO_GSS_CLASS_1 && messagekind==SSO_GSS_KIND_2) {

		// gss data blocks...
		debugStart("gss data blocks");

		// parse the client public key, if provided
		if (!parseSsoClientPublicKey(ptr,datasize,&ptr)) {
			debugEnd();
			*ptrout=ptr;
			return false;
		}

		debugEnd();
		*ptrout=ptr;
		return true;
	}


	// other class/kind combinations have
	// gss data block sizes and gss data blocks...

	// gss data block sizes...
	byte_t		gssversion[4];
	uint32_t	dhpsize;
	uint32_t	dhgsize;
	uint32_t	publickeysize;
	uint32_t	unknownsize;
	uint32_t	qopssize;
	uint32_t	gssstructuresize;

	read(ptr,gssversion,sizeof(gssversion),&ptr);
	readBE(ptr,&dhpsize,&ptr);
	readBE(ptr,&dhgsize,&ptr);
	readBE(ptr,&publickeysize,&ptr);
	readBE(ptr,&unknownsize,&ptr);
	readBE(ptr,&qopssize,&ptr);
	readBE(ptr,&gssstructuresize,&ptr);

	debugStart("gss data block sizes");
	// in sso request parcel (trip 0), we get:
	// 10 14 0c 01 from bteq
	// 10 00 00 01 from jdbc
	debugWrite("gss version:");
	debugHexDump(gssversion,sizeof(gssversion));
	debugWrite("dh \"p\" size: %d",(int)dhpsize);
	debugWrite("dh \"g\" size: %d",(int)dhgsize);
	debugWrite("public key size: %d",(int)publickeysize);
	debugWrite("unknown size: %d",(int)unknownsize);
	debugWrite("qops size: %d",(int)qopssize);
	debugWrite("gss structure size: %d",(int)gssstructuresize);

	// class-1/2/kind-1
	// (we see these in sso request - trip 0)
	// and class-5/kind-2
	// (we see these in sso request - trip 2 for tdnego)
	// are padded to 80 bytes
	// (this is probably space for the sizes of other data blocks)
	byte_t		pad[36];
	read(ptr,(byte_t *)pad,sizeof(pad),&ptr);
	//debugWrite("padding:");
	//debugHexDump(pad,sizeof(pad));

	debugEnd();

	// gss data blocks...
	debugStart("gss data blocks");

	// parse the client public key, if provided
	if (publickeysize && !parseSsoClientPublicKey(ptr,publickeysize,&ptr)) {
		debugEnd();
		*ptrout=ptr;
		return false;
	}

	// parse whatever this is, if provided
	if (unknownsize) {
		debugWrite("unknown data:");
		debugHexDump(ptr,unknownsize);
		ptr+=unknownsize;
	}

	// parse the qops, if provided
	if (qopssize && !parseSsoQops(ptr,qopssize,&ptr)) {
		debugEnd();
		*ptrout=ptr;
		return false;
	}

	// for version-1, negotiate qop
	if (version==SSO_GSS_DATA_VERSION_1) {

		// currently, we only support dh2048 and
		// QOP_AES128_CBC_PKCS5_SHA1_DH2048
		negotiatedqop=QOP_NONE;
		if (dhsupported && dh2048supported &&
				aessupported && aes128supported &&
				cbcsupported && pkcs5supported &&
				sha1supported) {
			negotiatedqop=QOP_AES128_CBC_PKCS5_SHA1_DH2048;
		}
		debugWrite("negotiated qop: %s",qopstr[negotiatedqop]);
	}

	// parse the gss structure, if provided
	// FIXME: I'm not sure this is really a gss structure, it starts with
	// 0xE0 and contains 0xC and 0xE1 fields, but they don't appear to be
	// the same thing as the stuff in a regular gss structure
	if (gssstructuresize && !parseSsoGssStructure(ptr,false,&ptr)) {
		debugEnd();
		*ptrout=ptr;
		return false;
	}

	debugEnd();
	*ptrout=ptr;
	return true;
}

bool sqlrprotocol_teradata::parseSsoClientPublicKey(const byte_t *ptr,
							uint32_t size,
							const byte_t **ptrout) {

	// bail if size was 0
	if (!size) {
		*ptrout=ptr;
		return true;
	}

	// FIXME: sanity-check the size

	read(ptr,clientpubkey,size,&ptr);

	debugWrite("client public key (%d bytes):",size);
	debugHexDump(clientpubkey,size);

	*ptrout=ptr;
	return true;
}

bool sqlrprotocol_teradata::parseSsoGssStructure(const byte_t *ptr,
						bool reply,
						const byte_t **ptrout) {

	debugStart((reply)?"sso gss reply structure":"sso gss structure");

	// FIXME: how do we use all of this data?

	// get the field
	byte_t	field;
	read(ptr,&field,&ptr);
	if (reply) {
		if (field!=SSO_GSS_REPLY_STRUCTURE) {
			debugWrite("unexpected field: 0x%02x "
				"(expected sso gss reply structure)",field);
			debugEnd();
			*ptrout=ptr;
			return false;
		}
	} else {
		if (field!=SSO_GSS_STRUCTURE) {
			debugWrite("unexpected field: 0x%02x "
				"(expected sso gss structure)",field);
			debugEnd();
			*ptrout=ptr;
			return false;
		}
	}

	// get the BER-encoded size
	uint64_t	size;
	if (!readBerEncInt(ptr,&size,&ptr)) {
		debugWrite("get ber-encoded len failed");
		debugEnd();
		*ptrout=ptr;
		return false;
	}

	// get the end of the data
	const byte_t	*end=ptr+size;
	
	// the data should be composed of a bunch of NEGO fields or possibly
	// another nested SSO_GSS_STRUCTURE/SSO_GSS_REPLY_STRUCTURE
	while (ptr!=end) {

		switch (*ptr) {
			case SSO_GSS_STRUCTURE:
				if (!parseSsoGssStructure(ptr,false,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			// FIXME: if we run into a 0xE1 in here, I'm not sure
			// it's really a gss reply structure.  It has 0xC fields
			// but I don't think they have the same meanings as the
			// 0xC fields in a 0xE0 structure.
			case SSO_GSS_REPLY_STRUCTURE:
				if (!parseSsoGssStructure(ptr,true,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			case SSO_MECH:
				if (!parseMechField(ptr,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			case SSO_C1:
				if (!parseGenericCField(ptr,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			case SSO_C2:
				if (!parseGenericCField(ptr,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			case SSO_C3:
				if (!parseGenericCField(ptr,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			case SSO_C4:
				if (!parseGenericCField(ptr,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			case SSO_C5:
				if (!parseGenericCField(ptr,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			case SSO_C6:
				if (!parseC6Field(ptr,&ptr)) {
					debugEnd();
					*ptrout=ptr;
					return false;
				}
				break;
			default:
				debugWrite("unexpected field: 0x%02x "
					"(expected 0xe0 or 0xc*)",*ptr);
				debugEnd();
				*ptrout=ptr;
				return false;
		}
	}

	debugEnd();

	*ptrout=ptr;
	return true;
}

bool sqlrprotocol_teradata::parseMechField(const byte_t *ptr,
						const byte_t **ptrout) {

	// get field and size
	byte_t	field;
	byte_t	mechoidsize;
	read(ptr,&field,&ptr);
	read(ptr,&mechoidsize,&ptr);
	if (field!=SSOREQ_MECH && field!=SSO_MECH) {
		debugWrite("unexpected field: 0x%02x "
				"(expected ssoreq_mech or sso_mech)",field);
		*ptrout=ptr;
		return false;
	}

	// get the mech oid
	const byte_t	*mechoid=ptr;
	ptr+=mechoidsize;

	// get the mech by oid
	uint8_t	mech=MECH_NONE;
	if (mechoidsize==sizeof(td1mechoid) &&
			!bytestring::compare(mechoid,
				td1mechoid,sizeof(td1mechoid))) {
		mech=MECH_TD1;
	} else if (mechoidsize==sizeof(td2mechoid) &&
			!bytestring::compare(mechoid,
				td2mechoid,sizeof(td2mechoid))) {
		mech=MECH_TD2;
	} else if (mechoidsize==sizeof(krb5mechoid) &&
			!bytestring::compare(mechoid,
				krb5mechoid,sizeof(krb5mechoid))) {
		mech=MECH_KRB5;
	} else if (mechoidsize==sizeof(spnegomechoid) &&
			!bytestring::compare(mechoid,
				spnegomechoid,sizeof(spnegomechoid))) {
		mech=MECH_SPNEGO;
	} else if (mechoidsize==sizeof(ldapmechoid) &&
			!bytestring::compare(mechoid,
				ldapmechoid,sizeof(ldapmechoid))) {
		mech=MECH_LDAP;
	} else if (mechoidsize==sizeof(proxymechoid) &&
			!bytestring::compare(mechoid,
				proxymechoid,sizeof(proxymechoid))) {
		mech=MECH_PROXY;
	} else if (mechoidsize==sizeof(tdnegomechoid) &&
			!bytestring::compare(mechoid,
				tdnegomechoid,sizeof(tdnegomechoid))) {
		mech=MECH_TDNEGO;
	} else if (mechoidsize==sizeof(jwtmechoid) &&
			!bytestring::compare(mechoid,
				jwtmechoid,sizeof(jwtmechoid))) {
		mech=MECH_JWT;
	}

	debugWrite("mech oid (%s):",mechstr[mech]);
	debugHexDump(mechoid,mechoidsize);

	// for SSOREQ_MECH, negotiate mech
	if (field==SSOREQ_MECH) {

		// currently, we only support TD2
		negotiatedmech=MECH_NONE;
		if (mech==MECH_TD2) {
			negotiatedmech=mech;
		}
		debugWrite("negotiated mech: %s",mechstr[mech]);
	}

	*ptrout=ptr;

	return true;
}

bool sqlrprotocol_teradata::parseGenericCField(const byte_t *ptr,
						const byte_t **ptrout) {

	// get field and size
	byte_t	field;
	byte_t	size;
	read(ptr,&field,&ptr);
	read(ptr,&size,&ptr);
	if (field<SSO_C1 || field>SSO_C5) {
		debugWrite("unexpected field: 0x%02x (expected 0xc*)",field);
		*ptrout=ptr;
		return false;
	}

	// get the data
	// FIXME: what are these?
	const byte_t	*data=ptr;
	ptr+=size;

	if (size==1) {
		debugWrite("0x%02x data: 0x%02x",field,*data);
	} else {
		debugWrite("0x%02x data:",field);
		debugHexDump(data,size);
	}

	*ptrout=ptr;

	return true;
}

bool sqlrprotocol_teradata::parseC6Field(const byte_t *ptr,
						const byte_t **ptrout) {
	// get the field
	byte_t	field;
	read(ptr,&field,&ptr);
	if (field<SSO_C6) {
		debugWrite("unexpected field: 0x%02x (expected 0xc6)",field);
		*ptrout=ptr;
		return false;
	}

	// get the BER-encoded size
	uint64_t	size;
	if (!readBerEncInt(ptr,&size,&ptr)) {
		debugWrite("get ber-encoded len failed");
		*ptrout=ptr;
		return false;
	}

	if (!parseSsoGssData(ptr,&ptr)) {
		return false;
	}

	*ptrout=ptr;

	return true;
}

bool sqlrprotocol_teradata::parseSsoQops(const byte_t *ptr,
						uint32_t size,
						const byte_t **ptrout) {

	// the next bit appears to be the supported qop algorithms...
	// (we'll send a set of supported combinations of them
	// in sso response - trip 1)

	debugStart("supported qop algorithms");

	// get field and size
	byte_t	algs;
	byte_t	algssize;
	read(ptr,&algs,&ptr);
	read(ptr,&algssize,&ptr);
	if (algs!=SSO_ALGORITHMS) {
		debugWrite("unexpected field: 0x%02x "
				"(expected sso_algorithms)",algs);
		debugEnd();
		*ptrout=ptr;
		return false;
	}

	// FIXME: sanity check the size

	// parse each supported qop algorithm
	const byte_t	*algsend=ptr+algssize;
	while (ptr<algsend) {

		// FIXME: bail if we run off the end of the parcel

		debugStart("supported qop algorithm");

		// get field and size
		byte_t	alg;
		byte_t	algsize;
		read(ptr,&alg,&ptr);
		read(ptr,&algsize,&ptr);
		if (alg!=SSO_ALGORITHM) {
			debugWrite("unexpected field: 0x%02x "
					"(expected sso_algorithm)",alg);
			debugEnd();
			debugEnd();
			*ptrout=ptr;
			return false;
		}

		// get algorithm details
		const byte_t	*algend=ptr+algsize;
		byte_t		currentconf=ALG_NONE;
		byte_t		currentkex=ALG_NONE;
		while (ptr<algend) {

			// FIXME: bail if we run off
			// the end of the parcel

			// get field and size
			byte_t	algdfield;
			read(ptr,&algdfield,&ptr);
			byte_t	algdsize;
			read(ptr,&algdsize,&ptr);

			// sanity check
			if (algdfield<0xd0 || algdfield>0xd6) {
				debugWrite("unexpected field: 0x%02x "
						"(expected sso_algdfield)",
								algdfield);
				debugEnd();
				debugEnd();
				*ptrout=ptr;
				return false;
			}

			debugWrite("%s: ",algdfieldname[algdfield-0xd0]);

			if (algdsize==1) {

				byte_t	val;
				read(ptr,&val,&ptr);

				switch (algdfield) {
					case CONF_ALG:
						currentconf=val;
						confAlg(val);
						break;
					case INT_ALG:
						intAlg(val);
						break;
					case KEX_ALG:
						currentkex=val;
						kexAlg(val);
						break;
					case CONF_ALG_MODE:
						confAlgMode(val);
						break;
					case CONF_ALG_PADDING:
						confAlgPadding(val);
						break;
				}

			} else if (algdsize==2) {

				// odd that these are always BE
				// but they appear to be
				uint16_t	val;
				readBE(ptr,&val,&ptr);

				switch (algdfield) {
					case CONF_ALG_KEY_SIZE:
						confAlgKeySize(
							currentconf,val);
						break;
					case KEX_ALG_KEY_SIZE:
						kexAlgKeySize(
							currentkex,val);
						break;
				}
			}
		}
		debugEnd();
	}
	debugEnd();

	*ptrout=ptr;

	return true;
}

bool sqlrprotocol_teradata::parseSsoMech(const byte_t *ptr,
						const byte_t **ptrout) {

	// the next bit appears to be the chosen mech...
	// (we sent a set of supported mechs after the gateway
	// config parcel, the client chooses one here)

	return parseMechField(ptr,ptrout);
}

bool sqlrprotocol_teradata::parseSsoMechParameters(const byte_t *ptr,
						const byte_t *end,
						const byte_t **ptrout) {

	// mech parameters
	//
	// .logmech td2 (all platforms)
	// 46  08  00  02  81  00  04  04
	// 04  00  01  00  00  00  1f  01
	//
	// .logmech ldap (all platforms)
	// 46  08  00  01  81  00  03  00
	// 00  00  01  00  00  00  1E  01
	//
	// .logmech tdnego (all platforms)
	// 00  00  00  00  15  01

	const byte_t	*mechparams=ptr;
	uint16_t	mechparamssize=end-mechparams;
	debugWrite("mech parameters:");
	debugHexDump(mechparams,mechparamssize);

	if (mechparamssize==16) {
		bytestring::copy(salt,mechparams,mechparamssize);
	}

	*ptrout=end;
	return true;
}

bool sqlrprotocol_teradata::parseLogonParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// see Teradata CLIv2, page 269

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=36) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","logon",parcelflavor,parceldatasize);

	// parse parcel data
	// FIXME: do something with these...
	//
	// the parcel data is the logon string:
	//   [tdpid/]username[@mech][,password[,'account']]
	// it isn't null-terminated, so scan it by hand
	const char	*ptr=(const char *)parceldata;
	const char	*end=ptr+parceldatasize;

	// tdpid - only present if there's a slash before the first comma
	const char	*tdpid=NULL;
	uint32_t	tdpidsize=0;
	for (const char *p=ptr; p<end && *p!=','; p++) {
		if (*p=='/') {
			tdpid=ptr;
			tdpidsize=p-ptr;
			ptr=p+1;
			break;
		}
	}

	// username - up to the first comma
	const char	*username=ptr;
	while (ptr<end && *ptr!=',') {
		ptr++;
	}
	uint32_t	usernamesize=ptr-username;

	// mech - the client can append one to the username, instead of
	// setting logmech, and it comes across in here when it does
	const char	*mech=NULL;
	uint32_t	mechsize=0;
	for (uint32_t i=0; i<usernamesize; i++) {
		if (username[i]=='@') {
			mech=username+i+1;
			mechsize=usernamesize-i-1;
			usernamesize=i;
			break;
		}
	}

	// password - up to the next comma
	const char	*password=NULL;
	if (ptr<end) {
		ptr++;
		password=ptr;
		while (ptr<end && *ptr!=',') {
			ptr++;
		}
	}

	// account - the rest, single-quoted
	const char	*account=NULL;
	uint32_t	accountsize=0;
	if (ptr<end) {
		ptr++;
		account=ptr;
		accountsize=end-account;
		if (accountsize>1 && *account=='\'' &&
				account[accountsize-1]=='\'') {
			account++;
			accountsize-=2;
		}
	}

	// debug
	if (tdpid) {
		debugWrite("tdpid: %.*s",tdpidsize,tdpid);
	}
	debugWrite("username: %.*s",usernamesize,username);
	if (mech) {
		debugWrite("mech: %.*s",mechsize,mech);
	}
	if (password) {
		debugWrite("password: (hidden)");
	}
	if (account) {
		debugWrite("account: %.*s",accountsize,account);
	}

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseSessionOptionParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// see Teradata CLIv2, page 299

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=114) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","sesion option",parcelflavor,parceldatasize);

	// parse parcel data
	// FIXME: do something with these...
	//
	// (see parcel.h - PclSessOptListType)
	//
	// each option is a single character:
	//   ansi transaction semantics - 'D' server default,
	//                                'T' teradata, 'A' ansi
	//   2pc mode                   - 'Y' or 'N'
	//   fips flag                  - 'N' none,
	//                                '2' fips127-2, '3' fips127-3
	//   date form                  - 'D' server default,
	//                                'I' integer, 'A' ansi
	//
	// a client that predates one of these options just leaves it off
	// the end of the parcel, so default anything that isn't there to 0
	char	options[10];
	bytestring::zero(options,sizeof(options));
	bytestring::copy(options,parceldata,
				(parceldatasize<sizeof(options))?
					parceldatasize:sizeof(options));

	char	ansitran=options[0];
	char	tpcoption=options[1];
	char	fipsflag=options[2];
	char	dateform=options[3];
	char	essflag=options[4];
	char	utilityworkload=options[5];
	char	redrive=options[6];
	char	extendedloadusage=options[7];
	char	rfu9=options[8];
	char	rfu10=options[9];

	// debug
	debugSessionOption("ansi transaction semantics",ansitran);
	debugSessionOption("2pc mode",tpcoption);
	debugSessionOption("fips flag",fipsflag);
	debugSessionOption("date form",dateform);
	debugSessionOption("ess flag",essflag);
	debugSessionOption("utility workload",utilityworkload);
	debugSessionOption("redrive",redrive);
	debugSessionOption("extended load usage",extendedloadusage);
	debugSessionOption("rfu 9",rfu9);
	debugSessionOption("rfu 10",rfu10);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseConnectParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// see Teradata CLIv2, page 251

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=88) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","connect",parcelflavor,parceldatasize);

	// parse parcel data
	// FIXME: do something with these...
	const byte_t	*ptr=parceldata;
	char		partitionname[16];
	byte_t		logonsequencenumber[4];
	uint16_t	function;
	uint16_t	pad;
	read(ptr,partitionname,sizeof(partitionname),&ptr);
	read(ptr,logonsequencenumber,sizeof(logonsequencenumber),&ptr);
	readBE(ptr,&function,&ptr);
	readBE(ptr,&pad,&ptr);

	// debug
	debugWrite("partition name: %.*s",
			(int)sizeof(partitionname),partitionname);
	debugWrite("logon sequence number:");
	debugHexDump(logonsequencenumber,sizeof(logonsequencenumber));
	debugWrite("function: %hd",function);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseConnectDataParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 253

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=3) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","connect data",parcelflavor,parceldatasize);

	// parse parcel data
	// FIXME: do something with these...
	const byte_t	*ptr=parceldata;
	const byte_t	*end=ptr+parceldatasize;

	// appears to be a space-delimited string
	char		**list;
	uint64_t	listcount;
	charstring::split((const char *)ptr,end-ptr,
				" ",1,true,&list,&listcount);

	const char	*server=list[0];
	const char	*pid=list[1];
	const char	*osuser=list[2];
	const char	*clientprogram=list[3];
	const char	*unknown1=list[4];
	const char	*unknown2=list[5];

	// debug
	debugWrite("server: %s",server);
	debugWrite("pid: %s",pid);
	debugWrite("os user: %s",osuser);
	debugWrite("client program: %s",clientprogram);
	debugWrite("unknown 1: %s",unknown1);
	debugWrite("unknown 2: %s",unknown2);

	// clean up
	for (uint64_t i=0; i<listcount; i++) {
		delete[] list[i];
	}
	delete[] list;

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseClientAttributeParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// no known reference (just had to study the trace)

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=189) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","client attribute",parcelflavor,parceldatasize);

	// parse parcel data
	const byte_t	*ptr=parceldata;
	const byte_t	*end=ptr+parceldatasize;

	// the string marker appears to depend on whether we're talking BE or LE
	byte_t		isstring=(getProtocolIsBigEndian())?0xbf:0x00;

	while (ptr<end) {

		// FIXME: do something with these...

		uint16_t	field;
		uint16_t	size;
		read(ptr,&field,&ptr);
		read(ptr,&size,&ptr);

		if (field==0x7FFF) {
			debugWrite("field: terminator");
			break;
		}

		const char	*fieldname=NULL;
		switch (field) {
			case 7:
				fieldname="server name";
				break;
			case 9:
				fieldname="os user";
				break;
			case 10:
				fieldname="client identifier";
				break;
			case 11:
				fieldname="os version";
				break;
			case 22:
				fieldname="vm";
				break;
			case 28:
				fieldname="vm type";
				break;
			case 30:
				fieldname="vm config";
				break;
			case 24:
			case 29:
			case 31:
			case 33:
				fieldname="unknown version";
				break;
			case 45:
				fieldname="comm processor name";
				break;
		}

		if (*ptr==isstring) {
			// if the first byte of the value
			// is 0x00(LE) or 0xbf(BE) then the value is a string
			if (fieldname) {
				debugWrite("%s: %.*s",
						fieldname,size-1,ptr+1);
			} else {
				debugWrite("unknown field %hd: %.*s",
						field,size-1,ptr+1);
			}
		} else {
			// otherwise, it's binary
			if (fieldname) {
				debugWrite("%s:",fieldname);
			} else {
				debugWrite("unknown field %hd:",field);
			}
			debugHexDump(ptr,size);
		}
		ptr+=size;
	}

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseSsoUsernameRequestParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// see Teradata CLIv2, page 314

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=136) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","sso username request",
					parcelflavor,parceldatasize);

	// parse parcel data
	// FIXME: do something with this...
	//
	// (see parcel.h - pclusernamereq_t and pclusernamereqEON_t)
	//
	// the original parcel is just a header, the eon variant added
	// a single byte to it
	byte_t	eonresponse=0;
	if (parceldatasize) {
		const byte_t	*ptr=parceldata;
		read(ptr,&eonresponse,&ptr);
	}

	// debug
	debugWrite("eon response: %d",eonresponse);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseLogoffParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 269

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=37) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","logoff",parcelflavor,parceldatasize);

	// no parcel data to parse

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseOptionsParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 277

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=85) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","options",parcelflavor,parceldatasize);

	// release any request left over from an overlapping,
	// unfinished start sequence, so its cursor isn't leaked
	if (req) {
		if (req->cur) {
			cont->closeResultSet(req->cur);
			cont->release(req->cur);
		}
		delete req;
	}

	// get a request
	req=new request(cont->getConfig()->getMaxBindCount());

	// get a cursor
	req->cur=cont->getCursor();
	if (!req->cur) {
		debugWrite("failed to get a cursor");
		debugEnd();
		// FIXME: return an error to the client if this happens
		return false;
	}
	// FIXME: kludgy...
	cont->setInputBindCount(req->cur,0);

	// parse parcel data
	const byte_t	*ptr=parceldata;
	read(ptr,&req->requestmode,&ptr);
	read(ptr,&req->function,&ptr);
	read(ptr,&req->selectdata,&ptr);
	read(ptr,&req->continuedcharactersstate,&ptr);
	read(ptr,&req->aphresponse,&ptr);
	read(ptr,&req->returnstatementinfo,&ptr);
	read(ptr,&req->udttransformsoff,&ptr);
	read(ptr,&req->maxdecprec,&ptr);
	read(ptr,&req->identitycolumnretrieval,&ptr);
	read(ptr,&req->dynamicresultsets,&ptr);
	if (parceldatasize>10) {
		read(ptr,&req->spreturnresult,&ptr);
	}
	if (parceldatasize>11) {
		read(ptr,&req->periodstructon,&ptr);
	}
	if (parceldatasize>12) {
		read(ptr,&req->columninfo,&ptr);
	}
	if (parceldatasize>13) {
		read(ptr,&req->trustedsessions,&ptr);
	}
	if (parceldatasize>14) {
		read(ptr,&req->multistatementerrors,&ptr);
	}
	if (parceldatasize>15) {
		read(ptr,&req->arraytransformsoff,&ptr);
	}
	if (parceldatasize>16) {
		read(ptr,&req->xmlresponseformat,&ptr);
	}
	if (parceldatasize>17) {
		read(ptr,&req->tasmfastfailreq,&ptr);
	}

	// debug
	debugWrite("flavor: %d",parcelflavor);
	debugWrite("data size: %d",parceldatasize);
	debugWrite("cursor id: %d",(req->cur)?req->cur->getId():-1);
	debugWrite("request mode: %c",(req->requestmode)?req->requestmode:'0');
	debugWrite("function: %c",(req->function)?req->function:'0');
	debugWrite("select data: %c",(req->selectdata)?req->selectdata:'0');
	debugWrite("continued characters state: %c",
					(req->continuedcharactersstate)?
					req->continuedcharactersstate:'0');
	debugWrite("aph response: %c",(req->aphresponse)?req->aphresponse:'0');
	debugWrite("return statement info: %c",(req->returnstatementinfo)?
						req->returnstatementinfo:'0');
	debugWrite("UDT transforms off: %c",(req->udttransformsoff)?
						req->udttransformsoff:'0');
	debugWrite("maximum decimal precision: %d",req->maxdecprec);
	debugWrite("identity column retrieval: %c",
					(req->identitycolumnretrieval)?
					req->identitycolumnretrieval:'0');
	debugWrite("dynamic result sets: %c",(req->dynamicresultsets)?
						req->dynamicresultsets:'0');
	debugWrite("sp return result: %d",req->spreturnresult);
	debugWrite("period struct on: %c",(req->periodstructon)?
						req->periodstructon:'0');
	debugWrite("column info: %d",req->columninfo);
	debugWrite("trusted sessions: %c",(req->trustedsessions)?
						req->trustedsessions:'0');
	debugWrite("multi statement errors: %c",(req->multistatementerrors)?
						req->multistatementerrors:'0');
	debugWrite("array transforms off: %c",(req->arraytransformsoff)?
						req->arraytransformsoff:'0');
	debugWrite("xml response format: %c",(req->xmlresponseformat)?
						req->xmlresponseformat:'0');
	debugWrite("tasm fast fail req: %c",(req->tasmfastfailreq)?
						req->tasmfastfailreq:'0');

	// override some values
	if (!req->function) {
		req->function='E';
	}
	if (!req->selectdata) {
		req->selectdata='I';
	}
	if (!req->maxdecprec) {
		req->maxdecprec=15;
	}

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseGenericReqParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// req - see Teradata CLIv2, page 292
	// fmreq - see Teradata CLIv2, page 264
	// indicreq - see Teradata CLIv2, page 266
	// multipartrequest - see Teradata CLIv2, page 274

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=1 && parcelflavor!=13 &&
			parcelflavor!=69 && parcelflavor!=148) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv",
			((parcelflavor==1)?"req":
			((parcelflavor==13)?"fmreq":
			((parcelflavor==69)?"indicreq":"multipartrequest"))),
			parcelflavor,parceldatasize);

	// parse parcel data
	req->querylen=parceldatasize;
	req->query=(char *)parceldata;

	// debug
	debugWrite("raw query size: %d",req->querylen);
	debugWrite("raw query: (%d) %.*s",process::getProcessId(),
						req->querylen,req->query);

	// parse the "using" of the query (if there is one)
	parseUsing();

	// translate insert to select in some cases
	translateInsertToSelect();

// FIXME: ODBC doesn't like receiving COMMIT WORK queries but
// Teradata Studio sends one during the initial handshake...
if (req->querylen>11 && !charstring::compareIgnoringCase(
					req->query+req->querylen-11,
					"COMMIT WORK")) {
	req->querylen-=11;
	req->fudgecommitwork=true;
}

	// debug
	debugWrite("query size: %d",req->querylen);
	debugWrite("query: %.*s",req->querylen,req->query);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

void sqlrprotocol_teradata::parseUsing() {

	// if the query contained a using clause then this data should
	// be bind values corresponding to the variables defined in that
	// clause...

	// bail if the query didn't contain a using clause
	const char	*bv=cont->skipWhitespaceAndComments(req->query);
	if (charstring::compareIgnoringCase(bv,"using",5)) {
		return;
	}

	// skip "using" and whitespace
	bv+=5;
	bv=cont->skipWhitespaceAndComments(bv);

	const char	*queryend=req->query+req->querylen;

	debugStart("bind variables");

	sqlrserverbindvar	*inbinds=cont->getInputBinds(req->cur);
	memorypool		*bindpool=cont->getBindPool(req->cur);
	bindpool->clear();

	const char	*ptr;
	uint16_t	ibcount=0;

	for (;;) {

		// reinit
		sqlrserverbindvar	*inbind=&(inbinds[ibcount]);
		bindtype		*inbindtype=&(req->bindtypes[ibcount]);
		inbind->variablesize=0;
		inbind->valuesize=0;
		inbindtype->typelen=0;

		// skip whitespace
		bv=cont->skipWhitespaceAndComments(bv);
		if (bv==queryend) {
			break;
		}

		// get variable name
		ptr=bv;
		while (bv!=queryend && *bv!='(') {
			inbind->variablesize++;
			bv++;
		}
		inbind->variablesize++;
		inbind->variable=
			(char *)bindpool->allocate(inbind->variablesize+1);
		inbind->variable[0]=cont->getBindFormat()[0];
		charstring::copy(inbind->variable+1,ptr,inbind->variablesize);
		inbind->variable[inbind->variablesize]='\0';
		if (bv==queryend) {
			break;
		}

		// skip (
		bv++;
		if (bv==queryend) {
			break;
		}

		// get type name
		inbindtype->type=bv;
		while (bv!=queryend && *bv!='(' && *bv!=')') {
			inbindtype->typelen++;
			bv++;
		}
		if (bv==queryend) {
			break;
		}

		// translate type name
		// (parseDataParcel also translates the type name, but 
		// for bulk loads, it will never get called, so we have
		// to make sure that the type is populated here)
		if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"TINYINT",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"SMALLINT",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"INTEGER",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"BIGINT",
						inbindtype->typelen)) {
			inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"CHAR",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"VARCHAR",
						inbindtype->typelen)) {
			inbind->type=SQLRSERVERBINDVARTYPE_STRING;
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"BYTE",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"VARBYTE",
						inbindtype->typelen)) {
			inbind->type=SQLRSERVERBINDVARTYPE_BLOB;
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"DECIMAL",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"NUMBER",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"FLOAT",
						inbindtype->typelen)) {
			inbind->type=SQLRSERVERBINDVARTYPE_DOUBLE;
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"DATE",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"TIME",
						inbindtype->typelen) ||
			!charstring::compareIgnoringCase(
						inbindtype->type,
						"TIMESTAMP",
						inbindtype->typelen)) {
			inbind->type=SQLRSERVERBINDVARTYPE_DATE;
			inbind->value.dateval.tz=NULL;
		}

		// override some types...
		if (inbind->variablesize>=6 &&
				!charstring::compareIgnoringCase(
					inbind->variable+1,"DELIM",5)) {
			inbind->type=SQLRSERVERBINDVARTYPE_DELIMITER;
		} else if (inbind->variablesize>=12 &&
				!charstring::compareIgnoringCase(
					inbind->variable+1,"NEWLINECHAR",11)) {
			inbind->type=SQLRSERVERBINDVARTYPE_NEWLINE;
		}

		// get size
		if (*bv=='(') {

			// skip (
			bv++;
			if (bv==queryend) {
				break;
			}

			inbind->valuesize=charstring::convertToInteger(bv);

			while (bv!=queryend && *bv!=')') {
				bv++;
			}
			if (bv==queryend) {
				break;
			}

			// skip )
			bv++;
			if (bv==queryend) {
				break;
			}
		}

		// skip )
		bv++;
		if (bv==queryend) {
			break;
		}

		// debug
		debugWrite("%s","");
		if (inbind->valuesize) {
			debugWrite("%d: %s(%.*s(%d))",
						ibcount,
						inbind->variable,
						inbindtype->typelen,
						inbindtype->type,
						inbind->valuesize);
		} else {
			debugWrite("%d: %s(%.*s)",
						ibcount,
						inbind->variable,
						inbindtype->typelen,
						inbindtype->type);
		}

		// bump bind count
		ibcount++;

		// bail if there are no more variables
		if (*bv!=',') {
			break;
		}

		// skip ,
		bv++;
		if (bv==queryend) {
			break;
		}
	}

	debugEnd();

	// we have bind variables
	req->bindvars=true;

	// set input bind count
	cont->setInputBindCount(req->cur,ibcount);

	// move query pointer to after using clause
	// (and update querylen accordingly)
	bv=cont->skipWhitespaceAndComments(bv);
	req->querylen-=(bv-req->query);
	req->query=bv;
}

void sqlrprotocol_teradata::translateInsertToSelect() {
}

bool sqlrprotocol_teradata::parseGenericRunStartupParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// runstartup - see Teradata CLIv2, page 299
	// fmrunstartup - see Teradata CLIv2, page 264

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=2 && parcelflavor!=14) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv",
			(parcelflavor==2)?"runstartup":"fmrunstartup",
			parcelflavor,parceldatasize);

	// no parcel data to parse
	req->runstartup=true;

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseGenericRespParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {

	// resp - see Teradata CLIv2, page 293
	// bigpresp - see Teradata CLIv2, page 362
	// bigkeepresp - see Teradata CLIv2, page 362

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=4 && parcelflavor!=153 && parcelflavor!=154) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv",
			(parcelflavor==4)?"resp":
			((parcelflavor==153)?"bigresp":"bigkeepresp"),
			parcelflavor,parceldatasize);

	// parse parcel data...
	if (parcelflavor==4) {
		maxmessagesize=leToHost(*((uint16_t *)parceldata));
	} else {
		maxmessagesize=leToHost(*((uint32_t *)parceldata));
	}

	// debug
	debugWrite("max message size: %d",maxmessagesize);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::isBulkLoadData(const byte_t *parcel) {

	// if we find a data parcel, then (in the context that this
	// method is called) the message must be data for a bulk load
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	return (parcelflavor==3);
}

bool sqlrprotocol_teradata::parseSetPositionParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 301

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=157) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","setposition",parcelflavor,parceldatasize);

	// we may want to set the position later
	req->setposition=true;

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd();

	return true;
}

bool sqlrprotocol_teradata::parseDataParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 253

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=3) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","data",parcelflavor,parceldatasize);

	if (passthroughmode==PASSTHROUGHMODE_ENABLED) {
		debugHexDump(parceldata,parceldatasize);
		debugParcelEnd();
		*parcelout=parceldata+parceldatasize;
		return true;
	}

	// parse bind values
	const byte_t	*bd=parceldata;

	sqlrserverbindvar	*inbinds=cont->getInputBinds(req->cur);
	uint16_t		inbindcount=cont->getInputBindCount(req->cur);

	debugStart("bind values");

	for (uint16_t count=0; count<inbindcount; count++) {

		sqlrserverbindvar	*inbind=&(inbinds[count]);
		bindtype		*inbindtype=&(req->bindtypes[count]);

		// debug
		debugWrite("%s","");
		if (inbind->valuesize) {
			debugWrite("%s(%.*s(%d)) = ",inbind->variable,
							inbindtype->typelen,
							inbindtype->type,
							inbind->valuesize);
		} else {
			debugWrite("%s(%.*s) = ",inbind->variable,
							inbindtype->typelen,
							inbindtype->type);
		}

		// get value from data
		inbind->isnull=cont->getNonNullBindValue();
		if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"TINYINT",
						inbindtype->typelen)) {
			parseTinyIntBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"SMALLINT",
						inbindtype->typelen)) {
			parseSmallIntBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"INTEGER",
						inbindtype->typelen)) {
			parseIntegerBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"BIGINT",
						inbindtype->typelen)) {
			parseBigIntBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"CHAR",
						inbindtype->typelen)) {
			parseCharBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"VARCHAR",
						inbindtype->typelen)) {
			parseVarCharBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"BYTE",
						inbindtype->typelen)) {
			parseByteBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"VARBYTE",
						inbindtype->typelen)) {
			parseVarByteBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"DECIMAL",
						inbindtype->typelen)) {
			// FIXME: support this
			// no obvious way to test this, ODBC maybe?
			inbind->type=SQLRSERVERBINDVARTYPE_DOUBLE;
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"NUMBER",
						inbindtype->typelen)) {
			// FIXME: support this
			// no obvious way to test this, ODBC maybe?
			inbind->type=SQLRSERVERBINDVARTYPE_DOUBLE;
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"FLOAT",
						inbindtype->typelen)) {
			// FIXME: no obvious way to test this, ODBC maybe?
			parseFloatBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"DATE",
						inbindtype->typelen)) {
			// FIXME: no obvious way to test this, ODBC maybe?
			parseDateBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"TIME",
						inbindtype->typelen)) {
			// FIXME: no obvious way to test this, ODBC maybe?
			parseTimeBind(bd,inbind,&bd);
		} else if (!charstring::compareIgnoringCase(
						inbindtype->type,
						"TIMESTAMP",
						inbindtype->typelen)) {
			// FIXME: no obvious way to test this, ODBC maybe?
			parseTimestampBind(bd,inbind,&bd);
		}
	}

	debugEnd();

	// we have bind values
	req->bindvals=true;

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd();

	return true;
}

void sqlrprotocol_teradata::parseTinyIntBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	char	val;
	read(ptr,&val,outptr);
	inbind->value.integerval=val;
	inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
	debugWrite("%d",val);
}

void sqlrprotocol_teradata::parseSmallIntBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	uint16_t	val;
	read(ptr,&val,outptr);
	inbind->value.integerval=val;
	inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
	debugWrite("%d",val);
}

void sqlrprotocol_teradata::parseIntegerBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	int32_t	val;
	read(ptr,(uint32_t *)&val,outptr);
	inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
	inbind->value.integerval=val;
	debugWrite("%d",val);
}

void sqlrprotocol_teradata::parseBigIntBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	int64_t	val;
	read(ptr,(uint64_t *)&val,outptr);
	inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
	inbind->value.integerval=val;
	debugWrite("%lld",(long long)val);
}

void sqlrprotocol_teradata::parseCharBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	char	*val=(char *)ptr;
	*outptr=ptr+inbind->valuesize;
	inbind->type=SQLRSERVERBINDVARTYPE_STRING;
	inbind->value.stringval=val;
	debugWrite("%.*s",inbind->valuesize,val);
}

void sqlrprotocol_teradata::parseVarCharBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	uint16_t	len;
	read(ptr,&len,&ptr);
	char	*val=(char *)ptr;
	*outptr=ptr+len;
	inbind->type=SQLRSERVERBINDVARTYPE_STRING;
	inbind->valuesize=len;
	inbind->value.stringval=val;
	debugWrite("%.*s",len,val);
}

void sqlrprotocol_teradata::parseByteBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	char	*val=(char *)ptr;
	*outptr=ptr+inbind->valuesize;
	inbind->type=SQLRSERVERBINDVARTYPE_BLOB;
	inbind->value.stringval=val;
	debugHexDump((byte_t *)val,inbind->valuesize);
}

void sqlrprotocol_teradata::parseVarByteBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	uint16_t	len;
	read(ptr,&len,&ptr);
	char	*val=(char *)ptr;
	ptr+=len;
	inbind->type=SQLRSERVERBINDVARTYPE_BLOB;
	inbind->valuesize=len;
	inbind->value.stringval=val;
	debugHexDump((byte_t *)val,len);
}

void sqlrprotocol_teradata::parseFloatBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {
	uint64_t	val;
	read(ptr,&val,outptr);
	double		*valptr=(double *)&val;
	inbind->value.doubleval.value=*valptr;
	// set precision/scale to max values
	// (NOTE: these were determined experimentally, and work with a
	// teradata backend, it's possible that other backends might not
	// like these values)
	inbind->value.doubleval.precision=38;
	inbind->value.doubleval.scale=37;
	inbind->type=SQLRSERVERBINDVARTYPE_DOUBLE;
	debugWrite("%f",inbind->value.doubleval.value);
}

void sqlrprotocol_teradata::parseDateBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {

	// copy out the date string
	char	*tmp=charstring::duplicate((char *)ptr,10);
	debugWrite("%s",tmp);
	*outptr=ptr+10;

	// init the dateval
	inbind->type=SQLRSERVERBINDVARTYPE_DATE;
	inbind->value.dateval.year=-1;
	inbind->value.dateval.month=-1;
	inbind->value.dateval.day=-1;
	inbind->value.dateval.hour=-1;
	inbind->value.dateval.minute=-1;
	inbind->value.dateval.second=-1;
	inbind->value.dateval.microsecond=-1;
	inbind->value.dateval.tz=NULL;
	inbind->value.dateval.isnegative=false;

	// parse the date/time/timestamp
	int16_t	year=0;
	int16_t	month=0;
	int16_t	day=0;
	int16_t	hour=0;
	int16_t	minute=0;
	int16_t	second=0;
	int32_t	fraction=0;
	bool	isnegative=false;
	if (datetime::parse(tmp,false,false,"-",
				&year,&month,&day,&hour,&minute,&second,
				&fraction,&isnegative)) {

		// set the dateval components
		inbind->value.dateval.year=year;
		inbind->value.dateval.month=month;
		inbind->value.dateval.day=day;
	}

	// clean up
	delete[] tmp;
}

void sqlrprotocol_teradata::parseTimeBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {

	// copy out the time string
	char	*tmp=charstring::duplicate((char *)ptr,8);
	debugWrite("%s",tmp);
	*outptr=ptr+8;

	// init the dateval
	inbind->type=SQLRSERVERBINDVARTYPE_DATE;
	inbind->value.dateval.year=-1;
	inbind->value.dateval.month=-1;
	inbind->value.dateval.day=-1;
	inbind->value.dateval.hour=-1;
	inbind->value.dateval.minute=-1;
	inbind->value.dateval.second=-1;
	inbind->value.dateval.microsecond=-1;
	inbind->value.dateval.tz=NULL;
	inbind->value.dateval.isnegative=false;

	// parse the date/time/timestamp
	int16_t	year=0;
	int16_t	month=0;
	int16_t	day=0;
	int16_t	hour=0;
	int16_t	minute=0;
	int16_t	second=0;
	int32_t	fraction=0;
	bool	isnegative=false;
	if (datetime::parse(tmp,false,false,"-",
				&year,&month,&day,&hour,&minute,&second,
				&fraction,&isnegative)) {

		// set the dateval components
		inbind->value.dateval.hour=hour;
		inbind->value.dateval.minute=minute;
		inbind->value.dateval.second=second;
		inbind->value.dateval.microsecond=fraction;
	}

	// clean up
	delete[] tmp;
}

void sqlrprotocol_teradata::parseTimestampBind(const byte_t *ptr,
						sqlrserverbindvar *inbind,
						const byte_t **outptr) {

	// copy out the timestamp string
	char	*tmp=charstring::duplicate((char *)ptr,19);
	debugWrite("%s",tmp);
	*outptr=ptr+19;

	// init the dateval
	inbind->type=SQLRSERVERBINDVARTYPE_DATE;
	inbind->value.dateval.year=-1;
	inbind->value.dateval.month=-1;
	inbind->value.dateval.day=-1;
	inbind->value.dateval.hour=-1;
	inbind->value.dateval.minute=-1;
	inbind->value.dateval.second=-1;
	inbind->value.dateval.microsecond=-1;
	inbind->value.dateval.tz=NULL;
	inbind->value.dateval.isnegative=false;

	// parse the date/time/timestamp
	int16_t	year=0;
	int16_t	month=0;
	int16_t	day=0;
	int16_t	hour=0;
	int16_t	minute=0;
	int16_t	second=0;
	int32_t	fraction=0;
	bool	isnegative=false;
	if (datetime::parse(tmp,false,false,"-",
				&year,&month,&day,&hour,&minute,&second,
				&fraction,&isnegative)) {

		// set the dateval components
		inbind->value.dateval.year=year;
		inbind->value.dateval.month=month;
		inbind->value.dateval.day=day;
		inbind->value.dateval.hour=hour;
		inbind->value.dateval.minute=minute;
		inbind->value.dateval.second=second;
		inbind->value.dateval.microsecond=fraction;
	}

	// clean up
	delete[] tmp;
}

bool sqlrprotocol_teradata::parseStatementInfoParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 303

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=169) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","169",parcelflavor,parceldatasize);

	parseStatementInfoExtensions(parceldata,parceldatasize);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseStatementInfoExtensions(
					const byte_t *ext,
					uint32_t extlen) {

	// The spec doesn't even mention that we could receive a
	// StatementInfo request parcel.  Apparently we can, and they appear
	// to be composed of extensions, just like StatementInfo response
	// parcels.  However, so far, I've only seen extension number 8, which
	// isn't defined in the spec, but appears to describe a parameter.

	debugStart("bind variables");

	// this request's bind phase begins here; clear the pool once,
	// before parsing any of its parameter extensions
	cont->getBindPool(req->cur)->clear();

	// input bind count
	uint16_t	ibcount=0;

	while (extlen) {

		// PBTILOUT - layout
		// PBTIID - extension
		// PBTILEN - size
		uint16_t	pbtilout;
		uint16_t	pbtiid;
		uint16_t	pbtilen;
		read(ext,&pbtilout,&ext);
		read(ext,&pbtiid,&ext);
		read(ext,&pbtilen,&ext);
		extlen-=(sizeof(uint16_t)*3);

		if (pbtiid==8) {
			parseParameterExtension(ext,pbtilen,ibcount);
			if (pbtilen) {
				ibcount++;
			}
		} else {
			debugStart("unhandled extension");
			debugWrite("layout: %d",pbtilout);
			debugWrite("extension: %d",pbtiid);
			debugWrite("size: %d",pbtilen);
			debugHexDump(ext,pbtilen);
			debugEnd();
		}

		// move on to the next extension
		ext+=pbtilen;
		extlen-=pbtilen;
	}

	debugEnd();

	// set input bind count
	cont->setInputBindCount(req->cur,ibcount);

	return true;
}

bool sqlrprotocol_teradata::parseParameterExtension(
						const byte_t *ext,
						uint32_t extlen,
						uint16_t ibcount) {

	// apparently one of these with a zero-size
	// means that its the end of the parameters
	if (!extlen) {
		return true;
	}

	memorypool		*bindpool=cont->getBindPool(req->cur);
	sqlrserverbindvar	*inbinds=cont->getInputBinds(req->cur);
	sqlrserverbindvar	*inbind=&(inbinds[ibcount]);
	bindtype		*inbindtype=&(req->bindtypes[ibcount]);

	// parse the extension
	uint16_t	type;
	uint16_t	valuesize;
	read(ext,&type,&ext);
	// always zeros - padding?
	ext+=6;
	read(ext,&valuesize,&ext);
	// always zeros - padding?
	ext+=6;

	// we have bind variables
	req->bindvars=true;

	// set bind variable name
	char	*var=charstring::parseNumber(ibcount+1);
	inbind->variablesize=charstring::getLength(var)+1;
	inbind->variable=(char *)bindpool->allocate(inbind->variablesize+1);
	inbind->variable[0]=cont->getBindFormat()[0];
	charstring::copy(inbind->variable+1,var,inbind->variablesize);
	inbind->variable[inbind->variablesize]='\0';
	delete[] var;

	// set bind variable type (and size, if applicable)
	const char	*typestr="";
	switch (type) {
		case 756:
		case 757:
			typestr="TINYINT";
			inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
			inbind->valuesize=0;
			break;
		case 500:
		case 501:
			typestr="SMALLINT";
			inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
			inbind->valuesize=0;
			break;
		case 496:
		case 497:
			typestr="INTEGER";
			inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
			inbind->valuesize=0;
			break;
		case 600:
		case 601:
			typestr="BIGINT";
			inbind->type=SQLRSERVERBINDVARTYPE_INTEGER;
			inbind->valuesize=0;
			break;
		case 452:
		case 453:
			typestr="CHAR";
			inbind->type=SQLRSERVERBINDVARTYPE_STRING;
			inbind->valuesize=valuesize;
			break;
		case 448:
		case 449:
			typestr="VARCHAR";
			inbind->type=SQLRSERVERBINDVARTYPE_STRING;
			inbind->valuesize=valuesize;
			break;
		case 692:
		case 693:
			typestr="BYTE";
			inbind->type=SQLRSERVERBINDVARTYPE_BLOB;
			inbind->valuesize=valuesize;
			break;
		case 688:
		case 689:
			typestr="VARBYTE";
			inbind->type=SQLRSERVERBINDVARTYPE_BLOB;
			inbind->valuesize=valuesize;
			break;
		case 484:
		case 485:
			typestr="DECIMAL";
			inbind->type=SQLRSERVERBINDVARTYPE_DOUBLE;
			inbind->valuesize=0;
			// FIXME: set these somehow...
			// no obvious way to test this, ODBC maybe?
			inbind->value.doubleval.precision=0;
			inbind->value.doubleval.scale=0;
			break;
		case 748:
		case 749:
		case 752:
		case 753:
			typestr="DATE";
			inbind->type=SQLRSERVERBINDVARTYPE_DATE;
			inbind->valuesize=0;
			inbind->value.dateval.tz=NULL;
			break;
		case 760:
		case 761:
			typestr="TIME";
			inbind->type=SQLRSERVERBINDVARTYPE_DATE;
			inbind->valuesize=0;
			inbind->value.dateval.tz=NULL;
			break;
		case 764:
		case 765:
			typestr="DATETIME";
			inbind->type=SQLRSERVERBINDVARTYPE_DATE;
			inbind->valuesize=0;
			inbind->value.dateval.tz=NULL;
			break;
		case 480:
		case 481:
			typestr="FLOAT";
			inbind->type=SQLRSERVERBINDVARTYPE_DOUBLE;
			inbind->valuesize=0;
			// set precision/scale to max values
			// (NOTE: these were determined experimentally, and
			// work with a teradata backend, it's possible that
			// other backends might not like these values)
			inbind->value.doubleval.precision=38;
			inbind->value.doubleval.scale=37;
			break;
		default:
			debugWrite("unknown bind type: %d",type);
			typestr="VARBYTE";
			inbind->type=SQLRSERVERBINDVARTYPE_STRING;
			inbind->valuesize=valuesize;
			break;
	}
	inbindtype->type=typestr;
	inbindtype->typelen=charstring::getLength(inbindtype->type);
	inbind->valuesize=0;

	debugWrite("%s","");
	if (inbind->valuesize) {
		debugWrite("%d: %s(%.*s(%d))",ibcount,
					inbind->variable,
					inbindtype->typelen,
					inbindtype->type,
					inbind->valuesize);
	} else {
		debugWrite("%d: %s(%.*s)",ibcount,
					inbind->variable,
					inbindtype->typelen,
					inbindtype->type);
	}

	return true;
}

bool sqlrprotocol_teradata::parseStatementInfoEndParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 303

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=170) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","170",parcelflavor,parceldatasize);

	// no parcel data to parse

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseMultipartIndicDataParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 271

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=142) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","142",parcelflavor,parceldatasize);

	// parse parcel data...
	const byte_t	*ptr=parceldata;

	sqlrserverbindvar	*inbinds=cont->getInputBinds(req->cur);
	uint16_t		ibcount=cont->getInputBindCount(req->cur);

	// parse null indicator
	uint32_t	nisize=ibcount/8+1;
	byte_t		ni=*ptr;
	if (getDebug()) {
		debugWrite("null indicator:");
		stringbuffer	b;
		for (uint16_t i=0; i<nisize; i++) {
			b.printBits(*(ptr+i));
			debugWrite("%s",b.getString());
			b.clear();
			debugWrite(" (%02x)",*(ptr+i));
		}
		debugWrite("%s","");
	}
	for (uint16_t i=0; i<ibcount; i++) {

		// set the null indicator
		sqlrserverbindvar	*inbind=&(inbinds[i]);
		inbind->isnull=(ni&0x80)?
				cont->getNullBindValue():
				cont->getNonNullBindValue();

		// move on
		ni<<=1;
		if (i%8==7) {
			ptr++;
			ni=*ptr;
		}
	}
	if (ibcount%8) {
		ptr++;
	}

	debugStart("bind values");

	// parse bind values...
	for (uint16_t i=0; i<ibcount; i++) {

		sqlrserverbindvar	*inbind=&(inbinds[i]);
		bindtype		*inbindtype=&(req->bindtypes[i]);

		// debug
		debugWrite("%s","");
		if (inbind->valuesize) {
			debugWrite("%s(%.*s(%d)) = ",
					inbind->variable,
					inbindtype->typelen,
					inbindtype->type,
					inbind->valuesize);
		} else {
			debugWrite("%s(%.*s) = ",
					inbind->variable,
					inbindtype->typelen,
					inbindtype->type);
		}

		// get value from data
		if (!charstring::compare(inbindtype->type,"CHAR")) {
			parseCharBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"VARCHAR")) {
			parseVarCharBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"TINYINT")) {
			parseTinyIntBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"SMALLINT")) {
			parseSmallIntBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"INTEGER")) {
			parseIntegerBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"BIGINT")) {
			parseBigIntBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"DECIMAL")) {
			// FIXME: no obvious way to test this, ODBC maybe?
			parseFloatBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"FLOAT")) {
			parseFloatBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"BYTE")) {
			parseByteBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"VARBYTE")) {
			parseVarByteBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"DATE")) {
			parseDateBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"TIME")) {
			parseTimeBind(ptr,inbind,&ptr);
		} else if (!charstring::compare(inbindtype->type,"DATETIME")) {
			parseTimestampBind(ptr,inbind,&ptr);
		}

		// override type for nulls...
		// Arguably the controller should do this, or the backend
		// (or backend driver) should handle it correctly.  Some
		// backends (eg. ODBC+teradata) don't necessarily handle the
		// null indicator correctly for all bind types.
		// Eg. SQLBindParameter(type=DATE,isnull=is-a-null) fails.
		// But, SQLBindParameter(type=STRING,isnull=is-a-null) works,
		// which is what gets called if
		// inbind->type=SQLRSERVERBINDVARTYPE_NULL.
		// So we'll go with that for now.
		if (inbind->isnull==cont->getNullBindValue()) {
			inbind->type=SQLRSERVERBINDVARTYPE_NULL;
		}
	}

	debugEnd();

	// we have bind values
	req->bindvals=true;

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseEndMultipartIndicDataParcel(
					const byte_t *parcel,
					const byte_t **parcelout) {
	// see Teradata CLIv2, page 257

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=143) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","143",parcelflavor,parceldatasize);

	// no parcel data to parse

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::parseSlobResponseParcel(const byte_t *parcel,
						const byte_t **parcelout) {
	// see parcel.h - PclSLOBResponse

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=215) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","215",parcelflavor,parceldatasize);

	// FIXME: parse parcel data
	debugWrite("...");

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

bool sqlrprotocol_teradata::prepareQuery() {

	uint32_t	querylen=req->querylen;
	const char	*query=req->query;

	// Apparently if we pass the database an options parcel with
	// function='S', followed by a request parcel containing an insert
	// query, then it will return column metadata for the table!
	//
	// JDBC-style fastload depends on this.
	//
	// The only way to do this without passing raw packets to a Teradata
	// backend is to translate the query to a select, and prepare that
	// instead.
	stringbuffer	s;
	if (req->function=='S') {

		// FIXME: this is rough, and assumes that we're inserting into
		// all columns, in order...
		const char	*end=query+querylen;
		const char	*q=cont->skipWhitespaceAndComments(query);
		bool		isinsert=
				!charstring::compareIgnoringCase(q,"insert",6);
		if (isinsert) {
			q+=6;
			isinsert=character::isWhitespace(*q);
		}
		if (isinsert) {
			while (character::isWhitespace(*q) && q!=end) {
				q++;
			}
			isinsert=(q!=end);
		}
		if (isinsert) {
			isinsert=!charstring::compareIgnoringCase(q,"into",4);
		}
		if (isinsert) {
			q+=4;
			isinsert=character::isWhitespace(*q);
		}
		if (isinsert) {
			while (character::isWhitespace(*q) && q!=end) {
				q++;
			}
			isinsert=(q!=end);
		}
		const char	*table=q;
		if (isinsert) {
			while (!character::isWhitespace(*q) && q!=end) {
				q++;
			}
			isinsert=(q!=end);
		} 
		if (isinsert) {
			const char	*space=q;
			s.append("select * from ")->append(table,space-table);
			query=s.getString();
			querylen=s.getSize();

			req->fudgeselect=true;
		}
	}

	// prepare the query
	debugStart("prepare query");
	bool	retval=cont->prepareQuery(req->cur,
					query,querylen,
					true,true,true,true);
	debugWrite("result: %s",(retval)?"success":"error");
	debugEnd();
	return retval;
}

bool sqlrprotocol_teradata::executeQuery() {

	debugStart("execute query");

	// execute the query
	bool	retval=cont->executeQuery(req->cur,true,true,true,true);

	debugWrite("result: %s",(retval)?"success":"error");
	debugEnd();

	return retval;
}

bool sqlrprotocol_teradata::parseCancelParcel(const byte_t *parcel,
						const byte_t **parcelout) {
	// see Teradata CLIv2, page 250

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);
	if (parcelflavor!=7) {
		unexpectedParcel(parcelflavor);
		*parcelout=parcel;
		return false;
	}

	debugParcelStart("recv","cancel",parcelflavor,parceldatasize);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

void sqlrprotocol_teradata::parseGenericParcels(const byte_t *parcel,
							const byte_t *end) {
	while (parcel!=end) {
		parseGenericParcel(parcel,&parcel);
	}
}

bool sqlrprotocol_teradata::parseGenericParcel(const byte_t *parcel,
						const byte_t **parcelout) {

	// parse parcel header
	uint16_t	parcelflavor;
	const byte_t	*parceldata;
	uint32_t	parceldatasize;
	parseParcelHeader(parcel,&parcelflavor,
					&parceldatasize,
					&parceldata);

	debugParcelStart("recv","generic",parcelflavor,parceldatasize);

	// return next parcel
	*parcelout=parceldata+parceldatasize;

	debugParcelEnd(parceldata,parceldatasize);

	return true;
}

void sqlrprotocol_teradata::appendParcelHeader(uint16_t flavor,
						uint32_t datasize) {
	if (datasize>=65536) {
		appendLargeParcelHeader(flavor,datasize);
	} else {
		appendSmallParcelHeader(flavor,datasize);
	}
}

void sqlrprotocol_teradata::appendLargeParcelHeader(uint16_t flavor,
							uint32_t datasize) {
	write(&respdata,(uint16_t)(flavor|0x8000));
	write(&respdata,(uint16_t)0);
	write(&respdata,(uint32_t)(2+2+4+datasize));
}

void sqlrprotocol_teradata::appendSmallParcelHeader(uint16_t flavor,
							uint32_t datasize) {
	write(&respdata,flavor);
	write(&respdata,(uint16_t)(2+2+datasize));
}

void sqlrprotocol_teradata::appendParcelHeader(uint16_t flavor) {
	write(&respdata,(uint16_t)(flavor|0x8000));
	write(&respdata,(uint16_t)0);
	req->parcelsizepos=respdata.getPosition();
	write(&respdata,(uint32_t)0);
}

void sqlrprotocol_teradata::endParcel() {
	size_t	originalpos=respdata.getPosition();
	respdata.setPositionRelativeToBeginning(req->parcelsizepos);
	respdata.write(hostTo((uint32_t)(2+2+4+
					originalpos-
					req->parcelsizepos-
					sizeof(uint32_t))));
	respdata.setPositionRelativeToBeginning(originalpos);
}

void sqlrprotocol_teradata::appendConfigResponseParcel() {

	// see parcel.h - PclConfigRspType

	debugParcelStart("send","config response",43);

	appendSmallParcelHeader(43,538);

	appendConfigResponseFixedPortion();
	appendConfigResponseIFPs();
	appendConfigResponseAMPs();
	appendConfigResponseCharSets();
	appendConfigResponseInDoubt();
	appendConfigResponseHasFields();
	appendConfigResponseTransactionSemantics();
	appendConfigResponseField7();
	appendConfigResponseField9();
	appendConfigResponseField10();
	appendConfigResponseField11();
	appendConfigResponseField12();
	appendConfigResponseField13();
	appendConfigResponseField14();
	appendConfigResponseField15();
	appendConfigResponseField16();
	appendConfigResponseField6();

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendConfigResponseFixedPortion() {

	// see parcel.h - PclConfigRspType

	// session/request numbers...
	uint32_t	firstsessionno=1000;
	uint32_t	firstrequestno=1000;
	uint16_t	maxsessioncount=120;
	write(&respdata,firstsessionno);
	write(&respdata,firstrequestno);
	write(&respdata,maxsessioncount);
	debugWrite("first session number: %d",firstsessionno);
	debugWrite("first request number: %d",firstrequestno);
	debugWrite("max session count: %d",maxsessioncount);
}

void sqlrprotocol_teradata::appendConfigResponseIFPs() {

	// see parcel.h - PclConfigRspType, IFPrec

	// IFP count...
	// (FIXME: variable number of these sometimes?)
	uint16_t	ifpcount=1;
	write(&respdata,ifpcount);

	// (v)IFP ((virtual) interface processor) record(s)...
	uint16_t	ifpid=30719;
	byte_t		ifpstate=0;
	byte_t		ifppad=0;
	write(&respdata,ifpid);
	write(&respdata,ifpstate);
	write(&respdata,ifppad);
	debugStart("IFP %d",ifpid);
	debugWrite("state: %d",ifpstate);
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseAMPs() {

	// see parcel.h - PclConfigRspType, AMPArray

	// AMP (access module processor) count...
	// (FIXME: variable number of these sometimes?)
	uint16_t	ampcount=2;
	write(&respdata,ampcount);

	// AMP records...
	uint16_t	amp[]={0,1};
	debugStart("AMPs");
	for (uint16_t i=0; i<ampcount; i++) {
		write(&respdata,amp[i]);
		debugWrite("%d",amp[i]);
	}
	debugEnd();


	// FIXME: not sure what this is, maybe another AMP?
	byte_t		unknown9=(!getProtocolIsBigEndian())?0x7F:0xFF;
	byte_t		unknown10=0;
	write(&respdata,unknown9);
	write(&respdata,unknown10);
	debugWrite("unknown9: %d (0x%02x)",unknown9,unknown9);
	debugWrite("unknown10: %d",unknown10);
}


void sqlrprotocol_teradata::appendConfigResponseCharSets() {

	// see parcel.h - PclConfigRspType, FltCharArrayNameCodeType

	// character set count...
	uint16_t	cscount=4;
	write(&respdata,cscount);

	// character set records
	byte_t		lecscodes[]={
		0x3e, 0x3f, 0x7f, 0x40
	};
	byte_t		becscodes[]={
		0xbe, 0xbf, 0xff, 0xc0
	};
	const byte_t	*cscodes=
			(!getProtocolIsBigEndian())?lecscodes:becscodes;
	byte_t		cspad=0;
	const char	*csnames[]={
		"UTF16                         ",
		"UTF8                          ",
		"ASCII                         ",
		"EBCDIC                        "
	};
	debugStart("charsets:");
	for (uint16_t i=0; i<cscount; i++) {
		write(&respdata,cscodes[i]);
		write(&respdata,cspad);
		write(&respdata,csnames[i],30);
		debugWrite("0x%02x - '%s'",cscodes[i],csnames[i]);
	}
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseInDoubt() {

	// see parcel.h - PclConfigRspType

	// in-doubt sessions exist ('Y'/'N')...
	// see parcel.h - PclConfigRspType, InDoubt
	char	indoubt='N';
	write(&respdata,indoubt);
	debugWrite("in doubt sessions: %c",indoubt);
}

void sqlrprotocol_teradata::appendConfigResponseHasFields() {

	// see parcel.h - PclConfigRspType

	// has fields...
	byte_t		hasfields=1;
	write(&respdata,hasfields);
	debugWrite("has fields: %d",hasfields);
}

void sqlrprotocol_teradata::appendConfigResponseTransactionSemantics() {

	// see parcel.h - PclConfigRspType, PclCfgNDMFeatureType

	// transaction semantics
	// 'T' - Teradata
	// 'A' - ANSI
	char	ts='T';

	write(&respdata,(uint16_t)1);
	write(&respdata,(uint16_t)sizeof(ts));
	write(&respdata,ts);

	debugWrite("transaction semantics: %c",ts);
}

void sqlrprotocol_teradata::appendConfigResponseField7() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?
	//
	// they look like max sizes or max values for things,
	// but it's not clear exactly what
	//
	// some possibilities...
	// 64000 - long varchar max length for latin, kanji1
	// 32000 - long varchar max length for unicode, graphic, kanjsjis
	//
	// 2536 - maybe max bind count - error 5793 suggests this
	//
	// 191 was historically the default varchar length in some dbs
	// because larger lengths caused performance to decline with their
	// indexing algorithms
	//
	// 38 is the max numeric column precision in some dbs
	//
	// 6 ???


	// FIXME: double check that these first two aren't just the
	// 16 bit number 0x310 in BE and 0x0031 in LE
	byte_t	unknown1=31;
	byte_t	unknown2=0;
	uint16_t	unknown3=100;
	uint32_t	unknown4=64000;
	uint32_t	unknown5=1000000;
	uint64_t	unknown6=2080377344;
	uint32_t	unknown7=7340032;
	uint32_t	unknown8=16775168;
	uint32_t	unknown9=4096;
	uint32_t	unknown10=191;
	uint32_t	unknown11=16;
	uint32_t	unknown12=65535;
	uint32_t	unknown13=2048;
	uint32_t	unknown14=128;
	uint32_t	unknown15=64;
	uint32_t	unknown16=2536;
	uint32_t	unknown17=1024000;
	uint32_t	unknown18=62000;
	uint32_t	unknown19=31000;
	uint32_t	unknown20=38;
	uint32_t	unknown21=64000;
	uint32_t	unknown22=64000;
	uint32_t	unknown23=64000;
	uint32_t	unknown24=32000;
	uint32_t	unknown25=32000;
	uint32_t	unknown26=64000;
	uint32_t	unknown27=64000;
	uint32_t	unknown28=2536;
	uint32_t	unknown29=6;
	uint32_t	unknown30=6;
	uint32_t	unknown31=6;
	uint32_t	unknown32=1000;
	uint32_t	unknown33=1024000;
	uint32_t	unknown34=16776192;
	uint32_t	unknown35=1048500;
	uint32_t	unknown36=64000;

	write(&respdata,(uint16_t)7);
	write(&respdata,(uint16_t)140);
	write(&respdata,unknown1);
	write(&respdata,unknown2);
	write(&respdata,unknown3);
	write(&respdata,unknown4);
	write(&respdata,unknown5);
	write(&respdata,unknown6);
	write(&respdata,unknown7);
	write(&respdata,unknown8);
	write(&respdata,unknown9);
	write(&respdata,unknown10);
	write(&respdata,unknown11);
	write(&respdata,unknown12);
	write(&respdata,unknown13);
	write(&respdata,unknown14);
	write(&respdata,unknown15);
	write(&respdata,unknown16);
	write(&respdata,unknown17);
	write(&respdata,unknown18);
	write(&respdata,unknown19);
	write(&respdata,unknown20);
	write(&respdata,unknown21);
	write(&respdata,unknown22);
	write(&respdata,unknown23);
	write(&respdata,unknown24);
	write(&respdata,unknown25);
	write(&respdata,unknown26);
	write(&respdata,unknown27);
	write(&respdata,unknown28);
	write(&respdata,unknown29);
	write(&respdata,unknown30);
	write(&respdata,unknown31);
	write(&respdata,unknown32);
	write(&respdata,unknown33);
	write(&respdata,unknown34);
	write(&respdata,unknown35);
	write(&respdata,unknown36);

	debugStart("unknown field 7");
	debugWrite("unknown1: 0x%02x",unknown1);
	debugWrite("unknown2: 0x%02x",unknown2);
	debugWrite("unknown3: %hd",unknown3);
	debugWrite("unknown4: %d",unknown4);
	debugWrite("unknown5: %d",unknown5);
	debugWrite("unknown6: %lld",(long long)unknown6);
	debugWrite("unknown7: %d",unknown7);
	debugWrite("unknown8: %d",unknown8);
	debugWrite("unknown9: %d",unknown9);
	debugWrite("unknown10: %d",unknown10);
	debugWrite("unknown11: %d",unknown11);
	debugWrite("unknown12: %d",unknown12);
	debugWrite("unknown13: %d",unknown13);
	debugWrite("unknown14: %d",unknown14);
	debugWrite("unknown15: %d",unknown15);
	debugWrite("unknown16: %d",unknown16);
	debugWrite("unknown17: %d",unknown17);
	debugWrite("unknown18: %d",unknown18);
	debugWrite("unknown19: %d",unknown19);
	debugWrite("unknown20: %d",unknown20);
	debugWrite("unknown21: %d",unknown21);
	debugWrite("unknown22: %d",unknown22);
	debugWrite("unknown23: %d",unknown23);
	debugWrite("unknown24: %d",unknown24);
	debugWrite("unknown25: %d",unknown25);
	debugWrite("unknown26: %d",unknown26);
	debugWrite("unknown27: %d",unknown27);
	debugWrite("unknown28: %d",unknown28);
	debugWrite("unknown29: %d",unknown29);
	debugWrite("unknown30: %d",unknown30);
	debugWrite("unknown31: %d",unknown31);
	debugWrite("unknown32: %d",unknown32);
	debugWrite("unknown33: %d",unknown33);
	debugWrite("unknown34: %d",unknown34);
	debugWrite("unknown35: %d",unknown35);
	debugWrite("unknown36: %d",unknown36);
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField9() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?
	// something enabled/disabled?
	// 0=off, 1=on?

	byte_t		data=1;

	write(&respdata,(uint16_t)9);
	write(&respdata,(uint16_t)sizeof(data));
	write(&respdata,data);

	debugStart("unknown field 9");
	debugWrite("data: %d",data);
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField10() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?
	// something set of things enabled/disabled?
	// 0=off, 1=on, 2=third option?

	// FIXME: double-check this
	byte_t		data[]={
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02,
		0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01,
		0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x02
	};

	write(&respdata,(uint16_t)10);
	write(&respdata,(uint16_t)sizeof(data));
	write(&respdata,data,sizeof(data));

	debugStart("unknown field 10");
	debugWrite("data:");
	debugHexDump(data,sizeof(data));
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField11() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?
	// something set of things enabled/disabled?
	// 0=off, 1=on, 2=third option?

	// FIXME: double-check this
	byte_t		data[]={
		0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x00, 0x01
	};

	write(&respdata,(uint16_t)11);
	write(&respdata,(uint16_t)sizeof(data));
	write(&respdata,data,sizeof(data));

	debugStart("unknown field 11");
	debugWrite("data:");
	debugHexDump(data,sizeof(data));
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField12() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?
	// something set of things enabled/disabled?
	// 0=off, 1=on, 2=third option?

	// FIXME: double-check this
	byte_t		data[]={
		0x01, 0x01, 0x01, 0x02, 0x01, 0x01
	};

	write(&respdata,(uint16_t)12);
	write(&respdata,(uint16_t)sizeof(data));
	write(&respdata,data,sizeof(data));

	debugStart("unknown field 12");
	debugWrite("data:");
	debugHexDump(data,sizeof(data));
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField13() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType

	// appears to be version strings...
	// FIXME: of what?
	const char	*version1="16.20.12.01                   ";
	size_t		version1size=charstring::getLength(version1);
	const char	*version2="16.20.12.01                     ";
	size_t		version2size=charstring::getLength(version2);

	write(&respdata,(uint16_t)13);
	write(&respdata,(uint16_t)(version1size+version2size));
	write(&respdata,version1,version1size);
	write(&respdata,version2,version2size);

	debugStart("unknown field 13");
	debugWrite("version1: '%s'",version1);
	debugWrite("version2: '%s'",version2);
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField14() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?

	// FIXME: double-check this
	byte_t		data[]={
		0x03, 0x03, 0x02, 0x03
	};

	write(&respdata,(uint16_t)14);
	write(&respdata,(uint16_t)sizeof(data));
	write(&respdata,data,sizeof(data));

	debugStart("unknown field 14");
	debugWrite("data:");
	debugHexDump(data,sizeof(data));
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField15() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?
	// something set of things enabled/disabled?
	// 0=off, 1=on?

	// FIXME: double-check this
	byte_t		data[]={
		0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
		0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01
	};

	write(&respdata,(uint16_t)15);
	write(&respdata,(uint16_t)sizeof(data));
	write(&respdata,data,sizeof(data));

	debugStart("unknown field 15");
	debugWrite("data:");
	debugHexDump(data,sizeof(data));
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField16() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?

	// FIXME: double-check this
	byte_t		data1[]={
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00
	};
	uint16_t	data2=32770;
	byte_t		data3[]={
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	write(&respdata,(uint16_t)16);
	write(&respdata,(uint16_t)(sizeof(data1)+sizeof(data2)+sizeof(data3)));
	write(&respdata,data1,sizeof(data1));
	write(&respdata,data2);
	write(&respdata,data3,sizeof(data3));

	debugStart("unknown field 16");
	debugWrite("data1:");
	debugHexDump(data1,sizeof(data1));
	debugWrite("data2: %d (0x%02x)",data2,data2);
	debugWrite("data3:");
	debugHexDump(data3,sizeof(data3));
	debugEnd();
}

void sqlrprotocol_teradata::appendConfigResponseField6() {

	// see parcel.h - PclConfigRspType, PclCfgExtendType
	// FIXME: what is this field?

	byte_t		data[]={
		0x01, 0x49
	};

	write(&respdata,(uint16_t)6);
	write(&respdata,(uint16_t)sizeof(data));
	write(&respdata,data,sizeof(data));

	debugStart("unknown field 6");
	debugWrite("data:");
	debugHexDump(data,sizeof(data));
	debugEnd();
}

void sqlrprotocol_teradata::appendGatewayConfigParcel() {

	// see parcel.h - pclgtwconfig_t

	debugParcelStart("send","gateway config",165);

	appendSmallParcelHeader(165,66);


	// has fields
	uint32_t	hasfields=1;
	write(&respdata,hasfields);
	debugWrite("has fields: %d",hasfields);



	// NOTE that for each field below, when sending the size of the data,
	// we're sending 4 bytes, plus the size of the data.  Even when there's
	// no data, we're still sending 4 bytes.  I'm not sure why we need to
	// do this, but it appears to be the correct thing to do.


	// SSO field
	uint16_t	ssofield=GWCONFIGFIELD_SSO;
	byte_t		ssolevel=1;

	write(&respdata,ssofield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+sizeof(ssolevel)));
	write(&respdata,ssolevel);

	debugWrite("sso level: %d",ssolevel);



	// GSS field
	uint16_t	gssfield=GWCONFIGFIELD_GSS_VERSION;
	byte_t		gssversion[]={
		// 16.20.12.01
		0x10, 0x14, 0x0C, 0x01
	};

	write(&respdata,gssfield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+sizeof(gssversion)));
	write(&respdata,gssversion,sizeof(gssversion));

	debugWrite("gss version:");
	debugHexDump(gssversion,sizeof(gssversion));



	// UTF field
	uint16_t	utffield=GWCONFIGFIELD_UTF;

	write(&respdata,utffield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)));

	debugWrite("utf: true");



	// session ID (FIXME: get this for real, maybe from sessionno)
	uint16_t	sessionidfield=GWCONFIGFIELD_SESSION_ID;
	uint16_t	sessionid=33;

	write(&respdata,sessionidfield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+sizeof(sessionid)));
	write(&respdata,sessionid);

	debugWrite("session id: %hd",sessionid);



	// control data
	uint16_t	controldatafield=GWCONFIGFIELD_CONTROL_DATA;

	write(&respdata,controldatafield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)));

	debugWrite("control data: true");



	// recoverable protocol
	uint16_t	recoverableprotocolfield=
				GWCONFIGFIELD_RECOVERABLE_PROTOCOL;

	write(&respdata,recoverableprotocolfield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)));

	debugWrite("recoverable protocol: true");



	// redrive
	uint16_t	redrivefield=GWCONFIGFIELD_REDRIVE;
	write(&respdata,redrivefield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)));
	debugWrite("redrive: true");



	// unknown field 8
	// ...and client complains about this:
	// ncs_bld_cache: unrecognized PclGTWCONFIG feature 8 (length 4)
	uint16_t	field8=8;

	write(&respdata,field8);
	write(&respdata,(uint16_t)(sizeof(uint32_t)));

	debugWrite("unknown field 8: true");



	// unknown field 9
	// ...and client complains about this:
	// ncs_bld_cache: unrecognized PclGTWCONFIG feature 9 (length 4)
	uint16_t	field9=9;

	write(&respdata,field9);
	write(&respdata,(uint16_t)(sizeof(uint32_t)));

	debugWrite("unknown field 9: true");



	// security policy
	uint16_t	securitypolicyfield=GWCONFIGFIELD_SECURITY_POLICY;
	byte_t		securitypolicylevel=1;

	write(&respdata,securitypolicyfield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+
					sizeof(securitypolicylevel)));
	write(&respdata,securitypolicylevel);

	debugWrite("security policy level: %d",securitypolicylevel);



	// unknown field 11
	// ...and client complains about this:
	// ncs_bld_cache: unrecognized PclGTWCONFIG feature 11 (length 5)
	uint16_t	field11=11;
	byte_t		field11level=1;

	write(&respdata,field11);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+sizeof(field11level)));
	write(&respdata,field11level);

	debugWrite("unknown field 11 level: %d",field11level);



	// negotiate mech
	uint16_t	negotiatemechfield=12;
	byte_t		negotiatemechlevel=1;

	write(&respdata,negotiatemechfield);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+
					sizeof(negotiatemechlevel)));
	write(&respdata,negotiatemechlevel);

	debugWrite("negotiate mech level: %d",negotiatemechlevel);



	// unknown field 14
	// ...and client complains about this:
	// ncs_bld_cache: unrecognized PclGTWCONFIG feature 14 (length 4)
	uint16_t	field14=14;

	write(&respdata,field14);
	write(&respdata,(uint16_t)(sizeof(uint32_t)));

	debugWrite("unknown field 14: true");



	debugParcelEnd();
}

void sqlrprotocol_teradata::appendHasFields() {
	write(&respdata,(uint32_t)1);
	debugWrite("has fields: 1");
}

void sqlrprotocol_teradata::appendTd1MechanismParcel() {

	// see parcel.h - pclauthmech_t

	// teradata1 mechanism
	debugParcelStart("send","auth mechanism (td1)",167);

	appendSmallParcelHeader(167,33);
	appendHasFields();
	appendMechOid(td1mechoid,sizeof(td1mechoid));
	appendMechRank(10);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendMechOid(byte_t *oid, uint32_t size) {
	write(&respdata,size);
	write(&respdata,oid,size);
	debugWrite("mech oid:");
	debugHexDump(oid,size);
}

void sqlrprotocol_teradata::appendDefaultMech() {

	// NOTE that when sending the size of the data, we're sending 4 bytes,
	// plus the size of the data.  I'm not sure why we need to do this,
	// but it appears to be the correct thing to do.

	// FIXME: what do 3 and 1 mean in the context of
	// this being the default mech?
	uint32_t	value1=3;
	uint32_t	value2=1;
	write(&respdata,(uint16_t)MECHCONFIGFIELD_DEFAULT);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+
					sizeof(value1)+sizeof(value2)));
	write(&respdata,value1);
	write(&respdata,value2);
	debugWrite("default mech: %d,%d",value1,value2);
}

void sqlrprotocol_teradata::appendMechRank(uint32_t rank) {

	// NOTE that when sending the size of the data, we're sending 4 bytes,
	// plus the size of the data.  I'm not sure why we need to do this,
	// but it appears to be the correct thing to do.

	// FIXME: what is this 1?
	uint32_t	value=1;
	write(&respdata,(uint16_t)MECHCONFIGFIELD_RANK);
	write(&respdata,(uint16_t)(sizeof(uint32_t)+
					sizeof(value)+sizeof(rank)));
	write(&respdata,value);
	write(&respdata,rank);
	debugWrite("mech rank: %d,%d",value,rank);
}

void sqlrprotocol_teradata::appendTd2MechanismParcel() {

	// see parcel.h - pclauthmech_t

	// teradata2 mechanism
	debugParcelStart("send","auth mechanism (td2)",167);

	appendSmallParcelHeader(167,45);
	appendHasFields();
	appendMechOid(td2mechoid,sizeof(td2mechoid));
	appendDefaultMech();
	appendMechRank(20);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendKrb5MechanismParcel() {

	// see parcel.h - pclauthmech_t

	// kerberos mechanism
	debugParcelStart("send","auth mechanism (krb5)",167);

	appendSmallParcelHeader(167,29);
	appendHasFields();
	appendMechOid(krb5mechoid,sizeof(krb5mechoid));
	appendMechRank(40);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendSpnegoMechanismParcel() {

	// see parcel.h - pclauthmech_t

	// teradata negotiation mechanism
	debugParcelStart("send","auth mechanism (spnego)",167);

	appendSmallParcelHeader(167,26);
	appendHasFields();
	appendMechOid(spnegomechoid,sizeof(spnegomechoid));
	appendMechRank(65);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendLdapMechanismParcel() {

	// see parcel.h - pclauthmech_t

	// ldap mechanism
	debugParcelStart("send","auth mechanism (ldap)",167);

	appendSmallParcelHeader(167,32);
	appendHasFields();
	appendMechOid(ldapmechoid,sizeof(ldapmechoid));
	appendMechRank(70);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendProxyMechanismParcel() {

	// see parcel.h - pclauthmech_t

	// teradata negotiation mechanism
	debugParcelStart("send","auth mechanism (proxy)",167);

	appendSmallParcelHeader(167,33);
	appendHasFields();
	appendMechOid(proxymechoid,sizeof(proxymechoid));
	appendMechRank(70);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendTdnegoMechanismParcel() {

	// see parcel.h - pclauthmech_t

	// teradata negotiation mechanism
	debugParcelStart("send","auth mechanism (tdnego)",167);

	appendSmallParcelHeader(167,33);
	appendHasFields();
	appendMechOid(tdnegomechoid,sizeof(tdnegomechoid));
	appendMechRank(10);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendJwtMechanismParcel() {

	// see parcel.h - pclauthmech_t

	// teradata negotiation mechanism
	debugParcelStart("send","auth mechanism (jwt)",167);

	appendSmallParcelHeader(167,33);
	appendHasFields();
	appendMechOid(tdnegomechoid,sizeof(tdnegomechoid));
	appendMechRank(30);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendLogonFailureParcel(uint16_t code,
						const char *errorstring) {

	// see Teradata CLIv2, page 261

	debugParcelStart("send","failure",9);
	debugWrite("error: %s",errorstring);
	debugParcelEnd();

	// get the length of the errorstring, limit it to 255 bytes
	uint16_t	errorstringlength=charstring::getLength(errorstring);
	if (errorstringlength>255) {
		errorstringlength=255;
	}

	// failure parcel
	appendParcelHeader(9,2+2+2+2+errorstringlength);

	// statement number
	write(&respdata,(uint16_t)0);

	// info ???
	write(&respdata,(uint16_t)0);

	// code
	write(&respdata,code);

	// length
	write(&respdata,errorstringlength);

	// msg
	write(&respdata,errorstring,errorstringlength);
}

void sqlrprotocol_teradata::setSessionNumber() {

	// FIXME: get this from a file or something...
	sessionno=1234;
}

void sqlrprotocol_teradata::appendAssignResponseParcel() {

	// see Teradata CLIv2, page 249

	debugParcelStart("send","assign response",101);

	appendSmallParcelHeader(101,94);

	const char	*publickey="        ";
	byte_t		sescopaddr[]={
		0x01, 0x04, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	const char	*publickeyn="                "
					"                ";
	const char	*relarray="16.20.";
	const char	*verarray="16.20.12.01   ";
	uint16_t	hostid=(!getProtocolIsBigEndian())?1025:2049;
	write(&respdata,publickey);
	write(&respdata,sescopaddr,sizeof(sescopaddr));
	write(&respdata,publickeyn);
	write(&respdata,relarray);
	write(&respdata,verarray);
	write(&respdata,hostid);

	debugWrite("public key exponent: '%s'",publickey);
	debugWrite("sescopaddr:");
	debugHexDump(sescopaddr,sizeof(sescopaddr));
	debugWrite("public key modulus: '%s'",publickeyn);
	debugWrite("release: %s",relarray);
	debugWrite("version: %s",verarray);
	debugWrite("host id: %d",hostid);

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendSsoResponseParcel(byte_t trip) {

	// see parcel.h - pclssorsp_t

	debugParcelStart("send","sso response",134);

	// determine datasize and authdatalen
	uint32_t	datasize=0;
	uint16_t	authdatalen=0;
	if (trip==1) {
		if (negotiatedmech==MECH_TDNEGO) {
			datasize=1993;
			authdatalen=1987;
		} else {
			datasize=960;
			authdatalen=950;
		}
	} else {
		// FIXME: this is apparently not correct for tdnego
		datasize=7;
		authdatalen=0;
	}

	appendSmallParcelHeader(134,datasize);

	byte_t	method=0;
	byte_t	code=(trip==1)?0:1;
	byte_t	mustbezero=0;
	write(&respdata,method);
	write(&respdata,code);
	write(&respdata,trip);
	write(&respdata,mustbezero);

	debugWrite("method: %d",method);
	debugWrite("code: %d",code);
	debugWrite("trip: %d",trip);
	debugWrite("mustbezero: %d",mustbezero);

	// no known reference (just had to study the trace)

	if (trip==1) {
		if (negotiatedmech==MECH_TDNEGO) {
			write(&respdata,authdatalen);
			appendSsoTdnegoSet();
		} else {
			write(&respdata,authdatalen);
			appendSsoGssData(negotiatedmech);
		}
	} else {
		write(&respdata,authdatalen);
	}

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendSsoTdnegoSet() {

	// no known reference (just had to study the trace)

	byte_t	c1data[]={
		0x02
	};
	byte_t	c3data[]={
		0x01
	};

	appendSsoGssReplyStructure(1983);
	appendSsoGssStructure(1973);
	appendSsoSpnegoSet();
	appendSsoLdapSet();
	appendSsoTd2Set();
	debugEnd();
	appendGenericCField(SSO_C1,c1data,sizeof(c1data));
	appendGenericCField(SSO_C3,c3data,sizeof(c3data));
	debugEnd();
}

void sqlrprotocol_teradata::appendSsoSpnegoSet() {

	byte_t	c1data[]={
		0x03
	};
	byte_t	c2data[]={
		0x03
	};

	appendSsoGssStructure(14);
	appendSsoMech(spnegomechoid,sizeof(spnegomechoid));
	debugWrite("mech: snpego");
	appendGenericCField(SSO_C1,c1data,sizeof(c1data));
	appendGenericCField(SSO_C2,c2data,sizeof(c2data));
	debugEnd();
}

void sqlrprotocol_teradata::appendSsoLdapSet() {

	byte_t	c1data[]={
		0x01
	};
	byte_t	c2data[]={
		0x01
	};

	appendSsoGssStructure(974);
	appendSsoMech(ldapmechoid,sizeof(ldapmechoid));
	debugWrite("mech: ldap");
	appendGenericCField(SSO_C1,c1data,sizeof(c1data));
	appendGenericCField(SSO_C2,c2data,sizeof(c2data));
	appendC6Field(MECH_LDAP);
	debugEnd();
}

void sqlrprotocol_teradata::appendSsoTd2Set() {

	byte_t	c1data[]={
		0x01
	};
	byte_t	c2data[]={
		0x01
	};

	appendSsoGssStructure(975);
	appendSsoMech(td2mechoid,sizeof(td2mechoid));
	debugWrite("mech: td2");
	appendGenericCField(SSO_C1,c1data,sizeof(c1data));
	appendGenericCField(SSO_C2,c2data,sizeof(c2data));
	appendC6Field(MECH_TD2);
	debugEnd();
}

void sqlrprotocol_teradata::appendSsoGssReplyStructure(uint64_t size) {
	debugStart("sso gss reply structure");
	write(&respdata,(byte_t)SSO_GSS_REPLY_STRUCTURE);
	writeBerEncInt(&respdata,size);
}

void sqlrprotocol_teradata::appendSsoGssStructure(uint64_t size) {
	debugStart("sso gss structure");
	write(&respdata,(byte_t)SSO_GSS_STRUCTURE);
	writeBerEncInt(&respdata,size);
}

void sqlrprotocol_teradata::appendSsoMech(const byte_t *mech, size_t mechsize) {

	debugWrite("mech oid:");
	debugHexDump(mech,mechsize);

	write(&respdata,(byte_t)SSO_MECH);
	writeBerEncInt(&respdata,mechsize);
	write(&respdata,mech,mechsize);
}

void sqlrprotocol_teradata::appendGenericCField(byte_t field,
							const byte_t *data,
							uint64_t size) {

	if (size==1) {
		debugWrite("0x%02x data: 0x%02x",field,*data);
	} else {
		debugWrite("0x%02x data:",field);
		debugHexDump(data,size);
	}

	write(&respdata,field);
	writeBerEncInt(&respdata,size);
	write(&respdata,data,size);
}

void sqlrprotocol_teradata::appendC6Field(byte_t mech) {
	write(&respdata,(byte_t)SSO_C6);
	writeBerEncInt(&respdata,950);
	appendSsoGssData(mech);
}

void sqlrprotocol_teradata::appendSsoGssData(byte_t mech) {

	// no known reference (just had to study the trace)

	// gss data header...
	byte_t		version=SSO_GSS_DATA_VERSION_3;
	byte_t		messageclass=SSO_GSS_CLASS_2;
	byte_t		messagekind=SSO_GSS_KIND_1;
	byte_t		flag=1;

	// size of data block sizes + data blocks
	uint32_t	datasize=934;

	// for capabilities...
	//
	// The low bits of capabilities[3] tell the client which flavor of
	// wrap/unwrap to use.  From the jdbc driver's
	// com.teradata.tdgss.jgssp2td2 classes:
	//
	// * 0x01 - the client wraps the way decrypt() expects: it hashes
	//   the shared key and puts a mic inside the ciphertext.  If this
	//   is clear, it uses an older format with no mic, and the key is
	//   the entire shared secret rather than a slice of it.
	// * 0x04 - the client makes one key per qop that we send back,
	//   by cutting the shared secret into consecutive slices.  See
	//   setSharedKey().  If this is clear, the key is the entire
	//   shared secret.
	// * 0x10 - the client uses newWrap()/newUnWrap() instead, which
	//   use an hmac rather than a plain hash, and wrap the token in
	//   ASN.1 DER.  We don't implement that, so we never set it.
	//   bteq sends us 0x10, but takes 0x00 for an answer.
	// * 0x08 - unknown.  We get 0x0d with mechs other than TD2, and
	//   0x05 with TD2, and it makes no difference to the jdbc driver
	//   either way, so we just send back what we get.
	//
	// Note that 0x04 without 0x01 makes the jdbc driver throw an
	// "Unknown peer capabilities" error.
	//
	// See notes in parseSsoGssData().
	byte_t		capabilities[]={
		0x00, 0x00, 0x00, 0x00
	};

	if (mech==MECH_TD2) {
		capabilities[3]|=0x05;
	} else {
		capabilities[3]|=0x0d;
	}

	// more capabilities?
	//
	// See notes in parseSsoGssData().
	byte_t		unknown[]={
		0x00, 0x00, 0x00, 0x00
	};

	write(&respdata,version);
	write(&respdata,messageclass);
	write(&respdata,messagekind);
	write(&respdata,flag);
	writeBE(&respdata,datasize);
	write(&respdata,capabilities,sizeof(capabilities));
	write(&respdata,unknown,sizeof(unknown));

	debugStart("gss data header");
	debugWrite("version: %d",(int)version);
	debugWrite("class: %d",(int)messageclass);
	debugWrite("kind: %d",(int)messagekind);
	debugWrite("flag: %d",(int)flag);
	debugWrite("data size: %d",(int)datasize);
	debugWrite("capabilities:");
	debugHexDump(capabilities,sizeof(capabilities));
	debugWrite("unknown:");
	debugHexDump(unknown,sizeof(unknown));
	debugEnd();


	// gss data block sizes...
	byte_t		gssversion[]={
		// 16.20.12.01
		0x10, 0x14, 0x0c, 0x01
	};

	uint32_t	dhpsize=sizeof(dhp);
	uint32_t	dhgsize=sizeof(dhg);
	uint32_t	unknownsize=0;
	uint32_t	qopssize=102;
	uint32_t	gssstructuresize=0;

	// padding to 80 bytes
	// (this is probably space for the sizes of other data blocks)
	byte_t	pad[36];
	bytestring::zero(pad,sizeof(pad));

	write(&respdata,gssversion,sizeof(gssversion));
	writeBE(&respdata,dhpsize);
	writeBE(&respdata,dhgsize);
	writeBE(&respdata,(uint32_t)serverpubkeysize);
	writeBE(&respdata,unknownsize);
	writeBE(&respdata,qopssize);
	writeBE(&respdata,gssstructuresize);
	write(&respdata,pad,sizeof(pad));

	debugStart("gss data block sizes");
	debugWrite("gss version:");
	debugHexDump(gssversion,sizeof(gssversion));
	debugWrite("dh \"p\" size: %d",(int)dhpsize);
	debugWrite("dh \"g\" size: %d",(int)dhgsize);
	debugWrite("public key size: %d",(int)serverpubkeysize);
	debugWrite("unknown size: %d",(int)unknownsize);
	debugWrite("qops size: %d",(int)qopssize);
	debugWrite("gss structure size: %d",(int)gssstructuresize);
	//debugWrite("padding:");
	//debugHexDump(pad,sizeof(pad));
	debugEnd();


	// gss data blocks...
	debugStart("gss data blocks");
	appendSsoGssKeys();
	appendSsoGssQops();
	debugEnd();
}

void sqlrprotocol_teradata::appendSsoGssKeys() {

	// dh g and p
	write(&respdata,dhp,sizeof(dhp));
	write(&respdata,dhg,sizeof(dhg));

	debugWrite("dh \"p\" (%lld bytes):",(long long)sizeof(dhp));
	debugHexDump(dhp,sizeof(dhp));
	debugWrite("dh \"g\" (%lld bytes):",(long long)sizeof(dhg));
	debugHexDump(dhg,sizeof(dhg));


	// server public key
	write(&respdata,serverpubkey,serverpubkeysize);

	debugWrite("server public key (%lld bytes):",(long long)serverpubkeysize);
	debugHexDump(serverpubkey,serverpubkeysize);
}

void sqlrprotocol_teradata::appendSsoGssQops() {

	// negotiated qops (Quality of Protection)
	write(&respdata,(byte_t)SSORESP_NEGOTIATED_QOPS);
	write(&respdata,(byte_t)100);

	// qop parameters
	// (currently, we only support QOP_AES128_CBC_PKCS5_SHA1_DH2048,
	// so these are the values for that)
	byte_t		confalg=ALG_AES;
	uint16_t	confalgkeysize=128;
	byte_t		mode=CONF_ALG_MODE_CBC;
	byte_t		padding=CONF_ALG_PADDING_PKCS5;
	byte_t		intalg=ALG_SHA1;
	byte_t		kexalg=ALG_DH;
	uint16_t	kexalgkeysize=2048;

	debugStart("negotiated qops");

	// The client sent a set of supported qop algorithms in the
	// SSO Request - trip 0, we'll send a set of supported
	// combinations of them here.
	//
	// (the server always sends 4, and they're all the same here
	// because we only support one qop - they're really 4 key
	// slots, the client cuts the shared secret into one slice
	// per qop and picks one by index, see setSharedKey())
	byte_t	qops[]={
		SSORESP_NEGOTIATED_QOP1,
		SSORESP_NEGOTIATED_QOP2,
		SSORESP_NEGOTIATED_QOP3,
		SSORESP_NEGOTIATED_QOP4
	};
	for (byte_t i=0; i<sizeof(qops); i++) {

		// qop
		write(&respdata,qops[i]);
		write(&respdata,(byte_t)23);

		// confidentiality algorithm
		write(&respdata,(byte_t)CONF_ALG);
		// FIXME: use sizeof(confalg) instead of 1, and similar for all
		write(&respdata,(byte_t)1);
		write(&respdata,confalg);

		// mode
		write(&respdata,(byte_t)CONF_ALG_MODE);
		write(&respdata,(byte_t)1);
		write(&respdata,mode);

		// padding
		write(&respdata,(byte_t)CONF_ALG_PADDING);
		write(&respdata,(byte_t)1);
		write(&respdata,padding);

		// confidentiality algorithm key size
		write(&respdata,(byte_t)CONF_ALG_KEY_SIZE);
		write(&respdata,(byte_t)2);
		writeBE(&respdata,confalgkeysize);

		// integrity algorithm
		write(&respdata,(byte_t)INT_ALG);
		write(&respdata,(byte_t)1);
		write(&respdata,intalg);

		// key exchange algorithm
		write(&respdata,(byte_t)KEX_ALG);
		write(&respdata,(byte_t)1);
		write(&respdata,kexalg);

		// key exchange algorithm key size
		write(&respdata,(byte_t)KEX_ALG_KEY_SIZE);
		write(&respdata,(byte_t)2);
		writeBE(&respdata,kexalgkeysize);

		debugStart("qop %d",i+1);
		debugWrite("conf alg: %s",algstr[confalg]);
		debugWrite("mode: %s",confalgmodestr[mode]);
		debugWrite("padding: %s",confalgpaddingstr[padding]);
		debugWrite("conf alg key size: %d",confalgkeysize);
		debugWrite("integrity alg: %s",algstr[intalg]);
		debugWrite("kex alg: %s",algstr[kexalg]);
		debugWrite("kex alg key size: %d",kexalgkeysize);
		debugEnd();
	}
	debugEnd();
}

void sqlrprotocol_teradata::appendSuccessParcel() {

	// see Teradata CLIv2, page 312

	// statement number (FIXME: see note in appendEndStatementParcel)
	uint16_t	statementnumber=(req)?1:0;

	// handle the activity count
	updateActivityCount();
	uint32_t	activitycount=(req)?req->activitycount:0;

	// warning code (always 0 as SQL Relay doesn't support warnings)
	uint16_t	warningcode=0;

	// field count
	uint16_t	fieldcount=0;
	if (req) {
		req->fieldcount=cont->colCount(req->cur);
		fieldcount=req->fieldcount;
	}

	// activity type
	uint16_t	activity=(req)?req->activity:0;

	// warning size (always 0 as SQL Relay doesn't support warnings)
	uint16_t	warningsize=0;

	// warning message (always empty as SQL Relay doesn't support warnings)
	const char	*warning="";

	debugParcelStart("send","success",8);
	debugWrite("statement number: %d",statementnumber);
	debugWrite("activity count: %d",activitycount);
	debugWrite("warning code: %d",warningcode);
	debugWrite("field count: %d",fieldcount);
	debugWrite("activity: %d",activity);
	debugWrite("warning size: %d",warningsize);
	debugWrite("warning: %.*s",warningsize,warning);
	debugParcelEnd();

	// success parcel...
	appendParcelHeader(8,16);

	write(&respdata,statementnumber);
	if (req) {
		req->activitycountpos=respdata.getPosition();
		req->activitycountsize=4;
	}
	write(&respdata,activitycount);
	write(&respdata,warningcode);
	write(&respdata,fieldcount);
	write(&respdata,activity);
	write(&respdata,warningsize);
	write(&respdata,warning,warningsize);

	// "slack" bytes
	write(&respdata,(uint16_t)0);
}

void sqlrprotocol_teradata::updateActivityCount() {

	if (!req) {
		return;
	}

	// set the activity count from the affected rows
	// (unless it's already been set)
	if (!req->activitycount) {
		req->activitycount=cont->getAffectedRows(req->cur);
	}

	// fudge some particular activity counts
	if ((int64_t)req->activitycount==-1) {
		req->activitycount=0;
	}
	switch (req->activity) {
		case SQL_DATABASE:
			req->activitycount=1;
			break;
		case SQL_DROP_TABLE:
			req->activitycount=26;
			break;
	}
}

void sqlrprotocol_teradata::appendStatementStatusParcel() {
	appendStatementStatusParcel(1);
}

void sqlrprotocol_teradata::appendStatementStatusParcel(
						uint32_t statementnumber) {
	// see Teradata CLIv2, page 310

	bool	includeext=(req->activity==SQL_SELECT && req->requestmode!='R');

	// statement status ???
	byte_t	statementstatus=0;

	// response mode
	byte_t	responsemode=0;
	switch (req->requestmode) {
		case 'F':
			responsemode=1;
			break;
		case 'R':
			responsemode=2;
			break;
		case 'I':
			responsemode=3;
			break;
		case 'M':
			responsemode=4;
			break;
	}

	// code (FIXME: ???)
	uint16_t	code=0;

	// handle the activity count
	updateActivityCount();

	// get the field count
	req->fieldcount=cont->colCount(req->cur);

	debugParcelStart("send","statement status",205);
	debugWrite("statement status: %d",statementstatus);
	debugWrite("response mode: %d",responsemode);
	debugWrite("statement number: %d",statementnumber);
	debugWrite("code: %d",code);
	debugWrite("activity: %d",req->activity);
	debugWrite("activity count: %lld",(long long)req->activitycount);
	debugWrite("field count: %d",req->fieldcount);
	debugParcelEnd();

	// statement status parcel...
	appendParcelHeader(205,(includeext)?46:28);

	write(&respdata,statementstatus);
	write(&respdata,responsemode);
	// reserved
	write(&respdata,(uint16_t)0);
	write(&respdata,statementnumber);
	write(&respdata,code);
	write(&respdata,req->activity);
	req->activitycountpos=respdata.getPosition();
	req->activitycountsize=8;
	write(&respdata,req->activitycount);
	write(&respdata,req->fieldcount);
	// reserved
	write(&respdata,(uint32_t)0);

	if (includeext) {

		// ess extension header
		// or
		// warning message extension
		// or
		// merge activity counts extension
		// or
		// multiload activity counts extension

		// FIXME: I don't know which of those this is...
		uint16_t	ext1=32;
		uint32_t	ext2=12;
		byte_t		ext[]={
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};
		write(&respdata,ext1);
		write(&respdata,ext2);
		write(&respdata,ext,sizeof(ext));
	}
}

void sqlrprotocol_teradata::appendColumnParcels() {

	switch (req->requestmode) {
		case 'F':
			appendFieldColumnParcels();
			break;
		case 'R':
			// in record mode we don't actually send anything
			return;
		case 'I':
		case 'M':
			if (req->returnstatementinfo=='Y') {
				appendStatementInfoParcel();
				appendStatementInfoEndParcel();
			} else {
				appendDataInfoParcel();
			}
			return;
	}
}

void sqlrprotocol_teradata::appendFieldColumnParcels() {

	uint16_t	colcount=cont->colCount(req->cur);

	// column names...
	appendTitleStartParcel();
	for (uint16_t i=0; i<colcount; i++) {
		appendFieldParcel(
			cont->getColumnName(req->cur,i),
			cont->getColumnNameSize(req->cur,i));
	}
	appendTitleEndParcel();

	// column formats...
	appendFormatStartParcel();
	stringbuffer	fieldformat;
	for (uint16_t i=0; i<colcount; i++) {
		getFieldFormat(&fieldformat,i);
		appendFieldParcel(fieldformat.getString(),
					fieldformat.getSize());
		fieldformat.clear();
	}
	appendFormatEndParcel();

	// column sizes...
	appendSizeStartParcel();
	for (uint16_t i=0; i<colcount; i++) {
		appendSizeParcel(cont->getColumnSize(req->cur,i));
	}
	appendSizeEndParcel();
}

void sqlrprotocol_teradata::getFieldFormat(bytebuffer *fieldformat,
							uint16_t col) {

	const char	*type=getColumnTypeName(col);

	if (!charstring::compare(type,"TINYINT") ||
		!charstring::compare(type,"SMALLINT") ||
		!charstring::compare(type,"INTEGER")) {
		fieldformat->printf("-(%d)9",
				cont->getColumnSize(req->cur,col));
	} else if (!charstring::compare(type,"DECIMAL")) {
		uint16_t	prec=cont->getColumnPrecision(req->cur,col);
		uint16_t	scale=cont->getColumnScale(req->cur,col);
		for (uint16_t i=0; i<prec-scale; i++) {
			fieldformat->write('-');
		}
		if (scale) {
			fieldformat->write('.');
		}
		for (uint16_t i=0; i<scale; i++) {
			fieldformat->write('9');
		}
	} else if (!charstring::compare(type,"NUMBER")) {
		fieldformat->write("FN9");
	} else if (!charstring::compare(type,"FLOAT")) {
		fieldformat->write("-9.99999999999999E-999");
	} else if (!charstring::compare(type,"DATE")) {
		fieldformat->write("YY/MM/DD");
	} else if (!charstring::compare(type,"TIME")) {
		// FIXME: HH24?
		fieldformat->write("HH:MI:SS.S(6)");
	} else if (!charstring::compare(type,"TIMESTAMP") ||
			!charstring::compare(type,"DATETIME")) {
		// FIXME: HH24?
		fieldformat->write("YYYY-MM-DDBHH:MI:SS.S(6)");
	} else {
		// fall back to char/varchar
		fieldformat->printf("X(%d)",
				cont->getColumnSize(req->cur,col));
	}
}

void sqlrprotocol_teradata::appendFieldParcel(const char *data, uint16_t size) {

	// see Teradata CLIv2, page 262

	debugParcelStart("send","field",18);
	debugHexDump((const byte_t *)data,size);
	debugParcelEnd();
	appendParcelHeader(18,size);
	write(&respdata,data,size);
}

void sqlrprotocol_teradata::appendDataInfoParcel() {

	// see Teradata CLIv2, page 253

	uint16_t	fieldcount=cont->colCount(req->cur);

	appendParcelHeader(71,2+fieldcount*(2+2));

	// field count
	write(&respdata,fieldcount);

	debugParcelStart("send","datainfo",71);
	debugWrite("field count: %d",fieldcount);

	for (uint16_t i=0; i<fieldcount; i++) {

		// type
		uint16_t	type=getColumnType(i);
		write(&respdata,type);

		// size
		uint16_t	size=cont->getColumnSize(req->cur,i);
		write(&respdata,size);

		debugStart("field %d",i);
		debugWrite("type: %d",type);
		debugWrite("size: %d",size);
		debugEnd();
	}

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendStatementInfoParcel() {

	// see Teradata CLIv2, page 303

	appendParcelHeader(169);
	debugParcelStart("send","statementinformation",169);

	appendEstimatedProcessingTimeExtension(0);
	appendEndEstimatedProcessingTimeExtension();
	uint16_t	colcount=cont->colCount(req->cur);
	for (uint16_t i=0; i<colcount; i++) {
		appendQueryExtension(i);
	}
	appendEndQueryExtension();

	// FIXME - extensions include:
	// 1 - parameter
	// 2 - query
	// 3 - summary
	// 4 - identity column
	// 5 - stored-proc output
	// 6 - stored-proc resultset
	// 7 - estimated processing time
	// when should we send the other ones, and in what order?

	endParcel();

	debugParcelEnd();
}

void sqlrprotocol_teradata::appendEstimatedProcessingTimeExtension(
							uint64_t time) {

	debugExtStart("estimated processing time");
	debugWrite("time: %lld",(long long)time);
	debugExtEnd();

	// PBTILOUT - statistics layout
	write(&respdata,(uint16_t)3);
	// PBTIID - estimated processing time
	write(&respdata,(uint16_t)7);
	// PBTILEN
	write(&respdata,(uint16_t)8);
	// data
	write(&respdata,(uint64_t)time);
}

void sqlrprotocol_teradata::appendEndEstimatedProcessingTimeExtension() {

	debugExtStart("end estimated processing time");
	debugExtEnd();

	// PBTILOUT - end-info
	write(&respdata,(uint16_t)4);
	// PBTIID - estimated processing time
	write(&respdata,(uint16_t)7);
	// PBTILEN
	write(&respdata,(uint16_t)0);
}

void sqlrprotocol_teradata::appendQueryExtension(uint16_t col) {

	debugExtStart((req->function=='E')?"query (limited)":"query (full)");
	
	// PBTILOUT - limited/full layout
	write(&respdata,(uint16_t)((req->function=='E')?2:1));
	// PBTIID - query
	write(&respdata,(uint16_t)2);
	// PBTILEN
	size_t	sizepos=respdata.getPosition();
	write(&respdata,(uint16_t)0);

	// data...
	size_t		startpos=respdata.getPosition();

	// PBTIFDB
	// db name (FIXME: we don't know this)
	uint16_t	pbtifdblen=0;
	const char	*pbtifdb="";

	// PBTIFTB
	// table name
	uint16_t	pbtiftblen=cont->getColumnTableSize(req->cur,col);
	const char	*pbtiftb=cont->getColumnTable(req->cur,col);

	// PBTIFCN
	// column name
	uint16_t	pbtifcnlen=cont->getColumnNameSize(req->cur,col);
	const char	*pbtifcn=cont->getColumnName(req->cur,col);

	// PBTIFCP
	// position in table (0 if not in a table (or unknown?))
	uint16_t	pbtifcp=0;

	// PBTIFAN
	// alias (FIXME: we don't know this, as opposed to name)
	uint16_t	pbtifanlen=cont->getColumnNameSize(req->cur,col);
	const char	*pbtifan=cont->getColumnName(req->cur,col);

	// PBTIFT
	// column "title" (FIXME: we don't know this, as opposed to name)
	uint16_t	pbtiftlen=cont->getColumnNameSize(req->cur,col);
	const char	*pbtift=cont->getColumnName(req->cur,col);

	// PBTIFF
	// column format
	bytebuffer	fieldformat;
	getFieldFormat(&fieldformat,col);
	uint16_t	pbtifflen=fieldformat.getSize();
	const byte_t	*pbtiff=fieldformat.getBuffer();

	// PBTIFDV
	// default (FIXME: we don't know this)
	uint16_t	pbtifdvlen=0;
	const char	*pbtifdv="";

	// PBTIFIC
	// identity
	char	pbtific=(cont->getColumnIsAutoIncrement(req->cur,col))?'Y':'N';

	// PBTIFDW
	// writable based on user's permission
	char	pbtifdw='U';

	// PBTIFNL
	// nullable
	char	pbtifnl=(cont->getColumnIsNullable(req->cur,col))?'Y':'N';

	// PBTIFMN
	// nulls can be returned
	// (FIXME: we don't know this, as opposed to nullable)
	char	pbtifmn=(cont->getColumnIsNullable(req->cur,col))?'Y':'N';

	// PBTIFSR
	// permitted in where clause
	// (FIXME: there could be other types that aren't allowed)
	int16_t	type=cont->getColumnType(req->cur,col);
	char	pbtifsr=(cont->isBlobType(type) ||
				cont->isClobType(type))?'N':'Y';

	// PBTIFWR
	// writable (i.e. is not an expression)
	// (FIXME: we don't know this but can't return a U)
	char	pbtifwr='Y';

	// PBTIFDT
	// data type
	uint16_t	pbtifdt=getColumnType(col);

	// PBTIFUT
	// UDT type
	// 1 = structured
	// 2 = distinct
	// 3 = internal
	// 0 = ambiguous
	// (FIXME: we don't know this)
	uint16_t	pbtifut=0;

	// PBTIFTY
	// type name
	uint16_t	pbtiftylen=getColumnTypeNameSize(col);
	const char	*pbtifty=getColumnTypeName(col);

	// PBTIFMI
	// misc info... (FIXME: ???)
	uint16_t	pbtifmi=0;

	// PBTIFMDL
	// byte length
	uint64_t	pbtifmdl=cont->getColumnSize(req->cur,col);

	// PBTIFND
	// precision
	uint16_t	pbtifnd=cont->getColumnPrecision(req->cur,col);

	// PBTIFNID
	// interval digits (FIXME: we don't know this)
	uint16_t	pbtifnid=0;

	// PBTIFNFD
	// scale
	uint16_t	pbtifnfd=cont->getColumnScale(req->cur,col);

	// PBTIFCT
	// charset
	// 1 = Latin
	// 2 = Unicode
	// 3 = Jap Shift-JIS
	// 4 = Graphic
	// 5 = Kanji
	// 0 = not char data
	// (FIXME: we don't know this)
	byte_t	pbtifct=(cont->isBitType(type) ||
				cont->isBoolType(type) ||
				cont->isFloatType(type) ||
				cont->isNumberType(type) ||
				cont->isBlobType(type) ||
				cont->isClobType(type) ||
				cont->isUnsignedType(type) ||
				cont->isBinaryType(type) ||
				cont->isDateTimeType(type))?0:1;

	// PBTIFMNC
	// char length
	// (FIXME: we don't know this, as opposed to byte length)
	uint64_t	pbtifmnc=(cont->isBitType(type) ||
				cont->isBoolType(type) ||
				cont->isFloatType(type) ||
				cont->isNumberType(type) ||
				cont->isBlobType(type) ||
				cont->isClobType(type) ||
				cont->isUnsignedType(type) ||
				cont->isBinaryType(type) ||
				cont->isDateTimeType(type))?
				0:cont->getColumnSize(req->cur,col);

	// PBTIFCS
	// case sensitive (FIXME: we don't know this)
	char	pbtifcs='U';

	// PBTIFSN
	// signed
	char	pbtifsn=(cont->getColumnIsUnsigned(req->cur,col))?'N':'Y';

	// PBTIFK
	// uniquely describes the row
	// (FIXME: we don't know this, except for auto-increment columns)
	char	pbtifk=(cont->getColumnIsAutoIncrement(req->cur,col))?'Y':'U';

	// PBTIFU
	// unique
	char	pbtifu=(cont->getColumnIsUnique(req->cur,col))?'Y':'N';

	// PBTIFE
	// expression (FIXME: we don't know this)
	char	pbtife='U';

	// PBTIFSO
	// permitted in order-by (FIXME: we don't know this)
	char	pbtifso='U';

	// tweak various byte-lengths and precisions
	if (!charstring::compare(pbtifty,"DECIMAL") ||
			!charstring::compare(pbtifty,"NUMBER")) {
		if (pbtifnd<3) {
			pbtifmdl=1;
		} else if (pbtifnd<5) {
			pbtifmdl=2;
		} else if (pbtifnd<10) {
			pbtifmdl=4;
		} else {
			pbtifmdl=8;
		}
	} else if (!charstring::compare(pbtifty,"FLOAT")) {
		pbtifmdl=8;
	} else if (!charstring::compare(pbtifty,"DATE")) {
		pbtifmdl=4;
	} else if (!charstring::compare(pbtifty,"TIME")) {
		pbtifnd=15;
	} else if (!charstring::compare(pbtifty,"TIMESTAMP") ||
			!charstring::compare(pbtifty,"DATETIME")) {
		pbtifnd=26;
	}

	if (getDebug()) {
		debugStart("col %d",col);

		if (req->function!='E') {
			debugWrite("PBTIFDB - db name: %.*s",
					pbtifdblen,pbtifdb);
			debugWrite("PBTIFTB - table name: %.*s",
					pbtiftblen,pbtiftb);
			debugWrite("PBTIFCN - column name: %.*s",
					pbtifcnlen,pbtifcn);
			debugWrite("PBTIFCP - position in table: %d",
					pbtifcp);
			debugWrite("PBTIFAN - alias: %.*s",
					pbtifanlen,pbtifan);
			debugWrite("PBTIFT - column title: %.*s",
					pbtiftlen,pbtift);
			debugWrite("PBTIFF - column format: %.*s",
					pbtifflen,pbtiff);
			debugWrite("PBTIFDV - default: %.*s",
					pbtifdvlen,pbtifdv);
			debugWrite("PBTIFIC - identity: %c",
					pbtific);
			debugWrite("PBTIFDW - writable (permission): %c",
					pbtifdw);
			debugWrite("PBTIFNL - nullable: %c",
					pbtifnl);
			debugWrite("PBTIFMN - nulls can be returned: %c",
					pbtifmn);
			debugWrite("PBTIFSR - permitted in where clause: %c",
					pbtifsr);
			debugWrite("PBTIFWR - writable: %c",
					pbtifwr);
		}

		debugWrite("PBTIFDT - data type: 0x%04x (%d)",
					pbtifdt,pbtifdt);

		if (req->function!='E') {
			debugWrite("PBTIFUT - UDT type: %d",
					pbtifut);
			debugWrite("PBTIFTY - type name: %.*s",
					pbtiftylen,pbtifty);
			debugWrite("PBTIFMI - misc info: %d",
					pbtifmi);
			debugWrite("PBTIFMDL - byte length: %lld",
					(long long)pbtifmdl);
		}

		debugWrite("PBTIFND - precision: %d",pbtifnd);
		debugWrite("PBTIFNID - interval digits: %d",pbtifnid);
		debugWrite("PBTIFNFD - scale: %d",pbtifnfd);

		if (req->function!='E') {
			debugWrite("PBTIFCT - charset: %d",
					pbtifct);
			debugWrite("PBTIFMNC - char length: %lld",
					(long long)pbtifmnc);
			debugWrite("PBTIFCS - case sensitive: %c",
					pbtifcs);
			debugWrite("PBTIFSN - signed: %c",
					pbtifsn);
			debugWrite("PBTIFK - uniquely describes the row: %c",
					pbtifk);
			debugWrite("PBTIFU - unique: %c",
					pbtifu);
			debugWrite("PBTIFE - expression: %c",
					pbtife);
			debugWrite("PBTIFSO - permitted in order-by: %c",
					pbtifso);
		}

		debugEnd();
	}
	debugExtEnd();

	if (req->function!='E') {
		write(&respdata,pbtifdblen);
		write(&respdata,pbtifdb,pbtifdblen);
		write(&respdata,pbtiftblen);
		write(&respdata,pbtiftb,pbtiftblen);
		write(&respdata,pbtifcnlen);
		write(&respdata,pbtifcn,pbtifcnlen);
		write(&respdata,pbtifcp);
		write(&respdata,pbtifanlen);
		write(&respdata,pbtifan,pbtifanlen);
		write(&respdata,pbtiftlen);
		write(&respdata,pbtift,pbtiftlen);
		write(&respdata,pbtifflen);
		write(&respdata,pbtiff,pbtifflen);
		write(&respdata,pbtifdvlen);
		write(&respdata,pbtifdv,pbtifdvlen);
		write(&respdata,pbtific);
		write(&respdata,pbtifdw);
		write(&respdata,pbtifnl);
		write(&respdata,pbtifmn);
		write(&respdata,pbtifsr);
		write(&respdata,pbtifwr);
	}
	write(&respdata,pbtifdt);
	if (req->function!='E') {
		write(&respdata,pbtifut);
		write(&respdata,pbtiftylen);
		write(&respdata,pbtifty,pbtiftylen);
		write(&respdata,pbtifmi);
	}
	write(&respdata,pbtifmdl);
	write(&respdata,pbtifnd);
	write(&respdata,pbtifnid);
	write(&respdata,pbtifnfd);
	if (req->function!='E') {
		write(&respdata,pbtifct);
		write(&respdata,pbtifmnc);
		write(&respdata,pbtifcs);
		write(&respdata,pbtifsn);
		write(&respdata,pbtifk);
		write(&respdata,pbtifu);
		write(&respdata,pbtife);
		write(&respdata,pbtifso);

		// ??? (not described in spec)
		byte_t	unknown[]={
			0x55,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};
		write(&respdata,unknown,sizeof(unknown));
	}

	// backpatch length
	size_t	endpos=respdata.getPosition();
	respdata.setPositionRelativeToBeginning(sizepos);
	respdata.write(hostTo((uint16_t)(endpos-startpos)));
	respdata.setPositionRelativeToBeginning(endpos);
}

void sqlrprotocol_teradata::appendEndQueryExtension() {

	debugExtStart("end query");
	debugExtEnd();
	
	// PBTILOUT - end-info
	write(&respdata,(uint16_t)4);
	// PBTIID - query
	write(&respdata,(uint16_t)2);
	// PBTILEN
	write(&respdata,(uint16_t)0);
}

void sqlrprotocol_teradata::appendStatementInfoEndParcel() {
	// see Teradata CLIv2, page 310
	appendParcelHeader(170,0);
	debugParcelStart("send","statementinformationend",170);
	debugParcelEnd();
}

void sqlrprotocol_teradata::appendRowParcels(bool *eors) {

	// get column count
	uint16_t	colcount=cont->colCount(req->cur);

	// reset activity count (FIXME: see note below)
	req->activitycount=0;

	// fetch rows (unless we need to resend this row)
	bool	error;
	while (req->resendrow || cont->fetchRow(req->cur,&error)) {

		// reset resend-row flag
		req->resendrow=false;

		// We need to make sure we don't try to send too much data at
		// once.  So, keep track of where we are in the send-data
		// buffer.  If we write too much data to it, we'll truncate it
		// here and resend the row when we get a continue request.
		size_t	pos=respdata.getPosition();

		// append fields
		for (uint16_t i=0; i<colcount; i++) {

			const char	*field=NULL;
			uint64_t	fieldsize=0;
			bool		lob=false;
			bool		null=false;
			if (!cont->getField(req->cur,i,
						&field,&fieldsize,
						&lob,&null)) {
				// FIXME: handle error
			}

			// bail if appending this field would be too much
			if (respdata.getSize()+fieldsize>maxmessagesize) {
				respdata.truncate(pos);
				req->resendrow=true;
				*eors=false;
				return;
			}

			appendField(i,field,fieldsize,null);
		}

		// FIXME: Somehow we have to know the total row count and send
		// it back in the 205 for the first group of rows.  Just sending
		// back the count for this group isn't right, and causes the
		// client to send a report a warning.
		req->activitycount++;

		// FIXME: kludgy
		cont->nextRow(req->cur);
	}

	if (error) {
		// FIXME: handle error
	}

	// if we got here then we fetched all rows,
	// and are at the end of the result set
	*eors=true;
}

void sqlrprotocol_teradata::backpatchActivityCount() {
	size_t	originalpos=respdata.getPosition();
	respdata.setPositionRelativeToBeginning(req->activitycountpos);
	if (req->activitycountsize==4) {
		respdata.write(hostTo((uint32_t)req->activitycount));
	} else {
		respdata.write(hostTo(req->activitycount));
	}
	respdata.setPositionRelativeToBeginning(originalpos);
}

void sqlrprotocol_teradata::appendTitleStartParcel() {
	// see Teradata CLIv2, page 313
	debugParcelStart("send","title start",20);
	debugParcelEnd();
	appendParcelHeader(20,0);
}

void sqlrprotocol_teradata::appendTitleEndParcel() {
	// see Teradata CLIv2, page 313
	debugParcelStart("send","title end",21);
	debugParcelEnd();
	appendParcelHeader(21,0);
}

void sqlrprotocol_teradata::appendFormatStartParcel() {
	// see Teradata CLIv2, page 265
	debugParcelStart("send","format start",22);
	debugParcelEnd();
	appendParcelHeader(22,0);
}

void sqlrprotocol_teradata::appendFormatEndParcel() {
	// see Teradata CLIv2, page 265
	debugParcelStart("send","format end",23);
	debugParcelEnd();
	appendParcelHeader(23,0);
}

void sqlrprotocol_teradata::appendSizeStartParcel() {
	// see Teradata CLIv2, page 302
	debugParcelStart("send","size start",24);
	debugParcelEnd();
	appendParcelHeader(24,0);
}

void sqlrprotocol_teradata::appendSizeEndParcel() {
	// see Teradata CLIv2, page 301
	debugParcelStart("send","size end",25);
	debugParcelEnd();
	appendParcelHeader(25,0);
}

void sqlrprotocol_teradata::appendSizeParcel(uint16_t size) {
	// see Teradata CLIv2, page 301
	debugParcelStart("send","size",26);
	debugWrite("size: %d",size);
	debugParcelEnd();
	appendParcelHeader(26,2);
	write(&respdata,size);
}

void sqlrprotocol_teradata::appendRecStartParcel() {
	// see Teradata CLIv2, page 292
	debugParcelStart("send","rec start",27);
	debugParcelEnd();
	appendParcelHeader(27,0);
}

void sqlrprotocol_teradata::appendRecEndParcel() {
	// see Teradata CLIv2, page 287
	debugParcelStart("send","rec end",28);
	debugParcelEnd();
	appendParcelHeader(28,0);
}

void sqlrprotocol_teradata::appendField(uint16_t col, 
						const char *field,
						uint64_t fieldsize,
						bool null) {
	if (!col) {
		if (req->requestmode=='F') {
			appendRecStartParcel();
		} else {
			req->currentfield=0;
			req->nibuffer.clear();
			req->rowbuffer.clear();
		}
	}

	switch (req->requestmode) {
		case 'F':
			appendFieldParcel(field,fieldsize);
			break;
		case 'R':
			appendRecordModeField(col,field,fieldsize,false);
			break;
		case 'I':
		case 'M':
			appendIndicatorModeField(col,field,fieldsize,null);
			break;
	}

	if (col==cont->colCount(req->cur)-1) {
		if (req->requestmode=='F') {
			appendRecEndParcel();
		} else {
			appendRecordParcel();
		}
	}
}

void sqlrprotocol_teradata::appendRecordModeField(uint16_t col, 
							const char *field,
							uint64_t fieldsize,
							bool null) {

	// get column type
	const char	*type=getColumnTypeName(col);

	// FIXME: various calls below assume that field is null-terminated,
	// which may not be true for all backends

	if (!charstring::compare(type,"TINYINT")) {

		if (null) {
			write(&req->rowbuffer,(byte_t)0);
		} else {
			uint16_t	val=charstring::convertToInteger(field);
			write(&req->rowbuffer,(byte_t)val);
		}

	} else if (!charstring::compare(type,"SMALLINT")) {

		if (null) {
			write(&req->rowbuffer,(uint16_t)0);
		} else {
			uint16_t	val=charstring::convertToInteger(field);
			write(&req->rowbuffer,val);
		}

	} else if (!charstring::compare(type,"INTEGER")) {

		if (null) {
			write(&req->rowbuffer,(uint32_t)0);
		} else {
			uint32_t	val=charstring::convertToInteger(field);
			write(&req->rowbuffer,val);
		}

	} else if (!charstring::compare(type,"BIGINT")) {

		if (null) {
			write(&req->rowbuffer,(uint64_t)0);
		} else {
			uint64_t	val=charstring::convertToInteger(field);
			write(&req->rowbuffer,val);
		}

	} else if (!charstring::compare(type,"CHAR") ||
			!charstring::compare(type,"BYTE")) {

		if (null) {
			uint32_t	len=cont->getColumnSize(req->cur,col);
			for (uint32_t i=0; i<len; i++) {
				write(&req->rowbuffer,' ');
			}
		} else {
			write(&req->rowbuffer,field,fieldsize);
		}

	} else if (!charstring::compare(type,"VARCHAR") ||
			!charstring::compare(type,"VARBYTE")) {

		if (null) {
			write(&req->rowbuffer,(uint16_t)0);
		} else {
			write(&req->rowbuffer,(uint16_t)fieldsize);
			write(&req->rowbuffer,field,fieldsize);
		}

	} else if (!charstring::compare(type,"DECIMAL") ||
			!charstring::compare(type,"NUMBER")) {

		if (null) {
			uint16_t	prec=cont->getColumnPrecision(
								req->cur,col);
			if (prec<3) {
				write(&req->rowbuffer,(byte_t)0);
			} else if (prec<5) {
				write(&req->rowbuffer,(uint16_t)0);
			} else if (prec<10) {
				write(&req->rowbuffer,(uint32_t)0);
			} else {
				write(&req->rowbuffer,(uint64_t)0);
			}
		} else {
			char	*temp=charstring::duplicate(field,fieldsize);
			charstring::bothTrim(temp);
			charstring::strip(temp,'.');
			int64_t		val=charstring::convertToInteger(temp);
			delete[] temp;
			uint16_t	prec=cont->getColumnPrecision(
								req->cur,col);
			if (prec<3) {
				write(&req->rowbuffer,(byte_t)val);
			} else if (prec<5) {
				write(&req->rowbuffer,(uint16_t)val);
			} else if (prec<10) {
				write(&req->rowbuffer,(uint32_t)val);
			} else {
				write(&req->rowbuffer,(uint64_t)val);
			}
		}

	} else if (!charstring::compare(type,"FLOAT")) {

		if (null) {
			write(&req->rowbuffer,(uint64_t)0);
		} else {
			double		val=charstring::convertToFloat(field);
			uint64_t	bytes;
			bytestring::copy(&bytes,&val,sizeof(bytes));
			write(&req->rowbuffer,bytes);
		}

	} else if (!charstring::compare(type,"DATE")) {

		if (null) {
			write(&req->rowbuffer,(uint32_t)0);
		} else {
			int16_t	year=0;
			int16_t	month=0;
			int16_t	day=0;
			int16_t	hour=0;
			int16_t	minute=0;
			int16_t	second=0;
			int32_t	fraction=0;
			bool	isnegative=false;
			if (datetime::parse(field,
						false,
						false,
						"-",
						&year,
						&month,
						&day,
						&hour,
						&minute,
						&second,
						&fraction,
						&isnegative)) {
				// 32-bit signed two’s complement integer, most
				// significant byte first (4 bytes); DATE is
				// calculated as follows:
				// (year-1900)*10000 + month*100 + day 
				uint32_t	date=
					(year-1900)*10000+month*100+day;
				write(&req->rowbuffer,date);
			} else {
				write(&req->rowbuffer,(uint32_t)0);
			}
		}

	} else if (!charstring::compare(type,"TIME")) {

		uint16_t	scale=cont->getColumnScale(req->cur,col);
		if (null) {
			write(&req->rowbuffer,"         ");
			if (scale) {
				req->rowbuffer.printf("%*s",scale,"");
			}
		} else {
			// apparently returned as text in this format:
			// hh:mm:ss.ffffff
			int16_t	year=0;
			int16_t	month=0;
			int16_t	day=0;
			int16_t	hour=0;
			int16_t	minute=0;
			int16_t	second=0;
			int32_t	fraction=0;
			bool	isnegative=false;
			if (datetime::parse(field,
						false,
						false,
						"-",
						&year,
						&month,
						&day,
						&hour,
						&minute,
						&second,
						&fraction,
						&isnegative)) {
				req->rowbuffer.printf(
						"%02d:%02d:%02d",
						hour,minute,second);
				if (scale) {
					req->rowbuffer.printf(".%0*d",
								scale,fraction);
				}
			} else {
				write(&req->rowbuffer,"         ");
				if (scale) {
					req->rowbuffer.printf("%*s",scale,"");
				}
			}
		}

	} else if (!charstring::compare(type,"TIMESTAMP") ||
			!charstring::compare(type,"DATETIME")) {

		uint16_t	scale=cont->getColumnScale(req->cur,col);
		if (null) {
			write(&req->rowbuffer,"                   ");
			if (scale) {
				req->rowbuffer.printf("%*s",scale,"");
			}
		} else {
			// apparently returned as text in this format:
			// yyyy-mm-dd hh:mm:ss.ffffff
			int16_t	year=0;
			int16_t	month=0;
			int16_t	day=0;
			int16_t	hour=0;
			int16_t	minute=0;
			int16_t	second=0;
			int32_t	fraction=0;
			bool	isnegative=false;
			if (datetime::parse(field,
						false,
						false,
						"-",
						&year,
						&month,
						&day,
						&hour,
						&minute,
						&second,
						&fraction,
						&isnegative)) {
				if (year<0) {
					year=0;
				}
				if (month<0) {
					month=0;
				}
				if (day<0) {
					day=0;
				}
				if (hour<0) {
					hour=0;
				}
				if (minute<0) {
					minute=0;
				}
				if (second<0) {
					second=0;
				}
				if (fraction<0) {
					fraction=0;
				}
				req->rowbuffer.printf(
					"%04d-%02d-%02d %02d:%02d:%02d",
					year,month,day,
					hour,minute,second);
				if (scale) {
					req->rowbuffer.printf(".%0*d",
								scale,fraction);
				}
			} else {
				write(&req->rowbuffer,"                   ");
				if (scale) {
					req->rowbuffer.printf("%*s",scale,"");
				}
			}
		}
	}

	req->currentfield++;
}

void sqlrprotocol_teradata::appendIndicatorModeField(uint16_t col, 
							const char *field,
							uint64_t fieldsize,
							bool null) {

	// append to nibuffer
	byte_t	ni=(req->currentfield%8)?req->nibuffer[req->currentfield/8]:0;

	// make sure to cast the right side of the << to a byte
	// or some compilers (native CC on Unixware 7.0.1) will fail with:
	// internal compiler error: ... Runs out of registers
	ni|=((byte_t)null)<<((byte_t)(7-(req->currentfield%8)));

	req->nibuffer[req->currentfield/8]=ni;

	// append to rowbuffer (also increments currentfield)
	appendRecordModeField(col,field,fieldsize,null);
}

void sqlrprotocol_teradata::appendRecordParcel() {

	// record - see Teradata CLIv2, page 287
	// multipartrecord - see Teradata CLIv2, page 272

	// FIXME: There's something that's supposed to be different about
	// multipart records...  Like, if the row is bigger than some
	// particular size, then the data needs to be split across multiple
	// records, or something.

	// send a (multipart)record parcel...

	uint16_t	parcelflavor=(req->requestmode=='M')?144:10;
	const char	*parcelname=(req->requestmode=='M')?
					"multipartrecord":"record";

	debugParcelStart("send",parcelname,parcelflavor);
	if (getDebug()) {
		if (req->requestmode=='I' || req->requestmode=='M') {
			debugWrite("null indicator:");
			stringbuffer	b;
			for (uint16_t i=0; i<req->nibuffer.getCount(); i++) {
				b.printBits(req->nibuffer[i]);
				debugWrite("%s",b.getString());
				b.clear();
				debugWrite(" (%02x)",req->nibuffer[i]);
			}
		}
		debugWrite("row buffer:");
		debugHexDump(req->rowbuffer.getBuffer(),
				req->rowbuffer.getSize());
	}
	debugParcelEnd();

	appendParcelHeader(parcelflavor,req->nibuffer.getCount()+
						req->rowbuffer.getSize());

	// null indicator
	if (req->requestmode=='I' || req->requestmode=='M') {
		for (uint16_t i=0; i<req->nibuffer.getCount(); i++) {
			write(&respdata,req->nibuffer[i]);
		}
	}

	// data
	write(&respdata,req->rowbuffer.getBuffer(),req->rowbuffer.getSize());

	// send an endmultipartrecord parcel...
	if (req->requestmode=='M') {
		debugParcelStart("send","endmultipartrecord",145);
		debugParcelEnd();
		appendParcelHeader(145,0);
	}
}

void sqlrprotocol_teradata::appendFailureParcel(const char *errorstring,
						uint16_t errorsize) {
	// see Teradata CLIv2, page 261

	debugParcelStart("send","failure",9);
	debugWrite("error: %.*s",errorsize,errorstring);
	debugParcelEnd();

	// failure parcel
	appendParcelHeader(9,2+2+2+2+errorsize+2+3);

	// statement number (FIXME: see note in appendEndStatementParcel)
	write(&respdata,(uint16_t)1);

	// info ???
	write(&respdata,(uint16_t)0);

	// code (FIXME: ???)
	byte_t code[]={
		0xdf, 0x0e
	};
	write(&respdata,code,sizeof(code));

	// size
	write(&respdata,errorsize);

	// msg
	write(&respdata,errorstring);

	// ins count ???
	write(&respdata,(uint16_t)1);

	// ins traid ???
	byte_t instriad[]={
		0x00, 0x08, 0x0a
	};
	write(&respdata,instriad,sizeof(instriad));
}

void sqlrprotocol_teradata::appendErrorParcel(const char *errorstring) {

	// see Teradata CLIv2, page 259

	uint16_t	errorsize=charstring::getLength(errorstring);

	debugParcelStart("send","error",49);
	debugWrite("error: %.*s",errorsize,errorstring);
	debugParcelEnd();

	// error parcel...
	appendParcelHeader(49,6+errorsize+2);

	// statement number (FIXME: see note in appendEndStatementParcel)
	write(&respdata,(uint16_t)1);

	// info ???
	write(&respdata,(uint16_t)0);

	// code (FIXME: ???)
	byte_t code[]={
		0xa3, 0x0e
	};
	write(&respdata,code,sizeof(code));

	// size
	write(&respdata,errorsize);

	// msg
	write(&respdata,errorstring);

	// ??? (not described in spec)
	byte_t	unknown[]={
		0x00, 0x00
	};
	write(&respdata,unknown,sizeof(unknown));
}

void sqlrprotocol_teradata::appendEndStatementParcel() {

	// FIXME: in a multi-statement request, statements are numbered 1-n,
	// we currently only support 1 statement per request so we're always
	// sending 1, but we ought to support multiple statements
	// someday...
	appendEndStatementParcel(1);
}

void sqlrprotocol_teradata::appendEndStatementParcel(uint16_t statementnumber) {

	// see Teradata CLIv2, page 258

	debugParcelStart("send","end statement",11);
	debugWrite("statement number: %d",statementnumber);
	debugParcelEnd();

	appendParcelHeader(11,2);

	write(&respdata,statementnumber);
}

void sqlrprotocol_teradata::appendEndRequestParcel() {
	// see Teradata CLIv2, page 257
	debugParcelStart("send","end request",12);
	debugParcelEnd();
	appendParcelHeader(12,0);
}

void sqlrprotocol_teradata::appendCursorErrorParcel() {
	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(req->cur,&errorstring,
					&errorsize,
					&errnum,
					&liveconnection);
	respdata.clear();
	appendFailureParcel(errorstring,errorsize);
}

void sqlrprotocol_teradata::appendConnectionErrorParcel() {
	const char	*errorstring;
	uint32_t	errorsize;
	int64_t		errnum;
	bool		liveconnection;
	cont->getError(&errorstring,
				&errorsize,
				&errnum,
				&liveconnection);
	respdata.clear();
	appendFailureParcel(errorstring,errorsize);
}

void sqlrprotocol_teradata::unexpectedParcel(uint16_t parcelflavor) {
	debugWrite("recv unexpected parcel: %d",parcelflavor);
snooze::macrosnooze(10);
}

bool sqlrprotocol_teradata::noParcelFound(const byte_t *parcel) {

	// if we've run off the end of the clientreqdata,
	// then no parcel was found
	return (parcel>=(clientreqdata+clientreqdatasize));
}

bool sqlrprotocol_teradata::noParcelFound(const byte_t *parcel,
						const char *expected) {

	// if we've run off the end of the clientreqdata,
	// then no parcel was found
	if (parcel>=(clientreqdata+clientreqdatasize)) {
		debugWrite("no parcel found, expected %s",expected);
		return true;
	}
	return false;
}

uint16_t sqlrprotocol_teradata::getActivity() {

	// skip whitespace and comments
	req->query=cont->skipWhitespaceAndComments(req->query);

	// FIXME: this is slow, use a dictionary (or something)...
	if (!charstring::compareIgnoringCase(req->query,"sel ",4) ||
		!charstring::compareIgnoringCase(req->query,"select ",7)) {
		return SQL_SELECT;
	} else if (!charstring::compareIgnoringCase(req->query,
						"create table ",13)) {
		return SQL_CREATE_TABLE;
	} else if (!charstring::compareIgnoringCase(req->query,
						"drop table ",11)) {
		return SQL_DROP_TABLE;
	} else if (!charstring::compareIgnoringCase(req->query,
						"insert ",7)) {
		return SQL_INSERT;
	} else if (!charstring::compareIgnoringCase(req->query,
						"update ",7)) {
		return SQL_UPDATE;
	} else if (!charstring::compareIgnoringCase(req->query,
						"delete ",7)) {
		return SQL_DELETE;
	} else if (!charstring::compareIgnoringCase(req->query,
						"database ",9)) {
		return SQL_DATABASE;
	} else if (!charstring::compareIgnoringCase(req->query,"bt",2) ||
			!charstring::compareIgnoringCase(req->query,
						"begin transaction",17)) {
		return SQL_BEGIN_TRANSACTION;
	} else if (!charstring::compareIgnoringCase(req->query,"et",2) ||
			!charstring::compareIgnoringCase(req->query,
						"end transaction",15)) {
		return SQL_END_TRANSACTION;
	} else if (!charstring::compareIgnoringCase(req->query,
						"commit",6)) {
		return ANSI_SQL_COMMIT_WORK;
	} else if (!charstring::compareIgnoringCase(req->query,
						"rollback",8)) {
		return SQL_ROLLBACK;
	} else if (!charstring::compareIgnoringCase(req->query,
						"help ",5)) {
		return SQL_HELP;
	} else if (!charstring::compareIgnoringCase(req->query,
						"begin loading ",14)) {
		return BEGIN_LOADING;
	} else if (!charstring::compareIgnoringCase(req->query,
						"checkpoint loading ",19)) {
		return CHECK_POINT_LOAD;
	} else if (!charstring::compareIgnoringCase(req->query,
						"end loading",11)) {
		return END_LOADING;
	} else if (!charstring::compareIgnoringCase(req->query,
						"set query_band ",15)) {
		return SET_QUERY_BAND_STMT;
	} else if (!charstring::compareIgnoringCase(req->query,
						"check workload for ",19)) {
		return CHECK_WORKLOAD_FOR;
	} else if (!charstring::compareIgnoringCase(req->query,
						"check workload end",18)) {
		return CHECK_WORKLOAD_END;
	}
	// FIXME: support more activities
	return 0;
}

bool sqlrprotocol_teradata::activityReturnsResults() {
	return (req->activity==SQL_SELECT ||
		req->activity==SQL_HELP ||
		req->activity==CHECK_WORKLOAD_END ||
		req->fudgeselect);
}

const char *sqlrprotocol_teradata::getColumnTypeName(uint16_t col) {

	const char	*type=cont->getColumnTypeName(req->cur,col);

	// Some backends (ODBC) return DATETIME or TIMESTAMP for DATE,
	// TIME, and TIMESTAMP types.  They are distinguishable by
	// column size though.
	if (!charstring::compare(type,"TIMESTAMP") ||
			!charstring::compare(type,"DATETIME")) {
		uint32_t	len=cont->getColumnSize(req->cur,col);
		if (len==10) {
			type="DATE";
		} else if (len==15) {
			type="TIME";
		}
	}
	return type;
}

uint16_t sqlrprotocol_teradata::getColumnTypeNameSize(uint16_t col) {

	uint16_t	typelen=cont->getColumnTypeNameSize(req->cur,col);

	// Some backends (ODBC) return DATETIME or TIMESTAMP for DATE,
	// TIME, and TIMESTAMP types.  They are distinguishable by
	// column size though.
	const char	*type=cont->getColumnTypeName(req->cur,col);
	if (!charstring::compare(type,"TIMESTAMP") ||
			!charstring::compare(type,"DATETIME")) {
		uint32_t	len=cont->getColumnSize(req->cur,col);
		if (len==10 || len==15) {
			typelen=4;
		}
	}
	return typelen;
}

uint16_t sqlrprotocol_teradata::getColumnType(uint16_t col) {

	const char	*type=getColumnTypeName(col);
	uint16_t	retval=0;

	// FIXME: this is slow, use a dictionary (or something)...
	if (!charstring::compare(type,"BLOB")) {
		retval=400;
	}
	// FIXME: ???
	/*if (!charstring::compare(type,"BLOB AS DEFERRED")) {
		retval=404;
	}
	if (!charstring::compare(type,"BLOB AS LOCATOR")) {
		retval=408;
	}
	if (!charstring::compare(type,"BLOB AS DEFERRED BY NAME")) {
		retval=412;
	}*/
	if (!charstring::compare(type,"CLOB")) {
		retval=416;
	}
	// FIXME: ???
	/*if (!charstring::compare(type,"CLOB AS DEFERRED")) {
		retval=420;
	}
	if (!charstring::compare(type,"CLOB AS LOCATOR")) {
		retval=424;
	}*/
	if (!charstring::compare(type,"UDT")) {
		retval=432;
	}
	// FIXME: ???
	/*if (!charstring::compare(type,"Distinct UDT")) {
		retval=436;
	}
	if (!charstring::compare(type,"Structure UDT")) {
		retval=440;
	}*/
	if (!charstring::compare(type,"VARCHAR")) {
		retval=448;
	}
	if (!charstring::compare(type,"CHAR")) {
		retval=452;
	}
	if (!charstring::compare(type,"LONGVARCHAR")) {
		retval=456;
	}
	if (!charstring::compare(type,"VARGRAPHIC")) {
		retval=464;
	}
	if (!charstring::compare(type,"GRAPHIC")) {
		retval=468;
	}
	if (!charstring::compare(type,"LONGVARGRAPHIC")) {
		retval=472;
	}
	if (!charstring::compare(type,"FLOAT")) {
		retval=480;
	}
	if (!charstring::compare(type,"DECIMAL")) {
		retval=484;
	}
	if (!charstring::compare(type,"INTEGER")) {
		retval=496;
	}
	if (!charstring::compare(type,"SMALLINT")) {
		retval=500;
	}
	if (!charstring::compare(type,"ARRAY")) {
		// one-dimension
		retval=504;
	}
	// FIXME: ???
	/*if (!charstring::compare(type,"ARRAY - multiple dimensions")) {
		// multi-dimension
		retval=508;
	}*/
	if (!charstring::compare(type,"BIGINT")) {
		retval=600;
	}
	if (!charstring::compare(type,"NUMBER")) {
		retval=604;
	}
	if (!charstring::compare(type,"VARBYTE")) {
		retval=688;
	}
	if (!charstring::compare(type,"BYTE")) {
		retval=692;
	}
	if (!charstring::compare(type,"LONGVARBYTE")) {
		retval=696;
	}
	if (!charstring::compare(type,"DATE")) {
		// ANSI format (YYYY-MM-DD string)
		//retval=748;
		// Teradata format (4-byte number)
		retval=752;
	}
	if (!charstring::compare(type,"BYTEINT") ||
		!charstring::compare(type,"TINYINT")) {
		retval=756;
	}
	if (!charstring::compare(type,"TIME")) {
		retval=760;
	}
	if (!charstring::compare(type,"TIMESTAMP") ||
		!charstring::compare(type,"DATETIME")) {
		retval=764;
	}
	// FIXME: ???
	/*if (!charstring::compare(type,"TIME WITH TIME ZONE")) {
		retval=768;
	}
	if (!charstring::compare(type,"TIMESTAMP WITH TIME ZONE")) {
		retval=772;
	}
	if (!charstring::compare(type,"INTERVAL YEAR")) {
		retval=776;
	}
	if (!charstring::compare(type,"INTERVAL YEAR TO MONTH")) {
		retval=780;
	}
	if (!charstring::compare(type,"INTERVAL MONTH")) {
		retval=784;
	}
	if (!charstring::compare(type,"INTERVAL DAY")) {
		retval=788;
	}
	if (!charstring::compare(type,"INTERVAL DAY TO HOUR")) {
		retval=792;
	}
	if (!charstring::compare(type,"INTERVAL DAY TO MINUTE")) {
		retval=796;
	}
	if (!charstring::compare(type,"INTERVAL DAY TO SECOND")) {
		retval=800;
	}
	if (!charstring::compare(type,"INTERVAL HOUR")) {
		retval=804;
	}
	if (!charstring::compare(type,"INTERVAL HOUR TO MINUTE")) {
		retval=808;
	}
	if (!charstring::compare(type,"INTERVAL HOUR TO SECOND")) {
		retval=812;
	}
	if (!charstring::compare(type,"INTERVAL MINUTE")) {
		retval=816;
	}
	if (!charstring::compare(type,"INTERVAL MINUTE TO SECOND")) {
		retval=820;
	}
	if (!charstring::compare(type,"INTERVAL SECOND")) {
		retval=824;
	}
	if (!charstring::compare(type,"PERIOD (DATE)")) {
		retval=832;
	}
	if (!charstring::compare(type,"PERIOD (TIME)")) {
		retval=836;
	}
	if (!charstring::compare(type,"PERIOD (TIME WITH TIME ZONE)")) {
		retval=840;
	}
	if (!charstring::compare(type,"PERIOD (TIMESTAMP)")) {
		retval=844;
	}
	if (!charstring::compare(type,"PERIOD (TIMESTAMP WITH TIME ZONE)")) {
		retval=848;
	}*/
	if (!charstring::compare(type,"XML")) {
		retval=852;
	}
	// FIXME: ???
	/*if (!charstring::compare(type,"XML Text Deferred")) {
		retval=856;
	}
	if (!charstring::compare(type,"XML Text Locator")) {
		retval=860;
	}*/
	// FIXME: SQL Relay supports more types than this, and we need to map
	// all of them to Teradata types...

	// add 1 of the column is nullable
	if (retval && cont->getColumnIsNullable(req->cur,col)) {
		retval++;
	}
	return retval;
}

void sqlrprotocol_teradata::debugParcelStart(const char *direction,
						const char *flavorname,
						uint16_t parcelflavor,
						uint32_t parceldatasize) {
	debugStart("%s %s parcel - %d (%d bytes) (%s)",
			direction,flavorname,parcelflavor,parceldatasize,
			(getProtocolIsBigEndian())?"BE":"LE");
}

void sqlrprotocol_teradata::debugParcelEnd(const byte_t *parceldata,
						uint32_t parceldatasize) {
#ifdef DEBUG_PARCEL_END
	debugWrite("parcel was:");
	debugHexDump(parceldata,parceldatasize);
#endif
	debugEnd();
}

void sqlrprotocol_teradata::debugParcelStart(const char *direction,
						const char *flavorname,
						uint16_t parcelflavor) {
	debugStart("%s %s parcel - %d",direction,flavorname,parcelflavor);
}

void sqlrprotocol_teradata::debugParcelEnd() {
	debugEnd();
}

void sqlrprotocol_teradata::debugExtStart(const char *extname) {
	debugStart("%s ext",extname);
}

void sqlrprotocol_teradata::debugExtEnd() {
	debugEnd();
}

void sqlrprotocol_teradata::debugMech(const byte_t *oid, size_t size) {

	if (!getDebug()) {
		return;
	}
}

void sqlrprotocol_teradata::debugSessionOption(const char *name, char value) {
	// unset options come across as 0
	if (character::isPrintable(value)) {
		debugWrite("%s: %c",name,value);
	} else {
		debugWrite("%s: 0x%02x",name,(byte_t)value);
	}
}

bool sqlrprotocol_teradata::generateEphemeralKeys() {

	// Diffie-Hellman Key Exchange...
	//
	// * I use DHp (DH prime modulus (large prime number)) and
	//   DHg (DH generator AKA base) to generate my private/public key
	//   pair
	// * I send DHp, DHg, and my public key to the client
	// * The client uses my DHp and DHg to generate their private/public
	//   key pair
	// * The client sends me their public key
	// * I use my private key and their public key to generate a secret
	// * They use their private key and my public key to generate a secret
	// * These secrets should be the same (shared secret)
	// * We negotiate a cipher (eg. AES128_GCM_PKCS5_SHA256)
	// * We negotiate a Key Derivation Function (KDF) - usually a hash like
	//   MD5, SHA1, or SHA256 but could be PBKDF2, or something else.
	// * If the KDF requires a salt then we have to negotiate a salt as well
	// * We both use the KDF to generate a shared key
	//   * If the KDF generates a larger key than required (eg. SHA256
	//     generates a 256-bit (32-byte) key but AES128 only requires a
	//     128-bit (16-byte) key) then the key is typically just
	//     truncated to the necessary length
	// * The shared key should also be the same
	// * We both use the shared key as the key for our cipher
	// * We both use the cipher for symmetric encryption and decryption

	debugStart("generate ephemeral keys");

	// clear the server public key buffer
	serverpubkey=NULL;
	serverpubkeysize=0;
	serverprivkey=NULL;
	serverprivkeysize=0;

	// reset the dh
	dh.reset();
	dh.setPrimeModulus(dhp,sizeof(dhp));
	dh.setGenerator(dhg,sizeof(dhg));

	// generate new public/private keys
	if (!dh.generateKeys()) {
		debugWrite("generate keys failed");
		debugEnd();
		return false;
	}

	// get the server pub/priv keys
	serverpubkey=dh.getPublicKey();
	serverpubkeysize=dh.getPublicKeySize();

	serverprivkey=dh.getPrivateKey();
	serverprivkeysize=dh.getPrivateKeySize();

	debugWrite("server public key (%lld bytes):",(long long)serverpubkeysize);
	debugHexDump(serverpubkey,serverpubkeysize);
	debugWrite("server private key (%lld bytes):",(long long)serverprivkeysize);
	debugHexDump(serverprivkey,serverprivkeysize);

	debugEnd();
	return true;
}

bool sqlrprotocol_teradata::generateSharedSecret() {

	// See generateEphemeralKeys() for an explanation of
	// Diffie-Hellman Key Exchange

	debugStart("generate shared secret");

	// set the client public key
	// FIXME: is the key guaranteed to be sizeof(clientpubkey) bytes,
	// or could it be shorter?
	dh.setPeerPublicKey(clientpubkey,sizeof(clientpubkey));

	// handle success/failure
	if (!dh.generateSharedSecret()) {
		delete[] sharedsecret;
		sharedsecret=NULL;
		sharedsecretsize=0;

		debugWrite("generate shared secret failed");
		debugEnd();
		return false;
	}

	// get the shared secret
	sharedsecret=dh.getSharedSecret();
	sharedsecretsize=dh.getSharedSecretSize();

	debugWrite("shared secret (%d bytes):",sharedsecretsize);
	debugHexDump(sharedsecret,sharedsecretsize);

	debugEnd();
	return true;
}

bool sqlrprotocol_teradata::setSharedKey(byte_t qopindex) {

	debugStart("set shared key (qop %d)",(int)qopindex);

	// There's no key derivation function.  The client cuts the shared
	// secret into consecutive slices, one per qop that we sent back in
	// the sso response parcel (trip 1), each one as long as that qop's
	// key, and uses the slice itself as the key.  The qop index in the
	// td2 token header says which slice to use.
	//
	// We send back SSORESP_NEGOTIATED_QOP_COUNT qops and they're all the
	// negotiated qop, so all of the slices are the same size.
	//
	// (this is what the client is doing when it generates one hash per
	// slice of the shared secret - one hashed key per qop, see below)
	//
	// (if we didn't set the 0x04 bit in the capabilities that we send
	// then the client would use the entire shared secret as the key)

	if (!sharedsecret) {
		debugWrite("no shared secret");
		debugEnd();
		return false;
	}
	if (qopindex>=SSORESP_NEGOTIATED_QOP_COUNT) {
		debugWrite("invalid qop index");
		debugEnd();
		return false;
	}

	sharedkeysize=qopsharedkeysize[negotiatedqop];
	if (!sharedkeysize) {
		debugWrite("no negotiated qop");
		debugEnd();
		return false;
	}

	uint32_t	offset=qopindex*sharedkeysize;
	if (offset+sharedkeysize>sharedsecretsize) {
		debugWrite("shared secret too small");
		debugEnd();
		return false;
	}

	bytestring::copy(sharedkey,sharedsecret+offset,sharedkeysize);
	debugWrite("shared key (%d bytes, offset %d):",sharedkeysize,offset);
	debugHexDump(sharedkey,sharedkeysize);

	// hash the shared key using the negotiated integrity algorithm
	// (the mic that comes along with the encrypted data is computed over
	// this hash, rather than over the key itself)
	//
	// (currently, we only support QOP_AES128_CBC_PKCS5_SHA1_DH2048,
	// so the integrity algorithm is always sha1)
	sha1	s1;
	if (!s1.append(sharedkey,sharedkeysize)) {
		debugWrite("s1.append() failed");
		debugEnd();
		return false;
	}
	const byte_t	*hash=s1.getHash();
	if (!hash) {
		debugWrite("s1.getHash() failed");
		debugEnd();
		return false;
	}
	hsharedkeysize=s1.getHashSize();
	bytestring::copy(hsharedkey,hash,hsharedkeysize);

	debugWrite("hashed shared key (%d bytes):",hsharedkeysize);
	debugHexDump(hsharedkey,hsharedkeysize);

	debugEnd();
	return true;
}

bool sqlrprotocol_teradata::decrypt(const byte_t *encdata,
						uint64_t encdatasize,
						bytebuffer *decdata) {

	// The encrypted data is:
	//
	//   ciphertext, then a 16-byte td2 token header
	//
	// and the token header doubles as the initialization vector.
	//
	// Decrypting the ciphertext gets us:
	//
	//   the data, then a mic, then a copy of the token header
	//
	// So the mic is a trailer, inside the ciphertext, not a header
	// outside of it, and the initialization vector is repeated at the
	// end of the plaintext so that we can tell that it wasn't tampered
	// with.
	//
	// For the connect message, the data is the tail of the lan header
	// followed by the parcels.  See ENCRYPTED_LAN_HEADER_SIZE.
	//
	// no known reference, this came out of the jdbc driver's
	// com.teradata.tdgss.jgssp2td2.Td2Crypto.unwrap() method

	debugStart("decrypt (%lld bytes)",(long long)encdatasize);

	// create the decryptor
	// (aes128 uses CBC and PKCS5 by default,
	// so we don't need to set those anywhere)
	aes128	a;

	// get the initialization vector size
	size_t	ivsize=a.getIvSize();

	// validate the encdata
	if (encdatasize<=ivsize) {
		debugWrite("encdata too small");
		debugEnd();
		return false;
	}

	// the last N bytes of the encdata are the td2 token header
	const byte_t	*token=encdata+encdatasize-ivsize;

	// parse the token header
	const byte_t	*ptr=token;
	byte_t		version;
	byte_t		msgtype;
	byte_t		flags;
	byte_t		qopindex;
	uint32_t	msglength;
	uint64_t	seqnum;
	read(ptr,&version,&ptr);
	read(ptr,&msgtype,&ptr);
	read(ptr,&flags,&ptr);
	read(ptr,&qopindex,&ptr);
	readBE(ptr,&msglength,&ptr);
	readBE(ptr,&seqnum,&ptr);

	debugStart("td2 token header");
	debugWrite("version: %d",(int)version);
	debugWrite("message type: %d",(int)msgtype);
	debugWrite("flags: 0x%02x",(int)flags);
	debugWrite("qop index: %d",(int)qopindex);
	debugWrite("message length: %d",(int)msglength);
	debugWrite("sequence number: %lld",(long long)seqnum);
	debugHexDump(token,ivsize);
	debugEnd();

	// we only know how to decrypt version-3 wrap
	// tokens with the privacy flag set
	if (version!=TD2TOKEN_VERSION_3 || msgtype!=TD2TOKEN_TYPE_WRAP ||
				!(flags&TD2TOKEN_FLAG_PRIVACY)) {
		debugWrite("unsupported td2 token");
		debugEnd();
		return false;
	}

	// The message length is the number of bytes of ciphertext
	// immediately preceding the token header.
	//
	// The jdbc driver just treats everything up to the token header as
	// ciphertext, rather than trusting this length.  We use the length
	// so that if there ever is anything in front of the ciphertext, it
	// shows up below instead of quietly corrupting the first block.
	if (msglength>encdatasize-ivsize) {
		debugWrite("invalid td2 token message length");
		debugEnd();
		return false;
	}
	const byte_t	*ciphertext=token-msglength;

	uint64_t	prefixsize=encdatasize-ivsize-msglength;
	if (prefixsize) {
		debugWrite("unexpected %lld bytes before "
				"the ciphertext:",(long long)prefixsize);
		debugHexDump(encdata,prefixsize);
	}

	// determine the shared key for the qop that the client chose
	if (!setSharedKey(qopindex)) {
		debugEnd();
		return false;
	}
	if (sharedkeysize<a.getKeySize()) {
		debugWrite("shared key too small");
		debugEnd();
		return false;
	}

	a.setIv(token,ivsize);
	a.setKey(sharedkey,a.getKeySize());

	debugWrite("qop: %s",qopstr[negotiatedqop]);
	debugWrite("iv (%lld bytes):",(long long)ivsize);
	debugHexDump(token,ivsize);
	debugWrite("key (%lld bytes):",(long long)a.getKeySize());
	debugHexDump(sharedkey,a.getKeySize());
	debugWrite("ciphertext (%d bytes):",msglength);
	debugHexDump(ciphertext,msglength);

	// set the data to decrypt
	if (!a.append(ciphertext,msglength)) {
		debugWrite("append failed: %d",a.getError());
		debugEnd();
		return false;
	}

	// get the decrypted data
	const byte_t	*ddata=a.getDecryptedData();
	uint64_t	ddatasize=a.getDecryptedDataSize();
	if (!ddata) {
		debugWrite("decryption failed: %d",a.getError());
		debugEnd();
		return false;
	}

	debugWrite("plaintext (%lld bytes):",(long long)ddatasize);
	debugHexDump(ddata,ddatasize);

	// split the plaintext into parcels, mic, and token header copy
	if (ddatasize<hsharedkeysize+ivsize) {
		debugWrite("plaintext too small");
		debugEnd();
		return false;
	}
	uint64_t	datasize=ddatasize-hsharedkeysize-ivsize;
	const byte_t	*mic=ddata+datasize;
	const byte_t	*tokencopy=mic+hsharedkeysize;

	debugWrite("mic (%d bytes):",hsharedkeysize);
	debugHexDump(mic,hsharedkeysize);

	// the copy of the token header must match the one we decrypted with
	if (bytestring::compare(tokencopy,token,ivsize)) {
		debugWrite("td2 token header mismatch:");
		debugHexDump(tokencopy,ivsize);
		debugEnd();
		return false;
	}

	// verify the mic
	// (it's a hash of the parcels, the hashed shared key, and the
	// token header - the hashed shared key stands in for the mic
	// itself, which occupies those bytes on the wire)
	sha1	s1;
	if (!s1.append(ddata,(uint32_t)datasize) ||
			!s1.append(hsharedkey,hsharedkeysize) ||
			!s1.append(tokencopy,(uint32_t)ivsize)) {
		debugWrite("s1.append() failed");
		debugEnd();
		return false;
	}
	const byte_t	*hash=s1.getHash();
	if (!hash) {
		debugWrite("s1.getHash() failed");
		debugEnd();
		return false;
	}
	if (s1.getHashSize()!=hsharedkeysize ||
			bytestring::compare(hash,mic,hsharedkeysize)) {
		debugWrite("mic mismatch:");
		debugHexDump(hash,s1.getHashSize());
		debugEnd();
		return false;
	}

	// copy out the parcels
	decdata->append(ddata,datasize);

	debugWrite("decrypted data (%lld bytes)",(long long)datasize);
	debugHexDump(decdata->getBuffer(),decdata->getSize());

	debugEnd();

	return true;
}

bool sqlrprotocol_teradata::encrypt(const byte_t *decdata,
						uint64_t decdatasize,
						bytebuffer *encdata) {

	// FIXME: implement this

	// this should be formatted the same way as the
	// encrypted data that we receive (see decrypt()):
	// * encrypt the parcels, a mic, and a copy of the td2 token
	//   header, using the shared key, with the token header as
	//   the initialization vector
	// * append a copy of the token header to the ciphertext

	return true;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrprotocol	*new_sqlrprotocol_teradata(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrprotocol_teradata(cont,parameters);
	}
}
