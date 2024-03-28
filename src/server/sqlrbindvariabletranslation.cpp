// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrbindvariabletranslationprivate {
	friend class sqlrbindvariabletranslation;
	private:
		domnode	*_parameters;
};

sqlrbindvariabletranslation::sqlrbindvariabletranslation(
				sqlrservercontroller *cont,
				domnode *parameters) {
	pvt=new sqlrbindvariabletranslationprivate;
	pvt->_parameters=parameters;
}

sqlrbindvariabletranslation::~sqlrbindvariabletranslation() {
	delete pvt;
}

bool sqlrbindvariabletranslation::run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {
	return true;
}

const char *sqlrbindvariabletranslation::getError() {
	return NULL;
}

domnode *sqlrbindvariabletranslation::getParameters() {
	return pvt->_parameters;
}

void sqlrbindvariabletranslation::endTransaction(bool commit) {
}

void sqlrbindvariabletranslation::endSession() {
}
