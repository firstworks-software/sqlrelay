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
		void		handleConnectString();
		bool		logIn(const char **error, const char **warning);
		const char	*logInError(const char *error, uint16_t stage);
		sqlrservercursor	*newCursor(uint16_t id);
		void		deleteCursor(sqlrservercursor *curs);
		void		logOut();
		const char	*getDbType();
		const char	*getDbVersion();
		const char	*getDbHostNameQuery();
		const char	*getDatabaseListQuery(bool wild);
		const char	*getTableListQuery(bool wild,
						uint16_t objecttypes,
						bool currentschemaonly);
		const char	*getColumnListQuery(
						const char *table, bool wild);
		const char	*selectDatabaseQuery();
		const char	*getCurrentDatabaseQuery();
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
		const char	*lang;
		const char	*server;
		const char	*db;
		const char	*charset;
		const char	*language;
		const char	*hostname;
		const char	*packetsize;

		bool		dbused;

		char		*dbversion;
		char		**databasefeatures;

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

		stringbuffer	tablelistquery;
		stringbuffer	columnlistquery;
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
		void		encodeBlob(stringbuffer *buffer,
						const char *data,
						uint32_t datasize);
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

		char		*cursorname;
		size_t		cursornamesize;

		void		checkRePrepare();

		CS_COMMAND	*languagecmd;
		CS_COMMAND	*cursorcmd;
		CS_COMMAND	*cmd;
		CS_INT		results;
		CS_INT		resultstype;
		CS_INT		ncols;
		CS_INT		affectedrows;

		CS_INT		rowsread;
		CS_INT		row;
		CS_INT		maxrow;
		CS_INT		totalrows;

		uint16_t	maxbindcount;
		CS_DATAFMT	*parameter;
		uint16_t	paramindex;
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

		const char	*query;
		uint32_t	size;
		bool		prepared;
		bool		clean;

		sapconnection	*sapconn;
};


stringbuffer	sapconnection::errorstring;
int64_t		sapconnection::errorcode;
bool		sapconnection::liveconnection;


sapconnection::sapconnection(sqlrservercontroller *cont) :
					sqlrserverconnection(cont) {
	dbused=false;
	dbversion=NULL;
	databasefeatures=NULL;
}

sapconnection::~sapconnection() {
	delete[] dbversion;
	for (int i=1; i<=MAXFEATURE; i++) {
		delete[] databasefeatures[i];
	}
	delete[] databasefeatures;
}

void sapconnection::handleConnectString() {

	sqlrserverconnection::handleConnectString();

	sybase=cont->getConnectStringValue("sybase");
	lang=cont->getConnectStringValue("lang");
	server=cont->getConnectStringValue("server");
	db=cont->getConnectStringValue("db");
	charset=cont->getConnectStringValue("charset");
	language=cont->getConnectStringValue("language");
	hostname=cont->getConnectStringValue("hostname");
	packetsize=cont->getConnectStringValue("packetsize");

	if (cont->getMaxColumnCount()==1) {
		// if max column count is set to 1 then force it
		// to 2 so the db version detection doesn't crash
		cont->setMaxColumnCount(2);
	}
}

bool sapconnection::logIn(const char **error, const char **warning) {

	// set sybase
	if (!charstring::isNullOrEmpty(sybase) &&
			!environment::setValue("SYBASE",sybase)) {
		*error=logInError(
			"Failed to set SYBASE environment variable.",1);
		return false;
	}

	// set lang
	if (!charstring::isNullOrEmpty(lang) &&
			!environment::setValue("LANG",lang)) {
		*error=logInError(
			"Failed to set LANG environment variable.",1);
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

	// allocate a context
	context=(CS_CONTEXT *)NULL;
	if (cs_ctx_alloc(CS_VERSION_100,&context)!=CS_SUCCEED) {
		*error=logInError(
			"Failed to allocate a context structure",2);
		return false;
	}

	// init the context
	if (ct_init(context,CS_VERSION_100)!=CS_SUCCEED) {
		*error=logInError(
			"Failed to initialize a context structure",3);
		return false;
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
	const char	*user=cont->getUser();
	if (ct_con_props(dbconn,CS_SET,CS_USERNAME,
		(CS_VOID *)((!charstring::isNullOrEmpty(user))?user:""),
		(CS_INT)charstring::getLength(user),
		(CS_INT *)NULL)!=CS_SUCCEED) {
		*error=logInError("Failed to set the user",5);
		return false;
	}


	// set the password to use
	const char	*password=cont->getPassword();
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
	bool	retval=true;
	CS_COMMAND	*cmd;
	if (ct_cmd_alloc(dbconn,&cmd)!=CS_SUCCEED) {
		*error=logInError("Failed to allocate ping command",6);
		return false;
	}
	if (ct_command(cmd,CS_LANG_CMD,(CS_CHAR *)"select 1",8,
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
	}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);
	ct_cmd_drop(cmd);

	return retval;
}

const char *sapconnection::logInError(const char *error, uint16_t stage) {

	loginerror.clear();
	if (error) {
		loginerror.append(error)->append(": ");
	}
	if (errorstring.getSize()) {
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

const char *sapconnection::getDatabaseListQuery(bool wild) {
	return "select "
		"	'' as table_cat, "
		"	'' as table_schem, "
		"	'' as table_name, "
		"	'' as table_type, "
		"	'' as remarks, "
		"	null";
}

const char *sapconnection::getTableListQuery(bool wild,
						uint16_t objecttypes,
						bool currentschemaonly) {

	stringbuffer	otypes;
	otypes.append("	(");
	if (objecttypes&DB_OBJECT_TABLE) {
		otypes.append("	type='U' ");
	}
	if (objecttypes&DB_OBJECT_VIEW) {
		if (otypes.getSize()) {
			otypes.append("	or ");
		}
		otypes.append("	type='V' ");
	}
	otypes.append(") ");

	tablelistquery.clear();
	tablelistquery.append(
		"select "
		"	'' as table_cat, "
		"	loginame as table_schem, "
		"	name as table_name, "
		"	'TABLE' as table_type, "
		"	'' as remarks, "
		"	null "
		"from "
		"	sysobjects "
		"where "
		"	loginame is not null ");
	if (currentschemaonly) {
		tablelistquery.append(
			"	and "
			"	upper(loginame)=upper('");
		tablelistquery.append(cont->getUser());
		tablelistquery.append("') ");
	}
	tablelistquery.append(
		"	and ");
	tablelistquery.append(otypes.getString());
	if (wild) {
		tablelistquery.append(
			"	and "
			"	name like '%s' ");
	}
	tablelistquery.append(
		"order by "
		"	name");

	return tablelistquery.getString();
}

const char *sapconnection::getColumnListQuery(
					const char *table, bool wild) {

	columnlistquery.clear();

	// select clause
	columnlistquery.append(
		"select "
		"	'' as table_cat, "
		"	'' as table_schem, "
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
		"			then 'auto_increment ' "
		"		else '' "
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
		"		else '' "
		"	end as column_key, "
		"	null ");

	// from clause
	columnlistquery.append(
		"from "
		"	sysobjects ob, "
		"	syscolumns co "
		"	left outer join ( "
		"		select "
		"			kc.id, "
		"			kc.colno, "
		"			min(case sk.type "
		"				when 1 then 1 "
		"				when 2 then 3 "
		"			end) as key_priority "
		"		from "
		"			( "
		"			select id, key1 as colno from syskeys where key1>0 "
		"			union all "
		"			select id, key2 as colno from syskeys where key2>0 "
		"			union all "
		"			select id, key3 as colno from syskeys where key3>0 "
		"			union all "
		"			select id, key4 as colno from syskeys where key4>0 "
		"			union all "
		"			select id, key5 as colno from syskeys where key5>0 "
		"			union all "
		"			select id, key6 as colno from syskeys where key6>0 "
		"			union all "
		"			select id, key7 as colno from syskeys where key7>0 "
		"			union all "
		"			select id, key8 as colno from syskeys where key8>0 "
		"			) kc, "
		"			syskeys sk "
		"		where "
		"			sk.id=kc.id "
		"		group by "
		"			kc.id, "
		"			kc.colno "
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
		"	ob.name='%s' "
		"	and "
		"	co.id=ob.id "
		"	and "
		"	ty.usertype=co.usertype ");
	if (wild) {
		columnlistquery.append(
			"	and "
			"	co.name like '%s' ");
	}

	// order by clause
	columnlistquery.append(
		"order by "
		"	co.colid");

	return columnlistquery.getString();
}

const char *sapconnection::selectDatabaseQuery() {
	return "use %s";
}

const char *sapconnection::getCurrentDatabaseQuery() {
	return "select db_name()";
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
		if (!charstring::compare(isolevel,"0")) {
			return "TRANSACTION_READ_UNCOMMITTED";
		}
		if (!charstring::compare(isolevel,"1")) {
			return "TRANSACTION_READ_COMMITTED";
		}
		if (!charstring::compare(isolevel,"2")) {
			return "TRANSACTION_REPEATABLE_READ";
		}
		if (!charstring::compare(isolevel,"3")) {
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
		if (!charstring::compare(isolevel,"0")) {
			return "SQL_TXN_READ_UNCOMMITTED";
		}
		if (!charstring::compare(isolevel,"1")) {
			return "SQL_TXN_READ_COMMITTED";
		}
		if (!charstring::compare(isolevel,"2")) {
			return "SQL_TXN_REPEATABLE_READ";
		}
		if (!charstring::compare(isolevel,"3")) {
			return "SQL_TXN_SERIALIZABLE";
		}
	}
	return isolevel;
}

const char * const *sapconnection::getDatabaseFeatures() {

	if (databasefeatures) {
		return databasefeatures;
	}

	databasefeatures=new char *[MAXFEATURE+1];
	databasefeatures[FEATURE_ALL_PROCEDURES_ARE_CALLABLE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_ALL_TABLES_ARE_SELECTABLE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_AUTO_COMMIT_FAILURE_CLOSES_ALL_RESULT_SETS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_CATALOG_SEPARATOR]=
		charstring::duplicate("");
	databasefeatures[FEATURE_CATALOG_TERM]=
		charstring::duplicate("");
	databasefeatures[FEATURE_DATA_DEFINITION_CAUSES_TRANSACTION_COMMIT]=
		charstring::duplicate("");
	databasefeatures[FEATURE_DATA_DEFINITION_IGNORED_IN_TRANSACTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_DOES_MAX_ROW_SIZE_INCLUDE_BLOBS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_EXTRA_NAME_CHARACTERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_GENERATED_KEY_ALWAYS_RETURNED]=
		charstring::duplicate("");
	databasefeatures[FEATURE_IDENTIFIER_QUOTE_STRING]=
		charstring::duplicate("");
	databasefeatures[FEATURE_IS_CATALOG_AT_START]=
		charstring::duplicate("");
	databasefeatures[FEATURE_IS_READ_ONLY]=
		charstring::duplicate("");
	databasefeatures[FEATURE_LOCATORS_UPDATE_COPY]=
		charstring::duplicate("");
	databasefeatures[FEATURE_MAX_BINARY_LITERAL_LENGTH]=
		charstring::duplicate("");
	databasefeatures[FEATURE_MAX_CATALOG_NAME_LENGTH]=
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
		charstring::duplicate("");
	databasefeatures[FEATURE_MAX_CURSOR_NAME_LENGTH]=
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
	databasefeatures[FEATURE_NULL_PLUS_NON_NULL_IS_NULL]=
		charstring::duplicate("");
	databasefeatures[FEATURE_NULLS_ARE_SORTED_AT_END]=
		charstring::duplicate("");
	databasefeatures[FEATURE_NULLS_ARE_SORTED_AT_START]=
		charstring::duplicate("");
	databasefeatures[FEATURE_NULLS_ARE_SORTED_HIGH]=
		charstring::duplicate("");
	databasefeatures[FEATURE_NULLS_ARE_SORTED_LOW]=
		charstring::duplicate("");
	databasefeatures[FEATURE_NUMERIC_FUNCTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_PROCEDURE_TERM]=
		charstring::duplicate("");
	databasefeatures[FEATURE_RESULT_SET_HOLDABILITY]=
		charstring::duplicate("");
	databasefeatures[FEATURE_ROW_ID_LIFETIME]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SQL_KEYWORDS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SQL_STATE_TYPE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SCHEMA_TERM]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SEARCH_STRING_ESCAPE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_STORES_LOWER_CASE_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_STORES_LOWER_CASE_QUOTED_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_STORES_MIXED_CASE_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_STORES_MIXED_CASE_QUOTED_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_STORES_UPPER_CASE_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_STORES_UPPER_CASE_QUOTED_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_STRING_FUNCTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_ANSI92_ENTRY_LEVEL_SQL]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_ANSI92_FULL_SQL]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_ANSI92_INTERMEDIATE_SQL]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_ALTER_TABLE_WITH_ADD_COLUMN]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_ALTER_TABLE_WITH_DROP_COLUMN]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_BATCH_UPDATES]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CATALOGS_IN_DATA_MANIPULATION]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CATALOGS_IN_INDEX_DEFINITIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CATALOGS_IN_PRIVILEGE_DEFINITIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CATALOGS_IN_PROCEDURE_CALLS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CATALOGS_IN_TABLE_DEFINITIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_COLUMN_ALIASING]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CONVERT]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CORE_SQL_GRAMMAR]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_CORRELATED_SUBQUERIES]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_DATA_DEFINITION_AND_DATA_MANIPULATION_TRANSACTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_DATA_MANIPULATION_TRANSACTIONS_ONLY]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_DIFFERENT_TABLE_CORRELATION_NAMES]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_EXPRESSIONS_IN_ORDER_BY]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_EXTENDED_SQL_GRAMMAR]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_FULL_OUTER_JOINS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_GET_GENERATED_KEYS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_GROUP_BY]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_GROUP_BY_BEYOND_SELECT]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_GROUP_BY_UNRELATED]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_INTEGRITY_ENHANCEMENT_FACILITY]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_LIKE_ESCAPE_CLAUSE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_LIMITED_OUTER_JOINS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_MINIMUM_SQL_GRAMMAR]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_MIXED_CASE_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_MIXED_CASE_QUOTED_IDENTIFIERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_RESULT_SETS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_MULTIPLE_TRANSACTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_NAMED_PARAMETERS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_NON_NULLABLE_COLUMNS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_OPEN_CURSORS_ACROSS_COMMIT]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_OPEN_CURSORS_ACROSS_ROLLBACK]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_OPEN_STATEMENTS_ACROSS_COMMIT]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_OPEN_STATEMENTS_ACROSS_ROLLBACK]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_ORDER_BY_UNRELATED]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_OUTER_JOINS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_POSITIONED_DELETE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_POSITIONED_UPDATE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SAVEPOINTS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SCHEMAS_IN_DATA_MANIPULATION]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SCHEMAS_IN_INDEX_DEFINITIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SCHEMAS_IN_PRIVILEGE_DEFINITIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SCHEMAS_IN_PROCEDURE_CALLS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SCHEMAS_IN_TABLE_DEFINITIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SELECT_FOR_UPDATE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_STORED_FUNCTIONS_USING_CALL_SYNTAX]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_STORED_PROCEDURES]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SUBQUERIES_IN_COMPARISONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SUBQUERIES_IN_EXISTS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SUBQUERIES_IN_INS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_SUBQUERIES_IN_QUANTIFIEDS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_TABLE_CORRELATION_NAMES]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_TRANSACTION_ISOLATION_LEVEL]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_TRANSACTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_UNION]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SUPPORTS_UNION_ALL]=
		charstring::duplicate("");
	databasefeatures[FEATURE_SYSTEM_FUNCTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_TIME_DATE_FUNCTIONS]=
		charstring::duplicate("");
	databasefeatures[FEATURE_USES_LOCAL_FILE_PER_TABLE]=
		charstring::duplicate("");
	databasefeatures[FEATURE_USES_LOCAL_FILES]=
		charstring::duplicate("");

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

	cursornamesize=charstring::getIntegerLength(id);
	cursorname=charstring::parseNumber(id);

	maxbindcount=conn->cont->getConfig()->getMaxBindCount();
	parameter=new CS_DATAFMT[maxbindcount];
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
	delete[] cursorname;
	delete[] parameter;
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
		retval=(retval && (ct_cmd_drop(cursorcmd)==CS_SUCCEED));
		cursorcmd=NULL;
	}
	cmd=NULL;
	return retval;
}

bool sapcursor::prepareQuery(const char *query, uint32_t size) {

	// initialize column count
	ncols=0;

	clean=true;

	this->query=(char *)query;
	this->size=size;

	paramindex=0;
	outbindindex=0;

	if ((!charstring::compare(query,"select",6) ||
		!charstring::compare(query,"SELECT",6)) &&
		character::isWhitespace(query[6])) {

		// initiate a cursor command
		// (don't use CS_NULLTERM for the 4th parameter, it randomly
		// causes weird things to happen)
		cmd=cursorcmd;
		if (ct_cursor(cursorcmd,
				CS_CURSOR_DECLARE,
				(CS_CHAR *)cursorname,
				(CS_INT)cursornamesize,
				(CS_CHAR *)query,
				size,
				CS_READ_ONLY)!=CS_SUCCEED) {
			return false;
		}

	} else if ((!charstring::compare(query,"exec",4) ||
			!charstring::compare(query,"EXEC",4)) &&
					character::isWhitespace(query[4])) {

		// initiate an rpc command
		cmd=languagecmd;
		if (ct_command(languagecmd,
				CS_RPC_CMD,
				(CS_CHAR *)query+5,
				size-5,
				CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}

	} else if ((!charstring::compare(query,"execute",7) ||
			!charstring::compare(query,"EXECUTE",7)) &&
					character::isWhitespace(query[7])) {

		// initiate an rpc command
		cmd=languagecmd;
		if (ct_command(languagecmd,
				CS_RPC_CMD,
				(CS_CHAR *)query+8,
				size-8,
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

	buffer->append("0x");
	for (uint32_t i=0; i<datasize; i++) {
		buffer->append(conn->cont->asciiToHex(data[i]));
	}
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

bool sapcursor::inputBind(const char *variable,
				uint16_t variablesize,
				const char *value,
				uint32_t valuesize,
				int16_t *isnull) {

	checkRePrepare();

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));
	if (charstring::isInteger(variable+1,variablesize-1)) {
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
	if (ct_param(cmd,&parameter[paramindex],
		(CS_VOID *)value,valuesize,0)!=CS_SUCCEED) {
		return false;
	}
	paramindex++;
	return true;
}

bool sapcursor::inputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value) {

	checkRePrepare();

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));
	if (charstring::isInteger(variable+1,variablesize-1)) {
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
	if (ct_param(cmd,&parameter[paramindex],
		(CS_VOID *)value,sizeof(int64_t),0)!=CS_SUCCEED) {
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

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));
	if (charstring::isInteger(variable+1,variablesize-1)) {
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
	if (ct_param(cmd,&parameter[paramindex],
		(CS_VOID *)value,sizeof(double),0)!=CS_SUCCEED) {
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

	outbindtype[outbindindex]=CS_CHAR_TYPE;
	outbindstrings[outbindindex]=value;
	outbindstringsizes[outbindindex]=valuesize;
	outbindindex++;

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));
	if (charstring::isInteger(variable+1,variablesize-1)) {
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

bool sapcursor::outputBind(const char *variable,
				uint16_t variablesize,
				int64_t *value,
				int16_t *isnull) {

	checkRePrepare();

	outbindtype[outbindindex]=CS_INT_TYPE;
	outbindints[outbindindex]=value;
	outbindindex++;

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));
	if (charstring::isInteger(variable+1,variablesize-1)) {
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

bool sapcursor::outputBind(const char *variable,
				uint16_t variablesize,
				double *value,
				uint32_t *precision,
				uint32_t *scale,
				int16_t *isnull) {

	checkRePrepare();

	outbindtype[outbindindex]=CS_FLOAT_TYPE;
	outbinddoubles[outbindindex]=value;
	outbindindex++;

	bytestring::zero(&parameter[paramindex],sizeof(parameter[paramindex]));
	if (charstring::isInteger(variable+1,variablesize-1)) {
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
	if (charstring::isInteger(variable+1,variablesize-1)) {
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

bool sapcursor::executeQuery(const char *query, uint32_t size) {

	// clear out any errors
	sapconn->errorcode=0;
	sapconn->liveconnection=true;

	// initialize row counts
	affectedrows=0;
	row=0;
	maxrow=0;
	totalrows=0;

	if (cmd==cursorcmd) {
		if (ct_cursor(cursorcmd,CS_CURSOR_ROWS,
					NULL,CS_UNUSED,
					NULL,CS_UNUSED,
					(CS_INT)getFetchAtOnce())!=CS_SUCCEED) {
			return false;
		}
		if (ct_cursor(cursorcmd,CS_CURSOR_OPEN,
					NULL,CS_UNUSED,
					NULL,CS_UNUSED,
					CS_UNUSED)!=CS_SUCCEED) {
			return false;
		}
	}

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
			if (ct_res_info(cmd,CS_ROW_COUNT,
				(CS_VOID *)&affectedrows,
				CS_UNUSED,(CS_INT *)NULL)!=CS_SUCCEED) {
				return false;
			}
		}  else if (resultstype==CS_PARAM_RESULT ||
				resultstype==CS_ROW_RESULT ||
				resultstype==CS_CURSOR_RESULT ||
				resultstype==CS_COMPUTE_RESULT) {
			break;
		}

		// the result set was a type that we want to ignore
		if (ct_cancel(NULL,cmd,CS_CANCEL_CURRENT)==CS_FAIL) {
			sapconn->liveconnection=false;
			// FIXME: call ct_close(CS_FORCE_CLOSE)
			return false;
		}
	}

	checkForTempTable(query,size);

	// reset the prepared flag
	prepared=false;

	// For queries which return rows or parameters (output bind variables),
	// get the column count and bind columns.
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
			allocateResultSetBuffers(ncols);
		} else if ((uint32_t)ncols>maxcolumncount) {
			ncols=maxcolumncount;
		}

		// bind columns
		for (CS_INT i=0; i<ncols; i++) {

			// reset the column-bind
			column[i]=templatecolumn;

			// actually...
			// if we're getting the output bind variables of a
			// stored procedure that returns dates, then use
			// the datetime type instead...
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
			}
		}

	}

	// if we're doing an rpc query, the result set should be a single
	// row of output parameter results, fetch it and populate the output
	// bind variables...
	if (resultstype==CS_PARAM_RESULT) {

		if (ct_fetch(cmd,CS_UNUSED,
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
				bytestring::zero(&dr,sizeof(CS_DATEREC));
				cs_dt_crack(sapconn->context,
						CS_DATETIME_TYPE,
						(CS_VOID *)data[i],&dr);

				datebind	*db=&outbinddates[i];
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

		discardResults();
		ncols=0;
	}

	// return success only if no error was generated
	return (!sapconn->errorcode);
}

uint64_t sapcursor::getAffectedRows() {
	return affectedrows;
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
			return BINARY_DATATYPE;
		case CS_BIT_TYPE:
			return BIT_DATATYPE;
		case CS_REAL_TYPE:
			return REAL_DATATYPE;
		case CS_FLOAT_TYPE:
			return FLOAT_DATATYPE;
		case CS_TEXT_TYPE:
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
		default:
			return UNKNOWN_DATATYPE;
	}
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
	return (resultstype!=CS_ROW_RESULT &&
			resultstype!=CS_CURSOR_RESULT &&
			resultstype!=CS_COMPUTE_RESULT);
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

	// handle NULLs
	if (nullindicator[col][row]==-1) {
		*null=true;
		return;
	}

	// handle normal datatypes
	*field=&data[col][row*conn->cont->getMaxFieldSize()];
	*fieldsize=datasize[col][row]-1;
}

void sapcursor::nextRow() {
	row++;
}

void sapcursor::closeResultSet() {

	if (clean) {
		return;
	}

	discardResults();
	discardCursor();

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

	if (results==CS_FAIL) {
		if (ct_cancel(NULL,cmd,CS_CANCEL_ALL)==CS_FAIL) {
			sapconn->liveconnection=false;
			// FIXME: call ct_close(CS_FORCE_CLOSE)?
		}
	}

	if (!conn->cont->getMaxColumnCount()) {
		deallocateResultSetBuffers();
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
	*errorsize=this->errorstring.getSize();
	charstring::safeCopy(errorbuffer,errorbuffersize,
				this->errorstring.getString(),*errorsize);
	*liveconnection=this->liveconnection;
	*errorcode=this->errorcode;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrserverconnection *new_sapconnection(
						sqlrservercontroller *cont) {
		return new sapconnection(cont);
	}
}
