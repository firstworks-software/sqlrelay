// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexporttable.h>
#include <rudiments/dynamicarray.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexporttable::sqlrexporttable() {
}

sqlrexporttable::~sqlrexporttable() {
}

bool sqlrexporttable::exportToTable(sqlrconnection *sqlrcon,
						const char *table) {

	// reset flags
	setExportRow(true);
	setCurrentRow(0);
	setCurrentColumn(0);
	setCurrentField(NULL);

	// call the pre-header event
	if (!headerStart()) {
		return false;
	}

	sqlrcursor	*selectcur=getSqlrCursor();
	const char * const *fieldstoignore=getFieldsToIgnore();

	stringbuffer	insertquery;
	insertquery.append("insert into ")->append(table)->append(" values (");
	const char	*bindformat=sqlrcon->bindFormat();
	char		bf=(!charstring::isNullOrEmpty(bindformat))?
							bindformat[0]:':';

	// export header
	uint32_t	cols=selectcur->colCount();
	clearNumberColumns();
	uint32_t	bindindex=0;
	for (setCurrentColumn(0);
		getCurrentColumn()<cols;
		setCurrentColumn(getCurrentColumn()+1)) {

		if (getCurrentColumn()) {
			insertquery.append(',');
		}

		setNumberColumn(getCurrentColumn(),
			isNumberTypeChar(selectcur->getColumnType(
						getCurrentColumn())));

		setCurrentField(selectcur->getColumnName(getCurrentColumn()));
		if (charstring::isInSet(getCurrentField(),fieldstoignore)) {
			continue;
		}

		if (!getIgnoreColumns()) {

			// call the pre-column event
			if (!columnStart()) {
				return false;
			}

			if (bf=='?') {
				insertquery.append('?');
			} else if (bf=='$') {
				insertquery.append('$')->append(bindindex);
			} else if (bf=='@' || bf==':') {
				insertquery.append(bf)->append(bindindex);
			}
			bindindex++;

			// call the post-column event
			if (!columnEnd()) {
				return false;
			}
		}
	}

	insertquery.append(')');

	// call the post-header event
	// (we call this before closing the header in case an overridden
	// headerEnd() wants to add more columns or something)
	if (!headerEnd()) {
		return false;
	}

	// call the pre-rows event
	if (!rowsStart()) {
		return false;
	}

	sqlrcursor	*insertcur=new sqlrcursor(sqlrcon);
	insertcur->prepareQuery(insertquery.getString(),
				insertquery.getStringLength());
	dynamicarray<char *>	bindnames;
	bindnames.setManageArrayValues(true);
	bindindex=0;

	// export rows...
	do {

		// reset export-row flag
		setExportRow(true);

		// call the pre-row event
		if (!rowStart()) {
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (getExportRow()) {

			for (setCurrentColumn(0);
				getCurrentColumn()<cols;
				setCurrentColumn(getCurrentColumn()+1)) {

				// ignore particular fields
				if (fieldstoignore) {
					if (charstring::isInSet(
						selectcur->getColumnName(
							getCurrentColumn()),
						fieldstoignore)) {
						continue;
					}
				}

				// get the field
				setCurrentField(selectcur->getField(
							getCurrentRow(),
							getCurrentColumn()));
				if (!getCurrentField()) {
					break;
				}

				// call the pre-field event
				if (!fieldStart()) {
					return false;
				}

				if (!bindnames[bindindex]) {
					bindnames[bindindex]=
					charstring::parseNumber(bindindex);
				}
				insertcur->inputBind(
					bindnames[bindindex],
					getCurrentField());
				bindindex++;

				// call the post-field event
				if (!fieldEnd()) {
					return false;
				}
			}
		}

		if (!insertcur->executeQuery()) {
			return false;
		}

		// call the post-row event
		// (we call this before closing the row in case an overridden
		// rowEnd() wants to add more fields or something)
		if (!rowEnd()) {
			return false;
		}

		setCurrentRow(getCurrentRow()+1);

	} while  (!selectcur->endOfResultSet() ||
			getCurrentRow()<selectcur->rowCount());

	// call the post-rows event
	if (!rowsEnd()) {
		return false;
	}

	return true;
}
