// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/bytestring.h>
#ifndef HAVE_POSTGRESQL_PQSETNOTICEPROCESSOR
	#include <rudiments/file.h>
#endif
#include <rudiments/sys.h>

#include <defines.h>
#include <datatypes.h>
#include <config.h>

#include <libpq-fe.h>

class SQLRSERVER_DLLSPEC postgresqlconnection : public sqlrserverconnection {
	friend class postgresqlcursor;
	public:
		postgresqlconnection(sqlrservercontroller *cont);
		~postgresqlconnection();
	private:
		void		initDatabaseFeatures();
		void		handleConnectString();
		bool		logIn(const char **error, const char **warning);
		bool		logIn(const char **error,
					const char **warning,
					const char *database);
		const char	*logInError(const char *errmsg);
		sqlrservercursor	*newCursor(uint16_t id);
		void		deleteCursor(sqlrservercursor *curs);
		void		logOut();
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
		const char	*getDbType();
		const char	*getDbVersion();
		const char	*getDbHostName();
		const char	*getDbIpAddressQuery();
		const char	*getDbIpAddress();
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
		bool		selectCatalog(const char *catalog);
		const char	*getCurrentCatalogQuery();
		const char	*selectSchemaQuery();
		const char	*getCurrentSchemaQuery();
		const char	*getCurrentUserQuery();
		bool		begin();
		bool		setIsolationLevel(const char *isolevel);
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		bool		getLastInsertId(uint64_t *id);
		const char	*getLastInsertIdQuery();
		const char	*getNoopQuery();
		const char	*getBindFormat();
		const char	*getNextvalFormat();

		dictionary< int32_t, char *>	datatypes;
		dictionary< int32_t, char *>	tables;

		PGconn	*pgconn;

		const char	*host;
		const char	*port;
		const char	*options;
		const char	*db;
		const char	*sslmode;
		uint16_t	typemangling;
		uint16_t	tablemangling;
		bool		enablecolumnisnullable;
		const char	*charset;
		char		*dbversion;
		char		*hostname;

#ifdef HAVE_POSTGRESQL_PQCONNECTDB
		stringbuffer	conninfo;
#endif

		stringbuffer	errormessage;

#ifdef HAVE_POSTGRESQL_PQOIDVALUE
		Oid	currentoid;
#endif
		char	*lastinsertidquery;

#ifndef HAVE_POSTGRESQL_PQSETNOTICEPROCESSOR
	private:
		file	devnull;
#endif

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

		char		*maxconnections;
		const char	*databasefeatures[FEATURE_COUNT];
};

class SQLRSERVER_DLLSPEC postgresqlcursor : public sqlrservercursor {
	friend class postgresqlconnection;
	private:
		postgresqlcursor(sqlrserverconnection *conn, uint16_t id);
		~postgresqlcursor();
#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
		bool		prepareQuery(const char *query,
						uint32_t size);
#endif
		bool		supportsNativeBinds(const char *query,
							uint32_t size);
		void		encodeBlob(stringbuffer *buffer,
							const char *data,
							uint32_t datasize);
		void		decodeBlob(char **data,
							uint64_t *datasize);
#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
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
		bool		inputBindClob(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						int16_t *isnull);
#endif
		bool		executeQuery(const char *query,
						uint32_t size);
#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t *errorcode,
						bool *liveconnection);
#endif
		void		getSqlState(char *sqlstatebuffer,
						uint32_t sqlstatebuffersize,
						uint32_t *sqlstatesize);
		bool		knowsRowCount();
		uint64_t	rowCount();
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		uint16_t	columnTypeFormat();
		const char	*getColumnName(uint32_t col);
		uint16_t	getColumnType(uint32_t col);
		const char	*getColumnTypeName(uint32_t col);
		uint32_t	getColumnSize(uint32_t col);
		uint16_t	getColumnIsBinary(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		uint32_t	getColumnScale(uint32_t col);
#ifdef HAVE_POSTGRESQL_PQFTABLE
		const char	*getColumnTable(uint32_t col);
#endif
		bool		noRowsToReturn();
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null);
		void		closeResultSet();

#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
		void		deallocateNamedStatement();
#endif
#if ((defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))) && \
		defined(HAVE_POSTGRESQL_PQDESCRIBEPREPARED)
		bool		columnInfoIsValidAfterPrepare();
#endif

		PGresult	*pgresult;
		ExecStatusType	pgstatus;
		char		sqlstate[6];
		int		ncols;
		int		nrows;
		uint64_t	affectedrows;
		int		currentrow;

		char		**typenamebuffer;
		char		tablenamebuffer[32];

		postgresqlconnection	*postgresqlconn;

#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
		char		*cursorid;
		stringbuffer	deallocatecursorid;
		bool		namedstmtallocated;
		uint16_t	maxbindcount;
		char		**bindvalues;
		int		*bindsizes;
		int		*bindformats;
		int		bindcount;
		int		usedbindcount;

		bool		bindformaterror;
#endif

#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
		bool		justexecuted;
#endif
};

#ifdef HAVE_POSTGRESQL_PQSETNOTICEPROCESSOR
static void nullNoticeProcessor(void *arg, const char *message) {
}
#endif

postgresqlconnection::postgresqlconnection(sqlrservercontroller *cont) :
						sqlrserverconnection(cont) {
	dbversion=NULL;
	initDatabaseFeatures();
	datatypes.setTrackInsertionOrder(false);
	tables.setTrackInsertionOrder(false);
	pgconn=NULL;
#ifdef HAVE_POSTGRESQL_PQOIDVALUE
	currentoid=InvalidOid;
#endif
	lastinsertidquery=NULL;
	hostname=NULL;

	datatypes.setManageArrayValues(true);
	tables.setManageArrayValues(true);
}

postgresqlconnection::~postgresqlconnection() {
#ifndef HAVE_POSTGRESQL_PQSETNOTICEPROCESSOR
	devnull.close();
#endif
	delete[] dbversion;
	delete[] lastinsertidquery;
	delete[] hostname;
	delete[] maxconnections;
}

void postgresqlconnection::initDatabaseFeatures() {

	maxconnections=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=
		"ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM";

	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		"true";

	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		"true";

	databasefeatures[FEATURE_ALTER_DOMAIN_CLAUSES]=
		"ADD_DOMAIN_CONSTRAINT,ADD_DOMAIN_DEFAULT,"
			"CONSTRAINT_NAME_DEFINITION,"
			"DROP_DOMAIN_CONSTRAINT,DROP_DOMAIN_DEFAULT";

	databasefeatures[FEATURE_ALTER_TABLE_OPERATIONS]=
		"ADD_COLUMN,DROP_COLUMN";

	databasefeatures[FEATURE_ANSI92_SQL_LEVELS]=
		"ENTRY_LEVEL";

	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
		"false";

	// the native odbc driver reports batch support, but sqlrelay runs
	// one statement per query, so it reports none
	databasefeatures[FEATURE_BATCH_OPERATIONS]=
		"";

	// none, see batch_operations above
	databasefeatures[FEATURE_BATCH_ROW_COUNTS]=
		"";

	databasefeatures[FEATURE_CATALOG_SEPARATOR]=
		".";

	databasefeatures[FEATURE_CATALOG_TERM]=
		"database";

	databasefeatures[FEATURE_CATALOG_USAGE]=
		"";

	databasefeatures[FEATURE_COLLATION_SEQ]=
		"";

	databasefeatures[FEATURE_CREATE_ASSERTION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_CHARACTER_SET_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_COLLATION_CLAUSES]=
		"CREATE_COLLATION";

	databasefeatures[FEATURE_CREATE_DOMAIN_CLAUSES]=
		"CREATE_DOMAIN,CONSTRAINT_NAME_DEFINITION,"
			"DEFAULT,CONSTRAINT,COLLATION";

	databasefeatures[FEATURE_CREATE_SCHEMA_CLAUSES]=
		"CREATE_SCHEMA,AUTHORIZATION";

	databasefeatures[FEATURE_CREATE_TABLE_CLAUSES]=
		"CREATE_TABLE,TABLE_CONSTRAINT,"
			"CONSTRAINT_NAME_DEFINITION,"
			"COMMIT_DELETE,COMMIT_PRESERVE,"
			"GLOBAL_TEMPORARY,LOCAL_TEMPORARY,"
			"COLUMN_CONSTRAINT,COLUMN_DEFAULT,"
			"COLUMN_COLLATION,"
			"CONSTRAINT_INITIALLY_DEFERRED,"
			"CONSTRAINT_INITIALLY_IMMEDIATE,"
			"CONSTRAINT_DEFERRABLE,"
			"CONSTRAINT_NON_DEFERRABLE";

	databasefeatures[FEATURE_CREATE_TRANSLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_VIEW_CLAUSES]=
		"CREATE_VIEW,CHECK_OPTION,CASCADED,LOCAL";

	databasefeatures[FEATURE_DATA_DEFINITION_TRANSACTION_BEHAVIOR]=
		"";

	databasefeatures[FEATURE_DDL_INDEX_OPERATIONS]=
		"CREATE_INDEX,DROP_INDEX";

	databasefeatures[FEATURE_DEFAULT_RESULT_SET_HOLDABILITY]=
		"HOLD_CURSORS_OVER_COMMIT";

	databasefeatures[FEATURE_DELETES_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_DOES_MAX_ROW_SIZE_INCLUDE_BLOBS]=
		"false";

	databasefeatures[FEATURE_DROP_ASSERTION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_CHARACTER_SET_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_COLLATION_CLAUSES]=
		"DROP_COLLATION";

	databasefeatures[FEATURE_DROP_DOMAIN_CLAUSES]=
		"DROP_DOMAIN,CASCADE,RESTRICT";

	databasefeatures[FEATURE_DROP_SCHEMA_CLAUSES]=
		"DROP_SCHEMA,CASCADE,RESTRICT";

	databasefeatures[FEATURE_DROP_TABLE_CLAUSES]=
		"DROP_TABLE,CASCADE,RESTRICT";

	databasefeatures[FEATURE_DROP_TRANSLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_DROP_VIEW_CLAUSES]=
		"DROP_VIEW,CASCADE,RESTRICT";

	// native odbc reports "_", native jdbc reports "";
	// matching native jdbc for now
	databasefeatures[FEATURE_EXTRA_NAME_CHARACTERS]=
		"";

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		"NEXT,READ_ONLY_CONCURRENCY";

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		"true";

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		"DELETE_TABLE,INSERT_COLUMN,INSERT_TABLE,"
			"REFERENCES_TABLE,REFERENCES_COLUMN,"
			"SELECT_TABLE,UPDATE_COLUMN,UPDATE_TABLE,"
			"USAGE_ON_DOMAIN,USAGE_ON_COLLATION,"
			"WITH_GRANT_OPTION";

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		"BASIC,BEYOND_SELECT,UNRELATED";

	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		"LOWER";

	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
		"\"";

	databasefeatures[FEATURE_INDEX_KEYWORDS]=
		"ASC,DESC";

	// postgresql supports these, but its native odbc driver reports 0;
	// matching native
	databasefeatures[FEATURE_INFO_SCHEMA_VIEWS]=
		"";

	databasefeatures[FEATURE_INSERTS_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_INSERT_OPERATIONS]=
		"INSERT_LITERALS,INSERT_SEARCHED,SELECT_INTO";

	databasefeatures[FEATURE_ISOLATION_LEVELS]=
		"READ_UNCOMMITTED,READ_COMMITTED,"
			"REPEATABLE_READ,SERIALIZABLE";

	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		"true";

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		"";

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		"true";

	databasefeatures[FEATURE_LOCK_TYPES]=
		"NO_CHANGE";

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		"63";

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		"32";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		"1600";

	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		"63";

	databasefeatures[FEATURE_MAX_CONNECTIONS]=maxconnections;

	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		"63";

	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		"63";

	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		"63";

	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		"1073741824";

	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		"63";

	databasefeatures[FEATURE_MAX_STATEMENTS]=
		"0";

	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		"63";

	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		"63";

	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		"false";

	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		"true";

	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		"HIGH";

	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		"abs,acos,asin,atan,atan2,ceiling,cos,cot,"
			"degrees,exp,floor,log,log10,mod,pi,power,"
			"radians,round,sign,sin,sqrt,tan,truncate";

	databasefeatures[FEATURE_OPEN_CURSORS_ACROSS]=
		"";

	databasefeatures[FEATURE_OPEN_STATEMENTS_ACROSS]=
		"COMMIT,ROLLBACK";

	databasefeatures[FEATURE_OTHERS_DELETES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OTHERS_INSERTS_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OTHERS_UPDATES_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OUTER_JOINS]=
		"BASIC,FULL,LIMITED";

	databasefeatures[FEATURE_OWN_DELETES_ARE_VISIBLE]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_PREDICATES]=
		"BETWEEN,COMPARISON,EXISTS,IN,"
			"ISNOTNULL,ISNULL,LIKE,OVERLAPS,"
			"QUANTIFIED_COMPARISON,UNIQUE";

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		"function";

	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		"SENSITIVE";

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		"CROSS_JOIN,EXCEPT_JOIN,FULL_OUTER_JOIN,"
			"INNER_JOIN,INTERSECT_JOIN,LEFT_OUTER_JOIN,"
			"NATURAL_JOIN,RIGHT_OUTER_JOIN,UNION_JOIN";

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		"FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,"
				"SCROLL_INSENSITIVE/READ_ONLY,"
				"SCROLL_INSENSITIVE/UPDATABLE";

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		"HOLD_CURSORS_OVER_COMMIT,CLOSE_CURSORS_AT_COMMIT";

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE";

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		"CASCADE,DELETE_TABLE,GRANT_OPTION_FOR,"
			"INSERT_COLUMN,INSERT_TABLE,"
			"REFERENCES_COLUMN,REFERENCES_TABLE,"
			"RESTRICT,SELECT_TABLE,UPDATE_COLUMN,"
			"UPDATE_TABLE,USAGE_ON_DOMAIN,"
			"USAGE_ON_COLLATION";

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		"ROWID_UNSUPPORTED";

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		"VALUE_EXPRESSION,NULL,DEFAULT,ROW_SUBQUERY";

	databasefeatures[FEATURE_SCHEMA_TERM]=
		"schema";

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		"DATA_MANIPULATION,INDEX_DEFINITIONS,"
			"PRIVILEGE_DEFINITIONS,PROCEDURE_CALLS,"
			"TABLE_DEFINITIONS";

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		"READ_ONLY";

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		"\\";

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		"MINIMUM";

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		"abort,access,aggregate,also,analyse,analyze,"
			"attach,backward,bit,cache,checkpoint,class,"
			"cluster,columns,comment,comments,concurrently,"
			"configuration,conflict,connection,content,"
			"conversion,copy,cost,csv,current_catalog,"
			"current_schema,database,delimiter,delimiters,"
			"depends,detach,dictionary,disable,discard,do,"
			"document,enable,encoding,encrypted,enum,event,"
			"exclusive,explain,extension,family,force,"
			"forward,freeze,functions,generated,greatest,"
			"groups,handler,header,if,ilike,immutable,"
			"implicit,import,include,index,indexes,inherit,"
			"inherits,inline,instead,isnull,label,leakproof,"
			"least,limit,listen,load,location,lock,locked,"
			"logged,mapping,materialized,mode,move,nothing,"
			"notify,notnull,nowait,off,offset,oids,operator,"
			"owned,owner,parallel,parser,passing,password,"
			"plans,policy,prepared,procedural,procedures,"
			"program,publication,quote,reassign,recheck,"
			"refresh,reindex,rename,replace,replica,reset,"
			"restrict,returning,routines,rule,schemas,"
			"sequences,server,setof,share,show,skip,"
			"snapshot,stable,standalone,statistics,stdin,"
			"stdout,storage,stored,strict,strip,"
			"subscription,support,sysid,tables,tablespace,"
			"temp,template,text,truncate,trusted,types,"
			"unencrypted,unlisten,unlogged,until,vacuum,"
			"valid,validate,validator,variadic,verbose,"
			"version,views,volatile,whitespace,wrapper,xml,"
			"xmlattributes,xmlconcat,xmlelement,xmlexists,"
			"xmlforest,xmlnamespaces,xmlparse,xmlpi,xmlroot,"
			"xmlserialize,xmltable,yes";

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		"2";

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		"NEXT,ABSOLUTE,RELATIVE,READ_ONLY_CONCURRENCY";

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		"FUNCTIONS,PROCEDURES";

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		"ascii,char,concat,lcase,left,length,ltrim,"
			"repeat,rtrim,space,substring,ucase,replace";

	databasefeatures[FEATURE_SUBQUERY_USAGE]=
		"COMPARISONS,EXISTS,INS,QUANTIFIEDS";

	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_DESCRIBE_PARAMETER]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_TRANSACTIONS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_NAMED_PARAMETERS]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_NON_NULLABLE_COLUMNS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_ORDER_BY_UNRELATED]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_SAVEPOINTS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_SELECT_FOR_UPDATE]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		"true";

	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		"database,ifnull,user";

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		"BASIC";

	databasefeatures[FEATURE_TABLE_TERM]=
		"table";

	// postgresql also supports QUARTER, but its native odbc driver
	// omits it; matching native
	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		"FRAC_SECOND,SECOND,MINUTE,HOUR,DAY,"
			"WEEK,MONTH,YEAR";

	// postgresql supports more, but its native odbc driver reports only
	// these; matching native
	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		"SECOND,MINUTE,HOUR,DAY";

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		"curdate,curtime,dayname,dayofmonth,dayofweek,"
			"dayofyear,hour,minute,month,monthname,now,"
			"quarter,second,week,year,timestampadd";

	// postgresql supports these, but its native odbc driver doesn't
	// implement this infotype; matching native (reports 0)
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

	// libpq has no call analogous to SQLSetCursorName.  The only way to
	// get a named cursor is DECLARE ... CURSOR FOR <query> plus FETCH,
	// which is a different execution strategy than the PQprepare/
	// PQexecPrepared (and PQsendQueryPrepared/single-row-mode) paths that
	// this module uses, and the cursor's lifetime is tied to the
	// transaction rather than to the statement.
	databasefeatures[FEATURE_SUPPORTS_SET_CURSOR_NAME]=
		"false";

}


void postgresqlconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	host=cont->getConnectStringValue("host");
	port=cont->getConnectStringValue("port");
	options=cont->getConnectStringValue("options");
	db=cont->getConnectStringValue("db");
	sslmode=cont->getConnectStringValue("sslmode");
	const char	*typemang=cont->getConnectStringValue("typemangling");
	if (!typemang || charstring::isNo(typemang)) {
		typemangling=0;
	} else if (charstring::isYes(typemang)) {
		typemangling=1;
	} else {
		typemangling=2;
	}
	const char	*tablemang=cont->getConnectStringValue("tablemangling");
	if (!tablemang || charstring::isNo(tablemang)) {
		tablemangling=0;
	} else {
		tablemangling=2;
	}
	enablecolumnisnullable=charstring::isYes(
			cont->getConnectStringValue("enablecolumnisnullable"));
	charset=cont->getConnectStringValue("charset");
	const char	*lastinsertidfunc=
			cont->getConnectStringValue("lastinsertidfunction");
	if (charstring::isNullOrEmpty(lastinsertidfunc)) {
		lastinsertidfunc="lastval()";
	}
	stringbuffer	liiquery;
	liiquery.append("select ");
	liiquery.append(lastinsertidfunc);
	lastinsertidquery=liiquery.detachString();

	// Re-process the fetchatonce parameter.  In the parent class, it ends
	// up being set to 1 if it was configured to be 0.  However, with
	// postgresql in particular we want it to default to 0 (which we will
	// interpret as meaning "fetch all rows at once", postgresql's default
	// behavior) and to be able to set it to either 0 or 1.  We'll interpret
	// 1 as meaning "single-step through the result set".
	//
	// Why provide this option?
	//
	// PostgreSQL allows you to either fetch all results at once (the
	// default) or single-step through the results.  However, PostgreSQL
	// doesn't implement single-stepping in a very friendly way.  It gets
	// set at the connection level, rather than the statement level, and it
	// causes odd problems if you have multiple cursors running queries
	// that return result sets at the same time (eg. nested selects).
	//
	// Functionally, if you use single-stepping, then you can't run nested
	// selects if you're using a non-zero result set buffer size on the
	// client-side.
	const char	*fao=cont->getConnectStringValue("fetchatonce");
	cont->setFetchAtOnce(fao && charstring::convertToUnsignedInteger(fao));

	cont->setMaxFieldSize(0);
}

bool postgresqlconnection::logIn(const char **error, const char **warning) {
	return logIn(error,warning,db);
}

bool postgresqlconnection::logIn(const char **error,
					const char **warning,
					const char *database) {

	// clear the datatype dictionary
	if (typemangling==2) {
		datatypes.clear();
	}

	// clear the table dictionary
	if (tablemangling==2) {
		tables.clear();
	}

	// log in
#ifdef HAVE_POSTGRESQL_PQCONNECTDB
	conninfo.clear();
	conninfo.append("user=")->append(cont->getLoginUser());
	conninfo.append(" password=")->append(cont->getLoginPassword());
	if (!charstring::isNullOrEmpty(host)) {
		conninfo.append(" host=")->append(host);
	}
	if (!charstring::isNullOrEmpty(port)) {
		conninfo.append(" port=")->append(port);
	}
	if (!charstring::isNullOrEmpty(options)) {
		conninfo.append(" options=")->append(options);
	}
	if (!charstring::isNullOrEmpty(database)) {
		conninfo.append(" dbname=")->append(database);
	}
	// sslmode isn't supported by older versions of postgresql, and
	// including it at all will cause PQconnectdb to fail.  Remove it
	// altogether if it's omitted or disabled.
	if (!charstring::isNullOrEmpty(sslmode) &&
			charstring::compare(sslmode,"disable")) {
		conninfo.append(" sslmode=")->append(sslmode);
	}
	pgconn=PQconnectdb(conninfo.getString());
#else
	pgconn=PQsetdbLogin(host,port,options,NULL,database,
				cont->getLoginUser(),cont->getLoginPassword());
#endif

	// check the status of the login
	if (PQstatus(pgconn)==CONNECTION_BAD) {
		*error=logInError("Log in failed");
		logOut();
		return false;
	}

#ifdef HAVE_POSTGRESQL_PQSETNOTICEPROCESSOR
	// make sure that no messages get sent to the console
	PQsetNoticeProcessor(pgconn,nullNoticeProcessor,NULL);
#else
	if (devnull.open("/dev/null",O_RDONLY)) {
		devnull.duplicate(1);
		devnull.duplicate(2);
	}
#endif

#if defined(HAVE_POSTGRESQL_PQSETCLIENTENCODING)
	if (charstring::getLength(charset)) {
		PQsetClientEncoding(pgconn,charset);
	}
#endif

	// build the datatype dictionary
	if (typemangling==2) {
		PGresult	*result=PQexec(pgconn,
					"select oid,typname from pg_type");
		if (!result) {
			*error=logInError("Get datatypes failed");
			return false;
		}
		for (int i=0; i<PQntuples(result); i++) {
			datatypes.setValue(
				charstring::convertToInteger(PQgetvalue(result,i,0)),
				charstring::duplicate(PQgetvalue(result,i,1)));
		}
		PQclear(result);
	}

	// build the table dictionary
	if (tablemangling==2) {
		PGresult	*result=PQexec(pgconn,
					"select oid,relname from pg_class");
		if (!result) {
			*error=logInError("Get tables failed");
			return false;
		}
		for (int i=0; i<PQntuples(result); i++) {
			tables.setValue(
				charstring::convertToInteger(PQgetvalue(result,i,0)),
				charstring::duplicate(PQgetvalue(result,i,1)));
		}
		PQclear(result);
	}

#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
	// don't use bind variables against older servers
	if (PQprotocolVersion(pgconn)<3) {
		cont->setFakeInputBinds(true);
	}
#endif

	return true;
}

const char *postgresqlconnection::logInError(const char *errmsg) {

	errormessage.clear();
	errormessage.append(errmsg)->append(": ");

	// get the error message from postgresql
	const char	*message=PQerrorMessage(pgconn);
	errormessage.append(message);
	return errormessage.getString();
}

sqlrservercursor *postgresqlconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new
			postgresqlcursor((sqlrserverconnection *)this,id);
}

void postgresqlconnection::deleteCursor(sqlrservercursor *curs) {
	delete (postgresqlcursor *)curs;
}

void postgresqlconnection::logOut() {

#ifndef HAVE_POSTGRESQL_PQSETNOTICEPROCESSOR
	devnull.close();
#endif

	if (pgconn) {
		PQfinish(pgconn);
		pgconn=NULL;
	}

	// clear the datatype dictionary
	if (typemangling==2) {
		datatypes.clear();
	}

	// clear the table dictionary
	if (typemangling==2) {
		tables.clear();
	}
}

void postgresqlconnection::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {
	const char	*errorstring=PQerrorMessage(pgconn);
	*errorsize=charstring::getLength(errorstring);
	if (*errorsize>=errorbuffersize) {
		*errorsize=(errorbuffersize)?errorbuffersize-1:0;
	}
	charstring::safeCopy(errorbuffer,errorbuffersize,
					errorstring,*errorsize);
	if (errorbuffersize) {
		errorbuffer[*errorsize]='\0';
	}
	// PostgreSQL doesn't have an error number per-se.  We'll set it
	// to 1 though, because 0 typically means "no error has occurred"
	// and some apps respond that way if errorcode is set to 0.
	// This ends up being important when using:
	// Oracle dblink -> ODBC -> SQL Relay -> PostgreSQL
	*errorcode=1;
	*liveconnection=(PQstatus(pgconn)==CONNECTION_OK);
}

const char *postgresqlconnection::getDbType() {
	return "postgresql";
}

const char *postgresqlconnection::getDbVersion() {
	delete[] dbversion;
	dbversion=NULL;
#if defined(HAVE_POSTGRESQL_PQSERVERVERSION)
	// PQserverVersion() packs the version into an integer:
	//   >= 10: major*10000 + patch              (120001 -> 12.1)
	//   <  10: major*10000 + minor*100 + patch  (90603  -> 9.6.3)
	// decode it to a dotted version
	int	version=PQserverVersion(pgconn);
	if (version>=100000) {
		charstring::printf(&dbversion,"%d.%d",
					version/10000,version%10000);
	} else {
		charstring::printf(&dbversion,"%d.%d.%d",
					version/10000,(version/100)%100,
					version%100);
	}
#else
#if defined(HAVE_POSTGRESQL_PQPARAMETERSTATUS)
	dbversion=charstring::duplicate(PQparameterStatus(pgconn,
							"server_version"));
#else
	PGresult	*result=PQexec(pgconn,"select version()");
	if (!result) {
		return NULL;
	}

	const char	*versionstring=PQgetvalue(result,0,0);
	char		**list;
	uint64_t	listsize;
	charstring::split(versionstring," ",true,&list,&listsize);
	if (listsize>=2) {
		dbversion=list[1];
		list[1]=NULL;
	}
	for (uint64_t i=0; i<listsize; i++) {
		delete[] list[i];
	}
	delete[] list;

	PQclear(result);
#endif
#endif
	return dbversion;
}

const char *postgresqlconnection::getDbHostName() {
	const char	*dbhostname=sqlrserverconnection::getDbHostName();
	if (charstring::getLength(dbhostname)) {
		return dbhostname;
	}
	if (!hostname) {
		hostname=sys::getHostName();
	}
	return hostname;
}

const char *postgresqlconnection::getDbIpAddressQuery() {
	return "select inet_server_addr()";
}

const char *postgresqlconnection::getDbIpAddress() {
	const char	*ipaddress=sqlrserverconnection::getDbIpAddress();
	return (charstring::getLength(ipaddress))?ipaddress:"127.0.0.1";
}

const char *postgresqlconnection::getCatalogListQuery(const char *catalog) {

	cataloglistquery.clear();

	// select clause
	cataloglistquery.append(
		"select "
		"	datname as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	cataloglistquery.append(
		"from "
		"	pg_database ");

	// where clause
	if (catalog) {
		cataloglistquery.append(
			"where "
			"	datname like '");
		cataloglistquery.append(catalog);
		cataloglistquery.append("' ");
	}

	// order by clause
	cataloglistquery.append(
		"order by "
		"	datname");

	return cataloglistquery.getString();
}

const char *postgresqlconnection::getSchemaListQuery(const char *catalog,
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
	bool	first=true;

	// where clause
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

const char *postgresqlconnection::getTableTypeListQuery(
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
		"	table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	tabletypelistquery.append(
		"from "
		"(select 'TABLE' as table_type "
		"union "
		"select 'VIEW' as table_type) as t ");

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

const char *postgresqlconnection::getTableListQuery(
						const char *catalog,
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
		"		when table_type="
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

static const char	*booltype=
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
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BOOLEAN' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*int2type=
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

static const char	*int4type=
			"select "
			"	'INTEGER' as type_name, "
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
			"	'INTEGER' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*int8type=
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

static const char	*numerictype=
			"select "
			"	'NUMERIC' as type_name, "
			"	2 as data_type, "
			"	1000 as column_size, "
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
			"	NULL "
			" ";

static const char	*float4type=
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

static const char	*float8type=
			"select "
			"	'DOUBLE PRECISION' as type_name, "
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
			"	'DOUBLE PRECISION' as local_type_name, "
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

static const char	*varchartype=
			"select "
			"	'VARCHAR' as type_name, "
			"	12 as data_type, "
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
			"	'VARCHAR' as local_type_name, "
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
			"	'TEXT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*byteatype=
			"select "
			"	'BYTEA' as type_name, "
			"	-3 as data_type, "
			"	255 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'BYTEA' as local_type_name, "
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

static const char	*timestamptype=
			"select "
			"	'TIMESTAMP' as type_name, "
			"	93 as data_type, "
			"	29 as column_size, "
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

static const char	*timestamptztype=
			"select "
			"	'TIMESTAMP WITH TIME ZONE' as type_name, "
			"	93 as data_type, "
			"	35 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'TIMESTAMP WITH TIME ZONE' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*intervaltype=
			"select "
			"	'INTERVAL' as type_name, "
			"	12 as data_type, "
			"	49 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'INTERVAL' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*uuidtype=
			"select "
			"	'UUID' as type_name, "
			"	1 as data_type, "
			"	36 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'UUID' as local_type_name, "
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

static const char	*jsonbtype=
			"select "
			"	'JSONB' as type_name, "
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
			"	'JSONB' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*xmltype=
			"select "
			"	'XML' as type_name, "
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
			"	'XML' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

const char *postgresqlconnection::getTypeInfoListQuery(const char *catalog,
							const char *schema,
							const char *type) {

	if (!charstring::compare(type,"*")) {
		if (!typeinfolistquery.getSize()) {
			typeinfolistquery.append("(");
			typeinfolistquery.append(booltype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(int2type);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(int4type);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(int8type);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(numerictype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(float4type);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(float8type);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(chartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(texttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(byteatype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(datetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(timetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(timestamptype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(timestamptztype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(intervaltype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(uuidtype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(jsontype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(jsonbtype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(xmltype);
			typeinfolistquery.append(")");
		}
		return typeinfolistquery.getString();
	} else if (!charstring::compareIgnoringCase(type,"boolean")) {
		return booltype;
	} else if (!charstring::compareIgnoringCase(type,"bool")) {
		return booltype;
	} else if (!charstring::compareIgnoringCase(type,"smallint")) {
		return int2type;
	} else if (!charstring::compareIgnoringCase(type,"int2")) {
		return int2type;
	} else if (!charstring::compareIgnoringCase(type,"integer")) {
		return int4type;
	} else if (!charstring::compareIgnoringCase(type,"int")) {
		return int4type;
	} else if (!charstring::compareIgnoringCase(type,"int4")) {
		return int4type;
	} else if (!charstring::compareIgnoringCase(type,"bigint")) {
		return int8type;
	} else if (!charstring::compareIgnoringCase(type,"int8")) {
		return int8type;
	} else if (!charstring::compareIgnoringCase(type,"numeric")) {
		return numerictype;
	} else if (!charstring::compareIgnoringCase(type,"decimal")) {
		return numerictype;
	} else if (!charstring::compareIgnoringCase(type,"real")) {
		return float4type;
	} else if (!charstring::compareIgnoringCase(type,"float4")) {
		return float4type;
	} else if (!charstring::compareIgnoringCase(type,"double precision")) {
		return float8type;
	} else if (!charstring::compareIgnoringCase(type,"float8")) {
		return float8type;
	} else if (!charstring::compareIgnoringCase(type,"float")) {
		return float8type;
	} else if (!charstring::compareIgnoringCase(type,"char")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"bpchar")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"character")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"varchar")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"character varying")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"text")) {
		return texttype;
	} else if (!charstring::compareIgnoringCase(type,"bytea")) {
		return byteatype;
	} else if (!charstring::compareIgnoringCase(type,"date")) {
		return datetype;
	} else if (!charstring::compareIgnoringCase(type,"time")) {
		return timetype;
	} else if (!charstring::compareIgnoringCase(type,"timestamp")) {
		return timestamptype;
	} else if (!charstring::compareIgnoringCase(type,"timestamp without time zone")) {
		return timestamptype;
	} else if (!charstring::compareIgnoringCase(type,"timestamp with time zone")) {
		return timestamptztype;
	} else if (!charstring::compareIgnoringCase(type,"timestamptz")) {
		return timestamptztype;
	} else if (!charstring::compareIgnoringCase(type,"interval")) {
		return intervaltype;
	} else if (!charstring::compareIgnoringCase(type,"uuid")) {
		return uuidtype;
	} else if (!charstring::compareIgnoringCase(type,"json")) {
		return jsontype;
	} else if (!charstring::compareIgnoringCase(type,"jsonb")) {
		return jsonbtype;
	} else if (!charstring::compareIgnoringCase(type,"xml")) {
		return xmltype;
	}
	return NULL;
}

const char *postgresqlconnection::getColumnListQuery(const char *catalog,
							const char *schema,
							const char *table,
							const char *column) {

	columnlistquery.clear();
	columnlistquery.append(
		"select "
		"	co.table_catalog as table_cat, "
		"	co.table_schema as table_schem, "
		"	co.table_name as table_name, "
		"	co.column_name, "
		"	null as data_type, "
		"	co.data_type as type_name, "
		"	case "
		"		when co.numeric_scale is null "
		"			then co.character_maximum_length "
		"		else co.numeric_precision "
		"	end as column_size, "
		"	null as buffer_length, "
		"	co.numeric_scale as decimal_digits, "
		"	co.numeric_precision_radix as num_prec_radix, "
		"	case "
		"		when co.is_nullable='NO' then 0 "
		"		when co.is_nullable='YES' then 1 "
		"		else 2 "
		"	end as nullable, "
		"	case "
		"		when co.column_default like 'nextval(%' "
		"			then 'auto_increment' "
		"		else '' "
		"	end as remarks, "
		"	co.column_default, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	co.character_octet_length as char_octet_length, "
		"	co.ordinal_position, "
		"	co.is_nullable, "
		"	co.numeric_precision, "
		"	case ck.key_priority "
		"		when 1 then 'PRI' "
		"		when 2 then 'UNI' "
		"		when 3 then 'MUL' "
		"		else null "
		"	end as column_key, "
		"	case "
		"		when co.column_default like 'nextval(%' "
		"			then 'YES' "
		"		else 'NO' "
		"	end as is_autoincrement, "
		"	null ");
	columnlistquery.append(
		"from "
		"	information_schema.columns co "
		"left outer join ("
		"	select "
		"		ku.table_schema, "
		"		ku.table_name, "
		"		ku.column_name, "
		"		min(case tc.constraint_type "
		"			when 'PRIMARY KEY' then 1 "
		"			when 'UNIQUE' then 2 "
		"			when 'FOREIGN KEY' then 3 "
		"		end) as key_priority "
		"	from "
		"		information_schema.table_constraints tc, "
		"		information_schema.key_column_usage ku "
		"	where "
		"		tc.constraint_name=ku.constraint_name "
		"		and "
		"		tc.table_schema=ku.table_schema "
		"		and "
		"		tc.constraint_type in "
		"			('PRIMARY KEY','UNIQUE','FOREIGN KEY') "
		"	group by "
		"		ku.table_schema, "
		"		ku.table_name, "
		"		ku.column_name "
		") ck "
		"on "
		"	co.table_schema=ck.table_schema "
		"	and "
		"	co.table_name=ck.table_name "
		"	and "
		"	co.column_name=ck.column_name ");
	bool	first=true;
	if (!charstring::isNullOrEmpty(catalog)) {
		columnlistquery.append("where ");
		columnlistquery.append(
			"	co.table_catalog like '");
		columnlistquery.append(catalog);
		columnlistquery.append("' ");
		first=false;
	}
	if (!charstring::isNullOrEmpty(schema)) {
		if (first) {
			columnlistquery.append("where ");
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	co.table_schema like '");
		columnlistquery.append(schema);
		columnlistquery.append("' ");
		first=false;
	}
	if (!charstring::isNullOrEmpty(table)) {
		if (first) {
			columnlistquery.append("where ");
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	co.table_name like '");
		columnlistquery.append(table);
		columnlistquery.append("' ");
		first=false;
	}
	if (!charstring::isNullOrEmpty(column)) {
		if (first) {
			columnlistquery.append("where ");
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	co.column_name like '");
		columnlistquery.append(column);
		columnlistquery.append("' ");
	}
	columnlistquery.append(
		"order by "
		"	co.ordinal_position");

	return columnlistquery.getString();
}

const char *postgresqlconnection::getPrimaryKeysListQuery(
						const char *catalog,
						const char *schema,
						const char *table) {

	primarykeyslistquery.clear();

	// select clause
	primarykeyslistquery.append(
		"select "
		"	tc.table_catalog as table_cat, "
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
			"	tc.table_catalog like '");
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

const char *postgresqlconnection::getKeyAndIndexListQuery(
						const char *catalog,
						const char *schema,
						const char *table) {

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	current_database() as table_cat, "
		"	n.nspname as table_schem, "
		"	t.relname as table_name, "
		"	case "
		"		when ix.indisunique then 'f' "
		"		else 't' "
		"	end as non_unique, "
		"	n.nspname as index_qualifier, "
		"	i.relname as index_name, "
		"	3 as type, "
		"	u.ord as ordinal_position, "
		"	att.attname as column_name, "
		"	case o.option & 1 "
		"		when 1 then 'D' "
		"		else 'A' "
		"	end as asc_or_desc, "
		"	ix.indnatts as cardinality, "
		"	null as pages, "
		"	null as filter_condition, "
		"	null ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	pg_catalog.pg_class t, "
		"	pg_catalog.pg_class i, "
		"	pg_catalog.pg_index ix, "
		"	pg_catalog.pg_namespace n, "
		"	pg_catalog.pg_am a, "
		"	pg_catalog.pg_attribute att, "
		"	lateral unnest(ix.indkey) "
		"		with ordinality as u(attnum, ord) "
		"	left join lateral ("
		"		select unnest(ix.indoption) as option"
		"	) o on true ");

	// where clause
	keyandindexlistquery.append(
		"where "
		"	t.oid=ix.indrelid "
		"	and "
		"	i.oid=ix.indexrelid "
		"	and "
		"	t.relnamespace=n.oid "
		"	and "
		"	i.relam=a.oid "
		"	and "
		"	att.attrelid=t.oid "
		"	and "
		"	att.attnum=u.attnum ");
	if (!charstring::isNullOrEmpty(schema)) {
		keyandindexlistquery.append(
			"	and "
			"	n.nspname like '");
		keyandindexlistquery.append(schema);
		keyandindexlistquery.append("' ");
	} else {
		keyandindexlistquery.append(
			"	and "
			"	n.nspname not in "
			"('pg_catalog','information_schema') ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		keyandindexlistquery.append(
			"	and "
			"	t.relname like '");
		keyandindexlistquery.append(table);
		keyandindexlistquery.append("' ");
	}

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	t.relname, "
		"	i.relname, "
		"	u.ord");

	return keyandindexlistquery.getString();
}

const char *postgresqlconnection::getProcedureListQuery(
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

const char *postgresqlconnection::getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {

	procedureparameterlistquery.clear();

	// select clause
	procedureparameterlistquery.append(
		"select "
		"	p.specific_catalog as procedure_cat, "
		"	p.specific_schema as procedure_schem, "
		"	r.routine_name as procedure_name, "
		"	p.parameter_name as column_name, "
		"	case p.parameter_mode "
		"		when 'IN' then 1 "
		"		when 'INOUT' then 2 "
		"		when 'OUT' then 4 "
		"		else 5 "
		"	end as column_type, "
		"	'' as data_type, "
		"	p.data_type as type_name, "
		"	p.character_maximum_length as column_size, "
		"	null as buffer_length, "
		"	p.numeric_scale as decimal_digits, "
		"	p.numeric_precision_radix as num_prec_radix, "
		"	1 as nullable, "
		"	'' as remarks, "
		"	p.parameter_default as column_def, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	p.character_octet_length as char_octet_length, "
		"	p.ordinal_position, "
		"	'YES' as is_nullable, "
		"	null ");

	// from clause
	procedureparameterlistquery.append(
		"from "
		"	information_schema.parameters p, "
		"	information_schema.routines r ");

	// where clause
	procedureparameterlistquery.append(
		"where "
		"	p.specific_name=r.specific_name "
		"	and "
		"	p.specific_schema=r.specific_schema ");
	if (!charstring::isNullOrEmpty(catalog)) {
		procedureparameterlistquery.append(
			"	and "
			"	p.specific_catalog like '");
		procedureparameterlistquery.append(catalog);
		procedureparameterlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(schema)) {
		procedureparameterlistquery.append(
			"	and "
			"	p.specific_schema like '");
		procedureparameterlistquery.append(schema);
		procedureparameterlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(procedure)) {
		procedureparameterlistquery.append(
			"	and "
			"	r.routine_name like '");
		procedureparameterlistquery.append(procedure);
		procedureparameterlistquery.append("' ");
	}

	// order by clause
	procedureparameterlistquery.append(
		"order by "
		"	r.routine_name, "
		"	p.ordinal_position");

	return procedureparameterlistquery.getString();
}

bool postgresqlconnection::selectCatalog(const char *catalog) {

	cont->clearError();

	// log out and log back in to the specified catalog
	logOut();
	const char	*error=NULL;
	const char	*warning=NULL;
	if (!logIn(&error,&warning,catalog)) {

		// Set the error, but don't use the error that was returned
		// from logIn() because it will have a message prepended to it.
		// Also, we can't get the message from PQgetError, because
		// if PQconnect fails then pgconn will be NULL and
		// PQgetError will just return a message saying that it's
		// NULL.  So, we'll just return the generic SQL Relay error
		// for these kinds of things.
		cont->setError(SQLR_ERROR_DBNOTFOUND_STRING,
				SQLR_ERROR_DBNOTFOUND,true);

		// log back in to the original database, we'll assume that works
		logOut();
		logIn(&error,&warning);
		return false;
	}
	return true;
}

const char *postgresqlconnection::getCurrentCatalogQuery() {
	return "select current_database()";
}

const char *postgresqlconnection::selectSchemaQuery() {
	return "set search_path to %s";
}

const char *postgresqlconnection::getCurrentSchemaQuery() {
	return "select current_schema()";
}

const char *postgresqlconnection::getCurrentUserQuery() {
	return "select current_user";
}

bool postgresqlconnection::begin() {

	// the query that sets the isolation level must be run as the first
	// query in a new transaction, so run "begin" and
	// "set transaction isolation level ..." queries directly through PQexec
	// to avoid running queries that check or deallocate the named
	// statement...

	cont->clearError();

	PGresult	*r=PQexec(pgconn,"begin");
	if (!r) {
		return false;
	}

	bool	retval=PQresultStatus(r)==PGRES_COMMAND_OK;
	PQclear(r);

	if (retval) {
		cont->setNeedsCommitOrRollback(true);
	}
	return retval;
}

bool postgresqlconnection::setIsolationLevel(const char *isolevel) {

	// the query that sets the isolation level must be run as the first
	// query in a new transaction, so run "begin" and
	// "set transaction isolation level ..." queries directly through PQexec
	// to avoid running queries that check or deallocate the named
	// statement...

	if (charstring::isNullOrEmpty(isolevel)) {
		return false;
	}

	cont->clearError();

	stringbuffer	silquery;
	if (cont->getInTransaction()) {
		silquery.append("set transaction isolation level ");
	} else {
		silquery.append("set session characteristics as "
				"transaction isolation level ");
	}
	silquery.append(isolevel);

	PGresult	*r=PQexec(pgconn,silquery.getString());
	if (!r) {
		return false;
	}

	bool	retval=PQresultStatus(r)==PGRES_COMMAND_OK;
	PQclear(r);
	return retval;
}

const char *postgresqlconnection::getIsolationLevelQuery() {
	return "show transaction_isolation";
}

const char *postgresqlconnection::mapIsolationLevel(
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
			return "read uncommitted";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_READ_COMMITTED")) {
			return "read committed";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_REPEATABLE_READ")) {
			return "repeatable read";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
			return "serializable";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(
					isolevel,"read uncommitted")) {
			return "TRANSACTION_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"read committed")) {
			return "TRANSACTION_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"repeatable read")) {
			return "TRANSACTION_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"serializable")) {
			return "TRANSACTION_SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_UNCOMMITTED")) {
			return "read uncommitted";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_COMMITTED")) {
			return "read committed";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_REPEATABLE_READ")) {
			return "repeatable read";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_SERIALIZABLE")) {
			return "serializable";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(
					isolevel,"read uncommitted")) {
			return "SQL_TXN_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"read committed")) {
			return "SQL_TXN_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"repeatable read")) {
			return "SQL_TXN_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
					isolevel,"serializable")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *postgresqlconnection::getDatabaseFeatures() {
	cont->capDatabaseFeatures(databasefeatures);
	return databasefeatures;
}

bool postgresqlconnection::getLastInsertId(uint64_t *id) {
#ifdef HAVE_POSTGRESQL_PQOIDVALUE
	if (lastinsertidquery) {
		return sqlrserverconnection::getLastInsertId(id);
	}
	*id=(currentoid!=InvalidOid)?currentoid:0;
	return true;
#else
	return false;
#endif
}

const char *postgresqlconnection::getLastInsertIdQuery() {
	return lastinsertidquery;
}

const char *postgresqlconnection::getNoopQuery() {
	return "do language plpgsql $$declare dummy int; begin end$$";
}

const char *postgresqlconnection::getBindFormat() {
#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
	return "$1";
#else
	return sqlrserverconnection::getBindFormat();
#endif
}

const char *postgresqlconnection::getNextvalFormat() {
	return "nextval('%s')";
}

postgresqlcursor::postgresqlcursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {
	postgresqlconn=(postgresqlconnection *)conn;
	pgresult=NULL;
	sqlstate[0]='\0';
#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
	charstring::printf(&cursorid,"%s-%d",conn->cont->getConnectionId(),id);
	charstring::replace(cursorid,'-','_');
	deallocatecursorid.append("deallocate ")->append(cursorid);
	namedstmtallocated=false;
	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	bindvalues=new char *[maxbindcount];
	bytestring::zero(bindvalues,maxbindcount*sizeof(char *));
	bindsizes=new int[maxbindcount];
	bindformats=new int[maxbindcount];
	bindcount=0;
	usedbindcount=0;
	bindformaterror=false;
#endif
#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	justexecuted=false;
#endif
	typenamebuffer=new char *[conn->cont->getMaxColumnCount()];
	for (uint32_t i=0; i<conn->cont->getMaxColumnCount(); i++) {
		typenamebuffer[i]=new char[32];
	}
}

postgresqlcursor::~postgresqlcursor() {
#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
	for (uint16_t i=0; i<bindcount; i++) {
		delete[] bindvalues[i];
	}
	delete[] bindvalues;
	delete[] bindsizes;
	delete[] bindformats;
	deallocateNamedStatement();
	delete[] cursorid;
#endif
	for (uint32_t i=0; i<conn->cont->getMaxColumnCount(); i++) {
		delete[] typenamebuffer[i];
	}
	delete[] typenamebuffer;
}

#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
bool postgresqlcursor::prepareQuery(const char *query, uint32_t size) {

	// initialize the column count
	ncols=0;

	// reset bind counter
	usedbindcount=0;

	// reset the bind format error flag
	bindformaterror=false;

	// reset the sqlstate
	sqlstate[0]='\0';

	// deallocate named statement
	deallocateNamedStatement();

	// prepare the query
	pgresult=PQprepare(postgresqlconn->pgconn,cursorid,query,0,NULL);

	// handle some kind of outright failure
	if (!pgresult) {
		return false;
	}

	// handle errors
	bool	result=true;
	pgstatus=PQresultStatus(pgresult);
	if (pgstatus==PGRES_BAD_RESPONSE ||
		pgstatus==PGRES_NONFATAL_ERROR ||
		pgstatus==PGRES_FATAL_ERROR) {
		result=false;
	}

	// mark the named statement allocated
	if (pgstatus==PGRES_COMMAND_OK) {
		namedstmtallocated=true;
	}

#if defined(HAVE_POSTGRESQL_PQDESCRIBEPREPARED)

#ifdef HAVE_POSTGRESQL_PQRESULTERRORFIELD
	// Stash the sqlstate.  The result carrying it is cleared just below,
	// but this function can still return failure afterward, leaving
	// getSqlState() nothing to read it from.
	const char	*state=PQresultErrorField(pgresult,PG_DIAG_SQLSTATE);
	size_t		statelen=charstring::getLength(state);
	if (statelen>sizeof(sqlstate)-1) {
		statelen=sizeof(sqlstate)-1;
	}
	charstring::safeCopy(sqlstate,sizeof(sqlstate),state,statelen);
	sqlstate[statelen]='\0';
#endif

	// clean up
	PQclear(pgresult);
	pgresult=NULL;

	// bail, if necessary
	if (!result) {
		return false;
	}
	
	// describe the query (get column info)
	pgresult=PQdescribePrepared(postgresqlconn->pgconn,cursorid);

	// handle some kind of outright failure
	if (!pgresult) {
		return false;
	}

	// handle errors
	result=true;
	pgstatus=PQresultStatus(pgresult);
	if (pgstatus==PGRES_BAD_RESPONSE ||
		pgstatus==PGRES_NONFATAL_ERROR ||
		pgstatus==PGRES_FATAL_ERROR) {
		result=false;
	}

	// get the col count
	ncols=PQnfields(pgresult);
#endif

	return result;
}

bool postgresqlcursor::inputBind(const char *variable, 
					uint16_t variablesize,
					const char *value, 
					uint32_t valuesize,
					int16_t *isnull) {

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return true;
	}

	if (*isnull==conn->getNullBindValue()) {
		bindvalues[pos]=NULL;
		bindsizes[pos]=0;
	} else {
		bindvalues[pos]=charstring::duplicate(value,valuesize);
		bindsizes[pos]=valuesize;
	}
	bindformats[pos]=0;
	bindcount++;
	usedbindcount++;
	return true;
}

bool postgresqlcursor::inputBind(const char *variable, 
					uint16_t variablesize,
					int64_t *value) {

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return true;
	}

	bindvalues[pos]=charstring::parseNumber(*value);
	bindsizes[pos]=charstring::getLength(bindvalues[pos]);
	bindformats[pos]=0;
	bindcount++;
	usedbindcount++;
	return true;
}

bool postgresqlcursor::inputBind(const char *variable, 
					uint16_t variablesize,
					double *value,
					uint32_t precision,
					uint32_t scale) {

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return true;
	}

	// convert the value to a string
	if (!precision && !scale) {
		bindvalues[pos]=charstring::parseNumber(*value);
	} else {
		bindvalues[pos]=charstring::parseNumber(*value,
							precision,scale);
	}
	bindsizes[pos]=charstring::getLength(bindvalues[pos]);
	bindformats[pos]=0;
	bindcount++;
	usedbindcount++;
	return true;
}

bool postgresqlcursor::inputBindBlob(const char *variable, 
					uint16_t variablesize,
					const char *value, 
					uint32_t valuesize,
					int16_t *isnull) {

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return true;
	}

	if (*isnull==conn->getNullBindValue()) {
		bindvalues[pos]=NULL;
		bindsizes[pos]=0;
	} else {
		// It's tricky to bind an "empty blob" in postgresql.
		// * if we bytesstring::duplicate(value,0), then it returns
		//   NULL because it can't create a byte_t array of length 0
		// * if bindvalues[pos] is NULL, then postgresql will bind a
		//   NULL
		// * postgresql doesn't have a null indicator that we can use
		//   to tell it that, even though we passed in a
		//   bindvalues[pos] of NULL, and a bindsizes[pos] of 0, we
		//   really mean "empty blob", not NULL
		// * so, if bindsizes[pos] is 0, then bindvalues[pos] needs to
		//   be some non-null value - we'll use an empty string
		// * we can't just use charstring::duplicate(value,valuesize)
		//   in all cases, though - it will stop copying at the first
		//   \0 that it encounters, and that will truncate binary data
		if (valuesize) {
			bindvalues[pos]=static_cast<char *>
				(bytestring::duplicate(value,valuesize));
		} else {
			bindvalues[pos]=charstring::duplicate("",0);
		}
		bindsizes[pos]=valuesize;
	}
	bindformats[pos]=1;
	bindcount++;
	usedbindcount++;
	return true;
}

bool postgresqlcursor::inputBindClob(const char *variable, 
					uint16_t variablesize,
					const char *value, 
					uint32_t valuesize,
					int16_t *isnull) {

	// "variable" should be something like ?1,?2,?3, etc.
	// If it's something like ?var1,?var2,?var3, etc. then it'll be
	// converted to 0.  1 will be subtracted and after the cast it will
	// be converted to 65535 and will cause the if below to fail.
	uint16_t	pos=charstring::convertToInteger(variable+1)-1;

	// validate bind index
	if (pos>=maxbindcount) {
		bindformaterror=true;
		return true;
	}

	if (*isnull==conn->getNullBindValue()) {
		bindvalues[pos]=NULL;
		bindsizes[pos]=0;
	} else {
		bindvalues[pos]=charstring::duplicate(value,valuesize);
		bindsizes[pos]=valuesize;
	}
	bindformats[pos]=0;
	bindcount++;
	usedbindcount++;
	return true;
}
#endif

bool postgresqlcursor::supportsNativeBinds(const char *query, uint32_t size) {
#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
	return true;
#else
	return false;
#endif
}

void postgresqlcursor::encodeBlob(stringbuffer *buffer,
					const char *data, uint32_t datasize) {

	// postgresql wants non-printable characters converted to octal with
	// a preceeding slash
	// postgresql also wants it to be quoted

	buffer->append("'");
	for (uint32_t i=0; i<datasize; i++) {
		if (data[i]<' ' || data[i]>'~' ||
			data[i]=='\'' || data[i]=='\\') {
			buffer->append('\\');
			buffer->append(conn->cont->asciiToOctal(data[i]));
		} else {
			buffer->append(data[i]);
		}
	}
	buffer->append("'");
}

void postgresqlcursor::decodeBlob(char **data, uint64_t *datasize) {

	// postgresql bytea text format is either hex (\xDEADBEEF)
	// or escape format (\NNN octal)

	char	*write=*data;
	char	*read=*data;
	char	*end=read+*datasize;

	if (*datasize>=2 && read[0]=='\\' && read[1]=='x') {

		// skip past \x
		read+=2;

		// decode hex format...
		char	buf[3];
		buf[2]='\0';
		for (; read+1<end; read+=2) {
			buf[0]=read[0];
			buf[1]=read[1];
			*write=
			(char)charstring::convertToUnsignedInteger(buf,16);
			write++;
		}

	} else {

		// decode escape format...
		for (; read<end; read++) {

			if (*read=='\\') {

				// decode escaped octal...

				// skip past backslash
				read++;

				// there should either be another backslash
				// or 3 octal digits...

				// handle another backslash
				if (read<end && *read=='\\') {
					*write='\\';
				} else

				// handle 3 octal digits
				if (read+2<end) {
					*write=(char)(
						(read[0]-'0')*64+
						(read[1]-'0')*8+
						(read[2]-'0'));
					read+=2;
				}

			} else {

				// copy out non-encoded character
				*write=*read;
			}

			// move on
			write++;
		}
	}

	*datasize=write-*data;
}

bool postgresqlcursor::executeQuery(const char *query, uint32_t size) {

	// initialize the row counts
	nrows=0;
	currentrow=-1;

	// clean up any result that might be lying around (eg. from a prepare)
	if (pgresult) {
		PQclear(pgresult);
		pgresult=NULL;
	}

	// reset the sqlstate
	sqlstate[0]='\0';

	// If we support PQsendQueryPrepared (in which case we'll also support
	// PQsendQuery) and PQsetSingleRowMode, and fetchatonce>0 then use
	// PQsendQueryPrepared/PQsendQuery and PQsetSingleRowMode.
	//
	// (see note in handleConnectString for why we would set
	// fetchatonce to 0 vs. 1)
	//
	// If we don't support those functions, or if fetchatonce is set to 0
	// (meaning fetch all rows, the default for postgresql in particular)
	// then fall back to using older functions.
	//
	// If we support PQexecPrepared and have bind variables, then use
	// PQexecPrepared.
	//
	// If we don't support that function, or if we don't have bind
	// variables, then fall back to using PQexec.
	bool	getrowcount=false;
#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	if (getFetchAtOnce()) {
		int	result=1;
		if (usedbindcount) {
			result=PQsendQueryPrepared(postgresqlconn->pgconn,
						cursorid,
						usedbindcount,bindvalues,
						bindsizes,bindformats,0);
			usedbindcount=0;
		} else {
			result=PQsendQuery(postgresqlconn->pgconn,query);
		}

		// handle some kind of outright failure
		if (!result) {
			return false;
		}

		// set single-row mode
		if (!PQsetSingleRowMode(postgresqlconn->pgconn)) {
			return false;
		}

		// get the result (and the first row)
		pgresult=PQgetResult(postgresqlconn->pgconn);

		justexecuted=true;
		currentrow=0;
	} else {
#endif
#if defined(HAVE_POSTGRESQL_PQPREPARE) && \
	defined(HAVE_POSTGRESQL_PQEXECPREPARED)
		if (usedbindcount) {
			pgresult=PQexecPrepared(postgresqlconn->pgconn,cursorid,
						usedbindcount,bindvalues,
						bindsizes,bindformats,0);
			usedbindcount=0;
		} else {
#endif
			pgresult=PQexec(postgresqlconn->pgconn,query);
#if defined(HAVE_POSTGRESQL_PQPREPARE) && \
	defined(HAVE_POSTGRESQL_PQEXECPREPARED)
		}
#endif
		getrowcount=true;
#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	}
#endif

	// handle some kind of outright failure
	if (!pgresult) {
		return false;
	}

	// handle errors
	ExecStatusType	pgstatus=PQresultStatus(pgresult);
	if (pgstatus==PGRES_BAD_RESPONSE ||
		pgstatus==PGRES_NONFATAL_ERROR ||
		pgstatus==PGRES_FATAL_ERROR) {
		return false;
	}

	// NOTE: this is a bit of a kludge.
	//
	// Since we set ncols in prepareQuery(), you'd think we would only need
	// to set it here if we don't support prepared queries.  However, since
	// it's set to 0 in closeResultSet() (see long note there as to why),
	// we must set it again here so that it will be correct for reexecuted
	// queries.
	ncols=PQnfields(pgresult);

	// validate column count
	uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
	if (maxcolumncount && (uint32_t)ncols>maxcolumncount) {
		stringbuffer	err;
		err.append(SQLR_ERROR_MAXCOLUMNCOUNTEXCEEDED_STRING);
		err.append(" (")->append(ncols)->append('>');
		err.append(maxcolumncount);
		err.append(')');
		conn->cont->setError(this,err.getString(),
				SQLR_ERROR_MAXCOLUMNCOUNTEXCEEDED,true);
		return false;
	}

	checkForTempTable(query,size);

	// if the function we called above fetches the
	// entire result set at once, then get the row count
	if (getrowcount) {
		nrows=PQntuples(pgresult);
	}

	// get the affected row count
	const char	*affrows=PQcmdTuples(pgresult);
	affectedrows=0;
	if (!charstring::isNullOrEmpty(affrows)) {
		affectedrows=charstring::convertToInteger(affrows);
	}

#ifdef HAVE_POSTGRESQL_PQOIDVALUE
	// get the oid of the inserted row (if this was an insert)
	Oid	coid=PQoidValue(pgresult);
	if (coid!=InvalidOid) {
		postgresqlconn->currentoid=coid;
	}
#endif

	// force re-fetch of column info
	setResultSetHeaderHasBeenHandled(false);

	return true;
}

#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
void postgresqlcursor::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {
	const char	*errorstring=
			(bindformaterror)?
				SQLR_ERROR_INVALIDBINDVARIABLEFORMAT_STRING:
				PQerrorMessage(postgresqlconn->pgconn);
	*errorsize=charstring::getLength(errorstring);
	if (*errorsize>=errorbuffersize) {
		*errorsize=(errorbuffersize)?errorbuffersize-1:0;
	}
	charstring::safeCopy(errorbuffer,errorbuffersize,
					errorstring,*errorsize);
	if (errorbuffersize) {
		errorbuffer[*errorsize]='\0';
	}
	// PostgreSQL doesn't have an error number per-se.  We'll set it
	// to 1 though, because 0 typically means "no error has occurred"
	// and some apps respond that way if errorcode is set to 0.
	// This ends up being important when using:
	// Oracle dblink -> ODBC -> SQL Relay -> PostgreSQL
	*errorcode=(bindformaterror)?SQLR_ERROR_INVALIDBINDVARIABLEFORMAT:1;
	*liveconnection=(PQstatus(postgresqlconn->pgconn)==CONNECTION_OK);
}
#endif

void postgresqlcursor::getSqlState(char *sqlstatebuffer,
					uint32_t sqlstatebuffersize,
					uint32_t *sqlstatesize) {

	// prefer the stash, fall back to the result, which is NULL on some
	// failure paths, as is the field itself when it isn't set
	const char	*state="";
	if (sqlstate[0]) {
		state=sqlstate;
#ifdef HAVE_POSTGRESQL_PQRESULTERRORFIELD
	} else if (pgresult) {
		const char	*field=PQresultErrorField(pgresult,
							PG_DIAG_SQLSTATE);
		if (field) {
			state=field;
		}
#endif
	}

	*sqlstatesize=charstring::getLength(state);
	if (*sqlstatesize>=sqlstatebuffersize) {
		*sqlstatesize=(sqlstatebuffersize)?sqlstatebuffersize-1:0;
	}
	charstring::safeCopy(sqlstatebuffer,sqlstatebuffersize,
					state,*sqlstatesize);
	if (sqlstatebuffersize) {
		sqlstatebuffer[*sqlstatesize]='\0';
	}
}

bool postgresqlcursor::knowsRowCount() {
#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	if (getFetchAtOnce()) {
		return false;
	}
#endif
	return true;
}

uint64_t postgresqlcursor::rowCount() {
	return nrows;
}

uint64_t postgresqlcursor::getAffectedRows() {
	return affectedrows;
}

uint32_t postgresqlcursor::colCount() {
	return ncols;
}

uint16_t postgresqlcursor::columnTypeFormat() {
	// typemangling=2 is the only mode where getColumnTypeName() returns
	// a real, looked-up name - otherwise it just stringifies the raw
	// oid, so send ids instead and let the client resolve a name from
	// the type id that getColumnType() already maps correctly
	if (postgresqlconn->typemangling==2) {
		return (uint16_t)COLUMN_TYPE_NAMES;
	} else {
		return (uint16_t)COLUMN_TYPE_IDS;
	}
}

const char *postgresqlcursor::getColumnName(uint32_t col) {
	return PQfname(pgresult,col);
}

uint16_t postgresqlcursor::getColumnType(uint32_t col) {
	// Types are strange in POSTGRESQL, there are no actual
	// types, only internal numbers that correspond to 
	// types which are defined in a database table 
	// somewhere.
	// If typemangling is turned on, translate to standard
	// types, otherwise return the type number.
	switch (PQftype(pgresult,col)) {
		case 16: //bool
			return BOOL_DATATYPE;
		case 17: //bytea
			return BYTEA_DATATYPE;
		case 18: //char
			return CHAR_DATATYPE;
		case 19: //name
			return NAME_DATATYPE;
		case 20: //int8
			return INT8_DATATYPE;
		case 21: //int2
			return INT2_DATATYPE;
		case 22: //int2vector
			return INT2VECTOR_DATATYPE;
		case 23: //int4
			return INT4_DATATYPE;
		case 24: //regproc
			return REGPROC_DATATYPE;
		case 25: //text
			return TEXT_DATATYPE;
		case 26: //oid
			return OID_DATATYPE;
		case 27: //tid
			return TID_DATATYPE;
		case 28: //xid
			return XID_DATATYPE;
		case 29: //cid
			return CID_DATATYPE;
		case 30: //oidvector
			return OIDVECTOR_DATATYPE;
		case 71: //pg_type
			return PG_TYPE_DATATYPE;
		case 75: //pg_attribute
			return PG_ATTRIBUTE_DATATYPE;
		case 81: //pg_proc
			return PG_PROC_DATATYPE;
		case 83: //pg_class
			return PG_CLASS_DATATYPE;
		case 210: //smgr
			return SMGR_DATATYPE;
		case 600: //point
			return POINT_DATATYPE;
		case 601: //lseg
			return LSEG_DATATYPE;
		case 602: //path
			return PATH_DATATYPE;
		case 603: //box
			return BOX_DATATYPE;
		case 604: //polygon
			return POLYGON_DATATYPE;
		case 628: //line
			return LINE_DATATYPE;
		case 629: //_line
			return _LINE_DATATYPE;
		case 651: //_cidr
			return _CIDR_DATATYPE;
		case 700: //float4
			return FLOAT4_DATATYPE;
		case 701: //float8
			return FLOAT8_DATATYPE;
		case 702: //abstime
			return ABSTIME_DATATYPE;
		case 703: //reltime
			return RELTIME_DATATYPE;
		case 704: //tinterval
			return TINTERVAL_DATATYPE;
		case 718: //circle
			return CIRCLE_DATATYPE;
		case 719: //_circle
			return _CIRCLE_DATATYPE;
		case 790: //money
			return MONEY_DATATYPE;
		case 791: //_money
			return _MONEY_DATATYPE;
		case 829: //macaddr
			return MACADDR_DATATYPE;
		case 869: //inet
			return INET_DATATYPE;
		case 650: //cidr
			return CIDR_DATATYPE;
		case 1000: //_bool
			return _BOOL_DATATYPE;
		case 1001: //_bytea
			return _BYTEA_DATATYPE;
		case 1002: //_char
			return _CHAR_DATATYPE;
		case 1003: //_name
			return _NAME_DATATYPE;
		case 1005: //_int2
			return _INT2_DATATYPE;
		case 1006: //_int2vector
			return _INT2VECTOR_DATATYPE;
		case 1007: //_int4
			return _INT4_DATATYPE;
		case 1008: //_regproc
			return _REGPROC_DATATYPE;
		case 1009: //_text
			return _TEXT_DATATYPE;
		case 1010: //_tid
			return _TID_DATATYPE;
		case 1011: //_xid
			return _XID_DATATYPE;
		case 1012: //_cid
			return _CID_DATATYPE;
		case 1013: //_oidvector
			return _OIDVECTOR_DATATYPE;
		case 1014: //_bpchar
			return _BPCHAR_DATATYPE;
		case 1015: //_varchar
			return _VARCHAR_DATATYPE;
		case 1016: //_int8
			return _INT8_DATATYPE;
		case 1017: //_point
			return _POINT_DATATYPE;
		case 1018: //_lseg
			return _LSEG_DATATYPE;
		case 1019: //_path
			return _PATH_DATATYPE;
		case 1020: //_box
			return _BOX_DATATYPE;
		case 1021: //_float4
			return _FLOAT4_DATATYPE;
		case 1022: //_float8
			return _FLOAT8_DATATYPE;
		case 1023: //_abstime
			return _ABSTIME_DATATYPE;
		case 1024: //_reltime
			return _RELTIME_DATATYPE;
		case 1025: //_tinterval
			return _TINTERVAL_DATATYPE;
		case 1027: //_polygon
			return _POLYGON_DATATYPE;
		case 1028: //_oid
			return _OID_DATATYPE;
		case 1033: //aclitem
			return ACLITEM_DATATYPE;
		case 1034: //_aclitem
			return _ACLITEM_DATATYPE;
		case 1040: //_macaddr
			return _MACADDR_DATATYPE;
		case 1041: //_inet
			return _INET_DATATYPE;
		case 1042: //bpchar
			return BPCHAR_DATATYPE;
		case 1043: //varchar
			return VARCHAR_DATATYPE;
		case 1082: //date
			return DATE_DATATYPE;
		case 1083: //time
			return TIME_DATATYPE;
		case 1114: //timestamp
		case 1296:
			return TIMESTAMP_DATATYPE;
		case 1115: //_timestamp
			return _TIMESTAMP_DATATYPE;
		case 1182: //_date
			return _DATE_DATATYPE;
		case 1183: //_time
			return _TIME_DATATYPE;
		case 1184: //timestamptz
			return TIMESTAMPTZ_DATATYPE;
		case 1185: //_timestamptz
			return _TIMESTAMPTZ_DATATYPE;
		case 1186: //interval
			return INTERVAL_DATATYPE;
		case 1187: //_interval
			return _INTERVAL_DATATYPE;
		case 1231: //_numeric
			return _NUMERIC_DATATYPE;
		case 1266: //timetz
			return TIMETZ_DATATYPE;
		case 1270: //_timetz
			return _TIMETZ_DATATYPE;
		case 1560: //bit
			return BIT_DATATYPE;
		case 1561: //_bit
			return _BIT_DATATYPE;
		case 1562: //varbit
			return VARBIT_DATATYPE;
		case 1563: //_varbit
			return _VARBIT_DATATYPE;
		case 1700: //numeric
			return NUMERIC_DATATYPE;
		case 1790: //refcursor
			return REFCURSOR_DATATYPE;
		case 2201: //_refcursor
			return _REFCURSOR_DATATYPE;
		case 2202: //regprocedure
			return REGPROCEDURE_DATATYPE;
		case 2203: //regoper
			return REGOPER_DATATYPE;
		case 2204: //regoperator
			return REGOPERATOR_DATATYPE;
		case 2205: //regclass
			return REGCLASS_DATATYPE;
		case 2206: //regtype
			return REGTYPE_DATATYPE;
		case 2207: //_regprocedure
			return _REGPROCEDURE_DATATYPE;
		case 2208: //_regoper
			return _REGOPER_DATATYPE;
		case 2209: //_regoperator
			return _REGOPERATOR_DATATYPE;
		case 2210: //_regclass
			return _REGCLASS_DATATYPE;
		case 2211: //_regtype
			return _REGTYPE_DATATYPE;
		case 2249: //record
			return RECORD_DATATYPE;
		case 2275: //cstring
			return CSTRING_DATATYPE;
		case 2276: //any
			return ANY_DATATYPE;
		case 2277: //anyarray
			return ANYARRAY_DATATYPE;
		case 2278: //void
			return VOID_DATATYPE;
		case 2279: //trigger
			return TRIGGER_DATATYPE;
		case 2280: //language_handler
			return LANGUAGE_HANDLER_DATATYPE;
		case 2281: //internal
			return INTERNAL_DATATYPE;
		case 2282: //opaque
			return OPAQUE_DATATYPE;
		case 2283: //anyelement
			return ANYELEMENT_DATATYPE;
		case 705: //unknown
		default:
			return UNKNOWN_DATATYPE;
	}
}

const char *postgresqlcursor::getColumnTypeName(uint32_t col) {
	// Types are strange in POSTGRESQL, there are no actual
	// types, only internal numbers that correspond to 
	// types which are defined in a database table 
	// somewhere.
	// typemangling=0 means return the internal number as a string
	// typemangling=1 means translate to a standard datatype
	// 			(handled by getColumnType above)
	// typemangling=2 means return the name as a string
	Oid	pgfieldtype=PQftype(pgresult,col);
	if (!postgresqlconn->typemangling) {
		charstring::printf(typenamebuffer[col],
						sizeof(typenamebuffer[col]),
						"%d",(int32_t)pgfieldtype);
		return typenamebuffer[col];
	}
	return postgresqlconn->datatypes.getValue((int32_t)pgfieldtype);
}

uint32_t postgresqlcursor::getColumnSize(uint32_t col) {

	// PQfsize returns the binary storage size for
	// fixed-length types or -1 for variable-length types.
	int32_t	size=PQfsize(pgresult,col);
	if (size<0) {
#ifdef HAVE_POSTGRESQL_PQFMOD
		// For variable-length types, PQfmod returns the type
		// modifier, which encodes length, precision, and scale
		// in type-specific ways.
		int32_t	typmod=PQfmod(pgresult,col);
		switch (PQftype(pgresult,col)) {
			case 1042: // bpchar
			case 1043: // varchar
				// length in characters, not bytes
				size=(typmod>=4)?(typmod-4):0;
				break;
			case 1700: // numeric
				// precision in decimal digits
				size=(typmod>=4)?((typmod-4)>>16)&0xFFFF:0;
				break;
			default:
				// length in bytes
				size=(typmod>=0)?typmod:0;
				break;
		}
#else
		size=0;
#endif
	}
	return size;
}

uint32_t postgresqlcursor::getColumnScale(uint32_t col) {
#ifdef HAVE_POSTGRESQL_PQFMOD
	// For variable-length types, PQfmod returns the type
	// modifier, which encodes length, precision, and scale
	// in type-specific ways.
	int32_t	typmod=PQfmod(pgresult,col);
	switch (PQftype(pgresult,col)) {
		case 1700: // numeric
			// scale in decimal digits
			return (typmod>=4)?((typmod-4)&0xFFFF):0;
		case 1083: // time
		case 1114: // timestamp
		case 1184: // timestamptz
		case 1186: // interval
		case 1266: // timetz
			// fractional-seconds precision
			// (-1 means the default of 6)
			return (typmod>=0)?typmod:6;
		default:
			return 0;
	}
#else
	return 0;
#endif
}

uint16_t postgresqlcursor::getColumnIsBinary(uint32_t col) {
	// is this binary data (all columns will contain binary data if it is)
	int16_t	binary=false;
#ifdef HAVE_POSTGRESQL_PQBINARYTUPLES
	binary=PQbinaryTuples(pgresult);
#endif
	return binary;
}

uint16_t postgresqlcursor::getColumnIsNullable(uint32_t col) {

#ifdef HAVE_POSTGRESQL_PQFTABLE

	// this is an expensive operation, don't do it by default
	if (!postgresqlconn->enablecolumnisnullable) {
		return 1;
	}

	// If the column is an expression or literal, it's nullable.  PQftable
	// ought to catch it, but if it doesn't then fall back to PQftablecol.
	Oid	tableoid=PQftable(pgresult,col);
	if (tableoid==InvalidOid) {
		return 1;
	}
	int	colnum=PQftablecol(pgresult,col);
	if (colnum<=0) {
		return 1;
	}

	// query pg_attribute
	char	query[128];
	charstring::printf(query,sizeof(query),
				"select attnotnull from pg_attribute "
				"where attrelid=%u and attnum=%d",
				(unsigned int)tableoid,colnum);
	PGresult	*result=PQexec(postgresqlconn->pgconn,query);
	if (!result) {
		return 1;
	}
	uint16_t	nullable=1;
	if (PQresultStatus(result)==PGRES_TUPLES_OK && PQntuples(result)>0) {
		const char	*v=PQgetvalue(result,0,0);
		nullable=(v && (*v=='t' || *v=='T' || *v=='1'))?0:1;
	}
	PQclear(result);
	return nullable;
#else
	// without PQftable we cannot map back to pg_attribute, so just
	// say nullable (the postgresql default for columns without an
	// explicit NOT NULL constraint)
	return 1;
#endif
}

#ifdef HAVE_POSTGRESQL_PQFTABLE
const char *postgresqlcursor::getColumnTable(uint32_t col) {
	// PQftable returns an oid rather than a table name, so we have to map
	// it to a table name.
	// tablemangling=0 means return the internal number as a string
	// tablemangling=2 means return the name as a string
	Oid	pgfieldtable=PQftable(pgresult,col);
	if (!postgresqlconn->tablemangling) {
		charstring::printf(tablenamebuffer,sizeof(tablenamebuffer),
						"%d",(int32_t)pgfieldtable);
		return tablenamebuffer;
	}
	return postgresqlconn->tables.getValue((int32_t)pgfieldtable);
}
#endif

bool postgresqlcursor::noRowsToReturn() {
#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	// if there are no columns, then there can't be any rows either
	if (getFetchAtOnce()) {
		return !ncols;
	} else {
#endif
		// Why test ncols below, if we can just test nrows?
		//
		// It's a bit of a kludge to improve performance, but also to
		// work around an issue with the sqlrclient protocol.
		//
		// Queries which don't specify columns like "select from test"
		// are apparently valid in postgresql.  For those queries,
		// ncols will be set to 0 but nrows will be set to the correct
		// row count.
		//
		// Unless we also check ncols here, then the server will end up
		// spinning through all of the rows, returning nothing for each
		// row.
		//
		// This is inefficient, so also checking for ncols=0 allows the
		// server to immediately tell the client that there are no rows
		// and proceed to closeResultSet().
		//
		// In addition, spinning through the rows, returning nothing
		// also causes problems for the sqlrclient protocol when the
		// result set buffer size is larger than the nrows.
		// 
		// For each row, the client sits there waiting for either a
		// field type, or and end-of-result-set flag.  If there are no
		// columns then the server sends nothing at all for the first
		// set of rows, then waits for the client to tell it to send
		// more rows.  So, both sides end up waiting on the other.
		//
		// Other protocols send a marker for each row, so it wouldn't
		// be a problem for them, but since it generally improves
		// performance and helps sqlrclient, we'll go ahead and test
		// ncols here too.
		return (!ncols || !nrows);
#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	}
#endif
}

bool postgresqlcursor::fetchRow(bool *error) {

	*error=false;
	// FIXME: set error if an error occurs

#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	if (getFetchAtOnce()) {
		if (!justexecuted) {
			PQclear(pgresult);
			pgresult=PQgetResult(postgresqlconn->pgconn);
		} else {
			justexecuted=false;
		}
		// The docs say call PQgetResult until it returns null, but it
		// will actually return non-null one time when called after the
		// end of the result set.  Fortunately, we can detect the true
		// end with PQresultStatus.
		if (PQresultStatus(pgresult)==PGRES_SINGLE_TUPLE && pgresult) {
			return true;
		}
		return false;
	}
#endif
	if (currentrow<nrows-1) {
		currentrow++;
		return true;
	}
	return false;
}

void postgresqlcursor::getField(uint32_t col,
				const char **field, uint64_t *fieldsize,
				bool *lob, bool *null) {

	// handle NULLs
	if (PQgetisnull(pgresult,currentrow,col)) {
		*null=true;
		return;
	}

	// handle normal datatypes
	*field=PQgetvalue(pgresult,currentrow,col);
	*fieldsize=PQgetlength(pgresult,currentrow,col);

	// decode encoded binary data
	// (unless the user has opted out via decodeblobs=no)
	if (PQftype(pgresult,col)==17 && postgresqlconn->getDecodeBlobs()) {
		decodeBlob((char **)field,fieldsize);
	}
}

void postgresqlcursor::closeResultSet() {

#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
	for (uint16_t i=0; i<bindcount; i++) {
		delete[] bindvalues[i];
		bindvalues[i]=NULL;
	}
	bindcount=0;
#endif

#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	if (getFetchAtOnce()) {
		for (;;) {
			if (pgresult) {
				PQclear(pgresult);
			} else {
				break;
			}
			pgresult=PQgetResult(postgresqlconn->pgconn);
		}
		justexecuted=false;
	} else {
#endif
		if (pgresult) {
			PQclear(pgresult);
			pgresult=NULL;
		}
#if defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE)
	}
#endif

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

#if (defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))
void postgresqlcursor::deallocateNamedStatement() {

	// The "namedstmtallocated" flag can drift out of sync with the server
	// across abnormal session termination paths, leaving us thinking a
	// statement isn't registered when it actually is.  The next PQprepare
	// on this cursor name then fails with "prepared statement already
	// exists" and the cursor is wedged.
	//
	// We can't just always DEALLOCATE, because that raises an error if
	// the statement doesn't actually exist, aborting any open
	// transaction.  So, when the flag says "not allocated", probe
	// pg_prepared_statements to see if we're out of sync.
	bool		exists=namedstmtallocated;
	if (!exists) {
		stringbuffer	probe;
		probe.append("select 1 from pg_prepared_statements "
				"where name='")->append(cursorid)->
				append("'");
		PGresult	*r=PQexec(postgresqlconn->pgconn,
					probe.getString());
		exists=(r && PQresultStatus(r)==PGRES_TUPLES_OK &&
				PQntuples(r)>0);
		PQclear(r);
	}

	if (exists) {
		PGresult	*r=PQexec(postgresqlconn->pgconn,
					deallocatecursorid.getString());
		PQclear(r);
	}
	namedstmtallocated=false;
}
#endif

#if ((defined(HAVE_POSTGRESQL_PQPREPARE) && \
		defined(HAVE_POSTGRESQL_PQEXECPREPARED)) || \
		(defined(HAVE_POSTGRESQL_PQSENDQUERYPREPARED) && \
		defined(HAVE_POSTGRESQL_PQSETSINGLEROWMODE))) && \
		defined(HAVE_POSTGRESQL_PQDESCRIBEPREPARED)
bool postgresqlcursor::columnInfoIsValidAfterPrepare() {
	return true;
}
#endif

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_postgresqlconnection(
						sqlrservercontroller *cont) {
		return new postgresqlconnection(cont);
	}
}
