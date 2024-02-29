// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexporttable.h>
#include <rudiments/dynamicarray.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexporttable::sqlrexporttable() : sqlrexport() {
	exportcon=NULL;
	exportcur=NULL;
	commitcount=0;
}

sqlrexporttable::~sqlrexporttable() {
}

void sqlrexporttable::setExportSqlrConnection(sqlrconnection *exportcon) {
	this->exportcon=exportcon;
}

sqlrconnection *sqlrexporttable::getExportSqlrConnection() {
	return exportcon;
}

void sqlrexporttable::setExportSqlrCursor(sqlrcursor *exportcur) {
	this->exportcur=exportcur;
}

sqlrcursor *sqlrexporttable::getExportSqlrCursor() {
	return exportcur;
}

stringbuffer *sqlrexporttable::getInsertQueryBuffer() {
	return &insertquery;
}

void sqlrexporttable::setCommitCount(uint64_t commitcount) {
	this->commitcount=commitcount;
}

uint64_t sqlrexporttable::getCommitCount() {
	return commitcount;
}

bool sqlrexporttable::exportData() {

	clearFlagsAndCounts();

	// capture the con, cur, table and commit count
	sqlrconnection	*exportcon=getExportSqlrConnection();
	sqlrcursor	*exportcur=getExportSqlrCursor();
	const char	*table=getTable();

	// sanity checks
	if (!exportcon) {
		return error(
			0,"No connection set with setExportSqlrConnection()");
	}
	if (!exportcur) {
		return error(
			0,"No connection set with setExportSqlrCursor()");
	}
	if (!table) {
		return error(0,"No table set with setTable()");
	}

	// get the cursor and column count
	sqlrcursor	*sqlrcur=getSqlrCursor();
	uint32_t	cols=sqlrcur->colCount();

	// set initial column/field
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(getCurrentColumnName());

	// determine numeric columns
	for (uint32_t i=0; i<cols; i++) {
		setIsNumericColumn(
			i,isNumberTypeChar(sqlrcur->getColumnType(i)));
	}

	// call the export-start event
	if (!exportStart()) {
		return false;
	}

	// call the columns-start event
	if (!columnsStart()) {
		return false;
	}

	stringbuffer	*insertquery=getInsertQueryBuffer();
	insertquery->append("insert into ")->append(table)->append(" values (");
	const char	*bindformat=exportcon->bindFormat();
	char		bf=(!charstring::isNullOrEmpty(bindformat))?
							bindformat[0]:':';

	// export bind variables...
	uint32_t	bindindex=1;
	for (setCurrentColumn(0);
			getCurrentColumn()<cols;
			setCurrentColumn(getCurrentColumn()+1)) {

		// set the current column (and field)
		setCurrentColumnName(
			sqlrcur->getColumnName(getCurrentColumn()));
		setCurrentField(getCurrentColumnName());

		// call the column-start event
		if (!columnStart()) {
			return false;
		}

		// reset the current field to the current column name too
		// (in case columnStart overrode the columm name)
		setCurrentField(getCurrentColumnName());

		// if we're not ignoring this column...
		if (!charstring::isInSet(getCurrentField(),
						getColumnsToIgnore())) {

			// append the bind variable
			if (bindindex>1) {
				insertquery->append(',');
			}
			if (bf=='?') {
				insertquery->append('?');
			} else if (bf=='$') {
				insertquery->append('$')->append(bindindex);
			} else if (bf=='@' || bf==':') {
				insertquery->append(bf)->append(bindindex);
			}
			bindindex++;
		}

		// call the column-end event
		if (!columnEnd()) {
			return false;
		}
	}

	// set the current column and field to NULL
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the columns-end event
	// (we call this before closing the columns in case an overridden
	// columnsEnd() wants to add more columns or something)
	if (!columnsEnd()) {
		return false;
	}

	insertquery->append(')');

	// reset current column/field
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(sqlrcur->getField(0,(uint32_t)0));

	// call the rows-start event
	if (!rowsStart()) {
		return false;
	}

	// start a transaction, if necessary
	if (getCommitCount()) {
		if (!beginStart()) {
			return false;
		}
		if (!exportcon->begin()) {
			if (!error(exportcon->errorNumber(),
					exportcon->errorMessage())) {
				return false;
			}
		}
		if (!beginEnd()) {
			return false;
		}
		
	}

	// prepare query
	exportcur->prepareQuery(insertquery->getString(),
				insertquery->getStringLength());

	// set up array of bind names
	dynamicarray<char *>	bindnames;
	bindnames.setManageArrayValues(true);

	// export rows...
	bool	success=true;
	bool	first=true;
	do {

		// commit/begin, if necessary
		if (getCommitCount() &&
			!((getCurrentRow()+1)%getCommitCount())) {
			if (!commitStart()) {
				return false;
			}
			if (!exportcon->commit()) {
				if (!error(exportcon->errorNumber(),
						exportcon->errorMessage())) {
					return false;
				}
			}
			if (!commitEnd()) {
				return false;
			}
			if (!beginStart()) {
				return false;
			}
			if (!exportcon->begin()) {
				if (!error(exportcon->errorNumber(),
						exportcon->errorMessage())) {
					return false;
				}
			}
			if (!beginEnd()) {
				return false;
			}
		}

		// reset export-row flag and current column/field
		setIgnoreRow(false);
		setCurrentColumn(0);
		setCurrentColumnName(sqlrcur->getColumnName(0));
		setCurrentField(sqlrcur->getField(getCurrentRow(),(uint32_t)0));

		// call the row-start event
		if (!rowStart()) {
			success=false;
			break;
		}

		// reset bind index
		bindindex=0;

		for (setCurrentColumn(0);
				getCurrentColumn()<cols;
				setCurrentColumn(getCurrentColumn()+1)) {

			// set the current column and field
			setCurrentColumnName(
				sqlrcur->getColumnName(getCurrentColumn()));
			setCurrentField(sqlrcur->getField(
						getCurrentRow(),
						getCurrentColumn()));
			if (!getCurrentField()) {
				break;
			}

			// call the field-start event
			if (!fieldStart()) {
				success=false;
				break;
			}

			// if we're not ignoring this row or column...
			if (!getIgnoreRow() &&
				!charstring::isInSet(
					sqlrcur->getColumnName(
						getCurrentColumn()),
					getColumnsToIgnore())) {

				// export the field
				if (first) {
					bindnames[bindindex]=
					charstring::parseNumber(bindindex+1);
				}
				exportcur->inputBind(
					bindnames[bindindex],
					getCurrentField());
				bindindex++;
			}

			// call the field-end event
			if (!fieldEnd()) {
				success=false;
				break;
			}
		}

		if (!success) {
			break;
		}

		if (!getIgnoreRow()) {
			// It's not impossible that there were 0 columns in
			// this result set.  If that was the case then
			// bindindex should still be 0 at this point, and no
			// values should be bound.  In that case, we don't want
			// to attempt to execute anything.
			if (bindindex) {
				if (!exportcur->executeQuery()) {
					if (!error(
						exportcur->errorNumber(),
						exportcur->errorMessage())) {
						success=false;
						break;
					}
				}
			}
			exportcur->clearBinds();
			first=false;
		}

		// set the current column and field to NULL
		setCurrentColumnName(NULL);
		setCurrentField(NULL);

		// call the row-end event
		if (!rowEnd()) {
			success=false;
			break;
		}

		// update exported row count
		if (!getIgnoreRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}

		// update current row
		setCurrentRow(getCurrentRow()+1);

	} while  (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount());

	// final commit, if necessary
	if (getCommitCount()) {
		if (!commitStart()) {
			return false;
		}
		if (!exportcon->commit()) {
			if (!error(exportcon->errorNumber(),
					exportcon->errorMessage())) {
				return false;
			}
		}
		if (!commitEnd()) {
			return false;
		}
	}

	// call the rows-end event
	if (!rowsEnd()) {
		return false;
	}

	// call the export-end event
	if (!exportEnd()) {
		return false;
	}

	return success;
}

void sqlrexporttable::clearFlagsAndCounts() {
	sqlrexport::clearFlagsAndCounts();
	insertquery.clear();
}
