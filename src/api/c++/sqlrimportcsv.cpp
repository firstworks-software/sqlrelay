// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportcsv.h>

#include <rudiments/file.h>
#include <rudiments/datetime.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#define NEED_IS_DATETIME_TYPE_CHAR
#include <datatypes.h>

sqlrimportcsv::sqlrimportcsv() : sqlrimportfile(), csvsax() {
	currenttablecol=0;
	foundfieldtext=false;
	fieldcount=0;
	emptyrecord=true;
	recordcount=0;
	committedcount=0;
}

sqlrimportcsv::~sqlrimportcsv() {
}

bool sqlrimportcsv::importData() {

	// update flags and counters
	clearFlagsAndCounts();

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


	// if this is just a normal column...

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

	// if this is a normal field...

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

	// clear the query buffer
	query.clear();

	// if we're ignoring empty records, and this
	// was an empty record, then ignore it
	if (getIgnoreEmptyRows() && emptyrecord) {

		// call the row-end event
		return rowEnd();
	}

	// if there were any actual values (i.e. not an empty csv)
	if (fieldcount) {

		// build the insert query...

		// insert into...
		query.append("insert into ")->append(getObjectName());

		// if we're not ignoring the columns specified in the
		// CSV header then build a column list from them...
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

		// if we're committing every so often, and this is the very
		// first record, then begin a transaction
		if (getCommitCount() && !recordcount) {
			if (!beginStart()) {
				return false;
			}
			if (!getSqlrConnection()->begin()) {
				if (!error(
					getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage())) {
					return false;
				}
			}
			if (!beginEnd()) {
				return false;
			}
		}

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

			if (!commitStart()) {
				return false;
			}
			if (!getSqlrConnection()->commit()) {
				if (!error(
					getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage())) {
					return false;
				}
			}
			if (!commitEnd()) {
				return false;
			}
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

			if (!beginStart()) {
				return false;
			}
			if (!getSqlrConnection()->begin()) {
				if (!error(
					getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage())) {
					return false;
				}
			}
			if (!beginEnd()) {
				return false;
			}
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
		if (!commitStart()) {
			return false;
		}
		if (!getSqlrConnection()->commit()) {
			if (!error(
				getSqlrConnection()->errorNumber(),
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
