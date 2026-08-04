// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexportcsv.h>

// not configurable (yet), but keep it in one place
static const char	delimiter=',';

sqlrexportcsv::sqlrexportcsv() : sqlrexportfile() {
}

sqlrexportcsv::~sqlrexportcsv() {
}

bool sqlrexportcsv::exportData() {
	return sqlrexportfile::exportData();
}

bool sqlrexportcsv::exportColumnName(bool first) {

	if (!sqlrexport::exportColumnName(first)) {
		return false;
	}

	// get the file descriptor
	filedescriptor	*fd=getFileDescriptor();

	// get the column name
	// (column names are null-terminated, so the length that
	// setCurrentField() derived for it is the right one)
	const char	*name=getCurrentField();
	uint32_t	namelen=getCurrentFieldLength();

	// we need to quote the column name if it's not a number, or if it
	// contains anything that would otherwise break the field, the row,
	// or the file apart
	bool	quote=(!charstring::isNumber(name,(int32_t)namelen) ||
					needsQuotes(name,namelen));

	// export the column name
	if (!first) {
		if (fd->write(delimiter)!=sizeof(char)) {
			return systemError();
		}
	}
	if (quote) {
		if (fd->write('"')!=sizeof(char)) {
			return systemError();
		}
	}
	if (!escapeValue(fd,name,namelen)) {
		return false;
	}
	if (quote) {
		if (fd->write('"')!=sizeof(char)) {
			return systemError();
		}
	}
	return true;
}

bool sqlrexportcsv::endProcessingColumns() {

	// (we call this before closing the columns in case an
	// overridden columnsEnd() wants to add mroe columns or
	// something)
	if (!sqlrexportfile::endProcessingColumns()) {
		return false;
	}

	if (!getExcludeColumns()) {
		if (getFileDescriptor()->write('\n')!=sizeof(char)) {
			return systemError();
		}
	}
	return true;
}

bool sqlrexportcsv::exportField(bool first) {

	if (!sqlrexportfile::exportField(first)) {
		return false;
	}

	// get the file descriptor
	filedescriptor	*fd=getFileDescriptor();

	// get the field
	// (field data can contain embedded nulls, so use the
	// length that came from the cursor, rather than
	// measuring the field itself)
	const char	*field=getCurrentField();
	uint32_t	length=getCurrentFieldLength();

	// we need to quote the field if it contains anything
	// that would otherwise break the field, the row, or the
	// file apart, or if it's not from a numeric column, or
	// if it is, but has 12 or more characters.  Excel (and
	// presumably other spreadsheet apps) likes to convert
	// 12+ digit numbers to scientific notation.
	bool	quote=(!getIsNumericColumn(getCurrentColumn()) ||
			length>=12 || needsQuotes(field,length));

	// export the field
	if (first) {
		first=false;
	} else {
		if (fd->write(delimiter)!=sizeof(char)) {
			return systemError();
		}
	}
	if (quote) {
		if (fd->write('"')!=sizeof(char)) {
			return systemError();
		}
	}
	if (!escapeValue(fd,field,length)) {
		return false;
	}
	if (quote) {
		if (fd->write('"')!=sizeof(char)) {
			return systemError();
		}
	}
	return true;
}

bool sqlrexportcsv::endProcessingRow() {

	// (we call this before closing the row in case an overridden
	// rowEnd() wants to add mroe fields or something)
	if (!sqlrexportfile::endProcessingRow()) {
		return false;
	}

	if (!getExcludeRow()) {
		if (getFileDescriptor()->write('\n')!=sizeof(char)) {
			return systemError();
		}
	}
	return true;
}

bool sqlrexportcsv::needsQuotes(const char *value, uint32_t length) {
	if (!value) {
		return false;
	}
	for (uint32_t i=0; i<length; i++) {
		char	ch=value[i];
		if (ch==delimiter || ch=='"' || ch=='\n' || ch=='\r') {
			return true;
		}
	}
	return false;
}

bool sqlrexportcsv::escapeValue(filedescriptor *fd, const char *field) {
	// (only for null-terminated values, field data
	// has to use the version below)
	return escapeValue(fd,field,(uint32_t)charstring::getLength(field));
}

bool sqlrexportcsv::escapeValue(filedescriptor *fd,
				const char *field, uint32_t length) {
	if (!field) {
		return true;
	}
	for (uint32_t i=0; i<length; i++) {
		// escape double quotes
		if (field[i]=='"') {
			if (fd->write("\"\"")!=2) {
				return systemError();
			}
		} else {
			if (fd->write(field[i])!=sizeof(char)) {
				return systemError();
			}
		}
	}
	return true;
}
