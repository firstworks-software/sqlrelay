// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/stdio.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrdirectivedeclarations.cpp"
	}
#endif

class sqlrdirectivesprivate {
	friend class sqlrdirectives;
	private:
};

sqlrdirectives::sqlrdirectives(sqlrservercontroller *cont) :
						sqlrservermodules(cont) {
	pvt=new sqlrdirectivesprivate;
	setDebug(cont->getConfig()->getDebugDirectives());
}

sqlrdirectives::~sqlrdirectives() {
	unload();
	delete pvt;
}

bool sqlrdirectives::load(domnode *parameters) {

	unload();

	// run through the directive list
	for (domnode *directive=parameters->getFirstTagChild();
				!directive->isNullNode();
				directive=directive->getNextTagSibling()) {

		if (isModuleDisabled(directive)) {
			continue;
		}

		// load directive
		loadDirective(directive);
	}

	return true;
}

void sqlrdirectives::loadDirective(domnode *directive) {

	// ignore non-directives
	if (charstring::compare(directive->getName(),"directive")) {
		return;
	}

	// get the directive name
	const char	*module=directive->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=directive->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	debugWrite("loading directive module: %s",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the directive module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("directive_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load "
				"directive module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the directive itself
	stringbuffer	functionname;
	functionname.append("new_sqlrdirective_")->append(module);
	sqlrdirective *(*newDirective)(sqlrservercontroller *,
						domnode *)=
		(sqlrdirective *(*)(sqlrservercontroller *,
						domnode *))
				dl->getSymbol(functionname.getString());
	if (!newDirective) {
		stdoutput.printf("failed to load directive: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrdirective	*dr=(*newDirective)(cont,directive);

#else
	dynamiclib	*dl=NULL;
	sqlrdirective	*dr;
	#include "sqlrdirectiveassignments.cpp"
	{
		dr=NULL;
	}
#endif

	debugWrite("success");

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=dr;
	sqlrmp->dl=dl;
	sqlrmp->module=module;
	blist.append(sqlrmp);
}

bool sqlrdirectives::run(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char *query) {
	if (!query) {
		return false;
	}
	for (listnode< sqlrmoduleplugin * > *node=blist.getFirst();
						node; node=node->getNext()) {

		debugWrite("running directive: %s...",node->getValue()->module);

		sqlrdirective	*dr=(sqlrdirective *)node->getValue()->m;
		if (!dr->run(sqlrcon,sqlrcur,query)) {
			return false;
		}
	}
	return true;
}
