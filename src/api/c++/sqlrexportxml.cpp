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

	// reset flags
	setExportRow(true);
	setCurrentRow(0);
	setCurrentColumn(0);
	setCurrentField(NULL);

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

	// export xml header
	fd->write("<?xml version=\"1.0\"?>\n");

	// export table name
	if (!charstring::isNullOrEmpty(table)) {
		fd->write("<table name=\"");
		escapeField(fd,table);
		fd->write("\">\n");
	}

	// call the pre-columns event
	if (!columnsStart()) {
		return false;
	}

	sqlrcursor	*sqlrcur=getSqlrCursor();
	const char * const *columnstoignore=getColumnsToIgnore();

	// export columns...
	uint32_t	cols=sqlrcur->colCount();
	if (!getIgnoreColumns()) {
		fd->printf("<columns count=\"%d\">\n",cols);
	}
	clearNumberColumns();
	for (setCurrentColumn(0);
		getCurrentColumn()<cols;
		setCurrentColumn(getCurrentColumn()+1)) {

		// set whether this is a numeric column or not
		setNumberColumn(getCurrentColumn(),
			isNumberTypeChar(sqlrcur->getColumnType(
						getCurrentColumn())));

		// set the current field
		setCurrentField(sqlrcur->getColumnName(getCurrentColumn()));

		// ignore particular columns
		if (charstring::isInSet(getCurrentField(),columnstoignore)) {
			continue;
		}

		if (!getIgnoreColumns()) {

			// call the pre-column event
			if (!columnStart()) {
				return false;
			}

			// export the column name and type
			fd->write("	<column name=\"");
			escapeField(fd,getCurrentField());
			fd->write("\" type=\"");
			escapeField(fd,sqlrcur->getColumnType(
						getCurrentField()));
			fd->write("\"/>\n");

			// call the post-column event
			if (!columnEnd()) {
				return false;
			}
		}
	}

	// call the post-columns event
	// (we call this before closing the columns in case an overridden
	// columnsEnd() wants to add more columns or something)
	if (!columnsEnd()) {
		return false;
	}

	if (!getIgnoreColumns()) {
		fd->write("</columns>\n");
	}

	// call the pre-rows event
	if (!rowsStart()) {
		return false;
	}

	// export rows
	fd->write("<rows>\n");
	while (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount()) {

		// reset export-row flag
		setExportRow(true);

		// call the pre-row event
		if (!rowStart()) {
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (getExportRow()) {

			fd->write("	<row>\n");

			for (setCurrentColumn(0);
				getCurrentColumn()<cols;
				setCurrentColumn(getCurrentColumn()+1)) {

				// ignore particular columns
				if (columnstoignore) {
					if (charstring::isInSet(
						sqlrcur->getColumnName(
							getCurrentColumn()),
						columnstoignore)) {
						continue;
					}
				}

				// get the field
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

				// export the field
				fd->write("	<field>");
				escapeField(fd,getCurrentField());
				fd->write("</field>\n");

				// call the post-field event
				if (!fieldEnd()) {
					return false;
				}
			}
		}

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
