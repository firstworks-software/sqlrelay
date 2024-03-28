// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrresultsetrowtranslationprivate {
	friend class sqlrresultsetrowtranslation;
	private:
		domnode	*_parameters;
};

sqlrresultsetrowtranslation::sqlrresultsetrowtranslation(
				sqlrservercontroller *cont,
				domnode *parameters) {
	pvt=new sqlrresultsetrowtranslationprivate;
	pvt->_parameters=parameters;
}

sqlrresultsetrowtranslation::~sqlrresultsetrowtranslation() {
	delete pvt;
}

bool sqlrresultsetrowtranslation::run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					uint32_t colcount,
					const char * const *fieldnames,
					const char ***field,
					uint64_t **fieldsize) {
	return true;
}

const char *sqlrresultsetrowtranslation::getError() {
	return NULL;
}

domnode *sqlrresultsetrowtranslation::getParameters() {
	return pvt->_parameters;
}

void sqlrresultsetrowtranslation::endTransaction(bool commit) {
}

void sqlrresultsetrowtranslation::endSession() {
}
