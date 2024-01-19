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

	// get the cursor and column count
	sqlrcursor	*selectcur=getSqlrCursor();
	uint32_t	cols=selectcur->colCount();

	// reset flags and counts
	setExportRow(true);
	setExportedRowCount(0);
	setCurrentRow(0);
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(getCurrentColumn()));
	setCurrentField(getCurrentColumnName());
	clearNumberColumns();

	// call the pre-columns event
	if (!columnsStart()) {
		return false;
	}

	stringbuffer	insertquery;
	insertquery.append("insert into ")->append(table)->append(" values (");
	const char	*bindformat=sqlrcon->bindFormat();
	char		bf=(!charstring::isNullOrEmpty(bindformat))?
							bindformat[0]:':';

	// export bind variables...
	uint32_t	bindindex=1;
	for (uint32_t i=0; i<cols; i++) {

		// set the current column (and field)
		setCurrentColumn(i);
		setCurrentColumnName(
			sqlrcur->getColumnName(getCurrentColumn()));
		setCurrentField(getCurrentColumnName());

		// set whether this is a numeric column or not
		setNumberColumn(getCurrentColumn(),
			isNumberTypeChar(selectcur->getColumnType(
						getCurrentColumn())));

		// call the pre-column event
		if (!columnStart()) {
			return false;
		}

		// if we're not ignoring this column...
		if (!charstring::isInSet(getCurrentField(),
						getColumnsToIgnore())) {

			// append the bind variable
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
		}

		// call the post-column event
		if (!columnEnd()) {
			return false;
		}
	}

	// call the post-columns event
	// (we call this before closing the columns in case an overridden
	// columnsEnd() wants to add more columns or something)
	if (!columnsEnd()) {
		return false;
	}

	insertquery.append(')');

	// reset current column/field
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(getCurrentColumn()));
	setCurrentField(sqlrcur->getField(getCurrentRow(),getCurrentColumn()));

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
		if (commitcount && !((getCurrentRow()+1)%commitcount)) {
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

		for (uint32_t i=0; i<cols; i++) {

			// set the current column and field
			setCurrentColumn(i);
			setCurrentColumnName(
				sqlrcur->getColumnName(getCurrentColumn()));
			setCurrentField(sqlrcur->getField(
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

			// if we're not ignoring this row or column...
			if (getExportRow() &&
				!charstring::isInSet(
					selectcur->getColumnName(
						getCurrentColumn()),
					getColumnsToIgnore())) {

				// export the field
				if (!bindnames[bindindex]) {
					bindnames[bindindex]=
					charstring::parseNumber(bindindex);
				}
				sqlrcur->inputBind(
					bindnames[bindindex],
					getCurrentField());
				bindindex++;
			}

			// call the post-field event
			if (!fieldEnd()) {
				success=false;
				break;
			}
		}

		if (!success) {
			break;
		}

		// It's not impossible that there were 0 columns in this result
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
		if (!rowEnd()) {
			success=false;
			break;
		}

		// update counts and currents
		if (getExportRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}
		setCurrentRow(getCurrentRow()+1);
		setCurrentColumn(0);
		setCurrentColumnName(
			sqlrcur->getColumnName(getCurrentColumn()));
		setCurrentField(NULL);

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
