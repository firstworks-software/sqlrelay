// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrquerytranslationprivate {
	friend class sqlrquerytranslation;
	private:
		domnode			*_parameters;
};

sqlrquerytranslation::sqlrquerytranslation(sqlrservercontroller *cont,
					domnode *parameters) {
	pvt=new sqlrquerytranslationprivate;
	pvt->_parameters=parameters;
}

sqlrquerytranslation::~sqlrquerytranslation() {
	delete pvt;
}

bool sqlrquerytranslation::requiresTree() {
	return false;
}

bool sqlrquerytranslation::run(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				const char *query,
				uint32_t querysize,
				stringbuffer *translatedquery) {
	return true;
}

bool sqlrquerytranslation::run(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				xmldom *querytree) {
	return true;
}

const char *sqlrquerytranslation::getError() {
	return NULL;
}

domnode *sqlrquerytranslation::getParameters() {
	return pvt->_parameters;
}

void sqlrquerytranslation::endTransaction(bool commit) {
}

void sqlrquerytranslation::endSession() {
}
