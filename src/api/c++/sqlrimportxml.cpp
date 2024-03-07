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

sqlrimportxml::sqlrimportxml() : sqlrimportfile(), xmlsax() {
	currenttag=NULLTAG;
	currentattribute=NULL;
	infield=false;
	sequencevalue=NULL;
	setExtension(".xml");
}

sqlrimportxml::~sqlrimportxml() {
	delete[] currentattribute;
	delete[] sequencevalue;
}

bool sqlrimportxml::importData() {

	if (!sqlrimportfile::importData()) {
		return false;
	}

	// NOTE: startProcessingImport() calls the import-start event
	// NOTE: endProcessingImport() calls the import-end event
	return startProcessingImport() &&
		xmlsax::parseFile(getFileName()) &&
		endProcessingImport();
}

bool sqlrimportxml::tagStart(const char *ns, const char *name) {

	if (!charstring::compare(name,"table")) {
		currenttag=TABLETAG;
		return true;
	} else if (!charstring::compare(name,"columns")) {
		currenttag=COLUMNSTAG;
		// NOTE: startProcessingColumns() calls the columns-start event;
		return startProcessingColumns();
	} else if (!charstring::compare(name,"column")) {
		currenttag=COLUMNTAG;
		// clear the column name buffer
		clearColumnNameBuffer();
		// don't call the column-start event yet, we need
		// the attributes to have been processed first
		return true;
	} else if (!charstring::compare(name,"rows")) {
		currenttag=ROWSTAG;
		// NOTE: startProcessingRows() calls the rows-start event
		return startProcessingRows();
	} else if (!charstring::compare(name,"row")) {
		currenttag=ROWTAG;
		// NOTE: startProcessingRow() calls the row-start event
		return startProcessingRow();
	} else if (!charstring::compare(name,"field")) {
		currenttag=FIELDTAG;
		// we're in a field
		infield=true;
		// clear the field buffer
		clearFieldBuffer();
		// don't call the column-start event yet, we need
		// the attributes to have been processed first
		return true;
	} else if (!charstring::compare(name,"sequence")) {
		currenttag=SEQUENCETAG;
		return true;
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
				setColumnNameBuffer(value);
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
		setFieldBuffer(string);
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

	if (!charstring::compare(name,"column")) {
		// NOTE: atttributeValue() should have set
		// the column name buffer by now
		// NOTE: processColumnName() calls the
		// column-start and column-end events
		return processColumnName();
	} else if (!charstring::compare(name,"columns")) {
		// NOTE: endProcessingColumns() calls the columns-end event
		return endProcessingColumns();
	} else if (!charstring::compare(name,"field")) {
		// we're not in a field any more
		infield=false;
		// NOTE: text() should have set the field buffer by now
		// NOTE: processField() calls the
		// field-start and field-end events
		return processField();
	} else if (!charstring::compare(name,"row")) {
		// NOTE: endProcessingRow() calls the row-end event
		return endProcessingRow();
	} else if (!charstring::compare(name,"rows")) {
		// NOTE: endProcessingRows() calls the rows-end event
		return endProcessingRows();
	} else if (!charstring::compare(name,"table")) {
		return true;
	} else if (!charstring::compare(name,"sequence")) {
		return resetSequence();
	}
	return true;
}

bool sqlrimportxml::resetSequence() {

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
			return error(sqlrcur2.errorNumber(),
					sqlrcur2.errorMessage());
		}

		// drop the sequence
		query.clear();
		query.append("drop sequence ")->append(getObjectName());
		if (!getSqlrCursor()->sendQuery(query.getString())) {
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
			return error(getSqlrConnection()->errorNumber(),
					getSqlrConnection()->errorMessage());
		}
		return true;
	}

	// sqlite, mysql, sap/sybase and mssql have autoincrementing fields
	// odbc can't tell what kind of underlying db we're using, so don't
	// do anything for those databases
	return true;
}
