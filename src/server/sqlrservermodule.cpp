// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>

class sqlrservermoduleprivate {
	friend class sqlrservermodule;
	private:
		domnode		*_parameters;
		bool		_enabled;
		stringbuffer	_logbuffer;
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

	// set enabled flag
	pvt->_enabled=!charstring::isNo(
			parameters->getAttributeValue("enabled"));
}

sqlrservermodule::~sqlrservermodule() {
	delete pvt;
}

domnode *sqlrservermodule::getParameters() {
	return pvt->_parameters;
}

bool sqlrservermodule::getEnabled() {
	return pvt->_enabled;
}

void sqlrservermodule::debugStart(const char *title, ...) {
	if (!getDebug()) {
		return;
	}
	if (!cont) {
		return;
	}
	pvt->_logbuffer.clear();
	va_list	argp;
	va_start(argp,title);
	pvt->_logbuffer.printf(title,&argp);
	va_end(argp);
	cont->raiseDebugStartEvent("%s",pvt->_logbuffer.getString());
}

void sqlrservermodule::debugWrite(const char *string, ...) {
	if (!getDebug()) {
		return;
	}
	if (!cont) {
		return;
	}
	pvt->_logbuffer.clear();
	va_list	argp;
	va_start(argp,string);
	pvt->_logbuffer.printf(string,&argp);
	va_end(argp);
	cont->raiseDebugWriteEvent("%s",pvt->_logbuffer.getString());
}

void sqlrservermodule::debugHexDump(const byte_t *data, uint64_t size) {
	if (!getDebug()) {
		return;
	}
	if (!cont) {
		return;
	}
	pvt->_logbuffer.clear();
	pvt->_logbuffer.printHex(data,size,0);
	cont->raiseDebugWriteEvent("%.*s",
			pvt->_logbuffer.getSize(),
			pvt->_logbuffer.getString());
}

void sqlrservermodule::debugEnd() {
	if (!getDebug()) {
		return;
	}
	if (!cont) {
		return;
	}
	cont->raiseDebugEndEvent();
}

void sqlrservermodule::endTransaction(bool commit) {
	// by default, do nothing
}

void sqlrservermodule::endSession() {
	// by default, do nothing
}
