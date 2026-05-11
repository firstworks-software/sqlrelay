/* Copyright (c) David Muse
   See the file COPYING for more information */

#include <sqlrelay/sqlrclient.h>

extern "C" {

#include <sqlrelay/sqlrclientwrapper.h>

sqlrcon sqlrcon_alloc(const char *server, uint16_t port, const char *socket,
				const char *user, const char *password, 
				int32_t retrytime, int32_t tries) {
	return sqlrcon_alloc_copyrefs(server,port,socket,user,password,
							retrytime,tries,0);
}

sqlrcon sqlrcon_alloc_copyrefs(const char *server,
				uint16_t port, const char *socket,
				const char *user, const char *password, 
				int32_t retrytime, int32_t tries,
				int copyrefs) {
	sqlrcon	sqlrconref=new sqlrconnection(server,port,socket,
					user,password,retrytime,tries,
					(copyrefs!=0));
	return sqlrconref;
}

int sqlrcon_isYes(const char *string) {
	return sqlrconnection::isYes(string);
}

int sqlrcon_isNo(const char *string) {
	return sqlrconnection::isNo(string);
}

void sqlrcon_free(sqlrcon sqlrconref) {
	delete (sqlrconnection *)sqlrconref;
}

void sqlrcon_setConnectTimeout(sqlrcon sqlrconref,
			int32_t timeoutsec, int32_t timeoutusec) {
	sqlrconref->setConnectTimeout(timeoutsec,timeoutusec);
}

void sqlrcon_getConnectTimeout(sqlrcon sqlrconref,
			int32_t *timeoutsec, int32_t *timeoutusec) {
	sqlrconref->getConnectTimeout(timeoutsec,timeoutusec);
}

int32_t sqlrcon_getConnectTimeoutSeconds(sqlrcon sqlrconref) {
	return sqlrconref->getConnectTimeoutSeconds();
}

int32_t sqlrcon_getConnectTimeoutMicroseconds(sqlrcon sqlrconref) {
	return sqlrconref->getConnectTimeoutMicroseconds();
}

void sqlrcon_setResponseTimeout(sqlrcon sqlrconref,
			int32_t timeoutsec, int32_t timeoutusec) {
	sqlrconref->setResponseTimeout(timeoutsec,timeoutusec);
}

void sqlrcon_getResponseTimeout(sqlrcon sqlrconref,
			int32_t *timeoutsec, int32_t *timeoutusec) {
	sqlrconref->getResponseTimeout(timeoutsec,timeoutusec);
}

int32_t sqlrcon_getResponseTimeoutSeconds(sqlrcon sqlrconref) {
	return sqlrconref->getResponseTimeoutSeconds();
}

int32_t sqlrcon_getResponseTimeoutMicroseconds(sqlrcon sqlrconref) {
	return sqlrconref->getResponseTimeoutMicroseconds();
}

void sqlrcon_setBindVariableDelimiters(sqlrcon sqlrconref,
						const char *delimiters) {
	sqlrconref->setBindVariableDelimiters(delimiters);
}

int sqlrcon_getBindVariableDelimiterQuestionMarkSupported(
						sqlrcon sqlrconref) {
	return sqlrconref->getBindVariableDelimiterQuestionMarkSupported();
}

int sqlrcon_getBindVariableDelimiterColonSupported(
						sqlrcon sqlrconref) {
	return sqlrconref->getBindVariableDelimiterColonSupported();
}

int sqlrcon_getBindVariableDelimiterAtSignSupported(
						sqlrcon sqlrconref) {
	return sqlrconref->getBindVariableDelimiterAtSignSupported();
}

int sqlrcon_getBindVariableDelimiterDollarSignSupported(
						sqlrcon sqlrconref) {
	return sqlrconref->getBindVariableDelimiterDollarSignSupported();
}

void sqlrcon_enableKerberos(sqlrcon sqlrconref,
					const char *service,
					const char *mech,
					const char *flags) {
	sqlrconref->enableKerberos(service,mech,flags);
}

void sqlrcon_enableTls(sqlrcon sqlrconref,
				const char *version,
				const char *cert,
				const char *password,
				const char *ciphers,
				const char *validate,
				const char *ca,
				uint16_t depth) {
	sqlrconref->enableTls(version,cert,password,ciphers,validate,ca,depth);
}

void sqlrcon_disableEncryption(sqlrcon sqlrconref) {
	sqlrconref->disableEncryption();
}


void sqlrcon_endSession(sqlrcon sqlrconref) {
	sqlrconref->endSession();
}

int sqlrcon_suspendSession(sqlrcon sqlrconref) {
	return sqlrconref->suspendSession();
}

uint16_t sqlrcon_getConnectionPort(sqlrcon sqlrconref) {
	return sqlrconref->getConnectionPort();
}

const char *sqlrcon_getConnectionSocket(sqlrcon sqlrconref) {
	return sqlrconref->getConnectionSocket();
}

int sqlrcon_resumeSession(sqlrcon sqlrconref, uint16_t port,
					const char *socket) {
	return sqlrconref->resumeSession(port,socket);
}

int sqlrcon_ping(sqlrcon sqlrconref) {
	return sqlrconref->ping();
}

const char *sqlrcon_identify(sqlrcon sqlrconref) {
	return sqlrconref->identify();
}

const char *sqlrcon_dbVersion(sqlrcon sqlrconref) {
	return sqlrconref->dbVersion();
}

const char *sqlrcon_dbHostName(sqlrcon sqlrconref) {
	return sqlrconref->dbHostName();
}

const char *sqlrcon_dbIpAddress(sqlrcon sqlrconref) {
	return sqlrconref->dbIpAddress();
}

const char *sqlrcon_serverVersion(sqlrcon sqlrconref) {
	return sqlrconref->serverVersion();
}

const char *sqlrcon_clientVersion(sqlrcon sqlrconref) {
	return sqlrconref->clientVersion();
}

const char *sqlrcon_bindFormat(sqlrcon sqlrconref) {
	return sqlrconref->bindFormat();
}

const char *sqlrcon_nextvalFormat(sqlrcon sqlrconref) {
	return sqlrconref->nextvalFormat();
}

int sqlrcon_selectDatabase(sqlrcon sqlrconref, const char *database) {
	return sqlrconref->selectDatabase(database);
}

const char *sqlrcon_getCurrentDatabase(sqlrcon sqlrconref) {
	return sqlrconref->getCurrentDatabase();
}


int sqlrcon_getDatabaseIsSchema(sqlrcon sqlrconref) {
	return sqlrconref->getDatabaseIsSchema();
}

int sqlrcon_selectCatalog(sqlrcon sqlrconref, const char *catalog) {
	return sqlrconref->selectCatalog(catalog);
}

const char *sqlrcon_getCurrentCatalog(sqlrcon sqlrconref) {
	return sqlrconref->getCurrentCatalog();
}

int sqlrcon_selectSchema(sqlrcon sqlrconref, const char *schema) {
	return sqlrconref->selectSchema(schema);
}

const char *sqlrcon_getCurrentSchema(sqlrcon sqlrconref) {
	return sqlrconref->getCurrentSchema();
}
const char *sqlrcon_getCurrentUser(sqlrcon sqlrconref) {
	return sqlrconref->getCurrentUser();
}

uint64_t sqlrcon_getLastInsertId(sqlrcon sqlrconref) {
	return sqlrconref->getLastInsertId();
}

int sqlrcon_autoCommitOn(sqlrcon sqlrconref) {
	return sqlrconref->autoCommitOn();
}

int sqlrcon_autoCommitOff(sqlrcon sqlrconref) {
	return sqlrconref->autoCommitOff();
}

int sqlrcon_getAutoCommit(sqlrcon sqlrconref) {
	return sqlrconref->getAutoCommit();
}

int sqlrcon_begin(sqlrcon sqlrconref) {
	return sqlrconref->begin();
}

int sqlrcon_commit(sqlrcon sqlrconref) {
	return sqlrconref->commit();
}

int sqlrcon_rollback(sqlrcon sqlrconref) {
	return sqlrconref->rollback();
}

const char *sqlrcon_getDefaultTransactionModel(sqlrcon sqlrconref) {
	return sqlrconref->getDefaultTransactionModel();
}

int sqlrcon_setTransactionModel(sqlrcon sqlrconref, const char *txmodel) {
	return sqlrconref->setTransactionModel(txmodel);
}

const char *sqlrcon_getTransactionModel(sqlrcon sqlrconref) {
	return sqlrconref->getTransactionModel();
}

const char *sqlrcon_getDefaultIsolationLevel(sqlrcon sqlrconref) {
	return sqlrconref->getDefaultIsolationLevel();
}

int sqlrcon_setIsolationLevel(sqlrcon sqlrconref, const char *isolationlevel) {
	return sqlrconref->setIsolationLevel(isolationlevel);
}

const char *sqlrcon_getIsolationLevel(sqlrcon sqlrconref) {
	return sqlrconref->getIsolationLevel();
}

const char *sqlrcon_getDatabaseFeature(sqlrcon sqlrconref,
						const char *feature) {
	return sqlrconref->getDatabaseFeature(feature);
}

const char *sqlrcon_errorMessage(sqlrcon sqlrconref) {
	return sqlrconref->errorMessage();
}

int64_t sqlrcon_errorNumber(sqlrcon sqlrconref) {
	return sqlrconref->errorNumber();
}

void sqlrcon_debugOn(sqlrcon sqlrconref) {
	sqlrconref->debugOn();
}

void sqlrcon_debugOff(sqlrcon sqlrconref) {
	sqlrconref->debugOff();
}

int sqlrcon_getDebug(sqlrcon sqlrconref) {
	return sqlrconref->getDebug();
}

void sqlrcon_debugPrintFunction(sqlrcon sqlrconref,
				int (*printfunction)(const char *,...)) {
	sqlrconref->debugPrintFunction(printfunction);
}

void sqlrcon_setDebugFile(sqlrcon sqlrconref, const char *filename) {
	sqlrconref->setDebugFile(filename);
}

void sqlrcon_setClientInfo(sqlrcon sqlrconref, const char *clientinfo) {
	sqlrconref->setClientInfo(clientinfo);
}

const char *sqlrcon_getClientInfo(sqlrcon sqlrconref) {
	return sqlrconref->getClientInfo();
}


sqlrcur sqlrcur_alloc(sqlrcon sqlrconref) {
	return sqlrcur_alloc_copyrefs(sqlrconref,0);
}

sqlrcur sqlrcur_alloc_copyrefs(sqlrcon sqlrconref, int copyreferences) {
	sqlrcur	sqlrcurref=new sqlrcursor(sqlrconref,(copyreferences!=0));
	return sqlrcurref;
}

void sqlrcur_free(sqlrcur sqlrcurref) {
	delete (sqlrcur )sqlrcurref;
}

void sqlrcur_setResultSetBufferSize(sqlrcur sqlrcurref, uint64_t rows) {
	sqlrcurref->setResultSetBufferSize(rows);
}

uint64_t sqlrcur_getResultSetBufferSize(sqlrcur sqlrcurref) {
	return sqlrcurref->getResultSetBufferSize();
}

void sqlrcur_dontGetColumnInfo(sqlrcur sqlrcurref) {
	sqlrcurref->dontGetColumnInfo();
}

void sqlrcur_getColumnInfo(sqlrcur sqlrcurref) {
	sqlrcurref->getColumnInfo();
}

void sqlrcur_mixedCaseColumnNames(sqlrcur sqlrcurref) {
	sqlrcurref->mixedCaseColumnNames();
}

void sqlrcur_upperCaseColumnNames(sqlrcur sqlrcurref) {
	sqlrcurref->upperCaseColumnNames();
}

void sqlrcur_lowerCaseColumnNames(sqlrcur sqlrcurref) {
	sqlrcurref->lowerCaseColumnNames();
}

void sqlrcur_cacheToFile(sqlrcur sqlrcurref, const char *filename) {
	sqlrcurref->cacheToFile(filename);
}

void sqlrcur_setCacheTtl(sqlrcur sqlrcurref, uint32_t ttl) {
	sqlrcurref->setCacheTtl(ttl);
}

const char *sqlrcur_getCacheFileName(sqlrcur sqlrcurref) {
	return sqlrcurref->getCacheFileName();
}

void sqlrcur_cacheOff(sqlrcur sqlrcurref) {
	sqlrcurref->cacheOff();
}

int sqlrcur_getDatabaseList(sqlrcur sqlrcurref, const char *databases) {
	return sqlrcurref->getDatabaseList(databases);
}

int sqlrcur_getCatalogList(sqlrcur sqlrcurref, const char *catalogs) {
	return sqlrcurref->getCatalogList(catalogs);
}

int sqlrcur_getSchemaList(sqlrcur sqlrcurref, const char *schemas) {
	return sqlrcurref->getSchemaList(schemas);
}

int sqlrcur_getTableTypeList(sqlrcur sqlrcurref) {
	return sqlrcurref->getTableTypeList();
}

int sqlrcur_getTableList(sqlrcur sqlrcurref, const char *tables) {
	return sqlrcurref->getTableList(tables);
}

int sqlrcur_getTypeInfoList(sqlrcur sqlrcurref, const char *type) {
	return sqlrcurref->getTypeInfoList(type);
}

int sqlrcur_getColumnList(sqlrcur sqlrcurref,
				const char *table, const char *columns) {
	return sqlrcurref->getColumnList(table,columns);
}

int sqlrcur_getPrimaryKeysList(sqlrcur sqlrcurref,
				const char *table, const char *columns) {
	return sqlrcurref->getPrimaryKeysList(table,columns);
}

int sqlrcur_getKeyAndIndexList(sqlrcur sqlrcurref,
				const char *table, const char *qualifier) {
	return sqlrcurref->getKeyAndIndexList(table,qualifier);
}

int sqlrcur_getProcedureList(sqlrcur sqlrcurref, const char *procedures) {
	return sqlrcurref->getProcedureList(procedures);
}

int sqlrcur_getProcedureParameterList(sqlrcur sqlrcurref,
				const char *procedure,
				const char *parameters) {
	return sqlrcurref->getProcedureParameterList(procedure,parameters);
}

int sqlrcur_sendQuery(sqlrcur sqlrcurref, const char *query) {
	return sqlrcurref->sendQuery(query);
}

int sqlrcur_sendQueryWithLength(sqlrcur sqlrcurref, const char *query,
							uint32_t length) {
	return sqlrcurref->sendQuery(query,length);
}

int sqlrcur_sendFileQuery(sqlrcur sqlrcurref, const char *path,
							const char *filename) {
	return sqlrcurref->sendFileQuery(path,filename);
}

void sqlrcur_prepareQuery(sqlrcur sqlrcurref, const char *query) {
	sqlrcurref->prepareQuery(query);
}

void sqlrcur_prepareQueryWithLength(sqlrcur sqlrcurref, const char *query,
							uint32_t length) {
	sqlrcurref->prepareQuery(query,length);
}

void sqlrcur_prepareFileQuery(sqlrcur sqlrcurref, const char *path,
							const char *filename) {
	sqlrcurref->prepareFileQuery(path,filename);
}

void sqlrcur_subString(sqlrcur sqlrcurref, const char *variable,
							const char *value) {
	sqlrcurref->substitution(variable,value);
}

void sqlrcur_subLong(sqlrcur sqlrcurref, const char *variable, int64_t value) {
	sqlrcurref->substitution(variable,value);
}

void sqlrcur_subDouble(sqlrcur sqlrcurref, const char *variable,
			double value, uint32_t precision, uint32_t scale) {
	sqlrcurref->substitution(variable,value,precision,scale);
}

void sqlrcur_clearBinds(sqlrcur sqlrcurref) {
	sqlrcurref->clearBinds();
}

uint16_t sqlrcur_countBindVariables(sqlrcur sqlrcurref) {
	return sqlrcurref->countBindVariables();
}

void sqlrcur_inputBindString(sqlrcur sqlrcurref, const char *variable,
							const char *value) {
	sqlrcurref->inputBind(variable,value);
}

void sqlrcur_inputBindStringWithLength(sqlrcur sqlrcurref,
						const char *variable,
						const char *value,
						uint32_t valuelength) {
	sqlrcurref->inputBind(variable,value,valuelength);
}

void sqlrcur_inputBindLong(sqlrcur sqlrcurref, const char *variable, 
							int64_t value) {
	sqlrcurref->inputBind(variable,value);
}

void sqlrcur_inputBindDouble(sqlrcur sqlrcurref, const char *variable, 
					double value,
					uint32_t precision, 
					uint32_t scale) {
	sqlrcurref->inputBind(variable,value,precision,scale);
}

void sqlrcur_inputBindDate(sqlrcur sqlrcurref, const char *variable,
				int16_t year, int16_t month, int16_t day,
				int16_t hour, int16_t minute, int16_t second,
				int32_t microsecond, const char *tz,
				int isnegative) {
	sqlrcurref->inputBind(variable,year,month,day,
				hour,minute,second,microsecond,tz,
				isnegative);
}

void sqlrcur_inputBindBlob(sqlrcur sqlrcurref, const char *variable,
					const char *value, uint32_t size) {
	sqlrcurref->inputBindBlob(variable,value,size);
}

void sqlrcur_inputBindClob(sqlrcur sqlrcurref, const char *variable,
					const char *value, uint32_t size) {
	sqlrcurref->inputBindClob(variable,value,size);
}

void sqlrcur_subStrings(sqlrcur sqlrcurref,
				const char **variables, const char **values) {
	sqlrcurref->substitutions(variables,values);
}

void sqlrcur_subLongs(sqlrcur sqlrcurref, const char **variables,
						const int64_t *values) {
	sqlrcurref->substitutions(variables,values);
}

void sqlrcur_subDoubles(sqlrcur sqlrcurref,
				const char **variables,
				const double *values,
				const uint32_t *precisions,
				const uint32_t *scales) {
	sqlrcurref->substitutions(variables,values,precisions,scales);
}

void sqlrcur_inputBindStrings(sqlrcur sqlrcurref, const char **variables, 
							const char **values) {
	sqlrcurref->inputBinds(variables,values);
}

void sqlrcur_inputBindLongs(sqlrcur sqlrcurref, const char **variables, 
						const int64_t *values) {
	sqlrcurref->inputBinds(variables,values);
}

void sqlrcur_inputBindDoubles(sqlrcur sqlrcurref, 
					const char **variables,
					const double *values,
					const uint32_t *precisions, 
					const uint32_t *scales) {
	sqlrcurref->inputBinds(variables,values,precisions,scales);
}

void sqlrcur_validateBinds(sqlrcur sqlrcurref) {
	sqlrcurref->validateBinds();
}

int sqlrcur_validBind(sqlrcur sqlrcurref, const char *variable) {
	return sqlrcurref->validBind(variable);
}

int sqlrcur_executeQuery(sqlrcur sqlrcurref) {
	return sqlrcurref->executeQuery();
}

int sqlrcur_fetchFromBindCursor(sqlrcur sqlrcurref) {
	return sqlrcurref->fetchFromBindCursor();
}

void sqlrcur_defineOutputBindString(sqlrcur sqlrcurref,
					const char *variable, uint32_t length) {
	sqlrcurref->defineOutputBindString(variable,length);
}

void sqlrcur_defineOutputBindInteger(sqlrcur sqlrcurref, const char *variable) {
	sqlrcurref->defineOutputBindInteger(variable);
}

void sqlrcur_defineOutputBindDouble(sqlrcur sqlrcurref, const char *variable) {
	sqlrcurref->defineOutputBindDouble(variable);
}

void sqlrcur_defineOutputBindDate(sqlrcur sqlrcurref, const char *variable) {
	sqlrcurref->defineOutputBindDate(variable);
}

void sqlrcur_defineOutputBindBlob(sqlrcur sqlrcurref,
						const char *variable) {
	sqlrcurref->defineOutputBindBlob(variable);
}

void sqlrcur_defineOutputBindClob(sqlrcur sqlrcurref,
						const char *variable) {
	sqlrcurref->defineOutputBindClob(variable);
}

void sqlrcur_defineOutputBindCursor(sqlrcur sqlrcurref,
						const char *variable) {
	sqlrcurref->defineOutputBindCursor(variable);
}

const char *sqlrcur_getOutputBindString(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindString(variable);
}

const char *sqlrcur_getOutputBindBlob(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindBlob(variable);
}

const char *sqlrcur_getOutputBindClob(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindClob(variable);
}

int64_t sqlrcur_getOutputBindInteger(sqlrcur sqlrcurref,
						const char *variable) {
	return sqlrcurref->getOutputBindInteger(variable);
}

double sqlrcur_getOutputBindDouble(sqlrcur sqlrcurref,
						const char *variable) {
	return sqlrcurref->getOutputBindDouble(variable);
}

int sqlrcur_getOutputBindDate(sqlrcur sqlrcurref, const char *variable,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute, int16_t *second,
				int32_t *microsecond, const char **tz,
				int *isnegative) {
	bool	isneg;
	int	retval=sqlrcurref->getOutputBindDate(variable,year,month,day,
					hour,minute,second,microsecond,tz,
					&isneg);
	*isnegative=isneg;
	return retval;
}

int16_t sqlrcur_getOutputBindDateYear(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateYear(variable);
}

int16_t sqlrcur_getOutputBindDateMonth(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateMonth(variable);
}

int16_t sqlrcur_getOutputBindDateDay(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateDay(variable);
}

int16_t sqlrcur_getOutputBindDateHour(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateHour(variable);
}

int16_t sqlrcur_getOutputBindDateMinute(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateMinute(variable);
}

int16_t sqlrcur_getOutputBindDateSecond(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateSecond(variable);
}

int32_t sqlrcur_getOutputBindDateMicrosecond(sqlrcur sqlrcurref,
						const char *variable) {
	return sqlrcurref->getOutputBindDateMicrosecond(variable);
}

const char *sqlrcur_getOutputBindDateTz(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateTz(variable);
}

int sqlrcur_getOutputBindDateIsNegative(sqlrcur sqlrcurref,
					const char *variable) {
	return sqlrcurref->getOutputBindDateIsNegative(variable);
}

uint32_t sqlrcur_getOutputBindLength(sqlrcur sqlrcurref, const char *variable) {
	return sqlrcurref->getOutputBindLength(variable);
}

sqlrcur sqlrcur_getOutputBindCursor(sqlrcur sqlrcurref, const char *variable) {
	return sqlrcurref->getOutputBindCursor(variable);
}

sqlrcur sqlrcur_getOutputBindCursor_copyrefs(sqlrcur sqlrcurref,
					const char *variable, int copyrefs) {
	return sqlrcurref->getOutputBindCursor(variable,copyrefs);
}

int sqlrcur_openCachedResultSet(sqlrcur sqlrcurref, const char *filename) {
	return sqlrcurref->openCachedResultSet(filename);
}

uint64_t sqlrcur_rowCount(sqlrcur sqlrcurref) {
	return sqlrcurref->rowCount();
}

uint32_t sqlrcur_colCount(sqlrcur sqlrcurref) {
	return sqlrcurref->colCount();
}

uint64_t sqlrcur_totalRows(sqlrcur sqlrcurref) {
	return sqlrcurref->totalRows();
}

uint64_t sqlrcur_affectedRows(sqlrcur sqlrcurref) {
	return sqlrcurref->affectedRows();
}

uint64_t sqlrcur_firstRowIndex(sqlrcur sqlrcurref) {
	return sqlrcurref->firstRowIndex();
}

int sqlrcur_endOfResultSet(sqlrcur sqlrcurref) {
	return sqlrcurref->endOfResultSet();
}

int sqlrcur_nextResultSet(sqlrcur sqlrcurref) {
	return sqlrcurref->nextResultSet();
}

const char *sqlrcur_errorMessage(sqlrcur sqlrcurref) {
	return sqlrcurref->errorMessage();
}

int64_t sqlrcur_errorNumber(sqlrcur sqlrcurref) {
	return sqlrcurref->errorNumber();
}

void sqlrcur_getNullsAsEmptyStrings(sqlrcur sqlrcurref) {
	sqlrcurref->getNullsAsEmptyStrings();
}

void sqlrcur_getNullsAsNulls(sqlrcur sqlrcurref) {
	sqlrcurref->getNullsAsNulls();
}

const char *sqlrcur_getFieldByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getField(row,col);
}

const char *sqlrcur_getFieldByName(sqlrcur sqlrcurref, uint64_t row,
							const char *col) {
	return sqlrcurref->getField(row,col);
}

const char *sqlrcur_getFieldByNameIgnoringCase(sqlrcur sqlrcurref,
						uint64_t row,
						const char *col) {
	return sqlrcurref->getFieldIgnoringCase(row,col);
}

int64_t sqlrcur_getFieldAsIntegerByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsInteger(row,col);
}

int64_t sqlrcur_getFieldAsIntegerByName(sqlrcur sqlrcurref, uint64_t row,
							const char *col) {
	return sqlrcurref->getFieldAsInteger(row,col);
}

int64_t sqlrcur_getFieldAsIntegerByNameIgnoringCase(sqlrcur sqlrcurref,
						uint64_t row,
						const char *col) {
	return sqlrcurref->getFieldAsIntegerIgnoringCase(row,col);
}

double sqlrcur_getFieldAsDoubleByIndex(sqlrcur sqlrcurref, uint64_t row,
								uint32_t col) {
	return sqlrcurref->getFieldAsDouble(row,col);
}

double sqlrcur_getFieldAsDoubleByName(sqlrcur sqlrcurref, uint64_t row,
							const char *col) {
	return sqlrcurref->getFieldAsDouble(row,col);
}

double sqlrcur_getFieldAsDoubleByNameIgnoringCase(sqlrcur sqlrcurref,
						uint64_t row,
						const char *col) {
	return sqlrcurref->getFieldAsDoubleIgnoringCase(row,col);
}

int sqlrcur_getFieldAsBooleanByIndex(sqlrcur sqlrcurref, uint64_t row,
							uint32_t col) {
	return sqlrcurref->getFieldAsBoolean(row,col);
}

int sqlrcur_getFieldAsBooleanByName(sqlrcur sqlrcurref, uint64_t row,
							const char *col) {
	return sqlrcurref->getFieldAsBoolean(row,col);
}

int sqlrcur_getFieldAsBooleanByNameIgnoringCase(sqlrcur sqlrcurref,
						uint64_t row,
						const char *col) {
	return sqlrcurref->getFieldAsBooleanIgnoringCase(row,col);
}

int sqlrcur_getFieldAsDateByIndex(sqlrcur sqlrcurref,
				uint64_t row, uint32_t col,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute,
				int16_t *second,
				int32_t *microsecond, int *isnegative) {
	bool	isneg;
	bool	retval=sqlrcurref->getFieldAsDate(row,col,
					year,month,day,
					hour,minute,second,
					microsecond,&isneg);
	*isnegative=(isneg)?1:0;
	return (retval)?1:0;
}

int sqlrcur_getFieldAsDateByIndexWithDdMm(sqlrcur sqlrcurref,
				uint64_t row, uint32_t col,
				int ddmm, int yyyyddmm,
				const char *datedelimiters,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute,
				int16_t *second,
				int32_t *microsecond, int *isnegative) {
	bool	isneg;
	bool	retval=sqlrcurref->getFieldAsDate(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters,
					year,month,day,
					hour,minute,second,
					microsecond,&isneg);
	*isnegative=(isneg)?1:0;
	return (retval)?1:0;
}

int sqlrcur_getFieldAsDateByName(sqlrcur sqlrcurref,
				uint64_t row, const char *col,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute,
				int16_t *second,
				int32_t *microsecond, int *isnegative) {
	bool	isneg;
	bool	retval=sqlrcurref->getFieldAsDate(row,col,
					year,month,day,
					hour,minute,second,
					microsecond,&isneg);
	*isnegative=(isneg)?1:0;
	return (retval)?1:0;
}

int sqlrcur_getFieldAsDateByNameIgnoringCase(sqlrcur sqlrcurref,
				uint64_t row, const char *col,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute,
				int16_t *second,
				int32_t *microsecond, int *isnegative) {
	bool	isneg;
	bool	retval=sqlrcurref->getFieldAsDateIgnoringCase(row,col,
					year,month,day,
					hour,minute,second,
					microsecond,&isneg);
	*isnegative=(isneg)?1:0;
	return (retval)?1:0;
}

int sqlrcur_getFieldAsDateByNameWithDdMm(sqlrcur sqlrcurref,
				uint64_t row, const char *col,
				int ddmm, int yyyyddmm,
				const char *datedelimiters,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute,
				int16_t *second,
				int32_t *microsecond, int *isnegative) {
	bool	isneg;
	bool	retval=sqlrcurref->getFieldAsDate(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters,
					year,month,day,
					hour,minute,second,
					microsecond,&isneg);
	*isnegative=(isneg)?1:0;
	return (retval)?1:0;
}

int sqlrcur_getFieldAsDateByNameWithDdMmIgnoringCase(sqlrcur sqlrcurref,
				uint64_t row, const char *col,
				int ddmm, int yyyyddmm,
				const char *datedelimiters,
				int16_t *year, int16_t *month, int16_t *day,
				int16_t *hour, int16_t *minute,
				int16_t *second,
				int32_t *microsecond, int *isnegative) {
	bool	isneg;
	bool	retval=sqlrcurref->getFieldAsDateIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters,
					year,month,day,
					hour,minute,second,
					microsecond,&isneg);
	*isnegative=(isneg)?1:0;
	return (retval)?1:0;
}

int16_t sqlrcur_getFieldAsDateYearByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsDateYear(row,col);
}

int16_t sqlrcur_getFieldAsDateYearByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateYear(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateYearByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateYear(row,col);
}

int16_t sqlrcur_getFieldAsDateYearByNameIgnoringCase(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateYearIgnoringCase(row,col);
}

int16_t sqlrcur_getFieldAsDateYearByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateYear(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateYearByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateYearIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateMonthByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsDateMonth(row,col);
}

int16_t sqlrcur_getFieldAsDateMonthByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMonth(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateMonthByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateMonth(row,col);
}

int16_t sqlrcur_getFieldAsDateMonthByNameIgnoringCase(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateMonthIgnoringCase(row,col);
}

int16_t sqlrcur_getFieldAsDateMonthByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMonth(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateMonthByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMonthIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateDayByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsDateDay(row,col);
}

int16_t sqlrcur_getFieldAsDateDayByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateDay(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateDayByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateDay(row,col);
}

int16_t sqlrcur_getFieldAsDateDayByNameIgnoringCase(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateDayIgnoringCase(row,col);
}

int16_t sqlrcur_getFieldAsDateDayByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateDay(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateDayByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateDayIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateHourByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsDateHour(row,col);
}

int16_t sqlrcur_getFieldAsDateHourByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateHour(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateHourByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateHour(row,col);
}

int16_t sqlrcur_getFieldAsDateHourByNameIgnoringCase(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateHourIgnoringCase(row,col);
}

int16_t sqlrcur_getFieldAsDateHourByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateHour(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateHourByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateHourIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateMinuteByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsDateMinute(row,col);
}

int16_t sqlrcur_getFieldAsDateMinuteByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMinute(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateMinuteByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateMinute(row,col);
}

int16_t sqlrcur_getFieldAsDateMinuteByNameIgnoringCase(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateMinuteIgnoringCase(row,col);
}

int16_t sqlrcur_getFieldAsDateMinuteByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMinute(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateMinuteByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMinuteIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateSecondByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsDateSecond(row,col);
}

int16_t sqlrcur_getFieldAsDateSecondByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateSecond(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateSecondByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateSecond(row,col);
}

int16_t sqlrcur_getFieldAsDateSecondByNameIgnoringCase(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateSecondIgnoringCase(row,col);
}

int16_t sqlrcur_getFieldAsDateSecondByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateSecond(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int16_t sqlrcur_getFieldAsDateSecondByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateSecondIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int32_t sqlrcur_getFieldAsDateMicrosecondByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldAsDateMicrosecond(row,col);
}

int32_t sqlrcur_getFieldAsDateMicrosecondByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMicrosecond(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int32_t sqlrcur_getFieldAsDateMicrosecondByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateMicrosecond(row,col);
}

int32_t sqlrcur_getFieldAsDateMicrosecondByNameIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return sqlrcurref->getFieldAsDateMicrosecondIgnoringCase(row,col);
}

int32_t sqlrcur_getFieldAsDateMicrosecondByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMicrosecond(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int32_t sqlrcur_getFieldAsDateMicrosecondByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return sqlrcurref->getFieldAsDateMicrosecondIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters);
}

int sqlrcur_getFieldAsDateIsNegativeByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return (sqlrcurref->getFieldAsDateIsNegative(row,col))?1:0;
}

int sqlrcur_getFieldAsDateIsNegativeByIndexWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return (sqlrcurref->getFieldAsDateIsNegative(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters))?1:0;
}

int sqlrcur_getFieldAsDateIsNegativeByName(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return (sqlrcurref->getFieldAsDateIsNegative(row,col))?1:0;
}

int sqlrcur_getFieldAsDateIsNegativeByNameIgnoringCase(sqlrcur sqlrcurref,
					uint64_t row, const char *col) {
	return (sqlrcurref->getFieldAsDateIsNegativeIgnoringCase(row,col))?1:0;
}

int sqlrcur_getFieldAsDateIsNegativeByNameWithDdMm(sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return (sqlrcurref->getFieldAsDateIsNegative(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters))?1:0;
}

int sqlrcur_getFieldAsDateIsNegativeByNameWithDdMmIgnoringCase(
					sqlrcur sqlrcurref,
					uint64_t row, const char *col,
					int ddmm, int yyyyddmm,
					const char *datedelimiters) {
	return (sqlrcurref->getFieldAsDateIsNegativeIgnoringCase(row,col,
					ddmm!=0,yyyyddmm!=0,
					datedelimiters))?1:0;
}

uint32_t sqlrcur_getFieldLengthByIndex(sqlrcur sqlrcurref,
					uint64_t row, uint32_t col) {
	return sqlrcurref->getFieldLength(row,col);
}

uint32_t sqlrcur_getFieldLengthByName(sqlrcur sqlrcurref,
						uint64_t row, const char *col) {
	return sqlrcurref->getFieldLength(row,col);
}

const char * const *sqlrcur_getRow(sqlrcur sqlrcurref, uint64_t row) {
	return sqlrcurref->getRow(row);
}

uint32_t *sqlrcur_getRowLengths(sqlrcur sqlrcurref, uint64_t row) {
	return sqlrcurref->getRowLengths(row);
}

const char * const *sqlrcur_getColumnNames(sqlrcur sqlrcurref) {
	return sqlrcurref->getColumnNames();
}

const char *sqlrcur_getColumnName(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnName(col);
}

const char *sqlrcur_getColumnTypeByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnType(col);
}

uint32_t sqlrcur_getColumnLengthByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnLength(col);
}

const char *sqlrcur_getColumnTypeByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnType(col);
}

uint32_t sqlrcur_getColumnLengthByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnLength(col);
}

uint32_t sqlrcur_getColumnPrecisionByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnPrecision(col);
}

uint32_t sqlrcur_getColumnPrecisionByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnPrecision(col);
}

uint32_t sqlrcur_getColumnScaleByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnScale(col);
}

uint32_t sqlrcur_getColumnScaleByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnScale(col);
}

int sqlrcur_getColumnIsNullableByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsNullable(col);
}

int sqlrcur_getColumnIsNullableByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnIsNullable(col);
}

int sqlrcur_getColumnIsPrimaryKeyByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsPrimaryKey(col);
}

int sqlrcur_getColumnIsPrimaryKeyByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnIsPrimaryKey(col);
}

int sqlrcur_getColumnIsUniqueByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsUnique(col);
}

int sqlrcur_getColumnIsUniqueByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnIsUnique(col);
}

int sqlrcur_getColumnIsPartOfKeyByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsPartOfKey(col);
}

int sqlrcur_getColumnIsPartOfKeyByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnIsPartOfKey(col);
}

int sqlrcur_getColumnIsUnsignedByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsUnsigned(col);
}

int sqlrcur_getColumnIsUnsignedByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnIsUnsigned(col);
}

int sqlrcur_getColumnIsZeroFilledByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsZeroFilled(col);
}

int sqlrcur_getColumnIsZeroFilledByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnIsZeroFilled(col);
}

int sqlrcur_getColumnIsBinaryByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsBinary(col);
}

int sqlrcur_getColumnIsBinaryByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getColumnIsBinary(col);
}

int sqlrcur_getColumnIsAutoIncrementByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getColumnIsAutoIncrement(col);
}

int sqlrcur_getColumnIsAutoIncrementByName(sqlrcur sqlrcurref,
							const char *col) {
	return sqlrcurref->getColumnIsAutoIncrement(col);
}

uint32_t sqlrcur_getLongestByName(sqlrcur sqlrcurref, const char *col) {
	return sqlrcurref->getLongest(col);
}

uint32_t sqlrcur_getLongestByIndex(sqlrcur sqlrcurref, uint32_t col) {
	return sqlrcurref->getLongest(col);
}

uint16_t sqlrcur_getResultSetId(sqlrcur sqlrcurref) {
	return sqlrcurref->getResultSetId();
}

void sqlrcur_suspendResultSet(sqlrcur sqlrcurref) {
	sqlrcurref->suspendResultSet();
}

int sqlrcur_resumeResultSet(sqlrcur sqlrcurref, uint16_t id) {
	return sqlrcurref->resumeResultSet(id);
}

int sqlrcur_resumeCachedResultSet(sqlrcur sqlrcurref,
					uint16_t id, const char *filename) {
	return sqlrcurref->resumeCachedResultSet(id,filename);
}

void sqlrcur_closeResultSet(sqlrcur sqlrcurref) {
	sqlrcurref->closeResultSet();
}

}
