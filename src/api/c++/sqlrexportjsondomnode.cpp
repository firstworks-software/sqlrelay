// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrexportjsondomnode.h>

#define NEED_IS_NUMBER_TYPE_CHAR
#include <datatypes.h>

sqlrexportjsondomnode::sqlrexportjsondomnode() : sqlrexport() {
	jsondomnode=NULL;
}

sqlrexportjsondomnode::~sqlrexportjsondomnode() {
}

void sqlrexportjsondomnode::setJsonDomNode(domnode *dn) {
	jsondomnode=dn;
}

domnode	*sqlrexportjsondomnode::getJsonDomNode() {
	return jsondomnode;
}

void sqlrexportjsondomnode::setColumnsDomNode(domnode *dn) {
	columnsdomnode=dn;
}

domnode	*sqlrexportjsondomnode::getColumnsDomNode() {
	return columnsdomnode;
}

void sqlrexportjsondomnode::setCurrentColumnDomNode(domnode *dn) {
	currentcolumndomnode=dn;
}

domnode	*sqlrexportjsondomnode::getCurrentColumnDomNode() {
	return currentcolumndomnode;
}

void sqlrexportjsondomnode::setRowsDomNode(domnode *dn) {
	rowsdomnode=dn;
}

domnode	*sqlrexportjsondomnode::getRowsDomNode() {
	return rowsdomnode;
}

void sqlrexportjsondomnode::setCurrentRowDomNode(domnode *dn) {
	currentrowdomnode=dn;
}

domnode	*sqlrexportjsondomnode::getCurrentRowDomNode() {
	return currentrowdomnode;
}

void sqlrexportjsondomnode::setCurrentFieldDomNode(domnode *dn) {
	currentfielddomnode=dn;
}

domnode	*sqlrexportjsondomnode::getCurrentFieldDomNode() {
	return currentfielddomnode;
}

bool sqlrexportjsondomnode::exportData() {

	clearOutput();

	// sanity check
	if (!getJsonDomNode()) {
		return error(0,"No domnode set with setJsonDomNode()");
	}

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

	// export columns...
	if (!getIgnoreColumns()) {
		// "h" for header, like in csvdom
		setColumnsDomNode(getJsonDomNode()->appendTag("h"));
		getColumnsDomNode()->setAttributeValue("t","a");
	}
	for (setCurrentColumn(0);
		getCurrentColumn()<cols;
		setCurrentColumn(getCurrentColumn()+1)) {

		// set the current column name (and field)
		setCurrentColumnName(
			sqlrcur->getColumnName(getCurrentColumn()));
		setCurrentField(getCurrentColumnName());

		// call the column-start event
		if (!columnStart()) {
			return false;
		}

		// reset the current field to the current column name
		// (in case columnStart overrode the columm name)
		setCurrentField(getCurrentColumnName());

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

		// call the column-end event
		if (!columnEnd()) {
			return false;
		}
	}

	// call the columns-end event
	if (!columnsEnd()) {
		return false;
	}

	// reset current column/field
	setCurrentColumn(0);
	setCurrentColumnName(sqlrcur->getColumnName(0));
	setCurrentField(sqlrcur->getField(0,(uint32_t)0));

	// call the rows-start event
	if (!rowsStart()) {
		return false;
	}

	// export rows
	do {

		// reset export-row flag and current column/field
		setIgnoreRow(false);
		setCurrentColumn(0);
		setCurrentColumnName(sqlrcur->getColumnName(0));
		setCurrentField(sqlrcur->getField(getCurrentRow(),(uint32_t)0));

		// call the row-start event
		if (!rowStart()) {
			return false;
		}

		// if rowStart() didn't disable export of this row...
		if (!getIgnoreRow()) {
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

			// call the field-start event
			if (!fieldStart()) {
				return false;
			}

			// if we're not ignoring this row or column...
			if (!getIgnoreRow() &&
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

			// call the field-end event
			if (!fieldEnd()) {
				return false;
			}
		}

		// set the current column and field to NULL
		setCurrentColumnName(NULL);
		setCurrentField(NULL);

		// call the row-end event
		if (!rowEnd()) {
			return false;
		}

		// update exported row count
		if (!getIgnoreRow()) {
			setExportedRowCount(getExportedRowCount()+1);
		}

		// update current row
		setCurrentRow(getCurrentRow()+1);

	} while (!sqlrcur->endOfResultSet() ||
			getCurrentRow()<sqlrcur->rowCount());

	// call the rows-end event
	if (!rowsEnd()) {
		return false;
	}

	// call the export-end event
	if (!exportEnd()) {
		return false;
	}

	return true;
}

void sqlrexportjsondomnode::clearOutput() {
	sqlrexport::clearOutput();
	columnsdomnode=NULL;
	currentcolumndomnode=NULL;
	rowsdomnode=NULL;
	currentrowdomnode=NULL;
	currentfielddomnode=NULL;
}
