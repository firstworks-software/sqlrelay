// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportxml.h>
#include <rudiments/stdio.h>

const unsigned short sqlrimportxml::NULLTAG=0;
const unsigned short sqlrimportxml::TABLETAG=1;
const unsigned short sqlrimportxml::SEQUENCETAG=2;
const unsigned short sqlrimportxml::COLUMNSTAG=3;
const unsigned short sqlrimportxml::COLUMNTAG=4;
const unsigned short sqlrimportxml::ROWSTAG=5;
const unsigned short sqlrimportxml::ROWTAG=6;
const unsigned short sqlrimportxml::FIELDTAG=7;

const unsigned short sqlrimportxml::NULLATTR=0;
const unsigned short sqlrimportxml::NAMEATTR=1;
const unsigned short sqlrimportxml::TYPEATTR=3;
const unsigned short sqlrimportxml::LENGTHATTR=4;
const unsigned short sqlrimportxml::PRECISIONATTR=5;
const unsigned short sqlrimportxml::SCALEATTR=6;
const unsigned short sqlrimportxml::NULLABLEATTR=7;
const unsigned short sqlrimportxml::PRIMARYKEYATTR=8;
const unsigned short sqlrimportxml::UNIQUEATTR=9;
const unsigned short sqlrimportxml::PARTOFKEYATTR=10;
const unsigned short sqlrimportxml::UNSIGNEDATTR=11;
const unsigned short sqlrimportxml::ZEROFILLEDATTR=12;
const unsigned short sqlrimportxml::BINARYATTR=13;
const unsigned short sqlrimportxml::AUTOINCREMENTATTR=14;

sqlrimportxml::sqlrimportxml() : sqlrimportfile(), xmlsax() {
	currenttag=NULLTAG;
	currentattribute=NULL;
	sequencevalue=NULL;
	colcount=0;
	infield=false;
	emptyrecord=true;
}

sqlrimportxml::~sqlrimportxml() {
	delete[] currentattribute;
	delete[] sequencevalue;
}

bool sqlrimportxml::importData() {

	// update flags and counters
	clearFlagsAndCounts();

	// set the table/sequence name from the file name,
	// if it wasn't already set
	if (!getObjectName()) {
		setObjectName(file::getBaseName(getFileName(),".xml"));
	}

	// run the import-start event, parse the file, run the import-end event
	return importStart() && xmlsax::parseFile(getFileName()) && importEnd();
}

bool sqlrimportxml::tagStart(const char *ns, const char *name) {

	// call the appropriate tag-start method
	if (!charstring::compare(name,"table")) {
		return tableTagStart();
	} else if (!charstring::compare(name,"sequence")) {
		return sequenceTagStart();
	} else if (!charstring::compare(name,"columns")) {
		return columnsTagStart();
	} else if (!charstring::compare(name,"column")) {
		return columnTagStart();
	} else if (!charstring::compare(name,"rows")) {
		return rowsTagStart();
	} else if (!charstring::compare(name,"row")) {
		return rowTagStart();
	} else if (!charstring::compare(name,"field")) {
		return fieldTagStart();
	}
	return true;
}

bool sqlrimportxml::attributeName(const char *name) {

	// set the current attribute
	delete[] currentattribute;
	currentattribute=charstring::duplicate(name);

	return true;
}

bool sqlrimportxml::attributeValue(const char *value) {

	switch (currenttag) {

		case TABLETAG:

			if (!charstring::compare(currentattribute,"name")) {
				// set the table name
				setObjectName(value);
			}
			break;

		case SEQUENCETAG:

			if (!charstring::compare(currentattribute,"name")) {
				// set the sequence name
				setObjectName(value);
			}

			if (!charstring::compare(currentattribute,"value")) {
				// set the sequence value
				delete[] sequencevalue;
				sequencevalue=charstring::duplicate(value);
			}
			break;

		case COLUMNSTAG:

			if (!charstring::compare(currentattribute,"count")) {
				// set the column count
				colcount=charstring::
					convertToUnsignedInteger(value);

				// update flags and counters
				clearAreNumericColumns();
				setCurrentColumn(0);
			}
			break;

		case COLUMNTAG:

			if (!charstring::compare(currentattribute,"name")) {

				// set the current column name
				cname=charstring::duplicate(value);
				setCurrentColumnName(cname);
				setCurrentField(cname);

			} else if (!charstring::compare(currentattribute,
								"type")) {
				// FIXME: don't do anything with this, I guess?
			}
			break;

		case ROWSTAG:
			break;

		case ROWTAG:
			break;

		case FIELDTAG:
			break;
	}
	return true;
}

bool sqlrimportxml::text(const char *string) {

	// if we're in a field
	if (infield) {
		
		// if this value has a mapping, then get that
		const char	*s=fieldmap.getValue(string);
		if (s) {
			string=s;
		}

		// check for a non-empty field
		// (do this AFTER remapping the field in case some set
		// of values get mapped to empty strings or NULLs)
		if (emptyrecord && !charstring::isNullOrEmpty(string)) {
			emptyrecord=false;
		}

		// determine whether to quote this field
		// FIXME: need currenttablecolumn like in sqlrimportcsv
		// rather than getCurrentColumn()
		bool	isnumeric=getIsNumericColumn(getCurrentColumn());
		bool	isdatetime=getIsDateTimeColumn(getCurrentColumn());
		quotefield[getCurrentColumn()]=!isnumeric;

		// set the current field
		stringbuffer	tmpstr;
		appendField(&tmpstr,string,isnumeric,isdatetime);
		fval=tmpstr.detachString();
		setCurrentField(fval);
	}
	return true;
}

void sqlrimportxml::appendField(stringbuffer *strb,
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
		// FIXME: implement this...
		return;
	}

	// handle normal values
	for (uint32_t index=0; value[index]; index++) {

		if (value[index]=='&') {

			// expand xml entities...

			// skip past the &
			index++;

			// get the number and convert
			// it to a character
			char	ch=(char)charstring::
				convertToUnsignedInteger(value+index);

			// double-up any single-quotes
			if (ch=='\'') {
				strb->append('\'');
			}

			// append the character
			strb->append(ch);

			// skip past the entity
			while (value[index] && value[index]!=';') {
				index++;
			}

		} else if (value[index]=='\\' &&
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

bool sqlrimportxml::tagEnd(const char *ns, const char *name) {

	// call the appropriate tag-end method
	if (!charstring::compare(name,"table")) {
		return tableTagEnd();
	} else if (!charstring::compare(name,"sequence")) {
		return sequenceTagEnd();
	} else if (!charstring::compare(name,"columns")) {
		return columnsTagEnd();
	} else if (!charstring::compare(name,"column")) {
		return columnTagEnd();
	} else if (!charstring::compare(name,"rows")) {
		return rowsTagEnd();
	} else if (!charstring::compare(name,"row")) {
		return rowTagEnd();
	} else if (!charstring::compare(name,"field")) {
		return fieldTagEnd();
	}
	return true;
}

bool sqlrimportxml::tableTagStart() {

	// set the current tag
	currenttag=TABLETAG;

	return true;
}

bool sqlrimportxml::sequenceTagStart() {

	// set the current tag
	currenttag=SEQUENCETAG;

	return true;
}

bool sqlrimportxml::columnsTagStart() {

	// set the current tag
	currenttag=COLUMNSTAG;

	// call the columns-start event
	return columnsStart();
}

bool sqlrimportxml::columnTagStart() {

	// set the current tag
	currenttag=COLUMNTAG;

	// update flags and counters
	cname=NULL;

	// don't call the column-start event yet, we need
	// the attributes to have been processed first
	return true;
}

bool sqlrimportxml::columnTagEnd() {

	// NOTE: atttributeValue() should have called
	// setCurrentColumnName() and setCurrentField() by now

	// call the column-start event
	if (!columnStart()) {
		return false;
	}

	// append the current column
	// (which columnStart() may have overridden)
	if (getCurrentColumnName()!=cname) {
		delete[] cname;
	}
	columns[columns.getCount()]=getCurrentColumnName();

	// reset the current field to the current column name
	// (in case columnStart() override the column name)
	setCurrentField(getCurrentColumnName());

	// call the column-end event
	if (!columnEnd()) {
		return false;
	}

	// next...
	setCurrentColumn(getCurrentColumn()+1);

	return true;
}

bool sqlrimportxml::columnsTagEnd() {

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

bool sqlrimportxml::rowsTagStart() {

	// set the current tag
	currenttag=ROWSTAG;

	// update flags and counters
	setImportedRowCount(0);
	setCurrentColumn(0);
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// begin a transaction (if necessary)
	if (!initialBegin()) {
		return false;
	}

	// call the rows-start event
	return rowsStart();
}

bool sqlrimportxml::rowTagStart() {

	// set the current tag
	currenttag=ROWTAG;

	// update flags and counters
	emptyrecord=true;
	setIgnoreRow(false);
	setCurrentColumn(0);
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the row-start event
	return rowStart();
}

bool sqlrimportxml::fieldTagStart() {

	// set the current tag
	currenttag=FIELDTAG;

	// update flags and counters
	infield=true;
	fval=NULL;

	// don't call the column-start event yet, we need
	// the attributes to have been processed first
	return true;
}

bool sqlrimportxml::fieldTagEnd() {

	// set the current column name
	setCurrentColumnName(columns[getCurrentColumn()]);

	// NOTE: text() should have called setCurrentField() by now

	// call the field-start event
	if (!fieldStart()) {
		return false;
	}

	// append the current field
	// (which fieldStart() may have overridden)
	if (getCurrentField()!=fval) {
		delete[] fval;
	}
	fields[fields.getCount()]=getCurrentField();

	// call the field-end event
	if (!fieldEnd()) {
		return false;
	}

	// next...
	setCurrentColumn(getCurrentColumn()+1);
	infield=false;

	return true;
}

bool sqlrimportxml::rowTagEnd() {
	
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

bool sqlrimportxml::rowsTagEnd() {

	// log
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

bool sqlrimportxml::tableTagEnd() {
	return true;
}

bool sqlrimportxml::sequenceTagEnd() {

	// reset the sequence...
	query.clear();

	// FIXME: arguably restarting a sequence should be an API call, and
	// there should be a backend method like restartSequence() implemented
	// by the connection modules

	// for firebird and interbase...
	if (charstring::contains(getDbType(),"firebird") ||
		charstring::contains(getDbType(),"interbase")) {

		// restart the sequence (generator) with the provided value
		query.append("set generator ")->append(getObjectName());
		query.append(" to ")->append(sequencevalue);
		if (!getSqlrCursor()->sendQuery(query.getString())) {
			if (getLogger()) {
				getLogger()->write(
					getCoarseLogLevel(),NULL,getLogIndent(),
					"%s",getSqlrCursor()->errorMessage());
			}
			return error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage());
		}
		return true;

	} else

	// for oracle...
	if (charstring::contains(getDbType(),"oracle")) {

		// get existing configuration for the sequence
		sqlrcursor	sqlrcur2(getSqlrConnection());
		char		*uppersequence=
				charstring::duplicate(getObjectName());
		charstring::upper(uppersequence);
		query.append("select * from all_sequences where "
				"sequence_name='");
		query.append(uppersequence)->append("'");
		delete[] uppersequence;
		if (!sqlrcur2.sendQuery(query.getString())) {
			if (getLogger()) {
				getLogger()->write(
					getCoarseLogLevel(),NULL,getLogIndent(),
					"%s",sqlrcur2.errorMessage());
			}
			return error(sqlrcur2.errorNumber(),
					sqlrcur2.errorMessage());
		}

		// drop the sequence
		query.clear();
		query.append("drop sequence ")->append(getObjectName());
		if (!getSqlrCursor()->sendQuery(query.getString())) {
			if (getLogger()) {
				getLogger()->write(
					getCoarseLogLevel(),NULL,getLogIndent(),
					"%s",getSqlrCursor()->errorMessage());
			}
			return error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage());
		}

		// recreate the sequence to start with the provided value, but
		// also using configuration parameters that we fetched above
		query.clear();
		query.append("create sequence ")->append(getObjectName());
		query.append(" start with ")->append(sequencevalue);
		query.append(" maxvalue ");
		query.append(sqlrcur2.getField(0,"MAX_VALUE"));
		query.append(" minvalue ");
		query.append(sqlrcur2.getField(0,"MIN_VALUE"));
		if (!charstring::compare(
				sqlrcur2.getField(0,"CYCLE_FLAG"),"N")) {
			query.append(" nocycle ");
		} else {
			query.append(" cycle ");
		}
		if (!charstring::compare(
				sqlrcur2.getField(0,"ORDER_FLAG"),"N")) {
			query.append(" noorder ");
		} else {
			query.append(" order ");
		}
		if (!charstring::compare(
				sqlrcur2.getField(0,"CACHE_SIZE"),"0")) {
			query.append(" nocache ");
		} else {
			query.append(" cache ");
			query.append(sqlrcur2.getField(0,"CACHE_SIZE"));
		}
		if (!getSqlrCursor()->sendQuery(query.getString())) {
			if (getLogger()) {
				getLogger()->write(
					getCoarseLogLevel(),NULL,getLogIndent(),
					"%s",getSqlrCursor()->errorMessage());
			}
			return error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage());
		}
		return true;

	// for postgreql, db2, and informix...
	} else if (charstring::contains(getDbType(),"postgresql") ||
			charstring::contains(getDbType(),"db2") ||
			charstring::contains(getDbType(),"informix")) {

		// restart the sequence with the provided value
		query.append("alter sequence ")->append(getObjectName());
		query.append(" restart with ")->append(sequencevalue);
		if (!getSqlrCursor()->sendQuery(query.getString())) {
			if (getLogger()) {
				getLogger()->write(
					getCoarseLogLevel(),NULL,getLogIndent(),
					"%s",getSqlrCursor()->errorMessage());
			}
			return error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage());
		}
		return true;
	}

	// sqlite, mysql, sap/sybase and mssql have autoincrementing fields
	// odbc can't tell what kind of underlying db we're using, so don't
	// do anything for those databases
	if (getLogger()) {
		getLogger()->write(getCoarseLogLevel(),NULL,getLogIndent(),
				"%s doesn't support sequences",getDbType());
	}

	return true;
}
