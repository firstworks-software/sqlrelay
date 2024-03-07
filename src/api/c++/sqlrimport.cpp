// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimport.h>

#include <rudiments/file.h>
#include <rudiments/datetime.h>
#include <rudiments/error.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#define NEED_IS_DATETIME_TYPE_CHAR
#include <datatypes.h>

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
	// by default, just return success
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

	// FIXME: push down
	if (getLogger() && !(getImportedRowCount()%100)) {
		getLogger()->write(getFineLogLevel(),
				NULL,getLogIndent(),
				"importing %lld rows",
				(unsigned long long)getImportedRowCount());
	}

	// by default, just return success
	return true;
}

bool sqlrimport::rowsEnd() {

	// FIXME: push down
	if (getLogger() && getCommitCount()) {
		getLogger()->write(getCoarseLogLevel(),NULL,getLogIndent(),
				"committed %lld records (to %s)",
				(unsigned long long)getImportedRowCount(),
				getObjectName());
	}
	if (getLogger()) {
		getLogger()->write(getCoarseLogLevel(),NULL,getLogIndent(),
				"imported %lld records",
				(unsigned long long)getImportedRowCount());
	}

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

	// FIXME: push down
	if (getLogger() && !((getImportedRowCount()/getCommitCount())%10)) {
		getLogger()->write(
			getCoarseLogLevel(),NULL,
			getLogIndent(),
			"committed %lld records (to %s)...",
			(unsigned long long)
			getImportedRowCount(),
			getObjectName());
	}

	// by default, just return success
	return true;
}

bool sqlrimport::error(int64_t errornumber, const char *errormessage) {

	// FIXME: push down
	if (getLogger() && getLogErrors()) {
		getLogger()->write(getCoarseLogLevel(),
			NULL,getLogIndent(),"%s",errormessage);
	}

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
	numericcolumn.clear();
	datetimecolumn.clear();
	currentfield=NULL;
	emptyrow=true;
	importedrowcount=0;
	commitcount=0;
}

void sqlrimport::setEmptyRow(bool emptyrow) {
	this->emptyrow=emptyrow;
}

bool sqlrimport::getEmptyRow() {
	return emptyrow;
}

bool sqlrimport::startProcessingColumns() {
	// call the columns-start event
	return columnsStart();
}

bool sqlrimport::processColumnName(char **cname) {

	// first, process any primary keys or static
	// columns that should go before this one
	if (!processPrimaryKeyAndStaticColumns()) {
		return false;
	}

	// now, process the actual column name...

	// remap the name, if the name has been mapped
	const char	*mappedname=columnmap.getValue(*cname);
	if (mappedname) {
		delete[] *cname;
		*cname=charstring::duplicate(mappedname);
	}

	// set the current column name
	setCurrentColumnName(*cname);
	setCurrentField(*cname);

	// call the column-start event
	if (!columnStart()) {
		delete[] *cname;
		*cname=NULL;
		return false;
	}

	// append the current column
	// (which columnStart() may have overridden)
	if (getCurrentColumnName()!=*cname) {
		delete[] *cname;
		*cname=NULL;
	}
	columns[columns.getCount()]=getCurrentColumnName();

	// reset the current field to the current column name
	// (in case columnStart() overrode the column name)
	setCurrentField(getCurrentColumnName());

	// call the column-end event
	if (!columnEnd()) {
		return false;
	}

	// next...
	setCurrentColumn(getCurrentColumn()+1);

	return true;
}

bool sqlrimport::processPrimaryKeyAndStaticColumns() {

	// if this column is the primary key...
	if (getInsertPrimaryKey() &&
			getCurrentColumn()==getPrimaryKeyColumnIndex()) {

		// set the current column name (and field)
		char	*cname=charstring::duplicate(
					getPrimaryKeyColumnName());
		setCurrentColumnName(cname);
		setCurrentField(cname);

		// call the column-start event
		if (!columnStart()) {
			delete[] cname;
			return false;
		}

		// append the current column
		// (which columnStart() may have overridden)
		if (getCurrentColumnName()!=cname) {
			delete[] cname;
		}
		columns[columns.getCount()]=getCurrentColumnName();

		// reset the current field to the current column name
		// (in case columnStart() overrode the column name)
		setCurrentField(getCurrentColumnName());

		// call the column-end event
		if (!columnEnd()) {
			return false;
		}

		// next...
		setCurrentColumn(getCurrentColumn()+1);
	}


	// if there are any static columns...
	if (getStaticValueCount()) {

		// loop, handling them
		for (;;) {

			// get the static column name for this position
			const char	*svname=
				getStaticValueColumnName(getCurrentColumn());
			if (!svname) {
				break;
			}

			// set the current column name (and field)
			char	*cname=charstring::duplicate(svname);
			setCurrentColumnName(cname);
			setCurrentField(cname);

			// call the column-start event
			if (!columnStart()) {
				delete[] cname;
				return false;
			}

			// append the current column
			// (which columnStart() may have overridden)
			if (getCurrentColumnName()!=cname) {
				delete[] cname;
			}
			columns[columns.getCount()]=getCurrentColumnName();

			// reset the current field to the current
			// column name (in case columnStart() overrode
			// the column name)
			setCurrentField(getCurrentColumnName());

			// call the column-end event
			if (!columnEnd()) {
				return false;
			}

			// next...
			setCurrentColumn(getCurrentColumn()+1);
		}
	}

	return true;
}

bool sqlrimport::determineColumnTypes() {

	// FIXME: this doesn't handle primary keys or static values, so 
	// numericcolumn[x] may not map correctly to column[x]

	// we need to figure out which columns are numbers or dates...

	// get info about these columns from the database
	query.clear();
	query.append("select ");

	if (getIgnoreColumns()) {
		// if we're ignoring the columns specified in the file,
		// then just grab the column names from the table itself
		query.append('*');
	} else {
		// if we built a list of column names from the ones specified
		// in the file, then select those specific columns
		//
		// NOTE: columns[] should contain the full list of columns,
		// including any inserted primary key columns, static columns,
		// and columns with empty names
		for (uint64_t i=0; i<columns.getCount(); i++) {

			if (i) {
				query.append(',');
			}

			// if we're ignoring columns with empty names and
			// this column has an empty name...
			if (getIgnoreColumnsWithEmptyNames() &&
				charstring::isNullOrEmpty(columns[i])) {

				// then fetch NULL instead of the column itself
				query.append("NULL");

			} else {

				// otherwise, fetch the column
				query.append(columns[i]);
			}
		}
	}
	query.append(" from ")->append(getObjectName());
	getSqlrCursor()->setResultSetBufferSize(1);
	if (!getSqlrCursor()->sendQuery(query.getString())) {
		if (!error(getSqlrConnection()->errorNumber(),
				getSqlrConnection()->errorMessage())) {
			return false;
		}
	}

	// run through the columns, figuring out which are numbers and dates...
	uint32_t	colcount=getSqlrCursor()->colCount();
	for (uint32_t i=0; i<colcount; i++) {
		setIsNumericColumn(i,
			isNumberTypeChar(getSqlrCursor()->getColumnType(i)));
		setIsDateTimeColumn(i,
			isDateTimeTypeChar(getSqlrCursor()->getColumnType(i)));
	}

	return true;
}

bool sqlrimport::endProcessingColumns() {

	// set the current column and field to NULL
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// if we're not ignoring columns, but there weren't any, then don't get
	// any info about columns from the database, just call the columns-end
	// event and bail
	if (!getIgnoreColumns() && !columns.getCount()) {
		return columnsEnd();
	}

	// we need to figure out which columns are numbers or dates
	if (!determineColumnTypes()) {
		return false;
	}

	// call the columns-end event
	return columnsEnd();
}

bool sqlrimport::startProcessingRows() {

	// update flags and counters
	setImportedRowCount(0);
	setCurrentColumn(0);
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// begin a transaction (if necessary)
	if (!initialBegin()) {
		return false;
	}

	// call the start-rows event
	return rowsStart();
}

bool sqlrimport::startProcessingRow() {

	// update flags and counters
	setIgnoreRow(false);
	setCurrentColumn(0);
	setCurrentColumnName(NULL);
	setCurrentField(NULL);
	setEmptyRow(true);

	// call the start-row event
	return rowStart();
}

bool sqlrimport::initialBegin() {

	// bail if we're not committing every so often
	if (!getCommitCount()) {
		return true;
	}

	// then begin a transaction
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
	return true;
}

bool sqlrimport::processField(char **fval) {

	// if we're manually adding the primary key, and this is the primary
	// key position, then add it
	// (don't count this when determining if a row was empty or not)
	if (getInsertPrimaryKey() &&
			getCurrentColumn()==getPrimaryKeyColumnIndex()) {

		// set the current column name
		setCurrentColumnName(columns[getCurrentColumn()]);

		// never quote these
		quotefield[getCurrentColumn()]=false;

		// set the current field
		char	*tmp;
		if (getPrimaryKeySequence()) {
			stringbuffer	tmpstr;
			tmpstr.printf(getSqlrConnection()->nextvalFormat(),
						getPrimaryKeySequence());
			tmp=tmpstr.detachString();
		} else {
			tmp=charstring::duplicate("null");
		}
		setCurrentField(tmp);

		// call the field-start event
		if (!fieldStart()) {
			delete[] tmp;
			return false;
		}

		// append the current field
		// (which fieldStart() may have overridden)
		if (getCurrentField()!=tmp) {
			delete[] tmp;
		}
		fields[fields.getCount()]=getCurrentField();

		// call the field-end event
		if (!fieldEnd()) {
			return false;
		}

		// next...
		setCurrentColumn(getCurrentColumn()+1);
	}


	// if there are any static columns...
	// (don't count these when determining if a row was empty or not)
	if (getStaticValueCount()) {

		// loop, handling them
		for (;;) {

			// set the current column name
			setCurrentColumnName(columns[getCurrentColumn()]);

			// get the static column name for this position
			const char	*colname=
				getStaticValueColumnName(getCurrentColumn());
			if (!colname) {
				break;
			}

			// get the static column value for this position
			const char	*colvalue=
				getStaticValue(getCurrentColumn());

			// always quote these
			quotefield[getCurrentColumn()]=true;

			// set the current field
			char	*tmp=massageValue(colvalue,false,false);
			setCurrentField(tmp);

			// call the field-start event
			if (!fieldStart()) {
				delete[] tmp;
				return false;
			}

			// append the current field
			// (which fieldStart() may have overridden)
			if (getCurrentField()!=tmp) {
				delete[] tmp;
			}
			fields[fields.getCount()]=getCurrentField();

			// call the field-end event
			if (!fieldEnd()) {
				return false;
			}

			// next...
			setCurrentColumn(getCurrentColumn()+1);
		}
	}

	// set the current column name
	setCurrentColumnName(columns[getCurrentColumn()]);

	// if this value has a mapping, then get that
	const char	*v=fieldmap.getValue(*fval);
	if (v) {
		delete[] *fval;
		*fval=charstring::duplicate(v);
	}

	// check for a non-empty field
	// (do this AFTER remapping the field in case some set
	// of values get mapped to empty strings or NULLs)
	if (getEmptyRow() && !charstring::isNullOrEmpty(*fval)) {
		setEmptyRow(false);
	}

	// determine whether to quote this field
	// FIXME: getCurrentColumn() will not be corerect here if we
	// inserted a primary key or static columns
	bool	isnumeric=getIsNumericColumn(getCurrentColumn());
	bool	isdatetime=getIsDateTimeColumn(getCurrentColumn());
	quotefield[getCurrentColumn()]=!isnumeric;

	// set the current field
	char	*tmp=massageValue(*fval,isnumeric,isdatetime);
	delete[] *fval;
	*fval=NULL;
	setCurrentField(tmp);

	// call the field-start event
	if (!fieldStart()) {
		delete[] tmp;
		return false;
	}

	// append the current field
	// (which fieldStart() may have overridden)
	if (getCurrentField()!=tmp) {
		delete[] tmp;
	}
	fields[fields.getCount()]=getCurrentField();

	// call the field-end event
	if (!fieldEnd()) {
		return false;
	}

	// next...
	setCurrentColumn(getCurrentColumn()+1);

	return true;
}

char *sqlrimport::massageValue(const char *value,
					bool isnumeric,
					bool isdatetime) {

	// unescape the value
	char	*unescaped=unescapeValue(value);
	if (unescaped) {
		value=unescaped;
	}

	// handle empty values
	if (charstring::isNullOrEmpty(value)) {
		delete[] unescaped;
		return charstring::duplicate("NULL");
	}

	// handle non-numbers in numeric columns
	if (isnumeric && !charstring::isNumber(value)) {
		delete[] unescaped;
		return charstring::duplicate("NULL");
	}

	// handle date/times
	if (isdatetime && getReformatDateTime()) {

		stringbuffer	strb;

		int16_t year;
		int16_t month;
		int16_t day;
		int16_t hour;
		int16_t minute;
		int16_t second;
		int32_t microsecond;
		bool isnegative;
		datetime::parse(value,
				getDdMm(),
				getYyyyDdMm(),
				getDateDelimiters(),
				&year,&month,&day,
				&hour,&minute,&second,
				&microsecond,&isnegative);
		if (hour==-1) {
			hour=0;
		}
		if (minute==-1) {
			minute=0;
		}
		if (second==-1) {
			second=0;
		}
		if (microsecond==-1) {
			microsecond=0;
		}

		// massage the year...
		// If it's less than (eg.) 100, then assume that the
		// century wasn't given.
		// If what was given is > (eg.) 10 years from the
		// current year, then assume it was meant to be a date
		// from the previous century.
		if (year<getNoCenturyThreshold()) {
			datetime	dt;
			dt.initFromSystemDateTime();
			int32_t	century=dt.getCentury();
			if (year>dt.getShortYear()+
					getLastCenturyThreshold()) {
				century--;
			}
			year=((century-1)*getNoCenturyThreshold())+year;
		}

		char	*dt=datetime::formatAs(
					getDateTimeFormat(),
					year,month,day,
					hour,minute,second,
					microsecond,isnegative);
		strb.append(dt);
		delete[] dt;

		delete[] unescaped;
		return strb.detachString();
	}

	// handle normal values
	stringbuffer	strb;
	for (uint32_t index=0; value[index]; index++) {

		if (value[index]=='\\' &&
			(!charstring::compare(getDbType(),"postgresql") ||
			!charstring::compare(getDbType(),"mysql"))) {

			// for postgres and mysql, escape \'s
			strb.append("\\\\");

		} else {

			char	ch=value[index];

			// double-up any single-quotes
			if (ch=='\'') {
				strb.append('\'');
			}

			// append the character
			strb.append(ch);
		}
	}

	delete[] unescaped;
	return strb.detachString();
}

char *sqlrimport::unescapeValue(const char *value) {
	return NULL;
}

bool sqlrimport::endProcessingRow() {
	
	// update flags and counters
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the row-end event
	// (call this before importing the row in case rowEnd() wants to call
	// setEmptyRow() or modify a field or something)
	if (!rowEnd()) {
		return false;
	}

	// clear the query buffer
	query.clear();

	// if we're ignoring this row in particular, there were no columns
	// (somehow), or if we're generally ignoring empty rows, and this
	// was an empty row, then ignore it
	if (getIgnoreRow() || !columns.getCount() ||
		(getIgnoreEmptyRows() && getEmptyRow())) {

		return true;
	}

	// insert the row
	if (!insertRow()) {
		return false;
	}

	// update flags and counters
	setImportedRowCount(getImportedRowCount()+1);
	setCurrentRow(getCurrentRow()+1);

	// do periodic commit (if necessary)
	return periodicCommit();
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
		if (!error(getSqlrConnection()->errorNumber(),
				getSqlrConnection()->errorMessage())) {
			return false;
		}
	}

	return true;
}

bool sqlrimport::periodicCommit() {

	// bail if we're not committing every so often
	if (!getCommitCount()) {
		return true;
	}

	// if it's time to commit, then commit and begin a new transaction
	if (!(getImportedRowCount()%getCommitCount())) {

		// commit
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

		// begin
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

	// bail if we're not committing every so often
	if (!getCommitCount()) {
		return true;
	}

	// do a final commit
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
	return true;
}

bool sqlrimport::endProcessingRows() {

	// do final commit (if necessary)
	if (!finalCommit()) {
		return false;
	}

	// clean up column names
	columns.clear();

	// set the current column and field to NULL
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the rows-end event
	return rowsEnd();
}

bool sqlrimport::systemError() {
	char	*err=error::getErrorString();
	bool	retval=error(error::getErrorNumber(),err);
	delete[] err;
	return retval;
}
