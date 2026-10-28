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

// returns the cursor's name for the first column named key, or NULL
static const char *findColumnName(sqlrcursor *cursor, const char *key) {
	if (!cursor) {
		return NULL;
	}
	for (uint32_t i=0; i<cursor->colCount(); i++) {
		const char	*name=cursor->getColumnName(i);
		if (name && !charstring::compare(name,key)) {
			return name;
		}
	}
	return NULL;
}

bool sqlrrowdictionary::getValue(const char *key, const char **value) {
	if (!findColumnName(pvt->_cursor,key)) {
		*value=NULL;
		return false;
	}
	*value=pvt->_cursor->getField(pvt->_row,key);
	return true;
}

const char *sqlrrowdictionary::getValue(const char *key) {
	if (!pvt->_cursor) {
		return NULL;
	}
	return pvt->_cursor->getField(pvt->_row,key);
}

bool sqlrrowdictionary::getKey(const char *key, const char **k) {
	*k=findColumnName(pvt->_cursor,key);
	return (*k!=NULL);
}

const char *sqlrrowdictionary::getKey(const char *key) {
	return findColumnName(pvt->_cursor,key);
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
