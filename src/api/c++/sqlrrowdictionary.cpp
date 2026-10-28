// Copyright (c) David Muse
// See the COPYING file for more information.

#include <sqlrelay/sqlrcollections.h>

class sqlrrowdictionaryprivate {
	private:
		friend class sqlrrowdictionary;

		sqlrcursor	*_cursor;
		uint64_t	_row;

		linkedlist<const char *>	*_keys;
};

sqlrrowdictionary::sqlrrowdictionary() :
			dictionarycollection<const char *,const char *>() {
	pvt=new sqlrrowdictionaryprivate;
	pvt->_cursor=NULL;
	pvt->_row=0;
	pvt->_keys=NULL;
}

sqlrrowdictionary::sqlrrowdictionary(sqlrcursor *cursor) :
			dictionarycollection<const char *, const char *>() {
	pvt=new sqlrrowdictionaryprivate;
	pvt->_cursor=cursor;
	pvt->_row=0;
	pvt->_keys=NULL;
}

sqlrrowdictionary::sqlrrowdictionary(sqlrcursor *cursor, uint64_t row) :
			dictionarycollection<const char *, const char *>() {
	pvt=new sqlrrowdictionaryprivate;
	pvt->_cursor=cursor;
	pvt->_row=row;
	pvt->_keys=NULL;
}

sqlrrowdictionary::~sqlrrowdictionary() {
	if (pvt->_keys) {
		delete pvt->_keys;
	}
	delete pvt;
}

void sqlrrowdictionary::setCursor(sqlrcursor *cursor) {
	pvt->_cursor=cursor;
}

void sqlrrowdictionary::setRow(uint64_t row) {
	pvt->_row=row;
}

bool sqlrrowdictionary::isReadOnly() {
	return true;
}

bool sqlrrowdictionary::getTrackInsertionOrder() {
	return true;
}

void sqlrrowdictionary::setValue(const char *key, const char *value) {
	// do nothing
}

void sqlrrowdictionary::setValues(const char **key, const char **value) {
	// do nothing
}

void sqlrrowdictionary::setValues(const char **key, const char **value,
							uint64_t count) {
	// do nothing
}

void sqlrrowdictionary::setValues(const char * const *key,
						const char * const *value) {
	// do nothing
}

void sqlrrowdictionary::setValues(const char * const *key,
						const char * const *value,
						uint64_t count) {
	// do nothing
}

bool sqlrrowdictionary::getValue(const char *key, const char **value) {
	// FIXME: arguably this should check that the key is a legitimate
	// column name, which could be a bit of a rabbit-hole to do 
	// efficiently
	*value=getValue(key);
	return true;
}

const char *sqlrrowdictionary::getValue(const char *key) {
	if (!pvt->_cursor) {
		return NULL;
	}
	return pvt->_cursor->getField(pvt->_row,key);
}

bool sqlrrowdictionary::getKey(const char *key, const char **k) {
	*k=key;
	return true;
}

const char *sqlrrowdictionary::getKey(const char *key) {
	return key;
}

linkedlist<const char *> *sqlrrowdictionary::getKeys() {
	if (!pvt->_keys) {
		pvt->_keys=new linkedlist<const char *>();
	}

	// The column names belong to the cursor's current result set, which
	// can change between calls, so rebuild the list every time.
	pvt->_keys->clear();
	if (pvt->_cursor) {
		for (uint32_t i=0; i<pvt->_cursor->colCount(); i++) {
			pvt->_keys->append(pvt->_cursor->getColumnName(i));
		}
	}
	return pvt->_keys;
}

uint64_t sqlrrowdictionary::getCount() {
	return (pvt->_cursor)?pvt->_cursor->colCount():0;
}

bool sqlrrowdictionary::remove(const char *key) {
	// do nothing
	return true;
}

bool sqlrrowdictionary::clear() {
	// do nothing
	return true;
}
