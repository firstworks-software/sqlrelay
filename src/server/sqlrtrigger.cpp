// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

class sqlrtriggerprivate {
	friend class sqlrtrigger;
	private:
		sqlrtriggers	*_ts;
		domnode	*_parameters;
};

sqlrtrigger::sqlrtrigger(sqlrservercontroller *cont,
				sqlrtriggers *ts,
				domnode *parameters) {
	pvt=new sqlrtriggerprivate;
	pvt->_ts=ts;
	pvt->_parameters=parameters;
}

sqlrtrigger::~sqlrtrigger() {
	delete pvt;
}

bool sqlrtrigger::runBefore(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				bool *suppress) {
	*suppress=false;
	return true;
}

bool sqlrtrigger::runAfter(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur) {
	return true;
}

sqlrtriggers *sqlrtrigger::getTriggers() {
	return pvt->_ts;
}

domnode *sqlrtrigger::getParameters() {
	return pvt->_parameters;
}

void sqlrtrigger::endTransaction(bool commit) {
}

void sqlrtrigger::endSession() {
}
