// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimport.h>

#include <rudiments/file.h>

sqlrimport::sqlrimport() {
	sqlrcon=NULL;
	sqlrcur=NULL;
	dbtype=NULL;
	objectname=NULL;
	ignorecolumns=false;
	commitcount=0;
	lg=NULL;
	coarseloglevel=0;
	fineloglevel=9;
	logindent=0;
	logerrors=true;
	lowercasecolumnnames=false;
	uppercasecolumnnames=false;
	importedrowcount=0;
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

#if 0
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
	importedrowcount=0;
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
	importedrowcount++;
	return true;
}

bool sqlrimport::rowsEnd() {
	// by default, just return success
	return true;
}

uint64_t sqlrimport::getImportedRowCount() {
	return importedrowcount;
}
#endif
