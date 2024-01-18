// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexportcsv.h>
#include <rudiments/filedescriptor.h>
#include <rudiments/file.h>
#include <rudiments/permissions.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexportcsv::sqlrexportcsv() {
}

sqlrexportcsv::~sqlrexportcsv() {
}

bool sqlrexportcsv::exportToFile(const char *filename) {
	return sqlrexport::exportToFile(filename);
}

bool sqlrexportcsv::exportToFile(const char *filename, const char *table) {

	// reset flags
	setExportRow(true);
	setExportedRowCount(0);
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

	// call the pre-columns event
	if (!columnsStart()) {
		return false;
	}

	sqlrcursor	*sqlrcur=getSqlrCursor();
	const char * const *columnstoignore=getColumnsToIgnore();

	// export columns...
	uint32_t	cols=sqlrcur->colCount();
	clearNumberColumns();
	bool	first=true;
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

			if (first) {
				first=false;
			} else {
				fd->write(',');
			}

			// call the pre-column event
			if (!columnStart()) {
				return false;
			}

			// export the column name
			bool	isnumber=
				charstring::isNumber(getCurrentField());
			if (!isnumber) {
				fd->write('"');
			}
			escapeField(fd,getCurrentField());
			if (!isnumber) {
				fd->write('"');
			}

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
		fd->write('\n');
	}

	// call the pre-rows event
	if (!rowsStart()) {
		return false;
	}

	// export rows...
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

			bool	first=true;
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

				// prepend a comma if necessary
				if (first) {
					first=false;
				} else {
					fd->write(',');
				}

				// call the pre-field event
				if (!fieldStart()) {
					return false;
				}

				// we need to quote the field if it's not a
				// number, or if it is a number, but has more
				// than 12 digits.  Excel (and presumably other
				// spreadsheet apps) likes to convert 12+
				// digit numbers to scientific notation.
				bool	quote=
					(!getNumberColumn(getCurrentColumn()) ||
					charstring::getLength(
						getCurrentField())>=12);

				// export the field
				if (quote) {
					fd->write('"');
				}
				escapeField(fd,getCurrentField());
				if (quote) {
					fd->write('"');
				}

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
			fd->write('\n');
		}

		setExportedRowCount(getExportedRowCount()+1);
		setCurrentRow(getCurrentRow()+1);
	}

	// call the post-rows event
	if (!rowsEnd()) {
		return false;
	}

	return true;
}

void sqlrexportcsv::escapeField(filedescriptor *fd, const char *field) {
	for (const char *f=field; *f; f++) {
		// escape double quotes and ignore non-ascii characters
		if (*f=='"') {
			fd->write("\"\"");
		} else if (*f>=' ' && *f<='~') {
			fd->write(*f);
		}
	}
}

bool sqlrexportcsv::exportToJsonDomNode(domnode *jsondomnode) {
	return sqlrexport::exportToJsonDomNode(jsondomnode);
}

bool sqlrexportcsv::exportToJsonDomNode(domnode *jsondomnode,
						const char *table) {

	// reset flags and counts
	setExportRow(true);
	setExportedRowCount(0);
	setCurrentRow(0);
	setCurrentColumn(0);
	setCurrentField(NULL);

	// call the pre-columns event
	if (!columnsStart()) {
		return false;
	}

	sqlrcursor	*sqlrcur=getSqlrCursor();
	const char * const *columnstoignore=getColumnsToIgnore();

	// export columns...
	uint32_t	cols=sqlrcur->colCount();
	domnode	*columns=jsondomnode->appendTag("columns");
	columns->setAttributeValue("t","a");
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

			// export the column name
			bool	isnumber=
				charstring::isNumber(getCurrentField());
			domnode	*column=columns->appendTag("v");
			if (isnumber) {
				column->setAttributeValue("t","n");
				column->setAttributeValue("v",
						getCurrentField());
			} else {
				column->setAttributeValue("t","s");
				column->setAttributeValue("v",
						getCurrentField());
			}

			// call the post-column event
			if (!columnEnd()) {
				return false;
			}
		}
	}

	// call the post-columns event
	if (!columnsEnd()) {
		return false;
	}

	// call the pre-rows event
	if (!rowsStart()) {
		return false;
	}

	// export rows
	do {

		// reset export-row flag
		setExportRow(true);

		// call the pre-row event
		if (!rowStart()) {
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (getExportRow()) {

			domnode	*row=jsondomnode->appendTag("row");
			row->setAttributeValue("t","a");

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
				domnode	*field=row->appendTag("v");
				if (getNumberColumn(getCurrentColumn())) {
					field->setAttributeValue("t","n");
					field->setAttributeValue("v",
							getCurrentField());
				} else {
					field->setAttributeValue("t","s");
					field->setAttributeValue("v",
							getCurrentField());
				}

				// call the post-field event
				if (!fieldEnd()) {
					return false;
				}
			}
		}

		// call the post-row event
		if (!rowEnd()) {
			return false;
		}

		setExportedRowCount(getExportedRowCount()+1);
		setCurrentRow(getCurrentRow()+1);

	} while (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount());

	// call the post-rows event
	if (!rowsEnd()) {
		return false;
	}

	return true;
}
