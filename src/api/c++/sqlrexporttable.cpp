// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexporttable.h>
#include <rudiments/dynamicarray.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexporttable::sqlrexporttable() {
	commitcount=0;
}

sqlrexporttable::~sqlrexporttable() {
}

void sqlrexporttable::setCommitCount(uint64_t commitcount) {
	this->commitcount=commitcount;
}

uint64_t sqlrexporttable::getCommitCount() {
	return commitcount;
}

bool sqlrexporttable::exportToTable(sqlrconnection *sqlrcon,
						sqlrcursor *sqlrcur,
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
	const char * const *columnstoignore=getColumnsToIgnore();

	stringbuffer	insertquery;
	insertquery.append("insert into ")->append(table)->append(" values (");
	const char	*bindformat=sqlrcon->bindFormat();
	char		bf=(!charstring::isNullOrEmpty(bindformat))?
							bindformat[0]:':';

	// export header
	uint32_t	cols=selectcur->colCount();
	clearNumberColumns();
	uint32_t	bindindex=1;
	for (setCurrentColumn(0);
		getCurrentColumn()<cols;
		setCurrentColumn(getCurrentColumn()+1)) {

		setNumberColumn(getCurrentColumn(),
			isNumberTypeChar(selectcur->getColumnType(
						getCurrentColumn())));

		setCurrentField(selectcur->getColumnName(getCurrentColumn()));
		if (charstring::isInSet(getCurrentField(),columnstoignore)) {
			continue;
		}

		if (!getIgnoreColumns()) {

			// call the pre-column event
			if (!columnStart()) {
				return false;
			}

			if (bindindex>1) {
				insertquery.append(',');
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

	// start a transaction, if necessary
	if (commitcount) {
		sqlrcon->begin();
	}

	// prepare query
	sqlrcur->prepareQuery(insertquery.getString(),
				insertquery.getStringLength());

	// set up array of bind names
	dynamicarray<char *>	bindnames;
	bindnames.setManageArrayValues(true);

	// export rows...
	bool	success=true;
	do {

		// commit/begin, if necessary
		if (commitcount && !((getCurrentRow()+1)%500)) {
			sqlrcon->commit();
			sqlrcon->begin();
		}

		// reset export-row flag
		setExportRow(true);

		// call the pre-row event
		if (!rowStart()) {
			success=false;
			break;
		}

		// reset bind index
		bindindex=1;

		// if rowStart() didn't disable export of this row...
		if (getExportRow()) {

			for (setCurrentColumn(0);
				getCurrentColumn()<cols;
				setCurrentColumn(getCurrentColumn()+1)) {

				// ignore particular fields
				if (columnstoignore) {
					if (charstring::isInSet(
						selectcur->getColumnName(
							getCurrentColumn()),
						columnstoignore)) {
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
					success=false;
					break;
				}

				if (!bindnames[bindindex]) {
					bindnames[bindindex]=
					charstring::parseNumber(bindindex);
				}
				sqlrcur->inputBind(
					bindnames[bindindex],
					getCurrentField());
				bindindex++;

				// call the post-field event
				if (!fieldEnd()) {
					success=false;
					break;
				}
			}
		}

		if (!success) {
			break;
		}

		// It's not impossible that there were 0 rows in this result
		// set.  If that was the case then bindindex should still be 1
		// at this point, and no values should be bound.  In that case,
		// we don't want to attempt to execute anything.
		if (bindindex>1) {
			if (!sqlrcur->executeQuery()) {
				success=false;
				break;
			}
		}
		sqlrcur->clearBinds();

		// call the post-row event
		// (we call this before closing the row in case an overridden
		// rowEnd() wants to add more fields or something)
		if (!rowEnd()) {
			success=false;
			break;
		}

		setCurrentRow(getCurrentRow()+1);

	} while  (!selectcur->endOfResultSet() ||
			getCurrentRow()<selectcur->rowCount());

	// final commit, if necessary
	if (commitcount) {
		sqlrcon->commit();
	}

	// call the post-rows event
	if (!rowsEnd()) {
		return false;
	}

	return success;
}
