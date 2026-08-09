// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/environment.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/charstring.h>
#include <rudiments/character.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>
#include <rudiments/snooze.h>

#include <datatypes.h>
#include <defines.h>
#include <config.h>

extern "C" {
	#include <ctpublic.h>
}

#ifdef HAVE_FREETDS_H
	#include <tdsver.h>
#endif

#ifndef HAVE_FREETDS_FUNCTION_DEFINITIONS
// in freetds prior to 0.61, the cs_ functions are not defined in any header
// file. C allows that...  C++ does not, so here they are
extern "C" {

extern	CS_INT	cs_ctx_alloc(CS_INT,CS_CONTEXT **);
extern	CS_INT	ct_init(CS_CONTEXT *,CS_INT);
extern	CS_INT	ct_callback(CS_CONTEXT *,CS_CONNECTION *,
					CS_INT,CS_INT,CS_VOID *);
extern	CS_INT	cs_config(CS_CONTEXT *,CS_INT,CS_INT,CS_VOID *,CS_INT,CS_INT *);
extern	CS_INT	ct_con_alloc(CS_CONTEXT *,CS_CONNECTION **);
extern	CS_INT	ct_con_props(CS_CONNECTION *,CS_INT,CS_INT,CS_VOID *,CS_INT,CS_INT *);
extern	CS_INT	cs_loc_alloc(CS_CONTEXT *,CS_LOCALE **);
extern	CS_INT	cs_locale(CS_CONTEXT *,CS_INT,CS_LOCALE *,CS_INT,CS_CHAR *,CS_INT,CS_INT *);
extern	CS_INT	ct_connect(CS_CONNECTION *,CS_CHAR *,CS_INT);
extern	CS_INT	ct_close(CS_CONNECTION *,CS_INT);
extern	CS_INT	ct_con_drop(CS_CONNECTION *);
extern	CS_INT	ct_exit(CS_CONTEXT *,CS_INT);
extern	CS_INT	cs_ctx_drop(CS_CONTEXT *);
extern	CS_INT	ct_cmd_alloc(CS_CONNECTION *,CS_COMMAND **);
extern	CS_INT	ct_command(CS_COMMAND *,CS_INT,CS_CHAR *,CS_INT,CS_INT);
extern	CS_INT	ct_send(CS_COMMAND *);
extern	CS_INT	ct_results(CS_COMMAND *,CS_INT *);
extern	CS_INT	ct_res_info(CS_COMMAND *,CS_INT,CS_VOID *,CS_INT,CS_INT *);
extern	CS_INT	ct_describe(CS_COMMAND *,CS_INT,CS_DATAFMT *);
extern	CS_INT	ct_bind(CS_COMMAND *,CS_INT,CS_DATAFMT *,CS_VOID *,CS_INT *,CS_SMALLINT *);
extern	CS_INT	ct_fetch(CS_COMMAND *,CS_INT,CS_INT,CS_INT,CS_INT *);
extern	CS_INT	ct_cmd_drop(CS_COMMAND *);
extern	CS_INT	cs_convert(CS_CONTEXT *,CS_DATAFMT *,CS_VOID *,CS_DATAFMT *,CS_VOID *,CS_INT *);
extern	CS_INT	cs_loc_drop(CS_CONTEXT *,CS_LOCALE *);
extern	CS_INT	ct_cancel(CS_CONNECTION *,CS_COMMAND *,CS_INT);
extern	CS_INT	ct_dynamic(CS_COMMAND *,CS_INT,CS_CHAR *,CS_INT,CS_CHAR *,CS_INT);
extern	CS_INT	cs_dt_crack(CS_CONTEXT *,CS_INT,CS_VOID *,CS_DATEREC *);

}
#endif

// this is here in case freetds ever supports cursors.  Verified live
// (against a real ASE, via freetds) that freetds does NOT reliably support
// ct_cursor()-based select cursors or ct_param()-based binds on plain
// language commands - turning this on breaks ordinary selects and DML.
// rpc commands (ct_command(CS_RPC_CMD) plus ct_param(), used by the
// exec/execute/{call} branches of prepareQuery()) were verified live to
// work correctly though, so that part isn't gated by this define; see
// prepareQuery(), supportsNativeBinds() and executeQuery() below.
//#define FREETDS_SUPPORTS_CURSORS

// some versions of freetds don't define this
#ifndef CS_UNSUPPORTED
	#define CS_UNSUPPORTED -10
#endif

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

class freetdsconnection;

class SQLRSERVER_DLLSPEC freetdscursor : public sqlrservercursor {
	friend class freetdsconnection;
	private:
		freetdscursor(sqlrserverconnection *conn, uint16_t id);
		~freetdscursor();
		void		allocateResultSetBuffers(int32_t columncount);
		void		deallocateResultSetBuffers();
		bool		open();
		bool		close();
		bool		prepareQuery(const char *query,
							uint32_t size);
		bool		supportsNativeBinds(const char *query,
							uint32_t size);
		void		encodeBlob(stringbuffer *buffer,
							const char *data,
							uint32_t datasize);
		void		decodeBlob(char **data, uint32_t *datasize);
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
		bool		knowsAffectedRows();
		uint64_t	getAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
		uint16_t	getColumnType(uint32_t col);
		// undoes the multiplication that freetds applies to the
		// size of character columns when the client charset is a
		// multi-byte charset
		void		deflateColumnSize(CS_INT index);
		uint32_t	getColumnSize(uint32_t col);
		uint32_t	getColumnPrecision(uint32_t col);
		uint32_t	getColumnScale(uint32_t col);
		uint16_t	getColumnIsNullable(uint32_t col);
		uint16_t	getColumnIsPartOfKey(uint32_t col);
		uint16_t	getColumnIsUnsigned(uint32_t col);
		uint16_t	getColumnIsBinary(uint32_t col);
		uint16_t	getColumnIsAutoIncrement(uint32_t col);
		bool		ignoreDateDdMmParameter(const char *data,
							uint32_t size);
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

		char		*cursorname;
		size_t		cursornamesize;

		void		checkRePrepare();
		bool		inputBind(CS_VOID *value,
						CS_INT valuesize,
						CS_SMALLINT indicator);
		bool		parseRpcParams(const char *p);

		// true when the current command is an rpc command whose
		// ct_command(CS_RPC_CMD) has already been issued by
		// prepareQuery(); executeQuery() uses this to know whether it
		// still needs to send the query as a plain language command
		bool		rpc;

		uint32_t	majorversion;
		uint32_t	minorversion;
		uint32_t	patchlevel;

		CS_COMMAND	*languagecmd;
		CS_COMMAND	*cursorcmd;
		CS_COMMAND	*cmd;
		CS_INT		results;
		CS_INT		resultstype;
		CS_INT		ncols;
		bool		knowsaffectedrows;
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
		datebind	*outbinddates;
		uint16_t	outbindindex;

		int32_t		columncount;
		CS_DATAFMT	templatecolumn;
		CS_DATAFMT	*column;
		char		**data;
		CS_INT		**datasize;
		CS_SMALLINT	**nullindicator;

		char		*query;
		uint32_t	size;
		bool		prepared;
		bool		clean;

		freetdsconnection	*freetdsconn;
};


class SQLRSERVER_DLLSPEC freetdsconnection : public sqlrserverconnection {
	friend class freetdscursor;
	public:
		freetdsconnection(sqlrservercontroller *cont);
		~freetdsconnection();
	private:
		void	initDatabaseFeatures();
		void	handleConnectString();
		bool	logIn(const char **error, const char **warning);
		const char	*logInError(const char *error, uint16_t stage);
		CS_INT	ctlibVersion(const char *version);
		const char	*ctlibVersionString(CS_INT version);
		sqlrservercursor	*newCursor(uint16_t id);
		void	deleteCursor(sqlrservercursor *curs);
		void	logOut();
		const char	*getDbType();
		const char	*getDbVersion();
		const char	*getDbHostNameQuery();
		const char	*getCatalogListQuery(
						const char *catalog);
		const char	*getCatalogListQuerySybase(
						const char *catalog);
		const char	*getCatalogListQuerySqlServer(
						const char *catalog);
		const char	*getSchemaListQuery(
						const char *catalog,
						const char *schema);
		const char	*getSchemaListQuerySybase(
						const char *catalog,
						const char *schema);
		const char	*getSchemaListQuerySqlServer(
						const char *catalog,
						const char *schema);
		const char	*getTableTypeListQuery(
						const char *catalog,
						const char *schema,
						const char *tabletypes);
		const char	*getTableTypeListQuerySybase(
						const char *catalog,
						const char *schema,
						const char *tabletypes);
		const char	*getTableTypeListQuerySqlServer(
						const char *catalog,
						const char *schema,
						const char *tabletypes);
		const char	*getTableListQuery(const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes);
		const char	*getTableListQuerySybase(
						const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes);
		const char	*getTableListQuerySqlServer(
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
		const char	*getColumnListQuerySybase(
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column);
		const char	*getColumnListQuerySqlServer(
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column);
		const char	*getPrimaryKeysListQuery(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getPrimaryKeysListQuerySybase(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getPrimaryKeysListQuerySqlServer(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getKeyAndIndexListQuerySybase(
						const char *catalog,
						const char *schema,
						const char *table);
		const char	*getKeyAndIndexListQuerySqlServer(
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
		const char	*getProcedureListQuerySybase(
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*getProcedureListQuerySqlServer(
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*getProcedureParameterListQuerySybase(
						const char *catalog,
						const char *schema,
						const char *procedure);
		const char	*getProcedureParameterListQuerySqlServer(
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
		const char	*freetds;
		const char	*server;
		const char	*db;
		const char	*charset;
		const char	*language;
		const char	*hostname;
		const char	*packetsize;
		const char	*csversion;

		bool		dbused;

		char		*dbversion;

		bool		sybasedb;

		uint32_t	bytesperchar;

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

stringbuffer	freetdsconnection::errorstring;
int64_t		freetdsconnection::errorcode;
bool		freetdsconnection::liveconnection;

freetdsconnection::freetdsconnection(sqlrservercontroller *cont) :
						sqlrserverconnection(cont) {
	dbused=false;
	dbversion=NULL;
	sybasedb=true;
	bytesperchar=1;
	initDatabaseFeatures();
}

freetdsconnection::~freetdsconnection() {
	delete[] dbversion;
	delete[] maxconnections;
}

void freetdsconnection::initDatabaseFeatures() {

	maxconnections=
		charstring::parseNumber(cont->getConfig()->getMaxConnections());

	// FIXME: we need separate methods for sybase and mssql

	databasefeatures[FEATURE_AGGREGATE_FUNCTIONS]=
		"ALL,AVG,COUNT,DISTINCT,MAX,MIN,SUM";

	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		"true";

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
		"DATA_MANIPULATION,INDEX_DEFINITIONS,"
			"PRIVILEGE_DEFINITIONS,PROCEDURE_CALLS,"
			"TABLE_DEFINITIONS";

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
		"CREATE_TABLE";

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
		"FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

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
		"$#@";

	databasefeatures[FEATURE_FOREIGN_KEY_DELETE_RULES]=
		"";

	databasefeatures[FEATURE_FOREIGN_KEY_UPDATE_RULES]=
		"";

	databasefeatures[FEATURE_FORWARD_ONLY_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		"false";

	databasefeatures[FEATURE_GRANT_CLAUSES]=
		"WITH_GRANT_OPTION";

	databasefeatures[FEATURE_GROUP_BY_CLAUSES]=
		"BASIC,BEYOND_SELECT,UNRELATED";

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
		"INSERT_LITERALS,INSERT_SEARCHED,SELECT_INTO";

	databasefeatures[FEATURE_ISOLATION_LEVELS]=
		"READ_UNCOMMITTED,READ_COMMITTED,REPEATABLE_READ,SERIALIZABLE";

	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		"true";

	databasefeatures[FEATURE_LOCAL_FILE_USAGE]=
		"";

	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		"true";

	databasefeatures[FEATURE_LOCK_TYPES]=
		"";

	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		"131072";

	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
		"30";

	databasefeatures[FEATURE_MAX_CHAR_LITERAL_LENGTH]=
		"131072";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_GROUP_BY]=
		"16";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_INDEX]=
		"16";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_ORDER_BY]=
		"16";

	databasefeatures[FEATURE_MAX_COLUMNS_IN_SELECT]=
		"4096";

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
		"16";

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
		"abs,acos,asin,atan,atan2,ceiling,cos,cot,degrees,"
			"exp,floor,log,log10,mod,pi,power,radians,rand,round,"
			"sign,sin,sqrt,tan";

	databasefeatures[FEATURE_OPEN_CURSORS_ACROSS]=
		"COMMIT,ROLLBACK";

	databasefeatures[FEATURE_OPEN_STATEMENTS_ACROSS]=
		"COMMIT,ROLLBACK";

	databasefeatures[FEATURE_OTHERS_DELETES_ARE_VISIBLE]=
		"SCROLL_SENSITIVE";

	databasefeatures[FEATURE_OTHERS_INSERTS_ARE_VISIBLE]=
		"";

	databasefeatures[FEATURE_OTHERS_UPDATES_ARE_VISIBLE]=
		"SCROLL_SENSITIVE";

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
			"ISNOTNULL,ISNULL,LIKE,"
			"QUANTIFIED_COMPARISON";

	databasefeatures[FEATURE_PROCEDURE_TERM]=
		"stored procedure";

	databasefeatures[FEATURE_QUOTED_IDENTIFIER_CASE_STORAGE]=
		"MIXED";

	databasefeatures[FEATURE_RELATIONAL_JOIN_OPERATORS]=
		"CROSS_JOIN,FULL_OUTER_JOIN,INNER_JOIN,"
			"LEFT_OUTER_JOIN,RIGHT_OUTER_JOIN,UNION_JOIN";

	databasefeatures[FEATURE_RESULT_SET_CONCURRENCIES]=
		"FORWARD_ONLY/READ_ONLY,FORWARD_ONLY/UPDATABLE,"
				"SCROLL_INSENSITIVE/READ_ONLY,SCROLL_SENSITIVE/READ_ONLY,"
				"SCROLL_SENSITIVE/UPDATABLE";

	databasefeatures[FEATURE_RESULT_SET_HOLDABILITIES]=
		"";

	databasefeatures[FEATURE_RESULT_SET_TYPES]=
		"FORWARD_ONLY,SCROLL_INSENSITIVE,SCROLL_SENSITIVE";

	databasefeatures[FEATURE_REVOKE_CLAUSES]=
		"GRANT_OPTION_FOR";

	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		"ROWID_UNSUPPORTED";

	databasefeatures[FEATURE_ROW_VALUE_CONSTRUCTOR_EXPRESSIONS]=
		"VALUE_EXPRESSION,NULL,DEFAULT,ROW_SUBQUERY";

	databasefeatures[FEATURE_SCHEMA_TERM]=
		"owner";

	databasefeatures[FEATURE_SCHEMA_USAGE]=
		"DATA_MANIPULATION,INDEX_DEFINITIONS,"
			"PRIVILEGE_DEFINITIONS,PROCEDURE_CALLS,"
			"TABLE_DEFINITIONS";

	databasefeatures[FEATURE_SCROLL_CONCURRENCIES]=
		"";

	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		"\\";

	databasefeatures[FEATURE_SQL_GRAMMAR_LEVELS]=
		"MINIMUM,CORE";

	databasefeatures[FEATURE_SQL_KEYWORDS]=
		"ARITH_OVERFLOW,BREAK,BROWSE,BULK,CHAR_CONVERT,"
			"CHECKPOINT,CLUSTERED,COMPUTE,CONFIRM,CONTROLROW,"
			"DATA_PGS,DATABASE,DBCC,DISK,DUMMY,DUMP,ENDTRAN,"
			"ERRLVL,ERRORDATA,ERROREXIT,EXIT,FILLFACTOR,HOLDLOCK,"
			"IDENTITY_INSERT,IF,INDEX,KILL,LINENO,LOAD,"
			"MAX_ROWS_PER_PAGE,MIRROR,MIRROREXIT,NOHOLDLOCK,"
			"NONCLUSTERED,NUMERIC_TRUNCATION,OFF,OFFSETS,ONCE,"
			"ONLINE,OVER,PARTITION,PERM,PERMANENT,PLAN,PRINT,"
			"PROC,PROCESSEXIT,RAISERROR,READ,READTEXT,"
			"RECONFIGURE,REPLACE,RESERVED_PGS,RETURN,ROLE,ROWCNT,"
			"ROWCOUNT,RULE,SAVE,SETUSER,SHARED,SHUTDOWN,SOME,"
			"STATISTICS,STRIPE,SYB_IDENTITY,SYB_RESTREE,"
			"SYB_TERMINATE,TEMP,TEXTSIZE,TRAN,TRIGGER,TRUNCATE,"
			"TSEQUAL,UNPARTITION,USE,USED_PGS,USER_OPTION,"
			"WAITFOR,WHILE,WRITETEXT";

	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		"1";

	databasefeatures[FEATURE_STATIC_CURSOR_ATTRIBUTES]=
		"";

	databasefeatures[FEATURE_STORED_PROGRAMS]=
		"PROCEDURES";

	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		"ascii,char,concat,difference,insert,lcase,length,"
			"ltrim,repeat,right,rtrim,soundex,space,substring,"
			"ucase";

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
		"false";

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
		"false";

	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		"true";

	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		"database,ifnull,user,convert";

	databasefeatures[FEATURE_TABLE_CORRELATION_NAMES]=
		"BASIC";

	databasefeatures[FEATURE_TABLE_TERM]=
		"table";

	// sap ase supports these, but the freetds odbc driver reports 0;
	// matching native
	databasefeatures[FEATURE_TIME_DATE_ADD_INTERVALS]=
		"";

	// sap ase supports these, but the freetds odbc driver reports 0;
	// matching native
	databasefeatures[FEATURE_TIME_DATE_DIFF_INTERVALS]=
		"";

	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		"curdate,curtime,dayname,dayofmonth,dayofweek,"
			"dayofyear,hour,minute,month,monthname,now,quarter,"
			"timestampadd,timestampdiff,second,week,year";

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

}

void freetdsconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	sybase=cont->getConnectStringValue("sybase");
	freetds=cont->getConnectStringValue("freetds");
	server=cont->getConnectStringValue("server");
	db=cont->getConnectStringValue("db");
	charset=cont->getConnectStringValue("charset");
	language=cont->getConnectStringValue("language");
	hostname=cont->getConnectStringValue("hostname");
	packetsize=cont->getConnectStringValue("packetsize");
	csversion=cont->getConnectStringValue("csversion");

	// freetds doesn't currently support array fetches
	cont->setFetchAtOnce(1);

	if (cont->getMaxColumnCount()==1) {
		// if max column count is set to 1 then force it
		// to 2 so the db version detection doesn't crash
		cont->setMaxColumnCount(2);
	}
}

CS_INT freetdsconnection::ctlibVersion(const char *version) {
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


const char *freetdsconnection::ctlibVersionString(CS_INT version) {
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

bool freetdsconnection::logIn(const char **error, const char **warning) {

	// set sybase
	if (!charstring::isNullOrEmpty(sybase) &&
			!environment::setValue("SYBASE",sybase)) {
		*error=logInError(
			"Failed to set SYBASE environment variable.",1);
		return false;
	}

	// set freetds
	if (!charstring::isNullOrEmpty(freetds)) {
		if (!environment::setValue("FREETDS",freetds)) {
			*error=logInError(
				"Failed to set FREETDS "
				"environment variable.",1);
			return false;
		}
		if (!environment::setValue("FREETDSCONF",freetds)) {
			*error=logInError(
				"Failed to set FREETDSCONF "
				"environment variable.",1);
			return false;
		}
	}

	// set server
	if (!charstring::isNullOrEmpty(server) &&
			!environment::setValue("DSQUERY",server)) {
		*error=logInError(
			"Failed to set DSQUERY environment variable.",2);
		return false;
	}

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
		// and the client library's own diagnostic - the locale name
		// and the locales.dat it isn't in - goes to stderr before
		// CS_MESSAGE_CB exists, so it can't be captured.  Naming the
		// locale is all that can be done from here.
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
		(CS_VOID *)freetdsconnection::csMessageCallback,CS_UNUSED,
			(CS_INT *)NULL)
			!=CS_SUCCEED) {
		*error=logInError(
			"Failed to set a cslib error message callback",4);
		return false;
	}
	if (ct_callback(context,NULL,CS_SET,CS_CLIENTMSG_CB,
		(CS_VOID *)freetdsconnection::clientMessageCallback)
			!=CS_SUCCEED) {
		*error=logInError(
			"Failed to set a client error message callback",4);
		return false;
	}
	if (ct_callback(context,NULL,CS_SET,CS_SERVERMSG_CB,
		(CS_VOID *)freetdsconnection::serverMessageCallback)
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
		CS_NULLTERM,(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the user",5);
		return false;
	}


	// set the password to use
	const char	*password=cont->getLoginPassword();
	if (ct_con_props(dbconn,CS_SET,CS_PASSWORD,
		(CS_VOID *)((!charstring::isNullOrEmpty(password))?password:""),
		CS_NULLTERM,(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the password",5);
		return false;
	}

	// set application name
	if (ct_con_props(dbconn,CS_SET,CS_APPNAME,
		(CS_VOID *)"sqlrelay",
		CS_NULLTERM,(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the application name",5);
		return false;
	}

	// set hostname
	if (!charstring::isNullOrEmpty(hostname) &&
		ct_con_props(dbconn,CS_SET,CS_HOSTNAME,
			(CS_VOID *)hostname,
			CS_NULLTERM,(CS_INT *)NULL)!=CS_SUCCEED) {
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
			(CS_CHAR *)language,CS_NULLTERM,(CS_INT *)NULL)!=
				CS_SUCCEED) {
		*error=logInError("Failed to set the language",6);
		return false;
	}

	// set charset - this only asks the server which charset to use, it
	// isn't the charset FreeTDS itself decodes incoming data as (that's
	// "client charset" in freetds.conf, and freetds.conf is the only way
	// to set it)
	if (!charstring::isNullOrEmpty(charset) &&
		cs_locale(context,CS_SET,locale,CS_SYB_CHARSET,
			(CS_CHAR *)charset,CS_NULLTERM,(CS_INT *)NULL)!=
				CS_SUCCEED) {
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
	// charset calibration.  freetds multiplies the size it reports for
	// character columns by the client charset's maximum bytes per
	// character, and no ctlib call reports what that charset is.
	// CS_CLIENTCHARSET only reads back one that we set ourselves, and
	// it's usually set in freetds.conf instead.  A 1-character column
	// comes back already multiplied though, so its size is the factor.
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
			bytesperchar=(uint32_t)fmt.maxlength;
		}
	}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);
	ct_cmd_drop(cmd);

	return retval;
}

const char *freetdsconnection::logInError(const char *error, uint16_t stage) {

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

sqlrservercursor *freetdsconnection::newCursor(uint16_t id) {
	return (sqlrservercursor *)new freetdscursor(
					(sqlrserverconnection *)this,id);
}

void freetdsconnection::deleteCursor(sqlrservercursor *curs) {
	delete (freetdscursor *)curs;
}

void freetdsconnection::logOut() {

	cs_loc_drop(context,locale);
	ct_close(dbconn,CS_UNUSED);
	ct_con_drop(dbconn);
	ct_exit(context,CS_UNUSED);
	cs_ctx_drop(context);
}

const char *freetdsconnection::getDbType() {
	return "freetds";
}

const char *freetdsconnection::getDbVersion() {
	return dbversion;
}

const char *freetdsconnection::getDbHostNameQuery() {
	return "select asehostname()";
}

const char *freetdsconnection::getCatalogListQuery(const char *catalog) {
	return (sybasedb)?
		getCatalogListQuerySybase(catalog):
		getCatalogListQuerySqlServer(catalog);
}

const char *freetdsconnection::getCatalogListQuerySybase(
						const char *catalog) {

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

const char *freetdsconnection::getCatalogListQuerySqlServer(
						const char *catalog) {

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

const char *freetdsconnection::getSchemaListQuery(const char *catalog,
							const char *schema) {
	return (sybasedb)?
		getSchemaListQuerySybase(catalog,schema):
		getSchemaListQuerySqlServer(catalog,schema);
}

const char *freetdsconnection::getSchemaListQuerySybase(
						const char *catalog,
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

const char *freetdsconnection::getSchemaListQuerySqlServer(
						const char *catalog,
						const char *schema) {

	schemalistquery.clear();

	// select clause
	schemalistquery.append(
		"select distinct "
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

const char *freetdsconnection::getTableTypeListQuery(
						const char *catalog,
						const char *schema,
						const char *tabletypes) {
	return (sybasedb)?
		getTableTypeListQuerySybase(catalog,schema,tabletypes):
		getTableTypeListQuerySqlServer(catalog,schema,tabletypes);
}

const char *freetdsconnection::getTableTypeListQuerySybase(
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
		"(select 'SYSTEM TABLE' as table_type "
		"union "
		"select 'TABLE' as table_type "
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

const char *freetdsconnection::getTableTypeListQuerySqlServer(
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

const char *freetdsconnection::getTableListQuery(const char *catalog,
						const char *schema,
						const char *table,
						uint16_t objecttypes) {
	return (sybasedb)?
		getTableListQuerySybase(catalog,schema,table,objecttypes):
		getTableListQuerySqlServer(catalog,schema,table,objecttypes);
}

const char *freetdsconnection::getTableListQuerySybase(
						const char *catalog,
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
		"	and "
		"	(");
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

const char *freetdsconnection::getTableListQuerySqlServer(
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
		"where "
		"	(");
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
			"	8000 as column_size, "
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
			"	8000 as column_size, "
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

static const char	*ntexttype=
			"select "
			"	'NTEXT' as type_name, "
			"	-1 as data_type, "
			"	1073741823 as column_size, "
			"	'N''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	1 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'NTEXT' as local_type_name, "
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
			"	8000 as column_size, "
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

static const char	*doubleprecisiontype=
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

static const char	*varchartype=
			"select "
			"	'VARCHAR' as type_name, "
			"	12 as data_type, "
			"	8000 as column_size, "
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

static const char	*nchartype=
			"select "
			"	'NCHAR' as type_name, "
			"	-15 as data_type, "
			"	4000 as column_size, "
			"	'N''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'NCHAR' as local_type_name, "
			"	0 as minimum_scale, "
			"	0 as maximum_scale, "
			"	null as sql_data_type, "
			"	null as sql_datetime_sub, "
			"	10 as num_prec_radix, "
			"	null as interval_precision, "
			"	NULL "
			" ";

static const char	*nvarchartype=
			"select "
			"	'NVARCHAR' as type_name, "
			"	-9 as data_type, "
			"	4000 as column_size, "
			"	'N''''' as literal_prefix, "
			"	'''''' as literal_suffix, "
			"	null as create_params, "
			"	1 as nullable, "
			"	1 as case_sensitive, "
			"	3 as searchable, "
			"	0 as unsigned_attribute, "
			"	0 as fixed_prec_scale, "
			"	0 as auto_unique_value, "
			"	'NVARCHAR' as local_type_name, "
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

static const char	*uidtype=
			"select "
			"	'UNIQUEIDENTIFIER' as type_name, "
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
			"	'UNIQUEIDENTIFIER' as local_type_name, "
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

const char *freetdsconnection::getTypeInfoListQuery(
					const char *catalog,
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
			typeinfolistquery.append(ntexttype);
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
			typeinfolistquery.append(doubleprecisiontype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(varchartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(nchartype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(nvarchartype);
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
			typeinfolistquery.append(uidtype);
			typeinfolistquery.append(") union (");
			typeinfolistquery.append(xmltype);
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
	} else if (!charstring::compareIgnoringCase(type,"ntext")) {
		return ntexttype;
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
	} else if (!charstring::compareIgnoringCase(type,"nchar")) {
		return nchartype;
	} else if (!charstring::compareIgnoringCase(type,"nvarchar")) {
		return nvarchartype;
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
	} else if (!charstring::compareIgnoringCase(type,"uniqueidentifier")) {
		return uidtype;
	} else if (!charstring::compareIgnoringCase(type,"xml")) {
		return xmltype;
	}
	return NULL;
}

const char *freetdsconnection::getColumnListQuery(
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column) {
	return (sybasedb)?
		getColumnListQuerySybase(catalog,schema,table,column):
		getColumnListQuerySqlServer(catalog,schema,table,column);
}

const char *freetdsconnection::getColumnListQuerySybase(
						const char *catalog,
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

const char *freetdsconnection::getColumnListQuerySqlServer(
						const char *catalog,
						const char *schema,
						const char *table,
						const char *column) {

	columnlistquery.clear();

	bool	temptable=(table && table[0]=='#');

	// select clause
	columnlistquery.append(
		"select "
		"	co.table_catalog as table_cat, "
		"	co.table_schema as table_schem, "
		"	co.table_name as table_name, "
		"	co.column_name, "
		"	null as data_type, "
		"	co.data_type as type_name, "
		"	co.character_maximum_length "
					"as column_size, "
		"	co.character_octet_length "
					"as buffer_length, "
		"	co.numeric_scale as decimal_digits, "
		"	co.numeric_precision_radix "
					"as num_prec_radix, "
		"	case co.is_nullable "
		"		when 'YES' then 1 "
		"		else 0 "
		"	end as nullable, "
		"	case "
		"		when COLUMNPROPERTY( "
		"			OBJECT_ID(");
	if (temptable) {
		columnlistquery.append(
			"			'tempdb..'+co.table_name), ");
	} else {
		columnlistquery.append(
			"			co.table_name), ");
	}
	columnlistquery.append(
		"			co.column_name, "
		"			'IsIdentity')=1 "
		"			then 'auto_increment' "
		"		else null "
		"	end as remarks, "
		"	co.column_default, "
		"	null as sql_data_type, "
		"	null as sql_datetime_sub, "
		"	co.character_octet_length "
				"as char_octet_length, "
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
		"		when COLUMNPROPERTY( "
		"			OBJECT_ID(");
	if (temptable) {
		columnlistquery.append(
			"			'tempdb..'+co.table_name), ");
	} else {
		columnlistquery.append(
			"			co.table_name), ");
	}
	columnlistquery.append(
		"			co.column_name, "
		"			'IsIdentity')=1 "
		"			then 'YES' "
		"		else 'NO' "
		"	end as is_autoincrement, "
		"	null ");

	// from clause
	columnlistquery.append("from ");
	if (temptable) {
		columnlistquery.append(
			"	tempdb.information_schema.columns co ");
	} else {
		columnlistquery.append(
			"	information_schema.columns co ");
	}
	columnlistquery.append(
		"left outer join ( "
		"	select "
		"		ku.table_catalog, "
		"		ku.table_schema, "
		"		ku.table_name, "
		"		ku.column_name, "
		"		min(case tc.constraint_type "
		"			when 'PRIMARY KEY' then 1 "
		"			when 'UNIQUE' then 2 "
		"			when 'FOREIGN KEY' then 3 "
		"		end) as key_priority "
		"	from ");
	if (temptable) {
		columnlistquery.append(
			"		tempdb.information_schema.table_constraints tc, "
			"		tempdb.information_schema.key_column_usage ku ");
	} else {
		columnlistquery.append(
			"		information_schema.table_constraints tc, "
			"		information_schema.key_column_usage ku ");
	}
	columnlistquery.append(
		"	where "
		"		tc.constraint_name=ku.constraint_name "
		"		and "
		"		tc.table_schema=ku.table_schema "
		"		and "
		"		tc.constraint_type in "
		"			('PRIMARY KEY','UNIQUE','FOREIGN KEY') "
		"	group by "
		"		ku.table_catalog, "
		"		ku.table_schema, "
		"		ku.table_name, "
		"		ku.column_name "
		") ck "
		"on "
		"	co.table_catalog=ck.table_catalog "
		"	and "
		"	co.table_schema=ck.table_schema "
		"	and "
		"	co.table_name=ck.table_name "
		"	and "
		"	co.column_name=ck.column_name ");

	// where clause
	bool	first=true;
	if (temptable) {
		columnlistquery.append(
			"where "
			"	co.table_name like '");
		columnlistquery.append(table);
		columnlistquery.append("____%%' ");
		first=false;
	} else {
		if (!charstring::isNullOrEmpty(catalog)) {
			columnlistquery.append(
				"where "
				"	co.table_catalog like '");
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
				"	co.table_schema like '");
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
				"	co.table_name like '");
			columnlistquery.append(table);
			columnlistquery.append("' ");
		}
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

	// order by clause
	columnlistquery.append(
		"order by "
		"	co.ordinal_position");

	return columnlistquery.getString();
}

const char *freetdsconnection::getPrimaryKeysListQuery(const char *catalog,
							const char *schema,
							const char *table) {
	return (sybasedb)?
		getPrimaryKeysListQuerySybase(catalog,schema,table):
		getPrimaryKeysListQuerySqlServer(catalog,schema,table);
}

const char *freetdsconnection::getPrimaryKeysListQuerySybase(
							const char *catalog,
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

const char *freetdsconnection::getPrimaryKeysListQuerySqlServer(
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

const char *freetdsconnection::getKeyAndIndexListQuery(
						const char *catalog,
						const char *schema,
						const char *table) {
	return (sybasedb)?
		getKeyAndIndexListQuerySybase(catalog,schema,table):
		getKeyAndIndexListQuerySqlServer(catalog,schema,table);
}

const char *freetdsconnection::getKeyAndIndexListQuerySybase(
						const char *catalog,
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
		"		when i.status & 2 = 2 then 0 "
		"		else 1 "
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

const char *freetdsconnection::getKeyAndIndexListQuerySqlServer(
						const char *catalog,
						const char *schema,
						const char *table) {

	keyandindexlistquery.clear();

	// select clause
	keyandindexlistquery.append(
		"select "
		"	db_name() as table_cat, "
		"	s.name as table_schem, "
		"	t.name as table_name, "
		"	case "
		"		when i.is_unique=1 then 0 "
		"		else 1 "
		"	end as non_unique, "
		"	s.name as index_qualifier, "
		"	i.name as index_name, "
		"	case i.type "
		"		when 1 then 1 "
		"		when 2 then 3 "
		"		else 0 "
		"	end as type, "
		"	ic.key_ordinal as ordinal_position, "
		"	c.name as column_name, "
		"	case ic.is_descending_key "
		"		when 1 then 'D' "
		"		else 'A' "
		"	end as asc_or_desc, "
		"	null as cardinality, "
		"	null as pages, "
		"	case "
		"		when i.has_filter=1 "
		"			then i.filter_definition "
		"		else null "
		"	end as filter_condition, "
		"	null ");

	// from clause
	keyandindexlistquery.append(
		"from "
		"	sys.indexes i, "
		"	sys.index_columns ic, "
		"	sys.columns c, "
		"	sys.tables t, "
		"	sys.schemas s ");

	// where clause
	keyandindexlistquery.append(
		"where "
		"	i.object_id=ic.object_id "
		"	and "
		"	i.index_id=ic.index_id "
		"	and "
		"	ic.object_id=c.object_id "
		"	and "
		"	ic.column_id=c.column_id "
		"	and "
		"	i.object_id=t.object_id "
		"	and "
		"	t.schema_id=s.schema_id "
		"	and "
		"	i.type>0 ");
	if (!charstring::isNullOrEmpty(catalog)) {
		keyandindexlistquery.append(
			"	and "
			"	db_name() like '");
		keyandindexlistquery.append(catalog);
		keyandindexlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(schema)) {
		keyandindexlistquery.append(
			"	and "
			"	s.name like '");
		keyandindexlistquery.append(schema);
		keyandindexlistquery.append("' ");
	}
	if (!charstring::isNullOrEmpty(table)) {
		keyandindexlistquery.append(
			"	and "
			"	t.name like '");
		keyandindexlistquery.append(table);
		keyandindexlistquery.append("' ");
	}

	// order by clause
	keyandindexlistquery.append(
		"order by "
		"	t.name, "
		"	i.name, "
		"	ic.key_ordinal");

	return keyandindexlistquery.getString();
}

const char *freetdsconnection::getProcedureListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {
	return (sybasedb)?
		getProcedureListQuerySybase(catalog,schema,procedure):
		getProcedureListQuerySqlServer(catalog,schema,procedure);
}

const char *freetdsconnection::getProcedureListQuerySybase(
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

const char *freetdsconnection::getProcedureListQuerySqlServer(
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

const char *freetdsconnection::getProcedureParameterListQuery(
						const char *catalog,
						const char *schema,
						const char *procedure) {
	return (sybasedb)?
		getProcedureParameterListQuerySybase(
					catalog,schema,procedure):
		getProcedureParameterListQuerySqlServer(
					catalog,schema,procedure);
}

const char *freetdsconnection::getProcedureParameterListQuerySybase(
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

const char *freetdsconnection::getProcedureParameterListQuerySqlServer(
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
		"	p.data_type as type_name, "
		"	p.character_maximum_length as column_size, "
		"	null as buffer_length, "
		"	p.numeric_scale as decimal_digits, "
		"	p.numeric_precision_radix as num_prec_radix, "
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

const char *freetdsconnection::selectCatalogQuery() {
	return "use %s";
}

const char *freetdsconnection::getCurrentCatalogQuery() {
	return "select db_name()";
}

const char *freetdsconnection::getCurrentSchemaQuery() {
	return "select user_name()";
}

const char *freetdsconnection::getCurrentUserQuery() {
	// suser_sname() isn't available on all ASE versions, but
	// suser_name() is
	return "select suser_name()";
}

const char *freetdsconnection::getLastInsertIdQuery() {
	return "select @@identity";
}

const char *freetdsconnection::getIsolationLevelQuery() {
	if (sybasedb) {
		return "select @@isolation";
	} else {
		return "select "
			"	case transaction_isolation_level "
			"		when 0 then 'UNSPECIFIED' "
			"		when 1 then 'READ UNCOMMITTED' "
			"		when 2 then 'READ COMMITTED' "
			"		when 3 then 'REPEATABLE READ' "
			"		when 4 then 'SERIALIZABLE' "
			"		when 5 then 'SNAPSHOT' "
			"	end "
			"from "
			"	sys.dm_exec_sessions "
			"where "
			"	session_id=@@spid";
	}
}

const char *freetdsconnection::mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat) {
	if (fromformat==toformat) {
		return isolevel;
	}
	if (sybasedb) {
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
	} else {
		if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
			if (!charstring::compare(isolevel,
				"TRANSACTION_READ_UNCOMMITTED")) {
				return "READ UNCOMMITTED";
			}
			if (!charstring::compare(isolevel,
				"TRANSACTION_READ_COMMITTED")) {
				return "READ COMMITTED";
			}
			if (!charstring::compare(isolevel,
				"TRANSACTION_REPEATABLE_READ")) {
				return "REPEATABLE READ";
			}
			if (!charstring::compare(isolevel,
				"TRANSACTION_SERIALIZABLE")) {
				return "SERIALIZABLE";
			}
			if (!charstring::compare(isolevel,
				"TRANSACTION_SNAPSHOT")) {
				return "SNAPSHOT";
			}
		} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
				toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
			if (!charstring::compareIgnoringCase(isolevel,
						"READ UNCOMMITTED")) {
				return "TRANSACTION_READ_UNCOMMITTED";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"READ COMMITTED")) {
				return "TRANSACTION_READ_COMMITTED";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"REPEATABLE READ")) {
				return "TRANSACTION_REPEATABLE_READ";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"SERIALIZABLE")) {
				return "TRANSACTION_SERIALIZABLE";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"SNAPSHOT")) {
				return "TRANSACTION_SNAPSHOT";
			}
		} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
			if (!charstring::compare(isolevel,
					"SQL_TXN_READ_UNCOMMITTED")) {
				return "READ UNCOMMITTED";
			}
			if (!charstring::compare(isolevel,
					"SQL_TXN_READ_COMMITTED")) {
				return "READ COMMITTED";
			}
			if (!charstring::compare(isolevel,
					"SQL_TXN_REPEATABLE_READ")) {
				return "REPEATABLE READ";
			}
			if (!charstring::compare(isolevel,
					"SQL_TXN_SERIALIZABLE")) {
				return "SERIALIZABLE";
			}
		} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
				toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
			if (!charstring::compareIgnoringCase(isolevel,
						"READ UNCOMMITTED")) {
				return "SQL_TXN_READ_UNCOMMITTED";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"READ COMMITTED")) {
				return "SQL_TXN_READ_COMMITTED";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"REPEATABLE READ")) {
				return "SQL_TXN_REPEATABLE_READ";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"SERIALIZABLE")) {
				return "SQL_TXN_SERIALIZABLE";
			}
			if (!charstring::compareIgnoringCase(isolevel,
						"SNAPSHOT")) {
				return "SQL_TXN_SERIALIZABLE";
			}
		}
	}
	return isolevel;
}

const char * const *freetdsconnection::getDatabaseFeatures() {
	cont->capDatabaseFeatures(databasefeatures);
	return databasefeatures;
}

const char *freetdsconnection::getNoopQuery() {
	return "waitfor delay '0:0'";
}

const char *freetdsconnection::getBindFormat() {
	return "@*";
}

const char *freetdsconnection::beginTransactionQuery() {
	return "BEGIN TRANSACTION";
}

freetdscursor::freetdscursor(sqlrserverconnection *conn, uint16_t id) :
						sqlrservercursor(conn,id) {

	#if defined(VERSION_NO)
	char	*versionstring=charstring::duplicate(VERSION_NO);
	#elif defined(TDS_VERSION_NO)
	char	*versionstring=charstring::duplicate(TDS_VERSION_NO);
	#else
	char	*versionstring=new char[1024];
	CS_INT	outlen;
	if (ct_config(NULL,CS_GET,CS_VER_STRING,
			(void *)versionstring,1023,&outlen)==CS_SUCCEED) {
		versionstring[outlen]='\0';
	} else {
		versionstring=charstring::copy(versionstring,"freetds v0.00.0");
	}
	#endif

	char	*v=charstring::findFirst(versionstring,'v');
	if (v) {
		*v='\0';
		majorversion=charstring::convertToInteger(v+1);
		char	*firstdot=charstring::findFirst(v+1,'.');
		if (firstdot) {
			*firstdot='\0';
			minorversion=charstring::convertToInteger(firstdot+1);
			char	*seconddot=
				charstring::findFirst(firstdot+1,'.');
			if (seconddot) {
				*seconddot='\0';
				patchlevel=
				charstring::convertToInteger(seconddot+1);
			} else {
				patchlevel=0;
			}
		} else {
			minorversion=0;
			patchlevel=0;
		}
	} else {
		majorversion=0;
		minorversion=0;
		patchlevel=0;
	}
	delete[] versionstring;

	// Affected row count is generally supported in versions >= 0.53 but
	// appears to be broken in 0.61.
	knowsaffectedrows=(majorversion>0 ||
				(minorversion>=53 && minorversion!=61));

	prepared=false;
	freetdsconn=(freetdsconnection *)conn;
	cmd=NULL;
	languagecmd=NULL;
	cursorcmd=NULL;

	cursornamesize=charstring::getIntegerLength(id);
	cursorname=charstring::parseNumber(id);

	rpc=false;

	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	parameter=new CS_DATAFMT[maxbindcount];
	inbindvalue=new CS_VOID *[maxbindcount];
	inbinddatasize=new CS_INT[maxbindcount];
	inbindindicator=new CS_SMALLINT[maxbindcount];
	inbindts=new char *[maxbindcount];
	outbindtype=new CS_INT[maxbindcount];
	outbindstrings=new char *[maxbindcount];
	outbindstringsizes=new uint32_t[maxbindcount];
	outbindints=new int64_t *[maxbindcount];
	outbinddoubles=new double *[maxbindcount];
	outbinddates=new datebind[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		inbindts[i]=new char[27];
	}

	// replace the regular expressions used to detect creation of a
	// temporary table
	setCreateTempTablePattern("^(create|CREATE)[ 	\r\n]"
					"+(table|TABLE)[ 	\r\n]+#");

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

freetdscursor::~freetdscursor() {
	close();
	delete[] cursorname;
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
	delete[] outbinddates;

	deallocateResultSetBuffers();
}

void freetdscursor::allocateResultSetBuffers(int32_t columncount) {

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

void freetdscursor::deallocateResultSetBuffers() {
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

bool freetdscursor::open() {

	clean=true;

	if (ct_cmd_alloc(freetdsconn->dbconn,&languagecmd)!=CS_SUCCEED) {
		return false;
	}
	if (ct_cmd_alloc(freetdsconn->dbconn,&cursorcmd)!=CS_SUCCEED) {
		return false;
	}
	cmd=NULL;

	// switch to the correct database
	// (only do this once per connection)
	bool	retval=true;
	if (!charstring::isNullOrEmpty(freetdsconn->db) &&
					!freetdsconn->dbused) {
		uint32_t	len=charstring::getLength(freetdsconn->db)+4;
		char		*query=new char[len+1];
		charstring::printf(query,len+1,"use %s",freetdsconn->db);
		if (!(prepareQuery(query,len) && executeQuery(query,len))) {
			char		err[2048];
			uint32_t	errlen;
			int64_t		errn;
			bool		live;
			getError(err,sizeof(err),&errlen,&errn,&live);
			stderror.printf("%s\n",err);
			retval=false;
		} else {
			freetdsconn->dbused=true;
		}
		closeResultSet();
		delete[] query;
	}

	if (!freetdsconn->dbversion) {

		// try the various queries that might return the version
		const char	*query[]={
			"select @@version",
			"sp_version installmaster",
			NULL
		};
		CS_INT		index[]={
			0,1,0
		};

		for (uint32_t i=0; query[i] && !freetdsconn->dbversion; i++) {

			const char	*q=query[i];
			int32_t		len=charstring::getLength(q);
			bool		error=false;

			if (prepareQuery(q,len) &&
					executeQuery(q,len) &&
					fetchRow(&error)) {
				freetdsconn->dbversion=
					charstring::duplicate(data[index[i]]);
			}

			closeResultSet();
		}

		// fall back to unknown
		if (!freetdsconn->dbversion) {
			freetdsconn->dbversion=
				charstring::duplicate("unknown");
		}

		// set sybasedb too, it's sybase if it's not microsoft
		freetdsconn->sybasedb=!charstring::contains(
					freetdsconn->dbversion,"Microsoft");
	}
	return retval;
}

bool freetdscursor::close() {

	bool	retval=true;
	if (languagecmd) {
		retval=(ct_cmd_drop(languagecmd)==CS_SUCCEED);
		languagecmd=NULL;
	}
	if (cursorcmd) {
		retval=(retval && (ct_cmd_drop(cursorcmd)==CS_SUCCEED));
		cursorcmd=NULL;
	}
	cmd=NULL;
	return retval;
}

// walk an rpc procedure name.  The name may be a bare token, or delimited
// with double-quotes (the ASE spelling, under quoted_identifier) or square
// brackets (the T-SQL/MS SQL Server spelling); either delimiter may contain
// whitespace.  A bare name ends at "(", "}", ";" or whitespace.  Returns
// the position just past the name (or its closing delimiter); *namestart
// and *namelen give the name itself, with any delimiters stripped.
static const char *getRpcName(const char *p, const char **namestart,
							CS_INT *namelen) {
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

bool freetdscursor::prepareQuery(const char *query, uint32_t size) {

	// if the client aborts while a query is in the middle of running,
	// commit or rollback will be called, potentially before closeResultSet
	// is called and, since we're really only using 1 cursor, it will fail
	// unless closeResultSet gets called, so just to make sure, we'll call
	// it here
	closeResultSet();

	// initialize column count
	ncols=0;

	clean=true;

	this->query=(char *)query;
	this->size=size;

	paramindex=0;
	outbindindex=0;
	rpc=false;

	if ((!charstring::compare(query,"select",6) ||
		!charstring::compare(query,"SELECT",6)) &&
		character::isWhitespace(query[6])) {

		// initiate a cursor command
		cmd=cursorcmd;
		#ifdef FREETDS_SUPPORTS_CURSORS
		if (ct_cursor(cursorcmd,
				CS_CURSOR_DECLARE,
				(CS_CHAR *)cursorname,
				(CS_INT)cursornamesize,
				(CS_CHAR *)query,
				size,
				//CS_READ_ONLY)!=CS_SUCCEED) {
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}
		#endif

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
		rpc=true;

		// literal parameters written directly into the query
		// (eg. "exec someproc 1,2") aren't bound by the client -
		// ct_param() them here or the rpc would silently run
		// without them
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
		rpc=true;

		// literal parameters written directly into the query
		// (eg. "execute someproc 1,2") aren't bound by the client -
		// ct_param() them here or the rpc would silently run
		// without them
		if (!parseRpcParams(p)) {
			return false;
		}

	} else if (query[0]=='{') {

		// handle ODBC/JDBC procedure-call syntax:
		// {call proc(...)} or {?=call proc(...)}

		// initiate an rpc command
		cmd=languagecmd;

		// find "call"
		const char	*p=query+1;
		while (*p && *p!='}' &&
				charstring::compare(p,"call",4) &&
				charstring::compare(p,"CALL",4)) {
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

		if (ct_command(languagecmd,
				CS_RPC_CMD,
				(CS_CHAR *)namestart,
				namelen,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}
		rpc=true;

	} else {

		// initiate a language command
		cmd=languagecmd;
		#ifdef FREETDS_SUPPORTS_CURSORS
		if (ct_command(languagecmd,
				CS_LANG_CMD,
				(CS_CHAR *)query,
				size,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}
		#endif
	}

	clean=false;
	prepared=true;
	return true;
}

bool freetdscursor::supportsNativeBinds(const char *query, uint32_t size) {

	// only rpc commands (exec/execute/{call ...}) get real ct_param()
	// binds - see the comment by FREETDS_SUPPORTS_CURSORS above.
	// Ordinary language commands and cursor-based selects still fall
	// back to faked (inlined literal) binds
	if (((!charstring::compare(query,"exec",4) ||
			!charstring::compare(query,"EXEC",4)) &&
					character::isWhitespace(query[4])) ||
		((!charstring::compare(query,"execute",7) ||
			!charstring::compare(query,"EXECUTE",7)) &&
					character::isWhitespace(query[7])) ||
		query[0]=='{') {
		return true;
	}
	return false;
}

void freetdscursor::encodeBlob(stringbuffer *buffer,
					const char *data, uint32_t datasize) {

	// sybase/mssqlserver wants each byte of blob data to be converted to
	// two hex characters and the whole thing to start with 0x
	// eg: hello - > 0x68656C6C6F
	// just "0x" is illegal though, so for empty data, use 0x00, which
	// technically is a single \0, not truly empty data, but that's the
	// best that we can do

	buffer->append("0x");
	if (!datasize) {
		buffer->append("00");
	} else {
		for (uint32_t i=0; i<datasize; i++) {
			buffer->append(conn->cont->asciiToHex(data[i]));
		}
	}
}

void freetdscursor::decodeBlob(char **data, uint32_t *datasize) {

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

void freetdscursor::checkRePrepare() {

	// Sybase doesn't allow you to rebind and re-execute when using 
	// ct_command.  You have to re-prepare too.  I'll make this transparent
	// to the user.
	if (!prepared) {
		prepareQuery(query,size);
	}
}

// ct_param() a comma-separated list of literal rpc parameters (eg. the
// "1,2" in "exec someproc 1,2"); quoted literals have their quotes
// stripped.  p points just past the procedure name; stops at end of
// string or a statement-terminating ";".
bool freetdscursor::parseRpcParams(const char *p) {

	p=conn->cont->skipWhitespace(p);

	// bind-variable references (eg. "exec testproc @in1,@in2 output")
	// are supplied by the client's own inputBind() calls, matched by
	// name - this text is just documentation and must be left alone.
	// Only parse it here when it's nothing but literal values, since
	// there's no way to tell, from here, that the client is about to
	// bind by name.
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

		// an rpc parameter has to match the target parameter's type
		// on the wire - the backend won't implicitly convert a char
		// value to int/float the way it would a literal in ordinary
		// SQL text, so figure out a type for unquoted numeric
		// literals
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

bool freetdscursor::inputBind(CS_VOID *value, CS_INT valuesize,
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

		inbindvalue[paramindex]=value;
		inbinddatasize[paramindex]=valuesize;
		inbindindicator[paramindex]=indicator;
		return ct_param(cmd,&parameter[paramindex],
					NULL,CS_UNUSED,0)==CS_SUCCEED;
	}

	// for non-cursor commands, we can supply the parameter values now
	return ct_param(cmd,&parameter[paramindex],
				value,valuesize,indicator)==CS_SUCCEED;
}

bool freetdscursor::inputBind(const char *variable,
				uint16_t variablesize,
				const char *value,
				uint32_t valuesize,
				int16_t *isnull) {
	checkRePrepare();

	(CS_VOID)bytestring::zero(&parameter[paramindex],
				sizeof(parameter[paramindex]));
	if (cmd!=cursorcmd &&
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
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

bool freetdscursor::inputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value) {
	checkRePrepare();

	(CS_VOID)bytestring::zero(&parameter[paramindex],
				sizeof(parameter[paramindex]));
	if (cmd!=cursorcmd &&
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
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

bool freetdscursor::inputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t precision,
				uint32_t scale) {
	checkRePrepare();

	(CS_VOID)bytestring::zero(&parameter[paramindex],
				sizeof(parameter[paramindex]));
	if (cmd!=cursorcmd &&
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
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

bool freetdscursor::inputBind(const char *variable,
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

bool freetdscursor::outputBind(const char *variable, 
				uint16_t variablesize,
				char *value, 
				uint32_t valuesize, 
				int16_t *isnull) {
	checkRePrepare();

	outbindtype[outbindindex]=CS_CHAR_TYPE;
	outbindstrings[outbindindex]=value;
	outbindstringsizes[outbindindex]=valuesize;
	outbindindex++;

	(CS_VOID)bytestring::zero(&parameter[paramindex],
				sizeof(parameter[paramindex]));
	if (cmd!=cursorcmd &&
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
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

bool freetdscursor::outputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value,
				int16_t *isnull) {
	checkRePrepare();

	outbindtype[outbindindex]=CS_INT_TYPE;
	outbindints[outbindindex]=value;
	outbindindex++;

	(CS_VOID)bytestring::zero(&parameter[paramindex],
				sizeof(parameter[paramindex]));
	if (cmd!=cursorcmd &&
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
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

bool freetdscursor::outputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t *precision,
				uint32_t *scale,
				int16_t *isnull) {
	checkRePrepare();

	outbindtype[outbindindex]=CS_FLOAT_TYPE;
	outbinddoubles[outbindindex]=value;
	outbindindex++;

	(CS_VOID)bytestring::zero(&parameter[paramindex],
				sizeof(parameter[paramindex]));
	if (cmd!=cursorcmd &&
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
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

bool freetdscursor::outputBind(const char *variable,
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
	outbindindex++;

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));
	if (cmd!=cursorcmd &&
		charstring::isInteger(variable+1,variablesize-1)) {
		parameter[paramindex].name[0]='\0';
		parameter[paramindex].namelen=0;
	} else {
		charstring::copy(parameter[paramindex].name,variable);
		parameter[paramindex].namelen=variablesize;
	}
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

bool freetdscursor::executeQuery(const char *query, uint32_t size) {

	// initialize results (We use CS_UNSUPPORTED so that if the query
	// fails to execute, discardResults won't attempt to cancel any
	// non-existent result sets.  Doing that crashes FreeTDS.)
	results=CS_UNSUPPORTED;

	// clear out any errors
	freetdsconn->errorcode=0;
	freetdsconn->liveconnection=true;

	// initialize row counts
	affectedrows=0;
	row=0;
	maxrow=0;
	totalrows=0;

	// an rpc command was already issued by prepareQuery(); anything else
	// (a cursor-based select or a plain language command) still needs to
	// go out here as a plain language command, since cursor support and
	// native binds on non-rpc commands aren't enabled - see the comment
	// by FREETDS_SUPPORTS_CURSORS above
	if (!rpc) {
		if (ct_command(cmd,CS_LANG_CMD,
				(CS_CHAR *)query,size,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}
	}

	#ifdef FREETDS_SUPPORTS_CURSORS
	if (cmd==cursorcmd) {
		if (ct_cursor(cursorcmd,CS_CURSOR_ROWS,
					NULL,CS_UNUSED,
					NULL,CS_UNUSED,
					(CS_INT)getFetchAtOnce())!=
					CS_SUCCEED) {
			return false;
		}
		if (ct_cursor(cursorcmd,CS_CURSOR_OPEN,
					NULL,CS_UNUSED,
					NULL,CS_UNUSED,
					CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}

		// supply values for placeholders defined by inputBind()
		for (uint16_t i=0; i<paramindex; i++) {
			if (ct_param(cursorcmd,&parameter[i],
					inbindvalue[i],inbinddatasize[i],
					inbindindicator[i])!=CS_SUCCEED) {
				return false;
			}
		}
	}
	#endif

	if (ct_send(cmd)!=CS_SUCCEED) {
		closeResultSet();
		return false;
	}

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
		// send a single CS_ROW_RESULT.
		//
		// But...
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
		// Currently SQL Relay only supports 1 result set per query, so
		// for a given query, we only really care about one result set,
		// the CS_PARAM_RESULT, CS_ROW_RESULT, CS_CURSOR_RESULT, or
		// CS_COMPUTE_RESULT.  We'll grab whichever of those we get
		// first, and ignore the rest.

		if (resultstype==CS_CMD_SUCCEED) {
			// If we got CS_CMD_SUCCEED, then try to get the
			// affected row count.  The query might have been
			// DML/DDL, or this could be one of the result sets of
			// a stored procedure or direct TransactSQL.  We need
			// to do this here because we're going to cancel this
			// result set below.
			affectedrows=0;
			if (knowsaffectedrows) {
				if (ct_res_info(cmd,CS_ROW_COUNT,
					(CS_VOID *)&affectedrows,
					CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
					return false;
				}
			}
		}  else if (resultstype==CS_PARAM_RESULT ||
				resultstype==CS_ROW_RESULT ||
				resultstype==CS_CURSOR_RESULT ||
				resultstype==CS_COMPUTE_RESULT) {
			break;
		}

		// the result set was a type that we want to ignore
		if (ct_cancel(NULL,cmd,CS_CANCEL_CURRENT)==CS_FAIL) {
			freetdsconn->liveconnection=false;
			// FIXME: call ct_close(CS_FORCE_CLOSE)?
			return false;
		}
	}

	checkForTempTable(query,size);

	// reset the prepared flag
	prepared=false;

	// For queries which return rows or parameters (output bind variables),
	// get the column count and bind columns.
	bool	moneycolumn=false;
	if (resultstype==CS_ROW_RESULT ||
			resultstype==CS_CURSOR_RESULT ||
			resultstype==CS_COMPUTE_RESULT ||
			resultstype==CS_PARAM_RESULT) {

		// get the column count
		if (ct_res_info(cmd,CS_NUMDATA,(CS_VOID *)&ncols,
				CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
			return false;
		}

		// allocate buffers and limit column count if necessary
		uint32_t	maxcolumncount=conn->cont->getMaxColumnCount();
		if (!maxcolumncount) {
			// see note in discardResults for
			// why we're doing this here
			deallocateResultSetBuffers();
			allocateResultSetBuffers(ncols);
		} else if ((uint32_t)ncols>maxcolumncount) {
			ncols=maxcolumncount;
		}

		// bind columns
		for (CS_INT i=0; i<ncols; i++) {

			// dealing with money columns cause freetds < 0.53 to
			// crash, take care of that here...
			if (majorversion==0 && minorversion<53
							&& !moneycolumn) {
				CS_DATAFMT	moneytest;
				ct_describe(cmd,i+1,&moneytest);
				if (moneytest.datatype==CS_MONEY_TYPE ||
					moneytest.datatype==CS_MONEY4_TYPE) {
					moneycolumn=true;
					freetdsconn->errorstring.clear();
					freetdsconn->errorstring.append(
						"FreeTDS versions prior to "
						"0.53 do not support MONEY "
						"or SMALLMONEY datatypes. "
						"Please upgrade SQL Relay to "
						"a version compiled against "
						"FreeTDS >= 0.53 ");
				}
			}

			// reset the column-bind
			column[i]=templatecolumn;

			// actually...
			// if we're getting the output bind variables of a
			// stored procedure that returns dates, then use the
			// datetime type instead...
			if (resultstype==CS_PARAM_RESULT &&
				outbindtype[i]==CS_DATETIME_TYPE) {
				column[i].datatype=CS_DATETIME_TYPE;
				column[i].format=CS_FMT_UNUSED;
				column[i].maxlength=sizeof(CS_DATETIME);
			}
	
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
				if (ct_describe(cmd,i+1,
						&column[i])!=CS_SUCCEED) {
					break;
				}
				deflateColumnSize(i);
			}
		}

	}

	// If we got a moneycolumn (and version<0.53) then cancel the
	// result set.  Otherwise FreeTDS will spew "unknown marker"
	// errors to the screen when closeResultSet() is called.
	if (moneycolumn) {
		if (ct_cancel(NULL,cmd,CS_CANCEL_CURRENT)==CS_FAIL) {
			freetdsconn->liveconnection=false;
			// FIXME: call ct_close(CS_FORCE_CLOSE)?
			return false;
		}
		return false;
	}


	// if we're doing an rpc query, the result set should be a single
	// row of output parameter results, fetch it and populate the output
	// bind variables...
	if (resultstype==CS_PARAM_RESULT) {

		if (ct_fetch(cmd,
				CS_UNUSED,
				CS_UNUSED,
				CS_UNUSED,
				&rowsread)!=CS_SUCCEED || !rowsread) {
			return false;
		}
		
		// copy data into output bind values
		CS_INT	maxindex=outbindindex;
		if (ncols<outbindindex) {
			// this shouldn't happen...
			maxindex=ncols;
		}
		for (CS_INT i=0; i<maxindex; i++) {
			if (outbindtype[i]==CS_CHAR_TYPE) {
				CS_INT	size=outbindstringsizes[i];
				if (datasize[i][0]<size) {
					size=datasize[i][0];
				}
				bytestring::copy(outbindstrings[i],
							data[i],size);
			} else if (outbindtype[i]==CS_INT_TYPE) {
				*outbindints[i]=
					charstring::convertToInteger(data[i]);
			} else if (outbindtype[i]==CS_FLOAT_TYPE) {
				*outbinddoubles[i]=
					charstring::convertToFloatC(data[i]);
			} else if (outbindtype[i]==CS_DATETIME_TYPE) {

				// convert to a CS_DATEREC
				CS_DATEREC	dr;
				cs_dt_crack(freetdsconn->context,
						CS_DATETIME_TYPE,
						(CS_VOID *)data[i],&dr);

				datebind	*db=&outbinddates[i];
				*(db->year)=dr.dateyear;
				*(db->month)=dr.datemonth+1;
				*(db->day)=dr.datedmonth;
				*(db->hour)=dr.datehour;
				*(db->minute)=dr.dateminute;
				*(db->second)=dr.datesecond;
				*(db->microsecond)=dr.datemsecond;
				*(db->tz)=NULL;
				*(db->isnegative)=false;
			}
		}


		discardResults();
		ncols=0;
	}

	// return success only if no error was generated
	return (!freetdsconn->errorcode);
}

bool freetdscursor::knowsAffectedRows() {
	return knowsaffectedrows;
}

uint64_t freetdscursor::getAffectedRows() {
	// freetds can set affectedrows to -1 when a DDL query is run
	return (affectedrows>=0)?affectedrows:0;
}

uint32_t freetdscursor::colCount() {
	return ncols;
}

const char *freetdscursor::getColumnName(uint32_t col) {
	return column[col].name;
}

uint16_t freetdscursor::getColumnType(uint32_t col) {
	switch (column[col].datatype) {
		case CS_CHAR_TYPE:
			// ctlib reports char, varchar, nchar and nvarchar all
			// as CS_CHAR_TYPE.  Sybase also sends its systypes
			// usertype, which does tell them apart - 1 char,
			// 2 varchar, 24 nchar, 25 nvarchar - but MS SQL Server
			// sends 0 for all of them, so there char is the best
			// we can do.
			if (freetdsconn->sybasedb) {
				switch (column[col].usertype) {
					case 2:
						return VARCHAR_DATATYPE;
					case 24:
						return NCHAR_DATATYPE;
					case 25:
						return NVARCHAR_DATATYPE;
				}
			}
			return CHAR_DATATYPE;
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
			// 4 varbinary - but MS SQL Server sends 0 for both,
			// so there binary is the best we can do.  Sybase
			// timestamp columns are binary(8) and come back with
			// usertype 80, so they fall through to binary too.
			if (freetdsconn->sybasedb &&
					column[col].usertype==4) {
				return VARBINARY_DATATYPE;
			}
			return BINARY_DATATYPE;
		case CS_BIT_TYPE:
			return BIT_DATATYPE;
		case CS_REAL_TYPE:
			return REAL_DATATYPE;
		case CS_FLOAT_TYPE:
			// 0.91 (and earlier?) report decimal/numeric types as
			// float, at least against sybase.  If we get a float
			// type, then check for the library version and check
			// the usertype to decide what it really is, and update
			// its datatype and maxsize.  The usertype appears to
			// consistently be 26 for decimals and 10 for numerics.
			// 
			// It's a bit of a hack to do this here, because
			// there's no guarantee that getColumnType will be
			// called before getColumnSize, or at all, for that
			// matter, but all existing protocol modules do.
			if (majorversion==0 && minorversion<=91) {
				if (column[col].usertype==26) {
					column[col].datatype=CS_DECIMAL_TYPE;
					column[col].maxlength=35;
					return DECIMAL_DATATYPE;
				} else if (column[col].usertype==10) {
					column[col].datatype=CS_NUMERIC_TYPE;
					column[col].maxlength=35;
					return NUMERIC_DATATYPE;
				}
			}
			return FLOAT_DATATYPE;
		case CS_TEXT_TYPE:
			// ctlib reports text, unichar and univarchar all as
			// CS_TEXT_TYPE.  Sybase also sends its systypes
			// usertype, which does tell them apart - 19 text,
			// 34 unichar, 35 univarchar - but MS SQL Server sends
			// 0 for all of them, so there text is the best we can
			// do.  unichar and univarchar are fixed- and
			// variable-length utf-16, which is what nchar and
			// nvarchar already mean here.
			if (freetdsconn->sybasedb) {
				switch (column[col].usertype) {
					case 34:
						return NCHAR_DATATYPE;
					case 35:
						return NVARCHAR_DATATYPE;
				}
			}
			return TEXT_DATATYPE;
		case CS_VARCHAR_TYPE:
			return VARCHAR_DATATYPE;
		case CS_VARBINARY_TYPE:
			return VARBINARY_DATATYPE;
		case CS_LONGCHAR_TYPE:
			return LONGCHAR_DATATYPE;
		case CS_LONGBINARY_TYPE:
			return LONGBINARY_DATATYPE;
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
		#ifdef CS_UNIQUE_TYPE
		case CS_UNIQUE_TYPE:
			return UNIQUEIDENTIFIER_DATATYPE;
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
			// sybase bigtime and MS SQL Server time both land here
			return TIME_DATATYPE;
		#endif
		#ifdef CS_BIGDATETIME_TYPE
		case CS_BIGDATETIME_TYPE:
			// ctlib reports sybase bigdatetime and MS SQL Server
			// datetime2 and datetimeoffset all as
			// CS_BIGDATETIME_TYPE.  Sybase sends its systypes
			// usertype, 48 for bigdatetime, and freetds 1.4 and
			// later synthesize a usertype for MS SQL Server, 42
			// for datetime2 and 43 for datetimeoffset.  Prior to
			// 1.4 it sends 0 for both of those, so there
			// timestamp is the best we can do.
			if (column[col].usertype==43) {
				return DATETIMEOFFSET_DATATYPE;
			}
			return TIMESTAMP_DATATYPE;
		#endif
		default:
			return UNKNOWN_DATATYPE;
	}
}

void freetdscursor::deflateColumnSize(CS_INT index) {

	// bail if the client charset is a single-byte charset
	//
	// A factor of 1 doesn't prove that nothing was multiplied.  The ping
	// measures a varchar, and freetds skips the multiply when the column's
	// charset already matches the client charset, so if the server's
	// charset is the client charset then the ping measures 1 while the
	// unicode columns are still multiplied - those convert through utf-16,
	// which never matches.
	if (freetdsconn->bytesperchar<2) {
		return;
	}

	// only character columns are multiplied
	switch (column[index].datatype) {
		case CS_CHAR_TYPE:
		case CS_LONGCHAR_TYPE:
		case CS_VARCHAR_TYPE:
		case CS_TEXT_TYPE:
		case CS_UNICHAR_TYPE:
			break;
		default:
			return;
	}

	// When the multiplied size would overflow, freetds clamps it to
	// 2^31-1 rather than multiplying at all.  Every MS SQL Server lob
	// reports that size, so leave it alone.
	if (column[index].maxlength==2147483647) {
		return;
	}

	// A column whose own collation charset matches the client charset
	// isn't multiplied either, and gets scaled down anyway.  ct_describe
	// reports the multiplied size and nothing else - not the collation,
	// not the size the server sent - so there's no way to spot it.  MS SQL
	// Server on TDS 7.1 and later only, since SAP/Sybase has one
	// server-wide charset.  Use the odbc connection for those, it reports
	// the size the server sent.
	column[index].maxlength/=freetdsconn->bytesperchar;
}

uint32_t freetdscursor::getColumnSize(uint32_t col) {
	// limit the column size
	uint32_t	maxfieldsize=conn->cont->getMaxFieldSize();
	if ((uint32_t)column[col].maxlength>maxfieldsize) {
		column[col].maxlength=maxfieldsize;
	}
	return column[col].maxlength;
}

uint32_t freetdscursor::getColumnPrecision(uint32_t col) {
	return column[col].precision;
}

uint32_t freetdscursor::getColumnScale(uint32_t col) {
	return column[col].scale;
}

uint16_t freetdscursor::getColumnIsNullable(uint32_t col) {
	return (column[col].status&CS_CANBENULL);
}

uint16_t freetdscursor::getColumnIsPartOfKey(uint32_t col) {
	return (column[col].status&(CS_KEY|CS_VERSION_KEY));
}

uint16_t freetdscursor::getColumnIsUnsigned(uint32_t col) {
	return (getColumnType(col)==USHORT_DATATYPE);
}

uint16_t freetdscursor::getColumnIsBinary(uint32_t col) {
	return (getColumnType(col)==IMAGE_DATATYPE);
}

uint16_t freetdscursor::getColumnIsAutoIncrement(uint32_t col) {
	return (column[col].status&CS_IDENTITY);
}

bool freetdscursor::ignoreDateDdMmParameter(const char *data, uint32_t size) {

	// This is for a very FreeTDS/MSSQL Server-specific issue...
	//
	// If we're translating dates in the result set then we have to be
	// careful about dates in the yyyy-xx-xx format.
	//
	// FreeTDS recognizes Sybase date/datetime columns and MSSQL datetime
	// columns as type CS_DATETIME_TYPE and formats them according to the
	// rules defined in locales.conf for the locale of the SQL Relay server.
	//
	// FreeTDS prior to 1.1 recognized MSSQL date columns as type
	// CS_CHAR_TYPE and formatted them in yyyy-mm-dd format universally.
	// 1.1 and later report them as CS_DATE_TYPE and format them like the
	// other date/datetime columns, but string columns and string literals
	// can still be in yyyy-xx-xx format, so this is still needed.
	//
	// The date conversion routines take any fields that look like a date
	// and translate them to the specified format.  Unfortunately if you're
	// in a region where dates are formatted dd/mm/yyyy (for example) then
	// dateddmm="yes" needs to be set so string literals like "03/04/2005"
	// will be recognized as April 3, 2005.  This would cause problems when
	// fetching date columns from MSSQL so we have to ignore dateddmm in
	// that case.
	//
	// Ideally we'd only ignore dateddmm for MSSQL, on date/datetime columns
	// that appeared to be in the yyyy-xx-xx format.  But, this is called
	// per-field, without the column type, so we end up ignoring dateddmm
	// for everything in yyyy-xx-xx format, whether fetched from a date
	// column or from a string column or from a string literal in the
	// original query.
	//
	// That means that if date translation is in effect, if dates are stored
	// in string fields in yyyy-xx-xx format, then they must be stored in
	// yyyy-mm-dd format.
	//
	// It also means that if date translation in in effect then unless the
	// translatedatetimes translation module is being used to normalize
	// string literals in the original query to some other format, then
	// they have to be in yyyy-mm-dd format as well.

	// Override the dateddmm parameter if we're using MSSQL Server and the
	// fields is in yyyy-xx-xx format and appears to be a date.
	return (!freetdsconn->sybasedb &&
			size==10 &&
			data[4]=='-' && data[7]=='-' &&
			charstring::isNumber(data,4) &&
			charstring::isNumber(data+5,2) &&
			charstring::isNumber(data+8,2));
}

bool freetdscursor::noRowsToReturn() {
	// unless the query was a successful select, send no data
	return (resultstype!=CS_ROW_RESULT &&
			resultstype!=CS_CURSOR_RESULT &&
			resultstype!=CS_COMPUTE_RESULT);
}

bool freetdscursor::skipRow(bool *error) {
	if (fetchRow(error)) {
		row++;
		return true;
	}
	return false;
}

bool freetdscursor::fetchRow(bool *error) {

	*error=false;
	// FIXME: set error if an error occurs

	if (row==(CS_INT)getFetchAtOnce()) {
		row=0;
	}
	if (row>0 && row==maxrow) {
		return false;
	}
	if (!row) {
		CS_RETCODE	fetchresult=ct_fetch(cmd,CS_UNUSED,
							CS_UNUSED,
							CS_UNUSED,
							&rowsread);

		// This is essential with freetds.
		// http://www.freetds.org/faq.html#pending
		//
		// Basically the TDS protocol doesn't handle multiple
		// simultaneous queries per connection.  You can open multiple
		// cursors but you can't use them at the same time.  Somehow
		// Sybase gets around this but FreeTDS doesn't.  In particular,
		// unless all rows and all results sets have been fetched or
		// cancelled for all cursors, another cursor cannot run another
		// query.  Since TDS supports multiple result sets per query,
		// it's not enough to just fetch all of the rows, all result
		// sets must be fetched or cancelled as well until ct_results
		// returns CS_END_RESULTS or CS_CANCELLED.  Since SQL Relay only
		// supports one result set per query, we can go ahead and cancel
		// any remaining result sets here.  closeResultSet would do this
		// for us but not before another query gets run on the same
		// cursor, so we must explicitly call it here too in case
		// someone wants to do something with another cursor.
		if (fetchresult==CS_END_DATA) {
			discardResults();
		}

		if (fetchresult!=CS_SUCCEED || !rowsread) {
			if (fetchresult==CS_FAIL || fetchresult==CS_ROW_FAIL) {
				*error=true;
			}
			return false;
		}
		maxrow=rowsread;
		totalrows=totalrows+rowsread;
	}
	return true;
}

void freetdscursor::getField(uint32_t col,
				const char **field, uint64_t *fieldsize,
				bool *lob, bool *null) {

	// handle NULLs
	if (nullindicator[col][row]==-1) {
		*null=true;
		return;
	}

	// handle normal datatypes...

	// get the data and data size for this field,
	// trimming the null terminator
	char		*d=&data[col][row*conn->cont->getMaxFieldSize()];
	uint32_t	ds=datasize[col][row]-1;

	// decode text-encoded binary data
	// (unless the user has opted out via decodeblobs=no)
	if (column[col].datatype==CS_IMAGE_TYPE &&
				freetdsconn->getDecodeBlobs()) {
		decodeBlob(&d,&ds);
	}

	// return the field and field size
	*field=d;
	*fieldsize=ds;
}

void freetdscursor::nextRow() {
	row++;
}

void freetdscursor::closeResultSet() {

	if (clean) {
		return;
	}

	discardResults();
	discardCursor();

	clean=true;
}

void freetdscursor::discardResults() {

	// if there are any unprocessed result sets, process them
	if (results==CS_SUCCEED) {
		do {
			if (ct_cancel(NULL,cmd,CS_CANCEL_CURRENT)==CS_FAIL) {
				freetdsconn->liveconnection=false;
				// FIXME: call ct_close(CS_FORCE_CLOSE)?
			}
			results=ct_results(cmd,&resultstype);
		} while (results==CS_SUCCEED);
	}

	// also clears a prepared-but-unsent command (eg. a failed bind)
	if (ct_cancel(NULL,cmd,CS_CANCEL_ALL)==CS_FAIL) {
		freetdsconn->liveconnection=false;
		// FIXME: call ct_close(CS_FORCE_CLOSE)?
	}

	// Deallocating the result set buffers here causes a problem, but only
	// in the freetds connection.
	// When using freetds, we have to call discardResults() when we hit
	// the end of the result set (see note in fetchRow()) but if we
	// deallocate the result set buffers at that point, then subsequent
	// attempts to fetch column info, will result in a reference-after-free.
	// So, unlike the in the sybase/sap connection code we defer the
	// deallocate until right before the next allocate, in prepareQuery().
	/*if (!conn->cont->getMaxColumnCount()) {
		deallocateResultSetBuffers();
	}*/
}


void freetdscursor::discardCursor() {

	#ifdef FREETDS_SUPPORTS_CURSORS
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
	#endif
}

CS_RETCODE freetdsconnection::csMessageCallback(CS_CONTEXT *ctxt, 
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

	// for a read from sql server failed message,
	// set liveconnection to false
	} else if (CS_SEVERITY(msgp->msgnumber)==78 &&
		CS_LAYER(msgp->msgnumber)==0 &&
		CS_ORIGIN(msgp->msgnumber)==0 &&
		(CS_NUMBER(msgp->msgnumber)==36 ||
		CS_NUMBER(msgp->msgnumber)==38)) {
		liveconnection=false;
	}
	// FIXME: sybase connection has another case, do we need it?

	return CS_SUCCEED;
}

CS_RETCODE freetdsconnection::clientMessageCallback(CS_CONTEXT *ctxt, 
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
		CS_LAYER(msgp->msgnumber)==63 &&
		CS_ORIGIN(msgp->msgnumber)==63 &&
		CS_NUMBER(msgp->msgnumber)==63) {
		liveconnection=false;

	// for a read from sql server failed message,
	// set liveconnection to false
	} else if (CS_SEVERITY(msgp->msgnumber)==78 &&
		CS_LAYER(msgp->msgnumber)==0 &&
		CS_ORIGIN(msgp->msgnumber)==0 &&
		(CS_NUMBER(msgp->msgnumber)==36 ||
		CS_NUMBER(msgp->msgnumber)==38)) {
		liveconnection=false;
	}
	// FIXME: sybase connection has another case, do we need it?

	return CS_SUCCEED;
}

CS_RETCODE freetdsconnection::serverMessageCallback(CS_CONTEXT *ctxt, 
						CS_CONNECTION *cnn,
						CS_SERVERMSG *msgp) {

	// This is a special case, for some reason, "use db" queries
	// throw a warning, ignore them.
	if ((CS_NUMBER(msgp->msgnumber)==5701 &&
			(CS_SEVERITY(msgp->msgnumber)==10 ||
				CS_SEVERITY(msgp->msgnumber)==0)) ||
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
	errorstring.append("  Server Name:")->append(msgp->svrname);
	errorstring.append("  Procedure Name:")->append(msgp->proc);

	return CS_SUCCEED;
}

const char *freetdsconnection::tempTablePrefix() {
	return "#";
}

sqlrtxmodel_t freetdsconnection::getNativeTransactionModel() {
	return SQLRTXMODEL_EXPLICIT_ERROR;
}

bool freetdsconnection::commit() {
	cont->closeAllResultSets();
	return sqlrserverconnection::commit();
}

bool freetdsconnection::rollback() {
	cont->closeAllResultSets();
	return sqlrserverconnection::rollback();
}

void freetdsconnection::getError(char *errorbuffer,
					uint32_t errorbuffersize,
					uint32_t *errorsize,
					int64_t *errorcode,
					bool *liveconnection) {
	*errorsize=this->errorstring.getSize();
	charstring::safeCopy(errorbuffer,errorbuffersize,
				this->errorstring.getString(),*errorsize);
	*liveconnection=this->liveconnection;
	*errorcode=this->errorcode;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_freetdsconnection(
						sqlrservercontroller *cont) {
		return new freetdsconnection(cont);
	}
}
