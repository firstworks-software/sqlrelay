// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/environment.h>
#include <rudiments/bytestring.h>
#include <rudiments/snooze.h>
#include <rudiments/sys.h>

#include <defines.h>
#include <datatypes.h>
#include <config.h>

#include <ibase.h>

// for pow()
#include <math.h>

// for struct tm
#include <time.h>

#define MAX_ITEM_BUFFER_SIZE 32768
#define MAX_SELECT_LIST_SIZE 256
#define MAX_BIND_VARS 512
#define MAX_LOB_CHUNK_SIZE 65535

// fb_interpret (firebird 2.0+) supersedes the deprecated isc_interprete, whose
// sizeless buffer walk can overflow msg and stack garbage after the real
// error; configure sets HAVE_FB_INTERPRET when the client has it
static ISC_LONG fbInterpret(char *msg, unsigned int msgsize,
					const ISC_STATUS **pvector) {
#ifdef HAVE_FB_INTERPRET
	return fb_interpret(msg,msgsize,pvector);
#else
	// isc_interprete takes a non-const ISC_STATUS**; it only advances the
	// walking pointer, so dropping const is safe
	return isc_interprete(msg,(ISC_STATUS **)pvector);
#endif
}

struct fieldstruct {
	int		sqlrtype;
	short		type;

	short		shortbuffer;
	long		longbuffer;
	float		floatbuffer;
	double		doublebuffer;
	ISC_QUAD	quadbuffer;
	ISC_DATE	datebuffer;
	ISC_TIME	timebuffer;
	ISC_TIMESTAMP	timestampbuffer;
	ISC_INT64	int64buffer;
	char		*textbuffer;
	ISC_QUAD	blobid;
	isc_blob_handle	blobhandle;
	bool		blobisopen;

	short		nullindicator;
};

struct datebind {
        int16_t         *year;
        int16_t         *month;
        int16_t         *day;
        int16_t         *hour;
        int16_t         *minute;
        int16_t         *second;
        const char      **tz;
	bool		*isnegative;
	ISC_TIMESTAMP	buffer;
};

class firebirdconnection;

class SQLRSERVER_DLLSPEC firebirdcursor : public sqlrservercursor {
	friend class firebirdconnection;
	private:
		firebirdcursor(sqlrserverconnection *conn, uint16_t id);
		~firebirdcursor();
		void		allocateResultSetBuffers(int32_t columncount);
		void		deallocateResultSetBuffers();
		bool		prepareQuery(const char *query,
						uint32_t size);
		bool		inputBind(const char *variable, 
						uint16_t variablesize,
						const char *value, 
						uint32_t valuesize,
						short *isnull);
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
						short *isnull);
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
		void		closeLobOutputBind(uint16_t index);
		bool		executeQuery(const char *query,
						uint32_t size);
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);
		void		checkForTempTable(const char *query,
							uint32_t size);
		bool		queryIsNotSelect();
		bool		queryIsCommitOrRollback();
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
		uint16_t	getColumnNameSize(uint32_t col);
		uint16_t	getColumnType(uint32_t col);
		uint32_t	getColumnSize(uint32_t col);
		uint32_t	getColumnPrecision(uint32_t col);
		uint32_t	getColumnScale(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		const char	*getColumnTable(uint32_t col);
		uint16_t	getColumnTableSize(uint32_t col);
		bool		noRowsToReturn();
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null);
		bool		getLobFieldLength(uint32_t col,
						uint64_t *length);
		bool		getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread);
		void		closeLobField(uint32_t col);
		void		closeResultSet();


		isc_stmt_handle	stmt;

		uint16_t	maxbindcount;

		XSQLDA	ISC_FAR	*inbindsqlda;
		ISC_TIMESTAMP	*inbindts;
		ISC_QUAD	*inbindblobid;
		isc_blob_handle	*inbindblobhandle;

		XSQLDA	ISC_FAR	*outbindsqlda;
		ISC_QUAD	*outbindblobid;
		isc_blob_handle	*outbindblobhandle;
		bool		*outbindblobisopen;
		uint16_t	outbindcount;
		datebind	*outdatebind;
		
		XSQLDA	ISC_FAR	*outsqlda;
		byte_t		*outsqldabuffer;
		fieldstruct	*field;

		ISC_LONG	querytype;

		firebirdconnection	*firebirdconn;

		bool	queryisexecsp;
		bool	bindformaterror;

		regularexpression	executeprocedure;
};

class SQLRSERVER_DLLSPEC firebirdconnection : public sqlrserverconnection {
	friend class firebirdcursor;
	public:
		firebirdconnection(sqlrservercontroller *cont);
		~firebirdconnection();
	private:
		void	initDatabaseFeatures();
		void	handleConnectString();
		bool	logIn(const char **error, const char **warning);
		sqlrservercursor	*newCursor(uint16_t id);
		void	deleteCursor(sqlrservercursor *curs);
		void	logOut();
		sqlrtxmodel_t	getNativeTransactionModel();
		bool	setAutoCommitOn();
		bool	setAutoCommitOff();
		bool	supportsAutoCommit();
		bool	commit();
		bool	rollback();
		bool	ping();
		void	getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t	*errorcode,
					bool *liveconnection);
		bool		selectCatalog(const char *catalog);
		char		*getCurrentCatalog();
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
		const char	*getGlobalTempTableListQuery();
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
		const char	*getBindFormat();
		const char	*getNextvalFormat();
		const char	*getCurrentUserQuery();
		const char	*getLastInsertIdQuery();
		const char	*setIsolationLevelQuery();
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		const char	*getNoopQuery();

		char		dpb[256];
		short		dpbsize;
		isc_db_handle	db;
		isc_tr_handle	tr;

		char		*database;
		char		*host;
		unsigned short	dialect;

		const char	*charset;

		bool		droptemptables;

		char		*dbversion;

		char		*lastinsertidquery;

		stringbuffer	schemalistquery;
		stringbuffer	tabletypelistquery;
		stringbuffer	tablelistquery;
		stringbuffer	procedurelistquery;
		stringbuffer	columnlistquery;
		stringbuffer	typeinfolistquery;
		stringbuffer	primarykeyslistquery;
		stringbuffer	keyandindexlistquery;
		stringbuffer	procedureparameterlistquery;

		ISC_STATUS	error[20];

		stringbuffer	errormsg;

		bool		autocommit;

		char		*maxconnections;
		const char	*databasefeatures[FEATURE_COUNT];
};

static char tpb[] = {
	isc_tpb_version3,
	isc_tpb_write,
	isc_tpb_read_committed,
	isc_tpb_rec_version,
	// FIXME: vladimir changed this to isc_tpb_nowait.  why?
	isc_tpb_wait
};

static char tpbac[] = {
	isc_tpb_version3,
	isc_tpb_write,
	isc_tpb_read_committed,
	isc_tpb_rec_version,
	// FIXME: vladimir changed this to isc_tpb_nowait.  why?
	isc_tpb_wait,
	isc_tpb_autocommit
};

firebirdconnection::firebirdconnection(sqlrservercontroller *cont) :
						sqlrserverconnection(cont) {
	dbversion=NULL;
	lastinsertidquery=NULL;
	database=NULL;
	host=NULL;
	autocommit=false;
	initDatabaseFeatures();
}

firebirdconnection::~firebirdconnection() {
	delete[] dbversion;
	delete[] lastinsertidquery;
	delete[] database;
	delete[] host;
	delete[] maxconnections;
}

void firebirdconnection::initDatabaseFeatures() {

	maxconnections=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=
		"ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM";

	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		"false";

	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		"false";

	databasefeatures[FEATURE_ALTER_DOMAIN_CLAUSES]=
		"";

	databasefeatures[FEATURE_ALTER_TABLE_OPERATIONS]=
		"ADD_COLUMN,DROP_COLUMN";

	databasefeatures[FEATURE_ANSI92_SQL_LEVELS]=
		"ENTRY_LEVEL";

	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
		"false";

	databasefeatures[FEATURE_BATCH_OPERATIONS]=
		"";

	databasefeatures[FEATURE_BATCH_ROW_COUNTS]=
		"";

	databasefeatures[FEATURE_CATALOG_SEPARATOR]=
		"";

	databasefeatures[FEATURE_CATALOG_TERM]=
		"";

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
		"CREATE_TABLE,TABLE_CONSTRAINT,"
			"CONSTRAINT_NAME_DEFINITION,COLUMN_CONSTRAINT,"
			"COLUMN_DEFAULT,COLUMN_COLLATION,"
			"GLOBAL_TEMPORARY,COMMIT_DELETE,COMMIT_PRESERVE";

	databasefeatures[FEATURE_CREATE_TRANSLATION_CLAUSES]=
		"";

	databasefeatures[FEATURE_CREATE_VIEW_CLAUSES]=
		"CREATE_VIEW,CHECK_OPTION";

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
		"$";

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		"CASCADE,NO_ACTION,SET_DEFAULT,SET_NULL";

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		"false";

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		"DELETE_TABLE,INSERT_TABLE,"
			"REFERENCES_TABLE,REFERENCES_COLUMN,"
			"SELECT_TABLE,UPDATE_COLUMN,UPDATE_TABLE,"
			"WITH_GRANT_OPTION";

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		"BASIC";

	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		"UPPER";

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
		"READ_COMMITTED,REPEATABLE_READ,SERIALIZABLE";

	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		"false";

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		"";

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		"true";

	databasefeatures[FEATURE_LOCK_TYPES]=
		"NO_CHANGE";

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		"32765";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		"32767";

	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_CONNECTIONS]=maxconnections;

	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		"65531";

	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_STATEMENTS]=
		"0";

	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		"10485760";

	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		"31";

	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		"false";

	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		"true";

	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		"LOW";

	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		"TAN,MOD,LOG,COS,ROUND,SQRT,ASIN,ATAN2,COT,"
			"POWER,LOG10,ABS,FLOOR,DEGREES,CEILING,ACOS,"
			"RADIANS,PI,SIN,SIGN,EXP,ATAN,TRUNCATE";

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
		"SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
		"SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
		"SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_PREDICATES]=
		"BETWEEN,COMPARISON,EXISTS,IN,"
			"ISNOTNULL,ISNULL,LIKE,"
			"QUANTIFIED_COMPARISON";

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		"PROCEDURE";

	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		"SENSITIVE";

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		"CROSS_JOIN,FULL_OUTER_JOIN,INNER_JOIN,"
			"LEFT_OUTER_JOIN,RIGHT_OUTER_JOIN";

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		"FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,"
				"SCROLL_INSENSITIVE/READ_ONLY,"
				"SCROLL_INSENSITIVE/UPDATABLE,"
				"SCROLL_SENSITIVE/READ_ONLY,"
				"SCROLL_SENSITIVE/UPDATABLE";

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		"HOLD_CURSORS_OVER_COMMIT,CLOSE_CURSORS_AT_COMMIT";

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		"DELETE_TABLE,GRANT_OPTION_FOR,"
			"INSERT_TABLE,REFERENCES_COLUMN,"
			"REFERENCES_TABLE,SELECT_TABLE,"
			"UPDATE_COLUMN,UPDATE_TABLE";

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		"ROWID_UNSUPPORTED";

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		"VALUE_EXPRESSION,NULL,ROW_SUBQUERY";

	databasefeatures[FEATURE_SCHEMA_TERM]=
		"";

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		"";

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		"READ_ONLY";

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		"\\";

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		"MINIMUM,CORE,EXTENDED";

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		"ADD,ADMIN,BIT_LENGTH,CURRENT_CONNECTION,"
			"CURRENT_TRANSACTION,DELETING,GDSCODE,INDEX,"
			"INSERTING,LONG,OFFSET,PLAN,POST_EVENT,"
			"RDB$DB_KEY,RDB$RECORD_VERSION,RECORD_VERSION,"
			"RECREATE,RETURNING_VALUES,ROW_COUNT,SQLCODE,"
			"UPDATING,VARIABLE,VIEW,WHILE";

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		"2";

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		"PROCEDURES";

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		"CHARACTER_LENGTH,LEFT,REPEAT,CONCAT,SUBSTRING,"
			"LENGTH,UCASE,CHAR,ASCII,SPACE,POSITION,LCASE,"
			"LTRIM,RIGHT,INSERT,CHAR_LENGTH,LOCATE,REPLACE,"
			"OCTET_LENGTH,RTRIM";

	databasefeatures[FEATURE_SUBQUERY_USAGE]=
		"COMPARISONS,EXISTS,INS,QUANTIFIEDS";

	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_DESCRIBE_PARAMETER]=
		"false";

	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		"true";

	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		"false";

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
		"DATABASE,IFNULL,USER";

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		"BASIC";

	databasefeatures[FEATURE_TABLE_TERM]=
		"table";

	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		"";

	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		"";

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		"DAYOFMONTH,MONTHNAME,MONTH,CURRENT_TIMESTAMP,"
			"HOUR,DAYOFYEAR,TIMESTAMPADD,DAYOFWEEK,QUARTER,"
			"TIMESTAMPDIFF,YEAR,CURTIME,NOW,DAYNAME,MINUTE,"
			"SECOND,CURRENT_DATE,CURRENT_TIME,WEEK,CURDATE,"
			"EXTRACT";

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
		"DELETE,UPDATE";

}

void firebirdconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	// override legacy "database" parameter with modern "db" parameter
	const char	*dbtmp=cont->getConnectStringValue("db");
	if (charstring::isNullOrEmpty(dbtmp)) {
		dbtmp=cont->getConnectStringValue("database");
	}
	database=charstring::duplicate(dbtmp);

	const char	*dialectstr=cont->getConnectStringValue("dialect");
	if (dialectstr) {
		dialect=charstring::convertToInteger(dialectstr);
		if (dialect<1) {
			dialect=1;
		}
		if (dialect>3) {
			dialect=3;
		}
	} else {
		dialect=3;
	}

	charset=cont->getConnectStringValue("charset");

	droptemptables=charstring::isYes(
			cont->getConnectStringValue("droptemptables"));

	cont->addGlobalTempTables(
			cont->getConnectStringValue("globaltemptables"));

	const char	*lastinsertidfunc=
			cont->getConnectStringValue("lastinsertidfunction");
	if (lastinsertidfunc) {
		stringbuffer	liiquery;
		liiquery.append("select id from ");
		liiquery.append(lastinsertidfunc);
		lastinsertidquery=liiquery.detachString();
	}

	// firebird doesn't support multi-row fetches
	cont->setFetchAtOnce(1);
}

bool firebirdconnection::logIn(const char **err, const char **warning) {

	// parse the host name from the database
	const char	*colon=charstring::findFirst(database,':');
	delete[] host;
	if (colon) {
		host=charstring::duplicate(database,colon-database);
	} else {
		host=sys::getHostName();
	}

	// initialize a parameter buffer
	char	*dpbptr=dpb;

	// set the parameter buffer version
	*dpbptr=isc_dpb_version1;
	dpbptr++;

	// no idea what this does, something involving the "cache"
	*dpbptr=isc_dpb_num_buffers;
	dpbptr++;
	*dpbptr=1;
	dpbptr++;
	*dpbptr=90;
	dpbptr++;

	// set the character set
	if (charstring::getLength(charset)) {
		*dpbptr=isc_dpb_lc_ctype;
		dpbptr++;
		*dpbptr=charstring::getLength(charset);
		dpbptr++;
		charstring::copy(dpbptr,charset);
		dpbptr+=charstring::getLength(charset);
	}

	// determine the parameter buffer size
	dpbsize=dpbptr-dpb;

	// handle user/password parameters
	const char	*user=cont->getLoginUser();
	if (user) {
		environment::setValue("ISC_USER",user);
	}
	const char	*password=cont->getLoginPassword();
	if (password) {
		environment::setValue("ISC_PASSWORD",password);
	}

	// attach to the database
	db=0L;
	tr=0L;
	if (isc_attach_database(error,charstring::getLength(database),
						database,&db,dpbsize,dpb)) {
		db=0L;

		errormsg.clear();

		char			msg[512];
		const ISC_STATUS	*errstatus=error;
		bool			first=false;
		while (fbInterpret(msg,sizeof(msg),&errstatus)) {
			if (first) {
				errormsg.append(": ");
			}
			errormsg.append(msg);
			first=true;
		}
		*err=errormsg.getString();
		return false;
	}

	// start a transaction
	if (isc_start_transaction(error,&tr,1,&db,(uint16_t)sizeof(tpb),&tpb)) {

		tr=0L;

		errormsg.clear();

		char			msg[512];
		const ISC_STATUS	*errstatus=error;
		bool			first=false;
		while (fbInterpret(msg,sizeof(msg),&errstatus)) {
			if (first) {
				errormsg.append(": ");
			}
			errormsg.append(msg);
			first=true;
		}
		*err=errormsg.getString();
		return false;
	}

	return true;
}

sqlrservercursor *firebirdconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new firebirdcursor(
					(sqlrserverconnection *)this,id);
}

void firebirdconnection::deleteCursor(sqlrservercursor *curs) {
	delete (firebirdcursor *)curs;
}

void firebirdconnection::logOut() {
	isc_detach_database(error,&db);
}

sqlrtxmodel_t firebirdconnection::getNativeTransactionModel() {
	return SQLRTXMODEL_IMPLICIT;
}

bool firebirdconnection::setAutoCommitOn() {
	autocommit=true;
	return !isc_commit_transaction(error,&tr) &&
		!isc_start_transaction(error,&tr,1,&db,
					(uint16_t)sizeof(tpbac),&tpbac);
}

bool firebirdconnection::setAutoCommitOff() {
	autocommit=false;
	return !isc_commit_transaction(error,&tr) &&
		!isc_start_transaction(error,&tr,1,&db,
					(uint16_t)sizeof(tpb),&tpb);
}

bool firebirdconnection::supportsAutoCommit() {
	return true;
}

bool firebirdconnection::commit() {
	if (autocommit) {
		return !isc_commit_retaining(error,&tr);
	} else {
		return !isc_commit_transaction(error,&tr) &&
			!isc_start_transaction(error,&tr,1,&db,
					(uint16_t)sizeof(tpb),&tpb);
	}
}

bool firebirdconnection::rollback() {
	if (autocommit) {
		return !isc_rollback_retaining(error,&tr);
	} else {
		return !isc_rollback_transaction(error,&tr) &&
			!isc_start_transaction(error,&tr,1,&db,
					(uint16_t)sizeof(tpb),&tpb);
	}
}

void firebirdconnection::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {

	// declare a buffer for the error
	errormsg.clear();

	char			msg[512];
	const ISC_STATUS	*pvector=error;

	// get the status message
	while (fbInterpret(msg,sizeof(msg),&pvector)) {
		errormsg.append(msg)->append(" \n");
	}

	// get the sql code
	ISC_LONG	sqlcode=isc_sqlcode(error);

	// return the detailed status-vector message rather than the
	// generic sqlcode text, which hides the real cause (e.g. -901);
	// fall back to the sqlcode text if there was no detail
	if (errormsg.getStringLength()) {
		charstring::safeCopy(errorbuffer,errorbuffersize,
					errormsg.getString(),
					errormsg.getStringLength());
	} else {
		isc_sql_interprete(sqlcode,errorbuffer,errorbuffersize);
	}

	// set return values
	*errorsize=charstring::getLength(errorbuffer);
	*errorcode=sqlcode;
	*liveconnection=!(charstring::contains(
				errormsg.getString(),
				"Error reading data from the connection") ||
			charstring::contains(
				errormsg.getString(),
				"Error writing data to the connection"));
}

bool firebirdconnection::ping() {

	// call isc_database_info to get page_size and num_buffers,
	// this should always be available unless the db is down
	// if we get an error, then return 0, otherwise return 1
	ISC_STATUS	status[20];
	char		dbitems[]={isc_info_page_size,
					isc_info_num_buffers,
					isc_info_end};
	char		resbuffer[40];

	isc_database_info(status,&db,
				sizeof(dbitems),dbitems,
				sizeof(resbuffer),resbuffer);

	return !(status[0]==1 && status[1]);
}

bool firebirdconnection::selectCatalog(const char *catalog) {

	// keep track of the original db and host
	char	*originaldb=this->database;
	char	*originalhost=this->host;

	// reset the db/host
	this->database=charstring::duplicate(catalog);
	this->host=NULL;

	cont->clearError();

	// log out and log back in to the specified database
	logOut();
	const char	*error=NULL;
	const char	*warning=NULL;
	if (!logIn(&error,&warning)) {

		// Set the error, but don't use the error that was returned
		// from logIn() because it will be confusing.  So, we'll
		// just return the generic SQL Relay error for these kinds of
		// things.
		cont->setError(SQLR_ERROR_DBNOTFOUND_STRING,
				SQLR_ERROR_DBNOTFOUND,true);

		// log back in to the original database, we'll assume that works
		delete[] this->database;
		this->database=originaldb;
		this->host=originalhost;
		logOut();
		logIn(&error,&warning);
		return false;
	}

	// clean up
	delete[] originaldb;
	delete[] originalhost;
	return true;
}

char *firebirdconnection::getCurrentCatalog() {
	return charstring::duplicate(database);
}

const char *firebirdconnection::getDbType() {
	return "firebird";
}

const char *firebirdconnection::getDbVersion() {
	ISC_STATUS	status[20];
	char		dbitems[]={isc_info_version,
					isc_info_end};
	char		resbuffer[256];
	if (!isc_database_info(status,&db,
				sizeof(dbitems),dbitems,
				sizeof(resbuffer),resbuffer)) {

		char	*ptr=resbuffer;

		// first byte is isc_info_version
		ptr++;

		// next 2 bytes are size of the isc_info_version data
		ptr=ptr+sizeof(uint16_t);

		// the next byte is the number of lines of text
		stringbuffer	dbvers;
		char	linecount=*ptr;
		ptr++;
		for (char lineindex=0; lineindex<linecount; lineindex++) {

			// the first byte of each line is the size of the line
			char	linelen=*ptr;
			ptr++;

			// then comes the line of text itself
			if (lineindex) {
				dbvers.append('\n');
			}
			dbvers.append(ptr,linelen);
		}

		delete[] dbversion;
		dbversion=dbvers.detachString();
		return dbversion;
	} 
	return "";
}

const char *firebirdconnection::getDbHostName() {
	return host;
}

const char *firebirdconnection::getCatalogListQuery(const char *catalog) {
	// no good way to get a list of catalogs in firebird
	return "select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim('') as table_name, "
		"	trim('') as table_type, "
		"	trim('') as remarks, "
		"	null "
		"from "
		"	rdb$database "
		"where "
		"	1=0";
}

const char *firebirdconnection::getSchemaListQuery(const char *catalog,
							const char *schema) {

	// firebird has no schemas
	return "select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim('') as table_name, "
		"	trim('') as table_type, "
		"	trim('') as remarks, "
		"	null "
		"from "
		"	rdb$database "
		"where "
		"	1=0";
}

const char *firebirdconnection::getTableTypeListQuery(
						const char *catalog,
						const char *schema,
						const char *tabletypes) {
	tabletypelistquery.clear();

	// select clause
	tabletypelistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim('') as table_name, "
		"	trim(table_type), "
		"	trim('') as remarks, "
		"	null ");

	// from clause
	tabletypelistquery.append(
		"from "
		"(select trim('GLOBAL TEMPORARY') "
			"as table_type from rdb$database "
		"union "
		"select trim('SYSTEM TABLE') "
			"as table_type from rdb$database "
		"union "
		"select trim('TABLE') "
			"as table_type from rdb$database "
		"union "
		"select trim('VIEW') "
			"as table_type from rdb$database) ");

	// where clause
	if (!charstring::isNullOrEmpty(tabletypes)) {
		tabletypelistquery.append(
			"where "
			"	trim(table_type) like upper('");
		tabletypelistquery.append(tabletypes);
		tabletypelistquery.append("') ");
	}

	// order by clause
	tabletypelistquery.append(
		"order by "
		"	table_type");

	return tabletypelistquery.getString();
}

const char *firebirdconnection::getGlobalTempTableListQuery() {
	return "select "
		"	trim(rdb$relation_name) "
		"from "
		"	rdb$relations "
		"where "
		"	rdb$system_flag=0 "
		"	and "
		"	rdb$relation_type=4 ";
}

const char *firebirdconnection::getTableListQuery(const char *catalog,
							const char *schema,
							const char *table,
							uint16_t objecttypes) {
	tablelistquery.clear();

	// select clause
	tablelistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(rdb$relation_name) as table_name, "
		"	trim('TABLE') as table_type, "
		"	trim('') as remarks, "
		"	null ");

	// from clause
	tablelistquery.append(
		"from "
		"	rdb$relations ");

	// where clause
	tablelistquery.append(
		"where "
		"	rdb$system_flag=0 ");
	if (schema) {
		tablelistquery.append(
			"	and "
			"	trim(rdb$owner_name) like upper('");
		tablelistquery.append(schema);
		tablelistquery.append("') ");
	}
	if (table) {
		tablelistquery.append(
			"	and "
			"	trim(rdb$relation_name) like upper('");
		tablelistquery.append(table);
		tablelistquery.append("') ");
	}

	// order by clause
	tablelistquery.append(
		"order by "
		"	rdb$owner_name, "
		"	rdb$relation_name");

	return tablelistquery.getString();
}

static const char	*booltype=
			"select "
			"	trim('BOOLEAN') as type_name, "
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
			"	trim('BOOLEAN') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*smallinttype=
			"select "
			"	trim('SMALLINT') as type_name, "
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
			"	trim('SMALLINT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*inttype=
			"select "
			"	trim('INTEGER') as type_name, "
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
			"	trim('INTEGER') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*biginttype=
			"select "
			"	trim('BIGINT') as type_name, "
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
			"	trim('BIGINT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*numerictype=
			"select "
			"	trim('NUMERIC') as type_name, "
			"	2 as data_type, "
			"	18 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('NUMERIC') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*decimaltype=
			"select "
			"	trim('DECIMAL') as type_name, "
			"	3 as data_type, "
			"	18 as column_size, "
			"	null as literal_prefix, "
			"	null as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	1 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('DECIMAL') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*floattype=
			"select "
			"	trim('FLOAT') as type_name, "
			"	6 as data_type, "
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
			"	trim('FLOAT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*doubleprectype=
			"select "
			"	trim('DOUBLE PRECISION') as type_name, "
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
			"	trim('DOUBLE PRECISION') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*chartype=
			"select "
			"	trim('CHAR') as type_name, "
			"	1 as data_type, "
			"	32767 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('CHAR') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*varchartype=
			"select "
			"	trim('VARCHAR') as type_name, "
			"	12 as data_type, "
			"	32765 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('VARCHAR') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*datetype=
			"select "
			"	trim('DATE') as type_name, "
			"	91 as data_type, "
			"	10 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('DATE') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*timetype=
			"select "
			"	trim('TIME') as type_name, "
			"	92 as data_type, "
			"	8 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('TIME') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*timestamptype=
			"select "
			"	trim('TIMESTAMP') as type_name, "
			"	93 as data_type, "
			"	19 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('TIMESTAMP') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*blobtype=
			"select "
			"	trim('BLOB') as type_name, "
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
			"	trim('BLOB') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

static const char	*blobsubtexttype=
			"select "
			"	trim('BLOB SUB_TYPE TEXT') as type_name, "
			"	-1 as data_type, "
			"	2147483647 as column_size, "
			"	trim('') as literal_prefix, "
			"	trim('') as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	trim('BLOB SUB_TYPE TEXT') as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			"from "
			"	rdb$database ";

const char *firebirdconnection::getTypeInfoListQuery(const char *catalog,
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
			typeinfolistquery.append(biginttype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(numerictype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(decimaltype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(floattype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(doubleprectype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(chartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(datetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(timetype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(timestamptype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(blobtype);
			typeinfolistquery.append("union ");
			typeinfolistquery.append(blobsubtexttype);
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
	} else if (!charstring::compareIgnoringCase(type,"bigint")) {
		return biginttype;
	} else if (!charstring::compareIgnoringCase(type,"numeric")) {
		return numerictype;
	} else if (!charstring::compareIgnoringCase(type,"decimal")) {
		return decimaltype;
	} else if (!charstring::compareIgnoringCase(type,"float")) {
		return floattype;
	} else if (!charstring::compareIgnoringCase(type,"double precision")) {
		return doubleprectype;
	} else if (!charstring::compareIgnoringCase(type,"char")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"character")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"varchar")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"character varying")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"date")) {
		return datetype;
	} else if (!charstring::compareIgnoringCase(type,"time")) {
		return timetype;
	} else if (!charstring::compareIgnoringCase(type,"timestamp")) {
		return timestamptype;
	} else if (!charstring::compareIgnoringCase(type,"blob")) {
		return blobtype;
	} else if (!charstring::compareIgnoringCase(type,"blob sub_type text")) {
		return blobsubtexttype;
	}
	return NULL;
}

const char *firebirdconnection::getColumnListQuery(const char *catalog,
							const char *schema,
							const char *table,
							const char *column) {

	columnlistquery.clear();

	// select clause
	columnlistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(rf.rdb$relation_name) as table_name, "
		"	trim(rf.rdb$field_name) as column_name, "
		"	fd.rdb$field_type as data_type,"
		"	trim(case fd.rdb$field_type "
		"		when 261 then 'BLOB SUB_TYPE BINARY' "
		"		when 14 then 'CHAR' "
		"		when 40 then 'CSTRING' "
		"		when 11 then 'D_FLOAT' "
		"		when 27 then 'DOUBLE PRECISION' "
		"		when 10 then 'FLOAT' "
		"		when 16 then case fd.rdb$field_sub_type "
		"			when 1 then 'NUMERIC' "
		"			when 2 then 'DECIMAL' "
		"			else 'BIGINT' "
		"		end "
		"		when 8 then 'INTEGER' "
		"		when 9 then 'QUAD' "
		"		when 7 then 'SMALLINT' "
		"		when 12 then 'DATE' "
		"		when 13 then 'TIME' "
		"		when 35 then 'TIMESTAMP' "
		"		when 37 then 'VARCHAR' "
		"		else 'UNKNOWN' "
		"	end) as type_name, "
		"	fd.rdb$field_length as column_size, "
		"	fd.rdb$field_length as buffer_length, "
		"	fd.rdb$field_scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	case rf.rdb$null_flag "
		"		when 1 then 0 "
		"		else 1 "
		"	end as nullable, "
		"	trim(case "
		"		when rf.rdb$identity_type is not null "
		"			then 'auto_increment ' || "
		"				coalesce(rf.rdb$description,'') "
		"		else coalesce(rf.rdb$description,'') "
		"	end) as remarks, "
		"	trim(rf.rdb$default_source) as column_default, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	fd.rdb$character_length as char_octet_length, "
		"	rf.rdb$field_position as ordinal_position, "
		"	trim(case rf.rdb$null_flag "
		"		when 1 then 'NO' "
		"		else 'YES' "
		"	end) as is_nullable, "
		"	fd.rdb$field_precision as numeric_precision, "
		"	trim(case ck.key_priority "
		"		when 1 then 'PRI' "
		"		when 2 then 'UNI' "
		"		when 3 then 'MUL' "
		"		else null "
		"	end) as column_key, "
		"	trim(case "
		"		when rf.rdb$identity_type is not null then 'YES' "
		"		else 'NO' "
		"	end) as is_autoincrement, "
		"	null ");

	// from clause
	columnlistquery.append(
		"from "
		"	rdb$relation_fields rf "
		"join "
		"	rdb$relations rl "
		"	on "
		"	rl.rdb$relation_name=rf.rdb$relation_name "
		"left join "
		"	rdb$fields fd "
		"	on "
		"	fd.rdb$field_name=rf.rdb$field_source "
		"left join ( "
		"	select "
		"		rc.rdb$relation_name, "
		"		ix.rdb$field_name, "
		"		min(case rc.rdb$constraint_type "
		"			when 'PRIMARY KEY' then 1 "
		"			when 'UNIQUE' then 2 "
		"			when 'FOREIGN KEY' then 3 "
		"		end) as key_priority "
		"	from "
		"		rdb$relation_constraints rc, "
		"		rdb$index_segments ix "
		"	where "
		"		rc.rdb$index_name=ix.rdb$index_name "
		"		and "
		"		rc.rdb$constraint_type in "
		"			('PRIMARY KEY','UNIQUE','FOREIGN KEY') "
		"	group by "
		"		rc.rdb$relation_name, "
		"		ix.rdb$field_name "
		") ck "
		"on "
		"	rf.rdb$relation_name=ck.rdb$relation_name "
		"	and "
		"	rf.rdb$field_name=ck.rdb$field_name ");

	// where clause
	bool	first=true;
	if (!charstring::isNullOrEmpty(schema)) {
		columnlistquery.append(
			"where "
			"	trim(rl.rdb$owner_name) like upper('");
		columnlistquery.append(schema);
		columnlistquery.append("') ");
		first=false;
	}

	if (!charstring::isNullOrEmpty(table)) {
		if (first) {
			columnlistquery.append("where ");
			first=false;
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	trim(rf.rdb$relation_name) like upper('");
		columnlistquery.append(table);
		columnlistquery.append("') ");
	}

	if (!charstring::isNullOrEmpty(column)) {
		if (first) {
			columnlistquery.append("where ");
			first=false;
		} else {
			columnlistquery.append("	and ");
		}
		columnlistquery.append(
			"	trim(rf.rdb$field_name) like upper('");
		columnlistquery.append(column);
		columnlistquery.append("') ");
	}

	// order by clause
	columnlistquery.append(
		"order by "
		"	rf.rdb$field_position");

	return columnlistquery.getString();
}

const char *firebirdconnection::getPrimaryKeysListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	primarykeyslistquery.clear();

	// select clause
	primarykeyslistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(rc.rdb$relation_name) as table_name, "
		"	trim(isg.rdb$field_name) as column_name, "
		"	isg.rdb$field_position+1 as key_seq, "
		"	trim(rc.rdb$constraint_name) as pk_name, "
		"	null ");

	// from clause
	primarykeyslistquery.append(
		"from "
		"	rdb$relation_constraints rc, "
		"	rdb$index_segments isg ");

	// where clause
	primarykeyslistquery.append(
		"where "
		"	rc.rdb$constraint_type='PRIMARY KEY' "
		"	and "
		"	rc.rdb$index_name=isg.rdb$index_name ");
	if (!charstring::isNullOrEmpty(table)) {
		primarykeyslistquery.append(
			"	and "
			"	trim(rc.rdb$relation_name) like upper('");
		primarykeyslistquery.append(table);
		primarykeyslistquery.append("') ");
	}

	// order by clause
	primarykeyslistquery.append(
		"order by "
		"	rc.rdb$relation_name, "
		"	isg.rdb$field_position");

	return primarykeyslistquery.getString();
}

const char *firebirdconnection::getKeyAndIndexListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	null as table_cat, "
		"	null as table_schem, "
		"	trim(i.rdb$relation_name) as table_name, "
		"	case i.rdb$unique_flag "
		"		when 1 then 0 "
		"		else 1 "
		"	end as non_unique, "
		"	trim('') as index_qualifier, "
		"	trim(i.rdb$index_name) as index_name, "
		"	3 as type, "
		"	isg.rdb$field_position+1 as ordinal_position, "
		"	trim(isg.rdb$field_name) as column_name, "
		"	trim(case i.rdb$index_type "
		"		when 1 then 'D' "
		"		else 'A' "
		"	end) as asc_or_desc, "
		"	i.rdb$statistics as cardinality, "
		"	null as pages, "
		"	null as filter_condition, "
		"	null ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	rdb$indices i, "
		"	rdb$index_segments isg ");

	// where clause
	keyandindexlistquery.append(
		"where "
		"	i.rdb$index_name=isg.rdb$index_name ");
	if (!charstring::isNullOrEmpty(table)) {
		keyandindexlistquery.append(
			"	and "
			"	trim(i.rdb$relation_name) like upper('");
		keyandindexlistquery.append(table);
		keyandindexlistquery.append("') ");
	}

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	i.rdb$relation_name, "
		"	i.rdb$index_name, "
		"	isg.rdb$field_position");

	return keyandindexlistquery.getString();
}

const char *firebirdconnection::getProcedureListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {

	procedurelistquery.clear();

	// select clause
	procedurelistquery.append(
		"select "
		"	null as procedure_cat, "
		"	trim(rdb$owner_name) as procedure_schem, "
		"	trim(rdb$procedure_name) as procedure_name, "
		"	0 as num_input_params, "
		"	0 as num_output_params, "
		"	0 as num_result_sets, "
		"	trim(rdb$description) as remarks, "
		"	trim('1') as procedure_type, "
		"	null ");

	// from clause
	procedurelistquery.append(
		"from "
		"	rdb$procedures ");

	// where clause
	if (!charstring::isNullOrEmpty(schema) ||
		!charstring::isNullOrEmpty(procedure)) {

		bool	first=true;
		procedurelistquery.append("where ");
		if (!charstring::isNullOrEmpty(schema)) {
			procedurelistquery.append(
				"trim(rdb$owner_name) like upper('");
			procedurelistquery.append(schema);
			procedurelistquery.append("') ");
			first=false;
		}
		if (!charstring::isNullOrEmpty(procedure)) {
			if (!first) {
				procedurelistquery.append("and ");
			}
			procedurelistquery.append(
				"trim(rdb$procedure_name) like upper('");
			procedurelistquery.append(procedure);
			procedurelistquery.append("') ");
		}
	}

	// order by clause
	procedurelistquery.append(
		"order by "
		"	rdb$owner_name, "
		"	rdb$procedure_name");

	return procedurelistquery.getString();
}

const char *firebirdconnection::getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {

	procedureparameterlistquery.clear();

	// select clause
	procedureparameterlistquery.append(
		"select "
		"	null as procedure_cat, "
		"	trim('') as procedure_schem, "
		"	trim(pp.rdb$procedure_name) as procedure_name, "
		"	trim(pp.rdb$parameter_name) as column_name, "
		"	case pp.rdb$parameter_type "
		"		when 0 then 1 "
		"		when 1 then 4 "
		"		else 0 "
		"	end as column_type, "
		"	trim('') as data_type, "
		"	trim(case f.rdb$field_type "
		"		when 261 then 'BLOB SUB_TYPE BINARY' "
		"		when 14 then 'CHAR' "
		"		when 40 then 'CSTRING' "
		"		when 11 then 'D_FLOAT' "
		"		when 27 then 'DOUBLE PRECISION' "
		"		when 10 then 'FLOAT' "
		"		when 16 then case f.rdb$field_sub_type "
		"			when 1 then 'NUMERIC' "
		"			when 2 then 'DECIMAL' "
		"			else 'BIGINT' "
		"		end "
		"		when 8 then 'INTEGER' "
		"		when 9 then 'QUAD' "
		"		when 7 then 'SMALLINT' "
		"		when 12 then 'DATE' "
		"		when 13 then 'TIME' "
		"		when 35 then 'TIMESTAMP' "
		"		when 37 then 'VARCHAR' "
		"		else 'UNKNOWN' "
		"	end) as type_name, "
		"	f.rdb$field_length as column_size, "
		"	null as buffer_length, "
		"	f.rdb$field_scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	1 as nullable, "
		"	trim(pp.rdb$description) as remarks, "
		"	trim(pp.rdb$default_source) as column_def, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	f.rdb$character_length as char_octet_length, "
		"	pp.rdb$parameter_number+1 as ordinal_position, "
		"	trim('YES') as is_nullable, "
		"	null ");

	// from clause
	procedureparameterlistquery.append(
		"from "
		"	rdb$procedure_parameters pp "
		"left join "
		"	rdb$fields f "
		"on "
		"	pp.rdb$field_source=f.rdb$field_name ");

	// where clause
	if (!charstring::isNullOrEmpty(procedure)) {
		procedureparameterlistquery.append(
			"where "
			"	trim(pp.rdb$procedure_name) like upper('");
		procedureparameterlistquery.append(procedure);
		procedureparameterlistquery.append("') ");
	}

	// order by clause
	procedureparameterlistquery.append(
		"order by "
		"	pp.rdb$procedure_name, "
		"	pp.rdb$parameter_type desc, "
		"	pp.rdb$parameter_number");

	return procedureparameterlistquery.getString();
}

const char *firebirdconnection::getBindFormat() {
	return "?";
}

const char *firebirdconnection::getNextvalFormat() {
	return "next value for %s";
}

const char *firebirdconnection::getCurrentUserQuery() {
	return "select current_user from rdb$database";
}

const char *firebirdconnection::getLastInsertIdQuery() {
	return lastinsertidquery;
}

const char *firebirdconnection::setIsolationLevelQuery() {
	return "set transaction %s";
}

const char *firebirdconnection::getIsolationLevelQuery() {
	return "select "
		"	case mon$isolation_mode "
		"		when 0 then "
		"cast('snapshot table stability' as varchar(24)) "
		"		when 1 then "
		"cast('snapshot' as varchar(8)) "
		"		when 2 then "
		"cast('read committed' as varchar(14)) "
		"		when 3 then "
		"cast('read committed no record version' as varchar(32)) "
		"		when 4 then "
		"cast('read consistency' as varchar(11)) "
		"	end "
		"from "
		"	mon$transactions "
		"where "
		"	mon$transaction_id=current_transaction";
}

const char *firebirdconnection::mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat) {
	if (fromformat==toformat) {
		return isolevel;
	}
	if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"TRANSACTION_READ_COMMITTED")) {
			return "read committed";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_REPEATABLE_READ")) {
			return "snapshot";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
			return "snapshot table stability";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(
				isolevel,"read committed") ||
			!charstring::compareIgnoringCase(
				isolevel,"read committed no record version") ||
			!charstring::compareIgnoringCase(
				isolevel,"read consistency")) {
			return "TRANSACTION_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot")) {
			return "TRANSACTION_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot table stability")) {
			return "TRANSACTION_SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_COMMITTED")) {
			return "read committed";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_REPEATABLE_READ")) {
			return "snapshot";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_SERIALIZABLE")) {
			return "snapshot table stability";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(
				isolevel,"read committed") ||
			!charstring::compareIgnoringCase(
				isolevel,"read committed no record version") ||
			!charstring::compareIgnoringCase(
				isolevel,"read consistency")) {
			return "SQL_TXN_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot")) {
			return "SQL_TXN_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(
				isolevel,"snapshot table stability")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *firebirdconnection::getDatabaseFeatures() {
	cont->capDatabaseFeatures(databasefeatures);
	return databasefeatures;
}

const char *firebirdconnection::getNoopQuery() {
	return "execute block as begin end";
}

firebirdcursor::firebirdcursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {

	firebirdconn=(firebirdconnection *)conn;

	outsqlda=NULL;
	outsqldabuffer=NULL;
	allocateResultSetBuffers(conn->cont->getMaxColumnCount());

	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	outbindcount=0;

	// set up input binds
	inbindsqlda=(XSQLDA ISC_FAR *)new byte_t[XSQLDA_LENGTH(maxbindcount)];
	inbindsqlda->version=SQLDA_VERSION1;
	inbindsqlda->sqln=maxbindcount;
	inbindts=new ISC_TIMESTAMP[maxbindcount];
	inbindblobid=new ISC_QUAD[maxbindcount];
	inbindblobhandle=new isc_blob_handle[maxbindcount];


	// set up output binds
	outbindsqlda=(XSQLDA ISC_FAR *)new byte_t[XSQLDA_LENGTH(maxbindcount)];
	outbindsqlda->version=SQLDA_VERSION1;
	outbindsqlda->sqln=maxbindcount;
	outbindblobid=new ISC_QUAD[maxbindcount];
	outbindblobhandle=new isc_blob_handle[maxbindcount];
	outbindblobisopen=new bool[maxbindcount];
	outdatebind=new datebind[maxbindcount];

	querytype=0L;
	stmt=0L;

	queryisexecsp=false;
	bindformaterror=false;

	setCreateTempTablePattern("(create|CREATE)[ 	\n\r]+(global|GLOBAL)[ 	\n\r]+(temporary|TEMPORARY)[ 	\n\r]+(table|TABLE)[ 	\n\r]+");
	executeprocedure.setPattern("(execute|EXECUTE)[ 	\n\r]+(procedure|PROCEDURE)");
	executeprocedure.study();
}

firebirdcursor::~firebirdcursor() {
	delete[] inbindsqlda;
	delete[] inbindts;
	delete[] inbindblobid;
	delete[] inbindblobhandle;

	delete[] outbindsqlda;
	delete[] outbindblobid;
	delete[] outbindblobhandle;
	delete[] outbindblobisopen;
	delete[] outdatebind;

	delete[] outsqldabuffer;
	delete[] field;
}

void firebirdcursor::allocateResultSetBuffers(int32_t columncount) {

	delete[] outsqldabuffer;

	if (!columncount) {
		outsqldabuffer=new byte_t[XSQLDA_LENGTH(1)];
		bytestring::zero(outsqldabuffer,XSQLDA_LENGTH(1));
		outsqlda=(XSQLDA ISC_FAR *)outsqldabuffer;
		outsqlda->version=SQLDA_VERSION1;
		outsqlda->sqln=1;
		field=NULL;
	} else {
		outsqldabuffer=new byte_t[XSQLDA_LENGTH(columncount)];
		bytestring::zero(outsqldabuffer,XSQLDA_LENGTH(columncount));
		outsqlda=(XSQLDA ISC_FAR *)outsqldabuffer;
		outsqlda->version=SQLDA_VERSION1;
		outsqlda->sqln=columncount;
		field=new fieldstruct[columncount];
		for (int32_t i=0; i<columncount; i++) {
			field[i].textbuffer=new char[
					conn->cont->getMaxFieldSize()+1];
		}
	}
}

void firebirdcursor::deallocateResultSetBuffers() {

	delete[] outsqldabuffer;
	outsqldabuffer=new byte_t[XSQLDA_LENGTH(1)];
	bytestring::zero(outsqldabuffer,XSQLDA_LENGTH(1));
	outsqlda=(XSQLDA ISC_FAR *)outsqldabuffer;
	outsqlda->version=SQLDA_VERSION1;
	outsqlda->sqln=1;

	delete[] field;
	field=NULL;
}

bool firebirdcursor::prepareQuery(const char *query, uint32_t size) {

	// initialize column count
	outsqlda->sqld=0;

	// are we executing a stored procedure
	queryisexecsp=executeprocedure.match(query);

	// reset the bind format error flag
	bindformaterror=false;

	// free the old statement if it exists
	if (stmt) {
		isc_dsql_free_statement(firebirdconn->error,
						&stmt,DSQL_drop);
		stmt=0L;
	}

	// allocate a cursor handle
	if (isc_dsql_allocate_statement(firebirdconn->error,
					&firebirdconn->db,&stmt)) {
		return false;
	}

	// prepare the cursor
	if (isc_dsql_prepare(firebirdconn->error,
				&firebirdconn->tr,
				&stmt,size,(char *)query,
				firebirdconn->dialect,
				(queryisexecsp)?outbindsqlda:outsqlda)) {
		return false;
	}

	// null output bind sqldata pointers so we can detect
	// which ones were set by outputBind later
	if (queryisexecsp) {
		for (uint16_t i=0; i<outbindsqlda->sqld; i++) {
			outbindsqlda->sqlvar[i].sqldata=NULL;
		}
	}

	// get the cursor type
	char	typeitem[]={isc_info_sql_stmt_type};
	char	resbuffer[1024];
	if (isc_dsql_sql_info(firebirdconn->error,&stmt,
				sizeof(typeitem),typeitem,
				1024,resbuffer)) {
		return false;
	}

	// (modern versions of isc_vax_integer take a const char * parameter,
	// but old versions take char * and this cast works with both)
	ISC_LONG	len=isc_vax_integer((char *)(resbuffer+1),2);
	querytype=isc_vax_integer((char *)(resbuffer+3),len);

	// find bind parameters, if any
	inbindsqlda->sqld=0;
	if (isc_dsql_describe_bind(firebirdconn->error,&stmt,1,inbindsqlda)) {
		return false;
	}
	inbindsqlda->sqln=inbindsqlda->sqld;

	return true;
}

bool firebirdcursor::inputBind(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	inbindsqlda->sqlvar[index].sqltype=SQL_TEXT+1;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=valuesize;
	inbindsqlda->sqlvar[index].sqldata=(char *)value;
	inbindsqlda->sqlvar[index].sqlind=isnull;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBind(const char *variable,
					uint16_t variablesize,
					int64_t *value) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	inbindsqlda->sqlvar[index].sqltype=SQL_INT64;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(int64_t);
	inbindsqlda->sqlvar[index].sqldata=(char *)value;
	inbindsqlda->sqlvar[index].sqlind=(short *)NULL;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBind(const char *variable,
					uint16_t variablesize,
					double *value,
					uint32_t precision,
					uint32_t scale) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	inbindsqlda->sqlvar[index].sqltype=SQL_DOUBLE;
	inbindsqlda->sqlvar[index].sqlscale=scale;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(double);
	inbindsqlda->sqlvar[index].sqldata=(char *)value;
	inbindsqlda->sqlvar[index].sqlind=(short *)NULL;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBind(const char *variable,
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

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}

	// build an ISC_TIMESTAMP
	tm	t;
	t.tm_sec=second;
	t.tm_min=minute;
	t.tm_hour=hour;
	t.tm_mday=day;
	t.tm_mon=month-1;
	t.tm_year=year-1900;
	isc_encode_timestamp(&t,&(inbindts[index]));

	inbindsqlda->sqlvar[index].sqltype=SQL_TIMESTAMP;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(ISC_TIMESTAMP);
	inbindsqlda->sqlvar[index].sqldata=(char *)&(inbindts[index]);
	inbindsqlda->sqlvar[index].sqlind=(short *)NULL;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBindBlob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}

	// create a blob
	bytestring::zero(&inbindblobhandle[index],sizeof(isc_blob_handle));
	if (isc_create_blob2(firebirdconn->error,
				&firebirdconn->db,
				&firebirdconn->tr,
				&inbindblobhandle[index],
				&inbindblobid[index],0,NULL)) {
		return false;
	}

	// write the value to the blob, MAX_LOB_CHUNK_SIZE bytes at a time
	uint32_t	bytesput=0;
	while (bytesput<valuesize) {
		uint16_t	bytestoput=0;
		if (valuesize-bytesput<MAX_LOB_CHUNK_SIZE) {
			bytestoput=valuesize-bytesput;
		} else {
			bytestoput=MAX_LOB_CHUNK_SIZE;
		}
		// (modern versions of isc_put_segment take a const char *
		// parameter, but old versions take char * and this cast works
		// with both)
		if (isc_put_segment(firebirdconn->error,
					&inbindblobhandle[index],
					bytestoput,(char *)(value+bytesput))) {
			return false;
		}
		bytesput=bytesput+bytestoput;
	}

	// close the blob
	isc_close_blob(firebirdconn->error,&inbindblobhandle[index]);

	inbindsqlda->sqlvar[index].sqltype=SQL_BLOB+1;
	inbindsqlda->sqlvar[index].sqlscale=0;
	inbindsqlda->sqlvar[index].sqlsubtype=0;
	inbindsqlda->sqlvar[index].sqllen=sizeof(ISC_QUAD);
	inbindsqlda->sqlvar[index].sqldata=(char *)&inbindblobid[index];
	inbindsqlda->sqlvar[index].sqlind=isnull;
	inbindsqlda->sqlvar[index].sqlname_length=0;
	inbindsqlda->sqlvar[index].sqlname[0]='\0';
	inbindsqlda->sqlvar[index].relname_length=0;
	inbindsqlda->sqlvar[index].relname[0]='\0';
	inbindsqlda->sqlvar[index].ownname_length=0;
	inbindsqlda->sqlvar[index].ownname[0]='\0';
	inbindsqlda->sqlvar[index].aliasname_length=0;
	inbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::inputBindClob(const char *variable,
					uint16_t variablesize,
					const char *value,
					uint32_t valuesize,
					int16_t *isnull) {
	return inputBindBlob(variable,variablesize,
				value,valuesize,isnull);
}

bool firebirdcursor::outputBind(const char *variable, 
				uint16_t variablesize,
				char *value, 
				uint32_t valuesize, 
				int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_TEXT+1;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=valuesize;
	outbindsqlda->sqlvar[index].sqldata=value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value,
				int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_INT64;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(int64_t);
	outbindsqlda->sqlvar[index].sqldata=(char *)value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t *precision,
				uint32_t *scale,
				int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_DOUBLE;
	outbindsqlda->sqlvar[index].sqlscale=*scale;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(double);
	outbindsqlda->sqlvar[index].sqldata=(char *)value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBind(const char *variable,
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

	// store the pointers
	outdatebind[outbindcount].year=year;
	outdatebind[outbindcount].month=month;
	outdatebind[outbindcount].day=day;
	outdatebind[outbindcount].hour=hour;
	outdatebind[outbindcount].minute=minute;
	outdatebind[outbindcount].second=second;
	outdatebind[outbindcount].tz=tz;
	outdatebind[outbindcount].isnegative=isnegative;

	char	*value=(char *)&(outdatebind[outbindcount].buffer);

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}
	outbindsqlda->sqlvar[index].sqltype=SQL_TIMESTAMP;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(ISC_TIMESTAMP);
	outbindsqlda->sqlvar[index].sqldata=value;
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBindBlob(const char *variable,
					uint16_t variablesize,
					uint16_t ind,
					int16_t *isnull) {

	outbindcount++;

	// make bind vars 1 based like all other db's
	long	index=charstring::convertToInteger(variable+1)-1;
	if (index<0) {
		bindformaterror=true;
		return false;
	}

	outbindblobisopen[index]=false;

	outbindsqlda->sqlvar[index].sqltype=SQL_BLOB+1;
	outbindsqlda->sqlvar[index].sqlscale=0;
	outbindsqlda->sqlvar[index].sqlsubtype=0;
	outbindsqlda->sqlvar[index].sqllen=sizeof(ISC_QUAD);
	outbindsqlda->sqlvar[index].sqldata=(char *)&outbindblobid[index];
	outbindsqlda->sqlvar[index].sqlind=isnull;
	outbindsqlda->sqlvar[index].sqlname_length=0;
	outbindsqlda->sqlvar[index].sqlname[0]='\0';
	outbindsqlda->sqlvar[index].relname_length=0;
	outbindsqlda->sqlvar[index].relname[0]='\0';
	outbindsqlda->sqlvar[index].ownname_length=0;
	outbindsqlda->sqlvar[index].ownname[0]='\0';
	outbindsqlda->sqlvar[index].aliasname_length=0;
	outbindsqlda->sqlvar[index].aliasname[0]='\0';
	return true;
}

bool firebirdcursor::outputBindClob(const char *variable,
					uint16_t variablesize,
					uint16_t index,
					int16_t *isnull) {
	return outputBindBlob(variable,variablesize,index,isnull);
}

bool firebirdcursor::getLobOutputBindLength(uint16_t index, uint64_t *length) {

	// open the blob
	outbindblobhandle[index]=0;
	if (isc_open_blob2(firebirdconn->error,
				&firebirdconn->db,
				&firebirdconn->tr,
				&outbindblobhandle[index],
				&outbindblobid[index],0,NULL)) {
		return false;
	}

	bool	retval=true;

	// read blob info
	char	blobitems[]={isc_info_blob_total_length};
	char	resultbuffer[64];
	if (isc_blob_info(firebirdconn->error,
				&outbindblobhandle[index],
				sizeof(blobitems),
				blobitems,
				sizeof(resultbuffer),
				resultbuffer)) {
		retval=false;
	}

	// get the blob length from the result buffer
	for (const char *p=resultbuffer; *p!=isc_info_end;) {

		// get the item type
		char	itemtype=*p;
		p++;

		// get the item length
		// (modern versions of isc_vax_integer take a const char *
		// parameter, but old versions take char * and this cast works
		// with both)
		uint16_t	itemlength=
				(uint16_t)isc_vax_integer((char *)p,2);
		p=p+2;

		// get the lob length
		if (itemtype==isc_info_blob_total_length) {
			// (modern versions of isc_vax_integer take a
			// const char * parameter, but old versions take a
			// char * and this cast works with both)
			*length=isc_vax_integer((char *)p,itemlength);
		}
 
		// move on
		p=p+itemlength;
	}
				
	// close the blob
	isc_close_blob(firebirdconn->error,&outbindblobhandle[index]);

	return retval;
}

bool firebirdcursor::getLobOutputBindSegment(uint16_t index,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {

	// open the blob, if necessary
	if (!outbindblobisopen[index]) {
		outbindblobhandle[index]=0;
		if (isc_open_blob2(firebirdconn->error,
					&firebirdconn->db,
					&firebirdconn->tr,
					&outbindblobhandle[index],
					&outbindblobid[index],0,NULL)) {
			return false;
		}
		outbindblobisopen[index]=true;
	}

	// read a blob segment, at most MAX_LOB_CHUNK_SIZE bytes at a time
	uint64_t	totalbytesread=0;
	uint64_t	bytestoread=0;
	uint64_t	remainingbytestoread=charstoread;
	ISC_STATUS	status=0;
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
		uint16_t	bytesread=0;
		status=isc_get_segment(firebirdconn->error,
					&outbindblobhandle[index],
					&bytesread,
					bytestoread,
					buffer+totalbytesread);

		// bail on error
		if (status && status!=isc_segment) {
			break;
		}

		// update total bytes read
		totalbytesread=totalbytesread+bytesread;

		// bail if we're done reading
		if (bytesread<bytestoread || totalbytesread==charstoread) {
			break;
		}
	}

	// return number of bytes/chars read
	*charsread=totalbytesread;

	return true;
}

void firebirdcursor::closeLobOutputBind(uint16_t index) {

	// close the blob, if necessary
	if (outbindblobisopen[index]) {
		isc_close_blob(firebirdconn->error,&outbindblobhandle[index]);
		outbindblobisopen[index]=false;
	}
}

bool firebirdcursor::executeQuery(const char *query, uint32_t size) {

	// for commit or rollback, execute the API call and return
	if (querytype==isc_info_sql_stmt_commit) {
		return conn->commit();
	} else if (querytype==isc_info_sql_stmt_rollback) {
		return conn->rollback();
	} else if (queryisexecsp) {

		// handle stored procedures...

		// allocate dummy buffers for any unbound output
		// params so isc_dsql_execute2 has somewhere to write
		bool		*isdummy=NULL;
		memorypool	*bindpool=conn->cont->getBindPool(this);
		for (uint16_t i=0; i<outbindsqlda->sqld; i++) {

			if (outbindsqlda->sqlvar[i].sqldata) {
				continue;
			}

			if (!isdummy) {
				isdummy=(bool *)bindpool->allocate(
					sizeof(bool)*outbindsqlda->sqld);
				bytestring::zero(isdummy,
					sizeof(bool)*outbindsqlda->sqld);
			}
			isdummy[i]=true;

			uint16_t	len=outbindsqlda->sqlvar[i].sqllen;
			outbindsqlda->sqlvar[i].sqldata=
					(char *)bindpool->allocate((len)?len:1);
		}

		// execute the stored procedure
		bool	retval=!isc_dsql_execute2(firebirdconn->error,
							&firebirdconn->tr,
							&stmt,1,
							inbindsqlda,
							outbindsqlda);

		// null-out dummy sqldata pointers
		if (isdummy) {
			for (uint16_t i=0; i<outbindsqlda->sqld; i++) {
				if (isdummy[i]) {
					outbindsqlda->sqlvar[i].sqldata=NULL;
				}
			}
		}

		for (uint16_t i=0; i<outbindsqlda->sqld; i++) {

			// skip unregistered output params
			if (!outbindsqlda->sqlvar[i].sqldata) {
				continue;
			}

			if (outbindsqlda->sqlvar[i].
					sqltype==SQL_TEXT+1) {

				// null-terminate strings
				outbindsqlda->sqlvar[i].
					sqldata[outbindsqlda->sqlvar[i].
								sqllen-1]=0;

			} else if (outbindsqlda->sqlvar[i].
					sqltype==SQL_TIMESTAMP) {

				// copy out date bind data
				tm	t;
				isc_decode_timestamp((ISC_TIMESTAMP *)
					outbindsqlda->sqlvar[i].sqldata,&t);
				*(outdatebind[i].year)=t.tm_year+1900;
				*(outdatebind[i].month)=t.tm_mon+1;
				*(outdatebind[i].day)=t.tm_mday;
				*(outdatebind[i].hour)=t.tm_hour;
				*(outdatebind[i].minute)=t.tm_min;
				*(outdatebind[i].second)=t.tm_sec;
				*(outdatebind[i].tz)=NULL;
				*(outdatebind[i].isnegative)=false;
			}
		}
		return retval;
	}

	// handle non-stored procedures...

	// get the max column count and field size
	uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
	uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();

	// check for create temp table query
	if (querytype==isc_info_sql_stmt_ddl) {
		checkForTempTable(query,size);
	}

	if (!maxcolumncount) {
		allocateResultSetBuffers(outsqlda->sqld);
	}

	// describe the cursor
	if (isc_dsql_describe(firebirdconn->error,&stmt,1,outsqlda)) {
		return false;
	}
	if (maxcolumncount && (uint32_t)outsqlda->sqld>maxcolumncount) {
		outsqlda->sqld=maxcolumncount;
	}

	for (uint16_t i=0; i<outsqlda->sqld; i++) {

		// save the actual field type
		field[i].type=outsqlda->sqlvar[i].sqltype;

		// handle the null indicator
		outsqlda->sqlvar[i].sqlind=&field[i].nullindicator;

		// coerce the datatypes and point where the data should go
		if (outsqlda->sqlvar[i].sqltype==SQL_TEXT || 
				outsqlda->sqlvar[i].sqltype==SQL_TEXT+1) {
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			field[i].sqlrtype=CHAR_DATATYPE;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
		} else if (outsqlda->sqlvar[i].sqltype==SQL_VARYING ||
				outsqlda->sqlvar[i].sqltype==SQL_VARYING+1) {
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			field[i].sqlrtype=VARCHAR_DATATYPE;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
		} else if (outsqlda->sqlvar[i].sqltype==SQL_SHORT ||
				outsqlda->sqlvar[i].sqltype==SQL_SHORT+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].shortbuffer;
			field[i].sqlrtype=SMALLINT_DATATYPE;

		// Looks like sometimes firebird returns INT64's as
		// SQL_LONG type.  These can be identified because
		// the sqlscale gets set too.  Treat SQL_LONG's with
		// an sqlscale as INT64's.
		} else if ((outsqlda->sqlvar[i].sqltype==SQL_LONG ||
				outsqlda->sqlvar[i].sqltype==SQL_LONG+1) &&
				!outsqlda->sqlvar[i].sqlscale) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].longbuffer;
			field[i].sqlrtype=INTEGER_DATATYPE;
		} else if (
		#ifdef SQL_INT64
				(outsqlda->sqlvar[i].sqltype==SQL_INT64 ||
				outsqlda->sqlvar[i].sqltype==SQL_INT64+1) ||
		#endif
				((outsqlda->sqlvar[i].sqltype==SQL_LONG ||
				outsqlda->sqlvar[i].sqltype==SQL_LONG+1) &&
				outsqlda->sqlvar[i].sqlscale)) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].int64buffer;
			if (outsqlda->sqlvar[i].sqlsubtype==1) {
				field[i].sqlrtype=NUMERIC_DATATYPE;
			} else {
				field[i].sqlrtype=DECIMAL_DATATYPE;
			}
		} else if (outsqlda->sqlvar[i].sqltype==SQL_FLOAT ||
			outsqlda->sqlvar[i].sqltype==SQL_FLOAT+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].floatbuffer;
			field[i].sqlrtype=FLOAT_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_DOUBLE ||
			outsqlda->sqlvar[i].sqltype==SQL_DOUBLE+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].doublebuffer;
			field[i].sqlrtype=DOUBLE_PRECISION_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_D_FLOAT ||
			outsqlda->sqlvar[i].sqltype==SQL_D_FLOAT+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].doublebuffer;
			field[i].sqlrtype=D_FLOAT_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_ARRAY || 
				outsqlda->sqlvar[i].sqltype==SQL_ARRAY+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].quadbuffer;
			field[i].sqlrtype=ARRAY_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_QUAD || 
				outsqlda->sqlvar[i].sqltype==SQL_QUAD+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].quadbuffer;
			field[i].sqlrtype=QUAD_DATATYPE;
		#ifdef SQL_TIMESTAMP
		} else if (outsqlda->sqlvar[i].sqltype==SQL_TIMESTAMP || 
				outsqlda->sqlvar[i].
					sqltype==SQL_TIMESTAMP+1) {
		#else
		} else if (outsqlda->sqlvar[i].sqltype==SQL_DATE || 
				outsqlda->sqlvar[i].sqltype==SQL_DATE+1) {
		#endif
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].timestampbuffer;
			field[i].sqlrtype=TIMESTAMP_DATATYPE;
		#ifdef SQL_TIMESTAMP
		} else if (outsqlda->sqlvar[i].sqltype==SQL_TYPE_TIME || 
				outsqlda->sqlvar[i].
					sqltype==SQL_TYPE_TIME+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].timebuffer;
			field[i].sqlrtype=TIME_DATATYPE;
		} else if (outsqlda->sqlvar[i].sqltype==SQL_TYPE_DATE || 
				outsqlda->sqlvar[i].
					sqltype==SQL_TYPE_DATE+1) {
			outsqlda->sqlvar[i].sqldata=
					(char *)&field[i].datebuffer;
			field[i].sqlrtype=DATE_DATATYPE;
		#endif
		} else if (outsqlda->sqlvar[i].sqltype==SQL_BLOB ||
				outsqlda->sqlvar[i].sqltype==SQL_BLOB+1) {
			outsqlda->sqlvar[i].sqldata=(char *)&field[i].blobid;
			outsqlda->sqlvar[i].sqllen=sizeof(ISC_QUAD);
			field[i].sqlrtype=BLOB_DATATYPE;
			field[i].blobisopen=false;
		} else {
			outsqlda->sqlvar[i].sqltype=SQL_VARYING;
			outsqlda->sqlvar[i].sqldata=field[i].textbuffer;
			field[i].sqlrtype=UNKNOWN_DATATYPE;
			if ((uint32_t)outsqlda->sqlvar[i].sqllen>
							maxfieldsize) {
				outsqlda->sqlvar[i].sqllen=maxfieldsize;
			}
		}
	}

	// Execute the query
	return !isc_dsql_execute(firebirdconn->error,&firebirdconn->tr,
							&stmt,1,inbindsqlda);
}

void firebirdcursor::getError(char *errorbuffer,
				uint32_t errorbuffersize,
				uint32_t *errorsize,
				int64_t *errorcode,
				bool *liveconnection) {

	// handle bind format errors
	if (bindformaterror) {
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

	// otherwise fall back to default implementation
	sqlrservercursor::getError(errorbuffer,
					errorbuffersize,
					errorsize,
					errorcode,
					liveconnection);
}

void firebirdcursor::checkForTempTable(const char *query, uint32_t size) {

	// see if the query matches the pattern for a temporary query that
	// creates a temporary table
	const char	*ptr=skipCreateTempTableClause(query);
	if (!ptr) {
		return;
	}

	// get the table name
	stringbuffer	tablename;
	const char	*endptr=query+size;
	while (ptr && *ptr && *ptr!=' ' &&
		*ptr!='\n' && *ptr!='	' && ptr<endptr) {
		tablename.append(*ptr);
		ptr++;
	}

	// look for "on commit preserve rows"
	bool	preserverowsoncommit=containsOnCommitPreserveRows(ptr);

	if (firebirdconn->droptemptables) {

		// if "droptemptables" was specified...
		conn->cont->addTempTableForDrop(tablename.getString());

	} else if (preserverowsoncommit) {

		// If "on commit preserve rows" was specified, then when
		// the commit/rollback is executed at the end of the
		// session, the data won't be truncated.  It needs to
		// be though, so we'll set it up to be truncated manually.
		conn->cont->addTempTableForTrunc(tablename.getString());
	}
}

bool firebirdcursor::queryIsNotSelect() {
	return (querytype!=isc_info_sql_stmt_select);
}

bool firebirdcursor::queryIsCommitOrRollback() {
	return (querytype==isc_info_sql_stmt_commit ||
		querytype==isc_info_sql_stmt_rollback);
}

uint64_t firebirdcursor::getAffectedRows() {

	char	infoitems[]={isc_info_sql_records};
	char	resbuffer[256];

	if (isc_dsql_sql_info(firebirdconn->error,&stmt,
				sizeof(infoitems),infoitems,
				sizeof(resbuffer),resbuffer)) {
		return 0;
	}

	uint64_t	affectedrows=0;

	for (const char *p=resbuffer; *p!=isc_info_end;) {

		char	itemtype=*p;
		p++;

		// (modern versions of isc_vax_integer take a const char *
		// parameter, but old versions take char * and this cast
		// works with both)
		uint16_t	itemlength=
				(uint16_t)isc_vax_integer((char *)p,2);
		p=p+2;

		if (itemtype==isc_info_sql_records) {

			// parse sub-items
			const char	*end=p+itemlength;
			while (p<end && *p!=isc_info_end) {

				char	subtype=*p;
				p++;

				uint16_t	sublength=
					(uint16_t)isc_vax_integer(
							(char *)p,2);
				p=p+2;

				uint64_t	count=
					(uint64_t)isc_vax_integer(
							(char *)p,sublength);
				p=p+sublength;

				switch (subtype) {
					case isc_info_req_insert_count:
					case isc_info_req_update_count:
					case isc_info_req_delete_count:
						affectedrows+=count;
						break;
				}
			}
		} else {
			p=p+itemlength;
		}
	}

	return affectedrows;
}

uint32_t firebirdcursor::colCount() {
	// for exec procedure queries, outsqlda contains output bind values
	// rather than column info and there is no result set, thus no column
	// info
	return (queryisexecsp)?0:outsqlda->sqld;
}

const char *firebirdcursor::getColumnName(uint32_t col) {
	return outsqlda->sqlvar[col].aliasname;
}

uint16_t firebirdcursor::getColumnNameSize(uint32_t col) {
	return outsqlda->sqlvar[col].aliasname_length;
}

uint16_t firebirdcursor::getColumnType(uint32_t col) {
	return field[col].sqlrtype;
}

uint32_t firebirdcursor::getColumnSize(uint32_t col) {
	return outsqlda->sqlvar[col].sqllen;
}

uint32_t firebirdcursor::getColumnPrecision(uint32_t col) {

	switch (field[col].sqlrtype) {
		case CHAR_DATATYPE:
			return outsqlda->sqlvar[col].sqllen;
		case VARCHAR_DATATYPE:
			return outsqlda->sqlvar[col].sqllen;
		case SMALLINT_DATATYPE:
			return 5;
		case INTEGER_DATATYPE:
			return 11;
		case NUMERIC_DATATYPE:
			// FIXME: can be from 1 to 18
			// (oddly, scale is given as a negative number)
			return 18+outsqlda->sqlvar[col].sqlscale;
		case DECIMAL_DATATYPE:
			// FIXME: can be from 1 to 18
			// (oddly, scale is given as a negative number)
			return 18+outsqlda->sqlvar[col].sqlscale;
		case FLOAT_DATATYPE:
			return 0;
		case DOUBLE_PRECISION_DATATYPE:
			return 0;
		case D_FLOAT_DATATYPE:
			return 0;
		case ARRAY_DATATYPE:
			// not sure
			return 0;
		case QUAD_DATATYPE:
			// not sure
			return 0;
		case TIMESTAMP_DATATYPE:
			// not sure
			return 0;
		case TIME_DATATYPE:
			return 8;
		case DATE_DATATYPE:
			return 10;
		case BLOB_DATATYPE:
			return outsqlda->sqlvar[col].sqllen;
		default:
			return outsqlda->sqlvar[col].sqllen;
	}
}

uint32_t firebirdcursor::getColumnScale(uint32_t col) {
	return -outsqlda->sqlvar[col].sqlscale;
}

uint16_t firebirdcursor::getColumnIsNullable(uint32_t col) {
	// the low bit of sqltype indicates nullability
	return outsqlda->sqlvar[col].sqltype&1;
}

const char *firebirdcursor::getColumnTable(uint32_t col) {
	return outsqlda->sqlvar[col].relname;
}

uint16_t firebirdcursor::getColumnTableSize(uint32_t col) {
	return outsqlda->sqlvar[col].relname_length;
}

bool firebirdcursor::noRowsToReturn() {
	// for exec procedure queries, outsqlda contains output bind values
	// rather than a result set and there is no result set
	return (queryisexecsp)?true:!outsqlda->sqld;
}

bool firebirdcursor::fetchRow(bool *error) {

	*error=false;

	ISC_STATUS	retcode=isc_dsql_fetch(firebirdconn->error,
							&stmt,1,outsqlda);

	// success
	if (!retcode) {
		return true;
	}

	// no more rows
	if (retcode==100) {
		return false;
	}

	// error
	*error=true;
	return false;
}

void firebirdcursor::getField(uint32_t col,
				const char **fld, uint64_t *fldsize,
				bool *lob, bool *null) {

	// handle a null field
	if ((outsqlda->sqlvar[col].sqltype & 1) && 
			field[col].nullindicator==-1) {

		*null=true;

	} else

	// handle a non-null field
	if (outsqlda->sqlvar[col].sqltype==SQL_TEXT ||
			outsqlda->sqlvar[col].sqltype==SQL_TEXT+1) {

		size_t	maxlen=outsqlda->sqlvar[col].sqllen;
		size_t	reallen=charstring::getLength(field[col].textbuffer);
		if (reallen>maxlen) {
			reallen=maxlen;
		}
		*fld=field[col].textbuffer;
		*fldsize=reallen;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_SHORT ||
			outsqlda->sqlvar[col].
				sqltype==SQL_SHORT+1) {

		*fldsize=charstring::printf(field[col].textbuffer,
						conn->cont->getMaxFieldSize(),
						"%hd",field[col].shortbuffer);
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_FLOAT ||
			outsqlda->sqlvar[col].
				sqltype==SQL_FLOAT+1) {

		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%.4f",(double)field[col].floatbuffer);
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_DOUBLE ||
			outsqlda->sqlvar[col].
				sqltype==SQL_DOUBLE+1 ||
			outsqlda->sqlvar[col].
				sqltype==SQL_D_FLOAT ||
			outsqlda->sqlvar[col].
				sqltype==SQL_D_FLOAT+1) {

		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%.4f",field[col].doublebuffer);
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].
				sqltype==SQL_VARYING ||
			outsqlda->sqlvar[col].
				sqltype==SQL_VARYING+1) {

		// the first 2 bytes are the size in 
		// an SQL_VARYING field
		int16_t	size;
		bytestring::copy((void *)&size,
				(void *)field[col].textbuffer,
				sizeof(int16_t));
		*fld=field[col].textbuffer+sizeof(int16_t);
		*fldsize=size;

	// Looks like sometimes firebird returns INT64's as
	// SQL_LONG type.  These can be identified because
	// the sqlscale gets set too.  Treat SQL_LONG's with
	// an sqlscale as INT64's.
	} else if ((outsqlda->sqlvar[col].
				sqltype==SQL_LONG ||
			outsqlda->sqlvar[col].
				sqltype==SQL_LONG+1) &&
			!outsqlda->sqlvar[col].sqlscale) {

		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%d",field[col].longbuffer);
		*fld=field[col].textbuffer;

	} else if (
	#ifdef SQL_INT64
			(outsqlda->sqlvar[col].
				sqltype==SQL_INT64 ||
			outsqlda->sqlvar[col].
				sqltype==SQL_INT64+1) ||
	#endif
			((outsqlda->sqlvar[col].
				sqltype==SQL_LONG ||
			outsqlda->sqlvar[col].
				sqltype==SQL_LONG+1) &&
			outsqlda->sqlvar[col].sqlscale)) {

		// int64's are weird.  To the left of the decimal
		// point is the value/10^scale, to the right is
		// value%10^scale
		ISC_INT64	v=field[col].int64buffer;
		if (outsqlda->sqlvar[col].sqlscale) {
			ISC_SHORT	scale=-outsqlda->sqlvar[col].sqlscale;
			int		p=(int)pow(10.0,(double)scale);
			*fldsize=charstring::printf(
					field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%lld.%0*lld",
					(int64_t)(v/p),scale,(int64_t)(v%p));
		} else {
			*fldsize=charstring::printf(
					field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%lld",(int64_t)v);
		}
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].sqltype==SQL_ARRAY ||
		outsqlda->sqlvar[col].sqltype==SQL_ARRAY+1 ||
		outsqlda->sqlvar[col].sqltype==SQL_QUAD ||
		outsqlda->sqlvar[col].sqltype==SQL_QUAD+1) {

		// FIXME: handle arrays for real here...
		*null=true;

	#ifdef SQL_TIMESTAMP
	} else if (outsqlda->sqlvar[col].sqltype==SQL_TIMESTAMP ||
		outsqlda->sqlvar[col].sqltype==SQL_TIMESTAMP+1) {

		// decode the timestamp
		tm	entry_timestamp;
		isc_decode_timestamp(&field[col].timestampbuffer,
						&entry_timestamp);
	#else
	} else if (outsqlda->sqlvar[col].sqltype==SQL_DATE ||
		outsqlda->sqlvar[col].sqltype==SQL_DATE+1) {

		// decode the timestamp
		tm	entry_timestamp;
		isc_decode_date(&field[col].timestampbuffer,
						&entry_timestamp);
	#endif

		// build a string of "yyyy-mm-dd hh:mm:ss" format
		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%d-%02d-%02d %02d:%02d:%02d",
					entry_timestamp.tm_year+1900,
					entry_timestamp.tm_mon+1,
					entry_timestamp.tm_mday,
					entry_timestamp.tm_hour,
					entry_timestamp.tm_min,
					entry_timestamp.tm_sec);
		*fld=field[col].textbuffer;

	#ifdef SQL_TIMESTAMP
	} else if (outsqlda->sqlvar[col].sqltype==SQL_TYPE_TIME ||
		outsqlda->sqlvar[col].sqltype==SQL_TYPE_TIME+1) {

		// decode the time
		tm	entry_time;
		isc_decode_sql_time(&field[col].timebuffer,
						&entry_time);
		// build a string of "hh:mm:ss" format
		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%02d:%02d:%02d",
					entry_time.tm_hour,
					entry_time.tm_min,
					entry_time.tm_sec);
		*fld=field[col].textbuffer;

	} else if (outsqlda->sqlvar[col].sqltype==SQL_TYPE_DATE ||
		outsqlda->sqlvar[col].sqltype==SQL_TYPE_DATE+1) {

		// decode the date
		tm	entry_date;
		isc_decode_sql_date(&field[col].datebuffer,
						&entry_date);
		// build a string of "yyyy-mm-dd" format
		*fldsize=charstring::printf(field[col].textbuffer,
					conn->cont->getMaxFieldSize(),
					"%d:%02d:%02d",
					entry_date.tm_year+1900,
					entry_date.tm_mon+1,
					entry_date.tm_mday);
		*fld=field[col].textbuffer;

	#endif
	} else if (outsqlda->sqlvar[col].sqltype==SQL_BLOB ||
			outsqlda->sqlvar[col].sqltype==SQL_BLOB+1) {
		*lob=true;
	}
}

bool firebirdcursor::getLobFieldLength(uint32_t col, uint64_t *length) {

	// ignore non-blobs
	if (field[col].sqlrtype!=BLOB_DATATYPE) {
		return false;
	}

	// open the blob
	field[col].blobhandle=0;
	if (isc_open_blob2(firebirdconn->error,
				&firebirdconn->db,
				&firebirdconn->tr,
				&field[col].blobhandle,
				&field[col].blobid,0,NULL)) {
		return false;
	}

	bool	retval=true;

	// read blob info
	char	blobitems[]={isc_info_blob_total_length};
	char	resultbuffer[64];
	if (isc_blob_info(firebirdconn->error,
				&field[col].blobhandle,
				sizeof(blobitems),
				blobitems,
				sizeof(resultbuffer),
				resultbuffer)) {
		retval=false;
	}

	// get the blob length from the result buffer
	for (const char *p=resultbuffer; *p!=isc_info_end;) {

		// get the item type
		char	itemtype=*p;
		p++;

		// get the item length
		// (modern versions of isc_vax_integer take a const char *
		// parameter, but old versions take a char * and this cast
		// works with both)
		uint16_t	itemlength=
				(uint16_t)isc_vax_integer((char *)p,2);
		p=p+2;

		// get the lob length
		if (itemtype==isc_info_blob_total_length) {
			// (modern versions of isc_vax_integer take a
			// const char * parameter, but old versions take a
			// char * and this cast works with both)
			*length=isc_vax_integer((char *)p,itemlength);
		}
 
		// move on
		p=p+itemlength;
	}

	// close the blob
	isc_close_blob(firebirdconn->error,&field[col].blobhandle);

	return retval;
}

bool firebirdcursor::getLobFieldSegment(uint32_t col,
					char *buffer, uint64_t buffersize,
					uint64_t offset, uint64_t charstoread,
					uint64_t *charsread) {

	// ignore non-blobs
	if (field[col].sqlrtype!=BLOB_DATATYPE) {
		return false;
	}

	// open the blob, if necessary
	if (!field[col].blobisopen) {
		field[col].blobhandle=0;
		if (isc_open_blob2(firebirdconn->error,
					&firebirdconn->db,
					&firebirdconn->tr,
					&field[col].blobhandle,
					&field[col].blobid,0,NULL)) {
			return false;
		}
		field[col].blobisopen=true;
	}

	// read a blob segment, at most MAX_LOB_CHUNK_SIZE bytes at a time
	uint64_t	totalbytesread=0;
	uint64_t	bytestoread=0;
	uint64_t	remainingbytestoread=charstoread;
	ISC_STATUS	status=0;
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
		uint16_t	bytesread=0;
		status=isc_get_segment(firebirdconn->error,
					&field[col].blobhandle,
					&bytesread,
					bytestoread,
					buffer+totalbytesread);

		// bail on error
		if (status && status!=isc_segment) {
			break;
		}

		// update total bytes read
		totalbytesread=totalbytesread+bytesread;

		// bail if we're done reading
		if (bytesread<bytestoread || totalbytesread==charstoread) {
			break;
		}
	}

	// return number of bytes/chars read
	*charsread=totalbytesread;

	return true;
}

void firebirdcursor::closeLobField(uint32_t col) {

	// ignore non-blobs
	if (field[col].sqlrtype!=BLOB_DATATYPE) {
		return;
	}

	// close the blob, if necessary
	if (field[col].blobisopen) {
		isc_close_blob(firebirdconn->error,&field[col].blobhandle);
		field[col].blobisopen=false;
	}
}

void firebirdcursor::closeResultSet() {
	outbindcount=0;
	if (stmt) {
		isc_dsql_free_statement(firebirdconn->error,&stmt,DSQL_close);
	}
	if (!conn->cont->getMaxColumnCount()) {
		deallocateResultSetBuffers();
	}
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_firebirdconnection(
						sqlrservercontroller *cont) {
		return new firebirdconnection(cont);
	}
}
