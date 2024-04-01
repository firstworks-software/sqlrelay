// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>

class sqlrservermoduleprivate {
	friend class sqlrservermodule;
	public:
		domnode	*_parameters;
};

sqlrservermodule::sqlrservermodule(sqlrservercontroller *cont,
						domnode *parameters) :
						sqlrserverbase() {
	pvt=new sqlrservermoduleprivate;
	this->cont=cont;
	pvt->_parameters=parameters;

	// set debug...

	// if debug is enabled in the parent tag,
	// then enable debug for this module
	setDebug(charstring::isYes(parameters->getParent()->
					getAttributeValue("debug")));
	if (getDebug()) {
		// if debug was enabled in the parent tag, but was
		// specifically disabled in the tag for this module, then
		// disable it
		if (charstring::isNo(parameters->getAttributeValue("debug"))) {
			setDebug(false);
		}
	} else {
		// if debug wasn't enabled in the parent tag, but was
		// specifically enabled in the tag for this module, then
		// enable it
		if (charstring::isYes(parameters->getAttributeValue("debug"))) {
			setDebug(true);
		}
	}
}

sqlrservermodule::~sqlrservermodule() {
	delete pvt;
}

domnode *sqlrservermodule::getParameters() {
	return pvt->_parameters;
}

void sqlrservermodule::endTransaction(bool commit) {
	// by default, do nothing
}

void sqlrservermodule::endSession() {
	// by default, do nothing
}
