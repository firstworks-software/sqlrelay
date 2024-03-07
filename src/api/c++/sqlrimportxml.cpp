// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportxml.h>
#include <rudiments/stdio.h>

const unsigned short sqlrimportxml::NULLTAG=0;
const unsigned short sqlrimportxml::TABLETAG=1;
const unsigned short sqlrimportxml::COLUMNSTAG=2;
const unsigned short sqlrimportxml::COLUMNTAG=3;
const unsigned short sqlrimportxml::ROWSTAG=4;
const unsigned short sqlrimportxml::ROWTAG=5;
const unsigned short sqlrimportxml::FIELDTAG=6;
const unsigned short sqlrimportxml::SEQUENCETAG=7;

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
	cname=NULL;
	infield=false;
	fval=NULL;
	sequencevalue=NULL;
}

sqlrimportxml::~sqlrimportxml() {
	delete[] currentattribute;
	delete[] sequencevalue;
	// do not delete[] cname or fval, as they have either been added to
	// columns[]/fields[] and deleted by them, or deleted manually
	// elsewhere
}

bool sqlrimportxml::importData() {

	// update flags and counters
	clearFlagsAndCounts();

	// set the table name from the file name,
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
	} else if (!charstring::compare(name,"sequence")) {
		return sequenceTagStart();
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
				setObjectName(value);
			}
			break;
		case COLUMNSTAG:
			break;
		case COLUMNTAG:
			if (!charstring::compare(currentattribute,"name")) {
				cname=charstring::duplicate(value);
			}
			break;
		case ROWSTAG:
			break;
		case ROWTAG:
			break;
		case FIELDTAG:
			break;
		case SEQUENCETAG:
			if (!charstring::compare(currentattribute,"name")) {
				setObjectName(value);
			}
			if (!charstring::compare(currentattribute,"value")) {
				delete[] sequencevalue;
				sequencevalue=charstring::duplicate(value);
			}
			break;
	}
	return true;
}

bool sqlrimportxml::text(const char *string) {
	if (infield) {
		fval=charstring::duplicate(string);
	}
	return true;
}

char *sqlrimportxml::unescapeValue(const char *value) {

	stringbuffer	strb;

	for (const char *v=value; *v; v++) {

		if (*v=='&') {

			// expand xml entities...

			// skip past the &
			v++;

			// get the number and convert
			// it to a character
			char	ch=(char)charstring::
					convertToUnsignedInteger(v);

			// append the character
			strb.append(ch);

			// skip past the entity
			while (*v && *v!=';') {
				v++;
			}
		} else {
			strb.append(*v);
		}
	}

	return strb.detachString();
}

bool sqlrimportxml::tagEnd(const char *ns, const char *name) {

	// call the appropriate tag-end method
	if (!charstring::compare(name,"table")) {
		return tableTagEnd();
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
	} else if (!charstring::compare(name,"sequence")) {
		return sequenceTagEnd();
	}
	return true;
}

bool sqlrimportxml::tableTagStart() {

	// set the current tag
	currenttag=TABLETAG;

	return true;
}

bool sqlrimportxml::columnsTagStart() {

	// set the current tag
	currenttag=COLUMNSTAG;

	// NOTE: startProcessingColumns() calls the columns-start event;
	return startProcessingColumns();
}

bool sqlrimportxml::columnTagStart() {

	// set the current tag
	currenttag=COLUMNTAG;

	// reset cname
	cname=NULL;

	// don't call the column-start event yet, we need
	// the attributes to have been processed first
	return true;
}

bool sqlrimportxml::columnTagEnd() {
	// NOTE: atttributeValue() should have set cname by now
	// NOTE: processColumnName() calls the
	// column-start and column-end events
	return processColumnName(&cname);
}

bool sqlrimportxml::columnsTagEnd() {
	// NOTE: endProcessingColumns() calls the columns-end event
	return endProcessingColumns();
}

bool sqlrimportxml::rowsTagStart() {

	// set the current tag
	currenttag=ROWSTAG;

	// NOTE: startProcessingRows() calls the rows-start event
	return startProcessingRows();
}

bool sqlrimportxml::rowTagStart() {

	// set the current tag
	currenttag=ROWTAG;

	// NOTE: startProcessingRow() calls the row-start event
	return startProcessingRow();
}

bool sqlrimportxml::fieldTagStart() {

	// set the current tag
	currenttag=FIELDTAG;

	// we're in a field
	infield=true;

	// reset fval
	fval=NULL;

	// don't call the column-start event yet, we need
	// the attributes to have been processed first
	return true;
}

bool sqlrimportxml::fieldTagEnd() {

	// we're not in a field any more
	infield=false;

	// NOTE: text() should have set fval by now
	// NOTE: processField() calls the field-start and field-end events
	return processField(&fval);
}

bool sqlrimportxml::rowTagEnd() {
	// NOTE: endProcessingRow() calls the row-end event
	return endProcessingRow();
}

bool sqlrimportxml::rowsTagEnd() {
	// NOTE: endProcessingRows() calls the rows-end event
	return endProcessingRows();
}

bool sqlrimportxml::tableTagEnd() {
	return true;
}

bool sqlrimportxml::sequenceTagStart() {

	// set the current tag
	currenttag=SEQUENCETAG;

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
