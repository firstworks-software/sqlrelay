// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportcsv.h>

#include <rudiments/file.h>
#include <rudiments/datetime.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#define NEED_IS_DATETIME_TYPE_CHAR
#include <datatypes.h>

sqlrimportcsv::sqlrimportcsv() : sqlrimport(), csvsax() {
	insertprimarykey=false;
	primarykeycolumnname=NULL;
	primarykeycolumnindex=0;
	primarykeysequence=NULL;
	ignorecolumnswithemptynames=false;
	ignoreemptyrecords=false;
	colcount=0;
	currenttablecol=0;
	numbercolumn=NULL;
	datecolumn=NULL;
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
	delete[] numbercolumn;
	delete[] datecolumn;
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

bool sqlrimportcsv::importFromFile(const char *filename) {

	// reset flags
	setCurrentRow(0);
	setCurrentColumn(0);
	setCurrentField(NULL);
	delete[] numbercolumn;
	delete[] datecolumn;
	numbercolumn=NULL;
	datecolumn=NULL;
	columnswithemptynames.clear();

	if (!objectname) {
		objectname=file::getBaseName(filename,".csv");
	}
	return csvsax::parseFile(filename);
}

bool sqlrimportcsv::headerStart() {

	// call the pr-columns event
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
		if (!ignorecolumns) {

			// call the pre-column event
			if (!columnStart()) {
				return false;
			}

			// append the primary key name
			uint64_t	index=columns.getCount();
			columns[index]=
				charstring::duplicate(primarykeycolumnname);

			// set the current field
			setCurrentField(columns[index]);

			// call the post-column event
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
			const char	*colname=
				staticvaluecolumnnames.getValue(
							getCurrentColumn());
			if (!colname) {
				break;
			}

			// and we're building a list of column names from the
			// ones specified in the CSV header, rather than just
			// grabbing the columns from the table itself...
			if (!ignorecolumns) {

				// call the pre-column event
				if (!columnStart()) {
					return false;
				}

				// append the column name
				uint64_t	index=columns.getCount();
				columns[index]=
					charstring::duplicate(colname);

				// set the current field
				setCurrentField(columns[index]);

				// call the post-column event
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
	if (!ignorecolumns && includecolumn) {

		// call the pre-column event
		if (!columnStart()) {
			return false;
		}

		// append the column name to the list of column names
		uint64_t	index=columns.getCount();
		columns[index]=charstring::duplicate(name);

		// set the current field
		setCurrentField(columns[index]);

		// call the post-column event
		if (!columnEnd()) {
			return false;
		}
	}

	// next...
	setCurrentColumn(getCurrentColumn()+1);

	return true;
}

bool sqlrimportcsv::headerEnd() {

	// we need to figure out which columns are numbers or dates...

	// bail if there were no columns
	// (eg. if the csv file was completely empty)
	if (!columns.getCount()) {
		return true;
	}

	// get info about these columns from the database
	query.clear();
	query.append("select ");

	if (ignorecolumns) {
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
	query.append(" from ")->append(objectname);
	sqlrcur->setResultSetBufferSize(1);
	if (!sqlrcur->sendQuery(query.getString())) {
		return false;
	}

	// get the column count
	colcount=sqlrcur->colCount();

	// run through the columns, figuring out which are numbers and dates...
	numbercolumn=new bool[colcount];
	datecolumn=new bool[colcount];
	for (uint32_t i=0; i<colcount; i++) {
		numbercolumn[i]=isNumberTypeChar(sqlrcur->getColumnType(i));
		datecolumn[i]=isDateTimeTypeChar(sqlrcur->getColumnType(i));
	}

	if (lg) {
		lg->write(coarseloglevel,NULL,logindent,
				"%ld columns",(unsigned long)colcount);
	}

	// call the post-columns event
	return columnsEnd();
}

bool sqlrimportcsv::bodyStart() {

	// reset counts
	recordcount=0;
	committedcount=0;
	setImportedRowCount(0);

	// call the pre-rows event
	return rowsStart();
}

bool sqlrimportcsv::recordStart() {

	// reset flags and counters
	currenttablecol=0;
	setCurrentColumn(0);
	fieldcount=0;
	emptyrecord=true;
	columnswithemptynamesnode=columnswithemptynames.getFirst();

	// call the pre-row event
	return rowStart();
}

bool sqlrimportcsv::field(const char *value, bool quoted) {

	// if we're manually adding the primary key, and this is the primary
	// key position, then add it
	// (don't count this when determining if a record was empty or not)
	if (insertprimarykey && getCurrentColumn()==primarykeycolumnindex) {

		// call the pre-field event
		if (!fieldStart()) {
			return false;
		}

		// append the field
		uint64_t	index=fields.getCount();
		if (primarykeysequence) {
			stringbuffer	tmp;
			tmp.printf(sqlrcon->nextvalFormat(),primarykeysequence);
			fields[index]=tmp.detachString();
		} else {
			fields[index]=charstring::duplicate("null");
		}

		// set the current field
		setCurrentField(fields[index]);

		// call the post-field event
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

			// call the pre-field event
			if (!fieldStart()) {
				return false;
			}

			// append the field
			uint64_t	index=fields.getCount();
			stringbuffer	tmp;
			appendField(&tmp,colvalue,0,true);
			fields[index]=tmp.detachString();

			// set the current field
			setCurrentField(fields[index]);

			// call the post-field event
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

		// call the pre-field event
		if (!fieldStart()) {
			return false;
		}

		// append the field
		uint64_t	index=fields.getCount();
		stringbuffer	tmp;
		appendField(&tmp,value,currenttablecol,false);
		fields[index]=tmp.detachString();

		// set the current field
		setCurrentField(fields[index]);

		// call the post-field event
		if (!fieldEnd()) {
			return false;
		}

		// next...
		setCurrentColumn(getCurrentColumn()+1);
		currenttablecol++;
		fieldcount++;

	} else {

		// next column...
		setCurrentColumn(getCurrentColumn()+1);
	}

	return true;
}

void sqlrimportcsv::appendField(stringbuffer *query,
					const char *value,
					uint32_t currenttablecol,
					bool overrideisstring) {

	if (!charstring::isNullOrEmpty(value)) {

		bool	isnumber=(!overrideisstring && numbercolumn)?
					numbercolumn[currenttablecol]:false;
		bool	isdate=(!overrideisstring && datecolumn)?
					datecolumn[currenttablecol]:false;
		if (!isnumber || isdate) {
			query->append('\'');
		}
		if (isdate) {
			int16_t year;
			int16_t month;
			int16_t day;
			int16_t hour;
			int16_t minute;
			int16_t second;
			int32_t microsecond;
			bool isnegative;
			// FIXME: pass in ddmm, yyyyddmm, datedelimiters
			datetime::parse(value,false,false,"-/",
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

			// FIXME: make this configurable
			// massage the year...
			// If it's less than 100, then assume that the century
			// wasn't given.  If what was given is > 10 years from
			// the current year, then assume it was meant to be a
			// date from the previous century.
			if (year<100) {
				datetime	dt;
				dt.initFromSystemDateTime();
				int32_t	century=dt.getCentury();
				if (year>dt.getShortYear()+10) {
					century--;
				}
				year=((century-1)*100)+year;
			}

			// FIXME: what about microseconds and negatives?
			char	*dt=datetime::formatAs(
						"YYYY-MM-DD HH24:MI:SS",
						year,month,day,
						hour,minute,second,
						microsecond,isnegative);
			query->append(dt);
			delete[] dt;

		} else if (isnumber && !charstring::isNumber(value)) {
			query->append("NULL");
		} else {
			escapeField(query,value);
		}
		if (!isnumber || isdate) {
			query->append('\'');
		}
	} else {
		query->append("NULL");
	}
}

bool sqlrimportcsv::recordEnd() {

	// ignore empty records, if we're configured to do so
	if (ignoreemptyrecords && emptyrecord) {

		// call the row-end event
		return rowEnd();
	}

	// build query
	query.clear();
	query.append("insert into ")->append(objectname);
	if (!ignorecolumns) {
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
			if (lowercasecolumnnames) {
				charstring::lower(cm);
			} else if (uppercasecolumnnames) {
				charstring::upper(cm);
			}
			query.append(cm);
			delete[] cm;
		}
		query.append(")");
	}
	query.append(" values (");
	for (uint64_t i=0; i<fields.getCount(); i++) {
		if (i) {
			query.append(',');
		}
		query.append(fields[i]);
		delete[] fields[i];
	}
	fields.clear();
	query.append(')');


	// if there were any actual values (i.e. not an empty csv)
	if (fieldcount) {

		// if we're committing every so often, and this is the very
		// first record, then begin a transaction
		if (commitcount && !recordcount) {
			sqlrcon->begin();
		}

		// send the query
		if (!sqlrcur->sendQuery(query.getString())) {
			if (lg && logerrors) {
				lg->write(coarseloglevel,NULL,logindent,
						"%s",sqlrcur->errorMessage());
			}
			if (commitcount) {
				sqlrcon->commit();
				sqlrcon->begin();
			}
		}

		// bump the recordcount
		recordcount++;

		// log
		if (lg && !(recordcount%100)) {
			lg->write(fineloglevel,NULL,logindent,
					"imported %lld records",
					(unsigned long long)recordcount);
		}

		// if we're committing every so often, and it's time to commit,
		// then commit, log and begin a new transaction
		if (commitcount && !(recordcount%commitcount)) {

			sqlrcon->commit();
			committedcount++;

			if (lg) {
				if (!(committedcount%10)) {
					lg->write(fineloglevel,NULL,logindent,
						"committed %lld records "
						"(to %s)...",
						(unsigned long long)
						recordcount,
						objectname);
				} else {
					lg->write(fineloglevel,NULL,logindent,
						"committed %lld records",
						(unsigned long long)
						recordcount);
				}
			}

			sqlrcon->begin();
		}
	}

	// call the row-end event
	if (!rowEnd()) {
		return false;
	}

	setImportedRowCount(getImportedRowCount()+1);
	setCurrentRow(getCurrentRow()+1);

	return true;
}

bool sqlrimportcsv::bodyEnd() {

	if (lg) {
		lg->write(coarseloglevel,NULL,logindent,
				"imported %lld records",
				(unsigned long long)recordcount);
	}

	// final commit
	if (commitcount) {
		sqlrcon->commit();
		if (lg) {
			lg->write(coarseloglevel,NULL,logindent,
					"committed %lld records (to %s)",
					(unsigned long long)recordcount,
					objectname);
		}
	}

	// clean up column names
	for (uint64_t i=0; i<columns.getCount(); i++) {
		delete[] columns[i];
	}

	// call the rows-end event
	return rowsEnd();
}


void sqlrimportcsv::escapeField(stringbuffer *strb, const char *field) {
	for (uint32_t index=0; field[index]; index++) {
		if (field[index]=='\\' &&
				(!charstring::compare(dbtype,"postgresql") ||
				!charstring::compare(dbtype,"mysql"))) {

			// for postgres and mysql, escape \'s
			strb->append("\\\\");

		} else {

			char	ch=field[index];

			// double-up any single-quotes
			if (ch=='\'') {
				strb->append('\'');
			}

			// append the character
			strb->append(ch);
		}
	}
}
