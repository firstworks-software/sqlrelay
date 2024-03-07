// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrimportcsv.h>

#include <rudiments/file.h>

sqlrimportcsv::sqlrimportcsv() : sqlrimportfile(), csvsax() {
}

sqlrimportcsv::~sqlrimportcsv() {
}

bool sqlrimportcsv::importData() {

	// set the table name from the file name, if it wasn't already set
	if (!getObjectName()) {
		setObjectName(file::getBaseName(getFileName(),".csv"));
	}

	// NOTE: startProcessingImport() calls the import-start event
	// NOTE: endProcessingImport() calls the import-end event
	return startProcessingImport() &&
		csvsax::parseFile(getFileName()) &&
		endProcessingImport();
}

bool sqlrimportcsv::headerStart() {
	// NOTE: startProcessingColumns() calls the columns-start event
	return startProcessingColumns();
}

bool sqlrimportcsv::column(const char *name, bool quoted) {
	char	*cname=charstring::duplicate(name);
	// NOTE: processColumnName() calls the
	// column-start and column-end events
	return processColumnName(&cname);
}

bool sqlrimportcsv::headerEnd() {
	// NOTE: endProcessingColumns() calls the columns-end event
	return endProcessingColumns();
}

bool sqlrimportcsv::bodyStart() {
	// NOTE: startProcessingRows() calls the rows-start event
	return startProcessingRows();
}

bool sqlrimportcsv::recordStart() {
	// NOTE: startProcessingRow() calls the row-start event
	return startProcessingRow();
}

bool sqlrimportcsv::field(const char *value, bool quoted) {
	char	*fval=charstring::duplicate(value);
	// NOTE: processField() calls the field-start and field-end events
	return processField(&fval);
}

bool sqlrimportcsv::recordEnd() {
	// NOTE: endProcessingRow() calls the row-end event
	return endProcessingRow();
}

bool sqlrimportcsv::bodyEnd() {
	// NOTE: endProcessingRows() calls the row-end event
	return endProcessingRows();
}
