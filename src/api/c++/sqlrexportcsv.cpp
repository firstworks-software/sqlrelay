// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexportcsv.h>
#include <rudiments/file.h>
#include <rudiments/filesystem.h>
#include <rudiments/permissions.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexportcsv::sqlrexportcsv() : sqlrexportfile() {
}

sqlrexportcsv::~sqlrexportcsv() {
}

bool sqlrexportcsv::exportData() {

	clearFlagsAndCounts();

	// output to stdoutput or create/open file
	setFileDescriptor(&stdoutput);
	file	f;
	if (!charstring::isNullOrEmpty(getFileName())) {
		if (!f.create(getFileName(),
			permissions::parsePermString("rw-r--r--"))) {
			return systemError();
		}
		f.setWriteBufferSize(
			filesystem::getOptimumTransferBlockSize(getFileName()));
		setFileDescriptor(&f);
	}
	filedescriptor	*fd=getFileDescriptor();

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
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	// call the columns-start event
	if (!columnsStart()) {
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	// export columns...
	bool	first=true;
	for (setCurrentColumn(0);
			getCurrentColumn()<cols;
			setCurrentColumn(getCurrentColumn()+1)) {

		// set the current column name (and field)
		setCurrentColumnName(
			sqlrcur->getColumnName(getCurrentColumn()));
		setCurrentField(getCurrentColumnName());

		// call the column-start event
		if (!columnStart()) {
			fd->flushWriteBuffer(-1,-1);
			return false;
		}

		// reset the current field to the current column name too
		// (in case columnStart overrode the columm name)
		setCurrentField(getCurrentColumnName());

		// if we're not excluding all columns or this column...
		if (!getExcludeColumns() &&
			!charstring::isInSet(getCurrentField(),
						getColumnsToExclude())) {

			// export the column name
			if (first) {
				first=false;
			} else {
				if (fd->write(',')!=sizeof(char)) {
					return systemError();
				}
			}
			bool	isnumber=
				charstring::isNumber(getCurrentField());
			if (!isnumber) {
				if (fd->write('"')!=sizeof(char)) {
					return systemError();
				}
			}
			if (!escapeField(fd,getCurrentField())) {
				return false;
			}
			if (!isnumber) {
				if (fd->write('"')!=sizeof(char)) {
					return systemError();
				}
			}
		}

		// call the column-end event
		if (!columnEnd()) {
			fd->flushWriteBuffer(-1,-1);
			return false;
		}
	}

	// set the current column and field to NULL
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the columns-end event
	// (we call this before closing the columns in case an
	// overridden columnsEnd() wants to add more columns or
	// something)
	if (!columnsEnd()) {
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	if (!getExcludeColumns()) {
		if (fd->write('\n')!=sizeof(char)) {
			return systemError();
		}
	}

	// reset current column/field
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(sqlrcur->getField(0,(uint32_t)0));

	// call the rows-start event
	if (!rowsStart()) {
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	// export rows...
	while (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount()) {

		// reset export-row flag and current column/field
		setExcludeRow(false);
		setCurrentColumn(0);
		setCurrentColumnName(sqlrcur->getColumnName(0));
		setCurrentField(sqlrcur->getField(getCurrentRow(),(uint32_t)0));

		// call the row-start event
		if (!rowStart()) {
			fd->flushWriteBuffer(-1,-1);
			return false;
		}

		bool	first=true;
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
				fd->flushWriteBuffer(-1,-1);
				return false;
			}

			// if we're not excluding this row or column...
			if (!getExcludeRow() &&
				!charstring::isInSet(
					sqlrcur->getColumnName(
						getCurrentColumn()),
					getColumnsToExclude())) {

				// we need to quote the field if it's not a
				// number, or if it is a number, but has more
				// than 12 digits.  Excel (and presumably other
				// spreadsheet apps) likes to convert 12+
				// digit numbers to scientific notation.
				bool	quote=
					(!getIsNumericColumn(
						getCurrentColumn()) ||
					charstring::getLength(
						getCurrentField())>=12);

				// export the field
				if (first) {
					first=false;
				} else {
					if (fd->write(',')!=sizeof(char)) {
						return systemError();
					}
				}
				if (quote) {
					if (fd->write('"')!=sizeof(char)) {
						return systemError();
					}
				}
				if (!escapeField(fd,getCurrentField())) {
					return false;
				}
				if (quote) {
					if (fd->write('"')!=sizeof(char)) {
						return systemError();
					}
				}
			}

			// call the field-end event
			if (!fieldEnd()) {
				fd->flushWriteBuffer(-1,-1);
				return false;
			}
		}

		// set the current column and field to NULL
		setCurrentColumnName(NULL);
		setCurrentField(NULL);

		// call the row-end event
		// (we call this before closing the row in case an overridden
		// rowEnd() wants to add more fields or something)
		if (!rowEnd()) {
			fd->flushWriteBuffer(-1,-1);
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (!getExcludeRow()) {
			if (fd->write('\n')!=sizeof(char)) {
				return systemError();
			}
		}

		// update exported row count
		if (!getExcludeRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}

		// update current row
		setCurrentRow(getCurrentRow()+1);
	}

	// call the rows-end event
	if (!rowsEnd()) {
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	// call the export-end event
	if (!exportEnd()) {
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	fd->flushWriteBuffer(-1,-1);
	return true;
}

bool sqlrexportcsv::escapeField(filedescriptor *fd, const char *field) {
	if (!field) {
		return true;
	}
	for (const char *f=field; *f; f++) {
		// escape double quotes
		if (*f=='"') {
			if (fd->write("\"\"")!=2) {
				return systemError();
			}
		} else {
			if (fd->write(*f)!=sizeof(char)) {
				return systemError();
			}
		}
	}
	return true;
}
