// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrresultsettranslationprivate {
	friend class sqlrresultsettranslation;
	private:
		domnode	*_parameters;
};

sqlrresultsettranslation::sqlrresultsettranslation(
				sqlrservercontroller *cont,
				domnode *parameters) {
	pvt=new sqlrresultsettranslationprivate;
	pvt->_parameters=parameters;
}

sqlrresultsettranslation::~sqlrresultsettranslation() {
	delete pvt;
}

bool sqlrresultsettranslation::run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *fieldname,
					uint32_t fieldindex,
					const char **field,
					uint64_t *fieldsize) {
	return true;
}

const char *sqlrresultsettranslation::getError() {
	return NULL;
}

domnode *sqlrresultsettranslation::getParameters() {
	return pvt->_parameters;
}

void sqlrresultsettranslation::endTransaction(bool commit) {
}

void sqlrresultsettranslation::endSession() {
}
