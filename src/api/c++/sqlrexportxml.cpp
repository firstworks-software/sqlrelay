// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexportxml.h>
#include <rudiments/file.h>
#include <rudiments/filesystem.h>
#include <rudiments/permissions.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexportxml::sqlrexportxml() : sqlrexportfile() {
}

sqlrexportxml::~sqlrexportxml() {
}

bool sqlrexportxml::exportData() {

	clearOutput();

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
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	// export xml header
	if (fd->write("<?xml version=\"1.0\"?>\n")!=22) {
		return systemError();
	}

	// export table tag
	if (fd->write("<table")!=6) {
		return systemError();
	}
	if (!charstring::isNullOrEmpty(getTable())) {
	 	if (fd->write(" name=\"")!=7) {
			return systemError();
		}
		if (!escapeField(fd,getTable())) {
			return false;
		}
		if (fd->write('\"')!=sizeof(char)) {
			return systemError();
		}
	}
	if (fd->write(">\n")!=2) {
		return systemError();
	}

	// call the columns-start event
	if (!columnsStart()) {
		fd->flushWriteBuffer(-1,-1);
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
		const char	*type=
			sqlrcur->getColumnType(getCurrentColumn());
	
		// call the column-start event
		if (!columnStart()) {
			fd->flushWriteBuffer(-1,-1);
			return false;
		}

		// set the current field to the current column name too
		// (in case columnStart overrode the columm name)
		setCurrentField(getCurrentColumnName());

		// if we're not ignoring all columns, or this column...
		if (!getIgnoreColumns() &&
			!charstring::isInSet(getCurrentField(),
						getColumnsToIgnore())) {

			// export the column name and type
			if (fd->write("	<column name=\"")!=15) {
				return systemError();
			}
			if (!escapeField(fd,getCurrentField())) {
				return false;
			}
			if (fd->write("\" type=\"")!=8) {
				return systemError();
			}
			if (!escapeField(fd,type)) {
				return false;
			}
			if (fd->write("\"/>\n")!=4) {
				return systemError();
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

	if (!getIgnoreColumns()) {
		if (fd->write("</columns>\n")!=11) {
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

	// export rows
	if (fd->write("<rows>\n")!=7) {
		return systemError();
	}
	while (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount()) {

		// reset export-row flag and current column/field
		setIgnoreRow(false);
		setCurrentColumn(0);
		setCurrentColumnName(sqlrcur->getColumnName(0));
		setCurrentField(sqlrcur->getField(getCurrentRow(),(uint32_t)0));

		// call the row-start event
		if (!rowStart()) {
			fd->flushWriteBuffer(-1,-1);
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (!getIgnoreRow()) {
			if (fd->write("	<row>\n")!=7) {
				return systemError();
			}
		}

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

			// if we're not ignoring this row or column...
			if (!getIgnoreRow() &&
				!charstring::isInSet(
					sqlrcur->getColumnName(
						getCurrentColumn()),
				getColumnsToIgnore())) {

				// export the field
				if (fd->write("	<field>")!=8) {
					return systemError();
				}
				if (!escapeField(fd,getCurrentField())) {
					return false;
				}
				if (fd->write("</field>\n")!=9) {
					return systemError();
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
		if (!getIgnoreRow()) {
			if (fd->write("	</row>\n")!=8) {
				return systemError();
			}
		}

		// update exported row count
		if (!getIgnoreRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}

		// update current row
		setCurrentRow(getCurrentRow()+1);
	}

	// call the rows-end event
	// (we call this before closing the rows in case an overridden
	// rowsEnd() wants to add more rows or something)
	if (!rowsEnd()) {
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	if (fd->write("</rows>\n")!=8) {
		return systemError();
	}
	if (fd->write("</table>\n")!=9) {
		return systemError();
	}

	// call the export-end event
	if (!exportEnd()) {
		fd->flushWriteBuffer(-1,-1);
		return false;
	}

	fd->flushWriteBuffer(-1,-1);
	return true;
}

bool sqlrexportxml::escapeField(filedescriptor *fd, const char *field) {
	if (!field) {
		return true;
	}
	for (const char *f=field; *f; f++) {
		if (*f=='"') {
			if (fd->write("\"\"")!=2) {
				return systemError();
			}
		} else if (*f<' ' || *f>'~' || *f=='&' || *f=='<' || *f=='>') {
			if (fd->printf("&%d;",(uint8_t)*f)!=
						((*f<=9)?3:((*f>=100)?5:4))) {
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
