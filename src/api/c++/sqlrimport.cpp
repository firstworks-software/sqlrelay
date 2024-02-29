// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimport.h>

#include <rudiments/file.h>
#include <rudiments/error.h>

sqlrimport::sqlrimport() {
	sqlrcon=NULL;
	sqlrcur=NULL;

	dbtype=NULL;
	objectname=NULL;

	ignorecolumns=false;
	lowercasecolumnnames=false;
	uppercasecolumnnames=false;

	reformatdatetime=false;
	ddmm=false;
	yyyyddmm=false;
	datedelimiters=NULL;
	nocenturythreshold=100;
	lastcenturythreshold=10;
	datetimeformat="YYYY-MM-DD HH24:MI:SS";

	commitcount=0;

	lg=NULL;
	coarseloglevel=0;
	fineloglevel=9;
	logindent=0;
	logerrors=true;

	clearFlagsAndCounts();

	columns.setManageArrayValues(true);
	fields.setManageArrayValues(true);
}

sqlrimport::~sqlrimport() {
	delete[] dbtype;
	delete[] objectname;
}

void sqlrimport::setSqlrConnection(sqlrconnection *sqlrcon) {
	this->sqlrcon=sqlrcon;
}

void sqlrimport::setSqlrCursor(sqlrcursor *sqlrcur) {
	this->sqlrcur=sqlrcur;
}

sqlrconnection *sqlrimport::getSqlrConnection() {
	return sqlrcon;
}

sqlrcursor *sqlrimport::getSqlrCursor() {
	return sqlrcur;
}

void sqlrimport::setDbType(const char *dbtype) {
	delete[] this->dbtype;
	this->dbtype=charstring::duplicate(dbtype);
}

const char *sqlrimport::getDbType() {
	return dbtype;
}

void sqlrimport::setObjectName(const char *objectname) {
	delete[] this->objectname;
	this->objectname=charstring::duplicate(objectname);
}

const char *sqlrimport::getObjectName() {
	return objectname;
}

void sqlrimport::setIgnoreColumns(bool ignorecolumns) {
	this->ignorecolumns=ignorecolumns;
}

bool sqlrimport::getIgnoreColumns() {
	return ignorecolumns;
}

void sqlrimport::mapColumnName(const char *from, const char *to) {
	if (!to) {
		columnmap.remove(from);
	} else {
		columnmap.setValue(from,to);
	}
}

const char *sqlrimport::getMappedColumnName(const char *from) {
	return columnmap.getValue(from);
}

void sqlrimport::setMixedCaseColumnNames() {
	lowercasecolumnnames=false;
	uppercasecolumnnames=false;
}

bool sqlrimport::getMixedCaseColumnNames() {
	return !lowercasecolumnnames && !uppercasecolumnnames;
}

void sqlrimport::setLowerCaseColumnNames() {
	lowercasecolumnnames=true;
	uppercasecolumnnames=false;
}

bool sqlrimport::getLowerCaseColumnNames() {
	return lowercasecolumnnames;
}

void sqlrimport::setUpperCaseColumnNames() {
	lowercasecolumnnames=false;
	uppercasecolumnnames=true;
}

bool sqlrimport::getUpperCaseColumnNames() {
	return uppercasecolumnnames;
}

void sqlrimport::mapFieldValue(const char *from, const char *to) {
	if (!to) {
		fieldmap.remove(from);
	} else {
		fieldmap.setValue(from,to);
	}
}

const char *sqlrimport::getMappedFieldValue(const char *from) {
	return fieldmap.getValue(from);
}

void sqlrimport::setReformatDateTime(bool reformatdatetime) {
	this->reformatdatetime=reformatdatetime;
}

bool sqlrimport::getReformatDateTime() {
	return reformatdatetime;
}

void sqlrimport::setDdMm(bool ddmm) {
	this->ddmm=ddmm;
}

bool sqlrimport::getDdMm() {
	return ddmm;
}

void sqlrimport::setYyyyDdMm(bool yyyyddmm) {
	this->yyyyddmm=yyyyddmm;
}

bool sqlrimport::getYyyyDdMm() {
	return yyyyddmm;
}

void sqlrimport::setDateDelimiters(const char *datedelimiters) {
	this->datedelimiters=datedelimiters;
}

const char *sqlrimport::getDateDelimiters() {
	return datedelimiters;
}

void sqlrimport::setNoCenturyThreshold(uint16_t nocenturythreshold) {
	this->nocenturythreshold=nocenturythreshold;
}

uint16_t sqlrimport::getNoCenturyThreshold() {
	return nocenturythreshold;
}

void sqlrimport::setLastCenturyThreshold(uint16_t lastcenturythreshold) {
	this->lastcenturythreshold=lastcenturythreshold;
}

uint16_t sqlrimport::getLastCenturyThreshold() {
	return lastcenturythreshold;
}

void sqlrimport::setDateTimeFormat(const char *datetimeformat) {
	this->datetimeformat=datetimeformat;
}

const char *sqlrimport::getDateTimeFormat() {
	return datetimeformat;
}

void sqlrimport::setCommitCount(uint64_t commitcount) {
	this->commitcount=commitcount;
}

uint64_t sqlrimport::getCommitCount() {
	return commitcount;
}

void sqlrimport::setLogger(logger *lg) {
	this->lg=lg;
}

logger *sqlrimport::getLogger() {
	return lg;
}

void sqlrimport::setCoarseLogLevel(uint8_t coarseloglevel) {
	this->coarseloglevel=coarseloglevel;
}

uint8_t sqlrimport::getCoarseLogLevel() {
	return coarseloglevel;
}

void sqlrimport::setFineLogLevel(uint8_t fineloglevel) {
	this->fineloglevel=fineloglevel;
}

uint8_t sqlrimport::getFineLogLevel() {
	return fineloglevel;
}

void sqlrimport::setLogIndent(uint32_t logindent) {
	this->logindent=logindent;
}

uint32_t sqlrimport::getLogIndent() {
	return logindent;
}

void sqlrimport::setLogErrors(bool logerrors) {
	this->logerrors=logerrors;
}

bool sqlrimport::getLogErrors() {
	return logerrors;
}

bool sqlrimport::importStart() {
	// by default, just return success
	return true;
}

bool sqlrimport::columnsStart() {
	// by default, just return success
	return true;
}

bool sqlrimport::columnStart() {
	// by default, just return success
	return true;
}

bool sqlrimport::columnEnd() {
	// by default, just return success
	return true;
}

bool sqlrimport::columnsEnd() {
	// by default, just return success
	return true;
}

bool sqlrimport::rowsStart() {
	return true;
}

bool sqlrimport::rowStart() {
	// by default, just return success
	return true;
}

bool sqlrimport::fieldStart() {
	// by default, just return success
	return true;
}

bool sqlrimport::fieldEnd() {
	// by default, just return success
	return true;
}

bool sqlrimport::rowEnd() {
	return true;
}

bool sqlrimport::rowsEnd() {
	// by default, just return success
	return true;
}

bool sqlrimport::error(int64_t errornumber, const char *errormessage) {
	// by default, just return error
	return false;
}

bool sqlrimport::importEnd() {
	// by default, just return success
	return true;
}

void sqlrimport::setImportedRowCount(uint64_t importedrowcount) {
	this->importedrowcount=importedrowcount;
}

uint64_t sqlrimport::getImportedRowCount() {
	return importedrowcount;
}

void sqlrimport::setIgnoreRow(bool ignorerow) {
	this->ignorerow=ignorerow;
}

bool sqlrimport::getIgnoreRow() {
	return ignorerow;
}

void sqlrimport::setCurrentRow(uint64_t currentrow) {
	this->currentrow=currentrow;
}

uint64_t sqlrimport::getCurrentRow() {
	return currentrow;
}

void sqlrimport::setCurrentColumn(uint32_t currentcol) {
	this->currentcol=currentcol;
}

uint32_t sqlrimport::getCurrentColumn() {
	return currentcol;
}

void sqlrimport::setCurrentColumnName(char *currentcolname) {
	this->currentcolname=currentcolname;
}

char *sqlrimport::getCurrentColumnName() {
	return currentcolname;
}

void sqlrimport::setCurrentField(char *currentfield) {
	this->currentfield=currentfield;
}

char *sqlrimport::getCurrentField() {
	return currentfield;
}

void sqlrimport::setIsNumericColumn(uint64_t index, bool value) {
	numericcolumn[index]=value;
}

bool sqlrimport::getIsNumericColumn(uint64_t index) {
	return numericcolumn[index];
}

void sqlrimport::clearAreNumericColumns() {
	numericcolumn.clear();
}

void sqlrimport::setIsDateTimeColumn(uint64_t index, bool value) {
	datetimecolumn[index]=value;
}

bool sqlrimport::getIsDateTimeColumn(uint64_t index) {
	return datetimecolumn[index];
}

void sqlrimport::clearAreDateTimeColumns() {
	datetimecolumn.clear();
}

bool sqlrimport::importData() {
	return true;
}

void sqlrimport::clearFlagsAndCounts() {
	ignorerow=false;
	currentrow=0;
	currentcol=0;
	currentcolname=NULL;
	currentfield=NULL;
	importedrowcount=0;
	numericcolumn.clear();
	datetimecolumn.clear();
}

bool sqlrimport::systemError() {
	char	*err=error::getErrorString();
	bool	retval=error(error::getErrorNumber(),err);
	delete[] err;
	return retval;
}
