// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>

class sqlrauthprivate {
	friend class sqlrauth;
	public:
		sqlrpwdencs	*_sqlrpe;
};

sqlrauth::sqlrauth(sqlrservercontroller *cont,
				sqlrpwdencs *sqlrpe,
				domnode *parameters) :
				sqlrservermodule(cont,parameters) {
	pvt=new sqlrauthprivate;
	this->cont=cont;
	pvt->_sqlrpe=sqlrpe;
}

sqlrauth::~sqlrauth() {
	delete pvt;
}

const char *sqlrauth::auth(sqlrcredentials *cred) {
	return NULL;
}

sqlrpwdencs *sqlrauth::getPasswordEncryptions() {
	return pvt->_sqlrpe;
}
