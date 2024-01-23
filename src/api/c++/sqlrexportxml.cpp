// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexportxml.h>
#include <rudiments/filedescriptor.h>
#include <rudiments/file.h>
#include <rudiments/permissions.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexportxml::sqlrexportxml() {
}

sqlrexportxml::~sqlrexportxml() {
}

bool sqlrexportxml::exportToFile(const char *filename, const char *table) {

	if (!sqlrexport::exportToFile(filename,table)) {
		return false;
	}

	// output to stdoutput or create/open file
	setFileDescriptor(&stdoutput);
	file	f;
	if (!charstring::isNullOrEmpty(filename)) {
		if (!f.create(filename,
			permissions::parsePermString("rw-r--r--"))) {
			// FIXME: report error
			return false;
		}
		setFileDescriptor(&f);
	}
	filedescriptor	*fd=getFileDescriptor();

	// get the cursor and column count
	sqlrcursor	*sqlrcur=getSqlrCursor();
	uint32_t	cols=sqlrcur->colCount();

	// reset flags and counts
	setExportRow(true);
	setExportedRowCount(0);
	setCurrentRow(0);
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(getCurrentColumnName());
	clearAreNumericColumns();

	// export xml header
	fd->write("<?xml version=\"1.0\"?>\n");

	// export table name
	if (!charstring::isNullOrEmpty(table)) {
		fd->write("<table name=\"");
		escapeField(fd,table);
		fd->write("\">\n");
	}

	// determine numeric columns
	for (uint32_t i=0; i<cols; i++) {
		setIsNumericColumn(
			i,isNumberTypeChar(sqlrcur->getColumnType(i)));
	}

	// call the pre-columns event
	if (!columnsStart()) {
		return false;
	}

	// export columns...
	if (!getIgnoreColumns()) {
		fd->printf("<columns count=\"%d\">\n",cols);
	}
	for (setCurrentColumn(0);
			getCurrentColumn()<cols;
			setCurrentColumn(getCurrentColumn()+1)) {

		// set the current column (and field)
		setCurrentColumnName(
			sqlrcur->getColumnName(getCurrentColumn()));
		setCurrentField(getCurrentColumnName());
	
		// call the pre-column event
		if (!columnStart()) {
			return false;
		}

		// if we're not ignoring all columns, or this column...
		if (!getIgnoreColumns() &&
			!charstring::isInSet(getCurrentField(),
						getColumnsToIgnore())) {

			// export the column name and type
			fd->write("	<column name=\"");
			escapeField(fd,getCurrentField());
			fd->write("\" type=\"");
			escapeField(fd,sqlrcur->getColumnType(
						getCurrentField()));
			fd->write("\"/>\n");
		}

		// call the post-column event
		if (!columnEnd()) {
			return false;
		}
	}

	// set the current column and field to NULL
	setCurrentColumnName(NULL);
	setCurrentField(NULL);

	// call the post-columns event
	// (we call this before closing the columns in case an
	// overridden columnsEnd() wants to add more columns or
	// something)
	if (!columnsEnd()) {
		return false;
	}

	if (!getIgnoreColumns()) {
		fd->write("</columns>\n");
	}

	// reset current column/field
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(sqlrcur->getField(0,(uint32_t)0));

	// call the pre-rows event
	if (!rowsStart()) {
		return false;
	}

	// export rows
	fd->write("<rows>\n");
	while (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount()) {

		// reset export-row flag and current column/field
		setExportRow(true);
		setCurrentColumn(0);
		setCurrentColumnName(sqlrcur->getColumnName(0));
		setCurrentField(sqlrcur->getField(getCurrentRow(),(uint32_t)0));

		// call the pre-row event
		if (!rowStart()) {
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (getExportRow()) {
			fd->write("	<row>\n");
		}

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
				return false;
			}

			// if we're not ignoring this row or column...
			if (getExportRow() &&
				!charstring::isInSet(
					sqlrcur->getColumnName(
						getCurrentColumn()),
				getColumnsToIgnore())) {

				// export the field
				fd->write("	<field>");
				escapeField(fd,getCurrentField());
				fd->write("</field>\n");
			}

			// call the post-field event
			if (!fieldEnd()) {
				return false;
			}
		}

		// set the current column and field to NULL
		setCurrentColumnName(NULL);
		setCurrentField(NULL);

		// call the post-row event
		// (we call this before closing the row in case an overridden
		// rowEnd() wants to add more fields or something)
		if (!rowEnd()) {
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (getExportRow()) {
			fd->write("	</row>\n");
		}

		// update exported row count
		if (getExportRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}

		// update current row
		setCurrentRow(getCurrentRow()+1);
	}

	// call the post-rows event
	// (we call this before closing the rows in case an overridden
	// rowsEnd() wants to add more rows or something)
	if (!rowsEnd()) {
		return false;
	}

	fd->write("</rows>\n");
	fd->write("</table>\n");

	return true;
}

void sqlrexportxml::escapeField(filedescriptor *fd, const char *field) {
	for (const char *f=field; *f; f++) {
		if (*f=='"') {
			fd->write("\"\"");
		} else if (*f<' ' || *f>'~' || *f=='&' || *f=='<' || *f=='>') {
			fd->printf("&%d;",(uint8_t)*f);
		} else {
			fd->write(*f);
		}
	}
}
