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
		const char	*getDatabaseListQuery(const char *db);
		const char	*getSchemaListQuery(const char *db,
						const char *schema);
		const char	*getTableTypeListQuery(const char *db,
						const char *schema,
						const char *tabletypes);
		const char	*getTableListQuery(
						const char *db,
						const char *schema,
						const char *table,
						uint16_t objecttypes);
		const char	*getTypeInfoListQuery(
						const char *db,
						const char *schema,
						const char *type);
		const char	*getColumnListQuery(
						const char *db,
						const char *schema,
						const char *table,
						const char *column);
		const char	*getPrimaryKeysListQuery(
						const char *db,
						const char *schema,
						const char *table);
		const char	*getKeyAndIndexListQuery(
						const char *db,
						const char *schema,
						const char *table);
		const char	*getProcedureListQuery(
						const char *db,
						const char *schema,
						const char *procedure);
		const char	*getProcedureParameterListQuery(
						const char *db,
						const char *schema,
						const char *procedure);
		#ifdef SQLITE_TRANSACTIONAL
		const char	*setIsolationLevelQuery();
		const char	*getIsolationLevelQuery();
		const char	*mapIsolationLevel(
				const char *isolevel,
				sqlrserverisolationlevelformat_t fromformat,
				sqlrserverisolationlevelformat_t toformat);
		const char * const	*getDatabaseFeatures();
		const char	*beginTransactionQuery();
		#endif
		bool		selectDatabase(const char *database);
		char		*getCurrentDatabase();
		bool		getLastInsertId(uint64_t *id);
		const char	*getNoopQuery();
		#ifdef SQLITE3
		char		*duplicate(const char *str);
		#endif
		#ifndef SQLITE_TRANSACTIONAL
		bool		isTransactional();
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

		#ifdef SQLITE3
		sqlite3	*sqliteptr;
		#else
		sqlite	*sqliteptr;
		#endif
		char	*errmesg;
		int64_t	errcode;

		char	*hostname;
		char	**databasefeatures;

		stringbuffer	tablelistquery;
		stringbuffer	tabletypelistquery;
		stringbuffer	columnlistquery;
		stringbuffer	typeinfolistquery;
		stringbuffer	primarykeyslistquery;
		stringbuffer	keyandindexlistquery;
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
		bool		knowsAffectedRows();
		uint32_t	colCount();
		const char	*getColumnName(uint32_t col);
		#ifdef HAVE_SQLITE3_STMT
		uint16_t	getColumnType(uint32_t col);
		const char	*getColumnTable(uint32_t col);
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
		bool		lastinsertrowid;

		#ifdef HAVE_SQLITE3_STMT
		char		**columntables;
		int		*columntypes;
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
	databasefeatures=NULL;
	db=NULL;
}

sqliteconnection::~sqliteconnection() {
	clearErrors();
	delete[] hostname;
	if (databasefeatures) {
		for (int i=0; i<FEATURE_COUNT; i++) {
			delete[] databasefeatures[i];
		}
		delete[] databasefeatures;
	}
	delete[] db;
}

void sqliteconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	db=charstring::duplicate(cont->getConnectStringValue("db"));

	cont->setFetchAtOnce(1);
	cont->setMaxColumnCount(0);
	cont->setMaxFieldSize(0);
}

bool sqliteconnection::logIn(const char **error, const char **warning) {
#ifdef SQLITE_TRANSACTIONAL
	#ifdef SQLITE3
		if (sqlite3_open(db,&sqliteptr)==SQLITE_OK) {
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

const char *sqliteconnection::getDatabaseListQuery(const char *db) {
	//return "pragma database_list";
	return "select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null";
}

const char *sqliteconnection::getSchemaListQuery(const char *db,
						const char *schema) {
	return "select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null "
		"where "
		"	1=0";
}

const char *sqliteconnection::getTableTypeListQuery(const char *db,
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
		"	null "
		"from "
		"(select 'TABLE' as table_type "
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

const char *sqliteconnection::getTableListQuery(const char *db,
						const char *schema,
						const char *table,
						uint16_t objecttypes) {

	tablelistquery.clear();

	// select clause
	tablelistquery.append(
		"select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	tbl_name as table_name, "
		"	'TABLE' as table_type, "
		"	'' as remarks, "
		"	null "
		"from "
		"( "
		"select "
		"	tbl_name "
		"from "
		"	sqlite_master "
		"where ");
	stringbuffer	otypes;
	otypes.append("	(");
	if (objecttypes&DB_OBJECT_TABLE) {
		otypes.append("	type='table' ");
	}
	if (objecttypes&DB_OBJECT_VIEW) {
		if (otypes.getSize()) {
			otypes.append("	or ");
		}
		otypes.append("	type='view' ");
	}
	otypes.append(") ");
	tablelistquery.append(otypes.getString());
	tablelistquery.append(
		"union all "
		"select "
		"	tbl_name "
		"from "
		"	sqlite_temp_master "
		"where ");
	tablelistquery.append(otypes.getString());
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

const char *sqliteconnection::getTypeInfoListQuery(const char *db,
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

const char *sqliteconnection::getColumnListQuery(const char *db,
					const char *schema,
					const char *table,
					const char *column) {

	columnlistquery.clear();

	columnlistquery.append(
		"select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	'")->append(table)->append("' as table_name, "
		"	p.name as column_name, "
		"	null as data_type, "
		"	p.type as type_name, "
		"	null as column_size, "
		"	null as buffer_length, "
		"	null as decimal_digits, "
		"	10 as num_prec_radix, "
		"	case "
		"		when p.'notnull'=1 then 0 "
		"		else 1 "
		"	end as nullable, "
		"	(select case when count(*)>0 then 'auto_increment' else '' end from sqlite_master where name='")->append(table)->append("' and instr(upper(sql),concat(' (',upper(p.name),' INTEGER PRIMARY KEY AUTOINCREMENT'))) as remarks, "
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
		"	null "
		"from "
		"	(select "
		"		* "
		"	from "
		"		pragma_table_info('")->append(table)->append("')) p ");
	if (!charstring::isNullOrEmpty(column)) {
		columnlistquery.append(
			"where "
			"	upper(p.name) like upper('")->append(column)->append("') ");
	}
	columnlistquery.append(
		"order by "
		"	p.cid");

	return columnlistquery.getString();
}

const char *sqliteconnection::getPrimaryKeysListQuery(const char *db,
					const char *schema,
					const char *table) {

	primarykeyslistquery.clear();
	primarykeyslistquery.append(
		"select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	'")->append(table)->append("' as table_name, "
		"	p.name as column_name, "
		"	p.pk as key_seq, "
		"	'' as pk_name, "
		"	null ");

	// from clause
	primarykeyslistquery.append(
		"from "
		"	(select "
		"		* "
		"	from "
		"		pragma_table_info('")->append(table)->append("')) p ");

	// where clause
	primarykeyslistquery.append(
		"where "
		"	p.pk>0 "
		"order by "
		"	p.pk");

	return primarykeyslistquery.getString();
}

const char *sqliteconnection::getKeyAndIndexListQuery(const char *db,
					const char *schema,
					const char *table) {

	keyandindexlistquery.clear();
	keyandindexlistquery.append(
		"select "
		"	'' as table_cat, "
		"	'' as table_schem, "
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
		"	case il.'unique' "
		"		when 1 then 'A' "
		"		else 'A' "
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
		"		pragma_index_list('")->append(table)->append("')) il, "
		"	pragma_index_info(il.name) ii "
		"order by "
		"	il.name, "
		"	ii.seqno");

	return keyandindexlistquery.getString();
}

const char *sqliteconnection::getProcedureListQuery(const char *db,
						const char *schema,
						const char *procedure) {
	return "select "
		"	'' as procedure_cat, "
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
					const char *db,
					const char *schema,
					const char *procedure) {
	return "select "
		"	'' as procedure_cat, "
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
				"TRANSACTION_READ_COMMITTED")) {
			return "0";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_JDBC) {
		if (!charstring::compareIgnoringCase(isolevel,"1")) {
			return "TRANSACTION_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"0")) {
			return "TRANSACTION_READ_COMMITTED";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE) {
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_UNCOMMITTED")) {
			return "1";
		}
		if (!charstring::compare(isolevel,
				"SQL_TXN_READ_COMMITTED")) {
			return "0";
		}
	} else if (fromformat==SQLRSERVERISOLATIONLEVELFORMAT_NATIVE &&
			toformat==SQLRSERVERISOLATIONLEVELFORMAT_ODBC) {
		if (!charstring::compareIgnoringCase(isolevel,"1")) {
			return "SQL_TXN_READ_UNCOMMITTED";
		}
		if (!charstring::compareIgnoringCase(isolevel,"0")) {
			return "SQL_TXN_READ_COMMITTED";
		}
	}
	return isolevel;
}

const char * const *sqliteconnection::getDatabaseFeatures() {

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

const char *sqliteconnection::beginTransactionQuery() {
	return "begin transaction";
}
#endif

bool sqliteconnection::selectDatabase(const char *database) {

	// keep track of the original db and host
	char	*originaldb=db;

	// reset the db/host
	db=charstring::duplicate(database);

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

char *sqliteconnection::getCurrentDatabase() {
	return charstring::duplicate(db);
}

bool sqliteconnection::getLastInsertId(uint64_t *id) {
	*id=sqlite3_last_insert_rowid(sqliteptr);
	return true;
}

const char *sqliteconnection::getNoopQuery() {
	return "pragma noop";
}

#ifndef SQLITE_TRANSACTIONAL
bool sqliteconnection::isTransactional() {
	return false;
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
	lastinsertrowid=false;
	#ifdef HAVE_SQLITE3_STMT
	columntables=NULL;
	columntypes=NULL;
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
	return (sqlite3_bind_text(stmt,
				getBindVariableIndex(variable,variablesize),
				value,valuesize,SQLITE_STATIC)==SQLITE_OK);
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

	// cache off the columns so they can be returned later if the result
	// set is suspended/resumed
	#ifdef HAVE_SQLITE3_STMT
	columntables=new char *[ncolumn];
	columnnames=new char *[ncolumn];
	columntypes=new int[ncolumn];
	if (lastinsertrowid) {
		columntables[0]=charstring::duplicate("");
		columnnames[0]=charstring::duplicate("LASTINSERTROWID");
		columntypes[0]=INTEGER_DATATYPE;
	} else {
		for (int i=0; i<ncolumn; i++) {
			columntables[i]=
				charstring::duplicate(
					#ifdef HAVE_SQLITE3_COLUMN_TABLE_NAME
					sqlite3_column_table_name(stmt,i)
					#else
					""
					#endif
					);
			columnnames[i]=charstring::duplicate(
					sqlite3_column_name(stmt,i));
			columntypes[i]=sqlite3_column_type(stmt,i);
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
	#endif

	// reset counters and flags
	nrow=0;
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

bool sqlitecursor::knowsAffectedRows() {
	return false;
}

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

	// Get the type before calling sqlite3_column_text.
	// sqlite3_column_text does a type conversion and the result of
	// sqlite3_column_type is undefined after the conversion.
	int	dtype=sqlite3_column_type(stmt,col);
	*field=(const char *)((dtype==SQLITE_BLOB)?
				sqlite3_column_blob(stmt,col):
				sqlite3_column_text(stmt,col));
	*fieldsize=sqlite3_column_bytes(stmt,col);
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
