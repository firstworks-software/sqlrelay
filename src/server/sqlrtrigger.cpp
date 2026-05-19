// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrtriggerprivate {
	friend class sqlrtrigger;
	private:
};

sqlrtrigger::sqlrtrigger(sqlrservercontroller *cont,
				domnode *parameters) :
				sqlrservermodule(cont,parameters) {
	pvt=new sqlrtriggerprivate;
}

sqlrtrigger::~sqlrtrigger() {
	delete pvt;
}

bool sqlrtrigger::runBeforePrepare(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur) {
	return true;
}

bool sqlrtrigger::runAfterPrepare(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur) {
	return true;
}

bool sqlrtrigger::runBeforeExecute(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur) {
	return true;
}

bool sqlrtrigger::runAfterExecute(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur) {
	return true;
}
