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

	if (!sqlrexport::exportToFile(filename,table)) {
		return false;
	}

	// capture the filename and table
	setFileName(filename);
	setTable(table);

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
	bool	first=true;
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

		// if we're not ignoring all columns or this column...
		if (!getIgnoreColumns() &&
			!charstring::isInSet(getCurrentField(),
						getColumnsToIgnore())) {

			// export the column name
			if (first) {
				first=false;
			} else {
				fd->write(',');
			}
			bool	isnumber=
				charstring::isNumber(getCurrentField());
			if (!isnumber) {
				fd->write('"');
			}
			escapeField(fd,getCurrentField());
			if (!isnumber) {
				fd->write('"');
			}
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
		fd->write('\n');
	}

	// reset current column/field
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(sqlrcur->getField(0,(uint32_t)0));

	// call the pre-rows event
	if (!rowsStart()) {
		return false;
	}

	// export rows...
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
					fd->write(',');
				}
				if (quote) {
					fd->write('"');
				}
				escapeField(fd,getCurrentField());
				if (quote) {
					fd->write('"');
				}
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
			fd->write('\n');
		}

		// update exported row count
		if (getExportRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}

		// update current row
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

	if (!sqlrexport::exportToJsonDomNode(jsondomnode,table)) {
		return false;
	}

	// capture the top-level domnode
	setJsonDomNode(jsondomnode);

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
		// "h" for header, like in csvdom
		setColumnsDomNode(getJsonDomNode()->appendTag("h"));
		getColumnsDomNode()->setAttributeValue("t","a");
	}
	for (setCurrentColumn(0);
		getCurrentColumn()<cols;
		setCurrentColumn(getCurrentColumn()+1)) {

		// set the current column name and field
		setCurrentColumnName(
			sqlrcur->getColumnName(getCurrentColumn()));
		setCurrentField(getCurrentColumnName());

		// call the pre-column event
		if (!columnStart()) {
			return false;
		}

		// if we're not ignoring all columns or this column...
		if (!getIgnoreColumns() &&
			!charstring::isInSet(getCurrentField(),
						getColumnsToIgnore())) {

			// export the column name
			// "v" for value, all jsondom arrays
			// consist of "v"alues
			bool	isnumber=
				charstring::isNumber(getCurrentField());
			domnode	*column=getColumnsDomNode()->appendTag("v");
			setCurrentColumnDomNode(column);
			if (isnumber) {
				column->setAttributeValue("t","n");
				column->setAttributeValue("v",
						getCurrentField());
			} else {
				column->setAttributeValue("t","s");
				column->setAttributeValue("v",
						getCurrentField());
			}
		}

		// call the post-column event
		if (!columnEnd()) {
			return false;
		}
	}

	// call the post-columns event
	if (!columnsEnd()) {
		return false;
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
	do {

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
			// "r" for record, like in csvdom
			setCurrentRowDomNode(
				getJsonDomNode()->appendTag("r"));
			getCurrentRowDomNode()->setAttributeValue("t","a");
		}

		for (setCurrentColumn(0);
			getCurrentColumn()<cols;
			setCurrentColumn(getCurrentColumn()+1)) {

			// set the current column name and field
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
				// "v" for value, all jsondom arrays
				// consist of "v"alues
				domnode	*field=getCurrentRowDomNode()->
								appendTag("v");
				setCurrentFieldDomNode(field);
				if (getIsNumericColumn(getCurrentColumn())) {
					field->setAttributeValue("t","n");
					field->setAttributeValue("v",
							getCurrentField());
				} else {
					field->setAttributeValue("t","s");
					field->setAttributeValue("v",
							getCurrentField());
				}
			}

			// call the post-field event
			if (!fieldEnd()) {
				return false;
			}
		}

		// call the post-row event
		if (!rowEnd()) {
			return false;
		}

		// update exported row count
		if (getExportRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}

		// update current row
		setCurrentRow(getCurrentRow()+1);

	} while (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount());

	// call the post-rows event
	if (!rowsEnd()) {
		return false;
	}

	return true;
}
