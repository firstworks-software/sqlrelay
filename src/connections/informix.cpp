// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/environment.h>

#include <datatypes.h>
#include <defines.h>
#include <config.h>

#ifdef INFORMIX_AT_RUNTIME
	#include "informixatruntime.cpp"
#else
	#include <infxcli.h>
#endif

#define MAX_OUT_BIND_LOB_SIZE	2097152

#define MAX_LOB_CHUNK_SIZE	2147483647

struct informixcolumn {
	char		*name;
	SQLSMALLINT	namesize;
	SQLLEN		type;
	SQLLEN		precision;
	SQLLEN		scale;
	SQLLEN		flags;
	SQLLEN		primarykey;
	SQLLEN		unique;
	SQLLEN		partofkey;
	SQLLEN		unsignednumber;
	SQLLEN		zerofill;
	SQLLEN		binary;
	SQLLEN		autoincrement;
	char		table[4096];
	uint16_t	tablesize;
};

struct datebind {
	int16_t			*year;
	int16_t			*month;
	int16_t			*day;
	int16_t			*hour;
	int16_t			*minute;
	int16_t			*second;
	int32_t			*microsecond;
	const char		**tz;
	SQL_TIMESTAMP_STRUCT	buffer;
};

class informixconnection;

class SQLRSERVER_DLLSPEC informixcursor : public sqlrservercursor {
	friend class informixconnection;
	public:
		informixcursor(sqlrserverconnection *conn, uint16_t id);
		~informixcursor();
	private:
		void		allocateResultSetBuffers(int32_t columncount);
		void		deallocateResultSetBuffers();
		bool		open();
		bool		close();
		bool		prepareQuery(const char *query,
						uint32_t size);
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
		bool		outputBind(const char *variable, 
						uint16_t variablesize,
						char *value, 
						uint32_t valuesize,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						int64_t *value,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						double *value,
						uint32_t *precision,
						uint32_t *scale,
						int16_t *isnull);
		bool		outputBind(const char *variable,
						uint16_t variablesize,
						int16_t *year,
						int16_t *month,
						int16_t *day,
						int16_t *hour,
						int16_t *minute,
						int16_t *second,
						int32_t *microsecond,
						const char **tz,
						bool *isnegative,
						int16_t *isnull);
		bool		outputBindBlob(const char *variable, 
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		bool		outputBindClob(const char *variable, 
						uint16_t variablesize,
						uint16_t index,
						int16_t *isnull);
		bool		getLobOutputBindLength(uint16_t index,
							uint64_t *length);
		bool		getLobOutputBindSegment(uint16_t index,
							char *buffer,
							uint64_t buffersize,
							uint64_t offset,
							uint64_t charstoread,
							uint64_t *charsread);
		bool		executeQuery(const char *query,
						uint32_t size);
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t i);
		uint16_t	getColumnNameSize(uint32_t i);
		uint16_t	getColumnType(uint32_t i);
		uint32_t	getColumnSize(uint32_t i);
		uint32_t	getColumnPrecision(uint32_t i);
		uint32_t	getColumnScale(uint32_t i);
		uint16_t	getColumnIsNullable(uint32_t i);
		uint16_t	getColumnIsUnsigned(uint32_t i);
		uint16_t	getColumnIsBinary(uint32_t i);
		uint16_t	getColumnIsAutoIncrement(uint32_t i);
		const char	*getColumnTable(uint32_t i);
		uint16_t	getColumnTableSize(uint32_t i);
		bool		noRowsToReturn();
		bool		skipRow(bool *error);
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **fld,
					uint64_t *fldsize,
					bool *lob,
					bool *null);
		void		nextRow();
		bool		getLobFieldLength(uint32_t col,
							uint64_t *length);
		bool		getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread);
		void		closeResultSet();

		SQLRETURN	erg;
		SQLHSTMT	stmt;
		SQLSMALLINT	ncols;
		SQLLEN 		affectedrows;

		int32_t		columncount;
		char		**field;
		SQLLEN		**loblength;
		SQLLEN		**indicator;
		informixcolumn	*column;

		uint16_t		maxbindcount;
		SQLLEN			*lobbindsize;
		SQL_DATE_STRUCT		*indatebind;
		SQL_TIMESTAMP_STRUCT	*intsbind;
		datebind		**outdatebind;
		char			**outlobbind;
		SQLLEN			*outlobbindlen;
		int16_t			**outisnullptr;
		SQLLEN			*outisnull;
		SQLLEN			sqlnulldata;
		BOOL			truevalue;

		uint64_t	rowgroupindex;
		uint64_t	totalinrowgroup;
		uint64_t	totalrows;
		uint64_t	rownumber;

		bool		noop;

		bool		bindformaterror;

		stringbuffer	errormsg;

		informixconnection	*informixconn;
};

class SQLRSERVER_DLLSPEC informixconnection : public sqlrserverconnection {
	friend class informixcursor;
	public:
		informixconnection(sqlrservercontroller *cont);
		~informixconnection();
	private:
		void	handleConnectString();
		bool	logIn(const char **error, const char **warning);
		const char	*logInError(const char *errmsg);
		sqlrservercursor	*newCursor(uint16_t id);
		void	deleteCursor(sqlrservercursor *curs);
		void	logOut();
		int16_t	getNullBindValue();
		bool	setAutoCommitOn();
		bool	setAutoCommitOff();
		bool	supportsTransactionBlocks();
		bool	commit();
		bool	rollback();
		void	getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t	*errorcode,
					bool *liveconnection);
		bool	liveConnection(SQLINTEGER nativeerror,
					const char *errorbuffer,
					SQLSMALLINT errsize);
		const char	*pingQuery();
		const char	*getDbType();
		const char	*getDbVersion();
		const char	*getDbHostNameQuery();
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
		const char	*selectCatalogQuery();
		const char	*getCurrentCatalogQuery();
		const char	*getCurrentSchemaQuery();
		const char	*getLastInsertIdQuery();
		const char	*setIsolationLevelQuery();
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		const char	*getNoopQuery();
		const char	*getBindFormat();

		SQLHENV		env;
		SQLRETURN	erg;
		SQLHDBC		dbc;

		const char	*informixdir;
		const char	*servername;
		const char	*db;
		const char	*lang;
		stringbuffer	dsn;

		stringbuffer	errormessage;

		int32_t		maxoutbindlobsize;

		char		dbversion[512];
		char		**databasefeatures;

		stringbuffer	cataloglistquery;
		stringbuffer	schemalistquery;
		stringbuffer	tabletypelistquery;
		stringbuffer	tablelistquery;
		stringbuffer	procedurelistquery;
		stringbuffer	columnlistquery;
		stringbuffer	typeinfolistquery;
		stringbuffer	primarykeyslistquery;
		stringbuffer	keyandindexlistquery;
		stringbuffer	procedureparameterlistquery;

		stringbuffer	errormsg;
};

informixconnection::informixconnection(sqlrservercontroller *cont) :
					sqlrserverconnection(cont) {

	maxoutbindlobsize=MAX_OUT_BIND_LOB_SIZE;
	databasefeatures=NULL;
}

informixconnection::~informixconnection() {
	if (databasefeatures) {
		for (int i=0; i<FEATURE_COUNT; i++) {
			delete[] databasefeatures[i];
		}
		delete[] databasefeatures;
	}
}

void informixconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	// get informix dir
	informixdir=cont->getConnectStringValue("informixdir");

	// get dsn components
	servername=cont->getConnectStringValue("servername");
	if (charstring::isNullOrEmpty(servername)) {
		servername=environment::getValue("INFORMIXSERVER");
	}
	db=cont->getConnectStringValue("db");

	// build dsn
	dsn.clear();
	if (!charstring::isNullOrEmpty(servername)) {
		dsn.append("Servername=")->append(servername);
	}
	if (!charstring::isNullOrEmpty(db)) {
		if (dsn.getSize()) {
			dsn.append(";");
		}
		dsn.append("Database=")->append(db);
	}
	const char	*user=cont->getUser();
	if (!charstring::isNullOrEmpty(user)) {
		if (dsn.getSize()) {
			dsn.append(";");
		}
		dsn.append("LogonID=")->append(user);
	}
	const char	*pass=cont->getPassword();
	if (!charstring::isNullOrEmpty(pass)) {
		if (dsn.getSize()) {
			dsn.append(";");
		}
		dsn.append("pwd=")->append(pass);
	}

	// get other parameters
	lang=cont->getConnectStringValue("lang");

	// multi-row fetch doesn't work with clobs/blobs because you're already
	// on a different row when SQLGetData is called to get the data for the
	// clob/blob on the first row, so override it to 1
	cont->setFetchAtOnce(1);

	maxoutbindlobsize=charstring::convertToInteger(
			cont->getConnectStringValue("maxoutbindlobsize"));
	if (maxoutbindlobsize<1) {
		maxoutbindlobsize=MAX_OUT_BIND_LOB_SIZE;
	}
}

bool informixconnection::logIn(const char **error, const char **warning) {

	// set the INFORMIX environment variable
	if (charstring::getLength(informixdir) &&
		!environment::setValue("INFORMIXDIR",informixdir)) {
		*error="Failed to set INFORMIXDIR environment variable";
		return false;
	}

	// set the LANG environment variable
	if (charstring::getLength(lang) &&
		!environment::setValue("LANG",lang)) {
		*error="Failed to set LANG environment variable";
		return false;
	}

	#ifdef INFORMIX_AT_RUNTIME
	if (!loadLibraries(&errormessage)) {
		*error=errormessage.getString();
		return NULL;
	}
	#endif

	// allocate environment handle
	erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		*error="Failed to allocate environment handle";
		SQLFreeHandle(SQL_HANDLE_ENV,env);
		return false;
	}

	// allocate connection handle
	erg=SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		*error="Failed to allocate connection handle";
		SQLFreeHandle(SQL_HANDLE_DBC,dbc);
		SQLFreeHandle(SQL_HANDLE_ENV,env);
		return false;
	}

	// set the connect timeout
	uint32_t	connecttimeout=cont->getConnectTimeout();
	if (connecttimeout) {
		erg=SQLSetConnectAttr(dbc,
				#ifdef SQL_ATTR_LOGIN_TIMEOUT
				SQL_ATTR_LOGIN_TIMEOUT,
				#else
				SQL_LOGIN_TIMEOUT,
				#endif
				(SQLPOINTER)connecttimeout,0);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			*error="Failed to set connect timeout";
			SQLFreeHandle(SQL_HANDLE_DBC,dbc);
			SQLFreeHandle(SQL_HANDLE_ENV,env);
			return false;
		}
	}

	// connect to the database
	erg=SQLDriverConnect(dbc,NULL,
				(SQLCHAR *)dsn.getString(),
				dsn.getSize(),
				NULL,0,NULL,SQL_DRIVER_COMPLETE);
	if (erg==SQL_SUCCESS_WITH_INFO) {
		*warning=logInError(NULL);
	} else if (erg!=SQL_SUCCESS) {
		*error=logInError("SQLConnect failed");
		SQLFreeHandle(SQL_HANDLE_DBC,dbc);
		SQLFreeHandle(SQL_HANDLE_ENV,env);
		return false;
	}

	// get db version
	SQLSMALLINT	dbversionlen;
	SQLGetInfo(dbc,SQL_DBMS_VER,
			(SQLPOINTER)dbversion,
			(SQLSMALLINT)sizeof(dbversion),
			&dbversionlen);

	return true;
}

const char *informixconnection::logInError(const char *errmsg) {

	errormessage.clear();
	if (errmsg) {
		errormessage.append(errmsg)->append(": ");
	}

	// get the error message from informix
	SQLCHAR		state[10];
	SQLINTEGER	nativeerrnum;
	SQLCHAR		errorbuffer[1024];
	SQLSMALLINT	errsize;

	SQLGetDiagRec(SQL_HANDLE_DBC,dbc,1,state,&nativeerrnum,
					errorbuffer,1024,&errsize);
	errormessage.append(errorbuffer,errsize);
	return errormessage.getString();
}

sqlrservercursor *informixconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new informixcursor(
					(sqlrserverconnection *)this,id);
}

void informixconnection::deleteCursor(sqlrservercursor *curs) {
	delete (informixcursor *)curs;
}

void informixconnection::logOut() {
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC,dbc);
	SQLFreeHandle(SQL_HANDLE_ENV,env);
}

int16_t informixconnection::getNullBindValue() {
	return SQL_NULL_DATA;
}

bool informixconnection::setAutoCommitOn() {
	return (SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_ON,
				sizeof(SQLINTEGER))==SQL_SUCCESS);
}

bool informixconnection::setAutoCommitOff() {
	return (SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_OFF,
				sizeof(SQLINTEGER))==SQL_SUCCESS);
}

bool informixconnection::supportsTransactionBlocks() {
	return false;
}

bool informixconnection::commit() {
	return (SQLEndTran(SQL_HANDLE_ENV,env,SQL_COMMIT)==SQL_SUCCESS);
}

bool informixconnection::rollback() {
	return (SQLEndTran(SQL_HANDLE_ENV,env,SQL_ROLLBACK)==SQL_SUCCESS);
}

void informixconnection::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {
	SQLCHAR		state[10];
	SQLINTEGER	nativeerrnum;
	SQLSMALLINT	errsize;

	SQLGetDiagRec(SQL_HANDLE_DBC,dbc,1,state,&nativeerrnum,
				(SQLCHAR *)errorbuffer,errorbuffersize,
				&errsize);

	// set return values
	*errorsize=errsize;
	*errorcode=nativeerrnum;
	*liveconnection=liveConnection(nativeerrnum,errorbuffer,errsize);
}

bool informixconnection::liveConnection(SQLINTEGER nativeerrnum,
					const char *errorbuffer,
					SQLSMALLINT errsize) {

	// When the DB goes down, Informix reports:
	// -11020: [Informix][Informix ODBC Driver]Communication link failure.
	// (if there are other errors then I haven't seen them yet)
	return nativeerrnum!=-11020;
}


const char *informixconnection::pingQuery() {
	return "select 1 from sysmaster:sysdual";
}

const char *informixconnection::getDbType() {
	return "informix";
}

const char *informixconnection::getDbVersion() {
	return dbversion;
}

const char *informixconnection::getDbHostNameQuery() {
	return "select dbinfo('dbhostname') from sysmaster:sysdual";
	//return "select os_nodename from sysmaster:sysmachineinfo";
}

const char *informixconnection::getCatalogListQuery(const char *catalog) {

	cataloglistquery.clear();

	// select clause
	cataloglistquery.append(
		"select "
		"	trim(name) as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	'' "
		"from "
		"	sysmaster:sysdatabases ");

	// where clause
	if (catalog) {
		cataloglistquery.append(
			"where "
			"	name like '");
		cataloglistquery.append(catalog);
		cataloglistquery.append("' ");
	}

	// order by clause
	cataloglistquery.append(
		"order by "
		"	table_cat");

	return cataloglistquery.getString();
}

const char *informixconnection::getSchemaListQuery(const char *catalog,
							const char *schema) {

	// FIXME:  This only returns users that own at least one table.
	// There doesn't appear to be a way to get a generic list of users.

	schemalistquery.clear();

	// select clause
	schemalistquery.append(
		"select distinct "
		"	trim(dbsname) as table_cat, "
		"	trim(owner) as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	'' "
		"from "
		"	sysmaster:systabnames ");

	// where clause
	if (catalog || schema) {
		schemalistquery.append("where ");
		bool	first=true;
		if (catalog) {
			schemalistquery.append(
				"	dbsname like '");
			schemalistquery.append(catalog);
			schemalistquery.append("' ");
			first=false;
		}
		if (schema) {
			if (!first) {
				schemalistquery.append("	and ");
			}
			schemalistquery.append(
				"	owner like '");
			schemalistquery.append(schema);
			schemalistquery.append("' ");
		}
	}

	// order by clause
	schemalistquery.append(
		"order by "
		"	table_cat, "
		"	table_schem");

	return schemalistquery.getString();
}

const char *informixconnection::getTableTypeListQuery(
						const char *catalog,
						const char *schema,
						const char *tabletypes) {

	tabletypelistquery.clear();

	// select clause
	tabletypelistquery.append(
		"select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	trim(table_type), "
		"	'' as remarks, "
		"	'' "
		"from "
		"	(select "
		"		'SYNONYM' as table_type "
		"	from "
		"		systables where tabid=1 "
		"	union "
		"	select "
		"		'TABLE' as table_type "
		"	from "
		"		systables where tabid=1 "
		"	union "
		"	select "
		"		'VIEW' as table_type "
		"	from "
		"	systables where tabid=1) t ");

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

const char *informixconnection::getTableListQuery(
					const char *catalog,
					const char *schema,
					const char *table,
					uint16_t objecttypes) {

	// This is a mess.  The sysmaster:systabnames view doesn't have an
	// object type column.  sysmaster:systabinfo has a ti_flags bitmap
	// column, but it doesn't appear to have values for table, view,
	// synonym, etc.  The systables view has a tabtype column, but its
	// dbname column is reliably empty, and it only shows tables in the
	// current catalog/schema.
	//
	// If we want to be able to filter by catalog, schema, and table then we
	// have to use sysmaster:systabnames.
	// If we want to be able to filter by objecttypes then we have to use
	// systabes, but we can only really do that if catalog/schema are the
	// current catalog/schema.
	//
	// For now, we're ignoring objecttypes.

	tablelistquery.clear();

	// select clause
	tablelistquery.append(
		"select distinct "
		"	trim(dbsname) as table_cat, "
		"	trim(owner) as table_schem, "
		"	trim(tabname) as table_name, "
		"	'TABLE' as table_type, "
		"	'' as remarks, "
		"	'' "
		"from "
		"	sysmaster:systabnames ");

	// where clause
	if (catalog || schema || table) {
		tablelistquery.append("where ");
		bool	first=true;
		if (catalog) {
			tablelistquery.append(
				"	dbsname like '");
			tablelistquery.append(catalog);
			tablelistquery.append("' ");
			first=false;
		}
		if (schema) {
			if (!first) {
				tablelistquery.append("	and ");
			}
			tablelistquery.append(
				"	owner like '");
			tablelistquery.append(schema);
			tablelistquery.append("' ");
			first=false;
		}
		if (table) {
			if (!first) {
				tablelistquery.append("	and ");
			}
			tablelistquery.append(
				"	tabname like '");
			tablelistquery.append(table);
			tablelistquery.append("' ");
		}
	}

	// order by clause
	tablelistquery.append(
		"order by "
		"	table_cat, "
		"	table_schem, "
		"	table_name");

	return tablelistquery.getString();
}

static const char	*booltype=
			"select "
			"	'BOOLEAN' as type_name, "
			"	-7 as data_type, "
			"	1 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BOOLEAN' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*smallinttype=
			"select "
			"	'SMALLINT' as type_name, "
			"	5 as data_type, "
			"	5 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'SMALLINT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*inttype=
			"select "
			"	'INTEGER' as type_name, "
			"	4 as data_type, "
			"	10 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'INTEGER' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*int8type=
			"select "
			"	'INT8' as type_name, "
			"	-5 as data_type, "
			"	19 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'INT8' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*serialtype=
			"select "
			"	'SERIAL' as type_name, "
			"	4 as data_type, "
			"	10 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	1 as auto_unique_value, "
			"	'SERIAL' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*serial8type=
			"select "
			"	'SERIAL8' as type_name, "
			"	-5 as data_type, "
			"	19 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	1 as auto_unique_value, "
			"	'SERIAL8' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*decimaltype=
			"select "
			"	'DECIMAL' as type_name, "
			"	2 as data_type, "
			"	32 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'DECIMAL' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*moneytype=
			"select "
			"	'MONEY' as type_name, "
			"	2 as data_type, "
			"	32 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'MONEY' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*smallfloattype=
			"select "
			"	'SMALLFLOAT' as type_name, "
			"	7 as data_type, "
			"	7 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'SMALLFLOAT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*floattype=
			"select "
			"	'FLOAT' as type_name, "
			"	8 as data_type, "
			"	15 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'FLOAT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*chartype=
			"select "
			"	'CHAR' as type_name, "
			"	1 as data_type, "
			"	32767 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'CHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*nchartype=
			"select "
			"	'NCHAR' as type_name, "
			"	1 as data_type, "
			"	32767 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'NCHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*varchartype=
			"select "
			"	'VARCHAR' as type_name, "
			"	12 as data_type, "
			"	255 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'VARCHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*nvarchartype=
			"select "
			"	'NVARCHAR' as type_name, "
			"	12 as data_type, "
			"	255 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'NVARCHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*lvarchartype=
			"select "
			"	'LVARCHAR' as type_name, "
			"	12 as data_type, "
			"	32739 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'LVARCHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*texttype=
			"select "
			"	'TEXT' as type_name, "
			"	-1 as data_type, "
			"	2147483647 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'TEXT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*bytetype=
			"select "
			"	'BYTE' as type_name, "
			"	-4 as data_type, "
			"	2147483647 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	0 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BYTE' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*blobtype=
			"select "
			"	'BLOB' as type_name, "
			"	-4 as data_type, "
			"	2147483647 as column_size, "
			"	'' as literal_prefix, "
			"	'' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	0 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BLOB' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*clobtype=
			"select "
			"	'CLOB' as type_name, "
			"	-1 as data_type, "
			"	2147483647 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'CLOB' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*datetype=
			"select "
			"	'DATE' as type_name, "
			"	91 as data_type, "
			"	10 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'DATE' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*datetimetype=
			"select "
			"	'DATETIME' as type_name, "
			"	93 as data_type, "
			"	25 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'DATETIME' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

static const char	*intervaltype=
			"select "
			"	'INTERVAL' as type_name, "
			"	12 as data_type, "
			"	25 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	'' as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'INTERVAL' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	'' as sql_data_type, "
			"	'' as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	'' as interval_precision, "
			"	'' "
			"from "
			"	systables where tabid=1 ";

const char *informixconnection::getTypeInfoListQuery(
						const char *catalog,
						const char *schema,
						const char *type) {

	if (!charstring::compare(type,"*")) {
		if (!typeinfolistquery.getSize()) {
			typeinfolistquery.append(booltype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(smallinttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(inttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(int8type);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(serialtype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(serial8type);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(decimaltype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(moneytype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(smallfloattype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(floattype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(chartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(nchartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(nvarchartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(lvarchartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(texttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(bytetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(blobtype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(clobtype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(datetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(datetimetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(intervaltype);
		}
		return typeinfolistquery.getString();
	} else if (!charstring::compareIgnoringCase(type,"boolean")) {
		return booltype;
	} else if (!charstring::compareIgnoringCase(type,"smallint")) {
		return smallinttype;
	} else if (!charstring::compareIgnoringCase(type,"integer")) {
		return inttype;
	} else if (!charstring::compareIgnoringCase(type,"int")) {
		return inttype;
	} else if (!charstring::compareIgnoringCase(type,"int8")) {
		return int8type;
	} else if (!charstring::compareIgnoringCase(type,"bigint")) {
		return int8type;
	} else if (!charstring::compareIgnoringCase(type,"serial")) {
		return serialtype;
	} else if (!charstring::compareIgnoringCase(type,"serial8")) {
		return serial8type;
	} else if (!charstring::compareIgnoringCase(type,"bigserial")) {
		return serial8type;
	} else if (!charstring::compareIgnoringCase(type,"decimal")) {
		return decimaltype;
	} else if (!charstring::compareIgnoringCase(type,"money")) {
		return moneytype;
	} else if (!charstring::compareIgnoringCase(type,"smallfloat")) {
		return smallfloattype;
	} else if (!charstring::compareIgnoringCase(type,"real")) {
		return smallfloattype;
	} else if (!charstring::compareIgnoringCase(type,"float")) {
		return floattype;
	} else if (!charstring::compareIgnoringCase(type,"double precision")) {
		return floattype;
	} else if (!charstring::compareIgnoringCase(type,"char")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"character")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"nchar")) {
		return nchartype;
	} else if (!charstring::compareIgnoringCase(type,"varchar")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"nvarchar")) {
		return nvarchartype;
	} else if (!charstring::compareIgnoringCase(type,"lvarchar")) {
		return lvarchartype;
	} else if (!charstring::compareIgnoringCase(type,"text")) {
		return texttype;
	} else if (!charstring::compareIgnoringCase(type,"byte")) {
		return bytetype;
	} else if (!charstring::compareIgnoringCase(type,"blob")) {
		return blobtype;
	} else if (!charstring::compareIgnoringCase(type,"clob")) {
		return clobtype;
	} else if (!charstring::compareIgnoringCase(type,"date")) {
		return datetype;
	} else if (!charstring::compareIgnoringCase(type,"datetime")) {
		return datetimetype;
	} else if (!charstring::compareIgnoringCase(type,"interval")) {
		return intervaltype;
	}
	return NULL;
}

const char *informixconnection::getColumnListQuery(const char *catalog,
							const char *schema,
							const char *table,
							const char *column) {

	// The sys* tables only return info for tables in the current database,
	// so we can't use the "catalog" parameter at all.  There aren't any
	// sysmaster tables that return column info.

	columnlistquery.clear();

	// select clause
	columnlistquery.append(
		"select "
		"	'' as table_cat, "
		"	trim(tb.owner) as table_schem, "
		"	trim(tb.tabname) as table_name, "
		"	trim(cl.colname) as column_name, "
		"	cl.coltype as data_type, "
		"	decode(mod(cl.coltype,256), "
		"		41, "
		"		decode(cl.extended_id, "
		"			10,'clob', "
		"			11,'blob', "
		"			'boolean'), "
		"		1,'smallint', "
		"		2,'int', "
		"		52,'bigint', "
		"		17,'int8', "
		"		5,'decimal', "
		"		8,'money', "
		"		4,'smallfloat', "
		"		3,'float', "
		"		0,'char', "
		"		15,'nchar', "
		"		13,'varchar', "
		"		16,'nvarchar', "
		"		40,'lvarchar', "
		"		7,'date', "
		"		10,'datetime', "
		"		12,'text', "
		"		11,'byte', "
		"		'unknown') as type_name, "
		"	decode(mod(cl.coltype,256), "
		"		5,floor(cl.collength/256), "
		"		8,floor(cl.collength/256), "
		"		10,8, "
		"		12,2147483648, "
		"		11,2147483648, "
		"		cl.collength) as column_size, "
		"	decode(mod(cl.coltype,256), "
		"		5,floor(cl.collength/256), "
		"		8,floor(cl.collength/256), "
		"		10,8, "
		"		12,2147483648, "
		"		11,2147483648, "
		"		cl.collength) as buffer_length, "
		"	decode(mod(cl.coltype,256), "
		"		5,mod(cl.collength,256), "
		"		8,mod(cl.collength,256), "
		"		0) as decimal_digits, "
		"	10 as num_prec_radix, "
		"	case "
		"		when (cl.coltype<256) then 1 "
		"		else 0 "
		"	end as nullable, "
		"	case "
		"		when mod(cl.coltype,256) in (6,18) "
		"			then 'auto_increment ' "
		"		else '' "
		"	end as remarks, "
		"	decode(mod(cl.coltype,256), "
		"		41,df.default, "
		"		0,df.default, "
		"		15,df.default, "
		"		13,df.default, "
		"		16,df.default, "
		"		40,df.default, "
		"		substr(df.default,"
		"			charindex(' ',"
		"			df.default)+1)) "
		"		as column_default, "
		"	'' as sql_data_type, "
		"	'' as sql_datetime_sub, "
		"	decode(mod(cl.coltype,256), "
		"		5,floor(cl.collength/256), "
		"		8,floor(cl.collength/256), "
		"		10,8, "
		"		12,2147483648, "
		"		11,2147483648, "
		"		cl.collength) as char_octet_length, "
		"	cl.colno as ordinal_position, "
		"	case "
		"		when (cl.coltype<256) then 'YES' "
		"		else 'NO' "
		"	end as is_nullable, "
		"	decode(mod(cl.coltype,256), "
		"		5,floor(cl.collength/256), "
		"		8,floor(cl.collength/256), "
		"		10,8, "
		"		12,2147483648, "
		"		11,2147483648, "
		"		cl.collength) as numeric_precision, "
		"	case ck.key_priority "
		"		when 1 then 'PRI' "
		"		when 2 then 'UNI' "
		"		when 3 then 'MUL' "
		"		else '' "
		"	end as column_key, "
		"	'' ");

	// from clause
	columnlistquery.append(
		"from "
		"	systables tb, "
		"	syscolumns cl "
		"	left outer join sysdefaults df "
		"		on "
		"		df.tabid=cl.tabid "
		"		and "
		"		df.colno=cl.colno "
		"	left outer join ( "
		"		select "
		"			ip.tabid, "
		"			ip.colno, "
		"			min(case cn.constrtype "
		"				when 'P' then 1 "
		"				when 'U' then 2 "
		"				when 'R' then 3 "
		"			end) as key_priority "
		"		from "
		"			( "
		"			select tabid, part1 as colno, idxname "
		"				from sysindexes where part1>0 "
		"			union all "
		"			select tabid, part2 as colno, idxname "
		"				from sysindexes where part2>0 "
		"			union all "
		"			select tabid, part3 as colno, idxname "
		"				from sysindexes where part3>0 "
		"			union all "
		"			select tabid, part4 as colno, idxname "
		"				from sysindexes where part4>0 "
		"			union all "
		"			select tabid, part5 as colno, idxname "
		"				from sysindexes where part5>0 "
		"			union all "
		"			select tabid, part6 as colno, idxname "
		"				from sysindexes where part6>0 "
		"			union all "
		"			select tabid, part7 as colno, idxname "
		"				from sysindexes where part7>0 "
		"			union all "
		"			select tabid, part8 as colno, idxname "
		"				from sysindexes where part8>0 "
		"			) ip, "
		"			sysindexes ix, "
		"			sysconstraints cn "
		"		where "
		"			ix.idxname=ip.idxname "
		"			and "
		"			ix.tabid=ip.tabid "
		"			and "
		"			cn.idxname=ix.idxname "
		"			and "
		"			cn.tabid=ix.tabid "
		"			and "
		"			cn.constrtype in ('P','U','R') "
		"		group by "
		"			ip.tabid, "
		"			ip.colno "
		"	) ck "
		"		on "
		"		cl.tabid=ck.tabid "
		"		and "
		"		cl.colno=ck.colno ");

	// where clause
	columnlistquery.append(
		"where "
		"	tb.tabid=cl.tabid ");
	if (!charstring::isNullOrEmpty(schema)) {
		columnlistquery.append(
			"	and "
			"	tb.owner like '");
		columnlistquery.append(schema);
		columnlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		columnlistquery.append(
			"	and "
			"	upper(tb.tabname) like upper('");
		columnlistquery.append(table);
		columnlistquery.append("') ");
	}
	if (!charstring::isNullOrEmpty(column)) {
		columnlistquery.append(
			"	and "
			"	cl.colname like '");
		columnlistquery.append(column);
		columnlistquery.append("' ");
	}

	// order by clause
	columnlistquery.append(
		"order by "
		"	ordinal_position");

	return columnlistquery.getString();
}

const char *informixconnection::getPrimaryKeysListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	// The sys* tables only return info for tables in the current database,
	// so we can't use the "catalog" parameter at all.  There aren't any
	// sysmaster tables that return column info.

	primarykeyslistquery.clear();

	// select clause
	primarykeyslistquery.append(
		"select "
		"	'' as table_cat, "
		"	trim(st.owner) as table_schem, "
		"	trim(st.tabname) as table_name, "
		"	trim(sc.colname) as column_name, "
		"	case "
		"		when sc.colno=si.part1 then 1 "
		"		when sc.colno=si.part2 then 2 "
		"		when sc.colno=si.part3 then 3 "
		"		when sc.colno=si.part4 then 4 "
		"		when sc.colno=si.part5 then 5 "
		"		when sc.colno=si.part6 then 6 "
		"		when sc.colno=si.part7 then 7 "
		"		when sc.colno=si.part8 then 8 "
		"		when sc.colno=si.part9 then 9 "
		"		when sc.colno=si.part10 then 10 "
		"		when sc.colno=si.part11 then 11 "
		"		when sc.colno=si.part12 then 12 "
		"		when sc.colno=si.part13 then 13 "
		"		when sc.colno=si.part14 then 14 "
		"		when sc.colno=si.part15 then 15 "
		"		when sc.colno=si.part16 then 16 "
		"	end as key_seq, "
		"	trim(cn.constrname) as pk_name, "
		"	'' ");

	// from clause
	primarykeyslistquery.append(
		"from "
		"	sysconstraints cn, "
		"	sysindexes si, "
		"	systables st, "
		"	syscolumns sc ");

	// where clause
	primarykeyslistquery.append(
		"where "
		"	cn.constrtype='P' "
		"	and "
		"	cn.idxname=si.idxname "
		"	and "
		"	cn.tabid=st.tabid "
		"	and "
		"	cn.tabid=sc.tabid "
		"	and "
		"	(sc.colno=si.part1 "
		"	or sc.colno=si.part2 "
		"	or sc.colno=si.part3 "
		"	or sc.colno=si.part4 "
		"	or sc.colno=si.part5 "
		"	or sc.colno=si.part6 "
		"	or sc.colno=si.part7 "
		"	or sc.colno=si.part8 "
		"	or sc.colno=si.part9 "
		"	or sc.colno=si.part10 "
		"	or sc.colno=si.part11 "
		"	or sc.colno=si.part12 "
		"	or sc.colno=si.part13 "
		"	or sc.colno=si.part14 "
		"	or sc.colno=si.part15 "
		"	or sc.colno=si.part16) ");
	if (!charstring::isNullOrEmpty(schema)) {
		primarykeyslistquery.append(
			"	and "
			"	st.owner like '");
		primarykeyslistquery.append(schema);
		primarykeyslistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		primarykeyslistquery.append(
			"	and "
			"	st.tabname like '");
		primarykeyslistquery.append(table);
		primarykeyslistquery.append("' ");
	}

	// order by clause
	primarykeyslistquery.append(
		"order by "
		"	table_name, "
		"	sc.colno");

	return primarykeyslistquery.getString();
}

const char *informixconnection::getKeyAndIndexListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	// The sys* tables only return info for tables in the current database,
	// so we can't use the "catalog" parameter at all.  There aren't any
	// sysmaster tables that return column info.

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	'' as table_cat, "
		"	trim(st.owner) as table_schem, "
		"	trim(st.tabname) as table_name, "
		"	case "
		"		when si.idxtype='U' then 0 "
		"		else 1 "
		"	end as non_unique, "
		"	'' as index_qualifier, "
		"	trim(si.idxname) as index_name, "
		"	3 as type, "
		"	sc.colno as ordinal_position, "
		"	trim(sc.colname) as column_name, "
		"	case "
		"		when si.part1<0 then 'D' "
		"		else 'A' "
		"	end as asc_or_desc, "
		"	si.levels as cardinality, "
		"	si.leaves as pages, "
		"	'' as filter_condition, "
		"	'' ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	sysindexes si, "
		"	systables st, "
		"	syscolumns sc ");

	// where clause
	keyandindexlistquery.append(
		"where "
		"	si.tabid=st.tabid "
		"	and "
		"	si.tabid=sc.tabid "
		"	and "
		"	(sc.colno=abs(si.part1) "
		"	or sc.colno=abs(si.part2) "
		"	or sc.colno=abs(si.part3) "
		"	or sc.colno=abs(si.part4) "
		"	or sc.colno=abs(si.part5) "
		"	or sc.colno=abs(si.part6) "
		"	or sc.colno=abs(si.part7) "
		"	or sc.colno=abs(si.part8) "
		"	or sc.colno=abs(si.part9) "
		"	or sc.colno=abs(si.part10) "
		"	or sc.colno=abs(si.part11) "
		"	or sc.colno=abs(si.part12) "
		"	or sc.colno=abs(si.part13) "
		"	or sc.colno=abs(si.part14) "
		"	or sc.colno=abs(si.part15) "
		"	or sc.colno=abs(si.part16)) ");
	if (!charstring::isNullOrEmpty(schema)) {
		keyandindexlistquery.append(
			"	and "
			"	st.owner like '");
		keyandindexlistquery.append(schema);
		keyandindexlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		keyandindexlistquery.append(
			"	and "
			"	st.tabname like '");
		keyandindexlistquery.append(table);
		keyandindexlistquery.append("' ");
	}

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	table_name, "
		"	index_name, "
		"	sc.colno");

	return keyandindexlistquery.getString();
}

const char *informixconnection::getProcedureListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {

	// The sys* tables only return info for procedures in the current
	// database, so we can't use the "catalog" parameter at
	// all.  There aren't any sysmaster tables that return
	// procedure info.

	procedurelistquery.clear();

	// select clause
	procedurelistquery.append(
		"select "
		"	'' as procedure_cat, "
		"	trim(owner) as procedure_schem, "
		"	trim(procname) as procedure_name, "
		"	0 as num_input_params, "
		"	0 as num_output_params, "
		"	0 as num_result_sets, "
		"	'' as remarks, "
		"	case isproc "
		"		when 'f' then '2' "
		"		else '1' "
		"	end as procedure_type, "
		"	'' "
		"from "
		"	sysprocedures ");

	// where clause
	if (!charstring::isNullOrEmpty(schema) ||
		!charstring::isNullOrEmpty(procedure)) {

		bool	first=true;
		procedurelistquery.append("where ");
		if (!charstring::isNullOrEmpty(schema)) {
			procedurelistquery.append(
				"owner like '");
			procedurelistquery.append(schema);
			procedurelistquery.append("' ");
			first=false;
		}
		if (!charstring::isNullOrEmpty(procedure)) {
			if (!first) {
				procedurelistquery.append("and ");
			}
			procedurelistquery.append(
				"procname like '");
			procedurelistquery.append(procedure);
			procedurelistquery.append("' ");
		}
	}

	// order by clause
	procedurelistquery.append(
		"order by "
		"	procedure_schem, "
		"	procedure_name");

	return procedurelistquery.getString();
}

const char *informixconnection::getProcedureParameterListQuery(
							const char *catalog,
							const char *schema,
							const char *procedure) {

	// The sys* tables only return info for procedures in the current
	// database, so we can't use the "catalog" parameter at
	// all.  There aren't any sysmaster tables that return
	// procedure info.

	procedureparameterlistquery.clear();

	// select clause
	procedureparameterlistquery.append(
		"select "
		"	'' as procedure_cat, "
		"	trim(sp.owner) as procedure_schem, "
		"	trim(sp.procname) as procedure_name, "
		"	trim(spc.paramname) as column_name, "
		"	spc.paramattr as column_type, "
		"	'' as data_type, "
		"	case mod(spc.paramtype,256) "
		"		when 0 then trim('CHAR') "
		"		when 1 then trim('SMALLINT') "
		"		when 2 then trim('INTEGER') "
		"		when 3 then trim('FLOAT') "
		"		when 4 then trim('SMALLFLOAT') "
		"		when 5 then trim('DECIMAL') "
		"		when 6 then trim('SERIAL') "
		"		when 7 then trim('DATE') "
		"		when 8 then trim('MONEY') "
		"		when 10 then trim('DATETIME') "
		"		when 11 then trim('BYTE') "
		"		when 12 then trim('TEXT') "
		"		when 13 then trim('VARCHAR') "
		"		when 14 then trim('INTERVAL') "
		"		when 15 then trim('NCHAR') "
		"		when 16 then trim('NVARCHAR') "
		"		when 17 then trim('INT8') "
		"		when 18 then trim('SERIAL8') "
		"		when 40 then trim('LVARCHAR') "
		"		when 41 then trim('BOOLEAN') "
		"		when 43 then trim('LVARCHAR') "
		"		when 45 then trim('BOOLEAN') "
		"		when 52 then trim('BIGINT') "
		"		when 53 then trim('BIGSERIAL') "
		"		else trim('UNKNOWN') "
		"	end as type_name, "
		"	spc.paramlen as column_size, "
		"	'' as buffer_length, "
		"	'' as decimal_digits, "
		"	10 as num_prec_radix, "
		"	1 as nullable, "
		"	'' as remarks, "
		"	'' as column_def, "
		"	'' as sql_data_type, "
		"	'' as sql_datetime_sub, "
		"	spc.paramlen as char_octet_length, "
		"	spc.paramid+1 as ordinal_position, "
		"	'YES' as is_nullable, "
		"	'' ");

	// from clause
	procedureparameterlistquery.append(
		"from "
		"	sysproccolumns spc, "
		"	sysprocedures sp ");

	// where clause
	procedureparameterlistquery.append(
		"where "
		"	spc.procid=sp.procid ");
	if (!charstring::isNullOrEmpty(schema)) {
		procedureparameterlistquery.append(
			"	and "
			"	sp.owner like '");
		procedureparameterlistquery.append(schema);
		procedureparameterlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(procedure)) {
		procedureparameterlistquery.append(
			"	and "
			"	sp.procname like '");
		procedureparameterlistquery.append(procedure);
		procedureparameterlistquery.append("' ");
	}

	// order by clause
	procedureparameterlistquery.append(
		"order by "
		"	procedure_name, "
		"	ordinal_position");

	return procedureparameterlistquery.getString();
}

const char *informixconnection::getBindFormat() {
	return "?";
}
const char *informixconnection::selectCatalogQuery() {
	return "database %s";
}

const char *informixconnection::getCurrentCatalogQuery() {
	return "select trim(dbinfo('dbname')) from sysmaster:sysdual";
}

const char *informixconnection::getCurrentSchemaQuery() {
	return "select trim(user) from sysmaster:sysdual";
}

const char *informixconnection::getLastInsertIdQuery() {
	return "select dbinfo('sqlca.sqlerrd1') from sysmaster:sysdual";
	//return "select dbinfo('serial8') from sysmaster:sysdual";
	//return "select dbinfo('bigserial') from sysmaster:sysdual";
}

const char *informixconnection::setIsolationLevelQuery() {
        return "set isolation %s";
}

const char *informixconnection::getIsolationLevelQuery() {
	return "select "
		"	is_level "
		"from "
		"	sysmaster:syssqlcurses "
		"where "
		"	sid=dbinfo('sessionid') "
		"	and "
		"	iscurrent='Y'";
}

const char *informixconnection::mapIsolationLevel(
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
			return "dirty read";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_READ_COMMITTED")) {
			return "committed read";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_REPEATABLE_READ")) {
			return "cursor stability";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
			return "repeatable read";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(
					isolevel,"dirty read")) {
			return "TRANSACTION_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"committed read")) {
			return "TRANSACTION_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"cursor stability")) {
			return "TRANSACTION_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"repeatable read")) {
			return "TRANSACTION_SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_UNCOMMITTED")) {
			return "dirty read";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_COMMITTED")) {
			return "committed read";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_REPEATABLE_READ")) {
			return "cursor stability";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_SERIALIZABLE")) {
			return "repeatable read";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(
					isolevel,"dirty read")) {
			return "SQL_TXN_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"committed read")) {
			return "SQL_TXN_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"cursor stability")) {
			return "SQL_TXN_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"repeatable read")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *informixconnection::getDatabaseFeatures() {

	if (databasefeatures) {
		return databasefeatures;
	}

	databasefeatures=new char *[FEATURE_COUNT];
	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ALTER_DOMAIN_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ALTER_TABLE_OPERATIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ANSI92_SQL_LEVELS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_BATCH_OPERATIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_BATCH_ROW_COUNTS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DATABASE_SEPARATOR]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DATABASE_TERM]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DATABASE_USAGE]=
		charstring::duplicate("");

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
		charstring::duplicate("");

	databasefeatures[FEATURE_DDL_INDEX_OPERATIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DEFAULT_ISOLATION_LEVEL]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DEFAULT_RESULT_SET_HOLDABILITY]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DELETES_ARE_DETECTED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_DOES_MAX_ROW_SIZE_INCLUDE_BLOBS]=
		charstring::duplicate("");

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
		charstring::duplicate("");

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
		charstring::duplicate("");

	databasefeatures[FEATURE_INDEX_KEYWORDS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_INFO_SCHEMA_VIEWS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_INSERT_OPERATIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_INSERTS_ARE_DETECTED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_IS_DATABASE_AT_START]=
		charstring::duplicate("");


	databasefeatures[FEATURE_ISOLATION_LEVELS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		charstring::duplicate("");

	databasefeatures[FEATURE_LOCK_TYPES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_DATABASE_NAME_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_CONNECTIONS]=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_STATEMENTS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_MIXED_CASE_IDENTIFIERS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		charstring::duplicate("");

	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		charstring::duplicate("");

	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		charstring::duplicate("");

	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		charstring::duplicate("");

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
		charstring::duplicate("");

	databasefeatures[FEATURE_OWN_DELETES_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_PREDICATES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		charstring::duplicate("");

	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		charstring::duplicate("");

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SCHEMA_TERM]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUBQUERY_USAGE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_DESCRIBE_PARAMETER]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_TRANSACTIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_NAMED_PARAMETERS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_NON_NULLABLE_COLUMNS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_ORDER_BY_UNRELATED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_SAVEPOINTS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_SELECT_FOR_UPDATE]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TABLE_TERM]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TIME_DATE_LITERALS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_TRANSACTION_DDL_DML]=
		charstring::duplicate("");

	databasefeatures[FEATURE_UNION_CLAUSES]=
		charstring::duplicate("");

	databasefeatures[FEATURE_UPDATES_ARE_DETECTED]=
		charstring::duplicate("");

	databasefeatures[FEATURE_VALUE_EXPRESSIONS]=
		charstring::duplicate("");

	databasefeatures[FEATURE_WHERE_CURRENT_OF_OPERATIONS]=
		charstring::duplicate("");

	return databasefeatures;
}

const char *informixconnection::getNoopQuery() {
        return "noop";
}

informixcursor::informixcursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {
	informixconn=(informixconnection *)conn;
	stmt=0;
	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	lobbindsize=new SQLLEN[maxbindcount];
	indatebind=new SQL_DATE_STRUCT[maxbindcount];
	intsbind=new SQL_TIMESTAMP_STRUCT[maxbindcount];
	outdatebind=new datebind *[maxbindcount];
	outlobbind=new char *[maxbindcount];
	outlobbindlen=new SQLLEN[maxbindcount];
	outisnullptr=new int16_t *[maxbindcount];
	outisnull=new SQLLEN[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		outdatebind[i]=NULL;
		outlobbind[i]=NULL;
		outlobbindlen[i]=0;
		outisnullptr[i]=NULL;
		outisnull[i]=0;
	}
	sqlnulldata=SQL_NULL_DATA;
	bindformaterror=false;
	allocateResultSetBuffers(conn->cont->getMaxColumnCount());
	truevalue=SQL_TRUE;
}

informixcursor::~informixcursor() {
	delete[] lobbindsize;
	delete[] indatebind;
	delete[] intsbind;
	delete[] outdatebind;
	delete[] outlobbind;
	delete[] outlobbindlen;
	delete[] outisnullptr;
	delete[] outisnull;
	deallocateResultSetBuffers();
}

void informixcursor::allocateResultSetBuffers(int32_t columncount) {

	if (!columncount) {
		this->columncount=0;
		field=NULL;
		loblength=NULL;
		indicator=NULL;
		column=NULL;
	} else {
		this->columncount=columncount;
		field=new char *[columncount];
		loblength=new SQLLEN *[columncount];
		indicator=new SQLLEN *[columncount];
		column=new informixcolumn[columncount];
		uint32_t	fetchatonce=getFetchAtOnce();
		int32_t		maxfieldsize=conn->cont->getMaxFieldSize();
		for (int32_t i=0; i<columncount; i++) {
			column[i].name=new char[4096];
			field[i]=new char[fetchatonce*maxfieldsize];
			loblength[i]=new SQLLEN[fetchatonce];
			indicator[i]=new SQLLEN[fetchatonce];
		}
	}
}

void informixcursor::deallocateResultSetBuffers() {
	if (columncount) {
		for (int32_t i=0; i<columncount; i++) {
			delete[] column[i].name;
			delete[] field[i];
			delete[] loblength[i];
			delete[] indicator[i];
		}
		delete[] column;
		delete[] field;
		delete[] loblength;
		delete[] indicator;
		columncount=0;
	}
}

bool informixcursor::open() {

	if (!stmt) {

		// allocate the cursor
		erg=SQLAllocHandle(SQL_HANDLE_STMT,informixconn->dbc,&stmt);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			return false;
		}

		// set the row array size
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_ARRAY_SIZE,
					(SQLPOINTER)getFetchAtOnce(),0);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			return false;
		}

		// enable smart-large-object automation for non-selects
		erg=SQLSetStmtAttr(stmt,SQL_INFX_ATTR_LO_AUTOMATIC,
						(SQLPOINTER)truevalue,0);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			return false;
		}
	}
	return true;
}

bool informixcursor::close() {

	if (stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT,stmt);
		stmt=0;
	}
	return true;
}

bool informixcursor::prepareQuery(const char *query, uint32_t size) {

	bindformaterror=false;

	// FIXME: we shouldn't have to do this, but the tests crash in
	// multiple locations if we don't...
	if (!close() || !open()) {
		return false;
	}

	// initialize column count
	ncols=0;

	// handle noops
	noop=!charstring::compare(query,"noop");
	if (noop) {
		return true;
	}

	// prepare the query
	erg=SQLPrepare(stmt,(SQLCHAR *)query,size);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::inputBind(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	if (*isnull==SQL_NULL_DATA) {
		// the 4th parameter (ValueType) must by
		// SQL_C_BINARY for this to work with blobs
		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_BINARY,
				SQL_CHAR,
				0,
				0,
				(SQLPOINTER)value,
				valuesize,
				&sqlnulldata);
	} else {
		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_CHAR,
				SQL_CHAR,
				// the parameter below must be set
				// for informix, unlike db2 and odbc
				valuesize,
				0,
				(SQLPOINTER)value,
				valuesize,
				NULL);
	}
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::inputBind(const char *variable,
					uint16_t variablesize,
					int64_t *value) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_LONG,
				SQL_INTEGER,
				0,
				0,
				value,
				sizeof(int64_t),
				NULL);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::inputBind(const char *variable,
					uint16_t variablesize,
					double *value,
					uint32_t precision,
					uint32_t scale) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_DOUBLE,
				SQL_DOUBLE,
				precision,
				scale,
				value,
				sizeof(double),
				NULL);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::inputBind(const char *variable,
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

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	bool	validdate=(year>=0 && month>=0 && day>=0);
	bool	validtime=(hour>=0 && minute>=0 && second>=0 && microsecond>=0);

	if (validdate && !validtime) {

		SQL_DATE_STRUCT	*ts=&(indatebind[pos-1]);
		ts->year=year;
		ts->month=month;
		ts->day=day;

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_DATE,
				SQL_DATE,
				0,
				0,
				ts,
				0,
				NULL);

	} else {

		SQL_TIMESTAMP_STRUCT	*ts=&(intsbind[pos-1]);
		ts->year=year;
		ts->month=month;
		ts->day=day;
		ts->hour=hour;
		ts->minute=minute;
		ts->second=second;
		ts->fraction=microsecond*1000;

		erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_TIMESTAMP,
				SQL_TIMESTAMP,
				0,
				0,
				ts,
				0,
				NULL);
	}
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::inputBindBlob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	lobbindsize[pos-1]=valuesize;
	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				SQL_C_BINARY,
				SQL_LONGVARBINARY,
				valuesize,
				0,
				(SQLPOINTER)value,
				valuesize,
				&(lobbindsize[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::inputBindClob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	lobbindsize[pos-1]=valuesize;
	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_INPUT,
				// SQL_C_CHAR works as expected with TEXT
				// columns, but when used with clobs it ends up
				// putting a null terminator at position
				// "valuesize".  With SQL_C_BINARY, it doesn't.
				SQL_C_BINARY,
				SQL_LONGVARCHAR,
				valuesize,
				0,
				(SQLPOINTER)value,
				valuesize,
				&(lobbindsize[pos-1]));
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::outputBind(const char *variable, 
					uint16_t variablesize,
					char *value, 
					uint32_t valuesize, 
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	outdatebind[pos-1]=NULL;
	outisnullptr[pos-1]=isnull;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_CHAR,
				SQL_CHAR,
				0,
				0,
				value,
				valuesize,
				&(outisnull[pos-1])
				);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::outputBind(const char *variable,
					uint16_t variablesize,
					int64_t *value,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	outdatebind[pos-1]=NULL;
	outisnullptr[pos-1]=isnull;

	*value=0;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_LONG,
				SQL_INTEGER,
				0,
				0,
				value,
				sizeof(int64_t),
				&(outisnull[pos-1])
				);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::outputBind(const char *variable,
					uint16_t variablesize,
					double *value,
					uint32_t *precision,
					uint32_t *scale,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	outdatebind[pos-1]=NULL;
	outisnullptr[pos-1]=isnull;

	*value=0.0;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_DOUBLE,
				SQL_DOUBLE,
				0,
				0,
				value,
				sizeof(double),
				&(outisnull[pos-1])
				);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::outputBind(const char *variable,
					uint16_t variablesize,
					int16_t *year,
					int16_t *month,
					int16_t *day,
					int16_t *hour,
					int16_t *minute,
					int16_t *second,
					int32_t *microsecond,
					const char **tz,
					bool *isnegative,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	datebind	*db=new datebind;
	db->year=year;
	db->month=month;
	db->day=day;
	db->hour=hour;
	db->minute=minute;
	db->second=second;
	db->microsecond=microsecond;
	db->tz=tz;

	*isnegative=false;

	outdatebind[pos-1]=db;
	outisnullptr[pos-1]=isnull;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_TIMESTAMP,
				SQL_TIMESTAMP,
				0,
				0,
				&(db->buffer),
				0,
				&(outisnull[pos-1])
				);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::outputBindBlob(const char *variable, 
					uint16_t variablesize,
					uint16_t index,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	outlobbind[index]=new char[informixconn->maxoutbindlobsize];
	outlobbindlen[index]=SQL_NULL_DATA;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_BINARY,
				SQL_LONGVARBINARY,
				informixconn->maxoutbindlobsize,
				0,
				outlobbind[index],
				informixconn->maxoutbindlobsize,
				&outlobbindlen[index]);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::outputBindClob(const char *variable, 
					uint16_t variablesize,
					uint16_t index,
					int16_t *isnull) {

	uint16_t	pos=charstring::convertToInteger(variable+1);
	if (!pos || pos>maxbindcount) {
		bindformaterror=true;
		return false;
	}

	outlobbind[index]=new char[informixconn->maxoutbindlobsize];
	outlobbindlen[index]=SQL_NULL_DATA;

	erg=SQLBindParameter(stmt,
				pos,
				SQL_PARAM_OUTPUT,
				SQL_C_CHAR,
				SQL_LONGVARCHAR,
				informixconn->maxoutbindlobsize,
				0,
				outlobbind[index],
				informixconn->maxoutbindlobsize,
				&outlobbindlen[index]);
	return (erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO);
}

bool informixcursor::getLobOutputBindLength(uint16_t index, uint64_t *length) {
	// FIXME: this code assumes that the lob characters are 1 byte long
	if (outlobbindlen[index]>informixconn->maxoutbindlobsize) {
		outlobbindlen[index]=informixconn->maxoutbindlobsize;
	}
	*length=outlobbindlen[index];
	return true;
}

bool informixcursor::getLobOutputBindSegment(uint16_t index,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {
	// FIXME: this code assumes that the lob characters are 1 byte long
	uint64_t	len=outlobbindlen[index];
	if (offset>len) {
		return false;
	}
	if (offset+charstoread>len) {
		charstoread=charstoread-((offset+charstoread)-len);
	}
	bytestring::copy(buffer,outlobbind[index]+offset,charstoread);
	*charsread=charstoread;
	return true;
}

bool informixcursor::executeQuery(const char *query, uint32_t size) {

	// initialize row counts
	rowgroupindex=0;
	totalinrowgroup=0;
	totalrows=0;

	// handle noops
	if (noop) {
		return true;
	}

	// execute the query
	erg=SQLExecute(stmt);
	if (erg!=SQL_SUCCESS &&
		erg!=SQL_SUCCESS_WITH_INFO &&
		erg!=SQL_NO_DATA) {
		return false;
	}

	checkForTempTable(query,size);

	// get the column count
	erg=SQLNumResultCols(stmt,&ncols);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		return false;
	}

	// allocate buffers and limit column count if necessary
	int32_t	maxcolumncount=conn->cont->getMaxColumnCount();
	if (!maxcolumncount) {
		allocateResultSetBuffers(ncols);
	} else if (ncols>maxcolumncount) {
		ncols=maxcolumncount;
	}

	// run through the columns
	for (SQLSMALLINT i=0; i<ncols; i++) {

		if (conn->cont->getSendColumnInfo()) {

			// column name
			erg=SQLColAttribute(stmt,i+1,SQL_COLUMN_LABEL,
					column[i].name,4096,
					&(column[i].namesize),
					NULL);
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}

			// column size
			// SQL_COLUMN_LENGTH isn't reliable in informix.  It
			// usually returns -1 or 0.  Just copy the result of
			// SQL_COLUMN_PRECISION below...

			// column type
			erg=SQLColAttribute(stmt,i+1,SQL_COLUMN_TYPE,
					NULL,0,NULL,&(column[i].type));
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}

			// informix doesn't support column size,
			// so we'll just use the precision

			// column precision
			erg=SQLColAttribute(stmt,i+1,SQL_COLUMN_PRECISION,
					NULL,0,NULL,&(column[i].precision));
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}

			// column scale
			erg=SQLColAttribute(stmt,i+1,SQL_COLUMN_SCALE,
					NULL,0,NULL,&(column[i].scale));
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}

			// column nullable
			// Informix doesn't support SQL_COLUMN_NULLABLE.
			// Nullability is just part of the "flags".
			erg=SQLColAttribute(stmt,i+1,SQL_INFX_ATTR_FLAGS,
					NULL,0,NULL,&(column[i].flags));
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}

			// primary key

			// unique

			// part of key

			// unsigned number
			erg=SQLColAttribute(stmt,i+1,SQL_COLUMN_UNSIGNED,
				NULL,0,NULL,&(column[i].unsignednumber));
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}

			// zero fill

			// binary

			// autoincrement
			erg=SQLColAttribute(stmt,i+1,
				SQL_COLUMN_AUTO_INCREMENT,
				NULL,0,NULL,&(column[i].autoincrement));
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}

			// table name
			erg=SQLColAttribute(stmt,i+1,
				SQL_COLUMN_TABLE_NAME,
				column[i].table,4096,
				(SQLSMALLINT *)&(column[i].tablesize),
				NULL);
			if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
				return false;
			}
			//column[i].tablesize=
				//charstring::getLength(column[i].table);
		}

		if (column[i].type==SQL_LONGVARBINARY ||
			column[i].type==SQL_INFX_UDT_BLOB) {
			erg=SQLBindCol(stmt,i+1,SQL_C_BINARY,
					field[i],
					conn->cont->getMaxFieldSize(),
					indicator[i]);
		} else {
			erg=SQLBindCol(stmt,i+1,SQL_C_CHAR,
					field[i],
					conn->cont->getMaxFieldSize(),
					indicator[i]);
		}
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			return false;
		}
	}

	// get the row count
	erg=SQLRowCount(stmt,&affectedrows);
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		// This might fail for queries like "database xxx", so we'll
		// tolarate failure and just set affectedrows to 0.  This
		// seems to be ok.
		affectedrows=0;
	}

	// convert date output binds and copy out isnulls
	for (uint16_t i=0; i<getOutputBindCount(); i++) {
		if (outdatebind[i]) {
			datebind	*db=outdatebind[i];
			*(db->year)=db->buffer.year;
			*(db->month)=db->buffer.month;
			*(db->day)=db->buffer.day;
			*(db->hour)=db->buffer.hour;
			*(db->minute)=db->buffer.minute;
			*(db->second)=db->buffer.second;
			*(db->microsecond)=db->buffer.fraction/1000;
			*(db->tz)=NULL;
		}
		if (outisnullptr[i]) {
			*(outisnullptr[i])=outisnull[i];
		}
	}
	
	return true;
}

void informixcursor::getError(char *errorbuffer,
				uint32_t errorbuffersize,
				uint32_t *errorsize,
				int64_t *errorcode,
				bool *liveconnection) {
	if (bindformaterror) {
		// handle bind format errors
		*errorsize=charstring::getLength(
				SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING);
		charstring::safeCopy(errorbuffer,
				errorbuffersize,
				SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING,
				*errorsize);
		*errorcode=SQLR_ERROR_INVALIDBINDVARIABLEFORMAT;
		*liveconnection=true;
		return;
	}

	SQLCHAR		state[10];
	SQLINTEGER	nativeerrnum;
	SQLSMALLINT	errsize;

	SQLGetDiagRec(SQL_HANDLE_STMT,stmt,1,state,&nativeerrnum,
				(SQLCHAR *)errorbuffer,errorbuffersize,
				&errsize);

	// set return values
	*errorsize=errsize;
	// leave it to informix to have negative numbers for error codes...
	// the best we can do for now is turn it into a positive number
	*errorcode=-nativeerrnum;
	*liveconnection=informixconn->liveConnection(nativeerrnum,
							errorbuffer,errsize);
}

uint64_t informixcursor::getAffectedRows() {
	return affectedrows;
}

uint32_t informixcursor::colCount() {
	return ncols;
}

const char *informixcursor::getColumnName(uint32_t i) {
	return column[i].name;
}

uint16_t informixcursor::getColumnNameSize(uint32_t i) {
	return column[i].namesize;
}

uint16_t informixcursor::getColumnType(uint32_t i) {
	switch (column[i].type) {
		case SQL_CHAR:
			// SQL_CHAR is returned for char and nchar
			// FIXME: is there some way to distinguish them?
			return CHAR_DATATYPE;
		case SQL_NUMERIC:
			return NUMERIC_DATATYPE;
		case SQL_DECIMAL:
			// SQL_DECIMAL is returned for decimal and money
			// FIXME: is there some way to distinguish them?
			return DECIMAL_DATATYPE;
		case SQL_INTEGER:
			return INTEGER_DATATYPE;
		case SQL_SMALLINT:
			return SMALLINT_DATATYPE;
		case SQL_FLOAT:
			return FLOAT_DATATYPE;
		case SQL_REAL:
			// SQL_REAL is returned for smallfloat
			return SMALLFLOAT_DATATYPE;
		case SQL_DOUBLE:
			// SQL_DOUBLE is returned for float
			return FLOAT_DATATYPE;
		case SQL_DATETIME:
			// SQL_DATETIME is returned for date
			return DATE_DATATYPE;
		case SQL_VARCHAR:
			return VARCHAR_DATATYPE;
		case SQL_WCHAR:
		case SQL_WVARCHAR:
		case SQL_WLONGVARCHAR:
		#ifdef SQL_DECFLOAT
		case SQL_DECFLOAT:
		#endif
			// I don't think informix actually supports these,
			// but they're defined in infxsql.h
			return UNKNOWN_DATATYPE;
		case SQL_TIME:
			// I don't think informix actually supports this,
			// but it's defined in sqlext.h
			return TIME_DATATYPE;
		case SQL_TIMESTAMP:
			// SQL_TIMESTAMP is returned for datetime
			return DATETIME_DATATYPE;
		case SQL_LONGVARCHAR:
			// SQL_LONGVARCHAR is returned for text
			return TEXT_DATATYPE;
		case SQL_BINARY:
			// I don't think informix actually supports this,
			// but it's defined in sqlext.h
			return BINARY_DATATYPE;
		case SQL_VARBINARY:
			// I don't think informix actually supports this,
			// but it's defined in sqlext.h
			return VARBINARY_DATATYPE;
		case SQL_LONGVARBINARY:
			// SQL_LONGVARBINARY is returned for byte
			return BYTE_DATATYPE;
		case SQL_BIGINT:
			// SQL_BIGINT is returned for int8's
			return INT8_DATATYPE;
		case SQL_TINYINT:
			// I don't think informix actually supports this,
			// but it's defined in sqlext.h
			return TINYINT_DATATYPE;
		case SQL_BIT:
			// SQL_BIT is returned for boolean
			return BOOLEAN_DATATYPE;
		case SQL_INFX_UDT_FIXED:
		case SQL_INFX_UDT_VARYING:
			// not sure what these are...
			return UNKNOWN_DATATYPE;
		case SQL_INFX_UDT_BLOB:
			return BLOB_DATATYPE;
		case SQL_INFX_UDT_CLOB:
			return CLOB_DATATYPE;
		case SQL_INFX_UDT_LVARCHAR:
		case SQL_INFX_RC_ROW:
		case SQL_INFX_RC_COLLECTION:
		case SQL_INFX_RC_LIST:
		case SQL_INFX_RC_SET:
		case SQL_INFX_RC_MULTISET:
		case SQL_INFX_UNSUPPORTED:
		case SQL_INFX_C_SMARTLOB_LOCATOR:
		case SQL_INFX_QUALIFIER:
			// not sure what these are...
			return UNKNOWN_DATATYPE;
		case SQL_INFX_DECIMAL:
			return DECIMAL_DATATYPE;
		case SQL_INFX_BIGINT:
			// SQL_INFX_BIGINT is returned for bigint's
			return BIGINT_DATATYPE;
		default:
			return UNKNOWN_DATATYPE;
	}
}

uint32_t informixcursor::getColumnSize(uint32_t i) {
	// informix doesn't support column size,
	// so we'll just use the precision
	return column[i].precision;
}

uint32_t informixcursor::getColumnPrecision(uint32_t i) {
	return column[i].precision;
}

uint32_t informixcursor::getColumnScale(uint32_t i) {
	return column[i].scale;
}

uint16_t informixcursor::getColumnIsNullable(uint32_t i) {
	return ISNULLABLE(column[i].flags);
}

uint16_t informixcursor::getColumnIsUnsigned(uint32_t i) {
	return column[i].unsignednumber;
}

uint16_t informixcursor::getColumnIsBinary(uint32_t i) {
	uint16_t	type=getColumnType(i);
	return (type==BINARY_DATATYPE ||
		type==LONGVARBINARY_DATATYPE ||
		type==VARBINARY_DATATYPE ||
		type==GRAPHIC_DATATYPE ||
		type==VARGRAPHIC_DATATYPE ||
		type==LONGVARGRAPHIC_DATATYPE ||
		type==BLOB_DATATYPE);
}

uint16_t informixcursor::getColumnIsAutoIncrement(uint32_t i) {
	return column[i].autoincrement;
}

const char *informixcursor::getColumnTable(uint32_t i) {
	return column[i].table;
}

uint16_t informixcursor::getColumnTableSize(uint32_t i) {
	return column[i].tablesize;
}

bool informixcursor::noRowsToReturn() {
	// if there are no columns, then there can't be any rows either
	return (ncols)?false:true;
}

bool informixcursor::skipRow(bool *error) {
	if (fetchRow(error)) {
		rowgroupindex++;
		return true;
	}
	return false;
}

bool informixcursor::fetchRow(bool *error) {

	*error=false;

	if (noop) {
		return false;
	}

	if (rowgroupindex==getFetchAtOnce()) {
		rowgroupindex=0;
	}
	if (rowgroupindex>0 && rowgroupindex==totalinrowgroup) {
		return false;
	}
	if (!rowgroupindex) {

		// SQLFetchScroll should return SQL_SUCCESS or
		// SQL_SUCCESS_WITH_INFO if it successfully fetched a group of
		// rows, otherwise we're at the end of the result and there are
		// no more rows to fetch.
		SQLRETURN	result=SQLFetchScroll(stmt,SQL_FETCH_NEXT,0);
		if (result==SQL_ERROR) {
			*error=true;
			return false;
		}
		if (result!=SQL_SUCCESS && result!=SQL_SUCCESS_WITH_INFO) {
			// there are no more rows to be fetched
			return false;
		}

		// Determine the current rownumber
		SQLGetStmtAttr(stmt,SQL_ATTR_ROW_NUMBER,
				(SQLPOINTER)&rownumber,0,NULL);

		// In the event that there's a bug in SQLFetchScroll and it
		// returns SQL_SUCCESS or SQL_SUCCESS_WITH_INFO even if we were
		// at the end of the result set and there were no more rows to
		// fetch, this will also catch the end of the result set.
		// I think there was a bug like that in DB2 version 7.2.
		if (rownumber==totalrows) {
			return false;
		}
		totalinrowgroup=rownumber-totalrows;
		totalrows=rownumber;
	}
	return true;
}

void informixcursor::getField(uint32_t col,
				const char **fld, uint64_t *fldsize,
				bool *lob, bool *null) {

	// handle NULLs
	if (indicator[col][rowgroupindex]==SQL_NULL_DATA) {
		*null=true;
		return;
	}

	// handle lobs
	if (column[col].type==SQL_INFX_UDT_CLOB ||
		column[col].type==SQL_INFX_UDT_BLOB) {
		*lob=true;
		return;
	}

	// handle normal datatypes
	*fld=&field[col][rowgroupindex*conn->cont->getMaxFieldSize()];
	*fldsize=indicator[col][rowgroupindex];
}

void informixcursor::nextRow() {
	rowgroupindex++;
}

bool informixcursor::getLobFieldLength(uint32_t col, uint64_t *length) {

	// get the length of the lob

	// a valid buffer must be provided, but it's ok to fetch 0 bytes into it
	SQLCHAR	buffer[1];
	erg=SQLGetData(stmt,col+1,SQL_C_BINARY,buffer,0,
					&(loblength[col][rowgroupindex]));
	if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
		return false;
	}

	// copy out the length
	*length=loblength[col][rowgroupindex];

	return true;
}

bool informixcursor::getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {

	// bail if we're attempting to start reading past the end
	if (offset>(uint64_t)loblength[col][rowgroupindex]) {
		return false;
	}

	// prevent attempts to read past the end
	if (offset+charstoread>(uint64_t)loblength[col][rowgroupindex]) {
		charstoread=charstoread-
			((offset+charstoread)-loblength[col][rowgroupindex]);
	}

	// read a lob segment, at most MAX_LOB_CHUNK_SIZE bytes at a time
	uint64_t	totalbytesread=0;
	SQLLEN		bytestoread=0;
	uint64_t	remainingbytestoread=charstoread;
	for (;;) {

		// figure out how many bytes to read this time
		if (remainingbytestoread<MAX_LOB_CHUNK_SIZE) {
			bytestoread=remainingbytestoread;
		} else {
			bytestoread=MAX_LOB_CHUNK_SIZE;
			remainingbytestoread=remainingbytestoread-
						MAX_LOB_CHUNK_SIZE;
		}

		// read the bytes
		SQLLEN	ind=0;
		erg=SQLGetData(stmt,col+1,SQL_C_BINARY,
					buffer+totalbytesread,
					bytestoread,&ind);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			return false;
		}

		// determine how many bytes were read
		uint64_t	bytesread=
			(ind>=bytestoread || ind==SQL_NO_TOTAL)?bytestoread:ind;

		// update total bytes read
		totalbytesread=totalbytesread+bytesread;

		// bail if we're done reading
		if ((SQLUINTEGER)bytesread<bytestoread ||
				totalbytesread==charstoread) {
			break;
		}
	}

	// return number of bytes/chars read
	*charsread=totalbytesread;

	return true;
}

void informixcursor::closeResultSet() {

	// informix doesn't like to close a null stmt
	if (stmt) {
		SQLCloseCursor(stmt);
	}

	for (uint16_t i=0; i<getOutputBindCount(); i++) {
		delete outdatebind[i];
		outdatebind[i]=NULL;
		delete outlobbind[i];
		outlobbind[i]=NULL;
		outlobbindlen[i]=0;
		outisnullptr[i]=NULL;
		outisnull[i]=0;
	}

	if (!conn->cont->getMaxColumnCount()) {
		deallocateResultSetBuffers();
	}

	// NOTE: this is a bit of a kludge.
	//
	// ncols is reset at the beginning of prepareQuery, and other methods,
	// but, since we rely on it to decide whether there are rows to return,
	// it really needs to be reset here.
	//
	// If sqlrservercontroller intercepts the query (eg. if it's a begin,
	// commit, rollback, etc.) then prepareQuery() will never be called,
	// and this won't be reset.  If it was > 0 from the previous query,
	// then a begin (for example) will think that it has rows to return,
	// and the subsequent SQLFetch will fail with a
	// "function sequence error".  We can avoid that by setting ncols=0
	// here, which will cause noRowsToReturn() to return false by default,
	// and avoid the fetch.
	//
	// Arguably, other things should be reset here too (eg. various row
	// counts), but this is the critical one for now, so we'll sort that
	// out later.
	ncols=0;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_informixconnection(
						sqlrservercontroller *cont) {
		return new informixconnection(cont);
	}
}
