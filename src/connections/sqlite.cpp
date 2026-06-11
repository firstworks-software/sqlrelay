// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/regularexpression.h>
#include <rudiments/sys.h>

#include <datatypes.h>
#include <defines.h>
#include <config.h>

extern "C" {
#ifdef SQLITE3
	#include <sqlite3.h>
#else
	#include <sqlite.h>
#endif
}

#ifndef SQLITE3
	#define	sqlite3_open			sqlite_open
	#define	sqlite3_close			sqlite_close
	#define	sqlite3_get_table		sqlite_get_table
	#define	sqlite3_errmsg			sqlite_errmsg
	#define	sqlite3_free_table		sqlite_free_table
	#define	sqlite3_last_insert_rowid	sqlite_last_insert_rowid
	#define sqlite3_free(mem)		sqlite_free((char *)mem)
#endif

#ifndef HAVE_SQLITE3_MALLOC
	#include <stdlib.h>
	#define sqlite3_malloc			malloc
#endif

class SQLRSERVER_DLLSPEC sqliteconnection : public sqlrserverconnection {
	friend class sqlitecursor;
	public:
		sqliteconnection(sqlrservercontroller *cont);
		~sqliteconnection();
	private:
		void		handleConnectString();
		bool		logIn(const char **error, const char **warning);
		sqlrservercursor	*newCursor(uint16_t id);
		void		deleteCursor(sqlrservercursor *curs);
		void		logOut();
		bool		ping();
		const char	*getDbType();
		const char	*getDbVersion();
		const char	*getDbHostName();
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
		#ifdef SQLITE_TRANSACTIONAL
		const char	*getDefaultIsolationLevel();
		bool		setIsolationLevel(const char *isolevel);
		const char	*setIsolationLevelQuery();
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		void		initDatabaseFeatures();
		const char	*beginTransactionQuery();
		#endif
		bool		selectCatalog(const char *catalog);
		char		*getCurrentCatalog();
		char		*getCurrentUser();
		bool		getLastInsertId(uint64_t *id);
		const char	*getNoopQuery();
		#ifdef SQLITE3
		char		*duplicate(const char *str);
		#endif
		#ifndef SQLITE_TRANSACTIONAL
		sqlrtxmodel_t	getNativeTransactionModel();
		bool		commit();
		bool		rollback();
		#endif
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);

		void		clearErrors();

		const char	*getNextvalFormat();

		char		*db;
		int32_t		busytimeoutms;

		#ifdef SQLITE3
		sqlite3	*sqliteptr;
		#else
		sqlite	*sqliteptr;
		#endif
		char	*errmesg;
		int64_t	errcode;

		char	*hostname;

		stringbuffer	tablelistquery;
		stringbuffer	tabletypelistquery;
		stringbuffer	columnlistquery;
		stringbuffer	typeinfolistquery;
		stringbuffer	primarykeyslistquery;
		stringbuffer	keyandindexlistquery;

		char		*maxconnections;
		const char	*databasefeatures[FEATURE_COUNT];
};

class SQLRSERVER_DLLSPEC sqlitecursor : public sqlrservercursor {
	friend class sqliteconnection;
	private:
		sqlitecursor(sqlrserverconnection *conn, uint16_t id);
		~sqlitecursor();

		bool		supportsNativeBinds(const char *query,
							uint32_t size);

		#ifdef HAVE_SQLITE3_STMT
		bool		prepareQuery(const char *query,
						uint32_t size);
		int32_t		getBindVariableIndex(
						const char *variable,
						uint16_t variablesize);
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
		bool		inputBindBlob(const char *variable,
						uint16_t variablesize,
						const char *value,
						uint32_t valuesize,
						int16_t *isnull);
		#endif
		bool		executeQuery(const char *query,
						uint32_t size);
		int		runQuery(const char *query);
		void		selectLastInsertRowId();
		bool		knowsRowCount();
		uint64_t	rowCount();
		#if defined(HAVE_SQLITE3_CHANGES) || \
			defined(HAVE_SQLITE3_CHANGES64)
		uint64_t	getAffectedRows();
		#else
		bool		knowsAffectedRows();
		#endif
		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
		#ifdef HAVE_SQLITE3_STMT
		uint16_t	getColumnType(uint32_t col);
		const char	*getColumnTable(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		#endif
		bool		noRowsToReturn();
		bool		skipRow(bool *error);
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null);
		void		closeResultSet();

		char		**columnnames;
		int		ncolumn;
		int		nrow;
		#if defined(HAVE_SQLITE3_CHANGES)
		int		affectedrows;
		#elif defined(HAVE_SQLITE3_CHANGES)
		int64_t		affectedrows;
		#endif
		bool		lastinsertrowid;

		#ifdef HAVE_SQLITE3_STMT
		char		**columntables;
		int		*columntypes;
		uint16_t	*columnisnullables;
		sqlite3_stmt	*stmt;
		bool		justexecuted;
		char		*lastinsertrowidstr;
		#else
		char		**result;
		int		rowindex;
		#endif

		regularexpression	selectlastinsertrowid;

		sqliteconnection	*sqliteconn;
};


sqliteconnection::sqliteconnection(sqlrservercontroller *cont) :
					sqlrserverconnection(cont) {
	sqliteptr=NULL;
	errmesg=NULL;
	errcode=0;
	hostname=NULL;
	initDatabaseFeatures();
	db=NULL;
	busytimeoutms=0;
}

sqliteconnection::~sqliteconnection() {
	clearErrors();
	delete[] hostname;
	delete[] db;
	delete[] maxconnections;
}

void sqliteconnection::initDatabaseFeatures() {

	maxconnections=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=
		"ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM";

	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		"false";

	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		"true";

	databasefeatures[FEATURE_ALTER_DOMAIN_CLAUSES]=
		"";

	databasefeatures[FEATURE_ALTER_TABLE_OPERATIONS]=
		"ADD_COLUMN,DROP_COLUMN";

	databasefeatures[FEATURE_ANSI92_SQL_LEVELS]=
		"";

	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
		"";

	databasefeatures[FEATURE_BATCH_OPERATIONS]=
		"";

	databasefeatures[FEATURE_BATCH_ROW_COUNTS]=
		"";

	databasefeatures[FEATURE_CATALOG_SEPARATOR]=
		".";

	databasefeatures[FEATURE_CATALOG_TERM]=
		"catalog";

	databasefeatures[FEATURE_CATALOG_USAGE]=
		"";

	databasefeatures[FEATURE_COLLATION_SEQ]=
		"";

	databasefeatures[FEATURE_CREATE_ASSERTION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_CHARACTER_SET_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_COLLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_DOMAIN_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_SCHEMA_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_TABLE_CLAUSES]=
		"CREATE_TABLE,TABLE_CONSTRAINT,CONSTRAINT_NAME_DEFINITION,LOCAL_TEMPORARY,COLUMN_CONSTRAINT,COLUMN_DEFAULT,COLUMN_COLLATION,CONSTRAINT_INITIALLY_DEFERRED,CONSTRAINT_INITIALLY_IMMEDIATE,CONSTRAINT_DEFERRABLE,CONSTRAINT_NON_DEFERRABLE";

	databasefeatures[FEATURE_CREATE_TRANSLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_VIEW_CLAUSES]=
		"CREATE_VIEW";

	databasefeatures[FEATURE_DATA_DEFINITION_TRANSACTION_BEHAVIOR]=
		"";

	databasefeatures[FEATURE_DDL_INDEX_OPERATIONS]=
		"CREATE_INDEX,DROP_INDEX";

	databasefeatures[FEATURE_DEFAULT_RESULT_SET_HOLDABILITY]=
		"CLOSE_CURSORS_AT_COMMIT";

	databasefeatures[FEATURE_DELETES_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_DOES_MAX_ROW_SIZE_INCLUDE_BLOBS]=
		"false";

	databasefeatures[FEATURE_DROP_ASSERTION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_CHARACTER_SET_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_COLLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_DOMAIN_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_SCHEMA_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_TABLE_CLAUSES]=
		"DROP_TABLE";

	databasefeatures[FEATURE_DROP_TRANSLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_VIEW_CLAUSES]=
		"DROP_VIEW";

	databasefeatures[FEATURE_EXTRA_NAME_CHARACTERS]=
		"";

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		"";

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		"";

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		"BASIC";

	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		"MIXED";

	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
		"\"";

	databasefeatures[FEATURE_INDEX_KEYWORDS]=
		"ASC,DESC";

	databasefeatures[FEATURE_INFO_SCHEMA_VIEWS]=
		"";

	databasefeatures[FEATURE_INSERTS_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_INSERT_OPERATIONS]=
		"INSERT_LITERALS,INSERT_SEARCHED";

	databasefeatures[FEATURE_ISOLATION_LEVELS]=
		"READ_UNCOMMITTED,SERIALIZABLE";

	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		"true";

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		"LOCAL_FILES";

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		"false";

	databasefeatures[FEATURE_LOCK_TYPES]=
		"NO_CHANGE";

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CONNECTIONS]=maxconnections;

	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		"0";

	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_STATEMENTS]=
		"0";

	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MIXED_CASE_IDENTIFIERS]=
		"IDENTIFIERS";

	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		"";

	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		"true";

	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		"HIGH,AT_START";

	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		"ABS,ROUND,SIGN";

	databasefeatures[FEATURE_OPEN_CURSORS_ACROSS]=
		"";

	databasefeatures[FEATURE_OPEN_STATEMENTS_ACROSS]=
		"";

	databasefeatures[FEATURE_OTHERS_DELETES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OTHERS_INSERTS_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OTHERS_UPDATES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OUTER_JOINS]=
		"BASIC,FULL,LIMITED";

	databasefeatures[FEATURE_OWN_DELETES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_PREDICATES]=
		"BETWEEN,COMPARISON,EXISTS,IN,ISNOTNULL,ISNULL,LIKE";

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		"";

	// native odbc reports SQL_IC_SENSITIVE, native jdbc reports neither
	// stored- nor mixed-case-supported (sqlite is case-insensitive even
	// for quoted identifiers); matching native jdbc for now
	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		"";

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		"CROSS_JOIN,FULL_OUTER_JOIN,INNER_JOIN,"
			"LEFT_OUTER_JOIN,RIGHT_OUTER_JOIN";

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		"FORWARD_ONLY/READ_ONLY";

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		"CLOSE_CURSORS_AT_COMMIT";

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		"FORWARD_ONLY";

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		"";

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		"ROWID_UNSUPPORTED";

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		"VALUE_EXPRESSION,NULL,ROW_SUBQUERY";

	databasefeatures[FEATURE_SCHEMA_TERM]=
		"schema";

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		"";

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		"READ_ONLY";

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		"\\";

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		"CORE,MINIMUM";

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		"ABORT,ACTION,AFTER,ANALYZE,ATTACH,AUTOINCREMENT,BEFORE,CASCADE,CONFLICT,DATABASE,DEFERRABLE,DEFERRED,DESC,DETACH,EXCLUSIVE,EXPLAIN,FAIL,GLOB,IGNORE,INDEX,INDEXED,INITIALLY,INSTEAD,ISNULL,KEY,LIMIT,NOTNULL,OFFSET,PLAN,PRAGMA,QUERY,RAISE,REGEXP,REINDEX,RENAME,REPLACE,RESTRICT,TEMP,TEMPORARY,TRANSACTION,VACUUM,VIEW,VIRTUAL";

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		"2";

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		"";

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		"CHAR,CONCAT,LCASE,LENGTH,LTRIM,REPLACE,RTRIM,SUBSTRING,UCASE";

	databasefeatures[FEATURE_SUBQUERY_USAGE]=
		"COMPARISONS,EXISTS,INS";

	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_DESCRIBE_PARAMETER]=
		"";

	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_TRANSACTIONS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_NAMED_PARAMETERS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_NON_NULLABLE_COLUMNS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_ORDER_BY_UNRELATED]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_SAVEPOINTS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_SELECT_FOR_UPDATE]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		"true";

	// sqlite has IFNULL, but its native jdbc driver reports no system
	// functions; matching native
	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		"";

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		"BASIC";

	databasefeatures[FEATURE_TABLE_TERM]=
		"table";

	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		"";

	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		"";

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		"DATE,TIME,DATETIME,JULIANDAY,STRFTIME,CURRENT_DATE,CURRENT_TIME,CURRENT_TIMESTAMP";

	databasefeatures[FEATURE_TIME_DATE_LITERALS]=
		"";

	databasefeatures[FEATURE_TRANSACTION_DDL_DML]=
		"DDL_AND_DML";

	databasefeatures[FEATURE_UNION_CLAUSES]=
		"UNION,UNION_ALL";

	databasefeatures[FEATURE_UPDATES_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_VALUE_EXPRESSIONS]=
		"CASE,CAST,COALESCE,NULLIF";

	databasefeatures[FEATURE_WHERE_CURRENT_OF_OPERATIONS]=
		"";

}


void sqliteconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	db=charstring::duplicate(cont->getConnectStringValue("db"));

	// how long to wait for a contended write lock to clear before
	// returning SQLITE_BUSY, default to 5 seconds
	const char	*bt=cont->getConnectStringValue("busytimeoutms");
	if (charstring::isNullOrEmpty(bt)) {
		busytimeoutms=5000;
	} else {
		busytimeoutms=charstring::convertToInteger(bt);
	}

	cont->setFetchAtOnce(1);
	cont->setMaxColumnCount(0);
	cont->setMaxFieldSize(0);
}

bool sqliteconnection::logIn(const char **error, const char **warning) {
#ifdef SQLITE_TRANSACTIONAL
	#ifdef SQLITE3
		if (sqlite3_open(db,&sqliteptr)==SQLITE_OK) {
			// set a busy timeout
			if (busytimeoutms>0) {
				sqlite3_busy_timeout(sqliteptr,busytimeoutms);
			}
			return true;
		}
		errmesg=duplicate(sqlite3_errmsg(sqliteptr));
		errcode=sqlite3_errcode(sqliteptr);
	#else
		if ((sqliteptr=sqlite3_open(db,666,&errmesg))) {
			return true;
		}
	#endif
	if (errmesg) {
		*error=errmesg;
	}
	return false;
#else
	return true;
#endif
}

sqlrservercursor *sqliteconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new sqlitecursor(
					(sqlrserverconnection *)this,id);
}

void sqliteconnection::deleteCursor(sqlrservercursor *curs) {
	delete (sqlitecursor *)curs;
}

void sqliteconnection::logOut() {
	#ifdef SQLITE_TRANSACTIONAL
	if (sqliteptr) {
		sqlite3_close(sqliteptr);
	}
	#endif
}

bool sqliteconnection::ping() {
	return true;
}

const char *sqliteconnection::getDbType() {
	return "sqlite";
}

const char *sqliteconnection::getDbVersion() {
	#ifdef SQLITE_VERSION
	return SQLITE_VERSION;
	#else
	return "unknown";
	#endif
}

const char *sqliteconnection::getDbHostName() {
	if (!hostname) {
		hostname=sys::getHostName();
	}
	return hostname;
}

const char *sqliteconnection::getCatalogListQuery(const char *catalog) {
	// no good way to get a list of catalogs in sqlite
	return "select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null "
		"where "
		"	1=0";
}

const char *sqliteconnection::getSchemaListQuery(const char *catalog,
						const char *schema) {
	return "select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null "
		"where "
		"	1=0";
}

const char *sqliteconnection::getTableTypeListQuery(
					const char *catalog,
					const char *schema,
					const char *tabletypes) {
	tabletypelistquery.clear();

	// select clause
	tabletypelistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	'' as table_name, "
		"	table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	tabletypelistquery.append(
		"from "
		"(select 'GLOBAL TEMPORARY' as table_type "
		"union "
		"select 'SYSTEM TABLE' as table_type "
		"union "
		"select 'TABLE' as table_type "
		"union "
		"select 'VIEW' as table_type) ");

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

const char *sqliteconnection::getTableListQuery(const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes) {

	tablelistquery.clear();

	// select clause
	tablelistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	tbl_name as table_name, "
		"	'TABLE' as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	tablelistquery.append(
		"from "
		"( "
		"select "
		"	tbl_name "
		"from "
		"	sqlite_master "
		"where ");
	tablelistquery.append("	(");
	bool	first=true;
	if (objecttypes&DB_OBJECT_TABLE) {
		tablelistquery.append("	type='table' ");
		first=false;
	}
	if (objecttypes&DB_OBJECT_VIEW) {
		if (!first) {
			tablelistquery.append("	or ");
		}
		tablelistquery.append("	type='view' ");
	}
	tablelistquery.append(") ");
	tablelistquery.append(
		"union all "
		"select "
		"	tbl_name "
		"from "
		"	sqlite_temp_master "
		"where "
		"	(");
	first=true;
	if (objecttypes&DB_OBJECT_TABLE) {
		tablelistquery.append("	type='table' ");
		first=false;
	}
	if (objecttypes&DB_OBJECT_VIEW) {
		if (!first) {
			tablelistquery.append("	or ");
		}
		tablelistquery.append("	type='view' ");
	}
	tablelistquery.append(") ");
	tablelistquery.append(
		") ");

	// where clause
	if (table) {
		tablelistquery.append(
			"where "
			"	tbl_name like '");
		tablelistquery.append(table);
		tablelistquery.append("' ");
	}

	// order by clause
	tablelistquery.append(
		"order by "
		"	tbl_name");

	return tablelistquery.getString();
}

static const char	*integertype=
			"select "
			"	'INTEGER' as type_name, "
			"	4 as data_type, "
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
			"	'INTEGER' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL ";

static const char	*realtype=
			"select "
			"	'REAL' as type_name, "
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
			"	'REAL' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL ";

static const char	*texttype=
			"select "
			"	'TEXT' as type_name, "
			"	-1 as data_type, "
			"	2147483647 as column_size, "
			"	'''' as literal_prefix, "
			"	'''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
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
			"	NULL ";

static const char	*blobtype=
			"select "
			"	'BLOB' as type_name, "
			"	-4 as data_type, "
			"	2147483647 as column_size, "
			"	null as literal_prefix, "
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
			"	NULL ";

static const char	*numerictype=
			"select "
			"	'NUMERIC' as type_name, "
			"	2 as data_type, "
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
			"	'NUMERIC' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL ";

static const char	*booleantype=
			"select "
			"	'BOOLEAN' as type_name, "
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
			"	'BOOLEAN' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL ";

static const char	*datetype=
			"select "
			"	'DATE' as type_name, "
			"	91 as data_type, "
			"	10 as column_size, "
			"	'''' as literal_prefix, "
			"	'''' as literal_suffix, "
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
			"	NULL ";

static const char	*timetype=
			"select "
			"	'TIME' as type_name, "
			"	92 as data_type, "
			"	8 as column_size, "
			"	'''' as literal_prefix, "
			"	'''' as literal_suffix, "
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
			"	NULL ";

static const char	*datetimetype=
			"select "
			"	'DATETIME' as type_name, "
			"	93 as data_type, "
			"	23 as column_size, "
			"	'''' as literal_prefix, "
			"	'''' as literal_suffix, "
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
			"	NULL ";

static const char	*varchartype=
			"select "
			"	'VARCHAR' as type_name, "
			"	12 as data_type, "
			"	2147483647 as column_size, "
			"	'''' as literal_prefix, "
			"	'''' as literal_suffix, "
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
			"	NULL ";

static const char	*chartype=
			"select "
			"	'CHAR' as type_name, "
			"	1 as data_type, "
			"	2147483647 as column_size, "
			"	'''' as literal_prefix, "
			"	'''' as literal_suffix, "
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
			"	NULL ";

static const char	*floattype=
			"select "
			"	'FLOAT' as type_name, "
			"	6 as data_type, "
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
			"	'FLOAT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL ";

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
			"	NULL ";

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
			"	NULL ";

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
			"	NULL ";

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
			"	NULL ";

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
			"	NULL ";

const char *sqliteconnection::getTypeInfoListQuery(const char *catalog,
							const char *schema,
							const char *type) {

	if (!charstring::compare(type,"*")) {
		if (!typeinfolistquery.getSize()) {
			typeinfolistquery.append(integertype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(realtype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(texttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(blobtype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(numerictype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(booleantype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(datetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(timetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(datetimetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(chartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(floattype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(doubletype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(inttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(biginttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(smallinttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(tinyinttype);
		}
		return typeinfolistquery.getString();
	} else if (!charstring::compareIgnoringCase(type,"integer")) {
		return integertype;
	} else if (!charstring::compareIgnoringCase(type,"real")) {
		return realtype;
	} else if (!charstring::compareIgnoringCase(type,"text")) {
		return texttype;
	} else if (!charstring::compareIgnoringCase(type,"blob")) {
		return blobtype;
	} else if (!charstring::compareIgnoringCase(type,"numeric")) {
		return numerictype;
	} else if (!charstring::compareIgnoringCase(type,"boolean")) {
		return booleantype;
	} else if (!charstring::compareIgnoringCase(type,"date")) {
		return datetype;
	} else if (!charstring::compareIgnoringCase(type,"time")) {
		return timetype;
	} else if (!charstring::compareIgnoringCase(type,"datetime")) {
		return datetimetype;
	} else if (!charstring::compareIgnoringCase(type,"varchar")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"char")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"float")) {
		return floattype;
	} else if (!charstring::compareIgnoringCase(type,"double")) {
		return doubletype;
	} else if (!charstring::compareIgnoringCase(type,"int")) {
		return inttype;
	} else if (!charstring::compareIgnoringCase(type,"bigint")) {
		return biginttype;
	} else if (!charstring::compareIgnoringCase(type,"smallint")) {
		return smallinttype;
	} else if (!charstring::compareIgnoringCase(type,"tinyint")) {
		return tinyinttype;
	}
	return NULL;
}

const char *sqliteconnection::getColumnListQuery(const char *catalog,
							const char *schema,
							const char *table,
							const char *column) {

	columnlistquery.clear();

	// select clause
	columnlistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	'")->append(table)->append("' as table_name, "
		"	p.name as column_name, "
		"	null as data_type, "
		"	case instr(p.type,'(') "
		"		when 0 then upper(p.type) "
		"		else upper(substr(p.type,1,"
					"instr(p.type,'(')-1)) "
		"	end as type_name, "
		"	null as column_size, "
		"	null as buffer_length, "
		"	null as decimal_digits, "
		"	10 as num_prec_radix, "
		"	case "
		"		when p.'notnull'=1 then 0 "
		"		else 1 "
		"	end as nullable, "
		"	case "
		"		when p.pk>0 "
		"		and (select count(*) "
		"			from sqlite_master "
		"			where name='")->append(table)->append("' "
		"			and upper(sql) "
		"				like '%AUTOINCREMENT%')>0 "
		"		then 'auto_increment' "
		"		else '' "
		"	end as remarks, "
		"	p.dflt_value as column_default, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	null as char_octet_length, "
		"	null as ordinal_position, "
		"	case "
		"		when p.'notnull'=1 then 'NO' "
		"		else 'YES' "
		"	end as is_nullable, "
		"	null as numeric_precision, "
		"	case "
		"		when p.pk=1 then 'PRI' "
		"		else '' "
		"	end as column_key, "
		"	null ");

	// from clause
	columnlistquery.append(
		"from "
		"	(select "
		"		* "
		"	from "
		"		pragma_table_info('")->
					append(table)->append("')) p ");

	// where clause
	if (!charstring::isNullOrEmpty(column)) {
		columnlistquery.append(
			"where "
			"	upper(p.name) like upper('")->
						append(column)->append("') ");
	}

	// order by clause
	columnlistquery.append(
		"order by "
		"	p.cid");

	return columnlistquery.getString();
}

const char *sqliteconnection::getPrimaryKeysListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	primarykeyslistquery.clear();

	// select clause
	primarykeyslistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	'")->append(table)->append("' as table_name, "
		"	p.name as column_name, "
		"	p.pk as key_seq, "
		"	null as pk_name, "
		"	null ");

	// from clause
	primarykeyslistquery.append(
		"from "
		"	(select "
		"		* "
		"	from "
		"		pragma_table_info('")->
					append(table)->append("')) p ");

	// where clause
	primarykeyslistquery.append(
		"where "
		"	p.pk>0 "
		"order by "
		"	p.pk");

	return primarykeyslistquery.getString();
}

const char *sqliteconnection::getKeyAndIndexListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	'")->append(table)->append("' as table_name, "
		"	case il.'unique' "
		"		when 1 then 0 "
		"		else 1 "
		"	end as non_unique, "
		"	'' as index_qualifier, "
		"	il.name as index_name, "
		"	3 as type, "
		"	ii.seqno+1 as ordinal_position, "
		"	ii.name as column_name, "
		"	case ii.desc "
		"		when 1 then 'D' "
		"		when 0 then 'A' "
		"		else null "
		"	end as asc_or_desc, "
		"	null as cardinality, "
		"	null as pages, "
		"	null as filter_condition, "
		"	null ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	(select "
		"		* "
		"	from "
		"		pragma_index_list('")->
					append(table)->append("')) il, "
		"	pragma_index_xinfo(il.name) ii "
		"where "
		"	ii.key=1 ");

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	il.name, "
		"	ii.seqno");

	return keyandindexlistquery.getString();
}

const char *sqliteconnection::getProcedureListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {
	return "select "
		"	null as procedure_cat, "
		"	'' as procedure_schem, "
		"	'' as procedure_name, "
		"	0 as num_input_params, "
		"	0 as num_output_params, "
		"	0 as num_result_sets, "
		"	'' as remarks, "
		"	'' as procedure_type, "
		"	null "
		"where "
		"	1=0";
}

const char *sqliteconnection::getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {
	return "select "
		"	null as procedure_cat, "
		"	'' as procedure_schem, "
		"	'' as procedure_name, "
		"	'' as column_name, "
		"	0 as column_type, "
		"	'' as data_type, "
		"	'' as type_name, "
		"	0 as column_size, "
		"	null as buffer_length, "
		"	0 as decimal_digits, "
		"	10 as num_prec_radix, "
		"	1 as nullable, "
		"	'' as remarks, "
		"	null as column_def, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	0 as char_octet_length, "
		"	0 as ordinal_position, "
		"	'YES' as is_nullable, "
		"	null "
		"where "
		"	1=0";
}

#ifdef SQLITE_TRANSACTIONAL
const char *sqliteconnection::getDefaultIsolationLevel() {
	// The pragma defaults to 0, which is serializable.  Return the
	// name rather than the pragma value; the JDBC driver expects
	// default isolation levels in this format (it passes through
	// mapIsolationLevel unmapped) and mapIsolationLevel maps it
	// for ODBC.
	return "SERIALIZABLE";
}

bool sqliteconnection::setIsolationLevel(const char *isolevel) {
	// Only allow levels that map to the read_uncommitted pragma.
	// Anything else would be passed to the pragma verbatim, which
	// sqlite would silently treat as false rather than erroring.
	if (charstring::compare(isolevel,"0") &&
			charstring::compare(isolevel,"1")) {
		return false;
	}
	return sqlrserverconnection::setIsolationLevel(isolevel);
}

const char *sqliteconnection::setIsolationLevelQuery() {
	return "pragma read_uncommitted=%s";
}

const char *sqliteconnection::getIsolationLevelQuery() {
	return "pragma read_uncommitted";
}

const char *sqliteconnection::mapIsolationLevel(
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
			return "1";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
			return "0";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(isolevel,"1")) {
			return "TRANSACTION_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"0")) {
			return "TRANSACTION_SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_UNCOMMITTED")) {
			return "1";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_SERIALIZABLE")) {
			return "0";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(isolevel,"1")) {
			return "SQL_TXN_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"0")) {
			return "SQL_TXN_SERIALIZABLE";
		}
		// the default isolation level (see above)
		if (!charstring::compareIgnoringCase(isolevel,
						"SERIALIZABLE")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *sqliteconnection::getDatabaseFeatures() {
	cont->capDatabaseFeatures(databasefeatures);
	return databasefeatures;
}

const char *sqliteconnection::beginTransactionQuery() {
	return "begin transaction";
}
#endif

bool sqliteconnection::selectCatalog(const char *catalog) {

	// keep track of the original db and host
	char	*originaldb=db;

	// reset the db/host
	db=charstring::duplicate(catalog);

	cont->clearError();

	// log out and log back in to the specified database
	logOut();
	const char	*error;
	const char	*warning;
	if (!logIn(&error,&warning)) {

		// Set the error.  We can't get the message from sqlite3_errmsg,
		// because sqliteptr will be NULL.  So, we'll just return the
		// generic SQL Relay error for these kinds of things.
		cont->setError(SQLR_ERROR_DBNOTFOUND_STRING,
				SQLR_ERROR_DBNOTFOUND,true);

		// log back in to the original database, we'll assume that works
		delete[] db;
		db=originaldb;
		logOut();
		logIn(&error,&warning);
		return false;
	}

	// clean up
	delete[] originaldb;
	return true;
}

char *sqliteconnection::getCurrentCatalog() {
	return charstring::duplicate(db);
}

char *sqliteconnection::getCurrentUser() {
	// sqlite has no users; return the user from the connect string
	return charstring::duplicate(cont->getConnectStringValue("user"));
}

bool sqliteconnection::getLastInsertId(uint64_t *id) {
	*id=sqlite3_last_insert_rowid(sqliteptr);
	return true;
}

const char *sqliteconnection::getNoopQuery() {
	return "pragma noop";
}

#ifndef SQLITE_TRANSACTIONAL
sqlrtxmodel_t sqliteconnection::getNativeTransactionModel() {
	return SQLRTXMODEL_NONE;
}

bool sqliteconnection::commit() {
	return true;
}

bool sqliteconnection::rollback() {
	return true;
}
#endif

void sqliteconnection::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {
	// set return values
	*errorsize=charstring::getLength(errmesg);
	charstring::safeCopy(errorbuffer,errorbuffersize,
					errmesg,*errorsize);
	*errorcode=errcode;
	*liveconnection=true;
	if (errmesg &&
		(!charstring::compare(errmesg,"access permission denied",24) ||
		!charstring::compare(errmesg,"not a directory",15))) {
		*liveconnection=false;
	}
}

void sqliteconnection::clearErrors() {
	if (errmesg) {
		#ifdef HAVE_SQLITE3_FREE_WITH_CHAR
			sqlite3_free(errmesg);
		#else
			sqlite3_free((void *)errmesg);
		#endif
		errmesg=NULL;
		errcode=0;
	}
}

const char *sqliteconnection::getNextvalFormat() {
	return "";
}

sqlitecursor::sqlitecursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {

	columnnames=NULL;
	ncolumn=0;
	nrow=0;
	#if defined(HAVE_SQLITE3_CHANGES) || \
		defined(HAVE_SQLITE3_CHANGES64)
	affectedrows=0;
	#endif
	lastinsertrowid=false;
	#ifdef HAVE_SQLITE3_STMT
	columntables=NULL;
	columntypes=NULL;
	columnisnullables=NULL;
	stmt=NULL;
	justexecuted=false;
	lastinsertrowidstr=NULL;
	#else
	rowindex=0;
	result=NULL;
	#endif

	sqliteconn=(sqliteconnection *)conn;

	selectlastinsertrowid.setPattern("^[ 	\r\n]*(select|SELECT)[ 	\r\n]+"
				"(last|LAST)[ 	\r\n]+(insert|INSERT)[ 	\r\n]+"
				"(rowid|ROWID)");
	selectlastinsertrowid.study();
}

sqlitecursor::~sqlitecursor() {

	// clean up old column names
	if (columnnames) {
		for (int i=0; i<ncolumn; i++) {
			delete[] columnnames[i];
		}
		delete[] columnnames;
	}

	#ifdef HAVE_SQLITE3_STMT
	// clean up old column tables
	if (columntables) {
		for (int i=0; i<ncolumn; i++) {
			delete[] columntables[i];
		}
		delete[] columntables;
	}

	// clean up old column types
	if (columntypes) {
		delete[] columntypes;
	}

	// clean up old column nullability
	if (columnisnullables) {
		delete[] columnisnullables;
	}
	#endif

	closeResultSet();
	#ifdef HAVE_SQLITE3_STMT
	sqlite3_finalize(stmt);
	delete[] lastinsertrowidstr;
	#endif
}

bool sqlitecursor::supportsNativeBinds(const char *query, uint32_t size) {
	#ifdef HAVE_SQLITE3_STMT
	return true;
	#else
	return false;
	#endif
}

#ifdef HAVE_SQLITE3_STMT
bool sqlitecursor::prepareQuery(const char *query, uint32_t size) {

	// reinit justexecuted flag
	justexecuted=false;

	// initialize column count
	ncolumn=0;

	// clear any errors
	sqliteconn->clearErrors();

	// don't prepare "select last insert rowid" queries
	if (selectlastinsertrowid.match(query)) {
		return true;
	}

	// completely reset the statement
	sqlite3_finalize(stmt);

	// prepare the query
	// try again if it fails with SQLITE_SCHEMA
	int	res=SQLITE_SCHEMA;
	while (res==SQLITE_SCHEMA) {
		#ifdef HAVE_SQLITE3_PREPARE_V2
			res=sqlite3_prepare_v2
		#else
			res=sqlite3_prepare
		#endif
			(sqliteconn->sqliteptr,query,size,&stmt,NULL);
	}
	if (res==SQLITE_OK) {
		return true;
	}
	sqliteconn->errcode=res;
	sqliteconn->errmesg=sqliteconn->duplicate(
				sqlite3_errmsg(sqliteconn->sqliteptr));
	return false;
}

int32_t sqlitecursor::getBindVariableIndex(const char *variable,
						uint16_t variablesize) {
	if (charstring::isInteger(variable+1,variablesize-1)) {
		return charstring::convertToInteger(variable+1);
	}
	return sqlite3_bind_parameter_index(stmt,variable);
}

bool sqlitecursor::inputBind(const char *variable,
				uint16_t variablesize,
				const char *value,
				uint32_t valuesize,
				int16_t *isnull) {
	if (*isnull==conn->getNullBindValue()) {
		return (sqlite3_bind_text(stmt,
				getBindVariableIndex(variable,variablesize),
				NULL,0,SQLITE_STATIC)==SQLITE_OK);
	} else {
		return (sqlite3_bind_text(stmt,
				getBindVariableIndex(variable,variablesize),
				value,valuesize,SQLITE_STATIC)==SQLITE_OK);
	}
}

bool sqlitecursor::inputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value) {
	return (sqlite3_bind_int64(stmt,
				getBindVariableIndex(variable,variablesize),
				*value)==SQLITE_OK);
}

bool sqlitecursor::inputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t precision,
				uint32_t scale) {
	return (sqlite3_bind_double(stmt,
				getBindVariableIndex(variable,variablesize),
				*value)==SQLITE_OK);
}

bool sqlitecursor::inputBindBlob(const char *variable,
				uint16_t variablesize,
				const char *value,
				uint32_t valuesize,
				int16_t *isnull) {
	return (sqlite3_bind_blob(stmt,
				getBindVariableIndex(variable,variablesize),
				value,valuesize,SQLITE_STATIC)==SQLITE_OK);
}
#endif

bool sqlitecursor::executeQuery(const char *query, uint32_t size) {

	// execute the query
	int	success=0;
#ifdef SQLITE_TRANSACTIONAL
	for (;;) {

		success=runQuery(query);

		// If we get a SQLITE_SCHEMA return value, we should retry
		// the query, once.
		//
		// If we get an SQLITE_ERROR and the error is "no such table:"
		// then we need to workaround a bug/feature.  If you create a
		// table, it's not visible to other sessions until the
		// sqlite_master table is queried.  In this case, a query
		// against the sqlite_master table should result in an
		// SQLITE_SCHEMA return value.
		//
		// For any other return values, jump out of the loop.
		if (success==SQLITE_SCHEMA) {

			// If we're using the statement API then we need to
			// reprepare the statement.  According to the API
			// docs this shouldn't happen with sqlite3_prepare_v2.
			// This appears to be generally true, but with
			// version 3.6.20 it does.
			#if defined(HAVE_SQLITE3_STMT)
				if (!prepareQuery(query,size)) {
					break;
				}
			#endif
			continue;
		} else if (success==SQLITE_ERROR &&
				sqliteconn->errmesg &&
				!charstring::compare(sqliteconn->errmesg,
							"no such table:",14)) {

			closeResultSet();
			// If for some reason, querying sqlite_master doesn't
			// return SQLITE_SCHEMA, rerun the original query and
			// jump out of the loop.
			if (runQuery("select * from sqlite_master")
							!=SQLITE_SCHEMA) {
				closeResultSet();
				success=runQuery(query);
				break;
			}
		} else {
			break;
		}
	}
#else
	// For non-transactional sqlite, the db must be opened and closed
	// before each query or the results of ddl/dml queries are never
	// visible to other sessions.
	if (sqliteconn->sqliteptr) {
		sqlite3_close(sqliteconn->sqliteptr);
	}
	#ifdef SQLITE3
	if (sqlite3_open(sqliteconn->db,&(sqliteconn->sqliteptr))!=SQLITE_OK) {
		sqliteconn->errmesg=
			sqliteconn->duplicate(
				sqlite3_errmsg(sqliteconn->sqliteptr));
		sqliteconn->errcode=
			sqlite3_errcode(sqliteconn->sqliteptr);
		return false;
	}
	// set a busy timeout
	if (sqliteconn->busytimeoutms>0) {
		sqlite3_busy_timeout(sqliteconn->sqliteptr,
					sqliteconn->busytimeoutms);
	}
	#else
	if (!(sqliteconn->sqliteptr=
			sqlite_open(sqliteconn->db,666,
						&sqliteconn->errmesg))) {
		return false;
	}
	#endif
	success=runQuery(query);
#endif

	checkForTempTable(query,size);

	// get the affected row count
	#if defined(HAVE_SQLITE3_CHANGES)
	affectedrows=sqlite3_changes(sqliteconn->sqliteptr);
	#elif defined(HAVE_SQLITE3_CHANGES64)
	affectedrows=sqlite3_changes64(sqliteconn->sqliteptr);
	#endif

	// cache off the columns so they can be returned later if the result
	// set is suspended/resumed
	#ifdef HAVE_SQLITE3_STMT
	columntables=new char *[ncolumn];
	columnnames=new char *[ncolumn];
	columntypes=new int[ncolumn];
	columnisnullables=new uint16_t[ncolumn];
	if (lastinsertrowid) {
		columntables[0]=charstring::duplicate("");
		columnnames[0]=charstring::duplicate("LASTINSERTROWID");
		columntypes[0]=INTEGER_DATATYPE;
		columnisnullables[0]=0;
	} else {
		for (int i=0; i<ncolumn; i++) {

			const char	*coltable="";
			#ifdef HAVE_SQLITE3_COLUMN_TABLE_NAME
			coltable=sqlite3_column_table_name(stmt,i);
			#endif

			columntables[i]=charstring::duplicate(coltable);
			columnnames[i]=charstring::duplicate(
					sqlite3_column_name(stmt,i));
			columntypes[i]=sqlite3_column_type(stmt,i);

			columnisnullables[i]=1;
			#ifdef HAVE_SQLITE3_COLUMN_TABLE_NAME
			const char	*coldb=
					sqlite3_column_database_name(stmt,i);
			const char	*colorigin=
					sqlite3_column_origin_name(stmt,i);
			int		notnull=0;
			if (!charstring::isNullOrEmpty(coltable) &&
				!charstring::isNullOrEmpty(colorigin) &&
				sqlite3_table_column_metadata(
					sqliteconn->sqliteptr,
					coldb,coltable,colorigin,
					NULL,NULL,&notnull,
					NULL,NULL)==SQLITE_OK &&
				notnull) {
				columnisnullables[i]=0;
			}
			#endif
		}
	}
	#else
	columnnames=new char *[ncolumn];
	for (int i=0; i<ncolumn; i++) {
		columnnames[i]=charstring::duplicate(result[i]);
	}
	rowindex=rowindex+ncolumn;
	#endif

	return (success==SQLITE_OK);
}

int sqlitecursor::runQuery(const char *query) {

	// clear any errors
	sqliteconn->clearErrors();

	// clean up old column names
	if (columnnames) {
		for (int i=0; i<ncolumn; i++) {
			delete[] columnnames[i];
		}
		delete[] columnnames;
		columnnames=NULL;
	}

	#ifdef HAVE_SQLITE3_STMT
	// clean up old column tables
	if (columntables) {
		for (int i=0; i<ncolumn; i++) {
			delete[] columntables[i];
		}
		delete[] columntables;
		columntables=NULL;
	}

	// clean up old column types
	if (columntypes) {
		delete[] columntypes;
		columntypes=NULL;
	}

	// clean up old column nullability
	if (columnisnullables) {
		delete[] columnisnullables;
		columnisnullables=NULL;
	}
	#endif

	// reset counters and flags
	nrow=0;
	#if defined(HAVE_SQLITE3_CHANGES) || \
		defined(HAVE_SQLITE3_CHANGES64)
	affectedrows=0;
	#endif
	#ifndef HAVE_SQLITE3_STMT
	ncolumn=0;
	rowindex=0;
	#endif
	lastinsertrowid=false;

	// handle special case of selecting the last row id
	if (selectlastinsertrowid.match(query)) {
		lastinsertrowid=true;
		#ifdef HAVE_SQLITE3_STMT
		justexecuted=true;
		#endif
		selectLastInsertRowId();
		return SQLITE_OK;
	}

#ifdef HAVE_SQLITE3_STMT
	// sqlite3_step executes the query and fetches the first row.  There's
	// no way to just execute the query, to see if there was an error or
	// not, without also fetching the first row.
	int	res=sqlite3_step(stmt);

	// error of some kind
	if (res!=SQLITE_DONE && res!=SQLITE_ROW) {
		sqliteconn->errcode=res;
		#ifndef HAVE_SQLITE3_PREPARE_V2
		// When using sqlite3_step with sqlite3_prepare, if
		// sqlite3_step returns SQLITE_ERROR then you have to call
		// sqlite3_reset or sqlite3_finalize to get the specific error
		// code.  You don't have to do this when using
		// sqlite3_prepare_v2.  In that case, sqlite3_step will return
		// the error code directly.
		if (res==SQLITE_ERROR) {
			sqliteconn->errcode=sqlite3_reset(stmt);
		}
		#endif
		sqliteconn->errmesg=
			sqliteconn->duplicate(
				sqlite3_errmsg(sqliteconn->sqliteptr));

		// if the error code was SQLITE_SCHEMA then return that,
		// otherwise return a generic SQLITE_ERROR
		return (sqliteconn->errcode==SQLITE_SCHEMA)?
					SQLITE_SCHEMA:SQLITE_ERROR;
	}

	// SQLITE_DONE or SQLITE_ROW
	ncolumn=sqlite3_column_count(stmt);
	nrow=(res!=SQLITE_DONE);
	justexecuted=true;
	return SQLITE_OK;
#else
	// run the appropriate query
	int	retval=sqlite3_get_table(sqliteconn->sqliteptr,
					query,
					&result,&nrow,&ncolumn,
					&sqliteconn->errmesg);
	if (retval==SQLITE_ERROR) {
		sqliteconn->errcode=sqlite3_errcode(sqliteconn->sqliteptr);
	}
	return retval;
#endif
}

void sqlitecursor::selectLastInsertRowId() {

	// fake a result set with 1 field
	nrow=1;
	ncolumn=1;
	#ifdef HAVE_SQLITE3_STMT
	lastinsertrowidstr=charstring::parseNumber(
					(int64_t)sqlite3_last_insert_rowid(
							sqliteconn->sqliteptr));
	#else
	result=new char *[2];
	result[0]=charstring::duplicate("LASTINSERTROWID");
	result[1]=charstring::parseNumber((int64_t)sqlite3_last_insert_rowid(
							sqliteconn->sqliteptr));
	#endif
}

bool sqlitecursor::knowsRowCount() {
	#ifdef HAVE_SQLITE3_STMT
	return false;
	#else
	return true;
	#endif
}

uint64_t sqlitecursor::rowCount() {
	#ifdef HAVE_SQLITE3_STMT
	return 0;
	#else
	return nrow;
	#endif
}

#if defined(HAVE_SQLITE3_CHANGES) || \
	defined(HAVE_SQLITE3_CHANGES64)
uint64_t sqlitecursor::getAffectedRows() {
	return affectedrows;
}
#else
bool sqlitecursor::knowsAffectedRows() {
	return false;
}
#endif

uint32_t sqlitecursor::colCount() {
	return ncolumn;
}

const char *sqlitecursor::getColumnName(uint32_t col) {
	return columnnames[col];
}

#ifdef HAVE_SQLITE3_STMT
const char *sqlitecursor::getColumnTable(uint32_t col) {
	return columntables[col];
}

uint16_t sqlitecursor::getColumnIsNullable(uint32_t col) {
	return columnisnullables[col];
}

uint16_t sqlitecursor::getColumnType(uint32_t col) {
	switch (columntypes[col]) {
		case SQLITE_INTEGER:
			return INTEGER_DATATYPE;
		case SQLITE_FLOAT:
			return FLOAT_DATATYPE;
		case SQLITE_TEXT:
			return STRING_DATATYPE;
		case SQLITE_BLOB:
			return BLOB_DATATYPE;
		case SQLITE_NULL:
			return NULL_DATATYPE;
		default:
			return UNKNOWN_DATATYPE;
	}
}
#endif

bool sqlitecursor::noRowsToReturn() {
	return (!nrow);
}

bool sqlitecursor::skipRow(bool *error) {
	#ifdef HAVE_SQLITE3_STMT
	return fetchRow(error);
	#else
	rowindex=rowindex+ncolumn;
	return true;
	#endif
}

bool sqlitecursor::fetchRow(bool *error) {

	*error=false;

	#ifdef HAVE_SQLITE3_STMT
	if (justexecuted) {
		justexecuted=false;
		return true;
	}
	if (lastinsertrowid) {
		return false;
	}
	int	result=sqlite3_step(stmt);
	if (result==SQLITE_ERROR) {
		*error=true;
	}
	return (result==SQLITE_ROW);
	#else
	// have to check for nrow+1 because the
	// first row is actually the column names
	return (rowindex<(ncolumn*(nrow+1)));
	#endif
}

void sqlitecursor::getField(uint32_t col,
				const char **field, uint64_t *fieldsize,
				bool *lob, bool *null) {

#ifdef HAVE_SQLITE3_STMT

	// handle lastinsertrowid specially
	if (lastinsertrowid) {
		*field=lastinsertrowidstr;
		*fieldsize=charstring::getLength(*field);
		*lob=false;
		*null=false;
		return;
	}

	// get the field size
	*fieldsize=sqlite3_column_bytes(stmt,col);

	// get the field data, with special handling for blobs...
	//
	// * if a blob has a value then:
	//   sqlite3_column_type() returns SQLITE_BLOB
	//   sqlite3_column_bytes() returns the size
	//   sqlite3_column_blob() returns that value
	//   we want to return that value and that size
	//   (we can handle this with sqlite3_column_blob)
	// * if a blob is empty then:
	//   sqlite3_column_type() returns SQLITE_BLOB
	//   sqlite3_column_bytes() returns 0
	//   sqlite3_column_blob() returns NULL
	//   we want to return something other than NULL (a "" will do) and 0
	//   (this is the weird case)
	// * if a blob is NULL then:
	//   sqlite3_column_type() returns SQLITE_NULL
	//   sqlite3_column_bytes() returns 0
	//   sqlite3_column_text() returns NULL
	//   we want to return NULL and 0
	//   (we can let sqlite3_column_text handle this)
	//
	// non-blobs are handled as-expected with sqlite3_column_text
	if (sqlite3_column_type(stmt,col)==SQLITE_BLOB) {
		if (*fieldsize) {
			*field=(const char *)sqlite3_column_blob(stmt,col);
		} else {
			*field="";
		}

	} else {
		*field=(const char *)sqlite3_column_text(stmt,col);
	}

	// set the null indicator
	*null=(*field==NULL);

	// set the lob indiciator false, otherwise we'll have to implement
	// methods for fetching the lob in chunks and there's no need to
	// do that, for now at least
	*lob=false;
#else
	// sqlite is kind of strange, the result set is not returned
	// in a 2-d array of pointers to rows/columns, but rather
	// a 1-d array pointing to fields.  You have to manually keep
	// track of which column you're on.
	if (result[rowindex]) {
		*field=result[rowindex];
		*fieldsize=charstring::getLength(result[rowindex]);
	} else {
		*null=true;
	}
	rowindex++;
#endif
}

void sqlitecursor::closeResultSet() {

#ifdef HAVE_SQLITE3_STMT
	if (lastinsertrowidstr) {
		delete[] lastinsertrowidstr;
		lastinsertrowidstr=NULL;
	}
	sqlite3_reset(stmt);
#else
	if (result) {
		if (lastinsertrowid) {
			delete[] result[0];
			delete[] result[1];
			delete[] result;
		} else {
			sqlite3_free_table(result);
		}
		result=NULL;
	}
#endif
}

#ifdef SQLITE3
char *sqliteconnection::duplicate(const char *str) {
	if (!str) {
		return NULL;
	}
	size_t	size=charstring::getLength(str);
	char	*buffer=(char *)sqlite3_malloc(size+1);
	charstring::copy(buffer,str,size);
	buffer[size]='\0';
	return buffer;
}
#endif

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_sqliteconnection(
						sqlrservercontroller *cont) {
		return new sqliteconnection(cont);
	}
}
