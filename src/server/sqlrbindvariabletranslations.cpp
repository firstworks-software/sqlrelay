// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/process.h>
#include <rudiments/character.h>
#include <rudiments/stdio.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrbindvariabletranslationdeclarations.cpp"
	}
#endif

class sqlrbindvariabletranslationsprivate {
	friend class sqlrbindvariabletranslations;
	private:
		const char	*_error;
};

sqlrbindvariabletranslations::sqlrbindvariabletranslations(
					sqlrservercontroller *cont) :
					sqlrservermodules(cont) {
	pvt=new sqlrbindvariabletranslationsprivate;
	setDebug(cont->getConfig()->getDebugBindVariableTranslations());
	pvt->_error=NULL;
}

sqlrbindvariabletranslations::~sqlrbindvariabletranslations() {
	unload();
	delete pvt;
}

bool sqlrbindvariabletranslations::load(domnode *parameters) {

	unload();

	// run through the bind variable translation list
	for (domnode *bindvariabletranslation=parameters->getFirstTagChild();
			!bindvariabletranslation->isNullNode();
			bindvariabletranslation=
				bindvariabletranslation->getNextTagSibling()) {

		if (isModuleDisabled(bindvariabletranslation)) {
			continue;
		}

		// load bind variable translation
		loadBindVariableTranslation(bindvariabletranslation);
	}

	return true;
}

void sqlrbindvariabletranslations::loadBindVariableTranslation(
					domnode *bindvariabletranslation) {

	// ignore non-bindvariabletranslations
	if (charstring::compare(bindvariabletranslation->getName(),
						"bindvariabletranslation")) {
		return;
	}

	// get the bind variable translation name
	const char	*module=
			bindvariabletranslation->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=bindvariabletranslation->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	debugWrite("loading bind variable translation module: %s",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the bind variable translation module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("bindvariabletranslation_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load bind variable "
					"translation module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the bind variable translation itself
	stringbuffer	functionname;
	functionname.append("new_sqlrbindvariabletranslation_")->append(module);
	sqlrbindvariabletranslation *(*newBindVariableTranslation)
					(sqlrservercontroller *,
					domnode *)=
		(sqlrbindvariabletranslation *(*)
					(sqlrservercontroller *,
					domnode *))
				dl->getSymbol(functionname.getString());
	if (!newBindVariableTranslation) {
		stdoutput.printf("failed to load "
				"bind variable translation: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrbindvariabletranslation	*bvtr=
		(*newBindVariableTranslation)
			(cont,bindvariabletranslation);

#else
	dynamiclib			*dl=NULL;
	sqlrbindvariabletranslation	*bvtr;
	#include "sqlrbindvariabletranslationassignments.cpp"
	{
		bvtr=NULL;
	}
#endif

	debugWrite("success");

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrrstp=new sqlrmoduleplugin;
	sqlrrstp->m=bvtr;
	sqlrrstp->dl=dl;
	sqlrrstp->module=module;
	blist.append(sqlrrstp);
}

bool sqlrbindvariabletranslations::run(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {

	pvt->_error=NULL;

	for (listnode< sqlrmoduleplugin * > *node=blist.getFirst();
						node; node=node->getNext()) {

		debugWrite("running bind variable translation: %s...",
						node->getValue()->module);

		sqlrbindvariabletranslation	*bvtr=
			(sqlrbindvariabletranslation *)node->getValue()->m;
		if (!bvtr->run(sqlrcon,sqlrcur)) {
			pvt->_error=bvtr->getError();
			return false;
		}
	}
	return true;
}

const char *sqlrbindvariabletranslations::getError() {
	return pvt->_error;
}
