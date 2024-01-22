// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexport.h>

sqlrexport::sqlrexport() {
	sqlrcon=NULL;
	sqlrcur=NULL;
	ignorecolumns=false;
	columnstoignore=NULL;
	fd=NULL;
	exportrow=true;
	currentrow=0;
	currentcol=0;
	currentcolname=NULL;
	currentfield=NULL;
	lg=NULL;
	coarseloglevel=0;
	fineloglevel=9;
	logindent=0;
	exportedrowcount=0;
}

sqlrexport::~sqlrexport() {
}

void sqlrexport::setSqlrConnection(sqlrconnection *sqlrcon) {
	this->sqlrcon=sqlrcon;
}

void sqlrexport::setSqlrCursor(sqlrcursor *sqlrcur) {
	this->sqlrcur=sqlrcur;
}

sqlrconnection *sqlrexport::getSqlrConnection() {
	return sqlrcon;
}

sqlrcursor *sqlrexport::getSqlrCursor() {
	return sqlrcur;
}

void sqlrexport::setIgnoreColumns(bool ignorecolumns) {
	this->ignorecolumns=ignorecolumns;
}

bool sqlrexport::getIgnoreColumns() {
	return ignorecolumns;
}

void sqlrexport::setColumnsToIgnore(const char * const *columnstoignore) {
	this->columnstoignore=columnstoignore;
}

const char * const *sqlrexport::getColumnsToIgnore() {
	return columnstoignore;
}

void sqlrexport::setLogger(logger *lg) {
	this->lg=lg;
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

logger *sqlrexport::getLogger() {
	return lg;
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
	return true;
}

bool sqlrexport::rowsEnd() {
	// by default, just return success
	return true;
}

void sqlrexport::setExportedRowCount(uint64_t exportedrowcount) {
	this->exportedrowcount=exportedrowcount;
}

uint64_t sqlrexport::getExportedRowCount() {
	return exportedrowcount;
}

void sqlrexport::setExportRow(bool exportrow) {
	this->exportrow=exportrow;
}

bool sqlrexport::getExportRow() {
	return exportrow;
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
	return numericcolumn[index];
}

void sqlrexport::clearAreNumericColumns() {
	numericcolumn.clear();
}

void sqlrexport::setFileDescriptor(filedescriptor *fd) {
	this->fd=fd;
}

filedescriptor *sqlrexport::getFileDescriptor() {
	return fd;
}

bool sqlrexport::exportToFile(const char *filename) {
	return exportToFile(filename,NULL);
}

bool sqlrexport::exportToFile(const char *filename, const char *table) {
	return true;
}

bool sqlrexport::exportToTable(sqlrconnection *sqlrcon,
					const char *table) {
	sqlrcursor	*sqlrcur=new sqlrcursor(sqlrcon);
	bool		retval=exportToTable(sqlrcon,sqlrcur,table);
	delete sqlrcur;
	return retval;
}

bool sqlrexport::exportToTable(sqlrconnection *sqlrcon,
					sqlrcursor *sqlrcur,
					const char *table) {
	return true;
}

bool sqlrexport::exportToJsonDomNode(domnode *jsondomnode) {
	return exportToJsonDomNode(jsondomnode,NULL);
}

bool sqlrexport::exportToJsonDomNode(domnode *jsondomnode, const char *table) {
	return true;
}
