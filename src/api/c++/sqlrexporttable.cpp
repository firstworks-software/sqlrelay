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

bool sqlrexporttable::exportToTable(sqlrconnection *exportcon,
						const char *table,
						uint64_t commitcount) {
	return sqlrexport::exportToTable(exportcon,table,commitcount);
}

bool sqlrexporttable::exportToTable(sqlrconnection *exportcon,
						sqlrcursor *exportcur,
						const char *table,
						uint64_t commitcount) {

	if (!sqlrexport::exportToTable(exportcon,exportcur,table,commitcount)) {
		return false;
	}

	// capture the con, cur, table and commit count
	setExportSqlrConnection(exportcon);
	setExportSqlrCursor(exportcur);
	setTable(table);
	setCommitCount(commitcount);

	// get the cursor and column count
	sqlrcursor	*sqlrcur=getSqlrCursor();
	uint32_t	cols=sqlrcur->colCount();

	// reset flags and counts
	setIgnoreRow(false);
	setExportedRowCount(0);
	setCurrentRow(0);
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(getCurrentColumnName());
	clearAreNumericColumns();

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
				if (!bindnames[bindindex]) {
					bindnames[bindindex]=
					charstring::parseNumber(bindindex);
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
			// bindindex should still be 1 at this point, and no
			// values should be bound.  In that case, we don't want
			// to attempt to execute anything.
			if (bindindex>1) {
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
