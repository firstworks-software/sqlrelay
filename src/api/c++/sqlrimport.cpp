// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimport.h>

#include <rudiments/file.h>
#include <rudiments/error.h>

sqlrimport::sqlrimport() {
	sqlrcon=NULL;
	sqlrcur=NULL;

	insertprimarykey=false;
	primarykeycolumnname=NULL;
	primarykeycolumnindex=0;
	primarykeysequence=NULL;

	staticvaluecolumnnames.setManageArrayValues(true);
	staticvalues.setManageArrayValues(true);

	dbtype=NULL;
	objectname=NULL;

	ignorecolumns=false;
	lowercasecolumnnames=false;
	uppercasecolumnnames=false;
	ignorecolumnswithemptynames=false;
	ignoreemptyrows=false;

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
	delete[] primarykeycolumnname;
	delete[] primarykeysequence;
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

void sqlrimport::insertPrimaryKey(const char *primarykeycolumnname,
					uint32_t primarykeycolumnindex,
					const char *primarykeysequence) {
	removePrimaryKey();
	this->primarykeycolumnname=charstring::duplicate(primarykeycolumnname);
	this->primarykeycolumnindex=primarykeycolumnindex;
	this->primarykeysequence=charstring::duplicate(primarykeysequence);
	insertprimarykey=true;
}

void sqlrimport::removePrimaryKey() {
	delete[] this->primarykeycolumnname;
	delete[] this->primarykeysequence;
	this->primarykeycolumnname=NULL;
	this->primarykeysequence=NULL;
	insertprimarykey=false;
}

bool sqlrimport::getInsertPrimaryKey() {
	return insertprimarykey;
}

const char *sqlrimport::getPrimaryKeyColumnName() {
	return primarykeycolumnname;
}

uint32_t sqlrimport::getPrimaryKeyColumnIndex() {
	return primarykeycolumnindex;
}

const char *sqlrimport::getPrimaryKeySequence() {
	return primarykeysequence;
}

void sqlrimport::insertStaticValue(const char *columnname,
					uint32_t columnindex,
					const char *value) {
	removeStaticValue(columnindex);
	staticvaluecolumnnames.setValue(
			columnindex,charstring::duplicate(columnname));
	staticvalues.setValue(
			columnindex,charstring::duplicate(value));
}

void sqlrimport::removeStaticValue(uint32_t columnindex) {
	staticvaluecolumnnames.remove(columnindex);
	staticvalues.remove(columnindex);
}

const char *sqlrimport::getStaticValueColumnName(uint32_t index) {
	return staticvaluecolumnnames.getValue(index);
}

const char *sqlrimport::getStaticValue(uint32_t index) {
	return staticvalues.getValue(index);
}

uint32_t sqlrimport::getStaticValueCount() {
	return staticvalues.getCount();
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

void sqlrimport::setIgnoreColumnsWithEmptyNames(
					bool ignorecolumnswithemptynames) {
	this->ignorecolumnswithemptynames=ignorecolumnswithemptynames;
}

bool sqlrimport::getIgnoreColumnsWithEmptyNames() {
	return ignorecolumnswithemptynames;
}

void sqlrimport::setIgnoreEmptyRows(bool ignoreemptyrows) {
	this->ignoreemptyrows=ignoreemptyrows;
}

bool sqlrimport::getIgnoreEmptyRows() {
	return ignoreemptyrows;
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

bool sqlrimport::beginStart() {
	// by default, just return success
	return true;
}

bool sqlrimport::beginEnd() {
	// by default, just return success
	return true;
}

bool sqlrimport::commitStart() {
	// by default, just return success
	return true;
}

bool sqlrimport::commitEnd() {
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
	return (index<numericcolumn.getCount())?numericcolumn[index]:false;
}

void sqlrimport::clearAreNumericColumns() {
	numericcolumn.clear();
}

void sqlrimport::setIsDateTimeColumn(uint64_t index, bool value) {
	datetimecolumn[index]=value;
}

bool sqlrimport::getIsDateTimeColumn(uint64_t index) {
	return (index<datetimecolumn.getCount())?datetimecolumn[index]:false;
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

bool sqlrimport::initialBegin() {

	// if we're committing every so often, then begin a transaction
	if (getCommitCount()) {
		if (!beginStart()) {
			return false;
		}
		if (!getSqlrConnection()->begin()) {
			if (!error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage())) {
				return false;
			}
		}
		if (!beginEnd()) {
			return false;
		}
	}
	return true;
}

bool sqlrimport::insertRow() {

	// build the insert query...

	// insert into...
	query.append("insert into ")->append(getObjectName());

	// if we're not ignoring the columns specified in the
	// file then build a column list from them...
	if (!getIgnoreColumns()) {

		query.append(" (");

		// run through the column names...
		bool	first=true;
		for (uint64_t i=0; i<columns.getCount(); i++) {

			// get the column name and remap it,
			// if the name has been mapped
			const char	*c=columns[i];
			const char	*m=columnmap.getValue(c);
			if (m) {
				c=m;
			}

			// if we're ignoring empty column names,
			// and this column name is empty, then ignore it
			if (getIgnoreColumnsWithEmptyNames() &&
					charstring::isNullOrEmpty(c)) {
				continue;
			}

			// determine if we need a comma or not
			if (first) {
				first=false;
			} else {
				query.append(',');
			}

			// upper-case or lower-case the column name,
			// if we need to
			char	*cm=charstring::duplicate(c);
			if (getLowerCaseColumnNames()) {
				charstring::lower(cm);
			} else if (getUpperCaseColumnNames()) {
				charstring::upper(cm);
			}

			// append the column name
			query.append(cm);

			// clean up
			delete[] cm;
		}

		query.append(')');

	}

	// values...
	query.append(" values (");

	// run through the fields...
	bool	first=true;
	for (uint64_t i=0; i<fields.getCount(); i++) {

		// get the column name and remap it,
		// if the name has been mapped
		const char	*c=columns[i];
		const char	*m=columnmap.getValue(c);
		if (m) {
			c=m;
		}

		// if we're ignoring empty column names,
		// and this column name is empty, then ignore it
		if (getIgnoreColumnsWithEmptyNames() &&
				charstring::isNullOrEmpty(c)) {
			continue;
		}

		// determine if we need a comma or not
		if (first) {
			first=false;
		} else {
			query.append(',');
		}

		// open-quote the field, if necessary
		if (quotefield[i]) {
			query.append('\'');
		}

		// append the field
		query.append(fields[i]);

		// close-quote the field, if necessary
		if (quotefield[i]) {
			query.append('\'');
		}
	}
	query.append(')');

	// clean up
	fields.clear();
	quotefield.clear();

	// send the query
	if (!getSqlrCursor()->sendQuery(query.getString())) {
		if (getLogger() && getLogErrors()) {
			getLogger()->write(getCoarseLogLevel(),
				NULL,getLogIndent(),
				"%s",getSqlrCursor()->errorMessage());
		}
		if (!error(getSqlrConnection()->errorNumber(),
				getSqlrConnection()->errorMessage())) {
			return false;
		}
	}

	return true;
}

bool sqlrimport::periodicCommit() {

	// if we're committing every so often, and it's time to commit,
	// then commit, log and begin a new transaction
	if (getCommitCount() && !(getImportedRowCount()%getCommitCount())) {

		if (!commitStart()) {
			return false;
		}
		if (!getSqlrConnection()->commit()) {
			if (!error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage())) {
				return false;
			}
		}
		if (!commitEnd()) {
			return false;
		}

		if (getLogger()) {
			if (!((getImportedRowCount()/getCommitCount())%10)) {
				getLogger()->write(
					getFineLogLevel(),NULL,
					getLogIndent(),
					"committed %lld records "
					"(to %s)...",
					(unsigned long long)
					getImportedRowCount(),
					getObjectName());
			} else {
				getLogger()->write(
					getFineLogLevel(),NULL,
					getLogIndent(),
					"committed %lld records",
					(unsigned long long)
					getImportedRowCount());
			}
		}

		if (!beginStart()) {
			return false;
		}
		if (!getSqlrConnection()->begin()) {
			if (!error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage())) {
				return false;
			}
		}
		if (!beginEnd()) {
			return false;
		}
	}
	return true;
}

bool sqlrimport::finalCommit() {

	// final commit
	if (getCommitCount()) {
		if (!commitStart()) {
			return false;
		}
		if (!getSqlrConnection()->commit()) {
			if (!error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage())) {
				return false;
			}
		}
		if (!commitEnd()) {
			return false;
		}
		if (getLogger()) {
			getLogger()->write(
				getCoarseLogLevel(),NULL,getLogIndent(),
				"committed %lld records (to %s)",
				(unsigned long long)getImportedRowCount(),
				getObjectName());
		}
	}
	return true;
}

bool sqlrimport::systemError() {
	char	*err=error::getErrorString();
	bool	retval=error(error::getErrorNumber(),err);
	delete[] err;
	return retval;
}
