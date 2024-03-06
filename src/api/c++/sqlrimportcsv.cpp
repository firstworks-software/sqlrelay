// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportcsv.h>

#include <rudiments/file.h>
#include <rudiments/datetime.h>

sqlrimportcsv::sqlrimportcsv() : sqlrimportfile(), csvsax() {
	currenttablecol=0;
	foundfieldtext=false;
	emptyrecord=true;
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

	// if we're not ignoring columns, but there weren't any (i.e. a totally
	// empty csv), then don't get any info about columns from the database,
	// just call the columns-end event and bail
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

bool sqlrimportcsv::bodyStart() {

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

bool sqlrimportcsv::recordStart() {

	// update flags and counters
	currenttablecol=0;
	emptyrecord=true;
	setIgnoreRow(false);
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
	}


	// if there are any static columns...
	// (don't count these when determining if a record was empty or not)
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

	// if we're ignoring this record in particular, there were no columns
	// (somehow), or if we're generally ignoring empty records, and this
	// was an empty record, then ignore it
	if (getIgnoreRow() || !columns.getCount() ||
		(getIgnoreEmptyRows() && emptyrecord)) {

		// call the row-end event
		if (!rowEnd()) {
			return false;
		}

		// update flags and counters
		setCurrentRow(getCurrentRow()+1);

		return true;
	}

	// insert the row
	if (!insertRow()) {
		return false;
	}

	// call the row-end event
	if (!rowEnd()) {
		return false;
	}

	// update flags and counters
	setImportedRowCount(getImportedRowCount()+1);
	setCurrentRow(getCurrentRow()+1);

	// log
	if (getLogger() && !(getImportedRowCount()%100)) {
		getLogger()->write(getFineLogLevel(),
				NULL,getLogIndent(),
				"imported %lld records",
				(unsigned long long)getImportedRowCount());
	}

	// do periodic commit (if necessary)
	return periodicCommit();
}

bool sqlrimportcsv::bodyEnd() {

	if (getLogger()) {
		getLogger()->write(getCoarseLogLevel(),NULL,getLogIndent(),
				"imported %lld records",
				(unsigned long long)getImportedRowCount());
	}

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
