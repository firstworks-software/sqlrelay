// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/process.h>
#include <rudiments/character.h>
#include <rudiments/stdio.h>
//#define DEBUG_MESSAGES 1
#include <rudiments/debugprint.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrresultsetrowtranslationdeclarations.cpp"
	}
#endif

class sqlrresultsetrowtranslationsprivate {
	friend class sqlrresultsetrowtranslations;
	private:
		bool		_debug;

		singlylinkedlist< sqlrmoduleplugin * >	_tlist;

		const char	*_error;
};

sqlrresultsetrowtranslations::sqlrresultsetrowtranslations(
						sqlrservercontroller *cont) :
						sqlrservermodules(cont) {
	debugFunction();
	pvt=new sqlrresultsetrowtranslationsprivate;
	pvt->_debug=cont->getConfig()->getDebugResultSetRowTranslations();
	pvt->_error=NULL;
}

sqlrresultsetrowtranslations::~sqlrresultsetrowtranslations() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrresultsetrowtranslations::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the result set translation list
	for (domnode *resultsetrowtranslation=parameters->getFirstTagChild();
			!resultsetrowtranslation->isNullNode();
			resultsetrowtranslation=
				resultsetrowtranslation->getNextTagSibling()) {

		if (isModuleDisabled(resultsetrowtranslation)) {
			continue;
		}

		// load result set translation
		loadResultSetRowTranslation(resultsetrowtranslation);
	}

	return true;
}

void sqlrresultsetrowtranslations::unload() {
	debugFunction();
	for (listnode< sqlrmoduleplugin * > *node=pvt->_tlist.getFirst();
						node; node=node->getNext()) {
		sqlrmoduleplugin	*sqlrmp=node->getValue();
		delete sqlrmp->m;
		delete sqlrmp->dl;
		delete sqlrmp;
	}
	pvt->_tlist.clear();
}

void sqlrresultsetrowtranslations::loadResultSetRowTranslation(
				domnode *resultsetrowtranslation) {
	debugFunction();

	// ignore non-resultsetrowtranslations
	if (charstring::compare(resultsetrowtranslation->getName(),
						"resultsetrowtranslation")) {
		return;
	}

	// get the result set translation name
	const char	*module=
			resultsetrowtranslation->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=resultsetrowtranslation->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	if (pvt->_debug) {
		stdoutput.printf("loading result set row translation: %s\n",module);
	}

#ifdef SQLRELAY_ENABLE_SHARED
	// load the result set translation module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("resultsetrowtranslation_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load "
				"result set row translation module: %s\n",
				module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the result set translation itself
	stringbuffer	functionname;
	functionname.append("new_sqlrresultsetrowtranslation_")->append(module);
	sqlrresultsetrowtranslation *(*newResultSetTranslation)
					(sqlrservercontroller *,
					domnode *)=
		(sqlrresultsetrowtranslation *(*)
					(sqlrservercontroller *,
					domnode *))
				dl->getSymbol(functionname.getString());
	if (!newResultSetTranslation) {
		stdoutput.printf("failed to load "
				"result set row translation: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrresultsetrowtranslation	*rstr=
		(*newResultSetTranslation)(cont,resultsetrowtranslation);

#else
	dynamiclib			*dl=NULL;
	sqlrresultsetrowtranslation	*rstr;
	#include "sqlrresultsetrowtranslationassignments.cpp"
	{
		rstr=NULL;
	}
#endif

	if (pvt->_debug) {
		stdoutput.printf("success\n");
	}

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=rstr;
	sqlrmp->dl=dl;
	sqlrmp->module=module;
	pvt->_tlist.append(sqlrmp);
}

bool sqlrresultsetrowtranslations::run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur,
						uint32_t colcount,
						const char * const *fieldnames,
						const char ***fields,
						uint64_t **fieldsizes) {
	debugFunction();

	pvt->_error=NULL;

	for (listnode< sqlrmoduleplugin * > *node=pvt->_tlist.getFirst();
						node; node=node->getNext()) {
		if (pvt->_debug) {
			stdoutput.printf("\nrunning translation:  %s...\n\n",
						node->getValue()->module);
		}

		sqlrresultsetrowtranslation	*rstr=
			(sqlrresultsetrowtranslation *)node->getValue()->m;
		if (!rstr->run(sqlrcon,sqlrcur,
					colcount,fieldnames,
					fields,fieldsizes)) {
			pvt->_error=rstr->getError();
			return false;
		}
	}
	return true;
}

const char *sqlrresultsetrowtranslations::getError() {
	return pvt->_error;
}

void sqlrresultsetrowtranslations::endTransaction(bool commit) {
	for (listnode< sqlrmoduleplugin * > *node=pvt->_tlist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endTransaction(commit);
	}
}

void sqlrresultsetrowtranslations::endSession() {
	for (listnode< sqlrmoduleplugin * > *node=pvt->_tlist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endSession();
	}
}
