// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/environment.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/charstring.h>
#include <rudiments/character.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>
#include <rudiments/process.h>

#include <config.h>
#include <datatypes.h>
#include <defines.h>

// default cap on the number of rows that executeQuery() will buffer for a
// query that returns rows and also has output binds - see bufferRows().
// override it with the maxbufferedrows connect string parameter.  a value
// of 0 (including a non-numeric value, which parses as 0) means unlimited,
// consistent with maxcolumncount/maxfieldsize and the other sqlrelay
// config options that use 0 to mean "no cap".
#define DEFAULT_MAX_BUFFERED_ROWS	1000

#ifdef SYBASE_AT_RUNTIME
	#include "sapatruntime.cpp"
#else
	extern "C" {
		#include <ctpublic.h>
	}
#endif

class SQLRSERVER_DLLSPEC sapconnection : public sqlrserverconnection {
	friend class sapcursor;
	public:
		sapconnection(sqlrservercontroller *cont);
		~sapconnection();
	private:
		void		initDatabaseFeatures();
		void		handleConnectString();
		bool		logIn(const char **error, const char **warning);
		const char	*logInError(const char *error, uint16_t stage);
		CS_INT		ctlibVersion(const char *version);
		const char	*ctlibVersionString(CS_INT version);
		sqlrservercursor	*newCursor(uint16_t id);
		void		deleteCursor(sqlrservercursor *curs);
		void		logOut();
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
		const char	*getCurrentUserQuery();
		const char	*getLastInsertIdQuery();
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		const char	*getNoopQuery();
		const char	*getBindFormat();
		const char	*beginTransactionQuery();
		const char	*tempTablePrefix();
		sqlrtxmodel_t	getNativeTransactionModel();
		bool		commit();
		bool		rollback();
		void		getError(char *errorbuffer,
						uint32_t errorbuffersize,
						uint32_t *errorsize,
						int64_t	*errorcode,
						bool *liveconnection);

		CS_CONTEXT	*context;
		CS_LOCALE	*locale;
		CS_CONNECTION	*dbconn;

		const char	*sybase;
		const char	*server;
		const char	*db;
		const char	*charset;
		const char	*language;
		const char	*hostname;
		const char	*packetsize;
		const char	*csversion;

		// maximum number of rows that executeQuery() will buffer for
		// a query that also has output binds - see bufferRows()
		uint64_t	maxbufferedrows;

		bool		dbused;

		// charset expansion factor - see deflateColumnSize()
		uint32_t	csexpansion;

		char		*dbversion;

		static	stringbuffer	errorstring;
		static	int64_t		errorcode;
		static	bool		liveconnection;

		static	CS_RETCODE	csMessageCallback(CS_CONTEXT *ctxt,
						CS_CLIENTMSG *msgp);
		static	CS_RETCODE	clientMessageCallback(CS_CONTEXT *ctxt,
						CS_CONNECTION *cnn,
						CS_CLIENTMSG *msgp);
		static	CS_RETCODE	serverMessageCallback(CS_CONTEXT *ctxt,
						CS_CONNECTION *cnn,
						CS_SERVERMSG *msgp);

		stringbuffer	loginerror;
		stringbuffer	loginwarning;

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

// one batch of buffered rows - see bufferRows().  each batch holds a copy of
// the per-column data/datasize/nullindicator triple that one ct_fetch()
// filled in, for as many rows as that fetch returned.
struct sapresultsetbatch {
	char			**data;
	CS_INT			**datasize;
	CS_SMALLINT		**nullindicator;
	CS_INT			cols;
	CS_INT			rows;
	sapresultsetbatch	*next;
};

struct datebind {
        int16_t         *year;
        int16_t         *month;
        int16_t         *day;
        int16_t         *hour;
        int16_t         *minute;
        int16_t         *second;
        int32_t         *microsecond;
        const char      **tz;
	bool		*isnegative;
};

class SQLRSERVER_DLLSPEC sapcursor : public sqlrservercursor {
	friend class sapconnection;
	private:
		sapcursor(sqlrserverconnection *conn, uint16_t id);
		~sapcursor();
		void		allocateResultSetBuffers(int32_t columncount);
		void		deallocateResultSetBuffers();
		bool		open();
		bool		close();
		bool		prepareQuery(const char *query,
						uint32_t size);
		bool		columnInfoIsValidAfterPrepare();
		void		encodeBlob(stringbuffer *buffer,
						const char *data,
						uint32_t datasize);
		void		decodeBlob(char **data,
						uint32_t *datasize);
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
		bool		executeQuery(const char *query,
						uint32_t size);
		bool		nextResultSet(bool *nextresultsetavailable);
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
		uint16_t	getColumnType(uint32_t col);
		uint32_t	getColumnSize(uint32_t col);
		uint32_t	getColumnPrecision(uint32_t col);
		uint32_t	getColumnScale(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		uint16_t	getColumnIsPartOfKey(uint32_t col);
		uint16_t	getColumnIsUnsigned(uint32_t col);
		uint16_t	getColumnIsBinary(uint32_t col);
		uint16_t	getColumnIsAutoIncrement(uint32_t col);
		bool		noRowsToReturn();
		bool		skipRow(bool *error);
		bool		fetchRow(bool *error);
		void		getField(uint32_t col,
					const char **field,
					uint64_t *fieldsize,
					bool *lob,
					bool *null);
		void		nextRow();
		void		closeResultSet();
		void		discardResults();
		void		discardCursor();
		void		discardDynamic();

		void		chooseCursorName();
		void		chooseDynamicName();

		// auto-generated cursor name, used unless the client
		// supplied one of its own with setCursorName()
		char		*autocursorname;
		size_t		autocursornamesize;

		// the name that the cursor was most recently declared with -
		// either autocursorname or the client-supplied name.  Not
		// owned by this class, don't delete it.
		const char	*cursorname;
		size_t		cursornamesize;

		// name of the ct_dynamic(CS_PREPARE) statement that this
		// cursor prepares parameterized selects as
		char		dynamicname[32];
		size_t		dynamicnamesize;

		void		checkRePrepare();
		bool		prepareDynamic();
		void		setParameterName(const char *variable,
						uint16_t variablesize);
		bool		inputBind(CS_VOID *value,
						CS_INT valuesize,
						CS_SMALLINT indicator);
		bool		parseRpcParams(const char *p);
		void		deflateColumnSize(CS_INT index);
		bool		drainResultSet();
		bool		readResultSets();
		bool		bindResultSetColumns();
		bool		describeResultSetColumns();
		bool		fetchOutputParams();
		bool		bufferRows();
		bool		appendRowBatch(CS_INT rows);
		void		freeRowBatches();

		CS_COMMAND	*languagecmd;
		CS_COMMAND	*cursorcmd;
		CS_COMMAND	*cmd;

		// true between ct_dynamic(CS_PREPARE) and ct_dynamic(CS_DEALLOC)
		bool		dynamicprepared;

		// true when the cursor was declared on a prepared dynamic
		// statement rather than on the select text
		bool		dynamiccursor;

		// true when prepareDynamic()'s CS_DESCRIBE_OUTPUT block
		// populated ncols/column[] with the prepared statement's real
		// output columns - see columnInfoIsValidAfterPrepare()
		bool		outputdescribed;
		CS_INT		results;
		CS_INT		resultstype;
		// executeQuery()'s loop overwrites resultstype as it walks the
		// sequence of result sets that a query generated.  this one
		// remembers the type of the result set that carried the rows
		// (if any), which is the one that the client sees.
		CS_INT		rowresultstype;
		CS_INT		ncols;
		CS_INT		affectedrows;

		CS_INT		rowsread;
		CS_INT		row;
		CS_INT		maxrow;
		CS_INT		totalrows;

		uint16_t	maxbindcount;
		CS_DATAFMT	*parameter;
		uint16_t	paramindex;
		CS_VOID		**inbindvalue;
		CS_INT		*inbinddatasize;
		CS_SMALLINT	*inbindindicator;
		char		**inbindts;
		CS_INT		*outbindtype;
		char		**outbindstrings;
		uint32_t	*outbindstringsizes;
		int64_t		**outbindints;
		double		**outbinddoubles;
		uint32_t	**outbindprecisions;
		uint32_t	**outbindscales;
		datebind	*outbinddates;
		int16_t		**outbindisnulls;
		uint16_t	outbindindex;

		int32_t		columncount;
		CS_DATAFMT	templatecolumn;
		CS_DATAFMT	*column;
		char		**data;
		CS_INT		**datasize;
		CS_SMALLINT	**nullindicator;

		// when a query has output binds, the rows can't be streamed
		// straight to the client - the status and parameter result
		// sets that fill the output binds come after the rows, and the
		// client protocol modules send the output binds before the row
		// data.  so the rows get buffered here during executeQuery()
		// and handed out by fetchRow()/getField() afterward.
		bool			rowsbuffered;
		sapresultsetbatch	*firstbatch;
		sapresultsetbatch	*lastbatch;
		sapresultsetbatch	*currentbatch;
		uint64_t		bufferedrows;

		const char	*query;
		uint32_t	size;
		bool		prepared;
		bool		clean;

		// holds the select text rewritten with positional bind
		// markers - see rewriteBindMarkersToPositional()
		stringbuffer	positionalquerybuffer;

		// true for a "{ X=call proc(...) }" rpc query - X is a
		// reserved bind that carries the proc's RETURN value back to
		// the client, rather than a real rpc parameter (see
		// src/protocols/tds.cpp's namedProc()).  it's populated from
		// the CS_STATUS_RESULT result set in executeQuery(), so
		// outputBind() must not send it to ct_param(), and the
		// CS_PARAM_RESULT handling must skip past it when mapping
		// real output params back to outbind slots.
		bool		hasreturnvalue;

		sapconnection	*sapconn;
};


stringbuffer	sapconnection::errorstring;
int64_t		sapconnection::errorcode;
bool		sapconnection::liveconnection;


sapconnection::sapconnection(sqlrservercontroller *cont) :
					sqlrserverconnection(cont) {
	dbused=false;
	csexpansion=1;
	dbversion=NULL;
	initDatabaseFeatures();
}

sapconnection::~sapconnection() {
	delete[] dbversion;
	delete[] maxconnections;
}

void sapconnection::initDatabaseFeatures() {

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
		"DATA_MANIPULATION,INDEX_DEFINITIONS,PROCEDURE_CALLS,TABLE_DEFINITIONS";

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
		"CREATE_TABLE,TABLE_CONSTRAINT,COLUMN_CONSTRAINT,COLUMN_DEFAULT,COLUMN_COLLATION,CONSTRAINT_NAME_DEFINITION";

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
		"@#$£¥";

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		"";

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		"";

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		"false";

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		"USAGE_ON_DOMAIN,REFERENCES_COLUMN,SELECT_TABLE,"
			"UPDATE_TABLE,UPDATE_COLUMN";

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		"BASIC,BEYOND_SELECT,UNRELATED";

	databasefeatures[FEATURE_IDENTIFIER_CASE_STORAGE]=
		"SENSITIVE";

	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
		"\"";

	databasefeatures[FEATURE_INDEX_KEYWORDS]=
		"ASC,DESC";

	// the native sap odbc driver reports these; freetds reports 0
	databasefeatures[FEATURE_INFO_SCHEMA_VIEWS]=
		"ASSERTIONS,CHARACTER_SETS";

	databasefeatures[FEATURE_INSERTS_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_INSERT_OPERATIONS]=
		"INSERT_LITERALS,INSERT_SEARCHED,SELECT_INTO";

	databasefeatures[FEATURE_ISOLATION_LEVELS]=
		"READ_UNCOMMITTED,READ_COMMITTED,SERIALIZABLE";

	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		"true";

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		"";

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		"true";

	databasefeatures[FEATURE_LOCK_TYPES]=
		"";

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		"255";

	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		"255";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		"16";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		"16";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		"16";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		"0";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_TABLE]=
		"250";

	databasefeatures[FEATURE_MAX_COLUMN_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_CONNECTIONS]=maxconnections;

	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_IDENTIFIER_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_INDEX_LENGTH]=
		"255";

	databasefeatures[FEATURE_MAX_PROCEDURE_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_ROW_SIZE]=
		"1962";

	databasefeatures[FEATURE_MAX_SCHEMA_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_STATEMENTS]=
		"0";

	databasefeatures[FEATURE_MAX_STATEMENT_LENGTH]=
		"0";

	databasefeatures[FEATURE_MAX_TABLES_IN_SELECT]=
		"256";

	databasefeatures[FEATURE_MAX_TABLE_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_USER_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_NEED_LONG_DATA_LENGTH]=
		"false";

	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		"true";

	databasefeatures[FEATURE_NULL_SORT_ORDER]=
		"LOW";

	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		"abs,acos,asin,atan,atan2,ceiling,cos,cot,degrees,exp,floor,"
				"log,log10,pi,power,radians,rand,round,sign,sin,sqrt,tan";

	databasefeatures[FEATURE_OPEN_CURSORS_ACROSS]=
		"COMMIT,ROLLBACK";

	databasefeatures[FEATURE_OPEN_STATEMENTS_ACROSS]=
		"COMMIT,ROLLBACK";

	databasefeatures[FEATURE_OTHERS_DELETES_ARE_VISIBLE]=
		"FORWARD_ONLY";

	databasefeatures[FEATURE_OTHERS_INSERTS_ARE_VISIBLE]=
		"FORWARD_ONLY";

	databasefeatures[FEATURE_OTHERS_UPDATES_ARE_VISIBLE]=
		"FORWARD_ONLY";

	databasefeatures[FEATURE_OUTER_JOINS]=
		"BASIC,LIMITED";

	databasefeatures[FEATURE_OWN_DELETES_ARE_VISIBLE]=
		"FORWARD_ONLY";

	databasefeatures[FEATURE_OWN_INSERTS_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OWN_UPDATES_ARE_VISIBLE]=
		"FORWARD_ONLY";

	databasefeatures[FEATURE_PREDICATES]=
		"BETWEEN,COMPARISON,EXISTS,IN,"
			"ISNOTNULL,ISNULL,LIKE,"
			"QUANTIFIED_COMPARISON";

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		"stored procedure";

	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		"SENSITIVE";

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		"CROSS_JOIN,FULL_OUTER_JOIN,INNER_JOIN,"
			"LEFT_OUTER_JOIN,RIGHT_OUTER_JOIN,UNION_JOIN";

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		"FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,"
				"SCROLL_INSENSITIVE/READ_ONLY";

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		"HOLD_CURSORS_OVER_COMMIT,CLOSE_CURSORS_AT_COMMIT";

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE";

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		"CASCADE,DELETE_TABLE,SELECT_TABLE,"
			"UPDATE_TABLE";

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		"ROWID_UNSUPPORTED";

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		"VALUE_EXPRESSION,NULL,DEFAULT,ROW_SUBQUERY";

	databasefeatures[FEATURE_SCHEMA_TERM]=
		"owner";

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		"DATA_MANIPULATION,INDEX_DEFINITIONS,PROCEDURE_CALLS,TABLE_DEFINITIONS";

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		"";

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		"\\";

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		"MINIMUM";

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		"ARITH_OVERFLOW,BREAK,BROWSE,BULK,CHAR_CONVERT,CHECKPOINT,"
				"CLUSTERED,COMPRESSED,COMPUTE,CONFIRM,CONTROLROW,COUNT_BIG,"
				"DATABASE,DBCC,DECRYPT,DECRYPT_DEFAULT,DETERMINISTIC,DISK,"
				"DUAL_CONTROL,DUMMY,DUMP,ENCRYPT,ENDTRAN,ERRLVL,ERRORDATA,"
				"ERROREXIT,EXCLUSIVE,EXIT,EXP_ROW_SIZE,FILLFACTOR,HOLDLOCK,"
				"IDENTITY_GAP,IDENTITY_START,IF,INDEX,INOUT,INSTALL,JAR,KILL,"
				"LINENO,LOAD,LOB_COMPRESSION,LOCK,MANAGE,MATERIALIZED,"
				"MAX_ROWS_PER_PAGE,MIRROR,MIRROREXIT,MODIFY,NOHOLDLOCK,"
				"NONCLUSTERED,NUMERIC_TRUNCATION,OFF,OFFSETS,ONCE,ONLINE,OUT,"
				"OVER,PARTITION,PERM,PERMANENT,PLAN,PRINT,PROC,PROCESSEXIT,"
				"PROXY_TABLE,QUIESCE,RAISERROR,READPAST,READTEXT,RECONFIGURE,"
				"RELEASE_LOCKS_ON_CLOSE,REMOVE,REORG,REPLACE,REPLICATION,"
				"RESERVEPAGEGAP,RETURN,RETURNS,ROLE,ROWCOUNT,RULE,SAVE,"
				"SEMI_SENSITIVE,SETUSER,SHARED,SHUTDOWN,STATISTICS,STRINGSIZE,"
				"STRIPE,SYB_IDENTITY,SYB_RESTREE,SYB_TERMINATE,TEMP,TEXTSIZE,"
				"TRACEFILE,TRAN,TRIGGER,TRUNCATE,TSEQUAL,UNPARTITION,USE,"
				"USER_OPTION,WAITFOR,WHILE,WRITETEXT,XMLEXTRACT,XMLPARSE,"
				"XMLTEST";

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		"2";

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		"PROCEDURES";

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		"ascii,char,char_length,character_length,concat,difference,"
				"insert,length,lcase,ltrim,octet_length,position,repeat,right,"
				"rtrim,soundex,space,substring,ucase";

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
		"true";

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
		"true";

	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		"true";

	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		"database,ifnull,user,convert";

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		"BASIC";

	databasefeatures[FEATURE_TABLE_TERM]=
		"table";

	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		"FRAC_SECOND,SECOND,MINUTE,HOUR,"
			"DAY,WEEK,MONTH,QUARTER,YEAR";

	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		"FRAC_SECOND,SECOND,MINUTE,HOUR,"
			"DAY,WEEK,MONTH,QUARTER,YEAR";

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		"curdate,curtime,current_date,current_time,current_timestamp,"
				"dayname,dayofmonth,dayofweek,dayofyear,extract,hour,minute,"
				"month,monthname,now,quarter,second,timestampadd,"
				"timestampdiff,week,year";

	databasefeatures[FEATURE_TIME_DATE_LITERALS]=
		"";

	databasefeatures[FEATURE_TRANSACTION_DDL_DML]=
		"";

	databasefeatures[FEATURE_UNION_CLAUSES]=
		"UNION,UNION_ALL";

	databasefeatures[FEATURE_UPDATES_ARE_DETECTED]=
		"";

	databasefeatures[FEATURE_VALUE_EXPRESSIONS]=
		"CASE,CAST,COALESCE,NULLIF";

	databasefeatures[FEATURE_WHERE_CURRENT_OF_OPERATIONS]=
		"DELETE,UPDATE";

	databasefeatures[FEATURE_SUPPORTS_SET_CURSOR_NAME]=
		"true";

}


void sapconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	sybase=cont->getConnectStringValue("sybase");
	server=cont->getConnectStringValue("server");
	db=cont->getConnectStringValue("db");
	charset=cont->getConnectStringValue("charset");
	language=cont->getConnectStringValue("language");
	hostname=cont->getConnectStringValue("hostname");
	packetsize=cont->getConnectStringValue("packetsize");
	csversion=cont->getConnectStringValue("csversion");

	// if unset, use the default cap.  if set, use whatever value it
	// parses to - including 0 for either an explicit "0" or a
	// non-numeric value - since bufferRows() treats 0 as unlimited.
	const char	*mbr=cont->getConnectStringValue("maxbufferedrows");
	maxbufferedrows=(charstring::isNullOrEmpty(mbr))?
				DEFAULT_MAX_BUFFERED_ROWS:
				charstring::convertToUnsignedInteger(mbr);

	if (cont->getMaxColumnCount()==1) {
		// if max column count is set to 1 then force it
		// to 2 so the db version detection doesn't crash
		cont->setMaxColumnCount(2);
	}
}

CS_INT sapconnection::ctlibVersion(const char *version) {
	#ifdef CS_VERSION_100
	if (!charstring::compare(version,"100")) {
		return CS_VERSION_100;
	}
	#endif
	#ifdef CS_VERSION_110
	if (!charstring::compare(version,"110")) {
		return CS_VERSION_110;
	}
	#endif
	#ifdef CS_VERSION_120
	if (!charstring::compare(version,"120")) {
		return CS_VERSION_120;
	}
	#endif
	#ifdef CS_VERSION_125
	if (!charstring::compare(version,"125")) {
		return CS_VERSION_125;
	}
	#endif
	#ifdef CS_VERSION_150
	if (!charstring::compare(version,"150")) {
		return CS_VERSION_150;
	}
	#endif
	#ifdef CS_VERSION_155
	if (!charstring::compare(version,"155")) {
		return CS_VERSION_155;
	}
	#endif
	#ifdef CS_VERSION_157
	if (!charstring::compare(version,"157")) {
		return CS_VERSION_157;
	}
	#endif
	#ifdef CS_VERSION_160
	if (!charstring::compare(version,"160")) {
		return CS_VERSION_160;
	}
	#endif
	return 0;
}


const char *sapconnection::ctlibVersionString(CS_INT version) {
	#ifdef CS_VERSION_160
	if (version==CS_VERSION_160) {
		return "160";
	}
	#endif
	#ifdef CS_VERSION_157
	if (version==CS_VERSION_157) {
		return "157";
	}
	#endif
	#ifdef CS_VERSION_155
	if (version==CS_VERSION_155) {
		return "155";
	}
	#endif
	#ifdef CS_VERSION_150
	if (version==CS_VERSION_150) {
		return "150";
	}
	#endif
	#ifdef CS_VERSION_125
	if (version==CS_VERSION_125) {
		return "125";
	}
	#endif
	#ifdef CS_VERSION_110
	if (version==CS_VERSION_110) {
		return "110";
	}
	#endif
	#ifdef CS_VERSION_100
	if (version==CS_VERSION_100) {
		return "100";
	}
	#endif
	return "unknown";
}

bool sapconnection::logIn(const char **error, const char **warning) {

	// set sybase
	if (!charstring::isNullOrEmpty(sybase) &&
			!environment::setValue("SYBASE",sybase)) {
		*error=logInError(
			"Failed to set SYBASE environment variable.",1);
		return false;
	}

	// set server
	if (!charstring::isNullOrEmpty(server) &&
			!environment::setValue("DSQUERY",server)) {
		*error=logInError(
			"Failed to set DSQUERY environment variable.",2);
		return false;
	}

	#ifdef SYBASE_AT_RUNTIME
	if (!loadLibraries(&loginerror)) {
		*error=loginerror.getString();
		return false;
	}
	#endif

	// try client-library versions newest to oldest.  older versions
	// support fewer features (eg. CS_VERSION_100 caps blobs at 255
	// bytes), but older client libraries reject the newer versions.
	CS_INT		versions[8];
	uint16_t	versioncount=0;
	#ifdef CS_VERSION_160
	versions[versioncount++]=CS_VERSION_160;
	#endif
	#ifdef CS_VERSION_157
	versions[versioncount++]=CS_VERSION_157;
	#endif
	#ifdef CS_VERSION_155
	versions[versioncount++]=CS_VERSION_155;
	#endif
	#ifdef CS_VERSION_150
	versions[versioncount++]=CS_VERSION_150;
	#endif
	#ifdef CS_VERSION_125
	versions[versioncount++]=CS_VERSION_125;
	#endif
	#ifdef CS_VERSION_110
	versions[versioncount++]=CS_VERSION_110;
	#endif
	#ifdef CS_VERSION_100
	versions[versioncount++]=CS_VERSION_100;
	#endif

	// if a version was requested, start the walk there (skip newer)
	CS_INT		requested=(charstring::isNullOrEmpty(csversion))?
						0:ctlibVersion(csversion);

	// use the first version that both calls accept
	context=(CS_CONTEXT *)NULL;
	CS_INT		usedversion=0;
	for (uint16_t i=0; i<versioncount; i++) {
		if (requested && versions[i]>requested) {
			continue;
		}
		if (cs_ctx_alloc(versions[i],&context)!=CS_SUCCEED) {
			context=(CS_CONTEXT *)NULL;
			continue;
		}
		if (ct_init(context,versions[i])!=CS_SUCCEED) {
			cs_ctx_drop(context);
			context=(CS_CONTEXT *)NULL;
			continue;
		}
		usedversion=versions[i];
		break;
	}
	if (!usedversion) {

		// An unrecognized locale fails here rather than at cs_locale,
		// and the client library's own diagnostic goes to stderr
		// before CS_MESSAGE_CB exists, so naming the locale is all
		// that can be done from here.
		stringbuffer	ctxerror;
		ctxerror.append("Failed to allocate/initialize "
						"a context structure");
		const char	*locname=cont->getConnectStringValue("lang");
		if (charstring::isNullOrEmpty(locname)) {
			locname=environment::getValue("LANG");
		}
		if (!charstring::isNullOrEmpty(locname)) {
			ctxerror.append(" (the locale ")->append(locname)->
				append(" may not be one the client library "
					"knows - it wrote the details to "
					"stderr)");
		}

		*error=logInError(ctxerror.getString(),2);
		return false;
	}

	// warn if a numeric version was requested but isn't the one used.
	// a non-numeric value (eg. "current") means "newest available".
	if (!charstring::isNullOrEmpty(csversion) &&
			charstring::isInteger(csversion) &&
			usedversion!=requested) {
		loginwarning.clear();
		loginwarning.append("csversion ")->append(csversion)->
			append(" not supported, falling back to ")->
			append(ctlibVersionString(usedversion));
		*warning=loginwarning.getString();
	}


	// configure the error handling callbacks
	if (cs_config(context,CS_SET,CS_MESSAGE_CB,
		(CS_VOID *)sapconnection::csMessageCallback,CS_UNUSED,
			(CS_INT *)NULL)
			!=CS_SUCCEED) {
		*error=logInError(
			"Failed to set a cslib error message callback",4);
		return false;
	}
	if (ct_callback(context,NULL,CS_SET,CS_CLIENTMSG_CB,
		(CS_VOID *)sapconnection::clientMessageCallback)
			!=CS_SUCCEED) {
		*error=logInError(
			"Failed to set a client error message callback",4);
		return false;
	}
	if (ct_callback(context,NULL,CS_SET,CS_SERVERMSG_CB,
		(CS_VOID *)sapconnection::serverMessageCallback)
			!=CS_SUCCEED) {
		*error=logInError(
			"Failed to set a server error message callback",4);
		return false;
	}


	// allocate a connection
	if (ct_con_alloc(context,&dbconn)!=CS_SUCCEED) {
		*error=logInError(
			"Failed to allocate a connection structure",4);
		return false;
	}


	// set the user to use
	const char	*user=cont->getLoginUser();
	if (ct_con_props(dbconn,CS_SET,CS_USERNAME,
		(CS_VOID *)((!charstring::isNullOrEmpty(user))?user:""),
		(CS_INT)charstring::getLength(user),
		(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the user",5);
		return false;
	}


	// set the password to use
	const char	*password=cont->getLoginPassword();
	if (ct_con_props(dbconn,CS_SET,CS_PASSWORD,
		(CS_VOID *)((!charstring::isNullOrEmpty(password))?password:""),
		(CS_INT)charstring::getLength(password),
		(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the password",5);
		return false;
	}

	// set application name
	if (ct_con_props(dbconn,CS_SET,CS_APPNAME,
		(CS_VOID *)"sqlrelay",8,
		(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the application name",5);
		return false;
	}

	// set hostname
	if (!charstring::isNullOrEmpty(hostname) &&
		ct_con_props(dbconn,CS_SET,CS_HOSTNAME,
			(CS_VOID *)hostname,
			(CS_INT)charstring::getLength(hostname),
			(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the hostname",5);
		return false;
	}

	// set packetsize
	uint16_t	ps=charstring::convertToInteger(packetsize);
	if (!charstring::isNullOrEmpty(packetsize) &&
		ct_con_props(dbconn,CS_SET,CS_PACKETSIZE,
			(CS_VOID *)&ps,sizeof(ps),
			(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the packetsize",5);
		return false;
	}

	#ifdef CS_SEC_ENCRYPTION
	CS_INT	enc=CS_TRUE;
	if (ct_con_props(dbconn,CS_SET,CS_SEC_ENCRYPTION,
			(CS_VOID *)&enc,CS_UNUSED,
			(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to enable password encryption",5);
		return false;
	}
	#endif

	// init locale
	locale=NULL;
	if (cs_loc_alloc(context,&locale)!=CS_SUCCEED) {
		*error=logInError("Failed to allocate a locale structure",5);
		return false;
	}
	if (cs_locale(context,CS_SET,locale,CS_LC_ALL,(CS_CHAR *)NULL,
			CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to initialize a locale structure",6);
		return false;
	}

	// set language
	if (!charstring::isNullOrEmpty(language) &&
		cs_locale(context,CS_SET,locale,CS_SYB_LANG,
			(CS_CHAR *)language,
			(CS_INT)charstring::getLength(language),
			(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the language",6);
		return false;
	}

	// set charset
	if (!charstring::isNullOrEmpty(charset) &&
		cs_locale(context,CS_SET,locale,CS_SYB_CHARSET,
			(CS_CHAR *)charset,
			(CS_INT)charstring::getLength(charset),
			(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the charset",6);
		return false;
	}

	// set locale
	if (ct_con_props(dbconn,CS_SET,CS_LOC_PROP,(CS_VOID *)locale,
				CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the locale",6);
		return false;
	}

	// connect to the database
	if (ct_connect(dbconn,(CS_CHAR *)NULL,(CS_INT)0)!=CS_SUCCEED) {
		*error=logInError("Failed to connect to the database",6);
		return false;
	}

	// If the password has expired then the db may allow the login
	// but every query will fail.  "ping" the db here to see if we get
	// that error or not.
	//
	// The ping asks for a 1-character column because it doubles as a
	// charset calibration.  Once @@client_csexpansion is above 1, Open
	// Client reports character columns at their byte width rather than
	// their declared width, and no ctlib call reports
	// @@client_csexpansion directly, so a 1-character column's reported
	// size is the factor - see sapcursor::deflateColumnSize().  This
	// mirrors freetdsconnection::logIn()'s identical calibration.
	bool	retval=true;
	CS_COMMAND	*cmd;
	if (ct_cmd_alloc(dbconn,&cmd)!=CS_SUCCEED) {
		*error=logInError("Failed to allocate ping command",6);
		return false;
	}
	const char	*ping="select convert(varchar(1),'x')";
	if (ct_command(cmd,CS_LANG_CMD,(CS_CHAR *)ping,
					charstring::getLength(ping),
					CS_UNUSED)!=CS_SUCCEED) {
		*error=logInError("Failed to create ping command",6);
		return false;
	}
	if (ct_send(cmd)!=CS_SUCCEED) {
		*error=logInError("Failed to send ping command",6);
		return false;
	}
	CS_INT	resultstype;
	if (ct_results(cmd,&resultstype)==CS_FAIL || resultstype==CS_CMD_FAIL) {
		*error=logInError(NULL,6);
		retval=false;
	} else if (resultstype==CS_ROW_RESULT) {
		CS_DATAFMT	fmt;
		(CS_VOID)bytestring::zero(&fmt,sizeof(fmt));
		if (ct_describe(cmd,1,&fmt)==CS_SUCCEED && fmt.maxlength>0) {
			csexpansion=(uint32_t)fmt.maxlength;
		}
	}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);
	ct_cmd_drop(cmd);

	return retval;
}

const char *sapconnection::logInError(const char *error, uint16_t stage) {

	loginerror.clear();
	if (error) {
		loginerror.append(error);
	}
	if (errorstring.getSize()) {
		if (error) {
			loginerror.append(": ");
		}
		loginerror.append(errorstring.getString(),
					errorstring.getSize());
	}

	if (stage>5) {
		cs_loc_drop(context,locale);
	}
	if (stage>4) {
		ct_con_drop(dbconn);
	}
	if (stage>3) {
		ct_exit(context,CS_UNUSED);
	}
	if (stage>2) {
		cs_ctx_drop(context);
	}

	return loginerror.getString();
}

sqlrservercursor *sapconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new sapcursor(
					(sqlrserverconnection *)this,id);
}

void sapconnection::deleteCursor(sqlrservercursor *curs) {
	delete (sapcursor *)curs;
}

void sapconnection::logOut() {
	cs_loc_drop(context,locale);
	ct_close(dbconn,CS_UNUSED);
	ct_con_drop(dbconn);
	ct_exit(context,CS_UNUSED);
	cs_ctx_drop(context);
}

const char *sapconnection::getDbType() {
	return "sap";
}

const char *sapconnection::getDbVersion() {
	return dbversion;
}

const char *sapconnection::getDbHostNameQuery() {
	return "select asehostname()";
}

const char *sapconnection::getCatalogListQuery(const char *catalog) {

	cataloglistquery.clear();

	// select clause
	cataloglistquery.append(
		"select "
		"	name as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	cataloglistquery.append(
		"from "
		"	master..sysdatabases ");

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
		"	name");

	return cataloglistquery.getString();
}

const char *sapconnection::getSchemaListQuery(const char *catalog,
						const char *schema) {

	schemalistquery.clear();

	// select clause
	schemalistquery.append(
		"select distinct "
		"	db_name() as table_cat, "
		"	user_name(uid) as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	schemalistquery.append(
		"from "
		"	sysobjects ");

	// where clause
	schemalistquery.append(
		"where "
		"	user_name(uid) is not null ");
	if (schema) {
		schemalistquery.append(
			"	and "
			"	user_name(uid) like '");
		schemalistquery.append(schema);
		schemalistquery.append("' ");
	}

	// order by clause
	schemalistquery.append(
		"order by "
		"	user_name(uid)");

	return schemalistquery.getString();
}

const char *sapconnection::getTableTypeListQuery(const char *catalog,
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
		"(select 'SYSTEM TABLE' as table_type "
		"union "
		"select 'TABLE' as table_type "
		"union "
		"select 'VIEW' as table_type) dt ");

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

const char *sapconnection::getTableListQuery(const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes) {

	tablelistquery.clear();

	// select clause
	tablelistquery.append(
		"select "
		"	db_name() as table_cat, "
		"	user_name(uid) as table_schem, "
		"	name as table_name, "
		"	'TABLE' as table_type, "
		"	'' as remarks, "
		"	null ");

	// from clause
	tablelistquery.append(
		"from "
		"	sysobjects ");

	// where clause
	tablelistquery.append(
		"where "
		"	user_name(uid) is not null ");
	if (!charstring::isNullOrEmpty(catalog)) {
		tablelistquery.append(
			"	and "
			"	db_name() like '");
		tablelistquery.append(catalog);
		tablelistquery.append("' ");
	}
	if (schema) {
		tablelistquery.append(
			"	and "
			"	user_name(uid) like '");
		tablelistquery.append(schema);
		tablelistquery.append("' ");
	}
	if (table) {
		tablelistquery.append(
			"	and "
			"	name like '");
		tablelistquery.append(table);
		tablelistquery.append("' ");
	}
	tablelistquery.append(
		"	and ");
	tablelistquery.append("	(");
	bool	first=true;
	if (objecttypes&DB_OBJECT_TABLE) {
		tablelistquery.append("	type='U' ");
		first=false;
	}
	if (objecttypes&DB_OBJECT_VIEW) {
		if (!first) {
			tablelistquery.append("	or ");
		}
		tablelistquery.append("	type='V' ");
	}
	tablelistquery.append(") ");

	// order by clause
	tablelistquery.append(
		"order by "
		"	name");

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
			"	1 as unsigned_attribute, "
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

static const char	*imagetype=
			"select "
			"	'IMAGE' as type_name, "
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
			"	'IMAGE' as local_type_name, "
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

static const char	*numerictype=
			"select "
			"	'NUMERIC' as type_name, "
			"	2 as data_type, "
			"	38 as column_size, "
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

static const char	*decimaltype=
			"select "
			"	'DECIMAL' as type_name, "
			"	3 as data_type, "
			"	38 as column_size, "
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
			"	23 as column_size, "
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

static const char	*smalldatetimetype=
			"select "
			"	'SMALLDATETIME' as type_name, "
			"	93 as data_type, "
			"	16 as column_size, "
			"	'''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	0 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'SMALLDATETIME' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*moneytype=
			"select "
			"	'MONEY' as type_name, "
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
			"	'MONEY' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*smallmoneytype=
			"select "
			"	'SMALLMONEY' as type_name, "
			"	2 as data_type, "
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
			"	'SMALLMONEY' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*unichartype=
			"select "
			"	'UNICHAR' as type_name, "
			"	-15 as data_type, "
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
			"	'UNICHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*univarchartype=
			"select "
			"	'UNIVARCHAR' as type_name, "
			"	-9 as data_type, "
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
			"	'UNIVARCHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*unitexttype=
			"select "
			"	'UNITEXT' as type_name, "
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
			"	'UNITEXT' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

const char *sapconnection::getTypeInfoListQuery(const char *catalog,
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
			typeinfolistquery.append(imagetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(varbinarytype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(binarytype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(texttype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(chartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(numerictype);
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
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(datetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(timetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(datetimetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(smalldatetimetype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(moneytype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(smallmoneytype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(unichartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(univarchartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(unitexttype);
			typeinfolistquery.append(")");
		}
		return typeinfolistquery.getString();
	} else if (!charstring::compareIgnoringCase(type,"bit")) {
		return bittype;
	} else if (!charstring::compareIgnoringCase(type,"tinyint")) {
		return tinyinttype;
	} else if (!charstring::compareIgnoringCase(type,"bigint")) {
		return biginttype;
	} else if (!charstring::compareIgnoringCase(type,"image")) {
		return imagetype;
	} else if (!charstring::compareIgnoringCase(type,"varbinary")) {
		return varbinarytype;
	} else if (!charstring::compareIgnoringCase(type,"binary")) {
		return binarytype;
	} else if (!charstring::compareIgnoringCase(type,"text")) {
		return texttype;
	} else if (!charstring::compareIgnoringCase(type,"char")) {
		return chartype;
	} else if (!charstring::compareIgnoringCase(type,"numeric")) {
		return numerictype;
	} else if (!charstring::compareIgnoringCase(type,"decimal")) {
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
	} else if (!charstring::compareIgnoringCase(type,"varchar")) {
		return varchartype;
	} else if (!charstring::compareIgnoringCase(type,"date")) {
		return datetype;
	} else if (!charstring::compareIgnoringCase(type,"time")) {
		return timetype;
	} else if (!charstring::compareIgnoringCase(type,"datetime")) {
		return datetimetype;
	} else if (!charstring::compareIgnoringCase(type,"smalldatetime")) {
		return smalldatetimetype;
	} else if (!charstring::compareIgnoringCase(type,"money")) {
		return moneytype;
	} else if (!charstring::compareIgnoringCase(type,"smallmoney")) {
		return smallmoneytype;
	} else if (!charstring::compareIgnoringCase(type,"unichar")) {
		return unichartype;
	} else if (!charstring::compareIgnoringCase(type,"univarchar")) {
		return univarchartype;
	} else if (!charstring::compareIgnoringCase(type,"unitext")) {
		return unitexttype;
	}
	return NULL;
}

const char *sapconnection::getColumnListQuery(const char *catalog,
						const char *schema,
						const char *table,
						const char *column) {

	columnlistquery.clear();

	// select clause
	columnlistquery.append(
		"select "
		"	db_name() as table_cat, "
		"	user_name(ob.uid) as table_schem, "
		"	ob.name as table_name, "
		"	co.name as column_name, "
		"	co.type as data_type, "
		"	ty.name as type_name, "
		"	co.length as column_size, "
		"	co.length as buffer_length, "
		"	co.scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	(co.status&8)/8 as nullable, "
		"	case "
		"		when (co.status&128)=128 "
		"			then 'auto_increment' "
		"		else null "
		"	end as remarks, "
		"	null as column_default, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	co.length as char_octet_length, "
		"	null as ordinal_position, "
		"	case (co.status&8)/8 "
		"		when 0 then 'YES' "
		"		else 'YES' "
		"	end as is_nullable, "
		"	co.prec as numeric_precision, "
		"	case ck.key_priority "
		"		when 1 then 'PRI' "
		"		when 2 then 'UNI' "
		"		when 3 then 'MUL' "
		"		else null "
		"	end as column_key, "
		"	case "
		"		when (co.status&128)=128 then 'YES' "
		"		else 'NO' "
		"	end as is_autoincrement, "
		"	null ");

	// from clause
	columnlistquery.append(
		"from "
		"	sysobjects ob, "
		"	syscolumns co "
		"	left outer join ( "
		"		select "
		"			c2.id, "
		"			c2.colid as colno, "
		"			min(case "
		"				when i.status & 2048 = 2048 "
		"					then 1 "
		"				when i.status & 2 = 2 "
		"					then 2 "
		"			end) as key_priority "
		"		from "
		"			sysindexes i, "
		"			syscolumns c2 "
		"		where "
		"			i.id=c2.id "
		"			and "
		"			i.indid between 1 and 254 "
		"			and "
		"			(i.status & 2048 = 2048 "
		"			or "
		"			i.status & 2 = 2) "
		"			and "
		"			(c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,1) "
		"			or c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,2) "
		"			or c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,3) "
		"			or c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,4) "
		"			or c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,5) "
		"			or c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,6) "
		"			or c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,7) "
		"			or c2.name= "
		"			index_col("
		"			object_name(i.id),"
		"			i.indid,8)) "
		"		group by "
		"			c2.id, "
		"			c2.colid "
		"	) ck "
		"	on "
		"	co.id=ck.id "
		"	and "
		"	co.colid=ck.colno, "
		"	systypes ty ");

	// where clause
	columnlistquery.append(
		"where "
		"	ob.type in ('S','U','V') "
		"	and "
		"	co.id=ob.id "
		"	and "
		"	ty.usertype=co.usertype ");
	if (!charstring::isNullOrEmpty(catalog)) {
		columnlistquery.append(
			"	and "
			"	db_name() like '");
		columnlistquery.append(catalog);
		columnlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(schema)) {
		columnlistquery.append(
			"	and "
			"	user_name(ob.uid) like '");
		columnlistquery.append(schema);
		columnlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		columnlistquery.append(
			"	and "
			"	ob.name like '");
		columnlistquery.append(table);
		columnlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(column)) {
		columnlistquery.append(
			"	and "
			"	co.name like '");
		columnlistquery.append(column);
		columnlistquery.append("' ");
	}

	// order by clause
	columnlistquery.append(
		"order by "
		"	co.colid");

	return columnlistquery.getString();
}

const char *sapconnection::getPrimaryKeysListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	primarykeyslistquery.clear();

	// select clause
	primarykeyslistquery.append(
		"select "
		"	db_name() as table_cat, "
		"	user_name(o.uid) as table_schem, "
		"	o.name as table_name, "
		"	index_col(o.name,i.indid,c.colid) as column_name, "
		"	c.colid as key_seq, "
		"	i.name as pk_name, "
		"	null ");

	// from clause
	primarykeyslistquery.append(
		"from "
		"	sysobjects o, "
		"	sysindexes i, "
		"	syscolumns c ");

	// where clause
	primarykeyslistquery.append(
		"where "
		"	i.status & 2048 = 2048 "
		"	and "
		"	o.id=i.id "
		"	and "
		"	o.id=c.id "
		"	and "
		"	c.colid<=i.keycnt ");
	if (!charstring::isNullOrEmpty(catalog)) {
		primarykeyslistquery.append(
			"	and "
			"	db_name() like '");
		primarykeyslistquery.append(catalog);
		primarykeyslistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		primarykeyslistquery.append(
			"	and "
			"	o.name like '");
		primarykeyslistquery.append(table);
		primarykeyslistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(schema)) {
		primarykeyslistquery.append(
			"	and "
			"	user_name(o.uid) like '");
		primarykeyslistquery.append(schema);
		primarykeyslistquery.append("' ");
	}

	// order by clause
	primarykeyslistquery.append(
		"order by "
		"	o.name, "
		"	c.colid");

	return primarykeyslistquery.getString();
}

const char *sapconnection::getKeyAndIndexListQuery(const char *catalog,
							const char *schema,
							const char *table) {

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	db_name() as table_cat, "
		"	user_name(o.uid) as table_schem, "
		"	o.name as table_name, "
		"	case "
		"		when i.status & 2 = 2 then 'FALSE' "
		"		else 'TRUE' "
		"	end as non_unique, "
		"	'' as index_qualifier, "
		"	i.name as index_name, "
		"	1 as type, "
		"	c.colid as ordinal_position, "
		"	index_col(o.name,i.indid,c.colid) as column_name, "
		"	'A' as asc_or_desc, "
		"	null as cardinality, "
		"	null as pages, "
		"	null as filter_condition, "
		"	null ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	sysobjects o, "
		"	sysindexes i, "
		"	syscolumns c ");

	// where clause
	keyandindexlistquery.append(
		"where "
		"	o.type='U' "
		"	and "
		"	o.id=i.id "
		"	and "
		"	i.indid>0 "
		"	and "
		"	i.indid<255 "
		"	and "
		"	o.id=c.id "
		"	and "
		"	c.colid<=i.keycnt ");
	if (!charstring::isNullOrEmpty(catalog)) {
		keyandindexlistquery.append(
			"	and "
			"	db_name() like '");
		keyandindexlistquery.append(catalog);
		keyandindexlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		keyandindexlistquery.append(
			"	and "
			"	o.name like '");
		keyandindexlistquery.append(table);
		keyandindexlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(schema)) {
		keyandindexlistquery.append(
			"	and "
			"	user_name(o.uid) like '");
		keyandindexlistquery.append(schema);
		keyandindexlistquery.append("' ");
	}

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	o.name, "
		"	i.name, "
		"	c.colid");

	return keyandindexlistquery.getString();
}

const char *sapconnection::getProcedureListQuery(
					const char *catalog,
					const char *schema,
					const char *procedure) {

	procedurelistquery.clear();

	// select clause
	procedurelistquery.append(
		"select "
		"	'' as procedure_cat, "
		"	user_name(uid) as procedure_schem, "
		"	name as procedure_name, "
		"	0 as num_input_params, "
		"	0 as num_output_params, "
		"	0 as num_result_sets, "
		"	'' as remarks, "
		"	case type "
		"		when 'P' then '1' "
		"		when 'SF' then '2' "
		"		else '0' "
		"	end as procedure_type, "
		"	null ");

	// from clause
	procedurelistquery.append(
		"from "
		"	sysobjects ");

	// where clause
	procedurelistquery.append(
		"where "
		"	type in ('P','SF') ");
	if (!charstring::isNullOrEmpty(schema)) {
		procedurelistquery.append(
			"and user_name(uid) like '");
		procedurelistquery.append(schema);
		procedurelistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(procedure)) {
		procedurelistquery.append(
			"and name like '");
		procedurelistquery.append(procedure);
		procedurelistquery.append("' ");
	}

	// order by clause
	procedurelistquery.append(
		"order by "
		"	user_name(uid), "
		"	name");

	return procedurelistquery.getString();
}

const char *sapconnection::getProcedureParameterListQuery(
					const char *catalog,
					const char *schema,
					const char *procedure) {

	procedureparameterlistquery.clear();

	// select clause
	procedureparameterlistquery.append(
		"select "
		"	'' as procedure_cat, "
		"	user_name(o.uid) as procedure_schem, "
		"	o.name as procedure_name, "
		"	c.name as column_name, "
		"	case c.status2 & 3 "
		"		when 1 then 1 "
		"		when 2 then 4 "
		"		else 0 "
		"	end as column_type, "
		"	'' as data_type, "
		"	t.name as type_name, "
		"	c.prec as column_size, "
		"	c.length as buffer_length, "
		"	c.scale as decimal_digits, "
		"	10 as num_prec_radix, "
		"	1 as nullable, "
		"	'' as remarks, "
		"	null as column_def, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	c.length as char_octet_length, "
		"	c.colid as ordinal_position, "
		"	'YES' as is_nullable, "
		"	null ");

	// from clause
	procedureparameterlistquery.append(
		"from "
		"	sysobjects o, "
		"	syscolumns c, "
		"	systypes t ");

	// where clause
	procedureparameterlistquery.append(
		"where "
		"	o.type='P' "
		"	and "
		"	o.id=c.id "
		"	and "
		"	c.usertype=t.usertype ");
	if (!charstring::isNullOrEmpty(schema)) {
		procedureparameterlistquery.append(
			"	and "
			"	user_name(o.uid) like '");
		procedureparameterlistquery.append(schema);
		procedureparameterlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(procedure)) {
		procedureparameterlistquery.append(
			"	and "
			"	o.name like '");
		procedureparameterlistquery.append(procedure);
		procedureparameterlistquery.append("' ");
	}

	// order by clause
	procedureparameterlistquery.append(
		"order by "
		"	o.name, "
		"	c.colid");

	return procedureparameterlistquery.getString();
}

const char *sapconnection::selectCatalogQuery() {
	return "use %s";
}

const char *sapconnection::getCurrentCatalogQuery() {
	return "select db_name()";
}

const char *sapconnection::getCurrentSchemaQuery() {
	return "select user_name()";
}

const char *sapconnection::getCurrentUserQuery() {
	// suser_sname() isn't available on all ASE versions, but
	// suser_name() is
	return "select suser_name()";
}

const char *sapconnection::getLastInsertIdQuery() {
	return "select @@identity";
}

const char *sapconnection::getIsolationLevelQuery() {
	return "select @@isolation";
}

const char *sapconnection::mapIsolationLevel(
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
			return "0";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_READ_COMMITTED")) {
			return "1";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_REPEATABLE_READ")) {
			return "2";
		}
		if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
			return "3";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(isolevel,"0")) {
			return "TRANSACTION_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"1")) {
			return "TRANSACTION_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"2")) {
			return "TRANSACTION_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(isolevel,"3")) {
			return "TRANSACTION_SERIALIZABLE";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_UNCOMMITTED")) {
			return "0";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_COMMITTED")) {
			return "1";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_REPEATABLE_READ")) {
			return "2";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_SERIALIZABLE")) {
			return "3";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(isolevel,"0")) {
			return "SQL_TXN_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"1")) {
			return "SQL_TXN_READ_COMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"2")) {
			return "SQL_TXN_REPEATABLE_READ";
		}
		if (!charstring::compareIgnoringCase(isolevel,"3")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *sapconnection::getDatabaseFeatures() {
	cont->capDatabaseFeatures(databasefeatures);
	return databasefeatures;
}

const char *sapconnection::getNoopQuery() {
	return "waitfor delay '0:0'";
}

const char *sapconnection::getBindFormat() {
	return "@*";
}

const char *sapconnection::beginTransactionQuery() {
	return "BEGIN TRANSACTION";
}

sapcursor::sapcursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {
	prepared=false;
	sapconn=(sapconnection *)conn;
	cmd=NULL;
	languagecmd=NULL;
	cursorcmd=NULL;
	dynamicprepared=false;
	dynamiccursor=false;
	outputdescribed=false;

	// no query has run yet, so there's nothing for nextResultSet() to
	// advance - matches the state readResultSets() leaves behind when it
	// runs out to the end of a query's result sets
	results=CS_END_RESULTS;
	rowresultstype=0;
	rowsbuffered=false;
	firstbatch=NULL;
	lastbatch=NULL;
	currentbatch=NULL;
	bufferedrows=0;

	autocursornamesize=charstring::getIntegerLength(id);
	autocursorname=charstring::parseNumber(id);
	cursorname=autocursorname;
	cursornamesize=autocursornamesize;

	// the dynamic statement name has to be unique within the connection,
	// and distinct from the cursor name
	chooseDynamicName();

	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	parameter=new CS_DATAFMT[maxbindcount];
	inbindvalue=new CS_VOID *[maxbindcount];
	inbinddatasize=new CS_INT[maxbindcount];
	inbindindicator=new CS_SMALLINT[maxbindcount];
	inbindts=new char *[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		inbindts[i]=new char[27];
	}
	outbindtype=new CS_INT[maxbindcount];
	outbindstrings=new char *[maxbindcount];
	outbindstringsizes=new uint32_t[maxbindcount];
	outbindints=new int64_t *[maxbindcount];
	outbinddoubles=new double *[maxbindcount];
	outbindprecisions=new uint32_t *[maxbindcount];
	outbindscales=new uint32_t *[maxbindcount];
	outbinddates=new datebind[maxbindcount];
	outbindisnulls=new int16_t *[maxbindcount];

	// replace the regular expression used to detect creation of a
	// temporary table
	setCreateTempTablePattern("^(create|CREATE)[ 	\r\n]+"
					"(table|TABLE)[ 	\r\n]+#");

	columncount=0;
	allocateResultSetBuffers(conn->cont->getMaxColumnCount());

	// define a template column-bind definition...
	// get the field as a null terminated character string
	// no longer than MAX_ITEM_BUFFER_SIZE, override some
	templatecolumn.datatype=CS_CHAR_TYPE;
	templatecolumn.format=CS_FMT_NULLTERM;
	templatecolumn.maxlength=conn->cont->getMaxFieldSize();
	templatecolumn.scale=CS_UNUSED;
	templatecolumn.precision=CS_UNUSED;
	templatecolumn.status=CS_UNUSED;
	templatecolumn.count=getFetchAtOnce();
	templatecolumn.usertype=CS_UNUSED;
	templatecolumn.locale=NULL;
}

sapcursor::~sapcursor() {
	close();
	delete[] autocursorname;
	delete[] parameter;
	delete[] inbindvalue;
	delete[] inbinddatasize;
	delete[] inbindindicator;
	for (uint16_t i=0; i<maxbindcount; i++) {
		delete[] inbindts[i];
	}
	delete[] inbindts;
	delete[] outbindtype;
	delete[] outbindstrings;
	delete[] outbindstringsizes;
	delete[] outbindints;
	delete[] outbinddoubles;
	delete[] outbindprecisions;
	delete[] outbindscales;
	delete[] outbinddates;
	delete[] outbindisnulls;

	freeRowBatches();

	deallocateResultSetBuffers();
}

void sapcursor::allocateResultSetBuffers(int32_t columncount) {

	if (!columncount) {
		this->columncount=0;
		column=NULL;
		data=NULL;
		datasize=NULL;
		nullindicator=NULL;
	} else {
		this->columncount=columncount;
		column=new CS_DATAFMT[columncount];
		data=new char *[columncount];
		datasize=new CS_INT *[columncount];
		nullindicator=new CS_SMALLINT *[columncount];
		uint32_t	fetchatonce=getFetchAtOnce();
		uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();
		for (int32_t i=0; i<columncount; i++) {
			data[i]=new char[fetchatonce*maxfieldsize];
			datasize[i]=new CS_INT[fetchatonce];
			nullindicator[i]=new CS_SMALLINT[fetchatonce];
		}
	}
}

void sapcursor::deallocateResultSetBuffers() {
	if (columncount) {
		delete[] column;
		for (int32_t i=0; i<columncount; i++) {
			delete[] data[i];
			delete[] datasize[i];
			delete[] nullindicator[i];
		}
		delete[] data;
		delete[] datasize;
		delete[] nullindicator;
		columncount=0;
	}
}

bool sapcursor::open() {

	clean=true;

	if (ct_cmd_alloc(sapconn->dbconn,&languagecmd)!=CS_SUCCEED) {
		return false;
	}
	if (ct_cmd_alloc(sapconn->dbconn,&cursorcmd)!=CS_SUCCEED) {
		return false;
	}
	cmd=NULL;

	// switch to the correct database, get dbversion
	// (only do this once per connection)
	bool	retval=true;
	if (!charstring::isNullOrEmpty(sapconn->db) && !sapconn->dbused) {
		int32_t	len=charstring::getLength(sapconn->db)+4;
		char	*query=new char[len+1];
		charstring::printf(query,len+1,"use %s",sapconn->db);
		if (!(prepareQuery(query,len) && executeQuery(query,len))) {
			char		err[2048];
			uint32_t	errlen;
			int64_t		errn;
			bool		live;
			getError(err,sizeof(err),&errlen,&errn,&live);
			stderror.printf("%s\n",err);
			retval=false;
		} else {
			sapconn->dbused=true;
		}
		closeResultSet();
		delete[] query;
	}

	if (!sapconn->dbversion) {

		// try the various queries that might return the version
		const char	*query[]={
			"select @@version",
			"sp_version installmaster",
			NULL
		};
		CS_INT		index[]={
			0,1,0
		};

		for (uint32_t i=0; query[i] && !sapconn->dbversion; i++) {

			const char	*q=query[i];
			int32_t		len=charstring::getLength(q);
			bool		error=false;

			if (prepareQuery(q,len) &&
					executeQuery(q,len) &&
					fetchRow(&error)) {
				sapconn->dbversion=
					charstring::duplicate(data[index[i]]);
			}

			closeResultSet();
		}

		// fall back to unknown
		if (!sapconn->dbversion) {
			sapconn->dbversion=
				charstring::duplicate("unknown");
		}
	}

	return retval;
}

bool sapcursor::close() {
	bool	retval=true;
	if (languagecmd) {
		retval=(ct_cmd_drop(languagecmd)==CS_SUCCEED);
		languagecmd=NULL;
	}
	if (cursorcmd) {
		discardDynamic();
		retval=(retval && (ct_cmd_drop(cursorcmd)==CS_SUCCEED));
		cursorcmd=NULL;
	}
	cmd=NULL;
	return retval;
}

static const char *getRpcName(const char *p, const char **namestart,
							CS_INT *namelen) {

	// a delimited name (double-quotes under quoted_identifier, square
	// brackets in T-SQL) may contain whitespace
	if (*p=='"' || *p=='[') {
		char	closing=(*p=='[')?']':'"';
		p++;
		*namestart=p;
		while (*p && *p!=closing) {
			p++;
		}
		*namelen=(CS_INT)(p-*namestart);
		if (*p==closing) {
			p++;
		}
	} else {
		*namestart=p;
		while (*p && *p!='(' && *p!='}' && *p!=';' &&
				!character::isWhitespace(*p)) {
			p++;
		}
		*namelen=(CS_INT)(p-*namestart);
	}
	return p;
}

static bool rewriteBindMarkersToPositional(const char *query, uint32_t size,
							stringbuffer *out) {

	// translateBindVariables() already rewrote the query to use sap's
	// native "@*" bind format (getBindFormat()) by the time it gets
	// here.  That format is right for rpc/language commands, but not
	// for a select that gets prepared with ct_dynamic(CS_PREPARE) or
	// declared with ct_cursor(CS_CURSOR_DECLARE) - both of those want
	// positional "?" markers instead.  The values are still supplied
	// via ct_param() in bind-variable order either way, so swapping the
	// marker here doesn't change how binding works, only what's written
	// into the prepared statement's SQL text.
	//
	// Note that "@@name" (a global variable like @@version or
	// @@identity) isn't a bind marker and is left alone - the name that
	// follows the first "@" has to start with an alphanumeric or an
	// underscore.
	//
	// Returns true if any bind marker was found.
	bool		foundmarker=false;
	const char	*p=query;
	const char	*end=query+size;
	bool		inquotes=false;
	char		prev='\0';
	while (p<end) {
		char	c=*p;
		if (!inquotes && c=='@' && (p==query ||
				character::isWhitespace(prev) ||
				prev=='(' || prev==',' || prev=='=')) {
			const char	*namestart=p+1;
			const char	*np=namestart;
			while (np<end && (character::isAlphanumeric(*np) ||
							*np=='_')) {
				np++;
			}
			if (np>namestart) {
				out->append('?');
				foundmarker=true;
				prev='?';
				p=np;
				continue;
			}
		}
		if (c=='\'' && prev!='\\') {
			inquotes=!inquotes;
		}
		out->append(c);
		prev=c;
		p++;
	}
	return foundmarker;
}

// True if the "select" keyword at p is introduced by a set operator
// (union, union all, except or intersect) joining it to the select before
// it, rather than starting a new statement - eg. in
// "select a from t1 union select b from t2" the second select is still
// part of the first statement.
static bool isSetOperatorSelect(const char *query, const char *p) {

	const char	*wordend=p;
	while (wordend>query && character::isWhitespace(wordend[-1])) {
		wordend--;
	}
	const char	*wordstart=wordend;
	while (wordstart>query && (character::isAlphanumeric(wordstart[-1]) ||
						wordstart[-1]=='_')) {
		wordstart--;
	}
	size_t	wordlen=wordend-wordstart;

	// skip back over a trailing "all" to find what precedes it - for
	// "union all" the operator keyword is "union", not "all"
	if (wordlen==3 && !charstring::compareIgnoringCase(wordstart,"all",3)) {
		wordend=wordstart;
		while (wordend>query && character::isWhitespace(wordend[-1])) {
			wordend--;
		}
		wordstart=wordend;
		while (wordstart>query &&
				(character::isAlphanumeric(wordstart[-1]) ||
						wordstart[-1]=='_')) {
			wordstart--;
		}
		wordlen=wordend-wordstart;
	}

	return (wordlen==5 &&
			!charstring::compareIgnoringCase(wordstart,"union",5)) ||
		(wordlen==6 &&
			!charstring::compareIgnoringCase(wordstart,"except",6)) ||
		(wordlen==9 &&
			!charstring::compareIgnoringCase(wordstart,"intersect",9));
}

// True if c can't be the character right before a fresh top-level word -
// either because it's itself part of an identifier (alphanumeric or '_'),
// or because it means whatever comes next is still part of a qualified or
// quoted name rather than a new keyword: "." (schema.table.column), "@"
// (a variable name), "#" (a temp table name), "\"" (a closing ansi-quoted
// identifier) or "[" (an opening bracketed identifier) - eg. the "delete"
// in "select * from t.delete" or "select * from [delete]" is a column or
// table name, not the start of a DELETE statement.
static bool isIdentifierContinuationChar(char c) {
	return character::isAlphanumeric(c) || c=='_' ||
		c=='.' || c=='@' || c=='#' || c=='"' || c=='[';
}

// Statement-leading keywords isMultiStatementBatch() looks for to recognize
// where a second top-level statement starts in a batch that has no required
// separator between statements.  Not exhaustive - deliberately limited to
// keywords that are unambiguous as the first word of a statement, to avoid
// misreading an identifier/alias as the start of a new one (eg. "set" and
// "begin" are left out - both are common as column aliases or variable
// names, unlike, say, "raiserror").
struct sapstatementkeyword {
	const char	*keyword;
	size_t		len;
};
static const sapstatementkeyword sapstatementkeywords[]={
	{"select",6}, {"insert",6}, {"update",6}, {"delete",6},
	{"exec",4}, {"execute",7}, {"declare",7},
	{"create",6}, {"drop",4}, {"alter",5}, {"truncate",8},
	{"print",5}, {"raiserror",9}, {"waitfor",7},
	{"grant",5}, {"revoke",6}, {"commit",6}, {"rollback",8}
};
static const size_t sapstatementkeywordcount=
	sizeof(sapstatementkeywords)/sizeof(sapstatementkeywords[0]);

// Returns the length of the statement-leading keyword at p, matched whole-
// word (case insensitive), or 0 if none of sapstatementkeywords starts
// there.
static size_t matchStatementKeyword(const char *p, const char *end) {
	for (size_t i=0; i<sapstatementkeywordcount; i++) {
		size_t	len=sapstatementkeywords[i].len;
		if ((size_t)(end-p)>=len &&
			!charstring::compareIgnoringCase(p,
					sapstatementkeywords[i].keyword,len) &&
			(p+len==end || !(character::isAlphanumeric(p[len]) ||
							p[len]=='_'))) {
			return len;
		}
	}
	return 0;
}

// True if the query text contains a second top-level statement - one that
// isn't inside a quoted string literal or inside parentheses (a subquery),
// and, if it's a select, isn't joined to the select before it by a set
// operator.  ASE's ct_cursor(CS_CURSOR_DECLARE) refuses to declare a cursor
// over more than one statement ("More than one SELECT statement is used to
// define the cursor", regardless of what kind of statement the second one
// is), so prepareQuery() uses this to decide whether a query that starts
// with "select " is really a single statement (cursor it) or a
// multi-statement tds LANGUAGE batch (run it un-cursored instead - see the
// CS_LANG_CMD branch below).
//
// This is a heuristic, not a real parser: it doesn't skip comments, so a
// statement keyword written inside a comment can cause a single statement
// to be misdetected as a batch.  The query still runs correctly either way
// - it's just routed through the un-cursored path instead of a cursor -
// but that's not entirely free: ct-lib only allows one non-cursor command
// per connection to have results pending, and a session's other cursors
// share that connection, so a misdetected select holds that shared slot
// until the client has fetched all of its rows, where a real cursor
// wouldn't have.  It also can't catch every possible second statement -
// only the keywords in sapstatementkeywords - so a single select followed
// by some other kind of statement not in that list is still misrouted to
// the cursor path, the same as before this function existed.
static bool isMultiStatementBatch(const char *query, uint32_t size) {

	const char	*p=query;
	const char	*end=query+size;
	bool		inquotes=false;
	int32_t		parendepth=0;
	bool		foundfirst=false;
	while (p<end) {
		char	c=*p;
		if (inquotes) {
			if (c=='\'') {
				inquotes=false;
			}
			p++;
			continue;
		}
		if (c=='\'') {
			inquotes=true;
			p++;
			continue;
		}
		if (c=='(') {
			parendepth++;
			p++;
			continue;
		}
		if (c==')') {
			if (parendepth>0) {
				parendepth--;
			}
			p++;
			continue;
		}
		if (!parendepth &&
			(p==query || !isIdentifierContinuationChar(p[-1]))) {
			size_t	matchlen=matchStatementKeyword(p,end);
			if (matchlen) {
				bool	isselect=(matchlen==6 &&
						!charstring::compareIgnoringCase(
							p,"select",6));
				if (!foundfirst) {
					foundfirst=true;
				} else if (!isselect ||
						!isSetOperatorSelect(query,p)) {
					return true;
				}
				p+=matchlen;
				continue;
			}
		}
		p++;
	}
	return false;
}

// Build a dynamic statement name that's unique within the connection and
// distinct from whatever the cursor is currently named.  The normal form is
// "sqlrdyn<cursor id>", with an underscore appended in the unlikely event
// that the client supplied exactly that as its cursor name.  Only one of the
// two forms can match the cursor name, so this always yields a distinct name.
void sapcursor::chooseDynamicName() {

	charstring::printf(dynamicname,sizeof(dynamicname),
					"sqlrdyn%d",(int32_t)getId());
	if (!charstring::compareIgnoringCase(cursorname,dynamicname)) {
		charstring::printf(dynamicname,sizeof(dynamicname),
					"sqlrdyn%d_",(int32_t)getId());
	}
	dynamicnamesize=charstring::getLength(dynamicname);
}

// Decide which name the cursor is about to be declared with.  Call this
// immediately before ct_cursor(CS_CURSOR_DECLARE)/ct_dynamic(CS_CURSOR_DECLARE)
// rather than from the constructor - the client can call setCursorName() any
// time before the query is prepared, and the name it sets has to be the one
// the cursor is declared with, since the client refers to it by name in
// "where current of" queries.
//
// cursorname isn't owned by this class either way.  The auto-generated name
// belongs to autocursorname and the client-supplied one belongs to the
// sqlrservercursor that setCursorName() stored it in, and only ct_cursor()/
// ct_dynamic() read it, right after this runs, so there's nothing to free
// here and no chance of it going away in between.
void sapcursor::chooseCursorName() {

	cursorname=getCursorName();
	if (charstring::isNullOrEmpty(cursorname)) {
		cursorname=autocursorname;
		cursornamesize=autocursornamesize;
	} else {
		cursornamesize=charstring::getLength(cursorname);
	}

	// The auto-generated cursor name is just the cursor id in decimal,
	// so it can never match the "sqlrdyn"-prefixed dynamic statement
	// name, but a client-supplied name can match it exactly.  The client
	// name has to be used verbatim, so rename the dynamic statement
	// instead - nothing outside of this class ever sees that name.
	// Renaming it abandons any statement still prepared under the old
	// name, so drop that first.
	if (!charstring::compareIgnoringCase(cursorname,dynamicname)) {
		discardDynamic();
		chooseDynamicName();
	}
}

bool sapcursor::prepareDynamic() {

	cmd=cursorcmd;

	// clear out any errors, so that a failed prepare reports its own
	// error rather than one left over from an earlier query
	sapconn->errorcode=0;
	sapconn->liveconnection=true;

	// drop whatever statement this cursor prepared last
	discardDynamic();

	// decide what to declare the cursor as, and keep the dynamic
	// statement name distinct from it
	chooseCursorName();

	// prepare the statement...
	if (ct_dynamic(cursorcmd,CS_PREPARE,
			(CS_CHAR *)dynamicname,(CS_INT)dynamicnamesize,
			(CS_CHAR *)positionalquerybuffer.getString(),
			(CS_INT)positionalquerybuffer.getSize())!=CS_SUCCEED) {
		return false;
	}
	if (ct_send(cursorcmd)!=CS_SUCCEED) {
		return false;
	}

	// ...and run the prepare's own result sets out.  ASE parses the
	// statement at this point, so a syntax error surfaces here rather
	// than at execute time.
	CS_INT	res;
	CS_INT	restype;
	bool	success=true;
	while ((res=ct_results(cursorcmd,&restype))==CS_SUCCEED) {
		if (restype==CS_CMD_FAIL) {
			success=false;
		}
		if (ct_cancel(NULL,cursorcmd,CS_CANCEL_CURRENT)==CS_FAIL) {
			sapconn->liveconnection=false;
			return false;
		}
	}
	if (res!=CS_END_RESULTS || !success || sapconn->errorcode) {
		return false;
	}
	dynamicprepared=true;

	// ask ct-lib to describe the prepared statement's output columns.
	// Nothing downstream of prepareQuery() populates ncols/column[] for
	// a dynamic-cursor select otherwise - bindResultSetColumns() only
	// runs against a real result set, from executeQuery() - so without
	// this, colCount() stays 0 for a prepared select and a client's own
	// CS_DESCRIBE_OUTPUT (relayed via sqlrprotocol_tds::prepare(), which
	// is gated on colCount()) sees no columns.
	if (ct_dynamic(cursorcmd,CS_DESCRIBE_OUTPUT,
			(CS_CHAR *)dynamicname,(CS_INT)dynamicnamesize,
			NULL,CS_UNUSED)!=CS_SUCCEED) {
		return false;
	}
	if (ct_send(cursorcmd)!=CS_SUCCEED) {
		return false;
	}
	CS_INT	describeres;
	CS_INT	describerestype;
	bool	describesuccess=true;
	while ((describeres=ct_results(cursorcmd,&describerestype))==
							CS_SUCCEED) {
		if (describerestype==CS_CMD_FAIL) {
			describesuccess=false;
		} else if (describerestype==CS_DESCRIBE_RESULT) {
			if (!describeResultSetColumns()) {
				describesuccess=false;
			}
		}
		// every result set has to be run out to the end before the
		// next ct_results() (see drainResultSet())
		if (ct_cancel(NULL,cursorcmd,CS_CANCEL_CURRENT)==CS_FAIL) {
			sapconn->liveconnection=false;
			return false;
		}
	}
	if (describeres!=CS_END_RESULTS || !describesuccess ||
						sapconn->errorcode) {
		return false;
	}
	outputdescribed=true;

	// ...then declare a cursor on the prepared statement, rather than
	// on the select text.  From here on the flow is the ordinary cursor
	// flow - ct_param() placeholders, then ct_cursor(CS_CURSOR_ROWS),
	// ct_cursor(CS_CURSOR_OPEN), ct_param() values and ct_send() in
	// executeQuery().
	if (ct_dynamic(cursorcmd,CS_CURSOR_DECLARE,
			(CS_CHAR *)dynamicname,(CS_INT)dynamicnamesize,
			(CS_CHAR *)cursorname,
			(CS_INT)cursornamesize)!=CS_SUCCEED) {
		return false;
	}
	dynamiccursor=true;

	clean=false;
	prepared=true;
	return true;
}

bool sapcursor::columnInfoIsValidAfterPrepare() {

	// Only prepareDynamic()'s CS_DESCRIBE_OUTPUT block (see outputdescribed)
	// actually populates ncols/column[] at prepare time, for a select
	// prepared with bind markers. Every other prepare - DML/DDL, or a
	// select declared as a cursor over its own text - leaves ncols at 0
	// until execute, and the framework's handleResultSetHeader() latches
	// the column info the first time it's told it's valid, so returning
	// true unconditionally here would latch a stale 0 for those and never
	// pick up the real post-execute column count.
	return outputdescribed;
}

bool sapcursor::prepareQuery(const char *query, uint32_t size) {

	// clear out any errors, so that a failed prepare reports its own
	// error rather than one left over from an earlier query
	sapconn->errorcode=0;
	sapconn->liveconnection=true;

	// initialize column count
	ncols=0;
	outputdescribed=false;

	clean=true;

	this->query=(char *)query;
	this->size=size;

	paramindex=0;
	outbindindex=0;
	hasreturnvalue=false;
	dynamiccursor=false;

	if ((!charstring::compare(query,"select",6) ||
		!charstring::compare(query,"SELECT",6)) &&
		character::isWhitespace(query[6]) &&
		!isMultiStatementBatch(query,size)) {

		positionalquerybuffer.clear();
		bool	hasbindmarkers=rewriteBindMarkersToPositional(
						query,size,
						&positionalquerybuffer);

		if (hasbindmarkers) {

			// A select with bind markers can't be declared as a
			// cursor over its own text.  ASE doesn't parse a
			// ct_cursor(CS_CURSOR_DECLARE) statement until the
			// cursor is opened - the declare returns CS_SUCCEED
			// either way - and when it finally does parse it, it
			// rejects the positional "?" markers, failing the
			// ct_send() with "Incorrect syntax near '?'".
			// Markers are only valid in dynamic SQL.
			//
			// So a select with binds gets prepared with
			// ct_dynamic(CS_PREPARE) and the cursor gets declared
			// on the prepared statement instead of on the text
			// (see prepareDynamic()).  Everything downstream of
			// the declare is the same either way, and it stays a
			// cursor - which matters, since ct-lib only allows one
			// non-cursor command per connection to have results
			// pending, and the other cursors of the same session
			// share this connection.
			return prepareDynamic();
		}

		// initiate a cursor command
		// (don't use CS_NULLTERM for the 4th parameter, it randomly
		// causes weird things to happen)
		cmd=cursorcmd;
		dynamiccursor=false;
		chooseCursorName();
		if (ct_cursor(cursorcmd,
				CS_CURSOR_DECLARE,
				(CS_CHAR *)cursorname,
				(CS_INT)cursornamesize,
				(CS_CHAR *)positionalquerybuffer.getString(),
				(CS_INT)positionalquerybuffer.getSize(),
				CS_READ_ONLY)!=CS_SUCCEED) {
			return false;
		}

	} else if ((!charstring::compare(query,"exec",4) ||
			!charstring::compare(query,"EXEC",4)) &&
					character::isWhitespace(query[4])) {

		// initiate an rpc command
		cmd=languagecmd;

		// get the procedure name
		const char	*p=conn->cont->skipWhitespace(query+4);
		const char	*namestart;
		CS_INT		namelen;
		p=getRpcName(p,&namestart,&namelen);

		if (ct_command(languagecmd,
				CS_RPC_CMD,
				(CS_CHAR *)namestart,
				namelen,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}

		// ct_param() literal parameters written into the query
		// (eg. "exec someproc 1,2" - the client doesn't bind those)
		if (!parseRpcParams(p)) {
			return false;
		}

	} else if ((!charstring::compare(query,"execute",7) ||
			!charstring::compare(query,"EXECUTE",7)) &&
					character::isWhitespace(query[7])) {

		// initiate an rpc command
		cmd=languagecmd;

		// get the procedure name
		const char	*p=conn->cont->skipWhitespace(query+7);
		const char	*namestart;
		CS_INT		namelen;
		p=getRpcName(p,&namestart,&namelen);

		if (ct_command(languagecmd,
				CS_RPC_CMD,
				(CS_CHAR *)namestart,
				namelen,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}

		// ct_param() literal parameters written into the query
		// (eg. "execute someproc 1,2" - the client doesn't bind those)
		if (!parseRpcParams(p)) {
			return false;
		}

	} else if (query[0]=='{') {

		// handle ODBC/JDBC procedure-call syntax:
		// {call proc(...)} or {?=call proc(...)}

		// find "call", noting whether a bind variable appears before
		// it - that's the reserved return-value form ("{ @1=call
		// proc(...) }", built by src/protocols/tds.cpp's namedProc())
		const char	*p=query+1;
		while (*p && *p!='}' &&
				charstring::compare(p,"call",4) &&
				charstring::compare(p,"CALL",4)) {
			if (*p=='=') {
				hasreturnvalue=true;
			}
			p++;
		}

		// skip past "call"
		if (!charstring::compare(p,"call",4) ||
				!charstring::compare(p,"CALL",4)) {
			p+=4;
		}

		// skip whitespace
		p=conn->cont->skipWhitespace(p);

		// get the procedure name
		const char	*namestart;
		CS_INT		namelen;
		p=getRpcName(p,&namestart,&namelen);

		// initiate a language command
		cmd=languagecmd;
		if (ct_command(languagecmd,
				CS_RPC_CMD,
				(CS_CHAR *)namestart,
				namelen,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}

	} else {

		// initiate a language command
		cmd=languagecmd;
		if (ct_command(languagecmd,
				CS_LANG_CMD,
				(CS_CHAR *)query,
				size,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}
	}

	clean=false;
	prepared=true;
	return true;
}

void sapcursor::encodeBlob(stringbuffer *buffer,
					const char *data, uint32_t datasize) {

	// sybase wants each byte of blob data to be converted to two
	// hex characters and the whole thing to start with 0x
	// eg: hello - > 0x68656C6C6F
	// just "0x" is illegal though, so use 0x00 for empty data, which
	// is really a single \0 rather than truly empty data

	buffer->append("0x");
	if (!datasize) {
		buffer->append("00");
	} else {
		for (uint32_t i=0; i<datasize; i++) {
			buffer->append(conn->cont->asciiToHex(data[i]));
		}
	}
}

void sapcursor::decodeBlob(char **data, uint32_t *datasize) {

	// sybase encoded binary format is two hex characters per byte
	// eg: 68656C6C6F -> hello

	char	*write=*data;
	char	*end=write+*datasize;
	char	buf[3];
	buf[2]='\0';
	for (char *read=write; read+1<end; read+=2) {
		buf[0]=read[0];
		buf[1]=read[1];
		*write=(char)charstring::convertToUnsignedInteger(buf,16);
		write++;
	}
	*datasize=write-*data;
}

void sapcursor::checkRePrepare() {

	// Sybase doesn't allow you to rebind and re-execute when using
	// ct_command.  You have to re-prepare too.  I'll make this transparent
	// to the user.
	// FIXME: skip if cmd==cursorcmd?
	if (!prepared) {
		prepareQuery(query,size);
	}
}

bool sapcursor::parseRpcParams(const char *p) {

	p=conn->cont->skipWhitespace(p);

	// bail if the parameters are bind-variable references
	// (eg. "exec testproc @in1,@in2 output" - the client binds those
	// by name itself, so the text here is just documentation)
	for (const char *s=p; *s && *s!=';'; s++) {
		if (*s=='@') {
			return true;
		}
	}

	while (*p && *p!=';') {

		const char	*valstart=p;
		const char	*valend;
		bool	quoted=false;
		if (*p=='\'' || *p=='"') {
			quoted=true;
			char	quote=*p;
			p++;
			valstart=p;
			while (*p && *p!=quote) {
				p++;
			}
			valend=p;
			if (*p==quote) {
				p++;
			}
		} else {
			while (*p && *p!=',' && *p!=';' &&
					!character::isWhitespace(*p)) {
				p++;
			}
			valend=p;
		}
		CS_INT	vallen=(CS_INT)(valend-valstart);

		bytestring::zero(&parameter[paramindex],
					sizeof(parameter[paramindex]));
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
		parameter[paramindex].maxlength=CS_UNUSED;
		parameter[paramindex].status=CS_INPUTVALUE;
		parameter[paramindex].locale=NULL;

		// type unquoted numeric literals - an rpc parameter has to
		// match the target parameter's type on the wire, and ASE
		// won't convert char to int/float the way it would a literal
		// in ordinary SQL text
		CS_RETCODE	rc;
		if (!quoted && charstring::isInteger(valstart,vallen)) {
			int64_t	intval=charstring::convertToInteger(valstart);
			parameter[paramindex].datatype=CS_INT_TYPE;
			rc=ct_param(languagecmd,&parameter[paramindex],
					(CS_VOID *)&intval,
					sizeof(intval),0);
		} else if (!quoted && charstring::isNumber(valstart,vallen)) {
			double	floatval=(double)
					charstring::convertToFloat(valstart);
			parameter[paramindex].datatype=CS_FLOAT_TYPE;
			rc=ct_param(languagecmd,&parameter[paramindex],
					(CS_VOID *)&floatval,
					sizeof(floatval),0);
		} else {
			parameter[paramindex].datatype=CS_CHAR_TYPE;
			rc=ct_param(languagecmd,&parameter[paramindex],
					(CS_VOID *)valstart,vallen,0);
		}
		if (rc!=CS_SUCCEED) {
			return false;
		}
		paramindex++;

		p=conn->cont->skipWhitespace(p);
		if (*p==',') {
			p=conn->cont->skipWhitespace(p+1);
		} else {
			break;
		}
	}
	return true;
}

void sapcursor::setParameterName(const char *variable,
					uint16_t variablesize) {

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));

	// numeric-named binds (@1, etc) are matched by position, not
	// by name.  cursorcmd is always positional here too, since
	// rewriteBindMarkersToPositional() replaced every bind marker in
	// the cursor's SQL with ? regardless of its name
	// (real bind var names are like @P1, not purely numeric)
	if (cmd==cursorcmd ||
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
}

bool sapcursor::inputBind(CS_VOID *value, CS_INT valuesize,
						CS_SMALLINT indicator) {

	if (cmd==cursorcmd) {

		// for a cursor command, the flow is:
		//
		// prepare:
		// * ct_cursor(CS_CURSOR_DECLARE)
		//
		// bind:
		// * ct_param(param,NULL);
		// * ct_param(param,NULL);
		// * ...
		//
		// execute:
		// * ct_cursor(CS_CURSOR_ROWS)
		// * ct_cursor(CS_CURSOR_OPEN)
		//
		// * ct_param(param,value);
		// * ct_param(param,value);
		// * ...
		//
		// * ct_send()
		//
		// So, at this phase, stash the value, valuesize, and indicator,
		// and declare a placeholder for the parameter.  We'll call
		// ct_param() again in executeQuery() to supply the values.
		//
		// A cursor declared on a prepared dynamic statement (see
		// prepareDynamic()) is the exception - it already knows its
		// parameters from the prepare, and ASE rejects the datastream
		// that the placeholder ct_param() generates for it ("The
		// datastream for token 32 should only be sent after other
		// datastreams"), so it just gets the values after the open.

		inbindvalue[paramindex]=value;
		inbinddatasize[paramindex]=valuesize;
		inbindindicator[paramindex]=indicator;
		if (dynamiccursor) {
			return true;
		}
		return ct_param(cmd,&parameter[paramindex],
					NULL,CS_UNUSED,0)==CS_SUCCEED;
	}

	// for non-cursor commands, we can supply the parameter values now
	return ct_param(cmd,&parameter[paramindex],
				value,valuesize,indicator)==CS_SUCCEED;
}

bool sapcursor::inputBind(const char *variable,
				uint16_t variablesize,
				const char *value,
				uint32_t valuesize,
				int16_t *isnull) {
	checkRePrepare();

	setParameterName(variable,variablesize);
	parameter[paramindex].datatype=CS_CHAR_TYPE;
	parameter[paramindex].maxlength=CS_UNUSED;
	parameter[paramindex].status=CS_INPUTVALUE;
	parameter[paramindex].locale=NULL;
	if (!inputBind((CS_VOID *)value,valuesize,
		(*isnull==conn->cont->getNullBindValue())?-1:0)) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::inputBindBlob(const char *variable,
				uint16_t variablesize,
				const char *value,
				uint32_t valuesize,
				int16_t *isnull) {
	checkRePrepare();

	setParameterName(variable,variablesize);
	// bind blobs as binary, not as character data.  the server would
	// otherwise try to convert the bytes to its own character set and
	// fail on any byte that isn't valid in the client's character set.
	// binary/varbinary tops out at 255 bytes, longer values have to
	// be sent as image.
	parameter[paramindex].datatype=
			(valuesize>255)?CS_IMAGE_TYPE:CS_BINARY_TYPE;
	parameter[paramindex].maxlength=CS_UNUSED;
	parameter[paramindex].status=CS_INPUTVALUE;
	parameter[paramindex].locale=NULL;
	if (!inputBind((CS_VOID *)value,valuesize,
		(*isnull==conn->cont->getNullBindValue())?-1:0)) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::inputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value) {
	checkRePrepare();

	setParameterName(variable,variablesize);
	parameter[paramindex].datatype=CS_INT_TYPE;
	parameter[paramindex].maxlength=CS_UNUSED;
	parameter[paramindex].status=CS_INPUTVALUE;
	parameter[paramindex].locale=NULL;
	if (!inputBind((CS_VOID *)value,sizeof(int64_t),0)) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::inputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t precision,
				uint32_t scale) {
	checkRePrepare();

	setParameterName(variable,variablesize);
	parameter[paramindex].datatype=CS_FLOAT_TYPE;
	parameter[paramindex].maxlength=CS_UNUSED;
	parameter[paramindex].status=CS_INPUTVALUE;
	parameter[paramindex].precision=precision;
	parameter[paramindex].scale=scale;
	parameter[paramindex].locale=NULL;
	if (!inputBind((CS_VOID *)value,sizeof(double),0)) {
		return false;
	}
	paramindex++;
	return true;
}

static const char *monthname[]={
	"Jan","Feb","Mar","Apr","May","Jun",
	"Jul","Aug","Sep","Oct","Nov","Dec",
	NULL
};

bool sapcursor::inputBind(const char *variable,
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
	checkRePrepare();

	// Sybase requires this format: "Jan 2 2012 4:5:3:000PM"
	if (month<1) {
		month=1;
	}
	if (month>12) {
		month=12;
	}
	const char	*ampm="AM";
	if (hour==0) {
		hour=12;
	} else if (hour==12) {
		ampm="PM";
	} else if (hour>12) {
		hour=hour-12;
		ampm="PM";
	}
	char	*buffer=inbindts[paramindex];
	charstring::copy(buffer,monthname[month-1]);
	charstring::append(buffer," ");
	charstring::append(buffer,(int64_t)day);
	charstring::append(buffer," ");
	charstring::append(buffer,(int64_t)year);
	charstring::append(buffer," ");
	charstring::append(buffer,(int64_t)hour);
	charstring::append(buffer,":");
	charstring::append(buffer,(int64_t)minute);
	charstring::append(buffer,":");
	charstring::append(buffer,(int64_t)second);
	charstring::append(buffer,":");
	charstring::append(buffer,(int64_t)microsecond);
	charstring::append(buffer,ampm);
	return inputBind(variable,variablesize,
				buffer,charstring::getLength(buffer),isnull);
}

bool sapcursor::outputBind(const char *variable, 
				uint16_t variablesize,
				char *value, 
				uint32_t valuesize, 
				int16_t *isnull) {
	checkRePrepare();

	// the reserved first output bind of a "{ X=call proc(...) }" rpc
	// query isn't a real rpc parameter - see the hasreturnvalue comment
	// on its declaration - so record it and skip ct_param() entirely.
	// it gets filled in from the CS_STATUS_RESULT in executeQuery().
	bool	isreturnvalue=(hasreturnvalue && !outbindindex);

	outbindtype[outbindindex]=CS_CHAR_TYPE;
	outbindstrings[outbindindex]=value;
	outbindstringsizes[outbindindex]=valuesize;
	outbindisnulls[outbindindex]=isnull;
	outbindindex++;

	if (isreturnvalue) {
		return true;
	}

	setParameterName(variable,variablesize);
	parameter[paramindex].datatype=CS_CHAR_TYPE;
	parameter[paramindex].maxlength=valuesize;
	parameter[paramindex].status=CS_RETURN;
	parameter[paramindex].locale=NULL;
	if (ct_param(cmd,&parameter[paramindex],
			(CS_VOID *)NULL,0,
			(CS_SMALLINT)*isnull)!=CS_SUCCEED) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::outputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value,
				int16_t *isnull) {
	checkRePrepare();

	// see the outputBind() overload above for why this matters
	bool	isreturnvalue=(hasreturnvalue && !outbindindex);

	outbindtype[outbindindex]=CS_INT_TYPE;
	outbindints[outbindindex]=value;
	outbindisnulls[outbindindex]=isnull;
	outbindindex++;

	if (isreturnvalue) {
		return true;
	}

	setParameterName(variable,variablesize);
	parameter[paramindex].datatype=CS_INT_TYPE;
	parameter[paramindex].maxlength=CS_UNUSED;
	parameter[paramindex].status=CS_RETURN;
	parameter[paramindex].locale=NULL;
	if (ct_param(cmd,&parameter[paramindex],
			(CS_VOID *)NULL,0,
			(CS_SMALLINT)*isnull)!=CS_SUCCEED) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::outputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t *precision,
				uint32_t *scale,
				int16_t *isnull) {
	checkRePrepare();

	// see sapcursor::outputBind(...,char *value,...) for why this matters
	bool	isreturnvalue=(hasreturnvalue && !outbindindex);

	outbindtype[outbindindex]=CS_FLOAT_TYPE;
	outbinddoubles[outbindindex]=value;
	// the precision and scale are outputs too - the server reports them
	// with the value, and fetchOutputParams() writes them back through
	// these.  A numeric or decimal parameter loses its scale otherwise,
	// and nothing downstream can tell 5.0000 from 5.
	outbindprecisions[outbindindex]=precision;
	outbindscales[outbindindex]=scale;
	outbindisnulls[outbindindex]=isnull;
	outbindindex++;

	if (isreturnvalue) {
		return true;
	}

	setParameterName(variable,variablesize);
	parameter[paramindex].datatype=CS_FLOAT_TYPE;
	parameter[paramindex].maxlength=CS_UNUSED;
	parameter[paramindex].status=CS_RETURN;
	parameter[paramindex].locale=NULL;
	if (ct_param(cmd,&parameter[paramindex],
			(CS_VOID *)NULL,0,
			(CS_SMALLINT)*isnull)!=CS_SUCCEED) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::outputBind(const char *variable,
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
	checkRePrepare();

	// see sapcursor::outputBind(...,char *value,...) for why this matters
	bool	isreturnvalue=(hasreturnvalue && !outbindindex);

	outbindtype[outbindindex]=CS_DATETIME_TYPE;
	outbinddates[outbindindex].year=year;
	outbinddates[outbindindex].month=month;
	outbinddates[outbindindex].day=day;
	outbinddates[outbindindex].hour=hour;
	outbinddates[outbindindex].minute=minute;
	outbinddates[outbindindex].second=second;
	outbinddates[outbindindex].microsecond=microsecond;
	outbinddates[outbindindex].tz=tz;
	outbinddates[outbindindex].isnegative=isnegative;
	outbindisnulls[outbindindex]=isnull;
	outbindindex++;

	if (isreturnvalue) {
		return true;
	}

	setParameterName(variable,variablesize);
	parameter[paramindex].datatype=CS_DATETIME_TYPE;
	parameter[paramindex].maxlength=CS_UNUSED;
	parameter[paramindex].status=CS_RETURN;
	parameter[paramindex].locale=NULL;
	if (ct_param(cmd,&parameter[paramindex],
			(CS_VOID *)NULL,0,
			(CS_SMALLINT)*isnull)!=CS_SUCCEED) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::executeQuery(const char *query, uint32_t size) {

	// clear out any errors.  do this before the re-prepare below, so
	// that if the re-prepare fails, its error survives to be reported.
	sapconn->errorcode=0;
	sapconn->liveconnection=true;

	checkRePrepare();

	// initialize row counts
	affectedrows=0;
	row=0;
	maxrow=0;
	totalrows=0;

	// initialize the result set state
	rowresultstype=0;
	freeRowBatches();

	if (cmd==cursorcmd) {
		if (ct_cursor(cursorcmd,CS_CURSOR_ROWS,
					NULL,CS_UNUSED,
					NULL,CS_UNUSED,
					(CS_INT)getFetchAtOnce())!=CS_SUCCEED) {
			closeResultSet();
			return false;
		}
		if (ct_cursor(cursorcmd,CS_CURSOR_OPEN,
					NULL,CS_UNUSED,
					NULL,CS_UNUSED,
					CS_UNUSED)!=CS_SUCCEED) {
			closeResultSet();
			return false;
		}

		// supply values for placeholders defined by inputBind()
		for (uint16_t i=0; i<paramindex; i++) {
			if (ct_param(cursorcmd,&parameter[i],
					inbindvalue[i],inbinddatasize[i],
					inbindindicator[i])!=CS_SUCCEED) {
				closeResultSet();
				return false;
			}
		}
	}

	// the command is dirty from here on - it has parameters queued on
	// it and is about to be sent - so make sure closeResultSet() will
	// actually clean it up rather than short-circuiting on "clean"
	clean=false;

	if (ct_send(cmd)!=CS_SUCCEED) {
		closeResultSet();
		return false;
	}

	if (!readResultSets()) {
		return false;
	}

	checkForTempTable(query,size);

	// reset the prepared flag
	prepared=false;

	// return success only if no error was generated
	return (!sapconn->errorcode);
}

bool sapcursor::readResultSets() {

	for (;;) {

		results=ct_results(cmd,&resultstype);

		// handle the end of all result sets
		if (results==CS_END_RESULTS) {
			break;
		}

		// handle failed queries
		if (results==CS_FAIL || resultstype==CS_CMD_FAIL) {
			closeResultSet();
			return false;
		}

		// Queries can generate multiple result sets.
		//
		// A DML/DDL query will just send a CS_CMD_SUCCEED.
		//
		// If we're not using cursors, then selects will also just
		// send a single CS_ROW_RESULT - unless the query is a
		// multi-statement tds LANGUAGE batch (see the CS_LANG_CMD
		// branch of prepareQuery()), which sends one CS_ROW_RESULT
		// (or CS_CMD_SUCCEED) per statement.
		//
		// If a cursor is used to execute a select, then each
		// ct_cursor() call generates a results set, and then the
		// ct_send also generates a CS_ROW_RESULT result set.
		//
		// RPC queries (EXEC some-stored-procedures or direct
		// TransactSQL may generate a series of result sets including
		// CS_CMD_SUCCEED, CS_CMD_DONE, CS_STATUS_RESULT, CS_ROW_RESULT
		// or CS_COMPUTE_RESULT result sets, in any combination or
		// order.
		//
		// Of the result sets that can carry rows - CS_ROW_RESULT,
		// CS_CURSOR_RESULT, CS_COMPUTE_RESULT and CS_PARAM_RESULT -
		// this pass only uses the first one and ignores the rest;
		// nextResultSet() picks this same loop back up on demand to
		// expose the following ones, one at a time.
		//
		// The CS_STATUS_RESULT and CS_PARAM_RESULT sets are different
		// though.  They don't carry the query's rows, they carry the
		// stored procedure's RETURN value and output parameters, and
		// ASE always sends them after the rows.  So they have to be
		// processed too, in the same pass, rather than breaking out of
		// this loop at the first result set that carries rows.
		//
		// Note that every result set that we consume has to be run out
		// to the end (see drainResultSet()) before the next
		// ct_results(), or ASE fails the next call with "cannot be
		// called until all fetchable results have been completely
		// processed".

		if (resultstype==CS_CMD_SUCCEED) {
			// If we got CS_CMD_SUCCEED, then try to get the
			// affected row count.  The query might have been
			// DML/DDL, or this could be one of the result sets of
			// a stored procedure or direct TransactSQL.  We need
			// to do this here because we're going to cancel this
			// result set below.
			affectedrows=0;
			if (ct_res_info(cmd,CS_ROW_COUNT,
				(CS_VOID *)&affectedrows,
				CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
				closeResultSet();
				return false;
			}
		} else if ((resultstype==CS_ROW_RESULT ||
				resultstype==CS_CURSOR_RESULT ||
				resultstype==CS_COMPUTE_RESULT) &&
				!rowresultstype) {

			rowresultstype=resultstype;

			if (!bindResultSetColumns()) {
				closeResultSet();
				return false;
			}

			// if the query has no output binds then the rows can
			// just be streamed to the client as it fetches them,
			// which is the common case by far, so stop here and
			// let fetchRow() take it from here
			if (!hasreturnvalue && !outbindindex) {
				break;
			}

			// otherwise the result sets that fill the output
			// binds are still to come - ASE sends them after the
			// rows - so buffer the rows and keep going
			if (!bufferRows()) {
				closeResultSet();
				return false;
			}
			rowsbuffered=true;
			currentbatch=firstbatch;
			row=0;
			continue;

		} else if (resultstype==CS_PARAM_RESULT &&
					(hasreturnvalue || outbindindex)) {

			// this carries the stored procedure's output
			// parameters.  it isn't the cursor's result set, it
			// just populates the output binds.
			if (!fetchOutputParams()) {
				closeResultSet();
				return false;
			}
			if (!drainResultSet()) {
				closeResultSet();
				return false;
			}
			continue;

		} else if (resultstype==CS_STATUS_RESULT && hasreturnvalue) {

			// this carries the stored procedure's RETURN value.
			// ct-lib reports it as its own 1-column result set
			// rather than folding it into the CS_PARAM_RESULT
			// row, so fetch it directly into the reserved first
			// output bind (see the hasreturnvalue comment on its
			// declaration) instead of canceling it like the other
			// result set types we don't care about.
			CS_DATAFMT	returnfmt;
			bytestring::zero(&returnfmt,sizeof(returnfmt));
			returnfmt.datatype=CS_INT_TYPE;
			returnfmt.format=CS_FMT_UNUSED;
			returnfmt.maxlength=sizeof(CS_INT);
			returnfmt.count=1;
			CS_INT		returnvalue=0;
			CS_INT		returnvaluelen=0;
			CS_SMALLINT	returnvalueind=0;
			if (ct_bind(cmd,1,&returnfmt,
					(CS_VOID *)&returnvalue,
					&returnvaluelen,
					&returnvalueind)!=CS_SUCCEED) {
				closeResultSet();
				return false;
			}
			CS_INT	rr=0;
			if (ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rr)!=CS_SUCCEED) {
				closeResultSet();
				return false;
			}
			if (outbindindex && outbindtype[0]==CS_INT_TYPE) {
				*outbindints[0]=(int64_t)returnvalue;
				if (outbindisnulls[0]) {
					*(outbindisnulls[0])=0;
				}
			}
			if (!drainResultSet()) {
				closeResultSet();
				return false;
			}
			continue;
		}

		// the result set was a type that we want to ignore
		if (!drainResultSet()) {
			closeResultSet();
			return false;
		}
	}

	return true;
}

bool sapcursor::nextResultSet(bool *nextresultsetavailable) {

	*nextresultsetavailable=false;

	// Only an un-cursored command (cmd==languagecmd - see the
	// CS_LANG_CMD branch of prepareQuery(), taken by a multi-statement
	// tds LANGUAGE batch, and by exec/execute/{call}) can have a second
	// row-bearing result set left to advance to:
	//  - a select cursor is always declared over a single statement's
	//    text (see isMultiStatementBatch()), so it never has more than
	//    the one result set the cursor's own ct_send() produced
	//  - a query with output binds already buffered every row-bearing
	//    result set it will ever expose, during executeQuery() (see
	//    rowsbuffered / bufferRows())
	//  - results!=CS_SUCCEED means readResultSets() already ran out to
	//    CS_END_RESULTS on the last pass, or the query never got this
	//    far in the first place - either way there's nothing left to
	//    read
	if (cmd!=languagecmd || rowsbuffered || results!=CS_SUCCEED) {
		return true;
	}

	// run out whatever's left of the current row-bearing result set -
	// ASE won't hand back the next one until this one has been
	// completely processed (see drainResultSet())
	if (!drainResultSet()) {
		return false;
	}

	// reset per-result-set state, the same way executeQuery() does
	// before reading the first one - affectedrows in particular, since
	// the controller re-reads getAffectedRows() right after a successful
	// advance (see sqlrservercontroller::nextResultSet()), and without
	// this a result set that isn't itself preceded by a fresh
	// CS_CMD_SUCCEED (eg. a plain select) would still be reporting
	// whatever the previous result set left behind
	affectedrows=0;
	row=0;
	maxrow=0;
	totalrows=0;
	rowresultstype=0;

	// pick the ct_results() loop back up where it left off
	if (!readResultSets()) {
		return false;
	}

	*nextresultsetavailable=(rowresultstype!=0);
	return true;
}

bool sapcursor::drainResultSet() {

	// every result set that gets consumed has to be run out to the end
	// before the next ct_results(), or ASE fails that call with "cannot
	// be called until all fetchable results have been completely
	// processed"
	if (ct_cancel(NULL,cmd,CS_CANCEL_CURRENT)==CS_FAIL) {
		sapconn->liveconnection=false;
		// FIXME: call ct_close(CS_FORCE_CLOSE)
		return false;
	}
	return true;
}

bool sapcursor::bindResultSetColumns() {

	// get the column count
	if (ct_res_info(cmd,CS_NUMDATA,(CS_VOID *)&ncols,
			CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
		return false;
	}

	// allocate buffers and limit column count if necessary
	uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
	if (!maxcolumncount) {
		// free any buffers left over from a previous result set of
		// this same query (see nextResultSet()) before sizing new
		// ones for this one - otherwise a query with more than one
		// row-bearing result set leaks a set of buffers per result
		// set advanced through
		deallocateResultSetBuffers();
		allocateResultSetBuffers(ncols);
	} else if ((uint32_t)ncols>maxcolumncount) {
		ncols=maxcolumncount;
	}

	// bind columns
	for (CS_INT i=0; i<ncols; i++) {

		// reset the column-bind
		column[i]=templatecolumn;

		// bind the columns for the fetches
		if (ct_bind(cmd,i+1,
				&column[i],
				(CS_VOID *)data[i],
				datasize[i],
				nullindicator[i])!=CS_SUCCEED) {
			break;
		}

		// describe the columns
		if (conn->cont->getSendColumnInfo()) {
			if (ct_describe(cmd,i+1,&column[i])!=CS_SUCCEED) {
				break;
			}
			deflateColumnSize(i);
		}
	}

	return true;
}

bool sapcursor::describeResultSetColumns() {

	// same as the CS_NUMDATA/ct_describe half of bindResultSetColumns(),
	// but against a CS_DESCRIBE_RESULT result set rather than a real one -
	// this is metadata only, no rows are being fetched, so there's no
	// ct_bind call and no need for data/nullindicator buffers.

	// get the column count
	if (ct_res_info(cmd,CS_NUMDATA,(CS_VOID *)&ncols,
			CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
		return false;
	}

	// allocate buffers and limit column count if necessary - same
	// convention as bindResultSetColumns(): an unbounded column count
	// means the buffers weren't sized at connect time, so size them for
	// this query now
	uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
	if (!maxcolumncount) {
		// free any buffers left over from a previous describe or
		// bind on this same cursor (eg. a re-execute that goes
		// through checkRePrepare()/prepareDynamic() again - see
		// bindResultSetColumns()) before sizing new ones for this one
		deallocateResultSetBuffers();
		allocateResultSetBuffers(ncols);
	} else if ((uint32_t)ncols>maxcolumncount) {
		ncols=maxcolumncount;
	}

	// describe the columns
	for (CS_INT i=0; i<ncols; i++) {
		column[i]=templatecolumn;
		if (ct_describe(cmd,i+1,&column[i])!=CS_SUCCEED) {
			return false;
		}
		deflateColumnSize(i);
	}

	return true;
}

bool sapcursor::fetchOutputParams() {

	// The CS_PARAM_RESULT result set is a single row with one column per
	// output parameter.  It's not the cursor's result set, it just fills
	// in the output binds, so it gets its own buffers here rather than
	// using the ones that the row result set uses.

	// get the column count
	CS_INT	paramcols=0;
	if (ct_res_info(cmd,CS_NUMDATA,(CS_VOID *)&paramcols,
			CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
		return false;
	}
	if (paramcols<1) {
		return true;
	}

	// When hasreturnvalue is set, outbind slot 0 is the reserved
	// RETURN-value slot, already populated from CS_STATUS_RESULT - it
	// isn't a real rpc parameter, so ASE's CS_PARAM_RESULT row doesn't
	// include a column for it.  Offset past it when mapping the row's
	// columns to outbind slots.
	CS_INT	outbindoffset=(hasreturnvalue)?1:0;

	// allocate buffers
	uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();
	CS_DATAFMT	*paramcolumn=new CS_DATAFMT[paramcols];
	char		**paramdata=new char *[paramcols];
	CS_INT		**paramdatasize=new CS_INT *[paramcols];
	CS_SMALLINT	**paramnullindicator=new CS_SMALLINT *[paramcols];
	for (CS_INT i=0; i<paramcols; i++) {
		paramdata[i]=new char[maxfieldsize];
		paramdatasize[i]=new CS_INT[1];
		paramnullindicator[i]=new CS_SMALLINT[1];
	}

	// bind columns
	bool	success=true;
	for (CS_INT i=0; i<paramcols; i++) {

		paramcolumn[i]=templatecolumn;
		paramcolumn[i].count=1;

		// if the stored procedure returns dates, then use the
		// datetime type instead of the template's char type
		CS_INT	outidx=i+outbindoffset;
		if (outidx<(CS_INT)outbindindex &&
			outbindtype[outidx]==CS_DATETIME_TYPE) {
			paramcolumn[i].datatype=CS_DATETIME_TYPE;
			paramcolumn[i].format=CS_FMT_UNUSED;
			paramcolumn[i].maxlength=sizeof(CS_DATETIME);
		}

		if (ct_bind(cmd,i+1,&paramcolumn[i],
				(CS_VOID *)paramdata[i],
				paramdatasize[i],
				paramnullindicator[i])!=CS_SUCCEED) {
			success=false;
			break;
		}
	}

	// fetch the single row
	CS_INT	paramrows=0;
	if (success && (ct_fetch(cmd,CS_UNUSED,
					CS_UNUSED,
					CS_UNUSED,
					&paramrows)!=CS_SUCCEED || !paramrows)) {
		success=false;
	}

	// copy data into the output bind values
	if (success) {

		CS_INT	availableoutbinds=outbindindex-outbindoffset;
		CS_INT	maxindex=availableoutbinds;
		if (paramcols<availableoutbinds) {
			// this shouldn't happen...
			maxindex=paramcols;
		}
		for (CS_INT i=0; i<maxindex; i++) {
			CS_INT	outidx=i+outbindoffset;
			if (outbindtype[outidx]==CS_CHAR_TYPE) {
				*(outbindisnulls[outidx])=
						*paramnullindicator[i];
				CS_INT	size=outbindstringsizes[outidx];
				if (paramdatasize[i][0]<size) {
					size=paramdatasize[i][0];
				}
				bytestring::copy(outbindstrings[outidx],
							paramdata[i],size);
			} else if (outbindtype[outidx]==CS_INT_TYPE) {
				*outbindints[outidx]=
					charstring::convertToInteger(
								paramdata[i]);
			} else if (outbindtype[outidx]==CS_FLOAT_TYPE) {
				*outbinddoubles[outidx]=
					charstring::convertToFloatC(
								paramdata[i]);

				// The value is bound as a float, whatever the
				// parameter really is, so it's the only thing
				// that comes back through it.  The server's
				// own description of the parameter still has
				// the precision and scale, and a numeric or
				// decimal needs them - 5.0000 is a
				// numeric(10,4), not a float 5.
				if (outbindprecisions[outidx] ||
						outbindscales[outidx]) {
					CS_DATAFMT	paramfmt;
					bytestring::zero(&paramfmt,
							sizeof(CS_DATAFMT));
					if (ct_describe(cmd,i+1,&paramfmt)==
								CS_SUCCEED) {
						if (outbindprecisions
								[outidx]) {
							*outbindprecisions
								[outidx]=
								(uint32_t)
								paramfmt.
								precision;
						}
						if (outbindscales[outidx]) {
							*outbindscales
								[outidx]=
								(uint32_t)
								paramfmt.scale;
						}
					}
				}
			} else if (outbindtype[outidx]==CS_DATETIME_TYPE) {

				// convert to a CS_DATEREC
				CS_DATEREC	dr;
				bytestring::zero(&dr,sizeof(CS_DATEREC));
				cs_dt_crack(sapconn->context,
						CS_DATETIME_TYPE,
						(CS_VOID *)paramdata[i],&dr);

				datebind	*db=&outbinddates[outidx];
				*(db->year)=dr.dateyear;
				*(db->month)=dr.datemonth+1;
				*(db->day)=dr.datedmonth;
				*(db->hour)=dr.datehour;
				*(db->minute)=dr.dateminute;
				*(db->second)=dr.datesecond;
				*(db->microsecond)=dr.datesecfrac;
				*(db->tz)=NULL;
				*(db->isnegative)=false;
			}
		}
	}

	// clean up
	for (CS_INT i=0; i<paramcols; i++) {
		delete[] paramdata[i];
		delete[] paramdatasize[i];
		delete[] paramnullindicator[i];
	}
	delete[] paramcolumn;
	delete[] paramdata;
	delete[] paramdatasize;
	delete[] paramnullindicator;

	return success;
}

bool sapcursor::bufferRows() {

	// fetch and buffer the entire result set, so that executeQuery()'s
	// loop can get on to the status/parameter result sets that follow it.
	// a maxbufferedrows of 0 means unlimited (see DEFAULT_MAX_BUFFERED_ROWS).
	uint64_t	maxbufferedrows=sapconn->maxbufferedrows;
	for (;;) {

		CS_RETCODE	result=ct_fetch(cmd,CS_UNUSED,
							CS_UNUSED,
							CS_UNUSED,
							&rowsread);
		if (result==CS_END_DATA ||
				(result==CS_SUCCEED && !rowsread)) {
			return true;
		}

		// a bad row shouldn't fail the whole query - it's tolerated
		// the same way fetchRow() tolerates it on the streaming path
		// (see fetchRow() below) - so just stop buffering and return
		// whatever rows were successfully buffered so far.
		if (result==CS_ROW_FAIL) {
			if (conn->cont->getDebug()) {
				conn->cont->raiseDebugWriteEvent(
					"sap: CS_ROW_FAIL while buffering "
					"rows, returning the %lld row(s) "
					"buffered so far",
					(long long)bufferedrows);
			}
			return true;
		}
		if (result!=CS_SUCCEED) {
			return false;
		}

		// if the result set is too big to buffer, then trim this
		// batch down to the precise number of rows that still fit,
		// discard the rest of the result set, and return the rows
		// that we have.  the query still succeeds, it just returns
		// a truncated result set, rather than using an unbounded
		// amount of memory.
		if (maxbufferedrows && bufferedrows+rowsread>maxbufferedrows) {
			CS_INT	keep=(CS_INT)(maxbufferedrows-bufferedrows);
			if (keep>0 && !appendRowBatch(keep)) {
				return false;
			}
			bufferedrows=bufferedrows+keep;
			if (conn->cont->getDebug()) {
				conn->cont->raiseDebugWriteEvent(
					"sap: result set exceeded "
					"maxbufferedrows (%lld), truncating "
					"to %lld row(s)",
					(long long)maxbufferedrows,
					(long long)bufferedrows);
			}
			return drainResultSet();
		}

		if (!appendRowBatch(rowsread)) {
			return false;
		}
		bufferedrows=bufferedrows+rowsread;
	}
}

bool sapcursor::appendRowBatch(CS_INT rows) {

	uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();

	sapresultsetbatch	*batch=new sapresultsetbatch;
	batch->cols=ncols;
	batch->rows=rows;
	batch->next=NULL;
	batch->data=new char *[ncols];
	batch->datasize=new CS_INT *[ncols];
	batch->nullindicator=new CS_SMALLINT *[ncols];
	for (CS_INT i=0; i<ncols; i++) {
		batch->data[i]=new char[rows*maxfieldsize];
		bytestring::copy(batch->data[i],data[i],rows*maxfieldsize);
		batch->datasize[i]=new CS_INT[rows];
		bytestring::copy(batch->datasize[i],datasize[i],
						rows*sizeof(CS_INT));
		batch->nullindicator[i]=new CS_SMALLINT[rows];
		bytestring::copy(batch->nullindicator[i],nullindicator[i],
						rows*sizeof(CS_SMALLINT));
	}

	if (lastbatch) {
		lastbatch->next=batch;
	} else {
		firstbatch=batch;
	}
	lastbatch=batch;

	return true;
}

void sapcursor::freeRowBatches() {

	sapresultsetbatch	*batch=firstbatch;
	while (batch) {
		sapresultsetbatch	*next=batch->next;
		for (CS_INT i=0; i<batch->cols; i++) {
			delete[] batch->data[i];
			delete[] batch->datasize[i];
			delete[] batch->nullindicator[i];
		}
		delete[] batch->data;
		delete[] batch->datasize;
		delete[] batch->nullindicator;
		delete batch;
		batch=next;
	}

	firstbatch=NULL;
	lastbatch=NULL;
	currentbatch=NULL;
	bufferedrows=0;
	rowsbuffered=false;
}

uint64_t sapcursor::getAffectedRows() {
	// sap can set affectedrows to -1 when a DDL query is run
	return (affectedrows>=0)?affectedrows:0;
}

uint32_t sapcursor::colCount() {
	return ncols;
}

const char *sapcursor::getColumnName(uint32_t col) {
	return column[col].name;
}

uint16_t sapcursor::getColumnType(uint32_t col) {
	switch (column[col].datatype) {
		case CS_CHAR_TYPE:
			// ctlib reports char, varchar, nchar and nvarchar all
			// as CS_CHAR_TYPE.  Sybase also sends its systypes
			// usertype, which does tell them apart - 1 char,
			// 2 varchar, 24 nchar, 25 nvarchar.  sysname and
			// longsysname send 18 and 42, and a type created with
			// sp_addtype sends 100 or higher, so all of those fall
			// through to char.
			switch (column[col].usertype) {
				case 2:
					return VARCHAR_DATATYPE;
				case 24:
					return NCHAR_DATATYPE;
				case 25:
					return NVARCHAR_DATATYPE;
			}
			return CHAR_DATATYPE;
		#ifdef CS_UNICHAR_TYPE
		case CS_UNICHAR_TYPE:
			// ctlib reports unichar and univarchar both as
			// CS_UNICHAR_TYPE.  Sybase also sends its systypes
			// usertype, which does tell them apart - 34 unichar,
			// 35 univarchar.  These don't move to a long type
			// under a multi-byte charset, the way the
			// CS_CHAR_TYPE ones do - they're already ucs-2.
			if (column[col].usertype==35) {
				return NVARCHAR_DATATYPE;
			}
			return NCHAR_DATATYPE;
		#endif
		case CS_INT_TYPE:
			return INT_DATATYPE;
		case CS_SMALLINT_TYPE:
			return SMALLINT_DATATYPE;
		case CS_TINYINT_TYPE:
			return TINYINT_DATATYPE;
		case CS_MONEY_TYPE:
			return MONEY_DATATYPE;
		case CS_DATETIME_TYPE:
			return DATETIME_DATATYPE;
		case CS_NUMERIC_TYPE:
			return NUMERIC_DATATYPE;
		case CS_DECIMAL_TYPE:
			return DECIMAL_DATATYPE;
		case CS_DATETIME4_TYPE:
			return SMALLDATETIME_DATATYPE;
		case CS_MONEY4_TYPE:
			return SMALLMONEY_DATATYPE;
		case CS_IMAGE_TYPE:
			return IMAGE_DATATYPE;
		case CS_BINARY_TYPE:
			// ctlib reports binary and varbinary both as
			// CS_BINARY_TYPE.  Sybase also sends its systypes
			// usertype, which does tell them apart - 3 binary,
			// 4 varbinary.  timestamp is binary(8) with usertype
			// 80, so it falls through to binary, the same way it
			// does in the freetds module.
			if (column[col].usertype==4) {
				return VARBINARY_DATATYPE;
			}
			return BINARY_DATATYPE;
		case CS_BIT_TYPE:
			return BIT_DATATYPE;
		case CS_REAL_TYPE:
			return REAL_DATATYPE;
		case CS_FLOAT_TYPE:
			return FLOAT_DATATYPE;
		case CS_TEXT_TYPE:
			return TEXT_DATATYPE;
		#ifdef CS_UNITEXT_TYPE
		case CS_UNITEXT_TYPE:
			// unitext only comes back as CS_UNITEXT_TYPE at the
			// newer context versions.  At CS_VERSION_100 Open
			// Client reports it as CS_IMAGE_TYPE, so there it
			// falls through to image.
			return NTEXT_DATATYPE;
		#endif
		case CS_VARCHAR_TYPE:
			return VARCHAR_DATATYPE;
		case CS_VARBINARY_TYPE:
			return VARBINARY_DATATYPE;
		case CS_LONGCHAR_TYPE:
			// char, varchar, nchar, nvarchar and sysname come back
			// as CS_LONGCHAR_TYPE rather than CS_CHAR_TYPE once
			// the column is wider than 255 bytes, or once
			// @@client_csexpansion is above 1.  The usertype is
			// the same either way, so this splits them the same
			// way the CS_CHAR_TYPE arm does - a char(1000) is
			// still a char (every usertype that can arrive here
			// was measured arriving as CS_CHAR_TYPE too, so
			// nothing needs to fall through to LONGCHAR).
			switch (column[col].usertype) {
				case 2:
					return VARCHAR_DATATYPE;
				case 24:
					return NCHAR_DATATYPE;
				case 25:
					return NVARCHAR_DATATYPE;
			}
			return CHAR_DATATYPE;
		case CS_LONGBINARY_TYPE:
			// and binary and varbinary the same way, past 255
			// bytes - 3 binary, 4 varbinary, as in the
			// CS_BINARY_TYPE arm
			if (column[col].usertype==4) {
				return VARBINARY_DATATYPE;
			}
			return BINARY_DATATYPE;
		case CS_LONG_TYPE:
			return LONG_DATATYPE;
		case CS_ILLEGAL_TYPE:
			return ILLEGAL_DATATYPE;
		case CS_SENSITIVITY_TYPE:
			return SENSITIVITY_DATATYPE;
		case CS_BOUNDARY_TYPE:
			return BOUNDARY_DATATYPE;
		case CS_VOID_TYPE:
			return VOID_DATATYPE;
		case CS_USHORT_TYPE:
			return USHORT_DATATYPE;
		#ifdef CS_BIGINT_TYPE
		case CS_BIGINT_TYPE:
			return BIGINT_DATATYPE;
		#endif
		#ifdef CS_UBIGINT_TYPE
		case CS_UBIGINT_TYPE:
			return UBIGINT_DATATYPE;
		#endif
		#ifdef CS_DATE_TYPE
		case CS_DATE_TYPE:
			return DATE_DATATYPE;
		#endif
		#ifdef CS_TIME_TYPE
		case CS_TIME_TYPE:
			return TIME_DATATYPE;
		#endif
		#ifdef CS_BIGTIME_TYPE
		case CS_BIGTIME_TYPE:
			return TIME_DATATYPE;
		#endif
		#ifdef CS_BIGDATETIME_TYPE
		case CS_BIGDATETIME_TYPE:
			// usertype 43 is datetimeoffset, which only freetds
			// synthesizes.  Sybase sends 48 for bigdatetime, and
			// its systypes 43 is bigint, which can't arrive as
			// CS_BIGDATETIME_TYPE, so this test is inert here.
			// It's kept so the two modules read the same.
			if (column[col].usertype==43) {
				return DATETIMEOFFSET_DATATYPE;
			}
			return TIMESTAMP_DATATYPE;
		#endif
		default:
			return UNKNOWN_DATATYPE;
	}
}

void sapcursor::deflateColumnSize(CS_INT index) {

	// bail if the client charset isn't expanding - most connections
	// don't set an expanding charset=, and @@client_csexpansion is 1
	if (sapconn->csexpansion<2) {
		return;
	}

	// Only some character types are reported at their byte width once
	// @@client_csexpansion is above 1 - measured directly against a live
	// ASE.  CS_UNICHAR_TYPE (unichar/univarchar) is
	// already ucs-2 and isn't multiplied - see the comment in
	// getColumnType() above.  Binary types and CS_UNITEXT_TYPE/
	// CS_IMAGE_TYPE aren't character data and aren't multiplied either.
	switch (column[index].datatype) {
		case CS_CHAR_TYPE:
		case CS_LONGCHAR_TYPE:
		case CS_VARCHAR_TYPE:
		case CS_TEXT_TYPE:
			break;
		default:
			return;
	}

	column[index].maxlength/=sapconn->csexpansion;
}

uint32_t sapcursor::getColumnSize(uint32_t col) {
	// limit the column size
	uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();
	if ((uint32_t)column[col].maxlength>maxfieldsize) {
		column[col].maxlength=maxfieldsize;
	}
	return column[col].maxlength;
}

uint32_t sapcursor::getColumnPrecision(uint32_t col) {
	return column[col].precision;
}

uint32_t sapcursor::getColumnScale(uint32_t col) {
	return column[col].scale;
}

uint16_t sapcursor::getColumnIsNullable(uint32_t col) {
	return (column[col].status&CS_CANBENULL);
}

uint16_t sapcursor::getColumnIsPartOfKey(uint32_t col) {
	return (column[col].status&(CS_KEY|CS_VERSION_KEY));
}

uint16_t sapcursor::getColumnIsUnsigned(uint32_t col) {
	return (getColumnType(col)==USHORT_DATATYPE);
}

uint16_t sapcursor::getColumnIsBinary(uint32_t col) {
	return (getColumnType(col)==IMAGE_DATATYPE);
}

uint16_t sapcursor::getColumnIsAutoIncrement(uint32_t col) {
	return (column[col].status&CS_IDENTITY);
}

bool sapcursor::noRowsToReturn() {
	// unless the query was a successful select, send no data
	return (rowresultstype!=CS_ROW_RESULT &&
			rowresultstype!=CS_CURSOR_RESULT &&
			rowresultstype!=CS_COMPUTE_RESULT);
}

bool sapcursor::skipRow(bool *error) {
	if (fetchRow(error)) {
		row++;
		return true;
	}
	return false;
}

bool sapcursor::fetchRow(bool *error) {

	*error=false;
	// FIXME: set error if an error occurs

	// if the rows were buffered during executeQuery(), then just walk
	// the list of batches
	if (rowsbuffered) {
		if (currentbatch && row==currentbatch->rows) {
			currentbatch=currentbatch->next;
			row=0;
		}
		return (currentbatch!=NULL);
	}

	if (row==(CS_INT)getFetchAtOnce()) {
		row=0;
	}
	if (row>0 && row==maxrow) {
		return false;
	}
	if (!row) {
		CS_RETCODE	result=ct_fetch(cmd,CS_UNUSED,
							CS_UNUSED,
							CS_UNUSED,
							&rowsread);
		if (result!=CS_SUCCEED || !rowsread) {
			if (result==CS_FAIL || result==CS_ROW_FAIL) {
				*error=true;
			}
			return false;
		}
		maxrow=rowsread;
		totalrows=totalrows+rowsread;
	}
	return true;
}

void sapcursor::getField(uint32_t col,
				const char **field, uint64_t *fieldsize,
				bool *lob, bool *null) {

	// if the rows were buffered during executeQuery(), then read from
	// the current batch rather than from the ct_bind() buffers
	char		**rowdata=data;
	CS_INT		**rowdatasize=datasize;
	CS_SMALLINT	**rownullindicator=nullindicator;
	if (rowsbuffered) {
		if (!currentbatch) {
			*null=true;
			return;
		}
		rowdata=currentbatch->data;
		rowdatasize=currentbatch->datasize;
		rownullindicator=currentbatch->nullindicator;
	}

	// handle NULLs
	if (rownullindicator[col][row]==-1) {
		*null=true;
		return;
	}

	// handle normal datatypes
	// (a zero-length value has no null terminator, and datasize 0,
	// so trimming it would underflow - datasize is also signed, and
	// ctlib leaves it unset on a couple of internal failure paths,
	// so guard on >0 rather than just non-zero)
	char		*d=&rowdata[col][row*conn->cont->getMaxFieldSize()];
	uint64_t	ds=(rowdatasize[col][row]>0)?(rowdatasize[col][row]-1):0;

	// Note: image columns are fetched with the same CS_CHAR_TYPE/
	// CS_FMT_NULLTERM ct_bind() format as everything else (see
	// templatecolumn in open()), and ct-lib's own binary-to-char
	// conversion already hands back the column's real bytes - it does
	// not hex-encode them.  decodeBlob() (a hex decode, the counterpart
	// of encodeBlob()'s hex-encode used for embedding blob literals in
	// query text) must not be run on that data - doing so corrupts the
	// value by decoding literal text as if it were hex.  binary and
	// varbinary columns are already handled the same way, with no
	// decode step, so this just brings image in line with them.

	// return the field and field size
	*field=d;
	*fieldsize=ds;
}

void sapcursor::nextRow() {
	row++;
}

void sapcursor::closeResultSet() {

	// Reset the prepared flag here, rather than only on executeQuery()'s
	// success path.  Sybase doesn't allow a command to be re-sent
	// without being re-prepared (see checkRePrepare()), and this runs on
	// every cleanup path, including the failure paths.  If it were left
	// set after a failed execute, then the next execute would re-send
	// the same command object, with the previous ct_param()s still
	// queued on it, and ASE would reject the leftover PARAM datastream.
	prepared=false;

	freeRowBatches();

	if (clean) {
		return;
	}

	discardResults();
	discardCursor();
	discardDynamic();

	clean=true;
}

void sapcursor::discardResults() {

	// if there are any unprocessed result sets, process them
	if (results==CS_SUCCEED) {
		do {
			if (ct_cancel(NULL,cmd,CS_CANCEL_CURRENT)==CS_FAIL) {
				sapconn->liveconnection=false;
				// FIXME: call ct_close(CS_FORCE_CLOSE)?
			}
			results=ct_results(cmd,&resultstype);
		} while (results==CS_SUCCEED);
	}

	// also clears a prepared-but-unsent command (eg. a failed bind)
	if (ct_cancel(NULL,cmd,CS_CANCEL_ALL)==CS_FAIL) {
		sapconn->liveconnection=false;
		// FIXME: call ct_close(CS_FORCE_CLOSE)?
	}

	if (!conn->cont->getMaxColumnCount()) {
		deallocateResultSetBuffers();
	}
}


void sapcursor::discardDynamic() {

	// drop the statement that prepareDynamic() prepared, if there is
	// one.  the command has to be idle at this point - discardResults()
	// runs first when this is called from closeResultSet().
	if (!dynamicprepared) {
		return;
	}
	dynamicprepared=false;

	if (ct_dynamic(cursorcmd,CS_DEALLOC,
			(CS_CHAR *)dynamicname,(CS_INT)dynamicnamesize,
			(CS_CHAR *)NULL,CS_UNUSED)!=CS_SUCCEED ||
			ct_send(cursorcmd)!=CS_SUCCEED) {
		return;
	}

	CS_INT	res;
	CS_INT	restype;
	while ((res=ct_results(cursorcmd,&restype))==CS_SUCCEED) {
		if (ct_cancel(NULL,cursorcmd,CS_CANCEL_CURRENT)==CS_FAIL) {
			sapconn->liveconnection=false;
			return;
		}
	}
}

void sapcursor::discardCursor() {
	if (cmd==cursorcmd) {
		if (ct_cursor(cursorcmd,CS_CURSOR_CLOSE,
					NULL,CS_UNUSED,
					NULL,CS_UNUSED,
					CS_DEALLOC)==CS_SUCCEED) {
			if (ct_send(cursorcmd)==CS_SUCCEED) {
				results=ct_results(cmd,&resultstype);
				discardResults();
			}
		}
	}
}

CS_RETCODE sapconnection::csMessageCallback(CS_CONTEXT *ctxt, 
						CS_CLIENTMSG *msgp) {
	if (errorcode) {
		return CS_SUCCEED;
	}

	errorcode=msgp->msgnumber;

	errorstring.clear();
	errorstring.append("Client Library error: ")->append(msgp->msgstring);
	errorstring.append(" severity(")->
		append((int32_t)CS_SEVERITY(msgp->msgnumber))->append(")");
	errorstring.append(" layer(")->
		append((int32_t)CS_LAYER(msgp->msgnumber))->append(")");
	errorstring.append(" origin(")->
		append((int32_t)CS_ORIGIN(msgp->msgnumber))->append(")");
	errorstring.append(" number(")->
		append((int32_t)CS_NUMBER(msgp->msgnumber))->append(")");

	if (msgp->osstringlen>0) {
		errorstring.append("  Operating System Error: ");
		errorstring.append(msgp->osstring);
	}

	// for a timeout message,
	// set liveconnection to false
	if (CS_SEVERITY(msgp->msgnumber)==CS_SV_RETRY_FAIL &&
		CS_LAYER(msgp->msgnumber)==63 &&
		CS_ORIGIN(msgp->msgnumber)==63 &&
		CS_NUMBER(msgp->msgnumber)==63) {
		liveconnection=false;
	} else
	// for a net-libraryoperation terminated due to disconnect,
	// set liveconnection to false
	if (CS_SEVERITY(msgp->msgnumber)==5 &&
		CS_LAYER(msgp->msgnumber)==5 &&
		CS_ORIGIN(msgp->msgnumber)==3 &&
		CS_NUMBER(msgp->msgnumber)==6) {
		liveconnection=false;
	}
	// FIXME: freetds connection has another case, do we need it?

	return CS_SUCCEED;
}

CS_RETCODE sapconnection::clientMessageCallback(CS_CONTEXT *ctxt, 
						CS_CONNECTION *cnn,
						CS_CLIENTMSG *msgp) {
	if (errorcode) {
		return CS_SUCCEED;
	}

	errorcode=msgp->msgnumber;

	errorstring.clear();
	errorstring.append("Client Library error: ")->append(msgp->msgstring);
	errorstring.append(" severity(")->
		append((int32_t)CS_SEVERITY(msgp->msgnumber))->append(")");
	errorstring.append(" layer(")->
		append((int32_t)CS_LAYER(msgp->msgnumber))->append(")");
	errorstring.append(" origin(")->
		append((int32_t)CS_ORIGIN(msgp->msgnumber))->append(")");
	errorstring.append(" number(")->
		append((int32_t)CS_NUMBER(msgp->msgnumber))->append(")");
	if (msgp->osstringlen>0) {
		errorstring.append("  Operating System Error: ");
		errorstring.append(msgp->osstring);
	}

	// for a timeout message,
	// set liveconnection to false
	if (CS_SEVERITY(msgp->msgnumber)==CS_SV_RETRY_FAIL &&
		CS_NUMBER(msgp->msgnumber)==63 &&
		CS_ORIGIN(msgp->msgnumber)==63 &&
		CS_LAYER(msgp->msgnumber)==63) {
		liveconnection=false;
	} else
	// for a net-libraryoperation terminated due to disconnect,
	// set liveconnection to false
	if (CS_SEVERITY(msgp->msgnumber)==5 &&
		CS_LAYER(msgp->msgnumber)==5 &&
		CS_ORIGIN(msgp->msgnumber)==3 &&
		CS_NUMBER(msgp->msgnumber)==6) {
		liveconnection=false;
	}
	// FIXME: freetds connection has another case, do we need it?

	return CS_SUCCEED;
}

CS_RETCODE sapconnection::serverMessageCallback(CS_CONTEXT *ctxt, 
						CS_CONNECTION *cnn,
						CS_SERVERMSG *msgp) {

	// This is a special case, for some reason, "use db" queries
	// throw a warning, ignore them.
	if ((CS_NUMBER(msgp->msgnumber)==5701 &&
			CS_SEVERITY(msgp->msgnumber)==10) ||
		(CS_NUMBER(msgp->msgnumber)==69 &&
			CS_SEVERITY(msgp->msgnumber)==22)) {
		return CS_SUCCEED;
	}

	if (errorcode) {
		return CS_SUCCEED;
	}

	errorcode=msgp->msgnumber;

	errorstring.clear();
	errorstring.append("Server message: ")->append(msgp->text);
	errorstring.append(" severity(")->
		append((int32_t)CS_SEVERITY(msgp->msgnumber))->append(")");
	errorstring.append(" number(")->
		append((int32_t)CS_NUMBER(msgp->msgnumber))->append(")");
	errorstring.append(" state(")->
		append((int32_t)msgp->state)->append(")");
	errorstring.append(" line(")->
		append((int32_t)msgp->line)->append(")");
	errorstring.append("  Server Name: ")->append(msgp->svrname);
	errorstring.append("  Procedure Name: ")->append(msgp->proc);

	return CS_SUCCEED;
}

const char *sapconnection::tempTablePrefix() {
	return "#";
}

sqlrtxmodel_t sapconnection::getNativeTransactionModel() {
	return SQLRTXMODEL_EXPLICIT_ERROR;
}

bool sapconnection::commit() {
	cont->closeAllResultSets();
	return sqlrserverconnection::commit();
}

bool sapconnection::rollback() {
	cont->closeAllResultSets();
	return sqlrserverconnection::rollback();
}

void sapconnection::getError(char *errorbuffer,
				uint32_t errorbuffersize,
				uint32_t *errorsize,
				int64_t *errorcode,
				bool *liveconnection) {
	*errorsize=this->errorstring.getStringLength();
	if (*errorsize>=errorbuffersize) {
		*errorsize=(errorbuffersize)?errorbuffersize-1:0;
	}
	charstring::safeCopy(errorbuffer,errorbuffersize,
				this->errorstring.getString(),*errorsize);
	if (errorbuffersize) {
		errorbuffer[*errorsize]='\0';
	}
	*liveconnection=this->liveconnection;
	*errorcode=this->errorcode;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_sapconnection(
						sqlrservercontroller *cont) {
		return new sapconnection(cont);
	}
}
