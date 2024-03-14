// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexport.h>

#include <rudiments/error.h>

sqlrexport::sqlrexport() {

	sqlrcon=NULL;
	sqlrcur=NULL;

	table=NULL;

	excludecolumns=false;
	columnstoexclude=NULL;

	lg=NULL;
	coarseloglevel=0;
	fineloglevel=9;
	logindent=0;
	logerrors=true;

	clearFlagsAndCounts();
}

sqlrexport::~sqlrexport() {
}

void sqlrexport::setSqlrConnection(sqlrconnection *sqlrcon) {
	this->sqlrcon=sqlrcon;
}

sqlrconnection *sqlrexport::getSqlrConnection() {
	return sqlrcon;
}

void sqlrexport::setSqlrCursor(sqlrcursor *sqlrcur) {
	this->sqlrcur=sqlrcur;
}

sqlrcursor *sqlrexport::getSqlrCursor() {
	return sqlrcur;
}

void sqlrexport::setTable(const char *table) {
	this->table=table;
}

const char *sqlrexport::getTable() {
	return table;
}

void sqlrexport::setExcludeColumns(bool excludecolumns) {
	this->excludecolumns=excludecolumns;
}

bool sqlrexport::getExcludeColumns() {
	return excludecolumns;
}

void sqlrexport::setColumnsToExclude(const char * const *columnstoexclude) {
	this->columnstoexclude=columnstoexclude;
}

const char * const *sqlrexport::getColumnsToExclude() {
	return columnstoexclude;
}

void sqlrexport::setLogger(logger *lg) {
	this->lg=lg;
}

logger *sqlrexport::getLogger() {
	return lg;
}

void sqlrexport::setCoarseLogLevel(uint8_t coarseloglevel) {
	this->coarseloglevel=coarseloglevel;
}

void sqlrexport::setFineLogLevel(uint8_t fineloglevel) {
	this->fineloglevel=fineloglevel;
}

void sqlrexport::setLogIndent(uint32_t logindent) {
	this->logindent=logindent;
}

uint8_t sqlrexport::getCoarseLogLevel() {
	return coarseloglevel;
}

uint8_t sqlrexport::getFineLogLevel() {
	return fineloglevel;
}

uint32_t sqlrexport::getLogIndent() {
	return logindent;
}

void sqlrexport::setLogErrors(bool logerrors) {
	this->logerrors=logerrors;
}

bool sqlrexport::getLogErrors() {
	return logerrors;
}

bool sqlrexport::exportStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::columnsStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::columnStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::columnEnd() {
	// by default, just return success
	return true;
}

bool sqlrexport::columnsEnd() {
	// by default, just return success
	return true;
}

bool sqlrexport::rowsStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::rowStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::fieldStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::fieldEnd() {
	// by default, just return success
	return true;
}

bool sqlrexport::rowEnd() {
	// by default, just return success
	return true;
}

bool sqlrexport::rowsEnd() {
	// by default, just return success
	return true;
}

bool sqlrexport::beginStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::beginEnd() {
	// by default, just return success
	return true;
}

bool sqlrexport::commitStart() {
	// by default, just return success
	return true;
}

bool sqlrexport::commitEnd() {
	// by default, just return success
	return true;
}

bool sqlrexport::error(int64_t errornumber, const char *errormessage) {
	// by default, just return error
	return false;
}

bool sqlrexport::exportEnd() {
	// by default, just return success
	return true;
}

void sqlrexport::setExportedRowCount(uint64_t exportedrowcount) {
	this->exportedrowcount=exportedrowcount;
}

uint64_t sqlrexport::getExportedRowCount() {
	return exportedrowcount;
}

void sqlrexport::setExcludeRow(bool excluderow) {
	this->excluderow=excluderow;
}

bool sqlrexport::getExcludeRow() {
	return excluderow;
}

void sqlrexport::setCurrentRow(uint64_t currentrow) {
	this->currentrow=currentrow;
}

uint64_t sqlrexport::getCurrentRow() {
	return currentrow;
}

void sqlrexport::setCurrentColumn(uint32_t currentcol) {
	this->currentcol=currentcol;
}

uint32_t sqlrexport::getCurrentColumn() {
	return currentcol;
}

void sqlrexport::setCurrentColumnName(const char *currentcolname) {
	this->currentcolname=currentcolname;
}

const char *sqlrexport::getCurrentColumnName() {
	return currentcolname;
}

void sqlrexport::setCurrentField(const char *currentfield) {
	this->currentfield=currentfield;
}

const char *sqlrexport::getCurrentField() {
	return currentfield;
}

void sqlrexport::setIsNumericColumn(uint64_t index, bool value) {
	numericcolumn[index]=value;
}

bool sqlrexport::getIsNumericColumn(uint64_t index) {
	return (index<numericcolumn.getCount())?numericcolumn[index]:false;
}

void sqlrexport::clearAreNumericColumns() {
	numericcolumn.clear();
}

bool sqlrexport::exportData() {
	return true;
}

void sqlrexport::clearFlagsAndCounts() {
	excluderow=false;
	currentrow=0;
	currentcol=0;
	currentcolname=NULL;
	currentfield=NULL;
	exportedrowcount=0;
	numericcolumn.clear();
}

bool sqlrexport::systemError() {
	char	*err=error::getErrorString();
	bool	retval=error(error::getErrorNumber(),err);
	delete[] err;
	return retval;
}
