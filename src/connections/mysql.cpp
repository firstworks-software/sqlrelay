// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/regularexpression.h>

#include <defines.h>
#include <datatypes.h>
#include <config.h>

#include <mysql.h>
#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID>=32200
	#include <errmsg.h>
#endif

// MySQL 8+ doesn't have my_bool, but MariaDB 10+ does
#ifndef MARIADB_BASE_VERSION
	#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID>=80000
		typedef bool my_bool;
	#endif
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

// mysql_change_user() once worked, but it hasn't for a long time.  It's not
// clear why, but the problem appears to be on the server-side.  I'm disabling
// it until I can figure out a workaround.
#undef HAVE_MYSQL_CHANGE_USER

class mysqlconnection;

class SQLRSERVER_DLLSPEC mysqlcursor : public sqlrservercursor {
	friend class mysqlconnection;
	private:
		mysqlcursor(sqlrserverconnection *conn, uint16_t id);
		~mysqlcursor();

		void		allocateResultSetBuffers(int32_t columncount);
		void		deallocateResultSetBuffers();
#ifdef HAVE_MYSQL_STMT_PREPARE
		bool		open();
		bool		close();
#endif
		bool		prepareQuery(const char *query,
						uint32_t size);
		bool		supportsNativeBinds(const char *query,
							uint32_t size);
#ifdef HAVE_MYSQL_STMT_PREPARE
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						int64_t *value);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						double *value,
						uint32_t precision,
						uint32_t scale);
		bool		inputBind(const char *variable,
						uint16_t variablesize,
						int64_t year,
						int16_t month,
						int16_t day,
						int16_t hour,
						int16_t minute,
						int16_t second,
						int32_t microsecond,
						const char *tz,
						bool isnegative,
						int16_t *isnull);
		bool		inputBindBlob(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		bool		inputBindClob(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
#endif
		bool		executeQuery(const char *query,
						uint32_t size);
#ifdef HAVE_MYSQL_COMMIT
		bool		queryIsNotSelect();
#endif
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
		bool		knowsRowCount();
		uint64_t	rowCount();
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
#ifdef HAVE_MYSQL_FIELD_NAME_LENGTH
		uint16_t	getColumnNameSize(uint32_t col);
#endif
		uint16_t	getColumnType(uint32_t col);
		uint32_t	getColumnSize(uint32_t col);
		uint32_t	getColumnPrecision(uint32_t col);
		uint32_t	getColumnScale(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		uint16_t	getColumnIsPrimaryKey(uint32_t col);
		uint16_t	getColumnIsUnique(uint32_t col);
		uint16_t	getColumnIsPartOfKey(uint32_t col);
		uint16_t	getColumnIsUnsigned(uint32_t col);
		uint16_t	getColumnIsZeroFilled(uint32_t col);
		uint16_t	getColumnIsBinary(uint32_t col);
		uint16_t	getColumnIsAutoIncrement(uint32_t col);
#ifdef HAVE_MYSQL_FIELD_ORG_TABLE
		const char	*getColumnTable(uint32_t col);
#endif
#ifdef HAVE_MYSQL_FIELD_ORG_TABLE_LENGTH
		uint16_t	getColumnTableSize(uint32_t col);
#endif
		bool		noRowsToReturn();
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null);

#ifdef HAVE_MYSQL_STMT_PREPARE
		bool		getLobFieldLength(uint32_t col,
						uint64_t *size);
		bool		getLobFieldSegment(uint32_t col,
						char *buffer,
						uint64_t buffersize,
						uint64_t offset,
						uint64_t charstoread,
						uint64_t *charsread);
		void		closeLobField(uint32_t col);
#endif

		void		closeResultSet();
		void		freeResult();

		bool		columnInfoIsValidAfterPrepare();

		void		encodeBlob(stringbuffer *buffer,
					const char *data, uint32_t datasize);

		MYSQL_RES	*mysqlresult;
		MYSQL_FIELD	**mysqlfields;
		unsigned int	ncols;
		my_ulonglong	nrows;
		my_ulonglong	affectedrows;
		int		queryresult;

#ifdef HAVE_MYSQL_STMT_PREPARE
		MYSQL_STMT	*stmt;
		bool		stmtreset;
		bool		stmtfreeresult;
		bool		stmtpreparefailed;

		MYSQL_RES	*mysqlmetadata;

		MYSQL_BIND	*fieldbind;
		char		*field;
		my_bool		*isnull;
		unsigned long	*fieldsize;

		bool		boundvariables;
		uint16_t	maxbindcount;
		MYSQL_BIND	*bind;
		unsigned long	*bindvaluesize;
		MYSQL_TIME	*bindtime;

		MYSQL_BIND	lobfield;
		unsigned long	lobfieldlength;

		bool		usestmtprepare;
		bool		bindformaterror;

		regularexpression	unsupportedbystmt;
#endif
		MYSQL_ROW	mysqlrow;
		unsigned long	*mysqlrowsizes;

		mysqlconnection	*mysqlconn;
};

class SQLRSERVER_DLLSPEC mysqlconnection : public sqlrserverconnection {
	friend class mysqlcursor;
	public:
		mysqlconnection(sqlrservercontroller *cont);
		~mysqlconnection();
	private:
		void		handleConnectString();
		bool		logIn(const char **error, const char **warning);
		sqlrservercursor	*newCursor(uint16_t id);
		void		deleteCursor(sqlrservercursor *curs);
		void		logOut();
		bool		isTransactional();
#ifdef HAVE_MYSQL_PING
		bool		ping();
#endif
		const char	*getDbType();
		const char	*getDbVersion();
		const char	*getDbHostName();
#ifdef HAVE_MYSQL_STMT_PREPARE
		const char	*getBindFormat();
#endif
		const char	*getNextvalFormat();
		const char	*getCatalogListQuery(
						const char *catalog);
		const char	*getSchemaListQuery(
						const char *catalog,
						const char *schema);
		const char	*getTableTypeListQuery(
						const char *catalog,
						const char *schema,
						const char *tabletypes);
		const char	*getTableListQuery(
						const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes);
		const char	*getTypeInfoListQuery(
						const char *catalog,
						const char *schema,
						const char *type);
		const char	*getColumnListQuery(
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column);
		const char	*getPrimaryKeysListQuery(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getKeyAndIndexListQuery(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getProcedureListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure);
		bool		getDatabaseIsSchema();
		char		*getCurrentCatalog();
		const char	*selectSchemaQuery();
		const char	*getCurrentSchemaQuery();
		const char	*getDefaultIsolationLevel();
		const char	*setIsolationLevelQuery();
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		bool		getLastInsertId(uint64_t *id);
		const char	*getNoopQuery();
		bool		setAutoCommitOn();
		bool		setAutoCommitOff();
		bool		commit();
		bool		rollback();
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
#ifdef HAVE_MYSQL_STMT_PREPARE
		int16_t		getNonNullBindValue();
		int16_t		getNullBindValue();
#endif
		void		endSession();

#if MYSQL_VERSION_ID<32200
		MYSQL	mysql;
#endif
		MYSQL	*mysqlptr;

		bool	connected;

		const char	*db;
		const char	*host;
		const char	*port;
		const char	*socket;
		const char	*charset;
		const char	*sslmodestr;
#ifdef HAVE_MYSQL_OPT_SSL_MODE
		unsigned int	sslmode;
#endif
#ifdef HAVE_MYSQL_OPT_SSL_ENFORCE
		const my_bool	*sslenforce;
#endif
#ifdef HAVE_MYSQL_OPT_SSL_VERIFY_SERVER_CERT
		const my_bool	*sslverifyservercert;
#endif
		const char	*tlsversion;
		const char	*sslkey;
		const char	*sslcert;
		const char	*sslcipher;
		const char	*sslca;
		const char	*sslcapath;
		const char	*sslcrl;
		const char	*sslcrlpath;
		bool		foundrows;
		bool		ignorespace;

		bool		usestmtapi;

		char	*dbversion;
		char	**databasefeatures;
		char	*dbhostname;

		stringbuffer	tablelistquery;
		stringbuffer	procedurelistquery;
		stringbuffer	cataloglistquery;
		stringbuffer	schemalistquery;
		stringbuffer	tabletypelistquery;
		stringbuffer	columnlistquery;
		stringbuffer	typeinfolistquery;
		stringbuffer	primarykeyslistquery;
		stringbuffer	keyandindexlistquery;
		stringbuffer	procedureparameterlistquery;

		static const my_bool	mytrue;
		static const my_bool	myfalse;

		bool		firstquery;

		stringbuffer	loginerror;

		bool		escapeblobs;

		uint64_t	lastinsertid;
};

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_mysqlconnection(
						sqlrservercontroller *cont) {
		return new mysqlconnection(cont);
	}
}

const my_bool	mysqlconnection::mytrue=TRUE;
const my_bool	mysqlconnection::myfalse=FALSE;

mysqlconnection::mysqlconnection(sqlrservercontroller *cont) :
					sqlrserverconnection(cont) {
	connected=false;
	dbversion=NULL;
	databasefeatures=NULL;
	dbhostname=NULL;

	// start this at false because we don't need to do a commit before
	// the first query when we very first start up
	firstquery=false;

	mysqlptr=NULL;

#ifdef HAVE_MYSQL_OPT_SSL_MODE
	sslmode=0;
#endif

	lastinsertid=0;
}

mysqlconnection::~mysqlconnection() {
	delete[] dbversion;
	if (databasefeatures) {
		for (int i=0; i<FEATURE_COUNT; i++) {
			delete[] databasefeatures[i];
		}
		delete[] databasefeatures;
	}
	delete[] dbhostname;
}

void mysqlconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	db=cont->getConnectStringValue("db");
	host=cont->getConnectStringValue("host");
	port=cont->getConnectStringValue("port");
	socket=cont->getConnectStringValue("socket");
	charset=cont->getConnectStringValue("charset");
	sslmodestr=cont->getConnectStringValue("sslmode");
#ifdef HAVE_MYSQL_OPT_SSL_MODE
	if (charstring::isNullOrEmpty(sslmodestr) ||
		!charstring::compare(sslmodestr,"disable")) {
		#ifdef HAVE_MYSQL_SSL_MODE_DISABLED
		sslmode=SSL_MODE_DISABLED;
		#endif
	} else if (!charstring::compare(sslmodestr,"prefer")) {
		#ifdef HAVE_MYSQL_SSL_MODE_PREFERRED
		sslmode=SSL_MODE_PREFERRED;
		#endif
	} else if (!charstring::compare(sslmodestr,"require")) {
		#ifdef HAVE_MYSQL_SSL_MODE_REQUIRED
		sslmode=SSL_MODE_REQUIRED;
		#endif
	} else if (!charstring::compare(sslmodestr,"verify-ca")) {
		#ifdef HAVE_MYSQL_SSL_MODE_VERIFY_CA
		sslmode=SSL_MODE_VERIFY_CA;
		#endif
	} else if (!charstring::compare(sslmodestr,"verify-identity")) {
		#ifdef HAVE_MYSQL_SSL_MODE_VERIFY_IDENTITY
		sslmode=SSL_MODE_VERIFY_IDENTITY;
		#endif
	}
#endif
#ifdef HAVE_MYSQL_OPT_SSL_ENFORCE
	sslenforce=&myfalse;
	if (!charstring::compare(sslmodestr,"require") ||
		!charstring::compare(sslmodestr,"verify-ca") ||
		!charstring::compare(sslmodestr,"verify-identity")) {
		sslenforce=&mytrue;
	}
#endif
#ifdef HAVE_MYSQL_OPT_SSL_VERIFY_SERVER_CERT
	sslverifyservercert=&myfalse;
	if (!charstring::compare(sslmodestr,"verify-identity")) {
		sslverifyservercert=&mytrue;
	}
#endif
	tlsversion=cont->getConnectStringValue("tlsversion");
	sslkey=cont->getConnectStringValue("sslkey");
	sslcert=cont->getConnectStringValue("sslcert");
	sslcipher=cont->getConnectStringValue("sslcipher");
	sslca=cont->getConnectStringValue("sslca");
	sslcapath=cont->getConnectStringValue("sslcapath");
	sslcrl=cont->getConnectStringValue("sslcrl");
	sslcrlpath=cont->getConnectStringValue("sslcrlpath");
	foundrows=charstring::isYes(cont->getConnectStringValue("foundrows"));
	ignorespace=charstring::isYes(
			cont->getConnectStringValue("ignorespace"));

	usestmtapi=charstring::compare(
			cont->getConnectStringValue("api"),"classic");

	// mysql doesn't support multi-row fetches
	cont->setFetchAtOnce(1);
}

bool mysqlconnection::logIn(const char **error, const char **warning) {

	// Handle host.
	// For really old versions of mysql, a NULL host indicates that the
	// unix socket should be used.  There's no way to specify what unix
	// socket or inet port to connect to, those values are hardcoded
	// into the client library.
	// For some newer versions, a NULL host causes problems, but an empty
	// string is safe.
#ifdef HAVE_MYSQL_REAL_CONNECT_FOR_SURE
	const char	*hostval=(!charstring::isNullOrEmpty(host))?host:"";
#else
	const char	*hostval=(!charstring::isNullOrEmpty(host))?host:NULL;
#endif

	// Handle db.
	const char	*dbval=(!charstring::isNullOrEmpty(db))?db:"";
	
	// log in
	const char	*user=cont->getUser();
	const char	*password=cont->getPassword();
#ifdef HAVE_MYSQL_REAL_CONNECT_FOR_SURE
	// Handle port and socket.
	int		portval=
			(!charstring::isNullOrEmpty(port))?
					charstring::convertToInteger(port):0;
	const char	*socketval=
			(!charstring::isNullOrEmpty(socket))?socket:NULL;
	unsigned long	clientflag=0;
	#ifdef CLIENT_MULTI_STATEMENTS
		clientflag|=CLIENT_MULTI_STATEMENTS;
	#endif
//clientflag|=(1UL << 17);
	#ifdef CLIENT_FOUND_ROWS
		if (foundrows) {
			clientflag|=CLIENT_FOUND_ROWS;
		}
	#endif
	#ifdef CLIENT_IGNORE_SPACE
		if (ignorespace) {
			clientflag|=CLIENT_IGNORE_SPACE;
		}
	#endif
	#if MYSQL_VERSION_ID>=32200
		// initialize database connection structure
		mysqlptr=mysql_init(NULL);
		if (!mysqlptr) {
			*error="mysql_init failed";
			return false;
		}
		#ifdef HAVE_MYSQL_OPT_SSL_MODE
			mysql_options(mysqlptr,
					MYSQL_OPT_SSL_MODE,
					&sslmode);
		#else
			#ifdef HAVE_MYSQL_OPT_SSL_ENFORCE
				mysql_options(mysqlptr,
						MYSQL_OPT_SSL_ENFORCE,
						sslenforce);
			#endif
			#ifdef HAVE_MYSQL_OPT_SSL_VERIFY_SERVER_CERT
				mysql_options(mysqlptr,
					MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
					sslverifyservercert);
			#endif
		#endif
		#ifdef HAVE_MYSQL_OPT_TLS_VERSION
			mysql_options(mysqlptr,
					MYSQL_OPT_TLS_VERSION,
					tlsversion);
		#endif
		#ifdef HAVE_MYSQL_SSL_SET
			mysql_ssl_set(mysqlptr,sslkey,sslcert,
					sslca,sslcapath,sslcipher);
		#endif
		#ifdef HAVE_MYSQL_OPT_SSLCRL
			mysql_options(mysqlptr,
					MYSQL_OPT_SSLCRL,
					sslcrl);
		#endif
		#ifdef HAVE_MYSQL_OPT_SSLCRLPATH
			mysql_options(mysqlptr,
					MYSQL_OPT_SSLCRLPATH,
					sslcrlpath);
		#endif
	
		bool	sslcafallback=false;
		MYSQL	*result=mysql_real_connect(mysqlptr,
							hostval,
							user,
							password,
							dbval,
							portval,
							socketval,
							clientflag);
		#ifdef HAVE_MYSQL_SSL_SET
			if (!result && mysql_errno(mysqlptr)==2026 &&
				(!charstring::compare(sslmodestr,"require") ||
				!charstring::compare(sslmodestr,"prefer")) &&
				(!charstring::isNullOrEmpty(sslca) ||
				!charstring::isNullOrEmpty(sslcapath))) {

				sslcafallback=true;
				mysql_ssl_set(mysqlptr,sslkey,sslcert,
							NULL,NULL,sslcipher);
				result=mysql_real_connect(mysqlptr,
								hostval,
								user,
								password,
								dbval,
								portval,
								socketval,
								clientflag);
			}
		#endif
		if (!result) {
			loginerror.clear();
			loginerror.append("mysql_real_connect failed: ");
			loginerror.append(mysql_error(mysqlptr));
			*error=loginerror.getString();
			logOut();
			return false;
		} else if (sslcafallback) {
			*warning="WARNING: no verification of server "
					"certificate will be done. "
					"Use sslmode=verify-ca or "
					"verify-identity.";
		}
	#else
		mysqlptr=&mysql;
		if (!mysql_real_connect(mysqlptr,hostval,user,password,
						portval,socketval,clientflag)) {
			loginerror.clear();
			loginerror.append("mysql_real_connect failed: ");
			loginerror.append(mysql_error(mysqlptr));
			*error=loginerror.getString();
			logOut();
			return false;
		}
	#endif
#else
	if (!mysql_connect(mysqlptr,hostval,user,password)) {
		loginerror.clear();
		loginerror.append("mysql_connect failed: ");
		loginerror.append(mysql_error(mysqlptr));
		*error=loginerror.getString();
		logOut();
		return false;
	}
#endif

#ifdef HAVE_MYSQL_OPT_RECONNECT
	// Enable autoreconnect in the C api
	// (ordinarily mysql_options should be called before mysql_connect,
	// but not for this option)
	mysql_options(mysqlptr,MYSQL_OPT_RECONNECT,&mytrue);
#endif

#ifdef HAVE_MYSQL_REPORT_DATA_TRUNCATION
	// The way this code works, if mysql_stmt_fetch returns any error,
	// then the fetch fails and no more rows are returned.  At some point,
	// MySQL started reporting data truncation as an error.  Disable this
	// though, we'd rather get the truncated data and keep fetching rows
	// rather than stopping all fetching at the point that the truncation
	// occurs.
	mysql_options(mysqlptr,MYSQL_REPORT_DATA_TRUNCATION,&myfalse);
#endif

#ifdef MYSQL_SELECT_DB
	if (mysql_select_db(mysqlptr,dbval)) {
		loginerror.clear();
		loginerror.append("mysql_select_db failed: ");
		loginerror.append(mysql_error(mysqlptr));
		*error=loginerror.getString();
		logOut();
		return false;
	}
#endif
	connected=true;

	// get server version to decide whether to fake binds and how to
	// escape blobs
	escapeblobs=false;
#ifdef HAVE_MYSQL_GET_SERVER_VERSION
	unsigned long	serverversion=mysql_get_server_version(mysqlptr);
	if (serverversion<40102) {
		cont->setFakeInputBinds(true);
		escapeblobs=true;
	}
#else
	char		**list;
	uint64_t	listsize;
	charstring::split(mysql_get_server_info(mysqlptr),
				".",true,&list,&listsize);

	if (listsize==3) {
		uint64_t	major=
				charstring::convertToUnsignedInteger(list[0]);
		uint64_t	minor=
				charstring::convertToUnsignedInteger(list[1]);
		uint64_t	patch=
				charstring::convertToUnsignedInteger(list[2]);
		if (major<4 || (major==4 && minor<1) ||
				(major==4 && minor==1 && patch<2)) {
			cont->setFakeInputBinds(true);
			escapeblobs=true;
		}
		for (uint64_t index=0; index<listsize; index++) {
			delete[] list[index];
		}
		delete[] list;
	}
#endif

	// get the db host name
	const char	*hostinfo=mysql_get_host_info(mysqlptr);
	const char	*space=charstring::findFirst(hostinfo,' ');
	if (space) {
		dbhostname=charstring::duplicate(hostinfo,space-hostinfo);
	} else {
		dbhostname=charstring::duplicate(hostinfo);
	}

#ifdef HAVE_MYSQL_SET_CHARACTER_SET
	// set the character set
	if (charstring::getLength(charset)) {
		mysql_set_character_set(mysqlptr,charset);
	}
#endif

	return true;
}

sqlrservercursor *mysqlconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new mysqlcursor((sqlrserverconnection *)this,id);
}

void mysqlconnection::deleteCursor(sqlrservercursor *curs) {
	delete (mysqlcursor *)curs;
}

void mysqlconnection::logOut() {
	connected=false;
	mysql_close(mysqlptr);
}

#ifdef HAVE_MYSQL_PING
bool mysqlconnection::ping() {
	return (!mysql_ping(mysqlptr))?true:false;
}
#endif

const char *mysqlconnection::getDbType() {
	return "mysql";
}

const char *mysqlconnection::getDbVersion() {
	delete[] dbversion;
	dbversion=charstring::duplicate(mysql_get_server_info(mysqlptr));
	return dbversion;
}

const char *mysqlconnection::getDbHostName() {
	return dbhostname;
}

#ifdef HAVE_MYSQL_STMT_PREPARE
const char *mysqlconnection::getBindFormat() {
	return "?";
}
#endif

const char *mysqlconnection::getNextvalFormat() {
	return "";
}

const char *mysqlconnection::getCatalogListQuery(const char *catalog) {

	cataloglistquery.clear();

	// select clause
	cataloglistquery.append(
		"select distinct "
		"	catalog_name as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	cataloglistquery.append(
		"from "
		"	information_schema.schemata ");

	// where clause
	if (catalog) {
		cataloglistquery.append(
			"where "
			"	catalog_name like '");
		cataloglistquery.append(catalog);
		cataloglistquery.append("' ");
	}

	// order by clause
	cataloglistquery.append(
		"order by "
		"	catalog_name");

	return cataloglistquery.getString();
}

const char *mysqlconnection::getSchemaListQuery(const char *catalog,
						const char *schema) {

	schemalistquery.clear();

	// select clause
	schemalistquery.append(
		"select "
		"	catalog_name as table_cat, "
		"	schema_name as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	schemalistquery.append(
		"from "
		"	information_schema.schemata ");

	// where clause
	bool	first=true;
	if (catalog) {
		schemalistquery.append(
			"where "
			"	catalog_name like '");
		schemalistquery.append(catalog);
		schemalistquery.append("' ");
		first=false;
	}
	if (schema) {
		if (first) {
			schemalistquery.append("where ");
			first=false;
		} else {
			schemalistquery.append("	and ");
		}
		schemalistquery.append(
			"	schema_name like '");
		schemalistquery.append(schema);
		schemalistquery.append("' ");
	}

	// order by clause
	schemalistquery.append(
		"order by "
		"	catalog_name, "
		"	schema_name");

	return schemalistquery.getString();
}

const char *mysqlconnection::getTableTypeListQuery(const char *catalog,
						const char *schema,
						const char *tabletypes) {
	tabletypelistquery.clear();

	// select clause
	tabletypelistquery.append(
		"select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	tabletypelistquery.append(
		"from "
		"(select 'TABLE' as table_type "
		"union "
		"select 'VIEW' as table_type "
		"union "
		"select 'ALIAS' as table_type "
		"union "
		"select 'SYNONYM' as table_type) as t ");

	// where clause
	if (!charstring::isNullOrEmpty(tabletypes)) {
		tabletypelistquery.append(
			"where "
			"	table_type like '");
		tabletypelistquery.append(tabletypes);
		tabletypelistquery.append("' ");
	}

	// order by clause
	tabletypelistquery.append(
		"order by "
		"	table_type");

	return tabletypelistquery.getString();
}

const char *mysqlconnection::getTableListQuery(const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes) {

	tablelistquery.clear();

	// select clause
	tablelistquery.append(
		"select "
		"	table_catalog as table_cat, "
		"	table_schema as table_schem, "
		"	table_name, "
		"	case "
		"		when table_type = "
		"'BASE TABLE' then 'TABLE' "
		"		else table_type "
		"	end as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	tablelistquery.append(
		"from "
		"	information_schema.tables ");

	// where clause
	tablelistquery.append(
		"where ");
	tablelistquery.append("	(");
	bool	first=true;
	if (objecttypes&DB_OBJECT_TABLE) {
		tablelistquery.append("	table_type='BASE TABLE' ");
		first=false;
	}
	if (objecttypes&DB_OBJECT_VIEW) {
		if (!first) {
			tablelistquery.append("	or ");
		}
		tablelistquery.append("	table_type='VIEW' ");
		first=false;
	}
	if (objecttypes&DB_OBJECT_ALIAS) {
		if (!first) {
			tablelistquery.append("	or ");
		}
		tablelistquery.append("	table_type='ALIAS' ");
		first=false;
	}
	if (objecttypes&DB_OBJECT_SYNONYM) {
		if (!first) {
			tablelistquery.append("	or ");
		}
		tablelistquery.append("	table_type='SYNONYM' ");
	}
	tablelistquery.append(") ");
	if (catalog) {
		tablelistquery.append(
			"	and "
			"	table_catalog like '");
		tablelistquery.append(catalog);
		tablelistquery.append("' ");
	}
	if (schema) {
		tablelistquery.append(
			"	and "
			"	table_schema like '");
		tablelistquery.append(schema);
		tablelistquery.append("' ");
	}
	if (table) {
		tablelistquery.append(
			"	and "
			"	table_name like '");
		tablelistquery.append(table);
		tablelistquery.append("' ");
	}

	// order by clause
	tablelistquery.append(
		"order by "
		"	table_cat, "
		"	table_schem, "
		"	table_name");

	return tablelistquery.getString();
}

static const char	*bittype=
			"select "
			"	'BIT' as type_name, "
			"	-7 as data_type, "
			"	1 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BIT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*tinyinttype=
			"select "
			"	'TINYINT' as type_name, "
			"	-6 as data_type, "
			"	3 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'TINYINT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*biginttype=
			"select "
			"	'BIGINT' as type_name, "
			"	-5 as data_type, "
			"	19 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BIGINT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*longblobtype=
			"select "
			"	'LONGBLOB' as type_name, "
			"	-4 as data_type, "
			"	2147483647 as column_size, "
			"	'0x' as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	0 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'LONGBLOB' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*varbinarytype=
			"select "
			"	'VARBINARY' as type_name, "
			"	-3 as data_type, "
			"	65535 as column_size, "
			"	'0x' as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'VARBINARY' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*binarytype=
			"select "
			"	'BINARY' as type_name, "
			"	-2 as data_type, "
			"	255 as column_size, "
			"	'0x' as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BINARY' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*longtexttype=
			"select "
			"	'LONGTEXT' as type_name, "
			"	-1 as data_type, "
			"	2147483647 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'LONGTEXT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*chartype=
			"select "
			"	'CHAR' as type_name, "
			"	1 as data_type, "
			"	255 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'CHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*decimaltype=
			"select "
			"	'DECIMAL' as type_name, "
			"	2 as data_type, "
			"	65 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'DECIMAL' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*inttype=
			"select "
			"	'INT' as type_name, "
			"	4 as data_type, "
			"	10 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'INT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*smallinttype=
			"select "
			"	'SMALLINT' as type_name, "
			"	5 as data_type, "
			"	5 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'SMALLINT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*floattype=
			"select "
			"	'FLOAT' as type_name, "
			"	6 as data_type, "
			"	12 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'FLOAT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*realtype=
			"select "
			"	'REAL' as type_name, "
			"	7 as data_type, "
			"	7 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'REAL' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*doubletype=
			"select "
			"	'DOUBLE' as type_name, "
			"	8 as data_type, "
			"	15 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'DOUBLE' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*varchartype=
			"select "
			"	'VARCHAR' as type_name, "
			"	12 as data_type, "
			"	65535 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'VARCHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*datetype=
			"select "
			"	'DATE' as type_name, "
			"	91 as data_type, "
			"	10 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'DATE' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*timetype=
			"select "
			"	'TIME' as type_name, "
			"	92 as data_type, "
			"	8 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'TIME' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*datetimetype=
			"select "
			"	'DATETIME' as type_name, "
			"	93 as data_type, "
			"	19 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'DATETIME' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*timestamptype=
			"select "
			"	'TIMESTAMP' as type_name, "
			"	93 as data_type, "
			"	19 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'TIMESTAMP' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*blobtype=
			"select "
			"	'BLOB' as type_name, "
			"	-4 as data_type, "
			"	65535 as column_size, "
			"	'0x' as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	0 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BLOB' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*texttype=
			"select "
			"	'TEXT' as type_name, "
			"	-1 as data_type, "
			"	65535 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'TEXT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*mediuminttype=
			"select "
			"	'MEDIUMINT' as type_name, "
			"	4 as data_type, "
			"	8 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'MEDIUMINT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*yeartype=
			"select "
			"	'YEAR' as type_name, "
			"	5 as data_type, "
			"	4 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'YEAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*jsontype=
			"select "
			"	'JSON' as type_name, "
			"	-1 as data_type, "
			"	2147483647 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'JSON' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*enumtype=
			"select "
			"	'ENUM' as type_name, "
			"	12 as data_type, "
			"	65535 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'ENUM' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*settype=
			"select "
			"	'SET' as type_name, "
			"	12 as data_type, "
			"	64 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'SET' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

const char *mysqlconnection::getTypeInfoListQuery(const char *catalog,
							const char *schema,
							const char *type) {

	if (!charstring::compare(type,"*")) {
		if (!typeinfolistquery.getSize()) {
			typeinfolistquery.append("(");
			typeinfolistquery.append(bittype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(tinyinttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(biginttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(longblobtype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(varbinarytype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(binarytype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(longtexttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(chartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(decimaltype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(inttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(smallinttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(floattype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(realtype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(doubletype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(datetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(timetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(datetimetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(timestamptype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(blobtype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(texttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(mediuminttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(yeartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(jsontype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(enumtype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(settype);
			typeinfolistquery.append(")");
		}
		return typeinfolistquery.getString();
	} else if (!charstring::compareIgnoringCase(type,"bit")) {
		return bittype;
	} else if (!charstring::compareIgnoringCase(type,"tinyint")) {
		return tinyinttype;
	} else if (!charstring::compareIgnoringCase(type,"bigint")) {
		return biginttype;
	} else if (!charstring::compareIgnoringCase(type,"longblob")) {
		return longblobtype;
	} else if (!charstring::compareIgnoringCase(type,"varbinary")) {
		return varbinarytype;
	} else if (!charstring::compareIgnoringCase(type,"binary")) {
		return binarytype;
	} else if (!charstring::compareIgnoringCase(type,"longtext")) {
		return longtexttype;
	} else if (!charstring::compareIgnoringCase(type,"char")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"decimal")) {
		return decimaltype;
	} else if (!charstring::compareIgnoringCase(type,"numeric")) {
		return decimaltype;
	} else if (!charstring::compareIgnoringCase(type,"int")) {
		return inttype;
	} else if (!charstring::compareIgnoringCase(type,"integer")) {
		return inttype;
	} else if (!charstring::compareIgnoringCase(type,"smallint")) {
		return smallinttype;
	} else if (!charstring::compareIgnoringCase(type,"float")) {
		return floattype;
	} else if (!charstring::compareIgnoringCase(type,"real")) {
		return realtype;
	} else if (!charstring::compareIgnoringCase(type,"double")) {
		return doubletype;
	} else if (!charstring::compareIgnoringCase(type,"double precision")) {
		return doubletype;
	} else if (!charstring::compareIgnoringCase(type,"varchar")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"date")) {
		return datetype;
	} else if (!charstring::compareIgnoringCase(type,"time")) {
		return timetype;
	} else if (!charstring::compareIgnoringCase(type,"datetime")) {
		return datetimetype;
	} else if (!charstring::compareIgnoringCase(type,"timestamp")) {
		return timestamptype;
	} else if (!charstring::compareIgnoringCase(type,"blob")) {
		return blobtype;
	} else if (!charstring::compareIgnoringCase(type,"text")) {
		return texttype;
	} else if (!charstring::compareIgnoringCase(type,"mediumint")) {
		return mediuminttype;
	} else if (!charstring::compareIgnoringCase(type,"year")) {
		return yeartype;
	} else if (!charstring::compareIgnoringCase(type,"json")) {
		return jsontype;
	} else if (!charstring::compareIgnoringCase(type,"enum")) {
		return enumtype;
	} else if (!charstring::compareIgnoringCase(type,"set")) {
		return settype;
	}
	return NULL;
}

const char *mysqlconnection::getColumnListQuery(const char *catalog,
						const char *schema,
						const char *table,
						const char *column) {

	columnlistquery.clear();

	// select clause
	columnlistquery.append(
		"select "
		"	table_catalog as table_cat, "
		"	table_schema as table_schem, "
		"	table_name as table_name, "
		"	column_name, "
		"	'' as data_type, " // case this...
		"	upper(data_type) as type_name, "
		"	character_maximum_length as column_size, "
		"	null as buffer_length, "
			// length in bytes of data transferred during fetch
		"	numeric_scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	case "
		"		when is_nullable='NO' then 0 "
		"		when is_nullable='YES' then 1 "
		"		else 2 "
		"	end as nullable, "
		"	extra as remarks, "
		"	column_default, "
		"	null as sql_data_type, "
			// type (int)
		"	null as sql_datetime_sub, "
			// subtype (int) for datetime/interval, otherwise null
		"	character_octet_length as char_octet_length, "
		"	ordinal_position, "
		"	is_nullable, "
		"	numeric_precision, "
		"	column_key, "
		"	null ");

	// from clause
	columnlistquery.append(
		"from "
		"	information_schema.columns ");

	// where clause
	bool	first=true;
	if (!charstring::isNullOrEmpty(catalog)) {
		columnlistquery.append(
			"where "
			"	table_catalog like '");
		columnlistquery.append(catalog);
		columnlistquery.append("' ");
		first=false;
	}

	if (!charstring::isNullOrEmpty(schema)) {
		if (first) {
			columnlistquery.append("where ");
			first=false;
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	table_schema like '");
		columnlistquery.append(schema);
		columnlistquery.append("' ");
	}

	if (!charstring::isNullOrEmpty(table)) {
		if (first) {
			columnlistquery.append("where ");
			first=false;
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	table_name like '");
		columnlistquery.append(table);
		columnlistquery.append("' ");
	}

	if (!charstring::isNullOrEmpty(column)) {
		if (first) {
			columnlistquery.append("where ");
			first=false;
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	column_name like '");
		columnlistquery.append(column);
		columnlistquery.append("' ");
	}

	// order by clause
	columnlistquery.append(
		"order by "
		"	ordinal_position");

	return columnlistquery.getString();
}

const char *mysqlconnection::getPrimaryKeysListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	primarykeyslistquery.clear();

	// select clause
	primarykeyslistquery.append(
		"select "
		"	'def' as table_cat, "
		"	tc.table_schema as table_schem, "
		"	tc.table_name, "
		"	ku.column_name, "
		"	ku.ordinal_position as key_seq, "
		"	tc.constraint_name as pk_name, "
		"	null ");

	// from clause
	primarykeyslistquery.append(
		"from "
		"	information_schema.table_constraints tc, "
		"	information_schema.key_column_usage ku ");

	// where clause
	primarykeyslistquery.append(
		"where "
		"	tc.constraint_type='PRIMARY KEY' "
		"	and "
		"	tc.constraint_name=ku.constraint_name "
		"	and "
		"	tc.table_schema=ku.table_schema "
		"	and "
		"	tc.table_name=ku.table_name ");
	if (!charstring::isNullOrEmpty(catalog)) {
		primarykeyslistquery.append(
			"	and "
			"	'def' like '");
		primarykeyslistquery.append(catalog);
		primarykeyslistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(schema)) {
		primarykeyslistquery.append(
			"	and "
			"	tc.table_schema like '");
		primarykeyslistquery.append(schema);
		primarykeyslistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		primarykeyslistquery.append(
			"	and "
			"	tc.table_name like '");
		primarykeyslistquery.append(table);
		primarykeyslistquery.append("' ");
	}

	// order by clause
	primarykeyslistquery.append(
		"order by "
		"	tc.table_name, "
		"	ku.ordinal_position");

	return primarykeyslistquery.getString();
}

const char *mysqlconnection::getKeyAndIndexListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	s.table_catalog as table_cat, "
		"	s.table_schema as table_schem, "
		"	s.table_name, "
		"	case s.non_unique "
		"		when 1 then 'true' "
		"		else 'false' "
		"	end as non_unique, "
		"	s.table_schema as index_qualifier, "
		"	s.index_name, "
		"	3 as type, "
		"	s.seq_in_index as ordinal_position, "
		"	s.column_name, "
		"	s.collation as asc_or_desc, "
		"	s.cardinality, "
		"	null as pages, "
		"	null as filter_condition, "
		"	null ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	information_schema.statistics s ");

	// where clause
	bool	first=true;
	if (!charstring::isNullOrEmpty(catalog)) {
		keyandindexlistquery.append(
			"where "
			"	s.table_catalog like '");
		keyandindexlistquery.append(catalog);
		keyandindexlistquery.append("' ");
		first=false;
	}
	if (!charstring::isNullOrEmpty(schema)) {
		if (first) {
			keyandindexlistquery.append("where ");
			first=false;
		} else {
			keyandindexlistquery.append("	and ");
		}
		keyandindexlistquery.append(
			"	s.table_schema like '");
		keyandindexlistquery.append(schema);
		keyandindexlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		if (first) {
			keyandindexlistquery.append("where ");
		} else {
			keyandindexlistquery.append("	and ");
		}
		keyandindexlistquery.append(
			"	s.table_name like '");
		keyandindexlistquery.append(table);
		keyandindexlistquery.append("' ");
	}

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	s.table_name, "
		"	s.index_name, "
		"	s.seq_in_index");

	return keyandindexlistquery.getString();
}

const char *mysqlconnection::getProcedureListQuery(
					const char *catalog,
					const char *schema,
					const char *procedure) {

	procedurelistquery.clear();

	// select clause
	procedurelistquery.append(
		"select "
		"	routine_catalog as procedure_cat, "
		"	routine_schema as procedure_schem, "
		"	routine_name as procedure_name, "
		"	0 as num_input_params, "
		"	0 as num_output_params, "
		"	0 as num_result_sets, "
		"	'' as remarks, "
		"	case routine_type "
		"		when 'PROCEDURE' then '1' "
		"		when 'FUNCTION' then '2' "
		"		else '0' "
		"	end as procedure_type, "
		"	null ");

	// from clause
	procedurelistquery.append(
		"from "
		"	information_schema.routines ");

	// where clause
	if (!charstring::isNullOrEmpty(catalog) ||
		!charstring::isNullOrEmpty(schema) ||
		!charstring::isNullOrEmpty(procedure)) {

		bool	first=true;
		procedurelistquery.append("where ");
		if (!charstring::isNullOrEmpty(catalog)) {
			procedurelistquery.append(
				"routine_catalog like '");
			procedurelistquery.append(catalog);
			procedurelistquery.append("' ");
			first=false;
		}
		if (!charstring::isNullOrEmpty(schema)) {
			if (!first) {
				procedurelistquery.append("and ");
			}
			procedurelistquery.append(
				"routine_schema like '");
			procedurelistquery.append(schema);
			procedurelistquery.append("' ");
			first=false;
		}
		if (!charstring::isNullOrEmpty(procedure)) {
			if (!first) {
				procedurelistquery.append("and ");
			}
			procedurelistquery.append(
				"routine_name like '");
			procedurelistquery.append(procedure);
			procedurelistquery.append("' ");
		}
	}

	// order by clause
	procedurelistquery.append(
		"order by "
		"	routine_catalog, "
		"	routine_schema, "
		"	routine_name");

	return procedurelistquery.getString();
}

const char *mysqlconnection::getProcedureParameterListQuery(
					const char *catalog,
					const char *schema,
					const char *procedure) {

	procedureparameterlistquery.clear();

	// select clause
	procedureparameterlistquery.append(
		"select "
		"	p.specific_catalog as procedure_cat, "
		"	p.specific_schema as procedure_schem, "
		"	p.specific_name as procedure_name, "
		"	p.parameter_name as column_name, "
		"	case p.parameter_mode "
		"		when 'IN' then 1 "
		"		when 'INOUT' then 2 "
		"		when 'OUT' then 4 "
		"		else 5 "
		"	end as column_type, "
		"	'' as data_type, "
		"	upper(p.data_type) as type_name, "
		"	p.character_maximum_length as column_size, "
		"	null as buffer_length, "
		"	p.numeric_scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	1 as nullable, "
		"	'' as remarks, "
		"	null as column_def, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	p.character_octet_length as char_octet_length, "
		"	p.ordinal_position, "
		"	'YES' as is_nullable, "
		"	null ");

	// from clause
	procedureparameterlistquery.append(
		"from "
		"	information_schema.parameters p ");

	// where clause
	if (!charstring::isNullOrEmpty(catalog) ||
		!charstring::isNullOrEmpty(schema) ||
		!charstring::isNullOrEmpty(procedure)) {

		bool	first=true;
		procedureparameterlistquery.append("where ");
		if (!charstring::isNullOrEmpty(catalog)) {
			procedureparameterlistquery.append(
				"p.specific_catalog like '");
			procedureparameterlistquery.append(catalog);
			procedureparameterlistquery.append("' ");
			first=false;
		}
		if (!charstring::isNullOrEmpty(schema)) {
			if (!first) {
				procedureparameterlistquery.append("and ");
			}
			procedureparameterlistquery.append(
				"p.specific_schema like '");
			procedureparameterlistquery.append(schema);
			procedureparameterlistquery.append("' ");
			first=false;
		}
		if (!charstring::isNullOrEmpty(procedure)) {
			if (!first) {
				procedureparameterlistquery.append("and ");
			}
			procedureparameterlistquery.append(
				"p.specific_name like '");
			procedureparameterlistquery.append(procedure);
			procedureparameterlistquery.append("' ");
		}
	}

	// order by clause
	procedureparameterlistquery.append(
		"order by "
		"	p.specific_name, "
		"	p.ordinal_position");

	return procedureparameterlistquery.getString();
}

bool mysqlconnection::getDatabaseIsSchema() {
	return true;
}

char *mysqlconnection::getCurrentCatalog() {
	return charstring::duplicate("def");
}

const char *mysqlconnection::selectSchemaQuery() {
	return "use `%s`";
}

const char *mysqlconnection::getCurrentSchemaQuery() {
	return "select database()";
}

const char *mysqlconnection::getDefaultIsolationLevel() {
	return "REPEATABLE_READ";
}

const char *mysqlconnection::setIsolationLevelQuery() {
	return "set @@session.tx_isolation='%s'";
}

const char *mysqlconnection::getIsolationLevelQuery() {
	return "select @@session.tx_isolation";
}

const char *mysqlconnection::mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat) {
	if (fromformat==toformat) {
		return isolevel;
	}
	if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"TRANSACTION_READ_UNCOMMITTED")) {
			return "READ-UNCOMMITTED";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_READ_COMMITTED")) {
			return "READ-COMMITTED";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_REPEATABLE_READ")) {
			return "REPEATABLE-READ";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
			return "SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(
					isolevel,"READ-UNCOMMITTED")) {
			return "TRANSACTION_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"READ-COMMITTED")) {
			return "TRANSACTION_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"REPEATABLE-READ")) {
			return "TRANSACTION_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"SERIALIZABLE")) {
			return "TRANSACTION_SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_UNCOMMITTED")) {
			return "READ-UNCOMMITTED";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_COMMITTED")) {
			return "READ-COMMITTED";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_REPEATABLE_READ")) {
			return "REPEATABLE-READ";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_SERIALIZABLE")) {
			return "SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(
					isolevel,"READ-UNCOMMITTED")) {
			return "SQL_TXN_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"READ-COMMITTED")) {
			return "SQL_TXN_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"REPEATABLE-READ")) {
			return "SQL_TXN_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"SERIALIZABLE")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *mysqlconnection::getDatabaseFeatures() {

	if (databasefeatures) {
		return databasefeatures;
	}

	databasefeatures=new char *[FEATURE_COUNT];
	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		charstring::duplicate("false");

	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		charstring::duplicate("false");

	databasefeatures[FEATURE_ALTER_DOMAIN_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ALTER_TABLE_OPERATIONS]=
		charstring::duplicate("ADD_COLUMN,DROP_COLUMN");

	databasefeatures[FEATURE_ANSI92_SQL_LEVELS]=
		charstring::duplicate("ENTRY_LEVEL");

	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
		charstring::duplicate("false");

	databasefeatures[FEATURE_BATCH_OPERATIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_BATCH_ROW_COUNTS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CATALOG_SEPARATOR]=
		charstring::duplicate(".");

	databasefeatures[FEATURE_CATALOG_TERM]=
		charstring::duplicate("database");

	databasefeatures[FEATURE_CATALOG_USAGE]=
		charstring::duplicate(
			"DATA_MANIPULATION,INDEX_DEFINITIONS,"
			"PRIVILEGE_DEFINITIONS,PROCEDURE_CALLS,"
			"TABLE_DEFINITIONS");

	databasefeatures[FEATURE_COLLATION_SEQ]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_ASSERTION_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_CHARACTER_SET_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_COLLATION_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_DOMAIN_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_SCHEMA_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_TABLE_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_TRANSLATION_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_CREATE_VIEW_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DATA_DEFINITION_TRANSACTION_BEHAVIOR]=
		charstring::duplicate("CAUSES_COMMIT");

	databasefeatures[FEATURE_DDL_INDEX_OPERATIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DEFAULT_RESULT_SET_HOLDABILITY]=
		charstring::duplicate("HOLD_CURSORS_OVER_COMMIT");

	databasefeatures[FEATURE_DELETES_ARE_DETECTED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DOES_MAX_ROW_SIZE_INCLUDE_BLOBS]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_DROP_ASSERTION_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DROP_CHARACTER_SET_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DROP_COLLATION_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DROP_DOMAIN_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DROP_SCHEMA_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DROP_TABLE_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DROP_TRANSLATION_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DROP_VIEW_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_EXTRA_NAME_CHARACTERS]=
		charstring::duplicate("$");

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		charstring::duplicate("BASIC,BEYOND_SELECT,UNRELATED");

	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		charstring::duplicate("MIXED");

	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
		charstring::duplicate("`");

	databasefeatures[FEATURE_INDEX_KEYWORDS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_INFO_SCHEMA_VIEWS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_INSERTS_ARE_DETECTED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_INSERT_OPERATIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ISOLATION_LEVELS]=
		charstring::duplicate(
			"READ_UNCOMMITTED,READ_COMMITTED,"
			"REPEATABLE_READ,SERIALIZABLE");

	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_LOCK_TYPES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		charstring::duplicate("16777208");

	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		charstring::duplicate("32");

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		charstring::duplicate("16777208");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		charstring::duplicate("64");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		charstring::duplicate("16");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		charstring::duplicate("64");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		charstring::duplicate("256");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		charstring::duplicate("512");

	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		charstring::duplicate("64");

	databasefeatures[FEATURE_MAX_CONNECTIONS]=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		charstring::duplicate("64");

	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		charstring::duplicate("256");

	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		charstring::duplicate("0");

	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		charstring::duplicate("2147483639");

	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		charstring::duplicate("0");

	databasefeatures[FEATURE_MAX_STATEMENTS]=
		charstring::duplicate("0");

	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		charstring::duplicate("65531");

	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		charstring::duplicate("256");

	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		charstring::duplicate("64");

	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		charstring::duplicate("16");

	databasefeatures[FEATURE_MIXED_CASE_IDENTIFIERS]=
		charstring::duplicate("IDENTIFIERS,QUOTED_IDENTIFIERS");

	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		charstring::duplicate("LOW");

	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		charstring::duplicate(
			"ABS,ACOS,ASIN,ATAN,ATAN2,BIT_COUNT,CEILING,"
			"COS,COT,DEGREES,EXP,FLOOR,LOG,LOG10,MAX,MIN,"
			"MOD,PI,POW,POWER,RADIANS,RAND,ROUND,SIN,SQRT,"
			"TAN,TRUNCATE");

	databasefeatures[FEATURE_OPEN_CURSORS_ACROSS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OPEN_STATEMENTS_ACROSS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OTHERS_DELETES_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OTHERS_INSERTS_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OTHERS_UPDATES_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OUTER_JOINS]=
		charstring::duplicate("BASIC,LIMITED");

	databasefeatures[FEATURE_OWN_DELETES_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_PREDICATES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		charstring::duplicate("PROCEDURE");

	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		charstring::duplicate("MIXED");

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		charstring::duplicate(
			"FORWARD_ONLY/READ_ONLY,"
			"FORWARD_ONLY/UPDATABLE,"
			"SCROLL_INSENSITIVE/READ_ONLY,"
			"SCROLL_INSENSITIVE/UPDATABLE");

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		charstring::duplicate("HOLD_CURSORS_OVER_COMMIT");

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		charstring::duplicate("FORWARD_ONLY,SCROLL_INSENSITIVE");

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		charstring::duplicate("ROWID_UNSUPPORTED");

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SCHEMA_TERM]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		charstring::duplicate("\\");

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		charstring::duplicate("CORE,MINIMUM");

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		charstring::duplicate(
			"ACCESSIBLE,ADD,ANALYZE,ASC,BEFORE,CASCADE,"
			"CHANGE,CONTINUE,DATABASE,DATABASES,DAY_HOUR,"
			"DAY_MICROSECOND,DAY_MINUTE,DAY_SECOND,DELAYED,"
			"DESC,DISTINCTROW,DIV,DUAL,ELSEIF,EMPTY,"
			"ENCLOSED,ESCAPED,EXIT,EXPLAIN,FIRST_VALUE,"
			"FLOAT4,FLOAT8,FORCE,FULLTEXT,GENERATED,GROUPS,"
			"HIGH_PRIORITY,HOUR_MICROSECOND,HOUR_MINUTE,"
			"HOUR_SECOND,IF,IGNORE,INDEX,INFILE,INT1,INT2,"
			"INT3,INT4,INT8,IO_AFTER_GTIDS,IO_BEFORE_GTIDS,"
			"ITERATE,JSON_TABLE,KEY,KEYS,KILL,LAG,"
			"LAST_VALUE,LEAD,LEAVE,LIMIT,LINEAR,LINES,LOAD,"
			"LOCK,LONG,LONGBLOB,LONGTEXT,LOOP,LOW_PRIORITY,"
			"MANUAL,MASTER_BIND,"
			"MASTER_SSL_VERIFY_SERVER_CERT,MAXVALUE,"
			"MEDIUMBLOB,MEDIUMINT,MEDIUMTEXT,MIDDLEINT,"
			"MINUTE_MICROSECOND,MINUTE_SECOND,"
			"NO_WRITE_TO_BINLOG,NTH_VALUE,NTILE,OPTIMIZE,"
			"OPTIMIZER_COSTS,OPTION,OPTIONALLY,OUTFILE,"
			"PARALLEL,PURGE,QUALIFY,READ,READ_WRITE,REGEXP,"
			"RENAME,REPEAT,REPLACE,REQUIRE,RESIGNAL,"
			"RESTRICT,RLIKE,SCHEMA,SCHEMAS,"
			"SECOND_MICROSECOND,SEPARATOR,SHOW,SIGNAL,"
			"SPATIAL,SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,"
			"SQL_SMALL_RESULT,SSL,STARTING,STORED,"
			"STRAIGHT_JOIN,TERMINATED,TINYBLOB,TINYINT,"
			"TINYTEXT,UNDO,UNLOCK,UNSIGNED,USAGE,USE,"
			"UTC_DATE,UTC_TIME,UTC_TIMESTAMP,VARBINARY,"
			"VARCHARACTER,VIRTUAL,WHILE,WRITE,XOR,"
			"YEAR_MONTH,ZEROFILL");

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		charstring::duplicate("2");

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		charstring::duplicate("FUNCTIONS,PROCEDURES");

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		charstring::duplicate(
			"ASCII,BIN,BIT_LENGTH,CHAR,CHARACTER_LENGTH,"
			"CHAR_LENGTH,CONCAT,CONCAT_WS,CONV,ELT,"
			"EXPORT_SET,FIELD,FIND_IN_SET,HEX,INSERT,"
			"INSTR,LCASE,LEFT,LENGTH,LOAD_FILE,LOCATE,"
			"LOCATE,LOWER,LPAD,LTRIM,MAKE_SET,MATCH,MID,"
			"OCT,OCTET_LENGTH,ORD,POSITION,QUOTE,REPEAT,"
			"REPLACE,REVERSE,RIGHT,RPAD,RTRIM,SOUNDEX,"
			"SPACE,STRCMP,SUBSTRING,SUBSTRING,SUBSTRING,"
			"SUBSTRING,SUBSTRING_INDEX,TRIM,UCASE,UPPER");

	databasefeatures[FEATURE_SUBQUERY_USAGE]=
		charstring::duplicate("COMPARISONS,EXISTS,INS,QUANTIFIEDS");

	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		charstring::duplicate("false");

	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_DESCRIBE_PARAMETER]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		charstring::duplicate("false");

	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_TRANSACTIONS]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_NAMED_PARAMETERS]=
		charstring::duplicate("false");

	databasefeatures[FEATURE_SUPPORTS_NON_NULLABLE_COLUMNS]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_ORDER_BY_UNRELATED]=
		charstring::duplicate("false");

	databasefeatures[FEATURE_SUPPORTS_SAVEPOINTS]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_SELECT_FOR_UPDATE]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		charstring::duplicate("true");

	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		charstring::duplicate(
			"DATABASE,USER,SYSTEM_USER,SESSION_USER,"
			"PASSWORD,ENCRYPT,LAST_INSERT_ID,VERSION");

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		charstring::duplicate("BASIC,DIFFERENT");

	databasefeatures[FEATURE_TABLE_TERM]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		charstring::duplicate(
			"DAYOFWEEK,WEEKDAY,DAYOFMONTH,DAYOFYEAR,MONTH,"
			"DAYNAME,MONTHNAME,QUARTER,WEEK,YEAR,HOUR,"
			"MINUTE,SECOND,PERIOD_ADD,PERIOD_DIFF,TO_DAYS,"
			"FROM_DAYS,DATE_FORMAT,TIME_FORMAT,CURDATE,"
			"CURRENT_DATE,CURTIME,CURRENT_TIME,NOW,SYSDATE,"
			"CURRENT_TIMESTAMP,UNIX_TIMESTAMP,FROM_UNIXTIME,"
			"SEC_TO_TIME,TIME_TO_SEC");

	databasefeatures[FEATURE_TIME_DATE_LITERALS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TRANSACTION_DDL_DML]=
		charstring::duplicate("");

	databasefeatures[FEATURE_UNION_CLAUSES]=
		charstring::duplicate("UNION,UNION_ALL");

	databasefeatures[FEATURE_UPDATES_ARE_DETECTED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_VALUE_EXPRESSIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_WHERE_CURRENT_OF_OPERATIONS]=
		charstring::duplicate("");

	return databasefeatures;
}

bool mysqlconnection::getLastInsertId(uint64_t *id) {
	// Some versions of mariadb client (10.1) incorrectly reset the last
	// insert id after each query instead of hanging on to it until another
	// insert overrides it, or until end-of-session.
	//
	// To enforce the correct behavior we'll:
	// * keep our own last insert id
	// * call mysql_insert_id() after each execute and update our last
	//   insert id if it returns non-zero
	// * reset our last insert id to 0 at end-of-session
	*id=lastinsertid;
	return true;
}

const char *mysqlconnection::getNoopQuery() {
	return "set @noop=null";
}

bool mysqlconnection::isTransactional() {
	return true;
}

bool mysqlconnection::setAutoCommitOn() {
#ifdef HAVE_MYSQL_AUTOCOMMIT
	return !mysql_autocommit(mysqlptr,true);
#elif defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID>=40000

	// re-init error data
	cont->clearError();

	// init some variables
	const char	*query="set autocommit=1";
	int		querysize=16;

	// run the query...
	sqlrservercursor	*cur=cont->newCursor();
	bool	retval=(cur->open() &&
			cur->prepareQuery(query,querysize) &&
			cur->executeQuery(query,querysize));

	// If there was an error, copy it out.  We'll be destroying the
	// cursor in a moment and the error will be lost otherwise.
	if (!retval) {
		cont->saveErrorFromCursor(cur);
	}

	// clean up
	cur->closeResultSet();
	cur->close();
	cont->deleteCursor(cur);

	return retval;
#else
	// do nothing
	return true;
#endif
}

bool mysqlconnection::setAutoCommitOff() {
#ifdef HAVE_MYSQL_AUTOCOMMIT
	return !mysql_autocommit(mysqlptr,false);
#elif defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID>=40000

	// re-init error data
	cont->clearError();

	// init some variables
	const char	*query="set autocommit=0";
	int		querysize=16;

	// run the query...
	sqlrservercursor	*cur=cont->newCursor();
	bool	retval=(cur->open() &&
			cur->prepareQuery(query,querysize) &&
			cur->executeQuery(query,querysize));

	// If there was an error, copy it out.  We'll be destroying the
	// cursor in a moment and the error will be lost otherwise.
	if (!retval) {
		cont->saveErrorFromCursor(cur);
	}

	// clean up
	cur->closeResultSet();
	cur->close();
	cont->deleteCursor(cur);

	return retval;
#else
	// do nothing
	return true;
#endif
}

bool mysqlconnection::commit() {
#ifdef HAVE_MYSQL_COMMIT
	return !mysql_commit(mysqlptr);
#elif defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID>=40000
	// 4.x supports transactions but doesn't have a mysql_commit function
	return sqlrserverconnection::commit();
#else
	// do nothing
	return true;
#endif
}

bool mysqlconnection::rollback() {
#ifdef HAVE_MYSQL_ROLLBACK
	return !mysql_rollback(mysqlptr);
#elif defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID>=40000
	// 4.x supports transactions but doesn't have a mysql_rollback function
	return sqlrserverconnection::rollback();
#else
	// do nothing
	return true;
#endif
}

void mysqlconnection::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {
	const char	*errorstring=mysql_error(mysqlptr);
	*errorsize=charstring::getLength(errorstring);
	charstring::safeCopy(errorbuffer,errorbuffersize,
					errorstring,*errorsize);
	*errorcode=mysql_errno(mysqlptr);
	*liveconnection=(!charstring::compare(errorstring,"") ||
		!charstring::compareIgnoringCase(errorstring,
				"mysql server has gone away",26) ||
		!charstring::compareIgnoringCase(errorstring,
				"Can't connect to local MySQL",28) ||
		!charstring::compareIgnoringCase(errorstring,
				"Can't connect to MySQL",22) ||
		!charstring::compareIgnoringCase(errorstring,
			"Lost connection to MySQL server during query",44));
}

#ifdef HAVE_MYSQL_STMT_PREPARE
int16_t mysqlconnection::getNonNullBindValue() {
	return 0;
}

int16_t mysqlconnection::getNullBindValue() {
	return 1;
}
#endif

void mysqlconnection::endSession() {
	firstquery=true;
	lastinsertid=0;
}

mysqlcursor::mysqlcursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {
	mysqlconn=(mysqlconnection *)conn;
	mysqlresult=NULL;
	ncols=0;
	nrows=0;
	affectedrows=0;

#ifdef HAVE_MYSQL_STMT_PREPARE
	stmt=NULL;
	stmtreset=false;
	stmtfreeresult=false;

	mysqlmetadata=NULL;

	boundvariables=false;

	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	bind=new MYSQL_BIND[maxbindcount];
	bindvaluesize=new unsigned long[maxbindcount];
	bytestring::zero(bind,maxbindcount*sizeof(MYSQL_BIND));
	bindtime=new MYSQL_TIME[maxbindcount];

	usestmtprepare=true;
	stmtpreparefailed=false;
	bindformaterror=false;
	unsupportedbystmt.setPattern(
			"^[ 	\r\n]*"
			"(/\\*.*\\*/[ 	\r\n]+)*"
			"(("
				"explain|EXPLAIN|"
				"create|CREATE|"
				"drop|DROP|"
				"procedure|PROCEDURE|"
				"function|FUNCTION|"
				"use|USE|"
				"call|CALL|"
				"start|START|"
				"check|CHECK|"
				"repair|REPAIR|"
				"savepoint|SAVEPOINT|"
				"release|RELEASE|"
				"connect|CONNECT|"
				"lock|LOCK|"
				"unlock|UNLOCK|"
				"show|SHOW"
			")[ 	\r\n]+)|"
			"(("
				"begin|BEGIN|"
				"rollback|ROLLBACK"
			")[ 	\r\n]*)");
	unsupportedbystmt.study();
#endif

	allocateResultSetBuffers(conn->cont->getMaxColumnCount());
}

mysqlcursor::~mysqlcursor() {
#ifdef HAVE_MYSQL_STMT_PREPARE
	delete[] bind;
	delete[] bindvaluesize;
	delete[] bindtime;
#endif
	deallocateResultSetBuffers();
}

void mysqlcursor::allocateResultSetBuffers(int32_t columncount) {

	if (!columncount) {
		mysqlfields=NULL;
#ifdef HAVE_MYSQL_STMT_PREPARE
		fieldbind=NULL;
		field=NULL;
		isnull=NULL;
		fieldsize=NULL;
#endif
	} else {
		mysqlfields=new MYSQL_FIELD *[columncount];
#ifdef HAVE_MYSQL_STMT_PREPARE
		uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();
		fieldbind=new MYSQL_BIND[columncount];
		field=new char[columncount*maxfieldsize];
		isnull=new my_bool[columncount];
		fieldsize=new unsigned long[columncount];
		bytestring::zero(fieldbind,columncount*sizeof(MYSQL_BIND));
		for (unsigned short index=0; index<columncount; index++) {
			fieldbind[index].buffer_type=MYSQL_TYPE_STRING;
			fieldbind[index].buffer=&field[index*maxfieldsize];
			fieldbind[index].buffer_length=maxfieldsize;
			fieldbind[index].is_null=&isnull[index];
			fieldbind[index].length=&fieldsize[index];
		}
#endif
	}

#ifdef HAVE_MYSQL_STMT_PREPARE
	bytestring::zero(&lobfield,sizeof(MYSQL_BIND));
	lobfield.buffer_type=MYSQL_TYPE_STRING;
#endif
}

void mysqlcursor::deallocateResultSetBuffers() {
#ifdef HAVE_MYSQL_STMT_PREPARE
	delete[] fieldbind;
	delete[] field;
	delete[] isnull;
	delete[] fieldsize;
	fieldbind=NULL;
	field=NULL;
	isnull=NULL;
	fieldsize=NULL;
#endif
	delete[] mysqlfields;
	mysqlfields=NULL;
}

#ifdef HAVE_MYSQL_STMT_PREPARE
bool mysqlcursor::open() {
	stmt=mysql_stmt_init(mysqlconn->mysqlptr);
	return true;
}

bool mysqlcursor::close() {
	if (stmtfreeresult) {
		mysql_stmt_free_result(stmt);
		stmtfreeresult=false;
	}
	if (mysqlresult) {
		mysql_free_result(mysqlresult);
		mysqlresult=NULL;
#ifdef HAVE_MYSQL_NEXT_RESULT
		while (!mysql_next_result(mysqlconn->mysqlptr)) {
			mysqlresult=mysql_store_result(mysqlconn->mysqlptr);
			if (mysqlresult) {
				mysql_free_result(mysqlresult);
				mysqlresult=NULL;
			}
		}
#endif
	}
#ifdef HAVE_MYSQL_STMT_PREPARE
	if (mysqlmetadata) {
		mysql_free_result(mysqlmetadata);
		mysqlmetadata=NULL;
	}
#endif
	if (stmt) {
		mysql_stmt_close(stmt);
		stmt=NULL;
	}
	return true;
}
#endif

bool mysqlcursor::prepareQuery(const char *query, uint32_t size) {

	// initialize column count
	ncols=0;

	// if this if the first query of the session, do a commit first,
	// doing this will refresh this connection with any data committed
	// by other connections, which is what would happen if a new client
	// connected directly to mysql
	// FIXME: is this necessary since queryIsNotSelect() returns true?
	if (mysqlconn->firstquery) {
		// NOTE: Set firstquery to false before calling commit().  So
		// that it won't loop up if commit() needs to run a COMMIT
		// query (which will require calling prepareQuery()).
		mysqlconn->firstquery=false;
		mysqlconn->commit();
	}

#ifdef HAVE_MYSQL_STMT_PREPARE

	// reset bind-related stuff
	if (boundvariables) {
		bytestring::zero(bind,maxbindcount*sizeof(MYSQL_BIND));
	}
	boundvariables=false;
	bindformaterror=false;

	// can't use stmt API to run a couple of types of queries as of 5.0
	// (This call is a little redundant though...  the sqlrservercontroller
	// calls supportsNativeBinds for each query to see if it needs to
	// fake binds.  Unfortunately it doesn't call it for things like
	// pings or "use xxx" or other internal queries.  It might be good
	// to sort all of that out at some point.)
	if (!supportsNativeBinds(query,size)) {
		return true;
	}

	// free any lingering statements
	if (stmtfreeresult) {
		mysql_stmt_free_result(stmt);
		stmtfreeresult=false;
	}

	// free any lingering result sets
	freeResult();

	// prepare the statement
	if (mysql_stmt_prepare(stmt,query,size)) {
		stmtpreparefailed=true;
		return false;
	}

	stmtfreeresult=true;

	uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();

	// get the column count
	ncols=mysql_stmt_field_count(stmt);

	// validate the column count
	if (maxcolumncount && ncols>maxcolumncount) {
		// mysql_stmt_bind_result expects:
		// "the array (fieldbind) to contain one element for
		// each colun of the result set."
		// If there isn't, then mysql_stmt_bind_result will
		// run off the end of the array, wreaking havoc.
		// So, bail with an error if we don't have enough
		// columns.
		stringbuffer	err;
		err.append(SQLR_ERROR_MAXCOLUMNCOUNTTOOSMALL_STRING);
		err.append(" (")->append(maxcolumncount);
		err.append('<')->append(ncols)->append(')');
		conn->cont->setError(this,err.getString(),
				SQLR_ERROR_MAXCOLUMNCOUNTTOOSMALL,true);
		return false;
	}

	// allocate buffers, if necessary
	if (!maxcolumncount) {
		allocateResultSetBuffers(ncols);
	}

	// get the metadata
	mysqlmetadata=NULL;
	if (ncols) {
		mysqlmetadata=mysql_stmt_result_metadata(stmt);

		// grab the field info
		if (mysqlmetadata) {
			mysql_field_seek(mysqlmetadata,0);
			for (unsigned int i=0; i<ncols; i++) {
				mysqlfields[i]=mysql_fetch_field(mysqlmetadata);
			}
		}

		// bind the fields
		if (mysql_stmt_bind_result(stmt,fieldbind)) {
			return false;
		}
	}
#endif

	return true;
}

bool mysqlcursor::supportsNativeBinds(const char *query, uint32_t size) {
#ifdef HAVE_MYSQL_STMT_PREPARE
	usestmtprepare=mysqlconn->usestmtapi &&
			!unsupportedbystmt.match(query);
	return usestmtprepare;
#else
	return false;
#endif
}

#ifdef HAVE_MYSQL_STMT_PREPARE
bool mysqlcursor::inputBind(const char *variable, 
				uint16_t variablesize,
				const char *value, 
				uint32_t valuesize,
				int16_t *isnull) {

	if (!usestmtprepare) {
		return true;
	}

	// "variable" should be something like ?1,:2,:3, etc.
	// If it's something like :var1,:var2,:var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return false;
	}

	bindvaluesize[pos]=valuesize;

	if (*isnull) {
		bind[pos].buffer_type=MYSQL_TYPE_NULL;
		bind[pos].buffer=(void *)NULL;
		bind[pos].buffer_length=0;
		bind[pos].length=0;
	} else {
		bind[pos].buffer_type=MYSQL_TYPE_STRING;
		bind[pos].buffer=(void *)value;
		bind[pos].buffer_length=valuesize;
		bind[pos].length=&bindvaluesize[pos];
	}
	bind[pos].is_null=(my_bool *)isnull;
	boundvariables=true;

	return true;
}

bool mysqlcursor::inputBind(const char *variable, 
				uint16_t variablesize,
				int64_t *value) {

	if (!usestmtprepare) {
		return true;
	}

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return false;
	}

	bindvaluesize[pos]=sizeof(int64_t);

	bind[pos].buffer_type=MYSQL_TYPE_LONGLONG;
	bind[pos].buffer=(void *)value;
	bind[pos].buffer_length=sizeof(int64_t);
	bind[pos].length=&bindvaluesize[pos];
	bind[pos].is_null=(my_bool *)&(mysqlconn->myfalse);
	boundvariables=true;

	return true;
}

bool mysqlcursor::inputBind(const char *variable, 
				uint16_t variablesize,
				double *value,
				uint32_t precision,
				uint32_t scale) {

	if (!usestmtprepare) {
		return true;
	}

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return false;
	}

	bindvaluesize[pos]=sizeof(double);

	bind[pos].buffer_type=MYSQL_TYPE_DOUBLE;
	bind[pos].buffer=(void *)value;
	bind[pos].buffer_length=sizeof(double);
	bind[pos].length=&bindvaluesize[pos];
	bind[pos].is_null=(my_bool *)&(mysqlconn->myfalse);
	boundvariables=true;

	return true;
}

bool mysqlcursor::inputBind(const char *variable,
				uint16_t variablesize,
				int64_t year,
				int16_t month,
				int16_t day,
				int16_t hour,
				int16_t minute,
				int16_t second,
				int32_t microsecond,
				const char *tz,
				bool isnegative,
				int16_t *isnull) {

	if (!usestmtprepare) {
		return true;
	}

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return false;
	}

	bindvaluesize[pos]=sizeof(MYSQL_TIME);

	bool	validdate=(year>=0 && month>=0 && day>=0);
	bool	validtime=(hour>=0 && minute>=0 && second>=0 && microsecond>=0);

	if (*isnull || (!validdate && !validtime)) {

		bind[pos].buffer_type=MYSQL_TYPE_NULL;
		bind[pos].buffer=(void *)NULL;
		bind[pos].buffer_length=0;
		bind[pos].length=0;

	} else {

		MYSQL_TIME	*t=&(bindtime[pos]);

		// MySQL supports date, time and datetime types.
		// Decide which to use.
		if (validdate && validtime) {
			t->time_type=MYSQL_TIMESTAMP_DATETIME;
			bind[pos].buffer_type=MYSQL_TYPE_DATETIME;
		} else if (validdate) {
			t->time_type=MYSQL_TIMESTAMP_DATE;
			bind[pos].buffer_type=MYSQL_TYPE_DATE;
		} else if (validtime) {
			t->time_type=MYSQL_TIMESTAMP_TIME;
			bind[pos].buffer_type=MYSQL_TYPE_TIME;
		}

		t->year=(year>=0)?year:0;
		t->month=(month>=0)?month:0;
		t->day=(day>=0)?day:0;
		t->hour=(hour>=0)?hour:0;
		t->minute=(minute>=0)?minute:0;
		t->second=(second>=0)?second:0;
		t->second_part=(microsecond>=0)?microsecond:0;
		t->neg=(!validdate && isnegative)?TRUE:FALSE;

		bind[pos].buffer=(void *)t;
		bind[pos].buffer_length=sizeof(MYSQL_TIME);
		bind[pos].length=&bindvaluesize[pos];
	}
	bind[pos].is_null=(my_bool *)isnull;
	boundvariables=true;

	return true;
}

bool mysqlcursor::inputBindBlob(const char *variable, 
				uint16_t variablesize,
				const char *value, 
				uint32_t valuesize,
				int16_t *isnull) {

	if (!usestmtprepare) {
		return true;
	}

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return false;
	}

	bindvaluesize[pos]=valuesize;

	if (*isnull) {
		bind[pos].buffer_type=MYSQL_TYPE_NULL;
		bind[pos].buffer=(void *)NULL;
		bind[pos].buffer_length=0;
		bind[pos].length=0;
	} else {
		bind[pos].buffer_type=MYSQL_TYPE_LONG_BLOB;
		bind[pos].buffer=(void *)value;
		bind[pos].buffer_length=valuesize;
		bind[pos].length=&bindvaluesize[pos];
	}
	bind[pos].is_null=(my_bool *)isnull;
	boundvariables=true;

	return true;
}

bool mysqlcursor::inputBindClob(const char *variable, 
				uint16_t variablesize,
				const char *value, 
				uint32_t valuesize,
				int16_t *isnull) {
	return inputBindBlob(variable,variablesize,value,valuesize,isnull);
}
#endif

bool mysqlcursor::executeQuery(const char *query, uint32_t size) {

	// initialize row count
	nrows=0;

#ifdef HAVE_MYSQL_STMT_PREPARE
	if (usestmtprepare) {

		// handle binds
		if (boundvariables && mysql_stmt_bind_param(stmt,bind)) {
			return false;
		}

		// execute the query
		queryresult=mysql_stmt_execute(stmt);

		// see note inside of getLastInsertId() for why we do this here
		uint64_t	id=mysql_insert_id(mysqlconn->mysqlptr);
		if (id) {
			mysqlconn->lastinsertid=id;
		}

		if (queryresult) {
			return false;
		}

		checkForTempTable(query,size);

		// get the affected row count
		affectedrows=mysql_stmt_affected_rows(stmt);

		if (ncols) {
			stmtreset=true;
		}

	} else {
#endif

		// initialize result set
		mysqlresult=NULL;

		// execute the query
		queryresult=mysql_real_query(mysqlconn->mysqlptr,query,size);

		// see note inside of getLastInsertId() for why we do this here
		uint64_t	id=mysql_insert_id(mysqlconn->mysqlptr);
		if (id) {
			mysqlconn->lastinsertid=id;
		}

		if (queryresult) {
			return false;
		}

		checkForTempTable(query,size);

		// store the result set
		mysqlresult=mysql_store_result(mysqlconn->mysqlptr);
		if (mysqlresult==(MYSQL_RES *)NULL) {

			// if there was an error then return failure, otherwise
			// the query must have been some DML or DDL
			char	*err=(char *)mysql_error(mysqlconn->mysqlptr);
			if (!charstring::isNullOrEmpty(err)) {
				return false;
			} else {

				// get affected rows, if it was DML then 
				// this should be set
				affectedrows=mysql_affected_rows(
						mysqlconn->mysqlptr);
				return true;
			}
		}

		// get the column count
		ncols=mysql_num_fields(mysqlresult);

		// validate the column count
		uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
		if (maxcolumncount && ncols>maxcolumncount) {
			stringbuffer	err;
			err.append(SQLR_ERROR_MAXCOLUMNCOUNTTOOSMALL_STRING);
			err.append(" (")->append(maxcolumncount);
			err.append('<')->append(ncols)->append(')');
			conn->cont->setError(this,err.getString(),
				SQLR_ERROR_MAXCOLUMNCOUNTTOOSMALL,true);
			return false;
		}

		// allocate buffers, if necessary
		if (!maxcolumncount) {
			allocateResultSetBuffers(ncols);
		}

		// get the row count
		nrows=mysql_num_rows(mysqlresult);

		// get the affected row count
		affectedrows=mysql_affected_rows(mysqlconn->mysqlptr);

		// grab the field info
		if (mysqlresult) {
			mysql_field_seek(mysqlresult,0);
			for (unsigned int i=0; i<ncols; i++) {
				mysqlfields[i]=mysql_fetch_field(mysqlresult);
			}
		}

#ifdef HAVE_MYSQL_STMT_PREPARE
	}
#endif

	return true;
}

#ifdef HAVE_MYSQL_COMMIT
bool mysqlcursor::queryIsNotSelect() {
	// Kludge.  The controller uses this to decide whether to run a
	// commit/rollback at the end of the session.  If it returns true
	// for any query during the session then commit/rollback will be run.
	// MySQL needs a commit/rollback to be run even if only selects were
	// run to release metadata locks.  (I originally thought this was only
	// true if the isolation level is set to repeatable-read (the default)
	// but it appears to be necessary for all isolation levels.)  We'll
	// trick the controller into running commit/rollback no matter what by
	// returning true for any query.
	return true;
}
#endif

void mysqlcursor::getError(char *errorbuffer,
				uint32_t errorbuffersize,
				uint32_t *errorsize,
				int64_t *errorcode,
				bool *liveconnection) {

	*liveconnection=true;

	const char	*err;
	unsigned int	errn;
	#ifdef HAVE_MYSQL_STMT_PREPARE
	if (bindformaterror) {
		errn=SQLR_ERROR_INVALIDBINDVARIABLEFORMAT;
		err=SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING;
	} else {
		if (usestmtprepare) {
			err=mysql_stmt_error(stmt);
			errn=mysql_stmt_errno(stmt);
		} else {
	#endif
			err=mysql_error(mysqlconn->mysqlptr);
			errn=mysql_errno(mysqlconn->mysqlptr);
	#ifdef HAVE_MYSQL_STMT_PREPARE
		}
	}
	#endif

	// Below we check both queryresult and errn.  At one time, we only
	// checked queryresult.  This may have been a bug.  It's possible that
	// back then we should have only checked errn.  But I have a fuzzy
	// memory of some version of mysql returning these error codes in
	// queryresult, so for now I'll leave that code and check both.
	#if defined(HAVE_MYSQL_CR_SERVER_GONE_ERROR) || \
			defined(HAVE_MYSQL_CR_SERVER_LOST) 
		#ifdef HAVE_MYSQL_CR_SERVER_GONE_ERROR
		if (queryresult==CR_SERVER_GONE_ERROR ||
				errn==CR_SERVER_GONE_ERROR) {
			*liveconnection=false;
		} else
		#endif
		#ifdef HAVE_MYSQL_CR_SERVER_LOST
		if (queryresult==CR_SERVER_GONE_ERROR /*||
				errn==CR_SERVER_LOST*/) {
			*liveconnection=false;
		} else
		#endif
	#endif
	if (!charstring::compare(err,"") ||
		!charstring::compareIgnoringCase(err,
				"mysql server has gone away",26) ||
		!charstring::compareIgnoringCase(err,
				"Can't connect to local MySQL",28) ||
		!charstring::compareIgnoringCase(err,
				"Can't connect to MySQL",22) ||
		!charstring::compareIgnoringCase(err,
			"Lost connection to MySQL server during query",44)) {
		*liveconnection=false;
	}

	// set return values
	*errorsize=charstring::getLength(err);
	charstring::safeCopy(errorbuffer,errorbuffersize,err,*errorsize);
	*errorcode=errn;
}

uint32_t mysqlcursor::colCount() {
	return ncols;
}

bool mysqlcursor::knowsRowCount() {
#ifdef HAVE_MYSQL_STMT_PREPARE
	return !usestmtprepare;
#else
	return true;
#endif
}

uint64_t mysqlcursor::rowCount() {
	return nrows;
}

uint64_t mysqlcursor::getAffectedRows() {
	return affectedrows;
}

const char *mysqlcursor::getColumnName(uint32_t col) {
	return mysqlfields[col]->name;
}

#ifdef HAVE_MYSQL_FIELD_NAME_LENGTH
uint16_t mysqlcursor::getColumnNameSize(uint32_t col) {
	return mysqlfields[col]->name_length;
}
#endif

uint16_t mysqlcursor::getColumnType(uint32_t col) {
	switch (mysqlfields[col]->type) {
		case FIELD_TYPE_STRING:
			return STRING_DATATYPE;
		case FIELD_TYPE_VAR_STRING:
			return VARSTRING_DATATYPE;
		case FIELD_TYPE_DECIMAL:
			return DECIMAL_DATATYPE;
#ifdef HAVE_MYSQL_FIELD_TYPE_NEWDECIMAL
		case FIELD_TYPE_NEWDECIMAL:
			return DECIMAL_DATATYPE;
#else
		case 246:
			// the FIELD_TYPE_NEWDECIMAL enum isn't defined on
			// MySQL 4.x but the number 246 (the value of that
			// enum) is used
			return DECIMAL_DATATYPE;
#endif
		case FIELD_TYPE_TINY:
			return TINYINT_DATATYPE;
		case FIELD_TYPE_SHORT:
			return SMALLINT_DATATYPE;
		case FIELD_TYPE_LONG:
			return INT_DATATYPE;
		case FIELD_TYPE_FLOAT:
			return FLOAT_DATATYPE;
		case FIELD_TYPE_DOUBLE:
			return REAL_DATATYPE;
		case FIELD_TYPE_LONGLONG:
			return BIGINT_DATATYPE;
		case FIELD_TYPE_INT24:
			return MEDIUMINT_DATATYPE;
		case FIELD_TYPE_TIMESTAMP:
			return TIMESTAMP_DATATYPE;
		case FIELD_TYPE_DATE:
			return DATE_DATATYPE;
		case FIELD_TYPE_TIME:
			return TIME_DATATYPE;
		case FIELD_TYPE_DATETIME:
			return DATETIME_DATATYPE;
#ifdef HAVE_MYSQL_FIELD_TYPE_YEAR
		case FIELD_TYPE_YEAR:
			return YEAR_DATATYPE;
#endif
#ifdef HAVE_MYSQL_FIELD_TYPE_NEWDATE
		case FIELD_TYPE_NEWDATE:
			return NEWDATE_DATATYPE;
#endif
		case FIELD_TYPE_NULL:
			return NULL_DATATYPE;
#ifdef HAVE_MYSQL_FIELD_TYPE_ENUM
		case FIELD_TYPE_ENUM:
			return ENUM_DATATYPE;
#endif
#ifdef HAVE_MYSQL_FIELD_TYPE_SET
		case FIELD_TYPE_SET:
			return SET_DATATYPE;
#endif
		case FIELD_TYPE_TINY_BLOB:
			return TINY_BLOB_DATATYPE;
		// For some versions of mysql, tinyblobs, mediumblobs and
		// longblobs all show up as FIELD_TYPE_BLOB despite field types
		// being defined for those types.  The different types have
		// predictable sizes though, so we'll use those to
		// differentiate them. 
		case FIELD_TYPE_BLOB:
			#if defined(MYSQL_VERSION_ID) && \
					MYSQL_VERSION_ID>100505
				if (mysqlfields[col]->flags&BINARY_FLAG) {
					// MariaDB 10.6+ and some versions of
					// 10.5 appear to use these sizes for
					// blobs
					if (mysqlfields[col]->length<=255) {
						return TINY_BLOB_DATATYPE;
					} else if (mysqlfields[col]->
							length<=65535) {
						return BLOB_DATATYPE;
					} else if (mysqlfields[col]->
							length<=16777215) {
						return MEDIUM_BLOB_DATATYPE;
					} else {
						return LONG_BLOB_DATATYPE;
					}
				} else {
					// ...and these lengths for texts
					if (mysqlfields[col]->length<=1020) {
						return TINY_BLOB_DATATYPE;
					} else if (mysqlfields[col]->
							length<=262140) {
						return BLOB_DATATYPE;
					} else if (mysqlfields[col]->
							length<=67108860) {
						return MEDIUM_BLOB_DATATYPE;
					} else {
						return LONG_BLOB_DATATYPE;
					}
				}
			#elif defined(MYSQL_VERSION_ID) && \
					MYSQL_VERSION_ID>=100000
				// MariaDB 10.5- appears to use these lengths
				// for both blobs and texts
				if (mysqlfields[col]->length<=255) {
					return TINY_BLOB_DATATYPE;
				} else if (mysqlfields[col]->length<=65535) {
					return BLOB_DATATYPE;
				} else if (mysqlfields[col]->length<=16777215) {
					return MEDIUM_BLOB_DATATYPE;
				} else {
					return LONG_BLOB_DATATYPE;
				}
			#elif defined(MYSQL_VERSION_ID) && \
					MYSQL_VERSION_ID>=50000
				// MySQL 5/8 appears to use these sizes
				// for both blobs and texts
				if (mysqlfields[col]->length<=765) {
					return TINY_BLOB_DATATYPE;
				} else if (mysqlfields[col]->length<=196605) {
					return BLOB_DATATYPE;
				} else if (mysqlfields[col]->length<=50441645) {
					return MEDIUM_BLOB_DATATYPE;
				} else {
					return LONG_BLOB_DATATYPE;
				}
			#else
				// MySQL 3/4 uses these sizes for tiny and
				// blob datatypes.  Medium and long both use
				// the same size but are distinguishable by
				// their max_lengths of 11 and 9 respectively.
				// No idea what the 11 and 9 actually mean.
				// Text types are the same.
				if (mysqlfields[col]->length<=255) {
					return TINY_BLOB_DATATYPE;
				} else if (mysqlfields[col]->length<=65535) {
					return BLOB_DATATYPE;
				} else if (mysqlfields[col]->length<=16777215 &&
					mysqlfields[col]->max_length==11) {
					return MEDIUM_BLOB_DATATYPE;
				} else {
					return LONG_BLOB_DATATYPE;
				}
			#endif
		case FIELD_TYPE_MEDIUM_BLOB:
			return MEDIUM_BLOB_DATATYPE;
		case FIELD_TYPE_LONG_BLOB:
			return LONG_BLOB_DATATYPE;
		default:
			return UNKNOWN_DATATYPE;
	}
}

uint32_t mysqlcursor::getColumnSize(uint32_t col) {

	switch (getColumnType(col)) {
		case STRING_DATATYPE:
			return (uint32_t)mysqlfields[col]->length;
		case VARSTRING_DATATYPE:
			return (uint32_t)mysqlfields[col]->length+1;
		case DECIMAL_DATATYPE:
			{
			uint32_t	size=mysqlfields[col]->length+1;
			unsigned int	decimals=mysqlfields[col]->decimals;
			if (decimals>0) {
				size++;
			}
			if (mysqlfields[col]->length<decimals) {
				size=decimals+2;
			}
			return size;
			}
		case TINYINT_DATATYPE:
			return 1;
		case SMALLINT_DATATYPE:
			return 2;
		case INT_DATATYPE:
			return 4;
		case FLOAT_DATATYPE:
			return (mysqlfields[col]->length<=24)?4:8;
		case REAL_DATATYPE:
			return 8;
		case BIGINT_DATATYPE:
			return 8;
		case MEDIUMINT_DATATYPE:
			return 3;
		case TIMESTAMP_DATATYPE:
			return 4;
		case DATE_DATATYPE:
			return 3;
		case TIME_DATATYPE:
			return 3;
		case DATETIME_DATATYPE:
			return 8;
#ifdef HAVE_MYSQL_FIELD_TYPE_YEAR
		case YEAR_DATATYPE:
			return 1;
#endif
#ifdef HAVE_MYSQL_FIELD_TYPE_NEWDATE
		case NEWDATE_DATATYPE:
			return 1;
#endif
		case NULL_DATATYPE:
#ifdef HAVE_MYSQL_FIELD_TYPE_ENUM
		case ENUM_DATATYPE:
			// 1 or 2 bytes delepending
			// on the # of enum values (65535 max)
			return 2;
#endif
#ifdef HAVE_MYSQL_FIELD_TYPE_SET
		case SET_DATATYPE:
			// 1,2,3,4 or 8 bytes depending
			// on the # of members (64 max)
			return 8;
#endif
		case TINY_BLOB_DATATYPE:
			return 255;
		case BLOB_DATATYPE:
			return 65535;
		case MEDIUM_BLOB_DATATYPE:
			return 16777215;
		case LONG_BLOB_DATATYPE:
			return 2147483647;
	}
	return (uint32_t)mysqlfields[col]->length;
}

uint32_t mysqlcursor::getColumnPrecision(uint32_t col) {
	return mysqlfields[col]->length;
}

uint32_t mysqlcursor::getColumnScale(uint32_t col) {
	return mysqlfields[col]->decimals;
}

uint16_t mysqlcursor::getColumnIsNullable(uint32_t col) {
	return !(IS_NOT_NULL(mysqlfields[col]->flags));
}

uint16_t mysqlcursor::getColumnIsPrimaryKey(uint32_t col) {
	return IS_PRI_KEY(mysqlfields[col]->flags);
}

uint16_t mysqlcursor::getColumnIsUnique(uint32_t col) {
	return mysqlfields[col]->flags&UNIQUE_KEY_FLAG;
}

uint16_t mysqlcursor::getColumnIsPartOfKey(uint32_t col) {
	return mysqlfields[col]->flags&MULTIPLE_KEY_FLAG;
}

uint16_t mysqlcursor::getColumnIsUnsigned(uint32_t col) {
	return mysqlfields[col]->flags&UNSIGNED_FLAG;
}

uint16_t mysqlcursor::getColumnIsZeroFilled(uint32_t col) {
	return mysqlfields[col]->flags&ZEROFILL_FLAG;
}

uint16_t mysqlcursor::getColumnIsBinary(uint32_t col) {
	#ifdef BINARY_FLAG
		return mysqlfields[col]->flags&BINARY_FLAG;
	#else
		return 0;
	#endif
}

uint16_t mysqlcursor::getColumnIsAutoIncrement(uint32_t col) {
	#ifdef AUTO_INCREMENT_FLAG
		return mysqlfields[col]->flags&AUTO_INCREMENT_FLAG;
	#else
		return 0;
	#endif
}

#ifdef HAVE_MYSQL_FIELD_ORG_TABLE
const char *mysqlcursor::getColumnTable(uint32_t col) {
	return mysqlfields[col]->org_table;
}
#endif

#ifdef HAVE_MYSQL_FIELD_ORG_TABLE_LENGTH
uint16_t mysqlcursor::getColumnTableSize(uint32_t col) {
	return mysqlfields[col]->org_table_length;
}
#endif

bool mysqlcursor::noRowsToReturn() {
	// for DML or DDL queries, return no data
#ifdef HAVE_MYSQL_STMT_PREPARE
	if (usestmtprepare) {
		return (!mysqlmetadata);
	}
#endif
	return (!mysqlresult);
}

bool mysqlcursor::fetchRow(bool *error) {

	*error=false;

#ifdef HAVE_MYSQL_STMT_PREPARE
	if (usestmtprepare) {
		int	result=mysql_stmt_fetch(stmt);
		if (result==1) {
			*error=true;
			return false;
		} else if (result==MYSQL_NO_DATA) {
			stmtreset=false;
			return false;
		}
		return !result;
	} else {
#endif
		mysqlrow=mysql_fetch_row(mysqlresult);
		if (!mysqlrow) {
			if (*mysql_error(mysqlconn->mysqlptr)) {
				*error=true;
			}
			return false;
		}
		mysqlrowsizes=mysql_fetch_lengths(mysqlresult);
		if (!mysqlrowsizes) {
			if (*mysql_error(mysqlconn->mysqlptr)) {
				*error=true;
			}
			return false;
		}
		return true;
#ifdef HAVE_MYSQL_STMT_PREPARE
	}
#endif
}

void mysqlcursor::getField(uint32_t col,
				const char **fld, uint64_t *fldsize,
				bool *lob, bool *null) {

#ifdef HAVE_MYSQL_STMT_PREPARE
	if (usestmtprepare) {
		if (!isnull[col]) {
			// use conn->cont->getColumnType() instead of
			// this->getColumnType() in case a column has been
			// remapped (eg. for getting odbc-format column lists)
			uint16_t	coltype=
					conn->cont->getColumnType(this,col);
			if (coltype==TINY_BLOB_DATATYPE ||
				coltype==BLOB_DATATYPE ||
				coltype==MEDIUM_BLOB_DATATYPE ||
				coltype==LONG_BLOB_DATATYPE) {
				*lob=true;
				return;
			} else {
				*fld=&field[col*
					conn->cont->getMaxFieldSize()];
				*fldsize=fieldsize[col];
			}
		} else {
			*null=true;
		}
	} else {
#endif
		if (mysqlrow[col]) {
			*fld=mysqlrow[col];
			*fldsize=mysqlrowsizes[col];
		} else {
			*null=true;
		}
#ifdef HAVE_MYSQL_STMT_PREPARE
	}
#endif
}

#ifdef HAVE_MYSQL_STMT_PREPARE
bool mysqlcursor::getLobFieldLength(uint32_t col, uint64_t *length)  {

	// lobfield needs to be zero'ed prior to each call to
	// mysql_stmt_fetch_column() because mysql_stmt_fetch_column()
	// fiddles with its member variables, and we don't want stale
	// values (especially pointers) lingering across uses
	bytestring::zero(&lobfield,sizeof(MYSQL_BIND));
	lobfield.buffer_type=MYSQL_TYPE_STRING;
	lobfield.buffer_length=fieldsize[col];
	*length=lobfield.buffer_length;

	// mariadb-client-lgpl_2.0.0 crashes if the size pointer isn't set
	lobfield.length=&lobfieldlength;

	return true;
}

bool mysqlcursor::getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {

	// mysql can't fetch the lob in chunks so we need to fetch the
	// entire thing into a buffer here at the beginning
	if (!offset) {
		lobfield.buffer=new char[lobfield.buffer_length];
		if (mysql_stmt_fetch_column(stmt,&lobfield,col,0)) {
			return false;
		}
	}

	// sanity checks
	if (!lobfield.buffer || offset>lobfield.buffer_length) {
		return false;
	}

	// deterine how many characters to actually read
	*charsread=charstoread;
	if (charstoread>lobfield.buffer_length-offset) {
		*charsread=lobfield.buffer_length-offset;
	}

	// copy out the data
	bytestring::copy(buffer,(byte_t *)lobfield.buffer+offset,*charsread);

	return true;
}

void mysqlcursor::closeLobField(uint32_t col) {
	delete[] (char *)lobfield.buffer;
	lobfield.buffer=NULL;
	lobfield.buffer_length=0;
	return;
}
#endif

void mysqlcursor::closeResultSet() {
#ifdef HAVE_MYSQL_STMT_PREPARE
	if (usestmtprepare) {

		if (stmtreset) {
			mysql_stmt_reset(stmt);
			stmtreset=false;
		}

		// In mariadb-client-lgpl_2.x, if a mysql_stmt_prepare fails,
		// then subsequent attempts to prepare the same stmt again fail
		// with: "Unknown prepared statement handler (27) given to
		// mysqld_stmt_reset" unless the statement is closed and
		// reopened.
		if (stmtpreparefailed) {
			mysql_stmt_close(stmt);
			stmt=mysql_stmt_init(mysqlconn->mysqlptr);
			stmtpreparefailed=false;
		}
	} else {
		freeResult();
	}
#else
	freeResult();
#endif
}

void mysqlcursor::freeResult() {
	if (mysqlresult) {
		mysql_free_result(mysqlresult);
		mysqlresult=NULL;
#ifdef HAVE_MYSQL_NEXT_RESULT
		while (!mysql_next_result(mysqlconn->mysqlptr)) {
			mysqlresult=mysql_store_result(mysqlconn->mysqlptr);
			if (mysqlresult) {
				mysql_free_result(mysqlresult);
				mysqlresult=NULL;
			}
		}
#endif
	}
#ifdef HAVE_MYSQL_STMT_PREPARE
	if (mysqlmetadata) {
		mysql_free_result(mysqlmetadata);
		mysqlmetadata=NULL;
	}
#endif
	if (!conn->cont->getMaxColumnCount()) {
		deallocateResultSetBuffers();
	}
}

bool mysqlcursor::columnInfoIsValidAfterPrepare() {
	return true;
}

void mysqlcursor::encodeBlob(stringbuffer *buffer,
					const char *data, uint32_t datasize) {
	if (!mysqlconn->escapeblobs) {
		sqlrservercursor::encodeBlob(buffer,data,datasize);
		return;
	}
	buffer->append('\'');
	for (uint32_t i=0; i<datasize; i++) {
		switch (data[i]) {
			case '\'':
				buffer->append('\\');
				buffer->append('\'');
				break;
			case '"':
				buffer->append('\\');
				buffer->append('"');
				break;
			case '\n':
				buffer->append('\\');
				buffer->append('n');
				break;
			case '\r':
				buffer->append('\\');
				buffer->append('r');
				break;
			case '\\':
				buffer->append('\\');
				buffer->append('\\');
				break;
			case 26:
				buffer->append('\\');
				buffer->append('Z');
				break;
			default:
				// FIXME: what about binary data?
				buffer->append(data[i]);
				break;
		}
	}
	buffer->append('\'');
}
