// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/domnode.h>
#include <rudiments/stdio.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrtriggerdeclarations.cpp"
	}
#endif

sqlrtriggers::sqlrtriggers(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrservermodules(cont,parameters) {
	pvt=NULL;
	setDebug(cont->getConfig()->getDebugTriggers());
}

sqlrtriggers::~sqlrtriggers() {
}

bool sqlrtriggers::load() {

	unload();

	// run through the module list
	for (domnode *moduledata=getParameters()->getFirstTagChild();
				!moduledata->isNullNode();
				moduledata=moduledata->getNextTagSibling()) {

		// skip disabled modules
		if (isModuleDisabled(moduledata)) {
			continue;
		}

		// parse "when"...
		bool	beforeexecute=false;
		bool	afterexecute=false;
		bool	beforeprepare=false;
		bool	afterprepare=false;
		const char	*when=moduledata->getAttributeValue("when");
		if (!charstring::isNullOrEmpty(when)) {
			char		**tokens=NULL;
			uint64_t	tokencount=0;
			charstring::split(when,",",true,&tokens,&tokencount);
			for (uint64_t i=0; i<tokencount; i++) {
				charstring::strip(tokens[i],' ');
				charstring::strip(tokens[i],'\t');
				const char	*t=tokens[i];
				if (!charstring::compareIgnoringCase(
							t,"beforeexecute") ||
					// for back-compatibility
					!charstring::compareIgnoringCase(
							t,"before")) {
					beforeexecute=true;
				} else if (!charstring::compareIgnoringCase(
							t,"afterexecute") ||
					// for back-compatibility
					!charstring::compareIgnoringCase(
							t,"after")) {
					afterexecute=true;
				} else if (!charstring::compareIgnoringCase(
							t,"bothexecute") ||
					// for back-compatibility
					!charstring::compareIgnoringCase(
							t,"both")) {
					beforeexecute=true;
					afterexecute=true;
				} else if (!charstring::compareIgnoringCase(
							t,"beforeprepare")) {
					beforeprepare=true;
				} else if (!charstring::compareIgnoringCase(
							t,"afterprepare")) {
					afterprepare=true;
				} else if (!charstring::compareIgnoringCase(
							t,"bothprepare")) {
					beforeprepare=true;
					afterprepare=true;
				} else if (!charstring::compareIgnoringCase(
							t,"all")) {
					beforeprepare=true;
					afterprepare=true;
					beforeexecute=true;
					afterexecute=true;
				}
				delete[] tokens[i];
			}
			delete[] tokens;
		}

		// load the trigger
		sqlrmoduleplugin	*p;
		loadModule(moduledata,&p);
		if (!p) {
			continue;
		}

		// add trigger to before-prepare list
		if (beforeprepare) {
			debugWrite("before-prepare trigger");
			bplist.append(p);
		}

		// add trigger to after-prepare list
		if (afterprepare) {
			debugWrite("after-prepare trigger");
			aplist.append(p);
		}

		// add trigger to before-execute list
		if (beforeexecute) {
			debugWrite("before-execute trigger");
			blist.append(p);
		}

		// add trigger to after-execute list
		if (afterexecute) {
			debugWrite("after-execute trigger");
			alist.append(p);
		}
	}
	return true;
}

void sqlrtriggers::loadModule(domnode *parameters, sqlrmoduleplugin **plugin) {

	*plugin=NULL;

	// ignore non-triggers
	if (charstring::compare(parameters->getName(),"trigger")) {
		return;
	}

	// get the module name
	const char	*module=getModuleName(parameters);
	if (charstring::isNullOrEmpty(module)) {
		return;
	}

	debugWrite("loading trigger: %s",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the trigger module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("trigger_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load trigger module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the trigger itself
	stringbuffer	functionname;
	functionname.append("new_sqlrtrigger_")->append(module);
	sqlrtrigger *(*newTrigger)(sqlrservercontroller *,
						domnode *)=
			(sqlrtrigger *(*)(sqlrservercontroller *,
						domnode *))
				dl->getSymbol(functionname.getString());
	if (!newTrigger) {
		stdoutput.printf("failed to load trigger: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrtrigger	*tr=(*newTrigger)(cont,parameters);

#else

	dynamiclib	*dl=NULL;
	sqlrtrigger	*tr;
	#include "sqlrtriggerassignments.cpp"
	{
		tr=NULL;
	}
#endif

	debugWrite("success");

	// build and return the plugin
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=tr;
	sqlrmp->dl=dl;
	sqlrmp->module=module;
	*plugin=sqlrmp;
	return;
}

bool sqlrtriggers::runBeforePrepareTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {
	return runBeforePrepare(sqlrcon,sqlrcur,&bplist);
}

bool sqlrtriggers::runAfterPrepareTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {
	return runAfterPrepare(sqlrcon,sqlrcur,&aplist);
}

bool sqlrtriggers::runBeforeExecuteTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {
	return runBeforeExecute(sqlrcon,sqlrcur,&blist);
}

bool sqlrtriggers::runAfterExecuteTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {
	return runAfterExecute(sqlrcon,sqlrcur,&alist);
}

bool sqlrtriggers::runBeforePrepare(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list) {
	for (listnode< sqlrmoduleplugin * > *node=list->getFirst();
						node; node=node->getNext()) {

		debugWrite("running before-prepare trigger: %s...",
					node->getValue()->module);

		sqlrtrigger	*tr=(sqlrtrigger *)node->getValue()->m;
		if (!tr->runBeforePrepare(sqlrcon,sqlrcur)) {
			return false;
		}
	}
	return true;
}

bool sqlrtriggers::runAfterPrepare(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list) {
	for (listnode< sqlrmoduleplugin * > *node=list->getFirst();
						node; node=node->getNext()) {

		debugWrite("running after-prepare trigger: %s...",
					node->getValue()->module);

		sqlrtrigger	*tr=(sqlrtrigger *)node->getValue()->m;
		if (!tr->runAfterPrepare(sqlrcon,sqlrcur)) {
			return false;
		}
	}
	return true;
}

bool sqlrtriggers::runBeforeExecute(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list) {
	for (listnode< sqlrmoduleplugin * > *node=list->getFirst();
						node; node=node->getNext()) {

		debugWrite("running before-execute trigger: %s...",
					node->getValue()->module);

		sqlrtrigger	*tr=(sqlrtrigger *)node->getValue()->m;
		if (!tr->runBeforeExecute(sqlrcon,sqlrcur)) {
			return false;
		}
	}
	return true;
}

bool sqlrtriggers::runAfterExecute(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrmoduleplugin * > *list) {
	for (listnode< sqlrmoduleplugin * > *node=list->getFirst();
						node; node=node->getNext()) {

		debugWrite("running after-execute trigger: %s...",
					node->getValue()->module);

		sqlrtrigger	*tr=(sqlrtrigger *)node->getValue()->m;
		if (!tr->runAfterExecute(sqlrcon,sqlrcur)) {
			return false;
		}
	}
	return true;
}
