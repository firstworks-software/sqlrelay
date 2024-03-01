// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportcsv.h>

#include <rudiments/file.h>
#include <rudiments/datetime.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#define NEED_IS_DATETIME_TYPE_CHAR
#include <datatypes.h>

sqlrimportcsv::sqlrimportcsv() : sqlrimportfile(), csvsax() {
	insertprimarykey=false;
	primarykeycolumnname=NULL;
	primarykeycolumnindex=0;
	primarykeysequence=NULL;
	ignorecolumnswithemptynames=false;
	ignoreemptyrecords=false;
	currenttablecol=0;
	foundfieldtext=false;
	fieldcount=0;
	emptyrecord=true;
	recordcount=0;
	committedcount=0;
	columnswithemptynamesnode=NULL;
	staticvaluecolumnnames.setManageArrayValues(true);
	staticvaluecolumnvalues.setManageArrayValues(true);
}

sqlrimportcsv::~sqlrimportcsv() {
	delete[] primarykeycolumnname;
	delete[] primarykeysequence;
}

void sqlrimportcsv::insertPrimaryKey(const char *primarykeycolumnname,
					uint32_t primarykeycolumnindex,
					const char *primarykeysequence) {
	removePrimaryKey();
	this->primarykeycolumnname=charstring::duplicate(primarykeycolumnname);
	this->primarykeycolumnindex=primarykeycolumnindex;
	this->primarykeysequence=charstring::duplicate(primarykeysequence);
	insertprimarykey=true;
}

void sqlrimportcsv::removePrimaryKey() {
	delete[] this->primarykeycolumnname;
	delete[] this->primarykeysequence;
	this->primarykeycolumnname=NULL;
	this->primarykeysequence=NULL;
	insertprimarykey=false;
}

void sqlrimportcsv::insertStaticValue(const char *columnname,
					uint32_t columnindex,
					const char *value) {
	removeStaticValue(columnindex);
	staticvaluecolumnnames.setValue(
			columnindex,charstring::duplicate(columnname));
	staticvaluecolumnvalues.setValue(
			columnindex,charstring::duplicate(value));
}

void sqlrimportcsv::removeStaticValue(uint32_t columnindex) {
	staticvaluecolumnnames.remove(columnindex);
	staticvaluecolumnvalues.remove(columnindex);
}

void sqlrimportcsv::setIgnoreColumnsWithEmptyNames(
					bool ignorecolumnswithemptynames) {
	this->ignorecolumnswithemptynames=ignorecolumnswithemptynames;
}

void sqlrimportcsv::setIgnoreEmptyRecords(bool ignoreemptyrecords) {
	this->ignoreemptyrecords=ignoreemptyrecords;
}

bool sqlrimportcsv::importData() {

	// update flags and counters
	clearFlagsAndCounts();
	columnswithemptynames.clear();

	// set the table name from the file name,
	// if it wasn't already set
	if (!getObjectName()) {
		setObjectName(file::getBaseName(getFileName(),".csv"));
	}

	// run the import-start event, parse the file, run the import-end event
	return importStart() && csvsax::parseFile(getFileName()) && importEnd();
}

bool sqlrimportcsv::headerStart() {

	// call the columns-start event
	return columnsStart();
}

bool sqlrimportcsv::column(const char *name, bool quoted) {

	// remap the name, if the name has been mapped
	const char	*mappedname=columnmap.getValue(name);
	if (mappedname) {
		name=mappedname;
	}

	// if this column is the primary key...
	if (insertprimarykey && getCurrentColumn()==primarykeycolumnindex) {

		// and we're building a list of column names from the ones
		// specified in the CSV header, rather than just grabbing
		// the columns from the table itself...
		if (!getIgnoreColumns()) {

			// set the current column name (and field)
			char	*cname=charstring::duplicate(
						primarykeycolumnname);
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
		}
		setCurrentColumn(getCurrentColumn()+1);
	}

	// if there are any static columns...
	if (staticvaluecolumnnames.getCount()) {

		// loop, handling them
		for (;;) {

			// get the static column name for this position
			const char	*svname=
				staticvaluecolumnnames.getValue(
							getCurrentColumn());
			if (!svname) {
				break;
			}

			// and we're building a list of column names from the
			// ones specified in the CSV header, rather than just
			// grabbing the columns from the table itself...
			if (!getIgnoreColumns()) {

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
				columns[columns.getCount()]=
						getCurrentColumnName();

				// reset the current field to the current
				// column name (in case columnStart() overrode
				// the column name)
				setCurrentField(getCurrentColumnName());

				// call the column-end event
				if (!columnEnd()) {
					return false;
				}
			}
			setCurrentColumn(getCurrentColumn()+1);
		}
	}

	// by default, we want to include this column in the list of column
	// names that we're building
	bool	includecolumn=true;

	// but, if we're ignoring columns with empty names...
	if (ignorecolumnswithemptynames) {

		// if this column name is empty, then don't include it
		// list of column names that we're building
		includecolumn=!charstring::isNullOrEmpty(name);

		if (!includecolumn) {

			// and put it in the list of columns to ignore when
			// importing data later too
			columnswithemptynames.append(getCurrentColumn());
		}
	}

	// and we're building a list of column names from the ones specified in
	// the CSV header, rather than just grabbing the columns from the table
	// itself, and not ignoring this column because it's name was empty...
	if (!getIgnoreColumns() && includecolumn) {

		// set the current column name
		char	*cname=charstring::duplicate(name);
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
	}

	// next...
	setCurrentColumn(getCurrentColumn()+1);

	return true;
}

bool sqlrimportcsv::headerEnd() {

	// set the current column and field to NULL
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// we need to figure out which columns are numbers or dates...

	// if we're not ignoring columns, but there weren't any (i.e. a totally
	// empty csv), then don't get any info about columns from the database,
	// just call the columns-end event and bail
	if (!getIgnoreColumns() && !columns.getCount()) {
		return columnsEnd();
	}

	// get info about these columns from the database
	query.clear();
	query.append("select ");

	if (getIgnoreColumns()) {
		// if we're ignoring the columns specified in the CSV header,
		// then just grab the column names from the table itself
		query.append('*');
	} else {
		// if we built a list of column names from the ones specified
		// in the CSV header, then select those specific columns
		//
		// NOTE: columns[] should contain the full list of columns,
		// including any inserted primary key or static columns
		for (uint64_t i=0; i<columns.getCount(); i++) {
			if (i) {
				query.append(',');
			}
			query.append(columns[i]);
		}
	}
	query.append(" from ")->append(getObjectName());
	getSqlrCursor()->setResultSetBufferSize(1);
	if (!getSqlrCursor()->sendQuery(query.getString())) {
		return false;
	}

	// run through the columns, figuring out which are numbers and dates...
	uint32_t	colcount=getSqlrCursor()->colCount();
	for (uint32_t i=0; i<colcount; i++) {
		setIsNumericColumn(i,
			isNumberTypeChar(getSqlrCursor()->getColumnType(i)));
		setIsDateTimeColumn(i,
			isDateTimeTypeChar(getSqlrCursor()->getColumnType(i)));
	}

	if (getLogger()) {
		getLogger()->write(getCoarseLogLevel(),
					NULL,getLogIndent(),
					"%ld columns",(unsigned long)colcount);
	}

	// call the columns-end event
	return columnsEnd();
}

bool sqlrimportcsv::bodyStart() {

	// update flags and counters
	recordcount=0;
	committedcount=0;
	setImportedRowCount(0);
	setCurrentColumn(0);
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the start-rows event
	return rowsStart();
}

bool sqlrimportcsv::recordStart() {

	// update flags and counters
	currenttablecol=0;
	fieldcount=0;
	emptyrecord=true;
	columnswithemptynamesnode=columnswithemptynames.getFirst();
	setCurrentColumn(0);
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the start-row event
	return rowStart();
}

bool sqlrimportcsv::field(const char *value, bool quoted) {

	// if we're manually adding the primary key, and this is the primary
	// key position, then add it
	// (don't count this when determining if a record was empty or not)
	if (insertprimarykey && getCurrentColumn()==primarykeycolumnindex) {

		// set the current column name
		setCurrentColumnName(columns[getCurrentColumn()]);

		// never quote these
		quotefield[getCurrentColumn()]=false;

		// set the current field
		char	*tmp;
		if (primarykeysequence) {
			stringbuffer	tmpstr;
			tmpstr.printf(getSqlrConnection()->nextvalFormat(),
							primarykeysequence);
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
		currenttablecol++;
		fieldcount++;
	}

	// if there are any static columns...
	// (don't count these when determining if a record was empty or not)
	if (staticvaluecolumnnames.getCount()) {

		// loop, handling them
		for (;;) {

			// set the current column name
			setCurrentColumnName(columns[getCurrentColumn()]);

			// get the static column name for this position
			const char	*colname=
				staticvaluecolumnnames.getValue(
							getCurrentColumn());
			if (!colname) {
				break;
			}

			// get the static column value for this position
			const char	*colvalue=
				staticvaluecolumnvalues.getValue(
							getCurrentColumn());

			// always quote these
			quotefield[getCurrentColumn()]=true;

			// set the current field
			stringbuffer	tmpstr;
			appendField(&tmpstr,colvalue,false,false);
			char		*tmp=tmpstr.detachString();
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
			currenttablecol++;
			fieldcount++;
		}
	}

	// should we include this field, or ignore it
	// because its column name was blank?
	bool	includefield=true;
	if (ignorecolumnswithemptynames &&
			columnswithemptynamesnode &&
			getCurrentColumn()==
				columnswithemptynamesnode->getValue()) {
		includefield=false;
		columnswithemptynamesnode=columnswithemptynamesnode->getNext();
	}

	// if we should include this field...
	if (includefield) {

		// set the current column name
		setCurrentColumnName(columns[getCurrentColumn()]);

		// if this value has a mapping, then get that
		const char	*v=fieldmap.getValue(value);
		if (v) {
			value=v;
		}

		// check for a non-empty field
		// (do this AFTER remapping the field in case some set
		// of values get mapped to empty strings or NULLs)
		if (emptyrecord && !charstring::isNullOrEmpty(value)) {
			emptyrecord=false;
		}

		// determine whether to quote this field
		bool	isnumeric=getIsNumericColumn(currenttablecol);
		bool	isdatetime=getIsDateTimeColumn(currenttablecol);
		quotefield[getCurrentColumn()]=!isnumeric;

		// set the current field
		stringbuffer	tmpstr;
		appendField(&tmpstr,value,isnumeric,isdatetime);
		char		*tmp=tmpstr.detachString();
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
		currenttablecol++;
		fieldcount++;

	} else {

		// set the current column name
		setCurrentColumnName(columns[getCurrentColumn()]);

		// FIXME: I think I should call events and setCurrentField()
		// in here somewhere

		// next column...
		setCurrentColumn(getCurrentColumn()+1);
	}

	return true;
}

void sqlrimportcsv::appendField(stringbuffer *strb,
					const char *value,
					bool isnumeric,
					bool isdatetime) {

	// handle empty values
	if (charstring::isNullOrEmpty(value)) {
		strb->append("NULL");
		return;
	}

	// handle non-numbers in numeric columns
	if (isnumeric && !charstring::isNumber(value)) {
		strb->append("NULL");
		return;
	}

	// handle date/times
	if (isdatetime && getReformatDateTime()) {

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
		strb->append(dt);
		delete[] dt;

		return;

	}

	// handle normal values
	for (uint32_t index=0; value[index]; index++) {


		if (value[index]=='\\' &&
			(!charstring::compare(getDbType(),"postgresql") ||
			!charstring::compare(getDbType(),"mysql"))) {

			// for postgres and mysql, escape \'s
			strb->append("\\\\");

		} else {

			char	ch=value[index];

			// double-up any single-quotes
			if (ch=='\'') {
				strb->append('\'');
			}

			// append the character
			strb->append(ch);
		}
	}
}

bool sqlrimportcsv::recordEnd() {
	
	// update flags and counters
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// ignore empty records, if we're configured to do so
	if (ignoreemptyrecords && emptyrecord) {

		// call the row-end event
		return rowEnd();
	}

	// build query
	query.clear();
	query.append("insert into ")->append(getObjectName());
	if (!getIgnoreColumns()) {
		query.append(" (");
		for (uint64_t i=0; i<columns.getCount(); i++) {
			if (i) {
				query.append(',');
			}
			const char	*c=columns[i];
			const char	*m=columnmap.getValue(c);
			if (m) {
				c=m;
			}
			char	*cm=charstring::duplicate(c);
			if (getLowerCaseColumnNames()) {
				charstring::lower(cm);
			} else if (getUpperCaseColumnNames()) {
				charstring::upper(cm);
			}
			query.append(cm);
			delete[] cm;
		}
		query.append(')');
	}
	query.append(" values (");
	for (uint64_t i=0; i<fields.getCount(); i++) {
		if (i) {
			query.append(',');
		}
		if (quotefield[i]) {
			query.append('\'');
		}
		query.append(fields[i]);
		if (quotefield[i]) {
			query.append('\'');
		}
	}
	fields.clear();
	quotefield.clear();
	query.append(')');


	// if there were any actual values (i.e. not an empty csv)
	if (fieldcount) {

		// if we're committing every so often, and this is the very
		// first record, then begin a transaction
		if (getCommitCount() && !recordcount) {
			getSqlrConnection()->begin();
		}

		// send the query
		if (!getSqlrCursor()->sendQuery(query.getString())) {
			if (getLogger() && getLogErrors()) {
				getLogger()->write(getCoarseLogLevel(),
					NULL,getLogIndent(),
					"%s",getSqlrCursor()->errorMessage());
			}
			if (getCommitCount()) {
				getSqlrConnection()->commit();
				getSqlrConnection()->begin();
			}
		}

		// bump the recordcount
		recordcount++;

		// log
		if (getLogger() && !(recordcount%100)) {
			getLogger()->write(getFineLogLevel(),
					NULL,getLogIndent(),
					"imported %lld records",
					(unsigned long long)recordcount);
		}

		// if we're committing every so often, and it's time to commit,
		// then commit, log and begin a new transaction
		if (getCommitCount() && !(recordcount%getCommitCount())) {

			getSqlrConnection()->commit();
			committedcount++;

			if (getLogger()) {
				if (!(committedcount%10)) {
					getLogger()->write(
						getFineLogLevel(),NULL,
						getLogIndent(),
						"committed %lld records "
						"(to %s)...",
						(unsigned long long)
						recordcount,
						getObjectName());
				} else {
					getLogger()->write(
						getFineLogLevel(),NULL,
						getLogIndent(),
						"committed %lld records",
						(unsigned long long)
						recordcount);
				}
			}

			getSqlrConnection()->begin();
		}
	}

	// call the row-end event
	if (!rowEnd()) {
		return false;
	}

	// update flags and counters
	setImportedRowCount(getImportedRowCount()+1);
	setCurrentRow(getCurrentRow()+1);

	return true;
}

bool sqlrimportcsv::bodyEnd() {

	if (getLogger()) {
		getLogger()->write(getCoarseLogLevel(),NULL,getLogIndent(),
					"imported %lld records",
					(unsigned long long)recordcount);
	}

	// final commit
	if (getCommitCount()) {
		getSqlrConnection()->commit();
		if (getLogger()) {
			getLogger()->write(
					getCoarseLogLevel(),NULL,getLogIndent(),
					"committed %lld records (to %s)",
					(unsigned long long)recordcount,
					getObjectName());
		}
	}

	// clean up column names
	columns.clear();

	// set the current column and field to NULL
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the rows-end event
	return rowsEnd();
}
