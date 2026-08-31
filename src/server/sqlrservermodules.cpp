// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/domnode.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include <config.h>

class sqlrservermodulesprivate {
	friend class sqlrservermodules;
	private:
		domnode		*_parameters;
		stringbuffer	_logbuffer;
};

sqlrservermodules::sqlrservermodules(sqlrservercontroller *cont,
						domnode *parameters) :
						sqlrserverbase() {
	pvt=new sqlrservermodulesprivate;
	this->cont=cont;
	pvt->_parameters=parameters;
	setDebug(parameters->getAttributeValue("debug"));
}

sqlrservermodules::~sqlrservermodules() {
	unload();
	delete pvt;
}

domnode *sqlrservermodules::getParameters() {
	return pvt->_parameters;
}

bool sqlrservermodules::isModuleDisabled(domnode *parameters) {
	if (charstring::isNo(parameters->getAttributeValue("enabled"))) {
		debugWrite("not loading %s - disabled",
				getModuleName(parameters));
		return true;
	}
	return false;
}

void sqlrservermodules::debugStart(const char *title, ...) {
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

void sqlrservermodules::debugWrite(const char *string, ...) {
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

void sqlrservermodules::debugEnd() {
	if (!getDebug()) {
		return;
	}
	if (!cont) {
		return;
	}
	cont->raiseDebugEndEvent();
}

bool sqlrservermodules::load() {

	unload();

	// run through the module tags
	for (domnode *moduledata=pvt->_parameters->getFirstTagChild();
				!moduledata->isNullNode();
				moduledata=moduledata->getNextTagSibling()) {

		// skip disabled modules
		if (isModuleDisabled(moduledata)) {
			continue;
		}

		// load the module
		loadModule(moduledata);
	}
	return true;
}

void sqlrservermodules::loadModule(domnode *parameters) {
	// by default, do nothing
}

const char *sqlrservermodules::getModuleName(domnode *parameters) {

	const char	*module=parameters->getAttributeValue("module");

	if (charstring::isNullOrEmpty(module)) {
		// try "file", that's what it used to be called
		module=pvt->_parameters->getAttributeValue("file");
	}

	return module;
}

void sqlrservermodules::unload() {

	// before list...
	// skip plugins shared with alist (eg. when="both" triggers)
	for (listnode< sqlrmoduleplugin * > *node=blist.getFirst();
						node; node=node->getNext()) {
		sqlrmoduleplugin	*sqlrmp=node->getValue();
		if (alist.find(sqlrmp)) {
			continue;
		}
		delete sqlrmp->m;
		delete sqlrmp->dl;
		delete sqlrmp;
	}

	// after list...
	for (listnode< sqlrmoduleplugin * > *node=alist.getFirst();
						node; node=node->getNext()) {
		sqlrmoduleplugin	*sqlrmp=node->getValue();
		delete sqlrmp->m;
		delete sqlrmp->dl;
		delete sqlrmp;
	}

	blist.clear();
	alist.clear();
}

void sqlrservermodules::endTransaction(bool commit) {

	// before list...
	// skip plugins shared with alist (eg. when="both" triggers)
	for (listnode< sqlrmoduleplugin * > *node=blist.getFirst();
						node; node=node->getNext()) {
		if (alist.find(node->getValue())) {
			continue;
		}
		node->getValue()->m->endTransaction(commit);
	}

	// after list...
	for (listnode< sqlrmoduleplugin * > *node=alist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endTransaction(commit);
	}
}

void sqlrservermodules::endSession() {

	// before list...
	// skip plugins shared with alist (eg. when="both" triggers)
	for (listnode< sqlrmoduleplugin * > *node=blist.getFirst();
						node; node=node->getNext()) {
		if (alist.find(node->getValue())) {
			continue;
		}
		node->getValue()->m->endSession();
	}

	// after list...
	for (listnode< sqlrmoduleplugin * > *node=alist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endSession();
	}
}
