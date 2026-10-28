// Copyright (c) David Muse
// See the COPYING file for more information.

#include <sqlrelay/sqlrcollections.h>

class sqlrscalarprivate {
	private:
		friend class sqlrscalar;

		sqlrscalarprivate() {
			_cursor=NULL;
			_row=0;
			_col=0;
			_null=NULL;
		}

		sqlrcursor	*_cursor;
		uint64_t	_row;
		uint32_t	_col;

		const char	*_null;
};

sqlrscalar::sqlrscalar() : scalarcollection<const char *>() {
	pvt=new sqlrscalarprivate;
}

sqlrscalar::sqlrscalar(sqlrcursor *cursor) : scalarcollection<const char *>() {
	pvt=new sqlrscalarprivate;
	pvt->_cursor=cursor;
}

sqlrscalar::sqlrscalar(sqlrcursor *cursor, uint64_t row, uint64_t col) :
					scalarcollection<const char *>() {
	pvt=new sqlrscalarprivate;
	pvt->_cursor=cursor;
	pvt->_row=row;
	pvt->_col=col;
}

sqlrscalar::~sqlrscalar() {
	delete pvt;
}

bool sqlrscalar::isReadOnly() {
	return true;
}

void sqlrscalar::setCursor(sqlrcursor *cursor) {
	pvt->_cursor=cursor;
	pvt->_row=0;
	pvt->_col=0;
}

void sqlrscalar::setRow(uint64_t row) {
	pvt->_row=row;
}

void sqlrscalar::setColumn(uint32_t col) {
	pvt->_col=col;
}

void sqlrscalar::setValue(const char *value) {
	// do nothing
}

const char *sqlrscalar::getValue() {
	if (!pvt->_cursor) {
		return NULL;
	}
	return pvt->_cursor->getField(pvt->_row,pvt->_col);
}

const char * &sqlrscalar::getReference() {
	return pvt->_null;
}

bool sqlrscalar::clear() {
	// do nothing
	return true;
}
