// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrerrortranslationprivate {
	friend class sqlrerrortranslation;
	private:

		sqlrerrortranslations	*_es;
		domnode			*_parameters;
};

sqlrerrortranslation::sqlrerrortranslation(sqlrservercontroller *cont,
					sqlrerrortranslations *es,
					domnode *parameters) {
	pvt=new sqlrerrortranslationprivate;
	pvt->_es=es;
	pvt->_parameters=parameters;
}

sqlrerrortranslation::~sqlrerrortranslation() {
	delete pvt;
}

bool sqlrerrortranslation::run(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				int64_t errornumber,
				const char *error,
				uint32_t errorsize,
				int64_t *translatederrornumber,
				stringbuffer *translatederror) {
	return true;
}

const char *sqlrerrortranslation::getError() {
	return NULL;
}

sqlrerrortranslations *sqlrerrortranslation::getErrorTranslations() {
	return pvt->_es;
}

domnode *sqlrerrortranslation::getParameters() {
	return pvt->_parameters;
}

void sqlrerrortranslation::endTransaction(bool commit) {
}

void sqlrerrortranslation::endSession() {
}
